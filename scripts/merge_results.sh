#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    echo "Usage: $0 <measurement-file1> <measurement_file2>"
    exit 1
}

# Two input files
IN1=$1
[[ -f $IN1 ]] || usage

IN2=$2
[[ -f $IN2 ]] || usage

TMP1=`mktemp -d`
tar -xf $IN1 -C $TMP1
OVERVIEW1=$(find $TMP1 -name 'run_all-overview-*')

TMP2=`mktemp -d`
tar -xf $IN2 -C $TMP2
OVERVIEW2=$(find $TMP2 -name 'run_all-overview-*')

SRCDIR=$(pwd)

OUTDIR=`mktemp -d`
mkdir $OUTDIR/tmp
pushd $OUTDIR/tmp
cp $TMP1/tmp/* . -r
cp $TMP2/tmp/* . -r
find . -name 'run_all-overview*' -delete
cat $OVERVIEW1 $OVERVIEW2 > 'run_all-overview-merged'

TS=$(date +%F_%H-%M-%S)
OUTFILE="run_all_${TS}.tgz"
tar -czf $SRCDIR/$OUTFILE ../tmp

popd

echo "Directories are $TMP1 $TMP2 $OUTDIR"
echo "Outfile is $OUTFILE"
