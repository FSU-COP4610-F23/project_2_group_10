#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>

#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("syscalls written to procfile with proc entry");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL
#define LOG_BUF_LEN 8192
#define MAX_LOAD 750

static char buf[LOG_BUF_LEN];
static int len = 0;
static bool started;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);


enum status {OFFLINE, IDLE, LOADING, UP, DOWN};      // you should note that enums are just integers.

struct student {
    char year;
    int weight;
    int destination;
    struct list_head student;
};

struct elevator {
    int currentFloor;
    int destination;
    enum status state;
    int numPassengers;
    int numServed;
    int load;
    struct list_head passengers;
    struct task_struct *kthread;    // this is the struct to make a kthread.
};

struct floor {
    int numWaitingStud;
    struct list_head studentsWaiting;
};

struct building {
    int numFloors;
    int totalWaitingStuds;
    struct floor floors[6];
};

static struct proc_dir_entry *proc_entry;
static struct elevator elevator_thread;
static struct building thisBuilding;
static DEFINE_MUTEX(start_mutex);
static DEFINE_MUTEX(elevator_mutex);
static DEFINE_MUTEX(building_mutex);


/* This function is called when when the start_elevator system call is called */
int start_elevator(void) {
    mutex_lock(&start_mutex);
    started = true;
    mutex_unlock(&start_mutex);
    return 0;
}

char intToYear(int year) {
    switch(year) {
        case 0:
            return 'F';
        case 1:
            return 'O';
        case 2:
            return 'J';
        case 3:
            return 'S';
        default:
            return 'X';
    }
}

int yearToWeight(int year) {
    switch(year) {
        case 0:
            return 100;
        case 1:
            return 150;
        case 2:
            return 200;
        case 3: 
            return 250;
        default:
            return 0; 
    }
}

char* enumToState(int stateNum) {
    switch(stateNum) {
        case 0:
            return "OFFLINE";
        case 1:
            return "IDLE";
        case 2:
            return "LOADING";
        case 3: 
            return "UP";
        case 4:
            return "DOWN";
        default:
            return "STOP"; 
    }
}

/* This function is called when when the issue_request system call is called */
int issue_request(int start_floor, int destination_floor, int type) {
    struct student * new_student = kmalloc(sizeof(struct student), GFP_KERNEL);
    if(!new_student) {
        printk(KERN_INFO "Error: could not allocate memory for new student\n");
        return -ENOMEM;
    }

    new_student->year = intToYear(type);
    new_student->weight = yearToWeight(type);
    new_student->destination = destination_floor;

	int adjusted_start_floor = start_floor - 1;

    mutex_lock(&building_mutex);
    list_add_tail(&new_student->student, &thisBuilding.floors[adjusted_start_floor].studentsWaiting);
    thisBuilding.totalWaitingStuds += 1;
    thisBuilding.floors[start_floor].numWaitingStud += 1;
    mutex_unlock(&building_mutex);
    return 0;
}

/* This function is called when when the stop_elevator system call is called */
int stop_elevator(void) {
    mutex_lock(&start_mutex);
    started = false;
    mutex_unlock(&start_mutex);
    return 0;
}

/* Function to process elevator state */
void process_elevator_state(struct elevator * e_thread) {
    struct student* student, * next;
    struct student *next_student;

    switch(e_thread->state) {
        case LOADING:
            ssleep(1);                    // sleeps for 1 second, before processing next stuff!
            mutex_lock(&elevator_mutex);
			
			list_for_each_entry_safe(student, next, &e_thread->passengers, student) {
                if (student->destination == e_thread->currentFloor) {
                    e_thread->numServed += 1;
                    e_thread->load -= student->weight;
                    list_del(&student->student);
                    kfree(student);
					e_thread->numPassengers--;
					
					if(e_thread->load == 0)
					{
						e_thread->destination = 0;
						e_thread->state = IDLE;
					}
                }
            }
	
			mutex_unlock(&elevator_mutex);
			
			int adjusted_current_floor = e_thread->currentFloor - 1;
			
            if (started) {
            mutex_lock(&building_mutex);
            list_for_each_entry_safe(student, next, &thisBuilding.floors[adjusted_current_floor].studentsWaiting, student) {
                if (e_thread->load + student->weight <= MAX_LOAD) {
                    list_move_tail(&student->student, &e_thread->passengers);
					e_thread->load += student->weight;
					thisBuilding.totalWaitingStuds -= 1;
					thisBuilding.floors[adjusted_current_floor + 1].numWaitingStud -= 1;
					e_thread->numPassengers++;
					
					}
			}
            mutex_unlock(&building_mutex);
            }

            //error check this if needed
            if (e_thread->numPassengers != 0) {
                next_student = list_entry(e_thread->passengers.next, struct student, student);
                e_thread->destination = next_student->destination;
                if (e_thread->destination > e_thread->currentFloor)
                    e_thread->state = UP;
                else e_thread->state = DOWN;
            }
            else
			{
                e_thread->state = IDLE;
            }
			
            break;
        case UP:
            ssleep(2);                                      // sleeps for 2 seconds, before processing next stuff!
            mutex_lock(&elevator_mutex);
			
            if (e_thread->currentFloor != e_thread->destination)
			{
				if(thisBuilding.floors[e_thread->currentFloor + 1].numWaitingStud > 0 && e_thread->load < 750 && started == true)
				{
					e_thread->state = LOADING;
				}
                e_thread->currentFloor += 1;
            }
			else
			{
				e_thread->state = LOADING;                   // changed states!
			}
            mutex_unlock(&elevator_mutex);
            break;
        case DOWN:
            ssleep(2);
            mutex_lock(&elevator_mutex);
            if (e_thread->currentFloor != e_thread->destination)
			{
				if(thisBuilding.floors[e_thread->currentFloor - 1].numWaitingStud > 0 && e_thread->load < 750 && started == true)
				{
					e_thread->state = LOADING;
				}
                e_thread->currentFloor -= 1;
            }
			else e_thread->state = LOADING;
            mutex_unlock(&elevator_mutex);
            break;
        case OFFLINE:
            ssleep(.3);
            mutex_lock(&start_mutex);
            mutex_lock(&elevator_mutex);
            if (started){
                e_thread->state = IDLE;
                
            }
            mutex_unlock(&start_mutex);
            mutex_unlock(&elevator_mutex);
            break;
        case IDLE:
			ssleep(.3);
            mutex_lock(&elevator_mutex);
			
            if(!started){
                e_thread->state = OFFLINE;
                mutex_unlock(&elevator_mutex);
                break;
            }
            mutex_lock(&building_mutex);
			// Check for passengers on the current floor
			bool foundPassenger = false;
			if (thisBuilding.floors[e_thread->currentFloor].numWaitingStud > 0) {
				foundPassenger = true;
				e_thread->state = LOADING;
			} else {
				// Check other floors for waiting passengers
				//hmmm...

				for (int i = 1; i <= thisBuilding.numFloors && !foundPassenger; i++) {
					if (e_thread->currentFloor + i <= thisBuilding.numFloors && thisBuilding.floors[e_thread->currentFloor + i].numWaitingStud > 0) {
						e_thread->destination = e_thread->currentFloor + i;
						e_thread->state = UP;
						foundPassenger = true;
						break;
					} else if (e_thread->currentFloor - i >= 1 && thisBuilding.floors[e_thread->currentFloor - i].numWaitingStud > 0) {
						e_thread->destination = e_thread->currentFloor - i;
						e_thread->state = DOWN;
						foundPassenger = true;
						break;
					}
				}
				if (!foundPassenger) {
					e_thread->state = IDLE; // No passengers waiting anywhere
				}
			}
			mutex_unlock(&building_mutex);
			mutex_unlock(&elevator_mutex);
			break;
			}
}

int elevator_active(void * _elevator) {
    struct elevator * e_thread = (struct elevator *) _elevator;
    printk(KERN_INFO "elevator thread has started running \n");
    int full = 0;
    while(!kthread_should_stop() || full > 0) {
        process_elevator_state(e_thread);
        mutex_lock(&elevator_mutex);
        full = e_thread->numPassengers;
        mutex_unlock(&elevator_mutex);
    }
    mutex_lock(&start_mutex);
    started = false;
    mutex_unlock(&start_mutex);
    mutex_lock(&elevator_mutex);
    e_thread->state = OFFLINE;
    mutex_unlock(&elevator_mutex);
    
    return 0;
}

int spawn_elevator(struct elevator * e_thread) {
    mutex_lock(&start_mutex);
    started = false;
    mutex_unlock(&start_mutex);
    // initialize and/or allocate everything inside building struct and everything it points to
    mutex_lock(&building_mutex);
    thisBuilding.numFloors = 6;
    thisBuilding.totalWaitingStuds = 0;
    for (int i = 0; i < thisBuilding.numFloors; i++) {
        thisBuilding.floors[i].numWaitingStud = 0;
        INIT_LIST_HEAD(&thisBuilding.floors[i].studentsWaiting);
    }
    mutex_unlock(&building_mutex);

    // initialize and/or allocate everything inside elevator struct and everything it points to
    mutex_lock(&elevator_mutex);
    e_thread->currentFloor = 1;
    e_thread->destination = 0;
    e_thread->state = OFFLINE;
    e_thread->numPassengers = 0;
    e_thread->numServed = 0;
    e_thread->load = 0;
    INIT_LIST_HEAD(&elevator_thread.passengers);
    e_thread->kthread = kthread_run(elevator_active, e_thread, "thread elevator\n"); // thread spawns here
    mutex_unlock(&elevator_mutex);

    return 0;
}

/* This function triggers every read! */
static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct student* student;

    mutex_lock(&elevator_mutex);
    //Recall that enums are just integer
    len = snprintf(buf, sizeof(buf), "Elevator state: %s\n", enumToState(elevator_thread.state));
    len += snprintf(buf + len, sizeof(buf), "Current floor: %d\n", elevator_thread.currentFloor);
    len += snprintf(buf + len, sizeof(buf), "Current load: %d\n", elevator_thread.load);
    mutex_unlock(&elevator_mutex);
    
    mutex_lock(&elevator_mutex);
    len += snprintf(buf + len, sizeof(buf), "Elevator status:");
    //Print all passengers here in a loop.
    list_for_each_entry(student, &elevator_thread.passengers, student) {
        len += snprintf(buf + len, sizeof(buf), " %c%d", student->year, student->destination);
    }
    len += snprintf(buf + len, sizeof(buf), "\n\n");
    mutex_unlock(&elevator_mutex);

    //Print the "elevator" here.
    for(int i = 6; i > 0; i--){
        len += snprintf(buf + len, sizeof(buf), "[");

        mutex_lock(&elevator_mutex);
        if(i == elevator_thread.currentFloor){
            len += snprintf(buf + len, sizeof(buf), "*");
        }
        else{len += snprintf(buf + len, sizeof(buf), " ");}
        len += snprintf(buf + len, sizeof(buf), "] Floor %d: ", i);
        mutex_unlock(&elevator_mutex);
        //Print out the linked list of students here if it's not empty

        mutex_lock(&building_mutex);
        list_for_each_entry(student, &thisBuilding.floors[i-1].studentsWaiting, student) {
            len += snprintf(buf + len, sizeof(buf), "%c%d ", student->year, student->destination);
        }
        mutex_unlock(&building_mutex);

        len += snprintf(buf + len, sizeof(buf), "\n");
    }
    len += snprintf(buf + len, sizeof(buf), "\n");

    mutex_lock(&elevator_mutex);
    len += snprintf(buf + len, sizeof(buf), "Number of passengers: %d\n", elevator_thread.numPassengers);
    mutex_unlock(&elevator_mutex);
    
    mutex_lock(&building_mutex);
    len += snprintf(buf + len, sizeof(buf), "Number of passengers waiting: %d\n", thisBuilding.totalWaitingStuds);
    mutex_unlock(&building_mutex);
    
    mutex_lock(&elevator_mutex);
    len += snprintf(buf + len, sizeof(buf), "Number of passengers serviced: %d\n", elevator_thread.numServed);
    mutex_unlock(&elevator_mutex);
    // you can finish the rest.

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
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
    struct student * student, * next;
	int i;

	if(elevator_thread.kthread)
	{
		kthread_stop(elevator_thread.kthread);
	}

	for(i = 0; i < thisBuilding.numFloors; i++)
	{
		list_for_each_entry_safe(student, next, &thisBuilding.floors[i].studentsWaiting, student) {
			list_del(&student->student);
			kfree(student);
		}
	}

    // This is where we unlink our system calls from our stubs
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    remove_proc_entry(ENTRY_NAME, PARENT); // this is where we remove the proc file!
}

module_init(elevator_init); // initiates kernel module
module_exit(elevator_exit); // exits kernel module
