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
/*  dpf	 -	display peak frequencies			*/
/*  dpf('noll') - dpf without running 'nll', uses existing list */
/*  dpf('nn') - dpf that does not label negative lines		*/
/*  ppf	 -	plot peak frequencies				*/
/*  dpf('noll') - ppf without running 'nll', uses existing list */
/*  ppf('nn') - ppf that does not label negative lines		*/
/*  dpir -	display integral amplitudes under a spectrum	*/
/*  pir  -	plot integral amplitudes under a spectrum	*/
/****************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"
#include "vnmrsys.h"
#include "allocate.h"
#include "buttons.h"
#include "pvars.h"
#include "wjunk.h"

#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define OFFSET          0.0

struct line_entry {
                    int on,lowlimit,highlimit;
                    int newlow,newhigh,low,high;
                  };
struct int_entry {
                    int below,lowlimit,highlimit;
                  };


extern int debug1;
extern int raster;
extern void set_dpf_flag(int b, char *cmd);
extern void set_dpir_flag(int b, char *cmd);
extern void redo_dpf_dpir();
extern int showPlotBox();
extern double G_getCharSize(char *key);
extern void set_line_thickness(const char *thick);
extern void set_graphics_font(const char *fontName);
extern void G_getSize(char *key, int *size);

static PFV label_proc;
static double scale;
static float  dispcalib;
double *linefrq,*lineamp;
int    *linepos;
static int    CharSize;
static double leader_len;
struct line_entry *ga;   
struct int_entry  *gb;   
static double dpir_off;
static double dpf_wc2;
static double dpf_sc2;
static int vertflag;
static int axish = 0;
static int axisp = 0;
static double axisp_freq = 1.0;

/*************/
static void setwindows()
/*************/
{
  Wgmode(); /* goto tek graphics and set screen 2 active */
  Wshow_graphics();
}

/***************************************************/
static int leader_top(int peak_x, int peak_y, int label_x, int ymax)
/***************************************************/
{
  int newpeak_y;
  int dx,dy;
  int bottom;

  dx = label_x - peak_x;
  dy = abs(dx) / 2;
  
  newpeak_y = ymax - dy - (int) (3.0 * dispcalib);
  bottom =  dfpnt2 + (int) (dispcalib * vp + 1.0);
  if (newpeak_y < bottom)
    {
       if (!plot) Winfoprintf("vp too large for dpf display");
       return(0);
    }
  if (newpeak_y > peak_y) 
  {
    amove(peak_x,peak_y);
    rdraw(0,newpeak_y - peak_y);
  }
  amove(peak_x,newpeak_y);
  rdraw(dx,dy);
  rdraw(0,(int) (3.0 * dispcalib));
  
  /* re-use dx for logic */
  dx = plot && ((raster == 0) || (raster > 2)); /* hpgl and postscript output */
  if (dx) 		/* HP plotter */
       rmove( CharSize/5,5);
  else if (plot)
       rmove(-CharSize/2,5);
  else 
       rmove(-4*CharSize/5,5);
  return(1);
}

/*********************************/
static void label_top(int index, double amp, int label_x)
/*********************************/
/*----------------------------------------------------------------------+
| Date      Author	Modified					|
| --------  ----------  --------					|
| 1/21/92   Rolf K.     1. fixed label height for sc2+wc2<wc2max	|
|			2. allowed for lower label height (for multi-	|
|			   level 1D plotting and labelling		|
+----------------------------------------------------------------------*/
{
  char label[MAXPATHL];
  int  peak_y;
  int  ymax, ymax0;
  int  temp, len;

  (void) amp;
  if (axish) sprintf(label,"%1.2f",linefrq[index]);
  else if (axisp) sprintf(label,"%1.3f",linefrq[index]/axisp_freq);
  else if (sfrq < 1.1) sprintf(label,"%1.2f",linefrq[index]/sfrq);
  else 		  sprintf(label,"%1.3f",linefrq[index]/sfrq);
  len =  xcharpixels * strlen(label);
  ymax0 = mnumypnts - len - (int) (5.0 * dispcalib);
  ymax = ymax0;
  if (wc2max > dpf_wc2 + dpf_sc2) ymax -= (int) ((wc2max - dpf_sc2 - dpf_wc2) * dispcalib); 
		/* fixed for proper positioning with reduced plotting ranges */
		/* (did not work properly with sc2>0), r.k.                  */
  if (ymax < ymax0/3) ymax = ymax0/3;
		/* changed from ymax0/2 to ymax0/3 for 2 level plotting */
  temp = dfpnt2 - len + (int) (dispcalib * ( vp + 60.0));
		/* changed from 90 to 60 for 2 level plotting (r.k.)    */
		/* (e.g.: for aexppl)                                   */
  if (ymax < temp) 
  {  ymax = temp;
     if (ymax > ymax0) ymax = ymax0;
  }

  peak_y = dfpnt2 + (int) (dispcalib * (lineamp[index] + vp + 20.0));
  if (peak_y < ymin)
  {
    peak_y = ymin;
  }
  if (leader_top(linepos[index],peak_y,label_x,ymax))
     dvstring(label);
}

/***************************************************/
static void leader_bot(int peak_x, int peak_y, int label_x, int label_y, int ymax)
/***************************************************/
{
  if (peak_x == label_x)
  {
    amove(peak_x,peak_y);
    rdraw(0,label_y - peak_y);
  }
  else
  { int dh,dx,dy;

    dx = label_x - peak_x;
    dy = abs(dx) / 2;
    dh = label_y - peak_y + (int) (3.0 * dispcalib);
    if (peak_y + dh + dy + (int) (3.0 * dispcalib) > ymax)
      dh = ymax - peak_y - dy - (int) (3.0 * dispcalib);
    amove(peak_x,peak_y);
    if (dh > 0)
    {
      rdraw(0,dh);
    }
    else if (peak_y + dy + (int) (3.0 * dispcalib) > ymax)
    {
      peak_y = ymax - dy - (int) (3.0 * dispcalib);
      if (peak_y < dfpnt2 + (int) (dispcalib * vp + 1.0))
      {
         dy = dfpnt2 + (int) (dispcalib * vp + 1.0) - peak_y;
         if (dy < 0)
            dy = 0;
         peak_y = dfpnt2 + (int) (dispcalib * vp + 1.0);
      }
      amove(peak_x,peak_y);
    }
    rdraw(dx,dy);
    rdraw(0,(int) (3.0 * dispcalib));
  }
  if (((raster == 0) || (raster > 2)) && plot) 		/* HP plotter */
    rmove( CharSize/5,5);				/* and PS     */
  else 
    rmove(-CharSize/2,5);
}

/*********************************/
static void label_bot(int index, double amp, int label_x)
/*********************************/
/*----------------------------------------------------------------------+
| Date      Author	Modified					|
| --------  ----------  --------					|
| 1/21/92   Rolf K.	1. added controls for maximum height of labels	|
|			   (similar to 'top' mode).			|
+----------------------------------------------------------------------*/
{
  char label[MAXPATHL];
  int  label_y,peak_y;
  int  ymax, ymax0, temp;

  if (axish) sprintf(label,"%1.2f",linefrq[index]);
  else if (axisp) sprintf(label,"%1.3f",linefrq[index]/axisp_freq);
  else if (sfrq < 1.1) sprintf(label,"%1.2f",linefrq[index]/sfrq);
  else 		  sprintf(label,"%1.3f",linefrq[index]/sfrq);
  ymax0 = mnumypnts - xcharpixels * strlen(label) - (int) (11.0 * dispcalib);
  ymax = ymax0;
  if (wc2max > dpf_wc2 + dpf_sc2) ymax -= (int) ((wc2max - dpf_sc2 - dpf_wc2) * dispcalib);
  if (ymax < ymax0/3) ymax = ymax0/3;
  temp = dfpnt2 - xcharpixels * strlen(label) + (int) (dispcalib * (vp + 60.0));
  if (ymax < temp) ymax = temp;

  label_y = dfpnt2 + (int) (dispcalib * (amp + vp + leader_len));
  if (label_y > ymax)
    label_y = ymax;
  amp = lineamp[index];
  peak_y = dfpnt2 + (int) (dispcalib * (amp + vp + 5.0));
  if (peak_y > label_y - (int) (6.0 * dispcalib))
    peak_y = label_y - (int) (6.0 * dispcalib);
  if ((peak_y < ymin) || (label_y < ymin))
  {
    peak_y = ymin;
    label_y = ymin + (int) (6.0 * dispcalib);
  }
  leader_bot(linepos[index],peak_y,label_x,label_y,ymax);
  dvstring(label);
}

/*************************/
static int findhigh(int index)
/*************************/
{
  int i,savei;
  double max = 0.0;

  savei = ga[index].low;
  for (i = ga[index].low; i <= ga[index].high; i++)
    if (lineamp[i] > max)
    {
      max = lineamp[i];
      savei = i;
    }
  return(savei);
}

/*********************************/
static double lefthigh(int index, int limit)
/*********************************/
{
  int i;
  double max = 0.0;

  for (i = ga[index].low; i <= limit; i++)
    if (lineamp[i] > max)
      max = lineamp[i];
  return(max);
}

/**********************************/
static double righthigh(int index, int limit)
/**********************************/
{
  int i;
  double max = 0.0;

  for (i = limit; i <= ga[index].high; i++)
    if (lineamp[i] > max)
      max = lineamp[i];
  return(max);
}

/*************************/
static void label_group(int index)
/*************************/
{
  int line,ix,i,ij,pos;
  double amp;

  line = ga[index].low;
  if (ga[index].high == line)
    (*label_proc)(line,lineamp[line],linepos[line]);
  else
  {
    ix = findhigh(index);
    for (i = ix; i >= line; i--)
    {
      ij = i;
      if ((ga[index].high > ix) && (i == ix))
        ij++;
      pos = ga[index].newlow + (i - line) * CharSize;
      amp = ((pos > linepos[i]) ? lineamp[ix] : lefthigh(index,ij));
      (*label_proc)(i,amp,pos);
    }
    for (i = ix + 1; i <= ga[index].high; i++)
    {
      pos = ga[index].newlow + (i - line) * CharSize;
      amp = ((pos < linepos[i]) ? lineamp[ix] : righthigh(index,i));
      (*label_proc)(i,amp,pos);
    }
  }
}

/**************************/
static void groupspace(int index)
/**************************/
{
  int minlow,maxhigh,center,halfgroup,ingroup;

  minlow = dfpnt + CharSize;
  maxhigh = dfpnt + dnpnt - CharSize;
  ingroup = ga[index].high - ga[index].low + 1;
  center = (ga[index].highlimit + ga[index].lowlimit) / 2;
  halfgroup = (ingroup / 2) * CharSize;
  if ((ingroup % 2) == 0)
    halfgroup -= CharSize / 2;
  ga[index].newlow = center - halfgroup;
  if (center + halfgroup < 0)
  {
    ga[index].newhigh = maxhigh;
    ga[index].newlow = ga[index].newhigh - 2 * halfgroup;
  }
  else
    ga[index].newhigh = center + halfgroup;
  if (ga[index].newhigh > maxhigh)
  {
    ga[index].newlow += maxhigh - ga[index].newhigh;
    ga[index].newhigh = maxhigh;
  }
  if (ga[index].newlow < minlow)
  {
    ga[index].newhigh += minlow - ga[index].newlow;
    ga[index].newlow = minlow;
  }
}

/**************************/
static void findactive(int *groups)
/**************************/
{
  int oldgroups,index;

  oldgroups = *groups;
  *groups = 0;
  for (index = 1; index <= oldgroups; index++)
    if (ga[index].on)
    {
      *groups += 1;
      if (*groups != index)
      {
        ga[*groups].on        = ga[index].on;
        ga[*groups].low       = ga[index].low;
        ga[*groups].lowlimit  = ga[index].lowlimit;
        ga[*groups].newlow    = ga[index].newlow;
        ga[*groups].high      = ga[index].high;
        ga[*groups].highlimit = ga[index].highlimit;
        ga[*groups].newhigh   = ga[index].newhigh;
      }
    }
}

/**************************/
static int groupcheck(int lines)
/**************************/
{
  int done,i,ctr;
  int laston,groups;

  groups = lines;
  if ((ga = (struct line_entry *)
            allocateWithId(sizeof(struct line_entry)*(lines + 1),"dpf"))==0)
  { Werrprintf("cannot allocate group buffer space");
    return(0);
  }
  for (i = 1; i <= groups; i++)
  {
    ga[i].on = 1;
    ga[i].low = ga[i].high = i;
    ga[i].highlimit = ga[i].lowlimit = linepos[i];
    ga[i].newlow = ga[i].newhigh = ga[i].highlimit;
  }
  i = 0;
  done = FALSE;
  while (!done && (groups > 1))
  {
    i++;
    laston = 1;
    done = TRUE;
    for (ctr = 2; ctr <= groups; ctr++)
      if ((ga[ctr].newlow - ga[laston].newhigh) < CharSize)
      {
        done = ga[ctr].on = FALSE;
        ga[laston].highlimit = ga[ctr].highlimit;
        ga[laston].high = ga[ctr].high;
        groupspace(laston);
      }
      else
        laston = ctr;
    findactive(&groups);
  }
  return(groups);
}

/*******************/
static int getlines(int noneg)
/*******************/
{
  vInfo  info;
  double freq;
  double amp;
  double tmpfrq;
  int    tmppos;
  int    r;
  int    index;
  int    count=0;

  if ( (r=P_getVarInfo(CURRENT,"llfrq",&info)) )
  { P_err(r,"llfrq",":");   return ERROR; }
  if (info.size >= 1)
  {
    if ((linefrq = (double *)
                   allocateWithId(sizeof(double)*(info.size + 1),"dpf"))==0)
    { Werrprintf("cannot allocate buffer space");
      return(0);
    }
    if ((lineamp = (double *)
                   allocateWithId(sizeof(double)*(info.size + 1),"dpf"))==0)
    { Werrprintf("cannot allocate buffer space");
      release(linefrq);
      return(0);
    }
    if ((linepos = (int *)
                   allocateWithId(sizeof(int)*(info.size + 1),"dpf"))==0)
    { Werrprintf("cannot allocate buffer space");
      release(linefrq);
      release(lineamp);
      return(0);
    }
    if (debug1)
      Wscrprintf("line      freq       amp       position\n");
    for (index = 1; index <= info.size; index++)
    {
      if ( (r=P_getreal(CURRENT,"llfrq",&freq,index)) )
      { P_err(r,"llfrq",":");   return ERROR; }
      tmpfrq = freq - rflrfp;
      tmppos = dfpnt  + dnpnt  * (wp  - tmpfrq  + sp ) / wp;
      if ( (r=P_getreal(CURRENT,"llamp",&amp,index)) )
      { P_err(r,"llamp",":");   return ERROR; }
      if (tmppos > dfpnt && tmppos < dfpnt+dnpnt && (amp >= 0 || !noneg))
      {
	count++;
        linefrq[count] = tmpfrq;
        linepos[count] = tmppos;
        if (amp < 0.0)
          amp = 0.0;
        lineamp[count] = amp * scale;
        if (debug1)
          Wscrprintf("%d   %g   %g   %d\n",
                  count,linefrq[count],lineamp[count],linepos[count]);
      }
    }
    return(count);
  }
  else
    return(0); 
}

/****************************************/
static void remove_lines(int maxlines, int *numlines)
/****************************************/
{
  int i,line;
  double min;

  line = 0;
  while (*numlines > maxlines)
  {
    min = 1.0e9;
    for (i = 1; i < *numlines; i++)
      if (linefrq[i] - linefrq[i+1] < min)
      {
        line = i;
        min = linefrq[i] - linefrq[i+1];
      }
    if (lineamp[line] > lineamp[line + 1])
      line++;
    for (i = line; i < *numlines; i++)
    {
      linefrq[i] = linefrq[i+1];
      lineamp[i] = lineamp[i+1];
      linepos[i] = linepos[i+1];
    }
    *numlines -= 1;
  }
}

/*************/
int dpf(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int groups;
  int noisemult_p = 0;
  int numlines,maxlines;
  int i;
  int donll = TRUE;
  int noneg = FALSE;
  int top = FALSE;
  char command[128];
  char name[64];

  (void) retc;
  (void) retv;
  Wturnoff_buttons();
  leader_len = 20.0;
  axish = FALSE;
  axisp = FALSE;
  axisp_freq = 1.0;
  if (argc > 1)
  {
    for (i=1; i<argc; i++)
    {
      if (strcmp(argv[i],"off") == 0) {
	set_dpf_flag(0,"");
        redo_dpf_dpir();
	RETURN;
      } else if (strcmp(argv[i],"turnoff") == 0) {
        set_dpf_flag(0,"");
        RETURN;
      } else if (strcmp(argv[i],"noll") == 0) donll = FALSE;
      else if (strcmp(argv[i],"pos") == 0) noneg = TRUE;
      else if (strcmp(argv[i],"top") == 0) top = TRUE;
      else if (strcmp(argv[i],"axish") == 0) axish = TRUE;
      else if (strcmp(argv[i],"axisp") == 0)
      {
         axisp = TRUE;
         P_getreal(CURRENT,"reffrq",&axisp_freq,1);
      }
      else if (strcmp(argv[i],"leader") == 0)
      {
         if (((i+1) < argc) && isReal(argv[i+1]) )
         {
            i++;
            leader_len = stringReal(argv[i]);
         }
      }
      else if (isReal(argv[i])) noisemult_p = i;
    }
  }
  if (donll) /* if not donll, dpf uses last previous line listing */
  {
    if (noisemult_p != 0)
    {
      if (noneg)
	strcpy(command,"nll('dpf','pos',");
      else
	strcpy(command,"nll('dpf',");
      strcat(command,argv[noisemult_p]);
      strcat(command,")\n");
    }
    else
    {
      if (noneg)
	strcpy(command,"nll('dpf','pos')\n");
      else
	strcpy(command,"nll('dpf')\n");
    }
    execString(command);
  }

  if(P_getreal(CURRENT, "dpf_sc2",&dpf_sc2,1)) dpf_sc2=sc2;
  if(P_getreal(CURRENT, "dpf_wc2",&dpf_wc2,1)) dpf_wc2=wc2;
  if(dpf_wc2>wc2) dpf_wc2=wc2;
  if(dpf_sc2<sc2) dpf_sc2=sc2;

  /* if (init2d(1,1)) return(ERROR); */
  scale = vs;
  if (normflag)
    scale *= normalize;
  /*   Wscrprintf("normflag=%d normalize=%g\n",normflag,normalize); */
  plot = (argv[0][0] == 'p');

/*select_init(get_rev, dis_setup, fdimname, doheaders, docheck2d, dospecpars,
            	doblockpars, dophasefile)*/
/*if (init2d(0,plot + 1)) return(ERROR); */
  if (select_init(
	0,
	plot+1,
	NO_FREQ_DIM,
	NO_HEADERS,
	DO_CHECK2D,
	DO_SPECPARS,
	NO_BLOCKPARS,
	NO_PHASEFILE
     ))
      return(ERROR);
  if ((numlines = getlines(noneg)) == 0) RETURN;
  if (!plot)
  {
    setwindows();
    dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;

    getOptName(PEAK_MARK,name);
    set_line_thickness(name);
    getOptName(PEAK_NUM,name);
    set_graphics_font(name);
    
  }
  else
  { 
    double size = G_getCharSize("PeakNum");
    charsize(size);
    //charsize((double)0.7);
    dispcalib = ppmm / ymultiplier;
  }
  CharSize = ycharpixels + ycharpixels / 5;
  maxlines = dnpnt / CharSize;

  if (maxlines < numlines)
    remove_lines(maxlines,&numlines);
  if (numlines > 0)
  {
    //color(PARAM_COLOR);
    color(PEAK_MARK_COLOR);
    groups = groupcheck(numlines);
    if (debug1)
    {
      Wscrprintf("index high low newhigh newlow highlimit lowlimit\n");
      for (i = 1; i <= groups; i++)
        Wscrprintf("%d %d %d %d %d %d %d\n",i,ga[i].high,ga[i].low,
                   ga[i].newhigh,ga[i].newlow,ga[i].highlimit,ga[i].lowlimit);
    }
    label_proc = (top) ? (PFV) label_top : (PFV) label_bot;
    for (i = 1; i <= groups; i++)
      label_group(i);

    if (!plot) { // construct the command, and set the flag and command
      char cmd[64];
      if(argc > 1) {
        if (isReal(argv[1]) ) sprintf(cmd,"%s(%s",argv[0],argv[1]);
	else sprintf(cmd,"%s('%s'",argv[0],argv[1]);
	for(i=2;i<argc;i++)
        {
           if (isReal(argv[i]) )
           {
	      strcat(cmd,",");
              strcat(cmd,argv[i]);
           }
	   else
           {
	      strcat(cmd,",'");
              strcat(cmd,argv[i]);
	      strcat(cmd,"'");
           }
	}
	strcat(cmd,")\n");
      } else sprintf(cmd,"%s\n",argv[0]);
      Wsetgraphicsdisplay("ds");
      set_dpf_flag(1, cmd);
    }
  }
  releaseAllWithId("dpf");
  endgraphics();
  disp_status("        ");
  set_graphics_font("Default");
  RETURN;
}

/***************************************************/
static void label_pirv(int index)
/***************************************************/
{
  char label[MAXPATHL];
  int offset0,offset1,ticlen;
  int ychar = 12;

  if(gb[index].lowlimit <= dfpnt || gb[index].highlimit <= dfpnt
	|| gb[index].lowlimit >= (dfpnt+dnpnt-xcharpixels)
	|| gb[index].highlimit >= (dfpnt+dnpnt-xcharpixels)) return;

  /*Charsize in pixels */
  
  if(!showPlotBox())
    offset0 = (int) (dpir_off*dispcalib) + CharSize + ychar/3;
  else offset0 = ychar/2;
  ticlen = ycharpixels/3;
  amove(gb[index].lowlimit,
        dfpnt2 - offset0 + (int) (dispcalib * vp));
  rdraw(0, -ticlen);
  rdraw(gb[index].highlimit - gb[index].lowlimit,0);
  rdraw(0, ticlen);
  amove((gb[index].lowlimit + gb[index].highlimit) / 2,
        dfpnt2 - offset0 + (int) (dispcalib * vp) - ticlen);
  rdraw(0, -ticlen);
  sprintf(label,"%1.2f",lineamp[index]);
 offset1=0;
 if (!showPlotBox()) {
  rmove(ycharpixels/3,(-xcharpixels * (int)strlen(label) - offset1)); 
 } else {
  rmove(-ycharpixels/2,(-xcharpixels * (int)strlen(label) - offset1)); 
 }
  dvstring(label);
}

/***************************************************/
static void label_pir(int index, int ysize)
/***************************************************/
{
  char label[MAXPATHL];
  int offset0,offset1,ticlen;
  int ychar = 12;

  if(gb[index].lowlimit <= dfpnt || gb[index].highlimit <= dfpnt
	|| gb[index].lowlimit >= (dfpnt+dnpnt-xcharpixels)
	|| gb[index].highlimit >= (dfpnt+dnpnt-xcharpixels)) return;

  /*Charsize in pixels */
  
  if(!showPlotBox())
    offset0 = (int) (dpir_off*dispcalib) + CharSize + ychar/3;
  else offset0 = ychar/2;
  ticlen = ycharpixels/3;
  amove(gb[index].lowlimit,
        dfpnt2 - offset0 + (int) (dispcalib * vp));
  rdraw(0, -ticlen);
  rdraw(gb[index].highlimit - gb[index].lowlimit,0);
  rdraw(0, ticlen);
  amove((gb[index].lowlimit + gb[index].highlimit) / 2,
        dfpnt2 - offset0 + (int) (dispcalib * vp) - ticlen);
  rdraw(0, -ticlen);
  sprintf(label,"%1.2f",lineamp[index]);
  if (ysize < 3  || index % 2 == 1) offset1 = 0; else offset1 = ycharpixels;
  rmove((-xcharpixels * (int)strlen(label)) / 2, -ycharpixels - offset1); 
  dstring(label);
}

/*******************/
static int getregions(int resets)
/*******************/
{
  double freq;
  double amp;
  int    r;
  int    index,groups;

  double vs1;
  double start;
  double edge;
  double norm;
  
  if ((lineamp = (double *)
              allocateWithId(sizeof(double)*(((resets + 1) / 2)+1),"dpir"))==0)
  { Werrprintf("cannot allocate buffer space");
    return(0);
  }
  if ((gb = (struct int_entry *)
          allocateWithId(sizeof(struct int_entry)*(((resets + 1) / 2)+1),"dpir"))==0)
  { Werrprintf("cannot allocate buffer space");
    release(lineamp);
    return(0);
  }

  start = sp + wp;
  edge = start;
  if (normInt)
  {
    norm = 0.0;
    if (debug1)
      Wscrprintf("integral normalization\n");
    for (index = 1; index <= resets; index++)
    {
      if ( (r=P_getreal(CURRENT,"lifrq",&freq,index)) )
      { P_err(r,"lifrq",":");   return ERROR; }
      if (freq - rflrfp < edge )
        if (index % 2 == 0)
        {
          if ( (r=P_getreal(CURRENT,"liamp",&amp,index)) )
          { P_err(r,"liamp",":");   return ERROR; }
          norm += amp;
        }
    }
    vs1 = (norm == 0.0) ? 1.0 : insval  / norm;
  }
  else
    vs1 =  insval;
  if (vs1 == 0.0)
    vs1 = 1.0;
  if (debug1)
    Wscrprintf("scale factor= %g, liamp size= %d\n",
                vs1,resets);
  groups = 0;
  gb[1].lowlimit = dfpnt;
  for (index = 1; index <= resets; index++)
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&freq,index)) )
    { P_err(r,"lifrq",":");   return ERROR; }
    freq -= rflrfp;
    if (freq < edge )
    {
      if (freq < sp)
        freq = sp;
      if ( (r=P_getreal(CURRENT,"liamp",&amp,index)) )
      { P_err(r,"liamp",":");   return ERROR; }
      if (index % 2)
        gb[groups + 1].lowlimit = dfpnt  + dnpnt  * (wp - freq + sp ) / wp;
      else
      {
        groups++;
        gb[groups].highlimit = dfpnt  + dnpnt  * (wp - freq + sp ) / wp;
        lineamp[groups] = vs1 * amp;
      }
    }
  }
  return(groups);
}

/********************/
static int check_int(int *number)
/********************/
{
  vInfo  info;
  char   intmod[9];
  int    res;
  
  *number = ((P_getVarInfo(CURRENT,"liamp",&info)) ? 0 : info.size);
  if ( (res=P_getstring(CURRENT,"intmod",intmod,1,8)) )
  { P_err(res,"intmod",":"); ABORT; }
  return(!(strcmp(intmod,INT_FULL))); 
}

/*************/
int dpir(int argc, char *argv[], int retc, char *retv[])
/*************/
{ int groups;
  int resets;
  int i, min, ysize;
  char name[64];
  char tmpstr[MAXSTR];

  (void) retc;
  (void) retv;

  if (argc > 1 && strcmp(argv[1],"off") == 0) {
	set_dpir_flag(0,"");
        redo_dpf_dpir();
	RETURN;
  } else if (argc > 1 && strcmp(argv[1],"turnoff") == 0) {
        set_dpir_flag(0,"");
        RETURN;
  }

  vertflag=0;
  if ( ! P_getstring(GLOBAL, "integralOrient", tmpstr, 1, MAXSTR) )
  {
     vertflag = (strcmp(tmpstr,"vert") == 0);
  }
  if (argc > 1 && strcmp(argv[1],"vert") == 0) {
      vertflag=1;
  }

  if(argc == 1 || strcmp(argv[1],"noli") != 0) {
     execString("nli\n");
  }
  if (check_int(&resets))
  {
    Werrprintf("intmod must not be set to 'full'");
    ABORT;
  }
  if (resets <= 1)
  {
    Werrprintf("no integral resets are defined");
    ABORT;
  }
  plot = (argv[0][0] == 'p');
  if (select_init(
	0,
	plot+1,
	NO_FREQ_DIM,
	NO_HEADERS,
	DO_CHECK2D,
	DO_SPECPARS,
	NO_BLOCKPARS,
	NO_PHASEFILE
     )) {
      ABORT;
  }

  if(strlen(argv[0]) > 4 && argv[0][4] == 'N') {
     normInt = 1;
  } else if(strlen(argv[0]) > 3 && argv[0][3] == 'N') {
     normInt = 1;
  }

  if ((groups = getregions(resets)) == 0) {
	ABORT;
  }

  min=mnumxpnts; 
  for (i = 1; i <= groups; i++) {
    if(gb[i].lowlimit <= 0 || gb[i].highlimit <= 0
	|| gb[i].lowlimit >= (mnumxpnts-xcharpixels)
	|| gb[i].highlimit >= (mnumxpnts-xcharpixels)) continue;
    if((gb[i].highlimit-gb[i].lowlimit) < min) 
	min = gb[i].highlimit-gb[i].lowlimit;
  }
  if(min != 0 && min < 3*xcharpixels) ysize = 3; else ysize=2;
    
  if (!plot)
  {
    setwindows();
    dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
    CharSize = ycharpixels + ycharpixels / 5;

    getOptName(INTEG_MARK,name);
    set_line_thickness(name);
    getOptName(INTEG_NUM,name);
    set_graphics_font(name);
  }
  else
  { 
    double size = G_getCharSize("IntegralNum");
    charsize(size);
    //charsize((double)0.7);
    dispcalib = ppmm / ymultiplier;
    CharSize = ycharpixels;
  }

  if (showPlotBox()) dpir_off=0.0;
  else dpir_off=9.0;

  if(dispcalib>0) {
    int size= 12;
    G_getSize("IntegralNum", &size);
    i = (int)(vp*dispcalib); 
    if (vertflag && i < 4*size)
    {
      char cmd[64];
      i = (int)(4*size/dispcalib)+1;
      if(vertflag) sprintf(cmd,"vp=%d ds %s('vert')\n",i,argv[0]);
      else sprintf(cmd,"vp=%d ds %s\n",i,argv[0]);
      if(!plot) execString(cmd); 
      //Werrprintf("pir requires minimum vp of %d",i);
      RETURN;
    } else if (i < 3*size)
    {
      char cmd[64];
      i = (int)(3*size/dispcalib)+1;
      if(vertflag) sprintf(cmd,"vp=%d ds %s('vert')\n",i,argv[0]);
      else sprintf(cmd,"vp=%d ds %s\n",i,argv[0]);
      if(!plot) execString(cmd); 
      //Werrprintf("pir requires minimum vp of %d",i);
      RETURN;
    }
  } else {
    if (vp < 30)
    {
      char cmd[64];
      i=30;
      if(vertflag) sprintf(cmd,"vp=%d ds %s('vert')\n",i,argv[0]);
      else sprintf(cmd,"vp=%d ds %s\n",i,argv[0]);
      if(!plot) execString(cmd); 
      //Werrprintf("pir requires minimum vp of 30");
      RETURN;
    }
  }

  //color(PARAM_COLOR);
  color(INTEG_MARK_COLOR);
  if (debug1)
  {
    Wscrprintf("dispcalib= %g, dfpnt2= %d, CharSize= %d, bottom= %d\n",
        dispcalib,dfpnt2,CharSize,
        dfpnt2 - CharSize + (int) (dispcalib * (vp - dpir_off - 5.0)));
    Wscrprintf("index highlimit lowlimit below\n");
    for (i = 1; i <= groups; i++)
      Wscrprintf("%d %d %d %s\n",i,gb[i].highlimit,gb[i].lowlimit,
                  (gb[i].below) ? "true" : "false");
  }
  if(vertflag) {
    for (i = 1; i <= groups; i++) label_pirv(i);
  } else {
    for (i = 1; i <= groups; i++) label_pir(i, ysize);
  }
  if (!plot) {
      char cmd[64];
      if(argc > 1) {
        if (isReal(argv[1]) ) sprintf(cmd,"%s(%s",argv[0],argv[1]);
	else sprintf(cmd,"%s('%s'",argv[0],argv[1]);
	for(i=2;i<argc;i++)
        {
           if (isReal(argv[i]) )
           {
	      strcat(cmd,",");
              strcat(cmd,argv[i]);
           }
	   else
           {
	      strcat(cmd,",'");
              strcat(cmd,argv[i]);
	      strcat(cmd,"'");
           }
	}
	strcat(cmd,")\n");
      } else sprintf(cmd,"%s\n",argv[0]);
      Wsetgraphicsdisplay("ds");
      set_dpir_flag(1,cmd);
  }
  releaseAllWithId("dpir");
  endgraphics();
  disp_status("        ");
  set_graphics_font("Default");
  RETURN;
}

