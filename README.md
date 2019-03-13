# Lessons
### Small scripts and programs to learn about Linux

A set of small scripts and programs which help me learn about the kernel and 
Linux as a whole. 

#### Structure
* `src/`   The source code for all the lessons
* `bin/`   The executables/modules if compiled
* `books/` Books and manuals on various things like x86 and Infiniband specifications
* `notes/` Notes taken from readings and the like 

#### Lessons
* `eattmpfsmem` Learn how memory usage is reported when writing to a `tmpfs`
                    filesystem
* `hugepage_reserve` Learn how memory usage is reported when writing to 
                         hugepages
* `timer_module` Learn how timers work and learn how modules are compiled
                     and work

#### TODO
* Script this stuff
* Makefile with targets for each lesson
* Split apart `timer_module`
