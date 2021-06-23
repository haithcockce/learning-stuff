# Intro to Infiniband and RDMA

## What?

- _Remote Direct Memory Access_ (or RDMA) is a transport protocol between two or more systems with a focus on High Performance Computing (HPC) environments. In its most basic setup, RDMA offers:
  - Kernel bypass; sending and receiving of data is done similar to simply reading from some area of memory (with a couple more moving parts to facilitate such) exposed directly to the application in question and thus does not require entering kernelspace to perform these operations
  - Zero-copy; most transport protocols/stacks require some intermediary area of memory to hold data where data is put first and then copied to the network interface device (for egress traffic) or copied into the socket buffers for the process (for ingress traffic). RDMA reads and writes directly from the areas of memory used for transport.
  - Minimal CPU involvement; most transport protocols require some CPU intervention (typically via a softirq) to move data off of or onto the network adapter. When sending data via RDMA, the data will be placed directly into remote memory.
  - Message-oriented communications; most networking stacks use a stream-style transfer wherein the data stream is segmented into discrete units for transfer (called _packets_ in TCP/UDP). For RDMA's message-oriented approach, the data is sent as a single logical unit. If need be, the backing hardware will need to segment the data. For stream-style communications, the software logically segments the data.

- _Infiniband_ (IB) is the network fabric and link-layer flow control
  - As RDMA requires very tight deadlines with massive throughput and still be lossless, typical networking fabrics will not be able to handle RDMA (by itself).
  - IB operates as the L2 or link layer between nodes and any additional infrastructure between RDMA applications (such as the physical network fabric adapters and whatnot)
  - In nearly every scenario, IB fabric adapters can work as IB devices or Ethernet devices for interoperability.

- _Verbs_  an interface for application stacks to use in order to facilitate RDMA and Inifniband communications and setup _channels_
  - Conceptually similar to an API, but multiple implementations exist in the world.
  - The userspace portions/interfaces are often referred to as _uverbs_ 
  - Verbs are divided logically into "control path" (manage RDMA/IB resources) and "data path" (using RDMA/IB resources to send/receive data)
  - At a high level, via the verbs interface, we use the control path/command channel to setup and create our Infiniband resources (such as the noted queues) and use the data path/data channels to send and receive data.

- As implied above, Infiniband and RDMA provide a whole new networking stack which can be very specifically tailored to particular HPC setups. As such, the internal bits of Infiniband can be divided somewhat into three somewhat distinct layers:
  - _Upper Layer Protocols_ (ULP) are application stack-specific protocols and libraries. For example, networked filesystems or networked storage solutions built on top of RDMA/IB would have libraries and operations at this layer.
  - _Mid-Layer_ or functionality largely arbitrated and initialized by the OS in question to manage the underlying RDMA/IB operations and IB network fabric.
  - Hardware-specific drivers

- _Host Channel Adapter_ (HCA), dedicated hardware which enables the RDMA protocol and operates like a Network Interface Card specialized for RDMA. Often referred to as just Channel Adapter (CA).

- _Subnet Manager_ (SM) main software to manage a network of IB nodes (like a router on a regular TCP/IP network)
  - Can run on a dedicated IB switch or simply on a system in the network. 
    - This implies a dedicated IB hardware switch is not necessary to run in an IB network but is often found in larger HPC clusters. 
    - When running on generic hardware, usually runs via `opensm`. 
  - An SM discovers and configures all the IB fabric devices to enable traffic flow between those devices.
  - An SM applies network traffic related configurations such as Quality of Service (QoS), routing, and partitioning of the fabric devices. 
  - Multiple SMs can reside on a single network
  - Can segment a network via _Partition Keys_ (PKeys), very similarly to a VLAN.

- Below is a heavily abstracted diagram detailing some of the above info

<img align="center" src="https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/media/tcp-vs-rdma.jpg" style="max-width:100%;">

- **Note** Be prepared for a _lot_ of initialisms. The userspace and kernelspace implementations of RDMA/IB use the initialisms within their code extensively, so knowing the initialisms helps in reading the code.

## Communication Overview

### Node-To-Node Communications

- RDMA encapsulates actions to perform some operation (such as sending or receiving data) as a _Work Request_ (WR), where completion of this is referred to as a _completion_.
- Work Requests are organized into queues generically referred to as _Work Queues_ (WQ);
  - _Send Queues_ (SQ) are Work Queues responsible for sending data
  - _Receive Queues_ (RQ) are Work Queues responsible for receiving data
  - Send and Receive queues are created in pairs called _Queue Pairs_ (QP) to establish node-to-node communication
- When a Work Request is queued to a Work Queue, the Work Request is first encapsulated into a _Work Queue Entity_ (WQE)
  - Queueing a Work Queue Entity to either Work Queue is referred to as _posting_
  - Work Queue Entities are differentiated as _Send Request_ (SR) when posted to a Send Queue and _Receive Request_ (RR) when posted to a Receive Queue
- _Shared Receive Queues_ (SRQ) provides a scalable method of receiving work
  - Posting Receive Requests is an atomic operation. For heavy ingress traffic, this atomic operation introduces an immediate bottleneck for addressing bursts of ingress activity.
  - A Shared Receive Queue maps to 1 or more Queue Pairs (and thus 1 or more Receive Queues)
  - A linked list of Receive Requests can be posted as a single atomic operation to a Shared Receive Queue for multiple Receive Requests to allow for scalability
- _Work Completions_ are entities of data containing info about the completion of some request
  - As noted above, upon completion of posting some Work Request, a Work Completion may be generated
  - If a Work Completion is created from some action, it is queued to a _Completion Queue_ (CQ) as a _Completion Queue Entity_ (CQE)
  - A Send Request may not end with a Completion Queue Entity
  - A Receive Request always ends with a Completion Queue Entity
- During setup of IB structures, a _Local Key_ (L_key) and _Remote Key_ (R_key) are generated for security in accessing memory regions (more on all this in "Application-To-Channel Adapter Communication")
- Communication types
  - While IB communication types span reliable-connected, unreliable-connected, reliable-datagram, and unreliable-datagram, most clients only use _unreliable datagrams_ (UD) or _reliable connected_ (RC) modes
  - UD operates similar to UDP in TCP/IP addressing while RC operates similar to TCP
  - **Note** [Connected mode is not an option](https://access.redhat.com/solutions/5960861) in some scenarios:
    - ConnectX-4 devices and above has Enhanced IPoIB enabled which forces use of UD mode and can not be disabled
    - RHEL 7 provided a kernel module parameter `ipoib_enhanced` for the module `ib_ipoib` to optionally disable Enhanced IPoIB and allow connected mode, however this was not added upstream meaning RHEL 8 and above can not disable Enhanced IPoIB (and thus are required to use UD mode)

<img align="center" src="https://github.com/haithcockce/learning-stuff/blob/master/docs/training/media/queue_pair.png?raw=true" style="max-width:100%;">

- **Note** The above is the most primative aspects of the RDMA/IB communication mechanisms. A lot of additional things are built on top of this, but communications between nodes ultimately fall into some form of the above scheme.



### Application-To-Channel Adapter Communications

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

## Hardware

Infiniband hardware is largely developed by Mellanox (recently acquired by NVIDIA) and Intel (though Intel recently handed it to Cornelis Networks). At the time of writing, no other IB hardware providers exist. 

### Mellanox

#### Hardware Details

- Mellanox cards are often described in terms of link width and link throughput when not described by theoretical max throughput. (https://www.hpcadvisorycouncil.com/pdf/Intro_to_InfiniBand.pdf)
- Link width
  - Links have one or more pairs of send/receive lanes (one send and one receive totalling to a single pair)
  - 1x means one pair, 4x is 4 pairs, 12x is 12 pairs and so on
- Link throughput
  - _Single Data Rate_ (SDR) is 2.5 Gb/s per lane (4x SDR is thus 10 Gb/s)
  - _Double Data Rate_ (DDR) is 5 Gb/s
  - _Quad Data Rate_ (QDR) is 10 Gb/s
  - _Fourteen Data Rate_ (FDR) is 14 Gb/s
  - _Enhanced Data Rate_ (EDR) is 25 Gb/s
  - Link throughput can be ascertained from `lspci`
- Additional throughputs and widths exist 
- Mellanox releases various series of IB HCAs named "ConnectX"

<img align="center" src="https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/media/mellanox-adapters.png" style="max-width:100%;">

- Mellanox HCAs have one or more ports 

### Addressing

- Addressing in IB is conceptually similar to IP addressing but with different conventions and names
- _Local Identifiers_ (LID)
  - 16 bits number used within a subnet by the switch for routing locally within the subnet
    - If you're familiar with IP Addressing, this is similar to your Private Network Addressing 
    - IE this is similar to you 192.168.9.*, 172.16.0.*, and 10.0.0.* networks 
  - Dynamically assigned at runtime
  - Info is embedded into Local Routing Header part of an IB packet for Link Layer routing 
- _Global Unique Identifiers_ (GUID)
  - Similar to a MAC address, they are assigned by vendor
  - 64 EUI-64 IEEE-deﬁned identifiers for elements in a subnet (such as an end node, port, switch, or multicast group)
- _Global Identifiers_ (GID)
  - Modeled after IPv6, 128 bits
  - The GUID provides the lower 64 bits of the GID
  - Used for routing across subnets via the Global Routing Header (GRH) in an IB packet for Network Layer routing
- _Partition Keys_ (PKeys)
  - Similar to a VLAN, PKeys can group IB ports/nodes all physically connected together into subnetworks that may or may not communicate with each other
  - PKeys also define levels of membership within partitions, where a "Full" member may communicate with all nodes of the partition while "Limited" can only communicate with Full members
  - PKeys are 16-bit hex values where the most significant bit defines Full (1) or Limited (0) membership and the remaining 15 bits provide a PKey identifier
  - IB Ports can be a member of multiple partitions, and the switch will always create a default partition for an entire subnet (`0x7fff` as its ID)


## IB Layers and Kernel Modules

As noted above, the software for interacting with IB fabric falls roughly into three categories, device drivers, mid layer, and ULPs. 

#### Device Drivers

- `mlx4` main low-level drivers for ConnectX-3 family of cards. Functionality is split into the following modules;
  - `mlx4_core` low-level hardware and firmware functionality such as device initialization, firmware command processing, and resource allocation to enable IB and Ethernet interoperability 
  - `mlx4_ib` IB-specific functionality of device and connects IB-specific parts of mid-layer to IB functionality of hardware. Technically part of the OS Mid-Layer.
  - `mlx4_en` Ethernet-specific functionality of the card and connects TCP/IP networking mid-layer to ethernet functionality of hardware. Technically part of the OS Mid-Layer.
- `mlx5` main low-level drivers for ConnectX-4 family of cards and above. Functionality is split into the following modules:
  - `mlx5_core` Low-level hardware and firmware functionality as well as some extended functionality specific to the hardware and firmware (such as initialization after a device reset). Also implements Ethernet functionality, meaning no mlx5 version of mlx4_en.
  - `mlx5_ib` Similar to `mlx4_ib` provides IB-specific functionality of device and connects IB-specific parts of mid-layer to IB functionality of hardware. Technically part of the OS Mid-Layer.
- `mlxfw` Main module enabling flashing firmware on mellanox CAs

# THE REST OF THIS IS UNDER CONSTRUCTION

#### OS-Mid Layer

##### Terminology and Initialisms

- _Communications Manager_ (CM) Provides services needed to allow clients to establish connections
- _Subnet Administrator Client_ (SA) Provides functionality allowing clients to communication with an SM
- _Subnet Management Agent_ (SMA) Responds to SM communications enabling the SM to query and configure the devices on each host
- _Management Datagram Services_ (MAD) Standard messaging format for SM-SMA communications
- _Perforamce Management Agent_ (PMA) Responds to SM communications (thus MAD packets) to enable hardware performance counter retrieval
- _Subnet Management Interface_ (SMI) generic interface for client-SM communications, typically QP 0
- _General Services Interface_ (GSI) generic interface for client-SM or client-client communications, typically QP 1, and often used for SA purposes as well as other device management operations such as IO device management, CM, Performance Management, etc

##### Modules

- `ib_core` core verbs modules
- `ib_cm` IB-specific connection management 
- `rdma_cm` RDMA-specific connection management
- `ib_sa` IB subnet administration helpers
- `ib_mad` IB subnet administration helper dealing with MADs
- `ib_umad` IB subnet administration helper dealing with MADs from userspace

#### ULP



#### Kernel Implementation 

- Core software is logically grouped into 5 major areas, HCA Resource Management, Memory Management, Commection Management, Work Request and Completion Event Processing, and Subnet Administration.
- Please note many seemingly duplicate functions exist which start with `ib_*` and `rdma_*`; a lot of the `rdma_*` functions obfuscate some of the pointer management and programmatic steps for you but often perform the same functionality.

##### Resource Management

- _IB Client_ 
  - An entity that uses IB resources (and in particular the IB devices) must register as an _IB client_ via `ib_register_client`
  - Its parameter, `ib_client`, must contain call backs which fire when IB devices are added or removed from the system 
  - When an IB client is completely done, IE going through tear down or termination, the client calls `ib_unregister_client`
- Query and Modify Info 
  - `ib_query_device` allows an IB client to retrieve attributes for a hardware device such as hardware capabilities and limitations (E.G. max size for queue pairs)
  - `ib_query_port` gathers port-specific info for an IB hardware device such as MTU, port state, its LID, and the SM's LID
  - `ib_query_pkey` gathers PKey info for the port
  - `ib_modify_device` and `ib_modify_port` changes some device or port-specific info such as indicating presence of a connection manager. Typically not used by any ULPs but could be used by Mid-Layer stuff.
- PDs
  - As noted above, PDs provide access control by associating IB resources such as QPs and MRs within a single domain of trust, and the client which allocates the PD can grant access to the PD to other nodes on the fabric in order to send and receive via the QPs contained within.
  - `ib_alloc_pd` and `ib_dealloc_pd` allocates and deallocates a PD respectively
- Address Handles
  - Similar to TCP/IP, the IB stack needs address handles to organize and reference local and remote node info to send and receive packets to/from
  - `rdma_create_ah` creates a address handle 
  - `rdma_modify_ah` and `rdma_query_ah` can change or query info about an address handle
  - `rdma_destroy_ah` deallocates an address handle
  - Many other functions wrap around the noted functions above (such as `ib_create_ah_from_wc` and `ipoib_create_ah`) but some implement their owen address handler stuff (such as `mlx4_ib_create_ah` which does not call down to `rdma_create_ah`)


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
- [Programmer Sought](https://www.programmersought.com/article/24218148942/)
43
- [Verbs Programming Tutorial](https://www.csm.ornl.gov/workshops/openshmem2013/documents/presentations_and_tutorials/Tutorials/Verbs%20programming%20tutorial-final.pdf)
44
- [InfiniBand: An Introduction + Simple IB verbs program with RDMA Write](https://blog.zhaw.ch/icclab/infiniband-an-introduction-simple-ib-verbs-program-with-rdma-write/)
45
- [OpenSM](https://docs.mellanox.com/display/MLNXOFEDv531001/OpenSM)
- [Subnet Manager](https://docs.mellanox.com/display/MLNXOSv381000/Subnet+Manager)
- [HPC Networks: Infiniband](http://people.cs.pitt.edu/~jacklange/teaching/cs1652-f19/lectures/infiniband.pdf)
