#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>

#include <mach/map.h>

#undef S3C_VA_WATCHDOG
#define S3C_VA_WATCHDOG (0)

#include <plat/regs-watchdog.h>

#define PFX "s3c2410-wdt: "

#define CONFIG_S3C2410_WATCHDOG_ATBOOT    (0)
#define CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME (15)

static int nowayout = WATCHDOG_NOWAYOUT;
static int tmr_margin = CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME;
static int tmr_atboot = CONFIG_S3C2410_WATCHDOG_ATBOOT;
static int soft_noboot;
static int debug;

module_param(tmr_margin,  int, 0);
module_param(tmr_atboot,  int, 0);
module_param(nowayout,    int, 0);
module_param(soft_noboot, int, 0);
module_param(debug,       int, 0);

MODULE_PARM_DESC(tmr_margin, 
		"Watchdog tmr_margin in seconds, (default="
			__MODULE_STRING(CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME) ")" );
MODULE_PARM_DESC(tmr_atboot,
		"Watchdog is started at boot time if set 1, (default="
			__MODULE_STRING(CONFIG_S3C2410_WATCHDOG_ATBOOT) ")" );
MODULE_PARM_DESC(nowayout,
		"Watchdog cannot be stopped once start, (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")" );
MODULE_PARM_DESC(soft_noboot,
		"Watchdog action, set to 1 to ignore reboots,"
		"0 to reboot (default depends on ONLY_TESTING)");
MODULE_PARM_DESC(debug,
		"Watchdog debug, set to >1 for debug, (default 0)");

static unsigned long open_lock;
static struct device   *wdt_dev; /* platform device attached to */
static struct resource *wdt_mem;
static struct resource *wdt_irq;
static struct clk      *wdt_clock;
static void __iomem    *wdt_base;
static unsigned int     wdt_count;
static char  expect_close;
static DEFINE_SPINLOCK(wdt_lock);

/* Watchdog control routines */

#define DBG(msg...) do {\
	if (debug) \
		printk(KERN_INFO msg);\
	} while (0)

/* functions */

static void watchdog_keepalive(void)
{
	spin_lock(&wdt_lock);
	writel(wdt_count, wdt_base + S3C2410_WTCNT);
	spin_unlock(&wdt_lock);
}

static void __watchdog_stop(void)
{
	unsigned long wtcon;

	wtcon = readl(wdt_base + S3C2410_WTCON);
	wtcon &= ~(S3C2410_WTCON_ENABLE | S3C2410_WTCON_RSTEN);
	writel(wtcon, wdt_base + S3C2410_WTCON);
}

static void watchdog_stop(void)
{
	spin_lock(&wdt_lock);
	__watchdog_stop();
	spin_unlock(&wdt_lock);
}

static void watchdog_start(void)
{
	unsigned long wtcon;

	spin_lock(&wdt_lock);
	__watchdog_stop();

	wtcon = readl(wdt_base + S3C2410_WTCON);
	wtcon |= S3C2410_WTCON_ENABLE | S3C2410_WTCON_DIV128;

	if (soft_noboot) {
		wtcon |= S3C2410_WTCON_INTEN;
		wtcon &= ~S3C2410_WTCON_RSTEN;
	} else {
		wtcon &= ~S3C2410_WTCON_INTEN;
		wtcon |= S3C2410_WTCON_RSTEN;
	}

	DBG("%s: wtd_cont=0x%08x, wtcon=%08lx\n", __func__, wdt_count, wtcon);

	writel(wdt_count, wdt_base + S3C2410_WTDAT);
	writel(wdt_count, wdt_base + S3C2410_WTCNT);
	writel(wtcon, wdt_base + S3C2410_WTCON);
	spin_unlock(&wdt_lock);
}

static int watchdog_set_heartbeat(int timeout)
{
	unsigned int freq = clk_get_rate(wdt_clock);
	unsigned int count;
	unsigned int divisor = 1;
	unsigned long wtcon;

	if (timeout < 1)
		return -EINVAL;

	freq /= 128;
	count = timeout * freq;

	DBG("%s: count=%d, timeout=%d, freq=%d",
		__func__, count, timeout, freq);

	/*
	 * If the count is bigger than the watchdog register,
	 * then work out what we need to do (and if) we can
	 * actually make this value
	 */
	if (count >= 0x10000) {
		for (divisor = 1; divisor <= 0x100; divisor++) {
			if ((count / divisor) < 0x10000)
				break;
		}

		if ((count / divisor) >= 0x10000) {
			dev_err(wdt_dev, "timeout %d too big\n", timeout);
			return -EINVAL;
		}
	}

	tmr_margin = timeout;

	DBG("%s: timeout=%d, divisor=%d, count=%d (%08x)\n",
		__func__, timeout, divisor, count, count / divisor);

	count /= divisor;
	wdt_count = count;


	/* Update the pre-scaler */
	wtcon = readl(wdt_base + S3C2410_WTCON);
	wtcon &= ~S3C2410_WTCON_PRESCALE_MASK;
	wtcon |= S3C2410_WTCON_PRESCALE(divisor - 1);

	writel(count, wdt_base + S3C2410_WTDAT);
	writel(wtcon, wdt_base + S3C2410_WTCON);

	return 0;
}

/*
 * /dev/watchdog handling
 */
static int watchdog_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &open_lock))
		return -EBUSY;

	if (nowayout)
		__module_get(THIS_MODULE);

	expect_close = 0;

	/* Start the timer */
	watchdog_start();
	return nonseekable_open(inode, file);
}

static int watchdog_release(struct inode *inode, struct file *file)
{
	/*
	 * Shut off the timer.
	 * Lock it in if it's a module and we set nowayout
	 */
	if (expect_close == 42)
		watchdog_stop();
	else {
		dev_err(wdt_dev, "Unexpected close, not stopping watchdog\n");
		watchdog_keepalive();
	}

	expect_close = 0;
	clear_bit(0, &open_lock);

	return 0;
}

static ssize_t watchdog_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	/*
	 * Refresh the timer.
	 */
	if (len) {
		if (!nowayout) {
			size_t i;

			/* In case it was set long ago */
			expect_close = 0;

			for (i = 0; i != len; i++) {
				char c;

				if (get_user(c, data + i))
					return -EFAULT;

				if (c == 'V')
					expect_close = 42;
			}
		}

		watchdog_keepalive();
	}
	return len;
}

#define OPTIONS (WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE)

static struct watchdog_info watchdog_ident = {
	.options    = OPTIONS,
	.firmware_version = 0,
	.identity   = "mini2440 Watchdog",
};

static long watchdog_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int  __user *p = argp;
	int new_margin;

	switch (cmd) {
		case WDIOC_GETSUPPORT:
			return copy_to_user(argp, &watchdog_ident, sizeof(watchdog_ident)) ? -EFAULT:0;
		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			return put_user(0, p);
		case WDIOC_KEEPALIVE:
			watchdog_keepalive();
			return 0;
		case WDIOC_SETTIMEOUT:
			if (get_user(new_margin, p))
				return -EFAULT;
			if (watchdog_set_heartbeat(new_margin))
				return -EINVAL;
			watchdog_keepalive();
			return put_user(tmr_margin, p);
		case WDIOC_GETTIMEOUT:
			return put_user(tmr_margin, p);
		default:
			return -ENOTTY;
	}
}

/* Kernel interface */

static struct file_operations watchdog_fops = {
	.owner   = THIS_MODULE,
	.llseek  = no_llseek,
	.write   = watchdog_write,
	.unlocked_ioctl = watchdog_ioctl,
	.open    = watchdog_open,
	.release = watchdog_release,
};

static struct miscdevice watchdog_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name  = "watchdog",
	.fops  = &watchdog_fops,
};

/* Interrupt handler code */

static irqreturn_t watchdog_irq(int irqno, void *param)
{
	dev_info(wdt_dev, "Watchdog timer expired (irq)\n");
	watchdog_keepalive();

	return IRQ_HANDLED;
}

/* Device interface */

static int __devinit watchdog_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device  *dev;
	unsigned int    wtcon;
	int started = 0;
	int ret;
	int size;

	DBG("%s: probe=%p\n", __func__, pdev);

	dev = &pdev->dev;
	wdt_dev = &pdev->dev;

	/* Get the memory region for the watchdog timer */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(dev, "No memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;
	wdt_mem = request_mem_region(res->start, size, pdev->name);
	if (wdt_mem == NULL) {
		dev_err(dev, "Failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}

	wdt_base = ioremap(res->start, size);
	if (wdt_base == NULL) {
		dev_err(dev, "Failed to ioremap() region\n");
		ret = -EINVAL;
		goto err_req;
	}

	DBG("probe: mapped wdt_base=%p\n", wdt_base);

	wdt_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (wdt_irq == NULL) {
		dev_err(dev, "No irq resource specified\n");
		ret = -ENOENT;
		goto err_map;
	}

	ret = request_irq(wdt_irq->start, watchdog_irq, 0, pdev->name, pdev);
	if (ret != 0) {
		dev_err(dev, "Failed to install irq (%d)\n", ret);
		goto err_map;
	}

	wdt_clock = clk_get(&pdev->dev, "watchdog");
	if (IS_ERR(wdt_clock)) {
		dev_err(dev, "Failed to find watchdog clock source\n");
		ret = PTR_ERR(wdt_clock);
		goto err_irq;
	}

	clk_enable(wdt_clock);
	/*
	 * See if we can actually set the requested timer margin, and if
	 * not, try the default value.
	 */
	if (watchdog_set_heartbeat(tmr_margin)) {
		started = watchdog_set_heartbeat(CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME);
		if (started == 0)
			dev_info(dev, 
					 	"tmr_margin value out of range, default %d used\n",
							CONFIG_S3C2410_WATCHDOG_DEFAULT_TIME);
		else
			dev_info(dev,
						"default timer value is out of range, cannot start\n");
	}

	ret = misc_register(&watchdog_miscdev);
	if (ret) {
		dev_err(dev, "Cannot register miscdev on minor=%d (%d)\n", WATCHDOG_MINOR, ret);
		goto err_clk;
	}

	if (tmr_atboot && started == 0) {
		dev_info(dev, "Starting watchdog timer\n");
		watchdog_start();
	} else if(!tmr_atboot) {
		/*
		 * If we're not enabling the watchdog, then ensure it is
		 * disabled if it has been left running from the bootloadr
		 * or other source
		 */
		watchdog_stop();
	}

	/* Print out a statement of readiness */
	wtcon = readl(wdt_base + S3C2410_WTCON);

	dev_info(dev, "Watchdog %sactive, reset %sabled, irq %sabled\n",
		(wtcon & S3C2410_WTCON_ENABLE) ? "" : "in",
		(wtcon & S3C2410_WTCON_RSTEN)  ? "" : "dis",
		(wtcon & S3C2410_WTCON_INTEN)  ? "" : "en");

	return 0;
	
err_clk:
	clk_disable(wdt_clock);
	clk_put(wdt_clock);

err_irq:
	free_irq(wdt_irq->start, pdev);

err_map:
	iounmap(wdt_base);

err_req:
	release_resource(wdt_mem);
	kfree(wdt_mem);

	return ret;
}

static int __devexit watchdog_remove(struct platform_device *dev)
{
	release_resource(wdt_mem);
	kfree(wdt_mem);
	wdt_mem = NULL;

	free_irq(wdt_irq->start, dev);
	wdt_irq = NULL;

	clk_disable(wdt_clock);
	clk_put(wdt_clock);
	wdt_clock = NULL;

	iounmap(wdt_base);
	misc_deregister(&watchdog_miscdev);

	return 0;
}

static void watchdog_shutdown(struct platform_device *dev)
{
	watchdog_stop();
}

#ifdef CONFIG_PM

static unsigned long wtcon_save;
static unsigned long wtdat_save;

static int watchdog_suspend(struct platform_device *dev, pm_message_t state)
{
	/* Save watchdog state, and turn it off .*/
	wtcon_save = readl(wdt_base + S3C2410_WTCON);
	wtdat_save = readl(wdt_base + S3C2410_WTDAT);

	/* Note that WTCNT doesn't need to be saved */
	watchdog_stop();

	return 0;
}

static int watchdog_resume(struct platform_device *dev)
{
	/* Restore watchdog state. */
	writel(wtdat_save, wdt_base + S3C2410_WTDAT);
	writel(wtdat_save, wtd_base + S3C2410_WTCNT); /* Reset count */
	writel(wtcon_save, wdt_base + S3C2410_WTCON);

	printk(KERN_INFO PFX "Watchdog %sabled\n", (wtcon_save & S3C2410_WTCON_ENABLE) ? "en" : "dis");

	return 0;
}

#else
#define watchdog_suspend NULL
#define watchdog_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver watchdog_driver = {
	.probe  = watchdog_probe,
	.remove = __devexit_p(watchdog_remove),
	.shutdown = watchdog_shutdown,
	.suspend  = watchdog_suspend,
	.resume   = watchdog_resume,
	.driver   = {
		.owner = THIS_MODULE,
		.name  = "s3c2440-wdt",
	},
};

static char banner[] __initdata = 
	KERN_INFO "mini2440 Watchdog Timer, (c) 2014 Harvis Wang\n";

static int __init watchdog_module_init(void)
{
	printk(banner);
	return platform_driver_register(&watchdog_driver);
}

static void __exit watchdog_module_exit(void)
{
	platform_driver_unregister(&watchdog_driver);
}

module_init(watchdog_module_init);
module_exit(watchdog_module_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Harvis Wang <jiankangshiye@aliyun.com>");
MODULE_DESCRIPTION("mini2440 Watchdog Device Driver");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:s3c2410-wdt");



