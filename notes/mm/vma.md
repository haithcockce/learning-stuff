# VM

### Overview

- Each process has a __virtual memory address space__ which is all the memory the application believes it has
- The virtual memory address space is divided into __virtual memory areas__ (VMAs) where each area has a specific purpose (code, mapped page, stack space, etc)
- __TODO: Check this__ Each VMA has an address that points to a __page table entry__ in the page tables. 
- A process has a single entry in the __page global directory__ (PGD)
- To interact with a page, the __virtual address__ (VA) the process has is translated to a PTE which then points to a physical mapping. 
- The mapping is one way, so invalidating a physical page (due to swapping or laundering) requires scanning all PTEs so this is inefficient 
- Reverse mappings (__rmap__) is made to provide a list of PTEs pointing to a physical page when given the physical page 

### Iterations

- The initial iteration had each physical page containing a linked list of all PTEs it belongs to
- The second iteration used an indirect path for file-backed memory which looked at a page's `address_space` member and checked all VMAs which have the `address_space` referenced. 
  This did nothing for anonymous memory however. The pointer to the `address_space` stuff is called `mapping`
- The third iteration changes the `page struct` to include `vma` which points to a list of VMAs the page is referenced to. 
  The trade off here is instead of a direct PTE to remove, the PTEs for each VMA must be scanned. Balances initial algorithm (search all VMAs) and update (keep list of all PTE references)

