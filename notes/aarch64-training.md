# AArch64 (ARM64v8) ISA Differences to x86_64

### Overview

- Called __Kernel-Alt__
- RHEL 7 product with RHEL 7 userspace but running a 4.11 kernel
- Different products
  - __ARMv8-A__ A64 arch with the virtual memory system architecture
  - Others are 32-bit so we do not support them

##### Architecture vs Linux Implementation

- Most code in Linux is "generic" C, but some areas are specifically raw assembly.  Used to flush
  TLS, enter kernel via syscalls and handling interrupts/exceptions, atomic operations, etc
- Linux uses segmentation due to x86 but at a bare minimum.
- A ton of acronyms and initializations exist

##### ARMv8-A Overview

- (similar) means ARM64v8 is similar to x86_64 in a particular manner.
- ARM uses fixed-width instructions whereas Intel has variable byte-size instructions
- Limits 48-bit address range (similar)
- 31 general purpose, 64-bit registers called `X0-X31`
  - Similar registers include a PC, stack pointer, etc
  - `X0-X7` arguments and results
  - `X29` frame register
  - `X30` link register
  - when referenced as `W0-W30`, 32-bit (lower or upper?)
- Special purpose registers
  - Execution levels 0-3
- ARM64v8 uses 16bits as a word, 32 as double word, 64 as quad
- Single Program Status Register (SPSR) (similar: EFLAGS)

## Execption, Privlege Levels, and Thread/Handler Mode

- EL0/PL0, userspace
- EL1/PL1, privledged/OS
- __S__uper__V__iser__C__all (SVC) == syscall, and the syscall number is in `X8`
- Tripping into kernelspace, you branch and manually pushes onto stacks
- EL2/PL2, Hypervisor  calls
- EL3/PL1, firmware and allows for switching between secutiry states (?)

##### Exceptions/Faults

- Intel has an interrupt table (idt_table, viewable in crash with `irq -d`) to handle interrupts by tripping into predefined fault code paths defined in `arch/x86/kernel/entry.S`. In ARMv8-A, we index into a vector with an index in the `ESR` register in the upper 4 bits (actually part of the address as an offset) into the `vectors` table that contains the branch instructions into the handlers (this is viewable with just `irq` and you will see a list of interupst with some listed as `(unused)`)
- When tripping to kernelspace, (similar) ARMv8-A saves nearly no reigsters and are typically saved by software (kernel)
- Many exception numbers are not used/do not exist and the vector has a large number of invalid function pointers (literally `el1_sync_invalid` for example)
- Link Register (`LR` in crash) tells you where you came from.

##### Asynchronous/Synchronous Exceptions

- Synchronous Exceptions return address is guaranteed to be the same as where you entered whereas async is not guaranteed

##### Entering the Kernel

- Store instructions are read left to right where as read is right to left
- Multiple instructions have multiple operations/operands,
- stack stores registers like so:
```
x0    x1
x2    x3
...
x28   X29
x30
```
Compared to x86_64:
```
r12   r13
etc
```
And the stack is manually manipulated rather than automagical things like `callq` automaically pushing `rip`. As such, you will likely see more registers being stored via callee saving.
- `ret` uses the link register to get back to where we were
- `X8` is used to call the syscall number when doing a SVC
- The syscall table can be reviewed via `rd sys_call_table ##`

##### Page Faults

Enter into `el0_sync` to do a mem abort (page fault in ARMv8-A) or data abort (?)

## Atomicity

- Instructions that need to read/write/rw memory in one instruction
- Intel implements `lock` instruction prepended to instructions that must be done atomically however, ARMv8-A uses `<instruction>xr` (exclusive register) that says to lock the area of memory for some time. This is similar to x86_64 in so much that a single instruction does nearly the same thing in x86_64 (though the single instruction takes several cycles to do the thing) whereas ARMv8-A simply breaks up the instruction into smaller instructions explicitly

## Crash Differences

- We still have 16 KiB stacks. However, the paging is 64KiB! We use the same page to store 4 total stacks
  - Numa aware?
- `bp_cpuinfo` == x86_64 `per_cpu`, however, there is a `__per_cpu_start` and `__per_cpu_end` to delimit
- Registers are laid down in the same order in both as described by `pr_regs` via `kernel_entry` function
- Stacks will have a `STACK_END_MAGIC` at the very "top" (as seen by `bt -f`) whose value is `0000000057ac6e9d` (similar, but value is different) and is used to detect stack overflow
- tasks do not have `thread_info` at the top of the stack and `thread_info` is extremely small compared to x86_64 and resides in `task_struct` it belongs to
- `bl` branch link with hard coded address vs. `blr` branching via the link register
- syscall numbers are different (`sys_write()` used to be `0x1` but is now `0x8` in ARMv8-A)
- Still little endian
  - bytes are stored in order high order to low order left to right
  - Strings are not read in english like this! Byte 0 is letter 0!
  - Likewise, instructions are interpreted backwards
- lower order two bits being off in PGD/PMD/etc means it is swapped out
- ASLR is enabled by default

##### Per PCU Table

- Still seen via `kmem -o`
- RHEL 6 and below indicated per-cpu variables via `per_cpu_*` or something similar, RHEL 7 and up has regular variable for each CPU.
- You can see each per-cpu variable via the following:
```bash
sym ­l | awk '/__per_cpu_start/{flag=1;next}/__per_cpu_end/{flag=0}flag'
```

## Extra Notes

- `vm` shows vmas for a process. The flags indicate read/write/execute, with the end number being 1 indicating read only, 3 being rw, and 5 being rwx (I think)
- `vm -p` will show the translation between va to pa as well as the start and end of the file and what the file is.
- Page size in 64 KiB in ARMv8-A (4 KiB in x86_64). Also references to pages are via a page structure (`page`) and is sized at 64 B (0x40 B), so memory to hold page structs can consume 1.4% of memory. For large memory, this is a large waste. Fewer syscalls to invoke a command though!
- How do we know where to return? `gs` register! (general segment register?) Every cpu has a `gs` reigster and `msr`  where we swap with to store where we were executing and what we execute on behalf on
- Operations ending with `!` means to do the calculation next to it first then do the operation
- When swapping, pages swapped are pages without any backing store so pages that have a backing store (shared library object or file on disk). Pages with a backing store do not get swapped out and are just flushed to disk.
- spinlock ticket number:
  - Matching: no waiting and holding
  - 0x1 off (0x2 in RHEL 7), one holder
  - More, then 1 holder and difference - 0x1/0x2 waiters
- __Memory barrier__
  - Finish all stores/reads to/from memory before moving on
  - C Compilers will attempt to interleave instructions sense an instructions required count of cycles may not be the same amount as the next one. That way, while you wait for one operation to complete (like a read from memory), you can do another non-dependent operation (like increment a different register) which is why instructions may look out of order. For example:
  ```
  REAL                    C-Code equivalent
  mov (%rdi),%rdi         mov (%rdi),%rdi
  mov %rcx,(%rdx)         add 0x1,%rdi
  add 0x1,%rdi            mov %rdi,(%rbx)
  mov %rdi,(%rbx)         mov %rcx,(%rdx)
  ```
- `help -m` gives tons of machine-specific info like an interrupt stack vector/table/whatever
- `dev -d` physical mapping of memory printed. Similar to `/proc/iomem` but more detailed
- crashkernel and regular kernel is literally the same (maybe with a few drivers omitted)
- __Paging__
  - `mm_struct.pgd` memory management structure's page global directory (not referred to as `pgd` in ARMv8-A as it is intel terminology)
  - In intel, we take the 48 bits of addressability and look at the lower order bits in an address, below is ARMv8-A
    - `PGD` upper 6 bits of this 48-bit mask
    - `PMD` next 13 lowest bits
    - `PTE` next 13 lowest bits
    - Next 16 bits (last 16 bits) is offset into page
