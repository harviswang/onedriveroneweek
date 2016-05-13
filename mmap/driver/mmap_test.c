#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/gpio.h>

#define DEVICE_NAME "mymmap"

static unsigned char array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static unsigned char *buffer;

static int my_open(struct inode *indoe, struct file *filp)
{
	return 0;
}

static int my_release(struct inode *indoe, struct file *filp)
{
	return 0;
}

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long page;
	unsigned char i;
	int err;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	/* get physicall address */
	page = virt_to_phys(buffer);

	/*  */
	err = remap_pfn_range(vma, vma->vm_start, page >> PAGE_SHIFT, size, PAGE_SHARED);
	if (err) {
		printk(KERN_ERR "remap_pfn_range() failed\n");
		return -1;
	} else {
		/* write ten bytes in buffer */
		for (i = 0; i < 10; i++) {
			buffer[i] = array[i];
		}
		return err;
	}
}	

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.mmap = my_mmap,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init dev_init(void)
{
	int err;

	/* registe misc device */
	err = misc_register(&misc);
	if (err) {
		printk(KERN_ERR "misc_register() failed\n");
		return -1;
	} else {
		/* alloc memory to buffer */
		buffer = (unsigned char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
		if (!buffer) {
			printk(KERN_ERR "kmalloc() failed\n");
			return -1;
		} else {
			/* set the malloced memory as reserved */
			SetPageReserved(virt_to_page(buffer));
			return err;
		}
	}
}

static void __exit dev_exit(void)
{
	int err;

	/* deregister misc device */
	err = misc_deregister(&misc);
	if (err) {
		printk(KERN_ERR "misc_deregister() failed\n");
	} else {
		/* clear reserved memory that we have alloced */
		ClearPageReserved(virt_to_page(buffer));

		/* free malloced memory */
		if (buffer != NULL) {
		    kfree(buffer);
		}
		buffer = NULL;
	}
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LKN@SCUT jiankangshiye@aliyun.com");

