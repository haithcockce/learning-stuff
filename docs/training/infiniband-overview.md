# Intro to Infiniband and RDMA

### What?

- _Remote Direct Memory Access_ (or RDMA) is a transport protocol between two or more systems with a focus on High Performance Computing (HPC) environments. In its most basic setup, RDMA offers:
  - Kernel bypass; sending and receiving of data is done similar to simply reading from some area of memory (with a couple more moving parts to facilitate such) exposed directly to the application in question and thus does not require entering kernelspace to perform these operations
  - Zero-copy; most transport protocols/stacks require some intermediary area of memory to hold data where data is put first and then copied to the network interface device (for egress traffic) or copied into the socket buffers for the process (for ingress traffic). RDMA reads and writes directly from the areas of memory used for transport.
  - Minimal CPU involvement; most transport protocols require some CPU intervention (typically via a softirq) to move data off of or onto the network adapter. When sending data via RDMA, the data will be placed directly into remote memory.
  - Message-oriented communications; most networking stacks use a stream-style transfer wherein the data stream is segmented into discrete units for transfer (called _packets_ in TCP/UDP). For RDMA's message-oriented approach, the data is sent as a single logical unit. If need be, the backing hardware will need to segment the data. For stream-style communications, the software logically segments the data.

- _Infiniband_ (IB) is the network fabric and link-layer flow control
  - As RDMA requires very tight deadlines with massive throughput and still be lossless, typical networking fabrics will not be able to handle RDMA (by itself).
  - IB operates as the L2 or link layer between nodes and any additional infrastructure between RDMA applications (such as the physical network fabric adapters and whatnot)

- _Verbs_ also referred to as _uverbs_ is an interface for application stacks to use in order to facilitate RDMA and Inifniband communications and setup _channels_
  - Conceptually similar to an API, but multiple implementations exist in the world.
  - Verbs are divided logically into "control path" (manage RDMA/IB resources) and "data path" (using RDMA/IB resources to send/receive data)
  - At a high level, via the verbs interface, we use the control path/command channel to setup and create our Infiniband resources (such as the noted queues) and use the data path/data channels to send and receive data.

- As implied above, Infiniband and RDMA provide a whole new networking stack which can be very specifically tailored to particular HPC setups. As such, the internal bits of Infiniband can be divided somewhat into three somewhat distinct layers:
  - _Upper Layer Protocols_ (ULP) are application stack-specific protocols and libraries. For example, networked filesystems or networked storage solutions built on top of RDMA/IB would have libraries and operations at this layer.
  - _Mid-Layer_ or functionality largely arbitrated and initialized by the OS in question to manage the underlying RDMA/IB operations and IB network fabric.
  - Hardware-specific drivers

- _HCA_ Host Channel Adapter, dedicated hardware which enables the RDMA protocol and operates like a Network Interface Card specialized for RDMA. Often referred to as just Channel Adapter (CA).
- Below is a heavily abstracted diagram detailing some of the above info

<img align="center" src="https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/media/tcp-vs-rdma.jpg">


- **Note** Be prepared for a _lot_ of initialisms. The userspace and kernelspace implementations of RDMA/IB use the initialisms within their code extensively, so knowing the initialisms helps in reading the code.

### Communication Overview

#### Node-To-Node Communications

- RDMA encapsulates actions to perform some operation as a _Work Request_ (WR), where completion of this is referred to as a _completion_.
- Work Requests are organized into queues generically referred to as _Work Queues_ (WQ);
  - _Send Queues_ (SQ) are Work Queues responsible for sending data
  - _Receive Queues_ (RQ) are Work Queues responsible for receiving data
  - Send and Receive queues are created in pairs called _Queue Pairs_ (QP) to establish node-to-node communication
- When a Work Request is queued to a Work Queue, the Work Request is first encapsulated into a _Work Queue Entity_ (WQE)
  - Queueing a Work Queue Entity to either Work Queue is referred to as _posting_
  - Work Queue Entities are differentiated as _Send Request_ (SR) when posted to a Send Queue and _Receive Request_ (RR) when posted to a Receive Queue
- _Work Completions_ are entities of data containing info about the completion of some request
  - As noted above, upon completion of posting some Work Request, a Work Completion may be generated
  - If a Work Completion is created from some action, it is queued to a _Completion Queue_ (CQ) as a _Completion Queue Entity_ (CQE)
  - A Send Request may not end with a Completion Queue Entity
  - A Receive Request always ends with a Completion Queue Entity
- During setup of IB structures, a _Local Key_ (L_key) and _Remote Key_ (R_key) are generated for security in accessing memory regions (more on all this in "Application-To-Channel Adapter Communication")

<img align="center" src="https://github.com/haithcockce/learning-stuff/blob/master/docs/training/media/queue_pair.png?raw=true">

- **Note** The above is raw RDMA with basic Infiniband networking. The number of variances on this is quite large and continues to grow as the technology expands.


#### Application-To-Channel Adapter Communications

As noted earlier, RDMA allows direct communication with hardware from userspace. Several entities exist to facilitate direct hardware communication.

- Via the verbs interface, create an _Infiniband Context_, a structure which aggregates info about an IB device.
  - Userspace version `ibv_context` aggregates device and device operations
  - Kernelspace version, `ib_ucontext` maintains the kernelspace bits of the context such as what verbs can be associated with the device.
- Create a _Protection Domain_ (PD), a simple structure referenced across several other IB/RDMA structures to associate structures (IE which queue pairs belong with which devices and the like)
  - Userspace version `ibv_pd` more or less just contains a pointed to the relevant `ibv_context`
  - Kernelspace version `ib_pd` contains the relevant `ib_ucontext` as well as R_key and L_key stuff (more on that in a moment)
- Register a _Memory Region_ (MR), an area of memory designated to perform various operations or use as the buffer to read from or write to another node
  - An MR is an aggregate structure containing pointers to relevant IB contexts, protection domains, the starting address of the region of memory, how large the region is, and L/R_Keys
  - Userspace version `ibv_mr` contains all the above
  - Kernelspace version `ib_mr` contains all the above, but also includes a possible list of queue pairs and includes some addition things for low-level management stuff (such as what the page size for memory is)
  - On registering, an L_key and R_key are generated and associated with the memory region
    - The L_key enables authenticating access to a memory region on the local system.
    - The R_key can be sent to remote systems and enable authenticating access to a local memory region from the remote system (IE performing an RDMA read/write)
- Create Send and Receive Completion Queues as well as a Queue Pair (overview thereof can be found in Node-To-Node Communications)
  - Userspace version `ibv_qp` aggregates an individual send and receive queue respectively, the relevant IB context and protection domain, identifiers and locks for synchronous access, a queue pair state descriptor and such
  - Kernelspace version `ib_qp` aggregates similar things, but points to the IB device directly rather than a context. It also includes pointer and info handling architecture and implementation-details (such as an event handler, security, max read and write sizes available for the queue pair, etc)
  - Once created, the completion queues and queue pairs need to be assigned to each other and initialized.
- _Reminder_ The setup actions are performed via the verbs. For most implementations, this is all provided from the `libibverbs` package.

### Hardware

### Kernel Modules

So many modules exist.

### Workflow in Program

1. Via the verbs, create an infiniband context
  - The application must first _register the memory_ which denotes an area of memory within the application as dedicated to RDMA operations.
  - Upon registering the memory for RDMA communications, the HCA creates a channel from itself to the area of memory.
2. Once the HCA is opened, the application establishes a connection.

### Kernel Bits

#### Modules

- [Mellanox Linux User Manual](https://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4_3.pdf)

# References

- [Mellanox Whitepaper](https://www.mellanox.com/pdf/whitepapers/Intro_to_IB_for_End_Users.pdf) ELI5 Overview
- [Mellanox Whitepaper](https://www.mellanox.com/pdf/whitepapers/IB_Intro_WP_190.pdf) More technical explanation of IB stack
- [Mellanox Linux User Manual](https://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4_3.pdf)
- [HPC Avisory Council Slide Deck](https://www.hpcadvisorycouncil.com/pdf/Intro_to_InfiniBand.pdf) Odds and Ends on terminology (like MAD, GMP, SMP, etc)
- [Intro to Infiniband Architecture](http://www.buyya.com/superstorage/chap42.pdf) Overview of the queue pairs
- [iWARP vs. RoCE](https://www.snia.org/sites/default/files/ESF/RoCE-vs.-iWARP-Final.pdf) and a [youtube](https://www.youtube.com/watch?v=nGTY14UptOA) version of the same thing.
- [Kernel Implementation](https://www.kernel.org/doc/ols/2005/ols2005v2-pages-279-290.pdf) Dated, but relevant kernel implementation
- [Introduction to Programming Infiniband RDMA [sic]](https://insujang.github.io/2020-02-09/introduction-to-programming-infiniband/)
[Programmer Sought](https://www.programmersought.com/article/24218148942/)
43
[Verbs Programming Tutorial](https://www.csm.ornl.gov/workshops/openshmem2013/documents/presentations_and_tutorials/Tutorials/Verbs%20programming%20tutorial-final.pdf)
44
[InfiniBand: An Introduction + Simple IB verbs program with RDMA Write](https://blog.zhaw.ch/icclab/infiniband-an-introduction-simple-ib-verbs-program-with-rdma-write/)
45
