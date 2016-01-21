/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************************
* dosyfit   fits 2D or 3D DOSY data to obtain diffusion coefficients        *
*           amplitudes and statistics                                       *
*****************************************************************************/

/*{{{*/
/*
Removed all NR code
Add fitting for nonlinear gradients 3ix02
Only close file if exists 3ix02
Try to open cfile whether or not calibflag
Only calculate gradient calibration data if > 1 peak

Initialise nonlinflag to 0 to allow backward compatibility with dosy macro
Add facility to subtract average baseplane noise
Increase precision of amplitude data in general_dosy_stats   GAM  14iii03
Allow arguments 3D (use peak volumes) and 3Damp (use peak amplitudes)  GAM  14iii03

GAM 18viii03  Fix errors in debugging output for 3D baseplane correction
GAM 18viii03  Fix formats for output of small peak volumes 
GAM 3ix03  Fix peak number offset in debugging output for 3D baseplane correction
GAM 18xi03  Fix incorrect loop count in calculating sumsq
GAM 6i04 Change NUG coefficients to expand in terms of grel squared 
GAM 6i04 Change to using general form of mrqmin, to make alternative fitting functions easier to implement
GAM 7i04 Change covar and alpha to dynamic allocation for compatibility with standard mrqmin etc
GAM 7i04 Use DSCALE to scale up diffusion coefficient for ease of fitting
GAM 7i04 covar, a->anr,y->ynr,sig -> signr and x2->x2nr now need to run from 1 to np not 0 to np-1
not elegant, but simplest way to make compatible with standard Numerical Recipes algorithms
GAM 9i04 Read in DAC_to_G with NUG fit_coefficients if NUG
GAM 15i04 Change nerror to use soft programme close with abortflag rather than hard exit and crash
MN  20v04 added the biexponential function to dosyfunc; and removed the unused function fgauss
MN  20v04 Changed NPARAM to 5 and moved the assnment of ma to before (!) it was used
MN  20v04 Updated lista to do a 5 parameter fit
MN  26v04 Biexpfit is working, always 7 peaks output of  diffusion_display.inp to facilitate for ddif.
No exclusion of peaks are done for biexponential fitting, no matter how bad the statistics.
MN 14vi04 THIS VERSION DOES A FULL FIT OF A GRID OF STARTING GUESSES AND CHOSES THE BEST FIT - FOR SOME REASON
I SOMETIMES GET SINGULAR VALUES IN GAUSSJ AND I CANNOT UNDESTAND WHY SO I HAVE TURNED OFF THE ABORT IN GAUSSJ FOR NOW
MN 06ix04 Added NuG correction via nugflag in the dosymnmacro - presently I have just typed in the values for aramis (Doneshot) 
MN 06ix04 Added the choise between mono or biexp fit - via ncomp in the dosymn macro
MN 09ix04 Changed a[].beta[] and fac[] to NPARAMS+1, I am not sure if this fixed my uninitialised variable problem, but I think it might
MN 09ix04 Added the feature of fixing one D value in a biexpoential fit, via Dfixflag and Dfix parameters (dosymn macro)
	  At the moment I am using a standard error of 0.5% for the fixed D-value
MN 09ix04 Rewrote the gridfit for choice of starting parameters for the biexponential fit. A grid centered of the initial guess for 
          monoexponential fit is fitted in 5 steps for both D values and two steps (0.75 and 0.25 of y[0]) for the amplitudes.
          For easier compatability with previous code I use the values that resulted in the fit with the lowest chisq as starting parametrs for the 
          "normal" fit procedure, strictly this does thsi fit twice. The size of the steps in the grid is set by gridfac - goind from guess/(gridfac^2)
          to guess*gridfac^2 (gridfac = 2.0 seems a reasonable value , it is what I am using now
MN 13ix04 Writing out the values for biexponential and fixed fit to general_dosy_stats - error for the fixed fit will be 0.0000000
          Printing out values for biexp and fixed fit to D_spectrum

MN 13ix04 Starting to implement the T1 - fitting 
          controling the fitting with T1flag
          introducing d2dim as number of t1 increments
MN 14ix04 Wrote the function t1dosyfunc for the t1 fitting
          Created the typedef struct t1_x to try to make the conversion of mrqmin and mrqcof easier 
          made mrqmin2d and mrqcof2d to be compatible with teh t1dosy fit
          changed NPARAMS to 8 and changed setting the mfit and lista according to the parameters ncomp, T1flag and Dfixflag
MN 15ix04 Reading in the gradient values and the d2 values into xt1 
          Reading in the y-values correctly for T1-fit
          Changing the array ynr and signr to go from 1 to np*d2dim DO I HAVE TO CHANGE ALL THE FREE_VECTOR ??????
          Writing out the correct headers for general_dosy_stats
MN 20ix04 Allocating memory for y_diff and y_fitted using NR matrices 
	  Printing out the values of all fitted parametrs in general_dosy_stats - with standard errors
          Using t1 func to calculate values for general_dosy_stats - when appropriate
	  showdosyfitmn(a,b) will show stats for t1dosy 

MN 23ix04 IMPLEMENTING A CONTROL SYSTEM FOR MEMORY ALLOCATION AND FILES 


27ix03 MN Moved some nr function to after dosyfit() - for enchaced readability
27ix03 MN Changed the nug correction to only depend on nonlinfal - scrapped nugflag
30ix04 MN implemented NUG fitting in t1dosyfunc 
OBS!!!! I HAVE DISABLED THE READING IN OF DAC_to_G FROM THE NUG FILE AS IT IS READ IN AS A PARAMETER
NOW THE nonlinfile ONLY CONSISTS OF FOUR LINES WITH THE COEFFICIENTS. MAYBE IT CANNOT BE LIKE THIS BECAUSE FO BACKWARD COMPATIBILITY
30ix04 MN mfit was not set to 2 as a default value in the switch(ncomp) statement, giving to high errors for monoexponential fits 
30ix04 MN statistcics for the calibration (calibflag) is now done for all the peaks in a t1dosy 

TODO
starting values for T1 fit
Reading in values from file instead of as parameters ?
Decide when a peak consists of a biexponential or not.
Print some output in the Vnmrwindow when doing biexp (probabaly a macro thing) and neaten it up generally, keep a copy in vnmrsys/Dosy.
make the read in od DAC_to_G compatible with old code
make sure that the initialisation of values is correct for T1 dosy and normal - Y is inversed for T1
Figure out why T1dosy and normal dosy give very different results

The use of both a and anr does make thing a bit over-complicated, it would be better to scrap one of them.
For historical reasons (!!) the parameter in the monoexponential fit is y= a[0] + a[2]exp(a[3]) ( OR y=anr[1] + anr[3]exp(anr[4]) )making the biexp
fit y= a[0] + a[2]exp(a[3]) + a[1]exp(a[4]); this combined with the a/anr makes it harder than it should to keep track on things in the code





*/
/*}}}*/

/*Changes since we agreed on a common version 20iv05*/
/*	MN 25iv05 Parameters for now type of fitting is only checked for if called as doyfitB
*	          to ensure backward comp.
*	MN 25iv05 calibflag explicitly set to 0 ('n') as it should not be used more and the warning text changed
* 	MN 15May07 fixed bug dosy.0036 - debug supplied by Dan Iverson	
* 	MN 15May07 NUG coefficients are now read in as a parameter, not as a file	
*
*
*
*
*
*
*
*/

/** Changes made since converting to the new VnmrJ version
  *
  * MN 14May07 Now reading from and writing all files to curexp/Dosy
  * MN 14May07 Max peaks (2D) is now 32768 
  *
  *MN 7Feb07 Moved to coding on hamster VnmrJ on RedHat (previously
  *          Fedora)
  *MN 7Feb07 Writing all files for userdir for backward comp
  *MN 8Feb07 Support for calibflag is removed i.e. calibflag is always set to 0
  *          and the file calibrated_gradients is never used
  *
  *MN 11Feb07 Rename nonlinflag to nugflag and nugc to nugcal. nugcal contains the relevant 
  *           DAC_to_G as the first array element and elements 2-5 are the coefficients for the power series
  *
  *MN 11Feb07 Always use the parameter DAC_to_G (as DAC_to_G_conv) to convert back to DAC points and then either
  *           parameter DAC_to_G or as read in from nugcal for the rest
  *   9iv08   Change from DAC_to_G to gcal_
  *   9iv08   Change from nugcal (now global) to nugcal_ (local)
  *   9iv08   Change from Dosy directory to dosy
  *   17iv08  Sanity checks for gcal_ and nugcal_
  *   14v08   Remove read of nonlinflag from input
  *   14v08   Correct file path for reading in dosy_in file
  *   14v08   Remove option of old-style input; remove dosyfitn alias
  *   14v08   Require dosy_in file to begin with "DOSY version <number>"
  *   15v08   Return DOSY version number (DOSYVERSION) to dosyfit('version'), to handshake with calling macro(s)
  *   5Jun08  Move default setting for ll2dflg to before 3D data is checked for
  *   10Jul08  Add check that first line of input file has correct format
  *GM 17Oct08  Add tabs in diffusion_display.inp to allow large D values (e.g. in gases)
  *GM 12ii09  Allow for negative first point in decay when finding starting guesses in vanilla fit
  *GM 12ii09  Test for realistic monoexponential peaks, separately for mono and biexp; D between 0 and 1e-5
  *GM 4iii09  Scale Gaussian peak widths correctly for sdp macro
  *GM 31iii09 No longer reject peaks in dosyfit, leave it to ddif
  *GM 31iii09 Limit maximum standard error in D to 10**-7 m**2/s
  *GM 1iv09 Set standard error in D to 10**-7 m**2/s to if not a number
  *GM 1iv09 Remove duplication of output of first component of biexp fit
  *GM 2iv09 Add sanity check on starting guess for D in monoexp fit (to avoid crash if lots of zeroes in data)
  *         NB need to fix starting guesses for other modes (ncomp=2, t1flag &c) in same way
  *GM 3iv09 Ensure gcal_ is replaced by nugcal_[1] if nugflag='y'
  *GM      28x09 use curexpdir/dosy filename in any error messages
  *GM 14x10 Change estimation of D to avoid crash when last two points of slow deacy are exactly equal
**/



/* NB baseplane correction not implemented correctly for amplitude 3D fitting
*/

/*Includes and defines*/
/*{{{*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vnmrsys.h"
#include "data.h"
#include "disp.h"
#include "pvars.h"
#include "init2d.h"
#include "graphics.h"
#include "group.h"
#include "tools.h"
#include "wjunk.h"
#include "vnmr_lm.h"

static double sqrarg;

#define COMPLETE 0
#define ERROR	1
#define TRUE	1
#define FALSE	0
#define MAXLENGTH	512
#define MAXPEAKS_2D	32768
#define MAXPEAKS_3D	8192
#define MAXPOINTS	600
#define MAXT1POINTS	128	/*Max points allowed in the T1 dimension for T1-DOSY */
#define NPARAMS		8
#define MAXITERS	4
#define MAXFITCOEFFS    4
#define DSCALE         2e-9
#define SQR(a)		(sqrarg=(a),sqrarg*sqrarg)
#define SWAP(a,b)       {float temp=(a);(a)=(b);(b)=temp;}
#define LABEL_LEN	20
#define COMMENT_LEN	80
#define MAX_ERROR	30.0	/* Maximum error permitted on the diffusion value, in per cent */
#define FIT_ERROR_TYPES	8	/* Number of ways a fit can be deemed to fail */
#define NR_END 1
#define FREE_ARG char*

/* #define DEBUG_DOSYFIT 1 */   /*Comment out not to compile the debugging code */
#define DOSYVERSION 2		/*Use to handshake with macros to confirm compatibility */

static void free_matrix (double **m, long nrl, long nrh, long ncl, long nch);
static double **matrix (long nrl, long nrh, long ncl, long nch);
void fclosetest (FILE * file);

/*}}}*/



/*Declarations*/
/*{{{*/
int abortflag;
static double gcal_, dosyconstant, ochisq;
static int lr, mfit, n, ma, np, nugflag,
  dosyversion, kgrid, itstgrid, ncomp, Dfixflag, dgrid,
  ita, itb, agrid, itc, npeak_it, ncomp_tmp, t1grid, it_t1a, it_t1b;
static double nug[MAXFITCOEFFS+1];
static double chigrid, tmp,
  Dfix, dtemp1, dtemp2, atemp1, atemp2, dstart, t1temp, t1temp2, t1start,
  alpha_fixed;
/* parameters for the T1-fitting */
static int d2dim, T1flag;

/*****************************************************************************
*  structure for a peak record
*****************************************************************************/
typedef struct pk_struct
{
  double f1, f2;
  double amp;
  double f1_min, f1_max, f2_min, f2_max;
  double fwhh1, fwhh2;
  double vol;
  int key;
  struct pk_struct *next;
  char label[LABEL_LEN + 1];
  char comment[COMMENT_LEN + 1];
} peak_struct;


/*****************************************************************************
*  structure for peak table
*       First entry in header is the number of peaks in the table, next
*       entries tell whether the corresponding key is in use (PEAK_FILE_FULL)
*       or not (PEAK_FILE_EMPTY). (i.e. header[20] tells whether a peak with
*       key 20 is currently in existence.)
*****************************************************************************/
typedef struct
{
  int num_peaks;
  FILE *file;
  float version;
  peak_struct *head;
  short header[MAXPEAKS_3D + 1];
  char f1_label, f2_label;
  int experiment;
  int planeno;
} peak_table_struct;

peak_table_struct **peak_table;
/*}}}*/

/*---------------------------------------
|					|
|	        dosyfit()		|
|					|
|--------------------------------------*/

int
dosyfit (int argc, char *argv[], int retc, char *retv[])
{
  /*Declarations */
/*{{{*/
  register int i, j, k, l, m;
  int itst, errorflag, badinc, badflag, ct, calibflag,
    bd[MAXPOINTS], ll2dflg, amp3Dflg;
  double y[MAXPOINTS], x[MAXPOINTS], frq[MAXPEAKS_2D],
    x2[MAXPOINTS], a[NPARAMS + 1], chisq, sumsq, gwidth;
  char rubbish[MAXLENGTH], expname[MAXLENGTH], fname[MAXLENGTH];
  char str = '\0', str1[LABEL_LEN + 1], jstr[MAXLENGTH];

  void dosyfunc (double x, double a[], double *y, double dyda[], int na);
  void t1func (double x, double a[], double *y, double dyda[], int na);
  void t1dosyfunc (t1_x x, double a[], double *y, double dyda[], int na);

  double **covar, **alpha;
  double fac, cy;
  double xc[MAXPOINTS], yc[MAXPOINTS], xdiff[MAXPOINTS],
    peak_height_sum[MAXPOINTS];
  double y_diff_wa[MAXPOINTS], y_sum[MAXPOINTS], y_dd[MAXPOINTS],
    y_std[MAXPOINTS];
  double avnoise, facb[NPARAMS + 1], factmp;
  FILE *table, *fit_results, *D_spectrum, *cfile, *errorfile, *in, *old_ptr;
  int rej_pk[MAXPEAKS_2D], fit_failed_type[FIT_ERROR_TYPES + 1], nugsize;
  int zerotest, io, jo, ko, lo, xo, xt, xthigh, xtlow, maxpeaks,
    systematic_dev_flg, fit_failed_flag;
  float xtf, dlow, dhigh;
  double **y_diff, **y_fitted;

  double dummydyda[NPARAMS + 1],	/*needed for a call but never used */
    signr[MAXPOINTS + MAXT1POINTS + 1],	/*holds the individual weights for the updated NR */
    ynr[MAXPOINTS + MAXT1POINTS + 1],	/*holds the y-values for the updated NR */
    x2nr[MAXPOINTS + 1],	/*holds the x-values for the updated NR */
    anr[NPARAMS + 1],		/*holds the fitted-values for the updated NR */
    guessnr[NPARAMS + 1],	/*used for the grid in biexp fitting */
    gridcovar[NPARAMS + 1][NPARAMS + 1],	/*holds the covar values in the gridsearch */
    ytemp[MAXPOINTS + 1],	/*temporary hold the y values in T1-DOSY */
    sig[MAXPOINTS + MAXT1POINTS + 1];	/*holds the individual weights for the old NR */

  int lista[NPARAMS + 1];	/*decides which parameter to fit */

#ifdef DEBUG_DOSYFIT
  FILE *debug;
#endif

  /* MN 14ix04 variables for T1 fit */
  t1_x xt1[MAXPOINTS];
  double d2array[MAXPOINTS];

  peak_struct **peak = NULL;
  extern peak_struct *create_peak ( /*f1, f2, amp */ );
  extern int read_peak_file ();
  extern void write_peak_file_record ( /*peak_table,peak,record */ );
  extern void delete_peak_table ( /*peak_table */ );

/*}}}*/

/* GM 4iii09 define sclaing factor for width of Gaussian line in diffusion_spectrum */
  gwidth = 2.0 * sqrt (2.0 * (log (2.0)));
  if (!strcmp (argv[0], "dosyfitv"))
    {
      if (retc)
        retv[0] = realString ((double) DOSYVERSION);
      else
        Wscrprintf ("dosyfit version is %d", DOSYVERSION);
      return (COMPLETE);
    }
  /*MN 5Jun08 moved statement to before the check for 3D */
  ll2dflg = FALSE;
  amp3Dflg = FALSE;
  if (argc > 1)
    {
      if (!strncmp (argv[1], "3D", 2))
	{
	  ll2dflg = TRUE;
	  maxpeaks = MAXPEAKS_3D;
	  amp3Dflg = FALSE;
	  if (!strcmp (argv[1], "3Damp"))
	    amp3Dflg = TRUE;
	}
      else
	{
	  Werrprintf ("dosyfit: unexpected argument %s\n", argv[1]);
	  return (ERROR);
	}
    }
  else
    maxpeaks = MAXPEAKS_2D;
/* Setting some flags to default values */
  /*{{{ */
  abortflag = 0;
/*}}}*/
/* MN 15May07 fixes bug dosy.0036 - debug supplied by Dan Iverson*/

  old_ptr = 0;

  for (i = 0; i < MAXFITCOEFFS; i++)
    nug[i + 1] = 0.0;
  systematic_dev_flg = FALSE;

#ifdef UNDEF
#endif

#ifdef DEBUG_DOSYFIT
  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_dosyfit");
  debug = fopen (rubbish, "w");	/* file for debugging information */

  fprintf (debug, "Start of dosyfit\n");
#endif





/*{{{*/
/*if 3D data setup the peak_table*/
  if (ll2dflg)
    {
      avnoise = 0.0;
      if (argc > 2)
	{
	  avnoise = atof (argv[2]);
	  printf ("Average noise per Hz squared is %f\n", avnoise);
	}

      peak_table =
	(peak_table_struct **) malloc (MAXPOINTS *
				       sizeof (peak_table_struct *));
      if (!peak_table)
	{
	  Werrprintf
	    ("dosyfit: Could not allocate memory for peak_table array!");
	  return (ERROR);
	}
      for (i = 0; i < MAXPOINTS; i++)
	peak_table[i] = NULL;
      peak = (peak_struct **) malloc (MAXPOINTS * sizeof (peak_struct *));
      if (!peak)
	{
	  Werrprintf
	    ("dosyfit: Could not allocate memory for peak_struct array!");
	  free (peak_table);
	  return (ERROR);
	}
      for (i = 0; i < MAXPOINTS; i++)
	{
	  peak[i] = create_peak (0, 0, 0.0);
	}
    }
/*}}}*/

#ifdef DEBUG_DOSYFIT
  fprintf (debug, "Before reading in of data\n");
#endif
/* Open all files needed */
  /*Read data from dosy_in */
/*{{{*/
  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/dosy_in");
  in = fopen (rubbish, "r");	/* input file for 2D DOSY */
  if (!in)
    {
      Werrprintf ("dosyfit: could not open file %s", rubbish);
      if (ll2dflg)
	{
	  free (peak);
	  free (peak_table);
	}
      return (ERROR);
    }

  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  if (!ll2dflg)
    strcat (rubbish, "/dosy/diffusion_display.inp");
  else
    strcat (rubbish, "/dosy/diffusion_display_3D.inp");
  table = fopen (rubbish, "w");	/* output file for 2D DOSY */
  if (!table)
    {
      Werrprintf ("dosyfit: could not open file %s", rubbish);
      fclosetest (in);
      return (ERROR);
    }



  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/general_dosy_stats");
  fit_results = fopen (rubbish, "w");	/* detailed fitting output */

  if (!fit_results)
    {
      Werrprintf ("dosyfit: could not open file %s", rubbish);
      fclosetest (in);
      fclosetest (table);
      return (ERROR);
    }
  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/diffusion_spectrum");
  D_spectrum = fopen (rubbish, "w");	/* diffusion spectrum, input for dsp */
  if (!D_spectrum)
    {
      Werrprintf ("dosyfit: could not open file %s", rubbish);
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      return (ERROR);
    }

  //strcpy(rubbish,userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/fit_errors");
  errorfile = fopen (rubbish, "w");	/* failed fits signaled in this file */
  if (!errorfile)
    {
      Werrprintf ("dosyfit: could not open file %s", rubbish);
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      return (ERROR);
    }

  /* Manchester 6.1B:  use older version of dosy_in read, with error checking   */

  //strcpy(jstr,userdir);
  strcpy (jstr, curexpdir);
  strcat (jstr, "/dosy/dosy_in");

  if (fscanf (in, "DOSY version %d\n", &dosyversion) != 1)
    {
      Werrprintf
	("dosyfit: this software version is not compatible with the dosy macro used");
      if (ll2dflg)
	{
	  free (peak);
	  free (peak_table);
	}
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      return (ERROR);
    }

  if (fscanf (in, "%d spectra will be deleted from the analysis :\n", &badinc)
      == EOF)
    {
      Werrprintf ("dosyfit: reached end of file %s", jstr);
      if (ll2dflg)
	{
	  free (peak);
	  free (peak_table);
	}
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      return (ERROR);
    }
  for (i = 0; i < badinc; i++)
    {
      if (fscanf (in, "%d\n", &bd[i]) == EOF)
	{
	  Werrprintf ("dosyfit: reached end of file %s", jstr);
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  return (ERROR);
	}
    }

  if ((fscanf (in, "Analysis on %d peaks\n", &n) == EOF)	/* read in calibration parameters */
      || (fscanf (in, "%d points per peaks\n", &np) == EOF)
      || (fscanf (in, "%d parameters fit\n", &mfit) == EOF)
      || (fscanf (in, "dosyconstant = %lf\n", &dosyconstant) == EOF)
      || (fscanf (in, "gradient calibration flag :  %d\n", &calibflag) ==
	  EOF))
    {
      Werrprintf ("dosyfit: reached end of file %s", jstr);
      if (ll2dflg)
	{
	  free (peak);
	  free (peak_table);
	}
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      return (ERROR);
    }
  calibflag = 0;		/*calibflag should not be used anymore */
  cfile = 0;			/*so no file should be needed */
/*}}}*/
#ifdef DEBUG_DOSYFIT
  fprintf (debug, "Before reading in of parameters from Vnmr\n");
#endif

  /*Read in parameters from Vnmr if new type macro or set them as defaults for old type */
/*{{{*/

  /* Set defaults */

  d2dim = 1;
  ncomp = 1;
  nugflag = 0;
  T1flag = 0;
  Dfixflag = 0;
  Dfix = 0;

  if (P_getreal (CURRENT, "gcal_", &gcal_, 1))
    {
      Werrprintf ("Error accessing parameter gcal_\n");
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      return (ERROR);
    }
  if (gcal_ == 0.0)
    {
      Werrprintf ("Parameter gcal_ must not be zero\n");
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      return (ERROR);
    }



  /* MN13ix04 Reading in d2dim - number of increments for the T1 part of T1-DOSY */
  if (!P_getreal (CURRENT, "d2dim", &tmp, 1))
    {
      d2dim = (int) (tmp + 0.5);
      if (d2dim < 1)
	d2dim = 1;
    }

  /* MN0ix04 Reading in ncomp - number of components to be fixed (i.e ncomp=2 is a biexponential fit */
  if (!P_getreal (CURRENT, "ncomp", &tmp, 1))
    {
      ncomp = (int) (tmp + 0.5);
      if ((ncomp < 1) && (ncomp > 3))
	ncomp = 1;
    }

  /* MN  27ix04 fetching nugflag as a parameter */
  if (!P_getstring (CURRENT, "nugflag", rubbish, 1, 2))
    {
      if (!strcmp ("y", rubbish))
	{
	  nugflag = 1;
	}
      else
	{
	  nugflag = 0;
	}
    }
#ifdef DEBUG_DOSYFIT
  fprintf (debug, "nugflag:  %d\n", nugflag);
  fprintf (debug, "ncomp:  %d\n", ncomp);
#endif
  if (nugflag)
    {
      P_getsize(CURRENT, "nugcal_", &nugsize);
      if (nugsize > 5)
	nugsize = 5;
      if (nugsize < 2)
	{
	  Wscrprintf ("dosyfit: nugcal_ must contain at least 2 values");
	  Werrprintf ("dosyfit: nugcal_ must contain at least 2 values");
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  fclosetest (cfile);
	  return (ERROR);
	}
      if (P_getreal (CURRENT, "nugcal_", &gcal_, 1))
	{
	  Werrprintf ("dosyfit: cannot read gcal_ from nugcal_");
	  Wscrprintf ("dosyfit: cannot read gcal_ from nugcal_");
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  fclosetest (cfile);
	  return (ERROR);
	}
      for (i = 1; i < nugsize; i++)
	{
	  if (P_getreal (CURRENT, "nugcal_", &nug[i], i + 1))
	    {
	      Werrprintf ("dosyfit: cannot read coefficients from nugcal_");
	      Wscrprintf ("dosyfit: cannot read coefficients from nugcal_");
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }
	}
      if ((nug[1] == 0.0) || (gcal_ == 0.0))
	{
	  Wscrprintf
	    ("dosyfit: 1st and 2nd elements of nugcal_ must be non-zero");
	  Werrprintf
	    ("dosyfit: 1st and 2nd elements of nugcal_ must be non-zero");
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  fclosetest (cfile);
	  return (ERROR);
	}
#ifdef DEBUG_DOSYFIT
      fprintf (debug, "nugsize:  %d\n", nugsize);
      fprintf (debug, "gcal_ %e :\n", gcal_);
      for (i = 0; i < MAXFITCOEFFS; i++)
	fprintf (debug, "nug %e :\n", nug[i + 1]);
#endif
    }

  /*MN 14ix04 T1flag decides whether to to a T1DOSY */
  if (!P_getstring (CURRENT, "T1flag", rubbish, 1, 2))
    {
      if (!strcmp ("y", rubbish))
	{
	  T1flag = 1;
	}
      else
	{
	  T1flag = 0;
	}
    }


  /* MN 09ix04 Dfixflag decides whether to use one fixed D value in the biexponential fiting Dfix is that value */
  if (!P_getstring (CURRENT, "Dfixflag", rubbish, 1, 2))
    {
      if (!strcmp ("y", rubbish))
	{
	  Dfixflag = 1;
	}
      else
	{
	  Dfixflag = 0;
	}
    }
  if (!P_getreal (CURRENT, "Dfix", &tmp, 1))
    {
      Dfix = tmp;
    }


/*}}}*/

/* Check for the existence of some files*/
  /*{{{ */
  if (calibflag)
    {				//MN8Feb07 calibflag should always be 0 so this should never be exceuted
      strcpy (rubbish, curexpdir);
      strcat (rubbish, "/dosy/calibrated_gradients");
      cfile = fopen (rubbish, "r");
      if (!cfile)
	{
	  Werrprintf ("dosyfit: could not open file %s", rubbish);
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  return (ERROR);
	}
    }


  if (ll2dflg)
    {
      strcpy (expname, curexpdir);
      for (i = 1, l = 0; i <= (np + badinc); i++, l++)
	{
	  strcpy (fname, expname);
	  strcat (fname, "/ll2d/peaks.bin.");
	  j = 0;
	  if (i >= 10)
	    {
	      j = i / 10;
	      str = j + '0';
	      strncat (fname, &str, 1);
	    }
	  str = (i - j * 10) + '0';
	  strncat (fname, &str, 1);
	  if (read_peak_file (&peak_table[i - 1], fname))
	    {
	      Werrprintf ("dosyfit: Could not read peak file n %d !\n", i);
	      free (peak_table);
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      free (peak);
	      return (ERROR);
	    }
	  if (i == 1)
	    {
	      n = peak_table[i - 1]->num_peaks;
	    }
	  peak[i - 1] = peak_table[i - 1]->head;
	}
      i = 1;
    }
/*}}}*/

/* Check for sensible values of n and np */
/*{{{*/
  if (n > maxpeaks)
    {
      if (!ll2dflg)
	fprintf (table, "Too many peaks for the analysis !\n");
      Werrprintf ("dosyfit: Too many peaks for the analysis !\n");
      if (ll2dflg)
	{
	  free (peak_table);
	  free (peak);
	}
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      fclosetest (cfile);
      return (ERROR);
    }
  if (np > MAXPOINTS)
    {
      Werrprintf
	("dosyfit: Too many increments for analysis, max. nunmber of gzlvl1 is %d\n",
	 MAXPOINTS);
      fclosetest (in);
      fclosetest (table);
      fclosetest (fit_results);
      fclosetest (D_spectrum);
      fclosetest (errorfile);
      fclosetest (cfile);
      if (ll2dflg)
	{
	  free (peak_table);
	  free (peak);
	}
      return (ERROR);
    }				/*}}} */

  /*      allocate memory */
/*{{{*/
/* MN20ix04 Changing to accomodate for t1dosy */


  /* GAM 7i04 adjusted for NR convention 1..NPARAMS */
  /*      allocate memory for NR fitting  */
  ma = NPARAMS;


/*}}}*/


  /*Setting the appropriate mfit and lista */
/*{{{*/
  for (i = 0; i < (NPARAMS + 1); i++)
    lista[i] = 0;
  switch (ncomp)
    {
    case 2:
      if (Dfixflag)
	{
	  mfit = 3;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[2] = 1;
	  lista[3] = 1;
	  lista[4] = 1;
	}
      else
	{
	  mfit = 4;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[2] = 1;
	  lista[3] = 1;
	  lista[4] = 1;
	  lista[5] = 1;
	  if (T1flag)
	    {
	      mfit = 7;
	      lista[6] = 1;
	      lista[7] = 1;
	      lista[8] = 1;
	    }
	}
      break;


    default:
      mfit = 2;
      for (i = 0; i < NPARAMS; i++)
	lista[i + 1] = 0;
      lista[3] = 1;
      lista[4] = 1;
      if (T1flag)
	{
	  mfit = 5;
	  lista[6] = 1;
	  lista[8] = 1;
	}
      break;
    }
  /*}}} */

  /*Read in the NUG coefficients */
/*{{{*/
/*}}}*/

  /*Read in x-values from file */
/*{{{*/
  /* MN 15ix04 making it compatible with T1fit */
  for (i = 0; i < (np * d2dim); i++)
    sig[i] = 1.0;		/*temp */
  for (i = 1; i <= (np * d2dim); i++)
    signr[i] = 1.0;		/*temp */
  for (i = 0, j = 0; i < (np + badinc); i++)	/*scan through all the input values, discarding the unwanted ones */
    {
      badflag = 0;
      for (l = 0; l < badinc; l++)
	if (i == bd[l] - 1)
	  badflag = TRUE;
      /* strcpy (jstr, userdir); */
      strcpy(jstr,curexpdir);
      strcat (jstr, "/dosy/dosy_in");	/* read in gradient array */
      if (badflag)
	{
	  if (fscanf (in, "%s", rubbish) == EOF)
	    {
	      Werrprintf ("dosyfit: reached end of file %s on increment %d",
			  jstr, i);
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }
	}
      else
	{
	  if (fscanf (in, "%lf", &x[j]) == EOF)
	    {
	      Werrprintf ("dosyfit: reached end of file %s on increment %d",
			  jstr, i);
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }
	  x[j++] /= (gcal_ * 32767.0);	/* change back from gauss/cm  to DAC units & divide by 32767 */
	}
    }



  /* MN15ix04 Setting the xvalues (t1x) for the T1fit */

  for (i = 0; i < MAXPOINTS; i++)
    d2array[i] = 0.0;
  for (i = 1; i <= d2dim; i++)
    {
      if (!P_getreal (CURRENT, "d2", &tmp, i))
	{
	  d2array[i] = tmp;
	}
    }
  for (i = 1, k = 1; i <= d2dim; i++)
    {
      for (j = 1; j <= np; j++)
	{
	  xt1[k].grad = x[j - 1] * x[j - 1];
	  xt1[k++].tau = d2array[i];
	}
    }

  if (!calibflag)		/* calibrate gradient array */
    {
      for (i = 0; i < np; i++)
	{
	  x2[i] = x[i] * x[i];
	  x2nr[i + 1] = x2[i];
	}
    }
  if (calibflag)
    for (i = 0; i < np; i++)
      {
	if (fscanf (cfile, "%lf", &x2[i]) == EOF)
	  {
	    strcpy (jstr, curexpdir);
	    strcat (jstr, "/dosy/calibrated_gradients");
	    Wscrprintf ("dosyfit: reached end of file %s", jstr);
	    fclosetest (in);
	    fclosetest (table);
	    fclosetest (fit_results);
	    fclosetest (D_spectrum);
	    fclosetest (errorfile);
	    fclosetest (cfile);
	    if (ll2dflg)
	      {
		free (peak);
		free (peak_table);
	      }
	    return (ERROR);
	  }
	x2nr[i + 1] = x2[i];
      }


/*}}}*/

  /*Read in frequencies from dosy_in */
/*{{{*/
  strcpy (jstr, curexpdir);
  //strcpy(jstr,curexpdir);
  strcat (jstr, "/dosy/dosy_in");	/* read in frequencies from dll, could read from file fp.out */
  if (!ll2dflg)
    {
      if (fscanf (in, "%s", rubbish) == EOF)
	{
	  Werrprintf ("dosyfit: reached end of file %s", jstr);
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  fclosetest (cfile);
	  return (ERROR);
	}
      while (strcmp (rubbish, "(Hz)") != 0)
	if (fscanf (in, "%s", rubbish) == EOF)
	  {
	    Werrprintf ("dosyfit: reached end of file %s", jstr);
	    if (ll2dflg)
	      {
		free (peak);
		free (peak_table);
	      }
	    fclosetest (in);
	    fclosetest (table);
	    fclosetest (fit_results);
	    fclosetest (D_spectrum);
	    fclosetest (errorfile);
	    fclosetest (cfile);
	    return (ERROR);
	  }
      for (i = 0; i < n; i++)
	if ((fscanf (in, "%s", rubbish) == EOF)
	    || (fscanf (in, "%lf", &frq[i]) == EOF))
	  {
	    Werrprintf ("dosyfit: reached end of file %s", jstr);
	    if (ll2dflg)
	      {
		free (peak);
		free (peak_table);
	      }
	    fclosetest (in);
	    fclosetest (table);
	    fclosetest (fit_results);
	    fclosetest (D_spectrum);
	    fclosetest (errorfile);
	    fclosetest (cfile);
	    return (ERROR);
	  }
    }
/*}}}*/

  /* Printing the correct headers for general_dosy_stats */
/*{{{*/
  if (ll2dflg)
    {
      fprintf (fit_results,
	       "\t\t3D data set only monoexponential fit implemented\n\n");
      fprintf (fit_results,
	       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area)\n");
    }
  else
    {
      fprintf (fit_results, "\t\t2D data set (or relaxation weighted) \n\n");
      switch (ncomp)
	{
	case 2:
	  if (T1flag)
	    {
	      fprintf (fit_results,
		       "\t\tBiexponential fitting - with T1 weighting\n\n");
	      fprintf (fit_results,
		       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area)*(1-a7*exp(-tau/a5) + a3*exp(-a4*gradient_area)*(1-a7*exp(-tau/a6) \n");
	    }
	  else
	    {
	      fprintf (fit_results, "\t\tBiexponential fitting\n\n");
	      fprintf (fit_results,
		       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area) + a3*exp(-a4*gradient_area)\n");
	      if (Dfixflag)
		fprintf (fit_results,
			 "One diffusion coefficient (a4) kept constant (not fitted - value user supplied)\n");
	    }
	  if (nugflag)
	    fprintf (fit_results,
		     "Non-uniform gradient compensation used (not shown in equations)\n\n");
	  fprintf (fit_results, "\n");
	  break;

	default:
	  if (T1flag)
	    {
	      fprintf (fit_results,
		       "\t\tMonoexponential fitting - with T1 weighting\n\n");
	      fprintf (fit_results,
		       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area)*(1-a7*exp(-tau/a5) \n");
	    }
	  else
	    {
	      fprintf (fit_results, "\t\tMonoexponential fitting\n\n");
	      fprintf (fit_results,
		       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area)\n");
	    }
	  if (nugflag)
	    fprintf (fit_results,
		     "Non-uniform gradient compensation used (not shown in equations)\n");
	  fprintf (fit_results, "\n");
	  break;
	}
    }
/*}}}*/

  /*allocating these as late as possible to facilitate error handling */
  covar = matrix (1, NPARAMS, 1, NPARAMS);
  alpha = matrix (1, NPARAMS, 1, NPARAMS);
  y_diff = matrix (0, (np * d2dim), 0, n);
  y_fitted = matrix (0, (np * d2dim), 0, n);
  /*Initialising some values to zero */
/*{{{*/
  ct = 0;
  for (i = 0; i < np; i++)
    {
      peak_height_sum[i] = 0.0;
      xdiff[i] = 0.0;
      for (j = 0; j < n; j++)
	{
	  y_diff[i][j] = 0.0;
	}
    }
/*}}}*/

  disp_status ("dosyfit");

  npeak_it = 1;
  printf ("Fitting on %d peaks\n", n);

  fit_failed_flag = 0;


  /* Loop over resonances begins */

  for (lr = 0; lr < n; lr++)
    {

      if (fit_failed_flag == 1)
	{
	  ncomp = 2;
	  mfit = 4;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[2] = 1;
	  lista[3] = 1;
	  lista[4] = 1;
	  lista[5] = 1;
	  fit_failed_flag = 0;
	}
      else if (fit_failed_flag == 2)
	{
	  ncomp = 2;
	  mfit = 3;
	  Dfixflag = 1;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[2] = 1;
	  lista[3] = 1;
	  lista[4] = 1;
	  fit_failed_flag = 0;
	}
      else if (fit_failed_flag == 3)
	{
	  ncomp = 2;
	  mfit = 7;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[2] = 1;
	  lista[3] = 1;
	  lista[4] = 1;
	  lista[5] = 1;
	  lista[6] = 1;
	  lista[7] = 1;
	  lista[8] = 1;
	  fit_failed_flag = 0;
	}
      else
	{
	  /* Do nothing */
	}


      /* read in amplitudes from dosy_in */
/*{{{*/
      if (lr == 0 && !ll2dflg)
	{
	  if (fscanf (in, "%s", rubbish) == EOF)
	    {
	      Werrprintf ("dosyfit: reached end of file %s", jstr);
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }
	  while (strcmp (rubbish, "(mm)") != 0)
	    {
	      if (fscanf (in, "%s", rubbish) == EOF)
		{
		  Werrprintf ("dosyfit: reached end of file %s", jstr);
		  if (ll2dflg)
		    {
		      free (peak);
		      free (peak_table);
		    }
		  fclosetest (in);
		  fclosetest (table);
		  fclosetest (fit_results);
		  fclosetest (D_spectrum);
		  fclosetest (errorfile);
		  fclosetest (cfile);
		  return (ERROR);
		}
	    }
	}

      if (lr % 32 == 0)
	disp_index (lr);
      /*MN14ix04 reading in the yvalues differently depending on T1flag */
      if (T1flag)
	{
	  for (j = 1, m = 0; j <= d2dim; j++)
	    {
	      for (i = 0; i < (np + badinc); i++)
		{
		  badflag = 0;
		  for (l = 0; l < badinc; l++)
		    if (i == bd[l] - 1)
		      badflag = 1;
		  fscanf (in, "%s", rubbish);
		  fscanf (in, "%s", rubbish);
		  if (badflag)
		    fscanf (in, "%s", rubbish);
		  else
		    fscanf (in, "%lf", &y[m++]);
		}
	    }
/*MN 11x04 inverting the sign of y for the T1-values */
	  for (k = 0; k < (np * d2dim); k++)
	    y[k] = -1.0 * y[k];
	}
      else
	{
	  for (i = 0, j = 0; i < (np + badinc); i++)
	    {
	      badflag = 0;
	      for (l = 0; l < badinc; l++)
		if (i == bd[l] - 1)
		  badflag = 1;
	      if (!ll2dflg)
		{
		  fscanf (in, "%s", rubbish);
		  fscanf (in, "%s", rubbish);
		  if (badflag)
		    fscanf (in, "%s", rubbish);
		  else
		    fscanf (in, "%lf", &y[j++]);
		}
	      else
		{
		  if (!badflag)
		    {
		      if (amp3Dflg)
			{
			  y[j] = peak[i]->amp;
			}
		      else
			{
			  y[j] = peak[i]->vol;
			}
		      if (avnoise > 0.0)
			{
			  y[j] =
			    y[j] - avnoise * (peak[i]->f1_max -
					      peak[i]->f1_min) *
			    (peak[i]->f2_max - peak[i]->f2_min);
			  printf
			    ("peak %d   point %d\traw volume %f\tcorrected vol %f\n",
			     lr + 1, i + 1, peak[i]->vol, y[j]);
			}
		      j++;
		    }
		}
	    }
	}
      /* If all points are zero, make the first point 0.01 to give the fit something to chew on */
      zerotest = 1;
      for (i = 0, j = 0; i < np; i++)
	{
	  if (y[i] != 0.0)
	    {
	      zerotest = 0;
	    }
	}
      if (zerotest == 1)
	{
	  y[0] = 0.01;
	}


/*}}}*/

#ifdef DEBUG_DOSYFIT
      fprintf (debug, "Before choosing the fitting type\n");
#endif


      /*Find the starting guesses depending on T1-data, biexp or vanilla DOSY */
      if (T1flag)
	{
	  /*For T1-dosy fitting */
/*{{{*/

	  /* Simple starting values */
	  /*{{{ */
	  sumsq = 0;
	  for (i = 0; i < np; i++)
	    {
	      sumsq = sumsq + (y[i] * y[i]);
	    }
	  for (i = 0; i < (np * d2dim); i++)
	    {
	      ynr[i + 1] = y[i];
	    }

	  a[0] = 0.0;
	  a[1] = 0.0;
	  a[2] = -y[0];
	  i = 0;
	  /* first np points is like a normal dosy */
	  while (-y[++i] >= 0.5 * a[2] && (i < np - 1))
             ;
/* prevent estimate failing if y[i - 1] = y[i] */
        fac=x2[i - 1];
        if ((y[i - 1] > y[i])&&((0.5 * a[2] - y[i]) < (y[i - 1] - y[i])))
	  fac =
	    x2[i - 1] + (0.5 * a[2] + y[i]) / (-y[i - 1] + y[i]) * (x2[i] -
								    x2[i -
								       1]);
/* perform sanity check on estimated diffusion coefficient */
	  if ((fac < x2[2]) || (fac > x2[np - 1]))
	    {
	      fac = x2[np / 2];
	    }
	  a[3] =
	    0.693 / (DSCALE * dosyconstant * 32767.0 * 32767.0 * 0.0001 *
		     gcal_ * gcal_ * fac);

	  for (i = 0; i < np * d2dim; i = i + np)
	    {
	      if ((-y[i] < 0.5 * (-y[np * d2dim - np]))
		  || (i >= (np * d2dim)))
		break;
	    }
	  a[5] =
	    xt1[i - np].tau + (-y[np * d2dim - np] - (-y[i])) / (-y[i - np] -
								 (-y[i])) /
	    (xt1[i].tau - xt1[i + np].tau);
	  a[5] = a[5] / 0.693;
	  if ((a[5] > 100) || (a[5] < 0.01))
	    a[5] = 3.0;
/* MN 11x04 alpha = (S0 - S(0)i)/S0 */
	  a[7] = ((-y[np * d2dim - np]) - (-y[0])) / -y[np * d2dim - np];
/*}}}*/
	  /* do a monoexponential fit of the first DOSY for a better starting guess */
	  /*{{{ */
	  errorflag = 0;
	  k = 1;
	  ncomp_tmp = ncomp;
	  ncomp = 1;
	  mfit = 2;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[3] = 1;
	  lista[4] = 1;
	  a[2] = -y[0];
	  for (i = 0; i < NPARAMS; i++)
	    {
	      anr[i + 1] = a[i];
	    }
	  for (i = 1; i <= np; i++)
	    ytemp[i] = -ynr[i];	/*sign needs to be reversed for use of dosyfunc */

	  lm_init (x2nr, ytemp, signr, np, anr, lista, NPARAMS, covar, alpha,
		  &chisq, dosyfunc);
	  itstgrid = 0;
	  for (;;)
	    {
	      ochisq = chisq;
	      lm_iterate (x2nr, ytemp, signr, np, anr, lista, NPARAMS, covar,
		      alpha, &chisq, dosyfunc);
	      k++;
	      if (k > 999)
		{
		  Wscrprintf ("Warning:  corrupt input data");
		  break;
		}
	      if (chisq > ochisq)
		itstgrid = 0;
	      else
		{
		  if (fabs (ochisq - chisq) < 0.01 * sumsq)
		    itstgrid++;
		  else
		    itstgrid = 0;
		}
	      if (itstgrid < MAXITERS)
		continue;
	      lm_covar (x2nr, ytemp, signr, np, anr, lista, NPARAMS, covar,
		      alpha, &chisq, dosyfunc);
	      break;
	    }
	  ncomp = ncomp_tmp;
	  for (j = 0; j < NPARAMS; j++)
	    a[j] = anr[j + 1];
/*}}}*/
	  /* do a monoexponential fit of the first T1 for a better starting guess */
	  /*{{{ */
	  errorflag = 0;
	  k = 1;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[3] = 1;
	  lista[6] = 1;
	  lista[8] = 1;
	  for (i = 0; i < NPARAMS; i++)
	    {
	      anr[i + 1] = a[i];
	    }
	  a[2] = -y[0];
	  for (i = 1; i <= np; i++)
	    ytemp[i] = ynr[1 + (i - 1) * np];	/*T1 data */
	  for (i = 0; i < NPARAMS; i++)
	    {
	      anr[i + 1] = a[i];
	    }
	  lm_init (d2array, ytemp, signr, np, anr, lista, NPARAMS, covar,
		  alpha, &chisq, t1func);
	  itstgrid = 0;
	  for (;;)
	    {
	      ochisq = chisq;
	      lm_iterate (d2array, ytemp, signr, np, anr, lista, NPARAMS, covar,
		      alpha, &chisq, t1func);
	      k++;
	      if (k > 999)
		{
		  Wscrprintf ("Warning:  corrupt input data");
		  break;
		}
	      if (chisq > ochisq)
		itstgrid = 0;
	      else
		{
		  if (fabs (ochisq - chisq) < 0.01 * sumsq)
		    itstgrid++;
		  else
		    itstgrid = 0;
		}
	      if (itstgrid < MAXITERS)
		continue;
	      lm_covar (d2array, ytemp, signr, np, anr, lista, NPARAMS, covar,
		      alpha, &chisq, t1func);
	      break;
	    }
	  for (j = 0; j < NPARAMS; j++)
	    a[j] = anr[j + 1];
/*}}}*/
	  /* Do a gridfit  if ncomp==2 */
/*{{{*/

	  if (ncomp == 2)
	    {
	      mfit = 5;
	      for (i = 0; i < NPARAMS; i++)
		lista[i + 1] = 0;
	      for (i = 1; i < NPARAMS; i++)
		lista[i + 1] = 1;

	      dgrid = 3;	/* gives a dgrid*2+1 grid */
	      agrid = 5;
	      t1grid = 3;
	      dstart = a[3];
	      t1start = a[5];
	      alpha_fixed = a[7];
	      kgrid = 1;
	      for (ita = 1; ita <= (dgrid * 2 + 1); ita++)
		{
		  if ((dgrid - ita + 1) > 0)
		    {
		      dtemp1 = (dgrid - ita + 2) * dstart;
		    }
		  else if ((dgrid - ita + 1) < 0)
		    {
		      dtemp1 = dstart / (-1.0 * (dgrid - ita + 0));
		    }
		  else
		    {
		      dtemp1 = dstart;
		    }
		  for (itb = 1; itb <= (dgrid * 2 + 1); itb++)
		    {
		      if ((dgrid - itb + 1) > 0)
			{
			  dtemp2 = (dgrid - itb + 2) * dstart;
			}
		      else if ((dgrid - itb + 1) < 0)
			{
			  dtemp2 = dstart / (-1.0 * (dgrid - itb + 0));
			}
		      else
			{
			  dtemp2 = dstart;
			}
		      for (it_t1a = 1; it_t1a <= (t1grid * 2 + 1); it_t1a++)
			{
			  if ((t1grid - it_t1a + 1) > 0)
			    {
			      t1temp = (t1grid - it_t1a + 2) * t1start;
			    }
			  else if ((t1grid - it_t1a + 1) < 0)
			    {
			      t1temp =
				t1start / (-1.0 * (t1grid - it_t1a + 0));
			    }
			  else
			    {
			      t1temp = t1start;
			    }
			  for (it_t1b = 1; it_t1b <= (t1grid * 2 + 1);
			       it_t1b++)
			    {
			      if ((t1grid - it_t1b + 1) > 0)
				{
				  t1temp2 = (t1grid - it_t1b + 2) * t1start;
				}
			      else if ((t1grid - it_t1b + 1) < 0)
				{
				  t1temp2 =
				    t1start / (-1.0 * (t1grid - it_t1b + 0));
				}
			      else
				{
				  t1temp2 = t1start;
				}
			      for (itc = 1; itc <= (agrid + 1); itc++)
				{
				  atemp1 =
				    (agrid - itc + 1) * (-y[0]) / agrid;
				  atemp2 = (-y[0]) - atemp1;
				  anr[2] = atemp1;
				  anr[3] = atemp2;
				  anr[4] = dtemp1;
				  anr[5] = dtemp2;
				  anr[6] = t1temp;
				  anr[7] = t1temp2;
				  anr[8] = alpha_fixed;
				  lm_init2d (xt1, ynr, signr, (np * d2dim),
					    anr, lista, NPARAMS, covar, alpha,
					    &chisq, t1dosyfunc);
				  k = 1;
				  itstgrid = 0;
				  for (;;)
				    {
				      k++;
				      ochisq = chisq;
				      lm_iterate2d (xt1, ynr, signr, (np * d2dim),
						anr, lista, NPARAMS, covar,
						alpha, &chisq, t1dosyfunc);
				      if (k > 999)
					{
					  Wscrprintf
					    ("Warning:  corrupt input data");
					  break;
					}
				      if (chisq > ochisq)
					itstgrid = 0;
				      else if (fabs (ochisq - chisq) <
					       0.01 * sumsq)
					itstgrid++;
				      if (itstgrid < MAXITERS)
					continue;
				      lm_covar2d (xt1, ynr, signr, (np * d2dim),
						anr, lista, NPARAMS, covar,
						alpha, &chisq, t1dosyfunc);
				      break;
				    }
				  if (kgrid == 1)
				    {
				      for (j = 1; j <= NPARAMS; j++)
					guessnr[j] = anr[j + 1];
				      chigrid = chisq;
				      kgrid = 0;
				    }
				  if (chisq < chigrid)
				    {
				      for (j = 1; j <= NPARAMS; j++)
					guessnr[j] = anr[j + 1];
				      chigrid = chisq;
				      /*covar is needed for sanity check */
				      for (j = 1; j <= NPARAMS; j++)
					{
					  for (i = 1; i <= NPARAMS; i++)
					    {
					      gridcovar[i][j] = covar[i][j];
					    }
					}
				    }
				}
			    }
			}
		    }
		}
	      for (i = 0; i < NPARAMS; i++)
		a[i] = guessnr[i];

	      /* Do a sanity check of the fitted data */
	      a[3] = a[3] * DSCALE;
	      a[4] = a[4] * DSCALE;
	      /* adding error facs for all the 4 parameters in a biexp fitting */
	      /* covar is now 1..NPARAMS x 1..NPARAMS */
	      factmp = chigrid / (double) ((np * d2dim) - mfit);
	      factmp = sqrt (factmp);
	      for (i = 1; i <= NPARAMS; i++)
		{
		  facb[i] = factmp * sqrt (gridcovar[i][i]);
		}
	      facb[4] *= DSCALE;
	      facb[5] *= DSCALE;

	      for (i = 1; i <= FIT_ERROR_TYPES; i++)
		{
		  fit_failed_type[i] = 0;
		}

	      if ((a[3] < 0.0) || (a[4] < 0.0))
		{		/*Negative Diffusion coefficients */
		  fit_failed_type[1] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 1\n");
#endif
		}

	      if ((a[3] > 100.0) || (a[4] > 100.0))
		{		/*Diffusion coefficients toolarge (at least for liquids) */
		  fit_failed_type[2] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 2\n");
#endif
		}

	      if ((a[2] < 0.0) || (a[1] < 0.0))
		{		/*Negative amplitudes */
		  fit_failed_type[3] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 3\n");
#endif
		}

	      if (((100.0 * facb[5] / a[4]) > MAX_ERROR)
		  || ((100.0 * facb[4] / a[3]) > MAX_ERROR))
		{		/*Stdev toohigh on diff. coeff. */
		  fit_failed_type[4] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 4\n");
#endif
		}

	      if (((100.0 * facb[3] / a[2]) > MAX_ERROR)
		  || ((100.0 * facb[2] / a[1]) > MAX_ERROR))
		{		/*Stdev toohigh on amplitude */
		  fit_failed_type[5] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 5\n");
#endif
		}

	      if ((a[5] < 0.0) || (a[6] < 0.0))
		{		/*Negative T1 values */
		  fit_failed_type[6] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 6\n");
#endif
		}

	      if ((a[5] > 1000.0) || (a[6] > 1000.0))
		{		/*T1 values unreasonably large large */
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 7\n");
#endif
		}


	      if (((100.0 * facb[6] / a[5]) > MAX_ERROR)
		  || ((100.0 * facb[7] / a[6]) > MAX_ERROR))
		{		/*Stdev too high on T1 values */
		  fit_failed_type[8] = 1;
#ifdef DEBUG_DOSYFIT
		  fprintf (debug, "Type 8\n");
#endif
		}


	      for (j = 1; j <= FIT_ERROR_TYPES; j++)
		{
		  if (fit_failed_type[j] == 1)
		    {
		      Wscrprintf
			("T1 DOSY fit failed on peak %d. Writing to the \"fit_errors\" file and defaulting to a monoexponential fit \n",
			 lr + 1);
		      fprintf (errorfile, "\t\tT1 DOSY fitting\n\n");
		      fprintf (errorfile,
			       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area)*(1-a7*exp(-tau/a5) + a3*exp(-a4*gradient_area)*(1-a7*exp(-tau/a6) \n");
		      if (nugflag)
			fprintf (errorfile,
				 "Non-uniform gradient compensation used (not shown in equations)\n");
		      fprintf (errorfile,
			       "Biexponential fit failed on peak %d\n ",
			       lr + 1);
		      if (fit_failed_type[1] == 1)
			{
			  fprintf (errorfile,
				   "Negative diffusion coefficient(s) found\n");
			}
		      if (fit_failed_type[2] == 1)
			{
			  fprintf (errorfile,
				   "Diffusion coefficient(s) to large (D>100 m2/s) \n");
			}
		      if (fit_failed_type[3] == 1)
			{
			  fprintf (errorfile,
				   "Negative amplitude(s) found \n");
			}
		      if (fit_failed_type[4] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on diffusion coeffcient(s) to large \n");
			}
		      if (fit_failed_type[5] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on amplitude(s) to large \n");
			}
		      if (fit_failed_type[6] == 1)
			{
			  fprintf (errorfile, "Neative T1 values\n");
			}
		      if (fit_failed_type[7] == 1)
			{
			  fprintf (errorfile,
				   "T1 values unreasonably large \n");
			}
		      if (fit_failed_type[8] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on T1 values to large \n");
			}
		      fprintf (errorfile, "Frequency %lf\n", frq[lr]);
		      fprintf (fit_results,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g) a3 %3.5g (%3.5g) a4 %3.5g (%3.5g) ",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4], a[1], facb[2], 1.0e10 * a[4],
			       1.0e10 * facb[5]);
		      fprintf (fit_results,
			       "a5 %3.5g (%3.5g) a6 %3.5g (%3.5g) a7 %3.5g (%3.5g)\n",
			       a[5], facb[6], a[6], facb[7], a[7], facb[8]);
		      fprintf (errorfile, "\n\n");


		      /*Set the correct values for a monoexponential fit */
		      ncomp = 1;
		      sumsq = 0;
		      for (i = 0; i < np; i++)
			{
			  sumsq = sumsq + (y[i] * y[i]);
			}
		      for (i = 0; i < (np * d2dim); i++)
			{
			  ynr[i + 1] = y[i];
			}

		      a[0] = 0.0;
		      a[1] = 0.0;
		      a[2] = -y[0];
		      i = 0;
		      while (-y[++i] >= 0.5 * a[2] && (i < np - 1));
/* prevent estimate failing if y[i - 1] = y[i] */
        fac=x2[i - 1];
        if ((y[i - 1] > y[i])&&((0.5 * a[2] - y[i]) < (y[i - 1] - y[i])))
		      fac =
			x2[i - 1] + (0.5 * a[2] + y[i]) / (-y[i - 1] +
							   y[i]) * (x2[i] -
								    x2[i -
								       1]);
/* perform sanity check on estimated diffusion coefficient */
		      if ((fac < x2[2]) || (fac > x2[np - 1]))
			{
			  fac = x2[np / 2];
			}
		      a[3] =
			0.693 / (DSCALE * dosyconstant * 32767.0 * 32767.0 *
				 0.0001 * gcal_ * gcal_ * fac);

		      for (i = 0; i < np * d2dim; i = i + np)
			{
			  if ((-y[i] < 0.5 * (-y[np * d2dim - np]))
			      || (i >= (np * d2dim)))
			    break;
			}
		      a[5] =
			xt1[i - np].tau + (-y[np * d2dim - np] -
					   (-y[i])) / (-y[i - np] -
						       (-y[i])) /
			(xt1[i].tau - xt1[i + np].tau);
		      a[5] = a[5] / 0.693;
		      if ((a[5] > 100) || (a[5] < 0.01))
			a[5] = 3.0;
/* MN 11x04 alpha = (S0 - S(0)i)/S0 */
		      a[7] =
			((-y[np * d2dim - np]) - (-y[0])) / -y[np * d2dim -
							       np];
		      /* do a monoexponential fit of the first DOSY for a better starting guess */
		      errorflag = 0;
		      k = 1;
		      mfit = 2;
		      for (i = 0; i < NPARAMS; i++)
			lista[i + 1] = 0;
		      lista[3] = 1;
		      lista[4] = 1;
		      a[2] = -y[0];
		      for (i = 0; i < NPARAMS; i++)
			{
			  anr[i + 1] = a[i];
			}
		      for (i = 1; i <= np; i++)
			ytemp[i] = -ynr[i];	/*sign needs to be reversed for use of dosyfunc */

		      lm_init (x2nr, ytemp, signr, np, anr, lista, NPARAMS,
			      covar, alpha, &chisq, dosyfunc);
		      itstgrid = 0;
		      for (;;)
			{
			  ochisq = chisq;
			  lm_iterate (x2nr, ytemp, signr, np, anr, lista, NPARAMS,
				  covar, alpha, &chisq, dosyfunc);
			  k++;
			  if (k > 999)
			    {
			      Wscrprintf ("Warning:  corrupt input data");
			      break;
			    }
			  if (chisq > ochisq)
			    itstgrid = 0;
			  else
			    {
			      if (fabs (ochisq - chisq) < 0.01 * sumsq)
				itstgrid++;
			      else
				itstgrid = 0;
			    }
			  if (itstgrid < MAXITERS)
			    continue;
			  lm_covar (x2nr, ytemp, signr, np, anr, lista, NPARAMS,
				  covar, alpha, &chisq, dosyfunc);
			  break;
			}
		      for (j = 0; j < NPARAMS; j++)
			a[j] = anr[j + 1];
		      /* do a monoexponential fit of the first T1 for a better starting guess */
		      errorflag = 0;
		      k = 1;
		      for (i = 0; i < NPARAMS; i++)
			lista[i + 1] = 0;
		      lista[3] = 1;
		      lista[6] = 1;
		      lista[8] = 1;
		      for (i = 0; i < NPARAMS; i++)
			{
			  anr[i + 1] = a[i];
			}
		      a[2] = -y[0];
		      for (i = 1; i <= np; i++)
			ytemp[i] = ynr[1 + (i - 1) * np];	/*T1 data */
		      for (i = 0; i < NPARAMS; i++)
			{
			  anr[i + 1] = a[i];
			}
		      lm_init (d2array, ytemp, signr, np, anr, lista, NPARAMS,
			      covar, alpha, &chisq, t1func);
		      itstgrid = 0;
		      for (;;)
			{
			  ochisq = chisq;
			  lm_iterate (d2array, ytemp, signr, np, anr, lista,
				  NPARAMS, covar, alpha, &chisq, t1func);
			  k++;
			  if (k > 999)
			    {
			      Wscrprintf ("Warning:  corrupt input data");
			      break;
			    }
			  if (chisq > ochisq)
			    itstgrid = 0;
			  else
			    {
			      if (fabs (ochisq - chisq) < 0.01 * sumsq)
				itstgrid++;
			      else
				itstgrid = 0;
			    }
			  if (itstgrid < MAXITERS)
			    continue;
			  lm_covar (d2array, ytemp, signr, np, anr, lista,
				  NPARAMS, covar, alpha, &chisq, t1func);
			  break;
			}
		      for (j = 0; j < NPARAMS; j++)
			a[j] = anr[j + 1];
		      fit_failed_flag = 3;	/*So that ncomp can be reset for next peak */
		      break;
		    }
		}

#ifdef UNDEF

#endif
	    }
	  /*Resetting proper values of mfit and lista */

	  for (i = 0; i < (NPARAMS + 1); i++)
	    lista[i] = 0;
	  switch (ncomp)
	    {
	    case 2:
	      lista[2] = 1;
	      lista[3] = 1;
	      lista[4] = 1;
	      lista[5] = 1;
	      mfit = 7;
	      lista[6] = 1;
	      lista[7] = 1;
	      lista[8] = 1;
	      a[3] = a[3] / DSCALE;
	      a[4] = a[4] / DSCALE;
	      break;

	    default:
	      lista[3] = 1;
	      lista[4] = 1;
	      mfit = 5;
	      lista[6] = 1;
	      lista[8] = 1;
	      break;
	    }


	  printf ("Peak %d is done  : %d to go \n", npeak_it, (n - npeak_it));
	  npeak_it++;
	}


      else if (ncomp == 2)
	{
	  /*For biexponential fitting */
	  /*{{{ */

	  /* shift along one for NR */
	  /* MN 15ix04 lets copy all of the values, even for t1 data */
	  for (i = 0; i < (np * d2dim); i++)
	    {
	      ynr[i + 1] = y[i];
	    }

	  /* set initial guesses for the fitting parameters */
	  /*MN 11x04 This should be done only for non-T1 data */
	  a[0] = 0.0;
	  a[1] = 0.0;
	  a[2] = y[0];
	  sumsq = 0;
	  for (i = 0; i < np; i++)
	    {
	      sumsq = sumsq + (y[i] * y[i]);
	    }
	  i = 0;
	  while (fabs (y[++i]) >= fabs (0.5 * a[2]) && (i < np - 1))
             ;
/* prevent estimate failing if y[i - 1] = y[i] */
        fac=x2[i - 1];
        if ((y[i - 1] > y[i])&&((0.5 * a[2] - y[i]) < (y[i - 1] - y[i])))
	  fac =
	    x2[i - 1] + (0.5 * a[2] - y[i]) / (y[i - 1] - y[i]) * (x2[i] -
								   x2[i - 1]);
/* perform sanity check on estimated diffusion coefficient */
	  if ((fac < x2[2]) || (fac > x2[np - 1]))
	    {
	      fac = x2[np / 2];
	    }
	  a[3] =
	    0.693 / (DSCALE * dosyconstant * 32767.0 * 32767.0 * 0.0001 *
		     gcal_ * gcal_ * fac);
	  /* Manchester 6.1B   16xi00   */
	  if (a[2] == 0.0)
	    a[2] = 1.0;
	  /* GAM 13v03 if (!(a[3]>0.0 && a[3]<1.0e-5)) a[3]=1.0e-10; */

	  errorflag = 0;
	  /*MN 09iii05 Getting the starting value as a monoexp */
	  /*{{{ */
	  k = 1;
	  ncomp = 1;
	  mfit = 2;
	  for (i = 0; i < NPARAMS; i++)
	    lista[i + 1] = 0;
	  lista[3] = 1;
	  lista[4] = 1;
	  for (i = 0; i < NPARAMS; i++)
	    {
	      anr[i + 1] = a[i];
	    }
	  lm_init (x2nr, ynr, signr, np, anr, lista, NPARAMS, covar, alpha,
		  &chisq, dosyfunc);
	  itstgrid = 0;
	  for (;;)
	    {
	      ochisq = chisq;
	      lm_iterate (x2nr, ynr, signr, np, anr, lista, NPARAMS, covar, alpha,
		      &chisq, dosyfunc);
	      k++;
	      if (k > 999)
		{
		  Wscrprintf ("Warning:  corrupt input data");
		  break;
		}
	      if (chisq > ochisq)
		itstgrid = 0;
	      else
		{
		  if (fabs (ochisq - chisq) < 0.01 * sumsq)
		    itstgrid++;
		  else
		    itstgrid = 0;
		}
	      if (itstgrid < MAXITERS)
		continue;
	      lm_covar (x2nr, ynr, signr, np, anr, lista, NPARAMS, covar, alpha,
		      &chisq, dosyfunc);
	      break;
	    }
	  for (j = 0; j < NPARAMS; j++)
	    a[j] = anr[j + 1];
	  ncomp = 2;
/*}}}*/
/* Then do the gridfit*/
/*{{{*/
	  if (!Dfixflag)
	    {
	      mfit = 4;
	      for (i = 0; i < NPARAMS; i++)
		lista[i + 1] = 0;
	      lista[2] = 1;
	      lista[3] = 1;
	      lista[4] = 1;
	      lista[5] = 1;

	      dgrid = 7;	/* gives a dgrid*2+1 grid */
	      agrid = 5;
	      dstart = a[3];
	      kgrid = 1;
	      for (ita = 1; ita <= (dgrid * 2 + 1); ita++)
		{
		  if ((dgrid - ita + 1) > 0)
		    {
		      dtemp1 = (dgrid - ita + 2) * dstart;
		    }
		  else if ((dgrid - ita + 1) < 0)
		    {
		      dtemp1 = dstart / (-1.0 * (dgrid - ita + 0));
		    }
		  else
		    {
		      dtemp1 = dstart;
		    }
		  for (itb = 1; itb <= (dgrid * 2 + 1); itb++)
		    {
		      if ((dgrid - itb + 1) > 0)
			{
			  dtemp2 = (dgrid - itb + 2) * dstart;
			}
		      else if ((dgrid - itb + 1) < 0)
			{
			  dtemp2 = dstart / (-1.0 * (dgrid - itb + 0));
			}
		      else
			{
			  dtemp2 = dstart;
			}
		      for (itc = 1; itc <= (agrid + 1); itc++)
			{
			  atemp1 = (agrid - itc + 1) * y[0] / agrid;
			  atemp2 = y[0] - atemp1;
			  anr[2] = atemp1;
			  anr[3] = atemp2;
			  anr[4] = dtemp1;
			  anr[5] = dtemp2;
			  for (j = 0; j < NPARAMS; j++)
			    a[j] = anr[j + 1];
			  lm_init (x2nr, ynr, signr, np, anr, lista, NPARAMS,
				  covar, alpha, &chisq, dosyfunc);
			  k = 1;
			  itstgrid = 0;
			  for (;;)
			    {
			      k++;
			      ochisq = chisq;
			      lm_iterate (x2nr, ynr, signr, np, anr, lista,
				      NPARAMS, covar, alpha, &chisq, dosyfunc);
			      if (k > 999)
				{
				  Wscrprintf ("Warning:  corrupt input data");
				  break;
				}
			      if (chisq > ochisq)
				itstgrid = 0;
			      else if (fabs (ochisq - chisq) < 0.01 * sumsq)
				itstgrid++;
			      if (itstgrid < MAXITERS)
				continue;
			      lm_covar (x2nr, ynr, signr, np, anr, lista,
				      NPARAMS, covar, alpha, &chisq, dosyfunc);
			      break;
			    }
			  if (kgrid == 1)
			    {
			      for (j = 0; j < NPARAMS; j++)
				guessnr[j] = anr[j + 1];
			      chigrid = chisq;
			      /*covar is needed for sanity check */
			      for (j = 1; j <= NPARAMS; j++)
				{
				  for (i = 1; i <= NPARAMS; i++)
				    {
				      gridcovar[i][j] = covar[i][j];
				    }
				}

			      kgrid = 0;
			    }
			  if (chisq < chigrid)
			    {
			      for (j = 0; j < NPARAMS; j++)
				guessnr[j] = anr[j + 1];
			      chigrid = chisq;
			      /*covar is needed for sanity check */
			      for (j = 1; j <= NPARAMS; j++)
				{
				  for (i = 1; i <= NPARAMS; i++)
				    {
				      gridcovar[i][j] = covar[i][j];
				    }
				}

#ifdef UNDEF
#endif
			    }
			}
		    }
		}
	      for (i = 0; i < NPARAMS; i++)
		a[i] = guessnr[i];

	      /* Do a sanity check of the fitted data */
	      a[3] = a[3] * DSCALE;
	      a[4] = a[4] * DSCALE;
	      /* adding error facs for all the 4 parameters in a biexp fitting */
	      /* covar is now 1..NPARAMS x 1..NPARAMS */
	      factmp = chigrid / (double) ((np * d2dim) - mfit);
	      factmp = sqrt (factmp);
	      for (i = 1; i <= NPARAMS; i++)
		{
		  facb[i] = factmp * sqrt (gridcovar[i][i]);
		}
	      facb[4] *= DSCALE;
	      facb[5] *= DSCALE;

	      for (i = 1; i <= FIT_ERROR_TYPES; i++)
		{
		  fit_failed_type[i] = 0;
		}

	      if ((a[3] < 0.0) || (a[4] < 0.0))
		{		/*Negative Diffusion coefficients */
		  fit_failed_type[1] = 1;
		}

	      if ((a[3] > 100.0) || (a[4] > 100.0))
		{		/*Diffusion coefficients to large (at least for liquids) */
		  fit_failed_type[2] = 1;
		}

	      if ((a[2] < 0.0) || (a[1] < 0.0))
		{		/*Negative amplitudes */
		  fit_failed_type[3] = 1;
		}
	      if (((100.0 * facb[4] / a[3]) > MAX_ERROR)
		  || ((100.0 * facb[5] / a[4]) > MAX_ERROR))
		{		/*Stdev to high on diff. coeff. */
		  fit_failed_type[4] = 1;
		}

	      if (((100.0 * facb[2] / a[1]) > MAX_ERROR)
		  || ((100.0 * facb[3] / a[2]) > MAX_ERROR))
		{		/*Stdev to high on amplitude */
		  fit_failed_type[5] = 1;
		}

	      for (j = 1; j <= FIT_ERROR_TYPES; j++)
		{
		  if (fit_failed_type[j] == 1)
		    {
		      Wscrprintf
			("Biexponential fit failed on peak %d. Writing to the \"fit_errors\" file and defaulting to a monoexponential fit \n",
			 lr + 1);
		      fprintf (errorfile, "\t\tBiexponential fitting\n\n");
		      fprintf (errorfile,
			       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area) + a3*exp(-a4*gradient_area)\n");
		      if (nugflag)
			fprintf (errorfile,
				 "Non-uniform gradient compensation used (not shown in equations)\n");
		      fprintf (errorfile,
			       "Biexponential fit failed on peak %d ",
			       lr + 1);
		      if (fit_failed_type[1] == 1)
			{
			  fprintf (errorfile,
				   "Negative diffusion coefficient(s) found\n");
			}
		      if (fit_failed_type[2] == 1)
			{
			  fprintf (errorfile,
				   "Diffusion coefficient(s) to large (D>100 m2/s) \n");
			}
		      if (fit_failed_type[3] == 1)
			{
			  fprintf (errorfile,
				   "Negative amplitude(s) found \n");
			}
		      if (fit_failed_type[4] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on diffusion coeffcient(s) to large \n");
			}
		      if (fit_failed_type[5] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on amplitude(s) to large \n");
			}
		      fprintf (errorfile, "Frequency %lf\n", frq[lr]);
		      fprintf (errorfile,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g) a3 %3.5g (%3.5g) a4 %3.5g (%3.5g)\n",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4], a[1], facb[2], 1.0e10 * a[4],
			       1.0e10 * facb[5]);
		      fprintf (errorfile, "\n\n");


		      /*Set the correct values for a monoexponential fit */
		      ncomp = 1;
		      mfit = 2;
		      for (i = 0; i < NPARAMS; i++)
			lista[i + 1] = 0;
		      lista[3] = 1;
		      lista[4] = 1;
		      fit_failed_flag = 1;	/*So that ncomp can be reset for next peak */

		      a[0] = 0.0;
		      a[1] = 0.0;
		      a[2] = y[0];
		      sumsq = 0;
		      for (i = 0; i < np; i++)
			{
			  sumsq = sumsq + (y[i] * y[i]);
			}
		      i = 0;
		      while (fabs (y[++i]) >= fabs (0.5 * a[2])
			     && (i < np - 1));
/* prevent estimate failing if y[i - 1] = y[i] */
        fac=x2[i - 1];
        if ((y[i - 1] > y[i])&&((0.5 * a[2] - y[i]) < (y[i - 1] - y[i])))
		      fac =
			x2[i - 1] + (0.5 * a[2] - y[i]) / (y[i - 1] -
							   y[i]) * (x2[i] -
								    x2[i -
								       1]);
/* perform sanity check on estimated diffusion coefficient */
		      if ((fac < x2[2]) || (fac > x2[np - 1]))
			{
			  fac = x2[np / 2];
			}
		      a[3] =
			0.693 / (DSCALE * dosyconstant * 32767.0 * 32767.0 *
				 0.0001 * gcal_ * gcal_ * fac);
		      /* Manchester 6.1B   16xi00   */
		      if (a[2] == 0.0)
			a[2] = 1.0;
		      /* GAM 13v03 if (!(a[3]>0.0 && a[3]<1.0e-5)) a[3]=1.0e-10; */
		      errorflag = 0;

		      break;
		    }
		}


	      if (ncomp == 2)
		{
		  for (i = 0; i < NPARAMS; i++)
		    a[i] = guessnr[i];
		}


#ifdef UNDEF
#endif
	    }
	  else if ((Dfixflag))
	    {

	      mfit = 3;
	      for (i = 0; i < NPARAMS; i++)
		lista[i + 1] = 0;
	      lista[2] = 1;
	      lista[3] = 1;
	      lista[4] = 1;;

	      dgrid = 7;	/* gives a dgrid*2+1 grid */
	      agrid = 5;
	      dstart = a[3];
	      kgrid = 1;
	      /*
	         for (i=0;i<NPARAMS;i++)      lista[i+1] = 0;
	         lista[1] = 0; lista[2] =1 ; lista[3] = 1; lista[4]=1; lista[5]=0;
	       */
	      for (ita = 1; ita <= (dgrid * 2 + 1); ita++)
		{
		  if ((dgrid - ita + 1) > 0)
		    {
		      dtemp1 = (dgrid - ita + 2) * dstart;
		    }
		  else if ((dgrid - ita + 1) < 0)
		    {
		      dtemp1 = dstart / (-1.0 * (dgrid - ita + 0));
		    }
		  else
		    {
		      dtemp1 = dstart;
		    }
		  for (itc = 1; itc <= (agrid + 1); itc++)
		    {
		      atemp1 = (agrid - itc + 1) * y[0] / agrid;
		      atemp2 = y[0] - atemp1;
		      anr[2] = atemp1;
		      anr[3] = atemp2;
		      anr[4] = dtemp1;
		      anr[5] = Dfix / 20.0;
#ifdef DEBUG_DOSYFIT
		      //      fprintf(debug,"%lf   %lf   %lf   %lf \n",anr[2],anr[3],anr[4],anr[5]);
#endif

		      for (j = 0; j < NPARAMS; j++)
			a[j] = anr[j + 1];
		      lm_init (x2nr, ynr, signr, np, anr, lista, NPARAMS,
			      covar, alpha, &chisq, dosyfunc);
		      k = 1;
		      itstgrid = 0;
		      for (;;)
			{
			  k++;
			  ochisq = chisq;
			  lm_iterate (x2nr, ynr, signr, np, anr, lista, NPARAMS,
				  covar, alpha, &chisq, dosyfunc);
			  if (k > 999)
			    {
			      Wscrprintf ("Warning:  corrupt input data");
			      break;
			    }
			  if (chisq > ochisq)
			    itstgrid = 0;
			  else if (fabs (ochisq - chisq) < 0.01 * sumsq)
			    itstgrid++;
			  if (itstgrid < MAXITERS)
			    continue;
			  lm_covar (x2nr, ynr, signr, np, anr, lista, NPARAMS,
				  covar, alpha, &chisq, dosyfunc);
			  break;
			}
		      if (kgrid == 1)
			{
			  for (j = 0; j < NPARAMS; j++)
			    guessnr[j] = anr[j + 1];
			  chigrid = chisq;
			  kgrid = 0;
			}
		      if (chisq < chigrid)
			{
			  for (j = 0; j < NPARAMS; j++)
			    guessnr[j] = anr[j + 1];
			  chigrid = chisq;
			  /*covar is needed for sanity check */
			  for (j = 1; j <= NPARAMS; j++)
			    {
			      for (i = 1; i <= NPARAMS; i++)
				{
				  gridcovar[i][j] = covar[i][j];
				}
			    }
			}
		    }
		}
	      for (i = 0; i < NPARAMS; i++)
		a[i] = guessnr[i];
	      a[4] = Dfix / 20.0;
#ifdef DEBUG_DOSYFIT
	      //fprintf(debug,"after fit: %lf   %lf   %lf   %lf \n",a[1],a[2],a[3],a[4]);
#endif
	      /* Do a sanity check of the fitted data */
	      a[3] = a[3] * DSCALE;
	      /* adding error facs for all the 4 parameters in a biexp fitting */
	      /* covar is now 1..NPARAMS x 1..NPARAMS */
	      factmp = chigrid / (double) ((np * d2dim) - mfit);
	      factmp = sqrt (factmp);
	      for (i = 1; i <= NPARAMS; i++)
		{
		  facb[i] = factmp * sqrt (gridcovar[i][i]);
		}
	      facb[4] *= DSCALE;

	      for (i = 1; i <= FIT_ERROR_TYPES; i++)
		{
		  fit_failed_type[i] = 0;
		}

	      if (a[3] < 0.0)
		{		/*Negative Diffusion coefficient */
		  fit_failed_type[1] = 1;
		}

	      if (a[3] > 100.0)
		{		/*Diffusion coefficient to large (at least for liquids) */
		  fit_failed_type[2] = 1;
		}

	      if ((a[2] < 0.0) || (a[1] < 0.0))
		{		/*Negative amplitudes */
		  fit_failed_type[3] = 1;
		}

	      if ((100.0 * facb[4] / a[3]) > MAX_ERROR)
		{		/*Stdev to high on diff. coeff. */
		  fit_failed_type[4] = 1;
		}

	      if (((100.0 * facb[2] / a[1]) > MAX_ERROR)
		  || ((100.0 * facb[3] / a[2]) > MAX_ERROR))
		{		/*Stdev to high on amplitude */
		  fit_failed_type[5] = 1;
		}

	      for (j = 1; j <= FIT_ERROR_TYPES; j++)
		{
		  if (fit_failed_type[j] == 1)
		    {
		      Wscrprintf
			("Biexponential fit (Dfix) failed on peak %d. Writing to the \"fit_errors\" file and defaulting to a monoexponential fit \n",
			 lr + 1);
		      fprintf (errorfile,
			       "\t\tBiexponential fitting - with one D value kept constant\n\n");
		      fprintf (errorfile,
			       "Exponential fit:\npeak height =  a1 * exp(-a2*gradient_area) + a3*exp(-a4*gradient_area)\n");
		      if (nugflag)
			fprintf (errorfile,
				 "Non-uniform gradient compensation used (not shown in equations)\n");
		      fprintf (errorfile,
			       "Biexponential fit failed on peak %d ",
			       lr + 1);
		      if (fit_failed_type[1] == 1)
			{
			  fprintf (errorfile,
				   "Negative diffusion coefficient found\n");
			}
		      if (fit_failed_type[2] == 1)
			{
			  fprintf (errorfile,
				   "Diffusion coefficient to large (D>100 m2/s) \n");
			}
		      if (fit_failed_type[3] == 1)
			{
			  fprintf (errorfile,
				   "Negative amplitude(s) found \n");
			}
		      if (fit_failed_type[4] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on diffusion coeffcient to large \n");
			}
		      if (fit_failed_type[5] == 1)
			{
			  fprintf (errorfile,
				   "Stdev on amplitude(s) to large \n");
			}
		      fprintf (errorfile, "Frequency %lf\n", frq[lr]);
		      fprintf (errorfile,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g) a3 %3.5g (%3.5g) a4 %3.5g (%3.5g)\n",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4], a[1], facb[2], 1.0e10 * a[4],
			       1.0e10 * facb[5]);
		      fprintf (errorfile, "\n\n");


		      /*Set the correct values for a monoexponential fit */
		      ncomp = 1;
		      mfit = 2;
		      Dfixflag = 0;
		      for (i = 0; i < NPARAMS; i++)
			lista[i + 1] = 0;
		      lista[3] = 1;
		      lista[4] = 1;
		      fit_failed_flag = 2;	/*So that ncomp and Dfixflag can be reset for next peak */

		      a[0] = 0.0;
		      a[1] = 0.0;
		      a[2] = y[0];
		      sumsq = 0;
		      for (i = 0; i < np; i++)
			{
			  sumsq = sumsq + (y[i] * y[i]);
			}
		      i = 0;
		      while (fabs (y[++i]) >= fabs (0.5 * a[2])
			     && (i < np - 1));
/* prevent estimate failing if y[i - 1] = y[i] */
        fac=x2[i - 1];
        if ((y[i - 1] > y[i])&&((0.5 * a[2] - y[i]) < (y[i - 1] - y[i])))
		      fac =
			x2[i - 1] + (0.5 * a[2] - y[i]) / (y[i - 1] -
							   y[i]) * (x2[i] -
								    x2[i -
								       1]);
/* perform sanity check on estimated diffusion coefficient */
		      if ((fac < x2[2]) || (fac > x2[np - 1]))
			{
			  fac = x2[np / 2];
			}
		      a[3] =
			0.693 / (DSCALE * dosyconstant * 32767.0 * 32767.0 *
				 0.0001 * gcal_ * gcal_ * fac);
		      /* Manchester 6.1B   16xi00   */
		      if (a[2] == 0.0)
			a[2] = 1.0;
		      /* GAM 13v03 if (!(a[3]>0.0 && a[3]<1.0e-5)) a[3]=1.0e-10; */
		      errorflag = 0;

		      break;
		    }
		}

	      if (ncomp == 2)
		{
		  for (i = 0; i < NPARAMS; i++)
		    a[i] = guessnr[i];
#ifdef DEBUG_DOSYFIT
		  //fprintf(debug,"after fit and ncomp check: %lf   %lf   %lf   %lf \n",a[1],a[2],a[3],a[4]);
#endif
		}


	    }
	  else
	    {
	      Werrprintf ("dosyfit: Cannot find sensible fitting parameters");
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	      ;
	    }
	  /*}}} */

	  printf ("Peak %d is done  : %d to go \n", npeak_it, (n - npeak_it));
	  npeak_it++;
/*}}}*/
	}

      else
	{
	  /*For vanilla DOSY */
/*{{{*/
	  /* shift along one for NR */
	  /* MN 15ix04 lets copy all of the values, even for t1 data */
	  for (i = 0; i < np; i++)
	    {
	      ynr[i + 1] = y[i];
	    }

	  /* set initial guesses for the fitting parameters */
	  /*MN 11x04 This should be done only for non-T1 data */
	  a[0] = 0.0;
	  a[1] = 0.0;
	  a[2] = y[0];
	  sumsq = 0;
	  for (i = 0; i < np; i++)
	    {
	      sumsq = sumsq + (y[i] * y[i]);
	    }
	  i = 0;
	  while (fabs (y[++i]) >= fabs (0.5 * a[2]) && (i < np - 1))
             ;
/* prevent estimate failing if y[i - 1] = y[i] */
        fac=x2[i - 1];
        if ((y[i - 1] > y[i])&&((0.5 * a[2] - y[i]) < (y[i - 1] - y[i])))
	  fac =
	    x2[i - 1] + (0.5 * a[2] - y[i]) / (y[i - 1] - y[i]) * (x2[i] -
								   x2[i - 1]);
/* perform sanity check on estimated diffusion coefficient */
	  if ((fac < x2[2]) || (fac > x2[np - 1]))
	    {
	      fac = x2[np / 2];
	    }

	  a[3] =
	    0.693 / (DSCALE * dosyconstant * 32767.0 * 32767.0 * 0.0001 *
		     gcal_ * gcal_ * fac);
#ifdef DEBUG_DOSYFIT
	  fprintf (debug,
		   "peak %d  a[2] = %f   a[3] = %f y[%d] = %f  y[%d] = %f\n",
		   lr + 1, a[2], a[3], i, y[i], i - 1, y[i - 1]);
	  fclose (debug);
	  strcpy (rubbish, curexpdir);
	  strcat (rubbish, "/dosy/debug_dosyfit");
	  debug = fopen (rubbish, "a");	/* file for debugging information */

#endif
	  /* Manchester 6.1B   16xi00   */
	  if (a[2] == 0.0)
	    a[2] = 1.0;
	  /* GAM 13v03 if (!(a[3]>0.0 && a[3]<1.0e-5)) a[3]=1.0e-10; */

	  errorflag = 0;
/*}}}*/
	}

/*Do the actual fitting */
      /*{{{ */
      /* GAM 7i04 */
      /* copy parameters into anr for NR */
      for (i = 0; i < NPARAMS; i++)
	{
	  anr[i + 1] = a[i];
	}

#ifdef DEBUG_DOSYFIT
      fprintf (debug, "before normal fitting of peak %d\n", lr);
      for (i = 0; i < NPARAMS; i++)
	{
	  //fprintf(debug,"a %d : %e\t",i,a[i]);
	}
      fprintf (debug, "\n");
#endif
      errorflag = 0;
      /* GAM 6i04 */
      /* MN 15ix04 using lm_init2d if T1data is to be fitted */
      if (T1flag)
	{
	  lm_init2d (xt1, ynr, signr, (np * d2dim), anr, lista, NPARAMS, covar,
		    alpha, &chisq, t1dosyfunc);
	}
      else
	{
	  lm_init (x2nr, ynr, signr, np, anr, lista, NPARAMS, covar, alpha,
		  &chisq, dosyfunc);
	}
      if (abortflag)
	{
	  if (ll2dflg)
	    {
	      free (peak);
	      free (peak_table);
	    }
	  fclosetest (in);
	  fclosetest (table);
	  fclosetest (fit_results);
	  fclosetest (D_spectrum);
	  fclosetest (errorfile);
	  fclosetest (cfile);
	  return (ERROR);
	}
      if (errorflag)
	continue;
      k = 1;
      itst = 0;
      for (;;)
	{
	  k++;
	  ochisq = chisq;
	  /* GAM 6i04 */
	  if (T1flag)
	    {
	      lm_iterate2d (xt1, ynr, signr, (np * d2dim), anr, lista, NPARAMS,
			covar, alpha, &chisq, t1dosyfunc);
	    }
	  else
	    {
	      lm_iterate (x2nr, ynr, signr, np, anr, lista, NPARAMS, covar, alpha,
		      &chisq, dosyfunc);
	    }

	  if (abortflag)
	    {
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }
	  /* MN 09ix04 I dont think 19 is sufficient for a biexp fit, changing it to 999 */
	  if (k > 999)
	    {
	      Wscrprintf ("Warning:  corrupt input data - in main fit loop");
	      break;
	    }

	  if (errorflag)
	    break;
	  if (chisq > ochisq)
	    itst = 0;
	  else
	    {
	      if (fabs (ochisq - chisq) < 0.01 * sumsq)
		itst++;
	      else
		itst = 0;
	    }
	  if (itst < MAXITERS)
	    continue;
	  /* GAM 6i04 */
	  if (T1flag)
	    {
	      lm_covar2d (xt1, ynr, signr, (np * d2dim), anr, lista, NPARAMS,
			covar, alpha, &chisq, t1dosyfunc);
	    }
	  else
	    {
	      lm_covar (x2nr, ynr, signr, np, anr, lista, NPARAMS, covar, alpha,
		      &chisq, dosyfunc);
	    }
/*}}}*/
	  /* copy parameters back out of anr  */


	  for (i = 0; i < NPARAMS; i++)
	    {
	      a[i] = anr[i + 1];
	    }
#ifdef DEBUG_DOSYFIT
	  fprintf (debug, "after Normal fitting\n");
#endif
	  for (i = 0; i < NPARAMS; i++)
	    {
	      //fprintf(debug,"a %d : %e\t",i,a[i]);
	    }
#ifdef DEBUG_DOSYFIT
	  fprintf (debug, "\n");
#endif
	  if (abortflag)
	    {
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }
	  if (errorflag)
	    break;
	  a[3] = a[3] * DSCALE;
	  /* MN 21v04 Scaling the D value for the second exponential */
	  /* MN15ix04 Changing from np to np*d2dim to get the correct value */
	  if (ncomp == 2)
	    {
	      a[4] = a[4] * DSCALE;
	    }
	  if (!errorflag)
	    {
	      fac = chisq / (double) ((np * d2dim) - mfit);
	      fac = sqrt (fac);
	      /* covar is now 1..NPARAMS x 1..NPARAMS */
/* MN 20ix04 fac will probably not be used in the future, but i'll keep it in for now */
	      fac *= sqrt (covar[4][4]);
	      fac *= DSCALE;
	      /* MN 24v04 adding error facs for all the 4 parameters in a biexp fitting */
	      factmp = chisq / (double) ((np * d2dim) - mfit);
	      factmp = sqrt (factmp);
	      for (i = 1; i <= NPARAMS; i++)
		{
		  facb[i] = factmp * sqrt (covar[i][i]);
		  /* MN 15ix04 the t1 parametrs are not scaled by DSCALE */
		}
	      facb[4] *= DSCALE;
	      facb[5] *= DSCALE;
	      /* GAM 7i04 a[3] and fac are now the actual D and error respectively, scaled back down */
	      /* MN 09ix04 Table printed out for the fixed fit, using 0.5% standard error for the fixed parameter */
	      /* MN 13ix04 Printing out values for biexp and fixed fit to D_spectrum */
	      /* if 2D dosy write out to diffusion_display.inp */
	      if (!ll2dflg)
		{
		  if (ncomp == 2)
		    {
		      switch (mfit)
			{
			case 3:
			  if (!(facb[4] < 1.0e-07))
			    {
			      facb[4] = 1.0e-07;
			    }
			  fprintf (table,
				   "%10.3lf\t%10.4lf\t%10.4lf\t%10.6lf\t%10.4lf\t%10.4lf\t%10.6lf\n",
				   frq[lr], a[2], 1.0e10 * a[3],
				   1.0e10 * facb[4], a[1], 1.0e10 * a[4],
				   1.0e10 * a[4] * 0.005);
			  fprintf (D_spectrum,
				   "%10.3lf,%10.4lf,%10.4lf,%10.2lf\n",
				   1.0e10 * a[3], a[2] / (1.0e10 * facb[4]),
				   gwidth * 1.0e10 * facb[4], 1.0);
			  fprintf (D_spectrum,
				   "%10.3lf,%10.4lf,%10.4lf,%10.2lf\n",
				   1.0e10 * a[4],
				   a[1] / (1.0e10 * a[4] * 0.005),
				   gwidth * 1.0e10 * a[4] * 0.005, 1.0);

			  break;

			default:

			  if (!(facb[4] < 1.0e-07))
			    {
			      facb[4] = 1.0e-07;
			    }
			  if (!(facb[5] < 1.0e-07))
			    {
			      facb[5] = 1.0e-07;
			    }
			  fprintf (table,
				   "%10.3lf\t%10.4lf\t%10.4lf\t%10.6lf\t%10.4lf\t%10.4lf\t%10.6lf\n",
				   frq[lr], a[2], 1.0e10 * a[3],
				   1.0e10 * facb[4], a[1], 1.0e10 * a[4],
				   1.0e10 * facb[5]);
			  if (a[2] > 0.0)
			    fprintf (D_spectrum,
				     "%10.3lf,%10.4lf,%10.4lf,%10.2lf\n",
				     1.0e10 * a[3], a[2] / (1.0e10 * facb[4]),
				     gwidth * 1.0e10 * facb[4], 1.0);
			  if (a[1] > 0.0)
			    fprintf (D_spectrum,
				     "%10.3lf,%10.4lf,%10.4lf,%10.2lf\n",
				     1.0e10 * a[4], a[1] / (1.0e10 * facb[5]),
				     gwidth * 1.0e10 * facb[5], 1.0);
			  break;
			}
		    }		/* if ncomp==2 */
		  else
		    {
		      /*MN 26v04 Printing zeros for parameters not used by monoexp fit - to facilitate the code in ddif */
		      if (!(fac < 1.0e-07))
			{
			  fac = 1.0e-07;
			}
		      fprintf (table,
			       "%10.3lf\t%10.4lf\t%10.4lf\t%10.6lf\t%10.4lf\t%10.4lf\t%10.6lf\n",
			       frq[lr], a[2], 1.0e10 * a[3], 1.0e10 * fac,
			       0.0, 0.0, 0.0);
		      /* The linewidths in 'diffusion_spectrum' file are no longer multiplied by 2 */
		      if (a[2] > 0.0)
			fprintf (D_spectrum,
				 "%10.3lf,%10.4lf,%10.4lf,%10.2lf\n",
				 1.0e10 * a[3], a[2] / (1.0e10 * fac),
				 gwidth * 1.0e10 * fac, 1.0);
		    }
		}
	      /* else update the ll2d labels and write out to diffusion_display_3D.inp */
	      else		/* ll2dflag */
		{
		  if (a[3] > 0.0 && ((100.0 * fac / a[3]) < MAX_ERROR))
		    {
		      fprintf (table, "%5d\t%10.4lf\t%10.6lf\n", peak[0]->key,
			       1.0e10 * a[3], 1.0e10 * fac);
		      /* DON'T change following line to add 0.05 to standard error */
		      fprintf (D_spectrum,
			       "%10.2lf,%10.4lf,%10.4lf,%10.2lf\n",
			       1.0e10 * a[3], a[2] / (1.0e10 * fac),
			       gwidth * 1.0e10 * fac + 0.0, 1.0);
		      /* display the results of 3D analysis using the label facility of the 'll2d' program */
		      i = lo = 0;
		      /* First loop to copy diff. coef., second loop to copy estimated error */
		      while (lo < 2)
			{
			  jo = ko = 0;
			  /* look for the correct rounded value of xt */
			  xtf = (lo == 0) ? (1.0e12 * a[3]) : (1.0e12 * fac);
			  xtlow = xtf;
			  xthigh = xtlow + 1;
			  dhigh = (float) xthigh - xtf;
			  dlow = xtf - (float) xtlow;
			  if (dhigh <= dlow)
			    xt = xo = xthigh;
			  else
			    xt = xo = xtlow;
			  xtf = (float) xt;
			  io = 1;
			  while (xt >= 1.0)
			    {
			      xt = xo / pow (10.0, io++);
			    }
			  io -= 2;
			  ko = TRUE;
			  xt = xo;
			  /* now copy the number into the string 'str1' */
			  while (io >= 0)
			    {
			      if (io <= 1 && ko == TRUE)
				{
				  str1[i++] = '0';
				  str1[i++] = '.';
				  if (io == 0)
				    str1[i++] = '0';
				}
			      else if (io == 1)
				str1[i++] = '.';
			      xt = xt - (jo * pow (10.0, io + 1));
			      jo = xt / pow (10.0, io--);
			      str = jo + '0';
			      if (ko == TRUE)
				{
				  str1[i++] = str;
				  ko = FALSE;
				}
			      else
				str1[i++] = str;
			    }
			  if (lo == 0)
			    {
			      str1[i++] = ' ';
			      str1[i++] = '(';
			    }
			  if (lo == 1)
			    {
			      if (xtf < 1.0)
				{
				  str1[i++] = '0';
				}
			      str1[i++] = ')';
			    }
			  lo++;
			}
		      while (i < LABEL_LEN - 1)
			str1[i++] = '\0';
		    }
		  /* if not a good diffusion coefficient signal it by label 'x' */
		  else
		    {
		      i = 0;
		      while (i < LABEL_LEN - 1)
			str1[i++] = '\0';
		      /*
		         str1[0] = 'x';
		       */
		    }
		  strcpy (peak[0]->label, str1);
		  if (lr == 0)
		    {
		      strcpy (fname, curexpdir);
		      strcat (fname, "/ll2d/peaks.bin");
		      old_ptr = peak_table[0]->file;
		      if ((peak_table[0]->file = fopen (fname, "r+")) == NULL)
			{
			  Werrprintf ("dosyfit: unable to open %s\n", fname);
			  fclosetest (in);
			  fclosetest (table);
			  fclosetest (fit_results);
			  fclosetest (D_spectrum);
			  fclosetest (errorfile);
			  fclosetest (cfile);
			  return (ERROR);
			}
		    }
		  write_peak_file_record (peak_table[0], peak[0],
					  peak[0]->key);
		}
	      if (!ll2dflg)
		fprintf (fit_results, "Frequency %lf\n", frq[lr]);
	      else
		fprintf (fit_results, "\n\tPeak number %d :\n", peak[0]->key);
	      /*MN 13ix04 Printing out the values for biexp och fixed fit when that applies */
	      /*MN 15ix04 prining out the values for the appropriate parameters SHOULD DO THE STAVS AS WELL */
	      switch (ncomp)
		{
		case 2:
		  if (T1flag)
		    {
		      fprintf (fit_results,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g) a3 %3.5g (%3.5g) a4 %3.5g (%3.5g) ",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4], a[1], facb[2], 1.0e10 * a[4],
			       1.0e10 * facb[5]);
		      fprintf (fit_results,
			       "a5 %3.5g (%3.5g) a6 %3.5g (%3.5g) a7 %3.5g (%3.5g)\n",
			       a[5], facb[6], a[6], facb[7], a[7], facb[8]);
		    }
		  else
		    {
		      fprintf (fit_results,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g) a3 %3.5g (%3.5g) a4 %3.5g (%3.5g)\n",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4], a[1], facb[2], 1.0e10 * a[4],
			       1.0e10 * facb[5]);
		    }

		  break;

		default:
		  if (T1flag)
		    {
		      fprintf (fit_results,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g) ",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4]);
		      fprintf (fit_results,
			       "a5 %3.5g (%3.5g) a7 %3.5g (%3.5g)\n", a[5],
			       facb[6], a[7], facb[8]);
		    }
		  else
		    {
		      fprintf (fit_results,
			       "a0 %10.5lf a1 %3.5g (%3.5g) a2 %3.5g (%3.5g)\n",
			       a[0], a[2], facb[3], 1.0e10 * a[3],
			       1.0e10 * facb[4]);
		    }

		  break;
		}

	      fprintf (fit_results,
		       "Gradient area (ns/m2)  exp. height    calc. height          Diff\n");
	      /* GAM 7i04 use dosyfunc to generate calculated attenuation for output */
	      /* MN 20ix04 use t1dosyfunc for t1dosy - and np*d2dim datapoints */

	      for (i = 0; i < (np * d2dim); i++)
		{
		  if (T1flag)
		    {
		      t1dosyfunc (xt1[i + 1], anr, &cy, dummydyda, NPARAMS);
		    }
		  else
		    {
		      dosyfunc (x2[i], anr, &cy, dummydyda, NPARAMS);
		    }
		  yc[i] = cy;
		  y_diff[i][lr] = y[i] - yc[i];
		  y_fitted[i][lr] = yc[i];
		  if (T1flag)
		    {
		      fprintf (fit_results,
			       "%10.6lf %15.6lf %15.6lf %15.6lf\n",
			       xt1[i + 1].grad, y[i], yc[i], y[i] - yc[i]);
		    }
		  else
		    {
		      fprintf (fit_results,
			       "%10.6lf %15.6lf %15.6lf %15.6lf\n",
			       dosyconstant * (1.0e-9) * 0.0001 * 32767.0 *
			       32767.0 * gcal_ * gcal_ * x2[i], y[i], yc[i],
			       y[i] - yc[i]);
		    }
		}
	      /* push the pointers to the next peak */
	      if (ll2dflg && lr < (n - 1))
		for (i = 0; i < (np + badinc); i++)
		  peak[i] = peak[i]->next;
	      if (fac / a[3] < 0.075)
		{
		  rej_pk[lr] = 0;
		  fac = -1.0 / a[3];
		  for (i = 0; i < np; i++)
		    {
		      /*      calculate the estimated value of x from the fit coefficients and the experimental y value */
		      /*      peak height sum represents total signal for a given gzlvl1 value */
		      /*      only do calculation if experimental signal greater than 1% of initial */

		      if (y[i] > 0.01 * a[2])
			{
			  peak_height_sum[i] += y[i];
			  xc[i] = fac * log ((y[i] - a[0]) / a[2]);
			  xdiff[i] += xc[i] * y[i];
			}
		    }
		  ct++;
		}
	      else
		rej_pk[lr] = 1;

	    }
	  break;
	}
      if (errorflag)
	{
	  fprintf (errorfile, "Fit failed on peak %d \n", lr + 1);
	}
    }				/* end of loop for the various resonances */

  /* Finally compile the statistics necessary for gradient calibration */
  /* if there is more than one peak                                    */
/*{{{*/
  if (ct > 1)
    {
      for (i = 0; i < (np * d2dim); i++)
	{
	  y_diff_wa[i] = 0.0;
	  y_sum[i] = 0.0;
	  y_dd[i] = 0.0;
	  for (j = 0; j < n; j++)
	    {
	      if (rej_pk[j] == 0)
		{
		  y_diff_wa[i] += y_diff[i][j];
		  y_sum[i] += y_fitted[i][j];
		}
	    }
	  y_diff_wa[i] /= y_sum[i];
	}
      /* y_diff_wa[i] contains the weighted average difference for gzlvl1[i] value, */
      /* still need to calculate standard deviation */
      for (i = 0; i < (np * d2dim); i++)
	{
	  y_dd[i] = 0.0;
	  for (j = 0; j < n; j++)
	    {
	      if (rej_pk[j] == 0)
		{
		  y_dd[i] +=
		    SQR ((y_diff[i][j] / y_fitted[i][j]) -
			 y_diff_wa[i]) * (y_fitted[i][j] / y_sum[i]);

		}
	    }
	  y_dd[i] /= (double) (n - 1);
	  y_std[i] = sqrt (y_dd[i]);
	}
      fprintf (fit_results,
	       "\nThe average percentage differences between experimental and\n");
      fprintf (fit_results,
	       "calculated data points are, for the %d values of gzlvl1:\n\n",
	       np);
      fprintf (fit_results,
	       "Percentage           Standard deviation (per cent)\n");
      for (i = 0; i < (np * d2dim); i++)
	{
	  fprintf (fit_results,
		   " %2.5lf			     %2.5lf\n",
		   y_diff_wa[i] * 100.0, y_std[i] * 100.0);
	  if (!calibflag && (fabs (y_diff_wa[i] / y_std[i]) > 3.0))
	    systematic_dev_flg = TRUE;
	}
      if (!ll2dflg && systematic_dev_flg == TRUE)
	Winfoprintf
	  ("Systematic deviations from exponential decay detected; these may be caused by signal overlap, polydispersity or non-uniform field gradients  \n");
      if (!calibflag)
	{
	  fclosetest (cfile);
	  strcpy (rubbish, curexpdir);
	  strcat (rubbish, "/dosy/calibrated_gradients");
	  cfile = fopen (rubbish, "w");
	  if (!cfile)
	    {
	      Werrprintf ("dosyfit: could not open file %s", rubbish);
	      if (ll2dflg)
		{
		  free (peak);
		  free (peak_table);
		}
	      free_matrix (covar, 1, NPARAMS, 1, NPARAMS);
	      free_matrix (alpha, 1, NPARAMS, 1, NPARAMS);
	      free_matrix (y_diff, 0, (np * d2dim), 0, n);
	      free_matrix (y_fitted, 0, (np * d2dim), 0, n);
	      fclosetest (in);
	      fclosetest (table);
	      fclosetest (fit_results);
	      fclosetest (D_spectrum);
	      fclosetest (errorfile);
	      fclosetest (cfile);
	      return (ERROR);
	    }

	  for (i = 0; i < (np * d2dim); i++)
	    {
	      if (xdiff[i] == 0.0)
		xdiff[i] = x2[i] * peak_height_sum[i];
	      fprintf (cfile, "%.3lf\n", xdiff[i] / peak_height_sum[i]);
	    }
	  fflush (cfile);
	}
    }

  if (ll2dflg)
    {
      fclosetest (old_ptr);
      for (i = 0; i < (np + badinc); i++)
	{
	  fflush (peak_table[i]->file);
	  delete_peak_table (&peak_table[i]);
	}
    }
  disp_index (0);
  disp_status ("       ");
#ifdef DEBUG_DOSYFIT
  fprintf (debug, "End of dosyfit\n");
#endif
  free_matrix (covar, 1, NPARAMS, 1, NPARAMS);
  free_matrix (alpha, 1, NPARAMS, 1, NPARAMS);
  free_matrix (y_diff, 0, (np * d2dim), 0, n);
  free_matrix (y_fitted, 0, (np * d2dim), 0, n);

  fclosetest (in);
  fclosetest (table);
  fclosetest (fit_results);
  fclosetest (D_spectrum);
  fclosetest (errorfile);
  fclosetest (cfile);
#ifdef DEBUG_DOSYFIT
  fprintf (debug, "After fileclose\n");
  fclosetest (debug);
#endif
  RETURN;
/*}}}*/
}

 /*FUNCTIONS*/
/*{{{*/
  void
fclosetest (FILE * file)
{				/*{{{ */
  if (file)
    fclose (file);
}				/*}}} */

static double **
matrix (long nrl, long nrh, long ncl, long nch)
																																																						/* allocate a float matrix with subscript range m[nrl..nrh][ncl..nch] *//*{{{ */
{
  long i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
  double **m;
  /* allocate pointers to rows */
  m = (double **) malloc ((size_t) ((nrow + NR_END) * sizeof (double *)));
  if (!m)
    nrerror ("allocation failure 1 in matrix()");
  m += NR_END;
  m -= nrl;
  /* allocate rows and set pointers to them */
  m[nrl] =
    (double *) malloc ((size_t) ((nrow * ncol + NR_END) * sizeof (double)));
  if (!m[nrl])
    nrerror ("allocation failure 2 in matrix()");
  m[nrl] += NR_END;
  m[nrl] -= ncl;
  for (i = nrl + 1; i <= nrh; i++)
    m[i] = m[i - 1] + ncol;
  /* return pointer to array of pointers to rows */
  return m;
}				/*}}} */

static void
free_matrix (double **m, long nrl, long nrh, long ncl, long nch)
{				/*{{{ */
/* free a float matrix allocated by matrix() */
  free ((FREE_ARG) (m[nrl] + ncl - NR_END));
  free ((FREE_ARG) (m + nrl - NR_END));
}

void
t1dosyfunc (t1_x x, double a[], double *y, double dyda[], int na)
{				/*{{{ */
  /* MN 14ix04 I hope this function is correct - derivated in Matematica */
  /* MN 30ix04 Changed the minus signs to be compatible with dosyfunc */
  /* MN 30ix04 implemented NUG fitting */
  double conv, xfac;

  conv = gcal_ * 32767.0 * 0.01;
  conv = conv * conv;
  xfac = 1.0 * DSCALE * x.grad * dosyconstant * conv;

  switch (ncomp)
    {
    case 2:
      if (nugflag)
	{

	  *y =
	    a[1] + a[3] * exp (-nug[1] * (xfac * a[4]) -
			       nug[2] * pow ((xfac * a[4]),
					     2.0) -
			       nug[3] * pow ((xfac * a[4]),
					     3.0) -
			       nug[4] * pow ((xfac * a[4]),
					     4.0)) * (1.0 -
						      a[8] * exp (-1.0 *
								  (x.tau) /
								  a[6])) +
	    a[2] * exp (-nug[1] * (xfac * a[5]) -
			nug[2] * pow ((xfac * a[5]),
				      2.0) - nug[3] * pow ((xfac * a[5]),
							   3.0) -
			nug[4] * pow ((xfac * a[5]),
				      4.0)) * (1.0 -
					       a[8] * exp (-1.0 * (x.tau) /
							   a[7]));


	  dyda[1] = 1.0;
	  dyda[2] =
	    exp (-nug[1] * (xfac * a[5]) - nug[2] * pow ((xfac * a[5]), 2.0) -
		 nug[3] * pow ((xfac * a[5]),
			       3.0) - nug[4] * pow ((xfac * a[5]),
						    4.0)) * (1.0 -
							     a[8] *
							     exp (-1.0 *
								  (x.tau) /
								  a[7]));
	  dyda[3] =
	    exp (-nug[1] * (xfac * a[4]) - nug[2] * pow ((xfac * a[4]), 2.0) -
		 nug[3] * pow ((xfac * a[4]),
			       3.0) - nug[4] * pow ((xfac * a[4]),
						    4.0)) * (1.0 -
							     a[8] *
							     exp (-1.0 *
								  (x.tau) /
								  a[6]));
	  dyda[4] =
	    a[3] * exp (-nug[1] * (xfac * a[4]) -
			nug[2] * pow ((xfac * a[4]),
				      2.0) - nug[3] * pow ((xfac * a[4]),
							   3.0) -
			nug[4] * pow ((xfac * a[4]),
				      4.0)) * (1.0 -
					       a[8] * exp (-1.0 * (x.tau) /
							   a[6])) * (-nug[1] *
								     xfac -
								     2 *
								     nug[2] *
								     a[4] *
								     pow
								     (xfac,
								      2.0) -
								     3 *
								     nug[3] *
								     pow (a
									  [4],
									  2.0)
								     *
								     pow
								     (xfac,
								      3.0) -
								     4 *
								     nug[4] *
								     pow (a
									  [4],
									  3.0)
								     *
								     pow
								     (xfac,
								      4.0));
	  dyda[5] =
	    a[2] * exp (-nug[1] * (xfac * a[5]) -
			nug[2] * pow ((xfac * a[5]),
				      2.0) - nug[3] * pow ((xfac * a[5]),
							   3.0) -
			nug[4] * pow ((xfac * a[5]),
				      4.0)) * (1.0 -
					       a[8] * exp (-1.0 * (x.tau) /
							   a[7])) * (-nug[1] *
								     xfac -
								     2 *
								     nug[2] *
								     a[5] *
								     pow
								     (xfac,
								      2.0) -
								     3 *
								     nug[3] *
								     pow (a
									  [5],
									  2.0)
								     *
								     pow
								     (xfac,
								      3.0) -
								     4 *
								     nug[4] *
								     pow (a
									  [5],
									  3.0)
								     *
								     pow
								     (xfac,
								      4.0));
	  dyda[6] =
	    -1.0 *
	    ((a[3] * a[8] *
	      exp ((-nug[1] * (xfac * a[4]) -
		    nug[2] * pow ((xfac * a[4]),
				  2.0) - nug[3] * pow ((xfac * a[4]),
						       3.0) -
		    nug[4] * pow ((xfac * a[4]),
				  4.0)) -
		   (1.0 * x.tau / a[6])) * x.tau) / (a[6] * a[6]));
	  dyda[7] =
	    -1.0 *
	    ((a[2] * a[8] *
	      exp ((-nug[1] * (xfac * a[5]) -
		    nug[2] * pow ((xfac * a[5]),
				  2.0) - nug[3] * pow ((xfac * a[5]),
						       3.0) -
		    nug[4] * pow ((xfac * a[5]),
				  4.0)) -
		   (1.0 * x.tau / a[7])) * x.tau) / (a[7] * a[7]));
	  dyda[8] =
	    -a[3] * exp (-nug[1] * (xfac * a[4]) -
			 nug[2] * pow ((xfac * a[4]),
				       2.0) - nug[3] * pow ((xfac * a[4]),
							    3.0) -
			 nug[4] * pow ((xfac * a[4]),
				       4.0) - ((x.tau) / a[6])) -
	    a[2] * exp (-nug[1] * (xfac * a[5]) -
			nug[2] * pow ((xfac * a[5]),
				      2.0) - nug[3] * pow ((xfac * a[5]),
							   3.0) -
			nug[4] * pow ((xfac * a[5]), 4.0) - ((x.tau) / a[7]));
	}

      else
	{
	  *y =
	    a[1] + a[3] * (1.0 -
			   a[8] * exp (-1.0 * (x.tau) / a[6])) * exp (-xfac *
								      a[4]) +
	    a[2] * (1.0 -
		    a[8] * exp (-1.0 * (x.tau) / a[7])) * exp (-xfac * a[5]);

	  dyda[1] = 1.0;
	  dyda[2] =
	    exp (-a[5] * xfac) * (1.0 - a[8] * exp ((-1.0 * x.tau) / a[7]));
	  dyda[3] =
	    exp (-a[4] * xfac) * (1.0 - a[8] * exp ((-1.0 * x.tau) / a[6]));
	  dyda[4] =
	    -a[3] * exp (-a[4] * xfac) * (1.0 -
					  a[8] * exp ((-1.0 * x.tau) /
						      a[6])) * xfac;
	  dyda[5] =
	    -a[2] * exp (-a[5] * xfac) * (1.0 -
					  a[8] * exp ((-1.0 * x.tau) /
						      a[7])) * xfac;
	  dyda[6] =
	    -1.0 *
	    ((a[3] * a[8] * exp (-a[4] * xfac - (1.0 * x.tau / a[6])) *
	      x.tau) / (a[6] * a[6]));
	  dyda[7] =
	    -1.0 *
	    ((a[2] * a[8] * exp (-a[5] * xfac - (1.0 * x.tau / a[7])) *
	      x.tau) / (a[7] * a[7]));
	  dyda[8] =
	    -1.0 * a[3] * exp (-a[4] * xfac - (1.0 * x.tau / a[6])) -
	    1.0 * a[2] * exp (-a[5] * xfac - (1.0 * x.tau / a[7]));
	}
      break;

    default:
      if (nugflag)
	{

	  *y =
	    a[1] + a[3] * exp (-nug[1] * (xfac * a[4]) -
			       nug[2] * pow ((xfac * a[4]),
					     2.0) -
			       nug[3] * pow ((xfac * a[4]),
					     3.0) -
			       nug[4] * pow ((xfac * a[4]),
					     4.0)) * (1.0 -
						      a[8] * exp (-1.0 *
								  (x.tau) /
								  a[6]));


	  dyda[1] = 1.0;
	  dyda[2] = 0.0;
	  dyda[3] =
	    exp (-nug[1] * (xfac * a[4]) - nug[2] * pow ((xfac * a[4]), 2.0) -
		 nug[3] * pow ((xfac * a[4]),
			       3.0) - nug[4] * pow ((xfac * a[4]),
						    4.0)) * (1.0 -
							     a[8] *
							     exp (-1.0 *
								  (x.tau) /
								  a[6]));
	  dyda[4] =
	    a[3] * exp (-nug[1] * (xfac * a[4]) -
			nug[2] * pow ((xfac * a[4]),
				      2.0) - nug[3] * pow ((xfac * a[4]),
							   3.0) -
			nug[4] * pow ((xfac * a[4]),
				      4.0)) * (1.0 -
					       a[8] * exp (-1.0 * (x.tau) /
							   a[6])) * (-nug[1] *
								     xfac -
								     2 *
								     nug[2] *
								     a[4] *
								     pow
								     (xfac,
								      2.0) -
								     3 *
								     nug[3] *
								     pow (a
									  [4],
									  2.0)
								     *
								     pow
								     (xfac,
								      3.0) -
								     4 *
								     nug[4] *
								     pow (a
									  [4],
									  3.0)
								     *
								     pow
								     (xfac,
								      4.0));
	  dyda[5] = 0.0;
	  dyda[6] =
	    -1.0 *
	    ((a[3] * a[8] *
	      exp ((-nug[1] * (xfac * a[4]) -
		    nug[2] * pow ((xfac * a[4]),
				  2.0) - nug[3] * pow ((xfac * a[4]),
						       3.0) -
		    nug[4] * pow ((xfac * a[4]),
				  4.0)) -
		   (1.0 * x.tau / a[6])) * x.tau) / (a[6] * a[6]));
	  dyda[7] = 0.0;
	  dyda[8] =
	    -a[3] * exp (-nug[1] * (xfac * a[4]) -
			 nug[2] * pow ((xfac * a[4]),
				       2.0) - nug[3] * pow ((xfac * a[4]),
							    3.0) -
			 nug[4] * pow ((xfac * a[4]),
				       4.0) - ((x.tau) / a[6]));
	}
      else
	{
	  *y =
	    a[1] + a[3] * (1.0 -
			   a[8] * exp (-1.0 * (x.tau) / a[6])) * exp (-xfac *
								      a[4]);

	  dyda[1] = 1.0;
	  dyda[2] = 0.0;
	  dyda[3] =
	    exp (-a[4] * xfac) * (1.0 - a[8] * exp ((-1.0 * x.tau) / a[6]));
	  dyda[4] =
	    -a[3] * exp (a[4] * xfac) * (1.0 -
					 a[8] * exp ((-1.0 * x.tau) / a[6])) *
	    xfac;
	  dyda[5] = 0.0;
	  dyda[6] =
	    -1.0 *
	    ((a[3] * a[8] * exp (-a[4] * xfac - (1.0 * x.tau / a[6])) *
	      x.tau) / (a[6] * a[6]));
	  dyda[7] = 0.0;
	  dyda[8] = -1.0 * a[3] * exp (-a[4] * xfac - (1.0 * x.tau / a[6]));
	}
      break;
    }


}

/*}}}*/

void
t1func (double x, double a[], double *y, double dyda[], int na)
{				/*{{{ */

  *y = a[1] + a[3] * (1.0 - a[8] * exp (-1.0 * (x) / a[6]));

  dyda[1] = 1.0;
  dyda[2] = 0.0;
  dyda[3] = 1.0 - a[8] * exp ((-1.0 * x) / a[6]);
  dyda[4] = 0.0;
  dyda[5] = 0.0;
  dyda[6] =
    -1.0 * (a[3] * a[8] * (exp (-1.0 * x / a[6])) * x) / (a[6] * a[6]);
  dyda[7] = 0.0;
  dyda[8] = -1.0 * a[3] * exp (-1.0 * x / a[6]);


}

/*}}}*/

void
dosyfunc (double x, double a[], double *y, double dyda[], int na)
{				/*{{{ */
  /* GAM 6i04 :  only 2 or 3 parameter fit implemented at present  */
  /* MN 20v04 :  added the code for biexponential fitting (4 or 5 parameter fit) */
  /* MN 30ix04  monoexponentials gives the same result as mathematica */

  double conv, yc, xfac;

  conv = gcal_ * 32767.0 * 0.01;
  conv = conv * conv;
  xfac = 1.0 * DSCALE * x * dosyconstant * conv;


  switch (ncomp)
    {
    case 2:
      if (nugflag)
	{

	  *y =
	    a[1] + a[3] * exp (-nug[1] * (xfac * a[4]) -
			       nug[2] * pow ((xfac * a[4]),
					     2.0) -
			       nug[3] * pow ((xfac * a[4]),
					     3.0) -
			       nug[4] * pow ((xfac * a[4]),
					     4.0)) +
	    a[2] * exp (-nug[1] * (xfac * a[5]) -
			nug[2] * pow ((xfac * a[5]),
				      2.0) - nug[3] * pow ((xfac * a[5]),
							   3.0) -
			nug[4] * pow ((xfac * a[5]), 4.0));
	  dyda[1] = 1.0;
	  dyda[2] =
	    exp (-nug[1] * (xfac * a[5]) - nug[2] * pow ((xfac * a[5]), 2.0) -
		 nug[3] * pow ((xfac * a[5]),
			       3.0) - nug[4] * pow ((xfac * a[5]), 4.0));
	  dyda[3] =
	    exp (-nug[1] * (xfac * a[4]) - nug[2] * pow ((xfac * a[4]), 2.0) -
		 nug[3] * pow ((xfac * a[4]),
			       3.0) - nug[4] * pow ((xfac * a[4]), 4.0));
	  dyda[4] =
	    a[3] * exp (-nug[1] * (xfac * a[4]) -
			nug[2] * pow ((xfac * a[4]),
				      2.0) - nug[3] * pow ((xfac * a[4]),
							   3.0) -
			nug[4] * pow ((xfac * a[4]),
				      4.0)) * (-nug[1] * xfac -
					       2 * nug[2] * a[4] * pow (xfac,
									2.0) -
					       3 * nug[3] * pow (a[4],
								 2.0) *
					       pow (xfac,
						    3.0) -
					       4 * nug[4] * pow (a[4],
								 3.0) *
					       pow (xfac, 4.0));
	  dyda[5] =
	    a[2] * exp (-nug[1] * (xfac * a[5]) -
			nug[2] * pow ((xfac * a[5]),
				      2.0) - nug[3] * pow ((xfac * a[5]),
							   3.0) -
			nug[4] * pow ((xfac * a[5]),
				      4.0)) * (-nug[1] * xfac -
					       2 * nug[2] * a[5] * pow (xfac,
									2.0) -
					       3 * nug[3] * pow (a[5],
								 2.0) *
					       pow (xfac,
						    3.0) -
					       4 * nug[4] * pow (a[5],
								 3.0) *
					       pow (xfac, 4.0));
	}
      else
	{
	  *y = a[1] + a[3] * exp (-xfac * a[4]) + a[2] * exp (-xfac * a[5]);
	  dyda[1] = 1.0;
	  dyda[2] = exp (-xfac * a[5]);
	  dyda[3] = exp (-xfac * a[4]);
	  dyda[4] = -xfac * a[3] * exp (-xfac * a[4]);
	  dyda[5] = -xfac * a[2] * exp (-xfac * a[5]);

	}
      break;


    default:
      if (nugflag)
	{
	  *y =
	    a[1] + a[3] * exp (-nug[1] * (xfac * a[4]) -
			       nug[2] * pow ((xfac * a[4]),
					     2.0) -
			       nug[3] * pow ((xfac * a[4]),
					     3.0) -
			       nug[4] * pow ((xfac * a[4]), 4.0));
	  dyda[1] = 1.0;
	  dyda[2] = 0.0;
	  dyda[3] =
	    exp (-nug[1] * (xfac * a[4]) - nug[2] * pow ((xfac * a[4]), 2.0) -
		 nug[3] * pow ((xfac * a[4]),
			       3.0) - nug[4] * pow ((xfac * a[4]), 4.0));
	  dyda[4] =
	    a[3] * exp (-nug[1] * (xfac * a[4]) -
			nug[2] * pow ((xfac * a[4]),
				      2.0) - nug[3] * pow ((xfac * a[4]),
							   3.0) -
			nug[4] * pow ((xfac * a[4]),
				      4.0)) * (-nug[1] * xfac -
					       2 * nug[2] * a[4] * pow (xfac,
									2.0) -
					       3 * nug[3] * pow (a[4],
								 2.0) *
					       pow (xfac,
						    3.0) -
					       4 * nug[4] * pow (a[4],
								 3.0) *
					       pow (xfac, 4.0));

	}
      else
	{

	  *y = a[1] + a[3] * exp (-xfac * a[4]);
	  dyda[1] = 1.0;
	  dyda[2] = 0.0;
	  dyda[3] = exp (-xfac * a[4]);
	  dyda[4] = -xfac * a[3] * exp (-xfac * a[4]);
	}
      break;

    }
  yc = *y;
}

/*}}}*/

/*}}}*/
