#include "hop_dist_ec.h"
#include "shl.h"
#include "shl_array.hpp"
#include "shl_array_expandable.hpp"
#include "omp.h"

// Configure array here

#define SHL_EC_THREAD_INIT(base)                                        \
    key_buff_ptr_thread_init(shl__G_dist__set, &dist_thread_ptr)

struct arr_thread_ptr {
    int32_t *rep_ptr;
    int32_t *ptr1;
    int32_t *ptr2;
    struct array_cache c;
};

struct arr_thread_ptr dist_thread_ptr;
#pragma omp threadprivate(dist_thread_ptr)

void key_buff_ptr_thread_init(shl_array<int32_t> *base,
                              struct arr_thread_ptr *p)
{
    shl_array_replicated<int32_t> *btc =
        (shl_array_replicated<int32_t>*) base;

    p->rep_ptr = base->get_array();
    p->ptr1 = btc->rep_array[0];
    p->ptr2 = btc->rep_array[1];

    p->c = (struct array_cache) {
        .rid = shl__get_rep_id(),
        .tid = shl__get_tid()
    };
}


void hop_dist(gm_graph& G, int32_t* G_dist,
    node_t& root)
{
    //Initializations
    gm_rt_initialize();
    G.freeze();
    #ifdef SHL_STATIC
    shl__init(gm_rt_get_num_threads(), 1);
    #else
    shl__init(gm_rt_get_num_threads(), 0);
    #endif
    COST;
    shl__start_timer();
    shl_array<edge_t>* shl__G_begin__set = shl__malloc<edge_t>((G.num_nodes()+1), "G.begin", shl__G_begin_IS_RO, shl__G_begin_IS_DYNAMIC, shl__G_begin_IS_USED, shl__G_begin_IS_GRAPH, shl__G_begin_IS_INDEXED, true /*do init*/);
    shl__G_begin__set->alloc();
    shl__G_begin__set->copy_from(G.begin);
    //    shl_array<int32_t>* shl__G_dist__set = shl__malloc<int32_t>(G.num_nodes(), "G_dist", shl__G_dist_IS_RO, shl__G_dist_IS_DYNAMIC, shl__G_dist_IS_USED, shl__G_dist_IS_GRAPH, shl__G_dist_IS_INDEXED, true /*do init*/);
    shl_array<int32_t> *shl__G_dist__set;
    shl__G_dist__set = new shl_array_expandable<int32_t>(G.num_nodes(), "G_dist", shl__get_rep_id);
    shl__G_dist__set->set_dynamic(false);
    shl__G_dist__set->set_used(true);

    shl__G_dist__set->alloc();
    shl__G_dist__set->copy_from(G_dist);
    shl_array<node_t>* shl__G_node_idx__set = shl__malloc<node_t>((G.num_edges()+1), "G.node_idx", shl__G_node_idx_IS_RO, shl__G_node_idx_IS_DYNAMIC, shl__G_node_idx_IS_USED, shl__G_node_idx_IS_GRAPH, shl__G_node_idx_IS_INDEXED, true /*do init*/);
    shl__G_node_idx__set->alloc();
    shl__G_node_idx__set->copy_from(G.node_idx);
    shl_array<edge_t>* shl__G_r_begin__set = shl__malloc<edge_t>((G.num_nodes()+1), "G.r_begin", shl__G_r_begin_IS_RO, shl__G_r_begin_IS_DYNAMIC, shl__G_r_begin_IS_USED, shl__G_r_begin_IS_GRAPH, shl__G_r_begin_IS_INDEXED, true /*do init*/);
    shl__G_r_begin__set->alloc();
    shl__G_r_begin__set->copy_from(G.r_begin);
    shl_array<node_t>* shl__G_r_node_idx__set = shl__malloc<node_t>((G.num_edges()+1), "G.r_node_idx", shl__G_r_node_idx_IS_RO, shl__G_r_node_idx_IS_DYNAMIC, shl__G_r_node_idx_IS_USED, shl__G_r_node_idx_IS_GRAPH, shl__G_r_node_idx_IS_INDEXED, true /*do init*/);
    shl__G_r_node_idx__set->alloc();
    shl__G_r_node_idx__set->copy_from(G.r_node_idx);
    shl__end_timer();

    struct shl_frame f = FRAME_DEFAULT;

    bool* G_updated = gm_rt_allocate_bool(G.num_nodes(),gm_rt_thread_id());
    COST;
    shl__start_timer();
    shl_array<bool>* shl__G_updated__set = shl__malloc<bool>(G.num_nodes(), "G_updated", shl__G_updated_IS_RO, shl__G_updated_IS_DYNAMIC, shl__G_updated_IS_USED, shl__G_updated_IS_GRAPH, shl__G_updated_IS_INDEXED, true /*do init*/);
    shl__G_updated__set->alloc();
    shl__G_updated__set->copy_from(G_updated);
    shl__end_timer();

    bool* G_updated_nxt = gm_rt_allocate_bool(G.num_nodes(),gm_rt_thread_id());
    COST;
    shl__start_timer();
    shl_array<bool>* shl__G_updated_nxt__set = shl__malloc<bool>(G.num_nodes(), "G_updated_nxt", shl__G_updated_nxt_IS_RO, shl__G_updated_nxt_IS_DYNAMIC, shl__G_updated_nxt_IS_USED, shl__G_updated_nxt_IS_GRAPH, shl__G_updated_nxt_IS_INDEXED, true /*do init*/);
    shl__G_updated_nxt__set->alloc();
    shl__G_updated_nxt__set->copy_from(G_updated_nxt);
    shl__end_timer();

    int32_t* G_dist_nxt = gm_rt_allocate_int(G.num_nodes(),gm_rt_thread_id());
    COST;
    shl__start_timer();
    shl_array<int32_t>* shl__G_dist_nxt__set = shl__malloc<int32_t>(G.num_nodes(), "G_dist_nxt", shl__G_dist_nxt_IS_RO, shl__G_dist_nxt_IS_DYNAMIC, shl__G_dist_nxt_IS_USED, shl__G_dist_nxt_IS_GRAPH, shl__G_dist_nxt_IS_INDEXED, true /*do init*/);
    shl__G_dist_nxt__set->alloc();
    shl__G_dist_nxt__set->copy_from(G_dist_nxt);
    shl__end_timer();

    f.fin = false ;

    shl__G_dist__set->expand();


    #pragma omp parallel
    {
        SHL_EC_THREAD_INIT();
        edge_t* shl__G_begin __attribute__ ((unused)) = shl__G_begin__set->get_array();
        int32_t* shl__G_dist __attribute__ ((unused)) = shl__G_dist__set->get_array();
        int32_t* shl__G_dist_nxt __attribute__ ((unused)) = shl__G_dist_nxt__set->get_array();
        node_t* shl__G_node_idx __attribute__ ((unused)) = shl__G_node_idx__set->get_array();
        edge_t* shl__G_r_begin __attribute__ ((unused)) = shl__G_r_begin__set->get_array();
        node_t* shl__G_r_node_idx __attribute__ ((unused)) = shl__G_r_node_idx__set->get_array();
        bool* shl__G_updated __attribute__ ((unused)) = shl__G_updated__set->get_array();
        bool* shl__G_updated_nxt __attribute__ ((unused)) = shl__G_updated_nxt__set->get_array();
        shl_graph shl_G(shl__G_begin, shl__G_r_begin, shl__G_node_idx, shl__G_r_node_idx);

        #ifdef SHL_STATIC
        #pragma omp for schedule(static,1024)
        #else
        #pragma omp for
        #endif
        for (node_t t0 = 0; t0 < G.num_nodes(); t0 ++)
        {
            shl__G_dist__wr(t0, (t0 == root)?0:INT_MAX) ;
            shl__G_updated__wr(t0, (t0 == root)?true:false) ;
            shl__G_dist_nxt__wr(t0, shl__G_dist__rd(t0)) ;
            shl__G_updated_nxt__wr(t0, shl__G_updated__rd(t0)) ;
        }

    } // opened in prepare_parallel_for

    while ( !f.fin)
    { /*=>k*/
        {

            f.fin = true ;
            f.__E8 = false ;

            #pragma omp parallel
            {
                SHL_EC_THREAD_INIT();
                edge_t* shl__G_begin __attribute__ ((unused)) = shl__G_begin__set->get_array();
                int32_t* shl__G_dist __attribute__ ((unused)) = shl__G_dist__set->get_array();
                int32_t* shl__G_dist_nxt __attribute__ ((unused)) = shl__G_dist_nxt__set->get_array();
                node_t* shl__G_node_idx __attribute__ ((unused)) = shl__G_node_idx__set->get_array();
                edge_t* shl__G_r_begin __attribute__ ((unused)) = shl__G_r_begin__set->get_array();
                node_t* shl__G_r_node_idx __attribute__ ((unused)) = shl__G_r_node_idx__set->get_array();
                bool* shl__G_updated __attribute__ ((unused)) = shl__G_updated__set->get_array();
                bool* shl__G_updated_nxt __attribute__ ((unused)) = shl__G_updated_nxt__set->get_array();
                shl_graph shl_G(shl__G_begin, shl__G_r_begin, shl__G_node_idx, shl__G_r_node_idx);
                #ifdef SHL_STATIC
                #pragma omp for schedule(static,1024)
                #else
                #pragma omp for schedule(dynamic,128)
                #endif
                for (node_t n = 0; n < G.num_nodes(); n ++)
                {
                    if (shl__G_updated__rd(n))
                    {
                        for (edge_t s_idx = shl__begin__rd(n);s_idx < shl__begin__rd(n+1); s_idx ++) {node_t s = shl__node_idx__rd(s_idx);
                            { // argmin(argmax) - test and test-and-set
                                int32_t G_dist_nxt_new = shl__G_dist__rd(n) + 1;
                                if (shl__G_dist_nxt__rd(s)>G_dist_nxt_new) {
                                    bool G_updated_nxt_arg = true;
                                    gm_spinlock_acquire_for_node(s);
                                    if (shl__G_dist_nxt__rd(s)>G_dist_nxt_new) {
                                        shl__G_dist_nxt__wr(s, G_dist_nxt_new);
                                        shl__G_updated_nxt__wr(s, G_updated_nxt_arg);
                                    }
                                    gm_spinlock_release_for_node(s);
                                }
                            }
                        }}
                    }
                } // opened in prepare_parallel_for

                #pragma omp parallel
                {
                    SHL_EC_THREAD_INIT();

                    edge_t* shl__G_begin __attribute__ ((unused)) = shl__G_begin__set->get_array();
                    int32_t* shl__G_dist __attribute__ ((unused)) = shl__G_dist__set->get_array();
                    int32_t* shl__G_dist_nxt __attribute__ ((unused)) = shl__G_dist_nxt__set->get_array();
                    node_t* shl__G_node_idx __attribute__ ((unused)) = shl__G_node_idx__set->get_array();
                    edge_t* shl__G_r_begin __attribute__ ((unused)) = shl__G_r_begin__set->get_array();
                    node_t* shl__G_r_node_idx __attribute__ ((unused)) = shl__G_r_node_idx__set->get_array();
                    bool* shl__G_updated __attribute__ ((unused)) = shl__G_updated__set->get_array();
                    bool* shl__G_updated_nxt __attribute__ ((unused)) = shl__G_updated_nxt__set->get_array();
                    shl_graph shl_G(shl__G_begin, shl__G_r_begin, shl__G_node_idx, shl__G_r_node_idx);

                    struct shl_per_thread_frame ft = FRAME_THREAD_DEFAULT;
                    ft.__E8_prv = false ;

                    #ifdef SHL_STATIC
                    #pragma omp for nowait schedule(static,1024)
                    #else
                    #pragma omp for nowait
                    #endif
                    for (node_t t4 = 0; t4 < G.num_nodes(); t4 ++)
                    {
                        shl__G_dist__wr(t4, shl__G_dist_nxt__rd(t4)) ;
                        shl__G_updated__wr(t4, shl__G_updated_nxt__rd(t4)) ;
                        shl__G_updated_nxt__wr(t4, false) ;
                        ft.__E8_prv = ft.__E8_prv || shl__G_updated__rd(t4) ;
                    }
                    ATOMIC_OR(&f.__E8, ft.__E8_prv);

                }
                f.fin =  !f.__E8 ;
            }
        } /*=>/k*/

    SHL_EC_THREAD_INIT();

        shl__start_timer();
        shl__G_begin__set->copy_back(G.begin);
        shl__G_dist__set->copy_back(G_dist);
        shl__G_dist_nxt__set->copy_back(G_dist_nxt);
        shl__G_node_idx__set->copy_back(G.node_idx);
        shl__G_r_begin__set->copy_back(G.r_begin);
        shl__G_r_node_idx__set->copy_back(G.r_node_idx);
        shl__G_updated__set->copy_back(G_updated);
        shl__G_updated_nxt__set->copy_back(G_updated_nxt);
        shl__end_timer();

        shl__end();
        for (int t=0; t<shl__num_threads(); t++) {

            if (shl__is_rep_coordinator(t) || true) {

                printf("Expand on tid=%d is %lf\n", t,
                       shl__G_dist__set->t_expand[t].timer_secs);
            }
        }

        gm_rt_cleanup();
    }

