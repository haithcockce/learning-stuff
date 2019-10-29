# Checking memory leaks

Presented By Nick Yelle

18 Sept 2019

`slub_debug=U` - This will track all allocations and frees in a `track` structure. A track will include a PID and stack trace of the allocation and the free.

`alloc_page` comes with ftrace points

`bcc` useful in rhel 8 for memory tracing
