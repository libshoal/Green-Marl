#ifndef GM_RUNTIME_H_
#define GM_RUNTIME_H_
#include <assert.h>
extern "C" {
#include <omp.h>
}
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "gm_mem_helper.h"

//----------------------------------------------------
// A thin layer over openmp
//----------------------------------------------------

class gm_runtime
{
  public:
    gm_runtime();
    virtual ~gm_runtime();

    void initialize();
    void initialize_barrelfish(unsigned nthreads);
    bool is_initialized();
    int get_num_threads();
    void set_num_threads(int n);
    int get_thread_id();

    double uniform(int tid = 0);                     // returns [0,1)
    long rand_long(long max, int tid = 0);           // returns [0, max-1]   : for large numbers e.g. nodes
    int rand(int min, int max, int tid = 0);   // returns [min, max-1]

  private:
    void expand_random_seeds(int old, int n);
    bool is_init;
    int num_threads;
    unsigned short* random_seeds;

};

extern gm_runtime _GM_RT;
extern int dummy[];

/*
 * Wrappers to call the gm_runtime methods
 *    - Shouldnt they be static members of the gm_runtime class?
 */
void gm_rt_initialize();
void gm_rt_initialize_barrelfish(unsigned nthreads);
bool gm_rt_is_initialized();
int gm_rt_get_num_threads();
void gm_rt_set_num_threads(int n);
int gm_rt_thread_id();
double gm_rt_uniform(int tid = 0);
int gm_rt_rand(int min, int max, int tid = 0);
long gm_rt_rand_long(long max, int tid = 0);

#endif
