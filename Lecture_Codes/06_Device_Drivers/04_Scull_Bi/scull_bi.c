#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/wait.h>

#define DEVICE_NAME "scull_bi"
#define CLASS_NAME  "scull_bi_class"
#define BUF_SIZE 4096

static int major;
static struct class *scull_class;
static struct device *scull_device;
static struct cdev scull_cdev;

/* Two buffers for bidirectional communication */
static char buffer_ab[BUF_SIZE];  // A → B
static char buffer_ba[BUF_SIZE];  // B → A

static int size_ab = 0;
static int size_ba = 0;

/* Wait queue */
static wait_queue_head_t read_q;

/* Flags */
static int data_ab = 0;
static int data_ba = 0;

/* Assign users alternately */
static int open_count = 0;

/* ================= OPEN ================= */
static int scull_open(struct inode *inode, struct file *file)
{
    file->private_data = (void *)(long)(open_count % 2);

    if ((long)file->private_data == 0)
        pr_info("scull_bi: User A connected\n");
    else
        pr_info("scull_bi: User B connected\n");

    open_count++;
    return 0;
}

/* ================= RELEASE ================= */
static int scull_release(struct inode *inode, struct file *file)
{
    pr_info("scull_bi: user disconnected\n");
    return 0;
}

/* ================= WRITE ================= */
static ssize_t scull_write(struct file *file,
                          const char __user *ubuf,
                          size_t len,
                          loff_t *offset)
{
    int user = (long)file->private_data;

    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (user == 0) {
        /* A → B */
        if (copy_from_user(buffer_ab, ubuf, len))
            return -EFAULT;

        size_ab = len;
        data_ab = 1;

        pr_info("A sent %zu bytes\n", len);

    } else {
        /* B → A */
        if (copy_from_user(buffer_ba, ubuf, len))
            return -EFAULT;

        size_ba = len;
        data_ba = 1;

        pr_info("B sent %zu bytes\n", len);
    }

    /* Wake reader */
    wake_up_interruptible(&read_q);

    return len;
}

/* ================= READ ================= */
static ssize_t scull_read(struct file *file,
                         char __user *ubuf,
                         size_t len,
                         loff_t *offset)
{
    int user = (long)file->private_data;

    if (user == 0) {
        /* A reads from B */
        wait_event_interruptible(read_q, data_ba);

        if (copy_to_user(ubuf, buffer_ba, size_ba))
            return -EFAULT;

        data_ba = 0;
        return size_ba;

    } else {
        /* B reads from A */
        wait_event_interruptible(read_q, data_ab);

        if (copy_to_user(ubuf, buffer_ab, size_ab))
            return -EFAULT;

        data_ab = 0;
        return size_ab;
    }
}

/* ================= FOPS ================= */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .read = scull_read,
    .write = scull_write,
    .release = scull_release,
};

/* ================= INIT ================= */
static int __init scull_init(void)
{
    dev_t dev;

    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0)
        return -1;

    major = MAJOR(dev);

    cdev_init(&scull_cdev, &fops);
    if (cdev_add(&scull_cdev, dev, 1) < 0)
        goto fail;

    scull_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(scull_class))
        goto fail;

    scull_device = device_create(scull_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(scull_device))
        goto fail;

    init_waitqueue_head(&read_q);

    pr_info("scull_bi: loaded major=%d\n", major);
    return 0;

fail:
    cdev_del(&scull_cdev);
    unregister_chrdev_region(dev, 1);
    return -1;
}

/* ================= EXIT ================= */
static void __exit scull_exit(void)
{
    device_destroy(scull_class, MKDEV(major, 0));
    class_destroy(scull_class);
    cdev_del(&scull_cdev);
    unregister_chrdev_region(MKDEV(major, 0), 1);

    pr_info("scull_bi: unloaded\n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANUPAMA");
MODULE_DESCRIPTION("Final Bidirectional SCULL Chat Driver");
