#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import re
import fileinput
import argparse

# http://stackoverflow.com/a/287944/491709
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

# Time for copy: 228.052000
# running time=3207.233000

total=None
copy=None

lines = 0

def print_warning(s):
    print 'Warning:', s

verified = False
verifiedCRC = False

# Correct output for hop_dist (DEPRECATED)
# --------------------------------------------------
hd_res = {
    '?': {
        0:  0,
        1:  15,
        2:  17,
        3:  18,
        4:  16,
        5:  17,
        6:  16,
        7:  14,
        8:  15,
        9:  16
    },
    'soc-LiveJournal1.bin': {
        0:  0,
        1:  1,
        2:  1,
        3:  1,
        4:  1,
        5:  1,
        6:  1,
        7:  1,
        8:  1,
        9:  1,
    },
    'twitter_rv.bin': {
        0:  0,
        1:  1,
        2:  1,
        3:  1,
        4:  1,
        5:  1,
        6:  1,
        7:  1,
        8:  1,
        9:  1,
        'CRC': '0x5564'
    }
}

# New array to hold correct output
# --------------------------------------------------
validate = {

    'hop_dist': {

        'CRCARR': 'G_dist',

        'soc-LiveJournal1': {
            'CRC' : '0xe68c',
        },

        'twitter_rv': {

            'CRC' : '0x5564'
        }
    },
    'pagerank': {

        'CRCARR': 'G_pg_rank',

        'soc-LiveJournal1': {
            'CRC' : '0x28b5',
        },

        'twitter_rv': {

            'CRC' : '0x82ed' # '0x5564'
        }
    },
    'triangle_counting': {

    }

}


class LineChecker:

    def __init__(self, workload, program):
        self.workload = workload.replace('.bin', '')
        self.program = program.replace('_ec', '')
        self.checked = False
        self.correct = True

    def check_line(self, line):
        pass

class CRCChecker(LineChecker):

    KEY = 'CRC'
    ARRNAME = 'CRCARR'

    def check_line(self, line):

        if not self.ARRNAME in validate[self.program]:
            return

        l = re.match('^CRC %s ([0-9xa-fA-F]*)' % validate[self.program][self.ARRNAME], line)
        if l:
            print 'Found CRC output', line, l.group(1)
            correct_output = validate[self.program][self.workload][self.KEY]
            self.checked = True
            self.correct &= correct_output == l.group(1)



# Correct output for triangle_counting
# --------------------------------------------------
tc_res = {
    'soc-LiveJournal1.bin': 132775101,
    'tiny.bin': 179
}

# Correct output for pagernak (DEPRECATED)
# --------------------------------------------------
pr_res = {
    'huge.bin': {
        0: 0.000000002,
        1: 0.000000006,
        2: 0.000000006,
        3: 0.000000003
        },
    'test.bin': {
        0: 0.000000928,
        1: 0.000001235,
        2: 0.000000838,
        3: 0.000000673
        },
    'big.bin': {
        0: 0.000000098,
        1: 0.000000043,
        2: 0.000000039,
        3: 0.000000155,
        },
    'soc-LiveJournal1.bin': {
        0: 0.000001174,
        1: 0.000004149,
        2: 0.000002173,
        3: 0.000001640,
        },
    'twitter_rv.bin': {
        0: 0.000000065,
        1: 0.000000021,
        2: 0.000000014,
        3: 0.000000031
        }
}
workload = None
program = None

def verify_triangle_counting(line):
    global verified
    global verifiedCRC
    if not args.workload:
        return True
    l = re.match('^number of triangles: ([0-9]+)', line)
    if l:
        res = int(tc_res[workload])
        verified = True
        return res == int(l.group(1))
    else:
        return True

def verify_hop_dist(line):
    global verified
    global verifiedCRC
    if not args.workload:
        return True
    if not workload in hd_res:
        return True
    l = re.match('^dist\[([0-9]*)\] = ([0-9]*)', line)
    # CRC dist is: 0x5564
    l2 = re.match('^CRC dist is: ([0-9x]*)', line)
    if l:
        res = hd_res[workload].get(int(l.group(1)), None)
        if not res == int(l.group(2)):
            print 'Result for', int(l.group(1)), 'is', int(l.group(2)), 'expecting', res
        verified = True
        return res == int(l.group(2))
    elif l2:
        if not 'CRC' in hd_res[workload]:
            return True

        verifiedCRC = True
        return hd_res[workload].get('CRC')
    else:
        return True

# Sample output line:
# rank[0] = 0.000000002
def verify_pagerank(line):
    l = re.match('^rank\[([0-9]*)\] = ([0-9.]*)', line)
    if not args.workload:
        return True
    if l:
        if not workload in pr_res:
            print_warning(('Output for workload [%s] not defined', workload))
            return True
        res = pr_res[workload].get(int(l.group(1)), None)
        print 'Result for', int(l.group(1)), 'is', float(l.group(2)), 'expecting', res
        global verified
        global verifiedCRC
        verified = True
        return res == float(l.group(2))
    else:
        return True

parser = argparse.ArgumentParser(description='Extract results')
parser.add_argument('-workload', help="Workload that is running. This is used to check the result")
parser.add_argument('-program', help="Program that is running. This is used to check the result")
args = parser.parse_args()

if args.workload:
    workload = os.path.basename(args.workload)
    print 'Workload is:', args.workload, workload

if args.program:
    program = os.path.basename(args.program)
    print 'Program is:', args.program, program

result = True

crc_checker = CRCChecker(workload, program)

while 1:
    line = sys.stdin.readline()
    if not line: break
    print '[OUT]', line.rstrip()
    lines += 1
    # Extract time for copy
    l = re.match('Time for copy: ([0-9.]*)', line)
    if l:
        copy = float(l.group(1))
    # Extract runtime
    l = re.match('running time=([0-9.]*)', line)
    if l:
        total = float(l.group(1))

    # check result output
    # --------------------------------------------------
    result = result and \
        verify_hop_dist(line) and \
        verify_pagerank(line) and \
        verify_triangle_counting(line)

    # Check CRC
    # --------------------------------------------------
    crc_checker.check_line(line)

if total and copy:
    print 'total:    %10.5f' % total
    print 'copy:     %10.5f' % copy
    print 'comp:     %10.5f' % (total-copy)


if not verified:
    result_out = bcolors.WARNING + "could not verify result" + bcolors.ENDC
elif result:
    result_out = bcolors.OKGREEN + "correct" + bcolors.ENDC
else:
    result_out = bcolors.FAIL + "incorrect" + bcolors.ENDC

result_out += ' CRC '

if not crc_checker.checked:
    result_out += bcolors.WARNING + "could not verify result" + bcolors.ENDC
elif crc_checker.correct:
    result_out += bcolors.OKGREEN + "correct" + bcolors.ENDC
else:
    result_out += bcolors.FAIL + "incorrect" + bcolors.ENDC

print 'lines processed:', lines, '- result' , result_out

if result and (crc_checker.correct or not crc_checker.checked):
    exit(0)

exit(1)
