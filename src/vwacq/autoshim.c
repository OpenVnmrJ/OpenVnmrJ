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


#if defined(INOVA) || defined(MERCURY)
typedef unsigned long ulong_t;
#include "lock_interface.h"
#else
#define INTOVF 32767
#define getstatblock() sendConsoleStatus()
#define get_acqstate() getAcqState()
#define update_acqstate(acqstate) setAcqState(acqstate)
#endif
#include "simplex.h"
#include "logMsgLib.h"
#include "hostAcqStructs.h"
/*
		Phil Hornung
		Varian Central Research
	      	February 1, 1985, May 3, 1985, May 31, 1985, June 17, 1985
		January 16,1996 

		Greg Brissey
	  	R & D
	      	July 15, 1985

relaxation time measurements tried
manual and automation shim control version

with air spinner control and status tested
aborts on automatic mode spinner failure
lock receiver gain adjustment added 7/9/85  GB
maximum simplex time allowed  added 7/10/85 GB
allow aborting in noise est. & T1/2 GB
if signal gets to large gain, is dropped 6db    GB

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

the bit in the shim command block is checked after the defaults and
initialization is done

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
the background mode is toggled on with the "Q," command which comes to
us as QUICK_SHIM or 8.  
*/

#define DONE_SH     0
#define LOCK_SH     1
#define FID_SH      2
#define SEL_COIL    3
#define DELAY       4
#define SPIN_ON     5
#define SPIN_OFF    6
#define MAX_TIME    7
#define QUICK_SHIM  8

/*  shim criteria definitions  */

#define LOOSE 1
#define MEDIUM 2
#define TIGHT 3
#define EXCELLENT 4

typedef unsigned long time_t2;
int		spinrate;
int		fid_init;


/* GLOBAL.c file */

int             mode_flag;


char       	criteria,
                f_crit;
int             adjLkGainforShim;
int             bestindex,
                delaytime,
                delayflag,
                fid_shim,	/* shim on FID if true,lock if false */
                fs_scl_fct,	/* scaling factor for fidshim */
                fox = 0,
		g_maxpwr,	/* max power limit */
/* 0 is normal, 2 background */
                host_abort,	/* 24 october 1984 */
                hostcount,	/* host mark that goes 1-9 to indicate action */
                lastindex,
                loopcounter,
                lostlock,	/* lock lost flag */
                maxtime,
		n_level,
                new,
                numvectors,
                rho,
                slewlimit;
long            
                codeoffset,	/* for fidshim */
                fs_np,		/* # points for fidshim */
		fs_dpf;

time_t2		timechk;

static int gainBumped = 0;

#if defined(INOVA) || defined(MERCURY)
static int failAsserted = 0;
#else
extern int failAsserted;
#endif

/* End of global.c */

#define PRTON   1
#define PRTOFF  0

static int simplexBest;
int controlMask;

int do_autoshim(char *comstr, short maxpwr, short maxgain)
{
				/* save acq. registers on stack */
   int             rstatus;	/* return status of auto-lock */
   int		   stat_before_autshm;

   spinrate = getSpinSet();

   maxtime = 0;			/* no time limit on shimming initially */
   rstatus = 0;			/* initialize return status to OK */
   g_maxpwr = maxpwr;

/* establish some pointers for other routines later on */
   init_shim();
#ifdef XXX
   init_dac();
#endif

   DPRINT(0, "Auto Shim\n");
   stat_before_autshm = get_acqstate();
   update_acqstate(ACQ_SHIMMING);
   getstatblock();		/* tell host that console is shimming now */

   lk2kcf();
   auto_x(comstr,(int)maxgain);
   lk2kcs();	/* reset lock filter to 2kHz slow */
   if (!host_abort && !failAsserted)
   {
      update_acqstate(stat_before_autshm);
      getstatblock();		/* tell host that shimming is complete now */
   }
   DPRINT(0, "Auto Shim Exits\n");
   return (rstatus);
}

auto_x(com,maxgain)
char *com;
int  maxgain;
{
   char           *pntr;
   int             temp;
   int             incr;

   gainBumped = 0;   /* initial spinner gain bump for this session */

   fid_shim = FALSE;		/* initialize fs flag */
   fid_init = TRUE;
   pntr = com;	/* front pointer */
   /*****************************************************/
   if (*pntr == QUICK_SHIM)
   {
      DPRINT(0,"Quick Shim Enabled AS CONTINUE!!\n");
      fox = 2;
      delayflag = TRUE;
      pntr++;
   }
   else
   {
     DPRINT(0,"Normal start up\n");
     set_defs();		
     delayflag = FALSE;
     fox = 0;
#ifdef DEBUG
     if (*pntr == DONE_SH)
       DPRINT(0,"Starting With a DONE CODE???\n");
#endif
   }
   /*****************************************************/
   while ((*pntr != DONE_SH) 
	  && ((controlMask & FAILURE) == 0)
	  && !host_abort && !failAsserted)
   {				/* while commands left to parse */

#ifdef DEBUG
      DPRINT1(0, "--------\nPARSE:0x%x:\n--------\n", *pntr);
#endif

      if ((temp = parse(pntr,&incr)) != 0)
      {
         DPRINT1(0,"call act with %d\n",temp);
	 act(temp, maxgain);
      }
      else
      {
         DPRINT2(0,"parse returned with %d and incr %d\n",temp,incr);
      }
      pntr += incr;
      DPRINT2(2,"new ptr is %d (%d)\n",*pntr, DONE_SH);
   }				/* end of while # 1 */
   errormessage();
   /*
    * when done set the gain back if it was bumped for 'rn' (spinning off)
    * and never returned to previous setting. Also restart spinning.
    */
   if (gainBumped == 1)
   {
      if (spinrate >= 5)
      {
         setspinnreg(spinrate, 0);  /* never bump the sample from autoshim */
                                    /* why is there no error action ???? TODO */
      }
      adjustGain();
      gainBumped = 0;
      Tdelay(100);
   }
}

errormessage()
{
   if ((controlMask & ABORT) != 0)
   {
      DPRINT(0, "aborted...\n");
   }
   else if (controlMask & DAC_RANGE)
   {
      DPRINT(0, "DAC Out-of-Range.\n");
   }
   else if ((controlMask & LOCK_UNDER) != 0)
   {
      DPRINT(0, "FAILED..Low Lock Level\n");
   }
   else if ((controlMask & LOCK_OVER) != 0)
   {
      DPRINT(0, "FAILED..High Lock Level\n");
   }
   else if ((controlMask & FAILURE) != 0)
   {
      DPRINT(0, "FAILED..\n");
   }
   return (TRUE);
}

/*
act means a command for operation has been parsed
when ',' or ';' or '\0' has been reached then action takes place
this allows for non-action commands to be parsed
only one action is allowed per command

parse returns
0 if no action else
1    shim
2    spinner bearing on
4    spinner bearing off
8    X set shim coil nyi
*/

parse(pp,inc)
char           *pp;
int            *inc;
{
   int  flag;
   long tmp_active;

   flag = 0;
   *inc = 0;
   switch (*pp++)		/* first command letter checked */
   {				/* each case moves the pointer  */
	 /* the correct amount to the next instr */

      case LOCK_SH:		/* shim on the lock signal */
	 fid_shim = FALSE;
	 DPRINT(0, "Lock Shim Selected\n");
	 break;
      case FID_SH:		/* shim on the fid */
#if defined(INOVA) || defined(MERCURY)
	 fid_shim = TRUE;
#else
	 fid_shim = FALSE;
#endif
         adjLkGainforShim = 0;
         {
            int fid_start = (int) *pp++;
            int fid_end = (int) *pp++;
            *inc += 2;
#if defined(INOVA) || defined(MERCURY)
            fid_limit(fid_start,fid_end);
#endif
	    DPRINT2(0, "Fid Shim Selected with limits %d and %d\n",
                           fid_start,fid_end);
         }
	 break;
      case SEL_COIL:		/* shim coils to shim and then do simplex */
         new = *pp++;
         tmp_active =  (long) ((*pp++ << 8) & 0xff00L);
         tmp_active |= (long) ((*pp++)      & 0x00ffL);
         tmp_active = (tmp_active << 16);
         tmp_active |= (long) ((*pp++ << 8) & 0xff00L);
         tmp_active |= (long) ((*pp++)      & 0x00ffL);
         set_active(tmp_active);
	 criteria = *pp++;	/* for method shim */
	 f_crit = *pp++;	/* for method shim */
         *inc += 7;
         DPRINT1(0,"Select coils 0x%lx and ", tmp_active);
         DPRINT2(0,"criteria %c and %c\n", criteria,f_crit);
	 flag |= 1;
	 break;
      case DELAY:
         delaytime =  (int) ((*pp++ << 8) & 0xff00);
         delaytime |= (int) ((*pp++)      & 0x00ff);
         *inc += 2;
	 delayflag = TRUE;	/* user has selected delay time */
	 DPRINT1(0, "delay time is %d ms\n", delaytime * 10);
	 break;
      case SPIN_ON:
	 flag |= 2;
	 DPRINT(0, "Spin on\n");
	 break;
      case SPIN_OFF:
	 flag |= 4;
	 DPRINT(0, "Spin off\n");
	 break;
      case MAX_TIME:		/* Max time for shimming */
         maxtime =  (int) ((*pp++ << 8) & 0xff00);
         maxtime |= (int) ((*pp++)      & 0x00ff);
         *inc += 2;
	 DPRINT1(0, "Maximum Shimming Time is %d sec\n", maxtime);
	 break;
      case QUICK_SHIM:		/* Quick Shimming */
	 fox = 2;
         adjLkGainforShim = 0;
	 DPRINT(0,"QUICK SHIM Enable integral to METHOD\n");
	 break;
      default: 
	 DPRINT1(0,"Got an unknown command :%c:\n",*(pp-1));
   }
   *inc += 1;
   return (flag);
}

set_defs()
{
   new = TRUE;
   slewlimit = 5;
   rho = 10;
   criteria = 'm';
   f_crit = 'm';
   host_abort = 0;
   controlMask = 40;	        /* kill error fields default to 40*/
                                /* starting default delay time */
   delaytime = (controlMask % 256) + 10;
#ifdef DEBUG
   DPRINT1(0, "control mask = %x T\n",controlMask);
   DPRINT1(0, "delay = %d T\n", delaytime);
#endif
}

/*
control mask fields

lock signal recovery rate	0-4  (values 0-15)
unused				5-7  (values 0-7 )
spectrometer id			8-10
debug on port one		11
					
error fields

user aborted			12
lock lost			13
lock over range			14
dac(s) out of range		15

integer best:  best is the present best value.

*/
act(flg, maxgain)
int             flg;
int             maxgain;
{
   int             lag,
                   dum;

 DPRINT2(0,"Call act() with flg = %d new= %d\n",flg,new);
   switch (flg)
   {
   case 1:
      if (new)
      {
	 DPRINT(0, "search not initiallized\n");
      }
      else			/* do simplex */
      {
         if (fid_shim && fid_init)
         {
             /* fill in fid shimming structure */
 	     DPRINT(0,"Call mk_fs_str()\n");
             mk_fs_str();
         }
         if (adjLkGainforShim)
         {
 	    DPRINT(0,"Adjust lock gain\n");
            adjustGain();
            adjLkGainforShim = 0;
         }
 	 DPRINT(0,"Call clock()\n");
	 secondClock(&timechk, 1);	/* start elapsed time */
 	 DPRINT1(0,"Call simplex() with maxgain = %d\n",maxgain);
	 simplexBest = simplex_hunt(maxgain);
 	 DPRINT1(0,"Done simplex() with maxgain = %d\n",maxgain);
	 /*--------------------------------
	 NEW
	 if we measured it ONCE, keep using it
	 ---------------------------------*/
	 if (!fid_shim) delayflag = TRUE;
      }
      break;
   case 2:			/* turn air bearing on and delay */
      /* UNDER TEST */
      if (spinrate >= 5)
      {
         setspinnreg(spinrate, 0);  /* never bump the sample from autoshim */
                                    /* why is there no error action ???? TODO */
      }
      if (gainBumped == 1)      /* will only ever be true if !fid_shim */
      {
         adjustGain();
         gainBumped = 0;
         Tdelay(100);
      }
      break;
   case 4:			/* turn air bearing off and delay */
      if (setspinnreg(0, 0) != 0)   /* never bump the sample from autoshim */
	controlMask &= ABORT;
      if (!fid_shim)
      {
         if (gainBumped == 0)   /* bump the gain only once */
         {
            adjustGain();
            gainBumped = 1;
            Tdelay(100);
         }
      }
      /* spinrate = 0; - spinrate is the set point!!! - don't erase */
      /* preliminary error handler */
#ifdef TODO
      spinner('Q', 0, 0, PRTOFF);	/* turn air bearing off and delay */
      DPRINT(0, "AIR BEARING OFF\n");
      Ldelay(&lag, 2000);
      while (((dum = spinstat('T', PRTOFF) & 0x90) != 0x10) && (!Tcheck(lag)));
      if (dum > 0x10)
	 controlMask &= ABORT;
#endif
      break;
   default:
      DPRINT(0, "unknown action or multiple action requested\n");
      break;
   }				/* end of switch */
}				/* end of act() */

mk_fs_str()
{         
   int            i;
   int            *scan_ptr;
   long           old_sum,
                  sum_data;
   struct a_args  *acq_args;
   struct lc      *fslc;
 
/*---------------------------------------------------------*/
/* find all of the parameters and the acode stream  */
/*---------------------------------------------------------*/
   fid_init = 0;
 
   i=0;
   /* parameters need by Getfid(), etc. were set in shandler,
      contained in datahandler.c */
   sum_data = Getfid();       /* allows for relaxation and scaling effects */
   DPRINT1(0,"\nUnscaled Data Table = %ld\n", sum_data);
   do
   {
     i++;
     old_sum = sum_data;
     sum_data = Getfid();       /* allows for relaxation and scaling effects */
     DPRINT1(0, "Unscaled Data Table = %ld\n", sum_data);
   }
   while ((old_sum - sum_data > sum_data / 64L)  && (i < 20));
   DPRINT1(0,"Preliminary setup readings: %d\n",i);
   fs_scl_fct = 0;              /* initialize counter */

   while (sum_data >= INTOVF)   /* if we overflow a pos int */
   {    
      sum_data >>= 1;           /* scale and increment */
      fs_scl_fct++;
   }
   fs_scl_fct += 3;             /* a factor of 8 in case fid improves */
   DPRINT1(0, "scaling factor = %d\n", fs_scl_fct);
   sum_data >>= 3;              /* scale sum also */
   DPRINT1(0, "scaled data table = %d\n", (int) sum_data);
}
      

/*
      27  25        20        15        10         5         0   (decmal)

       1 1 1 1 1 1 1 1 1 1 1 1
       B A 9 8 7 6 5 4 3 2 1 0 F E D C B A 9 8 7 6 5 4 3 2 1 0   COIL # (hex)
       ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
       Z X Z Y Y X Y X X X Y X - - - - - - - Z Z Z Z Z Z Z Z Z
       X Z X Z 3 3 Z 2 Y Z                   5 4 3 2 2 1 1 0 0
       Y 2 2 2       |                             c f c f c f
           |         Y
           Y	     2
           2

      |_______|_______|_______|_______|_______|_______|_______|

*/

set_crit(cc)			/* for shimi ( pases ints ) */
int             cc;
{
   char            tmp;

   switch (cc)
   {
   /*
   case BAD:
      tmp = 'b';
      break;
   THIS MODE IS DISABLED UNTIL ACQI NEEDS IT */
   case LOOSE:
      tmp = 'l';
      break;
   case MEDIUM:
      tmp = 'm';
      break;
   case TIGHT:
      tmp = 't';
      break;
   case EXCELLENT:
      tmp = 'e';
      break;
   default:
      tmp = 'l';
   }
   return (tmp);
}


#if defined(INOVA) || defined(MERCURY)
/*----------------------------------------------------------------------*/
/*  This routine can be used for deciding when to perform either        */
/*   autoshim or resetting DACs since the mask has the same meaning in  */
/*   both cases.							*/
/*									*/
/*  chkshim(mask,ct,sampchg):	 					*/
/*		Check Shim  or DAC Mask for when to do auto shimming  	*/
/*		or RESETTING the Shim DACs				*/
/*	        should be performed 					*/
/*	  Return 0 - if don't perform operation				*/
/*		 1 - to perform operation  				*/
/*----------------------------------------------------------------------*/

 chkshim(mask,ct,sampchg)
 int mask,*sampchg;
 long ct;
  {
    int doit;
    doit = FALSE;
    switch (mask)
      {
	case 1:
	case 2:
		if (ct == 0L)  /* If New Exp. or FID, Autoshim */
	         {
/*s*/             DPRINT(0,"Shim EXP or FID\n");
		   doit = TRUE;
	     	 }
		break;
	case 4: 
		/* if (lgostat == BSCODE)  /* If Done Block Size, Autoshim */
	         {
/*s*/             DPRINT(0,"Shim BS\n");
		   doit = TRUE;
	     	 }
		break;
	case 8: 
		if (*sampchg) 		/* if sample changed, Autoshim */
		 {
/*s*/             DPRINT(0,"Shim SAMP\n");
		   doit = TRUE;
		   *sampchg = 0;  /* only one autoshim in a go*/
                 }
		break;

	default: break;
      } /* end mask switch */

    return(doit);
  } /* end of chkmask */

#endif
