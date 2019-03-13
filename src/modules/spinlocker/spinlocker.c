#include <linux/init.h> 	// Required __init macro and 
#include <linux/module.h> 	// Required for module functions
#include <linux/spinlock.h> // Provides spinlock functionality 
#include <linux/kthread.h>  // for threads
#include <linux/sched.h>    // for task_struct
#include <linux/time.h>		// provides timestamp abilities

/**
 * Prototypes
 * 
 * entry_func
 *   - is registered later as the function to call when the module is loaded
 *   - `__init` causes the function to be discarded and its memory freed once
 *     it finishes executing. 
 *      - Apparently, this applies only to built in modules and is ignored in
 *        loadable modules? goo.gl/UDaO5q, goo.gl/WixTkW
 * 
 * exit_func
 *   - Registered later as the callback function when the module is removed
 *   - `__exit` will omit the marked function all together for built in modules
 *     but does nothing for loadable ones.
 */
static int __init mod_init(void);
static void __exit mod_cleanup(void);
static int spinner(void);

struct spinlock_t *my_lock;
static struct task_struct *t_1;
static struct task_struct *t_2;


static int __init mod_init(void) {
	printk("SPINLOCKER: initializing\n");
	//spin_lock_init(my_lock);
	char t_1_name[12] = "spinlocker1";
	t_1 = kthread_create(spinner, NULL, t_1_name);
	//t_2 = kthread_create(spinner, NULL, "spinlocker2");
	
	if ((t_1)) {
		printk("SPINLOCKER: waking up t_1: spinlocker1\n");
		wake_up_process(t_1);
	}



        return 0;
}

static void __exit mod_cleanup(void) {
        printk("leaving\n");
}

static int spinner() {
	unsigned long long i;
	unsigned long long throwaway;

	struct timespec start_time;
	struct timespec end_time;

	while (1) {
		getnstimeofday(&start_time);
		for (i = 0ULL; i < 10000000000000000000000ULL; i++) {
			throwaway = i / 2ULL;
		}
		/*
		printk("SPINLOCKER: throwaway: %llu\n", throwaway);
		getnstimeofday(&end_time);
		printk("SPINLOCKER: start time: %lu.%lu, end time: %lu.%lu\n",
				start_time.tv_sec, start_time.tv_nsec, 
				end_time.tv_sec, end_time.tv_nsec);
		printk("SPINLOCKER: iteration took %lu.%lu\n",
				(end_time.tv_sec - start_time.tv_sec),
				(end_time.tv_nsec - start_time.tv_nsec));
		*/
	}
	return 0;
}

// Regsisters entry_func as the function to call when module loads
module_init(mod_init);

// Registers exit_func as the function to call when module is removed.
module_exit(mod_cleanup);

