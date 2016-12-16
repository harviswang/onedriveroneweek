#include <linux/module.h>  
#include <linux/init.h>  
#include <linux/device.h> 
#include <linux/sched.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/types.h>  
#include <linux/completion.h>  

#define COMPLETE_DEV_NAME "complete"
static dev_t complete_dev = 0;
static struct class *complete_class = NULL;
DECLARE_COMPLETION(comp);

ssize_t complete_read(struct file *filp,char __user *buf,size_t count,loff_t *pos)
{
	printk(KERN_ERR "process %i (%s) going to sleep\n",current->pid, current->comm);
	wait_for_completion(&comp);
	printk(KERN_ERR "awoken %i (%s)\n",current->pid,current->comm);
	return 0;
}

ssize_t complete_write(struct file *filp,const char __user *buf,size_t count,loff_t *pos)
{
	printk(KERN_ERR "process %i (%s) awakening the readers...\n",current->pid,current->comm);
	complete(&comp);
	return count;
}

struct file_operations complete_fops = {
	.owner = THIS_MODULE,
	.read  = complete_read,
	.write = complete_write,
};

int complete_init(void) 
{
	int result;
	int err;
	struct device *device;
	
	err = alloc_chrdev_region(&complete_dev, 0, 1, COMPLETE_DEV_NAME);
	if (err) {
		return err;
	}

	result = register_chrdev(MAJOR(complete_dev), COMPLETE_DEV_NAME, &complete_fops);
	if (result < 0) {
		return result;
	}
	
	complete_class = class_create(THIS_MODULE, COMPLETE_DEV_NAME);
	if (IS_ERR(complete_class)) {
		return PTR_ERR(complete_class);
	}
	
	device = device_create(complete_class, NULL, complete_dev, NULL, COMPLETE_DEV_NAME);
	if (IS_ERR(device)) {
		return PTR_ERR(device);
	}

	return 0;
}

void complete_cleanup(void)
{
	unregister_chrdev(MAJOR(complete_dev), COMPLETE_DEV_NAME);
	device_destroy(complete_class, complete_dev);
	class_destroy(complete_class);
}

module_init(complete_init);
module_exit(complete_cleanup);
MODULE_LICENSE("Dual BSD/GPL");