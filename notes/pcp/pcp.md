# PCP: Performance Co-Pilot

###### Mark Goodwin, and the other guy
###### 17 and 18 October 2017

# TODO

* Supply outputs of examples
* Reorganize
* Determine what `pmdumplog` is used for

## Overview

* Framework for system-level performance analysis.
* Similar to sysstat and provides collection, monitoring, and analysis of sys metrics.
* Distributed so can be used for monitoring multiple remote hosts (or local)
* API to hook into
* __Conceptually__ An instance is something which would create generate metrics (such as a net
    interface, numa node, cpu, etc). The agents can read and report metrics from the instances
    consumable by the PCP tools. A daemon idles until poked by a client tool to engage the agents
    to produce metrics for the client tool.

## Architecture

* `pmcd` Performance Metrics Collector Daemon
  * Collects data
  * Lightweight and pull model.
  * No proactive sampling and done only when requested by clients.
* `pmdas` Performance metrics domain agents
  * The things that goes to get the data or satisfies the request for data. Similar to agents on a VM.
  * Unique to the things monitored
* `pmns` Performance metrics name space
  * hierarchical names space of available metrics
* Clients
  * The actual things requesting data?

```
PMDA1 (kernel) --.          .-> pmlogger
PMDA2 (DB)     --|          |-> pmchart
...            --->  PMCD ----> pmval
PMDA#          --'          '-> ...
```

#### Existing PMDAs

* Activemq, apache, freebsd, nginx, gluster, linux, mysql, etc

## Installation

* `yum install pcp`
* `systemctl enable pmcd pmlogger pmie`
* `systemctl start pmcd pmlogger pmie`
  * `pmcd` alone means just live data
  * `pmlogger` allows recording and archiving data
* `pcp`
  * Provides basic overview of stuff and installation
* `yum install pcp-webapi pcp-webapp...`
* Can also do just one command: `yum install pcp-zeroconf` and will do 'set and forget'
  * Recommend `/var` > 10GiB

## Commands

#### `pminfo`

* Metadata about performance metrics
* `pminfo -dtf mem.freemem`
  * `-d` describe metric
  * `-t` help text
  * `-f` print field
* `pminfo -l cgroup.blkio.dev.throttle.io_service_bytes.read`

#### `pmstore`

* PMDAs not only pull data, but you can push data into metrics
* For example, writing to files to clear counters in `/proc`

#### `pmval`

* CLI tool to sample from the specific metrics
* `pmval mem.freemem`

#### `pmchart`

* QT Gui app and configurable
* What do you want to plot and how frequently
* Time-aligned axis for all charts.
* GUI tools to create graphs from live data or archives. Comes with canned views or can be
  configured to view specific details
* Can click on charts to see timestamps
* Examples:
    * `pmchart -a <ARCHIVE> -c CPU -c Netbytes -c Memory &`
        * `-c CPU [-c <VIEW>]` is pulling up _views_ and not the specific dot-delimited metrics.
          Check the canned views (location below) or custom view created
* Configurations
    * Canned charts are in `/var/lib/pcp/config/pmchart/`

## Historical data

#### `pmlogger` and `pmdiff`

* `systemctl enable pmlogger && systemctl restart pmlogger`
* Configs `/etc/pcp/pmlogger`
  * Defaults to 1 minute collection
* Logs to `/var/log/pcp/pmlogger/<hostname>`
* Parseable via `-a/--archive`:
  * `pmval -a /var/log/pcp/pmlogger/<hostname>/<date> mem.freemem -S "Apr 09 02:00" -T "Apr 09 03:00" -t20min`
* `pmdiff` can be used to see the difference between two archive files and ranks by amount differences

#### `pmlogextract`

* Can reduce, extract, and concatenate/merge archives

#### `pmlogsummary`

* Calculate averages of metrics and output summary

#### `pmie`

* Performance Metrics Inference Engine
* Performance inference engine that can respond based on conditions based on arbitrarily provided rules
  * Example: if IOWait is higher than 95% for 3 hours, email.
* `/etc/pcp/pmie/config.default` Configurations for what to do

## Cases and casework

* Idea is simply to use the better tools that exist
* PCP is now integrated into sosreports.
* use `-p` and `-z` options

## `pmrep`

* CLI utility for pretty much anything pcp but usually to provide text output for stuff. Seems to
  be similar to `perf script` to gather
* Form: `metric[,label[,instance[,unit/scale[,type[,width]]]]]`
  * label is to put a name to the metric
* Examples:
  * `pmrep kernel.all.sysfork,forks,,,,8 [-t .5] [-x]`
  * `pmrep --archive 20120510 kernel.cpu.util.user -t 2 -s 10 -S10minutes`
* Config `/etc/pcp/pmrep.conf`
  * Has multiple examples for sysstat tools to compare to

## `pmdumplog`

* __NEED TO DETERMINE WHAT THIS TOOL IS USED FOR__
* `pmdumplog -L <ARCHIVE>` Information about archive and where it came from
* `<ARCHIVE>.0`  data file
* `<ARCHIVE>.index` temporal index, effectively index file. Separate from data file to make indexing
  data file much faster
* `<ARCHIVE>.meta` metadata containing metric names and descriptors
* Can use `-u` for uninterpolated replay of data so replay occurs with native options and sample.
  Native to how the data was collected

## Miscellaneous

* `--container` used to pull metrics from container
* Pastebin with examples from dudeman on `pmrep`:

```
- swiss-army chainsaw PCP utility from Marko Myllynen (RH)
- various output modes:
  stdout the default, but also archives & many others
  config file or command line
  each metric value to report can be configured:

       The  following  metricspec requests the metric kernel.all.sysfork to be
       reported under the label forks, converting to the default rate  count/s
       in an 8 wide column.  Although the definitions in this compact form are
       optional, they must always be provided in the order specified above.

               kernel.all.sysfork,forks,,,,8

  general form:
               metric[,label[,instance[,unit/scale[,type[,width]]]]]

- optionally print an extended header (-x -> like pmval)

- config: /etc/pcp/pmrep.conf
  can have named sections, like "[vmstat]", use :vmstat on command line.

- examples:
         Display network interface metrics on the local host:
           $ pmrep network.interface.total.bytes

       Display all outgoing network metrics for the wlan0 interface:
           $ pmrep -i wlan0 -v network.interface.out

       Display  per-device  disk  reads and writes from the host server1 using
       two seconds interval and CSV output format:
           $ pmrep -h server1 -o csv -t 2s disk.dev.read disk.dev.write

       Display timestamped vmstat(8) like information  using  MBs  instead  of
       bytes and also include the number of in-use inodes:
           $ pmrep -p -b MB vfs.inodes.count :vmstat

       Display  sar  -w  and sar -W like information at the same time from the
       PCP archive ./20150921.09.13 showing values recorded between 3 - 5 PM:
           $ pmrep -a ./20150921.09.13 -S @15:00 -T @17:00 :sar-w :sar-W

       Record all 389 Directory Server, XFS file  system  and  CPU/disk/memory
       related metrics every five seconds for the next five minutes to the PCP
       archive ./a:
        $ pmrep -o archive -F ./a -t 5s -T 5m ds389 xfs kernel.all.cpu disk mem
```

`proc.psinfo.pid proc.id.euid_nm proc.psinfo.priority proc.psinfo.ppid proc.psinfo.sname proc.memory.size proc.memory.rss proc.psinfo.processor proc.psinfo.stime proc.psinfo.utime proc.io.read_bytes proc.io.write_bytes proc.psinfo.maj_flt proc.psinfo.minflt proc.psinfo.cmd`
`proc.psinfo.vctxsw proc.psinfo.nvctxsw`
