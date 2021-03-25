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

- Each slab cache has both a local, per-cpu bit of slab cache info and a per-numa node bit of slab cache info
  - For optimizations, the per-cpu slab cache contains a small subset of the slabs for the slab cache
  - The per-numa node allows synchronous access to objects shared across them but is slower
- Creating a slab cache is straightforward
  - For each numa node, create a `kmem_cache_node` structure to maintain per-numa node slab cache bits
  - For each possible CPU, create a local `kmem_cache_cpu` structure to maintain the per-cpu slab cache bits
- "Free" objects are organized as a "linked list" 
  - A "free" object will actually contain a pointer to the next "free" object. ![linked list in slub](https://static.lwn.net/images/ns/kernel/slub-freelist.png)
  - Allocating from the "list" adjusts the linked list "head pointer" (`void* freelist`) to the next free object
  - Freeing from the list simply overwrites the object with a pointer to the original next free object and updates the freelist pointer
- Allocating from a slab
  - First grab the current CPU's `kmem_cache_cpu` structure and check if we have space to allocate from it. 
  - If we can not, 
    - Try and grab the per-numa node's, `kmem_cache_node` freelist. Similar to `kmem_cache_cpu` freelist, simply allocate from the first freelist free object and update accordingly
    - If there is no freelist pointer available for a specific page, then look for a partially full page within the slab, load it into the per-cpu `kmem_cache_cpu` if found, and try allocating again
    - If there is no partially full page in the slab (and thus the slab is full), simply add the slab to the local numa node's list of full slabs, allocate memory for a new slab, and set the freelist pointer appropriately

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

allocate an object from slab:
```c
mm/slub.c
void *kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags)
{                               
        void *ret = slab_alloc(s, gfpflags, _RET_IP_);

        trace_kmem_cache_alloc(_RET_IP_, ret, s->object_size, 
                                s->size, gfpflags);
                
        return ret;
}               

[...]

static __always_inline void *slab_alloc(struct kmem_cache *s,
                gfp_t gfpflags, unsigned long addr)
{
        return slab_alloc_node(s, gfpflags, NUMA_NO_NODE, addr);
}

/*
 * Inlined fastpath so that allocation functions (kmalloc, kmem_cache_alloc)
 * have the fastpath folded into their functions. So no function call
 * overhead for requests that can be satisfied on the fastpath.
 *
 * The fastpath works by first checking if the lockless freelist can be used.
 * If not then __slab_alloc is called for slow processing.
 *
 * Otherwise we can simply pick the next object from the lockless free list.
 */
static __always_inline void *slab_alloc_node(struct kmem_cache *s,
                gfp_t gfpflags, int node, unsigned long addr)
{
        void *object;
        struct kmem_cache_cpu *c;
        struct page *page;
        unsigned long tid;
        struct obj_cgroup *objcg = NULL;

        s = slab_pre_alloc_hook(s, &objcg, 1, gfpflags);
        if (!s)
                return NULL;
redo:
        /*
         * Must read kmem_cache cpu data via this cpu ptr. Preemption is
         * enabled. We may switch back and forth between cpus while
         * reading from one cpu area. That does not matter as long
         * as we end up on the original cpu again when doing the cmpxchg.
         *
         * We should guarantee that tid and kmem_cache are retrieved on
         * the same cpu. It could be different if CONFIG_PREEMPT so we need
         * to check if it is matched or not.
         */
        do {
                tid = this_cpu_read(s->cpu_slab->tid);
                c = raw_cpu_ptr(s->cpu_slab);
        } while (IS_ENABLED(CONFIG_PREEMPT) &&
                 unlikely(tid != READ_ONCE(c->tid)));

        /*
         * Irqless object alloc/free algorithm used here depends on sequence
         * of fetching cpu_slab's data. tid should be fetched before anything
         * on c to guarantee that object and page associated with previous tid
         * won't be used with current tid. If we fetch tid first, object and
         * page could be one associated with next tid and our alloc/free
         * request will be failed. In this case, we will retry. So, no problem.
         */
        barrier();

        /*
         * The transaction ids are globally unique per cpu and per operation on
         * a per cpu queue. Thus they can be guarantee that the cmpxchg_double
         * occurs on the right processor and that there was no operation on the
         * linked list in between.
         */

        object = c->freelist;
        page = c->page;
        if (unlikely(!object || !page || !node_match(page, node))) {
                object = __slab_alloc(s, gfpflags, node, addr, c);
                stat(s, ALLOC_SLOWPATH);
        } else {
                void *next_object = get_freepointer_safe(s, object);

                /*
                 * The cmpxchg will only match if there was no additional
                 * operation and if we are on the right processor.
                 *
                 * The cmpxchg does the following atomically (without lock
                 * semantics!)
                 * 1. Relocate first pointer to the current per cpu area.
                 * 2. Verify that tid and freelist have not been changed
                 * 3. If they were not changed replace tid and freelist
                 *
                 * Since this is without lock semantics the protection is only
                 * against code executing on this cpu *not* from access by
                 * other cpus.
                 */
                if (unlikely(!this_cpu_cmpxchg_double(
                                s->cpu_slab->freelist, s->cpu_slab->tid,
                                object, tid,
                                next_object, next_tid(tid)))) {

                        note_cmpxchg_failure("slab_alloc", s, tid);
                        goto redo;
                }
                prefetch_freepointer(s, next_object);
                stat(s, ALLOC_FASTPATH);
        }

        maybe_wipe_obj_freeptr(s, object);

        if (unlikely(slab_want_init_on_alloc(gfpflags, s)) && object)
                memset(object, 0, s->object_size);

        slab_post_alloc_hook(s, objcg, gfpflags, 1, &object);

        return object;
}

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
