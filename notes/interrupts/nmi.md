```
Understand the nature of external NMIs is such that a platform error (something on HPE's motherboard) generated a signal to the firmware.  The firmware needs logic to determine if it should: correct it, send an SMI, send an IRQ or send an NMI.  In this case it choose an NMI.  This means the firmware (HPE) _knows_ where the signal came from.  However, the firmware does _not_ have a register in which the kernel can poke to read that _reason_.  So the kernel is left in the dark and the kernel has to guess or poke around post-mortem to see what generated the signal, which is hard when HPE has secret registers and chips.

On the flip side, the industry was so frustrated by this they developed an ACPI spec called GHES to provide a standard framework to report these NMI reasons to the kernel (and a class of errors).  Unfortunately, this HP box does not support GHES.

HPE explained that on G9 the hpwdt will not provide any useful sourcing of the NMI as that is now done via AHS which only HPE can review.

So did the customer provide an AHS log for HPE to review?  If so, what info did it provide?
```
