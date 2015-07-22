The original Green Marl README is stored in README.GM.md

# How to run

This guide assumes that you already downloaded and installed [shoal](https://github.com/libshoal/shoal).

1.  Clone the repository: `git clone git@github.com:libshoal/Green-Marl.git gm` in the same folder then the Shoal library (i.e. `libshoal/` and `gm/` are in the same directory)
2.  Make Shoal available in Green Marl - this assumes that Shoal is already setup in the top-level directory relative to Green Marl: `ln -s ../libshoal shoal`
3.  Install Green Marl's dependencies as listed in `README.GM.md`, Section `3-1`.
3.  Configure `GM_TOP` in `setup.mk` to point to Green Marl's root directory
4.  Build the Green Marl compiler `make compiler -j24`
5.  Build the runtime library, libgm, `$(cd apps/output_cpp/gm_graph/avro-c-1.7.2 && make -j24)`, followed by `cd apps/output_cpp/gm_graph; make -j24; cd ../../..`
6.  Compile pagerank: `make sk_pagerank`
7.  Run pagerank: `scripts/run.sh  pagerank $(nproc) ours soc-LiveJournal1`. Graphs have to be stored in `../graphs/` relative to the Green Marl directory.

# 4M-pages

Shoal supports 4M pages. See the shoal documentation on how to setup
machines to use these.
