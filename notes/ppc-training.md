# PPC

### Overview

- Mneumonics to make it easier to read
- tons of onchip cache with 12 and 24 cores to a single chip
-

### Registers

- `r` prfix
- Do not call functions in kernel/userspace but rather branches (similar to arm but dissimilar to x86)
  - `r0` linkage
  - `r1` stack frame pointer
  - `r2` table of contents (?)
  - `r3-r10` parameters
  - `r11, r12` linkage
  - `r13` thread pointer used (called paca_struct which is processor specific data for each logical processor on the system)
  - paca_struct
  - `LR` link register
- `CR` Condition register
  - fileds 2-5 and 7 are "unused"
  - `CR0` result of fixed point instruction (add/sub)
  - `CR1` result of floating point
  - `CR6` relation is true for all element pairs
- `MSR` Machine state register
  - Used to detail interrupts sent
  - system reset, machine check, data storage, instruction storage, instruction segment, etc
  - Also has endianness flag

### Interrupt Classes

- NMI: System reset, machine check
- External (IO interrupts), Decrementer, etc

### Physical layout of memory

- Similar to memory, ppc uses 64KiB page size
- In x86, we use an interrupt to trip into table of pointers of stacks, ARM, uses the number to index into a vector. PPC has raw memory locations used for a table of code to handle the interrupts
- In x86 and arm, we enter via `entry.S` raw assembly. PPC uses `entry.S` and `exception-64s.S`
- 4 stacks per page and some pages will have residue of old stacks

### Assembly and Mnemonics

- `m{t,f}spr` move to/from special purpose register. Below moves `r13` to register `305` and then `304` to `r13`
- disass instructions are read right to left.

```
mtspr   305,r13
mfspr   r13,304
```
- `mfspr`
- `lwz  r10,8(r13)` loads left register with lower 32 bits of right register and zeros upper 32 bits in left register
- `mr   r31,r3` actually an or with `r3` and `r3` and stores the result in `r31`
- `lwsync` memory barrier
- `sc` system call that loads `r0` with the syscall number. And you need to multiple
- Will branch by default to `0x8` into a function since ftrace was not enabled in the compiled version.
- Likewise, do not branch to an adderss but rather always builds the address and branches to the address in a register
- Instructions may not be what they seem. For example, `mr` above is not just move but an or and store.

### Kernel code in RHEL 7

- `*T` target, `*A` source
- `R*` general purpose register
- `.` means condition register changed
- `+`/`-` applies to branch instructions only and set by `likely`/`unlikely` (`+` and `-` respectively)

### Raw spin lock

- Does _not_ use a ticketed fifo method
- When `0x0`, not locked
- load the `lock_token` into a register and check if we got it and it is not exclusive. If we did not get the lock exclusively, try again.

### Syscall

- PPC offers 32 and 64 bit syscalls,

### Extra Notes

- swap gs into kernel which swaps with an MSR segment register
- when in x86, the MSR in question hasa a pointer to per-cpu table. When tripping into kernel space, we tore the stack in the per-cpu table storage space and swapping with the kernel stack pointer
- For ppc, the `task_struct` is actually `paca_struct`
- x86 won't see an increase in page sie soon because the next page size up for x86 is 2 MiB
- Look up for 4KiB pages in x86: PGD -> PUD -> PMD -> PTE, hugepages (2MiB) PGD -> PUD -> PTE, 1 GiB: PGD -> PTE





QUESTION
- numa aware page stack things?
