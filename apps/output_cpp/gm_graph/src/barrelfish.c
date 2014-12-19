/**
 * \file
 * \brief Test the LUA interpreter library
 */
/*
 * Copyright (c) 2013, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <barrelfish/barrelfish.h>
#include <barrelfish/capabilities.h>

#include "gm_barrelfish.h"

void *shl__alloc_memory(size_t size)
{
    errval_t err;

    struct capref frame;
    err = frame_alloc(&frame, size, &size);
    if (err_is_fail(err)) {
        return NULL;
    }

    void *addr;
    err = vspace_map_one_frame(&addr, size, frame, NULL, NULL);
    if (err_is_fail(err)) {
        // XXX: free cap
        return NULL;
    }


    return addr;
}
