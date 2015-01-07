#include <iostream>

// compile: cd ~/projects/gm/scripts/ && g++ -I../shoal/contrib/numactl-2.0.9/ -L../shoal/contrib/numactl-2.0.9/ -lnuma core2node.cc -o core2node

using namespace std;

#include <numa.h>
#include <cassert>
#include <cstdio>

int
shl__node_from_cpu(int cpu)
{
    int ret    = -1;
    int ncpus  = numa_num_possible_cpus();
    int node_max = numa_max_node();
    struct bitmask *cpus = numa_bitmask_alloc(ncpus);

    for (int node=0; node <= node_max; node++) {
        numa_bitmask_clearall(cpus);
        if (numa_node_to_cpus(node, cpus) < 0) {
            perror("numa_node_to_cpus");
            fprintf(stderr, "numa_node_to_cpus() failed for node %d\n", node);
            abort();
        }

        if (numa_bitmask_isbitset(cpus, cpu)) {
            ret = node;
        }
    }

    numa_bitmask_free(cpus);
    if (ret == -1) {
        fprintf(stderr, "%s failed to find node for cpu %d\n",
                __FUNCTION__, cpu);
        abort();
    }

    return ret;
}

int main(int argc, char **argv)
{
    assert (argc>1);
    printf ("%d", shl__node_from_cpu(atoi(argv[1])));

    return 0;
}
