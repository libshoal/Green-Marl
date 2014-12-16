#!/bin/bash

# if [ yes != "$STDBUF" ]; then
# 	echo "Disabling buffer .. "
#     STDBUF=yes /usr/bin/stdbuf -i0 -o0 -e0 "$0" $@
#     exit $?
# fi

txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtylw='\e[0;33m' # Yello
txtrst='\e[0m'    # Text Reset

function error() {
	echo $1
	exit 1
}

function usage() {
    echo "Usage: $0 <options> {pagerank,hop_dist,triangle_counting} <num_threads> {ours,theirs} {huge,soc-LiveJournal1,twitter_rv,big} [-n] [-d] [-b]"
	echo ""
	echo "options: supported are: -h for hugepages, -d for distribution, -r for replication, -p for partitioning, -b for Barrelfish"
	echo "-d: run in GDB"
	echo "-n: do NOT run sanity checks"
	echo <<EOF
Options are:
-h Huge page support
-d Distribute
-r Replicate
-p partition
EOF
    exit 1
}


BASE=$(dirname $0)/../
WORKLOAD_BASE=$BASE/../graphs/

#WORKLOAD=$BASE/../graphs/huge.bin
#WORKLOAD=$BASE/../graphs/soc-LiveJournal1.bin
#WORKLOAD=$BASE/../graphs/big.bin
#WORKLOAD=/mnt/scratch/skaestle/graphs/twitter_rv-in-order-rename.bin
#WORKLOAD=/mnt/scratch/skaestle/graphs/soc-Live

# 
# Run on Barrelfish
# -------------------------------------------------
RUN_ON_BARRELFISH=0
BARRELFISH_BASE=$BASE


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
		-p)
			RUN_ON_BARRELFISH=1
			shift
			;;			
		*)
			parse_opts=0
	esac
done




[[ -n "$3" ]] || usage

NUM=$2

[[ "$3" == "ours" ]] || [[ "$3" == "theirs" ]] || error "Cannot parse argument #3"

if [[ "$3" == "ours" ]]; then
	INPUT=$BASE/apps/output_cpp/bin/$1
	INPUTARGS=" /dev/null"
fi

if [[ "$3" == "theirs" ]]; then
	INPUT=$BASE/../org_gm/apps/output_cpp/bin/$1
	INPUTARGS=" 100 0.001 0.85 -GMMeasureTime=1"
fi

[[ -f ${INPUT} ]] || error "Cannot find program [$INPUT]"

WORKLOAD=$WORKLOAD_BASE/$4.bin

BARRELFISH_PROGRAM=""
	case $1 in
		pagerank)
			BARRELFISH_WORKLOAD=GreenMarl_PageRank
			shift
			;;
#		hop_dist)
#			SHL_DISTRIBUTION=1
#			shift
#			;;
#		triangle_counting)
#			SHL_REPLICATION=1
#			shift
#			;;
		*)
			error "Cannot find barrelfish program [$1]"
	esac


	

[[ -f "${WORKLOAD}" ]] || error "Cannot find workload [$WORKLOAD]"

DEBUG=0
if [[ "$5" == "-d" ]]; then
	DEBUG=1
	shift
fi

CHECK=1
if [[ "$5" == "-n" ]]; then
	CHECK=0
	shift
fi

shift
shift
shift
shift

if [[ $DEBUG -ne 1 ]]; then
	echo "Executing program [${INPUT}] with [$NUM] threads"
	echo "Loading workload from [${WORKLOAD}]"
fi


AFF=""

# --------------------------------------------------
# sgs-r815-03
#
# use lscpu to find out how core IDs are mapped to virtual threads. If
# we use only n threads, where n is the number of physical CPUs, we do
# NOT want to use hyperthreads.
# --------------------------------------------------
if [[ $(hostname) == "sgs-r815-03" ]]; then

	# Generated by scripts/cpuinfo.py
	AFF="0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,48,49,50,51,52,53,54,55,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63,24,25,26,27,28,29,30,31"

fi
# --------------------------------------------------
# bach / sgs-r820-01 / ETH laptop
# --------------------------------------------------
if [ \( $(hostname) == bach* \) -o \
     \( $(hostname) == babybel \) -o \
     \( $(hostname) == emmentaler2 \) -o \
     \( $(hostname) == "skaestle-ThinkPad-X230" \) -o \
     \( $(hostname) == "sgs-r820-01" \) ]; then

    COREMAX=$(($NUM-1))
    AFF="0-${COREMAX}"
fi

[[ -n "$AFF" ]] || error "Affinity not set for machine [$(hostname)]"

# --------------------------------------------------
# CONFIGURATION
# --------------------------------------------------
export SHL_HUGEPAGE
export SHL_REPLICATION
export SHL_DISTRIBUTION
export SHL_PARTITION
# --------------------------------------------------

LUAFILE="settings.lua"

if [[ ! -f $LUAFILE ]]; then
	touch $LUAFILE
fi

res=0
if [[ $DEBUG -eq 0 ]]; then

	echo "Sourcing $BASE/env.sh"
	. $BASE/env.sh
	set -x

	echo $LD_LIBRARY_PATH

	# Checks enabled?
	CHECKS="$BASE/scripts/extract_result.py -workload ${WORKLOAD} -program ${INPUT}"
	if [[ $CHECK -ne 1 ]]; then CHECKS="cat"; fi

    if [[ $RUN_ON_BARRELFISH -eq 1 ]]; then
    	pushd $BARRELFISH_BASE
    	mkdir -p $BARRELFISH_BASE/results
    	BARRELFISH_RESULTS_DIR=$BARRELFISH_BASE/results/$(ls $BARRELFISH_BASE/results) # get the $results/year directory
    	BARRELFISH_RESULTS_DIR=$BARRELFISH_RESULTS_DIR/$(ls $BARRELFISH_RESULTS_DIR) # get the benchmark results directory

    	# TODO: find machine name

    	tools/harness/scalebench.py -v -t $BARRELFISH_WORKLOAD -m nos5 -e /mnt/local/acreto/barrelfish/build . $BARRELFISH_BASE/results

    	# TODO: results directory
    	mv $BARRELFISH_RESULTS_DIR/raw.txt TODO_RESULT_LOCATION

    	$BASE/scripts/extract_result.py -workload ${WORKLOAD} -program ${INPUT} -barrelfish 1 -rawfile ${barrelfish}/raw.txt

    	# cleanup test results
    	rm -rf $BARRELFISH_BASE/results

    	popd

    else
    	# Start benchmark
		GOMP_CPU_AFFINITY="$AFF" SHL_CPU_AFFINITY="$AFF" \
			stdbuf -o0 -e0 -i0 ${INPUT} ${WORKLOAD} ${NUM} ${INPUTARGS} $@ | $CHECKS

		# bash is sooo fragile!
		R=( "${PIPESTATUS[@]}" )

		GM_RC="${R[0]}"
		ER_RC="${R[1]}"
    fi

	# extract result return code
	if [[ $ER_RC -ne 0 ]]; then
	    error "Execution was unsuccessful"
		exit 1
	else
		# GM return code

		# Since we use a Pipe, we need to check the return code of the first program as well

		echo -n -e $txtylw "[ IGNORING GM RC - SEGFAULT ]" $txtrst
		# XXX Ignoring GM seg faults for now
		# if [[ $GM_RC -ne 0 ]]; then
		# 	error "GM program failed"
		# fi

	    echo "Execution was successful"
	    exit 0
	fi
else
	. $BASE/env.sh
	GOMP_CPU_AFFINITY="$AFF" SHL_CPU_AFFINITY="$AFF" \
		gdb $SK_GDBARGS --args ${INPUT} ${WORKLOAD} ${NUM} ${INPUTARGS} $@
fi

exit 1
