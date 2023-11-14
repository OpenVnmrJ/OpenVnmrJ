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
/*  li	 -	list integral values	 			*/
/*  dli	 -	display listed integral values			*/
/*  lni	 -	list normalized integral values	 		*/
/*  dlni -	display listed normalized integral values	*/
/*  z    -	add integral reset point at the cursor position */
/*  cz   -	clear all integral reset points			*/
/*  intbl   -	set the integral blanking mode			*/
/*  cintbl   -	exit the integral blanking mode			*/
/*								*/
/****************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "allocate.h"
#include "disp.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "pvars.h"
#include "variables.h"
#include "wjunk.h"
#include "vnmrsys.h"

#define CALIB 		1000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define MAXREG          30
#define MAXSAV          200
#define MAXPHASE        3600.0
#define MINPHASE        0.1
#define PI              3.14159265358979323846
#define DEG             (180.0 / PI) 
#define EPSILON 1e-15

#ifdef  DEBUG
extern int debug1;
#define DPRINT0(str) \
	if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1) \
	if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4)
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4,arg5)
#else
#define DPRINT0(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5)
#endif

/* Pointer to Function returning an Integer */
/*
 * typedef int (*PFI)();
 */

extern int currentindex();
extern void disp_status_OnOff(int onOff);
extern void maxfloat2(register float  *datapntr, register int npnt, register float  *max);
extern void integ2(float *frompntr, int fpnt, int npnt, float *value, int offset);
extern void integ(register float  *fptr, register float  *tptr, register int npnt);
extern void get_label(int direction, int style, char *label);
extern void Wturnoff_buttons();
extern int  rel_spec();
extern int  checkphase(short status);
extern int     start_from_ft;   /* set by ft if ds is to be executed */

static float *spectrum;
static float  thresh;

float *phase_data;
float *aphMask = NULL;
float  peakfactor,noisefactor, invivofactor;
double tailmult;
int    minpointsinpeak, invivo;
int    reg_start[MAXSAV],reg_end[MAXSAV];
int    aph0, aph_b;
int minpointsbelow,tailext,tailsize;
struct peak
       {
         double x,y,wx,wy,f;
       } speak[MAXREG];
double sigc1,  sigc2,  sigs1,  sigs2;
double sigcp1, sigcp2, sigsp1, sigsp2;
double sigcdp1,sigcdp2,sigsdp1,sigsdp2;

float amax, aav, xdev, pv, pdv;
int amax_index = 0;
int pnav,xxmi;

float *bphdata=NULL;

/* Function Prototypes */

static int fail_1(double *, double *, double, int);
static int init_lp(float *);
static int fail(int, int, float, int);
#ifdef XXX
static int sumvalue(float *, float *, float *, float *, float *, float *, int, float);
static int submitvalue(float *, float, float, float, float, float, float);
#endif
static int classify_regions(int, int *, int *, int *, float, int, int);
static int correct_phase(int);
static int find_noise(int, char * [], int, int);
static int correct_state(int);
static int find_regions(int, int, int, int *);
static int find_peak(int *, int *, int, int, float *, int *, int *, int *, int *);
static int group_peaks(int *);
static int cal_pv(int);
static int phase_regions(int *, int *, float, int);
static int adjust_ph(int, float *, int *, float *, float, int *);
static int rule_ph1(float, float, float, float *);
static int rule_ph2(float, float, float *, float);
static int rule_ph3(float, int, float, int, float *, float, int, int);
static int first_rp(int, int *, int *);
static int find_ref(int, int *);
static int second_lp(int, int, float *, int *, int);
static int find_ref1(int, int, int);
static int calc_lp(int *, int *, int *, int, int *, int *, float *);
int do_aph_n(int first, int last, double *new_rp, double *new_lp, int order);
int z_add(double freq);
int z_delete(double freq);

struct peakB
       { int state, linenum;
         double amax, phase;
         int ltail, rtail, php;
       } speakB[MAXSAV];


/*************/
static void setwindows()
/*************/
{
  Wclear_text();
  Wshow_text();
}

/******************************/
static int datapoint(double freq, double sw, int fn)
/******************************/
{
  return((int) ((sw - freq) * (double)fn / sw));
}

static void initAphMask(float val, int num)
{
   if (aphMask == NULL)
   {
      int index;
      float *ptr;
      ptr = aphMask = (float *) allocateWithId(sizeof(float) * num, "aph");
      for (index = 0; index < num; index++)
         *ptr++ = val;
   }
}

static void setAphMask(float val, int first, int last)
{
   if (aphMask)
   {
      int index;
      float *ptr;
      ptr = aphMask;
      if (last < first)
      {
         index = last;
         last = first;
         first = index;
      }
      if (first < 0)
         first = 0;
      if (last > fn/2)
         last = fn/2;
      for (index = first; index < last; index++)
         *(ptr+index) = val;
   }
}

/*
static void showAphMask()
{
   if (aphMask)
   {
      int index;
      float *ptr;
      ptr = aphMask;
      for (index = 0; index < fn/2; index++)
         fprintf(stderr,"%d",(int) *ptr++);
      fprintf(stderr,"\n");
   }
}
 */

/****************/
static int calc_integ_int()
/****************/
{
  vInfo  info;
  double value;
  int    r;
  int    index;
  float  lastint;
  float  newint;
  int    point;
  float *integral;

  if ((integral = (float *) allocateWithId(sizeof(float) * npnt,"dli"))==0)
  {
    Werrprintf("cannot allocate integral buffer");
    return(ERROR);
  }
  integ(spectrum+fpnt,integral,npnt);
  DPRINT0("calculated integral\n");

  if ( (r=P_getVarInfo(CURRENT,"lifrq",&info)) )
    info.size = 1;
  DPRINT1("lifrq size= %d\n",info.size);

  point = fpnt;
  lastint = *integral;
  index = 1;
  if ( (r=P_setreal(CURRENT,"liamp",0.0,0)) )  /* reset the array */
  { P_err(r,"liamp",":"); release(integral);  return ERROR; }

  while ((index <= info.size) && (point < fpnt + npnt - 1))
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
      value = 0.0;          /*  no resets defined  */
    point = datapoint(value,sw,fn/2);
    if (point>fpnt)
    {
      if (point>fpnt+npnt-1)
        point = fpnt + npnt - 1;
      newint = *(integral+point-fpnt);
      DPRINT5("newint=%g lastint=%g point=%d fpnt=%d npnt=%d\n",
                    newint,lastint,point,fpnt,npnt);
      if ( (r=P_setreal(CURRENT,"liamp",newint - lastint,index)) )
      { P_err(r,"liamp",":"); release(integral);  return ERROR; }
      lastint = newint;
    }
    else
    {
      if ( (r=P_setreal(CURRENT,"liamp",0.0,index)) )
      { P_err(r,"liamp",":"); release(integral);  return ERROR; }
    }
    index++;
  }
  release(integral);
  return(COMPLETE);
}

/****************/
static int calc_int()
/****************/
{
  vInfo  info;
  double value;
  int    r;
  int    index;
  float  lastint;
  float  newint;
  int    point;
  float *integral;

  if ((integral = (float *) allocateWithId(sizeof(float) * (fn/2),"dli"))==0)
  {
    Werrprintf("cannot allocate integral buffer");
    return(ERROR);
  }
  integ(spectrum,integral,fn/2);
  DPRINT0("calculated integral\n");

  if ( (r=P_getVarInfo(CURRENT,"lifrq",&info)) )
    info.size = 1;
  DPRINT1("lifrq size= %d\n",info.size);

  point = 0;
  lastint = *integral;
  index = 1;
  if ( (r=P_setreal(CURRENT,"liamp",0.0,0)) )  /* reset the array */
  { P_err(r,"liamp",":"); release(integral);  return ERROR; }
  
  while ((index <= info.size) && (point < fn/2 - 1))
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
      value = 0.0;          /*  no resets defined  */
    point = datapoint(value,sw,fn/2);
    DPRINT4("value=%g sw=%g fn/2=%d point=%d \n",
                    value,sw,fn/2,point);
    if (point >= fn/2)
       point = fn/2 -1;
    newint = *(integral+point);
    DPRINT5("newint=%g lastint=%g point=%d fpnt=%d npnt=%d\n",
                    newint,lastint,point,fpnt,npnt);
    if ( (r=P_setreal(CURRENT,"liamp",newint - lastint,index)) )
    {
       P_err(r,"liamp",":");
       release(integral);
       return ERROR;
    }
    lastint = newint;
    index++;
  }
  release(integral);
  return(COMPLETE);
}

/****************/
static int check_regions()
/****************/
/* set regions flag */
{ int    res;
  char   intmod[8];
  vInfo  info;
  
  if ( (res=P_getstring(CURRENT,"intmod",intmod,1,8)) )
  { P_err(res,"intmod",":"); return(FALSE); }
  if ((res = strcmp(intmod,INT_OFF)) == 0)  /* if intmod is off */
    return((P_getVarInfo(CURRENT,"lifrq",&info)) ? FALSE : info.active);
  return((res = strcmp(intmod,INT_PARTIAL)) == 0); 
}

/****************/
static int print_int()
/****************/
{
  double vs1;
  vInfo  info;
  double value;
  double start;
  double edge;
  int    r;
  int    index;
  double newint;
  double norm;
  char   line1[80];
  int    regions;
  char   units[15];
  

  if ( (r=P_getVarInfo(CURRENT,"liamp",&info)) )
  { P_err(r,"liamp",":");   return ERROR; }

  regions = (check_regions() && (info.size > 1));
  get_label(HORIZ,UNIT1,units);
  if (regions)
  {
    sprintf(line1,"region     start  %-8.8s end    integral\n",units);
  }
  else
  {
    sprintf(line1,"index   freq %-8.8s intensity\n",units);
  }
  Wscrprintf(line1);

  start = (sp + wp) / sfrq;
  edge = start;
  if (normInt)
  {
    norm = 0.0;
    DPRINT0("integral normalization\n");
    for (index = 1; index <= info.size; index++)
    {
      if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
      { P_err(r,"lifrq",":");   return ERROR; }
      if ((value - rflrfp) / sfrq < edge )
      {
        if ( (r=P_getreal(CURRENT,"liamp",&newint,index)) )
        { P_err(r,"liamp",":");   return ERROR; }
        if (!regions)
          norm += newint;
        else
          if (index % 2 == 0)
            norm += newint;
      }
    }
    vs1 = (norm == 0.0) ? 1.0 : insval  / norm;
  }
  else
    vs1 =  insval;
  if (vs1 == 0.0)
    vs1 = 1.0;
  DPRINT3("scale factor= %g, regions= %d, liamp size= %d\n",
                vs1,regions,info.size);
  for (index = 1; index <= info.size; index++)
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
    { P_err(r,"lifrq",":");   return ERROR; }
    value = (value - rflrfp) / sfrq;
    if (value < edge )
    {
      if (value < sp / sfrq)
        value = sp / sfrq;
      if ( (r=P_getreal(CURRENT,"liamp",&newint,index)) )
      { P_err(r,"liamp",":");   return ERROR; }
      if (regions)
      {
        if (index % 2)
          start = value;
        else
          Wscrprintf("%4d  %10g    %10g  %10g\n",
                     index / 2,start,value,vs1 * newint);
      }
      else
        Wscrprintf("%4d  %10g     %10g\n",
                     index,value,vs1 * newint);
    }
  }
  return(COMPLETE);
}

/****************/
static int max_integral(int retc, char *retv[])
/****************/
{
  vInfo  info;
  double value;
  double lastValue;
  double norm;
  double vs1;
  double maxint;
  double leftEdge;
  double rightEdge;
  int    r;
  int    index;
  double newint;
  int    regions;
  

  if ( (r=P_getVarInfo(CURRENT,"liamp",&info)) )
  { P_err(r,"liamp",":");   return ERROR; }

  regions = (check_regions() && (info.size > 1));
  leftEdge = (sp + wp) / sfrq;
  rightEdge = sp / sfrq;
  DPRINT3("scale factor= %g, regions= %d, liamp size= %d\n",
                insval,regions,info.size);
  maxint = 0.0;
  norm = 0.0;
  lastValue = (sw - rflrfp) / sfrq;
  for (index = 1; index <= info.size; index++)
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
    { P_err(r,"lifrq",":");   return ERROR; }
    value = (value - rflrfp) / sfrq;
    if ( (r=P_getreal(CURRENT,"liamp",&newint,index)) )
    { P_err(r,"liamp",":");   return ERROR; }
    if (!regions)
    {
       if ( (value < leftEdge) && (lastValue > rightEdge))
       {
         norm += newint;
         if (fabs(newint) > fabs(maxint))
            maxint = newint;
       }
    }
    else if (index % 2 == 0)
    {
       if ( (value < leftEdge) && (lastValue > rightEdge))
       {
         norm += newint;
         if (fabs(newint) > fabs(maxint))
            maxint = newint;
       }
    }
    lastValue = value;
  }
  if (normInt)
    vs1 = (norm == 0.0) ? 1.0 : insval  / norm;
  else
    vs1 =  insval;
  if (vs1 == 0.0)
    vs1 = 1.0;
  if (retc==0)
    Winfoprintf("height of the largest integral is %g. Value is %g",
                 is * maxint / ( (double)fn / 128.0 ), vs1 * maxint);
  if (retc>0)
    retv[0] = realString(is * maxint / ( (double)fn / 128.0 ));
  if (retc>1)
    retv[1] = realString(vs1 * maxint);
  return(COMPLETE);
}

/*************/
static int checkinput(int argc, char *argv[])
/*************/
{
  if (isReal(*++argv))
    sp = stringReal(*argv);
  else
  {
    Werrprintf("first argument must be the high field frequency");
    return(ERROR);
  }
  if ((argc>2) && (isReal(*++argv)))
    wp = stringReal(*argv) - sp;
  else
  {
    Werrprintf("second argument must be the low field frequency");
    return(ERROR);
  }
  if (wp < 0.0)
  {
    Werrprintf("argument 1 must be less than argument 2");
    return(ERROR);
  }
  if (argc != 3)
  {
    Werrprintf("only two arguments may be supplied");
    return(ERROR);
  }
  exp_factors(TRUE);
  return(COMPLETE);
}

/*  add integral reset points */
/*  or clear all integral reset points */
/*  or toggle integral blanking*/
/*  or list integral values */
/*************/
int dli(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int ctrace;
  int cz;
  int dummy;

  cz = (strcmp(argv[0],"cz")==0);
  if ((cz && (argc>1)) ||
      (strcmp(argv[0],"z")==0))
  {
    if(init2d(1,1)) return(ERROR);
    if (argc>1)
    {
      while (--argc > 0)
        if (isReal(*++argv))
          if (cz)
            z_delete(stringReal(*argv) + rflrfp);
          else
            z_add(stringReal(*argv) + rflrfp);
        else
          argc = 0;
    }
    else
      z_add(cr + rflrfp);
  }

  else if (cz)
    P_setreal(CURRENT,"lifrq",0.0,0);

  else
  {
    Wturnoff_buttons();
    if (argv[0][0] == 'd')
      setwindows();
    if(init2d(1,1)) return(ERROR);
    ctrace = currentindex();
    if ((spectrum = calc_spec(ctrace-1,0,FALSE,TRUE,&dummy))==0)
      return(ERROR);
    if ((strcmp(argv[0],"integ")==0) && (argc>1))
      if (checkinput(argc,argv))
        return(ERROR);
    if ( ! strcmp(argv[0],"integ") )
       calc_integ_int();
    else
       calc_int();
    if ( ! strcmp(argv[0],"integ") )
      max_integral(retc,retv);
    else if (argv[0][0] != 'n')
      print_int();
    if (argv[0][0] == 'd')
      Wsettextdisplay("dli");
    if (rel_spec()) return(ERROR);
  }
  releasevarlist();
  appendvarlist("lifrq,liamp");
  return(COMPLETE);
}

/****************/
int z_delete(double freq)
/****************/
{
  vInfo  info;
  double value;
  double *save;
  double *val;
  int    r;
  int    index;
  int    first;

  if ( (r=P_getVarInfo(CURRENT,"lifrq",&info)) )
    P_err(r,"info?","lifrq:");
  if (info.size>1)
  {
    if ((save = (double *) allocateWithId(sizeof(double) * (info.size+1),"cz"))==0)
    {
      Werrprintf("cannot allocate integral buffer");
      return(ERROR);
    }
    val = save;
    first = 1;
    for (index=1; index <= info.size; index++)
    {
      if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
      { P_err(r,"lifrq",":");   return ERROR; }
      val[index] = value;
      if (value > freq)
         first = index;
    }
    P_setreal(CURRENT,"lifrq",0.0,0);
    if (first == info.size)
       first--;
    if (val[first] - freq > freq - val[first+1])
       first++;
    if (first == info.size)
       first--;
       
    info.size--;
    val++;
    for (index=1; index <= info.size; index++)
    {
      if (index == first)
         val++;
      P_setreal(CURRENT,"lifrq",*val++,index);
    }
    release(save);
  }
  return COMPLETE;
}

/************/
int z_add(double freq)
/************/
{
  vInfo  info;
  double value;
  int    r;
  int    index;

  if ( (r=P_getVarInfo(CURRENT,"lifrq",&info)) )
    P_err(r,"info?","lifrq:");
  if (info.size>1)
  {
    for (index=1; index <= info.size; index++)
    {
      if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
      { P_err(r,"lifrq",":");   return ERROR; }
      if (value < freq)
      {
        P_setreal(CURRENT,"lifrq",freq,index);
        freq = value;
      }
    }
    P_setreal(CURRENT,"lifrq",0.0,info.size + 1);
  }
  else
  {
    P_setreal(CURRENT,"lifrq",freq,1);
    P_setreal(CURRENT,"lifrq",0.0,2);
  }
  return COMPLETE;
}

/******************************/
static double dp_to_frq(int dp, double sw, int fn)
/******************************/
{
  return((double) (sw * ( 1.0 - ((double) dp / (double)(fn-1)))));
}

/**************************/
static float aphpnt(int index)
/**************************/
{ float re,im;

  re = (float)fabs((double)((*(phase_data + 2*index + 2) -
                             *(phase_data + 2*index - 2)) / 2.0));
  im = (float)fabs((double)((*(phase_data + 2*index + 3) -
                             *(phase_data + 2*index - 3)) / 2.0));
  return((re>im) ? re : im);
}

/**************************/
static float regpnt(int index)
/**************************/
{
  return((float)fabs((double)((*(spectrum + index + 1) - *(spectrum + index - 1)) / 2.0)));
}

/*  Change made in `findregion' in response to a data set
    from Monsanto.  The first WHILE loop locates the next
    peak.  Notice it exits when `na' becomes larger than
    `minpointinpeak'.  Previously, `na' was set to 0 if
    the intensity dropped below the threshold; now it is
    decremented by 1 if its value is greater than 0.		*/

/***************************************/
static int findregion(int *sindex, int npt, float (*getpnt)() )
/***************************************/
{
  register int peak;
  register int eindex;
  register int na;
  register int n;

  n = npt - 1;
  peak = FALSE;
  eindex = *sindex;
  na = 0;
  while ((eindex < n) && (!peak))
    if ((*getpnt)(eindex++) <= thresh)
    {
      if (na > 0) na--;
    }
    else if (++na >= minpointsinpeak)
      peak = TRUE;
  if (peak)
  {
    *sindex = eindex - 1;
    while (eindex < n)
      if ((*getpnt)(eindex++) > thresh)
        na = 0;
      else if (++na >= minpointsbelow)
        return(eindex-1);
    return(n);
  }
  else
    return(0);
}

/*************************/
static void modify_regions(int *num)
/*************************/
{
  int i,j,min;

  i = 0;
  while (i < *num -1)
  {
    i++;
    if ((reg_start[i+1] - reg_end[i] < 1) && (i < *num - 1))
    {
      reg_end[i] = reg_end[i+1];
      for (j = i + 1; j < *num; j++)
      {
        reg_start[j] = reg_start[j+1];
        reg_end[j] = reg_end[j+1];
      }
      i--;
      *num -= 1;
    }
  }
  while (*num >= MAXREG)
  {
    min = fn / 2;
    for (j = 1; j < *num; j++)
      if (min > reg_start[j+1] - reg_end[j])
      {
        min = reg_start[j+1] - reg_end[j];
        i = j;
      }
    reg_end[i] = reg_end[i+1];
    for (j = i + 1; j < *num; j++)
    {
      reg_start[j] = reg_start[j+1];
      reg_end[j] = reg_end[j+1];
    }
    *num -= 1;
  }
}

/*************************/
static void adjust_tails(int num)
/*************************/
{
  int i,max;

  max = fn / 2 - 1;
  for (i=1; i <= num; i++)
  {
    reg_start[i] -= tailext;
    reg_end[i]   += tailext;
    if (reg_start[i] <= 0)
      reg_start[i] = 1;
    if (reg_end[i] >= max)
      reg_end[i] = max - 1;
  }
  for (i=1; i < num; i++)
    if (reg_end[i] > reg_start[i+1])
    {
      delta = (reg_end[i] - reg_start[i+1] +1) / 2;
      reg_end[i]     -= delta;
      reg_start[i+1] += delta;
    }
}

/*************************/
static int select_regions(float (*getpnt)(), int first, int last)
/*************************/
{
  int sindex,eindex;
  int r_number;
  int np;

#ifdef SIS
 if ( getpnt == regpnt )
     sindex = 1;
  else
     sindex = 2;
#else
  sindex = 2;
#endif

  sindex += first;
  r_number = 0;
  np = last/*(fn / 2)*/;
  while ( (eindex = findregion(&sindex,np,getpnt)) )
  {
    if (r_number < MAXSAV)
      r_number++;
    reg_start[r_number] = sindex - minpointsinpeak
                          - (int) (tailsize * tailmult);
    if (reg_start[r_number] < first)
      reg_start[r_number] = first;
    reg_end[r_number] = eindex - minpointsbelow
                        + (int) (tailsize * tailmult);
    if (reg_end[r_number] > first+np)
      reg_end[r_number] = first+np;
    if (r_number == MAXSAV - 1)
      modify_regions(&r_number);
    DPRINT5("number= %d, sindex= %d, eindex= %d, start= %d, end= %d\n",
              r_number,sindex-minpointsinpeak,eindex-minpointsbelow,
              reg_start[r_number],reg_end[r_number]);
    sindex = eindex;
  }
  modify_regions(&r_number);
  return(r_number);
}

/*********************************/
static void put_resets(int regions, int retc)
/*********************************/
{
  int i,index;

  P_setreal(CURRENT,"lifrq",0.0,0);
  index = 1;
  for (i = 1; i <= regions; i++)
  {
    P_setreal(CURRENT,"lifrq",
              dp_to_frq(reg_start[i],sw,fn / 2),index++);
    P_setreal(CURRENT,"lifrq",
              dp_to_frq(reg_end[i],sw,fn / 2),index++);
  }
  P_setreal(CURRENT,"lifrq",0.0,index);
  if (retc==0)
    { if (regions == 1)
        Winfoprintf("1 region has been found");
      else
        Winfoprintf("%d regions have been found",regions);
    }
}

/**************************************/
static float findthresh(float *specptr, int np, double xcdev, int incr)
/**************************************/
{
  register int n;
  register float sigx = 32768.0;
  register float sigxx = -32768.0;
  float  value;

  n = np/ 20;
  if (n < 20)
    n = 20;
  else if (n > 200)
    n = 200;
/* n -= 1; */
  n -= 4;
  specptr += 3 * incr;
  while (n--)
  {
    value = *specptr + *(specptr - 1 * incr) -
            *(specptr -  2 * incr) - *(specptr - 3 * incr);
/*    value = *(specptr+1) - *specptr; */
    specptr += incr;
    if (value < sigx)
      sigx = value;
    if (value > sigxx)
      sigxx = value;
  }
  return((sigxx - sigx) * xcdev / 2.0);
}

/********************************/
static int region_input(int argc, char *argv[], float *specptr,
                        int incr, int first, int last, int reg_mult)
/********************************/
{
  double pphz;
  double xcdev;
  double cor = 0.0;
  vInfo  info;
  double value;
  int r;
  int argno;

  pphz = ((double) (fn / 2 - 1)) / sw;
  if ( (r=P_getVarInfo(PROCESSED,"lb",&info)) )
  { P_err(r,"lb",":");   return ERROR; }
  if (info.active)
  {
    if ( (r=P_getreal(PROCESSED,"lb",&value,1)) )
    { P_err(r,"lb",":");   return ERROR; }
    cor = PI * value;
  }
  if ( (r=P_getVarInfo(PROCESSED,"gf",&info)) )
  { P_err(r,"gf",":");   return ERROR; }
  if (info.active)
  {
    if ( (r=P_getreal(PROCESSED,"gf",&value,1)) )
    { P_err(r,"gf",":");   return ERROR; }
    cor += 1.66 / value;
  }
  cor *= pphz;
  minpointsinpeak = (int) (cor + 7.5);
  if (minpointsinpeak > 25)
    minpointsinpeak = 25;
  else if (minpointsinpeak < 1)
    minpointsinpeak = 1;
  if ( (r=P_getreal(PROCESSED,"sfrq",&value,1)) )
  { P_err(r,"sfrq",":");   return ERROR; }
  tailsize = (int) (0.5 * cor + pphz * value / 200.0);
  if (tailsize < minpointsinpeak / 4)
    tailsize = 0;
  else if (tailsize > 2 * minpointsinpeak + minpointsinpeak / 2)
    tailsize = 2 * minpointsinpeak + minpointsinpeak / 2;
  DPRINT3("cor= %g, minpointsinpeak= %d, tailsize= %d\n",
                cor,minpointsinpeak,tailsize);

  tailext = (int)(sw * pphz / 100.0);
  tailmult = (reg_mult) ? 12.0 : 2.0;
  xcdev = 0.6;
  argno=1;
  if ((argc > argno) && (isReal(argv[argno])))
  {
    r = (int)(stringReal(argv[argno]) * pphz);
    if (r >= 0)
      tailext = r;
    argno++;
    if ((argc > argno) && (isReal(argv[argno])))
    {
      value = stringReal(argv[argno]);
      if (value > 0.0)
        tailmult = value;
      argno++;
      if ((argc > argno) && (isReal(argv[argno])))
      {
        value = stringReal(argv[argno]);
        if (value > 0.0)
          xcdev = value;
        argno++;
        if ((argc > argno) && (isReal(argv[argno])))
        {
          r = (int) stringReal(argv[argno]);
          if (r > 0)
            minpointsinpeak = r;
          argno++;
          if ((argc > argno) && (isReal(argv[argno])))
          {
            r = (int) stringReal(argv[argno]);
            if (r > 0)
              tailsize = r;
          }
        }
      }
    }
  }
  aphMask = NULL;
  while (argc > argno)
  {
     double frq1, frq2;
     if ( ! strcmp(argv[argno],"select") )
     {
        argno++;
        while ( (argc > argno+1) && isReal(argv[argno]) && isReal(argv[argno+1]) )
        {
           initAphMask(0.0, fn/2);
           frq1 = stringReal(argv[argno++]);
           frq2 = stringReal(argv[argno++]);
           setAphMask(1, datapoint(frq1+rflrfp,sw,fn/2), datapoint(frq2+rflrfp,sw,fn/2) );
        }
     }
     else if ( ! strcmp(argv[argno],"ignore") )
     {
        argno++;
        while ( (argc > argno+1) && isReal(argv[argno]) && isReal(argv[argno+1]) )
        {
           initAphMask(1.0, fn/2);
           frq1 = stringReal(argv[argno++]);
           frq2 = stringReal(argv[argno++]);
           setAphMask(0, datapoint(frq1+rflrfp,sw,fn/2), datapoint(frq2+rflrfp,sw,fn/2) );
        }
     }
     else
     {
        break;
     }
  }
/* showAphMask(); */

  minpointsbelow = minpointsinpeak + 3 + (int) (tailmult * (double) tailsize);
  if (tailsize == 0)
    tailsize = 1;
  thresh = findthresh(specptr+incr*first,(last-first),xcdev,incr);
  DPRINT3("tailext= %d, tailmult= %g, minpointsbelow= %d\n",
                tailext,tailmult,minpointsbelow);
  DPRINT4("xcdev= %g, tHresh= %g, minpointsinpeak= %d, tailsize= %d\n",
                xcdev,thresh,minpointsinpeak,tailsize);
  return(COMPLETE);
}

/*************/
static void numregion(int retc, char *retv[])
/*************/
{
  vInfo  info;

  if (P_getVarInfo(CURRENT,"lifrq",&info))
    info.size = 0;
  if (retc>0)
    retv[0] = intString(info.size / 2);
  else if (info.size <= 1)
    Winfoprintf("there are no regions");
  else if (info.size == 2)
    Winfoprintf("there is 1 region");
  else
    Winfoprintf("there are %d regions",info.size / 2);
}

/*************/
static int getregion(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  vInfo  info;
  int    r;
  int    index;
  double high,low;

  if (argc > 1)
    if (isReal(*++argv))
      index = (int) stringReal(*argv);
    else
    {
      Werrprintf("usage - getreg<region number>");
      return(ERROR);
    }
  else
    index = 1;
  if ( (r=P_getVarInfo(CURRENT,"lifrq",&info)) )
    P_err(r,"info?","lifrq:");
  if ((index > info.size / 2) || (index < 1))
  {
    Werrprintf("region %d is not defined",index);
    return(ERROR);
  }
  if ( (r=P_getreal(CURRENT,"lifrq",&low,index * 2 - 1)) )
  { P_err(r,"lifrq",":");   return ERROR; }
  if ( (r=P_getreal(CURRENT,"lifrq",&high,index * 2)) )
  { P_err(r,"lifrq",":");   return ERROR; }
  low  -= rflrfp;
  high -= rflrfp;
  if (retc==0)
    Winfoprintf("region %d: high field edge = %g low field edge = %g",
              index,high,low);
  if (retc>0)
    retv[0] = realString(high);
  if (retc>1)
    retv[1] = realString(low);
  return(COMPLETE);
}

/*************/
int region(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int ctrace;
  int i;
  int regions;

  if (strcmp(argv[0],"numreg") == 0)
    numregion(retc,retv);
  else
  { 
    Wturnoff_buttons();
    if(init2d(1,1)) return(ERROR);
    if (strcmp(argv[0],"getreg") == 0)
      getregion(argc,argv,retc,retv);
    else
    {
      ctrace = currentindex();
      if ((spectrum = calc_spec(ctrace-1,0,FALSE,TRUE,&i))==0)
        return(ERROR);
      if (region_input(argc,argv,spectrum,1,0,fn/2,TRUE))
        return(ERROR);
      regions = select_regions(regpnt,0,fn/2);
      adjust_tails(regions);
      put_resets(regions,retc);
      if (rel_spec()) return(ERROR);
      if (retc>0)
        retv[0] = intString(regions);
    }
    releasevarlist();
    appendvarlist("lifrq");
  }
  return(COMPLETE);
}

/********************/
static void initpavco()
/********************/
{
  sigc1   = 0.0;
  sigc2   = 0.0;
  sigs1   = 0.0;
  sigs2   = 0.0;
  sigcp1  = 0.0;
  sigcp2  = 0.0;
  sigsp1  = 0.0;
  sigsp2  = 0.0;
  sigcdp1 = 0.0;
  sigcdp2 = 0.0;
  sigsdp1 = 0.0;
  sigsdp2 = 0.0;
}

/*****************************/
static int sumpavco(struct peak *speak, double beta)
/*****************************/
{
  double ux,uy,frq,radius,weight;
  double xrsin,xrcos,yrsin,yrcos;
  double ang,sinang,cosang;

  ang = beta * speak->f;
  ux  = speak->x;
  uy  = speak->y;
  frq = speak->f;
  weight = radius = ux * ux + uy * uy;
  radius = sqrt(radius);
  if ( (radius == 0.0) || isnan(ux/radius) || isnan(uy/radius) )
  {
    return(COMPLETE);
  }
  sinang = weight * sin(ang);
  cosang = weight * cos(ang);
  ux /= radius;
  uy /= radius;

  xrcos = ux * cosang;
  yrcos = uy * cosang;
  xrsin = ux * sinang;
  yrsin = uy * sinang;

  sigc1 += xrcos;
  sigc2 += yrsin;
  sigs1 += xrsin;
  sigs2 += yrcos;

  if (aph0)
    return(COMPLETE);

  sigcp1 += frq * xrsin;
  sigcp2 += frq * yrcos;
  sigsp1 += frq * xrcos;
  sigsp2 += frq * yrsin;

  frq *= frq;

  sigcdp1 += frq * xrcos;
  sigcdp2 += frq * yrsin;
  sigsdp1 += frq * xrsin;
  sigsdp2 += frq * yrcos;
  return(COMPLETE);
}

/****************************/
static double angle(double num, double denom)
/****************************/
{
  double temp;

  if (denom == 0.0)
    temp = (num < 0.0) ? -PI / 2.0 : PI / 2.0;
  else
    temp = atan(num / denom);
  if (denom < 0.0)
    temp += (num < 0.0) ? -PI : PI;
  return(temp);
}

#define LNMAXREAL 87.5
#define MAXREAL   1e37

/**************************************/
static void submitpavco(double *alpha, double *beta, double *delta, int regions)
/**************************************/
{
  register double c,s,cc,ss,ccc,sss;
  double bottom,test,test1;

  c   =  sigc1   + sigc2;
  s   =  sigs1   - sigs2;

  if (aph0)
  {
    cc = 0.0;
    ss = 0.0;
  }
  else
  {
    register double fval1, fval2, fval3;
    cc  = -sigcp1  + sigcp2;
    ss  =  sigsp1  + sigsp2;
    ccc = -sigcdp1 - sigcdp2;
    sss = -sigsdp1 + sigsdp2;

    fval1 = fabs(c);
    fval2 = fabs(cc);
    fval3 = fabs(ccc);
    test  = (fval2 > 1.0) ? log(fval2) * 2.0 : 0.0;
    test1 = (fval1 > 1.0) ? log(fval1) : 0.0;
    if (fval3 > 1.0)
       test1 += log(fval3);
    if (test < test1)
      test = test1;

    fval1 = fabs(s);
    fval2 = fabs(ss);
    fval3 = fabs(sss);
    test1 = (fval2 > 1.0) ? log(fval2) * 2.0 : 0.0;
    if (test < test1)
      test = test1;
    test1 = (fval1 > 1.0) ? log(fval1) : 0.0;
    if (fval3 > 1.0)
       test1 += log(fval3);
    if (test < test1)
      test = test1;

    if (test > LNMAXREAL)
      bottom = MAXREAL;
    else
      bottom = cc * cc + c * ccc + ss * ss + s * sss;
  }

  if ((!aph0) && (fabs(bottom) > 1e-30) && (regions > 1))
  {
    *delta = -(c * cc + s * ss) / bottom;
    *beta += *delta;
  }
  else
    *delta = 0.0;
  *alpha = -angle(s + *delta * ss, c + *delta * cc);
}

/**************************/
static int minimize(int regions, double *new_rp, double *new_lp, double *thlp)
/**************************/
{
  double alpha,beta,delta;
  double lpv,fnv;
  int    i,fni;
  int    iter;
  vInfo  info;

  DPRINT0("aph minimiziation\n");
  if (aph0)
    lpv = lp;
  else
  {
    double tmpval;

    if ( (i=P_getreal(PROCESSED,"alfa",&tmpval,1)) )
    { P_err(i,"alfa",":");   return ERROR; }
    lpv = tmpval * 1e-6;
    if ( (i=P_getreal(PROCESSED,"rof2",&tmpval,1)) )
    { P_err(i,"rof2",":");   return ERROR; }
    lpv += tmpval * 1e-6;
    if (P_getVarInfo(PROCESSED,"lsfid",&info) == 0)
      if (info.active)
      {
        if ( (i=P_getreal(PROCESSED,"lsfid",&tmpval,1)) )
        { P_err(i,"lsfid",":");   return ERROR; }
        lpv += tmpval / sw;
      }
    lpv *= -sw * 360.0;
  }

  *thlp = lpv;

  fni   = fn / 4;
  fnv   = DEG * (double) (fn / 2);
  beta  = -lpv / fnv;
  delta = 0.0;
  iter  = 0;
  DPRINT2("lpv= %g, beta= %g\n",lpv,beta);
  do
  {
    DPRINT1("iteration number %d\n",iter);
    initpavco();
    for (i = 1; i <= regions; i++)
      if ((regions == 1) || ((int) speak[i].f > fni + 2) ||
		((int) speak[i].f < fni - 2))
        sumpavco(&speak[i],beta);
    submitpavco(&alpha,&beta,&delta,regions);
    lpv = -beta * fnv;
    DPRINT5("alpha=%g beta=%g delta=%g lp=%g rp=%g\n",
                  alpha,beta,delta,lpv,beta * fnv + DEG * alpha + 90.0);
    DPRINT4("exit conditions\n delta= %g > %g and lp= %g < %g\n",
                fabs(delta * fnv),MINPHASE,fabs(lpv),MAXPHASE);
  } while ((fabs(delta * fnv) > MINPHASE) && (fabs(lpv) < MAXPHASE) &&
           (iter++ <= 100*regions));
  if (fabs(lpv) < MAXPHASE)
  {
    lp = lpv;
    rp = beta * fnv + DEG * alpha + 90.0;
    while (rp > 180.0)
      rp -= 360.0;
    while (rp <= -180.0)
      rp += 360.0;
    *new_rp = rp;   *new_lp = lp;
    return(COMPLETE);
  }
  else
    return(ERROR);
}

/**************************/
static float xxyy(double x, double y, double wx, double wy)
/**************************/
{
  double yox;

  if (fabs(x) < EPSILON)
    return((fabs(y) < EPSILON) ? wy / EPSILON : wy / y);
  else if (fabs(x) > 1.0 / EPSILON)
    return(wx / x);
  else
  {
    yox = y / x;
    return((wx + yox * wy) / (x + yox * y));
  }
}

/********************************/
static int peakok(double x, double y, double wx, double wy, double *frq, int index)
/********************************/
{
  *frq = xxyy(x,y,wx,wy);
  return((reg_start[index] <= *frq) && (reg_end[index] >= *frq));
}

/********************************/
static void sumcentroid(double *x, double *y, double *wx, double *wy, int index)
/********************************/
{
  register int i,num;
  float *ptr;
  float tmp;

  i   = reg_start[index];
  if (i < 2)
    i = 2;
  num = reg_end[index] - i - 1;
  ptr = phase_data + 2 * i;
  *x  = 0;
  *y  = 0;
  *wx = 0;
  *wy = 0;
  while (num--)
  {
    tmp = (*(ptr + 2) - *(ptr - 2)) / 200.0;
    *x  += tmp;
    *wx += i * tmp;
    ptr++;
    tmp = (*(ptr + 2) - *(ptr - 2)) / 200.0;
    *y  += tmp;
    *wy += i * tmp;
    ptr++;
    i++;
  }
}

/********************************/
static void make_centroids(int *regions)
/********************************/
{
  double re,im,wre,wim,frq;
  int    i,num;
  float ph;

  i = 1;
  num = 0;
  while (i <= *regions)
  {
    sumcentroid(&re,&im,&wre,&wim,i);
    ph=angle(re,-im)*DEG;

    DPRINT5("x= %g, y= %g, wx= %g, wy= %g index=%d\n",re,im,wre,wim,i);
    if (peakok(re,im,wre,wim,&frq,i))
    {
      num++;
      speak[num].x  = re;
      speak[num].y  = im;
      speak[num].wx = wre;
      speak[num].wy = wim;
      speak[num].f  = frq;
    }
    DPRINT2("frq= %g, num= %d\n",frq,num);
    i++;
  }
  *regions = num;
}

/*****************************************************************************
* Auto-phase region of spectrum from point "first" to "last" using 0th and
*	1st order phasing if "order" = 1 or only 0th order phasing if
*	"order" = 0.  Return values of "lp" and "rp".  
*****************************************************************************/
/***********************/
int do_aph(int argc, char *argv[], int first, int last,
           double *new_rp, double *new_lp, int order)
/***********************/
{
  int ctrace;
  int regions;
  struct datapointers datablock;
  int res;
  double thlp;

  if (order == 0)
    aph0 = 1;
  else
    aph0 = 0;

  ctrace = currentindex();
  if ((spectrum = gettrace(ctrace-1, 0)) == 0)
  {
     return(3);
  }

  if ( (res = D_getbuf(D_DATAFILE, nblocks, c_buffer, &datablock)) )
  {
     D_error(res);
     return(3);
  }

  if (checkphase(datablock.head->status))
  {
     return(3);
  }

  phase_data = datablock.data;

  find_noise(argc,argv,first,last);

  if (region_input(argc,argv,phase_data,2,first,last,FALSE))
    return(ERROR);
  if (aphMask)
  {
      float *ptr1, *ptr2, *ptr3;
      int index;
      ptr1 = phase_data = (float *) allocateWithId(sizeof(float) * fn, "aph");
      ptr2 = datablock.data;
      ptr3 = aphMask;
      index = fn/2;
      while (index--)
      {
         *ptr1++ = *ptr2++ * *ptr3;
         *ptr1++ = *ptr2++ * *ptr3++;
      }
  }
  regions = select_regions(aphpnt,first,last);
  DPRINT1("regions selected = %d\n",regions);
  make_centroids(&regions);
  DPRINT1("regions used for centroids = %d\n",regions);

  if (minimize(regions,new_rp,new_lp, &thlp))
   {
    return(2);
   }

  else if (fail_1(new_rp,new_lp, thlp, last-first)) return(2);
   
  if ( (res=D_release(D_DATAFILE,c_buffer)) )
  { D_error(res); return(ERROR); }
  if (rel_spec()) return(ERROR);
  return(COMPLETE);
}




/**********************************************/
static int fail_1(double *new_rp, double *new_lp, double thlp, int  npts)
/**********************************************/
{
 int fail, i, n, bpp, app;
 float  x, y, xn, nps, bps, aps, xav;
 float ph, ph0, std, lpv;

 fail=FALSE;

 thresh = aav + 3.0*xdev;    

 if (aph_b)  *new_rp += 180.0; /* add for phasing Bruker data, Lynn Cai, 4-7-98 */

 lpv = *new_lp;
 ph0 = *new_rp;
 
if (lpv*thlp<0&&fabs(lpv-thlp)>60.0&&fabs(thlp)>20.0) return(TRUE);
else 
 {
/*   if 	 (aav >= 10.0*xdev)  std = 10.0*xdev;
   else if (aav >= 5.0*xdev)   std = (aav+xdev*3.0);   */

  if 	 (aav >= 53.5*xdev)  std = 10.0*xdev;
  else if (aav >= 15.0*xdev)  std = 1.5*(aav+xdev*1.0);
  else if (aav >= 10.0*xdev)  std = 2.1*(aav+xdev*3.0);  
  else if (aav >= 5.0*xdev)   std = 1.5*(aav+xdev*5.0);   

  else 		
   {
   if (amax < 20.0*xdev) 	std = (aav + xdev*2.0);
   else if (amax < 40.0*xdev)   std = (aav + xdev*3.0);
   else if (amax < 100.0*xdev)  std = (aav + xdev*4.0);
   else {
		if (amax < 300.0*xdev )  	std =   0.02*amax;
		else if (amax < 500.0*xdev )  	std =   0.01*amax;
		else 				std =   0.008*amax;
	}
   }

  if (aav<53.5*xdev && std < aav + 2.0*xdev) std = aav + 2.0*xdev;

/*  if (std > 13.0*xdev)  std = 13.0*xdev;  */

/*
  if ( lpv*thlp > 0)
     {
	if (fabs(lpv - thlp) > 100.0) std /= 4.0;
	else if (fabs(lpv - thlp) > 60.0) std /= 3.0;
	else if (fabs(lpv - thlp) > 30.0) std /= 2.0;
	else if (fabs(lpv - thlp) > 20.0&&fabs(lpv/thlp)>1.5) std /= 2.0;
     }
*/

  if (fabs(lpv - thlp) < 30.0) std *= 1.3;
 }


 i = 0;
 bps = amax;
 aps = amax;
 bps = aps = xxmi;
 lpv = *new_lp /(float)npts;
 xav = 0.0;
 n = 0;

while (i<xxmi)
 { 
  ph = ph0 + lpv * (npts - i);
  x = *(phase_data + i*2);
  y = *(phase_data + i*2+1);
  xn = x*cos(ph/DEG)+y*sin(ph/DEG);
  if (xn < bps) { bps = xn; bpp = i;}
/*  if (sqrt(x*x+y*y)<=thresh) { xav += xn; n++;}  */
  i++;		
 }

  nps = bps;

  i = xxmi;
  ph = ph0 + lpv * (npts - i);
  x = *(phase_data + i*2);
  y = *(phase_data + i*2+1);
  xn = x*cos(ph/DEG)+y*sin(ph/DEG);
  if (xn < nps) nps = xn;
/*  if (sqrt(x*x+y*y)<=thresh) { xav += xn; n++;}  */
 
  i = xxmi + 1;

 while (i<npts)
  { 
  ph = ph0 + lpv * (npts - i);
  x = *(phase_data + i*2);
  y = *(phase_data + i*2+1);
  xn = x*cos(ph/DEG)+y*sin(ph/DEG);
  if (xn < aps) {aps = xn; app = i;}
/*  if (sqrt(x*x+y*y)<=thresh) { xav += xn; n++;}  */
  i++;	
 }


 if (aps < nps) nps = aps;
/*  if (n != 0) xav = xav / (float) n;  */


/*  if (nps < xav - 3.0*xdev) fail = TRUE;  */

 if (nps<-std) fail=TRUE;

/*
 else if (fabs(bps-aps)>std/2.0&&((xxmi-bpp)/(app-xxmi)>1.15||(app-xxmi)/(xxmi-bpp)>1.15)) fail = TRUE;
*/

 if (fail) return (TRUE);
 else return (FALSE);


}

int bph() {
  double d;
  if (P_getreal(CURRENT,"blockph",&d,1)) d = 0;
  if(d > 0 && !d2flag && nblocks > 1) return (int)d;  
  else return 0;
}

/*************/
int aph(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  double new_rp, new_lp;
  int res;
  int peakmax = 0;
  int lparg = 0;
/*
  PFI cmd;
 */
  int done;
  double save_lp;

  vInfo paraminfo;
  if(P_getVarInfo(CURRENT, "blockph", &paraminfo) != -2) {
    P_setreal(CURRENT,"blockph", 0.0, 1);
  }

#ifdef CLOCKTIME
  /* Turn on a clocktime timer */
  if ( start_timer ( 3 ) != 0 )
  {  Werrprintf ( "aph:  \"start_timer ( 3 )\" fails\n" );
     fprintf ( stderr, "aph:  \"start_timer ( 3 )\" fails\n" );
     exit ( 1 );
  }
#endif

  aph0 = (strcmp(argv[0], "aph0") == 0);

  aph_b = (strcmp(argv[0], "aphb") == 0);  /* add for phasing Bruker data, Lynn Cai, 4-7-98 */
   
  Wturnoff_buttons();
  start_from_ft=1;

  if(init2d(1, 1))
  {
     return ERROR;
  }
  if (argc > 1)
  {
     if ( ! strcmp(argv[1],"lp") )
       lparg = 1;
     if ( !strcmp(argv[1],"peakmax") )
       peakmax = 1;
     if (argc > 2)
     {
        if ( ! strcmp(argv[2],"lp") )
          lparg = 1;
        if ( !strcmp(argv[2],"peakmax") )
          peakmax = 1;
     }
     if (lparg)
     {
        argc--;
        argv++;
     }
     if (peakmax)
     {
        argc--;
        argv++;
     }
  }

  if (datahead.status & S_DDR)
  {
    /*
     * aph for DDR data defaults to aph0 with lp=0.0
     * unless an "lp" argument is given, in which case
     * both rp and lp are optimized
     */
    if ( ! lparg )
    {
       /* if aph0 is called, lp is left alone. Only aph sets it to 0 */
       if ( ! aph0)
          lp = 0.0;
       aph0 = 1;
    }
  }
  if (peakmax)
     aph0 = 1;
  save_lp = lp;

/*
  cmd = (strcmp(argv[0], "aphold") == 0) ? do_aph : do_aph_n;

  if ( (res = cmd(argc,argv,0,fn/2,&new_rp,&new_lp,aph0 ? 0 : 1)) == 0)
  {
    if (retc > 0)
      retv[0] = intString(TRUE);
    if (retc > 1)
    {
      retv[1] = realString(new_rp);
      if (retc > 2)
        retv[2] = realString(new_lp);
    }
    else
    {
      lp = new_lp;  rp = new_rp;
      P_setreal(CURRENT,"lp",new_lp,0);
      P_setreal(CURRENT,"rp",new_rp,0);
      releasevarlist();
      appendvarlist("rp,lp");
    }
  }

*/



/* added Feb. 26, 97 to combine aphold and aph */


/*  find_noise(argc,argv,0,fn/2);  */
  done = 0;
  while (!done)
  {
     pv = 0.0;
     new_rp = new_lp = 0.0;

     if ( (res = do_aph(argc,argv,0,fn/2,&new_rp,&new_lp,aph0 ? 0 : 1)) == 0)  
     {
        if (retc > 0)
          retv[0] = intString(TRUE);
        if (retc > 1)
        {
          retv[1] = realString(new_rp);
          if (retc > 2)
            retv[2] = realString(new_lp);
        }
        else
        {
          lp = new_lp;  rp = new_rp;
          P_setreal(CURRENT,"lp",new_lp,0);
          P_setreal(CURRENT,"rp",new_rp,0);
          releasevarlist();
          appendvarlist("rp,lp");
        }
        done = 1;
     }
     else if ( (res != 3) && (res = do_aph_n(0,fn/2,&new_rp,&new_lp,aph0 ? 0 : 1))
        == 0)  
     {
        if (retc > 0)
          retv[0] = intString(TRUE);
        if (retc > 1)
        {
          retv[1] = realString(new_rp);
          if (retc > 2)
            retv[2] = realString(new_lp);
        }
        else
        {
          lp = new_lp;  rp = new_rp;
          P_setreal(CURRENT,"lp",new_lp,0);
          P_setreal(CURRENT,"rp",new_rp,0);
          releasevarlist();
          appendvarlist("rp,lp");
        }
        done = 1;

     }  /*----- end of conbination -------*/
     if (res == 3)
        done = -1;
     if (!done)
     {
          if (!aph0)
          {
             /* Try with aph0 */
             lp = save_lp;
             aph0 = 1;
          }
          else
          {
             /* aph0 also failed, exit while loop */
             done = -1;
          }
     }
  }

  
  /* res == 3 means there is no data or the data is
   * not phaseable (e.g., av or pwr)
   */
  if ( (done == -1) && (res == 3) )
  {
    releaseAllWithId("aph");
    if (retc > 0)
    {
       retv[0] = intString(FALSE);
       return(COMPLETE);
    }
    else
    {
       Werrprintf("Cannot phase this data");
       return(ERROR);
    }
  }
  /* aph0 failed. Simply adjust rp to optimize integral */
  if (done == -1 || peakmax)
  {
     float max, val;
     int ctrace;
     double rpmax;

     lp = save_lp;
     new_rp = rp = -180.0;
     max = 0.0;
     disp_status_OnOff(1);
     ctrace = currentindex();
     /* quick search in 1 degree steps */
     while (rp <= 180.0)
     {
         spectrum = gettrace(ctrace-1, 0);
         if (aphMask)
         {
            int index;
            float *ptr1, *ptr2, *ptr3;
            index = fn/2;
            ptr1 = phase_data;
            ptr2 = spectrum;
            ptr3 = aphMask;
            while (index--)
               *ptr1++ = *ptr2++ * *ptr3++;
            if (peakmax)
               maxfloat2(phase_data,fn/2, &val);
            else
               integ2(phase_data,0,fn/2, &val, 1);
         }
         else
         {
            if (peakmax)
               maxfloat2(spectrum,fn/2, &val);
            else
               integ2(spectrum,0,fn/2, &val, 1);
         }
         if (val > max)
         {
            max = val;
            new_rp = rp;
         }
         rp += 2.0;
     }
     rpmax = new_rp + 1.9;
     rp = new_rp - 1.9;
     /* fine search in 0.1 degree steps */
     while (rp <= rpmax)
     {
         spectrum = gettrace(ctrace-1, 0);
         if (aphMask)
         {
            int index;
            float *ptr1, *ptr2, *ptr3;
            index = fn/2;
            ptr1 = phase_data;
            ptr2 = spectrum;
            ptr3 = aphMask;
            while (index--)
               *ptr1++ = *ptr2++ * *ptr3++;
            integ2(phase_data,0,fn/2, &val, 1);
         }
         else
            integ2(spectrum,0,fn/2, &val, 1);
         if (val > max)
         {
            max = val;
            new_rp = rp;
         }
         rp += 0.1;
     }
     disp_status_OnOff(0);
     rp = new_rp;
     if (retc > 0)
        retv[0] = intString(TRUE);
     if (retc > 1)
     {
        retv[1] = realString(rp);
        if (retc > 2)
           retv[2] = realString(lp);
     }
     else
     {
        rp = new_rp;
        P_setreal(CURRENT,"lp",lp,0);
        P_setreal(CURRENT,"rp",rp,0);
        releasevarlist();
        appendvarlist("rp,lp");
     }
  }

  releaseAllWithId("aph");
#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  if ( stop_timer ( 3 ) != 0 )
  {  Werrprintf ( "aph:  \"stop_timer ( 3 )\" fails\n" );
     fprintf ( stderr, "aph:  \"stop_timer ( 3 )\" fails\n" );
     exit ( 1 );
  }
#endif

  return(COMPLETE);
}

/***********************/
int do_aph_n(int first, int last, double *new_rp, double *new_lp, int order)
/***********************/
{
  int ctrace;
  int regions, good_regions, normal, invert;
  struct datapointers datablock;
  int res,npts,i;
  int REF,not_good, php;
  float lpv, initlp, secondlp, thlp, ph, xm;

  if (order == 0)
    aph0 = 1;
  else
    aph0 = 0;

  ctrace = currentindex();
  if ((spectrum = gettrace(ctrace-1, 0)) == 0)
  {
     return(ERROR);
  }

  if ( (res = D_getbuf(D_DATAFILE, nblocks, c_buffer, &datablock)) )
  {
     D_error(res);
     return COMPLETE;
  }

  if (checkphase(datablock.head->status))
  {
     return(3);
  }

  regions = 0;
  if (aphMask)
  {
      float *ptr1, *ptr2, *ptr3;
      int index;
      ptr1 = phase_data;
      ptr2 = datablock.data;
      ptr3 = aphMask;
      index = fn/2;
      while (index--)
      {
         *ptr1++ = *ptr2++ * *ptr3;
         *ptr1++ = *ptr2++ * *ptr3++;
      }
  }
  else
  {
     phase_data = datablock.data;
  }


  if (!find_regions(2,first,last,&regions)) 
	return(2);
  DPRINT1("regions found = %d\n",regions);
  npts=last-first;

  if (aph0)
   {
    i=0; 
    REF=0;
    while (i<regions)
     {
	if (speakB[i].php == amax_index)
        {
           REF=i;
           i=regions;
        }
	i++;
     }
    initlp=0.0;
    res=adjust_ph(REF,&ph, &php, &xm, initlp, &good_regions);
    if ((res==1 && good_regions==1)||regions<=2)
      {
	speakB[REF].phase=ph;
	speakB[REF].php = php;
	speakB[REF].state=1;
      } 
    else
     {
      i=0;
      while ((!res||good_regions!=1)&&i<regions)  
      {
       if (i!=REF)
	{
        res=adjust_ph(i,&ph, &php, &xm, initlp, &good_regions);
	if (res&&good_regions)  
	 {
	  REF=i;
	  speakB[REF].phase=ph;
	  speakB[REF].php = php;
	  speakB[REF].state=1;
	 }
	}
       i++;
       }
      }  
    lpv=-lp;
   }

  else
   {
    thlp = 0.0;
    init_lp(&thlp);  /*init_lp()  calculates the theory value of lp  */
    lpv=thlp;
    if (fabs(lpv)<150.0) lpv=0.0;
    else if (lpv>0.0) lpv -= 100.0;
    else if (lpv<0.0) lpv += 100.0;
    initlp=-lpv; 

    phase_regions(&regions, &good_regions, initlp, npts);
  
    if (regions==1) {REF = 0; lpv = -thlp; }
    else 
      {
  	first_rp(regions, &good_regions, &REF);

	second_lp(regions,good_regions, &lpv,&REF,npts);

	if (lpv == 0.0) lpv = -thlp;
	else lpv += initlp;
	if (initlp!=0.0)
	   {
	     for (i=0; i<regions;i++)
	   	speakB[i].phase += initlp * (float) speakB[i].php / (float) npts; 
	   }


        secondlp=0.0;
	if (lpv != 0.0) 
	     {
		secondlp=lpv;
                classify_regions(regions,&not_good,&normal,&invert, lpv, REF,npts);
	   	if (not_good>3||regions-not_good<2) {lpv=0.0; correct_state(regions);}
	   	else  correct_phase(regions); 

/*		final_lp(regions,&lpv);
                if (fabs(secondlp - lpv) > 360.0) lpv = secondlp;
*/
	     }

	   if ( lpv/thlp > 0.7 && fabs(thlp)>10.0)
	     {
	      	if (secondlp*thlp < 0 && -secondlp/thlp <= 0.7) lpv = secondlp;
		else lpv= -thlp;
	     }
	   else if( -lpv/thlp > 6.5 && -lpv/thlp <20.0 && fabs(thlp)>10.0)  lpv=-thlp;
	   else if (lpv == 0.0) lpv = -thlp;

	   if (fabs(lpv - secondlp) > 30.0) 
	     {
		correct_phase(regions);
		correct_state(regions);
                classify_regions(regions,&not_good,&normal,&invert, lpv, REF,npts);
	   	if (not_good>3||regions-not_good<2) {lpv=0.0; correct_state(regions);}
	   	else  correct_phase(regions); 
	     }


  	if (fail(regions,REF,lpv, npts)==1) 
  	  { if (secondlp != lpv) 
	      {
		correct_phase(regions);
		correct_state(regions);
                classify_regions(regions,&not_good,&normal,&invert, secondlp, REF,npts);
	   	if (not_good>3||regions-not_good<2) {lpv=0.0; correct_state(regions);}
	   	else  correct_phase(regions); 

	 	if (fail(regions,REF, secondlp, npts)==0) lpv = secondlp;
	 	else if (lpv != -thlp)
			{lpv = -thlp;
	 	 	 if (fail(regions,REF, lpv, npts))  return(2);
			}
	     	else return(2);
	      }
           else if (lpv != -thlp)
	     { 
	 	lpv = -thlp;

		correct_phase(regions);
		correct_state(regions);
                classify_regions(regions,&not_good,&normal,&invert, lpv, REF,npts);
	   	if (not_good>3||regions-not_good<2) {lpv=0.0; correct_state(regions);}
	   	else  correct_phase(regions); 

	 	if (fail(regions,REF, lpv, npts)==1)  return(2);
	     }
      	   else return(2);
    	  }

       }	 
  }
 

  *new_lp=(double) -lpv;
  *new_rp=(double) (speakB[REF].phase-lpv*((double)speakB[REF].php/(double)npts-1.0));
//Winfoprintf("bph4 %d %d %f %f %f %f",ctrace,npts, speakB[REF].phase,lpv,speakB[REF].php,*new_rp);

  while (*new_rp > 180.0) *new_rp -= 360.0;
  while (*new_rp <= -180.0) *new_rp += 360.0;

  if ( (res=D_release(D_DATAFILE,c_buffer)) )
	  { D_error(res); return(ERROR); }
  if (rel_spec()) return(ERROR);
  return(COMPLETE);
}


/**************************/
static int init_lp(float *new_lp)
/**************************/
{
  double lpv;
  int    i;
  vInfo  info;
  double tmpval;

  DPRINT0("aph minimiziation\n");
  if (aph0)
    lpv = lp;
  else
  {
    if ( (i=P_getreal(PROCESSED,"alfa",&tmpval,1)) )
    { P_err(i,"alfa",":");   return ERROR; }
    lpv = tmpval * 1e-6;
    if ( (i=P_getreal(PROCESSED,"rof2",&tmpval,1)) )
    { P_err(i,"rof2",":");   return ERROR; }
    lpv += tmpval * 1e-6;
    if (P_getVarInfo(PROCESSED,"lsfid",&info) == 0)
      if (info.active)
      {
        if ( (i=P_getreal(PROCESSED,"lsfid",&tmpval,1)) )
        { P_err(i,"lsfid",":");   return ERROR; }
        lpv += tmpval / sw;
      }
    lpv *= -sw * 360.0;
  }
  *new_lp=(float) lpv;
  return COMPLETE;
}


/**********************************************/
static int fail(int regions, int REF, float lpv, int npts)
/**********************************************/
{
 int fail, r, i, php, j1, j2;
 float  x, y, xn, nps;
 float ph, ph0, std, std1;

 fail=FALSE;

 lpv = lpv /(float)npts;

 std=pv*0.2;

 if (std > 0.1*amax) std = 0.1*amax;

 if (std<xdev*5.0) std = xdev*5.0;

r=0;
while (!fail&&r<regions)
 {
  i=speakB[REF].php; 
  if (speakB[r].state!=-1)
  { 
    std1=0.1*speakB[r].amax;
    if (std1<std) std1=std;

    if (speakB[r].state==3||speakB[r].state==15||speakB[r].state==18) 
          ph0=180.0 + speakB[REF].phase-(lpv)*(float)i;
    else  ph0=speakB[REF].phase-(lpv)*(float)i;

  j1 = reg_start[r];
  j2=reg_end[r];

  php=speakB[r].php;
  i=speakB[r].php; 

  ph=ph0+lpv * (float) i;
  x = *(phase_data + i*2);
  y = *(phase_data + i*2+1);
  nps= x*cos(ph/DEG)+y*sin(ph/DEG);

  if (nps<-std1) fail=TRUE;
  else 
   {
    for (i=j1;i<php;i++)
     {
   	ph=ph0+lpv * (float)i;
   	x = *(phase_data + i*2);
   	y = *(phase_data + i*2+1);
   	xn= x*cos(ph/DEG)+y*sin(ph/DEG);
   	if (xn<nps) nps = xn;
     }

     if (nps<-std1) fail=TRUE;
     else 
      {
       for (i=php+1; i<j2;i++)
  	{
   	 ph=ph0+lpv * (float)i;
   	 x = *(phase_data + i*2);
   	 y = *(phase_data + i*2+1);
   	 xn= x*cos(ph/DEG)+y*sin(ph/DEG);
    	 if (xn<nps) nps = xn;
  	}
       if (nps<-std1) fail=TRUE;
      }
    }
  }
  r++;
 }
 
 if (fail) return (TRUE);
 else return (FALSE);

}

#ifdef XXX
/**********************************************/
static int final_lp(int regions, float *new_lp)
/**********************************************/
{

/* use minimization */

float s0, s1, s2, c0,c1,c2;
float delta;
float lpv;
int i, npts,iter;


  DPRINT0("aph minimiziation\n");


  npts = fn / 2;

  lpv=*new_lp/(float)npts;

  delta = 0.0;
  iter  = 0;

  do
  {
    DPRINT1("iteration number %d\n",iter);

    s0 = s1 = s2 = c0 = c1 = c2 = 0.0;

    for (i = 0; i < regions; i++)      
          sumvalue(&s0,&s1,&s2, &c0,&c1,&c2,i,lpv);

    if (!aph0) 
       {	
    	submitvalue(&delta,s0,s1,s2,c0,c1,c2);
        lpv +=delta; 
       }

  } while ((fabs(delta*npts) > MINPHASE) && (fabs(lpv*npts) < 3600.0)&&(iter++<100));
 

  if (fabs(lpv*npts) < 3600.0)
   {
    *new_lp = lpv*(float)npts;
   }

 return(COMPLETE);
}


/*****************************/
static int sumvalue(float *s0, float *s1, float *s2, float *c0,
                    float *c1, float *c2, int i, float lpv)
/*****************************/
{
  float A, ph, frq;

  A  = speakB[i].amax*speakB[i].amax;
  if (speakB[i].state>=5) A /=4.0;
  else if (speakB[i].state<0) A=0.0;


  frq = (float)speakB[i].php;

  ph  = (lpv*frq-speakB[i].phase)/DEG;

  *s0 += A*sin(ph);
  *s1 += frq*A*sin(ph);
  *s2 += frq*frq*A*sin(ph);

  *c0 += A*cos(ph);
  *c1 += frq*A*cos(ph);
  *c2 += frq*frq*A*cos(ph);

  return(COMPLETE);
}



/**************************************/
static int submitvalue(float *delta, float s0, float s1, float s2,
                       float c0, float c1, float c2)
/**************************************/
{
  float B1,B2,D;

  B1=c0*c2+s0*s2;
  B2=(c0*c1+s0*s1)*(c0*c1+s0*s1)/(c0*c0+s0*s0);
  D=(s0*c1-c0*s1);

  *delta = D / (B1-B2);


return(COMPLETE);

}
#endif



/******************************************************************/
static int classify_regions(int regions, int *not_good, int *normal, int *invert,
                            float lpv, int REF, int npts)
/******************************************************************/
{
int i, r_php, php;
float ph, dph, r_ph;

r_php=speakB[REF].php;
r_ph=speakB[REF].phase;
speakB[REF].state=1;

*not_good=0;
*normal=1;
*invert=0;

speakB[REF].state=1;

for (i=0;i<regions;i++)
  {
   if (i!=REF)
     {
	if (speakB[i].state==50) speakB[i].state=5;

	php=speakB[i].php;
   	ph = r_ph+lpv*(php - r_php)/npts;
	dph = ph - speakB[i].phase;

 	if (cos(dph/DEG)>0.6) 
	 {
	  *normal +=1;
	  if (speakB[i].state<5) speakB[i].state=2;
	  else speakB[i].state *=2;
	 }
	else if (cos(dph/DEG)<-0.6) 
	 {
	  *invert +=1;
	  if (speakB[i].state<5) speakB[i].state=3;
	  else speakB[i].state *=3;
	 }
	else 
	 { 
	  if (speakB[i].state==0) speakB[i].state = -1;
	  else speakB[i].state *= -1;
	  *not_good +=1;		       		     
	 }
     }
  }

return(COMPLETE);


/* status for a region : -1 --> not good for phasing;
			  0 --> good but don't know other properties (nothing is done for this region) 
			  1 --> be the reference
			  2 --> normal group 
			  3 --> invert group
			  4 --> not good for reference
			  5 --> not very good for calculating phasing but used for check lp value;
			  6 --> not good for calculating phasing but used for check final lp value;

*/

}



/******************************************************************/
static int correct_state(int regions)
/******************************************************************/
{
int i;

/* classification failed, go back to original state  */


for (i=0;i<regions;i++)
 {
  if (speakB[i].state < 0) 
       	  speakB[i].state *=-1; 
  else if(speakB[i].state==3) 
       	  speakB[i].state = 0;
  else if(speakB[i].state==10||speakB[i].state==12) 
       	  speakB[i].state /=2;  
  else if(speakB[i].state==15||speakB[i].state==18) 
       	  speakB[i].state /=3;  
 }

return (COMPLETE);

}


/******************************************************************/
static int correct_phase(int regions)
/******************************************************************/
{
int i;

/* for invert lines,  phase += 180.0  */


for (i=0;i<regions;i++)
 {
  if (speakB[i].state==3||speakB[i].state==15||speakB[i].state==18) 
    
      speakB[i].phase += 180.0;    
 }

return (COMPLETE);

}


/****************************************************/
static int find_noise(int argc, char *argv[], int first, int last)
/****************************************************/
{
  int npts,i,nn,k;
  float  xvalue, yvalue, value, xmx,xmi,amv,xmv, xav;
  float *spectr;
  double vv,cor1,cor2;
  vInfo  info;
  int r;

 spectr=phase_data;
 npts=last-first;

  nn = npts/20;
  if (nn < 20)
    nn = 20;
  else if (nn > 200)
    nn = 200;

  aav= 0.0;
  xmv=0.0;
  for (i=0; i<nn; i++)
   {
    xvalue = *(spectr+2*i);
    yvalue = *(spectr+2*i+1);
    value = sqrt (xvalue*xvalue+yvalue*yvalue);    
    xmv += xvalue; 
    aav += value; 
   }

   aav /=(float)nn;
   xav=xmv/(float)nn;
   k=0;

  xmv=0.0;
  amv= 0.0;
  for (i=npts/20; i<npts/20+nn; i++)
   {
    xvalue = *(spectr+2*i);
    yvalue = *(spectr+2*i+1);
    value = sqrt (xvalue*xvalue+yvalue*yvalue);    
    xmv += xvalue; 
    amv += value; 
   }
  amv /=(float)nn;

  if(aav>amv) {k=1; aav=amv;xav=xmv/(float)nn;}

  xmv=0.0;
  amv= 0.0;
  for (i=npts/10; i<npts/10+nn; i++)
   {
    xvalue = *(spectr+2*i);
    yvalue = *(spectr+2*i+1);
    value = sqrt (xvalue*xvalue+yvalue*yvalue);    
    xmv += xvalue; 
    amv += value; 
   }
  amv /=(float)nn;

  if(aav>amv) {k=2; aav=amv;xav=xmv/(float)nn;}

  xmx=-32768.0;
  xmi=32768.0;
  xmv=0.0;
  for (i=k*npts/20; i<k*npts/20+nn; i++)
   {
    xvalue = *(spectr+2*i);
    yvalue = *(spectr+2*i+1);
    value = sqrt (xvalue*xvalue+yvalue*yvalue);    
    xmv += (xvalue-xav)*(xvalue-xav); 
    if(xvalue>xmx) xmx=xvalue;
    if(xvalue<xmi) xmi=xvalue;
   }

  xdev=sqrt(xmv/nn);


  cor1=cor2=0.0;
  if ( (r=P_getVarInfo(PROCESSED,"lb",&info)) )
  { P_err(r,"lb",":");   return ERROR; }
  if (info.active)
  {
    if ( (r=P_getreal(PROCESSED,"lb",&vv,1)) )
    { P_err(r,"lb",":");   return ERROR; }
    cor1 +=vv;
  }
  if ( (r=P_getVarInfo(PROCESSED,"gf",&info)) )
  { P_err(r,"gf",":");   return ERROR; }
  if (info.active)
  {
    if ( (r=P_getreal(PROCESSED,"gf",&vv,1)) )
    { P_err(r,"gf",":");   return ERROR; }
    cor2 += 0.8 / vv;
  }
 

  vv=cor1+ 0.7*cor2;
  if (vv>4.0) vv=4.0;
  noisefactor=2.0+ (float) vv; 

  peakfactor=(float) npts /16384.0;
  if (peakfactor>1.0) peakfactor=1.0;

  vv=1.5*cor1+ cor2/2.0;
  minpointsinpeak= 2 + (float) vv*peakfactor;
  if(minpointsinpeak>30) minpointsinpeak=30;

  thresh = 0.0;

  if((argc-- > 1) && isReal(*++argv))
     thresh = stringReal (*argv);

  if (thresh<=0.0||thresh>=aav+10.0*xdev)   
    thresh = aav + 3.0*xdev;


   amax=0.0;
   amax_index=0;
   aav=0.0;
   pnav = 0;
   xxmi = 0;

   for (i=0; i<npts; i++)
    {
    xvalue = *(phase_data+2*i);
    yvalue = *(phase_data+2*i+1);
    value = sqrt (xvalue*xvalue+yvalue*yvalue); 
    if (amax < value) {amax=value; amax_index=i; xxmi = i;}
    if (value < thresh)  aav += value; 
    else { 
	   aav += thresh;   
	   pnav += 1; 
	 }
    }

     aav = aav/npts;   

/*     if (npts-pnav!=0) aav = aav/(float)(npts-pnav);  */



  return(COMPLETE);

}


/*********************************************/
static int find_regions(int incr, int first, int last, int *regions)
/******************************************/
{
  int npts, i, k;
  int sindex,eindex;
  int r_number;
  float r_maxv;
  int ln, r_maxp, ltail,rtail;
  float *valuearray, *vptr;

  float  ppv, pdev,udev,xvalue,yvalue,value;

   pdev=0.0;
   udev=0;
   pnav=0;
   i=0;
   *regions=0;

   thresh = aav + 3.0*xdev;

   invivofactor=100.0;
   invivo=FALSE;

   npts = last-first; 
   if ((valuearray = (float *) allocateWithId(sizeof(float) * npts,"findreg"))==0)
   {
     Werrprintf("cannot allocate buffer for region selection");
     return(ERROR);
   }
   vptr = valuearray;

   while (i<npts)
    {
    xvalue = *(phase_data+2*i);
    yvalue = *(phase_data+2*i+1);
    *vptr++ = value = sqrt (xvalue*xvalue+yvalue*yvalue); 
    i++;
    if (value<=thresh)
    {
       udev+=thresh-value;
    } 
    else
      {
       *regions += 1;
       while (value > thresh&&i<npts)
        {
         pnav++; 
	 pdev += value - thresh;
         if (i<npts)
	  {xvalue = *(phase_data+2*i);
   	  yvalue = *(phase_data+2*i+1);
    	  *vptr++ = value =  sqrt(xvalue*xvalue+yvalue*yvalue);
	  i++;   
          }
 	}      
        if (i < npts)
        {
           udev+=thresh-value;
        }
      }
    }

   if (npts-pnav!=0) udev /=(float)(npts-pnav);
   if (pnav!=0) pdev = pdev/(float)pnav;

  while (*regions>60)
  {  
   thresh += xdev;

   pdev=0.0;
   udev=0;
   pnav=0;
   i=0;
   *regions=0;
   vptr = valuearray;

   while (i<npts)
    {
    value = *vptr++;
    i++;
    if (value<=thresh) {udev+=thresh-value; } 
    else
      {
       *regions += 1;
       while (value > thresh&&i<npts)
        {
         pnav++; 
	 pdev += value - thresh;
	 if (i<npts)
	 {
    	  value = *vptr++;
	  i++;   
         }   
 	}      
        if (i < npts)
        {
           udev += thresh-value;
        }
      }
    }
   if (npts-pnav!=0) udev /=(float)(npts-pnav);
   if (pnav!=0) pdev=pdev/(float)pnav;

   }
 
k=1;
if (pnav!=0)
{
 
 while (npts/pnav<4&&*regions>1) 
  {
   if (k>2) thresh += (xdev>0.21*pdev/(float) (k-2))? xdev:0.21*pdev/(float) (k-2);
   else thresh += (xdev>0.21*pdev)? xdev:0.21*pdev;
 
   k++;

   pdev=0.0;
   udev=0;
   pnav=0;
   i=0;
   pv=0.0;
   *regions=0;
   vptr = valuearray;

   while (i<npts)
    {
    value = *vptr++;
    i++;
    if (value>thresh)
      {
       *regions += 1; ppv=0.0;
       while (value > thresh&&i<npts)
        {
         pnav++; 
	 pdev += value - thresh;
         if (value>ppv) ppv=value;
	 if (i<npts)
	 {
    	  value =  *vptr++;
	  i++;   
         }   
 	}    
       pv +=ppv;  
      }    
    }

   if (pnav!=0) { pdev=pdev/(float)pnav;}
   if (*regions!=0) pv=pv/(float) *regions;
  }

}

 
 while (*regions<2&&thresh>aav+2.5*xdev)
 {   
   thresh -= 0.5*xdev ;

   pdev=0.0;
   udev=0;
   pnav=0;
   i=0;
  *regions=0;
   vptr = valuearray;

   while (i<npts)
    {
    value = *vptr++;
    i++;
    if (value<=thresh) {udev+=thresh-value; } 
    else
      {
       *regions += 1;
       while (value > thresh&&i<npts)
        {
         pnav++; 
	 pdev += value - thresh;
	 if (i<npts)
	 {
      	  value =  *vptr++;
          i++;
         }   
 	}      
      }
    }
   if (npts-pnav!=0) udev /=(float)(npts-pnav);
   if (pnav!=0) pdev=pdev/(float)pnav;

 }
 
  releaseWithId("findreg");

  if (*regions>MAXSAV||*regions==0)
     return(FALSE);


  sindex = eindex= first;
  r_number = 0;
  peakfactor =thresh/pdev;
  ltail = 0;

  invivofactor = 1.0+ pv/thresh;
 
  if (k>3&&invivofactor<5.0)  invivo=TRUE; 
 
  if(pv<50.0*xdev) minpointsinpeak /= 2; 

  while ( eindex < npts)
  { i=find_peak(&sindex, &eindex, incr, npts, &r_maxv,&r_maxp,&ltail,&rtail,&ln);
    if (i==TRUE)
    {
     if(r_maxp>npts/40&&r_maxp<(last-npts/40) && (r_number < MAXSAV))
       {
    	reg_start[r_number] = sindex;
    	reg_end[r_number] = eindex;
    	speakB[r_number].linenum=ln;
    	speakB[r_number].amax=r_maxv;
    	speakB[r_number].php=r_maxp;
    	speakB[r_number].ltail=ltail;
    	speakB[r_number].rtail=rtail;  
	DPRINT3("number= %d, start= %d, end= %d\n",
                r_number, reg_start[r_number],reg_end[r_number]);
	r_number++;	
       } 
     sindex = eindex;
    }
  }

  if (r_number>1)
     group_peaks(&r_number);
   
  *regions=r_number;

  if (r_number==0) return(FALSE);
  else return(TRUE);

}

/********************************************************************************/
static int find_peak(int *sindex, int *eindex, int incr, int npts,
                     float *reg_maxv, int *reg_max, int *ltail, int *rtail, int *ln)
/************************************************************************************/
{
int i, ok, np_inpeak, line_done;
int tail,line_max, line_begin, line_end, last_point;
float line_maxv, line_beginv, line_endv;
float  xvalue, yvalue, value;

*reg_maxv=0.0;
*ln=0;
*reg_max=0;
i=*sindex; 
ok=FALSE;

xvalue = *(phase_data + i*incr);
yvalue = *(phase_data + i*incr+1);
value = sqrt (xvalue*xvalue+yvalue*yvalue);    
i++;
np_inpeak=0;
last_point = line_end=0;

while (!ok&&i<npts)
 { 
   if (value <= thresh) 
    {
     if (i<npts)
	  {xvalue = *(phase_data + i*incr);
     	   yvalue = *(phase_data + i*incr+1);
     	   value = sqrt (xvalue*xvalue+yvalue*yvalue);
           i++;
          }
    }
   else 
    {
     line_begin = i - 1;
     line_beginv = value;
     line_endv=value; 
     line_end = i - 1;
     line_maxv = value;
     line_max = i - 1;
     line_done = 0;
     while (line_done<2&&i<npts) 
       {
	if (i<npts)
	  {xvalue = *(phase_data + i*incr);
     	   yvalue = *(phase_data + i*incr+1);
    	   value = sqrt (xvalue*xvalue+yvalue*yvalue);
           i++;
           if (value < thresh) {line_done = 2; ok = TRUE;}
	   else 
            {
            if (value > line_endv +3.0*xdev&&line_done==1)
            {
              line_done = 2;
/*
              ok = TRUE;
 */
            }
	    else  
               { 
                if (value < line_maxv - 3.0*xdev) 
		   {line_done=1;line_endv=value; line_end=i-1;}
        	if (value > line_maxv) 
		   {line_maxv=value; line_max=i-1;line_endv=value; line_end=i-1;}
               }
            }
          }
        } 
/*
      ok = (line_done > 0) ? TRUE : FALSE;
 */
      np_inpeak +=line_end-line_begin+1;
      if (np_inpeak<minpointsinpeak+3) {np_inpeak=0; ok=FALSE;*reg_maxv=0.0;}
      else{
        *ln += 1;     
        if (line_maxv-line_beginv!=0) 
	  tail= (int) (line_maxv * (line_max - line_begin) / (line_maxv-line_beginv));
        else tail=(line_max-line_begin+3);        
        if (*ln==1) {*sindex = line_begin-1;*ltail = 1.5*tail-line_max+*sindex;}
        else  { if (1.5*tail>*ltail+line_max-*sindex) *ltail = 1.5*tail-line_max+*sindex;  }

        if (line_maxv-line_endv!=0) 
	   tail= (int) (line_maxv * (line_end - line_max) / (line_maxv-line_endv));
        else tail=(line_end-line_max+3);  

        if (*ln==1) {last_point=line_max+1.5*tail; }
        else  { if (last_point<1.5*tail+line_max) last_point=1.5*tail+line_max;}
	 
        if (*reg_maxv<line_maxv) {*reg_maxv=line_maxv; *reg_max=line_max;}
       }
      if ( (ok && (np_inpeak<2*minpointsinpeak+6 && *reg_maxv< 2.0*thresh)) ||
           (np_inpeak<3*minpointsinpeak+9 && *reg_maxv< 1.5*thresh) )
       {
       	  ok=FALSE;
	  *reg_maxv=0.0;
	  *ln=0;
	  np_inpeak=0;	  
       }

    }  /*end of else */

 } /*end of while !ok*/

 *eindex=i;
 *rtail=(last_point-line_end);


if(ok)
  {

  if (invivofactor<5.0&&invivo)
       {  
	 *ltail /= 1.0+2.5/invivofactor; 
	 *rtail /= 1.0+2.5/invivofactor; 
       }
 
  if (*reg_maxv>= 20.0* (float) thresh && (*ltail>npts/20||*rtail>npts/20)) 
	{

	 *ltail *= 0.5;
	 *rtail *= 0.5; 
 	if (*ltail>(*reg_max-*sindex)/2.0) *ltail=(*reg_max-*sindex)/2.0;
 	if (*rtail>(*eindex-*reg_max)/2.0) *rtail=(*eindex-*reg_max)/2.0;
	}
  else if (*reg_maxv>= 10.0* (float) thresh) 
	{

	 *ltail *= 0.8;
	 *rtail *= 0.8; 
 	if (*ltail>0.8*(*reg_max-*sindex)) *ltail=(*reg_max-*sindex)*0.8;
 	if (*rtail>0.8*(*eindex-*reg_max)) *rtail=(*eindex-*reg_max)*0.8;
	}
  else if (*reg_maxv>= 8.0* (float) thresh) 
	{
 	if (*ltail>(*reg_max-*sindex)) *ltail=(*reg_max-*sindex);
 	if (*rtail>(*eindex-*reg_max)) *rtail=(*eindex-*reg_max);
	}

  else if (*reg_maxv>= 5.0* (float) thresh&&(invivofactor>4.0)) 
	{

	 *ltail *= 1.5;
	 *rtail *= 1.5; 
   	 if (*ltail>2.5*(*reg_max-*sindex)) *ltail=2.0*(*reg_max-*sindex);
 	 if (*rtail>2.5*(*eindex-*reg_max)) *rtail=2.0*(*eindex-*reg_max);
	}

  else if (*reg_maxv>= 4.0* (float) thresh&&(invivofactor>4.0)) 
	{
	 *ltail *= 2.0;
	 *rtail *= 2.0; 
   	 if (*ltail>3.0*(*reg_max-*sindex)) *ltail=2.5*(*reg_max-*sindex);
 	 if (*rtail>3.0*(*eindex-*reg_max)) *rtail=2.5*(*eindex-*reg_max);
	}

  else if (*reg_maxv>= 2.5* (float) thresh&&(invivofactor>4.0)) 
	{
	 *ltail *= 2.0;
	 *rtail *= 2.0; 
   	 if (*ltail>3.0*(*reg_max-*sindex)) *ltail=3.0*(*reg_max-*sindex);
 	 if (*rtail>3.0*(*eindex-*reg_max)) *rtail=3.0*(*eindex-*reg_max);
	}
  else if (*reg_maxv>= 1.25* (float) thresh&&(invivofactor>4.0)) 
	{
   	 if (*ltail<2.5*(*reg_max-*sindex)) *ltail=2.5*(*reg_max-*sindex);
 	 if (*rtail<2.5*(*eindex-*reg_max)) *rtail=2.5*(*eindex-*reg_max);
  	 if (*ltail>4.0*(*reg_max-*sindex)) *ltail=4.0*(*reg_max-*sindex);
 	 if (*rtail>4.0*(*eindex-*reg_max)) *rtail=4.0*(*eindex-*reg_max);
	}
 else if (invivofactor>4.0)
	{
   	 if (*ltail<3.0*(*reg_max-*sindex)) *ltail=3.0*(*reg_max-*sindex);
 	 if (*rtail<3.0*(*eindex-*reg_max)) *rtail=3.0*(*eindex-*reg_max);
  	 if (*ltail>8.0*(*reg_max-*sindex)) *ltail=5.0*(*reg_max-*sindex);
 	 if (*rtail>8.0*(*eindex-*reg_max)) *rtail=5.0*(*eindex-*reg_max);
	}

  
  if ((*ln<=3&&*ltail!=*rtail)||*reg_maxv< 6.0*thresh) 
	{  
	 tail=*ltail;
	 if(tail<*rtail) tail= *rtail;
	 if (tail>*ltail*1.5&&tail<*ltail*2.0) 
		tail=(tail+*ltail)/2.0;
	 else if (tail>*rtail*1.5&&tail<*rtail*2.0) 
		tail=(tail+*rtail)/2.0;
	 else if (tail>*ltail*2.0&&tail<*ltail*3.0) 
		tail=*ltail*1.8; 
	 else if (tail>*rtail*2.0&&tail<*rtail*3.0) 
		tail=*rtail*1.8;
	 else if (tail>=*ltail*3.0&&tail>=*ltail*5.0) 
		tail= *ltail*2.0;
	 else if (tail>=*rtail*3.0&&tail>=*rtail*5.0) 
		tail= *rtail*2.0;
	 else if (tail>=*ltail*5.0) 
		tail= *ltail*2.3;
	 else if (tail>=*rtail*5.0) 
		tail= *rtail*2.3;

	 *ltail = tail;
	 *rtail = tail;
	}

  }


 return (ok);

}



/*******************************/
static int  group_peaks(int *r_number)
/********************************/
{
 int i, N, d, d1,d2,m,mi,ln, maxp,npts,end;
 float max;

 N=0;
 i=1;
 m=0;
 mi=0;
 max=speakB[0].amax;
 maxp=speakB[0].php;
 ln=speakB[0].linenum;
 end=reg_end[0];
 npts=fn/2;

 while (i<*r_number)
   {
    d=reg_start[i] - reg_end[N];
    if (m<=d) {m=d; mi=i;}

    d2=speakB[i].php-speakB[N].php; 


   if ((speakB[N].rtail>speakB[i].ltail*10&&d2>speakB[i].ltail*16&&d>speakB[i].ltail*5)||(speakB[i].ltail>speakB[N].rtail*10&&d2>speakB[N].rtail*16&&d>speakB[N].rtail*5))
      {
	if (speakB[N].rtail>speakB[i].ltail) 
	   {
		speakB[i].ltail *= 2;
		if (speakB[N].rtail > d - speakB[i].ltail)
		    speakB[N].rtail = d - speakB[i].rtail; 
	   }

	else if (speakB[i].ltail>speakB[N].rtail) 
	   {
		speakB[N].rtail *= 2;
		if (speakB[i].ltail > d - speakB[N].rtail)
		   speakB[i].ltail = d - speakB[N].rtail;
	   }

	N++;
        if (N!=i)
        {
	speakB[N].ltail = speakB[i].ltail; 
	speakB[N].rtail = speakB[i].rtail; 
	speakB[N].linenum = speakB[i].linenum;
	speakB[N].amax= speakB[i].amax; 
 	speakB[N].php= speakB[i].php;
	reg_start[N] = reg_start[i];
	reg_end[N] = reg_end[i];
	}
      }
   else if ((d<=speakB[i].ltail&&d<=speakB[N].rtail) || (d2<npts/50&&(speakB[N].amax<40.0*xdev||speakB[i].amax<40.0*xdev)))
      {

    	d1= reg_start[N] - speakB[N].ltail;
        if (d1>reg_start[i]-speakB[i].ltail)
	   d1=reg_start[i]-speakB[i].ltail;

	speakB[N].ltail=reg_start[N]-d1;

    	d1=speakB[i].rtail + reg_end[i];
        if (speakB[N].rtail + reg_end[N] > d1)
	   d1=speakB[N].rtail + reg_end[N]; 

	speakB[N].rtail = d1 - reg_end[i];

	speakB[N].linenum=speakB[i].linenum + speakB[N].linenum;

        if (speakB[N].amax < speakB[i].amax)
	  { speakB[N].amax = speakB[i].amax; speakB[N].php = speakB[i].php;}

	reg_end[N] = reg_end[i];        
      }
   else if ((d>speakB[i].ltail&&d<=speakB[N].rtail)||(d<=speakB[i].ltail&&d>speakB[N].rtail))
      {
      if ((speakB[N].amax>6.0*speakB[i].amax&&d>speakB[i].ltail&&d<=speakB[N].rtail)
               ||(speakB[i].amax>6.0*speakB[N].amax&&d<=speakB[i].ltail&&d>speakB[N].rtail))  

/* 	if (speakB[N].amax>6.0*speakB[i].amax||speakB[i].amax>6.0*speakB[N].amax)  */
	{
	 if (speakB[N].rtail>2*speakB[i].ltail&&d>speakB[i].ltail)
	     speakB[N].rtail = d-speakB[i].ltail;
	 else if (speakB[i].ltail>2*speakB[N].rtail&&d>speakB[N].rtail)  
	     speakB[i].ltail = d-speakB[N].rtail;	     
	 else  speakB[i].ltail = speakB[N].rtail = d/2;

	 N++;
         if (N!=i)
          {speakB[N].ltail = speakB[i].ltail; 
	   speakB[N].rtail = speakB[i].rtail; 
	   speakB[N].linenum = speakB[i].linenum;
	   speakB[N].amax= speakB[i].amax; 
 	   speakB[N].php= speakB[i].php;
	   reg_start[N] = reg_start[i];
	   reg_end[N] = reg_end[i];}
	}

       else
        {
    	d1= reg_start[N] - speakB[N].ltail;
        if (d1>reg_start[i]-speakB[i].ltail)
	   d1=reg_start[i]-speakB[i].ltail;

	speakB[N].ltail=reg_start[N]-d1;

    	d1=speakB[i].rtail + reg_end[i];
        if (speakB[N].rtail + reg_end[N] > d1)
	   d1=speakB[N].rtail + reg_end[N]; 

	speakB[N].rtail = d1 - reg_end[i];

	speakB[N].linenum=speakB[i].linenum + speakB[N].linenum;

        if (speakB[N].amax < speakB[i].amax)
	  { speakB[N].amax = speakB[i].amax; speakB[N].php = speakB[i].php;}

	reg_end[N] = reg_end[i];        
        }
      }
    else 
      {
	N++;
        if (N!=i)
        {
	speakB[N].ltail = speakB[i].ltail; 
	speakB[N].rtail = speakB[i].rtail; 
	speakB[N].linenum = speakB[i].linenum;
	speakB[N].amax= speakB[i].amax; 
 	speakB[N].php= speakB[i].php;
	reg_start[N] = reg_start[i];
	reg_end[N] = reg_end[i];
	}
      }
    i++;
   }

 if (N==0&&*r_number>1&&m!=0) 
   {
	speakB[N].ltail = speakB[0].ltail; 
	speakB[N].rtail = m/2; 

     	for (i=1;i<mi; i++)
         { ln += speakB[i].linenum;
           if (max<speakB[i].amax) 
		{max=speakB[i].amax; maxp=speakB[i].php;}
 	 }
	speakB[N].linenum = ln;
	speakB[N].amax= max; 
 	speakB[N].php= maxp;

	reg_start[N] = reg_start[0];
	if (mi==1) reg_end[N] = end;
	else reg_end[N] = reg_end[mi-1];

	N++;
	speakB[N].rtail = speakB[*r_number-1].rtail; 
	speakB[N].ltail = m-speakB[N-1].rtail; 

 	max=speakB[mi].amax;
 	maxp=speakB[mi].php;
 	ln=speakB[mi].linenum;

     	for (i=mi+1;i<*r_number; i++)
         { ln += speakB[i].linenum;
           if (max<speakB[i].amax) 
		{max=speakB[i].amax; maxp=speakB[i].php;}
 	 }
	speakB[N].linenum = ln;
	speakB[N].amax= max; 
 	speakB[N].php= maxp;

	reg_start[N] = reg_start[mi];
	reg_end[N] = reg_end[*r_number-1];

   } 

  *r_number = N+1;

  cal_pv(N+1);

 return(COMPLETE);

}


/*******************************/
static int cal_pv(int r_number)
/********************************/
{
 int n, N, i,d;

N=r_number;
if (N!=0)
{
 pv=0.0;
 i=0;
 for (n=0;n<N; n++)
     {
      pv += speakB[n].amax;
      i += reg_end[n]-reg_start[n]+1;

      reg_start[n]=reg_start[n]-speakB[n].ltail;
      if (n==0&&reg_start[n]<0) reg_start[n]=0;
      if (n>0&&reg_start[n]<reg_end[n-1]) 
       {
	d=reg_end[n-1]-reg_start[n];
	reg_start[n]=reg_start[n]+d/2;
	reg_end[n-1]=reg_start[n];
       }
      reg_end[n]=reg_end[n]+speakB[n].rtail;
      if (reg_end[n]>=fn/2) reg_end[n]=fn/2-1;
     }
 pv = (float) pv/N;
 pnav=i/N;

 pdv=0.0;
 for (n=0;n<N; n++)
     pdv += speakB[n].amax - pv;
 pdv = (float) pdv/N;
}
 return (0);
    
}


/******************************************************************/
static int phase_regions(int *regions, int *good_regions, float lpv, int npts)
/******************************************************************/
{
int i,n,php,res,good,rg;
float ph,xmax,xxmm;

/* phase each region, and estimate phase for each region again, us lpv */

xxmm=-amax;
xxmi=0;
*good_regions=0;

lpv=lpv/(float)npts;

rg=*regions;
n=0;

for(i=0;i<*regions;i++)
{ 
 if ((speakB[i].amax<3.0*thresh||speakB[i].amax<8.0*xdev)&&invivofactor>5.0&&rg>3)
  rg -=1;
 else
  {
  ph = speakB[i].phase;
  php = speakB[i].php;
  xmax = speakB[i].amax;
  res=adjust_ph(i,&ph, &php,&xmax, lpv, &good);
  if (res==FALSE&&rg>3) rg -=1;
  else  
     {
	if (!good || (res==FALSE)) speakB[n].state=5;	
	else  {speakB[n].state=0;*good_regions +=1;}
	speakB[n].phase=ph;
	speakB[n].php=php;
        speakB[n].amax=xmax;
  	if (xxmm<xmax) {xxmm=xmax; xxmi=n;}
	if (n!=i)
	{
         speakB[n].linenum=speakB[i].linenum;
         speakB[n].ltail=speakB[i].ltail;
         speakB[n].rtail=speakB[i].rtail;	
         reg_start[n]=reg_start[i];
         reg_end[n]=reg_end[i];
	}

        n++;
     }
  }  

}


*regions=n;

 pv=0.0;
 for(i=0;i<*regions;i++)
   {  
    pv += speakB[i].amax;
   }

 pv=pv/(float)n;

return(COMPLETE);

}


/******************************************************************/
static int adjust_ph(int r, float *phase, int *phasep, float *xmax,
                     float lpv, int *good)
/******************************************************************/
{

int ok, i, cn, stop, j1, j2,php,app,bpp,ss,chgstd,ff,sstt, separate;
float x, y, xn,xm, xp, bps,bns,aps,ans, old_bps, old_aps;
float dph, old_dph, ph, old_ph0, ph0, std, std1, step;

/* Temporary fix to keep program from crashing */
if (reg_end[r] < reg_start[r])
{
   DPRINT4("adjust_ph: reg_start[%d]=%d  reg_end[%d]=%d \n",r,reg_start[r],
						r,reg_end[r]);
   reg_end[r] = reg_start[r];
   return(FALSE);
}

j1 = reg_start[r];
j2=reg_end[r];


if (speakB[r].amax<xdev)  return(FALSE);

 separate = FALSE;
 stop=FALSE;
 php=speakB[r].php;
 ph0=0.0;
 ok=TRUE;
 dph=old_dph=0.0;
 app = bpp = 0;
 old_aps = old_bps = old_ph0 = 0.0;

 cn=0;
 while (!stop)
 { 
 bps=0.0;
 bns=0.0;
 aps=0.0;
 ans=0.0;

 for (i=j1;i<php;i++)
  {
   ph=ph0+lpv * (float) i;
   x = *(phase_data + i*2);
   y = *(phase_data + i*2+1);
   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
   if (xn>xdev) bps+=xn;
   else if (xn<-xdev) bns+=xn;
  }

  i=php; 
  ph=ph0+lpv * (float) i;
  x = *(phase_data + i*2);
  y = *(phase_data + i*2+1);
  xp= x*cos(ph/DEG)+y*sin(ph/DEG);

 for (i=php+1; i<j2;i++)
  {
   ph=ph0+lpv * (float) i;
   x = *(phase_data + i*2);
   y = *(phase_data + i*2+1);
   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
   if (xn>xdev) aps+=xn;
   else if (xn<-xdev) ans+=xn;
  }
  
 if (bps-bns==0) bps=0.0;
 else bps= (bps+bns)/(bps-bns);
 if (aps-ans==0) aps=0.0;
 else aps= (aps+ans)/(aps-ans);

 old_dph=dph;

 if (!rule_ph1(bps,aps,xp,&dph))
   { DPRINT0 ("this region not siutable for rule1 of adjust_ph.\n");
     stop=TRUE;    
   }

 else
   {
   if (fabs(dph)<5.0) {stop=TRUE;}
   if (fabs(old_dph+dph)<1.0) {stop=TRUE; dph=dph/2.0;}

   if (aph_b) dph = -dph;  /* add for phasing Bruker data, Lynn Cai, 4-7-98 */
   
   ph0 += dph;
   cn++;
   if (cn>30) {stop=TRUE;}
   }

 }

 cn=0;
 stop=FALSE;
 *good=TRUE;

  ss=0;
  step=1.0;
  xm=speakB[r].amax;
  std=xm/100.0;
  if (xdev>std) {std=xdev/2; ss=10;}
  if (std>5.0*xdev) 
     {
	 std=xm/300.0; 
	 step=0.7;
	 ss=1;	
	 if (std>5.0*xdev) 
	  { std=xm/500.0; 
	    step=0.5;
	    if (std>5.0*xdev) 
		{std=xm/800.0; 
		 step=0.4;
		 if (std>6.0*xdev) { step = 0.3; std=6.0*xdev;  }
		}
	  }  
	 if (std<pv/900.0) std=pv/900.0;
/*
	 if (std<5.0*xdev) std = 5.0*xdev;
*/
      }

 chgstd=0;
 ff=0;
 sstt=100;

 while (!stop)
 { 

 if(ff==0||dph!=0.0)
  { 
  ff=1; 
  xm=-xdev;
  for (i=j1;i<j2;i++)
   {
   ph=ph0+lpv * (float) i;
   x = *(phase_data + i*2);
   y = *(phase_data + i*2+1);
   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
   if (xn>xm) {xm=xn; php=i;}   
   }

 bps=xm;
 aps=xm;
 for (i=j1;i<php;i++)
  {
   ph=ph0+lpv * (float) i;
   x = *(phase_data + i*2);
   y = *(phase_data + i*2+1);
   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
   if (bps>=xn) {bps=xn;bpp=i;}
  }
 
 for (i=php+1; i<j2;i++)
  {
   ph=ph0+lpv * (double) i;
   x = *(phase_data + i*2);
   y = *(phase_data + i*2+1);
   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
   if (aps>xn) {aps=xn; app=i;}
  }

 }
  old_dph=dph;


 rule_ph2(bps,aps, &dph,std);

/* 1.0 is changed to step, change back if not good, only after ss==1 step was 0.5 */

  if (fabs(dph)<step)
    {
     if ((aps<-2.0*std||bps<-2.0*std)&&(!invivo||invivofactor>5.0)) rule_ph3(bps,bpp, aps, app,&dph,std,ss,php);

     if ((ss==0&&fabs(dph)<1.0)||(ss==1&&fabs(dph)<step) )
	{
	 if (bps<-3.0*xdev||aps<-3.0*xdev||fabs(bps-aps)>3.0*xdev)
		{ if (chgstd>-2) {std /=2.0; dph *=2.0; chgstd +=-1;}
		  else {dph *=2.0;stop=TRUE;}
		}
	 else stop=TRUE;
 	}   
     else if (ss==10&&fabs(dph)<step)
	{
	  stop=TRUE;
 	}
     }

   if (fabs(old_dph+dph)<step)
     {
      if (fabs(dph)>8.0) {std *=2.0;chgstd =1; }
      else if (fabs(dph)>3.0) {std *=1.5;chgstd =1;}
      else  stop=TRUE;
      dph=dph/2.0;
     }

   else if (dph/old_dph<-0.65&&old_dph/dph<-0.65) {  dph=dph/2.0;}
	 
    if (sstt==100&&chgstd!=0) sstt=chgstd;
    else if (sstt!=100&&sstt*chgstd<0) sstt=0;

    if (sstt==0&&(fabs(dph)<step||fabs(old_dph+dph)<step)) {dph /=2.0; stop=TRUE;}

    if (aph_b) dph = -dph;  /* add for phasing Bruker data, Lynn Cai, 4-7-98 */

    ph0 += dph;
    cn++;

    if (cn>30) {stop=TRUE; ok=FALSE;}
 

 
  if(stop)
     {
	 bps=xm;
	 aps=xm;
	 for (i=j1;i<php;i++)
	  {
	   ph=ph0+lpv * (float) i;
	   x = *(phase_data + i*2);
	   y = *(phase_data + i*2+1);
	   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
	   if (bps>=xn) {bps=xn;bpp=i;}
	  } 
	 for (i=php+1; i<j2;i++)
	  {
	   ph=ph0+lpv * (double) i;
	   x = *(phase_data + i*2);
	   y = *(phase_data + i*2+1);
	   xn= x*cos(ph/DEG)+y*sin(ph/DEG);
	   if (aps>xn) {aps=xn; app=i;}
	  }


	 if (!separate)
	  {

	    if (ss==10) 
		{ if (noisefactor<4.0) noisefactor = 4.0; 
		  std1 =xdev*noisefactor; 
		}
	    else if (ss==1) 
		{ 
		 if ( (int) 10*step == 7) {std1 = xm / 150.0;  if (std1 > 4.0*xdev) std1 = 4.0*xdev;}
		 else if ((int) 10*step == 5) {std1 = xm / 200.0; if (std1 > 4.0*xdev) std1 = 4.0*xdev;}
		 else if ((int) 10*step == 4) std1 = xm / 300.0;
		 else std1 = 8.0*xdev; 
/*
		 if (std1 < 8.0*xdev) std1 = 8.0*xdev;
*/
		
		}
	    else
		{ std1 = xm*0.01; 
 		  if (std1 < 2.0*xdev) std1 = 2.0*xdev;
 		  if (std1 > 4.0*xdev) std1 = 4.0*xdev;
		}
	
	    if ((bps<-2.0*std1||aps<-2.0*std1)&&
		(( (float)(app-php)>(float)(php-bpp)*1.3&&(app-php)>65 ) ||
		 ( (float)(php-bpp)>(float)(app-php)*1.3&&(php-bpp)>65 )))
	      {
	   	stop = FALSE;
	   	cn =0;
	   	separate = TRUE;
		old_ph0 = ph0;
		old_bps = bps;
		old_aps = aps;
	   	if (php-bpp>app-php) j1 = (php+bpp)/2; 
	   	if (app-php>php-bpp) j2 = (php+app)/2; 
	      }
 	  }

	 if (stop)
 	  {
	    if (ss==10) 
		{ if (noisefactor<4.0) noisefactor = 4.0; 
		  std =xdev*noisefactor; 
		}
	    else if (ss==1) 
		{ 
		 if (std < xm*0.01) std = xm*0.01; 
		}
	    else  
		{ 
		  if (std < xm*0.02) std = xm*0.02; 
		}

 	    if (std < 2.5*xdev) std = 2.5*xdev;

	    if ((bps<-2.0*std||aps<-2.0*std)&&(!invivo||invivofactor>5.0)&&
				((float) (app-php)/(float)(php-bpp)>1.05||(float)(php-bpp)/(float)(app-php)>1.05))
	         *good=FALSE;
            if (fabs(bps-aps)>2.0*std) *good=FALSE;

	    if (separate && *good == TRUE)
	     {
	     reg_start[r] = j1;
	     reg_end[r] = j2;
	     }
	    else if (separate && *good == FALSE)
	     {
	        ph0 = old_ph0;
	  	bps = old_bps;
	  	aps = old_aps;
		*good = TRUE;
		if ((bps<-2.0*std||aps<-2.0*std)&&(!invivo||invivofactor>5.0)&&
				((float) (app-php)/(float)(php-bpp)>1.05||(float)(php-bpp)/(float)(app-php)>1.05))
	         	*good=FALSE;
            	if (fabs(bps-aps)>2.0*std) *good=FALSE;
	     }

	 }

     }

 }


 *phase=ph0;  *phasep=php; *xmax=xm;  

 std=0.2*xm;
 if (std <xdev*noisefactor) std =xdev*noisefactor;

 if (xm<0.5*amax&&(bps<-std||aps<-std||fabs(bps-aps)>2.0*std)) return (FALSE); 
 
 return (TRUE);
}

/******************************************************************/
static int rule_ph1(float bps, float aps, float xp, float *dph)
/******************************************************************/

{
float PA, NA, ZA, PB, NB, ZB, xx, w1, yy;


if (fabs(aps)<EPSILON&&fabs(bps)<EPSILON)
  { if (xp>xdev) {*dph=0.0; return(TRUE);}
    if (xp<-xdev) {*dph=180.0;return(TRUE);}
    else return (FALSE);
  }
else
 {
   if (aps>=0) {PA=aps;ZA=1.0-aps;NA=0.0;}
   else {PA=0.0;ZA=1+aps;NA=-aps;}

   if (bps>=0) {PB=bps;ZB=1.0-bps;NB=0.0;}
   else {PB=0.0;ZB=1+bps;NB=-bps;}

   xx=0.0;
   yy=0.0;

   w1=PA;
   if (w1<ZB)  w1=ZB;
   xx +=60.0*w1;
   yy +=w1;

   w1=PA;
   if (w1<NB)  w1=NB;
   xx +=105.0*w1;
   yy +=w1;

   w1=ZA;
   if (w1<NB)  w1=NB;
   xx +=150.0*w1;
   yy +=w1;

   w1=ZA;
   if (w1<PB)  w1=PB;
   xx +=-60.0*w1;
   yy +=w1;

   w1=NA;
   if (w1<PB)  w1=PB;
   xx +=-105.0*w1;
   yy +=w1;

   w1=NA;
   if (w1<ZB)  w1=ZB;
   xx +=-150.0*w1;
   yy +=w1;

   w1=NA;
   if (w1<NB)  w1=NB;
   if (fabs(bps)>fabs(aps)) xx += 180.0*w1;
   else xx += -180.0*w1;
   yy +=w1;

   *dph = xx/yy;

   return (TRUE);
 }

}



/******************************************************************/
static int rule_ph2(float bps, float aps, float *dph, float std)
/******************************************************************/
{
float dp, step,w1,xx;
int di;



 step=1.0;

   if (bps < aps) di = 1;
   else di=-1;
   dp=fabs(bps-aps);
   xx=0.0;

 if (dp<=std/2.0) xx=0.0;
 else if (dp<std) 
     {
	w1=(2.0*dp-std)/std;	
  	xx +=1.5*step*w1;
     }

 else if(dp<2.0*std) 
     {
	w1=(dp-std)/std;
  	xx += 3.0*step*w1; 
	w1=(2.0*std-dp)/std;
  	xx += 1.5*step*w1; 

      }

 else if(dp<4.0*std) 
     {
	w1=(dp-2.0*std)/(2.0*std);
  	xx += 6.0*step*w1; 
	w1=(4.0*std-dp)/(2.0*std);
  	xx += 3.0*step*w1; 
      }
 else if(dp<10.0*std) 
     {
	w1=(dp-4.0*std)/(6.0*std);
  	xx += 20.0*step*w1; 
	w1=(10.0*std-dp)/(6.0*std);
  	xx += 6.0*step*w1; 
      }
 else if(dp>=10.0*std) 
  	xx += 20.0*step; 

   *dph= di*xx; 

   return (TRUE);
 }

/******************************************************************/
static int rule_ph3(float bps, int bpp, float aps, int app, float *dph,
                    float std, int ss, int pp)
/******************************************************************/
{
float dp, step,w1,xx;
int di;


 step=1.0;

 di=1; 
 xx=0.0;
 
   if (pp-bpp>app-pp) di = -1;
    else if (pp-bpp==app-pp&&aps<bps) di = -1;
    dp=fabs(aps);
    if (dp<fabs(bps)) dp=fabs(bps);
    if (ss == 1) step=0.5;

    if(dp<4.0*std) 
     {
	w1=(dp-2.0*std)/(2.0*std);
  	xx += 2.0*step*w1; 
      }
    else if(dp<8.0*std) 
     {
	w1=(dp-4.0*std)/(4.0*std);
  	xx += 4.0*step*w1; 
	w1=(8.0*std-dp)/(4.0*std);
  	xx += 2.0*step*w1; 
      }
    else if(dp>=8.0*std) 
  	xx += 4.0*step; 


   *dph= di*xx; 

   return (TRUE);

 }



/******************************************************************/
static int first_rp(int regions, int *good_regions, int *REF)
/******************************************************************/
{
 int good;

 	 good=*good_regions;
	 *REF=find_ref(regions,&good);
	 speakB[*REF].state=1;   
 	 *good_regions=good;
         return (TRUE);
}  


/******************************************************************/
static int find_ref(int regions, int *good)
/******************************************************************/
{
int i,n,k, ok;
int min_ln; 

 n=0;
 ok=FALSE;
 if (*good==0)
   {    n=0;
	for (i=1; i<regions; i++)
	 {	  
	  if(speakB[i].state==5&&speakB[i].amax>speakB[n].amax) {n=i;  ok=TRUE;}
          speakB[i].state=0; *good +=1;
 	 }
    }

 else if (*good==1)
   { n=0;
     if(speakB[n].state>=4||speakB[n].state==1)  
        {
	 for (i=1; i<regions; i++)
	  {	  
	   if (speakB[i].state<4&&speakB[i].state!=1)  {n=i; ok=TRUE;}
 	  }
	}
      if (speakB[n].amax<0.4*pv) 
 	{ok=FALSE; n=0;
	 for (i=1; i<regions; i++)
	 {	  
	  if(speakB[i].state==5&&speakB[i].amax>speakB[n].amax) {n=i;  ok=TRUE;}
          speakB[i].state=0; *good +=1;
 	 }

	}
    }


/*
 else if (*good==2)
   { i=0;
         while (!ok&&i<regions)
	{
	if (speakB[i].state<4&&speakB[i].state!=1) {n=i; ok=TRUE;}
	i++;
	}           
   }
*/

 else
  { 
   if (amax>=1.3*pv) 
      {	
	i=xxmi;
	if (speakB[i].state<4&&speakB[i].state!=1) {n=i; ok=TRUE;}	
      } 	
   min_ln=32768;
   if (!ok && *good>4)
     { 
	for (k=0;k<regions;k++)
	  {     if(min_ln>speakB[k].linenum) min_ln=speakB[k].linenum;
		if (speakB[k].state<4&&speakB[k].state!=1&&speakB[k].amax>=0.5*speakB[xxmi].amax) 
		  { n=k; ok=TRUE; }		
	  }
     
     }

   if (!ok)
     { 
	for (k=0;k<regions;k++)
	  { 
		if (min_ln>speakB[k].linenum&&speakB[k].state<4&&speakB[k].state!=1&&speakB[k].amax>=0.5*pv) 
		  {min_ln=speakB[k].linenum; n=k; ok=TRUE;}
	    else if (min_ln==speakB[k].linenum&&speakB[k].state<4&&speakB[k].state!=1&&speakB[k].amax>=0.5*pv)
		{if (!ok) {min_ln=speakB[k].linenum; n=k; ok=TRUE;}
		 else { if (speakB[k].amax>1.5*speakB[n].amax) n=k; }
		}
	  }
     
     }

   i=0;
   while (!ok&&i<regions)
     {              	
	 if (speakB[i].state<4&&speakB[i].state!=1&&speakB[i].amax>0.5*pv) {n=i; ok=TRUE;}
     	 i++;
     }

   i=0;
   while (!ok&&i<regions)
     {              	
	 if (speakB[i].state<4&&speakB[i].state!=1) {n=i; ok=TRUE;}
     	 i++;
     }

   i=0;
   while (!ok&&i<regions)
     {              	
	 if (speakB[i].state!=4&&speakB[i].state!=1&&speakB[i].state<6) {n=i; ok=TRUE;}
     	 i++;
     }


   }

 DPRINT1("region %d is the reference.\n ", n);

 return (n);
}

 
/******************************************************************/
static int  second_lp(int regions, int good_regions, float *new_lp,
                      int *REF, int npts)
/******************************************************************/
{
int i, ok, ok1, ok2, ok3,invert;
int r0,r1,r2, php, php1;
float tt, tt1, tlp, trp, ph1;

 r0=*REF; 
 ok1=FALSE;
 ok2=FALSE;
 ok3=TRUE;

 ok=FALSE;

 invert=0;

 good_regions=regions;
 r1=r2=r0;
 while(!ok&&good_regions>=2)
   {

    if ( (speakB[r0].state==4 && good_regions>=1 ) || ok3==FALSE) 
       {
	if (speakB[r1].state!=5&&speakB[r1].amax>0.4*pv) 
	 {
	 	r0=r1; 
	 	speakB[r0].state=1;
		ok3=TRUE;	
	 	if (speakB[r2].state!=5) {r1=r2; ok2=FALSE;}
	 	else ok1=FALSE;
	 }
	else if (speakB[r2].state!=5&&speakB[r2].amax>0.4*pv) 
	 {
		r0=r2; 
		ok3=TRUE;
		speakB[r0].state=1;	
	 	ok2=FALSE;
	 }
        else 
	  { 
		r0=find_ref(regions,&good_regions); 
                if (r0>=regions) ok3=FALSE;
		else 	ok3=TRUE;
		if (speakB[r0].state!=5) speakB[r2].state=1;
		else speakB[r2].state=50;
	   }
	}

    if (!ok1&&good_regions>=2)
 	   {
    	   r1=find_ref1(regions,good_regions,r0);
 	   if (r1<regions&&r1!=r2) ok1=TRUE;  
 	   if (speakB[r1].state!=5)	      
	  	speakB[r1].state=1; 	 
           else speakB[r1].state=50; 	      
 	   }

    if (!ok2&&good_regions>=3)
 	   {
    	   r2=find_ref1(regions,good_regions,r0);
 	   if (r2<regions&&r2!=r1) ok2=TRUE;  
	   if (speakB[r2].state!=5)
		speakB[r2].state=1; 	  
           else speakB[r2].state=50; 	      
 	   }

    if (!ok2&&good_regions<=2&&regions>4&&invert==0)	
         {
	      invert=1;
	      good_regions=0;
   	      for (i=0;i<regions;i++)
     	       {        
		if (speakB[i].state==1) speakB[i].state=4;
		else {  good_regions++;
		        if (speakB[i].state==4) speakB[i].state=0; 
			if (speakB[i].state==6) speakB[i].state=5;
		     } 
     	       }
	      *REF=find_ref(regions,&good_regions);
	      r0=*REF;
	      speakB[r0].state=1;
	      if (good_regions>=2)
 	       {
    	        r1=find_ref1(regions,good_regions,r0);
 	   	if (r1<regions) ok1=TRUE;  
	   	if (speakB[r1].state!=5)
	   	   speakB[r1].state=1; 	  
                else speakB[r1].state=50; 	      
 	       }
	     if (!ok2&&good_regions>=3)
 	       {
    	        r2=find_ref1(regions,good_regions,r0);
 	   	if (r2<regions&&r2!=r1) ok2=TRUE;  
	   	if (speakB[r2].state!=5)
	   	   speakB[r2].state=1; 	  
                else speakB[r2].state=50; 	      
 	       }

	 }

     if (ok1&&!ok2&&invert==0)
	  {
  		trp=speakB[*REF].phase;
 		php=speakB[*REF].php;

		if (r1==*REF) r1=r0;
		if (r1==*REF) r1=r2;

  		ph1=speakB[r1].phase;
 		php1=speakB[r1].php;

		tt=(ph1-trp);
		while (tt>180.0) tt-=360.0;
		while (tt<-180.0) tt+=360.0;
  		if (php1-php!=0) tlp = (float) tt/(php1-php);
		else tlp=0.0;
		*new_lp=(float)tlp*npts;
		ok=TRUE;
	   }
     else if (ok1&&ok2)
	   {    tt=speakB[r0].phase-speakB[r2].phase;
		while (tt>180.0) tt-=360.0;
		while (tt<-180.0) tt+=360.0;
      		tt1=speakB[r0].phase-speakB[r1].phase;
		while (tt1>180.0) tt1-=360.0;
		while (tt1<-180.0) tt1+=360.0;
                if (180.0-fabs(tt)<30.0&&180.0-fabs(tt1)<30.0&&good_regions>3)
 		 { good_regions --;
		   speakB[r0].state=4;
		   ok3=FALSE;
		 }
		if (ok1&&ok2&&ok3)
		 {
		  if(calc_lp(&r0,&r1,&r2,npts,&ok2,&good_regions,&tlp)) 
			{ *new_lp=(float) tlp*npts; ok=TRUE;} 
		 }
	   }

     else  {*new_lp=0.0;return(COMPLETE);}
         
   } /*end of while (!ok&&*good_region>=2) */

 
 if (ok&&fabs(*new_lp)<=3600.0)
  {
   if (*REF!=r0&&speakB[*REF].state==4) *REF=r0;
   if (invivo&&fabs(*new_lp)>3600.0) *new_lp=0.0;
  }
 else 
  {
   *new_lp=0.0;
  }
  
 return (COMPLETE);

}   

 
/******************************************************************/
static int find_ref1(int regions, int good_regions, int REF)
/******************************************************************/
{

/* rule: chose lines with phase diff around 90 and not too close if possible */

int i,n,ok,npts;
float dph;

npts=fn/2;
ok=FALSE;
n=0;

 if (good_regions==1)
   { n=0;
     if (speakB[n].state==1) n=1;
     for (i=n+1; i<regions; i++)
	 {	  
	  if(speakB[i].state==5&&speakB[i].amax>speakB[n].amax) {n=i; ok=TRUE;}
 	 }
   }

 else if (good_regions==2)
   { i=0;
     ok=FALSE;
     while (!ok&&i<regions)
	{
	if (speakB[i].state<4&&speakB[i].state!=1) {n=i; ok=TRUE;}
	i++;
	}           
   }
 else
  { 
    	  i=0; ok=FALSE;
	  while (REF+i<regions||REF-i>=0)
	    { i++;
	      if (REF+i<regions)
		 { dph=speakB[REF+i].phase-speakB[REF].phase;
		   while (dph>180.0) dph-=360.0;
		   while (dph<-180.0) dph+=360.0;
		   if (fabs(dph)<=150.0&&fabs(dph)>=30.0&&speakB[REF+i].state<4&&speakB[REF+i].state!=1
			&&speakB[REF+i].amax>0.5*pv&&abs(speakB[REF+i].php-speakB[REF].php)>npts/20) 
			{n=REF+i; ok=TRUE;}
		 }
	       if (REF-i>=0)
		{  dph=speakB[REF-i].phase-speakB[REF].phase;
		   while (dph>180.0) dph-=360.0;
		   while (dph<-180.0) dph+=360.0;
		   if (fabs(dph)<=150.0&&fabs(dph)>=30.0&&speakB[REF-i].state<4&&speakB[REF-i].state!=1
			 &&speakB[REF-i].amax>0.5*pv&&abs(speakB[REF-i].php-speakB[REF].php)>npts/20) 
			{n=REF-i; ok=TRUE;}
		}
	     }

     	  i=0; 
	  while (REF+i<regions||REF-i>=0)
	    { i++;
	      if (REF+i<regions)
		 { dph=speakB[REF+i].phase-speakB[REF].phase;
		   while (dph>180.0) dph-=360.0;
		   while (dph<-180.0) dph+=360.0;
		   if (fabs(dph)<=150.0&&fabs(dph)>=30.0&&speakB[REF+i].state<4
			&&speakB[REF+i].state!=1&&speakB[REF+i].amax>0.3*pv) 
			{n=REF+i; ok=TRUE;}
		 }
	       if (REF-i>=0)
		{  dph=speakB[REF-i].phase-speakB[REF].phase;
		   while (dph>180.0) dph-=360.0;
		   while (dph<-180.0) dph+=360.0;
		   if (fabs(dph)<=150.0&&fabs(dph)>=30.0&&speakB[REF-i].state<4
			&&speakB[REF-i].state!=1&&speakB[REF-i].amax>0.3*pv) 
			{n=REF-i; ok=TRUE;}
		}
	     }
	  if(!ok) 
 	    {if ((REF+i)==regions)
		{if (speakB[REF+i-1].state<4&&speakB[REF+i-1].state!=1) {n=regions-1;ok=TRUE;}
		 else{ i=0;
		       while (!ok&&i<=regions/2)
			{if(speakB[regions-2-i].state<4&&speakB[regions-2-i].state!=1
			    &&speakB[regions-2-i].amax>0.3*pv) {n=regions-2-i; ok=TRUE;}
			 if (!ok&&speakB[i].state<4&&speakB[i].state!=1&&speakB[i].amax>0.3*pv) 
			    {n=i; ok=TRUE;}
			 if(!ok) i++;
			}
 		      }
		}

	     else   { i=0;
		       while (!ok&&i<=regions/2)
			{if(speakB[i].state<4&&speakB[i].state!=1&&speakB[i].amax>0.3*pv) 
			  {n=i; ok=TRUE;}
			 if(speakB[regions-1-i].state<4&&speakB[regions-1-i].state!=1
				&&speakB[regions-1-i].amax>0.3*pv) {n=regions-1-i; ok=TRUE;}
			 if(!ok) i++;
			}
 		     }
		
	    }
   }	  

if(!ok)   
	{ i=0;
	  while (!ok&&i<regions)
	    {if(i!=REF&&speakB[i].state!=1&&speakB[i].state<4) {n=i; ok=TRUE;}
	     else i++;
	    }
        }


 if(!ok)   
	{ i=0;
	  while (!ok&&i<regions)
	    {if(i!=REF&&speakB[i].state!=1&&speakB[i].state!=4&&speakB[i].state<6) 
		{n=i; ok=TRUE;}
	     else i++;
	    }
        }


 return (n);

}

 

/********************************************/
static int calc_lp(int *r0, int *r1, int *r2, int npts,
                   int *ok2, int *good_regions, float *lpv)
/*******************************************/
{
float ph3, tlp, ph1,ph2, tp, tt, tt1;
int php3, php1,php2, ok, m1, good;

ok=FALSE;
good=*good_regions;

	if (fabs(speakB[*r0].php-speakB[*r1].php)<fabs(speakB[*r0].php-speakB[*r2].php))
	{m1=*r1; *r1=*r2; *r2=m1;
	}

	ph1=speakB[*r2].phase;
	ph2=speakB[*r0].phase;
	ph3=speakB[*r1].phase;
	php1=speakB[*r2].php;
	php2=speakB[*r0].php;
	php3=speakB[*r1].php;

if (speakB[*r0].state!=50&&speakB[*r1].state!=50&&speakB[*r2].state!=50)
 {
    tt1=(ph2-ph3);
    while (tt1>180.0) tt1 -= 360;
    while (tt1<-180.0) tt1 += 360;

    tlp = (float) tt1/(php2-php3);
    if (fabs(npts*tlp)<3600.0)
	 {
    	  tp=(float) ph2+tlp*(php1-php2);
	  tt=tp-ph1;
	  while(tt>360.0) tt-=360.0;
	  while(tt<-360.0) tt+=360.0;

	  if (fabs(tt)<=30.0||360.0-fabs(tt)<=30.0) {ok=TRUE;}
	  else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
		{speakB[*r2].state=4; good--; *ok2=FALSE;*good_regions=good;return(FALSE);}
	 }
    if (!ok)  
	  {
	   tlp = (float) (360.0+tt1)/(php2-php3);
    	   if (fabs(npts*tlp)<3600.0)
	    	{
  	 	      	tp=(float) ph2+tlp*(php1-php2);
		  	tt=tp-ph1;
	  		while(tt>360.0) tt-=360.0;
	  		while(tt<-360.0) tt+=360.0;
	   	        if (fabs(tt)<=30.0||360.0-fabs(tt)<=30.0) {ok=TRUE;}
	    	        else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r2].state=4;good--; *ok2=FALSE;*good_regions=good;return(FALSE);}
	     	}
	   }

    if (!ok)  {
		tlp = (float) (-360.0+tt1)/(php2-php3);
     	 	if (fabs(npts*tlp)<3600.0)
	     	  {
        		tp=(float) ph2+tlp*(php1-php2);
	  		tt=tp-ph1;
	  		while(tt>360.0) tt-=360.0;
	  		while(tt<-360.0) tt+=360.0;
	  		if (fabs(tt)<=30.0||360.0-fabs(tt)<=30.0) {ok=TRUE;}
	  		else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			   {speakB[*r2].state=4; good--; *ok2=FALSE;*good_regions=good;return(FALSE);}
		   }
		}	  

    if (!ok)
        {   m1=*r1; *r1=*r2; *r2=m1;
	    tt1=(ph2-ph1);
	    while (tt1>180.0) tt1 -= 360;
	    while (tt1<-180.0) tt1 += 360;

   	    tlp = (float) tt1/(php2-php1);
    	    if (fabs(npts*tlp)<3600.0)
	    	{

         	 tp=(float) ph2+tlp*(php3-php2);
	 	 tt=tp-ph3;
	 	 while(tt>360.0) tt-=360.0;
	 	 while(tt<-360.0) tt+=360.0;

	 	 if (fabs(tt)<=30.0||360.0-fabs(tt)<=30.0) {ok=TRUE;}
	 	 else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r2].state=4;good--; *ok2=FALSE;*good_regions=good;return(FALSE);}
		}
	}

       if (!ok)  
	{
	 tlp = (float) (360.0+tt1)/(php2-php1);
   	 if (fabs(npts*tlp)<3600.0)
	    {
         	tp=(float) ph2+tlp*(php3-php2);
	  	tt=tp-ph3;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	        if (fabs(tt)<=30.0||360.0-fabs(tt)<=30.0) {ok=TRUE;}
	        else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r2].state=4;good--; *ok2=FALSE;*good_regions=good;return(FALSE);}
	    }
	} 

       if (!ok)  
	{
	  tlp = (float) (-360.0+tt1)/(php2-php1);
	  if(fabs(npts*tlp)<3600.0)
	    {
    		tp=(float) ph2+tlp*(php3-php2);
	  	tt=tp-ph3;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	  	if (fabs(tt)<=30.0||360.0-fabs(tt)<=30.0) {ok=TRUE;}
	  	else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r2].state=4; good--; *ok2=FALSE;*good_regions=good;return(FALSE); }
	    }
	}

      if (!ok) {
		if (speakB[*r0].amax>2.0*speakB[*r1].amax||speakB[*r0].amax>2.0*speakB[*r2].amax)
		{
		 if (speakB[*r1].amax<speakB[*r2].amax) {m1=*r1; *r1=*r2; *r2=m1;}
		 speakB[*r2].state=6; ok2=FALSE;
 		}
		else speakB[*r0].state=6;

		good--; 
		*good_regions=good;
		return(FALSE);
	       }

 }
else
 {

 if (speakB[*r2].state==50) 
    {
    	 tt1=(ph2-ph3);
	 while (tt1>180.0) tt1 -= 360;
	 while (tt1<-180.0) tt1 += 360;

	 tlp = (float) tt1/(php2-php3);
	 if(fabs(npts*tlp)<3600.0)
	   {
            tp=(float) ph2+tlp*(php1-php2);
	    tt=tp-ph1;
	    while(tt>360.0) tt-=360.0;
	    while(tt<-360.0) tt+=360.0;

	    if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;}
	    else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
		{speakB[*r2].state=6; good--; *ok2=FALSE;*good_regions=good; return(FALSE); }
	   }

       if (!ok)  
	{	
	  tlp = (float) (360.0+tt1)/(php2-php3);
	  if(fabs(npts*tlp)<3600.0)
	    {
         	tp=(float) ph2+tlp*(php1-php2);
	  	tt=tp-ph1;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	        if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;}
	        else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r2].state=6; good--; *ok2=FALSE;*good_regions=good; return(FALSE);}
	    }
	}

       if (!ok)  
	{	
	 tlp = (float) (-360.0+tt1)/(php2-php3);
	 if(fabs(npts*tlp)<3600.0)
	   {
         	tp=(float) ph2+tlp*(php1-php2);
	  	tt=tp-ph1;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	  	if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;}
	  	else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) {
			speakB[*r2].state=6; good--; *ok2=FALSE;*good_regions=good;return(FALSE);}
	    }
 	 }	

     if (!ok)  
     {
      speakB[*r2].state=6; good--; *good_regions=good; *ok2=FALSE; return(FALSE);
     }

    }

 else if (speakB[*r1].state==50) 
    { 
   	  tt1=(ph2-ph1);
	  while (tt1>180.0) tt1 -= 360;
	  while (tt1<-180.0) tt1 += 360;

	  tlp = (float) tt1/(php2-php1);
	  if(fabs(npts*tlp)<3600.0)
	   {
            tp=(float) ph2+tlp*(php3-php2);
	    tt=tp-ph3;
	    while(tt>360.0) tt-=360.0;
	    while(tt<-360.0) tt+=360.0;
	    if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE; m1=*r1;*r1=*r2; *r2=m1;}
	    else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
		{speakB[*r1].state=6; *r1=*r2; good--; *good_regions=good; *ok2=FALSE; return(FALSE);}
	   }

       if (!ok)  
	{	
	  tlp = (float) (360.0+tt1)/(php2-php1);
	  if(fabs(npts*tlp)<3600.0)
	    {
         	tp=(float) ph2+tlp*(php3-php2);
	  	tt=tp-ph3;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	  	if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;m1=*r1;*r1=*r2; *r2=m1;}
	  	else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r1].state=6; *r1=*r2; good--; *good_regions=good; *ok2=FALSE; return(FALSE);}
	    }
	}

       if (!ok)  
	{	
	 tlp = (float) (-360.0+tt1)/(php2-php1);
	 if(fabs(npts*tlp)<3600.0)
	   {
         	tp=(float) ph2+tlp*(php3-php2);
	  	tt=tp-ph3;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	  	if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;m1=*r1;*r1=*r2; *r2=m1;}
	  	else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r1].state=6; *r1=*r2; good--; *good_regions=good; *ok2=FALSE; return(FALSE);}
	    }
	}	

     if (!ok)  
      {
       speakB[*r1].state=6; *r1=*r1; good--; *good_regions=good; *ok2=FALSE; return(FALSE);
      }
  
    }
 
 else  
    {
    	tt1=(ph3-ph1);
	while (tt1>180.0) tt1 -= 360;
	while (tt1<-180.0) tt1 += 360;

       tlp = (float) tt1/(php3-php1);
       if(fabs(npts*tlp)<3600.0)
        {
          tp=(float) ph1+tlp*(php2-php1);
	  tt=tp-ph2;
	  while(tt>360.0) tt-=360.0;
	  while(tt<-360.0) tt+=360.0;
	  if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;m1=*r1;*r1=*r1;*r2=*r0;*r0=m1;}
	  else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
		{speakB[*r0].state=6; good--; *good_regions=good; return(FALSE);}
	}

      if (!ok)  
	{	
 	  tlp = (float) (360.0+tt1)/(php3-php1);
 	  if(fabs(npts*tlp)<3600.0)
	    {
        	tp=(float) ph1+tlp*(php2-php1);
	  	tt=tp-ph2;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	  	if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;m1=*r1;*r1=*r1;*r2=*r0;*r0=m1;}
	  	else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
 		  {speakB[*r0].state=6; good--; *good_regions=good; return(FALSE); }
	    }
	}

      if (!ok)  
	{	
	  tlp = (float) (-360.0+tt1)/(php3-php1);
	  if(fabs(npts*tlp)<3600.0)
	     {
         	tp=(float) ph1+tlp*(php2-php1);
	  	tt=tp-ph2;
	  	while(tt>360.0) tt-=360.0;
	  	while(tt<-360.0) tt+=360.0;
	  	if (fabs(tt)<=50.0||360.0-fabs(tt)<=50.0) {ok=TRUE;m1=*r1;*r1=*r1;*r2=*r0;*r0=m1;}
	  	else if (fabs(tt-180.0)<=30.0||fabs(tt+180.0)<=30.0) 
			{speakB[*r0].state=6; good--; *good_regions=good; return(FALSE);}
	     }
	}	  

    if (!ok)  
     {
      speakB[*r0].state=4; good--; *good_regions=good; return(FALSE);
     }

    }

 }



  if (ok) {*lpv= tlp; return(TRUE);}

  else return(FALSE);

}
 
// auto phase each block individually
int do_block_aph(int argc, char *argv[], int first, int last, int order, 
	FILE *bphFile, int bfirst, int blast) {

  double new_rp, new_lp;
  int r;
  int done;
  int block;
  double save_lp, save_rp;
  int file_id = D_PHASFILE;
  int aph0Save=aph0;
  
  if ((d2flag) && (!revflag)) return ERROR;

  P_getreal(CURRENT,"lp",&save_lp,1);
  P_getreal(CURRENT,"rp",&save_rp,1);

  // allocate bphdata
  if(bfirst == 1 && blast == nblocks) { // realloacate 
    if(bphdata != NULL) releaseAllWithId("bph");
    bphdata = NULL;
  }
  if(bphdata == NULL) {
    if ((bphdata = (float *) allocateWithId(sizeof(float) * 2*nblocks,"bph"))==0)
    {
      Werrprintf("cannot allocate block phase buffer");
      return(ERROR);
    }
  }

  block=bfirst-1;
  specIndex = 1;
  while(block < blast) {
  
       aph0=aph0Save;
       if(block != c_buffer) {
        if (c_buffer >= 0)      /* release last used block */
        {
           if ( (r = D_release(file_id, c_buffer)) )
           {
              D_error(r);
              D_close(file_id);
              return(ERROR);
           }
           if ( (r = D_release(D_DATAFILE, c_buffer)) )
           {
              D_error(r);
              D_close(D_DATAFILE);
              return(ERROR);
           }
        }

        if ( (r = D_getbuf(file_id, nblocks, block, &c_block)) )
        {
           if ( (r = D_allocbuf(file_id, block, &c_block)) )
           {
              D_error(r);
              return(ERROR);
           }

           c_block.head->status = 0;
           c_block.head->mode = 0;
        }

        c_buffer = block;
        c_first = block * specperblock; 
        c_last = c_first + specperblock - 1;
       }

       specIndex = block * specperblock + 1;

       // init lp, rp with saved value
       lp = save_lp;
       rp = save_rp;
       c_block.head->lpval=lp;
       c_block.head->rpval=rp;
       bphdata[2*block] = (float)rp;
       bphdata[2*block+1] = (float)lp;

       done=0;
       while (done == 0)
       {
          pv = 0.0;
          new_rp = new_lp = 0.0;

          if ( (r = do_aph(argc,argv,first,last,&new_rp,&new_lp,aph0 ? 0 : 1)) == 0)  
          {
              bphdata[2*block] = (float)new_rp;
              bphdata[2*block+1] = (float)new_lp;
              done = 1;
          }
          else if ( (r != 3) && (r = do_aph_n(first,last,&new_rp,&new_lp,aph0 ? 0 : 1))
        	== 0)  
          {
              bphdata[2*block] = (float)new_rp;
              bphdata[2*block+1] = (float)new_lp;
              done = 1;
          }  //----- end of conbination -------
          if (r == 3)
             done = -1;
          if (!done)
          {
             if (!aph0)
             {
               // Try with aph0 
               lp = save_lp;
               aph0 = 1;
             }
             else
             {
               // aph0 also failed, exit while loop 
               done = -1;
             }
          }
       }
       if(bphFile && done == 1) {
	  fprintf(bphFile,"%d 1 %f %f\n", c_buffer, new_rp, new_lp); 
       } else if(bphFile) {
          //Winfoprintf("cannot phase block %d",c_buffer);
	  fprintf(bphFile,"%d 0 %f %f\n", c_buffer, save_rp, save_lp); 
       }
       block++;

       // release memory used by do_aph and do_aph_n
       releaseAllWithId("aph");
 }

 return COMPLETE;
}

int writephase(char *path) {

  FILE *file;
  int block;
  int r, file_id = D_PHASFILE;

  if ( (file = fopen(path,"w")) == NULL)
  {
      Wscrprintf("cannot write phase file %s\n", path);
      return ERROR;
  }

  block=0;
  while(block < nblocks) {
   
    if(bphdata != NULL) {
       fprintf(file,"%d 1 %f %f\n", block, bphdata[2*block],bphdata[2*block+1]); 
    } else {
       if(block != c_buffer) {
        if (c_buffer >= 0)      /* release last used block */
        {
           if ( (r = D_release(file_id, c_buffer)) )
           {
              D_error(r);
              D_close(file_id);
              fclose(file);
              return(ERROR);
           }
        }

        if ( (r = D_getbuf(file_id, nblocks, block, &c_block)) )
        {
           if ( (r = D_allocbuf(file_id, block, &c_block)) )
           {
              D_error(r);
              fclose(file);
              return(ERROR);
           }
        }

        c_buffer = block;
       }

       fprintf(file,"%d 0 %f %f\n", c_buffer, c_block.head->rpval,c_block.head->lpval); 
     }
     block++;
  }
  fclose(file);
  return COMPLETE;
}

typedef char string[MAXSTR];

void getStrTok(char *buf, string *toks, int *n, char *delimiter) {
   // if n=0, read all toks, otherwise read max n toks
    int size;
    char *strptr, *tokptr;
    char  str[1000];

    size = *n;

    /* remove newline if exists */
    if(buf[strlen(buf)-1] == '\n') {
        strcpy(str, "");
        strncat(str, buf, strlen(buf)-1);
    } else
       strcpy(str, buf);

    strptr = str;
    *n = 0;
    while ((size<=0 || size>(*n)) &&
        (tokptr = (char*) strtok(strptr, delimiter)) != (char *) 0) {

        if(strlen(tokptr) > 0) {
            strcpy(toks[*n], tokptr);
            (*n)++;
        }
        strptr = (char *) 0;
    }

}

int readphase(char *path) {

  FILE *file;
  char buf[MAXSTR];
  string toks[4];
  int block, ntoks=4;

  if ( (file = fopen(path,"r")) == NULL)
  {
      Wscrprintf("cannot read phase file %s\n", path);
      return ERROR;
  }

  if(bphdata != NULL) releaseAllWithId("bph");
  if ((bphdata = (float *) allocateWithId(sizeof(float) * 2*nblocks,"bph"))==0)
  {
    Werrprintf("cannot allocate block phase buffer");
    fclose(file);
    return(ERROR);
  }

  // fill bphdata
  block=0;
  while (fgets(buf,sizeof(buf),file)) {
    if(strlen(buf) > 1 && buf[0] != '#') {
   
      getStrTok(buf,toks,&ntoks," ");
      if(block < nblocks) {
        // skip toks[1] (a flag to indicate whether auto phasing was successful
        bphdata[2*block] = (float)atof(toks[2]);
        bphdata[2*block+1] = (float)atof(toks[3]);
	block++;
      }
    }
  }
  // in case the file does not have enough lines for all blocks
  while(block < nblocks) {
        bphdata[2*block] = (float)rp;
        bphdata[2*block] = (float)lp;
	block++;
  }
  fclose(file);
  return COMPLETE;
}

void setBph0(int trace, float value) {
   int block = trace;
   if(specperblock>0) block = trace/specperblock;

   if(bphdata != NULL && block < nblocks) {
	bphdata[2*block] = value;
   }
}

void setBph1(int trace, float value) {
   int block = trace;
   if(specperblock>0) block = trace/specperblock;

   if(bphdata != NULL && block < nblocks) {
	bphdata[2*block+1] = value;
   }
}

float getBph0(int trace) {
   int block = trace;
   if(specperblock>0) block = trace/specperblock;

   if(bphdata != NULL && block < nblocks) {
      return bphdata[2*block];
   } else return rp;
}

float getBph1(int trace) {
   int block = trace;
   if(specperblock>0) block = trace/specperblock;

   if(bphdata != NULL && block < nblocks) {
      return bphdata[2*block+1];
   } else return lp;
}

int bphase(int argc, char *argv[], int retc, char *retv[]) {
  char path[MAXPATH];
  int res;
  vInfo paraminfo;

  if(P_getVarInfo(CURRENT, "blockph", &paraminfo) == -2) {
    P_creatvar(CURRENT, "blockph", ST_REAL);
    P_setgroup(CURRENT,"blockph", G_PROCESSING);
  }

  if(argc>1 && strcmp(argv[1],"off") == 0) {
    P_setreal(CURRENT,"blockph", 0.0, 1);
    return COMPLETE;
  } else if(argc>1 && strcmp(argv[1],"on") == 0) {
    P_setreal(CURRENT,"blockph", 1.0, 1);
    return COMPLETE;
  } else {
    P_setreal(CURRENT,"blockph", 1.0, 1);
  }

  if(argc>2 && argv[2][0] == '/') strcpy(path,argv[2]);
  else sprintf(path,"%s/datdir/bph.txt",curexpdir);

  if(argc>1 && strcmp(argv[1],"read") == 0) 
    return readphase(path);
  else if(argc>1 && strcmp(argv[1],"write") == 0) 
    return writephase(path);

  aph0 = (strcmp(argv[0], "bph0") == 0);

  aph_b = (strcmp(argv[0], "bphb") == 0);  // add for phasing Bruker data, Lynn Cai, 4-7-98

  Wturnoff_buttons();

  if(init2d(1, 1))
  {
     return ERROR;
  }
  if (datahead.status & S_DDR)
  {
    /*
     * aph for DDR data defaults to aph0 with lp=0.0
     * unless an "lp" argument is given, in which case
     * both rp and lp are optimized
     */
    if ( (argc == 1) || strcmp(argv[1],"lp") )
    {
       /* if aph0 is called, lp is left alone. Only aph sets it to 0 */
       if ( ! aph0)
          lp = 0.0;
       aph0 = 1;
    }
  }

  if(argc > 1 && isdigit(argv[1][0])) { // auto phase given block

    int ind = atoi(argv[1]);
    if((res = do_block_aph(argc,argv,0,fn/2,aph0 ? 0 : 1,NULL,ind,ind)) == 0) {
       return(COMPLETE); 
    } else {
       return(ERROR);
    }

  } else { // auto phase all blocks

    FILE *bphFile;
  
    if ( (bphFile = fopen(path,"w")) == NULL)
    {
       Wscrprintf("cannot write %s\n", path);
       return(ERROR);
    } else 
    if((res = do_block_aph(argc,argv,0,fn/2,aph0 ? 0 : 1, bphFile, 1, nblocks)) == 0) {
       fclose(bphFile);
       return(COMPLETE); 
    } else {
       fclose(bphFile);
       return(ERROR);
    }

  }
}
