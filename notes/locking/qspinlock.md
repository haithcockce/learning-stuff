### Overview

- RHEL 6 and RHEL 7-7.3 have ticketed spin locks (now serving and "your" ticket
  - This was to overcome cache locality creating unfair spinlock access
- RHEL 7.4 has queued spin locks
- The structure is now `qspinlock` but is still referenced as `spinlock_t` for compatibility


### Lockword

- With ticketed spinlocks, the lock word provides the "now serving" and "your ticket" number
- The lockword differs depending on amount of present CPUs
- `NR_CPUS` <= 8192:
  - 0-7: locked byte
  - 8: Pending 
  - 9-15: unused
  - 16-18: tail index (index * 2 + 1 = queue entry 
  - 19-31: tail cpu
- `NR_CPUS` > 8192
  - 0-7: locked byte
  - 8: pending
  - 9-11: tail index (index * 2 + 1 = queue entry)
  - 12-31: tail cpu

```c
static inline __pure u32 encode_tail(int cpu, int idx)
{
	u32 tail;

	tail  = (cpu + 1) << _Q_TAIL_CPU_OFFSET;
	tail |= idx << _Q_TAIL_IDX_OFFSET; /* assume < 4 */

	return tail;
}

static inline __pure struct mcs_spinlock *decode_tail(u32 tail)
{
	int cpu = (tail >> _Q_TAIL_CPU_OFFSET) - 1;
	int idx = (tail &  _Q_TAIL_IDX_MASK) >> _Q_TAIL_IDX_OFFSET;

	return per_cpu_ptr(&qnodes[idx].mcs, cpu);
}
```


`include/asm-generic/qspinlock_types.h`
`kernel/locking/qspinlock.c`

static inline __pure struct mcs_spinlock *decode_tail(u32 tail)
{
        int cpu = (tail >> _Q_TAIL_CPU_OFFSET) - 1;
        int idx = (tail &  _Q_TAIL_IDX_MASK) >> _Q_TAIL_IDX_OFFSET;

        return per_cpu_ptr(&qnodes[idx].mcs, cpu);
}

