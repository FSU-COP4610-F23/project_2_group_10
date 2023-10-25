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
bool firstRun = true;

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 ts_now;
    char buf[256];
    int len = 0;

    ktime_get_real_ts64(&ts_now);

    struct timespec64 cur = {ts_now.tv_sec, ts_now.tv_nsec};
    long long diff_s = cur.tv_sec - prev.tv_sec - 1;
    long long diff_ns = cur.tv_nsec - prev.tv_nsec;

    //len is the num of chars in buf
    len = snprintf(buf, sizeof(buf), "current time: %lld.%lld\n", cur.tv_sec, (long long)cur.tv_nsec);
    prev.tv_sec = cur.tv_sec;
    prev.tv_nsec = cur.tv_nsec;

    if(diff_ns < 0){
        diff_ns += 1000000000;
        return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
    }
    else{
        diff_s += 1;
        if(firstRun != true){ len += snprintf(buf+len, sizeof(buf), "elapsed time: %lld.%lld\n", diff_s, diff_ns);}
        firstRun = false;
        return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
    }
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