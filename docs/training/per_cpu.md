# Per-CPU Infrastructure

#### The ins and outs of how the Linux kernel handles local, cpu-specific variables

## Overview

#### Statically-defined per-cpu variables

- Per-CPU variables are sometimes referred to as `pcp`, unrelated to performance co-pilot.
- Similar to statically and dynamically defined variables in programs, the kernel allocates statically defined variables on boot 
- Each CPU has an area of memory dedicated to statically and dynamically defined variables
- Each CPU contains a _base address_ denoting its starting point for its copies of the per-cpu variables
- Statically defined variables are declared via the `DEFINE_PER_CPU` macro
  - The definition signature is `DEFINE_PER_CPU(type, name)` where `type` is a struct type and `name` is what to call the per-cpu variable 
  - E.g. `static DEFINE_PER_CPU(struct delayed_work, vmstat_work);` creates a per-cpu variable named `vmstat_work` that is of type `delayed_work`, a struct type defined in `include/linux/workqueue.h`
  - Variations exist for the macro which optimize based on various things
    - `DEFINE_PER_CPU_ALIGNED` creates cache-line aligned per-cpu variables
    - `DEFINE_PER_CPU_READ_MOSTLY` creates per-cpu variables where the CPU can confidently prefetch instructions for it
  - Inherently, because these are defined on boot, no functions really exist to deallocate the memory allocated via `DEFINE_PER_CPU`.

#### Dynamically-allocated per-cpu variables

- Dynamically-allocated per-cpu variables typically hold an offset as its value
- Similar to statically-defined per-cpu variables which are located at an offset from the start of its relative CPU base address, the offset for dynamically-allocated variables help point to the location of the variable in memory as an offset from any CPU base address
- Dynamically-allocated variables are defined with the `__percpu` declaration
  - E.g. `struct kmem_cache_cpu __percpu *cpu_slab;` defines a variable named `cpu_slab` of type `kmem_cache_cpu`. This variable will exist for each CPU via the `__percpu` directive.
  - Other functions/macros exist which ultimately call down to allocations made with the `__percpu` directive like `alloc_percpu(type)` and `__alloc_percpu(size, align)`
  - These functions/macros are defined in `mm/percpu.c` where some functions/macros truly allocate per-cpu variables while others simply allocate from SLUB or virtual memory.
- Dynamically-allocated variables can be freed with `free_percpu(void*)`

## Finding Values 

#### Find statically-defined per-cpu variable values

- You can list statically-defined variables in crash with `sym -l` in crash. This does not print their values.
- To print the values, print the variable along with a CPU id:

    ```
    crash> p <the variable>[:N]
    ```
    - `N` is the CPU id in question or `a` for all CPUs
    - `<the variable>` is the variable name of interest
    - If `:N` is not provided, crash will print the pointer to those variables

- Below is an example of these commands

  ```
  crash> sym -l | grep this_cpu_off
  f180 (D) this_cpu_off

  crash> p this_cpu_off
  PER-CPU DATA TYPE:
    unsigned long this_cpu_off;
  PER-CPU ADDRESSES:
    [0]: ffff8a1f3ba0f180   # these are the pointers
    [1]: ffff8a1f3bb0f180

  crash> p this_cpu_off:a
  per_cpu(this_cpu_off, 0) = $12 = 0xffff8a1f3ba00000  # these are their values
  per_cpu(this_cpu_off, 1) = $13 = 0xffff8a1f3bb00000

  crash> p this_cpu_off:0
  per_cpu(this_cpu_off, 0) = $16 = 0xffff8a1f3ba00000
  ```

#### Find dynamically-allocated per-cpu variable values 

- Dynamically-allocated variables do not have a dedicated address on boot and thus are not listed with `crash> sym -l`
- As such, finding such a variable's value requires extracting its offset and calculating its address based off of the per-cpu base address of the CPU in question 

  - E.g.
    ```
    crash> p this_cpu_off:0
    per_cpu(this_cpu_off, 0) = $16 = 0xffff8a1f3ba00000
    ```
  - Then add the base address to the offset value in question. 

- The below example walks through finding the per-cpu `task_struct` slab cache: 

  1. The `kmem_cache` struct manages a slab cache. It contains the per-cpu variable `cpu_slab` which points to the slab cache's per-cpu copy:

      ```
      struct kmem_cache {
          struct kmem_cache_cpu __percpu *cpu_slab;
          [...]
      };
      ```
  
  2. Retriving the `kmem_cache` pointer for the `task_struct` slab cache:
  
      ```
      crash> kmem -s | grep task_struct
      ffff8a1f07d50a00     6080        188       215     43    32k  task_struct
      ^^^^^^^^^^^^^^^^
      ```

  3. Get the per-cpu offset for the `task_struct` slab cache from its `kmem_cache`:

      ```
      crash> struct kmem_cache.cpu_slab ffff8a1f07d50a00
      cpu_slab = 0x30740   # this is the offset
      ```

  4. Get the per-cpu base address for a cpu

      ```
      crash> p this_cpu_off:0
      per_cpu(this_cpu_off, 0) = $16 = 0xffff8a1f3ba00000
      ```

  5. Calculate the per-cpu copy of the `task_struct` slab cache for CPU 0

      ```
      crash> px 0xffff8a1f3ba00000 + 0x30740
      $10 = 0xffff8a1f3ba30740
      ```
    
  6. Go look!

      ```
      crash> struct kmem_cache_cpu 0xffff8a1f3ba30740
      struct kmem_cache_cpu {
        freelist = 0xffff8a1f3a4f0000, 
        tid = 0xb32, 
        page = 0xffffe2eac4e93c00, 
        partial = 0x0
      }
      ```
      
## Interacting with certain Per CPU variables

- Pcp variables have a number of macros defined to help facilitate interacting with certain pcp variables:

  ```c
  include/linux/percpu-defs.h
  [...]
  #define this_cpu_ptr(ptr)                                               \
  ({                                                                      \
          __verify_pcpu_ptr(ptr);                                         \
          SHIFT_PERCPU_PTR(ptr, my_cpu_offset);                           \
  })
  [...]
  /*
   * Operations with implied preemption/interrupt protection.  These
   * operations can be used without worrying about preemption or interrupt.
   */
  #define this_cpu_read(pcp)              __pcpu_size_call_return(this_cpu_read_, pcp)
  #define this_cpu_write(pcp, val)        __pcpu_size_call(this_cpu_write_, pcp, val)
  #define this_cpu_add(pcp, val)          __pcpu_size_call(this_cpu_add_, pcp, val)
  #define this_cpu_and(pcp, val)          __pcpu_size_call(this_cpu_and_, pcp, val)
  #define this_cpu_or(pcp, val)           __pcpu_size_call(this_cpu_or_, pcp, val)
  #define this_cpu_add_return(pcp, val)   __pcpu_size_call_return2(this_cpu_add_return_, pcp, val)
  #define this_cpu_xchg(pcp, nval)        __pcpu_size_call_return2(this_cpu_xchg_, pcp, nval)
  #define this_cpu_cmpxchg(pcp, oval, nval) \
          __pcpu_size_call_return2(this_cpu_cmpxchg_, pcp, oval, nval)
  #define this_cpu_cmpxchg_double(pcp1, pcp2, oval1, oval2, nval1, nval2) \
          __pcpu_double_call_return_bool(this_cpu_cmpxchg_double_, pcp1, pcp2, oval1, oval2, nval1, nval2)

  #define this_cpu_sub(pcp, val)          this_cpu_add(pcp, -(typeof(pcp))(val))
  #define this_cpu_inc(pcp)               this_cpu_add(pcp, 1)
  #define this_cpu_dec(pcp)               this_cpu_sub(pcp, 1)
  #define this_cpu_sub_return(pcp, val)   this_cpu_add_return(pcp, -(typeof(pcp))(val))
  #define this_cpu_inc_return(pcp)        this_cpu_add_return(pcp, 1)
  #define this_cpu_dec_return(pcp)        this_cpu_add_return(pcp, -1)
  ```
  
  - The macro names indicate what the functionality is. E.g. 
    - `this_cpu_read` implies simply reading a pcp variable
    - `this_cpu_inc` implies incrementing the pcp variable
    - `this_cpu_ptr` implies fetching the pointer to the current CPU's pcp variable in question

## References

- [A brief introduction to per-cpu variables](https://thinkiii.blogspot.com/2014/05/a-brief-introduction-to-per-cpu.html)
- [The New percpu Interface](http://books.gigatux.nl/mirror/kerneldevelopment/0672327201/ch11lev1sec11.html)

