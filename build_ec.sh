#!/bin/bash
(cd shoal/shoal && make) || exit 1
pushd /home/skaestle/projects/gm/apps/output_cpp/src;
set -x
g++  -Wno-unused-variable -O3 -g -DVERSION=\"76f6-dirty\" -Wall -I/home/skaestle/projects/gm//shoal/contrib/numactl-2.0.9 -I/home/skaestle/projects/gm//shoal//shoal//inc -I/home/skaestle/projects/gm//shoal/contrib/pycrc -I/home/skaestle/projects/gm//shoal/contrib/papi-5.3.0/src -I../generated -I../gm_graph/inc -I. -fopenmp -DDEFAULT_GM_TOP="\"/home/skaestle/projects/gm\"" -std=gnu++0x -DAVRO ../generated/hop_dist_ec.cc hop_dist_main.cc  ../gm_graph/lib/libgmgraph.a -L../gm_graph/lib -lgmgraph -L/home/skaestle/projects/gm//shoal/contrib/numactl-2.0.9 -lnuma -L/home/skaestle/projects/gm//shoal/contrib/papi-5.3.0/src -lpapi -L/home/skaestle/projects/gm//shoal/contrib/papi-5.3.0/src/libpfm4/lib -lpfm -L/home/skaestle/projects/gm//shoal//shoal/ -lshl -o ../bin/hop_dist_ec
EXIT=$?
set +x
popd
exit $EXIT