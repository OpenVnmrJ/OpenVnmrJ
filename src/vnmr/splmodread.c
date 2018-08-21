/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************************************************
*                                                                                 *
*  splmodread    takes the output data file from SPLMOD and makes an input file   *
*		 for ddif        						  *
*                                                                                 *
**********************************************************************************/



/*      9iv08 Change DAC_to_G to gcal_					*/
/*      9iv08 Change nugcal to nugcal_					*/
/*      9iv08 Change Dosy directory to dosy				*/
/*      15v08 Change to new format of dosy_in file                      */
/*	3iv09 Remove spurious printf statements				*/
/*	3iv09 Neaten up residfunc					*/
/*	18v09 GM neaten up error reporting and increase debug output	*/
/*	18v09 GM add tabs to diffusion_display.inp output		*/
/*	18v09 GM if 0ERROR ANALYZ 5, break out of results loop		*/
/*	19v09 GM Reject solution if more than tenfold attenuation	*/
/*		 between first and second point, i.e. if D too high	*/
/*	11xii09 GM Try complete rewrite of file read in	using keywords  */
/*		   to recognise data needed				*/
/*		   Remove variables no longer needed, e.g. maxcomp	*/
/*		   Test std dev of alpha in case percentage error	*/
/*		   is running into std dev and confusing parsing	*/

/*Includes and defines*/
/*{{{*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>


#include "vnmrsys.h"
#include "data.h"
#include "pvars.h"
#include "disp.h"
#include "init2d.h"
#include "graphics.h"
#include "group.h"
#include "wjunk.h"


#define ERROR			1
#define TRUE			1
#define FALSE			0
#define MAXLENGTH		512
#define MAXPEAKS		512
#define MAXPOINTS		600
#define MAX_ERROR   	30.0	/* Maximum error permitted on the diffusion value, in per cent */
#define MAXFITCOEFFS		4
#define DSCALE			2e-9
#define NR_END 			1
#define FREE_ARG 		char*
/* #define DEBUG_SPLMOD 	1 */ /*	  Comment out not to compile the debugging code */



/*Declarations*/
static double residfunc (double x, double lamda[], double alpha[],
			   int ncomp, int nugflag, double dosyconstant,
			   double nug[]);




int splmodread (int argc, char *argv[], int retc, char *retv[])
{
  /*Declarations */
  int i, j, k, l,		/*standard iterators */
    nPeaks,			/*number of peaks */
    dosyversion,		/*version number */
    badinc,			/*number of pruned levels */
    nugflag,			/*whether to use pure exponentials or corrected */
    badflag,			/*for removing data from pruned amplitudes */
    bd[MAXPOINTS],		/*array for amplitude levels deleted from analysis (prune) */
    bestsol,			/*best solution chosen by splmod */
    currsol,			/*solution chosen by us */
    currflag, /**/ inttmp,	/*temporary variable */
    nugsize,			/*length of array nugcal_ */
    checkpeakno,		/*check of peak number */
    solno,			/*solution number */
    nofit,			/*flag for failure to report a fit */
    nPoints;			/*number of points per peak (amplitude levels) */

  char rubbish[MAXLENGTH],	/* string for various purposes */
   *pptemp,			/* temprary pointer to pointer */
    js1[MAXLENGTH],		/* strings for various purposes */
   
    js2[MAXLENGTH],
    js3[MAXLENGTH],
    js4[MAXLENGTH],
    js5[MAXLENGTH],
    js6[MAXLENGTH],
    js7[MAXLENGTH],
    js8[MAXLENGTH],
    js9[MAXLENGTH], js10[MAXLENGTH], js11[MAXLENGTH], jstr[MAXLENGTH];

  double dosyconstant,		/*calculated from the pulse sequence */
    gcal_,			/*conversion factor from DAC points to Gauss */
    gwidth,			/*correction factor for Gaussian linewidth */
    doubletmp,			/*temporary variable */
    gradAmp[MAXPOINTS],		/*gradient amplitudes */
    alphaTmp[10],		/*for amplitudes */
    lamdaTmp[10],		/*for D-values */
    frq[MAXPEAKS],		/*frequencies for each peak */
    ampl[MAXPOINTS],		/*amplitudes for each peak */
    nug[MAXFITCOEFFS + 1];	/*coefficients for NUG correction */

  FILE *infile,			/*for dosy_splmod.out file */
   *dosyfile,			/*for dosy_in file */
   *fit_residuals,		/*for general_dosy_stats */
   *ddiffile,			/*for diffusion_display.inp */
   *D_spectrum;			/*for diffusion_spectrum */

  typedef struct FitValues_struct
  {
    double alpha[10], std_alpha[10], lamda[10], std_lamda[10];
  } FitValues_struct;
  FitValues_struct FitValues[10];


#ifdef DEBUG_SPLMOD
  FILE *debug;
#endif


#ifdef DEBUG_SPLMOD
  //strcpy (rubbish, userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_splmodread");
  debug = fopen (rubbish, "w");	/* file for debugging information */
  fprintf (debug, "Start of splmodread\n");
#endif
#ifdef DEBUG_SPLMOD
  fprintf (debug, "Before reading in of data\n");
#endif

/* GM 4iii09 define scaling factor for width of Gaussian line in diffusion_spectrum */
  gwidth = 2.0 * sqrt (2.0 * (log (2.0)));

  /*Read data from dosy_in */



  //strcpy (rubbish, userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/dosy_in");
  dosyfile = fopen (rubbish, "r");
  if (!dosyfile)
    {
      Werrprintf ("splmodread: could not open file %s", rubbish);
      return (ERROR);
    }
  strcpy (jstr, curexpdir);
  strcat (jstr, "/dosy/dosy_in");

  if (fscanf (dosyfile, "DOSY version %d\n", &dosyversion) == EOF)
    {
      Werrprintf ("splmodprepare: reached end of file %s", jstr);
      fclose (dosyfile);
      return (ERROR);
    }
  if (fscanf
      (dosyfile, "%d spectra will be deleted from the analysis :\n",
       &badinc) == EOF)
    {
      Werrprintf ("splmodprepare: reached end of file %s", jstr);
      fclose (dosyfile);
      return (ERROR);
    }
  for (i = 0; i < badinc; i++)
    {
      if (fscanf (dosyfile, "%d\n", &bd[i]) == EOF)
	{
	  Werrprintf ("splmodread: reached end of file %s", jstr);
	  fclose (dosyfile);
	  return (ERROR);
	}
    }

  if ((fscanf (dosyfile, "Analysis on %d peaks\n", &nPeaks) == EOF)	/* read in calibration parameters */
      || (fscanf (dosyfile, "%d points per peaks\n", &nPoints) == EOF)
      || (fscanf (dosyfile, "%d parameters fit\n", &inttmp) == EOF)
      || (fscanf (dosyfile, "dosyconstant = %lf\n", &dosyconstant) == EOF)
      || (fscanf (dosyfile, "gradient calibration flag :  %d\n", &inttmp) ==
	  EOF)
      || (fscanf (dosyfile, "non-linear gradient flag :  %d\n", &nugflag) ==
	  EOF))
    {
      Werrprintf ("splmodread: reached end of file %s", jstr);
      fclose (dosyfile);
      return (ERROR);
    }

  for (i = 0, j = 0; i < (nPoints + badinc); i++)
    {				/*scan through all the input values, discarding the unwanted ones */
      badflag = 0;
      for (l = 0; l < badinc; l++)
	if (i == bd[l] - 1)
	  badflag = TRUE;
      strcpy (jstr, curexpdir);
      strcat (jstr, "/dosy/dosy_in");	/* read in gradient array */
      if (badflag)
	{
	  if (fscanf (dosyfile, "%s", rubbish) == EOF)
	    {
	      Werrprintf
		("splmodread: reached end of file %s on increment %d", jstr,
		 i);
	      fclose (dosyfile);
	      return (ERROR);
	    }
	}
      else
	{
	  if (fscanf (dosyfile, "%lf", &gradAmp[j]) == EOF)
	    {
	      Werrprintf
		("splmodread: reached end of file %s on increment %d", jstr,
		 i);
	      fclose (dosyfile);
	      return (ERROR);
	    }
	  j++;
	}
    }

  if (fscanf (dosyfile, "%s", rubbish) == EOF)
    {
      Werrprintf ("splmodread: reached end of file %s", jstr);
      fclose (dosyfile);
      return (ERROR);
    }
  while (strcmp (rubbish, "(mm)") != 0)
    {
      if (fscanf (dosyfile, "%s", rubbish) == EOF)
	{
	  Werrprintf ("splmodread: reached end of file %s", jstr);
	  fclose (dosyfile);
	  return (ERROR);
	}
    }


/*read in NUG if nugflag*/

  for (i = 0; i <= MAXFITCOEFFS; i++)
    nug[i] = 0.0;
  nugflag = 0;
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
  if (nugflag)
    {
      P_getsize(CURRENT, "nugcal_", &nugsize);
      if (nugsize > 5)
	nugsize = 5;
      if (nugsize < 2)
	{
	  Wscrprintf ("splmodread: nugcal_ must contain at least 2 values");
	  Werrprintf ("splmodread: nugcal_ must contain at least 2 values");
	  fclose (dosyfile);
	  return (ERROR);
	}
      if (P_getreal (CURRENT, "nugcal_", &gcal_, 1))
	{
	  Werrprintf ("splmodread: cannot read gcal_ from nugcal_");
	  Wscrprintf ("splmodread: cannot read gcal_ from nugcal_");
	  fclose (dosyfile);
	  return (ERROR);
	}
      for (i = 1; i < nugsize; i++)
	{
	  if (P_getreal (CURRENT, "nugcal_", &nug[i], i + 1))
	    {
	      Werrprintf
		("splmodread: cannot read coefficients from nugcal_");
	      Wscrprintf
		("splmodread: cannot read coefficients from nugcal_");
	      fclose (dosyfile);
	      return (ERROR);
	    }
	}
#ifdef DEBUG_SPLMOD
      fprintf (debug, "nugsize:  %d\n", nugsize);
      fprintf (debug, "gcal_ %e :\n", gcal_);
      for (i = 0; i < MAXFITCOEFFS; i++)
	fprintf (debug, "nug %e :\n", nug[i + 1]);
#endif
    }



  /*open the fit_residuals */
  //strcpy (rubbish, userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/general_dosy_stats");
  fit_residuals = fopen (rubbish, "w");
  if (!fit_residuals)
    {
      Werrprintf ("splmodread: could not open file %s", rubbish);
      fclose (dosyfile);
      return (ERROR);
    }

  fprintf (fit_residuals, "\t\t2D data set\n");
  fprintf (fit_residuals,
	   "\t\tsee dosy_splmod.out for more statistical information.\n");
  if (nugflag)
    {
      fprintf (fit_residuals,
	       "Fitted for NUG corrected decays using Splmod\n\n\n\n");
    }
  else
    {
      fprintf (fit_residuals,
	       "Fitted for pure exponentials using Splmod\n\n\n\n");
    }

  /*Read data from dosy_splmod.out */
/*{{{*/
  //strcpy (rubbish, userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/dosy_splmod.out");
  infile = fopen (rubbish, "r");
  if (!infile)
    {
      Werrprintf ("splmodread: could not open file %s", rubbish);
      fclose (dosyfile);
      fclose (fit_residuals);
      return (ERROR);
    }

  //strcpy (rubbish, userdir);
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/diffusion_display.inp");
  ddiffile = fopen (rubbish, "w");
  if (!ddiffile)
    {
      Werrprintf ("splmodread: could not open file %s", rubbish);
      fclose (dosyfile);
      fclose (fit_residuals);
      fclose (infile);
      return (ERROR);
    }
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/diffusion_spectrum");
  D_spectrum = fopen (rubbish, "w");
  if (!D_spectrum)
    {
      Werrprintf ("splmodread: could not open file %s", rubbish);
      fclose (dosyfile);
      fclose (fit_residuals);
      fclose (infile);
      fclose (ddiffile);
      return (ERROR);
    }

  strcpy (jstr, curexpdir);
  strcat (jstr, "/dosy/dosy_splmod.out");

#ifdef DEBUG_SPLMOD
  fclose (debug);
#endif

  k = 0;
  solno = 0;
  nofit = 0;

  while (fgets (jstr, 512, infile) != NULL)	/* read each line of dosy_splmod.out in turn, parsing those lines which carry useful information */
    {
      if (strstr (jstr, "1SPLMOD") != NULL)	/* read in peak frequency */
	{
	  k++;
	  sscanf (jstr, "%s%s%s%s%s%s%s%s%s%s %d %s %lf", js1, js2, js3,
		  js4, js5, js6, js7, js8, js9, js10, &checkpeakno, js11,
		  &frq[k]);
	  solno = 0;
	  j = 0;
	  nofit = 1;
	  for (i = 0; i < 10; i++)
	    {
	      for (j = 0; j < 10; j++)
		{
		  FitValues[i].alpha[j] = 0.0;
		  FitValues[i].std_alpha[j] = 0.0;
		  FitValues[i].lamda[j] = 0.0;
		  FitValues[i].std_lamda[j] = 0.0;
		}
	    }

	  for (i = 0, j = 0; i < (nPoints + badinc); i++)
	    {
	      badflag = 0;
	      for (l = 0; l < badinc; l++)
		if (i == bd[l] - 1)
		  badflag = 1;
	      fscanf (dosyfile, "%s", rubbish);
	      fscanf (dosyfile, "%s", rubbish);
	      if (badflag)
		fscanf (dosyfile, "%s", rubbish);
	      else
		fscanf (dosyfile, "%lf", &ampl[j++]);
	    }
#ifdef DEBUG_SPLMOD
	  strcpy (rubbish, curexpdir);
	  strcat (rubbish, "/dosy/debug_splmodread");
	  debug = fopen (rubbish, "a");	/* file for debugging information */
	  fprintf (debug,
		   "For peak %d , check peak no is %d and freq is %lf \n",
		   k, checkpeakno, frq[k]);
	  fclose (debug);
#endif

	}
      else if (strstr (jstr, " +- ") != NULL)
	{
	  if (strstr (jstr, "ALPHA") != NULL)	/* a fit has succeeded, get ready to record results */
	    {
	      solno++;
	      j = 0;
	    }
	  else
	    {
	      if (strstr (jstr, "***") != NULL)	/* the percentage error is too high to print, so fit must be dud */
		{
		  solno--;
		  j = 0;
#ifdef DEBUG_SPLMOD
		  strcpy (rubbish, curexpdir);
		  strcat (rubbish, "/dosy/debug_splmodread");
		  debug = fopen (rubbish, "a");	/* file for debugging information */
		  fprintf (debug,
			   "For peak %d , fit is too poor to read in\n", k);
		  fclose (debug);
#endif
		}
	      else
		{
		  j++;
		  sscanf (jstr, "%s%s%s%s%s%s%s", js1, js2, js3, js4, js5,
			  js6, js7);
		  FitValues[solno].alpha[j] = strtod (js1, &pptemp);
		  FitValues[solno].std_alpha[j] = strtod (js3, &pptemp);
		  FitValues[solno].lamda[j] = strtod (js5, &pptemp);
		  FitValues[solno].std_lamda[j] = strtod (js7, &pptemp);
		  if (fabs (FitValues[solno].std_alpha[j]) > 100.0)
		    {
		      solno--;
#ifdef DEBUG_SPLMOD
		      strcpy (rubbish, curexpdir);
		      strcat (rubbish, "/dosy/debug_splmodread");
		      debug = fopen (rubbish, "a");	/* file for debugging information */
		      fprintf (debug,
			       "For peak %d , std dev of alpha is too high\n",
			       k);
		      fclose (debug);
#endif
		    }
		  else
		    {
		      nofit = 0;
		    }
#ifdef DEBUG_SPLMOD
		  strcpy (rubbish, curexpdir);
		  strcat (rubbish, "/dosy/debug_splmodread");
		  debug = fopen (rubbish, "a");	/* file for debugging information */
		  fprintf (debug,
			   "For peak %d , results are %e\t\%e\t\%e\t\%e \n",
			   k, FitValues[solno].alpha[j],
			   FitValues[solno].std_alpha[j],
			   FitValues[solno].lamda[j],
			   FitValues[solno].std_lamda[j]);
		  fprintf (debug, "%s", jstr);
		  fclose (debug);
#endif
		}
	    }
	}
      else if ((strstr (jstr, "FOUND") != NULL) && (nofit == FALSE))	/* a fit has been found and the best solution is being reported */
	{
	  sscanf (jstr, "%s%s%s%s%s%s %d", js1, js2, js3, js4, js5,
		  js6, &bestsol);
	  /* now process the results for this peak */
#ifdef DEBUG_SPLMOD
	  strcpy (rubbish, curexpdir);
	  strcat (rubbish, "/dosy/debug_splmodread");
	  debug = fopen (rubbish, "a");	/* file for debugging information */
	  fprintf (debug, "For peak %d , best sol is %d  \n", k, bestsol);
	  fclose (debug);
#endif
	  fprintf (fit_residuals, "\nFrequency %lf\n", frq[k]);


	  fprintf (fit_residuals, "y = Sum (Alpha[i]*Exp[-Lamda*x])\n");
	  fprintf (fit_residuals,
		   "Splmod solutions (best solution is %d components):\n",
		   bestsol);

	  for (i = bestsol; i >= 1; i--)
	    {
	      fprintf (fit_residuals, "Splmod solution: [%d]\n", i);
	      for (j = 1; j <= i; j++)
		{
		  fprintf (fit_residuals,
			   "Alpha[%d]= %10.4lf (+/- %5.4lf)	Lamda[%d]= %10.4lf (+/- %5.4lf)(* 10-10 m2s-1)\n",
			   j, FitValues[i].alpha[j],
			   FitValues[i].std_alpha[j], j,
			   1e14 * FitValues[i].lamda[j] / dosyconstant,
			   1e14 * FitValues[i].std_lamda[j] / dosyconstant);
		}
	    }

	  /* Check the fitted values for the best solution */
	  currsol = bestsol;
	  currflag = 0;
	  for (i = bestsol; i >= 1; i--)
	    {
	      if (currflag)
		{
		  currsol = currsol - 1;
		  currflag = 0;
		}
	      for (j = 1; j <= i; j++)
		{
		  fprintf (fit_residuals, "i is %d j is %d .\n", i, j);
		  fprintf (fit_residuals, "currsol is %d\n", currsol);

		  if (((100.0 * FitValues[i].std_alpha[j] /
			FitValues[i].alpha[j]) > MAX_ERROR)
		      || (FitValues[i].alpha[j] == 0.0))
		    {
		      currflag = 1;
		      fprintf (fit_residuals,
			       "STDEV of Alpha %d from SPLMOD solution %d was too high; choosing one less component.\n",
			       j, currsol);
		      fprintf (fit_residuals,
			       "FitVal %d Alpha %d is %lf .\n", i, j,
			       FitValues[i].alpha[j]);
		    }
		  if (((100.0 * FitValues[i].std_lamda[j] /
			FitValues[i].lamda[j]) > MAX_ERROR)
		      || (FitValues[i].lamda[j] == 0.0))
		    {
		      currflag = 1;
		      fprintf (fit_residuals,
			       "STDEV of Lamda %d from SPLMOD solution %d was too high; choosing one less component.\n",
			       j, currsol);
		      fprintf (fit_residuals,
			       "FitVal %d Lamda %d is %lf .\n", i, j,
			       FitValues[i].lamda[j]);
		    }
		  /* Reject solution if calculated signal attenuates more than tenfold between 1st and 2nd gradient value */
		  if (FitValues[i].lamda[j] >
		      2.30259 / (gradAmp[2] * gradAmp[2] -
				 gradAmp[1] * gradAmp[1]))
		    {
		      currflag = 1;
		      fprintf (fit_residuals,
			       "Lamda %d from SPLMOD solution %d was too high to be statistically significant; choosing one less component.\n",
			       j, currsol);
		      fprintf (fit_residuals,
			       "FitVal %d Lamda %d is %lf .\n", i, j,
			       FitValues[i].lamda[j]);
		    }
		}
	    }

	  fprintf (fit_residuals,
		   "SPLMOD solution %d was deemed the most appropriate\n",
		   currsol);

	  /*print values for best solution to diffusion_display.inp */
#ifdef DEBUG_SPLMOD
	  strcpy (rubbish, curexpdir);
	  strcat (rubbish, "/dosy/debug_splmodread");
	  debug = fopen (rubbish, "a");	/* file for debugging information */
	  fprintf (debug,
		   "Printing out results for peak %d to diffusion_display.inp \n",
		   k);
	  fclose (debug);
#endif
	  for (i = 1; i <= currsol; i++)
	    {
	      fprintf (ddiffile,
		       "%10.3lf\t%10.4lf\t%10.4lf\t%10.6lf\t%10.4lf\t%10.4lf\t%10.6lf\n",
		       frq[k], FitValues[currsol].alpha[i],
		       1e14 * FitValues[currsol].lamda[i] / dosyconstant,
		       1e14 * FitValues[currsol].std_lamda[i] /
		       dosyconstant, 0.0, 0.0, 0.0);
#ifdef DEBUG_SPLMOD
	      strcpy (rubbish, curexpdir);
	      strcat (rubbish, "/dosy/debug_splmodread");
	      debug = fopen (rubbish, "a");	/* file for debugging information */
	      fprintf (debug,
		       "Peak %d :   %10.3lf\t%10.4lf\t%10.4lf\t%10.6lf\t%10.4lf\t%10.4lf\t%10.6lf\n",
		       k, frq[k], FitValues[currsol].alpha[i],
		       1e14 * FitValues[currsol].lamda[i] / dosyconstant,
		       1e14 * FitValues[currsol].std_lamda[i] /
		       dosyconstant, 0.0, 0.0, 0.0);
	      fclose (debug);
#endif
	    }
	  /*print values for best solution to diffusion_spectrum */
	  for (i = 1; i <= currsol; i++)
	    {
	      fprintf (D_spectrum,
		       "%10.3lf,\t%10.4lf,\t%10.4lf,\t%10.6lf\n",
		       1e14 * FitValues[currsol].lamda[i] / dosyconstant,
		       FitValues[currsol].alpha[i],
		       gwidth * 1e14 * FitValues[currsol].std_lamda[i] /
		       dosyconstant, 1.0);
	    }
	  /*Print to general_dosy_stats */

	  fprintf (fit_residuals,
		   "Gradient area         exp. height    calc. height          Diff\n");

	  for (i = 1; i < 10; i++)
	    {
	      alphaTmp[i] = FitValues[currsol].alpha[i];
	      lamdaTmp[i] = FitValues[currsol].lamda[i];
	    }
	  for (i = 0; i < nPoints; i++)
	    {
	      doubletmp =
		residfunc (gradAmp[i], lamdaTmp, alphaTmp, currsol,
			   nugflag, dosyconstant, nug);
	      fprintf (fit_residuals, "%10.6lf %15.6lf %15.6lf %15.6lf\n",
		       dosyconstant * 0.0001 * (1.0e-9) * gradAmp[i] *
		       gradAmp[i], ampl[i], doubletmp, (ampl[i] - doubletmp));
	    }
	}
      else
	{
	}			/* no keyword found, line can be ignored */
    }				/* End of while loop over input file lines, and hence over peaks */

#ifdef DEBUG_SPLMOD
  strcpy (rubbish, curexpdir);
  strcat (rubbish, "/dosy/debug_splmodread");
  debug = fopen (rubbish, "a");	/* file for debugging information */
  fprintf (debug, "End of splmodread\n");
  fclose (debug);
#endif

  fclose (dosyfile);
  fclose (fit_residuals);
  fclose (infile);
  fclose (ddiffile);
  fclose (D_spectrum);
  RETURN;
}

 /*FUNCTIONS*/ static double
residfunc (double x, double lamda[], double alpha[], int ncomp, int nugflag, double dosyconstant, double nug[])	/*{{{ */
{
  int i;
  double y, xfac;
  xfac = x * x;
  y = 0.0;

  if (nugflag)
    {
      for (i = 1; i <= ncomp; i++)
	{
	  y =
	    y + alpha[i] * exp (-nug[1] * (xfac * lamda[i]) -
				nug[2] * pow ((xfac * lamda[i]),
					      2.0) -
				nug[3] * pow ((xfac * lamda[i]),
					      3.0) -
				nug[4] * pow ((xfac * lamda[i]), 4.0));
	}
    }
  else
    {
      for (i = 1; i <= ncomp; i++)
	{
	  y = y + alpha[i] * exp (-xfac * lamda[i]);
	}
    }
  return (y);

}				/*}}} */
