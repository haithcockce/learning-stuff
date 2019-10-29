# Intro to Openshift 3

- A collection of nodes where a _cluster_ has a _master_ node that manages the rest of the nodes
- **Pod** a single scheduling unit containing one or more containers, a virtualized network device, internal IP Addresses, ports, persistent storage, etc. These can be replicated for horizonal scaling
- Master node runs master services such as API and auth, scheduling, mgmt and replication, etcd, etc
- Nodes run kubelets and kube-proxy daemons
