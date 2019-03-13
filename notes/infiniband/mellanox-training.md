# Mellanox Training

### DPDK: Data Plane Development Kit

- Fast Packet Processing
- Hardware will DMA only from/to memory pages which are owned by this process
- Selective packet processing; DPDK can process what it wants and defer ARP,
  DHCP, etc packet processing to the kernel
- Depends only on rdma-core and not our modules vs. OFED modules
- A lot of OFED stuff is now in 7.5 since customers want as in house as possible

##### Userspace

- Interacts with hardware directly rather than context switch
  - No data copy between kernel and userspace
- Single task per core, so no context switching
- Polling mode in networking as well as interrupt driven work
- Without direct access, you have to use infiniband verbs (libibverbs)

### OVS

- Open VSwitch
- BIOS virt on, hugepages enabled with hugepage size of 1GiB

### Troubleshooting

- `testpmd> show port xstats 0`
