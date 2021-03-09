# Collectl Cheatsheet

## Installation 

**Note** Collectl is _not_ available for RHEL 8

- RHEL 5: `yum install https://archives.fedoraproject.org/pub/archive/epel/5/x86_64/collectl-4.0.2-1.el5.noarch.rpm -y`
- RHEL 6: `yum install https://archives.fedoraproject.org/pub/archive/epel/6/x86_64/Packages/c/collectl-4.0.2-1.el6.noarch.rpm -y`
- RHEL 7: `yum install https://archives.fedoraproject.org/pub/archive/epel/7/x86_64/Packages/c/collectl-4.3.0-5.el7.noarch.rpm -y`

Start and enable the service

- RHEL 5/6: `chkconfig collectl on && service collectl start`
- RHEL 7: `systemctl enable collectl && systemctl start collectl`

## Configs

- Collectl has a single config file in `/etc/collectl.conf`
- By default, it collects various metrics at 10, 60, and 120 second intervals as noted by `Interval`, `Interval2`, and `Interval3` config lines in the config file. These can be uncommented and changed (and a restart to collectl) to increase or decrease frequency of data collection. 

```bash
# grep -i interval /etc/collectl.conf 
#Interval =     10
#Interval2 =    60
#Interval3 =   120
```

- By default, data is written to `/var/log/collectl/<HOSTNAME>-<TIMESTAMP>.raw.gz` archive files where `<TIMESTAMP>` is the time and date the archive begins.

## Reading Archive Files

- The typical command is as follows:

    # collectl -o <D|T> -s <SUBSYS> --verbose -p <archive> [--from <TIMESTAMP>] [--thru <TIMESTAMP>]
  
  - `-o <D|T>` provides either date and time or just timestamps to samples, respectively. When looking through several days of data, `-o D` is preferred while `-o T` will suffice when looking at a single day.
  - `-s <SUBSYS>` denotes which metrics (subsystems) you want to view. More to be discussed later.
  - `-p <archive>` denotes reading collectl metrics from the archive file provided.
  - Optionally, `--from <TIMESTAMP>` and `--thru <TIMESTAMP>` can be provided to tell collectl to skip printing metrics before the `--from` timestamp and after `--thru` timestamp. `<TIMESTAMP>` is of the form `HH:MM:SS`.

## Subsystems

| Brief | Description | Detailed | Description |
| --- | --- | --- | --- |
| b\* | buddy info (memory fragmentation) | E | environmental (fan, power, temp) but requires ipmitool |
| c\* | CPU | C\* | individual CPUs, including interrupts if `-s j` or `-s J` is set |
| d\* | disk | D\* | individual disks |
| f\* | NFS | F\* | NFS |
| i\* | inodes | Z\* | Process |
| j | interrupts by CPU | J | interrupts by CPU by interrupt number |
| l | lustre | L | lustre |
| m\* | memory | M\* | Memory and NUMA node stats |
| n\* | Network | N\* | Network |
| s\* | sockets | | |
| t\* | TCP | T\* | TCP | 
| x | Interconnect | X | interconnect ports/rails (Infiniband/Quadrics) |
| y\* | Slabs (system object caches) | Y\* | slabs/slubs |

- Options come in "brief", "verbose", and "detail" flavors. 
  - Brief is a quick overview of a subsystems metrics. For example, CPU info here is limited to %sys, interrupts/s, and context switches/s and total active time over that interval as a percentage. Percentages here are averaged across all CPUs. Options designated as "brief" are lowercase letters (IE `-s cdm`)
  - Verbose is detailed summary data. For example, CPU info here is quite detailed but still averaged across all CPUs. This requires `--verbose` flag in tandem with "brief" options (IE `-s cdm --verbose`) 
  - Detailed is per-source unit statistics, such as per-cpu stats or per-disk stats. Using `--verbose` flag doesn't change this behavior. Detailed options are uppercase (IE `-s CDM`).

#### `--top`

- Some subsystems can be sorted based on columns with the `--top` option. 
  - For example, the per-process subsystem, `-s Z` can be sorted based time spent in userspace processing (`--top usrt`) and kernelspace processing (`--top syst`). 
- Check `collectl --showtopopts` for a full listing of what can be sorted.

## Examples

```bash
# collectl -o T -s c --verbose -p my-archive.raw.gz
```

This will print verbose cpu statistics from `my-archive.raw.gz` with timestamps.

```bash
# collectl -o D -s Z --top syst -p my-archive.raw.gz
``` 

Prints the top 5 consumers of kernelspace processing 

## Archives

- You can use `less` against an archive to see the header of the archive for metadata about the archive. For example: 

```bash
 r7 # less /var/log/collectl/r7-20210309-175808.raw.gz 
################################################################################
# Collectl:   V4.3.0-1  HiRes: 1  Options: -D /etc/collectl.conf 
# Host:       r7  DaemonOpts: -f /var/log/collectl -r00:00,7 -m -F60 -s+YZ
# Booted:     1614620413.23 [20210301-12:40:13]
# Distro:     Red Hat Enterprise Linux Server release 7.9 (Maipo)  Platform: Standard PC (Q35 + ICH9, 2009)
# Date:       20210309-175808  Secs: 1615330688 TZ: -0500
# SubSys:     bcdfijmnstYZ Options:  Interval: 10:60 NumCPUs: 2  NumBud: 2 Flags: ix
# Filters:    NfsFilt:  EnvFilt:  TcpFilt: ituc
# HZ:         100  Arch: x86_64-linux-thread-multi PageSize: 4096
# Cpu:        GenuineIntel Speed(MHz): 2592.004 Cores: 1  Siblings: 1 Nodes: 1
# Kernel:     3.10.0-1160.15.2.rt56.1152.el7.x86_64  Memory: 1880444 kB  Swap: 2097148 kB
# NumDisks:   3 DiskNames: vda dm-0 dm-1
# NumNets:    2 NetNames: eth0:?? lo:??
# NumSlabs:   103 Version: 2.1
# SCSI:       CD:0:00:00:00
################################################################################
>>> 1615330740.003 <<<
```

- **Note** Collectl archives can be converted to PCP archives. However, I could not find a builtin pmrep view that worked with the default collectl configs and only some of the pcp modules worked against the converted archive, like `pcp vmstat` and `pcp iostat`.

## Sources

- [Collectl Documentation](http://collectl.sourceforge.net/Documentation.html)
- [How to use the collectl utility to troubleshoot performance issues in Red Hat Enterprise Linux](https://access.redhat.com/articles/351143)
