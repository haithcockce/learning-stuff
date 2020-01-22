# Perf

## Tracing hardware and kernel events 

#### Intro

- Low-level, low-latency tracing and sampling of activity at the kernel and hardware level
- Can get statistics on very specific things such as CPU Cache misses, branch mispredictions, etc
- Can trace as well against specific pre-defined spots in the kernel or in custom spot in the kernel

###### When to use perf

- Investigating large kernelspace processing 
  - Ex: "System performs slow" and during times of interest is elevated `%sys`
- Investigating very brief performance issues
  - Ex: "We have random latency spikes where latency in processing transactions on the CPUs/in memory goes from an expected 50us to 10ms or more"
- Investigating highly specific kernel events that do not necessitate custom response behavior
  - Ex: Determining what is causing a large number of involuntary faults


- Extremely high resolution data set, can easily generate a large amount of data, and typically requires a well-defined target to investigate. Make sure perf is the right tool. 
  - "Performance is slow" Perf may not capture anything useful, for example, if slowness is completely in %usr
  - "Performance is slow and there is extremely high %sys" perf can trace kernel activity, so it will capture something. Is extremely high %sys expected? If not, then perf!
  - "We have random latency spikes" Not well defined enough. Is the latency in IO activity? Latency in networking? Etc?
  - "We have random latency spikes where latency in processing transactions on the CPUs/in memory goes from an expected 50us to 10ms or more" Handling interrupts, handling page faults, and handling scheduling activity will be the most common reasons for latency at that resolution of time especially when all activity is on CPUs/in memory. Perf can tell us when the processes are being interrupted and why. 
  - "Certain processes are taking longer to perform the same thing on some system than others." If the performance data (collectl/pcp/sar) show similar metrics or nothing obvious, perf _could_ be used, but strace would be a more straightforward tool here, especially since we can not yet tell if the bulk of the time is in userspace or kernelspace processing. 
  - "Certain processes are taking longer to perform the same thing on some system than others and it is all in one syscall" Here perf is perfect to use with the function tracer. Tons of overhead, but will tell us exactly what we are doing and where in the kernel for that syscall. 


###### How Perf Traces and Samples

- The kernel exposes a variety of debugging subsystems via the pseudofilesystem, `debugfs`. Perf predominantly uses the `ftrace` functionality and `perf_events`
  - _Performance Counters for Linux_ CPUs have architecture specific registers which increment based on certain events occurring. 
    - AKA `perf_events` 
    - Think count of context switches.
  - `ftrace` is the Function Tracer (thus the name) embedded in `debugfs` but does so much more than tracing functions. Ftrace relies on tracepoints
    - _Tracepoints_ predefined static hooks in kernel code to call a function. 
    - _Probe_ The function called by the hook 
    - If a tracepoint is "on", then a probe is enabled. "Off" means the probe is disabled.
    - Output of the tracing is written to a pseudofile in `debugfs` to be consumed by another tool (or read with `cat` or `less`)
  
- Example code of a tracepoint in the kernel code: 

```c
/kernel/timer.c
1173 static void call_timer_fn(struct timer_list *timer, void (*fn)(unsigned long),
1174               unsigned long data)
1175 {
[...]
1197     trace_timer_expire_entry(timer);
1198     fn(data);
1199     trace_timer_expire_exit(timer);
```

    - Line 1197: if the tracepoint is enabled, then we can trace specifically right before we call the function in a timer 
    - Line 1198: the function being called in the timer
    - Line 1199: if the tracepoint is enabled, then we can trace specifically immediately following the function called in a timer
    - Note that the tracepoints for Line 1197 and 1199 are separate tracepoints. 

- Perf provides insight via analysis of samples 
  - _Sampling_ is the action of recording observations of variables
    - For example, when sampling coin flips, the coin is the variable because it can be heads or tails at any point, and a sample would be a coin flip result such as a coin flip resulting in heads.
    - When you build a large enough _sample size_, or amount of observations, you can create statistics on the resulting data and predict probabilities on future observations. 
    - For a contrived example, after 1,000,000 coin tosses, we could assess that the fairness of the coin is 0.5 (or 50% either heads or tails) and from that assess that, given the fairness of 0.5, the probability of getting heads next coin flip is 0.5 (or 50%). 
  - When a tracepoint is hit or event is triggered that kicks off `perf_events`, then perf will record some data that it is configured to gather. 
    - If we sample backtraces over and over, then we can build statistics on the function calls and which functions called which other functions. 
    - For example, a perf recording of some process might show all instances of kernelspace processing start with `sys_open()`, however 80% of those opens lead to files in a filesystem being opened and the remaining 20% are network sockets being opened. Of that 80% of files opening, 70% of `sys_open()` calls open xfs files while 10% of `sys_open()` calls opened a procfs (`/proc`) file
    - Perf is often configured to gather backtraces but can be configured to capture any number of things. 
  - The sampling allows _statistical profiling_ of the application. In other words, by checking statistics of the samples, we can interpret trends and behaviors (such as if specific functions are called a lot or locks are contested a ton, why a process keeps getting kicked off a cpu, etc)

##### Interacting with `debugfs`

- Perf interacts with the `ftrace` infrastructure. Most tracing within `ftrace` can be enabled manually by echoing values into `debugfs` entries. For example: 

```bash
echo function > /sys/kernel/debug/tracing/current_tracer  # Enable function tracing in kernel
echo 1 > /sys/kernel/debug/tracing/events/irq/enable      # Enable tracing in interrupts
echo 1 > /sys/kernel/debug/tracing/tracing_on             # Begin tracing
echo 0 > /sys/kernel/debug/tracing/tracing_on             # End tracing
less /sys/kernel/debug/tracing/trace                      # Check the output
```

- Tracepoints are listed in `/sys/kernel/debug/tracing/available_events`
- With the complexity of what can be traced, navigating the `debugfs` and setting up regex filters can be error prone and overall difficult. 
- Perf helps facilitate all of this!

##### Events/Tracepoints/Counters in Perf

- `perf list` lists all tracepoints and counters possible that perf can interact with
- `perf probe` can add/list/remove dynamic events at specific points in a function, lines in source code, or elsewhere 

#### General Profiling

- Most of this will be pulled from [here](http://www.brendangregg.com/perf.html)
- Profiling is most useful for times where trends in behavior need to be interpreted in the system or application but not much may be understood about the system or application's behavior. Example uses:
  - "The system is performing poorly" and SAR shows fairly elevated %sys
  - "Application runs slow but system is fine". SAR shows little, but collectl/pcp shows the application incurring higher %sys
  - "Kworker threads are extremely active" 

- Typical uses 

  - `perf record -ag -- sleep N` record backtraces (`-g`) of all cpus (`-a`) at a regular frequency (`-F X` can change frequency to `X` HZ) for `N` seconds
  - `perf record -g -t <TID> -F 99` record backtraces (`-g`) for the thread `<TID>` at a frequency of 99 HZ
  - `perf record -e workqueue:workqueue_queue_work` record instances of work being queued to a workqueue from a process

- Example from contrived workload: 

```bash
 r7 # for i in {1..8}; do dd if=/dev/urandom of=/dev/null & done
[1] 24165
[2] 24166
[3] 24167
[4] 24168
[5] 24169
[6] 24170
[7] 24171
[8] 24172

 r7 # perf record -ag -- sleep 30
[ perf record: Woken up 112 times to write data ]
[ perf record: Captured and wrote 28.475 MB perf.data (240077 samples) ]

 r7 # perf archive
Now please run:

$ tar xvf perf.data.tar.bz2 -C ~/.debug

wherever you need to run 'perf report' on.

 r7 # tar -xf perf.data.tar.bz2 -C ~/.debug
 r7 # perf report --stdio --sort overhead_sys,comm --show-cpu-utilization
# To display the perf.data header info, please use --header/--header-only options.
#
#
# Total Lost Samples: 0
#
# Samples: 240K of event 'cpu-clock'
# Event count (approx.): 60002000000
#
# Children      Self       sys       usr  Command       
# ........  ........  ........  ........  ..............
#
    99.86%    99.86%    78.00%    21.86%  dd            
            |          
            |--78.17%--__GI___libc_read
            |          |          
            |           --70.67%--system_call
            |                     |          
            |                      --68.70%--sys_read
            |                                |          
            |                                 --68.02%--vfs_read
            |                                           |          
            |                                           |--64.81%--urandom_read
            |                                           |          |          
            |                                           |          |--51.82%--extract_crng
            |                                           |          |          |          
            |                                           |          |           --51.25%--_extra
            |                                           |          |                     |     
            |                                           |          |                      --50.
            |                                           |          |          
            |                                           |          |--9.93%--crng_backtrack_pro
            |                                           |          |          |          
            |                                           |          |           --9.82%--_crng_b
            |                                           |          |                     |     
            |                                           |          |                      --9.3
```

- Contrast the above to a `perf record` during idle times: 

```bash
 r7 # perf record -ag -o perf.data.idle -- sleep 30
[ perf record: Woken up 32 times to write data ]
[ perf record: Captured and wrote 8.150 MB perf.data.idle (69202 samples) ]
 r7 # perf report --stdio --sort overhead_sys,comm --show-cpu-utilization -i perf.data.idle
# To display the perf.data header info, please use --header/--header-only options.
#
#
# Total Lost Samples: 0
#
# Samples: 69K of event 'cpu-clock'
# Event count (approx.): 17300500000
#
# Children      Self       sys       usr  Command     
# ........  ........  ........  ........  ............
#
    99.06%    99.06%    99.06%     0.00%  swapper     
            |
            ---start_cpu
               |          
               |--70.40%--start_secondary
               |          cpu_startup_entry
               |          |          
               |           --70.40%--arch_cpu_idle
               |                     |          
               |                      --70.40%--default_idle
               |                                |          
               |                                 --70.39%--native_safe_halt
               |          
                --28.66%--x86_64_start_kernel
                          x86_64_start_reservations
                          start_kernel
                          rest_init
                          cpu_startup_entry
                          |          
                           --28.65%--arch_cpu_idle
                                     default_idle
                                     |          
                                      --28.63%--native_safe_halt

     0.47%     0.47%     0.32%     0.15%  pmdaproc    
     0.12%     0.12%     0.08%     0.04%  pmdalinux   
     0.11%     0.11%     0.11%     0.00%  perf        
     0.06%     0.06%     0.06%     0.00%  kworker/0:2 
     0.04%     0.04%     0.04%     0.01%  cgrulesengd
```

- Breadown of what is observed. 
  - To synthesize a load to produce interesting data, several `dd` commands are fored to the background to churn on the CPUs. From there, `perf record` begins recording. It grabs the backtraces (`-g`) of processes running on all CPUs (`-a`). It records while the `sleep 30` command is executing. Note, this command is arbitrary and optional. If a command is provided, then perf will record while the command is executing and finish recording when the command finishes. 
  - Perf will create a per-cpu buffer to write events to which are later on picked up by perf. In both, we see `Total Lost Samples: 0` meaning all events were recorded and captured. Sometimes, depending on activity and what is being recorded, you can lose samples because the events happen at a frequency which fills the buffers faster than perf can handle them. Here, we lost no samples of events.
  - Next we see `Samples: 240K of event 'cpu-clock'`, which indicates the event being monitored and the count of times that event fired. `cpu-clock` is a per-cpu event which performs regular time keeping and happens extremely frequently. 
  - `Event count (approx.): 17300500000` means, approximately, 17300500000 events fired while recording. This would be summed across all events being monitored. 
  - The interesting stuff is next! Perf breaksdown a bactrace into `self`, or the function we are looking at currently, and `children`, or functions called by the one we are looking at. For example, in the busy perf data, when looing at `68.70%--sys_read`, the `sys_read` is "self" while the function it calls, `vfs_read` is the "child" function. 
  - The "Children" and "Self" numbers are typically the same and indicate the percentage of all samples the process is a part of. So for the busy perf data, `dd` is in 99.86% of all samples recorded. In the idle perf data, `swapper` is in 99.06% of all samples while `pmdaproc` is in 0.47% of all samples. 
  - `sys` and `usr` is the percentage of samples a process was executing in kernelspace or userspace respectively and should sum to `self`. For the busy perf data, 78% of all samples were `dd` processes in kernelspace execution whereas 21.86% of all samples are `dd` processes in userspace execution. 
  - The next parts are regarding the call graph (all bactraces seen in the perf data organized as a tree). For example, in the idle perf data, `swapper` always calls `start_cpu` which calls `cpu_startup_entry` then `start_secondary` in 70.40% of all samples in the data while `start_cpu` branches instead to `cpu_startup_entry` -> `rest_init` -> `start_kernel` -> `x86_64_start_reservations` -> `x86_64_start_kernel` in 28.66% of all samples. 
  - As a reminder, perf is recording events and doing sample-based metric reporting. This creates a square-rectangle scenario where high `%sys` in SAR/collectl/pcp would mean tons of samples of execution in kernelspace in perf data while high `%sys` in perf does not mean high `%sys` in SAR/collectl/pcp. For example, above, `swapper` is a kernelspace thread which idles a CPU. If a system is idle, then a ton of samples will be in kernelspace processing but due to `swapper` idling CPUs. 
  - When using perf like this, check for areas where the kernelspace processing is concentrated in the same functions or bactraces. In the busy perf data, over 60% of all samples are `dd` attempting to read `/dev/urandom`. If the application was something else and the chief complaint is performance issues with that application, then the application vendor would need to investigate why it is reading from `/dev/urandom` so much. 
  - Alternatively, bactraces can start in a variety of places but all end in the same set of functions. Take for example updating inodes in a filesystem. If a wide variety of operations are occurring on the same inode (read, write, truncate, close, unlink, etc) you could have a wide variety of starting points for the bactraces but they could end up grinding behind the same lock for the same inode. 

#### Static Kernel Tracing

- Developers builtin tracepoints in logically reasonable locations to gather a number of different types of data. These built in tracepoints are _static_ as the locations and what is traced does not change unless the kernel maintainers change the code in future commits. 
- `perf list` can show a listing of all available tracepoints. Unfortunately, determining what gives you what you need can be tricky. All events are implemented in `include/trace/events/` where the details of what is reported can be checked. 

```bash
 $ ls include/trace/events/
9p.h                f2fs.h       irq.h      net.h        rpm.h       udp.h
asoc.h              filelock.h   jbd2.h     oom.h        sched.h     vmscan.h
bcache.h            filemap.h    jbd.h      pagemap.h    scsi.h      vsock_virtio_transport_common.h
block.h             fs_dax.h     kmem.h     power.h      signal.h    workqueue.h
bridge.h            gfpflags.h   kvm.h      printk.h     skb.h       writeback.h
btrfs.h             gpio.h       libata.h   qdisc.h      sock.h      xdp.h
compaction.h        host1x.h     lock.h     random.h     sunrpc.h    xen.h
context_tracking.h  hswadsp.h    mce.h      rcu.h        syscalls.h
devlink.h           i2c.h        migrate.h  rdma.h       target.h
dma_fence.h         intel_ish.h  mmc.h      regmap.h     task.h
ext3.h              intel-sst.h  module.h   regulator.h  thp.h
ext4.h              iommu.h      napi.h     rpcrdma.h    timer.h
```

- For example, if unsure about what is printed when tracing `timer_expire_entry` events, below is the implementation: 

```c
 74 /**
 75  * timer_expire_entry - called immediately before the timer callback
 76  * @timer:  pointer to struct timer_list
 77  *
 78  * Allows to determine the timer latency.
 79  */
 80 TRACE_EVENT(timer_expire_entry,
 81 
 82     TP_PROTO(struct timer_list *timer),
 83 
 84     TP_ARGS(timer),
 85 
 86     TP_STRUCT__entry(
 87         __field( void *,    timer   )
 88         __field( unsigned long, now )
 89         __field( void *,    function)
 90     ),
 91 
 92     TP_fast_assign(
 93         __entry->timer      = timer;
 94         __entry->now        = jiffies;
 95         __entry->function   = timer->function;
 96     ),
 97 
 98     TP_printk("timer=%p function=%pf now=%lu", __entry->timer, __entry->function,__entry->now)
 99 );
```

- So it prints the address of the timer, the function being executed in the timer, and the time when the timer fired in terms of jiffies. 
- Below is an example of the output:

```bash
 r7 # perf record -e timer:timer_expire_entry 
^C[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.227 MB perf.data (68 samples) ]

 r7 # perf report --stdio | head -20
# To display the perf.data header info, please use --header/--header-only options.
#
#
# Total Lost Samples: 0
#
# Samples: 68  of event 'timer:timer_expire_entry'
# Event count (approx.): 68
#
# Overhead  Trace output                                                           
# ........  .......................................................................
#
     1.47%  timer=0xffff9c5df66f6080 function=delayed_work_timer_fn now=4470243840
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243550
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243600
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243650
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243700
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243750
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243800
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243850
     1.47%  timer=0xffff9c5e19a7fd60 function=process_timeout now=4470243900
```

##### Example Usage: Load average but no blocked or running tasks

- An example usage in support of static tracepoints would be when a customer notes elevated load average but no interesting CPU usage or blocked task count metrics. Typically, elevated load averages with no interesting CPU usage and low blocked task counts can be attributed to intermittent forking of children processes. 
- Perf can enable a tracepoint near where a new process is created in the kernel to show what is being forked so much. 
```bash
 r7 # ps | grep bash  # grab the PID of the bash process where we will synthesize load
27701 pts/0    00:00:43 bash
 r7 # while [ 1 ]; do for i in {1..1000}; do  ls > /dev/null & done; sleep 1; done  # synthesize load
 r7 # sar -q 10  # below, we can see load average rising but nothing in particularly interesting otherwise
Linux 3.10.0-1121.el7.x86_64 (r7)       22/01/20        _x86_64_        (2 CPU)

13:33:00      runq-sz  plist-sz   ldavg-1   ldavg-5  ldavg-15   blocked
13:33:10            0       157      0.15      0.43      0.25         0
13:33:20            0       158      0.13      0.41      0.25         0
13:33:30            0       157      0.55      0.50      0.28         0
13:33:40            0       157      1.27      0.66      0.33         0
13:33:50            0       157      1.96      0.84      0.39         0
13:34:00            0       157      2.32      0.96      0.44         0
13:34:10            0       157      2.56      1.06      0.47         0
13:34:20            0       157      2.97      1.20      0.53         0
13:34:30           10       165      3.33      1.34      0.58         0
13:34:40            8       165      3.63      1.48      0.63         0
13:34:50            9       165      4.78      1.79      0.75         0
13:35:00            9       164      4.68      1.86      0.78         0
13:35:10            8       162      4.28      1.87      0.79         0
13:35:20            3       158      4.03      1.89      0.81         0
13:35:30            4       160      4.13      1.98      0.85         0
^C
 r7 # perf record -e sched:sched_process_exec -a -p 27701 -- sleep 10  # record against the bash process we created load in 
Warning:
PID/TID switch overriding SYSTEM
[ perf record: Woken up 31 times to write data ]
[ perf record: Captured and wrote 7.649 MB perf.data (5005 samples) ]
 r7 # perf script | head -5
 r7 # perf script | head -5
              ls 28085 [001] 176637.736503: sched:sched_process_exec: filename=/usr/bin/ls pid=28085 old_pid=28085
              ls 28086 [000] 176637.736803: sched:sched_process_exec: filename=/usr/bin/ls pid=28086 old_pid=28086
              ls 28087 [001] 176637.738598: sched:sched_process_exec: filename=/usr/bin/ls pid=28087 old_pid=28087
              ls 28088 [000] 176637.738768: sched:sched_process_exec: filename=/usr/bin/ls pid=28088 old_pid=28088
              ls 28089 [001] 176637.740263: sched:sched_process_exec: filename=/usr/bin/ls pid=28089 old_pid=28089
 r7 # perf script | awk '{print $1}' | sort | uniq -c
   5000 ls
      5 sleep
```

- Unsurprisingly, the forking is resulting in a ton of `ls` commands running and a few `sleep` commands. This matches the synthesized load. 

##### Example: `kworker` threads running wild

- In some instances, kworker threads can become extremely active. As a small primer on kworker threads, these threads are responsible for carrying out deferred work queued into the workqueues by other entities. Think similar behavior of interrupts but they run in a process at some later point in a kworker process. 
- If a kworker thread is extremely active, we can trace what is being queued into the workqueues and assess where the work may be coming from. 

```bash
 r7 # perf record -e workqueue:workqueue_queue_work -a -- sleep 60
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.475 MB perf.data (1005 samples) ]
 r7 # perf report --stdio | head -20
# To display the perf.data header info, please use --header/--header-only options.
#
#
# Total Lost Samples: 0
#
# Samples: 1K of event 'workqueue:workqueue_queue_work'
# Event count (approx.): 1005
#
# Overhead  Trace output                                                                                                          
# ........  ......................................................................................................................
#
    36.92%  work struct=0xffff9c5e18d93600 function=flush_to_ldisc workqueue=0xffff9c5e3d10e000 req_cpu=5120 cpu=1
    18.11%  work struct=0xffff9c5e18d93600 function=flush_to_ldisc workqueue=0xffff9c5e3d10e000 req_cpu=5120 cpu=0
     5.97%  work struct=0xffffffffb8e5df40 function=sync_cmos_clock workqueue=0xffff9c5e3d10e000 req_cpu=5120 cpu=0
     5.47%  work struct=0xffffffffb8e9a280 function=vmstat_shepherd workqueue=0xffff9c5e3d10e000 req_cpu=5120 cpu=1
     4.68%  work struct=0xffff9c5e18d93000 function=flush_to_ldisc workqueue=0xffff9c5e3d10e000 req_cpu=5120 cpu=0
     3.38%  work struct=0xffff9c5e3fd17de0 function=vmstat_update workqueue=0xffff9c5e3d10e000 req_cpu=5120 cpu=1
     2.89%  work struct=0xffff9c5df66f6060 function=disk_events_workfn workqueue=0xffff9c5e3d10e800 req_cpu=5120 cpu=0
     2.89%  work struct=0xffff9c5e1ad3c0b8 function=ata_sff_pio_task workqueue=0xffff9c5e1ad44200 req_cpu=5120 cpu=0
     2.69%  work struct=0xffff9c5e3fc17de0 function=vmstat_update workqueue=0xffff9c5e3d10e000 req_cpu=0 cpu=0
```

- In the above, work was queued into workqueues ~1000 times and roughly 55.03% of all work queued was `flush_to_ldisc`. 
- A quick search through the kernel source shows `flush_to_ldisc` is defined in `drivers/tty/tty_buffer.c` and thus likely related to writing to a terminal or console. The comment for the function is as follows: 

```c
424 /**
425  *  flush_to_ldisc
426  *  @work: tty structure passed from work queue.
427  *
428  *  This routine is called out of the software interrupt to flush data
429  *  from the buffer chain to the line discipline.
430  *
431  *  Locking: holds tty->buf.lock to guard buffer list. Drops the lock
432  *  while invoking the line discipline receive_buf method. The
433  *  receive_buf method is single threaded for each tty instance.
434  */
```

- As such, we can safely assume the bulk of the work here is simply flushing the contents of a tty buffer and writing the contents to a terminal or console. 

##### Questions

- For the forking example, why trace only the `sched_process_exec` event and not do a full profile via something like `perf record -ag`? 
- For the workqueue example, why trace only the `workqueue_queue_work` event and not trace the `kworker` activity directly? 

## Resources 

- Tracepoints

  - `/usr/share/doc/kernel-doc-3.10.0/Documentation/trace/tracepoints.txt` Contains an overview of tracepoint information 

- http://www.brendangregg.com/perf.html
- http://www.brendangregg.com/blog/2017-03-16/perf-sched.html
