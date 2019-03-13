```c
 751 /*
 752  * The pg_data_t structure is used in machines with CONFIG_DISCONTIGMEM
 753  * (mostly NUMA machines?) to denote a higher-level memory zone than the
 754  * zone denotes.
 755  *
 756  * On NUMA machines, each NUMA node would have a pg_data_t to describe
 757  * it's memory layout.
 758  *
 759  * Memory statistics and page replacement data structures are maintained on a
 760  * per-zone basis.
 761  */
 762 struct bootmem_data;
 763 typedef struct pglist_data {                                                                               
```
