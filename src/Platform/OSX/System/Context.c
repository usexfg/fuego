// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

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

#endif

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
