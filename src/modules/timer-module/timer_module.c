#include <linux/init.h> 	// Required __init macro and 
#include <linux/module.h> 	// Required for module functions
#include <linux/time.h> 	// Required for msecs_to_jiffies
#include <linux/timer.h> 	// Required for mod_timer

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
 * 
 * dat_callback_yo
 *   - Registered later as the callback function for a timer. 
 */
static int __init entry_func(void);
static void __exit exit_func(void);
void dat_callback_yo(unsigned long data);

/**
 * Global structs
 *
 * timer_list my_timer
 *   - The timer itself. It is a single timer only, so do not get fooled by
 *     the type `timer_list`
 *   - Given the list_head entry member, timer_list is then technically a 
 *     doubly-linked list of timers. List is not circular.  
 *   - Global lists (TVEC_BASES, HRTIMER_CPU_BASE) exist where entries in these
 *     lists are per-cpu lists of timers (IE, tvec_bases[0] is cpu 0). Entries
 *     in these lists are the per-cpu timer lists there the timers fire on the
 *     specific CPU. The lists are ordered in ascending order of time in the 
 *     future to fire. 
 * 
 * struct timer_list {
 *   [0] struct list_head entry;	// Points to previous and next timers 
 *  [16] unsigned long expires; 	// When timer expires
 *  [24] void (*function)(unsigned long);	// callback function
 *  [32] unsigned long data; 		// ???
 *  [40] struct tvec_base *base;	// The start of the per-cpu list
 *  [48] void *start_site; 		// ???
 *  [56] char start_comm[16];		// Command name if requested from command
 *  [72] int start_pid; 		// PID of above command
 */

struct timer_list my_timer;


static int __init entry_func(void) {
        printk("Yo what it do\n");

	// Initializes the my_timer timer and sets the callback function
	// (dat_callback_yo) for the timer but indicates the timer is not a
	// high res timer (0). 
        setup_timer(&my_timer, dat_callback_yo, 0);

        // jiffies is global per-cpu var in linux/time.h
	// sets the timer to fire in about 1000 msecs into the future
        mod_timer(&my_timer, (jiffies + msecs_to_jiffies(1000)) );

        return 0;
}

static void __exit exit_func(void) {
        printk("leaving\n");
}

void dat_callback_yo(unsigned long data) {
        printk("SHAKE IT FAST\n");

	// Sets another time to fire the timer in the future. 
	// Note since this is set in the callback function called when the
	// timer fires, this perpetuates the firing on dat_callback_yo. 
        mod_timer(&my_timer, (jiffies + msecs_to_jiffies(1000)) );

}

// Regsisters entry_func as the function to call when module loads
module_init(entry_func);

// Registers exit_func as the function to call when module is removed.
module_exit(exit_func);

