# Coschedule

### Overview

- Interleave application stack activity and OS management activity in a way that
  maximizes application stack execution across all relevant CPUs concurrently so
  bulk synchronous activity is not delayed.
- Ideally, batch OS management activity in larger and regular time slices in
  lock step so application stack can have free reign of relevant CPUs for bulk
  synchronous activity
- Execution occurs by the bulk paralleled activity occurring at times [x:x+t]
  across all CPUs, management activity occurs at times [x+t:x+t+n]:n << t across
  all CPUs so no bulk synchronous activity is occurring on any CPU, then bulk
  synchronous activity is occurring at times [x+t+n:x+2n+t] and so on.

### Why?

- Reduce the impact of OS noise (interrupts and daemon scheduling)
- Massively paralleled workloads require synchronization across many cores. A
  delay in one core (due to an interrupt for network activity for example)
  causes a pause across all cores while the app waits for the non-app activity
  to finish.
- Noise across all CPUs at random times means the app frequently has to wait for
  all CPUs in question to be available and free from OS activity to progress.
- Differs from what is observed now where OS management activity is bound to a
  subset of CPUs and the rest of the CPUs handle the bulk synchronous work.
  - The difference is the management and paralleled work is colocated on the
    same CPUs rather than being partitioned across separate CPUs.

### References

https://www.researchgate.net/publication/228566683_Linux_kernel_co-scheduling_for_bulk_synchronous_parallel_applications
