```bash
 $ perf report -i ./perf.data.1527 --stdio --pid 16507  --show-cpu-utilization 2>/dev/null
 $ perf report -i perf.data.1527 --stdio --sort overhead_sys,comm --show-cpu-utilization 2>/dev/null | grep -e ora -e Child | head -20

```
