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
