#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/semaphore.h>

#include <asm/uaccess.h>

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;
	int quantum; /* sizeof(this->data->data[0])/sizeof(this->data->data[0][0]) */
	int qset;    /* sizeof(this->data->data)/sizeof(this->data->data[0]) */
	unsigned long size;      /* used size */
	unsigned int access_key; /* sculluid, scullpriv */
	struct semaphore sem;    /* mutext lock */
	struct cdev cdev;        /* char device struct */
};

static ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos); 
static ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static int scull_release(struct inode *inode, struct file *filp);
static int scull_open(struct inode *inode, struct file *filp);
static int scull_trim(struct scull_dev *dev);
static struct scull_qset *scull_follow(struct scull_dev *dev, int item);

static int scull_major = 0;
static int scull_minor = 0;
static int scull_quantum = 1000;
static int scull_qset    = 1000;
static int scull_nr_devs = 4;
static struct scull_dev dev;
module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset,  int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);


static struct file_operations scull_fops = {
	.owner  = THIS_MODULE,
	.read   = scull_read,
	.write  = scull_write,
	.open   = scull_open,
	.release= scull_release,
};

/*
 * scull_setup_cdev
 * dev: 
 * index: 0, 1, 2 or others
 */
static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;
	int devno;

	devno = MKDEV(scull_major, scull_minor + index);
	cdev_init(&dev->cdev, &scull_fops); /* first arg must be a pointer */
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops   = &scull_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

static int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev; /* private data, here just store a scull_dev pointer */

	/* now trim to 0 the length of the device if open was write-only */
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
	}

	return 0; /* success */
}

static ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) 
{
	struct scull_dev *dev;
	struct scull_qset *dptr;
	int quantum;
	int qset;
	int itemsize;
	int item;
	int s_pos;
	int q_pos;
	int rest;
	ssize_t retval = 0;

	dev = filp->private_data;
	quantum = dev->quantum;
	qset = dev->qset;
	itemsize = quantum * qset;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* which scull_qset, which scull_qset->data[] */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; /* offset of scull_qset->data */
	q_pos = rest % quantum; /* offset of scull_qset->data[s_pos] */

	/* get the "which scull_qset" */
	dptr = scull_follow(dev, item);
	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out;

	/* read data from scull_dev */
	if (count > quantum - q_pos)
		count = quantum - q_pos; /* why not read next quantum */

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

out:
	up(&dev->sem);
	return retval;
}

static ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev;
	struct scull_qset *dptr;
	int quantum;
	int qset;
	int itemsize;
	int item;
	int s_pos;
	int q_pos;
	int rest;
	int retval;

	dev = filp->private_data;
	quantum = dev->quantum;
	qset = dev->qset;
	itemsize = quantum * qset;
	retval = -EINVAL;
	
	/* lock start */
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* which linked scull_qset, which scull_qset->data */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos= rest / quantum; /* offset of scull_qset->data */
	q_pos= rest % quantum; /* offset of scull_qset->data[s_pos] */

	/* walk to the item'th scull_qset */
	dptr = scull_follow(dev, item);
	if (dptr == NULL)
		goto out;
	/* no scull_qset->data for writting */
	if (!dptr->data) {
		dptr->data = (void **)kmalloc(qset * sizeof(char *), GFP_KERNEL); /* (void **) is needed ? */
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}

	/* no scull_qset->data[s_pos] */
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
	}

	/* write one quantum at most */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	/* real writting */
	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval =  count;

	/* update scull_dev->size as we have written something in it */
	if (dev->size < *f_pos)
		dev->size = *f_pos;

out:
	up(&dev->sem);
	return retval;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	return 0; /* do nothing just return to OS */
}

static int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset; /* dev != NULL */
	int i;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}

	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset    = scull_qset;
	dev->data    = NULL;

	return 0;
}

static struct scull_qset *scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *dptr;
	struct scull_qset *next;
	int i;

	for (i = 0, dptr = dev->data; dptr && i < item; dptr = next) {
		next = dptr->next;	
	}
	
	return dptr;
}

static int __init scull_module_init(void)
{
	int i;
	dev_t devno; /* dev number */
	int result;

	/* register a major number for our device */
	if (scull_major) {
		devno = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(devno, scull_nr_devs, "scull");
	} else {
		result = alloc_chrdev_region(&devno, scull_minor, scull_nr_devs, "scull");
		scull_major = MAJOR(devno);
	}
	
	/* initialise semaphore before device register */
	init_MUTEX(&dev.sem);
	for (i = 0; i < scull_nr_devs; i++)
		scull_setup_cdev(&dev, i);
	
	return 0;
}

static void __exit scull_module_exit(void)
{
	cdev_del(&dev.cdev);	
}

module_init(scull_module_init);
module_exit(scull_module_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Harvis Wang <jiankangshiye@aliyun.com>");

