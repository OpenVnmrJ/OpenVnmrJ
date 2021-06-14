/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------
|   Modified   Author     Purpose
|   --------   ------     -------
|    4/21/89   Greg B.    1. initparms() Added Code to initialize 
|				global interleave parameter (il)
|    6/19/89   Greg B.    1. Speed Optimizations, initialization routine
|			     are no longer called here.
|			  2. RF apout words are set, not calculated here any more.
+---------------------------------------------------------------------*/
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "group.h"
#include "symtab.h"
#include "variables.h"
#include "oopc.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "aptable.h"
#include "macros.h"
#include "apdelay.h"
#include "dsp.h"
#include "ssha.h"
#include "pvars.h"
#include "abort.h"
#include "cps.h"


#define CALLNAME 0
#define OK 0
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1
#define MAXSTR 256
#define MAXARYS 256
#define STDVAR 70
#define MAXVALS 20
#define MAXPARM 60
#define EXEC_SU 1
#define EXEC_SHIM 2

/* audio filter board time constants */
/* four  pole Butterworth            */
#define POLE4 2.0
/* six   pole Bessel                 */
#define POLE6 2.33
/* eight pole Elliptical             */
#define POLE8 1.29
/* eight pole Butterworth            */
#define POLE8BUTTER 3.8

#define DQD_IL_FACTOR	4

extern FILE *sliderfile;
extern double sign_add();
extern double whatiffreq();
extern void pulsesequence();

extern int bgflag;
extern int ap_interface;	/* ap bus interface type 1=500style, 2=amt style */ 
extern int newacq;
extern int SkipHSlineTest;
extern int presHSlines;
extern int acqiflag;
extern int checkflag;
extern int dps_flag;
extern int ok2bumpflag;
extern int lockfid_mode;
extern int initializeSeq;
extern int tuneflag;

extern char rfwg[];
extern char gradtype[];


extern char   **cnames;	/* pointer array to variable names */
extern int	newtransamp;	/* True if transmitter uses Class A amps */
extern int      nnames;		/* number of variable names */
extern int      ntotal;		/* total number of variable names */
extern int      ncvals;		/* NUMBER OF VARIABLE  values */
extern double **cvals;	/* pointer array to variable values */
extern double relaxdelay;
extern int grad_flag;	/* in gradient.c */
extern codeint scanflag,initflagrt,ilssval,ilctss; /* inova realtime vars */
extern codeint tmprt; /* inova realtime vars */


extern int traymax;

codeint lkflt_fast;	/* value to set lock loop filter to when fast */
codeint lkflt_slow;	/* value to set lock loop filter to when slow */
static ra_initcnt = 0;  /* If lc has been updated in an ra the 1st fid not
			 * acquired yet (no data) lc must be reset, after that
			 * lc need not be reset, hence this count flag */
static int PSfile;	/* file discriptor for Acode disk file */
int HSrotor;
int HS_Dtm_Adc;		/* Flag to convert.c to select INOVA 5MHz High Speed DTM/ADC */
int activeRcvrMask;	/* Which receivers to use */

static int interfiddelayflag = FALSE; /* flag for starting and stopping */
double d0; 	      /* defined array element overhead time */
double interfidovhd;  /* calculated array element overhead time */

struct _dsp_params	dsp_params;

static int locActive;

static union {
         unsigned int ival;
         codeint cval[2];
      } endianSwap;

static void write_Acodes(int act);
int putcode(codeint word);

/*-----------------------------------------------------------------------
|  createPS()
|	create the acode for the Pulse Sequence.
|				Author Greg Brissey  6/30/86
+-----------------------------------------------------------------------*/
createPS()
{
    int	i;
    double maxlbsw;
    double getval();
    extern void reset_pgdflag();

    if (ra_flag && !newacq)
    {
      int lc_updated;   /* true if ra lc data existed for fid ix & thus lc updated */
      /* add ra acqpar values to lc structure for fid ix */
      lc_updated = ra_initacqparms(ix);
      if (!lc_updated)
      {
        if ((ra_initcnt == 0) && (ix != 1))
        {
	   initacqparms(ix); /* reinit lc if not 1st fid & 1st time lc_updated false */
	   ra_initcnt++;
        }
      }
    }
    else if (ra_flag && newacq)
    {
      ra_inovaacqparms(ix);
    }
    set_counters();

    if (P_getreal(GLOBAL, "maxsw_loband", &maxlbsw, 1) < 0){
	maxlbsw = 100000.0;
    }

/*    clearHSlines();	*/	/* set line to safe zero states */


    putcode(ACQBITMASK);	/* set up global acquisition bit mask */
    putcode(HSrotor);

    if (newacq)
    {
	new_lcinit_arrayvars();

        if ((ix == getStartFidNum()) && (anygradwg || anygradcrwg)) {
            /*
             *We may have a monitor system (ILI, ISI, ...) and
             * we might have an MTS Grad Power Amp attached
             */
	    putcode(SAFETY_CHECK);
	    putcode(setupflag);
        }
 
 	if (ix == getStartFidNum())
         {
	   ifzero(initflagrt);	/* Interleaving: skip after 1st pass */
	    init_interfiddelay();

            if (sw > maxlbsw + 0.1)  /* Init HS DTM/ADC digital hardware */
	    {
		HS_Dtm_Adc = 1;		/* this flag check in n_begin (CBEGIN) in convert.c */
	    }				/* and further down in cps.c  for 5MHz FOO check */
            else
		HS_Dtm_Adc = 0;

	    activeRcvrMask = parmToRcvrMask("rcvrs");

	    set_nfidbuf();	/* set stm size for init_stm */
    	    putcode(CBEGIN);		/* no op */
	    if (!acqiflag)
            	send_auto_pars();
	   if (getIlFlag())
	   {
		elsenz(initflagrt);
	   	 assign(ilssval,ssval);
	   	 assign(ilctss,ctss);
	   }
           endif(initflagrt);  	/* Interleaving */
        }
	if (getIlFlag())
	{
	   ifzero(initflagrt);	/* Interleaving: skip after 1st pass */
	   elsenz(initflagrt);
		mult(bsctr,bsval,ct);
           endif(initflagrt);  	/* Interleaving */
	}
    }
    else
    {
    	putcode(CBEGIN);		/* no op */
    	putcode(INIT);		/* initialize STM,INPUT,OUTPUT broads */
    }

    /* --- for SIS --- */
    if ((cattn[TODEV] == SIS_UNITY_ATTN_MAX) && (newtrans == FALSE))
    {
    	if (ix == 1)		/* load rfpattern at first FID only */
    	{
           putcode(LDPATRAM);
    	}
    }


/* IF ANY RF WFG's */
    if ((ix == getStartFidNum()) && (anywg)) /*load rfpattern, 1st FID only*/
    {
       int addr,
	   chan;

       if (newacq) ifzero(initflagrt); /* Interleaving: skip after 1st pass */
        putcode(WGGLOAD);	/* load patterns */
        for (chan = 1; chan <= NUMch; chan++)
        { /* reset the WFG's for the RF channels */
          if ( addr = getWfgAPaddr(chan) )
             	   command_wg(addr, 0x80);
        }
       if (newacq) endif(initflagrt);  /* Interleaving */
    }

/*  IF GCU PRESENT */
    if ((setupflag) && (anygradwg))
    {
	init_ecc();
    }

     if ((anygradwg) || (anygradcrwg))
     {
 	init_crb();
     }

    if (ix == getStartFidNum()) /* only do this on the first increment */
    {                           /* since the reset pulse to the amp   */
                                /* has a pulse width of 50 msec.      */
	if (newacq) ifzero(initflagrt); /* Interleaving: skip after 1st pass */
          all_grad_reset();	/* resets, zeros, and enables/disables */
        if (newacq) endif(initflagrt);  /* Interleaving */
    }   			/* any gradient controllers (and gradients) */
    /* PFG */
    if (setupflag && anypfg)
       ecc_handle();

    if (setupflag == 0)		/* if go not su,shim,lock,etc then clear table*/
        putcode(CLEAR);		/* clear data table */

    setRF();		/* set RF to initial states */

    if (newacq)
    {
      if (ix == getStartFidNum()) 
      {  
	ifzero(initflagrt); /* Interleaving: skip after 1st pass */
	  if (anywg)		/* still necessary */
   	    reset_pgdflag();
	  initHSlines();   /* setup initial RF & pgdec high-speed line states */

	  G_Delay(DELAY_TIME, 1.0e-6, 0); /*set Acq. HSlines to initial state*/
	  /* updt_interfiddelay(1.0e-6); */
        endif(initflagrt);  /* Interleaving */
	download_master_decc_values();
      }   
      gatedecoupler(A,1.5e-5);	/* Reinit status for timing. std and intrlv */
      statusindx = A;
    }
    else
    {
	initHSlines();   /* setup initial RF & pgdec high-speed line states */
	if (anywg)		/* still necessary */
   	    reset_pgdflag();
	G_Delay(DELAY_TIME, 1.0e-6, 0); /*set Acq. HSlines to initial state*/
    }

    if (setupflag == GO)
       pre_fidsequence();		/* users pre fid functions */

    loadshims();

    /*** d0: skip after 1st pass	***/
    if (newacq)  ifzero(initflagrt); 

    if (bgflag)
        fprintf(stderr,"initauto1(): new sample: %d \n",loc);
    if ( (locActive != 0) && ((setupflag == GO) || (setupflag >= CHANGE)) )
    {
	putcode(GETSAMP);	/* remove sample Acode */
	if (newacq)
	   putcode(ok2bumpflag);
    }

    initauto1();		/* do what has to be done before VT changes */

    /*** d0: END real-time ifzero	***/
    if (newacq)  endif(initflagrt);

    /*** Interleaving: skip after 1st pass	***/
    if (newacq && getIlFlag()) ifzero(initflagrt);

    if (vttemp != oldvttemp)	/* change vt setting only if it changed */
	setvt();		/* why?, (because it takes time) */

    if (!newacq)
       G_Delay(DELAY_TIME, 2.0e-6, 0);/* time for SFIFO code before foo */
    /*  this delay PREVENTS FIFO UNDERFLOW during set up */
    putcode(STFIFO);		/* start fifo, (rf actually gets set here) */
    putcode(SFIFO);		/* Stop and wait for fifo to finish */
    curfifocount = 0;		/* reinitialize number of fifo words */

    if (vttemp != oldvttemp)	/* wait 4 vt only if it changed */
	wait4vt();		/* why?, (because it takes time) */

    /*** d0: skip after 1st pass	***/
    if (newacq)  ifzero(initflagrt); 

    if (bgflag)
       fprintf(stderr,"initauto2(): new sample: %d \n",loc);

      /*----  LOAD NEW SAMPLE ---------*/
    if ( (locActive != 0) && ((setupflag == GO) || (setupflag >= CHANGE)) )
    {
	putcode(LOADSAMP);	/* load sample Acode */
	if (newacq)
        {
           putcode(spinactive);
	   putcode(ok2bumpflag);
        }
    }

    /*** d0: END real-time ifzero	***/
    if (newacq)  endif(initflagrt);


    initauto2();		/* do what has to be done after VT changes */

    /*** Interleaving: END real-time ifzero	***/
    if (newacq && getIlFlag())  endif(initflagrt);

    if (newacq && (ix == getStartFidNum()))
	initSSHAshimmethod();
    initwshim();	/* invoke autoshimming, can be arrayed */


    /*** Interleaving: skip after 1st pass	***/
    if (newacq) ifzero(initflagrt); 

    if ((!ra_flag) || (!newacq))
	initautogain();	/* only done at beginning of experiment */    

    /* put interlock for lock and/or spin prior to NSC, only need thme once */
    if (newacq)
    {
      interlocktests();	/* test for lock,spin, & VT failures if requested */
    }

    /*** Interleaving: END real-time ifzero	***/
    if (newacq) endif(initflagrt);


    oldvttemp = vttemp;		/* set old temperature to new */

/*     pre_pulsesequence();	/* user pre - pulse sequence control func */
    setMRGains();
    setMRFilters();

    if (setupflag != 0)		/* if not a go stop here */
    {
    	if (newacq)
	{ 
	   G_Delay(DELAY_TIME, 2.0e-4, 0);	/* time before foo */
	}
	putcode(SETUP);
	putcode((setupflag * 8) - 1);
	if (setupflag != EXEC_SHIM)   /* if shim then go ahead and make the acodes */
        {
            checkpowerlevels(100.0);  /* check resting state */
    	    write_Acodes(0);	/* write out generated lc,auto & acodes */
	    return;		      /* for arnold's fid shimming  */
	}
    }

    if (!newacq)  /* For INOVA this test must be done prior to CBEGIN */
    {
      if (sw > maxlbsw + 0.1)	/* Init Wideline digital hardware */
      {	double	maxsw;
        putcode(SETWL);
        if (fb > 256001)
	    putcode(1);
        else
            putcode(0);
        P_getreal( GLOBAL, "parmax", &maxsw, 5 );
        if (maxsw > 4.9e6)
           putcode(5);		/* 5 MHz ADC */
        else
           putcode(2);		/* 2 MHz ADC */
      }
    }

    /* Do noise check on first fid only, by order of SLP */
    if (ix == getStartFidNum())
    {
	if (newacq)
        {
           ifzero(initflagrt); /* Interleaving: skip after 1st pass */
	   assign(one,initflagrt); /* LAST Item in INITIALIZATION */
        }
        donoisecalc();		/* acquire noise data & calc noise of channel */
        if (newacq)
           endif(initflagrt);  /* Interleaving */
    }
    if (newacq)
    {
       putcode(FIDCODE);   /* patch autoshim acode with this acode offset */
    }
    putcode(INIT);  /* re initialize digital hardware (STM),incase shimming */
    updt_interfiddelay(6.0*INOVA_STD_APBUS_DELAY);
    if (newacq)
    {
	putcode(GAINA);		/* set receiver gain Acode */
	setMRGains();
	setMRFilters();
	updt_interfiddelay(2.0*INOVA_STD_APBUS_DELAY);
	G_Delay(DELAY_TIME,3.0e-3,0); /*delay for setting receiver gain*/
	updt_interfiddelay(3.0e-3);
    }
    putcode(CLEAR); /* clear data table incase of fid shimming */
    if (newacq)
    {
	/* update delay after endofscan */
	updt_interfiddelay(0.5e-3+relaxdelay);	/*sw1 interrupt + relaxdelay*/
	if (interfiddelayflag == TRUE)
	{
	   if (!var_active("d0",CURRENT))
           {
		d0 = interfidovhd;
		putval("d0",d0);
	   }
	   else 
	   {
		if (d0 < interfidovhd)
		   interfidovhd = d0;
	   }
	}
	interfiddelayflag = FALSE;	/* stop accumulating overhead times */
	assign(zero,scanflag);	/* Init scan flag to false */
    }

    /* pre-acquisition delay moved to initauto2(), autofuncs.c  */
    /*if (setupflag == 0) preacqdelay();/*  pre-acquisition delay */
    
    nsc_ptr = (int) (Codeptr - Aacode); /* offset of start of scan */
    if (bgflag)
	fprintf(stderr,"nsc_ptr offset: %d \n",nsc_ptr);
    putcode(NSC);
    if (newacq)
    {
	ifzero(scanflag);
	   delay(d0-interfidovhd); /* each array, delay any excess of ovrhd */
	   assign(one,scanflag); /* scan has started */
	elsenz(scanflag);
	   delay(d0); /* during scan, delay for same period 	*/
				 /* time as for array element overhead.	*/
	endif(scanflag);
	/* For RA: Insert cleardatatable for first scan at each blocksize. */
	/* 	   Summing will be done on host */
	if ( (ra_flag) && (ix == getStartFidNum()) )
	{
	   ifzero(bsval);
	   elsenz(bsval);
	     modn(ct,bsval,tmprt);
	     ifzero(tmprt);
		clearapdatatable();
	     elsenz(tmprt);
		delay(1.0e-6);	/* delay for same time as clearapdata... */
	     endif(tmprt);
	   endif(bsval);
	}
    }


    /* For Block Size wshim we must add an shim acode within the NSC loop */
/*    if ( (oldwhenshim == 4) ) /* wshim = 'bs' */
/*    {
/*      putcode(SHIMA);             /* Acode for auto shimming */
/*      putcode(1);          /* Acode for auto shimming */
/*      putcode(INIT);  /* re initialize digital hardware (STM),incase shimming */
	              /* to avoid bustrap when wshim='f', 29K onto stack, not good! */
/*      putcode(CLEAR); /* clear data table incase of fid shimming */
/*    }
*/

    if (!newacq)
    {
      interlocktests();	/* test for lock,spin, & VT failures if requested */
    }
    init_lkrelay();
    init_dqdfrq();
    /* --- initialize expilict acquisition parameters --- */
    hwlooping = 0;
    acqtriggers = 0;
    hwloopelements = 0;
    starthwfifocnt = 0;
    grad_flag = FALSE;

/******************************************
*  Start pulsesequence and table section  *
******************************************/

    sliderfile = 0;	/* make sure it is zero, i.e. not open */
    loadtablecall = -1;	/* initializes loadtablecall variable */
    inittablevar();	/* initialize all internal table variables */

    initparms_img();  
    initsisdecblank();	/* initialize dec blanking for SIS VXR & Unity */

    SkipHSlineTest = 1; /* incase of ifzero(v1) type of constructs don't */
			/* test HSlines against presHSlines */

#ifndef AIX
    hdwshiminit();
    if (lockfid_mode) {
	putcode(LOCKSEQUENCE);
    } else {
	pulsesequence();	/* generate Acodes from USER Pulse Sequence */
        initializeSeq = 0;
    }
#endif

    SkipHSlineTest = 0; /* reset back to testing HSlines against presHSlines */
/*    presHSlines = 0; */ 	/* don't assuming any HSline state after sequence */

    for (i = 0; i < MAXTABLE; i++)
    {
       tmptable_order[i] = table_order[i];
       if (Table[i]->reset)
          reset_table(Table[i]);
    }

/*************************************************************
*  NOTE:  the master pointer for tables and each individual  *
*         table pointer are not freed explicitly.  They are  *
*         released when PSG exits.  I know this is sloppy.   *
*************************************************************/
    if (!newacq)
    {
	if (sliderfile) fclose(sliderfile);
    }
/****************************************
*  End pulsesequence and table section  *
****************************************/

    if (hwlooping)
    {
	text_error("Missing endhardloop in Pulse Sequence");
	psg_abort(1);
    }

    if (newacq)
    {
	if (isSSHAselected())
	{
	    if (isSSHAdelayTooShort())
	    {
		text_error( "WARNING: hdwshim inactive, delay > 100 ms required" );
	    }
	    if (isSSHAactive())
	    {
		text_error( "hdwshim='y' selected in a pulse sequence with no delays" );
	    }
	}
	if (isSSHAPselected())
	{
	    if (isSSHAactive() || isSSHAstillDoItNow() || isSSHAPresatInit())
	    {
		text_error( "hdwshim='p' selected in a pulse sequence with no presat" );
	    }
	    if (isSSHAdelayTooShort())
	    {
		text_error( "WARNING: hdwshim inactive, pulse > 100 ms required" );
	    }
	}
    }

    test4acquire();	/* if no acquisition has been done yet, do it */

    endPowerCheck();

    if (newacq)
    {
	putcode(EXIT);
    }

    /* compress by default!! and don't if commanded */

    if (!checkflag)
       write_Acodes(ix);

    /* write out generated lc,auto & acodes */

    if (newacq)
    {
	if (sliderfile) flush_ipalist(sliderfile);
        if ( HS_Dtm_Adc == 1 )
	   check5MhzFoo();		/* check for possible FOO on 5MHz DTM/ADC */
    }

    return;
}
static void setModInfoch(chan,dmf,dres,dmsize,dmmsize,homosize,dm,dmm,dseq,homo)
int chan;
double	*dmf;
double	*dres;
int	*dmsize;
int	*dmmsize;
int	*homosize;
char	*dm;
char	*dmm;
char	*dseq;
char	*homo;
{
   ModInfo[chan].MI_dmf = dmf;
   ModInfo[chan].MI_dres = dres;
   ModInfo[chan].MI_dmsize = dmsize;
   ModInfo[chan].MI_dmmsize = dmmsize;
   ModInfo[chan].MI_homosize = homosize;
   ModInfo[chan].MI_dm = dm;
   ModInfo[chan].MI_dmm = dmm;
   ModInfo[chan].MI_dseq = dseq;
   ModInfo[chan].MI_homo = homo;
}
static void setModInfo(hwchan,chan)
int hwchan;
int chan;
{
   static int dummy = 1;
   static char *dumval = "n";

   switch (hwchan)
   {
      case TODEV:  
            setModInfoch(chan,&xmf,  &xres,  &xmsize,   &xmmsize,    &dummy,
	                      xm,	xmm,	xseq,	   dumval);
            break;
      case DODEV:  
            setModInfoch(chan,&dmf,  &dres,  &dmsize,   &dmmsize,    &homosize,
	                      dm,	dmm,	dseq,	   homo);
            break;
      case DO2DEV:  
            setModInfoch(chan,&dmf2,  &dres2, &dm2size, &dmm2size,   &homo2size,
	                      dm2,	dmm2,	dseq2,	   homo2);
            break;
      case DO3DEV:  
            setModInfoch(chan,&dmf3,  &dres3, &dm3size, &dmm3size,   &homo3size,
	                      dm3,	dmm3,	dseq3,	   homo3);
            break;
      case DO4DEV:  
            setModInfoch(chan,&dmf4,  &dres4, &dm4size, &dmm4size,   &homo4size,
	                      dm4,	dmm4,	dseq4,	   homo4);
            break;
      default:
            break;
   }
}

static int channelvalok(num,tmpstr)
int num;
char tmpstr[];
{
   int val[6];
   int i;
   int ok = 1;

   if (strlen(tmpstr) != num)
      return(0);
   for (i=0; i < num; i++)
      val[i] = 0;
   for (i=0; i < num; i++)
      switch (tmpstr[i])
      {
         case '1': val[0] = 1;
                   break;
         case '2': val[1] = 1;
                   break;
         case '3': val[2] = 1;
                   break;
         case '4': val[3] = 1;
                   break;
         case '5': val[4] = 1;
                   break;
         case '6': val[5] = 1;
                   break;
         default:  break;
      }
   for (i=0; i < num; i++)
      if (val[i] != 1)
         ok = 0;
   return(ok);
}
static int getchannelval(index,tmpstr)
int index;
char tmpstr[];
{
   int val;
   switch (tmpstr[index-1])
   {
      case '1': val = TODEV;
                break;
      case '2': val = DODEV;
                break;
      case '3': val = DO2DEV;
                break;
      case '4': val = DO3DEV;
                break;
      case '5': val = DO3DEV;
                break;
      case '6': val = DO3DEV;
                break;
      default:  val = TODEV;
                break;
   }
   return(val);
}

/*
**  this table relates the dsp hardware to this acquistion
**  the correct entry is made by matching osfilt and key 
**  (1 char only now) and the oversample rate.
**  The table is designed to match changed software for the 
**  dsp.  Entries for each key should be in descending order
**  and new entries may be added, with DSPENTRY reflecting the
**  total # of filter combinations avalible 
*/
#define DSP_PROM_ENTRY  18

struct _dspline 
{ 
    char key[8];
    int oversamp;
    int options;
    int cmd_index;
    int ectc;
    double maxsw;
};

struct _dspline dsp_prom_tab[DSP_PROM_ENTRY] = {
{ "brick",50,0,19,86,  160000.0}, 
{ "brick",25,0,18,95,  160000.0}, 
{ "brick",20,0,17,105, 160000.0}, 
{ "brick",15,0,16,116, 160000.0}, 
{ "brick",10,0,15,128, 100000.0}, 
{ "brick", 5,0,14,151, 100000.0}, 
{ "brick", 4,0,13,181, 100000.0}, 
{ "brick", 3,0,12,216,  50000.0}, 
{ "brick", 2,0,11,256,  36000.0}, 
{ "aplus", 50,0,10,188,300000.0}, 
{ "aplus", 25,0, 9, 95,300000.0}, 
{ "aplus", 20,0, 8, 76,300000.0}, 
{ "aplus", 15,0, 7, 57,300000.0}, 
{ "aplus", 10,0, 6, 38,300000.0}, 
{ "aplus",  5,0, 5, 20,300000.0}, 
{ "aplus",  4,0, 4, 16,300000.0}, 
{ "aplus",  3,0, 3, 12,300000.0}, 
{ "aplus",  2,0, 2,  8,300000.0}, 
};

#define DSP_DOWN_ENTRY 134 
struct _dspline dsp_down_tab[DSP_DOWN_ENTRY]=  {
{ "brick", 68,0,135,256,400000.0},
{ "brick", 67,0,134,256,400000.0},
{ "brick", 66,0,133,256,400000.0},
{ "brick", 65,0,132,256,400000.0},
{ "brick", 64,0,131,256,400000.0},
{ "brick", 63,0,130,256,400000.0},
{ "brick", 62,0,129,256,400000.0},
{ "brick", 61,0,128,256,400000.0},
{ "brick", 60,0,127,256,400000.0},
{ "brick", 59,0,126,256,400000.0},
{ "brick", 58,0,125,256,400000.0},
{ "brick", 57,0,124,256,400000.0},
{ "brick", 56,0,123,256,300000.0},
{ "brick", 55,0,122,256,300000.0},
{ "brick", 54,0,121,256,300000.0},
{ "brick", 53,0,120,256,300000.0},
{ "brick", 52,0,119,256,300000.0},
{ "brick", 51,0,118,256,300000.0},
{ "brick", 50,0,117,256,300000.0},
{ "brick", 49,0,116,256,300000.0},
{ "brick", 48,0,115,256,300000.0},
{ "brick", 47,0,114,256,300000.0},
{ "brick", 46,0,113,256,300000.0},
{ "brick", 45,0,112,256,300000.0},
{ "brick", 44,0,111,256,300000.0},
{ "brick", 43,0,110,256,300000.0},
{ "brick", 42,0,109,256,300000.0},
{ "brick", 41,0,108,256,300000.0},
{ "brick", 40,0,107,256,300000.0},
{ "brick", 39,0,106,256,300000.0},
{ "brick", 38,0,105,256,300000.0},
{ "brick", 37,0,104,256,300000.0},
{ "brick", 36,0,103,256,300000.0},
{ "brick", 35,0,102,256,300000.0},
{ "brick", 34,0,101,256,300000.0},
{ "brick", 33,0,100,256,200000.0},
{ "brick", 32,0,99,256,200000.0},
{ "brick", 31,0,98,256,200000.0},
{ "brick", 30,0,97,256,200000.0},
{ "brick", 29,0,96,256,200000.0},
{ "brick", 28,0,95,256,200000.0},
{ "brick", 27,0,94,256,200000.0},
{ "brick", 26,0,93,256,200000.0},
{ "brick", 25,0,92,256,200000.0},
{ "brick", 24,0,91,256,200000.0},
{ "brick", 23,0,90,256,200000.0},
{ "brick", 22,0,89,256,100000.0},
{ "brick", 21,0,88,256,100000.0},
{ "brick", 20,0,87,256,100000.0},
{ "brick", 19,0,86,256,100000.0},
{ "brick", 18,0,85,256,100000.0},
{ "brick", 17,0,84,256,100000.0},
{ "brick", 16,0,83,256,100000.0},
{ "brick", 15,0,82,256,100000.0},
{ "brick", 14,0,81,256,100000.0},
{ "brick", 13,0,80,256,100000.0},
{ "brick", 12,0,79,256,100000.0},
{ "brick", 11,0,78,256,100000.0},
{ "brick", 10,0,77,256,100000.0},
{ "brick",  9,0,76,256,100000.0},
{ "brick",  8,0,75,256,100000.0},
{ "brick",  7,0,74,256,100000.0},
{ "brick",  6,0,73,226,100000.0},
{ "brick",  5,0,72,187,100000.0},
{ "brick",  4,0,71,146,100000.0},
{ "brick",  3,0,70,108,100000.0},
{ "brick",  2,0,69, 70,100000.0},
{ "aplus", 68,0,68,256,400000.0},
{ "aplus", 67,0,67,252,400000.0},
{ "aplus", 66,0,66,248,400000.0},
{ "aplus", 65,0,65,244,400000.0},
{ "aplus", 64,0,64,241,400000.0},
{ "aplus", 63,0,63,237,400000.0},
{ "aplus", 62,0,62,233,400000.0},
{ "aplus", 61,0,61,229,400000.0},
{ "aplus", 60,0,60,226,400000.0},
{ "aplus", 59,0,59,222,400000.0},
{ "aplus", 58,0,58,218,400000.0},
{ "aplus", 57,0,57,214,400000.0},
{ "aplus", 56,0,56,211,400000.0},
{ "aplus", 55,0,55,207,400000.0},
{ "aplus", 54,0,54,203,400000.0},
{ "aplus", 53,0,53,199,400000.0},
{ "aplus", 52,0,52,196,400000.0},
{ "aplus", 51,0,51,192,400000.0},
{ "aplus", 50,0,50,188,400000.0},
{ "aplus", 49,0,49,184,400000.0},
{ "aplus", 48,0,48,181,400000.0},
{ "aplus", 47,0,47,177,400000.0},
{ "aplus", 46,0,46,173,400000.0},
{ "aplus", 45,0,45,169,400000.0},
{ "aplus", 44,0,44,166,400000.0},
{ "aplus", 43,0,43,162,400000.0},
{ "aplus", 42,0,42,158,400000.0},
{ "aplus", 41,0,41,154,400000.0},
{ "aplus", 40,0,40,151,400000.0},
{ "aplus", 39,0,39,147,400000.0},
{ "aplus", 38,0,38,143,400000.0},
{ "aplus", 37,0,37,139,400000.0},
{ "aplus", 36,0,36,136,400000.0},
{ "aplus", 35,0,35,132,400000.0},
{ "aplus", 34,0,34,128,400000.0},
{ "aplus", 33,0,33,124,400000.0},
{ "aplus", 32,0,32,121,400000.0},
{ "aplus", 31,0,31,117,400000.0},
{ "aplus", 30,0,30,113,400000.0},
{ "aplus", 29,0,29,109,400000.0},
{ "aplus", 28,0,28,106,400000.0},
{ "aplus", 27,0,27,102,400000.0},
{ "aplus", 26,0,26, 98,400000.0},
{ "aplus", 25,0,25, 95,400000.0},
{ "aplus", 24,0,24, 91,400000.0},
{ "aplus", 23,0,23, 87,400000.0},
{ "aplus", 22,0,22, 83,400000.0},
{ "aplus", 21,0,21, 79,400000.0},
{ "aplus", 20,0,20, 76,400000.0},
{ "aplus", 19,0,19, 72,400000.0},
{ "aplus", 18,0,18, 68,400000.0},
{ "aplus", 17,0,17, 64,400000.0},
{ "aplus", 16,0,16, 61,400000.0},
{ "aplus", 15,0,15, 57,400000.0},
{ "aplus", 14,0,14, 53,400000.0},
{ "aplus", 13,0,13, 49,400000.0},
{ "aplus", 12,0,12, 46,400000.0},
{ "aplus", 11,0,11, 42,400000.0},
{ "aplus", 10,0,10, 38,400000.0},
{ "aplus",  9,0, 9, 34,400000.0},
{ "aplus",  8,0, 8, 31,400000.0},
{ "aplus",  7,0, 7, 27,400000.0},
{ "aplus",  6,0, 6, 23,400000.0},
{ "aplus",  5,0, 5, 20,380000.0},
{ "aplus",  4,0, 4, 16,370000.0},
{ "aplus",  3,0, 3, 12,350000.0},
{ "aplus",  2,0, 2,  8,300000.0},
};

static int dsp_set(swval,factor,dpchar)
double swval;
int factor;
char dpchar;
{
   char tmpstr[256];
   struct _dspline *tap;
   int i,tab_index,nentries,reduce;
   int save_factor,mask;

   save_factor = factor;

   dsp_params.flags = 0;
   dsp_params.rt_oversamp = 1;
   dsp_params.rt_extrapts = 0;
   dsp_params.il_oversamp = 1;
   dsp_params.il_extrapts = 0;
   dsp_params.rt_downsamp = 0;


   /* we are in pass thru mode if factor == 1*/
   if (factor == 1) return;

   if (P_getstring(GLOBAL,"dsptype",tmpstr,1,255))
   {
      tmpstr[0] = '\0';
   }
   switch(tmpstr[0])
   {
     /* switched default from 5.2F(f) */
     case '1':	
     default:
      tap = &(dsp_down_tab[0]);
      nentries = DSP_DOWN_ENTRY;
      break;
     case '0':	
      tap = &(dsp_prom_tab[0]);
      nentries = DSP_PROM_ENTRY;
      break;
   }

   if (P_getstring(CURRENT,"osfilt",tmpstr,1,255))
   {
      text_error("Osfilt not found - default to a!\n");
      tmpstr[0] = 'a';
      tmpstr[1] = '\0';
   }
   if ((tmpstr[0] != 'a') && (tmpstr[0] != 'b'))
   {
      tmpstr[0] = 'a';
      tmpstr[1] = '\0';
   }

   tab_index = -1; /* indicate no match */
   reduce = 0;
   do 
   { /* check for consistency and max sw */
     for (i=0;i<nentries;i++)
     {
/*        text_error("%s   %d\n",tap[i].key,i); */
/* can expand from first char now by changing 1 --- NOTICE STRNCASECMP */
        if (strncasecmp(tmpstr,(tap)[i].key,1) == 0) 
        {
           if (factor >= (tap)[i].oversamp) 
	   {
	     tab_index = i;
	     break;
	   }
        }
      }

/* look for trouble */
      if (tab_index == -1)
      {
         text_error("Could not match dsp parameters!\n");
         psg_abort(1);
      }

      if (dqd[0] == 'y')
      {
        if (DQD_IL_FACTOR*factor*sw > tap[i].maxsw)
        {
           factor = tap[i].maxsw/(DQD_IL_FACTOR*sw); /* reduce to work */	
           tab_index= -1;
           reduce++;
        }
      }
      else
      {
        if ((factor != 1) && (factor*sw > tap[i].maxsw))
        {
           factor = tap[i].maxsw/sw; /* reduce to work */	
           tab_index= -1;
           reduce++;
        }
      }
      if (factor <= 1) 
      { 
        /* pass thru always works */
        if (save_factor != 1)
        {
          text_error("oversamp = 1\n");
          putCmd("setvalue('oversamp',%d)\n",1);
          putCmd("setvalue('oversamp',%d,'processed')\n",1);
        }
        return;
      }
      
   }  while ((tab_index < 0) && (reduce < 3));

   if (save_factor != (tap)[i].oversamp )
   {
      int newoversamp;

      text_error("reducing oversamp\n");
      newoversamp = (tap)[i].oversamp;
      if (dqd[ 0 ] == 'y')
        newoversamp *= DQD_IL_FACTOR;
      putCmd("setvalue('oversamp',%d)\n",newoversamp);
      putCmd("setvalue('oversamp',%d,'processed')\n",newoversamp);
      if (bgflag)
	fprintf(stderr,"new oversamp: %d\n", newoversamp);
   }
   dsp_params.flags = (tap)[i].cmd_index;
   dsp_params.rt_oversamp = (tap)[i].oversamp;
   mask = 0x0f & (1+(int)(swval*factor/100000.0));
   if (mask > 4) mask = 4;
   dsp_params.rt_downsamp = mask;
   /* fix to ectc */
   dsp_params.rt_extrapts = (tap)[i].ectc-(tap)[i].oversamp-mask;
   if (dpchar == 'y')
   {
     dsp_params.flags |= 0x400; /* scale to 20 bits */
     setDSPgain(4);
   }
   if (bgflag)
      fprintf(stderr,"new DSP struct in dsp_set: 0x%x %d %d %d %d 0x%x\n",
	dsp_params.flags,
	dsp_params.rt_oversamp,
	dsp_params.rt_extrapts,
	dsp_params.il_oversamp,
	dsp_params.il_extrapts,
	dsp_params.rt_downsamp);
}

/*-----------------------------------------------------------------
|	initparms()/
|	initializes the main variables used 
|   Modified   Author     Purpose
|   --------   ------     -------
|   12/13/88   Greg B.    1. Added Code to initialize fine attenuator
|			     parameters tpwrf,dpwrf.
|			  2. initialize dpwr, now a standard parameter
|			     replacing the coarse attenuator function
|			     of dhp.
|    4/21/89   Greg B.    1. Added Code to initialize interleave parameter (il)
+------------------------------------------------------------------*/
initparms()
{
    double getval();
    double getvalnwarn();
    double tmpval;
    char   tmpstr[20];
    char   audfilter[12];
    char   dpchar;
    int    tmp, getchan;
    vInfo  info;
    double maxlbsw;
    extern int 	  rotorSync;

    sw = getval("sw");
    np = getval("np");

    if ( P_getreal(CURRENT,"nf",&nf,1) < 0 )
    {
        nf = 0.0;                /* if not found assume 0 */
    }
    if (nf < 2.0) 
	nf = 1.0;

/*****************************
*  Set the observe channel.  *
*****************************/

    if (ap_interface == 4)
    {
       if ( (P_getstring(CURRENT, "dn", tmpstr, 1, 9) == 0) &&
            strcmp(tmpstr,"H1") && (tmpstr[0] != '\000') )
       {
          if ( (P_getstring(GLOBAL, "rfchtype", tmpstr, 2, 12) == 0) &&
               !strcmp(tmpstr,"U+ H1 Only") &&
               !tuneflag )
          {
             int tmp;
             tmp = OBSch;
             OBSch = DECch;
             DECch = tmp;
             setRFInfo(TODEV,OBSch);
             setModInfo(TODEV,OBSch);
             setRFInfo(DODEV,DECch);
             setModInfo(DODEV,DECch);
          }
       }
       if (P_getstring(CURRENT, "rfchannel", tmpstr, 1, 12) == 0)
       {
          if ( channelvalok(NUMch,tmpstr) )
          {
             OBSch = getchannelval(1,tmpstr);
             setRFInfo(TODEV,OBSch);
             setModInfo(TODEV,OBSch);
             if (NUMch >= 2)
             {
                DECch = getchannelval(2,tmpstr);
                setRFInfo(DODEV,DECch);
                setModInfo(DODEV,DECch);
                if (NUMch >= 3)
                {
                   DEC2ch = getchannelval(3,tmpstr);
                   setRFInfo(DO2DEV,DEC2ch);
                   setModInfo(DO2DEV,DEC2ch);
                   if (NUMch >= 4)
                   {
                      DEC3ch = getchannelval(4,tmpstr);
                      setRFInfo(DO3DEV,DEC3ch);
                      setModInfo(DO3DEV,DEC3ch);
                   }
                }
             }
          }
          else
          {
             fprintf(stdout,"Invalid value for rfchannel.\n");
             psg_abort(1);
          }
       }
       char nucleiNameStr[MAXSTR], nucleus[MAXSTR], strbuf[MAXSTR];  
       nucleiNameStr[0]='\0'; strbuf[0]='\0'; nucleus[0]='\0';
       strcpy(nucleiNameStr,"'");

       int i;
       for(i=1; i<=NUMch; i++)
       {
            if (i == OBSch)
            {
               getstrnwarn("tn",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            } 
            else if (i == DECch)
            {
               getstrnwarn("dn",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            } 
            else if (i == DEC2ch) 
            {
               getstrnwarn("dn2",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            } 
            else if (i == DEC3ch) 
            {
               getstrnwarn("dn3",nucleus);
               if (strcmp(nucleus,"")==0)
                  strcpy(nucleus,"-");
               sprintf(strbuf,"%s ",nucleus);
               strcat(nucleiNameStr,strbuf);
            }
       }
       nucleiNameStr[strlen(nucleiNameStr)-1] = '\0';
       strcat(nucleiNameStr,"'");
       if (P_getstring(CURRENT, "rfchnuclei", strbuf, 1, 255) >= 0)
       {
          putCmd("rfchnuclei = %s",nucleiNameStr);
       }

       if (bgflag)
         fprintf(stderr,"Obs= %d Dec= %d Dec2= %d Dec3= %d\n",
                         OBSch,DECch,DEC2ch,DEC3ch);
       xmf = getvalnwarn("xmf");	/* observe modulation freq */
       if (xmf == 0.0) xmf=1000.0;
/*       getstrnwarn("xm",xm); */
/*       if (xm[0]==0) strcpy(xm,"n"); */
       strcpy(xm,"n");
       xmsize = strlen(xm);
/*       getstrnwarn("xmm",xmm); */
/*       if (xmm[0]==0) strcpy(xmm,"c"); */
       strcpy(xmm,"c");
       xmmsize = strlen(xmm);
       getstrnwarn("xseq",xseq);
       xres = getvalnwarn("xres");	/* prg decoupler digital resolution */
       if (xres < 1.0)
          xres = 1.0;
    }
    if ( P_getreal(CURRENT, "locktc", &tmpval, 1) < 0 )
    {
       if ( P_getreal(GLOBAL, "locktc", &tmpval, 1) < 0 )
          tmpval = 1.0;
    }
    lkflt_fast = (codeint) (tmpval + 0.5);
    if ( P_getreal(CURRENT, "lockacqtc", &tmpval, 1) < 0 )
    {
       if ( P_getreal(GLOBAL, "lockacqtc", &tmpval, 1) < 0 )
          tmpval = 4.0;
    }
    lkflt_slow = (codeint) (tmpval + 0.5);
    if (ap_interface == 4)
    {  if ( (lkflt_fast < 1) || (lkflt_fast > 4) ) lkflt_fast=(codeint)1;
       if ( (lkflt_slow < 1) || (lkflt_slow > 4) ) lkflt_slow=(codeint)4;
    }
    else
    {  if (lkflt_fast != 2) lkflt_fast=(codeint)1;
       if (lkflt_fast == 2) lkflt_fast=(codeint)4;
       if (lkflt_slow != 1) lkflt_slow=(codeint)4;
    }

    /* Determine fb limit */
    if (P_getreal(GLOBAL, "maxsw_loband", &maxlbsw, 1) < 0){
	maxlbsw = 100000.0;
    }

    nt = getval("nt");
    sfrq = getval("sfrq");

/* Digital Quadrature Detection */

    if ((tmp=P_getstring(GLOBAL, "fsq", tmpstr, 1, 9)) < 0)
      dqd[ 0 ] = 'n';
    else
      dqd[ 0 ] = tmpstr[ 0 ];

    if ((P_getstring(CURRENT, "dp", tmpstr, 1, 9)) < 0)
      dpchar = 'n';
    else
      dpchar = tmpstr[ 0 ];
    getstr("il",il);		/* interleave parameter */
    if (P_getstring(GLOBAL,"dsp",tmpstr,1,12) < 0)
       tmpstr[0] = 'n';
    if (sw > maxlbsw + 1.0)  /* if WideLine */
    {
       tmpstr[0] = 'n';      /* no DSP */
       if (!newacq)          /* also if not inova */
       {
          if (dpchar == 'n')     /* single precision only */
          {
             text_error("must set dp='y' if sw is greater than %g",maxlbsw);
             psg_abort(1);
          }
          else if (il[0] != 'n') /* no interleaving */
          {
             text_error("must set il='n' if sw is greater than %g",maxlbsw);
             psg_abort(1);
          }
       }
    }
    if (dps_flag)            /* no DSP if dps */
       tmpstr[0] = 'n';

    dsp_params.flags = 0;
    dsp_params.rt_oversamp = 1;
    dsp_params.rt_extrapts = 0;
    dsp_params.il_oversamp = 1;
    dsp_params.il_extrapts = 0;
    dsp_params.rt_downsamp = 1;
   
    if ( (tmpstr[0] == 'r') &&
              !P_getVarInfo(CURRENT, "oversamp", &info) &&
              info.active && (nf < 1.5) )
    {
       double tmpval;
       int factor=1;
       if (!P_getreal(CURRENT, "oversamp", &tmpval, 1))
       {
          double maxsw;

          maxsw = maxlbsw;
          factor = (int)(tmpval+0.5);

          if (dqd[ 0 ] == 'y' && newacq && factor >= DQD_IL_FACTOR) {
             int newoversamp;

/*  Next instruction relies on integer arithmetic to lower oversamp to
    the largest multiple of DQD_IL_FACTOR that is less than oversamp,
    if oversamp is itself not a multiple of DQD_IL_FACTOR.		*/

             newoversamp = DQD_IL_FACTOR * (factor / DQD_IL_FACTOR);

             if (newoversamp < DQD_IL_FACTOR)
               newoversamp = DQD_IL_FACTOR;  /* already covered by if above */

             putCmd("setvalue('oversamp',%d)\n",newoversamp);
             putCmd("setvalue('oversamp',%d,'processed')\n",newoversamp);

             if (bgflag)
               fprintf( stderr, "new oversamp: %d, rt oversamp: %d, il oversamp: %d\n",
		  newoversamp, newoversamp/DQD_IL_FACTOR, DQD_IL_FACTOR );
	     factor = newoversamp / DQD_IL_FACTOR;
		  }

		  if (!newacq && (factor > 20))
		     factor = 20;
	/*
		  else if (newacq)
		  {
		     if (factor > 50)
			factor = 50;
		     if (factor >= 10)
			maxsw = 400000.0;
		     else
			maxsw = 300000.0;
		  }
	*/
		  if (!newacq && ((sw * factor) > (maxsw + 0.01) ))
		  {
		     text_error("oversamp * sw > max sw (%g)\n",maxsw);
		     psg_abort(1);
		  }
		  if (!newacq && (sw > 42000.0) )
		  {
		     if (P_getstring(CURRENT, "osfilt", tmpstr, 1, 9) >= 0 )
			if ( (tmpstr[0] == 'b') || (tmpstr[0] == 'B') )
			{
			   text_error("sw cannot be greater than 42000 for osfilt='b'\n");
			   psg_abort(1);
			}
		  }
	       }
	       if (factor >= 2)
	       {
		  dsp_params.flags = 1;  /* hardware DSP flag */
		  dsp_params.rt_oversamp = factor;  /* DSP oversampling factor */
		  if (newacq)
		  {
		     dsp_set(sw,factor,dpchar);
		  }
		  else if (factor <= 7)
		  {
		     dsp_params.rt_extrapts = 52;  /* DSP extra complex points */
		  }
		  else
		  {
		     dsp_params.rt_extrapts = 127;  /* DSP extra complex points */
		  }
		  dsp_params.rt_extrapts *= 2;  /* DSP extra total data points */
	       }
         if (bgflag)
            fprintf(stderr,"new DSP struct with dsp='r': 0x%x %d %d %d %d 0x%x\n",
			    dsp_params.flags,
			    dsp_params.rt_oversamp,
			    dsp_params.rt_extrapts,
			    dsp_params.il_oversamp,
			    dsp_params.il_extrapts,
			    dsp_params.rt_downsamp);
    }

    if (( (tmpstr[0] == 'i') && !acqiflag &&
              !P_getVarInfo(CURRENT, "oversamp", &info) &&
              info.active && (nf < 1.5) ) ||
	  (dqd[ 0 ] == 'y' && tmpstr[0] == 'r'))
    {
       double tmpval;
       int factor;  /* overrides previous value of factor?? */

       tmpval = 2.0;
       factor = 1;

       if (dqd[ 0 ] == 'y' && tmpstr[0] == 'r') {
          factor = DQD_IL_FACTOR;
       }
       else if (P_getreal(CURRENT, "oversamp", &tmpval, 1) == 0)
       {
          factor = (int)(tmpval+0.5);
       }

       if ( P_getreal(CURRENT, "oscoef", &tmpval, 1) == 0 )
           if ((tmpval >= 3) && (factor >= 2) )
           {
              dsp_params.il_oversamp = factor;  /* DSP oversampling factor */
              dsp_params.il_extrapts = (int) (tmpval / 2.0);  /* DSP extra complex pts */
              dsp_params.il_extrapts *=  2;  /* DSP extra total data points */
              P_setreal(CURRENT,"oscoef",(double)(dsp_params.il_extrapts + 1), 1);
              if ( getDSPinfo(factor,dsp_params.il_extrapts + 1, sw, maxlbsw) )
              {
                 psg_abort(1);
              }

           }

       if (bgflag)
         fprintf(stderr,"new DSP struct with dsp='i': 0x%x %d %d %d %d 0x%x\n",
                         dsp_params.flags,
                         dsp_params.rt_oversamp,
                         dsp_params.rt_extrapts,
                         dsp_params.il_oversamp,
                         dsp_params.il_extrapts,
                         dsp_params.rt_downsamp);
    }

    if (P_getstring(GLOBAL,"audiofilter",audfilter,1,12) < 0){
       audfilter[0] = 'e';
       audfilter[1] = '\0';
    }
    if (sw >= maxlbsw+0.1){
	beta = POLE6;
    }else if (audfilter[0] == 'b'){
	beta = POLE4;
    }else if (audfilter[0] == '2'){
	beta = POLE8BUTTER;
    }else{
	beta = POLE8;
    }
    fb = getval("fb") * dsp_params.rt_oversamp * dsp_params.il_oversamp;
    filter = getval("filter");		/* pulse Amp filter setting */
    tof = getval("tof");
    bs = getval("bs");
    if (!var_active("bs",CURRENT))
	bs = 0.0;
    pw = getval("pw");
    pw90 = getval("pw90");
    p1 = getval("p1");

    pwx = getvalnwarn("pwx");
    pwxlvl = getvalnwarn("pwxlvl");
    tau = getvalnwarn("tau");
    satdly = getvalnwarn("satdly");
    satfrq = getvalnwarn("satfrq");
    satpwr = getvalnwarn("satpwr");
    getstrnwarn("satmode",satmode);

    /* --- delays --- */
    d1 = getval("d1"); 		/* delay */
    d2 = getval("d2"); 		/* a delay: used in 2D experiments */
    d3 = getvalnwarn("d3");	/* a delay: used in 3D experiments */
    d4 = getvalnwarn("d4");	/* a delay: used in 4D experiments */
    phase1 = (int) sign_add(getvalnwarn("phase"),0.005);
    phase2 = (int) sign_add(getvalnwarn("phase2"),0.005);
    phase3 = (int) sign_add(getvalnwarn("phase3"),0.005);
    rof1 = getval("rof1"); 	/* Time receiver is turned off before pulse */
    rof2 = getval("rof2");	/* Time after pulse before receiver turned on */
    alfa = getval("alfa"); 	/* Time after rec is turned on that acqbegins */
    pad = getval("pad"); 	/* Pre-acquisition delay */
    padactive = var_active("pad",CURRENT);
    hst = getval("hst"); 	/* HomoSpoil delay */

#ifndef VIS_ACQ
    tpwr = ( (cattn[TODEV] != 0.0) ? getval("tpwr") : 0.0 );
    if ( P_getreal(CURRENT,"tpwrm",&tpwrf,1) < 0 )
       if ( P_getreal(CURRENT,"tpwrf",&tpwrf,1) < 0 )
          tpwrf = 4095.0;
#else
    tpwr = getval("tpwr");	/* SISCO RF Attenuation Values */
#endif

    getstr("rfband",rfband);	/* RF band, high or low */

    getstr("hs",hs);
    hssize = strlen(hs);
    /* setlockmode(); */		/* set up lockmode variable,homo bits */
    if (bgflag)
    {
      fprintf(stderr,"sw = %lf, sfrq = %10.8lf\n",sw,sfrq);
      fprintf(stderr,"hs='%s',%d\n",hs,hssize);
    }
    gain = getval("gain");
    /* if the preamp mixer is pre 20 Mhz IF for unity+ and 	*/
    /* inova systems 500 Mhz and above, the broadband (lowband) */
    /* mixer does not have 18 dB attenuators.  This means the	*/
    /* lowest receiver gain possible is 18.			*/ 
    if (whatiffreq(0) == 10.5) 
    {
       if (rftype[0] == 'd' && H1freq > 450)
          if (sfrq < 310)
             if (gain < 18)
             {  gain=18;
                text_error("gain reset to 18");
             }
    }
    gainactive = var_active("gain",CURRENT); /* non arrayable */
    /* InterLocks is set by go.  It will have three chars.
     * char 0 is for lock
     * char 1 is for spin
     * char 2 is for temp
     */
    getstr("interLocks",interLock); /* non arrayable */
    spin = (int) sign_add(getval("spin"),0.005);
    /* Do not try to regulate the liquids spin controller if
     * the MAS spin controller is in use
     */
    if ( (P_getstring(GLOBAL,"spintype",tmpstr,1,12) == 0) &&
         ( ! strcmp(tmpstr,"mas") ) )
       spinactive = 0;
    else
       spinactive = var_active("spin",CURRENT); /* non arrayable */

    HSrotor = 0;  /* high speed spinner selected */
    if (spin >= (int) sign_add(getval("spinThresh"),0.005))
    {
       /* Selected Solids spinner */
       HSrotor = 1;
       if (!newacq)
       {
          spinactive = 0;     /* cannot control spinner if not Inova */
          if (rotorSync == 0) /* if no rotorsync, cannot read spin speed */
             HSrotor = 0;
       }
    }
    vttemp = getval("temp");	/* get vt temperature */
    tempactive = var_active("temp",CURRENT); /* non arrayable */
    vtwait = getval("vtwait");	/* get vt timeout setting */
    vtc = getval("vtc");	/* get vt timeout setting */
    if (getparm("traymax","real",GLOBAL,&tmpval,1))
    {
      traymax=0;
    }
    else
    {
      traymax= (int) (tmpval + 0.5);
    } 

    if (getparm("loc","real",GLOBAL,&tmpval,1))
	psg_abort(1); 
    if (!var_active("loc",GLOBAL))
    {
        locActive = 0;
        tmpval = 0.0;
        if (setparm("loc","real",GLOBAL,&tmpval,1))
          psg_abort(1);
    }
    else
      locActive = 1;


    loc = (int) sign_add(tmpval,0.005);

    /* if using Gilson Liquid Handler Racks then gilpar is defined
     * and is an array of 4 values */
    if ((traymax == 96) || (traymax == (8*96)))  /* test for Gilson/Hermes */
    {
       long trayloc = 0;
       long trayzone = 0;

       if ( P_getreal(GLOBAL, "vrack", &tmpval, 1) >= 0 )
          trayloc = (int) (tmpval + 0.5);

       if ( P_getreal(GLOBAL, "vzone", &tmpval, 1) >= 0 )
          trayzone = (int) (tmpval + 0.5);

       /* rrzzllll */
       loc = loc + (10000 * trayzone) + (1000000 * trayloc);
       if (bgflag)
          fprintf(stderr,"GILSON: ----- vrack: %ld, vzone: %ld, Encoded Loc = %d\n",trayloc,trayzone,loc);
    }

    getstr("alock",alock);
    getstr("wshim",wshim);
    getlockmode(alock,&lockmode);		/* type of autolocking */
    whenshim = setshimflag(wshim,&shimatanyfid); /* when to shim */

    if ( ( tmp=P_getstring(CURRENT, "dn", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn" does not exist, don't bother with the rest of channel 2 */
    getchan = (NUMch > 1) && getchan && (tmpstr[0]!='\000');
    if (getchan)		/* variables associated with 2nd channel */
    {
       dfrq = getval("dfrq");
       dmf  = getval("dmf");		/* 1st decoupler modulation freq */
       dof  = getval("dof");
       dres = getvalnwarn("dres");	/* prg decoupler digital resolution */
       if (dres < 1.0) dres = 1.0;
       getstrnwarn("dseq",dseq);
       if (cattn[DODEV] != 0.0)
       {
          dpwr = getval("dpwr");
          dhp     = 0.0;
          dhpflag = FALSE;
          dlp     = 0.0;
       }
       else
       {
          dhp = getval("dhp");
          dlp = getval("dlp");
          dhpflag = var_active("dhp", CURRENT);
          dpwr = dhp;
       }
       if ( P_getreal(CURRENT,"dpwrm",&dpwrf,1) < 0 )
          if ( P_getreal(CURRENT,"dpwrf",&dpwrf,1) < 0 )
             dpwrf = 4095.0;
       getstr("dm",dm);
       dmsize = strlen(dm);
       getstr("dmm",dmm);
       dmmsize = strlen(dmm);
       getstr("homo",homo);
       homosize = strlen(homo);
    }
    else
    {
       dfrq    = 1.0;
       dmf     = 1000;
       dof     = 0.0;
       dres    = 1.0;
       dseq[0] = '\000';
       dhp     = 0.0;
       dhpflag = FALSE;
       dlp     = 0.0;
       dpwr    = 0.0;
       dpwrf   = 0.0;
       strcpy(dm,"n");
       dmsize  = 1;
       strcpy(dmm,"c");
       dmmsize = 1;
       strcpy(homo,"n");
       homosize= 1;
    }
    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next line are default values for chan 2\n");
       fprintf(stderr,"dm='%s',%d, dmm='%s',%d\n",dm,dmsize,dmm,dmmsize);
       fprintf(stderr,"homo='%s',%d\n",homo,homosize);
    }

    if ( (tmp=P_getstring(CURRENT, "dn2", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn2" does not exist, don't bother with the rest of channel 3 */
    getchan = (NUMch > 2) && getchan && (tmpstr[0]!='\000');
    if (getchan)			/* variables associated with 3rd channel */
    {
      dfrq2 = getval("dfrq2");
      dmf2  = getval("dmf2");		/* 2nd decoupler modulation freq */
      dof2  = getval("dof2");
      dres2 = getvalnwarn("dres2");	/* prg decoupler digital resolution */
      if (dres2 < 1.0) dres2 = 1.0;
      getstrnwarn("dseq2",dseq2);
      dpwr2 = ( (cattn[DO2DEV] != 0.0) ? getval("dpwr2") : 0.0 );
      if ( P_getreal(CURRENT,"dpwrm2",&dpwrf2,1) < 0 )
         if ( P_getreal(CURRENT,"dpwrf2",&dpwrf2,1) < 0 )
            dpwrf2 = 4095.0;
      getstr("dm2",dm2);
      dm2size = strlen(dm2);
      getstr("dmm2",dmm2);
      dmm2size = strlen(dmm2);
      getstr("homo2",homo2);
      homo2size = strlen(homo2);
    }
    else
    {
       dfrq2    = 1.0;
       dmf2     = 1000;
       dof2     = 0.0;
       dres2    = 1.0;
       dseq2[0] = '\000';
       dpwr2    = 0.0;
       dpwrf2   = 0.0;
       strcpy(dm2,"n");
       dm2size  = 1;
       strcpy(dmm2,"c");
       dmm2size = 1;
       strcpy(homo2,"n");
       homo2size= 1;
    }
    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next two lines are default values for chan 3\n");
       fprintf(stderr,"dfrq2 = %10.8lf, dof2 = %10.8lf, dpwr2 = %lf\n",
	   dfrq2,dof2,dpwr2);
       fprintf(stderr,"dmf2 = %10.8lf, dm2='%s',%d, dmm2='%s',%d\n",
	   dmf2,dm2,dm2size,dmm2,dmm2size);
       fprintf(stderr,"homo2='%s',%d\n",homo2,homo2size);
    }

    if ( (tmp=P_getstring(CURRENT, "dn3", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn3" does not exist, don't bother with the rest of channel 3 */
    getchan = (NUMch > 3) && getchan && (tmpstr[0]!='\000');
    if (getchan)			/* variables associated with 3rd channel */
    {
      dfrq3 = getval("dfrq3");
      dmf3  = getval("dmf3");		/* 3nd decoupler modulation freq */
      dof3  = getval("dof3");
      dres3 = getvalnwarn("dres3");	/* prg decoupler digital resolution */
      if (dres3 < 1.0) dres3 = 1.0;
      getstrnwarn("dseq3",dseq3);
      dpwr3 = ( (cattn[DO3DEV] != 0.0) ? getval("dpwr3") : 0.0 );
      if ( P_getreal(CURRENT,"dpwrm3",&dpwrf3,1) < 0 )
         if ( P_getreal(CURRENT,"dpwrf3",&dpwrf3,1) < 0 )
            dpwrf3 = 4095.0;
      getstr("dm3",dm3);
      dm3size = strlen(dm3);
      getstr("dmm3",dmm3);
      dmm3size = strlen(dmm3);
      getstr("homo3",homo3);
      homo3size = strlen(homo3);
    }
    else
    {
       dfrq3    = 1.0;
       dmf3     = 1000;
       dof3     = 0.0;
       dres3    = 1.0;
       dseq3[0] = '\000';
       dpwr3    = 0.0;
       dpwrf3   = 0.0;
       strcpy(dm3,"n");
       dm3size  = 1;
       strcpy(dmm3,"c");
       dmm3size = 1;
       strcpy(homo3,"n");
       homo3size= 1;
    }

    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next two lines are default values for chan 3\n");
       fprintf(stderr,"dfrq3 = %10.8lf, dof3 = %10.8lf, dpwr3 = %lf\n",
	   dfrq3,dof3,dpwr3);
       fprintf(stderr,"dmf3 = %10.8lf, dm3='%s',%d, dmm3='%s',%d\n",
	   dmf3,dm3,dm3size,dmm3,dmm3size);
       fprintf(stderr,"homo3='%s',%d\n",homo3,homo3size);
    }

    if ( (tmp=P_getstring(CURRENT, "dn4", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn4" does not exist, don't bother with the rest of channel 4 */
    getchan = (NUMch > 4) && getchan && (tmpstr[0]!='\000');
    if (getchan)			/* variables associated with 4th channel */
    {
      dfrq4 = getval("dfrq4");
      dmf4  = getval("dmf4");		/* 4nd decoupler modulation freq */
      dof4  = getval("dof4");
      dres4 = getvalnwarn("dres4");	/* prg decoupler digital resolution */
      if (dres4 < 1.0) dres4 = 1.0;
      getstrnwarn("dseq4",dseq4);
      dpwr4 = ( (cattn[DO4DEV] != 0.0) ? getval("dpwr4") : 0.0 );
      if ( P_getreal(CURRENT,"dpwrm4",&dpwrf4,1) < 0 )
         if ( P_getreal(CURRENT,"dpwrf4",&dpwrf4,1) < 0 )
            dpwrf4 = 4095.0;
      getstr("dm4",dm4);
      dm4size = strlen(dm4);
      getstr("dmm4",dmm4);
      dmm4size = strlen(dmm4);
      getstr("homo4",homo4);
      homo4size = strlen(homo4);
    }
    else
    {
       dfrq4    = 1.0;
       dmf4     = 1000;
       dof4     = 0.0;
       dres4    = 1.0;
       dseq4[0] = '\000';
       dpwr4    = 0.0;
       dpwrf4   = 0.0;
       strcpy(dm4,"n");
       dm4size  = 1;
       strcpy(dmm4,"c");
       dmm4size = 1;
       strcpy(homo4,"n");
       homo4size= 1;
    }

    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next two lines are default values for chan 4\n");
       fprintf(stderr,"dfrq4 = %10.8lf, dof4 = %10.8lf, dpwr4 = %lf\n",
	   dfrq4,dof4,dpwr4);
       fprintf(stderr,"dmf4 = %10.8lf, dm4='%s',%d, dmm4='%s',%d\n",
	   dmf4,dm4,dm4size,dmm4,dmm4size);
       fprintf(stderr,"homo4='%s',%d\n",homo4,homo4size);
    }

}
/*-----------------------------------------------------------------
|	getval()/1
|	returns value of variable 
+------------------------------------------------------------------*/
double getval(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
        fprintf(stdout,"'%s': not found, value assigned to zero.\n",variable);
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstr()/1
|	returns string value of variable 
+------------------------------------------------------------------*/
void getstr(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {   
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
        fprintf(stdout,"'%s': not found, value assigned to null.\n",variable);
	buf[0] = 0;
    }
}
/*-----------------------------------------------------------------
|	getvalnwarn()/1
|	returns value of variable 
+------------------------------------------------------------------*/
double getvalnwarn(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstrnwarn()/1
|	returns string value of variable 
+------------------------------------------------------------------*/
void getstrnwarn(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {   
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
	buf[0] = 0;
    }
}
/*-----------------------------------------------------------------
|	sign_add()/2
|  	 uses sign of first argument to decide to add or subtract 
|			second argument	to first
|	returns new value (double)
+------------------------------------------------------------------*/
double sign_add(arg1,arg2)
double arg1;
double arg2;
{
    if (arg1 >= 0.0)
	return(arg1 + arg2);
    else
	return(arg1 - arg2);
}
/*-----------------------------------------------------------------
|	putcode()/1
|	puts integer word into Codes array, increments pointer
|
+------------------------------------------------------------------*/
int putcode(codeint word)
{
    if (bgflag)
	fprintf(stderr,"Code(%p) = %d(dec) or %4x(hex) \n",
		Codeptr,word,word);
    *Codeptr++ = word; 		/* Put word into Codes array */

    /* test for acodes overflowing malloc acode memory */
    if ((long)Codeptr >= CodeEnd)  
    {    
        char msge[128];
	sprintf(msge,"Too many Acodes.");
	text_error(msge);
	psg_abort(0);
    }
    return(0);
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
/*-----------------------------------------------------------------
|	clearapdatatable()/0
|	zeros the data table 
+------------------------------------------------------------------*/
clearapdatatable()
{
    notinhwloop("clearapdatatable");
    putcode(CLEARDATA);
}

#define COMPSIZE  254	/* byte maximum */	
/*********************************************
*   acode compression code 
*   idea and design
*
*   comparing a source and reference 
*   by XOR and run length encoding 
*   the result 
*   returns the compressed size
**********************************************/
typedef unsigned char BASICSIZE;

int 
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

#define NO_CODE	   0
#define REF_CODE  64
#define CMP_CODE 128

extern codelong *CodeIndex;

static int 
do_ref_tab(int num)
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
find_reference(int num)
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

put_code_offset(ptr,size,refnum,checknum)
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
|	write_Acodes()/1  action
|	writes lc,auto & acodes out to disk file 
+------------------------------------------------------------------*/
extern codeint *convert_Acodes();
extern codeint *preCodes;
static int max_acode = 0;
static int do_compress = 0;

init_compress(int num)
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

static void write_Acodes(int act)
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
    sizetowrite = ((long)Codeptr - (long)Codes);
    pstar = (unsigned char *) Codes;
    check = sizetowrite;
    pstar = (unsigned char *) preCodes;
    bufn = 0;  /* normal block */
    kind = NO_CODE;
    if (newacq)
    {
       pstar = (unsigned char *)convert_Acodes(Aacode, Codeptr, &newcodesize);
       check = sizetowrite = newcodesize;
    	if (bgflag) 
    	{
          fprintf(stderr,"convert_Acodes(): code start %p, end %p, size %d\n",
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
    /* skip over the code offset information */
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
        fprintf(stderr,"write_Acodes(): Codeptr %p, Codes %p,\
          size = 0x%lx, %ld \n",Codeptr,Codes,(long)Codeptr - (long)Codes,
          	(long)Codeptr - (long)Codes);
        fprintf(stderr,"write_Acodes(): act = %d,%d bytes written\n",act,bytes);
    }
}

init_codefile(codepath)
char *codepath;
{
    if (bgflag)
      fprintf(stderr,"Opening Code file: '%s' \n",codepath);
    unlink(codepath);
    PSfile = open(codepath,O_EXCL | O_WRONLY | O_CREAT,0666);
    if (PSfile < 0)
    {	text_error("code file already exists. PSG Aborted..\n");
	psg_abort(1);
    }
}

close_codefile()
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

/*-----------------------------------------------------------------
|       getmaxval()/1
|       Gets the maximum value of an arrayed or list real parameter. 
+------------------------------------------------------------------*/
int getmaxval(const char *parname )
{
    int      size,r,i,tmpval,maxval;
    double   dval;
    vInfo    varinfo;

    if (r = P_getVarInfo(CURRENT, parname, &varinfo)) {
        printf("getmaxval: could not find the parameter \"%s\"\n",parname);
	psg_abort(1);
    }
    if ((int)varinfo.basicType != 1) {
        printf("getmaxval: \"%s\" is not an array of reals.\n",parname);
	psg_abort(1);
    }

    size = (int)varinfo.size;
    maxval = 0;
    for (i=0; i<size; i++) {
        if ( P_getreal(CURRENT,parname,&dval,i+1) ) {
	    printf("getmaxval: problem getting array element %d.\n",i+1);
	    psg_abort(1);
	}
	tmpval = (int)(dval+0.5);
	if (tmpval > maxval)	maxval = tmpval;
    }
    return(maxval);

}


/*-----------------------------------------------------------------
|	putval()/2
|	Sets a vnmr parmeter to a given value.
+------------------------------------------------------------------*/
putval(paramname,paramvalue)
char *paramname;
double paramvalue;
{
   int stat,expnum;
   char	message[STDVAR];
   char addr[MAXSTR];

   expnum = getExpNum();
   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	sprintf(addr,"putval: cannot get Vnmr address for %s.\n",paramname);
        text_error(addr);
   }
   else
   {
   	sprintf(message,"sysputval(%d,'%s',%g)\n",
			expnum,paramname,paramvalue);
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   sprintf(addr,"putval: Error in parameter: %s.\n",paramname);
           text_error(addr);
	}
   }
   
}

/*-----------------------------------------------------------------
|	putstr()/2
|	Sets a vnmr parmeter to a given string.
+------------------------------------------------------------------*/
putstr(paramname,paramstring)
char *paramname;
char *paramstring;
{
   int stat,expnum;
   char	message[STDVAR];
   char addr[MAXSTR];

   expnum = getExpNum();
   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	sprintf(addr,"putval: cannot get Vnmr address for %s.\n",paramname);
        text_error(addr);
   }
   else
   {
   	sprintf(message,"sysputval(%d,'%s','%s')\n",
					expnum,paramname,paramstring);
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   sprintf(addr,"putval: Error in parameter: %s.\n",paramname);
           text_error(addr);
	}
   }
   
}

/*-----------------------------------------------------------------
|       init_interfiddelay()/1
|       Initializes variables for interfiddelay. 
+------------------------------------------------------------------*/
init_interfiddelay()
{
    interfiddelayflag = FALSE;
    if (newacq && (ix==1))
    {
        d0 = 0.0;
	interfidovhd = 0.0;
    	if ( P_getreal(CURRENT,"d0",&d0,1) >= 0 )
        {
	   if ((!var_active("d0",CURRENT)) || (d0 > 0.0))
           {
		interfiddelayflag = TRUE;
           }
        }
    }
}

/*-----------------------------------------------------------------
|       updt_interfiddelay()/1
|       Updates delay time "interfiddelay" with more time. 
+------------------------------------------------------------------*/
updt_interfiddelay(updttime)
double updttime;
{
    totaltime = totaltime + updttime;
    if (interfiddelayflag)
	interfidovhd = interfidovhd + updttime;
}



