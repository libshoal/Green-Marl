#!/bin/bash

# if [ yes != "$STDBUF" ]; then
#   echo "Disabling buffer .. "
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
    echo "options: supported are: -h for hugepages, -d for distribution, -r for replication, -p for partitioning"
    echo "-d: run in GDB"
    echo "-n: do NOT run sanity checks"
    echo "-b for Barrelfish"
    echo <<EOF
Options are:
-h Huge page support
-d Distribute
-r Replicate
-p partition
EOF
    exit 1
}


BASE=$(readlink -e $(dirname $0)/../)
WORKLOAD_BASE=$(readlink -e $BASE/../graphs/)
ARRAY_SETTINGS_FILE=$BASE/local_array_settings.lua

echo "Base Directory: $BASE"

#WORKLOAD=$BASE/../graphs/huge.bin
#WORKLOAD=$BASE/../graphs/soc-LiveJournal1.bin
#WORKLOAD=$BASE/../graphs/big.bin
#WORKLOAD=/mnt/scratch/skaestle/graphs/twitter_rv-in-order-rename.bin
#WORKLOAD=/mnt/scratch/skaestle/graphs/soc-Live

#
# Run on Barrelfish
# -------------------------------------------------
RUN_ON_BARRELFISH=0
BARRELFISH_BASE=$(readlink -e $BASE/../../)


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

WORKLOAD=$WORKLOAD_BASE/$4.bin

BARRELFISH_PROGRAM=""
    case $1 in
        pagerank)
            BARRELFISH_WORKLOAD=GreenMarl_PageRank
            shift
            ;;
#       hop_dist)
#           SHL_DISTRIBUTION=1
#           shift
#           ;;
#       triangle_counting)
#           SHL_REPLICATION=1
#           shift
#           ;;
        *)
            error "Cannot find barrelfish program [$1]"
    esac



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


if [[ "$5" == "-b" ]]; then
    echo "Running on Barrelfish"
    RUN_ON_BARRELFISH=1
    shift
fi

CONCAT_SETTINGS=0
if [[ "$5" == "-l" ]]; then
    CONCAT_SETTINGS=1
    shift
fi

if [[ $RUN_ON_BARRELFISH -eq 0 ]]; then
    [[ -f "${WORKLOAD}" ]] || error "Cannot find workload [$WORKLOAD]"
    [[ -f ${INPUT} ]] || error "Cannot find program [$INPUT]"
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

if [[ $RUN_ON_BARRELFISH -eq 0 ]]; then
    [[ -n "$AFF" ]] || error "Affinity not set for machine [$(hostname)]"
fi

# --------------------------------------------------
# CONFIGURATION
# --------------------------------------------------
export SHL_HUGEPAGE
export SHL_REPLICATION
export SHL_DISTRIBUTION
export SHL_PARTITION
# --------------------------------------------------


# final version of the file
SETTINGS_FILE="$BASE/scripts/shl__settings.lua"

# where does those come from
SHL_TRIM=1
SHL_STATIC=1
SHL_STRIDE=4096

echo "Generating Global settings file..."
$BASE/scripts/generate_settings.py -D $SHL_DISTRIBUTION -R $SHL_REPLICATION -P $SHL_PARTITION \
                                   -H $SHL_HUGEPAGE     -T $SHL_TRIM        -T $SHL_STATIC \
                                   -W $SHL_STRIDE       -o $SETTINGS_FILE

if [[ $CONCAT_SETTINGS -eq 1 ]]; then
    if [[ -f $ARRAY_SETTINGS_FILE ]]; then
        echo "Concatenating Local settings to global settings"
        cat $ARRAY_SETTINGS_FILE >> $SETTINGS_FILE
    else
        echo -n -e $txtylw "No settings file present" $txtrst
    fi
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
        echo $BARRELFISH_BASE

        pushd $BARRELFISH_BASE
        mkdir -p $BARRELFISH_BASE/results

        # move the settings file into the build directory
        mv $SETTINGS_FILE $BARRELFISH_BASE/build

        tools/harness/scalebench.py -v -t $BARRELFISH_WORKLOAD -m babybel1 -e $BARRELFISH_BASE/build $BARRELFISH_BASE $BARRELFISH_BASE/results; SC_RC=$?

        if [[ $SC_RC -ne 0 ]]; then
            echo "scalebench failed"
            rm -rf $BARRELFISH_BASE/results
            rm -rf $SETTINGS_FILE
            exit 1
        fi
            
        popd

        # producing the output directory folder
        BARRELFISH_RESULTS_DIR=$BARRELFISH_BASE/results/$(ls $BARRELFISH_BASE/results) # get the $results/year directory
        BARRELFISH_RESULTS_DIR=$BARRELFISH_RESULTS_DIR/$(ls $BARRELFISH_RESULTS_DIR) # get the benchmark results directory
        echo $BARRELFISH_RESULTS_DIR

        echo "extracting results..."
        echo ${BARRELFISH_RESULTS_DIR}/raw.txt

        $BASE/scripts/extract_result.py -workload ${WORKLOAD} -program ${INPUT} -barrelfish 1 -rawfile ${BARRELFISH_RESULTS_DIR}/raw.txt; RC=$?

        # cleanup test results
        rm -rf $BARRELFISH_BASE/results
        # rm -rf $$ARRAY_SETTINGS_FILE

        if [[ $SC_RC -ne 0 ]]; then
            echo "result wrong"
            exit 1
        fi
        
        exit 0
    else
        # Start benchmark
        GOMP_CPU_AFFINITY="$AFF" SHL_CPU_AFFINITY="$AFF" \
                 stdbuf -o0 -e0 -i0 ${INPUT} ${WORKLOAD} ${NUM} ${INPUTARGS} $@ | $CHECKS

        # remove the settings file
        rm -rf $SETTINGS_FILE

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
        #   error "GM program failed"
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
