#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    echo "Usage: $0 <measurement-file>"
    exit 1
}

IN=$1
[[ -f $IN ]] || usage


TMP=`mktemp -d`

tar -xf $IN -C $TMP
OVERVIEW=$(find $TMP -name 'run_all-overview-*')

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

# Clear existing measurements
while read line
do
    FILE=$(read_entry "$line" 1)
    APP=$(read_entry "$line" 2)
    CONF=$(read_entry "$line" 3 | python -c "import re; import fileinput; print ''.join([ x for x in fileinput.input()[0] if re.match('\w', x)])")
    NUM=$(read_entry "$line" 4)

    if [[ basicInfo -eq 0 ]]; then

	echo "Reading basic info from [$TMP/$FILE]"
	read_basic_info $TMP/$FILE
	basicInfo=1
    fi

    if [[ -z "$NUM" ]]; then
	NUM=$CONF
	CONF=""
    fi

    echo $FILE "-" $APP "-" $CONF "-" $NUM

    F_COMP=~/papers/oracle/measurements/${WORKLOAD}_${MACHINE}_${APP}_${CONF}_comp
    F_INIT=~/papers/oracle/measurements/${WORKLOAD}_${MACHINE}_${APP}_${CONF}_init

    check_file $F_COMP
    check_file $F_INIT

    echo "x y e" > $F_COMP
    echo "x y e" > $F_INIT

done < $OVERVIEW

# Find measurements
while read line
do
    FILE=$(read_entry "$line" 1)
    APP=$(read_entry "$line" 2)
    CONF=$(read_entry "$line" 3 | python -c "import re; import fileinput; print ''.join([ x for x in fileinput.input()[0] if re.match('\w', x)])")
    NUM=$(read_entry "$line" 4)

    if [[ -z "$NUM" ]]; then
	NUM=$CONF
	CONF=""
    fi

    echo $FILE "-" $APP "-" $CONF "-" $NUM

    F_COMP=~/papers/oracle/measurements/${WORKLOAD}_${MACHINE}_${APP}_${CONF}_comp
    F_INIT=~/papers/oracle/measurements/${WORKLOAD}_${MACHINE}_${APP}_${CONF}_init

    check_file $F_COMP
    check_file $F_INIT

    T_COMP=$(cat $TMP/$FILE | awk '/^comp/ { print $2 }' | skstat.py)
    T_INIT=$(cat $TMP/$FILE | awk '/^copy/ { print $2 }' | skstat.py)

    echo "$NUM $T_COMP" >> $F_COMP
    echo "$NUM $T_INIT" >> $F_INIT

done < $OVERVIEW

echo "Should I delete? $TMP"; read
rm -rf $TMP
