```
in rhel7, the current unix epoch is stored in a global struct called timekeeper, which is a timekeeper struct. the xtime_sec member of this struct is the epoch.

from a working core of the same kernel version (use /cores/crashext/bin/find-kernel.sh), get the offset of this member.

crash> struct timekeeper.xtime_sec -o
struct timekeeper {
  [0x38] u64 xtime_sec;
}

from the broken core, get the address of the global timekeeper struct, add the offset, and read the value there.

crash> sym timekeeper
ffffffff81db6de0 (b) timekeeper

crash> eval ffffffff81db6de0 + 0x38
hexadecimal: ffffffff81db6e18  

crash> rd ffffffff81db6e18
ffffffff81db6e18:  00000000591dd225                    %..Y....

convert from epoch to human readable:

[jsiddle@corvus ~]$ echo $((0x591dd225))
1495126565

[jsiddle@corvus ~]$ date --date='@1495126565'
Thu May 18 12:56:05 EDT 2017

server was rebooted on Thu May 18 12:56:05 EDT 2017

```
