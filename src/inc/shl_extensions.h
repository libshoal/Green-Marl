#ifndef __SHL_EXTENSIONS_H
#define __SHL_EXTENSIONS_H

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

#endif
