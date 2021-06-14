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
#include <stdio.h>
#include <sys/types.h>
#include "acodes.h"
#include "rfconst.h"
#include "acqparms2.h"
#include "lc_gem.h"
#include "shrexpinfo.h"
#include "dsp.h"
#include "abort.h"

typedef	struct _aprecord {
        int		preg;	/* this is not saved */
        short		*apcarray;
} aprecord;

extern	Acqparams	*Alc;  /* pointer to low core structure in Acode set */
extern	codeint	ilflagrt, ntrt, strt, tmprt;
extern	double	totaltime;
extern	int	acqtriggers;
extern	int	bgflag;
extern	int	curfifocount;
extern	int	curqui;
extern	int	grad_flag;
extern	int	hwlooping, hwloop_cnt, hwloop_ptr;
extern	int	nsc_ptr;
extern	int	newacq;
extern	int	noiseflag;

extern	aprecord	apc;

extern SHR_EXP_STRUCT ExpInfo;

/***************************************************************/
/*          Hardware Looping Code	                       */
/***************************************************************/

static double fifostarttime; /* loop time of hardware loop */
static double prevlooptime = -1.0; /* loop time of previous hardware loop */

/* time for word to be transfered from the pre-fifo to the looping fifo */
#define PF2FTIME 400.0e-9

/*-------------------------------------------------------------------
|   starthardloop()
|	rtindex is to v1-v14, number of times to cycle through loop
|	This start a hardware loop.
|	hwlooping flag is set.
|	# of fifo words in hardware loop is set.
+------------------------------------------------------------------*/
void starthardloop(rtindex)
int	rtindex;	/* index to real time variable in code (v1-v14) */
{
    if(bgflag)
	fprintf(stderr,"starthardloop(): v# index: %d \n",rtindex);
    if (hwlooping)	/* No nesting of hardware loops */
    {
	text_error("Missing endhardloop after previous starthardloop\n");
	psg_abort(0);
    }
    putcode(0);
    hwlooping = TRUE;			/* mark the start of a hardware loop */
    fifostarttime = totaltime;		/* must know time of loop */
    hwloop_ptr = apc.preg;		/* save location for use */
    hwloop_cnt = curfifocount;		/* current number of words in fifo */
    putcode(HWLOOP);			/* hardwareloop Acode */
    putcode(0);				/* will be number of words in loop */
    if (newacq) putcode(2);		/* inserted and removed */
    putcode(rtindex);			/* real time variable with # of loops */
}
/*-------------------------------------------------------------------
|
|	endhardloop()/1 
|	This ends a hardware loop.
|	hwlooping flag is unset.
|	# of fifo words in the hardwarre loop are calculated 
|	 1. setICM is inserted in nop, if this loop acquires data
|	     and no previous hardware loop did.
|	 2. # of fifo words in hardware loop is set.
|	 3. enable interrupt is nulled if only one hardware loop
|	    exists.
|	
|				Author Greg Brissey  7/10/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to calc acode offsets 
+------------------------------------------------------------------*/
void endhardloop()
{
int	fifowords;
double	timeofloop,time4load;

   if (bgflag)
      fprintf(stderr,"endhardloop(): \n");
   if (!hwlooping)
   {  text_error("Missing starthardloop \n");
      psg_abort(0);
   }
   hwlooping = FALSE;	/* not hardware looping any more, mark it */
   fifowords = curfifocount - hwloop_cnt;
   if (fifowords == 0)  /* No pulse elements in hardware loop. */
   {  text_error("Warning No pulse elements in Hardware loop.\n");
      apc.preg -= 3;	/* --- delete the hardware loop --- */
      if (newacq) apc.preg--;
   }
   else
   {
      char     mess[80];
      if (fifowords > 2048)
      {  sprintf(mess,"Hardware loop %d exceeds max (2048) fifo words.\n",
                fifowords);
         text_error(mess);
	 psg_abort(0);
      }
      apc.apcarray[hwloop_ptr + 1] = fifowords;

      timeofloop = totaltime - fifostarttime; /* total delays in loop */
      time4load = ( (double)fifowords) * PF2FTIME; /* time to load loop */
      if ( (bgflag) && (prevlooptime != -1.0) )
         fprintf(stderr,
           "%8.1f us for loop to empty, %8.1f us to load next %d word loop.\n",
           prevlooptime * 1.0e6,time4load * 1.0e6,fifowords);
      if ( (prevlooptime != -1.0) && (prevlooptime <= time4load) )
         fprintf(stdout,
		 "Warning: Time between hardware loops maybe insufficient.\n");
      prevlooptime = timeofloop;

      if (newacq)
         putcode(EHWLOOP);
   }
}

void turnoff_swdsp()
{
    if (ix == 1)
    {
       text_error("Inline DSP turned off\n");
       putCmd("off('oversamp')\n");
       putCmd("off('oversamp','processed')\n");
    }
    dsp_params.flags = 0;
    dsp_params.rt_oversamp = 1;
    dsp_params.rt_extrapts = 0;
    dsp_params.il_oversamp = 1;
    dsp_params.il_extrapts = 0;
    dsp_params.rt_downsamp = 1;

    ExpInfo.DspOversamp = 0;
    ExpInfo.DspOsCoef = 0;
    ExpInfo.DspSw = 0.0;
    ExpInfo.DspFb = 0.0;
    ExpInfo.DspOsskippts = 0;
    ExpInfo.DspOslsfrq = 0.0;
    ExpInfo.DspFiltFile[0] = '\0';
}

/*-------------------------------------------------------------------
|
|	acquire(datapts,dwell) 
|	If acquire is in a hardware loop, the number of data pts 
|	w/ # timerwords needed for dwell are checked to be sure 
|	the number of fifo words needed collect the data is not 
|	too big for a hardware loop.
|	Then the curfifocount is updated accordingly.
|       If this is the first acquire set the input IC mode.
|	If the acquire in not in a hardware loop, set the input card IC
|	mode if this is the first acquire and not a noise acquire.
|	update the curfifocount by 32, there 32 fifo words in an
|	acquire loop.
+------------------------------------------------------------------*/
#define FIFOLOOPSIZE	2048
#define	MAXLOOPLENGTH	32
#define	TIME1MS		0x4000

void acquire(double datapts,double dwell)
{
int	ntwords;		/* number of timer words */
int	pairs;			/* number of ctc's */
int	fifowrds;		/* # fifo words required for acquire */
int	implicitlpsize = 0;	/* the implicit acquisition fifo looping size */
int	nloops = 0;
int	explicit_acquire_flag;
int     twords;

   if (bgflag)
      fprintf(stderr,"acquire(): data pts: %f, dwell: %12.10f \n",datapts,dwell);
   explicit_acquire_flag =
	(!noiseflag && ((int) (datapts + 0.5) != (int) (np+0.5)));
   if (dsp_params.il_oversamp > 1)	/* In-line DSP */
   {
      if ( noiseflag )
         ;  /* Do nothing special */
      else if ( explicit_acquire_flag )
         turnoff_swdsp();
      else
      {
         datapts *= (double) dsp_params.il_oversamp;
         datapts += (double) dsp_params.il_extrapts;
         dwell   /= (double) dsp_params.il_oversamp;
      }
      if (bgflag)
         fprintf(stdout, "after in-line DSP: datapts: %f\n", datapts );
   }

   if (dwell < 2.0e-7)
   {
      text_error("acquire(): dwell time must be larger than 2e-7\n");
      psg_abort(0);
   }
   if (datapts < 1.5)
   {
      text_error("must acquire at least two (one pair) data points\n");
      psg_abort(0);
   }

   dwell *= 1e3;			/* in msec */
   ntwords = (dwell > 1.6) ? 2 : 1;
   pairs = (int) (datapts + 0.005) / 2;
   acqtriggers++;			/* increment number acquisitions done */

   rcvron();				/* turn rcvr on (also for explicit) */

   if (hwlooping)			/* an acquire in a hardware loop ? */
   {
      if (pairs > FIFOLOOPSIZE /ntwords)
      {
         text_error("Too many data points to acquire in hardware loop.\n");
         psg_abort(0);
      }
      else
      {  /* increment fifocount for the CTCs that will be generated */
         curfifocount += ntwords * pairs;
      }
      if (acqtriggers == 1)
      {
         apc.apcarray[hwloop_ptr - 1] = SETICM;	/* change NOOP to SETICM */
      }
   }
   else	/* an acquire outside a hardware loop */
   {
      if ( (acqtriggers == 1) && (! noiseflag))
      {
         if (newacq)
            putcode(SETICM);		/* Acode to set up input card mode */
      }
      implicitlpsize = MAXLOOPLENGTH / ntwords;
      fifowrds = (pairs * ntwords);
      nloops =  pairs / implicitlpsize;  /* pairs/loop_size=lp cnt */
      curfifocount += implicitlpsize; /* acqop generates implicit word lp */
      curfifocount += fifowrds - (nloops * implicitlpsize);
   }

   /* --- type of acquire: data or noise --- */
   if (!noiseflag)
      putcode(ACQXX);			/* acqop Acode */
   else
      putcode(NOISE);			/* noiseop Acode */

   if (newacq)
   {  if (hwlooping)
         putcode(0);		/* tell apio acqop not to use hardware loop */
      else
         putcode(2);		/* tell apio acqop to use hardware loop */
      putcode( ((int) datapts) >> 16 );   /* top word # of cmplx pts to take */
      putcode( ((int) datapts) & 0xFFFF); /* bot word # of cmplx pts to take */
   }
   else
   {
      /* make sure number of ctc > np and multiple of loopsize */
      if (pairs > nloops*implicitlpsize)
      {  nloops++;
         pairs = nloops * implicitlpsize;
         Alc->np = pairs*2;
      }

      putcode(curqui);
      putcode(implicitlpsize);
      putcode(nloops-1);			/* integer fifoloopcount */
   }


   twords = 0;
   if (dwell > 1.6)
   {  dwell -= 2e-4;
      putcode((int)(dwell-1.0) | TIME1MS);
      twords++;
      dwell = dwell - (double) ((int)(dwell));
   }
   dwell *= 1e4;                  /* in 100 ns */
   if (dwell > 1.5)
   {  dwell -= 2;
      putcode((int) (dwell + 0.5) );
      twords++;
   }
   if ( (ntwords == 1) || (twords == 1) )
   {
      putcode(0);
   }

/* because we multiplied dwell to 0.1 usec, we now need to *1e-7 */
    totaltime += (datapts * dwell * 1e-7/ 2.0);
 
    if (bgflag) 
        fprintf(stderr,"acquire(): time = %lf totaltime = %lf\n",
	    datapts*dwell / 2.0,totaltime);
}

/*-------------------------------------------------------------------
|
|	donoisecalc() 
|	generate Acodes to obtain data of noise, inorder to calculate
|	 the noise of the receiver channels.
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
void donoisecalc()
{
   noiseflag = TRUE;		/* indicate to acquire() this is noise acq */

   rcvron();			/* turn receiver on */
   acqtriggers = 0;		/* No acquisitions up to this point */
   hwlooping = FALSE;		/* Not in a hardware loop */
   acquire(256.0,1.0/sw);	/* acquire 256 pts (128 cmplx) */

   noiseflag = FALSE;	/* remove noise acquire mark */
}
/*-------------------------------------------------------------------
|
|	test4acquire() 
|	check to see if data has been acquired yet.
|	if not, then do an implicit acuire.
|	else do not.
+------------------------------------------------------------------*/
test4acquire()
{
   if (acqtriggers == 0)		/* No data acquisition yet? */
   {
/*      if (nf > 1.0)
 *      {
 *         text_error("Number of FIDs (nf) Not Equal to One\n");
 *	 psg_abort(0);
 *      }
 */
      rcvron();				/* turn receiver on */
      setphase90(TODEV,zero);		/* tx phase to zero */
      delay(alfa+1.0/(2.0*fb));		/* alpha delay */
      acquire(np,1.0/sw);		/* acquire data */
   }

   if (grad_flag == TRUE) 
   {
      zero_all_gradients();
   }
   if (newacq)
   {
      putcode(HKEEP);			/* do house keeping */
      if ( getIlFlag() )
      {
         ifzero(ilflagrt);
    	    putcode(IFZFUNC);		/* branch to NSC if ct<nt */
	    putcode((codeint)ct);
	    putcode((codeint)ntrt);
    	    putcode(nsc_ptr);		/* pointer to nsc */
	 elsenz(ilflagrt);
     	    add(strt,one,tmprt);	
   	    putcode(IFZFUNC);		/* branch to NSC if ct<strt+1 */
	    putcode((codeint)ct);
	    putcode((codeint)tmprt);
    	    putcode(nsc_ptr);		/* pointer to nsc */
	    modn(ct, bsval, tmprt);
    	    putcode(IFZFUNC);		/* branch to NSC if ct%bs */
	    putcode((codeint)zero);
	    putcode((codeint)tmprt);
    	    putcode(nsc_ptr);		/* pointer to nsc */
	 endif(ilflagrt);
      }
      else
      {
    	   putcode(IFZFUNC);		/* branch to NSC if ct<nt */
	   putcode((codeint)ct);
	   putcode((codeint)ntrt);
    	   putcode(nsc_ptr);		/* pointer to nsc */
      }
   }
   else
   {
      putcode(STFIFO);		/* start fifo if it already hasn't */
      putcode(HKEEP);		/* do house keeping */
      putcode(BRANCH);		/* branch back to NSC */
      putcode(nsc_ptr);		/* pointer to nsc */
   }
}

/*-------------------------------------------------------------------
|
|	notinhwloop() 
|	set the invalid hardware loop flag 
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
notinhwloop(name)
char *name;		/* name of calling routine */
{
   char mess[80];
   if (bgflag)
       fprintf(stderr,"notinhwloop('%s'):\n",name);
   if (hwlooping)
   {
       sprintf(mess," '%s' is not valid between starthardloop() and endhardloop()\n",name);
       text_error(mess);
       psg_abort(0);		/* abort process */
   }
}
