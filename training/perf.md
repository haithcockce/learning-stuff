# Perf

## Tracing hardware and kernel events 

#### Intro

- Low-level, low latency tracing and sampling of activity at the kernel and hardware level
- Can get statistics on very specific things such as CPU Cache misses, branch mispredictions, etc
- Can trace as well against specific pre-defined spots in the kernel or in custom spot in the kernel

###### How

- The kernel exposes a variety of subsystems via the pseudofilesystem, `debugfs`. 
  - _Performance Counters for Linux_ CPUs have architecture specific registers which increment based on certain events occurring. 
    - AKA `perf_events` 
    - Think count of context switches.
  - _Tracepoints_ predefined hooks in kernel code to call a function. 
    - _Probe_ The function called by the hook 
    - If a tracepoint is "on", then a probe is enabled. "Off" means the probe is disabled.
  - _Events_ dynamic and custom defined tracepoints. 
    - Custom defined functions can be called
    - Uses kprobe (kernelspace probing). Uprobes (userspace probing) exists as well
    - Think systemtap. 

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
- Perf deals with all of this!

#### Events/Tracepoints/Counters in Perf

- `perf list` lists all tracepoints and counters possible. 
- `perf probe` can add/list/remove dynamic events at specific points in a function, lines in source code, or elsewhere 

## Resources 

- Tracepoints

  - `/usr/share/doc/kernel-doc-3.10.0/Documentation/trace/tracepoints.txt` Contains an overview of tracepoint information 

- http://www.brendangregg.com/perf.html
- http://www.brendangregg.com/blog/2017-03-16/perf-sched.html
