#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("A waiter that demonstrates kernel threads with proc entry");

#define NUM_TABLES 4

#define ENTRY_NAME "waiter"
#define PERMS 0644
#define PARENT NULL

enum state {MOVING, CLEANING};      // you should note that enums are just integers.

struct waiter {
    int current_table;              // an example of information to store in your waiter.
    enum state status;
    struct task_struct *kthread;    // this is the struct to make a kthread.
};

struct bar {
    int tables[4];
};

static struct proc_dir_entry *proc_entry;

/* Here is our global variables */
static struct waiter waiter_thread;
static struct bar bar;

/* Move to the next table */
int move_to_next_table(int table) {
    return (table + 1) % NUM_TABLES;
}

/* Function to process waiter state */
void process_waiter_state(struct waiter * w_thread) {
    switch(w_thread->status) {
        case MOVING:
            ssleep(1);                    // sleeps for 1 second, before processing next stuff!
            w_thread->current_table = move_to_next_table(w_thread->current_table);
            w_thread->status = CLEANING;
            break;
        case CLEANING:
            ssleep(2);                                      // sleeps for 2 seconds, before processing next stuff!
            bar.tables[waiter_thread.current_table] += 1;   // increment how many times we cleaned table!
            w_thread->status = MOVING;                      // changed states!
        default:
            break;
    }
}

/* waiter */
int waiter_active(void * _waiter) {
    struct waiter * w_thread = (struct waiter *) _waiter;
    printk(KERN_INFO "waiter thread has started running \n");
    while(!kthread_should_stop()) {
        process_waiter_state(w_thread);
    }
    return 0;
}

/* This is where we spawn our waiter thread */
int spawn_waiter(struct waiter * w_thread) {
    static int current_table = 0;

    w_thread->current_table = current_table;
    w_thread->kthread =
        kthread_run(waiter_active, w_thread, "thread waiter\n"); // thread actually spawns here!

    return 0;
}

// function to print to proc file the bar state using waiter state
int print_bar_state(char * buf) {
    int i;
    int len = 0;

    // convert enums (integers) to strings
    const char * states[2] = {"MOVING", "CLEANING"};

    len += sprintf(buf + len, "Bar state: %s\n", states[waiter_thread.status]);
    for(i = 0; i < NUM_TABLES; i++) {
        int table = i + 1;

        // ternary operators equivalent to the bottom if statement.
        len += (i != waiter_thread.current_table)
            ? sprintf(buf + len, "[ ] Table %d: %d times cleaned. \n", table, bar.tables[i])
            : sprintf(buf + len, "[*] Table %d: %d times cleaned. \n", table, bar.tables[i]);

        // if(i != waiter_thread.current_table)
        //     len += sprintf(buf + len, "[ ] Table %d: %d times cleaned. \n", i, bar.tables[i]);
        // else
        //     len += sprintf(buf + len, "[*] Table %d: %d times cleaned. \n", i, bar.tables[i]);
    }
    return len;
}

/* This function triggers every read! */
static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[256];
    int len = 0;

    if (*ppos > 0 || count < 256)
        return 0;

    // recall that this is triggered every second if we do watch -n1 cat /proc/waiter
    len = print_bar_state(buf);   // this is how we write to the file!

    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
}

/* This is where we define our procfile operations */
static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};

/* Treat this like a main function, this is where your kernel module will
   start from, as it gets loaded. */
static int __init waiter_init(void) {
    spawn_waiter(&waiter_thread);                       // this is where we spawn the thread.

    if(IS_ERR(waiter_thread.kthread)) {
        printk(KERN_WARNING "error creating thread");
        remove_proc_entry(ENTRY_NAME, PARENT);
        return PTR_ERR(waiter_thread.kthread);
    }

    proc_entry = proc_create(                   // this is where we create the proc file!
        ENTRY_NAME,
        PERMS,
        PARENT,
        &procfile_pops
    );

    return 0;
}

/* This is where we exit our kernel module, when we unload it! */
static void __exit waiter_exit(void) {
    kthread_stop(waiter_thread.kthread);                // this is where we stop the thread.
    remove_proc_entry(ENTRY_NAME, PARENT);              // this is where we remove the proc file!
}

module_init(waiter_init); // initiates kernel module
module_exit(waiter_exit); // exits kernel module