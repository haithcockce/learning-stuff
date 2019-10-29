- `gs:0x####` means we will work with per-cpu stuff. The per-cpu areas start with the addresses 
  listed in `help -m` in the ibase section 
- Stacks are read from bottom-to-top, right-to-left. 
   - As such, the addresses decrease going from right-to-left and 
     increase left-to-right
   - This happens since addressing starts in high memory (0xffffffff...) 
     and grows towards low mem (0x0000... and userspace)
