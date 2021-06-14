/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* This software allows the pulsesequence function to call the function:

  "gen_apshaped_pulse(pattern, pulse width, pulse phase, power table,
	       phase table, rx1, rx2, device)"

   Description:
	gen_apshaped_pulse provides fine-grained "waveform generator-type"
	pulse shaping capability for systems with direct frequency synthesis
	and on-board small angle phase and linear amplitude control via AP bus.
	A pulse shape file for the waveform generator (/vnmr/shapelib/*.RF)
	is interpreted and translated into AP bus statements.
	The amplitude pattern is stored in a phase table, which is then read
	in a loop. Also the phase pattern is stored in a phase table; small
	angle phase shifting is used to implement the phase pattern. Irrespec-
	tive of the actual phase resolution, the minimum step size is used.
	Small angle phase shifting occurs even if the phase within the pulse
	is constant. If the duration field in the pattern file is >1.0, this
	results in multiple slices being performed for a single line. This
	can limit the complexity of the shaped pulse, because it will increase
	the number of pulse slices. Currently, the maximum number of pulse
	slices is 1024 (MAXSLICES); this may be too much already, depending
	on the slice width, because it corresponds to 4096 - 5120 FIFO words
	already - a possible consequence is FIFO underflow (Note that with
	most - if not all - pulse shapes that have been defined up to now
	the "relative duration" field is left at the default value of 1.0).
	The "gate" field in the pattern file is currently NOT used in this
	function - the transmitter is switched on throughout the shape.
	Note that the real-time variables v12 and v13 are used by this
	function. Also, the two phase table names that are supplied as
	arguments should be different with every call to this function.

   Arguments:
	"pattern" is a shape file (*.RF) in an application directory's
	   shapelib. Note that the "gate" field (field 4) in the
	   pattern is not used, and the "relative duration" field (field 3)
	   should preferably be 1.0 or at least small numbers, see above.
	"pulse width" is the total pulse width (excluding the receiver gating
	   delays around the pulse).
	"pulse phase" is the 90 degrees phase shift of the pulse. For small
	   angle phase shifting note that this function sets the phase step
	   size to the minimum on the one channel that is used.
	"power table, phase table" are two table variables (t1 - t60) that
	   will be used as intermediate storage addresses for the amplitude
	   and phase tables. If this function is called more than once,
	   different table names should be used in each call.
	"rx1, rx2" receiver gating times before and after the pulse.
	"device" is the r.f. channel (TODEV, DODEV, etc. ) on which the pulse
	   should be performed.

   This function will mostly be called through macros "apshaped_pulse",
	"apshaped_decpulse", "apshaped_dec2pulse".

   Debugging: if a numeric parameter "tdebug" exists and is set >0, the
	      function prints out the actual phase and power tables at full
	      length (for ix == 1 only).

   created 93/01/14	r.kyburz

*/


/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "acodes.h"
#include "oopc.h"
#include "acqparms.h"
#include "rfconst.h"
#include "aptable.h"
#include "apdelay.h"
#include "macros.h"
#include "vfilesys.h"

#define CURRENT		1
#define MAXPATHL	128
#define PHASERESOLUTION	0.25
#define MAXSLICES	1024
#define MINDELAY	0.2e-6
#define INOVAMINDELAY 0.0995e-6

FILE	*shapefile;

extern double	getval();
extern char	curexp[MAXPATHL];

static int	*shape_pwr;
static int	*shape_phase;
static char	line[MAXSTR];
extern int      rcvroff_flag;   /* global receiver flag */
extern int      rcvr_hs_bit;
extern int      ap_interface;
extern int	newacq;


/*--------------------------------------------------------------+
| readshapeline()/0						|
|    read one line from the shape file, excluding comments	|
|    (skip '#' characters and all text up to the next EOL)	|
|								|
|    930114	r.kyburz					|
+--------------------------------------------------------------*/
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
int readshapeline()
{
   int  i = 0,
        comment = FALSE;
 
   char c = fgetc(shapefile);
   while ((c != '\n') && (c != EOF))
   {
      if (c == '#') comment = TRUE;
      if (! comment)
      {
         line[i] = c;
         i++;
      }
      c = fgetc(shapefile);
   }
   line[i] = '\0';
   if (c == EOF)
      return(FALSE);
   else
      return(TRUE);
}
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


/*--------------------------------------------------------------+
| gen_apshaped_pulse()/8					|
|    perform a "WFG shaped" pulse via AP bus statements		|
|								|
|    930114	r.kyburz					|
+--------------------------------------------------------------*/
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
void gen_apshaped_pulse(shape, pws, phs, pwrtbl, phstbl, rx1, rx2, device)
char	shape[MAXSTR];
codeint	phs,
	pwrtbl,
	phstbl;
int	device;
double	pws,
	rx1,
	rx2;
{
   /*-----------------------+
   | define local variables |
   +-----------------------*/
   struct p_slice
   {
      float phase, amplitude, duration;
   };
   char fname[MAXSTR],
        errstr[MAXSTR];
   int 	i, k, nitems,
	*start_pwrpntr,
	*start_phspntr;
   int npulses = 0,
       nslices = 0,
       output = FALSE;
   double	plength, aptime, tmp, mindelay;
   float	phas, amp, dur, gate;
   struct p_slice *spulse, *start_spulse;
   struct p_slice slice;

   if (ap_interface < 4)
   {
      text_error("apshaped_pulse is available only on UnityPlus.\n");
      psg_abort(1);
   }

    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;
   /*-------------------------------+
   | Set debugging flag, if desired |
   +-------------------------------*/
   if (P_getreal(CURRENT, "tdebug", &tmp, 1) == 0)
      output = (int) (tmp + 0.5);
   if (ix > 1) output = FALSE;

   /*-------------------------+
   | Don't bother if no pulse |
   +-------------------------*/
   if (pws==0.0) return;

   /*--------------------------+
   | Find shape file name(s) |
   +--------------------------*/

   shapefile = NULL;
   if (appdirFind(shape,"shapelib",fname,".RF",R_OK) )
      shapefile = fopen(fname,"r");
   if (shapefile == 0)
   {
      abort_message("apshaped_pulse:  shape %s not found.\n", shape);
   }
   if (output) printf("table file found: %s\n",fname);

   /*-----------------------------------------------------------+
   | scan the shape file, find out about the number of	actual	|
   | shape lines and shape slices, for subsequent memory	|
   | allocation for shape structure, phase and amplitude tables |
   +-----------------------------------------------------------*/
   while (readshapeline())
   {
      nitems = sscanf(line, "%f %f %f %f", &phas, &amp, &dur, &gate);
      if ((nitems != 0) && (nitems != EOF))
      {
	 if ((nitems < 4) || ((nitems == 4) && (gate != 0.0)))
	 {
            npulses++;
            if (nitems > 2)
	       nslices += (int) (dur + 0.5);
	    else nslices += 1;
         }
      }
   }
   if (output)
      printf("shape definition: %d pulses, %d slices\n",npulses,nslices);


   /*-------------------------------------+
   | no slices defined: ill-defined shape |
   +-------------------------------------*/
   if (nslices == 0)
   {
      strcpy(errstr,"apshaped_pulse:  incorrect shape definition in ");
      strcat(errstr,fname);
      text_error(errstr);
      fclose(shapefile);
      psg_abort(1);
   }

   /*--------------------------------------------------+
   | check on the upper limit for the number of slices |
   | to avoid FIFO underflow problems		       |
   +--------------------------------------------------*/
   if (nslices > MAXSLICES)
   {
      strcpy(errstr,"apshaped_pulse:  too many elements in pulse shape ");
      strcat(errstr,fname);
      text_error(errstr);
      fclose(shapefile);
      psg_abort(1);
   }


   /*--------------------------------------------------+
   | Create pointers to the shaped pulse structure for |
   | intermediately storing the pulse shape.	       |
   +--------------------------------------------------*/
   start_spulse = (struct p_slice *)malloc(sizeof(slice) * (npulses + 2));
   spulse = start_spulse;
   if (spulse == NULL)
   {
      text_error("apshaped_pulse:  unable to allocate buffer memory");
      fclose(shapefile);
      psg_abort(1);
   }

   /*--------------------------------------------------+
   | Create pointers to the power and phase values to  |
   | be used in the shaped pulse.		       |
   +--------------------------------------------------*/
   start_pwrpntr = (int *)malloc(sizeof(int) * (nslices + 2));
   shape_pwr = start_pwrpntr;
   if (start_pwrpntr == NULL)
   {
      text_error("apshaped_pulse:  unable to create pointer to power values");
      fclose(shapefile);
      psg_abort(1);
   }
   start_phspntr = (int *)malloc(sizeof(int) * (nslices + 2));
   shape_phase = start_phspntr;
   if (start_phspntr == NULL)
   {
      text_error("apshaped_pulse:  unable to create pointer to phase values");
      fclose(shapefile);
      psg_abort(1);
   }
 
   /*---------------------------------------+
   | go back to the start of the shape file |
   +---------------------------------------*/
   rewind(shapefile);

   /*--------------------------------------------------+
   | now read the shape into memory (spulse structure) |
   +--------------------------------------------------*/
   while (readshapeline())
   {
      nitems = sscanf(line, "%f %f %f %f", &phas, &amp, &dur, &gate);
      if ((nitems != 0) && (nitems != EOF))
      {
	 if ((nitems < 4) || ((nitems == 4) && (gate != 0.0)))
	 {
	    spulse->phase = phas;
            if ((nitems > 1) && (amp <= 1023.0))
	       spulse->amplitude = amp;
	    else
	       spulse->amplitude = 1023.0;
            if (nitems > 2)
	    {
	       if ((int) (dur + 0.5) < 256)
	          spulse->duration = dur;
	       else
	          spulse->duration = 255.0;
	    }
	    else
	       spulse->duration = 1.0;
	    spulse++;
	 }
      }
   }

   /*-----------------+
   | Close shape file |
   +-----------------*/
   fclose(shapefile);

   /*-------------------------------------------------------+
   | create amplitude and phase tables for the shaped pulse |
   +-------------------------------------------------------*/
   spulse = start_spulse;
   if (output)
   {
      printf("\n%s table values:\n",fname);
      printf("   phase  amplitude\n");
   }
   for (i = 0; i < npulses; i++)
   {
      for (k = 0; k < (int) (spulse->duration + 0.5); k++)
      {
         /*-----------------------------------------------------------------+
	 | extract phase value, calculate step number for minimum step size |
	 +-----------------------------------------------------------------*/
	 *shape_phase = (int) ((spulse->phase)/PHASERESOLUTION + 0.5);

         /*------------------------------------------+
	 | calculate phase steps as positive integer |
	 +------------------------------------------*/
         while (*shape_phase < 0)
	    *shape_phase += (int) (360.0/PHASERESOLUTION + 0.5);

         /*--------------------+
	 | extract power value |
	 +--------------------*/
	 *shape_pwr = (int) ((4095.0/1023.0*spulse->amplitude) + 0.5);

         if (output) printf("%8d %8d\n",*shape_phase,*shape_pwr);

	 /*--------------------------------------------------------+
	 | jump to next pair of table element (actual pulse slice) |
	 +--------------------------------------------------------*/
         shape_phase++;
         shape_pwr++;
      }

      /*---------------------------+
      | jump to next pulse element |
      +---------------------------*/
      spulse++;
   }

   /*-------------------------------------------------+ 
   | free memory allocated for shaped pulse structure |
   +-------------------------------------------------*/
   free(start_spulse);

   /*-----------------------------------------------------------+
   |  Create real-time power and phase array for shaped pulse.  |
   |  If two or more shaped pulses use the same power and       |
   |  phase array, tables for the arrays calculated by pulses   |
   |  2 through N are not created by settable().                |
   +-----------------------------------------------------------*/
   settable(pwrtbl, nslices, start_pwrpntr);
   settable(phstbl, nslices, start_phspntr);

   /*---------------------------------------------+ 
   | in the pulse loop we will automatically step |
   | through the amplitude and phase tables	  |
   +---------------------------------------------*/
   setautoincrement(pwrtbl);
   setautoincrement(phstbl);

   /*----------------------------------------------+
   |  Release pointers to phase and power arrays.  |
   +----------------------------------------------*/
   free(start_pwrpntr);
   free(start_phspntr);

   /*---------------------------------------------+
   |  Calculate the number of loops in real-time  |
   |  based on "nslices".                         |
   +---------------------------------------------*/
   assign(zero, v12);
   mult(three, three, v13);
   i = 9;
   npulses = nslices;
   while (i <= npulses/3)
   {
      mult(v13, three, v13);
      i *= 3;
   }
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

   /*-----------------------+
   | Calculate slice length |
   | based on "nslices".    |
   +-----------------------*/
   plength = pws / (double) nslices;
   if (output) printf("slice duration: %12.9lf sec\n\n", plength);

   /*----------------------------------------------------------+
   | Calculate time spent for AP bus events within each slice  |
   +----------------------------------------------------------*/
   aptime = POWER_DELAY + SAPS_DELAY;

   if ( (plength - aptime) < mindelay)
   {
      text_error("apshaped_pulse: pulse too short or too many elements in shape");
      psg_abort(1);
   }


   /*==================================+
   ||				      ||
   ||  The shaped pulse is executed.  ||
   ||  =============================  ||
   ||				      ||
   +==================================*/

   /*------------------------------------------+
   | set 90 degrees phase, set phase step size |
   +------------------------------------------*/
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   SetRFChanAttr(RF_Channel[device], SET_PHASESTEP, PHASERESOLUTION, 0);


   /*--------------------------------+
   | gate receiver off, do rx1 delay |
   +--------------------------------*/
   if (newacq)
      HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
   else
      SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
   SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, OFF, 0);
   if (rx1 - aptime > 0.0)
      delay(rx1 - aptime);

   /*----------------------------------------------------+
   | before turning on the transmitter and entering the  |
   | pulse loop preset phase and amplitude for the first |
   | slice, to avoid a glitch at the start of the pulse  |
   +----------------------------------------------------*/
   SetRFChanAttr(RF_Channel[device], SET_RTPWRF, pwrtbl, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE, phstbl, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   decr(v12);

   /*-----------------------------------------------------+
   | turn on transmitter, then in a soft loop execute one |
   | slice, then set power and phase for the next slice   |
   | after the loop do the last slice, then switch rf off |
   +-----------------------------------------------------*/
   SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, ON, 0);
   loop(v12, v13);
      delay(plength - aptime);
      SetRFChanAttr(RF_Channel[device], SET_RTPWRF, pwrtbl, 0); 
      SetRFChanAttr(RF_Channel[device], SET_RTPHASE, phstbl, 0);
      SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   endloop(v13);
   delay(plength);
   SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, OFF, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE, zero, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   SetRFChanAttr(RF_Channel[device], SET_PWRF, 4095, 0); 

   /*-----------------------------------------------+
   | perform rx2 delay, gate receiver back on again |
   +-----------------------------------------------*/
   if (rx2 - aptime > 0.0)
      delay(rx2 - aptime);
   if (newacq) {
      if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
   }
   else {
      if ( isBlankOn(OBSch) )
	   SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
   }
   if ( isBlankOn(device) )
      SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, ON, 0);
}

/*--------------------------------------------------------------+
| G_apshaped_pulse()/8					|
|    perform a "WFG shaped" pulse via AP bus statements		|
|								|
|    930114	r.kyburz					|
|    960821	m.howitt - modified to remove table ptr from	|
|				calling arguments.		|
+--------------------------------------------------------------*/
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
G_apshaped_pulse(shape, pws, phs, rx1, rx2, device)
char	shape[MAXSTR];
codeint	phs;
int	device;
double	pws,
	rx1,
	rx2;
{
   /*-----------------------+
   | define local variables |
   +-----------------------*/
   struct p_slice
   {
      float phase, amplitude, duration;
   };
   char filep1[MAXSTR],             /* vnmruser */
        filep2[MAXSTR],             /* vnmrsystem */
	fname[MAXSTR],
        errstr[MAXSTR];
   int 	i, k, nitems,
	*start_pwrpntr,
	*start_phspntr;
   int npulses = 0,
       nslices = 0,
       output = FALSE;
   extern double rfchan_getpwrf();
   double	plength, aptime, tmp, mindelay, mod_factor;
   float	phas, amp, dur, gate;
   struct p_slice *spulse, *start_spulse;
   struct p_slice slice;
   codeint acodelocpwr,acodelocphs;

   if (ap_interface < 4)
     mod_factor=4095.0;
   else
     mod_factor = rfchan_getpwrf(device);
   
   if (ap_interface < 4)
   {
      text_error("apshaped_pulse is available only on UnityPlus.\n");
      psg_abort(1);
   }

    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;
   /*-------------------------------+
   | Set debugging flag, if desired |
   +-------------------------------*/
   if (P_getreal(CURRENT, "tdebug", &tmp, 1) == 0)
      output = (int) (tmp + 0.5);
   if (ix > 1) output = FALSE;

   /*-------------------------+
   | Don't bother if no pulse |
   +-------------------------*/
   if (pws==0.0) return;

   /*--------------------------+
   | Create shape file name(s) |
   +--------------------------*/
   strcpy(filep1,userdir);
   strcat(filep1,"/shapelib/");
   strcat(filep1,shape);
   strcat(filep1,".RF");

   strcpy(filep2,getenv("vnmrsystem"));
   strcat(filep2,"/shapelib/");
   strcat(filep2,shape);
   strcat(filep2,".RF");

   /*-------------------------+
   | Find and open shape file |
   +-------------------------*/
   if (output) printf("looking for pulse shape file %s.RF ...\n",shape);
   shapefile = fopen(filep1, "r");	/* check for local file first */
   if (shapefile == NULL)
   {
      shapefile = fopen(filep2, "r");	/* if not found, find global file */
      if (shapefile == NULL)
      {
         text_error("apshaped_pulse:  shape file not found.\n");
         psg_abort(1);
      }
      else
      {
         strcpy(fname,filep2);	/* remember file name for error messages */
      }
   }
   else
   {
      strcpy(fname,filep1);	/* remember file name for error messages */
   }
   if (output) printf("table file found: %s\n",fname);

   /*-----------------------------------------------------------+
   | scan the shape file, find out about the number of	actual	|
   | shape lines and shape slices, for subsequent memory	|
   | allocation for shape structure, phase and amplitude tables |
   +-----------------------------------------------------------*/
   while (readshapeline())
   {
      nitems = sscanf(line, "%f %f %f %f", &phas, &amp, &dur, &gate);
      if ((nitems != 0) && (nitems != EOF))
      {
	 if ((nitems < 4) || ((nitems == 4) && (gate != 0.0)))
	 {
            npulses++;
            if (nitems > 2)
	       nslices += (int) (dur + 0.5);
	    else nslices += 1;
         }
      }
   }
   if (output)
      printf("shape definition: %d pulses, %d slices\n",npulses,nslices);


   /*-------------------------------------+
   | no slices defined: ill-defined shape |
   +-------------------------------------*/
   if (nslices == 0)
   {
      strcpy(errstr,"apshaped_pulse:  incorrect shape definition in ");
      strcat(errstr,fname);
      text_error(errstr);
      fclose(shapefile);
      psg_abort(1);
   }

   /*--------------------------------------------------+
   | check on the upper limit for the number of slices |
   | to avoid FIFO underflow problems		       |
   +--------------------------------------------------*/
   if (nslices > MAXSLICES)
   {
      strcpy(errstr,"apshaped_pulse:  too many elements in pulse shape ");
      strcat(errstr,fname);
      text_error(errstr);
      fclose(shapefile);
      psg_abort(1);
   }


   /*--------------------------------------------------+
   | Create pointers to the shaped pulse structure for |
   | intermediately storing the pulse shape.	       |
   +--------------------------------------------------*/
   start_spulse = (struct p_slice *)malloc(sizeof(slice) * (npulses + 2));
   spulse = start_spulse;
   if (spulse == NULL)
   {
      text_error("apshaped_pulse:  unable to allocate buffer memory");
      fclose(shapefile);
      psg_abort(1);
   }

   /*--------------------------------------------------+
   | Create pointers to the power and phase values to  |
   | be used in the shaped pulse.		       |
   +--------------------------------------------------*/
   start_pwrpntr = (int *)malloc(sizeof(int) * (nslices + 2));
   shape_pwr = start_pwrpntr;
   if (start_pwrpntr == NULL)
   {
      text_error("apshaped_pulse:  unable to create pointer to power values");
      fclose(shapefile);
      psg_abort(1);
   }
   start_phspntr = (int *)malloc(sizeof(int) * (nslices + 2));
   shape_phase = start_phspntr;
   if (start_phspntr == NULL)
   {
      text_error("apshaped_pulse:  unable to create pointer to phase values");
      fclose(shapefile);
      psg_abort(1);
   }
 
   /*---------------------------------------+
   | go back to the start of the shape file |
   +---------------------------------------*/
   rewind(shapefile);

   /*--------------------------------------------------+
   | now read the shape into memory (spulse structure) |
   +--------------------------------------------------*/
   while (readshapeline())
   {
      nitems = sscanf(line, "%f %f %f %f", &phas, &amp, &dur, &gate);
      if ((nitems != 0) && (nitems != EOF))
      {
	 if ((nitems < 4) || ((nitems == 4) && (gate != 0.0)))
	 {
	    spulse->phase = phas;
            if ((nitems > 1) && (amp <= 1023.0))
	       spulse->amplitude = amp;
	    else
	       spulse->amplitude = 1023.0;
            if (nitems > 2)
	    {
	       if ((int) (dur + 0.5) < 256)
	          spulse->duration = dur;
	       else
	          spulse->duration = 255.0;
	    }
	    else
	       spulse->duration = 1.0;
	    spulse++;
	 }
      }
   }

   /*-----------------+
   | Close shape file |
   +-----------------*/
   fclose(shapefile);

   /*-------------------------------------------------------+
   | create amplitude and phase tables for the shaped pulse |
   +-------------------------------------------------------*/
   spulse = start_spulse;
   if (output)
   {
      printf("\n%s table values:\n",fname);
      printf("   phase  amplitude\n");
   }
   for (i = 0; i < npulses; i++)
   {
      for (k = 0; k < (int) (spulse->duration + 0.5); k++)
      {
         /*-----------------------------------------------------------------+
	 | extract phase value, calculate step number for minimum step size |
	 +-----------------------------------------------------------------*/
	 *shape_phase = (int) ((spulse->phase)/PHASERESOLUTION + 0.5);

         /*------------------------------------------+
	 | calculate phase steps as positive integer |
	 +------------------------------------------*/
         while (*shape_phase < 0)
	    *shape_phase += (int) (360.0/PHASERESOLUTION + 0.5);

         /*--------------------+
	 | extract power value |
	 +--------------------*/
	 *shape_pwr = (int) ((mod_factor/1023.0*spulse->amplitude) + 0.5);

         if (output) printf("%8d %8d\n",*shape_phase,*shape_pwr);

	 /*--------------------------------------------------------+
	 | jump to next pair of table element (actual pulse slice) |
	 +--------------------------------------------------------*/
         shape_phase++;
         shape_pwr++;
      }

      /*---------------------------+
      | jump to next pulse element |
      +---------------------------*/
      spulse++;
   }

   /*-------------------------------------------------+ 
   | free memory allocated for shaped pulse structure |
   +-------------------------------------------------*/
   free(start_spulse);

   /*-----------------------------------------------------------+
   |  Create real-time power and phase array for shaped pulse.  |
   |  If two or more shaped pulses use the same power and       |
   |  phase array, tables for the arrays calculated by pulses   |
   |  2 through N are not created by settable().                |
   +-----------------------------------------------------------*/
   /* settable(pwrtbl, nslices, start_pwrpntr); */

      putcode(TABLE);
      acodelocpwr = Codeptr - Aacode;
      putcode((codeint) nslices);	/* size of table */
      putcode((codeint) TRUE);		/* auto-inc flag */
      putcode((codeint) 1);		/* divn-rtrn factor */
      putcode(0);			/* auto-inc ct loc */
      for (i = 0; i < nslices; i++)
         putcode((codeint) start_pwrpntr[i]);

   /* settable(phstbl, nslices, start_phspntr); */

      putcode(TABLE);
      acodelocphs = Codeptr - Aacode;
      putcode((codeint) nslices);	/* size of table */
      putcode((codeint) TRUE);		/* auto-inc flag */
      putcode((codeint) 1);		/* divn-rtrn factor */
      putcode(0);			/* auto-inc ct loc */
      for (i = 0; i < nslices; i++)
         putcode((codeint) start_phspntr[i]);

   /*---------------------------------------------+ 
   | in the pulse loop we will automatically step |
   | through the amplitude and phase tables	  |
   +---------------------------------------------*/
   /* setautoincrement(pwrtbl); */
   /* setautoincrement(phstbl); */

   /*----------------------------------------------+
   |  Release pointers to phase and power arrays.  |
   +----------------------------------------------*/
   free(start_pwrpntr);
   free(start_phspntr);

   /*---------------------------------------------+
   |  Calculate the number of loops in real-time  |
   |  based on "nslices".                         |
   +---------------------------------------------*/
   assign(zero, v12);
   mult(three, three, v13);
   i = 9;
   npulses = nslices;
   while (i <= npulses/3)
   {
      mult(v13, three, v13);
      i *= 3;
   }
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

   /*-----------------------+
   | Calculate slice length |
   | based on "nslices".    |
   +-----------------------*/
   plength = pws / (double) nslices;
   if (output) printf("slice duration: %12.9lf sec\n\n", plength);

   /*----------------------------------------------------------+
   | Calculate time spent for AP bus events within each slice  |
   +----------------------------------------------------------*/
   aptime = POWER_DELAY + SAPS_DELAY;

   if ( (plength - aptime) < mindelay)
   {
      text_error("apshaped_pulse: pulse too short or too many elements in shape");
      psg_abort(1);
   }


   /*==================================+
   ||				      ||
   ||  The shaped pulse is executed.  ||
   ||  =============================  ||
   ||				      ||
   +==================================*/

   /*------------------------------------------+
   | set 90 degrees phase, set phase step size |
   +------------------------------------------*/
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   SetRFChanAttr(RF_Channel[device], SET_PHASESTEP, PHASERESOLUTION, 0);


   /*--------------------------------+
   | gate receiver off, do rx1 delay |
   +--------------------------------*/
   if (newacq)
      HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
   else
      SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
   SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, OFF, 0);
   if (rx1 - aptime > 0.0)
      delay(rx1 - aptime);

   /*----------------------------------------------------+
   | before turning on the transmitter and entering the  |
   | pulse loop preset phase and amplitude for the first |
   | slice, to avoid a glitch at the start of the pulse  |
   +----------------------------------------------------*/
      putcode(TASSIGN);
      putcode(acodelocpwr);
      putcode(tablert);
   SetRFChanAttr(RF_Channel[device], SET_RTPWRF, tablert, 0);
      putcode(TASSIGN);
      putcode(acodelocphs);
      putcode(tablert);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE, tablert, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   decr(v12);

   /*-----------------------------------------------------+
   | turn on transmitter, then in a soft loop execute one |
   | slice, then set power and phase for the next slice   |
   | after the loop do the last slice, then switch rf off |
   +-----------------------------------------------------*/
   SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, ON, 0);
   loop(v12, v13);
      delay(plength - aptime);
      putcode(TASSIGN);
      putcode(acodelocpwr);
      putcode(tablert);
      SetRFChanAttr(RF_Channel[device], SET_RTPWRF, tablert, 0); 
      putcode(TASSIGN);
      putcode(acodelocphs);
      putcode(tablert);
      SetRFChanAttr(RF_Channel[device], SET_RTPHASE, tablert, 0);
      SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   endloop(v13);
   delay(plength);
   SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, OFF, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE, zero, 0);
   SetRFChanAttr(RF_Channel[device], SET_RTPHASE90, phs, 0);
   SetRFChanAttr(RF_Channel[device], SET_PWRF, (int) mod_factor, 0); 

   /*-----------------------------------------------+
   | perform rx2 delay, gate receiver back on again |
   +-----------------------------------------------*/
   if (rx2 - aptime > 0.0)
      delay(rx2 - aptime);
   if (newacq) {
      if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
   }
   else {
      if ( isBlankOn(OBSch) )
	   SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
   }
   if ( isBlankOn(device) )
      SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, ON, 0);
}
