# Intro to Infiniband and RDMA

### What?

- _Remote Direct Memory Access_ (or RDMA) is a transport protocol between two or more systems with a focus on High Performance Computing (HPC) environments. In its most basic setup, RDMA offers:
  - Kernel bypass; sending and receiving of data is done similar to simply reading from some area of memory (with a couple more moving parts to facilitate such) exposed directly to the application in question and thus does not require entering kernelspace to perform these operations
  - Zero-copy; most transport protocols/stacks require some intermediary area of memory to hold data where data is put first and then copied to the network interface device (for egress traffic) or copied into the socket buffers for the process (for ingress traffic). RDMA reads and writes directly from the areas of memory used for transport.
  - Minimize CPU involvement; most transport protocols require some CPU intervention (typically via a softirq) to move data off of or onto the network adapter. When sending data via RDMA, the data will be placed directly into remote memory. 
  - Message-oriented; most networking stacks use a stream-style transfer wherein the data stream is segmented into discrete units for transfer (called _packets_ in TCP/UDP). For RDMA's message-oriented approach, the data is sent as a single logical unit. If need be, the backing hardware will need to segment the data. 

- _Infiniband_ is the network fabric and link-layer flow control
  - As RDMA requires very tight deadlines with massive throughput and still be lossless, typical networking fabrics will not be able to handle RDMA (by itself).

- _Verbs_ the API to setup and manage interconnects or _channels_.
- _HCA_ Host Channel Adapter, dedicated hardware which enables the RDMA protocol and operates like a Network Interface Card specialized for RDMA.

### How does it work? 

1. Via the verbs, create an infiniband context 
  - The application must first _register the memory_ which denotes an area of memory within the application as dedicated to RDMA operations.
  - Upon registering the memory for RDMA communications, the HCA creates a channel from itself to the area of memory.
2. Once the HCA is opened, the application establishes a connection.
  - 


# References

- [Mellanox Whitepaper](https://www.mellanox.com/pdf/whitepapers/Intro_to_IB_for_End_Users.pdf) ELI5 Overview
- [Mellanox Whitepaper](https://www.mellanox.com/pdf/whitepapers/IB_Intro_WP_190.pdf) More technical explanation of IB stack
- [Mellanox Linux User Manual](https://www.mellanox.com/related-docs/prod_software/Mellanox_OFED_Linux_User_Manual_v4_3.pdf)
- [HPC Avisory Council Slide Deck](https://www.hpcadvisorycouncil.com/pdf/Intro_to_InfiniBand.pdf) Odds and Ends on terminology (like MAD, GMP, SMP, etc)
- [Intro to Infiniband Architecture](http://www.buyya.com/superstorage/chap42.pdf) Overview of the queue pairs
- [iWARP vs. RoCE](https://www.snia.org/sites/default/files/ESF/RoCE-vs.-iWARP-Final.pdf) and a [youtube](https://www.youtube.com/watch?v=nGTY14UptOA) version of the same thing.
- [Kernel Implementation](https://www.kernel.org/doc/ols/2005/ols2005v2-pages-279-290.pdf) Dated, but relevant kernel implementation 
- [Introduction to Programming Infiniband RDMA [sic]](https://insujang.github.io/2020-02-09/introduction-to-programming-infiniband/)
