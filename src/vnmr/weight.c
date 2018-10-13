/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------
|						|
|		     weight.c			|
|						|
|  This module contains all the functions for	|
|  parameterizing, calculating, and applying	|
|  weighting functions for time-domain data.	|
|						|
|						|
+----------------------------------------------*/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>

/*  VMS does not define M_PI in its math.h file */

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif 
#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923
#endif 

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "data.h"
#include "disp.h"
#include "group.h"
#include "vnmrsys.h"
#include "ftpar.h"
#include "pvars.h"
#include "sky.h"
#include "wjunk.h"

#ifdef UNIX
#include <fcntl.h>
#else 
#include  file
#include  "unix_io.h"		/* Use our version of open, read, etc.  */
#endif 

#define COMPLETE	0
#define ERROR		1
#define FALSE		0
#define TRUE		1
#define MAXSTR		256

#define MAX_WTVAL	70.0
#define MIN_WTVAL	-70.0
#define MIN_WTFUNC	3.9754497e-31
#define MAX_WTFUNC	2.5154387e+30


/*---------------------------------------
|					|
|	get_weightpar_names()/2		|
|					|
|   This function initializes the ap-	|
|   propriate weighting parameters names|
|   for the requested dimension         |
|					|
+--------------------------------------*/
void get_weightpar_names(int dim, struct wtparams *wtpar)
{
  if (dim == FN2_DIM)
  {
     strcpy(wtpar->lbname, "lb2");
     strcpy(wtpar->sbname, "sb2");
     strcpy(wtpar->sbsname, "sbs2");
     strcpy(wtpar->saname, "sa2");
     strcpy(wtpar->sasname, "sas2");
     strcpy(wtpar->gfname, "gf2");
     strcpy(wtpar->gfsname, "gfs2");
     strcpy(wtpar->awcname, "awc2");
  }
  else if (dim == FN1_DIM)
  {
     strcpy(wtpar->lbname, "lb1");
     strcpy(wtpar->sbname, "sb1");
     strcpy(wtpar->sbsname, "sbs1");
     strcpy(wtpar->saname, "sa1");
     strcpy(wtpar->sasname, "sas1");
     strcpy(wtpar->gfname, "gf1");
     strcpy(wtpar->gfsname, "gfs1");
     strcpy(wtpar->awcname, "awc1");
  }
  else         /* FN0_DIM  */
  {
     strcpy(wtpar->lbname, "lb");
     strcpy(wtpar->sbname, "sb");
     strcpy(wtpar->sbsname, "sbs");
     strcpy(wtpar->saname, "sa");
     strcpy(wtpar->sasname, "sas");
     strcpy(wtpar->gfname, "gf");
     strcpy(wtpar->gfsname, "gfs");
     strcpy(wtpar->awcname, "awc");
  }
}

/*---------------------------------------
|					|
|	get_weightpar_vals()/2		|
|					|
|   This function initializes the ap-	|
|   propriate weighting parameters      |
|   from the requested parameter tree.  |
|   The wtpar names are assumed to be   |
|   set by get_weightpar_names()        |
|					|
+--------------------------------------*/
int get_weightpar_vals(int tree, struct wtparams *wtpar)
{
  if (P_getparinfo( tree, wtpar->lbname, &(wtpar->lb), &(wtpar->lb_active) ))
     return(ERROR);
  if (P_getparinfo( tree, wtpar->sbname, &(wtpar->sb), &(wtpar->sb_active) ))
     return(ERROR);
  if (P_getparinfo( tree, wtpar->gfname, &(wtpar->gf), &(wtpar->gf_active) ))
     return(ERROR);
  if (P_getparinfo( tree, wtpar->saname, &(wtpar->sa), &(wtpar->sa_active) ))
     wtpar->sa_active = ACT_OFF;
  if (P_getparinfo( tree, wtpar->sbsname, &(wtpar->sbs), &(wtpar->sbs_active) ))
     return(ERROR);
  if (P_getparinfo( tree, wtpar->gfsname, &(wtpar->gfs), &(wtpar->gfs_active) ))
     return(ERROR);
  if (P_getparinfo( tree, wtpar->sasname, &(wtpar->sas), &(wtpar->sas_active) ))
     wtpar->sas_active = ACT_OFF;
  if (P_getparinfo( tree, wtpar->awcname, &(wtpar->awc), &(wtpar->awc_active) ))
     return(ERROR);
  return(COMPLETE);
}

/*---------------------------------------
|					|
|	set_weightpar_vals()/2		|
|					|
|   This function initializes the    	|
|   requested parameter tree from the   |
|   supplied weighting parameters       |
|					|
+--------------------------------------*/
void set_weightpar_vals(int tree, struct wtparams *wtpar)
{
  P_setreal(tree, wtpar->lbname, wtpar->lb, 1);
  P_setactive(tree, wtpar->lbname, (wtpar->lb_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->sbname, wtpar->sb, 1);
  P_setactive(tree, wtpar->sbname, (wtpar->sb_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->gfname, wtpar->gf, 1);
  P_setactive(tree, wtpar->gfname, (wtpar->gf_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->saname, wtpar->sa, 1);
  P_setactive(tree, wtpar->saname, (wtpar->sa_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->sbsname, wtpar->sbs, 1);
  P_setactive(tree, wtpar->sbsname, (wtpar->sbs_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->gfsname, wtpar->gfs, 1);
  P_setactive(tree, wtpar->gfsname, (wtpar->gfs_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->sasname, wtpar->sas, 1);
  P_setactive(tree, wtpar->sasname, (wtpar->sas_active) ? ACT_ON : ACT_OFF);

  P_setreal(tree, wtpar->awcname, wtpar->awc, 1);
  P_setactive(tree, wtpar->awcname, (wtpar->awc_active) ? ACT_ON : ACT_OFF);
}

/*---------------------------------------
|					|
|	      init_wt1()/2		|
|					|
|   This function initializes the ap-	|
|   propriate spectral and weighting	|
|   parameters prior to data process-	|
|   ing.				|
|					|
+--------------------------------------*/
int init_wt1(struct wtparams *wtpar, int fdimname)
{
  char	swname[6];

  if (fdimname & S_NI2)
  {
     strcpy(swname, "sw2");
     get_weightpar_names(FN2_DIM, wtpar);
  }
  else if (fdimname & (S_NF|S_NI))
  {
     strcpy(swname, "sw1");
     get_weightpar_names(FN1_DIM, wtpar);
  }
  else
  {
     strcpy(swname, "sw");
     get_weightpar_names(FN0_DIM, wtpar);
  }
  if (P_getparinfo(PROCESSED, swname, &(wtpar->sw), NULL))
     return(ERROR);
  return(get_weightpar_vals(CURRENT, wtpar));
}


/*---------------------------------------
|                                       |
|             init_wt2()/7              |
|                                       |
+--------------------------------------*/
int init_wt2(struct wtparams *wtpar, register float  *wtfunc,
             register int n, int rftflag, int fdimname, double fpmult, int rdwtflag)
{
  char                  wtfname[MAXSTR],
                        wtfilename[MAXPATHL],
                        parfilename[MAXPATHL],
			run_usrwt[MAXSTR];
  int                   maxpoint,
                        sinesquared,
                        wtfile,
			wtfileflag,
                        sa_first, sa_last,
                        res;
  float                 sind,
                        cosd,
                        ph,
			phi,
                        awc;
  register int          i;
  register float        lbconst,
			lbvar,
			gfconst,
			gfvar,
			f,
                        max,
                        sinc,
                        cosc,
                        *fpnt,
			lastwtval,
                        sbfunc;
  FILE                  *fopen(),
                        *fileres;


  if (wtpar->sw == 0.0)
  {
     Werrprintf("Error:  sw is zero.");
     return(ERROR);
  }
 
/**************************************************
*  Section for user-defined weighting functions.  *
**************************************************/
 
  strcpy(wtfname, "");
  if (fdimname & S_NI2)
  {
     res = P_getstring(CURRENT, "wtfile2", wtfname, 1, MAXPATHL-1);
  }
  else if (fdimname & (S_NF|S_NI))
  {
     res = P_getstring(CURRENT, "wtfile1", wtfname, 1, MAXPATHL-1);
  }
  else
  {
     res = P_getstring(CURRENT, "wtfile", wtfname, 1, MAXPATHL-1);
  }

  wtfileflag = ((res == 0) && (strcmp(wtfname, "") != 0));

/***********************************************
*  Initialize weighting function data.  It is  *
*  necessary to do this even if no weighting   *
*  is active because of WTI.                   *
***********************************************/

  fpnt = wtfunc;
  for (i = 0; i < n; i++)
     *fpnt++ = 1.0;

/************************
*  Set weighting flag.  *
************************/

  if ( !(wtpar->lb_active || wtpar->sb_active || wtpar->gf_active || wtpar->sa_active ||
   	wtfileflag) )
  {
     wtpar->wtflag = FALSE;
     return(COMPLETE);
  }

/***************************************************************
*  Initiate user-defined weighting routines.  The executable   *
*  "wtfile" is found in the user's "wtlib" directory and the   *
*  corresponding parameter set, in the current experiment      *
*  directory with the extension ".wtp".  The file containing   *
*  the user-written, formatted weighting data is found in the  *
*  current experiment directory.                               *
***************************************************************/
 
  fpnt = wtfunc;
  if (wtfileflag)
  {
     strcpy(parfilename, curexpdir);
#ifdef UNIX
     strcat(parfilename, "/");
#endif 
     strcat(parfilename, wtfname);
     strcat(parfilename, ".wtp");

     strcpy(wtfilename, userdir);
#ifdef UNIX
     strcat(wtfilename, "/wtlib/");
#else 
     vms_fname_cat(wtfilename, "[.wtlib]");
#endif 
     strcat(wtfilename, wtfname);
#ifndef UNIX				/*  Presumably VMS  */
     strcat(wtfilename, ".exe" );
#endif 

     fileres = fopen(wtfilename, "r");

/*****************************************************************
* If there is no user-weighting program in the user's wtlib,     *
* look for a file containing the user-weighting function written *
* out explicitly in the current experiment directory             *
*****************************************************************/

     if (fileres == 0)
     {
        strcpy(wtfilename, curexpdir);
#ifdef UNIX
        strcat(wtfilename, "/");
#endif 
        strcat(wtfilename, wtfname);
 
        fileres = fopen(wtfilename, "r");
        if (fileres == 0)
        {
           Werrprintf("Unable to find requested user-weighting files");
           return(ERROR);
        }
 
/****************************************************
*  Read in user-defined, formatted weighting data.  *
****************************************************/
 
        i = 0;
        while (fscanf(fileres, "%f", fpnt) != EOF)
        {
           fpnt++;
           if (++i == n)
              break;
        }
 
        lastwtval = *(fpnt - 1);
        for ( ; i < n; i++)
           *fpnt++ = lastwtval;
        fclose(fileres);
     }
     else
     {
        fclose(fileres);

        if (!rdwtflag)
        {
#ifdef UNIX

/*  The UNIX command includes the complete path.  */

	    sprintf( &run_usrwt[ 0 ],
		"%s %s %s %s %14d %7d %d",
		 wtfilename, curexpdir, wtfname, parfilename,
		 (int) (wtpar->sw*1000.0), n, rftflag);

#else 

/*         
 *  For VMS, necessary to define a DCL symbol
 *  to reference the user's program.
 */
            {
                char    wt_sym_value[ MAXPATHL ];
                int     symbol_descr[ 2 ], value_descr[ 2 ], one;
 
                wt_sym_value[ 0 ] = '$';                          
                wt_sym_value[ 1 ] = '\0';
                strcat( &wt_sym_value[ 0 ], wtfilename );
                symbol_descr[ 0 ] = strlen( wtfname );
                symbol_descr[ 1 ] = (int) wtfname;
                value_descr[ 0 ]  = strlen( &wt_sym_value[ 0 ] );
                value_descr[ 1 ] =  (int) &wt_sym_value[ 0 ];
                one = 1;        /* because LIB$GETSYMBOL wants a reference */

                LIB$SET_SYMBOL( &symbol_descr[ 0 ], &value_descr[ 0 ], &one );

/*
 *  The VMS command only has the command name and arguments,
 *  and not the complete path.
 */
		sprintf( &run_usrwt[ 0 ],
		    "%s %s %s %s %14d %7d %d",
		     wtfname, curexpdir, wtfname, parfilename,
		    (int) (wtpar->sw*1000.0), n, rftflag);
            }
#endif 

/**********************************************************
*  Now execute the user's program to produce the desired  *
*  weighting function.  The weighting function will be    *
*  written out to disk by the user's program.             *
**********************************************************/
 
	    system( &run_usrwt[ 0 ] );
        }  

/********************************************************                   
*  Read user-calculated weighting data in from the ap-  *
*  propriate file in the current experiment directory.  *
********************************************************/
 
        strcpy(wtfilename, curexpdir);
#ifdef UNIX
        strcat(wtfilename, "/");
#endif 
        strcat(wtfilename, wtfname);
        strcat(wtfilename, ".wtf");

        wtfile = open(wtfilename, O_RDONLY, 0666);
        if (wtfile == 0)
        {
           Werrprintf("Error opening user-calculated weighting file");
           return(ERROR);
        }
        if ((res = read(wtfile, fpnt, sizeof(float)*n)) < 0)
        {
           Werrprintf("Error in reading user-calculated weighting data");
           close(wtfile);
           return(ERROR);
        }

        close(wtfile);
     }
  }

/************************************************
*  Section for exponential weighting functions  *
************************************************/
 
  lbconst = 0.0;
  if (wtpar->lb_active)
  {
     if (wtpar->lb < -1e6)
        wtpar->lb = -1e6;
     if (wtpar->lb > 1e6)
        wtpar->lb = 1e6;
     lbconst = wtpar->lb/(0.31831*wtpar->sw);
     if (rftflag)
        lbconst /= 2.0;
  }
  if (wtpar->sa_active)
  {
     sa_first = (wtpar->sas_active) ? wtpar->sas : 0;
     sa_last = wtpar->sa + sa_first;
  }
  else
  {
     sa_first = 0;
     sa_last = n;
  }
 
/*****************************************
*  Section for sine weighting functions  *
*****************************************/
 
  sinesquared = 0;
  ph = phi = 0.0;
  cosc = sinc = cosd = sind = 0.0;
  if (wtpar->sb_active)
  {
     sinesquared = (wtpar->sb < 0.0);
     if (sinesquared)
        wtpar->sb *= (-1);
 
     if (wtpar->sb > 1000.0)
     {
        wtpar->sb = 1000.0;
     }
     else if (wtpar->sb < (1.0/wtpar->sw))
     {
        wtpar->sb = 1.0/wtpar->sw;
     }
 
     ph = M_PI_2/(wtpar->sw*wtpar->sb);
     if (rftflag)
        ph /= 2.0;

     phi = ph;
     sind = sin((double) (ph));
     cosd = cos((double) (ph));
     if (wtpar->sbs_active)
     {
        if (wtpar->sbs < -1000.0)
        {
           wtpar->sbs = -1000.0;
        }
        else if (wtpar->sbs > 1000.0)
        {
           wtpar->sbs = 1000.0;
        }
 
        ph = -M_PI_2*wtpar->sbs/wtpar->sb;
	if ((ph > 0.0) && (ph < M_PI))
	{
          sinc = sin((double) (ph));
          cosc = cos((double) (ph));
	}
	else
	{
	  sinc = 0.0;
	  cosc = 1.0;
	}
     }
     else
     {
	ph   = 0.0;
        sinc = 0.0;
        cosc = 1;
     }
 
     sbfunc = sinc;
     if (sinesquared)
        wtpar->sb *= (-1);
  }
  else
  {
     sbfunc = 1.0;
  }
 
/*********************************************
*  Section for gaussian weighting functions  *
*********************************************/
 
  maxpoint = 0;
  gfconst = 0.0;
  if (wtpar->gf_active)
  {
     if (wtpar->gf < -1000.0)
     {
        wtpar->gf = -1000;
     }
     else if (wtpar->gf > 1000.0)
     {
        wtpar->gf = 1000.0;
     }
     if (wtpar->gf == 0.0)
     {
         wtpar->gf = 0.1;
     }

     gfconst = 1.0/( wtpar->sw*wtpar->gf);
     if (rftflag)
        gfconst /= 2.0;

     if (wtpar->gfs_active)
     {
        if (wtpar->gfs < -1000.0)
        {
           wtpar->gfs = -1000.0;
        }
        else if (wtpar->gfs > 1000.0)
        {
           wtpar->gfs = 1000.0;
        }
 
        maxpoint = wtpar->sw*wtpar->gfs;
        if (rftflag)
           maxpoint *= 2;

        if (maxpoint < 0)
        {
           maxpoint = 0;
        }
        else if (maxpoint >= n)
        {
           maxpoint = n-1;
        }
     }
  }
 
/********************************************
*  Section for additive weighting constant  *
********************************************/
 
  if (wtpar->awc_active)
  {
     awc = wtpar->awc;
  }
  else
  {
     awc = 0.0;
  }

/************************************************
*  Create weighting function in weight buffer.  *
************************************************/
 
  max = 0.0;
  fpnt = wtfunc;
  for (i = 0; i < n; i++)
  {
     f = (*fpnt) * sbfunc;
     if (wtpar->lb_active)
     {
        lbvar = i*lbconst;
        if (lbvar > MAX_WTVAL)
        {
           f *= MIN_WTFUNC;
        }
        else if (lbvar < MIN_WTVAL)
        {
           f *= MAX_WTFUNC;
        }
        else
        {
           f *= (float) (exp( -lbvar ));
        }
     }

     f += awc;
     if (wtpar->gf_active)
     {
        gfvar = (i - maxpoint)*gfconst;
        gfvar *= gfvar;
        if (gfvar > MAX_WTVAL)
        {
           f *= MIN_WTFUNC;
        }
        else
        {
           f *= (float) (exp( -gfvar ));
        }
     }
     if (wtpar->sa_active)
     {
        if (i < sa_first)
           f = 0.0;
        if (i > sa_last)
           f = 0.0;
     }

     if (f < 0.0)
     {
        f = 0.0;
     }
     else if (f > max)
     {
        max = f;
     }
 
     *fpnt++ = f;
     if (wtpar->sb_active)
     {
	if ((ph > 0.0) && (ph < M_PI))
	{
	   /* cordic rotation */
           sbfunc = sinc*cosd + cosc*sind;
           cosc   = cosc*cosd - sinc*sind;
           sinc = sbfunc;
           if (sinesquared)
	     sbfunc *= sbfunc;
	}
	else
	{
	    sbfunc = 0.0;
 	}
	ph += phi;
     }
  }
 
/*************************************************
*  Scale weighting function so that the maximum  *
*  value therein is 1.0, neglecting the effect   *
*  "fpmult".                                     *
*************************************************/
 
  fpnt = wtfunc;                  /* Reset "fpnt" pointer */
  if ((max > 0.0) && (max != 1.0))
  {
     f = 1/max;
     for (i = 0; i < n; i++)
     {
        *fpnt *= f;
        fpnt++;
     }
  }

  *wtfunc *= (float) fpmult;          /* Scales first point in weighting function */

  return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            weightfid()/5              |
|                                       |
+--------------------------------------*/
void weightfid(float *wtfunc, float *outp, int n, int rftflag, int dtype)
{
  int	i;
 
/*********************************
*  Weight the time domain data.  *
*********************************/

  if (rftflag)					/* REAL data */
  {
     dtype /= 2;
     for (i = 0; i < dtype; i++)
        vvvrmult(wtfunc, 1, outp + i, dtype, outp + i, dtype, 2*n);
  }
  else						/* COMPLEX data */
  {
     for (i = 0; i < dtype; i++)
        vvvrmult(wtfunc, 1, outp + i, dtype, outp + i, dtype, n);
  }
}
