# Zones and Nodes

### Set up of zones and nodes in the kernel

- Every numa node has a corresponding `pglist_data` structure 
- All nodes are stored in the list, `pglist_data node_data[N]`. 
   - Populated node indexes are their indexes into the list
   - For example, node 4 will be at `node_data[4]`
   - _Note_ for some architectures (like ppc), the nodes are not
     contiguous (so you can have nodes 0, 1, 3, 4, 6, 7, etc) so
     the list will contain empty nodes at the indexes where there 
     is no node (continuing the example above, `node_data[2]` will
     have an empty `pglist_data` struct)
- In the `pglist_data.node_zones` are the **zones** within the node.
- Each node will have the same amount of entries in this list to 
  represent the different possible zones in that node that can exist 
  (for example, DMA, DMA32, Normal, Movable, Device)  
- Similar to above, only the zones that exist will be populated 
  with data in this list. 
   - For example, only 1 DMA and only 1 DMA32 zone can exist on a
     system in x86, so those will be populated in only one node but
     none of the others
