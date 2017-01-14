#include <linux/module.h>  
#include <linux/init.h>  
#include <linux/device.h> 
#include <linux/sched.h>  
#include <linux/fs.h>  
#include <linux/completion.h>  
#include <linux/miscdevice.h>
#include <linux/stat.h>  /* S_IRUGO/S_IWUGO */
#include <asm/uaccess.h> /* copy_to_user/copy_from_user */
#include <linux/string.h>

#define BUFFER_SIZE 128
#define COMPLETE_DEV_NAME "completion"
DECLARE_COMPLETION(comp);
static char completion_buffer[BUFFER_SIZE] = { 0 };
static unsigned int read_index = 0;
static unsigned int write_index = 0;

ssize_t completion_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
    printk(KERN_ERR "process %i (%s) going to sleep\n", current->pid, current->comm);
    wait_for_completion(&comp);
    if (copy_to_user(buf, &completion_buffer[read_index], count)) {
        printk(KERN_ERR "copy_to_user failed\n");
    } else {
        read_index += count;
        if (read_index >= BUFFER_SIZE) {
            read_index %= BUFFER_SIZE;
        }
    }
    printk(KERN_ERR "awoken %i (%s)\n", current->pid, current->comm);
    return count;
}

ssize_t completion_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
    if (copy_from_user(&completion_buffer[write_index], buf, count)) {
        printk(KERN_ERR "copy_from_user failed\n");
    } else {
        write_index += count;
        if (write_index >= BUFFER_SIZE) {
            write_index %= BUFFER_SIZE;
        }
    }
    printk(KERN_ERR "process %i (%s) awakening the readers...\n", current->pid, current->comm);
    complete(&comp);
    return count;
}

static struct file_operations completion_fops = {
    .owner = THIS_MODULE,
    .read  = completion_read,
    .write = completion_write,
};

static struct miscdevice completion_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = COMPLETE_DEV_NAME,
    .fops = &completion_fops,
    .mode = S_IRUGO | S_IWUGO,
};

int complete_init(void)
{
    int err = 0;

    err = misc_register(&completion_miscdevice);
    if (err) {
        dev_err(completion_miscdevice.this_device, "misc_register() failed\n");
    }
    return err;
}

void complete_cleanup(void)
{
    int err;

    err = misc_deregister(&completion_miscdevice);
    if (err) {
        dev_err(completion_miscdevice.this_device, "misc_deregister() failed\n");
    }
}

module_init(complete_init);
module_exit(complete_cleanup);
MODULE_LICENSE("Dual BSD/GPL");