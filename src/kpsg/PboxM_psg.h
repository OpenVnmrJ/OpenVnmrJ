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
/* Pbox_psg.h - Pbox (version 6.1C) include file for pulse sequence programming
                on MERCURY systems with NO waveform generators. 
                Eriks Kupce, Varian Application laboratories, Oxford, 09.07.1999

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Pbox 6.1C ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Changes : (+) get_sh(), getRsh(), getDsh() and getGsh() statements added;
             (+) shaped gradient statements pbox_xgrad(), pbox_ygrad() and
                 pbox_zgrad() require gradient length, for example
                 pbox_xgrad(gshape, gw, glvl, gof1, gof2). The pbox_grad()
                 statement is still the same (no gradient length required);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifndef PBOXM_PSG_H
#define PBOXM_PSG_H

#include "Pbox_psg.h"

#ifndef DPS
#include <stdlib.h>
#include <sys/file.h>
#endif

#ifndef M_LN10
#define M_LN10  2.30258509299404568402
#endif

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#ifndef MAXSTR
#define MAXSTR 512
#endif

#ifndef GRADIENT_DELAY 
#define GRADIENT_DELAY 10.0e-6
#endif

#define MAXPWSTEPS 1024
#undef MAXGSTEPS
#define MAXGSTEPS 200
#define MINSLENGTH 0.2e-6 		/* minimum slice length in shaped pulse, us */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ Utility Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void fixname(char *nm)            /* remove the shapefile extension, if any */
{
int  i=0;

  while(nm[i] != '\0') i++;
  if(i<1)
  {
    printf("filename is missing\n");
    exit(1);
  }

  i--;
  while((nm[i] != '.') && (i > 0)) i--;

  if (i > 0)
  {
    if (nm[i+1] == 'R' && nm[i+2] == 'F' && nm[i+3] == '\0') 
      nm[i]='\0'; 
    else if (nm[i+1] == 'G' && nm[i+2] == 'R' && nm[i+3] == 'D' && nm[i+4] == '\0') 
      nm[i]='\0'; 
  }
}


shape getRsh(char *shname)          /* retrieve parameters from .RF file header */
{
  shape    rshape;
  FILE     *inpf;
  int      j, k;
  char     ch, str[MAXSTR];
  extern char userdir[], systemdir[];
  double   am, ln;

  rshape.ok = 0;
  (void) sprintf(rshape.name, "%s" , shname);
  fixname(rshape.name);
  (void) sprintf(str, "%s/shapelib/%s.RF" , userdir, rshape.name);
  if ((inpf = fopen(str, "r")) == NULL)
  {
    (void) sprintf(str, "%s/shapelib/%s.RF", systemdir, rshape.name);
    if ((inpf = fopen(str, "r")) == NULL)
    {
      printf("Pbox_psg, getRsh : Cannot find \"%s\"\n", str);
      exit(1);
    }
  }
  j = fscanf(inpf, "%c %s %lf %lf %lf %lf %lf %lf %lf\n", &ch, str, &rshape.pw,   
  &rshape.pwr, &rshape.pwrf, &rshape.dres, &rshape.dmf, &rshape.dcyc, &rshape.B1max);


  if ((j > 4) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
      (str[2] == 'o') && (str[3] == 'x'))
  {
    rshape.pw /= 1000000.0;
    rshape.ok = j-2;	   /* number of parameters retrieved successfully */
  }
  else
    rshape.pw = 0.0, rshape.pwr = 0.0, rshape.pwrf = 0.0;

  fseek(inpf, 0, 0);
  while ((getc(inpf)) == '#') fgets(str, MAXSTR, inpf);  /* ignore com-s */
  k = ftell(inpf); fseek(inpf, k-1, 0);

  j = 0;
  while (((k = fscanf(inpf, "%lf %lf %lf %lf\n", &am, &am, &ln, &am)) == 4) && 
         (j < MAXPWSTEPS))
    j += (int) (ln + 0.1);
  rshape.np=j;
  fclose(inpf);						/* close the INfile */
  return rshape; 
}

shape getDsh(char *shname)      /* retrieve parameters from .DEC file header */
{
  shape    dshape;
  FILE     *inpf;
  int      j, ok=1;
  char     ch, str[MAXSTR];
  char     shapename[MAXSTR];

  dshape.ok = 0;
  dshape.pw = 0.0, dshape.pwr = 0.0, dshape.pwrf = 0.0, dshape.dres = 0.0,
  dshape.dmf = 0.0, dshape.dcyc = 0.0, dshape.B1max = 0.0;
  (void) sprintf(dshape.name, "%s" , shname);
  fixname(dshape.name);

  (void) sprintf(shapename, "%s.DEC" , dshape.name);
  if (appdirFind(shapename, "shapelib", str, "", R_OK))
  {
     if ((inpf = fopen(str, "r")) == NULL)
     {
        ok = 0;
        if (strcmp(shname,"") != 0)
           text_error("Pbox_psg, getDsh : Cannot find \"%s\"\n", str);
     }
  }
  else
  {
     ok = 0;
     if (strcmp(shname,"") != 0)
        text_error("Pbox_psg, getDsh : Cannot find \"%s\"\n", str);
  }

  if (ok == 1)
  {
    j = fscanf(inpf, "%c %s %lf %lf %lf %lf %lf %lf %lf\n", &ch, str, &dshape.pw, &dshape.pwr,
            &dshape.pwrf, &dshape.dres, &dshape.dmf, &dshape.dcyc, &dshape.B1max);
    fclose(inpf);

    if ((j > 6) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') &&
        (str[2] == 'o') && (str[3] == 'x') && (dshape.pwr < 63.1))
    {
      dshape.pw /= 1000000.0;
      dshape.ok = j-2;
    }
    else
      dshape.pw = 0.0, dshape.pwr = 0.0, dshape.pwrf = 0.0, dshape.dres = 0.0,
      dshape.dmf = 0.0, dshape.dcyc = 0.0, dshape.B1max = 0.0;
  }

  return dshape;
}

shape getRshape(shname)          /* retrieve parameters from .RF file header */
char  *shname;
{
  char     rsh[MAXSTR];

  getstr(shname, rsh);

  return getRsh(rsh); 
}

static double amp[MAXPWSTEPS];		/* amplitude */
static double pha[MAXPWSTEPS];		/* phase     */

int setRshape(shname, pws)		/* read and set up .RF shaped pulse */
char  *shname;				/* for systems with no WFG and linear  */
double pws;				/* modulators, mainly for MERCURY */
{
  FILE     *inpf;
  int      j, k, nn;
  char     str[MAXSTR];
  double   am, ph, ln, gt, mxl, rd=M_PI/180.0;
  extern char userdir[], systemdir[];

  fixname(shname);
  (void) sprintf(str, "%s/shapelib/%s.RF" , userdir, shname);
  if ((inpf = fopen(str, "r")) == NULL)
  {
    (void) sprintf(str, "%s/shapelib/%s.RF", systemdir, shname);
    if ((inpf = fopen(str, "r")) == NULL)
    {
      printf("Pbox_psg, setRshape : Cannot find \"%s\"\n", str);
      exit(1);
    }
  }

  while ((getc(inpf)) == '#') fgets(str, MAXSTR, inpf);  /* ignore com-s */
  k = ftell(inpf); fseek(inpf, k-1, 0);

  j = 0; nn = 0; mxl = 0.0;
  while (((k = fscanf(inpf, "%lf %lf %lf %lf\n", &ph, &am, &ln, &gt)) == 4) && 
         ((nn += (int) (ln + 0.1)) < MAXPWSTEPS))
  {
    for (k = 0; k < ln; k++, j++) 
    {
      amp[j] = (am+=gt);
      pha[j] = ph*rd; 
    }  
    mxl = am > mxl ? am : mxl;				/* find max ampl */
  }
  fclose(inpf);						/* close the INfile */

  if (nn > MAXPWSTEPS) 
  {
    printf("Number of steps in %s exceeds the limit of %d. Aborting...\n", 
            shname, MAXPWSTEPS); 
    exit(1);
  }
  else if (nn < 2) 
  {
    printf("Shapefile %s : %d - unacceptable shapefile format. Aborting...\n", shname, k);
    exit(1);
  }

  am = pws / (double) nn;			/* slice width */
  ln = am / exp(0.05*M_LN10); 	                /* set max resolution */
  ln /= MINSLENGTH;

  if (ln < 16.0)				/* abort if not enough resolution */
  {
    printf("%s - Unable to execute with the existing hardware. Abort.\n", shname); 
    exit(1);
  }
  else if (ln < 64.0)
    printf("%s - WARNING ! Low dynamic range : %.0f\n", shname, ln); 

  for(j=0; j<nn; j++)
  {
    amp[j] = (int) (amp[j]*ln/mxl);		/* scale amplitudes */
    amp[j] *= MINSLENGTH;
  }
  return nn; 
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Channel 1 (xmtr) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void shaped_pulse(rfshape, pws, iph, del1, del2)		
char  *rfshape;
codeint iph; 
double pws, del1, del2;
{
  int i, nstp;
  double  slw;
  
  if (rfshape[0] == '\0')	/* for NULL string  do square pulse */
  {
    rcvroff(); obs_pw_ovr(TRUE); xmtroff();
    txphase(iph);
    if (del1 > 0.2e-6) delay(del1);
    xmtron();
    delay(pws);
    xmtroff();
    rcvron();  obs_pw_ovr(FALSE);
    if (del2 > 0.2e-6) delay(del2);
  }
  else
  {
    nstp = setRshape(rfshape, pws);
    slw = pws / (double) nstp; 	       /* slice width */
  
    rcvroff(); obs_pw_ovr(TRUE); xmtroff();
    if (del1 > 0.2e-6) delay(del1);
    for (i = 0; i < nstp-1; i+=2)
    {
      amp[i] *= cos(pha[i]);
      if (amp[i] < 0.0)
      {
        add(two,iph,v14);
        txphase(v14);
        amp[i] = -amp[i];
      }
      else
        txphase(iph);
      delay(slw - amp[i]);
      if (amp[i] > 0.1e-6) 
      {
        xmtron();
        delay(amp[i]);
        xmtroff();
      }

      amp[i+1] *= sin(pha[i+1]); 
      if (amp[i+1] < 0.0)
      {
        add(three,iph,v14);
        txphase(v14);
        amp[i+1] = -amp[i+1];
      }
      else
      {
        add(one,iph,v14);
        txphase(v14);
      }
      delay(slw - amp[i+1]);
      if (amp[i+1] > 0.1e-6) 
      {
        xmtron();
        delay(amp[i+1]);
        xmtroff();
      }
    }
    if (i < nstp)			/* do the last step if nstp is odd */
    {
      amp[i] *= cos(pha[i]);
      if (amp[i] < 0.0)
      {
        add(two,iph,v14);
        txphase(v14);
        amp[i] = -amp[i];
      }
      else
        txphase(iph);
      delay(slw - amp[i]);
      if (amp[i] > 0.1e-6) 
      {
        xmtron();
        delay(amp[i]);
        xmtroff();
      }
    }
    rcvron();  obs_pw_ovr(FALSE);
    if (del2 > 0.2e-6) delay(del2);
  }
}


void pbox_pulse(shape *rshape, codeint iph, double del1, double del2)
{
  obspower(rshape->pwr);
  shaped_pulse(rshape->name, rshape->pw, iph, del1, del2);
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Channel 2 (dec) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


void decshaped_pulse(char *rfshape, double pws,
                     codeint iph, double del1, double del2)		
{
  int i, nstp;
  double  slw;

  if (rfshape[0] == '\0')	/* for NULL string  do square pulse */
  {
    rcvroff(); dec_pw_ovr(TRUE); decoff();
    decphase(iph);
    if (del1 > 0.2e-6) delay(del1);
    decon();
    delay(pws);
    decoff();
    rcvron();  dec_pw_ovr(FALSE);
    if (del2 > 0.2e-6) delay(del2);
  }
  else
  {
    nstp = setRshape(rfshape, pws);
    slw = pws / (double) nstp; 	                     /* slice width */

    rcvroff(); dec_pw_ovr(TRUE); decoff();
    if (del1 > 0.2e-6) delay(del1);
    for (i = 0; i < nstp-1; i+=2)
    {
      amp[i] *= cos(pha[i]);
      if (amp[i] < 0.0)
      {
        add(two,iph,v14);
        decphase(v14);
        amp[i] = -amp[i];
      }
      else
        decphase(iph);
      delay(slw - amp[i]);
      if (amp[i] > 0.1e-6) 
      {
        decon();
        delay(amp[i]);
        decoff();
      }

      amp[i+1] *= sin(pha[i+1]); 
      if (amp[i+1] < 0.0)
      {
        add(three,iph,v14);
        decphase(v14);
        amp[i+1] = -amp[i+1];
      }
      else
      {
        add(one,iph,v14);
        decphase(v14);
      }
      delay(slw - amp[i+1]);
      if (amp[i+1] > 0.1e-6) 
      {
        decon();
        delay(amp[i+1]);
        decoff();
      }
    }
    if (i < nstp)		/* do the last step if nstp is odd */
    {
      amp[i] *= cos(pha[i]);
      if (amp[i] < 0.0)
      {
        add(two,iph,v14);
        decphase(v14);
        amp[i] = -amp[i];
      }
      else
        decphase(iph);
      delay(slw - amp[i]);
      if (amp[i] > 0.1e-6) 
      {
        decon();
        delay(amp[i]);
        decoff();
      }
    }
    rcvron();  dec_pw_ovr(FALSE);
    if (del2 > 0.2e-6) delay(del2);
  }
}


void pbox_decpulse(shape *rshape, codeint iph, double del1, double del2)
{
  decpower(rshape->pwr);
  decshaped_pulse(rshape->name, rshape->pw, iph, del1, del2);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Shaped Gradients ~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* experiments with shaped gradients on spectrometers with no gradient WFG. */

shape getGsh(char *shname)			/* read .GRD shaped gradient */
{
  shape    gshape;
  FILE     *inpf;
  int      j, k;
  char     ch, str[MAXSTR];
  double   am, ln;
  extern char userdir[], systemdir[];

  gshape.ok = 0;
  (void) sprintf(gshape.name, "%s" , shname);
  fixname(gshape.name);
  (void) sprintf(str, "%s/shapelib/%s.GRD" , userdir, gshape.name);
  if ((inpf = fopen(str, "r")) == NULL)
  {
    (void) sprintf(str, "%s/shapelib/%s.GRD", systemdir, gshape.name);
    if ((inpf = fopen(str, "r")) == NULL)
    {
      printf("Pbox_psg : Cannot find \"%s\"\n", str);
      exit(1);
    }
  }

  j = fscanf(inpf, "%c %s %lf\n", &ch, str, &ln);
  if ((j == 3) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
      (str[2] == 'o') && (str[3] == 'x'))
  {
     gshape.pw = ln;					 /* get pw */
     gshape.ok = j-2;
  }
  else
    gshape.pw = 0.0;
  fseek(inpf, 0, 0);

  while ((getc(inpf)) == '#') fgets(str, MAXSTR, inpf);  /* ignore com-s */
  k = ftell(inpf); fseek(inpf, k-1, 0);

  j = 0;
  while (((k = fscanf(inpf, "%lf %lf\n", &am, &ln)) > 0) && 
         (j < MAXGSTEPS))
  j += (int) (ln + 0.1);
  gshape.np = j; 
  fclose(inpf);			/* close the INfile */
  return gshape; 
}

shape getGshape(shname)          /* retrieve parameters from .RF file header */
char  *shname;
{
  char     gsh[MAXSTR];

  getstr(shname, gsh);

  return getGsh(gsh); 
}

int setGshape(shname)			/* read .GRD shaped gradient */
char  *shname;
{
  FILE     *inpf;
  int      j, k, nn;
  char     str[MAXSTR];
  double   am, ln=0.0, mxl = 0.0;
  extern char userdir[], systemdir[];

  fixname(shname);
  (void) sprintf(str, "%s/shapelib/%s.GRD" , userdir, shname);
  if ((inpf = fopen(str, "r")) == NULL)
  {
    (void) sprintf(str, "%s/shapelib/%s.GRD", systemdir, shname);
    if ((inpf = fopen(str, "r")) == NULL)
    {
      printf("Pbox_psg : Cannot find \"%s\"\n", str);
      exit(1);
    }
  }
  while ((getc(inpf)) == '#') fgets(str, MAXSTR, inpf);  /* ignore com-s */
  k = ftell(inpf); fseek(inpf, k-1, 0);

  j = 0; nn = 0;
  while (((k = fscanf(inpf, "%lf %lf\n", &am, &ln)) > 0) && 
         ((nn += (int) (ln + 0.1)) < MAXGSTEPS))
  {
    for (k = 0; k < ln; k++, j++) 
    {
      amp[j] = am;  
    }
    mxl = am > mxl ? am : mxl;			/* find max ampl */ 
  }
  fclose(inpf);					/* close the INfile */

  if (nn > MAXGSTEPS) 
  {
    printf("Number of steps in %s exceeds the limit of %d. Aborting...\n", 
            shname, MAXGSTEPS); 
    exit(1);
  }
  else if (nn < 2) 
  {
    printf("Shapefile %s : %d - unacceptable shapefile format. Aborting...\n", shname, k);
    exit(1);
  }

  for (j = 0; j< nn; j++) amp[j] /= mxl; 

  return nn; 
}

				      /* modified shaped gradient statement */
void shapedgradient(gname, gw, glvl, axis, nstp, dchr) 
char  *gname, *dchr, axis;
int    nstp;
double glvl, gw; 
{
   int i;

   nstp = setGshape(gname);
   gw /= (double) nstp;
   gw -= GRADIENT_DELAY;
   if (gw < 1.0e-6)
   {
     printf("Pbox_psg, shaped_gradient : steps too short !\n");
     printf("Use fewer steps in the gradient shapefile or longer pulse\n");
     exit(1);
   }

   for (i = 0; i< nstp; i++)
   {
     rgradient(axis, glvl*amp[i]); 
     delay(gw);
   }
}

void pbox_grad(shape *gshape,
               double glvl, double gof1, double gof2) /* default (z) gradient */
{
  delay(gof1);
  shapedgradient(gshape->name, gshape->pw, glvl, 'Z', 1, "NOWAIT");
  delay(gof2);
}

void pbox_zgrad(shape *gshape, double gw,
                double glvl, double gof1, double gof2)	/* z gradient */
{
  delay(gof1);
  shapedgradient(gshape->name, gw, glvl, 'Z', 1, "NOWAIT");
  delay(gof2);
}


/* ~~~~~~~~~~~~~~~~~~~~~ Miscallaneous Functions ~~~~~~~~~~~~~~~~~~~~~~ */

void pfg_pulse(glvl, gw, gof1, gof2)
double glvl, gw, gof1, gof2; 
{
   delay(gof1);
   rgradient('z',glvl);
   delay(gw);
   rgradient('z',0.0);
   delay(gof2);
}


void setlimit(const char *name, double param, double limt)
{
    if( param > limt)  	{
       abort_message("Parameter %s exceeds the limit of %f!\n", 
       name, limt);
       }
}


/*********************************
 *  Commented out presat() and pre_sat(). 
 *  Most Mercury-series pulsesequences declare
 *  satdly, satfrq, satpwr, etc, explicitely, 
 *  making this available globally would only 
 *  interfere at this point
 */
/* void presat()
/* {
/*   satdly = getval("satdly");
/* 
/*   if (satdly)
/*   {
/*     obspower(getval("satpwr"));
/*     rcvroff(); obs_pw_ovr(TRUE); xmtroff();
/*     delay(d1-satdly);
/*     xmtron(); delay(satdly); xmtroff();
/*     rcvron();  obs_pw_ovr(FALSE);
/*     delay(5.0e-6);
/*   }
/*   else
/*      delay(d1);
/* }
/* 
/*     
/* void pre_sat()
/* {
/*   satpwr = getval("satpwr");
/*   setlimit("satpwr", satpwr, 10.0);
/*   satfrq = getval("satfrq");
/*   satdly = getval("satdly");
/*     
/*   if ((satmode[0] == 'y') && (satdly > 1.0e-5))
/*   {
/*     rcvroff(); obs_pw_ovr(TRUE); xmtroff();
/*     obspower(satpwr); obsoffset(satfrq);
/*     if (d1 > satdly)
/*       delay(d1-satdly);
/*     else
/*       delay(d1);
/*     xmtron(); delay(satdly); xmtroff();
/*     rcvron();  obs_pw_ovr(FALSE);
/*     obsoffset(tof);
/*     delay(5.0e-6);
/*   }
/*   else
/*     delay(d1);
/* }
/* */
#endif
