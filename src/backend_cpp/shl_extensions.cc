#include "shl_extensions.h"

#include <cstdio>
#include <vector>
#include <cassert>

using namespace std;

/* **************************************************
 * COST MODEL
 * ************************************************** */

// Keep track of which loops current code is executed in
vector<shl__loop_t> shl__loops;

void shl__loop_enter(shl__loop_t l)
{
    printf("loop_enter: %d\n", l);
    switch (l) {
    case LOOP_NODES:
        shl__loops.push_back(LOOP_NODES);
        break;
    case LOOP_NBS:
        if (shl__loops.back() == LOOP_NODES) {
            // Remove LOOP_NODES, we are now iterating
            // over edges
            shl__loops.pop_back();
            shl__loops.push_back(LOOP_EDGES);
            break;
        }
    default:
        assert(!"Don't know how to determine cost for given loop type");
    }
}

void shl__loop_leave(shl__loop_t l)
{
    shl__loop_t tail = shl__loops.back();
    if (tail==LOOP_EDGES) {

        // Undo
        assert (l==LOOP_NBS);
        shl__loops.pop_back();
        shl__loops.push_back(LOOP_NODES);
    }
    printf("loop_leave: %d\n", l);
}
