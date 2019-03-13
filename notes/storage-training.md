# Storage Training with DJeffrey

## Being betrayed by crash

- `dev -d` Unreliable. Can report false pasitives in multi-queue devices and 0 requests for scsi layer stuff
- `kmem` slab caches are merged in RHEL 7, so objects can come from weird named caches 

## Address Ranges

### Linear Map 

- `0xffff880000000000`
- Physical = Address - `0xffff880000000000`
- Typical contiguous page allocations 

### Codespace 

- `0xffffffff00000000`
- Kernel image and module addresses and code 

### Percpu

- `0x#####` 
- `kmem -o` lists offsets

## Vmcores

### First vmcore

- Init is killed and usually because backing storage for / is dead (either removed or "gave up")
- `dev -d | grep 'sda '` to check the request queue 
- `struct request_queue.queuedata 0xffff88be7a708000` produced `queuedata = 0xffff8860ae6a1000`
- `p ((struct scsi_device *)0xffff8860ae6a1000)->sdev_state`

### scsi show stuff 

- In epython  
- `scsishow --check`
- `scsishow --time` check for timed out commands
- `scsi_eh_#` is error handler 
- `scsi_cmnd->device->host->host_failed and scsi_cmnd->device->host->host_busy` both = 1 causes scsi error handler to kick off and work

### ioctl stuff

- When doing an `ioctl`, the file descriptor number is often rdi when entering kernelspace 

### Meltdown

- `__x86_indirect_thunk_rax` is a trampoline function for spectre/meltdown

### BIO

- `bio.bd_dev` is a hex value of device mapper number