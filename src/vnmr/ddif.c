/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************************
* ddif      synthesises a 2D DOSY spectrum given a file containing          *
*           diffusion data.                                                 *
*****************************************************************************/

/* Remove all Numerical Recipes code */
/* 11 ii 09 Try to incorporate DI changes to 2.2C ddif.c in this version	*/

/* Move NR procedures to end 21iv99, but forward declare erff  */
/* Correct phasfile to phasefile  1ix99  GAM/PBC	*/
/* GAM 19xi03 define left and right peak edges as adjacent peaks if there is no minimum */
/* GAM 19xi03 cater for peak lists running in either direction */
/* GAM 31xii03 correct statement order, test for downflag, and nextpeak value on last point */
/* GAM 15i04 add more precision to frequency output */
/* MN  26v04 changed the struct peak to accomodate for biexponential fitting */
/* MN  26v04 implemented display of biexponential fitting- it should be automatic as dosyfit writes zeros to dffusion_display.inp for the biexp factors if monoexp fitting is done */
/* MN  26v04 No peaks are rejected on their stats for biexponential fitting */
/* MN 13ix04 set limits according to both diff_coefs */
/* MN 17v05 commented out the search for maximum - did sometimes mess up the display */
/* MN 17v05 Added a sanity check for too small standard errors - and if so setting it to something that will give us a display */
/* MN 20ix05 Added the possibility for several peaks in the diffusion dimension  */

/** Changes for the new VnmrJ version
  * 
  * MN 14May07 Now reads and writes some files to curexp/Dosy 
  * MN/GAM 19ii08 change search for peak range to avoid repeating search for the same F2 peak when multiple D's for same peak 
  * MN/GAM 20ii08 reorder peaks if necessary;  correct peak definition to share overlap; avoid problems caused by duplicate peaks
**/

/* 9iv08 use dosy not Dosy directory in curexp; change output format	*/
/* 7v08 cut BUFWORDS in line with VA code 				*/
/* 10vi08 Add free(intbuf) - thanks to Dan Iverson for spotting this 	*/
/* 17x08 GM increase ITMAX to 1000 to allow for extremely low D values 	*/
/* 17x08 GM remove minimum upper diffusion bound of 1.5 x 10**-10 ms/s	*/

/* 11ii09 GM replace all references to peak array to pointers		*/
/* 11ii09 GM force close and reopen of debug file to catch crashes	*/
/* 11ii09 GM change calculation of out_spreblock0			*/
/* 2Mar09 MN raise stdev to 0.000001 if zero				*/
/* 4iii09 GM correct factor of root 2 in error function argument	*/
/* 29iii09 GM change peak region so as not to overlap with adjacent peaks */
/* 31iii09 GM add peak number to output display				*/
/* 1iv09 GM Correct search for max/min D				*/
/* 1iv09 GM Correct suppression of rejected peaks			*/
/* 1iv09 GM Do not display errors for peaks rejected by dosyfit		*/
/* 1iv09 GM Abort if no statistically significant diffusion peaks found,*/
/*          to avert dconi creash					*/
/* 2iv09 GM Adjust peak numbering in output to allow for duplicates	*/
/* 24iv09 GM/MN move test for no statistically significant peaks to	*/
/*        end of loop							*/
/* 5v09 GM change calculation of signal intensity to give correct	*/
/*         signal intensity extrapolated back to zero diffusion 	*/
/*         weighting and to give correct relative intensities to 	*/
/*	   multiple diffusion peaks found by SPLMOD			*/
/* 7v09	GM replace peak height in diffusion_spectrum with peak		*/
/*	   integral in diffusion_integral_spectrum			*/
/* 7v09 GM correct calculation of exact point number			*/
/* 19v09 GM don't test for statistically signficant peaks if CONTIN	*/
/* 19v09 GM change main loop to read in CONTIN data first		*/
/* 19v09 GM recorrect calculation of exact point number                 */
/* 3vii09 GM don't reject negative/small apparent diff. coefficients	*/
/* 4vii09 GM set rfl1 and rfp1 in terms of sd and wd			*/
/* 4vii09 GM find min and max D at outset, and set sd/wd 		*/
/* 4vii09 GM Change logic for peak rejection: sd > 5% of diffusion
 	     range if more than one peak, don't reject if only one	*/
/* 4vii09 GM only reject peaks at outset, not during calculation	*/
/* 15x09 GM find max D for which std dev is <= 5%			*/
/* 15x09 GM correct heading of error column in textual output		*/
/* 13xii09 GM correct calculation of half/diffpt			*/
/* 13xii09 GM adjust effective wd to allow for F1 scale starting 	*/
/*            one point in (see VNMR.News 2002-06-15),			*/
/*            choosing default wd to have point with zero D displayed	*/
/* 13xii09 GM adjust std dev of D to at least 0.01% of wd		*/


#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>

#include "group.h"
#include "tools.h"
#include "data.h"
#include "variables.h"
#include "vnmrsys.h"
/* DI */
#include "pvars.h"
#include "wjunk.h"
#include "allocate.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#define ERROR	1
#define TRUE    1
#define FALSE   0
#define COMPLETE	0
/* DI #define	MAXNUMPEAKS     1024*/
#define	MAXPOINTS	512
/*#define BUFWORDS      131072  *//* words/bufferscale in buffer  */
#define BUFWORDS	65536	/* words/bufferscale in buffer  */
							       
/*#define DEBUG_DDIF 1 Comment out not to compile the debugging code */
/*Declarations*/
/*{{{*/

extern int bufferscale;		/* scaling factor for internal Vnmr buffers */
extern int interuption;
extern int start_from_ft;
extern int start_from_ft2d;
extern void rotate2 (float *spdata, int nelems, double lpval, double rpval);
extern void set_vnmrj_ft_params(int procdim, int argc, char *argv[]);


static int partial_display_flg, fn0, out_fn1, out_nblocks, out_sperblock0,
  out_sperblock1, continflag, bypointsflag, revflag;
static int int_mode_flg;
static dfilehead phasehead, fidhead;

static double sw, rfl, rfp, fn, sfrq, sp, wp, pi, rp, lp, vs;
static int r, numpeaks;
static float *intbuf;
static double mindiffcoef, maxdiffcoef, maxsd, wd, sd, low_diff_limit,
  high_diff_limit;
static double option_min, option_max, pkscale;
static char data_file[MAXPATH];
/* MN 26v04 Changed the struct peak to contain the peak amplitudes and stats for the second peak in a biexponential fit */

/* DI */
typedef struct
{
  double pos, amp1, diff_coef, std_dev, amp2, diff_coef2, std_dev2;
  int left, pt, right;
} region;

region *peak;
region tmppk;			/* DI by analogy */

static int i_ddif (int, char *[]);
FILE *diffusion_sp_file;
FILE *diffusion_int_file;
#ifdef DEBUG_DDIF
FILE *debug;
#endif
/*}}}*/


/*************************************

	 ddif()

**************************************/
int
ddif (int argc, char *argv[], int retc, char *retv[])
{
  /*Declarations */
/*{{{*/
  int ini, ipnt, nextpeak = 0, currpeak, prevpeak;
  int i, l, ij;
  float *inp;
  double prevnompt, arg1, arg2, arg3, arg4, erff1, erff2, erff3, erff4,
    currdiff, halfptdiff, root2, peakamp, peakmax;
//  double diffpt;
  dpointers inblock;
  int m, mm, n;
  double factor3, dbl_fn1,	/* DI dbl_ni, */
    infr, inamp, insd, ingf, outint,	/*temp variables for modifying in diffusion_spectrum */
    continval[MAXPOINTS][MAXPOINTS];	/*for the contin fitted amplitudes */
  char rubbish[1024], fac3_str[256], str_rubbish[256], *pptemp;
  FILE *ddiffile_contin = NULL;
/*}}}*/

#ifdef DEBUG_DDIF
  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "w");	/* file for debugging information */
  fprintf (debug, "Start of ddif\n");
#endif

  /* initialization bits */
  Wclear_text ();
  Wsettextdisplay ("clear");

  root2 = sqrt (2.0);
  continflag = 0;		//initialise 
  if (!P_getstring (CURRENT, "continflag", rubbish, 1, 2))
    {
      if (!strcmp ("y", rubbish))
	{
	  continflag = 1;
	}
      else
	{
	  continflag = 0;
	}
    }
  if (continflag)
    {
      //strcpy(rubbish,userdir);
      strcpy (rubbish, curexpdir);
      strcat (rubbish, "/dosy/diffusion_display.contin");
      ddiffile_contin = fopen (rubbish, "r");
      if (!ddiffile_contin)
	{
	  Werrprintf ("ddif: could not open file %s", rubbish);
	  return (ERROR);
	}
    }
#ifdef DEBUG_DDIF
  fprintf (debug, "\nbufferscale = %d\n", bufferscale);
  fprintf (debug, "\nBefore i_ddif\n");
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
/* DI */
  if (i_ddif (argc, argv))
    {
      releaseAllWithId ("ddif");
      if (ddiffile_contin)
         fclose(ddiffile_contin);
      ABORT;
    }

#ifdef DEBUG_DDIF
  fprintf (debug, "\nAfter i_ddif\n");
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  pi = M_PI;
/* DI    disp_status("IN3    "); */
  /* start of main loop */
  if ((r = D_getbuf (D_DATAFILE, fidhead.nblocks, 0, &inblock)))
    {
      D_error (r);
/* DI */
      releaseAllWithId ("ddif");
      if (ddiffile_contin)
         fclose(ddiffile_contin);
      ABORT;
    }
  if ((inblock.head->status & (S_DATA | S_SPEC | S_FLOAT | S_COMPLEX)) ==
      (S_DATA | S_SPEC | S_FLOAT | S_COMPLEX))
    {
      inp = (float *) inblock.data;
      for (i = 0; i < fn0; i++)
	{
	  intbuf[i] = inp[i];
	}
      rotate2 (intbuf, fn0 / 2, lp, rp);
      /* get rid of present data and get a new file */
	/***  make up new file header   ***/
      D_trash (D_DATAFILE);
      /* fidhead.np = fn0; */
      fidhead.np = out_fn1;
      fidhead.ntraces = out_sperblock1;
      fidhead.nblocks = out_nblocks;
      fidhead.ebytes = 4;
      fidhead.nbheaders = 1;
      fidhead.tbytes = fidhead.ebytes * fidhead.np;
      fidhead.bbytes =
	fidhead.tbytes * fidhead.ntraces +
	fidhead.nbheaders * sizeof (struct datablockhead);
      fidhead.status =
	(S_DATA | S_SPEC | S_FLOAT | S_COMPLEX | S_NP | S_NI | S_SECND |
	 S_TRANSF);
      if ( (r = D_newhead (D_DATAFILE, data_file, &fidhead)) )
	{
	  D_error (r);
/* DI */
	  releaseAllWithId ("ddif");
          if (ddiffile_contin)
             fclose(ddiffile_contin);
	  ABORT;
	}
	/*--------------------------------------------------------------*/
      /* Set params for 2D data and display.                          */
	/*--------------------------------------------------------------*/
      /* if (dbl_ni > (double)(out_fn1/2)) dbl_ni = (double)(out_fn1/2); */
      dbl_fn1 = (double) (out_fn1);
      /* DI        dbl_ni = (double)out_fn1/2; */
      P_setreal (PROCESSED, "fn", (double) (fn0), 0);
      P_setreal (CURRENT, "fn", (double) (fn0), 0);
      P_setreal (PROCESSED, "fn1", dbl_fn1, 0);
      P_setreal (CURRENT, "fn1", dbl_fn1, 0);
/* DI   	P_setreal(PROCESSED, "ni", dbl_ni, 0); */
/* DI   	P_setreal(CURRENT, "ni", dbl_ni, 0); */
      if ( (r = P_setstring (PROCESSED, "axis", "pD", 0)) )
	P_err (r, "axis", ":");
      P_setstring (CURRENT, "axis", "pD", 0);
      if ( (r = P_setreal (PROCESSED, "sp1", sd, 0)) )
	P_err (r, "sp1", ":");
      P_setreal (CURRENT, "sp1", sd, 0);
      if ( (r = P_setreal (PROCESSED, "wp1", wd, 0)) )
	P_err (r, "wp1", ":");
      P_setreal (CURRENT, "wp1", wd, 0);
      if ( (r = P_setreal (PROCESSED, "sw1", wd, 0)) )
	P_err (r, "sw1", ":");
      P_setreal (CURRENT, "sw1", wd, 0);
      if ( (r = P_setreal (PROCESSED, "rfl1", wd, 0)) )
	P_err (r, "rfl1", ":");
      P_setreal (CURRENT, "rfl1", wd, 0);
      if ( (r = P_setreal (PROCESSED, "rfp1", (sd + wd), 0)) )
	P_err (r, "rfp1", ":");
      P_setreal (CURRENT, "rfp1", (sd + wd), 0);
      /* This sets procdim, and a few other parameters */
      set_vnmrj_ft_params(2,0,NULL);

#ifdef DEBUG_DDIF
      fprintf (debug, "\nAfter parameters set for 2D display\n");
#endif
	/*--------------------------------------------------------------*/
      /* Determine precise peak position, before calculating          */
      /* 2nd dimension !                                              */
	/*--------------------------------------------------------------*/

/* MCHR MN/GAM 19-20ii08 */
/* substantial rewrite of next section, down to 'Calculate 2D data' */

/* omit calculation of peak range if already done for this F2 peak */
      prevnompt = -2;		/* previous nominal peak position */

/* sort peak list to ensure (peak+i)->pt increases as i increases */

      revflag = 0;
      if (peak->pt > (peak + numpeaks - 1)->pt)
	{
	  revflag = 1;
	  for (i = 0; i < numpeaks / 2; i++)
	    {
	      tmppk.pos = (peak + i)->pos;
	      (peak + i)->pos = (peak + numpeaks - i - 1)->pos;
	      (peak + numpeaks - i - 1)->pos = tmppk.pos;
	      tmppk.amp1 = (peak + i)->amp1;
	      (peak + i)->amp1 = (peak + numpeaks - i - 1)->amp1;
	      (peak + numpeaks - i - 1)->amp1 = tmppk.amp1;
	      tmppk.diff_coef = (peak + i)->diff_coef;
	      (peak + i)->diff_coef = (peak + numpeaks - i - 1)->diff_coef;
	      (peak + numpeaks - i - 1)->diff_coef = tmppk.diff_coef;
	      tmppk.std_dev = (peak + i)->std_dev;
	      (peak + i)->std_dev = (peak + numpeaks - i - 1)->std_dev;
	      (peak + numpeaks - i - 1)->std_dev = tmppk.std_dev;
	      tmppk.amp2 = (peak + i)->amp2;
	      (peak + i)->amp2 = (peak + numpeaks - i - 1)->amp2;
	      (peak + numpeaks - i - 1)->amp2 = tmppk.amp2;
	      tmppk.diff_coef2 = (peak + i)->diff_coef2;
	      (peak + i)->diff_coef2 = (peak + numpeaks - i - 1)->diff_coef2;
	      tmppk.std_dev2 = (peak + i)->std_dev2;
	      (peak + i)->std_dev2 = (peak + numpeaks - i - 1)->std_dev2;
	      (peak + numpeaks - i - 1)->std_dev2 = tmppk.std_dev2;
	      tmppk.left = (peak + i)->left;
	      (peak + i)->left = (peak + numpeaks - i - 1)->left;
	      (peak + numpeaks - i - 1)->left = tmppk.left;
	      tmppk.pt = (peak + i)->pt;
	      (peak + i)->pt = (peak + numpeaks - i - 1)->pt;
	      (peak + numpeaks - i - 1)->pt = tmppk.pt;
	      tmppk.right = (peak + i)->right;
	      (peak + i)->right = (peak + numpeaks - i - 1)->right;
	      (peak + numpeaks - i - 1)->right = tmppk.right;
	    }
	}
#ifdef DEBUG_DDIF
      fprintf (debug, "\nAfter fixing peak order\n");
#endif

      prevpeak = 0;
      for (m = 0; m < numpeaks; m++)
	{
	  /* Set nominal peak maximum position from input data */
	  i = (peak + m)->pt;
	  if ((peak + m)->pt == prevnompt)
	    {
	      (peak + m)->pt = (peak + m - 1)->pt;
	      (peak + m)->left = (peak + m - 1)->left;
	      (peak + m)->right = (peak + m - 1)->right;
	    }
	  else
	    {
	      currpeak = (peak + m)->pt;
	      prevnompt = (peak + m)->pt;

/* MN 20ix05 Added the possibility for several peaks in the diffusion dimension  */

	      ij = 1;
	      if (m < (numpeaks - 1))
		{
		  ij = 0;
		  while ((currpeak == (peak + m + ij)->pt)
			 && ((m + ij + 1) < numpeaks))
		    {
		      nextpeak = (peak + m + ij + 1)->pt;
		      if ((m + ij + 1) == (numpeaks - 1))
			nextpeak = fn0 - 2;
		      ij++;
		    }
		}
	      else
		{
		  nextpeak = fn0 - 2;
		}


/* Now find left and right edges of peak:  either at a threshold of 0.5%
 * of the maximum value, or wherever there is a minimum, or if the peak
 * reaches the end of the spectrum, or if the peak reaches 
 * the next peak; unless dosybypoints='y' in which case there is only
 * one point.
 */
	      if (bypointsflag == 0)
		{
		  i = (peak + m)->pt + 2;
		  while ((intbuf[i] > intbuf[(peak + m)->pt] * 0.005)
			 && (intbuf[i - 2] >= intbuf[i])
			 && (i < (fn0 - 2)) && ((i < nextpeak)
						&& (i > prevpeak)))
		    {
		      i += 2;
		    }
		  (peak + m)->right = i - 2;

		  i = (peak + m)->pt - 2;
		  while ((intbuf[i] > intbuf[(peak + m)->pt] * 0.005)
			 && (intbuf[i + 2] >= intbuf[i])
			 && (i > 0) && ((i < nextpeak) && (i > prevpeak)))
		    {
		      i -= 2;
		    }
		  (peak + m)->left = i + 2;
		}
	      else
		{
		  (peak + m)->left = (peak + m)->pt;
		  (peak + m)->right = (peak + m)->pt;
		}
	      prevpeak = (peak + m)->pt;
	    }
	}
      for (mm = 0; mm < numpeaks; mm++)
	{
	  m = mm;
	  if (revflag == 1)
	    {
	      m = numpeaks - 1 - m;
	    }
/* now add integral of peak to diffusion_integral_spectrum, counting backwards through peaks if necessary */
	  fscanf (diffusion_sp_file, "%lf,   %lf,   %lf,  %lf\n", &infr,
		  &inamp, &insd, &ingf);
#ifdef DEBUG_DDIF
	  fprintf (debug,
		   "peak #%d: F %lf\t L %d\t P %d\t R %d\t A %lf\t sp. %lf\t vs %lf\n",
		   m, (peak + m)->pos, (peak + m)->left,
		   (peak + m)->pt, (peak + m)->right,
		   (peak + m)->amp1, intbuf[(peak + m)->pt], vs);
	  fprintf (debug, "Diffusion_spectrum: %lf\t%lf\t%lf\t%lf\n", infr,
		   inamp, insd, ingf);
#endif
	  i = (peak + m)->left;
	  outint = intbuf[i];
	  while (i <= (peak + m)->right)
	    {
	      outint = outint + intbuf[i];
	      i = i + 2;
	    }

#ifdef DEBUG_DDIF
	  fprintf (debug,
		   "Diffusion_integral_spectrum: %lf\t%lf\t%lf\t%lf\n",
		   infr, outint / insd, insd, ingf);
#endif
	  fprintf (diffusion_int_file, "%lf,\t%lf,\t%lf,\t%lf\n", infr,
		   outint / insd, insd, ingf);
	}
      fclose (diffusion_int_file);
#ifdef DEBUG_DDIF
      fprintf (debug, "\nAfter finding peak edges\n");
#endif
	/*--------------------------------------------------------------*/
      /* Calculate 2D data.                                           */
      /* In the 2D data the columns are equivalent to the rows in 1D  */
      /* This turns out to be an indexing problem.                    */
	/*--------------------------------------------------------------*/
      for (n = 0; n < fidhead.nblocks; n++)
	{
	  disp_index (n + 1);
#ifdef DEBUG_DDIF
	  fprintf (debug, "Processing block %d out of %d\n", n + 1,
		   fidhead.nblocks);
#endif
	  if ( (r = D_allocbuf (D_DATAFILE, n, &inblock)) )
	    {
	      D_error (r);
/* DI */ releaseAllWithId ("ddif");
              if (ddiffile_contin)
                 fclose(ddiffile_contin);
	      ABORT;
	    }
	  inblock.head->scale = (short) 1;
	  inblock.head->status =
	    (short) S_DATA | S_SPEC | S_FLOAT | S_COMPLEX | NI_CMPLX;
	  inblock.head->index = (short) n;
	  inblock.head->mode = (short) NP_PHMODE | NI_PHMODE;
	  inblock.head->ctcount = 0;
	  inblock.head->lpval = 0.0;
	  inblock.head->rpval = 0.0;
	  inblock.head->lvl = 0.0;
	  inblock.head->tlt = 0.0;
	  inp = (float *) inblock.data;
	  for (i = 0; i < fidhead.np * fidhead.ntraces; i++)
	    {
	      inp[i] = 0.0;
	    }




		/****   and now calculate the second dimension ***/
	  /* CHANGE 8ix99 */
	  /* CHANGE 13xii09 */
	  /* halfptdiff is half the change in diffusion coefficient per point */
	  halfptdiff = wd / ((double) fidhead.np);
	  // diffpt = 2.0 * wd / ((double) fidhead.np);
	  for (m = 0; m < numpeaks; m++)
	    /*For each NMR peak calculate its width and height in the diffusion dimension */
	    {

#ifdef DEBUG_DDIF
	      fprintf (debug, "\nPeak number %d out of %d\n", m + 1,
		       numpeaks);
#endif

	      currdiff = sd + wd;
	      if ((continflag) && (n == 0))
		{
		  for (l = 0; l < fidhead.np / 2; l++)	/* Read in CONTIN trace for F1 */
		    {
		      if (fscanf (ddiffile_contin, "%s", fac3_str) == EOF)
			{
			  Werrprintf
			    ("ddif: end of file diffusion_display.contin\n");
                          fclose(ddiffile_contin);
			  return (ERROR);
			}
		      if (fscanf (ddiffile_contin, "%s\n", str_rubbish) ==
			  EOF)
			{
			  Werrprintf
			    ("ddif: end of file diffusion_display.contin\n");
                          fclose(ddiffile_contin);
			  return (ERROR);
			}
		      continval[m][l] = strtod (fac3_str, &pptemp);
		    }
		}

/* For every value of F1 (diffusion coefficient), calculate the F2 trace by adding the appropriate Gaussian weighted F2 peaks */
	      for (l = 0; l < fidhead.np / 2; l++)	/* For every point of the peak in the diffusion dimension
							 ** calculate its height, this is done using the erf function
							 ** integrate between the digital points    */
		{

		  if (continflag)
		    {
		      factor3 = continval[m][fidhead.np / 2 - l];
#ifdef DEBUG_DDIF
		      fprintf (debug, "factor3: %e \n", factor3);
#endif
		    }
		  else
		    {
		      factor3 = 1.0 / (2.0 * sqrt (2.0 * pi));
/* MN 26v04 The if statement below does not change anything AND is not corrected for biexpfit so I comment it out for now*/
/*
				if (int_mode_flg==FALSE)
					{
					arg1=diffpt*(1+((int) ((peak+m)->diff_coef/diffpt)));
					arg1=(arg1-(peak+m)->diff_coef)/(peak+m)->std_dev;
					arg2=((diffpt*((int) ((peak+m)->diff_coef/diffpt)))-(peak+m)->diff_coef)/(peak+m)->std_dev;
					erff1=  erff(arg1/root2);
					erff2=  erff(arg2/root2);
					factor3 = 1.0/(2.0*sqrt(2.0*pi));  
					factor3=factor3/(erff1-erff2);
					}
*/
/* MN 26v04 Calculation of factor3 is corrected to suit biexpfit. If monofit the biexp part equals zero*/
/* GM 1iv09 ignore peaks with high sd */
		      erff1 = 0.0;
		      erff2 = 0.0;
		      erff3 = 0.0;
		      erff4 = 0.0;
/* GM 5v09 Restore the option of argument 'c', i.e. if int_mode_flg, scale up peak height in proportion to std error in D */
		      arg1 =
			((currdiff + halfptdiff) -
			 (peak + m)->diff_coef) / ((peak + m)->std_dev);
		      arg2 =
			((currdiff - halfptdiff) -
			 (peak + m)->diff_coef) / ((peak + m)->std_dev);
		      erff1 = erff (arg1 / root2);
		      erff2 = erff (arg2 / root2);
		      if (((peak + m)->amp2 != 0.0)
			  && ((peak + m)->std_dev2 < wd * 0.05))
			{
			  arg3 =
			    ((currdiff + halfptdiff) -
			     (peak + m)->diff_coef2) / ((peak + m)->std_dev2);
			  arg4 =
			    ((currdiff - halfptdiff) -
			     (peak + m)->diff_coef2) / ((peak + m)->std_dev2);
			  erff3 = erff (arg3 / root2);
			  erff4 = erff (arg4 / root2);
			}
		      if (int_mode_flg == TRUE)
			{
			  factor3 *=
			    (((peak + m)->amp1 / ((peak + m)->amp1 +
						  (peak +
						   m)->amp2)) * (erff1 -
								 erff2) +
			     ((peak + m)->amp2 / ((peak + m)->amp1 +
						  (peak +
						   m)->amp2)) * (erff3 -
								 erff4));
			}
		      else
			{
			  factor3 *=
			    (peak +
			     m)->std_dev * (((peak + m)->amp1 /
					     ((peak + m)->amp1 +
					      (peak + m)->amp2)) * (erff1 -
								    erff2) +
					    (peak +
					     m)->std_dev2 * ((peak +
							      m)->amp2 /
							     ((peak +
							       m)->amp1 +
							      (peak +
							       m)->amp2)) *
					    (erff3 - erff4));
			}
		      currdiff = currdiff - halfptdiff - halfptdiff;
		    }

/* MCHR GAM 5v09 Correct relative intensities of multiple diffusion peaks from SPLMOD */

		  peakamp = (peak + m)->amp1;
		  if (((peak + m)->amp2 != 0.0)
		      && ((peak + m)->std_dev2 < (peak + m)->diff_coef2))
		    {
		      peakamp = peakamp + (peak + m)->amp2;
		    }
		  peakmax = 0.0;
		  for (i = (peak + m)->right; i >= (peak + m)->left; i -= 2)
		    {
		      if (intbuf[i] > peakmax)
			peakmax = intbuf[i];
		    }
		  peakmax = peakmax * vs;
		  for (i = (peak + m)->right; i >= (peak + m)->left; i -= 2)
		    {
		      ini = i / 2;
		      if ((ini >= n * fidhead.ntraces) &&
			  (ini < (n + 1) * fidhead.ntraces))
			{
			  ini = ini % fidhead.ntraces;
			  ipnt = ini * fidhead.np + l * 2;
/* MN 20ix05 Added the possibility for several peaks in the diffusion dimension  */
/* MCHR GAM 20ii08 */
/* Halve edge points to avoid double counting at overlap if not doing pointwise plot*/
			  if (continflag == 1)
			    {
			      pkscale = 1.0;
			    }
			  else
			    {
			      pkscale = peakamp / peakmax;
			    }
			  if (((i == (peak + m)->right)
			       || (i == (peak + m)->left))
			      && (bypointsflag == 0))
			    {
			      inp[ipnt] =
				inp[ipnt] +
				pkscale * intbuf[i] * factor3 / 2.0;
			      inp[ipnt + 1] =
				inp[ipnt + 1] +
				pkscale * intbuf[i + 1] * factor3 / 2.0;
			    }
			  else
			    {
			      inp[ipnt] =
				inp[ipnt] + pkscale * intbuf[i] * factor3;
			      inp[ipnt + 1] =
				inp[ipnt + 1] +
				pkscale * intbuf[i + 1] * factor3;
			    }
			}
		    }
		}		/* end of for loop varying l for points in F1 */
#ifdef DEBUG_DDIF
	      fprintf (debug,
		       "\nAfter construction of trace for peak number %d\n",
		       m + 1);
#endif
	    }			/* end of for loop for peaks */
	  if ( (r = D_markupdated (D_DATAFILE, n)) )
	    {
	      D_error (r);
/* DI */ releaseAllWithId ("ddif");
              if (ddiffile_contin)
                 fclose(ddiffile_contin);
	      ABORT;
	    }
	  if ( (r = D_release (D_DATAFILE, n)) )
	    {
	      D_error (r);
/* DI */ releaseAllWithId ("ddif");
              if (ddiffile_contin)
                 fclose(ddiffile_contin);
	      ABORT;
	    }
	}			/* end of for loop over blocks */
    }				/* end of if data in right format */

/* MCHR GAM 10vi08 */
  start_from_ft = TRUE;
  start_from_ft2d = TRUE;
  releasevarlist ();
  appendvarlist ("cr");
  Wsetgraphicsdisplay ("ds");
#ifdef DEBUG_DDIF
  fprintf (debug, "\nBefore releaseAll\n");
#endif
/* DI */ releaseAllWithId ("ddif");
/* DI   disp_status("       "); */
  disp_index (0);
#ifdef DEBUG_DDIF
  fprintf (debug, "End of ddif\n");
  fclose (debug);		/* file for debugging information */
#endif
  if (continflag)
    {
      fclose (ddiffile_contin);
    }



  RETURN;
}

/*---------------------------------------
|					|
|		i_ddif()		|
|					|
+--------------------------------------*/
static int i_ddif(int argc, char *argv[])
{
  char path[MAXPATH], diffname[MAXPATH], rubbish[1024];
  short status_mask;
  int i, j, numOKpeaks;
  double previouspeakf, rnp, rfn1;
  int not_a_good_index, too_low_error, too_low_error2;
  FILE *diffusion_file;
/* DI */ char dummy[MAXSTR];
/* DI	disp_status("IN"); */
  if (P_getreal (PROCESSED, "fn", &fn, 1) ||
      P_getreal (CURRENT, "fn1", &rfn1, 1) ||
      P_getreal (CURRENT, "sw", &sw, 1) ||
      P_getreal (CURRENT, "rfl", &rfl, 1) ||
      P_getreal (CURRENT, "rfp", &rfp, 1) ||
      P_getreal (CURRENT, "rp", &rp, 1) ||
      P_getreal (CURRENT, "lp", &lp, 1) ||
      P_getreal (CURRENT, "sfrq", &sfrq, 1) ||
      P_getreal (CURRENT, "np", &rnp, 1) ||
      P_getreal (PROCESSED, "sp", &sp, 1) ||
      P_getreal (PROCESSED, "vs", &vs, 1) ||
      P_getstring (CURRENT, "dosybypoints", rubbish, 1, 2) ||
      P_getreal (PROCESSED, "wp", &wp, 1))
    {
      Werrprintf ("Error accessing parameters\n");
      return (ERROR);
    }
  if (!strcmp ("y", rubbish))
    {
      bypointsflag = 1;
    }
  else
    {
      bypointsflag = 0;
    }

  fn0 = (int) fn;
  option_min = 0.0;
  option_max = 0.0;
  partial_display_flg = FALSE;
#ifdef DEBUG_DDIF
  fprintf (debug, "\nAfter reading in parameters in i_ddif\n");
#endif
  option_min = 0.0;
  option_max = 0.0;
  partial_display_flg = FALSE;
  int_mode_flg = FALSE;
  if (argc < 2 || argc > 4 || argc == 3)
    {
      Werrprintf ("ddif: number of arguments is incorrect\n");
      return (ERROR);
    }
  else
    {
/* first check that the first argument is either 'i' or 'c' */
      if (argv[1][0] != 'c')
	{
	  if (argv[1][0] == 'i')
	    {
	      int_mode_flg = TRUE;
	    }
	  else
	    {
	      Werrprintf ("ddif: first argument must be 'c' or 'i'\n");
	      return (ERROR);
	    }
	}
/* and now check the other arguments */
      if (argc == 4)
	{
	  partial_display_flg = TRUE;
	  option_min = atof (argv[2]);
	  option_max = atof (argv[3]);
	  Wscrprintf
	    ("\n\t\tDiffusion range plotted: %.2lf to %.2lf\n\n",
	     option_min, option_max);
	}
    }


  out_fn1 = (int) rfn1;
  strcpy (diffname, curexpdir);
  //strcpy(diffname,userdir);
  strcat (diffname, "/dosy/diffusion_display.inp");
  strcpy (data_file, curexpdir);
  //strcpy(data_file,userdir);
  strcat (data_file, "/datdir/data");
  if ((diffusion_file = fopen (diffname, "r")) == NULL)
    {
      Werrprintf ("Error opening %s file\n", diffname);
      return (ERROR);
    }
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/diffusion_spectrum");
  if ((diffusion_sp_file = fopen (rubbish, "r")) == NULL)
    {
      Werrprintf ("Error opening %s file\n", rubbish);
      fclose (diffusion_file);
      return (ERROR);
    }
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/diffusion_integral_spectrum");
  if ((diffusion_int_file = fopen (rubbish, "w")) == NULL)
    {
      Werrprintf ("Error opening %s file\n", rubbish);
      fclose (diffusion_file);
      fclose (diffusion_sp_file);
      return (ERROR);
    }

/* DI start */

  i = 0;
  /* Count number of peaks */
  /* Could put the count as first integer in the file */
  while (fscanf (diffusion_file, "%[^\n]\n", dummy) != EOF)
    {
      i++;
    }
  numpeaks = i;
  peak = (region *) allocateWithId (numpeaks * sizeof (region), "ddif");
  rewind (diffusion_file);
  maxdiffcoef = 0.0;
  mindiffcoef = 20000000.0;
  i = 0;
  while (fscanf
	 (diffusion_file, "%lf %lf %lf %lf %lf %lf %lf\n",
	  &((peak + i)->pos), &((peak + i)->amp1),
	  &((peak + i)->diff_coef), &((peak + i)->std_dev),
	  &((peak + i)->amp2), &((peak + i)->diff_coef2),
	  &((peak + i)->std_dev2)) != EOF)
    {
      if ((((peak + i)->diff_coef) > maxdiffcoef)
	  && (((peak + i)->diff_coef) > 20.0 * (peak + i)->std_dev))
	maxdiffcoef = (peak + i)->diff_coef;
      if (((peak + i)->diff_coef) < mindiffcoef)
	mindiffcoef = (peak + i)->diff_coef;
      i++;
    }
  fclose (diffusion_file);
/* DI end */
  numpeaks = i;
  if (numpeaks == 1)
    {
      maxdiffcoef = (peak + 0)->diff_coef;
    }
  low_diff_limit = partial_display_flg ? option_min : 0.0;
  high_diff_limit = partial_display_flg ? option_max : 1000000000.0;
  if (!partial_display_flg)
    {

      sd = 0.0;
/* . . . and its width */
      wd = (maxdiffcoef + 0.1 * maxdiffcoef) - sd;
/* GM added 13xii09 */
/* make the secod point in F1 correspond to zero D, so that it is displayed */
      wd = wd * rfn1 / (rfn1 - 2);
      sd = -2.0 * wd / rfn1;
/* disable minimum upper limit of 1.5 x 10**-10 m2/s		*/
/*        if ((wd+sd)<(maxdiffcoef+1.5)) wd = (maxdiffcoef+1.5)-sd; */
    }
  else
    {
/* For partial display set the limits to those chosen by the operator */
      sd = low_diff_limit;
      wd = high_diff_limit - low_diff_limit;
    }
  if (numpeaks == 1)
    {
      maxsd = 1000.0;
    }
  else
    {
      maxsd = 0.05 * (sd + wd);
    }
#ifdef DEBUG_DDIF
  fprintf (debug, "\nNumber of lines counted in i_ddif is %d\n", i);
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  not_a_good_index = numpeaks + 2;	/* ensures that before start
					 * no peak is assigned a bad
					 * index! */
  too_low_error = numpeaks + 4;	/* ensures that before start
				 * no peak is assigned */
  too_low_error2 = numpeaks + 6;	/* ensures that before start
					 * no peak is assigned */
/* *** GAM 1 iv 09 use different error code */
/* *** GAM 26 ii 08 remove any 2nd peak from listing */
#ifdef DEBUG_DDIF
  fprintf (debug,
	   "\nmindiffcoef = %f, maxdiffcoef = %f  i = %d notgood= %d\n",
	   mindiffcoef, maxdiffcoef, i, not_a_good_index);
#endif
  if (continflag)
    {
      Wscrprintf
	("\n\tCONTIN calculation constrained to diffusion range %.2lf to %.2lf\n\n",
	 option_min, option_max);
    }
  else
    {
      Wscrprintf ("Peak\tFreq/ppm\tAmplitude\tD/(10e-10 m2/s)+/-std.err.\n");
      Wscrprintf
	("--------------------------------------------------------------------\n");
    }
  numOKpeaks = 0;
  previouspeakf = 0.0;
  j = 0;
  for (i = 0; i < numpeaks; i++)
    {
      if (previouspeakf != (peak + i)->pos)
	j++;
      previouspeakf = (peak + i)->pos;
      /*MN 17v05 Check for too small (by chance perhaps) stderrors and if so, set it to 0.1% of the D value */
/* GM 13xii09 Restore this check, and change so that minimum std dev is 0.01% of wd, to avoid divide by zero errors in gcf	*/
      if (continflag)
	{
	  /*DO NOTHING */
	}
      else
	{
#ifdef DEBUG_DDIF
  fprintf (debug, "std dev before check : %f\n", (peak + i)->std_dev);
#endif
	  if ((peak + i)->std_dev < (0.0001 * wd))
	    (peak + i)->std_dev = 0.0001 * wd;
#ifdef DEBUG_DDIF
  fprintf (debug, "std dev after check : %f\n", (peak + i)->std_dev);
#endif
	  if ((peak + i)->diff_coef2 != 0.0)
	    {
	      if ((peak + i)->std_dev2 < (0.0001 * wd))
		(peak + i)->std_dev2 = 0.0001 * wd;
	    }
	}

      /*
       * Check that diffusion coefficients are within specified range, and
       * eliminate those with large standard deviations > 5% of range
       */


      if (continflag)
	{
	  /*DO NOTHING */
	}
      else
	{
	  if (((peak + i)->diff_coef < low_diff_limit)
	      || ((peak + i)->diff_coef > high_diff_limit)
	      || ((peak + i)->std_dev > maxsd))
	    {
	      not_a_good_index = i;
	      (peak + i)->std_dev *= 1000.0;
	    }
	}

/* check for standard error = 0 and set it to a minimum; flag that and later print that out on screen

THIS SHOULD HAVE NO EFFECT - LOW SD'S ALREADY ADJUSTED ABOVE
if (continflag)
	{
	}
      else
	{
	  if ((peak + i)->std_dev == 0.0)
		{
		  too_low_error = i;
		  (peak + i)->std_dev = 0.000001;
		}
	  if ((peak + i)->diff_coef2 != 0.0)
	    {
	      if ((peak + i)->std_dev2 == 0.0)
		{
		  too_low_error2 = i;
		  (peak + i)->std_dev2 = 0.000001;
		}
	    }
	}


*/





      /* else leave as found */
      /* MN 13ix04 set limits according to both diff_coefs */
      if (continflag)
	{
	  /*DO NOTHING */
	}
      else
	{
/* no longer needed  GAM 4vii09 

	    if (((peak + i)->diff_coef > maxdiffcoef)
		&& (i != not_a_good_index))
	      {
		maxdiffcoef = (peak + i)->diff_coef;
	      }
	  if ((((peak + i)->diff_coef2) > maxdiffcoef)
	      && (i != not_a_good_index)
	      && ((peak + i)->amp2 > 0.0))
	    {
	      maxdiffcoef = (peak + i)->diff_coef2;
	    }
	  if (((peak + i)->diff_coef < mindiffcoef)
	      && (i != not_a_good_index))
	    {
	      mindiffcoef = (peak + i)->diff_coef;
	    }
	  if (((peak + i)->diff_coef2) < mindiffcoef
	      && (i != not_a_good_index)
	      && ((peak + i)->amp2 > 0.0))
	    {
	      mindiffcoef = (peak + i)->diff_coef2;
	    }
*/
/* MCHR GAM 20ii08 */
/* remove trailing zeroes from output */

	  if (i != not_a_good_index)
	    {
	      numOKpeaks++;
	      if (i == too_low_error)
		{
		  Wscrprintf
		    ("%4d\t%8.4lf %16.4lf %14.4lf   +/- %10.6lf  **stdev raised to 0.000001**\n",
		     j, (peak + i)->pos / sfrq, (peak + i)->amp1,
		     (peak + i)->diff_coef, (peak + i)->std_dev);
		}
	      else
		{
		  Wscrprintf
		    ("%4d\t%8.4lf %16.4lf %14.4lf   +/- %10.6lf\n", j,
		     (peak + i)->pos / sfrq, (peak + i)->amp1,
		     (peak + i)->diff_coef, (peak + i)->std_dev);
		}
	      if ((peak + i)->amp2 != 0.0)
              {
		if (i == too_low_error2)
		  {
		    Wscrprintf
		      ("%4d\t%8.4lf %16.4lf %14.4lf   +/- %10.6lf ** stdev raised to 0.000001**\n",
		       j, (peak + i)->pos / sfrq, (peak + i)->amp2,
		       (peak + i)->diff_coef2, (peak + i)->std_dev2);
		  }
		else
		  {
		    Wscrprintf
		      ("%4d\t%8.4lf %16.4lf %14.4lf   +/- %10.6lf\n", j,
		       (peak + i)->pos / sfrq, (peak + i)->amp2,
		       (peak + i)->diff_coef2, (peak + i)->std_dev2);
		  }
              }
	    }
	  else
	    {
	      if ((peak + i)->std_dev == 1000000.0)
		{
		  Wscrprintf
		    ("%4d\t%8.4lf %16.4lf %14.4lf                   **rejected**\n",
		     j, (peak + i)->pos / sfrq, (peak + i)->amp1,
		     (peak + i)->diff_coef);
		}
	      else
		{
		  Wscrprintf
		    ("%4d\t%8.4lf %16.4lf %14.4lf   +/- %10.6lf  **rejected**\n",
		     j, (peak + i)->pos / sfrq, (peak + i)->amp1,
		     (peak + i)->diff_coef, (peak + i)->std_dev / 1000.0);
		}
	      if ((peak + i)->amp2 != 0.0)
		{
		  if ((peak + i)->std_dev2 == 1000000.0)
		    {
		      Wscrprintf
			("%4d\t%8.4lf %16.4lf %14.4lf                   **rejected**\n",
			 j, (peak + i)->pos / sfrq, (peak + i)->amp2,
			 (peak + i)->diff_coef2);
		    }
		  else
		    {
		      Wscrprintf
			("%4d\t%8.4lf %16.4lf %14.4lf   +/- %10.6lf  **rejected**\n",
			 j, (peak + i)->pos / sfrq, (peak + i)->amp2,
			 (peak + i)->diff_coef2,
			 (peak + i)->std_dev2 / 1000.0);
		    }
		}
	    }
	}
/* MCHR GM 19v09 try again correct calculation of point number according to VNMR News 2002-06-15  */
      (peak + i)->pt =
	2 * (int) (0.5 + fn * (1 - ((peak + i)->pos + rfl - rfp) / sw) / 2.0);
    }
  if ((numOKpeaks == 0) && (continflag == 0))
    {
      Werrprintf ("No statistically significant diffusion peaks found");
      return (ERROR);
    }

#ifdef DEBUG_DDIF
  fprintf (debug, "\nmindiffcoef = %f, maxdiffcoef = %f\n",
	   mindiffcoef, maxdiffcoef);
  fprintf (debug, "\nNumber of peaks not rejected = %d\n", numOKpeaks);
#endif
/* For full display, evaluate the start of diffusion scale, . . . */
/* CHANGE 19x00:  force start of diffusion display to zero */
/*      Old calculations
 *      sd = mindiffcoef-0.1*mindiffcoef;
 *      if (mindiffcoef-sd < 1.5) sd = mindiffcoef-1.5;
 *      if (sd < 0.0) sd = 0.0;
 */
	/*--------------------------------------------------------------*/
  /* Set up 2D data file information.                             */
	/*--------------------------------------------------------------*/
#ifdef DEBUG_DDIF
  fprintf (debug, "\nsd = %f, wd = %f\n", sd, wd);
  fprintf (debug, "\nBefore setting up 2D data file\n");
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  out_sperblock0 = (BUFWORDS * bufferscale / fn0);
  if (out_sperblock0 > (out_fn1 / 2))
    out_sperblock0 = out_fn1 / 2;
#ifdef DEBUG_DDIF
  fprintf (debug, "\nBUFWORDS = %d, fn0 = %d, bufferscale = %d\n", BUFWORDS,
	   fn0, bufferscale);
  fprintf (debug, "\nout_sperblock0 = %d\n", out_sperblock0);
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  out_nblocks = out_fn1 / (2 * out_sperblock0);
  if (out_nblocks == 0)		/* must be at least one block */
    out_nblocks = 1;
#ifdef DEBUG_DDIF
  fprintf (debug, "\nout_nblocks = %d\n", out_nblocks);
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  out_sperblock1 = fn0 / (2 * out_nblocks);
#ifdef DEBUG_DDIF
  fprintf (debug, "\nout_sperblock1 = %d\n", out_sperblock1);
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
	/*--------------------------------------------------------------*/
  /* Get initial 1D data set.                                     */
	/*--------------------------------------------------------------*/
#ifdef DEBUG_DDIF
  fprintf (debug, "\nGetting initial 1D data file\n");
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  if ( (r = D_gethead (D_DATAFILE, &fidhead)) )
    {
      if (r == D_NOTOPEN)
	{
	  Wscrprintf ("spectrum had to be re-opened?\n");
	  strcpy (path, curexpdir);
	  strcat (path, "/datdir/data");
	  r = D_open (D_DATAFILE, path, &fidhead);	/* open the file */
	}
      if (r)
	{
	  D_error (r);
	  return (ERROR);
	}
    }
  status_mask = (S_DATA | S_SPEC | S_FLOAT | S_COMPLEX);
  if ((fidhead.status & status_mask) != status_mask)
    {
      Werrprintf ("no spectrum in file, status = %d", fidhead.status);
      return (ERROR);
    }
  /*
   * Set PHASFILE status to !S_DATA - this is required to force a
   * recalculation of the display from the new data in DATAFILE (in the
   * ds routine, see proc2d.c)
   */
  if ( (r = D_gethead (D_PHASFILE, &phasehead)) )
    {
      if (r == D_NOTOPEN)
	{
	  Wscrprintf ("phas NOTOPEN\n");
	  strcpy (path, curexpdir);
	  strcat (path, "/datdir/phasefile");
	  r = D_open (D_PHASFILE, path, &phasehead);
	}
      if (r)
	{
	  D_error (r);
	  return (ERROR);
	}
    }
  phasehead.status = 0;
  if ( (r = D_updatehead (D_PHASFILE, &phasehead)) )
    {
      D_error (r);
      Wscrprintf ("PHAS updatehead\n");
      return (ERROR);
    }
/* DI */
  if ((intbuf =
       (float *) allocateWithId (sizeof (float) * fn0, "ddif")) == NULL)
    {
      Werrprintf ("ddif: could not allocate memory\n");
      return (ERROR);
    }
#ifdef DEBUG_DDIF
  fprintf (debug, "\nAfter allocating intbuf\n");
  fclose (debug);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_ddif");
  debug = fopen (rubbish, "a");	/* file for debugging information */
#endif
  return (COMPLETE);
}
