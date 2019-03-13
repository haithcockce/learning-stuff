# Infiniband Architecture

### Introduction

- _Fabric_ connects _switches_ to _Processor Nodes_ and _IO Nodes_.
- Processor nodes have _Fabric Adapters_ operating as HCAs in processor mode
  while IO nodes have Fabric Adapters operating as TCAs.
- Provides IO and IPC capabilities across multiple systems, but only defines the
  way data is transported from IO driver on processor nodes to scsi adapter
- _System Area Network_ connects all the nodes in the network
- IO operations are offloaded to card rather than go through protocols on OS,
  and API within client in userspace can bypass the OS and kernel

##### Communications

- Services requests, or _work queue elements_, are queued up in _work queues_
  typically created in pairs called _queue pairs_ (QP) for sending and receiving
  (where and what to send and where to put received data respectively)
- Work Queue Elements (WQE) can be thought of as jobs
- Upon WQE completion, a _Completion Queue Element_ (CPE) is made and put on a
  completion queue describing the associated WQE
- WQE can be processed in parallel even from the same work queue (as some WQE
  need to wait for an ack or data is broken up into multiple packets (except for
  when packets must be processed in order)).
- Operations
  - Send queue: SEND. WQE tells hardware to send data. A receive WQE at the 
    other side of the transaction tells hardware to pick it up
  - Send queue: RDMA. Operate on a location in memory on the other system.
    - RDMA-WRITE: Push data to the client
    - RDMA-READ: read data from client
    - ATOMIC: RW combined; read from client, and push an update to that spot if
      the value is what is expected.
  - Send queue: Memory Binding. An area of memory in the memory region with a
    specified set of who to share it with and with what permissions
  - Recv: We consume a recv WQE and make a CQE to let the server know we
    finished

##### Connections

- Connected mode - Work queue pairs are associated with a single remote host and
  identified by a Local and Global ID (LID and GID)
- Datagram mode - Work queue pairs are not associated with a remote host but the
  WQE defines the destination
