/*  armdefs.h -- ARMulator common definitions:  ARM6 Instruction Emulator.
    Copyright (C) 1994 Advanced RISC Machines Ltd.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA. */

/* heavily modified to only support arm7 and thumb David Welch 2010 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1
#define LOW 0
#define HIGH 1
#define LOWHIGH 1
#define HIGHLOW 2

#ifndef __STDC__
typedef char *VoidStar;
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>

typedef uint32_t ARMword;
typedef int32_t ARMsword;
typedef uint64_t ARMdword;
typedef int64_t ARMsdword;
#else
typedef unsigned int ARMword;   /* must be 32 bits wide */
typedef signed int ARMsword;
typedef unsigned long long ARMdword;    /* Must be at least 64 bits wide.  */
typedef signed long long ARMsdword;
#endif
typedef struct ARMul_State ARMul_State;

typedef unsigned ARMul_CPInits (ARMul_State * state);
typedef unsigned ARMul_CPExits (ARMul_State * state);
typedef unsigned ARMul_LDCs (ARMul_State * state, unsigned type, ARMword instr, ARMword value);
typedef unsigned ARMul_STCs (ARMul_State * state, unsigned type, ARMword instr, ARMword * value);
typedef unsigned ARMul_MRCs (ARMul_State * state, unsigned type, ARMword instr, ARMword * value);
typedef unsigned ARMul_MCRs (ARMul_State * state, unsigned type, ARMword instr, ARMword value);
typedef unsigned ARMul_CDPs (ARMul_State * state, unsigned type, ARMword instr);
typedef unsigned ARMul_CPReads (ARMul_State * state, unsigned reg, ARMword * value);
typedef unsigned ARMul_CPWrites (ARMul_State * state, unsigned reg, ARMword value);

struct ARMul_State
{
    ARMword Emulate;      /* to start and stop emulation */
    unsigned EndCondition;    /* reason for stopping */
    unsigned ErrorCode;       /* type of illegal instruction */
    ARMword Reg[16];      /* the current register file */
    ARMword RegBank[7][16];   /* all the registers */
    /* 40 bit accumulator.  We always keep this 64 bits wide,
     and move only 40 bits out of it in an MRA insn.  */
    ARMdword Accumulator;
    ARMword Cpsr;         /* the current psr */
    ARMword Spsr[7];      /* the exception psr's */
    ARMword NFlag, ZFlag, CFlag, VFlag, IFFlags;  /* dummy flags for speed */
    ARMword SFlag;
    ARMword TFlag;        /* Thumb state */
    ARMword Bank;         /* the current register bank */
    ARMword Mode;         /* the current mode */
    ARMword instr, pc, temp;  /* saved register state */
    ARMword loaded, decoded;  /* saved pipeline state */
    unsigned long NumScycles, NumNcycles, NumIcycles, NumCcycles, NumFcycles; /* emulated cycles used */
    unsigned long NumInstrs;  /* the number of instructions executed */
    unsigned NextInstr;
    unsigned VectorCatch;     /* caught exception mask */
    unsigned CallDebug;       /* set to call the debugger */
    unsigned CanWatch;        /* set by memory interface if its willing to suffer the
                                 overhead of checking for watchpoints on each memory
                                 access */
    unsigned MemReadDebug, MemWriteDebug;
    unsigned long StopHandle;

    unsigned char *OSptr;     /* OS Handle */
    char *CommandLine;        /* Command Line from ARMsd */

    ARMul_MRCs *MRC[16];      /* MRC instruction */
    ARMul_MCRs *MCR[16];      /* MCR instruction */
    unsigned long LastTime;   /* Value of last call to ARMul_Time() */

    unsigned EventSet;        /* the number of events in the queue */
    unsigned long Now;        /* time to the nearest cycle */
    struct EventNode **EventPtr;  /* the event list */

    unsigned Exception;       /* enable the next four values */
    unsigned Debug;           /* show instructions as they are executed */
    unsigned NresetSig;       /* reset the processor */
    unsigned NfiqSig;
    unsigned NirqSig;

    unsigned abortSig;
    unsigned NtransSig;
    unsigned bigendSig;
    unsigned lateabtSig;
    ARMword Vector;       /* synthesize aborts in cycle modes */
    ARMword Aborted;      /* sticky flag for aborts */
    ARMword Reseted;      /* sticky flag for Reset */
    ARMword Inted, LastInted; /* sticky flags for interrupts */
    ARMword Base;         /* extra hand for base writeback */
    ARMword AbortAddr;        /* to keep track of Prefetch aborts */

    const struct Dbg_HostosInterface *hostif;

    unsigned verbose;     /* Print various messages like the banner */
};

#define ResetPin NresetSig
#define FIQPin NfiqSig
#define IRQPin NirqSig
#define AbortPin abortSig
#define TransPin NtransSig
#define BigEndPin bigendSig
#define LateAbortPin lateabtSig

/***************************************************************************\
*                        Properties of ARM we know about                    *
\***************************************************************************/

/* The bitflags */
#define ARM_Nexec_Prop   0x02
#define ARM_Debug_Prop   0x10
#define ARM_Isync_Prop   ARM_Debug_Prop
#define ARM_Lock_Prop    0x20
#define ARM_v4_Prop      0x40

/***************************************************************************\
*                   Macros to extract instruction fields                    *
\***************************************************************************/

#define BIT(n) ( (ARMword)(instr>>(n))&1)   /* bit n of instruction */
#define BITS(m,n) ( (ARMword)(instr<<(31-(n))) >> ((31-(n))+(m)) )  /* bits m to n of instr */
#define TOPBITS(n) (instr >> (n))   /* bits 31 to n of instr */

/***************************************************************************\
*                      The hardware vector addresses                        *
\***************************************************************************/

#define ARMResetV 0L
#define ARMUndefinedInstrV 4L
#define ARMSWIV 8L
#define ARMPrefetchAbortV 12L
#define ARMDataAbortV 16L
#define ARMAddrExceptnV 20L
#define ARMIRQV 24L
#define ARMFIQV 28L
#define ARMErrorV 32L       /* This is an offset, not an address ! */

#define ARMul_ResetV ARMResetV
#define ARMul_UndefinedInstrV ARMUndefinedInstrV
#define ARMul_SWIV ARMSWIV
#define ARMul_PrefetchAbortV ARMPrefetchAbortV
#define ARMul_DataAbortV ARMDataAbortV
#define ARMul_AddrExceptnV ARMAddrExceptnV
#define ARMul_IRQV ARMIRQV
#define ARMul_FIQV ARMFIQV

/***************************************************************************\
*                          Mode and Bank Constants                          *
\***************************************************************************/

#define USER26MODE   0L
#define FIQ26MODE    1L
#define IRQ26MODE    2L
#define SVC26MODE    3L
#define USER32MODE  16L
#define FIQ32MODE   17L
#define IRQ32MODE   18L
#define SVC32MODE   19L
#define ABORT32MODE 23L
#define UNDEF32MODE 27L
#define SYSTEMMODE  31L

#define ARM32BITMODE (state->Mode > 3)
#define ARMMODE (state->Mode)
#define ARMul_MODEBITS 0x1fL
#define ARMul_MODE32BIT ARM32BITMODE

#define USERBANK 0
#define FIQBANK 1
#define IRQBANK 2
#define SVCBANK 3
#define ABORTBANK 4
#define UNDEFBANK 5
#define DUMMYBANK 6
#define SYSTEMBANK USERBANK

#define BANK_CAN_ACCESS_SPSR(bank) ((bank) != USERBANK && (bank) != SYSTEMBANK && (bank) != DUMMYBANK)

/***************************************************************************\
*                  Definitons of things in the emulator                     *
\***************************************************************************/

extern void ARMul_EmulateInit (void);
extern void ARMul_NewState ( ARMul_State * state);
extern void ARMul_Reset (ARMul_State * state);
extern ARMword ARMul_DoProg (ARMul_State * state);
extern ARMword ARMul_DoInstr (ARMul_State * state);

/***************************************************************************\
*                Definitons of things for event handling                    *
\***************************************************************************/

extern void ARMul_ScheduleEvent (ARMul_State * state, unsigned long delay, unsigned (*func) ());
extern void ARMul_EnvokeEvent (ARMul_State * state);
extern unsigned long ARMul_Time (ARMul_State * state);

/***************************************************************************\
*                          Useful support routines                          *
\***************************************************************************/

extern ARMword ARMul_GetReg (ARMul_State * state, unsigned mode, unsigned reg);
extern void ARMul_SetReg (ARMul_State * state, unsigned mode, unsigned reg, ARMword value);
extern ARMword ARMul_GetPC (ARMul_State * state);
extern ARMword ARMul_GetNextPC (ARMul_State * state);
extern void ARMul_SetPC (ARMul_State * state, ARMword value);
extern ARMword ARMul_GetR15 (ARMul_State * state);
extern void ARMul_SetR15 (ARMul_State * state, ARMword value);

extern ARMword ARMul_GetCPSR (ARMul_State * state);
extern void ARMul_SetCPSR (ARMul_State * state, ARMword value);
extern ARMword ARMul_GetSPSR (ARMul_State * state, ARMword mode);
extern void ARMul_SetSPSR (ARMul_State * state, ARMword mode, ARMword value);

/***************************************************************************\
*                  Definitons of things to handle aborts                    *
\***************************************************************************/

extern void ARMul_Abort (ARMul_State * state, ARMword address);
#define ARMul_ABORTWORD 0xefffffff  /* SWI -1 */
#define ARMul_PREFETCHABORT(address) if (state->AbortAddr == 1) state->AbortAddr = (address & ~3L)
#define ARMul_DATAABORT(address) state->abortSig = HIGH ; state->Aborted = ARMul_DataAbortV ;
#define ARMul_CLEARABORT state->abortSig = LOW

/***************************************************************************\
*              Definitons of things in the memory interface                 *
\***************************************************************************/

extern unsigned ARMul_MemoryInit (ARMul_State * state, unsigned long initmemsize);
extern void ARMul_MemoryExit (ARMul_State * state);

extern ARMword ARMul_LoadInstrS (ARMul_State * state, ARMword address, ARMword isize);
extern ARMword ARMul_LoadInstrN (ARMul_State * state, ARMword address, ARMword isize);
extern ARMword ARMul_ReLoadInstr (ARMul_State * state, ARMword address, ARMword isize);

extern ARMword ARMul_LoadWordS (ARMul_State * state, ARMword address);
extern ARMword ARMul_LoadWordN (ARMul_State * state, ARMword address);
extern ARMword ARMul_LoadHalfWord (ARMul_State * state, ARMword address);
extern ARMword ARMul_LoadByte (ARMul_State * state, ARMword address);

extern void ARMul_StoreWordS (ARMul_State * state, ARMword address, ARMword data);
extern void ARMul_StoreWordN (ARMul_State * state, ARMword address, ARMword data);
extern void ARMul_StoreHalfWord (ARMul_State * state, ARMword address, ARMword data);
extern void ARMul_StoreByte (ARMul_State * state, ARMword address, ARMword data);

extern ARMword ARMul_SwapWord (ARMul_State * state, ARMword address, ARMword data);
extern ARMword ARMul_SwapByte (ARMul_State * state, ARMword address, ARMword data);

extern void ARMul_Icycles (ARMul_State * state, unsigned number, ARMword address);
extern void ARMul_Ccycles (ARMul_State * state, unsigned number, ARMword address);

extern ARMword ARMul_ReadWord (ARMul_State * state, ARMword address);
extern ARMword ARMul_ReadByte (ARMul_State * state, ARMword address);
extern ARMword ARMul_SafeReadByte (ARMul_State * state, ARMword address);
extern void ARMul_WriteWord (ARMul_State * state, ARMword address, ARMword data);
extern void ARMul_WriteByte (ARMul_State * state, ARMword address, ARMword data);
extern void ARMul_SafeWriteByte (ARMul_State * state, ARMword address, ARMword data);

extern ARMword ARMul_MemAccess (ARMul_State * state, ARMword, ARMword, ARMword, ARMword, ARMword, ARMword, ARMword, ARMword, ARMword, ARMword);


/***************************************************************************\
*               Definitons of things in the host environment                *
\***************************************************************************/

extern unsigned ARMul_OSInit (ARMul_State * state);
extern void ARMul_OSExit (ARMul_State * state);
extern unsigned ARMul_OSHandleSWI (ARMul_State * state, ARMword number);
extern ARMword ARMul_OSLastErrorP (ARMul_State * state);

extern ARMword ARMul_Debug (ARMul_State * state, ARMword pc, ARMword instr);
extern unsigned ARMul_OSException (ARMul_State * state, ARMword vector, ARMword pc);
extern int rdi_log;

/***************************************************************************\
*                            Host-dependent stuff                           *
\***************************************************************************/

extern void ARMul_UndefInstr      (ARMul_State *, ARMword);
extern void ARMul_FixCPSR         (ARMul_State *, ARMword, ARMword);
extern void ARMul_FixSPSR         (ARMul_State *, ARMword, ARMword);
extern void ARMul_ConsolePrint    (ARMul_State *, const char *, ...);
extern void ARMul_SelectProcessor (ARMul_State *, unsigned);

