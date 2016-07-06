/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Id: uicDcr.h,v 1.2 2004/03/31 21:27:28 rthrift Exp  */
/* uicDcr.h - IBM Universal Int Controller (UIC) DCR access assembly routines */

/*******************************************************************************
   This source and object code has been made available to you by IBM on an
   AS-IS basis.

   IT IS PROVIDED WITHOUT WARRANTY OF ANY KIND, INCLUDING THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE OR OF NONINFRINGEMENT
   OF THIRD PARTY RIGHTS.  IN NO EVENT SHALL IBM OR ITS LICENSORS BE LIABLE
   FOR INCIDENTAL, CONSEQUENTIAL OR PUNITIVE DAMAGES.  IBM'S OR ITS LICENSOR'S
   DAMAGES FOR ANY CAUSE OF ACTION, WHETHER IN CONTRACT OR IN TORT, AT LAW OR
   AT EQUITY, SHALL BE LIMITED TO A MAXIMUM OF $1,000 PER LICENSE.  No license
   under IBM patents or patent applications is to be implied by the copyright
   license.

   Any user of this software should understand that neither IBM nor its
   licensors will be responsible for any consequences resulting from the use
   of this software.

   Any person who transfers this source code or any derivative work must
   include the IBM copyright notice, this paragraph, and the preceding two
   paragraphs in the transferred software.

   Any person who transfers this object code or any derivative work must
   include the IBM copyright notice in the transferred software.

   COPYRIGHT   I B M   CORPORATION 2000
   LICENSED MATERIAL  -  PROGRAM PROPERTY OF  I B M"

*******************************************************************************/

/* Copyright 1984-1999 Wind River Systems, Inc. */

/*
modification history
--------------------
12 Nov. 2003, Varian, Inc., R. Thrift:
	code formatting and comments cleanup, no functional changes.
01d,12jun02,pch  SPR 74987: C++
01c,19nov01,pch  cleanup
01b,15may00,mcg  register name updates to match 405GP User Manual
01a,21sep99,mcg  created
*/

/*
This file contains the definitions of the UIC interrupt controller DCR register
access functions.
*/

#ifndef INCuicDcrh
#define INCuicDcrh

#ifdef __cplusplus
    extern "C" {
#endif

/*
 * Universal Interrupt Controller register definitions. Each is a separate
 * DCR register.
 */

#define UIC_SR        (UIC_DCR_BASE+0x0)  /* UIC status                       */
#define UIC_ER        (UIC_DCR_BASE+0x2)  /* UIC enable                       */
#define UIC_CR        (UIC_DCR_BASE+0x3)  /* UIC critical                     */
#define UIC_PR        (UIC_DCR_BASE+0x4)  /* UIC polarity                     */
#define UIC_TR        (UIC_DCR_BASE+0x5)  /* UIC triggering                   */
#define UIC_MSR       (UIC_DCR_BASE+0x6)  /* UIC masked status                */
#define UIC_VR        (UIC_DCR_BASE+0x7)  /* UIC vector                       */
#define UIC_VCR       (UIC_DCR_BASE+0x8)  /* UIC vector configuration         */

#ifndef _ASMLANGUAGE

/*
 *  Universal Interrupt Controller DCR access functions
 */
UINT32  sysDcrUiccrGet(void);
void    sysDcrUiccrSet(UINT32);
UINT32  sysDcrUicerGet(void);
void    sysDcrUicerSet(UINT32);
UINT32  sysDcrUicmsrGet(void);
UINT32  sysDcrUicprGet(void);
void    sysDcrUicprSet(UINT32);
UINT32  sysDcrUicsrGet(void);
void    sysDcrUicsrClear(UINT32);
UINT32  sysDcrUictrGet(void);
void    sysDcrUictrSet(UINT32);
UINT32  sysDcrUicvrGet(void);
void    sysDcrUicvcrSet(UINT32);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
    }
#endif

#endif  /* INCuicDcrh */
