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
/*  ll	 -	list line positions and intensities	 	*/
/*  dll	 -	display listed line positions and intensities	*/
/*  nll	 -	calculate line positions and intensities	*/
/*  nl	 -	position the cursor at the nearest line		*/
/*  peak -	return the freq. and intensity of largest peak	*/
/*  getll-	return the freq. and intensity of specified line*/
/*  fp	 -	find peaks in an array of spectra and place the	*/
/*		result in the file fp.out			*/
/*  adept -	automatic DEPT analysis				*/
/*  analyze -   analyze experimental data (T1,KINETICS,---)	*/
/*  expl -	exponential display/plot			*/
/*  dels -      remove spectra from analysis by eliminating     */
/*	        from 'fp.out'					*/
/*								*/
/****************************************************************/

/* GM 14xii09 correct loop count in getpeak                     */
/* GM 14xii09 correct end point in nearest_line                 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "allocate.h"
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"

#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define LINEAR		0
#define SQUARE		1
#define LOG		2
#define LOGE10		2.302585
#define WCMIN		1.0
#define NOTITLE         "No Title"

/*  MAXLINES is the maximum number of lines dll will search for */
#define MAXLINES        8192

extern int debug1;
extern int start_from_ft;	/* tells ds to recalc phasefile before disp. */
extern void Wmoreprintf(char *, ...);
extern void Wgmode();
extern void Wturnoff_buttons();
extern void Wshow_graphics();
extern void WmoreStart();
extern void WmoreEnd();
extern void UpdateVal(int direction, int n_index, double val, int displ);
extern void calc_noise(float *dptr, double rcrv, double lcrv, double *noise);
extern int  checkphase(short status);
extern int currentindex();
extern void set_sp_wp(double *spval, double *wpval, double swval, int pts, double ref);
extern int datapoint(double freq, double sw, int fn);
extern int dataheaders(int getphasefile, int checkstatus);
extern int removephasefile();
extern void checkreal(double *r, double min, double max);
extern void get_label(int direction, int style, char *label);
extern void get_scale_pars(int direction, double *start, double *len, double *scl, int *rev);
extern void set_scale_pars(int direction, double start, double len, double scl, int rev);
extern void set_scale_label(int direction, char *label);
extern void set_scale_axis(int direction, char axisv);
extern int scale2d(int drawbox, int yoffset, int drawscale, int dcolor);
extern int dstring(char *s);
extern int dvstring(char *s);
extern int show_plotterbox();
extern void get_rflrfp(int direction, double *val);
extern int adraw(int x, int y);
extern int amove(int x, int y);
extern int rdraw(int x, int y);
extern int box(int x, int y);
extern int color(int c);
extern int colorindex(char *colorname, int *index);
extern int setdisplay();
extern int endgraphics();
extern int  setplotter();
extern int kgetparams(char arrayname[], double *offset);
extern int do_aph(int argc, char *argv[], int first, int last,
           double *new_rp, double *new_lp, int order);


static float *spectrum;
static double scale;
static float *read_freq,*read_amp;
static char *linetype;
static int  *spec_index;
/*static int  line_index[128];*/
static int  *line_index;

/* Function prototypes */

static int write_fph_table(int, int, int [], double []);
static int readhead(FILE *, char [], char [], char [], int *, int *, int *);
static int readfile(FILE *, int *, int *, int *);
static int setypos(int, float, float, float);
static void do_expl_out(char [], char [], float, float, float, float, int);    
static void findth(float *, float *, int);
static int findpeaklines(float *, float, int);
static int find_a_line(float *, float, int, int *, float *, int *, int *, int);
static int new_css(int, int *, int, int, int);
static int setlbl(char *name);
static int printcurve(char *xaxis, int exptype,
               float a0, float a1, float a2, float a3);
static double peak_phase(float *av_data, int point, float thresh);
void find_nearest_peak(float *, int *);
void primary_color(int);
void setaxis(int, char *);
void syntaxerror(int, char *);
int fph(int argc, char *argv[], int retc, char *retv[]);
int checkscwc(char *scn,char *wcn,char *wcmaxn, double offset);
int pickLines(double noisemult, int pos, float *specData, int fptr, int npts, double thresh, double vScale, double *lineFrq, double *lineAmp, int maxLines, double sw, double rflrfp, int fn);

/****************/
static int freebuffers()
/****************/
{ int res;

  if(c_buffer>=0) /* release last used block */
  {
    if ( (res=D_release(D_PHASFILE,c_buffer)) )
    {
      D_error(res); 
      D_close(D_PHASFILE);  
      return(ERROR);
    }
  }
  return(COMPLETE);
}

/*************/
static void setwindows()
/*************/
{
  Wclear_text();
  Wshow_text();
}

/******************************/
static double dp_to_frq(int dp, double sw, int fn)
/******************************/
{
  return((double) (sw * ( 1.0 - ((double) dp / (double) fn))));
}

/****************/
static int calc_lines(int argc, char *argv[], int retc, char *retv[])
/****************/
/*----------------------------------------------------------------------+
| Date      Author	Modification					|
| --------  ----------  ------------					|
| 1/21/92   Rolf K.	1. algorithm completely rewritten to allow for	|
|			   noise suppression (less response from broad,	|
|			   noisy signals)				|
+----------------------------------------------------------------------*/
{
  int    pos = FALSE;		/* default: show positive and negative peaks */
  int    ind;
  double noisemult = 3.0;	/* default: noisemult = 3.0 */

  /* two arguments can be supplied to dll and nll:                     */
  /*    noisemult, a real number that influences the noise suppression */
  /*    'pos' - causing negative lines to be suppressed                */
  /*    the order of the arguments is not relevant.		       */
  if (argc > 1)
  {
    if (isReal(argv[1]))
    {
      noisemult = stringReal(argv[1]);
      if (noisemult < 0.0)
        noisemult = 3.0;
      if (argc > 2)
      {
        if (strcmp(argv[2],"pos")==0)
          pos = TRUE;
      }
    }
    else
    {
      if (strcmp(argv[1],"pos")==0)
        pos = TRUE;
      if (argc > 2)
      {
        if (isReal(argv[2]))
        {
          noisemult = stringReal(argv[2]);
          if (noisemult < 0.0)
            noisemult = 3.0;
        }
      }
    }
  }

  ind = pickLines(noisemult, pos, spectrum, fpnt, npnt, th, scale, NULL, NULL, 0,sw,rflrfp,fn);

  if ((strcmp(argv[0],"nll")==0) && (retc==0) 
	&& (argc<2 || strcmp(argv[1],"dpf") != 0 ))
    Winfoprintf("%d lines have been found",ind);
  if (retc>0)
    retv[0] = realString((double) ind);
  if (retc>1)
    retv[1] = realString((double) scale);
  return COMPLETE;
}

int pickLines(double noisemult, int pos, float *spec, int fptr, int npts, 
	double thresh, double vScale, double *lineFrq, double *lineAmp, int maxLines, double newsw, double newrflrfp, int newfn) {

  register float  thv;
  int    r;
  register int    index;
  register int    nw;
  register int    state;
  double noise,noiselim;
  double noise2;
  double llim,rlim;
  register int    found;
  register float  last;
  register float  point;
  float  localmin;
  float  localmax;
  int    localminp;
  int    localmaxp;
  double llamp;
  int    llpos;

#define SEARCH   0
#define UP       1
#define DOWN     2
#define MIN      3
#define MAX      4

  sw=newsw;
  rflrfp=newrflrfp;
  fn = newfn;

  thv = thresh / vScale;


  /* Get noise at right edge of spectrum */
  if (fn > 257)  /* make sure noise calculation uses several points */
    llim = (double) (0.02*sw - rflrfp);
  else
    llim = (double) (0.05*sw - rflrfp);
  rlim =  - rflrfp;
  calc_noise(spec, rlim, llim, &noise2);
  /* Get noise at left edge of spec */
  llim = (double) (sw - rflrfp);
  if (fn > 257)  /* make sure noise calculation uses several points */
    rlim = (double) (0.98*sw - rflrfp);
  else
    rlim = (double) (0.95*sw - rflrfp);
  calc_noise(spec, rlim, llim, &noise);
  /* Use smallest of right-edge left-edge noise */
  if (noise2 < noise)
    noise = noise2;
  noiselim = noisemult * noise;
  if (debug1)
    Wscrprintf("rms noise: %g\n", noise*vScale);

  if(lineFrq == NULL || lineAmp == NULL) {

    if ( (r=P_setreal(CURRENT,"llfrq",0.0,0)) )  /* reset the array */
    { P_err(r,"llfrq",":");   return 0; }
    if ( (r=P_setreal(CURRENT,"llamp",0.0,0)) )  /* reset the array */
    { P_err(r,"llamp",":");   return 0; }
  }

/***********************************************************************
*  find lines above threshold, try to avoid multiple lines for         *
*  noisy, broad signals. Only peaks with minima (noisemult * rmsnoise) *
*  below the maximum on both sides are listed. The default for         *
*  noisemult is 3, which seems to work fine for 1H and similar spectra *
*  with high signal-to-noise; for 13C spectra with very low s/n        *
*  smaller values for noisemult (<<1.0) allow to detect more signals.  *
*  noisemult=0.0 will display all maxima.                              *
*  Negative values for noisemult (argument 1) default to 3.0 .         *
***********************************************************************/

  nw = fptr;
  last = *(spec + fptr);
  index = 0;
  localmax = localmin = llamp = last;
  localmaxp = localminp = llpos = nw;
  state = SEARCH;
  nw++;
  while ((nw < fptr + npts) && (index < MAXLINES))
  {
    point = *(spec + nw);
    found = FALSE;
    switch (state)
    {
      /* state SEARCH is ONLY USED AT THE START (left end) of the spectrum;
	 this state is maintained until a possible minimum or maximum is
	 approached, in which case we switch to MIN, resp. MAX state */
      case SEARCH: if (point > last)
                   {
                     if ((point - localmin) > noiselim)
                     {
                       state = MAX;
                     }
                     if (point > localmax)
                     {
                       localmax = point;
                       localmaxp = nw;
                     }
                   }  
                   else
                   {   
                     if ((localmax - point) > noiselim)
                     {
                       state = MIN;
                     }
                     if (point < localmin)
                     {
                       localmin = point;
                       localminp = nw;
                     }
                   }  
                   break;
      /* explanation for the various states:
	 UP means the spectrum is moving up, but the last local minimum is
	    closer than the specified limit
	 MAX means a movement up, exceeding the limit between the last local
	    minimum and the current point
	 MIN means a movement down, ecceeding the limit between the last local
	    maximum and the current point
	 DOWN means a movement down, where the last local maximum is closer
	    than the specified limit;
	 a maximum is only expelled when moving into MIN state, at which
	    point a local maximum becomes a peak;
	 a minimum is only expelled when moving into MAX state, at which
	    point a local minimum becomes a peak, unless "pos" is selected */
      case UP:     if (point < last)  /* turns down */
                   {
	             /* set local maximum, if necessary */
                     if ((localmax < last) && (localmaxp > localminp))
                     {
                       localmax = last;
                       localmaxp = nw - 1;
                     }
                     if ((localmax - point) > noiselim)
                     {
                       state = MIN;
                       if ((localmax > thv) && (localmaxp != llpos))
                       {
                         found = TRUE;
                         llamp = localmax;
                         llpos = localmaxp;
                       }
	               /* set local minimum, if necessary */
                       if (localmaxp > localminp)
                       {
                         localmin = point;
                         localminp = nw;
                       }
                     }  
                     else	/* no minimum, just turn down */
                     {   
                       state = DOWN;
                     }
                   }  
                   else  /* continues same direction */
                   {
                     if ((point - localmin) > noiselim)
                     {
                       state = MAX;
                       if ((localmin < -thv) &&
                            (localminp != llpos) && (pos == FALSE))
                       {
                         found = TRUE;
                         llamp = localmin;
                         llpos = localminp;
                       }
	               /* set local maximum, if necessary */
                       if (localmaxp < localminp)
                       {
                         localmax = point;
                         localmaxp = nw;
                       }
                     }  
                   }  
                   break;
      case DOWN:   if (point > last)  /* turns up */
                   {
	             /* set local minimum, if necessary */
                     if ((localmin > last) && (localminp > localmaxp))
                     {
                       localmin = last;
                       localminp = nw - 1;
                     }
                     if ((point - localmin) > noiselim)
                     {
                       state = MAX;
                       if ((localmin < -thv) &&
                            (localminp != llpos) && (pos == FALSE))
                       {
                         found = TRUE;
                         llamp = localmin;
                         llpos = localminp;
                       }
	               /* set local maximum, if necessary */
                       if (localmaxp < localminp)
                       {
                         localmax = point;
                         localmaxp = nw;
                       }
                     }  
                     else	/* no maximum, just turn up */
                     {   
                       state = UP;
                     }
                   }  
                   else  /* continues same direction */
                   {
                     if ((localmax - point) > noiselim)
                     {
                       state = MIN;
                       if ((localmax > thv) && (localmaxp != llpos))
                       {
                         found = TRUE;
                         llamp = localmax;
                         llpos = localmaxp;
                       }
	               /* set local minimum, if necessary */
                       if (localmaxp > localminp)
                       {
                         localmin = point;
                         localminp = nw;
                       }
                     }  
                   }  
                   break;
      case MAX:    if (point < last)  /* turns down */
                   {
	             /* set local maximum, if necessary */
                     if (localmax < last)
                     {
                       localmax = last;
                       localmaxp = nw - 1;
                     }
                     if ((last - point) > noiselim)
                     {
                       state = MIN;
                       if ((last > thv) && (nw - 1 != llpos))
                       {
                         found = TRUE;
                         llamp = last;
                         llpos = nw - 1;
                       }
	               /* set local minimum, if necessary */
                       if (localmaxp > localminp)
                       {
                         localmin = point;
                         localminp = nw;
                       }
                     }  
                     else	/* no minimum, just turn down */
                     {   
                       state = DOWN;
                     }
                   }  
                   break;
      case MIN:    if (point > last)  /* turns up */
                   {
	             /* set local minimum, if necessary */
                     if (localmin > last)
                     {
                       localmin = last;
                       localminp = nw - 1;
                     }
                     if ((point - last) > noiselim)
                     {
                       state = MAX;
                       if ((last < -thv) &&
                           (nw - 1 != llpos) && (pos == FALSE))
                       {
                         found = TRUE;
                         llamp = last;
                         llpos = nw - 1;
                       }
	               /* set local maximum, if necessary */
                       if (localmaxp < localminp)
                       {
                         localmax = point;
                         localmaxp = nw;
                       }
                     }  
                     else	/* no maximum, just turn up */
                     {   
                       state = UP;
                     }
                   }  
                   break;
    }
    if ((found) && !(pos && llamp < 0.0) )
    {
      index++;
      double frq = dp_to_frq(llpos,sw,fn / 2);
      if(lineFrq == NULL || lineAmp == NULL) {
        if ( (r=P_setreal(CURRENT,"llamp",(double) llamp,index)) )
        { P_err(r,"llamp",":");   return index; }
        if ( (r=P_setreal(CURRENT,"llfrq",frq,index)) )
        { P_err(r,"llfrq",":");   return index; }
      } else if(index < maxLines) {
	lineFrq[index-1]=frq;
	lineAmp[index-1]=llamp;
      }
    }
    last = point;
    nw++;
  }

  return index;
}

/****************/
static int print_lines()
/****************/
{
  vInfo  info;
  double freq;
  int    r;
  int    index;
  double amp;
  char   line1[80];
  char   units[15];
  double start,len,axis_scl;
  int reversed;

  if ( (r=P_getVarInfo(CURRENT,"llfrq",&info)) )
  { P_err(r,"llfrq",":");   return ERROR; }

  get_label(HORIZ,UNIT1,units);
  get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
  sprintf(line1,"index   freq %-8.8s  intensity\n",units);
  WmoreStart();
  Wmoreprintf(line1);

  for (index = 1; index <= info.size; index++)
  {
    if ( (r=P_getreal(CURRENT,"llfrq",&freq,index)) )
    { P_err(r,"llfrq",":");   return ERROR; }
    freq = (freq - rflrfp) / axis_scl;
    if ( (r=P_getreal(CURRENT,"llamp",&amp,index)) )
    { P_err(r,"llamp",":");   return ERROR; }
    Wmoreprintf("%4ld  %10g    %10g\n",index,freq,amp * scale);
  }
  WmoreEnd();
  return COMPLETE;
}

/*****************************************/
static void getpeak(float *spectrum, int fp, int np, float *max, double *freq)
/*****************************************/
{
  float  absmax;
  float *index,*datapntr;

  absmax = *max = 0.0;
  index = datapntr = spectrum + fp;
/* GM 14xii09 loop over np points not np-1 */
  while (--np > -1 )
  {
    if (fabs(*datapntr) > absmax)
    {
      index = datapntr;
      *max = *datapntr++;
      absmax = fabs((double) *max);
    }
    else datapntr++;
  }
  *freq = dp_to_frq((int) (index - spectrum),sw,fn / 2) - rflrfp;
}

void getNearestPeak(float *spectrum, int fp, int np, float *max, double *freq) {
    getpeak(spectrum, fp, np, max, freq);
}

/*****************************************/
static void getpeakmin(float *spectrum, int fp, int np,
                      float *min, double *freq)
/*****************************************/
{
  float  absmin;
  float *index,*datapntr;

  absmin = *min = 1.0e20;
  index = datapntr = spectrum + fp;
  while (--np)
  {
    if (fabs(*datapntr) < absmin)
    {
      index = datapntr;
      *min = *datapntr++;
      absmin = fabs((double) *min);
    }
    else datapntr++;
  }
  *freq = dp_to_frq((int) (index - spectrum),sw,fn / 2) - rflrfp;
}

static void disp_line(double amp, double freq, int retc, char *retv[])
{
  if (retc==0)
  {
    char   units[15];
    double start,len,axis_scl;
    int reversed;

    get_label(HORIZ,UNIT1,units);
    get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
    Winfoprintf("height= %g mm   frequency= %g %s",
                 scale*amp,freq/axis_scl,units);
  }
  if (retc>0)
    retv[0] = realString((double) (scale * amp));
  if (retc>1)
    retv[1] = realString((double) freq);
  if (retc>2)
    retv[2] = realString((double) amp);
}

/**********************************/
static double nearest_line(double cr, double rflrfp, int retc, char *retv[])
/**********************************/
{
  double freq;
  float  max;
  int    start;
  int    le;

  start = datapoint(cr+rflrfp,sw,fn/2);
  le = npnt / 40;
  if (le > 400)
    le = 400;
  else if (le < 10)
    le = 10;
  start -= le / 2 - 1;
  if (start < 0)
    start = 0;
  if (start + le >= fn/2)
/* GM 14xii09 last point to search = fn/2 - 1, so le points from start */
    start = fn / 2 - le;
  getpeak(spectrum,start,le,&max,&freq);
  disp_line((double) max, freq, retc, retv);
  return(freq);
}

/*************************************/
static int priv_getline(int argc, char *argv[], int retc, char *retv[])
/*************************************/
{
  vInfo  info;
  double freq;
  double amp;
  int    r;
  int    index;

  if (argc > 1)
    if (isReal(*++argv))
      index = (int) stringReal(*argv);
    else
    {
      Werrprintf("argument must be the line index");
      return(ERROR);
    }
  else
    index = 1;
    
  if ( (r=P_getVarInfo(CURRENT,"llfrq",&info)) )
  { P_err(r,"llfrq",":");   return ERROR; }
  if ((index > info.size) || (index < 1))
  {
    Werrprintf("line %d not found",index);
    return(ERROR);
  }
  if ( (r=P_getreal(CURRENT,"llfrq",&freq,index)) )
  { P_err(r,"llfrq",":");   return ERROR; }
  freq -= rflrfp;
  if ( (r=P_getreal(CURRENT,"llamp",&amp,index)) )
  { P_err(r,"llamp",":");   return ERROR; }
  disp_line(amp, freq, retc, retv);
  return(COMPLETE);
}

/**********************************/
static void printmaxpeak(int fp, int np, int retc, char *retv[])
/**********************************/
{
  int dummy;
  float  maxpeak;
  double freq;
  int  numspec = nblocks * specperblock;
  int i = 0;
  float maxp=0.0;
  double frq=0.0;
  while(numspec > 0) {
    if ( (spectrum = calc_spec(i,0,FALSE,TRUE,&dummy)) ) {
      getpeak(spectrum,fp,np,&maxpeak,&freq);
      if(maxpeak > maxp) {
	maxp=maxpeak;
	frq=freq;
      }
    }
    numspec--;
    i++;
  }
  disp_line((double) maxp, frq, retc, retv);
}

/**********************************/
static void printpeak(int fp, int np, int retc, char *retv[])
/**********************************/
{
  float  maxpeak;
  double freq;

  getpeak(spectrum,fp,np,&maxpeak,&freq);
  disp_line((double) maxpeak, freq, retc, retv);
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
  set_sp_wp(&sp, &wp, sw, fn/2, rflrfp);
  exp_factors(TRUE);
  return(COMPLETE);
}

/*************/
int dll(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int ctrace;
  int dummy;

  if (argv[0][0] == 'd') {
    Wturnoff_buttons();
    setwindows();
  }
  if(init2d(1,1)) return(ERROR);
  ctrace = currentindex();
  if ((spectrum = calc_spec(ctrace-1,0,FALSE,TRUE,&dummy))==0)
    return(ERROR);
  scale = vs;
  if (normflag)
    scale *= normalize;
  if (strcmp(argv[0],"maxpeak")==0) {
    if (argc>1)
      if (checkinput(argc,argv))
        return(ERROR);
    printmaxpeak(fpnt,npnt,retc,retv);
    appendvarlist("dummy");
  } 
  else if (strcmp(argv[0],"peak")==0)
  {
    if (argc>1)
      if (checkinput(argc,argv))
        return(ERROR);
    printpeak(fpnt,npnt,retc,retv);
    appendvarlist("dummy");
  }
  else if (strcmp(argv[0],"peakmin")==0)
  {
    float  minpeak;
    double freq;

    if (argc>1)
      if (checkinput(argc,argv))
        return(ERROR);
    getpeakmin(spectrum,fpnt,npnt,&minpeak,&freq);
    disp_line((double) minpeak, freq, retc, retv);
    appendvarlist("dummy");
  }
  else if (strcmp(argv[0],"getll")==0)
  {
    priv_getline(argc,argv,retc,retv);
    appendvarlist("dummy");
  }
  else if (strcmp(argv[0],"nl")==0) 
  {
    cr = nearest_line(cr,rflrfp,retc,retv);
    UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
    appendvarlist("cr");
  }
  else if (strcmp(argv[0],"nl2")==0) 
  {
    double del;
    del = nearest_line(cr - delta,rflrfp,retc,retv);
    delta = cr - del;
    UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
    appendvarlist("cr");
  }
  else  // dll, nll
  {
    calc_lines(argc,argv,retc,retv);
    appendvarlist("llfrq,llamp");
    if (argv[0][0] != 'n')
    {
      print_lines();
      Wsettextdisplay("dll");
    }
  }
  if (freebuffers()) return(ERROR);
  return(COMPLETE);
}

/*****************************************/
static int write_fp_table(int numlines, int numspec, int dpoint[], double amp[])
/*****************************************/
{
  FILE *datafile;
  char  filename[MAXPATH];
  int   line_no,spec_no,r;
  double freq;

/* count the number of active lines */
  r = 0;
  for (line_no=1; line_no <= numlines; line_no++)
    if (dpoint[line_no] >= 0)
      r++;
/* open fp.out  */
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fp.out");
#else
  strcat(filename,"fp.out");
#endif
  if ( (datafile=fopen(filename,"w+")) )
  {
    fprintf(datafile,"List of %d lines in spectrum 1 to spectrum %d\n",
            r,numspec);

/* output frequencies of the active lines */
    fprintf(datafile,
        "line         frequency (Hz)\n");
    for (line_no=1; line_no <= numlines; line_no++)
      if (dpoint[line_no] >= 0)
      {
        if ( (r=P_getreal(CURRENT,"llfrq",&freq,line_no)) )
        { P_err(r,"llfrq",":");
          fclose(datafile);
          return ERROR;
        }
        fprintf(datafile,"%4d       %11.2f\n",line_no,freq - rflrfp);
      }

/* output amplitudes of the active lines */
    fprintf(datafile,
        "line    spectrum    amplitude (mm)\n");
    for (line_no = 1; line_no <= numlines; line_no++)
      for (spec_no = 1; spec_no <= numspec; spec_no++)
        if (dpoint[line_no] >= 0)
          fprintf(datafile,"%4d        %4d  %11.2f\n",
                 line_no,spec_no,amp[numspec * (line_no - 1) + spec_no]);
    fclose(datafile);
    chmod(filename,0666);
    return 0;
  }
  else
  { Werrprintf("cannot open file %s",filename);
    return 1;
  }
}

/************************************/
static int read_fp_table(int *numlines, int *numspec)
/************************************/
{
  FILE *datafile;
  char  filename[MAXPATHL];
  int   line_no,spec_no,i,l;

  /* open fp.out  */
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fp.out");
#else
  strcat(filename,"fp.out");
#endif
  if ( (datafile=fopen(filename,"r")) )
  { l = 1;
    if (fscanf(datafile,"List of %d lines in spectrum 1 to spectrum %d\n",
            numlines,numspec)!=2)
      { syntaxerror(l,filename);
        fclose(datafile);
        return 1;
      }
    l++;
    
    /* read frequencies of the active lines */
    fscanf(datafile,"line         frequency (Hz)\n");
    l++;
    if ((read_freq=(float *) allocateWithId(sizeof(float) * *numlines,
                                                           "readfp"))==0 ||
        (line_index=(int *) allocateWithId(sizeof(int) * *numlines,
                                                           "readfp"))==0)
      { Werrprintf("cannot allocate buffer memory");
        fclose(datafile);
        return 1;
      }
    if ((read_amp=(float *) allocateWithId(sizeof(float) * *numlines * *numspec,
                                                           "readfp"))==0 ||
        (spec_index=(int *) allocateWithId(sizeof(int)  *  *numlines * *numspec,
                                                           "readfp"))==0)
      { Werrprintf("cannot allocate buffer memory");
        fclose(datafile);
        return 1;
      }
    for (line_no=0; line_no < *numlines; line_no++)
      { if (fscanf(datafile,"%d%f\n",&i,&read_freq[line_no])!=2)
	  { syntaxerror(l,filename);
            fclose(datafile);
            return 1;
          }
/***************
        if (i!=line_no+1)
          { Werrprintf("wrong index in file %s, line %d",filename,l);
            fclose(datafile);
            return 1;
          }
****************/
        l++;
      }

    /* read amplitudes of the active lines */
    /* fscanf(datafile,"line    spectrum    amplitude (mm)\n"); */
          fscanf(datafile,"%*[^\n]\n");
    l++;
    for (line_no = 0; line_no < *numlines; line_no++)
      for (spec_no = 0; spec_no < *numspec; spec_no++)
        { if (fscanf(datafile,"%d%d%f\n",&line_index[line_no],
                      &spec_index[*numspec * line_no + spec_no],
                        &read_amp[*numspec * line_no + spec_no]) != 3)
	    { syntaxerror(l,filename);
              fclose(datafile);
              return 1;
            }
/********************
          if ((i!=line_no+1)||(j!=spec_no+1))
            { Werrprintf("wrong index in file %s, line %d",filename,l);
              fclose(datafile);
              return 1;
            }
********************/
          l++;
        }
    fclose(datafile);
    return 0;
  }
  else
  { Werrprintf("cannot open file %s, use 'nll' and 'fp'",filename);
    return 1;
  }
}

#define NPOINT 2

/******************************/
static double fpeak(int dp, int npoints)
/******************************/
{
  register float *ptr;
  register int i;
  register float max;

  ptr = spectrum + dp;
  i = 2 * npoints;
  max = 0.0;
  while (--i)
    if (fabs(max) < fabs(*ptr))
      max = *ptr++;
    else
      ptr++;
  return((double) max);
}

/*************/
int fp(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  vInfo   info;
  int     r;
  int     numspec,numlines;
  int     spec_no,line_no;
  int     dummy;
  int    *dpoint;
  int     npoints;
  double  value;
  double *amp;

  Wturnoff_buttons();
  if(init2d(1,1)) return(ERROR);
  disp_status("FP      ");
  if ((argc > 1) && (strcmp(argv[1],"phase") == 0))
  {
    fph(argc,argv,retc,retv);
  }
  else
  {
    if ( (r=P_getreal(CURRENT,"npoint",&value,1)) )
      npoints = NPOINT;
    else
    {
      if ( (r=P_getVarInfo(CURRENT,"npoint",&info)) ) 
        info.active = ACT_OFF;
      if (!info.active)
        npoints = NPOINT;
      else
      {
        npoints = (int) value;
        if (npoints < 1)
          npoints = 1;
        else if (npoints > fn/4)
          npoints = fn/4;
      }
    }
    if ( (r=P_getVarInfo(CURRENT,"llfrq",&info)) )
    { P_err(r,"llfrq",":");   return ERROR; }
    numlines = info.size;
    numspec = nblocks * specperblock;
    if ((dpoint = (int *) allocateWithId(sizeof(int) * (numlines + 1),"fp"))==0)
    {
      Werrprintf("cannot allocate frequency buffer");
      return(ERROR);
    }
    if ((amp = (double *) allocateWithId(
             sizeof(double) * (numspec * numlines + 1),"fp"))==0)
    {
      Werrprintf("cannot allocate amplitude buffer");
      release(dpoint);
      return(ERROR);
    }
/* dpoint[line_no] >= 0  means line is active */
/* dpoint[line_no] == -1 means line is not active */
    for (line_no = 1; line_no <= numlines; line_no++)
      dpoint[line_no] = (argc == 1) - 1;
    while (argc-- > 1)
    {
      if (isReal(*++argv))
      {
        line_no = (int) stringReal(*argv);
        if ((line_no >= 1) && (line_no <= numlines))
          dpoint[line_no] = 0;
        else
        {
          Werrprintf("line %d does not exist:  range is 1 to %d",
                    line_no,numlines);
          releaseAllWithId("fp");
          return(ERROR);
        }
      }
      else
      {
        Werrprintf("arguments must be the line indexes");
        releaseAllWithId("fp");
        return(ERROR);
      }
    }
    for (line_no = 1; line_no <= numlines; line_no++)
    {
      if (dpoint[line_no] >= 0)
      {
        if ( (r=P_getreal(CURRENT,"llfrq",&value,line_no)) )
          value = 0.0;          /*  no resets defined  */
        dpoint[line_no] = datapoint(value,sw,fn/2) - (npoints - 1);
        if (dpoint[line_no] <= 0)
          dpoint[line_no] = 0;
        else if (dpoint[line_no] + 2 * npoints >= fn/2)
          dpoint[line_no] = fn/2 - 2 * npoints;
        if (debug1)
          Wscrprintf("line no. %d   data point is %d   freq= %g\n",
                    line_no,dpoint[line_no],value);
      }
    }
    scale = vs;
    for (spec_no = 1; spec_no <= numspec; spec_no++)
    {
      if ( (spectrum = calc_spec(spec_no-1,0,FALSE,TRUE,&dummy)) )
      {
        if (normflag)
          scale = vs * normalize;
        for (line_no = 1; line_no <= numlines; line_no++)
          if (dpoint[line_no] >= 0)
            amp[numspec*(line_no-1)+spec_no] =
                        scale * fpeak(dpoint[line_no],npoints);
      }
      else
      {
        for (line_no = 1; line_no <= numlines; line_no++)
          amp[numspec*(line_no-1)+spec_no] = 0.0;
      }
    }

    write_fp_table(numlines,numspec,dpoint,amp);
    releaseAllWithId("fp");
    appendvarlist("dummy");
    if (freebuffers()) return(ERROR);
  }
  disp_status("        ");
  return(COMPLETE);
}

#define PEAK_THRESH 0.05

/****************************************************************************
* FPH() Modeled after fp for finding amplitude of lines picked by dll, fph
*	finds phases of peaks.  Starting at top of peak found by dll (which
*	finds peaks in the real part of the spectrum which are not necessarily
*	in phase), find nearest peak in the absolute-value spectrum and tries
*	to walk down the sides of the peak until it gets to the baseline.
*	Then hands that region of the spectrum to aph which tries to find a
*	zero-order phase for that region of the spectrum.
* Problems :
*	aph (and also this function) don't work very well on very noisy
*	spectra, or on spectra with fairly broad lines.
****************************************************************************/
/*************/
int fph(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  vInfo   info;
  int     r, res, i;
  int     numspec,numlines;
  int     spec_no,line_no;
  int     dummy, old_spec_index;
  int    *dpoint;
  double  value;
  double *phase;
  float  *phase_data, *av_data, thresh, max;
  struct datapointers datablock;

  (void) retc;
  (void) retv;
  argv++; argc--;
  if ( (r=P_getVarInfo(CURRENT,"llfrq",&info)) )
  { P_err(r,"llfrq",":");   return ERROR; }
  numlines = info.size;
  numspec = nblocks * specperblock;
  if ((dpoint = (int *) allocateWithId(sizeof(int) * (numlines + 1),"fph"))==NULL)
  {
    Werrprintf("cannot allocate frequency buffer");
    return(ERROR);
  }
  if ((phase = (double *) allocateWithId(
             sizeof(double) * (numspec * numlines + 1),"fph"))==NULL)
  {
    Werrprintf("cannot allocate phase buffer");
    release(dpoint);
    return(ERROR);
  }
/* dpoint[line_no] >= 0  means line is active */
/* dpoint[line_no] == -1 means line is not active */
  for (line_no = 1; line_no <= numlines; line_no++)
    dpoint[line_no] = (argc == 1) - 1;
  while (argc-- > 1)
    if (isReal(*++argv))
    {
      line_no = (int) stringReal(*argv);
      if ((line_no >= 1) && (line_no <= numlines))
        dpoint[line_no] = 0;
      else
      {
        Werrprintf("line %d does not exist:  range is 1 to %d",
                    line_no,numlines);
        releaseAllWithId("fph");
        return(ERROR);
      }
    }
    else
    {
      Werrprintf("arguments must be the line indexes");
      releaseAllWithId("fph");
      return(ERROR);
    }

  if ((av_data = (float *) allocateWithId(sizeof(float) * (fn/2),"fph"))==NULL)
  {
    Werrprintf("cannot allocate frequency buffer");
    return(ERROR);
  }

  for (line_no = 1; line_no <= numlines; line_no++)
    if (dpoint[line_no] >= 0)
    {
      if ( (r=P_getreal(CURRENT,"llfrq",&value,line_no)) )
        value = 0.0;          /*  no resets defined  */
      dpoint[line_no] = datapoint(value,sw,fn/2);
      if (dpoint[line_no] < 0)
        dpoint[line_no] = 0;
      else if (dpoint[line_no] >= fn/2)
        dpoint[line_no] = fn/2 - 1;
      if (debug1)
        Wscrprintf("line no. %d   data point is %d   freq= %g\n",
                    line_no,dpoint[line_no],value);
    }

  old_spec_index = specIndex;
  scale = vs;
  if (normflag)
    scale = vs * normalize;
  for (spec_no = 1; spec_no <= numspec; spec_no++)
    if ( (spectrum = calc_spec(spec_no-1,0,FALSE,TRUE,&dummy)) )
    {
      specIndex = spec_no;
      if ( (res = D_getbuf(D_DATAFILE, nblocks, c_buffer, &datablock)) )
      {
        D_error(res);
        return COMPLETE;
      }
      if (checkphase(datablock.head->status)) {
        Werrprintf("Cannot phase this data");
        return ERROR;
        }
      phase_data = datablock.data;
      max = -1.0e+20;
      for (i=0;i<fn/2;i++)  {
        av_data[i] = sqrt(*(phase_data+2*i)*(*(phase_data+2*i)) +
                     (*(phase_data+2*i+1))*(*(phase_data+2*i+1)));
        if (av_data[i] > max)
	  max = av_data[i];
	}
      for (line_no = 1; line_no <= numlines; line_no++)  {
	if (dpoint[line_no] >= 0) {
          find_nearest_peak(av_data,&dpoint[line_no]);
	}
      }
      for (line_no = 1; line_no <= numlines; line_no++)
        if (dpoint[line_no] >= 0)  {
          thresh = PEAK_THRESH*av_data[dpoint[line_no]];  /*th/scale*/  /*max*/
          phase[numspec*(line_no-1)+spec_no] = peak_phase(av_data,dpoint[line_no],thresh);
	  }
    }
    else
      for (line_no = 1; line_no <= numlines; line_no++)
        phase[numspec*(line_no-1)+spec_no] = 0.0;

  specIndex = old_spec_index;
  write_fph_table(numlines,numspec,dpoint,phase);
  releaseAllWithId("fph");
  appendvarlist("dummy");
  if (freebuffers()) return(ERROR);
  disp_status("        ");
  return(COMPLETE);
}

/****************************************************************************
* FIND_NEAREST_PEAK() - finds a peak in the spectrum "av_data" which is nearest
*	to the point "point".  (av_data intended to be absolute-value spectrum).
****************************************************************************/
/***********************************/
void find_nearest_peak(float *av_data, int *point)
/***********************************/
{
    int i, max_i;
    float max, pres, thv;

    thv = th / scale;
    max_i = *point;
    i = 1;
    while ((*(av_data + *point + i) < thv) && (*(av_data + *point - i) < thv) &&
		(*point + i < fn/2) && (*point-i > 0)){
      i++;
      max_i = i;
      }
    max = thv;
    while ((*point + i+2 < fn/2) && (*point - i-2> 0) &&
          ((*(av_data + *point+i)>=max)||(*(av_data + *point-i)>=max)
        || (*(av_data + *point+i+1)>=max)||(*(av_data + *point-i-1)>=max)
        || (*(av_data + *point+i+2)>=max)||(*(av_data + *point-i-2)>=max))){
      if ((pres = *(av_data + *point + i)) > max)  {
	max = pres;
	max_i = *point + i;
	}
      if ((pres = *(av_data + *point - i)) > max)  {
	max = pres;
	max_i = *point - i;
	}
      i++;
      }
    *point = max_i;
}

/****************************************************************************
* PEAK_PHASE() - Takes an absolute-valued spectrum "av_data" and peak point
*	"point" and a threshhold for deciding how low to go when trying
*	to define the limits of the peak.  Returns the 0th order phase
*	which phases the peak at "point".
****************************************************************************/
/***********************************/
static double peak_phase(float *av_data, int point, float thresh)
/***********************************/
{
    int i, left_pt, right_pt;
    float thresh_1, thresh_2;
    double new_rp, new_lp;
    char *argv[1];

    thresh_1 = thresh;
    thresh_2 = 1.1*thresh_1;

    i = point;
    while ((i > 0) && (*(av_data + i) > thresh_1))
      i--;
    left_pt = i;
    while ((i > 0) && (*(av_data + i) < thresh_2))
      i--;
    left_pt -= ((left_pt - i)/2);
    i = point;
    while ((i < fn/2) && (*(av_data + i) > thresh_1))
      i++;
    right_pt = i;
    while ((i < fn/2) && (*(av_data + i) < thresh_2))
      i++;
    right_pt += ((i - right_pt)/2);
/*fprintf(stderr,"l=%d,p=%d,r=%d\n",left_pt,point,right_pt);
fprintf(stderr,"l=%f,p=%f,r=%f\n",dp_to_frq(left_pt,sw,fn/2) - rflrfp,
				  dp_to_frq(point,sw,fn/2) - rflrfp,
				  dp_to_frq(right_pt,sw,fn/2) - rflrfp);*/
    do_aph(0,argv,left_pt,right_pt,&new_rp,&new_lp,0);
    return(new_rp);
}

/****************************************************************************
* Just like write_fp_table for writing out peak phases.
****************************************************************************/
/*****************************************/
static int write_fph_table(int numlines, int numspec, int dpoint[], double phase[])
/*****************************************/
{
  FILE *datafile;
  char  filename[MAXPATHL];
  int   line_no,spec_no,r;
  double freq;

/* count the number of active lines */
  r = 0;
  for (line_no=1; line_no <= numlines; line_no++)
    if (dpoint[line_no] >= 0)
      r++;
/* open fp.out  */
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fp.out");
#else
  strcat(filename,"fp.out");
#endif
  if ( (datafile=fopen(filename,"w+")) )
  {
    fprintf(datafile,"List of %d lines in spectrum 1 to spectrum %d\n",
            r,numspec);

/* output frequencies of the active lines */
    fprintf(datafile,
        "line         frequency (Hz)\n");
    for (line_no=1; line_no <= numlines; line_no++)
      if (dpoint[line_no] >= 0)
      {
        if ( (r=P_getreal(CURRENT,"llfrq",&freq,line_no)) )
        { P_err(r,"llfrq",":");
          fclose(datafile);
          return ERROR;
        }
        fprintf(datafile,"%4d       %11.2f\n",line_no,freq - rflrfp);
      }

/* output phases of the active lines */
    fprintf(datafile,
        "line    spectrum        phase\n");
    for (line_no = 1; line_no <= numlines; line_no++)
      for (spec_no = 1; spec_no <= numspec; spec_no++)
        if (dpoint[line_no] >= 0)
          fprintf(datafile,"%4d        %4d  %11.2f\n",
                 line_no,spec_no,phase[numspec * (line_no - 1) + spec_no]);
    fclose(datafile);
    chmod(filename,0666);
    return 0;
  }
  else
  { Werrprintf("cannot open file %s",filename);
    return 1;
  }
}

/*
 *
 *   adept  calculation
 *
 */
static void standard_constants(float cn[4][4])
{
   cn[0][0] = 1.0;   cn[0][1] =  0.0;   cn[0][2] =  0.0;  cn[0][3] =  0.0;
   cn[1][0] = 0.0;   cn[1][1] =  1.0;   cn[1][2] =  1.0;  cn[1][3] =  0.0;
   cn[2][0] = 1.0;   cn[2][1] =  0.0;   cn[2][2] =  0.0;  cn[2][3] = -1.0;
   cn[3][0] = 1.0;   cn[3][1] = -0.75;  cn[3][2] = -0.75; cn[3][3] =  1.0;
}
/*****************************************************************/
static int findconstants(int i1, int i2, double korr, int i3, int i4,
                         char v1, char v2, float *a, float *b, int numlines)
/*****************************************************************/
{ register double sxy,syz,szz,syy,sxz;
  register int i;

  sxy = 0.0; syz = 0.0; szz = 0.0; syy=0.0; sxz=0.0;
  for (i=0; i<numlines; i++)
    { if ((linetype[i]==v1) || (linetype[i]==v2))
        { sxy += (read_amp[4*i+i1] + korr*read_amp[4*i+i2]) * read_amp[4*i+i3];
          syz += read_amp[4*i+i3] * read_amp[4*i+i4];
          sxz += (read_amp[4*i+i1] + korr*read_amp[4*i+i2]) * read_amp[4*i+i4];
          szz += read_amp[4*i+i4] * read_amp[4*i+i4];
          syy += read_amp[4*i+i3] * read_amp[4*i+i3];
        }
    }
  if ((syy < 1e-5) || (fabs(szz * syy - syz * syz) < 1e-5))
    {
      return(0);
    }
  else
    { *b = (sxy * syz - sxz * syy) / (szz * syy - syz * syz);
      *a = - (sxy + *b * syz) / syy; 
      return(1);
    }
}

/*********************************/
static int adept_constants(int numlines, float cn[4][4])
/*********************************/
{ int i;
  char lt;
  float size1,size2,size1m,a,b;
  int ok;
  int coef_err;
  int d_ok = FALSE;
  int t_ok = FALSE;
  int q_ok = FALSE;

  for (i=0; i<numlines; i++)
    {
      if (read_amp[4*i] <= 0.0) /* intensity of each line in first spectrum */
        lt = 'U';	/* undefined */
      else
      {
          size1  = read_amp[4*i]   + read_amp[4*i+3];
          size1m = read_amp[4*i]   - read_amp[4*i+3];
          size2  = read_amp[4*i+1] + read_amp[4*i+2];
          /* size2 is average of peak in spectrum 2 and 3 */
          if ((size1m > size1) && (size1m > read_amp[4*i]) &&
              (size1m > 2.0 * size2))
             lt = 'T';	/* triplet */
          else if ((size2 >= 0.5 * size1) && (size1 >= 0.5 * size2))
             lt = 'D';	/* doublet */
          else if ((size1 >= 2.0 * size1m) && (size1 >= 2.0 * size2))
             lt = 'Q';	/* quartet */
          else
             lt = 'U';
      }
      d_ok = (d_ok || (lt == 'D'));
      t_ok = (t_ok || (lt == 'T'));
      q_ok = (q_ok || (lt == 'Q'));
      linetype[i] = lt;
    }
  standard_constants(cn);
  coef_err = FALSE;
  if (t_ok && q_ok)
  {
     ok = findconstants(1,2,1.0,0,3,'T','Q',&a,&b,numlines);
     coef_err = (!ok || (a > 5.0) || (b > 5.0));
     if (ok && !coef_err)
     {
         cn[1][0] = a;
         cn[1][3] = b;
     }
  }

  if (d_ok && q_ok)
  {
     ok = findconstants(0,3,-1.0,1,3,'D','Q',&a,&b,numlines);
     coef_err = (coef_err || !ok || (a > 5.0) || (b > 5.0));
     if (ok && !coef_err)
     {
        cn[2][1] = a;
        cn[2][3] = -1.0 + b;
     }
  }

  if (d_ok && t_ok)
  {
     ok = findconstants(0,1,-0.75,2,3,'D','T',&a,&b,numlines);
     coef_err = (coef_err || !ok || (a > 5.0) || (b > 5.0));
     if (ok && !coef_err)
     {
        cn[3][2] = a;
        cn[3][3] = b;
     }
  }
  if (coef_err)
     standard_constants(cn);
 
  return(coef_err);
}

/********************************************/
static int adept_printresult(int numlines, float cn[4][4], int plotflag, int coef_flag, int standard_coef)
/********************************************/
{ int i,sum[3],first,j,k,r,ytable;
  int printflag = TRUE;
  float y2,y3,y4;
  double sfrq;
  char ch,s[80],textpath[MAXPATHL];
  FILE	*textfile = NULL;

  if ( (r=P_getreal(PROCESSED,"sfrq",&sfrq,1)) )
    { P_err(r,"sfrq",":");   return 1; }
  strcpy(s,"ADEPT SPECTRUM ANALYSIS");
  if (standard_coef)
     strcat(s," with theoretical coefficients");
  if (standard_coef > 1)
     strcat(s," due to optimization error");
  if (plotflag)
    { setplotter();
      ytable = mnumypnts - 3*ycharpixels/2;
      amove(xcharpixels,ytable);
      dstring(s);
      ytable -= 2*ycharpixels;
      amove(xcharpixels,ytable);
      dstring("index       frequency        ppm      intensity");
      ytable -= ycharpixels;
      printflag = FALSE;
    }
  else
    { Wscrprintf("%s\n",s);
      Wscrprintf("\n");
      Wscrprintf("index       frequency      ppm       intensity\n");

      strcpy(textpath, curexpdir);
#ifdef UNIX
      strcat(textpath, "/dept.out");
#else
      strcat(textpath, "dept.out");
#endif
      textfile = fopen(textpath, "w");
      if (textfile == NULL)
      {
         Werrprintf("Unable to open dept.out file");
         printflag = FALSE;
      }

      if (printflag)
      {
         fprintf(textfile, "%s\n", s);
         fprintf(textfile, "\n");
         fprintf(textfile, "index       frequency      ppm       intensity\n");
      }
      ytable = 0;
    }
  for (i=0; i<numlines; i++)
    {
      y2 = cn[1][0] * read_amp[4*i  ] + cn[1][1] * read_amp[4*i+1] +
           cn[1][2] * read_amp[4*i+2] + cn[1][3] * read_amp[4*i+3];
      y3 = cn[2][0] * read_amp[4*i  ] + cn[2][1] * read_amp[4*i+1] +
           cn[2][2] * read_amp[4*i+2] + cn[2][3] * read_amp[4*i+3];
      y4 = cn[3][0] * read_amp[4*i  ] + cn[3][1] * read_amp[4*i+1] +
           cn[3][2] * read_amp[4*i+2] + cn[3][3] * read_amp[4*i+3];
      read_amp[4*i+1] = y2;
      read_amp[4*i+2] = y3;
      read_amp[4*i+3] = y4;
    }
  for (k=0; k<3; k++)
    sum[k] = 0;
  j = 0;
  for (i=0; i<numlines; i++)
    { first = 1;
      if (read_amp[4*i]>0)
        {
            for (k=1; k<4; k++)
               if (read_amp[4*i+k] > read_amp[4*i])
                  read_amp[4*i] = read_amp[4*i+k];
            for (k=0; k<3; k++)
            { if (read_amp[4*i+k+1]>0.25*read_amp[4*i])
                { if (k==0)
                    ch = 'D';
                  else if (k==1)
                    ch = 'T';
                  else
                    ch = 'Q';
                  sum[k]++;
                  j++;
                  if (first)
                    sprintf(s,"%4d %c %12.1f %12.3f %12.3f",
                      j,ch,read_freq[i],read_freq[i]/sfrq,read_amp[4*i+k+1]);
                  else
                    sprintf(s,"%4d %c                           %12.3f",
                      j,ch,read_amp[4*i+k+1]);
                  first = 0;
                  if (plotflag)
                    { if (ytable>ycharpixels)
                        { amove(xcharpixels,ytable);
                          dstring(s);
                          ytable -= ycharpixels;
                        }
                    }
                  else
		  {
                    Wscrprintf("%s\n",s);
		    if (printflag)
		       fprintf(textfile, "%s\n",s);
		  }
                }
              if ((plotflag)&&(ytable<=ycharpixels))
                { ytable -=ycharpixels;
                  amove(xcharpixels,ytable);
                  dstring("--- no room for other lines ---");
                }
            }
        }
    }
  sprintf(s,"Number of protonated carbons: %d",sum[0]+sum[1]+sum[2]);
  if (plotflag)
    { if (ytable>0)
        { amove(xcharpixels,ytable);
          dstring(s);
          ytable -= ycharpixels;
        }
    }
  else
  {
    Wscrprintf("\n%s\n",s);
    if (printflag)
       fprintf(textfile, "\n%s\n",s);
  }
  sprintf(s,"CH : %4d",sum[0]);
  if (plotflag)
    { if (ytable>0)
        { amove(xcharpixels,ytable);
          dstring(s);
          ytable -= ycharpixels;
        }
    }
  else
  {
    Wscrprintf("\n%s\n",s);
    if (printflag)
       fprintf(textfile, "\n%s\n",s);
  }
  sprintf(s,"CH2: %4d",sum[1]);
  if (plotflag)
    { if (ytable>0)
        { amove(xcharpixels,ytable);
          dstring(s);
          ytable -= ycharpixels;
        }
    }
  else
  {
    Wscrprintf("%s\n",s);
    if (printflag)
       fprintf(textfile, "%s\n",s);
  }
  sprintf(s,"CH3: %4d",sum[2]);
  if (plotflag)
    { if (ytable>0)
        { amove(xcharpixels,ytable);
          dstring(s);
          ytable -= ycharpixels;
        }
    }
  else
  {
    Wscrprintf("%s\n",s);
    if (printflag)
       fprintf(textfile, "%s\n",s);
  }
  if (coef_flag)
  {
    sprintf(s,"Spectral Coeffients");
    if (plotflag)
    {  if (ytable>0)
       {  amove(xcharpixels,ytable);
          dstring(s);
          ytable -= ycharpixels;
       }
    }
    else
    {
      Wscrprintf("\n%s\n",s);
      if (printflag)
         fprintf(textfile, "\n%s\n",s);
    }
    for (i=0; i<4; i++)
    {
       sprintf(s,"%12.3f  %12.3f  %12.3f  %12.3f",
               cn[i][0], cn[i][1], cn[i][2], cn[i][3]);
       if (plotflag)
       {  if (ytable>0)
          {  amove(xcharpixels,ytable);
             dstring(s);
             ytable -= ycharpixels;
          }
       }
       else
       {
         Wscrprintf("%s\n",s);
         if (printflag)
            fprintf(textfile, "%s\n",s);
       }
    }
  }
  if (plotflag)
    endgraphics();
  if (printflag)
     fclose(textfile);
  return 0;
}

/**************************************/
static int adept_combinespectra(register float cn[4][4])
/**************************************/
{ int r;
  register int i;
  register float *spec1,*spec2,*spec3,*spec4;
  register float y2,y3,y4;
  int j;

  D_allrelease();
  if (dataheaders(0,1))
    { releaseAllWithId("readfp");
       return 1;
    }
  if (datahead.nblocks!=4)
    { Werrprintf("Not four spectra measured as required by 'adept'");
      return 1;
    }
  removephasefile();
  for (i=1; i < 4; i++)
     for (j=0; j<4; j++)
        cn[i][j] = cn[i][j] / 2.0;
  if ( (r=D_getbuf(D_DATAFILE,datahead.nblocks,0,&c_block)) )
    { D_error(r); return ERROR; }
  spec1 = c_block.data;
  if ( (r=D_getbuf(D_DATAFILE,datahead.nblocks,1,&c_block)) )
    { D_error(r); return ERROR; }
  spec2 = c_block.data;
  if ( (r=D_getbuf(D_DATAFILE,datahead.nblocks,2,&c_block)) )
    { D_error(r); return ERROR; }
  spec3 = c_block.data;
  if ( (r=D_getbuf(D_DATAFILE,datahead.nblocks,3,&c_block)) )
    { D_error(r); return ERROR; }
  spec4 = c_block.data;
  i = datahead.np;
  while (i--)
    { y2 = cn[1][0] * *spec1 + cn[1][1] * *spec2 +
           cn[1][2] * *spec3 + cn[1][3] * *spec4;
      y3 = cn[2][0] * *spec1 + cn[2][1] * *spec2 +
           cn[2][2] * *spec3 + cn[2][3] * *spec4;
      y4 = cn[3][0] * *spec1 + cn[3][1] * *spec2 +
           cn[3][2] * *spec3 + cn[3][3] * *spec4;
      spec1++;
      *(spec2++) = y2;
      *(spec3++) = y3;
      *(spec4++) = y4;
    }
  if ( (r=D_release(D_DATAFILE,0)) )
    { D_error(r); return ERROR; }
  if ( (r=D_markupdated(D_DATAFILE,1)) )
    { D_error(r); return ERROR; }
  if ( (r=D_release(D_DATAFILE,1)) )
    { D_error(r); return ERROR; }
  if ( (r=D_markupdated(D_DATAFILE,2)) )
    { D_error(r); return ERROR; }
  if ( (r=D_release(D_DATAFILE,2)) )
    { D_error(r); return ERROR; }
  if ( (r=D_markupdated(D_DATAFILE,3)) )
    { D_error(r); return ERROR; }
  if ( (r=D_release(D_DATAFILE,3)) )
    { D_error(r); return ERROR; }
  return(COMPLETE);
}

/*----------------------------------------------------------------------------
|	argtest(argc,argv,argname)
|	test whether argname is one of the arguments passed
+---------------------------------------------------------------------------*/
static int argtest(int argc, char *argv[], char *argname)
{
  int found = 0;

  while ((--argc) && !found)
    found = (strcmp(*++argv,argname) == 0);
  return(found);
}
/**************/
int adept(int argc, char *argv[], int retc, char *retv[])
/**************/
{ int numlines,numspec,plotflag;
  float cn[4][4];
  int print_coef = FALSE;
  int standard = 0;

  (void) retc;
  (void) retv;
  plotflag = (argv[0][0]=='p');
  if ( !argtest(argc,argv,"noll"))
    { execString("nll fp\n");
    }
  if (argtest(argc,argv,"coef"))
     print_coef = TRUE;
  if (read_fp_table(&numlines,&numspec))
    { releaseAllWithId("readfp");
       return 1;
    }
  if (numspec!=4)
    { Werrprintf("Not four spectra measured as required by 'adept'");
      return 1;
    }
  if ((linetype=(char *) allocateWithId(sizeof(char) * numlines,
                                                         "readfp"))==0)
    { Werrprintf("cannot allocate buffer memory");
      return 1;
    }
  Wturnoff_buttons();
  if (!plotflag)
     { Wclear_graphics();
      Wshow_text();
      Wsettextdisplay("adept");
    }
  disp_status("ADEPT ");
  if ( (standard = argtest(argc,argv,"theory")) )
     standard_constants(cn);
  else if (adept_constants(numlines,cn))
  {
    standard += 2;
    standard_constants(cn);
  }
  if (adept_printresult(numlines,cn,plotflag,print_coef,standard))
    { releaseAllWithId("readfp");
       return 1;
    }
  if (adept_combinespectra(cn))
    { releaseAllWithId("readfp");
       return 1;
    }
  releaseAllWithId("readfp");
  appendvarlist("dummy");
  start_from_ft = 1;
  if (plotflag)
    execString("pl('all') pscale\n");
  disp_status("      ");
  return 0;
}

/**************************/
int analyze(int argc, char *argv[], int retc, char *retv[])
/**************************/
/* analyze allows to analyze experimental data points by fitting them	*/
/* to a specific function. Exponential curve fitting is presently only	*/
/* implemented, but other curve fitting programs can be supplied.	*/
/* analyze is called as follows:					*/
/* analyze('programname','xarray'<,'option'><,'option'>)		*/
/*   'programname'	name of the curve fitting program		*/
/*			('expfit' only implemented presently)		*/
/*   'xarray'		name of the variable array containing x-values	*/
/*   'option'		option(s) for curve fitting program		*/
/*			('T1','kinetics','incremental','list')		*/
/* The information from file 'fp.out' is used to construct y values	*/
/*									*/
/* analyze works as follows:						*/
/*  1. For t1, T2, kinetics, and regression,  a text file  		*/
/*     'analyze.inp' is constructed, which contains:			*/
/*	  <optional descriptive text line>				*/
/*	  <optional y-axis title - regression only>			*/
/*	  number of peaks   number of (x,y) pairs per peak		*/
/*	index#(1)    (except diffusion and cp_analysis)			*/
/*	  x y   (first peak,first pair)					*/
/*	  x y   (first peak,second pair)				*/
/*	  ......							*/
/*	index#(2)							*/
/*	  x y   (second peak, first pair)				*/
/*	  ......							*/
/*	Information from the file 'fp.out' and from the array 'xarray'	*/
/*      are used to construct this file.				*/
/*   2. Input for 'diffusion' and 'cp_analysis' is slightly different:	*/
/*	  List of <number> x-y data pairs				*/
/*	  <Descriptive text line>					*/
/*	  <X-values>  	<Y-values>	(2 strings without blanks)	*/
/*	  x y   (first peak,first pair)	(continues as above)		*/
/*   3. The program 'programname' is called with the following syntax:	*/
/*	  programname option(s) <analyze.inp >analyze.out 		*/
/*	'analyze.out' serves as input for 'expl'			*/

{ char *progname,*xname;
  char arrayname[12];
  int npairs,index,numlines,numspec,argnum,i,j,r;
  int poly = 0;
  int regression = FALSE;
  int kinetics   = FALSE;
  int cp_analysis= FALSE;
  int r_default  = FALSE;
  double *xarray;
  FILE *datafile;
  char  filename[MAXPATH];
  char s[256];
  vInfo  info;
  double offset;

  (void) retc;
  (void) retv;
  if (argc<3)
    { Werrprintf("usage - analyze('programname','xarray'<,'option')>");
      return ERROR;
    }

  argnum = 1;
  while (argnum < argc)
  {
     if (strcmp(argv[argnum],"regression")==0 ||
         strcmp(argv[argnum],"r")==0)
     {
	regression = TRUE;
        r_default = (argnum == 1) ? TRUE : FALSE;
     }
     else if (strcmp(argv[argnum],"cp_analysis")==0 ||
              strcmp(argv[argnum],"contact_time")==0 )
	cp_analysis= TRUE;
     else if (strcmp(argv[argnum],"kinetics")==0)
	kinetics   = TRUE;
     else if (strcmp(argv[argnum],"increment")==0)
	kinetics   = TRUE;
     argnum++;
  }
  disp_status("ANALYZE ");
  progname = argv[1];
  xname    = argv[2];
 
  if (argc>=4 && !regression)
  {
     poly = (strcmp(argv[3],"diffusion")==0 || strcmp(argv[3],"poly1")==0 ||
           strcmp(argv[3],"poly2")==0);
  }
  else if (argc>=3 && !regression)
  {
    if (strcmp(argv[2],"poly0") == 0 ||
        strcmp(argv[2],"poly1") == 0 ||
        strcmp(argv[2],"poly2") == 0 ||
        strcmp(argv[2],"poly3") == 0)
      regression = 1;
      r_default = 1;
  }
  strcpy(filename,curexpdir);
  strcat(filename,"/analyze.inp");
  if (!poly && ! regression)
    { 
      if (kinetics) 
      { 
	if (kgetparams(arrayname,&offset))
 	  return ERROR;
        xname = &(arrayname[0]);
      }

      if ( (r = P_getVarInfo(PROCESSED,xname,&info)) )
        { P_err(r,xname,":");   return ERROR; }
      npairs = info.size;
      if (npairs == 1 && strcmp(argv[3],"T2") == 0)
      {	strcpy(xname,"bt");
        r = P_getVarInfo(PROCESSED,xname,&info);
	if (!r)
           npairs = info.size;
      }
     if ((xarray=(double *) allocateWithId(sizeof(double)*npairs,"analyze"))==NULL)
        { Werrprintf("cannot allocate buffer memory");
          return 1;
        }
        for (index = 0; index < npairs; index++)
        { if ( (r=P_getreal(CURRENT,xname,&xarray[index],index+1)) )
            { P_err(r,xname,":"); 
              releaseAllWithId("analyze");
              return ERROR;
            }
        }
      if (read_fp_table(&numlines,&numspec))
        { releaseAllWithId("readfp");
          releaseAllWithId("analyze");
          return 1;
        }
      if (numspec!=npairs && (numspec < 4 || numspec >npairs))
	{
 	  if (numspec > npairs)
	   Werrprintf("Size of array '%s' does not agree with data in 'fp.out'",
              xname);
	  else
	   Werrprintf("At least 4 spectra(or points) needed for this analysis");
          releaseAllWithId("readfp");
          releaseAllWithId("analyze");
          return 1; 
	  
        }
    if ( (datafile=fopen(filename,"w+")) )
      { fprintf(datafile,"%12d %12d\n",numlines,numspec);
        for (i=0; i<numlines; i++)
          { 
	    fprintf(datafile,"%d\n",line_index[i]);
            for (j=0; j<numspec; j++)
              { fprintf(datafile,"%12g %12g\n",
		  xarray[spec_index[numspec*i+j] - 1],read_amp[numspec*i+j]);
              }
          }
        fclose(datafile);
      }
    else
      { Werrprintf("cannot open file %s",filename);
        releaseAllWithId("readfp");
        releaseAllWithId("analyze");
        return 1;
      }
  }  
      chmod(filename,0666);
      strcpy(filename,curexpdir);
#ifdef UNIX
      strcat(filename,"/analyze.out");
#else
      strcat(filename,"analyze.out");
#endif
      unlink(filename);
      strcpy(filename,curexpdir);
#ifdef UNIX
      strcat(filename,"/analyze.list");
#else
      strcat(filename,"analyze.list");
#endif
      unlink(filename);
      /* construct calling string with options */
      if (r_default)
        strcpy(s,"expfit");
      else
        strcpy(s,progname);
      if (regression)
        { strcat(s," regression");
	  if (r_default)
	  {  strcat(s," list ");
	     strcat(s,argv[2]);
	  }
        }
      else if (cp_analysis)
        { strcat(s," cp_analysis");
        }
      for (i=3; i<argc; i++)
        { strcat(s," ");
          strcat(s,argv[i]);
        }
      strcat(s," ");
      strcat(s,curexpdir);

/*      strcat(s," <analyze.inp >analyze.list"); */
      strcat(s," <");
      strcat(s,curexpdir);
#ifdef UNIX
      strcat(s,"/analyze.inp >");
#else
      strcat(s,"analyze.inp >");
#endif
      strcat(s,curexpdir);
#ifdef UNIX
      strcat(s,"/analyze.list");
#else
      strcat(s,"analyze.list");
#endif

      system(s);

      strcpy(filename,curexpdir);
#ifdef UNIX
      strcat(filename,"/analyze.inp");
#else
      strcat(filename,"analyze.inp");
#endif
      if ( (datafile=fopen(filename,"r")) )
        { fclose(datafile);
          /* if (!poly) unlink(filename); */
        }
      else
        { Werrprintf("problem running program '%s' to analyze data",progname);
          releaseAllWithId("readfp");
          releaseAllWithId("analyze");
          /* if (!poly) unlink(filename);  */
          return 1;
        }
    
  releaseAllWithId("analyze");
  releaseAllWithId("readfp");
  appendvarlist("dummy");
  disp_status("        ");
  return 0;
}

int kgetparams(char arrayname[], double *offset)
 /* --------- get x-values for kinietics */
{
  double val;
  int r;

  if ( (r = P_getreal(PROCESSED,"d1",offset,1)) )
  	{ P_err(r,"d1",":");   return ERROR; }
  if ( (r = P_getreal(PROCESSED,"d2",&val,1)) )
  	{ P_err(r,"d2",":");   return ERROR; }
  *offset += val;
  if ( (r = P_getreal(PROCESSED,"at",&val,1)) )
  	{ P_err(r,"at",":");   return ERROR; }
  *offset += val;
  if ( (r = P_getreal(PROCESSED,"nt",&val,1)) )
  	{ P_err(r,"nt",":");   return ERROR; }
  *offset *= val;
  if ( (r = P_getstring(PROCESSED,"array",arrayname,1,11)) )
  	{ P_err(r,"array",":");   return ERROR; }
  return (COMPLETE);
}

#define MAXCURVES 8
#define MAXSETS   128
#define MAXPOINTS 32768

/**********************************/
static void point_symbol(int x, int y, int index, int check, int usesymbol)
/**********************************/
{ int dx,dy;
  if (check)
    if (y > dnpnt2+dfpnt2 ||  y <dfpnt2 ||
        x > dnpnt +dfpnt  ||  x <dfpnt ) return;

  dx = xcharpixels/2;
  dy = ycharpixels/3;
  if (usesymbol == 2)
  {
    dx = xcharpixels/4;
    dy = ycharpixels/6;
  }
  switch (index % 8)
    { case 0:	amove(x,y-dy); rdraw(0,2*dy);
                amove(x-dx,y); rdraw(2*dx,0);
                break;
      case 1:   amove(x-dx,y-dy); rdraw(2*dx,0);
                rdraw(0,2*dy); rdraw(-2*dx,0); rdraw(0,-2*dy);
                break;
      case 2:   amove(x,y-dy); rdraw(dx,dy);
                rdraw(-dx,dy); rdraw(-dx,-dy); rdraw(dx,-dy);
                break;
      case 3:   amove(x-dx,y-dy); rdraw(2*dx,0);
                rdraw(-dx,2*dy); rdraw(-dx,-2*dy);
                break;
      case 4:   amove(x,y-dy); rdraw(dx,2*dy);
                rdraw(-2*dx,0); rdraw(dx,-2*dy);
                break;
      case 5:   amove(x-dx,y-dy); rdraw(2*dx,dy);
		rdraw(-2*dx,dy); rdraw(0,-2*dy);
                break;
      case 6:	amove(x-dx,y);	rdraw(2*dx,-dy);
		rdraw(0,2*dy);	rdraw(-2*dx,-dy);
		break;
      case 7:	amove(x-dx,y-dy); rdraw(2*dx,2*dy);
		amove(x-dx,y+dy); rdraw(2*dx,-2*dy);
                break;
    }
}

/**********************************/
static void point_link_draw(int x, int y, int dx, int dy)
/**********************************/
{ 
    int chk1=0, chk2=0;

    if (y > dnpnt2+dfpnt2 ||  y <dfpnt2 ||
        x > dnpnt +dfpnt  ||  x <dfpnt ) chk1 = 1;
    if ((y+dy) > dnpnt2+dfpnt2 ||  (y+dy) <dfpnt2 ||
        (x+dx) > dnpnt +dfpnt  ||  (x+dx) <dfpnt ) chk2 = 1;
    if ((chk1 == 0) && (chk2 == 0)) 	/* && -> || fails for LaserJet, ok for PS */
    {
	amove(x, y);
	rdraw(dx, dy);
    }
}

#define T1T2      0 
#define KINI      1 
#define KIND      2 
#define DIFFUSION 3 
#define POLY1     5 
#define POLY2     6 
#define EXPONENTIAL 7
#define CP_ANALYSIS  8   /* Contact Time */
#define POLY3     9 
#define POLY0    10 

float D1,C0,C1,C2;

/******************************/
double msubt(double t, double a0, double a1, double a2, double a3, int exptype)
/******************************/
{ 
  float uu;
  if (exptype == CP_ANALYSIS)
  {
    return ( ((a3 - (a3-a0)*exp(-t/a1)) * exp(-t/a2)) + a0);
  }
  else if (exptype == DIFFUSION)
  { uu = C0 + C1*t + C2*t*t;
    return ( a0*exp(-D1*uu) + a2*exp(-a1*D1*uu));
  }
  else if (exptype == POLY0)
    return (a0);
  else if (exptype == POLY1)
    return (a0 + a1*t);
  else if (exptype == POLY2)
    return (a0 + a1*t +a2*t*t);
  else if (exptype == POLY3)
    return (a0 + a1*t +a2*t*t + a3*t*t*t);
  else if (exptype==KIND || exptype == EXPONENTIAL)
    return (a0*exp(-t/a1)+a2);
  else if (exptype==KINI)
    return (-a0*exp(-t/a1)+a2+a0);
  else
    return (a0-a2)*exp(-t/a1)+a2;
}

/*************/
static int logerr(float u)
/*************/
{ if (fabs(u) < 1e-10)
     {Werrprintf(
	"expl: Data point coordinate too small for calculating logarithm.");
      return(1);
     }
 else if (u < 0.0)
     {Werrprintf(
	"expl: Cannot calculate logarithm of negative number.");
      return(1);
     }
 else return(0);
}

/*************/
static void fileformat()
/*************/
{
if (!Wissun()) Wclear_graphics();
Wscrprintf("\nFile Format for 'regression.inp':\n");
Wscrprintf("		<Optional Text Line>\n");
Wscrprintf("		<Y Title: Optional Text Line>\n");
Wscrprintf("		numsets         numpoints in set\n");
Wscrprintf("		<NEXT>    Optional set divider\n");
Wscrprintf("		x           y       (first point, first set)\n");
Wscrprintf("		x           y       (second point, first set)\n");
Wscrprintf("		--------------\n");
Wscrprintf("		<NEXT>    Optional set divider\n");
Wscrprintf("		x           y       (first point, next set)\n");
Wscrprintf("		--------------\n");
Wscrprintf("If optional set divider,'NEXT' is used, set 'numsets' and \n");
Wscrprintf("		'numpoints in set' to 0.\n");
}

/**
 * From the comma separated list in "buf", read the "field"th number and
 * return it in "value".
 * Returns 1 on success, 0 on failure.
 */
static int readField(char *buf, int field, double *pvalue)
{
    int i;
    char *pfield;

    for (i = 1, pfield = buf; i < field; i++, pfield++) {
        pfield = strchr(pfield, ',');
        if (pfield == NULL) {
            break;
        }
    }
    if (pfield == NULL || sscanf(pfield, "%lf", pvalue) < 1) {
        return 0;
    } else {
        return 1;
    }
}

/*  drawxy currently draws data from LC using the lcdata file format   */
/*  The first argument is the file anme of the lcdata file.  The       */
/*  second argument is the name of the parameter which holds details   */
/*  on how and where to plot the data.  Two optional arguments control */
/*  clearing the area where the plot will be drawn and the color of the*/
/*  plot.  Plotxy plots instead of drawing to the screen.              */
/*  The information parameter has 12 values specifying:	               */
/*  sc   wc   sc2   wc2	 determine the area to draw the data           */
/*  xmin xmax xslope xintecept set range of X values to draw and a     */
/*     linear transformation in the form val = mx + b                  */
/*     The transform is applied before the ranges are checked.         */
/*  yslope yintercept  apply a linear transform to the Y data in the   */
/*     form val = my + b                                               */
/*  angle is either 0 or 90 to specify the X axis either horizontal or */
/*     or vertical, respectively                                       */
/*  index specifies which data element to use as the Y coordinate      */
/*     from the lcdata file.  The X coordinate is hard coded as element*/
/*     1.  The Y coordinate will generally be 2, 3, or 4               */

int drawxy(int argc, char *argv[], int retc, char *retv[])
{
   int  noClearFlag = 0;
   FILE *datafile;
   double xminVal, xmaxVal, xSlope, xIntercept;
   double ySlope, yIntercept;
   double xy_sc, xy_wc, xy_sc2, xy_wc2;
   double xmultiplier;
   double val;
#ifdef XXX
   int  orient;
#endif
   int  angle;
   int  ydim;
   int  done;
   int  ret;
   int  lastx, lasty, dolast;
   int  xy_dfpnt, xy_dnpnt, xy_dfpnt2, xy_dnpnt2;
   int  dfpt, dfpt2;
   int newx, newy;
   double var1, var2;
   int  curp;
   char buf[1000];
   int firstline;

   (void) retc;
   (void) retv;
   if (argc < 3)
   {
      Werrprintf("Usage: %s('datapath','infopar' <,'noclear'><,color>)",argv[0]);
   }
   if (argv[0][0]=='p')
      setplotter();
   else
      setdisplay();
   if ( (argc >= 4) && (strcmp(argv[3],"noclear") == 0) )
   {
      noClearFlag = 1;
   }
   curp = YELLOW;
   if (argc >= 4)
   {
      int col = 0;
      int found = 0;
      int argnum = 3;

      while ((argc>argnum) && (!found))
         found = colorindex(argv[argnum++],&col);
      if (found)
      {
         curp = col;
      }
   }
   if (P_getreal(CURRENT,argv[2],&xy_sc,1))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xy_wc,2))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xy_sc2,3))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xy_wc2,4))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xminVal,5))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xmaxVal,6))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xSlope,7))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&xIntercept,8))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&ySlope,9))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&yIntercept,10))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   if (P_getreal(CURRENT,argv[2],&val,11))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   angle = (int) val+0.1;
   if (P_getreal(CURRENT,argv[2],&val,12))
   {
      Werrprintf("%s: info parameter %s does not exist",argv[0],argv[2]);
      ABORT;
   }
   ydim = (int) (val+0.1);
#ifdef XXX
   /* This argument was intended to select drawing from left to right or
    * right to left.  Currently not implemented.
    */
   if (P_getreal(CURRENT,argv[2],&val,13))
   {
      orient = 1;
   }
   else
      orient = (int) val+0.1;
#endif

/*
fprintf(stderr,"xy_sc= %g xy_wc= %g xy_sc2= %g xy_wc2= %g\n", xy_sc, xy_wc, xy_sc2, xy_wc2);
fprintf(stderr,"xminVal= %g xmaxVal= %g\n", xminVal, xmaxVal);
fprintf(stderr,"xSlope= %g xIntercept= %g\n", xSlope, xIntercept);
fprintf(stderr,"ySlope= %g yIntercept= %g\n", ySlope, yIntercept);
fprintf(stderr,"angle= %d orient= %d\n", angle, orient);
fprintf(stderr,"ydim= %d\n", ydim);
 */
   if ((datafile=fopen(argv[1],"r"))==0)
   {
      Werrprintf("%s: cannot open file %s",argv[0], argv[1]);
      ABORT;
   }

   checkreal(&xy_wc,WCMIN,wcmax);   
   checkreal(&xy_sc,0.0,wcmax-xy_wc);	       
   checkreal(&xy_wc2,WCMIN,wc2max);
   checkreal(&xy_sc2,0.0,wc2max-xy_wc2);	    
   xy_dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-xy_sc-xy_wc)/wcmax);
   xy_dnpnt  = (int)((double)(mnumxpnts-right_edge)*xy_wc/wcmax);
   xy_dfpnt2 = (int)((double)(mnumypnts-ymin)*xy_sc2/wc2max)+ymin;
   xy_dnpnt2 = (int)((double)(mnumypnts-ymin)*xy_wc2/wc2max);
/*
fprintf(stderr,"xy_dfpnt= %d xy_dnpnt= %d xy_dfpnt2= %d xy_dnpnt2= %d\n",
        xy_dfpnt,xy_dnpnt,xy_dfpnt2,xy_dnpnt2);
fprintf(stderr,"mnumxpnts= %d right_edge= %d mnumypnts= %d ymin=%d\n",
        mnumxpnts,right_edge,mnumypnts,ymin);
*/
 
   if (xy_dfpnt < 1)
     xy_dfpnt = 1;
   if ((xy_dfpnt + xy_dnpnt) >= (mnumxpnts - right_edge - 2))
     xy_dnpnt = mnumxpnts - right_edge - 2 - xy_dfpnt;
   Wgmode();
   if (!noClearFlag)
   {
      color(BACK);
      amove(xy_dfpnt,xy_dfpnt2);
      box(xy_dnpnt+1, xy_dnpnt2+1);
   }
   color(curp);
   done = 0;
   while (!done)
   {
      char line[MAXSTR];

      ret = fscanf(datafile,"%[^\n]\n", line);
/*
fprintf(stderr,"ret= %d line= %s\n",ret,line);
 */
      if ((ret == 0) || (ret == EOF))
      {
         fclose(datafile);
         Werrprintf("%s: format error in file %s",argv[0],argv[1]);
         ABORT;
      }
      if (strcmp(line,"; data:") == 0)
         done = 1;
   }

/* 
fprintf(stderr,"move= %d, %d\n", xy_dfpnt, xy_dfpnt2);
fprintf(stderr,"xy_dnpnt= %d xy_dnpnt2 %d\n", xy_dnpnt, xy_dnpnt2);
 */
   if (angle == 90)
   {
     amove(xy_dfpnt+xy_dnpnt,xy_dfpnt2);
     xmultiplier = xy_dnpnt2 / (xmaxVal-xminVal);
     dfpt = xy_dfpnt2;
     dfpt2 = xy_dfpnt + xy_dnpnt;
     ySlope = -ySlope;
     yIntercept = -yIntercept;
   }
   else
   {
     amove(xy_dfpnt,xy_dfpnt2);
     xmultiplier = xy_dnpnt / (xmaxVal-xminVal);
     dfpt = xy_dfpnt;
     dfpt2 = xy_dfpnt2;
   }
/*
fprintf(stderr,"xminVal= %g xmult= %g \n",xminVal,xmultiplier);
 */

   /* Go through data until first point we want to show (t >= xminVal). */
   firstline = 1;
   while (fgets(buf, sizeof(buf), datafile) != NULL) {
       if (firstline) {
           firstline = 0;
           if (strchr(buf,'.') == NULL) {
               continue;        /* This line is not data -- skip it */
           }
       }
       if (sscanf(buf, "%lf", &var1) < 1) {
           break;               /* No data in range */
       }
       var1 *= xSlope;
       var1 += xIntercept;
       if (var1 >= xminVal) {
           newx = dfpt + (int)((var1-xminVal)*xmultiplier);
            /* Read the ydim'th field into var2 */
           if (readField(buf, ydim, &var2) == 0) {
               fprintf(stderr,"Bad data line in \"%s\": %s\n", argv[1], buf);
               break;           /* Bad data file */
           }
           var2 *= ySlope;
           var2 += yIntercept;
           newy = dfpt2 + (int)(var2);
           if (angle)
               amove(newy, newx);
           else
               amove(newx, newy);
           break;               /* That was our first data point */
       }
   }

   /* Plot all the relevant data. */
   lastx = lasty = dolast = 0;
   while (fgets(buf, sizeof(buf), datafile) != NULL) {
       if (sscanf(buf, "%lf", &var1) < 1) {
           break;               /* No more data */
       }
       var1 *= xSlope;
       var1 += xIntercept;
       if (var1 <= xmaxVal) {
           newx = dfpt + (int)((var1-xminVal)*xmultiplier);
           /* Read the ydim'th field into var2 */
           if (readField(buf, ydim, &var2) == 0) {
               fprintf(stderr,"Bad data line in \"%s\": %s\n", argv[1], buf);
               break;           /* Bad data file */
           }
           var2 *= ySlope;
           var2 += yIntercept;
           newy = dfpt2 + (int)(var2);


/*
fprintf(stderr,"xc= %d  yc= %d\n",xy_dfpnt + (int)((var1-xminVal)*xmultiplier),
             xy_dfpnt2 + (int)(var2) );
 */
         if (newy == lasty)
         {
            dolast = 1;
            lastx = newx;
         }
         else if ((newx != lastx) || (newy != lasty))
         {
            if (dolast)
            {
/*
fprintf(stderr,"adraw last %d %d\n", lastx, lasty);
 */
               if (angle)
                  adraw( lasty, lastx);
               else
                  adraw( lastx, lasty);
               dolast = 0;
            }
/*
fprintf(stderr,"adraw2 %d %d\n", newx, newy);
 */
            if (angle)
               adraw( newy, newx);
            else
               adraw( newx, newy);
            lastx = newx;
            lasty = newy;
         }
      }
      else
      {
         break;
      }
   }
   if (dolast)
   {
/*
fprintf(stderr,"adraw2 last %d %d\n", lastx, lasty);
 */
      if (angle)
         adraw( lasty, lastx);
      else
         adraw( lastx, lasty);
   }
   fclose(datafile);
   RETURN;
}


/*************/
/*  expl is a program to plot experimental data					*/
/*  expl stores scale and box parameters in expl.out 			*/
/*  format:	sc   wc   sc2   wc2					*/
/*        	xmin xmax "linear/square/log"				*/
/*        	ymin ymax "linear/square/log"				*/
/*		FLAG xscaleflag yscaleflag				*/
/*		setnum							*/
/*  keywords:   'linear', 'square', 'log' control scales, x-axis first  */
/*	 	'nocurve' data points only are plotted			*/
/*	 	'link' data points are connected, no theoretical curve  */
/*		'nosymbol' no data points are plotted, only curve/link 	*/
/*		'tinysymbol' use tiny data point symbols		*/
/*		'oldbox' uses input from expl.out to plot first data    */
/*		 	in analyze.out on existing plot   		*/ 
/*		'regression' plots data from 'regression.inp' (points   */
/*		only) and makes output file 'analyze.inp' for regres-   */
/*		sion.  							*/

/*	format of input file 'regression.inp':				*/

/*		<Optional Text Line>					*/
/*		<Y Title: Optional Text Line>				*/
/*		numsets		numpoints in set			*/
/*		<NEXT		numpoints in set>			*/
/*		x-value(1)  	y-value(1)				*/
/*		x-value(2)  	y-value(2)				*/
/*		..........  	..........				*/
/*		x-value(n)  	y-value(n)				*/
/*  	When 'rinput' which allows data sets of variable size is used,
	then line starting with 'NEXT' is inserted at the beginning
	of each data set, and the size of the largest data set
	follows numsets.						*/

/*	format of input file for expl, 'analyze.out':			*/
		
/*		exp n <regression>	(n=1..9 is curve type,
					use n=4 with key word 'link' with no 
					theoretical curve. "regression" if
					present indicates output from
					regression)
		<D1 C0 C1 C2>		(floating point constants. Used if and
					only if curve type=3, diffusion)
		n1  np	<x scale type>  <y scale type>
					(n1 is number of curves, np is number
					of data point pairs in set. Scale
					types used in regression mode are
					'linear', 'square', 'log')
		text line		Use "No Title" when title not desired.
		<text line>	 	Present if and only if output 
					from regression.
		<text line>		Y-title present if and only if output
					from regression.
		region# a0 a1 a2 <a3>   (a0,a1,a2,<a3> from expfit)
		x-value(1)  y-value(1)
		..........  ..........				*********/

int expl_cmd(int argc, char *argv[], int retc, char *retv[])
{ int plotflag,argnum,i,j,k,l,numlines,numspec,foundany;
  float xmin,xmax,ymin,ymax,xmultiplier,ymultiplier,xx,yy;
  float newxmin, temp1;
  float *x[MAXCURVES];
  float *xxx[MAXCURVES];
  float *y[MAXCURVES];
  float *yyy[MAXCURVES];
  float a0[MAXCURVES],a1[MAXCURVES],a2[MAXCURVES],a3[MAXCURVES];
  int index[MAXSETS];
  int line_index[MAXSETS];
  int spec_count[MAXSETS];
  int charpix;
  int exptype   = 0;
  int axis1     = LINEAR; 
  int axis2     = LINEAR;
  int lbl1	= LINEAR;
  int lbl2	= LINEAR;
  int cntr      = 0;
  int link      = 0;
  int usesymbol = 1;
  int sss       = 0;
  int ypos      = 0;
  int inx	= 0;
  int cnt       = 0;
  int regression= FALSE;
  int symbolnum = 0;
  int clearScreen = TRUE;
  int oldbox    = FALSE;
  int oldscale  = FALSE;
  int plotcurve = TRUE;
  int rflag     = FALSE;
  int realargflag = FALSE;
  int xscaleflag= TRUE;
  int yscaleflag= TRUE;
  int NEXTflag  = FALSE;
  int rfileresult = 0;
  int yflag     = 0; /* set to 1 if x-axis is changed, 2 if y-axis */
  int lcount    = 0; /* number of data sets, line count */
  int lcount2   = 0; /* number of data sets, line count */
  int infoprint = 0;
  int setnum    = 0;
  int oldnumspec;    /* numper of points in a data set */
  int maxnumspec;    /* numper of points in largest data set */
  int foundfirst = 0;
  int newindex[MAXCURVES];
  /* int xminsave, xmaxsave, yminsave, ymaxsave; */
  FILE *boxfile;
  FILE *datafile;
		/* first used for expl.out then with 'filename' */
  char filename[MAXPATHL];
		/* if regression 'regression.out'else 'analyze.out' */
  char s[128];
  char s0[128];
  char sy[128];
  char xaxis[16], yaxis[16];
  char xaxistmp[16], yaxistmp[16];
  char flag[16];
  char *gdisplay;
  char stemp[20],graphcmd[20];
  char c;

  (void) retc;
  (void) retv;
  strcpy(xaxis,"linear");
  strcpy(yaxis,"linear");
  if (argv[0][0]=='p')
    plotflag = 2;
  else
    plotflag = 1;

  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/analyze.out");
#else
  strcat(filename,"analyze.out");
#endif
  for (i=0; i<MAXSETS; i++) line_index[i] = 0;
  for (i=0; i<MAXSETS; i++) spec_count[i] = 0;
  for (i=0; i<MAXCURVES; i++) line_index[i] = i + 1;
  for (i=0; i<MAXCURVES; i++) newindex[i] = line_index[i];
  if (argc<2)
    for (i=0; i<MAXCURVES; i++) index[i] = i;
  

  /**** *****  Part 1. Read Input Arguments  ***** ****/
  if (argc>=2)
    { 
      for (i=0; i<MAXSETS; i++) index[i] = -1;
      argnum = 1;
      while ((argnum<argc) && (argnum<MAXSETS+1+cntr))
        { if (isReal(argv[argnum]))
            { index[argnum-1-cntr] = (int) stringReal(argv[argnum]) - 1;
 	      realargflag = TRUE;
	      lcount++;
            }
          else if (strcmp(argv[argnum],"r")==0 ||
                   strcmp(argv[argnum],"regression")==0)
    	    {   regression = TRUE;
  		strcpy(filename,curexpdir);
#ifdef UNIX
  		strcat(filename,"/regression.inp");
#else
  		strcat(filename,"regression.inp");
#endif
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"yaxis")==0)
		yflag = 1;  
          else if (strcmp(argv[argnum],"square")==0 ||
                   strcmp(argv[argnum],"log")==0 || 
                   strcmp(argv[argnum],"ln")==0  ||
                   strcmp(argv[argnum],"linear")==0)
    	    {   if (!yflag) 
		{ 
		  strcpy(xaxis,argv[argnum]);
		  lbl1 = axis1 = setlbl(xaxis);
 		}
		else 
		{ 
		  strcpy(yaxis,argv[argnum]);
		  lbl2 = axis2 = setlbl(yaxis);
		}
		yflag++;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"link")==0)
	    {   link = 1;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"oldbox")==0)
	    {   oldbox = 1;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"oldscale")==0)
	    {   oldscale = 1;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"noclear")==0)
	    {   clearScreen = 0;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"nocurve")==0)
	    {   plotcurve = 0;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"nosymbol")==0)
	    {   if (usesymbol == 1)
		{
		usesymbol = 0;
	        cntr++;
		}
     	    }
          else if (strcmp(argv[argnum],"tinysymbol")==0)
	    {   usesymbol = 2;
	        cntr++;
     	    }
          else if (strcmp(argv[argnum],"file")==0)
 	    {   argnum++;
		if (argnum>=argc)
		  { Werrprintf("filename required following 'file'");
		    return 1;
		  }
  		strcpy(filename,argv[argnum]);
	        cntr += 2;
     	    }
	  else
            { Werrprintf("usage - <p>expl<(index,index,index,..)>");
	      Wscrprintf("argv[argnum] = %s\n",argv[argnum]);
              return 1;
            }
          argnum++;
        }
	if (regression) plotcurve = FALSE;
	if ((usesymbol == 0) && (plotcurve == 0)) link = 1;
	strcpy(xaxistmp,xaxis); strcpy(yaxistmp,yaxis);
    }
  if (index[0] == -1)
    {
        if (oldbox)
          {  Werrprintf("Usage: expl('oldbox',curve#) curve#:1..%d",MAXCURVES);
	     return 1;
	  }
 	for (i=0; i<MAXCURVES; i++) index[i] = i;
    }
  if (regression) for (i=0; i<MAXCURVES; i++) line_index[i] = index[i] + 1;
  /* set limits for box when wc or wc2 too small, sc+wc or sc2+wc2 too large */
  checkscwc("sc","wc","wcmax",0.1); 
  checkscwc("sc2","wc2","wc2max",0.02); 


  /**** *****  Part 2. Read Input File Headers   ***** ****/
  if (oldbox)
    {
      j=0;
      while (index[j] != -1 && j <= MAXSETS) j++;
      symbolnum = index[j-1];
      index[0] = 0;  /* data for one peak in analyze.out */
    }
  if ((datafile=fopen(filename,"r"))==0) /* 'regression.inp' or 'analyze.out' */
    { Werrprintf("cannot open file %s, no analysis done",filename);
      if (regression) 
          fileformat();
      else
      {
#ifdef UNIX
      strcpy(s,"cat ");
#else
      strcpy(s,"type ");
#endif
      strcat(s,curexpdir);
#ifdef UNIX
      strcat(s,"/analyze.list");
#else
      strcat(s,"analyze.list");
#endif
      system(s);
      }
      return 1;
    }
  l = 1;
  if (!regression && fscanf(datafile,"%s %d",s,&exptype)!=2)
    { syntaxerror(l,filename);
      fclose(datafile);
      return 1;
    }
  if (exptype==DIFFUSION) fscanf(datafile,"%g  %g  %g  %g\n",&D1,&C0,&C1,&C2);
  if ((exptype % 10 == 4) && (plotcurve == 1)) link=1;
  l=2;
  if (( !regression && strcmp(s,"exp")!=0) 
	|| (exptype<0) || (exptype % 10 > POLY3))
    { Werrprintf("curve type not implemented");
      fclose(datafile); 
      return 1; 
    }

  s[0]='\0';
  strcpy(sy,"No Title");

  if (regression)
  {
    int ixtmp;
    l = 0;
    if (readhead(datafile,s0,sy,filename,&ixtmp,&ixtmp,&l))
    {
      ABORT;
    }
    rfileresult = readfile(datafile,&numlines,&maxnumspec,&spec_count[0]);
    /* determines numlines maxnumspec, speccount */
    fclose(datafile);
    if (rfileresult == 1)
    {
      fileformat();
      ABORT;
    }
    l = 0;
    datafile=fopen(filename,"r");
    if (rfileresult == 2)
      readhead(datafile,s0,sy,filename,&numlines,&maxnumspec,&l);
    else	
      readhead(datafile,s0,sy,filename,&ixtmp,&ixtmp,&l);
    numspec = maxnumspec;
  }
  else if (exptype == DIFFUSION)
  { if (fscanf(datafile,"%d %d\n",&numlines,&numspec)!=2)
    {   syntaxerror(l,filename);
        fclose(datafile);
        return 1;
    }
  }
  else  /* look for possible 'regression' following exptype in analyze.out */
    if (fscanf(datafile,"%d",&numlines))
  {    fscanf(datafile,"%d\n",&numspec);
  }
  else /* 'regression' follows exptype in line 1 of 'analyze.out' */
  {
       char sxx[40];
       fscanf(datafile,"%s\n",sxx);
       if (strcmp(sxx,"regression") != 0 ||
           fscanf(datafile,"%d %d %s %s\n",&numlines,&numspec,xaxis,yaxis)!=4)
       {
	 l++;
         syntaxerror(l,filename);
         fclose(datafile);
         return 1;
       }
       rflag = TRUE;  /* rflag set */
  }
  gdisplay = newString(Wgetgraphicsdisplay(graphcmd,20));
  /* Wscrprintf("gdisplay = %s\n",gdisplay); */
/*  if (oldbox || regression)  Removed 6-21-90 */
  {
      char  box_filename[MAXPATHL];

      strcpy(box_filename,curexpdir);
#ifdef UNIX
      strcat(box_filename,"/expl.out");
#else
      strcat(box_filename,"expl.out");
#endif
      if ( (boxfile=fopen(box_filename,"r")) )   /** worry about axis labeling if one set and not the other **/
      {					     /** don't worry about regression !!! */
        fscanf(boxfile,"%lf   %lf   %lf   %lf\n",&sc,&wc,&sc2,&wc2);
        fscanf(boxfile,"%f   %f   %s\n",&xmin,&xmax,xaxis);
        if (yflag >= 2) axis1 = setlbl(xaxis);

        fscanf(boxfile,"%f   %f   %s\n",&ymin,&ymax,yaxis);
	if (fscanf(boxfile,"%s %d %d\n",flag,&xscaleflag,&yscaleflag) == 3)
	{
	    if (oldscale)
            {  xscaleflag = FALSE; 
               yscaleflag = FALSE; 
	    }
	    fscanf(boxfile,"%d",&setnum);
    	    if (strcmp(gdisplay,"expl") == 0)
	    {
	       xscaleflag %= 2;
	       yscaleflag %= 2;
	    }
        }
        if(yflag < 2) axis2 =setlbl(yaxis);
        fclose(boxfile);
      }
      else if (oldbox)
	{ Werrprintf("Unable to open %s",box_filename);
	  return ERROR;
 	}
  }
  if (regression || rflag)
  {
     if (regression)
     {
         setaxis(axis1,xaxis);
         setaxis(axis2,yaxis);
	 lbl1 = axis1;
	 lbl2 = axis2;
     }
  }
  /* Wscrprintf(
	"rflag=%d, reg=%d, lbl1=%d, lbl2=%d, yflag=%d, xaxis=%s, yaxis=%s\n",
	rflag,regression,lbl1,lbl2,yflag,xaxis,yaxis);
  */
  l++; /* l >= 3 */
  cntr = 0;
  if (!regression)
  {
    while  ((c = getc(datafile)) != '\n'  && cntr<128)
    { s0[cntr]=c;
      cntr++;
    }
    s0[cntr]='\0';
  }
  if (rflag) 
  {   fscanf(datafile,"%[^\n]\n", s0);
      fscanf(datafile,"%[^\n]\n", sy);
  }

  /**** *****  Part 2b Set Limits  ***** ****/
  infoprint = 0;
  if (lcount > numlines) lcount = numlines;
  if (lcount == 0) lcount = numlines;
  if (lcount > MAXCURVES)
  {   lcount = MAXCURVES;
      infoprint++;
  }
  oldnumspec = numspec;
  if (numspec> MAXPOINTS) 
  {   numspec = MAXPOINTS;
      infoprint++;
  }
  maxnumspec = numspec;
  if (maxnumspec == 0)
  {
      if (regression)
          Werrprintf("no data points in 'regression.inp' ");
      else
          Werrprintf("no data points in input file");
      fclose(datafile);
      return(1);
  }
  if (lcount*numspec > MAXPOINTS) 
  {   lcount = MAXPOINTS / numspec;
      infoprint++;
  }
  if (infoprint)
  {   Winfoprintf("Number of curves=%d, number of points per curve=%d",
		lcount,numspec);
  }
  lcount2 = 0;
  for (i=0; i<MAXSETS; i++)
  {   if (index[i] != -1 && lcount2 < lcount) lcount2++;
	    else index[i] = -1;
  }

  /**** *****  Part 3. Read Input Data.  ***** ****/
  if (exptype>10) exptype = exptype % 10;
  if (xscaleflag)
    { xmin = 1e32;
      xmax = -1e32;
    }
  if (yscaleflag)
    { ymin = 1e32;
      ymax = -1e32;
    }
  foundany = 0;
  cnt = 0;
  /* changed i<lcount to <numlines in next line (MER 3-29-93) */
  for (i=0; i<numlines; i++)	    /* i is counter for data sets */
    { if (i) fscanf(datafile,"\n");
      l++;
      if (regression || rflag)
      {
        foundfirst = 1;
        if (fscanf(datafile,"%f",&temp1) != 1)
	  { if (regression)
              fscanf(datafile,"%s \n",stemp);
 	    else
              fscanf(datafile,"%s %d\n",stemp,&spec_count[i]);
	    fscanf(datafile,"%f",&temp1);
	    NEXTflag = TRUE;
	    oldnumspec = spec_count[i];
	  }
      }
      if (i == 0)
      {
      for (j=0; j<lcount; j++)
         { x[j]   = (float *) allocateWithId(sizeof(float)*maxnumspec,"expl");
           xxx[j] = (float *) allocateWithId(sizeof(float)*maxnumspec,"expl");
           y[j]   = (float *) allocateWithId(sizeof(float)*maxnumspec,"expl");
           yyy[j] = (float *) allocateWithId(sizeof(float)*maxnumspec,"expl");
           if ((x[j]==0)||(y[j]==0))
           { Werrprintf("cannot allocate buffer");
             releaseAllWithId("expl");
             fclose(datafile);
             return 1;
           }
         }
      }
      if (!regression) 
      {
	 if (rflag && foundfirst)
	 {
	    line_index[i] = (int) temp1;
	    foundfirst = 0;
	 }
	 else
	    fscanf(datafile,"%d",&line_index[i]);
      }
      if (realargflag && !regression) inx = line_index[i] -1;
      else inx =i;
      j = 0;
      while (index[j] != inx && j < MAXSETS-1 )j++;
       
      if (index[j] != inx) /* this data set is not plotted */
        { 
	  int limit = oldnumspec;
	  if (NEXTflag) limit = spec_count[i];
          if (!regression && (exptype == CP_ANALYSIS || exptype == POLY3))
	     fscanf(datafile,"%f",&xx);
	  if (!regression && (fscanf(datafile," %f %f %f\n",
                        &xx,&xx,&xx))!=3)
	    { syntaxerror(l,filename);
              fclose(datafile);
              releaseAllWithId("expl");
              return 1;
            }
          l++;
	  if (foundfirst) 
            fscanf(datafile,"%f \n",&xx);
          for (k=foundfirst; k<limit; k++)
            { if (fscanf(datafile,"%f %f\n",&xx,&yy)!=2)
	        { syntaxerror(l,filename);
                  fclose(datafile);
                  releaseAllWithId("expl");
                  return 1;
                }
              l++;
            }
        }
      else
        { int limit = numspec;
	  int delta = 0;
	  if (NEXTflag) 
	    {  limit = spec_count[i];
	       delta = limit - numspec;
	       if (delta > 0) limit = numspec;
	    }

	  if (regression)
          {
            newindex[cnt] = index[cnt] + 1;
	    cnt++;
 	  }
          else 
          {  
             newindex[j] = line_index[i];
	     cnt++;
	     if ((exptype != CP_ANALYSIS && exptype != POLY3 &&
                     (fscanf(datafile," %f %f %f\n",
                        &a0[j],&a1[j],&a2[j]))!=3) || 
                   ((exptype == CP_ANALYSIS || exptype == POLY3) && 
		     (fscanf(datafile," %f %f %f %f\n",
                        &a0[j],&a1[j],&a2[j],&a3[j]))!=4) )
	    { syntaxerror(l,filename);
              fclose(datafile);
              releaseAllWithId("expl");
              return 1;
            }
          }
	  if (exptype != CP_ANALYSIS && exptype != POLY3) a3[j] = 0.0;
          if (rflag && i == 0 && lcount == 1) 
	      printcurve(xaxistmp,exptype,a0[i],a1[i],a2[i],a3[i]);
          l++;
	  if (foundfirst)
	    {
		x[j][0]=temp1;
                fscanf(datafile,"%f\n",&y[j][0]);
	    }
          for (k=0; k<limit; k++)
            { if (!foundfirst || k)
	       if (fscanf(datafile,"%f %f\n",&x[j][k],&y[j][k])!=2)
	        { syntaxerror(l,filename);
                  fclose(datafile);
                  releaseAllWithId("expl");
                  return 1;
                }
	      if (axis1==SQUARE) 
	          xxx[j][k] =x[j][k]*fabs(x[j][k]); 
	      else if (axis1==LOG) 
		{ if (logerr(x[j][k])) 
		    return(1);
		  else 
		    xxx[j][k]=log(x[j][k])/LOGE10;
		}
 	      else xxx[j][k]=x[j][k];
	      if (axis2==SQUARE) yyy[j][k] =y[j][k]*fabs(y[j][k]); 
	      else if (axis2==LOG)
		{ if (logerr(y[j][k])) 
		    return(1);
		  else yyy[j][k]=log(y[j][k])/LOGE10;
		}
 	      else yyy[j][k]=y[j][k];
	      if (xscaleflag) 
                {  if (xxx[j][k]<xmin) xmin = xxx[j][k];
                   if (xxx[j][k]>xmax) xmax = xxx[j][k];
	        }
	      if (yscaleflag) 
                {  if (yyy[j][k]<ymin) ymin = yyy[j][k];
                   if (yyy[j][k]>ymax) ymax = yyy[j][k];
	        }
              l++;
            }
          foundany = 1;
	  if (i == numlines-1)
	    { break;
	    }
          if (delta) 
	    for (k=0; k<delta; k++)
	       if (fscanf(datafile,"%f %f\n",&xx,&yy)!=2)
	        { syntaxerror(l,filename);
                  fclose(datafile);
                  releaseAllWithId("expl");
                  return 1;
                }
        }
    }
  fclose(datafile);
   
/*****
Wscrprintf("index[i]=%d %d %d %d %d %d newindex=%d %d %d %d %d %d\n",
  index[0],index[1],index[2],index[3],index[4],index[5],
  newindex[0],newindex[1],newindex[2],newindex[3],newindex[4],newindex[5]);
*****/

  /**** *****  Part 4. In Regression, Make Input File for Analysis.  ***** ****/
  if (regression)
  {
    strcpy(filename,curexpdir);
#ifdef UNIX
    strcat(filename,"/analyze.inp");
#else
    strcat(filename,"analyze.inp");
#endif
    if ( (datafile=fopen(filename,"w+")) )
      { 
 	fprintf(datafile,"%s\n",s0);
 	fprintf(datafile,"%s\n",sy);
        fprintf(datafile,"%12d %12d %s %s\n",lcount,numspec,xaxis,yaxis);
	fprintf(datafile,"\n");
        for (i=0; i<lcount; i++)
          {
	    int limit = numspec;

	    if (NEXTflag) 
	      { 
	        limit = spec_count[index[i]];
	        if (limit > numspec) limit = numspec;
	        fprintf(datafile,"    NEXT    %d\n", limit); 
	      }
	    fprintf(datafile,"%d\n",1+index[i]);
	    
            for (j=0; j<limit; j++)
              { fprintf(datafile,"%12g %12g\n",xxx[i][j],yyy[i][j]);
              }
          }
        fclose(datafile);
      }
    else
      { Werrprintf("cannot open file %s",filename);
        releaseAllWithId("expl");
        return 1;
      }
  }


  /**** *****  Part 5. Do display or Plotting of Data.  ***** ****/
  P_getstring(CURRENT,"axis",stemp, 1, 10);
  /* if axis='cc', chart parameters may be constrained to certain aspect ratios */
  P_setstring(CURRENT,"axis","hh", 0);
  if (select_init(1,plotflag,0,0,0,1,0,0)) 
  {
      P_setstring(CURRENT,"axis",stemp, 0);
      releaseAllWithId("expl");
      if (regression)
        Winfoprintf("Try button again or try returning /vnmr/fidlib/fid1d\n");
      if (plotflag == 2)
        Winfoprintf("pexpl command ignored");
      RETURN;
  }
  P_setstring(CURRENT,"axis",stemp, 0);

  if (!foundany)
    { Werrprintf("no data available");
      return 1;
    }

  if (xscaleflag) 
   {
      xmax = xmax + 0.1 * (xmax-xmin);
      newxmin = xmin - 0.1 * (xmax-xmin);
      if (xmin >= 0.0 && newxmin < 0.0) xmin = 0.0;
      else xmin = newxmin;
      if (exptype<DIFFUSION && !regression && xmin<0.0) xmin = 0.0; 
      if (fabs(xmax - xmin) < 1e-16)
      {
         xmax += 0.5;
         xmin -= 0.5;
      }
    }
  if (yscaleflag) 
   {
      ymax = ymax + 0.1 * (ymax-ymin);
      ymin = ymin - 0.1 * (ymax-ymin);
      if ((ymax>0.0) && (ymin<0.0) && (fabs(ymax+ymin)<0.2*ymax))
        { if (ymax >= -ymin) ymin = -ymax;
          else ymax = -ymin;
        }
      if (fabs(ymax - ymin) < 1e-16)
      {
         ymax += 0.5;
         ymin -= 0.5;
      }
    }
  xmultiplier = dnpnt / (xmax-xmin);
  ymultiplier = dnpnt2 / (ymax-ymin);

  disp_status("EXPL");
  if (plotflag==1 && !oldbox)
    { if (clearScreen)
         Wclear_graphics();
      Wturnoff_buttons();
      Wshow_graphics();
      Wsetgraphicsdisplay("expl");
      show_plotterbox();
    }

  appendvarlist("dummy");

  if (regression || lbl1 || lbl2 || (exptype >= POLY1 && exptype <= POLY0))
  {
    set_scale_axis(HORIZ,' ');
    set_scale_axis(VERT,' ');
  }
  else
  {
    if (link || axis1 || axis2 ||(exptype>=DIFFUSION && exptype <=POLY3))
    {
        set_scale_axis(HORIZ,' ');
        set_scale_axis(VERT,' ');
    }
    else
    {
        set_scale_axis(HORIZ,'s');
        set_scale_label(HORIZ,"time");
        set_scale_axis(VERT,' ');
    }
  }
  if (xmin > xmax)
     set_scale_pars(HORIZ,xmax,xmin-xmax,1.0,0);
  else
     set_scale_pars(HORIZ,xmin,xmax-xmin,1.0,1);
  if (ymin > ymax)
     set_scale_pars(VERT, ymax,ymin-ymax,1.0,0);
  else
     set_scale_pars(VERT, ymin,ymax-ymin,1.0,1);

  if (!oldbox)
    {
      scale2d(1,0,1,SCALE_COLOR);   	               /* 09/08/87, RL */
      if ((ymin<0.0) && (ymax>0.0))
        { color(SCALE_COLOR);
          amove(dfpnt,dfpnt2 - (int)(ymultiplier * ymin));
          rdraw(dnpnt-1,0);
        }
      if ((xmin<0.0) && (xmax>0.0))
        { color(SCALE_COLOR);
          amove(dfpnt - (int)(xmultiplier * xmin),dfpnt2);
          rdraw(0,dnpnt2-1);
        }
      if (lbl2==LOG)
        { if      (lbl1==LOG) sprintf(s,"Log10(y) vs Log10(x)");  
          else if (lbl1==SQUARE) sprintf(s,"Log10(y) vs Square(x)");  
          else if (lbl1==LINEAR) sprintf(s,"Log10(y) vs x");  
        }
      else if (lbl2==SQUARE)
        { if      (lbl1==LOG) sprintf(s,"Square(y) vs Log10(x)");  
          else if (lbl1==SQUARE) sprintf(s,"Square(y) vs Square(x)");  
          else if (lbl1==LINEAR) sprintf(s,"Square(y) vs x");  
        }
      else if (lbl2==LINEAR)
        { if      (lbl1==LOG) sprintf(s,"y vs Log10(x)");  
          else if (lbl1==SQUARE) sprintf(s,"y vs Square(x)");  
        }
      if (strlen(s) && strcmp(s,NOTITLE))
      {     sss = dfpnt + (dnpnt - (strlen(s) * xcharpixels))/2;
            amove(sss,dfpnt2 - 3*ycharpixels);
            dstring(s);
      }
      if (regression || (exptype>=DIFFUSION && exptype<= POLY0))
      {     sss = dfpnt + (dnpnt - (strlen(s0) * xcharpixels))/2;
            amove(sss,dfpnt2 - 3*ycharpixels);
            if (strcmp(s0,NOTITLE))
                dstring(s0);
      }
      if (!Wissun()) charpix = ycharpixels;
      else charpix = xcharpixels;
      if (strcmp(sy,NOTITLE) && (dnpnt2+3*ycharpixels >= (int) strlen(sy) * charpix))
      {     
	    /* Y-axis title only with regression */
            sss = strlen(sy) * charpix;
            if (sss > dnpnt2)
            {
               sss = dfpnt2 - (sss - dnpnt2)/2;
               if (sss < dfpnt2 - 3*ycharpixels)
                  sss = dfpnt2 - 3*ycharpixels;
            }
            else
	       sss = dfpnt2 + (dnpnt2 - sss)/2;
            amove(dfpnt - 9*xcharpixels,sss);
            dvstring(sy);
      }
   }
  for (i=0; i<MAXCURVES; i++)
    { if ((index[i]>=0) && (index[i] < numlines))
        { 
	  int limit = numspec;
	  if (NEXTflag) 
	    { 
	      limit = spec_count[i];
	      if (limit > numspec) limit = numspec;
	    }
          if (!oldbox) symbolnum = i; 
 	  primary_color(symbolnum);
          amove(xcharpixels,mnumypnts-(symbolnum+2)*ycharpixels);
          sprintf(s,"#%d",newindex[i]);
          if (!oldbox) dstring(s);
	  if (usesymbol) /* default */
	  {
            point_symbol(5*xcharpixels,
              mnumypnts-(symbolnum+2)*ycharpixels+ycharpixels/3,symbolnum,0,usesymbol);
            for (j=0; j<limit; j++)
              point_symbol(dfpnt+(int)((xxx[i][j]-xmin)*xmultiplier),
                      dfpnt2 + (int)((yyy[i][j]-ymin)*ymultiplier),symbolnum,1,usesymbol);
	  }
          if (link)
	    for (j=1; j<limit; j++)
	       point_link_draw( dfpnt + (int)((xxx[i][j-1]-xmin)*xmultiplier),
                                dfpnt2 + (int)((yyy[i][j-1]-ymin)*ymultiplier),
	        	 	(int)((xxx[i][j]-xxx[i][j-1])*xmultiplier),
                        	(int)((yyy[i][j]-yyy[i][j-1])*ymultiplier) );
          if (plotcurve && !link)
            { yy = msubt(xmin,a0[i],a1[i],a2[i],a3[i],exptype);
	      ypos = setypos(axis2,yy,ymultiplier,ymin);
              amove(dfpnt,ypos);
              for (j=0; j<dnpnt; j+=4)
                { xx = j/xmultiplier + xmin;
		  if (axis1==SQUARE) xx=sqrt(xx);
		  else if (axis1==LOG) xx=exp(xx*LOGE10);
                  yy = msubt(xx,a0[i],a1[i],a2[i],a3[i],exptype);
	          ypos = setypos(axis2,yy,ymultiplier,ymin);
	
                  adraw(dfpnt+j,ypos);
                }
            }
	  do_expl_out(xaxis,yaxis,xmin,xmax,ymin,ymax, setnum);
        }
      else  if (index[i] >= numlines)
	{ Werrprintf("no data available for curve %d",index[i]+1);
	}
    }
  if (plotflag)
    endgraphics();

  disp_status("    ");
  releaseAllWithId("expl");
  RETURN;
}

/******************************************/
/* read the head of the file 'regression.inp' */
static int readhead(FILE *datafile, char *s0, char *sy, char *filename,
                    int *numlines, int *numspec, int *l)
/******************************************/
{
     char tmp1[1024];
     char tmp2[1024];
     char tmp3[1024];
     strcpy(s0,"No Title");
     fscanf(datafile,"%[^\n]\n", tmp1);
     fscanf(datafile,"%[^\n]\n", tmp2);
     if (fscanf(datafile,"%[^\n]\n", tmp3) != 1)
     {
        syntaxerror(*l,filename);
        fclose(datafile);
        fileformat();
        return 1;
     }
     if (sscanf(tmp3,"%d %d", numlines, numspec) == 2)
     {
        strcpy(s0,tmp1);
        (*l)++;
        strcpy(sy,tmp2);
	(*l)++;
        RETURN;
     }
     if (sscanf(tmp2,"%d %d", numlines, numspec) == 2)
     {
        strcpy(s0,tmp1);
        (*l)++;
        RETURN;
     }
     if (sscanf(tmp1,"%d %d", numlines, numspec) == 2)
     {
        RETURN;
     }
     syntaxerror(*l,filename);
     fclose(datafile);
     fileformat();
     return 1;
}

/******************************************/
static int readfile(FILE *datafile, int *numlines, int *maxnumspec, int *spec_count)
/******************************************/
{
  char stemp[20];
  int counter = 0;
  float xx,yy;

  *spec_count = 0;
  *maxnumspec = 0;
  *numlines = 0;

  if (fscanf(datafile,"%f",&xx)) return (2);

  while (fscanf(datafile,"%s\n",stemp) == 1) 
  {
     counter = 0;
     while ((fscanf(datafile,"%f ",&xx)) == 1) 
     {
       if (fscanf(datafile,"%f\n",&yy) !=1)
       {
         Wscrprintf("Problem with data format in 'regression.inp'\n");
	 ABORT; 
       }
       counter++;
     }
     *spec_count = counter;
     if (counter > *maxnumspec) *maxnumspec = counter;
     (*numlines)++;
     spec_count++;
  }
  /* Wscrprintf("numlines=%d maxnumspec=%d spec_count=%d\n",
	*numlines,*maxnumspec,*spec_count); */
  RETURN;
}

/******************************************/
static int setypos(int axis2,float y,float ymultiplier,float ymin)
/******************************************/
/*	Return y-position of point in expl curve plot. 	*/
{
  	int ypos;
  	float yy;
	yy=y;
	if (axis2==SQUARE) yy *=fabs(yy);
	else if (axis2==LOG) 
	    { if (logerr(yy)) yy=0;
	      else yy = log(yy)/LOGE10;
	    }
	ypos = dfpnt2 + (int)((yy-ymin)*ymultiplier);
	if (ypos > dnpnt2+dfpnt2) ypos = dnpnt2 + dfpnt2+1; 
  	else if (ypos <dfpnt2) ypos = dfpnt2-1;
	return ypos;
}

/*************************************************/
static void do_expl_out(char xaxis[], char yaxis[], float xmin, float xmax, float ymin, float ymax, int setnum)    
/*************************************************/
/*	save information about expl box and scales	*/
{
  char  filename[MAXPATHL];
  FILE *datafile;

  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/expl.out");
#else
  strcat(filename,"expl.out");
#endif
  if ( (datafile=fopen(filename,"w+")) )
  {
    fprintf(datafile,"%f   %f   %f   %f\n",sc,wc,sc2,wc2);
    fprintf(datafile,"%g   %g   %s\n",xmin,xmax,xaxis);
    fprintf(datafile,"%g   %g   %s\n",ymin,ymax,yaxis);
    fprintf(datafile,"%s %d %d\n","FLAG",2,2);
    fprintf(datafile,"%d\n",setnum);
    fclose(datafile);
  }
  else Werrprintf("Unable to open '%s'",filename);
}

/**************/
void primary_color(int i)
/**************/
{ if (plot)
    color(i+1);
  else
    switch (i)
      { case 1: color(BLUE); break;
        case 2: color(MAGENTA); break;
        case 3: color(GREEN); break;
        case 4: color(CYAN); break;
        default : color(RED); break;
      }
}

int checkscwc(char *scn,char *wcn,char *wcmaxn, double offset)
{
    double sc,wc,wcmax;
    if (P_getreal(CURRENT,scn,&sc,1) == 0 && 
        P_getreal(CURRENT,wcn,&wc,1) == 0 &&
        P_getreal(GLOBAL ,wcmaxn,&wcmax,1) == 0 )
    {
      if (wcmax==0)
         return (1);
      /* not less than 10 mm */
      if (wc < 10.0)
      {
         wc = 10.0;
         P_setreal(CURRENT,wcn,wc,1);
      }
      else if (wc > (1.0 -offset) * wcmax )
      {
          wc = (1.0 -offset) * wcmax;
          P_setreal(CURRENT,wcn, wc ,1);
      }
      if (wc+sc > (1.0 -offset) * wcmax )
      {
          P_setreal(CURRENT,scn,  (1.0 -offset) * wcmax - wc ,1);
      }
    }
    return(0);
}

static int setlbl(char *name)
{
    int tmp = 0;
    {
 	if      (strcmp(name,"linear") == 0)
		tmp = LINEAR;
 	else if (strcmp(name,"square") == 0)
		tmp = SQUARE;
 	else if (strcmp(name,"log") == 0)
		tmp = LOG;
       }
    return (tmp);
}

void setaxis(int axis, char *axisname)
{
 	if        (axis == 1)   strcpy(axisname,"square");
 	else   if (axis == 2)   strcpy(axisname,"log");
 	else /*if (axis == 0)*/ strcpy(axisname,"linear");
}

void syntaxerror(int ln, char *filename)
{
   Werrprintf("syntax error on line %d of file %s",ln,filename);
}

static int printcurve(char *xaxis, int exptype,
               float a0, float a1, float a2, float a3)
{
  int lbl;
  char s[40];
  char c = 'x';
  s[0]='\0';
  lbl = setlbl(xaxis);
  if (lbl == 1)
    strcpy(s,"u = x*x  "); 
  else if (lbl == 2)
    strcpy(s,"u = log(x)  "); 
  else c='x';
  if (exptype == POLY0)
    Winfoprintf("mean = %.5g, standard deviation = %.5g",a0,a1);
  else if (exptype == POLY1)
    Winfoprintf("%sy = %.5g + %.5g * %c",s,a0,a1,c);
  else if (exptype == POLY2)
    Winfoprintf("%sy = %.5g + %.5g * %c + %.5g * %c*%c",s,a0,a1,c,a2,c,c);
  else if (exptype == POLY3)
    Winfoprintf("%sy = %.5g + %.5g * %c + %.5g * %c*%c + %.5g * %c*%c*%c",
	s,a0,a1,c,a2,c,c,a3,c,c,c);
  else if (exptype == EXPONENTIAL)
    Winfoprintf("%sy = %.5g * exp(-%c/%.5g) + %.5g",s,a0,c,a1,a2); 
  return (FALSE);
}

#define MAXCNT 		10
/************************************/
int dels(int argc, char *argv[], int retc, char *retv[])
/*--------------------------remove spectra from 'fp.out' and therfore from
			    t1, t2, and kinetics analysis. Arguments are 
			    positions in table of lines to be eliminated.  */
/************************************/
{
   FILE           *datafile;
   FILE           *tempfile;
   char            filename[MAXPATHL];
   char            tempfilename[MAXPATHL];
   int             line_no,
                   spec_no,
                   cnt, i, j, k, l,
                   writeflag = 1,
                   numlines,
                   numspec,
                   iarg[MAXCNT];
   float           read_amp;
   float           read_freq;

  (void) retc;
  (void) retv;
   if (argc == 1)
   {
      Winfoprintf(
        "uasge - dels(index1,...) - deletes spectra from T1 or T2 analysis");
      return 1;
   }
   /* open fp.out  */
   strcpy(filename, curexpdir);
   strcpy(tempfilename, curexpdir);
#ifdef UNIX
   strcat(filename, "/fp.out");
   strcat(tempfilename, "/fptemp");
#else
   strcat(filename, "fp.out");
   strcat(tempfilename, "fptemp");
#endif
   if ( (datafile = fopen(filename, "r")) )
   {
      l = 1;
      if (fscanf(datafile, "List of %d lines in spectrum 1 to spectrum %d\n",
		 &numlines, &numspec) != 2)
      {
	 syntaxerror(l, filename);
	 fclose(datafile);
	 return 1;
      }

      tempfile = fopen(tempfilename, "w");

      cnt = 0;
      while ((cnt < argc - 1) && (cnt < MAXCNT) && 
	     (atoi(argv[cnt + 1]) <= numspec))
      {
	 cnt++;
	 iarg[cnt] = atoi(argv[cnt]);
	 /* Wscrprintf("cnt = %d iarg[cnt]=%d\n", cnt,iarg[cnt]); */
      }
      fprintf(tempfile, "List of %d lines in spectrum 1 to spectrum %d\n",
	      numlines, numspec - cnt);
      l=2;

      /* read frequencies of the active lines */
      fscanf(datafile, "line         frequency (Hz)\n");
      fprintf(tempfile, "line         frequency (Hz)\n");
      l=3;
      for (line_no = 0; line_no < numlines; line_no++)
      {
	 if (fscanf(datafile, "%d%f\n", &i, &read_freq) != 2)
	 {
	    syntaxerror(l, filename);
	    fclose(datafile);
	    fclose(tempfile);
	    return 1;
	 }
	 fprintf(tempfile, "%4d       %11.2f\n", i, read_freq);
	 l++;
      }

      /* read amplitudes of the active lines */
      /* fscanf(datafile,"line    spectrum    amplitude (mm)\n"); */
      fscanf(datafile, "%*[^\n]\n");
      fprintf(tempfile, "line    spectrum    amplitude (mm)\n");
      l++;
      for (line_no = 0; line_no < numlines; line_no++)
	 for (spec_no = 0; spec_no < numspec; spec_no++)
	 {
	    if (fscanf(datafile, "%d%d%f\n", &i, &j, &read_amp) != 3)
	    {
	       syntaxerror(l, filename);
	       fclose(datafile);
	       fclose(tempfile);
	       return 1;
	    }
	    l++;
	    writeflag = 1;
	    for (k = 1; k <= cnt; k++)
	       if (iarg[k] == spec_no + 1)
		  writeflag = 0;
	    if (writeflag)
	       fprintf(tempfile, "%4d        %4d  %11.2f\n", i, j, read_amp);
	 }
      fclose(datafile);
      fclose(tempfile);
      chmod(filename,0666);
      chmod(tempfilename,0666);
      unlink(filename);
      link(tempfilename,filename);
      unlink(tempfilename);
      return 0;
   }
   else
   {
      Werrprintf("cannot open file %s, use 'nll' and 'fp'", filename);
      return 1;
   }
}



/**********************     PCSS stuff *****************************/
/**********************    by Lynn Cai *****************************/
#define MAXLINE 4096
#define MAXPEAK 1024

struct line
       { 
         float max;
         int maxp, w;
       } sline[MAXLINE];

struct multiplet
       { 
         float max;
         int begin, end, ln, maxp, w;
       } smultiplet[MAXPEAK];

int n_pcss;
float thresh, maxj, maxgw;


static int fpcss(float *spec1, int pts, double hzpp)
{

   int maxd, maxw, marg;
   int ok;
   int npl;
   int nmult = 0;
   float xdev;
   
   maxd = (int) maxj / hzpp;
   maxw = (int) maxgw / hzpp;
   marg = (int) maxj / 5.0 / hzpp;
   ok=1;
 
   findth(spec1, &xdev, pts);

   npl = findpeaklines(spec1, xdev, pts);

     if (npl>=MAXLINE)
                { Werrprintf("too many lines, increase threshold or decrease vs");
                  ok = 0;
                }

     if (npl==0)
                { Werrprintf("too less lines, decrease threshold or increase vs");
                  ok = 0;
                }

   if(ok) new_css(npl, &nmult, maxd, marg, maxw);
   n_pcss = nmult;

 return(ok);
}

/****************************************/
/* findth()  find the threshold 	*/
/****************************************/
static void findth(float *spec, float *xdev, int npts)
{
  int i,nn,k, n, nk;
  float  value, dev, av, mv;

  nn = npts/20;
  if (nn < 20)
    nn = 20;
  else if (nn > 200)
    nn = 200;

if (thresh==0.0) 
{
  av= 0.0;
  for (i=0; i<npts; i++)
   {
    value = *(spec+i);
    av += value; 
   }
  av /=(float)npts;
  dev=0.0;
  for (i=0; i<nn; i++)
   {
    value = *(spec+i);
    dev += (value-av)*(value-av);
   }
  dev=sqrt(dev/nn);
  thresh = av+6.0*dev;
} 
 
else
{
  av= 0.0; 
  n = 0;
  for (i=0; i<nn; i++)
   {
    value = *(spec+i);
    if (value < thresh)
      { av += value; n++;}
   }

   if (n!=0) av /=(float)n;
   k=0;
   nk = n;
   
  mv=0.0;
  n = 0;
  for (i=npts/20; i<npts/20+nn; i++)
   {
    value = *(spec+i);
    if (value < thresh)
      { mv += value; n++;}
   }
   
  if(n>nk)
   {k=1; av=mv/(float)n; nk=n;}

  mv=0.0;
  n = 0;
  for (i=npts/10; i<npts/10+nn; i++)
   {
    value = *(spec+i);
    if (value < thresh)
      { mv += value; n++;} 
   }

  if(n>nk)
   {k=2; av=mv/(float)n; nk=n;}


  dev=0.0;
  for (i=k*npts/20; i<k*npts/20+nn; i++)
   {
    value = *(spec+i);    
    if (value < thresh)
       dev += (value-av)*(value-av); 
   }

  dev=sqrt(dev/nk);
}

*xdev = dev;  

}

static int findpeaklines(float *spec1, float xdev, int pts)
{
  int start,end;
  int l_number, width;
  int maxp, ok;
  float  maxv;

   start=0;
   end=0;
   l_number=0;

  while ( end < pts-1)
  {
   ok=find_a_line(spec1, xdev, start, &end, &maxv, &maxp, &width, pts);
   if (ok==1)
    {
     if (l_number>=MAXLINE)
        {
         Werrprintf("too many lines, increase threshold or decrease vs");
         end =pts;
        }
     else{
    	sline[l_number].max=maxv;
    	sline[l_number].maxp=maxp;
    	sline[l_number].w = width;
	l_number++;	
        start = end;
      }
    }
  }
   
  return(l_number);

}


static int find_a_line(float *spec, float xdev, int start, int *end, float *max, int *maxp, int *w, int npts)
{
int i, ok, np_inpeak, line_done;
int line_max, line_begin, line_end, lw, rw;
float line_maxv,line_beginv, line_endv;
float  value;
int minpointsinpeak = 3;

i=start; 
ok=FALSE;

value = *(spec + i);
   
line_max  = line_begin  = line_end  = 0;
line_maxv = line_beginv = line_endv = 0.0;
while (!ok&&i<npts)
 { 
   if (value <= thresh) 
    {
     i++;
     if (i<npts)
	  value = *(spec + i);
    }
   else 
    {
     line_begin = i;
     line_beginv=value; 
     line_endv=value;
     line_end=i;
     line_maxv = value;
     line_max=i;
     line_done = 0;
     while (line_done<2&&i<npts) 
       {
	i++;
	if (i<npts)
	  { 
     	   value = *(spec + i);
           if (value < thresh) {line_done = 2; ok = TRUE;}
	   else 
            {
            if (value > line_endv+3.0*xdev&&line_done==1) {line_done = 2; ok = TRUE;}
	    else  
               { 
                if (value < line_maxv - 3.0*xdev) 
		   {line_done=1;line_endv=value; line_end=i;}
        	if (value > line_maxv) 
		   {line_maxv=value; line_max=i;line_endv=value; line_end=i;}
               }
            }
          }
        } 
      np_inpeak =line_end-line_begin+1;
      if (np_inpeak<minpointsinpeak) ok=FALSE;

    }  /*end of else */

 } /*end of while !ok*/

 *end = i-1;

 if (line_max<=line_begin) lw = 0;
 else if (line_maxv==line_beginv) lw = (line_max-line_begin)/2;
 else lw=(line_max-line_begin)*(line_maxv/(line_maxv-line_beginv))/2;
 
 if (line_max>=line_end) rw = 0;
 else if (line_maxv==line_endv) rw = (line_end-line_max)/2;
 else rw=(line_end-line_max)*(line_maxv/(line_maxv-line_endv))/2;

 *w = lw + rw;
 *max = line_maxv;
 *maxp = line_max;

 return(ok);

}

static int new_css(int nl, int *nmult, int DD, int MM, int WW)
{
int i, j, k, d, np, d1, w, width;
float value, sum, wt;
int peak_done;

k = 0;
i = 0;
peak_done = 1;

while (k < nl)
 {
  if (i>=MAXPEAK)
  	{ Werrprintf("too many peaks, increase threshold or decrease vs");
          ABORT;
        }
        
  if (peak_done == 1) 
    { 
      smultiplet[i].ln = 1;
      smultiplet[i].begin = sline[k].maxp;
      smultiplet[i].end = sline[k].maxp;
      k++;
    }

 if ( k < nl )
    d = sline[k].maxp - sline[k-1].maxp;
 else d = WW+2*MM;

  if (d < DD - MM) 
   {
    w = sline[k].maxp - smultiplet[i].begin;
    if ((w < WW - MM) || (w < WW && d < DD-2*MM) || (w < WW+MM && d < DD-3*MM))
     { 
     smultiplet[i].ln += 1;
     smultiplet[i].end = sline[k].maxp;
     k++;
     peak_done = 0;
     }
    else 
     { i++; peak_done = 1;}
   }
  else if (d < DD && k < nl-1)
   {
    d1 = sline[k+1].maxp - sline[k].maxp;
    
    w = sline[k].maxp - smultiplet[i].begin;
    if (w < WW && d1 > d ) 
     { 
     smultiplet[i].ln += 1;
     smultiplet[i].end = sline[k].maxp;
     k++;
     peak_done = 0;
     }
    else 
     { i++; peak_done = 1;}
   }
  else 
     { i++; peak_done = 1;}
 }

np=i+1;
*nmult=np;

k=0;
for (i=0; i<np; i++ )
 {
  w = smultiplet[i].end - smultiplet[i].begin;
  value = 0.0;
  wt = 0.0;
  sum = 0.0;
  width = 0;
  for (j=0; j < smultiplet[i].ln; j++)
   {
    width += sline[k].w;
    value += sline[k].max; 
    d = sline[k].maxp - smultiplet[i].begin;
    if (d<w/6||d>5*w/6) 
     {
      wt += 0.125;
      sum += (float) sline[k].maxp*0.125;
     }
    else if (d<w/3||d>2*w/3) 
     {
      wt += 0.25;
      sum += (float) sline[k].maxp*0.25;
     }
    else
     {
      wt += 1.0;
      sum +=(float) sline[k].maxp;
     }    
    k++;
   }

  if (smultiplet[i].ln == 0)
  {
     smultiplet[i].w = width;
     smultiplet[i].maxp = (int) sum;  
  }
  else
  {
     smultiplet[i].w = width/smultiplet[i].ln;
     smultiplet[i].maxp = (int) sum/wt;  
  }
  smultiplet[i].max = value;
 }
RETURN;
}




/*********************************/
int do_pcss(int argc, char *argv[], int retc, char *retv[])
/*********************************/
{
  FILE *datafile;
  char  filename[MAXPATHL];
  int   i;
  double freq, width, hzpp, rflrfp;
  int ctrace, dummy, ok;
 
  (void) retc;
  (void) retv;
  maxj=20.0;
  maxgw=60.0;
  thresh=0.0;   
  
  Wturnoff_buttons();
  if(init2d(1,1)) return(ERROR);
  disp_status("PCSS      ");
  ctrace = currentindex();
  if ((spectrum = calc_spec(ctrace-1,0,FALSE,TRUE,&dummy))==0)
    return(ERROR);
  scale = vs;
  if (normflag)
    scale *= normalize;
 		
  if (strcmp(argv[0], "do_pcss") == 0)
     {
	if((argc-- > 1) && isReal(*++argv))
		thresh = stringReal (*argv);
 
	if((argc-- > 1) && isReal(*++argv))
		maxj = stringReal (*argv);
 
	if((argc-- > 1) && isReal(*++argv))
		maxgw = stringReal (*argv);
     
	th = thresh;
	hzpp = sw/(double) ((fn/2)-1);
	ok=fpcss(spectrum, fn/2, hzpp);
      
        if (ok)
         {
	     /* open pcss.outpar  */
	  strcpy(filename,curexpdir);
#ifdef UNIX
	     strcat(filename,"/pcss.outpar");
#else
	     strcat(filename,"pcss.outpar");
#endif
	  
	  get_rflrfp(HORIZ,&rflrfp);
	
	  if ( (datafile=fopen(filename,"w+")) )
  	   {
	    for (i=0; i < n_pcss; i++)
	      {
        	width =  (double) smultiplet[i].w*hzpp;
        	freq = sw - rflrfp - (double) smultiplet[i].maxp*hzpp;
        	fprintf(datafile,"%11.2f, %11.2f, %11.2f, 0.5\n", 
                	freq, smultiplet[i].max, width);
      	      }

   	    fclose(datafile);
    	    chmod(filename,0666);
  	   }
    	  else
  	   { Werrprintf("cannot open file %s",filename);
  	     return(ERROR); 
  	   }
  	   
       	 } 
     }

  disp_status("        ");
  return(COMPLETE);
}

