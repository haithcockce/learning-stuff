# Intro to Infiniband and RDMA
# TODO
- Get a few machines and perform some operations to see what's up in perf and what not
- Troubleshooting steps. Get loberman's feedback

### Table Of Contents

- [What Is Infiniband And RDMA](#what-is-infiniband-and-rdma)
- [Communication Overview](#communication-overview)
  - [Node To Node Communications](#node-to-node-communications)
  - [Application To Channel Adapter Communications](#application-to-channel-adapter-communications)
- [Network Organization and Management Overview](#network-organization-and-management-overview)
  - [Addressing](#addressing)
  - [Subnet Management](#subnet-management)
  - [Communications Management](#communications-management)
- Everything after this is under construction

## What Is Infiniband And RDMA

- _Remote Direct Memory Access_ (or RDMA) is a transport protocol between two or more systems with a focus on High Performance Computing (HPC) environments. In its most basic setup, RDMA offers:
  - Kernel bypass; sending and receiving of data is done similar to simply reading from some area of memory (with a couple more moving parts to facilitate such) exposed directly to the application in question and thus does not require entering kernelspace to perform these operations
  - Zero-copy; most transport protocols/stacks require some intermediary area of memory to hold data where data is put first and then copied to the network interface device (for egress traffic) or copied into the socket buffers for the process (for ingress traffic). RDMA reads and writes directly from the areas of memory used for transport.
  - Minimal CPU involvement; most transport protocols require some CPU intervention (typically via a softirq) to move data off of or onto the network adapter. When sending data via RDMA, the data will be placed directly into remote memory.
  - Message-oriented communications; most networking stacks use a stream-style transfer wherein the data stream is segmented into discrete units for transfer (called _packets_ in TCP/UDP). For RDMA's message-oriented approach, the data is sent as a single logical unit. If need be, the backing hardware will need to segment the data. For stream-style communications, the software logically segments the data.

- _Infiniband_ (IB) is the network fabric and link-layer flow control
  - As RDMA requires very tight deadlines with massive throughput and still be lossless, typical networking fabrics will not be able to handle RDMA (by itself).
  - IB operates as the L2 or link layer between nodes and any additional infrastructure between RDMA applications (such as the physical network fabric adapters and whatnot)
  - In nearly every scenario, IB fabric adapters can work as IB devices or Ethernet devices for interoperability.

- _Verbs_  is an interface for application stacks to use in order to facilitate RDMA and Inifniband communications and setup _channels_
  - Conceptually similar to an API, but multiple implementations exist in the world.
  - The userspace portions/interfaces are often referred to as _uverbs_
  - Verbs are divided logically into "control path" (manage RDMA/IB resources) and "data path" (using RDMA/IB resources to send/receive data)
  - At a high level, via the verbs interface, we use the control path/command channel to setup and create our Infiniband resources (such as the queues noted below) and use the data path/data channels to send and receive data.

- As implied above, Infiniband and RDMA provide a whole new networking stack which can be very specifically tailored to particular HPC setups.
  - Below is a sort of comparison between the IB and IP network stacks

<img align="center" src="https://github.com/haithcockce/learning-stuff/blob/master/docs/training/media/ib_layers_colored.png?raw=true" style="max-width:100%;">

- In order to handle userspace/hardware interaction directly while also providing network management, the internal bits of Infiniband can be divided somewhat into three somewhat distinct layers:
  - _Upper Layer Protocols_ (ULP) are application stack-specific protocols and libraries. For example, networked filesystems or networked storage solutions built on top of RDMA/IB would have libraries and operations at this layer.
  - _Mid-Layer_ or functionality largely arbitrated and initialized by the OS in question to manage the underlying IB devices and IB network.
  - Hardware-specific drivers responsible for hardware dependent operations such as device initialization and teardown as well as reading device capabilities.

- Below is a heavily abstracted diagram detailing some of the above info

<img align="center" src="https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/media/tcp-vs-rdma.jpg" style="max-width:100%;">

- **Note** Be prepared for a _lot_ of initialisms. The userspace and kernelspace implementations of RDMA/IB use the initialisms within their code extensively, so knowing the initialisms helps in reading the code.

## Communication Overview

### Node To Node Communications

- RDMA encapsulates actions to perform some operation (such as sending or receiving data) as a _Work Request_ (WR), where completion of this is referred to as a _completion_.
- Work Requests are organized into queues generically referred to as _Work Queues_ (WQ);
  - _Send Queues_ (SQ) are Work Queues responsible for sending data
  - _Receive Queues_ (RQ) are Work Queues responsible for receiving data
  - Send and Receive queues are created in pairs called _Queue Pairs_ (QP) to establish node-to-node communication
- When a Work Request is queued to a Work Queue, the Work Request is first encapsulated into a _Work Queue Entity_ (WQE)
  - Queueing a Work Queue Entity to either Work Queue is referred to as _posting_
  - Work Queue Entities are differentiated as _Send Request_ (SR) when posted to a Send Queue and _Receive Request_ (RR) when posted to a Receive Queue
- _Shared Receive Queues_ (SRQ) provides a scalable method of receiving work
  - Consuming IB traffic sent to you in order to pass it to the application in question requires posting a Receive Request to the Receive Queue
  - Posting Receive Requests is an atomic operation. For heavy ingress traffic, this atomic operation introduces an immediate bottleneck when addressing said heavy ingress activity.
  - A Shared Receive Queue maps to 1 or more Queue Pairs (and thus 1 or more Receive Queues)
  - A linked list of Receive Requests can be posted as a single atomic operation to a Shared Receive Queue for multiple Receive Requests to allow for scalability
- _Work Completions_ are entities of data containing info about the completion of some request
  - As noted above, upon completion of posting some Work Request, a Work Completion may be generated
  - If a Work Completion is created from some action, it is queued to a _Completion Queue_ (CQ) as a _Completion Queue Entity_ (CQE)
  - A Send Request may not end with a Completion Queue Entity
  - A Receive Request always ends with a Completion Queue Entity
- During setup of IB structures, a _Local Key_ (L_key) and _Remote Key_ (R_key) are generated for security in accessing memory regions (more on all this in "Application-To-Channel Adapter Communication")
- Communication types
  - While IB communication types span reliable-connected, unreliable-connected, reliable-datagram, and unreliable-datagram, most clients only use _unreliable datagrams_ (UD) or _reliable connected_ (RC) modes. More recent hardware uses only UD.
  - UD operates similar to UDP in TCP/IP addressing; packets are sent by the server node without receiving acknowledgement from the client node.
  - RC operates similar to TCP; packets are sent by the server node and require an acknowledgement by the client.
  - **Note** [Connected mode is not an option](https://access.redhat.com/solutions/5960861) in some scenarios:
    - ConnectX-4 devices and above has Enhanced IPoIB enabled which forces use of UD mode and can not be disabled
    - RHEL 7 provided a kernel module parameter `ipoib_enhanced` for the module `ib_ipoib` to optionally disable Enhanced IPoIB and allow connected mode, however this was not added upstream meaning RHEL 8 and above can not disable Enhanced IPoIB (and thus are required to use UD mode)

<img align="center" src="https://github.com/haithcockce/learning-stuff/blob/master/docs/training/media/queue_pair.png?raw=true" style="max-width:100%;">

- **Note** The above is the most primative aspects of the RDMA/IB communication mechanisms. A lot of additional things are built on top of this, but communications between nodes ultimately fall into some form of the above scheme.

- For managing the network topography and node communications, IB devices often have a number of services and agents/clients which interact with said services built into the device itself.
  - The services range from general network layout management, establishing and maintaining node-to-node communications, monitoring performance on devices and so on.
  - The IB devices, for the most part, arbitrate the services and agents, while other software in the OS and/or in userpspace use these capabilities to perform communications between nodes and the like.
    - You can think of this as futexes in the kernel; the kernel arbitrates futexes, however, the application using the futexes needs to manage proper use thereof.
    - You can also think of this as the kernel providing the means for two applications to communicate to each other over NICs via IP Addressing and ports and the like. The applications in question, however, must interact with these facilities in order to actually communicate over the NICs.



### Application To Channel Adapter Communications

As noted earlier, RDMA allows direct communication with hardware from userspace. Several entities exist to facilitate direct hardware communication.

- _Host Channel Adapter_ (HCA), dedicated hardware which enables the RDMA protocol and operates like a Network Interface Card specialized for RDMA. Often referred to as just Channel Adapter (CA).
- Via the verbs interface, create an _Infiniband Context_, a structure which aggregates info about an IB device.
  - Userspace version `ibv_context` aggregates device and device operations
  - Kernelspace version, `ib_ucontext` maintains the kernelspace bits of the context such as what verbs can be associated with the device.
- Create a _Protection Domain_ (PD), a simple structure referenced across several other IB/RDMA structures to associate structures (IE which queue pairs belong with which devices and the like)
  - Userspace version `ibv_pd` more or less just contains a pointed to the relevant `ibv_context`
  - Kernelspace version `ib_pd` contains the relevant `ib_ucontext` as well as R_key and L_key stuff (more on that in a moment)
- Register a _Memory Region_ (MR), an area of memory designated to perform various operations or use as the buffer to read from or write to another node
  - An MR is an aggregate structure containing pointers to relevant IB contexts, protection domains, the starting address of the region of memory, how large the region is, and L/R_Keys
  - Userspace version `ibv_mr` contains all the above
  - Kernelspace version `ib_mr` contains all the above, but also includes a possible list of queue pairs and includes some additional things for low-level management stuff (such as what the page size for memory is)
  - On registering, an L_key (local key) and R_key (remote key) are generated and associated with the memory region
    - The L_key enables authenticating access to a memory region on the local system.
    - The R_key can be sent to remote systems and enable authenticating access to a local memory region from the remote system (IE performing an RDMA read/write)
- Create Send and Receive Completion Queues as well as a Queue Pair (overview thereof can be found in Node-To-Node Communications)
  - Userspace version `ibv_qp` aggregates an individual send and receive queue respectively, the relevant IB context and protection domain, identifiers and locks for synchronous access, a queue pair state descriptor and such
  - Kernelspace version `ib_qp` aggregates similar things, but points to the IB device directly rather than a context. It also includes pointer and info handling architecture and implementation-details (such as an event handler, security, max read and write sizes available for the queue pair, etc)
  - Once created, the completion queues and queue pairs need to be assigned to each other and initialized.
- _Reminder_ The setup actions are performed via the verbs. For most implementations, this is all provided from the `libibverbs` package.



## Network Organization and Management Overview

### Addressing

- Addressing in IB is conceptually similar to IP addressing but with different conventions and names
- _Local Identifiers_ (LID)
  - 16-bit number used within a subnet by the switch for routing locally within the subnet
    - If you're familiar with IP Addressing, this is similar to your Private Network Addressing
    - IE this is similar to 192.168.9.*, 172.16.0.*, and 10.0.0.* networks
  - Dynamically assigned at runtime
  - Info is embedded into Local Routing Header part of an IB packet for Link Layer routing
- _Global Unique Identifiers_ (GUID)
  - Similar to a MAC address, they are assigned by vendor
  - 64-bit IEEE-deﬁned identifiers for elements in a subnet (such as an end node, port, switch, or multicast group)
- _Global Identifiers_ (GID)
  - Modeled after IPv6, 128 bits
  - The GUID provides the lower 64 bits of the GID
  - Used for routing across subnets via the Global Routing Header (GRH) in an IB packet for Network Layer routing
- _Partition Keys_ (PKeys)
  - Similar to a VLAN, PKeys can group IB ports/nodes all physically connected together into subnetworks that may or may not communicate with each other
  - PKeys also define levels of membership within partitions, where a "Full" member may communicate with all nodes of the partition while "Limited" can only communicate with Full members
  - PKeys are 16-bit hex values where the most significant bit defines Full (1) or Limited (0) membership and the remaining 15 bits provide a PKey identifier
  - IB Ports can be a member of multiple partitions, and the switch will always create a default partition for an entire subnet (`0x7fff` as its ID)

### Subnet Management

- _General Services Manager_ (GSM) a class of managers which communicate over the _General Services Interface_ (GSI, QP1) with _General Services Agents_ (GSA) to perform some actions not associated with subnet management.
- _Management Datagrams_ (MAD) a dedicated type of packet used for communicating between Managers and Agents.
  - Listed because, like most components of IB, a userspace entity can register a client to monitor and respond to MADs, thus creating a userspace portion, `umad`.
  - For example, `ibping`, similar to `ping` but performing only at the Link Layer, requires use of umad device files to communicate
- _Subnet Manager_ (SM) main software to manage a network of IB nodes (like a router on a regular TCP/IP network)
  - Can run on a dedicated IB switch or simply as an application on a system in the network.
    - This implies a dedicated IB hardware switch is not necessary to run in an IB network but is often found in larger HPC clusters.
    - When running as an application on generic hardware, usually runs via `opensm`.
  - An SM discovers and configures all the IB fabric devices to enable traffic flow between those devices.
  - An SM applies network traffic related configurations such as Quality of Service (QoS), routing, and partitioning of the fabric devices.
  - Multiple SMs can reside on a single network
  - Can segment a network via PKeys, very similarly to a VLAN.
  - _Subnet Manager Agent_ (SMA) entity built into the IB device which responds to the SM
    - Main entity which provides IB device info for the SM
    - Communicates over the _Subnet Manager Interface_ (SMI), QP0, with SM
    - Communicates via MADs
- _Subnet Administrator_ (SA) the subnet database built by the SM containing records of paths and channel adapter info.
  - Managed and updated by the SM.
  - An SA exists for each SM on the network
  - Clients query the SA for info on paths.
  - The SM regularly scans the network for changes to the fabric and updates the SA accordingly if a change is found
  - _Subnet Administrator Client_ enables SA communications via MADs
- _Performance Manager_ maintains hardware performance counters. Queried via the _Perforamce Management Agent_ (PMA). The PMA Responds to SM communications (thus MAD packets) to enable hardware performance counter retrieval.

### Communications Management

- _Communications Manager_ (CM) is a type of GSM which operates specifically for establishing and maintaining connections between nodes
  - The manager that sits on the IB device that the kernel communicates with
  - Performs connection establishment as a 3-way handshake (like TCP/IP);
    - System operates like a server and listens for incoming connections
    - Clients send a connection request to server
    - Server will reply with an accept or reject (ACK or NACK in TCP/IP)
    - Client responds to the accept with a _Ready-To-Use_ (RTU) message
  - CM is responsible for timing out and retying the above requests if needed
  - CM services can also be brought into userspace via `ib_ucm` should you want to register your own CM
    - [`ibacm`](https://linux.die.net/man/1/ibacm) is an example service in userspace which helps map IB endpoints to names/addresses and caching such info
- Communication IDs
  - Instances of connections are tracked via Communication IDs
  - A communication ID is associated with a QP, the RDMA route (which includes the SA Path Record), an IB device, and an event handler called whenever a communication event happens
  - Communication IDs operate conceptually similar to a socket in that a socket describes a combination of a local IP address to communication over and a remote IP address to communicate to. The main conceptual difference is communications can be asynchronous and communications must be explicitly bound to an IB device before listening/communication (a socket can be bound via `INADDR_ANY` wherein you do not care about a specific device to listen to and will accept connections over any device)



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

##### Modules

- `ib_core` core IB module providing functionality around core IB activity, such as function callback when an IB event occurs, device registration and enumeration, client registration to respond to IB activity, etc.
- `ib_cm` and `rdma_cm` Connection management, such as CM IDs; creating, maintaining, and destroying communications with other nodes; querying GID and LID info, etc
- `ib_sa` IB subnet administration helpers
- `ib_mad` Functionality do manage MADs and offer kernelspace hooks for custom agents/services to respond to MADs
- `ib_umad` Functionality dealing with enabling MAD management from userspace

##### Services

**Note** Pretty much all the OS-Mid Layer functions are described in [Network Organization and Management Overview](#network-organization-and-management-overview)

- Subnet Manager Agent (SMA)
- Subnet Administrator (SA) Client
- Management Datagram (MAD) handlers
- Communication Manager (CA) and its agent (CMA)
- _iWARP_ a networking protocol that implements RDMA for efficient data transfer over IP networks
  - Not an acronym (for some reason)
  - Still utilizes full TCP/IP stack while applications implement RDMA protocols
  - Essentially, RDMA activity is encapsulated into packets with TCP and Ethernet headers for network transport.
  - iWARP extensions to TCP/IP eliminates TCP/IP stack processing (as its offloaded to RDMA-enabled NIC), zero-copy like regular IB, and can post data from userspace directly to NIC like IB
  - Requires dedicated hardware on nodes, but switches do not need dedicated hardware as the traffic is still encapsulated within Ethernet/TCP headers wherein flow control and congestion management is done with existing Ethernet/TCP portions of switches
- _RDMA over Converged Ethernet_ (RoCE)
  - RDMA but over pure Ethernet (v1) or pure UDP and Ethernet (v2)
  - No TCP flow control/congestion management (because it uses UDP instead of TCP)

<img align="center" src="https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/media/roce_vs_iwarp.png" style="max-width:100%;">

#### ULP

- _Internet Protocol over Infiniband_ (IPoIB) almost the reverse of iWARP/RoCE, IP protocol data and such gets encapsulated into IB packets and set through IB fabric
- _Sockets Direct Protocol_ (SDP) RDMA-accelerated alternative to the TCP protocol on IP. The goal is to do this in a manner which is transparent to the application.
  - _Note_ uses `SOCK_STREAM` so can be used for basic unix sockets like a pipe or as a TCP/IP replacement
- _SCSI RDMA Protocol_ (SRP) allows direct access from one node to another node's SCSI devices via RDMA
- _iSCSI Extensions for RDMA_ (iSER) extensions to the iSCSI protocol to enable transport via RDMA.
  - These still are provided over TCP or iWARP and use existing Ethernet devices and fabric.
  - Despite going through TCP/IP layers, data is still transferred directly into/out of SCSI buffers (zero copy) and with little CPU intervention
- _Reliable Datagram Sockets_ (RDS) connectionless and record-oriented protocol over IB and RoCE.
  - RDS is a generic and transport-independent protocol
  - Currently can perform RDS over IB only, though RDS over TCP should be enabled in the future
- _Network Filesystem over RDMA_ (NFSoRDMA) as the name implies, NFS over RDMA and currently requires RoCE

<img align="center" src="https://raw.githubusercontent.com/haithcockce/learning-stuff/master/docs/training/media/ib_diagram.png" style="max-width:100%;">


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
- [Infiniband Network Architecture](https://github.com/haithcockce/learning-stuff/blob/master/books/InfiniBand%20Network%20Architecture.pdf)
