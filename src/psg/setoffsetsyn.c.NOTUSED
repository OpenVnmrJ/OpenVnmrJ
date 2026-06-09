/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "rfconst.h"
#include "acqparms.h"
/*-----------------------------------------------------------------------
|
| double  setoffsetsyn()/3
|	calculate the offset synthesizer frequency of the old style RF
|	return calculated tbo or dbo (offset freq Hz)
|
|				Author Greg Brissey  6/16/86
|------------  NOTE ------------------------------------------------
| changed frequency calc, lock & solvent corrections are applied before 
|                         tof or dof are added. This correction is also 
|			  rounded up to the larger of the tof,dof stepsize
|			  prior to tof or dof addition.  Thus tof & dof are
|			  not adjusted & give control to the stepsize of that
|			  device. tbo & dbo are not used any more.
|					Greg Brissey  8/20/87
+----------------------------------------------------------------------*/
extern int bgflag;
extern char  rfband[];	/* lockfreq correction flag */
extern int  lkfreqflg;	/* lockfreq correction flag */
extern int curfifocount;
extern double lkfactor;
extern double solvfactor;
extern double sfrq;
extern double dfrq;
extern long   max_todostep;
extern char  ptsoff[];	/* NEW PTS type for trans & dec */

double
setoffsetsyn(specfreq,offset,device,setpts)
double specfreq;	/* base freq. (sfrq or dfrq) */
double offset;		/* freq offset (tof or dof) */
int    device;          /* DODEV or TODEV */
int    setpts;		/* PTS is set if TRUE */
{
    char type;
    int digit;
    int bcd;
    long ifreq;		/* long gives 10 digit accurcy */
    long baseFreq;	/* base freq */
    double freq;	/* double gives 16 digit accurcy */
    double baseMHz;	/* base freq, (i.e., tbo,dbo ) */
    double iffreq;
    long mainpts;	/* in unit of 100Khz to pass on to jerry	*/
    double pts;		/* PTS syn value */
    double sd_frq;
    int leiferflag;	/* you make it backwards -- you live in infamy */
    int sisptsflag;	/* */
    int setptsflag =1; /* debug	*/

#ifndef SIS
    extern double freqcorrect();
#endif

    type = rftype[device - 1];
    leiferflag = ((rftype[device - 1] == 'm') || (rftype[device - 1] == 'M'));

    sisptsflag = ((ptsoff[device - 1] == 'y') || (ptsoff[device - 1] == 'Y'));

    if (bgflag)
        fprintf(stderr,
        "setoffsetsyn: specfreq = %11.7lf(MHz), offset = %7.2lf(Hz),device=%d,setpts=%d\n",
	 specfreq,offset,device,setpts);
#ifndef SIS
    /*--- correct sfrq (dfrq) for lockfreq & solvent if necessary  ---*/
    specfreq = freqcorrect(specfreq);
#endif

    if (type != 'c')            /* normal broadband system */
    {
      if (type != 'a')
      {
	 if ( leiferflag )
	   iffreq = 20.5;	/* VIS's odd iffreq */
         else 
	 {
	   if (H1freq > 390)   /* instrument proton frequency, selects IF freq */
              iffreq = 205.5;
           else
              iffreq = 158.5;
         }
 
         baseMHz = 1.45; /* offset syn sweet spot in MHz */

         /* obtain exact pts value from SpecFreq*/
	 if ( !leiferflag )
             pts = iffreq + baseMHz - specfreq;
	 else {
	     offset = -offset;
	     baseMHz = 1.50;
             pts = iffreq + baseMHz + specfreq;	 
	     /* VIS is backwards & odd iffreq */ }

         if (bgflag)
 	 {
	   if ( !leiferflag )
             fprintf(stderr,
              " %11.7lf(pts) = %7.3lf(iffreq) + %11.7lf(base) - %11.7lf(specfreq)\n",
                pts,iffreq,baseMHz,specfreq);
	   else
             fprintf(stderr,
              " %11.7lf(pts) = %7.3lf(iffreq) + %11.7lf(base) + %11.7lf(specfreq)\n",
                pts,iffreq,baseMHz,specfreq);
         }

         if (pts < 0.0)   pts = -pts;              /* absolute value */

         /* round exact pts to 100KHz, PTS hardware good only to 100KHz */
         pts = (double) ((long) ((pts * 10.0) + .5)) / 10.0;
         if (bgflag)
            fprintf(stderr,"setoffsetsyn: %7.3lf(pts) rounded to 100KHz\n",pts);
         
	 if ( !leiferflag )
             baseMHz = specfreq - iffreq;     /* back calc new base freq */
         else
             baseMHz = (-specfreq) - iffreq;  /* back calc new base freq VIS */

         /* rfband High or Low ? */
         if ( ((rfband[device-1] == 'h') || (rfband[device-1] == 'H') ||
              (specfreq > (H1freq * 0.75))) && (!leiferflag))       
	      /* or freq > 75% h1freq  and not VIS */
             baseMHz -= pts;
         else
             baseMHz += pts;

         if (bgflag)
          fprintf(stderr,
          " %11.7lf(base) = (+/-)%11.7lf(SpecFreq) (+/-) %11.7lf(pts) - %6.2lf(iffrq)\n",
                   baseMHz,specfreq,pts,iffreq);
      }
      else    /* fix freqeuncy (works only for proton fix freq boards) */
       {
         if (H1freq < 210)   /* instrument proton frequency, selects IF freq for 'a' */
           iffreq = 198.5;
         else
           if (H1freq < 310)
             iffreq = 298.5;
           else
           if (H1freq < 410)
             iffreq = 398.5;
           else
	   {
             text_error("No fix frequency board for h1freq greater than 300");
	     abort(1);
	   }
               
         baseMHz = specfreq - iffreq;     /* back calc new base freq (no VIS type 'a'*/
         if (bgflag)
          fprintf(stderr,
           "setoffsetsyn: %11.7lf(base) = %11.7lf(SpecFreq) - %6.2lf(iffreq)\n",
                   baseMHz,specfreq,iffreq);
         pts = 0.0;	/* no PTS for type 'a' RF  */
      }
    }
    else
    {
	text_error("setoffsetsyn() called with a direct syn RF type");
	abort(1);
    }

    if (setpts && (pts != 0.0) && !sisptsflag)  /* gen apbus words to set PTS */
       setPTS(pts,device);

/*-------------------------------------------------------------------------*/
    baseFreq = (long) ((baseMHz * 1.0e7)+.005);
    /*set new tbo,dbo, convert to 1/10Hz */
    if (bgflag)
        fprintf(stderr,"setoffsetsyn: baseFreq = %ld (1/10Hz)\n",baseFreq);
 
    /* round baseMHz (tbo,dbo) to the larger of the tof & dof stepsize */
    if (max_todostep > 1L)
       baseFreq = ((baseFreq / max_todostep) * max_todostep)  +
                 (((baseFreq % max_todostep) >= (max_todostep / 2L)) ? 
			max_todostep : 0L);
 
    if (bgflag)
       fprintf(stderr,"setoffsetsyn:round to %ld(.1Hz), baseFreq= %ld(.1Hz)\n",
                   max_todostep,baseFreq);

    /* set new tbo,dbo, convert to 0.1Hz */
    baseMHz = ((double) baseFreq) + (offset * 10.0);


    ifreq = (long) (baseMHz + 0.45);	
    /* actually round this one! */
    /* eg. just to be sure 30.0 = 30 */

    if (bgflag)
    if ((setptsflag) && (bgflag))
	fprintf(stderr,
	"setoffsetsyn(): tbo = %11.2lf, tof= %7.2lf, freq=%11.7lf, ifreq=%ld\n",
		baseMHz,offset,specfreq,ifreq);

    if (!sisptsflag)
    {
    for (digit = 0; digit < 8; digit++)
    {
	bcd = ifreq % 10;
	bcd = 0x8000 | (device << DEVPOS) | (digit << DIGITPOS) | bcd;
	ifreq /= 10;
	putcode((codeint) bcd);
	curfifocount++;
      }
    }
    else 
     {
      if (bgflag)
      fprintf(stderr, "setSISPTS: pts = %11.71f, ifreq = %ld , device = %d, setpts = %d\n",
	      pts , ifreq, device, setpts);
      mainpts = (long)((pts*10.0) + 0.0005);
					/* units of 100KHz */
      setSISPTS(mainpts,ifreq,device,setpts);
    }
    return(baseMHz/10.0);
}

