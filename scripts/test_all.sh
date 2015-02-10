#!/bin/bash

#set -x

OPTS=

# Configure the workload to be used
# --------------------------------------------------
#
# Twitter is much bigger and runs much longer. It does not work with
# triangle_counting, so we just fall back to soc-LiveJournal1 in that
# case.
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

BASE=$(readlink -e $(dirname $0))
source $BASE/shared.sh

# File to remeber log files
LOGFILES=$(mktemp /tmp/test_all-overview-XXXXXX)

# --------------------------------------------------
# Number of cores
# --------------------------------------------------
if [[ -z "$CORELIST" ]]; then
    CORELIST="$(nproc) 1"
fi

(
    # Programs
    # --------------------------------------------------
	for PROG in "pagerank" "hop_dist" "triangle_counting"; do

	    if [[ $PROG == "triangle_counting" ]]; then
		WORKLOAD="soc-LiveJournal1"
	    fi

	    # Configurations
	    # --------------------------------------------------
	    for OPTS in "" "-d" "-d -r" "-d -r -p" "-d -r -p -h"; do

			run_wrapper "$OPTS" "$CORELIST" "$PROG" "$LOGFILES" "$WORKLOAD"

			if [[ $RC -ne 0 ]]; then
				exit 1
			fi
		done
	done
) || exit 1

# Create archive from log file and cleanup
TS=$(date +%F_%H-%M-%S)
echo "Building log file test_all_${TS}.tgz and cleaning up .. "
(cat $LOGFILES | awk '{ print $1 }' | xargs tar -czf "test_all_${TS}.tgz" $LOGFILES)  \
    && (cat $LOGFILES | awk '{print $1 }' | xargs rm $LOGFILES)


exit 0
