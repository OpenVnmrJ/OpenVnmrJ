/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
extern  int fifolpsize;	/* size in words of looping fifo */
extern  int newacq;	/* size in words of looping fifo */
extern  int bgflag;

#define SEC_TBASE 0x4000
#define MSEC_TBASE 0x3000
#define USEC_TBASE 0x2000
#define NSEC_TBASE 0x1000
#define EXT_TBASE 0x0000


/* structures for rttimerwords() */
struct _timebase {
		double timemax;
		double ovrhd;
		double tbase;
		int mintick;	/* minimum tick count for timebase */
		int maxtick;	/* maximum tick count for timebase */
		int starttick;	/* one tick of clock is represented by n count */
		 };
struct _pcntrl {
                double minpulse;	/* min pulse for contr0ller */
		int minpulsecnt;	/* count for clock to give min pulse */
	        struct _timebase clkbase[4];
	       };

struct _pcntrl  pulsecntrler;

static int ToverHead = 0;
static void carry(int index, int ticks[]);

/*----------------------------------------------------------------------
|
|	timerwords()/3
|	Form 1 or 2 timerwords given the time delay in seconds (double)
|       The maximum delay is 8190 seconds.
|       The available clocks are:
|	 Old Output Board
|         1      100 nanosecond clock
|         2      1   microsecond clock
|         3      1   millisecond clock
|         4      1   second clock
|
|	 New Output Board (minimum pulse still 200nsec)
|         1      25  nanosecond clock
|         2      1   microsecond clock
|         3      1   millisecond clock
|         4      1   second clock
|
|       The accuracy of the timer words is as follows:
|            if time >  4099.0  secs, then accurate to the nearest 1.0  sec
|       else if time >  4.099   secs, then accurate to the nearest 1.0 msec
|       else if time >  4.099  msecs, then accurate to the nearest 1.0 usec
|       else if time <= 4.099  msecs, then accurate to the nearest 100 nsec
|
+----------------------------------------------------------------------*/
#define DTICK	1.25e-8		/* 12.5 nanoseconds */
#define MAX_EVENT1 2.5		/* Max EVENT1 delay */
timerwords(time,tword1,tword2)
register double  time;
int  *tword1;
int  *tword2;
{
  if (newacq) /* 10 ns timing */
  {
     if (time < MAX_EVENT1)
     {
	*tword1 = (int) ((time/DTICK) + 0.5);
	*tword2 = 0;
        if (bgflag)
            fprintf(stderr,"time: %17.12lf is %d ticks of 12.5 ns.\n",
								time,*tword1);
     }
     else
     {
	double dseconds;
	int iseconds;

	iseconds = (time/1.0);
	dseconds = (double) iseconds;
	*tword2 = iseconds;
	*tword1 = (int) (( (time - dseconds)/DTICK ) + 0.5);
        if (bgflag)
            fprintf(stderr,
		"time: %17.12lf is %d seconds and %d ticks of 12.5 ns.\n",
                    time,*tword1,*tword2);
     }
  }
  else if (fifolpsize < 65) /* determine output board type */
  {
    int  word[2];

    if (bgflag)
        fprintf(stderr,"time delay: %17.12lf \n",time);
    word[0] = word[1] = 0;
    time += 5e-8;	/* add 50nsec to round up to nearest 100nsec */
    if (time >= 1e-7)   /* if time < 100nsec time = 0 */
    {
      if (time < 2e-7)	/* time < 200 nsec, forse to 200 nsec */
        time = 2.005e-7;
      if (time > 8190.0) /* time > 8190, force to 8190 */
        time = 8190.005;
      if (time > 4099.0) /* time > 4099sec, use 2 sec timebase words */
      {
        word[0] = (4 << 12) | 4095;
        word[1] = (4 << 12) | (int) (time - 4095.0 + 0.5);
      }
      else if (time > 4.099)  /* time 4099 - 4.099 sec, use sec timebase 1st*/
      {
        time += 0.0005;
        word[0] = (int) time;
        if (word[0] > 4095)
          word[0] = 4095;
        time -= (double) word[0];
        word[0] |= (4 << 12);
        if (time >= 2e-3)	/* if remainder time >= 2msec, use msec */
          word[1] = (3 << 12) | (int) (time * 1e3);
        else if (time >= 1e-3)  /* if time > 1msec, use usec timebase */
          word[1] = (2 << 12) | 1000;
      }
      else if (time > 4.099e-3)	/* time 4.099 - .004099 msec, use msec 1st*/
      {
        time *= 1e3;
        time += 0.0005;
        word[0] = (int) time;
        if (word[0] > 4095)
          word[0] = 4095;
        time -= (double) word[0];
        word[0] |= (3 << 12);	/* 1st msec timebase, 0-4095 */
        if (time >= 2e-3)
          word[1] = (2 << 12) | (int) (time * 1e3);
        else if (time >= 1e-3)
          word[1] = (1 << 12) | 10;
      }
      else if (time >= 4.096e-4)
      {
        time *= 1e6;
        time += 0.0005;
        word[0] = (int) time;
        if (word[0] > 4095)
          word[0] = 4095;
        time -= (double) word[0];
        if ((time >= 0.1) && (time < 0.2))
        {
          time += 1.0;
          word[0] -= 1;
        }
        word[0] |= (2 << 12);
        if (time >= 0.2)
          word[1] = (1 << 12) | (int) (time * 1e1);
      }
      else
      {
        word[0] = (1 << 12) | (int) (time * 1e7);
      }
    }
    *tword1 = word[0];
    *tword2 = word[1];
    if (bgflag)
        fprintf(stderr,"timer word 1 is %d  timer2 word 2 is %d\n",
                word[0],word[1]);
  }
  else   /* New Output Board Routine */
  {
    /*---------------------------------------------------------------------
    |       The accuracy of the timer words is as follows:
    |            if time >  4100.096  secs, then accurate to the nearest 1.0  sec
    |       else if time >  4.100096  secs, then accurate to the nearest 1.0 msec
    |       else if time >  4.1987  msecs, then accurate to the nearest 1.0 usec
    |       else if time >  0.10255  msecs, then accurate to the nearest 25 nsec
    |	count of zero is one timer unit of delay  (e.g. 4000 = 1 sec )
    |
    |					Author:  Greg Brissey  12/16/87
    +----------------------------------------------------------------------*/
    register int  word0,word1;	/* I feel the need for speed */

    if (bgflag)
        fprintf(stderr,"time delay: %17.12lf \n",time);
    word0 = word1 = 0;
    if (bgflag)
        fprintf(stderr,"time delay: %17.12lf \n",time);
    if (time >= 1e-7)  /* time not greater than 100 nsec then forget it */
    {
      if (time < 200.0e-9)	/* if 199 - 100 nsec  force to 200 nsec */
        time = 200.5e-9;

      if (time > 8192.0)  /* force time to the 2 time word max 8192 sec */
        time = 8192.005;

      if (time >= 4100.096)  /* 8192 - 4100sec, use two sec timebase words */
      {
        word0 = SEC_TBASE | 4095;	/* 4096 sec  + 150nsec */
        word1 = SEC_TBASE | (int) (time - 4097.0 + 0.5); /* 0 == 1sec */
      }
      /* ---- 4100.096 - 4.100096 ---- */
      else if (time > 4.100096)	/*---- use sec & msec time base ----*/
      {
        time += 0.0005;		/* round to msec */
        word0 = ((int) time) - 1;

        if (word0 > 4095) /* max 4096 sec delay, 4095 count */
          word0 = 4095;

        time -= (double) (word0 + 1);/* remaining time */
        word0 |= SEC_TBASE;	/* set timebase */

        if (time >= 1e-3)
          word1 = MSEC_TBASE | (((int) (time * 1e3)) - 1);
      }
      /* ---- 4.100096 - .0041987 ---- */
      else if (time > 4.1987e-3) /*---- use msec & usec time bases ----*/
      {
        time *= 1e3;
        time += .0005;		/* round to nearest usec */
        word0 = ((int) time) - 1;

        if (word0 > 4095)	/* max dealy 4096 msec, 4095 count */
          word0 = 4095;

        time -= (double) (word0 + 1);	/* remaining time */
        word0 |= MSEC_TBASE;	/* set timebase */

        if (time >= 1e-3)
          word1 = USEC_TBASE | ((int) (time * 1e3)) - 1;
      }
      /* ---- 0.004099 - .0004198700 ---- */
      else if (time > 1.02550e-4) /*---- use usec & 25nsec time bases */
      {
        time *= 1e6;
        time += 0.0125;		/* round to nearest 25 nsec */
        word0 = ((int) time) - 1;

        if (word0 > 4095)	/* max 4096 usec delay, count 4095 */
          word0 = 4095;

        time -= (((double) (word0 + 1)) + 0.150);/* remaining time (150nsec/word)*/
	word0 |= USEC_TBASE;		/* set timebase */

        if (time < 1.0)
	{
	    word0 -= 1;	/* take 1usec put back into nsec timebase */
	    time += 1.0;
        }
	time -= 0.2;	/* subtract minimum pulse length 200nsec, count of 1 */
	time /= 0.025;
        word1 = NSEC_TBASE | ((int) time) + 1;
      }
      else
      {
        time += 12.5e-9;	/* round to nearest 25 nsec */
	time *= 1e9;	/* put to nsec */
        time -= 200.0;	/* minimum pulse lenght */
	time /= 25.0;
        word0 = NSEC_TBASE | ((int) time) + 1;
      }
    }
    *tword1 = word0;
    *tword2 = word1;
    if (bgflag)
        fprintf(stderr,"timer word 1 is 0x%x  timer2 word 2 is 0x%x\n",
                word0,word1);
  }
}

/*-----------------------------------------------------------------------
|   rttimerwords(time,twords)
|      calculate timerwords (1-4, not limited to two) to give delay
|      accurate to the resolution of the nsec timebase
|   The structure _pcntrl contians all the information on the pulse 
|   controller board necessary to calculate the timebase counts.
|   return timerwords in the array twords in the order nsec - sec .
|				Author: Greg Brissey   1/15/91
+---------------------------------------------------------------------*/
rttimerwords(time,twords)
double time;
int twords[4];
{
    int ticks[4];
    register int i;	
    register struct _timebase *tbptr;
    register struct _timebase *tbptr1;
    void carry();

    if (bgflag)
        fprintf(stderr,"rttimerwords(): time delay: %17.12lf \n",time);

    /*------------------------------------------------------------------*/
    /* INOVA only returns the first two words for the timerwords. 	*/
    /* The first word is the number of seconds the second word is the	*/
    /* number of ticks.							*/
    /*------------------------------------------------------------------*/
    if (newacq) /* 10 ns timing */
    {
       if (time < MAX_EVENT1)
       {
	  ticks[2] = (int) ((time/DTICK) + 0.5);
	  ticks[1] = 0;
          if (bgflag)
            fprintf(stderr,"time: %17.12lf is %d ticks of 12.5 ns.\n",
							time,ticks[2]);
       }
       else
       {
	  double dseconds;
	  int iseconds;

	  iseconds = (time/1.0);
	  dseconds = (double) iseconds;
	  ticks[1] = iseconds;
	  ticks[2] = (int) (( (time - dseconds)/DTICK ) + 0.5);
          if (bgflag)
            fprintf(stderr,
		"time: %17.12lf is %d seconds and %d ticks of 12.5 ns.\n",
                    time,ticks[2],ticks[1]);
       }
       twords[0] = 1;
       twords[1] = (ticks[1] >> 16) & 0xffff;	/* high order word */
       twords[2] = ticks[1] & 0xffff;		/* low order word */
       twords[3] = (ticks[2] >> 16) & 0xffff;	/* high order word */
       twords[4] = ticks[2] & 0xffff;		/* low order word */
       return;
    }

    for (i=0 ;i<3 ;i++ )
    {
       tbptr = &pulsecntrler.clkbase[i];
       tbptr1 = &pulsecntrler.clkbase[i+1];
       if (time > tbptr1->timemax + tbptr->ovrhd)
       {
	   ticks[i] = ((time - tbptr->ovrhd) / tbptr->tbase) + .00005;
	   time -= ((ticks[i] * tbptr->tbase) + tbptr->ovrhd) ;
	   if (time < pulsecntrler.minpulse)
	   {
	      time += (tbptr->mintick * tbptr->tbase);
	      ticks[i] -= tbptr->mintick;
	      if (ticks[i] < tbptr->mintick) /* only a issue for old output boards */
	      {
		 time += ticks[i] * tbptr->tbase;
		 ticks[i] = 0;
	      }
           }
	   if (tbptr->starttick == 0)
	    ticks[i]--;		/* subtract one since a count of 0 is one count */
       }
       else
       {
	  ticks[i] = 0;
       }
    }
    i=3;
    ToverHead = 0;
    tbptr = &pulsecntrler.clkbase[3];
    time -= pulsecntrler.minpulse; /* remove minimum pulse of nsec time base */
    ticks[i] = pulsecntrler.minpulsecnt;
    ticks[i] += (time / tbptr->tbase) + 0.00005;
    if (ticks[i] < tbptr->mintick)
    {
       carry(2,ticks);
    }
    if (ToverHead) 
      ticks[i] -= ToverHead / (int) ((tbptr->tbase * 1.0e9) + .0005);

    twords[0] = 0;
    if (ticks[3]) 
    { 
      twords[0] += 1;
      twords[1] = NSEC_TBASE | ticks[3]; /* nsecs */
    }
    else
    {
      twords[1] = 0; /* nsecs */
    }
    if (ticks[2]) 
    { 
      twords[0] += 1;
      twords[2] = USEC_TBASE | ticks[2]; /* usec */
    }
    else
    {
      twords[2] = 0; /* nsecs */
    }
    if (ticks[1]) 
    { 
      twords[0] += 1;
      twords[3] = MSEC_TBASE | ticks[1]; /* msec */
    }
    else
    {
      twords[3] = 0; /* nsecs */
    }
    if (ticks[0])
    { 
      twords[0] += 1;
      twords[4] = SEC_TBASE | ticks[0]; /* sec */
    }
    else
    {
      twords[4] = 0; /* nsecs */
    }
    if (bgflag)
    {
      int i;
      fprintf(stderr,"rttimerwords(): timer words: ");
      for (i=1;i<=4;i++)
        fprintf(stderr,"%d-0x%x (%d) ",i,twords[i],twords[i]);
      fprintf(stderr,"\n");
    }
}
/*----------------------------------------------------------------
|  carry(index,ticks)
|    recusive routine to take care of carrying the count to the
|    different timebases
|				Author: Greg Brissey   1/15/91
+-----------------------------------------------------------------*/
static void carry(int index, int ticks[])
{
    register struct _timebase *tbptr;
    register struct _timebase *tbptr1;

    tbptr = &pulsecntrler.clkbase[index];
    tbptr1 = &pulsecntrler.clkbase[index+1];
    if (ticks[index] < tbptr->mintick + 1)
    {
       carry(index-1,ticks);
    }
    ticks[index]--;
    if (ticks[index+1] == 0)  /* timer word was not used before add overhead time */
       ToverHead += (tbptr1->ovrhd * 1.0e9) + .01;
    if (index+1 == 3)
    {
      ticks[index+1] += 1000 / (int) ((tbptr1->tbase * 1.0e9) + .0005);
    }
    else
     ticks[index+1] += 1000;
}
/*-------------------------------------------------------------------------
| absolutecnt
|     returns the logical count for the timebase to give the delay
|     i.e. remove hardware dependence of timebase count. 
|     Used for RT delays
|     E.G. new output board timerword 0x1003 = 250 Nsec this routine would return
|	   250/25 = 10; etc.
|				Author: Greg Brissey   1/15/91
+--------------------------------------------------------------------------*/
absolutecnt(timerword)
int timerword;
{
    register struct _timebase *tbptr;
    int timercnt;

    timercnt = timerword & 0x0fff;
    switch (timerword & 0xf000)
    {
        case SEC_TBASE:
                tbptr = &pulsecntrler.clkbase[0];
	        if (tbptr->starttick == 0)
	           timercnt++;		/* add one since a count of 0 is one count */
                break;
        case MSEC_TBASE:
                tbptr = &pulsecntrler.clkbase[1];
	        if (tbptr->starttick == 0)
	           timercnt++;		/* add one since a count of 0 is one count */
                break;
        case USEC_TBASE:
                tbptr = &pulsecntrler.clkbase[2];
	        if (tbptr->starttick == 0)
	           timercnt++;		/* add one since a count of 0 is one count */
                break;
        case NSEC_TBASE:
                tbptr = &pulsecntrler.clkbase[3];
    	        timercnt += ((pulsecntrler.minpulse / tbptr->tbase) + 0.00005) 
			    - pulsecntrler.minpulsecnt; 
                break;
    }
    return(timercnt);
}
/*  New output board 
struct _pcntrl pulsecntrler = { 200.0e-9, 1,
				4096.0, 150.0e-9, 1.0, 1, 4096, 0,
				4.096 , 150.0e-9, 1.0e-3, 1, 4096, 0,
				.004096, 150.0e-9, 1.0e-6, 1, 4096, 0,
				.00010200, 0.0, 25.0e-9,  1, 4096, 1,
			      };
   Old output board
struct _pcntrl pulsecntrler = { 200.0e-9, 2,
				4095.0, 200.0e-9, 1.0, 2, 4095, 1,
				4.095 , 200.0e-9, 1.0e-3, 2, 4095, 1,
				.004095, 200.0e-9, 1.0e-6, 2, 4095, 1,
				.000407500, 0.0, 100.0e-9,  2, 4095, 1,
			      };
*/
/*----------------------------------------------------------
| inittimerconst(fifosize)
|    initialize pusle controller timerword  constants
|				Author: Greg Brissey 1/15/91
+---------------------------------------------------------*/
void
inittimerconst()
{
    register struct _timebase *tbptr;
   if (fifolpsize > 64)
   {
       pulsecntrler.minpulse = 200.0e-9;
       pulsecntrler.minpulsecnt = 1;

       /* sec timebase constants */
       tbptr = &pulsecntrler.clkbase[0];
       tbptr->timemax = 4096.0;
       tbptr->ovrhd = 150.0e-9;
       tbptr->tbase = 1.0;
       tbptr->mintick = 1;
       tbptr->maxtick = 4096;
       tbptr->starttick = 0;
       /* msec timebase constants */
       tbptr = &pulsecntrler.clkbase[1];
       tbptr->timemax = 4.096;
       tbptr->ovrhd = 150.0e-9;
       tbptr->tbase = 1.0e-3;
       tbptr->mintick = 1;
       tbptr->maxtick = 4096;
       tbptr->starttick = 0;

       /* usec timebase constants */
       tbptr = &pulsecntrler.clkbase[2];
       tbptr->timemax = .004096;
       tbptr->ovrhd = 150.0e-9;
       tbptr->tbase = 1.0e-6;
       tbptr->mintick = 1;
       tbptr->maxtick = 4096;
       tbptr->starttick = 0;

       /* nsec timebase constants */
       tbptr = &pulsecntrler.clkbase[3];
       tbptr->timemax = 0.00010200;
       tbptr->ovrhd = 0.0;
       tbptr->tbase = 25.0e-9;
       tbptr->mintick = 1;
       tbptr->maxtick = 4096;
       tbptr->starttick = 1;
       
   }
   else
   {
       pulsecntrler.minpulse = 200.0e-9; 
       pulsecntrler.minpulsecnt = 2;
 
       /* sec timebase constants */ 
       tbptr = &pulsecntrler.clkbase[0];
       tbptr->timemax = 4095.0;
       tbptr->ovrhd = 200.0e-9;
       tbptr->tbase = 1.0;
       tbptr->mintick = 2;
       tbptr->maxtick = 4095;
       tbptr->starttick = 1;
       /* msec timebase constants */
       tbptr = &pulsecntrler.clkbase[1];
       tbptr->timemax = 4.095;
       tbptr->ovrhd = 200.0e-9;
       tbptr->tbase = 1.0e-3;    
       tbptr->mintick = 2;
       tbptr->maxtick = 4095;
       tbptr->starttick = 1;
 
       /* usec timebase constants */
       tbptr = &pulsecntrler.clkbase[2];
       tbptr->timemax = .004095;
       tbptr->ovrhd = 200.0e-9;
       tbptr->tbase = 1.0e-6;
       tbptr->mintick = 2;
       tbptr->maxtick = 4095;
       tbptr->starttick = 1;
 
       /* nsec timebase constants */
       tbptr = &pulsecntrler.clkbase[3];
       tbptr->timemax = 0.00040750;
       tbptr->ovrhd = 0.0;
       tbptr->tbase = 100.0e-9;
       tbptr->mintick = 2;  
       tbptr->maxtick = 4095;
       tbptr->starttick = 1;
   }
}

