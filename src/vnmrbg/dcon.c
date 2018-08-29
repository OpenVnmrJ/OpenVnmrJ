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
/*  dcon.c	-	display a 2D colour map	 	*/
/*                      or a 2D stacked plot 		*/
/*							*/
/********************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "sky.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"
#include "allocate.h"
#include "buttons.h"
#include "dscale.h"
#include "init_display.h"
#include "init_proc.h"

extern int debug1;

#define FALSE			0
#define TRUE			1
#define SELECT_BOX_COLOR	0
#define SELECT_GRAY_COLOR	1
#define SELECT_AV_COLOR		2
#define SELECT_PH_COLOR		3
#define SELECT_SPEC_COLOR	4

#define MAX_PH_THRESH           6
#define MAX_AV_THRESH           14

extern void rast(void *src, int n, int times, int x, int y);
extern void init_rast(void *src, int n, int init);
extern void backing_up();
extern void set_gray_offset(double a, double b);
extern void setcolormap(int firstcolor, int numcolors, int th, int phcolor);
extern void graph_batch(int on);

static float discalib;
static int dmode,linearflag;
extern int raster;
int dcon_num_colors,dcon_first_color,dcon_colorflag;
double graysl,graycntr;
int gray_p_flag;

extern int Gphsd;

#ifdef CLOCKTIME
   extern int dcon_timer_no;
#endif 

static int filled_display(int phflag, int pos_only);

/*
 *  The following procedures will whitewash ybars.
 *  The function is initialized by calling init_whitewash.  The total
 *  number of display points must be passed as an argument.  This
 *  argument is typically either (dfpnt + dnpnt) or mnumxpnts.
 *  The arguments to the procedure whitewash are a ybars pointer,
 *  the first ybar element to be whitewashed,  and the number of
 *  ybar elements to be whitewashed.  close_whitewash  must be called
 *  to release the ybar buffer.
 */
static int  *ww = 0;

/*
 *  Release whitewash buffer
 */
/********************/
void close_whitewash()
/********************/
{
  if (ww)
    release(ww);
  ww = 0;
}

/*
 *  Allocate and zero the whitewash buffer
 */
/*********************/
int init_whitewash(pts)
/*********************/
register int pts;
{
  register int i;

  close_whitewash();  /* release ww buffer;  this is only a precaution */
  if ((ww = (int *) allocateWithId(sizeof(int)*pts,"dcon"))==0)
  {
    Werrprintf("cannot allocate whitewash buffer");
    return(1);
  }
  for (i=0; i<pts; i++)
    ww[i] = 0;
  return(0);
}

/************************/
void whitewash(out,fpt,npt)
/************************/
struct ybar *out;
int fpt,npt;
{
  register int i;
  register int *wwptr;
  struct ybar *outptr;

  wwptr = ww + fpt;
  outptr = out + fpt;
  for (i=0; i<npt; i++)
    { if (outptr->mx < *wwptr)
      { outptr->mn = *wwptr;
        outptr->mx = *wwptr - 1;
      }
      else if (outptr->mn < *wwptr)
      {
        outptr->mn = *wwptr+1;
        *wwptr = outptr->mx;
      }
      else
        *wwptr = outptr->mx;
      wwptr++;
      outptr++;
    }
}

/***********************/
static void fillit(out,ithv,n)
/***********************/
int ithv,n;
struct ybar *out;
{ int i;
  for (i=0; i<n; i++)
    { if (out->mn>ithv) out->mn = ithv;
      out++;
    }
}

/***********************/
static void nobase(out,ithv,n)
/***********************/
int ithv,n;
struct ybar *out;
{ int i;
  for (i=0; i<n; i++)
    { if (out->mx<ithv) out->mn = out->mx + 1;
      out++;
    }
}

/**********************/
static void fill2(out,ithv,n)
/**********************/
int ithv,n;
struct ybar *out;
{ int i;
  for (i=0; i<n; i++)
    { if (out->mx<ithv) out->mn = out->mx + 1;
      else if (out->mn>ithv) out->mn = ithv;
      out++;
    }
}

/******************/
void stack_changecolor(n)
/******************/
int n;
{ color((n % (NUM_AV_COLORS-2)) + FIRST_AV_COLOR + 1);
}

/*********************/
static int i_dcon(plotmode)
/*********************/
/* initialize dcon program */
int plotmode;
{ Wturnoff_buttons();
  if (init2d(1,plotmode)) return 1;
  if (!d2flag)
    { Werrprintf("no 2D data in data file");
      return 1;
    }
  discalib = (float)(mnumypnts-ymin) / wc2max;
  ResetLabels();
  return 0;	/* successfull initialization */
}

/*****************/
int dcon_displayparms()
/*****************/
/* display certain parameters in last two lines of display */
{
  InitVal(FIELD1,VERT, SP_NAME, PARAM_COLOR,UNIT4,
                       SP_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD2,VERT, WP_NAME, PARAM_COLOR,UNIT4,
                       WP_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD3,HORIZ,SP_NAME, PARAM_COLOR,UNIT4,
                       SP_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD4,HORIZ,WP_NAME, PARAM_COLOR,UNIT4,
                       WP_NAME, PARAM_COLOR,SCALED,2);
  DispField(FIELD5,PARAM_COLOR,"th",th,1);
  DispField(FIELD6,PARAM_COLOR,"vs2d",vs2d,1);
  return(0);
}

/****************/
static int stackplot()
/****************/
{ int i,ctrace,ithv;
  float vs1,cury;
  float *phasfl;
  short *bufpnt;
  struct ybar *out;
  extern double expf_dir();
  float  incry;

  if ((bufpnt = (short *) allocateWithId(sizeof(short)*npnt,"dcon"))==0)
    { Werrprintf("cannot allocate buffer"); return 1; }
  if ((out = (struct ybar *) allocateWithId(sizeof(struct ybar)*mnumxpnts,
    "dcon"))==0)
    { Werrprintf("cannot allocate ybar buffer");
      return 1;
    }
  if (dmode!=4)
     if (init_whitewash(dnpnt+dfpnt))
        return(1);
  ctrace = fpnt1;
  if (!dcon_colorflag)
	  color(SPEC_COLOR);
  cury = (float)(dfpnt2);
  incry = (float) expf_dir(VERT);
  vs1 = discalib * vs2d;
  disp_status("STACKP  ");
  for (i=1; i<=npnt1; i++)
    { if ((phasfl=gettrace(ctrace,fpnt))==0)
      {
        disp_status("        ");
	return 1;
      }
      scfix1(phasfl,1,vs1,bufpnt,1,npnt);
      ctrace++;
      ithv = (int)(th * discalib + cury + 0.5);
      if (dnpnt > npnt)
         expand(bufpnt,npnt,out+dfpnt,dnpnt,(int)(cury));
      else
         compress(bufpnt,npnt,out+dfpnt,dnpnt,(int)(cury));
      switch (dmode)
	{ case 1: fillit(out+dfpnt,ithv,dnpnt); break;
	  case 2: nobase(out+dfpnt,ithv,dnpnt); break;
	  case 3: fill2(out+dfpnt,ithv,dnpnt); break;
        }
      if (dmode!=4) whitewash(out,dfpnt,dnpnt);
      if (dcon_colorflag)
    	  stack_changecolor(fpnt1+i);

      grf_batch(1);
      ybars(dfpnt,dfpnt+dnpnt-1,out,0,mnumypnts,dfpnt2);
      grf_batch(0);
      cury += incry;
    }
  disp_specIndex(specIndex);
  disp_status("        ");
  return 0;
}

/*************/
int ds2d(argc,argv,retc,retv)	int argc; char *argv[]; int retc; char *retv[];
/*************/
/*  ds2d   -  display 2-D stacked plot on screen	*/
/*  ds2dn  -  same, but do not erase the screen         */
/*  pl2d   -  plot 2-D stacked plot			*/
{ int plotmode;
  void close_whitewash();
  int argnum;
  int doaxis;
  int redisp;

  doaxis = 1;
  argnum = 1;
  dmode = 0;
  redisp = 0;
  if (argc>argnum)
  {
     int i;

     for (i=argnum; i<argc; i++)
     { if (strcmp(argv[i],"fill")==0) { dmode = 1; th = -1.5; }
       else if (strcmp(argv[i],"nobase")==0) dmode = 2;
       else if (strcmp(argv[i],"fillnb")==0) dmode = 3;
       else if (strcmp(argv[i],"noww")==0) dmode = 4;
       else if (strcmp(argv[i],"noaxis")==0) doaxis = 0;
       else if (strcmp(argv[i],"redisplay")==0) redisp = 1;
       else
       { Werrprintf("usage - %s<(mode), mode=fill,nobase,noww,fillnb>",argv[0]);
         return 1;
       }
     }
  }
  dcon_colorflag = SELECT_AV_COLOR;		/* was set to 1, confused Tek */
					/* That value represented gray colors */
  if (strcmp(argv[0],"pl2d")==0) plotmode = 2;	/* graphics on plotter */
  else plotmode = 1;	/* graphics on screen */
  if (!plot)
      grf_batch(1);
  if (redisp)
    erase_dcon_box();
  else
    if (i_dcon(plotmode))
      { endgraphics();
        if (!plot)
	    grf_batch(0);
        return 1;
      }
  if (!plot && !redisp)
    { Wgmode(); /* goto tek graphics and set screen 2 active */
      if (argv[0][4]!='n')
        { Wclear_graphics();
          show_plotterbox();
        }
      if (argv[0][4]=='n') graph_batch(1);	/* write to pixmap */
      Wshow_graphics();
      dcon_displayparms();
    }
  if (doaxis && !redisp)
     scale2d(1,0,1,SCALE_COLOR);
                        /* first argument causes the entire box to be drawn  */
                        /* second argument is the vertical offset of the box */
                        /* third argument is non-zero to tell scale2d this   */
                        /*  is a Draw, not an erase operation.               */
                        /* fourth argument is the color                      */
  if ((WisSunColor() || Wistek()) && !plot && dcon_colorflag)
    setcolormap(FIRST_AV_COLOR,NUM_AV_COLORS,0,0);
  stackplot();
  close_whitewash();
  if (!plot) graph_batch(0);
  if (redisp) return 0;
  releaseAllWithId("dcon");
  D_allrelease();
  endgraphics();
  if (!plot)
    Wsetgraphicsdisplay("ds2d");
  return 0;
}

/************************************************************************/
/*  CONTOUR WENT AWAY AND IS REPLACED BY FILLED_DISPLAY 		*/
/************************************************************************/

/***************************/
int dcon(argc,argv,retc,retv)
/***************************/
int argc; char *argv[]; int retc; char *retv[];
{ int plotmode,i,argnum,fg1;
  int phflag,pos_only;
  int rev_phase,norm_phase;
  int rev_phaseangle,norm_phaseangle;
  int doaxis=1, redisp=0;

#ifdef CLOCKTIME
  /* Turn on a clocktime timer */
  (void)start_timer ( dcon_timer_no );
#endif 

/*  set_hourglass_cursor(); */
  argnum = 1;
  if (argc>1)
    for (i=1; i<argc; i++)
      if (strcmp( argv[i], "redisplay" ) == 0)
        { /* doaxis = 0; */ redisp = 1; }
  if ((argc>1)&&(strcmp(argv[1],"plot")==0))
    { plotmode = 2;
      argnum++;
    }
  else
    plotmode = 1;

/*  No gray scale on Tektronix terminals (for now)  */

/*  argnum = 1; */
  if (Wistek() && plotmode == 1 && argc>argnum)
      for (i=argnum; i<argc; i++)
        if (strcmp( argv[i], "gray" ) == 0) {
          Werrprintf( "No gray scale display on Tektronix, sorry!" );
          return 1;
        }
if (!redisp)
{
  if (i_dcon(plotmode)) 
    { endgraphics();
      return 1;
    }
  if (plot && !raster)
    { Werrprintf("Image printing only available on raster printers");
      return 1;
    }
  if (!plot)
    { Wgmode(); 		/* goto tek graphics and set screen 2 active */
      if (argv[0][4]!='n') 
        { Wclear_graphics();
          show_plotterbox();
        }
      if (argv[0][4]=='n') graph_batch(1);
      Wshow_graphics();
      dcon_displayparms();
    }
}
  if (!plot) {
     grf_batch(1);
     if (redisp)
        erase_dcon_box();
  }

  linearflag = 0;
  doaxis = 1;

  /* set default color flag */
  rev_phase = get_phase_mode(get_direction(REVDIR));
  rev_phaseangle = get_phaseangle_mode(get_direction(REVDIR));
  norm_phase = get_phase_mode(get_direction(NORMDIR));
  norm_phaseangle = get_phaseangle_mode(get_direction(NORMDIR));
  if ( (d2flag) && (datahead.status & S_SPEC) )
  {
     if (datahead.status & S_HYPERCOMPLEX)
     {
        dcon_colorflag = ( (rev_phase && norm_phase) ? SELECT_PH_COLOR
				: SELECT_AV_COLOR );
     }
     else
     {
        dcon_colorflag = ( (rev_phase || rev_phaseangle) ? SELECT_PH_COLOR : SELECT_AV_COLOR );
     }
  }
  else
  {
     dcon_colorflag = ( (norm_phase || norm_phaseangle) ? SELECT_PH_COLOR : SELECT_AV_COLOR );
  }

  if (argc>argnum)
    { for (i=argnum; i<argc; i++)
        { if (strcmp(argv[i],"linear")==0) linearflag = 1;
          else if (strcmp(argv[i],"phcolor")==0)
            dcon_colorflag = SELECT_PH_COLOR;
          else if (strcmp(argv[i],"avcolor")==0)
            dcon_colorflag = SELECT_AV_COLOR;
          else if (strcmp(argv[i],"spectrum")==0){
            dcon_colorflag = 0;
          }
          else if (strcmp(argv[i],"gray")==0) 
            dcon_colorflag = SELECT_GRAY_COLOR;
          else if (strcmp(argv[i],"noaxis") == 0)
            doaxis = 0;
          else if (strcmp(argv[i],"redisplay") == 0)
            ; 
          else
            { Werrprintf(
                "usage - dcon(x,x,..) where x=linear,phcolor,avcolor,gray,spectrum");
	      graph_batch(0);
              return 1;
            }
        }
    }
if (!redisp)
{
  if (dcon_colorflag==SELECT_PH_COLOR)
    { if (linearflag) 
        { Werrprintf("linear flag ignored in phcolor mode\n");
          linearflag = 0;
        }
    }
if (dcon_colorflag==SELECT_PH_COLOR)
  { if (th > MAX_PH_THRESH)
      { th = MAX_PH_THRESH;
        P_setreal(CURRENT,"th",th,1);
      }
  }
else
  { if (th > MAX_AV_THRESH)
      { th = MAX_AV_THRESH;
        P_setreal(CURRENT,"th",th,1);
      }
  }
if (th<0)
  { th = 0;
    P_setreal(CURRENT,"th",th,1);
  }
if (dcon_colorflag==SELECT_GRAY_COLOR)
{
  graysl = 1.0;
  graycntr = (double) NUM_GRAY_COLORS / 2;
  gray_p_flag=P_getreal(CURRENT,"grayctr",&graycntr,1);
  fg1 = P_getreal(CURRENT,"graysl",&graysl,1);
  gray_p_flag = !(gray_p_flag && fg1);
}
}

if (plot)
  { if (dcon_colorflag==SELECT_GRAY_COLOR)
      dcon_num_colors = 64;
    else
      dcon_num_colors = 1;
    dcon_first_color = 1;

    dcon_colorflag = SELECT_BOX_COLOR;
  }
else if (!WisSunColor() && (dcon_colorflag==SELECT_GRAY_COLOR))
  { dcon_first_color = 1;
    dcon_num_colors = 64;
  }
else
{
  if (!(WisSunColor() || Wistek()))
  {
     if (dcon_colorflag == SELECT_AV_COLOR)
     {
        if ( (d2flag) && (datahead.status & S_SPEC) )
        {
           if (rev_phase)
              rev_phase++;
           if (rev_phaseangle)
              rev_phaseangle++;
           if (datahead.status & S_HYPERCOMPLEX)
           {
              if (norm_phase)
                 norm_phase++;
              if (norm_phaseangle)
                 norm_phaseangle++;
           }
        }
        else
        {
           if (norm_phase)
              norm_phase++;
           if (norm_phaseangle)
              norm_phaseangle++;
        }
     }

     dcon_colorflag = SELECT_BOX_COLOR;
  }

  switch (dcon_colorflag)
  { default:
    case SELECT_BOX_COLOR: dcon_first_color = 1;
                          dcon_num_colors  = 1;
                          break;
    case SELECT_AV_COLOR: dcon_first_color = FIRST_AV_COLOR;
                          dcon_num_colors  = NUM_AV_COLORS;
			  if ( (d2flag) && (datahead.status & S_SPEC) )
			  {
			     if (rev_phase)
			        rev_phase++;
			     if (rev_phaseangle)
			        rev_phaseangle++;
                             if (datahead.status & S_HYPERCOMPLEX)
                             {
                                if (norm_phase)
                                   norm_phase++;
                                if (norm_phaseangle)
                                   norm_phaseangle++;
                             }
			  }
			  else
			  {
                             if (norm_phase)
			        norm_phase++;
                             if (norm_phaseangle)
			        norm_phaseangle++;
			  }
			if (!redisp)
			{
                          if (th>dcon_num_colors-1)
                            th=dcon_num_colors-1;
			}
                          break;
    case SELECT_PH_COLOR: 
			if (!redisp)
			{
			  dcon_first_color = FIRST_PH_COLOR;
                          dcon_num_colors  = NUM_PH_COLORS;
                          if (th>(dcon_num_colors/2) -1)
                            th = (dcon_num_colors/2) -1;
			}
                          break;
    case SELECT_GRAY_COLOR:
			if (!redisp)
			{
			  dcon_first_color = FIRST_GRAY_COLOR;
                          dcon_num_colors  = NUM_GRAY_COLORS;
                          th = 0;
			}
                          break;
  }
}
if (!redisp)
{
  if ((!WisSunColor() || plot) && (dcon_colorflag==SELECT_GRAY_COLOR))
    dcon_colorflag = SELECT_BOX_COLOR;
  if ((WisSunColor() || Wistek()) && !plot)
    {
      if (dcon_colorflag==SELECT_GRAY_COLOR)
        change_contrast(graycntr,graysl);
      else
        setcolormap(dcon_first_color,dcon_num_colors,
            (int)th,dcon_colorflag==SELECT_PH_COLOR);
    }
  if (plot)
      set_gray_offset(graycntr,graysl);
}
  if ( (d2flag) && (datahead.status & S_SPEC) )
  {
     if (datahead.status & S_HYPERCOMPLEX)
     {
        if ((rev_phase > 1) && (norm_phase > 1))
        {
           pos_only = phflag = TRUE;
        }
        else
        {
           pos_only = FALSE;
           phflag = (rev_phase && norm_phase);
        }
     }
     else
     {
        if ((rev_phase > 1) || (rev_phaseangle > 1))
        {
           pos_only = phflag = TRUE;
        }
        else
        {
           pos_only = FALSE;
           phflag = rev_phase;
           if (rev_phaseangle == 1) phflag = rev_phaseangle;
        }
     }
  }
  else
  {
     if ((norm_phase > 1) || (norm_phaseangle > 1))
     {
       pos_only = phflag = TRUE;
     }
     else
     {
       pos_only = FALSE;
       phflag = norm_phase;
       if (norm_phaseangle == 1) phflag = norm_phaseangle;
     }
  }
  if (doaxis && !redisp)
     scale2d(1,0,1,SCALE_COLOR);
                        /* first argument causes the entire box to be drawn  */
                        /* second argument is the vertical offset of the box */
                        /* see above for explanation of third argument       */
                        /* fourth argument is the color                      */
  filled_display(phflag,pos_only);
  if (redisp) {
      if (!plot)
          graph_batch(0);
      return 0;
  }
  releaseAllWithId("dcon");
  D_allrelease();
  if (!plot)
  {
    Wsetgraphicsdisplay("dcon");
    disp_specIndex(specIndex);
#ifdef X11
    backing_up();
#endif 
    graph_batch(0);
  }
  endgraphics();

#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  (void)stop_timer ( dcon_timer_no );
#endif 

/*  restore_original_cursor(); */
  return 0;
}

static char  *pixbufpnt;
static float *maxbufpnt;
static int   maxl,minl,index_c,pixbufsize;
static void linear_map();
static void log_map();
static void screen_rasters();
static void mono_boxes();
static void shaded_boxes();
static int cy,ly;

/* STEP lets you use long integers instead of floats */
/* it is this big for rounding errors */
#define STEP	262144

/*********************/
static int filled_display(int phflag, int pos_only)
/*********************/
{ 
  int ctrace,line_frac;
  register int i;
  register float *p,ydata;
  float *phasfl;
  float vs1,fcy;
  float expf2x;
  void (*mop)(),(*gop)();

  expf2x = ((float) dnpnt2)/((float) npnt1); 
  /* expf2x = (float)(dnpnt2-1)/(float)(npnt1); works with one line short */
  /***********************************************************************/
  /* y compression ratio  how many lines of y data go into a screen line */
  /***********************************************************************/
  pixbufpnt = 0;
  /* plotters and monochrome go here */
  if (dcon_colorflag == SELECT_BOX_COLOR)
  {
     minl = 0;
     maxl = dcon_num_colors;
     index_c = 0;
     gop = shaded_boxes;
     if (dcon_num_colors == 1)
     {
       linearflag = TRUE;
       gop = mono_boxes;
     }
  }
  else
  {
     /* basically TEK and SUN HERE */
     gop = screen_rasters;
     if (phflag && !pos_only)
     {
	maxl = NUM_PH_COLORS / 2;
	minl = -maxl;
	index_c = ZERO_PHASE_LEVEL;
     }
     else
     {
	maxl = NUM_AV_COLORS - 1;
	minl = 0;
	index_c = FIRST_AV_COLOR;
     }
     if (dcon_colorflag == SELECT_GRAY_COLOR)
     {
	maxl = NUM_GRAY_COLORS-1;
	minl = 0;
	index_c = FIRST_GRAY_COLOR;
     }
  }
  /******************************************************************/
  /* optimally allocate space (the final frontier)		    */
  /******************************************************************/

  if ((dnpnt > npnt) && (dcon_colorflag != SELECT_BOX_COLOR))
    pixbufsize = dnpnt;
  else
    pixbufsize = npnt;

  pixbufsize += 4;	/* one long of insurance */
  if ((pixbufpnt = (char *)allocateWithId(sizeof(char)*pixbufsize,"dcon"))==0)
  { 
     Werrprintf("cannot allocate buffer");
     disp_status("    ");
     return 1;
  }
  if ((maxbufpnt = (float *)allocateWithId(sizeof(float)*npnt,"dcon"))==0)
  { 
     Werrprintf("cannot allocate maximum buffer");
     disp_status("    ");
     return 1;
  }

  vs1 = vs2d;
  if (!(WisSunColor() || Wistek()))
    for (i=0; i<(int)th; i++) vs1 /= 2.0;
  /*
  **	cy (int) is the bottom edge 
  **	of the line(s) on the screen
  **	ly (int) and fcy (float) track the phase file lines
  */
  fcy = (float)(dfpnt2);
  line_frac = dfpnt2*STEP;
  cy = dfpnt2;
  ly = cy;
  color(SPEC_COLOR);
  Wgmode();
  if (linearflag)
    mop = linear_map;
  else
    mop = log_map;
  disp_status("Cont");
  if (WisSunColor())
    init_rast(pixbufpnt,dnpnt,TRUE);
  /*********************************************************************/
  /*  the outermost loop goes throught the traces */
  /*  new buf flag ?? */
  /*  clear the buffer */
  /*********************************************************************/
  i = npnt; 
  p = maxbufpnt; 
  while (i--) 
    *p++ = 0.0;
  /*************************************************** 
  go through the requested traces 
  ***************************************************/
  for (ctrace = fpnt1; ctrace < fpnt1+npnt1; ctrace++)
  {
     if ((phasfl=gettrace(ctrace,fpnt))==0)
     {
        disp_status("    ");
        if (WisSunColor())
          init_rast(pixbufpnt,dnpnt,FALSE);
        return 1;
     }
     /* put it into the buffer   */
     /* this loop needs speed up */
     if (phflag && !pos_only)
     { 
       for (i=0; i<npnt; i++)
       { 
	 if (fabs(phasfl[i])>fabs(maxbufpnt[i]))
		    maxbufpnt[i] = phasfl[i];
       }
     }
     else
     { 
	/* speed this up too! */
	for (i=0; i<npnt; i++)
	{ 
	  if (phasfl[i]>maxbufpnt[i])
	  {
	     ydata = phasfl[i];
	     if (ydata < 0.0) /* clip it */
		maxbufpnt[i] = 0;
	     else 	    /* use it */
                maxbufpnt[i] = ydata;
	  }
        }
     }
     /*************************************************** 
     have we enough data for 1 or more screen lines ? 
	 have start line
	 add the line fraction 
	 truncate both
	 if difference > 0 draw it
	 else accumulate
     ***************************************************/
     fcy += expf2x;
   /*  line_frac += line_inc; */
   /*  cy = line_frac/STEP; */
     cy = (int) fcy;
  /**************************************************************/
  /*   do the mapping and the sending				*/
  /**************************************************************/
     if (cy>ly)
     {
       (*mop)();
       (*gop)(); 
       i = npnt; 
       p = maxbufpnt; 
       while (i--) 
         *p++ = 0.0;
       ly=cy;
     }
/*     if (ctrace == fpnt1+npnt1/2)
         flush( Gphsd ); */
  }
/*  flush( Gphsd ); */
  if (WisSunColor())
    init_rast(pixbufpnt,dnpnt,FALSE);
  disp_status("    ");
  return 0;
}   /* filled_display ends */

static void linear_map()
{
   int i,itemp; 
   float *dd;
   char  *cc;
   dd = maxbufpnt;
   cc = pixbufpnt;
   for (i=0; i < npnt; i++)
   {
     itemp = (int) (vs2d*(*dd++));
     if (itemp > maxl) 
	itemp = maxl;
     if (itemp < minl) 
	itemp = minl;
     *cc++ = (char) itemp;
   }
}

static void log_map()
{
   union u_tag {
      float ftdd;
      int   itdd;
   } uval;
   int i,itemp; 
   float *dd,tdd;
   char  *cc;
   dd = maxbufpnt;
   cc = pixbufpnt;
   for (i=0; i < npnt; i++)
   {
       /* Fast log */
       tdd = (vs2d*(*dd++));
       if (tdd < 0.0) 
       {
	/* you must negate the FLOAT!! */
        uval.ftdd = -tdd;
       /* IEEE */
       itemp = (uval.itdd >> 23) - 125;
       if (itemp > maxl)
	  itemp = maxl;
       if (itemp < 0) 
	 itemp = 0;
       itemp = -itemp;
       }
       else
       {
       /* IEEE */
       uval.ftdd = tdd;
       itemp = (uval.itdd >> 23) - 125;
       if (itemp > maxl)
	  itemp = maxl;
       if (itemp < 0) 
	 itemp = 0;
       }
       *cc++ = (char) itemp ;
    }
}

#ifdef NOTUSED
static void histo_map()
{
   int i,itemp; 
   float *dd;
   char  *cc;
   dd = maxbufpnt;
   cc = pixbufpnt;
   for (i=0; i < npnt; i++)
   {
     /* maxl minl 0 howmany scales vs2d invariant */
     /* action with vs2d */
     /* if histo_buf == NULL return */
     /* index = (int) (hvs*(*dd++)); */
     /* clip index - then itemp = *(histo_buf + index ) */
     /* histo buf should not require clipping */
     itemp = (int) (vs2d*(*dd++));
     if (itemp > maxl) 
	itemp = maxl;
     if (itemp < minl) 
	itemp = minl;
     *cc++ = (char) itemp;
   }
}
#endif
static void mono_boxes()
{
   /* the buffer should only have char 0 and 1*/
   /* there were npnt -1 's in the inner tests */
   int i,i0,inx;
   char *tpntr;
   tpntr = pixbufpnt;
   i = 0;
   inx = STEP*dnpnt/npnt; /* not (dnpnt-1)/(npnt-1) */
   while (i < npnt)
   {
     if (*tpntr != 0)
     { 
       i0 = i;
       while ((i<npnt) && (*tpntr != 0))
       {
	 i++;
         tpntr++;
       }
       while(((i-i0)*inx)< STEP)
       {
	 i++;
         tpntr++;
       }
       amove(dfpnt+(inx*i0)/STEP,ly); 
       box((inx*(i-i0))/STEP,cy-ly);
     }
     else /* don't send a zero */
     { 
       while ((i < npnt) && (*tpntr == 0))
       {
         tpntr++;
         i++;
       }
     }
   }
}

/* 
**	Handles both compress and expand 
*/
static void shaded_boxes()
{
  int i,i0,temp,inx;
  int d_s,d_e;
  char *tpntr;
  tpntr = pixbufpnt;
  i = 0;
  inx = (STEP*dnpnt)/npnt;       
  while (i<npnt)		 /* cover all data points  */
  {
    if (*tpntr !=0)
    { 
      i0 = i;
      temp = *tpntr;                  
      /* chain together any equal value boxes along X */
      while ((i<npnt) && (*tpntr == temp))
      {
	tpntr++;
	i++;
      }
      /* 
      find the maximum when compressing 
      */
      while(((i-i0)*inx)< STEP)
      {
	if (temp < *tpntr)
	  temp = *tpntr;
        i++;
	tpntr++;
      }
      d_s = dfpnt+(i0*inx)/STEP;
      d_e = dfpnt+(i*inx)/STEP;
      grayscale_box(d_s,ly,d_e-d_s,cy-ly,temp);
    }
    else
    { 
      /* skips zeros */
      while ((i<npnt) && (*tpntr == 0)) 
      {
	i++;
	tpntr++;
      }
    }
  }
}

static char *outp,*inp;
static int accx,inx;


static void screen_rasters()
{
   /* horizontal compress / expand */
   int i,ctmp,cmax,cmaxa;

   if (dnpnt > npnt)	/* expand */
   {
      inx  = (STEP*npnt)/dnpnt;  
      accx = 0;
      outp = pixbufpnt+dnpnt;  
      /* it appears that exp* npnt = dnpnt+1 ? */
      inp  = pixbufpnt+npnt-1;
      /* A right to left copy!! */
      i = npnt;
      while (--i)
      {
	 cmax = *inp-- + index_c;
	 while (accx < STEP)
	 {
	   *outp-- = cmax;
	   accx += inx;
	 }
	 accx -= STEP;
      }
      /* one block left now ensure outp does not underflow */
      cmax = *inp-- + index_c;
      while (outp >= pixbufpnt)
      {
	  *outp-- = cmax;
      }
   }
   else			/* compress */
   {
      i = dnpnt;
      inx  = (STEP*dnpnt)/npnt; 
      accx = 0;
      outp = pixbufpnt;
      inp = pixbufpnt;
      while (i--)
      {
	cmax  = 0;
	cmaxa = 0;
	while (accx < STEP)
	{
	   accx += inx;
	   ctmp = *inp++;
	   if (ctmp < 0) 
	   {
	     if (cmaxa < -ctmp)
	     {
	       cmax = ctmp;
	       cmaxa = -ctmp;
	     }
	   }
	   else
	   {
	     if (cmaxa < ctmp)
             {
	       cmax = ctmp;
	       cmaxa = cmax;
             }
	   }
	}
	*outp++ = cmax + index_c;
	accx -= STEP;
      }
   }
   rast(pixbufpnt,dnpnt,cy-ly,dfpnt,ly); 
}
