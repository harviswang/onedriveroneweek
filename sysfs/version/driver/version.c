#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>


ssize_t
version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "hello");
}

DEVICE_ATTR(version, 0444, version_show, NULL);

static struct device_attribute *version_attributes[] = {
		&dev_attr_version,
		NULL,
};

static struct device version_dev = {
	.init_name = "version",
};

static int version_dev_setup(void)
{
	int i;
	int error = -EINVAL;

	do {
		error = device_register(&version_dev);
		if (error) {
			put_device(&version_dev);
			break;
		}
		
		for (i = 0; version_attributes[i] != NULL; i++) {
			error = device_create_file(&version_dev, version_attributes[i]);
			if (error) {
				int j;
				for (j = 0; j < i; i++) {
					device_remove_file(&version_dev, version_attributes[j]);
				}
				device_del(&version_dev);
				break;
			}
		}
	} while(0);
	
	return error;
}

static int version_init(void)
{
	return version_dev_setup();
}

static void version_exit(void)
{
	device_del(&version_dev);
}

module_init(version_init);
module_exit(version_exit);
MODULE_AUTHOR("Harvis Wang");
MODULE_LICENSE("GPL");

