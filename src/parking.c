#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Parking Garage Kernel Module");

/* ─── Constants ─────────────────────── */
#define TOTAL_FLOORS    5
#define TOTAL_SPACES    50
#define SPACES_COMPACT  1
#define SPACES_SUV      2
#define SPACES_TRUCK    3

/* ─── Car Struct ─────────────────────── */
struct Car {
    int id;
    char type[20];
    int floor;
    int spaces;
    struct list_head list;  /* kernel linked list */
};

/* ─── Floor Struct ───────────────────── */
struct Floor {
    int floor_number;
    int spaces_used;
    struct list_head cars;  /* list of cars on floor */
};

/* ─── Globals ────────────────────────── */
static struct Floor floors[TOTAL_FLOORS + 1]; /* 1 to 5 */
static int total_cars   = 0;
static int total_spaces_used = 0;
static int next_car_id  = 1;
static DEFINE_MUTEX(garage_mutex);  /* lock for safety */

/* ─── /proc/parking display ──────────── */
static int parking_show(struct seq_file *m, void *v)
{
    int f;
    struct Car *car;

    mutex_lock(&garage_mutex);

    seq_printf(m, "Parking Garage Status\n");
    seq_printf(m, "---------------------\n");
    seq_printf(m, "Total Floors : %d\n", TOTAL_FLOORS);
    seq_printf(m, "Total Spaces : %d\n", TOTAL_SPACES);
    seq_printf(m, "Cars Parked  : %d\n", total_cars);
    seq_printf(m, "Available    : %d\n",
               TOTAL_SPACES - total_spaces_used);
    seq_printf(m, "\nFloor Details:\n");

    /* Print from top floor to bottom */
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

/* ─── proc file setup ────────────────── */
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

/* ─── Module Init ────────────────────── */
static int __init parking_init(void)
{
    int i;

    /* Initialize all floors */
    for (i = 1; i <= TOTAL_FLOORS; i++) {
        floors[i].floor_number = i;
        floors[i].spaces_used  = 0;
        INIT_LIST_HEAD(&floors[i].cars);
    }

    /* Add 2 test cars so we can see them */
    struct Car *car1 = kmalloc(sizeof(*car1), GFP_KERNEL);
    car1->id     = next_car_id++;
    car1->floor  = 3;
    car1->spaces = SPACES_COMPACT;
    strcpy(car1->type, "Compact");
    INIT_LIST_HEAD(&car1->list);
    list_add_tail(&car1->list, &floors[3].cars);
    total_cars++;
    total_spaces_used += car1->spaces;

    struct Car *car2 = kmalloc(sizeof(*car2), GFP_KERNEL);
    car2->id     = next_car_id++;
    car2->floor  = 3;
    car2->spaces = SPACES_SUV;
    strcpy(car2->type, "SUV");
    INIT_LIST_HEAD(&car2->list);
    list_add_tail(&car2->list, &floors[3].cars);
    total_cars++;
    total_spaces_used += car2->spaces;

    struct Car *car3 = kmalloc(sizeof(*car3), GFP_KERNEL);
    car3->id     = next_car_id++;
    car3->floor  = 1;
    car3->spaces = SPACES_TRUCK;
    strcpy(car3->type, "Truck");
    INIT_LIST_HEAD(&car3->list);
    list_add_tail(&car3->list, &floors[1].cars);
    total_cars++;
    total_spaces_used += car3->spaces;

    proc_create("parking", 0, NULL, &parking_fops);
    printk(KERN_INFO "Parking Garage: module loaded\n");
    return 0;
}

/* ─── Module Exit ────────────────────── */
static void __exit parking_exit(void)
{
    int i;
    struct Car *car, *tmp;

    /* Free all cars from memory */
    for (i = 1; i <= TOTAL_FLOORS; i++) {
        list_for_each_entry_safe(car, tmp,
                                 &floors[i].cars, list) {
            list_del(&car->list);
            kfree(car);
        }
    }

    remove_proc_entry("parking", NULL);
    printk(KERN_INFO "Parking Garage: module unloaded\n");
}

module_init(parking_init);
module_exit(parking_exit);
