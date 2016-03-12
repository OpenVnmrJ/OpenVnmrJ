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

#ifndef INCautolockh
#define INCautolockh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define NO_METHOD	0
#define USE_Z0		1
#define USE_LOCKFREQ	2

#define LOCK_OFF        0
#define LOCK_ON         1

typedef struct {
	long	method;
	long	smallest;
	long	largest;
	long	middle;
	long	initial;
	long	current;
	long	best;
} FIND_LOCK_OBJ;

typedef FIND_LOCK_OBJ *FIND_LOCK_ID;

typedef struct _alockmsg_ {
	int	mode;
	int	maxpwr;
	int	maxgain;
	int	arg4;
	double	arg5;
} ALOCK_MSG;

typedef ALOCK_MSG *ALOCK_MSG_ID;

extern int do_autolock(int lkmode,int maxpwr,int maxgain, int sampleHasChanged);
extern int doAutoLock(int lkmode,int maxpwr,int maxgain, int sampleHasChanged);
extern int getlkfid(int *data,int nt,int filter);
extern int dac_limit() ;
extern double getLockFreqAP() ;
extern int getLockLevel() ;
extern int setLockMode(int newMode);
extern int getLockMode();
extern int setLockPower(int newPower);
extern int getLockPower() ;
extern int setLockGain(int newGain);
extern int getLockGain() ;
extern int setLockPhase(int newPhase);
extern int getLockPhase() ;
extern int calcgain(long level,long reqlevel);

#ifdef __cplusplus
}
#endif

#endif
