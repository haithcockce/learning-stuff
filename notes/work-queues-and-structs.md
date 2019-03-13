# Work Queues, `work_struct`, and Deferred Work Handling

All about deferred work using work queues in the kernel

### Workqueues - An Overview

Workqueues allow a function to be called at some future time. This is inherently different from timers which deterministically(ish) fire off a function call at a specific time in the future (either relative to current time or at a specific time). Workqueues instead will fire the function off in the future, but non-deterministically.

### Structs

###### `workqueue_struct`

Worker thread representation.

Members:
- `struct cpu_workqueue_struct[NR_CPUS]` list of pointers to each CPU's workqueue. One per worker thread per processor.
- `struct list_head list`
- `const char *name`
- `int singlethread`
- `int freezeable`
- `int rt`

###### `cpu_workqueue_struct`

CPU workqueue with work to carry out

Members:
- `spinlock_t lock` protects this structure
- `struct list_head worklist` list of work to perform
- `wait_queue_head_t more_work` ? maybe the deferred work ?
- `struct work_struct *current_struct` current work being carried out
- `struct workqueue_struct *wq` The worker thread back pointer
- `task_t *thread` ? associated thread ? What is this? I guess the actual thread to carry out the action. Like `events/2 -> kernel_thread() -> kthread() -> worker_thread()`

### Functions

###### `create_workqueue()` and `create_singlethread_workqueue()`

Will create a workqueue and CPU worker thread(s) that carry out the work for that workqueue. The first will create a CPU worker thread for each CPU. The second will create only one worker thread associated with a specified CPU.

###### `DECLARE_WORK(name, void (*function)(void *), void *data);`

Create a `work_struct` at compile time.
- `name` is the name of the `work_struct`
- `(*function)` is the function to execute (aka callback function)
- `*data` is the parameters to the callback function

###### `INIT_WORK(struct work_struct *work, void (*function)(void *), void *data);` and `PREPARE_WORK(struct work_struct *work, void (*function)(void *), void *data);`

Create a `work_struct` at runtime. `PREPARE_WORK` doesn't do full initialization and is used more for changing already submitted work.
- `work_struct` is the structure to be filled in
- `(*function)` function to execute (aka callback function)
- `*data` is parameters for callback function

###### `int queue_work(struct workqueue_struct *queue, struct work_struct *work);` and `int queue_delayed_work(struct workqueue_struct *queue, struct work_struct *work, unsigned long delay);`

Both add `work` to the `queue` passed in, but `queue_delayed_work` will force a delay in execution of the callback function until after `delay` amount of jiffies.

###### `flush_work(struct work_struct *work)`

Forces the current process to block and wait until the `work_struct`'s callback function finishes. __NOTE__ This is called by something _other than_ the workqueue thread. So effectively the process will wait until the workqueue thread will run the `work_struct`

### References

`http://www.makelinux.net/ldd3/chp-7-sect-6`




perf record -a -g -- sleep 300








tuning should have been checked from the start.
let them know the parameters will take time to take effect and could even push the system into a more poorly performing state. The best way to do this is a reboot. Start from fresh state if possible.

timestampssssssssss
