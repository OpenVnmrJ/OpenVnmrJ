/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <strings.h>
#include <stdio.h>
#include <math.h>

#define CURRENT		1
#define MAXPATHL	128

extern char	curexp[MAXPATHL];

/*---------------------------------------------------------------
|								|
|			    expt1()/0				|
|								|
|  Exponential sampling for the t1 domain; can be incorporated	|
|  into any existing 2D sequence by adding the line		|
|								|
|								|
|	   		#include <expt1.c>			|
|								|
|   after the							|
|								|
|	  	       #include <standard.h>			|
|								|
|								|
|  line.  To effect exponential sampling, the value for the t1	|
|  evolution time, "d2", is recalculated within the pulse se-	|
|  quence by the line						|
|								|
|	                d2 = expt1calc()			|
|								|
|								|
|  It is also necessary to declare expt1calc() as		|
|								|
|								|
|		     extern double  expt1calc()			|
|								|
|								|
|  at the beginning of the pulse sequence.			|
+--------------------------------------------------------------*/

/* Parameters:

	nifix	-	the number of increments for fixed-interval
			sampling of t1.
	   ni	-	the number of increments for exponential
			sampling of t1.


   The function which describes "d2" is a combination of an increasing
   exponential function and an increasing linear function with slope
   1/sw1 (when plotted agains "nifix").  The exponential time constant,
   lambda, is given by the following equation:


             lambda = {ln[ nifix - niexp + 1 ]}/(niexp - 1)   */


double expt1calc()
{
 char		t1filename[MAXPATHL];
 int		nifix,
		niexp,
		arraydim,
		t1incr,
		incval,
		outputflag = TRUE;
 double		expconst,
		tmp,
		swf1;
 FILE		*textfile;


/*******************
*  LOAD VARIABLES  *
*******************/

 if (P_getreal(CURRENT, "arraydim", &tmp, 1) < 0)
 {
    text_error("expt1:  cannot find parameter arraydim");
    abort(1);
 }

 arraydim = (int) (tmp + 0.5);
 if (arraydim < 2)
 {
    text_error("expt1:  arraydim must be > 1");
    abort(1);
 }

 if (P_getreal(CURRENT, "ni", &tmp, 1) < 0)
 {
    text_error("expt1:  cannot find parameter ni"); 
    abort(1);
 }

 niexp = (int) (tmp + 0.5);
 if (niexp < 2)
 { 
    text_error("expt1:  ni must be > 1"); 
    abort(1); 
 }

 if (P_getreal(CURRENT, "nifix", &tmp, 1) < 0)
 {
    text_error("expt1:  cannot find parameter nifix"); 
    abort(1);
 }

 nifix = (int) (tmp + 0.5);
 if (nifix < 2) 
 {  
    text_error("expt1:  nifix must be > 1");  
    abort(1);  
 }

 if (P_getreal(CURRENT, "sw1", &swf1, 1) < 0)
 {
    text_error("expt1:  cannot find parameter sw1");
    abort(1);
 }


/*********************************************************
*  CALCULATE EXPONENTIAL INCREMENT VALUES AND D2 VALUES  *
*********************************************************/

 if (ix == 1)
 {
    strcpy(t1filename, curexp);
    strcat(t1filename, "/expt1.out");
    textfile = fopen(t1filename, "w");
    if (textfile == 0)
    {
       text_error("Could not open expt1 output file");
       outputflag = FALSE;
    }
    else
    {
       fprintf(textfile, "NI = %d for fixed-interval sampling\n", nifix);
       fprintf(textfile, "NI = %d for exponential sampling\n", niexp);
       fprintf(textfile, "\n");
       fprintf(textfile, "\n");
       fprintf(textfile, "NUMBER       INCREMENT          D2(ms)\n");
       fprintf(textfile, "------       ---------          ------\n");
       fprintf(textfile, "\n");
    }

    expconst = log((double) (nifix - niexp + 1))/(niexp - 1);
    for (t1incr = 0; t1incr < niexp; t1incr++)
    {
       incval = (int) (exp(expconst*t1incr) + t1incr);
       if (outputflag)
       {
          fprintf(textfile, "%4d          %4d              %5.2f\n",
             t1incr + 1, incval, (incval/swf1)*1000);
       }
    }

    if (outputflag)
    {
       fprintf(textfile, "\n");
       fclose(textfile);
    }
 }


/*****************************************************
*  CALCULATE APPROPRIATE D2 VALUE FOR 2D EXPERIMENT  *
*****************************************************/

 t1incr = ix/(arraydim/niexp);
 expconst = log((double) (nifix - niexp + 1))/(niexp - 1);
 incval = (int) (exp(expconst*t1incr) + t1incr);

 return((double) (incval)/swf1);
}
