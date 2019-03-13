# Subnet Manager

### Overview

- Each infiniband network needs to have a subnet manager
- This can be some RHEL box with `opensm` installed and running or a dedicated switch
    - Alternatively, two infiniband nodes can just be back to back without an SM
- `sminfo` provides subnet manager information this system is currently connected to
