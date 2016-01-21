/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Pbox_psg.c - Pbox (version 6.1D) include file for pulse sequence programming. 
                Appropriate parts of this file may be commented out in order to
                minimize the size of the compiled pulse sequences. For example,
                the triax PFG and 4-th Channel sections should be commented out 
                if the spectrometer has no such facilities.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Pbox 6.1D ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Changes : (+) new Pbox macros - opx(), pboxpar() pboxSpar(), pboxUpar(), setwave(),
                 putwave(), cpx(), pbox_shape(), pboxAshape();
             (+) FIRST_FID definition;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef UNIX
#include <sys/file.h>
#else
#define  F_OK   0
#define  X_OK   1
#define  W_OK   2
#define  R_OK   4
#endif

#include "oopc.h"
#include "group.h"
#include "rfconst.h"
#include "acqparms.h"
#include "aptable.h"
#include "apdelay.h"
#include "macros.h"
#include "Pbox_psg.h"
#include "vfilesys.h"
#include "abort.h"
#include "cps.h"

extern int DPSprint(double, const char *, ...);
extern int DPStimer(int code, int subcode, int vnum, int fnum, ...);
extern char userdir[];
extern char systemdir[];
extern int dps_flag;
extern int dpsTimer;
#ifdef NVPSG
extern void shapedgradient(char *name,double width,double amp,char which,
                      int loops,int wait_4_me);
#endif

int    ipx=0;    /* shape counter */
int    px_debug=0;
double pbox_pw=0, pbox_pwr=0, pbox_pwrf=0, pbox_dres=0, pbox_dmf=0;
char   pbox_name[MAXSTR], px_cmd[2048], px_opts[2048];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ Utility Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void fixname(char *nm)            /* remove the shapefile extension, if any */
{
int  i=0;

  while(nm[i] != '\0') i++;
  if(i<1)
  {
/*  text_message("fixname: filename is missing\n"); */
    return;
  }

  i--;
  while((nm[i] != '.') && (i > 0)) i--;

  if (i > 0)
  {
    if (nm[i+1] == 'R' && nm[i+2] == 'F' && nm[i+3] == '\0') 
      nm[i]='\0'; 
    else if (nm[i+1] == 'D' && nm[i+2] == 'E' && nm[i+3] == 'C' && nm[i+4] == '\0')
      nm[i]='\0';   
    else if (nm[i+1] == 'G' && nm[i+2] == 'R' && nm[i+3] == 'D' && nm[i+4] == '\0')
      nm[i]='\0'; 
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Channel 1 (xmtr) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void pbox_pulse(shape *rshape, codeint iph, double del1, double del2)		
{
  obspower(rshape->pwr);
  if (rshape->pwrf) obspwrf(rshape->pwrf);
  shaped_pulse(rshape->name, rshape->pw, iph, del1, del2);
}

void pbox_xmtron(shape *dshape)		
{
  obspower(dshape->pwr);
  if (dshape->pwrf) obspwrf(dshape->pwrf);
#ifndef NVPSG
  obsprgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
  xmtron();
#else
  xmtron();
  obsprgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
#endif
}

void pbox_xmtroff()		
{
#ifndef NVPSG
  xmtroff();
  obsprgoff();
#else 
  obsprgoff();
  xmtroff();
#endif
}

void pbox_spinlock(shape *dshape, double mixdel, codeint iph)
{
  int nloops;

  nloops = (int) (0.5 + mixdel/dshape->pw);
  if (nloops)
  {
    obspower(dshape->pwr);
    if (dshape->pwrf) obspwrf(dshape->pwrf);
    spinlock(dshape->name, 1.0/dshape->dmf, dshape->dres, iph, nloops); 
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Channel 2 (dec) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void pbox_decpulse(shape *rshape, codeint iph, double del1, double del2)
{
  decpower(rshape->pwr);
  if (rshape->pwrf) decpwrf(rshape->pwrf);
  decshaped_pulse(rshape->name, rshape->pw, iph, del1, del2);
}

void pbox_simpulse(shape *rshape, shape *rshape2,
                   codeint iph, codeint iph2, double del1, double del2)
{
  obspower(rshape->pwr);
  if (rshape->pwrf) obspwrf(rshape->pwrf);
  decpower(rshape2->pwr);
  if (rshape2->pwrf) decpwrf(rshape2->pwrf);
  simshaped_pulse(rshape->name, rshape2->name, rshape->pw, rshape2->pw, iph, iph2, del1, del2);
}

void pbox_decon(shape *dshape)		
{
  decpower(dshape->pwr);
  if (dshape->pwrf) decpwrf(dshape->pwrf);
#ifndef NVPSG
  decprgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
  decon();
#else
  decon();
  decprgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
#endif 
}

void pbox_decoff()		
{
#ifdef NVPSG
  decoff();
  decprgoff();
#else
  decprgoff();
  decoff();
#endif
}

void pbox_decspinlock(shape *dshape, double mixdel, codeint iph)
{
  int nloops;

  nloops = (int) (0.5 + mixdel/dshape->pw);
  if (nloops)
  {
    decpower(dshape->pwr);
    if (dshape->pwrf) decpwrf(dshape->pwrf);
    decspinlock(dshape->name, 1.0/dshape->dmf, dshape->dres, iph, nloops); 
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Channel 3 (dec2) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void pbox_dec2pulse(shape *rshape, codeint iph, double del1, double del2)
{
  dec2power(rshape->pwr);
  if (rshape->pwrf) dec2pwrf(rshape->pwrf);
  dec2shaped_pulse(rshape->name, rshape->pw, iph, del1, del2);
}

void pbox_sim3pulse(shape *rshape, shape *rshape2, shape *rshape3,
                    codeint iph, codeint iph2, codeint iph3,
                    double del1, double del2)
{
  obspower(rshape->pwr);
  if (rshape->pwrf) obspwrf(rshape->pwrf);
  decpower(rshape2->pwr);
  if (rshape2->pwrf) decpwrf(rshape2->pwrf);
  dec2power(rshape3->pwr);
  if (rshape3->pwrf) dec2pwrf(rshape3->pwrf);
  sim3shaped_pulse(rshape->name, rshape2->name, rshape3->name, rshape->pw, 
                   rshape2->pw, rshape3->pw, iph, iph2, iph3, del1, del2);
}

void pbox_dec2on(shape *dshape)		
{
  dec2power(dshape->pwr);
  if (dshape->pwrf) dec2pwrf(dshape->pwrf);
#ifndef NVPSG
  dec2prgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
  dec2on();
#else
  dec2on();
  dec2prgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
#endif
}

void pbox_dec2off()		
{
#ifndef NVPSG
  dec2off();
  dec2prgoff();
#else
  dec2prgoff();
  dec2off();
#endif
}

void pbox_dec2spinlock(shape *dshape, double mixdel, codeint iph)
{
  int nloops;

  nloops = (int) (0.5 + mixdel/dshape->pw);
  if (nloops)
  {
    dec2power(dshape->pwr);
    if (dshape->pwrf) dec2pwrf(dshape->pwrf);
    dec2spinlock(dshape->name, 1.0/dshape->dmf, dshape->dres, iph, nloops); 
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Channel 4 (dec3) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void pbox_dec3pulse(shape *rshape, codeint iph, double del1, double del2)
{
  dec3power(rshape->pwr);
  if (rshape->pwrf) dec3pwrf(rshape->pwrf);
  dec3shaped_pulse(rshape->name, rshape->pw, iph, del1, del2);
}

/*
void pbox_sim4pulse(rshape, rshape2, rshape3, rshape4, iph, iph2, iph3, iph4, del1, del2)
shape *rshape, *rshape2, *rshape3, *rshape4;
codeint iph, iph2, iph3, iph4; 
double del1, del2;
{
  obspower(rshape->pwr);
  if (rshape->pwrf) obspwrf(rshape->pwrf);
  decpower(rshape2->pwr);
  if (rshape2->pwrf) decpwrf(rshape2->pwrf);
  dec2power(rshape3->pwr);
  if (rshape3->pwrf) dec2pwrf(rshape3->pwrf);
  dec3power(rshape4->pwr);
  if (rshape4->pwrf) dec3pwrf(rshape4->pwrf);
  sim4shaped_pulse(rshape->name, rshape2->name, rshape3->name, rshape4->name, rshape->pw, 
             rshape2->pw, rshape3->pw, rshape4->pw, iph, iph2, iph3, iph4, del1, del2);
}
*/

void pbox_dec3on(shape *dshape)		
{
  dec3power(dshape->pwr);
  if (dshape->pwrf) dec3pwrf(dshape->pwrf);
#ifndef NVPSG
  dec3prgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
  dec3on();
#else
  dec3on();
  dec3prgon(dshape->name, 1.0/dshape->dmf, dshape->dres); 
#endif
}

void pbox_dec3off()		
{
#ifndef NVPSG
  dec3off();
  dec3prgoff();
#else
  dec3prgoff();
  dec3off();
#endif
}

void pbox_dec3spinlock(shape *dshape, double mixdel, codeint iph)
{
  int nloops;

  nloops = (int) (0.5 + mixdel/dshape->pw);
  if (nloops)
  {
    dec3power(dshape->pwr);
    if (dshape->pwrf) dec3pwrf(dshape->pwrf);
    dec3spinlock(dshape->name, 1.0/dshape->dmf, dshape->dres, iph, nloops); 
  }
}


/* ~~~~~~~~~~~~~~~~~~~ Shaped Gradients : DPS functions ~~~~~~~~~~~~~~~~~~~~*/

static void x_pbox_grad(shape *gshape, double glvl, double gof1, double gof2)
{
  DPSprint(gof1, "10 delay  1 0 1 gof1 %.9f \n", (float)(gof1));
  DPSprint(gshape->pw, "76 shapedgradient  12 2 2  ?%c  ?%s gshape->np %d 1 %d gshape->pw %.9f glvl %.9f \n", 'Z', gshape->name, (int)(gshape->np), (int)(1), (float)(gshape->pw), (float)(glvl));
  DPSprint(gof2, "10 delay  1 0 1 gof2 %.9f \n", (float)(gof2));
}

static void x_pbox_xgrad(shape *gshape, double gw,
                         double glvl, double gof1, double gof2)
{
  DPSprint(gof1, "10 delay  1 0 1 gof1 %.9f \n", (float)(gof1));
  DPSprint(gw, "76 shapedgradient  12 2 2  ?%c  ?%s gshape->np %d 1 %d gw %.9f glvl %.9f \n", 'X', gshape->name, (int)(gshape->np), (int)(1), (float)(gw), (float)(glvl));
  DPSprint(gof2, "10 delay  1 0 1 gof2 %.9f \n", (float)(gof2));
}

static void x_pbox_ygrad(shape *gshape, double gw,
                         double glvl, double gof1, double gof2)
{
  DPSprint(gof1, "10 delay  1 0 1 gof1 %.9f \n", (float)(gof1));
  DPSprint(gw, "76 shapedgradient  12 2 2  ?%c  ?%s gshape->np %d 1 %d gw %.9f glvl %.9f \n", 'Y', gshape->name, (int)(gshape->np), (int)(1), (float)(gw), (float)(glvl));
  DPSprint(gof2, "10 delay  1 0 1 gof2 %.9f \n", (float)(gof2));
}

static void x_pbox_zgrad(shape *gshape, double gw,
                         double glvl, double gof1, double gof2)
{
  DPSprint(gof1, "10 delay  1 0 1 gof1 %.9f \n", (float)(gof1));
  DPSprint(gw, "76 shapedgradient  12 2 2  ?%c  ?%s gshape->np %d 1 %d gw %.9f glvl %.9f \n", 'Z', gshape->name, (int)(gshape->np), (int)(1), (float)(gw), (float)(glvl));
  DPSprint(gof2, "10 delay  1 0 1 gof2 %.9f \n", (float)(gof2));
}

static void t_pbox_grad(shape *gshape, double glvl, double gof1, double gof2)
{
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof1);
  DPStimer(76,0,2,1,(int)gshape->np,(int)1,0,0 ,(double)gshape->pw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof2);
}

static void t_pbox_xgrad(shape *gshape, double gw,
                         double glvl, double gof1, double gof2)
{
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof1);
  DPStimer(76,0,2,1,(int)gshape->np,(int)1,0,0 ,(double)gw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof2);
}

static void t_pbox_ygrad(shape *gshape, double gw,
                         double glvl, double gof1, double gof2)
{
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof1);
  DPStimer(76,0,2,1,(int)gshape->np,(int)1,0,0 ,(double)gw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof2);
}

static void t_pbox_zgrad(shape *gshape, double gw,
                         double glvl, double gof1, double gof2)
{
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof1);
  DPStimer(76,0,2,1,(int)gshape->np,(int)1,0,0 ,(double)gw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gof2);
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Shaped Gradients ~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* experiments with shaped gradients on spectrometers with no gradient WFG. */

static double grd[MAXGSTEPS];
				      /* modified shaped gradient statement */
void shaped_gradient(int nstp, double glvl, double gw, char axis) 
{
   int i;

   gw /= (double) nstp;
   gw -= GRADIENT_DELAY;
   if (gw < 5.0e-6)
   {
     text_error("Pbox_psg, shaped_gradient : steps too short !\n");
     text_error("Use fewer steps in the gradient shapefile or longer pulse\n");
     exit(1);
   }

   if (dps_flag)
   {
     if (dpsTimer == 0)
     {
       DPSprint(0.0, "57 rgradient  11 0 3  ?%c glvl %.9f  0 0.0 0 0.0 \n", axis, (float)(glvl));
       DPSprint(gw, "10 delay  1 0 1 gw %.9f \n", (float)(gw) * (float)(nstp));
     }
     else
     {
       DPStimer(10,0,0,1,0,0,0,0 ,(double)gw * (double)(nstp));
       DPStimer(57,0,0,0);
     }
     return;
   }

   for (i = 0; i< nstp; i++)
   {
     rgradient(axis, glvl*grd[i]); 
     delay(gw);
   }
}

shape getGsh(char *shname)			/* read .GRD shaped gradient */
{
  shape    gshape;
  FILE     *inpf = NULL;
  int      j, k, nn;
  char     ch, str[MAXSTR];
  char     shapename[MAXSTR];
  double   am, ln, mxl = 32768.0;

  gshape.np = 0, gshape.ok = 0;

  (void) sprintf(gshape.name, "%s" , shname);
  fixname(gshape.name);
  (void) sprintf(shapename, "%s.GRD" , gshape.name);
  if (appdirFind(shapename, "shapelib", str, "", R_OK))
  {
     if ((inpf = fopen(str, "r")) == NULL)
     {
        abort_message("Pbox_psg, getGsh : Cannot find \"%s\"\n", shapename);
     }
  }
  else
  {
     abort_message("Pbox_psg, getGsh : Cannot find \"%s\"\n", shapename);
  }

  j = fscanf(inpf, "%c %s %lf %d %lf\n", &ch, str, &ln, &k, &am);
  if ((j == 5) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
      (str[2] == 'o') && (str[3] == 'x'))
  {
     gshape.pw = ln;					 /* get pw */
     gshape.af = am;
     gshape.ok = j-2;
  }
  else
    gshape.pw = 0.0;

  fseek(inpf, 0, 0);
  while ((getc(inpf)) == '#') fgets(str, MAXSTR, inpf);  /* ignore com-s */
  k = ftell(inpf); fseek(inpf, k-1, 0);

  j = 0; nn = 0;
  while (((k = fscanf(inpf, "%lf %lf\n", &am, &ln)) > 0) && 
         ((nn += (int) (ln + 0.1)) < MAXGSTEPS))
  {
  for (k = 0; k < ln; k++, j++) grd[j] = am;   
  }

  fclose(inpf);			/* close the INfile */

  gshape.np = nn; 
  if (nn > MAXGSTEPS) 
  {
    abort_message("Number of steps in %s exceeds the limit of %d. Aborting...\n", 
            gshape.name, MAXGSTEPS); 
  }

  for (j = 0; j< nn; j++) grd[j] /= mxl; 

  return gshape; 
}

shape getGshape(char *shname)          /* retrieve parameters from .RF file header */
{
  char     gsh[MAXSTR];

  getstr(shname, gsh);

  return getGsh(gsh); 
}

/* default (z) gradient */
void pbox_grad(shape *gshape, double glvl, double gof1, double gof2)
{
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_pbox_grad(gshape, glvl, gof1, gof2);
    else
      t_pbox_grad(gshape, glvl, gof1, gof2);
    return;
  }
  delay(gof1);
  shapedgradient(gshape->name, gshape->pw, glvl, 'Z', gshape->np, 1);
  delay(gof2);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~ Triax Shaped Gradients ~~~~~~~~~~~~~~~~~~~~~~~~~*/

void pbox_xgrad(shape *gshape, double gw, double glvl,
                double gof1, double gof2)
{
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_pbox_xgrad(gshape, gw, glvl, gof1, gof2);
    else
      t_pbox_xgrad(gshape, gw, glvl, gof1, gof2);
    return;
  }
  delay(gof1);
  shapedgradient(gshape->name, gw, glvl, 'X', gshape->np, 1);
  delay(gof2);
}

void pbox_ygrad(shape *gshape, double gw, double glvl,
                double gof1, double gof2)
{
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_pbox_ygrad(gshape, gw, glvl, gof1, gof2);
    else
      t_pbox_ygrad(gshape, gw, glvl, gof1, gof2);
    return;
  }
  delay(gof1);
  shapedgradient(gshape->name, gw, glvl, 'Y', gshape->np, 1);
  delay(gof2);
}

void pbox_zgrad(shape *gshape, double gw, double glvl,
                double gof1, double gof2)
{
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_pbox_zgrad(gshape, gw, glvl, gof1, gof2);
    else
      t_pbox_zgrad(gshape, gw, glvl, gof1, gof2);
    return;
  }
  delay(gof1);
  shapedgradient(gshape->name, gw, glvl, 'Z', gshape->np, 1);
  delay(gof2);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ Utility Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~ */

shape getRsh(char *shname)    /* retrieve parameters from .RF file header */
{
  shape    rshape;
  FILE     *inpf;
  int      j, ok=1;
  char     ch, str[MAXSTR];
  char     shapename[MAXSTR];

  rshape.ok = 0;
  rshape.pw = 0.0; rshape.pwr = 0.0; rshape.pwrf = 0.0; rshape.dres = 0.0;
  rshape.dmf = 0,0; rshape.dcyc = 0.0; rshape.B1max = 0.0;
  (void) sprintf(rshape.name, "%s" , shname);
  fixname(rshape.name);

  (void) sprintf(shapename, "%s.RF" , rshape.name);
  if (appdirFind(shapename, "shapelib", str, "", R_OK))
  {
     if ((inpf = fopen(str, "r")) == NULL)
     {
        ok = 0;
        if (strcmp(shname,"") != 0)
          text_error("Pbox_psg, getRsh : Cannot find \"%s\"\n", str);
     }
  }
  else
  {
     ok = 0;
     if (strcmp(shname,"") != 0)
        text_error("Pbox_psg, getRsh : Cannot find \"%s\"\n", str);
  }

  if (ok > 0)
  {
    j = fscanf(inpf, "%c %s %lf %lf %lf %lf %lf %lf %lf\n", &ch, str, &rshape.pw, &rshape.pwr,  
            &rshape.pwrf, &rshape.dres, &rshape.dmf, &rshape.dcyc, &rshape.B1max);
    fclose(inpf);

    if ((j > 4) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
        (str[2] == 'o') && (str[3] == 'x') && (rshape.pwr < 63.1))
    {
      rshape.pw /= 1000000.0;
      rshape.ok = j-2;	   /* number of parameters retrieved successfully */
    }
    else
      rshape.pw = 0.0, rshape.pwr = 0.0, rshape.pwrf = 0.0;
  }

  return rshape; 
}


shape getDsh(char *shname)	/* retrieve parameters from .DEC file header */
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

shape getRshape(char *shname)    /* retrieve parameters from .RF file header */
{
  char     rsh[MAXSTR];

  getstr(shname, rsh);

  return getRsh(rsh); 
}

shape getDshape(char *shname)    /* retrieve parameters from .RF file header */
{
  char     dsh[MAXSTR];

  getstr(shname, dsh);

  return getDsh(dsh); 
}


void pbox_get()          /* retrieve parameters from pbox.cal file */
{
  FILE     *inpf = NULL;
  int      j;
  char     str[MAXSTR];

  if (appdirFind("pbox.cal", "shapelib", str, "", R_OK))
  {
     if ((inpf = fopen(str, "r")) == NULL)
     {
        abort_message("Pbox_psg, pbox_get : Cannot find \"%s\"\n", str);
     }
  }
  else
  {
     abort_message("Pbox_psg, pbox_get : Cannot find \"%s\"\n", str);
  }
  j = fscanf(inpf, "%s %lf %lf %lf %lf %lf\n", pbox_name, &pbox_pw, 
             &pbox_pwr, &pbox_pwrf, &pbox_dres, &pbox_dmf);
  fclose(inpf);

  if ((j > 5) && (pbox_pwr < 63.1))
    pbox_pw /= 1000000.0;
  else
    pbox_pw = 0.0, pbox_pwr = 0.0, pbox_pwrf = 0.0;

  return; 
}

void setlimit(const char *name, double param, double limt)
{
    if( param > limt)  	{
       text_error("Parameter %s exceeds the limit of %f! Aborting...\n", 
       name, limt);
       psg_abort(1);	}
}


/* ~~~~~~~~~~~~~~~~~~~~~ Miscellaneous Functions ~~~~~~~~~~~~~~~~~~~~~~ */

/* composite dps elements */

static void x_pfg_pulse(double glvl, double gw, double gof1, double gof2)
{
   DPSprint(gof1, "10 delay  1 0 1 gof1 %.9f \n", (float)(gof1));
   DPSprint(0.0, "57 rgradient  11 0 3  ?%c glvl %.9f  0 0.0 0 0.0 \n", 'z', (float)(glvl));
   DPSprint(gw, "10 delay  1 0 1 gw %.9f \n", (float)(gw));
   DPSprint(0.0, "57 rgradient  11 0 3  ?%c 0.0 %.9f  0 0.0 0 0.0 \n", 'z', (float)(0.0));
   DPSprint(gof2, "10 delay  1 0 1 gof2 %.9f \n", (float)(gof2));
}

static void t_pfg_pulse(double glvl, double gw, double gof1, double gof2)
{
   DPStimer(10,0,0,1,0,0,0,0 ,(double)gof1);
   DPStimer(57,0,0,0);
   DPStimer(10,0,0,1,0,0,0,0 ,(double)gw);
   DPStimer(57,0,0,0);
   DPStimer(10,0,0,1,0,0,0,0 ,(double)gof2);
}

static void x_homodec(shape *decseq)
{
  double  acqdel, decdel;

  if(decseq->dcyc < 0.001)
    text_error("WARNING : low dutycycle - %.4f !!!\n", decseq->dcyc);
  decdel = decseq->dcyc/sw;
  acqdel = (1.0-decseq->dcyc)/sw;

  DPSprint(0.0, "60 obspower  1 0 1 decseq->pwr %.9f \n", (float)(decseq->pwr));
  if (decseq->pwrf)
    DPSprint(0.0, "61 obspwrf  1 0 1 decseq->pwrf %.9f \n", (float)(decseq->pwrf));
  DPSprint(alfa+1.0/fb-(eventovrhead(11))-(eventovrhead(12)), "10 delay  1 0 1 alfa+1.0/fb-(eventovrhead(11))-(eventovrhead(12)) %.9f \n", (float)(alfa+1.0/fb-(eventovrhead(11))-(eventovrhead(12))));
  DPSprint(0.0, "41 txphase  1 1 0 zero %d \n", (int)(zero));
  DPSprint(0.0, "46 obsprgon  1 0 3  ?%s  %.9f 1.0/decseq->dmf %.9f decseq->dres %.9f \n", decseq->name, (float)(1.0/decseq->dmf), (float)(1.0/decseq->dmf), (float)(decseq->dres));
  DPSprint(0.0, "74 initval  0 1 1 v14 %d np/2.0 %.9f \n", (int)(v14), (float)(np/2.0));
  DPSprint(0.0, "21 starthardloop  1 1 0 v14 %d \n", (int)(v14));
    DPSprint(acqdel, "1 acquire  1 0 2 2.0 %.9f acqdel %.9f \n", (float)(2.0), (float)(acqdel));
    DPSprint(0.0, "53 rcvroff  1 2 0 off 0 1 1 \n"); DPSprint(0.0, "11 xmtron  1 1 0 on 1 \n");
    DPSprint(decdel, "10 delay  1 0 1 decdel %.9f \n", (float)(decdel)); DPSprint(0.0, "11 xmtroff  1 1 0 off 0 \n"); DPSprint(0.0, "53 rcvron  1 2 0 on 1 1 1 \n");
  DPSprint(0.0, "14 endhardloop  1 0 0 \n");
  DPSprint(0.0, "45 obsprgoff  1 1 0 off 0 \n");
}

static void t_homodec(shape *decseq)
{
  double  acqdel, decdel;

  if(decseq->dcyc < 0.001)
    text_error("WARNING : low dutycycle - %.4f !!!\n", decseq->dcyc);
  decdel = decseq->dcyc/sw;
  acqdel = (1.0-decseq->dcyc)/sw;

  DPStimer(60,0,0,0);
  if (decseq->pwrf)
    DPStimer(61,0,0,0);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)alfa+1.0/fb-(eventovrhead(11))-(eventovrhead(12)));
  DPStimer(41,0,0,0);
  DPStimer(46,0,0,0);
  DPStimer(74,0,1,1,(int)v14,0,0,0 ,(double)np/2.0);
  DPStimer(21,0,1,0,(int)v14,0,0,0);
    DPStimer(1,0,0,2,0,0,0,0 ,(double)2.0,(double)acqdel);
    DPStimer(53,0,0,0); DPStimer(11,0,0,0);
    DPStimer(10,0,0,1,0,0,0,0 ,(double)decdel); DPStimer(11,0,0,0); DPStimer(53,0,0,0);
  DPStimer(14,0,1,0,(int)0,0,0,0);
  DPStimer(45,0,0,0);
}

static void x_pre_sat()
{
  if (satmode[0] == 'y')
  {
    if (d1 - satdly > 0)
      DPSprint(d1-satdly, "10 delay  1 0 1 d1-satdly %.9f \n", (float)(d1-satdly));
    else
      DPSprint(0.02, "10 delay  1 0 1 0.02 %.9f \n", (float)(0.02));
    DPSprint(0.0, "60 obspower  1 0 1 satpwr %.9f \n", (float)(satpwr));
    if (satfrq != tof)
      DPSprint(0.0, "33 obsoffset  1 0 1 satfrq %.9f \n", (float)(satfrq));
    DPSprint(satdly, "48 rgpulse  1 0 1 1 rof1 %.9f rof1 %.9f  1 zero %d satdly %.9f \n", (float)(rof1), (float)(rof1), (int)(zero), (float)(satdly));
    if (satfrq != tof)
      DPSprint(0.0, "33 obsoffset  1 0 1 tof %.9f \n", (float)(tof));
   }
   else
     DPSprint(d1, "10 delay  1 0 1 d1 %.9f \n", (float)(d1));
}

static void t_pre_sat()
{
  if (satmode[0] == 'y')
  {
    if (d1 - satdly > 0)
      DPStimer(10,0,0,1,0,0,0,0 ,(double)d1-satdly);
    else
      DPStimer(10,0,0,1,0,0,0,0 ,(double)0.02);
    DPStimer(60,0,0,0);
    if (satfrq != tof)
      DPStimer(33,0,0,0);
    DPStimer(48,0,0,3,0,0,0,0 ,(double)satdly,(double)rof1,(double)rof1);
    if (satfrq != tof)
      DPStimer(33,0,0,0);
   }
   else
     DPStimer(10,0,0,1,0,0,0,0 ,(double)d1);
}

static void x_presat()
{
  if ((satmode[0] == 'y') && (satdly > 1.0e-5))
  {
    DPSprint(0.0, "60 obspower  1 0 1 satpwr %.9f \n", (float)(satpwr));
    if (d1 > satdly)
      DPSprint(d1-satdly, "10 delay  1 0 1 d1-satdly %.9f \n", (float)(d1-satdly));
    else
      DPSprint(d1, "10 delay  1 0 1 d1 %.9f \n", (float)(d1));
    DPSprint(satdly, "48 rgpulse  1 0 1 1 rof1 %.9f rof1 %.9f  1 zero %d satdly %.9f \n", (float)(rof1), (float)(rof1), (int)(zero), (float)(satdly));
  }
  else
    DPSprint(d1, "10 delay  1 0 1 d1 %.9f \n", (float)(d1));
}

static void t_presat()
{
  if ((satmode[0] == 'y') && (satdly > 1.0e-5))
  {
    DPStimer(60,0,0,0);
    if (d1 > satdly)
      DPStimer(10,0,0,1,0,0,0,0 ,(double)d1-satdly);
    else
      DPStimer(10,0,0,1,0,0,0,0 ,(double)d1);
    DPStimer(48,0,0,3,0,0,0,0 ,(double)satdly,(double)rof1,(double)rof1);
  }
  else
    DPStimer(10,0,0,1,0,0,0,0 ,(double)d1);
}


/* composite executable elements */

void pfg_pulse(double glvl, double gw, double gof1, double gof2)
{
   if (dps_flag)
   {
     if (dpsTimer == 0)
       x_pfg_pulse(glvl, gw, gof1, gof2);
     else
       t_pfg_pulse(glvl, gw, gof1, gof2);
     return;
   }

   delay(gof1);
   rgradient('z',glvl);
   delay(gw);
   rgradient('z',0.0);
   delay(gof2);
}

void homodec(shape *decseq)
{
  double  acqdel, decdel;
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_homodec(decseq);
    else
      t_homodec(decseq);
    return;
  }

  if(decseq->dcyc < 0.001)
    text_error("WARNING : low dutycycle - %.4f !!!\n", decseq->dcyc);
  decdel = decseq->dcyc/sw;          
  acqdel = (1.0-decseq->dcyc)/sw;

  obspower(decseq->pwr);
  if (decseq->pwrf) 
    obspwrf(decseq->pwrf);
  delay(alfa + 1.0/fb - PRG_START_DELAY - PRG_STOP_DELAY);
  txphase(zero);
  obsprgon(decseq->name, 1.0/decseq->dmf, decseq->dres);
  initval(np/2.0, v14);
  starthardloop(v14);
    acquire(2.0, acqdel);          /* explicit acquisition */
    rcvroff(); xmtron();
    delay(decdel); xmtroff(); rcvron();  
  endhardloop();
  obsprgoff(); 
}

void pre_sat() /* use satfrq */
{
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_pre_sat();
    else
      t_pre_sat();
    return;
  }

  if (satmode[0] == 'y')
  {
    if (d1 - satdly > 0)
      delay(d1 - satdly);
    else
      delay(0.02);
    obspower(satpwr);
    if (satfrq != tof)
      obsoffset(satfrq);
    rgpulse(satdly,zero,rof1,rof1);
    if (satfrq != tof)
      obsoffset(tof);
  }
  else
    delay(d1);
}

void presat() /* use tof instead of satfrq */
{
  setlimit("satpwr", satpwr, 10.0);
  if (dps_flag)
  {
    if (dpsTimer == 0)
      x_presat();
    else
      t_presat();
    return;
  }

  if ((satmode[0] == 'y') && (satdly > 1.0e-5))
  {
    obspower(satpwr);
    if (d1 > satdly)
      delay(d1-satdly);
    else
      delay(d1);
    rgpulse(satdly, zero, rof1, rof1);
  }
  else
    delay(d1);
}


void new_name(char *name)
{
  getstr("seqfil", name);   
  sprintf(name, "%s_%d", name, ++ipx); 
  
  return; 
}


/* check whether the parameter is arrayed */
/* works with double arrays */
int isarry(char *arrpar)
{
  char array[MAXSTR];
  int i, j, k, m, n;

  getstr("array",array);

  if ((j = strlen(array)) < (n = strlen(arrpar))) return 0;
  else if (array[0] == '(') i=1, j--;
  else i=0;

  k=0; m=0;
  while(i < j)
  {
    while((i < j) && (arrpar[m] == array[i]))
      i++, m++;
    if((m == n) && ((i == j) || (array[i] == ','))) return 1;
    else
    {
      while((i < j) && (array[i] != ',')) i++;
    }
    m = 0; i++;

  }

  return 0;
}
/* One problem remains with isarry() : in the case of double arrays,
   for instance, array = 'pw,ofsX' it will generate pw*ofsX shapes
   and not ofsX shapes, as would be required */

/* ---------------------- Pbox macros -------------------------- */

void opx(char *name)  /* Syntax : opx(""); opx("xxx"); opx("xxx.RF"); opx(pwpat); */
{
  pbox_pw=0.0; pbox_pwr=0.0; pbox_pwrf=0.0; pbox_dres=0.0; pbox_dmf=0.0;
  (void) sprintf(pbox_name, "%s/shapelib/pbox.cal" , userdir);
  remove(pbox_name);
  pbox_name[0] = '\0';
  
  sprintf(px_cmd, "Pbox %s -u %s -w", name, userdir);
  sprintf(px_opts, "%s", "");

  return;
}


void setwave(char *wave)      /* Syntax : setwave("esnob 2.5m -12.4k"); */
{
  sprintf(px_cmd, "%s \"%s\" ", px_cmd, wave); 
  
  return; 
}


void putwave(char *sh, double bw, double ofs, double st,
             double pha, double fla)
{
  sprintf(px_cmd, "%s \"%s %.7f %.2f %.2f %.2f %.2f\" ", 
          px_cmd, sh, bw, ofs, st, pha, fla); 
  
  return; 
}


void pboxpar(char *pxname, double pxval) /* syntax: pboxpar("steps", 500.0) */
{
  sprintf(px_opts, "%s -%s %.3f", px_opts, pxname, pxval); 
  
  return; 
}


/* old style, to insure back-compatibility */
void pbox_par(char *pxname, char *pxval)  /* syntax: pbox_par("steps", "500") */
{
  sprintf(px_opts, "%s -%s %s", px_opts, pxname, pxval); 
  
  return; 
}


void pboxSpar(char *pxname, char *pxval) /* syntax: pboxSpar("steps", "500") */
{
  sprintf(px_opts, "%s -%s %s", px_opts, pxname, pxval); 
  
  return; 
}


/* e.g. pboxUpar("attn", 49, "d")  */
void pboxUpar(char *pxname, double pxval, char *pxunits)
{  
  sprintf(px_opts, "%s -%s %.3f%s", px_opts, pxname, pxval, pxunits); 
  
  return; 
}


void cpx(double rfpw90, double rfpwr)  /* syntax : cpx(ref_pw90, ref_pwr) */
{
  if(rfpw90 > 1.0)       /* check validity of calibration data */
  {
    text_error("cpx : incorrect dimension for pulse length : %.2f sec\n", rfpw90);
    exit(0);
  }
  sprintf(px_cmd, "%s -p %.0f -l %.2f", px_cmd, rfpwr, 1.0e6*rfpw90);  
  if (px_debug) text_message("%s%s\n", px_cmd, px_opts);
  
  system((char *)strcat(px_cmd,px_opts));       /* run Pbox */
  
  return;
}


shape pbox_inp(char *file_name, double rfpw90, double rfpwr)
{
  shape xsh;

  if(rfpw90 > 1.0)       /* check validity of calibration data */
  {
    printf("pbox_inp : incorrect dimension for pulse length : %.2f sec\n", rfpw90);
    exit(0);
  }
  sprintf(px_cmd, "Pbox -f %s -p %.0f -l %.2f", file_name, rfpwr, 1.0e6*rfpw90);
  if (px_debug) printf("%s\n", px_cmd);

  system(px_cmd);                                       /* run Pbox */
  (void) pbox_get();                        /* get shape parameters */

  xsh.pw = pbox_pw;
  xsh.pwr = pbox_pwr;
  xsh.pwrf = pbox_pwrf;
  xsh.dmf = pbox_dmf;
  xsh.dres = pbox_dres;
  strcpy(xsh.name, pbox_name);

  return xsh;
}


shape pbox_shape(char *shn, char *wvn, double pw_bw,
                 double ofs, double rf_pw90, double rf_pwr) 
{
  shape  sh;
  char   str[MAXSTR];
       
  sprintf(px_cmd, "Pbox %s -u %s -w \"%s %.7f %.1f\" ", shn, userdir, wvn, pw_bw, ofs);

  cpx(rf_pw90, rf_pwr);
  sh = getRsh(shn);

  if (sh.pwrf > 4095.0) 
  {
    sprintf(str, " -attn %.0f%c\n", rf_pwr, 'd');

    system((char *)strcat(px_cmd,str));                      

    sh = getRsh(shn);
    if (sh.pwrf > 4095.0) 
    {
      text_error("pbox_make : power error, pwrf = %.0f\n ", sh.pwrf);
      psg_abort(1);
    }
  }

  return sh;
}


shape pboxAshape(char *shn, char *wvn, double bw, double pw,
                 double ofs, double rf_pw90, double rf_pwr) 
{
  shape  sh;
  char   str[MAXSTR];
       
  sprintf(px_cmd, "Pbox %s -u %s -w \"%s %.1f/%.7f %.1f\" ", shn, userdir, wvn, bw, pw, ofs);

  cpx(rf_pw90, rf_pwr);
  sh = getRsh(shn);

  if (sh.pwrf > 4095.0) 
  {
    sprintf(str, " -attn %.0f%c\n", rf_pwr, 'd');

    system((char *)strcat(px_cmd,str));                      

    sh = getRsh(shn);
    if (sh.pwrf > 4095.0) 
    {
      text_error("pbox_make : power error, pwrf = %.0f\n ", sh.pwrf);
      psg_abort(1);
    }
  }

  return sh;
}

/* make mixing waveform */
shape  pbox_mix(char *shp, char *mixpat, double mixpwr,
                double refpw90, double refpwr)
{
  char  cmd[MAXSTR], repflg[MAXSTR], tmp[MAXSTR];
  double  reps=0.0;

  getstr("repflg", repflg);
  if (repflg[A] == 'y') reps = 1.0;

  strcpy(tmp, shp);

  sprintf(cmd, "Pbox %s -u %s -w %s -attn %.0fd -p %.0f -l %.2f -%.0f\n",
          tmp, userdir, mixpat, mixpwr, refpwr, 1e6*refpw90, reps);
  system(cmd);
  if(repflg[A] == 'y') text_message("  cmd : %s", cmd);
  return (getDsh(tmp));
}

/* make adiabatic 180 */
shape  pbox_ad180(char *shp, double refpw90, double refpwr)
{
  char  cmd[MAXSTR], repflg[MAXSTR], tmp[MAXSTR];

  getstr("repflg", repflg);

  strcpy(tmp, shp);

  sprintf(cmd, "Pbox %s -u %s -w \"cawurst-10 %.2f/%.7f\" -p %.0f -l %.2f\n",
              tmp, userdir, 1.0/refpw90, refpw90*20.0, refpwr, 1e6*refpw90);
  system(cmd);
  if (repflg[A] == 'y')
    text_message("  cmd : %s", cmd);

  return (getRsh(tmp));
}

/* make backwards adiabatic 180 */
shape  pbox_ad180b(char *shp, double refpw90, double refpwr)
{
  char  cmd[MAXSTR], repflg[MAXSTR], tmp[MAXSTR];

  getstr("repflg", repflg);

  strcpy(tmp, shp);

  sprintf(cmd, "Pbox %s -u %s -w \"cawurst-10 %.2f/%.7f\" -p %.0f -l %.2f\n",
              tmp, userdir, -1.0/refpw90, refpw90*20.0, refpwr, 1e6*refpw90);
  system(cmd);
  if (repflg[A] == 'y')
    text_message("  cmd : %s", cmd);

  return (getRsh(tmp));
}
