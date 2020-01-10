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

##### Interacting with `debugfs`

- Most tracing can be enabled specifically by echoing values into `debugfs` entries. For example: 

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

#### Events/Tracepoints/Counters in Perf

- `perf list` lists all tracepoints and counters possible. 
- `perf probe` can add/list/remove dynamic events at specific points in a function, lines in source code, or elsewhere 

## Resources 

- Tracepoints

  - `/usr/share/doc/kernel-doc-3.10.0/Documentation/trace/tracepoints.txt` Contains an overview of tracepoint information 

- http://www.brendangregg.com/perf.html
- http://www.brendangregg.com/blog/2017-03-16/perf-sched.html
