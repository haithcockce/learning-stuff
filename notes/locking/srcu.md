# Sleepable RCU 

- Means Read Copy Update
- Updates are done only after a `synchronize_<blah>(*srcu)` is performed.
  - These block until all relevent read side operations finish within the srcu struct 
  - Read side actions are done with `rcu_read_lock()` and `rcu_read_unlock()`
  - `synchronize_sched()` will block until _all_ grace periods complete, IE `rcu_read_unlock` is called and `preempt_disable()` completes
- A grace period is one "completed" value in the `rcu->completed` field. 
