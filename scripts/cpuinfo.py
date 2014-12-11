#!/usr/bin/env python

import sys
import os
import re
import argparse

import fileinput
sys.path.append(os.getenv('HOME') + '/bin/')

import tools

class CPUInfo:

    def __init__(self):
        self.proc = None
        self.socket = None
        self.proc2phy = {}


    def parse_line(self, line):
        # Processor ID as used by Linux
        m = re.match('^processor\s*:\s(\d+)', line)
        if m:
            self.proc = int(m.group(1))

        # ID of socket
        m = re.match('^physical id\s*:\s(\d+)', line)
        if m:
            self.socket = int(m.group(1))

        # ID of core on that socket
        m = re.match('^core id\s*:\s(\d+)', line)
        if m:
            self.proc2phy[self.proc] = (self.socket, int(m.group(1)))
            print "Adding", self.proc, (self.socket, int(m.group(1)))
            self.socket = None
            self.proc = None


    def get_sockets(self):

        return sorted(set([ s for (t, (s, c)) in self.proc2phy.items() ]))

    def get_cores(self):

        return sorted(set([ (s, c) for (t, (s, c)) in self.proc2phy.items() ]))

    def get_phy_cores(self):

        res = []

        for (ss,sc) in self.get_cores():

            threads = [ t for (t, (s,c)) in self.proc2phy.items() if (s,c) == (ss,sc) ]
            assert len(threads)==2

            res.append(sorted(threads)[0])

        return sorted(res)

    def get_num_sockets(self):

        return len(self.get_sockets())

    def get_num_cores_per_socket(self):

        return len(self.get_cores())/self.get_num_sockets()

    def get_affinity(self):
        """
        Generate affinity string to be used with OpenMP and shoal.

        We will sockets first, using only one thread per physical
        core. Then, we add hyperthreads.

        Assumptions: sockets are named (0,..,n), where n is what is returned
        by get_sockets(), and cores are named (0,..,m) where m is the result of
        get_cores()

        """
        res = [] # list of cores that are already included

        # Add the first core on each socket
        for ss in range(self.get_num_sockets()):
            for sc in range(self.get_num_cores_per_socket()):

                threads = [ t for (t, (s,c)) in self.proc2phy.items() if (s,c) == (ss,sc) ]
                assert len(threads)==2

                res.append(sorted(threads)[0])

        # Add the remaining (i.e. second) core on that socket
        for ss in range(self.get_num_sockets()):
            for sc in range(self.get_num_cores_per_socket()):
                for t in [ t for (t, (s,c)) in self.proc2phy.items() if (s,c) == (ss,sc) ]:
                    if not t in res:
                        res.append(t)

        return res


def main():

    info = CPUInfo()



    # Read from stdin
    for l in fileinput.input('-'):
        info.parse_line(l)

    print 'Core mapping: ', str(info.proc2phy)
    print 'Number of sockets: ', len(info.get_sockets())
    print 'Number of physical cores:', len(info.get_phy_cores())

    print 'Sockets: ', ','.join(map(str, info.get_sockets()))

    print 'Physical cores are (socket, coreid)', \
        ' '.join(map(str, info.get_cores()))

    print 'Linux\'s physical cores are:', \
        ','.join(map(str, info.get_phy_cores()))

    print 'Affinity mask: ', \
        ','.join(map(str, info.get_affinity()))


if __name__ == "__main__":
    main()
