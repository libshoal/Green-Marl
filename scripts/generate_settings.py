#!/usr/bin/env python

import sys
import os
import fileinput
import argparse

DEFAULT_PAGE_SIZE = 4096

OUTPUT_FILENAME = os.path.join(os.path.dirname(__file__), "array_settings.lua")

def write_setting(f, label, value) :
	print label, value
	f.write('    %s = %u,\n' % (label, value))

def write_settinglast(f, label, value) :
	f.write('    %s = %u\n' % (label, value))



parser = argparse.ArgumentParser(description='Generating a settings.lua file for the benchmark')
parser.add_argument('-D', help="Global Distribution enable")
parser.add_argument('-R', help="Global Replication enable")
parser.add_argument('-P', help="Global Partitioning enable")
parser.add_argument('-H', help="Global Huge Pages enable")
parser.add_argument('-T', help="Global NUMA TRIM enable")
parser.add_argument('-S', help="Global OpenMP Static Schedule")
parser.add_argument('-W', help="Global Distribution Stride Size")
parser.add_argument('-o', help="Output File")
args = parser.parse_args()

if args.o :
	outfilename=args.o
else :
	outfilename=OUTPUT_FILENAME

print "Output File: ", outfilename

try :
	outfile = open(outfilename,'w')
except IOError as e:
    print outfile,": I/O error({0}): {1}".format(e.errno, e.strerror)
    exit (1);


outfile.write('-- global configuration\n') 
outfile.write('global = {\n') 

write_setting(outfile, "distribution", 1 if args.D else 0)
write_setting(outfile, "replication", 1 if args.R else 0)
write_setting(outfile, "partitioning", 1 if args.P else 0)
write_setting(outfile, "hugepages", 1 if args.H else 0)
write_setting(outfile, "trim", 1 if args.T else 0)
write_setting(outfile, "stride", args.W if args.W else DEFAULT_PAGE_SIZE)
write_settinglast(outfile, "static", 1 if args.T else 0)

outfile.write('}\n') 

outfile.close() 