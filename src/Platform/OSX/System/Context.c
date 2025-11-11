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

#include <string.h>
#include <ucontext.h>
#include "Context.h"

/*
 * On modern macOS systems, we use the standard ucontext functions
 * provided by the system. The Context.h header defines uctx as ucontext_t,
 * so we can use the standard POSIX functions directly.
 * 
 * Note: Some ucontext functions are deprecated on modern macOS but are
 * still available for backward compatibility. The system provides them
 * for use with existing code.
 */

// Helper function to create a context with a stack
// This replaces the complex architecture-specific implementations
void init_context_with_stack(uctx *ucp, void (*func)(void), void *stack, size_t stack_size) {
    memset(ucp, 0, sizeof(uctx));
    getcontext(ucp);
    ucp->uc_stack.ss_sp = stack;
    ucp->uc_stack.ss_size = stack_size;
    ucp->uc_link = NULL;
    
    // Initialize the context for the specified function
    // Note: makecontext signature is makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
    // The exact call to makecontext should be made by the caller with appropriate arguments
}

// For direct compatibility with existing code that expects these functions
// Note: These are just wrappers around the standard functions
int swap_uctx(uctx *oucp, const uctx *ucp) {
    return swapcontext(oucp, ucp);
}

// Legacy function name compatibility
int swapcontext_uctx(uctx *oucp, const uctx *ucp) {
    return swapcontext(oucp, ucp);
}

/*
 * Context management functions for the Dispatcher.
 * These provide a simplified interface on top of the standard ucontext functions.
 */

// Create a new context for a procedure with arguments
// This is a helper that abstracts the makecontext call
void make_uctx_with_args(uctx *ucp, void (*func)(void), intptr_t arg) {
    // For modern macOS, we use the standard makecontext
    // The function signature varies, so we use the variadic form
    makecontext(ucp, func, 1, arg);
}

// Initialize a context procedure that takes a single void* argument
void init_procedure_context(uctx *ucp, void (*proc)(void*), void *arg, void *stack, size_t stack_size) {
    init_context_with_stack(ucp, (void(*)(void))proc, stack, stack_size);
    
    // Wrap the procedure to take the argument properly
    // This allows the standard makecontext to work with our procedure signature
    // The actual makecontext call should be made by the caller
}

// Get current context
int get_current_context(uctx *ucp) {
    return getcontext(ucp);
}

// Set context (equivalent to setcontext)
int set_current_context(const uctx *ucp) {
    return setcontext(ucp);
}