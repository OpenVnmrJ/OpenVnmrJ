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
/****************************************************************
*  usrwt.c :							*
*								*
*  This module provides the user with the ability to write an   *
*  independent program, to be stored in $VNMRUSER/WTLIB/SRC,    *
*  which will be used by VNMR to calculate an array of weight-  *
*  ing data for any set of FID's, be it 1D or 2D.  This module  *
*  will be supplied in object form to be link loaded with the   *
*  user written module.                                         *
*								*
*  Changed to no use pipes for coordinating the execution       *
*  between parent and child.  Parent runs this program using	*
*  the `system' call which causes the parent to wait until	*
*  the child completes.						*
*								*
*  The number of `command line' arguments is now 7.  The older  *
*  version has 11.  If a new-style parent calls an old-style	*
*  user weight function, the weight funtion will abort because  *
*  the number of `command line' arguments is incorrect.		*
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAXWTCONST	10
#define MAXWTVAL	1.0e+6
#define MINWTVAL	-1.0e+6
#define MAXPATHL	128
#define WT_ERR		-4
#define DONE		-5

extern void wtcalc(float *wtpntr, int npoints, float delta_t);

float	wtconst[MAXWTCONST];	/* user-defined weighting constants */


int main(int argc, char *argv[])
{
   char		curexpdir[MAXPATHL],
		wtfname[MAXPATHL],
		wtfilename[MAXPATHL],
		parfilename[MAXPATHL];
   int		npoints,	/* number of points in weighting function */
		realftflag,
		res,
		i,
		wtfile;
   float	*wtpntr,	/* pointer to weighting vector */
		delta_t,	/* equivalent to 1/SW */
		sw;
   FILE		*parfiledes;

   if (argc != 7) {
      printf( "User weighting program called incorrectly\n" );
      exit(EXIT_FAILURE);
   }

   (void) strcpy(curexpdir, argv[1]);	  /* path to current experiment directory */
   (void) strcpy(wtfname, argv[2]);	  /* name of weighting data file */
   (void) strcpy(parfilename, argv[3]);/* name of weighting parameter file */

   sw = (float) (atoi(argv[4]));  /* spectral width * 1000 */
   sw /= 1000;
   delta_t = 1/sw;		  /* acquisition dwell time */
   npoints = atoi(argv[5]);	  /* number of weighting function points */
   realftflag = atoi(argv[6]);	  /* real FT flag */
   if (realftflag)
      delta_t /= 2.0;

   (void) strcpy(wtfilename, curexpdir);
   (void) strcat(wtfilename, "/");
   (void) strcat(wtfilename, wtfname);
   (void) strcat(wtfilename, ".wtf");

#ifdef DEBUG
   (void) printf("Current experiment directory = %s\n", curexpdir);
   (void) printf("WT filename = %s\n", wtfilename);
   (void) printf("PAR filename = %s\n", parfilename);
   (void) printf("SW = %f\n", sw);
   (void) printf("Dwell time = %f\n", delta_t);
   (void) printf("WT points = %d\n", npoints);
   (void) printf("Real FT flag = %d\n", realftflag);
#endif

/******************************************
*  Read in weighting function constants.  *
******************************************/

   parfiledes = fopen(parfilename, "r");
   if (parfiledes == 0)
   {
      for (i = 0; i < MAXWTCONST;)
         wtconst[i++] = 1.0;
   }
   else
   {
      i = 0;
      while (i < MAXWTCONST)
      {
         if (fscanf(parfiledes, "%f", &wtconst[i++]) == EOF)
            break;
         if (wtconst[i - 1] > MAXWTVAL)
         {
            (void) printf("WT parameter %d has been set to the ",i);
            (void) printf("maximum value %f\n", MAXWTVAL);
            wtconst[i - 1] = MAXWTVAL;
         }
         else if (wtconst[i - 1] < MINWTVAL)
         { 
            (void) printf("WT parameter %d has been set to the ", i);
            (void) printf("minimum value %f\n", MINWTVAL); 
            wtconst[i - 1] = MINWTVAL;
         }
      }
      (void) fclose(parfiledes);	/* close weighting constant file */

#ifdef DEBUG
      (void) printf("Max WT parameter index = %d\n", i);
#endif

      i -= 1;
      for ( ; i < MAXWTCONST; i++)
         wtconst[i] = 1.0;
   }

#ifdef DEBUG
   for (i = 0; i < MAXWTCONST; i++)
      (void) printf("WT parameter %d = %f\n", i+1, wtconst[i]);
#endif


/************************************************
*  Set up pointers to weighting function data.  *
************************************************/

   wtpntr = (float *) (malloc((npoints + 1) * sizeof(float)));
   if (wtpntr == NULL)
   {
      (void) printf("\n");
      (void) printf("Insufficient memory for user-calculated weighting data\n");
      exit(EXIT_FAILURE);
   }

/*****************************************************
*  Call user-written routine to calculate weighting  *
*  function values.                                  *
*****************************************************/

   wtcalc(wtpntr, npoints, delta_t);

/*********************************************
*  Open weighting function file in current   *
*  experiment directory.  Write the weight-  *
*  ing function data in binary format to     *
*  this file.                                *
*********************************************/

   wtfile = open(wtfilename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
   res = write(wtfile, wtpntr, sizeof(float)*npoints);
   if (res < sizeof(float)*npoints)
   {
      (void) printf("\n");
      (void) printf("Error in writing user-calculated weighting data\n");

      free(wtpntr);
      (void) close(wtfile);
      exit(EXIT_FAILURE);
   }

#ifdef DEBUG
   (void) printf("Number of bytes written = %d\n", res);
#endif

/************************************************************
*  Release weighting function memory, close the weighting   *
*  function data file, and close the write end of the sec-  *
*  ond pipe, signaling VNMR that this process has finished. *
************************************************************/

   free(wtpntr);
   (void) close(wtfile);
   exit(EXIT_SUCCESS);
}
