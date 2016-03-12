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
#ifndef INCacqcmdsh
#define INCacqcmdsh

/*  3 notes:

    1)  The X-codes previously defined on the VM02 are
        fenced off with ifndef ACQ_SUN.  This is done
        so you can include both ACQ_SUN.h and this
        file without a lot of messages about symbols
        being redefined.

    2)  If you include both this file and ACQ_SUN.h,
        then include ACQ_SUN.h first.

    3)  This file does NOT define ACQ_SUN, since here
        we only define a series of X-codes which the
        new digital console (NDC) inherits from the VM02
        programs.  ACQ_SUN.h also defines data structs
        not present in the NDC.  Some host computer
        programs for the NDC make use of these older
        data structs so the source code may be reused.   */

#ifndef ACQ_SUN
/* --- interactive hardware commands --- */
#define LKMODE		1
#define LKPOWER		2
#define LKGAIN		3
#define LKPHASE		4
#define INSERT		5
#define EJECT		6
#define BEARON		7
#define BEAROFF		8
#define SETSPD		9
#define SETRATE		10
#define GETSPD		11
#define GETRATE		12
#define SHIMDAC		13
#define GETDAC		14
#define GETSTATUS	15
#define CODECHG		16
#define INFIFO		17
#define LK_GAIN		3
#define LK_PWR		2
#define LK_PHS		4
#define LK_LVL		18
#define TWEAK		19
#define SET_DAC		20
#define SEL_SET		21
#define SHIMI		22
#define SPN_STA		23
#define RTN_SHLK	24
#define FIX_ACODE	25
#define SET_ATTN	26
#define	RCVRGAIN	27
#define STARTTUNE	28
#define STOPTUNE	29
#define WSRAM		30
#define RSRAM		31
#define SET_TUNE	32
#define RESET_VT	33
#define SETLOC		34
#define SETMASTHRES	35
#define SYNC_FREE	36

/*  shim criteria definitions  */

#define LOOSE 1
#define MEDIUM 2
#define TIGHT 3
#define EXCELLENT 4

#endif    ACQ_SUN

/* Added for nessie */
/*  The A_update task responds to these  */
#define FIX_ACODES	35
#define FIX_RTVARS	36
#define CHG_TABLE	37
#define CHG_RTVAR	38
#define CHG_ACODE	39
#define FIX_APREG	40
#define FIX_APREGS	41

/*  The X_interp task responds to these  */
#define LKTC		42
#define LKACQTC		43

#define STOP_SHIMI	44
#define SHIMSET		45
#define SETSTATUS	46
#define EJECTOFF        82

/*  More defines common to both nessie and the VM02  */
/*  These MUST correspond with definitions in ACQ_SUN.h, SCCS category xracq  */

#ifndef ACQ_SUN
#define SETTEMP		51
#define BUMPSAMPLE	52
#define READLKLVL	53
#define LKFREQ		54
#define STATRATE	55
#endif

/* --- Lock Mode Values --- */
#define LKOFF           0
#define LKON            1
#define AUTOLKON        3
/* MERCURY */
#define LM_10DB         0x02
#define LM_20DB         0x04
#define LM_4_400        0x08
#define LM_ON_OFF       0x10
#define LM_FAST         0x20
#define LM_SLOW         0X40

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)




#else                                                   
/* --------- NON-ANSI/C++ prototypes ------------  */




#endif

#ifdef __cplusplus
}
#endif

#endif

