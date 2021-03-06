#!/bin/bash

#set -x

PROG=pagerank
OPTS=

WORKLOAD=twitter_rv
#WORKLOAD="soc-LiveJournal1"

txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtrst='\e[0m'    # Text Reset

# Print error message
function error() {
    echo -e $txtred$1$txtrst
    exit 1
}


NUM_RUNS=3
RUN_ON_BARRELFISH=0
BARRELFISH=
if [[ "$1" == "-b" ]]; then
	# --------------------------------------------------
	# BARRELFISH
	# --------------------------------------------------

    echo "Running on Barrelfish"
    RUN_ON_BARRELFISH=1
    BARRELFISH="-b"
    PROGRAMS="pagerank hop_dist triangle_counting"
    OPTIONS=("" "-p" "-r" "-r -p" "-a" "-p -a" "-r -a " "-r -p -a")
    BARRELFISH_WORKLOAD="/nfs/soc-LiveJournal1.bin"
else
	# --------------------------------------------------
	# LINUX
	# --------------------------------------------------

    PROGRAMS="pagerank"
    BARRELFISH_WORKLOAD="null"
    OPTIONS=("" "-d" "-d -r" "-d -r -p" "-d -r -p -h" "-d -r -h")
fi

# XXX - we should probably also put the C-code generated by the Green Marl compiler into the archive

# File to remeber log files
LOGFILES=$(mktemp /tmp/run_all-overview-XXXXXX)

BASE=$(readlink -e $(dirname $0)/../)
. $BASE/env.sh

# --------------------------------------------------
# Set the number of cores for the benchmark according to the machine
# --------------------------------------------------cd
CORELIST=""
[[ $(hostname) == bach* ]] && CORELIST="16 12 8 4 2"
[[ $(hostname) == "sgs-r815-03" ]] && CORELIST="64 32 16 8"
[[ $(hostname) == "sgs-r820-01" ]] && CORELIST="64 32 16 8"
[[ $(hostname) == "babybel" ]] && CORELIST="4 10 20"
# Check --------------------------------------------
if [[ RUN_ON_BARRELFISH -eq 0 ]]; then
    [[ -n "$CORELIST" ]] || error "Don't know this machine"
else
    CORELIST="20"
fi
# --------------------------------------------------

(echo -n '#' && git describe) >> $LOGFILES
(echo -n '#' && cd shoal && git describe) >> $LOGFILES

(
    # Programs
    # --------------------------------------------------
    for PROG in $PROGRAMS; do

		# Twitter does not work on triangle_counting
	    if [[ "$PROG" == "triangle_counting" ]]; then
			echo "Encountered triangle_counting, using soc-LiveJournal1"
			WORKLOAD="soc-LiveJournal1"
	    fi

        # Configurations
        # --------------------------------------------------
        for optidx in ${!OPTIONS[*]}; do

            OPTS=${OPTIONS[$optidx]}

            if [[ $RUN_ON_BARRELFISH -eq 0 ]]; then
                # Build (one for configuration)
                if [[ $OPTS == *-p* ]]; then
                    echo -n "Building sk_$PROG (static) .. "
                    (CXXFLAGS=-DSHL_STATIC make "sk_$PROG" > /dev/null) || error "Build failed"
                else
                    echo -n "Building sk_$PROG .. "
                    (make "sk_$PROG" > /dev/null) || error "Build failed"
                fi
                echo "done"
            fi

            # Cores
            # --------------------------------------------------
            for CORES in $CORELIST; do

                # Create and remember log files
                TMP=`mktemp /tmp/tmp-run_all-XXXXXX`
                echo $TMP " " $PROG " " $OPTS " " $CORES >> $LOGFILES
                echo "Running: $TMP $PROG $OPTS $CORES"


                export SHL__NUM_CORES=$CORES
                export SHL__WORKLOAD=$BARRELFISH_WORKLOAD
                # Run
                export NUM=$NUM_RUNS
                exec_avg scripts/run.sh $OPTS $PROG $CORES ours $WORKLOAD $BARRELFISH $BARRELFISH &> $TMP; RC=$?

                # Evaluate return code
                if [[ $RC -eq 0 ]]; then
                    RCS=$txtgrn"ok  "$txtrst
                else
                    RCS=$txtred"fail"$txtrst
                fi

                # Print result
                echo -e " .. return code [$RCS] for [$PROG] with [$CORES] [$OPTS] [$BARRELFISH] was [$RC] ... log at [$TMP]"
                if [[ $RC -eq 0 ]]; then
                    echo -n "  total: "; cat $TMP | awk '/^total:/ { print $2 }' | skstat.py
                    echo -n "  comp : "; cat $TMP | awk '/^comp:/ { print $2 }' | skstat.py
                    echo -n "  copy : "; cat $TMP | awk '/^copy:/ { print $2 }' | skstat.py

                    R=( "${PIPESTATUS[@]}" )

                    START_RC="${R[2]}"
                    if [[ $START_RC -ne 0 ]]; then
                        echo "WARNING: skstat.py did not exit properly"
                    fi
                fi

            done
        done
    done

)

# Create archive from log file and cleanup
TS=$(date +%F_%H-%M-%S)
echo "Building log file run_all_${TS}.tgz and cleaning up .. "
(cat $LOGFILES | grep -v '^#' | awk '{ print $1 }' | xargs tar -czf "run_all_${TS}.tgz" $LOGFILES)  \
    && (cat $LOGFILES | grep -v '^#' | awk '{print $1 }' | xargs rm $LOGFILES)
