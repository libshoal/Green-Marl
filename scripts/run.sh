#!/bin/bash

# if [ yes != "$STDBUF" ]; then
# 	echo "Disabling buffer .. "
#     STDBUF=yes /usr/bin/stdbuf -i0 -o0 -e0 "$0" $@
#     exit $?
# fi

function error() {
	echo $1
	exit 1
}

function usage() {
    echo "Usage: $0 <options> {pagerank,hop_dist,triangle_counting} <num_threads> {ours,theirs} {huge,soc-LiveJournal1,twitter_rv,big}"
	echo ""
	echo <<EOF
Options are:
-h Huge page support
-d Distribute
-r Replicate
-p partition
EOF
    exit 1
}

# Parse options
# --------------------------------------------------

SHL_HUGEPAGE=0
SHL_REPLICATION=0
SHL_PARTITION=0
SHL_DISTRIBUTION=0

parse_opts=1

while [[ parse_opts -eq 1 ]]; do
	case $1 in
		-h)
			SHL_HUGEPAGE=1
			shift
			;;
		-d)
			SHL_DISTRIBUTION=1
			shift
			;;
		-r)
			SHL_REPLICATION=1
			shift
			;;
		-p)
			SHL_PARTITION=1
			shift
			;;
		*)
			parse_opts=0
	esac
done


#WORKLOAD_BASE=/run/shm/
BASE=/home/skaestle/projects/gm/
WORKLOAD_BASE=$BASE/../graphs/

#WORKLOAD=$BASE/../graphs/huge.bin
#WORKLOAD=$BASE/../graphs/soc-LiveJournal1.bin
#WORKLOAD=$BASE/../graphs/big.bin
#WORKLOAD=/mnt/scratch/skaestle/graphs/twitter_rv-in-order-rename.bin
#WORKLOAD=/mnt/scratch/skaestle/graphs/soc-Live

[[ -n "$3" ]] || usage

NUM=$2

[[ "$3" == "ours" ]] || [[ "$3" == "theirs" ]] || error "Cannot parse argument #3"

if [[ "$3" == "ours" ]]; then
	INPUT=$BASE/apps/output_cpp/bin/$1
	INPUTARGS=""
fi

if [[ "$3" == "theirs" ]]; then
	INPUT=$BASE/../org_gm/apps/output_cpp/bin/$1
	INPUTARGS=" 100 0.001 0.85 -GMMeasureTime=1"
fi

[[ -f ${INPUT} ]] || error "Cannot find program [$INPUT]"

WORKLOAD=$WORKLOAD_BASE/$4.bin

[[ -f "${WORKLOAD}" ]] || error "Cannot find workload [$WORKLOAD]"

DEBUG=0
if [[ "$5" == "-d" ]]; then
	DEBUG=1
	shift
fi

shift
shift
shift
shift

echo "Executing program [${INPUT}] with [$NUM] threads"
echo "Loading workload from [${WORKLOAD}]"


AFF=""

# --------------------------------------------------
# sgs-r815-03
#
# use lscpu to find out how core IDs are mapped to virtual threads. If
# we use only n threads, where n is the number of physical CPUs, we do
# NOT want to use hyperthreads.
# --------------------------------------------------
if [[ $(hostname) == "sgs-r815-03" ]]; then

    echo "Running on sgs-r815-03"
    if [[ $NUM -gt 32 ]]
    then
	COREMAX=$(($NUM-1))
	AFF="0-64"
    else
	COREMAX=$(($NUM*2-1))
	AFF="0-${COREMAX}:2"
    fi
fi
# --------------------------------------------------
# bach
# --------------------------------------------------
if [ \( $(hostname) == bach* \) -o \
    \( $(hostname) == "sgs-r820-01" \) ]; then

    echo "Running on bach"
    COREMAX=$(($NUM-1))
    AFF="0-${COREMAX}"
fi

[[ -n "$AFF" ]] || error "Affinity not set for machine"

# --------------------------------------------------
# CONFIGURATION
# --------------------------------------------------
export SHL_HUGEPAGE
export SHL_REPLICATION
export SHL_DISTRIBUTION
export SHL_PARTITION
# --------------------------------------------------

res=0
if [[ $DEBUG -eq 0 ]]; then
	. $BASE/env.sh
	set -x
	GOMP_CPU_AFFINITY="$AFF" SHL_CPU_AFFINITY="$AFF" \
		stdbuf -o0 -e0 -i0 ${INPUT} ${WORKLOAD} ${NUM} ${INPUTARGS} $@ | $BASE/scripts/extract_result.py -workload ${WORKLOAD}

	GM_RC=${PIPESTATUS[0]}

	if [[ $? -ne 0 ]]; then
	    error "Execution was unsuccessful"
	else

		# Since we use a Pipe, we need to check the return code of the first program as well
		if [[ $GM_RC -ne 0 ]]; then
			error "GM program failed"
		fi

	    echo "Execution was successful"
	    exit 0
	fi
else
	. $BASE/env.sh
	set -x
	GOMP_CPU_AFFINITY="$AFF" SHL_CPU_AFFINITY="$AFF" \
		gdb --args ${INPUT} ${WORKLOAD} ${NUM} $@
fi
