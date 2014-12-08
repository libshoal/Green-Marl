#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools

import subprocess
import re

WORKLOAD='soc-LiveJournal1'
#WORKLOAD='twitter_rv'
#WORKLOAD='test'

PROG='pagerank'
#PROG='triangle_counting'
#PROG='hop_dist'


OUTPUT='%s/papers/oracle/measurements/' % os.getenv('HOME')

def generate_combinations(depth):
    assert depth > 0
    if (depth==1):
        return [ [0], [1] ]
    else:
        r = []
        for x in generate_combinations(depth-1):
            r.append(x+[0])
            r.append(x+[1])
        return r

def generate_conf_combinations(arrays, setting):

    r = subprocess.check_output(['mktemp', '/tmp/run_all-overview-XXXXX']).rstrip()
    print r

    file_list = []
    file_list.append(r)

    f_overview = open(r, 'w')

    for conf in generate_combinations(len(arrays)):

        out = open('settings.lua', 'w')
        out.write("settings = {\n")
        out.write("arrays = {\n")

        f_run_name = subprocess.check_output(['mktemp', '/tmp/tmp-run_all-XXXXXX']).rstrip()
        file_list.append(f_run_name)

        conf_string = ''.join([ '1' if hp else '0' for hp in conf ])

        for i in range(len(arrays)):
            arr = arrays[i]
            hp = conf[i]
            out.write("%s = {\n" % arr)
            out.write("%s = %s\n" % (setting, ('true' if hp else 'false')))
            out.write("},\n")

        out.write("}\n")
        out.write("}\n")

        res = execute('scripts/run.sh %s %s `nproc` ours %s' %
                      ('-d -r -h', PROG , WORKLOAD), 3, f_run_name)

        f_overview.write('%s x x x x %s\n' % (conf_string, f_run_name))

    f_overview.close()
    subprocess.check_call(['tar', '-czf', 'output.tgz'] + file_list)


def generate_pagerank_hugepages():

        arrays = [ 'G_begin',
                   'G_pg_rank',
                   'G_r_begin',
                   'G_r_node_idx',
                   'G_pg_rank_nxt' ]

        generate_conf_combinations(arrays, 'hugepage')

def execute(cmd, num=1, fName=None):

    print "Executing [%s], writing to  [%s]" % (cmd, fName)
    res = None

    try:

        if fName:
            print "Writing output to", fName
            fOut = open(fName, 'a')
            fOut.write("Starting benchmark .. ")

        for iteration in range(num):

            # Execute process
            proc = subprocess.Popen(cmd, shell=True,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)

            # Parse output
            while True:
                line = proc.stdout.readline()
                if line != '':
                    if fName:
                        fOut.write(line)
                    print '[BENCH%d] %s' % (not not fName, line.rstrip())
                    m = re.match('^Files are: (\S+) (\S+) (\S+)', line)
                    if m:
                        print 'Output files are: %s %s %s' % (m.group(1), m.group(2), m.group(3))
                        res = (m.group(1), m.group(2), m.group(3))
                else:
                    # Break if no line could be read
                    break

        if fName:
            print "Closing file", fName
            fOut.close()

        # Wait for process to terminate
        proc.wait()
        print 'benchmark terminate with return code %d' % proc.returncode
        return res

    except KeyboardInterrupt:
        print 'Killing benchmark'
        subprocess.Popen.kill(proc)
        print 'Done'

        return None

# for conf in [ "", "-d", "-d -r", "-d -r -p", "-d -r -p -h" ]:
#     res = execute('scripts/run_remote.sh %s ours %s pr "%s"' %
#                   (PROG, WORKLOAD, conf))

#     # total, comp, init
#     conf_short = ''.join([ x for x in conf if re.match('[a-z]', x) ])

#     for (num,name) in [(0, "total"), (1, "comp"), (2, "init")]:
#         subprocess.check_call(['cp', res[num], '%s/%s_%s_%s_%s_%s' %
#                                (OUTPUT, WORKLOAD, MACHINE, PROG, conf_short, name)])

generate_pagerank_hugepages()
