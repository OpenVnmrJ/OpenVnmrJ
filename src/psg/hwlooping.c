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
#include <math.h>
#include "oopc.h"
#include "apdelay.h"
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "shrexpinfo.h"
#include "cps.h"
#include "abort.h"
#include "pvars.h"
#include "dsp.h"
#include "macros.h"

extern int  ap_interface;
extern int  acqiflag;
extern int  rcvroff_flag;	/* global receiver flag */
extern int  rcvr_hs_bit;
extern int  OBSch;
extern int  newacq;
extern int  bgflag;			/* debug flag */
extern double dsposskipdelay;
/*extern int  dsp_info[];*/
int noise_dwell;

extern int grad_flag;	/* external "gradients used" flag in gradient.c */
extern codeint adccntrl, ilflagrt, tmprt, strt;

static double fifostarttime; /* loop time of hardware loop */
static double prevlooptime = -1.0; /* loop time of previous hardware loop */
static int acqinhwloop = FALSE;    /* used for inova */
static int explicitacq = FALSE;
static codeint disovld_ptr;
static int hwloopcnt4time;
static int go_dqd=0;

void find_dqdfrq(double *truefrq, double *dqdfrq);

extern SHR_EXP_STRUCT ExpInfo;

/* time for fifo word to be transfered from the pre-fifo to the looping fifo */
#define PF2FTIME 400.0e-9

/*-------------------------------------------------------------------
|
|	starthardloop()/1 
|	index is to v1-v14, number of times to cycle through loop
|	This start a hardware loop.
|	hwlooping flag is set.
|	curfifocount is saved so that the # of fifo words in the
|	 hardwarre loop can be calcualted later in 'endhardloop'.
|	pointer to the starthardloop Acode is save so that the
|	 values can update later by 'endhardloop'
|	 1. setICM is inserted in nop, if this loop acquires data
|	     and no previous hardware loop did.
|	 2. # of fifo words in hardware loop is set.
|	 3. enable interrupt is nulled if only one hardware loop
|	    exists.
|	
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
void starthardloop(codeint rtindex)
{
    setSSHAdisable();
    if(bgflag)
	fprintf(stderr,"starthardloop(): v# index: %d \n",rtindex);
    if (hwlooping)	/* No nesting of hardware loops */
    {
	text_error("Missing endhardloop after previous starthardloop\n");
	psg_abort(0);
    }
    hwlooping = TRUE;	/* mark the start of a hardware loop */
    starthwfifocnt = curfifocount; /* must know # words in fifo */
    fifostarttime = totaltime; /* must know time of loop */
    hwloopelements = 0;  /* reset pulse elements in hardware loop to zero */
    putcode(0);		/* nop that might be change to a setSTM acode */
    hwloop_ptr = (codeint) (Codeptr - Aacode); /* save location for use */
    putcode(HWLOOP);	/* hardwareloop Acode */
    putcode(0);		/* will be then number of fifo words in loop */
    putcode(2);		/* default to enable looping interrupt in acq.  */
			/* for multiple hardware loops */
    putcode(rtindex);   /* real time variable containing # of cycles of loop */

    /* Update time for loop */
    hwloopcnt4time = get_acqvar(rtindex); /* get count for this loop	*/
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
    codeint fifowords;
    double timeofloop,time4load;

    if(bgflag)
	fprintf(stderr,"endhardloop(): \n");
    if (!hwlooping)
    {
	text_error("Missing starthardloop \n");
	psg_abort(0);
    }
    hwlooping = FALSE;	/* not hardware looping any more, mark it */
    fifowords = curfifocount - starthwfifocnt;
    if (fifowords == 0)  /* No pulse elements in hardware loop. */
    {
	codeint *tmpptr;

	text_error("Warning No pulse elements in Hardware loop.\n");
	/* --- NO_OP out the hardware loop --- */
	/* tmpptr = codestadr + hwloop_ptr; */ /* get address into codes */
	tmpptr = Aacode + hwloop_ptr; /* get address into codes */
	*tmpptr++ = NO_OP;	/* NO_OP the HWLOOP Acode */
	*tmpptr++ = NO_OP;	/* NO_OP out hwloop variables */
	*tmpptr++ = NO_OP;	/* NO_OP out hwloop variables */
	*tmpptr++ = NO_OP;	/* NO_OP out hwloop variables */
    }
    else
    {
	codeint *tmpptr;
        char     mess[80];

	if (fifowords > fifolpsize)
	{
            sprintf(mess,
	      "Hardware loop exceeds max fifo words. %d > %d Max.\n",
                fifowords,fifolpsize);
            text_error(mess);
	    psg_abort(0);
	}
	/* tmpptr = codestadr + hwloop_ptr + 1; */
	tmpptr = Aacode + hwloop_ptr + 1;
	*tmpptr = fifowords;

        timeofloop = totaltime - fifostarttime; /* total delays in loop */
        time4load = ( (double)fifowords) * PF2FTIME; /* time to load loop */
        if ( (bgflag) && (prevlooptime != -1.0) )
           fprintf(stderr,
           "%8.1lf usec for loop to empty, %8.1lf usec to load next %d word loop.\n",
           prevlooptime * 1.0e6,time4load * 1.0e6,fifowords);
        if ( (prevlooptime != -1.0) && (prevlooptime <= time4load) )
           fprintf(stdout,
           "Warning: Time between hardware loops maybe insufficient.\n");
        prevlooptime = timeofloop;

	/* inova needs end hardware loop code.  It checks for number 	*/
	/* of fifowords in hw loop down in the console.			*/
	if (newacq)
	{
	   putcode(EHWLOOP);
	   if (acqinhwloop == TRUE)
	   {
		putcode(DISABLEOVRFLOW);
		putcode(adccntrl);
 	   }
	}

	acqinhwloop = FALSE;	/* Always set to FALSE */

	/* Add time to totaltime.  The time to add is only for the 	*/
	/* loops greater than one.  The first loop has already been 	*/
	/* added to the totaltime. 					*/
	if (hwloopcnt4time > 1)		
	   totaltime += (timeofloop * (hwloopcnt4time - 1));
    }
    setSSHAreenable();
}

void init_dqdfrq()
{
    double truefrq=1.0, dqdfrq=0.0;
    find_dqdfrq(&truefrq,&dqdfrq);
    if (fabs(dqdfrq) > 0.1)
      init_spare_offset(truefrq+dqdfrq, OBSch);
}

void find_dqdfrq(double *truefrq, double *dqdfrq)
{
  double dqd_overrange=0;
  int itmp, itmp2, itmp3, jtmp2, jtmp3, warnflag=0;
  char dqd_latch[MAXSTR];

  if ((dqd[0] == 'y') && !noiseacquire && !hwlooping &&
       ((dsp_params.il_oversamp > 1) || (dsp_params.rt_oversamp > 1)) )
  {
    if (*truefrq > 0.5) warnflag=1;
/*    *truefrq = sfrq; */
    *truefrq = sfrq + 10.5;
    *truefrq *= 1.0e6;
    *dqdfrq = ExpInfo.DspOslsfrq;
    P_getreal(GLOBAL, "overrange", &dqd_overrange, 1);
    P_getstring(GLOBAL, "latch", dqd_latch, 1, MAXSTR);
    if ((dqd_overrange > 0.1) && (dqd_latch[0] == 'y'))
    {
      if ((dqd_overrange > 9e4) && (fabs(*dqdfrq) > dqd_overrange) && (warnflag == 1)) /* overrange=1e5 */
      {
        text_error("WARNING: oslsfrq too large.\n");
        fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
      }
      else /* overrange = 1e4 */
      {
       if (fabs(*dqdfrq) > dqd_overrange)
       {
        itmp  = ((int)(*truefrq / 1e5)) % 10; /* could replace 1e4,1e4 with overrange */
        itmp2 = ((int)((*truefrq + *dqdfrq) / 1e5)) % 10;
        jtmp2 = ((int)((*truefrq + *dqdfrq) / 1e4)) % 10;
        if (itmp != itmp2)
        {
	  if ( ((*dqdfrq < 0) && (jtmp2 != 9)) || ((*dqdfrq > 0) && (jtmp2 != 0)) )
	  {
            itmp3 = ((int)((*truefrq - *dqdfrq) / 1e5)) % 10;
	    jtmp3 = ((int)((*truefrq - *dqdfrq) / 1e4)) % 10;
	    if (itmp != itmp3)
	    {
	      if ( ((*dqdfrq < 0) && (jtmp3 != 9)) || ((*dqdfrq > 0) && (jtmp3 != 0)) )
	      {
		if (warnflag == 1)
		{
	          text_error("WARNING: oslsfrq too large.\n");
	          fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
		}
	      }
	      else
	      {
                if (go_dqd == 0)
                {
	          *dqdfrq = -(*dqdfrq);
                  ExpInfo.DspOslsfrq = *dqdfrq;
                  go_dqd = 1;
                }
                else if (warnflag == 1)
                {
	          text_error("WARNING: oslsfrq too large.\n");
	          fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
                }
	      }
	    }
	    else
	    {
              if (go_dqd == 0)
              {
	        *dqdfrq = -(*dqdfrq);
                ExpInfo.DspOslsfrq = *dqdfrq;
                go_dqd = 1;
              }
              else if (warnflag == 1)
              {
	        text_error("WARNING: oslsfrq too large.\n");
	        fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
              }
	    }
	  }
        }
       }
      }
    }
    else
    {
/*
      itmp = ((int)(*truefrq / 1e4)) % 10;
      if (go_dqd == 0)
      {
        if (itmp < 5)
        {
          if (*dqdfrq < 0) *dqdfrq = -(*dqdfrq);
        }
        else
        {
          if (*dqdfrq > 0) *dqdfrq = -(*dqdfrq);
        }
        ExpInfo.DspOslsfrq = *dqdfrq;
        go_dqd = 1;
      }

      itmp  = ((int)(*truefrq / 1e5)) % 10;
      itmp2 = ((int)((*truefrq + *dqdfrq) / 1e5)) % 10;
      if ((itmp != itmp2) && (warnflag == 1))
      {
	text_error("WARNING: oslsfrq too large.\n");
	fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
      }
*/
      itmp  = ((int)(*truefrq / 1e5)) % 10;
      itmp2 = ((int)((*truefrq + *dqdfrq) / 1e5)) % 10;
      if (itmp != itmp2)
      {
        itmp3 = ((int)((*truefrq - *dqdfrq) / 1e5)) % 10;
	if (itmp != itmp3)
	{
	  if (warnflag == 1)
	  {
	    text_error("WARNING: oslsfrq too large.\n");
	    fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
	  }
	}
	else
	{
          if (go_dqd == 0)
          {
	    *dqdfrq = -(*dqdfrq);
            ExpInfo.DspOslsfrq = *dqdfrq;
            go_dqd = 1;
          }
          else if (warnflag == 1)
          {
	    text_error("WARNING: oslsfrq too large.\n");
	    fprintf(stderr,"acquire(): oslsfrq overrange warning, cannot signal average\n  sfrq %12.1f  oslsfrq %12.1f  totfrq %12.1f\n", *truefrq - 10.5e6, *dqdfrq, *truefrq + *dqdfrq - 10.5e6);
          }
	}
      }
    }

    *truefrq = tof; /* absolutely need this!!! */
  }
  else
  {
    *truefrq = 0.0;
    *dqdfrq  = 0.0;
  }
}

/*-------------------------------------------------------------------
|
|	acquire(datapts,dwell) 
|	acquire data.
|	If acquire is in a hardware loop number of data pts w/ # timer
|	 words needed for dwell arechecked tobe sure the number of fifo 
|	 words needed collect the data is not too big for a hardware loop.
|	 Then the curfifocount is updated accordingly.
|        If this is the first acquire set the input IC mode.
|	If the acquire in not in a hardware loop, set the input card IC
|	 mode if this is the frist acquire nad not a noise acquire.
|	 update the curfifocount by 32, there 32 fifo words in an
|	 acquire loop.
|	Generate the proper Acodes for the acquire..
|	set the valid hardware loop flag 
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
void acquire(double datapts, double dwell)
{
    int ntwords;		/* number of timer words */
    int tword1;			/* timer word one */
    int tword2;			/* timer word two */
    int nptop;			/* top word of datapts */
    int npbot;			/* bot word of datapts */
    int pairs;			/* number of ctc's */
    int fifowrds;		/* # fifo words required for acquire */
    int implicitlpsize;		/* the implicit acquisition fifo looping size */
    int nloops;
    int explicit_acquire_flag;
    double save_dwell = 0.0;
    double truefrq=0.0, dqdfrq=0.0;

    if (bgflag)
	fprintf(stderr,"acquire(): data pts: %lf, dwell: %le \n",
		datapts,dwell);
    explicit_acquire_flag =
	(!noiseacquire && ((int) (datapts + 0.5) != (int) (np+0.5)));
    if (dsp_params.il_oversamp > 1) /* In-line DSP */
    {
       if ( noiseacquire )
          ;  /* Do nothing special */
       else if ( explicit_acquire_flag )
          turnoff_swdsp();
       else
       {
          /*datapts *= (double) dsp_info[1];*/
          /*datapts += (double) dsp_info[3];*/
          /*dwell   /= (double) dsp_info[1];*/
          datapts *= (double) dsp_params.il_oversamp;
          datapts += (double) dsp_params.il_extrapts;
          dwell   /= (double) dsp_params.il_oversamp;
       }
       if (bgflag)
         fprintf(stderr, "after in-line DSP: datapts: %lf\n", datapts );
    }

/*  if (dsp_info[0] >= 1)  Real time DSP */
    if (dsp_params.rt_oversamp > 1) /* Real time DSP */
    {
       if ( noiseacquire && ((int) (datapts + 0.5) != 256) )
          turnoff_hwdsp();
       else if ( explicit_acquire_flag )
          turnoff_hwdsp();
       else
       {
          /*datapts *= (double) dsp_info[1];*/
          datapts *= (double) dsp_params.rt_oversamp;
          /*datapts += (double) dsp_info[3];*/
          datapts += (double) dsp_params.rt_extrapts;
          save_dwell = dwell;
          /*dwell   /= (double) dsp_info[1];*/
          dwell   /= (double) dsp_params.rt_oversamp;
       }
    }
    if (!explicit_acquire_flag)
        find_dqdfrq(&truefrq, &dqdfrq);

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
    timerwords(dwell,&tword1,&tword2); /* obtain timer words for dwell */
    ntwords = (tword2 == 0) ? 1 : 2;
    pairs = (int) (datapts + 0.005) / 2;
    acqtriggers++;	/* increment number acquisitions performed */

    if (ap_interface < 4)
       HSgate(rcvr_hs_bit,FALSE);
    else
    {
        if (newacq)
	   HSgate(INOVA_RCVRGATE,FALSE);	/* Turn receiver On */
	else
           SetRFChanAttr(RF_Channel[OBSch], SET_RCVRGATE, ON, 0);
    }
    if (hwlooping)	/* an acquire in a hardware loop ? */
    {
	acqinhwloop = TRUE;
	if ( ((ntwords == 1) && (pairs > fifolpsize)) ||	
	     ((ntwords == 2) && (pairs > (fifolpsize/2))) )
	{
	    text_error("Too many data points to acquire in hardware loop.\n");
	    psg_abort(0);
	}
	else
	{	/* increment fifocount for the CTCs that will be generated */
	    curfifocount += ntwords * pairs;
	}
	if (acqtriggers == 1)
        {
            codeint *ptr;

            ptr = Aacode + hwloop_ptr - 1;
            *ptr = SETICM;
        }
    }
    else	/* an acquire outside a hardware loop */
    {
/*        if (fabs(dqdfrq) > 0.1)
	    obsoffset(truefrq+dqdfrq);
	    set_spare_freq(OBSch); 
*/
	if ( (acqtriggers == 1) && (! noiseacquire))
	{
	    putcode(SETICM);		/* Acode to set up input card mode */
	}
	if (fifolpsize < 64)
	    curfifocount += 32;		/* acquire generates 32 fifo word loop  */
	else
	{
	    implicitlpsize = fifolpsize / 8;
	    fifowrds = (pairs * ntwords);
	    nloops =  fifowrds / implicitlpsize;  /* fifowords / loop_size = lp cnt */
	    curfifocount += implicitlpsize; /* acquire generates implicit word lp */
	    curfifocount += fifowrds - (nloops * implicitlpsize);
	}
    }

    /* --- type of acquire: data or noise --- */
    if (!noiseacquire)
	putcode(EXACQT);		/* acqop Acode */
    else
	putcode(NOISE);			/* noiseop Acode */

/*
 *  Remove output board differences
 *  new OB don't output HSlines, gate() does it
 *
 *    if (fifolpsize < 512)
 *       putcode((codeint) HSlines);
 *
 */

    multhwlp_ptr = (codeint) (Codeptr - Aacode);/*save location for use */
    if (hwlooping)
	putcode(0);		/* tell apio acqop not to use hardware loop */
    else
	putcode(2);		/* tell apio acqop to use hardware loop */

    convertdbl(datapts,&nptop,&npbot);
    putcode((codeint) nptop);		/* top word # of cmplx pts to take */
    putcode((codeint) npbot);		/* bot word # of cmplx pts to take */
    if (newacq)
    {
        putLongCode(tword1);    /* timerword one */
    }
    else
    {
    putcode((codeint) tword1);		/* first timer word */
    putcode((codeint) tword2);		/* second timer word */
    }

    /* add acquisition time to total time */
    checkpowerlevels(datapts * dwell / 2.0);
    if (newacq && (save_dwell > 0.0) )
    {
       /* add an extra dwell period for DSP hardware */
       if (noiseacquire)
       {
          timerwords(save_dwell,&tword1,&tword2);
          noise_dwell = tword1;
       }
       else
       {
          delay(save_dwell);
       }   
    }
    totaltime += (datapts * dwell / 2.0);
 
    /* turn receiver off if receiver off flag is enabled */
    if (newacq)
    {
	if (rcvroff_flag)
	   HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
    }

    /* --- disable ADC & Receiver OverLoad --- */
    if (newacq)
    {
      if ((acqinhwloop == FALSE) && (!noiseacquire))
      {
	   explicitacq = TRUE;
    	   disovld_ptr = (codeint) (Codeptr - Aacode); /* save location */
    	   putcode(0);	/* nop to change to a DISABLEOVRFLOW acode */
    	   putcode(0);	/* nop to change to a DISABLEOVRFLOW acode */
      }

    }
    if (fabs(dqdfrq) > 0.1) 
/*	set_std_freq(OBSch); */
	obsoffset(truefrq);
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
    codeint *tmpptr;

    noiseacquire = TRUE;	/* indicate to acquire() this is noise acq */
    if (ap_interface < 4)
      HSgate(rcvr_hs_bit,FALSE);		/* receiver On */
    else
       SetRFChanAttr(RF_Channel[OBSch], SET_RCVRGATE, ON, 0);
    acqtriggers = 0;		/* No acquisitions up to this point */
    hwlooping = FALSE;		/* Not in a hardware loop */
    acquire(256.0,1.0/sw);	/* acquire 256 pts (128 cmplx) */
    tmpptr = Aacode + multhwlp_ptr; /* get address into codes multhwlp flag*/
    *tmpptr = 1;		/* what was set in acquire reset to non-multi */
    noiseacquire = FALSE;	/* remove noise acquire mark */
}
/*-------------------------------------------------------------------
|
|	test4acquire() 
|	check too see if data has been acquired yet.
|	if it has not then do an implicit acuire.
|	else do not.
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
void test4acquire()
{
    int	    i;
    int	    chan;
    int     MINch;
    double  acqdelay;	/* delay time between receiver On an data acquired*/
    codeint *tmpptr;	/* temp pointer into codes */
    extern void prg_dec_off();
    double truefrq=0.0, dqdfrq=0.0;
    char osskip[4];

    if (bgflag)
	fprintf(stderr,"test4acquire(): acqtriggers = %d \n",acqtriggers);
    if (acqtriggers == 0)	/* No data acquisition Yet? */
    {
	if (nf > 1.0)
	{
	    text_error("Number of FIDs (nf) Not Equal to One\n");
	    psg_abort(0);
	}
        if (ap_interface < 4)
	   HSgate(rcvr_hs_bit,FALSE);	/* turn receiver On */
	else
           SetRFChanAttr(RF_Channel[OBSch], SET_RCVRGATE, ON, 0);

	if (newacq)
        {
           /* execute osskip delay if osskip parameter set */
           if ((P_getstring(GLOBAL,"qcomp",osskip,1,2)) == 0)
           {
             if (osskip[0] == 'y')
             {
               /* fprintf(stderr,"hwlooping:test4acquire(): executing dsposskipdelay= %g\n", dsposskipdelay); */
               if (dsposskipdelay >= 0.0) G_Delay(DELAY_TIME, dsposskipdelay, 0);
             }
           }

           HSgate(INOVA_RCVRGATE,FALSE);        /* turn receiver On */
        }

	/* txphase(zero);	*/	/* set xmitter phase to zero */
	/* decphase(zero);	*/	/* set decoupler phase to zero */
	/* acqdelay = alfa + (1.0 / (beta * fb) ); */

	for (i = 1; i <= NUMch; i++)  /* zero HS phaseshifts */
	   SetRFChanAttr(RF_Channel[i], SET_RTPHASE90, zero, 0);

	if ((!noiseacquire) && (dsp_params.il_oversamp > 1))
	   find_dqdfrq(&truefrq, &dqdfrq);
	if (fabs(dqdfrq) > 0.1)
	   set_spare_freq(OBSch); 
/*	   obsoffset(truefrq+dqdfrq); */

	acqdelay = alfa + (1.0 / (beta * fb) );
	if (acqdelay > ACQUIRE_START_DELAY)
	   acqdelay = acqdelay - ACQUIRE_START_DELAY;
	if ((fabs(dqdfrq) > 0.1) && (acqdelay > 1.7e-6)) /* more like 40us?? */
	    acqdelay = acqdelay - 1.7e-6;

        if ((acqdelay < 0.0) && (ix == 1))
	   text_error("Acquisition filter delay (fb, alfa) is negative (%f).\n",
             acqdelay);
        else
	   G_Delay(DELAY_TIME,acqdelay,0);	/* alfa delay */
	acquire(np,1.0/sw);	/* acquire data */

	MINch = (ap_interface < 4) ? DODEV : TODEV;
	for (chan = MINch; chan <= NUMch; chan++)
	{
           if ( is_y(rfwg[chan-1]) )
           {
	      if ( (ModInfo[chan].MI_dm[0] == 'n') ||
                   ((ModInfo[chan].MI_dm[0] == 'y') &&
		    (ModInfo[chan].MI_dmm[0] != 'p')) )
              {
                 prg_dec_off(2, chan);
              }
           }
        }

	tmpptr = Aacode + multhwlp_ptr; /* get address into codes */
	*tmpptr = 1;		/* impliicit acquisition */
    }
    if (newacq)
    {
	if (explicitacq)
	{
              codeint *ptr;
	      /* update last acquire with disable overload */;
    	      ptr = Aacode + disovld_ptr;
	      *ptr++ = DISABLEOVRFLOW;
      	      *ptr = adccntrl;
	}
	/* Always set to FALSE for the next array element */
	explicitacq = FALSE;
    }

    if (grad_flag == TRUE) 
    {
      zero_all_gradients();
    }
    if (newacq)
    {
      gatedecoupler(A,15.0e-6);	/* init to status A conditions */
      statusindx = A;
    }
    putcode(STFIFO);	/* start fifo if it already hasn't */
    putcode(HKEEP);		/* do house keeping */
    if (newacq)
    {
      if ( getIlFlag() )
      {
	ifzero(ilflagrt);
    	   putcode(IFZFUNC);	/* brach to start of scan (NSC) if ct<nt */
	   putcode((codeint)ct);
	   putcode((codeint)ntrt);
    	   putcode(nsc_ptr);	/* pointer to nsc */
	elsenz(ilflagrt);
     	   add(strt,one,tmprt);	
   	   putcode(IFZFUNC);	/* brach to start of scan (NSC) if ct<strt+1 */
	   putcode((codeint)ct);
	   putcode((codeint)tmprt);
    	   putcode(nsc_ptr);	/* pointer to nsc */
	   modn(ct, bsval, tmprt);
    	   putcode(IFZFUNC);	/* brach to start of scan (NSC) if ct%bs */
	   putcode((codeint)zero);
	   putcode((codeint)tmprt);
    	   putcode(nsc_ptr);	/* pointer to nsc */
	endif(ilflagrt);
      }
      else
      {
    	   putcode(IFZFUNC);	/* brach to start of scan (NSC) if ct<nt */
	   putcode((codeint)ct);
	   putcode((codeint)ntrt);
    	   putcode(nsc_ptr);	/* pointer to nsc */
      }
    }
    else
    {
    	putcode(BRANCH);	/* brach back to start of scan (NSC) */
    	putcode(nsc_ptr);	/* pointer to nsc */
    }
}
/*-------------------------------------------------------------------
|
|	okinhwloop() 
|	set the valid hardware loop flag 
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
void okinhwloop()
{
   if (hwlooping)
       hwloopelements++;
   if (bgflag)
       fprintf(stderr,"okinhwloop(): elements: %d\n",hwloopelements);
}

/*-------------------------------------------------------------------
|
|	notinhwloop() 
|	set the invalid hardware loop flag 
|				Author Greg Brissey  7/10/86
+------------------------------------------------------------------*/
void notinhwloop(char *name)
{
   char mess[80];
   if (bgflag)
       fprintf(stderr,"notinhwloop('%s'):\n",name);
   if (hwlooping)
   {
       sprintf(mess," '%s' is not valid in a Hardware Loop \n",name);
       text_error(mess);
       psg_abort(0);		/* abort process */
   }
}
