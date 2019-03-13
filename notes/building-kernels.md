# Building Test Kernels

### Making the patch

0. Make changes to the source as appropriate
1. See the changes with `git diff`
2. In the root directory, `make rh-test-patch`: 

```
 $ make rh-test-patch
make -C redhat rh-test-patch 
make[1]: Entering directory '/home/chaithco/source/rhel7/redhat'
BUILDID is ".test". Update '/home/chaithco/source/rhel7/localversion' to change.
TEST is "None". Update '/home/chaithco/source/rhel7/tests' to change.
make[1]: Leaving directory '/home/chaithco/source/rhel7/redhat'
```

3. Confirm the patch exists in `redhat/linux-kernel-test.patch`

### Setting up the build directory 

1. `mkdir <location to clone rhpkg source to>`
2. `rhpkg clone kernel`
3. `cd kernel/` This directory will have _only_ a README file however `git tag` should show kernel versions
4. `git checkout -b private-chaithco-sfdc<CASE NO>-<RELEASE> kernel-<VERSION>` for example: 
  - `git checkout -b private-chaithco-sfdc02061301-rhel-7.4 kernel-3.10.0-693.11.6.el7`
  - `sfdc<CASE NO>` can also be `bz<BZ NUMBER>`
5. Put the patch in the build directory: `cp ~/source/rhel7/redhat/linux-kernel-test.patch ~/source/testkernels/02061301/rhel7/kernel`

### Setting up the kernel rpm 

1. In `kernel.spec`, include the build id to be the case or bz number. For example: 

```
%define buildid .02061301
```

2. Add the following to the description section: 

```
%description
The kernel package contains the Linux kernel (vmlinuz), the core of any
Linux operating system.  The kernel handles the basic functions
of the operating system: memory allocation, process allocation, device
input and output, etc.

This RPM has been provided by Red Hat for testing purposes only and is
NOT supported for any other use. This RPM may contain changes that are
necessary for debugging but that are not appropriate for other uses, 
or that are not compatible with third-party hardware or software. This
RPM should NOT be deployed for purposes other than testing and                                                         
debugging.
```

3. Update the change log: 

```
%changelog
* Wed May 16 2018 Charles Haithcock <chaithco@redhat.com> [3.10.0-693.11.6.el7.02061301]
- [perf] perf: Fix a race between ring_buffer_detach() and ring_buffer_wakeup()                                        

* Thu Dec 28 2017 Denys Vlasenko <dvlasenk@redhat.com> [3.10.0-693.11.6.el7]
- [x86] spec_ctrl: Eliminate redundant FEATURE Not Present messages (Andrea Arcangeli) [1519795 1519798] {CVE-2017-5715
- [x86] mm/kaiser: init_tss is supposed to go in the PAGE_ALIGNED per-cpu section (Andrea Arcangeli) [1519795 1519798] 
- [x86] spec_ctrl: svm: spec_ctrl at vmexit needs per-cpu areas functional (Andrea Arcangeli) [1519795 1519798] {CVE-20
```

### Building 

1. `git add linux-kernel-test.patch kernel.spec`
2. `git commit -m 'perf: Fix a race between ring_buffer_detach() and ring_buffer_wakeup()'`
3. `git push -u origin private-chaithco-sfdc02061301-rhel-7.4`
4. `brew list-targets | less` will list all the targets. They don't always match between distros...
5. `rhpkg scratch-build --target rhel-7.4-z-test --arches x86_64 noarch`
