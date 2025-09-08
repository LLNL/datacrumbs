# Notes

## Set Cap

```bash
cp $(pwd)/bin/datacrumbs /tmp/ # cannot run cap on nfs. We need file on a file system which supports file attributes.
sudo setcap 'cap_bpf,cap_perfmon+ep' /tmp/datacrumbs 
/tmp/datacrumbs "lead" "--user" "haridev" "--config_path" "/home/haridev/datacrumbs/etc/datacrumbs/configs" "--data_dir" "/home/haridev/datacrumbs/etc/datacrumbs/data" "--trace_log_dir" "/home/haridev/datacrumbs/etc/datacrumbs/logs" "--inclusion_path" "/tmp"
```