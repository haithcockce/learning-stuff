# Page

### Page Flags

- In 32-bit systems, `flags` the left-most 6 bits are the node number, then 2 bits for zone, then the rest are actually flags
- In 64, it's 10 bits for node number, 2 for zone, then the rest for flags

### `mem_map`

- An array of pages 
```c
Sparsemem abstracts the use of discontiguous mem_maps[].  This kind
of mem_map[] is needed by discontiguous memory machines (like in the
old CONFIG_DISCONTIGMEM case) as well as memory hotplug systems.
Sparsemem replaces DISCONTIGMEM when enabled, and it is hoped that
it can eventually become a complete replacement.

A significant advantage over DISCONTIGMEM is that it's completely
separated from CONFIG_NUMA.  When producing this patch, it became
apparent in that NUMA and DISCONTIG are often confused.

Another advantage is that sparse doesn't require each NUMA node's
ranges to be contiguous.  It can handle overlapping ranges between
nodes with no problems, where DISCONTIGMEM currently throws away
that memory.

Sparsemem uses an array to provide different pfn_to_page()
translations for each SECTION_SIZE area of physical memory.  This
is what allows the mem_map[] to be chopped up.

In order to do quick pfn_to_page() operations, the section number
of the page is encoded in page->flags.  Part of the sparsemem
infrastructure enables sharing of these bits more dynamically (at
compile-time) between the page_zone() and sparsemem operations.
However, on 32-bit architectures, the number of bits is quite
limited, and may require growing the size of the page->flags
type in certain conditions.  Several things might force this to
occur: a decrease in the SECTION_SIZE (if you want to hotplug smaller
areas of memory), an increase in the physical address space, or an
increase in the number of used page->flags.

One thing to note is that, once sparsemem is present, the NUMA node
information no longer needs to be stored in the page->flags.  It
might provide speed increases on certain platforms and will be
stored there if there is room.  But, if out of room, an alternate
(theoretically slower) mechanism is used.

This patch introduces CONFIG_FLATMEM.  It is used in almost all
cases where there used to be an #ifndef DISCONTIG, because
SPARSEMEM and DISCONTIGMEM often have to compile out the same areas
of code.
```


