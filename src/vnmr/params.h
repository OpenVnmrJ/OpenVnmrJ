/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef PARAMS_H
#define PARAMS_H
/* parameter protection definitions using a bit field */
/* if the bit is set, the comment is true */
/* Note:  You have 32 bits to work with, as the field is a longword */
#define P_ARR	1	/* bit 0  - cannot array the parameter */
#define P_ACT	2	/* bit 1  - cannot change active/not active status */
#define P_VAL	4	/* bit 2  - cannot change the parameter value */
#define P_MAC	8	/* bit 3  - causes macro to be executed */
#define P_REX	16	/* bit 4  - avoids automatic re-display */
#define P_DEL	32	/* bit 5  - cannot delete parameter */
#define P_SYS	64	/* bit 6  - system ID for spectrometer - datastation */
#define P_CPY	128	/* bit 7  - cannot copy parameter from tree to tree */
#define P_NOA	256	/* bit 8  - will not set array parameter */
#define P_ENU	512	/* bit 9  - cannot set parameter enumeral values */
#define P_GRP	1024	/* bit 10 - cannot change the parameter's group */
#define P_PRO	2048	/* bit 11 - cannot change protection bits */
#define P_IPA	4096	/* bit 12 - causes _ipa macro to be called */
#define P_MMS   8192    /* bit 13 - lookup min, max, step values in table */
#define P_LOCK 16384    /* bit 14 - variable marked for locking (rtx) */
#define P_GLO  32768    /* bit 15 - global variable not sent to multiple Vnmrbg's in VJ */
#define P_JRX  65536    /* bit 16 - forces automatic redisplay of parameter templates in VJ */

#endif
