#ifndef SHL_GRAPH_H_
#define SHL_GRAPH_H_

#include "gm_internal.h"

typedef node_t node_id;
typedef edge_t edge_id;

// This is a stripped down graph representation to be used as a per-thread
// data structure
//
// This should also be relevant for the Oracle code-base
//
// Heavily based on gm_graph.h

//------------------------------------------------------------------------------------------
// Representation of Graph
//
// (1) Fronzen Form: CSR implementation (which is a compacted adjacnency list)
//       - Nodes are identified by node-idx (0 ~ N-1)
//       - Edges are idenfified by edge-idx (0 ~ M-1)
//       - Basic form consists of two arrays
//            edge_t begin    O(E)  : beginning the neighbor-list of each node-idx
//            node_t node_idx O(N)  : destination node-idx of each edge-idx.
//       - For instance, following code iterates all the (out) neighbors of node k;
//            edge_t begin = G.begin(k);
//            edge_t end = G.begin(k+1);
//            for(edge_t t = begin; t < end; t++) {
//               node_t n = G.node_idx[t];
//               ......
//            }
//
// (2) Properties are (assumed to be) stored in array.
//       - Node properties are stored in node-idx order.
//       - Edge properties are stored in edge-idx order.
//       - These indices are for when the graph is initially frozen (loaded).
//
//
// (3) Additional indicies are created by request.
//      - make_reverse_edges();  ==> create reverse edges
//      - do_semi_sort();        ==> sort edges from the same source by the order of destination nodes
//      - prepare_edge_source(); ==> create O(E) array of source node-idx (as opposed to destination node-idx)
//
//      * make_reverse_edge:
//           - create following data structures:
//              edge_t r_begin    O(E) :  beginning of in-neighbor list of each node-idx
//              node_t r_node_idx O(N) :  destination of each reverse edge, i.e. source of original edge
//              edge_t e_rev2idx  O(E) :  a mapping of reverse edge-idx ==> original edge-idx
//
//           - For instance, following code iterates all the incoming neighbors of node k;
//               edge_t begin = G.r_begin(k);
//               edge_t end = G.r_begin(k+1);
//               for(edge_t t = begin; t < end; t++) {
//                   node_t n = G.r_node_idx[t];
//                   ......
//                }
//           - For instance, following code look at all the edge values of incoming edges
//                edge_t begin = G.r_begin(k);
//                edge_t end = G.r_begin(k+1);
//                for(edge_t t = begin; t < end; t++) {
//                   value_t V = EdgePropA[e_rev2idx[t]];
//                   ......
//                }
//
//      * do_semi_sort:  // [XXX: to be changed as CSR representation MUST always be sorted]
//          - node_idx array is semi-sorted. If reverse-edge has been created, r_node_idx array is also semi-sorted.
//          - create following data structure:
//              edge_t e_idx2idx  O(M) : a mappping of sorted edge-idx ==> original edge-idx
//
//          - Henthforth, once semi-sorting has been applied, edge properties have to be indirected as in following example.
//                edge_t begin = G.r_begin(k);
//                edge_t end = G.r_begin(k+1);
//                for(edge_t t = begin; t < end; t++) {
//                   value_t V = EdgePropA[e_idx2idx[t]];
//                   ......
//                }
//          - e_rev2idx is automatically updated, as r_node_idx array is sorted.
//
//
//      * prepare_edge_source:
//         - create following data structure
//              node_t* node_idx_src     O(M) // source of each edge-idx
//              node_t* r_node_idx_src;  O(M) // source of each reverse edge-idx (i.e. org destination)
//
//
//
//
//------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------
// Representation of Graph
//
// 1. IDX vs ID
//
// node_t(edge_t) is used to represent both node(edge) ID and node(edge) IDX.
// ID is used to indicate a speicific node(edge) in flexible format
// IDX is to represent the position of node(edge) in the compact form.
//
// As for node, node ID is same to node IDX.
// As for edge, edge ID can be different from edge IDX.
// When freezing the graph, the system creates a mapping between ID -> IDX and IDX -> ID.
//
// 2. The graph is represented as two format.
//
//   Flexible Format
//     Map<node_ID, vector<Node_ID, Edge_ID> > ; neighborhood list
//
//--------------------------------------------------------------------------

class shl_graph
{
  public:
    shl_graph(edge_t *b, edge_t *r_b, node_t *n, node_t *r_n);
    ~shl_graph();

    edge_t* begin;             // O(N) array of edge_t
    node_t* node_idx;          // O(M) array of node_t (destination of each edge)

    edge_t* r_begin;           // O(N) array of edge_t
    node_t* r_node_idx;        // O(M) array of node_t (destination of each reverse edge, i.e. source of original edge)

    static const node_t NIL_NODE = (node_t) -1;
    static const edge_t NIL_EDGE = (edge_t) -1;

    bool is_neighbor(node_t src, node_t to); // need semi sorting
    edge_t get_edge_idx_for_src_dest(node_t src, node_t dest);
};

#endif
