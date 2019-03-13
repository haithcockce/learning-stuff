# Memory Blocks and the Buddy Allocator

### Overview

`mm/page_alloc.c`:

```
Freeing function for a buddy system allocator.

The concept of a buddy system is to maintain direct-mapped table
(containing bit values) for memory blocks of various "orders".                                         
The bottom level table contains the map for the smallest allocatable
units of memory (here, pages), and each level above it describes
pairs of units from the levels below, hence, "buddies".
At a high level, all that happens here is marking the table entry
at the bottom level available, and propagating the changes upward
as necessary, plus some accounting needed to play nicely with other
parts of the VM system.
At each level, we keep a list of pages, which are heads of continuous
free pages of length of (1 << order) and marked with `\_mapcount` -2. Page's
order is recorded in `page\_private(page)` field.
So when we are allocating or freeing one, we can derive the state of the
other.  That is, if we allocate a small block, and both were
free, the remainder of the region must be split into blocks.
If a block is freed, and its buddy is also free, then this
triggers coalescing into a block of larger size.

-- nyc
```

### High order and compound pages

```c
/*
 * Higher-order pages are called "compound pages".  They are structured thusly:
 *
 * The first PAGE_SIZE page is called the "head page".
 *
 * The remaining PAGE_SIZE pages are called "tail pages".
 *
 * All pages have PG_compound set.  All tail pages have their ->first_page
 * pointing at the head page.
 *
 * The first tail page's ->lru.next holds the address of the compound page's
 * put_page() function.  Its ->lru.prev holds the order of allocation.
 * This usage means that zero-order pages may not be compound.
 */
```

### When is a page a buddy?

```c
/*
 * This function checks whether a page is free && is the buddy
 * we can do coalesce a page and its buddy if
 * (a) the buddy is not in a hole &&
 * (b) the buddy is in the buddy system &&
 * (c) a page and its buddy have the same order &&
 * (d) a page and its buddy are in the same zone.
 *
 * For recording whether a page is in the buddy system, we set ->_mapcount -2.
 * Setting, clearing, and testing _mapcount -2 is serialized by zone->lock.
 *
 * For recording page's order, we use page_private(page).
 */
```
