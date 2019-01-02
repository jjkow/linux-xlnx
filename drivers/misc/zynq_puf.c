#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define DRIVER_NAME "zynq-puf"
 

unsigned long *base_addr;   /* Vitual Base Address */
struct resource *res;       /* Device Resource Structure */
unsigned long remap_size;   /* Device Memory Size */
 
static ssize_t proc_zynq_puf_write(struct file *file, const char __user * buf,
                  size_t count, loff_t * ppos)
{
    printk(KERN_WARNING "Write mode not supported.");
    return 0;
}

static int proc_zynq_puf_show(struct seq_file *p, void *v)
{
    u32 rand_value;
    rand_value = ioread32(base_addr);
    seq_printf(p, "0x%x", rand_value);
    return 0;
}

static int proc_zynq_puf_open(struct inode *inode, struct file *file)
{
    unsigned int size = 16;
    char *buf;
    struct seq_file *m;
    int res;

    buf = (char *)kmalloc(size * sizeof(char), GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    res = single_open(file, proc_zynq_puf_show, NULL);

    if (!res) {
        m = file->private_data;
        m->buf = buf;
        m->size = size;
    } else {
        kfree(buf);
    }

    return res;
}
 
static const struct file_operations proc_zynq_puf_operations = {
    .open = proc_zynq_puf_open,
    .read = seq_read,
    .write = proc_zynq_puf_write,
    .llseek = seq_lseek,
    .release = single_release
};
 
static void zynq_puf_shutdown(struct platform_device *pdev)
{
   printk(KERN_WARNING "Shutting down Zynq PUF.");
}

static int zynq_puf_remove(struct platform_device *pdev)
{
    remove_proc_entry(DRIVER_NAME, NULL);

    iounmap(base_addr);

    release_mem_region(res->start, remap_size);

    return 0;
}

static int zynq_puf_probe(struct platform_device *pdev)
{
    struct proc_dir_entry *zynq_puf_proc_entry;
    int ret = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No memory resource\n");
        return -ENODEV;
    }

    remap_size = res->end - res->start + 1;
    if (!request_mem_region(res->start, remap_size, pdev->name)) {
        dev_err(&pdev->dev, "Cannot request IO\n");
        return -ENXIO;
    }

    base_addr = ioremap(res->start, remap_size);
    if (base_addr == NULL) {
        dev_err(&pdev->dev, "Couldn't ioremap memory at 0x%08lx\n",
            (unsigned long)res->start);
        ret = -ENOMEM;
        goto err_release_region;
    }

    zynq_puf_proc_entry = proc_create(DRIVER_NAME, 0, NULL,
                       &proc_zynq_puf_operations);
    if (zynq_puf_proc_entry == NULL) {
        dev_err(&pdev->dev, "Couldn't create proc entry\n");
        ret = -ENOMEM;
        goto err_create_proc_entry;
    }

    printk(KERN_INFO DRIVER_NAME " probed at VA 0x%08lx\n",
           (unsigned long) base_addr);

    return 0;

    err_create_proc_entry:
       iounmap(base_addr);
    err_release_region:
       release_mem_region(res->start, remap_size);

    return ret;
}

static const struct of_device_id zynq_puf_of_match[] = {
    {.compatible = "arbiter_puf_0,inverter_puf_0"},
    {},
};

MODULE_DEVICE_TABLE(of, zynq_puf_of_match);

static struct platform_driver zynq_puf_driver = {
    .driver = {
           .name = DRIVER_NAME,
           .owner = THIS_MODULE,
           .of_match_table = zynq_puf_of_match},
    .probe = zynq_puf_probe,
    .remove = zynq_puf_remove,
    .shutdown = zynq_puf_shutdown,
};

module_platform_driver(zynq_puf_driver);

MODULE_AUTHOR("Jan Kowalewski");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_NAME ": Zynq PUF driver");
MODULE_ALIAS(DRIVER_NAME);

