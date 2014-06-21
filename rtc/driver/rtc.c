#include <linux/bcd.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>

#include <plat/regs-rtc.h>

#include <asm/io.h>
#include <asm/irq.h>

static void rtc_enable(struct platform_device *pdev, int flag);
static int rtc_setfreq(struct device *dev, int freq);
static int __devexit rtc_remove(struct platform_device *pdev);
#ifdef CONFIG_PM
static int rtc_suspend(struct platform_device *pdev, pm_message_t state);
static int rtc_resume(struct platform_device *pdev);
#else
#define rtc_suspend NULL
#define rtc_resume  NULL
#endif

static int rtc_open(struct device *dev);
static void rtc_release(struct device *dev);
static int rtc_setpie(struct device *dev, int flag);
static void rtc_setaie(int flag);
static int rtc_gettime(struct device *dev, struct rtc_time *rtc_tm);
static int rtc_settime(struct device *dev, struct rtc_time *tm);
static int rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm);
static int rtc_setalarm(struct device *dev, struct rtc_wkalrm *alarm);
static irqreturn_t rtc_alarmirq(int irq, void *dev_id);
static irqreturn_t rtc_tickirq(int irq, void *dev_id);

static const struct rtc_class_ops rtc_fops;
static struct resource *rtc_mem;
static void   __iomem  *rtc_base;

static int rtc_alarmno = NO_IRQ;
static int rtc_tickno  = NO_IRQ;

static DEFINE_SPINLOCK(rtc_pie_lock);

static int __devinit rtc_probe(struct platform_device *pdev)
{
	int ret;
	struct rtc_device *rtc;
	struct resource   *res;

	rtc_alarmno = platform_get_irq(pdev, 0);
	if (rtc_alarmno < 0) {
		dev_err(&pdev->dev, "No irq for alarm\n");
		return -ENOENT;
	}

	rtc_tickno = platform_get_irq(pdev, 1);
	if (rtc_tickno < 0) {
		dev_err(&pdev->dev, "No irq for rtc tick\n");
		return -ENOENT;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Failed to get memory region resource\n");
		return -ENOENT;
	}

	rtc_mem = request_mem_region(res->start, res->end - res->start + 1, pdev->name);
	if (rtc_mem == NULL) {
		dev_err(&pdev->dev, "Failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_no_res;
	}

	rtc_base = ioremap(res->start, res->end - res->start + 1);
	if (rtc_base == NULL) {
		dev_err(&pdev->dev, "Failed ioremap()\n");
		ret = -EINVAL;
		goto err_nomap;
	}

	rtc_enable(pdev, 1);
	rtc_setfreq(&pdev->dev, 1);

	device_init_wakeup(&pdev->dev, 1);

	rtc = rtc_device_register("my2440", &pdev->dev, &rtc_fops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "Cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_nortc;
	}

	rtc->max_user_freq = 128;

	platform_set_drvdata(pdev, rtc);

	return 0;

err_nortc:
	rtc_enable(pdev, 0);
	iounmap(rtc_base);

err_nomap:
	release_resource(rtc_mem);

err_no_res:
	return ret;
}

/*
 * flag: =0, before turn off power
 *       =1, after system reset
 */
static void rtc_enable(struct platform_device *pdev, int flag)
{
	unsigned int tmp;

	if (!flag) {
		tmp = readb(rtc_base + S3C2410_RTCCON);
		writeb(tmp & ~S3C2410_RTCCON_RTCEN, rtc_base + S3C2410_RTCCON);

		tmp = readb(rtc_base + S3C2410_TICNT);
		writeb(tmp & ~S3C2410_TICNT_ENABLE, rtc_base + S3C2410_TICNT);
	} else {
		tmp = readb(rtc_base + S3C2410_RTCCON);
		if ((tmp & S3C2410_RTCCON_RTCEN) == 0) {
			dev_info(&pdev->dev, "RTC disabled, re-enable\n");
			tmp = readb(rtc_base + S3C2410_RTCCON);
			writeb(tmp | S3C2410_RTCCON_RTCEN, rtc_base + S3C2410_RTCCON);
		}

		tmp = readb(rtc_base + S3C2410_RTCCON);
		if (tmp & S3C2410_RTCCON_CNTSEL) {
			dev_info(&pdev->dev, "Removing RTCCON_CNTSEL\n");
			tmp = readb(rtc_base + S3C2410_RTCCON);
			writeb(tmp & ~S3C2410_RTCCON, rtc_base + S3C2410_RTCCON);
		}

		tmp = readb(rtc_base + S3C2410_RTCCON);
		if (tmp & S3C2410_RTCCON_CLKRST) {
			dev_info(&pdev->dev, "Removing RTCCON_CLKRST\n");
			tmp = readb(rtc_base + S3C2410_RTCCON);
			writeb(tmp & ~S3C2410_RTCCON_CLKRST, rtc_base + S3C2410_RTCCON);
		}
	}
}

static int rtc_setfreq(struct device *dev, int freq)
{
	unsigned int tmp;

	if (!is_power_of_2(freq))
		return -EINVAL;

	spin_lock_irq(&rtc_pie_lock);

	tmp = readb(rtc_base + S3C2410_TICNT);
	tmp &= S3C2410_TICNT_ENABLE;

	tmp |= (128 / freq) - 1;

	writeb(tmp, rtc_base + S3C2410_TICNT);
	spin_unlock_irq(&rtc_pie_lock);

	return 0;
}



	
static struct platform_driver rtc_driver = {
	.probe  = rtc_probe,
	.remove = __devexit_p(rtc_remove),
	.suspend= rtc_suspend,
	.resume = rtc_resume,
	.driver = {
		.name  = "s3c2410-rtc",
		.owner = THIS_MODULE,
	},
};

static int __init rtc_module_init(void)
{
	return platform_driver_register(&rtc_driver);
}

static void __exit rtc_module_exit(void)
{
	platform_driver_unregister(&rtc_driver);
}

module_init(rtc_module_init);
module_exit(rtc_module_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Harvis Wang <jiankangshiye@aliyun.com>");
MODULE_DESCRIPTION("MINI2330 RTC Driver");

#if 0
struct platform_device s3c_device_rtc = {
	.name = "s3c2410-rtc",
	.id   = -1,
	.num_resources = ARRAY_SIZE(s3c_rtc_resource),
	.resource = s3c_rtc_resource,
};
EXPORT_SYMBOL(s3c_device_rtc);

static struct platform_device *smdk2440_devices[] __init_data = {
	&s3c_device_rtc,
};

static void __init smdk2440_machine_init(void)
{
	platform_add_devices(smdk2440_devices, ARRAY_SIZE(smdk2440_devices));
}
#endif

static const struct rtc_class_ops rtc_fops = {
	.open    = rtc_open,
	.release = rtc_release,
	.irq_set_freq  = rtc_setfreq,
	.irq_set_state = rtc_setpie,
	.read_time = rtc_gettime,
	.set_time  = rtc_settime,
	.read_alarm = rtc_getalarm,
	.set_alarm  = rtc_setalarm,
};

static int rtc_open(struct device *dev)
{
	int ret;

	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device   *rtc_dev = platform_get_drvdata(pdev);

	ret = request_irq(rtc_alarmno, rtc_alarmirq, IRQF_DISABLED, "my2440-rtc alarm", rtc_dev);
	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", rtc_alarmno, ret);
		return ret;
	}

	ret = request_irq(rtc_tickno, rtc_tickirq, IRQF_DISABLED, "my2440-rtc tick", rtc_dev);
	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", rtc_tickno, ret);
		goto tick_err;
	}

	return ret;

tick_err:
	free_irq(rtc_alarmno, rtc_dev);
	return ret;
}

static irqreturn_t rtc_alarmirq(int irq, void *dev_id)
{
	struct rtc_device *rtc_dev = dev_id;

	rtc_update_irq(rtc_dev, 1, RTC_AF | RTC_IRQF);

	return IRQ_HANDLED;
}

static irqreturn_t rtc_tickirq(int irq, void *dev_id)
{
	struct rtc_device *rtc_dev = dev_id;

	rtc_update_irq(rtc_dev, 1, RTC_PF | RTC_IRQF);

	return IRQ_HANDLED;
}

static void rtc_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device   *rtc_dev = platform_get_drvdata(pdev);

	rtc_setpie(dev, 0);

	free_irq(rtc_alarmno, rtc_dev);
	free_irq(rtc_tickno,  rtc_dev);
}

static int rtc_setpie(struct device *dev, int flag)
{
	unsigned int tmp;

	spin_lock_irq(&rtc_pie_lock);

	tmp = readb(rtc_base + S3C2410_TICNT);
	tmp &= ~S3C2410_TICNT_ENABLE;

	if (flag)
		tmp |= S3C2410_TICNT_ENABLE;

	writeb(tmp, rtc_base + S3C2410_TICNT);

	spin_unlock_irq(&rtc_pie_lock);
	return 0;
}

static int rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int have_retried = 0;

retry_get_time:
	rtc_tm->tm_min  = readb(rtc_base + S3C2410_RTCMIN);
	rtc_tm->tm_hour = readb(rtc_base + S3C2410_RTCHOUR);
	rtc_tm->tm_mday = readb(rtc_base + S3C2410_RTCDATE);
	rtc_tm->tm_mon  = readb(rtc_base + S3C2410_RTCMON);
	rtc_tm->tm_year = readb(rtc_base + S3C2410_RTCYEAR);
	rtc_tm->tm_sec  = readb(rtc_base + S3C2410_RTCSEC);

	if (rtc_tm->tm_sec == 0 && !have_retried) {
		have_retried = 1;
		goto retry_get_time;
	}

	rtc_tm->tm_sec   = bcd2bin(rtc_tm->tm_sec);
	rtc_tm->tm_min   = bcd2bin(rtc_tm->tm_min);
	rtc_tm->tm_hour  = bcd2bin(rtc_tm->tm_hour);
	rtc_tm->tm_mday  = bcd2bin(rtc_tm->tm_mday);
	rtc_tm->tm_mon   = bcd2bin(rtc_tm->tm_mon);
	rtc_tm->tm_year  = bcd2bin(rtc_tm->tm_year);

	rtc_tm->tm_year += 100;
	rtc_tm->tm_mon  -= 1;

	return 0;
}

static int rtc_settime(struct device *dev, struct rtc_time *tm)
{

	tm->tm_year -= 100;
	tm->tm_mon  += 1;

	if (tm->tm_year < 0 || tm->tm_year >= 100) {
		dev_err(dev, "rtc only supports 100 years\n");
		return -EINVAL;
	}

	writeb(bin2bcd(tm->tm_sec), rtc_base + S3C2410_RTCSEC);
	writeb(bin2bcd(tm->tm_min), rtc_base + S3C2410_RTCMIN);
	writeb(bin2bcd(tm->tm_hour), rtc_base + S3C2410_RTCHOUR);
	writeb(bin2bcd(tm->tm_mday), rtc_base + S3C2410_RTCDATE);
	writeb(bin2bcd(tm->tm_mon),  rtc_base + S3C2410_RTCMON);
	writeb(bin2bcd(tm->tm_year), rtc_base + S3C2410_RTCYEAR);

	return 0;
}

static int rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned int alarm_en;
	struct rtc_time *alarm_tm = &alarm->time;

	alarm_tm->tm_sec  = readb(rtc_base + S3C2410_ALMSEC);
	alarm_tm->tm_min  = readb(rtc_base + S3C2410_ALMMIN);
	alarm_tm->tm_hour = readb(rtc_base + S3C2410_ALMHOUR);
	alarm_tm->tm_mon  = readb(rtc_base + S3C2410_ALMMON);
	alarm_tm->tm_mday = readb(rtc_base + S3C2410_ALMDATE);
	alarm_tm->tm_year = readb(rtc_base + S3C2410_ALMYEAR);

	alarm_en = readb(rtc_base + S3C2410_RTCALM);

	alarm->enabled = (alarm_en & S3C2410_RTCALM)? 1: 0;

	if (alarm_en & S3C2410_RTCALM_SECEN)
		alarm_tm->tm_sec = bcd2bin(alarm_tm->tm_sec);
	else
		alarm_tm->tm_sec = 0xff;

	if (alarm_en & S3C2410_RTCALM_MINEN)
		alarm_tm->tm_min = bcd2bin(alarm_tm->tm_min);
	else
		alarm_tm->tm_min = 0xff;

	if (alarm_en & S3C2410_RTCALM_HOUREN)
		alarm_tm->tm_hour = bcd2bin(alarm_tm->tm_hour);
	else
		alarm_tm->tm_hour = 0xff;

	if (alarm_en & S3C2410_RTCALM_DAYEN)
		alarm_tm->tm_mday = bcd2bin(alarm_tm->tm_mday);
	else
		alarm_tm->tm_mday = 0xff;

	if (alarm_en & S3C2410_RTCALM_MONEN) {
		alarm_tm->tm_mon = bcd2bin(alarm_tm->tm_mon);
		alarm_tm->tm_mon -= 1;
	} else 
		alarm_tm->tm_mon = 0xff;

	if (alarm_en & S3C2410_RTCALM_YEAREN)
		alarm_tm->tm_year = bcd2bin(alarm_tm->tm_year);
	else
		alarm_tm->tm_year = 0xffff;

	return 0;
}

static int rtc_setalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned int alarm_en;
	struct rtc_time *tm = &alarm->time;

	alarm_en  = readb(rtc_base + S3C2410_RTCALM);
	alarm_en &= S3C2410_RTCALM_ALMEN;

	writeb(0x00, rtc_base + S3C2410_RTCALM);
	if (tm->tm_sec < 60 && tm->tm_sec >= 0) {
		alarm_en |= S3C2410_RTCALM_SECEN;
		writeb(bin2bcd(tm->tm_sec), rtc_base + S3C2410_ALMSEC);
	}

	if (tm->tm_min < 60 && tm->tm_min >= 0) {
		alarm_en |= S3C2410_RTCALM_SECEN;
		writeb(bin2bcd(tm->tm_sec), rtc_base + S3C2410_ALMMIN);
	}

	if (tm->tm_hour < 24 && tm->tm_hour >= 0) {
		alarm_en |= S3C2410_RTCALM_HOUREN;
		writeb(bin2bcd(tm->tm_hour), rtc_base + S3C2410_ALMHOUR);
	}

	writeb(alarm_en, rtc_base + S3C2410_RTCALM);
	rtc_setaie(alarm->enabled);

	if (alarm->enabled)
		enable_irq_wake(rtc_alarmno);
	else
		disable_irq_wake(rtc_alarmno);

	return 0;
}

static void rtc_setaie(int flag)
{
	unsigned int tmp;

	tmp  = readb(rtc_base + S3C2410_RTCALM);
	tmp &= ~S3C2410_RTCALM_ALMEN;

	if (flag)
		tmp |= S3C2410_RTCALM_ALMEN;

	writeb(tmp, rtc_base + S3C2410_RTCALM);
}

static int __devexit rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	rtc_device_unregister(rtc);

	rtc_setpie(&pdev->dev, 0);
	rtc_setaie(0);

	iounmap(rtc_base);
	release_resource(rtc_mem);
	kfree(rtc_mem);

	return 0;
}

#ifdef CONFIG_PM

static int ticnt_save;

static int rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	ticnt_save = readb(rtc_base + S3C2410_TICNT);
	rtc_enable(pdev, 0);

	return 0;
}

static int rtc_resume(struct platform_device *pdev)
{
	rtc_enable(pdev, 1);
	writeb(ticnt_save, rtc_base + S3C2410_TICNT);

	return 0;
}


#endif


		

