# Overview of PCIe

- Apparently the activity works like tcp. There's a transmitter, receiver, and packets are sent between the two
- Similar to TCP, each packet has a sequence number and packets are retransmitted in a replay buffer if the transmitter receives NACKs or nothing within a replay timer
- If a correctable receiver error is constantly happening, then it is likely something to do with the signal quality (hardware or otherwise)

- http://pciexpress-datalinklayer.blogspot.com/
- https://lists.gt.net/linux/kernel/2250177
- http://xillybus.com/tutorials/pci-express-tlp-pcie-primer-tutorial-guide-1
https://www.design-reuse.com/articles/38374/pcie-error-logging-and-handling-on-a-typical-soc.html
