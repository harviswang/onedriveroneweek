#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/time.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>

#include <plat/regs-timer.h>

#define DEVICE_NAME "mini2440-pwm"

#define PWM_IOCTL_SET_FREQ 	1
#define PWM_IOCTL_STOP		0

static struct semaphore lock;

/*
 * freq: pclk/50/16/65536 ~ pclk/50/16
 * if pclk = 50MHz, freq is 1Hz to 62500Hz
 * human ear: 20Hz ~ 20000Hz
 */
static void PWM_set_freq(unsigned long freq)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg0;
	unsigned long tcfg1;

	struct clk *clk_p;
	unsigned long pclk;

	/* set GPB0 as tout0, pwn output */
	s3c2410_gpio_cfgpin(S3C2410_GPB(0), S3C2410_GPB0_TOUT0);

	tcon  = __raw_readl(S3C2410_TCON);
	tcfg1 = __raw_readl(S3C2410_TCFG1); /* Must read tcfg1 first */
	tcfg0 = __raw_readl(S3C2410_TCFG0);

	/* prescaler = 50 */
	tcfg0 &= ~S3C2410_TCFG_PRESCALER0_MASK;
	tcfg0 |= (50 - 1);

	/* mux = 1/16 */
	tcfg1 &= ~S3C2410_TCFG1_MUX0_MASK;
	tcfg1 |= S3C2410_TCFG1_MUX0_DIV16;

	/* Must write tcfg1 first or will be an error */
	__raw_writel(tcfg1, S3C2410_TCFG1);
	__raw_writel(tcfg0, S3C2410_TCFG0);

	clk_p = clk_get(NULL, "pclk");
	pclk  = clk_get_rate(clk_p);
	tcnt  = (pclk/50/16)/freq;

	__raw_writel(tcnt,   S3C2410_TCNTB(0));
	__raw_writel(tcnt/2, S3C2410_TCMPB(0));

	/* disable deadzone, auto-reload, inv-off, update TCNTB0&TCMPB0 */
	tcon &= ~0x1f;
	tcon |= 0xb;
	__raw_writel(tcon, S3C2410_TCON);

	tcon &= ~2; /* clear manual update bit */
	__raw_writel(tcon, S3C2410_TCON);
}

static void PWM_stop(void)
{
	s3c2410_gpio_cfgpin(S3C2410_GPB(0), S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_setpin(S3C2410_GPB(0), 0);
}

static int PWM_open(struct inode *inode, struct file *file)
{
	if (!down_trylock(&lock))
		return 0;
	else
		return -EBUSY;
}

static int PWM_close(struct inode *inode, struct file *file)
{
	PWM_stop();
	up(&lock);

	return 0;
}

static int PWM_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("ioctl pwd: %x %lx\n", cmd, arg);
	switch (cmd) {
		case PWM_IOCTL_SET_FREQ:
			if (arg == 0)
				return -EINVAL;

			PWM_set_freq(arg);
			break;

		case PWM_IOCTL_STOP:
			PWM_stop();
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static struct file_operations dev_fops = {
	.owner  = THIS_MODULE,
	.open   = PWM_open,
	.release= PWM_close,
	.ioctl  = PWM_ioctl,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &dev_fops,
};

static int __init PWM_module_init(void)
{
	int ret;

	init_MUTEX(&lock);
	ret = misc_register(&misc);
	printk(DEVICE_NAME"\tinitialized!\n");

	return ret;
}

static void __exit PWM_module_exit(void)
{
	misc_deregister(&misc);
}

module_init(PWM_module_init);
module_exit(PWM_module_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Harvis Wang<jiankangshiey@aliyun.com>");

