#ifndef __SHL_EXTENSIONS_H
#define __SHL_EXTENSIONS_H

#include <string>
#include <map>

using namespace std;

struct sk_gm_array {
    string dest;
    string src;
    string type;
    string num;
    bool dynamic;
    bool buildin;
    bool init_done;
    bool is_edge_property;
    bool is_node_property;
    bool is_indexed;
};


// Keep consistent with shl_extensions.cc
typedef enum {
    LOOP_UNKNOWN,
    LOOP_NBS,
    LOOP_EDGES,
    LOOP_EDGES_NBS,
    LOOP_NODES,
    LOOP_CONSTANT
} shl__loop_t;

void shl__loop_enter(shl__loop_t);
void shl__loop_leave(shl__loop_t);
const char* shl__loop_print(void);

const char* shl__get_array_type(const char* s);

#endif
