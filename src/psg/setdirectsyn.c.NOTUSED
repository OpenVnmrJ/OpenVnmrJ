/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
/*-------------------------------------------------------------------
|
|	setdirectPTS()/3 
|	set PTS for new style RF scheme, direct synthesis
| Base (eg SFRQ or DFRQ are base synthesizer frequencies in mHz)
| Offset (eg TO or DO are in tenths of a Hz)
| Both values are broken down into two integer values by dividing by
| 256.						
| The base is also made into kHz, for example sfrq = 400.1 (mHz) 
| and to = 10101.5 Hz, Then the Base is changed into 1562 & 228	
| (1562 * 256 + 228 = 400,100kHz),  and TO into 39 & 1175
| (39 * 256 + 1175/10 = 10101.5 Hz)
| The same procedure is used for DFRQ and DO.
| In high band 500 or 400 w/o PTS500 freq = freq + 10.5 / 2
| However a VXR400 w PTS500 freq = freq + 10.5
|				Author Greg Brissey  6/16/86
|------------  NOTE ------------------------------------------------
| changed frequency calc, lock & solvent corrections are applied before 
|                         tof or dof are added. This correction is also 
|			  rounded up to the larger of the tof,dof stepsize
|			  prior to tof or dof addition.  Thus tof & dof are
|			  not adjusted & give control to the stepsize of that
|			  device. tbo & dbo are not used any more.
|					Greg Brissey  8/20/87
+------------------------------------------------------------------*/
extern char rfband[];	/* low or high band */
extern int  bgflag;	/* debug flag */
extern int  lkfreqflg;	/* lockfreq correction flag */
extern int  curfifocount;
extern int  H1freq;	/* instrunment proton frequency */
extern int  ptsval[];	/* PTS type for trans & dec */
extern double sfrq;
extern double dfrq;
extern double lkfactor;
extern double solvfactor;
extern long   max_todostep;

setdirectPTS(specfreq,offset,device)
double specfreq;	/* base freq.  sfrq or dfrq  (MHz) */	
double offset;		/* offset freq.  tof or dof (Hz) */
int device;		/* trans or decoupler */
{
#ifndef SIS
    extern  double freqcorrect();	/* SpecFreq in MHz */
#endif
    double frqdif;	/* frequency diff between sfrq & dfrq */
    long   freqMHz;	/* integer PTS freq in KHz */
    long   freqKHz;	/* integer PTS freq in KHz */
    long   freqtHz;	/* integer remaining PTS freq in 1/10 Hz */
    int    topKHz;	/* top byte of freqKHz */
    int    botKHz;	/* bottom byte of freqKHz */
    int    toptHz; 	/* top byte of freqtHz */
    int    bottHz;	/* bottom byte of freqtHz */
    int    mode;	/* rf mode, high or low band & special homo bit */

    device--; /* this proceduce uses 0 for transmitter and 1 for decoupler */

    if (bgflag)
       fprintf(stderr,
             "setdirectsyn: specfreq = %11.7lf(MHz), offset = %7.2lf (Hz)\n",
		specfreq,offset);
#ifndef SIS
    /*--- correct sfrq (dfrq) for lockfreq & solvent if necessary  ---*/
    specfreq = freqcorrect(specfreq);
#endif

    /*--- round freq to largest stepsize of tof & dof  ---*/
    freqMHz = (long) specfreq;      /* extract MHz digits */
    specfreq *= 1.0e7; /*  change units to 0.1 Hz */
    /* extract 100kHz- 0.1Hz */
    freqtHz = (long)  (specfreq - (((double) freqMHz) * 1.0e7));
    if (bgflag)
       fprintf(stderr,
       "setdirectsyn: freqMHz = %ld, freqtHz = %ld (0.1Hz)\n",freqMHz,freqtHz);
 
    /* Round SpecFreq to the larger of the tof & dof stepsize */
    if (max_todostep > 1L)
       freqtHz = ((freqtHz / max_todostep) * max_todostep) +
                 (((freqtHz % max_todostep) >= (max_todostep / 2L)) ? max_todostep : 0L);
    if (bgflag)
       fprintf(stderr,
	  "setdirectsyn: round to %ld (.1Hz), freqtHz = %ld (.1Hz)\n",
                   max_todostep,freqtHz);

    /* SpecFreq in Hz + offset (tof,dof) */
    specfreq =  ( ((double) freqMHz) * 1.0e6 /*Hz*/)
                    + (((double) freqtHz) / 10.0 /*Hz*/) + offset /*Hz*/;
 
    if (bgflag)
          fprintf(stderr,
             "setdirectsyn: %11.2lf(SpecFreq) = %11.2lf(SpecFreq) + %11.2lf(offset)\n",
                specfreq,( ((double) freqMHz) * 1.0e6 ) + (((double) freqtHz) / 10.0),
                offset);

    if (bgflag)
	fprintf(stderr,
	"setdirectPTS(): freq = %13.1lf, offset = %10.1lf device = %d\n",
      		specfreq,offset,device);

    if ( ((rfband[device] == 'h') || (rfband[device] == 'H')) && /* high band?*/
	 ( (H1freq > 450) || (ptsval[device] < 400) ) )
    {
	specfreq = (specfreq + 10.5e6) / 2.0;	/* high band */
    }
    else
    {
	specfreq = (specfreq + 10.5e6);		/* low band */
    }

    freqKHz = (long) ( (specfreq / 1000.0) + .0005);		/* freq KHz */
    freqtHz = (long) ( (specfreq - ((double) freqKHz * 1000.0)) * 10.0);/*.1Hz*/

    topKHz = (int) (freqKHz >> 8);
    botKHz = (int) (freqKHz & 0x00ff);
    toptHz = (int) (freqtHz >> 8);
    bottHz = (int) (freqtHz & 0x00ff);

    if (bgflag)
	fprintf(stderr,
	"setdirectPTS(): PTSfreq = %13.1lf, freqKHz = %ld, freqtHz = %ld \n",
	      specfreq,freqKHz,freqtHz);

    /* setup High or Low band and also the special heterodecoupler mode */
    /* bit0 0=High band 1=Low Band */
    /* bit1 0=not special HOMO Decoupling mode 1=It is. */
    mode = 0;
    frqdif = dfrq - sfrq;
    if (frqdif < 0.0) frqdif = -frqdif;  /* absolute value */
    if (frqdif < 1.0)
	mode |= 2;	/* decoupler mode bit ON */
    if (device == 0)    /* transmitter */
    {
        if ( (rfband[device] == 'l') || (rfband[device] == 'L') )
	    mode = 3;	/* low band set bit0 and bit1 to 1 */
	else
	    mode = 0;	/* no homodecoupling mode for transmitter */
    }
    else
    {
        if ( (rfband[device] == 'l') || (rfband[device] == 'L') )
	    mode |= 1;	/* low band set bit-0 to 1 */
    }
    if (bgflag)
	fprintf(stderr,
	   "setdirectPTS(): frqdif = %5.1lf, mode = %d\n",frqdif,mode);
    putcode(SETPTS);
    putcode((codeint) topKHz);
    putcode((codeint) botKHz);
    putcode((codeint) toptHz);
    putcode((codeint) bottHz);
    putcode((codeint) mode);
    putcode((codeint) device);
    if (ptsval[device] < 200) 	/* flag that pts160 is being used */
	putcode(1);
    else
	putcode(0);

    curfifocount += 9;	/* setting direct pts uses 9 apbus words */
}
