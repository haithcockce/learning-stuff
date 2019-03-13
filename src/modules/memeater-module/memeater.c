#include <linux/init.h> 	// Required __init macro and 
#include <linux/module.h> 	// Required for module functions

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
    going_down();
    return 0;
}

noinline void going_down() {
    printk("Aw shit!\n");
    char* killer = NULL;
    *killer = 1;
}

/**
 * Unecessesary to have this since the module will crash on loading. Leaving
 * it for learning purposes so I do not have conflicting program contents.
 */
static void __exit exit_func(void) {
    printk("leaving\n");
}

// Regsisters entry_func as the function to call when module loads
module_init(entry_func);

// Registers exit_func as the function to call when module is removed.
module_exit(exit_func);

