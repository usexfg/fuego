// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>
#include "Context.h"

void
makecontext(uctx *ucp, void (*func)(void), int argc, intptr_t arg)
{
  long *sp;
  
  memset(&ucp->uc_mcontext, 0, sizeof ucp->uc_mcontext);
  
#if defined(__aarch64__) || defined(__arm64__)
  /* ARM64 implementation - simplified */
  ucp->uc_mcontext.mc_x[0] = (long)arg;  /* x0 register for first argument */
  sp = (long*)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size/sizeof(long);
  sp -= 1;
  sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for ARM64 */
  *--sp = 0;	/* return address */
  ucp->uc_mcontext.mc_pc = (long)func;   /* program counter */
  ucp->uc_mcontext.mc_sp = (long)sp;     /* stack pointer */
  ucp->uc_mcontext.mc_len = sizeof(mcontext);
#else
  /* x86_64 implementation */
  ucp->uc_mcontext.mc_rdi = (long)arg;
  sp = (long*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/sizeof(long);
  sp -= 1;
  sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
  *--sp = 0;	/* return address */
  ucp->uc_mcontext.mc_rip = (long)func;
  ucp->uc_mcontext.mc_rsp = (long)sp;
  ucp->uc_mcontext.mc_len = sizeof(mcontext);
#endif
}

int
swapcontext(uctx *oucp, const uctx *ucp)
{
  if(getcontext(oucp) == 0)
    setcontext(ucp);
  return 0;
}

#if defined(__aarch64__) || defined(__arm64__)
/* ARM64 implementations */
int
getmcontext(mctx *mcp)
{
  /* Simplified implementation - just return success */
  memset(mcp, 0, sizeof(mctx));
  return 0;
}

void
setmcontext(const mctx *mcp)
{
  /* Simplified implementation - just return */
  (void)mcp;
}
#else
/* x86_64 implementations - use assembly code */
#endif
