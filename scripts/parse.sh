#!/bin/bash

txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtylw='\e[0;33m' # Yello
txtrst='\e[0m'    # Text Reset

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

# --------------------------------------------------
# Calculate
#
# wrapper for bc, using 8 digits after comma and passing everything
# given as arguments to the function as arguments to bc
#
# --------------------------------------------------
function calc() {

	VAR=$@

#   DEBUG
#	echo -n "Executing [$VAR] .. {" >/dev/stderr

	PROG=$(cat <<EOF
scale=8
$VAR
quit
EOF
)

    RES=$(echo "$PROG" | bc)

	export BC_RC=$?

#   DEBUG
#	echo -n "} .. RC=$BC_RC RES=$RES" >/dev/stderr

	echo $RES
}

# --------------------------------------------------
# Make sure measurement was executing properly
#
# - output is correct
# - time measurements sum up correctly
#
# arguments:
# 1: file to check
# --------------------------------------------------
function check_measurement() {

    CHECK_FILE=$1

    NUM_RUNS=$(cat $CHECK_FILE | grep 'SHOAL.*initialization' | wc -l)
    echo -n " -> Detected $NUM_RUNS runs -- "

    NUM_SUCC=$(cat $CHECK_FILE | grep "result.*correct.*CRC.*correct" | wc -l)
    echo -n " succ: $NUM_SUCC"

    echo ""

    SUM=0

    # We currently use only "copy" and "comp" in the results.
    # We need to make sure that we understand for all the additional costs how to account them for.

    for title in "copy" "comp" "crc" "alloc"; do

	# extract measurements ..
	CHECK_NUMS=$(cat $CHECK_FILE | \
	    gawk -v TITLE="$title" '{ if ($0 ~ "^"TITLE ":") print $ 2}')

	# run statistics
	CHECK_STAT=$(echo "$CHECK_NUMS" | skstat.py)

	# convert to array
	CHECK_STAT_ARR=($CHECK_STAT)

	# need measurement and stderr
	RES=${CHECK_STAT_ARR[0]}
	STDERR=${CHECK_STAT_ARR[1]}

	# Check stderr
	# --------------------------------------------------
	FACT=$(calc "$STDERR/$RES")
#   SK: I'd like to pass the RC of bc here, so that in case this fails, we can set FACT to sth >1.0
#	echo "RC=$? FACT=$FACT BC_RC=$BC_RC"
	MAXACCEPT="0.05"
	RESULT=$(calc "$FACT<$MAXACCEPT")
	if [[ $RESULT -eq 1 ]]; then
		echo -e $txtgrn "standard error good" $txtrst " [$STDERR/$RES=$FACT, $MAXACCEPT, $RESULT]";
	else
		echo -e $txtred "standard error too high" $txtrst " [$STDERR/$RES=$FACT, $MAXACCEPT, $RESULT]";
		echo "This is for [$title], measurements are:"
		echo $CHECK_NUMS
		echo "END"
	fi

	# Sum up measurements
	SUM=$(calc "$SUM+$RES")
    done

    # Make sure the Green Marl calculated total is similar to the sum of our break-down
    GMTOTAL=$(cat $CHECK_FILE | gawk '/^gmtotal:/ { print $2 }' | skstat.py | cut -f2 -d' ')
    echo -n " sum(copy+comp+total+crc) is $SUM, GMTOTAL is $GMTOTAL"

    RATIO=$(calc "$GMTOTAL/$SUM")

    # Check if the "unknown" time is less than 1%
    RATIO_GOOD=1
    if [[ $(calc "$RATIO>1.01") -eq 1 ]]; then RATIO_GOOD=0; fi
    if [[ $(calc "$RATIO<0.99") -eq 1 ]]; then RATIO_GOOD=0; fi

    echo -n " ratio is: "
    if [[ $RATIO_GOOD -ne 1 ]]; then echo -n -e $txtred; fi

    echo -e "$RATIO" $txtrst
}

function read_basic_info() {

    FILE=$1
    [[ -f $FILE ]] || error "Cannot read file in basic info"

    # Workload is: /some/path/twitter_rv.bin twitter_rv.bin
    WORKLOAD=$(cat $FILE | awk '/^Workload/ { print $4 }' | head -n 1 | sed -e 's/.bin//')
    export WORKLOAD

    # Hostname: <name>
    # MACHINE=$(cat $FILE | awk '/^Hostname/ { print $2 }')
    # export MACHINE
}

echo "Overview file is $OVERVIEW"

basicInfo=0

# Clear existing measurements
# --------------------------------------------------
while read line
do
    # Ignore lines starting with #
    if (echo "$line" | grep '^#' ); then echo "Ignoring $line"; continue; fi

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
# --------------------------------------------------
while read line
do
    # Ignore lines starting with #
    if (echo "$line" | grep '^#' ); then echo "Ignoring $line"; continue; fi

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

    check_measurement $TMP/$FILE

    T_COMP=$(cat $TMP/$FILE | awk '/^comp/ { print $2 }' | skstat.py)
    T_INIT=$(cat $TMP/$FILE | awk '/^copy/ { print $2 }' | skstat.py)

    echo "$NUM $T_COMP" >> $F_COMP
    echo "$NUM $T_INIT" >> $F_INIT

done < $OVERVIEW

echo "Should I delete? $TMP"; read
rm -rf $TMP
