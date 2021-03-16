# SLAB/SLUB

#### Single List of Allocated Blocks and the Unqueued SLAB Allocator

### Conceptual Overview

- A kernel object caching mechanism
  - A "kernel object" is most anything that represents some sort of kernel-specific and kernel-internal structure or item
  - For example, TCP sockets, inodes, dentrys, etc are all kernel objects
- The kernel caches most of these objects in dedicated chunks of memory called slabs
  - Slabs are grouped by object (e.g. dentrys are grouped with dentrys only)
  - The caching allows for optimization benefits through object reuse. For example, reusing objects means no overhead for memory allocation
- The slab/slub part of the kernel provides an "API" wherein the implementation (slab or slub or slob) can be dropped in easily

#### General Architectural Overview

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
- 

## SLAB

- **Note** SLAB exists in RHEL 6 and below

### Architectural Overview

## SLUB

- **Note** SLUB replaced SLAB in RHEL 7 and above

##### Structures

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
