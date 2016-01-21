/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|	vcolor.c    
|
|		These routines are some more builtin commands or
|               procedures on the magical interpreter. They are 
|
+-----------------------------------------------------------------------------*/


#include "vnmrsys.h"
#include "graphics.h"
#include "group.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

#ifdef  DEBUG
extern int Tflag;
#define TPRINT0(str) \
	if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	if (Tflag) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(str, arg1, arg2, arg3) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3)
#define TPRINT4(str, arg1, arg2, arg3, arg4) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define TPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3,arg4,arg5)
#else 
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#define TPRINT4(str, arg1, arg2, arg3, arg4) 
#define TPRINT5(str, arg1, arg2, arg3, arg4, arg5) 
#endif 

extern void show_color(int num, int red, int grn, int blu);
extern int      BACK;

#ifdef INTERACT
static int      Bnmr = 1;
#else
extern int      Bnmr;
#endif

static int color_planes = 1;

static unsigned char rastColor[MAX_COLOR+1];
static unsigned char pens[MAX_COLOR+1];
static unsigned char ps_red[MAX_COLOR+1];
static unsigned char ps_grn[MAX_COLOR+1];
static unsigned char ps_blu[MAX_COLOR+1];
static unsigned char mred[MAX_COLOR+1];
static unsigned char mgrn[MAX_COLOR+1];
static unsigned char mblu[MAX_COLOR+1];
static int isInitialized = 0;
static int graphicsColorLoaded = 0;
static int psColorLoaded = 0;

int colorIndexNoPen(char *colorname, int *index);

#define HPGL 1
#define PCL  2
#define PS   3
#define RAST 4
#define PEN  5


/*
 * test value of item to change color
 * a negative number is returned if an error occurs
 */
static int getItemIndex(char *val)
{
   int ival = -1;

   if (isReal(val))
   {
      int tmp;
      tmp = (int) stringReal(val);
      if ((tmp >= 0) && (tmp <= MAX_COLOR))
         ival = tmp;
      else if (tmp > MAX_COLOR) ival = -tmp; // do nothing if ival is negative.
   }
   return(ival);
}

static int devtype(char *val)
{
   int ival = -1;

   if (!strcmp("graphics",val) || !strcmp("newgraphics",val))
      ival = RAST;
   else if (!strcmp("pcl",val))
      ival = PCL;
   else if (!strcmp("ps",val))
      ival = PS;
   else if (!strcmp("hpgl",val))
      ival = HPGL;
   else if (!strcmp("pen",val))
      ival = PEN;
   return(ival);
}

/*
 * test value of color to be between 0 and 255
 * a negative number is returned if an error occurs
 */
static int get_color_val(char *val)
{
   int ival = -1;

   if (isReal(val))
      ival = (int) stringReal(val);
   if (ival > 255)
      ival = -1;
   if (ival < 0)
      Werrprintf("color %s is out of range",val);
   return(ival);
}

static void set_ps_color(int index, int red, int grn, int blu)
{
   if (index > psColorLoaded)
      psColorLoaded = index;
   ps_red[index] = red;   ps_grn[index] = grn; ps_blu[index] = blu;
}

static void set_color(int index, int red, int grn, int blu)
{
   if (index > graphicsColorLoaded)
      graphicsColorLoaded = index;
   mred[index] = red;   mgrn[index] = grn; mblu[index] = blu;
}

/*------------------------------------------------------------------------------
|
|	setcolor
|
|	This procedure sets colors of graphics
|	Usage  setcolor(keyword,red,green,blue)
|
+-----------------------------------------------------------------------------*/
int setcolor(int argc, char *argv[], int retc, char *retv[])
{   
   int dev_type;
   int itemIndex;
   int colorval;
   int red,grn,blu;
   extern FILE *plotfile;

   (void) retc;
   (void) retv;
   if (argc < 3)
   {
      Werrprintf("setcolor requires at least two arguments");
      RETURN;
   }
   if (strcmp(argv[1],"plotter") == 0)
   {
      if (!plotfile)
      {
         if ( (argc == 4) && isReal(argv[2]) && isReal(argv[3]) )
         {
            color_planes  = (int) stringReal(argv[2]);
            color_planes += (int) stringReal(argv[3]);
            if (color_planes < 1)
               color_planes = 1;
            else if (color_planes > 4)
               color_planes = 4;
         }
         else
         {
            Werrprintf("%s with option %s requires bw and color planes",
                           argv[0],argv[1]);
         }
      }
      else
      {
         Werrprintf("Cannot change plotter configuration with active plot. Issue page command.");
      }
   }
   else if ((dev_type = devtype(argv[1])) != -1)
   {
      if ( (itemIndex = getItemIndex(argv[2])) < 0 )
      {
         // Werrprintf("improper item index '%s' provided as second argument to setcolor", argv[2]);
         RETURN;
      }
      else
      {
         switch (dev_type) {
            case HPGL: if (argc == 4)
                       {
                          if (colorIndexNoPen(argv[3],&colorval))
                             pens[itemIndex] = colorval;
                       }
                       break;
            case PCL:  if (argc == 4)
                       {
                          if (colorIndexNoPen(argv[3],&colorval))
                             rastColor[itemIndex] = colorval;
                       }
                       break;
            case PS:   
                       if (argc == 6)
                       {
                          red = get_color_val(argv[3]);
                          grn = get_color_val(argv[4]);
                          blu = get_color_val(argv[5]);
                          if ( (red >= 0) && (grn >= 0) && (blu >= 0) )
                          {
                             set_ps_color(itemIndex,red,grn,blu);
                          }
                       }
                       break;
            case RAST: if ( !Bnmr && (argc == 6))
                       {
                          red = get_color_val(argv[3]);
                          grn = get_color_val(argv[4]);
                          blu = get_color_val(argv[5]);
                          if ( (red >= 0) && (grn >= 0) && (blu >= 0) )
                          {
                             set_color(itemIndex,red,grn,blu);
                             if (Wissun())
                             {
                                show_color(itemIndex,red,grn,blu);
                             }
                          }
                       }
                       break;
            case PEN:  if (argc == 4)
                       {
                          if (colorIndexNoPen(argv[3],&colorval))
                             pens[colorval] = itemIndex;
                       }
                       break;
         }
      }
   }
   RETURN;
}

int graphics_colors_loaded()
{
    return graphicsColorLoaded;
}

int ps_colors_loaded()
{
    return psColorLoaded;
}

void
get_color(int index, int *red, int *grn, int *blu)
{
   if ((index < 0) || (index > MAX_COLOR))
       index = 0;
   *red = mred[index];   *grn = mgrn[index]; *blu = mblu[index];
}

int get_raster_color(int index)
{
   if (color_planes < 2)
      return(MONOCHROME);
   return((index > MAX_COLOR) ? BLACK : rastColor[index]);
}

unsigned char
*get_red_colors()
{
    return mred;
}

unsigned char
*get_green_colors()
{
    return mgrn;
}

unsigned char
*get_blue_colors()
{
    return mblu;
}

unsigned char
*get_ps_red_colors()
{
    return ps_red;
}

unsigned char
*get_ps_green_colors()
{
    return ps_grn;
}

unsigned char
*get_ps_blue_colors()
{
    return ps_blu;
}

void get_ps_color(int index, double *red, double *grn, double *blu)
{
   if ((index < 0) || (index > MAX_COLOR) || (color_planes < 2) )
   {
     *red = *grn = *blu = 0.0;
   }
   else
   {
      *red = (double) (ps_red[index]) / 255.0;
      *grn = (double) (ps_grn[index]) / 255.0;
      *blu = (double) (ps_blu[index]) / 255.0;
   }
}

int get_pen_color(int index)
{
   if (index > MAX_COLOR)
      return(1);
   if (index < 0)
   {
      index *= -1;
      return((index > 8) ? 8 : index);
   }
   index = pens[index];
   return((index > MAX_COLOR) ? 1 : pens[index]);
}

int get_plot_planes()
{
   return(color_planes);
}

void set_plot_planes(int n)
{
    color_planes = n;
    if (color_planes < 1)
        color_planes = 1;
    else if (color_planes > 4)
        color_planes = 4;
}

void
init_colors()
{
   int n;

   if (isInitialized)
        return;
   isInitialized = 1;
   for (n = 256; n <= MAX_COLOR; n++) {
      ps_red[n] = 1;
      ps_grn[n] = 1;
      ps_blu[n] = 1;
   }
   mred[BLACK]   = 0;	mgrn[BLACK]   = 0;	mblu[BLACK]   = 0;
   mred[RED]     = 255;	mgrn[RED]     = 0;	mblu[RED]     = 0;
   mred[YELLOW]  = 255;	mgrn[YELLOW]  = 255;	mblu[YELLOW]  = 0;
   mred[GREEN]   = 0;	mgrn[GREEN]   = 255;	mblu[GREEN]   = 0;
   mred[CYAN]    = 0;	mgrn[CYAN]    = 255;	mblu[CYAN]    = 255;
   mred[BLUE]    = 0;	mgrn[BLUE]    = 0;	mblu[BLUE]    = 255;
   mred[MAGENTA] = 255;	mgrn[MAGENTA] = 0;	mblu[MAGENTA] = 255;
   mred[ORANGE]  = 255;	mgrn[ORANGE]  = 165;	mblu[ORANGE]  = 0;
   mred[WHITE]   = 255;	mgrn[WHITE]   = 255;	mblu[WHITE]   = 255;

   mred[FID_COLOR] = 0;	   mgrn[FID_COLOR] = 255;    mblu[FID_COLOR] = 255;
   mred[IMAG_COLOR] = 255; mgrn[IMAG_COLOR] = 255;   mblu[IMAG_COLOR] = 0;
   mred[SPEC_COLOR] = 0;   mgrn[SPEC_COLOR] = 255;   mblu[SPEC_COLOR] = 255;
   mred[INT_COLOR] = 0;	   mgrn[INT_COLOR] = 255;    mblu[INT_COLOR] = 0;
   mred[CURSOR_COLOR] = 255; mgrn[CURSOR_COLOR] = 0;   mblu[CURSOR_COLOR] = 0;
   mred[PARAM_COLOR] = 255;  mgrn[PARAM_COLOR] = 255;  mblu[PARAM_COLOR] = 0;
   mred[SCALE_COLOR] = 255;  mgrn[SCALE_COLOR] = 255;  mblu[SCALE_COLOR] = 255;
   mred[THRESH_COLOR] = 255; mgrn[THRESH_COLOR] = 255; mblu[THRESH_COLOR] = 0;
   mred[SPEC2_COLOR] = 0;   mgrn[SPEC2_COLOR] = 255;    mblu[SPEC2_COLOR] = 0;
   mred[SPEC3_COLOR] = 255;   mgrn[SPEC3_COLOR] = 255;    mblu[SPEC3_COLOR] = 0;
   mred[BG_IMAGE_COLOR] = 0; mgrn[BG_IMAGE_COLOR] = 0; mblu[BG_IMAGE_COLOR] = 0;
   mred[FG_IMAGE_COLOR] = 255; mgrn[FG_IMAGE_COLOR] = 255; mblu[FG_IMAGE_COLOR] = 255;
   mred[ABSVAL_FID_COLOR] = 255; mgrn[ABSVAL_FID_COLOR] = 0; mblu[ABSVAL_FID_COLOR] = 255;
   mred[FID_ENVEL_COLOR] = 0; mgrn[FID_ENVEL_COLOR] = 0; mblu[FID_ENVEL_COLOR] = 255;


/* 2D absolute value */
   mred[CONT0_COLOR] = 0; mgrn[CONT0_COLOR] = 0;	mblu[CONT0_COLOR] = 0; 
   mred[CONT1_COLOR] = 140; mgrn[CONT1_COLOR] = 0;	mblu[CONT1_COLOR] = 181;
   mred[CONT2_COLOR] = 100; mgrn[CONT2_COLOR] = 100; mblu[CONT2_COLOR] = 255;
   mred[CONT3_COLOR] = 0; mgrn[CONT3_COLOR] = 150; mblu[CONT3_COLOR] = 255;
   mred[CONT4_COLOR] = 0; mgrn[CONT4_COLOR] = 200; mblu[CONT4_COLOR] = 200;
   mred[CONT5_COLOR] = 0;  mgrn[CONT5_COLOR] = 240;	mblu[CONT5_COLOR] = 240;
   mred[CONT6_COLOR] = 0;  mgrn[CONT6_COLOR] = 240;	mblu[CONT6_COLOR] = 0;
   mred[CONT7_COLOR] = 200; mgrn[CONT7_COLOR] = 255;	mblu[CONT7_COLOR] = 0;
   mred[CONT8_COLOR] = 235; mgrn[CONT8_COLOR] = 235;	mblu[CONT8_COLOR] = 0;
   mred[CONT9_COLOR] = 255; mgrn[CONT9_COLOR] = 225;	mblu[CONT9_COLOR] = 0;
   mred[CONT10_COLOR] = 255; mgrn[CONT10_COLOR] = 160;	mblu[CONT10_COLOR] = 40;
   mred[CONT11_COLOR] = 255; mgrn[CONT11_COLOR] = 115;	mblu[CONT11_COLOR] = 0;
   mred[CONT12_COLOR] = 255; mgrn[CONT12_COLOR] = 0;	mblu[CONT12_COLOR] = 0; 
   mred[CONT13_COLOR] = 255; mgrn[CONT13_COLOR] = 125; mblu[CONT13_COLOR] = 165;
   mred[CONT14_COLOR] = 255; mgrn[CONT14_COLOR] = 165; mblu[CONT14_COLOR] = 220;
   mred[CONT15_COLOR] = 255; mgrn[CONT15_COLOR] = 255; mblu[CONT15_COLOR] = 255;
/* 2D phased display */
   mred[CONTM7_COLOR] = 0; mgrn[CONTM7_COLOR] = 255; mblu[CONTM7_COLOR] = 255;
   mred[CONTM6_COLOR] = 0; mgrn[CONTM6_COLOR] = 195; mblu[CONTM6_COLOR] = 255;
   mred[CONTM5_COLOR] = 0; mgrn[CONTM5_COLOR] = 150; mblu[CONTM5_COLOR] = 255;
   mred[CONTM4_COLOR] = 0; mgrn[CONTM4_COLOR] = 0; mblu[CONTM4_COLOR] = 255;
   mred[CONTM3_COLOR] = 125; mgrn[CONTM3_COLOR] = 70; mblu[CONTM3_COLOR] = 255;
   mred[CONTM2_COLOR] = 180; mgrn[CONTM2_COLOR] = 130; mblu[CONTM2_COLOR] = 220;
   mred[CONTM1_COLOR] = 210; mgrn[CONTM1_COLOR] = 0; mblu[CONTM1_COLOR] = 255;
   mred[CONTP0_COLOR] = 0; mgrn[CONTP0_COLOR] = 0;	mblu[CONTP0_COLOR] = 0;
   mred[CONTP1_COLOR] = 210; mgrn[CONTP1_COLOR] = 210;	mblu[CONTP1_COLOR] = 0;
   mred[CONTP2_COLOR] = 255; mgrn[CONTP2_COLOR] = 255;	mblu[CONTP2_COLOR] = 0;
   mred[CONTP3_COLOR] = 255; mgrn[CONTP3_COLOR] = 215;	mblu[CONTP3_COLOR] = 0;
   mred[CONTP4_COLOR] = 255; mgrn[CONTP4_COLOR] = 185; mblu[CONTP4_COLOR] = 115;
   mred[CONTP5_COLOR] = 255; mgrn[CONTP5_COLOR] = 145;	mblu[CONTP5_COLOR] = 50;
   mred[CONTP6_COLOR] = 255; mgrn[CONTP6_COLOR] = 0;	mblu[CONTP6_COLOR] = 0;
   mred[CONTP7_COLOR] = 255; mgrn[CONTP7_COLOR] = 255; mblu[CONTP7_COLOR] = 255;
   mred[PEAK_COLOR] = 255; mgrn[PEAK_COLOR] = 0; mblu[PEAK_COLOR] = 0;
   mred[NUM_COLOR] = 0; mgrn[NUM_COLOR] = 255; mblu[NUM_COLOR] = 255;
   mred[AV_BOX_COLOR] = 255; mgrn[AV_BOX_COLOR] = 0; mblu[AV_BOX_COLOR] = 0;
   mred[PH_BOX_COLOR] = 255; mgrn[PH_BOX_COLOR] = 0; mblu[PH_BOX_COLOR] = 0;
   mred[LABEL_COLOR] = 255; mgrn[LABEL_COLOR] = 255; mblu[LABEL_COLOR] = 0;
   mred[XPLANE_COLOR] = 0; mgrn[XPLANE_COLOR] = 255; mblu[XPLANE_COLOR] = 0;
   mred[YPLANE_COLOR] = 255; mgrn[YPLANE_COLOR] = 255; mblu[YPLANE_COLOR] = 0;
   mred[ZPLANE_COLOR] = 0; mgrn[ZPLANE_COLOR] = 255; mblu[ZPLANE_COLOR] = 255;
   mred[UPLANE_COLOR] = 255; mgrn[UPLANE_COLOR] = 255; mblu[UPLANE_COLOR] = 0;
   mred[PINK_COLOR] = 255; mgrn[PINK_COLOR] = 204; mblu[PINK_COLOR] = 204;
   mred[GRAY_COLOR] = 200; mgrn[GRAY_COLOR] = 200; mblu[GRAY_COLOR] = 200;

   ps_red[BLACK]   = 0;	ps_grn[BLACK]   = 0;	ps_blu[BLACK]   = 0;
   ps_red[RED]     = 255;	ps_grn[RED]     = 0;	ps_blu[RED]     = 0;
   ps_red[YELLOW]  = 255;	ps_grn[YELLOW]  = 255;	ps_blu[YELLOW]  = 0;
   ps_red[GREEN]   = 0;	ps_grn[GREEN]   = 255;	ps_blu[GREEN]   = 0;
   ps_red[CYAN]    = 0;	ps_grn[CYAN]    = 255;	ps_blu[CYAN]    = 255;
   ps_red[BLUE]    = 0;	ps_grn[BLUE]    = 0;	ps_blu[BLUE]    = 255;
   ps_red[MAGENTA] = 255;	ps_grn[MAGENTA] = 0;	ps_blu[MAGENTA] = 255;
   ps_red[ORANGE]  = 255;	ps_grn[ORANGE]  = 165;	ps_blu[ORANGE]  = 0;
   ps_red[WHITE]   = 255;	ps_grn[WHITE]   = 255;	ps_blu[WHITE]   = 255;

   ps_red[FID_COLOR] = 0;   ps_grn[FID_COLOR] = 255;    ps_blu[FID_COLOR] = 255;
   ps_red[IMAG_COLOR] = 255; ps_grn[IMAG_COLOR] = 255;   ps_blu[IMAG_COLOR] = 0;
   ps_red[SPEC_COLOR] = 0;   ps_grn[SPEC_COLOR] = 255;   ps_blu[SPEC_COLOR] = 255;
   ps_red[INT_COLOR] = 0;   ps_grn[INT_COLOR] = 255;    ps_blu[INT_COLOR] = 0;
   ps_red[CURSOR_COLOR] = 255; ps_grn[CURSOR_COLOR] = 0;   ps_blu[CURSOR_COLOR] = 0;
   ps_red[PARAM_COLOR] = 120;  ps_grn[PARAM_COLOR] = 120;  ps_blu[PARAM_COLOR] = 0;
   ps_red[SCALE_COLOR] = 0;  ps_grn[SCALE_COLOR] = 0;  ps_blu[SCALE_COLOR] = 0;
   ps_red[THRESH_COLOR] = 120; ps_grn[THRESH_COLOR] = 120; ps_blu[THRESH_COLOR] = 0;
   ps_red[SPEC2_COLOR] = 0;   ps_grn[SPEC2_COLOR] = 255;    ps_blu[SPEC2_COLOR] = 0;
   ps_red[SPEC3_COLOR] = 120;   ps_grn[SPEC3_COLOR] = 120;    ps_blu[SPEC3_COLOR] = 0;
   ps_red[BG_IMAGE_COLOR] = 0; ps_grn[BG_IMAGE_COLOR] = 0; ps_blu[BG_IMAGE_COLOR] = 0;
   ps_red[FG_IMAGE_COLOR] = 255; ps_grn[FG_IMAGE_COLOR] = 255; ps_blu[FG_IMAGE_COLOR] = 255;

/* 2D absolute value */
   ps_red[CONT0_COLOR] = 0; ps_grn[CONT0_COLOR] = 0;	ps_blu[CONT0_COLOR] = 0; 
   ps_red[CONT1_COLOR] = 140; ps_grn[CONT1_COLOR] = 0;	ps_blu[CONT1_COLOR] = 181;
   ps_red[CONT2_COLOR] = 100; ps_grn[CONT2_COLOR] = 100; ps_blu[CONT2_COLOR] = 255;
   ps_red[CONT3_COLOR] = 0; ps_grn[CONT3_COLOR] = 150; ps_blu[CONT3_COLOR] = 255;
   ps_red[CONT4_COLOR] = 0; ps_grn[CONT4_COLOR] = 200; ps_blu[CONT4_COLOR] = 200;
   ps_red[CONT5_COLOR] = 0;  ps_grn[CONT5_COLOR] = 240;	ps_blu[CONT5_COLOR] = 240;
   ps_red[CONT6_COLOR] = 0;  ps_grn[CONT6_COLOR] = 240;	ps_blu[CONT6_COLOR] = 0;
   ps_red[CONT7_COLOR] = 200; ps_grn[CONT7_COLOR] = 255;	ps_blu[CONT7_COLOR] = 0;
   ps_red[CONT8_COLOR] = 235; ps_grn[CONT8_COLOR] = 235;	ps_blu[CONT8_COLOR] = 0;
   ps_red[CONT9_COLOR] = 255; ps_grn[CONT9_COLOR] = 225;	ps_blu[CONT9_COLOR] = 0;
   ps_red[CONT10_COLOR] = 255; ps_grn[CONT10_COLOR] = 160;	ps_blu[CONT10_COLOR] = 40;
   ps_red[CONT11_COLOR] = 255; ps_grn[CONT11_COLOR] = 115;	ps_blu[CONT11_COLOR] = 0;
   ps_red[CONT12_COLOR] = 255; ps_grn[CONT12_COLOR] = 0;	ps_blu[CONT12_COLOR] = 0; 
   ps_red[CONT13_COLOR] = 255; ps_grn[CONT13_COLOR] = 125; ps_blu[CONT13_COLOR] = 165;
   ps_red[CONT14_COLOR] = 255; ps_grn[CONT14_COLOR] = 165; ps_blu[CONT14_COLOR] = 220;
   ps_red[CONT15_COLOR] = 255; ps_grn[CONT15_COLOR] = 255; ps_blu[CONT15_COLOR] = 255;
/* 2D phased display */
   ps_red[CONTM7_COLOR] = 0; ps_grn[CONTM7_COLOR] = 255; ps_blu[CONTM7_COLOR] = 255;
   ps_red[CONTM6_COLOR] = 0; ps_grn[CONTM6_COLOR] = 195; ps_blu[CONTM6_COLOR] = 255;
   ps_red[CONTM5_COLOR] = 0; ps_grn[CONTM5_COLOR] = 150; ps_blu[CONTM5_COLOR] = 255;
   ps_red[CONTM4_COLOR] = 0; ps_grn[CONTM4_COLOR] = 0; ps_blu[CONTM4_COLOR] = 255;
   ps_red[CONTM3_COLOR] = 125; ps_grn[CONTM3_COLOR] = 70; ps_blu[CONTM3_COLOR] = 255;
   ps_red[CONTM2_COLOR] = 180; ps_grn[CONTM2_COLOR] = 130; ps_blu[CONTM2_COLOR] = 220;
   ps_red[CONTM1_COLOR] = 210; ps_grn[CONTM1_COLOR] = 0; ps_blu[CONTM1_COLOR] = 255;
   ps_red[CONTP0_COLOR] = 0; ps_grn[CONTP0_COLOR] = 0;	ps_blu[CONTP0_COLOR] = 0;
   ps_red[CONTP1_COLOR] = 210; ps_grn[CONTP1_COLOR] = 210;	ps_blu[CONTP1_COLOR] = 0;
   ps_red[CONTP2_COLOR] = 255; ps_grn[CONTP2_COLOR] = 255;	ps_blu[CONTP2_COLOR] = 0;
   ps_red[CONTP3_COLOR] = 255; ps_grn[CONTP3_COLOR] = 215;	ps_blu[CONTP3_COLOR] = 0;
   ps_red[CONTP4_COLOR] = 255; ps_grn[CONTP4_COLOR] = 185; ps_blu[CONTP4_COLOR] = 115;
   ps_red[CONTP5_COLOR] = 255; ps_grn[CONTP5_COLOR] = 145;	ps_blu[CONTP5_COLOR] = 50;
   ps_red[CONTP6_COLOR] = 255; ps_grn[CONTP6_COLOR] = 0;	ps_blu[CONTP6_COLOR] = 0;
   ps_red[CONTP7_COLOR] = 255; ps_grn[CONTP7_COLOR] = 255; ps_blu[CONTP7_COLOR] = 255;
   ps_red[PEAK_COLOR] = 255; ps_grn[PEAK_COLOR] = 0; ps_blu[PEAK_COLOR] = 0;
   ps_red[NUM_COLOR] = 0; ps_grn[NUM_COLOR] = 255; ps_blu[NUM_COLOR] = 255;
   ps_red[AV_BOX_COLOR] = 255; ps_grn[AV_BOX_COLOR] = 0; ps_blu[AV_BOX_COLOR] = 0;
   ps_red[PH_BOX_COLOR] = 255; ps_grn[PH_BOX_COLOR] = 0; ps_blu[PH_BOX_COLOR] = 0;
   ps_red[LABEL_COLOR] = 255; ps_grn[LABEL_COLOR] = 255; ps_blu[LABEL_COLOR] = 0;
   ps_red[PINK_COLOR] = 255; ps_grn[PINK_COLOR] = 204; ps_blu[PINK_COLOR] = 204;
   ps_red[GRAY_COLOR] = 200; ps_grn[GRAY_COLOR] = 200; ps_blu[GRAY_COLOR] = 200;

   rastColor[BLACK]   = BLACK;
   rastColor[RED]     = RED;
   rastColor[YELLOW]  = YELLOW;
   rastColor[GREEN]   = GREEN;
   rastColor[CYAN]    = CYAN;
   rastColor[BLUE]    = BLUE;
   rastColor[MAGENTA] = MAGENTA;
   rastColor[ORANGE]  = ORANGE;
   rastColor[WHITE]   = WHITE;

   rastColor[FID_COLOR] = CYAN;
   rastColor[IMAG_COLOR] = GREEN;
   rastColor[SPEC_COLOR] = CYAN;
   rastColor[INT_COLOR] = BLUE;
   rastColor[CURSOR_COLOR] = RED;
   rastColor[PARAM_COLOR] = BLACK;
   rastColor[SCALE_COLOR] = RED;
   rastColor[THRESH_COLOR] = YELLOW;
   rastColor[SPEC2_COLOR] = GREEN;
   rastColor[SPEC3_COLOR] = YELLOW;
   rastColor[BG_IMAGE_COLOR] = BLACK;
   rastColor[FG_IMAGE_COLOR] = WHITE;

/* 2D absolute value */
   rastColor[CONT0_COLOR] = BLACK;
   rastColor[CONT1_COLOR] = BLUE;
   rastColor[CONT2_COLOR] = CYAN;
   rastColor[CONT3_COLOR] = GREEN;
   rastColor[CONT4_COLOR] = MAGENTA;
   rastColor[CONT5_COLOR] = RED;
   rastColor[CONT6_COLOR] = YELLOW;
   rastColor[CONT7_COLOR] = BLACK;
   rastColor[CONT8_COLOR] = BLUE;
   rastColor[CONT9_COLOR] = CYAN;
   rastColor[CONT10_COLOR] = GREEN;
   rastColor[CONT11_COLOR] = MAGENTA;
   rastColor[CONT12_COLOR] = RED;
   rastColor[CONT13_COLOR] = YELLOW;
   rastColor[CONT14_COLOR] = BLACK;
   rastColor[CONT15_COLOR] = BLUE;
/* 2D phased display */
   rastColor[CONTM7_COLOR] = RED;
   rastColor[CONTM6_COLOR] = RED;
   rastColor[CONTM5_COLOR] = RED;
   rastColor[CONTM4_COLOR] = RED;
   rastColor[CONTM3_COLOR] = RED;
   rastColor[CONTM2_COLOR] = RED;
   rastColor[CONTM1_COLOR] = RED;
   rastColor[CONTP0_COLOR] = BLACK;
   rastColor[CONTP1_COLOR] = BLACK;
   rastColor[CONTP2_COLOR] = BLACK;
   rastColor[CONTP3_COLOR] = BLACK;
   rastColor[CONTP4_COLOR] = BLACK;
   rastColor[CONTP5_COLOR] = BLACK;
   rastColor[CONTP6_COLOR] = BLACK;
   rastColor[CONTP7_COLOR] = BLACK;
   rastColor[PEAK_COLOR] = RED;
   rastColor[NUM_COLOR] = CYAN;
   rastColor[AV_BOX_COLOR] = RED;
   rastColor[PH_BOX_COLOR] = RED;
   rastColor[LABEL_COLOR] = GREEN;
   rastColor[PINK_COLOR] = RED;
   rastColor[GRAY_COLOR] = GREEN;

   pens[BLACK]   = 1;
   pens[RED]     = 2;
   pens[YELLOW]  = 3;
   pens[GREEN]   = 4;
   pens[CYAN]    = 5;
   pens[BLUE]    = 6;
   pens[MAGENTA] = 7;
   pens[ORANGE]   = 8;
   pens[WHITE]   = 9;

   pens[FID_COLOR] = CYAN;
   pens[IMAG_COLOR] = GREEN;
   pens[SPEC_COLOR] = CYAN;
   pens[INT_COLOR] = BLUE;
   pens[CURSOR_COLOR] = RED;
   pens[PARAM_COLOR] = BLACK;
   pens[SCALE_COLOR] = RED;
   pens[THRESH_COLOR] = YELLOW;
   pens[SPEC2_COLOR] = GREEN;
   pens[SPEC3_COLOR] = YELLOW;
   pens[BG_IMAGE_COLOR] = BLACK;
   pens[FG_IMAGE_COLOR] = WHITE;

/* 2D absolute value */
   pens[CONT0_COLOR] = BLACK;
   pens[CONT1_COLOR] = BLUE;
   pens[CONT2_COLOR] = CYAN;
   pens[CONT3_COLOR] = GREEN;
   pens[CONT4_COLOR] = MAGENTA;
   pens[CONT5_COLOR] = RED;
   pens[CONT6_COLOR] = YELLOW;
   pens[CONT7_COLOR] = BLACK;
   pens[CONT8_COLOR] = BLUE;
   pens[CONT9_COLOR] = CYAN;
   pens[CONT10_COLOR] = GREEN;
   pens[CONT11_COLOR] = MAGENTA;
   pens[CONT12_COLOR] = RED;
   pens[CONT13_COLOR] = YELLOW;
   pens[CONT14_COLOR] = BLACK;
   pens[CONT15_COLOR] = BLUE;
/* 2D phased display */
   pens[CONTM7_COLOR] = RED;
   pens[CONTM6_COLOR] = RED;
   pens[CONTM5_COLOR] = RED;
   pens[CONTM4_COLOR] = RED;
   pens[CONTM3_COLOR] = RED;
   pens[CONTM2_COLOR] = RED;
   pens[CONTM1_COLOR] = RED;
   pens[CONTP0_COLOR] = BLACK;
   pens[CONTP1_COLOR] = BLACK;
   pens[CONTP2_COLOR] = BLACK;
   pens[CONTP3_COLOR] = BLACK;
   pens[CONTP4_COLOR] = BLACK;
   pens[CONTP5_COLOR] = BLACK;
   pens[CONTP6_COLOR] = BLACK;
   pens[CONTP7_COLOR] = BLACK;
   pens[PEAK_COLOR] = RED;
   pens[NUM_COLOR] = CYAN;
   pens[AV_BOX_COLOR] = RED;
   pens[PH_BOX_COLOR] = RED;
   pens[LABEL_COLOR] = GREEN;
   pens[PINK_COLOR] = RED;
   pens[GRAY_COLOR] = GREEN;
}

/*
 *  Given a string colorname,  determine the appropriate color
 *  index.  If the colorname is found,  the argument index will
 *  be set.  Successful matching of the colorname will cause this
 *  procedure to return a 1.  A zero (0) will be returned if no
 *  match is found.
 */
/*****************************/
int colorIndexNoPen(char *colorname, int *index)
/*****************************/
{
   int found = 1;

   if (strcmp(colorname,"red")==0)
      *index = RED;
   else if (strcmp(colorname,"green")==0)
      *index = GREEN;
   else if (strcmp(colorname,"blue")==0)
      *index = BLUE;
   else if (strcmp(colorname,"cyan")==0)
      *index = CYAN;
   else if (strcmp(colorname,"magenta")==0)
      *index = MAGENTA;
   else if (strcmp(colorname,"yellow")==0)
      *index = YELLOW;
   else if (strcmp(colorname,"orange")==0)
      *index = ORANGE;
   else if (strcmp(colorname,"white")==0)
      *index = WHITE;
   else if (strcmp(colorname,"black")==0)
      *index = BLACK;
   else
      found = 0;
   return(found);
}

/*****************************/
int colorindex(char *colorname, int *index)
/*****************************/
{
   int found = getOptID(colorname,index);
   if(found) return 1;
 
   found = 1;
   if (strcmp(colorname,"red")==0)
      *index = RED;
   else if (strcmp(colorname,"green")==0)
      *index = GREEN;
   else if (strcmp(colorname,"blue")==0)
      *index = BLUE;
   else if (strcmp(colorname,"cyan")==0)
      *index = CYAN;
   else if (strcmp(colorname,"magenta")==0)
      *index = MAGENTA;
   else if (strcmp(colorname,"yellow")==0)
      *index = YELLOW;
   else if (strcmp(colorname,"orange")==0)
      *index = ORANGE;
   else if (strcmp(colorname,"white")==0)
      *index = WHITE;
   else if (strcmp(colorname,"pink")==0)
      *index = PINK_COLOR;
   else if (strcmp(colorname,"gray")==0)
      *index = GRAY_COLOR;
   else if (strcmp(colorname,"black")==0)
      *index = BLACK;
   else if (strcmp(colorname,"cursor")==0)
      *index = CURSOR_COLOR;
   else if (strcmp(colorname,"integral")==0)
      *index = INT_COLOR;
   else if (strcmp(colorname,"threshold")==0)
      *index = THRESH_COLOR;
   else if (strcmp(colorname,"scale")==0)
      *index = SCALE_COLOR;
   else if (strcmp(colorname,"fid")==0)
      *index = FID_COLOR;
   else if (strcmp(colorname,"spectrum")==0)
      *index = SPEC_COLOR;
   else if (strcmp(colorname,"imaginary")==0)
      *index = IMAG_COLOR;
   else if (strcmp(colorname,"absval")==0)
      *index = ABSVAL_FID_COLOR;
   else if (strcmp(colorname,"envelope")==0)
      *index = FID_ENVEL_COLOR;
   else if (strcmp(colorname,"parameter")==0)
      *index = PARAM_COLOR;
   else if (strcasecmp(colorname,"expSpec")==0)
      *index = EXP_SPEC;
   else if (strcasecmp(colorname,"dsSpec")==0)
      *index = DS_SPEC;
   else if (strcasecmp(colorname,"craftSpec")==0)
      *index = CRAFT_SPEC;
   else if (strcasecmp(colorname,"residSpec")==0)
      *index = RESIDUAL_SPEC;
   else if (strcasecmp(colorname,"sumSpec")==0)
      *index = SUM_SPEC;
   else if (strcasecmp(colorname,"modelSpec")==0)
      *index = MODEL_SPEC;
   else if (strcasecmp(colorname,"refSpec")==0)
      *index = REF_SPEC;
   else if (strcasecmp(colorname,"notUsed")==0)
      *index = NOT_USED;
   else if (strcasecmp(colorname,"roli")==0)
      *index = ROLI;
   else if (strcasecmp(colorname,"craftRois")==0)
      *index = CRAFT_ROIS;
   else if (strcasecmp(colorname,"alignRois")==0)
      *index = ALIGNMENT_ROIS;
   else if (strcasecmp(colorname,"align2Rois")==0)
      *index = ALIGNMENT2_ROIS;
   else if (strcasecmp(colorname,"align3Rois")==0)
      *index = ALIGNMENT3_ROIS;
   else if (strcasecmp(colorname,"segRois")==0)
      *index = SEGMENT_ROIS;
   else if ((strlen(colorname) > (size_t)3) &&
            (*colorname     == 'p') &&
            (*(colorname+1) == 'e') &&
            (*(colorname+2) == 'n') &&
            isReal(colorname+3))
   {
      *index =  (int) stringReal(colorname+3);
      if ( (*index >= 1) && (*index <= 8) )
      {
         *index *= -1;
      }
      else
         found = 0;
   }
   else
      found = 0;
   return(found);
}

#ifndef INTERACT
int savecolors(int argc, char *argv[], int retc, char *retv[])
{   
    char  pltname[56];
    char  srcPath[256];
    char  cmd[512];
    FILE  *fd;

    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;
    if (P_getstring(GLOBAL, "plotter" ,pltname, 1,52) < 0)
        RETURN;
    if (strcmp(pltname, "DEFAULT") == 0)
        RETURN;
    sprintf(srcPath,"%s/templates/color/DEFAULT",userdir);
    fd = fopen(srcPath, "r");
    if (fd == NULL)
        RETURN;
    fclose(fd);
    sprintf(cmd,"cp %s %s/templates/color/%s", srcPath, userdir, pltname);
    system(cmd);
    RETURN;
}
#endif
