#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>

static ssize_t cp0_show(struct device *dev, struct device_attribute *attr, 
    char *buf)
{
    unsigned int value = 0;
    unsigned int cp0_register = 12;
    unsigned int cp0_select = 0;
    __asm__ __volatile__("mfc0 %0, $%1, %2\n"
                         :  "=r" (value)       /* output */
                         : "im" (cp0_register), "im" (cp0_select) /* input */
                         : "memory");
    return sprintf(buf, "cp0($%d, %d) = 0x%x\n", cp0_register, cp0_select, value);
}

/*
 * echo "$12, 0" > cp0
 * cp0_register = 12
 * cp0_select = 0
 */
static ssize_t cp0_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
    unsigned int value = 0;
    unsigned int cp0_register = 12;
    unsigned int cp0_select = 0;

    __asm__ __volatile__("mfc0 %0, $%1, %2\n"
                         :  "=r" (value)       /* output */
                         : "im" (cp0_register), "im" (cp0_select) /* input */
                         : "memory");

    dev_info(dev, "cp0($%d, %d) = 0x%x\n", cp0_register, cp0_select, value);
    return count;
}

DEVICE_ATTR(cp0, S_IRUGO | S_IWUSR | S_IWGRP, cp0_show, cp0_store);

static int cp0_probe(struct platform_device *pdev)
{
    int error = 0;
    dev_info(&pdev->dev, "%s line:%d\n", __func__, __LINE__);
    error = device_create_file(&pdev->dev, &dev_attr_cp0);
    dev_info(&pdev->dev, "%s line:%d error = %d\n", __func__, __LINE__, error);
    return error;
}

static struct platform_driver cp0_driver = {
    .probe = cp0_probe,
    .driver = {
        .name = "cp0",
        .owner = THIS_MODULE,
    },
    
};

static struct platform_device cp0_device = {
    .name = "cp0",
    .id = 1,
    .num_resources = 0,
};

int cp0_entry(void)
{
    int err = 0;
	printk("%s line:%d\n", __func__, __LINE__);
    err = platform_device_register(&cp0_device);
    printk("err = %d\n", err);
    err = platform_driver_register(&cp0_driver);
    printk("err = %d\n", err);
    
	return err;
}

void cp0_exit(void)
{
	printk("%s\n", __func__);
}

module_init(cp0_entry);
module_exit(cp0_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Harvis Wang");
