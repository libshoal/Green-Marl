#!/usr/bin/env python

import re
import os
import subprocess
import numpy
import fileinput
from datetime import *

## NOTE: We want this file with only dependencies on standart-library things
## Dependencies to own files are an absolut no-go

def parse_sk_m(line):
    """
    Parse sk_m measurements.

    This is a line having this format:     
    $ sk_m_print(6,shm_fifo) idx= 249 tscdiff= 1969

    @return None, in case the lines is not an sk_m, a tuple (core, title, idx, tscdiff) otherwise

    """
    m = re.match('sk_m_print\((\d+),(\S+)\)\s+idx=\s*(\d+)\s+tscdiff=\s*(\d+)', line)
    if m:
        core = int(m.group(1))
        title = m.group(2)
        idx = int(m.group(3))
        tscdiff = int(m.group(4))
        return (core, title, idx, tscdiff)

    else:
        return None

def parse_sk_m_input(stream):
    """
    Read all lines from given handle and execute parse_sk_m on all of them.

    @return: A list of tuples ((core, title), [values .. ])

    """
    d = {}

    for l in stream.readlines():
        o = parse_sk_m(l)
        if not o:
            continue
        (core, title, idx, tscdiff) = o
        if not (core,title) in d:
            d[(core,title)] = [ tscdiff ]
        else:
            d[(core,title)].append(tscdiff)

    return d.items()

    
def statistics_cropped(l, r=.1):
    """
    Print statistics for the given list of integers
    @param r Crop ratio. .1 corresponds to dropping the 10% highest values
    @return A tuple (mean, stderr, min, max, median)
    """
    if not isinstance(l, list) or len(l)<1:
        return None

    crop = int(len(l)*r)
    m = 0
    for i in range(crop):
        m = 0
        for e in l:
            m = max(m, e)
        l.remove(m)

    return statistics(l)


def statistics(l):
    """
    Print statistics for the given list of integers
    @return A tuple (mean, stderr, median, min, max)
    """
    if not isinstance(l, list) or len(l)<1:
        return None

    nums = numpy.array(l)

    m = nums.mean(axis=0)
    median = numpy.median(nums, axis=0)
    d = nums.std(axis=0)

    return (m, d, median, nums.min(), nums.max())


def latex_header(f, args=[]):
    header = (
        "\\documentclass[a4wide]{article}\n"
        "\\usepackage{url,color,xspace,verbatim,subfig,ctable,multirow,listings}\n"
        "\\usepackage[utf8]{inputenc}\n"
        "\\usepackage[T1]{fontenc}\n"
        "\\usepackage{txfonts}\n"
        "\\usepackage{rotating}\n"
        "\\usepackage{paralist}\n"
        "\\usepackage{subfig}\n"
        "\\usepackage{graphics}\n"
        "\\usepackage{enumitem}\n"
        "\\usepackage{times}\n"
        "\\usepackage{amssymb}\n"
        "\\usepackage[colorlinks=true]{hyperref}\n"
        "\\usepackage[ruled,vlined]{algorithm2e}\n"
        "\n"
        "\\graphicspath{{figs/}}\n"
        "\\urlstyle{sf}\n"
        "\n"
        "\\usepackage{tikz}\n"
        "\\usepackage{pgfplots}\n"
        "\\usetikzlibrary{shapes,positioning,calc,snakes,arrows,shapes,fit,backgrounds}\n"
        "\n"
        "%s\n"
        "\\begin{document}\n"
        "\n"
        ) % '\n'.join(args)
    f.write(header)


def _pgf_footer(f):
    s = ("  \\end{tikzpicture}\n"
         "\\end{figure}\n")
    f.write(s)


def _pgf_header(f, caption='TODO', label='TODO'):
    s = (("\\begin{figure}\n"
          "  \\caption{%s}\n"
          "  \\label{%s}\n"
          "  \\begin{tikzpicture}[scale=.75]\n")
         % (caption, label))
    f.write(s)


def latex_footer(f):
    footer = (
        "\n"
        "\\end{document}\n"
        )
    f.write(footer)


def _pgf_plot_header(f, plotname, caption, xlabel, ylabel, attr=[], desc='...'):
    label = "pgfplot:%s" % plotname
    s = (("Figure~\\ref{%s} shows %s\n"
          "\\pgfplotsset{width=\linewidth}\n") % (label, desc))
    if xlabel:
        attr.append('xlabel={%s}' % xlabel)
    if ylabel:
        attr.append('ylabel={%s}' % ylabel)
    t = ("    \\begin{axis}[%s]\n") % (','.join(attr))
    f.write(s)
    _pgf_header(f, caption, label)
    f.write(t)


def _pgf_plot_footer(f):
    f.write("    \\end{axis}\n")
    _pgf_footer(f)


def do_pgf_3d_plot(f, datafile, caption='', xlabel=None, ylabel=None, zlabel=None):
    """
    Generate PGF plot code for the given data
    @param f File to write the code to
    @param data Data points to print as list of tuples (x, y, err)
    """
    attr = ['scaled z ticks=false',
            'z tick label style={/pgf/number format/fixed}']
    if zlabel:
        attr.append('zlabel={%s}' % zlabel)
    now = datetime.today()
    plotname = "%02d%02d%02d" % (now.year, now.month, now.day)
    _pgf_plot_header(f, plotname, caption, xlabel, ylabel, attr)
    f.write(("    \\addplot3[surf] file {%s};\n") % datafile)
    _pgf_plot_footer(f)


def run_pdflatex(fname, openFile=True):
    d = os.path.dirname(fname)
    if d == '':
        d = '.'
    print 'run_pdflatex in %s' % d
    if subprocess.call(['pdflatex',
                     '-output-directory', d,
                     '-interaction', 'nonstopmode', '-shell-escape',
                        fname], cwd=d) == 0:
        if openFile:
            subprocess.call(['okular', fname.replace('.tex', '.pdf')])

