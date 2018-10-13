/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Simplex constant definitions  */
#define Z1F		2
#define Z1C		3
#define Z1RATIO		30
#define Z2F		4
#define Z2C		5
#define Z3 		6
#define Z4 		7
#define Z2RATIO		50
#define CENTER		2047
#define TRUE		1
#define FALSE		0
#define NUMDACS		32
#define NUMVECS		33
#define LIMIT 		2048
#define STEP		0
#define EXPAND		1
#define CONTRACT 	2
#define	RECAST		3
#define ABORT   	0x1000
#define LOCK_OVER	0x4000
#define LOCK_UNDER	0x2000
#define DAC_RANGE	0x8000
#define FAILURE		0xF000		/* all the above */
#define MINHALF         -1		/* 0xbf000000L   */
#define HALF            1		/* 0x3f000000L   */
#define MINONE          -2		/* 0xbf800000L   */
#define ONE         	2		/* 0x3f800000L   */
#define MINTWO          -4		/* 0xc0000000L   */
#define TWO             4		/* 0x40000000L   */
#define THREEHALVES     3		/* 0x40000000L   */
#define MAXGAIN     	70
#if defined(INOVA) || defined(MERCURY)
#define SIG2BIG         5200		/* was 1550 3-11-89 - 1300 was 9/19/95 */
#define SIG2SML     	500	        /* was 400 9/19/95 */
#else
#define SIG2BIG         1500
#define SIG2SML     	160
#endif
#define SIG34SCALE	4000		/* */
