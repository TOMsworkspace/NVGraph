export PIN_ROOT=
mkdir -p obj-intel64
make DEBUG=1 obj-intel64/nvgraph_tracer.so

#./pin-3.22/pin -t obj-intel64-pin-3.22/nvgraph_tracer.so -s 1000000 -t 1000000000 -- ~/project/libtorch-example/autograd/build/autograd

#./pin-3.22/pin -t obj-intel64-pin-3.22/nvgraph_tracer.so -s 10000000 -t 5000000 -o gcn-cora-5M-- /usr/bin/python /home/tyj/project/pytorch_geometric/examples/gcn.py
