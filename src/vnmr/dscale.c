/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************/
/*							*/
/*  display	-	common display functions	*/
/*  dscale	-	display a frequency scale	*/
/*  pscale	-	plot a frequency scale		*/
/*  axis 	-	returns axis labels and scaling	*/
/*							*/
/********************************************************/


/************************************************/
/* standard display function definitions	*/
/************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vnmrsys.h"
#include "graphics.h"
#include "group.h"
#include "data.h"
#include "disp.h"
#include "init2d.h"
#include "allocate.h"
#include "sky.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"
#include "init_display.h"
#include "init_proc.h"
#include "dscale.h"

#define FALSE	0
#define TRUE	1
#define BAND		10
#define OUT_NUM		(MAXSPEC+2)

#define OVERLAID_ALIGNED 3

#ifdef  DEBUG
extern int debug1;
#define DPRINT(str) \
	if (debug1) fprintf(stderr,str)
#define DPRINT1(str, arg1) \
	if (debug1) fprintf(stderr,str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3,arg4,arg5)
#else 
#define DPRINT(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) 
#endif 

extern void startGraphFunc(char *cmd, int reexec, int redo);
extern void finishGraphFunc();
extern int get_vj_overlayType();
extern int colorindex(char *colorname, int *index);
extern void set_graph_clear_flag(int n);
extern void     Wturnoff_buttons();
extern void Wactivate_mouse();
extern void window_redisplay();
extern void set_graphics_font(const char *fontName);
extern void set_line_thickness(const char *thick);
extern void set_spectrum_thickness(char *min, char *max, double ratio);
extern void set_background_region(int x, int y, int w, int h, int color, int alpha);

#ifdef VNMRJ
extern int isJprintMode();
extern void set_tic_location(int x, int y);
extern void appendJvarlist(const char *name);
extern int getOverlayMode();
#endif

static struct ybar *out[OUT_NUM];
static int out_size[OUT_NUM];
static int old_df[OUT_NUM];
static int old_dn[OUT_NUM];
static double vp0 = 0;
void envelope(short *buf, int n, int sgn);
int dscale_on();
int m_drawVscale = FALSE;
int m_yoff = 0;
double m_vscale = 1.0;
int aspMode = 0;
int get_drawVscale();
void fillPlotBox2D();
void fillPlotBox();
extern int isInset();
static int dscaleDecimalX=0;
static int dscaleDecimalY=0;
static int arrayDis=0;

void setArraydis(int mode) {
   arrayDis=mode;
}

double getPlotMargin();

int getDscaleDecimal(int i) { 
  if(i==1) return dscaleDecimalY; 
  else return dscaleDecimalX;
}

int showPlotBox() {
  double d;
  if(isInset() || (!d2flag && plot) || P_getreal(GLOBAL,"showPlotBox", &d, 1) || d < 1.0) return 0;
  else return 1;
}

// 1 normal aspMode, 2 aspMode without axis label 
void setAspMode(int mode) {
   aspMode = mode;
}

extern int fwc,fsc,fwc2,fsc2;

void set_vscaleMode(int mode) {
   double margin = getPlotMargin();
   if(mode==TRUE) wc = wcmax-2*margin - 4*xcharpixels*(double)wcmax/(double)(mnumxpnts-right_edge);
   else wc = wcmax-2*margin;
   P_setreal(CURRENT,"wc",wc,0);
   fwc=(int)wc;
   fsc=(int)sc;
   fwc2=(int)wc2;
   fsc2=(int)sc2;
   m_drawVscale = mode;
}

void set_vscale(int off, double vscale)
{
    m_yoff = off;
    if(vscale > 0) m_vscale = vscale;
    else m_vscale = 1.0;
}

/***********/
void init_proc2d()
/***********/
/* called upon bootup to initialize proc2d parameters */
{ int i;
  for (i=0; i<OUT_NUM; i++)
    { out_size[i] = 0;
      out[i] = 0;
    }
}

/**********************************************/
/* the following simulates the mouse function */
/**********************************************/

static int mouse_active_button;
static int (*save_user_mouse_button)();

static void mouse_button(int butnum, int updown, int x, int y)
{ if (updown) /* if releasing a button */
    mouse_active_button = 0; 
  else
    { (*save_user_mouse_button)(butnum + 1,x,usercoordinate(y),0);
      mouse_active_button = butnum + 1;
    }
}

static void move_mouse(int x, int y)
{ if (mouse_active_button)
    (*save_user_mouse_button)(mouse_active_button,x,usercoordinate(y),1);
}

void activate_mouse(int (*user_mouse_button)(), int (*mouse_reset)())
{ 
  if(user_mouse_button != save_user_mouse_button) { 
    mouse_active_button = 0;
    save_user_mouse_button = user_mouse_button;
  }
  Wactivate_mouse(move_mouse,mouse_button,mouse_reset);
}

/****************************************/
/* end of the mouse function simulation */
/****************************************/

/***********************************************/
/* the following are standard display routines */
/***********************************************/

/****************************************************************/
int fid_ybars(float *phasfl, double scale, int df, int dn,
              int n, int off, int newspec, int dotflag)
/****************************************************************/
{ short *bufpnt;
  int ybarsize;

  /* this condition typically occurs if the graphics screen is re-sized */
  if (df + dn > mnumxpnts)
    return(1);

  ybarsize = mnumxpnts;
  if (mnumypnts>ybarsize)
    ybarsize = mnumypnts;
  if ((bufpnt = (short *) allocateWithId(sizeof(short)*n,"newProc"))==0)
    { Werrprintf("cannot allocate buffer space");
      Wturnoff_buttons();
      return 1;
    }
  if (out_size[newspec] != ybarsize)
    { if (out[newspec] != 0)
        release(out[newspec]);
      if ((out[newspec] =
        (struct ybar*)allocateWithId(sizeof(struct ybar)*ybarsize,"newProc"))==0)
        { Werrprintf("cannot allocate ybar buffer space");
          return 1;
        }
      out_size[newspec] = ybarsize;
    }
  scfix1(phasfl,2,(float) scale,bufpnt,1,n);
  if (dn > n) 
    { if (dotflag)
        fid_expand(bufpnt,n,out[newspec]+df,dn,off);
      else
        expand(bufpnt,n,out[newspec]+df,dn,off);
    }
  else
    compress(bufpnt,n,out[newspec]+df,dn,off);
  release(bufpnt);
  return 0;
}

/****************************************************************/
int fill_ybars(int df, int dn, int off, int newspec)
/****************************************************************/
{
  register int index;
  struct ybar *out0;
  register int top, bot;

  out0 = out[newspec]+df;
  for (index = 0; index < dn; index++)
  {
     top = abs(out0->mx - off);
     bot = abs(out0->mn - off);
     if (out0->mx < off)
       out0->mx = off + bot;
     else if (out0->mn > off)
       out0->mn = off - top;
     else if (top >= bot)
       out0->mn = off - top;
     else
       out0->mx = off + bot;
     out0++;
  }
  return(0);
}

/****************************************************************/
int abs_ybars(float *phasfl, double scale, int df, int dn, int n,
              int off, int newspec, int dotflag, int sgn, int env)
/****************************************************************/
{
    short *bufpnt;
    int ybarsize, bytesize;

    (void) dotflag;
    /* this condition typically occurs if the graphics screen is re-sized */
    if (df + dn > mnumxpnts)
      return(1);

    ybarsize = mnumxpnts;
    if (mnumypnts>ybarsize)
	ybarsize = mnumypnts;
    if ((bufpnt = (short *) allocateWithId(sizeof(short)*n,"newProc"))==0)
    {
	Werrprintf("cannot allocate buffer space");
	Wturnoff_buttons();
	return 1;
    }
    if (out_size[newspec] != ybarsize)
    {
	if (out[newspec])
	    release(out[newspec]);
	bytesize = sizeof(struct ybar) * ybarsize;
	out[newspec] = (struct ybar*)allocateWithId(bytesize, "newProc");
	if (out[newspec] == 0) {
	    Werrprintf("cannot allocate ybar buffer space");
	    return 1;
	}
	out_size[newspec] = ybarsize;
    }
    scabs(phasfl,2,scale,bufpnt,1,n, sgn);
    if (env) {
        envelope(bufpnt, n, sgn);
    }
    if (dn > n) 
	expand(bufpnt,n,out[newspec]+df,dn,off);
    else
	compress(bufpnt,n,out[newspec]+df,dn,off);
    release(bufpnt);
    return 0;
}

void envelope(short *buf, int n, int sgn)
{
    short xpeak1;
    short ypeak1;
    short ypeak2;
    short slope;
    int i;
    int j;

    if (n < 2) {
        return;
    }
    ypeak1 = buf[0] * sgn;
    xpeak1 = 0;
    ypeak2 = buf[1] * sgn;
    slope = (ypeak2 - ypeak1);
    for (i = 2; i < n; ++i) {
        short pk = sgn * buf[i];
        if (slope <= 0) {
            if (pk > ypeak2) {
                slope = 1;
            } else {
                ypeak2 = pk;
            }
        } else {
            if (pk >= ypeak2) {
                ypeak2 = pk;
            } else {
                /* Interpolate points between last two peaks */
                double y = ypeak1;
                double dy = (double)(ypeak2 - ypeak1) / (i - 1 - xpeak1);
                for (j = xpeak1 + 1; j < i - 1; ++j) {
                    y += dy;
                    buf[j] = (short)rint(y) * sgn;
                }
                slope = -1;
                xpeak1 = i - 1;
                ypeak1 = ypeak2;
                ypeak2 = pk;
            }
        }
    }
}

/****************************************************************/
int envelope_ybars(float *phasfl, double scale, int df, int dn,
                   int n, int off, int newspec, int dotflag, int sgn)
/****************************************************************/
{
    int rtn;

    rtn = abs_ybars(phasfl,scale,df,dn,n,off,newspec,dotflag, sgn, 1);
    if (rtn == 0) {
	/* Modify to eliminate a lot of wiggles. */
    }
    return rtn;
}

/***************************************************/
int calc_ybars(float *phasfl, int skip, double scale, int df, int dn,
               int n, int off, int newspec)
/***************************************************/
{ short *bufpnt;
  int ybarsize;

  /* this condition typically occurs if the graphics screen is re-sized */
  if (df + dn > mnumxpnts)
    return(1);

  ybarsize = mnumxpnts;
  if (mnumypnts>ybarsize)
    ybarsize = mnumypnts;
  if ((bufpnt = (short *) allocateWithId(sizeof(short)*n,"newProc"))==0)
    { Werrprintf("cannot allocate buffer space");
      Wturnoff_buttons();
      return 1;
    }
  if (out_size[newspec] != ybarsize)
    { if (out[newspec] != 0)
        release(out[newspec]);
      if ((out[newspec] =
        (struct ybar*)allocateWithId(sizeof(struct ybar)*ybarsize,"newProc"))==0)
        { Werrprintf("cannot allocate ybar buffer space");
          return 1;
        }
      out_size[newspec] = ybarsize;
    }
  scfix1(phasfl,skip,(float) scale,bufpnt,1,n);
  if (dn > n) 
    expand(bufpnt,n,out[newspec]+df,dn,off);
  else
    compress(bufpnt,n,out[newspec]+df,dn,off);
  release(bufpnt);
  return 0;
}

/*****************************/
static void pl_compress(short *bufpnt, int pnx,
                        struct ybar *out0, int onx, int vo)
/*****************************/
{ register int inx; /* counter through input points */
  register double d; /* counter through input data points */
  register double f; /* compression factor */
  register int v;
  register int y_max,y_min;
  register int f_max,f_min;
  int onx_count = 0;
  register struct ybar *out;

  out = out0;
  f = (double)(onx)/(double)pnx;
  d = 0.0;
  y_max = -32767; y_min = 32767;
  f_min = f_max = 0;
  for (inx=0; inx<pnx; inx++)
    { v = *bufpnt++ + vo;
      if (v>y_max)
      {
         f_max = inx;
         y_max = v;
      }
      if (v<y_min)
      {
         f_min = inx;
         y_min = v;
      }
      d += f;
      if (d>=2.0)
	{ d -= 2.0;
          if (f_max < f_min)
          {
             out->mx = out->mn = y_max;
             out++;
             out->mx = out->mn = y_min;
             out++;
          }
          else
          {
             out->mx = out->mn = y_min;
             out++;
             out->mx = out->mn = y_max;
             out++;
          }
          y_max = -32767; y_min = 32767;
          onx_count += 2;
        }
    }
   if (onx_count < onx)
   {
      out->mn = y_min;
      out->mx = y_max;
   }
}

/***************************************************/
int calc_plot_ybars(float *phasfl, int skip, double scale, int df, int dn,
                    int n, int off, int newspec)
/***************************************************/
{ short *bufpnt;
  int ybarsize;

  /* this condition typically occurs if the graphics screen is re-sized */
  if (df + dn > mnumxpnts)
    return(1);

  ybarsize = mnumxpnts;
  if (mnumypnts>ybarsize)
    ybarsize = mnumypnts;
  if ((bufpnt = (short *) allocateWithId(sizeof(short)*n,"newProc"))==0)
    { Werrprintf("cannot allocate buffer space");
      Wturnoff_buttons();
      return 1;
    }
  if (out_size[newspec] != ybarsize)
    { if (out[newspec] != 0)
        release(out[newspec]);
      if ((out[newspec] =
        (struct ybar*)allocateWithId(sizeof(struct ybar)*ybarsize,"newProc"))==0)
        { Werrprintf("cannot allocate ybar buffer space");
          return 1;
        }
      out_size[newspec] = ybarsize;
    }
  scfix1(phasfl,skip,(float) scale,bufpnt,1,n);
  pl_compress(bufpnt,n,out[newspec]+df,dn,off);
  release(bufpnt);
  return 0;
}

/*
 *  return the data point index corresponding to
 *  the supplied un-referenced frequency       
 *  hzpp = sw / rfn
 *  freq = sw - hzpp * dp
 *  dp = (sw - freq) / hzpp
 *  dp = (sw - freq) * rfn / sw
 */
/* freq  non-referenced frequency */
/* rfn   total real or complex pairs of data points */
/************************/
int datapoint(double freq, double sw, int rfn)
/************************/
{
  return((int) ( (sw - freq) * (double) rfn / sw + 0.01) );
}

/***************************************************/
int int_ybars(float *integral, double scale, int df, int dn, int fpt,
              int npt, int off, int newspec, int resets, double sw, int fn)
/***************************************************/
{ float *bufpnt;
  double value;
  float  lastint;
  int    r;
  int    index;
  int    point;
  int    lastpt;

  if ((bufpnt = (float *) allocateWithId(sizeof(float) * (npt+1),"newProc"))==0)
  {
    Werrprintf("cannot allocate bufpnt");
    return(1);
  }

  lastpt = fpt;
  lastint = *integral;
  index = 1;
  while ((index <= resets) && (lastpt < fpt + npt - 1))
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
      value = 0.0;          /*  no resets defined  */
    point = datapoint(value,sw,fn/2);
    if (point < 0) point = 0;
    if (point>fpt)
    {
      if (point>fpt+npt-1)
        point = fpt + npt - 1;
      if (lastpt<fpt)
        lastpt = fpt;
      if (point < lastpt)
        point = fpt + npt - 1;
      vssubr(integral+lastpt,lastint,bufpnt+lastpt-fpt,point-lastpt+1);
    }
    lastint = *(integral+point);
    lastpt = point;
    index++;
  }

  calc_ybars(bufpnt,1,scale,df,dn,npt,off,newspec);
  release(bufpnt);
  return(0);
}

/***************************************************/
int displayspec(int df, int dn, int vertical, int *newspec, int *oldspec,
                int *erase, int max, int min, int dcolor)
/***************************************************/
{ int    temp;

  char name1[64], name2[64];
  double d, factor = 0.0; 
  int npts;
  
  if(d2flag && P_getreal(GLOBAL, "thickFactor", &factor, 2)) factor = 0.0;
  else if(!d2flag && P_getreal(GLOBAL, "thickFactor", &factor, 1)) factor = 0.0;
 
  if(sw>0) d = wp/sw;
  else d = 1.0;
  if(factor < 0) { // thickness depends on wp/sw and factor = 0.0 to -1.0
     d = (1.0 - d)*fabs(factor);
  } else { // thickness depends on data points and graphics pixels, factor = 0.0 to 1.0 
     npts = d*fn/2;
     if(npts>0) d = factor*(double)dnpnt/(double)npts;
     else d = 0.0;
  }
  if(d>1.0) d=1.0;
  if(d<0) d=0.0;

  getOptName(SPEC_LINE_MIN,name1);
  getOptName(SPEC_LINE_MAX,name2);
  //Winfoprintf("## thick %s %s %f",name1,name2,d);

  grf_batch(1);
  xormode();
  if(dcolor>=0) color(dcolor);
  set_spectrum_thickness(name1, name2, d);
  if (*erase)
    ybars(old_df[*oldspec],old_dn[*oldspec],out[*oldspec],vertical,0,0);
  ybars(df, df+dn-1, out[*newspec], vertical, max, min);
  normalmode();
  grf_batch(0);
  temp = *oldspec;
  *oldspec = *newspec;
  *newspec = temp;
  *erase   = 1;
  old_df[*oldspec] = df;
  old_dn[*oldspec] = df + dn -1;
  return 0;
}

/*******************************************/
void erasespec(int specindex, int vertical, int dcolor )
/*******************************************/
{
  grf_batch(1);
  xormode();
  color(dcolor);
  ybars(old_df[ specindex ],old_dn[ specindex ],out[ specindex ],vertical,0,0);
  normalmode();
  grf_batch(0);
}

/* moved the following two functions to graphics_win.c  */
/*************************************
int x_cursor(old_pos,new_pos)
int *old_pos;
int  new_pos;
{
  if (*old_pos != new_pos)
  {
    if (*old_pos>0)
    {
      amove(*old_pos,1);
      rdraw(0,mnumypnts-3);
    }
    if (new_pos>0)
    {
      amove(new_pos,1);
      rdraw(0,mnumypnts-3);
    }
    *old_pos = new_pos;
  }
}


int y_cursor(old_pos,new_pos)
int *old_pos;
int  new_pos;
{
  DPRINT2("y_cursor  old= %d new= %d\n",*old_pos,new_pos);
  if (*old_pos != new_pos)
  {
    if (*old_pos>0)
    {
      amove(1,*old_pos);
      rdraw(mnumxpnts-right_edge-3,0);
    }
    if (new_pos>0)
    {
      amove(1,new_pos);
      rdraw(mnumxpnts-right_edge-3,0);
    }
    *old_pos = new_pos;
  }
}
********************************/


/********************************/
int set_cursors(int two_cursors, int either_cr, int *newpos,
                int oldpos, int oldpos2, int *idelta, int df, int dn)
/********************************/
{
  if (either_cr)
  {
    if (*newpos<df) 
    {
      if ((*newpos<df-BAND) && (oldpos>=df))
        *newpos = oldpos;
      else
        *newpos = df;
    }
    if (two_cursors)
    {
      if (*idelta<2)
        *idelta = 2;
      if (*idelta>=df+dn-2)
        *idelta=df+dn-3;
    }
    else
      *idelta = 0;
    if (*newpos >= df + dn - *idelta)
    {
      if ((*newpos>df+dn+BAND) && (oldpos<df+dn- *idelta) && (oldpos>=df))
        *newpos = oldpos;
      else
        *newpos = df + dn - *idelta -  1;
    }
  }
  else
  {
    if ((*newpos<df-BAND)||(*newpos>df+dn+BAND))
      *newpos = oldpos2;
    *idelta  = *newpos - oldpos;
    if (*idelta<2)  *idelta  = 2;
    if (*idelta>=df+dn-oldpos)
      *idelta=df+dn-oldpos-1;
  }
  return 0;
}

/*******************/
float vs_mult(int x_pos, int y_pos, int max, int min, int df, int dn, int index)
/*******************/
{
  int start;
  register int i,end,level;

  if (y_pos - min < 1)
    return(0.5);
  else
  {
    start = x_pos - BAND/2;
    end   = x_pos + BAND/2;
    if (start < df) start = df;
    if (end > df + dn) end = df + dn;
    level = min + 1;
    for (i=start; i<=end; i++)
      if (out[index][i].mx>level)
        level = out[index][i].mx;
    if (level >= max - 1) 
      return(0.1);
    else
      return((float) (y_pos - min) / (float) (level - min));
  }
}

/**************************/
struct ybar *outindex(int index)
/**************************/
/*  return the pointer to a specified ybars buffer */
{
  return(((index<0) || (index>=OUT_NUM)) ? 0 : out[index]);
}

void exit_display()
/****************/
{ register int i;
  for (i=0; i<OUT_NUM; i++)
    { if (out[i])
        release(out[i]);
      out[i] = 0;
      out_size[i] = 0;
    }
}

/****************/
void init_display()
/****************/
{ exit_display();
}

/****************/
/*-----------------------------------------------
|						|
|     end of the standard display routines	|
|						|
+----------------------------------------------*/
/* s,len	start and length of scale 	*/
/* reverse,dez	number of decimal places	*/
/* ftic		first tic			*/
/* ticw,ticw2,ticw3	distance between tics 	*/
/* dpnts        number of display points        */
/* given the start and length of a scale, this routine calculates the	*/
/* best positioning of tics on the scale. 3 parameters are returned 	*/

static void ctics(double s, double len, int reverse, int *dez, double *ftic,
                  double *ticw, double *ticw2, double *ticw3, int dpnts)
#define AVTICS	10.0
{ register double w,exp,step,minstep;

  if (len < 0.0)
     len = -len;
  if (len < 1e-6)
     len = 1e-6;
  w = len/AVTICS; exp = 1.0; *dez = 0;
  if (w < 0.1)
    w = 1.0;
  while (w>6.5)  { exp *= 10.0; w *= 0.1; }
  while (w<0.65) { exp *=  0.1; w *= 10.0; *dez += 1; }
  if (w<1.6)
  {
    *ticw  = exp;
    *ticw2 = exp;
    *ticw3 = exp / 5.0;
  }
  else if (w<3.2)
  {
    *ticw  = 2.0 * exp;
    *ticw2 = exp;
    *ticw3 = exp / 5.0;
  }
  else
  {
    *ticw  = 5.0 * exp;
    *ticw2 = 5.0 * exp;
    *ticw3 = exp;
  }

  // Have at least ten tics in the scale. This should display at least two numbers 
  while ((int) (len/ *ticw3) < 15 )
  {
     *ticw3 *= 0.1;
     *ticw = *ticw2 = 5.0 * *ticw3;
     *dez += 1;
  }
  
  if(plot) minstep=15;
  else minstep=5;

  step = *ticw3 * (double)(dpnts) / len;
  while (step < minstep) {
    *ticw  *= 2.0;
    *ticw2 = *ticw;
    *ticw3 = *ticw / 5.0;
    step = *ticw3 * (double)(dpnts) / len;
  }

/*
  step = *ticw3 * (double)(dpnts) / len;

  DPRINT3("ticw3= %g  dnpnt= %d  step= %g\n",*ticw3,dnpnt,step);
  DPRINT2("dpnt/step= %g  mnumpnt/step= %g\n",
               (double) dpnts / step, (double) mnumxpnts / step);
  if (step < 3.0)
  {
    *ticw3 = *ticw2;
    step = *ticw3 * (double)(dpnts) / len;
    DPRINT3("ticw3= %g  dnpnt= %d  step= %g\n",*ticw3,dnpnt,step);
    DPRINT2("dpnt/step= %g  mnumpnt/step= %g\n",
               (double) dpnts / step, (double) mnumxpnts / step);
    if (step < 3.0)
    {
      *ticw3 = *ticw2 = *ticw;
      step = *ticw3 * (double)(dpnts) / len;
      DPRINT3("ticw3= %g  dnpnt= %d  step= %g\n",*ticw3,dnpnt,step);
      DPRINT2("dpnt/step= %g  mnumpnt/step= %g\n",
               (double) dpnts / step, (double) mnumxpnts / step);
    }
  }
*/

  if (reverse) 
    { *ftic = *ticw3 * (double)((int)((s-len) / *ticw3));
      if (*ftic<(s-len)) *ftic += *ticw3;
    }
  else
    { *ftic = *ticw3 * (double)((int)(s / *ticw3));
      if (*ftic<s) *ftic += *ticw3;
    }
}

/**************/
void dpreal(double v, int dez, char *nm)
/**************/
/* convert a floating point value to a 8 character string */
{ char t[16];
  double vt;
  vt=v;
  if (fabs(vt) < exp(-2.302*dez-0.01))
     vt=0;
  sprintf(t,"%%8.%df",dez);
  sprintf(nm,t,vt);
}

/*****************/
static int min(int a, int b)
/*****************/
{ if (a<=b) return a; else return b;
}

/*****************/
static int max(int a, int b)
/*****************/
{ if (a>=b) return a; else return b;
}

/**********************************/
static int tictype(register double freq, register double step)
/**********************************/
{
  register int tmp;

  if (freq < 0.0)
    freq = -freq;
  tmp = ((int)(freq / step + 0.01));
  freq -= ((double) tmp) * step;
  if (freq < 0.0)
    freq = -freq;
  tmp = (int)(freq < (0.01 * step));
  return(tmp);
}

/**************/
void erase_dcon_box()
/**************/
{
//	int ddfpnt2;

//	ddfpnt2 = dfpnt2;
#ifdef VNMRJ
        set_graph_clear_flag(TRUE);
#endif

/*	amove(max(dfpnt-6*xcharpixels,1),max(ddfpnt2-3*ycharpixels,1));
	box(min(dnpnt+8*xcharpixels,
	  mnumxpnts-right_edge-max(dfpnt-6*xcharpixels,1)-2),
	  min(dnpnt2+3*ycharpixels+2,
	  mnumypnts-max(ddfpnt2-3*ycharpixels,1)-2)
	);
*/
	color(BACK);
/*	amove(dfpnt+1,ddfpnt2+2);
	box(dnpnt-1,dnpnt2-2); */ /* old way! */

	amove(dfpnt,dfpnt2);
	box(dnpnt,dnpnt2);

/*	amove(dfpnt-1,ddfpnt2-1);
	rdraw(dnpnt+1,0);
	rdraw(0,dnpnt2+2);
	rdraw(-dnpnt-1,0);
	rdraw(0,-dnpnt2-2);
*/
#ifdef VNMRJ
        set_graph_clear_flag(FALSE);
#endif
}

/* display 2-dimensional scale */
/**********************/
void scale2d(int drawbox, int yoffset, int drawscale, int dcolor)
/**********************/
{ double ftic,ticw,next,step,stop;
  double ticw2,ticw3;
  int dez,i,s,vpos,lastx;
  int labelpos;
  char nm[32];
  char label[32];
  int  ddfpnt2,ddfpnt;
  double start,len,axis_scl,axis_intercept;
  int    reversed;
  char   axisval;
  int ntics, ticct;
  int ticX, ticY;
  char thickName[64];
  char fontName[64];
  float dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;

  if(showPlotBox()) yoffset = 0;

  if(aspMode) ddfpnt2 = dfpnt2;
  else if (drawbox <= 0 && isInset()) {  // 1D inset, draw axis below the spectrum 
    ddfpnt2 = dfpnt2 - 2*dispcalib + vp*dispcalib; 
    ddfpnt2 = dfpnt2 + yoffset - 2*dispcalib; 
  } else if (drawbox <= 0) { // 1D, draw axis 2mm lower than vp=0
    ddfpnt2 = dfpnt2 + yoffset - 2*dispcalib; 
  } else {
    ddfpnt2 = dfpnt2 + yoffset;
  }
  //if(!drawbox && get_drawVscale()) dnpnt2 = mnumypnts-ddfpnt2;
  ddfpnt = dfpnt - 1; // use ddfpnt to determine position of vertical scale. 
  DPRINT5("dfpnt2=%d,yoffset=%d,dfpnt=%d,dnpnt=%d,dnpnt2=%d\n",
      dfpnt2,yoffset,dfpnt,dnpnt,dnpnt2);

    if(!dscale_on()) {
        if(d2flag && !plot) {
#ifdef VNMRJ
            set_graph_clear_flag(TRUE);
#endif
            color(BACK);
            amove(max(dfpnt-6*xcharpixels,1),max(ddfpnt2-3*ycharpixels,1));
            box(min(dnpnt+8*xcharpixels,
	        mnumxpnts-right_edge-max(dfpnt-6*xcharpixels,1)-2),
	        min(dnpnt2+3*ycharpixels+2,
	        mnumypnts-max(ddfpnt2-3*ycharpixels,1)-2));
   	    fillPlotBox2D();
#ifdef VNMRJ
            set_graph_clear_flag(FALSE);
#endif
	}
        /* then draw the box */
        color(dcolor);			/* plot maps this to pen 1 */
        amove(dfpnt-1,ddfpnt2-1);
        rdraw(dnpnt+1,0);
        rdraw(0,dnpnt2+2);
        rdraw(-dnpnt-1,0);
        rdraw(0,-dnpnt2-2);
	return;
    }

      if(arrayDis) {
	arrayDis=0;
        // only draw ticks for the first spectrum.
	double dss_wc,dss_sc;
	if(P_getreal(CURRENT,"dss_wc",&dss_wc,1)) dss_wc = wc;
	if(P_getreal(CURRENT,"dss_sc",&dss_sc,1)) dss_sc = sc;
	dnpnt = (int)((double)(mnumxpnts-right_edge)*dss_wc/wcmax); 
        dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-dss_wc-dss_sc)/wcmax);
        ddfpnt = dfpnt-1;
        if(dnpnt<1) return; 

      }

  getOptName(AXIS_LINE,thickName);
  set_line_thickness(thickName);

/*  2D plots use pen 1 for the scale.
    Use 'drawbox' to distinguish between the two situations.	*/

  if ((!drawbox) || ((ddfpnt2 > 0 ) && (dfpnt > 0)))
    { if (!plot) Wgmode();
      if (drawbox)				/* 2D plots */
      { /* first clear area behind the box
           (but only if not plotting!!)   */
	if (!plot)
	{
#ifdef VNMRJ
            set_graph_clear_flag(TRUE);
#endif
            color(BACK);
            amove(max(dfpnt-6*xcharpixels,1),max(ddfpnt2-3*ycharpixels,1));
            box(min(dnpnt+8*xcharpixels,
	        mnumxpnts-right_edge-max(dfpnt-6*xcharpixels,1)-2),
	        min(dnpnt2+3*ycharpixels+2,
	        mnumypnts-max(ddfpnt2-3*ycharpixels,1)-2)
	    );
	// fill plot box with BOX_BACK color
	fillPlotBox2D();
/*
        color(BOX_BACK);
	amove(dfpnt,dfpnt2+1);
	box(dnpnt,dnpnt2);
*/
#ifdef VNMRJ
            set_graph_clear_flag(FALSE);
#endif
	}
        /* then draw the box */
        color(dcolor);			/* plot maps this to pen 1 */
        amove(dfpnt-1,ddfpnt2-1);
        rdraw(dnpnt+1,0);
        rdraw(0,dnpnt2+2);
        rdraw(-dnpnt-1,0);
        rdraw(0,-dnpnt2-2);
      }
      else					/* 1D plots */
      {
        if (!plot) {
#ifdef VNMRJ
            if (!isJprintMode() && !aspMode)
                xormode();
#else 
            xormode();
#endif
	// fill plot box with BOX_BACK color
/*
        color(BOX_BACK);
	amove(dfpnt,dfpnt2 - 2*dispcalib + 1);
	box(dnpnt,dnpnt2 + 2*dispcalib);
*/
        }

        // then draw the box 
/*
        color(dcolor);			// plot maps this to pen 1 
        amove(dfpnt-1,ddfpnt2-1);
        rdraw(dnpnt+1,0);
        rdraw(0,dnpnt2+2*dispcalib);
        rdraw(-dnpnt-1,0);
        rdraw(0,-dnpnt2-2*dispcalib);
*/
        color(dcolor);
        if(!showPlotBox() || isInset()) { // otherwise  axis will be drawn as part of plot box.
          amove(dfpnt,ddfpnt2);
          rdraw(dnpnt,0);				//  Draw axis  
          if(get_drawVscale()) {
            amove(ddfpnt,ddfpnt2);
            rdraw(0,ddfpnt2+dnpnt2);	//  Draw vertical axis 
          }
	}
      }

      get_scale_axis(HORIZ,&axisval);
      if (axisval!='n')
        {


   /*  draw the horizontal axis  */

          get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
          get_intercept(HORIZ,&axis_intercept);
          if ( (reversed) && (axis_intercept > 0.0))
             start += axis_intercept;
          else if (reversed)
             start += len;

   /*  Next block is a trick that deals with a negative scale factor.  The
       ctics program jams if axis_scl is negative.  Use the reverse feature
       of scale2d/ctics (also used with FID), since (like with FIDs), the
       numbers are increasing left to right, the opposite of (normal) spectra.
       Changing the sign of start completes the operation (this change of
       sign is implied by the negative scale factor).				*/

          else if (axis_scl < 0)
          {
            axis_scl = -axis_scl;
            start = -start;
            reversed = 1;
          }
          /* avoid division by zero */
          if (axis_scl < 1e-6)
             axis_scl=1.0;
          start = (start - axis_intercept) / axis_scl;
          len  /= axis_scl;
          DPRINT3("sp=%g,wp=%g,axis_scl=%g\n", start,len,axis_scl);
          ctics((double)start,(double)len,reversed,
                &dez,&ftic,&ticw,&ticw2,&ticw3,dnpnt);
	  dscaleDecimalX = dez;
          DPRINT5("dez=%d,ftic=%g,ticw=%g,ticw2=%g,ticw3=%g\n",
                   dez,ftic,ticw,ticw2,ticw3);
          step = ticw3*(double)(dnpnt)/len;
          if (reversed)
            { next = (double)(dfpnt)+
                     (ftic-start+len)*(double)(dnpnt)/len;
              stop = start + 0.0001 * len;
              lastx = 0;
              step = -step;
            }
          else
            { next = (double)(dfpnt+dnpnt)-
                     (ftic-start)*(double)(dnpnt)/len;
              stop = start + 1.0001 * len;
              lastx = mnumxpnts;
            }
          DPRINT4("next=%g,stop=%g,step=%g,lastx=%d\n",
                        next,stop,step,lastx);
          DPRINT2("xchar=%d,ychar=%d\n", xcharpixels,ycharpixels);
          if ( !drawscale )
           if (Wistek()) color(BLACK);

    /*  draw the label for the (horizontal) axis */

  	  getOptName(AXIS_LABEL,fontName);
          set_graphics_font(fontName);
          if (drawbox)
          {
            get_label(HORIZ,(axisval == ' ') ? UNIT1 : UNIT2,label);
            s = strlen(label);

	    //if(showPlotBox() && aspMode != 2) {
	      // display label below axis numbers
              labelpos = dfpnt + (dnpnt - (s * xcharpixels))/2;
              amove(labelpos,ddfpnt2 - 3*ycharpixels);
              dstring(label);
              labelpos = mnumxpnts;
/*
	    } else if(aspMode != 2) {
	      // display label on the same line of axis numbers
              labelpos = dfpnt + dnpnt;
              labelpos -= s * xcharpixels/2;
              amove(labelpos,ddfpnt2 - (int) (1.50*ycharpixels));
              dstring(label);
	    }
*/
          }
          else
          {
            get_label(HORIZ,UNIT1,label);
            s = strlen(label);

	    if(showPlotBox() && aspMode != 2) {
	      // display label below axis numbers
              labelpos = dfpnt + (dnpnt - (s * xcharpixels) + xcharpixels/2)/2;
              amove(labelpos,ddfpnt2 - 3*ycharpixels);
              dstring(label);
              labelpos = mnumxpnts;
	    } else if(aspMode != 2) {
	      // display label on the same line of axis numbers
              labelpos = dfpnt + dnpnt;
              labelpos -= s * xcharpixels + xcharpixels/2;
              amove(labelpos,ddfpnt2 - (int) (1.50*ycharpixels));
              dstring(label);
	    } else labelpos=mnumxpnts;

            if (!reversed)
              lastx = labelpos;
          }
          if ( !drawscale )
           if (Wistek()) color(dcolor);

    /*  draw the tic marks and the numbers for the (horizontal) axis  */

          ntics = (int)(len/ticw3+0.5) + 4; /* needs at least 2 extra tics in count */
		/* multiply ntics by unit scale factor 
		   axisf - 's','m','u','n'
		   axis  - 'c','d','1','2','3','h','k','m','n','p','u'
		   need conversion factors for all
		   ntics not really necessary */
	  ntics = 1e4; /* just make sure no infinite loop */
          ticct = 0;
  	  getOptName(AXIS_NUM,fontName);
          set_graphics_font(fontName);
          do
	    {
              if (tictype(ftic,ticw))
              {
                  // amove((int)(next+0.5),ddfpnt2-ycharpixels/2-1);
                ticX = (int)(next+0.5);
                ticY = ddfpnt2-ycharpixels/2-1;
                amove(ticX, ticY);
	        rdraw(0,ycharpixels/2);
	        dpreal(ftic,dez,nm);
	        i = 0; while (nm[i]==' ') i++;
	        s = (int)next - (8-i)*xcharpixels/2;
	        if (((!reversed) && (s+(9-i)*xcharpixels<lastx)) ||
                    ((reversed) && (s-(9-i)*xcharpixels>lastx) &&
	                         (s+(9-i)*xcharpixels<labelpos)))
	        { lastx = s;
		    // amove(s,ddfpnt2 - (int) (1.50*ycharpixels));
                  ticY = ticY - ycharpixels;
		  amove(s, ticY);
		  if (s>0)
                  {
#ifdef VNMRJ
                    set_tic_location(ticX, ticY);
#endif
                    DPRINT2("s=%d  s+xch=%d\n", s, s+xcharpixels);
                    if ( !drawscale )
                       if (Wistek()) color(BLACK);
                    dstring(nm+i);
                    if ( !drawscale )
                       if (Wistek()) color(dcolor);
#ifdef VNMRJ
                     set_tic_location(-1, -1);
#endif
                  }
	        }
              }
              else if (tictype(ftic,ticw2))
              {
                amove((int)(next+0.5),ddfpnt2-(int) (3 * ycharpixels/8)-1);
	        rdraw(0,(int) (3 * ycharpixels/8));
              }
              else
              {
                amove((int)(next+0.5),ddfpnt2-ycharpixels/4-1);
	        rdraw(0,ycharpixels/4);
              }
              ticct++;
              next = next-step;
	      ftic = ftic+ticw3;
	    }
	    while ((ftic <= stop) && (ticct <= ntics));
            if (ticct > ntics) 
            {
	      Werrprintf("horizontal scale tic label error");
              next = next+step*((double)((ntics+1)/2));
	      ftic = ftic-ticw3*((double)((ntics+1)/2));
              amove((int)(next+0.5),ddfpnt2-ycharpixels/2-1);
	      rdraw(0,ycharpixels/4);
	      dpreal(ftic,dez,nm);
	      i = 0; while (nm[i]==' ') i++;
	      s = (int)next - (8-i)*xcharpixels/2;
	      if (((!reversed) && (s+(9-i)*xcharpixels<lastx)) ||
                  ((reversed) && (s-(9-i)*xcharpixels>lastx) &&
	                       (s+(9-i)*xcharpixels<labelpos)))
	      { lastx = s;
	        amove(s,ddfpnt2 - (int) (1.50*ycharpixels));
		if (s>0)
                {
                  DPRINT2("s=%d  s+xch=%d\n", s, s+xcharpixels);
                  if ( !drawscale )
                     if (Wistek()) color(BLACK);
                  dstring(nm+i);
                  if ( !drawscale )
                     if (Wistek()) color(dcolor);
                }
	      }
            }
        }
      if (drawbox)
         get_scale_axis(VERT,&axisval);
      DPRINT1("vertical axis = %c\n",axisval);
      if ((drawbox && (axisval!='n')) || (!drawbox && get_drawVscale()) )
        {

    /* draw the vertical axis  */
      DPRINT("start vertical axis\n");

          if(drawbox) {
            get_scale_pars(VERT,&start,&len,&axis_scl,&reversed);
            get_intercept(VERT,&axis_intercept);
          } else {
	    start = (ddfpnt2-m_yoff)/m_vscale;
            len = (dnpnt2)/m_vscale;
            axis_scl = 1.0;
            reversed = 1;
            axis_intercept = 0.0;
          }

  	  getOptName(AXIS_LABEL,fontName);
          set_graphics_font(fontName);

          DPRINT4("get_scale_pars  start= %g  len= %g  axis_scl= %g  rev= %d\n",
                   start,len,axis_scl,reversed);
          if ( (reversed) && (axis_intercept > 0.0))
             start += axis_intercept;
          else if (reversed)
             start += len;

    /*  See the comment in the equivalent place in
        the section which draws the horizontal axis.  */

          else if (axis_scl < 0)
          {
            axis_scl = -axis_scl;
            start = -start;
            reversed = 1;
          }
          start = (start - axis_intercept) / axis_scl;
          len  /= axis_scl;
          if (len < 0.0) len = -len;
          ctics((double)start,(double)len,reversed,
                &dez,&ftic,&ticw,&ticw2,&ticw3,dnpnt2);
	  dscaleDecimalY = dez;
          DPRINT("finished ctics\n");
          step = ticw3*(double)(dnpnt2)/len;
          if (reversed)
          { next = (double)(ddfpnt2)+
                   (ftic-start+len)*(double)(dnpnt2)/len;
            stop = start + 0.0001 * len;
            vpos = ddfpnt2+ycharpixels;
            step = -step;
          }
          else
          { next = (double)(ddfpnt2+dnpnt2)-
                   (ftic-start)*(double)(dnpnt2)/len;
            stop = start + 1.0001 * len;
            vpos = ddfpnt2+dnpnt2-ycharpixels;
          } 
          get_label(VERT,UNIT3,label);
          DPRINT1("finished get_label %s\n",label);
 
 	  // display label on the left of axis numbers
	  if(plot) {
	    char tmpstr[64];
	    sprintf(tmpstr,"%d",(int)(start+len));
	    if((strlen(tmpstr)+dez)>4) s = ddfpnt - (6+dez)*xcharpixels;
	    else s = ddfpnt - (5+dez)*xcharpixels;
	  } else {
	    char tmpstr[64];
	    sprintf(tmpstr,"%d",(int)(start+len));
	    if(dez>0) s = ddfpnt - (5+strlen(tmpstr)+dez)*xcharpixels;
	    else s = ddfpnt - (4+strlen(tmpstr)+dez)*xcharpixels;
	  }
	  amove(s,mnumypnts-ycharpixels-dnpnt2/2-strlen(label));
          if (d2flag && !aspMode) dvstring(label);

/*
	  // display label on the same position as numbers.
	  if (vpos>mnumypnts-ycharpixels)
	    vpos = mnumypnts-ycharpixels;
          s = ddfpnt - 5*xcharpixels;
	  amove(s,vpos-ycharpixels/3);
	  label[2] = 0;
          if (d2flag) dstring(label);
	  vpos -=ycharpixels;

    //  draw the label for the (vertical) axis 

          if (strlen(label+3) > 6)       //  if the (sc) label is present 
          {
             int pos;
             pos = strlen(label+3) - 4;
	     label[pos+3] = 0;
	     amove(s,vpos-ycharpixels/3);
	     if (d2flag) dstring(label+3);
	     vpos -=ycharpixels;
	     amove(s,vpos-ycharpixels/3);
             if (d2flag) dstring("(sc)");
          }
          else
          {
            amove(s,vpos-ycharpixels/3);
	    if (d2flag) dstring(label+3);
          }
          if (reversed) vpos += ycharpixels;
*/

    /*  draw the tic marks and the numbers for the (vertical) axis  */

  	  getOptName(AXIS_NUM,fontName);
          set_graphics_font(fontName);

          ntics = (int)(len/ticw3+0.5) + 4; /* needs at least 2 extra tics in count */
	  ntics = 1e4; /* just make sure no infinite loop */
          ticct = 0;
          do
	    {
              if (tictype(ftic,ticw))
              {
                amove(ddfpnt-xcharpixels-1,(int)(next+0.5));
	        rdraw(xcharpixels,0);
	        dpreal(ftic,dez,nm);
	        i = 0; while (nm[i]==' ') i++;
	        s = ddfpnt-xcharpixels*(9-i); if (s<0) s=0;
	        if (((!reversed) && ((int)next<=vpos-2*ycharpixels/3)) ||
                    ((reversed) && ((int)next>=vpos+2*ycharpixels/3)))
		{ amove(s,(int)next-ycharpixels/3);
		  dstring(nm+i);
		  vpos = (int)next;
                }
              }
              else if (tictype(ftic,ticw2))
              {
                amove(ddfpnt- (int) (0.75 * xcharpixels) -1,(int)(next+0.5));
	        rdraw((int) (0.75 * xcharpixels),0);
              }
              else
              {
                amove(ddfpnt-xcharpixels/2 -1,(int)(next+0.5));
	        rdraw(xcharpixels/2,0);
              }
              ticct++;
              next = next-step;
	      ftic = ftic+ticw3;
	    }
	    while ((ftic <= stop) && (ticct <= ntics));
            if (ticct > ntics)
            {
              Werrprintf("vertical scale tic label error");
              next = next+step*((double)((ntics+1)/2));
	      ftic = ftic-ticw3*((double)((ntics+1)/2));
              amove(ddfpnt-xcharpixels-1,(int)(next+0.5));
	      rdraw(xcharpixels,0);
	      dpreal(ftic,dez,nm);
	      i = 0; while (nm[i]==' ') i++;
	      s = ddfpnt-xcharpixels*(9-i); if (s<0) s=0;
	      if (((!reversed) && ((int)next<=vpos-2*ycharpixels/3)) ||
                  ((reversed) && ((int)next>=vpos+2*ycharpixels/3)))
              { amove(s,(int)next-ycharpixels/3);
                dstring(nm+i);
	        vpos = (int)next;
              }
            }
        }
  }
  else 
    Werrprintf("scale outside boundaries, adjust sc, wc, sc2, or wc2");
  normalmode();
  set_graphics_font("Default");
}

/********************************************************/
/* options for dscale and pscale			*/
/*   axis - if the letter p, h, k, etc. is supplied,	*/
/*          it will be used instead of the current	*/
/*          value of the parameter axis			*/
/*   'fid'- this keyword causes a time domain axis      */
/*          to be displayed                             */
/*   vp0  - this is supplied as the first real number	*/
/*          it defines the vertical position where the	*/
/*          scale is drawn.  the default is the current	*/
/*          value of the parameter vp			*/
/*   sp0  - this is supplied as the second real number	*/
/*          it is a modified start of plot. If, for	*/
/*          example, the display is from 347 to 447 Hz, */
/*          but the scale is desired to read 0 to 100	*/
/*          Hz., sp0 would be input as 0		*/
/********************************************************/
/**************************/
static void checkinput(int argc, char *argv[], double *vp_off)
/**************************/
{ int arg_no;
  int done;

/*
  double d;
  if(!P_getreal(GLOBAL, "overlayMode", &d, 1) && (int)d >= OVERLAID_ALIGNED) vp0 = 0.0;
  else vp0 = vp;
*/
  if(showPlotBox()) vp0 = 0;
  else vp0 = vp;

  for (arg_no = 1; arg_no<argc; arg_no++)
    if (!strcmp(argv[arg_no],"rev"))
    {
      set_scale_rev(HORIZ,-1);
    }
    else if (!isReal(argv[arg_no]) && strcmp(argv[arg_no],"fid") &&
        !colorindex(argv[arg_no],&done))
    {
      if (strlen(argv[arg_no]) == 1)
         set_scale_axis(HORIZ,argv[arg_no][0]);
      else
         set_axis_label(HORIZ,argv[arg_no]);
    }
  arg_no = 1;
  done = FALSE;
  *vp_off = 5.0;
  *vp_off = 0.0;
  while ((arg_no<argc) && (!done))
  {
    if (isReal(argv[arg_no]))
    {
      *vp_off = vp0 - (double) stringReal(argv[arg_no]);
      done = TRUE;
    }
    arg_no++;
  }
  done = FALSE;
  while ((arg_no<argc) && (!done))
  {
    if (isReal(argv[arg_no]))
    {
      sp = (double) stringReal(argv[arg_no]);
      set_scale_start(HORIZ,sp);
      done = TRUE;
    }
    arg_no++;
  }
  done = FALSE;
  while ((arg_no<argc) && (!done))
  {
    if (isReal(argv[arg_no]))
    {
      wp = (double) stringReal(argv[arg_no]);
      set_scale_len(HORIZ,wp);
      done = TRUE;
    }
    arg_no++;
  }
}

/*************/
static void setwindows()
/*************/
{
  Wshow_graphics();
  Woverlap();
}

static double scale_sp=0.0;
static double scale_wp=0.0;
static double scale_scl=0.0;
static double scale_vp_off= 0.0;
static int    scale_rev,scale_color,scale_vp,scale_df,scale_dn,scale_df2;
static int   scale_axis;
static int    scale_flag = 0;

void getDscaleFrameInfo()
{
  getFrameScalePars(&scale_sp, &scale_wp, &scale_scl, &scale_vp_off,
                  &scale_rev, &scale_color, &scale_vp, &scale_df, &scale_dn, &scale_df2,
                  &scale_axis, &scale_flag);
}

/*******************************************/
static void set_dscale(int df, int dn, int df2, int vpos,
                      int dcolor, double vp_off)
/*******************************************/
{
  char axisval;
  double start,len,axis_scl;
  int reversed;

  get_scale_axis(HORIZ,&axisval);
  get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
  scale_sp = start;
  scale_wp = len;
  scale_scl = axis_scl;
  scale_rev = reversed;
  scale_color = dcolor;
  scale_vp = vpos;
  scale_vp_off = vp_off;
  scale_df = df;
  scale_dn = dn;
  scale_df2 = df2;
  scale_flag = TRUE;
  scale_axis = axisval;
  setFrameScalePars(scale_sp, scale_wp, scale_scl, scale_vp_off,
                  scale_rev, scale_color, scale_vp, scale_df, scale_dn, scale_df2,
                  scale_axis, scale_flag);
}

void reset_dscale()
{
   if (scale_wp > 0.0)
   {
      set_scale_axis(HORIZ,scale_axis);
      set_scale_pars(HORIZ,scale_sp,scale_wp,scale_scl,scale_rev);
   }
}

/**************/
int dscale_on()
/**************/
{
  double d;
  char str[20];

/* scale_flag is used only if mfShowAxis does not exist and
   display is not dcon, dconi or dpcon */

#ifdef VNMRJ
   if (get_vj_overlayType() >= OVERLAID_ALIGNED)
      return FALSE;

#endif

  if(P_getreal(GLOBAL, "mfShowAxis", &d, 1)) {
        Wgetgraphicsdisplay(str, 20);
	if(strstr(str, "con") != NULL) return TRUE;
  }
  else if(d > 0.5) return TRUE;
  else return FALSE;
  return(scale_flag);
} 

/**************/
int new_dscale(int erase, int draw)
/**************/
{
  float  dispcalib;

/*
  double d;
  if(!P_getreal(GLOBAL, "overlayMode", &d, 1) && (int)d >= OVERLAID_ALIGNED) vp0 = 0.0;
  else vp0 = vp;
*/
  if(showPlotBox()) vp0 = 0;
  else vp0 = vp;

  if(!dscale_on()) return 0;

  if (erase == MAYBE)
     erase = scale_flag;
  if (!erase && !scale_flag)
  {
     scale_color = SCALE_COLOR;
     // scale_vp_off = 5.0;
     //scale_vp_off = SCALE_Y_OFFSET;
  }
  if (erase)
  {
    char axisval;
    double start,len,axis_scl;
    int reversed;
    int    t_df,t_dn,t_df2;
    char label[64];

    get_scale_axis(HORIZ,&axisval);
    if (axisval == '0')
       get_axis_label(HORIZ, label);
    get_scale_pars(HORIZ,&start,&len,&axis_scl,&reversed);
    t_df = dfpnt;
    t_dn = dnpnt;
    t_df2 = dfpnt2;
    set_scale_axis(HORIZ,scale_axis);
    set_scale_pars(HORIZ,scale_sp,scale_wp,scale_scl,scale_rev);
    dfpnt = scale_df;
    dnpnt = scale_dn;
    dfpnt2 = scale_df2;
    scale2d(FALSE,scale_vp,0,scale_color); /* erase old scale */
    scale_flag = FALSE;
    if (axisval == '0')
       set_axis_label(HORIZ, label);
    else
       set_scale_axis(HORIZ,axisval);
    set_scale_pars(HORIZ,start,len,axis_scl,reversed);
    dfpnt = t_df;
    dnpnt = t_dn;
    dfpnt2 = t_df2;
  }
  if (draw)
  {
/*
    int df_on = FALSE;

    if (WgraphicsdisplayValid("df") || WgraphicsdisplayValid("dfs"))
    {
      df_on = TRUE;
    }
 */
    dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
    scale_vp = (int) ((vp0-scale_vp_off) * dispcalib);
    set_dscale(dfpnt,dnpnt,dfpnt2,scale_vp,scale_color,scale_vp_off);
    scale2d(FALSE,scale_vp,131071,scale_color); /* draw new scale */
  }
  return(0);
}

int dscale_onscreen()
{
   return(scale_flag);
}

void clear_dscale()
{
   scale_flag = FALSE;
   setFrameScalePars(scale_sp, scale_wp, scale_scl, scale_vp_off,
                  scale_rev, scale_color, scale_vp, scale_df, scale_dn, scale_df2,
                  scale_axis, scale_flag);
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
/*******************************************************/
static void getcolor(int argc, char *argv[], int *d_color)
/*******************************************************/
{
   int argnum, col, found;

   *d_color = SCALE_COLOR;
   argnum = 1;
   found = FALSE;
   while ((argc>argnum) && (!found))
      found = colorindex(argv[argnum++],&col);
   if (found)
   {
      *d_color = col;
   }
}

int get_drawVscale()
{
   // return 1 if m_drawVscale==1 and not in overlay mode.
   return (dscale_on() && m_drawVscale && !get_vj_overlayType());
}

/*************/
int dscale(int argc, char *argv[], int retc, char *retv[])
/*************/
{ float dispcalib;
  int   ds_on,plot;
  int   df_on;
  int   dcolor;
  double vp_offset;
  int dscalen = 0; 

  (void) retc;
  (void) retv;
  ds_on = (WgraphicsdisplayValid("ds") || WgraphicsdisplayValid("dss") ||
           WgraphicsdisplayValid("dssn") || WgraphicsdisplayValid("addi"));
  df_on = (WgraphicsdisplayValid("df") || WgraphicsdisplayValid("dfs"));
  getDscaleFrameInfo();

  // P_setreal(GLOBAL, "mfShowAxis", (double)1, 1);

  if (strstr(argv[0],"dscalen") == argv[0] || strstr(argv[0],"pscalen") == argv[0])
    dscalen = 1;

  if (argv[0][0] == 'p')
    plot = 1;
  else {
    plot = 0;
#ifdef VNMRJ
    if (argc <= 1) {
        if ( ! dscale_on()) {
            if (P_getreal(GLOBAL, "mfShowAxis", &vp_offset, 1) == 0) {
                P_setreal(GLOBAL, "mfShowAxis", 1.0, 1);
                appendJvarlist("mfShowAxis");
            }
        }
    }
#endif
  }
  if ((argc>1) && (strcmp(argv[1],"off")==0) && (argv[0][0] != 'p'))
  {
    if (scale_flag)
      new_dscale(TRUE,FALSE);
    P_setreal(GLOBAL, "mfShowAxis", 0.0, 1);
    appendJvarlist("mfShowAxis");

    return(0);
  } else if ((argc>1) && (strcmp(argv[1],"vscale")==0))
  {
    if(m_drawVscale == FALSE) set_vscaleMode(TRUE);
    else set_vscaleMode(FALSE);
    // Note, window_redisplay() does not work for ds.
    if(WgraphicsdisplayValid("ds")) execString("ds('again')\n"); 
    else if (ds_on || df_on) window_redisplay();
    return(0);
  }

  if ((df_on) || argtest(argc,argv,"fid"))
  {
     if (initfid(plot+1))
        return(1);
     df_on = TRUE;
  }
/*
 *  select_init(get_rev, dis_setup, fdimname, doheaders, docheck2d,
 *              dospecpars, doblockpars, dophasefile)
 */
  else if (select_init(0, plot+1, 0, 1, 1, 1, 0, 0))
  {
    return 1;
  }
  DPRINT("procedure dscale\n");
  DPRINT4("plot= %d, sp= %g, vp= %g, sfrq= %g\n",plot,sp,vp,sfrq);
  checkinput(argc,argv,&vp_offset);
  getcolor(argc,argv,&dcolor);
  DPRINT3("sp= %g, vp= %g, sfrq= %g\n",sp,vp,sfrq);
  if (plot)
  {
    scale2d(FALSE,(int) ((vp0 - vp_offset) * ppmm / ymultiplier),131071,dcolor);
    amove(0,0);
  }
  else
  {
    dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
    if (scale_flag)
       new_dscale(TRUE,FALSE);
    setwindows();
    scale2d(FALSE,(int) ((vp0 - vp_offset) * dispcalib),131071,dcolor);
    if (!dscalen && (ds_on || df_on)) {
      set_dscale(dfpnt,dnpnt,dfpnt2,
                 (int)((vp0-vp_offset)*dispcalib),dcolor,vp_offset);
    }
  }
  endgraphics();
  return 0;
}

void addi_dscale()
{
   char *argv[3];
   int argc = 1;

   argv[0] = "dscale";
   if (dscale_on())
   {
      argv[1] = "off";
      argc = 2;
   }
   argv[argc] = NULL;
   dscale(argc, argv, 0, NULL);
}

/*************/
int axis_info(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  int   direction = DIRECTION_ERROR;
  double start,len,axis_scl,scl;
  int reversed;
  char label[15];

  if (argc == 1)
  {
     Werrprintf("usage- axis(domain) with domain= 'fn', 'fn1', or 'fn2'");
     return(1);
  }
  else if ((strcmp(argv[1],"fn")  == 0) || (strcmp(argv[1],"fn1") == 0) ||
           (strcmp(argv[1],"fn2") == 0))
  {
/*
 *  select_init(get_rev, dis_setup, fdimname, doheaders, docheck2d,
 *              dospecpars, doblockpars, dophasefile)
 */
     if (select_init(0, 1, 0, 1, 1, 1, 0, 0))
     {
        Werrprintf("axis: problem with parameters");
        return(1);
     }
     direction = get_direction_from_id(argv[1]);
  }
  else
  {
     Werrprintf("axis(domain) with domain= 'fn', 'fn1', or 'fn2',  not %s",
                 argv[1]);
     return(1);
  }
  if ((direction == HORIZ) || (direction == VERT))
  {
     get_scale_pars(direction,&start,&len,&axis_scl,&reversed);
     get_scalesw(direction,&scl);
     get_label(direction, UNIT1, label);
  }
  else
  {
     Werrprintf("domain %s not present", argv[1]);
     return(1);
  }
  if (retc > 0)
  {
     retv[0] = newString(label);
     if (retc > 1)
        retv[1] = realString(axis_scl);
     if (retc > 2)
        retv[2] = realString(scl);
  }
  else
  {
     Winfoprintf("axis label is %s,  scaling factor is %g",label,axis_scl);
  }
  return(0);
}

void getPlotBox_Pix(double *x, double *y, double *w, double *h) {
    *x=dfpnt;
    *w=dnpnt;
    *y=dfpnt2;
    *h=dnpnt2;
}

void drawPlotBox() {
  double dispcalib;
  char thickName[64];
  int ddfpnt2;
  double plot_dfpnt,plot_dnpnt, plot_dfpnt2, plot_dnpnt2;
  if(!showPlotBox()) return;
  if(isInset()) return;

  getPlotBox_Pix(&plot_dfpnt, &plot_dfpnt2, &plot_dnpnt, &plot_dnpnt2); 

    dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
    ddfpnt2 = plot_dfpnt2 - 2*dispcalib; // make dscale 2mm lower.

    // fill box, this is only temporary
    fillPlotBox();
/*
    color(BOX_BACK);
    amove(dfpnt,dfpnt2 - 2*dispcalib + 1);
    box(dnpnt,dnpnt2 + 2*dispcalib);
*/

    // draw box
    getOptName(AXIS_LINE,thickName);
    set_line_thickness(thickName);
    color(SCALE_COLOR);                  // plot maps this to pen 1 
    amove(plot_dfpnt-1,ddfpnt2-1);
    rdraw(plot_dnpnt+1,0);
    rdraw(0,plot_dnpnt2+2*dispcalib);
    rdraw(-plot_dnpnt-1,0);
    rdraw(0,-plot_dnpnt2-2*dispcalib);
}

void getVLimit(int *vmin, int *vmax) {
    double dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
    if(!showPlotBox()) return;

    *vmin = ((double)(mnumypnts-ymin)*sc2/wc2max)+ymin - 2*dispcalib + 2;
    *vmax = mnumypnts - (wc2max-wc2-sc2)*dispcalib - 2;
}

void fillPlotBox() {
    double dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
    int vmin,vmax,x,y,w,h;
    double plot_dfpnt,plot_dnpnt, plot_dfpnt2, plot_dnpnt2;
    if(!showPlotBox()) return;

    getPlotBox_Pix(&plot_dfpnt, &plot_dfpnt2, &plot_dnpnt, &plot_dnpnt2); 

    vmin = plot_dfpnt2 - 2*dispcalib + 2;
    vmax = mnumypnts - (wc2max-wc2-sc2)*dispcalib - 2;

    x = plot_dfpnt;
    w = plot_dnpnt;
    y = mnumypnts - vmax;
    h = vmax - vmin + 2;
    set_background_region(x,y,w,h,BOX_BACK,100);
}

void fillPlotBox2D() {
    int x,y,w,h;
    double plot_dfpnt,plot_dnpnt, plot_dfpnt2, plot_dnpnt2;
    if(!showPlotBox()) return;

    getPlotBox_Pix(&plot_dfpnt, &plot_dfpnt2, &plot_dnpnt, &plot_dnpnt2); 

    x = plot_dfpnt;
    w = plot_dnpnt;
    y = mnumypnts - plot_dnpnt2 - plot_dfpnt2 - 1;
    h = plot_dnpnt2 + 1;
    set_background_region(x,y,w,h,BOX_BACK,100);
}

 // 1D plot box margin
double getPlotMargin() {
  double margin=0.0;
  if(plot || P_getreal(GLOBAL,"showPlotBox", &margin, 1) || margin < 1.0) 
	return 0.0;

  if(!P_getreal(GLOBAL,"showPlotBox", &margin, 2) ) {
    return margin;
  } return 0.0; 
}
