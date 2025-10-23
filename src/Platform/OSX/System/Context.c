// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>
#include "Context.h"

#if defined(__arm64__) || defined(__aarch64__)

// ARM64 offsets from asm.s
#define REG_SZ                   (8)
#define MCONTEXT_GREGS           (184)
#define SP_OFFSET                432
#define PC_OFFSET                440
#define PSTATE_OFFSET            448
#define FPSIMD_CONTEXT_OFFSET    464
#define REG_OFFSET(reg)          (MCONTEXT_GREGS + ((reg) * REG_SZ))

void
makecontext(uctx *ucp, void (*func)(void), intptr_t arg)
{
  unsigned char *sp;
  unsigned char *mcontext_base = (unsigned char *)&ucp->uc_mcontext;
  
  // Clear the mcontext
  memset(&ucp->uc_mcontext, 0, sizeof ucp->uc_mcontext);
  
  // Set x0 register (first argument) using the offset
  *(long *)(mcontext_base + REG_OFFSET(0)) = (long)arg;
  
  // Set up stack pointer (16-byte aligned for ARM64)
  sp = (unsigned char *)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size;
  sp = (unsigned char *)((uintptr_t)sp - ((uintptr_t)sp % 16));
  *(long *)(mcontext_base + SP_OFFSET) = (long)sp;
  
  // Set program counter
  *(long *)(mcontext_base + PC_OFFSET) = (long)func;
}

#else

// x86_64 implementation
void
makecontext(uctx *ucp, void (*func)(void), intptr_t arg)
{
  long *sp;
  
  memset(&ucp->uc_mcontext, 0, sizeof ucp->uc_mcontext);
  ucp->uc_mcontext.mc_rdi = (long)arg;
  sp = (long*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/sizeof(long);
  sp -= 1;
  sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
  *--sp = 0;	/* return address */
  ucp->uc_mcontext.mc_rip = (long)func;
  ucp->uc_mcontext.mc_rsp = (long)sp;
}

#endif

int
swapcontext(uctx *oucp, const uctx *ucp)
{
  if(getcontext(oucp) == 0)
    setcontext(ucp);
  return 0;
}
