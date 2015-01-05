#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import fileinput
import argparse
import re

values = {}

KEYS = [
    'comp',
    'total',
    'copy'
]

timer_values = {}

def parse_papi_file(fname):
    event = None
    count = None
    workload = None
    program = None
    conf = None
    hugepages = None

    for line in fileinput.input(fname):

        # hugepage         shl__G_begin=1       shl__G_pg_rank=1       shl__G_r_begin=0    shl__G_r_node_idx=1   shl__G_pg_rank_nxt=0
        m = re.match('^hugepage', line)
        if m:

            print line
            arrs = line.rstrip().split()

            arrconfs = []

            for i in range(1, len(arrs)):
                arrconfs.append(arrs[i].split('='))

            arrconfs = sorted(arrconfs, key=lambda x: x[0])
            arrconfstr = ''.join([ c for (a,c) in  arrconfs ])

            assert hugepages == None or hugepages == arrconfstr
            arrconfstr = hugepages

        # Time
        for s in KEYS:
            search = ('^%s:\s+([0-9\.]+)' % s)
            m = re.match(search, line)
            if m:
                timer_values[s].append(float(m.group(1)))

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

        # lua
        m = re.match('LUA', line)
        if m:
            print line

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

    output = {}

    for fname in args.fname:

        # Init
        # ------------------------------

        for k in KEYS:
            timer_values[k] = []

        print 'FILE', fname
        parse_papi_file(fname);

        output[fname] = {}

        # Calculate
        # ------------------------------

        print timer_values

        for (key, vals) in timer_values.items():
            print '[%20s] has %3d entries' % (key, len(vals)),

            if len(vals)>0:
                (m, stderr, median, vmin, vmax) = tools.statistics(vals)
                print '%10.3f %10.3f' % (m, stderr)

                output[fname][key] = (m, stderr)


    # Output statistics
    print '| file',
    for l in [ 'comp', 'total', 'copy' ]:
        print '|', l, 'stderr',
    print '|'
    print '|-'

    for (fname, values) in output.items():
        print '| %20s ' % fname,
        for l in [ 'comp', 'total', 'copy' ]:
            (m, stderr) = values[l]
            print ('| %10.3f | %10.3f ' % (m, stderr)),
        print '|'

    print '|-'

    # print values

    # replace = {
    #     'triangle_counting': 'tc',
    #     'pagerank': 'pr',
    #     'hop_dist': 'hd'
    # }

    # for (program, data) in values.items():
    #     print '|', replace[program],

    #     for (workload, size) in data.items():
    #         print workload, int(size)/1024./1024.

if __name__ == "__main__":
    main()
