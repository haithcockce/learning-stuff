# Building Kernels

### Building test kernels 

#### Making the patch

0. Make changes to the source as appropriate
  - Yes really just start making the changes 
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

#### Setting up the build directory 

1. `mkdir <location to clone rhpkg source to>`
2. `rhpkg clone kernel`
3. `cd kernel/` This directory will have _only_ a README file however `git tag` should show kernel versions
4. `git checkout -b private-chaithco-sfdc<CASE NO>-<RELEASE> kernel-<VERSION>` for example: 
  - `git checkout -b private-chaithco-sfdcCASENOHERE-rhel-7.4 kernel-3.10.0-693.11.6.el7`
  - `sfdc<CASE NO>` can also be `bz<BZ NUMBER>`
5. Put the patch in the build directory: `cp ~/source/rhel7/redhat/linux-kernel-test.patch ~/source/testkernels/CASENOHERE/rhel7/kernel`

#### Setting up the kernel rpm 

1. In `kernel.spec`, include the build id to be the case or bz number. For example: 

```
%define buildid .CASENOHERE
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

#### Building 

1. `git add linux-kernel-test.patch kernel.spec`
2. `git commit -m 'perf: Fix a race between ring_buffer_detach() and ring_buffer_wakeup()'`
3. `git push -u origin private-chaithco-sfdcCASENOHERE-rhel-7.4`
4. `brew list-targets | less` will list all the targets. They don't always match between distros...
5. `rhpkg scratch-build --target rhel-7.4-z-test --arches x86_64 noarch`


### Building kernels for engineering

#### Setup your tree

- Make a new tree for the build, so `mkdir the-tree` or whatever
- `git init`
- Grab the code for the tree we are fixing: `git remote add rhel6.10.z git://git.app.eng.bos.redhat.com/rhel-6.10.z.git`
- Pull the tags so you can pull the code: `git fetch --all --tags` then `git checkout kernel-2.6.32-754.12.1.el6`
- Then add the upstream tree as a remote: `git remote add upstream git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`
- Pull upstream's tags as well: `git fetch upstream`

#### Adding the change 

- Grab the tree version and put it in your own branch: `git checkout -b bz1690048 kernel-2.6.32-754.12.1.el6`
- `git cherry-pick -esx <upstream hash>`
  - If it conflicts (and it likely will because kernel is hard), the conflict will be embedded into the file and looks like this:

```
<<<<<<< HEAD
My cool code
=======
the code I'm trying to pull in
>>>>>>> githasha23444029834
```

  - Above you will need to resolve the conflict. __NOTE__ the actual conflict may not even reside in the stuff above (such as if the order of logic changes)
  - If the merge conflicts, then `git cherry-pick --continue` to finish the merge
- If the cherry pick magically doesn't conflict, then you will need to merge! `git commit` 
  - __Note__ the `git commit` and `git cherry-pick --continue` are mutually exclusive 
