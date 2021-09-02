# Introduction to strace

- [What Is Strace](#what-is-strace)
  - [Libraries](#libraries)
  - [Syscalls](#syscalls)
  - [Strace](#strace)
- [Common Use](#common-use)
  - [Key points with strace](#key-points-with-strace)

## What Is Strace

#### Libraries

- As programs execute, they call and return from functions to perform various operations.
- Most of these function calls are within userspace. Programs with more sophisticated and intricate operations (such as locking, process management, interacting with specialty hardware, etc) often use external sources of predefined functionality (_libraries_) to accomplish some specific task. Examples:
  - An application can create new processes and threads but must use function calls to the kernel like `fork()` or functionality provided from the `pthread` library.
  - Infiniband and RDMA-enabled hardware requires an application to be built with functionality provided from the libraries in the `libibverbs` package.
  - Python applications can import modules (conceptually similar to libraries) for nearly everything from advanced date and time management (`datetime` module) up through to sophisticated and elaborate artificial intelligence libraries (`sklearn`, `scipy`, `pandas`, `pyspark`, etc)

#### Syscalls

- Some functionality is provided explicitly from the kernel for abstraction and security reasons;
  - Allocating memory used to require applications to know what areas of memory are already mapped and in use in order to prevent memory corruption and a host of other issues. The kernel now abstracts that via the virtual/physical memory divide for security purposes as well as to allow applications to run without having to know what else is running on the system.
  - The kernel also (by default) automatically manages process execution and load balancing processes across CPUs. This prevents processes from starving other processes of CPU time. Processes may also explicitly indicate their end of execution and give up CPU time they may still have allocated to them via yielding the CPU.
  - Reading from and writing to storage devices used to require applications to know the size of blocks for the storage device, the cylinder on the disk it needs to interact with, etc. The kernel now manages all of this including the ordering of operations to interact with the device.
- _Syscalls_ (System Calls) provide the "API" function calls to access this functionality provided from the kernel.
  - Processes executing in such a way where they do not require the kernel typically means the process is executing in userspace. This usually means the applications are interacting with their own private bits of data and functions.
  - However, operations requiring interacting with the underlying system requires interacting with the kernel. Doing so causes the application to enter kernelspace.
  - Userspace is a dedicated area of execution for processes to perform operations within itself. Kernelspace is a dedicated area of execution for the kernel to carry out operations it provides.
  - Syscalls are the functions provided from the kernel which tell the kernel a process needs something done on its behalf (such as sending data over the network, creating new processes, allocating memory, etc).
  - Calling a syscall within a process temporarily suspends userspace execution for that process, causes the kernel to perform some action defined by the syscall, then return the results to the calling process, then the process resumes execution in userspace with the returned results of the syscall.

#### Strace

- When problems arise with an application or the system, strace can provide insight into the transitions to/from kernelspace.
  - Syscalls, like any function call, take parameters and return values. Strace can provide parameters and return values of syscalls within the execution of an application should these need to be inspected.
  - Strace can provide a timeline of execution should this need to be inspected as well.
- **Note** _strace provides data only for interactions with syscalls and nothing more_
  - While strace is indeed sophisticated, it does not provide data about what functions within the application or within the kernel are called.
  - Other tools exist for this functionality!
    - `ltrace` allows tracing functions taken within libraries
    - `perf` and `eBPF` allows stracing functions taken within the kernel
- Example output of strace tracing the command `echo 'Hellow world!'`:

  ```bash
   r8 # strace echo 'Hello world!'
  1 execve("/usr/bin/echo", ["echo", "Hello world!"], 0x7ffcdbe3cae0 /* 27 vars */) = 0
  2 brk(NULL)                               = 0x55f28314a000
  3 arch_prctl(0x3001 /* ARCH_??? */, 0x7ffe2650f640) = -1 EINVAL (Invalid argument)
  4 access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
  [...]
  101 fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0), ...}) = 0
  102 write(1, "Hello world!\n", 13)          = 13
  103 close(1)                                = 0
  104 close(2)                                = 0
  105 exit_group(0)                           = ?
  106 +++ exited with 0 +++
  ```

  - Line 1: The child process begins execution (via `execve` syscall) of the `echo` binary with the parameters `"echo"` and `"Hello world!"`. Strace also tells us 27 additional variables were provided to the `echo` binary but are hidden for legibility (though options exist to list all variables).
  - Line 2: The `echo` process creates space on its stack via `brk` syscall.
  - Line 3-100: Effectively, every execution of a new process with a binary requires the shell and possibly other entities to perform pre-execution operations. These operations often sanity check execution ahead of time and set the environment for execution.
    - Line 3: `arch_prctl` sets architecture-specific thread states for processes and threads. It returned `1` and set the errno to `EINVAL` meaning a parameter was invalid for `arch_prctl`.
    - Line 4: The shell checks to see if the preload library `ld.so.preload` exists in `/etc` via the `access` syscall. `access` returned an error (-1) and set errno to `ENOENT` indicating the preload library does not exist.
    - The rest of the lines until Line 100 perform a variety of operations such as setting locale, reading from libraries so we know how to actually "echo" and thus write to the terminal, protecting areas of memory to prevent security vulnerabilities during execution, etc.
    - These series of operations are common for nearly any application using glibc and posix compatible libraries and functions. Tracing applications running on even Windows and MacOS systems will likely produce similar function calls to create a safe execution environment, pass variables around as needed, set locale, grab functionality from a library to actually write to a terminal, etc.
  - Line 101: The `echo` binary calls `fstat` against the file descriptor `1`, commonly stdout in bash, and checks to see it is a character device, or in other words a terminal. `fstat` returns `0` commonly indicating success.
  - Line 102: `echo` finally writes the `"Hello world!"` string with an added newline character. It attempts to write `13` characters to the file descriptor `1`. It returns `13` meaning it successfully wrote `13` characters to `1`.
  - Line 103: `echo` now closes stdout.
  - Line 104: `echo` closes file descriptor `2`, commonly stderr within bash and posix-compatible applications.
  - Line 105: `echo` explicitly terminates with `exit_group` indicating it is finished, this process will not execute anything else, and its parent process can clean up after the `echo` child process. Note, this will always return `?` with strace on successful termination mostly because the process no longer exists for strace to collect a return value from `exit_group` explicitly, however, the return value is passed back up to the parent process (in this case strace).
  - Line 106: Strace catches the return value of `0` and indicates the `echo` child process terminated with noted return value.

## Common Use

- For customer cases, the following strace command is most commonly recommended (found via the strace article [How do I use strace to trace system calls made by a command?](https://access.redhat.com/articles/2483)):

  ```bash
  $ strace -fvttTyy -s 4096 -o /tmp/strace.txt [-e trace=SYSCALL] <-p PID|COMMAND>
  ```

  - `-f` causes child processes to be captured into the same file, separate processes are prepended with their PID number in the trace output
  - `-v` prints unabbreviated versions of environment, stat, and similar calls
  - `-tt` prints the start time of each line, including microseconds
  - `-T` shows the time between the beginning and end of the system call
  - `-yy` prints paths associated with file descriptors and sockets (available in strace 4.7 and above)
  - `-s 4096` prints the first 4096 characters of any strings, the default value of 32 often causes useful information to be missed
  - `-o /tmp/strace.txt` outputs the trace to a text file for retrospective analysis, as the output is often large with long lines which makes live analysis difficult to impossible
  - `-e trace=SYSCALL` optional flag to trace specific syscalls, e.g. `-e trace=open,close,read,write`. The default is to trace all syscalls. Only use this option when you have proven out specific syscalls need to be traced.
  - `<-p PID|COMMAND>` indicates strace should start tracing the noted pid (`-p PID`) or should trace the command (`COMMAND`).

- Example output from the above using the same `echo 'Hello world!'` command as above;

  ```bash
   r8 # strace -fvttTyy -s 4096 -- echo 'Hello world!'
  1 6754  14:21:22.304481 execve("/usr/bin/echo", ["echo", "Hello world!"], ["LS_COLORS=rs=0:di=38;5;33[...]in/strace"]) = 0 <0.000350>
  2 6754  14:21:22.304990 brk(NULL)         = 0x564023ce9000 <0.000011>
  3 6754  14:21:22.305036 arch_prctl(0x3001 /* ARCH_??? */, 0x7fff28f41890) = -1 EINVAL (Invalid argument) <0.000010>
  [...]
  101 6754  14:21:22.308819 fstat(1</dev/pts/0<char 136:0>>, {st_dev=makedev(0, 0x17), st_ino=3, st_mode=S_IFCHR|0620, st_nlink=1, st_uid=0, st_    gid=5, st_blksize=1024, st_blocks=0, st_rdev=makedev(0x88, 0), st_atime=1630347682 /* 2021-08-30T14:21:22.539725106-0400 */, st_atime_nsec    =539725106, st_mtime=1630347682 /* 2021-08-30T14:21:22.539725106-0400 */, st_mtime_nsec=539725106, st_ctime=1630345045 /* 2021-08-30T13:37    :25.539725106-0400 */, st_ctime_nsec=539725106}) = 0 <0.000014>
  102 6754  14:21:22.308914 write(1</dev/pts/0<char 136:0>>, "Hello world!\n", 13) = 13 <0.000055>
  103 6754  14:21:22.308993 close(1</dev/pts/0<char 136:0>>) = 0 <0.000005>
  104 6754  14:21:22.309019 close(2</dev/pts/0<char 136:0>>) = 0 <0.000013>
  105 6754  14:21:22.309064 exit_group(0)     = ?
  106 6754  14:21:22.309129 +++ exited with 0 +++
  ```

  - Line 1: Strace produces a full listing of all parameters passed to `execve`. This listing is extremely long, so I manually truncated it myself for legibility. The `execve` call was successful (as the return value is `0`) and calling `execve` took 350 microseconds (`0.000350` seconds). The pid for this process is `6754` and the execution of `execve` began at `14:21:22.304481`
  - All the same content from the first example is provided, but the options enabled a variety of additional datum to be captured as observed in the above example.

#### Key points with strace

- Some key points in the kinds of data collected with the recommended strace options;
  - Strace provides data about which calls are taken to enter kernelspace along with their parameters and return values. As such, you will likely need to search for the definitions of the functions and what the parameters represent as well as their return values. 
  - Strace still can not trace functions taken exclusively in userspace or kernelspace. However, the timestamps can provide insight into how long a syscall took to execute to completion _and_ the time spent in userspace (or off the CPU) between executions of syscalls.
    - In the second example, the time spent between syscalls can be calculated by adding the start time in the third column to the time to completion in the last column. Then calculating the delta between that sum and the start time of the next syscall execution start time.
    - For example, the time between the end of the `execve` syscall and the beginning of the `brk` syscall is `14:21:22.304990 - (14:21:22.304481 + 0.000350) = 0.000159` or 159 microseconds. The 159us could have been spent in userspace, could include an interrupt for the kernel to do something, etc.
  - The timestamps and syscalls taken can enable inferring behavior within the application.
    - Reviewing the syscalls taken in a particular sequence, their parameters, and their return values can provide insight into what the application may be doing. For example, strace may show an application using the `select` syscall repeatedly ad nauseum against a few file descriptors. Such output would implicate the application is constantly checking for activity of some kind on some files.
    - Another example could include noting a child thread begins to hang within a syscall and never returns. Alternatively, a child thread could continue execution in userspace and never call another syscall until termination.
  - **Note** Strace provides data only for the syscalls taken. Even though application behavior can be inferred from the syscalls taken, _this tells you nothing of the expected behavior_. As such, if an strace is taken of a third party application, at most, Red Hat can comment on the strace output and make evidenced assumptions on observed behavior. _The application vendor is the only one responsible for noting anything about expected behavior_.

## Exercises

#### My application is hanging! 

- A customer notes their application is hanging. While this is largely still in the territory of the application vendor needing to dig in, we can, at minimum, gather an strace to determine if the application is indeed very clearly hanging in any particular spot. 
- **Note** _If this is the only information you have, inform the customer that we are extremely unlikely to be able to determine where the hang is if we do not ship the application. The strace will ultimately need to be reviewed by the application vendor and the customer must understand we can not guarantee any strong degree of confidence in any conclusions drawn from the strace until the vendor has reviewed the strace output or identified where the hang is occurring._
- The customer provides [some strace data](https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/data/strace/example1.trace) and confirmed the trace was captured at the time of the hang.

##### Assignment

- Review the trace and provide elaboration on what is observed.
  - Where is the hang occurring if at all? 
  - How could the kernel/OS be implecated?

#### My application is hanging on the `poll()` function!

- A customer notes their application is hanging and believes the application to be hanging on a call to the `poll()` function. They note the call to `poll()` is looking for input to the file being polled but does not know what file is being polled. 
- The customer provides [some strace data](https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/data/strace/example2.trace) and confirmed the trace was captured at the time of the hang.

##### Assignment

- Review the trace and provide elaboration on what is observed.
  - Where is the hang occurring if at all? 
  - How could the kernel/OS be implecated?

#### My application hangs/is slow

- A specific syscall takes more time (find nsss/sssd case if possible)

- Nearly all syscalls take more time (lower cpu speed manually)
