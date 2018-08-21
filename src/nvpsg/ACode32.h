/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ACODE32_H
#define ACODE32_H

#define MAX_ROLLCALL_STRLEN 600 

#define ACDKEY          (0xACD00000)
#define KEYMASK         (0xFFF00FFF)
#define ARGMASK         (0x000FF000)
#define INTER_REV_CHK   (ACDKEY | 0)		// 0x000
#define FIFOSTART	(ACDKEY | 1)		// 0x001
#define FIFOHALT	(ACDKEY | 2)		// 0x002
#define WAIT4XSYNC	(ACDKEY | 3)		// 0x003
#define WAIT4ISYNC	(ACDKEY | 4)		// 0x004
#define WAIT4STOP       (ACDKEY | 5)		// 0x005

#define DLOAD           (ACDKEY | 6)		// 0x006
#define TLOAD           (ACDKEY | 7)		// 0x007
#define EXECUTEPATTERN  (ACDKEY | 8)		// 0x008
#define MULTIPATTERN    (ACDKEY | 9)		// 0x009
#define PATTERNDEF      (ACDKEY | 10)		// 0x00d

#define TABLEDEF        (ACDKEY | 11)		// 0x00b
#define SAFESTATE       (ACDKEY | 12)		// 0x00c
#define SOFTDELAY       (ACDKEY | 13)		// 0x00d
#define BIGDELAY        (ACDKEY | 14)		// 0x00e

#define EXECRTPATTERN   (ACDKEY | 15)           // 0x00f

#define FLUSHFFBUF	(ACDKEY | 16)		// 0x010

/* real time variable group with sub code definitions */
#define RTOP		(ACDKEY | 100)		// 0x064
#define	   CLR			(1)
#define	   INC			(2)
#define    DEC			(3)
#define RT2OP		(ACDKEY | 101)		// 0x065
#define	   SET			(20)
#define	   MOD2			(21)
#define	   MOD4			(22)
#define	   HLV			(23)
#define	   DBL			(24)
#define	   NOT			(25)
#define	   NEG			(26)
#define RT3OP		(ACDKEY | 102)		// 0x066
#define	   ADD			(30)
#define	   SUB			(31)
#define	   MUL			(32)
#define	   DIV			(33)
#define	   MOD			(34)
#define	   OR			(35)
#define	   AND			(36)
#define	   XOR			(37)
#define	   LSL			(38)
#define	   LSR			(39)
#define	   LT           	(40)
#ifdef     GT
#undef     GT
#endif
#define	   GT           	(41)
#define	   GE           	(42)
#define	   LE           	(43)
#define	   EQ           	(44)
#define	   NE           	(45)

#define SYNC_XGATE_COUNT	(ACDKEY | 51)

#define RTINIT                  (ACDKEY | 103)	// 0x067
#define FRTINIT                 (ACDKEY | 104)	// 0x068

#define RTASSERT		(ACDKEY | 105)	// 0x069
#define RTASSERTH		(ACDKEY | 106)	// 0x06a
#define RTASSERTL		(ACDKEY | 107)	// 0x06b
#define LRTASSERT		(ACDKEY | 108)	// 0x06c

#define RTCLIP   		(ACDKEY | 109)	// 0x06d
#define RTCLIPH		        (ACDKEY | 110)	// 0x06e
#define RTCLIPL		        (ACDKEY | 111)	// 0x06f
#define LRTCLIP 		(ACDKEY | 112)	// 0x070

#define JUMP		(ACDKEY | 115)		// 0x073
#define JMP_LT		(ACDKEY | 116)		// 0x074
#define JMP_NE		(ACDKEY | 117)		// 0x075
#define VDELAY_LIST     (ACDKEY | 119)          // 0x077
#define VDELAY          (ACDKEY | 120)		// 0x078
/* these are unused?? */
#ifdef  BRANCH
#undef  BRANCH
#endif
#define BRANCH		(ACDKEY | 121)		// 0x079
#define BRA_EQ		(ACDKEY | 122)		// 0x07a
#define BRA_GT		(ACDKEY | 123)		// 0x07b
#define BRA_LT		(ACDKEY | 124)		// 0x07c
#define BRA_NE		(ACDKEY | 125)		// 0x07d
/* */
#define PAD_DELAY       (ACDKEY | 126)		// 0x07e
#ifdef  TASSIGN
#undef  TASSIGN
#endif
#define TASSIGN         (ACDKEY | 127)		// 0x07f
#define TPUT            (ACDKEY | 128)		// 0x080
#ifdef  TABLE
#undef  TABLE
#endif
#define TABLE           (ACDKEY | 129)		// 0x081

#define NVLOOP          (ACDKEY | 130)		// 0x082
#define ENDNVLOOP       (ACDKEY | 131)		// 0x083

#define VLOOPTICKS      (ACDKEY | 132)          // 0x084

#define IFZERO          (ACDKEY | 135)		// 0x087
#define ELSENZ          (ACDKEY | 136)		// 0x088
#define ENDIFZERO       (ACDKEY | 137)		// 0x089
#define IFMOD2ZERO      (ACDKEY | 138)		// 0x089
#define TCOUNT          (ACDKEY | 139)		// 0x089

/* control and DDR group */
#define INITSCAN        (ACDKEY | 200)		// 0x0c8
#define NEXTSCAN        (ACDKEY | 201)		// 0x0c9
#define ENDOFSCAN       (ACDKEY | 202)		// 0x0ca
#define NEXTCODESET     (ACDKEY | 203)		// 0x0cb
#define END_PARSE       (ACDKEY | 204)		// 0x0cc
#define SEND_ZERO_FID   (ACDKEY | 205)		// 0x0cc
/* usage of NOISE is TBD */
#ifdef  NOISE
#undef  NOISE
#endif
#define NOISE           (ACDKEY | 206)		// 0x0ce
#define IL_MODE		(ACDKEY | 207)		// 0x0cf
#define ILCJMP		(ACDKEY | 208)		// 0x0d0


/* DDR Related           */
#define INITDDR         (ACDKEY | 210)		// 0x0d2
#define SETACQ_DDR      (ACDKEY | 211)		// 0x0d3
#define NEXTSCANDDR     (ACDKEY | 212)		// 0x0d4
#define PINSYNC_DDR     (ACDKEY | 213)		// 0x0d5
#define RG_MOD_DDR      (ACDKEY | 214)		// 0x0d6
#define SETAVAR_DDR     (ACDKEY | 215)		// 0x0d6
#define SETPVAR_DDR     (ACDKEY | 216)		// 0x0d6

/* RF group */
#define VRFAMP          (ACDKEY | 300)      // 0x12c
#define VRFAMPS         (ACDKEY | 301)      // 0x12d
#define VPHASE          (ACDKEY | 302)      // 0x12e
#define VPHASEC         (ACDKEY | 303)      // 0x12f
#define VPHASEQ         (ACDKEY | 304)      // 0x130
#define VPHASECQ        (ACDKEY | 305)      // 0x131
#define DECPROGON       (ACDKEY | 306)      // 0x132
#define DECPROGOFF      (ACDKEY | 307)      // 0x133
#define TINFO           (ACDKEY | 308)      // 0x134
#define PNEUTEST        (ACDKEY | 309)      // 0x135
#define MASTER_CHECK    (ACDKEY | 310)      // 0x136
#define RFSPARELINE     (ACDKEY | 311)      // 0x137
#define RFOPMODES       (ACDKEY | 312)      // 0x138
#define RTFREQ          (ACDKEY | 313)      // 0x139
#define AMPTBLS         (ACDKEY | 314)      // 0x13a
#define TEMPCOMP        (ACDKEY | 315)      // 0x13b
#define ADVISE_FREQ     (ACDKEY | 316)      // 0x13c
#define FORTH           (ACDKEY | 317)      // 0x13d
#define SELECTRFAMP     (ACDKEY | 318)      // 0x13e

/* RF decouping flags / policy */
#define DECCONTINUE     (1)
#define DECSTOPEND      (2)
#define DECZEROTIME     (3)
#define DECRESERVED     (12)
#define DECLAST         (16)
#define DECFRYM         (32)

/* PFG GROUP */
#define PFGVGRADIENT    (ACDKEY | 340)		// 0x154
#define PFGENABLE       (ACDKEY | 341)		// 0x155
#define PFGSETZLVL      (ACDKEY | 342)		// 0x156
#define PFGSETZCORR     (ACDKEY | 343)		// 0x157

/* GRADIENT group */
#ifdef  VGRADIENT
#undef  VGRADIENT
#endif
#define VGRADIENT       (ACDKEY | 351)		// 0x15f
#define ECC_TIMES	(ACDKEY | 360)		// 0x168
#define ECC_AMPS	(ACDKEY | 361)		// 0x169
#define SDAC_VALUES	(ACDKEY | 362)		// 0x16a
#define DUTYCYCLE_VALUES (ACDKEY | 363)		// 0x16b
#define GRAD_DELAYS (ACDKEY | 364)		// 0x16c

#define OBLGRD          (ACDKEY | 365)          // 0x16d
#define OBLPEGRD        (ACDKEY | 366)          // 0x16e
#define OBLSHAPEDGRD    (ACDKEY | 367)          // 0x16f
#define OBLPESHAPEDGRD  (ACDKEY | 368)          // 0x170
#define SHAPEDGRD       (ACDKEY | 369)          // 0x171

#define SETGRDROTATION  (ACDKEY | 390)          // 0x186
#define SETVGRDROTATION (ACDKEY | 391)          // 0x187
#define SETVGRDANGLIST  (ACDKEY | 392)          // 0x188
#define EXEVGRDROTATION (ACDKEY | 393)          // 0x189
#define SETPEVALUE      (ACDKEY | 395)          // 0x18A


/* LOCK GROUP - goes to MASTER */  
/* MASTER SPECIFIC */

/* 0x190 -> dec 400 */
#define LOCKINFO        (ACDKEY | 400)		// 0x190
#define SETNSR          (ACDKEY | 401)		// 0x191
#ifdef  SMPL_HOLD
#undef  SMPL_HOLD
#endif
#define SMPL_HOLD	(ACDKEY | 402)		// 0x192
#define NSRVSET         (ACDKEY | 403)		// 0x193
#define TNLK		(ACDKEY | 404)		// 0x194

#define XGATE	        WAIT4XSYNC
#define ROTORSYNC_TRIG	(ACDKEY | 500)		// 0x1f4
#define READHSROTOR	(ACDKEY | 501)		// 0x1f5
#define MASTERLOCKSETTC (ACDKEY | 502)		// 0x1f6
#define MASTERLOCKHOLD  (ACDKEY | 503)		// 0x1f7
#define LOCKAUTO	(ACDKEY | 504)		// 0x1f8
#define SHIMAUTO	(ACDKEY | 505)		// 0x1f9

#ifdef  SETVT
#undef  SETVT
#endif
#define SETVT           (ACDKEY | 506)		// 0x1fa
#define WAIT4VT         (ACDKEY | 507)		// 0x1fb
#ifdef  SETSPIN
#undef  SETSPIN
#endif
#define SETSPIN         (ACDKEY | 508)		// 0x1fc
#ifdef  CHECKSPIN
#undef  CHECKSPIN
#endif
#define CHECKSPIN       (ACDKEY | 509)		// 0x1fd
#define LOCKCHECK	(ACDKEY | 510)		// 0x1fe
#define SETSHIMS        (ACDKEY | 511)		// 0x1ff
#define SYNCSETSHIMS    (ACDKEY | 512)		// 0x200
#ifdef  SSHA
#undef  SSHA
#endif
#define SSHA            (ACDKEY | 513)		// 0x201
#define RECEIVERGAIN    (ACDKEY | 514)		// 0x202
#ifdef  AUTOGAIN
#undef  AUTOGAIN
#endif
#define AUTOGAIN        (ACDKEY | 515)		// 0x203
#ifdef  GETSAMP
#undef  GETSAMP
#endif
#define GETSAMP         (ACDKEY | 516)		// 0x204
#define HOMOSPOIL       (ACDKEY | 517)		// 0x205
#define TUNEOPS         (ACDKEY | 518)		// 0x206
#define SENDSYNC        (ACDKEY | 519)		// 0x207
#define ROLLCALL        (ACDKEY | 520)		// 0x208
#ifdef  LOADSAMP
#undef  LOADSAMP
#endif
#define LOADSAMP        (ACDKEY | 521)		// 0x209
#define SETACQSTATE     (ACDKEY | 522)		// 0x20a

#define MRIUSERBYTE	(ACDKEY | 525)          // 0x20d
#define MRIUSERGATES	(ACDKEY | 526)          // 0x20e

#define OLDCODE         (ACDKEY | 599)		// 0x257

/* ---------- SETUP TYPES FROM PSG, or aliases of GO   -----------------*/
#define GO 0
#define SU 1
#define SHIM 2
#define LOCK 3
#define SPIN 4
#define CHANGE 5
#define SAMPLE 6

#define LATCHDELAY      50.0e-9
#define LATCHDELAYTICKS 4

struct char8
{
#ifdef LINUX
    unsigned char char7;
    unsigned char char6;
    unsigned char char5;
    unsigned char char4;
    unsigned char char3;
    unsigned char char2;
    unsigned char char1;
    unsigned char char0;
#else
    unsigned char char0;
    unsigned char char1;
    unsigned char char2;
    unsigned char char3;
    unsigned char char4;
    unsigned char char5;
    unsigned char char6;
    unsigned char char7;
#endif
};

struct word2
{
#ifdef LINUX
    unsigned int word1;
    unsigned int word0;
#else
    unsigned int word0;
    unsigned int word1;
#endif
};

typedef union 
{
    double        d;
    long long    ll;
    struct word2 w2;
    struct char8 c8;
} union64;

#endif
