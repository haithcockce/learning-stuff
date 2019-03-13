# Page Tables

#### History

- Back in the day, systems executed a single thing. 
- Multics and Bell Lab's Time Sharing system allowed for multiple things to be executed.
- Time sharing and simultaneous multiprocessing created issues with memory 
  - This required loading things into areas of memory nothing else took. 
  - Inherently, not all the memory an application requests will be used, so this means wasted memory. 
  - At the same time, applications should not have memory limited in case they do need that memory but others do not
  - In more general terms, an application should not have to care other applications run along side it

#### Overview 

- In general, page tables map physical addresses to virtual addresses where the virutal addresses are used by applications
- A basic page table has a tables of addresses which point to a frame table listing mapped frames of memory 
  - This can be wasteful however. The above requires maintaining an entry for each frame. 
  - When not all of memory is used, this can be quite wasteful. Keep in mind, we need more than just the mapping, but also metadata (dirty? Paged out? unmapped?) This is quite wasteful. 
  - Furthermore, instead of walking the whole table linearly, for performance, you need a hash function to index into the table. But then you likely will have terrible spatial locality. 
- Multilevel page tables are used now. Each level points to additional tables and entries are filled in ony when necessary. 

- `handle_mm_fault()` in `mm/memory.c` handles translation between the PGD/PUD/PMD/PTE 
- You can use `vtop` in crash 
- [This kcs](https://access.redhat.com/solutions/968793) shows how to manually do it
