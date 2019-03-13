#include <linux/init.h>      // Required __init macro and 
#include <linux/module.h>    // Required for module functions
#include <linux/kernel.h>    
#include <linux/vmalloc.h>   // Provides vmalloc()
#include <linux/random.h>    // Provides get_random_bytes()

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
noinline void eat_mem(void);


#define VMALLOC_SZ 1024 * 4  // 4 KiB of memory 


static int __init entry_func(void) {
    eat_mem();
    return 0;
}

/**
 * Chew through memory one page at a time
 */
noinline void eat_mem() {
    printk("Imma eat yo memz\n");
    for(;;) {
        void* buffer = vmalloc(VMALLOC_SZ);
        get_random_bytes(buffer, VMALLOC_SZ);
    }
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

