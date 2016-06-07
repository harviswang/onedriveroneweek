#include <linux/kernel.h>
#include <linux/module.h>

static int __init hello_module_init(void)
{
	printk("Hello, Mini2440 module is installed !\n");
	return 0;
}

static void __exit hello_module_clean(void)
{
	printk("Good-bye, hello module was removed !\n");
}

module_init(hello_module_init);
module_exit(hello_module_clean);
MODULE_LICENSE("Dual BSD/GPL");
	
