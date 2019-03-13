# Read/Write Semaphore

[From here](https://0xax.gitbooks.io/linux-insides/content/SyncPrim/linux-sync-5.html):

- `0x0000000000000000` - reader/writer semaphore is in unlocked state and no one is waiting for a lock;
- `0x000000000000000X` - X readers are active or attempting to acquire a lock and no writer waiting;
- `0xffffffff0000000X` - may represent different cases. The first is - X readers are active or attempting to acquire a lock with waiters for the lock. The second is - one writer attempting a lock, no waiters for the lock. And the last - one writer is active and no waiters for the lock;
- `0xffffffff00000001` - may represent two different cases. The first is - one reader is active or attempting to acquire a lock and exist waiters for the lock. The second case is one writer is active or attempting to acquire a lock and no waiters for the lock;
- `0xffffffff00000000` - represents situation when there are readers or writers are queued, but no one is active or is in the process of acquire of a lock;
- `0xfffffffe00000001` - a writer is active or attempting to acquire a lock and waiters are in queue.

#### Waiters

- Waiters for a semaphore are managed via a `rwsem_waiter` struct and contains a task_struct pointer and flags (RHEL 6) or type (RHEL 7) indicating the type of waiting

