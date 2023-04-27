#!/bin/bash
#
#
#
# Traces:
#    gcn_citeseer_1000M
#
#
# Experiments:
#    nopref: --warmup_instructions=50000000 --simulation_instructions=150000000 --config=$(PYTHIA_HOME)/config/nopref.ini
#    spp: --warmup_instructions=50000000 --simulation_instructions=150000000 --l2c_prefetcher_types=spp_dev2 --config=$(PYTHIA_HOME)/config/spp_dev2.ini
#    bingo: --warmup_instructions=50000000 --simulation_instructions=150000000 --l2c_prefetcher_types=bingo --config=$(PYTHIA_HOME)/config/bingo.ini
#    mlop: --warmup_instructions=50000000 --simulation_instructions=150000000 --l2c_prefetcher_types=mlop --config=$(PYTHIA_HOME)/config/mlop.ini
#    pythia: --warmup_instructions=50000000 --simulation_instructions=150000000 --l2c_prefetcher_types=scooby --config=$(PYTHIA_HOME)/config/pythia.ini
#
#
#
#

#/home/tyj/project/NVGraph/bin/perceptron-multi-multi-no-ship-4core --warmup_instructions=5000000 --simulation_instructions=15000000 --l2c_prefetcher_types=cbp --config=/home/tyj/project/NVGraph/config/cbp.ini  -traces /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz > gcn_citeseer_1000M_cbp.out 2>&1
/home/tyj/project/NVGraph/bin/perceptron-multi-multi-no-ship-4core --warmup_instructions=5000000 --simulation_instructions=15000000 --l2c_prefetcher_types=rlp --config=/home/tyj/project/NVGraph/config/rlp.ini  -traces /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz /home/tyj/project/Pythia/traces/gcn_citeseer_1000M.xz > gcn_citeseer_1000M_rlp.out 2>&1
