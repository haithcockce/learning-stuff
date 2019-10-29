# Workqueues

All about deferred work using workqueues in the kernel

# TODO

- Research RHEL 6 global workqueue

### Workqueues - An Overview

- Workqueues allow something to execute in the future.
- Differs from timers/softirqs/tasklets in that the aforementioned run in the
  context of an interrupt while workqueue activity runs in the context of a
  process. Thus timing of execution of actions is managed by either interrupt
  handlers vs. the process scheduler.
- A large number of improvements went in between RHEL 6 and 7. This is discussed
  below where appropriate

##### Entities

- _Workqueue_ The list of stuff to do and where to queue new stuff to.
  Conceptually similar to a "Todo" list.  
- _Work_ The stuff to execute. For the "Todo" list above, this could be "Do
  laundry", "Clean room", "Grocery shopping", etc. A single unit of "work" is
  referred to as a _work item_
- _Worker_ The process responsible for executing the queued work for a workqueue.
  For the "Todo" list example, the "worker" would be you (no one else will clean
  your nasty room unless you pay them).
- _Worker Pool_ Specific to RHEL 7 and above, the entity which actually
  organizes who works which work item. More on this later.

##### Conceptual Workflow

```
[1] Create workqueue
.----------.                      .-------------.
| thread A |--(create queue Q)--> | workqueue Q |
'----------'                      | [-,-,-,...] |
                                  '-------------'
[2] Make a work item to queue
.----------.                          .--------.
| thread A |--(create work item W)--> | work W |
'----------'                          '--------'
[3] Queue the work item
.----------.                      .-------------.
| thread A |--(queue work Q,W)--> | Q [W,-,-,-] |
'----------'                      '-------------'
[4] A worker processes the work
.----------.                 .---------------.
| worker B |--(process Q)--> | worker B, [W] |
'----------'                 '---------------'
```

- `[1]` Create workqueue:
  - RHEL 6: `create_workqueue([vars])`
  - RHEL 7: `alloc_workqueue([vars])`
- `[2]` Make a work item to queue
  - At compile time: `DECLARE_WORK([vars])`
  - At run time: `INIT_WORK([vars])` (required), `PREPARE_WORK([vars])`
- `[3]` Queue the work item to be worked on at some point in time later
  - `queue_work([vars])`, `queue_delayed_work([vars])`
- `[4]` The the workers will carry out the work
- Some additional actions:
  - Thread A wait until all work items in workqueue Q are processed:
  `flush_work([vars])`
  - Remove a work item from its queue: `cancel_work([vars])`
  - Destroy a workqueue: `destroy_workqueue()`

##### Implementation

The workqueue infrastructure received an implementation overhaul for scalability
reasons.

###### RHEL 6

- Upon workqueue creation, a workqueue and corresponding kernel thread is
  created for the workqueue. The threads are called `events`.
- A workqueue can be singlethreaded or multithreaded.
  - When singlethreaded, the first possible CPU is used (typically CPU 0) and
    contains its own workqueue and worker thread.
  - When multithreaded, each CPU gets its own workqueue and worker thread all
    with the same name. For example, on boot, all CPUs get an `events/#` thread.
- When queueing work, the work item can be queued arbitrarily or to a specific
  workqueue (and thus run on that workqueue's CPU)
  - When scheduling arbitrarily, `schedule_work(work)` puts the work in a
    global work queue (these seem to be similar to the multithreaded ones). I
    believe any sleeping worker thread will awake to handle this but not sure.
  - When scheduling arbitrarily with `queue_work(work)` the item is queued to
    the current CPU's workqueue
  - A specific workqueue (thus CPU) can be designated with `queue_work_on(work)`
- When there is work to do, during the next scheduling tick, the corresponding
  worker thread will wake up and handle its respective workqueue

###### RHEL 7

- Work items are still "queued" but are thrown into a pool of work wherein a
  bunch of workers assigned to pool will work the work items.
  - Worker pools come in two flavors, normal and high priority where high prio
    work runs before normal work.
  - Each CPU has a normal and high priority worker pool.
  - These normal and high-priority worker pools are "bound"
  - Several worker pools are dynamically made as unbound.
  - Work queued to bound workqueues (IE bound worker pools) means the work needs
    to run on that corresponding CPU
  - Work queued to unbound workqueues (IE unbound worker pools) can work on any
    CPU within it's NUMA node  
- Worker threads here are called `kworker`

### Troubleshooting

_**Help! events/kworker is chewing CPU time!**_

- Perf tracing will tell us what we need.

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

- `http://www.makelinux.net/ldd3/chp-7-sect-6`
- https://www.kernel.org/doc/html/v4.14/core-api/workqueue.html
- https://0xax.gitbooks.io/linux-insides/content/Interrupts/linux-interrupts-9.html
- https://lwn.net/Articles/11360/
- https://kukuruku.co/post/multitasking-in-the-linux-kernel-workqueues/
