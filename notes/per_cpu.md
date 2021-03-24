[A brief introduction to per-cpu variables](https://thinkiii.blogspot.com/2014/05/a-brief-introduction-to-per-cpu.html)

- The statically defined per-cpu variables are what is printed with the top of `sym -l` in crash
- For dynamically defined per-cpu, the value of the variable is typically an offset, 
  and you have to use the CPU offset into its block of per-cpu stuff to check the specific CPU's stuff

#### Find statically allocated per-cpu stuff
```
crash> p <the variable>:N
```
- Where `N` is the CPU id in question or `a` for all CPUs
- `<the variable>` is the variable name of interest
  - `crash> sym -l` would show the offsets and their names
EXAMPLE
```
crash> p this_cpu_off
PER-CPU DATA TYPE:
  unsigned long this_cpu_off;
PER-CPU ADDRESSES:
  [0]: ffff8a1f3ba0f180
  [1]: ffff8a1f3bb0f180

crash> p this_cpu_off:a
per_cpu(this_cpu_off, 0) = $12 = 0xffff8a1f3ba00000
per_cpu(this_cpu_off, 1) = $13 = 0xffff8a1f3bb00000

crash> p this_cpu_off:0
per_cpu(this_cpu_off, 0) = $16 = 0xffff8a1f3ba00000
```

#### Find dynamically allocated per-cpu stuff
- EITHER OR:
  1. `crash> p this_cpu_off:N` where `N` is either a CPU id number (e.g. `3`) or `a` for all, then add the per-cpu value to the cpu offset base
  2. `help -m` `ibase[cpus]` section is the per-cpu offset base (and thus the same value as what is found in 1.)
- EXAMPLE
  - Retriving the `kmem_cache` for the `task_struct` slab cache:
    ```
    crash> kmem -s | grep task_struct
    ffff8a1f07d50a00     6080        188       215     43    32k  task_struct
    ```
  - Get the per-cpu offset for the `task_struct` slab cache:
    ```
    crash> struct kmem_cache.cpu_slab ffff8a1f07d50a00
    cpu_slab = 0x30740
    ```
  - Get the per-cpu base address for a cpu
    ```
    crash> p this_cpu_off:a
    per_cpu(this_cpu_off, 0) = $8 = 0xffff8a1f3ba00000
    per_cpu(this_cpu_off, 1) = $9 = 0xffff8a1f3bb00000
    
    OR
    
    crash> help -m | grep -A 1 ibase
                           ibase[cpus]:
       ffff8a1f3ba00000 ffff8a1f3bb00000 
    ```
  - Calculate the `task_struct`'s `kmem_cache_cpu` address for one of those CPUs
    ```
    crash> px 0xffff8a1f3ba00000 + 0x30740
    $10 = 0xffff8a1f3ba30740
    ```
  - Go look!
    ```
    crash> struct kmem_cache_cpu 0xffff8a1f3ba30740
    struct kmem_cache_cpu {
      freelist = 0xffff8a1f3a4f0000, 
      tid = 0xb32, 
      page = 0xffffe2eac4e93c00, 
      partial = 0x0
    }
    ```



