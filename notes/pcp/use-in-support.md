# PCP Use in Support

### 0. Architecture

```
PMDA1 (kernel) --.          .-> pmlogger
PMDA2 (DB)     --|          |-> pmchart
...            --+>  PMCD --+-> pmval
PMDA#          --'          '-> ...
```

- **`PMDA`** Performance Metric Domain Agents
  - Things poked to actually retrieve data. For example, the kernel will read
    `/proc/<PID>/stat` and present it in a universally consumable way for PCP to
    read on any version
  - Organized via domains (subsystems). Example: `pminfo kernel`
  - Can write your own for any thing.
  - Examples: linux, kernel, apache, activemq, nginx, mysql, etc
- **`PMCD`** Performance Metrics Collector Daemon
  - Lightweight and pull Model
  - Collects the data but only when requested and not proactively
    - This can cause initial samples in some tools to be 'N/A' or '0' or something similar
- **`PMNS`** Performance Metrics Namespace
  - The hierarchical organization of metrics (`pminfo kernel`)

### 1. Installation

- Customers sometimes have it installed already, so check `installed-rpms` for
  packages and `/var/log/pmlogger/` for data

##### `pcp-zeroconf`

- Will start and enable logging immediately:

```
 r7 # systemctl status pmlogger
● pmlogger.service - Performance Metrics Archive Logger
   Loaded: loaded (/usr/lib/systemd/system/pmlogger.service; enabled; vendor preset: disabled)
   Active: active (exited) since Fri 2019-02-08 09:25:42 EST; 1min 1s ago
     Docs: man:pmlogger(1)
 Main PID: 4267 (code=exited, status=0/SUCCESS)
   CGroup: /system.slice/pmlogger.service
           └─10776 /usr/libexec/pcp/bin/pmlogger -P -r -T24h10m -c config.default -v 100mb -m pmlogger_check 20190208.09.25

Feb 08 09:25:42 r7 systemd[1]: Starting Performance Metrics Archive Logger...
Feb 08 09:25:42 r7 pmlogger[4267]: /usr/share/pcp/lib/pmlogger: Warning: Performance Co-Pilot archive logger(s) not permanently enabled.
Feb 08 09:25:42 r7 pmlogger[4267]: To enable pmlogger, run the following as root:
Feb 08 09:25:42 r7 pmlogger[4267]: # /usr/bin/systemctl enable pmlogger.service
Feb 08 09:25:42 r7 pmlogger[4267]: Starting pmlogger ...
Feb 08 09:25:42 r7 systemd[1]: Started Performance Metrics Archive Logger.
```

- Despite the above, `pmlogger` is still enabled and will start on reboots
- Available in `rhel-7-server-rpms` repo

### 2. Components

- `pmcd` metric collection daemon
  - Configs at `/etc/pcp/pmcd/`
- `pmlogger` records data from pmcd into `/var/log/pcp/pmlogger/<HOSTNAME>/`
  - Defaults logs to `<DATE>.0`, `<DATE>.index`, and `<DATE>.meta`
  - `<DATE>.0` is the data
  - `<DATE>.index` is the temporal index used to look up metrics in `<DATE>.0`
    at specific times
  - `<DATE>.meta` describes the data such as what metrics are collected, when
    the collection started and ended for that file, timezone of the system it
    was collected from, PID of pmlogger, etc
  - `pmlogger(1)`
  - Configs at `/etc/pcp/pmlogger/control{/,.d/}`
- **`pmie`** Performance Metrics Inference Engine
  - Perform an arbitrarily defined action under defined conditions.
  - For example, you can kick off `perf record -a -g -- sleep 30` when `%sys` is
    over 80% for 10 seconds.
- `pmchart` QT GUI for plotting metrics
- **`pmrep`** Performance Metrics Reporter
  - "Swiss army knife" of metric reporting and can slice and dice the metrics as
    fine as you want.
  - Steep learning curve
- Tons of additional tools for reviewing data [here](https://access.redhat.com/articles/2372811)

### 3. Sanity Checks

##### Does the pcp data cover the time frame in question?

- `pmdumplog -L <ARCHIVE FILE>` produces a dump of overall information, for example, which host it came from, what time frame it occurred in, etc
- Example:

```
pmdumplog -L <ARCHIVE FILE>
Log Label (Log Format Version 2)
Performance metrics from host <HOSTNAME>
  commencing Wed Nov  1 08:25:05.459 2017
  ending     Wed Nov  1 09:02:25.946 2017
Archive timezone: CDT+5
PID for pmlogger: 34140
```

##### Was the data collected when the specific metrics were set to monitor?

- `pminfo -a <ARCHIVE FILE>` produces a list of enabled metrics when the pmcollector was running.
- Example:

```
 $ pminfo -a ./20171101.07.25 | grep ^proc | head
proc.nprocs
proc.runq.runnable
proc.runq.blocked
proc.id.uid
proc.id.euid
proc.id.suid
proc.id.fsuid
proc.id.gid
proc.id.egid
proc.id.sgid
```

##### Were the proper PMDAs enabled when metric collection happened?

- If a command fails with a similar message as below, then no:

```
 r7 # pcp -a /var/log/pcp/pmlogger/r7/20190208.09.30 dmcache
Error: not all required metrics are available
Missing: ['dmcache.cache.used', 'dmcache.cache.total', 'dmcache.metadata.used', 'dmcache.metadata.total', 'dmcache.read_hits', 'dmcache.read_misses', 'dmcache.write_hits', 'dmcache.write_misses']
```

### 4. Analysis

#### 4.a General Tips

- Nearly all pcp-related commands come with some form of `-a`/`--archive` option to
  read through archive files
- Make sure to use the customer's timezone! Nearly all pcp-related commands come
  with a `-z`/`--hostzone` option set timestamps with respect to the customer's
  system
- `-t` can change the time interval on reporting. By default, pcp-zeroconf gathers
  data at 10 second intervals and, for example, `-t 10m` can set the interval to
  every 10 minutes in reporting metrics with tools
- Many tools have predefined "views" which emulate other tools.
  - `pmrep` has views in `/etc/pcp/pmrep/pmrep.conf`
  - `pcp` has views as executables in `/usr/libexec/pcp/bin/`

#### 4.b SAR-Like Analysis

- CPU
  - `pcp atopsar -c`
  - `pmrep :sar`
  - `pmrep :sar-u-ALL`
  - `pmrep :sar-u-ALL-P-LL`
- Proc/s and Cswch/s
  - `pmrep :sar-w`
- pswpin/s and pswpout/s
  - `pmrep :sar-W`
- Page scan and reclaim activity
  - `pmrep :sar-B-old-kernel`
- IO activity
  - `pmrep :sar-b`
- Page frame caching and releasing
  - Still searching
- Memory usage stats
  - `pmrep :sar-r`
- Swap usage
  - `pmrep :sar-S`
- Hugepages
  - `pmrep :sar-H`
- Dentry/inode usage
  - Still searching
- Load average and process states
  - `pmrep :sar-q`
- TTY Device activity
  - Still searching
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

#### Collectl-Like Analysis

- Here, defer to using the above but using a higher resolution delta

#### PMREP Custom Views

```
[collectl-sc--verbose]
# User  Nice   Sys  Wait   IRQ  Soft Steal Guest NiceG  Idle  CPUs  Intr  Ctxsw  Proc  RunQ   Run   Avg1  Avg5 Avg15 RunT BlkT
header = yes
repeat_header = 30
unitinfo = no
globals = no
timestamp = yes
precision = 2
delimiter = " "
# User
User = User,,,,7
User.formula = 100 * kernel.all.cpu.user / hinv.ncpu
User.unit = s
# Nice
Nice = Nice,,,,7
Nice.formula = 100 * kernel.all.cpu.nice / hinv.ncpu
Nice.unit = s
# Sys
Sys = Sys,,,,7
Sys.formula = 100 * kernel.all.cpu.sys / hinv.ncpu
Sys.unit = s
# Wait
Wait = Wait,,,,7
Wait.formula = 100 * kernel.all.cpu.wait.total / hinv.ncpu
Wait.unit = s
# IRQ
IRQ = IRQ,,,,7
IRQ.formula = 100 * kernel.all.cpu.irq.hard / hinv.ncpu
IRQ.unit = s
# Soft
Soft = Soft,,,,7
Soft.formula = 100 * kernel.all.cpu.irq.soft / hinv.ncpu
Soft.unit = s
# Steal
Steal = Steal,,,,7
Steal.formula = 100 * kernel.all.cpu.steal / hinv.ncpu
Steal.unit = s
# Guest
Guest = Guest,,,,7
Guest.formula = 100 * kernel.all.cpu.guest / hinv.ncpu
Guest.unit = s
# NiceG
NiceG = NiceG,,,,7
NiceG.formula = 100 * kernel.all.cpu.guest_nice / hinv.ncpu
NiceG.unit = s
# Idle
Idle = Idle,,,,7
Idle.formula = 100 * kernel.all.cpu.idle / hinv.ncpu
Idle.unit = s
# CPUs
hinv.ncpu = CPUs,,,,4
# Intr
kernel.all.intr = Intr/s,,,,9
# Ctxsw
kernel.all.pswitch = Ctxsw/s,,,,9
# RunQ
kernel.all.runnable = RunQ,,,,7
# Run
kernel.all.running = Run,,,,7
# Avg1/5/15
kernel.all.load = ldavg,,,,
# RunT
# kernel.all.running = RunT,,,,7
# BlkT
kernel.all.blocked = BlkT,,,,7
```
