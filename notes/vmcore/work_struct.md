The overall situation is the system hung and a vmcore was generated. It turns out the blocked tasks were many in number with cgrulesengd being the oldest D state task. It was in the process of moving sshd into a new cgroup. This was analyzed earlier.

The parts in code blocks below are pulled from an analysis done by Vern Lovejoy unless otherwise noted.

Now to investigate the work_struct stuff.

1. Blocked `cgrulesengd` process:

    ```
    crash> bt
    PID: 2391   TASK: ffff881027715520  CPU: 3   COMMAND: "cgrulesengd"
     #0 [ffff88102545b828] schedule at ffffffff81548980
     #1 [ffff88102545b900] schedule_timeout at ffffffff815498f5
     #2 [ffff88102545b9b0] wait_for_common at ffffffff81549553
     #3 [ffff88102545ba40] wait_for_completion at ffffffff8154968d
     #4 [ffff88102545ba50] flush_work at ffffffff810a0097
     #5 [ffff88102545bac0] lru_add_drain_all at ffffffff81145d33
     #6 [ffff88102545bd10] mem_cgroup_move_task at ffffffff8118a087
     #7 [ffff88102545bd90] cgroup_attach_task at ffffffff810dabcf
     #8 [ffff88102545bde0] attach_task_by_pid at ffffffff810dad4b
     #9 [ffff88102545be30] cgroup_tasks_write at ffffffff810dae23
    #10 [ffff88102545be40] cgroup_file_write at ffffffff810d6b1a
    #11 [ffff88102545bef0] vfs_write at ffffffff81199c68
    #12 [ffff88102545bf30] sys_write at ffffffff8119a7a1
    #13 [ffff88102545bf80] system_call_fastpath at ffffffff8100b0d2
        RIP: 00000034314db6d0  RSP: 00007ffe75c3cff0  RFLAGS: 00010206
        RAX: 0000000000000001  RBX: ffffffff8100b0d2  RCX: 00007f5e02b33005
        RDX: 0000000000000005  RSI: 00007f5e02b33000  RDI: 0000000000000002
        RBP: 00007f5e02b33000   R8: 00000000ffffffff   R9: 0000000000000000
        R10: 00000000ffffffff  R11: 0000000000000246  R12: 0000000000000005
        R13: 000000000068a4b0  R14: 0000000000000005  R15: 00007ffe75c3d5e3
        ORIG_RAX: 0000000000000001  CS: 0033  SS: 002b
    ```

2. `wait_for_completion()` is the start of our wait. Let's look at `flush_work()` to see what we are waiting on. This means investigating the `work_struct` passed in to `flush_work()`:

    ```c
    int flush_work(struct work_struct *work)
    {
        struct cpu_workqueue_struct *cwq;
        struct list_head *prev;
        struct wq_barrier barr;

        might_sleep();
        cwq = get_wq_data(work);
    ```

3. Skipping the full details of the stack unwind, we determine `0xffff880061c53748` is the `work_struct` pointer:

    ```
      #2 [ffff88102545b9b0] wait_for_common at ffffffff81549553
        ffff88102545b9b8: ffff881027715520 0000000000000000
        ffff88102545b9c8: 0000000000013640 0000000000000002

     wait_queue_t 0xffff88102545b9d8

        ffff88102545b9d8: 0000000000000001 ffff881027715520
        ffff88102545b9e8: ffffffff8106c500 ffff88102545ba88
        ffff88102545b9f8: ffff88102545ba88 ffff880061c59d00
                                           rbx
        ffff88102545ba08: ffff880061c59d08 ffff880061c53748
                          r12              r13=struct work_struct *work
        ffff88102545ba18: ffff880061c59d00 ffff880061c53740
                          r14              r15
        ffff88102545ba28: 0000000000013640 0000000000013740
                          rbp
        ffff88102545ba38: ffff88102545ba48 ffffffff8154968d
     #3 [ffff88102545ba40] wait_for_completion at ffffffff8154968d
    ```

4. Casting to a `work_struct`:

    ```
    crash> work_struct ffff880061c53740
    struct work_struct {
      data = {
        counter = 0xffff880061c59d01
      },
      entry = {
        next = 0xffff88102545ba60,
        prev = 0xffff880061c53968
      },
      func = 0xffffffff81145d90 <lru_add_drain_per_cpu>
    }
    ```

5. From here, we need to see what process is handling the `work_struct` and why they are not processing the `work_struct`. This is not intuitive, though, as we need to get the `cpu_workqueue_struct` from the `work_struct` which has the `thread` working the queue. 

What is this step? 

    ```
    crash> p (0xffff880061c59d01&0xfffffffffffffffc)
    $2 = 0xffff880061c59d00

    crash> cpu_workqueue_struct 0xffff880061c59d00
    struct cpu_workqueue_struct {
      lock = {
        raw_lock = {
          slock = 0x0
        }
      },
      worklist = {
    - - - - - - - <snip> - - - - - - -
    ```

Well, let's look back at the code path we are in from 2. but more thoroughly:

    ```c
    /**
     * flush_work - block until a work_struct's callback has terminated
     * @work: the work which is to be flushed
     *
     * Returns false if @work has already terminated.
     *
     * It is expected that, prior to calling flush_work(), the caller has
     * arranged for the work to not be requeued, otherwise it doesn't make
     * sense to use this function.
     */
    int flush_work(struct work_struct *work)
    {
        struct cpu_workqueue_struct *cwq;
        struct list_head *prev;
        struct wq_barrier barr;

        might_sleep();
        cwq = get_wq_data(work);  // ---.
                                  //    |
    static inline                 //    v
    struct cpu_workqueue_struct *get_wq_data(struct work_struct *work)                                                                                                                                                                           
    {
        return (void *) (atomic_long_read(&work->data) & WORK_STRUCT_WQ_DATA_MASK);
    }
    // and this is defined as:
    #define WORK_STRUCT_FLAG_MASK (3UL)
    #define WORK_STRUCT_WQ_DATA_MASK (~WORK_STRUCT_FLAG_MASK)  // == 0xfffffffffffffffc
    ```

Ah, so to get the CPU workqueue struct, we recreate the work done in `get_wq_data(work)` which is get the pointer to the `work->data` member and logically or it with `0xfffffffffffffffc`. From there, he casts it to a `cpu_workqueue_struct`. Let's go back to that. 

Cast the `work_struct`:

    ```
    crash> work_struct ffff880061c53740
    struct work_struct {
      data = {
        counter = 0xffff880061c59d01
      }, 
      entry = {
        next = 0xffff88102545ba60, 
        prev = 0xffff880061c53968
      }, 
      func = 0xffffffff81145d90 <lru_add_drain_per_cpu>
    }
    - - - - - - - <snip> - - - - - - -
    ```

Get the `cpu_workqueue_struct` from the `work_struct`:

    ```
    crash> p (0xffff880061c59d01&0xfffffffffffffffc)
    $2 = 0xffff880061c59d00
    ```

Cast the `cpu_workqueue_struct`:

    ```
    crash> cpu_workqueue_struct 0xffff880061c59d00
    struct cpu_workqueue_struct {
      lock = {
        raw_lock = {
          slock = 0x0
        }
      }, 
      worklist = {
        next = 0xffff880061c53a48, 
        prev = 0xffff88102545ba60
      }, 
      more_work = {
        lock = {
          raw_lock = {
            slock = 0x0
          }
        }, 
        task_list = {
          next = 0xffff880061c59d20, 
          prev = 0xffff880061c59d20
        }
      }, 
      current_work = 0xffffffffa051f860 <nfs_automount_task>, 
      wq = 0xffff881029088e80, 
      thread = 0xffff8810290b2ab0   <----
    }
    ```

And now you have a task to look at! 

    ```
    crash> ps 0xffff8810290b2ab0
       PID    PPID  CPU       TASK        ST  %MEM     VSZ    RSS  COMM
         37      2   2  ffff8810290b2ab0  UN   0.0       0      0  [events/2]
    ```

6. What is the task and what is it doing? 


```
crash> bt 37
PID: 37     TASK: ffff8810290b2ab0  CPU: 2   COMMAND: "events/2"
 #0 [ffff8810290cba40] schedule at ffffffff81548980
 #1 [ffff8810290cbb18] schedule_timeout at ffffffff815498f5
 #2 [ffff8810290cbbc8] wait_for_common at ffffffff81549553
 #3 [ffff8810290cbc58] wait_for_completion at ffffffff8154968d
 #4 [ffff8810290cbc68] kthread_stop at ffffffff810a647b
 #5 [ffff8810290cbc98] bdi_unregister at ffffffff81152066
 #6 [ffff8810290cbcc8] nfs_put_super at ffffffffa04d71f9 [nfs]
 #7 [ffff8810290cbcd8] generic_shutdown_super at ffffffff8119c11b
 #8 [ffff8810290cbcf8] kill_anon_super at ffffffff8119c206
 #9 [ffff8810290cbd18] nfs_kill_super at ffffffffa04da905 [nfs]
#10 [ffff8810290cbd38] deactivate_super at ffffffff8119c9a7
#11 [ffff8810290cbd58] mntput_no_expire at ffffffff811bc92f
#12 [ffff8810290cbd88] release_mounts at ffffffff811bd079
#13 [ffff8810290cbdb8] mark_mounts_for_expiry at ffffffff811bd36a
#14 [ffff8810290cbe28] nfs_expire_automounts at ffffffffa04e5cf5 [nfs]
#15 [ffff8810290cbe38] worker_thread at ffffffff8109fba0
#16 [ffff8810290cbee8] kthread at ffffffff810a640e
#17 [ffff8810290cbf48] kernel_thread at ffffffff8100c28a

```


 Looks like pid 2391 is waiting on pid 37.



rash> bt 37
PID: 37     TASK: ffff8810290b2ab0  CPU: 2   COMMAND: "events/2"
 #0 [ffff8810290cba40] schedule at ffffffff81548980
 #1 [ffff8810290cbb18] schedule_timeout at ffffffff815498f5
 #2 [ffff8810290cbbc8] wait_for_common at ffffffff81549553
 #3 [ffff8810290cbc58] wait_for_completion at ffffffff8154968d
 #4 [ffff8810290cbc68] kthread_stop at ffffffff810a647b
 #5 [ffff8810290cbc98] bdi_unregister at ffffffff81152066
 #6 [ffff8810290cbcc8] nfs_put_super at ffffffffa04d71f9 [nfs]
 #7 [ffff8810290cbcd8] generic_shutdown_super at ffffffff8119c11b
 #8 [ffff8810290cbcf8] kill_anon_super at ffffffff8119c206
 #9 [ffff8810290cbd18] nfs_kill_super at ffffffffa04da905 [nfs]
#10 [ffff8810290cbd38] deactivate_super at ffffffff8119c9a7
#11 [ffff8810290cbd58] mntput_no_expire at ffffffff811bc92f
#12 [ffff8810290cbd88] release_mounts at ffffffff811bd079
#13 [ffff8810290cbdb8] mark_mounts_for_expiry at ffffffff811bd36a
#14 [ffff8810290cbe28] nfs_expire_automounts at ffffffffa04e5cf5 [nfs]
#15 [ffff8810290cbe38] worker_thread at ffffffff8109fba0
#16 [ffff8810290cbee8] kthread at ffffffff810a640e
#17 [ffff8810290cbf48] kernel_thread at ffffffff8100c28a
crash>



 #2 [ffff8810290cbbc8] wait_for_common at ffffffff81549553

    ffff8810290cbbd0: ffff8810290b2ab0 0000000000000000
    ffff8810290cbbe0: 00000000290cbc50 0000000000000002
    ffff8810290cbbf0: 0000000000000001 ffff8810290b2ab0
    ffff8810290cbc00: ffffffff8106c500 ffff88061693ff08
    ffff8810290cbc10: ffff88061693ff08 0000000000000286
                                       rbx=k
    ffff8810290cbc20: 0000000000000007 ffff880fba6f8040    <----
                      r12              r13
    ffff8810290cbc30: ffff88061693fef8 ffff880d48ad71b0
                      r14              r15
    ffff8810290cbc40: ffff8801007ad400 ffff8810290cbdd0
                      rbp
    ffff8810290cbc50: ffff8810290cbc60 ffffffff8154968d
 #3 [ffff8810290cbc58] wait_for_completion at ffffffff8154968d


    ffff8810290cbc60: ffff8810290cbc90 ffffffff810a647b
 #4 [ffff8810290cbc68] kthread_stop at ffffffff810a647b


    ffff8810290cbc70: ffff8810290cbc90 ffffffff81151f7a
                      rbx              r12=bdi_writeback *wb
    ffff8810290cbc80: ffff880d48ad7048 ffff880d48ad7148
                      rbp
    ffff8810290cbc90: ffff8810290cbcc0 ffffffff81152066
 #5 [ffff8810290cbc98] bdi_unregister at ffffffff81152066



    293 int kthread_stop(struct task_struct *k)   <----
    294 {
    295         struct kthread *kthread;
    296         int ret;
    297
    298         trace_sched_kthread_stop(k);
    299         get_task_struct(k);
    300
    301         kthread = to_kthread(k);
    302         barrier(); /* it might have exited */
    303         if (k->vfork_done != NULL) {
    304                 kthread->should_stop = 1;
    305                 wake_up_process(k);    <-----





Waiting on pid 17853

crash>    ps ffff880fba6f8040
   PID    PPID  CPU       TASK        ST  %MEM     VSZ    RSS  COMM
  17853      2   0  ffff880fba6f8040  UN   0.0       0      0  [flush-0:45]
crash>



crash> bt 17853
PID: 17853  TASK: ffff880fba6f8040  CPU: 0   COMMAND: "flush-0:45"
 #0 [ffff88061693fc28] schedule at ffffffff81548980
 #1 [ffff88061693fd00] rwsem_down_failed_common at ffffffff8154bb15
 #2 [ffff88061693fd60] rwsem_down_read_failed at ffffffff8154bca6
 #3 [ffff88061693fda0] call_rwsem_down_read_failed at ffffffff812a8334
 #4 [ffff88061693fe08] percpu_down_read at ffffffff8129ebdf
 #5 [ffff88061693fe38] exit_signals at ffffffff81098094
 #6 [ffff88061693fe68] do_exit at ffffffff810817e7
 #7 [ffff88061693fee8] kthread at ffffffff810a6401
 #8 [ffff88061693ff48] kernel_thread at ffffffff8100c28a
crash>


 #1 [ffff88061693fd00] rwsem_down_failed_common at ffffffff8154bb15


    ffff88061693fd08: 0000000000000286 ffff88061693fd40
    ffff88061693fd18: ffffffff8108ef2c fffffffeffffffff
                                       rbx=sem
    ffff88061693fd28: 00000000ffffffff ffffffff81ef57d0
                      r12              r13
    ffff88061693fd38: ffff88061693fef8 ffffffff811529f0
                      r14              r15
    ffff88061693fd48: ffff880d48ad7148 ffff881029af2ab0
                      rbp
    ffff88061693fd58: ffff88061693fd98 ffffffff8154bca6
 #2 [ffff88061693fd60] rwsem_down_read_failed at ffffffff8154bca6

   -30=rwsem_waiter *waiter
    ffff88061693fd68: ffff8801006cbdc0 ffffffff81ef57e0
    ffff88061693fd78: ffff880fba6f8040 ffff880600000001
    ffff88061693fd88: ffffffff8108fbb2 ffffffff81ef57d0
    ffff88061693fd98: ffff88061693fe00 ffffffff812a8334
 #3 [ffff88061693fda0] call_rwsem_down_read_failed at ffffffff812a8334


crash> sym ffffffff81ef57d0
ffffffff81ef57d0 (B) cgroup_threadgroup_rwsem+0x10
crash>



crash> p &cgroup_threadgroup_rwsem
$4 = (struct percpu_rw_semaphore *) 0xffffffff81ef57c0 <cgroup_threadgroup_rwsem>
crash> percpu_rw_semaphore
struct percpu_rw_semaphore {
    unsigned int *fast_read_ctr;
    atomic_t write_ctr;
    struct rw_semaphore rw_sem;
    atomic_t slow_read_ctr;
    wait_queue_head_t write_waitq;
}
SIZE: 0x50
crash> percpu_rw_semaphore -o
struct percpu_rw_semaphore {
   [0x0] unsigned int *fast_read_ctr;
   [0x8] atomic_t write_ctr;
  [0x10] struct rw_semaphore rw_sem;
  [0x30] atomic_t slow_read_ctr;
  [0x38] wait_queue_head_t write_waitq;
}
SIZE: 0x50
crash> rw_semaphore ffffffff81ef57d0
struct rw_semaphore {
  count = 0xfffffe9900000001,
  wait_lock = {
    raw_lock = {
      slock = 0x0
    }
  },
  wait_list = {
    next = 0xffff88061693fd68,
    prev = 0xffff88054dffbdc0
  }
}
crash> list 0xffff88054dffbdc0| wc -l
359
crash>


rash>  ps -S
  RU: 9
  IN: 838
  UN: 364
crash>



back to pid 2391



crash> bt 2391
PID: 2391   TASK: ffff881027715520  CPU: 3   COMMAND: "cgrulesengd"
 #0 [ffff88102545b828] schedule at ffffffff81548980
 #1 [ffff88102545b900] schedule_timeout at ffffffff815498f5
 #2 [ffff88102545b9b0] wait_for_common at ffffffff81549553
 #3 [ffff88102545ba40] wait_for_completion at ffffffff8154968d
 #4 [ffff88102545ba50] flush_work at ffffffff810a0097
 #5 [ffff88102545bac0] lru_add_drain_all at ffffffff81145d33
 #6 [ffff88102545bd10] mem_cgroup_move_task at ffffffff8118a087
 #7 [ffff88102545bd90] cgroup_attach_task at ffffffff810dabcf
 #8 [ffff88102545bde0] attach_task_by_pid at ffffffff810dad4b



   2063  * Find the task_struct of the task to attach by vpid and pass it along to the
   2064  * function to attach either it or all tasks in its threadgroup. Will lock
   2065  * cgroup_mutex and threadgroup; may take task_lock of task.
   2066  */
   2067 static int attach_task_by_pid(struct cgroup *cgrp, u64 pid, bool threadgroup)
   2068 {
   2069         struct task_struct *tsk;
. . .

   2134
   2135         threadgroup_lock(tsk);    <---
   2136
   2137         if (threadgroup)
   2138                 ret = cgroup_attach_proc(cgrp, tsk);
   2139         else
   2140                 ret = cgroup_attach_task(cgrp, tsk);  <------
   2141
   2142         threadgroup_unlock(tsk);
   2143
   2144         put_task_struct(tsk);
   2145         cgroup_unlock();
   2146         return ret;


   2526 static inline void threadgroup_lock(struct task_struct *tsk)
   2527 {
   2528         percpu_down_write(&cgroup_threadgroup_rwsem);   <---
   2529 }



    126  */
    127 void percpu_down_write(struct percpu_rw_semaphore *brw)
    128 {
    129         /* tell update_fast_ctr() there is a pending writer */
    130         atomic_inc(&brw->write_ctr);
    131         /*
    132          * 1. Ensures that write_ctr != 0 is visible to any down_read/up_read
    133          *    so that update_fast_ctr() can't succeed.
    134          *
    135          * 2. Ensures we see the result of every previous this_cpu_add() in
    136          *    update_fast_ctr().
    137          *
    138          * 3. Ensures that if any reader has exited its critical section via
    139          *    fast-path, it executes a full memory barrier before we return.
    140          *    See R_W case in the comment above update_fast_ctr().
    141          */
    142         synchronize_sched_expedited();
    143
    144         /* exclude other writers, and block the new readers completely */
    145         down_write(&brw->rw_sem);   <-----
