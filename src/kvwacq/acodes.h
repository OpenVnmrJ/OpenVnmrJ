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
#ifndef INCacodesh
#define INCacodesh

/* 
**  PROTO TYPING ACODE DEFINITION FILE
*/
#define ACODE_BUFFER	(21839)
#define RTVAR_BUFFER	(0xaaaaaa)



/*
**	ACODES
 *
 *   defines here must match defines in convert.c, SCCS category PSG
*/
#define FIFOSTART	(1)
#define FIFOHALT	(2)
#define FIFOSTARTSYNC	(3)
#define FIFOWAIT4STOP	(4)
#define EVENT1_DELAY	(5)
#define EVENT2_DELAY	(6)
#define DELAY		(7)
#define FIFOHARDRESET	(8)
#define FIFOWAIT4STOP_2	(9)
#define CLRHSL		(10)
#define LOADHSL		(11)
#define MASKHSL		(12)
#define UNMASKHSL	(13)
#define LOADHSL_R	(14)
#define MASKHSL_R	(15)
#define UNMASKHSL_R	(16)
#define PAD_DELAY	(17)
#define SAFEHSL		(18)
#define VDELAY		(19)

#define TASSIGN		(20)
#define TSETPTR		(21)
#define JTSETPTR	(23)


/*** AP Bus Output Commands	***/
#define APBCOUT		(30)
#define TAPBCOUT	(31)
#define    APADDRMSK		0x0fff 
#define    APBYTF		0x1000
#define    APINCFLG		0x2000
#define    APCCNTMSK		0x0fff
#define    APCONTNUE		0x1000
#define APRTOUT		(32)
#define EXEAPREAD	(33)
#define GETAPREAD	(34)

#define JTAPBCOUT       (35)

/*** Acquires and HWlooping	***/
#define ACQUIRE		(40)
#define TACQUIRE	(41)
#define BEGINHWLOOP	(42)
#define ENDHWLOOP	(43)
#define RECEIVERCYCLE	(44)
#define ENABLEOVRLDERR	(45)
#define DISABLEOVRLDERR	(46)
#define ENABLEHSSTMOVRLDERR	(47)
#define DISABLEHSSTMOVRLDERR	(48)

/*** Phases	***/
#define SETPHASE90	(50)
#define SETPHASE90_R	(51)
#define PHASESTEP	(52)
#define SETPHASE	(53)
#define SETPHASE_R	(54)

/*** Gradients and Waveform Generator	***/
#define RTGRADIENT	(60)
#define VGRADIENT	(61)
#define    GRADDATASIZ 		0xc000	/* 2 bits to define data size	  */
					/* 0 = 16bit, 1= 20bit; 2,3 undef */
#define INCGRADIENT	(62)
#define WGINSTRUCTION	(63)
#define WGRTAMPLITUDE	(64)
#define WGVAMPLITUDE	(65)
#define WGIAMPLITUDE	(66)
#define WGLOAD		(67)

/*** MISC Hardware	***/
#define RECEIVERGAIN	(70)
#define GAINA           (71)
#define DECUP           (73)
#define AUTOGAIN        (76)
#define GETSAMP         (77)
#define LOADSAMP        (78)
#define LOADSHIM        (79)
#define HOMOSPOIL	(80)
#define SETTUNEFREQ	(81)
#define TUNESTART	(82)
#define CLEARSCANDATA	(83)
#define SETPRESIG	(84)
#define	   LARGE		1		/* For SETPRESIG acode	*/
#define    NORMAL		0		/* For SETPRESIG acode	*/
#define SETSHIMS	(85)
#define SAFETYCHECK	(86)			/* Human safety system */
#define SETSHIM_AP	(87)			/* Set a shim dac via MSR AP bus interface */

#define INIT_STM	(90)
#define INIT_FIFO	(91)
#define		FIFODEBUG_BIT		0x0001
#define INIT_ADC	(92)
#define		AUDIO_CHAN_SELECT_MASK	0x000f
#define		ENABLE_RCVR_OVERLOAD	0x0100
#define		ENABLE_ADC_OVERLOAD	0x0200
#define		ENABLE_CTC		0x0400
#define		ENABLE_DSP		0x0800
#define		OBS1_CHAN		0x0000
#define		OBS2_CHAN		0x0000
#define		LOCK_CHAN		0x0001
#define		TEST_CHAN		0x0003
#define INTERP_REV_CHK	(93)
#define INIT_HS_STM 	(94)		/* 5MHz DTM/ADC */

#define TSTACQUIRE	(112)
#define END_PARSE	(113)
#define APTABLE		(114)
#define APTASSIGN	(115)
#define INITDELAY	(116)
#define INCRDELAY	(117)
#define INCRDELAY_R	(118)
#define	OBSLOCK		(151)
#define	SET_GR_RELAY	(159)

#define RTOP		(200)
#define	   CLR			(1)
#define	   INC			(2)
#define    DEC			(3)
#define RT2OP		(201)
#define	   SET			(20)
#define	   MOD2			(21)
#define	   MOD4			(22)
#define	   HLV			(23)
#define	   DBL			(24)
#define	   NOT			(25)
#define RT3OP		(202)
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
#define RTINIT		(203)
#define RTERROR		(204)
#define BRANCH		(220)
#define BRA_EQ		(221)
#define BRA_GT		(222)
#define BRA_LT		(223)
#define BRA_NE		(224)
#define JUMP		(230)
#define JMP_LT		(231)
#define JMP_NE		(232)
#define JMP_MOD2	(233)

/*  Currently we have two Update Acodes, to be used with interactive
    acquisitions.  For lock display we use LOCK_UPDT; for all other
    types of acquisitions we use ACQI_UPDT.  Currently a HALT OP is
    stuffed in the FIFO for interactive lock; this HALT must not be
    done with other kinds of acquisitions.			*/

#define INITSCAN	(300)
#define NEXTSCAN	(301)
#define ENDOFSCAN	(302)
#define NEXTCODESET	(303)
#define ACQI_UPDT	(304)
#define LOCK_UPDT	(305)
#define SIGNAL_COMPLETION (306)
#define STATBLOCK_UPDT	(307)
#define SYNC_PARSER	(308)
#define NOISE_CMPLT	(309)
#define SET_ACQI_UPDT_CMPLT (310)
#define START_ACQI_UPDT	(311)
#define SET_DATA_OFFSETS (312)
#define SET_NUM_POINTS	(313)

#define LOCKPHASE_R	(401)
#define LOCKPOWER_R	(402)
#define LOCKGAIN_R	(403)
#define LOCKPHASE_I	(404)
#define LOCKPOWER_I	(405)
#define LOCKGAIN_I	(406)
#define LOCKSETTC	(407)
#define LOCKSETACQTC	(408)
#define LOCKTC		(409)
#define LOCKACQTC	(410)
#define LOCKSETAPREG	(411)

#define LOCKAUTO	(412)
#define SHIMAUTO	(413)

#define LOCKZ0_I	(414)
#define LOCKMODE_I	(415)

#define LOCKCHECK	(416)
#define SETLKDECPHAS90  (417)   /* 90 phase control for lock/decouper brd */
#define SETLKDECATTN    (418)   /* attenuation control for lock/decouper brd */
#define SETLKDEC_ONOFF  (419)   /* lock/decouper brd , Decoupler on/off */
#define LOCKFREQ_I	(420)
#define SAVELOCKPH	(421)

#define SET_LKFAST	(440)
#define SET_LKSLOW	(441)

#define EVENTN		(450)

#define SETVT		(500)
#define WAIT4VT		(501)
#define SETSPIN		(505)
#define WAIT4SPIN	(506)
#define CHECKSPIN	(507)

#define SOFTDELAY_I	(1001)

#define XGATE		(1100)
#define ROTORSYNC_TRIG	(1101)
#define READHSROTOR	(1102)


#endif
