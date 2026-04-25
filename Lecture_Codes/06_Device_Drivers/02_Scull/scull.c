#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>

//macros defined here

#define DEVICE_NAME "scull"
#define CLASS_NAME  "scull_class"
#define SCULL_BUFFER_SIZE 4096

static int major; // device assigned major number
static struct class* scull_class = NULL; //device class

/*Create a variable that:
- is private to this file (static)
- will store address of a kernel "device class" (struct class*)
- currently points to nothing (NULL)*/

static struct device* scull_device = NULL;
static struct cdev scull_cdev;

static char *scull_buffer;
static size_t scull_size = 0;

static int scull_open(struct inode *inode, struct file *file) {
    pr_info("scull: opened\n");
    return 0;
}

static int scull_release(struct inode *inode, struct file *file) {
    pr_info("scull: closed\n");
    return 0;
}

static ssize_t scull_read(struct file *file, char __user *ubuf, size_t len, loff_t *offset) {
    if (*offset >= scull_size)
        return 0;

    if (*offset + len > scull_size)
        len = scull_size - *offset;

    if (copy_to_user(ubuf, scull_buffer + *offset, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static ssize_t scull_write(struct file *file, const char __user *ubuf, size_t len, loff_t *offset) {
        //ADD THIS BLOCK HERE
    if (file->f_flags & O_APPEND) {
        *offset = scull_size;
    }
    if (*offset + len > SCULL_BUFFER_SIZE)
        len = SCULL_BUFFER_SIZE - *offset;

    if (len == 0)
        return -ENOMEM;

    if (copy_from_user(scull_buffer + *offset, ubuf, len))
        return -EFAULT;

    *offset += len;
    if (*offset > scull_size)
        scull_size = *offset;

    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .read = scull_read,
    .write = scull_write,
    .release = scull_release,
};

static int __init scull_init(void) {
    dev_t dev;

    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
        pr_err("scull: failed to allocate char dev region\n");
        return -1;
    }

    major = MAJOR(dev);
    scull_buffer = kmalloc(SCULL_BUFFER_SIZE, GFP_KERNEL);
    if (!scull_buffer) {
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    cdev_init(&scull_cdev, &fops);
    if (cdev_add(&scull_cdev, dev, 1) == -1) {
        kfree(scull_buffer);
        unregister_chrdev_region(dev, 1);
        return -1;
    }

    scull_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(scull_class)) {
        cdev_del(&scull_cdev);
        kfree(scull_buffer);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(scull_class);
    }

    scull_device = device_create(scull_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(scull_device)) {
        class_destroy(scull_class);
        cdev_del(&scull_cdev);
        kfree(scull_buffer);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(scull_device);
    }

    pr_info("scull: registered with major number %d\n", major);
    return 0;
}

static void __exit scull_exit(void) {
    device_destroy(scull_class, MKDEV(major, 0));
    class_unregister(scull_class);
    class_destroy(scull_class);
    cdev_del(&scull_cdev);
    unregister_chrdev_region(MKDEV(major, 0), 1);
    kfree(scull_buffer);
    pr_info("scull: unregistered\n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Simplified SCULL Device Driver");

