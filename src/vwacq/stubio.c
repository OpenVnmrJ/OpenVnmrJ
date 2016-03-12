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

/* Phil Hornung 	Varian Central Research

   guesstime on lock power 19 june 1985
   confirmed to work reliably 9 January 1985
   new label version 21 March 1985
   hardware timer usage begun 2 May 1985

   noise() returns a measured noise level which also sets
   an int n_level.

   we know from much experimentation that endtest should be 1-3%
   of signal regardless of the noise level.

   So the get_io routine must average to that level with n_aves required.
   Similarly, the slewlimit must also somehow depend on the noise level
   and the desired accuracy.  Both of these requirements are addressed
   in the filter function.


   The setting of the number of averages is accomplished by the check_io
   function.  
   first, it looks at x = signal/128 (criteria = 't') for a basic standard.
   if the noise is below that, the number of averages is 1.
   if the noise is above that
      number averages = n_level/x;
      if averages < 16 then report x as noise barrier;
      if averages > 16 set averages to 16 and set x=noise/4.

   Slewlimit is set to x.
   Finally as criteria increases, the minumum number of averages increases
   to assure improved accuracy.  x and slewlimit do not change
   (perhaps slewlimit should go down!).

   filter uses these settings in two ways:
   it uses number averages differentially compared to slewlimit to determine
   when the signal settles, and then it averages that number of times
   and returns the average value.

   check_io is executed once each loop so that as the field improves,
   the number of averages may go down!

*/
#include "simplex.h"
#include "logMsgLib.h"
#if defined(INOVA) || defined(MERCURY)
#include "autoObj.h"
#include "lock_interface.h"
#endif

#define PRTON   1
#define PRTOFF  0

extern char     criteria;
extern int      
                delaytime,
                delayflag,
		fs_scl_fct,
		g_maxpwr,	/* max lockpower limit */
                host_abort,
		n_level,	/* noise level, set in stubio.c */
                numvectors,	/* number of active shim coils */
		fid_shim,
	        slewlimit;

static int      n_aves,
                plug,
                target;
static int      fid_start,
		fid_end,
		limitflag = FALSE;

#if defined(INOVA) || defined(MERCURY)
static int failAsserted = 0;
#else
extern int failAsserted;
#endif

extern int      numvectors;	/* number of active shim coils */
extern int      bestindex;	/* best yvalue index, set in maxmincomp */
extern struct entry
{
   int             yvalue;   	 /* lock value, always negative */
   int             vector[NUMDACS];/* associated shim coil settings */
}               table[NUMDACS];

noise()				/* noise is now 16 averages, static noise
				 * level n_level */
{
   int             tar[16],
                   save,
                   i;
   long            x,
                   y,
                   z,
                   s1,
                   s2;

   save = delaytime;
   s1 = 0L;
   /* lock averages for 0.8 sec */
   for (i = 0; i < 16; i++)
   {
      if (!fid_shim)
         tar[i] = -methoda(5);
      else
         tar[i] = getfid();     
   /* get fid may need Ldelay Tcheck - so keep save ....*/
      if (host_abort || failAsserted)
	 return (-99);
      s1 += (long) tar[i];
   }
   delaytime = save;
   x = s1 >> 4;
   s2 = 0L;
   for (i = 0; i < 16; i++)
   {
      z = ((long) tar[i]) - x;
      if (z < 0L)
	 z = -z;
      s2 += z;
   }
   y = (s2 + 8) >> 4;
   n_level = (int) y;
   DPRINT2(0, "average is %ld  NOISE(ave dev) = %ld\n", x, y);
   if (!delayflag && !fid_shim)	/* if user has not set delaytime go calc it,
				 * else not */
   {
      if (guess_time() == -99)
      {
	 DPRINT(0, "guess_time = -99 \n");
	 return (-99);		/* user aborted */
      }
   }
   return (n_level);
}

/*----------------------------------------------------------------------*/
/*  get_io(v,threshold) - 						*/
/*    threshold is usually global var barrier set in simplex.c,     	*/
/*    sometimes it is 0.						*/
/*----------------------------------------------------------------------*/
get_io(v, threshold)
int             v[],
                threshold;
{
   int             i,
                   silk_lvl;

   for (i = 0; i < NUMDACS; i++)
      if (is_active(i) == TRUE)
      {
	 set_dac(i, v[i]);
      }
   if (host_abort || failAsserted)
      return(0);
   if (!fid_shim)
   {
      silk_lvl = filter(n_aves, threshold);
      return (silk_lvl);
   }
   else
      return (getfid());
}

/*----------------------------------------------------------------------*/
/*  filter(n,t_level) - 						*/
/*----------------------------------------------------------------------*/
filter(n, t_level)
int             n,
                t_level;
{
   static int      cbuff[100];
   int            *top,
                  *mid,
                  *bottom,
                  *end,
                   tmp,
                   twon;
   long            sl;
   int            *slide;
   long            sum;

/*    register int *slide; */
/*    register long sum; */
   sl = (long) slewlimit;
   top = cbuff;
   bottom = top;
   end = cbuff + plug;
   twon = 2 * n - 1;
   while ((top < (bottom + twon)) && !host_abort && !failAsserted)
   {
      tmp = -methoda(delaytime);
      *top++ = tmp;
   }

   if ( (tmp > t_level) || host_abort || failAsserted)
      return (t_level + 20);
   else
   {
      /* now filter */
      mid = bottom + n;
      do
      {
	 sum = 0L;
	 *top++ = -methoda(delaytime);	/* top points to next data hole */
	 slide = bottom;
	 while (slide < mid)
	    sum -= (long) *slide++;
	 while (slide < top)
	    sum += (long) *slide++;
	 sum /= n;
	 if (sum < 0L)
	    sum = -sum;
	 bottom++;
	 mid++;
      }
      while ((sum > sl) && (top < end) && !host_abort && !failAsserted);

      if (host_abort)
         return(0);
      sum = 0L;
      slide = mid - 1;
      while (slide < top)
	 sum += (long) *slide++;
      sum /= n;
      return ((int) sum);
   }				/* end else */
}

/*----------------------------------------------------------------------*/
/*	io checking adaptive routine Phil Hornung 14 dec 1984

	Noise barrier = signal / 128 is tightest noise barrier used.
        (noise barrier is not global var barrier).
	If noise > noise barrier then must average by
	(noise / noise barrier)^2 times.
	As the field improves, the number of averages may go down!
	However, as the field specification tightens, a separate
	mechanism causes additional averaging.
/*----------------------------------------------------------------------*/
/*  check_io(y) - returns slewlimit (called barrier in simplex.c)	*/
/*----------------------------------------------------------------------*/
check_io(y)
int             y;
{
   int             x,
                   z,
                   j;

   z = -y;
   switch (criteria)
   {
   case 'b':
   case 'l':
      j = 1;
      x = z >> 5;
      plug = 30;
      break;			/* ~3% noise */
   case 'm':
      j = 1;
      x = z >> 6;
      plug = 40;
      break;			/* ~2% noise */
   case 't':
      j = 3;
      x = z >> 7;
      plug = 50;
      break;			/* <1% noise */
   case 'e':
      j = 5;
      x = z >> 7;
      plug = 70;
      break;			/* <1% noise */
   default:
      j = 1;
      x = z >> 5;
      plug = 30;
   }
   if (x < 5)
      x = 5;			/* guard */
   if (x < n_level)		/* high noise environment */
   {
      n_aves = (4 * n_level) / x;
      n_aves *= n_aves;
      n_aves >>= 4;
      if (n_aves > 9)
      {
	 n_aves = 9;		/* set guard level */
	 x = n_level / 3;	/* we don't do windows either */
      }
   }
   else
      n_aves = 1;

   if (n_aves < j)
      n_aves = j;
#ifdef DEBUG
   if (n_aves > 1) DPRINT1(0,"n_aves=%d ",n_aves);
#endif
   slewlimit = x;
   return (x);
}

methoda(int dly)			
{				
#if defined(INOVA) || defined(MERCURY)
   extern AUTO_ID pTheAutoObject;
   Tdelay(dly);	
   return(autoLkValueGet(pTheAutoObject));
#else
   Tdelay(dly);	
   return((int) getLockLevel());
#endif
}

/* figure out the time scale too

   thinking process:
   1) don't need to figure out the relaxation rate each time!
   2) can't go too fast and keep separate data but we don't need much
      accuracy! just ranges
   3) set a 0.2 sec rep rate for 3 seconds
   4) change lock power which should not have any non-T1 effect
   5) set the response time to

Revision note:  
  This bogusly ass u me s that solvent is the only recovery component. 
  Set a minimum time of 250 ms to hold the magnet out of the picture.
  Reduced the maximum to 3.0 sec....
*/

#define GMAX 99
guess_time()
{
   int             index,
                   t1_buf[100];	/* 5 sec worth */
   int             savep,
		   power_hi,
                   save,
                   del,
                   m,
                   n,
                   o,
                   pdrop;
  long             first,
                   last;

   save = delaytime; /* fixed time here */
   savep = getpower();
   if (savep > (g_maxpwr-6))
   {
     DPRINT3(0, "power level %d > %d. Assume default delay %d\n",
			savep,g_maxpwr,save);
     return (0);
   }
   power_hi = savep + 6;
   setpower(power_hi);
   Tdelay(600);
   t1_buf[0] = abs(methoda(5));	/* index 0 is stable */
   first = t1_buf[0];
   setpower(savep);
   /* decrease power by 6 db then watch relaxation */
   for (index = 1; index <= GMAX; index++)
   {
      t1_buf[index] = abs(methoda(5));
      if (host_abort || failAsserted)
      {
	 return (-99);		/* user aborted */
      }
   }
   del = t1_buf[GMAX] - t1_buf[0];
   last = t1_buf[GMAX];
   /*  */
   if (!chlock())		/* if lock lost then bump up 6 more db */
   {
      setpower(savep + 6);
      Tdelay(20);
   }
   if (!chlock())
      DPRINT(0, "lost lock help!!");
   if (abs(del) > 2 * n_level)
   {				/* proceed else default to some value */
#ifdef DEBUG
      DPRINT(0, "try to estimate sampling time\n");
#endif
      /*
       * the half decay time t1/2 = 0.69 T1 so find t1/2 from both ends and
       * average, then divide by two
       */
      del /= 2;
      o = t1_buf[0] + del;
      m = GMAX;
      n = 0;
      if (del > 0)		/* it went up */
      {				/* if we are, savep = 2 or savep = 1 */
	 while ((m > 0) && (t1_buf[m] > o))
	    m--;
	 while ((n < GMAX) && (t1_buf[n] < o))
	    n++;
      }
      else			/* it went down */
      {
	 while ((m > 0) && (t1_buf[m] < o))
	    m--;		/* howmany above T1/2 */
	 while ((n < GMAX) && (t1_buf[n] > o))
	    n++;		/* howmany Bove T1/2 */
      }
      pdrop = (int)((first * 100L) / last) - 100;
#ifdef DEBUGXXX
      for (index = 0; index < 100; index ++) 
      {
	 DPRINT2(0, "%6d   %3d ", t1_buf[index],index);
      }
      DPRINT5(0, 
       "\n Start=%ld, End=%ld, Drop = %d%%, Pts Before T1/2: m = %d n = %d \n",
	     first, last, pdrop, m, n);
      DPRINT2(0, " mean T1/2 val=%d, abs T1/2 val=%d, \n", del, o);
#endif
      /* if (m - n) > 10, probably a glitch should do better */
      m = (m + n) >> 1;		/* average acq (.05 sec) above t1/2 */
      m *= 5;			/* convert to time units (each pt = 5/100 sec
				 * ) */
      if (m < 25)
	 m = 25;
      if (m > 300)
	 m = 300;
      /* clip .25 sec to 3 sec for sampling rate */
      DPRINT1(0, "T1/2 Guess is %d ms\n ", m * 10);
      delaytime = m;
   }
   else
   {
      DPRINT(0, "not enough difference to measure (ie lock power missadjusted) \n");
      delaytime = save;
   }
   return (0);
}

fid_limit(start,stop)
int start,stop;
{
   fid_start = start;
   fid_end = stop;
   limitflag = (start != 0 || stop != 100) ? TRUE : FALSE;
   DPRINT2(0, "\nfid_start  = %d fid_end = %d\n\n", fid_start, fid_end);
}

long Getfid()
{
#if defined(INOVA) || defined(MERCURY)
   int            *fid_addr;
   int            *pdt_start,
                  *pdt_78,
                  *pdt_end;
   register int   *scan_ptr;
   int             dt18,
		   incrx2 = 2,
	   	   incr = 1,
		   ishift,
                   avg_dat;
   long 	   mult = 1L;
   register long   sum_data;
   long		   fs_np;
   int		   fs_dpf;

   long		 getFidNp();
   int		 getFidDpf();

   DPRINT(0,"Getfid(): call getshimfid\n");
   fid_addr = (int *) getshimfid();
   fs_dpf = getFidDpf();
   fs_np = getFidNp();
   DPRINT3(0,"Getfid(): getshimfid returned: data: 0x%lx, dpf: %d, np = %ld \n",
		fid_addr, fs_dpf, fs_np);
   
   /* apint(&fs_args, codeoffset, FID_SHIM); */
/*---------------------------------------------------------*/
/*  now do an offset correct and sum the data for simplex */
/*---------------------------------------------------------*/
   /* pdt_start = fs_args.fid_table;	/* some local pointers for */
   pdt_start = fid_addr;	/* some local pointers for */
   ishift = (fs_dpf == 4) ? 2:3;
   /* pdt_78 = (fs_args.fid_table + ((fs_np >> ishift) * 7)); */
   pdt_78 = (fid_addr + ((fs_np >> ishift) * 7));
   pdt_78 = (int *)((long)pdt_78 & 0xfffffffeL);  /* make sure address is even */
   pdt_end = fid_addr + fs_np;	/* convenience */
   dt18 = fs_np >> 3;
   DPRINT5(0,"Getfid(): pdt_start: 0x%lx, ishift: %d, pdt_78: 0x%lx, pdt_end: 0x%lx, dt18: %d\n",
	pdt_start, ishift, pdt_78, pdt_end, dt18);
   if (fs_dpf == 4)
   {  /* for double precision data, when nt = 1, the high byte 
	 contains no useful information and is ignored.        */
      pdt_start++;
      pdt_78++;
      pdt_end += fs_np;
      incrx2 = 4;
      incr = 2;
   }
   DPRINT5(0,"Getfid(): pdt_start: 0x%lx, ishift: %d, pdt_78: 0x%lx, pdt_end: 0x%lx, dt18: %d\n",
	pdt_start, ishift, pdt_78, pdt_end, dt18);
   DPRINT2(0,"Getfid(): incrx2: %d, incr: %d\n",incrx2, incr);
/*-------------------------------------------------------*/
/* first find the offset in the reals */
/*-------------------------------------------------------*/
   sum_data = 0;
   for (scan_ptr = pdt_78; scan_ptr < pdt_end; scan_ptr += incrx2)
      sum_data += *scan_ptr;
   avg_dat = (int) (sum_data / dt18);
                    		/* divide by # points for average */
   DPRINT3(0, "Getfid(): pdt_78: 0x%lx sum_data: %ld real offset:  %d\n", 
	pdt_78,sum_data,avg_dat);
   for (scan_ptr = pdt_start; scan_ptr < pdt_end;
	scan_ptr += incrx2)
      *scan_ptr -= avg_dat;
/*------------------------------------------------------*/
/* now do the imaginaries */
/*------------------------------------------------------*/
   sum_data = 0;
   for (scan_ptr = pdt_78 + incr; scan_ptr < pdt_end;
	scan_ptr += incrx2)
      sum_data += *scan_ptr;
   sum_data /= dt18;
   avg_dat = (int) sum_data;
   DPRINT3(0, "Getfid(): pdt_78: 0x%lx sum_data: %ld imag offset: %d\n", 
		pdt_78,sum_data,avg_dat);
   for (scan_ptr = pdt_start + 1; scan_ptr <= pdt_end;
	scan_ptr += 2)
      *scan_ptr -= avg_dat;
/*-------------------------------------------------------*/
/* now sum up the fid and scale it */
/*-------------------------------------------------------*/
   sum_data = 0;
   DPRINT3(0, "Getfid(): pdt_start: 0x%lx data start: 0x%lx, pdt_end: 0x%lx\n",
          pdt_start, fid_addr, pdt_end);
   if (limitflag)
   {
      mult = (fs_dpf == 4) ? 2L : 1L;
      pdt_start = fid_addr + (mult * (long) fid_start * fs_np) / 100;
      pdt_end   = fid_addr + (mult * (long) fid_end   * fs_np) / 100;
      pdt_start = (int *)((long)pdt_start & 0xfffffffeL);  /* make sure address is even */
      if (fs_dpf == 4)
         pdt_start++;
      DPRINT5(0,
         "START: 0x%lx END: 0x%lx pdt_start: 0x%lx data start: 0x%lx  pdt_end: 0x%lx\n",
      fid_start, fid_end, pdt_start, fid_addr, pdt_end);
   }
   for (scan_ptr = pdt_start; scan_ptr < pdt_end; scan_ptr += incr)
   {
      if (*scan_ptr < 0)
	 sum_data -= *scan_ptr;
      else
	 sum_data += *scan_ptr;
   }
   DPRINT1(0, "Getfid(): Data table sum: %ld\n", sum_data);
   return (sum_data);
#endif
}

getfid()
{
#if defined(INOVA) || defined(MERCURY)
   int 	    i,
	    ix,
	    count = 0;
   long     sumdata;

   sumdata = Getfid() >> fs_scl_fct;
   while (sumdata > INTOVF)
   {
      sumdata >>= 1;
      count++;
   }
   if (count != 0)
   {
      fs_scl_fct += count;
      for (i=0; i<numvectors; i++)
   	 table[i].yvalue >>= count;
   }
   ix = (int) sumdata;
   DPRINT1(0, "getfid(): scaled data table: %d\n", ix);
   return (-ix);
#endif
}

