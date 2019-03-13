# Our Networking Versioning is Weird

To the untrained eye that is. 

We do not ship drivers in the same format as upstream is - that is, if they have been told to change to a specific driver version, that version numbering scheme does not necessarily apply to the Red Hat kernel's driver version in any way.
 
As an example:
 
- Upstream developers produce a `driver` with the version of 1.0.
- Red Hat pulls in version 1.0 and supports it.
- Upstream developers produce version 1.1
- Upstream developers produce version 1.2
- Upstream developers produce version 1.3 and release it
- Red Hat pulls in _upstream_ version 1.3 and labels it version 1.1 in our source, as it is +.1 to the current version we ship.
 
Therefore, changing module numbers as reported by the driver is not helpful to us when comparing any values - what would be more helpful is if they provide us with a specific bug, or a kernel commit, or something we can reference to see if we have the fix they are looking for in any versions. We also package our drivers with the kernel only, so it will require a whole kernel upgrade.

----------------------------------------

A) This appears to be a misunderstanding of how Red Hat ships drivers, and our versioning scheme. We do not simply pull the latest version from upstream and backport it; we pull various patches and increment the version on a totally different scheme than what may be reflected in the Linux kernel, and subsequently, in Cisco's shipped version of the driver, as a a whole.
 
To provide an example, assume that the upstream kernel provides a version of a driver, called 1.0. We then pull in this driver, which now makes the Red Hat version 1.0.
 
At this moment, the driver versions match. Then assume that the linux kernel upstream adds four patches, incrementing the version to 1.4. When Red Hat pulls the same patches, they may do it collectively, indicating that all four patches are part of the same release; our Red Hat supplied driver, packaged directly with the kernel, is now 1.1, a stark contrast to 1.4 upstream.
 
Therefore, there is no way by 'versioning numbers' that we can guarantee a specific version.
 
B) Cisco is a partner with us, particularly with UCS; we have plenty of open communication paths outside of TSAnet to discuss this. Particularly, the Cisco Partner TAM should likely be made aware of this to determine the best course of action, as there is probably some discussion between Cisco engineering and our own to determine the proper versioning for firmware of NIC's, if it is causing a problem. TSAnet is intended to solve mutual customer problems, not to necessarily ensure we are following each other's standards.
 
C) Is there something that explicitly lists an incompatibility? Firmware version and driver differences can certainly matter, but is there an actual problem the customer is experiencing, or are they making sure it works before an upgrade? In this event, again, we may not have a 'compatibility matrix' solution for them as we do not determine the driver version the same; our drivers are always shipped with the kernel and are incremented accordingly saved for the appropriate re-bases. The reasoning for this is _every_patch_ must then be tested, instead of just throwing whatever is the latest from upstream/Cisco without any Red Hat verification.

