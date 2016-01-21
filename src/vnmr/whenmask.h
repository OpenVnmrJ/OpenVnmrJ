/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------
|
|	Header file for When mask definitions
|
+----------------------------------------------------------------------*/

/* --- Has anyone else already typedef these,check on other header files --- */
#if  !defined(WHEN_MASK)

/* --- define this header file name for conditional compile use of others */
#define WHEN_MASK

/* When Conditional Processing Bit Values */
#define WHEN_ERR_PROC 0x1
#define WHEN_EXP_PROC 0x2
#define WHEN_NT_PROC  0x4
#define WHEN_BS_PROC  0x8
#define WHEN_SU_PROC 0x10
#define WHEN_GA_PROC 0x20

#endif
