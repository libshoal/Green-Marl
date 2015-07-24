The original Green Marl README is stored in README.GM.md

# How to run

This guide assumes that you already downloaded and installed [shoal](https://github.com/libshoal/shoal).

1.  Clone the repository: `git clone git@github.com:libshoal/Green-Marl.git gm` in the same folder then the Shoal library (i.e. `libshoal/` and `gm/` are in the same directory)
2.  Make Shoal available in Green Marl: `cd gm && ln -s ../libshoal shoal`
3.  Install Green Marl's dependencies as listed in [README.GM.md](README.GM.md), Section `3-1`.
3.  Configure `GM_TOP` in `setup.mk` to point to Green Marl's root directory
4.  Build the Green Marl compiler `make compiler -j24` - this includes Shoal's compiler extensions
5.  Build the runtime library, libgm, `$(cd apps/output_cpp/gm_graph/avro-c-1.7.2 && make)`, followed by `cd apps/output_cpp/gm_graph; make; cd ../../..`
6.  Compile pagerank: `make sk_pagerank`
7.  Run pagerank: `scripts/run.sh  pagerank $(nproc) ours soc-LiveJournal1`. Graphs have to be stored in `../graphs/` relative to the Green Marl directory.

# Acquiring a graph

In the paper, we used the Twitter and LiveJournal graphs as workload for evaluation.

## Twitter

ToDo

## LiveJournal

A smaller graph is LiveJournal, which can be acquired from
    [here](http://snap.stanford.edu/data/soc-LiveJournal1.html).

The unzipped version of the graph is a plain-text representation. For
Green Marl, we convert this to a binary representation with the
integrated converter:

`apps/output_cpp/bin/gm_format_converter soc-LiveJournal1.txt soc-LiveJournal1.bin schema -GMInputFormat=EDGE`

The md5 checksum for our version of the LiveJournal graph is: `8447a521d15781478a46e97ee2fcac60`

# Note on 4M-pages

Shoal supports 4M pages. See the shoal documentation on how to setup
machines to use these.
