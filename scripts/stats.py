#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import fileinput
import re

values = {}

KEYS = [
    'comp',
    'total',
    'copy'
]

def main():

    # Init
    # ------------------------------

    for k in KEYS:
        values[k] = []

    # Parse
    # ------------------------------

    for line in fileinput.input('-'):

        print line.rstrip()

        for s in KEYS:
            search = ('^%s:\s+([0-9\.]+)' % s)
            m = re.match(search, line)
            if m:
                values[s].append(float(m.group(1)))

    # Calculate
    # ------------------------------

    print values

    for (key, vals) in values.items():
        print '[%20s] has %3d entries' % (key, len(vals)),

        if len(vals)>0:
            (m, stderr, median, vmin, vmax) = tools.statistics(vals)
            print '%10.3f %10.3f' % (m, stderr)

if __name__ == "__main__":
    main()
