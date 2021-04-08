# Lists

## Overview

- The kernel uses circular, doubley-linked lists; IE
  - An entry in the list contains a pointer to the next item (`next`) and a pointer to the previous item (`prev`)
  - The `next` pointer at "end" of the list points to the beginning of the list, and thus is circular;

            list_head          list_head
            +------+------+    +------+------+
            | prev | next |    | prev | next |
        --->|      |   o------>|      |   o------> ...
        <------o   |      |<------o   |      |<---
            +------+------+    +------+------+
            
  - An empty list is represented by the `prev` and `next` pointer being equal. Below is a subset of a queue of `list_head` lists in the `rq` struct;

        rt = {
          active = {
            bitmap = {0x0, 0x1000000000}, 
            queue = {{
                next = 0xffff8df37ba2a010,   <--- this list is empty 
                prev = 0xffff8df37ba2a010    <--- as next == prev
              }, {
                next = 0xffff8df37ba2a020,   <--- this list is empty 
                prev = 0xffff8df37ba2a020    <--- as next == prev

- When a struct is expected to have multiple instances of itself managed in a central location (such as the lists of processes on a system), the struct definition will embed a `list_head` in itself typically to enable traversing over instances of itself. More on this in implementation. 
- Multiple iterating functions exist enabling traversal throughout the list and adding/removing from the list.

## Implementation Details 

#### Structs

- `list_head` 

  - Defined in `include/linux/types.h`

        struct list_head {
                struct list_head *next, *prev;
        };

- Example list; `wait_queue_head` which manages a list of functions to be executed at a later time
  - The `wait_queue_head` is a pointer to the list of `wait_queue_entry` structs 
  - Within each `wait_queue_entry` struct is a `list_head` struct allowing someone to traverse the list of `wait_queue_entry` structs via the `wait_queue_head` entry point

                         wait_queue_entry wait_queue_entry wait_queue_entry
                         +-----------+    +-----------+    +-----------+
        wait_queue_head  | flags ... |    | flags ... |    | flags ... |
        +-----------+    +-----------+    +-----------+    +-----------+
        | list lock |    | function  |    | function  |    | function  |
        +-----------+    +-----------+    +-----------+    +-----------+      ...
        | list_head |--->| list_head |--->| list_head |--->| list_head |--->
        |           |    |           |<---|           |<---|           |<---
        +-----------+    +-----------+    +-----------+    +-----------+

