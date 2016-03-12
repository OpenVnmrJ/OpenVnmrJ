/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#ifndef INCa32Bridgefuncsh
#define INCa32Bridgefuncsh

#include <wdLib.h>
#include <semLib.h>
#include "lc.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)



/* extern int sendCntrlFifoList(int* list, int size, int ntimes , int rem, int startfifo); */
extern int sendCntrlFifoList(int* list, int size, int ntimes);
extern int writeCntrlFifoWord(int word);
extern int writeCntrlFifoBuf(int *list, int size);
extern int flushCntrlFifoRemainingWords();
extern int startCntrlFifo();
extern void wait4CntrlFifoStop();
extern int allocDataWillBlock();
extern FID_STAT_BLOCK *allocAcqDataBlock(ulong_t fidnum, ulong_t np, ulong_t ct, ulong_t endct, 
                  ulong_t nt, ulong_t size, long *tag2snd, long *scan_data_adr );

extern int abortFifoBufTransfer();
extern int SystemRollCall();
extern int ClearCtlrStates();
extern int SystemSync(int postdelay, int prepflag);
extern void clearSystemSyncFlag();

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif
