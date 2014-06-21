#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include <asm/irq.h>
#include <asm/uaccess.h>

#include <mach/regs-gpio.h>
#include <mach/hardware.h>

#define DEVICE_NAME "keys"

struct button_irq_desc {
	int irq;
	int pin;
	int pin_setting;
	int number;
	char *name;
};

static struct button_irq_desc button_irqs [] = {
	{IRQ_EINT8,  S3C2410_GPG(0), S3C2410_GPG0_EINT8,  0, "KEY0"},
	{IRQ_EINT11, S3C2410_GPG(3), S3C2410_GPG3_EINT11, 1, "KEY1"},
	{IRQ_EINT13, S3C2410_GPG(5), S3C2410_GPG5_EINT13, 2, "KEY2"},
	{IRQ_EINT14, S3C2410_GPG(6), S3C2410_GPG6_EINT14, 3, "KEY3"},
	{IRQ_EINT15, S3C2410_GPG(7), S3C2410_GPG7_EINT15, 4, "KEY4"},
	{IRQ_EINT19, S3C2410_GPG(11),S3C2410_GPG11_EINT19,5, "KEY5"},
};

static volatile char key_values [] = {'0', '0', '0', '0', '0', '0'};

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static volatile int ev_press = 0;

static irqreturn_t buttons_interrupt(int irq, void *dev_id)
{
	struct button_irq_desc *button_irq = (struct button_irq_desc *)dev_id;
	int down;

	down = !s3c2410_gpio_getpin(button_irq->pin);
	if (down != (key_values[button_irq->number] & 1)) {
		/* printk(KERN_ALERT "key_values[button_irq->number] & 1 = %d\n", key_values[button_irq->number] & 1); */
		key_values[button_irq->number] = '0' + down;
		ev_press = 1;
		wake_up_interruptible(&button_waitq);
	}	

	return IRQ_RETVAL(IRQ_HANDLED);
}

static int s3c24xx_buttons_open(struct inode *inode, struct file *file)
{
	int i;
	int err = 0;

	for (i = 0; i < ARRAY_SIZE(button_irqs); i++) {
		s3c2410_gpio_cfgpin(button_irqs[i].pin, button_irqs[i].pin_setting);

		if (button_irqs[i].irq < 0) {
			continue;
		}

		err = request_irq(button_irqs[i].irq, 
						  buttons_interrupt, 
						  IRQ_TYPE_EDGE_BOTH,
						  button_irqs[i].name,
						  (void *)&button_irqs[i]);
		if (err)
			break;
	}

	if (err) {
		i--;
		for (; i >= 0; i--) {
			if (button_irqs[i].irq < 0) {
				continue;
			}

			disable_irq(button_irqs[i].irq);
			free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
		}

		return -EBUSY;
	}

/*	ev_press = 1; *//* it is not needed any more, I've tested eveything works OK */
	return 0;
}

static int s3c24xx_buttons_close(struct inode *inode, struct file *file)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(button_irqs); i++) {
		if (button_irqs[i].irq < 0)
			continue;

		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
	}

	return 0;
}

static int s3c24xx_buttons_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	unsigned long err;
	if (!ev_press) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(button_waitq, ev_press);
	}

	ev_press = 0;

	err = copy_to_user(buff, (const void *)key_values, min(sizeof(key_values), count));

	return err ? -EFAULT: min(sizeof(key_values), count);
}

static unsigned int s3c24xx_buttons_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &button_waitq, wait);
	
	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open  = s3c24xx_buttons_open,
	.release = s3c24xx_buttons_close,
	.read  = s3c24xx_buttons_read,
	.poll  = s3c24xx_buttons_poll,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &dev_fops,
};

static int __init buttons_module_init(void)
{
	int ret;

	ret = misc_register(&misc);

	printk(DEVICE_NAME "\tinitialized\n");

	return ret;
}

static void __exit buttons_module_exit(void)
{
	misc_deregister(&misc);
}

module_init(buttons_module_init);
module_exit(buttons_module_exit);
MODULE_AUTHOR("Harvis Wang <jiankangshiye@aliyun.com>");
MODULE_LICENSE("Dual BSD/GPL");

