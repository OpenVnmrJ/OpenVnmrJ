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
/*************************************************************/
/*************************************************************/
/***                                                       ***/
/***      Constants, Type and Variable Declarations        ***/
/***                                                       ***/
/*************************************************************/
/*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "params.h"
#include "group.h"
#include "variables.h"
#include "lc_gem.h"
#include "acodes.h"
#include "rfconst.h"
#include "aptable.h"
#include "shrexpinfo.h"
#include "pvars.h"
#include "abort.h"
#include "dsp.h"

#define ERROR	1
#define	MAXSTR	28
#define WARNING_MSG   14
#define HARD_ERROR    15
#define	MAXSTR_UPLUS  256

extern	char	rfwg[];
extern  char	gradtype[];
extern	codeint	bsctr;
extern	codeint	bsval;
extern	codeint	ct;
extern	codeint	ctss;
extern	codeint	ilctss;
extern	codeint	ilflagrt;
extern	codeint	ilssval;
extern	codeint	initflagrt;
extern	codeint	maxsum;
extern	codeint	ntrt;
extern  codeint	one;
extern	codeint	ssval;
extern	codeint	strt;
extern  codeint	three;
extern	codeint	tmprt;
extern  codeint	two;
extern	codeint	v1,v10;
extern  codeint	zero;
extern	double	totaltime;
extern	double	tau;
extern	int	acqiflag;
extern	int	tuneflag;
extern	int	clr_at_blksize_mode;
extern	int	initializeSeq;
extern	int	dps_flag;
extern	int	grad_flag;
extern	int	HSlines;
extern  int	ok2bumpflag;
extern	int	newacq;
extern	int	PSfile;
extern	int	setupflag;
extern	int	bgflag;
extern	int	pulsesequence();
extern	int	ra_flag;
extern	short	*Aacode;
extern	short	*Codeptr;
extern	short	*Codes;
extern  long	CodeEnd;
extern	autodata	*Aauto;/* ptr to automation structure in Acode set */
extern	Acqparams	*Alc;  /* pointer to low core structure in Acode set */

extern SHR_EXP_STRUCT	ExpInfo;

#define	IDC	2304	/* software release and version  number */
/*------------------------------------*/
/* ap high speed line bit definitions */
/*------------------------------------*/
#define	SP1	  1	/* 0x01 flag line, not off board  */
#define RCVR	  2	/* 0x02 receiver HS line          */
#define	RFPC180	  4	/* 0x04 low band phase 180 bit    */
#define	RFPC90	  8	/* 0x08 low band phase 90 bit     */
#define	CTXON	 16	/* 0x10 low band tx gate line     */
#define	RFPH180	 32	/* 0x20 high band phase 90 bit    */
#define	RFPH90	 64	/* 0x40 high band phase 90 bit    */
#define	HTXON	128	/* 0x80 high band tx gate line    */
/* transmit  high speed line bits */
#define	DC180	32	/* decoupler phase 180 bit */
#define	DC90	64	/* decoupler phase 90 bit */
#define	DC270	96	/* decoupler phase 90/180 bits */
#define	RFPC270	12	/* carbon/BB phase 90/180 bits */
#define	RFPH270	96	/* Hydrogen phase 90 bit */

#define HI_BND_CW	0x01
#define	LO_BND_CW	0x02
#define OBS_HI_BND	0x04
#define	HOMDEC_ON	0x04
#define	DEC_HI_BND	0x08
#define	DM_WALTZ	0x10
#define	HOMDEC_TST	0x80

#define OS_MAX_NTAPS	50000
#define OS_MIN_NTAPS	3

#define is_y(target)	((target == 'y') || (target == 'Y'))
#define anyrfwg		( (is_y(rfwg[0])) || (is_y(rfwg[1])) )
#define anywg		anyrfwg		/* Mercury-Vx only supports rfwg */

/*-----------------------*/
/* analog port constants */
/*-----------------------*/
#define	APB_SELECT	24576	/* A000, if used with minus sign */
#define	APB_WRITE	20480	/* B000, if used with minus sign */
#define	APB_INRWR	28672	/* 9000, if used with minus sign */
#define	APB_SETWR	32768	/* 8000, is used with minus sign */

#define	RF_ADDR		0xa00	/* apbus address for al rf boards */
#define	LB_XMT_SEL	0x00
#define	HB_XMT_SEL	0x18
#define	HOMO_XMT_SEL	0x50

#define	MAXLOOPLENGTH	32

#define	SMPL		0
#define HOLD		1

/*-----------------------------------*/
/* t y p e   d e c l a r a t i o n s */
/*-----------------------------------*/
typedef struct _ptrfld {
	int	ca,ls,vlc,ja;
} ptrfld;

typedef	struct _ifjumpadr {
        int	ja1,ja2;
} ifjumpadr;

typedef	struct _aprecord {
        int		preg;	/* this is not saved */
        short		*apcarray;
} aprecord;

static union {
         unsigned int ival;
         codeint cval[2];
      } endianSwap;


/*-------------------------------------------*/
/* v a r i a b l e   d e c l a r a t i o n s */
/*-------------------------------------------*/

/* ap pointer */
aprecord	apc;
/* autodata pointer */
autodata	*autor;
/* array of ... */
ifjumpadr	ifa[3];
/* char */
char		alock[MAXSTR],amptype[MAXSTR],dm[MAXSTR],dmm[MAXSTR];
char		dseq[MAXSTR],f15k[MAXSTR],hs[MAXSTR],il[MAXSTR],interLock[4];
char		load[MAXSTR],method[135];
char		satmode[MAXSTR],syncr[MAXSTR],wshim[MAXSTR];
char		rftype[MAXSTR_UPLUS];
/* boolean */
int		acqtriggers,again,automated;
int		bb_refgen,blankingon_flag,broadband;
int		cardb,homdec,hwlooping,go_dqd;
int		indirect,noiseflag,ok,padflag;
int		shimatanyfid,tpf;
/* integers */
int		apbcount,apb_ap;
int		blank_apb,bsct_ap;
int		check,cpflag,ct_ap,curfifocount,curqui,cwmode;
int		declvlonoff,dmmode;
int		fattn[2];
int		hb_dds_bits,hr,hv,hwloop_cnt,hwloop_ptr;
int		ix;
int		lastcw,lastdmm,loc,lockmode;
int		maxscale,mf_dm,mf_dmm,mf_hs,mlb,mxcnst;
int		npr_ap,nsc_ptr,nsp;
int		oph_ap;
int		pdata,phase1;
int		rcvr_cntl;
int		seqpos,shimset,skipbse_ptr;
int		squi_ap,startqui,suflag;
int		traymax,txonx;
int		vttype;
int		xltype;
/* looparray */
ptrfld		la[3];
/* shorts and other pants */
short		lin_attn[48] = {
  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  5,  5,  6,
  7,  8,  9, 10, 11, 12, 14, 16, 17, 20, 22, 25, 28, 31, 35, 39,
 44, 49, 56, 62, 70, 79, 89,100,112,126,142,160,180,202,227,255
};
/* structs */
struct _dsp_params	dsp_params;
/* reals */
double		alfa,at,bs,cttime,cttimeval;
double		cur_tpwr,cur_tpwrf,cur_dpwr,cur_dpwrf;
double		d1,d2,d3,dfrq,dhpmax,dhp,dlp,dmf,dod,dof,dof_init;
double		dpwr,dpwrf,dqd_offset=0.0,dres;
double		fb,fifoloopcount;
double		gain,gmax,gradstepsz,hb_offset,hst;
double		lb_offset,lockfreq,lockgain,lockphase,lockpower;
double		ldshimdelay;
double		mlr,mxcnstr,newrcvr,ni,ni2,ni3,np,nt,oldspin,oldvttemp;
double		p1,pad,pplvl,preacqtime,pw,pwx,pwxlvl,rof1,rof2;
double		satdly,satfrq,satpwr,sfrq,spin,ss,sw;
double		tof,tof_init,tot_np,tpwr,tpwrf,vtc,vttemp,vtwait;
double		z0;

void setpower(double value, int device);
void delay(double delay);
void initdmfapb(double dmf_local);
void setoffset(double offset, int address);
void setlbfreq(double freq, double off);
void write_Acodes(int act);

/**************************************************************/
/**************************************************************/
/***                                                        ***/
/***       PSG  Procedures  and  Main  routine              ***/
/***                                                        ***/
/**************************************************************/
/**************************************************************/

/*-------------------------------*/
/*       putcode                 */
/*-------------------------------*/
void putcode(int datum)                                 
{
   if (ok)
   {  apc.apcarray[apc.preg] = (short) datum;
      Codeptr++;	/* for phase table compatiblity, someday should use */
			/* only Codeptr */
      apc.preg ++;
      if (&apc.apcarray[apc.preg] >= (short *) CodeEnd)
      {
        abort_message("Acode overflow, %ld words generated.",
                (long) (apc.preg));
      }
   }
}
/*-----------------------------------------------------------------
|       putLongCode()/1
|       puts long integer word into short interger Codes array
|
+------------------------------------------------------------------*/
void putLongCode(unsigned int longWord)
{
   if (bgflag)
      fprintf(stderr,"LongCode = %d(dec) or %4x(hex) \n", longWord,longWord);
   endianSwap.ival = longWord;
#ifdef LINUX
   putcode( endianSwap.cval[1] );
   putcode( endianSwap.cval[0] );
#else
   putcode( endianSwap.cval[0] );
   putcode( endianSwap.cval[1] );
#endif
}
/**************************************************************/
/*        bit, byte, word, long manipulation routines         */
/**************************************************************/

/*-------------------------------*/
/*       gate                    */
/*-------------------------------*/
void gate(int gatebit, int on)                    
{
   if ( ! newacq )
   {  if (on) 
         curqui |= gatebit;
      else
         curqui &= ~gatebit;
   }
   else
   { if (on)
      {  putcode(IMASKON);
         HSlines |= gatebit;
      }
      else
      {  putcode(IMASKOFF);
         HSlines &= ~gatebit;
      }
      /* put 32-bit of HS lines, even though we have only 9, for  */
      /* compatibility with INOVA */
      putcode( 0 );			/* high order word */
      putcode( gatebit & 0xffff );	/* low order word */
      return;
    }
}

/*-------------------------------*/
/*          xgate                */
/*-------------------------------*/
void xgate(double n)
{
   if (n != 1) 
   {
        text_error("Number of external triggers must be 1. Reset");
   }
   putcode(XGATE);
}

/*-------------------------------*/
/*       putrealdata             */
/*-------------------------------*/
void putrealdata(double r)                                   
{
int	pdata;
   pdata = (int) r;
   putcode(pdata >> 16);
   putcode(pdata & 0xFFFF);
}

/**************************************************************/
/*              ap integer math operations                    */
/**************************************************************/
/*-------------------------------*/
/*       assign                  */
/*-------------------------------*/
void assign(int a, int b)
{
  notinhwloop("assign");
  putcode(ASSIGNFUNC);
  putcode(a);
  putcode(b);
}

/*-------------------------------*/
/*       add                     */
/*-------------------------------*/
void add(int a, int b, int c)
{
  notinhwloop("add");
  putcode(ADDFUNC);
  putcode(a);
  putcode(b);
  putcode(c);
}

/*-------------------------------*/
/*       sub                     */
/*-------------------------------*/
void sub(int a, int b, int c)
{
  notinhwloop("sub");
  putcode(SUBFUNC);
  putcode(a);
  putcode(b);
  putcode(c);
}

/*-------------------------------*/
/*       dbl                     */
/*-------------------------------*/
void dbl(int a, int b)
{
  notinhwloop("dbl");
  putcode(DBLFUNC);
  putcode(a);
  putcode(b);
}

/*-------------------------------*/
/*       hlv                     */
/*-------------------------------*/
void hlv(int a, int b)
{
  notinhwloop("hlv");
  putcode(HLVFUNC);
  putcode(a);
  putcode(b);
}

/*-------------------------------*/
/*       modn                    */
/*-------------------------------*/
void modn(int a, int b, int c)
{
 notinhwloop("modn");
 putcode(MODFUNC);
 putcode(a);
 putcode(b);
 putcode(c);
}

/*-------------------------------*/
/*       mod4                    */
/*-------------------------------*/
void mod4(int a, int b)
{
  notinhwloop("mod4");
  putcode(MOD4FUNC);
  putcode(a);
  putcode(b);
}

/*-------------------------------*/
/*       mod2                    */
/*-------------------------------*/
void mod2(int a, int b)
{
  notinhwloop("mod2");
  putcode(MOD2FUNC);
  putcode(a);
  putcode(b);
}

/*-------------------------------*/
/*       mult                    */
/*-------------------------------*/
void mult(int a, int b, int c)
{
 notinhwloop("mult");
 putcode(MULTFUNC);
 putcode(a);
 putcode(b);
 putcode(c);
}

/*-------------------------------*/
/*       divn                    */
/*-------------------------------*/
void divn(int a, int b, int c)
{
 notinhwloop("divn");
 putcode(DIVFUNC);
 putcode(a);
 putcode(b);
 putcode(c);
}

/*-------------------------------*/
/*       incr                    */
/*-------------------------------*/
void incr(int a)
{
  notinhwloop("incr");
  putcode(INCRFUNC);
  putcode(a);
}

/*-------------------------------*/
/*       decr                    */
/*-------------------------------*/
void decr(int a)
{
  notinhwloop("decr");
  putcode(DECRFUNC);
  putcode(a);
}

/*-------------------------------*/
/*       orr                     */
/*-------------------------------*/
void orr(int a, int b, int c)
{
  notinhwloop("orr");
  putcode(ORFUNC);
  putcode(a);
  putcode(b);
  putcode(c);
}

/*-------------------------------*/
/*       ifnz                    */
/*-------------------------------*/
void ifnz(int a, int b, int c)
{
  notinhwloop("ifnz");
  putcode(IFNZFUNC);
  putcode(a);
  putcode(b);
  putcode(c);
}

/*-------------------------------*/
/*       ifmi                    */
/*-------------------------------*/
void ifmi(int a, int b, int c)
{
  notinhwloop("ifmi");
  putcode(IFMIFUNC);
  putcode(a);
  putcode(b);
  putcode(c);
}

/**************************************************************/
/*                  ap loop control                           */
/**************************************************************/

/*-------------------------------*/
/*       fixjumpaddress          */
/*-------------------------------*/
void fixjumpaddress(int loc, int address)                    
{
   apc.apcarray[loc] = address-loc;
}

/*-------------------------------*/
/*       initval                 */
/*-------------------------------*/
/*   initialize apdp temps (v's) with real value---
 *      a floating point integer(0 to 10**9) is
 *      converted to apdp integer and stored in
 *      the desired temp(v) location.  the incoming
 *      value is rounded up and the result is positive.
 */
void initval(double rc, int  stl)                       
{
double	rv,sign_add();
  rv = rc + 0.500005;
  if (rv >= 32768.0) rv = 32767.1;
  if ( ! newacq )
     apc.apcarray[stl] = (short)rv;
  else
      set_acqvar((codeint)stl,(int) sign_add(rc,0.0005) );
}

/*-------------------------------*/
/*          setpwrf              */
/*-------------------------------*/
void setpwrf(double value, int device)
{
int	tvalue;
/* check we don't try to set low band or homodecoupler finepower */
   if ( ((device==TODEV)&&(cardb)) || ((device==DODEV)&& (! cardb)) )
   {  abort_message("setpwrf(): fine power control for high band transmitter only\n");
   }
/* check if the device really is there according to config */
   if (fattn[0] != 256)
   {  abort_message("setpwrf(): fine power hardware not present, check config\n");
   }

   tvalue = (int)(value + 0.5);
/* printf("setpwrf(): value=%f, tvalue=%d\n",value,tvalue); */
   putcode(-APB_SELECT + 0x150);		/* select xmtr register  */
   putcode(-APB_WRITE  + 0x100 + tvalue);	/* set fine power for xmit */
}

/*-------------------------------*/
/*          rlpwrf               */
/*-------------------------------*/
void rlpwrf(double value, int device)
{  
   if ( ((device==TODEV)&&(cardb))    || 
        ((device==DODEV)&& (! cardb)) ||
        (fattn[0] != 256) )
   {
      /* No fine attenuator present, do +/- fine control through setpower */
      if (device==TODEV)
      {  cur_tpwrf=value;
         putcode(APBOUT);
         putcode(3);
         setpower(cur_tpwr,TODEV);
      }
      else if (device==DODEV)
      {  cur_dpwrf=value;
         putcode(APBOUT);
         if (!cardb && !indirect )
            putcode(2);
         else
            putcode(3);
         setpower(cur_dpwr,DODEV);
      }
   }
   else
   {  if (device==TODEV)
         cur_tpwrf=value;
      else
         cur_dpwrf=value;
      putcode(APBOUT);
      putcode(1);
      setpwrf(value, device);
   }
}

/*-------------------------------*/
/*          setpower             */
/*-------------------------------*/
/* sets value on rf-control attanuators */
void setpower(double value, int device)
{
int	ipwrf;
int	used_value;
int	tvalue;
int	db20 = 0;
int	reg;
int	i;
   used_value = (int) (value+0.5);
/* figure out which board we really are programming */
/* and check limits */
   if (device==TODEV)
   {  if (cardb)
         reg = LB_XMT_SEL;
      else 
         reg = HB_XMT_SEL;
      cur_tpwr = value;
      ipwrf = (int)(cur_tpwrf+0.5);
   }
   else if (device==DODEV)
   {  if (value > dhpmax) 
      {  printf("psg: Decoupler power reset to preset maximum of %5.1g",dhpmax);
         used_value = (int) (dhpmax+0.5);
      }
      if (cardb)
         reg = HB_XMT_SEL;
      else if (indirect)
         reg = LB_XMT_SEL;
      else
         reg = HOMO_XMT_SEL;
      cur_dpwr=value;
      ipwrf = (int)(cur_dpwrf+0.5);
   }
   else
   {  abort_message("psg: cannot set power for device %d, only TODEV or DODEV\n",
			device);
   }
/* figure out how to set linear attn and which pads */
   if (reg == LB_XMT_SEL)	/* for low band */
   {  used_value += 5;		/* range is 0-68 dB */
      if (used_value > 47)	
      {  db20 = 1;
         used_value -= 20;
      }
      else
         db20 = 0;
   }
   else if (reg == HB_XMT_SEL)	/* for high band */
   {  if (fattn[0] != 256)
      {  used_value += 15;		/* range is 0-78 dB */
         db20 = 8;
         while (used_value > 48)
         {  used_value -= 10;
            db20 >>= 1;
         }
         db20 = (~db20 & 0xf);
      }
      else
      {  
         reg = 0x1C0;
         tvalue = 63 - (int)(value+0.5); 	/* range is now 0-79 */
         if (tvalue>63) tvalue = (tvalue-16)*2 + 1;
         else           tvalue = tvalue * 2;
/* printf("setpower(): value=%f,tvalue=%d\n",value,tvalue); */
         putcode(-APB_SETWR  + 0x100 + tvalue); /* set for power setting */
         return;
      }
   }
   if (used_value <1) used_value=1;
   tvalue = lin_attn[used_value-1];
   i = (int) ( (double)(ipwrf-255) * (double)tvalue / 255.0 );
   tvalue +=  i;
   if (tvalue < 0)
   {  tvalue=0;
      printf("Coarse and Fine power add to below minimum\n");
      printf("Reset to zero for %s\n",(device==TODEV)?"TODEV":"DODEV");
   }
/* printf("setpower(): dev=%d, val=%f,db20=0x%x,u_v=%d,tvalue=%d\n",
	device,value,db20,used_value,tvalue); */
   putcode(-APB_SELECT + RF_ADDR + reg);	/* select xmtr register  */
   putcode(-APB_WRITE  + RF_ADDR + 0x50);	/* set for power setting */
   putcode(-APB_INRWR  + RF_ADDR + tvalue);	/* set fine power for xmit */
   curfifocount += 3;
   if (reg != HOMO_XMT_SEL)
   {  putcode(-APB_INRWR  + RF_ADDR + db20);	/* set extra pads for xmit */
      curfifocount++;
   }
}

/*-------------------------------*/
/*          rlpower              */
/*-------------------------------*/
void rlpower(double value, int device)
{
   if ((device == TODEV) || (device == DODEV))
   {  putcode(APBOUT);
      if ( ((device==TODEV)&&(cardb))    || 
           ((device==DODEV)&& (! cardb)) ||
           (fattn[0] != 256) )
         putcode(3);
      else
         putcode(0);
      setpower(value, device);
   }
   else
   {  abort_message("rlpower(value, device): device must be TODEV or DODEV\n");
   }
}

/*-------------------------------*/
/*       homodecpwr              */
/*-------------------------------*/
void homodecpwr(double value)     /* set power of homodecoupler */
{
int	homo_pwr;
int	attn_db, attn_lin;
    homo_pwr = (int) value + 20;
    if (homo_pwr > 83) homo_pwr=83;
    else if (homo_pwr < 0) homo_pwr=0;
    
    attn_db = (83 - homo_pwr)/5;
    if (attn_db > 7) attn_db = 7;	       /*can have up to 7 * 5dB steps */
    attn_lin = 48 - 83 + 5*attn_db + homo_pwr; /*the rest is in linear attn   */

    putcode( -APB_SELECT + RF_ADDR + HOMO_XMT_SEL);
    putcode( -APB_WRITE  + RF_ADDR + 0x80);
    putcode( -APB_INRWR  + RF_ADDR + attn_db);
    curfifocount += 3;
    setpower((double)attn_lin, DODEV);
}

/*-------------------------------*/
/*      offset                   */
/*-------------------------------*/
void offset(double freq, int device)
/*  output transmitter or decoupler offset frequency to apb port */
{
double	hd_offset;
int	cnt_place, cnt_strt;
   putcode(APBOUT);
   cnt_place = apc.preg;
   putcode(0);		/* place for apbus count */
   cnt_strt = apc.preg;
   if (cardb)		/* low band observe, high band decouple */
   {  if (device == TODEV)
         setlbfreq(sfrq,freq - tof_init);
      else
      {  hd_offset=(20.0*hv/hr-dfrq)*1e6-freq+dof_init;
         setoffset(hd_offset,HB_XMT_SEL);	/* set high band offset */
      }
   }
   else if (indirect)	/* high band observe, low band decoupler */
   {  if (device == TODEV)
      {  hd_offset=(20.0*hv/hr-sfrq)*1e6-freq+tof_init;
         setoffset(hd_offset,HB_XMT_SEL);	/* set high band offset */
      }
      else
         setlbfreq(dfrq,freq - dof_init);
   }
   else			/* high band observe, homo decouple */
   {  if (device == TODEV)
      {  hd_offset=(20.0*hv/hr-sfrq)*1e6-freq+tof_init;
         setoffset(hd_offset,HB_XMT_SEL);	/* set high band offset */
      }
      else
      {  if (homdec)
         {  hd_offset=(20.0*hv/hr-dfrq)*1e6-freq+dof_init;
            setoffset(hd_offset,HOMO_XMT_SEL); /* set homo dec offset */
         }
         else
            printf("psg: no homo decoupler in system, or set `HOMDEC='y'`\n");
      }
   }
   apc.apcarray[cnt_place] = apc.preg - cnt_strt - 1;
}

/***************************************************************"
 *       options/accessories manipulation routines              "
 ***************************************************************"
 */
/*-------------------------------*/
/*       waitforvt               */
/*-------------------------------*/
void waitforvt()
{
double	vtwr;
   if (((interLock[2] != 'N') && (interLock[2] != 'n')) && tpf && (vttype!=0) )
   {  vtwr = vtwait;			/* vtwait is in seconds */
      if (vtwr < 1.0) vtwr = 1.0;	/* approx 10 second */
/* prime timeout counter */
      putcode(WTFRVT);
      if (newacq)
      { putcode( ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
        putcode((codeint) vtwr);
      }
      else
         putrealdata(vtwr);
   }
}

/*----------------------------------------*/
/* procedure to set Oxford VT controllers */
/*----------------------------------------*/
void setvt()
{
int	temp;
  putcode(SETVT);			/* setvt acode 63 */
  putcode(vttype);			/* Oxford = 2, none = 0 */
  putcode(440);				/* PID hardcoded */
  if (tpf)				/* If VT used then set it */
     temp = (int)(vttemp*10.0+0.5);
  else                               /* else set VT passive */
     temp = 30000;                   /* temp. value that says go passive */
  putcode(temp);                     /* temperature * 10 */
  putcode((int)(vtc*10.0+0.5));      /* low temp. cutoff for cooling gas */
  if ( (interLock[2] != 'N') && (interLock[2] != 'n'))
     putcode( ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                HARD_ERROR : WARNING_MSG);
  else
     putcode(0);
}

/*-------------------------------"
 *       dofiltercontrol         "
 *-------------------------------"
 */
/* establish receiver filter, correct filter bandwidth is */
/* established by parameter entry                         */
void dofiltercontrol()
{
int	filter_value;
    if (fb<1000.0) fb=1000.0;
    if (fb > 25000)
    {  rcvr_cntl = 8;
    }
    else
    {  rcvr_cntl = 0;
       if (fb > 3570)
          filter_value = (int)(137.5 - 262500.0/fb) + 1;
       else
          filter_value = (int)(87.5  - 87500.0/fb) + 1;
       if (filter_value > 127) filter_value = 127;
       if (filter_value <   0) filter_value = 0;
       putcode(-APB_SELECT + RF_ADDR + 0x28);
       putcode(-APB_WRITE  + RF_ADDR + 0x00);
       putcode(-APB_INRWR  + RF_ADDR + (filter_value&0xff));
       putcode(-APB_SELECT + RF_ADDR + 0x28);
       putcode(-APB_WRITE  + RF_ADDR + 0x01);
       putcode(-APB_INRWR  + RF_ADDR + (filter_value&0xff));
       curfifocount += 6;
   }
}

/**************************************************************/
/**************************************************************/

/*-------------------------------*/
/*       scanstart               */
/*-------------------------------*/
#define	VTFAILSTATUS	3
#define	LOCKFAILSTATUS	2
#define	SPINFAILSTATUS	4
void scanstart()
{
double	getval();
  if ( (interLock[0] != 'n') && (interLock[0] != 'N') )
  {  putcode(CKLOCK);
      putcode( ((interLock[0] == 'y') || (interLock[0] == 'Y')) ?
                HARD_ERROR : WARNING_MSG);
  }
  if ( ((interLock[2] != 'n') && (interLock[2] != 'N')) && tpf && (vttype!=0) )
  {  putcode(CKVTR);
     putcode( ((interLock[2] == 'y') || (interLock[2] == 'Y')) ?
                   HARD_ERROR : WARNING_MSG);
  }
  if (automated) /* automated system */
     if ( (spin>=0.0) && ((interLock[1] != 'n') && (interLock[1] != 'N')) )
     {  putcode(CHKSPIN);
        if (newacq)
        { putcode( spin >= getval("spinThresh") ? 100 : 1);
        }
        putcode( ((interLock[1] == 'y') || (interLock[1] == 'Y')) ?
                   HARD_ERROR : WARNING_MSG);
        if (newacq)
        {
           if (ok2bumpflag)
              putcode( 1 );
           else
              putcode( 0 );
        }
     }
}

/*-------------------------------*/
/*       gatedmmode              */
/*-------------------------------*/
void gatedmmode(int f)                                 
/* gate decoupler modulation mode according to field	*/
/*  specified c=cw; w=waltz; for Krikkit		*/
{
int	tsp;
int	use_wfg=0;
   tsp = f;
   if (f >= mf_dmm) tsp = mf_dmm - 1;
   dmmode &= ~0x63;	/* kill all mod mode bits */
   switch (dmm[tsp])
   {	case 'c':
	case 'C':
		dmmode |= 0x40;
		break;
	case 'f':
	case 'F':
		dmmode |= 0x42;
		break;
	case 'g':
	case 'G':
		dmmode |= 0x61;
		break;
	case 'm':
	case 'M':
		dmmode |= 0x62;
		break;
	case 'p':
	case 'P':
		use_wfg=1;
		dmmode |= 0x40;
		break;
	case 'r':
	case 'R':
		dmmode |= 0x41;
		break;
	case 'u':
	case 'U':
		dmmode |= 0x43;
		break;
	case 'w':
	case 'W':
		dmmode |= 0x60;
		break;
	case 'x':
	case 'X':
		dmmode |= 0x63;
		break;
	default:
		break;
   }
/* printf("gatedmmode(): f=%d, tsp=%d, dmmode=0x%x\n",f,tsp,dmmode); */
   if (lastdmm != dmmode )
   {  lastdmm = dmmode;
      if ((dmm[tsp] == 'g') || (dmm[tsp] == 'G'))
      {  /* printf("dmm='g' dmf=45*dmf\n"); */
         putcode(APBOUT);
         putcode(11);
         initdmfapb(45.0*dmf); /* GARP is in 2 degree */
      }
      else
      {  putcode(APBOUT);
         putcode(11);
         initdmfapb(dmf); /* Others are in 90 degree */
      }
      putcode(-APB_SELECT + RF_ADDR + 0x20);
      putcode(-APB_WRITE  + RF_ADDR + 0x10);
      putcode(-APB_INRWR  + RF_ADDR + (dmmode & 0xFF));
   }
   if (f >= mf_dm)
      tsp = mf_dm - 1;
   else
      tsp = f;
   if (lastcw != cwmode)
   {  cwmode &= ~(HI_BND_CW + LO_BND_CW);
      if (!indirect)
         cwmode |= HI_BND_CW;
      else
         cwmode |= LO_BND_CW;
      lastcw = cwmode;
      putcode(APBOUT);
      putcode(2);
      putcode(-APB_SELECT + RF_ADDR + 0x20);
      putcode(-APB_WRITE  + RF_ADDR + 0x50);
      putcode(-APB_INRWR  + RF_ADDR + cwmode);
      delay(2e-6);
      curfifocount += 14;
   }
   if (! use_wfg)
   {  gate(HTXON,FALSE);
      gate(CTXON,FALSE);
      if (dm[tsp]=='y')
      {  if (!indirect)
            gate(HTXON,TRUE);
         else
            gate(CTXON,TRUE);
      }
   }
   if ( use_wfg )
   {  if (dm[tsp]=='y')
         prg_dec_on(dseq,1/dmf,dres,DODEV);
      else
      {  if ( pgd_is_running(DODEV) )
            prg_dec_off(2,DODEV);
      }
   }
}

/*-------------------------------*/
/*       status                  */
/*-------------------------------*/
void status(int f)                                     
{
int	tsp;
   if ( (f<0) || (f>25) )
   {
     abort_message("PSG: status() index out of range\n");
     seqpos = 0;
   }
   else seqpos = f;
   tsp = f;
   if (rof2 < 2e-7)				/* force a delay to       */
      delay(2e-7);				/* update high-speed lines*/
						/* before apbout          */
   if (cardb || indirect)
   {  gatedmmode(f);
   }
   else 
   {  dmmode &= ~0x63;				/* set for cw             */
      dmmode |= 0x40;
      if (!homdec)				/* no homo decoupler 	  */
      {  putcode(APBOUT);                       /* via apb-bus            */
         putcode(2);                            /* three words            */
         putcode(-APB_SELECT + RF_ADDR + 0x20); /* select J-boards APchip */
         putcode(-APB_WRITE  + RF_ADDR + 0x00); /* write address on board */
         putcode(-APB_INRWR  + RF_ADDR + dmmode);/* write value to adddres */
         curfifocount += 3;
      }
      else					/* yes homo decoupler */
      {  tsp=f;					/* reset tsp incase it was*/
						/* was changed above      */
         if (f >= mf_dm) tsp = mf_dm - 1;
         if (dm[tsp] == 'y')			/* on or off?             */
            dmmode |= HOMDEC_ON;
         else    
            dmmode &= ~HOMDEC_ON;
         putcode(APBOUT);			/* via apb-bus            */
         putcode(2);				/* three words            */
         putcode(-APB_SELECT + RF_ADDR + 0x20);	/* select J-boards APchip */
         putcode(-APB_WRITE  + RF_ADDR + 0x00);	/* write address on board */
         putcode(-APB_INRWR  + RF_ADDR + dmmode);/* write value to adddres */
         curfifocount += 3;
      }
   }
}

/**************************************************************/
/**************************************************************/

/*-------------------------------*/
/*       rcvron                  */
/*-------------------------------*/
void rcvron()                                                 
{
   gate(RCVR,FALSE);
   delay(2e-7);				/*small delay to set high speed lines*/
}

/*-------------------------------*/
/*       rcvroff                 */
/*-------------------------------*/
void rcvroff()                                                
{
   gate(RCVR,TRUE);
   delay(2e-7);				/*small delay to set high speed lines*/
}

/***************************************************************/
/*              gate high speed lines                          */
/***************************************************************/

/*-------------------------------*/
/*       blankingon              */
/*-------------------------------*/
void blankingon() 
{
   gate(RCVR,FALSE);
   delay(2e-7);				/*small delay to set high speed lines*/
}

/*-------------------------------*/
/*       blankingoff             */
/*-------------------------------*/
void blankingoff() 
{
   gate(RCVR,TRUE);
   delay(2e-7);				/*small delay to set high speed lines*/
}

/*-------------------------------*/
/*       aux1on                  */
/*-------------------------------*/
void aux1on()
{
   putcode(APBOUT);
   putcode(1);
   putcode(0xaa22);
   putcode(0xba01);
}

/*-------------------------------*/
/*       aux2on                  */
/*-------------------------------*/
void aux2on()
{
   putcode(APBOUT);
   putcode(1);
   putcode(0xaa22);
   putcode(0xba02);
}

/*-------------------------------*/
/*       aux12off                */
/*-------------------------------*/
void aux12off()
{
   putcode(APBOUT);
   putcode(1);
   putcode(0xaa22);
   putcode(0xba00);
}

/*-------------------------------*/
/*       spareon                 */
/*-------------------------------*/
void spareon() 
{
   gate(SP1,TRUE);      /*set spare high speed line to high*/
}

/*-------------------------------*/
/*       spareoff                */
/*-------------------------------*/
void spareoff()
{
   gate(SP1,FALSE);     /*set spare high speed line to low */
}

/*-------------------------------*/
/*       declvlon                */
/*-------------------------------*/
void declvlon()	/* also enforces 'cw' during pulses */
{
   cwmode &= ~(HI_BND_CW + LO_BND_CW);
   putcode(APBOUT);
   putcode(2);
   putcode(-APB_SELECT + RF_ADDR + 0x20);
   putcode(-APB_WRITE  + RF_ADDR + 0x50);
   putcode(-APB_INRWR  + RF_ADDR + cwmode);
   curfifocount +=3;
}
/*-------------------------------*/
/*       declvloff                */
/*-------------------------------*/
void declvloff()
{
   if (!indirect)
      cwmode |= HI_BND_CW;
   else
      cwmode |= LO_BND_CW;
   putcode(APBOUT);
   putcode(2);
   putcode(-APB_SELECT + RF_ADDR + 0x20);
   putcode(-APB_WRITE  + RF_ADDR + 0x50);
   putcode(-APB_INRWR  + RF_ADDR + cwmode);
   curfifocount +=3;
}
/*--------------------------------------------------------------*/
/* setphase90  -  generic routine to set phase quadrent		*/
/*--------------------------------------------------------------*/
void setphase90(int device, int value)
{
   /*********************************************
   *  Set up table access for phase statement.  *
   *********************************************/
   if ((value >= t1) && (value <= t60))
   {
       value = tablertv(value);
   }
   putcode(SETPHAS90);	/* replaces TXPHASE, DECPHASE */
   if (newacq)
   {  if (device == TODEV)
      {  if (cardb) putcode(2);
         else       putcode(1);
      }
      else
      {  if (cardb) putcode(1);
         else       putcode(2);
      }
   }
   else
      putcode(device);
   putcode(value);
}

/*----------------------------------------------*/
/* set step size for Small Angle Phase shift	*/
/*----------------------------------------------*/
void stepsize(double step, int device)
{
int		dev = 1;
unsigned int	uangle;
   notinhwloop("stepsize");
   if (device==TODEV)
   {  if (cardb)
         dev = 2;
      else 
         dev = 1;
   }
   else if (device==DODEV)
   {  if (indirect)
         dev = 2;
      else
         dev = 1;
   }
   putcode(PHASESTEP);
   putcode(dev);
   if ( (dev == 1) && (fattn[0] == 256) )
   {  uangle = (unsigned int)(step/360.0 * (double)0x10000 * (double) 0x10000);
      putcode(uangle>>16);
      putcode(uangle&0xFFFF);
   }
   else
   {
      putcode(0);
      putcode( (int) (step * 256.0 / 360.0) & 0xff);
   }
}

/*----------------------------------------------*/
/* set Small Angle Phase shift = value * step	*/
/*----------------------------------------------*/
void setSAP(int rtvar, int device)
{
int	dev = 1;
int     flag;
   if (device==TODEV)
   {  if (cardb)
         dev = 2;
      else 
         dev = 1;
   }
   else if (device==DODEV)
   {  if (cardb)
         dev = 2;
      else 
         dev = 1;
   }
   flag = 0x100;
   if ((rtvar >= t1) && (rtvar <= t60))
   {
       rtvar = tablertv(rtvar);
       flag = 0x00;
   }
   if ( (dev == 1) && (fattn[0] == 256) )
      flag |= 0x200;

   putcode(SETPHASE);  /* replaces TXPHASE, DECPHASE */
   putcode(dev | flag);
   putcode(rtvar);
}

/***************************************************************/
/*      setup time-delay word for pulse and delay              */
/***************************************************************/
/*-------------------------------*/
/*           delay               */
/*-------------------------------*/
/*     note: establish 'curqui' gate pattern prior to
      call and pass delay with call.
A 'timerword' is 16 bits long, with bits 0 - 13 used for a maximum count
of 16383 and bits 14 - 15 used for the two standard time bases frequency
input.  The time base selection is:            
        0       100 nanosecond clock
        1       1 millisecond clock
The maximum number of timerwords is
The upper limit for a delay is 8190 seconds.
*/

#define	TIME1MS		0x4000
#define	TIME100NS	0x0000

void delay(double delay)       /*forward referenced*/
{
double	time,rem;
int	apbcnt_ap;
int	lpcnt;
   if (delay < 0.0)
   { char msge[128];
     sprintf(msge,"PSG: negative time delay, set to 0.0\n");
     text_error(msge);
     return;
   }
   if (delay < 1e-7) return;
   totaltime += delay;
   time = delay;
   if (time > 8190.0) time=8190.0;	/* upper limit */
   if (time < 2e-7)   time=2e-7;	/* lower limit */
   rem = time;
   putcode(EVENTN);
   putcode(curqui);
   apbcnt_ap = apc.preg;		/*precede apb data table by count - 1*/
   putcode(0);				/*save room for apb count    */
/*   apc.preg++;	*/		/*save room for apb count    */
   apb_ap = apc.preg;			/*start of apb data table    */
   if (time > 6*16.384)			/* time is large, make into loop */
   {  lpcnt = (int)((time/(3.0*16.384))+0.5);
      rem = rem - 3.0*lpcnt*16.384;
      rem = rem - 3.0*lpcnt*2e-7;	/* correct for fall through time */
      lpcnt--;
      notinhwloop("delay");
   }
   else lpcnt=0;
   while (rem > 16.0)			/* remaining 16 seconds */
   {  rem = rem - 2e-7;			/* correct for FIFO fall through time */
      putcode(16000 | TIME1MS);
      rem = rem - 16.0;
      curfifocount++;
   }
   if (rem > 1.6e-3)			/* remaining miliseconds */
   {  rem = rem - 2e-7;			/* correct for FIFO fall through time */
      putcode( (int)(rem*1e3-1) | TIME1MS );
      rem = ( rem*1e3 - (double)((int)(rem*1e3)) )/1e3;
      curfifocount++;
   }
   if (rem > 1.5e-7)
   {  rem = rem - 2e-7;			/* correct for FIFO fall through time */
      putcode( (int)(rem*1e7 + 0.50000001) );
      curfifocount++;
   }
   apbcount = ((apc.preg-apb_ap)&0x3f) | (lpcnt << 6); /*length time table*/
   apc.apcarray[apbcnt_ap] = apbcount ;	/* set apb count              */
}

/*-------------------------------*/
/*       newpreacqdelay          */
/*-------------------------------*/
void newpreacqdelay()  /*pre-acquisition delay (pad)*/          
/*         -----------    occurs once per data acquisition*/
{
int	padcnt_ap;
int	padly_cnt;
int	padly_ap;
   if (padflag && (pad>0.0) )
   {  putcode(PADLY);  /* padly checks ct <> 0 and skips delay */
      padcnt_ap = apc.preg;		/*precede apb data table by count - 1*/
      putcode(0);				/*save room for apb count    */
      padly_ap = apc.preg;			/*start of apb data table    */
      delay(pad); 				/* delay */
      if (! newacq)
      {  putcode(HALT);
         putcode(STFIFO);
         putcode(SFIFO);
      }
      padly_cnt = apc.preg-padly_ap;		/* length time table   */
      apc.apcarray[padcnt_ap] = padly_cnt;	/* set apb count              */
      padflag = FALSE;
      preacqtime = pad;
   }
}

/*-------------------------------*/
/*       hsdelay                 */
/*-------------------------------*/
/* hsdelay---like 'delay'; */
void hsdelay(double time)                                    
{
int	tsp;
  if (seqpos >= mf_hs) tsp = mf_hs - 1; else tsp = seqpos;
  if ( (hs[tsp] == 'y') || (hs[tsp] == 'Y'))
  {  time = time - hst;
     if (time < 0.0)
     {  if (ix == 1)
        {  printf(" \n");
           printf("Delay time is less than homospoil time (hst).\n");
           printf("No homospoil pulse produced.\n");
           printf(" \n");
        }
        time = time + hst;
     }   
     else
     {  if (shimset==10)
        {   putcode(APBOUT);
            putcode(1);
            putcode(0xAA40);
            putcode(0xBA08);
            delay(hst);
            putcode(APBOUT);
            putcode(1);
            putcode(0xAA40);
            putcode(0xBA00);
            curfifocount += 4;
        }
        else
        {
            putcode(APBOUT);
            putcode(1);
            putcode(0xAA22);
            putcode(0xBA02);
            delay(hst);
            putcode(APBOUT);
            putcode(1);
            putcode(0xAA22);
            putcode(0xBA00);
            curfifocount += 4;
        }
     }
  }
  if (time > 0.0) delay(time);
}

/*------------------------------*/
/*      hgradient_present       */
/*------------------------------*/
/* whether homospoil gradient available through rgradient */
int hgradient_present()
{
/*  return(1); */	/* not present for Gemini 2000 */
  return(0);		/* present for Mercury */
}

/*------------------------------*/
/*      hgradient_set           */
/*------------------------------*/
/* set value for homospoil gradient
   present for Mercury, not present for Gemini 2000 */
void hgradient_set(int gamp)
{
  if (gamp == 0) /* homospoil_off */
  {
     if (shimset==10)
     {  putcode(APBOUT);
        putcode(1);
        putcode(0xAA40);
        putcode(0xBA00);
     }
     else
     {  putcode(APBOUT);
        putcode(1);
        putcode(0xAA22);
        putcode(0xBA00);
     }
  }
  else           /* homospoil_on */
  {  if (shimset==10)
     {  putcode(APBOUT);
        putcode(1);
        putcode(0xAA40);
        putcode(0xBA08);
      }
     else
     {
        putcode(APBOUT);
        putcode(1);
        putcode(0xAA22);
        putcode(0xBA02);
     }
  }
}

/*---------------------------------*/
/*      shimgradient_set           */
/*---------------------------------*/
/* set value of shimdac gradient for Mercury */
void shimgradient_set(char gid, int val)
{
  int coildac;

  switch (gid)
  {
    case 'z': case 'Z':
      coildac = 1;
      break;
    case 'x': case 'X':
      coildac = 8;
      break;
    case 'y': case 'Y':
      coildac = 9;
      break;
    default:
      coildac = 1;
      break;
  }
  switch (gid)
  {
    case 'z': case 'Z': case 'x': case 'X': case 'y': case 'Y':
      putcode( APBOUT );
      putcode( 3 );
      putcode( 0xaa40 );
      putcode( 0xba00 + (coildac >> 2) );
      putcode( 0x9a00 + ((coildac & 3) << 6) + ((val >> 8) & 0xf) );
      putcode( 0xba00 + (val & 0xff) );
      break;
    default: ;
  }
}

/***************************************************************/
/*      decoupler, homo-spoil, pulse, simul-pulses             */
/***************************************************************/

/*-------------------------------*/
/*       rgpulse                 */
/*-------------------------------*/
/* removes rf phase gates from 'curqui' and uses the phase selected
  by a modulo four of the 'phaseptr' argument.
  register 5 contains selected phase gates */

void rgpulse(double width, int phaseptr, double rx1, double rx2)
{
   if (width > 0.0)
   {  if (cardb)
      {  txonx = CTXON;			/* carbon or BB transmitter*/
         gate(RFPC270,FALSE);		/* carbon or BB rf phase = zero */
      }
      else 
      {  txonx = HTXON;			/* hydrogen transmitter*/
         gate(RFPH270,FALSE);		/* hydrogen rf phase = zero */
      }
      if (width >= 3e-7)
      {  width -= 1e-7;
         rx2   += 1e-7;
      }
      setphase90(TODEV,phaseptr);
      gate(RCVR,TRUE);
      delay(rx1);
      gate(txonx,TRUE);
      delay(width);
      gate(txonx,FALSE);
      delay(rx2);
      gate(RCVR,FALSE);
      delay(2e-7);			/*small delay to set high speed lines*/
   }
}

/*-------------------------------*/
/*       decrgpulse              */
/*-------------------------------*/
void decrgpulse(width, phaseptr, rx1,rx2)   
double	width;
int	phaseptr;
double	rx1,rx2;   
{
   if (width > 0.0)
   {  if (!indirect)
      {  gate(DC270,FALSE);		/*dc phase = zero */
         txonx = HTXON;
      }
      else
      {  gate(RFPC270,FALSE);		/*dc phase = zero */
         txonx = CTXON;
      }
      if (width >= 3e-7)
      {  width -= 1e-7;
         rx2   += 1e-7;
      }
      setphase90(DODEV,phaseptr);
      gate(RCVR,TRUE);
      delay(rx1);
      gate(txonx,TRUE);			/*turn decoupler on*/
      delay(width);			/*wait out pulse width*/
      gate(txonx,FALSE);		/*turn decoupler off*/
      delay(rx2);
      gate(RCVR,FALSE);
      delay(2e-7);			/*small delay to set high-speed-lines*/
   }
}

/*-------------------------------*/
/*       simpulse                */
/*-------------------------------*/
/* procedure 'simpulse' performs a simultaneous pulse on transmitter	*/
/* and decoupler.  the pulses are 'centered' so	that the shorter of the	*/
/* two pulses occurs exactly in the center of the longer pulse		*/
/* Note that t1 and ph1 always are the observe, t2 and ph2 always are	*/
/*      the decoupler, even if indirect is true				*/

void simpulse(t1,t2, ph1,ph2, rx1,rx2)
double	t1,t2;
int	ph1,ph2;
double	rx1,rx2;
{
   gate(RFPC270,FALSE);
   gate(DC270,FALSE);
   setphase90(TODEV,ph1);
   setphase90(DODEV,ph2);
   gate(RCVR,TRUE);
   delay(rx1);

   if (t1 >= 3e-7) t1 -= 1e-7;
   if (t2 >= 3e-7) t2 -= 1e-7;
   rx2 += 1e-7;


   if (!indirect)
   {  if (t2>t1 && !indirect)
      {  gate(HTXON,TRUE);        delay((t2-t1)/2.0);
         gate(CTXON,TRUE);        delay(t1);
         gate(CTXON,FALSE);       delay((t2-t1)/2.0);
         gate(HTXON,FALSE);
      }
      else
      {  gate(CTXON,TRUE);        delay((t1-t2)/2.0);
         gate(HTXON,TRUE);        delay(t2);
         gate(HTXON,FALSE);       delay((t1-t2)/2.0);
         gate(CTXON,FALSE);
      }
   }
   else
   {  if (t2>t1)
      {  gate(CTXON,TRUE);        delay((t2-t1)/2.0);
         gate(HTXON,TRUE);	  delay(t1);
         gate(HTXON,FALSE);	  delay((t2-t1)/2.0);
         gate(CTXON,FALSE);
      }
      else
      {  gate(HTXON,TRUE);        delay((t1-t2)/2.0);
         gate(CTXON,TRUE);        delay(t2);
         gate(CTXON,FALSE);       delay((t1-t2)/2.0);
         gate(HTXON,FALSE);
      }

   }
   delay(rx2);
   gate(RCVR,FALSE);
   delay(2e-7);		/*small delay to set high-speed-lines*/
}
/*-------------------------------*/
/*       obs_pw_ovr              */
/*-------------------------------*/
void obs_pw_ovr(longpulse)
int	longpulse;
{
int	bits;
   if (cardb)
      bits = 0x80;
   else
      bits = 0x40;
   putcode(APBOUT);
   putcode(2);
   putcode(-APB_SELECT + RF_ADDR + 0x20);	/* on J-board                */
   putcode(-APB_WRITE  + RF_ADDR + 0x50);	/* obs/tune register	     */
   if (longpulse)
   {  cwmode |= bits;
      putcode(-APB_INRWR  + RF_ADDR + cwmode);
   }
   else
   {  cwmode &= ~bits;
      putcode(-APB_INRWR  + RF_ADDR + cwmode);
   }
}

/*-------------------------------*/
/*       dec_pw_ovr              */
/*-------------------------------*/
void dec_pw_ovr(longpulse)
int	longpulse;
{
int	bits;
   if (indirect)
      bits = 0x80;
   else
      bits = 0x40;
   putcode(APBOUT);
   putcode(2);
   putcode(-APB_SELECT + RF_ADDR + 0x20);	/* on J-board                */
   putcode(-APB_WRITE  + RF_ADDR + 0x50);	/* obs/tune register	     */
   if (longpulse)
   {  cwmode |= bits;
      putcode(-APB_INRWR  + RF_ADDR + cwmode);	/* select low band observe   */
   }
   else
   {  cwmode &= ~0x40;
      putcode(-APB_INRWR  + RF_ADDR + cwmode);	/* select low band observe   */
   }
}

/***************************************************************/
/*          APbus  data initialization                         */
/***************************************************************/

/*-------------------------------*/
/*       initdmfapb              */
/*-------------------------------*/
void initdmfapb(double dmf_local)                                             
/* set the decoupler modulation frequency        */
/* from 0 to 2 MHz, step=???, driven by 40 MHZ, 23 bits */
/* so we mult/div by 78125/16384 */
{
double	stepsize;
int	byteval;
   stepsize = 78125.0/16384.0;
   byteval  = (int)(dmf_local/stepsize);
   putcode(-APB_SELECT + RF_ADDR + 0x20);
   putcode(-APB_WRITE  + RF_ADDR + 0x22);
   putcode(-APB_INRWR  + RF_ADDR + (byteval&0xFF) );
   putcode(-APB_SELECT + RF_ADDR + 0x20);
   putcode(-APB_WRITE  + RF_ADDR + 0x20);
   putcode(-APB_INRWR  + RF_ADDR + ((byteval>> 8)&0xFF) );
   putcode(-APB_SELECT + RF_ADDR + 0x20);
   putcode(-APB_WRITE  + RF_ADDR + 0x24);
   putcode(-APB_INRWR  + RF_ADDR + ((byteval>>16)&0xFF) );
   curfifocount += 9;
}

#define FACTOR  (double)(0x40000000/10e6)
#define	LOCK_SEL	0x030
#define LOCK_PLLM	0x020
#define	LOCK_PLLA	0x022
#define	LOCK_STR	0x02e
/*------------------------------*/
/*	setoffset		*/
/*  number=offset*(2^32)/40e6   */
/*------------------------------*/
void setoffset(double offset, int address)
{
int	bits32, llbyte, hlbyte, lhbyte, hhbyte;
int	llbyte2, hlbyte2, lhbyte2, hhbyte2;
double	dqdfrq = 0.0;

   bits32 = (int) (offset * FACTOR);
   llbyte2 = ((bits32)&0xFF);
   hlbyte2 = ((bits32 >>  8)&0xFF);
   lhbyte2 = ((bits32 >> 16)&0xFF);
   hhbyte2 = ((bits32 >> 24)&0xFF);
   if (go_dqd == 1)
      find_dqdfrq(&dqdfrq,offset,address);
   if (fabs(dqdfrq) > 0.01)
      bits32 = (int) ((offset + dqdfrq) * FACTOR);
   llbyte = ((bits32)&0xFF);
   hlbyte = ((bits32 >>  8)&0xFF);
   lhbyte = ((bits32 >> 16)&0xFF);
   hhbyte = ((bits32 >> 24)&0xFF);

   if (address == HB_XMT_SEL)
   {  hb_dds_bits = bits32;
/*  printf("hb_dds_bits=0x%08x\n",hb_dds_bits); */
   }

   if ( (address != HB_XMT_SEL) || (fattn[0] != 256) )
   {  
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x0A);	/* DDS1 AMC register */
      putcode(-APB_INRWR  + RF_ADDR + 0x8F);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x08);	/* DDS1 SMC register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x00);	/* DDS1 PIRA 0-7 */
      putcode(-APB_INRWR  + RF_ADDR + llbyte);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x01);	/* DDS1 PIRA 8-15 */
      putcode(-APB_INRWR  + RF_ADDR + hlbyte);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x02);	/* DDS1 PIRA 16-23 */
      putcode(-APB_INRWR  + RF_ADDR + lhbyte);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x03);	/* DDS1 PIRA 24-31 */
      putcode(-APB_INRWR  + RF_ADDR + hhbyte);

      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x04);	/* DDS1 PIRB 0-7 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x05);	/* DDS1 PIRB 8-15 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x06);	/* DDS1 PIRB 16-23 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x07);	/* DDS1 PIRB 24-31 */
      putcode(-APB_INRWR  + RF_ADDR);

      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x1A);	/* DDS2 AMC */
      putcode(-APB_INRWR  + RF_ADDR + 0xAF);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x18);	/* DDS2 SMC */
      if (address == LOCK_SEL)
         putcode(-APB_INRWR  + RF_ADDR + 0x08);
      else
         putcode(-APB_INRWR  + RF_ADDR + 0x0A);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x10);	/* DDS2 PIRA 0-7 */
      putcode(-APB_INRWR  + RF_ADDR + llbyte2);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x11);	/* DDS2 PIRA 8-15 */
      putcode(-APB_INRWR  + RF_ADDR + hlbyte2);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x12);	/* DDS2 PIRA 16-23 */
      putcode(-APB_INRWR  + RF_ADDR + lhbyte2);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x13);	/* DDS2 PIRA 24-32 */
      putcode(-APB_INRWR  + RF_ADDR + hhbyte2);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x14);	/* DDS2 PIRB 0-7 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x15);	/* DDS2 PIRB 8-15 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x16);	/* DDS2 PIRB 16-23 */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x17);	/* DDS2 PIRB 24-31 */
      if (address == LOCK_SEL)
         putcode(-APB_INRWR  + RF_ADDR + ((int)(lockphase*256.0/360.0)&0xFF));
      else
         putcode(-APB_INRWR  + RF_ADDR);

      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x1E);	/* DDS2 AHC */
      putcode(-APB_INRWR  + RF_ADDR);

      putcode(-APB_SELECT + RF_ADDR + address);
      putcode(-APB_WRITE  + RF_ADDR + 0x0E);	/* DDS1 AHC register */
      putcode(-APB_INRWR  + RF_ADDR);
      curfifocount += 66;
   }
   else
   {
      putcode(-APB_SELECT + 0x108);		/* DDS1 SMC register */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_SELECT + 0x10A);		/* DDS1 AMC register */
      putcode(-APB_WRITE  + 0x18F);
      putcode(-APB_SELECT + 0x10c);		/* DDS1 ARR register */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_SELECT + 0x100);
      putcode(-APB_WRITE  + 0x100 + llbyte);	/* DDS1 PIRA 0-7 */
      putcode(-APB_INRWR  + 0x100 + hlbyte);	/* DDS1 PIRA 8-15 */
      putcode(-APB_INRWR  + 0x100 + lhbyte);	/* DDS1 PIRA 16-23 */
      putcode(-APB_INRWR  + 0x100 + hhbyte);	/* DDS1 PIRA 24-31 */

      putcode(-APB_INRWR  + 0x100);		/* DDS1 PIRB 0-7 */
      putcode(-APB_INRWR  + 0x100);		/* DDS1 PIRB 8-15 */
      putcode(-APB_INRWR  + 0x100);		/* DDS1 PIRB 16-23 */
      putcode(-APB_INRWR  + 0x100);		/* DDS1 PIRB 24-31 */

      putcode(-APB_SELECT + 0x118);		/* DDS2 SMC */
      putcode(-APB_WRITE  + 0x10A);
      putcode(-APB_SELECT + 0x11A);		/* DDS2 AMC */
      putcode(-APB_WRITE  + 0x18F);
      putcode(-APB_SELECT + 0x11C);		/* DDS2 ARR */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_SELECT + 0x110);
      putcode(-APB_WRITE  + 0x100 + llbyte2);	/* DDS2 PIRA 0-7 */
      putcode(-APB_INRWR  + 0x100 + hlbyte2);	/* DDS2 PIRA 8-15 */
      putcode(-APB_INRWR  + 0x100 + lhbyte2);	/* DDS2 PIRA 16-23 */
      putcode(-APB_INRWR  + 0x100 + hhbyte2);	/* DDS2 PIRA 24-32 */

      putcode(-APB_INRWR  + 0x100);		/* DDS2 PIRB 0-7 */
      putcode(-APB_INRWR  + 0x100);		/* DDS2 PIRB 8-15 */
      putcode(-APB_INRWR  + 0x100);		/* DDS2 PIRB 16-23 */
      putcode(-APB_INRWR  + 0x100);		/* DDS2 PIRB 24-31 */

      putcode(-APB_SELECT + 0x11E);		/* DDS2 AHC */
      putcode(-APB_WRITE  + 0x100);

      putcode(-APB_SELECT + 0x10E);		/* DDS1 AHC register */
      putcode(-APB_WRITE  + 0x100);
      curfifocount += 30;
   }
}

/*------------------------------*/
/*   lk_zero_dds_acc            */
/*------------------------------*/
void lk_zero_dds_acc()
{
   putcode(APBOUT);
   putcode(11);         /* n-1 */
   putcode(-APB_SELECT + RF_ADDR + LOCK_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x0C);	/* DDS1 ARR register */
   putcode(-APB_INRWR  + RF_ADDR);
   putcode(-APB_SELECT + RF_ADDR + LOCK_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x1C);	/* DDS2 ARR register */
   putcode(-APB_INRWR  + RF_ADDR);
   putcode(-APB_SELECT + RF_ADDR + LOCK_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x1E);	/* DDS2 AHC */
   putcode(-APB_INRWR  + RF_ADDR);
   putcode(-APB_SELECT + RF_ADDR + LOCK_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x0E);	/* DDS1 AHC register */
   putcode(-APB_INRWR  + RF_ADDR);
   curfifocount += 12;
}

/*------------------------------*/
/*      zero_dds_acc            */
/*------------------------------*/
void zero_dds_acc()
{
   putcode(APBOUT);
   if ( fattn[0] != 256 )
   {  if (homdec)
      {  putcode(47);		/* n-1 */
         curfifocount += 48;
      }
      else
      {  putcode(35);
         curfifocount += 36;
      }
   }
   else
   {  if (homdec)
      {  putcode(36);		/* n-1 */
         curfifocount += 37;
      }
      else
      {  putcode(24);
         curfifocount += 25;
      }
   }

/* do hi-band */
   if ( fattn[0] != 256 )
   {  putcode(0xAA00 | HB_XMT_SEL);
      putcode(0xBA00 | 0x14);
      putcode(0x9A00);
      putcode(0xAA00 | HB_XMT_SEL);
      putcode(0xBA00 | 0x15);
      putcode(0x9A00);
      putcode(0xAA00 | HB_XMT_SEL);
      putcode(0xBA00 | 0x16);
      putcode(0x9A00);
      putcode(0xAA00 | HB_XMT_SEL);
      putcode(0xBA00 | 0x17);
      putcode(0x9A00);
      putcode(-APB_SELECT + RF_ADDR + HB_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x0C);	/* DDS1 ARR register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + HB_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x1C);	/* DDS2 ARR register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + HB_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x1E);	/* DDS2 AHC register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + HB_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x0E);	/* DDS1 AHC register */
      putcode(-APB_INRWR  + RF_ADDR);
   }
   else
   {  putcode(-APB_SELECT + 0x114);		/* DDS2 PIRB 0x14 - 0x17 */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_INRWR  + 0x100);
      putcode(-APB_INRWR  + 0x100);
      putcode(-APB_INRWR  + 0x100);
      putcode(-APB_SELECT + 0x10C);		/* DDS1 ARR register */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_SELECT + 0x11C);		/* DDS2 ARR register */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_SELECT + 0x11E);		/* DDS2 AHC register */
      putcode(-APB_WRITE  + 0x100);
      putcode(-APB_SELECT + 0x10E);		/* DDS1 AHC register */
      putcode(-APB_WRITE  + 0x100);
   }
/* do lo-band */
   putcode(-APB_SELECT + RF_ADDR + LB_XMT_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x0C);	/* DDS1 ARR register */
   putcode(-APB_INRWR  + RF_ADDR);
   putcode(-APB_SELECT + RF_ADDR + LB_XMT_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x1C);	/* DDS2 ARR register */
   putcode(-APB_INRWR  + RF_ADDR);
   putcode(-APB_SELECT + RF_ADDR + LB_XMT_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x1E);	/* DDS2 AHC register */
   putcode(-APB_INRWR  + RF_ADDR);
   putcode(-APB_SELECT + RF_ADDR + LB_XMT_SEL);
   putcode(-APB_WRITE  + RF_ADDR + 0x0E);	/* DDS1 AHC register */
   putcode(-APB_INRWR  + RF_ADDR);
/* do homo decoupler */
   if (homdec)
   {  putcode(-APB_SELECT + RF_ADDR + HOMO_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x0C);	/* DDS1 ARR register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + HOMO_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x1C);	/* DDS2 ARR register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + HOMO_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x1E);	/* DDS2 AHC register */
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + HOMO_XMT_SEL);
      putcode(-APB_WRITE  + RF_ADDR + 0x0E);	/* DDS1 AHC register */
      putcode(-APB_INRWR  + RF_ADDR);
   }
}

/*------------------------------*/
/*	lk_sample		*/
/*------------------------------*/
void lk_sample()
{
   notinhwloop("lk_sample");
   putcode(SMPL_HOLD);
   putcode(SMPL);
}
/*------------------------------*/
/*	lk_hold			*/
/*------------------------------*/
void lk_hold()
{
   notinhwloop("lk_hold");
   putcode(SMPL_HOLD);
   putcode(HOLD);
}
/*------------------------------*/
/*    set_grad_shim_relay	*/
/*------------------------------*/
void set_grad_shim_relay()
{
char	tn[MAXSTR];
   getparm("tn","string",CURRENT,tn,MAXSTR); 
   if (strcmp(tn,"lk") == 0)
   {  if ( ! automated )
      {  text_error("No spinner controller present, cannot set gradient relay");
         text_error("Check 'config'");
         text_error("Or set tn='H2' and switch the cables to the probe for");
         text_error("low-band and lock manually");
         psg_abort(1);
      }
   }
   if ( ! strcmp(tn,"lk"))
   {  lk_hold();
      putcode(APBOUT);				/* via apb-bus            */
      putcode(2);				/* three words            */
      putcode(-APB_SELECT + RF_ADDR + 0x20);	/* select J-boards APchip */
      putcode(-APB_WRITE  + RF_ADDR + 0x80);	/* write address on board */
      putcode(-APB_INRWR  + RF_ADDR + 0xA0);	/* Lock XMTR & RCVR off */
      putcode(SET_GR_RELAY);
      putcode(1);
   }
}
/*------------------------------*/
/*	setlockfreq		*/
/*------------------------------*/
void setlockfreq()
{
int	r=1 ,v=1;
double	lk_offset;
   if      (xltype == 200) { r=1;  v=1;  }
   else if (xltype == 300) { r=13; v=23; }
   else if (xltype == 400) { r=13; v=33; }

   lk_offset = (lockfreq - 20.0 * v / r) * 1e6;
/* first set the phase lock loop */
   if (r!=1 && v!=1)
   {
      putcode(-APB_SELECT + RF_ADDR + LOCK_SEL  );
      putcode(-APB_WRITE  + RF_ADDR + LOCK_PLLM );
      if (v < 90)
         putcode(-APB_INRWR + RF_ADDR + (((v-1)|0x80)&0xFF) );
      else
	 putcode(-APB_INRWR + RF_ADDR + ((v/10 - 1)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + LOCK_SEL  );
      putcode(-APB_WRITE  + RF_ADDR + LOCK_PLLA );
      putcode(-APB_INRWR  + RF_ADDR + ((v%10 + (r-1)*16)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + LOCK_SEL  );
      putcode(-APB_WRITE  + RF_ADDR + LOCK_STR  );
      putcode(-APB_INRWR  + RF_ADDR );
      curfifocount += 9;
   }
/* printf("LOCK: r=%d, v=%d, error=xxxxxxxxf, offset=%12.4f, sfrq=%g\n",
			r,v,lk_offset,lockfreq); */
   setoffset(lk_offset,LOCK_SEL);
}

#define LB_PLL_SEL	0x08
#define LB_PLL_M1	0x20
#define LB_PLL_A1	0x22
#define LB_PLL_STR1	0x2e
#define LB_PLL_M2	0x30
#define LB_PLL_A2	0x32
#define LB_PLL_STR2	0x3e
/*-------------------------------*/
/*       setlbfreq               */
/*-------------------------------*/
void setlbfreq(double freq, double off)
{
double	offset;
double	ref_freq;
double	LO_freq, xabs, best_xabs;
int	test_r, test_v, r=1, v=1;
   if (bb_refgen)
   {  /* test has shown that 90 < r <= 150		*/
      /*                     98 < v <  221		*/
      /* when  20 < freq < 161 for BB RefGen 200-400 MHz*/
      best_xabs = 1000.0;
      for (test_r=150; test_r>90; test_r--)
         for (test_v=221; test_v>98; test_v--)
         {  LO_freq=360.0 * test_v / test_r - 360.0;
            xabs = fabs(LO_freq-freq-off/1e6-10.65);
            if (xabs < best_xabs)
            {
                best_xabs=xabs;
                r=test_r; v=test_v;
            }
         }
      offset = (360.0*v/r-360.0-freq)*1e6 - off;
   }
   else
   {  /* test has shown that  8 < r <= 16		*/
      /*                     28 < v <=110		*/
      /* when nucleus is 13C/31P for 200, 300, 400 MHz	*/
      best_xabs = 1000.0;
      if (xltype == 200) ref_freq=10.0;
      else               ref_freq=20.0;
      for (test_r=16; test_r>8; test_r--)
         for (test_v=110; test_v>28; test_v--)
         {  LO_freq = ref_freq * test_v / test_r;
            xabs = fabs(LO_freq-freq-off/1e6-10.65);
            if (xabs < best_xabs)
            {  best_xabs = xabs;
               r = test_r;
               v = test_v;
            }
         }
      offset = (ref_freq * v/r - freq)*1e6 - off;
    }
    lb_offset = offset; /* save for dqd */
/* printf("LOW: r=%d, v=%d, error=%f, offset=%12.4f sfrq=%g\n",
		r,v,best_xabs,offset,freq); */
   if (bb_refgen)
   {  putcode(-APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + LB_PLL_M1);
      putcode(-APB_INRWR  + RF_ADDR + ((r/10 - 1)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + LB_PLL_A1);
      putcode(-APB_INRWR  + RF_ADDR + ((r%10)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + LB_PLL_STR1);
      putcode(-APB_INRWR  + RF_ADDR);
      putcode(-APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + LB_PLL_M2);
      putcode(-APB_INRWR  + RF_ADDR + ((v/10 - 1)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + LB_PLL_A2);
      putcode(-APB_INRWR  + RF_ADDR + ((v%10)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + LB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + LB_PLL_STR2);
      putcode(-APB_INRWR  + RF_ADDR);
      curfifocount += 18;
   }
   else
   {  putcode(0xAA10);
      putcode(0xBA22); /* select M register */
      putcode(0x9A00 | (v+127) );	/* (M-1) + 128 for sign bit */
      putcode(0xAA10);
      putcode(0xBA2e);   /* select R+A register */
      putcode(0x9A00 | (r-1)*16 );
      putcode(0xAA10);
      putcode(0xBA20);
      putcode(0x9a00);   /* anything for strobe */
      curfifocount += 9;

      putcode(0xAA12);
      if ((int)freq < (xltype/3) )
         putcode(0xba00);	/* select 13C filter */
      else
         putcode(0xBA01);	/* select 31P filter */
      curfifocount += 2;
   }

   setoffset(offset,LB_XMT_SEL); 
}

#define HB_PLL_SEL	0x08
#define HB_PLL_M1	0x40
#define HB_PLL_A1	0x42
#define HB_PLL_STR	0x4e
/*-------------------------------*/
/*       sethbfreq               */
/*-------------------------------*/
void sethbfreq(freq,off)
double	freq;
double	off;
{
double	offset;
double	LO_freq, xabs, best_xabs;
int	test_r, test_v, r=1, v=1;

/* first lets find the best r and v values	*/
/* test has shown that  8 < r <= 16		*/
/*                     85 < v <=310		*/
/* when nucleus is 19F/1H for 200, 300, 400 MHz	*/
   best_xabs = 1000.0;
   for (test_r=16; test_r>8; test_r--)
      for (test_v=310; test_v>85; test_v--)
      {  LO_freq = 20.0 * test_v / test_r;
         xabs = fabs(LO_freq-freq-off/1e6-10.65);
         if (xabs < best_xabs)
         {  best_xabs = xabs;
            r = test_r;
            v = test_v;
         }
      }

   offset = (20.0*v/r-freq)*1e6-off;
   hr=r; hv=v;	/* save for homodec. freq. calculations */
   hb_offset = offset; /* save for dqd */
/* printf("HI: r=%d, v=%d, error=%f, offset=%12.4f, sfrq=%g\n",
			r,v,best_xabs,offset,freq); */
   if (bb_refgen)
   {  putcode(-APB_SELECT + RF_ADDR + HB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + HB_PLL_M1);
      putcode(-APB_INRWR  + RF_ADDR + ((v/10 - 1)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + HB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + HB_PLL_A1);
      putcode(-APB_INRWR  + RF_ADDR + (((v%10)+(r-1)*16)&0xFF) );
      putcode(-APB_SELECT + RF_ADDR + HB_PLL_SEL);
      putcode(-APB_WRITE  + RF_ADDR + HB_PLL_STR);
      putcode(-APB_INRWR  + RF_ADDR);
      curfifocount += 9;
   }
   else
   {  putcode (0xAA10);
      putcode (0xBA80);
      putcode (0x9a00 | (v/10 - 1));
      putcode (0xAA10);
      putcode (0xBA10);   /* select R+A register */
      putcode (0x9A00 | ((r-1)*16 + v%10) );
      putcode (0xAA10);
      putcode (0xBA50);
      putcode (0x9a00);   /* anything for strobe */
      curfifocount += 9;
   }

   setoffset(offset,HB_XMT_SEL);
}

/*-------------------------------*/
/*       initapbvalues           */
/*-------------------------------*/
/* output n consecutive values to analog port */
/* with value count less one                  */
/* followed by values to be output.           */

void initapbvalues()                                           
{
int	apbcnt_ap;
double	hd_off;
/*lock-transmitter offset frequency, ----------------------------------------*/
/* if (ix != 1) 
 * { status (0);
 *   return;
 * } */

   if ((ix == 1) && suflag)
   {  putcode(LK_SYNC);
      apbcnt_ap = apc.preg;	/*apb data table is preceded by count - 1*/
      putcode(0);				/*save room for apb count    */
      apb_ap = apc.preg;			/*start of apb data table    */
      setlockfreq();				/*set 2H frequency           */
      lk_zero_dds_acc();
      apbcount = apc.preg-apb_ap;		/*length of apb data table   */
      apc.apcarray[apbcnt_ap] = apbcount - 1;	/*set apb count              */
      if (newacq)
      {  putcode(SAVELKPH);
         putcode((int)lockphase);		/* put phase in struct       */
	 if ( ! strncmp(amptype,"aa",2) )
	    putcode(11);
	 else if ( ! strncmp(amptype,"bb",2) )
	    putcode(22);
	 else if ( ! strncmp(amptype,"cc",2) )
	    putcode(33);
	 else
	 {
            text_error("PSG: Illegal value for amptype");
	    text_error("     Run 'config' as vnmr1 to correct");
            exit(1);
         }
      }
   }
   putcode(APBOUT);
   apbcnt_ap = apc.preg;	/*apb data table is preceded by count - 1*/
   putcode(0);					/*save room for apb count    */
/*   apc.preg++;	*/			/*save room for apb count    */
   apb_ap = apc.preg;				/*start of apb data table    */
/*select filter, funny scale, error small if fb small, 9.5% near 25K         */
/* this routine also sets a bit pattern for rcvr_cntl for 55k filter         */
   dofiltercontrol();				/* 2 words, BB or dual       */
/*decoupler modulation frequency---------------------------------------------*/
   if (cardb)
   {  cwmode = HI_BND_CW;			/* init cwmode               */
      dmmode = DEC_HI_BND;			/* init dmmode               */
      setlbfreq(sfrq,tof-tof_init);		/* set low band frequency    */
      setpower(tpwr,TODEV);			/* set low band power level  */
      sethbfreq(dfrq,dof-dof_init);		/* set hi  band frequency    */
      setpower(dhp, DODEV);			/* set hi band power level   */
      if (fattn[0])
         setpwrf(dpwrf,DODEV);			/* set fine power, if present*/
      putcode(-APB_SELECT + RF_ADDR + 0x20);	/* on J-board                */
      putcode(-APB_WRITE  + RF_ADDR + 0x50);	/* obs/tune register	     */
      putcode(-APB_INRWR  + RF_ADDR + cwmode);	/* select low band observe   */
   }
   else
   {  cwmode = OBS_HI_BND;
      dmmode = 0;
      sethbfreq(sfrq,tof-tof_init);		/* set hi band frequency     */
      setlbfreq(sfrq/3.1,0.0);			/* set lb to something       */
      if (fattn[0])
         setpwrf(tpwrf,TODEV);			/* set fine power, if present*/
      setpower(tpwr,TODEV);			/* set hi band power level   */
      rcvr_cntl |= 0x11;			/* select hi band in rcvr    */
						/* and 10 dB attn            */
      if (indirect)
      {  setlbfreq(dfrq,dof-dof_init);		/* set low band frequency    */
         setpower(dhp,DODEV);
      }
      else
      {  if (homdec)
         { 
            hd_off=(20.0*hv/hr-dfrq)*1e6-dof+dof_init;
            setoffset(hd_off,HOMO_XMT_SEL);/* set homo decoupler offset */
            homodecpwr(dhp);			/*and homo-decoupler power   */
         }
      }
      putcode(-APB_SELECT + RF_ADDR + 0x20);	/* on J-board                */
      putcode(-APB_WRITE  + RF_ADDR + 0x50);	/* obs/tune register	     */
      putcode(-APB_INRWR  + RF_ADDR + cwmode); /* select highband obs */
   }
/*finalize setup-------------------------------------------------------------*/
   apbcount = apc.preg-apb_ap;			/*length of apb data table   */
   apc.apcarray[apbcnt_ap] = apbcount - 1;	/*set apb count              */
   zero_dds_acc();
   status(0);					/*set dmmode and dmf now     */
   lastcw = -1;					/* reset, so first status in */
   lastdmm = -1;				/* sequence is done always   */

/* set rfchnuclei parameter with nucleus labels */
 
   char nucleiNameStr[MAXSTR], str[MAXSTR], str2[MAXSTR];

   P_getstring(CURRENT, "tn", str,  1, MAXSTR);
   if (strcmp(str, "") == 0)
      strcpy(str, "-");

   P_getstring(CURRENT, "dn", str2, 1, MAXSTR);
   if (strcmp(str2, "") == 0)
      strcpy(str2, "-");

   if (cardb == 1)
   {
      if ((2.0*dfrq) >= xltype)
      {
         sprintf(nucleiNameStr,"'%s %s'",str2,str);
      }
      else
      {
         sprintf(nucleiNameStr,"'- %s'",str);
      }
   }
   else
   {
       if (indirect == 1)
       {
          sprintf(nucleiNameStr,"'%s %s'",str,str2);
       }  
       else
       {
          sprintf(nucleiNameStr,"'%s -'",str);
       }
   }


   if (P_getstring(CURRENT, "rfchnuclei", str, 1, MAXSTR) >= 0)
   {
      sprintf(str2,"%s",nucleiNameStr);
      putCmd("rfchnuclei = %s", nucleiNameStr);
   }
}

/****************************************************************/
/*    low  memory data and parameter initialization             */
/****************************************************************/

/*-------------------------------*/
/*       initdecphasetab         */
/*-------------------------------*/
void initdecphasetab()                                        
/*   set up table for decoupler phases*/
{
   if (newacq)
   {  putcode(SETPHATTR);
   }
   if (!indirect)
   {  if (newacq) putcode(1);
      if (newacq) putcode(0); putcode(0);
      if (newacq) putcode(0); putcode(DC270);
      if (newacq) putcode(0); putcode(DC180);
      if (newacq) putcode(0); putcode(DC90);
      if (newacq)
      {  putcode(0);
         putcode(DC270);
         if (fattn[0]==256)
            putcode(0x1);
         else
            putcode(0xA);
         putcode(0x18);
      }
   }
   else
   {  if (newacq) putcode(2);
      if (newacq) putcode(0); putcode(0);
      if (newacq) putcode(0); putcode(RFPC270);
      if (newacq) putcode(0); putcode(RFPC180);
      if (newacq) putcode(0); putcode(RFPC90);
      if (newacq)
      {  putcode(0);
         putcode(RFPC270);
         if (fattn[1]==256)
            putcode(0x2);
         else
            putcode(0xA);
         putcode(0x00);
      }
   }
}

/*-------------------------------*/
/*       initobsphasetab         */
/*-------------------------------*/
void initobsphasetab()                                        
/*   setup obspulse rfphase pattern table */
{
   if (newacq)
   {  putcode(SETPHATTR);
   }
   if (cardb)			/*for lowband */
   {  if (newacq) putcode(2);
      if (newacq) putcode(0); putcode(0);
      if (newacq) putcode(0); putcode(RFPC270);
      if (newacq) putcode(0); putcode(RFPC180);
      if (newacq) putcode(0); putcode(RFPC90);
      if (newacq)
      {  putcode(0);
         putcode(RFPC270);
         putcode(0xA);
         putcode(0x00);
      }
   }
   else				/*for hydrogen*/
   {  if (newacq) putcode(1);
      if (newacq) putcode(0); putcode(0);
      if (newacq) putcode(0); putcode(RFPH270);
      if (newacq) putcode(0); putcode(RFPH180);
      if (newacq) putcode(0); putcode(RFPH90);
      if (newacq)
      {  putcode(0);
         putcode(RFPH270);
         if (fattn[0]==256)
            putcode(0x1);
         else
            putcode(0xA);
         putcode(0x18);
      }
   }
}

/*-------------------------------*/
/*       inittemps               */
/*-------------------------------*/
void inittemps()                                              
{
int	i;
   for (i=1; i<=16; i++)
      putcode(0);
}

/*-------------------------------*/
/*       initquistates           */
/*-------------------------------*/
void initquistates()                                          
/*   initialize quiescent states*/
{
/* establish start and current quiescent states */
   curqui = 0;
   seqpos = 1;
   startqui = curqui;
   apc.apcarray[squi_ap] = startqui;
}

/*-------------------------------*/
/*       initparms              */
/*-------------------------------*/
#define	CIDV	0
#define	ACODEB	11
#define	ACODEP	12
void initparms()                                             
/*get experimental parameters and setup data pointers */
{
char	*sh_name,*get_shimname();
int	codeoffset,count,f,index,rtv_i;
double	tmp,getval();
/*init control section of 'acode'*/
  if (!newacq)
  {  apc.preg = CIDV;
     npr_ap = apc.preg;			/*pointer to npr*/
     putrealdata(tot_np+0.5);		/*np        loc 13*/
     putrealdata(nt);			/*nt        loc 15*/
     ct_ap = apc.preg;
     putcode(0);			/*ct        loc 17*/
     putcode(0);			/*ct        loc x*/
     putcode(0); putcode(0);		/*(long)isum,rsum*/
     putcode(0); putcode(0);
     putcode(0); putcode(0);		/*total data points, set by apint*/
     putcode(0); putcode(0);		/*long pointer, used by apint*/
     putcode(0); putcode(0);		/*stmar     loc x*/ 
     putcode(0); putcode(0);		/*stmcr     loc x*/ 
     f = (int)(nt + 0.5);
     rtv_i = (f - ((int)ss % f)) % f;
     putcode(rtv_i >> 16);
     putcode(rtv_i & 0xFFFF);		/*rtvptr    loc x*/
     putcode(0); putcode(0);		/*elemid    loc x*/
     squi_ap = apc.preg+1;
     putcode(0); putcode(0);		/*startqui  loc x*/
     putcode(IDC);			/*id/vers   loc 0*/
     putcode( sizeof(struct lc) / 2);	/*offset to auto structure*/
   
     cttime = cttimeval / (d1+np/sw+0.1);
     if ((int)(ss)>0) cttime=0.0;	/*no ct display with steady state*/
     if (cttime==0.0) f=0;
     else if (cttime>32767.0)
          {  f=32767;
             cttime=32767.0;
          }
     else { f=(int)(cttime);
                if (f<1) f=1;
          }
     putcode(f);			/*ctsize    loc 8*/
   
     putcode(mlb);			/*dsize     loc 9*/
     putcode(3);			/*asize     loc 10*/
     putcode(0);			/*acodeb    loc 11*/
     putcode(0);			/*acodep    loc 12*/
     putcode(85);			/*status    loc x*/
     putcode(4);			/*dpf       loc x*/
     putcode(maxscale);			/*maxscale  loc x*/
     putcode(0);			/*icmode    loc x*/
     putcode(0);			/*stmchk    loc x*/
     putcode(0);			/*nflag     loc x*/
     putcode(0);			/*scale     loc x*/
     putcode(check);			/*check     loc x*/
     oph_ap = apc.preg;
     putcode(0);			/*oph       loc xx*/
     putcode((int)(bs));		/*bs        loc x*/
     bsct_ap = apc.preg;
     putcode((int)(bs));		/*bsct      loc x*/
     if (ss < 0.0)
        putcode((int)(-ss));		/*ss        loc x*/
     else
     {  if (ix == 1)
           putcode((int)(ss));
        else
           putcode(0);
     }
     putcode(0);			/*ssct      loc x*/
     putcode(0);			/*oflag     loc x*/
     initdecphasetab();			/*dcpptab   loc x*/
     initobsphasetab();			/*obsphtab  loc x"*/
     putcode(0);			/*rfphaspat loc x*/
     putcode(0);			/*curdec    loc x"*/
     putcode(cpflag);			/*cp flag, 1=cp, 0=qp*/
     putcode(mxcnst);			/*mxcnst    loc x*/
     putcode(0);			/*tablert   new */
     nsp = apc.preg;			/*noise param start ptr*/
     putcode(0); putcode(0); putcode(0);/*16 U500 parameter not used in G+ */
     putcode(0); putcode(0); putcode(0);
     putcode(0); putcode(0); putcode(0);
     putcode(0); putcode(0); putcode(0);
     putcode(0); putcode(0); putcode(0);
     putcode(0);
   /*user variables */
     putcode(0);			/*id2       loc x*/
     putcode(0);			/*zero      loc x*/
     putcode(1);			/*one       loc x*/
     putcode(2);			/*two       loc x*/
     putcode(3);			/*three     loc x*/
     inittemps();			/*temp1-14  loc x*/
     putcode(0);			/*filler/spare*/
  }

/* fill in autodata structure */
/*  autor = (autodata *) &apc.apcarray[apc.preg]; */
  autor = (autodata *)  Aauto; /*&apc.apcarray[sizeof(struct lc)/2]; */

/* 32 is hardcoded here (not MAX_SHIMS) because lc_gem.h structure only */
/* allows for 32 shims, not MAX_SHIMS=48 */
   for (index=2; index < 32; index++)
   {   if ( (sh_name = get_shimname(index)) != NULL)
       {  tmp = getval(sh_name);
          autor->coil_val[index] = (short) tmp;
       }
       else
          autor->coil_val[index] = (short) 0;
   }
   autor->coil_val[1] = z0;

   autor->lockpower = (short) lockpower;
   autor->lockgain  = (short) lockgain;
   autor->lockphase = (short) lockphase;
   strncpy(autor->com_string,method,127);
   autor->com_string[127] = '\000';
   if (suflag==2) strcpy(wshim,"fid ");
   switch (wshim[0])
   {  case 'e': 
	autor->when_mask = 1; shimatanyfid=FALSE;
	break;
      case 'f':
	autor->when_mask = 2; 
	if ( (count=atoi(&wshim[1])) )	/* single = sign here */
	   shimatanyfid = ( (ix-1) % count)==0;
	else
	   shimatanyfid=TRUE;
	break;
      case 'b':
	autor->when_mask = 4;
	shimatanyfid=TRUE;
	break;
      case 's':
	autor->when_mask = 8;
	shimatanyfid=FALSE;
	break;
      case 'g':
      case 'n':
	autor->when_mask = 0;
        shimatanyfid=FALSE;
	break;
      default:
	printf("psg: wshim parameter illegal\n");
	autor->when_mask = 0;
        shimatanyfid=FALSE;
	break;
   }
   getparm("load",  "string",CURRENT,load,    MAXSTR);
   if (load[0]=='y') autor->when_mask |= 256;
   if ( (suflag==3) || (suflag==6) ) lockmode=2;       /*allways,phase,pwr,gn*/
   else
   {  switch (alock[0])
      {  case 'y': lockmode = 1;       /*autolock hardware*/
		   break;
	 case 'a': lockmode = 3;       /*allways,pwr,gain*/
		   break;
	 case 's': lockmode = 4;       /*s.change,pwr,gain*/
		   break;
	 case 'n': lockmode = 0;
		   break;
	 case 'u': lockmode = 5;
		   break;
	 default:  lockmode = 0;
		   printf("psg: alock parameter illegal\n");
		   break;
      }
   }
   if ( ! newacq)
      autor->control_mask = 6;		/*debug only, may be solvent dependent*/
   autor->recgain     = (short) gain;
   autor->sample_mask = (short) loc;
/*code offset */
  if ( ! newacq )
     apc.preg += sizeof(autodata)/2;
  codeoffset = apc.preg; 
  apc.apcarray[ACODEB] = codeoffset;	/* start of acode execution */
  apc.apcarray[ACODEP] = codeoffset;	/* start of acode execution */
}


/*-------------------------------*/
/*       initialize              */
/*-------------------------------*/
void initialize()                                             
{
   seqpos = 1;
   mxcnst = (int)(mxcnstr);  /* adjustment to maxscale */
/*init loop count addresses*/
   initparms();
   initquistates();
}

/*-------------------------------*/
/*       initauto1               */
/*-------------------------------*/
void initauto1()
{
  if ( (autor->sample_mask!=0) && ((suflag==0) || (suflag>=5)) ) 
  {
    if (newacq)  ifzero(initflagrt); 	/* At start of experiment */
     putcode(GETSAMP);			/* remove old sample       */
    if (newacq)  endif(initflagrt);
  }
  if ( newacq )
  {
     {  putcode(GAIN);
	putcode((int)rcvr_cntl);
     }
  }
  else
  {  putcode(GAIN);			/* gain opcode             */
     putcode((int)rcvr_cntl);		/* specify receiver state  */
  }
  if ( ! newacq )
     putcode(LOADSHIM);			/* set shim dacs           */
}

/*-------------------------------*/
/*       initauto2               */
/*-------------------------------*/
void initauto2()
{
double	getval();
  if ( (autor->sample_mask!=0) && ((suflag==0) || (suflag>=5)) ) 
  {
    if (newacq)
    {
       ifzero(initflagrt); 	/* At start of experiment */
       putcode(LOADSAMP);			/*insert new sample       */
       putcode(spin >= 0.0);
       endif(initflagrt);
    }
    else
    {
       putcode(LOADSAMP);			/*insert new sample       */
    }
  }

  if ( ((suflag==0) || (suflag==4) || (suflag==6)) && (spin>=0.0) ) 
  {  if ( spin != oldspin )
     {  putcode(SPINA);			/*spin opcode             */
        putcode((int)spin);		/*   arg1: spinner rate   */
        if ( ! newacq)
        {  if ( (interLock[1] != 'N') && (interLock[1] != 'n'))
              putcode( ( (interLock[1] == 'y') || (interLock[1] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
           else
              putcode(0);
        }
        else
           putcode( getval("spinThresh") + 0.05);
	if (ok2bumpflag)
	  putcode( 1 );
	else
	  putcode( 0 );
        oldspin = spin;			/*remember for future     */
     }
  }
  if (suflag==0) 
     newpreacqdelay();			/* preacqdelay (PAD) */

  if ( (lockmode!=0) && ((suflag==0) || (suflag==3) || (suflag==6)) )
  {  if ( (ix==1) || (lockmode >= 8) )
     {
      putcode(LOCKA);			/* autolock opcode         */
      putcode(lockmode&7);		/*    arg1: autolock mode  */
      putcode(0);			/*    arg2: position       */
      putcode(18);			/*    arg3: power          */
     }
  }

  if ( ! newacq )
  {  if ( (shimatanyfid || (ix==1))  &&
          ((suflag==0) || (suflag==2) || (suflag==6)) )
          putcode(SHIMA);	/* autoshim acode          */
  }
  else
     initwshim();		/* autoshim acode + method, etc */

  if (again && (suflag==0) && !ra_flag)
  {
     putcode(AUTOGAIN);	/* set autogain flag*/
     again = 0;         /* only do autogain once */
  }
}

/***************************************************************/
/*      get parameter from 'go' into psg program               */
/***************************************************************/
/*-------------------------------*/
/*       convertparams           */
/*-------------------------------*/
void convertparams(fidn)                                          
int	fidn;
{
char	tval[MAXSTR];
int	blocks;
int	dspflag = FALSE;
int	factor=1;
int	newoversamp;
double	tmpval;
vInfo	info;
   std_lc_init();

   getparm("at",    "real",  CURRENT,&at,     1);
   getparm("sw",    "real",  CURRENT,&sw,     1);
   getparm("nt",    "real",  CURRENT,&nt,     1);
   getparm("np",    "real",  CURRENT,&np,     1);
   getparm("d1",    "real",  CURRENT,&d1,     1);
   getparm("pw",    "real",  CURRENT,&pw,     1);
   getparm("dm",    "string",CURRENT,dm,      MAXSTR); mf_dm = strlen(dm);
   getparm("dmm",   "string",CURRENT,dmm,     MAXSTR); mf_dmm= strlen(dmm);
   getparm("cp",    "string",CURRENT,tval,    MAXSTR); cpflag= (tval[0] != 'y');
   getparm("spin",  "real",  CURRENT,&spin,   1);      oldspin = -1.0;
   if (!var_active("spin", CURRENT) || !automated ) spin=-1.0;
   getparm("hs",    "string",CURRENT,hs,      MAXSTR); mf_hs = strlen(hs);
   getparm("hst",   "real",  CURRENT,&hst,    1);
   getparm("fb",    "real",  CURRENT,&fb,     1);
   if (!var_active("ss",CURRENT))
      ss = 0.0;
   else
   {  getparm("ss",    "real",  CURRENT,&ss,     1);
   }
   getparm("bs",    "real",  CURRENT,&bs,     1);
   if (!var_active("bs", CURRENT) ) bs = 0.0;
   getparm("tof",   "real",  CURRENT,&tof,    1);	tof_init = tof;
   getparm("dof",   "real",  CURRENT,&dof,    1);	dof_init = dof;
   getparm("dmf",   "real",  CURRENT,&dmf,    1);
   getparm("dpwr",  "real",  CURRENT,&dpwr,    1);
   dhp=dpwr;
   getparm("dlp",   "real",  CURRENT,&dlp,    1);
   getparm("alfa",  "real",  CURRENT,&alfa,  1);
   getparm("rof1",  "real",  CURRENT,&rof1,   1);
   getparm("rof2",  "real",  CURRENT,&rof2,   1);
   if ( P_getreal(CURRENT,"mxconst",&mxcnstr,1) < 0 )
   {
      mxcnstr = 0.0;                /* if not found assume 0 */
   }
   getparm("pad",   "real",  CURRENT,&pad,   1);	padflag = TRUE;
   getparm("p1",    "real",  CURRENT,&p1,     1);
   getparm("d2",    "real",  CURRENT,&d2,     1);
   if (P_getreal(CURRENT,"d3",&d3,1) < 0)
   {  d3 = 0.0;
   }
   if (P_getreal(CURRENT,"pwx",&pwx,1) < 0)
   {  pwx = 0.0;
   }
   if (P_getreal(CURRENT,"pwxlvl",&pwxlvl,1) < 0)
   {  pwxlvl = 0.0;
   }
   getparm("temp",  "real",  CURRENT,&vttemp, 1);	oldvttemp = 29999.0;
   if (!var_active("temp", CURRENT) ) tpf = FALSE; else tpf = TRUE;
   getparm("vtc",   "real",  CURRENT,&vtc,    1);
   getparm("sfrq",  "real",  CURRENT,&sfrq,   1);
   getparm("dfrq",  "real",  CURRENT,&dfrq,   1);
   getparm("wshim", "string",CURRENT,wshim,   MAXSTR);
   getparm("load",  "string",CURRENT,load,    MAXSTR);
   getparm("il",    "string",CURRENT,il,      MAXSTR);
   getparm("interLocks","string",CURRENT,interLock,4);
   getparm("dod",   "real",  CURRENT,&dod,    1);
   getparm("vtwait","real",  CURRENT,&vtwait, 1);
   getparm("z0",    "real",  GLOBAL, &z0,     1);
   getparm("lockgain", "real",GLOBAL,&lockgain,   1);
   if (lockgain>39.0)
      lockgain=39.0;
   getparm("lockphase","real",GLOBAL,&lockphase,  1);
   getparm("lockpower","real",GLOBAL,&lockpower,  1);
   if (lockpower>48.0)
      lockpower=48.0;
   getparm("gain",  "real",  CURRENT,&gain,   1);
   again = !var_active("gain", CURRENT);
   getparm("traymax","real",  GLOBAL,&tmpval,    1);
   traymax = (int) (tmpval + 0.5);
   getparm("loc",   "real",  GLOBAL,&tmpval,    1);
   loc = (int) (tmpval + 0.5);
   if (!var_active("loc", GLOBAL)) loc=0;

   if (P_getreal(CURRENT,"ldshimdelay",&ldshimdelay,1) < 0)
   {  ldshimdelay = 3.0;
   }

   /* if using Gilson Liquid Handler or Hermes then gilpar is defined */
   /* and is an array of 4 values */
   if ((traymax == 96)	|| (traymax == (8*96))) /* test for Gilson/Hermes */
   {
      int trayloc,trayzone;

      if ( P_getreal(GLOBAL, "vrack", &tmpval, 1) >= 0 )
         trayloc = (int) (tmpval + 0.5);
      else
      {
         abort_message("can't find vrack\n");
      }

      if ( P_getreal(GLOBAL, "vzone", &tmpval, 1) >= 0 )
         trayzone = (int) (tmpval + 0.5);
      else
      {
         abort_message("can't find vzone\n");
      }

      /* rrzzllll */
      loc = loc + (10000 * trayzone) + (1000000 * trayloc);
      if (bgflag)
         fprintf(stderr,"VAST: -- vrack: %d, vzone: %d, Encoded Loc = %d\n",
                trayloc,trayzone,loc);

   }

   getparm("com$string","string",CURRENT,method,  130);
   getparm("alock", "string",CURRENT,alock,    MAXSTR);
   getparm("tpwr",  "real",  CURRENT,&tpwr,   1);
   getparm("lockfreq", "real",GLOBAL,&lockfreq,  1);
   tau = getvalnwarn("tau");
   cardb = ( (int) (2.0*sfrq) < xltype );
   indirect = !cardb && ( (int) (2.0*dfrq) < xltype );

   if ( P_getreal(CURRENT,"tpwrm",&tpwrf,1) < 0 )
       if ( P_getreal(CURRENT,"tpwrf",&tpwrf,1) < 0 )
          tpwrf = 255.0;
   cur_tpwrf = tpwrf;
   if ( P_getreal(CURRENT,"dpwrm",&dpwrf,1) < 0 )
       if ( P_getreal(CURRENT,"dpwrf",&dpwrf,1) < 0 )
          dpwrf = 255.0;
   cur_dpwrf = dpwrf;
/* check for WFG parameter if they exist */
   if (P_getstring(CURRENT, "dseq", dseq, 1, MAXSTR) < 0)
      dseq[0] = '\000';
   if (P_getreal(CURRENT, "dres", &dres, 1) < 0)
      dres = 90.0;

/* Finally check if `acqproc' dsp is requested and adjust sw, np, fb */
   if (!P_getVarInfo(CURRENT, "oversamp", &info))
   {  if (info.active)
         if (!P_getreal(CURRENT, "oversamp", &tmpval, 1))
            if (tmpval > 1)
            {  factor = (int)(tmpval+0.5);
               if (factor > 1)
                  dspflag = TRUE;
            }
   }

   if (acqiflag || dps_flag) dspflag = FALSE;

   dsp_params.flags = 0;
   dsp_params.rt_oversamp = 1;
   dsp_params.rt_extrapts = 0;
   dsp_params.il_oversamp = 1;
   dsp_params.il_extrapts = 0;
   dsp_params.rt_downsamp = 1;
   tot_np=np;
 
   if (dspflag)
   {  if ( !P_getreal(CURRENT, "oscoef", &tmpval, 1) )
      {
         int oscoef;
         double dspfb;
         double dsplsfreq;
         char   dspfilt[256];
 
         oscoef = (int) (tmpval + 0.01);
         dspfb = 0.0;
         dsplsfreq = 0.0;

      /* verify that 3 < oscoef < 50000 */
         if ((int)tmpval/2 > OS_MAX_NTAPS)
         {
            abort_message("oscoef too large");
         }
 
         if (tmpval < OS_MIN_NTAPS)
         {
            abort_message("oscoef too small");
         }

          dsp_params.il_oversamp = factor;
          dsp_params.il_extrapts = ((int)(tmpval/2)) * 2;
          tot_np = np*dsp_params.il_oversamp+(float)(dsp_params.il_extrapts);
          fb = fb * dsp_params.il_oversamp;
      /* verify that sw isn't too big, 100K */
          P_getVarInfo(CURRENT, "sw", &info); /* got sw before, assume its ok */
          if (info.prot & P_MMS)
             P_getreal( GLOBAL, "parmax", &tmpval, (int)(info.minVal+0.1) );
          else
             tmpval = info.maxVal;
          if (sw*dsp_params.il_oversamp > tmpval)
          {  abort_message("oversamp * sw > %f Hz\n",tmpval);
          }
      /* verify that np isn't too big, 128000 */
          P_getVarInfo(CURRENT, "np", &info); /* got np before, assume its ok */          if (info.prot & P_MMS)
             P_getreal( GLOBAL, "parmax", &tmpval, (int)(info.minVal+0.1) );
          else
             tmpval = info.maxVal;
          if (tot_np > tmpval)
          {  newoversamp = 128000 / ((int)np);
	     text_message("oversamp * np > %d, reducing oversamp to %d\n",(int)tmpval,newoversamp);
             fb = fb / dsp_params.il_oversamp;
             dsp_params.il_oversamp = newoversamp;
             tot_np = np*dsp_params.il_oversamp+(float)(dsp_params.il_extrapts);
             fb = fb * dsp_params.il_oversamp;
	     putCmd("setvalue('oversamp',%d) setvalue('oversamp',%d,'processed')\n",newoversamp,newoversamp);
	     if (newoversamp < 2) psg_abort(1);
          }  
      /* verify that il<>'y', can't do DSP and il */
          if (il[0] == 'y')
          {
             abort_message("Can't do DSP with il='y'");
          }
          if ((P_getreal(CURRENT, "osfb", &tmpval, 1) >= 0) &&
              var_active("osfb",CURRENT))
          {
             if (sw*(double)dsp_params.il_oversamp*
			(double)dsp_params.il_oversamp/(tmpval*2.0) > 1.0)
                dspfb = tmpval;
          }
          if ((P_getreal(CURRENT, "oslsfrq", &tmpval, 1) >= 0 ) &&
              var_active("oslsfrq",CURRENT))
	     dsplsfreq = tmpval;

          if (P_getstring(CURRENT, "filtfile", dspfilt, 1, 256) < 0)
             dspfilt[0] = '\0';
          set_dsp_pars(sw, dspfb, dsplsfreq, dsp_params.il_oversamp, oscoef,dspfilt);
      }
   }

   if (newacq)
   {  int	curct, bsct4ra;
      double	relaxdelay;

      blocks = (tot_np*4 + 1023L)/1024L;	/* block=1k, 4 bytes/word */
      /* --- completed transients (ct) --- */
      if ((P_getreal(CURRENT,"ct",&tmpval,1)) >= 0)
      {
         curct = (int) (tmpval + 0.0005);
      }
      else
      {   abort_message("convertparms(): cannot find ct.");
      }
      bsct4ra = (bs + 0.005);
      if (bsct4ra > 0)
         bsct4ra = curct/bsct4ra;

      /* for ra, check nt and ct and interleaving */
      /* for ra, curct handled in  ra_initacqparms or ra_inovaacqparms. */

      if (curct == nt)
      {
         if (getIlFlag())
         {
            if (getStartFidNum() > 1) bsct4ra = bsct4ra - 1;
         }
         else
         {
            bsct4ra = 0;
         }
      }
      else
      {
         if (getIlFlag())
         {
            if (getStartFidNum() > 1) bsct4ra = bsct4ra - 1;
         }
      }
      if (bsct4ra < 0) bsct4ra = 0;

/* Get relaxation delay for acqi and interelement delay */
      if (option_check("qtune"))
      {
         relaxdelay = -1.0;
      }
      else if ( P_getreal(CURRENT,"relaxdelay",&relaxdelay,1) < 0 )
      {
         if (acqiflag)
            relaxdelay  = 0.020;	/* if not found assume 20 millisecs */
         else
            relaxdelay  = 0.0;		/* if not found assume 0 */
      }
      else if (relaxdelay > 2.0)
      {
         text_error("relaxdelay truncated to maximum relaxdelay: 2 sec");
         relaxdelay = 2.0;
      }


      custom_lc_init(
        /*  1. Alc->idver    */ (codeint) (0),
        /*  2. Alc->elemid   */ (codeulong) fidn,
        /*  3. Alc->ctctr    */ (codeint) cttime,
        /*  4. Alc->dsize    */ (codeint) blocks,
        /*  5. Alc->np       */ (codelong) tot_np,
        /*  6. Alc->nt       */ (codelong) (nt + 0.0001),
        /*  7. Alc->dpf      */ (codeint) 4,	/* dpflag, always 4 */
        /*  8. Alc->bs       */ (codeint) (bs + 0.005),
        /*  9. Alc->bsct     */ (codeint) bsct4ra,
        /* 10. Alc->ss       */ (codeint) ss,
        /* 11. Alc->asize    */ (codeint) 0xC001,	/* no hw DSP */
        /* 12. Alc->cpf      */ (codeint) cpflag,
        /* 13. Alc->maxconst */ (codeint) maxsum,
        /* 14. arraydim      */ (codeulong) (ExpInfo.ArrayDim),
        /* 15. relaxdelay    */ (codeulong) (relaxdelay*1e7),
        /* 16. Alc->acqct    */ (codelong) curct,
	/* 17. Alc->clrbsflag*/ (codeint) clr_at_blksize_mode
           );
   }
/* switch between BB refgen and 4_nuc refgen */
/*   getparm("bb_ref", "string",CURRENT,tval,    MAXSTR); */
   bb_refgen = (rftype[0] == 'f');

/* get sim optional parameter or presaturation */
    P_getreal(CURRENT,"satdly",&satdly,1);
    P_getreal(CURRENT,"satfrq",&satfrq,1);
    P_getreal(CURRENT,"satpwr",&satpwr,1);
    P_getstring(CURRENT, "satmode", satmode, 1, MAXSTR);

/*calculate memory length of data*/
   mlr = np;
   mlr = mlr*2.0;
   mlb = (int)((mlr+255.5)/256.0);
}

/*-------------------------------*/
/*       readparams              */
/*-------------------------------*/
void readparams()                                             
{
char	tval[MAXSTR];
double	treal;
double  phase;
   broadband = TRUE;
   getparm("fattn",  "real",  GLOBAL, &treal,     1);     fattn[0] = (int)treal;
   getparm("fattn",  "real",  GLOBAL, &treal,     2);     fattn[1] = (int)treal;
/*   fattn[0] = fattn[1] = 0; */
   getparm("h1freq", "real",  GLOBAL, &treal,     1);     xltype  = (int) treal;
   getparm("h1freq", "real",  GLOBAL, &treal,     1);     xltype  = (int) treal;
   getparm("vttype", "real",  GLOBAL, &treal,     1);     vttype  = (int) treal;
   getparm("amptype","string",GLOBAL, amptype,MAXSTR);
   getparm("rftype", "string",GLOBAL, rftype, MAXSTR);
   getparm("homdec", "string",GLOBAL, tval,  MAXSTR);  homdec = (tval[0]=='y');
   getparm("spinopt","string",GLOBAL, tval,MAXSTR); automated = (tval[0]=='y');
   getparm("parmax", "real",  GLOBAL, &dhpmax,    9);
   getparm("shimset","real",  GLOBAL, &treal, 1);    shimset = (int)treal;
   if (P_getreal(CURRENT,"pplvl",     &pplvl,     1) < 0) pplvl = 0.0;
   if (P_getreal(CURRENT,"cttime",    &cttimeval, 1) < 0) cttimeval = 5.0;
   if (P_getreal(CURRENT,"gmax",      &gmax,      1) < 0) gmax = 0.0;
   if (P_getreal(CURRENT,"phase",     &phase,     1) < 0) phase1 = (int) phase;
   if (P_getreal(GLOBAL, "gradstepsz",&gradstepsz,1) < 0) gradstepsz = 0.0;
}

int find_dqdfrq(dqdfrq,offset,address)
double *dqdfrq, offset;
int address;
{
   int devicetest = 0;
   *dqdfrq = 0.0;
   if ((cardb && (address==LB_XMT_SEL)) || (!cardb && (address==HB_XMT_SEL)))
      devicetest = 1;
   if ((devicetest==1) && (go_dqd==1))
   {
      *dqdfrq = dqd_offset;
   }
   return(0);
}

void init_dqd() /* see offset() */
{
double offset;
int     cnt_place, cnt_strt;
int address;
char dqd[MAXPATHL], dspstr[MAXPATHL];
   if (cardb)
   {
      address = LB_XMT_SEL;     /* low band */
      offset  = lb_offset;
   }
   else
   {
      address = HB_XMT_SEL;     /* hi band */
      offset  = hb_offset;
   }
   if (go_dqd == 0)
   {
      dqd_offset = 0.0;
      if (P_getstring(GLOBAL, "dsp", dspstr, 1, MAXPATHL) < 0)
         strcpy(dspstr, "n");
      if (P_getstring(GLOBAL, "fsq", dqd, 1, MAXPATHL) < 0)
         strcpy(dqd, "n");
      if ((go_dqd==0) && ((dspstr[0]=='i') || (dspstr[0]=='r')) && (dqd[0]=='y'))
      {
         dqd_offset = ExpInfo.DspOslsfrq;
         if (offset < 10.65e6)
         {
            if (dqd_offset < 0) dqd_offset = -(dqd_offset);
         }
         else
         {
            if (dqd_offset > 0) dqd_offset = -(dqd_offset);
         }
         ExpInfo.DspOslsfrq = -(dqd_offset);
         go_dqd = 1;
      }
   }
   if (fabs(dqd_offset) > 0.01)
   {
      putcode(APBOUT);
      cnt_place = apc.preg;
      putcode(0);          /* place for apbus count */
      cnt_strt = apc.preg;
      setoffset(offset,address);                /* set frequency */
      apc.apcarray[cnt_place] = apc.preg - cnt_strt - 1;
   }
}

/***************************************************************/
/*                     psgmid                                  */
/***************************************************************/
/*-------------------------------*/
/*       createsequence          */
/*-------------------------------*/
void createsequence()
{
/*initialization code*/
   initialize();		/*init parameters used in this program   */
   if (ra_flag)
   {  if (newacq)
         ra_mercacqparms(ix);
      else
         ra_initacqparms(ix);	/*recover console values in lc, eg. rtp  */
   }

   Codeptr= Aacode;
   if (newacq) set_counters();
   if (newacq && (ix==1))
   {  initobsphasetab();
      initdecphasetab();
   }
   if (newacq)
   {   new_lcinit_arrayvars();
       if (ix == getStartFidNum())
       {  ifzero(initflagrt);
          set_nfidbuf();	/* set stm size fro init_stm */
          putcode(CBEGIN);	/* NOOP for old, ID check for newacq	 */
          if (!acqiflag)
             send_auto_pars();	/* hmm... */
          if (getIlFlag())
          {  elsenz(initflagrt);
             assign(ilssval,ssval);
             assign(ilctss,ctss);
          }
          endif(initflagrt);	/* Interleaving */
       }
       if (getIlFlag())
       {
          ifzero(initflagrt);  /* Interleaving: skip after 1st pass */
          elsenz(initflagrt);
             mult(bsctr,bsval,ct);
          endif(initflagrt);   /* Interleaving */
       }
   }
   else
   { putcode(CBEGIN);		/* NOOP for old, ID check for newacq     */
     putcode(INIT);		/*setup output and input cards		 */
   }
   if (anywg)
      wg_reset();

   if ((ix == getStartFidNum()) && (anywg)) /*load rfpattern, 1st FID only*/
   {
      ifzero(initflagrt);	/* Interleaving: skip after 1st pass */
      putcode(WGGLOAD);		/* load patterns  and reset */
      endif(initflagrt);	/* Interleaving */
   }
   if (ix == 1)                 /*only do this on the first increment    */
   {                            /*since the reset pulse to the amp       */
                                /*has a pulse width of 50 msec.          */
      all_grad_reset();         /*resets, zeros, and enables/disables    */
   }
   if ((suflag==0) && (tolower(gradtype[2])=='p'))
      ecc_handle();		/* reset ecc values                      */
   if (suflag == 0)
      putcode(CLEAR);		/*clear data table if real acquisition   */
   initapbvalues();		/*init 'some' hardware to default values */
   if (newacq)
   {  initHSlines();
   }
   pre_fidsequence();		/* user pre-fid functions */
   if (newacq)
   {  loadshims();
   }
   initauto1();			/* init., what has to before VT change   */
   if ( vttemp != oldvttemp )
     setvt();			/*set Varian                             */
   set_grad_shim_relay();	
   delay(dod);			/*delay to let things settle             */
   if (! newacq)
      putcode(HALT);
   putcode(STFIFO);		/*start fifo, if not running already     */
   putcode(SFIFO);		/*stop fifo                              */
   if ( vttemp != oldvttemp )
        waitforvt();		/*wait for vt, if first fid or vt array  */
   initauto2();			/*init what has to after VT change       */
   oldvttemp = vttemp;		/*remember for future, but after initauto2 */

   if (newacq)			/*insert at similar location to inova	*/
      scanstart();		/*check if spinning, temperature, lock   */

/* if only setup (suflag <>0) then this code */
   if (suflag != 0)
   {  putcode(SETUP);		/*setup acode, to stop acq               */
      putcode(suflag*8-1);	/*setup done code, returned from acq     */
      return;
   }

/* code for actual acquisition of data */
   if (ix  == getStartFidNum())
   {  if (newacq)
      {  ifzero(initflagrt);	/* Interleaving: skip after 1st pass */
         assign(one,initflagrt);/* LAST Item in INITIALIZATION */
      }
      donoisecalc();		/* acquire noise data & calc noise */
      if (newacq)
         endif(initflagrt);	/* Interleaving */
   }

   if ( newacq )
      putcode(FIDCODE);		/* patch autoshimcode with this offset */
   putcode(INIT);
   putcode(CLEAR);
   nsc_ptr = Codeptr-Aacode;		/*save scanstart address */
   putcode(NSC);		/*nsc set ovfl,scale,mode,prec,check,oph */
   init_dqd();
   if (anywg)
      wg_reset();
   zero_dds_acc();		/* resync LO and xmtr */
   if (!newacq)
      scanstart();		/*check if spinning, temperature, lock   */
   inittablevar();		/*initialize all table vars */
   acqtriggers = 0;		/* reset after donoisecalc() */
   if (!tuneflag)
      obs_pw_ovr(TRUE);
   pulsesequence();		/*do the actual pulsesequence            */
   initializeSeq = 0;
   if (!tuneflag)
      obs_pw_ovr(FALSE);
   test4acquire();			/*gather data*/
   if (newacq)
      putcode(EXIT);		/* done with this acode set */
}

/*-------------------------------*/
/*       meat                    */
/*-------------------------------*/
void meat()
{
   if (newacq)
      apc.apcarray    = (short *) Aacode;
   else
      apc.apcarray    = (short *) Codes;
   acqtriggers     = 0;
   apc.preg        = 0;
   blank_apb       = 0;
   blankingon_flag = TRUE;
   check           = 64;
   declvlonoff     = FALSE;
   hwlooping       = FALSE;
   lastcw          = -1;
   lastdmm         = -1;
   maxscale        = 10;
   noiseflag       = FALSE;
   ok              = TRUE;
   suflag          = setupflag;
   if (ok)
       createsequence();
   if (PSfile)
      write_Acodes(ix);
}

#define COMPSIZE  254   /* byte maximum */
/*----------------------------------------------*/
/*   acode compression code			*/
/*   comparing a source and reference		*/
/*   by XOR and run length encoding		*/
/*   the result returns the compressed size	*/
/*----------------------------------------------*/
typedef unsigned char BASICSIZE;
 
static int
compress(dest,ref,src,num)
register BASICSIZE *dest, *ref, *src;
register int num;
{
  register int i,sneak;
  register BASICSIZE *d,k,zcnt;
  d = dest;
  zcnt = 0;
  sneak = 0;
  for (i=0; i < num; i++)
  {
    /* printf("%3x   %3x  %5d\n",*ref,*src,i); */
    /* XOR is faster size invariant and order independent */
    k = *ref++ ^ *src++;
    if (k == 0)   /* compress it */
    {
      zcnt++;
      if (zcnt > COMPSIZE)
      {
        *d++ = 0;
        *d++ = zcnt;
        sneak+=2;
        zcnt = 0;
      }
    }
    else
    {
      if (zcnt > 0)
      {
        *d++ = 0;
        *d++ = zcnt;
        sneak += 2;
        zcnt = 0;
      }  
      *d++ = k;
      sneak++;
    }
  }
  if (zcnt > 0)
  {
    *d++ = 0;
    *d++ = zcnt;
    sneak += 2;
  }
  return(sneak);
}  

/***********************************************
        compressed acode handlers
***********************************************/
 
/* 32 entries */
 
static struct
{
    int size;
    unsigned char *ucptr;
}
ref_table[32];
 
static unsigned char *tmp_array=NULL;
static int size_tmp_buff=0;
 
#define NO_CODE    0
#define REF_CODE  64
#define CMP_CODE 128
 
extern codelong *CodeIndex;
 
static int
do_ref_tab(num)
int num;
{      
  int i;
  if (tmp_array == NULL)
  {           
    for (i = 0; i < 32; i++)
    {         
      ref_table[i].size = -1;
      ref_table[i].ucptr = NULL;
    }
    tmp_array = (unsigned char *) malloc(num+1000);
    size_tmp_buff = num+1000;
    if (tmp_array == NULL)
    {
      fprintf(stderr,"Malloc Failed: could not make tmp_array\n");
      return(-1);
    }       
  }         
  return(1);
}    

static int
find_reference(num)
{  
   int i;
   i = 0;
   while ((i < 32) && (ref_table[i].size != num))
     i++;
   return(i);
}  
 
 
static int
add_reference(num_elements,ref_ptr)
register int num_elements;
register unsigned char *ref_ptr;
{
    register unsigned char *pntr1,*pntr,*tt;
    register int index,j;
/*
    Find empty buffer #
*/
    index = 0;
    while ((index < 32) && (ref_table[index].ucptr != NULL))
      index++;
/*
    check for errors - caller uses NO_CODE
*/
    if (index == 32)
      return(-1);  /* out of slots !! */
    pntr1 = (unsigned char *) malloc(num_elements + 200);
    if (pntr1 == NULL)
      return(-2);
    if (size_tmp_buff < num_elements)
    {
      tt = (unsigned char *) realloc(tmp_array,num_elements+200);
      if (tt == NULL)
      {
        fprintf(stderr,"Realloc Failed: could not make tmp_array\n");
        return(-4);
      }
      size_tmp_buff = num_elements + 200;
    }  
/*****************************************
    got space now copy and init
*****************************************/
    ref_table[index].ucptr = pntr1;
    ref_table[index].size = num_elements;
    pntr = ref_ptr;
    pntr1 += 2 * sizeof(codelong);
    for (j=0; j < num_elements; j++)
    {
      *pntr1++ = *pntr++;
    }
    /* initialize rest of buffer!!! */
    for (j=0; j < 180; j++)
       *pntr1++ = 0;
    return(index);
}
 
void put_code_offset(ptr,size,refnum,checknum)
codelong *ptr;
int size, refnum,checknum;
{
/*
   fprintf(stderr,"pco %d in %d in %d\n",size,refnum,checknum);
*/
   *ptr = (codelong) size;
   /* size in upper word a checksum */
   *(ptr+1) = (refnum & 0x0ff) | (checknum << 8);
}
 
/*-----------------------------------------------------------------
|       write_Acodes()/1  action
|       writes lc,auto & acodes out to disk file
+------------------------------------------------------------------*/
extern short *preCodes;
static int max_acode = 0;
static int do_compress = 0;
 
void init_compress(num)
{
    char compress[MAXSTR];
/****************************************************************
      Decide about compression
****************************************************************/
    if (P_getstring(GLOBAL, "compress", compress, 1, MAXSTR) < 0)
         strcpy(compress,"Y");
/****************************************************************/
    if (compress[0] != 'n')
       do_compress = 1;
    if (num <= 1)
       do_compress = 0;
}
 
void write_Acodes(int act)
{
    int bytes,i,ccnt,bufn,check;
    int sizetowrite,kind;
    int newcodesize;
    unsigned char *pstar;
    codeint *pntr;
 
#ifndef LINUX
    if (!do_compress)
#endif
       act = 0;
    sizetowrite = 2 * apc.preg;
    check = sizetowrite;
    pstar = (unsigned char *) preCodes;
    bufn = 0;  /* normal block */
    kind = NO_CODE;

    if (newacq)
    {
       pstar = (unsigned char *)convert_Acodes(Aacode,Codeptr,&newcodesize);
       check = sizetowrite = newcodesize;
        if (bgflag)
        {
          fprintf(stderr,"convert_Acodes(): code start 0x%p, end 0x%p, size 0x %x\n",
                  Aacode, Codeptr, newcodesize);
        }
    }

    if (sizetowrite > max_acode)
    {
       if (max_acode == 0)
          set_acode_size(sizetowrite);
       set_max_acode_size(sizetowrite);
       max_acode = sizetowrite;
    }
#ifdef LINUX
    pntr = (codeint *) pstar;
    pntr += 4;
    for (i=0; i< sizetowrite/2; i++)
    {
       *pntr = htons( *pntr );
       pntr++;
    }
    pntr = (codeint *) pstar;
#endif
    if (act == 1)
      do_ref_tab(sizetowrite);
    else
      if (size_tmp_buff == 0) /* if malloc failed turn off compression */
        act = 1;
    switch (act)
    {
      case 0: case 1: break;
      default:
      if ((bufn = find_reference(sizetowrite)) == 32)
      {  
        bufn = add_reference(sizetowrite, pstar+2*sizeof(codelong));
/*
        fprintf(stderr,"Making a reference block #%d of size %d\n",
                bufn,sizetowrite);
*/
        kind = REF_CODE;
        if (bufn < 0)
        {
          bufn = 0;
          kind = NO_CODE;
        }
      }
      else
      {
        kind=CMP_CODE;
        ccnt = compress (tmp_array+2*sizeof(codelong),ref_table[bufn].ucptr+2*sizeof(codelong),
                         pstar+2*sizeof(codelong),sizetowrite);
/*
        fprintf(stderr,"Using reference block #%d of size %d\n",
                bufn,ref_table[bufn].size);
        fprintf(stderr,"write_acodes %d/%d\n",ccnt,sizetowrite);
*/
        sizetowrite = ccnt;
        pstar = tmp_array;
      }
    }
    put_code_offset(pstar,sizetowrite,bufn|kind,check);
    bytes = write(PSfile,pstar,sizetowrite + 2 * sizeof(codelong));
    if (bgflag)
    {
        fprintf(stderr,"write_Acodes(): Codeptr 0x%p, Codes 0x%p,\
          size = 0x%lx, %ld \n",Codeptr,Codes,(long)Codeptr - (long)Codes,
                (long)Codeptr - (long)Codes);
        fprintf(stderr,"write_Acodes(): act = %d,%d bytes written\n",act,bytes);
    }
}    

void close_codefile()
{
    if (newacq)
    {
       /*
        * We write extra stuff so that when this file is mmapped,
        * we will not read past the end of the file
        */
       write(PSfile,(char *) preCodes,max_acode + 2 * sizeof(codelong));
    }
    close(PSfile);
}

