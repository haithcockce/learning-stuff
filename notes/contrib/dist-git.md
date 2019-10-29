1. `kinit`
2. Add `http://download-node-02.eng.bos.redhat.com/rel-eng/internal/rcm-tools-fedora.repo` as a repo
3. `sudo dnf install rhpkg`
4. `rhpkg clone <pkg>`
5. `cd <pkg>`
6. `git checkout $(rhpkg gitbuildhash <PACKAGE>-<VERS>)`
7. `rhpkg --dist rhel-7.3 prep`
8. `rhpkg switch-branch <rel>`
