/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************/
/*								*/
/*  dsn	    -	display signal to noise ratio			*/
/*  dres    -	display linewidth of cursor selected line	*/
/*		dres(freq,peak_fraction) width at peak fraction */
/*  tempcal -   temperature calibration				*/
/*  h2cal   -   decoupler field calibration			*/
/*								*/
/****************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "data.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"
#include "vnmrsys.h"
#include "buttons.h"
#include "pvars.h"
#include "wjunk.h"

#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1

extern int debug1;
extern int  rel_spec();
extern void maxfloat(register float  *datapntr, register int npnt, register float  *max);
extern int currentindex();
extern void integ2(float *frompntr, int fpnt, int npnt, float *value, int offset);

/****************/
void calc_noise(float *datapntr, double rcrv, double lcrv, double *noise)
/****************/
{
  int    i,lc,rc,mc,npi;
  double hzpp,grad,dc,value;
  float  ar,al;

  hzpp = sw / (double) ((fn / 2) - 1);
  if (debug1)
    Wscrprintf("lcr = %g rcr = %g hzpp = %g\n",lcrv,rcrv,hzpp);
  lc = (sw - lcrv - rflrfp) / hzpp;
  rc = (sw - rcrv - rflrfp) / hzpp;
  mc = lc + ((rc - lc) / 2);
  npi = mc - lc + 1;
  if (debug1)
    Wscrprintf("lc = %d rc = %d mc = %d npi = %d\n",lc,rc,mc,npi);
  integ2(datapntr,lc,npi,&al,1);
  al /= (double) npi;
  npi = rc - mc + 1;
  integ2(datapntr,mc,npi,&ar,1);
  ar /= (double) npi;
  grad = 2.0 * (ar - al) / (double) (rc - lc + 1);
  dc = 0.5 * (3.0 * al - ar);
  npi = rc - lc + 1;
  *noise = 0.0;
  datapntr += lc;
  if (debug1)
    Wscrprintf("dc = %g grad = %g ar= %g al= %g\n",dc,grad,ar,al);
  for (i = 1; i <= npi; i++)
  {
    value = *datapntr++ - dc;
    *noise += value * value;
    dc += grad;
  }
  if (debug1)
    Wscrprintf("noise**2 = %g\n",*noise);
  *noise = 2.0 * sqrt(*noise / (double) npi);
}

/*************/
static int checkinput(int argc, char *argv[], double *rcr, double *lcr)
/*************/
{
  if (isReal(*++argv))
    *lcr = stringReal(*argv);
  else
  {
    Werrprintf("first argument must be the maximum frequency of the noise region");
    return(ERROR);
  }
  if ((argc>2) && (isReal(*++argv)))
    *rcr = stringReal(*argv);
  else
  {
    Werrprintf("second argument must be the minimum frequency of the noise region");
    return(ERROR);
  }
  if (*lcr < *rcr)
  {
    Werrprintf("argument 1 must be greater than argument 2");
    return(ERROR);
  }
  if (argc != 3)
  {
    Werrprintf("only two arguments may be supplied");
    return(ERROR);
  }
  return(COMPLETE);
}

/*************/
int dsn(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int ctrace;
  int dummy;
  float signal;
  double noise;
  float *spectrum;
  double rcrv,lcrv;

  Wturnoff_buttons();
  if(init2d(1,1)) return(ERROR);
  ctrace = currentindex();
  if ((spectrum = calc_spec(ctrace-1,0,FALSE,TRUE,&dummy))==0)
    return(ERROR);
  if (argc>1)
  {
    if (checkinput(argc,argv,&rcrv,&lcrv))
      return(ERROR);
    if ((lcrv > sw - rflrfp) || (lcrv < -rflrfp))
    {
      Werrprintf("maximum noise region frequency outside limits of %g to %g.",sw-rflrfp, 0.0 - rflrfp);
      return(ERROR);
    }
    if (rcrv < -rflrfp)
    {
      Werrprintf("minimum noise region frequency outside limits of %g to %g.",sw-rflrfp, 0.0 - rflrfp);
      return(ERROR);
    }
  }
  else
  {
    if (delta <= 0.0)
    {
      Werrprintf("noise region (parameter delta) must be greater than zero");
      return(ERROR);
    }
    rcrv = cr - delta;
    lcrv = cr;
    if ((lcrv > sw - rflrfp) || (lcrv < -rflrfp))
    {
      Werrprintf("maximum noise region frequency (cr) outside limits of %g to %g.",sw-rflrfp, 0.0 - rflrfp);
      return(ERROR);
    }
    if (rcrv < -rflrfp)
    {
      Werrprintf("minimum noise region frequency (cr-delta) outside limits of %g to %g.",sw-rflrfp, 0.0 - rflrfp);
      return(ERROR);
    }
  }
  if (debug1)
    Wscrprintf("lcr = %g rcr= %g\n",lcrv,rcrv);
  maxfloat(spectrum + fpnt,npnt,&signal);
  if (debug1)
    Wscrprintf("signal = %g\n",signal);
  calc_noise(spectrum,rcrv,lcrv,&noise);
  if (debug1)
    Wscrprintf("noise = %g\n",noise);
  if (retc==0)
  Winfoprintf("calc rms s/n ratio = %g",signal / noise);
  if (retc>0)
    retv[0] = realString(signal / noise);
  if (retc>1)
  {
    if (normflag)
      noise *= normalize;
    retv[1] = realString(noise);
  }
  appendvarlist("dummy");
  return(rel_spec());
}

#define SEARCH 10
#define MSEARCH 2
/*************/
int dres(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int ctrace;
  int dummy;
  int i;
  int msearch;
  float *spectrum,max,*ptr,peakfrac;
  double rcrv,lw,hzpp;
  int fp,np,imax;
  double lowfreq, highfreq;

  Wturnoff_buttons();
  if(init2d(1,1)) return(ERROR);
  ctrace = currentindex();
  if ((spectrum = calc_spec(ctrace-1,0,FALSE,TRUE,&dummy))==0)
    return(ERROR);
  peakfrac = 0.5;
  if (argc > 1)
  {
    if (isReal(*++argv))
      rcrv = stringReal(*argv);
    else
    {
      Werrprintf("first argument must be the peak frequency");
      return(ERROR);
    }
    if (argc > 3)
    {
      Werrprintf("Usage: dres(<peak frequency <, fraction of peak amplitude> >)");
      return(ERROR);
    }
    else if (argc == 3)
    {
      if (isReal(*++argv))
      {
        peakfrac = stringReal(*argv);
	if (peakfrac < .0010)
        { peakfrac = 0.0010;
	  Winfoprintf("Fraction of peak amplitude set to 0.0010");
	}
        else if (peakfrac > .99)
        { peakfrac = 0.99;
	  Winfoprintf("Fraction of peak amplitude set to 0.99");
 	}
      }
      else
      {
        Werrprintf("Usage: dres(<peak frequency <, fraction of peak amplitude> >)");
        return(ERROR);
      }
    }
  }
  else
    rcrv = cr;
  rcrv += rflrfp;
  hzpp = sw / (double) (fn / 2);
  max = 0.0;
  imax = fp = (int) ((sw - rcrv) / hzpp);
  msearch = MSEARCH;
  if (fn > 16384) msearch = MSEARCH * fn /16384;
  fp = ((fp - msearch < 0) ? 0 : fp - msearch);
  np = 2 * msearch + 1;
  if (fp + np > fn / 2)
    fp = (fn / 2) - np;
  if (debug1)
    Wscrprintf("first pt= %d num pt= %d total pt= %d\n",fp,np,fn/2);
  for (i = 0; i < np; i++)
    if (*(spectrum + fp + i) > max)
    {
      imax = fp + i;
      max = *(spectrum + imax);
    } 
  if (debug1)
    Wscrprintf("max= %g at imax= %d\n",max,imax);
  fp = ((imax - SEARCH < 0) ? 0 : imax - SEARCH);
  np = 2 * SEARCH;
  if (peakfrac < 0.499)
    np = np * sqrt(1.0/peakfrac);
  
  if (np > fn / 2)
    np = fn / 2;
  if (fp + np > fn / 2)
    fp = fn / 2 - np;
  if (fp + np < 0)
    fp = 0;
  max *= peakfrac;
  i = imax;
  while ((*(spectrum + i) > max) && (i > 1))
    i--;
  while ((*(spectrum + imax) > max) && (imax < fn / 2))
    imax++;
  if (debug1)
    Wscrprintf("right pt= %d left pt= %d\n",i,imax);
  if ((i < 4) || (imax > (fn / 2) - 4))
  {
    Werrprintf("unreliable result");
    lw = 0.0;
    ptr = spectrum + imax;
    lowfreq  = hzpp * ((double) imax - (max - *ptr)/(*(ptr - 1) - *ptr));
    ptr = spectrum + i;
    highfreq = hzpp * ((double) i    + (max - *ptr)/(*(ptr + 1) - *ptr));
    if (highfreq < lowfreq)
    {
       double tmp;
       tmp = highfreq;
       highfreq = lowfreq;
       lowfreq = tmp;
    }
    lowfreq = (sw - lowfreq) - rflrfp;
    highfreq = (sw - highfreq) - rflrfp;
  }
  else
  {
    ptr = spectrum + imax;
    lowfreq  = hzpp * ((double) imax - (max - *ptr)/(*(ptr - 1) - *ptr));
    ptr = spectrum + i;
    highfreq = hzpp * ((double) i    + (max - *ptr)/(*(ptr + 1) - *ptr));
    lowfreq = (sw - lowfreq) - rflrfp;
    highfreq = (sw - highfreq) - rflrfp;
    if (highfreq < lowfreq)
    {
       double tmp;
       tmp = highfreq;
       highfreq = lowfreq;
       lowfreq = tmp;
    }
    lw = highfreq - lowfreq;
    if (retc==0)
    {
      if (peakfrac < 0.499)
        Winfoprintf("linewidth = %g Hz at fractional peak height = %g",lw,peakfrac);
      else
        Winfoprintf("linewidth = %g Hz   digital resolution = %g  Hz",lw,hzpp);
    }
  }
  appendvarlist("dummy");
  if (retc>0)
    retv[0] = realString(lw);
  if (retc>1)
    retv[1] = realString(hzpp);
  if (retc>2)
    retv[2] = realString(highfreq);
  if (retc>3)
    retv[3] = realString(lowfreq);
  return(rel_spec());
}

#define METHANOL 0
#define GLYCOL 1

/**************************/
int tempcal(int argc, char *argv[], int retc, char *retv[])
/**************************/
{ int r,solvent;
  double delta,delta1,sfrq,temp;
  if ( (r=P_getreal(CURRENT,"delta",&delta,1)) )
    { P_err(r,"delta",":");
      return 1;
    }
  if ( (r=P_getreal(PROCESSED,"sfrq",&sfrq,1)) )
    { P_err(r,"sfrq",":");
      return 1;
    }
  delta1 = delta * 60.0 / sfrq;
  if (argc==1)
    solvent = METHANOL;
  else if ((argc==2) &&
     ((strcmp(argv[1],"methanol")==0) || (strcmp(argv[1],"m")==0)))
    solvent = METHANOL;
  else if ((argc==2) &&
     ((strcmp(argv[1],"glycol")==0) || (strcmp(argv[1],"g")==0) ||
      (strcmp(argv[1],"e")==0)))
    solvent = GLYCOL;
  else
    { Werrprintf("usage - tempcal with 'methanol' or 'm' or 'glycol' or 'g' or 'e'");
      return 1;
    }
  if (solvent==METHANOL)
    temp = 129.85 - 0.491 * delta1 - 66.2e-4 * delta1 * delta1;
  else
    temp = 192.85 - 1.694 * delta1;
  if (retc==0)
    Winfoprintf("delta = %g Hz   temperature = %6.2f deg C",delta,temp);
  if (retc>0)
    retv[0] = realString(temp);
  return 0;
}

/************************/
int h2cal(int argc, char *argv[], int retc, char *retv[])
/************************/
{ int i,r,done,ok;
  vInfo  info;
  double offset[2],jr[2],jreduce[2],gammah2,v0,j0,tmp;
  char inputtext[80];
  char dofname[MAXSTR];

  if ( ! P_getstring(PROCESSED,"array",dofname,1,MAXSTR) )
  {
     if ( strcmp(dofname,"dof") && strcmp(dofname,"dof2") && strcmp(dofname,"dof3") && strcmp(dofname,"dof4") )
        strcpy(dofname,"dof");
  }
  else
  {
    strcpy(dofname,"dof");
  }
  if ( (r=P_getVarInfo(PROCESSED,dofname,&info)) )
    { P_err(r,dofname,":");
      return 1;
    }
  if (info.size==2)
    { /* get offset values from dof array */
      if ( (r=P_getreal(PROCESSED,dofname,&offset[0],1)) )
        { P_err(r,dofname,":");
          return 1;
        }
      if ( (r=P_getreal(PROCESSED,dofname,&offset[1],2)) )
        { P_err(r,dofname,":");
          return 1;
        }
    }
  else
    { done = 0;
      while (!done)
        { W_getInput("Low field frequency? ",inputtext,79);
          if (strlen(inputtext)==0)
            { Werrprintf("program aborted");
              return 1;
            }
          if (isReal(inputtext))
            { offset[0] = stringReal(inputtext);
              done = 1;
            }
        }
      done = 0;
      while (!done)
        { W_getInput("High field frequency? ",inputtext,79);
          if (strlen(inputtext)==0)
            { Werrprintf("program aborted");
              return 1;
            }
          if (isReal(inputtext))
            { offset[1] = stringReal(inputtext);
              done = 1;
            }
        }
    }
  if (argc==1)
    { done = 0;
      while (!done)
        { W_getInput("Residual coupling, low field? ",inputtext,79);
          if (strlen(inputtext)==0)
            { Werrprintf("program aborted");
              return 1;
            }
          if (isReal(inputtext))
            { jr[0] = stringReal(inputtext);
              done = 1;
            }
        }
      done = 0;
      while (!done)
        { W_getInput("Residual coupling, high field? ",inputtext,79);
          if (strlen(inputtext)==0)
            { Werrprintf("program aborted");
              return 1;
            }
          if (isReal(inputtext))
            { jr[1] = stringReal(inputtext);
              done = 1;
            }
        }
      done = 0;
      while (!done)
        { W_getInput("J0 (Dioxane=142 Hz)? ",inputtext,79);
          if (strlen(inputtext)==0)
            { j0 = 142.0;
              done = 1;
            }
          else if (isReal(inputtext))
            { j0 = stringReal(inputtext);
              done = 1;
            }
        }
    }
  else if ((argc==4) && isReal(argv[1]) && isReal(argv[2]) && isReal(argv[3]))
    { jr[0] = stringReal(argv[1]);
      jr[1] = stringReal(argv[2]);
      j0  = stringReal(argv[3]);
    }
  else if ((argc==3) && isReal(argv[1]) && isReal(argv[2]))
    { jr[0] = stringReal(argv[1]);
      jr[1] = stringReal(argv[2]);
      j0  = 142.0;
    }
  else
    { Werrprintf("usage - h2cal or h2cal(jr1,jr2) or h2cal(jr1,jr2,j0)");
      return 1;
    }
  jr[0] = fabs(jr[0]);
  jr[1] = fabs(jr[1]);
  ok = TRUE;
  i = 0;
  for (i=0; i<2; i++)
  {
    tmp = j0*j0-jr[i]*jr[i];
    if ((ok == TRUE) && (tmp > 1e-15))
      jreduce[i] = jr[i]/sqrt(tmp);
    else
      ok = FALSE;
  }
  v0 = 0.0;
  gammah2 = 1.0;
  if (ok == TRUE)
  {
    tmp = jreduce[0] + jreduce[1];
    if (tmp > 1e-15)
    {
      gammah2 = (offset[0]-offset[1]) / tmp;
      v0 = offset[1] + gammah2 * jreduce[1];
    }
    else
      ok = FALSE;
  }

  if (ok == TRUE)
  {
    if (gammah2<0)
      gammah2 = -gammah2;
    if (retc == 0)
    {
      Wshow_text();
      Wclear_text();
      Wscrprintf("             Gamma-H2 calculation\n");
      Wscrprintf("      See J. Magnetic Resonance 7:442 (1972) ");
      Wscrprintf("\n");
      Wscrprintf("   J0 = %10.2f Hz,   dof[1] = %10.2f Hz,  dof[2] = %10.2f Hz\n",
        j0,offset[0],offset[1]);
      Wscrprintf("Jr[1] = %10.2f Hz,   Jr[2]  = %10.2f Hz\n",jr[0],jr[1]);
      Wscrprintf("\n");
      Wscrprintf("Gamma-H2        = %10.2f Hz\n",gammah2);
      Wscrprintf("90 degree pulse = %10.1f usec\n",1e6/(4*gammah2));
      Wscrprintf("coalescence frequency = %10.2f Hz\n",v0);
      Wsettextdisplay("h2cal");
    }
  }
  else
    Werrprintf("inappropriate data for gamma-h2 calibration");
  if (retc>0)
    retv[0] = realString((ok == TRUE) ? gammah2 : 0.0);
  if (retc>1)
    retv[1] = realString((ok == TRUE) ? 1e6/(4*gammah2) : 0.0);
  if (retc>2)
    retv[2] = realString((ok == TRUE) ? v0 : 0.0);
  return 0;
}
