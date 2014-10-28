#!/bin/bash

PREFIX=$HOME/projects/gm
SHLPREFIX=$PREFIX/shoal/
export LD_LIBRARY_PATH=$SHLPREFIX/shoal:$SHLPREFIX/contrib/numactl-2.0.9:$SHLPREFIX/contrib/papi-5.3.0/src:$SHLPREFIX/contrib/papi-5.3.0/src/libpfm4/lib
