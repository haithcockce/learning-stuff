# Intermediate Vmcore Analysis

### There is a lot. Like a lot a lot.

Example retrace vmcore: 237449516

#### Understanding Datasets

###### Understanding Kernel Memory

* In virtual memory, code (instructions) starts at `0xffffffff81000000` for x86_64
* Data is stored `0xffff880000000000` for x86_64
* Code is typically static but you can make changes to running code via systemtap, perf, and modules
  * Examples: antivirus may overwrite syscalls to point to them in order to perform security things.

###### Memory allocations (SLAB/SLUB)

* Basic allocations for memory are via memory allocators, SLAB, SLUB, and SLOB allocator
  * SLAB in RHEL 6 and below
  * SLUB in RHEL 7 and above
  * This is dynamic memory usage, similar to malloc via applications, but done via `kmem_alloc()`
* `kmem -s`
  * displays kernel memory allocations
  * Allocations placed into buckets of similar stuff, like `rpc_inode_cache`. However, you also have
    generic cache. Those are simply allocations made of things of the same size.
  * In troubleshooting, you typically have a bad slab cache entry, and then you work your way up:
    * object -> slab page -> slab -> slab cache
* Stacks
  * The working space required to accomplish a task.
  * Interact with it via `push` and `pop`.
  * Every thread has a userspace and kernelspace stack.
  * Interrupts also have stacks and page faults. Viewable via `help -m` as well as what address the
    stacks start at.

##### Data Structures and blah

* Structures
  * Organization of data in memory. Like a house. An area for a bathroom, area for a laundry, etc
    so effectively like blueprints
  * When given an address, can map the blueprint to a block of memory.
* Global structures and Symbols
  * Symbols are predefined structures located in the code section
    * __reminder__ this is at `0xffffffff81000000`

##### System Calls and Stacks

* A kernel allows for a generic-ish interface for a userspace thing to interact with hardware
* A syscall is the kernel API effectively
* When doing a syscall, we encounter a context switch
  * A context switch, you dump the registers to memory and work to do the new thing you switch to.
* When doing something, we grow the stack whenever we are calling a function, and shrink it when we
  are done with the function.

##### Interrupts

* Devices interact with an interrupt controller (APIC) and a CPU to handle some work
* The device talks to the APIC to raise an interrupt to perform the task
* Interrupts can be masked to ignore until later. The APIC queues them until later
* The line an interrupt comes on is an interrupt request vector
* You can see the registered IRQ vectors via `crash> irq`. It shows the irq descriptor and irq
  action which has the function to call when we get the irq
* SoftIRQ
  * Software defined IRQ. Used for deferred work we build up otherwise like IO and network activity

##### Kernel Threads

* Defined with brackets: `[kworker/1]`
* Typically deferred work

##### Kernel Constructs

* Tasks
  * organizing threads. Contains a lot of data and meta data about the process/thread such as
    pointers to children, pid, last run time
* Timers
  * Actions to perform later on.
  * Fired via APIC

##### Stack and Registers

RHEL 7

```c
struct pt_regs {
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long bp;
    unsigned long bx;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long ax;
    unsigned long cx;
    unsigned long dx;
    unsigned long si;
    unsigned long di;
    unsigned long orig_ax;
    unsigned long ip;
    unsigned long cs;
    unsigned long flags;
    unsigned long sp;
    unsigned long ss;
}
```

```
crash> bt -r 1| tail -15
ffff96a97dfaff10:  000000000000003c 00000000c7a77ba9 
ffff96a97dfaff20:  0000000000000000 0000000000000000 
ffff96a97dfaff30:  0000000000000000 0000000000000000 
ffff96a97dfaff40:  0000000000000000 0000000000000000 
ffff96a97dfaff50:  system_call_fastpath+28 0000559a2daf01e0 
                                                r15
ffff96a97dfaff60:  0000000000000e28 ffffffffffffffff 
                         r14              r13
ffff96a97dfaff70:  0000000000000001 00007ffdc1b57830 
                         r12              rbp
ffff96a97dfaff80:  00007ffdc1b57500 0000000000000293 
                         rbx              r11
ffff96a97dfaff90:  00000000ffffffff 0000000000000000 
                         r10              r9
ffff96a97dfaffa0:  00000000000962e2 00000000000000e8 
                         r8               rax
ffff96a97dfaffb0:  ffffffffffffffff 000000000000002f 
                         rcx              rdx
ffff96a97dfaffc0:  00007ffdc1b57500 0000000000000004 
                         rsi              rdi
ffff96a97dfaffd0:  00000000000000e8 00007f03c1a01113 
                      orig_rax            rip
ffff96a97dfaffe0:  0000000000000033 0000000000000216 
                         cs              rflags
ffff96a97dfafff0:  00007ffdc1b551c8 000000000000002b 
                         rsp              ss
```
