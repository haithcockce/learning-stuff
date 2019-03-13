# Systemd--

###### Kyle Walker, 24 October 2017

## Design - High Level Overview

- System-V For RHEL 5 (Upstart in RHEL 6) everything is self contained and independent of other
  operations. There is a start, service, then finish. Then the next unit. "Fire and forget"
- Systemd: dependency-based. For example, if service 2 depends on service 1 but service 3 is
  completely independent of the others, then service 2 will _not_ start until after service 1, but
  service 3 is whenever.
- A target is a bundling of services confirmed as started. For example, graphical.target, for
  example, is similar to run level 5.
- Ordering vs. Requirements
  - `Before=` and `After=` defines starting up something after something else
  - `Wants=` and `Requires=` defines hard requirements. For example, NFS _requires_ networking, so
    NFS service units should have `After=` _and_ `Requires=` something like `network-online.target`
  - Likewise, certain things may need particular fancy storage (apache served by NFS), so the
    service using the special storage, add `RequiresMountsFor=`
- `man bootup` or `man systemd.specials` shows dependency info and more specialized targets (such as local-fs, good for bad
  bootups)
- Unit files: EVERYTHING and found in `/etc/systemd/system/` (custom units?), `/run/systemd/system/` (ephemeral),
  `/usr/lib/systemd/system`.
  - `systemd delta` in sosreports gathers details of any custom units that override defaults.
  - `man systemd.directives` effectively indexes anything you want to look up

## Use

- example

```
[Unit]
Description=HNG

[Service]
ExecStart=/bin/bash -c "while true; do sleep 1; done"

[Install]
WantedBy=multi-user.target
```

- Make sure to tell Systemd everything it needs to know to do the thing. For example, does it need
  networking online (NFS for example)? Does it need to run as a specific user (Oracle for example)?
  Does it rely Does it require other services?
  - Many issues regarding processes being killed spontaneously is because of setting `su` in the
    unit. Check system-cgls and see if the service is under system.slice or user.slice
  - `man systemd.specials` describes the specialized targets
- When a service reports errors or exits to stderr and/or stdout, systemd will catch it and report
  it via `systemctl status <SERVICE>`

## Troubleshooting - Common Issues

- KISS the unit files
- If a systemctl command doesn't return, check system-cgls

#### Common Issues

- Service fails to start boot but starts fine after boot.
  - check `journalctl` such as `journalctl --no-pager --boot [--verbose]` for logs
  - If you have the same unit file, try it out. Otherwise, get it from the customer (maybe in
    sosreport provided maybe not)
  - Make sure the syntax matches exactly as it should in man pages.
- Timeout on start of service
  - set `SYSTEMD_LOG_LEVEL=debug systemctl restart <SERVICE>` and get strace of pid 1 (systemd).
  - `strace -Tttfv -s 4096 -p 1 -o /tmp/$HOSTNAME.systemd.strace & STRACEPID=$! && systemctl start <SERVICE> && kill $STRACEPID`
  - checkout if service type is forking and doesn't fork
- Hang on reboot or service doesn't stop correctly!
  - persistent journaling `mkdir /var/log/journal && systemctl restart systemd-journald` also
    checkout `(access.redhat.com/solutions/2993501)[access.redhat.com/solutions/2993501]`


## Extra

- Leonard Poeding? Systemd for administrators blog series
- `systemd-nspawn` and `machinectl` and whatnot.
- `systemd-run <COMMAND>`
