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

- `/proc/meminfo` provides memory usage in terms of KiB system wide

```bash
[...]
Slab:             128560 kB
SReclaimable:      51348 kB
SUnreclaim:        77212 kB
[...]
```

- Field description:

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

- Field descriptions

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

- There's a good amount of columns here, but the key ones are as follows:

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

- Description

  - The `grep` command filters out noise for the awk command to parse 
  - The `awk` command creates a list of named slab caches and associates calculates the size of each in KiB
  - The rest sorts the output based on memory consumption and prints only the top 10
  - _**Note, the above command is for x86 systems. s390 and ppc use 64 KiB page sizes so replace the '4' in the awk command with '64'**_

- Collectl

  - By default, collectl captures slab info which can be read with the `-s <y|Y>` subsystem options

- PCP

  - There is no default view in PCP allow checking per-slab cache breakdown of slab usage. You can use [this `pmrep` view](https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/pcp/custom-pmrep-views/slabinfo) to produce `/proc/slabinfo` like data

# The rest of this is under construction

## SLAB

- **Note** SLAB exists in RHEL 6 and below

### Architectural Overview

## SLUB

- **Note** SLUB replaced SLAB in RHEL 7 and above

##### Structures

- The slab/slub part of the kernel provides an "API" wherein the implementation (slab or slub or slob) can be dropped in easily
- `kmem_cache` the main slab cache structure containing info about the size of the objects, the function pointer for the constructor, the name, etc
  - `struct kmem_cache_node *node[MAX_NUMNODES];` a list of per-numa node 

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
