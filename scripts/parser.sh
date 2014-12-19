#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    cat <<EOF
$0 [-mode] <measurement-file>

Where mode is:
 -w : determine working-set size
 -c : array configuration
EOF
    exit 1
}

SCRIPTDIR=$(dirname $0)
MODE=PAPI

while [[ -n "$1" ]]; do

	case "$1" in
		"-w")
			shift
			MODE=WORKINGSETSIZE
			break
			;;

		"-c")
			shift
			MODE=CONF
			break
			;;

		"-p")
			shift
			MODE=PERFORMANCE
			break
			;;

		*)
			break
			;;
	esac
done

IN=$1
[[ -f $IN ]] || usage

TMP=`mktemp -d`

tar -xf $IN -C $TMP
OVERVIEW=$(find $TMP -name '*_all-overview-*')

# --------------------------------------------------
# CONFIGURATION
# --------------------
WORKLOAD="determinedAutomatically"
MACHINE="determinedAutomatically"
# --------------------------------------------------

function read_entry() {

    echo $(echo "$1" | awk -vnum=$2 -F'   ' '{ print $num }')
}

function check_file() {

    if [[ -L "$1" ]]; then
	echo -n "SYMLINK "; rm "$1"
    fi

    if [[ -e "$1" ]]; then
	echo -n "YES "
    else
	echo -n "NO  "
    fi
    echo "$1"
}

function read_basic_info() {

    FILE=$1
    [[ -f $FILE ]] || error "Cannot read file in basic info"

    # Workload is: /some/path/twitter_rv.bin twitter_rv.bin
    WORKLOAD=$(cat $FILE | awk '/^Workload/ { print $4 }' | head -n 1 | sed -e 's/.bin//')
    export WORKLOAD

    # Hostname: <name>
    MACHINE=$(cat $FILE | awk '/^Hostname/ { print $2 }')
    export MACHINE
}

echo "Overview file is $OVERVIEW"

basicInfo=0

TMPFILE=$(mktemp)

# Find measurements
while read line
do

    FILE=$(read_entry "$line" 1)
    APP=$(read_entry "$line" 2)
    CONF=$(read_entry "$line" 3 | python -c "import re; import fileinput; print ''.join([ x for x in fileinput.input()[0] if re.match('\w', x)])")
    NUM=$(read_entry "$line" 4)

    # if [[ basicInfo -eq 0 ]]; then
	# 	echo "Reading basic info from [$TMP/$FILE]"
	# 	read_basic_info $TMP/$FILE
	# 	basicInfo=1
    # fi

    if [[ -z "$NUM" ]]; then
	NUM=$CONF
	CONF=""
    fi

	echo $FILE "-" $APP "-" $CONF "-" $NUM
	echo $TMP/$FILE >> $TMPFILE

done < $OVERVIEW

cat $TMPFILE

case $MODE in
	PAPI)
		cat $TMPFILE | xargs ${SCRIPTDIR}/parse_papi.py
		;;
	PERFORMANCE)
		cat $TMPFILE | xargs ${SCRIPTDIR}/parse_performance.py
		;;
	WORKINGSETSIZE)
		cat $TMPFILE | xargs ${SCRIPTDIR}/parse_workingsetsize.py
		;;
	CONF)
		cat $TMPFILE | xargs ${SCRIPTDIR}/parse_array_conf.py
		;;
	*)
		echo "Don't know what to do"
		exit 1
		;;
esac

rm $TMPFILE

echo "Should I delete? $TMP"; read
rm -rf $TMP
