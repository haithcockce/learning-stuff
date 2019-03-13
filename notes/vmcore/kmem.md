# What the fuck does the output of kmem mean? 

A primer <3

This will largely be from examples to help it make sense.

#### Overview

- When reading the contents of memory, `rd <ADDR>` is used and crash interprets
  it as a kernel virtual address. From there, it will translate the VA to a PA
  and dump the contents of the 4KiB page therein. 
- Each page has a corresponding page struct accosiated with it (I believe 
  `page_t` but not sure). 
- Many areas of MM use the page struct to reference pages to move them around
  (for example between lrus and what not). 
  - In particular, each zone has a `free_area` associated with it which has a
    list of `free_list` structs which maintain, I believe, a list of page 
    structs of free pages. 


#### Example

```
crash> kmem ffff880b2d700000
ZONE  NAME        SIZE    FREE      MEM_MAP       START_PADDR  START_MAPNR
  2   Normal    12058624  1870387  ffffea0004000000   100000000     1048575  
AREA    SIZE  FREE_AREA_STRUCT
  3      32k  ffff880c7ffd11c8  
ffffea002cb5c000  (ffff880b2d700000 is 1st of 8 pages)

      PAGE        PHYSICAL      MAPPING       INDEX CNT FLAGS
ffffea002cb5c000 b2d700000                0        0  0 2fffff00000000
```

Output: 
- `ffff880b2d700000` was a VA for a page I was interested in. It was part of a
  `task_struct` but was unreadible because it was not in the vmcore for some
  reason. 
- __ZONE__ is the numa node zone it belongs to (like DMA, DMA32, NORMAL, etc)
  - The output indicates the page is part of ZONE 2. `kmem -z` will show you 
    which numa node that belongs to (hint it's 0)
- __NAME__ the zone name
- __AREA__ The `free_area` struct the page resides in. See Overview for
  `free_area_` info
- __FREE_AREA_STRUCT__ I _believe_ is the `free_list` the page is in. 
- `ffffea002cb5c000` is the page struct pointer. You can cast it to a page to
  see details about that page.
