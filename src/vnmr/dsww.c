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
/*  dss	-	display a stacked spectra		*/
/*  dssa-	display stacked spectra with automatic	*/
/*		adjustment of chart positioning		*/
/*  dssh-	display spectra horizontally	 	*/
/*  pl	-	plot spectra				*/
/*  writespectrum - write spectra as binary		*/
/*							*/
/********************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "data.h"
#include "allocate.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "pvars.h"
#include "variables.h"
#include "wjunk.h"
#include "vnmrsys.h"
#include "dscale.h"
#include "init_display.h"
#include "buttons.h"

extern int VnmrJViewId;
extern int interuption;
extern int hires_ps;
extern int inRepaintMode;
extern void close_whitewash();    /* this procedure is in dcon.c */
extern void whitewash(struct ybar *out, int fpt, int npt); /* this procedure is in dcon.c */
extern int init_whitewash(int pts);    /* this procedure is in dcon.c */
extern void calc_hires_ps(float *ptr, double scale, int dfpnt, int depnt, int npnt,
           int vertical, int ypos, int maxv, int minv, int dcolor);
extern int  rel_spec();
extern int  plot_raster();
extern int colorindex(char *colorname, int *index);
extern void saveGraphFunc(char *argv[], int argc, int reexec, int redo);
extern void integ(register float  *fptr, register float  *tptr, register int npnt);
extern void getVLimit(int *vmin, int *vmax);
extern void drawPlotBox();
extern int get_drawVscale();
extern void set_dpf_flag(int b, char *cmd);
extern void set_dpir_flag(int b, char *cmd);
extern void clearMspec(); 
extern double getPlotMargin();
extern void setArraydis(int);
extern int aspFrame(char *, int, int, int, int, int);
extern int Mv(int argc, char *argv[], int retc, char *retv[]);

#define CALIB 		1000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1

int          intindex;
int          spec_index;
int          erase;
float       *spectrum;
float       *integral;
float        dispcalib;
float        scale;
int          ctrace;
int          updateflag;
int          intflag;
int          intresets;
int          regions;
int          resetflag;
short        horizontal;
short        automatic;
int          specmax;
int          specmin;
static int   pen_plot;
double dss_sc, dss_wc, dss_sc2, dss_wc2; 

#ifdef  DEBUG
extern int debug1;
#define DPRINT1(str, arg1) \
	if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#else 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#endif 

/* Following routine renamed so as to prevent confusion with routine
   of similar name and function (but not identical function) in ds.c	*/

/****************/
static void dsww_specdisp(int local_color, int do_ww )
/****************/
{ int ypos;
  struct ybar *outindex();

  ypos = dfpnt2 + (int)(dispcalib * vp);
  DPRINT3("mnumypts= %d ypos + specmax= %d ypos - specmin= %d\n",
                mnumypnts,ypos+specmax,ypos-specmin);
  if (plot && hires_ps && ! do_ww)
  {
        calc_hires_ps(spectrum+fpnt,vs * scale,dfpnt,dnpnt,npnt,0,ypos, 
           (ypos+specmax > mnumypnts - 3) ? mnumypnts - 3 : ypos+specmax,
           (ypos-specmin < 1) ? 1 : ypos-specmin,local_color);

  }
  else
  {
     int vmin = 1;
     int vmax = mnumypnts - 3;
     getVLimit(&vmin, &vmax);
     vmin = (ypos-specmin < vmin) ? vmin : ypos-specmin;
     vmax = (ypos+specmax > vmax) ? vmax : ypos+specmax;

     if ((pen_plot) && (dnpnt <= npnt))
        calc_plot_ybars(spectrum+fpnt,1,vs * scale,dfpnt,dnpnt,npnt,
             ypos,spec_index);
     else
        calc_ybars(spectrum+fpnt,1,vs * scale,dfpnt,dnpnt,npnt,
             ypos,spec_index);
     erase = 0;
     if (do_ww)
       whitewash(outindex(spec_index),dfpnt,dnpnt);
     displayspec(dfpnt,dnpnt,0,&spec_index,&spec_index,&erase,
	    vmax,vmin,local_color);
  }
}

/****************/
static void side_specdisp(int local_color)
/****************/
{
  int xoffset;

  xoffset = mnumxpnts - dfpnt + 8*xcharpixels + (int)(dispcalib * vp);
  if (plot && hires_ps)
  {
        calc_hires_ps(spectrum+fpnt,vs * scale,dfpnt2,dnpnt2,npnt,1,xoffset,
           (xoffset+specmax > mnumxpnts - 1) ? mnumxpnts - 1 : xoffset+specmax,
           (xoffset-specmin < xoffset-mnumxpnts/80+1) ?
              xoffset-mnumxpnts/80+1 : xoffset-specmin,local_color);

  }
  else
  {
     calc_ybars(spectrum+fpnt,1,vs * scale,dfpnt2,dnpnt2,npnt,
                xoffset,spec_index);
     erase = 0;
     displayspec(dfpnt2,dnpnt2,1,&spec_index,&spec_index,&erase,
           (xoffset+specmax > mnumxpnts - 1) ? mnumxpnts - 1 : xoffset+specmax,
           (xoffset-specmin < xoffset-mnumxpnts/80+1) ?
              xoffset-mnumxpnts/80+1 : xoffset-specmin,local_color);
  }
}

/****************/
static int intdisp(int local_color)
/****************/
{ extern struct ybar *outindex();
  struct ybar *out_ptr;
  double  vs1;
  double value;
  int    r;
  int    index;
  int    point;
  int    lastpt;
  float  factor;
  int    ypos;
  int vmin = 1;
  int vmax = mnumypnts - 3;

  if (!integral)
  {
    if ((integral =
        (float *) allocateWithId(sizeof(float) * fn / 2,"dss"))==0)
    {
      Werrprintf("cannot allocate integral buffer");
      return(ERROR);
    }
  }
  integ(spectrum,integral,fn / 2);
  vs1 = is * dispcalib / ( (double)fn / 128.0);

  int_ybars(integral,vs1,dfpnt,dnpnt,fpnt,npnt,
             dfpnt2 + (int)(dispcalib * (io + vp)),intindex,intresets,sw,fn);
  out_ptr = outindex(intindex) + dfpnt;
  if (intresets>1)
  {
    if (dnpnt > npnt)
      factor = (float) dnpnt / (float) (npnt -1);
    else 
      factor = (float) dnpnt / (float) npnt;
    lastpt = fpnt;
    index = 1;
    DPRINT3("fpnt=%d npnt=%d out_ptr=%x\n",fpnt,npnt,out_ptr);
    while ((index <= intresets) && (lastpt < fpnt + npnt - 1))
    {
      if ( (r=P_getreal(CURRENT,"lifrq",&value,index)) )
        value = 0.0;          /*  no resets defined  */
      point = datapoint(value,sw,fn/2);
      if (point>fpnt)
      {
        if (point>fpnt+npnt-1)
          point = fpnt + npnt -1;
        if (lastpt < fpnt)
          lastpt = fpnt;
        DPRINT2("start=%d num=%d \n",
               (int) (factor*(float)(lastpt-fpnt)),
               2 + (int) (factor * (float)(point-lastpt)));
        if ((regions) && (index % 2))
          clear_ybar(out_ptr + (int) (factor*(float)(lastpt-fpnt)),
                     2 + (int) (factor * (float)(point-lastpt)));
        else
          clear_ybar(out_ptr + (int) (factor*(float)(point-1-fpnt)),
                     2 + (int) factor);
      }
      index++;
      lastpt = point;
    }
  }
  erase = 0;
  ypos = dfpnt2 + (int)(dispcalib * vp);
  getVLimit(&vmin, &vmax);
  vmin = (ypos-specmin < vmin) ? vmin : ypos-specmin;
  vmax = (ypos+specmax > vmax) ? vmax : ypos+specmax;

  displayspec(dfpnt,dnpnt,0,&intindex,&intindex,&erase,
           vmax,vmin,local_color);
  return(COMPLETE);
}

/*************/
static void setwindows(int argc, char *argv[])
/*************/
{
  if ((argv[0][4]!='n') && (argv[0][3]!='n'))
    { 
#ifdef VNMRJ
      saveGraphFunc(argv, argc, 1, 1);  
#endif
      Wclear_graphics();
      clear_dscale();
      show_plotterbox();
    }
  else {
#ifdef VNMRJ
      saveGraphFunc(argv, argc, 0, 3);
#endif
  }
  Wgmode(); /* goto tek graphics and set screen 2 active */
  Wshow_graphics();
  drawPlotBox();
}

#define SPACING 15.0
// This is a replacement of setscws.
// new parameters dss_sc, dss_wc, dss_sc2, dss_wc2, instead of 
// sc,wc,sc2,wc2 are used to defined the size and location of traces.
static void setscwc_new(int first, int last, int step)
{ int range;
  range = (last - first + step)/step;

  if(P_getreal(CURRENT,"dss_sc",&dss_sc,1)) dss_sc = 0;
  if(P_getreal(CURRENT,"dss_wc",&dss_wc,1)) dss_wc = wc;
  if(P_getreal(CURRENT,"dss_sc2",&dss_sc2,1)) dss_sc2 = 0;
  if(P_getreal(CURRENT,"dss_wc2",&dss_wc2,1)) dss_wc2 = wc2;
  if((sc+wc) > wcmax) wc = wcmax - sc;
  if((sc2+wc2) > wc2max) wc2 = wc2max - sc2;
  if(dss_wc > wc) dss_wc = wc;
  if(dss_wc2 > wc2) dss_wc2 = wc2;
  if((dss_sc+dss_wc) > wc) dss_sc = wc - dss_wc;
  if((dss_sc2+dss_wc2) > wc2) dss_sc2 = wc2 - dss_wc2;

  if (vp > wc2max - 5.0)
  {
    vp = wc2max - 5.0;
    P_setreal(CURRENT,"vp",vp,0);
  }
  if (automatic && (d2flag) && (first != last))
  { 
    // auto display 2D data as 1D array
    // set vp, vo, ho, and calculate dss_sc, dss_wc
    vp = 0.0;
    vo = wc2 / (double) range;
    if (dss_sc + dss_wc + ho > wc)
      ho = wc - dss_sc - dss_wc;
    else if (dss_sc + ho < 0.0)
      ho = -dss_sc;
    ho /= (double) range - 1.0;
    if(ho < 0) {
        dss_sc = -(double)(range - 1) * ho;
        dss_wc = wc - dss_sc; 
    } else {
        dss_sc = 0;
        dss_wc = wc - (double)(range - 1) * ho; 
    }
  }
  else if (first != last)
  { // redisplay after vo, ho, dss_sc or dss_wc is changed

    if (vp + (double) range * vo > wc2)
      vo = (wc2 - vp)/(double) range;
    else if(vp + (double) range * vo < 0) 
      vo = - vp/(double) range; 

    if(dss_sc < 0) dss_sc = 0;
    if(dss_wc < 5) dss_wc = 5;
    else if(dss_wc > wc) dss_wc = wc;

    if (dss_sc + (double)(range - 1) * ho < 0.0) {
      double tmp = -(double)(range - 1) * ho;
      if(tmp >= 0) dss_sc = tmp; 
      else ho = - dss_sc/(double)(range -1);
    }
    else if (dss_sc + dss_wc + (double)(range - 1) * ho > wc) {
      double tmp = wc - dss_wc - (double)(range - 1) * ho;
      if(dss_sc > 0 && tmp >=0) dss_sc = tmp;
      else ho = (wc - dss_sc -dss_wc)/(double)(range -1);
    }
    if(dss_sc + dss_wc > wc) {
	dss_sc = wc - dss_wc;
        if(ho < 0) ho = - dss_sc/(double)(range -1); 
	else ho = (wc - dss_sc -dss_wc)/(double)(range -1);
    }
  }
  if (first != last)
  {
    if (automatic) // dssa
    {
      vo = (wc2 - vp)/ (double) range;
      ho = -dss_sc / (double) (range -1);
    }
    else if (horizontal) // dssh
    {
      dss_wc = (wc - SPACING) / (double) range;
      dss_sc = wc - dss_wc;
      ho = -dss_wc - SPACING/ (double) (range -1);
      vo = 0.0;
    }
  }
  if (first != last)
  {
    if (d2flag&&automatic)
    {
      P_setreal(CURRENT,"ho",ho * (double)(range -1),0);
      P_setreal(CURRENT,"vo",vo,0);
    }
    else
    {
      P_setreal(CURRENT,"ho",ho,0);
      P_setreal(CURRENT,"vo",vo,0);
    }
  }
  P_setreal(CURRENT,"dss_sc",dss_sc,0);
  P_setreal(CURRENT,"dss_wc",dss_wc,0);
}

static void setscwc(int first, int last, int step)
{ int range;
  range = (last - first + step)/step;
  if (vp > wc2max - 5.0)
  {
    vp = wc2max - 5.0;
    P_setreal(CURRENT,"vp",vp,0);
  }
  if (automatic && (d2flag) && (first != last))
  {
    vp = 0.0;
    vo = wc2 / (double) range;
    if (sc + wc + ho > wcmax)
      ho = wcmax - sc -wc;
    else if (sc + ho < 0.0)
      ho = -sc;
    ho /= (double) range - 1.0;
  }
  else if (first != last)
  {
    if (vp + (double) range * vo > wc2max)
      vo = (wc2max - vp)/(double) range;
    else if ((int) (dispcalib * (vp + (double) range * vo)) + dfpnt2 < 0)
      vo = -(vp + (double) dfpnt2 / dispcalib) / (double) range;
    if (sc + wc + (double)(range - 1) * ho > wcmax)
      ho = (wcmax - sc -wc)/(double)(range -1);
    else if (sc + (double)(range - 1) * ho < 0.0)
      ho = - sc/(double)(range -1);
  }
  if (first != last)
  {
    if (automatic)
    {
      vo = (wc2max - vp)/ (double) range;
      ho = -sc / (double) (range -1);
    }
    else if (horizontal)
    {
      // wc = (wcmax - SPACING) / (double) range;
      // sc = wcmax - wc;
      ho = (wcmax - wc) / 2.0;
      if (ho < 0.0)
          ho = 0.0;
      wc = (wc - SPACING) / (double) range;
      sc = wcmax - wc - ho;
      P_setreal(CURRENT,"sc",sc,0);
      P_setreal(CURRENT,"wc",wc,0);
      ho = -wc - SPACING/ (double) (range -1);
      vo = 0.0;
    }
  }
  if (first != last)
  {
    if (d2flag&&automatic)
    {
      P_setreal(CURRENT,"ho",ho * (double)(range -1),0);
      P_setreal(CURRENT,"vo",vo,0);
    }
    else
    {
      P_setreal(CURRENT,"ho",ho,0);
      P_setreal(CURRENT,"vo",vo,0);
    }
  }
}

/****************/
static int check_int()
/****************/
/* set intflag, regions, and number of integral resets */
{ int    res;
  vInfo  info;
  char   intmod[8];
  
  intresets = ((P_getVarInfo(CURRENT,"lifrq",&info)) ? 1 : info.size);
  if ( (res=P_getstring(CURRENT,"intmod",intmod,1,8)) )
  { P_err(res,"intmod",":"); return(ERROR); }
  intflag = (strcmp(intmod,INT_OFF) != 0); 
  if (intflag)
  {
    regions = (strcmp(intmod,INT_PARTIAL) == 0); 
    res=P_setactive(CURRENT,"lifrq",(regions) ? ACT_ON : ACT_OFF);
  }
  else
    regions = ((P_getVarInfo(CURRENT,"lifrq",&info)) ? FALSE : info.active);
  return(COMPLETE);
}

/********************/
static int setspecmaxmin()
/********************/
{
  vInfo  info;
  double cutoff;

  specmax = mnumypnts - 3;
  specmin = mnumypnts - 3;
  if (P_getVarInfo(CURRENT,"cutoff",&info))
    return(COMPLETE);
  if (info.active == ACT_OFF)
    return(COMPLETE);
  P_getreal(CURRENT,"cutoff",&cutoff,1);
  if (cutoff < 2.0)
    cutoff = 2.0;
  if (info.size == 2)
  {
    specmax = (int) (dispcalib * cutoff);
    P_getreal(CURRENT,"cutoff",&cutoff,2);
    if (cutoff < 0.0)
      cutoff = 0.0;
    specmin = (int) (dispcalib * cutoff);
  }
  else if (info.size == 1)
  {
    specmax = (int) (dispcalib * cutoff);
    specmin = (int) (dispcalib * cutoff);
  }
  if (specmax > mnumypnts - 3)
    specmax = mnumypnts - 3;
  if (specmin < 0)
    specmin = 0;
  return(COMPLETE);
}

/*************/
static void checkinput(argc,argv,firstindex,lastindex,step,selected,top,side,do_dc,int_on,showPar,color_traces)
/*************/
int argc,*firstindex,*lastindex,*step,*selected,*top,*side,*do_dc,*int_on,*showPar,*color_traces;
char *argv[];
{ int arg_no;
  int maxindex;

  *step = 1;
  arg_no = 1;
  *top = *side = *do_dc = *int_on = *color_traces=FALSE;
  *showPar = 1;
  // sc wc will be set by setscwc or setscwc_new, which changes sc, wc only if
  // horizontal=1. So when horizontal=0, sc wc won't be changed.
  // that's why after dssh, dss will be the same as dssh (until sc wc are directly changed).
  // pl will never set horizontal=1, so for plot, dssh has to be executed first,
  // then keep horizontal=1. 
  
  automatic = 0;
  horizontal = 0;
  if(strlen(argv[0]) > 3) {
    horizontal = (argv[0][3] == 'h'); 
    automatic = (argv[0][3] == 'a');
  }

  maxindex = nblocks * specperblock;

  if (plot && (argv[0][2] != 'w'))
  {
    *firstindex = specIndex;
    *lastindex = specIndex;
  }
  else if(d2flag) 
  {
    *firstindex = fpnt1;
    *lastindex = fpnt1 + npnt1;
  }
  else
  {
    *firstindex = 1;
    *lastindex = maxindex;
// set firstindex, lastindex, step according to parameters
/*
    if(!P_getreal(CURRENT,"arraystart", &d, 1)) *firstindex = d;
    if(!P_getreal(CURRENT,"arraystop", &d, 1)) *lastindex = d;
    if(!P_getreal(CURRENT,"arraydelta", &d, 1)) *step = d;
*/
  }

  while (argc>arg_no)
  {
    if (isReal(argv[arg_no]))
    {
      *step = 1;
      *lastindex = *firstindex = (int) stringReal(argv[arg_no]);
      arg_no++;
      if (argc>arg_no)
        if (isReal(argv[arg_no]))
        {
          *lastindex = (int) stringReal(argv[arg_no]);
          arg_no++;
        }
      if (argc>arg_no)
      {
        if (isReal(argv[arg_no]))
        {
          *step = (int) stringReal(argv[arg_no]);
          arg_no++;
        }
      }
    }
    else if (strcmp(argv[arg_no],"all")==0)
    {
      *step = 1;
      *firstindex = 1;
      *lastindex  = maxindex;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"top")==0)
    {
      *top = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"side")==0)
    {
      *side = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"dodc")==0)
    {
      *do_dc = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"color")==0)
    {
      *color_traces = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"int")==0)
    {
      *int_on = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"select")==0)
    {
      *selected = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"nopars")==0)
    {
      *showPar = FALSE;
      arg_no++;
    }
    else
      arg_no++;
  }
  if (*firstindex < 1)
    *firstindex = 1;
  if (*step < 1)
    *step = 1;
  if (*lastindex < *firstindex)
    *lastindex = *firstindex;
}

/*******************************************************/
static void getcolor(argc,argv,plotting,spec_color,int_color,int_on)
/*******************************************************/
int   argc,plotting,int_on;
char *argv[];
int  *spec_color,
     *int_color;
{
   int argnum, col, found;

   *spec_color = SPEC_COLOR;
   *int_color = INT_COLOR;
   argnum = 1;
   found = FALSE;
   while ((argc>argnum) && (!found))
      found = colorindex(argv[argnum++],&col);
   if (found)
   {
       *spec_color = col;
   }
   found = FALSE;
   while ((argc>argnum) && (!found))
      found = colorindex(argv[argnum++],&col);
   if (found)
   {
       *int_color = col;
   }
}

void calcDisplayPars() {
  char tmpStr[16];
  double hzpp, hzpp1;

  dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-dss_wc-dss_sc)/wcmax);
  dnpnt  = (int)((double)(mnumxpnts-right_edge)*dss_wc/wcmax);
  dfpnt2 = (int)((double)(mnumypnts-ymin)*(sc2+dss_sc2)/wc2max)+ymin;
  dnpnt2 = (int)((double)(mnumypnts-ymin)*dss_wc2/wc2max);
  if (d2flag != 0) {
         Wgetgraphicsdisplay(tmpStr, 16);
         if (strncmp(tmpStr,"ds", 2) != 0) {
            int west = xcharpixels * 6;
            if (west > dfpnt && west < dnpnt) {
               west = west - dfpnt;
               dfpnt += west;
               dnpnt -= west;
            }
         }
  }
  if(!d2flag && get_drawVscale()) {
     if(dfpnt < 6*xcharpixels) {
        dnpnt=dnpnt-dfpnt+6*xcharpixels;
        dfpnt=6*xcharpixels;
     }
  }
  if (dnpnt2 < 1)
    dnpnt2 = 1;

  if (dfpnt < 1)
    dfpnt = 1;
  if (dnpnt < 1)
    dnpnt = 1;
  if ((dfpnt + dnpnt) >= (mnumxpnts - right_edge - 2))
  {
    dnpnt = mnumxpnts - right_edge - 2 - dfpnt;
    if (dnpnt < 1)
    {
      dnpnt = 1;
      dfpnt = mnumxpnts - right_edge - 2 - dnpnt;
    }
  }

  hzpp = sw/((double)(fn/2));
  npnt = (int)(wp/hzpp + 0.01) + 1;
  if(npnt<1) npnt = 1;

  if (get_axis_freq(HORIZ))
  {
     fpnt = (int)((sw-sp-rflrfp-wp)/hzpp + 0.01);
  }
  else
  {
     fpnt = (int)(sp/hzpp + 0.01);
  }

  if (d2flag)
  {
     hzpp1 = sw1/((double)(fn1/2));
     npnt1 = (int)(wp1/hzpp1 + 0.01);
     if(npnt1<1) npnt1 = 1;

     if (get_axis_freq(VERT))
        fpnt1 = (int)((sw1-sp1-rflrfp1-wp1)/hzpp1 + 0.01);
     else
        fpnt1 = (int)(sp1/hzpp1 + 0.01);
  }
}

/***********************/
int dsww(int argc, char *argv[], int retc, char *retv[])
/***********************/
{ int firstindex;
  int lastindex;
  int step;
  int selected = 0;
  int top;
  int side;
  int do_dc;
  int do_ww;
  int int_on;
  int int_color;
  int spec_color;
  int showPar;
  int color_traces;
  int dsSelectIndex = 1;
  int index;
  int redoDscale;
  int maxindex;
  double save_sc, save_wc, save_sc2, save_wc2;
  double selVal, d;
  int dssnflg = (strlen(argv[0])==5 && argv[0][4] == 'n');
  if(!dssnflg) dssnflg = (strlen(argv[0])==4 && argv[0][3] == 'n');

  if(retc > 1) {
	retv[0] = realString((double)nblocks);
	retv[1] = realString((double)specperblock);
	RETURN;	
  } else if(retc > 0) {
	retv[0] = realString((double)nblocks);
	RETURN;	
  }

  plot = (argv[0][0] == 'p');
  do_ww = (argv[0][2] == 'w');
  redoDscale = dscale_onscreen();
  if(!plot) { 
    set_dpf_flag(0,"");
    set_dpir_flag(0,"");
  }
  clearMspec();
  aspFrame("clearAspSpec",0,0,0,0,0);

/*  Leave the buttons active if plotting so ds stays active.  */

  if (!plot)
    Wturnoff_buttons();

  hires_ps = 0;
  if(init2d(1,plot+1)) return(ERROR);
  maxindex=nblocks * specperblock;
  checkinput(argc,argv,&firstindex,&lastindex,&step,&selected,&top,&side,&do_dc,&int_on, &showPar, &color_traces);
  if (!plot)
    setwindows(argc, argv);
  if ((firstindex < 1) || (firstindex >maxindex))
  {
    Werrprintf("spectrum %d does not exist",firstindex);
    return(ERROR);
  }
  pen_plot = plot;
  if (pen_plot)
     pen_plot = (!plot_raster());
  if (lastindex < firstindex)
    lastindex = firstindex;
  if (lastindex > maxindex)
    lastindex = maxindex;
  if (lastindex != firstindex)
    top = side = FALSE;
  if (selected)
  {
    top = side = FALSE;
    step = 1;
    firstindex = 1;
    lastindex = P_getsize(CURRENT,"dsSelect",NULL);
    if (lastindex <= 0)
    {
      selected = 0;
      lastindex = 1;
    }
  }
  else
  {
     int e;
     e = P_getsize(CURRENT,"dsSelect",NULL);
     if ( e <= 0)
     {
        P_creatvar(CURRENT,"dsSelect",ST_INTEGER);
        P_setgroup(CURRENT,"dsSelect", G_DISPLAY);
     }
     P_setreal(CURRENT,"dsSelect",(double) firstindex,0);
  }
  DPRINT1("current trace= %d\n",firstindex);
  integral = 0;

// now set arraystart, arraystop, arraydelta parameters
  if(!P_getreal(CURRENT,"arraystart",&d,1) && firstindex > (int)d) {
  	P_setreal(CURRENT,"arraystart", (double)firstindex, 1);
#ifdef VNMRJ
        appendJvarlist("arraystart");
  	// writelineToVnmrJ("pnew", "1 arraystart");
#endif
  }
  if(!P_getreal(CURRENT,"arraystop",&d,1) && lastindex < (int)d) {
  	P_setreal(CURRENT,"arraystop", (double)lastindex, 1);
#ifdef VNMRJ
        appendJvarlist("arraystop");
  	// writelineToVnmrJ("pnew", "1 arraystop");
#endif
  }
  if(!P_getreal(CURRENT,"arraydelta",&d,1) && step != (int)d) {
  	P_setreal(CURRENT,"arraydelta", (double)step, 1);
#ifdef VNMRJ
        appendJvarlist("arraydelta");
  	//  writelineToVnmrJ("pnew", "1 arraydelta");
#endif
  }

/*  spec_index is the index into the y-bar buffers for the spectrum;
    intindex is the index for the integral.  The pl/dsww program
    finishes with the spectrum before working with the integral, so
    both spectrum and integral can use the same y-bar buffer.  Avoid
    conflict with ds program which uses y-bar buffers 0, 1 and 2.	*/

  spec_index = 3;
  intindex  = 3;

/*  The following call to init_display is removed.  The init_display
    program deletes the y-bar buffers.  If plotting, we need to keep
    these y-bar buffers for the ds program.  If not plotting, exit_display
    will be called later (see below) and this command will become the
    current graphics command.  In that case ds is expected to start over
    and make new y-bar buffers, not relying on old y-bar buffers.	*/

/*init_display();*/

    save_sc=sc;
    save_wc=wc;
    save_sc2=sc2;
    save_wc2=wc2;

  if (side)
    dispcalib = (float) (mnumxpnts-right_edge) / (float) wcmax;
  else
    dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  if (top)
    vp += wc2 + 10;
  else if (!side) {
    if(plot && firstindex==lastindex) {
      setscwc(firstindex,lastindex,step);
      setArraydis(0);

    } else { 
      setscwc_new(firstindex,lastindex,step);
      setArraydis(1);
    }
  }
  setspecmaxmin();
  if (check_int()) return(ERROR);
  intflag = (intflag && (firstindex == lastindex));
  if (selected)
  {
     step = 1;
     P_getreal(CURRENT,"dsSelect", &selVal, step);
     firstindex = (int) (selVal + 0.1);
     if (firstindex < 1)
        firstindex = 1;
     if (firstindex > maxindex)
       firstindex = maxindex;
  }

  if ( ! plot )
  {
     specIndex = firstindex;
     if (inRepaintMode && redoDscale)
       new_dscale(FALSE,TRUE);
  }
  disp_specIndex(firstindex);

/*  Need to change this for multiple traces...  */

  getcolor(argc,argv,plot,&spec_color,&int_color,int_on);
  ctrace = index = firstindex;
  if (do_ww)
    if (init_whitewash(mnumxpnts))
      do_ww = FALSE;
  while ( ((!selected && (ctrace <= lastindex)) ||
           (selected && (step <= lastindex))) && !interuption)
  {
    if(plot && firstindex==lastindex)
      exp_factors(TRUE);
    else 
      calcDisplayPars();
    if(color_traces && maxindex>1)
    	spec_color=((index % (NUM_AV_COLORS-2)) + FIRST_AV_COLOR + 1);
    if ((spectrum = calc_spec(ctrace-1,0,do_dc,TRUE,&updateflag))==0)
      return(ERROR);
    scale = dispcalib;
    if (normflag)
      scale *= normalize;
    if (!int_on)
    {
      if (side)
        side_specdisp(spec_color);
      else
        dsww_specdisp( spec_color, do_ww );
    }
    if ((intflag || int_on) && !side)
      intdisp(int_color);
    if(plot) {
      sc += ho;
      if (!horizontal)
       sc2 += vo;
    } else {
      dss_sc += ho;
      if (!horizontal)
       dss_sc2 += vo;
    } 
    if (selected)
    {
       step++;
       if ( ! P_getreal(CURRENT,"dsSelect", &selVal, step))
       {
          ctrace = (int) (selVal + 0.1);
          if (ctrace < 1)
             ctrace = 1;
          if (ctrace > maxindex)
            ctrace = maxindex;
       }
    }
    else
    {
       P_setreal(CURRENT,"dsSelect", (double) ctrace, dsSelectIndex);
       ctrace += step;
       dsSelectIndex++;
    }
    index++;
  }
  if (!plot && showPar)
  {
    ResetLabels();
    DispField(FIELD1,PARAM_COLOR,"vs",vs,1);
    InitVal(FIELD2,HORIZ,SP_NAME, PARAM_COLOR,UNIT4,
                         SP_NAME, PARAM_COLOR,SCALED,2);
    InitVal(FIELD3,HORIZ,WP_NAME, PARAM_COLOR,UNIT4,
                         WP_NAME, PARAM_COLOR,SCALED,2);
    if ( (firstindex != lastindex) && !selected)
    {
      DispField(FIELD4,PARAM_COLOR,"first",(double) firstindex,0);
      DispField(FIELD5,PARAM_COLOR,"last",(double) lastindex,0);
      DispField(FIELD6,PARAM_COLOR,"step",(double) step,0);
    }
  }
/*  exit_display();  Taken out.  See comments concerning init_display.  */
  if (plot)
    amove(0,0);
  else
  {
    char cmd[MAXSTR];
    sprintf(cmd,"dataInfo %d %d %s",VnmrJViewId,1,"dss");
    writelineToVnmrJ("vnmrjcmd",cmd);

    if(dssnflg) Wsetgraphicsdisplay("dssn");
    else Wsetgraphicsdisplay("dss");
    exit_display();
  }
  endgraphics();

    sc=save_sc;
    wc=save_wc;
    sc2=save_sc2;
    wc2=save_wc2;
    P_setreal(CURRENT,"sc",sc,0);
    P_setreal(CURRENT,"wc",wc,0);
    P_setreal(CURRENT,"sc2",sc2,0);
    P_setreal(CURRENT,"wc2",wc2,0);
    // reset dfpnt,dnpnt,dfpnt2,dnpnt2
    if(plot) exp_factors(FALSE);
    else exp_factors(TRUE);

  if (do_ww)
    close_whitewash();
  if (integral)
    release(integral);
  integral = 0;
  appendvarlist("dss_sc,dss_wc,sc,wc,vo,ho");
  aspFrame("ds",0,0,0,0,0);
  return(rel_spec());
}

/***********************/
int writespectrum(int argc, char *argv[], int retc, char *retv[])
/***********************/
{
   int dummy, specI;
   char filename[MAXSTR];
   char tmpFilename[MAXSTR];
   char *argv2[4];
   float *ptr;
   int i;
   int ibuf;
   float fbuf;
   int fd;
   int doAll = 0;
   float scaling = 0.0;
   int intType = 0;
   char traceVal[16];
   int f2 = 1;
   int doMove = 0;

   if(init2d(1,1)) return(ERROR);
   if (argc >=2)
      sprintf(filename,"%s",argv[1]);
   else
      sprintf(filename,"%s/spec%d",curexpdir,specIndex);
   unlink(filename);
   for (i=2; i<argc; i++)
   {
      if (isReal(argv[i]))
         scaling = (float) stringReal(argv[i]);
      else if ( ! strcmp(argv[i],"all") )
         doAll = 1;
      else if ( ! strcmp(argv[i],"int") )
         intType = 1;
      else if ( ! strcmp(argv[i],"float") )
         intType = 0;
      else if ( ! strcmp(argv[i],"f1") )
         f2 = 0;
      else if ( ! strcmp(argv[i],"f2") )
         f2 = 1;
   }
   if (d2flag)
   {
      P_getstring(CURRENT,"trace", traceVal, 1, 15);
      if ( (f2==1) && strcmp(traceVal,"f2") )
      {
         P_setstring(CURRENT,"trace", "f2", 1);
         if(init2d(1,1)) return(ERROR);
      }
      else if ( (f2==0) && strcmp(traceVal,"f1") )
      {
         P_setstring(CURRENT,"trace", "f1", 1);
         if(init2d(1,1)) return(ERROR);
      }
   }
   strcpy(tmpFilename,"/dev/shm");
   if ( ! access(tmpFilename,W_OK) )
   {
      sprintf(tmpFilename,"/dev/shm/spec%d", (int) getpid());
      unlink(tmpFilename);
      doMove = 1;
      argv2[0] = "Mv";
      argv2[1] = tmpFilename;
      argv2[2] = filename;
      argv2[3] = NULL;
   }
   else
   {
      strcpy(tmpFilename,filename);
   }
   fd = open(tmpFilename, ( O_CREAT | O_WRONLY ), 0666 );
   if ( doAll )
   {
      for (specI = 0; specI < nblocks * specperblock; specI++)
      {
         if ((spectrum = calc_spec(specI,0,FALSE,TRUE,&dummy))==0)
         {
            close(fd);
            return(ERROR);
         }
         scale = vs;
         if (normflag)
            scale *= normalize;
         if (scaling != 0.0)
            scale = scaling;
         ptr = spectrum;
         if (intType)
         {
            for (i=0; i<fn/2; i++)
            {
               ibuf = (int) (*ptr * scale);
               ptr++;
               write(fd, &ibuf , sizeof(int) );
            }
         }
         else
         {
            for (i=0; i<fn/2; i++)
            {
               fbuf = *ptr * scale;
               ptr++;
               write(fd, &fbuf , sizeof(float) );
            }
         }
         rel_spec();
      }
      close(fd);
      if (doMove)
         Mv(3,argv2,0,NULL);
      RETURN;
   }
   else
   {
      if ((specIndex < 1) || (specIndex > nblocks * specperblock))
      {
         Werrprintf("spectrum %d does not exist",specIndex);
         close(fd);
         return(ERROR);
      }
      if ((spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&dummy))==0)
      {
         close(fd);
         return(ERROR);
      }
      scale = vs;
      if (normflag)
         scale *= normalize;
      if (scaling != 0.0)
         scale = scaling;
      ptr = spectrum;
      if (intType)
      {
         for (i=0; i<fn/2; i++)
         {
            ibuf = (int) (*ptr * scale);
            ptr++;
            write(fd, &ibuf , sizeof(int) );
         }
      }
      else
      {
         for (i=0; i<fn/2; i++)
         {
            fbuf = *ptr * scale;
            ptr++;
            write(fd, &fbuf , sizeof(float) );
         }
      }
      close(fd);
      if (doMove)
         Mv(3,argv2,0,NULL);
      return(rel_spec());
   }
}

