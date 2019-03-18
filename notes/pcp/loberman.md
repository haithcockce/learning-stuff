# Loberman Training 

- To kill `pmlogger`, you need to both disable it and comment out the cron job for it. 
- Defaults to 14 days of logs. To change this, change `/etc/cron.d/pcp-pmlogger`
- Naming conventions: <DATE>.<TIME>.<#> -> YYYYMMDD.HH.MM.N. Creating logs is limited to a specific size for each archive file. If data collection surpasses this size limit before rotating out to the next tiem unit, a new archive is created and each of these are numbered by N. 
- `/var/log/pcp/pmlogger/<HOSTNAME>/pmlogger.log` indicates about how much space the logs will consume. 
