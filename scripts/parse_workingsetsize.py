#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import fileinput
import argparse
import re

values = {}

def parse_papi_file(fname):
    event = None
    count = None
    workload = None
    program = None
    conf = None

    for line in fileinput.input(fname):

        m = re.match('.*Total working set size: ([0-9.\-,]+).*', line)
        if m:
            print line
            store(program, workload, m.group(1))

        m = re.match('^Workload is:.*\s([a-zA-Z0-9\.-]+)$', line)
        if m:
            print line
            workload = m.group(1)

        # Program is: /home/skaestle/projects/gm//apps/output_cpp/bin/triangle_counting triangle_counting
        m = re.match('^Program is:.*\s([a-zA-Z0-9\.-_]+)$', line)
        if m:
            program = m.group(1)

        # Running 1 iterations of [scripts/run.sh -d -r -p -h triangle_counting 64 ours soc-LiveJournal1]
        m = re.match('^Running.*run.sh\s+([dhrp -]*)\s+[a-z_]+\s+[0-9]+', line)
        if m:
            conf = m.group(1)

def store(program, workload, size):

    if not program in values:
        values[program] = {}

    if not workload in values[program]:
        values[program][workload] = size
    else:
        assert values[program][workload] == size

def main():

    parser = argparse.ArgumentParser(description='Parse PAPI result files')
    parser.add_argument('fname', nargs='+')
    args = parser.parse_args()

    for fname in args.fname:
        parse_papi_file(fname);

    print values

    replace = {
        'triangle_counting': 'tc',
        'pagerank': 'pr',
        'hop_dist': 'hd'
    }

    for (program, data) in values.items():
        print '|', replace[program],

        for (workload, size) in data.items():
            print workload, int(size)/1024./1024.

if __name__ == "__main__":
    main()
