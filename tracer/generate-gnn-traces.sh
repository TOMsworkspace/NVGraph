#!/bin/bash

tracespath="/home/tyj/project/Pythia/traces/"
pinbin="/home/tyj/project/Pythia/tracer/pin-3.22/pin"
pintool="/home/tyj/project/Pythia/tracer/obj-intel64-pin-3.22/champsim_tracer.so"
python="python3"
pygpath="/home/tyj/project/pytorch_geometric/examples"

#cora
# gcnout="gcn_cora_1000M"
# gatout="gat_cora_1000M"
# graphsageout="graphsage_cora_1000M"

#citeseer
# gcnout="gcn_citeseer_1000M"
# gatout="gat_citeseer_1000M"
# graphsageout="graphsage_citeseer_1000M"

#PubMed
gcnout="gcn_pubmed_1000M"
gatout="gat_pubmed_1000M"
graphsageout="graphsage_pubmed_1000M"

#IMDB_BINARY
# ginout="gin_IMDB_BINARY_1000M"

#REDDIT_BINARY
ginout="gin_REDDIT_BINARY_1000M"

#PROTEINS
# ginout="gin_PROTEINS_1000M"

#echo ${gcnout}

skip_inst=1000000
coll_inst=1000000000

#collect 1000M instructions, skip 1000M instructions

if [ ! -f ${tracespath}/${gcnout}.xz ]; then
    if [ ! -f ./${gcnout} ]; then
        echo "generating traces for gcn in 1000M instructions."
        ${pinbin} -t ${pintool} -s ${skip_inst} -t ${coll_inst} -o ${gcnout} -- ${python} ${pygpath}/gcn.py
    fi
    xz -z ./${gcnout}
    mv ./${gcnout}.xz ${tracespath}/
    echo "move gcn trace to .. ${tracespath}/${gcnout}.xz"
else
    echo "gcn traces exist!!"
fi



if [ ! -f ${tracespath}/${gatout}.xz ]; then
    if [ ! -f ./${gatout} ]; then
        echo "generating traces for gat in 1000M instructions."
        ${pinbin} -t ${pintool} -s ${skip_inst} -t ${coll_inst} -o ${gatout} -- ${python} ${pygpath}/gat.py
    fi
    xz -z ./${gatout}
    mv ./${gatout}.xz ${tracespath}
    echo "move gat trace to .. ${tracespath}/${gatout}.xz"
else
    echo "gat traces exist!!"
fi



if [ ! -f ${tracespath}/${graphsageout}.xz ]; then
    if [ ! -f ./${graphsageout} ]; then
        echo "generating traces for grapgsage in 1000M instructions."
        ${pinbin} -t ${pintool} -s ${skip_inst} -t ${coll_inst} -o ${graphsageout} -- ${python} ${pygpath}/graph_sage_unsup.py
    fi
    xz -z ./${graphsageout}
    mv ./${graphsageout}.xz ${tracespath}
    echo "move graphsage trace to .. ${tracespath}/${graphsageout}.xz"
else
    echo "graphsage traces exist!!"
fi



if [ ! -f ${tracespath}/${ginout}.xz ]; then
    if [ ! -f ./${ginout} ]; then
        echo "generating traces for gin in 1000M instructions."
        ${pinbin} -t ${pintool} -s ${skip_inst} -t ${coll_inst} -o ${ginout} -- ${python} ${pygpath}/pytorch_lightning/gin.py
    fi
    xz -z ./${ginout}
    mv ./${ginout}.xz ${tracespath}
    echo "move gin trace to .. ${tracespath}/${ginout}.xz"
else
    echo "gin traces exist!!"
fi