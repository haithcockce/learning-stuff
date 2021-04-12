# Red-Black Trees

## Overview

- Similar to an AVL tree, a binary search tree wherein the tree maintains balance. In particular,

  1. The difference in order between sibling subtrees is at most 1
  2. A leaf node does not have explicit children, and thus has children name "NIL" which are always painted black
  2. A path from root to any leaf node is always the same count of "black" nodes, including NIL nodes
  3. This balance is maintained via rotations of trees and subtrees

- Nodes describe some element, a parent pointer, and up to two child pointers.
- Full conceptual details fall outside the scope of this training. Refer to the [Red-black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree) wikipedia page for more info

### Linux-Specific Overview

- Similar to the [`list_head` ](https://github.com/haithcockce/learning-stuff/blob/master/docs/training/data_structures/list.md), the data structures organized by an rbtree have the rbtree node information embedded into it

  - Because the rbtree organization bits are embedded in the structures they organize, users of the Linux implementation of rbtrees are expected to have their own traversal and locking implementations.
  - This also means elements are accessed via the `container_of()` macro when traversing an rbtree.
  - Also similar to the `list_head` functionality using `list_entry*` functions to access the elements of a list rather than the `list_head` structure directly, `rb_entry()` exists to access the containing structure.

- The Linux kernel implementation still paints nodes as black where needed via a neat trick using properties of kernelspace addresses

## Implementation

- rbtree node

  - As noted above, structures organized into an rbtree have the node info embedded in it.

    ```c
    struct rb_node {
            unsigned long  __rb_parent_color;   // parent ptr & color
            struct rb_node *rb_right;
            struct rb_node *rb_left;
    } ;
    ```

- rbtree root

  - The root is just another `rb_node`. and a macro is provided to instantiate a blank `rb_root`

    ```c
    struct rb_root {
            struct rb_node *rb_node;
    };
    [...]
    #define RB_ROOT (struct rb_root) { NULL, }
    ```

- First node vs. root node

  - A property of BST require the nodes to be sorted in order of a depth first search, wherein the left-most child is the global minimum for the tree while the right-most child is the global maximum.
  - Because of this, many uses of a rbtree want the left-most child. The Linux kernel provides a method of storing this easily;

    ```c
    struct rb_root_cached {
            struct rb_root rb_root;
            struct rb_node *rb_leftmost;
    };
    ```

- Accessing parent and coloring

  - Kernelspace addresses for struct instances are typically aligned to some value but always at least an even value (evenly divisible by 2).
  - Because of the alignment, the lowest bit of these addresses effectively do not store anything and are used in designating color (black)
  - The kernel uses the lowest bit to indicate red (0x0) or black (0x1).

    ```c
    // rbtree_augmented.h
    #define RB_RED          0
    #define RB_BLACK        1

    #define __rb_parent(pc)    ((struct rb_node *)(pc & ~3))

    #define __rb_color(pc)     ((pc) & 1)
    #define __rb_is_black(pc)  __rb_color(pc)
    #define __rb_is_red(pc)    (!__rb_color(pc))
    #define rb_color(rb)       __rb_color((rb)->__rb_parent_color)
    #define rb_is_red(rb)      __rb_is_red((rb)->__rb_parent_color)
    #define rb_is_black(rb)    __rb_is_black((rb)->__rb_parent_color)

    [...]
    static inline void rb_set_black(struct rb_node *rb)
    {
            rb->__rb_parent_color |= RB_BLACK;
    }
    ```

  - As such, to walk an rbtree from a child node to the root node, take the rb_node pointer in question, grab the embedded parent pointer `__rb_parent_color`,  remove the `RB_BLACK` value (`__rb_parent_color & (~0x1)`), and dereference the value as an `rb_node` structure. Repeat this until `__rb_parent_color` is `NULL` or `0x1`.  

## VMCore Stuff

- The `tree` command in crash allows walking an rbtree (and other types of trees as well) starting from a given node. Below examples use the `vmap_area_root` pointer to the rbtree of virtual memory mapped areas on the system.

  - When provided only an address to an `rb_root` or `rb_node`, `tree` will simply traverse the tree from that node and print the pointers to the `rb_node` structures

    ```
    crash> tree vmap_area_root | head
    ffff9393fb0153d8
    ffff9393fa38e618
    ffff9393c7044558
    ffff9393c77cb678
    ffff9393c7d168b8
    ffff9393c7d16a98
    ffff9393c7d16618
    ffff9393c7d166d8
    ffff9393c7d162b8
    ffff9393c7d165b8
    ```

  - The virtual memory mapped areas on the system are organized into an rbtree wherein nodes are `vmap_area` structures. An `rb_node` is embedded into the `vmap_area` structure

    ```
    crash> struct vmap_area
    struct vmap_area {
        unsigned long va_start;
        unsigned long va_end;
        unsigned long flags;
        struct rb_node rb_node;    <---
        struct list_head list;
        struct llist_node purge_list;
        struct vm_struct *vm;
        struct callback_head callback_head;
    }
    ```

  - When the `tree` command is provided an offset to the embedded `rb_node`, `tree` can then print the pointers to the containing structures. In our example of the `vmap_area` tree, the pointers are to individual `vmap_area` structures

    ```
    crash> tree -o vmap_area.rb_node vmap_area_root | head
    ffff9393fb0153c0
    ffff9393fa38e600
    ffff9393c7044540
    ffff9393c77cb660
    ffff9393c7d168a0
    ffff9393c7d16a80
    ffff9393c7d16600
    ffff9393c7d166c0
    ffff9393c7d162a0
    ffff9393c7d165a0
    ```

  - The `tree` command also can print the locations of the nodes/structures in the tree (`-p` option)

    ```
    crash> tree -o vmap_area.rb_node vmap_area_root -p | head
    ffff9393fb0153c0
      position: root
    ffff9393fa38e600
      position: root/l
    ffff9393c7044540
      position: root/l/l
    ffff9393c77cb660
      position: root/l/l/l
    ffff9393c7d168a0
      position: root/l/l/l/l
    ```

  - Similar to the `list` command, `tree` can also print out various member values from the structures it walks. In the `vmap_area` output above, the below example prints out the `va_start` and `va_end` addresses from each `vmap_area`

    ```
    crash> tree -o vmap_area.rb_node vmap_area_root -s vmap_area.va_start,va_end -p | head -16
    ffff9393fb0153c0
      position: root
      va_start = 0xffffb0fe00da4000
      va_end = 0xffffb0fe00da9000
    ffff9393fa38e600
      position: root/l
      va_start = 0xffffb0fe00904000
      va_end = 0xffffb0fe00909000
    ffff9393c7044540
      position: root/l/l
      va_start = 0xffffb0fe006fc000
      va_end = 0xffffb0fe00701000
    ffff9393c77cb660
      position: root/l/l/l
      va_start = 0xffffb0fe0067c000
      va_end = 0xffffb0fe00681000
    ```

  - The `tree` command can also walk a tree using a child node as the relative root node (IE traverse the subtree rooted by the node in question)

    ```
    crash> tree vmap_area_root -p
    ffff9393fb0153d8
      position: root
    ffff9393fa38e618
      position: root/l
    ffff9393c7044558
      position: root/l/l
    ffff9393c77cb678
      position: root/l/l/l
    ffff9393c7d168b8
      position: root/l/l/l/l
    ffff9393c7d16a98
      position: root/l/l/l/l/l
    ffff9393c7d16618
      position: root/l/l/l/l/l/l
    ffff9393c7d166d8
      position: root/l/l/l/l/l/l/l
    ffff9393c7d162b8             <--- starting here
      position: root/l/l/l/l/l/l/r
    ffff9393c7d165b8
      position: root/l/l/l/l/l/l/r/l
    ffff9393fbb775b8
      position: root/l/l/l/l/l/l/r/r
    ffff9393c7d16798
      position: root/l/l/l/l/l/l/r/r/l
    ffff9393fbb9a978
      position: root/l/l/l/l/l/l/r/r/r
    ffff9393c7d16378
      position: root/l/l/l/l/l/r
    ffff9393c7d16858
      position: root/l/l/l/l/l/r/l
    ffff9393c7d16a38

    crash> tree -o vmap_area.rb_node -p -N ffff9393c7d162b8
    ffff9393c7d162a0
      position: root
    ffff9393c7d165a0
      position: root/l
    ffff9393fbb775a0
      position: root/r
    ffff9393c7d16780
      position: root/r/l
    ffff9393fbb9a960
      position: root/r/r
    ```

  - However, traversal, currently, only walks child nodes and their descendents and there is no way to walk the parent hierarchy with the command.

## References

- [Wikipedia: Red-black tree](wikipedia.org/wiki/Red–black_tree)
- Kernel source

  - [Red-black Trees (`rbtree`) in Linux](https://www.kernel.org/doc/Documentation/rbtree.txt)
  - `lib/rbtree.c`
  - `include/linux/rbtree_augmented.h`
  - `include/linux/rbtree.h`  
