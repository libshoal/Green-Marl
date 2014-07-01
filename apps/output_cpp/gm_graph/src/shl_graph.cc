#include "shl_graph.h"

shl_graph::shl_graph() {
    begin = NULL;
    node_idx = NULL;

    r_begin = NULL;
    r_node_idx = NULL;
}

shl_graph::~shl_graph() {
}

bool shl_graph::is_neighbor(node_t src, node_t dest) {
    return get_edge_idx_for_src_dest(src, dest) != NIL_EDGE;
}

edge_t shl_graph::get_edge_idx_for_src_dest(node_t src, node_t to)
{

    // assumption: Edges are semi-sorted.

    // Do binary search
    edge_t begin_edge = begin[src];
    edge_t end_edge = begin[src + 1] - 1; // inclusive
    if (begin_edge > end_edge) return NIL_EDGE;

    node_t left_node = node_idx[begin_edge];
    node_t right_node = node_idx[end_edge];
    if (to == left_node) return begin_edge;
    if (to == right_node) return end_edge;

    /*int cnt = 0;*/
    while (begin_edge < end_edge) {
        left_node = node_idx[begin_edge];
        right_node = node_idx[end_edge];

        /*
         cnt++;
         if (cnt > 490) {
         printf("%d ~ %d (val:%d ~ %d) vs %d\n", begin_edge, end_edge, left_node, right_node, to);
         }
         if (cnt == 500) assert(false);
         */

        if (to < left_node) return NIL_EDGE;
        if (to > right_node) return NIL_EDGE;

        edge_t mid_edge = (begin_edge + end_edge) / 2;
        node_t mid_node = node_idx[mid_edge];
        if (to == mid_node) return mid_edge;
        if (to < mid_node) {
            if (end_edge == mid_edge) return NIL_EDGE;
            end_edge = mid_edge;
        } else if (to > mid_node) {
            if (begin_edge == mid_edge) return NIL_EDGE;
            begin_edge = mid_edge;
        }

    }
    return NIL_EDGE;
}
