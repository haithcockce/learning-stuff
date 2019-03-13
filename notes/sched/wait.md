# Wait Stuff

#### `wait_event(wq, condition)`

- Sleep until condition is true
- Go into `TASK_UNINTERRUPTIBLE` until `@condition` is
  true which is checked everytime the waitqueue `@wq`
  is woken up.
- We first check if `@condition` is true and call 
  `__wait_event` if not. There, we prep addition to the
  `@wq` and get added to it. If `@condition` is true,
  then break out of the loop of checking and waiting. 
  This makes us fall out of the loop to `finish_wait` 
  which puts us back to running and remove wait stuff. 
- Wake up happens by someone signalling the wait queue
  which causes a wake up to all processes. These will 
  just check if `@condition` is true and act accordingly

