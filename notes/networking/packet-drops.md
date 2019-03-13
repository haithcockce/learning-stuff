# Packet Drops

To investigate packet drops, go through all the layers

## Layer 1/2

- `ethtool -S <IF>` stats on what is dropped and why on which queues. Looks for `drop`, `err`
- `/proc/net/dev` general stats on which devices had errors and packets etc. Not as high res as
  `ethtool -S <IF>`

## Layer 3/4

- `netstat -s` provides high resolution data into both L3/4 (Networking and Transport) layers

#### Network Layer

```
 $ grep -A 8 ^Ip: sos_commands/networking/netstat_-s
Ip:
    30296629 total packets received
    0 forwarded
    0 incoming packets discarded
    30018258 incoming packets delivered
    2708558 requests sent out
    56 dropped because of missing route
    552066 reassemblies required
    276033 packets reassembled ok
```

#### Transport Layer

###### TCP

```
 $ grep -A 10 ^Tcp: sos_commands/networking/netstat_-s
Tcp:
    4338 active connections openings
    5957 passive connection openings
    14 failed connection attempts
    3421 connection resets received
    1 connections established
    4113658 segments received
    2704834 segments send out
    382 segments retransmited
    0 bad segments received.
    55086 resets sent
```

###### UDP
- `### receive buffer errors` Recv a skb (data/packet) and attempted to append it to the socket but
  not enough space to attach it (full socket). skb is then dropped.


```
 $ grep -A 6 ^Udp: sos_commands/networking/netstat_-s
Udp:
    11036184 packets received
    876 packets to unknown port received.
    14850446 packet receive errors
    4836 packets sent
    14850446 receive buffer errors
    0 send buffer errors
```

- In the above, we see a ton of receive buffer errors meaning the recv UDP sockets are filling, so
  see what the current sizes of the recv UDP sockets are via `ss` and see if they are defaults and
  need to be increased or already huge af and app is just not consuming them. 

#### Application Layer (Sort of)

- `ss -peaonmi` Can provide a ton of information about sockets including the current size of the
  send and receive buffers, how much data is currently in the send and recv buffers, which pids and
  their process names owns that socket, the file descriptor number and inode for that socket, the
  kernel address for the socket (sk), the IP Address and port the socket belongs to, etc

```
 $ grep -A 1 ^udp sos_commands/networking/ss_-peaonmi
udp    UNCONN     0      0         *:255                   *:*                   users:(("amsHelper",pid=3457,fd=10))
udp    UNCONN     0      0         *:22499                 *:*                   users:(("rpc.statd",pid=48388,fd=7)) uid:29 ino:82201 sk:ffff882012fa8440 <->
	 skmem:(r0,rb262144,t0,tb262144,f0,w0,o0,bl0)
udp    UNCONN     0      0         *:111                   *:*                   users:(("rpcbind",pid=45559,fd=7)) ino:3035486 sk:ffff88dff2fc8880 <->
	 skmem:(r0,rb262144,t0,tb262144,f4096,w0,o0,bl0)
udp    UNCONN     0      0      192.168.20.1:123                   *:*                   users:(("ntpd",pid=2609,fd=22)) uid:38 ino:44073 sk:ffff885ff17d0440 <->
	 skmem:(r0,rb262144,t0,tb262144,f0,w0,o0,bl0)
udp    UNCONN     0      0      192.168.10.1:123                   *:*                   users:(("ntpd",pid=2609,fd=21)) uid:38 ino:30829 sk:ffff88dffb698440 <->
	 skmem:(r0,rb262144,t0,tb262144,f0,w0,o0,bl0)
```

- `r0, t0` The current amount of bytes waiting to be consumed in the recv side or sent on the send
  side respectively
- `rb######, tb#####` Current size of the recv/send buffer for the socket. Check `sysctl -a` to see
  if this matches the default `net.core.{rmem,wmem}_default` for send/recv side sizes.
