// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define _XOPEN_SOURCE 700
#include <string.h>
#include <ucontext.h>
#include <stdarg.h>
#include "Context.h"

/* This file provides compatibility functions for macOS ucontext */

// Compatibility functions for mcontext operations
int getmcontext(mctx *mcp) {
    // This is a no-op compatibility function
    // The actual context switching is handled by the system ucontext functions
    memset(mcp, 0, sizeof(mcontext_t));
    return 0;
}

int setmcontext(const mctx *mcp) {
    // This is a no-op compatibility function
    // The actual context switching is handled by the system ucontext functions
    (void)mcp; // Suppress unused parameter warning
    return 0;
}
