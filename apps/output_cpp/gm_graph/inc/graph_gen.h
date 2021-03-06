#ifndef GRAPH_GEN_H_
#define GRAPH_GEN_H_

#include "gm_graph.h"

void create_uniform_random_graph_new(gm_graph &G, node_t N, edge_t M, long seed, bool use_xorshift_rng);
gm_graph* create_uniform_random_graph(node_t N, edge_t M, long seed, bool use_xorshift_rng);
gm_graph* create_uniform_random_graph2(node_t N, edge_t M, long seed);
gm_graph* create_uniform_random_nonmulti_graph(node_t N, edge_t M, long seed);
gm_graph* create_RMAT_graph(node_t N, edge_t M, long rseed, double a, double b, double c, bool permute);

#endif /* GRAPH_GEN_H_ */
