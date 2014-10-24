#!/bin/bash

#set -x

PROG=pagerank
OPTS=
WORKLOAD=twitter_rv

txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtrst='\e[0m'    # Text Reset

# Print error message
function error() {
	echo -e $txtred$1$txtrst
	exit 1
}

# File to remeber log files
LOGFILES=$(mktemp /tmp/run_all-overview-XXXXXX)

(
	for PROG in "pagerank" "triangle_counting" "hop_dist"; do
		for OPTS in "" "-d" "-d -r" "-d -r -p" "-d -r -p -h" "-d -r -h"; do
			# Create and remember log files
			TMP=`mktemp /tmp/tmp-run_all-XXXXXX`
			echo $TMP " " $PROG " " $OPTS >> $LOGFILES

			# Build
			echo "Building sk_$PROG"
			if [[ $OPTS == *-p* ]]; then
			    (CXXFLAGS=-DSHL_STATIC make "sk_$PROG" > /dev/null) || error "Build failed"
			else
			    (make "sk_$PROG" > /dev/null) || error "Build failed"
			fi
			# Run
			export NUM=3
			exec_avg scripts/run.sh $OPTS $PROG 32 ours $WORKLOAD &> $TMP
			RC=$?
			if [[ $RC -eq 0 ]]; then
				RCS=$txtgrn"ok  "$txtrst
			else
				RCS=$txtred"fail"$txtrst
			fi
			echo -e "Return code [$RCS] for [$PROG] with [$OPTS] was [$RC] ... log at [$TMP]"
			if [[ $RC -eq 0 ]]; then
			    # total:    3538.68300
			    # copy:     1857.26100
			    # comp:     1681.42200
			    echo -n "  total: "; cat $TMP | awk '/^total:/ { print $2 }' | skstat.py
			    echo -n "  comp : "; cat $TMP | awk '/^comp:/ { print $2 }' | skstat.py
			fi
		done
	done
)

# Create archive from log file and cleanup
TS=$(date +%F_%H-%M-%S)
echo "Building log file run_all_${TS}.tgz and cleaning up .. "
(cat $LOGFILES | awk '{ print $1 }' | xargs tar -czf "run_all_${TS}.tgz" $LOGFILES)  \
    && (cat $LOGFILES | awk '{print $1 }' | xargs rm $LOGFILES)