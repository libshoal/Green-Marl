#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    echo "Usage: $0 <action>"
	echo ""
	echo "Action: one of {init, build, run}"
    exit 1
}

[[ -n "$1" ]] || usage

ACTION=$1
CORES=$(($(nproc)/2))

BASE=$(readlink -e $(dirname $0))
source $BASE/shared.sh

case "$ACTION" in
	"init")
		echo "Initializing GIT submodules"
		git submodule init
		git submodule update
		;;
	"build")
		echo "Building depdendencies"
		make sk_buildbot
		;;
	"run")
		echo "Running tests"
		run_wrapper "" $CORES "pagerank" "/dev/null" "soc-LiveJournal1"
		;;
	"run-d")
		echo "Running tests"
		run_wrapper "-d" $CORES "pagerank" "/dev/null" "soc-LiveJournal1"
		;;
	"run-p")
		echo "Running tests"
		run_wrapper "-p" $CORES "pagerank" "/dev/null" "soc-LiveJournal1"
		;;
	"run-r")
		echo "Running tests"
		run_wrapper "-r" $CORES "pagerank" "/dev/null" "soc-LiveJournal1"
		;;
	"run-h")
		echo "Running tests"
		run_wrapper "-h" $CORES "pagerank" "/dev/null" "soc-LiveJournal1"
		;;
	*)
		echo "unknown action"
		exit 1
		;;
esac
