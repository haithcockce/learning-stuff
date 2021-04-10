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

## VMCore Stuff

## References

- [Wikipedia: Red-black tree](wikipedia.org/wiki/Red–black_tree)
- Kernel source

  - [Red-black Trees (`rbtree`) in Linux](https://www.kernel.org/doc/Documentation/rbtree.txt)
  - `lib/rbtree.c`
  - `include/linux/rbtree_augmented.h`
  - `include/linux/rbtree.h`  
