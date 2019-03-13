# Looking into NMIs and reason codes

```
[I:1]  This is the core that was examined:

  crash> sys
        KERNEL: /cores/retrace/repos/kernel/x86_64/usr/lib/debug/lib/modules/2.6.32-358.14.1.el6.x86_64/vmlinux
      DUMPFILE: /cores/retrace/tasks/964603248/crash/vmcore  [PARTIAL DUMP]
          CPUS: 32
          DATE: Sun Jun  4 07:57:27 2017
        UPTIME: 00:01:48
  LOAD AVERAGE: 1.39, 0.52, 0.19
         TASKS: 1310
      NODENAME: apconl01.tmoviles.com.ar
       RELEASE: 2.6.32-358.14.1.el6.x86_64
       VERSION: #1 SMP Mon Jun 17 15:54:20 EDT 2013
       MACHINE: x86_64  (2599 Mhz)
        MEMORY: 96 GB
         PANIC: "Kernel panic - not syncing: An NMI occurred, please see the Integrated Management Log for details."


[I:2]  The panic backtrace:

  crash> bt
  PID: 0      TASK: ffffffff81a8d020  CPU: 0   COMMAND: "swapper"
   #0 [ffff880053607cd0] machine_kexec at ffffffff81035d6b
   #1 [ffff880053607d30] crash_kexec at ffffffff810c0d42
   #2 [ffff880053607e00] panic at ffffffff8150d66f
   #3 [ffff880053607e80] hpwdt_pretimeout at ffffffffa01074cd [hpwdt]
   #4 [ffff880053607ea0] notifier_call_chain at ffffffff81513875
   #5 [ffff880053607ee0] atomic_notifier_call_chain at ffffffff815138da
   #6 [ffff880053607ef0] notify_die at ffffffff8109cc3e
   #7 [ffff880053607f20] do_nmi at ffffffff815114b1
   #8 [ffff880053607f50] nmi at ffffffff81510e00
      [exception RIP: intel_idle+0xb1]
      RIP: ffffffff812d3a71  RSP: ffffffff81a01e58  RFLAGS: 00000046
      RAX: 0000000000000030  RBX: 0000000000000010  RCX: 0000000000000001
      RDX: 0000000000000000  RSI: ffffffff81a01fd8  RDI: ffffffff81a8fc40
      RBP: ffffffff81a01ec8   R8: 0000000000000005   R9: 000000000000006d
      R10: 0000001960e80200  R11: 0000000000000000  R12: 0000000000000030
      R13: 14c4e942606ce645  R14: 0000000000000004  R15: 0000000000000000
      ORIG_RAX: ffffffffffffffff  CS: 0010  SS: 0018
  --- <NMI exception stack> ---
   #9 [ffffffff81a01e58] intel_idle at ffffffff812d3a71
  #10 [ffffffff81a01ed0] cpuidle_idle_call at ffffffff814153a7
  #11 [ffffffff81a01ef0] cpu_idle at ffffffff81009fc6


[I:3]  We can see the NMI reason code passed to hpwdt_pretimeout preserved on the stack of panic():

First, looking at the function prototype and calling assembler:

  480 static int hpwdt_pretimeout(struct notifier_block *nb, unsigned long ulReason,
  481                                 void *data)
  482 {       

The reason code is the 2nd argument, it will be passed in via %rsi:

  crash> dis ffffffff81513875 | tail
  0xffffffff81513875 <notifier_call_chain+0x55>:	test   %r14,%r14
  crash> dis -lr ffffffff81513875 | tail
  0xffffffff81513864 <notifier_call_chain+0x44>:	je     0xffffffff81513883 <notifier_call_chain+0x63>
  /usr/src/debug/kernel-2.6.32-358.14.1.el6/linux-2.6.32-358.14.1.el6.x86_64/kernel/notifier.c: 84
  0xffffffff81513866 <notifier_call_chain+0x46>:	mov    0x8(%rcx),%r15
  /usr/src/debug/kernel-2.6.32-358.14.1.el6/linux-2.6.32-358.14.1.el6.x86_64/kernel/notifier.c: 93
  0xffffffff8151386a <notifier_call_chain+0x4a>:	mov    %r12,%rdx
  0xffffffff8151386d <notifier_call_chain+0x4d>:	mov    %rbx,%rsi      <<<<< hpwdt reason 
  0xffffffff81513870 <notifier_call_chain+0x50>:	mov    %rcx,%rdi

However %rbx is not modified in hpwdt_pretimeout:

  crash> dis -lr hpwdt_pretimeout+0x8d | grep rbx

We see that %rbx is finally preserved on the stack of panic():

  crash> dis panic
  0xffffffff8150d5c1 <panic>:     push   %rbp
  0xffffffff8150d5c2 <panic+0x1>: mov    %rsp,%rbp
  0xffffffff8150d5c5 <panic+0x4>: push   %rbx               <<<<<
  0xffffffff8150d5c6 <panic+0x5>: sub    $0x68,%rsp
  0xffffffff8150d5ca <panic+0x9>: callq  0xffffffff8100ad40 <mcount>
  0xffffffff8150d5cf <panic+0xe>: mov    %rsi,-0x38(%rbp)


[I:4]  Annotating the stack to find the value:

   #2 [ffff880053607e00] panic+0xae at ffffffff8150d66f
      /usr/src/debug/kernel-2.6.32-358.14.1.el6/linux-2.6.32-358.14.1.el6.x86_64/kernel/panic.c: 111
      ffff880053607e08: 0000000000000000 0000000000000000 
      ffff880053607e18: 0000000000000008 ffff880053607e88 
      ffff880053607e28: ffff880053607e38 kprobe_exceptions_notify+0x16 
      ffff880053607e38: 0000000000000000 ffffc900217fc072 
      ffff880053607e48: ffffc900217fc072 0000000000000000 
      ffff880053607e58: 0000000000000000 0000000000000002 
      ffff880053607e68: ffff880053607e78 0000000000000010  
                                            %rbx
      ffff880053607e78: ffff880053607e98 hpwdt_pretimeout+0x8d 
                          %rbp             %rip
   #3 [ffff880053607e80] hpwdt_pretimeout+0x8d at ffffffffa01074cd [hpwdt]


[I:5]  Matching up this value with the source code:

The value on the stack is 0x10 (16 decimal), looking at the source code:


  9 enum die_val { 
 10         DIE_OOPS = 1,
 11         DIE_INT3,
 12         DIE_DEBUG,
 13         DIE_PANIC,
 14         DIE_NMI,
 15         DIE_DIE,
 16         DIE_NMIWATCHDOG,
 17         DIE_KERNELDEBUG,
 18         DIE_TRAP,
 19         DIE_GPF,
 20         DIE_CALL,
 21         DIE_NMI_IPI,
 22         DIE_PAGE_FAULT,
 23         DIE_NMIUNKNOWN,
 24         DIE_NMIMEMPARITY,
 25         DIE_NMIIOCHECK,                                <<<<< 0x10 (16 decimal)
 26 }; 
```
