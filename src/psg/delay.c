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
|    6/19/89   Greg B.    1. removed use of Codeoffset & startofAcode, now use
|			     use Codeptr & Aacode.
|
+----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "oopc.h"
#include "acodes.h"
#include "acqparms.h"
#include "macros.h"
#include "rfconst.h"
#include "ssha.h"
#include "abort.h"
#include "delay.h"

extern int HSlines;
extern int curfifocount;
extern int acqiflag;
extern int bgflag;
extern int newacq;

#define OK 0
#define MINDELAY 0.195e-6
#define INOVAMINDELAY 0.0995e-6
#define SKIP_DELAY	0x8000	/* recognized by evnt1op() to skip this set */
				/* of acodes, needed for IPA, also used in  */
				/* acqi, fiddisplay.c			    */

struct delaystruct
{
   double          time;
   double          units;
   int             max;
   int             min;
   int             scale;
   char           *label;
};

/*-------------------------------------------------------------------
|
|	G_Delay(variable arg_list)/n
|
|	Form the timerwords needed to obtain the time delay (sec)
|
|				Author Frits Vosman  1/3/89
+------------------------------------------------------------------*/
void G_Delay(int firstkey, ...)
{
   struct delaystruct delays;
   va_list         ptr_to_args;
   int             counter = 0;
   int             keyword;
   int		   use_gtab;
   int             gtable = 0;

/* fill in the defaults */
   delays.time    = d1;
   delays.label  = "";
   delays.scale  = 1;
   delays.max    = 60;
   delays.min    = 0;
   delays.units  = 1;

   use_gtab=FALSE;

/* initialize for variable arguments */
   va_start(ptr_to_args, firstkey);
   keyword = firstkey;
/* now check if the user want it his own way */
   while (keyword && ok)
   {
      switch (keyword)
      {
	 case DELAY_TIME:
	    delays.time = va_arg(ptr_to_args, double);
	    break;
	 case SLIDER_LABEL:
	    delays.label = va_arg(ptr_to_args, char *);
	    break;
	 case SLIDER_SCALE:
	    delays.scale = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MAX:
	    delays.max = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MIN:
	    delays.min = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_UNITS:
	    delays.units = va_arg(ptr_to_args, double);
	    break;
	 case SET_GTAB:
	    use_gtab = TRUE;
	    gtable = va_arg(ptr_to_args, int);
	    break;
	 default:
	    fprintf(stdout, "wrong G_Delay() keyword %d specified", keyword);
	    ok = FALSE;
	    break;
      }
      keyword = va_arg(ptr_to_args, int);
   }
   va_end(ptr_to_args);
/* if there was an error, here or in other calls to G_Pulse, Offset, Gain, */
/* or G_Delay, return here, checking only the validity of the arguments    */
   if (!ok)
      return;

   if (isSSHAselected())
   {
     if (isSSHAstillDoItNow())
     {
	if (delays.time >= 0.1)
	{
	    turnOnSSHA(delays.time);
	    setSSHAdelayNotTooShort();
	}
	else
	{
	    text_error( "WARNING: delay of %g is too short for hdwshim\n", delays.time );
	    setSSHAoff();
	}
     }
   }

/* using the old (modified) delay subroutines already present in PSG */
/*   counter = (int) (Codeoffset - startofAcode);*/

   if (use_gtab == TRUE)
   {
	gdelayer(gtable,delays.time, TRUE );
   }
   else
   {
   	counter = (int) (Codeptr - Aacode);
#ifdef DOIPA
	if (newacq && acqiflag)
	   insertIPAcode(counter);
#endif
   	delayer(delays.time, (strcmp(delays.label,"") && acqiflag) );
   }

/* do we want interactive control? */
#ifdef DOIPA
   if ( strcmp(delays.label, "")  && acqiflag )
   {
      write_to_acqi(delays.label, delays.time, delays.units, delays.min,
		    delays.max, TYPE_DELAY, delays.scale, counter*2, 0);
   }
#endif

   if (isSSHAselected() && isSSHAactive())
   {
	turnOffSSHA();
   }
}

/*----------------------------------------------------------------
|   delay(time)/1
|
|	Form the timerwords needed to obtain the time delay (sec)
|
|  
|				Author Greg Brissey  5/13/86
|
|  MOD. 3/23/92 check output board type (fifolpsize) to determine
|	the type of EVENT acode to use.
|				Greg Brissey
+----------------------------------------------------------------*/
void delayer(double time, int do_0_delay)
{
    int tword1;
    int tword2;
    double time1,mindelay;

    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;

    if (time < 0.0)
    {
        if (ix == 1)
        {
	   fprintf(stdout, "\n");
	   fprintf(stdout, "Warning:  Improper delay value set to zero.\n");
	   fprintf(stdout, "          A negative delay cannot be executed.\n");
        }
    }
    else if ((time > 0.0) && (time < mindelay))
    {
        if (ix == 1)
        {
	   fprintf(stdout, "\n");
	   fprintf(stdout, "Warning:  Improper delay value set to zero.\n");
           fprintf(stdout, "          A non-zero delay less than 0.2 us cannot be executed!\n");
        }
    }
    else if (time >= mindelay || do_0_delay)	/* time must be >= to the minimum
					           delay or do nothing except
                                                   when preparing for acqi */
    {
	totaltime += time; /* total timer events for a exp duration estimate */
        checkpowerlevels(time);
        if (bgflag)
	  fprintf(stderr,"time = %lf, totaltime = %lf \n",time,totaltime);
        if (newacq)
	{
	   timerwords(time,&tword1,&tword2);	
	}
        else if (time == 0.0)
        {
           tword1 = SKIP_DELAY;
           tword2 = NO_OP;
        }
	else
	{
	   if ((time > 0.004) && (time < 4.0))
	   {
	      time1 = ((double) ((int) (time * 1000.0)) - 2.0) / 1000.0;
	      timerwords(time1, &tword1, &tword2);
	      create_delay(tword1, tword2, do_0_delay);
	      time -= time1;
	      if (time1 > 0.0045)
		 time -= 0.15e-6;
	   }
	   timerwords(time,&tword1,&tword2);
	}
	create_delay(tword1, tword2, do_0_delay);
    }
}

int create_delay(int tword1, int tword2, int do_0_delay)
{
        if (tword2 == 0)
        {
	  if (newacq)
	  {
             if (do_0_delay)
	     {
	  	putcode(EVENT2_TWRD);	/* an event2 */
		putcode(0);		/* 0 secs    */
		putcode(0);
	     }
	     else
             {
	     	putcode(EVENT1_TWRD);	/* yes, an event1 */
                putLongCode(tword1);
             }
	  }
	  else
	  {
	     putcode(EVENT1_TWRD);	/* yes, an event1 */
	     putcode((codeint) tword1);	/* timerword one */
             if (do_0_delay)
             	putcode(NO_OP);		/* add a NO_OP as second timer word */
                                        /* if we are doing interactive      */
	  }
	  curfifocount++;		/* increment # of fifowords */
	}
	else				/* two timerwords */
	{
	  putcode(EVENT2_TWRD);	/* an event2 */
	  if (newacq)
	  {
             putLongCode(tword2);
             putLongCode(tword1);
	  }
	  else
	  {
	     putcode((codeint) tword1);	/* timerword one */
	     putcode((codeint) tword2);	/* timerowrd two */
	  }
	  curfifocount += 2;
	}
    return(OK);
}

/*-------------------------------------------------------------------
|
|	G_RTDelay(variable arg_list)/n
|
|	Form the timerwords needed to obtain the various Real-Time 
|	controlled delay (sec)
|
|				Author Greg Brissey  1/8/91
+------------------------------------------------------------------*/
void G_RTDelay(int firstkey, ...)
{
   RT_Event RTdelay;
   va_list         ptr_to_args;
   int             counter;
   int             keyword;
   int 		   error;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

/* fill in the defaults */
  RTdelay.rtevnt_type = SET_INITINCR; /* TCNT,TWRD,HSLINE,TWRD_HSLINE,TWRD_TCNT */
  RTdelay.incrtime = 10.0E-6; /* Incremental delay to 10 usec */
  RTdelay.timerbase = 0;
  RTdelay.timercnt = 0;
  RTdelay.hslines = 0;

/* initialize for variable arguments */
   va_start(ptr_to_args, firstkey);
   keyword = firstkey;
/* now check if the user want it his own way */
   while (keyword && ok)
   {
      switch (keyword)
      {
	 case RTDELAY_MODE:
	    RTdelay.rtevnt_type = va_arg(ptr_to_args, int);
	    break;
	 case RTDELAY_INCR:
	    RTdelay.incrtime = va_arg(ptr_to_args, double);
	    break;
	 case RTDELAY_TBASE: /* has multiple uses */
	    RTdelay.timerbase = va_arg(ptr_to_args, int);
	    break;
	 case RTDELAY_COUNT: /* has multiple uses */
	    RTdelay.timercnt = va_arg(ptr_to_args, int);
	    break;
	 case RTDELAY_HSLINES:
	    RTdelay.hslines = va_arg(ptr_to_args, int);
	    break;
	 default:
	    fprintf(stdout, "wrong G_RTDelay() keyword %d specified", keyword);
	    ok = FALSE;
	    break;
      }
      keyword = va_arg(ptr_to_args, int);
   }
   va_end(ptr_to_args);
/* if there was an error, here or in other calls to G_Pulse, Offset, Gain, */
/* or G_Delay, return here, checking only the validity of the arguments    */
   if (!ok)
      return;
/* using the old (modified) delay subroutines already present in PSG */
/*   counter = (int) (Codeoffset - startofAcode);*/
   counter = (int) (Codeptr - Aacode);
   param.setwhat = SET_VALUE;
   param.valptr = (char *) &RTdelay;
   error = Send(RT_Delay, MSG_SET_EVENT_VAL_pr, &param, &result);
}

/*----------------------------------------------------------------
|   gdelayer(gtable,time)/1
|
|	Form the timerwords needed to obtain the time delay (sec)
|	same as delayer except puts it out to global table location.
|
|				Author matt howitt  10/15/92
+----------------------------------------------------------------*/
int gdelayer(int gtable, double time, int do_0_delay)
{
    int tword1;
    int tword2;
    double mindelay;

    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;

    validate_imaging_config("Delay list");

    if (time < 0.0)
    {
        if (ix == 1)
        {
	   fprintf(stdout, "\n");
	   fprintf(stdout, "Warning:  Improper delay value set to zero.\n");
	   fprintf(stdout, "          A negative delay cannot be executed.\n");
        }
    }
    else if ((time > 0.0) && (time < mindelay))
    {
        if (ix == 1)
        {
	   fprintf(stdout, "\n");
	   fprintf(stdout, "Warning:  Improper delay value set to zero.\n");
           fprintf(stdout, "          A non-zero delay less than 0.2 us cannot be executed!\n");
        }
    }
    else if (time >= mindelay || do_0_delay)	/* time must be >= to the minimum
					           delay or do nothing except
                                                   when preparing for acqi */
    {
	totaltime += time; /* total timer events for a exp duration estimate */
        if (bgflag)
	  fprintf(stderr,"time = %lf, totaltime = %lf \n",time,totaltime);
        if (newacq)
	{
	   timerwords(time,&tword1,&tword2);	
	}
        else if (time == 0.0)
        {
           tword1 = SKIP_DELAY;
           tword2 = NO_OP;
        }
        else
	   timerwords(time,&tword1,&tword2);

	if (tword2 == 0)		/* one timerword ? */
	{
	  if (newacq)
	  {
	     short *ptr;
             if (do_0_delay)
	     {
	  	putgtab(gtable,EVENT2_TWRD);	/* an event2 */
		putgtab(gtable,0);		/* 0 secs    */
		putgtab(gtable,0);
	     }
	     else
	     	putgtab(gtable,EVENT1_TWRD);	/* yes, an event1 */
	     ptr = (short *) &tword1;
	     putgtab(gtable,(codeint) *ptr);	/* timerword one */
	     ptr++;
	     putgtab(gtable,(codeint) *ptr);	/* timerword one */
	  }
	  else
	  {
	    putgtab(gtable,EVENT1_TWRD);		/* yes, an event1 */
	    putgtab(gtable,(codeint) tword1);	/* timerword one */
            putgtab(gtable,NO_OP);	/* add a NO_OP as second timer word */
                                        /* if we are doing interactive      */
	  }
	    curfifocount++;		/* increment # of fifowords */
        }
	else				/* two timerwords. */
	{
	  putgtab(gtable,EVENT2_TWRD);	/* an event2 */
	  if (newacq)
	  {
	     short *ptr;
	     ptr = (short *) &tword2;
	     putgtab(gtable,(codeint) *ptr);	/* timerword one */
	     ptr++;
	     putgtab(gtable,(codeint) *ptr);	/* timerword one */
	     ptr = (short *) &tword1;
	     putgtab(gtable,(codeint) *ptr);	/* timerword one */
	     ptr++;
	     putgtab(gtable,(codeint) *ptr);	/* timerword one */
	  }
	  else
	  {
	    putgtab(gtable,(codeint) tword1);	/* timerword one */
	    putgtab(gtable,(codeint) tword2);	/* timerowrd two */
	  }
	    curfifocount += 2;
	}
    }
    return(OK);
}
