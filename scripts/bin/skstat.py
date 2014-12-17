#!/usr/bin/env python
from numpy import *

import math
import fileinput
import sys
import argparse

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools

def crop_list(l, r=.1):
    """
    Crop the given list of integers
    @param r Crop ratio. .1 corresponds to dropping the 10% highest values
    @return Croped list
    """
    if not isinstance(l, list) or len(l)<1:
        return None

    crop = int(math.floor(len(l)*r))
    m = 0
    for i in range(crop):
        m = 0
        for e in l:
            m = max(m, e)
        l.remove(m)

    return l

def read_floats(f):
    v = []
    num=0
    num_fail=0

    try:
        for line in f:
            try:
                v.append(float(line))
                num+=1
            except ValueError:
                num_fail+=1
    except IOError:
        print "Failed to read input values from stdin"
        sys.exit(-1)

    return (v, num, num_fail)

def output(m, d, dmin, dmax, median, prefix=''):
        print "%s %.1f %.1f %.1f %.1f %.1f" % (prefix, m, d, dmin, dmax, median)
    

def main():
    """
    Basic statistical analysis of values given on stdin
    """
    parser = argparse.ArgumentParser(description=(
            'Basic statistics for list of integeres given as argument'))
    parser.add_argument('--crop', type=float, nargs='?', default=0.0,
                        help=('Remove given percentage of values from '
                              'upper range of values'))
    parser.add_argument('--skm', action='store_const', default=False, const=True)
    parser.add_argument('--max', action='store_const', default=False, const=True,
                        help='Take maximum out of the several values of skm')
    args = parser.parse_args()

    if (args.skm):
        r = tools.parse_sk_m_input(sys.stdin)
        if not r:
            print "No measurements to parse"
            return 1

        res = []

        # Do stats
        for ((core, title), values) in r:
            res.append((core, do_stat(values, args.crop)))

        if args.max:
            res = [ max(res, key=lambda x: x[1][0]) ]
        else:
            res = sorted(res, key=lambda x: x[1][0])

        # Do output
        for (core, (m, d, dmin, dmax, median)) in res:
            output(m, d, dmin, dmax, median, 'measurement [%s] core [%02d] num [%d]:' 
                   % (title, core, len(values)))

        return 0

    else:
        (v, num, num_fail) = read_floats(sys.stdin)
        (m, d, dmin, dmax, median) = do_stat(v, args.crop)
        output(m, d, dmin, dmax, median)

        # Report errors to user
        if num_fail>0:
            print >> sys.stderr, "%d lines could not be interpreted as numbers, " \
                "this is %f" % (num_fail, (float(num_fail)/float(num)))

def do_stat(v, crop=0.0):

    v = crop_list(v, crop)

    if not isinstance(v, list) or len(v)<1:
        print "Usage: do some statistical analysis on the data read from stdin."
        print "No elements received via stdin"
        print "Output format: mean std min max median"
        sys.exit(-1)

    nums = array(v)

    m = nums.mean(axis=0)
    d = nums.std(axis=0)

    return (m, d, \
                nums.min(), \
                nums.max(), \
                median(nums), \
                )

if __name__ == "__main__":
    exit(main())

