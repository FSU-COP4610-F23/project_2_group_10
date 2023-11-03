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
static bool started = false;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

static struct proc_dir_entry *proc_entry;
static struct elevator elevator_thread;
static struct building thisBuilding;


enum status {OFFLINE, IDLE, LOADING, UP, DOWN};      // you should note that enums are just integers.

struct elevator {
    int currentFloor;
    enum status state;
    int numPassengers;
    struct list_head passengers;
    struct task_struct *kthread;    // this is the struct to make a kthread.
};

struct building {
    int numFloors;
    struct floor floors[6];
};

struct floor {
    int numWaitingStud;
    struct list_head studentsWaiting;
};

struct student {
    char year;
    int destination;
    struct list_head student;
};


/* This function is called when when the start_elevator system call is called */
int start_elevator(void) {
    started = true;
    return 0;
}

/* This function is called when when the issue_request system call is called */
int issue_request(int start_floor, int destination_floor, int type) {
    struct student * new_student = kmalloc(sizeof(struct student), GFP_KERNEL);
    if(!new_student) {
        printk(KERN_INFO "Error: could not allocate memory for new student\n");
        return -ENOMEM;
    }

    new_student->year = intToYear(type);
    new_student->destination = destination_floor;

    list_add_tail(&new_student->student, &thisBuilding.floors[start_floor].studentsWaiting);  
    return 0;
}

/* This function is called when when the stop_elevator system call is called */
int stop_elevator(void) {
    kthread_stop(elevator_thread.kthread);
    return 0;
}

/* Function to process elevator state */
void process_elevator_state(struct elevator * e_thread) {
    switch(e_thread->state) {
        case LOADING:
            ssleep(1);                    // sleeps for 1 second, before processing next stuff!
            if (&elevator_thread.passengers->destination > e_thread->currentFloor)
                e_thread->state = UP;
            else e_thread->state = DOWN;
            break;
        case UP:
            ssleep(2);                                      // sleeps for 2 seconds, before processing next stuff!
            if (e_thread->currentFloor != &elevator_thread.passengers->destination)
                e_thread->currentFloor += 1;
            else e_thread->state = LOADING;                   // changed states!
            break;
        case DOWN:
            ssleep(2);
            if (e_thread->currentFloor != &elevator_thread.passengers->destination)
                e_thread->currentFloor -= 1;
            else e_thread->state = LOADING;
            break;
        case OFFLINE:
            if (started)
                e_thread->state = IDLE;
        default:
            break;
    }
}

int elevator_active(void * _elevator) {
    struct elevator * e_thread = (struct elevator *) _elevator;
    printk(KERN_INFO "elevator thread has started running \n");
    while(!kernel_should_stop() || e_thread->numPassengers > 0)
        process_elevator_state(e_thread);
    e_thread->state = OFFLINE;
    return 0;
}

int spawn_elevator(struct elevator * e_thread) {
    // initialize and/or allocate everything inside building struct and everything it points to
    thisBuilding.numFloors = 6;
    for (int i = 0; i < thisBuilding.numFloors; i++) {
        thisBuilding.floors[i].numWaitingStud = 0;
        INIT_LIST_HEAD(&thisBuilding.floors[i].studentsWaiting);
    }

    // initialize and/or allocate everything inside elevator struct and everything it points to
    e_thread->currentFloor = 1;
    e_thread->state = OFFLINE;
    e_thread->numPassengers = 0;
    INIT_LIST_HEAD(&elevator_thread.passengers);
    e_thread->kthread = kthread_run(elevator_active, e_thread, "thread elevator\n"); // thread spawns here

    return 0;
}

/* This function triggers every read! */
static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 ts_now;

    //if( (elapsed.tv_sec == 2) && (elapsed.tv_nsec == 0) ), move to next floor

    //else if( ((elapsed.tv_sec == 2) && (elapsed.tv_nsec == 0)) && ((!floorline) || (!psngrsOnElev)) ),
    //load or unload the passengers

    char buf[10000];
    int len = 0;

    len = sprintf(buf, "Elevator state: \n");
    len += sprintf(buf + len, "Current floor: \n");
    len += sprintf(buf + len, "Current load: \n");
    len += sprintf(buf + len, "Elevator status: \n");
    // you can finish the rest.

    return simple_read_from_buffer(ubuf, count, ppos, log_buffer, buf_offset);
}

/* This is where we define our procfile operations */
static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};

/* Treat this like a main function, this is where your kernel module will
   start from, as it gets loaded. */
static int __init elevator_init(void) {
//spawn elevator
    spawn_elevator(&elevator_thread);                       // this is where we spawn the thread.

    if(IS_ERR(elevator_thread.kthread)) {
        printk(KERN_WARNING "error creating thread");
        remove_proc_entry(ENTRY_NAME, PARENT);
        return PTR_ERR(elevator_thread.kthread);
    }

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
static void __exit elevator_exit(void) {

    // This is where we unlink our system calls from our stubs
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    remove_proc_entry(ENTRY_NAME, PARENT); // this is where we remove the proc file!
}

module_init(elevator_init); // initiates kernel module
module_exit(elevator_exit); // exits kernel module