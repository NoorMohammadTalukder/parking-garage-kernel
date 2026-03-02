#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NoorMohammadTalukder");
MODULE_DESCRIPTION("Parking Garage Kernel Module");

#define TOTAL_FLOORS    5
#define TOTAL_SPACES    50
#define SPACES_COMPACT  1
#define SPACES_SUV      2
#define SPACES_TRUCK    3
#define WARNING_LEVEL   10

struct Car {
    int id;
    char type[20];
    int floor;
    int spaces;
    struct list_head list;
};

struct Floor {
    int floor_number;
    int spaces_used;
    struct list_head cars;
};

static struct Floor floors[TOTAL_FLOORS + 1];
static int total_cars        = 0;
static int total_spaces_used = 0;
static int next_car_id       = 1;
static DEFINE_MUTEX(garage_mutex);
static struct task_struct *manager_thread;

static int garage_manager(void *data)
{
    int available;
    printk(KERN_INFO "Parking Manager: thread started\n");

    while (!kthread_should_stop()) {
        available = TOTAL_SPACES - total_spaces_used;
        printk(KERN_INFO
               "Parking Manager: Cars=%d Available=%d\n",
               total_cars, available);

        if (available <= WARNING_LEVEL && available > 0)
            printk(KERN_WARNING
                   "Parking Manager: WARNING! Only %d spaces left!\n",
                   available);

        if (available == 0)
            printk(KERN_WARNING
                   "Parking Manager: GARAGE IS FULL!\n");

        ssleep(5);
    }

    printk(KERN_INFO "Parking Manager: thread stopped\n");
    return 0;
}

static int enter_garage(char *type, int floor)
{
    struct Car *car;
    int spaces;

    if (floor < 1 || floor > TOTAL_FLOORS)
        return -1;

    if (strcmp(type, "compact") == 0)
        spaces = SPACES_COMPACT;
    else if (strcmp(type, "suv") == 0)
        spaces = SPACES_SUV;
    else if (strcmp(type, "truck") == 0)
        spaces = SPACES_TRUCK;
    else
        return -1;

    if (total_spaces_used + spaces > TOTAL_SPACES)
        return -2;

    car = kmalloc(sizeof(*car), GFP_KERNEL);
    if (!car) return -3;

    mutex_lock(&garage_mutex);
    car->id     = next_car_id++;
    car->floor  = floor;
    car->spaces = spaces;
    strcpy(car->type, type);
    INIT_LIST_HEAD(&car->list);
    list_add_tail(&car->list, &floors[floor].cars);
    total_cars++;
    total_spaces_used += spaces;
    mutex_unlock(&garage_mutex);

    printk(KERN_INFO "Parking: %s added as Car#%d floor %d\n",
           type, car->id, floor);
    return car->id;
}

static int exit_garage(int car_id)
{
    int f;
    struct Car *car, *tmp;

    mutex_lock(&garage_mutex);
    for (f = 1; f <= TOTAL_FLOORS; f++) {
        list_for_each_entry_safe(car, tmp,
                                 &floors[f].cars, list) {
            if (car->id == car_id) {
                list_del(&car->list);
                total_cars--;
                total_spaces_used -= car->spaces;
                printk(KERN_INFO
                       "Parking: Car#%d removed floor %d\n",
                       car_id, f);
                kfree(car);
                mutex_unlock(&garage_mutex);
                return 0;
            }
        }
    }
    mutex_unlock(&garage_mutex);
    return -1;
}

static int parking_show(struct seq_file *m, void *v)
{
    int f;
    int available;
    struct Car *car;

    mutex_lock(&garage_mutex);

    available = TOTAL_SPACES - total_spaces_used;

    seq_printf(m, "Parking Garage Status\n");
    seq_printf(m, "---------------------\n");
    seq_printf(m, "Total Floors : %d\n", TOTAL_FLOORS);
    seq_printf(m, "Total Spaces : %d\n", TOTAL_SPACES);
    seq_printf(m, "Cars Parked  : %d\n", total_cars);
    seq_printf(m, "Available    : %d\n", available);

    if (available == 0)
        seq_printf(m, "Status       : FULL!\n");
    else if (available <= WARNING_LEVEL)
        seq_printf(m, "Status       : ALMOST FULL!\n");
    else
        seq_printf(m, "Status       : OK\n");

    seq_printf(m, "\nFloor Details:\n");

    for (f = TOTAL_FLOORS; f >= 1; f--) {
        seq_printf(m, "  Floor %d: ", f);
        if (list_empty(&floors[f].cars)) {
            seq_printf(m, "[ Empty ]");
        } else {
            list_for_each_entry(car, &floors[f].cars, list) {
                seq_printf(m, "[Car#%d %s] ",
                           car->id, car->type);
            }
        }
        seq_printf(m, "\n");
    }

    mutex_unlock(&garage_mutex);
    return 0;
}

static ssize_t parking_write(struct file *file,
                              const char __user *ubuf,
                              size_t count, loff_t *ppos)
{
    char buf[64];
    char type[20];
    int  floor, car_id, ret;

    if (count > sizeof(buf) - 1)
        return -EINVAL;

    if (copy_from_user(buf, ubuf, count))
        return -EFAULT;

    buf[count] = '\0';

    if (sscanf(buf, "enter %s %d", type, &floor) == 2) {
        ret = enter_garage(type, floor);
        if (ret > 0)
            printk(KERN_INFO "Parking: Car#%d entered\n", ret);
    } else if (sscanf(buf, "exit %d", &car_id) == 1) {
        ret = exit_garage(car_id);
        if (ret == 0)
            printk(KERN_INFO "Parking: Car exited\n");
    }
    return count;
}

static int parking_open(struct inode *inode, struct file *file)
{
    return single_open(file, parking_show, NULL);
}

static const struct proc_ops parking_fops = {
    .proc_open    = parking_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops input_fops = {
    .proc_write   = parking_write,
};

static int __init parking_init(void)
{
    int i;

    for (i = 1; i <= TOTAL_FLOORS; i++) {
        floors[i].floor_number = i;
        floors[i].spaces_used  = 0;
        INIT_LIST_HEAD(&floors[i].cars);
    }

    proc_create("parking", 0, NULL, &parking_fops);
    proc_create("parking_input", 0222, NULL, &input_fops);

    manager_thread = kthread_run(garage_manager,
                                 NULL,
                                 "garage_manager");
    if (IS_ERR(manager_thread)) {
        printk(KERN_ERR "Parking: failed to start thread\n");
        return PTR_ERR(manager_thread);
    }

    printk(KERN_INFO "Parking Garage: module loaded\n");
    return 0;
}

static void __exit parking_exit(void)
{
    int i;
    struct Car *car, *tmp;

    if (manager_thread)
        kthread_stop(manager_thread);

    for (i = 1; i <= TOTAL_FLOORS; i++) {
        list_for_each_entry_safe(car, tmp,
                                 &floors[i].cars, list) {
            list_del(&car->list);
            kfree(car);
        }
    }

    remove_proc_entry("parking", NULL);
    remove_proc_entry("parking_input", NULL);
    printk(KERN_INFO "Parking Garage: module unloaded\n");
}

module_init(parking_init);
module_exit(parking_exit);
