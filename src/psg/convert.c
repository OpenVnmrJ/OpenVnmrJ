/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "acodes.h"
#include "acqparms.h"
#include "aptable.h"
#include "group.h"
#include "expDoneCodes.h"
#include "REV_NUMS.h"
#include "dsp.h"
#include "lc_index.h"

extern double sign_add();

extern int gen_apbcout();

extern int debug2;
extern int acqiflag;
extern int rtinit_count;
extern int      ap_interface;
extern codeint  dpfrt, arraydimrt, relaxdelayrt, nfidbuf, rtcpflag;
extern codeint	rtrecgain, npnoise, dtmcntrl, gindex, ctss, adccntrl;
extern codeint	ilflagrt, tmprt, acqiflagrt, maxsum, incrdelay, endincrdelay;
extern codeint  ntrt,npr_ptr,bsctr,strt, activercvrs, clrbsflag;
/*extern int  dsp_info[];*/
extern int  noise_dwell;
extern int loc;		/* sample to change */
extern double psync;
extern double ldshimdelay,oneshimdelay;
extern int HS_Dtm_Adc;		/* Flag to convert.c to select INOVA 5MHz High Speed DTM/ADC */
extern int rcvr2nt;

extern int traymax;	/* used to determine in GETSAMP & PUTSAMP if sample detection is skipped */
extern int prepScan;	/* used to decide if Acode sync is to be used */
			/* nonprobe changer 48, gilson 96 */

/* defines from macros.h */
#define NSEC 1
#define USEC 2
#define MSEC 3
#define SECONDS  4	/* Changed from SEC to SECONDS due to macro 	*/
			/* redefinition warnings.			*/


#define	APBCOUT		30
#define TAPBCOUT        31
#define APRTOUT		32
#define EXEAPREAD	33
#define GETAPREAD	34
#define APTABLE         114
#define APTASSIGN       115
#define NEW_INITDELAY	116
#define NEW_INCRDELAY	117
#define INCRDELAY_R	118
#define	FIFOSTART	1
#define	FIFOHALT	2
#define FIFOWAIT4STOP   4
#define TDELAY		7
#define FIFOHARDRESET   8
#define FIFOWAIT4STOP_2	9
#define	LOADHSL		11
#define MASKHSL         12
#define UNMASKHSL       13
#define LOADHSL_R	14
#define MASKHSL_R	15
#define UNMASKHSL_R	16
#define PAD_DELAY	17
#define SAFEHSL		18
#define VDELAY		19
#define	EVENT1_DELAY	5
#define	EVENT2_DELAY	6
#define	ACQUIRE		40
#define BEGINHWLOOP	42
#define ENDHWLOOP	43
#define RECEIVERCYCLE	44
#define ENABLEOVRLDERR  (45)
#define DISABLEOVRLDERR (46)
#define ENABLEHSSTMOVRLDERR     (47)
#define DISABLEHSSTMOVRLDERR    (48)
#define SETSHIMS (85)
#define SETSHIM_AP      (87)                    /* Set a shim dac via MSR AP bus interface */
#define FIFOSTARTSYNC	3

#define	RTOP		200
#define	INC		2
#define	DEC		3
#define	RT2OP		201
#define	SET		20
#define	MOD2		21
#define	MOD4		22
#define	HLV		23
#define	DBL		24
#define NOT		25
#define	RT3OP		202
#define	ADD		30
#define	SUB		31
#define	MUL		32
#define DIV		33
#define	MOD		34
#define OR              35
#define	AND		36
#define	XOR		37
#define	LSL		38
#define	LSR		39
#define LOCKSEQUENCE	213

#define BRA_EQ		(221)
#define BRA_GT		(222)
#define BRA_LT		(223)
#define BRA_NE		(224)
#define JUMP		230
#define JMP_LT		231
#define JMP_NEQ		232
#define JMP_MOD2	233
#define INIT_STM	90
#define INIT_FIFO 	91
#define INTERP_REV_CHK	93
#define INIT_HS_STM	94
#define RTINIT		203
#define RTERROR		204
#define	INITSCAN	300
#define NEXTSCAN	301
#define ENDOFSCAN	302
#define NEXTCODESET	303
#define ACQI_UPDT       (304)
#define SET_ACQI_UPDT_CMPLT       (310)
#define START_ACQI_UPDT       (311)
#define SET_DATA_OFFSETS (312)
#define SET_NUM_POINTS	(313)
#define SIGNAL_COMPLETION (306)
#define STATBLOCK_UPDT (307)
#define TASSIGNX	20
#define TSETPTR         21
#define APINCFLG	0x2000
#define ENABLE_CTC 	0x0400
#define LOCKPHASE_I     (404)
#define LOCKPOWER_I     (405)
#define LOCKGAIN_I      (406)
#define LOCKSETTC       (407)
#define LOCKACQTC       (410)
#define LOCKAUTO        (412)
#define SHIMAUTO        (413)
#define LOCKZ0_I	(414)
#define LOCKMODE_I	(415)
#define		LKHOLD		12
#define		LKSAMPLE	13
#define SETLKDECPHAS90  (417)	/* 90 phase control for lock/decouper brd */
#define SETLKDECATTN    (418)	/* attenuation control for lock/decouper brd */
#define SETLKDEC_ONOFF  (419)	/* lock/decouper brd , Decoupler on/off */
#define LOCKFREQ_I	(420)
#define SSHA_INOVA	(421)
#define SETPHASE90	(50)
#define SETPHASE90_R	(51)
#define NEW_PHASESTEP	(52)	/* New Acode define is same as old */
#define NEW_SETPHASE	(53)	/* New Acode define is same as old */
#define SETPHASE_R	(54)
#define RECEIVERGAIN    (70)
#define HOMOSPOIL       (80)
#define SETTUNEFREQ     (81)
#define TUNESTART	(82)
#define CLEARSCANDATA	(83)
#define SETPRESIG	(84)
#define NEW_VGRADIENT       (61)
#define INCGRADIENT     (62)
#define WGLOAD          (67)
#define SAFETYCHECK	(86)

#define INIT_ADC        (92)


#define OBSOLETE  888
#define TODO	  999

#define SYNC_PARSER     (308)
#define NOISE_CMPLT     (309)
#define LOCKCHECK  	(416)
#define SETSPN  	(505)
#define CHECKSPN  	(507)
#define INOVAXGATE 	(1100)
#define INOVAROTORSYNC 	(1101)
#define RDHSROTOR	(1102)

#define	Z0	 1

#define STD_APBUS_DELAY  32			/* 400 nanosecs */
#define READ_APBUS_DELAY 40			/* 500 nanosecs */
#define PFG_APBUS_DELAY  72			/* 900 nanosecs */
#define PFGL200_APBUS_DELAY  232		/* 2.900 usecs */

typedef struct _trBlk {
	      codeint   oldCode;
	      codeint   oldLen;
              int       (*func)();
	      codeint   newCode;
            } trBlk;

typedef struct _apTbl {
		codeint oldacodeloc;
		codeint newacodeloc;
		codeint auto_inc;
	} apTbl;

typedef struct _phasetbl {
		int	phase90[4];
		int	phasebits;
		int	phasequad;
		int	phasestep;
		int	phaseprecision;
		int	apaddr;
		int	apdelay;
	} phasetbl;

#define MAXTABLES	20
#define MAXAPTABLES	80

typedef struct _tblhdr {
		int	num_entries;
		int	size_entry;
		int	mod_factor;
	} tblhdr;

/* Table information */
int num_tables = 0;
static int *tblptr[MAXTABLES];
static int num_aptables = 0;
static apTbl *apTable[MAXAPTABLES];

static int createtable(int size);
static int *createphasetable(codeint channeldev);

int     createglobaltable();

int	n_begin(),n_setup();
int	n_apbout(), n_apcout(), n_tapbout(), n_iapread();
int	n_scopy(), n_scopy_r(), n_copy(), n_rtinit(), n_init();
int	n_rtop(), n_rt2op(), n_rt3op();
int	n_branch(), n_ifnz(), n_endif(), n_ifnzb();
int	n_ifminus(), n_endloop();
int	n_tassign(), n_initfreq();
int	n_padly();
int	n_todo();
int	n_lock(), n_lktc();
int	n_loadshim(), n_shim(), n_setshim();
int	n_hkeep(), n_nsc(), n_seticm(), n_cleardata();
int	n_todo_1(), n_todo_2(), n_todo_3();
int	n_todo_4(), n_todo_5();
int	n_nextcodeset(), n_branch_lt();
int	n_phasestep(), n_setphase(), n_setphattr(), n_setphase90();
int	n_hsline(), n_spare12();
int	n_table(), n_hwloop(), n_endhwloop(), n_gtabindx();
int	n_gain(), n_noise();
int	n_pvgradient(), n_wgd3(), n_wgv3(), n_wgi3(), n_incwgrad();
int	n_incpgrad(),n_tvgradient(),n_inctgrad();
int	n_wgcmd(), n_wg3(), n_wgiload();
int	n_vtset(), n_tunefreq();
int 	n_cpsamp();
int 	n_scopytst();
int	n_ipacode();
int     n_again();
int     n_fidcode();
int	n_initdelay(), n_incrdelay(), n_rt_event1();
int	n_smpl_hold();
int     n_rdrotor();
int     n_enableovrflow(),n_disableovrflow();
int	n_lock_seq();
/* int     n_setlkdecphase90(); */

static unsigned int	orgHSdata;
static int forceHSlines = 0;


trBlk  trTable[] = {
    { CBEGIN, 		1,	n_begin,	0 },
    { APBOUT, 		0, 	n_apbout,	APBCOUT },
    { STFIFO, 		1,	NULL,		OBSOLETE },
    { SFIFO, 		1,	NULL,		OBSOLETE },
    { HSLINES, 		3,	n_hsline,	LOADHSL },
    { ISAFEHSL,		3,	n_scopy,	SAFEHSL},
    { INITHSL,		3,	n_scopy,	LOADHSL},
    { EVENT1_TWRD, 	3,	n_scopy,	EVENT1_DELAY },
    { EVENT2_TWRD, 	5,	n_scopy,	EVENT2_DELAY },
    { EXACQT, 		6, 	n_scopy,	ACQUIRE },
    { INIT, 		1,	n_init,		INITSCAN },
    { INCRFUNC, 	2,	n_rtop,		INC },
    { DECRFUNC, 	2,	n_rtop,		DEC },
    { ASSIGNFUNC, 	3,	n_rt2op,	SET },
    { MOD2FUNC, 	3,	n_rt2op,	MOD2 },
    { MOD4FUNC, 	3,	n_rt2op,	MOD4 },
    { DBLFUNC, 		3,	n_rt2op,	DBL },
    { HLVFUNC, 		3,	n_rt2op,	HLV },
    { NOTFUNC, 		3,	n_rt2op,	NOT },
    { ADDFUNC, 		4,	n_rt3op,	ADD },
    { SUBFUNC, 		4,	n_rt3op,	SUB },
    { MULFUNC, 		4,	n_rt3op,	MUL },
    { DIVFUNC, 		4,	n_rt3op,	DIV },
    { MODFUNC, 		4,	n_rt3op,	MOD },
    { ANDFUNC, 		4,	n_rt3op,	AND },
    { XORFUNC, 		4,	n_rt3op,	XOR },
    { LSLFUNC, 		4,	n_rt3op,	LSL },
    { LSRFUNC, 		4,	n_rt3op,	LSR },
    { BRANCH, 		2,	n_branch,	JUMP },
    { IFNZFUNC, 	4,	n_ifnz,		JMP_NEQ },
    { IFMOD2ZERO, 	4,	n_ifnz,		JMP_MOD2 },
    { IFNZBFUNC, 	4,	n_ifnzb,	JMP_NEQ },
    { ENDIF, 		0,	n_endif,	0 },
    { IFMIFUNC, 	4,	n_ifminus,	JMP_LT },
    { ENDLOOP, 		4,	n_endloop,	JMP_LT },
    { INOVARTERROR,	3,	n_scopy,	RTERROR},
    { SETICM, 		1,	n_seticm, 	RECEIVERCYCLE },
    { HKEEP, 		1,	n_hkeep,	ENDOFSCAN },
    { NSC, 		1,	n_nsc,		NEXTSCAN },
    { TASSIGN, 		0,	n_tassign,	APTASSIGN },
    { NNOISE,   	3,	n_copy,		LOCKPOWER_I },
    { NACQXX,   	3,	n_copy,		LOCKPHASE_I },
    { NEXACQT,   	3,	n_copy,		LOCKGAIN_I },
    { XSAPBIO, 		4,	n_copy,		LOCKZ0_I },
    { LOADF, 		4,	n_rtinit,	RTINIT },
    { APCOUT, 		6,	n_apcout, 	APBCOUT },
    { TABLE, 		0,	n_table,	APTABLE },
    { CLEAR, 		1,	NULL,		OBSOLETE },
    { RFIFO, 		1,	NULL,		OBSOLETE },
    { WT4VT, 		3,	n_scopy,	501 },
    { CKVTR, 		2,	NULL,		OBSOLETE },
    { SETPHAS90, 	3,	n_setphase90,	SETPHASE90_R },
    { LOCKDEC_ON_OFF, 	4,	n_scopy,	SETLKDEC_ONOFF },
    { LOCKDEC_ATTN, 	4,	n_scopy,	SETLKDECATTN },
    { LOCKDECPHS90, 	3,	n_setphase90,	SETLKDECPHAS90 },
    { INITDELAY, 	7,	n_initdelay,	NEW_INITDELAY },
    { INCRDELAY, 	3,	n_incrdelay,	NEW_INCRDELAY },
    { ORFUNC, 		4,	n_rt3op,	OR },
    { NOISE, 		6,	n_noise,	ACQUIRE },
    { SETWL, 		3,	NULL,		OBSOLETE },
    { RT_EVENT1, 	5,	n_rt_event1,	VDELAY },
    { RD_HSROTOR,	2,	n_rdrotor,	RDHSROTOR },
    { ACQBITMASK,	2,	NULL,		OBSOLETE },
    { CHKHDWARE, 	5,	NULL,		OBSOLETE },
    { PFLCNT, 		1,	NULL,		OBSOLETE },
    { SETVT, 		6,	n_vtset,	500 },
    { SETPHASE, 	3,	n_setphase,	SETPHASE_R },
    { HWLOOP, 		4,	n_hwloop,	BEGINHWLOOP },
    { EHWLOOP, 		1,	n_endhwloop,	ENDHWLOOP },
    { PHASESTEP,  	3,	n_phasestep,	NEW_PHASESTEP },
    { CLEARDATA, 	1,	n_cleardata,	CLEARSCANDATA },
    { SPINA, 		4,	n_scopy,	SETSPN },
    { GAINA, 		1,	n_gain,		RECEIVERGAIN },
    { LOCKA, 		3,	n_lock,		LOCKAUTO },
    { CHKSPIN, 		4,	n_scopy,	CHECKSPN },
    { AUTOGAIN, 	1,	n_again,	AUTOGAIN },
    { GETSAMP, 		2,	n_cpsamp,	GETSAMP },
    { LOADSAMP, 	3,	n_cpsamp,	LOADSAMP },
    { GTABINDX, 	5,	n_gtabindx,	TSETPTR },
    { PADLY, 		0,	n_padly,	PAD_DELAY },
    { WGCMD, 		3,	n_wgcmd,	APBCOUT },
    { WG3, 		3,	n_wg3,		APBCOUT },
    { WGGLOAD, 		1,	n_scopy,	WGLOAD },
    { WGILOAD, 		6,	n_wgiload,	APBCOUT },
    { WGD3, 		5,	n_wgd3,		NEW_VGRADIENT },
    { PVGRADIENT, 	5,	n_pvgradient,	NEW_VGRADIENT },
    { TVGRADIENT, 	5,	n_tvgradient,	NEW_VGRADIENT },
    { WGV3, 		7,	n_wgv3,		NEW_VGRADIENT },
    { INCWGRAD, 	9,	n_incwgrad,	INCGRADIENT },
    { INCPGRAD, 	9,	n_incpgrad,	INCGRADIENT },
    { INCTGRAD, 	9,	n_inctgrad,	INCGRADIENT },
    { WGI3, 		11,	n_wgi3,		INCGRADIENT },
    { LKFILTER, 	3,	n_lktc,		LOCKSETTC },
    { SMPL_HOLD, 	2,	n_smpl_hold,	LOCKMODE_I },
    { SETSIGREG, 	2,	n_scopy,	SETPRESIG },
    { SETPHATTR,  	14, 	n_setphattr,	NEW_PHASESTEP },
    { SHIMA, 		0,	n_shim,		SHIMAUTO },
    { LOADSHIM, 	48,	n_loadshim,	LOADSHIM },
    /* { SETSHIM,		4,	n_setshim,	SETSHIMS }, */
    { SETSHIM,		3,	n_scopy,	SETSHIM_AP },
    { SAFETY_CHECK, 	2,	n_scopy,	SAFETYCHECK },
/*    { TUNE_FREQ, 	2,	n_todo_4,	0 }, */
    { TUNE_FREQ, 	2,	n_tunefreq,	SETTUNEFREQ },
    { TUNE_START,	3,	n_scopy,	TUNESTART },
    { INITFREQ,   	0,	n_initfreq,	0 },
    { NO_OP, 		1,	NULL,		OBSOLETE },
    { EXIT, 		1,	n_nextcodeset,	NEXTCODESET },
    { IHOMOSPOIL,	2,	n_scopy,	HOMOSPOIL},
    { HALT, 		1,	NULL,		0 },
    { EVENT, 		1,	NULL,		0 },
    { CKLOCK, 		2,	n_scopy,	LOCKCHECK },
    { ovprec, 		1,	NULL,		0 },
    { RAMODE, 		1,	NULL,		0 },
    { acq, 		1,	NULL,		0 },
    { PHASESHIFT, 	6,	NULL,		0 },
    { IFZFUNC, 		4,	n_branch_lt,	JMP_LT },
    { acqstart,   	1,	NULL,		0 },
    { acqend, 		1,	NULL,		0 },
    { EVENT1, 		3,	NULL,		0 },
    { EVENT2, 		4,	NULL,		0 },
    { INCRFREQ,   	3,	NULL,		0 },
    { CALL, 		2,	NULL,		0 },
    { SETPTS, 		8,	NULL,		0 },
    { SETPOWER, 	4,	NULL,		0 },
    { DECUP, 		3,	NULL,		0 },
    { GRADIENT, 	3,	NULL,		OBSOLETE },
    { VGRADIENT, 	5,	NULL,		OBSOLETE },
    { EVENTN, 		1,	NULL,		0 },
    { LDPATRAM, 	1,	NULL,		OBSOLETE },
    { RFSHPAMP, 	1,	NULL,		OBSOLETE },
    { SETSHPTR, 	3,	NULL,		OBSOLETE },
    { SETLOOPSIZE, 	2,	NULL,		OBSOLETE },
    { SETHKDELAY, 	2,	NULL,		OBSOLETE },
    { lkdisp, 		1,	NULL,		0 },
    { SETUP, 		2,	n_setup,	SIGNAL_COMPLETION },
    { switchend, 	1,	NULL,		0 },
    { TAPBOUT, 		0,	n_tapbout,	APBCOUT },
    { INITTABLEVAR, 	1,	NULL,		0 },
    { SAVRTNAP, 	1,	NULL,		0 },
    { WGDLOAD, 		1,	NULL,		0 },
    { INCGRAD, 		9,	NULL,		OBSOLETE },
    { JUSTSTFIFO, 	1,	NULL,		OBSOLETE },
    { INITVSCAN, 	1,	n_init,		INITSCAN },
    { APSOUT, 		5,	NULL,		OBSOLETE },
    { SLI, 		5,	NULL,		OBSOLETE },
    { VSLI, 		4,	NULL,		OBSOLETE },
    { IACQIUPDTCMPLT, 	1,	n_scopy,	SET_ACQI_UPDT_CMPLT },
    { ISTARTACQIUPDT, 	1,	n_scopy,	START_ACQI_UPDT },
    { ISETDATAOFFSET, 	4,	n_scopy_r,	SET_DATA_OFFSETS },
    { ISETNP, 		3,	n_scopy_r,	SET_NUM_POINTS },
    { ENABLEOVRFLOW, 	4,	n_enableovrflow, ENABLEOVRLDERR },
    { DISABLEOVRFLOW, 	2,	n_disableovrflow, DISABLEOVRLDERR },
    { EXTGATE, 		3,	n_scopy,	INOVAXGATE },
    { ROTORSYNC,	3,	n_scopy,	INOVAROTORSYNC },
    { IPACODE, 		2,	n_ipacode,	0 },
    { FIDCODE, 		1,	n_fidcode,	0 },
    { IAPREAD,		7,	n_iapread,	EXEAPREAD},
    { IMASKON, 		3,	n_scopy,	MASKHSL },
    { IMASKOFF, 	3,	n_scopy,	UNMASKHSL },
    { SETLOCKFREQ,   	3,	n_scopy,	LOCKFREQ_I },
    { LOCKSEQUENCE,	1,	n_lock_seq,	0 },
    { SSHA,		3,	n_scopy,	SSHA_INOVA },
    /*  Leave the SPARE12 entry as the last entry in the translation table.  */
    { SPARE12, 		2,	n_spare12,	MASKHSL },
};

#define LASTCODE  SPARE12
#define ACODE_BUFFER    (21839)

static codeint  stack_short[20];
static codeint  *newstartptr = NULL;
static codeint  *oldstartptr, *ncodeptr;
static codeint  *hwloopptr, *rtinit_ptr;
static codeint  *autogainptr = NULL;
static codeint  *autoshimptr = NULL;
static int	lstackptr = 0;
static int	stackptr = 0;
static int	newAcodeSize = 0;
static unsigned int   stack_long[62];

static codeint       old_nsc_branch[12];
static unsigned int  new_nsc_branch[12];
static int  num_nsc;

static unsigned int  new_init_branch;

static int phasetablenum[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

/* flags = 0, rt_oversamp = 1, rt_extrapts = 0, il_oversamp = 1, il_extrapts = 0, rt_downsamp = 1  */
struct _dsp_params dsp_off_params  = { 0, 1, 0, 1, 0, 1 }; 

static union {
         unsigned int ival;
         codeint cval[2];
      } endianSwap;

#define IXPRINT0(str)           if (ix == 1) printf(str)
#define IXPRINT1(str,arg1)      if (ix == 1) printf(str,arg1)
#define IXPRINT2(str,arg1,arg2) if (ix == 1) printf(str,arg1,arg2)

static int
rcvrActive(short n)
{
   extern int rt_tab[];
    int mask;
    int rtn;

    rtn = (rt_tab[activercvrs+TABOFFSET] >> n) & 1;
    return rtn;
}


codeint
*convert_Acodes(oldstart, oldlast, retsize)
codeint  *oldstart, *oldlast;
int	 *retsize;
{
	int	 num, size;
	codeint  retLen;
	codeint  *cptr;
        static codeint  *mallocptr = NULL;
	static int	old_size = 0;


	size = oldlast - oldstart;
	cptr = oldstart;
	oldstartptr = oldstart;
	if (size <= 0)
	{
	    printf("psg: the size of acode is zero\n");
	    psg_abort(1);
	}
	if (newAcodeSize < size * 4)
	{
	    if (mallocptr != NULL)
		free(mallocptr);
	    newAcodeSize = size * 4;
	    mallocptr = (codeint *) malloc(newAcodeSize + 2 * sizeof(codelong));
	    if (mallocptr == NULL)
	    {
		printf("psg: do not have enough memory\n");
		psg_abort(1);
	    }
	    newstartptr = mallocptr + 4;;
	}
	num_nsc = 0;
	ncodeptr = newstartptr;
	*ncodeptr++ = ACODE_BUFFER;
	num_aptables = 0;
        if (rtinit_count)
        {
           /* Save room for the RT init codes.  As a RTINIT acode is parsed,
            * the three arguments will be passed at the rtinit_ptr location
            */
	   *ncodeptr++ = RTINIT;
	   *ncodeptr++ = rtinit_count * 3;
           rtinit_ptr = ncodeptr;
	   ncodeptr += rtinit_count * 3;
        }
	while (cptr < oldlast)
	{
	     if (*cptr < 0 || *cptr > LOCKSEQUENCE)
	     {
		IXPRINT2("psg: Unknown Code '%d' addr is %d\n", *cptr, cptr);
		cptr++;
		continue;
	     }
	     num = 0;
	     /* IXPRINT2("psg: Acode: '%d' addr is %d\n", *cptr, cptr); */
	     while (1)
	     {
		if (trTable[num].oldCode == *cptr)
		{
		    if (trTable[num].func != NULL)
		       retLen = trTable[num].func(cptr, num);
		    else
		    {
		       if (trTable[num].newCode != OBSOLETE)
		          IXPRINT1("psg: skip code '%d'\n", trTable[num].oldCode);
		    }
		    cptr += trTable[num].oldLen;
		    break;
		}
		else
		{
		    if (trTable[num].oldCode == LASTCODE)
		    {
			IXPRINT2("psg: Unknown code '%d'  addr is %d\n", *cptr, cptr);
			cptr++;
			break;
		    }
		}
		num++;
	     }
	}
	*retsize = (int)(ncodeptr - newstartptr);
	cleanupaptableinfo();
	return(mallocptr);
}

push_short (data)
codeint  data;
{
	if (stackptr < 18)
	{
	    stack_short[stackptr] = data;
	    stackptr++;
	}
	else
	{
	    text_error("Too many nested loops or if statements\n");
	    psg_abort(1);
	}
}

codeint
pop_short()
{
	if (stackptr > 0)
	{
	    stackptr--;
	    return(stack_short[stackptr]);
	}
	else
	{
	    text_error("missing loop() or ifzero()\n");
	    psg_abort(1);
	}
}

push_long (data)
unsigned int  data;
{
	if (lstackptr < 60)
	{
	    stack_long[lstackptr] = data;
	    lstackptr++;
	}
	else
	{
	    text_error("Too many nested loops or if statements\n");
	    psg_abort(1);
	}
}


unsigned int
pop_long()
{
	if (lstackptr > 0)
	{
	    lstackptr--;
	    return(stack_long[lstackptr]);
	}
	else
	{
	    text_error("missing loop() or ifzero()\n");
	    psg_abort(1);
	}
}

putcode_long(data, addr)
unsigned int data;
codeint      *addr;
{
   endianSwap.ival = data;
#ifdef LINUX
   *addr++ = endianSwap.cval[1];
   *addr++ = endianSwap.cval[0];
#else
   *addr++ = endianSwap.cval[0];
   *addr++ = endianSwap.cval[1];
#endif
}

/*  n_scopy will change Acode, add the length at the second addr, and
    copy the rest */
int
n_scopy(data,num)
codeint  *data;
int   num;
{
	codeint   len;

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len;
	data++;
	while (len > 0)
	{
	    *ncodeptr++ = *data++;
	    len--;
	}
	return(0);
}

/*  n_scopy_r will change Acode, add the length at the second addr,
    copy the rest, and add "activercvrs" at the end */
int
n_scopy_r(data,num)
codeint  *data;
int   num;
{
        n_scopy(data, num);
	*ncodeptr++ = activercvrs;
	return(0);
}

/*  n_scopytst will change Acode, add the length at the second addr, and
    copy the rest */
int
n_scopytst(data,num)
codeint  *data;
int   num;
{
	codeint   len;

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len;
	printf("n_scopytst: Acode: %d, New Acode: %d, len: %d\n",
	 *data, trTable[num].newCode,len);
	data++;
	while (len > 0)
	{
	    printf("n_scopytst: Arg: %d\n",*data);
	    *ncodeptr++ = *data++;
	    len--;
	}
	return(0);
}

int
n_rtinit(addr, num)
codeint  *addr;
int	 num;
{
	addr++;
        *rtinit_ptr++ = *addr++;
        *rtinit_ptr++ = *addr++;
        *rtinit_ptr++ = *addr++;
}

/*  n_copy will change Acode and copy the rest */
int
n_copy(addr, num)
codeint  *addr;
int	 num;
{
	int   len;

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	addr++;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
	return(0);
}

/*  n_cpsamp will change Acode and copy the rest */
int
n_cpsamp(addr, num)
codeint  *addr;
int	 num;
{
	int   len;
        int   robotype;

	len = trTable[num].oldLen - 1;
	/* printf("n_loadsamp: Acode: %d, New Acode: %d, len: %d\n",  */
	 /* *addr, trTable[num].newCode,len);   */
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 3 + len;	/* 1 long == 2 shorts */
	addr++; /* skip over old acode */
/*
        printf("n_cpsamp: loc: %lu, lsw: %u hsw: %u ncodeptr: 0x%lx \n",
	loc,(loc & 0xffff),((loc & 0xffff0000) >> 16),ncodeptr);
*/
	*ncodeptr++ = loc & 0xffff;
	*ncodeptr++ = (((unsigned int)loc) & 0xffff0000) >> 16;

        /* if Gilson or NanoProbe changer then skip sample detection step */
	robotype = ((traymax == 48) || (traymax == 96)) ? 1 : 0;  /* skip sample verification */
        if ( (traymax == 12) || (traymax == 97) )
           robotype = 2;
	*ncodeptr++ = robotype;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

/*  n_vtset will change Acode and copy the rest */
int
n_vtset(addr, num)
codeint  *addr;
int	 num;
{
	int   len;

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len;
	addr++; /* skip over old acode */
/*
        printf("n_vtset: SETVT nacode: %d, len: %d\n",trTable[num].newCode,len);
	printf("n_vtset: SETVT acode: %d\n",*addr++);
*/
	while (len > 0)
	{
	    /* printf("n_vtset: copy %d\n",*addr); */
	    *ncodeptr++ = *addr++;
	    len--;
	}
	return(0);
}

int
n_initdelay(addr, num)
codeint  *addr;
int	 num;
{
	int   len, index, maxindex;

	/* Number of stored delay initializations for incdelay is 	*/
	/* (endincrdelay - incrdelay)/2.  Each delay is two realtime	*/
	/* locations.							*/
	maxindex = (endincrdelay - incrdelay)/2;

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	addr++;
	index = *addr++;
	len--;
	if (index > maxindex)
	{
	   printf("initdelay: index = %d greater than maxindex = %d.\n",
						index,maxindex);
	   psg_abort(1);
	}
	*ncodeptr++ = 5;
	*ncodeptr++ = incrdelay + (index*2);
	addr++;			/* extra count word */
	len--;
	/* printf("initdelay: d0:0x%x d1:0x%x d2:0x%x d3:0x%x\n", */
	/* 			*addr, *(addr+1), *(addr+2), *(addr+3) ); */
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
	return(0);
}

int
n_incrdelay(addr, num)
codeint  *addr;
int	 num;
{
	int   index, maxindex, rtflag;

	/* Number of stored delay initializations for incdelay is 	*/
	/* (endincrdelay - incrdelay)/2.  Each delay is two realtime	*/
	/* locations.							*/
	maxindex = (endincrdelay - incrdelay)/2;

	addr++;
	index = *addr++;
	rtflag = index & 0xff00;
	if (rtflag)
	   *ncodeptr++ = INCRDELAY_R;	/* realtime mult pointer */
	else
	   *ncodeptr++ = trTable[num].newCode;	/* absval mult */
	index = index & 0x0ff;
	if (index > maxindex)
	{
	   printf("incrdelay: index = %d greater than maxindex = %d.\n",
						index,maxindex);
	   psg_abort(1);
	}
	*ncodeptr++ = 2;
	*ncodeptr++ = incrdelay + (index*2);
	*ncodeptr++ = *addr++;

	return(0);
}

/* Time Increments in 12.5 ns ticks */
#define		T_100NSEC	8
#define		T_USEC		80
#define		T_MSEC		80000
#define		T_SEC		80000000
int
n_rt_event1(addr, num)
codeint  *addr;
int	 num;
{
	int   instruct, absbase;
	addr++;
	instruct = *addr++;
	if (instruct == TCNT)
	{
	   *ncodeptr++ = trTable[num].newCode;	/* vdelay */
	   *ncodeptr++ = 5;
	   absbase = *addr++;
	   *ncodeptr++ = *addr++;	/* rtcount: rtvar index */
	   addr++;			/* hslines: not used	*/
	   switch (absbase)
	   {
		case NSEC:
			putcode_long(1, ncodeptr);	 /* increment */
			ncodeptr += 2;
			putcode_long(T_100NSEC, ncodeptr); /* offset */
			ncodeptr += 2;
			break;
		case USEC:
			putcode_long(T_USEC, ncodeptr);	/* increment */
			ncodeptr += 2;
			putcode_long(0, ncodeptr);	/* offset */
			ncodeptr += 2;
			break;
		case MSEC:
			putcode_long(T_MSEC, ncodeptr);	/* increment */
			ncodeptr += 2;
			putcode_long(0, ncodeptr);	/* offset */
			ncodeptr += 2;
			break;
		case SECONDS:
			putcode_long(T_SEC, ncodeptr);	/* increment */
			ncodeptr += 2;
			putcode_long(0, ncodeptr);	/* offset */
			ncodeptr += 2;
			break;
		default:
	   	 	printf("rt_event1: Invalid Timebase: %d.\n",absbase);
	   		psg_abort(1);
			break;
	   }
	   
	}
	else
	{
	   printf("Warning - rt_event1 Instruction: %d Not Implemented.\n",
						instruct);
	   addr++;	/* rtbase */
	   addr++;	/* rtcount */
	   addr++;	/* hslines */
	}

	return(0);
}


int
n_hwloop(addr, num)
codeint  *addr;
int	 num;
{
	*ncodeptr++ = trTable[num].newCode;
	addr++;
	*ncodeptr++ = 3;
	addr++;		/* Increment past number of fifo words        */
			/* this is taken care of with ENDHWLOOP acode */
	addr++;		/* Increment past multiple hwloops flag, not needed */
	*ncodeptr++ = *addr++;	/* set rtvar index */
        hwloopptr = ncodeptr;
	ncodeptr += 2; /* Reserve space for jump location - set by ENDHWLOOP */

	return(0);
}

int
n_endhwloop(addr, num)
codeint  *addr;
int	 num;
{
	unsigned int  offset;
	*ncodeptr++ = trTable[num].newCode;
	addr++;
	*ncodeptr++ = 0;
	offset = ncodeptr - hwloopptr;
	putcode_long(offset, hwloopptr);

	return(0);
}

#define APADDRMSK	0x0fff 
#define APBYTF		0x1000

#define LENMASK		0x00ff
#define RTVAR_BIT    0x100   /* bit to indicate real-time variable */
#define NEGLOGIC_BIT 0x200   /* bit to indicate negative logic */
#define PWR_NOT_ATTN 0x400   /* another bit for logic */

int
n_apcout(addr, num)
codeint  *addr;
int	 num;
{
	int	 i;
	codeint  len,controlword,offset,maxval,value,apaddr;

	addr++;				/* skip acode */
	apaddr = *addr++;		/* get apaddr */
	controlword = *addr++;
	maxval = *addr++;
	offset = *addr++;
	value = *addr++;
	/* if (controlword & PWR_NOT_ATTN) != 0) */
	/*    value = value + offset; */
	/* else */
	/*    value = maxval - (value + offset); */

	if ((controlword & LENMASK) == 2)
	{
	    if ((controlword & RTVAR_BIT) != 0)
	    {
	    	*ncodeptr++ = APRTOUT;
	    	*ncodeptr++ = 6;
	    	*ncodeptr++ = STD_APBUS_DELAY;  	/* delay */
	    	*ncodeptr++ = apaddr;
	    	*ncodeptr++ = maxval;
	    	*ncodeptr++ = 0;	/* minval */
	    	*ncodeptr++ = offset;
	    	*ncodeptr++ = value;

	    }
	    else {
	    	/* set xmtr controller linear attenuation */
		value = value + offset;
		if (value > maxval)
		{
		    value = maxval;
		    text_error("Warning: power overflow, set to maximum");
		}
		/* Make sure value is not less than zero... which may	*/
		/* look like a large positive value to attenuator	*/
		if (value < 0) 
		{
		    value = 0;
		    text_error("Warning: power underflow, set to minimum");
		}
		/* IXPRINT2("apcout<byte>: apaddr: 0x%x  value: 0x%x\n", */
		/*					apaddr,value); */
		*ncodeptr++ = trTable[num].newCode;
	    	*ncodeptr++ = 4;
	    	*ncodeptr++ = STD_APBUS_DELAY;  /* delay */
	    	*ncodeptr++ = 1;
	    	*ncodeptr++ = apaddr;
	    	*ncodeptr++ = value;
	    }
	}
	else if ((controlword & LENMASK) == 1){
	    if ((controlword & RTVAR_BIT) != 0)
	    {
	    	*ncodeptr++ = APRTOUT;
	    	*ncodeptr++ = 6;
	    	*ncodeptr++ = STD_APBUS_DELAY;  	/* delay */
	    	*ncodeptr++ = apaddr | APBYTF;
	    	*ncodeptr++ = maxval;
	    	*ncodeptr++ = 0;	/* minval */
	    	*ncodeptr++ = offset;
	    	*ncodeptr++ = value;
	    }
	    else {
	    	/* check for maxval and minval before shifting */
		value = value + offset;
		if (value > maxval)
		{
		    value = maxval;
		    text_error("Warning: power overflow, set to maximum");
		}
		/* Make sure value is not less than zero... which may	*/
		/* look like a large positive value to attenuator	*/
		if (value < 0) 
		{
		    value = 0;
		    text_error("Warning: power underflow, set to minimum");
		}
		value = value*2;
		/* IXPRINT2("apcout<byte>: apaddr: 0x%x  value: 0x%x\n", */
		/* 					apaddr,value); */
		*ncodeptr++ = trTable[num].newCode;
	   	*ncodeptr++ = 4;
	    	*ncodeptr++ = STD_APBUS_DELAY;  /* delay */
	    	*ncodeptr++ = 1;
	    	*ncodeptr++ = apaddr | APBYTF;
	    	*ncodeptr++ = value << 8;	/* shift for 0.5 dB step */
	    }
	}
	else 
	{
	    IXPRINT2("convert(apcout) Unknown bytes apaddr: 0x%x cntl: 0x%x\n",
						apaddr,controlword);
	    psg_abort(1);
	}
	return(0);
}


int
n_tapbout(addr, num)
codeint  *addr;
int	 num;
{
	int i;
	codeint  len;

	addr++;
	len = *addr++;

 	trTable[num].oldLen = len + 2;

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len;
	for (i=0; i<len; i++)
	   *ncodeptr++ = *addr++;

}

#define APCCNTMSK	0x0fff
#define APCONTNUE	0x1000

#define APSELECT 0xA000         /* select apchip register */
#define APWRITE  0xB000         /* write to apchip reister */
#define APPREINCWR 0x9000       /* preincrement register and write */
#define APCTLMASK 0xf000
#define APDATAMASK 0x00ff

int
n_apbout(addr, num)
codeint  *addr;
int	 num;
{
	int	 ret,lindex,dlindex, i, aindex,j,nacodes;
	codeint  len,datalen,curaddr;
	codeint  *tmpcodeptr, *tmpdelayptr;

	addr++;
	len = *addr++;

/*	for (i=0; i<=len; i++)
 *	{
 *	    IXPRINT2("n_apbout %d = 0x%x\n",i,*(addr+i));
 *	}
 */
	ret = len + 3;
	if (len <= 0)
 	    trTable[num].oldLen = 2;
	else
 	    trTable[num].oldLen = len + 3;

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 0;
	nacodes = gen_apbcout(ncodeptr,addr,len);
	*(ncodeptr-1) = nacodes;	/* update with correct length */
	ncodeptr = ncodeptr+nacodes;

	/* IXPRINT1("gen_apbcout ncodeptr = 0x%x\n",ncodeptr); */

}

int
n_iapread(addr, num)
codeint  *addr;
int	 num;
{
	codeint  apaddrreg,rtparam;
	addr++;
	apaddrreg = *addr++;
	rtparam = *addr++;

	/* exec ap read */
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 2;
	*ncodeptr++ = READ_APBUS_DELAY;
	*ncodeptr++ = apaddrreg;

	/* parser sync */
	*ncodeptr++ = SYNC_PARSER;
	*ncodeptr++ = 4;
	*ncodeptr++ = *addr++;	/* delay secs */
	*ncodeptr++ = *addr++;  /* delay secs */
	*ncodeptr++ = *addr++;	/* delay ticks */
	*ncodeptr++ = *addr++;  /* delay ticks */

	/* read readback fifo */
	*ncodeptr++ = GETAPREAD;
	*ncodeptr++ = 2;
	*ncodeptr++ = apaddrreg;
	*ncodeptr++ = rtparam;
}


int
n_todo(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;

	*ncodeptr++ = TODO;
	len = trTable[num].oldLen;
	*ncodeptr++ = len;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

int
n_initfreq(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;

	IXPRINT1("psg: skip code '%d'\n", trTable[num].oldCode);
	len = *(addr + 10);
	len += 11;
	trTable[num].oldLen = len;
	return(0);
}

int
n_begin()
{
	*ncodeptr++ = INTERP_REV_CHK;
	*ncodeptr++ = 2;
	*ncodeptr++ = INOVA_INTERP_REV;
	*ncodeptr++ = 0;

	*ncodeptr++ = INIT_FIFO;

	*ncodeptr++ = 2;
	if (debug2)
	   *ncodeptr++ = 1;	/* write to file */
	else
	   *ncodeptr++ = 0;	
	*ncodeptr++ = 0;

	/* Added new fourth arg: HS DTM/ADC (5MHz DTM/ADC) flag 4/23/96 GMB */
	*ncodeptr++ = INIT_STM;
	*ncodeptr++ = 5;
	*ncodeptr++ = dpfrt;  /* dp float pointer */
	if ( get_acqvar(npnoise) > get_acqvar(npr_ptr) )
	   *ncodeptr++ = npnoise; /* bytes of data point */
	else
	   *ncodeptr++ = npr_ptr; /* bytes of data point */
	*ncodeptr++ = nfidbuf;	/* number of buffers */
	*ncodeptr++ = HS_Dtm_Adc; /* If true select the 5MHz STM/ADC */
	*ncodeptr++ = activercvrs; /* Which STMs to initialize */

	*ncodeptr++ = INIT_ADC;
	*ncodeptr++ = 3;
	/* Select channel, interrupts, etc.. */
	*ncodeptr++ = HS_Dtm_Adc;/* if using HS DTM, set flag 1(disable ADC), now set in lc_hdl.c */
	*ncodeptr++ = adccntrl;	/* adcntrl rt parm  */
	*ncodeptr++ = activercvrs; /* Which ADCs to initialize */

        if (prepScan)
        {
           /* only send one SYNC acode */
           prepScan = 0;
           /*
            * -1's are a special signal to interpreter to wait
            * for sethw release of semaphore
            */
	   *ncodeptr++ = SYNC_PARSER;
	   *ncodeptr++ = 4;
	   *ncodeptr++ = -1;
	   *ncodeptr++ = -1;
	   *ncodeptr++ = -1;
   	   *ncodeptr++ = -1;
        }
}

int
n_setup(addr, num)
codeint  *addr;
codeint	 num;
{
	addr++;
	/* load default line safe state */
	*ncodeptr++ = LOADHSL_R;
	*ncodeptr++ = 1;
	*ncodeptr++ = HSlines_ptr;
        *ncodeptr++ = EVENT1_DELAY;
        *ncodeptr++ = 2;
        *ncodeptr++ = (codeint) 0x0;
        *ncodeptr++ = (codeint) 0x800;		/* 100 usec */

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 1;
	*ncodeptr++ = SETUP_CMPLT;

	*ncodeptr++ = NEXTCODESET;
	*ncodeptr++ = 4;
	*ncodeptr++ = zero;	/* ilflagrt 	*/
	*ncodeptr++ = zero; 	/* bsctr	*/
	*ncodeptr++ = zero; 	/* ntrt		*/
	*ncodeptr++ = zero; 	/* ct		*/

}

int
n_rtop(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;

	*ncodeptr++ = RTOP;
	len = trTable[num].oldLen;
	*ncodeptr++ = len;
	*ncodeptr++ = trTable[num].newCode;
	len--;
	addr++;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}


int
n_rt2op(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;

	*ncodeptr++ = RT2OP;
	len = trTable[num].oldLen;
	*ncodeptr++ = len;
	*ncodeptr++ = trTable[num].newCode;
	len--;
	addr++;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}


int
n_rt3op(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;

	*ncodeptr++ = RT3OP;
	len = trTable[num].oldLen;
	*ncodeptr++ = len;
	*ncodeptr++ = trTable[num].newCode;
	len--;
	addr++;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}


int
n_ifnz(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  jump;
	codeint  *tptr;
	unsigned int  offset;

	tptr = addr + 3;  /* get the address of offset */
	jump = *tptr;

	/* assume no branch */
	tptr = oldstartptr + jump;
	push_short(*tptr);
	*tptr = ENDIF;

	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = *addr++;
	offset = ncodeptr - newstartptr;
	push_long (offset); /* store the address of jump */
	*ncodeptr++ = 0;
	*ncodeptr++ = 0;
        /* Force the next HS line change to be sent */
        forceHSlines = 1;
}


/*----------------------------------------------------------------------*/
/* n_ifnzb								*/
/*	This routine differs from n_ifnz in that a branch is associated	*/
/*	with it.  This supports the ifzero...elsenz....endif construct,	*/
/*	where n_ifnz supports the ifzero...endif construct.		*/
/*----------------------------------------------------------------------*/
int
n_ifnzb(addr, num)
codeint  *addr;
codeint	 num;
{
	unsigned int  offset;

	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = *addr++;
	offset = ncodeptr - newstartptr;
	push_long (offset); /* store the address of jump */
	*ncodeptr++ = 0;
	*ncodeptr++ = 0;
        /* Force the next HS line change to be sent */
        forceHSlines = 1;
}


int
n_branch(addr, num)
codeint  *addr;
codeint	 num;
{
	int	 n;
	codeint  *tptr, jump;
	unsigned int  ifOffset, elseOffset, offset;

	tptr = addr + 1;
	jump = *tptr;
	n = 0;
	while (n < num_nsc)
	{
	     if (old_nsc_branch[n] == jump) /* BRANCH(NSC) */
	     {
		*ncodeptr++ = trTable[num].newCode;
		*ncodeptr++ = 2;
		putcode_long(new_nsc_branch[n], ncodeptr);
		ncodeptr += 2;
	        return(0);
	     }
	     n++;
	}
		
	tptr = oldstartptr + jump;
	push_short(*tptr);
	*tptr = ENDIF;

	ifOffset = pop_long();
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 2;
	elseOffset = ncodeptr - newstartptr;
	push_long (elseOffset);  /* store the offset address */
	*ncodeptr++ = 0;
	*ncodeptr++ = 0;
	tptr = newstartptr + ifOffset;
	offset = ncodeptr - newstartptr;
	putcode_long(offset, tptr);  /* set jump offset for ifzero */
        /* Force the next HS line change to be sent */
        forceHSlines = 1;
}

int
n_branch_lt(addr, num)
codeint  *addr;
codeint	 num;
{
	int	 n;
	codeint  *tptr, jump;

	tptr = addr + 3;
	jump = *tptr;
	n = 0;
	while (n < num_nsc)
	{
	     if (old_nsc_branch[n] == jump) /* BRANCH(NSC) */
	     {
		addr++;
		*ncodeptr++ = trTable[num].newCode;
		*ncodeptr++ = 4;
	        *ncodeptr++ = *addr++;
	        *ncodeptr++ = *addr++;
		putcode_long(new_nsc_branch[n], ncodeptr);
		ncodeptr += 2;
	        return(0);
	     }
	     n++;
	}
}


int
n_endif(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  *tptr;
	unsigned int  offset;

	offset = pop_long();
	tptr = newstartptr + offset; /* the address of jump offset */
	offset = ncodeptr - newstartptr;
	putcode_long(offset, tptr); /* set jump offset for ifzero or elsenz  */
	*addr = pop_short(); /* restore the Acode */
        /* Force the next HS line change to be sent */
        forceHSlines = 1;
	return(0);
}

int
n_ifminus(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  jump;
	codeint  *tptr;
	unsigned int  offset;

	tptr = addr + 3;  /* get the address of offset */
	jump = *tptr;
	tptr = oldstartptr + jump - 4;
	if (*tptr == IFMIFUNC)	/* comes from endloop */
	{
	  /* this is loop Acode */
		*tptr = ENDLOOP;
		addr++;
		*ncodeptr++ = trTable[num].newCode;
		*ncodeptr++ = 4;
		*ncodeptr++ = *addr++;
		*ncodeptr++ = *addr++;
		offset = ncodeptr - newstartptr;
		push_long (offset); /* store the offset of the jump address */
		*ncodeptr++ = 0;
		*ncodeptr++ = 0;
		offset = ncodeptr - newstartptr;
		push_long (offset); /* store the offset of loop back address */
	}
	else
	{
	    text_error("missing endloop()\n");
	    psg_abort(1);
	}
}


int
n_endloop(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  jump;
	codeint  *tptr;
	unsigned int  offset, loopOffset, backOffset;
	unsigned int  ifOffset, elseOffset;

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	addr++;
	*ncodeptr++ = *addr++;  /* counter */
	*ncodeptr++ = *addr++;  /* number of loop */
	backOffset = pop_long();
	putcode_long(backOffset, ncodeptr);  /* set jump back for endloop */
	ncodeptr += 2;
	loopOffset = pop_long();
	tptr = newstartptr + loopOffset;
	offset = ncodeptr - newstartptr;
	putcode_long(offset, tptr);  /* set jump offset for loop */
}


int
n_hkeep(addr, num)
codeint  *addr;
codeint	 num;
{
        if (rcvr2nt)
        {
	   *ncodeptr++ = trTable[num].newCode + 20;
	   *ncodeptr++ = 8;
	   *ncodeptr++ = rcvr2nt;
        }
        else
        {
	   *ncodeptr++ = trTable[num].newCode;
	   *ncodeptr++ = 7;
        }
	*ncodeptr++ = ssval;
	*ncodeptr++ = ssctr;
	*ncodeptr++ = ntrt;
	*ncodeptr++ = ct;
	*ncodeptr++ = bsval;
	*ncodeptr++ = bsctr;
	*ncodeptr++ = activercvrs;

	/* phase cycling */
	*ncodeptr++ = RTOP;
	*ncodeptr++ = 2;
	*ncodeptr++ = INC;
	*ncodeptr++ = ctss;
	if (acqiflag)
	{
	   *ncodeptr++ = RT3OP;
	   *ncodeptr++ = 4;
	   *ncodeptr++ = MOD;
	   *ncodeptr++ = ctss;
	   *ncodeptr++ = bsval;
	   *ncodeptr++ = ctss;
	}
	else
	{
	   *ncodeptr++ = RT3OP;
	   *ncodeptr++ = 4;
	   *ncodeptr++ = MOD;
	   *ncodeptr++ = ctss;
	   *ncodeptr++ = ntrt;
	   *ncodeptr++ = ctss;
	}
}


int
n_init(addr, num)
codeint  *addr;
codeint	 num;
{
	new_init_branch = ncodeptr - newstartptr;

	if (acqiflag)
	{
	   *ncodeptr++ = RT2OP;
	   *ncodeptr++ = 3;
	   *ncodeptr++ = SET;
	   *ncodeptr++ = zero;
	   *ncodeptr++ = ct;
	}

        if (rcvr2nt)
        {
	   *ncodeptr++ = trTable[num].newCode + 20;
	   *ncodeptr++ = 10;
	   *ncodeptr++ = rcvr2nt;
        }
        else
        {
	   *ncodeptr++ = trTable[num].newCode;
	   *ncodeptr++ = 9;
        }
	*ncodeptr++ = npr_ptr;
	*ncodeptr++ = ssval;
	*ncodeptr++ = ssctr;
	*ncodeptr++ = ntrt;
	*ncodeptr++ = ct;
	*ncodeptr++ = bsval;
	*ncodeptr++ = bsctr; /* bsctr */
	*ncodeptr++ = maxsum;
	*ncodeptr++ = activercvrs;

	*ncodeptr++ = RT2OP;
	*ncodeptr++ = 3;
	*ncodeptr++ = SET;
	*ncodeptr++ = ct;
	*ncodeptr++ = strt;
}

static void
set_dsp(codeint **pncodeptr, struct _dsp_params *pDsp_params, int ap_adr)
{
    codeint *ncodeptr;

    /* fprintf(stdout,"set_dsp - params: rt downld: %d, flags: %d\n",pDsp_params->rt_downsamp,pDsp_params->flags); */
    ncodeptr = *pncodeptr;
    /************ tell the card about time correction ******/
    *ncodeptr++ = APBCOUT;
    *ncodeptr++ = 4;
    *ncodeptr++ = STD_APBUS_DELAY;  	/* delay */
    *ncodeptr++ = 1;
    *ncodeptr++ = ap_adr;
    /*	*ncodeptr++ = (codeint) dsp_info[2];*/
    /* *ncodeptr++ = (codeint) dsp_params.rt_downsamp; */
    *ncodeptr++ = (codeint) pDsp_params->rt_downsamp;
    /*******************************************************/
    *ncodeptr++ = APBCOUT;
    *ncodeptr++ = 4;
    *ncodeptr++ = STD_APBUS_DELAY;  	/* delay */
    *ncodeptr++ = 1;
    *ncodeptr++ = ap_adr+2;
    /*	*ncodeptr++ = (codeint) dsp_info[0];*/
    /* *ncodeptr++ = (codeint) dsp_params.flags; */
    *ncodeptr++ = (codeint) pDsp_params->flags;

    *pncodeptr = ncodeptr;
}

int
n_noise(addr, num)
codeint  *addr;
codeint	 num;
{
   extern int rt_tab[];

	if (acqiflag) return;
/*
	dsp_off_params.flags = 0;
        dsp_off_params.rt_oversamp = 1;
        dsp_off_params.rt_downsamp = 1;
*/

        /* printf("n_noise, offset: %d\n",ncodeptr - newstartptr); */

	*ncodeptr++ = RT2OP;
	*ncodeptr++ = 3;
	*ncodeptr++ = SET;
	*ncodeptr++ = zero;
	*ncodeptr++ = tmprt;

	*ncodeptr++ = INITSCAN;
	*ncodeptr++ = 9;
	*ncodeptr++ = npnoise;
	*ncodeptr++ = zero; /* ssval */
	*ncodeptr++ = ssctr;
	*ncodeptr++ = one;  /* nt */
	*ncodeptr++ = strt; /* ct */
	*ncodeptr++ = zero;  /* bsval */
	*ncodeptr++ = tmprt; /* bsctr */
	*ncodeptr++ = maxsum;
	*ncodeptr++ = activercvrs;

        /* Set DSP hardware */
        if (rcvrActive(0))
	{
           /* fprintf(stdout,"1st rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xe84);
        }
	else
	{
           /* fprintf(stdout,"1st rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xe84);
        }

        if (rcvrActive(1))
	{
           /* fprintf(stdout,"2nd rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xea4);
        }
        else
	{
           /* fprintf(stdout,"2nd rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xea4);
        }

        if (rcvrActive(2))
	{
           /* fprintf(stdout,"3rd rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xec4);
        }
        else
	{
           /* fprintf(stdout,"3rd rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xec4);
        }

        if (rcvrActive(3))
	{
           /* fprintf(stdout,"4th rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xee4);
        }
        else
	{
           /* fprintf(stdout,"4th rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xee4);
        }

	/* Next Scan Opcode */
	*ncodeptr++ = NEXTSCAN;
	*ncodeptr++ = 8;
	*ncodeptr++ = npnoise;
	*ncodeptr++ = dpfrt; /* dp index */
	*ncodeptr++ = one;  /* nt */
	*ncodeptr++ = strt; /* ct */
	*ncodeptr++ = zero; /* bsval */
	*ncodeptr++ = dtmcntrl;  /* dtmcntrl */
	*ncodeptr++ = zero; /* fidctr */
	*ncodeptr++ = activercvrs;

        *ncodeptr++ = EVENT1_DELAY;
        *ncodeptr++ = 2;
        *ncodeptr++ = (codeint) 0;
        *ncodeptr++ = (codeint) 2400;	/* 30 usec dsp settling delay */


#ifdef XXXX
          if (HS_Dtm_Adc == 0)
          {
	   *ncodeptr++ = DISABLEOVRLDERR;
	   *ncodeptr++ = 2;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
          }
          else
          {
	   *ncodeptr++ = DISABLEHSSTMOVRLDERR;
	   *ncodeptr++ = 1;
	   *ncodeptr++ = dtmcntrl;
          }
#else
	   *ncodeptr++ = DISABLEOVRLDERR;
	   *ncodeptr++ = 3;
	   *ncodeptr++ = dtmcntrl;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
/*
   printf("n_disableovrflow: len: %d, dtmcntrl: %d->0x%x, adccntrl: %d->0x%x, actrcvr: 0x%x\n",
	 *(ncodeptr-4),dtmcntrl,rt_tab[dtmcntrl],adccntrl,rt_tab[adccntrl],*(ncodeptr-1));
*/
#endif


	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 5;
	*ncodeptr++ = *addr++;   /* flags=hwlooping */
	*ncodeptr++ = *addr++;   /* datapoints (msw) */
	*ncodeptr++ = *addr++;   /* datapoints (lsw) */
	*ncodeptr++ = *addr++;   /* timerword 1 (msw) */
	*ncodeptr++ = *addr++;   /* timerword 1 (lsw) */

        *ncodeptr++ = EVENT1_DELAY;
        *ncodeptr++ = 2;
	putcode_long(noise_dwell, ncodeptr);
	ncodeptr += 2;

	/* End of Scan */
	*ncodeptr++ = ENDOFSCAN;
	*ncodeptr++ = 7;
	*ncodeptr++ = zero; /* ssval */
	*ncodeptr++ = ssctr;
	*ncodeptr++ = one;  /* nt */
	*ncodeptr++ = strt; /* ct */
	*ncodeptr++ = zero;  /* bsval */
	*ncodeptr++ = tmprt; /* cbs */
	*ncodeptr++ = activercvrs;

        *ncodeptr++ = STATBLOCK_UPDT;
        *ncodeptr++ = 0;

        *ncodeptr++ = EVENT1_DELAY;
        *ncodeptr++ = 2;
        *ncodeptr++ = (codeint) 0x24;
        *ncodeptr++ = (codeint) 0x9f00;	/* 30 msec dsp settling delay */

	if (!debug2)
	{
	   /* after noise, stop fifo and let noise data come to host then */
	   /* startup avoids FOO problems with memory allocation, etc...  */
	   /* If debug2 (output fifowords to file) is set do not stop the */
	   /* fifo, as this will keep words from being written to the     */
	   /* fifo.							  */
           *ncodeptr++ = NOISE_CMPLT;
           *ncodeptr++ = 0;
	}
}

int
n_tassign(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len, loc;
	int	 pindex;

	addr++;
	loc = *addr++;
	for (pindex = 0; pindex < num_aptables; pindex++)
	{
	     if (apTable[pindex]->oldacodeloc == loc)
		break;
	}
	if (pindex >= num_aptables)
	{
	     printf("psg: tassign error, couldn't find table address\n");
	     psg_abort(1);
	}
	if ( apTable[pindex]->auto_inc )
	     trTable[num].oldLen = 3;
	else
	     trTable[num].oldLen = 4;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 3;
	*ncodeptr++ = apTable[pindex]->newacodeloc;
	if ( apTable[pindex]->auto_inc )
	     *ncodeptr++ = 0;
	else
	     *ncodeptr++ = *addr++;
	*ncodeptr++ = *addr++;
}

/* SETPHATTR */
int
n_todo_1(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;

	*ncodeptr++ = TODO;
	if (ap_interface == 4)
	    trTable[num].oldLen = 14;
	else
	    trTable[num].oldLen = 13;
	len = trTable[num].oldLen;
	*ncodeptr++ = len;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

int
n_loadshim(data,num)
codeint  *data;
int   num;
{
	codeint   len;
	int tword1,tword2;

	/* printf("n_loadshim: add Sync Parser Acode\n");  */
        *ncodeptr++ = SYNC_PARSER;
        *ncodeptr++ = 4;
	/* printf("n_loadshim:  delay: %lf sec.\n",ldshimdelay);  */
        timerwords(ldshimdelay,&tword1,&tword2);
	putcode_long(tword2, ncodeptr);
	ncodeptr += 2;
	putcode_long(tword1, ncodeptr);
	ncodeptr += 2;

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = trTable[num].oldLen;
	*ncodeptr++ = zero;
	data++;
	while (len > 0)
	{
	    *ncodeptr++ = *data++;
	    len--;
	}
	return(0);
}


/*   SHIMA */
int
n_shim(addr, num)
codeint  *addr;
codeint	 num;
{
	double   max,min,step;
	codeint  len;

        /* printf("n_shim, offset: %d\n",ncodeptr - newstartptr); */
        addr++;
	*ncodeptr++ = trTable[num].newCode;
	len = *(addr+1);
	trTable[num].oldLen = len+3;
	/* *ncodeptr++ = len + 4; */
	*ncodeptr++ = len + 10;
	*ncodeptr++ = ct;
	*ncodeptr++ = whenshim;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = *addr++;

        autogainptr = ncodeptr;   /* to be patch later via FIDCODE */
        /* printf("n_shim: autogainptr: %d\n",autogainptr); */
	*ncodeptr++ = 0; /* offset into Acodes to obain FID */
	*ncodeptr++ = ntrt;
	*ncodeptr++ = npr_ptr;
	*ncodeptr++ = dpfrt;
	par_maxminstep(GLOBAL, "lockpower", &max, &min, &step);
	*ncodeptr++ = (short) max;
	par_maxminstep(GLOBAL, "lockgain", &max, &min, &step);
	*ncodeptr++ = (short) max;

	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

int n_fidcode(addr,num)
codeint  *addr;
codeint	 num;
{
   if (autogainptr != NULL)
     *autogainptr = ncodeptr - newstartptr; /* patch AUTOSHIM with Acode offset */
					  /*  to codes that acquire FID */
   if (autoshimptr != NULL)
     *autoshimptr = ncodeptr - newstartptr; /* patch AUTOGAIN with Acode offset */
					  /*  to codes that acquire FID */
 /*
   printf("n_fidcode: autogainptr: %d, value: %d \n",
	autogainptr,ncodeptr - newstartptr);
   printf("n_fidcode: autoshimptr: %d, value: %d \n",
	autoshimptr,ncodeptr - newstartptr);
*/
}

int
n_todo_2(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;
	codeint  *addrLen;

	*ncodeptr++ = TODO;
	addrLen = addr + 2;
	len = *addrLen;
	len += 3;
	trTable[num].oldLen = len;
	*ncodeptr++ = len;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}


/*  LOADSHIM  */
int
n_todo_3(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;
	codeint  *addrLen;

	*ncodeptr++ = TODO;
	addrLen = addr + 1;
	len = *addrLen;
	len = len - (Z0 + 1);
	if (len < 0)
	    len = 0;
	
	len += 2;
	trTable[num].oldLen = len;
	*ncodeptr++ = len;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

int
n_setshim(data,num)
codeint  *data;
int   num;
{
	codeint   len;
	int tword1,tword2;

	/* printf("n_isetshim: add Sync Parser Acode\n");  */
	*ncodeptr++ = SYNC_PARSER;
	*ncodeptr++ = 4;
	/* printf("n_setshim:  delay: %lf sec.\n",oneshimdelay);  */
	timerwords(oneshimdelay,&tword1,&tword2);
	putcode_long(tword2, ncodeptr);
	ncodeptr += 2;
	putcode_long(tword1, ncodeptr);
	ncodeptr += 2;

/*	len = trTable[num].oldLen - 1;
	data++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len;
	*ncodeptr++ = *data++;
	*ncodeptr++ = *data++;
	*ncodeptr++ = *data++; */

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len;
	data++;
	*ncodeptr++ = *data++;
	*ncodeptr++ = *data++;
	*ncodeptr++ = *data++; 

	return(0);
}

/*  TUNE_FREQ */
int
n_todo_4(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len;
	codeint  *addrLen;

	*ncodeptr++ = TODO;
	addrLen = addr + 2;
	len = *addrLen;
	len = len + 5;
	trTable[num].oldLen = len;
	*ncodeptr++ = len;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

int
n_tunefreq(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len,newlen,nacodes;
	codeint  *addrLen,*newLen;

	*ncodeptr++ = trTable[num].newCode;
	addrLen = addr + 2;
	len = *addrLen;
	len = len + 5;
	trTable[num].oldLen = len;

        addr++;
	newLen = ncodeptr;	/* place holder for acode length */
	*ncodeptr++ = 0;	/* place holder for acode length */
	*ncodeptr++ = *addr++;	/* rf channel	1-4		*/
	*ncodeptr++ = 0;	/* holder for length of pts codes */
	len = *addr++;	/* length of pts acodes */
	nacodes = gen_apbcout(ncodeptr,addr,len);
	*(ncodeptr-1) = nacodes;	/* update with correct pts length */
	addr = addr + len + 1;
	ncodeptr = ncodeptr+nacodes;
	*ncodeptr++ = *addr++;		/* band select */
	*newLen = nacodes+3;		/* update acode length */

}

/* TABLE */
int
n_todo_5(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len, table_size;
	codeint  *data;

	data = addr;
	addr++;
	table_size = *addr;
	len = table_size + 5;
	trTable[num].oldLen = len;
	*ncodeptr++ = TODO;
	*ncodeptr++ = len;
	while (len > 0)
	{
	    *ncodeptr++ = *data++;
	    len--;
	}
}


int
n_table(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len, offset;
	int	 pindex = 0;

	addr++;
	offset = addr - oldstartptr;
	num_aptables++;
	if (num_aptables < MAXAPTABLES)
	{
	    pindex = num_aptables-1;
            apTable[pindex] = (apTbl *) (malloc(sizeof(apTbl)));
            if (apTable[pindex] == NULL)
            {
            	text_error("convert.c: Unable to allocate aptable info.\n");
            	psg_abort(1);
            }
	}
	else 
	{
	     printf("convert.c: Too many aptables already defined.\n");
	     psg_abort(1);
	}
	apTable[pindex]->oldacodeloc = offset;

	len = *addr + 4;
	trTable[num].oldLen = len + 1;
	*ncodeptr++ = trTable[num].newCode;
	apTable[pindex]->newacodeloc = ncodeptr - newstartptr;
	apTable[pindex]->newacodeloc += 1; 	/* Add extra word for */
						/* acode length field */
	*ncodeptr++ = len;
	*ncodeptr++ = *addr++;			/* size of table */
	len--;
	apTable[pindex]->auto_inc = *addr;	/* auto-inc flag */
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}


int
n_hsline(addr,num)
codeint  *addr;
int      num;
{
	unsigned int  newHSdata, mask, unmask, flag;
	codeint  *scode;

        addr++;
 	scode = (codeint *) &newHSdata;
	scode[0] = *addr++;
	scode[1] = *addr++;
	flag = newHSdata ^ orgHSdata;
        if (flag == 0)
	    return(0);
        else
        {
	   mask = newHSdata & flag;
	   if (mask)
	   {
	       *ncodeptr++ = MASKHSL;
	       *ncodeptr++ = 2;
	       putcode_long(mask, ncodeptr);
	       ncodeptr += 2;
	   }
	   unmask = (~newHSdata) & flag;
	   if (unmask)
	   {
	       *ncodeptr++ = UNMASKHSL;
	       *ncodeptr++ = 2;
	       putcode_long(unmask, ncodeptr);
	       ncodeptr += 2;
	   }
        }
	orgHSdata = newHSdata;
}

int
n_spare12(addr,num)
codeint  *addr;
int      num;
{
	unsigned int sparelines,spare1,spare2;
	spare1 = 1 << 21;
	spare2 = 1 << 22;
        addr++;
	sparelines = *addr++;
	switch (sparelines) {
	  case 0x00 :
	    *ncodeptr++ = UNMASKHSL;
	    *ncodeptr++ = 2;
	    putcode_long(spare1 | spare2, ncodeptr);
	    ncodeptr += 2;
	    break;
	  case 0x10 :
	    *ncodeptr++ = MASKHSL;
	    *ncodeptr++ = 2;
	    putcode_long(spare1, ncodeptr);
	    ncodeptr += 2;
	    *ncodeptr++ = UNMASKHSL;
	    *ncodeptr++ = 2;
	    putcode_long(spare2, ncodeptr);
	    ncodeptr += 2;
	    break;
	  case 0x20 :
	    *ncodeptr++ = MASKHSL;
	    *ncodeptr++ = 2;
	    putcode_long(spare2, ncodeptr);
	    ncodeptr += 2;
	    *ncodeptr++ = UNMASKHSL;
	    *ncodeptr++ = 2;
	    putcode_long(spare1, ncodeptr);
	    ncodeptr += 2;
	    break;
	  case 0x30 :
	    *ncodeptr++ = MASKHSL;
	    *ncodeptr++ = 2;
	    putcode_long(spare1 | spare2, ncodeptr);
	    ncodeptr += 2;
	    break;
	  default:
	    printf("WARNING: Unknown spare line value: %x , Skipped.\n",
					sparelines);
	    break;
	}
		
}


/*    set receiver gain */
int
n_gain(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;

	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 3;
	*ncodeptr++ = 0xb42;   /* Change Me then change the one in n_again */
	*ncodeptr++ = STD_APBUS_DELAY;	/* dito */
	*ncodeptr++ = rtrecgain;
}

/*  n_again convert old AUTOGAIN to the INOVA AUTOGAIN */
int
n_again(addr, num)
codeint  *addr;
int	 num;
{
	int   len;
        double   max,min,step;

        par_maxminstep(CURRENT, "gain", &max, &min, &step);
/*
        printf("n_again: AUTOGAIN : gain: max %d, min %d, step %d\n",
		(int)max,(int)min,(int)step);
*/
        /* printf("n_again, offset: %d\n",ncodeptr - newstartptr); */

	len = trTable[num].oldLen - 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 11;
	addr++; /* skip over old acode */
        /* printf("n_again: AUTOGAIN : %d, len: %d\n",trTable[num].newCode,3);*/
	/* printf("n_again: npIndex: %d, ntINdex: %d, recvgainIndex: %d\n", */
	 /* npr_ptr,ntrt,rtrecgain); */
	*ncodeptr++ = ntrt;
	*ncodeptr++ = npr_ptr;
	*ncodeptr++ = dpfrt;
	*ncodeptr++ = 0xb42;   /* Change Me then change the one in n_gain */
	*ncodeptr++ = STD_APBUS_DELAY;	/* dito */
	*ncodeptr++ = rtrecgain;
	*ncodeptr++ = (short) max;
	*ncodeptr++ = (short) min;
	*ncodeptr++ = (short) step;
/*	*ncodeptr++ = (short) (((dsp_info[0] & 0x400) == 0x400) ?  20 : 16);*/
	*ncodeptr++ = (short) (((dsp_params.flags & 0x400) == 0x400) ?  20 : 16);
         autoshimptr = ncodeptr;   /* to be patch later via FIDCODE */
         /* printf("n_again: autoshimptr: %d\n",autoshimptr); */
 	*ncodeptr++ = 0; /* offset into Acodes to obain FID */
	return(0);
}

int n_rdrotor(addr, num)
codeint  *addr;
int	 num;
{
        int tword1,tword2;
        int len;

	len = trTable[num].oldLen - 1;
	addr++; /* skip over old acode */
	/* printf("n_rdrotor: add Sync Parser Acode\n");  */
        *ncodeptr++ = SYNC_PARSER;
        *ncodeptr++ = 4;
	/* printf("n_rdrotor: psync delay: %lf sec.\n",psync);  */
        timerwords(psync,&tword1,&tword2);
	/* printf("n_rdrotor: tword1: %d \n",tword1);  */
	putcode_long(tword2, ncodeptr);
	ncodeptr += 2;
	putcode_long(tword1, ncodeptr);
	ncodeptr += 2;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 1;
	while (len > 0)
	{
	    /* printf("n_rdrotor: copy %d\n",*addr);  */
	    *ncodeptr++ = *addr++;
	    len--;
	}
}


n_padly(addr,num)
codeint  *addr;
codeint	 num;
{
/*
 *  { EVENT1_TWRD,      3,      n_scopy,        EVENT1_DELAY },
 *  { EVENT2_TWRD,      5,      n_scopy,        EVENT2_DELAY },
 */
        codeint  tlen;
        codeint  len;
        codeint *count_addr;
        codeint  count;
 
        addr++;
        len = *addr++;
        trTable[num].oldLen = len + 1;
        *ncodeptr++ = trTable[num].newCode;
        count_addr = ncodeptr;
        *ncodeptr++ = 0;
        *ncodeptr++ = ct;
        count = 1; /* for ct */
        len -= 2;  /* skip the STFIFO and SFIFO from old padly */
        while (len > 0)
        {
	   if (*addr == IPACODE)
	   {
	      addr++;
	      addr++;
	      len -= 2;
	   }
           if (*addr == EVENT1_TWRD)
           {
              *ncodeptr++ = EVENT1_DELAY;
              tlen = 3 - 1;
           }
           else
           {
              *ncodeptr++ = EVENT2_DELAY;
              tlen = 5 - 1;
           }
           count++;
           addr++;
           len -= (tlen + 1);
           while (tlen > 0)
           {
               *ncodeptr++ = *addr++;
               count++;
               tlen--;
           }
        }
        *count_addr = count;
}

#define OLD_LOCK_HOLD	1

n_smpl_hold(addr,num)
codeint  *addr;
codeint	 num;
{
	int value;
	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 1;
	value = *addr++;
	if (value == OLD_LOCK_HOLD)
	   *ncodeptr++ = LKHOLD;
	else
	   *ncodeptr++ = LKSAMPLE;
}


n_lktc(addr,num)
codeint  *addr;
codeint	 num;
{
	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 1;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = LOCKACQTC;
	*ncodeptr++ = 2;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = ct;
}

n_lock(addr,num)
codeint  *addr;
codeint	 num;
{
   double   max,min,step;
   int tempgain;

	addr++;
        /* printf("n_alock, offset: %d\n",ncodeptr - newstartptr); */
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = ct;
	*ncodeptr++ = *addr++;
	par_maxminstep(GLOBAL, "lockpower", &max, &min, &step);
	*ncodeptr++ = (short) max;
	par_maxminstep(GLOBAL, "lockgain", &max, &min, &step);
	*ncodeptr++ = (short) max;

	addr++;
}

/* GRADIENT and WAVEFORM GENERATOR */
/* GRADIENT and WAVEFORM GENERATOR */

#define	GRAD20 0x4000

int
n_wgiload(addr,num)
codeint  *addr;
codeint	 num;
{
   ushort apaddr;
   int nvalues;	

	addr++;
	/* Point to address on the waveform generator to start loading */
	apaddr = (*addr++ & 0x0ff8);
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr;
	*ncodeptr++ = *addr++;	/* this was previously only a byte, check!!! */

	/* load data */
	apaddr = apaddr | 0x01;
	nvalues = *addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 3 + (nvalues*2);
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = nvalues*2;
	*ncodeptr++ = apaddr;
	while (nvalues > 0)
	{
	   *ncodeptr++ = *addr++;
	   *ncodeptr++ = *addr++;
	   nvalues--;
	}

}

int
n_wgcmd(addr,num)
codeint  *addr;
codeint	 num;
{
   ushort apaddr;

	addr++;
	apaddr = (*addr++ & 0x0ff8) | 0x0002;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr;
	*ncodeptr++ = *addr++;	/* this was previously only a byte, check!!! */
}

int
n_wg3(addr,num)
codeint  *addr;
codeint	 num;
{
   ushort apaddr;

	addr++;
	apaddr = *addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr;
	*ncodeptr++ = *addr++;
}

int
n_pvgradient(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;
   ushort apaddr;

	addr++;
	apaddr = *addr++;
	/* set instruction address to 0*/
	*ncodeptr++ = APBCOUT;
	*ncodeptr++ = 4;
	*ncodeptr++ = PFG_APBUS_DELAY;  	/* delay */
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr | APBYTF;
	*ncodeptr++ = 0;

	apaddr = apaddr | 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 7;
	*ncodeptr++ = PFG_APBUS_DELAY;
	*ncodeptr++ = (apaddr & 0x0fff) | APBYTF | GRAD20;
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult */
	*ncodeptr++ = *addr++;			/* incr */
	*ncodeptr++ = *addr++;			/* base */
}

int
n_tvgradient(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;
   ushort apaddr;

	addr++;
	apaddr = *addr++;

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 7;
	*ncodeptr++ = PFG_APBUS_DELAY;
	*ncodeptr++ = (apaddr & 0x0fff);
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult */
	*ncodeptr++ = *addr++;			/* incr */
	*ncodeptr++ = *addr++;			/* base */
}

int
n_incpgrad(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;
   ushort apaddr;

	addr++;
	apaddr = *addr++;
	/* set instruction address to 0*/
	*ncodeptr++ = APBCOUT;
	*ncodeptr++ = 4;
	*ncodeptr++ = PFG_APBUS_DELAY;  	/* delay */
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr | APBYTF;
	*ncodeptr++ = 0;

	apaddr = apaddr | 1;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 11;
	*ncodeptr++ = PFG_APBUS_DELAY;
	*ncodeptr++ = (apaddr & 0x0fff) | APBYTF | GRAD20;
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult1 */
	*ncodeptr++ = *addr++;			/* incr1 */
	*ncodeptr++ = *addr++;			/* vmult2 */
	*ncodeptr++ = *addr++;			/* incr2 */
	*ncodeptr++ = *addr++;			/* vmult3 */
	*ncodeptr++ = *addr++;			/* incr3 */
	*ncodeptr++ = *addr++;			/* base */
}

int
n_inctgrad(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;
   ushort apaddr;

	addr++;
	apaddr = *addr++;

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 11;
	*ncodeptr++ = PFG_APBUS_DELAY;
	*ncodeptr++ = (apaddr & 0x0fff);
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult1 */
	*ncodeptr++ = *addr++;			/* incr1 */
	*ncodeptr++ = *addr++;			/* vmult2 */
	*ncodeptr++ = *addr++;			/* incr2 */
	*ncodeptr++ = *addr++;			/* vmult3 */
	*ncodeptr++ = *addr++;			/* incr3 */
	*ncodeptr++ = *addr++;			/* base */
}

int
n_wgd3(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;

	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 7;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = (*addr++ & 0x0fff);
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult */
	*ncodeptr++ = *addr++;			/* incr */
	*ncodeptr++ = *addr++;			/* base */
}

int
n_wgv3(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;
   ushort apaddr,inst_id,vloops;

	addr++;
	apaddr = *addr++;
	inst_id = *addr++;
	vloops = get_acqvar(*addr++);
	vloops = (vloops & 0x00ff) | (inst_id & 0xff00);

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 7;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = (apaddr & 0x0fff);
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult */
	*ncodeptr++ = *addr++;			/* incr */
	*ncodeptr++ = *addr++;			/* base */
	
	/* This assumes that vloops is not changed interactively */
	*ncodeptr++ = APBCOUT;
	*ncodeptr++ = 4;
	*ncodeptr++ = STD_APBUS_DELAY;  	/* delay */
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr;
	*ncodeptr++ = vloops;
}

int
n_wgi3(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;
   ushort apaddr,inst_id,loops;

	addr++;
	apaddr = *addr++;
	inst_id = *addr++;
	loops = *addr++;
	loops = (loops & 0x00ff) | (inst_id & 0xff00);

	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 11;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = (apaddr & 0x0fff);
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult 1 */
	*ncodeptr++ = *addr++;			/* incr 1 */
	*ncodeptr++ = *addr++;			/* vmult 2 */
	*ncodeptr++ = *addr++;			/* incr 2 */
	*ncodeptr++ = *addr++;			/* vmult 3 */
	*ncodeptr++ = *addr++;			/* incr 3 */
	*ncodeptr++ = *addr++;			/* base */
	
	/* set loops */
	*ncodeptr++ = APBCOUT;
	*ncodeptr++ = 4;
	*ncodeptr++ = STD_APBUS_DELAY;  	/* delay */
	*ncodeptr++ = 1;
	*ncodeptr++ = apaddr;
	*ncodeptr++ = loops;
}

int
n_incwgrad(addr,num)
codeint  *addr;
codeint	 num;
{
   int tempgain;

	addr++;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 11;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = (*addr++ & 0x0fff);
	*ncodeptr++ = 32767;			/* maxval */
	*ncodeptr++ = -32768;			/* minval */
	*ncodeptr++ = *addr++;			/* vmult1 */
	*ncodeptr++ = *addr++;			/* incr1 */
	*ncodeptr++ = *addr++;			/* vmult2 */
	*ncodeptr++ = *addr++;			/* incr2 */
	*ncodeptr++ = *addr++;			/* vmult3 */
	*ncodeptr++ = *addr++;			/* incr3 */
	*ncodeptr++ = *addr++;			/* base */
}

/* Global Tables */
int
n_gtabindx(addr,num)
codeint  *addr;
codeint	 num;
{
   int numacodes;

	addr++;		/* acode */
	addr++;		/* global segment index */
    	/* IXPRINT2("GTABINDX: table=%d vindex=%d.\n",*addr,*(addr+2)); */
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 3;
	*ncodeptr++ = *addr++;	/* table number */
	numacodes = *addr++;
	*ncodeptr++ = *addr++;			/* vindex */
	*ncodeptr++ = gindex;			/* temp global tbl ptr */

	if (numacodes > 4)		/* assume frequency setting */
	{
	   *ncodeptr++ = TAPBCOUT;			
	   *ncodeptr++ = 1;	
	   *ncodeptr++ = gindex;	
	}
	else {				/* delay setting */
	   *ncodeptr++ = TDELAY;			
	   *ncodeptr++ = 1;	
	   *ncodeptr++ = gindex;	
	}
}

/* PHASES */

int
n_setphase90(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len,channel,tablenum,flag;

	addr++;
	channel = (*addr & 0x7)-1;
	if ((tablenum=phasetablenum[channel]) < 0)
	{
	   printf("No Phase table for channel %d.\n",channel);
	   psg_abort(1);
	}
	flag = *addr & 0x8;
	if (flag > 0) 
	{
	   len = ( (*addr >> 4) + 7) / 8;
	}
        else
        {
	   len = 1;
        }
	trTable[num].oldLen = 2 + len;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len+3;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = tablenum;
	*ncodeptr++ = ctss;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}

#ifdef XXXX
int
n_setlkdecphase90(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len,channel,tablenum,flag;

	addr++;
	channel = (*addr & 0x7)-1;
	if ((tablenum=phasetablenum[channel]) < 0)
	{
	   printf("No Phase table for channel %d.\n",channel);
	   psg_abort(1);
	}
	flag = *addr & 0x8;
	if (flag > 0) 
	{
	   len = ( (*addr >> 4) + 7) / 8;
	}
        else
        {
	   len = 1;
        }
	trTable[num].oldLen = 2 + len;
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = len+3;
	*ncodeptr++ = *addr++;
	*ncodeptr++ = tablenum;
	*ncodeptr++ = ctss;
	while (len > 0)
	{
	    *ncodeptr++ = *addr++;
	    len--;
	}
}
#endif

int
n_setphase(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len,channel,tablenum,flag;

	addr++;
	channel = (*addr & 0xff)-1;
	flag = (*addr++ >> 8) & 0xff;
	if ((flag & 1) == 1) 
	   *ncodeptr++ = NEW_SETPHASE;
	else
	   *ncodeptr++ = trTable[num].newCode;
	if ((flag & 2) == 0) 
	{
	   /* only use 0.25 degree phase resolution */
	   printf("Only 0.25 degree phase resolution implemented.\n");
	   psg_abort(1);
	}
	len = trTable[num].oldLen-1;
	*ncodeptr++ = len;
	
	if ((tablenum=phasetablenum[channel]) >= 0)
	{
	    *ncodeptr++ = tablenum;
	    *ncodeptr++ = *addr++;
	}
        else
	{
	   printf("No Phase table for channel %d.\n",channel);
	   psg_abort(1);
	}
}

int
n_phasestep(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len,channel,tablenum,flag;

	addr++;
	channel = (*addr++ & 0xff)-1;
	*ncodeptr++ = trTable[num].newCode;
	len = trTable[num].oldLen-1;
	*ncodeptr++ = len;
	
	if ((tablenum=phasetablenum[channel]) >= 0)
	{
	    *ncodeptr++ = tablenum;
	    *ncodeptr++ = *addr++;
	}
        else
	{
	   printf("No Phase table for channel %d.\n",channel);
	   psg_abort(1);
	}
}

int
n_setphattr(addr, num)
codeint  *addr;
codeint	 num;
{
	codeint  len,channel,tablenum,flag,*shortptr;
	int i;
	int *th;
	phasetbl *pphtbl;
        int val;

	addr++;
	channel = (*addr++ & 0xff)-1;
	th = createphasetable(channel);
	th++; /* increment past num_entries */
	th++; /* increment past size_entry */
	th++; /* increment past mod_factor */
	pphtbl = (phasetbl *) th;

	/* set 90 degree lines */
	for (i=0; i<4; i++)
	{
	    shortptr = (short *) &pphtbl->phase90[i];
	    *shortptr++ = htons(*addr++);
	    *shortptr = htons(*addr++);
	}
	shortptr = (short *) &pphtbl->phasebits;
	*shortptr++ = htons(*addr++);
	*shortptr = htons(*addr++);
	    
	pphtbl->phasequad = 0;
	pphtbl->phasestep = 0;
	pphtbl->phaseprecision = htonl(360);	/* always assume 0.25 degree prec */
	val = (*addr++ << 8); /* dev addr */
	val |= *addr++; /* dev register */
	pphtbl->apaddr = htonl(val);
	pphtbl->apdelay = htonl(STD_APBUS_DELAY);
	
}

int
n_cleardata(addr,num)
codeint  *addr;
int      num;
{
	/* Next Scan Opcode */
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 2;
	*ncodeptr++ = dtmcntrl;
	*ncodeptr++ = activercvrs;
}

int
n_seticm(addr,num)
codeint  *addr;
int      num;
{
   extern int rt_tab[];
	/* Set Receiver phase cycle Opcode */
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 3;
	*ncodeptr++ = oph;
	*ncodeptr++ = dtmcntrl;
	*ncodeptr++ = activercvrs;
	if (acqiflag)
	{
#ifdef XXXXX
          if (HS_Dtm_Adc == 0)
          {
	   *ncodeptr++ = DISABLEOVRLDERR;
	   *ncodeptr++ = 2;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
	  }
	  else
	  {
	   *ncodeptr++ = DISABLEHSSTMOVRLDERR;
	   *ncodeptr++ = 1;
	   *ncodeptr++ = dtmcntrl;
          }
#else
	   *ncodeptr++ = DISABLEOVRLDERR;
	   *ncodeptr++ = 3;
	   *ncodeptr++ = dtmcntrl;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
 /*
   printf("n_disableovrflow: len: %d, dtmcntrl: %d->0x%x, adccntrl: %d->0x%x, actrcvr: 0x%x\n",
	 *(ncodeptr-4),dtmcntrl,rt_tab[dtmcntrl],adccntrl,rt_tab[adccntrl],*(ncodeptr-1));
  */
#endif
	}
	else
	{
#ifdef XXXX
          if (HS_Dtm_Adc == 0)
          {
            /* printf("n_seticm: ENABLEOVRLDERR (DTM)\n"); */
	   *ncodeptr++ = ENABLEOVRLDERR;
	   *ncodeptr++ = 4;
	   *ncodeptr++ = ssctr;
	   *ncodeptr++ = ct;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
	  }
          else
          {
            /* printf("n_seticm: ENABLEHSSTMOVRLDERR (HS ADM)\n"); */

	   *ncodeptr++ = ENABLEHSSTMOVRLDERR;
	   *ncodeptr++ = 3;
	   *ncodeptr++ = ssctr;
	   *ncodeptr++ = ct;
	   *ncodeptr++ = dtmcntrl;
          }
#else
	   *ncodeptr++ = ENABLEOVRLDERR;
	   *ncodeptr++ = 5;
	   *ncodeptr++ = ssctr;
	   *ncodeptr++ = ct;
	   *ncodeptr++ = dtmcntrl;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
/*
	printf("n_enableovrflow: len: %d, ss: %d, ct: %d, dtmcntrl: 0x%x, adccntrl=0x%x,actrcvrs: 0x%x\n",
	 *(ncodeptr-6),*(ncodeptr-5),*(ncodeptr-4),
	 *(ncodeptr-3),rt_tab[dtmcntrl], 
	 *(ncodeptr-2),*(ncodeptr-1));
*/
#endif
	}
}


/*
    enable ADC & Receiver OverLoad, test for standard 500KHz ADC
    and 5MHz STM/ADC (only ADC overload), then use appropriate acode
*/
int
n_enableovrflow(addr,num)
codeint  *addr;
int      num;
{
   extern int rt_tab[];
#ifdef XXXX
   if (HS_Dtm_Adc == 0)
   {
            /* printf("n_seticm: ENABLEOVRLDERR (DTM)\n"); */
	   *ncodeptr++ = ENABLEOVRLDERR;
	   *ncodeptr++ = 4;
	   *ncodeptr++ = ssctr;
	   *ncodeptr++ = ct;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
   }
   else
   {
            /* printf("n_seticm: ENABLEHSSTMOVRLDERR (HS ADM)\n"); */

	   *ncodeptr++ = ENABLEHSSTMOVRLDERR;
	   *ncodeptr++ = 3;
	   *ncodeptr++ = ssctr;
	   *ncodeptr++ = ct;
	   *ncodeptr++ = dtmcntrl;
   }
#else
	   *ncodeptr++ = ENABLEOVRLDERR;
	   *ncodeptr++ = 5;
	   *ncodeptr++ = ssctr;
	   *ncodeptr++ = ct;
	   *ncodeptr++ = dtmcntrl;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
/*
	printf("n_enableovrflow: len: %d, ss: %d, ct: %d, dtmcntrl: 0x%x, adccntrl=0x%x,actrcvrs: 0x%x\n",
	 *(ncodeptr-6),*(ncodeptr-5),*(ncodeptr-4),
	 *(ncodeptr-3),rt_tab[dtmcntrl], 
	 *(ncodeptr-2),*(ncodeptr-1));
*/
#endif
	/* printf("n_enableovrflow: Acode: %d, New Acode: %d, len: %d, ss: %d, ct: %d, cntrl: %d\n",
	 *addr, *(ncodeptr-5),*(ncodeptr-4),*(ncodeptr-3),*(ncodeptr-2),*(ncodeptr-1));
	*/
   addr = addr + 5;
   return(0);
}

/*
    disable ADC & Receiver OverLoad, test for standard 500KHz ADC
    and 5MHz STM/ADC (only ADC overload), then use appropriate acode
*/
int
n_disableovrflow(addr,num)
codeint  *addr;
int      num;
{

   extern int rt_tab[];
#ifdef XXXX
   if (HS_Dtm_Adc == 0)
   {
	   *ncodeptr++ = DISABLEOVRLDERR;
	   *ncodeptr++ = 2;
	   *ncodeptr++ = adccntrl;
	   *ncodeptr++ = activercvrs;
   }
   else
   {
	   *ncodeptr++ = DISABLEHSSTMOVRLDERR;
	   *ncodeptr++ = 1;
	   *ncodeptr++ = dtmcntrl;
   }
#else
   *ncodeptr++ = DISABLEOVRLDERR;
   *ncodeptr++ = 3;
   *ncodeptr++ = dtmcntrl;
   *ncodeptr++ = adccntrl;
   *ncodeptr++ = activercvrs;
/*
   printf("n_disableovrflow: len: %d, dtmcntrl: %d->0x%x, adccntrl: %d->0x%x, actrcvr: 0x%x\n",
	 *(ncodeptr-4),dtmcntrl,rt_tab[dtmcntrl],adccntrl,rt_tab[adccntrl],*(ncodeptr-1));
*/
	 /* *(ncodeptr-3),*(ncodeptr-2), *(ncodeptr-1)); */
#endif
/*
   printf("n_disableovrflow: Acode: %d, New Acode: %d, len: %d, cntrl: %d\n",
	 *addr, *(ncodeptr-3),*(ncodeptr-2), *(ncodeptr-1));
*/
   addr = addr + 3;
   return(0);
}


int
n_nsc(addr,num)
codeint  *addr;
int      num;
{
   extern int rt_tab[];

	if (num_nsc < 12)
	{
	   old_nsc_branch[num_nsc] = addr - oldstartptr;
	   new_nsc_branch[num_nsc] = ncodeptr - newstartptr;
	   num_nsc++;
	}
	else
	{
	   printf("too many NSC code\n");
	   psg_abort(1);
	}

/*
	dsp_off_params.flags = 0;
        dsp_off_params.rt_oversamp = 1;
*/
	/* phase cycling */
	*ncodeptr++ = BRA_EQ;
	*ncodeptr++ = 4;
	*ncodeptr++ = rtcpflag;
	*ncodeptr++ = one;
	*ncodeptr++ = 0;
	*ncodeptr++ = 7;
	*ncodeptr++ = RT2OP;
	*ncodeptr++ = 3;
	*ncodeptr++ = MOD4;
	*ncodeptr++ = ctss;
	*ncodeptr++ = oph;

	/* Next Scan Opcode */
        if (rcvr2nt)
        {
	   *ncodeptr++ = trTable[num].newCode + 20;
	   *ncodeptr++ = 9;
	   *ncodeptr++ = rcvr2nt;
        }
        else
        {
	   *ncodeptr++ = trTable[num].newCode;
	   *ncodeptr++ = 8;
        }
	*ncodeptr++ = npr_ptr;
	*ncodeptr++ = dpfrt; /* dp index */
	*ncodeptr++ = ntrt;
	*ncodeptr++ = ct;
	*ncodeptr++ = bsval;
	*ncodeptr++ = dtmcntrl;
	*ncodeptr++ = fidctr;
	*ncodeptr++ = activercvrs;

	/* Clear data at blocksize, if requested */
	*ncodeptr++ = BRA_EQ;
	*ncodeptr++ = 4;
	*ncodeptr++ = bsval;	/* Skip if bsval is 0 */
	*ncodeptr++ = zero;
	*ncodeptr++ = 0;
	*ncodeptr++ = 24;
	*ncodeptr++ = BRA_EQ;
	*ncodeptr++ = 4;
	*ncodeptr++ = clrbsflag; /* Skip if clrbsflag is false */
	*ncodeptr++ = zero;
	*ncodeptr++ = 0;
	*ncodeptr++ = 18;
	*ncodeptr++ = RT3OP;
	*ncodeptr++ = 4;
	*ncodeptr++ = MOD;
	*ncodeptr++ = ct;
	*ncodeptr++ = bsval;
	*ncodeptr++ = tmprt;	/* tmprt = ct % bsval */
	*ncodeptr++ = BRA_NE;
	*ncodeptr++ = 4;
	*ncodeptr++ = tmprt;	/* Skip if we're not at blocksize */
	*ncodeptr++ = zero;
	*ncodeptr++ = 0;
	*ncodeptr++ = 6;
	*ncodeptr++ = CLEARSCANDATA;
	*ncodeptr++ = 2;	/* Passed all tests - clear data */
	*ncodeptr++ = dtmcntrl;
	*ncodeptr++ = activercvrs;

	/* orgHSdata = (unsigned int) get_acqvar(HSlines_ptr); */

        /* Set DSP hardware */
        /* fprintf(stdout,"seting dsp, activercvrs: 0x%hx\n",rt_tab[activercvrs+TABOFFSET]); */
        if (rcvrActive(0))
        {
           /* fprintf(stdout,"1st rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xe84);
        }
	else
        {
           /* fprintf(stdout,"1st rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xe84);
        }

        if (rcvrActive(1))
        {
           /* fprintf(stdout,"2nd rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xea4);
        }
        else
        {
           /* fprintf(stdout,"2nd rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xea4);
        }

        if (rcvrActive(2))
        {
           /* fprintf(stdout,"3rd rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xec4);
        }
        else
        {
           /* fprintf(stdout,"3rd rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xec4);
        }

        if (rcvrActive(3))
        {
           /* fprintf(stdout,"4th rcvr active\n"); */
	   set_dsp(&ncodeptr, &dsp_params, 0xee4);
        }
        else
        {
           /* fprintf(stdout,"4th rcvr off\n"); */
	   set_dsp(&ncodeptr, &dsp_off_params, 0xee4);
        }

	/* avoid dsp setup time for very short experiments... */
        *ncodeptr++ = EVENT1_DELAY;
        *ncodeptr++ = 2;
        *ncodeptr++ = (codeint) 0;
        *ncodeptr++ = (codeint) 2400;	/* 30 usec dsp settling delay */
	return(0);
}

int
n_nextcodeset(addr,num)
codeint  *addr;
int      num;
{
	*ncodeptr++ = RTOP;
	*ncodeptr++ = 2;
	*ncodeptr++ = INC;
	*ncodeptr++ = fidctr;
	*ncodeptr++ = ACQI_UPDT;
	*ncodeptr++ = 2;
	*ncodeptr++ = acqiflagrt;
	*ncodeptr++ = relaxdelayrt;
 	*ncodeptr++ = JMP_NEQ;
	*ncodeptr++ = 4;
	*ncodeptr++ = acqiflagrt;
	*ncodeptr++ = zero;
	putcode_long(new_init_branch, ncodeptr);
	ncodeptr += 2;

	/* If last array element, signal experiment complete */
	if (ix >= get_acqvar(arraydimrt))
	{
	    if ( getIlFlag() )
	    {
	    	*ncodeptr++ = BRA_LT;
	    	*ncodeptr++ = 4;
	    	*ncodeptr++ = ct;
	    	*ncodeptr++ = ntrt;
	    	*ncodeptr++ = 0;
	   	*ncodeptr++ = 5;
	    }
	    *ncodeptr++ = SIGNAL_COMPLETION;
	    *ncodeptr++ = 1;
	    *ncodeptr++ = EXP_COMPLETE;
	    
	}
	*ncodeptr++ = trTable[num].newCode;
	*ncodeptr++ = 4;
	*ncodeptr++ = ilflagrt;
	*ncodeptr++ = bsctr; 
	*ncodeptr++ = ntrt;
	*ncodeptr++ = ct;
	return(0);
}

int
n_ipacode(addr, num)
codeint  *addr;
codeint	 num;
{
 codeint oldaddr,newaddr;
	newaddr = ncodeptr - newstartptr;
	addr++;
	oldaddr = *addr++;
#ifdef DOIPA
	change_ipalocation(oldaddr,newaddr);
#endif
}

n_lock_seq(addr, num)
codeint  *addr;
codeint	 num;
{
    /*IXPRINT2("n_lock_seq(%d, %d)\n", *addr, num);/*CMP*/

    /* Lock FID is upside down; this negates it */
    *ncodeptr++ = RT2OP;
    *ncodeptr++ = 3;
    *ncodeptr++ = SET;
    *ncodeptr++ = two;
    *ncodeptr++ = oph;

    *ncodeptr++ = BRA_NE;
    *ncodeptr++ = 4;
    *ncodeptr++ = ct;
    *ncodeptr++ = zero;
    *ncodeptr++ = 0;
    *ncodeptr++ = 8;
    {
	*ncodeptr++ = APBCOUT;
	*ncodeptr++ = 4;
	*ncodeptr++ = STD_APBUS_DELAY;
	*ncodeptr++ = 1;
	*ncodeptr++ = 0x0b51 | 0x1000;
	*ncodeptr++ = 0x1a << 8;	/* 0x1a = 50 ms between lock pulses */
    }
    *ncodeptr++ = EVENT1_DELAY;
    *ncodeptr++ = 2;
    *ncodeptr++ = (codeint) 0xf4;	/* 200 ms delay */
    *ncodeptr++ = (codeint) 0x2400;

    *ncodeptr++ = FIFOHALT;
    *ncodeptr++ = 0;
    *ncodeptr++ = FIFOSTARTSYNC;
    *ncodeptr++ = 0;
    *ncodeptr++ = FIFOWAIT4STOP_2;
    *ncodeptr++ = 0;

    *ncodeptr++ = EVENT1_DELAY;
    *ncodeptr++ = 2;
    *ncodeptr++ = (codeint) 0x39;	/* 47 ms delay */
    *ncodeptr++ = (codeint) 0x5f80;

    return 0;
}

static int *createphasetable(codeint channeldev)
{
   int tblnum;
   tblhdr *th;
  
   /* IXPRINT1("createphasetable dev: %d.\n",channeldev); */
   tblnum = createtable( sizeof(tblhdr)+sizeof(phasetbl) );
   th = (tblhdr *) tblptr[tblnum];
   th->num_entries = sizeof(phasetbl)/sizeof(int);
   th->size_entry = sizeof(int);
   th->mod_factor = 1;
   phasetablenum[channeldev] = tblnum;
   return((int *)tblptr[tblnum]);
}

int 
createglobaltable(num_entries, entry_size, data_entry)
int num_entries;
int entry_size;			/* in bytes */
char *data_entry;
{
   int tblnum,tsize,i,j;
   tblhdr *th;
   int *tempptr;
   codeint *tmpdata;
  
   /* IXPRINT2("createglobaltable entries: %d size: %d.\n",num_entries, */
   /* 							entry_size); */
   tsize = sizeof(tblhdr)+(num_entries*entry_size);
   tblnum = createtable( tsize );
   th = (tblhdr *) tblptr[tblnum];
   th->num_entries = num_entries;
   th->size_entry = entry_size;
   th->mod_factor = 1;
   tempptr = (int *)tblptr[tblnum];
   tempptr++; /* increment past num_entries */
   tempptr++; /* increment past size_entry */
   tempptr++; /* increment past mod_factor */
   memcpy( (char *)tempptr, data_entry, num_entries*entry_size);
   /* tmpdata = data_entry; */
   /* for (i=0; i<num_entries; i++) */
   /*	for (j=0; j<(entry_size/2); j++) */
   /*		printf("createglobaltable data = 0x%x\n",*tmpdata++); */
   return(tblnum);
}

static int createtable(int size)
{
   /* IXPRINT2("createtable num: %d size: %d.\n",num_tables,size); */
   tblptr[num_tables] = malloc( size );
   if (tblptr[num_tables] == NULL)
   {
	printf("Could not create table. size: %d\n",size);
	psg_abort(1);
   }

   num_tables++;

   if (num_tables > MAXTABLES)
   {
	printf("Max number of tables created.\n");
	psg_abort(1);
   }

   /* returns number of table created */
   return(num_tables-1);
}

calcheader(headerbuf)
int *headerbuf;
{
    int offset,i,size;
    tblhdr *th;

    offset = (num_tables+1)*sizeof(int);
    *headerbuf++ = num_tables;
    *headerbuf++ = offset;
    for (i=0; i<(num_tables-1); i++)
    {
	th = (tblhdr *) tblptr[i];
	size = sizeof(tblhdr)+(th->num_entries*th->size_entry);
	offset = offset+size;
    	*headerbuf++ = offset;
    }
}

writetablefile(tfilename)
char *tfilename;
{
    int Tblfd;	/* file discriptor Code disk file */
    int bytes,i,size,hdrsize;
    int *tblheader;
    int *tPtr;
    tblhdr *th;

    Tblfd = open(tfilename,O_EXCL | O_WRONLY | O_CREAT,0666);
    if (Tblfd < 0)
    {	text_error("Exp table file already exists. PSG Aborted..\n");
   	   psg_abort(1);
    }

    hdrsize = (num_tables+1)*sizeof(int);
    tblheader = (int *)malloc(hdrsize);
    if (tblheader == NULL)
    {	text_error("Unable to allocate memory for tblheader. PSG Aborted..\n");
   	   psg_abort(1);
    }
    
    calcheader(tblheader);
    bytes = write(Tblfd, tblheader, hdrsize );
    /* IXPRINT1("Tbl Header: Bytes written to file: %d (bytes).\n",bytes); */
    free(tblheader);
    
    for (i=0; i<num_tables; i++)
    {
	th = (tblhdr *) tblptr[i];
	size = sizeof(tblhdr)+(th->num_entries*th->size_entry);
#ifdef LINUX
        tPtr = tblptr[i];
        *tPtr = htonl( *tPtr );
        tPtr++; /* increment past num_entries */
        *tPtr = htonl( *tPtr );
        tPtr++; /* increment past size_entry */
        *tPtr = htonl( *tPtr );
        tPtr++; /* increment past mod_factor */
#endif
	bytes = write(Tblfd, tblptr[i], size );
    	/* IXPRINT2("Tbl: %d Bytes written to file: %d (bytes).\n",i,bytes); */
    }
    close(Tblfd);
}

void initorgHSlines(hslines)
int hslines;
{
	orgHSdata = (unsigned int) hslines;
}


cleanupaptableinfo()
{
	codeint  len, loc;
	int	 pindex;

	if (num_aptables == 0) return;

	for (pindex = 0; pindex < num_aptables; pindex++)
	{
	     free(apTable[pindex]);
	}
}
