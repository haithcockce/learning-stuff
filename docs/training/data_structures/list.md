# Lists

## Overview

- The kernel uses circular, doubley-linked lists; IE

  - An entry in the list contains a pointer to the next item (`next`) and a pointer to the previous item (`prev`)
  - The `next` pointer at "end" of the list points to the beginning of the list, and thus is circular;

    ```
        list_head          list_head
        +------+------+    +------+------+
        | prev | next |    | prev | next |
    --->|      |   o------>|      |   o------> ...
    <------o   |      |<------o   |      |<---
        +------+------+    +------+------+
    ```
            
  - An empty list is represented by the `prev` and `next` pointer being equal. Below is a subset of a queue of `list_head` lists in the `rq` struct;

    ```
    rt = {
      active = {
        bitmap = {0x0, 0x1000000000}, 
        queue = {{
            next = 0xffff8df37ba2a010,   <--- this list is empty 
            prev = 0xffff8df37ba2a010    <--- as next == prev
          }, {
            next = 0xffff8df37ba2a020,   <--- this list is empty 
            prev = 0xffff8df37ba2a020    <--- as next == prev
    ```

- When a struct is expected to have multiple instances of itself managed in a central location (such as the lists of processes on a system), the struct definition will embed a `list_head` in itself typically to enable traversing over instances of itself. More on this in implementation. 
- Multiple iterating functions exist enabling traversal throughout the list and adding/removing from the list.
- Multiple functions also exist to allow interacting with the `list_head` stuff directly or the their containing structures. 

## Implementation Details 

#### Structs

- `list_head` 

  - Defined in `include/linux/types.h`

    ```c
    struct list_head {
            struct list_head *next, *prev;
    };
    ```

  - If a `list_head` contains only `next` and `prev`, how do we keep track of a list of structures? This is covered later on.

- Example list; `wait_queue_head` which manages a list of functions to be executed at a later time

  - Implementation of `wait_queue_head` and `wait_queue_entry`

    ```c
    include/linux/wait.h:
    struct wait_queue_entry {
            unsigned int            flags;
            void                    *private;
            wait_queue_func_t       func;
            struct list_head        entry;   // next/prev pointers will point 
    };                                       // to other wait_queue_entry structs

    struct wait_queue_head {
            spinlock_t              lock;
            struct list_head        head;  // entry point to the list
    };                                     // of wait_queue_entry
    ```

  - The `wait_queue_head` is a pointer to the list of `wait_queue_entry` structs 
  - Within each `wait_queue_entry` struct is a `list_head` struct allowing someone to traverse the list of `wait_queue_entry` structs via the `wait_queue_head` entry point

    ```
                     wait_queue_entry wait_queue_entry wait_queue_entry
                     +-----------+    +-----------+    +-----------+
    wait_queue_head  | flags ... |    | flags ... |    | flags ... |
    +-----------+    +-----------+    +-----------+    +-----------+
    | list lock |    | function  |    | function  |    | function  |
    +-----------+    +-----------+    +-----------+    +-----------+      ...
    | list_head |--->| list_head |--->| list_head |--->| list_head |--->
    |           |    |           |<---|           |<---|           |<---
    +-----------+    +-----------+    +-----------+    +-----------+
    ```

#### Functionality 

- Nearly all functionality is defined in `include/linux/list.h`.
- Interacting with the lists can be done either directly on the `list_head` struct or the structure with the `list_head` embedded in it. 

  - Below are some examples of interacting with the `list_head` structs themselves;

    - Main function to add an entry to a list:

      ```c 
      /*
       * Insert a new entry between two known consecutive entries.
       *
       * This is only for internal list manipulation where we know
       * the prev/next entries already!
       */
      static inline void __list_add(struct list_head *new,
                                    struct list_head *prev,
                                    struct list_head *next)
      {
              if (!__list_add_valid(new, prev, next))
                      return;

              next->prev = new;
              new->next = next;
              new->prev = prev;
              WRITE_ONCE(prev->next, new);
      }
      [...]
      /**
       * list_add - add a new entry
       * @new: new entry to be added
       * @head: list head to add it after
       *
       * Insert a new entry after the specified head.
       * This is good for implementing stacks.
       */
      static inline void list_add(struct list_head *new, struct list_head *head)
      {
              __list_add(new, head, head->next);
      }
      ```

    - Deleting an item from the list

      ```c 
      static inline void list_del(struct list_head *entry)
      {
              __list_del_entry(entry);
              entry->next = LIST_POISON1;  // Note; we mark deleted entries with 
              entry->prev = LIST_POISON2;  // special values to add debugging 
      }
      ```

  - Below are examples of interacting with the structs in the list via the struct's embedded `list_head`. Note, these typically refer to the structs as _entries_.

    - Grab the current entry in the list 

      ```c
      /**
       * list_entry - get the struct for this entry
       * @ptr:        the &struct list_head pointer.
       * @type:       the type of the struct this is embedded in.
       * @member:     the name of the list_head within the struct.
       */
      #define list_entry(ptr, type, member) \
              container_of(ptr, type, member)  // grabs the entry with the list_head we have
      ```

    - Grab the first entry in a list 

      ```c 
      /**
       * list_first_entry - get the first element from a list
       * @ptr:        the list head to take the element from.
       * @type:       the type of the struct this is embedded in.
       * @member:     the name of the list_head within the struct.
       *
       * Note, that list is expected to be not empty.
       */
      #define list_first_entry(ptr, type, member) \
              list_entry((ptr)->next, type, member)
      ```

- Many functions exist to traverse a `list_head` list in a variety of ways; 

  - Traversing based on the `list_head` struct itself:
  
    - Iterate through the whole list 

      ```c
      /**
       * list_for_each        -       iterate over a list
       * @pos:        the &struct list_head to use as a loop cursor.
       * @head:       the head for your list.
       */
      #define list_for_each(pos, head) \
              for (pos = (head)->next; pos != (head); pos = pos->next)
      ```

    - Iterate from somewhere in the list to the end 

      ```c
      /**
       * list_for_each_continue - continue iteration over a list
       * @pos:        the &struct list_head to use as a loop cursor.
       * @head:       the head for your list.
       *
       * Continue to iterate over a list, continuing after the current position.
       */
      #define list_for_each_continue(pos, head) \
              for (pos = pos->next; pos != (head); pos = pos->next)
      ```

    - Iterate over the whole list in reverse

      ```c
      /**
       * list_for_each_prev   -       iterate over a list backwards
       * @pos:        the &struct list_head to use as a loop cursor.
       * @head:       the head for your list.
       */
      #define list_for_each_prev(pos, head) \
              for (pos = (head)->prev; pos != (head); pos = pos->prev)
      ```

  - Iterate based on the structures that have the `list_head` embedded in them

    - Iterate over every entry in a list 

      ```c 
      /**
       * list_for_each_entry  -       iterate over list of given type
       * @pos:        the type * to use as a loop cursor.
       * @head:       the head for your list.
       * @member:     the name of the list_head within the struct.
       */
      #define list_for_each_entry(pos, head, member)                          \
              for (pos = list_first_entry(head, typeof(*pos), member);        \
                   !list_entry_is_head(pos, head, member);                    \
                   pos = list_next_entry(pos, member))
      ```

- `container_of(ptr, type, member)`

  - Allows grabbing the pointer to the entry (container) our `list_head` is embedded in

    ```
    +--------+
    | type   | ---> returns pointer to the container
    +--------+      holding our ptr*
    | ...    |
    | member | <--- ptr*
    | ...    |
    +--------+
    ```

  - `container_of` allows retrieving the structure in the list managed by that structure's `list_head`. This was used in the `list_entry()` function earlier

## Vmcore Stuff 

- List traversal can be done via a variety of ways, but the easist is the `list` command. 
- Below is listing the list of tasks starting with systemd;

  ```plaintext
  crash> ps | grep systemd
        1      0   1  ffff8df347792f80  IN   0.2   94944  10708  systemd

  crash> struct -o task_struct.tasks ffff8df347792f80
  struct task_struct {
    [ffff8df347793780] struct list_head tasks;
  }

  crash> list ffff8df347793780 | head -5
  ffff8df347793780
  ffff8df347790800
  ffff8df347794f40
  ffff8df347796700
  ffff8df347791fc0
  ```

- The `list` allows to grab info from each listed structure as it is listed; 

  ```plaintext 
  crash> list ffff8df347793780 -l task_struct.tasks -s task_struct.comm,pid
  ffff8df347793780
    comm = "systemd\000\060\000\000\000\000\000\000"
    pid = 0x1
  ffff8df347790800
    comm = "kthreadd\000\000\000\000\000\000\000"
    pid = 0x2
  ffff8df347794f40
    comm = "rcu_gp\000d\000\000\000\000\000\000\000"
    pid = 0x3
  ffff8df347796700
    comm = "rcu_par_gp\000\000\000\000\000"
    pid = 0x4
  ffff8df347791fc0
    comm = "kworker/0:0\000\000\000\000"
    pid = 0x5
  ffff8df3477c6700
    comm = "kworker/0:0H\000\000\000"
    pid = 0x6
  ffff8df3477c1fc0
    comm = "kworker/0:1\000\000\000\000"
    pid = 0x7
  ffff8df3477c3780
    comm = "kworker/u4:0\000\000\000"
    pid = 0x8
  ```
