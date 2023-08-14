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
/*
modification history
--------------------
5-16-95,gmb  added to vwacq category, take from xracq 
9-28-95,gmb  removed sigavg() calls w/ getLockLevel()  adjusted level to
		match those of the Inova MSR board lock level DAC
       calibration values:
		Meter %		Acqi %		DAC value
		10		7		480
		20		16		1018
		30		24		1550
		42		34		2203
		52		43		2750
		60		50		3160
		72		60		3818
		80		68		4334
		90		76		4864
	       100		84		5390
	      >100		96		6060
	      >100	      >100		6845
10-30-95 added floating point saturation tolerant lk_anlz -
replaces wonko,signalpwr,and a bunch of other stuff......

4-30-97, Lynn Cai, New fuzzy logic autolock programs, add new functions for fuzzy autolock and delete functions that are not used in the new version. 
5-16-97, LC, changes for power increase/decrease and when mode==6.
5-20-97, LC, chage the rules for power_gain_adj() so it works for easily saturated samples.
06-1997. ROL, reworked to use either z0 or lockfreq to find the lock signal
02-17-98, LC, changes for phase adjustment when mode==6.
01-05-05,  GMB - grabbed from vwacq and now going into nvacq
*/

/*--------------------------------------------------------------------------
Changes to autolock.c
   risort() removed  2-7-90

   (6-23-89)
   Tries two times to find signal before autopwr().   
   xadj_pwr_gain(), check_level(), adjust_gain() added to reduce duplication. 
   In autophs() extra case label (ENDORDROP) added to make it work 
	as intended. 
   constant MAXDROP set to 15 used in autophs() was 30. 
   sigavg does not sort data and ignores imaginary data which is not used. 
   autopwr() only reduces a high level to 7000.  
   autopwr() changed to not run up power so high before beginning rundown. 
   diagnostic messages reduced and a few placed in #ifdef DEBUG to  
	reduce length. Unused variables removed to reduce length. 
   The effects of the other changes is to reduce chances of failure from 
	a lost lock.

   autopwr() has a maximum power setting of 30.    7/10/85  GB	
    
*/
/* percent lock level for Inova MSR Brd DAC */

#ifdef INOVA
#define LK2p_LEVEL   100
#define LK3p_LEVEL   150
#define LK5p_LEVEL   256
#define LK10p_LEVEL  500
#define LK15p_LEVEL  750
#define LK20p_LEVEL  1018
#define LK25p_LEVEL  1284
#define LK30p_LEVEL  1550
#define LK42p_LEVEL  2203
#define LK52p_LEVEL  2750
#define LK60p_LEVEL  3160
#define LK72p_LEVEL  3818
#define LK80p_LEVEL  4334
#define LK90p_LEVEL  4864
#define LK100p_LEVEL 5390
#define LK135p_LEVEL 7000
#else
#define LK2p_LEVEL   (2*16)
#define LK3p_LEVEL   (3*16)
#define LK5p_LEVEL   (5*16)
#define LK10p_LEVEL  (10*16)
#define LK15p_LEVEL  (15*16)
#define LK20p_LEVEL  (20*16)
#define LK25p_LEVEL  (25*16)
#define LK30p_LEVEL  (30*16)
#define LK42p_LEVEL  (42*16)
#define LK52p_LEVEL  (52*16)
#define LK60p_LEVEL  (60*16)
#define LK72p_LEVEL  (72*16)
#define LK80p_LEVEL  (80*16)
#define LK90p_LEVEL  (((90 - 1300/16) * 15) + 1300)
#define LK100p_LEVEL (((100 - 1300/16) * 15) + 1300)
#define LK135p_LEVEL 2048
#endif

#define LOCKOFFSET	10
#define ABORTCODE	21
#define LOCKED		7

#define LK20HZ	2

#define AUTOLOCK	0012 	/* $0A 10 auto locking */
#define AUTOPWR		0032	/* $1A 26 autopower	*/
#define AUTOPHS		0042	/* $22 34 autophasing  */
#define LKFAIL		0071	/* $39 57 autolocking failure */

#define MINGAIN		10
#define STMINGAIN	38
#define MINPWR		8
#define STMINPWR	20
#define SCANS		1
#define GAINDLY 85      /*   Gain loop delay 7/12/85, 50 too fast for Acetone*/
#define POWERDLY 300    /*   min time to recover form power change */
#define PHASEDLY 85     /*   4Hz filter at 2kHZ requires a min of .5 sec to */
			/*   recover signal level */
#define SHIMDLY 185	/*   wait 1850 ms when changing a shim (Z0) */
#define LOCKFREQDLY 10  /*   wait 100 ms when changing the lock frequency */

#define TRUE		1
#define FALSE		0
#define FIDSIZ 		256

#define sign(val) \
   ( ((val) < 0 ) ? -1 : 1 )

#define RANGE_LKFREQ_APWORDS	1750


#include <vxWorks.h>
/*
#include "lock_interface.h"
#include "logMsgLib.h"
#include "adcObj.h"
*/

#include "logMsgLib.h"
#include "hostAcqStructs.h"
#include "autolock.h"
#include "errorcodes.h"

extern int host_abort;
extern int failAsserted;

#ifdef PUNT_FOR_NOW
extern int sampleHasChanged;
#endif

static int g_maxpwr;
static int g_maxgain;
static int g_mode;
/* place to retain the best setting in case of abort */
static int BestPower;
static int BestGain;
static int BestPhase;
static int BestZ0;
int debug_autolock =0;

static FIND_LOCK_OBJ	theFindLockObj;

/* programs to work with a Find Lock object */

static
get_lkfreq_limits( long *p_low_lkfreq, long *p_high_lkfreq )
{
	*p_low_lkfreq = getLockFreqAP() - RANGE_LKFREQ_APWORDS/2;
	*p_high_lkfreq = getLockFreqAP() + RANGE_LKFREQ_APWORDS/2;
}

/* We want to reference the program to set the Find Lock Parameter
   (z0 or lockfreq) using the Find Lock Object.  A C-language Macro
   won't do here; thus the two short programs here.			*/

static int
set_z0_program( long z0value )
{
   set_lock_offset( (int) z0value );
}

static int
set_lkfreq_program( long lkfreq_apvalue )
{
   /* set_lkfreq_ap( lkfreq_apvalue ); */
   set_LockFreq( lkfreq_apvalue );  /* this posses a problem DDS expects a double */
}


prepareFindLockObj( int method2use )
{
   int limitz0;

   if (method2use == USE_LOCKFREQ) {
      theFindLockObj.method = USE_LOCKFREQ;
      theFindLockObj.initial = getLockFreqAP();
      get_lkfreq_limits( &theFindLockObj.smallest, &theFindLockObj.largest );
      theFindLockObj.middle = (theFindLockObj.smallest + theFindLockObj.largest) / 2;
   }
   else {
      theFindLockObj.method = USE_Z0;
      theFindLockObj.initial = get_lock_offset();
      limitz0 = dac_limit();
      if (limitz0 < 0)
       limitz0 = 0 - limitz0;
      theFindLockObj.largest = limitz0;
      theFindLockObj.smallest = 0 - theFindLockObj.largest;
      theFindLockObj.middle = 0;
   }

   theFindLockObj.current = theFindLockObj.initial;
   theFindLockObj.best = theFindLockObj.initial;
}

/*  delay an appropriate amount of time, depending on whether
    Z0 or lockfreq is being adjusted to find the lock signal.	*/

static
find_lock_delay()
{
   if (theFindLockObj.method == USE_LOCKFREQ)
     Tdelay( LOCKFREQDLY );
   else
     Tdelay( SHIMDLY );
}

static int
get_find_lock_offset()
{
   int retval;

   if (theFindLockObj.method == USE_LOCKFREQ)
     retval = getLockFreqAP();
   else
     retval = get_lock_offset();
   DPRINT2( debug_autolock, "get find lock offset will return %d (0x%x)\n", retval, retval );

   return( retval );
}

static
set_find_lock_offset( int offset )
{
   DPRINT2( debug_autolock, "set find lock offset starts with %d (0x%x)\n", offset, offset );
   if (theFindLockObj.method == USE_LOCKFREQ)
   {
     /* set_lkfreq_ap( offset ); */
     set_LockFreq( offset );  /* this posses a problem DDS expects a double */
   }
   else
   {
     /* set_ShimZ0( offset ); */
     set_lock_offset( offset );
   }
   theFindLockObj.current = offset;
}

showFindLockObj()
{
   if (theFindLockObj.method == USE_LOCKFREQ)
   {
      printf( "Using lock frequency to find the lock resonance\n" );
      printf( "current: 0x%x\n", theFindLockObj.current );
      printf( "initial: 0x%x\n", theFindLockObj.initial );
      printf( "best: 0x%x\n", theFindLockObj.best );
      printf( "smallest: 0x%x\n", theFindLockObj.smallest );
      printf( "largest: 0x%x\n", theFindLockObj.largest );
      printf( "delay: %d\n", LOCKFREQDLY );
   }
   else if (theFindLockObj.method == USE_Z0)
   {
      printf( "Using Z0 to find the lock resonance\n" );
      printf( "current: %d\n", theFindLockObj.current );
      printf( "initial: %d\n", theFindLockObj.initial );
      printf( "best: %d\n", theFindLockObj.best );
      printf( "smallest: %d\n", theFindLockObj.smallest );
      printf( "largest: %d\n", theFindLockObj.largest );
      printf( "delay: %d\n", SHIMDLY );
   }
   else
      printf( "Find Lock object not properly set up\n" );
}

int testMethod;

/*  End of programs to work with a Find Lock object  */

int do_autolock(int lkmode,int maxpwr,int maxgain, int sampleHasChanged)
{
   int  rstatus = 0;	/* return status of auto-lock */
   int old_stat;
   int  method2use;

   DPRINT3( debug_autolock, "do autolock starts with mode = %d,maxpwr=%d,maxgain=%d\n", lkmode,maxpwr,maxgain );
   host_abort = 0;
   if (lkmode & 16)
   {
      method2use = USE_LOCKFREQ;
      lkmode &= ~16;

      DPRINT(-1,"Autolock using lock frequency is Not supported at this time.\n");
   }
   else
    method2use = USE_Z0;
     
   if (lkmode == 1)		 /* alock='y' Turn on hardware Autolock */
   {
      lkmode = 6;
   }
   if (lkmode == 5)      /* alock='u'   */
   {
      setLockMode(LOCK_OFF);	 /* Turn off hardware lock */
   }
   if ( (lkmode == 6) && (getLockMode() == LOCK_ON) && locked(100) ) 
   {
      DPRINT(-1,"Already locked\n");
      return(0);
   }

   /* alock='n' = 0 alock='y' = 1, alock='a' = 3, alock='s' = 4, alock='u' = 5, alock='f' = 3+8 */
   if ( (lkmode == 2) || (lkmode == 3) || (lkmode == 6) || 
      ((lkmode == 4) &&  (sampleHasChanged == TRUE))) 
   {	   			/* Check Mode */
      /* init_dac(); Inova & Nirvana not used */

      DPRINT(-1,"Auto Lock\n");

      old_stat = getUpdateAcqState();
      updateAcqState(ACQ_ALOCK);

      prepareFindLockObj( method2use );
      DPRINT2( debug_autolock, "in do autolock, z0: %d, lockfreq: 0x%x\n",
		    get_lock_offset(), getLockFreqAP() );
      /* getstatblock();   /* force statblock upto hosts */
	DPRINT1( debug_autolock, "mode now %d in do autolock\n", lkmode );

      if (rstatus = fuzzy_autolock(lkmode, maxpwr, maxgain, sampleHasChanged) )
      {
#ifdef TODO
      updateCodes(HARD_ERROR,rstatus);
#endif
      }

#ifdef TODO
      hwpar_update = 1;
      retrnshm(autopt);
#endif
      DPRINT(0,"Auto Lock Done\n");
      lk2kcs();	/* reset lock filter to 2kHz slow */
      if (!host_abort && !failAsserted)
         updateAcqState(old_stat);
   }
   return(rstatus);
}

int getBestLock()
{
   BestPower = getLockPower();
   BestGain = getLockGain();
   BestZ0 = get_find_lock_offset();
   BestPhase = getLockPhase();

 return(1);

}

int setBestLock()   /* untested as of 9/27/95 */
{
   setLockMode(LOCK_OFF);		/* turn hardware lock off */
   setLockPower(BestPower);
   setLockGain(BestGain);
   set_find_lock_offset(BestZ0);
   setLockPhase(BestPhase);
   setLockMode(LOCK_ON);		/* turn hardware lock on */

   theFindLockObj.best = BestZ0;

 return(1);

}

int adjustGain()
{
   int doit,cntdwn,gain;
   long sum;

   doit = TRUE;
   Ldelay(&cntdwn,800);
   sum = getLockLevel();
   gain = getLockGain();
   /* while signal is > 73% or < 33% of max signal then change it */
   DPRINT3(debug_autolock,"Level= %ld  30Percent= %d  70Percent= %d\n",
			sum,LK42p_LEVEL,LK72p_LEVEL);
   while(!host_abort && doit && (sum > LK72p_LEVEL || sum < LK42p_LEVEL))
   {  
      /* adjust_gain(&gain,&doit,sum,13000L); */
      adjust_gain(&gain,&doit,sum,LK52p_LEVEL);
      if (Tcheck(cntdwn)) break;	/* timeout stop */
      Tdelay(GAINDLY);
      sum = getLockLevel();
   }
   DPRINT2(debug_autolock,"Gain= %d  Sig. Level = %ld\n",getLockGain(),sum);
}

/* alock=y translates to g_mode == 6. In this mode, the system will
   find z0 and use existing power, phase and gain values. The search
   for z0 is done a several steps. In findresPart1, the z0 range is
   quickly search for a maximum integral of the real channel. During
   the search, an average
   response is also determined. Depending on how much larger the maximum
   is, relative to the average, the range is reduced by a factor of 8.
   This reduction may occur in steps.

   In the second phase of findresPart 1,
   Instead of the integral of the real channel, the sum of positive points
   above a minimum plus the sum of negative points below the minmum is
   divided by the number of times the points cross zero. The maximum
   of the measurement is then used as the starting point for 
   findresPart2.

   findresPart2 searches a narrow range of z0. It turns off the lock,
   allows t to settle, then turns it on and looks for "anylock". It repeats
   this over the small range and then picks the midpoint of all the z0
   values that had "anylock". 
 */


#define NUMLOCAL 12
int findresPart2(int start, int startLocked, int power)
{
   int vals[NUMLOCAL+1];
   int dacs[NUMLOCAL+1];
   int i;
   int done;
   int curZ0;
   int anyLk;
   int incr;
   int lockZ0;
   
   incr = 5;
   if (theFindLockObj.largest > 10000)
      incr *= 16;

   lockZ0 = start;
   DPRINT1(debug_autolock,"Findres local start= %d\n",start);
   for (i=0; i <= NUMLOCAL; i++)
      vals[i] = 0;

   i = NUMLOCAL / 2;
   if (startLocked)
   {
      vals[i-1] = 1;
      dacs[i-1] = start;
      curZ0 = start + incr;
      anyLk = 1;
   }
   else
   {
      i--;
      curZ0 = start;
      anyLk = 0;
   }
   done = 0;
   while ( !done && (i < NUMLOCAL) && !host_abort &&  !failAsserted  )
   {
      dacs[i] = curZ0;
      set_find_lock_offset(curZ0);
      setLockMode(LOCK_OFF);
      setLockPower(1);
      Tdelay(POWERDLY);
      setLockPower(power);
      setLockMode(LOCK_ON);
      Tdelay(100);
      if (anylock(100)) 
      {
         vals[i] = 1;
         anyLk = 1;
         DPRINT2(debug_autolock,"found anylock at vals[%d] = %d\n",i,dacs[i]);
         lockZ0 = curZ0;
         done = 1;
      }
      else if (anyLk)
      {
         /* Once lock is found and the moved away, no need to continue */
         done = 1;
      }
      i++;
      curZ0 += incr;
   }
   /* If locked values found upfield, no need to look downfield */
   i = NUMLOCAL / 2;
   if (host_abort || failAsserted)
      return(0);
   if ( ! ( (vals[i-1] == 0) && done ) )
   {
      i -= 2;
      curZ0 = start - incr;
      done = 0;
      while ( !done && (i >= 0) && !host_abort && !failAsserted )
      {
         dacs[i] = curZ0;
         set_find_lock_offset(curZ0);
         setLockMode(LOCK_OFF);             /* turn hardware lock off */
         setLockPower(1);
         Tdelay(POWERDLY);
         setLockPower(power);
         setLockMode(LOCK_ON);             /* turn hardware lock on */
         Tdelay(100);
         if (anylock(100)) 
         {
            vals[i] = 1;
            anyLk = 1;
            DPRINT2(debug_autolock,"found anylock at vals[%d] = %d\n",i,dacs[i]);
            lockZ0 = curZ0;
            done = 1;
         }
         else if (anyLk)
         {
            /* Once lock is found and the moved away, no need to continue */
            done = 1;
         }
         i--;
         curZ0 -= incr;
      }
   }
   if (host_abort || failAsserted)
      return(0);
#ifdef OLDSTYLE
   done = start = anyLk = 0;
   for (i = 0; i <= NUMLOCAL; i++)
   {
      DPRINT2(debug_autolock,"Findres local val[%d] = %d\n", i, vals[i]);
      if (anyLk && (vals[i] == 0) )
      {
          done = i-1;
          i = 13;
      }
      else if ( !anyLk && (vals[i] == 1) )
      {
          start = i;
          anyLk = 1;
      }
   }
#endif
   if (anyLk)
   {
      DPRINT3(debug_autolock,"found anylock anyLk= %d start= %d done= %d\n",
               anyLk,start,done);
#ifdef OLDSTYLE
      if (done)
      {
         curZ0 = ( dacs[done] + dacs[start] ) / 2;
      }
      else
      {
         curZ0 = dacs[start];
      }
#endif
      curZ0 = lockZ0;
      DPRINT1(debug_autolock,"use Z0= %d\n",curZ0);
      set_find_lock_offset(curZ0);
      setLockMode(LOCK_OFF);
      setLockPower(1);
      Tdelay(POWERDLY);
      setLockPower(power);
      setLockMode(LOCK_ON);
      Tdelay(100);
      return(1);
   }
   else
   {
      return(0);
   }
}

#define GLOBALSTEPS 50
int findresPart1(int lkdata[], int lksize)
{
   int startZ0;
   int startZ0tmp;
   int endZ0;
   int stepZ0;
   int iter;
   int index;
   int vals[GLOBALSTEPS];
   int curZ0;
   int i;
   unsigned int re;
   int min, max, max2, sum, best, bestCross;
   int ave;
   int factor;
   int range;

   DPRINT(debug_autolock,"Findres global\n");
   startZ0tmp = startZ0 = theFindLockObj.smallest;
   endZ0 = theFindLockObj.largest;
   stepZ0 = (endZ0 - startZ0) / GLOBALSTEPS;
   DPRINT3(debug_autolock,"Findres global start= %d end= %d step= %d\n", startZ0, endZ0, stepZ0);
   /* First lock FID is sometimes corrupted. Get one and toss it */
   set_find_lock_offset(startZ0);
   find_lock_delay();
   getlkfid(lkdata,SCANS,LK20HZ);
   min = iter = 0;
   while (iter < 3)
   {
      curZ0 = startZ0;
      max = sum = best = 0;
      bestCross = 100;
      for (index=0; index < GLOBALSTEPS; index++)
      {
         int tmp, maxpt;
         int cross, pos, neg, last;
         if (host_abort || failAsserted)
           return(0);
         set_find_lock_offset(curZ0);
         getlkfid(lkdata,SCANS,LK20HZ);
         tmp = maxpt = re = 0;
         last = pos = neg = cross = 0;
         for (i=0; i < lksize/2; i++)
         {
            tmp = lkdata[i*2];
            if (tmp > 0)
            {
               if (tmp > min)
               {
                  pos++;
                  if (last < 0)
                     cross++;
                  last = tmp;
                  re += tmp;
               }
               if (tmp > maxpt)
                  maxpt = tmp;
            }
            else
            {
               tmp = -tmp;
               if (tmp > min)
               {
                  neg++;
                  if (last > 0)
                     cross++;
                  last = -tmp;
                  re += tmp;
               }
               if (tmp > maxpt)
                  maxpt = tmp;
            }
         }
         re = re / 10000;
         vals[index] = re / (cross + 1);
         DPRINT4(debug_autolock,"cross= %d max= %d re= %d val= %d\n", 
                    cross, maxpt, re, vals[index]);
/*
         re = 0;
         for (i=0; i < lksize/2; i++)
         {
            tmp = lkdata[i*2];
            if (tmp < 0)
               re -= tmp;
            else
               re += tmp;
            re += lkdata[i*2];
         }
         if (re < 0)
            vals[index] = -re;
         else
            vals[index] = re;
 */
         sum += vals[index];
         if ( vals[index] > max)
         {
            max =  vals[index];
            best = index;
            bestCross = cross;
            DPRINT2(debug_autolock,"best index= %d max= %d\n", 
                    index, max);
         }
         DPRINT3(debug_autolock,"Findres global vals[%d] = %d at z0= %d\n", index, vals[index], curZ0);
         if ( (bestCross < 3) && (vals[index] < max / 10) )
         {
            int tmp;
            tmp =  vals[index];
            while (index < GLOBALSTEPS)
            {
                vals[index] = tmp;
                index++;
            }
            endZ0 = curZ0;
            tmp = 0;
            while ( (tmp < best) && (vals[tmp] < max / 10) )
            {
               startZ0 += stepZ0;
               tmp++;
            }
         }
         curZ0 += stepZ0;
      }
      if (iter == 0)
      {
         ave = sum / GLOBALSTEPS;
         i = sum = 0;
         for (index=0; index < GLOBALSTEPS; index++)
         {
            if (vals[index] < 3 * ave)
            {
               sum += vals[index];
               i++;
            }
         }
         ave = sum / i;
         DPRINT1(debug_autolock,"Findres average noise %d\n", ave);
         min = ave * 3 / (lksize / 2) ;
      }
      DPRINT3(debug_autolock,"max= %d ave= %d ratio= %d\n",max, ave, max / ave);
      if (max / ave > 8)
      {
         iter += 3;
         factor = 16;
      }
      else if (max / ave > 4)
      {
         iter += 2;
         factor = 8;
      }
      else
      {
         factor = 4;
         iter += 1;
      }
      range = (endZ0 - startZ0) / factor;
      index = best;
      best = startZ0tmp + (index-1)*stepZ0;
      startZ0 = best - range;
      if (startZ0 < theFindLockObj.smallest)
         startZ0 = theFindLockObj.smallest;
      startZ0tmp = startZ0;
      endZ0 = best + range;
      if (endZ0 > theFindLockObj.largest)
         endZ0 = theFindLockObj.largest;
      stepZ0 = (endZ0 - startZ0) / GLOBALSTEPS;
      DPRINT1(debug_autolock,"Findres2 reduction factor= %d\n", factor);
      DPRINT3(debug_autolock,"Findres2 global start= %d end= %d step= %d\n", startZ0, endZ0, stepZ0);
   }

/* Next phase */
      curZ0 = startZ0;
      stepZ0 = (endZ0 - startZ0) / 25;
      max2 = sum = best = 0;
      min = ave * 3 / (lksize / 2) ;
      DPRINT1(debug_autolock,"Findres min= %d\n", min);
      for (index=0; index < 25; index++)
      {
         int tmp, cross, pos, neg, last;
         if (host_abort || failAsserted)
           return(0);
         set_find_lock_offset(curZ0);
         Tdelay(100);
         getlkfid(lkdata,SCANS,LK20HZ);
         tmp = max = re = 0;
         last = pos = neg = cross = 0;
         for (i=0; i < lksize/2; i++)
         {
            tmp = lkdata[i*2];
            if (tmp > 0)
            {
               if (tmp > min)
               {
                  pos++;
                  if (last < 0)
                     cross++;
                  last = tmp;
                  re += tmp;
               }
               if (tmp > max)
                  max = tmp;
            }
            else
            {
               tmp = -tmp;
               if (tmp > min)
               {
                  neg++;
                  if (last > 0)
                     cross++;
                  last = -tmp;
                  re += tmp;
               }
               if (tmp > max)
                  max = tmp;
            }
         }
         re = re / 10000;
         vals[index] = re / (cross + 1);
         DPRINT3(debug_autolock,"Findres2 vals[%d]= %d at z0= %d\n",
                  index, vals[index], curZ0);
         DPRINT4(debug_autolock,"cross= %d max= %d re= %d val= %d\n", 
                    cross, max, re, vals[index]);

         if (vals[index] > max2)
         {
            max2 = vals[index];
            best = curZ0;
            DPRINT2(debug_autolock,"Findres2 best= %d at z0= %d\n", vals[index], curZ0);
         }
         curZ0 += stepZ0;
      }
   return(best);
}

static void oneLastTry(int initpwr, int initgain, int maxpwr, int maxgain)
{
   int level;
   int curgain;

   updateAcqState(ACQ_APOWER);
   DPRINT4(debug_autolock,"OneLastTry initpwr= %d initgain= %d maxpwr= %d maxgain= %d\n",
       initpwr, initgain, maxpwr, maxgain);
   setLockMode(LOCK_OFF);
   setLockPower(1);
   Tdelay(POWERDLY);
   setLockPower(maxpwr);
   setLockGain(maxgain);
   setLockMode(LOCK_ON);
   level = getLockLevel();
   DPRINT3(debug_autolock,"initpower = %d power= %d level= %d\n",
              initpwr, maxpwr, level);
   while ((maxpwr > initpwr) && !host_abort && !failAsserted)
   {
      maxpwr -= 3;
      if (maxpwr < initpwr)
         maxpwr = initpwr;
      setLockPower(maxpwr);
      Tdelay(100);
      level = getLockLevel();
      DPRINT3(debug_autolock,"initpower = %d power= %d level= %d\n",
              initpwr, maxpwr, level);
   }
   curgain = maxgain;
   updateAcqState(ACQ_AGAIN);
   while ( (curgain > initgain) && chlock()  &&
            !host_abort && !failAsserted )
   {
      curgain--;
      setLockGain(curgain);
      Tdelay(GAINDLY / 2);
      if (!chlock())
      {
         curgain += 3;
         if (curgain > maxgain)
            curgain =  maxgain;
         setLockGain(curgain);
         Tdelay(GAINDLY / 2);
         /* use this to exit the while loop */
         curgain = initgain - 6;
      }
      level = getLockLevel();
      DPRINT3(debug_autolock,"initgain = %d gain= %d level= %d\n",
              initgain, curgain, level);
   }
}

/*--------------------------------------------------------------------------------------*/
/*  fuzzy_autolock(mode,maxpwr,maxgain)  --- fuzzy  control		  				*/
/*   1. mode is used to control the adjustment of lock power, gain, and phase */	
/*   2. maximum values for lock power and gain		*/
/*--------------------------------------------------------------------------------------*/
int fuzzy_autolock(int mode,int maxpwr, int maxgain, int sampleHasChanged)
{
#ifdef PUNT_FOR_NOW
   extern int adjLkGainforShim;
#endif
   int initpwr, initphas, initgain; 
   int initmod, initfreq, initlevel; 
   int doit, count; 
   long level, spr, max_spr;	/* 512 data points max 20 hz mode */
   int gain, power;
   int lowest_pwr;
   int i;
   /* int lkflag, lowest_pwr, lkcount; */
   int lksize, freq, phase, ph, lkdata[FIDSIZ*2];
   int *st_data, *datap;
   int currentZ0;
   int lkcount;
   int lkflag;
   int ival;


   lksize = FIDSIZ - LOCKOFFSET;
   datap = &lkdata[LOCKOFFSET];

   /* set global parameters    */

   g_maxpwr = maxpwr;
   g_maxgain = maxgain;
   g_mode = mode; 


 /* save initial lock parameter settings */

   BestPower = initpwr = getLockPower(); 
   BestGain = initgain = getLockGain();
   DPRINT2(debug_autolock,"Initial gain = %d power= %d\n", BestGain, BestPower);
   BestPhase = initphas = getLockPhase();
   BestZ0 = get_find_lock_offset();
   initmod = getLockMode();
   spr = 0;

   lk2kcf();			/* reset filter to 2kHz fast filter */

   DPRINT(debug_autolock,"Beginning: AUTOLOCK\n");
     
  setLockMode(LOCK_ON); 
  Tdelay(PHASEDLY);

/*   DPRINT1(-1,"LOCKGAI = %d\n", maxgain);
     DPRINT1(-1,"LOCKGAI = %d\n", BestGain);
*/

  lkflag = 0;
  if  (g_mode == 6)
  {
    if (locked(100)) 
    {
       DPRINT(debug_autolock,"Simple lock capture worked\n");
       return(0);
    }
    else
    {
        setLockMode(LOCK_OFF);             /* turn hardware lock off */
        setLockPower(1);
        Tdelay(POWERDLY);
	power = initpwr +12;
	if (power > maxpwr) power = maxpwr;
        setLockPower(power);
	setLockGain(maxgain);
        setLockMode(LOCK_ON);             /* turn hardware lock on */
        Tdelay(100);
        DPRINT(debug_autolock,"recycle check lock\n");
        if (anylock(100)) 
        {
           DPRINT(debug_autolock,"recycle lock found\n");
           setLockPower(initpwr);
           Tdelay(POWERDLY);
	   setLockGain(initgain);
           if (locked(100))
           {
              return(0);
           }
/*
	   setLockGain(maxgain);
           if (findresPart2(BestZ0, 1, power))
           {
              setLockPower(initpwr+6);
              Tdelay(POWERDLY);
              setLockPower(initpwr);
              Tdelay(POWERDLY);
	      setLockGain(initgain);
           }
 */
           oneLastTry(initpwr, initgain, power, maxgain);
           if (locked(100))
           {
              return(0);
           }
           else
           {
	      set_find_lock_offset(BestZ0); 
              setLockPower(initpwr);
	      setLockGain(initgain);
	      return(ALKERROR + ALKRESFAIL);	      
           }
        }
        if (host_abort || failAsserted)
        {
	   set_find_lock_offset(BestZ0); 
           setLockPower(initpwr);
	   setLockGain(initgain);
	   return(ALKERROR + ALKRESFAIL);	      
        }
        updateAcqState(ACQ_AFINDRES); 
        setLockMode(LOCK_OFF);             /* turn hardware lock off */
        setLockPower(power);
	setLockGain(maxgain);
        lk20Hz();		/* lock set to 20Hz with fast filter */
        currentZ0 = findresPart1(lkdata,lksize);
        if (host_abort || failAsserted)
        {
	   set_find_lock_offset(BestZ0); 
           setLockPower(initpwr);
	   setLockGain(initgain);
	   return(ALKERROR + ALKRESFAIL);	      
        }
        lk2kcf();
        if (findresPart2(currentZ0, 0, power))
        {
           setLockPower(initpwr+6);
           Tdelay(POWERDLY);
           setLockPower(initpwr);
           Tdelay(POWERDLY);
	   setLockGain(initgain);
        }
        else
        {
	   set_find_lock_offset(BestZ0); 
           setLockPower(initpwr);
	   setLockGain(initgain);
        }
        if (host_abort || failAsserted)
        {
	   return(ALKERROR + ALKRESFAIL);	      
        }
        if (locked(100))
        {
           return(0);
        }
        oneLastTry(initpwr, initgain, power, maxgain);
        if (locked(100))
        {
           return(0);
        }
        else
        {
	   set_find_lock_offset(BestZ0); 
           setLockPower(initpwr);
	   setLockGain(initgain);
	   return(ALKERROR + ALKRESFAIL);	      
        }
    }
  }
   
/*-----------------------------------------------------------------*/
/*	 	Try to find good FID signal 			   */
/*-----------------------------------------------------------------*/

  getlkfid(lkdata,SCANS,LK20HZ);
/*  st_data = datap;
/*  for(i=0; i<30; i++)
/*  { DPRINT2(-1,"r=%d i=%d\n",*st_data, *(st_data+1) );
/*    st_data += 2;
/*  }
/* */
  xch_analyz_1(datap,lksize, &initlevel, &initfreq, &phase, &spr);


  /* replace the if, else  with the ival contruct to avoid a compiler bug 
    autolock.c: In function `fuzzy_autolock':
    > >>> autolock.c:1241: Internal compiler error:
    > >>> autolock.c:1241: Could not split insn
    also compile at no optimization or -O1, -O3 also worked, failed
    with -O2 without the contruct fix.

     Greg Brissey   1/6/2005
  */
  /* Warning the following  ival, if construct though odd is necessary to avoid the error above */
  ival = ((lkflag == 0) && ( locked(100) ) && (initfreq == 0) && 
          (initlevel >= LK20p_LEVEL) );
  if (ival)
  {
	lkflag = TRUE;
  }
  if (!ival && !host_abort && !failAsserted)
  {    
    for (lkcount = 0; ((lkflag == 0) && (lkcount < 3)); lkcount++)
    { 
     DPRINT(debug_autolock,"finding FID...\n");     
     updateAcqState(ACQ_AFINDRES); 

     setLockMode(LOCK_OFF);
     power = getLockPower();
     gain = getLockGain();
     if (g_mode == 6) 
     { 
        power += 12;
        /* power = (power > g_maxpwr) ? g_maxpwr : power; */
        if (power > g_maxpwr) 
           power = g_maxpwr; 
     }
     else 
     {
        if (power < g_maxpwr) 
           power = g_maxpwr;
     }
     setLockPower(power);     
     if (gain < g_maxgain) 
        setLockGain(g_maxgain);
     Tdelay(POWERDLY);
     lowest_pwr = power;

     getlkfid(lkdata,SCANS,LK20HZ);
     xch_analyz_1(datap,lksize, &level, &freq, &phase, &spr);

     if ( (freq > 12 )|| (spr < 5000) )
     {
         if ( !findfid(lkdata, mode) )
	 { 
	    setLockMode(initmod); 
            setBestLock();
	    return(ALKERROR + ALKRESFAIL);	      
         }
         else 
         {
            getBestLock();
         }
     }
     else if (freq == 0)
     {
	if (g_mode == 6) 
        {
           setLockPower(power-6); 
           lowest_pwr = power-6;
	   Tdelay(POWERDLY);
        }
	else if ( (BestPower+5 > MINPWR) && (spr > 20000) ) 
	{
           setLockPower(BestPower+5); 
           lowest_pwr = BestPower+5;
	   Tdelay(POWERDLY); 
	}
	lkflag = 2;
     }
     

     if (lkflag == 0 )
     {
    	 getlkfid(lkdata,SCANS,LK20HZ);
    	 xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	 max_spr = spr;
         count = 0;
	 power = getLockPower();
	 lowest_pwr = power;
	 while ( (freq >= 1) && (count < 3) && !host_abort )
         {   count++;
             Z0_adj1(lkdata, &level, &freq, &phase, &spr);
	     if (max_spr < spr) 
                max_spr = spr;

	     if ( (count > 1) && (freq > 0) && (freq < 2) && 
                  (g_mode !=3) && (g_mode != 4) && (g_mode != 6) )
             {
	   	phase_adj1(lkdata, &level, &freq, &phase, &spr);
             }

	     if ((count > 1) && (freq > 0) && (freq < 3) &&  (spr > 15000))
	     { 
		  if ((g_mode==6) && ((power - count*5) < initpwr) )
                  {  
                     setLockPower(initpwr);
                     lowest_pwr = initpwr;
                  }
		  else 
                  {
                     setLockPower(power - count*5);
                     lowest_pwr = power - count*5;
                  }
		  Tdelay(POWERDLY); 
	 	  
   		  getlkfid(lkdata,SCANS,LK20HZ);
    		  xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
		  if (max_spr < spr) 
                     max_spr = spr;
	    }
	 } 
	
	 if (freq == 0) 
         {
            lkflag = 2; 
            getBestLock();
         }
	 else if ( (freq > 0) && (freq < 3) && (spr > 15000) )
	 {  
	     if ( (g_mode==6) && ((power - count*5) < initpwr) )
             {
                setLockPower(initpwr);
                lowest_pwr = initpwr;
             }
	     else 
             {
                setLockPower(power - count*5); 
                lowest_pwr = power - count*5;
             }
	     Tdelay(POWERDLY); 

   	     getlkfid(lkdata,SCANS,LK20HZ);
    	     xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	     max_spr = (max_spr < spr) ? spr :  max_spr;
	     if (freq == 0) 
             {
                lkflag = 2; 
                getBestLock();
             }
	 }

 

	 level = getLockLevel();

	 if ( (freq < 2) && (level < LK20p_LEVEL)) 
            doit = TRUE;
	 else 
            doit = FALSE;

	 count = 0;
	 while (!host_abort && doit && (count < 2) && (g_mode !=3) &&
                (g_mode != 4) && (g_mode != 6) && !host_abort) 
 	 {
	        count++;
		phase_adj1(lkdata, &level, &freq, &phase, &spr);
	        if ( level >= LK20p_LEVEL) {doit = FALSE; BestPhase = getLockPhase();}
		if (max_spr < spr) max_spr = spr;
		if (freq == 0) 
                {
                   lkflag = 2; 
                   getBestLock();
                }
	 }
  	
	
/*-----------------------------------------------------------------*/
/*	 	stability test and adjustment 			   */
/*  if level is not stable  -- reduce  power   			   */
/*-----------------------------------------------------------------*/
	
    if ( (g_mode !=6) && (max_spr > 10000) && !host_abort)
      {
	power_stable_adj1(lkdata, level);
        BestPower = getLockPower();
	lowest_pwr = BestPower;
    	getlkfid(lkdata,SCANS,LK20HZ);
    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	if (freq == 0) 
        {
          lkflag = 2; 
          getBestLock(); 
        }
      }


/*--------------------------------------------------------------------------------------*/
/*	 	signal level test and adjust all before set lock   			*/
/*											*/
/*  if level is high									*/ 
/*		-- adjust power/gain, then adjust Z0/phase, 			   	*/
/*  if level is medium				 					*/
/*     		-- adjust Z0/phase			   				*/
/*  if level is low and power/gain is medium/low					*/
/*     		-- increase power/gain, adjust Z0/phase, 			  	*/
/*--------------------------------------------------------------------------------------*/


        if (lkflag !=2 )
        {
    	  getlkfid(lkdata,SCANS,LK20HZ);
    	  xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
          Z0_adj1(lkdata, &level, &freq, &phase, &spr);
	  if (max_spr < spr) max_spr = spr;
	  if (freq == 0) 
          {
             lkflag = 2; 
             BestZ0 = get_find_lock_offset();
          }
          level = getLockLevel();
          if (freq < 2 && level < LK20p_LEVEL && g_mode !=3 && g_mode != 4 && g_mode != 6) 
 	  { 
             phase_adj1(lkdata, &level, &freq, &phase, &spr);
	     if (freq == 0) 
             {
	       lkflag = 2; 
               BestZ0 = get_find_lock_offset(); 
               BestPhase = getLockPhase();
             }
	   }
         }
    
        level = getLockLevel();

        if ( g_mode !=6 && freq ==0 && abs(level) > LK60p_LEVEL )
        {
	  power_adj1(lkdata);
	  lowest_pwr = getLockPower();
        }
        else  
        { gain = getLockGain();
          if (gain < g_maxgain) { setLockGain(g_maxgain);  Tdelay(GAINDLY); }
          getlkfid(lkdata,SCANS,LK20HZ);
    	  xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	  if (max_spr < spr) max_spr = spr;
/*
	  if ( g_mode !=6 && spr > 10000) 
		{power_stable_adj2(spr);
	 	 lowest_pwr = getLockPower();
		}
*/
         }
 
 	 currentZ0 = get_find_lock_offset(); 
	 initfreq = freq;

	 if (freq > 0) Z0_adj2(lkdata, mode);
 	 Z0_adj3(lkdata, mode);
    	 getlkfid(lkdata,SCANS,LK20HZ);
     	 xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	 if (max_spr < spr) max_spr = spr;

 	 if (freq > initfreq ) 
	 {
	     if (lkflag == 2)	set_find_lock_offset(BestZ0); 
	     else	      set_find_lock_offset(currentZ0);
	     find_lock_delay();
	 }

	 level = getLockLevel();
	 if (level < LK20p_LEVEL && g_mode !=3 && g_mode != 4 && g_mode != 6) 
          	    phase_adj1(lkdata, &level, &freq, &phase, &spr);
	 

      }  /*  end of  if lkflag == 0   */
 


/*--------------------------------------------------------------------------------------*/
/*	 	lock test and stability test 					   	*/
/*   if lock signal not found (locked(100) fails 					*/
/* 		-- increase power/gain and check lock again			 	*/
/*		-- if still not locked, increase power/gain				*/
/*		     and adjust Z0 adjust phase						*/
/*   if lock signal found and lock level not stable	    				*/
/* 		-- need to reduce power						 	*/
/*   if lock signal found and lock level change (before and after lock) is big		*/
/* 		-- need to adjust phase				 			*/
/*   if locked signal found stable and lock level change  is not big   			*/
/* 		-- find resonace , final reduce power to lock level less than 90%	*/
/*--------------------------------------------------------------------------------------*/


    initlevel = getLockLevel();

    setLockMode(LOCK_ON);
    Tdelay(POWERDLY);
    level = getLockLevel();

    if ( ( (initlevel - level) > LK5p_LEVEL && spr > 10000) || 
         ( (initlevel - level) > LK3p_LEVEL && spr < 12000 ) ) 
      {
 	  phase = getLockPhase();
  	  phase_adj2(lkdata, 20);
	  ph = getLockPhase();
   	  level = getLockLevel();
 	  if ( !locked(100) )
	    {
	    	setLockMode(LOCK_OFF);
		currentZ0 = get_find_lock_offset();

		set_find_lock_offset(currentZ0 - 20);
		DPRINT1( debug_autolock, "in fuzzy autolock, reduced Z0 by 20 to %d\n", currentZ0 - 20 );
	    	find_lock_delay();
        	getlkfid(lkdata,SCANS,LK20HZ);
        	xch_analyz(datap, lksize, &level, &freq, &phase, &spr);
		count = 0;
		while (!host_abort && (count < 2) && (freq >= 1) )
			{ count++; Z0_adj1(lkdata, &level, &freq, &phase, &spr); }

		level = getLockLevel();
	 	if ( level < LK20p_LEVEL && freq < 2 && g_mode !=3 && g_mode != 4 && g_mode != 6) 
 		   {
			phase_adj1(lkdata, &level, &freq, &phase, &spr);
		   }

 	 	Z0_adj3(lkdata, mode);

 	 	if ( g_mode !=3 && g_mode != 4 && g_mode != 6) 
 		   {
	            if (spr < 125000 ) ph = 10;
		    else ph = 20;
	 	    phase_adj2(lkdata, ph);
		   }

	    	setLockMode(LOCK_ON);
	    	Tdelay(POWERDLY);
	     }

      }


    if (locked(100))
    {
      lkflag = TRUE;
    }
   else
    {
      Tdelay(POWERDLY);
      if (locked(100))
       {
        lkflag = TRUE;
       }
      else
       {
	gain = getLockGain();
     	if (gain < g_maxgain) setLockGain(g_maxgain);

	power = getLockPower();
        if (power < g_maxpwr)
 	  {
	  if (power + 8 < 3*g_maxpwr/5) power = 3*g_maxpwr/5;
	  else if (power + 8 > g_maxpwr-3) power = g_maxpwr - 3;
	  else power += 8;
	  setLockPower (power);
	  Tdelay(POWERDLY);
	  }
        else if ( gain < g_maxgain) Tdelay(GAINDLY);

        if (!locked(100))
             {
	    	setLockMode(LOCK_OFF);

		currentZ0 = get_find_lock_offset();
		set_find_lock_offset(currentZ0 - 30);
		DPRINT1( debug_autolock, "in fuzzy autolock, reduced Z0 by 30 to %d\n", currentZ0 - 30 );
	    	Tdelay(PHASEDLY);

	    	getlkfid(lkdata,SCANS,LK20HZ);
	    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	        if (freq >= 1)   Z0_adj1(lkdata, &level, &freq, &phase, &spr);
		if (freq > 0) Z0_adj2(lkdata, mode);
		Z0_adj3(lkdata, mode);

	    	getlkfid(lkdata,SCANS,LK20HZ);
	    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
		level = getLockLevel();
	 	if ( freq > 0 && freq < 2 && level < LK20p_LEVEL && g_mode !=3 && g_mode != 4 && g_mode != 6) 
 		   {
			phase_adj1(lkdata, &level, &freq, &phase, &spr);
		   }

 	 	if ( g_mode !=3 && g_mode != 4 && g_mode != 6) 
 		   {
	            if (spr < 125000 ) ph = 10;
		    else ph = 20;
	 	    phase_adj2(lkdata, ph);
		   }

	    	setLockMode(LOCK_ON);
	    	Tdelay(POWERDLY);

	    }

 	if (locked(100)) 
        {
           lkflag = TRUE;
        }
	else  
        {
           lkflag = FALSE;
        }

       }
     }


    getlkfid(lkdata,SCANS,LK20HZ);
    xch_analyz(datap, lksize, &level, &freq, &phase, &spr);

    if (freq > 0 && spr > 8000) 
       {
		lkflag = FALSE;
	    	setLockMode(LOCK_OFF);
		currentZ0 = get_find_lock_offset();

		set_find_lock_offset(currentZ0 - 10);
		DPRINT1( debug_autolock, "in fuzzy autolock, reduced Z0 by 10 to %d\n", currentZ0 - 10 );
	    	find_lock_delay();
        	getlkfid(lkdata,SCANS,LK20HZ);
        	xch_analyz(datap, lksize, &level, &freq, &phase, &spr);
		count = 0;
		while (!host_abort && (count < 2) && (freq >= 1))  
		{ count++; Z0_adj1(lkdata, &level, &freq, &phase, &spr); }

		level = getLockLevel();
	 	if ( level < LK20p_LEVEL && freq < 2 && g_mode !=3 && g_mode != 4 && g_mode != 6) 
 		   {
			phase_adj1(lkdata, &level, &freq, &phase, &spr);
		   }

		if (freq > 0 ) Z0_adj2(lkdata, mode);

 	 	Z0_adj3(lkdata, mode);

 	 	if ( g_mode !=3 && g_mode != 4 && g_mode != 6) 
 		   {
	            if (spr < 125000 ) ph = 10;
		    else ph = 20;
	 	    phase_adj2(lkdata, ph);
		   }

	    	setLockMode(LOCK_ON);
	    	Tdelay(POWERDLY);
		getlkfid(lkdata,SCANS,LK20HZ);
    		xch_analyz(datap, lksize, &level, &freq, &phase, &spr);

	      if (locked(100) && freq == 0)
              {
                 lkflag = TRUE;
              }
	      else 
	      {
		set_find_lock_offset(currentZ0);
		DPRINT1( debug_autolock, "in fuzzy autolock, set Z0 to %d\n", currentZ0 );
	    	find_lock_delay();
		if (locked(100))
                {
                 lkflag = TRUE;
                }
	  	else 
                {
                 lkflag = FALSE;
                }
	      }
       }

   } /* end of while lkcount < 3 */
 } /*end of else*/





 if (!lkflag || host_abort) return(ALKERROR + ALKRESFAIL);	

 getBestLock();

 
 if (g_mode == 6) 	/* slowly reset power to initial value */
  {
	  power = getLockPower();
	  while (power>initpwr) 
		{ power -= 6;
		  if (power<initpwr) power=initpwr;
          	  setLockPower(power);
	  	  Tdelay(POWERDLY);
		}
	  if (!locked(100)) {setLockPower(BestPower); Tdelay(POWERDLY);}

	  setLockGain(initgain);
	  Tdelay(PHASEDLY);
	  if (!locked(100)) {setLockGain(BestGain); Tdelay(PHASEDLY);}

/*
          if (spr < 125000 ) ph = 10;
 	  else ph = 20;
	  phase_adj2(lkdata, ph);
	  if (!locked(100)) {setLockPhase(BestPhase); Tdelay(PHASEDLY);}
*/
#ifdef PUNT_FOR_NOW
          adjLkGainforShim = 1;
#endif

        if (locked(100)) return(0);
        else
    	  { 
    	   Tdelay(POWERDLY);
    	   if (!locked(100))
            { setLockPower(initpwr);  /* reset power to initial value */
	      setLockGain(initgain);
	      setLockPhase(initphas);
#ifdef PUNT_FOR_NOW
              adjLkGainforShim = 1;
#endif
	      DPRINT(0,"Auto-power-phase Failed\n");
	      return(ALKERROR + ALKPOWERFAIL);   
	    }
    	  } 

  }
 else
  {
 
   getlkfid(lkdata,SCANS,LK20HZ);
   xch_analyz(datap,lksize, &level, &freq, &phase, &spr);

   power = getLockPower();
   if (spr > 14000 && power - 10 > lowest_pwr) 
	{
	 setLockPower(power - 8);
	 Tdelay(POWERDLY);

   	 if (!locked(100)) 
	  {
	   setLockPower(power - 5);	 
	   Tdelay(POWERDLY);
   	   if (!locked(100)) 
	    {
	     setLockPower(power - 5);	 
	     Tdelay(POWERDLY);
 	    }
           else getBestLock();
	  }
         else getBestLock();
	}

/*
   if (spr > 8000 )
    {  
     power_stable_adj2(spr);
     if (!locked(100)) 
	{
   	  setLockPower(BestPower);
     	  setLockGain(BestGain);
 	  setLockPhase(BestPhase);
	  Tdelay(POWERDLY);
	}
      else getBestLock();
     }

*/

   if (spr > 10000)
    {
     power_adj2(lkdata);
     if (!locked(100)) 
	{
   	  setLockPower(BestPower);
     	  setLockGain(BestGain);
 	  setLockPhase(BestPhase);
	  Tdelay(POWERDLY);
	}
     else getBestLock();
    }


/*-----------------------------------------------------------------*/
/*   	finial power, gain & phase adjustment			   */
/*-----------------------------------------------------------------*/


     if (spr > 5500 ) 
	{
	 power_gain_adj(lkdata);
	 if (!locked(100)) 
	   {
	    setLockPower(BestPower);
	    setLockPhase(BestPhase);
	    Tdelay(POWERDLY);
	   }
         else getBestLock();
        }
     else
        {
	   gain = getLockGain();
	   doit = TRUE;
   	   level = getLockLevel(); 
	   count = 0;
   	   while(!host_abort && (level > LK72p_LEVEL || level < LK30p_LEVEL) &&
                 doit && count < 4 )
   	     { count++; 
               updateAcqState(ACQ_AGAIN);
	       adjust_gain(&gain,&doit,level,LK52p_LEVEL);
	       level = getLockLevel();
   	     }
         }

	if ( g_mode !=3 && g_mode != 4 && max_spr > 14000)
	 {
	   if (spr < 14000 ) ph = 10;
	   else ph = 20;
 	   phase_adj2(lkdata, ph);
	
	   gain = getLockGain();
	   doit = TRUE;
   	   level = getLockLevel(); 
	   count = 0;

   	   while(!host_abort && (level > LK72p_LEVEL || level < LK30p_LEVEL) &&
                 doit && count < 4 )
   	     {  count++;
               updateAcqState(ACQ_AGAIN);
	       adjust_gain(&gain,&doit,level,LK52p_LEVEL);
	       level = getLockLevel();
   	     }

	   if (!locked(100)) 
	    {
	    setLockGain(BestGain);
 	    setLockPhase(BestPhase);
	    Tdelay(80);
	    }
           else getBestLock();
	 }


     if (locked(100)) 
        return(0);
     else
      {
    	getlkfid(lkdata,SCANS,LK20HZ);
    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);

	lkflag = FALSE;
	count = 0;
 	while (!host_abort && !lkflag && count <3)
 	 {
 	  count++;

	  gain = getLockGain();
     	  if (gain < g_maxgain) setLockGain(g_maxgain);

	  power = getLockPower();
          if (power < g_maxpwr)
 	   {
	    if (power + 5 < g_maxpwr)  setLockPower (power+5);
	    else setLockPower(g_maxpwr);
	    Tdelay(POWERDLY);
	   }
          else if ( gain < g_maxgain) Tdelay(GAINDLY);

          if (locked(100)) lkflag = TRUE;
  	  else
             {
	    	setLockMode(LOCK_OFF);
	    	Tdelay(PHASEDLY);
	    	getlkfid(lkdata,SCANS,LK20HZ);
	    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	        if (freq >= 1 )   Z0_adj1(lkdata, &level, &freq, &phase, &spr);
	        if (freq > 0) Z0_adj2(lkdata, 3); 
		Z0_adj3(lkdata, mode);

		level = getLockLevel();
	 	if ( level < LK20p_LEVEL && g_mode !=3 && g_mode != 4) 
 		   {
			phase_adj1(lkdata, &level, &freq, &phase, &spr);
		   }

 	 	if ( g_mode !=3 && g_mode != 4) 
 		   {
	            if (spr < 125000 ) ph = 10;
		    else ph = 20;
	 	    phase_adj2(lkdata, ph);
		   }

	    	setLockMode(LOCK_ON);
	    	Tdelay(2*POWERDLY);
	 	
		if (locked(100)) lkflag = TRUE;
		else { phase_adj2(lkdata,15);
			Tdelay (2*PHASEDLY);
			if (locked(100)) lkflag = TRUE;
		     }
             }

	 } /*end of while !lkflag && count<3 */


	if(lkflag)
	 {
 	  getlkfid(lkdata,SCANS,LK20HZ);
 	  xch_analyz(datap, lksize, &level, &freq, &phase, &spr);
	  getBestLock();
          if (spr > 5500 ) 
	   {
  	    power_gain_adj(lkdata);
	    if (!locked(100)) 
	     {
	      setLockPower(BestPower);
	      setLockPhase(BestPhase);
	      Tdelay(POWERDLY);

	     }
            else getBestLock();
           }
            
 
	   gain = getLockGain();
	   doit = TRUE;
      	   level = getLockLevel(); 
	   count = 0;
    	   while((level > LK72p_LEVEL || level < LK30p_LEVEL) && doit && count < 4 )
   	     { count++; 
               updateAcqState(ACQ_AGAIN);
	       adjust_gain(&gain,&doit,level,LK52p_LEVEL);
	       level = getLockLevel();
   	     }
 	 }

        if (locked(100)) return(0);
        else
    	  { 
    	   Tdelay(POWERDLY);
    	   if (!locked(100))
            { setLockPower(initpwr);  /* reset power to initial value */
	      setLockGain(initgain);
	      setLockPhase(initphas);
#ifdef PUNT_FOR_NOW
              adjLkGainforShim = 1;
#endif
	      DPRINT(debug_autolock,"Auto-power-phase Failed\n");
	      return(ALKERROR + ALKPOWERFAIL);   
	    }
    	  } 
      }
  }


  return(0);

}   /* end of autolock */


/*----------------------------------------------------------------------*/
/* 	findfid(data,mode) 						*/
/*		- try to get best SNR for the FID signal		*/
/*			   by shifting the Z0 field starting from z0)	*/
/*	1. Offset Z0 to off resonance sigal for noise level		*/
/*	2. find a Z0 with good snr..		   			*/
/*	3. find the  Z0 with best snr.. 				*/
/*									*/
/*----------------------------------------------------------------------*/
#define SCANZ0	21

int findfid(int *lkdata, int mode)
{
  long THR = 3L, STD = 4000;
  int lksize;
  int *st_data;
  int i,distance, diff, di,ik, ok, count;
  int limitZ0, startZ0;
  int currentZ0;
  int bestval, s_bestval;
  long totnoise,snr,bestsnr, s_bestsnr, level, spr;
  int phs, freq, bfreq, s_bfreq, phase, initfreq;
  float unit_Z0;

/* initial variables */

  if (host_abort)
    return(0);
  /*startZ0 = 0;*/
  startZ0 = (theFindLockObj.largest + theFindLockObj.smallest) / 2;
  DPRINT2( debug_autolock, "in findfid, startZ0 = %d (0x%x)\n", startZ0, startZ0 );
  bestsnr = 0L;
  lksize = FIDSIZ - LOCKOFFSET;
  st_data = &lkdata[LOCKOFFSET];

  lk20Hz();		/* lock set to 20Hz with fast filter */

  /*limitZ0 = dac_limit();*/
  limitZ0 = (theFindLockObj.largest - theFindLockObj.smallest) / 2;
  DPRINT1( debug_autolock, "in findfid, limitZ0 = %d\n", limitZ0 );

  set_find_lock_offset(startZ0-8*limitZ0/SCANZ0);
  find_lock_delay();
  getlkfid(lkdata,SCANS,LK20HZ);
  xch_analyz_1(st_data,lksize, &level, &freq, &phs, &totnoise);
  DPRINT3(debug_autolock,
      "in findfid, pt1, Z0=%d freq=%d snr=%d\n",
      startZ0-8*limitZ0/SCANZ0,freq,totnoise );

  set_find_lock_offset(startZ0+8*limitZ0/SCANZ0);
  find_lock_delay();
  getlkfid(lkdata,SCANS,LK20HZ);
  xch_analyz_1(st_data,lksize, &level, &freq, &phs, &snr);
  if (totnoise>snr) totnoise=snr;
  DPRINT3(debug_autolock,
      "in findfid, pt2, Z0=%d freq=%d snr=%d\n",
      startZ0+8*limitZ0/SCANZ0,freq,snr );

  set_find_lock_offset(startZ0);
  find_lock_delay();
  getlkfid(lkdata,SCANS,LK20HZ);
  xch_analyz_1(st_data,lksize, &level, &freq, &phs, &snr);
  if (totnoise>snr) totnoise=snr;
  DPRINT3(debug_autolock,
      "in findfid, pt3, Z0=%d freq=%d snr=%d\n",
      startZ0,freq,snr );

  if (totnoise == 0) totnoise = 1;


 count = 0;
 ok = FALSE;

while (!ok && (count < 4) && !host_abort)
 {
  if (count <2) distance = 2*limitZ0 /(SCANZ0-1);
  else distance = 4*limitZ0 / (3*SCANZ0-3);

  if (count == 1 || count == 3)
    {
     startZ0 += distance/2; 
    }

  if (count == 2 && bestsnr > 5000) {setLockPower(9*g_maxpwr/10); Tdelay(POWERDLY);}
  else if ( count == 3 && bestsnr > 5000 ) {setLockPower(8*g_maxpwr/10);Tdelay(POWERDLY);}

  if(count!=0)
   {
    set_find_lock_offset(startZ0);
    find_lock_delay();
    getlkfid(lkdata,SCANS,LK20HZ);
    xch_analyz_1(st_data,lksize, &level, &freq, &phs, &snr);
    DPRINT3(debug_autolock,
      "in findfid, pt4, Z0=%d freq=%d snr=%d\n",
      startZ0,freq,snr );
   }

  count++;

  bestval = startZ0; 
  bfreq = freq;
  initfreq = freq; 
  s_bestsnr = 0L;

  i = 1;
  di = 0;


  if (bestsnr/totnoise >= THR && bestsnr >= STD) 
    { 
	 if (freq <= 12 && snr > STD) ok = TRUE;
    }


 while (!ok && !host_abort)  /*----find bestsnr-----*/
 {
    DPRINT2( debug_autolock, "in findfid with di = %d and i = %d\n", di, i );
    if(di >= 0 || i == 1)  
    { 
     currentZ0 = startZ0 + i*distance; 
     DPRINT2( debug_autolock, "in findfid, comparing %d with %d at point 1\n",
		      currentZ0, theFindLockObj.largest);
     if (currentZ0 <= theFindLockObj.largest)
     {
        set_find_lock_offset(currentZ0);
        find_lock_delay();
        getlkfid(lkdata,SCANS,LK20HZ);
        xch_analyz_1(st_data,lksize, &level, &freq, &phs, &snr);
  	if (totnoise>snr) totnoise=snr;
        DPRINT3(debug_autolock,
            "in findfid, resonance search, Z0=%d freq=%d snr=%d\n",
            currentZ0,freq,snr );
        DPRINT1(debug_autolock, "bestsnr=%d\n",bestsnr);
        if (bestsnr<snr) 
          {
	    s_bestsnr = bestsnr;
	    s_bestval = bestval;
	    s_bfreq = bfreq;
            bestsnr = snr;
	    bestval = currentZ0;
	    bfreq = freq;
            if (snr/totnoise >= THR && snr >= STD/2 && freq < 30) di = 1;
          }
	else 
	  {
           if (s_bestsnr < snr) 
            {
	    s_bestsnr =  snr;
	    s_bestval = currentZ0;
	    s_bfreq = freq;
            } 
          if   (bestsnr/totnoise >= THR && bestsnr >= STD && bfreq < 20) 
	     {
		ok = TRUE;
	     }
           }  
         }
     }

    DPRINT2( debug_autolock, "in findfid with di = %d and i = %d\n", di, i );
    if (((di <=  0) || (i == 1)) && !host_abort)
    { 
      currentZ0 = startZ0 - i*distance;
      DPRINT2( debug_autolock, "in findfid, comparing %d with %d at point 2\n",
		      currentZ0, theFindLockObj.smallest );
      if (currentZ0 >= theFindLockObj.smallest)
       {
        set_find_lock_offset(currentZ0);
        find_lock_delay();
        getlkfid(lkdata,SCANS,LK20HZ);
        xch_analyz_1(st_data,lksize, &level, &freq, &phs, &snr);
  	if (totnoise>snr) totnoise=snr;
        DPRINT3(debug_autolock,
            "in findfid, resonance search, Z0=%d freq=%d snr=%d\n",
            currentZ0,freq,snr );
        DPRINT1(debug_autolock, "bestsnr=%d\n",bestsnr);
        if (bestsnr<snr) 
          {
	    s_bestsnr = bestsnr;
	    s_bestval = bestval;
	    s_bfreq = bfreq;
            bestsnr = snr;
	    bestval = currentZ0;
	    bfreq = freq;
            if (snr/totnoise >= THR && snr >= STD/2 && freq < 30) di = - 1;
          }
        else 
	  {
 	   if (s_bestsnr < snr) 
            {
	    s_bestsnr =  snr;
	    s_bestval =  currentZ0;
	    s_bfreq = freq;
            }

          if (bestsnr/totnoise >= THR && bestsnr >= STD && bfreq < 20)
	     {
		ok = TRUE;
	     }

 	 }
       } 
     }
  
    DPRINT2( debug_autolock, "in findfid with di = %d and i = %d\n", di, i );
    DPRINT5( debug_autolock, "1st test for range, startZ0: %d, i: %d, distance: %d, range: %d %d\n",
	          startZ0, i, distance, theFindLockObj.smallest, theFindLockObj.largest );
    if ( startZ0 - i*distance < theFindLockObj.smallest &&
         startZ0 + i*distance > theFindLockObj.largest )  
          ok = TRUE;

    i++;

  } /*end while !ok */

 if (bestsnr/totnoise < THR || bestsnr < STD)  ok = FALSE;

 } /*end while !ok && count < 4 */


  if ( host_abort || (bestsnr/totnoise < THR) || (bestsnr < STD) ) 
    {
     DPRINT(debug_autolock," cannot find FID signal\n");
     return (0);
    }
  else 
    {
      startZ0 = bestval;
      
      if (bfreq > 20)  ok = FALSE;
      while (!ok && !host_abort)
       {
        for (i = 1; i < 4; i++)
	 {
	  currentZ0 = startZ0 + di*i*distance;
    DPRINT3( debug_autolock, "2nd test for range, current Z0: %d, range: %d %d\n",
	          currentZ0, theFindLockObj.smallest, theFindLockObj.largest );
       	  if ((currentZ0 <= theFindLockObj.largest &&
	       currentZ0 >= theFindLockObj.smallest))
           {
     	    set_find_lock_offset(currentZ0);
        DPRINT1( debug_autolock, "in findfid, point 5, set Z0 to %d\n", currentZ0 );
     	    find_lock_delay();
     	    getlkfid(lkdata,SCANS,LK20HZ);
            xch_analyz_1(st_data,lksize, &level, &freq, &phs, &snr);
    	    if (bestsnr<snr) 
     	     {
		s_bestsnr = bestsnr;
	    	s_bestval = bestval;
	    	s_bfreq = bfreq;
      	        bestsnr = snr;
	    	bestval = currentZ0;
	     	bfreq = freq;
             }
            else if (s_bestsnr < snr) 
             {
	       s_bestsnr =  snr;
	       s_bestval =  currentZ0;
	       s_bfreq = freq;
             }
	   }
	  else ok = TRUE;
          if (host_abort)
             i = 5;

 	 } 

        if (bestval == startZ0) 
	  {
	   ok = TRUE;
	  }
	else if (!ok && bestval != startZ0) startZ0 = currentZ0;
       }

     if ((bfreq > 12) && !host_abort)
      {
       set_find_lock_offset(bestval);
       DPRINT1( debug_autolock, "in findfid, point 6, set Z0 to %d\n", bestval );
       find_lock_delay();
       getlkfid(lkdata,SCANS,LK20HZ);
       ik = lk_anlyz(st_data,lksize, &bestsnr,&bfreq,&phs);
 
       set_find_lock_offset(s_bestval);
       DPRINT1( debug_autolock, "in findfid, point 7, set Z0 to %d\n", s_bestval );
       find_lock_delay();
       getlkfid(lkdata,SCANS,LK20HZ);
       ik = lk_anlyz(st_data,lksize, &s_bestsnr,&s_bfreq,&phs);

      	if (s_bestsnr == 0) diff = distance / 4;
 	else diff = bestval - s_bestval;
        ok = FALSE;

	while (!ok && !host_abort && (abs(diff) >= distance / 4) ) 
	 {

	  currentZ0 = bestval - diff / 2; 
	  set_find_lock_offset(currentZ0);
       DPRINT1( debug_autolock, "in findfid, point 8, set Z0 to %d\n", currentZ0 );
 	  find_lock_delay();
   	  getlkfid(lkdata,SCANS,LK20HZ);
       	  ik = lk_anlyz(st_data,lksize, &snr,&freq,&phs);
      	  if (bestsnr<snr) 
          {
	    s_bestsnr = bestsnr;
	    s_bestval = bestval;
	    s_bfreq = bfreq;
            bestsnr = snr;
	    bestval = currentZ0;
	    bfreq = freq;
	  }
 	  else if (snr > s_bestsnr) 
  	   {
  	    s_bestsnr = snr;
  	    s_bestval = currentZ0;
	    s_bfreq = freq;
  	   }

	  else ok = TRUE;

          diff = bestval - s_bestval;

        }

        if (host_abort)
           return(0);

        set_find_lock_offset(bestval);
       DPRINT1( debug_autolock, "in findfid, point 9, set Z0 to %d\n", bestval );
        find_lock_delay();
	getlkfid(lkdata,SCANS,LK20HZ);
      	xch_analyz_1(st_data,lksize, &level, &freq, &phs, &spr);

      	if (freq > 12 )
          {
           if (bfreq - s_bfreq != 0) 
      		unit_Z0 = (float) (bestval - s_bestval) / (float) (bfreq - s_bfreq);
  	   else if (bfreq != 0) unit_Z0 = (float)(bestval)/(float)(bfreq);
  	   else if (s_bfreq != 0) unit_Z0 = (float)(s_bestval)/(float)(s_bfreq);
  	   else unit_Z0 = 0.0;

   	   currentZ0 = bestval - bfreq*unit_Z0; 

    DPRINT3( debug_autolock, "3rd test for range, current Z0: %d, range: %d %d\n",
	          currentZ0, theFindLockObj.smallest, theFindLockObj.largest );
	   if ((currentZ0 > theFindLockObj.smallest) &&
	       (currentZ0 < theFindLockObj.largest))
  	      {
		  set_find_lock_offset(currentZ0);
       DPRINT1( debug_autolock, "in findfid, point 10, set Z0 to %d\n", currentZ0 );
 		  find_lock_delay();
	      }
	  }

      }
     else
      {
        set_find_lock_offset(bestval);
       DPRINT1( debug_autolock, "in findfid, point 10, set Z0 to %d\n", bestval );
        find_lock_delay();
      }

     return (1);
    }
}




/*----------------------------------------------------------------------*/
/* 	Z0_adj1(lkdata, lv, frq, phs, max_spr)				*/
/*	   								*/
/*									*/
/*	Z0 is changed to get close to resonance: freq<=1  		*/
/*									*/
/*									*/
/*----------------------------------------------------------------------*/


int Z0_adj1(lkdata, lv, frq, phs, max_spr)
int lkdata[], *frq, *phs;
long *lv, *max_spr;
{

 int di, count, freq, phase, step, oldfreq, bfreq;
 long level, spr;
 int f, limitZ0, currentZ0, startZ0, bestZ0;
 int lksize, *datap;
 int diok, stop;

   if (host_abort)
      return(0);
   lksize = FIDSIZ - LOCKOFFSET;
   datap = &lkdata[LOCKOFFSET];
   /*limitZ0 = dac_limit();*/
   limitZ0 = (theFindLockObj.largest - theFindLockObj.smallest) / 2;
   DPRINT1( debug_autolock, "in Z0 adj1, limitZ0 = %d\n", limitZ0 );
   f = limitZ0 / 2048;  
   if (f == 0) f = 1;

   updateAcqState(ACQ_AFINDRES);

   *max_spr = 0;

    count = 0;
    di = 1;
    diok = FALSE;
    oldfreq = *frq;
    startZ0 = get_find_lock_offset();
    stop = FALSE;
    bestZ0 = startZ0;
    bfreq = *frq;
    if (bfreq < 3)  step = f*3*bfreq;
    else step = ( f*8*bfreq ) / 3;
    if (step > f*100) step = f*100;

    while (!host_abort && !stop && freq >= 1)
      {
	count++;
	if (!diok) currentZ0 = startZ0 + count*di*step;
        else  currentZ0 += di*step;
	set_find_lock_offset(currentZ0);
       DPRINT1( debug_autolock, "in Z0 adj1, point 1, set Z0 to %d\n", currentZ0 );
	find_lock_delay();
    	getlkfid(lkdata,SCANS,LK20HZ);
    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);

	if (*max_spr < spr)   *max_spr = spr;

	if (freq < bfreq) 
	 {bfreq = freq; 
	  bestZ0 = currentZ0;
	 }

	if (!diok)
	 {
	  if (oldfreq > freq ) diok = TRUE;
	  else
	  {
	  currentZ0 = startZ0 - count*di*step;
	  set_find_lock_offset(currentZ0);
       DPRINT1( debug_autolock, "in Z0 adj1, point 2, set Z0 to %d\n", currentZ0 );
	  find_lock_delay();
          getlkfid(lkdata,SCANS,LK20HZ);
    	  xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	  if (*max_spr < spr)   *max_spr = spr;

	  if (freq < bfreq) 
	   {bfreq = freq; 
	    bestZ0 = currentZ0;
	   }

	  if (oldfreq > freq ) 
	   	{
	    	diok = TRUE;
	    	di *= -1;
	   	}
  	  }
	 }

 	if (diok)
 	 {
	  if ( freq > oldfreq ) 
	   {
	   currentZ0 -= di*step;
	   step /= 2;
	   }
	  else
	   {
	    *frq = oldfreq;
	    oldfreq = freq;
  	    if (bfreq < 3)  step = f*3*bfreq;
     	    else step = ( f*8*bfreq ) / 3;
	    if (step > f*100) step = f*100;
	   }
	 }

        if (count > 10 || step < 3 || freq < 2 ) stop = TRUE;

      }

   set_find_lock_offset(bestZ0);
   DPRINT1( debug_autolock, "in Z0 adj1, point 3, set Z0 to %d\n", bestZ0 );
   find_lock_delay();
   getlkfid(lkdata,SCANS,LK20HZ);
   xch_analyz(datap,lksize, &level, &freq, &phase, &spr); 
   if (*max_spr < spr)   *max_spr = spr;

  *lv = level;
  *phs = phase;
  *frq = freq;

  return(0);

}


/*----------------------------------------------------------------------*/
/* 	Z0_adj3(lkdata, mode)						*/
/*	   								*/
/*	The lock is placed into the 2kHz faster mode.			*/
/*									*/
/*	Z0 is changed to get max x-chanel level 	 		*/
/*									*/
/*	the simple phase-adjustment is done 	  			*/
/*									*/
/*----------------------------------------------------------------------*/


int Z0_adj3(lkdata, mode)
  int lkdata[], mode;
{
   int step;
   int i, di, stop;
   int freq, phase, phs;
   long level, maxlevel, MX2L, spr;
   int f;
   int currentZ0, maxlvZ0, MX2Z0, limitZ0, startZ0;
   int lksize, delay;
   int *datap;
  
/* initial variables */

   if (host_abort)
      return(0);
   delay = 85;
   lksize = FIDSIZ - LOCKOFFSET;
   datap = &lkdata[LOCKOFFSET];
   startZ0 = get_find_lock_offset();
   /*limitZ0 = dac_limit();*/
   limitZ0 = (theFindLockObj.largest - theFindLockObj.smallest) / 2;
   DPRINT1( debug_autolock, "in Z0 adj3, limitZ0 = %d\n", limitZ0 );

/*---------adjust phase --------------*/

    level = getLockLevel();
    if ( level < LK20p_LEVEL && mode != 3 && mode != 4  && mode != 6)
	 phase_adj1(lkdata, &level, &freq, &phase, &spr);


   updateAcqState(ACQ_AFINDRES);

   f = limitZ0 / 2048;  
   if (f == 0) f = 1;
   maxlvZ0 = startZ0;


/* -------adjust Z0 -------------*/

   getlkfid(lkdata,SCANS,LK20HZ);
   xch_analyz(datap,lksize, &level, &freq, &phase, &spr);

   if (freq ==0  ) step = 2*f;
   else if (freq < 3 ) step = 3*f;
   else step = 5*f;
   

   maxlevel = abs(level);
   currentZ0 = startZ0 + step;
   set_find_lock_offset(currentZ0);
   DPRINT1( debug_autolock, "in Z0 adj3, point 1, set Z0 to %d\n", currentZ0 );
   find_lock_delay(); 
   getlkfid(lkdata,SCANS,LK20HZ);
   cal_xch_level(datap,lksize,&level);

   if (abs(level) > maxlevel) 
	{ 
	MX2L = maxlevel;
	MX2Z0 = maxlvZ0;
	maxlevel = abs(level);
	maxlvZ0 = currentZ0;
	di = 1; 
	}
   else 
	{ 
	di = -1;
	MX2L = abs(level);
	MX2Z0 = currentZ0;
	}

   stop = FALSE;
   while(!host_abort && !stop)
    {
    currentZ0 = maxlvZ0 + di*step;
    set_find_lock_offset(currentZ0);
   DPRINT1( debug_autolock, "in Z0 adj3, point 2, set Z0 to %d\n", currentZ0 );
    find_lock_delay(); 
    getlkfid(lkdata,SCANS,LK20HZ);
    cal_xch_level(datap,lksize,&level);
    if (abs(level) > maxlevel) 
      { 
	 MX2L = maxlevel;
	 MX2Z0 = maxlvZ0;
	 maxlevel = abs(level);
	 maxlvZ0 = currentZ0; 
      }
    else 
      {
        stop = TRUE;
        if (abs(level) > MX2L) 
	 { 
	  MX2L = abs(level);
	  MX2Z0 = currentZ0;
	 }
       }
    }

   stop = FALSE;
   while(!host_abort && !stop && abs(maxlvZ0 - MX2Z0) >= 2)
   {
    currentZ0 = (maxlvZ0 + MX2Z0) / 2;   
    set_find_lock_offset(currentZ0);
   DPRINT1( debug_autolock, "in Z0 adj3, point 3, set Z0 to %d\n", currentZ0 );
    find_lock_delay(); 
    getlkfid(lkdata,SCANS,LK20HZ);
    cal_xch_level(datap,lksize,&level);
    if (abs(level) > maxlevel) 
	{
	 MX2L = maxlevel;
	 MX2Z0 = maxlvZ0;
	 maxlevel = abs(level);
	 maxlvZ0 = currentZ0;	 
     	} 
    else if (abs(level) > MX2L) 
	{ 
	MX2L = abs(level);
	MX2Z0 = currentZ0;
	}
    else  stop = TRUE;
    	 
   }  /*end of while !stop */

        
    set_find_lock_offset(maxlvZ0);
    DPRINT1( debug_autolock, "in Z0 adj3, point 4, set Z0 to %d\n", maxlvZ0 );
    find_lock_delay();

    return(TRUE);

} 


/*----------------------------------------------------------------------*/
/* 	Z0_adj2(lkdata, mode)						*/
/*	   								*/
/*	The lock is placed into the 2kHz faster mode.			*/
/*									*/
/*	Z0 is changed to get max x-chanel level 	 		*/
/*									*/
/*	the simple phase-adjustment is done 	  			*/
/*									*/
/*----------------------------------------------------------------------*/


int Z0_adj2(lkdata, mode)
  int lkdata[], mode;
{
   int step;
   int i, di, stop;
   int freq, phase, phs;
   long level, maxlevel, MX2L, spr, startlevel;
   int f, diok, count;
   int currentZ0, maxlvZ0, MX2Z0, limitZ0, startZ0;
   int lksize, delay;
   int *datap;
  
/* initial variables */

   if (host_abort)
      return(0);
   delay = 85;
   lksize = FIDSIZ - LOCKOFFSET;
   datap = &lkdata[LOCKOFFSET];
   startZ0 = get_find_lock_offset();
   /*limitZ0 = dac_limit();*/
   limitZ0 = (theFindLockObj.largest - theFindLockObj.smallest) / 2;
   DPRINT1( debug_autolock, "in Z0 adj2, limitZ0 = %d\n", limitZ0 );

/*---------adjust phase --------------*/


    level = getLockLevel();
    if ( level < LK20p_LEVEL && mode != 3 && mode != 4 && mode != 6 )
	 phase_adj1(lkdata, &level, &freq, &phase, &spr);


   updateAcqState(ACQ_AFINDRES);

   f = limitZ0 / 2048;  
   if (f == 0) f = 1;
   maxlvZ0 = startZ0;


/* -------adjust Z0 -------------*/

   getlkfid(lkdata,SCANS,LK20HZ);
   xch_analyz(datap,lksize, &level, &freq, &phase, &spr);

   if (freq ==0  ) step = 5*f;
   else if (freq < 3 ) step = 8*f;
   else step = 12*f;

  maxlevel = level;
  startlevel = level;
  diok = FALSE;
  count = 0;

  while (!host_abort && (count < 4) && !diok)
  { 
     count++;
     currentZ0 = startZ0 + step*count;
     set_find_lock_offset(currentZ0);
     DPRINT1( debug_autolock, "in Z0_adj2, point 1, set Z0 to %d\n", currentZ0 );
     find_lock_delay(); 
     getlkfid(lkdata,SCANS,LK20HZ);
     cal_xch_level(datap,lksize,&level);
     if (level > maxlevel)    
	{ 
	MX2L = maxlevel;
	MX2Z0 = maxlvZ0;
	maxlevel = level;
	maxlvZ0 = currentZ0;
	}
     else 
      { 
       if (count == 1) { MX2L = level;	MX2Z0 = currentZ0;}
       else if (level > MX2L ) { MX2L = level;	MX2Z0 = currentZ0;}
      }  

     if (level - startlevel > 100)  { diok = TRUE; di = 1; }

    if (!diok)   
     {
     currentZ0 = startZ0 - step*count;
     set_find_lock_offset(currentZ0);
     DPRINT1( debug_autolock, "in Z0_adj2, point 2, set Z0 to %d\n", currentZ0 );
     find_lock_delay(); 
     getlkfid(lkdata,SCANS,LK20HZ);
     cal_xch_level(datap,lksize,&level);
     if (level > maxlevel)    
	{ 
	MX2L = maxlevel;
	MX2Z0 = maxlvZ0;
	maxlevel = level;
	maxlvZ0 = currentZ0;
	}
     else if (level > MX2L ) { MX2L = level;	MX2Z0 = currentZ0;}
       
     if (level - startlevel > 100)  { diok = TRUE; di = -1; }
     }
  }


   stop = FALSE;
   currentZ0 = maxlvZ0;
   while(!host_abort && diok && !stop)
    {
    currentZ0 += di*step;
    set_find_lock_offset(currentZ0);
    DPRINT1( debug_autolock, "in Z0_adj2, point 3, set Z0 to %d\n", currentZ0 );
    find_lock_delay(); 
    getlkfid(lkdata,SCANS,LK20HZ);
    cal_xch_level(datap,lksize,&level);
    if (level > maxlevel) 
      { 
	 MX2L = maxlevel;
	 MX2Z0 = maxlvZ0;
	 maxlevel = level;
	 maxlvZ0 = currentZ0; 
      }
    else 
      {
        if (level - maxlevel < -100) stop = TRUE;
        if (level > MX2L) 
	 { 
	  MX2L = level;
	  MX2Z0 = currentZ0;
	 }
       }
    }

   stop = FALSE;
   while(!host_abort && !stop && abs(maxlvZ0 - MX2Z0) >= 2)
   {
    currentZ0 = (maxlvZ0 + MX2Z0) / 2;   
    set_find_lock_offset(currentZ0);
    DPRINT1( debug_autolock, "in Z0_adj2, point 4, set Z0 to %d\n", currentZ0 );
    find_lock_delay(); 
    getlkfid(lkdata,SCANS,LK20HZ);
    cal_xch_level(datap,lksize,&level);
    if (level > maxlevel) 
	{
	 MX2L = maxlevel;
	 MX2Z0 = maxlvZ0;
	 maxlevel = level;
	 maxlvZ0 = currentZ0;	 
     	} 
    else if (level > MX2L) 
	{ 
	MX2L = level;
	MX2Z0 = currentZ0;
	}
    else  stop = TRUE;
    	 
   }  /*end of while !stop */


    set_find_lock_offset(maxlvZ0);
    DPRINT1( debug_autolock, "in Z0_adj2, set Z0 to %d\n", maxlvZ0 );
    find_lock_delay();

    return(TRUE);

} 




/*----------------------------------------------------------------------*/
/* power_adj1(lkdata)						*/
/*	   								*/
/*	Power is decreased as long as x-chanel level 	 		*/
/*	does not decrease  (max of 20db allowed).	     		*/
/*	 								*/
/*	If in linear area and x-chanel level is big (>90%)	 	*/
/*		then reduce power until it < 90%			*/
/*									*/
/*	the gain is adjusted to give a signal level of approx 50%.	*/
/*									*/
/*----------------------------------------------------------------------*/


int power_adj1(lkdata)
  int lkdata[];
{
   int power,gain,pwrchg, step, oldstep, startpwr;
   int minpwr = STMINPWR;
   int ok, i, di, stop, count;
   int phase, phs, freq;
   long level,oldlevel, spr;
   long lvchg, oldlvchg, dl, dis;
   int lksize, delay;
   int *st_data;
  
/* initial variables */
  if (host_abort)
     return(0);
  delay = 85;
  lksize = FIDSIZ - LOCKOFFSET;
  st_data = &lkdata[LOCKOFFSET];

  setLockGain(g_maxgain);

  
    level = getLockLevel(); 
    if ( level < LK20p_LEVEL && g_mode !=3 && g_mode != 4)
	phase_adj1(lkdata, &level, &freq, &phase, &spr);


   updateAcqState(ACQ_APOWER);

   startpwr = getLockPower();

  /*----------------adjust power --------------- */

   getlkfid(lkdata,SCANS,LK20HZ);
   xch_analyz(st_data,lksize, &level, &freq, &phase, &spr);

   oldlevel = abs(level);
   power = startpwr;

   if (abs(oldlevel) > LK100p_LEVEL && power > (7*g_maxpwr/8))
        {step = 8; dis = 200L;}
   else if (abs(oldlevel)>LK80p_LEVEL && power > (3*g_maxpwr/4))
        {step = 6; dis = 150L;} 
   else if (abs(oldlevel)>LK60p_LEVEL && power > (g_maxpwr/2) )
        {step = 4; dis = 100L;} 
   else {step = 2; dis = 50L;}
  
   power = startpwr - step;
   setLockPower(power);
   Tdelay(POWERDLY);
   getlkfid(lkdata,SCANS,LK20HZ);
   cal_xch_level(st_data,lksize,&level);
   dl = oldlevel - abs(level);

   oldstep = 0;
   ok = FALSE;
   oldlvchg = 0;
   lvchg = dl;

   while (!host_abort && !ok && (dl < dis) &&
          (abs(level) > LK60p_LEVEL) && (power > minpwr + 10) )
    {
	oldstep = step;
   	if (abs(level) > LK100p_LEVEL && power > (7*g_maxpwr/8))
   	     {step = 8; dis = 200L;}
   	else if (abs(level)>LK80p_LEVEL && power > (3*g_maxpwr/4))
   	     {step = 6; dis = 150L;} 
   	else if (abs(level)>LK60p_LEVEL && power > (g_maxpwr/2) )
   	     {step = 4; dis = 100L;} 
   	else {step = 2; dis = 50L;}

      	power = power - step;
	if( power < minpwr ) 
	  {ok = TRUE;
	   power = minpwr;}
	setLockPower(power); 
        Tdelay(POWERDLY);
	oldlevel = abs(level);
	getlkfid(lkdata,SCANS,LK20HZ);
	cal_xch_level(st_data,lksize,&level);
	dl = oldlevel - abs(level);
   	oldlvchg = lvchg;
   	lvchg = dl;

    }  /*end while !ok .. */


    while (!host_abort && (abs(level) > LK135p_LEVEL) && (power > minpwr + 5))
      {
	oldstep = step;
   	if (abs(level) > LK100p_LEVEL && power > (7*g_maxpwr/8))
   	     {step = 8; }
   	else if (abs(level)>LK80p_LEVEL && power > (3*g_maxpwr/4))
   	     {step = 6; } 
   	else if (abs(level)>LK60p_LEVEL && power > (g_maxpwr/2) )
   	     {step = 4; } 
   	else {step = 2; }

	     power += -step;
	     setLockPower(power); 
             Tdelay(POWERDLY);
	     oldlevel = abs(level);
	     getlkfid(lkdata,SCANS,LK20HZ);
	     cal_xch_level(st_data,lksize,&level);
  	     oldlvchg = lvchg;
   	     lvchg = oldlevel - abs(level);
      }
  

  if ( abs (level) < LK30p_LEVEL )  
	{
	   power += 2;
           if (power <= g_maxpwr)
	      {
	     	setLockPower(power); 
             	Tdelay(POWERDLY);
	      }
	}


/* ----- adjust gain -- while signal is > 73% or <   33% of max signal then change it */ 
/*
       gain = g_maxgain;   
       ok = TRUE; 
       level = getLockLevel();
	count = 0;
       while (!host_abort && (count < 3) && ok &&
              (level > LK72p_LEVEL || level < LK30p_LEVEL))
   	   {   count++;
 	      adjust_gain(&gain,&ok,level,LK52p_LEVEL);
	      level = getLockLevel();
   	   }   
*/

    return(TRUE);

} 
 

/*----------------------------------------------------------------------*/
/* power_adj2(lkdata)						*/
/*	   								*/
/*	Power is decreased as long as x-chanel level 	 		*/
/*	does not decrease  (max of 20db allowed).	     		*/
/*	 								*/
/*	If in linear area and getlklevel is big (>90%)	 		*/
/*		then reduce power until it < 90%			*/
/*									*/
/*	the gain is adjusted to give a signal level of approx 50%.	*/
/*									*/
/*----------------------------------------------------------------------*/


int power_adj2(lkdata)
  int lkdata[];
{
   int power,gain,pwrchg, step, oldstep, startpwr;
   int minpwr = STMINPWR;
   int ok, i, di, stop, count;
   int phase, phs, freq;
   long level,oldlevel, spr;
   long lvchg, oldlvchg, dl, dis;
   int lksize, delay;
   int *st_data;
  
/* initial variables */
  delay = 85;
  lksize = FIDSIZ - LOCKOFFSET;
  st_data = &lkdata[LOCKOFFSET];
  
   level = getLockLevel();

   
    if ( level < LK20p_LEVEL && g_mode !=3 && g_mode != 4) 
 	phase_adj1(lkdata, &level, &freq, &phase, &spr);
	 

   updateAcqState(ACQ_APOWER);

   startpwr = getLockPower();
   gain = getLockGain();

 
   if (gain != 7*g_maxgain/8 ) 
     {setLockGain(7*g_maxgain/8);
      Tdelay(GAINDLY);
     }

 /*----------------adjust power --------------- */

   oldlevel = abs(level);
   power = startpwr;

   if (abs(oldlevel) > LK100p_LEVEL && power > (7*g_maxpwr/8))
        {step = 8; dis = 200L;}
   else if (abs(oldlevel)>LK80p_LEVEL && power > (3*g_maxpwr/4))
        {step = 6; dis = 150L;} 
   else if (abs(oldlevel)>LK60p_LEVEL && power > (g_maxpwr/2) )
        {step = 4; dis = 100L;} 
   else {step = 2; dis = 50L;}
  
   power = startpwr - step;
   setLockPower(power);
   Tdelay(POWERDLY);
   level = getLockLevel();
   dl = oldlevel - level;

   oldstep = 0;
   ok = FALSE;
   oldlvchg = 0;
   lvchg = dl;

   while (!host_abort && !ok && dl < dis && level > LK52p_LEVEL && power > minpwr + 5)
    {
	oldstep = step;
   	if (abs(level) > LK100p_LEVEL && power > (7*g_maxpwr/8))
   	     {step = 8; dis = 200L;}
   	else if (abs(level)>LK80p_LEVEL && power > (3*g_maxpwr/4))
   	     {step = 6; dis = 150L;} 
   	else if (abs(level)>LK60p_LEVEL && power > (g_maxpwr/2) )
   	     {step = 4; dis = 100L;} 
   	else {step = 2; dis = 50L;}

	if (oldlvchg <= 100 && lvchg <= 100) 
	  {step = 2*oldstep; 
	   if ( step >8 ) {step = 8; dis = 200L;}
	   else  dis *= 2;
	  }

      	power = power - step;
	if( power < minpwr ) 
	  {ok = TRUE;
	   power = minpwr;}
	setLockPower(power); 
        Tdelay(POWERDLY);
	oldlevel = abs(level);
   	level = getLockLevel();
	dl = oldlevel - abs(level);
   	oldlvchg = lvchg;
   	lvchg = dl;

    }  /*end while !ok .. */


    while (!host_abort && abs(level) > LK100p_LEVEL && power > minpwr)
      {
	oldstep = step;
   	if (abs(level) > LK135p_LEVEL && power > 3*g_maxpwr/4)
   	     step = 6;  
   	else if ( power > g_maxpwr/2 )
   	     step = 4; 
   	else if ( power > minpwr + 5 )
   	     step = 3;  
   	else step = 2;

	if (oldlvchg <= 50 && lvchg <= 50) 
	  {
	   step = oldstep +2; 
	   if ( step > 6 ) step = 6;
	  }

	     power += -step;
	     setLockPower(power); 
             Tdelay(POWERDLY);
	     oldlevel = abs(level);
   	     level = getLockLevel();
  	     oldlvchg = lvchg;
   	     lvchg = oldlevel - abs(level);
      }
 

  if ( abs (level) > LK80p_LEVEL )
   {
    if (oldlvchg == 0) oldlvchg = 1;

    if ( abs((lvchg*oldstep)/(step*oldlvchg)) >2 && power > minpwr) 
	{

	   if (abs(level)>LK90p_LEVEL)
	        {step = 4; } 
	   else if (abs(level)>LK72p_LEVEL)
	        {step = 3;  } 
	   else {step = 2;  }

	     power += -step;
	     setLockPower(power); 
             Tdelay(POWERDLY);
	     oldlevel = abs(level);
   	     level = getLockLevel();

     	}
   }


  if ( abs (level) < LK60p_LEVEL )  
	{
	   power += 2;
	   if (power < g_maxpwr)
	     {
	     	setLockPower(power); 
             	Tdelay(POWERDLY);
 	     }
	}


/* ----- adjust gain -- while signal is > 73% or <   33% of max signal then change it */ 
/*
       gain = g_maxgain;   
       ok = TRUE; 
       level = getLockLevel();
	count = 0;
       while (!host_abort && count < 3 && ok &&
              (level > LK72p_LEVEL || level < LK30p_LEVEL))
   	   {   count++;
 	      adjust_gain(&gain,&ok,level,LK52p_LEVEL);
	      level = getLockLevel();
   	   }   

*/
    return(TRUE);

} 
 

/*----------------------------------------------------------------------*/
/* power_stable_adj1(lkdata, level)						*/
/*	   								*/
/*	   startpwr - starting lock power				*/
/*	  if lock level is not stable, reduce power			*/
/*----------------------------------------------------------------------*/

 int power_stable_adj1(lkdata, level)
 int lkdata[];
 long level;
   {
   int power, pwrchg, startpwr;
   int  i, minpwr = STMINPWR;
   long ml, dl, LL;
   int  stable, delay;
   int lksize;
   int *st_data;
  
   /*-------- initial variables ---------*/
 
   if (host_abort)
      return(0);
   delay = 85;
   lksize = FIDSIZ - LOCKOFFSET;
   st_data = &lkdata[LOCKOFFSET];

  /*----------------Test stability of signal--------------- */

  updateAcqState(ACQ_APOWER);

   startpwr = getLockPower();

 
   setLockGain(7*g_maxgain/8);
   Tdelay(GAINDLY);
 
   getlkfid(lkdata,SCANS,LK20HZ);
   cal_xch_level(st_data,lksize,&ml);
   Tdelay(20);
   dl = ml;

   for (i=0; i<10; i++)
    { 
     getlkfid(lkdata,SCANS,LK20HZ);
     cal_xch_level(st_data,lksize,&LL);
     if (ml < LL) ml = LL;  
     if (dl > LL) dl = LL;
     Tdelay(20);
    }

   if ( abs(ml) < abs(dl) ) 
    {
      LL = ml;
      ml = dl;
      dl = LL;
    }

   dl = abs(ml) - abs(dl);

   stable = TRUE;
   power = startpwr;
 
   if (dl > LK5p_LEVEL && abs(ml) > LK30p_LEVEL)
     {
      	stable = FALSE;
   	while(!host_abort && !stable && abs(ml) > LK30p_LEVEL)	  /* make sure signal is stable within 5%   */
         {		
	   i = rule1_pwrchg(&pwrchg,dl,ml);
	   if(i == 1 || pwrchg == 0) stable = TRUE;
	   else 
 	    {
	     power += pwrchg;
	     if( power < minpwr) 
		{stable = TRUE; power = minpwr;} 
	     setLockPower(power); 
             Tdelay(POWERDLY);
	     if (!stable)
	      {
     		getlkfid(lkdata,SCANS,LK20HZ);
  		cal_xch_level(st_data,lksize,&ml);
	   	dl = ml;
	 	Tdelay(20);
	   	for (i=0; i<10; i++)
	   	  { 
     		   getlkfid(lkdata,SCANS,LK20HZ);
     		   cal_xch_level(st_data,lksize,&LL);
	  	   if (ml < LL) ml = LL;  
		   if (dl > LL) dl = LL;
	     	   Tdelay(20);
	     	  }
  	        if ( abs(ml) < abs(dl) ) 
 		   {
    		    LL = ml;
    		    ml = dl;
  		    dl = LL;
		   }

  	        dl = abs(ml) - abs(dl);
	      }
            }
	 }  /*end while !stable .. */

    }

 return(0);

}


/*----------------------------------------------------------------------*/
/* power_stable_adj2(spr)							*/
/*	   								*/
/*	   startpwr - starting lock power				*/
/*	  if lock level is not stable, reduce power			*/
/*----------------------------------------------------------------------*/

 int power_stable_adj2(spr)
 long spr;
   {
   int power, pwrchg, startpwr;
   int  i, minpwr = STMINPWR;
   long ml, dl, LL;
   int  stable;

  /*----------------Test stability of signal--------------- */

  updateAcqState(ACQ_APOWER);

  startpwr = getLockPower();

  setLockGain(7*g_maxgain/8);
  Tdelay(GAINDLY);

  ml = getLockLevel();
  dl = ml;
  Tdelay(20);

   for (i=0; i<10; i++)
    { 
     LL = getLockLevel();
     if (ml < LL) ml = LL;  
     if (dl > LL) dl = LL;
     Tdelay(20);
    }

   dl = ml - dl;

   stable = TRUE;
   power = startpwr;
 
   if (dl > LK5p_LEVEL && ml > LK30p_LEVEL)
     {
      	stable = FALSE;
   	while(!host_abort && !stable && ml > LK30p_LEVEL)	  /* make sure signal is stable within 5%  or     */
         {		
	   i = rule1_pwrchg(&pwrchg,dl,ml);
	   if(i == 1 || pwrchg == 0) stable = TRUE;
	   else 
 	    {
	     power += pwrchg;
	     if( power < minpwr) 
		{stable = TRUE; power = minpwr;} 
	     setLockPower(power); 
             Tdelay(POWERDLY);
	     if (!stable)
	      {
     		ml = getLockLevel();
	   	dl = ml;
	 	Tdelay(20);
	   	for (i=0; i<10; i++)
	   	  { 
     		   LL = getLockLevel();
	  	   if (ml < LL) ml = LL;  
		   if (dl > LL) dl = LL;
	     	   Tdelay(20);
	     	  }

  	        dl = ml - dl;
	      }
            }
	 }  /*end while !stable .. */

    }


 return(0);

}


/*----------------------------------------------------------------------*/
/* power_gain_adj()							*/
/*	   								*/
/*	   startpwr - starting lock power				*/
/*	  if lock level is not stable, reduce power			*/
/*----------------------------------------------------------------------*/

int power_gain_adj(lkdata)
  int lkdata[];
{
   int power, pwrchg, startpwr, gain;
   int  i, minpwr = STMINPWR;
   long ml, dl, LL, level;
   int  stable, delay, ph;
   int lksize, count;
   int *st_data;
  
   /*-------- initial variables ---------*/

   delay = 85;
   lksize = FIDSIZ - LOCKOFFSET;
   st_data = &lkdata[LOCKOFFSET];

  /*----------------Test stability of signal--------------- */

  updateAcqState(ACQ_APOWER);

  startpwr = getLockPower();
  gain = getLockGain();
  if(gain<g_maxgain)  {setLockGain(g_maxgain); Tdelay(85);}

	stable = TRUE;
	gain = getLockGain();
	level = getLockLevel(); 
	count = 0;
   	while(!host_abort && (level > LK72p_LEVEL || level < LK30p_LEVEL) &&
              stable && count < 4 )
   	     {  count++;
	      adjust_gain(&gain,&stable, level, LK52p_LEVEL);
	      level = getLockLevel();
   	     }


   getlkfid(lkdata,SCANS,LK20HZ);
   cal_xch_level(st_data,lksize,&ml);
   Tdelay(30);
   dl = ml;

   for (i=0; i<12; i++)
    { 
     getlkfid(lkdata,SCANS,LK20HZ);
     cal_xch_level(st_data,lksize,&LL);
     if (ml < LL) ml = LL;  
     if (dl > LL) dl = LL;
     Tdelay(30);
    }

   if ( abs(ml) < abs(dl) ) 
    {
      LL = ml;
      ml = dl;
      dl = LL;
    }

   dl = abs(ml) - abs(dl);

   stable = TRUE;
   power = startpwr;
 
   if (dl >= LK2p_LEVEL && abs(ml) >= LK20p_LEVEL)
     {
      	stable = FALSE;
   	while(!host_abort && !stable && abs(ml) >= LK20p_LEVEL)	  /* make sure signal is stable within 3%  */
         {
	   updateAcqState(ACQ_APOWER);		
	   i = rule3_pwrchg(&pwrchg,dl,ml);
	   if(i == 1 || pwrchg == 0) stable = TRUE;
	   else 
 	    {
	     power += pwrchg;
	     if( power < minpwr) 
		{stable = TRUE; power = minpwr;} 
	     setLockPower(power); 
             Tdelay(POWERDLY);

             if (!locked(100))
		{stable = TRUE; power = BestPower; 
	    	 setLockPower(power); 
            	 Tdelay(POWERDLY);
		}
	     else  BestPower = power;
		
	     if (!stable)
	      {
     		getlkfid(lkdata,SCANS,LK20HZ);
  		cal_xch_level(st_data,lksize,&ml);
	   	dl = ml;
	 	Tdelay(30);
	   	for (i=0; i<12; i++)
	   	  { 
     		   getlkfid(lkdata,SCANS,LK20HZ);
     		   cal_xch_level(st_data,lksize,&LL);
	  	   if (ml < LL) ml = LL;  
		   if (dl > LL) dl = LL;
	     	   Tdelay(30);
	     	  }
  	        if ( abs(ml) < abs(dl) ) 
 		   {
    		    LL = ml;
    		    ml = dl;
  		    dl = LL;
		   }

  	        dl = abs(ml) - abs(dl);
	      }
            }


	 }  /*end while !stable .. */

	   
        if (abs(ml) < LK30p_LEVEL ) ph = 15;
	else ph = 20;
 	phase_adj2(lkdata, ph);
 	 
        updateAcqState(ACQ_AGAIN);
	stable = TRUE;
	gain = getLockGain();
	level = getLockLevel(); 
	count = 0;
   	while(!host_abort && (level > LK72p_LEVEL || level < LK30p_LEVEL) &&
              stable && count < 4 )
   	     {  count++;
	      adjust_gain(&gain,&stable, level, LK52p_LEVEL);
	      level = getLockLevel();
   	     }

    }

 return(0);

}


/*----------------------------------------------------------------------*/
/* rule1_pwrchg()							*/
/* a fuzzy rule based adjustment of power				*/
/*									*/
/*----------------------------------------------------------------------*/

int rule1_pwrchg(pwrchg, input, level)
int *pwrchg;
long level,input;
{
 long LCB = LK30p_LEVEL, LCM = LK15p_LEVEL, LCS = LK5p_LEVEL;
 long LLVB = LK100p_LEVEL, LLB = LK80p_LEVEL, LLM = LK52p_LEVEL, LLS = LK30p_LEVEL;
 int PCVB = 10, PCB = 6, PCM = 4, PCS = 2;
 float p1, p2, x;
 int output;

if(input > LCS && level > LLS)
{
 if (input >= LCB && level >= LLVB) output = PCVB;

 else if (input >= LCM && level >= LLB)
  {
   if (input >= LCB)
    {
     p1 = (float) (LLVB-level);
     p2 = (float) (level-LLB);
    }
   else
    {
     p1 = (float) (LCB-input);
     p2 = (float) (input-LCM);
    } 
   x = (p2* (float) PCVB + p1* (float) PCB) / (p1+p2);
   output = (int) x;  
  }

 else if (input >= LCS && level >= LLM)
  {
   if (input >= LCM)
    {
     p1 = (float) (LLB-level);
     p2 = (float) (level-LLM);
    }
   else
    {
     p1 = (float) (LCM-input);
     p2 = (float) (input-LCS);
    } 
   x = (p2* (float) PCB + p1* (float) PCM) / (p1+p2);
   output = (int) x;
  }

 else 
  {
     p1 = (float) (LLM-level);
     p2 = (float) (level-LLS);

/*   p1 = (float) (LCM-input);
   p2 = (float) (input-LCS);   
*/

   x = (p2* (float) PCM + p1* (float) PCS) / (p1+p2);
   output = (int) x;
  }

  *pwrchg = - output;
  return(0); 

} 
 
else  return(1);  

}


/*----------------------------------------------------------------------*/
/* rule3_pwrchg()							*/
/* a fuzzy rule based adjustment of power				*/
/*									*/
/*----------------------------------------------------------------------*/

int rule3_pwrchg(pwrchg, input, level)
int *pwrchg;
long level,input;
{
 long LCB = LK10p_LEVEL, LCM = LK5p_LEVEL, LCS = LK2p_LEVEL;
 long LLVB = LK52p_LEVEL, LLB = LK42p_LEVEL, LLM = LK30p_LEVEL, LLS = LK20p_LEVEL;
 int PCVB = 6, PCB = 4, PCM = 2, PCS = 1;
 float p1, p2, x;
 int output;

if(input >= LCS && level >= LLS)
{
 if (input >= LCB && level >= LLVB) output = PCVB;

 else if (input >= LCM && level >= LLB)
  {
   if (input >= LCB)
    {
     p1 = (float) (LLVB-level);
     p2 = (float) (level-LLB);
    }
   else
    {
     p1 = (float) (LCB-input);
     p2 = (float) (input-LCM);
    } 
   x = (p2* (float) PCVB + p1* (float) PCB) / (p1+p2);
   output = (int) x;  
  }

 else if (input >= LCS && level >= LLM)
  {
   if (input >= LCM)
    {
     p1 = (float) (LLB-level);
     p2 = (float) (level-LLM);
    }
   else
    {
     p1 = (float) (LCM-input);
     p2 = (float) (input-LCS);
    } 
   x = (p2* (float) PCB + p1* (float) PCM) / (p1+p2);
   output = (int) x;
  }

 else 
  {
     p1 = (float) (LLM-level);
     p2 = (float) (level-LLS);

/*   p1 = (float) (LCM-input);
   p2 = (float) (input-LCS);   
*/
   x = (p2* (float) PCM + p1* (float) PCS) / (p1+p2);
   output = (int) x;
  }

  *pwrchg = - output;
  return(0); 

} 
 
else  return(1);  

}



/*----------------------------------------------------------------------*/
/* 	phase_adj1(lkdata, lv, frq, phs, spr)				*/
/*	   								*/
/*									*/
/*	phase is changed to get positive valuse for xch signal		*/
/*									*/
/*									*/
/*----------------------------------------------------------------------*/


int phase_adj1(lkdata, lv, frq, phs, max_spr)
int lkdata[], *frq, *phs;
long *lv, *max_spr;
{

 int count, freq, phase, step, ph;
 long level, oldlevel, spr;
 int delay, *datap, lksize;
 int stop, di;

   if (host_abort)
      return(0);
   datap = &lkdata[LOCKOFFSET];
   lksize = FIDSIZ - LOCKOFFSET;
   delay = 85;

    updateAcqState(ACQ_APHASE); 

    level = *lv;
    freq = *frq;
    phase = *phs;

    count = 0;
    step = 120;
    ph = getLockPhase();

    if (freq ==1)
      {
	ph -= 90;
	setLockPhase(ph);
	Tdelay(delay);
    	getlkfid(lkdata,SCANS,LK20HZ);
    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	if (*max_spr < spr)   *max_spr = spr;
	level = getLockLevel();
	if (freq == 0 && level < 0) 
     	 {
	  step = 180;    
	  ph += step;
	  while (ph > 360) ph -= 360;
	  while (ph < 0) ph += 360;
	  setLockPhase (ph);
 	  Tdelay(delay); 
    	  getlkfid(lkdata,SCANS,LK20HZ);
    	  xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	  if (*max_spr < spr)   *max_spr = spr;
	  level = getLockLevel();
     	 }
      }

    if ( level < LK20p_LEVEL || freq >= 1) 
	{ stop = FALSE;
	    ph -= phase; }
    else  stop = TRUE;

    while (!host_abort && !stop && count < 2)
      {
	count++;
	while (ph > 360) ph -= 360;
	while (ph < 0) ph += 360;
	setLockPhase(ph);
	Tdelay(delay);
    	getlkfid(lkdata,SCANS,LK20HZ);
    	xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	if (*max_spr < spr)   *max_spr = spr;
	level = getLockLevel();
	if (abs(phase) > 20)  
          {
	   ph -= phase;
	  } 
	else  stop = TRUE;
      }


  if (freq == 0 && level < 0) 
     {
	  step = 180;    
	  ph += step;
	  while (ph > 360) ph -= 360;
	  while (ph < 0) ph += 360;
	  setLockPhase (ph);
 	  Tdelay(delay); 
    	  getlkfid(lkdata,SCANS,LK20HZ);
    	  xch_analyz(datap,lksize, &level, &freq, &phase, &spr);
	  if (*max_spr < spr)   *max_spr = spr;
     }

  level = getLockLevel();

  if (freq == 0 && level < LK20p_LEVEL) 
     {  step = 40;
	stop = FALSE;
    	count = 0;
        di = 1;
    	while (!host_abort && !stop && count < 4)
      	 {
	  count++;
	  ph += di*step;
	  while (ph > 360) ph -= 360;
	  while (ph < 0) ph += 360;
	  setLockPhase (ph);
 	  Tdelay(delay); 
          oldlevel = level;
    	  level = getLockLevel();
	  if ( level < oldlevel ) 
	    { 
	      if (count == 1)  
		{ di = -1; 
		  ph += di*step; 
		  level = oldlevel;
		}
              else stop = TRUE;
  	    }
         }
     }

  *lv = level;
  *frq = freq;
  *phs = phase;

  return(0);

}


/*----------------------------------------------------------------------*/
/* phase_adj2(data,step)   final adjustment of  phase	 		*/
/*									*/
/*									*/
/*	1	The phase is so changed that 			 	*/
/*		the lock level of the signal is maximum 		*/
/*	2	When mode==3 or 4, don't change phase			*/
/*									*/
/*		Note that, lock level should be the real chanel signal  */
/*		level							*/
/*----------------------------------------------------------------------*/

#define ENDLIMIT 1
#define LOSTLOCK 2
#define SIGDROP  4
#define ENDORDROP  5
#define MAXDROP  15

int phase_adj2(lkdata, step) 
int  lkdata[];
int step;
{

   int   count, ok, phase, phschg, bestphs, b2phs, startphs;
   long  level, maxlevel, MX2L;

  /*--------- adjust phase ----------*/

 if (g_mode == 3 || g_mode == 4 && g_mode != 6)
  return(FALSE);

   updateAcqState(ACQ_APHASE);
   startphs = getLockPhase();
   maxlevel = getLockLevel();  
   count = 0;

   if (maxlevel < 0)
    {
    phase = startphs + 180;
    setLockPhase(phase);
    Tdelay(PHASEDLY); 
    maxlevel = getLockLevel();
    }

   bestphs = phase = startphs;  
   phschg = step;
   ok = FALSE;

   while (!host_abort && !ok) 
    {count++;
     phase += phschg; 
	  while (phase > 360) phase -= 360;
	  while (phase < 0) phase += 360;

     setLockPhase(phase);
     Tdelay(PHASEDLY);
     level = getLockLevel();  
     if (level - maxlevel > 50) 
    	{MX2L = maxlevel;
	 b2phs = bestphs;
	 maxlevel = level;
	 bestphs = phase;	 
     	} 
     else {
      	   ok = TRUE;
           if (level > maxlevel) 
     		{
		MX2L = maxlevel; 
		b2phs = bestphs;
		maxlevel = level;
		bestphs = phase;
    		}

           if (count == 1 || level > MX2L) 
     		{
		MX2L = level; 
		b2phs = phase;
    		}
	  }   
    }

  if (bestphs == startphs)
   {
     ok = FALSE;
     phase = bestphs;
     while (!host_abort && !ok) 
      {
       phase -= phschg; 
	  while (phase > 360) phase -= 360;
	  while (phase < 0) phase += 360;
       setLockPhase(phase);
       Tdelay(PHASEDLY);
       level = getLockLevel();  
       if (level - maxlevel > 50) 
    	  {
	   MX2L = maxlevel;
	   b2phs = bestphs;
	   maxlevel = level;
	   bestphs = phase;	 
     	  } 
       else 
	  {
      	   ok = TRUE;
           if (level > maxlevel) 
     		{
		MX2L = maxlevel; 
		b2phs = bestphs;
		maxlevel = level;
		bestphs = phase;
    		}

           if (level > MX2L) 
     		{
		MX2L = level; 
		b2phs = phase;
    		}
	  }   

       }  /* end of while !ok */
     }


        if (level < maxlevel ) 
    	  {
   	   setLockPhase(bestphs);
  	   Tdelay(PHASEDLY);
     	  } 


  return (TRUE);


} /* end of phase_adj2 */


/***** low level functions *********/


int adjust_gain(gain,doit,level,normlevel)
int *gain, *doit;
long level, normlevel;
{  
   int oldgain  =  *gain;

   *gain +=  calcgain(level,normlevel);
   if (*gain  <   MINGAIN) { *gain  =  MINGAIN; *doit  =  FALSE; }
   if (*gain > g_maxgain) { *gain  =  g_maxgain; *doit  =  FALSE; }
   if (*gain  ==  oldgain) *doit  =  FALSE;
   if (*doit) 
   {
     setLockGain(*gain);
     Tdelay(GAINDLY);
   }
}

/*-------------------------------------------------------------*/
/* locked(obstime) - test for lock  for observation time       */
/*		     specified.				       */
/*  (int) obstime - time in hundredsth of a second to observe  */
/*		    the lock status bit. If lock fails at any  */
/*		    during this observation period return      */
/*		    Immediately with a ZERO		       */
/*		    If at the end of the observation time no   */
/*		    lock failure has occurred return ONE       */
/*-------------------------------------------------------------*/
 int locked(observe)
  int observe;
  {
    int timchk;
    Ldelay(&timchk,observe);	/* start observation time down */
    while (!host_abort && !Tcheck(timchk)) 
    {
      if (!chlock()) return(0);
      Tdelay(2); /* don't use 1 since 60*1/100 = 0 ticks */
    }
    return(1);		/* locked */
  } /* end of test lock signal */



/*-------------------------------------------------------------*/
/* anylock(obstime) - test for lock  for observation time      */
/*		     specified.				       */
/*  (int) obstime - time in hundredsth of a second to observe  */
/*		    the lock status bit. If lock occurs at any */
/*		    time during this observation period return */
/*		    Immediately with a ONE                     */
/*		    If at the end of the observation time no   */
/*		    lock has occurred return ZERO              */
/*-------------------------------------------------------------*/
int anylock(observe)
int observe;
{
    int timchk;
    Ldelay(&timchk,observe);	/* start observation time down */
    while (!host_abort && !Tcheck(timchk)) 
    {
      if (chlock()) return(1);
      Tdelay(2);	/* force a taskDelay(1) */
    }
    return(0);		/* not locked */
} /* end of test lock signal */



#include <math.h>
#define M_2PI	(6.2831853)
#define M_PI	(3.1415927)
#define RAD2DEG (57.29577951308232087679)

/* lk_anlyz takes a lock fid and returns an analysis of the lock

1. rms signal power (in adc units normalize)
2. frequency shift arbitrary units largest shift=+/-180,000
3. initial phase in degrees.

returns number times signal clips - upper routine may wish to adapt 
if there are 5 pairs below saturation, you'll get good data 

*/

static int re_acc = 0;
static int im_acc = 0;

int lk_base(data,size,pwr,scans)
int *data,size,*pwr,scans;
{
  double x,y,acc,pacc;
  int i,j,k;
  int ovflag;
  
  acc = 0.0;
  pacc = 0.0;
  re_acc = 0;
  im_acc = 0;
  k = 0;
  size /= 2;
  for (i = 0; i < size; i++)
  {
    x = (double) *data++;
    y = (double) *data++;
    pacc += x*x + y*y;
    re_acc += x;
    im_acc += y;

    if ((x > 32768.0) || (x < -32768.0) || (y > 32768.0) || (y < -32768.0))
    {
      k++;
    }
  }
  *pwr = (int) (sqrt((pacc)/((double) size)));
  re_acc /= (size*scans);
  im_acc /= (size*scans);
  DPRINT3(0,"base_line (%5d,%5d)  %d\n",re_acc,im_acc,*pwr);
  /*  make sure there are at least 5 pairs */
  return(k);
}

/* the overflow and phase calculation are verified with 
   test data to be correct
*/

int lk_anlyz(data,size,pwr,freq,phase)
int *data,size,*pwr,*freq,*phase;
{
  double x,y,theta,theta_old,acc,delta,pacc;
  double nacc;
  int i,j,k;
  int ovflag;
  
  acc = 0.0;
  nacc = 0.0;
  k = 0;
  x = (double) (*data++ - re_acc);
  if (x == 0.0)  x = 0.01;  /* kill off a NAN if y=x=0 */
  y = (double) (*data++ - im_acc);
  
  if ((x > 32768.0) || (x < -32768.0) || (y > 32768.0) || (y < -32768.0))
   {
    k++;
    ovflag = 1;
   }
  else { ovflag = 0; pacc = x*x + y*y;}

  theta_old = atan2(y,x);
  *phase = (int) (RAD2DEG*theta_old); 
  for (i = 1; i < size/2 -1; i++)
  {
    x = (double) (*data++ - re_acc);
    y = (double) (*data++ - im_acc);
    if (x == 0.0)  x = 0.01;  
    if ((x > 32768.0) || (x < -32768.0) || (y > 32768.0) || (y < -32768.0))
    {
      k++;
      ovflag |= 2;
    }
    else pacc += x*x + y*y;

    if (ovflag < 2)
    {
      theta = atan2(y,x);
      if (ovflag == 0)
      {
        delta = theta - theta_old;
        if (delta > M_PI)  delta -= M_2PI;
        if (delta < -M_PI) delta += M_2PI;
        acc += delta;
	nacc += 1.0;
      }
      theta_old = theta;
    }
    if (ovflag >= 2) ovflag = 1; /* set up for next loop */
    else ovflag = 0; /* this point has a valid theta old */
  }
  *pwr = (int) (sqrt((pacc)/((double) size)));  /*  make sure there are at least 5 pairs */

  *freq = (int) (100000.0*acc/nacc); /*  */

  return(k);
}


int lk_phase(data,size,phase)
int *data,size,*phase;
{
  double x,y,theta;
   
  x = (double) (*data++ - re_acc);
  if (x == 0.0)  x = 0.01;  /* kill off a NAN if y=x=0 */
  y = (double) (*data++ - im_acc);
  theta = atan2(y,x);
  *phase = (int) (RAD2DEG*theta); 
  return(1);
}


int cal_xch_level(data,size,level)
int *data, size;
long *level;
{
  double x,y, acc;
  int i, k;
  
  acc = 0.0;
  k = 0;
  for (i = 1; i < size/2; i++)
  {
    x = (double) (*data++ - re_acc);
    y = (double) (*data++ - im_acc);
    if (x == 0.0)  x = 0.01;  

    if ( (x <= 32768.0) && (x >= -32768.0) )
    {
      k++;  
      acc += x;
    }

  }

  if (k == 0) k = 1;

  *level = (long) ( -acc / (4*k) );  

  return(0);
}

int xch_analyz(data, size, level, frq, phs, spr)
int *data, size, *frq, *phs;
long *level, *spr;
{
  double x,y, acc, xx, yy, theta, pacc;
  int i, j, k;
  int freq, fnew, fold;
  int dis, count;

  acc = 0.0;
  k = 0;
  dis = FIDSIZ/100;
  j = 0;
  fnew = 0;
  freq = 0;
 
  x = (double) (*data++ - re_acc);
  y = (double) (*data++ - im_acc);

  if (x == 0.0)  x = 0.01;  /* kill off a NAN if y=x=0 */
  theta = atan2(y,x);

  fnew = (int) x;
  pacc = x*x + y*y;

  xx = 0.0;
  yy = 0.0;

  if ( fabs(x) > 400.0 ) count = 1;
  else count = 0;

   for (i = 1; i < size/2; i++)
    {
     x = (double) (*data++ - re_acc);
     y = (double) (*data++ - im_acc);

     if ( (x <= 32768.0) && (x >= -32768.0) )
       {
	pacc += x*x + y*y;

        k++;  
        acc += x;
        if ( i == 64 || i == 126 || i == 190 ) 
	  {
	    if (x == 0.0)  x = 0.01;
	    xx += atan2(y,x);
	  } 
        if ( i == 1 || i == 63 || i == 127 ) 
	  {
	    if (x == 0.0)  x = 0.01;
	    yy += atan2(y,x);
	  } 
       }

     if ( fabs(x) > 400.0 )
      {
       fold = fnew;
       fnew = (int) x;
       if ( fnew*fold < 0 && count >= 10 ) { count = 1; freq++; } 
       else if (fnew*fold >= 0 ) count++;
      } 

     }


  theta += (xx - yy)/3.0;


  *phs = (int) (RAD2DEG*theta); 

  if (k == 0) k = 1;

  *level = (long) ( -acc / (4*k) );  
  *spr = (int) (sqrt((pacc)/((double) size)));

   *frq = freq; 


  return(1);
}


int xch_analyz_1(data, size, level, frq, phs, spr)
int *data, size, *frq, *phs;
long *level, *spr;
{
  double x,y, acc, xx, yy, theta, pacc;
  int *st_data;
  int i, j, k;
  int freq, f0, fnew, fold;
  int dis, count;

/*   st_data = data;
/*   for(i=0; i<80; i++)
/*   { DPRINT2(-1,"r=%d i=%d\n",*st_data, *(st_data+1) );
/*     st_data += 2;
/*   }
/* */

  acc = 0.0;
  k = 0;
  dis = FIDSIZ/100;
  j = 0;
  fnew = 0;
  freq = 0;
  f0 = 0;
 
  x = (double) (*data++ - re_acc);
  y = (double) (*data++ - im_acc);

  if (x == 0.0)  x = 0.01;  /* kill off a NAN if y=x=0 */
  theta = atan2(y,x);

  fnew = (int) x;
  pacc = x*x + y*y;

  xx = 0.0;
  yy = 0.0;

  if ( fabs(x) > 200.0 ) count = 1;
  else count = 0;

   for (i = 1; i < size/2; i++)
    {
     x = (double) (*data++ - re_acc);
     y = (double) (*data++ - im_acc);

     if ( (x <= 32768.0) && (x >= -32768.0) )
       {
	pacc += x*x + y*y;

        k++;  
        acc += x;
        if ( i == 64 || i == 126 || i == 190 ) 
	  {
	    if (x == 0.0)  x = 0.01;
	    xx += atan2(y,x);
	  } 
        if ( i == 1 || i == 63 || i == 127 ) 
	  {
	    if (x == 0.0)  x = 0.01;
	    yy += atan2(y,x);
	  } 
       }

     if ( fabs(x) > 200.0 )
      {
       fold = fnew;
       fnew = (int) x;
       if ( fnew*fold < 0 && count >= 1 ) { count = 1; freq++; } 
       else if (fnew*fold >= 0 ) count++;
      } 

     }


  theta += (xx - yy)/3.0;


  *phs = (int) (RAD2DEG*theta); 

  if (k == 0) k = 1;

  *level = (long) ( -acc / (4*k) );  
  *spr = (int) (sqrt((pacc)/((double) size)));

   *frq = freq; 


  return(1);
}

/*  end of automation function */
