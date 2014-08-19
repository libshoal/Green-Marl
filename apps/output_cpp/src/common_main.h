#ifndef COMMON_MAIN_H
#define COMMON_MAIN_H


#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "gm.h"

#ifdef BARRELFISH
extern "C" {
#include <omp.h>
#include <vfs/vfs.h>
#include <xomp/xomp.h>
#include <barrelfish/waitset.h>
extern void messages_wait_and_handle_next(void);
char **argvals;
int argcount;

}
#else
#include <omp.h>
#endif

class main_t
{
protected:
#ifdef __HDFS__
    gm_graph_hdfs G;
    gm_graph_hdfs& get_graph() {
        return G;
    }
#else
    gm_graph G;
    gm_graph& get_graph() {
        return G;
    }
#endif

    double time_to_exclude;
    void add_time_to_exlude(double ms) {
        time_to_exclude += ms;
    }

public:
    virtual ~main_t() {
    }

    main_t() {
        time_to_exclude = 0;
    }

    virtual void main(int argc, char** argv) {
        bool b;

        printf("common_main:main:\n");

        // check if node/edge size matches with the library (runtime-check)
        gm_graph_check_node_edge_size_at_link_time();

        if (argc < 3) {

            printf("%s <graph_name> <num_threads> ", argv[0]);
            print_arg_info();
            printf("\n");

            exit (EXIT_FAILURE);
        }

        int new_argc = argc - 3;
        char** new_argv = &(argv[3]);
        b = check_args(new_argc, new_argv);
        if (!b) {
            printf("error procesing argument\n");
            printf("%s <graph_name> <num_threads> ", argv[0]);
            print_arg_info();
            printf("\n");
            exit (EXIT_FAILURE);
        }

        int num = atoi(argv[2]);
        printf("running with %d threads\n", num);

#ifdef BARRELFISH

        argvals = argv;
        argcount = argc;

        printf("Barrelfish specific prepare:\n");
        printf("  vfs initialization\n");
        vfs_init();

        errval_t err;

        xomp_wid_t wid;
        printf("  parsing cmd line for Xomp Worker args\n");
        err = xomp_worker_parse_cmdline(argc, argv, &wid);
        switch (err_no(err)) {
            case SYS_ERR_OK:
                printf("initializing XOMP worker...");
                struct xomp_args arg;
                arg.type = XOMP_ARG_TYPE_WORKER;
                arg.args.worker.id = wid;
                if (bomp_xomp_init(&arg)) {
                    printf("XOMP worker initialization failed.\n");
                    exit (EXIT_FAILURE);
                }
                printf("This point should not be reached...\n");
                exit (EXIT_FAILURE);

                break;
            case XOMP_ERR_BAD_INVOCATION:
                gm_rt_initialize_barrelfish(num);
                break;
            default:
                printf("Unexpected failure during argument parsing\n");
                exit (EXIT_FAILURE);
                break;
        }
#endif

        gm_rt_set_num_threads(num); // gm_runtime.h

        //--------------------------------------------
        // Load graph anc creating reverse edges
        //--------------------------------------------
        struct timeval T1, T2;
        char *fname = argv[1];
        gettimeofday(&T1, NULL);
        printf("loading graph...%s\n", fname);
        b = G.load_binary(fname);
        if (!b) {
            printf("error reading graph\n");
            exit (EXIT_FAILURE);
        }
        gettimeofday(&T2, NULL);
        printf("graph loading time=%lf\n", (T2.tv_sec - T1.tv_sec) * 1000 + (T2.tv_usec - T1.tv_usec) * 0.001);

        gettimeofday(&T1, NULL);
        G.make_reverse_edges();
        gettimeofday(&T2, NULL);
        printf("reverse edge creation time=%lf\n", (T2.tv_sec - T1.tv_sec) * 1000 + (T2.tv_usec - T1.tv_usec) * 0.001);

        //gettimeofday(&T1, NULL);
        //G.do_semi_sort();
        //gettimeofday(&T2, NULL);
        //printf("semi sorting time=%lf\n", (T2.tv_sec - T1.tv_sec) * 1000 + (T2.tv_usec - T1.tv_usec) * 0.001);

        //------------------------------------------------
        // Any extra preperation Step (provided by the user)
        //------------------------------------------------
        b = prepare();
        if (!b) {
            printf("Error prepare data\n");
            exit (EXIT_FAILURE);
        }

        gettimeofday(&T1, NULL);
        b = run();
        gettimeofday(&T2, NULL);
        printf("running time=%lf\n", (T2.tv_sec - T1.tv_sec) * 1000 + (T2.tv_usec - T1.tv_usec) * 0.001 - time_to_exclude);
        if (!b) {
            printf("Error runing algortihm\n");
            exit (EXIT_FAILURE);
        }

        b = post_process();
        if (!b) {
            printf("Error post processing\n");
            exit (EXIT_FAILURE);
        }

        //----------------------------------------------
        // Clean up routine
        //----------------------------------------------
        b = cleanup();
        if (!b) exit (EXIT_FAILURE);
    }

    virtual bool check_answer() {
        return true;
    }
    virtual bool run() = 0;
    virtual bool prepare() {
        return true;
    }
    virtual bool post_process() {
        return true;
    }
    virtual bool cleanup() {
        return true;
    }
    // check remaining arguments
    virtual bool check_args(int argc, char** argv) {
        return true;
    }
    virtual void print_arg_info() {
    }
};

#endif
