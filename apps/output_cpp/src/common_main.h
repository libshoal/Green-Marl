#ifndef COMMON_MAIN_H
#define COMMON_MAIN_H


#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "gm.h"

#include "shl.h"
#ifdef BARRELFISH
#include "gm_barrelfish.h"
extern "C" {
#include <omp.h>
#include <vfs/vfs.h>
#include <vfs/vfs_path.h>
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

        // check if node/edge size matches with the library (runtime-check)
        gm_graph_check_node_edge_size_at_link_time();

        if (argc < 4) {

            printf("%s <graph_name> <num_threads> <nfspath>", argv[0]);
            print_arg_info();
            printf("\n");

            exit (EXIT_FAILURE);
        }

        int new_argc = argc - 4;
        char** new_argv = &(argv[4]);
        b = check_args(new_argc, new_argv);
        if (!b) {
            printf("error procesing argument\n");
            printf("%s <graph_name> <num_threads> ", argv[0]);
            print_arg_info();
            printf("\n");
            exit (EXIT_FAILURE);
        }

        int num = atoi(argv[2]);

#ifdef SHL_STATIC
        shl__init(num, 1);
#else
        shl__init(num, 0);
#endif

#ifdef BARRELFISH

        argvals = argv;
        argcount = argc;

        errval_t err;

        xomp_wid_t wid;

        err = xomp_worker_parse_cmdline(argc, argv, &wid);
        switch (err_no(err)) {
            case SYS_ERR_OK: {
                struct xomp_args arg;
                arg.type = XOMP_ARG_TYPE_WORKER;
                arg.args.worker.id = wid;
                if (bomp_xomp_init(&arg)) {
                    printf("XOMP worker initialization failed.\n");
                    exit (EXIT_FAILURE);
                }
                printf("This point should not be reached...\n");
                exit (EXIT_FAILURE);

            } break;
            case XOMP_ERR_BAD_INVOCATION: {
                printf("Barrelfish specific prepare:\n");
                printf("  vfs initialization\n");
                vfs_init();
                printf("  vfs mkdir /nfs\n");
                err = vfs_mkdir("/nfs");
                if (err_is_fail(err)) {
                    printf("ERROR: failed to create path, %s\n",err_getstring(err));
                    exit (EXIT_FAILURE);
                }
                if (argv[3][0] != '0') {
                    printf("  vfs mount /nfs %s\n", argv[3]);
                    err = vfs_mount("/nfs", argv[3]);
                    if (err_is_fail(err)) {
                        printf("ERROR: failed to mount path, %s\n",err_getstring(err));
                        exit(EXIT_FAILURE);
                    }
                }

                printf("  initialize library for barrelfish\n");
                gm_rt_initialize_barrelfish(num);
            } break;
            default:
                printf("Unexpected failure during argument parsing\n");
                exit (EXIT_FAILURE);
        }
#endif
        printf("running with %d threads\n", num);
        gm_rt_set_num_threads(num); // gm_runtime.h

        //--------------------------------------------
        // Load graph and creating reverse edges
        //--------------------------------------------
        struct timeval T1, T2;
        gettimeofday(&T1, NULL);
        printf("loading graph...%s\n", argv[1]);
        b = G.load_binary(argv[1]);
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

        /*
         * XXX This line signals the termination of the execution for scalebench
         */
        printf("XXXXXXXXXX GM DONE XXXXXXXXXXXXXX\n");

#ifdef BARRELFISH
        // XXX avoid page faults
       while(1)
           ;
#endif

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
