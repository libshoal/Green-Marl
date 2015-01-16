#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')
sys.path.append(os.getenv('HOME') + '/bin/ploting/')

import tools
import fileinput
import argparse
import re

from skplot import SkPlot, SkData
import matplotlib.pyplot as plt

BASE=os.getenv('HOME') + '/papers/oracle/measurements/pcm-numa/'

# This is what the format is:
# Core,IPC,Instructions,Cycles,Local DRAM accesses,Remote DRAM accesses
#      0   1            2      3                   4

# core -> (ipc, local, remote)
data = {}

# element = None
# title = None
# what = ''

def parse(conf):

    # Generate file name for this measurement
    filename = BASE + 'pcm-numa-trace-' + conf + '.cvs'

    d = SkData()
    i = 0

    # Parse result file
    # --------------------------------------------------

    print 'Parsing file', filename

    for line in fileinput.input(filename):

        elements = line.rstrip().split(',')
        if re.match('^[0-9]', line):

            idx = int(elements[0])

            if not idx in data:
                data[idx] = []

            data[idx].append([elements[1], elements[2], elements[3],
                                      elements[4], elements[5]])

        i += 1

    print ' .. processed', i, 'lines'


    # Plot for the following cores
    # --------------------------------------------------

    # Create plots
    for (idx, title, limit) in [(0, 'IPC',   None),
                                (3, 'local',  15000000),
                                (4, 'remote', 15000000)]:

        print ' .. creating plot for', title

        # One line per each of these cores
        for core in [0, 8, 16, 24]:

            print ' .. processing core %d' % core
            i = 0

            dbuffer = SkData()
            dbuffer.set_plot_label('core %d' % core)

            for value in data[core]:
                dbuffer.add_v(value[idx])
                i += 1

            print ' .. ploting %d elements' % i
            if limit:
                plt.ylim(ymax=limit)
            dbuffer.output()

        outname = '/tmp/plot-%s-%s.png' % (conf, title)

        print ' .. rendering plot to %s ' % outname

        # Generate
        SkPlot.plot_header('PCM NUMA %s %s' % (conf, title), 'time', '')
        SkPlot.finalize(outname)
        SkPlot.reset()

        print ' .. done'

def main():

    # global title, element

    # p = argparse.ArgumentParser()
    # p.add_argument('--title')
    # p.add_argument('--element', type=int)
    # a = p.parse_args()

    # title = a.title
    # element = a.element

    for conf in [ '', 'd', 'r', 'dr' ]:
        parse(conf)

if __name__ == "__main__":
    main()
