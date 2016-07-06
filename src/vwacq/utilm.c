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

#include "simplex.h"
#include "logMsgLib.h"

extern int      numvectors;	/* number of active shim coils */
extern int      bestindex;	/* best yvalue index, set in maxmincomp */
extern struct entry
{
   int             yvalue;   	 /* lock value, always negative */
   int             vector[NUMDACS];/* associated shim coil settings */
}               table[NUMDACS];

#define MAX_SHIMS  48
static int shimval[MAX_SHIMS];
#define Z0 1


int  daclimit;
int  shimset_z0;
/*
 *  We use to have an array of steps but it does not look like
 *  that feature is used to advantage yet.  Therefore, I removed
 *  it until it can be used.
 *
 *  static int  step[MAX_SHIMS];
 */
static int  step;
static int  z1ftoc,
            z2ftoc;		/* fine to coarse z1 & z2 relative scaling */

static long active;		/* indicates which coils ar adjusted */

int get_dac(index)
int index;
{
   return( shimval[index] );
}

all_dacs(arr,num)
int arr[];
int num;
{
   int index;
       num;

   if (num > MAX_SHIMS)
      num = MAX_SHIMS;
   for (index=0; index < num; index++)
      arr[index] = shimval[index];
}

#ifdef RTI_VXWORKS
set_dac(index, value)
int index, value;
{
   int dacs[8];
   int zero;

   zero = 0;
   dacs[0]=0;  /* Ingored */
   dacs[1]= index; /* Z0 DAC */
   dacs[2]= value;
   shimHandler( dacs, &zero, 3, 0 );
   shimval[index] = value;
}

#else
set_dac(index, value)
int index, value;
{
   short dacs[8];
   int bytes;

   dacs[0]=0;  /* Ingored */
   dacs[1]=(short) index; /* Z0 DAC */
   dacs[2]=(short) value;
   shimHandler( dacs, 3 );
   shimval[index] = value;
}
#endif

dac_limit()
{
   if ( (shimset_z0 <=2) || (shimset_z0 == 10) )
      daclimit = 2047;
   else
      daclimit = 32767;
   return(daclimit);
}

int get_maxdacs()
{
/*
 *  return(MAX_SHIMS);
 */
    return(NUMDACS);
}

void
set_shimset(val)
int val;
{
   shimset_z0 = val;
}

init_shim()
{
   int index;

   switch (shimset_z0)
   {
   case 0:
      z1ftoc = 0;       /* course-fine exists only for shim sets 1 and 2 */
      z2ftoc = 0;
      break;
   case 1:
   case 10:
      z1ftoc = -30;	/* z1 fine to coarse relative scale (30f to 1c) */
      z2ftoc = -50;	/* z2 fine to coarse relative scale (50f to 1c) */
      break;
   case 2:
      z1ftoc = -30;
      z2ftoc = -50;
      break;
   default:
      z1ftoc = 0;
      z2ftoc = 0;
      break;
   }
/*
 * when step was an array,  it was initialized here
 *
 *   for (index=0; index < MAX_SHIMS; index++)
 *      step[index] = 20;
 */
   step = 20;
   get_all_dacs(shimval, MAX_SHIMS);
}

int get_step(index)
int index;
{
   return(step);
}

set_active(val)
long val;
{
   active = val;
}

is_active(number)
/* Is coil active(is its DAC value changed in current simplex)? */
int             number;
{
   long            cmask;

   if (number >= 32)
      return (FALSE);
   cmask = 1L << number;
   if ((cmask & active) != 0)
      return (TRUE);
   else
      return (FALSE);
}

int c_f_ex(do_course)
int do_course;
{
   int qm = 0;

   if (z1ftoc != 0)   /* course-fine exists only for shim sets 1 and 2 */
   {
      if ((is_active(Z1C) == TRUE) && (do_course))
	 qm += ctof(Z1C, Z1F, z1ftoc);	/* prevent downscale on bad */
      if ((is_active(Z2C) == TRUE) && (do_course))
	 qm += ctof(Z2C, Z2F, z2ftoc);
      if (is_active(Z1F) == TRUE)
	 qm += ftoc(Z1F, Z1C, z1ftoc);
      if (is_active(Z2F) == TRUE)
	 qm += ftoc(Z2F, Z2C, z2ftoc);
   }
   return(qm);
}

set_c_f_dac(use_course)
int use_course;
{
   if (z1ftoc != 0)   /* course-fine exists only for shim sets 1 and 2 */
   {
      if (use_course)
      {
         if (is_active(Z1F) == TRUE)
	    swap_active(Z1C, Z1F);
         if (is_active(Z2F) == TRUE)
	    swap_active(Z2C, Z2F);
      }
      else
      {
         if (is_active(Z1C) == TRUE)
	    swap_active(Z1F, Z1C);
         if (is_active(Z2C) == TRUE)
	    swap_active(Z2F, Z2C);
      }
   }
}

char *shim_nm[] = {
        "z0f",         /* Dac  0  */
        "z0",          /* Dac  1  */
        "z1",          /* Dac  2  */
        "z1c",         /* Dac  3  */
        "z2",          /* Dac  4  */
        "z2c",         /* Dac  5  */
        "z3",          /* Dac  6  */
        "z4",          /* Dac  7  */
        "z5",          /* Dac  8  */
        "z6",          /* Dac  9  */
        "z7",          /* Dac 10  */
        "",            /* Dac 11  */
        "zx3",         /* Dac 12  */
        "zy3",         /* Dac 13  */
        "z4x",         /* Dac 14  */
        "z4y",         /* Dac 15  */
        "x1",          /* Dac 16  */
        "y1",          /* Dac 17  */
        "xz",          /* Dac 18  */
        "xy",          /* Dac 19  */
        "x2y2",        /* Dac 20  */
        "yz",          /* Dac 21  */
        "x3",          /* Dac 22  */
        "y3",          /* Dac 23  */
        "yz2",         /* Dac 24  */
        "zx2y2",       /* Dac 25  */
        "xz2",         /* Dac 26  */
        "zxy",         /* Dac 27  */
        "z3x",         /* Dac 28  */
        "z3y",         /* Dac 29  */
        "z2x2y2",      /* Dac 30  */
        "z2xy",        /* Dac 31  */
   };
#define NUMSHIM  (sizeof(shim_nm) / sizeof(char *))
/*---------	Prints DAC label. */
dac_lbl(jj)
int             jj;
{
   if (jj < NUMSHIM)
      printf("%6s",shim_nm[jj]);
   return 1;
}

/* COARSE & FINE handler works this way:
	Each loop looks at coarse spreads.  Spreads are differences between
	max and min DAC values in vector table.  If below thresholds, then 
        marks in fines, marks out coarses, and recasts with similar spread.
	When fines are active, the rangechecker flags boundary hits on
	fines and finds spread of values, sets the centers on the coarse
	by a div mod (all coarses are equal) and remeasures.

	Included: ctof(), ftoc(), swap_active(), spread().

	phil hornung varian central research
*/

int
ctof(cnum, fnum, relscale)	/* cnum is active coil*/
/*------------------------ Switches active to fine if spread of DAC values
			   is low (<5).  Part of COARSE & FINE handler. 
			   This code solves the coarse problem completely */
int             cnum,
                fnum,
                relscale;
{
   int             center,
                   i,
                   spreadval;

   spread(&center, &spreadval, cnum);
   if (spreadval < daclimit/200)    /*  spread < 5  */
   {
      DPRINT(0, "changing scale to fine  \n");
      for (i = 0; i < NUMDACS; i++)
         if (is_active(i) == TRUE)
         {
	    set_dac(i,table[bestindex].vector[i]);
         }
      swap_active(fnum, cnum);
/*
 * This suggests a use for an array of step.  However,  step was used once in simplex
 * before this routine is called and step is never used again.
 *
 *    step[fnum] = relscale;
 */
      return (4);
   }
   return (0);
}

swap_active(in, out)
/*------------------ Switches active DAC between fine and coarse. */
int             in,
                out;
{
   active &= ~(1L << out);
   active |= (1L << in);
}

int
ftoc(fnum, cnum, relscale)	/* assumes fnum is active */
/*------------------------ Resets coarse when fine nears boundary.
			   Part of COARSE & FINE handler. */
int             fnum,
                cnum,
                relscale;
{
   int             i,
                   center,
                   y, /* spread value */
                   newc;
   long 	   limit1;
   long            ltmp1,ltmp2;

/* this code solves the fine problem boundary */
   limit1 = (long) (daclimit / 4) * 3;
   spread(&center, &y, fnum);
   ltmp1 = (long) center + (long) y;
   ltmp2 = (long) center - (long) y;
   if ((ltmp1 > limit1) || (ltmp2 < -limit1))
   {				/* slide it all but first check */
      newc = center / relscale;

      y = get_dac(cnum) - newc;
      set_dac(cnum,y);
      DPRINT1(0, "altering fine scale by moving coarse %d units  \n",newc);
#ifdef DEBUG
      if ((center < relscale) && (center > -relscale))
	 DPRINT(0, "should do coarse\n");
#endif

      for (i = 0; i < numvectors; i++)
      {
	 table[i].vector[cnum] = y;
	 table[i].vector[fnum] -= center;
      }
      return (1);
   }
   else
      return (0);
}

spread(cntr, width, num)
/*---------------------- Finds center and spread of DAC values.
			 Part of COARSE & FINE handler. */
int            *cntr,
               *width,
                num;
{
   int             min,
                   max,
                   tmp,
                   i;
   long            ltmp;

   max = table[0].vector[num];
   min = max;
   for (i = 1; i < numvectors; i++)
   {
      tmp = table[i].vector[num];
      if (tmp > max)
	 max = tmp;
      else if (tmp < min)
	 min = tmp;
   }
   ltmp = (long) max + (long) min;
   ltmp >>= 1;
   *cntr = (int) ltmp;
   ltmp = (long) max - (long) min;
   ltmp >>= 1;
   *width = (int) ltmp;
}

int adj_step(index,chk1)
int index, chk1;
{
   if (z1ftoc == 0)   /* for shim sets other than 1 and 2 */
   {
      if (chk1 && ((index == Z1F) || (index == Z2F)) )
         return(2);
      else if ((index == Z1F) || (index == Z2F))
         return(32);
      else
         return(1);
   }
   else if (chk1 && (index == Z1C))
      return(2);
   else
      return(1);
}

int adj2_step(index,chk1,chk2)
int index, chk1, chk2;
{
   if (z1ftoc == 0)   /* for shim sets other than 1 and 2 */
   {
      if (chk1 && ((index == Z1F) || (index == Z2F) || (index == Z3) )
	    || (chk2      && (index == Z3  || index == Z4 )) )
         return(2);
      else if ((index == Z1F) || (index == Z2F))
         return(32);
      else
         return(1);
   }
   else if ((chk1 && (index == Z1C || index == Z2C || index == Z3))
	    || (chk2      && (index == Z3  || index == Z4 )) )
      return(2);
   else
      return(1);
}
