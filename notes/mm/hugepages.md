you can control hugepage pool
size on each individual numa node via the sysfs interface:
    `/sys/devices/system/node/node[N]/hugepages/hugepages-2048kB/nr_hugepages`
