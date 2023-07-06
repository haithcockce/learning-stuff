# Performance Co-Pilot (PCP) Primer
- Compaion slide deck found [here](https://github.com/haithcockce/learning-stuff/blob/master/docs/pcp/PCP%20Training.pdf).
## Terminology and Concepts
The metrics themselves are described below first. Many of the concepts and entities in PCP can be thought of as "internal" and "external" where "external" entities are ones where a user may interact with directly, while "internal" entities are ones a user does not typically interact with directly but are used by other PCP components. Below, the internal and external components are described after the performance metrics are described.

### Metrics, Metric Naming, and Metric Organization

- **Domain** A logical grouping of metrics typically based on source such as NFS Client, Nginx, Windows, Linux (kernel), etc
- **PMNS** Performance Metrics Namespace is the method by which metrics are organized hierarchically into tree-like structures. Levels in the trees define the relationships between metrics within those subtrees. For example, paging activity would be a child of memory in general while page scanning and major faults would be children of paging activity. Typically, a PMNS is a single tree of metrics wherein a system can (and often will) have more than one PMNS available to use.
- The name of a performance metric is the path down through the PMNS delineated by '.' where the left-most parts of the metric name are the top levels of the hierarchy and the right-most parts are the bottom levels of the hierarchy.
- _Example_ `network.interface.in.bytes`
  - The metric is part of the network namespace.
  - The metric is part of a group of metrics describing network interface metrics.
  - The metric is part of a group of metrics describing ingress traffic over network interfaces.
  - The metric describes network ingress traffic over interfaces in terms of bytes.

- _Example_ `mem.util.swapFree`
  - The metric is part of the memory namespace.
  - The metric is part of a group of metrics describing memory utilization metrics.
  - The metric describes the amount of free space in the system's swap spaces and swapfiles.
- **Instances** are literal sources of a metric such as the cumulative amount of swap space on the system or the currently running kernel version.
- **Instance Domains** are collections of literal sources of metrics such as NICs, CPUs, disks, all PIDs, etc
- _Example_
  ```
   r7 # pminfo -f network.interface.in.bytes
  
  network.interface.in.bytes                   ← Metric
      inst [0 or "eth0"] value 15853434        ← Instance <----- Instance Domain
      inst [1 or "lo"] value 18080             ← Instance <-/
  ```
  
### Internal Architectural Components

- **PMID** Performance Metric IDentifier. A unique value mapped to a specific metric. Typically you don't interact with this.
- **PMDA** Performance Metric Domain Agent. A PMDA sits between other PCP components and the system-specific sources of metrics to retrieve metrics and convert them in a way consumable for PCP and other PCP components. Thus, metrics could come from a Mac OSX kernel (darwin), Linux kernel, or Windows kernels, as well as from applications such as Nginx, Informatica, Apache, etc.
- **PMCD** Performance Metric Collector Daemon. The PMCD is the core internal component responsible for collecting metrics from PMDAs when requested by other components. PMCD must be running in order to gather metrics on a live system or record them into logs.

### External Architectural Components

- `pminfo` Displays metrics in use, metadata and information about metrics themselves, as well as where the metric values are being pulled from. Can also present values of metrics.
- `pmval` Dumps arbitrary metric values for a metric. Useful when needing to inspect a specific metric quickly.
- `pmlogger` Daemon responsible for collecting and logging metrics for asynchronous metric analysis into archive files.
- `pmrep` Performance metrics reporter. A highly customizable tool to present metrics nearly however desired. Most analysis with PCP data is done with pmrep.
- `pmdumplog` Dumps information about archives created with pmlogger including what metrics were collected, what the instances and instance domains were, what times does the archive span, etc.

### Visual Representation

Below is a visual representation of how all the core components work together.

![PCP Architecture Diagram](https://github.com/haithcockce/learning-stuff/blob/master/docs/pcp/pcp-architecure.png?raw=true)

## Common Usage in Support

### Archives

- Metrics can be logged with pmlogger into archives, similar to SAR’s sa files and `*.raw.gz` collectl logs
- Archives are stored in `/var/log/pcp/pmlogger/<HOSTNAME>/` by default
- Archives are named by the date the capture started and split into three files
  - Data files, ending in .# (.0, .1, etc), contain the raw metric data
  - Index files, ending in `.index`, and is a temporal index for data files to allow rapid access of data points
  - Metadata files, ending in `.meta`, describe instance domains, metrics, etc captured, the timezone the data comes from, and other information about the archives

- _Example_
  ```
    -rw-r--r--. 1 pcp pcp   1229456 Jun 22 04:41 20200622.00.10.0.xz
    -rw-r--r--. 1 pcp pcp   1205188 Jun 22 09:11 20200622.00.10.1.xz
    -rw-r--r--. 1 pcp pcp   1213824 Jun 22 13:42 20200622.00.10.2.xz
    -rw-r--r--. 1 pcp pcp 104873740 Jun 22 18:13 20200622.00.10.3
    -rw-r--r--. 1 pcp pcp   1645140 Jun 22 18:17 20200622.00.10.4
    -rw-r--r--. 1 pcp pcp     71652 Jun 22 18:17 20200622.00.10.index
    -rw-r--r--. 1 pcp pcp   3633933 Jun 22 18:16 20200622.00.10.meta
  ```
  - Each of the above all belong to the same data set, because they all start with the same prefix `20200622.00.10`
  - The prefix indicates the data starts at June 22 2020 at 12:10AM
  - The files ending in `.0.xz`, `.1.xz`, `.2.xz`, `.3`, and `.4` are all raw binary data files
  - The files ending in `.index` and `.meta` are the temporal index file and the meta data file for the data set respectively

- `pmdumplog -L` can dump information about the archive including details about the timezone it came from, the time period the archive covers, and the host it comes from.

### Common Options

- `-a/--archive` Nearly all pcp tooling comes with this option and indicates to use the archive file created from pmlogger as the source for data rather than the current system.
- `-t/--interval=` Nearly all pcp tooling comes with this option and describes the interval on which metrics are reported. IE metrics can be reported on the 10 minute interval like default SAR configurations or down to the 5 second inteval for higher resolution investigations.
- `-z/--hostzone` Nearly all pcp tooling comes with this option and informs to report time stamps and dates from the perspective of the timezone the data was collected in rather than the timezone the data is being reviewed in. IE if an archive comes from a system in Pacific Timezone while the system used to inspect the data in the Eastern Standard Timezone, the times and dates will be interpreted from the EST perspective by default but form the PST timezone if -z is provided.
- `-S/-T <[‘@ DAY MON #] HH:MM[:SS YYYY’]>` Tells the tool to begin reporting at (`-S`) or record up to (`-T`) the timestamp provided.

### Basic Example Commands

```
pmrep :sar-B-old-kernel -z -a 20200629.00.10
```
- pmrep produces paging activity represented in a sar like fashion ( :sar-B-old-kernel) from the archive that began at 12:10 AM on June 29 2020 (-a 20200629.00.10) and the data needs to be interpreted from the source host timezone at the default 1 second interval.

```
pmrep :vmstat -t 30s
```
- pmrep produces output similar to the vmstat tool (`:vmstat`). The data is sampled every 30 seconds (`-t 30s`).

```
pmrep :sar-u-ALL-P-ALL -t 1m -z -a 20200629.00.10 -S '10:00:00'
```
- CPU usage is presented for all CPUs similar to SAR (`:sar-u-ALL-P-ALL`), averaged over a 1 minute interval (`-t 1m`) where metrics are from the source system's timezone (`-z`), the metrics are sourced from the archive starting at 12:10 AM on June 29 2020 (`-a 20200629.00.10`), but begin reporting metrics captured after 10 AM.

### Common Workflow

1. Install `pcp-zeroconf`
  - `pcp-zeroconf` is a one-shot package to install and start necessary components for general support.
  - `# yum install pcp-zeroconf`
  - The package is available in the `rhel-7-server-rpms` repository for RHEL 7 and `rhel-8-for-x86_64-appstream-rpms` repository for RHEL 8.
  - For RHEL 6 and under, PCP is discouraged from use.
2. Wait for the issue to be reproduced and gather a fresh sosreport with timestamps of the occurance of the issue. Should the sosreport not contain the archives in the default location `/var/log/pcp/pmlogger/<HOSTNAME>/`, request a tarball of the archives.
3. Use `pmdumplog -z -L <ARCHIVE>` to determine which archives span times of interest.
  - For example;
    ```
     r8 # pmdumplog -z -L /var/log/pcp/pmlogger/r8/20200630.00.10.0.xz
    Note: timezone set to local timezone of host "r8" from archive
    
    Log Label (Log Format Version 2)
    Performance metrics from host r8
        commencing Tue Jun 30 00:10:43.866245 2020
        ending     Tue Jun 30 13:36:23.894761 2020
    Archive timezone: EDT+4
    PID for pmlogger: 18738
    ```
4. Begin analysis. Given the workflow for reviewing performance metrics historically been to review SAR and then collectl, the following steps recreate this workflow.

### SAR-Like Analysis

- CPU usage
  - `pmrep :sar-u-ALL-P-ALL`
  - `pmrep :sar-u-ALL`
  - `pmrep :sar`
  - `pcp atopsar -c`
- Processes and Context Switches Per Second
  - `pmrep :sar-w`
- Swap activity
  - `pmrep :sar-W`
- Page scan and reclaim activity
  - `pmrep :sar-B-old-kernel` for RHEL 7
  - `pmrep :sar-B` for RHEL 8 and above
- System-wide IO activity
  - `pmrep :sar-b`
- Memory usage stats
  - `pmrep :sar-r`
- Swap usage
  - `pmrep :sar-S`
- Hugepages
  - `pmrep :sar-H`
- Load average and process states
  - `pmrep :sar-q`
- Per-device IO activity
  - `pmrep :sar-d-dev`
  - `pmrep :sar-d-dm`
- Network throughput metrics
  - `pmrep :sar-n-DEV`
- Network error metrics
  - `pmrep :sar-n-EDEV`
- NFS Client metrics
  - `pmrep :sar-n-NFSv4`
- NFS Server metrics
  - `pmrep :sar-n-NFSDv4`
- Network socket
  - `pmrep :sar-n-SOCK`

### Collectl-like Analysis

- For most of collectl-like analysis, you can lower the interval to a much more frequent interval to gain the higher resolution capabilities provided from collectl. However, for collectl-specific metric output, custom pmrep views found [here](https://github.com/haithcockce/learning-stuff/tree/master/docs/pcp/custom-pmrep-views) for pmrep. The configs can be added to the default pmrep configuration file `/etc/pcp/pmrep/pmrep.conf` or added to a separate file and used with pmrep with the `-c <FILE>` option.

### Common Issues

- Running pmrep or other tools against an archive produces something similar to the following:
  ```
     r7 # pcp -a /var/log/pcp/pmlogger/r7/20190208.09.30 dmcache
    Error: not all required metrics are available
    Missing: ['dmcache.cache.used', ... , 'dmcache.write_misses']
  ```  
  - The above error typically indicates the tool, pmrep view, etc needs a specific metric that is not collected in the archive. The archive can be checked it truly lacks to metrics in question with `pminfo` or `pmdumplog`.
- The output is missing values across several different tools. Check the PMCD and pmlogger logs. Example logs:
  ```
  $ less var/log/pcp/pmlogger/<HOSTNAME>/pmlogger.log
  Warning [/var/lib/pcp/config/pmlogger/config.default, line 96]
  Description unavailable for metric "mem.util.swapFree" ... not logged
  Reason: No PMCD agent for domain of request
  
  $ less var/log/pcp/pmcd/pmcd.log
  [Tue Oct  8 13:46:24] pmcd(34290) Warning: pduread: timeout (after 5.000
  sec) while attempting to read 12 bytes out of 12 in HDR on fd=19
  ```
  - If a PMDA dies, the PMCD logs will indicate it can not read from the PMDA as noted above.
