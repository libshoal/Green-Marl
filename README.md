The original Green Marl README is stored in README.GM.md

# How to run

1.  `git clone ssh://vcs-user@code.systems.ethz.ch:8006/diffusion/GM/green-marl.git gm -b standalone`
2.  `git submodule init && git submodule update`
3.  Build shoal and dependencies
    - `cd shoal/contrib/numactl-2.0.9 && make -j24`
    - `cd shoal/contrib/papi-5.3.0/src && ./configure && make -j24` (this currently fails for ftests, but that is okay)
4.  Build Green Marl compiler `make sk_clean` or, alternatively, `make compiler -j24`
5.  Build libgm `cd apps/output_cpp/gm_graph/avro-c-1.7.2 && make -j24`, followed by `cd apps/output_cpp/gm_graph && make -j24`
6.  Compile pagerank `make sk_pagerank`
7.  Run ``scripts/run.sh pagerank `nproc` ours soc-LiveJournal1``
    - This requires the  graph to be in `../graphs` relative to the Green Marl directory. From within ETH, copy from `~/skaestle/projects/graphs` on any NFS mounted machine

# 4M-pages

Shoal supports 4M pages. See the shoal documentation on how to setup
machines to use these.

# Testing

We have scripts to test a wide variety of Green Marl programs with
different configurations on Linux. The only thing we really do not
vary is the number of threads used.

Make sure that the master branch is *always* green for all of these
tests.

The tests can be execute like this:

```scripts/test_all.sh```

The scripts takes *only 5 minutes* to execute. Be aware, that some of
the tests might fail depending on the machine, for example, support
for 4M pages must be available.
