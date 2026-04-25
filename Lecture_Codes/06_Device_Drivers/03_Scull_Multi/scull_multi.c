#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "scull"
#define CLASS_NAME  "scull_class"
#define SCULL_BUFFER_SIZE 4096
#define NUM_DEVICES 6  // Create /dev/scull0 to /dev/scull5

struct scull_dev {
    char *buffer;
    size_t size;
    struct cdev cdev;
};

static int major;
static struct class* scull_class = NULL;
static struct scull_dev scull_devices[NUM_DEVICES];

static int scull_open(struct inode *inode, struct file *file) {
    int minor = iminor(inode);
    file->private_data = &scull_devices[minor];
    pr_info("scull%d: opened\n", minor);
    return 0;
}

static int scull_release(struct inode *inode, struct file *file) {
    int minor = iminor(inode);
    pr_info("scull%d: closed\n", minor);
    return 0;
}

static ssize_t scull_read(struct file *file, char __user *ubuf, size_t len, loff_t *offset) {
    struct scull_dev *dev = file->private_data;

    if (*offset >= dev->size)
        return 0;
    if (*offset + len > dev->size)
        len = dev->size - *offset;

    if (copy_to_user(ubuf, dev->buffer + *offset, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static ssize_t scull_write(struct file *file, const char __user *ubuf, size_t len, loff_t *offset) {
    struct scull_dev *dev = file->private_data;
    
    if (file->f_flags & O_APPEND)
        *offset = dev->size;


    if (*offset + len > SCULL_BUFFER_SIZE)
        len = SCULL_BUFFER_SIZE - *offset;

    if (len == 0)
        return -ENOMEM;

    if (copy_from_user(dev->buffer + *offset, ubuf, len))
        return -EFAULT;

    *offset += len;
    if (*offset > dev->size)
        dev->size = *offset;

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
    int i;
    int ret;
    //set of charc device numbers alloted to your device
    ret = alloc_chrdev_region(&dev, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        pr_err("scull: failed to allocate major number\n");
        return ret;
    }

    major = MAJOR(dev);
    scull_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(scull_class)) {
        unregister_chrdev_region(MKDEV(major, 0), NUM_DEVICES);
        return PTR_ERR(scull_class);
    }

    for (i = 0; i < NUM_DEVICES; i++) {
        scull_devices[i].buffer = kmalloc(SCULL_BUFFER_SIZE, GFP_KERNEL);
        if (!scull_devices[i].buffer) {
            ret = -ENOMEM;
            goto cleanup;
        }

        scull_devices[i].size = 0;
        cdev_init(&scull_devices[i].cdev, &fops);
        ret = cdev_add(&scull_devices[i].cdev, MKDEV(major, i), 1);
        if (ret) {
            kfree(scull_devices[i].buffer);
            goto cleanup;
        }

        device_create(scull_class, NULL, MKDEV(major, i), NULL, "scull%d", i);
    }

    pr_info("scull: created %d devices with major %d\n", NUM_DEVICES, major);
    return 0;

cleanup:
    while (--i >= 0) {
        device_destroy(scull_class, MKDEV(major, i));
        cdev_del(&scull_devices[i].cdev);
        kfree(scull_devices[i].buffer);
    }
    class_destroy(scull_class);
    unregister_chrdev_region(MKDEV(major, 0), NUM_DEVICES);
    return ret;
}

static void __exit scull_exit(void) {
    int i;
    for (i = 0; i < NUM_DEVICES; i++) {
        device_destroy(scull_class, MKDEV(major, i));
        cdev_del(&scull_devices[i].cdev);
        kfree(scull_devices[i].buffer);
    }
    class_unregister(scull_class);
    class_destroy(scull_class);
    unregister_chrdev_region(MKDEV(major, 0), NUM_DEVICES);
    pr_info("scull: removed %d devices\n", NUM_DEVICES);
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANUPAMA");
MODULE_DESCRIPTION("SCULL Driver with Multiple Devices");

