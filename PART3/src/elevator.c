#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>

#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("syscalls written to procfile with proc entry");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL
#define LOG_BUF_LEN 1024

static char log_buffer[LOG_BUF_LEN];
static int buf_offset = 0;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

static struct proc_dir_entry *proc_entry;
//static struct Building of some type (static struct ?? Building)
//static struct Floor of some type (static struct ?? Floor)
//struct Student of some type (static struct ?? Student)

/* This function is called when when the start_elevator system call is called */
int start_elevator(void) {
    buf_offset += snprintf(log_buffer + buf_offset, LOG_BUF_LEN - buf_offset, "start_elevator call is called\n");
    return 0;
}

/* This function is called when when the issue_request system call is called */
int issue_request(int start_floor, int destination_floor, int type) {
    buf_offset += snprintf(log_buffer + buf_offset,
                            LOG_BUF_LEN - buf_offset,
                            "issue_request call with start=%d, destination=%d, type=%d\n",
                            start_floor,
                            destination_floor,
                            type);
    return 0;
}

/* This function is called when when the stop_elevator system call is called */
int stop_elevator(void) {
    buf_offset += snprintf(log_buffer + buf_offset, LOG_BUF_LEN - buf_offset, "stop_elevator call is called\n");
    return 0;
}

/* This function triggers every read! */
static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 ts_now;

    //if( (elapsed.tv_sec == 2) && (elapsed.tv_nsec == 0) ), move to next floor

    //else if( ((elapsed.tv_sec == 2) && (elapsed.tv_nsec == 0)) && ((!floorline) || (!psngrsOnElev)) ),
    //load or unload the passengers

    return simple_read_from_buffer(ubuf, count, ppos, log_buffer, buf_offset);
}

/* This is where we define our procfile operations */
static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};

/* Treat this like a main function, this is where your kernel module will
   start from, as it gets loaded. */
static int __init syscall_log_init(void) {


    // This is where we link our system calls to our stubs
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    //make kthreads, linked lists, and mutexs here.

    proc_entry = proc_create(                   // this is where we create the proc file!
        ENTRY_NAME,
        PERMS,
        PARENT,
        &procfile_pops
    );


    return 0;
}

/* This is where we exit our kernel module, when we unload it! */
static void __exit syscall_log_exit(void) {

    // This is where we unlink our system calls from our stubs
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    remove_proc_entry(ENTRY_NAME, PARENT);              // this is where we remove the proc file!
}

module_init(syscall_log_init); // initiates kernel module
module_exit(syscall_log_exit); // exits kernel module