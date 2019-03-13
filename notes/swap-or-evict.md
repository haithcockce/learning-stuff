# When to determine if we swap or evict pages

1. When going into direct page reclaim, we have the following code path (roughly):

  * `mm/vmscan.c:do_try_to_free_pages()` --> `shrink_zones()` --> `shrink_zone()` --> `shrink_lruvec()`
  * `shrink_lruvec()` is a per-zone freer for memory and works across all LRUs for a specific zone

2. Within `shrink_lruvec()`, `get_scan_count()` is called.

  * `get_scan_count()` works to count the amount anon and filebacked pages in the zone's LRU
  * Then we grab the priority of each anon and file pages. Anon prio is swappiness while file prio is 200 - swappiness
  * Then we calculate the amount of pages to scan from each anon and file lru. To scan = (prio * recently scanned) / rotated for each anon and file lru

3. After we get the amount to scan in `nr`, we loop until all is drained.

  * `nr` is `nr[NR_LRU_LISTS]`. This should be length of 4:
    * `nr[0]` inactive anon pages to scan
    * `nr[1]` active anon pages to scan
    * `nr[2]` inactive file pages to scan
    * `nr[3]` active file pages to scan
  * We loop until `nr[0]`, `nr[3]`, and `nr[2]` are 0.
  * For each LRU, if the LRU has pages to scan, grab a number to scan (min{remaining pages, `SWAP_CLUSTER_MAX`}) and call `shink_list` with the amount to scan.

4. In `shrink_list`, if the LRU we are currently working with is an active LRU, `shrink_active_list` is called, __likely__ just rotating pages and moving pages to the inactive LRU.

  * `shrink_active_list`__ _moving pages to the inactive LRU is an assumption_ __ I will need to come back to this eventually.
  * First, we add a drain function to the cpu we are on for the lru we are currently working with (I think)
  * From here, we determine what we are allowed to do with the current LRU and extract a number of pages from the LRU to a separate list to reduce lock contention via `isolate_lru_pages()`
  * From this bundle of isolated pages, we flush from the bundle via `shrink_page_list()`.
  * Leftover pages are put back

5. Flushing occurs within `shrink_page_list()`

  * `lru_to_page()` grabs a page and removes the lru reference in the page and increment the amount of pages scanned
  * From here, we increment various other counts, such as the number dirty, unqueued, dirty, and congested, etc based on how the page is traversing the LRU. This is the scanning stage.
    * If a page is ready for writeback, but we have cycled the LRU and see it again, then the block device is potentially having problems. Just mark it as already viewed and continue
    * If a page is not marked for immediate reclaim, but also does not have IO activity associated with it, mark it for immediate reclaim
  * If the page is anon memory and is not part of any swap cache and is not `__GFP_IO`, then attempt to allocate swap space for it and add the page to the swap mapping.
  * Then work to unmap the page from various processes attached to it.
  * If the page is dirty, part of file cache, and written back, mark it as reclaimed
  * A lot more happens to determine various states of the page and moving it around to keep or remove it as appropriate.
  * Once all the determining finishes, flush the tlb
  with `try_to_unmap_flush()`
  * Then flushing occurs by taking teh `free_pages` list we populated and giving it to `free_hot_cold_page_list()`. This walks the list and checks migration status to try and free a page with `free_one_page()`
