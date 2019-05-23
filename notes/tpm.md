# Trusted Platform Module (TPM)

Notes on how the TPM works and how to interact with it.

### Overview

* The TPM is a chip on the mobo/CPU
* Standards are defined by the Trusted Computing Group (TCG)
* Provides storage for keys (blogs of encrypted data) and hashes
* Note it does not encrypt/decrypt but rather just provides the keys used for encryption/decryption
* Keys in a TPM:
    *

### General Use

##### Packages

* Interactions with the TPM from a shell are largely facilitated via `tpm-tools` and arbitrated via trousers (`tcsd`), so it will need to be installed (`sudo yum install tpm-tools tcsd`)
    * Not to be confused with `tpmtools` which performs similar actions to a degree
* Apparently `tpm-tools` communicates over `127.0.0.1` for connections with the TPM (as revealed via `strace`)
* Fortunately, the TPM infrastructure is largely built into the kernel now
    * The required modules are, apparently, `tpm`, `tpm_tis`, and `tpm_bios` however these are all built into the kernel now

##### Hardware Enablement

* Interactions with the TPM also require the TPM to be enabled via the BIOS. If not enabled, `/dev/tpm0` will _not_ exist and interactions will thus fail
* If unsure if the TPM is discovered on boot, check `dmesg`:
```
# dmesg | grep tpm
[    0.957701] tpm_tis 00:05: 1.2 TPM (device-id 0x1, rev-id 1)
```

##### Software Enablement

* Trouser will need to be running in order to use TPM:
```
# systemctl enable tcsd
# systemctl restart tcsd
```
* On initialization, a TPM will be associated with an "owner" which is simply a user with a password used to interact with the TPM.
* Interactions with the TPM are checked against a TPM-internal key which is checked against the owner. If the password is unknown, then the TPM will need to be cleared which will reset the password _and invalidate any keys therein_.
    * If resetting the TPM fails from the OS layer, you will need to clear it within the BIOS.
* After clearing the TPM, the TPM will need an owner. Again, the owner is just the user with the password which allows interactions with the TPM, so taking ownership is effectively just setting the passwords.
* Once the passwords are set, the TPM needs to be enabled and active. I currently _**assume**_ this is similar to making a service active and enabled in LINUX as I can not find details about this.
* To perform the above, run the following:
```
# tpm_clear  # clears the tpm (obviously)
Enter owner password:
TPM Successfuly Cleared.  You need to reboot to complete this operation.  After reboot the TPM will be in the default state: unowned, disabled and inactive.
# tpm_takeownership
Enter owner password:
Confirm password:
Enter SRK password:
Confirm password:
# tpm_setenable --enable
Enter owner password:
Disabled status: false
# tpm_setactive
Enter owner password:
Persistent Deactivated Status: false
Volatile Deactivated Status: false
```

##### Documentation

* `/usr/share/doc/kernel-doc-3.10.0/Documentation/security/`

##### Troubleshooting

`Tspi_Context_Connect failed: 0x00003011 - layer=tsp, code=0011 (17), Communication failure`
  * Typically means `tcsd` is not running
  * `systemctl start tcsd` usually resolves the issue

`Tspi_TPM_TakeOwnership failed: 0x00000007 - layer=tpm, code=0007 (7), TPM is disabled`
  * Typically means the TPM is disabled.
  * `tpm_setenable` usually resolves the issue

`TPM Error:0x9a2` 
  * Means the owner, endorsement, and/or locked out is not NULL 
  * TPM will need to be reset from the hardware
  * If unsure, run `tpm2_getcap -c properties-variable` (not sure if this pertains to tpm < 2.0)
