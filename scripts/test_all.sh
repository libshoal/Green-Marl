#!/bin/bash

#set -x

OPTS=
#WORKLOAD=twitter_rv
WORKLOAD="soc-LiveJournal1"

txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtrst='\e[0m'    # Text Reset

# Print error message
function error() {
	echo -e $txtred$1$txtrst
	exit 1
}

# File to remeber log files
LOGFILES=$(mktemp /tmp/test_all-overview-XXXXXX)

# --------------------------------------------------
# Number of cores
# --------------------------------------------------
CORELIST=$(nproc) 1

(
    # Programs
    # --------------------------------------------------
	for PROG in "pagerank" "hop_dist" "triangle_counting"; do

	    # Configurations
	    # --------------------------------------------------
	    for OPTS in "" "-d" "-d -r" "-d -r -p" "-d -r -p -h"; do

                    # Build (one for configuration)
		    if [[ $OPTS == *-p* ]]; then
		    	echo -n "Building sk_$PROG (static) .. "
		    	(CXXFLAGS=-DSHL_STATIC make "sk_$PROG" > /dev/null) || error "Build failed"
		    else
		    	echo -n "Building sk_$PROG .. "
		    	(make "sk_$PROG" > /dev/null) || error "Build failed"
		    fi
		    echo "done"

		    # Cores
		    # --------------------------------------------------
		    for CORES in $CORELIST; do

			# Create and remember log files
			TMP=`mktemp /tmp/tmp-test_all-XXXXXX`
			echo $TMP " " $PROG " " $OPTS " " $CORES >> $LOGFILES
			echo "Running: $TMP $PROG $OPTS $CORES"

			# Run
			scripts/run.sh $OPTS $PROG $CORES ours $WORKLOAD &> $TMP
			RC=$?

			# Evaluate return code
			if [[ $RC -eq 0 ]]; then
				RCS=$txtgrn"ok  "$txtrst
			else
				RCS=$txtred"fail"$txtrst
			fi

			# Print result
			echo -e " .. return code [$RCS] for [$PROG] with [$CORES] [$OPTS] was [$RC] ... log at [$TMP]"
			# if [[ $RC -eq 0 ]]; then
			#     echo -n "  total: "; cat $TMP | awk '/^total:/ { print $2 }' | skstat.py
			#     echo -n "  comp : "; cat $TMP | awk '/^comp:/ { print $2 }' | skstat.py
			# fi
		    done
		done
	done
)

# Create archive from log file and cleanup
TS=$(date +%F_%H-%M-%S)
echo "Building log file test_all_${TS}.tgz and cleaning up .. "
(cat $LOGFILES | awk '{ print $1 }' | xargs tar -czf "test_all_${TS}.tgz" $LOGFILES)  \
    && (cat $LOGFILES | awk '{print $1 }' | xargs rm $LOGFILES)
