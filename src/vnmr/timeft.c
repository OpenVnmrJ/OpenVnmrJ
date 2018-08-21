/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/******************************************************************************
* File timeft.c:  Contains driver for command timeft.  This command executes  *
*      --------   several ft's at once.  It is intended for use in	      *
* conjunction with the clocktime timing package.			      *
******************************************************************************/
#define FILE_IS_TIMEFT.C

/* Debug code */
/* #include <math.h> */

#include <stdio.h>
#include "vnmrsys.h"

#ifdef AP
extern char *skyalloc();
#else (not) AP
extern char *malloc();
#endif AP

extern int dummyft_timer_no;
extern int math;

static int tft_init();

/******************************************************************************/
timeft ( argc, argv )
/* 
Purpose:
-------
     Routine main is the main driver for Vnmr command timeft.  This 
command simply executes ft's, presumably for timing purposes. 

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Count of command line arguments.
argv  :  (I  )  Command line arguments.
*/
int  argc;
char *argv[];

{ /* Begin function timeft */
   /*
   Local Variables:
   ---------------
   order     :  Number complex pairs in each fft to take.
   ninner    :  Number of fft's to take.
   logorder  :  Logarithm of order of fft to take.
   dum_data  :  Data to take fft of.
   old_math  :  Indicates state of "math" indicator, before this routine is
		called.
   i	     :  Loop control variable, for entries of work space vector.
   */
   int  order, ninner, logorder, old_math, i;
   float *dum_data;
   /* Debug code */
   /* float factor; */
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Initialize */
   if ( tft_init ( argc, argv, &order, &ninner ) != 0 )
      ABORT;

   /* Set "math" indicator */
   old_math = math;
   math = 'f';

   /* Obtain log of order of fft to take */
   logorder = fnpower ( order ) - 1;

   /* Obtain work space */
#ifdef AP
   if ( ( dum_data = (float *)skyalloc ( (unsigned)(2*sizeof(float)*order) ) ) 
      == 0 )
   {  perror ( "timeft: skyalloc" );
      exit ( 1 );
   }
#else (not) AP
   if ( ( dum_data = (float *)malloc ( (unsigned)(2*sizeof(float)*order) ) ) 
      == 0 )
   {  perror ( "timeft: malloc" );
      exit ( 1 );
   }
#endif AP

/* Debug code */
/* 
#define NPERIODS 4
   factor = 2 * M_PI * NPERIODS / order;
   for ( i = 0 ; i < order ; i++ )
   {  dum_data[2*i]   = cos ( i * factor );
      dum_data[2*i+1] = sin ( i * factor );
   }
*/
   for ( i = 0 ; i < order+order ; i++ )
      dum_data[i] = (float)0;

   /* Take the fft's */
   (void)start_timer ( dummyft_timer_no );
   while ( ninner-- > 0 )
      fft ( dum_data, order, logorder, (float)(-1), (float)(1) );
      /* sleep ( (unsigned)1 ); */
   (void)stop_timer ( dummyft_timer_no );

   /* Free work space used */
#ifdef AP
   skydeall ( (char *)dum_data );
#else (not) AP
   free ( (char *)dum_data );
#endif AP

   /* restore "math" indicator */
   math = old_math;

   /* Normal successful return */
   RETURN;

} /* End function timeft */

/******************************************************************************/
static int tft_init ( argc, argv, porder, pninner )
/* 
Purpose:
-------
     Routine tft_init shall do initialization for program timeft.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc     :  (I  )  Count of command line arguments.
argv     :  (I  )  Command line arguments.
porder   :  (  O)  Number of complex pairs in each fft to take.
pninner  :  (  O)  Number of fft's to take, repeatedly.
*/
int  argc;
int  *porder, *pninner;
char *argv[];

{ /* Begin function tft_init */
   /*
   Local Variables:  None.
   ---------------
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Check for proper number of arguments */
   if ( argc != 3 )
   {  Werrprintf ( "Usage:  timeft numpairs numreps\n" );
      ABORT;
   }

   /* Obtain order of fft to take, and number of repetitions. */
   if ( sscanf ( argv[1], "%d", porder ) != 1 )
   {  Werrprintf ( "Could not parse \"%s\" for order\n", argv[1] );
      ABORT;
   }
   if ( sscanf ( argv[2], "%d", pninner ) != 1 )
   {  Werrprintf ( "Could not parse \"%s\" for num. reps.\n", argv[2] );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function tft_init */

#undef FILE_IS_TIMEFT.C
