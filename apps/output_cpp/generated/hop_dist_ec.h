#ifndef GM_GENERATED_CPP_HOP_DIST_H
#define GM_GENERATED_CPP_HOP_DIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <limits.h>
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <shl.h>
#include <shl_graph.h>
#include "gm.h"

void hop_dist(gm_graph& G, int32_t* G_dist,
    node_t& root);
#define shl__G_begin_IS_USED 1
#define shl__G_begin_IS_RO 1
#define shl__G_begin_IS_GRAPH 1
#define shl__G_begin_IS_DYNAMIC 0
#define shl__G_begin_IS_INDEXED 0
#define shl__G_dist_IS_USED 1
#define shl__G_dist_IS_RO 0
#define shl__G_dist_IS_GRAPH 0
#define shl__G_dist_IS_DYNAMIC 0
#define shl__G_dist_IS_INDEXED 1
#define shl__G_dist_nxt_IS_USED 1
#define shl__G_dist_nxt_IS_RO 0
#define shl__G_dist_nxt_IS_GRAPH 0
#define shl__G_dist_nxt_IS_DYNAMIC 1
#define shl__G_dist_nxt_IS_INDEXED 0
#define shl__G_node_idx_IS_USED 1
#define shl__G_node_idx_IS_RO 1
#define shl__G_node_idx_IS_GRAPH 1
#define shl__G_node_idx_IS_DYNAMIC 0
#define shl__G_node_idx_IS_INDEXED 0
#define shl__G_r_begin_IS_USED 0
#define shl__G_r_begin_IS_RO 1
#define shl__G_r_begin_IS_GRAPH 1
#define shl__G_r_begin_IS_DYNAMIC 0
#define shl__G_r_begin_IS_INDEXED 1
#define shl__G_r_node_idx_IS_USED 0
#define shl__G_r_node_idx_IS_RO 1
#define shl__G_r_node_idx_IS_GRAPH 1
#define shl__G_r_node_idx_IS_DYNAMIC 0
#define shl__G_r_node_idx_IS_INDEXED 1
#define shl__G_updated_IS_USED 1
#define shl__G_updated_IS_RO 0
#define shl__G_updated_IS_GRAPH 0
#define shl__G_updated_IS_DYNAMIC 1
#define shl__G_updated_IS_INDEXED 1
#define shl__G_updated_nxt_IS_USED 1
#define shl__G_updated_nxt_IS_RO 0
#define shl__G_updated_nxt_IS_GRAPH 0
#define shl__G_updated_nxt_IS_DYNAMIC 1
#define shl__G_updated_nxt_IS_INDEXED 0

/* w/ SHOAL extensions */
#define COST shl__estimate_working_set_size(8, \
    shl__estimate_size<edge_t>((G.num_nodes()+1), "G.begin", shl__G_begin_IS_RO, shl__G_begin_IS_DYNAMIC, shl__G_begin_IS_USED, shl__G_begin_IS_GRAPH, shl__G_begin_IS_INDEXED),\
    shl__estimate_size<int32_t>(G.num_nodes(), "G_dist", shl__G_dist_IS_RO, shl__G_dist_IS_DYNAMIC, shl__G_dist_IS_USED, shl__G_dist_IS_GRAPH, shl__G_dist_IS_INDEXED),\
    shl__estimate_size<int32_t>(G.num_nodes(), "G_dist_nxt", shl__G_dist_nxt_IS_RO, shl__G_dist_nxt_IS_DYNAMIC, shl__G_dist_nxt_IS_USED, shl__G_dist_nxt_IS_GRAPH, shl__G_dist_nxt_IS_INDEXED),\
    shl__estimate_size<node_t>((G.num_edges()+1), "G.node_idx", shl__G_node_idx_IS_RO, shl__G_node_idx_IS_DYNAMIC, shl__G_node_idx_IS_USED, shl__G_node_idx_IS_GRAPH, shl__G_node_idx_IS_INDEXED),\
    shl__estimate_size<edge_t>((G.num_nodes()+1), "G.r_begin", shl__G_r_begin_IS_RO, shl__G_r_begin_IS_DYNAMIC, shl__G_r_begin_IS_USED, shl__G_r_begin_IS_GRAPH, shl__G_r_begin_IS_INDEXED),\
    shl__estimate_size<node_t>((G.num_edges()+1), "G.r_node_idx", shl__G_r_node_idx_IS_RO, shl__G_r_node_idx_IS_DYNAMIC, shl__G_r_node_idx_IS_USED, shl__G_r_node_idx_IS_GRAPH, shl__G_r_node_idx_IS_INDEXED),\
    shl__estimate_size<bool>(G.num_nodes(), "G_updated", shl__G_updated_IS_RO, shl__G_updated_IS_DYNAMIC, shl__G_updated_IS_USED, shl__G_updated_IS_GRAPH, shl__G_updated_IS_INDEXED),\
    shl__estimate_size<bool>(G.num_nodes(), "G_updated_nxt", shl__G_updated_nxt_IS_RO, shl__G_updated_nxt_IS_DYNAMIC, shl__G_updated_nxt_IS_USED, shl__G_updated_nxt_IS_GRAPH, shl__G_updated_nxt_IS_INDEXED))
#define shl__G_dist__wr(i, v) shl__G_dist__set->set(i, v)
//#define shl__G_dist__rd(i) shl__G_dist__set->get(i)
#define shl__G_dist__rd(i) dist_thread_ptr.rep_ptr[i]
#define shl__G_dist_nxt__wr(i, v) shl__G_dist_nxt[i] = v
#define shl__G_dist_nxt__rd(i) shl__G_dist_nxt[i]
#define shl__G_updated__wr(i, v) shl__G_updated[i] = v
#define shl__G_updated__rd(i) shl__G_updated[i]
#define shl__G_updated_nxt__wr(i, v) shl__G_updated_nxt[i] = v
#define shl__G_updated_nxt__rd(i) shl__G_updated_nxt[i]
#define shl__begin__rd(i) shl__G_begin[i]
#define shl__node_idx__rd(i) shl__G_node_idx[i]

struct shl_frame {
    bool __E8;
    bool fin;
};

#define FRAME_DEFAULT {false, false}

struct shl_per_thread_frame {
    bool __E8_prv;
};

#define FRAME_THREAD_DEFAULT {false}

#endif
