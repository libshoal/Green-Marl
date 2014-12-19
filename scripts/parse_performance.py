#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import fileinput
import argparse
import re

from tools import statistics

values = {}
linux = True

def parse_measurement_file(fname):
    event = None
    count = None
    workload = None
    program = None
    conf = None
    numcores = -1

    global linux

    for line in fileinput.input(fname):

        # Performance
        # --------------------------------------------------
        m = re.match('^comp:\s+(\d+)', line)
        if m:
            count = float(m.group(1))
            print 'Found comp result: %f' % count
            store(program, numcores, conf, 'comp', count)

        # Barrelfish?
        # --------------------------------------------------
        if 'Barrelfish CPU driver starting' in line:
            linux = False

        # Number of threads
        # --------------------------------------------------
        # [OUT] SHOAL (v 1.0) initialization .. 20 threads .. dumping NUMA topology
        m = re.match('^\[OUT\]\s+SHOAL.*initialization.*\s(\d+)\sthreads', line)
        if m:
            numcores = int(m.group(1))

        # [OUT] PAPI:          PAPI_TLB_TL             486             486
        # --------------------------------------------------
        m = re.match('^\[OUT\].*PAPI.*:.*\s+PAPI_([A-Z_0-9]+)\s+([0-9]+)', line)
        if m:
            event = m.group(1)
            count = m.group(2)
            store(program, numcores, conf, event, count)

        # Workload
        # --------------------------------------------------
        m = re.match('^Workload is:.*\s([a-zA-Z0-9\.-]+)$', line)
        if m:
            workload = m.group(1)

        # PROGRAM
        # --------------------------------------------------
        # Program is: /home/skaestle/projects/gm//apps/output_cpp/bin/triangle_counting triangle_counting
        m = re.match('^Program is:.*\s([a-zA-Z0-9\.-_]+)$', line)
        if m:
            program = m.group(1)

        # CONFIGURATION
        # --------------------------------------------------
        # Running 1 iterations of [scripts/run.sh -d -r -p -h triangle_counting 64 ours soc-LiveJournal1]
        m = re.match('^Running.*run.sh\s+([dhrp -]*)\s+[a-z_]+\s+[0-9]+', line)
        if m:
            conf = m.group(1)

def store(program, cores, conf, event, count):

    global values

    # Store ..
    if not program in values:
        values[program] = {}
    if not cores in values[program]:
        values[program][cores] = {}
    if not conf in values[program][cores]:
        values[program][cores][conf] = []
    values[program][cores][conf].append((event, count))

def main():

    parser = argparse.ArgumentParser(description='Parse measurement file')
    parser.add_argument('fname', nargs='+')
    args = parser.parse_args()

    for fname in args.fname:
        parse_measurement_file(fname);

    print values

    confs = [ None, '-d', '-d -r', '-d -r -p', '-d -r -p -h' ]
#    events = [ e for (e, _) in values.items()[0][1].items()[0][1]]

    replace = {
        'triangle_counting': 'tc',
        'pagerank': 'pr',
        'hop_dist': 'hd'
    }

    print 'OS:', ('Linux' if linux else 'Barrelfish')

    programs = [ p for (p, d) in values.items() ]
    print 'Programs are: ', programs

    for (program, data) in values.items():
        print 'Numbers for ', program

        # Extracting cores and sort
        cores = sorted([ c for (c, d) in data.items() ])

        for core in cores:
            datai = data[core]
            print core, 'cores'

            # Check all configurations from log file
            for (conf, _) in datai.items():
                assert conf in confs

            for conf in confs:

                if not conf in datai:
                    mean = -1
                    stderr = 0

                else:
                    dataii = datai[conf]
                    (dmean, dstderr, dmin, dmax, dmedian) = \
                        statistics([ s for (x,s) in dataii if x == 'comp' ])
                    mean = dmean
                    stderr = dstderr

                print '%-30s %10.3f %10.3f' % (conf, mean, stderr)

    # for event in events:

    #     print event
    #     print

    #     print '||',
    #     for c in conf:
    #         print c, '|',
    #     print
    #     print '|-'

    #     for (program, data) in values.items():
    #         print '|', replace[program], '|',

    #         for c in conf:
    #             for (e, t) in data[c]:
    #                 if e == event:
    #                     value = int(t)
    #                     print ("{0:.2f} | ".format(value/10000.)),
    #         print

    #     print

if __name__ == "__main__":
    main()
