# RWLOCK_T

These are simple, thankfully:

- The lock word is initialized to `0x1000000` and thus unlocked
- A reader always decrements 
- A writer will subtract away the original value.
  - Thus if a writer takes the lock, `lock = 0x0` 
  - And readers behind it will decrement it negative
  - Otherwise, the lock word will still be a positive number
  - When positive, the lock word will look negative. Check the amount of `f`s
