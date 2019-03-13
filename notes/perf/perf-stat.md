# Perf Stat 

Notes on what `perf stat` does and what the output means

### Output

Example command and output: 

```
# perf stat -r 5 -B dd if=/dev/urandom of=/dev/null bs=1 count=50

 Performance counter stats for 'dd if=/dev/urandom of=/dev/null bs=1 count=50' (5 runs):

          1.879704      task-clock (msec)         #    0.639 CPUs utilized            ( +-  6.16% )
                 2      context-switches          #    0.001 M/sec                    ( +- 10.21% )
                 0      cpu-migrations            #    0.000 K/sec                  
               226      page-faults               #    0.120 M/sec                    ( +-  0.11% )
   <not supported>      cycles                   
   <not supported>      stalled-cycles-frontend  
   <not supported>      stalled-cycles-backend   
   <not supported>      instructions             
   <not supported>      branches                 
   <not supported>      branch-misses            

       0.002941782 seconds time elapsed                                          ( +-  6.79% )
```

- __task-clock__ 
  - __CPUs utilized__ The "parallelism" of the process. Will show how many CPUs were used to 
    execute the task. Over 1 CPU means migrations or forking while less 1 means either all forks
    completed at a very short time or only one thread ran on only one cpu. 
  - __( +- 6.16% )__ The standard deviation
- __context-switches__ Count of context switches or the times the CPU had to switch processes or
  jump to/from kernelspace/userspace
  - __M/sec__ Millions of context switches/second 
  - __( +- 10.21% )__ Standard deviation of cs/s
- __cpu-migrations__ Times the process was resecheduled from one CPU to another
  - __0.000 K/sec__ Thousands of migrations/second
- __page-faults__ Times the MMU had to perform either a major or minor fault to satisfy a request
  - __0.120 M/sec__ Millions of page faults/second
  - __( +-  0.11% )__ Standard Deviation of pf/s
- __cycles__ Count of special hardware events. Due to ambiguity, I interpret this to mean 
  specifically clock cycles, IE the times a CPU oscillator pulses.
  - The above is assumed because, if monitoring instructions and cycles, instructions/cycle is
    calculated automatically in the output. An instruction cycle is the life of an entire 
    instruction starting from fetch/pre-fetch all the way to execution and so on which is 
    separate from a clock cycle. 
  - If supported and measured, __GHz__ is displayed as well indicating something
- __stalled-cycles-frontend__ when pipelining, the frontend does fetching, branch prediction, and
  decode. This is done in serial. 
  - __
