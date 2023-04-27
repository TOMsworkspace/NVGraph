#!/bin/bash

echo 0 > /proc/sys/kernel/nmi_watchdog



cd /home/tyj/project/pytorch_geometric/examples

perf stat -B -e mem_load_retired.l1_hit,mem_load_retired.l1_miss,mem_load_retired.l2_hit,mem_load_retired.l2_miss,mem_load_retired.l3_hit,mem_load_retired.l3_miss,\
cache-references,cache-misses,cycles,instructions,branches,branch-misses,faults,migrations,context-switches,task-clock /home/tyj/anaconda3/envs/PYG/bin/python3 pytorch_lightning/gin.py



echo 1 > /proc/sys/kernel/nmi_watchdog


#
# cache miss rate event
# l1: mem_load_retired.l1_hit, mem_load_retired.l1_miss
# l2: mem_load_retired.l2_hit, mem_load_retired.l2_miss
# l3: mem_load_retired.l3_hit, mem_load_retired.l3_miss