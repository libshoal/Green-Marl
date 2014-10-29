#include "shl_extensions.h"

#include <cstdio>
#include <vector>
#include <cassert>

using namespace std;

// SK: also change in shl_extensions.h
const char *shl__loop_to_string[6] = {
    "LOOP_UNKNOWN",
    "LOOP_NBS",
    "LOOP_EDGES",
    "LOOP_EDGES_NBS", // SK: from neighbor iteration
    "LOOP_NODES",
    "LOOP_CONSTANT"
};

/* **************************************************
 * COST MODEL
 * ************************************************** */

// Keep track of which loops current code is executed in
vector<shl__loop_t> shl__loops;

void shl__loop_enter(shl__loop_t l)
{
    printf("loop_enter: %d=%s  old[%s] ", l, shl__loop_to_string[l], shl__loop_print());
    switch (l) {
    case LOOP_NODES:
    case LOOP_CONSTANT:
        shl__loops.push_back(l);
        break;
    case LOOP_NBS:
        if (shl__loops.back() == LOOP_NODES) {
            // Remove LOOP_NODES, we are now iterating
            // over edges
            shl__loops.pop_back();
            shl__loops.push_back(LOOP_EDGES_NBS);
            break;
        }
        else if (shl__loops.back() == LOOP_EDGES_NBS) {

            shl__loops.push_back(LOOP_EDGES);
            break;
        }
        else {

            // SK: it can also be nodes -> nbs -> nbs, which is what?
            assert(!"NYI: got LOOPS_NBS, but tail is not LOOP_NODES");
        }
    default:
        assert(!"Don't know how to determine cost for given loop type");
    }
    printf("new[%s]\n", shl__loop_print());
}

void shl__loop_leave(shl__loop_t l)
{
    shl__loop_t tail = shl__loops.back();

    // Switch the last element of the "stack"
    switch (tail) {
    case LOOP_EDGES_NBS:
        // Undo
        assert (l==LOOP_NBS);
        shl__loops.pop_back();
        shl__loops.push_back(LOOP_NODES);
        break;
    case LOOP_EDGES:
    case LOOP_NODES:
    case LOOP_CONSTANT:
        assert (tail==l || l==LOOP_NBS); // for tail = LOOP_EDGES, l = LOOP_NBS
        shl__loops.pop_back();
    }

    printf("loop_leave: %d\n", l);
}

const char* shl__loop_print(void)
{
    static char buffer[1024];
    char *ptr = buffer;

    ptr += sprintf(ptr, "stack: ");

    vector<shl__loop_t>::iterator i;
    for (i=shl__loops.begin(); i!=shl__loops.end(); i++) {

        ptr += sprintf(ptr, "%d=%s ", (*i), shl__loop_to_string[*i]);
    }

    return buffer;
}
