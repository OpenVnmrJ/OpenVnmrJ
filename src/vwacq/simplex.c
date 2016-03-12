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
/*-------------------------------------------------------------------

Simplex.c Changes: 
   Oned() added, an improved method of adjusting a single coil.(6-27-89)

   (6-22-89)
   Does not use fltpak.c (Floating-Point Package).
   Main loop redone eliminating expansion() and contraction().
   Omega calculation which cycles recast step size.
   Changes to lock_check to allow lower lock levels.
   Reduction of yspread for shifting to fine to 5.
   Setlimit uses lock noise to set exit criteria in tst_end()
   Uses getyvalues() in place of recast() if recast_age <= 3.

   x_stp and y_spr defined relative to LIMIT and SIG2BIG. (3-8-89)
   Recast_age used, prevents recast when recast_age < 3. (3-8-89)
   fox is REBORN as background mode.

   In trackbest changed to (age > 6)
   In endcondition loop_min reduced.

A program to adjust the homogeneity of of a magnetic field using the
simplex method.

A simplex is a set of 3 points in the parameter space.
A function to be minimized is measured at these three points.
Each point has a vector, the coordinate values which are the DAC settings
for the shim coils for each point, and a function value also called the
y-value associated with it. All this is stored in the structure 'table'.
Normally in magnet shimming, it is desired to maximize the function.
It is replaced by its negative so that one searches for a minimum,
which is usual with the simplex method.

From the three points a centroid is calculated, in which the
average values of the vector components weighted by the associated 
y-value is used.  A new point is determined on the opposite side of the 
centroid from the worst point and the worst point is eliminated. If this
new point is less than lowest, the simplex is enlarged.  If it is greater
than the highest then the simplex is reduced. Eventualy the simplex will
converge around the lowest point.

The program provides for selecting the coils that are to be adjusted
in the simplex. The dimension of the simplex is the number of active coils.
The simplex method used here has been modified by extensive experiments to
improve convergence and to keep it out of trouble from a number of causes
such as bad lock level measurements or approaching too close to DAC limits.
The procedure recast throws out the values in the table and replaces them
with new ones as determined in selfsize.

The criteria, loose, medium, tight, and excellent have costants associated
with them which also have been experimentally adjusted, so that if better
shimming is desired, it can be achieved using higher criteria.

The simplex method is briefly described by Ernst in a paper covering work
done at Varian. (Measurement and Control of Magnetic Field Homogeneity,RSI,
Vol. 39, p 998). 
-----------------------------------------------------------------------*/

/* In stubio_c:	 get_io(v,threshold)
   			Gets lock level for DAC settings (v).
   		 setshim(dac#,dacvalue)
   			Sets a DAC.
*/

#include "simplex.h" 
#if defined(INOVA) || defined(MERCURY)
#include "lock_interface.h" 
#endif
#include "logMsgLib.h" 

typedef unsigned long time_t2;

extern int      mode_flag;

extern char     criteria,
                f_crit;
extern int      bestindex,      /* best yvalue index, set in maxmincomp */
				/* also set to 0 in autotask.c */
                delaytime,
                delayflag,
	        fox,		/* 0 normal, 1 start up background, 2 continue */
                host_abort,
	        hostcount,	/* host mark that goes 1-9 to indicate action */
	        lostlock,	/* lock lost global flag */
		n_level,	/* noise level, set in stubio.c */
                new,
 	        rho,
                loopcounter,
	        maxtime,
                numvectors,	/* number of active shim coils */
	        slewlimit;
extern int      cmdrecvd;	/* mailbox full flag */
extern int      fid_shim;	/* true for fid shim, false for lock */
extern int      fs_scl_fct;	/* scaling factor for fidshim */
extern int      interact;	/* interactive program flag */
extern int      fs_dpf;		/* fidshim dp flag, 0 for int, 4 for long */

extern long 	codeoffset; 	/* for fidshim */
extern long 	fs_np;          /* # points for fidshim */
extern int controlMask;

extern time_t2   timechk;

struct entry
{
   int             yvalue;
   int             vector[NUMDACS];
}               table[NUMVECS];

extern int      errormessage(); /* gives lock error message  */

static int      age,		/* number of simplexes to last best */
	        recast_age,
                barrier,  	/* slewlimit, based on noise level */
                bestval,	/* found from 'table',  best yvalue */
                worstval,	/* found from 'table', worst yvalue */
				/* bestval is the most negative     */
                centroid[NUMDACS], /* see make_centroid */
                pstar[NUMDACS], /* initial excursion of simplex */
                pstarstar[NUMDACS], /* excursion of simplex in contraction() */
                e_tog,		/* set in trackbest */
                endtest,	/* set in trackbest, abs(bestval-worstval) */
                errorflag,      /* if true, calls for aborting autoshim */
		firstactive,
                kappa,
		limit,		/* used by tst_end */
                lock_err,	/* used to set do_restart if poor lock level */
                lastindex,
		numactive,	/* number active DACs */
                maxdacs,
                daclimit,
                g_maxgn,
                range_err,	/* counts approches to limits of DACs */
		recastflag,
                star,
                startval,
                step_dir,
	        worstindex,     /* worst yvalue index, set in maxmincomp */
		x_step,
		y_spread,
                ystar,		/* yvalue of new point in simplex_hunt() */
                ystarstar;	/* yvalue of new point in contraction() */
static long     oldtime = 0L,
                totime = 0L;

#if defined(INOVA) || defined(MERCURY)
static int failAsserted = 0;
#else
extern int failAsserted;
#endif

int numact()
   /*----------*/
{
  int count = 0, j;

  firstactive = -1;
  for (j=0; j<maxdacs; j++)
    {
       if (is_active(j)) 
       {
	  count++;
	  if (firstactive < 0) firstactive = j;
       }
    }
  return(count);
}

simplex_hunt(maxgain)
int maxgain;
/*----------- main of automatic shimming program */
{
   /* DPRINT2(0, "In simplex_hunt: CRITERIA=%c F_CRIT=%c\n",criteria,f_crit);
   */
   oldtime = 0L;
   maxdacs = get_maxdacs();
   daclimit = dac_limit();
   bestindex = worstindex = 0;
   g_maxgn = maxgain;
   /*  pah note copies shimval */
   all_dacs(table[bestindex].vector,
            sizeof(table[bestindex].vector)/sizeof(table[bestindex].vector[0]));
/*
   get_io or Get_io can not be called before check_io
 
 */
   table[bestindex].yvalue = 0;
   /***************************************** 
   if FOX == 2, you take the established value 
   *****************************************/
   if (fox != 2)
     n_level = noise();
   if (host_abort || failAsserted)
   {
       DPRINT(0, "abort from host");
       controlMask |= ABORT;
       return(0);
   }
   chktime();
   set_c_f_dac((int) ((criteria == 'l') || (criteria == 'b')));
   numactive = numact(); /* DPRINT1(0,"numactive = %d\n",numactive); */
   numvectors = numactive + 1;
   setupinitial();
   if (numactive == 1)
   { 
      return (oned());
   }
   else
   {
      return (run_simplex() );
   }
}

run_simplex()
/*------------
	Main of simplex adjustment of shimming.
	Setupinitial().
	Trackbest().
	Lock_check() if not fid_shim.
 	In loop which runs simplex until end condition is TRUE:
		Make_centroid().
		Makes pstar vector.  (pstar = 2 * centroid - oldworst).
  		Calls contraction if new lock value for new simplex is worst 
		  than old worst, otherwise calls tryexpansion.
		Trackbest().
		Heuristic recast mechanism (recast if rho <0).
		If it switches between coarse and fine, it gets new yvalue.
		Lock_check() if not fid_shim.
---------------*/
{
   int             i,
                   qm;

   /***********************/
   age = 1;
   if (fox != 2)
   {
     recast();
     trackbest(); /* could remove??? */
   }
   else
   {
     DPRINT(0,"FOX == 2\n");
     getyvalue();
   }
   maxmincomp();
   endtest = abs(bestval - worstval);
   /***********************/
   startval = table[bestindex].yvalue;
   barrier = check_io(worstval);
	      /* initializes slewlimit used in filter() called by get_io */
   print_table();
   if ((!fid_shim) && !host_abort && !failAsserted)
      lock_check(1);		/* ensure lock channel behaves properly  */

   				/* start main loop of simplex		 */
   while ((i = endcondition() != TRUE) && !host_abort && !failAsserted)
   {
      recast_age++;
      /* calculate best point and first step */
      make_centroid();
      /* calculate initial excursion pstar */
      veclincomb(centroid, TWO, table[worstindex].vector, MINONE, pstar);

      ystar = get_io(pstar, worstval);		      /* measure new point */
	
      if (ystar > worstval )		 		/*** STEP FAILED ***/
      {
         star = CONTRACT;
         rho -= 1;
         /* forward contraction */
         veclincomb(table[worstindex].vector,MINHALF,centroid,THREEHALVES,
		 pstarstar);
         ystarstar = get_io(pstarstar, worstval);
         if (ystarstar > worstval)
         {				       /* try backward contraction */
            veclincomb(table[worstindex].vector, HALF, centroid, HALF,
		 pstarstar);
            ystarstar = get_io(pstarstar, worstval);
         }
         if (ystarstar < bestval)
            rho += 1;
         if (ystarstar > worstval || age <= 3 || recast_age <= 3)
	 {
	    		    /* if age <= 3 or recast_age <= 3 will accept
	        	       backward contraction worst than worstval   */
	    insert_entry(pstarstar, ystarstar);
	    getyvalue();
	 }
	 else
	    recast();
      }
      else					     /*** STEP SUCCEEDED ***/ 
      {
         star = STEP;
         if (ystar < bestval)
         {					      /* try expansion !  */
            veclincomb(centroid, MINONE, pstar, TWO, pstarstar);
            ystarstar = get_io(pstarstar, ystar);
            /*  is expansion successful ?? */
            if (ystarstar < ystar)
            {			           /*  is expansion successful ?? */
	       star = EXPAND;
	       rho += 1;
	       insert_entry(pstarstar, ystarstar);
            }
         }
	 if (star == STEP)
	    insert_entry(pstar, ystar);
      }

      loopcounter += 1;

      trackbest();		/* check max & min and age of best point */

      /* HEURESTIC RECAST MECHANISM */
      if ((rho < 0) && (age > 1) && (numactive > 2) && !host_abort && !failAsserted)
      {
	 recast(); 
	 trackbest();
      }

      if (host_abort || failAsserted)
      {
	 DPRINT(0, "abort from host");
	 controlMask |= ABORT;
	 break;
      }

      print_table();

      qm = c_f_ex((int) (criteria != 'b'));
      if (qm > 2)
      {
	 DPRINT(0,"fine/course RECAST ");
         kappa = 4;
	 recast(); 
	 trackbest();
      }
      else if (qm != 0)
	 getyvalue();
      barrier = check_io(worstval);
      if ((!fid_shim) && !host_abort && !failAsserted)
	 lock_check(1);		/* ensure lock channel behaves properly */
   }

   qm = host_abort;   /* save current value */
   host_abort = 1;    /* prevent get_io from actually measuring levels; just set the dacs */
   DPRINT(0,"End of simplex_hunt.\n");
   i = get_io(table[bestindex].vector, 0);	/* set best DAC values */
   host_abort = qm;   /* reset to old value */
   i = table[bestindex].yvalue;
   return (-i);
}

#define DIV     250

use_criteria(x_stp,y_spr)
   int             *x_stp,
                   *y_spr;
{
   int e_yspread = SIG2BIG / DIV;
   switch (criteria)
   {
   case 'e':
      rho = 12;
     *x_stp = daclimit / 650; 	   /* 3 */
     *y_spr = e_yspread;	   /* 5 */
      break;
   case 't':
      rho = 10;
     *x_stp = daclimit / 400; 	   /* 5 */
     *y_spr = 2 * e_yspread;       /* 10 */
      break;
/*    case 'm':	rho = 8;  x_stp = 15; y_spr = 20+kappa*5; break; */
   case 'm':
      rho = 8;
     *x_stp = daclimit / 68;	   /* 30 */
     *y_spr = e_yspread * (6 + kappa);
      break;
   case 'l':
      rho = 6;
     *x_stp = daclimit / 68;	   /* 30 */
     *y_spr = e_yspread * (6 + 2 * kappa);
      break;
   case 'b':
   default:
      rho = 9;
     *x_stp = daclimit / 68;	   /* 30 */
     *y_spr = e_yspread * (8 + kappa);
   }
}

recast()
/*------ Sets rho, x_stp, y_spr and calls selfsize. */
/* a method of initializing 'table' or filling with new values when simplex
	appears not to be making progress. */
/* modified to re-size at differing scales */
{
   int             y_spr,
                   x_stp;

   star = RECAST;
   use_criteria(&x_stp,&y_spr);
   recast_age = 0;
   recastflag = 1;
   /* DPRINT2(0, "\n x_stp=%d, y_spr=%d \n",x_stp, y_spr); */
   age = 1;
   if (loopcounter == 1) age = 0;
   x_step = x_stp;   			    /* used in oned() */
   y_spread = y_spr;
   if (numvectors > 3)
   {
      DPRINT(0,"Start of SELF SIZE\n");
      selfsize(x_stp,y_spr);
   }
   else 
   {
      DPRINT(0,"Start of NEW NET\n");
      newnet(&x_stp);
   }
   if (--kappa < 1)
      kappa = 3;
}

newnet(xstep)
int             *xstep;

/*
     measure 1 point each active adjustment.
     as each coil is adjusted, track the measured response
       and use present best for next start.
*/
{
   int             yb,
                   index,
                   mkr = 1,
                   bestmkr = 0;

   if  (criteria == 'e' || criteria == 't')
      *xstep += (daclimit / 2000) * (kappa - 2);
   else if (kappa > 3)
      *xstep = 4 * *xstep / 3;
   else 
      *xstep >>= (3-kappa);
   *xstep *= step_dir;
   step_dir = -step_dir;
   vec_equate(table[0].vector, table[bestindex].vector);
   /* get starting point */
   yb = get_io(table[0].vector, 0);
   table[0].yvalue = yb;
   for (index = 0; (index < maxdacs) && !host_abort && !failAsserted; index++)
   {
      if (is_active(index) == TRUE)
      {
	 /* center point is present best */
	 vec_equate(table[mkr].vector, table[bestmkr].vector);
	 table[mkr].vector[index] += *xstep / adj_step(index,
              ((criteria == 'l') || (criteria == 'b')) );
	 rangecheck(table[mkr].vector);
	 table[mkr].yvalue = get_io(table[mkr].vector, 0);
	 /*
	  * if any point is better than initial one, then use it for
	  * succeeding moves
	  */
	 if (yb > table[mkr].yvalue)
	 {
	    bestmkr = mkr;
	    yb = table[mkr].yvalue;
	 }
	 mkr++;
      }
   }
}

selfsize(inxstep, yspread)
/*----------------------
     	Measure 3 points for each active adjustment.
     	If center > (average - yspread) of sides then bowed downwards
      		response => cut step size & place it on down side if necessary.
     	If center on line between sides then surface is flat
       		extend line to achieve good response.
     	As each coil is adjusted, track the measured response
       		and use present best for next start.
     	Excursion reduced when high curvature and domed field.
     	Extreme excursions modified.
	For criteria 'm', xstep = 30, yspread = 30 to 50. 
	Usually excursion = xstep.
-----------------------*/
int             inxstep,
                yspread;

{
   int             index,
                   mkr,
                   bestmkr,
                   tempvec[NUMDACS];
   int             yc,
                   ym,
                   yp,
                   yb,
                   avg,
                   excursion,
                   deltay,
		   xstep,
                   sign,
		   excr0;

   vec_equate(table[0].vector, table[bestindex].vector);	/* vout = vin */
   bestmkr = 0;
   excr0 = 0;
   /* get starting point */
   yb = get_io(table[0].vector, 0);
   table[0].yvalue = yb;
   mkr = 1;
   for (index = 0; (index < maxdacs) && !host_abort && !failAsserted; index++)
   {
      if (is_active(index) == TRUE)
      {
	 /* center point is present best */
         if (fid_shim) yc = get_io(table[0].vector, 0);
	 else yc = yb;
	 vec_equate(tempvec, table[bestmkr].vector);
	 vec_equate(table[mkr].vector, tempvec);
	 xstep = inxstep / adj2_step(index,
                 ((criteria == 'l') || (criteria == 'b')),fid_shim);
	 tempvec[index] += xstep;
	 /* make positive step in this dac */
	 yp = get_io(tempvec, yb / 2);
	 /* it got better - step on moving the center to yp  etc */
	 if (yp < (yc-10)) 
	 {
	    ym = yc;
	    yc = yp;
	    /* make negative step in this dac */
	    tempvec[index] += xstep;
	    yp = get_io(tempvec, yb / 2);
	    excr0 = xstep;
	 }
	 /* it did not get better --- */
	 else
	 {
	   /* make negative step in this dac */
	   tempvec[index] -= 2 * xstep;
	   ym = get_io(tempvec, yb / 2);
	 }
	 /* DPRINT4(0,"yp=%d yc=%d ym=%d xstep=%d\n",yp,yc,ym,xstep);*/
	 /* if its linear avg will equal yb if bowed yb<avg */
	 avg = (yp + ym) / 2;
	 deltay = (yp - ym) / 2;
	 /* set up slope direction */
	 if (deltay >= 0)
	    sign = 1;
	 else
	    sign = -1;
	 /* is the surface bowed ? */
	 if ((yc - avg) < -yspread)
	 {			/* valley, remember best is down  */
	    DPRINT(0, "surface is bowed on ");
	    if (DebugLevel > 0) dac_lbl(index);
	    /*
	     * define deltay as difference between best of two sides and
	     * center you must make a step to preserve dimensionality limit
	     * to + or - step or less
	     */
	    if (yp < ym)
	       deltay = yc - yp;
	    else
	       deltay = yc - ym;
	    deltay = abs(deltay);
	    if (deltay > yspread)
	       excursion = (-sign * xstep * yspread) / deltay;
	    else
	       excursion = -sign * xstep;
	 }
	 else			/* its not bowed    */
	 {
	    deltay = abs(deltay);
	    excursion = -sign * xstep;
	    if (deltay > SIG2BIG / 300)	/* its flat !!! */
	       excursion += -sign * xstep * (deltay / (SIG2BIG * 10 / DIV));
	 }
	 /* excr0 handles a center shift... it could be changed to move to 
	 best in triplet!!! */
	 excursion += excr0;
	 /* problem handler */
	 if (abs(excursion) < daclimit / 1000)
	    excursion = -sign * daclimit / 1000;
	 if (abs(excursion) > daclimit / 10)
	    excursion = -sign * daclimit / 40;	/* fold insensitive back */
	 table[mkr].vector[index] += excursion;
	 rangecheck(table[mkr].vector);
	 table[mkr].yvalue = get_io(table[mkr].vector, 0);
	 /* 
	  * if any point is better than initial one, then use it for
	  * succeeding moves
	  */
	 if (yb > table[mkr].yvalue)
	 {
	    bestmkr = mkr;
	    yb = table[mkr].yvalue;
	 }
	 mkr += 1;
      }
   }
}

trackbest()
/*---------
	Checks for best point validity whenever a new best point has
   		been found or when age > 6.  Each new point is 
		checked to avoid bad best points from servo swing.
	Maxmincomp finds bestval and worstval (lock values) which are then
		used to find endtest.  */
{
   do
   {
      maxmincomp();
      if ((bestindex != lastindex) || (age > 6)) 
      {				/* check newest point */
	 age = 0;
	 table[bestindex].yvalue = get_io(table[bestindex].vector, 0);
      }
      else
	 age++;
      lastindex = bestindex;
   }
   while (age < 1);
   endtest = abs(bestval - worstval);
}

insert_entry(newvec, newval)
/*-------------------------- Replaces worst case vector by latest vector */
int             newvec[NUMDACS],
                newval;
{
   vec_equate(table[worstindex].vector, newvec);
   table[worstindex].yvalue = newval;
}

lock_check(mult) 
/*----------
  	Called by simplex_hunt, at start and after each new simplex result.
  	Sets errorflag if lock lost.
 	If signal too big, reduces lock gain, or if too small, increases gain.
  	If signal doubles does restart. */
int		mult;
{
#define DELTALGAIN 4;  /* when lock signal is too large, was 6, 6-90 */

   int             tempgain;
   int             do_restart = 0,
		   temp = 0,
		   deltagain = 6;

   if (!chlock())		/* if lock lost */
   {
      temp = get_io(table[bestindex].vector, 0);
      if (!chlock())		/* one more try before giving up */
      {
         lock_err++;
         controlMask |= LOCK_UNDER;
         errorflag = TRUE;
      }
   }
   if (table[bestindex].yvalue < -SIG2BIG)
   {
      tempgain = getgain();
      if (tempgain == 0)
      {
	 lock_err += 4;
	 controlMask |= LOCK_OVER;
      }
      else
	 lock_err = 0;
      tempgain -= DELTALGAIN;
      if (tempgain < 0)
	 tempgain = 0;
      setgain(tempgain);
      DPRINT1(0, "Lock Over Scale, Gain drop 4 db to %d \n",
	     tempgain);
      do_restart = 1;
   }
   if ( table[bestindex].yvalue > - mult * SIG2SML / 2)
   {
      lock_err++;
      tempgain = getgain();
      if (tempgain >= g_maxgn)
	 lock_err += 4;
      if (lock_err > 3)
	 controlMask |= LOCK_UNDER;
      if (g_maxgn - tempgain < deltagain)
      {
         deltagain = g_maxgn - tempgain;
      }
      
      if (deltagain >0)
      {
         tempgain += deltagain;
         setgain(tempgain);
         DPRINT2(0, "Lock Under Scale, Gain increased %1d db to %d  \n",
	     deltagain,tempgain);
         do_restart = 1;
      }
   }
   if (abs(table[bestindex].yvalue) >= 2 * (abs(startval)))	/* signal doubled? */
   {
      DPRINT2(0, "Signal Doubled %d -> %d, recalc noise & T1/2 \n",
	     startval, table[bestindex].yvalue);
      do_restart = 1;
   }
   if (lock_err > 3)
      errorflag = TRUE;
   if ((!errorflag) && !host_abort && !failAsserted && (do_restart))
      restart();
}

getyvalue()	
/*--------- Loads response values.
	    (Gets new lock values for table of DAC settings). */
{
   int             i;

   for (i = 0; i < numvectors; i++)
      table[i].yvalue = get_io(table[i].vector, 0);
}

endcondition()
/*------------
	Checks if time is up for simplex then if end conditions have been met.
	Calls tst_end(loop_min,loopmax,spread) which returns TRUE or FALSE
		depending on criteria.
	Resets loopcounter when criteria is changed. */
{
   int             tflg;

   if (chktime())
      return (TRUE);		/* is time up for simplex ? */
   if (errorflag)
   {
      errormessage();
      return (TRUE);		/* is time up for simplex ? */
   }
   /* now process normal */
   else
   {
      switch (criteria)
      {
      case 'b':
	 tflg = tst_end(15, 40, 2 * barrier);
	 break;
      case 'l':
	 tflg = tst_end(10, 30, 4 * barrier);
	 break;
      case 'm':
	 tflg = tst_end(10, 35, 2 * barrier);
	 break;
      case 't':
	 tflg = tst_end(12, 40, barrier);
	 break;
      case 'e':
	 tflg = tst_end(15, 50, barrier);
	 break;
      default:
	 tflg = TRUE;
	 criteria = f_crit;
      }
      if ((tflg == TRUE) && (criteria != f_crit))
      {
	 /* step criteria and not end condition */
	 switch (criteria)
	 {
	 case 'b':
	    criteria = 'l';
	    break;
	 case 'l':
	    criteria = 'm';
	    break;
	 case 'm':
	    criteria = 't';
	    break;
	 case 't':
	    criteria = 'e';
	    break;
	 }
	 loopcounter = 0;
         /* DPRINT1(0,"Criteria now %c\n",criteria); */
	 return (FALSE);
      }
      else
	 return (tflg);
   }
}

setlimit(exit_spread) int exit_spread;
/*-------------
	Returns value used in exiting test for simplex_hunt loop.  If 
	noise is low compared to exit_spread, uses value based on noise. */
{
   int mult = 1;

   limit = exit_spread;
   if (fid_shim ) 
   {
      mult = (3 * (- bestval)) / SIG2BIG;
      if (numactive > 4) mult *= 2;
      if (mult < 1) mult = 1; 
   }
   if (limit > 4 * mult * n_level) 
      limit  = 4 * mult * n_level;
   if (limit < 5) limit = 5; /* see check_io */
   if (fid_shim && (limit < exit_spread >> 1)) 
      limit = exit_spread >> 1;
   return (limit);
}

tst_end(loop_min, loop_max, exit_spread)
/*---------------------------------
	Returns TRUE count > loop_max, FALSE if < loop_min, if in between
		returns TRUE if endtest < exit_spread. */
int             loop_min,
                loop_max,
                exit_spread; /* multiple of barrier */
{
   limit = setlimit(exit_spread);
   if (loopcounter == 2 && limit != exit_spread)
     DPRINT(0,"### Limit Based On Measured Noise ###\n");

   if (loopcounter > loop_max)
      return (TRUE);		/* returns also exit */
   /* if ((loopcounter < loop_min && numactive > 1) || loopcounter < 10) */
   if (loopcounter < 4)
      return (FALSE); 
   if (age < 2)
   {
      if (endtest < limit) /* if ((max level - min level) < noise) */
	 e_tog = 0;
      return (FALSE);
   }
   if (endtest < limit)
   {
      if (e_tog == 0)
      {
         if (loopcounter < loop_min)
         {
	    if (star != RECAST && recast_age > 3 && criteria != 'b')
	    {   recast();
	        trackbest();
	    }
         }
         else	
	    return (TRUE); /* end condition has been met */
      }
      else
      {
	 e_tog = 0;
      }
   }
   return (FALSE);
}

make_centroid()
/*-------------
  	The centroid vector is calculated from current active DAC settings
	weighted by their locklevel relative to the worst value + 20. */
{
   int             i,
                   j,
	 	   itemp,
                   offset,
		   scalefac;
   long            temp,
                   tempsum,
		   product,
		   xx;
   for (i = 0; i < NUMDACS; i++)
      centroid[i] = table[bestindex].vector[i];
   /* make all values defined */
   offset = worstval + SIG2SML / 20;   /* adjust for fid_shim? */
   tempsum = 0L;
   for (i = 0; i < numvectors; i++)
      /* calculate offset */
      if (i != worstindex)
      {
	 tempsum += offset - table[i].yvalue;
      }

   /* calculate scale factor to prevent temp overflow, 
      assume yvalue is not more than 16 bits (32767),
      NUMDACS not more than 5 bits (32), scalefac = 16 + 5 - 31 = -10
   */
   scalefac = -10;
   if (tempsum < 1L)
      tempsum = 1L;
   xx = tempsum;
   while (xx < 524288L)
   {
      xx <<= 1;
      scalefac--;
   }
   itemp = daclimit;
   while (itemp > 1)
   {
     scalefac++;
     itemp >>= 1;
   }
   if (scalefac < 0) scalefac = 0;
   tempsum >>= scalefac;
   if (scalefac != 0) DPRINT1(0,"##### scalefac = %d #####\n",scalefac);

   for (j = 0; j < maxdacs; j++)
   {
      temp = 0L;
      if (is_active(j) == TRUE)
      {
	 for (i = 0; i < numvectors; i++)
	 {
	    if (i != worstindex)
	    {
	       product = (offset - table[i].yvalue) * table[i].vector[j];
	       temp += (product >> scalefac);

	    }
	 }
	 centroid[j] = (int) (temp / tempsum);
      }
   }
}

maxmincomp()
/*---------- Finds bestval and worstval (lock values). */
{			
   int             i,
                   tt;

   bestindex = worstindex = 0;
   bestval = worstval = table[0].yvalue;
   for (i = 1; i < numvectors; i++)
   {
      tt = table[i].yvalue;
      if (tt > worstval)
      {
	 worstval = tt;
	 worstindex = i;
      }
      else if (tt < bestval)
      {
	 bestval = tt;
	 bestindex = i;
      }
   }
   if (bestindex == worstindex)
      for (i = 0; i < numvectors; i++)
	 if ((i != bestindex) && (table[i].yvalue == worstval))
	    worstindex = i;
}

print_table()
/*----------- Prints table of DAC settings with signal level if debug is on.*/
{
   int             i,
                   j;

   if (DebugLevel)
   {
      printf("\n");
      screenline();
      printf("    Loopcounter = %3d  ", loopcounter);
      if (recastflag && star != RECAST) 
      {
	 printf("RECAST ");
      }
      recastflag = 0;
      switch (star)
      {
      case STEP:
	 printf("Step     ");
	 break;
      case EXPAND:
	 printf("Expand   ");
	 break;
      case CONTRACT:
	 printf("Contract ");
	 break;
      case RECAST:
	 printf("RECAST   ");
	 break;
      default:
	 printf("nop");
      }
      printf("age = %2d  rho = %3d\n", age, rho);
      printf("Present criteria is %c to %c\n\n        ",
	 criteria, f_crit);
      for (j = 0; j < maxdacs; j++)
	 if (is_active(j) == TRUE)
	    dac_lbl(j);
      printf("\n");
      for (i = 0; i < numvectors; i++)
      {
	 if (i == worstindex)
	    printf("w>");
	 else if (i == bestindex)
	    printf("b>");
	 else
	    printf("  ");
	 printf("%6d", table[i].yvalue);
	 for (j = 0; j < maxdacs; j++)
	    if (is_active(j) == TRUE)
	       printf("%5d ", table[i].vector[j]);
	 printf("\n");
      }
      if (loopcounter > 1)
         printf("endtest/limit =   %5d/%3d\n", endtest, limit);
      screenline();
   }				/* end if != Q */
}

screenline()
/*----------  	Prints border for result table. */
{
   printf("--------------------------------------------------\n");
}

veclincomb(v1, c1, v2, c2, vout)
/*------------------------------
  	Output vector, vout, is a linear combination v1*c1 and v2*c2 */
int             v1[NUMDACS],
                c1,
                v2[NUMDACS],
		c2,
                vout[NUMDACS];

{
   int             j;

   for (j = 0; j < maxdacs; j++)
   {
      if (is_active(j) == TRUE)
      {
	 vout[j] = c1 * (v1[j]/2) + c2 * (v2[j]/2);
      }
      else
	 vout[j] = table[bestindex].vector[j];
   }
   rangecheck(vout);
}

setupinitial()	
/*------------	Sets globals for search. */
{
   barrier = check_io(SIG2BIG/2);
	       	      /* initializes n_aves and slewlimit in stubio.c */
   host_abort = 0; /* set by methoda in stubio.c.
				 Means problem trying to read lock signal. */
   resolve_coils();
   e_tog = 0;
   switch (criteria)
   {
   case 'e':
      rho = 12;
      e_tog = 1;
      break;
   case 't':
      rho = 10;
      e_tog = 1;
      break;
   case 'm':
      rho = 8;
      break;
   case 'l':
      rho = 6;
      break;
   case 'b':
      rho = 10;
   }
   endtest = 1000;
   age = 1;
   kappa = 4;
   lastindex = bestindex;
   range_err = 0;
   lock_err = 0;
   errorflag = FALSE;
   star = STEP;
   loopcounter = 1;
   hostcount = 1;
   step_dir = 1;
}

resolve_coils()	
/*------------	Initializes table based on existing DAC values in 
	        table[bestindex] and on step, all set to 20 in autotask.c.
  		Counts number of active coils. */
{
   int compind,
       vecind;

   vecind = 0;
   for (compind = 1; compind <= numvectors; compind++)
   {
      vec_equate(table[compind].vector, table[0].vector);
      table[compind].yvalue = 0;
      /* find active vectors */
      while ((is_active(vecind) != TRUE) && (vecind < NUMDACS))
	 vecind++;
      if (vecind < NUMDACS)
	 table[compind].vector[vecind] += 20;
      vecind++;
   }
}

rangecheck(vchk)
/*--------------    Checks if any DAC settings are out of range. If so 
		    resets within limit.  Stops simplex with more than 5
		    approaches to limit. */
int             vchk[NUMDACS];
{
   int             i,
                   temp;

   for (i = 0; i < NUMDACS; i++)
   {
      if ((vchk[i] > daclimit) || (-vchk[i] > daclimit))
      {				/* confine to boundary area */
	 temp = vchk[i];
	 if (temp > 0)
	    temp = daclimit - daclimit / 40;
	 else
	    temp = -daclimit + daclimit / 40;
	 vchk[i] = temp;

#ifdef DEBUG
	 DPRINT(0, "range limit on ");
	 dac_lbl(i);
	 DPRINT(0, "\n");
	 DPRINT1(0, " corrected value = %d\n", vchk[i]);
#endif

	 range_err++;
      }				/* limiter */
   }				/* loop */
   if (range_err > 6)
   {
      errorflag = TRUE;
      controlMask |= DAC_RANGE;
   }
/* quit when repeated range problems occur */
}

vec_equate(vout, vin)
/*------------------- Sets one DAC-setting vector equal to another. */
int             vout[NUMDACS],
                vin[NUMDACS];
{
   register int    jj;

   for (jj = 0; jj < NUMDACS; jj++)
      vout[jj] = vin[jj];
}

restart()
/*------- Recalculate noise level & T1/2  and 
	  reevaluates all vertices of simplex. */
{
   int  i;

   adjustGain();
   n_level = noise();
   if (!host_abort && !failAsserted)
   {
      check_io( worstval); /* should be ok */
      if (numactive != 1) DPRINT(0, "restart");
      for (i = 0; i < numvectors; i++)
	 table[i].yvalue = get_io(table[i].vector, 0);
      maxmincomp();		/* find best & worst vector */
      startval = table[bestindex].yvalue;
   }
}

chktime()
/*------- Check the present elapsed time to the maximum shimming
    	  time that was entered in the method (0 time = infinity). */
{
   int newtime;


   newtime = secondClock(&timechk,0);
   if (DebugLevel)
      printf("Elapsed Time = %4dmin %2d sec \n", newtime / 60, newtime % 60);
   /* if maxtime is Zero, No time limit */
   if (maxtime == 0) return(FALSE);
   if (newtime >= maxtime)			
   {
   	 DPRINT(0,"Time Limit Reached\n");	
	 return (TRUE);
   }
   return (FALSE);
}

/* "  oned.c  - c version from pascal integer version  " */

#define COUNTLIMIT  6
#define ITMAX       12
#define XLIMIT      10
#define TOL         500
#define MAXLONG   0x7ffffffL
#define GOLD 13
#define GOLDD 8
#define GLIMIT 3
#define TINY  1L
#define DIVIDER  4096L
#define DELTAMIN 10
 /* glimit = 1.625; set glimit to golden mean value to avoid large changes */
 /* TINY = 1 prevents division by zero */

int             xmin,
		dummy = 1;

static int swap(a, b)
         /*----------*/
int            *a,
               *b;
{
   int             temp;

   temp = *a;
   *a = *b;
   *b = temp;
}

static long max_simp(a, b)
          /*---------*/
long            a,
                b;
{
   if (a > b)
      return (a);
   else
      return (b);
}

static long l_abs(a)
long 	a;
{
   if (a < 0L)
      return (- a);
   else
      return (a);
}

static long sign(a, b)
          /*---------*/
long            a,
                b;
{
   long            absa;

   absa = l_abs(a);
   if (b > 0)
      return (absa);
   else
      return ( - absa);
}

onedGet_io(ix,bar)
  int ix,bar;
{
   int i;
   table[0].vector[firstactive] = ix;
   rangecheck(table[0].vector);
   i = get_io(table[0].vector,bar);
   return (i);
}

int setu(cx,bx,deltamin)
int 		*cx, *bx, deltamin;
{
   int 	delta;

   if (abs(delta = GOLD * (*cx - *bx) / GOLDD) < deltamin)
   {
      if (*cx > *bx) delta = deltamin; else delta = -deltamin;
   }
   return (*cx + delta);
}

int mbrak(ax, bx, cx, fa, fb, fc)
/*--------------------------------------------------------------------------
|	given a function func and distinct initial points ax and bx,
|	searches in a downhill direction, and returns new points, ax, bx,
|	cx, which bracket the minimum, and function values at ax,bx, and cx.
*--------------------------------------------------------------------------*/
int            *ax,
               *bx,
               *cx,
               *fa,
               *fb,
               *fc;
{
   int             ulim,
                   u,
		   fbb,
                   fu,
		   Cflag,
                   flag1 = 1,
                   count = 0,
		   divider = 1,
	           olddivider = 1,
		   t_step;
   long            q,
                   r,
                   temp,
                   temp2,
		   oldtemp2 = 1,
                   max_simp(),
                   sign();
   *ax = table[0].vector[firstactive];
   t_step = x_step / 2;
   *fa = onedGet_io(*ax,0);
   do
   {
      count++;
      t_step *= 2;
      *bx = *ax + t_step;
      *fb = onedGet_io(*bx,0);
   }
   while ((abs(*fa - *fb) < barrier) && (count <4));
   if (*fb > *fa)
   {
      swap(ax, bx);
      swap(fa, fb);
   }
   
   *cx = setu(bx,ax,DELTAMIN);
   *fc = onedGet_io(*cx,0);
   DPRINT(0,"          ax      bx      cx        fa      fb      fc\n");
/* 1 */
   count = 0;
   while (!host_abort && !failAsserted && flag1
           && (*fb > *fc - barrier) && count < COUNTLIMIT)
   {
      Cflag = 0;
      count++;
      DPRINT6(0,"     %7d   %5d   %5d   %7d   %5d   %5d\n",
		*ax, *bx, *cx, *fa, *fb, *fc);
      flag1 = 0;
      r = ((long) (*bx - *ax)) * ((long) (*fb - *fc));
      q = ((long) (*bx - *cx)) * ((long) (*fb - *fa));
      temp2 = 2L * sign(max_simp(l_abs(q - r), TINY), q - r);
      if ( l_abs(q) > MAXLONG / DIVIDER || l_abs(r) > MAXLONG / DIVIDER)
      {  /* protect against long overflow */
         divider = DIVIDER;
	 q /= DIVIDER;
         r /= DIVIDER;
      }
      else
         divider = 1;
      temp = ((*bx - *cx) * q - (*bx - *ax) * r);
      if (temp2==0)
      {  temp2 = oldtemp2;
	 divider = olddivider;
      }
      u = *bx - temp / temp2 / divider;
      oldtemp2 = temp2;
      olddivider = divider;
      ulim = *bx + (GLIMIT * (*cx - *bx));
      if (abs(ulim - *bx) < x_step || abs(ulim - *cx) < x_step)
      	 if (*cx > *bx) ulim = *cx + x_step; else ulim = *cx - x_step;
      if ((*bx - u) * (u - *cx) >= 0 )	/* parabolic u is between b and c */
      {
	 fu = onedGet_io(u,dummy);
	 if (fu + barrier <= *fc)
	 {
	    /* ch = 'A';/* DPRINT(0,"A#"); */
	    *ax = *bx;
	    *fa = *fb;
	    *bx = u;
	    *fb = fu;
	    return 1;		/* or flag1=true; exits */
	 }
	 else if (fu >= *fb + barrier)
	 {
	    /* ch = 'B';/* DPRINT(0,"B#"); */
	    *cx = u;
	    *fc = fu;
	    return 1;		/* or flag1=true; exits */
	 }
	 if (!flag1)
	 {
	    Cflag = 1;
	    /* ch = 'C'; /* DPRINT(0,"C#\n"); */
	    u = setu(cx,bx,1);
	    fu = onedGet_io(u,dummy);
	 }
      }
      else if ((*cx - u) * (u - ulim) > 0)
      {
	 /* ch = 'D';/* DPRINT(0,"D#"); */
	 fu = onedGet_io(u,dummy);
	 if (fu < *fc)
	 {
	    *bx = *cx;
	    *cx = u;
	    u = setu(cx,bx, DELTAMIN);
	    *fb = *fc;
	    *fc = fu;
	    fu = onedGet_io(u,dummy);
	 }
      }
      else if ((u - ulim) * (ulim - *cx) >= 0)
      {
	 /* ch = 'E';/* DPRINT(0,"E#"); */
	 u = ulim;
	 fu = onedGet_io(u,dummy);
      }
      else
      {
	 /* ch = 'F';/* DPRINT(0,"F#"); */
	 u = setu(cx, bx, DELTAMIN);
	 fu = onedGet_io(u,dummy);
      }
      if (*fb > *fa + barrier  && *fc > *fb + barrier)
      {
         swap(ax, cx);
         swap(fa, fc);
	 flag1 = 1;
	 *cx = setu(bx,ax,DELTAMIN);
	 *fc = onedGet_io(*cx,dummy);
      }
      else if (!flag1)
      {
	 if (!Cflag) *ax = *bx;
	 *bx = *cx;
	 *cx = u;
	 if (!Cflag) *fa = *fb;
	 *fb = *fc;
	 *fc = fu;
	 flag1 = 1;
 	 fbb = *fb + barrier;
	 if (*fa >= fbb && *fc >= fbb) 
	   {  /* DPRINT(0,"Q#"); */
              return 1;
	   }
      }
   }				/* while flag1 if (*fb >=*fc) */
   if (count >= COUNTLIMIT) return (0); else return (1);
}

pbrent(ax, bx, cx, fa, fxx, fb, xmin, brent)
/*---------------------------------------------------------------------
|	Given a function F, and give a bracketing triplet of abscissas
|	ax, bx, cx(bx is between ax and cx) and f(bx) < both f(ax) and
|	f(cx), isolates minimum to precision TOL using Brent's method.
|	Returns minimum, xmin, and minimum function value, brent.
*---------------------------------------------------------------------*/
int             ax,
                bx,
                cx,
	       *fa,
	       *fxx,
	       *fb,
               *xmin,
               *brent;

#define CGOLD  3
#define ZEPS   1
{
   int             a,
                   b,
                   d = 1,
                   e,
                   etemp,
                   fu,
                   fv,
                   fw,
                   fx,
		   oldfx = 0,
                   tol1,
                   tol2,
                   u = 0,
                   v,
                   w,
                   x,
                   xm,
                   flag1,
                   flag3,
                   iter;
   long            p,
                   q,
                   r;

   if (ax < cx) a = ax; else a = cx;
   if (ax > bx) b = ax; else b = cx;
   v = bx;
   w = v;
   x = v;
   e = 0;
   fx = *fxx;
   fv = fx;
   fw = fx;
   if (fid_shim)
      tol1 = (daclimit / ( TOL * 3 / 2)) + ZEPS;
   else
      tol1 = (daclimit / TOL) + ZEPS;
   tol2 = 2 * tol1;
   for (iter = 1; iter < ITMAX; iter++)
   {
      flag1 = 1;
      if (oldfx == fx) age += 1;
      else age = 1;
      flag3 = (!fid_shim && iter < 4 && age > 1);
      if (flag3 && (host_abort == 0) )
   	fx = (fx + onedGet_io(x,dummy))/2;
      DPRINT7(0,"%2d   %7d   %5d   %5d   %7d   %5d   %5d\n", 
	 iter, a, x, b, *fa,fx,*fb);
      oldfx = fx;
      xm = a/2 + b/2;  /* careful about int 32k  (a + b)/2 might overflow */
      if ( (abs(x - xm) <= (tol2 - (b - a) / 2)) || host_abort || failAsserted )
      {
	 /* DPRINT(0," A\n"); */
	 *xmin = x;
	 *brent = fx;
	 return 1;
      }
      if (abs(e) > tol1)
      {
	 /* DPRINT(0," B"); */
	 r = (x - w) * (fx - fv);
	 q = (x - v) * (fx - fw);
         if ( l_abs(q) > MAXLONG / DIVIDER || l_abs(r) > MAXLONG / DIVIDER)
         {  /* protect against long overflow */
	    q /= DIVIDER;
            r /= DIVIDER;
         }
	 p = (x - v) * q - (x - w) * r;
	 q = 2 * (q - r);
	 if (q > 0)
	    p = -p;
	 q = l_abs(q);
	 etemp = e;
	 e = d;
	 flag1 = ((l_abs(p) >= l_abs(q * etemp / 2)) || (p <= q * (a - x)) ||
		  (p >= q * (b - x)));
	 if (!flag1)		/* parabolic fit is ok */
	 {
	    /* DPRINT(0," C\n"); */
	    d = p / q;
	    u = x + d;
	    if (((u - a) < tol2) || ((b - u) < tol2))
	       d = sign((long)tol1, (long)(xm - x));
	 }
      }
      if (flag1)
      {
	 /* DPRINT(0," D\n"); */
	 if (x >= xm)
	    e = a - x;
	 else
	    e = b - x;
	 d = ((CGOLD * e) / 8);
	 if (d == 0)
	    d = 1;
      }
      if (abs(d) >= tol1)
	 u = x + d;
      else
	 u = (x + sign((long)tol1, (long)d));
      fu = onedGet_io(u,dummy);
      if (!fid_shim)
      {
         if (abs(fu -fx) <barrier)
         {
	    fu = (fu + onedGet_io(u,dummy)) / 2;
         }
      }
      if (!host_abort && !failAsserted)
      {
         if (fu - fx - barrier / 2 <= 0)
         {
	    if (u >= x)
	    {
	       a = x;
	       *fa = fx;
	    }
	    else
	    {
	       b = x;
	       *fb = fx;
	    }
	    v = w;
	    fv = fw;
	    w = x;
	    fw = fx;
	    x = u;
	    fx = fu;
         }
         else
         {
	    if (u < x)
	    {
	       a = u;
	       *fa = fu;
  	    }
	    else
	    {
	       b = u;
	       *fb = fu;
  	    }
	    if ((abs(fx - fw) <= 0) || (fu - fw <= 0) || (abs(w - x) < 0))
	    {
	       v = w;
	       fv = fw;
	       w = u;
	       fw = fu;
	    }
	    else if ((fu - fv <= 0) || (abs(v - x) < 0) || (abs(v - w) < 0))
	    {
	       v = u;
	       fv = fu;
	    }
         }
      }
   }				/* for iter */
   *xmin = x;
   *brent = fx;
   if (iter >= ITMAX) return (0); else return (1);
}

int oned() 
/*--------------*/
{
   int             ax,
                   bx,
                   cx,
                   fa,
                   fb,
                   fc,
		   i,j,k,
		   offset,
		   cntr = 0,
		   res,
                   brent;

   fa = onedGet_io(table[0].vector[firstactive],dummy);
   table[0].yvalue = fa;
   barrier = setlimit(check_io(fa));	/* was setlimit(---) */
   use_criteria(&x_step,&y_spread);
   DPRINT1(0, "\n barrier = %d \n",barrier);
   xmin = table[0].vector[firstactive];

   do
   {
      if ((!fid_shim) && !host_abort && !failAsserted) 
      {
        if (!cntr) startval = table[bestindex].yvalue;
        worstindex = 0;
        lock_check(2);		
      }
      cntr += 1;
      if (cntr > 1) DPRINT1(0, "\n LOOPCOUNTER = %d \n",cntr);
      res = mbrak(&ax, &bx, &cx, &fa, &fb, &fc);
      if (!host_abort && !failAsserted)
      {
         DPRINT6(0,"     %7d   %5d   %5d   %7d   %5d   %5d \n\n",
   	    ax, bx, cx, fa, fb, fc);
   
         if (!fid_shim && fb < -SIG2BIG) /* reduce lockgain */
         {
            worstval = 0;
            table[0].yvalue = fb;
            fb = onedGet_io(bx,dummy);
            lock_check(2);
            fb = table[0].yvalue;
            fa = onedGet_io(ax,dummy);
            fc = onedGet_io(cx,dummy);
         }
         res =  pbrent(ax, bx, cx, &fa, &fb, &fc, &xmin, &brent);
         if (!res) DPRINT(0,"count > ITMAX\n");
      
         table[0].vector[firstactive] = xmin;
         if (fid_shim)
	    offset = daclimit/200;
         else 
 	    offset = daclimit / 67;
         offset /= adj_step(firstactive,
              ((criteria == 'l') || (criteria == 'b')) );
         for(i=0; i < cntr; i++) offset = (3 * offset) / 2;
   
         i = onedGet_io(xmin - offset,dummy);
         if (!fid_shim) 
            i = (i + 3 * onedGet_io(xmin - offset,dummy)) >> 2;
         k = onedGet_io(xmin + offset,dummy);
         if (!fid_shim) 
            k = (k + 3 * onedGet_io(xmin + offset,dummy)) >> 2;
         j = (brent + onedGet_io(xmin,dummy)) >> 1;
         DPRINT6(0,"\n\n  zmin=%d  f(z-%d)=%d f(z)=%d f(z+%d)=%d\n\n",
	    xmin, offset, i ,j, offset, k);
      }
      else
         i = onedGet_io(xmin,dummy);

      if (host_abort || failAsserted)
      {
	 DPRINT(0, "abort from host");
	 controlMask |= ABORT;
	 break;
      }
   } while (!chktime() && (i < j - barrier/2 || k < j - barrier/2));
   return (-j);
}
