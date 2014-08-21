#ifndef __SHL_EXTENSIONS_H
#define __SHL_EXTENSIONS_H

typedef enum {
    LOOP_UNKNOWN,
    LOOP_NBS,
    LOOP_EDGES,
    LOOP_NODES,
    LOOP_CONSTANT
} shl__loop_t;

void shl__loop_enter(shl__loop_t);
void shl__loop_leave(shl__loop_t);

#endif
