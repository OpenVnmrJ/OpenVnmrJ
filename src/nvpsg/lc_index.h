/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* 
    lc_index.h
    Indices for real-time variables 
*/

/*
    these indices correspond to the order in the lc structure.
    A 800 number is assigned for those which need to be worked on again.
*/

#define MAX_ADC_OBJECTS 4
#define MAX_STM_OBJECTS 4

#define TABOFFSET 2
#define STARTINDEX 0
#define NPINDEX (0)
#define NTINDEX (1)
#define CTINDEX (2)
#define CTSSINDEX (9)
#define FIDINDEX (10)
#define DPFINDEX (20)
#define OPHINDEX (27)
#define BSINDEX (28)
#define BSCTINDEX (29)
#define SSINDEX (30)
#define SSCTINDEX (31)
#define CFCTINDEX (34)
#define CPFINDEX (35)
#define RTTABINDEX (37)
#define TPWRINDEX (46)
#define DPWRINDEX (47)
#define TPHSINDEX (48)
#define DPHSINDEX (49)
#define SRATEINDEX (51)
#define RTTMPINDEX (52)
#define SPARE1INDEX (53)
#define ID2INDEX (54)
#define ID3INDEX (55)
#define ID4INDEX (56)
#define ZEROINDEX (57)
#define ONEINDEX (58)
#define TWOINDEX (59)
#define THREEINDEX (60)

#define V1INDEX (61)
#define V2INDEX (V1INDEX + 1)
#define V3INDEX (V2INDEX + 1)
#define V4INDEX (V3INDEX + 1)
#define V5INDEX (V4INDEX + 1)
#define V6INDEX (V5INDEX + 1)
#define V7INDEX (V6INDEX + 1)
#define V8INDEX (V7INDEX + 1)
#define V9INDEX (V8INDEX + 1)
#define V10INDEX (V9INDEX + 1)
#define V11INDEX (V10INDEX + 1)
#define V12INDEX (V11INDEX + 1)
#define V13INDEX (V12INDEX + 1)
#define V14INDEX (V13INDEX + 1)
#define V15INDEX (V14INDEX + 1)
#define V16INDEX (V15INDEX + 1)
#define V17INDEX (V16INDEX + 1)
#define V18INDEX (V17INDEX + 1)
#define V19INDEX (V18INDEX + 1)
#define V20INDEX (V19INDEX + 1)
#define V21INDEX (V20INDEX + 1)
#define V22INDEX (V21INDEX + 1)
#define V23INDEX (V22INDEX + 1)
#define V24INDEX (V23INDEX + 1)
#define V25INDEX (V24INDEX + 1)
#define V26INDEX (V25INDEX + 1)
#define V27INDEX (V26INDEX + 1)
#define V28INDEX (V27INDEX + 1)
#define V29INDEX (V28INDEX + 1)
#define V30INDEX (V29INDEX + 1)
#define V31INDEX (V30INDEX + 1)
#define V32INDEX (V31INDEX + 1)

#define HSINDEX (802)
#define NSPINDEX (803)
#define ARRAYDIMINDEX (805)
#define MXSCLINDEX (807)
#define RLXDELAYINDEX (808)
#define NFIDBUFINDEX (809)
#define RECGAININDEX (810)
#define NPNOISEINDEX (811)
#define DTMCTRLINDEX (812)
#define ACTIVERCVRINDEX (813)
#define GTBLINDEX (814)
#define ADCCTRLINDEX (815)
#define SCANFLAGINDEX (816)
#define ILFLAGRTINDEX (817)
#define ILSSINDEX (818)
#define ILCTSSINDEX (819)
#define TMPRTINDEX (820)
#define ACQIFLAGRT (821)
#define MAXSUMINDEX (822)
#define STARTTINDEX (823)
#define INITFLAGINDEX (824)
#define INCDELAYINDEX (825)
#define ENDINCDELAYINDEX (826)
#define CLRBSFLAGINDEX (827)

#define LASTINDEX (900)

#define RT_TAB_SIZE (LASTINDEX+2)
#define RTVAR_BUFFER    (0xaaaaaa)

