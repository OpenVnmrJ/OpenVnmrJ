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
/*  dfs	-	display stacked fid			*/
/*  dfsa-	display stacked fid with automatic	*/
/*		adjustment of chart positioning		*/
/*  dfsh-	display fid horizontally	 	*/
/*  plfid-	plot fid				*/
/*							*/
/********************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"
#include "dscale.h"
#include "buttons.h"
#include "init_display.h"
#include "pvars.h"
#include "wjunk.h"
#include "vnmrsys.h"

extern int debug1;
extern int Bnmr;  /* background flag */

#define CALIB 		40000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1

#define MAXMIN(val,max,min) \
  if (val > max) val = max; \
  else if (val < min) val = min

/*static int     spec1,spec2,next;*/
static int     spec_index;
static float  *spectrum;
static float   dispcalib;
static float   scale;
static int     dotflag;

extern int interuption;
extern int inRepaintMode;
extern int init_whitewash(int pts);    /* this procedure is in dcon.c */
extern void close_whitewash();    /* this procedure is in dcon.c */
extern void whitewash(struct ybar *out, int fpt, int npt); /* this procedure is in dcon.c */
extern struct ybar *outindex(int index);
extern void exit_display();
extern int colorindex(char *colorname, int *index);
extern void maxfloat(register float  *datapntr, register int npnt, register float  *max);
extern void getVLimit(int *vmin, int *vmax);
extern void drawPlotBox();
extern int get_drawVscale();
extern void clearMspec();
extern void calcDisplayPars();
extern double getPlotMargin();
extern double dss_sc, dss_wc, dss_sc2, dss_wc2;
extern void saveGraphFunc(char *argv[], int argc, int reexec, int redo);
extern float *get_one_fid(int curfid, int *np, dpointers *c_block, int dcflag);

static short   horizontal;
static short   automatic;
static int     specmax;
static int     specmin;

/****************/
static int imagdisp(int local_color, int do_ww)
/****************/
{
  int erase;
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);

  erase = 0;
  fid_ybars(spectrum+(fpnt * 2)+1,(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vpi + wc2/2.0)),spec_index,dotflag);
  if (do_ww)
    whitewash(outindex(spec_index),dfpnt,dnpnt);
  displayspec(dfpnt,dnpnt,0,&spec_index,&spec_index,&erase,vmax,vmin,local_color);
  return(COMPLETE);
}

/****************/
static int freebuffers()
/****************/
{ int res;

  if(c_buffer>=0) /* release last used block */
    if ( (res=D_release(D_PHASFILE,c_buffer)) )
    {
      D_error(res); 
      D_close(D_PHASFILE);  
      return(ERROR);
    }
  return(COMPLETE);
}

/*************/
static void setwindows(char *argv[])
/*************/
{
  if ((argv[0][4]!='n') && (argv[0][3]!='n'))
    { 
#ifdef VNMRJ
      saveGraphFunc(argv, 1, 1, 1);
#endif
      Wclear_graphics();
      clear_dscale();
      show_plotterbox();
    } else {
#ifdef VNMRJ
      saveGraphFunc(argv, 1, 0, 3);
#endif
    }
  Wgmode(); /* goto tek graphics and set screen 2 active */
  Wshow_graphics();
  drawPlotBox();
}

/*************/
static int init_vars2()
/*************/
{
  char flag[16];

/*spec1  = 0;
  spec2  = 1;
  next   = 2;*/
  spec_index = 3;
  scale = 0.0;
/*init_display();*/
  dotflag = TRUE;
  if (P_getstring(CURRENT,"dotflag" ,flag, 1,16) == 0) 
      dotflag = (flag[0] != 'n');
  return(COMPLETE);
}

/****************/
static int calc_fid(int trace)
/****************/
{ float datamax;

  if (debug1) Wscrprintf("function calc_fid\n");
  if ((spectrum = get_one_fid(trace,&fn,&c_block, FALSE)) == 0) return(ERROR);
  c_buffer = trace;
  c_first  = trace;
  c_last   = trace;
  if (normflag)
  {
    maxfloat(spectrum,pointsperspec / 2, &datamax);
    scale = (float) dispcalib / datamax;
    if (debug1)
      Wscrprintf("datamax=%g ", datamax);
  }
  else
    scale = (float) dispcalib / CALIB;
  if (debug1)
    Wscrprintf("normflag= %d scale=%g\n", normflag, scale);
  return(COMPLETE);
}

/****************/
static int fiddisp(int local_color, int do_ww)
/****************/
{
  int erase;
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);

  if (debug1)
  {
    Wscrprintf("starting fid display\n");
    Wscrprintf("fpnt=%d, npnt=%d, mnumxpnts=%d, dispcalib=%g\n",
            fpnt,npnt,mnumxpnts,dispcalib);
  }
  erase = 0;
  fid_ybars(spectrum+(fpnt * 2),(double) (vs * scale),dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * (vp + wc2/2.0)),spec_index,dotflag);
  if (do_ww)
    whitewash(outindex(spec_index),dfpnt,dnpnt);
  displayspec(dfpnt,dnpnt,0,&spec_index,&spec_index,&erase,vmax,vmin,local_color);
  return(COMPLETE);
}

/*************************************************************/
/*************************************************************/
/*************************************************************/

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
    vo = dss_wc2 / (double) range;
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
        dss_wc = wc;
    }
  }
  else if (first != last)
  { // redisplay after vo, ho, dss_sc or dss_wc is changed
/*
    if (vp + (double) range * vo > wc2)
      vo = (wc2 - vp)/(double) range;
    else if(vp + (double) range * vo < 0)
      vo = - vp/(double) range;
*/
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
      if(dss_sc + dss_wc > wc) dss_sc = wc - dss_wc;
      vo = (wc2/2.0 - vp)/ (double) range;
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
    P_setreal(CURRENT,"vpf",vp,0);
  }
  if ((d2flag) && (first != last))
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

//  Decided not to adjust vo unless specifically requested
//
//    if (vp + (double) range * vo > wc2max)
//      vo = (wc2max - vp)/(double) range;
//    else if ((int) (dispcalib * (vp + wc2/2.0 + (double) range * vo)) + dfpnt2 < 0)
//      vo = -(vp + wc2/2.0 + (double) dfpnt2 / dispcalib) / (double) range;

    if (sc + wc + (double)(range - 1) * ho > wcmax)
      ho = (wcmax - sc -wc)/(double)(range -1);
    else if (sc + (double)(range - 1) * ho < 0.0)
      ho = - sc/(double)(range -1);
  }
  if (first != last)
  {
    if (automatic)
    {
      vo = (wc2max/2.0 - vp)/ (double) range;
      ho = -sc / (double) (range -1);
    }
    else if (horizontal)
    {
      wc = (wcmax - SPACING) / (double) range;
      sc = wcmax - wc;
      P_setreal(CURRENT,"sc",sc,0);
      P_setreal(CURRENT,"wc",wc,0);
      ho = -wc - SPACING/ (double) (range -1);
      vo = 0.0;
    }
  }
  if (first != last)
  {
    if (d2flag)
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
static int checkinput(int argc, char *argv[],
                      int *firstindex,int *lastindex,int *step,int *imag_on,int *color_traces)
/*************/
{ int arg_no;
  int maxindex;

  *step = 1;
  arg_no = 1;
  *imag_on = *color_traces = FALSE;
  horizontal = 0;
  automatic =  0;
  if(strlen(argv[0])>3) {
    horizontal = (argv[0][3] == 'h');
    automatic =  (argv[0][3] == 'a');
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
  }

  while (argc>arg_no)
  {
    if (isReal(argv[arg_no]))
    {
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
      *firstindex = 1;
      *lastindex  = maxindex;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"imag")==0)
    {
      *imag_on = TRUE;
      arg_no++;
    }
    else if (strcmp(argv[arg_no],"color")==0)
    {
      *color_traces = TRUE;
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
  return(COMPLETE);
}

/*******************************************************/
static void get_fid_color(int argc, char *argv[], int *disp_color, int imag_on)
/*******************************************************/
{
   int argnum, col, found;

   *disp_color = (imag_on) ? IMAG_COLOR : FID_COLOR;
   argnum = 1;
   found = FALSE;
   while ((argc>argnum) && (!found))
      found = colorindex(argv[argnum++],&col);
   if (found)
   {
      *disp_color = col;
   }
}

/*************/
int dfww(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int firstindex;
  int lastindex;
  int step;
  int ctrace;
  int do_ww;
  int local_color;
  int imag_on;
  int redoDscale;
  int color_traces;
  int index=1;
  int maxindex=nblocks * specperblock;
  double save_sc, save_wc, save_sc2, save_wc2;

  (void) retc;
  (void) retv;
  redoDscale = dscale_onscreen();
  Wturnoff_buttons();
  plot = (argv[0][0] == 'p');
  do_ww = (argv[0][2] == 'w');
  if (Bnmr && !plot)
    return(COMPLETE);
  revflag = 0;
  if(initfid(plot+1)) return(ERROR);
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  if (init_vars2()) return(ERROR);
  clearMspec();

  checkinput(argc,argv,&firstindex,&lastindex,&step,&imag_on,&color_traces);
  if (!plot)
    setwindows(argv);
  if ((firstindex < 1) || (firstindex > maxindex))
  {
    Werrprintf("spectrum %d does not exist",firstindex);
    return(ERROR);
  }

    save_sc=sc;
    save_wc=wc;
    save_sc2=sc2;
    save_wc2=wc2;

  if (lastindex < firstindex)
    lastindex = firstindex;
  if (lastindex > maxindex)
    lastindex = maxindex;
  //setscwc(firstindex,lastindex,step);
  if(plot) setscwc(firstindex,lastindex,step);
  else setscwc_new(firstindex,lastindex,step);
  setspecmaxmin();
  disp_specIndex(firstindex);

/*  Need to change this for multiple traces...  */

  get_fid_color(argc,argv,&local_color,imag_on);
  ctrace = index = firstindex;
  if (!plot && inRepaintMode && redoDscale)
     new_dscale(FALSE,TRUE);
  if (do_ww)
    if (init_whitewash(mnumxpnts))
      do_ww = FALSE;
  while ((ctrace <= lastindex) && !interuption)
  {
    if(plot) exp_factors(FALSE);
    else calcDisplayPars();
    if(color_traces)
    	local_color=((index % (NUM_AV_COLORS-2)) + FIRST_AV_COLOR + 1);

    if (calc_fid(ctrace-1))
      return(ERROR);
    if (imag_on)
      imagdisp(local_color, do_ww);
    else
      fiddisp(local_color, do_ww);
    if(plot) {
      sc += ho;
      if (!horizontal)
        sc2 += vo;
    } else {
      dss_sc += ho;
      if (!horizontal)
        dss_sc2 += vo;
    }
    ctrace += step;
    index++;
  }
  if (!plot)
  {
    ResetLabels();
    DispField(FIELD1,PARAM_COLOR,"vf",vs,1);
    InitVal(FIELD2,HORIZ,SP_NAME, PARAM_COLOR,UNIT4,
                         SP_NAME, PARAM_COLOR,SCALED,2);
    InitVal(FIELD3,HORIZ,WP_NAME, PARAM_COLOR,UNIT4,
                         WP_NAME, PARAM_COLOR,SCALED,2);
    if (firstindex != lastindex)
    {
      DispField(FIELD4,PARAM_COLOR,"first",(double) firstindex,0);
      DispField(FIELD5,PARAM_COLOR,"last",(double) lastindex,0);
      DispField(FIELD6,PARAM_COLOR,"step",(double) step,0);
    }
  }
  if (plot)
    amove(0,0);
  else {
    exit_display();
    Wsetgraphicsdisplay("dfs");
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

  if (do_ww)
    close_whitewash();
  if (freebuffers()) return(ERROR);
  appendvarlist("dss_sc,dss_wc,sc,wc,vo,ho");
  return(COMPLETE);
}
