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
#ifndef INClock_ih
#define INClock_ih

#define INTOVF 32767  /* 0x7FFF, see autoshim.c stubio.c */

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


#define Z0 1	/* WARNING: Must reflect value assigned in shims.h (vnmr) */

#define GET_LOCKED 1
#define GET_Z0     2
#define GET_Z0_LIMIT     3
#define GET_POWER  4
#define GET_PHASE  5
#define GET_MODE   6
#define GET_GAIN   7
#define GET_LKFREQ 8

#define SET_Z0     2
/* #define SET_POWER  3 */
#define SET_PHASE  4
#define SET_MODE   5
/* #define SET_GAIN   6 */
#define SET_LK2KCF 7
#define SET_LK2KCS 8
#define SET_LK20HZ 9
#define SET_LKFREQ 10

#define chlock() \
        get_lk_hw(GET_LOCKED)

#define get_lock_offset() \
        get_lk_hw(GET_Z0)

#define get_limit_lock() \
        get_lk_hw(GET_Z0_LIMIT)

#define getgain() \
        get_lk_hw(GET_GAIN)

#define getpower() \
        get_lk_hw(GET_POWER)

#define getphase() \
        get_lk_hw(GET_PHASE)

#define getmode() \
        get_lk_hw(GET_MODE)

#define get_lkfreq_ap() \
        get_lk_hw(GET_LKFREQ)

#define set_lock_offset(val) \
        set_lk_hw(SET_Z0,val)

#define set_lkfreq_ap(val) \
        set_lk_hw(SET_LKFREQ,val)

/* #define setgain(val) \
/*          set_lk_hw(SET_GAIN,val)
/* 
/* #define setpower(val) \
/*         set_lk_hw(SET_POWER,val)
/* 
/* #define setlkphase(val) \
/*         set_lk_hw(SET_PHASE,val)
/*  */
#define setmode(val) \
        set_lk_hw(SET_MODE,val)

#define lk2kcf() \
        set_lk_hw(SET_LK2KCF,0)

#define lk2kcs() \
        set_lk_hw(SET_LK2KCS,0)

#define lk20Hz() \
        set_lk_hw(SET_LK20HZ,0)

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int init_dac(void);
extern int set_lk_hw(int type, int value);
extern int get_lk_hw(int type);
extern int  Tdelay( int val );
extern int  Ldelay( int *tickset, int val );
extern int  Tcheck( int tickset );
extern unsigned long secondClock(unsigned long *chkval,int mode);
extern int calcgain(long level,long reqlevel);
extern int setLockPar( int acode, int value, int startfifo );
extern int setLockPower( int lockpower_value, int startfifo );
extern int storeLockPar( int acode, int value );

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int init_dac();
extern int set_lk_hw();
extern int get_lk_hw();
extern int  Tdelay();
extern int  Ldelay();
extern int  Tcheck();
extern unsigned long secondClock();
extern int calcgain();
extern int setLockPar();
extern int setLockPower();
extern int storeLockPar();

#endif

#ifdef __cplusplus
}
#endif

#endif
