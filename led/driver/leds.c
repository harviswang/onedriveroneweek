#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include <mach/regs-gpio.h>
#include <mach/hardware.h>

#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>


#define MODULE_NAME "leds" /* always equal the *.ko file's name */
#define DEVICE_NAME "mini2440-leds"
static char *device_name = DEVICE_NAME;
module_param(device_name, charp, S_IRUGO);

static unsigned long led_table [] = {
	S3C2410_GPB(5),
	S3C2410_GPB(6),
	S3C2410_GPB(7),
	S3C2410_GPB(8),
};

static unsigned int led_cfg_table [] = {
	S3C2410_GPIO_OUTPUT,
	S3C2410_GPIO_OUTPUT,
	S3C2410_GPIO_OUTPUT,
	S3C2410_GPIO_OUTPUT,
};


static int s3c2440_leds_ioctl(
	struct inode *inode, 
	struct file *file, 
	unsigned int cmd, 
	unsigned long arg)
{
	switch (cmd) {
		case 0:
		case 1:
			if (arg > 4)
				return -EINVAL;

			s3c2410_gpio_setpin(led_table[arg], !cmd);
			return 0;
		default:
			return -EINVAL;
	}
}

static struct file_operations dev_ops = {
	.owner = THIS_MODULE,
	.ioctl = s3c2440_leds_ioctl,	
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &dev_ops,
};


static int __init s3c2440_leds_module_init(void)
{
	int ret;
	int i;

	for (i = 0; i < 4; i++) {
		s3c2410_gpio_cfgpin(led_table[i], led_cfg_table[i]);
		s3c2410_gpio_setpin(led_table[i], 0);
	}

	misc.name = device_name;
	ret = misc_register(&misc);

	printk("#####Module Init######\n");
	printk("MODULE\tDEVICE\n");
	printk("%s\t/dev/%s\n", MODULE_NAME, device_name);

	return ret;
}

static void __exit s3c2440_leds_module_exit(void)
{
	misc_deregister(&misc);

	printk("#####Module Exit######\n");
	printk("MODULE\tDEVICE\n");
	printk("%s\t/dev/%s\n", MODULE_NAME, device_name);
}

module_init(s3c2440_leds_module_init);
module_exit(s3c2440_leds_module_exit);
MODULE_LICENSE("Dual GPL/BSD");
MODULE_AUTHOR("Harvis Wang <jiankangshiye@aliyun.com>");

