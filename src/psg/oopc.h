/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------
| This header File contains the structures, defines, and message type 
| for all devices.
+----------------------------------------------------------------------*/

#if  !defined(SUN_HAL) && !defined(ACQ_HALstruc) && !defined(OOPC)
#define OOPC	/* define OOPC so other header may detect its usage */

typedef char c68char;
typedef short c68int;
typedef long c68long;

#endif

/* Object Oriented structures  */
typedef int Message;	/* device message type */

/* Object Message Parameter Structure */
typedef struct {
		int setwhat;
		int value;
		double DBvalue;
		char *valptr;
	       } Msg_Set_Param;

/* Object Message Result Parameter Structure */
typedef struct {
		int resultcode;
		int reqvalue;
		int genfifowrds;
		double DBreqvalue;
		char *reqptr;
	       } Msg_Set_Result;

#ifndef OBJECTDEFINED
#define OBJECTDEFINED
typedef int (*Functionp)();

/* Object Handle Structure */
typedef struct {Functionp dispatch; char *objname; } *Object;
#endif

/* Structure for handing back a Handle to a Newly Created Object */
typedef struct {Object object;} Msg_New_Result;

/* Structure for Event RT Delay object control */
typedef struct {
		double incrtime; /* incremental delay */
		int rtevnt_type; /* RT Event Type */
		int timerbase;   /* rt val or abs. time base */
		int timercnt;    /* rt val or abs. time count */
		int hslines;	    /* rt val or abs. HS lines */
	       } RT_Event;

/* The Send macro for communicating to Objects */
#define Send(obj,msg,param,result) \
((*(obj->dispatch))(obj,msg,param,result))

#define ZERO 0
#define ONE 1

/* value type of attributes */
#define UNKNOWN_TYPE -1
#define NO_VALUE 0
#define SHORT 1
#define INTEGER 2
#define LONG 3
#define DOUBLE 4
#define POINTER 5

/* Object Oriented, Device Message defines  */
#define   MSG_NEW_DEV_r		10
#define   MSG_SET_DEV_ATTR_pr	11
#define   MSG_GET_DEV_ATTR_pr	12
#define   MSG_SET_AP_ATTR_pr	13
#define   MSG_GET_AP_ATTR_pr	14
#define   MSG_SET_ATTN_ATTR_pr	15
#define   MSG_GET_ATTN_ATTR_pr	16
#define   MSG_SET_ATTN_VAL_pr	17
#define   MSG_GET_ATTN_VAL_pr	18
#define   MSG_SET_APBIT_ATTR_pr	19
#define   MSG_GET_APBIT_ATTR_pr	20
#define   MSG_SET_APBIT_MASK_pr	21
#define   MSG_GET_APBIT_MASK_pr	22
#define   MSG_SET_EVENT_ATTR_pr	23
#define   MSG_GET_EVENT_ATTR_pr	24
#define   MSG_SET_EVENT_VAL_pr	25
#define   MSG_GET_EVENT_VAL_pr	26
#define   MSG_SET_FREQ_ATTR_pr	27
#define   MSG_GET_FREQ_ATTR_pr	28
#define   MSG_SET_FREQ_VAL_pr	29
#define   MSG_GET_FREQ_VAL_pr	30
#define   MSG_SET_RFCHAN_ATTR_pr	31
#define   MSG_GET_RFCHAN_ATTR_pr	32
#define   MSG_SET_RFCHAN_VAL_pr		33
#define   MSG_GET_RFCHAN_VAL_pr		34

/* Any additions to these defines should be included in objerror.h */
/* and must also be defined in attr_valtype.c */

/* Message Set Commands */
#define SET_ALL		0
#define SET_DEVSTATE	1
#define SET_DEVCNTRL	2
#define SET_DEVCHANNEL	3
#define SET_APADR	4
#define SET_APREG	5
#define SET_APBYTES	6
#define SET_APMODE	7
#define SET_MAXVAL	8
#define SET_MINVAL	9
#define SET_OFFSET	10
#define SET_VALUE	11
#define SET_DEFAULTS	12
#define SET_BIT		13
#define CLEAR_BIT	14
#define SET_TRUE_EQ_ONE 15
#define SET_TRUE_EQ_ZERO 16
#define SET_TRUE        17
#define SET_FALSE       18
#define SET_RTPARAM     19
#define SET_TYPE        20
#define SET_MAXRTPARS   21
#define SET_VALID_RTPAR 22
#define SET_DBVALUE     23

/* Frequency Obj */
#define SET_H1FREQ	24
#define SET_PTSVALUE    25
#define SET_IFFREQ    	26
#define SET_OVERRANGE	27
#define SET_INIT_OFFSET 28
#define SET_RFTYPE      29
#define SET_RFBAND      30
#define SET_FREQSTEP    31
#define SET_OSYNBFRQ    32
#define SET_OSYNCFRQ    33
#define SET_SPECFREQ    34
#define SET_OFFSETFREQ  35
#define SET_PTSOPTIONS  36

/* Channel Obj */
#define SET_FREQVALUE	37
#define SET_RTPWR	38
#define SET_RTPWRF	39
#define SET_PWR		40
#define SET_PWRF	41
#define SET_RTPHASE90	42
#define SET_RTPHASE	43
#define SET_PHASE90	44
#define SET_PHASE	45
#define SET_PHLINE0	46
#define SET_PHLINE90	47
#define SET_PHLINE180	48
#define SET_PHLINE270	49
#define SET_PHASEBITS	50
#define SET_PHASEAPADR	51
#define SET_PHASESTEP	52
#define SET_PHASEATTR	53
#define SET_HSLADDR	54
#define SET_XMTRGATEBIT	55
#define SET_XMTRGATE	56
#define SET_XMTRTYPE	57
#define SET_AMPTYPE	58
#define SET_FREQOBJ	59
#define SET_ATTNOBJ	60
#define SET_LNATTNOBJ	61
#define SET_APOVRADR	62
#define SET_XMTR2AMPBDBIT	63
#define SET_AMPHIMIN	64
#define SET_XMTRX2BIT	65
#define SET_HLBRELAYOBJ	66
#define SET_XMTRX2OBJ	67
#define SET_WGGATEBIT	68
#define SET_WGGATE	69

#define SET_WGAPBASE	70
#define SET_WGCMD	71
#define SET_WGIBPTR	72
#define SET_OVRUNDRFLG	73

/*  Not Used number 74 */

/* Freq Sweep  */
#define SET_SWEEPCENTER 75
#define SET_SWEEPWIDTH  76
#define SET_SWEEPNP     77
#define SET_SWEEPMODE   78
#define SET_INITSWEEP   79
#define SET_INCRSWEEP   80

/* RT Incremental Delay */
#define SET_INITINCR    81
#define SET_ABSINCR     82
#define SET_RTINCR      83

/* new definitions for Hydra */
#define SET_MOD_OBJ	84
#define SET_MOD_MODE	85
#define SET_MASK	86
#define SET_PHASE_REG	87
#define SET_PHASE_MODE	88
#define	SET_MOD_VALUE	89
#define SET_MIXER_OBJ	90
#define SET_MIXER_VALUE	91
#define SET_HS_OBJ	92
#define SET_HS_VALUE	93
#define SET_XGMODE_OBJ	94
#define SET_XGMODE_VALUE 95
#define SET_GATE_MODE	96
#define SET_RCVRGATEBIT	97
#define SET_RCVRGATE	98
#define SET_TUNE_FREQ	99
#define SET_GATE_PHASE	999

/* Message Get Commands */
#define GET_ALL		100
#define GET_STATE	101
#define GET_CNTRL	102
#define GET_CHANNEL	103
#define GET_ADR		104
#define GET_REG		105
#define GET_BYTES	106
#define GET_MODE	107
#define GET_MAXVAL	108
#define GET_MINVAL	109
#define GET_OFFSET	110
#define GET_VALUE	111
#define GET_RTVALUE	112
#define GET_TYPE        113
#define GET_MAXRTPARS   114
#define GET_VALID_RTPAR 115

#define GET_OF_FREQ 	116
#define GET_PTS_FREQ	117
#define GET_SPEC_FREQ 	118
#define GET_RFBAND 	119
#define GET_BASE_FREQ	120


#define GET_XMTRTYPE 	121
#define GET_AMPTYPE 	122
#define GET_HIBANDMASK 	123
#define GET_AMPHIBAND 	124
#define GET_PHASEBITS 	125

#define GET_IB_RESOLVE 	126
#define GET_PULSE_IB 	127
#define GET_DEC_IB 	128
#define GET_GRAD_IB 	129

#define GET_PREAMP_SELECT       130

/*  Not Used number 131 */
 
#define GET_FREQSTEP    132

#define GET_SWEEPCENTER 133
#define GET_SWEEPWIDTH  134
#define GET_SWEEPNP     135
#define GET_SWEEPINCR   136
#define GET_SWEEPMODE   137
#define GET_SWEEPMAXWIDTH   138

#define GET_LBANDMAX	139
#define GET_MAXXMTRFREQ	140
#define GET_XMTRGATE    141
#define GET_XMTRGATEBIT	142
#define GET_PWR		143

#define GET_SIZETUNE	144

#define SET_HSSEL_FALSE	145
#define SET_HSSEL_TRUE	146

#define GET_PTSOPTIONS	147
#define GET_IFFREQ	148
#define GET_PTSVALUE	149
#define GET_RFTYPE	150
#define GET_OSYNBFRQ	151
#define GET_OSYNCFRQ	152
#define GET_PWRF	153
/* SISCO */
#define SET_FREQLIST	154
#define SET_GTAB	155

/* EXTRA */
#define SET_OFFSETFREQ_STORE	156
#define SET_FREQ_FROMSTORAGE	157


/* errors */
#define   UNKNOWN_MSG		-1
#define   NOT_PRESENT		-2
#define   UNKNOWN_DEV_ATTR	-3
#define   UNKNOWN_AP_ATTR	-4
#define   UNKNOWN_ATTN_ATTR	-5
#define   UNKNOWN_APBIT_ATTR	-6
#define   NO_MEMORY		-7
#define   ILLEGAL_APBIT_BIT_ATTR -8
#define	  UNKNOWN_EVENT_ATTR	-9
#define   UNKNOWN_EVENT_TYPE    -10
#define   UNKNOWN_EVENT_TYPE_ATTR -11
#define	  NO_RTPAR_MEMORY 	-12
#define	  PAST_MAX_RTPARS 	-13
#define   UNKNOWN_FREQ_ATTR	-14
#define   UNKNOWN_RFCHAN_ATTR	-15
#define   ERROR_ABORT		-16

/* Device States */
#define OK 		0
#define INACTIVE	0
#define ACTIVE		1

#define OFF 		0
#define ON		1

/* Device Channels */
#define OBSERVE		1
#define DECOUPLER	2
#define DECOUPLER2	3
#define DECOUPLER3	4

/* Device Control Types */
#define VALUEONLY	1
#define BITMASK		2

/* OBSERVE Attenuator defaults */
#define	OBS_ATTN_AP_ADR		5
#define	OBS_ATTN_AP_REG		12
#define OBS_ATTN_AP_REG500	0
#define	OBS_ATTN_BYTES		1
#define	OBS_ATTN_MODE		-1
#define	OBS_ATTN_MAXVAL		63	/* max attn value of 63 db */
#define	OBS_ATTN_MINVAL		0	/* min attn value of  0 db */
#define	OBS_ATTN_OFSET		63	/* OffSet to Zero */
#define	OBS_ATTN_OFSET500	0/*500 style work in reverse, min attn value of 63db */
#define SIS_UNITY_ATTN_MAX	127	/* max value 63.5dB in 0.5dB steps */

/* DECOUPLER Attenuator defaults */
#define	DEC_ATTN_AP_ADR		OBS_ATTN_AP_ADR
#define	DEC_ATTN_AP_REG		16
#define DEC_ATTN_AP_REG500	4
#define	DEC_ATTN_BYTES		OBS_ATTN_BYTES
#define	DEC_ATTN_MODE         	OBS_ATTN_MODE
#define	DEC_ATTN_MAXVAL         OBS_ATTN_MAXVAL
#define	DEC_ATTN_MINVAL         OBS_ATTN_MINVAL
#define	DEC_ATTN_OFSET          OBS_ATTN_OFSET
#define	DEC_ATTN_OFSET500	0/*500 style work in reverse */

/* 2nd DECOUPLER Attenuator defaults */
#define	DEC2_ATTN_AP_ADR	5
#define	DEC2_ATTN_AP_REG	15
#define	DEC2_ATTN_BYTES		OBS_ATTN_BYTES
#define	DEC2_ATTN_MODE         	OBS_ATTN_MODE
#define	DEC2_ATTN_MAXVAL        OBS_ATTN_MAXVAL
#define	DEC2_ATTN_MINVAL        OBS_ATTN_MINVAL
#define	DEC2_ATTN_OFSET         OBS_ATTN_OFSET

/* 3rd DECOUPLER Attenuator defaults */
#define	DEC3_ATTN_BYTES		OBS_ATTN_BYTES
#define	DEC3_ATTN_MAXVAL        OBS_ATTN_MAXVAL
#define	DEC3_ATTN_MINVAL        OBS_ATTN_MINVAL
#define	DEC3_ATTN_OFSET         OBS_ATTN_OFSET

/* LOCK Attenuator defaults */
#define	LK_ATTN_AP_ADR		5
#define	LK_ATTN_AP_REG		18
#define	LK_ATTN_BYTES		1
#define	LK_ATTN_MODE		-1
#define	LK_ATTN_MAXVAL		63/* max attn value of 63db */
#define	LK_ATTN_MINVAL		0/* min attn value of 0db */
#define	LK_ATTN_OFSET		0/* Zero */

/* RF Relay defaults */
#define	RF_RELAY_AP_ADR		5
#define	RF_RELAY_AP_REG		14
#define	RF_RELAY_BYTES		1
#define	RF_RELAY_MODE		0

#define	RF_OPTS_AP_REG		17
#define	RF_OPTS2_AP_REG		13

#define APSELECT 0xA000         /* select apchip register */
#define APWRITE  0xB000         /* write to apchip reister */
#define APPREINCWR 0x9000       /* preincrement register and write */

/* RF 1 Routing Relay Bits */  /* reg 14, 0x0E */
#define DEC_RELAY5	0	/* Dec select for 500 & 600 */
#define AMP_RELAY	1	/* Amp Output routine */
#define DEC2_RELAY	2	/* 2nd Decoupler Channel routing */
#define OBS_RELAY	3	/* Obs Channel routing */
#define DEC_RELAY	4	/* Dec Channel routing */
#define HPAMPHB_RELAY	5	/* High Power Amp High Band */
#define HPAMPLB_RELAY	6	/* High Power Amp Low Band */
#define TUNE_RELAY	7	/* Tuning Relay */

/* delete these later */
/* #define RFSPARE	2 */

/* RF Amp Setting Bits */  /* reg 17, 0x11 */
#define DEC_XMTR_HI_LOW_BAND	0	/* Dec Transmitter Hi/Lo band select */
#define HOMO2			1	/* Obs & Dec freq within 1MHz */
#define OBS_XMTR_HI_LOW_BAND 	2	/* Obs Transmitter Hi/Lo band select */
#define LO_RELAY		3	/* LO freq relay */
#define LOWBAND2_CW		4	/* 1/3/90 spare bit, was TIME_SHARE_MOD */
#define DEC2_XMTR_HI_LOW_BAND	5	/* 2nd Dec Transmitter Hi/Lo band select */
#define MODB			6	/* Dec modulation mode, was a highspeed line */
#define MODA			7	/* Dec modulation mode, was a highspeed line */

/* RF 2 Opts Bits */	/* reg 13, 0x0D */
#define HIGHBAND_CW	0	/* CW */
#define LOWBAND_CW	1	/* CW */
#define PREAMP_GAIN_SELECT	2	/* preamp gain select, for 500 & 600 */
#define TR_SWITCH	3	/* T/R switch to preamp */
#define MIXER_SELECT	4	/* preamp mixer select */
#define PREAMP_SELECT	5	/* preamp select, for 500 & 600 */
#define HIGHBAND2_CW	6	/* second Amp High band CW mode */
#define LEG_RELAY	7	/* Magnet Leg RF routing */

/* delete these later */
/*#define RFSPARE2	6
*/

/* Event defaults */
#define EVENT_HS_ROTOR      1
#define EVENT_EXT_TRIGGER   2
#define EVENT_RT_DELAY      3

/* Linear Attenuator defaults */
#define	LN_ATTN_AP_ADR		5
#define	DC_LN_ATTN_AP_REG	20 /* two byte device uses 20 + 21 */
#define	TX_LN_ATTN_AP_REG	22 /* two byte device uses 22 + 23 */
#define	LN_ATTN_BYTES		2
#define	LN_ATTN_MODE		1
#define	LN_ATTN_MAXVAL		4095/* max attn value of 63db */
#define	LN_ATTN_MINVAL		0/* min attn value of 0db */
#define	LN_ATTN_OFFSET		0/* Zero */

/* Frequncy Object RF Types */
#define	DIRECTSYN		1
#define	OFFSETSYN		2
#define	FIXED_OFFSET		3
#define	IMAGE_OFFSETSYN		4
#define DIRECT_NON_DBL		5
#define LOCK_DECOUP_OFFSETSYN	6   /* lock/decoupler board 2/20/97 */

/* rfband */
#define RF_LOW_BAND		1
#define RF_HIGH_BAND		2
#define RF_BAND_AUTOSELECT	3

/* PTS Option BIT assignments  */
#define USE_SETPTS	0	/* Use SETPTS acode else use AP words directly */
#define LATCH_PTS	1	/* Use the Latching capability of PTS */
#define OVR_UNDR_RANGE	2	/* Use the Over/Under Range capability of PTS */
#define SIS_PTS_OFFSETSYN 3	/* SIS RF Modulator - use PTS offsetsyn */

/* Small Angle Phaseshift AP Address */
#define OBS_SMPHASE_APADR 6
#define DEC_SMPHASE_APADR 9
#define DEC2_SMPHASE_APADR 10

#define	MAGLEG_HILO		0
#define	MAGLEG_MIXER		1
#define	MAGLEG_RELAY_0		2
#define	MAGLEG_RELAY_1		3
#define MAGLEG_RELAY_2		4
#define MAGLEG_ARRAY_MODE	5
#define MAGLEG_PREAMP_SEL_0	6
#define MAGLEG_PREAMP_SEL_1	7
#define MAGLEG_SPARE1		5 /* Temporary aliases for above 3 values */
#define MAGLEG_SPARE2		6
#define MAGLEG_SPARE3		7

#define MAGLEG_APADDR		0x0b
#define AMP1_2_APADDR		0x0b
#define AMP3_4_APADDR		0x0b
#define AMPSOL_APADDR		0x0b
#define AMP_AP_APADDR		0x0b
#define MAGLEG_APREG		0x49
#define AMP1_2_APREG		0x34
#define AMP3_4_APREG		0x35
#define AMPSOL_APREG		0x4d
#define AMP_AP_APREG		0x3e

#define PIC_AP_APREG		0x4e	/* 4 channel magnet leg (PIC) register, 9/5/02 */
#define PIC_AP_APREG		0x4e	/* 4 channel magnet leg (PIC) register, 9/5/02 */
#define PIC_MIX1SEL		0	/* = ! MAGLEG_HILO */
#define PIC_MIX2SEL		1       /* = ! MAGLEG_HILO */
#define PIC_PSEL0		2	/* = (MAGLEG_HILO && ! MAGLEG_MIXER) || Tune */
#define PIC_PSEL1		3	/* = (MAGLEG_MIXER) || Tune */
#define PIC_IN2SEL0		4	/* = (MAGLEG_HILO && ! MAGLEG_MIXER) || Tune */
#define PIC_IN2SEL1		5	/* = (MAGLEG_MIXER) || Tune */
#define PIC_ARRAY		6	/* = mrarray='y' */
#define PIC_DELAY		7	/* = highq='y' */



#define AMP1_ENABLE		0
#define AMP1_SELECT1		1
#define AMP1_SELECT2		2
#define AMP1_SELECT3		3
#define AMP1_APGATE		4
#define AMP1_CW_ON		5
#define	AMP1_HILO_PREAMP	6
#define AMP1_PAD1		7

#define AMP2_ENABLE		0
#define AMP2_SELECT1		1
#define AMP2_SELECT2		2
#define AMP2_SELECT3		3
#define AMP2_APGATE		4
#define AMP2_CW_ON		5
#define	AMP2_PAD2		6
#define AMP2_PAD3		7

#define AMP3_ENABLE		0
#define AMP3_SELECT1		1
#define AMP3_SELECT2		2
#define AMP3_SELECT3		3
#define AMP3_APGATE		4
#define AMP3_CW_ON		5
#define	AMP3_NC6		6
#define AMP3_NC7		7

#define HILO_APADDR		0x0b
#define HILO_APREG		0x36
#define HILO_CH1		0
#define HILO_CH2		1
#define HILO_CH3		2
#define	HILO_CH4		3
#define	HILO_AMT2_HL		4
#define	HILO_AMT3_HL		5
#define	HILO_AMT2_LL		6
#define	HILO_NC			7

#define HS_SEL0			0
#define HS_SEL1			1
#define HS_SEL2			2
#define HS_SEL3			3
#define HS_SEL4			4
#define HS_SEL5			5
#define HS_SEL6			6
#define HS_SEL7			7

/* BreakOut Board for inova */
#define BOB_APADDR		0x0d
#define BOB_REG			0x10
