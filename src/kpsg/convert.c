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

/************************************************/
/* This is the Mercury-Vx Version of convert.c  */
/************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "REV_NUMS.h"
#include "acodes.h"
#include "group.h"
#include "acqparms2.h"
#include "aptable.h"
#include "expDoneCodes.h"
#include "abort.h"


#define MAXTABLES	20
#define MAXAPTABLES	80
#define ACODE_BUFFER    (21839)

#define NOBSLOCK	151
#define APBCOUT         30
#define TAPBCOUT        31
#define APTABLE         114
#define APTASSIGN       115
#define TDELAY		7
#define	LOADHSL		11
#define	MASKHSL		12
#define	UNMASKHSL	13
#define PAD_DELAY       17
#define	SAFEHSL		18
#define ACQUIRE         40
#define BEGINHWLOOP     42
#define ENDHWLOOP       43
#define RECEIVERCYCLE	44
#define WGLOAD		(67)

#define RTOP            200
#define INC             2
#define DEC             3
#define RT2OP           201
#define SET             20
#define MOD2            21
#define MOD4            22
#define HLV             23
#define DBL             24
#define RT3OP           202
#define ADD             30
#define SUB             31
#define MUL             32
#define DIV             33
#define MOD             34
#define OR              35
#define AND             36

#define ENDIF		161
#define	BRA_EQ		(221)
#define BRA_LT		(223)
#define BRA_NE		(224)
#define JUMP            230
#define JMP_LT          231
#define JMP_NEQ         232
#define JMP_MOD2        233
#define	INIT_STM	90
#define	INIT_FIFO	91
#define	INTERP_REV_CHK	93
#define RTINIT          203
#define INITSCAN        300
#define NEXTSCAN        301
#define ENDOFSCAN       302
#define NEXTCODESET     303
#define ACQI_UPDT	(304)
#define SIGNAL_COMPLETION (306)
#define SYNC_PARSER	(308)
#define STATBLOCK_UPDT	(307)
#define NOISE_CMPLT     (309)
#define TSETPTR         21
#define LOCKPHASE_I     (404)
#define LOCKPOWER_I     (405)
#define LOCKGAIN_I      (406)
#define LOCKAUTO        (412)
#define SHIMAUTO	(413)
#define LOCKZ0_I	(414)
#define SAVELOCKPH	(421)
#define	NEVENTN		(450)

#define LOCKMODE_I      (415)
#define	OLD_LOCK_HOLD	(1)
#define LKHOLD		12
#define LKSAMPLE	13
#define SETPHASE90_R    (51)
#define NEW_PHASESTEP   (52)    /* New Acode define is same as old */
#define NEW_SETPHASE	(53)	/* New Acode define is same as old */
#define SETPHASE_R      (54)
#define RECEIVERGAIN    (70)
#define TUNESTART	(82)
#define CLEARSCANDATA   (83)

#define INIT_ADC	(92)

#define OBSOLETE  888

#define LOCKCHECK       (416)
#define SETSPN         (505)
#define CHECKSPN       (507)

extern int	acqiflag;
extern int      fidscanflag;
extern int	hb_dds_bits;
extern int	rtinit_count;
extern int	shimset;
extern int	traymax;
extern int      prepScan;	/* used to decide if Acode sync is to be used */
extern int	whenshim;

extern codeint  clrbsflag;
extern codeint  dpfrt,npnoise, nfidbuf, /* HS_Dtm_Adc, */ adccntrl,rtrecgain;
extern codeint	acqiflagrt;
extern codeint	arraydimrt;
extern codeint	dtmcntrl;
extern codeint	gindex;
extern codeint	ilflagrt;
extern codeint	maxsum;
extern codeint	relaxdelayrt;
extern codeint	rtcpflag;
extern codeint	tmprt;

extern codeint	ntrt, strt;

extern unsigned long	ix;

typedef struct _apTbl {
                codeint oldacodeloc;
                codeint newacodeloc;
                codeint auto_inc;
        } apTbl;

typedef struct _phasetbl {
	long	phase90[4];
	long	phasebits;
	long	phasequad;
	long	phasestep;
	long	phaseprecision;
	long	apaddr;
	long	apdelay;
} phasetbl;

typedef struct _tblhdr {
		int	num_entries;
		int	size_entry;
		int	mod_factor;
	} tblhdr;

typedef struct _trBlk {
	codeint   oldCode;
	codeint   oldLen;
	int       (*func)();
	codeint   newCode;
} trBlk;

/* Table information */
static int num_aptables = 0;
static apTbl *apTable[MAXAPTABLES];

int	n_again(),	n_apbout(),	n_begin(),	n_branch();
int	n_branch_lt(),	n_check(),	n_copy(),	n_cpsamp();
int	n_endif(),	n_endloop(),	n_endhwloop(),	n_eventn();
int	n_fidcode(),	n_gain(),	n_gtabindx();
int	n_hkeep(),	n_hsline(),	n_hwloop();
int	n_ifminus(),	n_ifnz(),	n_ifnzb(),	n_init();
int	n_lksync(),	n_loadshim(),	n_lock(),	n_nextcodeset();
int	n_noise(),	n_nsc(),	n_padly(),	n_phasestep();
int	n_rtinit(),	n_rtop(),	n_rt2op(),	n_rt3op();
int	n_scopy(),	n_seticm(),	n_setphase(),	n_setphase90();
int	n_setphattr(), 	n_setup();
int	n_shim(),	n_smpl_hold(),	n_table(),	n_tassign();

trBlk  trTable[] = {
	{ NO_OP,	1,	NULL,		OBSOLETE },
	{ CBEGIN,	1,	n_begin,	0 },
	{ EXIT,		1,	n_nextcodeset,	NEXTCODESET },
	{ STOP,		1,	NULL,		OBSOLETE },
	{ HALT,		1,	NULL,		0 },
	{ CLEAR,	1,	NULL,		OBSOLETE },
	{ APBOUT,	0,	n_apbout,	APBCOUT },
	{ STFIFO,	1,	NULL,		OBSOLETE },
	{ RFIFO,	1,	NULL,		OBSOLETE },
	{ SFIFO,	1,	NULL,		OBSOLETE },
	{ WTFRVT,	3,	n_scopy,	501 },
	{ CKVTR,	2,	NULL,		OBSOLETE },
	{ CKLOCK,	2,	n_scopy,	LOCKCHECK },
	{ TXPHAS,	1,	NULL,		OBSOLETE },
	{ SETPHAS90,	3,	n_setphase90,	SETPHASE90_R },
	{ SETOPH,	1,	NULL,		OBSOLETE },
	{ INIT,		1,	n_init,		INITSCAN },
	{ BRANCH,	2,	n_branch,	JUMP },
	{ DECP,		2,	NULL,		OBSOLETE },
	{ INCRFUNC,	2,	n_rtop,		INC },
	{ DECRFUNC,	2,	n_rtop,		DEC },
	{ ADDFUNC,	4,	n_rt3op,	ADD },
	{ SUBFUNC,	4,	n_rt3op,	SUB },
	{ MULTFUNC,	4,	n_rt3op,	MUL },
	{ DIVFUNC,	4,	n_rt3op,	DIV },
	{ DBLFUNC,	3,	n_rt2op,	DBL },
	{ HLVFUNC,	3,	n_rt2op,	HLV },
	{ MODFUNC,	4,	n_rt3op,	MOD },
	{ MOD2FUNC,	3,	n_rt2op,	MOD2 },
	{ MOD4FUNC,	3,	n_rt2op,	MOD4 },
	{ ORFUNC,	4,	n_rt3op,	OR },
	{ ASSIGNFUNC,	3,	n_rt2op,	SET },
	{ IFZFUNC,	4,	n_branch_lt,	JMP_LT },
	{ IFNZFUNC,	4,	n_ifnz,		JMP_NEQ },
	{ IFMOD2ZERO,	4,	n_ifnz,		JMP_MOD2 },
	{ IFNZBFUNC,	4,	n_ifnzb,	JMP_NEQ },
	{ ENDIF,        0,      n_endif,        0 },
	{ IFMIFUNC,	4,	n_ifminus,	JMP_LT },
	{ ENDLOOP,	4,	n_endloop,	JMP_LT },
	{ NOISE,	6,	n_noise,	ACQUIRE },
	{ acqstart,	1,	NULL,		0 },
	{ acqend,	1,	NULL,		0 },
	{ EVENT1,	3,	NULL,		0 },
	{ EVENT2,	4,	NULL,		0 },
	{ initstm,	1,	NULL,		0 },
	{ autostm,	1,	NULL,		0 },
	{ setipc,	1,	NULL,		0 },
	{ setopc,	1,	NULL,		0 },
	{ LOADF,	4,	n_rtinit,	RTINIT },
	{ PFLCNT,	1,	NULL,		OBSOLETE },
	{ APCOUT,	2,	n_check,	APBCOUT },
	{ CALL,		2,	NULL,		0 },
	{ SETVT,	6,	n_scopy,	500 },
	{ SETPHASE,	3,	n_setphase,	SETPHASE_R },
	{ HWLOOP,	4,	n_hwloop,	BEGINHWLOOP },
	{ EHWLOOP,      1,      n_endhwloop,    ENDHWLOOP },
	{ PHASESTEP,	4,	n_phasestep,	NEW_PHASESTEP },
	{ SPINA,	4,	n_scopy,	SETSPN },
	{ GAIN,		2,	n_gain,		RECEIVERGAIN },
	{ LOCKA,	4,	n_lock,		LOCKAUTO },
	{ DECUP,	3,	NULL,		0 },
	{ CHKSPIN,	4,	n_scopy,	CHECKSPN },
	{ SHIMA,	0,	n_shim,		SHIMAUTO },   /* 0 in INOVA */
	{ AUTOGAIN,	1,	n_again,	AUTOGAIN },
	{ GETSAMP,	1,	n_cpsamp,	GETSAMP },
	{ LOADSAMP,	2,	n_cpsamp,	LOADSAMP },
	{ LOADSHIM,	48,	n_loadshim,	LOADSHIM },
	{ gradient,	1,	NULL,		OBSOLETE },
	{ RCVR_CNTL,	1,	NULL,		0 },
	{ EVENTN,	0,	n_eventn,	NEVENTN },
	{ wtfrlk,	1,	NULL,		OBSOLETE },
	{ lkdisp,	1,	NULL,		0 },
	{ GTABINDX, 	5,	n_gtabindx,	TSETPTR },
	{ PADLY,	2,	n_padly,	PAD_DELAY },  /* ?? */
	{ SETUP,	2,	n_setup,	SIGNAL_COMPLETION },
	{ SETICM,       1,      n_seticm,       RECEIVERCYCLE },
	{ HKEEP,	1,	n_hkeep,	ENDOFSCAN },
	{ SAVELKPH,	3,	n_scopy,	SAVELOCKPH },
	{ NSC,		1,	n_nsc,		NEXTSCAN },
	{ ACQXX,	6,	n_scopy,	ACQUIRE },
	{ switchend,	1,	NULL,		0 },
	{ LK_SYNC,	0,	n_lksync,	APBCOUT },
	{ TABLE,	0,	n_table,	APTABLE },
	{ TASSIGN,	0,	n_tassign,	APTASSIGN },
	{ SYNCST,	1,	NULL,		0 },
	{ SMPL_HOLD,	2,	n_smpl_hold,	LOCKMODE_I },
	{ SET_GR_RELAY,	2,	n_scopy,	SET_GR_RELAY },
	{ WGGLOAD,	1,	n_scopy,	WGLOAD },
	{ FIDCODE,      1,	n_fidcode,	0 },
	{ IMASKON,	3,	n_scopy,	MASKHSL },
	{ IMASKOFF,	3,	n_scopy,	UNMASKHSL },
	{ HSLINES,	3,	n_hsline,	LOADHSL },
	{ INITHSL,	3,	n_scopy,	LOADHSL },
	{ ISAFEHSL,	3,	n_scopy,	SAFEHSL },
	{ SETPHATTR,	14,     n_setphattr,	NEW_PHASESTEP },
	{ NNOISE,	3,      n_copy,		LOCKPOWER_I },
	{ NEXACQT,	3,      n_copy,		LOCKGAIN_I },
	{ NACQXX,	3,      n_copy,		LOCKPHASE_I },
	{ XSAPBIO, 	4,	n_copy,		LOCKZ0_I },
	{ TUNE_START,	3,	n_scopy,	TUNESTART },
	{ OBSLOCK,	1,	n_scopy,	NOBSLOCK },
	{ XGATE,	1,	n_scopy,	XGATE },
};

#define	LASTCODE	sizeof(trTable)/sizeof(trBlk)

/* end of op code table */

/* Table information */
static codeint	*autogainptr = NULL;
static codeint	*autoshimptr = NULL;
static codeint	*hwloopptr;
static codeint	*oldstartptr, *ncodeptr;
static codeint  *newstartptr = NULL;
static codeint	*rtinit_ptr;
static codeint	stack_short[20];

static unsigned int	stack_long[62];
static int		lstackptr = 0;
static int		stackptr = 0;
static unsigned int	stack_long[62];
static codeint		old_nsc_branch[12];
static unsigned int	new_nsc_branch[12];
static int		num_nsc;

static int phasetablenum[4] = { -1, -1, -1, -1};

static int	forceHSlines = 0;
static int	newAcodeSize = 0;
       int	num_tables = 0;
static int	*tblptr[MAXTABLES];

static unsigned int	new_init_branch;

static union {
         unsigned int ival;
         codeint cval[2];
      } endianSwap;

#define IXPRINT0(str)           if (ix == 1) printf(str)
#define IXPRINT1(str,arg1)      if (ix == 1) printf(str,arg1)
#define IXPRINT2(str,arg1,arg2) if (ix == 1) printf(str,arg1,arg2)

codeint
*convert_Acodes(oldstart, oldlast, retsize)
codeint  *oldstart, *oldlast;
int      *retsize;
{
int		num, size;
codeint		retLen;
codeint		*cptr;
static codeint	*mallocptr = NULL;
static int 	old_size = 0;


   size = oldlast - oldstart;
   cptr = oldstart;
   oldstartptr = oldstart;
   if (size <= 0)
   {
      abort_message("psg: the size of acode is zero\n");
   }
   if (newAcodeSize < size * 4)
   {
      if (mallocptr != NULL)
         free(mallocptr);
      newAcodeSize = size * 4;
      mallocptr = (codeint *) malloc(newAcodeSize + 2 * sizeof(codelong));
      if (mallocptr == NULL)
      {
         abort_message("psg: do not have enough memory\n");
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
      if (*cptr < 0 || *cptr > LASTACODE)
      {
         IXPRINT2("psg: unknown code '%d' addr is %d\n", *cptr, cptr);
         cptr++;
         continue;
      }
      num = 0;
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
             if (num == LASTCODE)
             {
                IXPRINT2("psg: unknown code '%d'  addr is %d\n", *cptr, cptr);
                cptr++;
                break;
            }
         }
         num++;
      }
   }
   *retsize = ((long)ncodeptr - (long)newstartptr);
/*   cleanupaptableinfo(); */
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
       abort_message("Too many nested loops or if statements\n");
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
       abort_message("missing loop() or ifzero()\n");
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
       abort_message("Too many nested loops or if statements\n");
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
       abort_message("missing loop() or ifzero()\n");
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

copy(data,num)
codeint	*data;
codeint	num;
{
   *ncodeptr++ = num;
   data++;
   while (num > 0)
   {
      *ncodeptr++ = *data++;
      num--;
   }
   return(0);

}

n_begin(addr,num)
codeint	*addr;
int	num;
{
   *ncodeptr++ = INTERP_REV_CHK;
   *ncodeptr++ = 2;
   *ncodeptr++ = MERCURY_INTERP_REV;
   *ncodeptr++ = 0;

   *ncodeptr++ = INIT_FIFO;
   *ncodeptr++ = 2;
/*   if (debug2)
/*      *ncodeptr++ = 1;		/* write to file */
/*   else */
      *ncodeptr++ = 0;
   *ncodeptr++ = 0;

   /* Added new fourth arg: HS DTM/ADC (5MHz DTM/ADC) flag 4/23/96 GMB */
   *ncodeptr++ = INIT_STM;
   *ncodeptr++ = 4;
   *ncodeptr++ = dpfrt;		/* dp float pointer */
   if ( get_acqvar(npnoise) > get_acqvar(npr_ptr) )
      *ncodeptr++ = npnoise;	/* bytes of data point */
   else
      *ncodeptr++ = npr_ptr;	/* bytes of data point */
   *ncodeptr++ = nfidbuf;	/* number of buffers */
   *ncodeptr++ = 0;		/* HS_Dtm_Adc If true select the 5MHz STM/ADC */

   *ncodeptr++ = INIT_ADC;
   *ncodeptr++ = 3;
   /* Select channel, interrupts, etc.. */
   *ncodeptr++ = 0;		/* HS_Dtm_Adc */
				/* if using HS DTM, set flag 1(disable ADC) */
				/* now set in lc_hdl.c */
   *ncodeptr++ = adccntrl;	/* adcntrl rt parm  */
   *ncodeptr++ = ssval;		/* iff ssval=0, enable ADC overflow */

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

n_again(addr,num)
codeint	*addr;
int	num;
{
int   len;
double   max,min,step;

   par_maxminstep(CURRENT, "gain", &max, &min, &step);

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
   *ncodeptr++ = 0xa28;   /* Change Me then change the one in n_gain */
   *ncodeptr++ = 0;  /* dito */
   *ncodeptr++ = rtrecgain;
   *ncodeptr++ = (short) max;
   *ncodeptr++ = (short) min;
   *ncodeptr++ = (short) step;
   *ncodeptr++ = 16;
    autoshimptr = ncodeptr;   /* to be patch later via FIDCODE */
    /* printf("n_again: autoshimptr: %d\n",autoshimptr); */
   *ncodeptr++ = 0; /* offset into Acodes to obain FID */
   return(0);
}

n_apbout(addr,num)
codeint	*addr;
int	num;
{
  *ncodeptr++ = trTable[num].newCode;
  addr++;
  trTable[num].oldLen = (*addr) + 3;	/* 1 for acode, 1 for count, count=#-1*/
  *ncodeptr++ = (*addr) + 2;		/* do NOT include acode for A_interp */
  copy(addr,(*addr)+1);
}

n_branch(addr,num)
codeint	*addr;
int	num;
{
int		n;
codeint		*tptr, jump;
unsigned int	ifOffset, elseOffset, offset;

   addr++;
   tptr = addr;
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

n_branch_lt(addr,num)
codeint	*addr;
int	num;
{
int	n;
codeint	*tptr, jump;

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

n_copy(data,num)
codeint	*data;
codeint	num;
{
codeint   len;
   len = trTable[num].oldLen - 1;
   *ncodeptr++ = trTable[num].newCode;
   data++;
   while (len > 0)
   {
      *ncodeptr++ = *data++;
      len--;
   }
   return(0);
}

n_cpsamp(addr,num)
codeint	*addr;
int	num;
{
   int   len;
   int   robotype;

   len = trTable[num].oldLen - 1;
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 3 + len;			/* 1 long == 2 shorts */
   addr++;				/* skip over old acode */
   *ncodeptr++ = loc & 0xffff;
   *ncodeptr++ = (((unsigned int)loc) & 0xffff0000) >> 16;

   /* if Gilson or NanoProbe changer then skip sample detection step */
   robotype = ((traymax == 48) || (traymax == 96)) ? 1 : 0;
   if ( (traymax == 12) || (traymax == 97) )
      robotype = 2;
   *ncodeptr++ = robotype;
   while (len > 0)
   {
      *ncodeptr++ = *addr++;
      len--;
   }
}

int
n_endif(addr, num)
codeint  *addr;
codeint  num;
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
n_endhwloop(addr, num)
codeint  *addr;
int      num;
{
        unsigned int  offset;
        *ncodeptr++ = trTable[num].newCode;
        addr++;
        *ncodeptr++ = 0;
        offset = ncodeptr - hwloopptr;
        putcode_long(offset, hwloopptr);

        return(0);
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
n_eventn(addr,num)
codeint	*addr;
codeint	num;
{
int	cnt;

   *ncodeptr++ = trTable[num].newCode;
   addr++;
   addr++; /* skip curqui */
   cnt = (*addr&0x3f);
   *ncodeptr++ = cnt + 1;
   trTable[num].oldLen = cnt + 3;

   *ncodeptr++ = *addr++;
   while (cnt > 0)
   {
      *ncodeptr++ = *addr++;
      cnt--;
   }
   return(0);
}

int n_fidcode(addr,num)
codeint  *addr;
codeint  num;
{
   if (autogainptr != NULL)
      *autogainptr = ncodeptr - newstartptr; /* patch AUTOSHIM with Acode */
					     /* offset to codes that      */
					     /*  acquire FID              */
   if (autoshimptr != NULL)
      *autoshimptr = ncodeptr - newstartptr; /* patch AUTOGAIN with Acode */
					     /* offset to codes that      */
					     /* acquire FID               */
}

n_gain(addr,num)
codeint	*addr;
int	num;
{
   addr++;
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 3;
   *ncodeptr++ = 0xa28;		/* change me, change n_gain too */
   *ncodeptr++ = *addr++;
   *ncodeptr++ = rtrecgain;
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

n_hkeep(addr,num)
codeint	*addr;
int	num;
{
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 6;
   *ncodeptr++ = ssval;
   *ncodeptr++ = ssctr;
   *ncodeptr++ = ntrt;
   *ncodeptr++ = ct;
   *ncodeptr++ = bsval;
   *ncodeptr++ = bsctr;

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
   if (fidscanflag)
   {
      *ncodeptr++ = ACQI_UPDT;
      *ncodeptr++ = 2;
      *ncodeptr++ = acqiflagrt;
      *ncodeptr++ = relaxdelayrt;
   }
}

n_hsline(addr,num)
codeint	*addr;
int	num;
{
   if (ix==1)
      printf("Acode %d marked as NULL, check\n",trTable[num].oldCode);
}

int
n_hwloop(addr, num)
codeint  *addr;
int      num;
{
        *ncodeptr++ = trTable[num].newCode;
        addr++;
        *ncodeptr++ = 3;
        addr++;         /* Increment past number of fifo words        */
                        /* this is taken care of with ENDHWLOOP acode */
        addr++;         /* Increment past multiple hwloops flag, not needed */
        if (*addr > 0) *ncodeptr++ = *addr;
        else           *ncodeptr++ = *addr + (short)(addr - oldstartptr);
        addr++;
        hwloopptr = ncodeptr;
        ncodeptr += 2; /* Reserve space for jump location - set by ENDHWLOOP */

        return(0);
}

n_ifminus(addr,num)
codeint	*addr;
int	num;
{
codeint		jump;
codeint		*tptr;
unsigned int	offset;

   tptr = addr + 3;  /* get the address of offset */
   jump = *tptr;
   tptr = oldstartptr + jump - 4;
   if (*tptr == IFMIFUNC)  /* comes from endloop */
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
       abort_message("missing endloop()\n");
   }
}

n_ifnz(addr,num)
codeint	*addr;
int	num;
{
codeint  jump;
codeint  *tptr;
unsigned int  offset;

   tptr = addr + 3;  /* get the address of offset */
   jump = *tptr;

   /* assume no branch */
   tptr = oldstartptr + jump ;
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
/* n_ifnzb                                                              */
/*      This routine differs from n_ifnz in that a branch is associated */
/*      with it.  This supports the ifzero...elsenz....endif construct, */
/*      where n_ifnz supports the ifzero...endif construct.             */
/*----------------------------------------------------------------------*/
int
n_ifnzb(addr, num)
codeint  *addr;
codeint  num;
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
n_init(addr,num)
codeint	*addr;
codeint	num;
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

   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 8;
   *ncodeptr++ = npr_ptr;
   *ncodeptr++ = ssval;
   *ncodeptr++ = ssctr;
   *ncodeptr++ = ntrt;
   *ncodeptr++ = ct;
   *ncodeptr++ = bsval;
   *ncodeptr++ = bsctr;
   *ncodeptr++ = maxsum;

   *ncodeptr++ = RT2OP;
   *ncodeptr++ = 3;
   *ncodeptr++ = SET;
   *ncodeptr++ = ct;
   *ncodeptr++ = strt;
}

n_lksync(addr,num)
codeint	*addr;
codeint	num;
{
   n_apbout(addr,num);
}

n_loadshim(data,num)
codeint	*data;
int	num;
{
codeint	len;
long	tword1,tword2;

   if (shimset != 10)
   {  *ncodeptr++ = SYNC_PARSER;
      *ncodeptr++ = 4;
      *ncodeptr++ = 0;
      *ncodeptr++ = 10;
      *ncodeptr++ = 0;
      *ncodeptr++ = 0;

/* putcode_long(tword2, ncodeptr);
/* ncodeptr += 2;
/* putcode_long(tword1, ncodeptr);
/* ncodeptr += 2;
*/
   }

   len = trTable[num].oldLen - 1;
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = trTable[num].oldLen;
   *ncodeptr++ = ct;
   data++;
   while (len > 0)
   {
       *ncodeptr++ = *data++;
       len--;
   }
   return(0);
}

n_lock(addr,num)
codeint	*addr;
int	num;
{
double   max,min,step;
   addr++;
   /* printf("n_alock, offset: %d\n",ncodeptr - newstartptr); */
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 4;
   *ncodeptr++ = ct;
   *ncodeptr++ = *addr++;	/* amount of time to wait for hardware */
   par_maxminstep(GLOBAL, "lockpower", &max, &min, &step);
   *ncodeptr++ = (short) max;
   par_maxminstep(GLOBAL, "lockgain", &max, &min, &step);
   *ncodeptr++ = (short) max;
   addr++;
   addr++;
/* re-init adc after autolock (otherwhise acquire lock signal) */
   *ncodeptr++ = INIT_ADC;
   *ncodeptr++ = 3;
   *ncodeptr++ = 0;		/* HS_Dtm_Adc */
   *ncodeptr++ = adccntrl;	/* adcntrl rt parm  */
   *ncodeptr++ = ssval;		/* iff ss=0 enable adc overflow */
}

n_nextcodeset(addr,num)
codeint	*addr;
int	num;
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

n_noise(addr,num)
codeint	*addr;
int	num;
{
short	*ptr;

   if (acqiflag) return;

   /* printf("n_noise, offset: %d\n",ncodeptr - newstartptr); */

   *ncodeptr++ = RT2OP;
   *ncodeptr++ = 3;
   *ncodeptr++ = SET;
   *ncodeptr++ = zero;
   *ncodeptr++ = tmprt;

   *ncodeptr++ = INITSCAN;
   *ncodeptr++ = 8;
   *ncodeptr++ = npnoise;
   *ncodeptr++ = zero; /* ssval */
   *ncodeptr++ = ssctr;
   *ncodeptr++ = one;  /* nt */
   *ncodeptr++ = strt; /* ct */
   *ncodeptr++ = zero;  /* bsval */
   *ncodeptr++ = tmprt; /* bsctr */
   *ncodeptr++ = 0x7F;

   /* Set DSP hardware */
/************ tell the card about time correction ******/
/*   *ncodeptr++ = APBCOUT;
/*   *ncodeptr++ = 4;
/*   *ncodeptr++ = STD_APBUS_DELAY;          /* delay */
/*   *ncodeptr++ = 1;
/*   *ncodeptr++ = 0xe84;
/*   *ncodeptr++ = (codeint) dsp_info[2];
/*******************************************************/
/*   *ncodeptr++ = APBCOUT;
/*   *ncodeptr++ = 4;
/*   *ncodeptr++ = STD_APBUS_DELAY;          /* delay */
/*   *ncodeptr++ = 1;
/*   *ncodeptr++ = 0xe86;
/*   *ncodeptr++ = (codeint) dsp_info[0];

   /* Next Scan Opcode */
   *ncodeptr++ = NEXTSCAN;
   *ncodeptr++ = 7;
   *ncodeptr++ = npnoise;
   *ncodeptr++ = dpfrt;		/* dp index */
   *ncodeptr++ = one;		/* nt */
   *ncodeptr++ = strt;		/* ct */
   *ncodeptr++ = zero;		/* bsval */
   *ncodeptr++ = dtmcntrl;	/* dtmcntrl */
   *ncodeptr++ = zero;		/* fidctr */

/*   *ncodeptr++ = EVENT1_DELAY;
/*   *ncodeptr++ = 2;
/*   *ncodeptr++ = (codeint) 0;
/*   *ncodeptr++ = (codeint) 2400;   /* 30 usec dsp settling delay */
/*
/*   if (HS_Dtm_Adc == 0)
/*   {
/*      *ncodeptr++ = DISABLEOVRLDERR;
/*      *ncodeptr++ = 1;
/*      *ncodeptr++ = adccntrl;
/*   }
/*   else
/*   {
/*      *ncodeptr++ = DISABLEHSSTMOVRLDERR;
/*      *ncodeptr++ = 1;
/*      *ncodeptr++ = dtmcntrl;
/*   }
/* */

   addr++;
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 5;
   *ncodeptr++ = *addr++;   /* flags=hwlooping */
   *ncodeptr++ = *addr++;   /* datapoints (msw) */
   *ncodeptr++ = *addr++;   /* datapoints (lsw) */
   *ncodeptr++ = *addr++;   /* timerword 1 (msw) */
   *ncodeptr++ = *addr++;   /* timerword 1 (lsw) */

/*   *ncodeptr++ = EVENTN;
/*   *ncodeptr++ = 2;
/*   ptr = (short *) &noise_dwell; */
/*   *ncodeptr++ = 1;
/*   *ncodeptr++ = 0x4001;
/*   ptr++;
/*   *ncodeptr++ = (codeint) *ptr;
/* NOMERCURY */

   /* End of Scan */
   *ncodeptr++ = ENDOFSCAN;
   *ncodeptr++ = 6;
   *ncodeptr++ = zero; /* ssval */
   *ncodeptr++ = ssctr;
   *ncodeptr++ = one;  /* nt */
   *ncodeptr++ = strt; /* ct */
   *ncodeptr++ = zero;  /* bsval */
   *ncodeptr++ = tmprt; /* cbs */

   *ncodeptr++ = STATBLOCK_UPDT;
   *ncodeptr++ = 0;

/*   *ncodeptr++ = EVENT1_DELAY;
/*   *ncodeptr++ = 2;
/*   *ncodeptr++ = (codeint) 0x24;
/*   *ncodeptr++ = (codeint) 0x9f00; /* 30 msec dsp settling delay */
/* NOMERCURY */

/*   if (!debug2) */
     { 
      /* after noise, stop fifo and let noise data come to host then */
      /* startup avoids FOO problems with memory allocation, etc...  */
      /* If debug2 (output fifowords to file) is set do not stop the */
      /* fifo, as this will keep words from being written to the     */
      /* fifo.                                                       */
      *ncodeptr++ = NOISE_CMPLT;
      *ncodeptr++ = 0;
    }
}

n_nsc(addr,num)
codeint	*addr;
int	num;
{
        if (num_nsc < 12)
        {
           old_nsc_branch[num_nsc] = addr - oldstartptr;
           new_nsc_branch[num_nsc] = ncodeptr - newstartptr;
           num_nsc++;
        }
        else
        {
           abort_message("too many NSC code\n");
        }
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
        *ncodeptr++ = trTable[num].newCode;
        *ncodeptr++ = 7;
        *ncodeptr++ = npr_ptr;
        *ncodeptr++ = dpfrt; /* dp index */
        *ncodeptr++ = ntrt;
        *ncodeptr++ = ct;
        *ncodeptr++ = bsval;
        *ncodeptr++ = dtmcntrl;
        *ncodeptr++ = fidctr;

        /* orgHSdata = (unsigned int) get_acqvar(HSlines_ptr); */

        /* Set DSP hardware */
/************ tell the card about time correction ******/
/*        *ncodeptr++ = APBCOUT;
/*        *ncodeptr++ = 4;
/*        *ncodeptr++ = STD_APBUS_DELAY;          /* delay */
/*        *ncodeptr++ = 1;
/*        *ncodeptr++ = 0xe84;
/*        *ncodeptr++ = (codeint) dsp_info[2];
/*******************************************************/
/*        *ncodeptr++ = APBCOUT;
/*        *ncodeptr++ = 4;
/*        *ncodeptr++ = STD_APBUS_DELAY;          /* delay */
/*        *ncodeptr++ = 1;
/*        *ncodeptr++ = 0xe86;
/*        *ncodeptr++ = (codeint) dsp_info[0]; */

	/* Clear data at blocksize, if requested */
        *ncodeptr++ = BRA_EQ;
        *ncodeptr++ = 4;
        *ncodeptr++ = bsval;    /* Skip if bsval is 0 */
        *ncodeptr++ = zero;
        *ncodeptr++ = 0;
        *ncodeptr++ = 23;
        *ncodeptr++ = BRA_EQ;
        *ncodeptr++ = 4;
        *ncodeptr++ = clrbsflag; /* Skip if clrbsflag is false */
        *ncodeptr++ = zero;
        *ncodeptr++ = 0;
        *ncodeptr++ = 17;
        *ncodeptr++ = RT3OP;
        *ncodeptr++ = 4;
        *ncodeptr++ = MOD;
        *ncodeptr++ = ct;
        *ncodeptr++ = bsval;
        *ncodeptr++ = tmprt;    /* tmprt = ct % bsval */
        *ncodeptr++ = BRA_NE;
        *ncodeptr++ = 4;
        *ncodeptr++ = tmprt;    /* Skip if we're not at blocksize */
        *ncodeptr++ = zero;
        *ncodeptr++ = 0;
        *ncodeptr++ = 5;
        *ncodeptr++ = CLEARSCANDATA;
        *ncodeptr++ = 1;        /* Passed all tests - clear data */
        *ncodeptr++ = dtmcntrl;

        return(0);

}

n_padly(addr,num)
codeint  *addr;
codeint  num;
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
        *ncodeptr++ = trTable[num].newCode;
        *ncodeptr++ = *addr++ + 1;
        *ncodeptr++ = ct;
}

n_phasestep(addr,num)
codeint	*addr;
int	num;
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
       *ncodeptr++ = *addr++;
   }
   else
   {
      abort_message("No Phase table for channel %d.\n",channel);
   }
}

n_rt2op(addr,num)
codeint	*addr;
int	num;
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
       if (*addr > 0) *ncodeptr++ = *addr;
       else           *ncodeptr++ = *addr + (short)(addr - oldstartptr);
       addr++;
       len--;
   }

}

n_rt3op(addr,num)
codeint	*addr;
int	num;
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
       if (*addr > 0) *ncodeptr++ = *addr;
       else           *ncodeptr++ = *addr + (short)(addr - oldstartptr);
       addr++;
       len--;
   }

}

n_rtop(addr,num)
codeint	*addr;
int	num;
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
      if (*addr > 0) *ncodeptr++ = *addr;
      else           *ncodeptr++ = *addr + (short)(addr - oldstartptr);
      addr++;
      len--;
   }
}

n_rtinit(addr,num)
codeint	*addr;
int	num;
{
   addr++;
   *rtinit_ptr++ = *addr++;
   *rtinit_ptr++ = *addr++;
   *rtinit_ptr++ = *addr++;
}

n_scopy(data,num)
codeint	*data;
codeint	num;
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

int
n_seticm(addr,num)
codeint  *addr;
int      num;
{
   /* Set Receiver phase cycle Opcode */
   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 2;
   *ncodeptr++ = oph;
   *ncodeptr++ = dtmcntrl;
/*   if (acqiflag)
/*   {
/*      if (HS_Dtm_Adc == 0)
/*      {
/*         *ncodeptr++ = DISABLEOVRLDERR;
/*         *ncodeptr++ = 1;
/*         *ncodeptr++ = adccntrl;
/*      }
/*      else
/*      {
/*         *ncodeptr++ = DISABLEHSSTMOVRLDERR;
/*         *ncodeptr++ = 1;
/*         *ncodeptr++ = dtmcntrl;
/*      }
/*   }
/*   else
/*   {
/*      if (HS_Dtm_Adc == 0)
/*      {
/*         /* printf("n_seticm: ENABLEOVRLDERR (DTM)\n"); */
/*         *ncodeptr++ = ENABLEOVRLDERR;
/*         *ncodeptr++ = 3;
/*         *ncodeptr++ = ssctr;
/*         *ncodeptr++ = ct;
/*         *ncodeptr++ = adccntrl;
/*      }
/*      else
/*      {
/*         /* printf("n_seticm: ENABLEHSSTMOVRLDERR (HS ADM)\n"); */
/*
/*         *ncodeptr++ = ENABLEHSSTMOVRLDERR;
/*         *ncodeptr++ = 3;
/*         *ncodeptr++ = ssctr;
/*         *ncodeptr++ = ct;
/*         *ncodeptr++ = dtmcntrl;
/*      }
/*   } */
}

n_setphase(addr,num)
codeint	*addr;
int	num;
{
codeint  len,channel,tablenum,flag;

   addr++;
   channel = (*addr & 0xff)-1;
   flag = (*addr++ >> 8) & 0xff;
   *ncodeptr++ = trTable[num].newCode;
   len = trTable[num].oldLen-1;
   *ncodeptr++ = len;

   if ((tablenum=phasetablenum[channel]) >= 0)
   {
       *ncodeptr++ = tablenum | (flag<<8);
       if (*addr > 0) *ncodeptr++ = *addr;
       else           *ncodeptr++ = *addr + (short)(addr - oldstartptr);
       addr++;
   }
   else
   {
      abort_message("No Phase table for channel %d.\n",channel);
   }
}

int
n_setphase90(addr, num)
codeint	*addr;
codeint	num;
{
codeint	len,channel,tablenum,flag;

   addr++;
   channel = (*addr & 0x7)-1;
   if ((tablenum=phasetablenum[channel]) < 0)
   {
      abort_message("No Phase table for channel %d.\n",channel);
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
       if (*addr > 0) *ncodeptr++ = *addr;
       else           *ncodeptr++ = *addr + (short)(addr - oldstartptr);
       addr++;
       len--;
   }
}

int
n_setphattr(addr, num)
codeint	*addr;
codeint	num;
{
codeint	len,channel,tablenum,flag,*shortptr;
int	i;
int	*th;
int	*createphasetable();
phasetbl *pphtbl;
int     val;

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
   pphtbl->phaseprecision = htonl(256);   /* always assume 0.25 degree prec */
   val = (*addr++ << 8); /* dev addr */
   val |= *addr++; /* dev register */
   pphtbl->apaddr = htonl(val);
   pphtbl->apdelay = htonl(hb_dds_bits);
}

n_setup(addr, num)
codeint	*addr;
codeint	num;
{
   addr++;
/* load default line safe state */
/*   *ncodeptr++ = LOADHSL_R;
/*   *ncodeptr++ = 1;
/*   *ncodeptr++ = HSlines_ptr;
/*   *ncodeptr++ = EVENT1_DELAY;
/*   *ncodeptr++ = 2;
/*   *ncodeptr++ = (codeint) 0x0;
/*   *ncodeptr++ = (codeint) 0x800;		/* 100 usec */

   *ncodeptr++ = trTable[num].newCode;
   *ncodeptr++ = 1;
   *ncodeptr++ = SETUP_CMPLT;

   *ncodeptr++ = NEXTCODESET;
   *ncodeptr++ = 4;
   *ncodeptr++ = zero;     /* ilflagrt     */
   *ncodeptr++ = zero;     /* bsctr        */
   *ncodeptr++ = zero;     /* ntrt         */
   *ncodeptr++ = zero;     /* ct           */
}

n_shim(addr,num)
codeint	*addr;
int	num;
{
double   max,min,step;
codeint  len;

   /* printf("n_shim, offset: %d\n",ncodeptr - newstartptr); */
   addr++;
   *ncodeptr++ = trTable[num].newCode;
   len = *(addr+1);
   trTable[num].oldLen = len+3;
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

n_smpl_hold(addr,num)
codeint	*addr;
int	num;
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

n_tassign(addr,num)
codeint	*addr,num;
{
codeint  len, loc;
int      pindex;

   addr++;
   loc = *addr++;
   for (pindex = 0; pindex < num_aptables; pindex++)
   {
       if (apTable[pindex]->oldacodeloc == loc)
          break;
   }
   if (pindex >= num_aptables)
   {
       abort_message("psg: tassign error, couldn't find table address\n");
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

n_table(addr,num)
codeint	*addr;
codeint	num;
{
codeint  len, offset;
int      pindex, found;

   addr++;
   offset = addr - oldstartptr;
   num_aptables++;
   if (num_aptables < MAXAPTABLES)
   {
      pindex = num_aptables-1;
      apTable[pindex] = (apTbl *) (malloc(sizeof(apTbl)));
      if (apTable[pindex] == NULL)
      {
         abort_message("convert.c: Unable to allocate aptable info.\n");
      }
   }
   else
   {
      abort_message("convert.c: Too many aptables already defined.\n");
   }
   apTable[pindex]->oldacodeloc = offset;

   len = *addr + 4;
   trTable[num].oldLen = len + 1;
   *ncodeptr++ = trTable[num].newCode;
   apTable[pindex]->newacodeloc = ncodeptr - newstartptr;
   apTable[pindex]->newacodeloc += 1;	/* Add extra word for */
					/* acode length field */
   *ncodeptr++ = len;
   *ncodeptr++ = *addr++;		/* size of table */
   len--;
   apTable[pindex]->auto_inc = *addr;	/* auto-inc flag */
   while (len > 0)
   {
       *ncodeptr++ = *addr++;
       len--;
   }
}


int *
createphasetable(channeldev)
codeint	channeldev;
{
int	tblnum;
tblhdr	*th;

   /* IXPRINT1("createphasetable dev: %d.\n",channeldev); */
   tblnum = createtable( sizeof(tblhdr)+sizeof(phasetbl) );
   th = (tblhdr *) tblptr[tblnum];
   th->num_entries = sizeof(phasetbl)/sizeof(long);
   th->size_entry = sizeof(long);
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
   tmpdata = (codeint *)tempptr;
   memcpy( (char *)tempptr, data_entry, num_entries*entry_size);
#ifdef LINUX
   for (i=0; i<num_entries*entry_size/2; i++)
   {
      *tmpdata = htons(*tmpdata);
      tmpdata++;
   }
#endif
   /* tmpdata = data_entry; */
   /* for (i=0; i<num_entries; i++) */
   /*	for (j=0; j<(entry_size/2); j++) */
   /*		printf("createglobaltable data = 0x%x\n",*tmpdata++); */
   return(tblnum);
}

int
createtable(size)
int size;
{
   /* IXPRINT2("createtable num: %d size: %d.\n",num_tables,size); */
   tblptr[num_tables] = (int *) malloc( size );
   if (tblptr[num_tables] == NULL)
   {
      abort_message("Could not create table. size: %d\n",size);
   }

   num_tables++;

   if (num_tables > MAXTABLES)
   {
      abort_message("Max number of tables created.\n");
   }

   /* returns number of table created */
   return(num_tables-1);
}




n_check(addr,num)
codeint	*addr;
int	num;
{
   if (ix==1)
      printf("Acode %d marked as OBSOLETE, check\n",trTable[num].oldCode);
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
    {	abort_message("Exp table file already exists. PSG Aborted..\n");
    }

    hdrsize = (num_tables+1)*sizeof(int);
    tblheader = (int *)malloc(hdrsize);
    if (tblheader == NULL)
    {	abort_message("Unable to allocate memory for tblheader. PSG Aborted..\n");
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

