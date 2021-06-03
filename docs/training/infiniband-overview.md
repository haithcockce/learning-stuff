# Intro to Infiniband and RDMA

### Overview

- _Remote Direct Memory Access_ (or RDMA) is a transport protocol between two or more systems with a focus on High Performance Computing (HPC) environments. In its most basic setup, RDMA offers:
  - Kernel bypass; sending and receiving of data is done similar to simply reading from some area of memory (with a couple more moving parts to facilitate such) exposed directly to the application in question and thus does not require entering kernelspace to perform these operations
  - Zero-copy; most transport protocols/stacks require some intermediary area of memory to hold data where data is put first and then copied to the network interface device (for egress traffic) or copied into the socket buffers for the process (for ingress traffic). RDMA reads and writes directly from the areas of memory used for transport.
  - Minimize CPU involvement; most transport protocols require some CPU intervention (typically via a softirq) to move data off of or onto the network adapter. When sending data via RDMA, the data will be placed directly into remote memory. 



# References

- [Mellanox Whitepaper](https://www.mellanox.com/pdf/whitepapers/Intro_to_IB_for_End_Users.pdf) ELI5 Overview
- [Mellanox Whitepaper](https://www.mellanox.com/pdf/whitepapers/IB_Intro_WP_190.pdf) More technical explanation of IB stack
- [HPC Avisory Council Slide Deck](https://www.hpcadvisorycouncil.com/pdf/Intro_to_InfiniBand.pdf) Odds and Ends on terminology (like MAD, GMP, SMP, etc)
- [Intro to Infiniband Architecture](http://www.buyya.com/superstorage/chap42.pdf) Overview of the queue pairs
- [iWARP vs. RoCE](https://www.snia.org/sites/default/files/ESF/RoCE-vs.-iWARP-Final.pdf) and a [youtube](https://www.youtube.com/watch?v=nGTY14UptOA) version of the same thing.
- [Kernel Implementation](https://www.kernel.org/doc/ols/2005/ols2005v2-pages-279-290.pdf) Dated, but relevant kernel implementation 
