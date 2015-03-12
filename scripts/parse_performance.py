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
num_errors = 0

def parse_measurement_file(fname):
    event = None
    count = None
    workload = None
    program = None
    conf = None
    numcores = -1
    last_array = None

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


        # Array size
        # --------------------------------------------------
        m = re.match('ERROR', line)
        if m:
            global num_errors
            num_errors += 1
            print 'ERROR in configuration, total:', num_errors

        # Parse array
        # --------------------------------------------------
        m = re.match('Array\[\s+(\S+)\]+', line)
        if m:
            last_array = m.group(1).replace('shl__', '')

        # Pagesize
        # --------------------------------------------------
        m = re.match('Allocating with pagesize (\d+)', line)
        if m:
            assert last_array # Line before should have been array configuration
            huge = True if m.group(1) == '2097152' else False
            large = True if m.group(1) == '1073741824' else False
            assert huge or large or m.group(1) == '4096' # 4K, 2M or 1G page
            print 'Found page size for array %20s L=%5s H=%5s size=%10s' % \
                (last_array, str(large), str(huge), m.group(1))

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

    confs = [ None, '-d', '-d -r', '-d -r -p', '-d -r -p -h', '-r -h', '-r', '-h' ]
    confs_available = set()

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
                print 'Found configuration', conf
                confs_available.add(conf)
                assert conf in confs

            print 'Following configurations are available', str(confs_available)

            for conf in confs_available:

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
