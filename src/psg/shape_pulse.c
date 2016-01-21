/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* These program functions allow the pulsesequence function to call
   the function:


  "shape_pulse(pulse shape, pulse width, pulse phase, power table,
	       phase table, maximum power level, number of pulse
	       intervals, rx1, rx2)"


   A description of these new program functions follows.

   The pulse shape can currently take the values:  (1) gauss; (2)
   halfgauss; and (3) hermite.

   First, the shaped pulse is divided into an odd (or even) number of
   pulse intervals.  The minimum length for any of these intervals
   is given by PULSEMIN (10 us).  The maximum number of intervals is
   not resricted by software.  The actual number of intervals may be
   entered by the user; an odd number is preferable.  If the number
   of intervals used is too large, FIFO underflow problems may occur.
   For pulse shapes that exhibit no zero crossings, e.g., gaussian
   or halfgaussian, no additional shape definition is really gained
   by setting the number of intervals to greater than 79 or 39
   (respectively).

   A dc offset may be applied to the shaped pulse by using
   MINPWR.  MINPWR should be set to a default value of 33 if tuned
   isolators are used in the transmitter line.  These isolators
   are located either at the entry port of the 1H/19F pre-amp or
   at the output of the 1H/19F amplifier.  MINPWR should be
   set to a default value of 0 if the system is configured with
   either a T-switch or a T/R-switch.

   The value of the shaped function at the midpoint of each interval
   is first calculated and is then used to generate the power for
   that interval from the following equation:


             MINPWR + (spwr - MINPWR)*(average value) .


   The "power" statement also extends any previous event by approxi-
   mately 4 us (the APBUSTIME in the program).  It has been empirically
   determined that a power change of 40 dB requires 3-4 us to settle.
   This program currently uses NO delay for power settling.  If such a
   delay is desired, the constant PWRDELAY should be set to some value
   other than zero.

   These program functions make use of two real-time AP tables which
   are selected in the pulse sequence.  It is also IMPORTANT TO NOTE
   that the AP variables V12-V14 are trashed by this function.

   Currently, only amplitude and alternating phase-modulated shaped
   pulses are supported with this software.  Shaped pulses such as
   the hyperbolic secant, which require small-angle phase modula-
   tion schemes, are NOT supported at this time. */



/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#include <stdio.h>
#include <math.h>

#define CURRENT		1
#define MAXPATHL	128
#define PULSEMIN 	10.0e-6
#define APBUSTIME 	4.0e-6
#define PWRDELAY 	0.0e-6
#define MAXPWR 		63.0
#define MINPWR 		0.0
#define MAXCHGPWR 	40.0
#define REFPOWER	48	/* in dB; typical for protons 	*/
#define MAXTPWR		63	/* in dB			*/
#define MAXDPWR		63	/* in dB			*/

FILE	*fopen();
FILE	*filename;

extern char	curexp[MAXPATHL];
extern int	*malloc();

/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/



/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
double gauss(x,y)
double	x,
	y;
{
 double	value;

/* Since the power scale is in dB's and since dB's are logarithmic,
   the actual function from which power increments are calculated is
   the LOG of the function describing the true shape of the function.
   The actual function must also be normalized so that it yields a
   value of 1 for its maximum and a value of 0 for its minimum. */

 value =  x*(2*y - x)/(y*y);
 return(value);
}
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
double hermite(x,y)
double	x,
	y;
{
 double	t0,
	aa,
	bb,
	value;


 aa = getval("herma");	/* Constant "a" for the Hermite pulse */
 bb = getval("hermb");	/* Constant "b" for the Hermite pulse */

 aa = aa*1e+6;		/* initially in units of ms(-2) */
 bb = bb*1e+6;		/* initially in units of ms(-2) */

/* Since the power scale is in dB's and since dB's are logarithmic,
   the actual function from which power increments are calculated is
   the LOG of the function describing the true shape of the function.
   The actual function must also be normalized so that it yields a
   value of 1 for its maximum and a value of 0 for its minimum. */

 t0 = (x - y)*(x - y);
 value = 10*(1 - aa*t0)*(exp(-bb*t0));

 if (fabs(value) < 1.0)
    value = 1.0;

 value = ( (value < 0.0) ? (-1)*log10(-value) : log10(value) );
 return(value);
}
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/



/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
shape_pulse(shape, pws, phs, pwrtbl, phstbl, spwr, npulses, rx1, rx2)
char	shape[MAXSTR];
codeint	phs,
	pwrtbl,
	phstbl;
int	npulses;
double	pws,
	rx1,
	rx2,
	spwr;
{
 char		shapefilename[MAXPATHL];
 int	 	i,
		output = FALSE,
		powerval,
 		*shape_pwr,
		*start_pwrpntr,
		*shape_phase,
		*start_phspntr,
		calculate;
 double		delta_pwr,
		base_pwr,
		pulse_time,
		shape_value,
		plength,
		tmp;
 extern double	gauss(),
		hermite();


/*************************************************************
*  The minimum pulse interval and the maximum power for the  *
*  shaped pulse are adjusted to stay within the specified    *
*  bounds.                                                   *
*************************************************************/

 if (P_getreal(CURRENT, "tdebug", &tmp, 1) == 0)
    output = (int) (tmp + 0.5);

 if (pws < 0.199e-7)	/* no pulse */
    return;

 if (npulses <= 0)
 {
    text_error("Shape_pulse:  npulses must be >= 0");
    abort(1);
 }

 plength = pws/((double) (npulses));
 while (plength < PULSEMIN)
 {
    npulses -= 2;
    if (npulses <= 0)
    {
       text_error("Shape_pulse:  unable to make pulse interval long enough");
       abort(1);
    }

    plength = pws/((double) (npulses));
 }


/****************************************************
*  Check to see if new power and phase array needs  *
*  to be calculated.                                *
****************************************************/

 calculate = ((Table[pwrtbl - BASEINDEX]->table_number == 0) ||
 	      (Table[pwrtbl - BASEINDEX]->table_size != npulses) ||
	      (Table[phstbl - BASEINDEX]->table_size != npulses));

 if (!calculate)
 {
    if (Table[phstbl - BASEINDEX]->table_number == 0)
    {
       text_error("Shape_pulse:  undefined table for phases");
       abort(1);
    }

    if (Table[pwrtbl - BASEINDEX]->table_number != last_table)
    {
       calculate = TRUE;
    }
    else
    {
       if (Table[phstbl - BASEINDEX]->table_number != (last_table + 1))
       {
          text_error("Shape_pulse:  mismatch in power and phase tables");
          abort(1);
       }
    }
 }


/*********************************************
*  Calculate new power and phase arrays for  *
*  shaped pulse if necessary.                *
*********************************************/

 if (calculate)
 {
    if (spwr > MAXPWR)
       spwr = MAXPWR;
    if (spwr < MINPWR)
       spwr = MINPWR;

    if ((spwr - MAXCHGPWR) > MINPWR)
    {
       delta_pwr = MAXCHGPWR;
       base_pwr = spwr - MAXCHGPWR;
    }
    else
    {
       delta_pwr = spwr - MINPWR;
       base_pwr = MINPWR;
    }


/*****************************************************
*  Create pointers to the power and phase values to  *
*  be used in the shaped pulse.                      *
*****************************************************/

    start_pwrpntr = (int *)malloc(sizeof(int) * (npulses + 1));
    shape_pwr = start_pwrpntr;
    if (start_pwrpntr == NULL)
    {
       text_error("shape_pulse:  unable to create pointer to power values");
       abort(1);
    }

    start_phspntr = (int *)malloc(sizeof(int) * (npulses + 1));
    shape_phase = start_phspntr; 
    if (start_phspntr == NULL) 
    { 
       text_error("shape_pulse:  unable to create pointer to phase values"); 
       abort(1); 
    }


/***********************************************
*  Calculate the appropriate power levels for  *
*  the shaped pulse.                           *
***********************************************/

    if ((output) && (ix == 1))
    {
       strcpy(shapefilename, curexp);
       strcat(shapefilename, "/");
       strcat(shapefilename, shape);
       strcat(shapefilename, ".out");
       filename = fopen(shapefilename, "w");
       if (filename == 0)
       {
          text_error("Unable to open shape output file");
          output = FALSE;
       }
       else
       {
          fprintf(filename, "POWER (dB)            PHASE\n");
          fprintf(filename, "----------            -----\n");
          fprintf(filename, "                           \n");
       }
    }

    for (i = 0; i < npulses; i++)
    {
       pulse_time = plength*((double) (i) + 0.5);

       if (strcmp(shape, "rhalfgauss") == 0)
       { /* rising half-gaussian */
          shape_value = gauss(pulse_time, pws);
       }
       else if (strcmp(shape, "fhalfgauss") == 0)
       { /* falling half-gaussian */
          pulse_time = plength*npulses - pulse_time;
          shape_value = gauss(pulse_time, pws);
       }
       else if (strcmp(shape, "gauss") == 0)
       { /* gaussian */
          shape_value = gauss(pulse_time, pws/2.0);
       }
       else if (strcmp(shape, "hermite") == 0)
       { /* hermitian */
          shape_value = hermite(pulse_time, pws/2.0);
       }
       else
       {
          text_error("The selected shaped pulse is not supported.\n");
          if ((output) && (ix == 1))
          {
             fprintf(filename, "The selected shaped pulse is not supported.\n");
             fclose(filename);
          }

          abort(1);
       }

       if (shape_value < 0)
       {
          shape_value = (-1)*shape_value;
          *shape_phase = 2;
       }
       else
       {
          *shape_phase = 0;
       }

       *shape_pwr = (int) (shape_value*delta_pwr + base_pwr + 0.5);
       if ( ((strcmp(shape, "fhalfgauss") == 0) && (i == 0)) ||
		((strcmp(shape, "rhalfgauss") == 0) && (i == (npulses - 1))) )
       {
          *shape_pwr -= 6;
       }

       if ((output) && (ix == 1))
       {
          powerval = REFPOWER - (MAXTPWR - (*shape_pwr)) - (MAXDPWR - dhp);

          if (*shape_phase == 0)
          {
             fprintf(filename, "   %4d              %6.1f\n", powerval, 0.0);
          }
          else
          {
             fprintf(filename, "   %4d              %6.1f\n", powerval, 180.0);
          }
       }

       shape_phase++;
       shape_pwr++;
    }

    if ((output) && (ix == 1))
    {
       fprintf(filename, "\n");
       fprintf(filename, "\n");
       fprintf(filename, "The pattern of the shaped pulse = %s.\n", shape);
       fclose(filename);
    }


/*************************************************************
*  Create real-time power and phase array for shaped pulse.  *
*  If two or more shaped pulses use the same power and       *
*  phase array, tables for the arrays calculated by pulses   *
*  2 through N are not created by settable().                *
*************************************************************/

    settable(pwrtbl, npulses, start_pwrpntr);
    settable(phstbl, npulses, start_phspntr);

    setautoincrement(pwrtbl);
    setautoincrement(phstbl);


/************************************************
*  Release pointers to phase and power arrays.  *
************************************************/

    if (free(start_pwrpntr) == NULL)
    {
       text_error("shape_pulse:  unable to free pointer to power values");
       abort(1);
    }
    else if (free(start_phspntr) == NULL)
    {
       text_error("shape_pulse:  unable to free pointer to phase values"); 
       abort(1); 
    }
 }


/***********************************************
*  Calculate the number of loops in real-time  *
*  based on "npulses".                         *
***********************************************/

 assign(zero, v12);
 mult(three, three, v13);
 i = 9;

 while (npulses)
 {
    if (npulses >= i)
    {
       add(v12, v13, v12);
       npulses -= i;
    }
    else
    {
       divn(v13, three, v13);
       i /= 3;
    }
 }


/**********************************
*  The shaped pulse is executed.  *
**********************************/

 power(zero, TODEV);	/* maximum attenuation during rx1 delay */
 rcvroff();
 delay(rx1);

 loop(v12, v13);
    getelem(phstbl, v14, v14);
    add(v14, phs, v14);
    apovrride();
    power(pwrtbl, TODEV);
    txphase(v14);
    delay(PWRDELAY);
    gate(TXON, TRUE);
    delay(plength - APBUSTIME);
    gate(TXON, FALSE);
 endloop(v13);

 power(zero, TODEV);	/* maximum attenuation during rx2 delay */
 delay(rx2);
 rcvron();

 return;
}
