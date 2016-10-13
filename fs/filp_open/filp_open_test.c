/*
 * 在kernel中使用文件系统中的文件, 驱动中需要firmware操作的时候会用到
 * 测试file_name="/proc/misc"时, ko文件无法insmod, TODO
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

#define SIZE 64
int filp_test_entry(void)
{
    char *file_name = "/usr/share/hwdata/usb.ids";
    struct file *filp = NULL;
    mm_segment_t old_fs;
    char buff[SIZE + 1] = {0};
    int n;
    
    filp = filp_open(file_name, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        pr_err("open file %s failed\n", file_name);
        return -1;
    }
    
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    printk("filp->f_pos = %lld\n", filp->f_pos);
    
    filp->f_pos = 0;
    do {
        printk("filp->f_pos = %lld\n", filp->f_pos);
        n = filp->f_op->read(filp, buff, SIZE, &filp->f_pos);
        buff[n] = '\0';
        printk("buff: %s\n", buff);
    } while (n > 0);
    
    filp_close(filp, NULL);
    set_fs(old_fs);
    return 0;
}

void filp_test_exit(void)
{

}

module_init(filp_test_entry);
module_exit(filp_test_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Harvis Wang");
