#include "pagerank.h"

void pagerank(gm_graph& G, double e, 
    double d, int32_t max, 
    double*G_pg_rank)
{
    //Initializations
    gm_rt_initialize();
    G.freeze();
    G.make_reverse_edges();

    double diff = 0.0 ;
    int32_t cnt = 0 ;
    double N = 0.0 ;
    double* G_pg_rank_nxt = gm_rt_allocate_double(G.num_nodes(),gm_rt_thread_id());

    cnt = 0 ;
    N = (double)(G.num_nodes()) ;

    #pragma omp parallel for
    for (node_t t0 = 0; t0 < G.num_nodes(); t0 ++) 
        G_pg_rank[t0] = 1 / N ;

    do
    {
        diff = ((float)(0.000000)) ;
        #pragma omp parallel
        {
            double diff_prv = 0.0 ;

            diff_prv = ((float)(0.000000)) ;

            #pragma omp for nowait schedule(dynamic,128)
            for (node_t t = 0; t < G.num_nodes(); t ++) 
            {
                double val = 0.0 ;
                double __S1 = 0.0 ;

                __S1 = ((float)(0.000000)) ;
                for (edge_t w_idx = G.r_begin[t];w_idx < G.r_begin[t+1] ; w_idx ++) 
                {
                    node_t w = G.r_node_idx[w_idx];
                    __S1 = __S1 + G_pg_rank[w] / ((double)((G.begin[w+1] - G.begin[w]))) ;
                }
                val = (1 - d) / N + d * __S1 ;
                diff_prv = diff_prv +  std::abs((val - G_pg_rank[t]))  ;
                G_pg_rank_nxt[t] = val ;
            }
            ATOMIC_ADD<double>(&diff, diff_prv);
        }

        #pragma omp parallel for
        for (node_t i3 = 0; i3 < G.num_nodes(); i3 ++) 
            G_pg_rank[i3] = G_pg_rank_nxt[i3] ;

        cnt = cnt + 1 ;
    }
    while ((diff > e) && (cnt < max));


    gm_rt_cleanup();
}

