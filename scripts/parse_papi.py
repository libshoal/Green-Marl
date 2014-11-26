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

        # [OUT] PAPI:          PAPI_TLB_TL             486             486
        m = re.match('^\[OUT\].*PAPI.*:.*\s+PAPI_([A-Z_0-9]+)\s+([0-9]+)', line)
        if m:
            event = m.group(1)
            count = m.group(2)
            store(program, conf, event, count)

        m = re.match('^Workload is:.*\s([a-zA-Z0-9\.-]+)$', line)
        if m:
            workload = m.group(1)

        # Program is: /home/skaestle/projects/gm//apps/output_cpp/bin/triangle_counting triangle_counting
        m = re.match('^Program is:.*\s([a-zA-Z0-9\.-_]+)$', line)
        if m:
            program = m.group(1)

        # Running 1 iterations of [scripts/run.sh -d -r -p -h triangle_counting 64 ours soc-LiveJournal1]
        m = re.match('^Running.*run.sh\s+([dhrp -]*)\s+[a-z_]+\s+[0-9]+', line)
        if m:
            conf = m.group(1)

def store(program, conf, event, count):
    # Store ..
    if not program in values:
        values[program] = {}
    if not conf in values[program]:
        values[program][conf] = []
    values[program][conf].append((event, count))

def main():

    parser = argparse.ArgumentParser(description='Parse PAPI result files')
    parser.add_argument('fname', nargs='+')
    args = parser.parse_args()

    for fname in args.fname:
        parse_papi_file(fname);

    print values

#    conf = [ c for (c,d) in values.items()[0][1].items() ]
    conf = [ None, '-d', '-d -r', '-d -r -p', '-d -r -p -h' ]
    events = [ e for (e, _) in values.items()[0][1].items()[0][1]]

    replace = {
        'triangle_counting': 'tc',
        'pagerank': 'pr',
        'hop_dist': 'hd'
    }

    for event in events:

        print event
        print

        print '||',
        for c in conf:
            print c, '|',
        print
        print '|-'

        for (program, data) in values.items():
            print '|', replace[program], '|',

            for c in conf:
                for (e, t) in data[c]:
                    if e == event:
                        value = int(t)
                        print ("{0:.2f} | ".format(value/10000.)),
            print

        print

if __name__ == "__main__":
    main()
