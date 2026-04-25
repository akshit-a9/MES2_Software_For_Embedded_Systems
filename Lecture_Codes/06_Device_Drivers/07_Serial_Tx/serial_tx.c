// serial_tx.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/slab.h>

#define DEVICE_NAME "serial_tx"
#define CLASS_NAME  "serial_tx_class"
#define SERIAL_PORT "/dev/ttyUSB0"   // change if needed

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANUPAMA");
MODULE_DESCRIPTION("Simple Serial TX Driver (Kernel → UART)");

static int major;
static struct class *serial_class = NULL;
static struct cdev serial_cdev;

/*
========================================================
Helper: Get tty_struct from device file
========================================================
*/
static struct tty_struct *get_tty_from_file(struct file *file)
{
    if (!file)
        return NULL;

    // For tty devices, private_data holds tty_struct
    return (struct tty_struct *)file->private_data;
}

/*
========================================================
WRITE FUNCTION
========================================================
*/
static ssize_t serial_write(struct file *file,
                            const char __user *ubuf,
                            size_t len,
                            loff_t *offset)
{
    struct file *tty_file;
    struct tty_struct *tty;
    char *kbuf;
    int written = 0;

    /*
    ----------------------------------------------------
    Allocate kernel buffer
    ----------------------------------------------------
    */
    kbuf = kmalloc(len, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    /*
    ----------------------------------------------------
    Copy from user space
    ----------------------------------------------------
    */
    if (copy_from_user(kbuf, ubuf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }

    /*
    ----------------------------------------------------
    Open serial port (ttyS1)
    ----------------------------------------------------
    */
    tty_file = filp_open(SERIAL_PORT, O_WRONLY | O_NOCTTY, 0);
    if (IS_ERR(tty_file)) {
        pr_err("serial_tx: Failed to open %s\n", SERIAL_PORT);
        kfree(kbuf);
        return PTR_ERR(tty_file);
    }

    tty = get_tty_from_file(tty_file);
    if (!tty || !tty->ops || !tty->ops->write) {
        pr_err("serial_tx: Invalid tty ops\n");
        filp_close(tty_file, NULL);
        kfree(kbuf);
        return -EIO;
    }

    /*
    ----------------------------------------------------
    Write to serial port
    ----------------------------------------------------
    */
    written = tty->ops->write(tty, kbuf, len);

    /*
    ----------------------------------------------------
    Cleanup
    ----------------------------------------------------
    */
    filp_close(tty_file, NULL);
    kfree(kbuf);

    return written;
}

/*
========================================================
OPEN / CLOSE
========================================================
*/
static int serial_open(struct inode *inode, struct file *file)
{
    pr_info("serial_tx: Device opened\n");
    return 0;
}

static int serial_release(struct inode *inode, struct file *file)
{
    pr_info("serial_tx: Device closed\n");
    return 0;
}

/*
========================================================
FILE OPERATIONS
========================================================
*/
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = serial_open,
    .write   = serial_write,
    .release = serial_release,
};

/*
========================================================
INIT
========================================================
*/
static int __init serial_init(void)
{
    dev_t dev;

    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
        return -1;

    major = MAJOR(dev);

    cdev_init(&serial_cdev, &fops);
    if (cdev_add(&serial_cdev, dev, 1) < 0)
        goto fail_cdev;

    serial_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(serial_class))
        goto fail_class;

    if (IS_ERR(device_create(serial_class, NULL, dev, NULL, DEVICE_NAME)))
        goto fail_device;

    pr_info("serial_tx: Loaded. Major=%d\n", major);
    return 0;

fail_device:
    class_destroy(serial_class);
fail_class:
    cdev_del(&serial_cdev);
fail_cdev:
    unregister_chrdev_region(dev, 1);
    return -1;
}

/*
========================================================
EXIT
========================================================
*/
static void __exit serial_exit(void)
{
    device_destroy(serial_class, MKDEV(major, 0));
    class_destroy(serial_class);
    cdev_del(&serial_cdev);
    unregister_chrdev_region(MKDEV(major, 0), 1);

    pr_info("serial_tx: Unloaded\n");
}

module_init(serial_init);
module_exit(serial_exit);
