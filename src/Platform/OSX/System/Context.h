/*
 * MIT License
 *
 * Copyright (c) 2017-2025 Fuego Developers
 * Copyright (c) 2016-2019 The Karbowanec developers
 * Copyright (c) 2012-2018 The CryptoNote developers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <ucontext.h>

/* Use standard ucontext on all modern macOS systems */
typedef ucontext_t uctx;

/* Helper function to initialize a context */
static inline void init_context(uctx *ucp, void (*func)(void), void *stack, size_t stack_size) {
    memset(ucp, 0, sizeof(uctx));
    getcontext(ucp);
    ucp->uc_stack.ss_sp = stack;
    ucp->uc_stack.ss_size = stack_size;
    ucp->uc_link = NULL;
    
    /* makecontext will be called by the caller with appropriate arguments */
}

#ifdef __cplusplus
}
#endif