# SLAB/SLUB

#### Single List of Allocated Blocks and the Unqueued SLAB Allocator

### Conceptual Overview

- A kernel object caching mechanism
  - A "kernel object" is most anything that represents some sort of kernel-specific and kernel-internal structure or item
  - For example, TCP sockets, inodes, dentrys, etc are all kernel objects
- The kernel caches most of these objects in dedicated chunks of memory called slabs
  - Slabs are grouped by object (e.g. dentrys are grouped with dentrys only)
  - The caching allows for optimization benefits through object reuse. For example, reusing objects means no overhead for memory allocation
- Some parts of slab can be reclaimed under memory pressure while others can not be reclaimed

### General Architectural Overview

![General slab architecture](https://www.kernel.org/doc/gorman/html/understand/understand-html037.png)
- Starting from the bottom and working our way up the picture, kernel objects of the same type are grouped together on one or more pages of contiguous memory. IE a page of slab cache holding dentrys may have several dentrys on a single page
- These contiguous pages of memory are organized into _slabs_. For example, 
  - dentrys are small, so a dentry slab may be one page in size
  - a task_struct is larger than 1 page in size and thus a single slab for task_structs will be more than one page in size
- The slabs are organized based on object type into _named slab caches_. 
  - All dentry slabs will be organized into the dentry named slab cache
  - Some slab caches exist for generic purposes where the objects are a set size (64 bytes, 1kb, etc) and sometimes are referred to as _unamed_ or _generic_ slab caches
- The lists of slabs in a slab cache are managed based on how full the list is (specifics are slab-/slub-specific)
- Slabs are kept to a per-numa node bases for locality optimizations
- The above is generic and applies to both slab and slub conceptually. The specific details, however, differ quite a bit. For example, the figure above includes 'free', 'partial', and 'full' but I did not mention such. Slub does not manage the same lists noted while slab does. 
- _Note the difference in the confusing terminology_:
  - A slab is the chunk of contiguous memory kernel objects reside in
  - Slab cache is the named/generic slab lists
  - Slab/Slub overall is the caching mechanism

### SLAB/SLUB Troubleshooting

#### Single state overview of slab usage

- `/proc/meminfo` provides memory usage in terms of KiB system wide
  ```bash
  [...]
  Slab:             128560 kB
  SReclaimable:      51348 kB
  SUnreclaim:        77212 kB
  [...]
  ```
  - `Slab:` the size of all memory consumption by slab/slub and should be the sum of `SReclaimable` and `SUnreclaim`
  - `SReclaimable:` the size of reclaimable slab caches
  - `SUnreclaim:` the size of unreclaimable cache

- `/proc/zoneinfo` provides memory usage in terms of pages on a per-zone basis
  ```bash
  Node 0, zone      DMA
    per-node stats
  [...]
        nr_slab_reclaimable 12899
        nr_slab_unreclaimable 19358
  ```
  - `nr_slab_[un]reclaimable` provides similar statistics to `/proc/meminfo` for that zone in terms of pages

- `/proc/slabinfo`
  ```bash
  slabinfo - version: 2.1
  # name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>
  nf_conntrack          48     84    320   12    1 : tunables    0    0    0 : slabdata      7      7      0
  kvm_async_pf           0      0    168   24    1 : tunables    0    0    0 : slabdata      0      0      0
  kvm_vcpu               0      0  15104    2    8 : tunables    0    0    0 : slabdata      0      0      0
  kvm_mmu_page_header      0      0    168   24    1 : tunables    0    0    0 : slabdata      0      0      0
  x86_emulator           0      0   2672   12    8 : tunables    0    0    0 : slabdata      0      0      0
  x86_fpu                0      0   4160    7    8 : tunables    0    0    0 : slabdata      0      0      0
  [...]
  ```
  - `name` the name of the slab cache
  - `pagesperslab` the amount of pages in a single slab for the named slab cache
  - `num_slabs` the amount of slabs currently allocated for the named slab cache

- As noted above, kernel objects are cached in slabs which have one or more pages of memory in it. Pages of memory used for slab can not be 
  used for things other than slab until that page is reclaimed. As such, even if a page of memory has no slab objects in it, _that page of 
  memory is still used for slab and is not free for use until reclaim_. 
- As such calculating memory usage by named slab cache is done by calculating the amount of slabs by the pages per slab: 
  ```bash
  $ grep -v -e active_objs -e slabinfo proc/slabinfo | \
      awk '{ slab_cache[$1] += ($6 * $15 * 4) } END { for (slab_name in slab_cache) { printf "%20s %10s KiB\n", slab_name, slab_cache[slab_name] } }' | \
      sort -nrk 2 | \
      head
  ```
  - The `grep` command filters out noise for the awk command to parse 
  - The `awk` command creates a list of named slab caches and associates calculates the size of each in KiB
  - The rest sorts the output based on memory consumption and prints only the top 10
  - _**Note, the above command is for x86 systems. s390 and ppc use 64 KiB page sizes so replace the '4' in the awk command with '64'**_

#### Slab growth over time

- Collectl

  - By default, collectl captures slab info which can be read with the `-s <y|Y>` subsystem options

- PCP

  - There is no default view in PCP allow checking per-slab cache breakdown of slab usage. You can use [this `pmrep` view](https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/pcp/custom-pmrep-views/slabinfo) to produce `/proc/slabinfo` like data

#### Tracking who allocates slab object

- [Main KCS](https://access.redhat.com/solutions/358933)
  - [Is there a way to track slab allocations or leaks with systemtap?](https://access.redhat.com/articles/2850581)
  - [How to track slab allocations using perf](https://access.redhat.com/solutions/2850631)
- Tracking with `slub_debug=UT,<slab cache>`
  - Tracks userspace entities doing allocations and frees (`U`) and info can be found in `/sys/kernel/slab/<slab cache>/{alloc,free}_calls`
  ```
   r7 # cat /sys/kernel/slab/dentry/alloc_calls 
  25105 __d_alloc+0x25/0x1b0 age=3/223703/229786 pid=0-8265 cpus=0-1
  ```
  - **_Use with caution_** It can overwhelm a system printing backtraces
  - You may need to expand the kernel ring buffer size as 


# The rest of this is under construction

## Slab Management Overview

- Creating a slab cache is straightforward. Calling `kmem_cache_create` performs the following steps;
  - For each numa node, create a `kmem_cache_node` structure to maintain per-numa node slab cache bits
  - For each possible CPU, create a local `kmem_cache_cpu` structure to maintain the per-cpu slab cache bits
- "Free" objects are organized as a "linked list" 
  - A "free" object will actually contain a pointer to the next "free" object. <img align="right" src="https://static.lwn.net/images/ns/kernel/slub-freelist.png">
  - Allocating from the "list" adjusts the linked list "head pointer" (`void* freelist`) to the next free object
  - Freeing from the list simply overwrites the object with a pointer to the original next free object and updates the freelist pointer
- Each slab cache has both a local, per-cpu bit of slab cache info and a per-numa node bit of slab cache info
  - For optimizations, the per-cpu slab cache, `kmem_cache_cpu` contains a small subset of the slabs for the slab cache
    - `freelist` a pointer pointing to the first free object on a page of slab
    - `page` a pointer to the slab page `freelist` may point to. This will be the first page checked when allocating locally.
    - `partial` a pointer to a slab page that has some objects allocated and others freed. A separate page pointer from `page` sometimes. This will be the backup page to check if `page` doesn't have any free objects.
  - The per-numa node slab cache bits, `kmem_cache_node` allows synchronous access to objects shared across them but is slower
    - `list_lock` prevents concurrent updates to its lists of slabs
    - `nr_partial` a count of the amount of partial slabs it has for that slab cache
    - `struct list_head partial` the list of partial slab pages
    - `nr_slabs` total amount of slabs for this slab cache on this node
    - `total_objects` the total count of objects on that numa node for that slab cache
    - `struct list_head full` the list of full slab pages

### Logicflows for Interacting With Slab

#### Allocating from a slab (`kmem_cache_alloc`)

1. First grab the current CPU's `kmem_cache_cpu` structure and check if we have space to allocate from it (IE the freelist pointer points to something). Allocate from that freelist pointer and update the pointer head as needed. 
  - This is referred to as the "fast path" 
  - `mm/slub.c`: `kmem_cache_alloc` (and the numa node specific version, `kmem_cache_alloc_node`) -> `slab_alloc_node`

    ```c
            do { 
                tid = this_cpu_read(s->cpu_slab->tid);
                c = raw_cpu_ptr(s->cpu_slab);
            } while (IS_ENABLED(CONFIG_PREEMPT) &&
                     unlikely(tid != READ_ONCE(c->tid)));
    [...]
            object = c->freelist;
            page = c->page;
            if (unlikely(!object || !page || !node_match(page, node))) {
                    object = __slab_alloc(s, gfpflags, node, addr, c);  // Enter slow path here
                    stat(s, ALLOC_SLOWPATH);
            } else {
                    void *next_object = get_freepointer_safe(s, object);
    ```

2. If we can not, go the "slow path"; if we have jumped around CPUs during this process (IE the CPU we started on isn't the CPU we are currently executing on), try again from 1.; load the per-cpu `kmem_cache_cpu` for our new CPU and see if we have space to allocate from it. If not, continue.
  - `mm/slub.c`: `slab_alloc_node` -> `__slab_alloc` -> `___slab_alloc`
  
    ```c
    __slab_alloc()
            c = this_cpu_ptr(s->cpu_slab);
    [...]
            p = ___slab_alloc(s, gfpflags, node, addr, c);
    ___slab_alloc()
            /* must check again c->freelist in case of cpu migration or IRQ */
            freelist = c->freelist;
            if (freelist)
                    goto load_freelist;
    [...]
        load_freelist:
            /*
             * freelist is pointing to the list of objects to be used.
             * page is pointing to the page from which the objects are obtained.
             * That page must be frozen for per cpu allocations to work.
             */
            VM_BUG_ON(!c->page->frozen);
            c->freelist = get_freepointer(s, freelist);
            c->tid = next_tid(c->tid);
            return freelist;
    ```
    
4. If we don't have a free object, and thus no freelist pointer, then check to see if we have a free object on the slab page we originally had in `kmem_cache_cpu`. If so, grab the next free object from the freelist on that page, update the `kmem_cache_cpu` freelist pointer, and return. Otherwise continue.
  - `mm/slub.c`: `___slab_alloc`

    ```c
    ___slab_alloc()
    
            freelist = get_freelist(s, page);
    [...]
    load_freelist:
            /*
             * freelist is pointing to the list of objects to be used.
             * page is pointing to the page from which the objects are obtained.
             * That page must be frozen for per cpu allocations to work.
             */
            VM_BUG_ON(!c->page->frozen);
            c->freelist = get_freepointer(s, freelist);
            c->tid = next_tid(c->tid);
            return freelist;
    ```

6. If we have a `partial` list, then set the backup `partial` pointer as the main `page` pointer and reattempt allocating from step 3. Otherwise continue.
  - `mm/slub.c`: `___slab_alloc`

    ```c
            if (!freelist) {
                    c->page = NULL;
                    stat(s, DEACTIVATE_BYPASS);
                    goto new_slab;
            }
    [...]
    new_slab:

            if (slub_percpu_partial(c)) {
                    page = c->page = slub_percpu_partial(c);
                    slub_set_percpu_partial(c, page);
                    stat(s, CPU_PARTIAL_ALLOC);
                    goto redo;
            }
    ```

8. Try and grab the per-numa node's, `kmem_cache_node` for the current node and check if it has any slabs on its `partial` lists. If so, grab the next partial slab for that numa node and throw it into the current cpu's `kmem_cache_node` main `page` and return the next `freelist` pointer. Otherwise, continue.
  - `mm/slub.c`: `___slab_alloc` -> `new_slab_objects`

    ```c
    ___slab_alloc()
    
            freelist = new_slab_objects(s, gfpflags, node, &c);

    new_slab_objects()

            freelist = get_partial(s, flags, node, c);

            if (freelist)
                    return freelist;
    
    ___slab_alloc()
    
            page = c->page;
            if (likely(!kmem_cache_debug(s) && pfmemalloc_match(page, gfpflags)))
                    goto load_freelist;  // refer to steps 3. or 4. for the load_freelist code path

    ```

10. At this point, the current CPU didn't have any free objects on its main page and backup `partial` page, and the current CPU's numa node also has no partial pages, so allocate a new slab! Put the `kmem_cache_cpu` slab onto the numa node's full list (within `deactivate_slab`), allocate memory for the slab, setup the slab and its `freelist`, and set the new slab to the `kmem_cache_cpu`'s backup `page` pointer. Return the new `freelist`.
  - `mm/slub.c`: `___slab_alloc` -> `deactivate_slab` -> `___slab_alloc`

    ```c
    ___slab_alloc()
    
            freelist = new_slab_objects(s, gfpflags, node, &c);

    new_slab_objects()

            freelist = get_partial(s, flags, node, c);

            if (freelist)
                    return freelist;
    
            page = new_slab(s, flags, node);
            if (page) {
                    c = raw_cpu_ptr(s->cpu_slab);
                    if (c->page)
                            flush_slab(s, c);

                    /*
                     * No other reference to the page yet so we can
                     * muck around with it freely without cmpxchg
                     */
                    freelist = page->freelist;
                    page->freelist = NULL;

                    stat(s, ALLOC_SLAB);
                    c->page = page;
                    *pc = c;
            }

            return freelist;

    ___slab_alloc()
    
            freelist = new_slab_objects(s, gfpflags, node, &c);

    [...]
    
            page = c->page;
            if (likely(!kmem_cache_debug(s) && pfmemalloc_match(page, gfpflags)))
                    goto load_freelist;  // refer to steps 3. or 4. for the load_freelist code path
    ```


allocating slab:
```c
1729 static struct page *allocate_slab(struct kmem_cache *s, gfp_t flags, int node)
1730 {
1731         struct page *page;
1732         struct kmem_cache_order_objects oo = s->oo;
1733         gfp_t alloc_gfp;
1734         void *start, *p, *next;
1735         int idx, order;
1736         bool shuffle;
1737 
1738         flags &= gfp_allowed_mask;
1739 
1740         if (gfpflags_allow_blocking(flags))
1741                 local_irq_enable();
1742 
1743         flags |= s->allocflags;
1744 
1745         /*
1746          * Let the initial higher-order allocation fail under memory pressure
1747          * so we fall-back to the minimum order allocation.
1748          */
1749         alloc_gfp = (flags | __GFP_NOWARN | __GFP_NORETRY) & ~__GFP_NOFAIL;
1750         if ((alloc_gfp & __GFP_DIRECT_RECLAIM) && oo_order(oo) > oo_order(s->min))
1751                 alloc_gfp = (alloc_gfp | __GFP_NOMEMALLOC) & ~(__GFP_RECLAIM|__GFP_NOFAIL);
1752 
1753         page = alloc_slab_page(s, alloc_gfp, node, oo);
1754         if (unlikely(!page)) {
1755                 oo = s->min;
1756                 alloc_gfp = flags;
1757                 /*
1758                  * Allocation may have failed due to fragmentation.
1759                  * Try a lower order alloc if possible
1760                  */
1761                 page = alloc_slab_page(s, alloc_gfp, node, oo);
1762                 if (unlikely(!page))
1763                         goto out;
1764                 stat(s, ORDER_FALLBACK);
1765         }
1766 
1767         page->objects = oo_objects(oo);
1768 
1769         order = compound_order(page);
1770         page->slab_cache = s;
1771         __SetPageSlab(page);
1772         if (page_is_pfmemalloc(page))
1773                 SetPageSlabPfmemalloc(page);
1774 
1775         kasan_poison_slab(page);
1776 
1777         start = page_address(page);
1778 
1779         setup_page_debug(s, start, order);
1780 
1781         shuffle = shuffle_freelist(s, page);
1782 
1783         if (!shuffle) {
1784                 start = fixup_red_left(s, start);
1785                 start = setup_object(s, page, start);
1786                 page->freelist = start;
1787                 for (idx = 0, p = start; idx < page->objects - 1; idx++) {
1788                         next = p + s->size;
1789                         next = setup_object(s, page, next);
1790                         set_freepointer(s, p, next);
1791                         p = next;
1792                 }
1793                 set_freepointer(s, p, NULL);
1794         }
1795 
1796         page->inuse = page->objects;
1797         page->frozen = 1;
1798 
1799 out:
1800         if (gfpflags_allow_blocking(flags))
1801                 local_irq_disable();
1802         if (!page)
1803                 return NULL;
1804 
1805         inc_slabs_node(s, page_to_nid(page), page->objects);
1806 
1807         return page;
1808 }
```

freeing slab:
```c
1819 static void __free_slab(struct kmem_cache *s, struct page *page)
1820 {
1821         int order = compound_order(page);
1822         int pages = 1 << order;
1823 
1824         if (kmem_cache_debug_flags(s, SLAB_CONSISTENCY_CHECKS)) {
1825                 void *p;
1826 
1827                 slab_pad_check(s, page);
1828                 for_each_object(p, s, page_address(page),
1829                                                 page->objects)
1830                         check_object(s, page, p, SLUB_RED_INACTIVE);
1831         }
1832 
1833         __ClearPageSlabPfmemalloc(page);
1834         __ClearPageSlab(page);
1835 
1836         page->mapping = NULL;
1837         if (current->reclaim_state)
1838                 current->reclaim_state->reclaimed_slab += pages;
1839         unaccount_slab_page(page, order, s);
1840         __free_pages(page, order);
1841 }
```

allocating slab page: 
```c
1603 static inline struct page *alloc_slab_page(struct kmem_cache *s,
1604                 gfp_t flags, int node, struct kmem_cache_order_objects oo)
1605 {
1606         struct page *page;
1607         unsigned int order = oo_order(oo);
1608 
1609         if (node == NUMA_NO_NODE)
1610                 page = alloc_pages(flags, order);
1611         else
1612                 page = __alloc_pages_node(node, flags, order);
1613 
1614         if (page)
1615                 account_slab_page(page, order, s);
1616 
1617         return page;
1618 }
```

adding a page to a partial list:
```c
1864 /*
1865  * Management of partially allocated slabs.
1866  */
1867 static inline void
1868 __add_partial(struct kmem_cache_node *n, struct page *page, int tail)
1869 {
1870         n->nr_partial++;
1871         if (tail == DEACTIVATE_TO_TAIL)
1872                 list_add_tail(&page->slab_list, &n->partial);
1873         else
1874                 list_add(&page->slab_list, &n->partial);
1875 }
```

remove a page from a partial list:
```c
1884 static inline void remove_partial(struct kmem_cache_node *n,
1885                                         struct page *page)
1886 {
1887         lockdep_assert_held(&n->list_lock);
1888         list_del(&page->slab_list);
1889         n->nr_partial--;
1890 }
```

##### Structures

- The slab/slub part of the kernel provides an "API" wherein the implementation (slab or slub or slob) can be dropped in easily

- `kmem_cache` the main slab cache structure containing info about the size of the objects, the function pointer for the constructor, the name, etc
  - Located in `include/linux/slub_def.h`
  - `struct kmem_cache_cpu __percpu *cpu_slab;` _Only_ the offset for each CPU's `kmem_cache` structure for this named slab cache (e.g., the offset for each CPU's `task_struct` named slab cache)
  - `struct kmem_cache_node *node[MAX_NUMNODES];` a list of per-numa node kmem_cache_node structures
- `kmem_cache_node`
  - `mm/slab.h`
- `kmem_cache_cpu`
  - `include/linux/slub_def.h`
  - `void **freelist`
  - `struct page *page`
  
##### Functions

```c
struct kmem_cache *
kmem_cache_create(const char *name, unsigned int size, unsigned int align,
                slab_flags_t flags, void (*ctor)(void *))
```

Creates a new slab cache
- `const char *name` the name of the slab cache to be created
- `unsigned int size` size in bytes of the objects to be stored in the slab cache
- `unsigned int align` alignment factor to shift addresses of objects to align with particular boundaries. 
  - The page size for the x86 linux kernel is 4K so addresses tend to be aligned to values evenly divisible by 4K when fetching something in memory
- `slab_flags_t flags` bitmask of flags to indicate the state of the slab cache, what debugging options are enabled for it, etc.
- `void (*ctor)(void *)` function pointer used as object constructor, IE when creating a slab object, the constructor function pointer is called to setup a blank object for use as defined by the creator


### Architectural Overview

## References

- [The SLUB allocator](https://lwn.net/Articles/229984/)
- [The Slab Allocator in the Linux kernel](https://lwn.net/Articles/229984/)
- [Overview of Linux Memory Management Concepts: Slabs](http://www.secretmango.com/jimb/Whitepapers/slabs/slab.html)
- [Understanding The Linux Kernel: Chapter 8 Slab Allocator](https://www.kernel.org/doc/gorman/html/understand/understand011.html)
- [The Slab Allocator in the Linux kernel](https://hammertux.github.io/slab-allocator)
- [Issue 2022: Linux: SLUB bulk alloc slowpath omits required TID increment
](https://bugs.chromium.org/p/project-zero/issues/detail?id=2022) (this talks about transaction ids)
