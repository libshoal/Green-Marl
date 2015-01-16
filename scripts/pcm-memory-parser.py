#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')
sys.path.append(os.getenv('HOME') + '/bin/ploting/')

import tools
import fileinput
import argparse

from skplot import SkPlot, SkData

title = None

# socket 0: 10 11
# socket 1: 22 23

element = None

def parse(title, element, filename=''):

    what = ''

    d = SkData()
    i = 0

    for line in fileinput.input(filename):

        elements = line.rstrip().split(';')

        if i>1:
            d.add_v(elements[element])

        else:
            what += elements[element] + ' '

        i += 1

    print 'processed', i, 'values'

    d.output()
    SkPlot.plot_header('memory utilization %s [%s]' % (what, title), \
                       'time', 'bandwidth [MB/s]')

    print 'writing', '/tmp/plot-%s.png' % title
    SkPlot.finalize('/tmp/plot-%s.png' % title)

def main():

    global title, element

    p = argparse.ArgumentParser()
    p.add_argument('--title')
    p.add_argument('--element', type=int)
    a = p.parse_args()

    title = a.title
    element = a.element

    parse(title, element)

if __name__ == "__main__":
    main()
