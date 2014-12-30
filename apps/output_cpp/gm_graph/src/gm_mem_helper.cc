#include <assert.h>
#include "gm_mem_helper.h"
#ifdef BARRELFISH
#include "gm_barrelfish.h"
#endif

void gm_mem_helper::resize(int n) {
    if ((max > 0) && (n <= max)) return;
    primitivePointers.resize(n);
    complexPointers.resize(n);
    max = n;
}

void gm_mem_helper::save(void* ptr, int typeinfo, int thread_id) {
    primitivePointers[thread_id].push_back(ptr);
}

void gm_mem_helper::save(gm_complex_data_type* ptr, int typeinfo, int thread_id) {
    complexPointers[thread_id].push_back(ptr);
}

void gm_mem_helper::clear(void* ptr, int typeinfo, int thread_id) {
    std::list<void*>& L = primitivePointers[thread_id];
    std::list<void*>::iterator i;
    for (i = L.begin(); i != L.end(); i++) {
        if (*i == ptr) {
            // primitive aray.
            // This will work for primitive types.
#ifdef BARRELFISH
            /* TODO: Free memory */
#else
            float* P = (float*) *i;
            delete[] P;
#endif
            L.erase(i);
            return;
        }
    }

    std::list<gm_complex_data_type*>& L_c = complexPointers[thread_id];
    std::list<gm_complex_data_type*>::iterator i_c;
    for (i_c = L_c.begin(); i_c != L_c.end(); i_c++) {
        if (*i_c == ptr) {
            //complex type
            delete[] *i_c;

            L_c.erase(i_c);
            return;
        }
    }
}

void gm_mem_helper::cleanup() {
    // remove every thread'
    for (int p = 0; p < max; p++) {
        std::list<void*>& L = primitivePointers[p];
        std::list<void*>::iterator i;
        for (i = L.begin(); i != L.end(); i++) {
#ifdef BARRELFISH
            /* TODO: free */
#else
            float* P = (float*) *i;
            delete[] P;
#endif
        }
        L.clear();

        std::list<gm_complex_data_type*>& L_c = complexPointers[p];
        std::list<gm_complex_data_type*>::iterator i_c;
        for (i_c = L_c.begin(); i_c != L_c.end(); i_c++) {
            delete *i_c;
        }
        L_c.clear();
    }
}

/*
 * Wrappers to call the gm_mem_helper methods
 */
int64_t* gm_rt_allocate_long(size_t sz, int thread_id) {
#ifdef BARRELFISH
    int64_t *ptr = (int64_t *)shl__alloc_memory(sz * sizeof(int64_t));
#else
    int64_t* ptr = new int64_t[sz];
#endif
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}

int32_t* gm_rt_allocate_int(size_t sz, int thread_id) {
#ifdef BARRELFISH
    int32_t *ptr = (int32_t *)shl__alloc_memory(sz * sizeof(int32_t));
#else
    int32_t* ptr = new int32_t[sz];
#endif
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}

float* gm_rt_allocate_float(size_t sz, int thread_id) {
#ifdef BARRELFISH
    float *ptr = (float *)shl__alloc_memory(sz * sizeof(float));
#else
    float* ptr = new float[sz];
#endif
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}

bool* gm_rt_allocate_bool(size_t sz, int thread_id) {
#ifdef BARRELFISH
    bool *ptr = (bool *)shl__alloc_memory(sz * sizeof(bool));
#else
    bool* ptr = new bool[sz];
#endif
    assert(ptr != NULL);
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}

double* gm_rt_allocate_double(size_t sz, int thread_id) {
#ifdef BARRELFISH
    double *ptr = (double *)shl__alloc_memory(sz * sizeof(double));
#else
    double* ptr = new double[sz];
#endif
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}

node_t* gm_rt_allocate_node_t(size_t sz, int thread_id) {
#ifdef BARRELFISH
    node_t *ptr = (node_t *)shl__alloc_memory(sz * sizeof(node_t));
#else
    node_t* ptr = new node_t[sz];
#endif
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}
edge_t* gm_rt_allocate_edge_t(size_t sz, int thread_id) {
#ifdef BARRELFISH
    edge_t *ptr = (edge_t *)shl__alloc_memory(sz * sizeof(edge_t));
#else
    edge_t* ptr = new edge_t[sz];
#endif
    _GM_MEM.save(ptr, 0, thread_id);
    return ptr;
}

void gm_rt_deallocate(void* ptr, int thread_id) {
#ifdef BARRELFISH
    /* TODO: free memory */
#endif
    _GM_MEM.clear(ptr, 0, thread_id);
}

void gm_rt_cleanup() {
    _GM_MEM.cleanup();
}
