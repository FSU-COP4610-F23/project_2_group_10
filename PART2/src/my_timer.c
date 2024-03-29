#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Example of kernel module for timer");

#define ENTRY_NAME "my_timer"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* timer_entry;
struct timespec64 prev = {0, 0};
int firstRun = 1;

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 ts_now;
    char buf[256];
    int len = 0;

    ktime_get_real_ts64(&ts_now);
    
    len = snprintf(buf, sizeof(buf), "current time: %lld.%lld\n", (long long)ts_now.tv_sec,
        (long long)ts_now.tv_nsec);

    if(prev.tv_sec != 0 || prev.tv_nsec != 0){
        long long diff_s = ts_now.tv_sec - prev.tv_sec - 1;
        long long diff_ns = ts_now.tv_nsec - prev.tv_nsec;

        if(diff_ns < 0){
            diff_ns += 1000000000;
        }
        else{
            diff_s += 1;
        }
        if(!firstRun){len += snprintf(buf+len, sizeof(buf), "elapsed time: %lld.%lld\n",
            diff_s, diff_ns);}
        else{firstRun--;}
    }
    prev.tv_sec = ts_now.tv_sec;
    prev.tv_nsec = ts_now.tv_nsec;
    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
}

static const struct proc_ops timer_fops = {
    .proc_read = timer_read,
};

static int __init timer_init(void)
{
    timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    if (!timer_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);
