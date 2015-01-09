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
        },

        'test': {

            'CRC': '0xe544'
        }
    },
    'triangle_counting': {

    }

}

# Line checkers
# --------------------------------------------------

class LineChecker:
    """
    Check shoal output line-by-line

    """

    def __init__(self, workload, program):
        self.workload = workload.replace('.bin', '')

        # Allows working copies of programs to still be sanity checked, e.g.:
        # - hop_dist.ec
        # - pagerank.team
        self.program = program.split('.')[0]

        self.program = self.program.replace('_ec', '')

        self.checked = False
        self.correct = True

    def check_line(self, line):
        pass

    def summarize(self):
        pass

# --------------------------------------------------

class CRCChecker(LineChecker):
    """
    Check if CRC checksum of arrays match

    """

    KEY = 'CRC'
    ARRNAME = 'CRCARR'

    def check_line(self, line):

        if not self.ARRNAME in validate[self.program]:
            return

        l = re.match('^CRC shl__%s ([0-9xa-fA-Fn\.]*)' % validate[self.program][self.ARRNAME], line)
        if l:
            print 'Found CRC output', line, l.group(1)
            correct_output = validate[self.program][self.workload][self.KEY]
            self.checked = True
            self.correct &= (correct_output == l.group(1) or l.group(1) == "n.a.")

# --------------------------------------------------

class ArrayConfChecker(LineChecker):
    """
    Check array configuration

    """

    arrays = []

    def check_line(self, line):

        # Array[   shl__G_r_node_idx]: elements=  68993774-68.99 M size= 275975096-275.98 M -- hugepage=[ ]  -- used=[X] replication=[X]
        l = re.match('^Array\[([^\]]*)\].*size=\s*(\d+)', line)
        if l:
            print 'Found array configuration', l.group(1), l.group(2)

            self.arrays.append({
                'name': l.group(1),
                'size': l.group(2),
                'distribution': ('distribution=[X]' in line),
                'hugepage':     ('hugepage=[X]' in line),
                'replication':  ('replication=[X]' in line),
                'single-node':  ('single-node=[X]' in line),
                'partition':    ('partition=[X]' in line),
            })

    def summarize(self):

        for s in [ 'hugepage' ]:
            print s,
            for arr in self.arrays:
                print ("%s=%d" % (arr['name'], arr[s])),

            print

        for a in self.arrays:
            print str(a)



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
parser.add_argument('-rawfile', help="Tells the script that the result file should be taken instead of stdin")
parser.add_argument('-barrelfish', help="Barrelfish OS")
args = parser.parse_args()

if args.workload:
    workload = os.path.basename(args.workload)
    print 'Workload is:', args.workload, workload

if args.program:
    program = os.path.basename(args.program)
    print 'Program is:', args.program, program

use_file_input=False
if args.rawfile :
    rawfile=args.rawfile
    print 'rawfile is:', args.rawfile, rawfile
    use_file_input=True

if args.barrelfish :
    barrelfish=args.barrelfish
    print 'Barrelfish Results from file'
else :
    print 'Linux Result from STDIN'

baseline = False
if 'org_gm' in args.program:
    baseline = True
result = True

crc_checker = CRCChecker(workload, program)
array_checker = ArrayConfChecker(workload, program)

checkers = [ crc_checker, array_checker ]

if use_file_input :
    try:
        inFile = open(rawfile, 'r')
    except IOError as e:
        print rawfile,": I/O error({0}): {1}".format(e.errno, e.strerror)
        exit (1);

copy = 0.0
computation=0.0

while 1:
    if use_file_input :
        line = inFile.readline()
    else :
        line = sys.stdin.readline()
    if not line: break
    print '[OUT]', line.rstrip()
    lines += 1
    # Copy
    # --------------------------------------------------
    l = re.match('([a-zA-Z_]*): copy_from ([ ]*)([0-9.]*)', line)
    if l:
        copy += float(l.group(3))
    # Copy back
    # --------------------------------------------------
    l = re.match('SHOAL_Copyback ([ ]*)([0-9.]*)', line)
    if l:
        copy += float(l.group(2))
    # Allocate
    # --------------------------------------------------
    l = re.match('([a-zA-Z_]*): allocate ([ ]*)([0-9.]*)', line)
    if l:
        copy += float(l.group(3))
    l = re.match('SOAL_Computation ([ ]*)([0-9.]*)', line)
    if l:
        computation += float(l.group(2))
    # Extract runtime
    l = re.match('running time=([0-9.]*)', line)
    if l:
        total = float(l.group(1))
    # Extract CRC time
    l = re.match('t_crc: ([0-9.]*)', line)
    if l:
        t_crc = float(l.group(1))

    # check result output
    # --------------------------------------------------
    result = result and \
        verify_hop_dist(line) and \
        verify_pagerank(line) and \
        verify_triangle_counting(line)

    # Check CRC
    # --------------------------------------------------
    for checker in checkers:
        checker.check_line(line)

if total and copy and computation and t_crc:
    print 'copy:     %10.5f' % copy
    print 'comp:     %10.5f' % computation
    print 'total:    %10.5f' % (copy + computation)
    print 'gmtotal:    %10.5f' % total
    print 'crc:    %10.5f' % t_crc

for checker in checkers:
    checker.summarize()


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

if result and (
        (crc_checker.correct and crc_checker.checked) or
        program == "triangle_counting" or
        baseline):  # triangle_counting does not have checksums
    exit(0)

exit(1)
