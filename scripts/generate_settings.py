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

def write_dmadevice(f, label, count, bus, dev, fun) :
	f.write('%s = {\n' % label)
	f.write('    count=%u, \n    bus=%u, \n    dev=%u, \n    fun=%u\n' % (count, bus, dev, fun))
	f.write('}\n')



parser = argparse.ArgumentParser(description='Generating a settings.lua file for the benchmark')
parser.add_argument('-C', help="Enable CRC check")
parser.add_argument('-D', help="Global Distribution enable")
parser.add_argument('-R', help="Global Replication enable")
parser.add_argument('-P', help="Global Partitioning enable")
parser.add_argument('-H', help="Global Huge Pages enable")
parser.add_argument('-T', help="Global NUMA TRIM enable")
parser.add_argument('-S', help="Global OpenMP Static Schedule")
parser.add_argument('-W', help="Global Distribution Stride Size")
parser.add_argument('-A', help="Enabling asynchronous memcopy using DMA")
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

write_setting(outfile, "distribution", int(args.D) if args.D else 0)
write_setting(outfile, "replication", int(args.R) if args.R else 0)
write_setting(outfile, "partitioning", int(args.P) if args.P else 0)
write_setting(outfile, "hugepage", int(args.H) if args.H else 0)
write_setting(outfile, "trim", int(args.T) if args.T else 0)
write_setting(outfile, "stride", int(args.W) if args.W else 4096)
write_setting(outfile, "crc", int(args.C) if args.C else 1)
write_settinglast(outfile, "static", int(args.S) if args.S else 0)
outfile.write('}\n')

outfile.write('-- dma configuration\n')
outfile.write('dma = {\n')
write_setting(outfile, "enable",  int(args.A) if args.A else 0)
if int(args.A) == 1 :
	# Haswell IOAT Device
	# write_setting(outfile, "device", 0x2f20)
	# the number of DMA device (consecutive device IDs)

	write_setting(outfile, "vendor", 0x8086)
	# Sandy Bridve / Ivy Bridge IOAT DMA Device
	write_setting(outfile, "device", 0x0e20)
	write_settinglast(outfile, "device_count", 2)

outfile.write('}\n')
if int(args.A) == 1 :
	write_dmadevice(outfile, "dma_device_0", 8, 0, 4, 0)
	write_dmadevice(outfile, "dma_device_1", 8, 0x80, 4, 0)


outfile.close()
