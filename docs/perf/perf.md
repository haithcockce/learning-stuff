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

- An example usage in support of static tracepoints would be when a customer notes elevated load average but no interesting CPU usage or blocked task count metrics. Typically, elevated load averages with no interesting CPU usage and low blocked task counts can be attributed to intermittent forking of children processes or burst activity within longer living processes. 
- Perf can enable a tracepoint near where a new process is created in the kernel to show what is being forked so much. 

```bash
 r7 # while [ 1 ]; do for i in {1..1000}; do  ls > /dev/null & done; sleep 1; done  # synthesize load
 r7 # sar -q 10  # below, we can see load average rising but nothing in particularly interesting otherwise
Linux 3.10.0-1123.el7.x86_64 (r7) 	03/16/2020 	_x86_64_	(2 CPU)

01:12:38 PM   runq-sz  plist-sz   ldavg-1   ldavg-5  ldavg-15   blocked
01:12:48 PM         0       160      3.39      1.31      0.52         0
01:12:58 PM         7       160      3.51      1.40      0.55         0
01:13:08 PM         0       155      3.37      1.43      0.57         0
01:13:18 PM         0       155      3.01      1.42      0.58         0
01:13:28 PM         0       155      2.95      1.46      0.60         0
01:13:38 PM         0       155      2.64      1.44      0.60         0
01:13:48 PM         0       160      3.77      1.72      0.70         0
01:13:58 PM         0       154      3.59      1.75      0.72         0
01:14:08 PM         0       154      3.44      1.77      0.74         0
^C
 r7 # perf record -e sched:sched_process_fork -e sched:sched_process_exec -a -- sleep 10
[ perf record: Woken up 36 times to write data ]
[ perf record: Captured and wrote 9.272 MB perf.data (10538 samples) ]
 r7 # perf script
           sleep  8142 [000]  4125.883636: sched:sched_process_exec: filename=/usr/bin/sleep pid=8142 old_pid=8142
            bash 14520 [001]  4126.787632: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8143  (1)
            bash 14520 [001]  4126.787918: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8144
            bash 14520 [001]  4126.788157: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8145
            bash 14520 [001]  4126.788505: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8146
              ls  8143 [000]  4126.788679: sched:sched_process_exec: filename=/usr/bin/ls pid=8143 old_pid=8143          (2)
            bash 14520 [001]  4126.788744: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8147
            bash 14520 [001]  4126.788991: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8148
            bash 14520 [001]  4126.789255: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8149
            bash 14520 [001]  4126.789536: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8150
            bash 14520 [001]  4126.789767: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8151
            bash 14520 [001]  4126.789991: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8152
            bash 14520 [001]  4126.790215: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8153
            bash 14520 [001]  4126.790462: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8154
            bash 14520 [001]  4126.790690: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8155
            bash 14520 [001]  4126.790954: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8156
            bash 14520 [001]  4126.791183: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8157
            bash 14520 [001]  4126.791447: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8158
            bash 14520 [001]  4126.791714: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8159
            bash 14520 [001]  4126.791923: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8160
              ls  8152 [000]  4126.792616: sched:sched_process_exec: filename=/usr/bin/ls pid=8152 old_pid=8152
              ls  8147 [001]  4126.792808: sched:sched_process_exec: filename=/usr/bin/ls pid=8147 old_pid=8147
              ls  8148 [000]  4126.792827: sched:sched_process_exec: filename=/usr/bin/ls pid=8148 old_pid=8148
              ls  8146 [000]  4126.793280: sched:sched_process_exec: filename=/usr/bin/ls pid=8146 old_pid=8146
              ls  8149 [000]  4126.793636: sched:sched_process_exec: filename=/usr/bin/ls pid=8149 old_pid=8149
              ls  8153 [001]  4126.794045: sched:sched_process_exec: filename=/usr/bin/ls pid=8153 old_pid=8153
              ls  8151 [000]  4126.795040: sched:sched_process_exec: filename=/usr/bin/ls pid=8151 old_pid=8151
              ls  8154 [000]  4126.795230: sched:sched_process_exec: filename=/usr/bin/ls pid=8154 old_pid=8154
              ls  8150 [001]  4126.795284: sched:sched_process_exec: filename=/usr/bin/ls pid=8150 old_pid=8150
            bash 14520 [001]  4126.796356: sched:sched_process_fork: comm=bash pid=14520 child_comm=bash child_pid=8161
              ls  8155 [001]  4126.796730: sched:sched_process_exec: filename=/usr/bin/ls pid=8155 old_pid=8155
              ls  8145 [001]  4126.798180: sched:sched_process_exec: filename=/usr/bin/ls pid=8145 old_pid=8145
[...]
```

- Above, the PIDs can be matched between instances of forks, where we see bash forking, and instances of process execution, with is the ls commands running. For example, bash forks a child whose PID is 8143 (1) and later on, ls executes with pid 8143 (2). 
- The above events help when the issue is with super heavy forking activity, but if the perpetrator are a large number of already active threads having burst activity, forking will be nonexistant and the above tracepoints are useless. Below is a python reproducer script to create several threads and spin in userspace: 

```python
#!/usr/bin/env python

from time import sleep
from random import randint
import threading

def work():
    n = 0
    for i in xrange(500):
        n = n + i
        n = n / 2
        n = n - randint(0, 100)
    return n

def spin():
    while True:
        work()
        sleep(1)

if __name__ == '__main__':
    threadpool = []
    for i in xrange(1000):
        t = threading.Thread(target=spin)
        threadpool.append(t)
        t.start()
```

- Below is the same perf command as above: 

```bash
 r7 # perf record -e sched:sched_process_fork -e sched:sched_process_exec -a -- sleep 10[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.318 MB perf.data (1 samples) ]
 r7 # perf script
           sleep  5906 [000]  2617.029599: sched:sched_process_exec: filename=/usr/bin/sleep
```

- Because there is no forking avitivity but rather the burst activity is all in long living threads, the `sched:sched_process_fork` and `sched:sched_process_exec` were not triggered except for the lone sleep command above. 
- With burst acitivity, the long living processes will instead jump onto and off of the CPUs frequently, so `sched:sched_wakeup` can be used instead: 

```bash
 r7 # perf record -e sched:sched_wakeup -a -- sleep 10
[ perf record: Woken up 509 times to write data ]
[ perf record: Captured and wrote 127.805 MB perf.data (1392668 samples) ]
 r7 # perf script | head -20
            perf 10305 [001]  2980.951844: sched:sched_wakeup: python:3621 [120] success=1 CPU:001
            perf 10305 [001]  2980.951848: sched:sched_wakeup: python:4336 [120] success=1 CPU:001
          python  3824 [000]  2980.951848: sched:sched_wakeup: python:4361 [120] success=1 CPU:000
          python  3824 [000]  2980.951855: sched:sched_wakeup: python:4379 [120] success=1 CPU:000
          python  3824 [000]  2980.951861: sched:sched_wakeup: python:3656 [120] success=1 CPU:000
          python  3824 [000]  2980.951866: sched:sched_wakeup: python:4072 [120] success=1 CPU:000
          python  3824 [000]  2980.951871: sched:sched_wakeup: python:3948 [120] success=1 CPU:000
            perf 10305 [001]  2980.951883: sched:sched_wakeup: python:4171 [120] success=1 CPU:001
            perf 10305 [001]  2980.951889: sched:sched_wakeup: python:3873 [120] success=1 CPU:001
            perf 10305 [001]  2980.951909: sched:sched_wakeup: python:3778 [120] success=1 CPU:001
          python  3778 [001]  2980.951916: sched:sched_wakeup: python:4018 [120] success=1 CPU:001
         swapper     0 [000]  2980.951933: sched:sched_wakeup: python:4377 [120] success=1 CPU:000
         swapper     0 [000]  2980.951934: sched:sched_wakeup: python:3597 [120] success=1 CPU:000
          python  3597 [000]  2980.951948: sched:sched_wakeup: python:4332 [120] success=1 CPU:000
            perf 10305 [001]  2980.951950: sched:sched_wakeup: perf:10317 [120] success=1 CPU:001
          python  3597 [000]  2980.951954: sched:sched_wakeup: python:3838 [120] success=1 CPU:000
          python  3597 [000]  2980.951960: sched:sched_wakeup: python:4317 [120] success=1 CPU:000
            perf 10317 [001]  2980.951973: sched:sched_wakeup: python:3765 [120] success=1 CPU:001
            perf 10317 [001]  2980.951978: sched:sched_wakeup: python:3829 [120] success=1 CPU:001
            perf 10317 [001]  2980.951987: sched:sched_wakeup: python:4223 [120] success=1 CPU:001
```

- The `sched:sched_wakeup` tracepoint is triggered quite literally when a process has been kicked to wake back up and get into the runqueues on the CPUs. 
- The above can be read as such, column 3 is the CPU the process in column 1 with PID in column 2 were running on when the `sched:sched_wakeup` event was triggered. The process woken up is column 6 on the CPU in column 9. 
  - For example, in the first record, the `perf` command with PID 10305 was running on CPU 1 when python with PID 3621 wokeup on CPU 1. 
  - Similarly, python with PID 3824 was running on CPU 0 when python with PIDs 4361, 4379, 3656, 4072, and 3948 wokeup on CPU 0.
- The large number of python processes waking up help isolate the issue in this case to python likely performing the burst activity.

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


#### Dynamic Kernel Tracing

- The kernel has a number of built in tracepoints, however, despite the larger number of tracepoints, sometimes you need extremely specific instances of tracing done (or a subsystem may not have many tracepoints to begin with such as most of Infiniband). 
- The linux kernel allows inserting probes dynamically without rewriting/recompiling the kernel in select spots. 
- These dynamic tracepoints/probes do not have associated actions with them like the static tracepoints do (probing `sched:sched_switch` gets the previous and next tasks for example). To get data from them, you will need to pass in options to another `perf record` command.
- Once a probe point is inserted, then it becomes an event you can monitor on (`perf record -g -e probe:my_cool_probe` for example)
- **Note** _The remainder of this section requires the debuginfo kernel installed._
  - On Red Hat Enterprise Linux 7 and below, `debuginfo-install kernel`
  - On Red Hat Enterprise Linux 8 and above, `dnf debuginfo-install kernel`

- `perf probe --line <FUNC or FILE>`
  - Lists points in the function or source code where probes can be inserted
  - `perf probe --line do_sys_open`
  - `perf probe --line fs/open.c:100` 

- `perf probe --add <FUNC or FILE:NNN>`
  - Adds a probe at the first available point in `FUNC` or at `FILE:NNN` if able to where `NNN` is the line number
  - Remember this is just a tracepoint/probe that has no unique additional functionalit
  - Example: 
```bash
 r7 # perf probe -a do_sys_open
Added new event:
  probe:do_sys_open    (on do_sys_open)

You can now use it in all perf tools, such as:

        perf record -e probe:do_sys_open -aR sleep 1
```

- `perf probe --list` lists active probes (tracepoints that are currently active)

#### Mixing Profiling and Tracing

- Tracing and profiling can be mixed to create more targeted data sets. 
- Useful for when particular kernelspace events need to be monitored and gather a sample set. For example:
  - Gathering a breakdown of all events which cause a specific process to be scheduled off a CPU and how we get there (backtrace)
  - Profiling the syscalls taken by a userspace process and what call chains typically lead us there
  - Gather a breakdown of time spent in functions when performing `sys_open()` calls
- Tracing scheduler latency best exemplifies this but is not the only reason to do so. 

##### Example Pt 1: Why Does My Application Show Great Latency Randomly?

- Many customers run low latency apps for things like stock trading. These require microsecond (us) and nanosecond resolution often and requires constantly to be on a CPU typically. 
- If troubleshooting shows the application is incurring latency in kernelspace even when not doing actions that would stall an application spontaneously (such as waiting on a `wait()` or `select()` or `read()` from a socket/pipe), then the most common reason is being scheduled off the CPU or receiving interrupts. 
- We can check for certain events via `perf`. For example: 
```bash
 r7 # perf record -e sched:sched_switch -g -- sleep 1
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.016 MB perf.data (2 samples) ]
 r7 # perf script
sleep 31285 [001] 971577.494048: sched:sched_switch: sleep:31285 [120] R ==> cgrulesengd:827 [120]
        ffffffffb8985a24 __schedule+0x514 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinux)
        ffffffffb8986be1 schedule_user+0x31 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinu
        ffffffffb89931cd int_careful+0x14 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinux)
            7f93991a6140 _start+0x0 (/usr/lib64/ld-2.17.so)

sleep 31285 [001] 971577.494608: sched:sched_switch: sleep:31285 [120] S ==> swapper/1:0 [120]
        ffffffffb8985a24 __schedule+0x514 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinux)
        ffffffffb8985d79 schedule+0x29 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinux)
        ffffffffb8984da0 do_nanosleep+0x70 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinux
        ffffffffb82cb29b hrtimer_nanosleep+0xbb (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vm
        ffffffffb82cb3f6 sys_nanosleep+0x96 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinu
        ffffffffb8992ed2 system_call+0x162 (/usr/lib/debug/lib/modules/3.10.0-1121.el7.x86_64/vmlinux
            7f9398e9c840 __GI___libc_nanosleep+0x10 (/usr/lib64/libc-2.17.so)
```
- The above example uses the `sched:sched_switch` event as a tracepoint and gathers backtraces (`-g`) when they happen for the sleep command. Of course, this is contrived and we expected this sleep process to be scheduled off the CPU as it goes to sleep. 
- Below is far more interesting: 
```bash
 r7 # perf record -e sched:sched_switch -g -- stress-ng --cpu 2 --cpu-method hamming
stress-ng: info:  [12966] defaulting to a 86400 second run per stressor
stress-ng: info:  [12966] dispatching hogs: 2 cpu
^C[ perf record: Woken up 2 times to write data ]
stress-ng: info:  [12966] successful run completed in 80.38s (1 min, 20.38 secs)
[ perf record: Captured and wrote 0.479 MB perf.data (2725 samples) ]
r7 # perf report --stdio
# To display the perf.data header info, please use --header/--header-only options.
#
#
# Total Lost Samples: 0
#
# Samples: 2K of event 'sched:sched_switch'
# Event count (approx.): 2725
#
# Children      Self  Trace output
# ........  ........  ...........................................................
#
    58.83%    58.83%  stress-ng-cpu:12968 [120] R ==> xfsaild/dm-0:401 [120]
            |
            |--1.87%--0x40c010
            |          retint_careful
            |          schedule_user
            |          __schedule
            |
            |--1.65%--0x40c013
            |          retint_careful
            |          schedule_user
            |          __schedule
            |
            |--1.61%--0x40c089
            |          retint_careful
[...]

     8.11%     8.11%  stress-ng-cpu:12967 [120] R ==> kworker/0:3:10583 [120]
     7.78%     7.78%  stress-ng-cpu:12967 [120] R ==> rcu_sched:9 [120]
     3.23%     3.23%  stress-ng-cpu:12967 [120] R ==> in:imjournal:1300 [120]
     3.19%     3.19%  stress-ng-cpu:12968 [120] R ==> kworker/1:0:31548 [120]
     2.50%     2.50%  stress-ng-cpu:12968 [120] R ==> rcu_sched:9 [120]
[...]
```
- Above, we stressed the CPUs with two threads performing hamming distance calculations. To record what was happening with those threads, we recorded against those threads with a `perf record` and watching for anytime those `stress-ng` threads were kicked off a CPU for something else. 
- In the breakdown, we see the majority of rescheduleing was to let xfsaild run. The call chains `retint_careful` -> `schedule_user` -> `__schedule` means that we are choking up the CPU. Processes are attempting to preempt the application and we give up the CPU to allow those applications to run. Here it's mostly `xfsaild` but a few other things like `kworker` and `rcu_sched`. Here, we can at least confirm (in this second contrived example) that the system is not tuned properly for the workload expected as other kernelspace threads are taking over from time to time. CPU affinity should be set for various kernel threads and rcu callbacks should be disabled to attempt mitigating this. 

##### Example Pt 2

- Thankfully, because checking latency via perf is such a prominent exercise, perf now ships with an entire subtool for checking scheduling and latency: `perf sched`. It's quite similar to the above example but captures more data with more relevant tracepoints enabled and formats it in a more readible way (useful for extremely large systems). 
```bash
r7 # perf sched record -- stress-ng --cpu 2 --cpu-method hamming
stress-ng: info:  [14570] defaulting to a 86400 second run per stressor
stress-ng: info:  [14570] dispatching hogs: 2 cpu
^C[ perf record: Woken up 5 times to write data ]
stress-ng: info:  [14570] successful run completed in 38.92s
[ perf record: Captured and wrote 8.804 MB perf.data (80536 samples) ]
 r7 # perf sched script
             perf 14569 [001] 975758.720109: sched:sched_stat_runtime: comm=perf pid=14569 runtime=182986 [ns] vruntim
            perf 14569 [001] 975758.720111:       sched:sched_wakeup: perf:14570 [120] success=1 CPU:001
            perf 14569 [001] 975758.720113:       sched:sched_switch: perf:14569 [120] R ==> perf:14570 [120]
            perf 14570 [001] 975758.720220:       sched:sched_wakeup: migration/1:13 [0] success=1 CPU:001
            perf 14570 [001] 975758.720220: sched:sched_stat_runtime: comm=perf pid=14570 runtime=110849 [ns] vruntim
            perf 14570 [001] 975758.720221:       sched:sched_switch: perf:14570 [120] R ==> migration/1:13 [0]
     migration/1    13 [001] 975758.720223: sched:sched_migrate_task: comm=perf pid=14570 prio=120 orig_cpu=1 dest_cp
     migration/1    13 [001] 975758.720230:       sched:sched_switch: migration/1:13 [0] S ==> perf:14569 [120]
            perf 14569 [001] 975758.720271: sched:sched_stat_runtime: comm=perf pid=14569 runtime=42392 [ns] vruntime
            perf 14569 [001] 975758.720275:       sched:sched_switch: perf:14569 [120] S ==> swapper/1:0 [120]
         swapper     0 [000] 975758.720303:       sched:sched_switch: swapper/0:0 [120] R ==> perf:14570 [120]
       stress-ng 14570 [000] 975758.720854: sched:sched_stat_runtime: comm=stress-ng pid=14570 runtime=629416 [ns] vr
       stress-ng 14570 [000] 975758.720860:       sched:sched_wakeup: rcu_sched:9 [120] success=1 CPU:000
       stress-ng 14570 [000] 975758.720862: sched:sched_stat_runtime: comm=stress-ng pid=14570 runtime=6416 [ns] vrun
       stress-ng 14570 [000] 975758.720862:       sched:sched_switch: stress-ng:14570 [120] R ==> rcu_sched:9 [120]
       rcu_sched     9 [000] 975758.720864: sched:sched_stat_runtime: comm=rcu_sched pid=9 runtime=4581 [ns] vruntime
       rcu_sched     9 [000] 975758.720865:       sched:sched_switch: rcu_sched:9 [120] S ==> stress-ng:14570 [120]
       stress-ng 14570 [000] 975758.721854: sched:sched_stat_runtime: comm=stress-ng pid=14570 runtime=989175 [ns] vr
[...]
```
- This is quite a bit of data, but the `perf sched record` captured not only the same info as the `sched_switch` that we originally checked for, but also instances of when tasks move to another CPU (`sched_migrate_task`) and instances where we check for how long something has been running (`sched_stat_runtime`). In these, we can see which process we switch to and on which CPU along with a plethora of other scheduling information. 
- The same data can be fed to the other parts of the `perf sched` subtool. For example, we can check statistics on latency for processes: 
```bash
 r7 # perf sched latency | head

 -----------------------------------------------------------------------------------------------------------------
  Task                  |   Runtime ms  | Switches | Average delay ms | Maximum delay ms | Maximum delay at       |
 -----------------------------------------------------------------------------------------------------------------
  systemd:(32)          |      4.329 ms |       62 | avg:    1.338 ms | max:    3.360 ms | max at: 975759.402856 s
  khugepaged:37         |      0.043 ms |        4 | avg:    1.015 ms | max:    1.034 ms | max at: 975761.167889 s
  stress-ng:14570       |      4.835 ms |        8 | avg:    0.061 ms | max:    0.171 ms | max at: 975758.722948 s
  stress-ng-cpu:(2)     |  77817.511 ms |      415 | avg:    0.048 ms | max:    2.845 ms | max at: 975776.744779 s
  perf:14569            |      5.905 ms |        4 | avg:    0.031 ms | max:    0.117 ms | max at: 975758.720230 s
  sshd:8058             |      0.430 ms |        6 | avg:    0.018 ms | max:    0.078 ms | max at: 975797.645459 s 
```
- The above is the table indicating how long processes ran for in millisecond, the amount of times the process had to switch (IE give up a CPU to run on another CPU or let something else run on this CPU while it queued back up), the average amount of delay (time between wakeup and actually running, so think of it as time waiting int he CPU queues), the maximum delay encountered and the point in time which that occurred. 
- The subtool also provides a way of visualizing (within reason) the activity: 
```bash
 r7 # perf sched timehist -MVw | head
Samples do not have callchains.
           time    cpu  012  task name                       wait time  sch delay   run time
                             [tid/pid]                          (msec)     (msec)     (msec)
--------------- ------  ---  ------------------------------  ---------  ---------  ---------
  975758.720111 [0001]       perf[14569]                                                      awakened: perf[14570]
  975758.720113 [0001]   s   perf[14569]                         0.000      0.000      0.000
  975758.720220 [0001]       perf[14570]                                                      awakened: migration/1[13]
  975758.720221 [0001]   s   perf[14570]                         0.000      0.001      0.108
  975758.720223 [0001]    m    migration/1[13]                                                  migrated: perf[14570] cpu 1 => 0
  975758.720230 [0001]   s   migration/1[13]                     0.000      0.001      0.008
  975758.720275 [0001]   s   perf[14569]                         0.116      0.000      0.045
```
- Per line, we see 
  - the perf with tid 14569 running on CPU 1 when another perf thread, 14570 woke up to run on a CPU. 
  - Perf:14569 incurred a scheduling event (s) on CPU 1 and finished running (for less than a millisecond)
  - Perf:14570 began running on CPU 1 when a migration thread woke up (meaning it will soon move a process to another CPU)
  - Perf:14570 incurred a scheduling event (s) on CPU 1 and finished running after ~108 us
  - The migration thread on CPU 1 moved perf:14570 from CPU 1 to CPU 0
  - That same migration thread finished running after ~8 us
  - Perf:14569 ran and finished running after ~45 us
- The output is extremely dense and can be difficult to consume, but it is extremely handy in tracking down latency causes. Thankfully, we can limit the output to specific tids/pids if needed: 
```bash
 r7 # perf sched timehist -MVw --tid=14572,14571 | head
Samples do not have callchains.
           time    cpu  012  task name                       wait time  sch delay   run time
                             [tid/pid]                          (msec)     (msec)     (msec)
--------------- ------  ---  ------------------------------  ---------  ---------  ---------
  975758.725127 [0000]   m     stress-ng[14570]                                                 migrated: stress-ng[14571] cpu 0 => 1
  975758.725129 [0000]       stress-ng[14570]                                                 awakened: stress-ng[14571]
  975758.725252 [0000]   m     stress-ng[14570]                                                 migrated: stress-ng[14572] cpu 0 => 0
  975758.725254 [0000]       stress-ng[14570]                                                 awakened: stress-ng[14572]
  975758.725379 [0001]   s   stress-ng-cpu[14571]                0.000      0.057      0.192
  975758.725426 [0001]       swapper                                                          awakened: stress-ng-cpu[14571]
  975758.725461 [0000]  s    stress-ng-cpu[14572]                0.000      0.046      0.161
```
- The output is limited to only the threads actually doing the churn from stress-ng (`stress-ng-cpu`). What we see here mostly is stree-ng forking and creating it's children (thus multiple instances of those threads waking up and being migrated immediately to another CPU). 

## Resources 

- Tracepoints

  - `/usr/share/doc/kernel-doc-3.10.0/Documentation/trace/tracepoints.txt` Contains an overview of tracepoint information 

- http://www.brendangregg.com/perf.html
- http://www.brendangregg.com/blog/2017-03-16/perf-sched.html
