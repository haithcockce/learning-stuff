# Lists

- The kernel uses circular, doubley-linked lists 
- Multiple iterating functions and structures exist. 
- The most common is the `list_head` structure made of `prev` and `next` pointers only
- Traversal of lists with this method means the `list_head` structure is 
  embedded into a list item. When traversing the kernel performs a 
  `container_of` against the `list_head` to get the item

#### Structs

```
list_head          list_head
+------+------+    +------+------+
| prev | next |    | prev | next |
|      |   o------>|      |   o------> ...
----o  |      |<------o   |      |<---
+------+------+    +------+------+
```

- Example list

```
                 wait_queue_t
                 +-----------+    +-----------+    +-----------+
wait_queue_head_t| flags/??? |    | flags/??? |    | flags/??? |
+-----------+    +-----------+    +-----------+    +-----------+
| list lock |    | function  |    | function  |    | function  |
+-----------+    +-----------+    +-----------+    +-----------+      ...
| list_head |--->| list_head |--->| list_head |--->| list_head |--->
|           |<---|           |<---|           |<---|           |<---
+-----------+    +-----------+    +-----------+    +-----------+
```
