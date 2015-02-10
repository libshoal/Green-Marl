#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    echo "Usage: $0"
    exit 1
}

#[[ -n "$1" ]] || usage

git submodule init
git submodule update
make sk_buildbot

export CORELIST="32 1"
scripts/test_all.sh
