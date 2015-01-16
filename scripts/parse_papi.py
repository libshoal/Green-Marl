#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import fileinput
import argparse
import re

values = {}
eventlist  = []

def parse_papi_file(fname):

    global eventlist

    event = None
    count = None
    workload = None
    program = None
    conf = None

    for line in fileinput.input(fname):

        # [OUT] [ ] Partition
        m = re.match('^\[OUT\] \[', line)
        if m:
            print 'Configuration found:', line.rstrip()

        # [OUT] PAPI:          PAPI_TLB_TL             486             486
        m = re.match('^\[OUT\].*PAPI.*:.*\s+PAPI_([A-Z_0-9]+)\s+([0-9]+)', line)
        if m:
            event = m.group(1)
            count = m.group(2)
            print 'Found PAPI event %s %s' % (event, count)
            store(program, conf, event, count)

        m = re.match('^Workload is:.*\s([a-zA-Z0-9\.\-_]+)$', line)
        if m:
            workload = m.group(1)
            print 'Found workload:', workload

        # Program is: /home/skaestle/projects/gm//apps/output_cpp/bin/triangle_counting triangle_counting
        m = re.match('^Program is:.*\s([a-zA-Z0-9\.-_]+)$', line)
        if m:
            program = m.group(1)
            print 'Found program:', program

        # Running 1 iterations of [scripts/run.sh -d -r -p -h triangle_counting 64 ours soc-LiveJournal1]
        m = re.match('^Running.*run.sh\s+([dhrp -]*)\s+[a-z_]+\s+[0-9]+', line)
        if m:
            conf = m.group(1)
            print 'Found conf:', conf

def store(program, conf, event, count):
    # Store ..
    if not program in values:
        values[program] = {}
    if not conf in values[program]:
        values[program][conf] = {}
    if not event in values[program][conf]:
        values[program][conf][event] = []

    eventlist.append(event)
    values[program][conf][event].append(int(count))

def main():

    parser = argparse.ArgumentParser(description='Parse PAPI result files')
    parser.add_argument('fname', nargs='+')
    args = parser.parse_args()

    for fname in args.fname:
        print 'Parsing file', fname
        parse_papi_file(fname);

    print '--------------------------------------------------'
    print values
    print '--------------------------------------------------'

#    conf = [ c for (c,d) in values.items()[0][1].items() ]
    conf = [ None, '-d', '-d -r', '-d -r -p', '-d -r -p -h' ]
    events = set(eventlist)

    replace = {
        'triangle_counting': 'tc',
        'pagerank': 'pr',
        'hop_dist': 'hd'
    }

    PRINTSTDERR=False

    for event in events:

        print event
        print

        print '|    |',
        for c in conf:
            print "{0:10s}".format(c), '|',
            if PRINTSTDERR:
                print 'stderr    ', '|',
        print
        print '|-'

        for (program, data) in values.items():
            print '|', replace[program], "|",

            for c in conf:
                for (e, t) in data[c].items():
                    if e == event:

                        (mean, stderr, _, _, _) = tools.statistics([ e/10000. for e in t])
                        print "{0:10.2f} |".format(mean),
                        if PRINTSTDERR:
                            print "{0:10.2f} |".format(stderr),
            print

        print

if __name__ == "__main__":
    main()
