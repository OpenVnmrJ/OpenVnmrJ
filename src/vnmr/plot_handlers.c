/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************/
/*						*/
/* plot_handlers.c-	plotter setup/close	*/ 
/*						*/
/* Provides the necessary routines for graphics */
/* display on hard copy devices such as		*/
/* hp plotters, laserjets, and postscript 	*/
/* plotters and printers.  Printer languages	*/
/* are HPGL, LaserJet, and PostScript.		*/
/* LaserJets are raster copies from a SunView   */
/* construct called a pixrect.			*/
/* The type of plotter is      			*/
/* defined by the global variable "plotter"     */
/* Graphics display and plotting use identical  */
/* function calls. Plotting is started by a     */
/* call to "setplotter()". Subsequent graphics  */
/* calls are then routed to the plotter.        */
/* Screen graphics is started by a call to      */
/* "setdisplay()". Subsequent graphics calls    */
/* are then routed to the screen. A global      */
/* variable plot is set to 1 during plotting    */
/* and to 0 during screen display.		*/
/*						*/
/* Important note concerning VMS programming:   */
/*   Those plotters which are Raster oriented   */
/*   for example, the LaserJet 150, are not	*/
/*   supported under VMS at this time, as the	*/
/*   rasterization calls upon Sun-written	*/
/*   routines not available on VMS.  These	*/
/*   have raster = 1 or 2.  Raster = 3 or 4	*/
/*   represent PostScript devices, which ARE    */
/*   available under VMS.			*/
/*						*/
/* These routines are excerpted from graphics.c */
/*						*/
/************************************************/

#ifdef  DEBUG
extern int      Tflag;
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
#else 
#define TPRINT0(str) 
#define TPRINT1(str, arg1) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#define TPRINT4(str, arg1, arg2, arg3, arg4) 
#endif 
#define EMMM	2.8346
extern int interuption;
extern int active;                      /* Required for Tektronix */

#include "vnmrsys.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "graphics.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"
#ifdef VNMR
#include "group.h"
#endif 
#include "vfilesys.h"

#ifdef UNIX
#include <sys/file.h>
#endif 

#ifdef VNMRJ
extern char *get_p11_id();
extern int   VnmrJViewId;
#endif

#define MAXCOLUMNS	    6
#define PS_OPER_MAX     160
FILE	*plotfile=0;
/* globals for device naming */
char        PlotterHost[128];  /* global for all */
char        PrinterHost[128];  /* global for all */
char        PlotterName[128];  /* global for all */
char        PrinterName[128];  /* global for all */
char        PlotterShared[28];  /* global for all */
char        PrinterShared[28];  /* global for all */
char	    LprinterPort[28];
char	    LplotterPort[28];
char        PlotterType[128]; /* global for all */
char        PrinterType[128]; /* global for all */
char        PlotterFormat[32];  /* global for all */
char        *OsPrinterName = NULL;  // OS printer name
char        *OsPlotterName = NULL;  // OS printer name
char        *defaultPlotterName = NULL;  // OS default printer name
static char TmpPlotterName[128];
static char TmpPlotterType[128];
static char TmpOsPlotterName[512];
static char TmpAttr[MAXSTR];
static char TmpLraster[6];
static char PaperName[128];
static char Lbaud[10];
static char TmpTitle[128];
static char headerPath[MAXPATHL];
static char logoPath[MAXPATH];
static char *tableHeaders[MAXCOLUMNS];
static char *noneDevice = "none";
/* globals for printing/plotting devices */
static double Lppmm = 11.811;
static double Lraster_page;		/* not used anymore */
static double Lxoffset = 0.0;
static double Lxoffset1 = 0.0;
static double Lyoffset = 0.0;
static double Lyoffset1 = 0.0;
static double Lwcmaxmin = 50.0;
static double Lwcmaxmax = 250.0;
static double Lwc2maxmin = 37.0;
static double Lwc2maxmax = 186.0;
static double Lright_edge = 14.5;
static double Lleft_edge = 14.5;
static double Ltop_edge = 14.5;
static double Lbottom_edge = 14.5;
static double LpaperWidth = 215.9; // millimeter
static double LpaperHeight = 279.4;
static double scaleX = 0.24;
static double scaleY = 0.24;
static double psFontSize = 1.1;
static double psRed = 0.0;
static double psGrn = 0.0;
static double psBlu = 0.0;
static double infoFontSize = 0.60;
static int   Lraster = 4;
static int   Lraster_charsize = 0;
static int   Lraster_resolution = 0;
static int   Lxcharp1 = 15;
static int   Lycharp1 = 30;
static int   orig_Lraster = 4;
static int cp_tog = 0;			/* PostScript */
static int orgx = 0;                    /* PostScript */
static int orgy = 0;
static int lineWidth = 1;
static int transX = 0;
static int transY = 0;
static int plotFileSeq = 0;
static int psh = 0;
static int psw = 0;
static int pltXpnts = 2957;
static int pltYpnts = 2208;
static int maxpen = 1;
static int pslw = 1;
static int orig_pslw = 1;
static int tmpMaxpen = 1;
static int plotterAssigned = 0;
static int printerAssigned = 0;

static int inMiniPlot = 0;
static int pageColumns = 3;
static int plotDebug = 0;
static int columnIndex = -1;
static int columnWdith = 0;
static int columnWdith2 = 0;
static int infoPlotX = 0;
static int infoPlotY = 0;
static int infoPlotGapX = 0;
static int tableColumns = 0;
static int tableHeaderNum = 0;
// static int pageMax = 12;
static int pageNo = 1;
static int layoutMode = 4;
static int pageHeaderY = 40;   // the top position of header
static int pageHeaderB = 140;  // the bottom position of header
static int pageHeaderH = 140;  // the height of header
static int headerPlotted = 0;
static int headerAssigned = 0;
static int logoPlotted = 0;
static int logoAssigned = 0;
static int logoPos = 0;
static double logoXpos = 0.0;
static double logoYpos = 0.0;
static double logoWidth = 0.0;
static double logoHeight = 0.0;
static int logoSize = 0;
static int columnLocx[MAXCOLUMNS];
static double logoY = 200.0;
static double ps_ppmm = 11.811;
static double ps_charX = 15.0;
static double ps_charY = 30.0;
static double orig_Lwc2maxmax = 186.0;

int 	raster_resolution,raster_charsize,save_raster_charsize;

/* includes from graphics.c which are used both places */

#define MAXSHADES	65
#define SHADESIZE	8
#define PSMARGIN	42
// define margin in millimeter
#define RASTERMARGIN    14.81
/* includes from graphics.c */
extern int 	in_plot_mode;
extern int	raster;
extern int	ygraphoff;
extern int	xgrx,xgry,g_color,g_mode;
extern short	xorflag;
extern int      hires_ps;
#ifndef INTERACT
extern double	graysl;
#else 
static double	graysl = 1.0;
#endif 

double	gray_offset;
char	pltype[64];
char	plotpath[MAXPATHL];
int	curpencolor;
int	revchar=0;
int	xcharp1,ycharp1;
int     hires_ps = 0;

#ifdef VNMR
int loadPlotInfo(char *pltype);
int Efgets(char *ss, int size, FILE *filestream, char *message);
extern int is_vplot_session(int queryOnly);
extern void dump_raster_image();
extern pid_t HostPid;
extern void set_raster_gray_matrix(int size);
extern int create_pixmap();
extern void disp_plot(char *t);
extern void imagefile_init();
extern int plot_header(char *filename, int x, int y, int w, int h);
extern int plot_logo(char *name, double x, double y, double w, double h,
                     int sizescale);
extern int imagefile_flush();
extern int ps_imagefile_flush();
extern int  pclose_call(FILE *pfile);
extern int get_plot_planes();
extern int get_raster_color(int index);
extern void get_ps_color(int index, double *red, double *grn, double *blu);
extern int get_pen_color(int index);
#endif
extern void set_plot_planes(int n);
extern void write_ps_image_header(FILE *pfile);
extern double sc, sc2, wc, wc2;
extern double vp, vs, vs2d;
extern double is, ho, io, vo, vsproj, lvl, tlt;
extern double dss_sc, dss_sc2, dss_wc, dss_wc2;

void ps_b_init();
void ps_flush();

static char *ps_font_list[] = { "AvantGarde-Book", "AvantGarde-BookOblique",
   "AvantGarde-Demi", "AvantGarde-DemiOblique",
   "Bookman-Demi", "Bookman-DemiItalic", "Bookman-Light", "Bookman-LightItalic",
   "Century-Bold", "Century-BoldItalic", "Century-Light", "Century-LightItalic",
   "Courier", "Courier-Bold", "Courier-BoldOblique", "Courier-Oblique",
   "Helvetica", "Helvetica-Bold", "Helvetica-BoldOblique",
   "Helvetica-Narrow", "Helvetica-Narrow-Bold", "Helvetica-Narrow-BoldOblique",
   "Helvetica-Narrow-Oblique",
   "Helvetica-Oblique",
   "Lucida", "Lucida-Bold", "Lucida-BoldItalic", "Lucida-Italic",
   "LucidaSans", "LucidaSans-Bold", "LucidaSans-BoldItalic", "LucidaSans-Italic",
   "NewCenturySchlbk-Bold", "NewCenturySchlbk-BoldItalic", "NewCenturySchlbk-Italic",
   "NewCenturySchlbk-Roman",
   "Palatino-Bold", "Palatino-BoldItalic", "Palatino-Italic", "Palatino-Roman",
   "Symbol",
   "Times-Bold", "Times-BoldItalic", "Times-Italic", "Times-Roman",
   "ZapfChancery-MediumItalic" };

static char psFontName[36];


#ifdef INTERACT
static int
is_vplot_session(int n)
{
	return (0);
}

int is_vplot_page()
{
	return (0);
}
#endif 

#ifdef VNMR

static int getPrinterAttrs(char *name, int isSystem, int isPlotter, int peekOnly) 
{
   char   filepath[MAXPATHL];
   FILE  *fin;
   char  *p;
   char   data[128];
   char   type[128];
   char   s[1024];
   int    k;

   if (name == NULL)
       return 0;
   if (isSystem)
       sprintf(filepath,"%s/devicenames",systemdir);
   else
       sprintf(filepath,"%s/.devicenames",userdir);
   fin = fopen(filepath,"r");
   if (fin == NULL)
       return -1;
   k = 0;
   while ((p = fgets(s,1023, fin)) != NULL) {
       if (sscanf(p,"%s%s",type, data) == 2) {
           if (!strcmp(type,"Name")) {
               if (!strcmp(name, data)) {
                    k = 1;
                    break;
               }
           }
       }
   } 
   if (k == 0) {
       fclose(fin);
       return k;
   }

   if (peekOnly == 0) {
       if (isPlotter) {
           PlotterType[0] = '\0';
           PlotterHost[0] = '\0';
           PlotterShared[0] = '\0';
           PlotterFormat[0] = '\0';
           strcpy(LplotterPort, "0");
       }
       else {
           PrinterType[0] = '\0';
           PrinterShared[0] = '\0';
       }
   }
   while ((p = fgets(s,1023, fin)) != NULL) {
       if (sscanf(p,"%s%s",type, data) != 2)
          continue;
       if (!strcmp(type,"Name")) {
          if (strcmp(name, data) != 0)
              break;
          continue;
       }
       if (!strcmp(type,"Printer")) {
    	  sscanf(p,"%s%[^\n\r]s",type, data); // preserves spaces in printer name
          if (peekOnly == 0) {
        	  // trim leading and trailing spaces
        	  char *nstart=data;
        	  while(*nstart==' ')
        		  nstart++;
        	  char *nend=nstart+strlen(nstart)-1;
        	  while(*nend==' ')
        		  *nend--=0;
              if (isPlotter) {
                  if (OsPlotterName != NULL)
                      free(OsPlotterName);
                  OsPlotterName = malloc(strlen(nstart) + 1);
                  if (OsPlotterName != NULL)
                      strcpy(OsPlotterName, nstart);
              }
              else {
                  if (OsPrinterName != NULL)
                      free(OsPrinterName);
                  OsPrinterName = malloc(strlen(nstart) + 1);
                  if (OsPrinterName != NULL)
                      strcpy(OsPrinterName, nstart);
              }
              //Winfoprintf("printer=[%s]",OsPrinterName);
          }
          else
          {
             strcpy(TmpOsPlotterName, data);
          }
          continue;
       }
       if (!strcmp(type,"Use")) {
          if (isPlotter) {
               if (strcmp(data,"Plotter") && strcmp(data,"Both")) {
                //   Werrprintf("Device %s can not be set to be a plotter"
                //                  ,name);
                //   fclose(fin);
                //   return 0;
               }
          }
          else {
               if (strcmp(data,"Printer") && strcmp(data,"Both")) {
                     Werrprintf("Device %s can not be set to be a printer"
                                  ,name);
               }
          }
          continue;
       }
       if (!strcmp(type,"Type")) {
          if (strlen(data) < 124) {
              if (peekOnly == 0) {
                 if (isPlotter)
                     strcpy(PlotterType, data);
                 else
                     strcpy(PrinterType, data);
              }
              else
                 strcpy(TmpPlotterType, data);
          }
          continue;
       }
       if (!strcmp(type,"Host")) {
          if (strlen(data) < 124) {
              if (peekOnly == 0) {
                 if (isPlotter)
                     strcpy(PlotterHost, data);
                 else
                     strcpy(PrinterHost, data);
              }
          }
          continue;
       }
       if (!strcmp(type,"Port")) {
          if (strlen(data) < 24) {
              if (peekOnly == 0) {
                 if (isPlotter)
                     strcpy(LplotterPort, data);
                 else
                     strcpy(LprinterPort, data);
              }
          }
          continue;
       }
       if (!strcmp(type,"Baud")) {
          if (peekOnly == 0)
              strcpy(Lbaud, data);
          continue;
       }
       if (!strcmp(type,"Shared")) {
          if (peekOnly == 0) {
              if (isPlotter) {
                  if (strlen(data) < 12)
                      strcpy(PlotterShared, data);
                  if (strlen(PlotterType) > 0)
                      break;
              }
              else {
                  if (strlen(data) < 12)
                      strcpy(PrinterShared, data);
                  if (strlen(PrinterType) > 0)
                      break;
              }
          }
       }
   }

   fclose(fin);
   return 1;
}

int
ps_info_font_height() {
    int h;

    h = (int)(ps_charY * infoFontSize * ps_ppmm/5.91);
    return (h);
}

int
ps_info_font_width() {
    int w;

    w = (int)(ps_charX * infoFontSize * ps_ppmm/5.91);
    return (w);
}

static void adjustWCMaxValues()
{
    double fw, fh, fv, ih;
    int k;
    double d;

    orig_Lwc2maxmax = Lwc2maxmax;
    if (Lraster < 1 || Lraster > 4)
        return;
    if (LpaperWidth < 60.0 || LpaperHeight < 60.0) // old type or too small
        return;
    if (Lwcmaxmax < 20.0 || Lwc2maxmax < 20.0)
        return;
    if (Lleft_edge < 0.0)
        Lleft_edge = 0.0;
    if (Lright_edge < 0.0)
        Lright_edge = 0.0;
    if (Ltop_edge < 0.0)
        Ltop_edge = 0.0;
    if (Lbottom_edge < 0.0)
        Lbottom_edge = 0.0;
    ps_ppmm = (double) Lppmm;
    ps_charX = (double) Lxcharp1;
    ps_charY = (double) Lycharp1;
    fw = LpaperWidth;
    fh = LpaperHeight;
    if (Lraster == 2 || Lraster == 4) // landscape     
    {
        if (LpaperHeight > LpaperWidth)
        {
             fw = LpaperHeight;
             fh = LpaperWidth;
        }
    }
    if (Lwcmaxmax > fw || Lwc2maxmax > fh) // information was not correct
        return;

    fv = fw - Lright_edge - Lleft_edge - Lwcmaxmax;
    if (fv < 1.0)
        Lwcmaxmax = fw - Lright_edge - Lleft_edge;
    ih = (double) ps_info_font_height() * 1.2;
    fv = Ltop_edge * Lppmm;
    if (fv < ih)
        Ltop_edge = ih / Lppmm;
    if (Lraster == 2 || Lraster == 4)
        fv = (double) ps_info_font_height() * 4.0;
    else
        fv = (double) ps_info_font_height() * 10.0;
    ih = fv / Lppmm + Ltop_edge + Lbottom_edge + BASEOFFSET;
    fv = fh - Lwc2maxmax - ih;
    if (fv < 0.0)
        Lwc2maxmax = fh - ih;
    k = (int) (Lwc2maxmax + 0.5);
    Lwc2maxmax = (double) k;
    k = (int) (Lwcmaxmax + 0.5);
    Lwcmaxmax = (double) k;
    pageHeaderH = ps_info_font_height() * 4;
    logoY = fh - Ltop_edge - Lbottom_edge;
    pageHeaderY = (int) (logoY * Lppmm);
    if (Lraster == 1 || Lraster == 3)  // portrait
        pageHeaderY = pageHeaderY - ps_info_font_height() * 6;
    pageHeaderB = pageHeaderY -pageHeaderH;
    pageHeaderH = ps_info_font_height() * 3.6;

    if (Lraster > 2)  // not PCL
        return;
    if (Lppmm > 0.0)
        d = Lppmm * 25.4 + 0.5;
    else
        d = 150.0;
    if (Lraster_resolution == 0)
        Lraster_resolution = (int) d;
    if (Lraster_charsize < 5 || Lraster_charsize > 100)
    {
         d = d / 150.0;
         if (d < 0.6)
             d = 0.6;
         Lraster_charsize = (int) (d * 15.0);
    }
    if (Lxcharp1 < 5 || Lxcharp1 > 100)
    {
         Lxcharp1 = Lraster_charsize;
         Lycharp1 = Lraster_charsize * 2;
    }
    ps_ppmm = (double) Lppmm;
    ps_charX = (double) Lxcharp1;
    ps_charY = (double) Lycharp1;
}


#define WCLOG  ".vjwclog"
#define TMPWCLOG  ".tmpvjwclog"

static void adjustWcWc2()
{
#ifdef VNMRJ
    double oldWcmax, oldWc2max;
    double newWcmax, newWc2max;
    double oldWc, oldWc2;
    double newWc, newWc2;
    double fwcmax, fwc2max;
    double d1, d2;
    double fwc, fwc2;
    int k, lines;
    char   *p, s[256];
    FILE  *fin, *fout;

    if (Lraster < 1 || Lraster > 4)
        return;
    if (P_getreal(GLOBAL,"wcmax"  ,&oldWcmax,  1) != 0)
       return;
    if (P_getreal(CURRENT,"wc"  ,&oldWc,  1) != 0)
       return;
    if (P_getreal(GLOBAL,"wc2max"  ,&oldWc2max,  1) != 0)
       return;
    if (P_getreal(CURRENT,"wc2"  ,&oldWc2,  1) != 0)
       return;
    newWcmax = (double) Lwcmaxmax;
    newWc2max = (double)Lwc2maxmax;
    if ((oldWcmax == newWcmax) && (oldWc2max == newWc2max))
       return;

    sprintf(TmpOsPlotterName,"%s/%s%d",userdir, WCLOG, VnmrJViewId);
    fin = fopen(TmpOsPlotterName,"r");
    sprintf(TmpOsPlotterName,"%s/%s%d",userdir, TMPWCLOG, VnmrJViewId);
    fout = fopen(TmpOsPlotterName,"w");
    newWc = 0;
    newWc2 = 0;
    lines = 0;
    if (fin != NULL) {
       while ((p = fgets(s,250, fin)) != NULL) {
          if (sscanf(p,"%lf%lf%lf%lf",&fwcmax, &fwc2max, &fwc, &fwc2) == 4) {
             d1 = (double) fwcmax;
             d2 = (double) fwc2max;
             if ((d1 == newWcmax) && (d2 == newWc2max))
             { 
                 newWc = (double)fwc;
                 newWc2 = (double)fwc2;
             }
             else if (fout != NULL) {
                 lines++;
                 if ((d1 != oldWcmax) || (d2 != oldWc2max)) {
                     if (lines < 30)
                        fprintf(fout, "%s", s);
                 }
             }
          }
       }
       fclose(fin);
   }
   if ((newWc < 1.0) || (newWc2 < 1.0)) {
       newWc = newWcmax * oldWc / oldWcmax; 
       newWc2 = newWc2max * oldWc2 / oldWc2max; 
       k = (int)(newWc+0.5);
       newWc = (double)k;
       k = (int)(newWc2+0.5);
       newWc2 = (double)k;
   }
   P_setreal(CURRENT,"wc", newWc, 1);
   P_setreal(CURRENT,"wc2", newWc2, 1);
   if (fout != NULL) {
       fprintf(fout, "%g %g %g %g\n", oldWcmax, oldWc2max, oldWc, oldWc2);
       fprintf(fout, "%g %g %g %g\n", newWcmax, newWc2max, newWc, newWc2);
       fclose(fout);
       sprintf(TmpOsPlotterName,"mv %s/%s%d %s/%s%d",userdir,TMPWCLOG,VnmrJViewId, userdir, WCLOG, VnmrJViewId);
       system(TmpOsPlotterName);
   }
#endif
}

// set PS_AR letter attributes
static void setDefaultTypes()
{
    Lppmm = 11.811;
    Lraster = 4;
    orig_Lraster = Lraster;
    strcpy(PlotterFormat,"POSTSCRIPT");
    Lraster_charsize = 0;
    Lraster_resolution = 0;
    Lxoffset = 0;
    Lyoffset = 0;
    Lxoffset1 = 0;
    Lyoffset1 = 0;
    Lxcharp1 = 15;
    Lycharp1 = 30;
    Lwcmaxmin = 50.0;
    Lwcmaxmax = 250.0;
    Lwc2maxmin = 37.2;
    Lwc2maxmax = 186.0;
    LpaperWidth = 215.9;
    LpaperHeight = 279.4;
    Lright_edge = 14.5;
    Lleft_edge = 14.5;
    Ltop_edge = 14.5;
    Lbottom_edge = 14.5;
    maxpen = 1;
    // pslw = 1;
    // orig_pslw = 1;
}

#ifdef __INTERIX
static void getDefaultPlotterName()
{
     char   path[MAXPATHL];
     FILE  *fin;
     int    i, k;

     sprintf(path,"%s/persistence/.defaultprinter",userdir);
     fin = fopen(path,"r");
     if (fin == NULL)
         return;
     k = 0;
     while (fgets(path, 128, fin) != NULL) {
         k = (int)strlen(path);
         if (k > 0) {
            for (i = 0; i < k; i++) {
               if (path[i] == '\n' || path[i] == '\r') {  // Line feed or Return
                  path[i] = '\0';
                  break;
               }
            }
            k = (int)strlen(path);
            if (k > 0)
               break;
         }
     }
     fclose(fin);
     if (k <= 0)
         return;
     if (defaultPlotterName != NULL) {
         if (strcmp(defaultPlotterName, path) == 0)
             return;
         free(defaultPlotterName);
     }
     defaultPlotterName = malloc(strlen(path) + 2);
     if (defaultPlotterName == NULL)
         return;
     strcpy(defaultPlotterName, path);
     // setenv("nmrplotter", defaultPlotterName, 1);
     // setenv("PRINTER", defaultPlotterName, 1);  // for Windows
}
#endif


/*-------------------------------------------------------------------------
|   setDeviceName/1
|
|   This routine reads the devicenames file, determines if the devicename
|   exists and calles loadPlotInfo if the device is a plot device.
|   If type = 1, then we have a plotter device, if type = 0, we have 
|   a printer device.
+-------------------------------------------------------------------------*/
int setDeviceName(char *dname, int isPlotter)
{
   char   *name;
   int    n, k, retVal; 
   int    badDevice;
   int    notNone;
   double tmp;

   if (dname == NULL)
       return 0;
   n = (int)strlen(dname);
   k = 0;
   while (k < n) {  // trim dname
       if (*(dname+k) != ' ')
          break;
       k++;
   }
   name = dname + k;
   k = n - 1;
   while (k >= 0) {
       if (*(dname+k) == ' ')
          *(dname+k) = '\0';
       else
          break;
       k--;
   }
   if (strlen(name) > 126) {
      Werrprintf("Illeagle plotter name '%s', too long. ",name);
      return 0;
   }

   if (strcmp(name, noneDevice) == 0)
       notNone = 0;
   else
       notNone = 1;
 
   if (isPlotter) {
       if (OsPlotterName != NULL) {
           free(OsPlotterName);
           OsPlotterName = NULL;
       }
       strcpy(PlotterName,name);
   }
   else {
       if (OsPrinterName != NULL) {
           free(OsPrinterName);
           OsPrinterName = NULL;
       }
   }

   retVal = 0;
   badDevice = 0;
   // retVal = getPrinterAttrs(name, 0, isPlotter, 0);
   if (retVal <= 0) {
       retVal = getPrinterAttrs(name, 1, isPlotter, 0);
   }

   if (retVal <= 0) { // plotter type not defined
      if (retVal < 0) {
          if (isPlotter)
              Werrprintf("Could not open devicenames file");
      }
      else if (strlen(name) > 0) {
          if (notNone) {
              badDevice = 1;
              Werrprintf("Could not find entry '%s' in devicenames",name);
          }
      }
      if (isPlotter) {
          if (badDevice && Lraster >= 3) {
              if (plotterAssigned) // stay with the previous plotter
                 return 0;
          }
          PlotterType[0] = '\0';
      }
      if (notNone)
          strcpy(name, "");
   }
   if (isPlotter) {
      maxpen = 1;
      pslw = 1;
      orig_pslw = 1;
      plotterAssigned = 1;
      retVal = loadPlotInfo(PlotterType);
      if (retVal <= 0) {
          if (strlen(PlotterType) > 0)
              Werrprintf("Could not find entry '%s' in devicetable for plotter '%s'", PlotterType, name);
          setDefaultTypes();
          strcpy(PlotterType, "PS_AR");
      }
      else {
          if (OsPlotterName == NULL) {
              if (strlen(name) > 0) {
                 OsPlotterName = malloc(strlen(name) + 2);
                 strcpy(OsPlotterName, name);
              }
          }
      }
      strcpy(PlotterName,name); /* make a copy for later use */
      P_setstring(GLOBAL,"plotter",PlotterName,0);
      adjustWCMaxValues();
      adjustWcWc2();
      wc2max = (double) Lwc2maxmax;
      wcmax = (double) Lwcmaxmax;
      P_setreal(GLOBAL,"wcmax", wcmax, 1);
      P_setreal(GLOBAL,"wc2max", wc2max, 1);
      if (Lraster == 0)
         maxpen = 8;
      P_setreal(GLOBAL,"maxpen", (double)maxpen, 1);
      set_plot_planes(maxpen);
      if (P_getreal(GLOBAL,"pslw" ,&tmp, 1) == 0) {
         pslw = (int) tmp;
         orig_pslw = pslw;
      }
#ifdef __INTERIX
      if (OsPlotterName == NULL && notNone) {
         getDefaultPlotterName();
         /***
         if (defaultPlotterName != NULL) {
             OsPlotterName = malloc(strlen(defaultPlotterName) + 2);
             strcpy(OsPlotterName, defaultPlotterName);
         }
         ***/
      }
#endif
      if (OsPlotterName != NULL) {
         setenv("nmrplotter", OsPlotterName, 1);
         setenv("PRINTER", OsPlotterName, 1);  // for Windows
      }
   }
   else {
      if (OsPrinterName == NULL) {
#ifdef __INTERIX
          if (notNone) {
              getDefaultPlotterName();
              if (defaultPlotterName != NULL) {
                  OsPrinterName = malloc(strlen(defaultPlotterName) + 2);
                  strcpy(OsPrinterName, defaultPlotterName);
              }
          }
#endif
          if (strlen(name) > 0 && OsPrinterName == NULL) {
              OsPrinterName = malloc(strlen(name) + 1);
              strcpy(OsPrinterName, name);
          }
      }
      printerAssigned = 1;
      strcpy(PrinterName,name);
      P_setstring(GLOBAL,"printer",PrinterName,0);
   }
   return 1;
}

static void change_pcl_raster()
{
    orig_Lraster = Lraster;
    if (Lraster < 1 || Lraster > 2)  // not PCL
        return;
    /****
    if (access("/usr/bin/gs", X_OK) != 0)
        return;
    ****/
    Lraster = Lraster + 2;  // change to PS
}

static void ps_to_pcl(char *filename)
{
     int n;

     if (raster < 3 || raster > 4)  // not PS
         return;
     if (orig_Lraster < 1 || orig_Lraster > 2)  // not PCL
         return;
     if ((filename != NULL) && (strlen(filename) > 0)) { // save to file
         if (strstr(filename, ".pcl") == NULL) // save in ps format should be fine
             return;
     }

     sprintf(TmpAttr, "%s.pcl", plotpath);
     if (maxpen > 1)
         sprintf(TmpOsPlotterName, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE  -sDEVICE=cljet5c -sOutputFile=%s -q %s", TmpAttr, plotpath );
     else
         sprintf(TmpOsPlotterName, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE  -sDEVICE=ljet4 -sOutputFile=%s -q %s", TmpAttr, plotpath );
     system(TmpOsPlotterName);
     n = 0;
     while (n < 5) {
         if (access(TmpAttr, F_OK) == 0)
             break;
         n++;
         usleep(2.0e+5);
     }
     if (access(TmpAttr, F_OK) != 0) {
         sprintf(TmpOsPlotterName, "convert %s pcl:%s", plotpath, TmpAttr);
         system(TmpOsPlotterName);
     }
     unlink(plotpath);
     strcpy(plotpath, TmpAttr);
}

int old_setDeviceName(char *name, int plottertype)
{  char   filepath[MAXPATHL];
   char  *p;
   char   plotname[128];
   char   s[1024];
   char   emessage[1024];
   char   usage[48];
   FILE  *namesinfo;

   strcpy(emessage,"Bad devicenames file");
   /* open the devicenames file */
#ifdef UNIX
   sprintf(filepath,"%s/devicenames",systemdir);
#else 
   sprintf(filepath,"%sdevicenames",systemdir);
#endif 
   if ( (namesinfo=fopen(filepath,"r")) )
   {
      TPRINT1("setDeviceName: opened file '%s'\n",filepath);
      /* find entry in table corresponding to name */
      p = fgets(s,1023,namesinfo);
      while (p)
      {  if (!strncmp(p,"Name",4))/* check if we are at Name */
	 {  sscanf(p,"%*s%s",plotname); /* get name from file */
            /* If name matches, load in name parameters */
	    if (!strcmp(plotname,name)) 
            {  
	       if (Efgets(s,1023,namesinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%s",usage);

	       if (Efgets(s,1023,namesinfo,emessage) == 0) return 0;
	       if (plottertype)
	          sscanf(s,"%*s%s",PlotterType);
               else
	          sscanf(s,"%*s%s",PrinterType);

	       if (Efgets(s,1023,namesinfo,emessage) == 0) return 0;
	       if (plottertype)
	          sscanf(s,"%*s%s",PlotterHost);
	       else
	          sscanf(s,"%*s%s",PrinterHost);

	       if (Efgets(s,1023,namesinfo,emessage) == 0) return 0;
	       if (plottertype)
	          sscanf(s,"%*s%s",LplotterPort);
	       else
	          sscanf(s,"%*s%s",LprinterPort);

	       if (Efgets(s,1023,namesinfo,emessage) == 0) return 0;
               sscanf(s,"%*s%s",Lbaud);

	       if (Efgets(s,1023,namesinfo,emessage) == 0) return 0;
	       if (plottertype)
	          sscanf(s,"%*s%s",PlotterShared);
	       else
	          sscanf(s,"%*s%s",PrinterShared);
               fclose(namesinfo);
               /* check if It is ok to set this device to printer or plotter*/
               if (plottertype)
               {  if (strcmp(usage,"Plotter") && strcmp(usage,"Both"))
		  {   Werrprintf("Device %s can not be set to be a plotter"
				  ,name);
                      return 0;
		  }
               }
	       else
               {  if (strcmp(usage,"Printer") && strcmp(usage,"Both"))
		  {   Werrprintf("Device %s can not be set to be a printer"
				  ,name);
                      return 0;
		  }
               }

               if (plottertype)
               {   if (!loadPlotInfo(PlotterType))
                      return 0;
                   else
	           {
                       strcpy(PlotterName,name); /* make a copy for later use */
                       P_setstring(GLOBAL,"plotter",PlotterName,0);
                       return 1;
                   }
               }
	       else
	       {  strcpy(PrinterName,name); 
		  P_setstring(GLOBAL,"printer",PrinterName,0);
		  return 1;
	       }
	    }
	 }
         p = fgets(s,1023,namesinfo);
      }
      if (strcmp(name, noneDevice) != 0) {
          Werrprintf("Could not find entry '%s' in devicenames",name);
      }
      fclose(namesinfo);
      return 0;
   }
   else
   {
      TPRINT1("setPlotterName: trouble opening file '%s'\n",filepath);
      Werrprintf("Could not open devicenames file");
      return 0;
   }
}

/*-------------------------------------------------------------------------
|   setPlotterName/1
|
|   This routine reads the devicenames file, determines if the devicename
|   exists and calles loadPlotInfo to setup plotter parameters.
+-------------------------------------------------------------------------*/
int setPlotterName(char *name)
{  
   int retVal;

   if (name == NULL)
       return 0;
   retVal = setDeviceName(name,1);
   if (retVal <= 0)
      return(retVal);
   if (P_getstring(GLOBAL, "sysplotter", TmpPlotterType, 1, 120) == 0) {
      if (getPrinterAttrs(TmpPlotterType,  1, 1, 1) < 1) { // not exist
           P_setstring(GLOBAL, "sysplotter", name, 0);
      }
   }

   if (P_getstring(GLOBAL, "PDFpreview", TmpPlotterType, 1, 2) == 0) {
       if (Lraster < 3)
           execString("PDFpreview='n'\n");
       else
           execString("PDFpreview='y'\n");
   }

   return(retVal);
}

/*-------------------------------------------------------------------------
|   setPrinterName/1
|
|   This routine reads the devicenames file, determines if the devicename
|   exists it does not need to call loadPlotInfo for plotting parameters.
+-------------------------------------------------------------------------*/

int setPrinterName(char *name)
{  
   int retVal = setDeviceName(name,0);
   if (retVal > 0) {
      if (OsPrinterName != NULL)
         setenv("nmrprinter", OsPrinterName, 1);
   }
   return(retVal);
}

static void setTypeValue(char *type, char *data)
{
    if (!strcmp(type,"ppmm")) {
        Lppmm = atof(data);
        return;
    }
    if (!strcmp(type,"raster")) {
        Lraster = atoi(data);
        change_pcl_raster();
        return;
    }
    if (!strcmp(type,"format")) {
        strcpy(PlotterFormat, data);
        return;
    }
    if (!strcmp(type,"raster_charsize")) {
        Lraster_charsize = atoi(data);
        return;
    }
    if (!strcmp(type,"raster_page")) {
        Lraster_page = atof(data);   // not used any more
        return;
    }
    if (!strcmp(type,"raster_resolution")) {
        Lraster_resolution = atoi(data);
        return;
    }
    if (!strcmp(type,"xoffset")) {
        Lxoffset = atof(data);
        return;
    }
    if (!strcmp(type,"yoffset")) {
        Lyoffset = atof(data);
        return;
    }
    if (!strcmp(type,"xoffset1")) {
        Lxoffset1 = atof(data);
        return;
    }
    if (!strcmp(type,"yoffset1")) {
        Lyoffset1 = atof(data);
        return;
    }
    if (!strcmp(type,"xcharp1")) {
        Lxcharp1 = atoi(data);
        return;
    }
    if (!strcmp(type,"ycharp1")) {
        Lycharp1 = atoi(data);
        return;
    }
    if (!strcmp(type,"wcmaxmin")) {
        Lwcmaxmin = atof(data);
        return;
    }
    if (!strcmp(type,"wcmaxmax")) {
        Lwcmaxmax = atof(data);
        return;
    }
    if (!strcmp(type,"wc2maxmin")) {
        Lwc2maxmin = atof(data);
        return;
    }
    if (!strcmp(type,"wc2maxmax")) {
        Lwc2maxmax = atof(data);
        orig_Lwc2maxmax = Lwc2maxmax;
        return;
    }
    if (!strcmp(type,"papersize")) {
        strcpy(PaperName, data);
        return;
    }
    if (!strcmp(type,"paperwidth")) {
        LpaperWidth = atof(data);
        return;
    }
    if (!strcmp(type,"paperheight")) {
        LpaperHeight = atof(data);
        return;
    }
    if (!strcmp(type,"right_edge")) {
        Lright_edge = atof(data);
        return;
    }
    if (!strcmp(type,"left_edge")) {
        Lleft_edge = atof(data);
        return;
    }
    if (!strcmp(type,"top_edge")) {
        Ltop_edge = atof(data);
        return;
    }
    if (!strcmp(type,"bottom_edge")) {
        Lbottom_edge = atof(data);
        return;
    }
    if (!strcasecmp(type,"color")) {
        if (!strcasecmp(data,"Color"))
           maxpen = 8;
        else
           maxpen = 1;
        return;
    }
    if (!strcasecmp(type,"linewidth")) {
     /********
        pslw = atoi(data);
        if (pslw < 0)
           pslw = 1;
        else if (pslw > 100)
           pslw = 100;
        orig_pslw = pslw;
     ********/
        return;
    }
}

static int getTypeAttrs(char *name, int isSystem, int peekOnly, char *attr)
{
   char   filepath[MAXPATHL];
   char  *p;
   char   data[128];
   char   type[128];
   char   s[1024];
   int    k;
   FILE   *fin;

   if (isSystem)
       sprintf(filepath,"%s/devicetable",systemdir);
   else
       sprintf(filepath,"%s/.devicetable",userdir);
   fin = fopen(filepath,"r");
   if (fin == NULL)
       return -1;
   k = 0;
   while ((p = fgets(s,1023, fin)) != NULL) {
       if (sscanf(p,"%s%s",type, data) == 2) {
           if (!strcmp(type,"PrinterType")) {
               if (!strcmp(name, data)) {
                    k = 1;
                    break;
               }
           }
       }
   }
   if (k == 0) {
       fclose(fin);
       return 0;
   }

   if (peekOnly == 0) {
       Lppmm = -1.0;
       Lwc2maxmax = -1.0;
       Lraster = -1;
       orig_Lraster = 0;
   }
   while ((p = fgets(s,1023, fin)) != NULL) {
       if (sscanf(p,"%s%s",type, data) != 2)
          continue;
       if ( (attr != NULL) && !strcmp(type,attr) ) 
       {
          strcpy(TmpAttr, data);
       }
       if (!strcmp(type,"PrinterType")) {
          if (strcmp(name, data) != 0)
              break;
          continue;
       } 
       if (!strcmp(type,"Printcap")) // not used any more
          continue;
       if (!strcmp(type,"ppmm")) {
          if (peekOnly == 0) {
              if (Lppmm >= 0.0 && Lwc2maxmax >= 0.0) // duplicated
                  break;
              Lppmm = atof(data);
          }
          continue;
       }
       if (!strcmp(type,"raster")) {
          if (peekOnly == 0) {
              if (Lraster >= 0 && Lwc2maxmax >= 0.0)
                  break;
              Lraster = atoi(data);
              change_pcl_raster();
          }
          else {
              if (strlen(data) < 4) {
                 k = atoi(data);
                 if (k == 1 || k == 2) {
                    k = k + 2;
                    sprintf(TmpLraster, "%d", k);
                 }
                 else
                     strcpy(TmpLraster, data);
              }
          }
          continue;
       }
       if (peekOnly == 0)
       {
          setTypeValue(type, data);
       }
       else if (!strcasecmp(type,"color"))
       {
           if (!strcasecmp(data,"Color"))
               tmpMaxpen = 8;
           else
               tmpMaxpen = 1;
       }
   }
   fclose(fin);
   return 1; 
}

/*-------------------------------------------------------------------------
|   loadPlotInfo/1
|
|   This routine reads the devicetable information and loads the
|   plotting variables for a plot type.  It returns a 1 if successful and 
|   a zero if not
+-------------------------------------------------------------------------*/
int loadPlotInfo(char *pltype)
{
   int    retVal; 

   Lright_edge = RASTERMARGIN;
   Lleft_edge = RASTERMARGIN;
   Ltop_edge = RASTERMARGIN;
   Lbottom_edge = RASTERMARGIN;
   Lxoffset = 0.0;
   Lxoffset1 = 0.0;
   Lyoffset = 0.0;
   Lyoffset1 = 0.0;
   LpaperHeight = 0.0;
   LpaperWidth = 0.0;
   PaperName[0] = '\0';
#ifdef UNIX
   retVal = 0;
   if (pltype == NULL || (strlen(pltype) < 1))
       return 0;
   // retVal = getTypeAttrs(pltype, 0, 0);
   if (retVal <= 0) {
       retVal = getTypeAttrs(pltype, 1, 0, NULL);
   }
#else 
   // retVal = getTypeAttrs(pltype, 0, 0);
   if (retVal <= 0) {
       retVal = getTypeAttrs(pltype, 1, 0, NULL);
   }
#endif 
   if (retVal < 0) {
      TPRINT0("loadPlotInfo: trouble opening file devicetable\n");
      Werrprintf("Could not open file devicetable.");
      return 0;
   }
   if (retVal < 1) {
      // Werrprintf("Could not find entry '%s' in devicetable",pltype);
      return 0;
   }
   return 1;
}

int getplottertype(int argc, char *argv[], int retc, char *retv[])
{ 
    int  retVal = 0;
    int  OsName = 0;

    if (argc <= 1) /* no arguments */
    {
       if (P_getstring(GLOBAL,"plotter",TmpPlotterName,1,128))
          strcpy(TmpPlotterName,noneDevice);
    }
    else
    {
       strcpy(TmpPlotterName,argv[1]);
       if ( (argc >= 3) && !strcmp(argv[2],"osname") )
       {
          OsName = 1;
          strcpy(TmpOsPlotterName,argv[1]);
       }
    }

    TmpPlotterType[0] = '\0';
    strcpy(TmpLraster, "-1");
    retVal = getPrinterAttrs(TmpPlotterName, 1, 1, 1);
    if (retVal <= 0) {  // plotter was not found in devicenames
       TmpOsPlotterName[0] = '\0';
       if (strcmp(TmpPlotterName, noneDevice) != 0) {
          if (strlen(TmpPlotterName) > 0)
             Werrprintf("getplottertype: device '%s' not defined in /vnmr/devicenames!", TmpPlotterName);
          else {
             retVal = 1;
             TmpPlotterType[0] = '\0';
          }
       }
       else {
          retVal = 1;
          TmpPlotterType[0] = '\0';
       }
    }
    if (OsName) {
       if (retc > 0)
          retv[0] = newString(TmpOsPlotterName);
       else
          Winfoprintf("OS name of '%s' is '%s'", argv[1], TmpOsPlotterName);
       RETURN;
    }
    if (argc == 3)
    {
       TmpAttr[0] = '\0';
       if (retVal > 0) {
          if (strlen(TmpPlotterType) < 1)
              strcpy(TmpPlotterType, "PS_AR");
          retVal = getTypeAttrs(TmpPlotterType, 1, 1, argv[2]);
          if (retVal == 0) {
             if (strcmp(TmpPlotterType, "PS_AR") != 0) {
                 strcpy(TmpPlotterType, "PS_AR");
                 retVal = getTypeAttrs(TmpPlotterType, 1, 1, argv[2]);
             }
          }
       }
       if (retc > 0)
          retv[0] = newString(TmpAttr);
       else
          Winfoprintf("%s: Plotter %s atrribute %s is '%s'", argv[0], argv[1], argv[2], TmpAttr);
       RETURN;
    }
    if (retVal > 0) {
       if (strlen(TmpPlotterType) < 1)
           strcpy(TmpPlotterType, "PS_AR");
       retVal = getTypeAttrs(TmpPlotterType, 1, 1, NULL);
       if (retVal == 0) {
          if (strcmp(TmpPlotterType, "PS_AR") != 0) {
              strcpy(TmpPlotterType, "PS_AR");
              retVal = getTypeAttrs(TmpPlotterType, 1, 1, NULL);
          }
       }
    }
    if (retc > 0) {
       retv[0] = newString(TmpLraster);
       if (retc > 1)
           retv[1] = newString(TmpPlotterType);
    }
   //  Winfoprintf("getplottertype: '%s'  type: %s  rast: %s", TmpPlotterName, TmpPlotterType, TmpLraster);
    RETURN;
}

int getplotterpens(int argc, char *argv[], int retc, char *retv[])
{
    tmpMaxpen = maxpen;
    strcpy(TmpLraster, "1");
    if (argc > 1)
    {
       tmpMaxpen = 1;
       TmpPlotterType[0] = '\0';
       if (getPrinterAttrs(argv[1], 1, 1, 1) > 0)
       {
           if (strlen(TmpPlotterType) < 1)
               strcpy(TmpPlotterType, "PS_AR");
           getTypeAttrs(TmpPlotterType, 1, 1, NULL);
       }
    }
    if (TmpLraster[0] == '0')
       tmpMaxpen = 8;

    if (retc > 0)
       retv[0] = intString(tmpMaxpen);
    else
       Winfoprintf("getplotterpens: %d pens", tmpMaxpen);
    RETURN;
}

char *getpapername()
{
   return PaperName;
}

double getpaperwidth()
{
   return (double) LpaperWidth;
}

double getpaperheight()
{
   return (double) LpaperHeight;
}

int getPlotWidth()
{
    return pltXpnts;
}

int getPlotHeight()
{
    return pltYpnts;
}

int getPSoriginX()
{
   return transX;
}

int getPSoriginY()
{
   return transY;
}

double getPSscaleX()
{
   return scaleX;
}

double getPSscaleY()
{
   return scaleY;
}

double getpaper_leftmargin()
{
   return (double) Lleft_edge;
}

double getpaper_rightmargin()
{
   return (double) Lright_edge;
}

double getpaper_topmargin()
{
   return (double) Ltop_edge;
}

double getpaper_bottommargin()
{
   return (double) Lbottom_edge;
}

int old_loadPlotInfo(char *pltype)
{  char   filepath[MAXPATHL];
   char  *p;
   char   plotype[128];
   char   printcap[128];
   char	  emessage[128];
   char   s[1024];
   FILE  *plotinfo;

   /* open the devicetable file */
   strcpy(emessage,"Bad devicetable file");
#ifdef UNIX
   sprintf(filepath,"%s/devicetable",systemdir);
#else 
   sprintf(filepath,"%sdevicetable",systemdir);
#endif 
   if ( (plotinfo=fopen(filepath,"r")) )
   {
      TPRINT1("loadPlotInfo: opened file '%s'\n",filepath);
      /* find entry in table corresponding to pltype */
      p = fgets(s,1023,plotinfo);
      while (p)
      {  if (!strncmp(p,"PrinterType",11))/* check if we are at type */
	 {  sscanf(p,"%*s%s",plotype); /* get type from file */
	    if (!strcmp(plotype,pltype)) /* if it is , load in parameters */
            {  
	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%s",printcap);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lppmm);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%d",&Lraster);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%d",&Lraster_charsize);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lraster_page);	/* not used any more */

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%d",&Lraster_resolution);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lright_edge);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lxoffset);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lyoffset);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lxoffset1);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lyoffset1);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%d",&Lxcharp1);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%d",&Lycharp1);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lwcmaxmin);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lwcmaxmax);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lwc2maxmin);

	       if (Efgets(s,1023,plotinfo,emessage) == 0) return 0;
	       sscanf(s,"%*s%lf",&Lwc2maxmax);
	       TPRINT2("PlotInfo: ppmm=%g raster=%d\n",
	                 Lppmm,Lraster);
               TPRINT3("raster_charsize=%d raster_page=%g resolution=%d\n",
                         Lraster_charsize,Lraster_page,Lraster_resolution);
               TPRINT3("right_edge=%d xoffset=%g yoffset=%g\n",
                         Lright_edge, Lxoffset, Lyoffset);
               TPRINT4("xoffset1=%g yoffset1=%g xcharp1=%d ycharp1=%d\n",
                         Lxoffset1,Lyoffset1,Lxcharp1, Lycharp1);
               TPRINT4("wcmaxmin=%g wcmaxmax=%g wc2maxmin=%g wc2maxmax=%g\n",
                         Lwcmaxmin,Lwcmaxmax,Lwc2maxmin,Lwc2maxmax);
               fclose(plotinfo);
	       return 1;
	    }
	 }
         p = fgets(s,1023,plotinfo);
      }
      Werrprintf("Could not find entry '%s' in devicetable",pltype);
      fclose(plotinfo);
      return 0;
   }
   else
   {
      TPRINT1("loadPlotInfo: trouble opening file '%s'\n",filepath);
      Werrprintf("Could not open devicetable file");
      return 0;
   }
}


/*******************************************************
Efgets()
	gets a stream into char *ss and returns 1 if ok
	if unsuccessful, prints message, closes stream
	and returns 0
	used for devicenames and devicetable files
*******************************************************/
int Efgets(char *ss, int size, FILE *filestream, char *message)
{ 
  char *p;
  if (!(p = fgets(ss,size,filestream)))
  {  
    Werrprintf("%s",message);
    fclose(filestream); 
    return 0;
   }
   else return 1;
}



/**************************************************/
/*						  */
/* setplotter:  initialize for plotting           */
/* and for raster printing			  */
/* set plot=1 and set up the following variables: */
/* mnumxpnts	total number of points, x axis    */
/* mnumypnts    total number of points, y axis    */
/* xcharpixels	number of pixels per char, x axis */
/* ycharpixels  number of pixels per char, y axis */
/* ymin		zero baseline above bottom	  */
/* ygraphoff	leaves two lines of alpha free 	  */
/* right_edge   size of right hand margin         */
/*						  */
/**************************************************/

int init_tog=0;

char PSHX[]="/gb1 { /ybs exch def   %% get y base\n\
  /xst exch def   %% get x start\n\
  /ydel exch def  %% get ydel\n\
  -1 1 {       %% organize for loop\n\
  pop 64 div neg 1 add setgray	  %% 64 gray levels\n\
  xst ybs moveto neg dup 	  %% a second copy of delx \n\
  0 ydel rlineto  0 rlineto 0 ydel neg rlineto closepath\n\
  fill xst exch add /xst exch def } for } def\n";

static char ARROW[]="/Adict 14 dict def\n\
  Adict begin\n\
  /mtrx matrix def\n\
end\n\
/A\n\
{ Adict begin\n\
  /hLen exch def\n\
  /hThich exch 2 div def\n\
  /aThick exch 2 div def\n\
  /tipy exch def /tipx exch def\n\
  /taily exch def /tailx exch def\n\
  /dx tipx tailx sub def\n\
  /dy tipy taily sub def\n\
  /Alength dx dx mul dy dy mul add sqrt def\n\
  /deg dy dx atan def\n\
  /base Alength hLen sub def\n\
  /savematrix mtrx currentmatrix def\n\
  tailx taily translate\n\
  deg rotate\n\
  0 aThick neg moveto\n\
  base aThick neg lineto\n\
  base hThich neg lineto\n\
  Alength 0 lineto\n\
  base hThich lineto\n\
  base aThick lineto\n\
  0 aThick lineto\n\
  closepath\n\
  savematrix setmatrix\n\
  end\n\
} def\n";

void
set_ploter_coordinate(int x, int y)
{
   orgx = x;
   orgy = y;
}

void
set_ploter_ppmm(double p)
{
        Lppmm = p;
}

double get_ploter_ppmm()
{
        return ( (double) Lppmm);
}

int  get_plotter_raster()
{
   return(Lraster);
}

void
ps_font(char *name)
{
  int k;

  if (!plotfile || inMiniPlot)
      return;
  ps_flush();
  if (name == NULL)
      return;
  if (strcmp(name, psFontName) == 0) {
      if (psFontSize == 1.0)
         return;
  }
  strcpy(psFontName, name);
  psFontSize = 1.0;
  xcharpixels = (int)((double)xcharp1 * ppmm / 5.91);
  ycharpixels = (int)((double)ycharp1 * ppmm / 5.91);
  k = (int) (3.880 * ppmm + 0.5);
  fprintf(plotfile,"/%s findfont %d scalefont setfont\n", name, k);
  revchar = 0;
}

char *get_ps_font()
{
   return psFontName;
}

int ps_linewidth(int n) {
   int old_pslw;

   if (plotfile == NULL)
       return(pslw);
   old_pslw = pslw;
   ps_flush();
   if (n > 0 && (n != pslw)) {
       pslw = n;
       fprintf(plotfile,"%d setlinewidth\n", pslw);
   }
   return(old_pslw);
}

// x,y is the point of left-bottom corner
void ps_ellipse(int x, int y, int w, int h) {
   int x0, y0, r;
   double s;

   if (plotfile == NULL)
      return;
   ps_flush();
   fprintf(plotfile,"G\n");
   s = (double) h / (double) w;
   r = w / 2;
   x0 = x + r;
   y0 = y + h / 2;
   y0 = (int) ((double)y0 / s);
   fprintf(plotfile,"1.0 %6.4f scale\n",s);
   fprintf(plotfile,"%d %d %d 0 360 arc sk\n", x0, y0, r);

   fprintf(plotfile,"R\n");
}

// x,y is the point of left-bottom corner
void ps_round_rect(int x, int y, int w, int h) {
   int x2, y2, r;

   if (plotfile == NULL)
      return;
   ps_flush();
   fprintf(plotfile,"G\n");
   if (w > h) r = w / 4;
   else r = h / 4;
   if (r > 30)
       r = 30;
   if (r < 2)
       r = 2;
   x2 = x + w;
   y2 = y + h;
   fprintf(plotfile,"%d %d mv\n",x, y + r);
   fprintf(plotfile,"%d %d %d %d %d arcto\n",x, y2, x2, y2, r);
   fprintf(plotfile,"4 {pop} repeat\n");
   fprintf(plotfile,"%d %d %d %d %d arcto\n",x2, y2, x2, y, r);
   fprintf(plotfile,"4 {pop} repeat\n");
   fprintf(plotfile,"%d %d %d %d %d arcto\n",x2, y, x, y, r);
   fprintf(plotfile,"4 {pop} repeat\n");
   fprintf(plotfile,"%d %d %d %d %d arcto\n",x, y, x, y2, r);
   fprintf(plotfile,"4 {pop} repeat\n");
   fprintf(plotfile,"sk\n");
   fprintf(plotfile,"R\n");
}

// x,y is the point of left-bottom corner
void ps_rect(int x, int y, int w, int h) {
   if (plotfile == NULL)
      return;
   ps_flush();
   fprintf(plotfile,"%d %d mv\n",x, y);
   fprintf(plotfile,"0 %d lr\n", h);
   fprintf(plotfile,"%d 0 lr\n", w);
   fprintf(plotfile,"0 %d lr\n", -h);
   fprintf(plotfile,"%d 0 lr\n", -w);
   fprintf(plotfile,"sk\n");
}

// the points are double
void ps_Dpolyline(Dpoint_t *pnts, int npts) {
   int n, c;

   if (plotfile == NULL)
      return;
   if (npts < 2)
       return;
   ps_flush();
   c = 1;
   fprintf(plotfile,"P\n");
   fprintf(plotfile,"%d %d mv\n", (int)pnts[0].x, mnumypnts - (int)pnts[0].y);
   for (n = 1; n < npts; n++) {
       fprintf(plotfile,"%d %d ls\n", (int)pnts[n].x,  mnumypnts -(int)pnts[n].y);
       c++;
       if (c > PS_OPER_MAX) {
           if (n < (npts - 2)) {
               fprintf(plotfile,"sk\n");
               fprintf(plotfile,"P\n");
               fprintf(plotfile,"%d %d mv\n", (int)pnts[n].x,
                           mnumypnts -(int)pnts[n].y);
               c = 1;
           }
           else
               c = 2;
       }
   }
   fprintf(plotfile,"sk\n");
}

void ps_fillPolygon(Gpoint_t *pnts, int npts) {
    int n;

   if (plotfile == NULL)
      return;
   if (npts < 2)
       return;
   ps_flush();
   fprintf(plotfile,"P\n");
   fprintf(plotfile,"%d %d mv\n", pnts[0].x, mnumypnts - pnts[0].y);
   for (n = 1; n < npts; n++)
       fprintf(plotfile,"%d %d ls\n", pnts[n].x,  mnumypnts - pnts[n].y);
   fprintf(plotfile,"CP fill\n");
}

void ps_arrow(int x1, int y1, int x2, int y2, int thick) {
   int w, hw, hl;
   double r;

   if (plotfile == NULL)
      return;
   ps_flush();
   r = ps_ppmm / 11.8;
   if (thick < 1)
      thick = pslw;
   w = (int) (r * thick);
   if (w < 1)
      w = 1;
   hw = w + (int) (r * 20);  // head width
   hl = (int) (r * 30);  // head length
   if (hl < 8)
      hl = 8;
   fprintf(plotfile,"P\n");
   fprintf(plotfile,"%d %d %d %d %d %d %d A\n", x1, y1, x2, y2, w, hw, hl);
   fprintf(plotfile,"fill\n");
}

static
void ps_default_font_color()
{
    psFontSize = 0.1;
    psRed = 2.0;
    if (init_tog && plotfile != NULL) {
        ps_font("Courier-Bold");
        fprintf(plotfile,"0.0 0.0 0.0 C\n");
    }
    //  ps_font("Helvetica-Bold");
}

static  int psHeight, psWidth;

void write_psfile_header()
{
     if (plotfile == NULL)
         return;
     fprintf(plotfile, "%%%%BeginProlog\n");
     /* ybar definition */
     fprintf(plotfile,
             "/yb { moveto lineto stroke } def\n");
     fprintf(plotfile,"/yhm { /ybxx exch def  -1 exch { exch ybxx add moveto 0 exch rlineto } for  stroke } def\n");
     fprintf(plotfile,"/hb { { 1 exch rlineto } repeat } def\n");
     fprintf(plotfile,"/vb { { 1 rlineto } repeat } def\n");
     fprintf(plotfile,
             "/yv { moveto 0 rlineto stroke } def\n");
     fprintf(plotfile,
             "/yh { moveto 0 exch rlineto stroke } def\n");
         /* these defines shorten the plot file substantially */
     fprintf(plotfile,"/mv { moveto } def\n");
     fprintf(plotfile,"/rv { rmoveto } def\n");
     fprintf(plotfile,"/ls { lineto } def\n");
     fprintf(plotfile,"/lr { rlineto } def /sk {stroke} def\n");
     fprintf(plotfile,
             "/gb { setgray moveto exch dup 0\n");
     fprintf(plotfile,
             "rlineto exch 0 exch rlineto neg 0\n");
     fprintf(plotfile,
             "rlineto closepath fill } def\n");
     fprintf(plotfile, "/bx { exch dup 0 rlineto\n");
     fprintf(plotfile, "exch 0 exch rlineto\n");
     fprintf(plotfile, "neg 0 rlineto closepath fill } def\n");
     fprintf(plotfile,"/C { setrgbcolor } def\n");
     fprintf(plotfile,"/T { dup stringwidth pop  neg 0 rmoveto show } def\n");
     fprintf(plotfile,"/G { gsave } def\n");
     fprintf(plotfile,"/R { grestore } def\n");
     fprintf(plotfile,"/P { newpath } def\n");
     fprintf(plotfile,"/CP { closepath } def\n");
     /****
         fprintf(plotfile,
              "/Courier-Bold findfont %d scalefont setfont\n",(int)(3.88*ppmm+0.5));
      ****/
         /* Courier is mono-spaced - necessary for Vnmr */
     fputs(PSHX,plotfile);
     fputs(ARROW,plotfile);
     write_ps_image_header(plotfile);
     fprintf(plotfile, "%%%%EndProlog\n");
}

static int setPS_RasterPage() 
{
     double tmp;
     double  fw, fh;
     int    xoffset,yoffset;
     int    xoffset1,yoffset1;
     int    vplot_mode;
     int    newType;
     
     vplot_mode = is_vplot_session(0);
     xoffset = (int)Lxoffset;
     yoffset = (int)Lyoffset;
     xoffset1 = (int)Lxoffset1;
     yoffset1 = (int)Lyoffset1;
     newType = 0;
     if (LpaperWidth > 20.0 && LpaperHeight > 20.0)
     {
         newType = 1;
         if (Lleft_edge < 0.0)
             Lleft_edge = 0.0;
         if (Lright_edge < 0.0)
             Lright_edge = 0.0;
         if (Ltop_edge < 0.0)
             Ltop_edge = 0.0;
         if (Lbottom_edge < 0.0)
             Lbottom_edge = 0.0;
     }
#ifdef SUN
     if (raster == 1 || raster == 2) {
         if (!create_pixmap())
            return 0;
     }
#endif
     if (raster < 3) {
         (*(*active_gdevsw)._charsize)(1.0);
         return 1;
     }
         /* PostScript */
     if (init_tog == 0)
     {
             /*
             ** Make a Conforming PS header
             ** cope with 8.5x11 and A4
             ** with 15mm borders -> max area = 250 x 186
             ** EM scale
             ** 43,43,569,751
             ** and  11 x17 and A3
             ** 43,43,751,1180
             */
         if (newType)
         {
             psw = (int) (LpaperWidth * 72.0 / 25.4 + 0.5);
             psh = (int) (LpaperHeight * 72.0 / 25.4 + 0.5);
         }
         else
         {
             if (raster == 3)
             {
                psw = (int) (Lwcmaxmax / 25.4) * 72 + PSMARGIN * 2;
                psh = (int) (orig_Lwc2maxmax / 25.4) * 72 + PSMARGIN * 2;
             }
             else
             {
                psw = (int) (orig_Lwc2maxmax / 25.4) * 72 + PSMARGIN * 2;
                psh = (int) (Lwcmaxmax / 25.4) * 72 + PSMARGIN * 2;
             }
         }
         if (Lwcmaxmax > orig_Lwc2maxmax) {
              psWidth = (int) ((double)Lwcmaxmax * ppmm);
              psHeight = (int) ((double)orig_Lwc2maxmax * ppmm);
         }
         else {
              psWidth = (int) ((double)orig_Lwc2maxmax * ppmm);
              psHeight = (int) ((double)Lwcmaxmax * ppmm);
         }
         fprintf(plotfile,"%%!PS-Adobe-3.0\n");
         fprintf(plotfile,"%%%%Title: VNMR Plotting\n");
         fprintf(plotfile,"%%%%Creator: Ace Spectroscopist\n");
         if (vplot_mode < 2) {
              fprintf(plotfile,"%%%%BoundingBox: 0 0 %4d %4d\n",psw,psh);
/*
                fprintf(plotfile,"%%%%BoundingBox:%4d %4d %4d %4d\n",
                         43,43,psw,psh);
*/
         }
         // fprintf(plotfile,"%%%%EndComments\n%%%%EndProlog\n%%%%Page 1 1\n");
         fprintf(plotfile,"%%wcmaxmax=%3.1f wc2maxmax=%3.1f\n",
               Lwcmaxmax,orig_Lwc2maxmax);
         fprintf(plotfile,"gsave\n");

         write_psfile_header();

         if (newType) {
             double tmp;
             int  doubleSides = 0;
             /*********
             fprintf(plotfile,"/setpagedevice where\n");
             fprintf(plotfile,"  { pop 1 dict\n");
             fprintf(plotfile,"    dup /PageSize [%d %d] put\n", psw, psh);
             fprintf(plotfile,"    setpagedevice\n");
             fprintf(plotfile,"  } if\n");
             *********/
             if ( ! P_getreal(GLOBAL,"plduplex" ,&tmp, 1) )
                doubleSides = (int) (tmp+0.1);
             if (doubleSides == 1) {
                fprintf(plotfile,"%%%%BeginSetup\n");
                fprintf(plotfile,"<< /PageSize [%d %d] /ManualFeed false /NumCopies 1 /Duplex true >> setpagedevice\n", psw, psh);
                fprintf(plotfile,"%%%%EndSetup\n");
             }
             else {
                fprintf(plotfile,"/setpagedevice where\n");
                fprintf(plotfile,"  { pop 1 dict\n");
                fprintf(plotfile,"    dup /PageSize [%d %d] put\n", psw, psh);
                fprintf(plotfile,"    setpagedevice\n");
                fprintf(plotfile,"  } if\n");
             }
         }
         fprintf(plotfile,"%%%%Page: 1 1\n");
         init_tog = 1;

#ifdef VNMR
         imagefile_init();
#endif
         ps_default_font_color();
         // ps_font("Courier-Bold");
         // ps_font("NewCenturySchlbk-Italic");
         if (vplot_mode < 2)
         {
              if (raster == 3)
              {
                  if (newType)
                  {
                     transX = (int) (Lleft_edge * 72.0 / 25.4);
                     transY = (int) (Lbottom_edge * 72.0 / 25.4);
                  }
                  else 
                  {
                     transX = PSMARGIN + (int) (xoffset*72 / 25.4);
                     transY = PSMARGIN + (int) (yoffset*72 / 25.4);
                  }
                  fprintf(plotfile,"%d %d translate\n", transX, transY);
              }
              else
              {
                  if (newType)
                  {
                     xoffset = (int) (Lbottom_edge * 72.0 / 25.4);
                     transX = psw - xoffset;
                     transY = (int) (Lleft_edge * 72.0 / 25.4);
                  }
                  else 
                  {
                    /* ^^ psw 43 translate 90 rotate imports correctly */
                    /* add a half of ymin */
                     transX = psw - (int) (yoffset*72 / 25.4) - PSMARGIN + 10;
                     transY = PSMARGIN + (int) (xoffset*72 / 25.4);
                  }
                  fprintf(plotfile,"%d %d translate 90 rotate\n",transX, transY);
              }
         }
         scaleX = EMMM/ppmm;
         scaleY = EMMM/ppmm;
         fprintf(plotfile,"%6.4f %6.4f scale\n",scaleX, scaleY);
         if (P_getreal(GLOBAL,"pslw" ,&tmp, 1))
         {
                fprintf(plotfile,"1 setlinewidth\n");
                pslw = 1;
         }
         else
         {
                int itmp;
                itmp = (int) tmp;
                if (itmp < 0)
                   itmp = 0;
                if (itmp > 100)
                   itmp = 100;
                fprintf(plotfile,"%d setlinewidth\n",itmp);
                pslw = itmp;
         }
         orig_pslw = pslw;
         fprintf(plotfile,"1 setlinecap\n");
         cp_tog = 0;
         init_tog = 1;
             /* fill types for boxes */
         // (*(*active_gdevsw)._color)(1,1);
         // fprintf(plotfile,"0.0 0.0 0.0 C\n");
         // psRed = 2.0;
     }
     cp_tog=0;
     ps_b_init();
     (*(*active_gdevsw)._charsize)(1.0);

     if (newType)
     {
         tmp = 0.0;
         fw = LpaperWidth - Lleft_edge - Lright_edge;
         fh = LpaperHeight - Ltop_edge - Lbottom_edge;
         if (Lraster == 2 || Lraster == 4) { // landscape     
             if (LpaperHeight > LpaperWidth)
                 tmp = 1.0;
         }
         else {
             if (LpaperWidth > LpaperHeight)
                 tmp = 1.0;
         }
         if (tmp > 0.0) {
             fw = fh;
             fh = LpaperWidth - Lleft_edge - Lright_edge;
         }
         pltXpnts = (int)(fw*ppmm);
         pltYpnts = (int)(fh*ppmm);
         if (pltXpnts < mnumxpnts)
             pltXpnts = mnumxpnts;
         if (pltYpnts < mnumypnts)
             pltYpnts = mnumypnts;
     }
     else {
         pltXpnts = 2957;
         pltYpnts = 2208;
     }
     return 1;
}

static void
init_new_page() {
    int  n;

    if (layoutMode > 2)
        infoPlotGapX = (int) (ppmm * 10.0);
    else
        infoPlotGapX = (int) (ppmm * 5.0);
    infoPlotY = pageHeaderB - ps_info_font_height();
    n = mnumxpnts / ps_info_font_width();
    pageColumns = n / 50;
    if (pageColumns < 2)
        pageColumns = 2;
    n = pageColumns - 1;
    columnWdith = (mnumxpnts - infoPlotGapX * n) / pageColumns;
    if (layoutMode > 1)
        infoPlotGapX = (int) (ppmm * 10.0);
    columnWdith2 = (mnumxpnts - columnWdith - infoPlotGapX * n) / n;
    columnIndex = 0;
    headerPlotted = 0;
    logoPlotted = 0;
    infoPlotX = 0;
    psRed = 2.0;
    psFontSize = 0.1;
}

int
setplotter()
{ 
  double x0v,y0v,tmp;
  int    e,xoffset,yoffset;
  int    vplot_mode;

   vplot_mode = is_vplot_session(0);
   save_raster_charsize = 0;
   plot = 0; /* as long as not everything has been set properly */
   raster = 0;
   active_gdevsw = &(gdevsw_array[C_TERMINAL]);
   xorflag = 0;   /* initialize to normal mode of display */
   /* get chart paper size */
   if ( (e=P_getreal(GLOBAL,"wcmax"  ,&wcmax,  1)) )
   {  P_err(e,"global ","wcmax:"); return 1; 
   }
   if ( (e=P_getreal(GLOBAL,"wc2max" ,&wc2max, 1)) )
   {  P_err(e,"global ","wc2max:"); return 1; 
   }
   if ( (e=P_getreal(GLOBAL,"x0" ,&x0v, 1)) )
   {  P_err(e,"global ","x0:"); return 1; 
   }
   if ( (e=P_getreal(GLOBAL,"y0" ,&y0v, 1)) )
   {  P_err(e,"global ","y0:"); return 1; 
   }
   if ( (e=P_getstring(GLOBAL,"plotter" ,pltype, 1,60)) ) 
      pltype[0] = '\0';
   //   strcpy(pltype, noneDevice);
   if (plotterAssigned == 0) {
      setPlotterName(pltype);
   }
   if (strcmp(pltype, noneDevice)==0)
   {  Werrprintf("no plotter on system or plotter undefined");
      return 1;
   }
   /* set all the appropriate variables for plot device*/
   /* first make some checks on chart size */
   if (wcmax > (double)Lwcmaxmax || wcmax < (double)Lwcmaxmin)
   {  
	if (vplot_mode > 0)
	{
	   if (wcmax < (double)Lwcmaxmin)
		wcmax = Lwcmaxmin;
	   else
		wcmax = Lwcmaxmax;
	}
	else
	{
	   // Werrprintf("wcmax out of bound, %g<wcmax<%g",Lwcmaxmin,Lwcmaxmax);
	   if (wcmax < Lwcmaxmin)
		wcmax = Lwcmaxmin;
	   if (wcmax > Lwcmaxmax)
		wcmax = Lwcmaxmax;
           P_setreal(GLOBAL,"wcmax", wcmax, 1);
       	   // return 1;
	}
   }
   if (wc2max > (double)Lwc2maxmax || wc2max < (double)Lwc2maxmin)
   {  
	if (vplot_mode > 0)
	{
	   if (wc2max < (double)Lwc2maxmin)
		wc2max = (double)Lwc2maxmin;
	   else
		wc2max = (double)Lwc2maxmax;
	}
	else
	{
	  // Werrprintf("wc2max out of bound, %g<wc2max<%g",Lwc2maxmin,Lwc2maxmax);
	   if (wc2max < (double)Lwc2maxmin)
		wc2max = (double)Lwc2maxmin;
	   if (wc2max > (double)Lwc2maxmax)
		wc2max = (double)Lwc2maxmax;
           P_setreal(GLOBAL,"wc2max", wc2max, 1);
          // return 1;
	}
   }
   ppmm	  = (double)Lppmm;
   ps_ppmm  = ppmm;
   raster = Lraster;

/*  C_RASTER entry in `gdevsw_array' is only available on the SUN */

   if ( plot )
      switch (raster)
      {
	case 0:	active_gdevsw = &(gdevsw_array[C_PLOT]); break;
#ifdef SUN
	case 1:
	case 2: active_gdevsw = &(gdevsw_array[C_RASTER]); break;
#endif 
	case 3: active_gdevsw = &(gdevsw_array[C_PSPLOT]); break;
	case 4: active_gdevsw = &(gdevsw_array[C_PSPLOT]); break;
	default:
		Werrprintf( "value of %d not valid for raster", raster );
		Wscrprintf( "check your devicetable file\n" );
		return( 1 );
      }
   else
      active_gdevsw = &(gdevsw_array[C_TERMINAL]);
   raster_resolution = Lraster_resolution;
   raster_charsize = Lraster_charsize;
   ymin = (int)(BASEOFFSET*ppmm);   /* 1/5 mm user inits */
   if (inMiniPlot)
       ymin = 0;
 
   // if (vplot_mode > 0) { // jplot
   //     if (raster == 4) /* PS_AR */
   //        ymin = 0;
   // }
   mnumxpnts = (int)(wcmax*ppmm);   /* 1/5 mm user units */   
   mnumypnts = (int)(wc2max*ppmm) + ymin;
   xcharp1     = Lxcharp1;
   ycharp1     = Lycharp1;
   right_edge  = (int)Lright_edge;
   left_edge   = (int)Lleft_edge;
   top_edge    = (int)Ltop_edge;
   bottom_edge = (int)Lbottom_edge;
   xoffset     = (int)(Lxoffset * x0v + Lxoffset1);
   yoffset     = (int)(Lyoffset * y0v + Lyoffset1);
   ygraphoff   =  0;
   pltXpnts = mnumxpnts;
   pltYpnts = mnumypnts;

   vplot_mode = is_vplot_session(0);
   if (vplot_mode > 2)
   {
   	is_vplot_session(0);
   	plot = 1;
      	in_plot_mode = 1;
   	return 0;
   }
   orgx = 0;
   orgy = 0;
   ymultiplier = 1;
   hires_ps = 0;
   psRed = 2.0;

   set_raster_gray_matrix(raster_resolution);

   if (!plotfile)	/* open plot file, if not open already */
   { 
      curpencolor = -1;

#ifdef UNIX
   /* create a unique filename based on system name, pid and sequence number */
   /* place this file in systemdir/tmp  */
      sprintf(plotpath,"%s/tmp/%sPLT%d%d",
		systemdir,HostName,(int)HostPid,plotFileSeq++);
#else 
      sprintf(plotpath,"%splotfile.%d_%d",userdir,HostPid,plotFileSeq++);
#endif 
      if (plotFileSeq > 999999)
          plotFileSeq = 1;
      plotfile=fopen(plotpath,"w+");
      if (plotfile)
      { 
	 switch (raster)
	 {
#ifdef SUN
            case 1: case 2: disp_plot("RAST"); /* display RAST on status screen */
	       break;
#endif 
	    case 0: disp_plot("PLOT"); /* display PLOT on status screen */
	       break;
            case 3: case 4: disp_plot("PS");
	       break;
	    default:
	       Werrprintf( "value of %d not valid for raster", raster );
	       Wscrprintf( "check your devicetable file\n" );
	       return( 1 );
         }
         init_new_page();
      }
   }
   if (plotfile)
   {
      plot = 1;
      in_plot_mode = 1;
      switch (raster)
      {
	case 0:	active_gdevsw = &(gdevsw_array[C_PLOT]); break;
#ifdef SUN
	case 1:
	case 2: active_gdevsw = &(gdevsw_array[C_RASTER]); break;
#endif 
	case 3: 
	case 4: active_gdevsw = &(gdevsw_array[C_PSPLOT]);
                if ( ! P_getreal(GLOBAL,"pshr" ,&tmp, 1) ) 
                {
                   hires_ps = (int) tmp;
                }
                break;
	default:
		Werrprintf( "value of %d not valid for raster", raster );
		Wscrprintf( "check your devicetable file\n" );
		return( 1 );
      }
      if (raster > 0)
      {  
         setPS_RasterPage();
         return 0;
      }
      else
      {  
         if (curpencolor==-1)	/* do this only for the first time in a plot */
           {
             fprintf(plotfile,"DF;\n");
             fprintf(plotfile,"\033.M50;;10:\n");/* select xon/xoff handshake */
             fprintf(plotfile,"\033.N10;19:\n");/* select xon/xoff handshake */
             fprintf(plotfile,"\033.I81;;17:\n");/* select xon/xoff handshake */
             /* initialize the plotter */
             fprintf(plotfile,"IN;IP%d,%d,%d,%d;\n",xoffset,yoffset,
	       (int)(xoffset+40/ppmm*mnumxpnts),
               (int)(yoffset+40/ppmm*mnumypnts));
             fprintf(plotfile,"SC%d,%d,%d,%d;\n",0,mnumxpnts,0,mnumypnts);
             /* fill types for boxes */
             /* the next line is unnecessary and causes Zeta problems */
             /* fprintf(plotfile,"FT3,3;\n"); */
             (*(*active_gdevsw)._color)(1,1);
           }
      }
      (*(*active_gdevsw)._charsize)(1.0);
      return 0;
   }
   else 
   {  Werrprintf("unable to open plotfile %s",plotpath);
      return 1;
   }
}

static void dicomPlot(char *filename)
{
    char data[512];
    int port;
    int dpi;
    int saveToFile;

    if (filename != NULL && (strlen(filename) > 0))
         saveToFile = 1;
    else
         saveToFile = 0;
    if (!saveToFile) 
    {
        if (strlen(PlotterHost) < 1) {
            Werrprintf("Dicom printer hostname was not defined in file devicenames.");
            return;
        }
        if (strlen(LplotterPort) < 1) {
            Werrprintf("Dicom printer port was not defined in file devicenames. ");
            return;
        }
    }
    port = atoi(LplotterPort);
    dpi = ppmm * 25.4;
    if (port <= 0)
        port = 104;
    if (dpi <= 10)
        dpi = 36;
    sprintf(data, "%s/bin/createdicom -infile %s -tag %s/user_templates/plot/dicom.default -dpi %d -outfile %s.dcm",
           systemdir, plotpath, systemdir, dpi, plotpath );
    system(data);
    sprintf(data, "%s.dcm", plotpath);
    if (! access(data, R_OK))
    {
         if (filename != NULL && (strlen(filename) > 0))
         {
             sprintf(data, "mv('%s.dcm', '%s')\n", plotpath, filename);
             Winfoprintf("Plot file saved to file %s\n", filename);
             execString(data);
         }
         else
         {
              if (strlen(PlotterHost) < 1) {
                  Werrprintf("Dicom printer hostname was empty. ");
                  return;
              }
              sprintf(data, "%s/bin/dicomlpr -file %s.dcm -port %d -host %s",
                     systemdir, plotpath, port, PlotterHost);
              system(data);
         }
    }
    unlink(plotpath);
    sprintf(data, "%s.dcm", plotpath);
    if (! access(data, R_OK)) {
         unlink(data);
    }
}

static void
dupHeaderFile(char *src) {
    int  fd;

    if (headerAssigned > 1)
        unlink(headerPath);
    headerPath[0] = '\0';
    headerAssigned = 0;
    if (src == NULL || strlen(src) < 1)
        return;
    sprintf(headerPath, "/tmp/hdXXXXXX");
    fd = mkstemp(headerPath);
    if (fd < 0) {
        strcpy(headerPath, src);
        headerAssigned = 1;
        return;
    }
    close(fd);
    sprintf(TmpOsPlotterName,"cp %s %s", src, headerPath);
    system(TmpOsPlotterName);
    headerAssigned = 3;
}

static void
plotHeader() {
    int x, w;

    if (headerAssigned == 0)
        return;
    if (headerPlotted != 0)
        return;
    if (strlen(headerPath) < 2)
        return;
    setplotter();
    ps_linewidth(1);
    if (raster == 4)
        x = mnumxpnts / 4;
    else
        x = 0;
    w = mnumxpnts - x;
    headerPlotted = plot_header(headerPath, x, pageHeaderY, w, pageHeaderH);
    ps_flush();
    ps_linewidth(orig_pslw);
    ps_default_font_color();
}

static void plotLogo()
{
    double tmpX, tmpY, tmpH, tmpW;
    int sizescale;


    if (logoAssigned == 0)
        return;
    if (logoPlotted != 0)
        return;
    if (strlen(logoPath) < 1)
    {
        if (P_getstring(GLOBAL,"plotlogo",logoPath,1,MAXPATH) != 0)
           return;
    }
    if (strlen(logoPath) < 1)
       return;
    if (logoPath[0] != '/')
    {
       if ( ! appdirFind(logoPath, "iconlib", logoPath, NULL, R_OK) )
               return;
    }
    
    if (logoPos)
    {
       tmpX = logoXpos;
       tmpY = logoYpos;
    }
    else
    {
       tmpX = 0;
       tmpY = logoY;
    }
    if (logoSize)
    {
       sizescale = 1;
       tmpW = logoWidth;
       tmpH = logoHeight;
    }
    else
    {
       sizescale = 0;
       tmpW = mnumxpnts / 4;
       tmpH = pageHeaderH;
    }
    logoPlotted = plot_logo(logoPath, tmpX, tmpY, tmpW, tmpH, sizescale);
    ps_flush();
}

#ifdef OLD
static void setPSpaperSize(int toDo)
{
    double dw, dh;
    int   iw, ih;
    int   tmpFD;

    iw = psw;
    ih = psh;
    if (LpaperWidth < 20.0 || LpaperHeight < 20.0)
    {
        if (iw < 612)
            iw = 612; // letter size
        if (ih < 792)
            ih = 792;
    }
    if (toDo == 0)
    {
        dw = LpaperWidth - 215.9;
        dh = LpaperHeight - 279.4;
        if (dw < 2.0 && dw > -2.0)
        {
            if (dh < 2.0 && dh > -2.0)
               return;
        }
    }
    if (iw < 72 || ih < 72)
    {
        iw = 612;
        ih = 792;
    }

    strcpy(TmpPlotterName, "/vnmr/tmp/pltXXXXXX");
    tmpFD = mkstemp(TmpPlotterName);
    if (tmpFD < 0)
    {
        strcpy(TmpPlotterName, "/vnmr/tmp/pltXXXXXX");
    }
    else
    {
        close(tmpFD);
        unlink(TmpPlotterName);
    }
    sprintf(TmpOsPlotterName, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=pswrite -dDEVICEWIDTHPOINTS=%d -dDEVICEHEIGHTPOINTS=%d -sOutputFile=%s -q %s", psw, psh, TmpPlotterName, plotpath); 

    system(TmpOsPlotterName);
    if (access(TmpPlotterName, R_OK) == 0)
    {
        sprintf(TmpOsPlotterName, "mv %s %s", TmpPlotterName, plotpath);
        system(TmpOsPlotterName);
    }
}
#endif


/*---------------------------------------------------------------------------
|
|   systemPlot
|
|   This procedure creates the appropriate system command to plot a 
|   plot file on the current plotter.  This routine handles local and
|   remote plotters with either shared or non shared file systems
|
+---------------------------------------------------------------------------*/
void systemPlot(char *filename, int retc, char *retv[])
{  char s[MAXPATH * 2];
   extern FILE *popen_call();
   extern char *fgets_nointr();
   FILE        *stream;
   int         len;
   char        *p;
   char        data[1024];

   /* create system command to plot the plotfile */
   /* command structure looks like    plot filename printername    */
   /* the shell routine plot makes a symbolic link to the plotfile */
   /* and deletes it when it is finished with it                   */

   if (strcmp(PlotterFormat, "DICOM") == 0) 
   {
       dicomPlot(filename);
       if (retc > 0)
         retv[0] = newString("0");
       return;
   }
#ifdef LINUX
   if (plotterAssigned == 0) {
       if ( P_getstring(GLOBAL,"plotter" ,pltype, 1,60) != 0 )
           pltype[0] = '\0';
       setPlotterName(pltype);
   }
   if (raster >= 3)
       ps_to_pcl(filename);
   if ((filename != NULL) && (strlen(filename) > 0))
   {
      /* print to a file */
      p = PlotterName;
      if (OsPlotterName != NULL)
         p = OsPlotterName;
      sprintf(s,"vnmrplot \"%s\" \"%s\" %s",plotpath, p, filename);
   }
   else if (!strcmp(PlotterName, noneDevice)) {
      sprintf(s,"vnmrplot \"%s\" \"%s\" clear",plotpath, PlotterName);
   }
   else {
      p = OsPlotterName;
#ifdef __INTERIX
     /***  vnmrplot will work without printer name
      if (p == NULL || (strlen(p) < 1)) {
         getDefaultPlotterName();
         p = defaultPlotterName;
         if (defaultPlotterName != NULL) {
            setenv("nmrplotter", defaultPlotterName, 1);
            setenv("PRINTER", defaultPlotterName, 1);  // for Windows
         }
      }
     ***/
#endif

      if (p == NULL || (strlen(p) < 1)) // print to default printer
          sprintf(s,"vnmrplot \"%s\"", plotpath);
      else
          sprintf(s,"vnmrplot \"%s\" \"%s\"",plotpath, p);
   }
#else
   if ((filename != NULL) && (strlen(filename) > 0))
   {
      sprintf(s,"vnmrplot %s \"%s\" %s",plotpath,PlotterName,filename);
   }
   else if (!strcmp(PlotterName,noneDevice) || !strcmp(PlotterName,""))
   {
      /* delete the file */
      sprintf(s,"vnmrplot %s none clear", plotpath);
   }
   else if (!strcmp(PlotterHost,HostName) || (PlotterShared[0] == 'N') ||
		(PlotterShared[0] == 'n') )
   {
      sprintf(s,"vnmrplot %s \"%s\"",plotpath,PlotterName);
      TPRINT1("vnmrplot command '%s'\n",s);
   }
   else /* Remote shared file system,  send a rsh command */
   {
      sprintf(s,"rsh %s %s/bin/vnmrplot %s \"%s\"",
		PlotterHost,systemdir,plotpath,PlotterName);
      TPRINT1("remote vnmrplot command '%s'\n",s);
   }
#endif  /* LINUX */

   if ((stream = popen_call( s, "r")) != NULL)
   {
      p = fgets_nointr(data,1024,stream);
      while (p != NULL)
      {
         len = strlen(data);
         if (len > 0)
         {
            if (data[len - 1] == '\n')
               data[len - 1] = '\0';
            if (retc > 0)
               retv[0] = newString(data);
            else
               Winfoprintf("%s",data);
         }
         p = fgets_nointr(data,1024,stream);
      }
      pclose_call(stream);
   }
   if (access(plotpath, F_OK))
       unlink(plotpath);
}

/****************************************************/
/* plotpage:	close the plot file and change page */
/****************************************************/
void plotpage(int n, char *filename, int retc, char *retv[])
{
    int x, y, len;
    int hasImages;
    char *p11Id;

    if (!plotfile)
    {
         if (n!=-1)	/* "page(-1)" used in _plotter to force page */
            Werrprintf("No active plot\n");
         return;
    }
      
    hasImages = 0;
    if ((raster > 0) && ( raster < 3))
#ifdef SUN
    { 
#ifdef VNMRJ
            len = 0;
            p11Id = get_p11_id();
            if (p11Id != NULL)
                len = (int) strlen(p11Id);
            if (len > 0)
            {
                color(SCALE_COLOR);
                y = mnumypnts - ycharpixels * 2;
                x = mnumxpnts - xcharpixels * (len + 2);
 	        amove(x, y);
                dstring(p11Id);
            }
#endif 
          fprintf(plotfile,"%cE",27); 	/* reset printer */
          switch( get_plot_planes() )
          {           /* 4 or 3 planes per row,  depending on ink cartridges */       
              case      4:   fprintf(plotfile,"%c*r-4U",27); break;
              case      3:   fprintf(plotfile,"%c*r-3U",27); break;
                  default:                                   break;
          }

	  /* resolution (dots/inch) */
          if (raster_resolution<0)
            fprintf(plotfile,"%c*r%dS",27,1280*abs(raster_resolution)/192);
          else
            fprintf(plotfile,"%c*t%dR",27,raster_resolution);
#ifdef VNMRJ
	  dump_raster_image();
#else 
          fprintf(plotfile,"%c*r%dA\n",27,1);  /* start graphics at cursor */
	  dump_raster_image();
          fprintf(plotfile,"%c*rB",27);	/* end raster graphics */
#endif 
          fprintf(plotfile,"\n");
          fprintf(plotfile,"%cE",27); 	/* reset printer, eject paper */
          fclose(plotfile);
          systemPlot(filename,retc,retv); /* send out plot command to system */
          raster = 0;
          plotfile = 0;
          plot = 0;
	  active_gdevsw = &(gdevsw_array[C_TERMINAL]);
          disp_plot("    "); /* clear PLOT on status screen */
    }
#else 
      Werrprintf("VMS programming error:  RASTER is set\n");
#endif 
    else
    { 
      if (raster > 2) 
      {
	ps_flush();
#ifdef VNMRJ
        if (is_vplot_session(0) == 0)
        {
            len = 0;
            p11Id = get_p11_id();
            if (p11Id != NULL)
                len = (int) strlen(p11Id);
            if (len > 0)
            {
                color(SCALE_COLOR);
                if (mnumypnts > psHeight)
                     y = mnumypnts - ycharpixels * 2;
                else
                     y = psHeight - ycharpixels * 2;
                if (mnumxpnts > psWidth)
                     x = mnumxpnts - xcharpixels * (len + 2);
                else
                     x = psWidth - xcharpixels * (len + 2);
 	        amove(x, y);
                dstring(p11Id);
	        ps_flush();
            }
            else if (raster == 4) {
                // add empty string to make PDF viewer show landscape mode
 	        amove(transX, transY);
                dstring("  ");
	        ps_flush();
            }
        }

        plotLogo();
        plotHeader();

        if (pageNo < 3)   // page 1
	     hasImages = imagefile_flush();
        else
	     hasImages = ps_imagefile_flush();
#endif
        fprintf(plotfile, "save restore\n");
        fprintf(plotfile,
          "%%plotpage\nshowpage grestore\n%%%%Trailer\n%%%%Pages: 1\n%%%%EOF\n");
	  init_tog = 0;
      }
      else
      {
	  fprintf(plotfile,"SP0;\n");
          if (n!=0) fprintf(plotfile,"PG1;\n");
      }
      fclose(plotfile);
      plot = 0;
      plotfile = NULL;
      inMiniPlot = 0;
      infoPlotY = 0;
      columnIndex = -1;
      headerPlotted = 0;
      dupHeaderFile(NULL);
      logoPlotted = 0;
      logoAssigned = 0;
      logoPos = 0;
      logoSize = 0;
      plotDebug = 0;
      pageNo = 1;
      logoPath[0] = '\0';

#ifdef OLD
      if (raster == 3 || raster == 4) 
         setPSpaperSize(hasImages);
#endif
      systemPlot(filename,retc,retv); /* send out plot command to system */
      active_gdevsw = &(gdevsw_array[C_TERMINAL]);
      disp_plot("    "); /* clear PLOT on status screen */
    }
}
#endif

/* setup questions what scale for postscript
   300 dots per inch  11.81 dots per mm
   150 dots per inch   5.91 dots per mm
   page sizes and orientations
   writing scales
   fonts
*/

/* 
   PostScript Plotter Support 

   cp_tog 0 = no current path  1 = a current path 
   back end batching mechanism for file compression
   ps_key = 1 lines,  2 boxes 0 match anything (move)
   number_items = lines on stack for batch drivers
   xlast ybase yheight -> box driver only
   ybars are batched internal to the ybar driver 
*/

static int xlast,ybase,yheight,number_items,ps_key;
/* 
  allow for small op stack cartridges 
  increasing the max will not decrease file size much
*/

void
ps_set_linewidth(int w)
{
	if (w > 0)
	   lineWidth = w;
}

/**************
perform batch initialization 
*****************/
void ps_b_init()
{
  ps_key = 0;
  number_items = 0;
  ybase = -1;
  xlast = 0;
}

void ps_flush()
{
   if (!plotfile)
      return;
   if ((number_items > 0) && (ps_key == 1))
   {
     fprintf(plotfile,"sk\n");
   }
   if ((number_items > 0) && (ps_key == 2))
   {
     if (xlast > 0)
	fprintf(plotfile,"%d %d %d %d gb1\n",number_items,yheight,xlast+orgx,ybase+ orgy);
   }
   cp_tog = 0;
   /* if no need to flush lines or boxes then leave cp_tog */
   ps_key = 0;
   number_items = 0;
}

/*************************************************** 
   if number_items > PS_OPER_MAX or keys are non-zero 
   and different then draw items and re-establish current path 
***************************************************/
static int ps_path(int key)
{
   int trans;
   number_items++;
   trans = (ps_key != 0) && (key != 0) && (ps_key != key);
   if ((number_items > PS_OPER_MAX) || (trans))
   {
      ps_flush();
      cp_tog = 0;
   }
   if (key != 0)
     ps_key = key;
   return(cp_tog);
}

void
ps_endgraphics()
{ 
/*
   if (is_vplot_session(0) > 0)
	return;
*/
   if (ps_key == 2)
     number_items++;  /**** HACK *****/
   ps_flush();  /* forces completion of any drawing operations */
   plot = 0;
   raster = 0;
   active_gdevsw = &(gdevsw_array[C_TERMINAL]);
}

void
ps_graf_clear()
{ 
  ps_flush(); 
}

/***********/
void
ps_color(int c, int dum)
/***********/
{ 
  double red,grn,blu;
  int	k;

  (void) dum;
  if (!plotfile)
     return;
  if (maxpen < 2) {
     ps_flush();
     return;
  }
  if (c < 0)
     return;
  k = is_vplot_session(0);
  if (k > 0 && k < 8)
	return;
  if (k == 8)
     c = get_raster_color(c);
  get_ps_color(c,&red,&grn,&blu);
  if (psRed == red && psGrn == grn && psBlu == blu) {
     ps_flush();
     return;
  }
  psRed = red;
  psGrn = grn;
  psBlu = blu;
  ps_flush();  /* forces completion of any line segments */
  fprintf(plotfile,"%4.2f %4.2f %4.2f setrgbcolor\n",red,grn,blu);
}

void
ps_rgb_color(double r, double g, double b)
{
  double red,grn,blu;

  if (maxpen < 2) {
     ps_flush();
     return;
  }
  if (!plotfile)
     return;
  red = r / 255.0;
  grn = g / 255.0;
  blu = b / 255.0;
  if (psRed == red && psGrn == grn && psBlu == blu)
     return;
  psRed = red;
  psGrn = grn;
  psBlu = blu;
  ps_flush();  /* forces completion of any line segments */
  fprintf(plotfile,"%4.2f %4.2f %4.2f C\n",red,grn,blu);
}

void
jplot_charsize(double f)
{
  xcharpixels = (int)((double)xcharp1*f*ppmm/5.91);
  ycharpixels = (int)((double)ycharp1*f*ppmm/5.91);
  revchar = 0;
}

void
ps_fontsize(double f)
{
    jplot_charsize(f);
}

/**************/   
int
ps_charsize(double f)
/**************/
{
  int k;

   if (!plotfile || inMiniPlot)
      return(0);
   if (is_vplot_session(0) > 0)
      return(0);
  /* character size is independent of printer resolution */
  xcharpixels = (int)((double)xcharp1*f*ppmm/5.91);
  ycharpixels = (int)((double)ycharp1*f*ppmm/5.91);
  if (psFontSize == f)
      return(ycharpixels);
  ps_flush();  /* forces completion of any line segments */
  psFontSize = f;
  k = (int) (3.880*ppmm*f+0.5);
  fprintf(plotfile,"/%s findfont %d scalefont setfont\n", psFontName, k);
  revchar = 0;
  return(ycharpixels);
}

void
ps_amove(int x, int y)
{ 
  int xd,yd;
  ps_path(0);  /* let automatic force stroke dont set a key*/
  xd = x - xgrx;
  yd = y - xgry;
  xgrx=x;  
  xgry=y;  
  if (cp_tog > 0)
     fprintf(plotfile,"%d %d rv\n",xd,yd);
  else
  {
     fprintf(plotfile,"%d %d mv\n",x+orgx,y+orgy);
     cp_tog = 1;
  }
}

void
ps_rdraw(int x, int y)
{
   ps_path(1);  /* one segment of line (1)  */
   if (cp_tog == 0)
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   xgrx += x; xgry += y;
   if (x == 0 && y == 0)
   {
       fprintf(plotfile,"1 0 lr\n");
       fprintf(plotfile,"-1 0 lr\n");
   }
   else
       fprintf(plotfile,"%d %d lr\n",x,y);
}

void
ps_adraw(int x, int y)
{
   int xdel,ydel;
   /* k makes sure at least one pixel shows up */
   ps_path(1);
   if (cp_tog == 0)
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   if ((xgrx == x) && (xgry == y))
   {
     fprintf(plotfile,"1 0 lr\n");
     ps_flush();
   }
   else
   {
     xdel = x - xgrx;
     ydel = y - xgry;
     fprintf(plotfile,"%d %d lr\n",xdel,ydel);
   }
   xgrx = x; xgry = y;
}

void
ps_dchar(char ch)		 
{
   int kk;
   kk = revchar;
   ps_flush(); 
   if (cp_tog == 0) 
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   if (revchar)
      fprintf(plotfile,"-90 rotate\n");
   revchar = 0;
   if (((ch == '(') || (ch == ')')))
     fprintf(plotfile,"(\\%c) show\n",ch);
   else
     fprintf(plotfile,"(%c) show\n",ch);
   if (kk) 
      fprintf(plotfile,"90 rotate\n");
   xgrx += xcharpixels;
}

void
ps_dstring(char *s)
{
   int kk;
   char tbuff[512],*ic,*oc;
   char *oc1 = NULL;
   char *oc2 = NULL;
   ps_flush(); 
   if (cp_tog == 0) 
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   kk = revchar;
   if (revchar)
      fprintf(plotfile,"90 rotate\n");
   revchar = 0;
   ic = s;
   if (strlen(s) > sizeof(tbuff)/2)
   {
      oc = oc1 = oc2 = (char *) malloc(2* strlen(s) );
   }
   else
   {
      oc = oc1 = tbuff;
   }
   while (*ic != '\0')
   {
     if ((*ic == '(') || (*ic == ')'))
       *oc++ = '\\';
     *oc++ = *ic++;
   }
   *oc='\0';
   fprintf(plotfile,"(%s) show\n",oc1);
   if (oc2)
      free(oc2);
   if (kk) 
      fprintf(plotfile,"-90 rotate\n");
   xgrx += xcharpixels*strlen(s);
   cp_tog = 0;
}

void
ps_dvchar(char ch)
{
   /* this is not de-bracketted!! */
   ps_flush(); 
   if (cp_tog == 0) 
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   if (!revchar)
     fprintf(plotfile,"90 rotate\n");
   fprintf(plotfile,"(%c) show\n -90 rotate\n",ch);
   xgry += xcharpixels;
   revchar = 0;
   cp_tog=0;
}

/* this is not de-bracketted! */
void
ps_dvstring(char *s)
{
  ps_flush(); 
   if (cp_tog == 0) 
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
  fprintf(plotfile,"90 rotate (%s) show -90 rotate\n",s);
  cp_tog=0;
}

void
ps_vchar(char ch)
{
   int kk;
   kk = revchar;
   ps_flush(); 
   if (cp_tog == 0) 
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   if (revchar)
      fprintf(plotfile,"-90 rotate\n");
   revchar = 0;
   if (((ch == '(') || (ch == ')')))
     fprintf(plotfile,"(\\%c) show\n",ch);
   else
     fprintf(plotfile,"(%c) show\n",ch);
   if (kk) 
      fprintf(plotfile,"90 rotate\n");
   xgry -= ycharpixels;
   cp_tog=0;
}

void
ps_vstring(char *s)
{
   char *ic;
   ic = s;
   while (*ic != '\0')
   {
      ps_vchar(*ic);
      ic++;
   }
}

void
ps_rstring(char *s)
{
   int kk;
   char tbuff[512],*ic,*oc;
   char *oc1 = NULL;
   char *oc2 = NULL;

   kk = cp_tog;
   ps_flush();
   if (kk == 0)
      fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
   kk = revchar;
   if (revchar)
      fprintf(plotfile,"90 rotate\n");
   revchar = 0;
   ic = s;
   if (strlen(s) > sizeof(tbuff)/2)
   {
      oc = oc1 = oc2 = (char *) malloc(2* strlen(s) );
   }
   else
   {
      oc = oc1 = tbuff;
   }
   while (*ic != '\0')
   {
     if ((*ic == '(') || (*ic == ')'))
       *oc++ = '\\';
     *oc++ = *ic++;
   }
   *oc='\0';
   fprintf(plotfile,"(%s) T\n",oc1);
   if (oc2)
      free(oc2);
   if (kk)
      fprintf(plotfile,"-90 rotate\n");
   xgrx += xcharpixels*strlen(s);
   cp_tog = 0;
}

void
ps_dimage(int x, int y, int w, int h)		 
{
   (void) x; /* suppress warning message */
   (void) y; /* suppress warning message */
   (void) w; /* suppress warning message */
   (void) h; /* suppress warning message */
}

static
void hires_absolute(float *ptr, double scale, int dfpnt, int depnt, int npnts,
           int vertical, int offset, int maxv, int minv, int dcolor)
{
  register int i,npnt;
  register double nextval;
  register double maxval, minval;
  register double xfactor;
  register int xorig, yorig;

  cp_tog = 0;
  ps_flush(); 
  color(dcolor);

  if (depnt < 2)
     return;
  npnt = npnts; /* assign to register */
  fprintf(plotfile,"gsave\n");

  maxval = (double) (maxv - offset) / scale;
  minval = (double) (minv - offset) / scale;
  xfactor = (double) depnt / (double) (npnt - 1);

  if (vertical)
  { 
    xorig = orgx + mnumxpnts - offset;
    yorig = dfpnt + orgy;
    amove(xorig - (int) (*ptr * scale), yorig);
    fprintf(plotfile,"1.0  1.0 scale\n");

    for (i=0; i<npnt; i++)
    { 
        nextval = *(ptr + i);
        if (nextval > maxval)
           nextval = maxval;
        if (nextval < minval)
           nextval = minval;
        fprintf(plotfile,"%0.3f %0.3f lineto\n",
                xorig - (nextval * scale),
                i * xfactor + yorig);
        if (i && ( (i % 2000) == 0) )
        {
           fprintf(plotfile,"sk\n%0.3f %0.3f mv\n",
                xorig - (nextval * scale),
                i * xfactor + yorig);
        }
    }
  }  /* vertical */
  else
  { 

    xorig = dfpnt + orgx;
    yorig = orgy + offset;
    amove(xorig, yorig + (int) (*ptr * scale));
    fprintf(plotfile,"1.0 1.0 scale\n");
    xfactor = (double) depnt / (double) (npnt - 1);
    for (i=0; i<npnt; i++)
    { 
        nextval = *(ptr + i);
        if (nextval > maxval)
           nextval = maxval;
        else if (nextval < minval)
           nextval = minval;
        fprintf(plotfile,"%0.3f %0.3f lineto\n",
                i * xfactor + xorig,
                (nextval * scale) + yorig );
        if (i && ( (i % 2000) == 0) )
        {
           fprintf(plotfile,"sk\n%0.3f %0.3f mv\n",
                i * xfactor + xorig,
                (nextval * scale) + yorig );
        }
    }
  }
  fprintf(plotfile,"sk\n");

  fprintf(plotfile,"grestore\n");
  cp_tog = 0;
}

static
void hires_relative(float *ptr, double scale, int dfpnt, int depnt, int npnt,
           int vertical, int offset, int maxv, int minv, int dcolor)
{
  register int i, start, stop, pnts;
  register double nextval, lastval;
  register double maxval, minval;

  cp_tog = 0;
  ps_flush(); 
  color(dcolor);

  if (depnt < 2)
     return;
  fprintf(plotfile,"gsave\n");

  start = stop = 0;
  pnts = 0;
  maxval = (double) (maxv - offset) / scale;
  minval = (double) (minv - offset) / scale;

  if (vertical)
  { 
    amove(orgx + mnumxpnts - offset - (int) (*ptr * scale), dfpnt + orgy );
    fprintf(plotfile,"1.0  1.0 scale\n");
    fprintf(plotfile,"/vb { { %f rlineto } repeat } def\n",
           (double) depnt / (double) (npnt - 1));
    fprintf(plotfile,"/csm { currentpoint stroke moveto } def\n");

    for (i=0; i<npnt; i++)
    { 
	if (start == stop)
	{
          if (pnts > 0)
	     fprintf(plotfile,"\n%d vb\ncsm\n", pnts);
          pnts = 0;
	  stop = i;
	  start  = stop + PS_OPER_MAX;
	  if (start >= npnt)
	     start = npnt-1;
          if (start - stop < 1)
             break;
        }
        nextval = *(ptr + start);
        if (nextval > maxval)
           nextval = maxval;
        if (nextval < minval)
           nextval = minval;
        lastval = *(ptr + start -1);
        if (lastval > maxval)
           lastval = maxval;
        if (lastval < minval)
           lastval = minval;
        fprintf(plotfile,"%f ", (lastval - nextval) * scale);
        pnts++;
        start--;
        if (pnts % 12 == 0)
             fprintf(plotfile,"\n");
    }
    if (pnts > 0)
       fprintf(plotfile,"\n%d vb\n", pnts);
  }  /* vertical */
  else
  { 
    amove(dfpnt + orgx, (int) (*ptr * scale) + orgy + offset);
    fprintf(plotfile,"1.0 1.0 scale\n");
    fprintf(plotfile,"/hb { { %f exch rlineto } repeat } def\n",
           (double) depnt / (double) (npnt - 1));
    fprintf(plotfile,"/csm { currentpoint stroke moveto } def\n");
    for (i=0; i<npnt; i++)
    { 
	if (start == stop)
	{
          if (pnts > 0)
	     fprintf(plotfile,"\n%d hb\ncsm\n", pnts);
          pnts = 0;
	  stop = i;
	  start  = stop + PS_OPER_MAX;
	  if (start >= npnt)
	     start = npnt-1;
          if (start - stop < 1)
             break;
        }
        nextval = *(ptr + start);
        if (nextval > maxval)
           nextval = maxval;
        else if (nextval < minval)
           nextval = minval;
        lastval = *(ptr + start -1);
        if (lastval > maxval)
           lastval = maxval;
        else if (lastval < minval)
           lastval = minval;
        fprintf(plotfile,"%f ", (nextval - lastval) * scale);
        pnts++;
        start--;
        if (pnts % 12 == 0)
             fprintf(plotfile,"\n");
    }
    if (pnts > 0)
       fprintf(plotfile,"\n%d hb\n", pnts);
  }
  fprintf(plotfile,"sk\n");

  fprintf(plotfile,"grestore\n");
  cp_tog = 0;
}

void calc_hires_ps(float *ptr, double scale, int dfpnt, int depnt, int npnt,
           int vertical, int offset, int maxv, int minv, int dcolor)
{
   if (hires_ps == 2)
       hires_relative(ptr, scale, dfpnt, depnt, npnt,
           vertical, offset, maxv, minv, dcolor);
   else
       hires_absolute(ptr, scale, dfpnt, depnt, npnt,
           vertical, offset, maxv, minv, dcolor);
}

/**********************************************/
void
ps_ybars(int dfpnt, int depnt, struct ybar *out, int vertical,
         int maxv, int minv) 
/**********************************************/
{ 
  register int i,del,start,stop,base;
  void range_ybar();			
  ps_flush(); 
  /* first check vertical limits */
  if (maxv)
    range_ybar(dfpnt,depnt,out,maxv,minv);
  if (vertical)
  { 
    for (i=dfpnt; i<depnt; i++)
    { 
       /* is in mnumypnts space */
       if (out[i].mx>=out[i].mn) 
       {
	 del = out[i].mx - out[i].mn;
	 if (del == 0)
	   del = 1;
	 if (del < lineWidth)
	   del = lineWidth;
	 fprintf(plotfile,"%d %d %d yv\n",
	       del,mnumxpnts-out[i].mx+orgx,i+orgy);
	 /* mn went to mx */ 
       }
    }
  }
  else
  { 
    start = -1;
    stop = base = 0;
    for (i=dfpnt; i<depnt; i++)
    { 
      /* is in mnumypnts space */
      del = out[i].mx - out[i].mn;
      if (del == 0)
      {
	del =1;
      }
      if (del > 0)
      {
	if (del < lineWidth)
	   del = lineWidth;
	if (start == -1)
	{
	  start = i;
	  base  = out[i].mn;
	  stop  = start + PS_OPER_MAX;
	  if (stop >= depnt)
	     stop = depnt-1;
        }
        fprintf(plotfile,"%d %d\n",del,out[i].mn-base + orgy);
        if (i == stop)
        {
	   fprintf(plotfile,"%d %d %d yhm\n",stop+orgx,start+orgx,base);
	   start = stop + 1;
	   stop = start + PS_OPER_MAX;
	   if (stop >= depnt)
	     stop = depnt-1;
	   base = out[start].mn;
        }
      }
      else   /* del < 0 */
      {
	if (start >= 0)
	{
	  fprintf(plotfile,"%d %d %d yhm\n",i-1+orgx,start+orgx,base);
	  start = -1;
	  base = 0;
	}
      }
    }
  }
  cp_tog = 0;
}


void
ps_normalmode() {}

void
ps_xormode() {}

/************************************/
int
ps_grayscale_box(int oldx, int oldy, int x, int y, int shade)
/************************************/
{
  double  yshade;

  if ((x<1)||(y<1)) return 1;
  yshade = (double) shade * graysl - gray_offset;
  shade = (int) yshade;
  if (shade < 0)  return 1;
  if (shade > 64) shade = 64;
  /* if they match or not, they'll be right soon */
  if (ps_key != 2)
    fprintf(plotfile,"%% ps_key = %d number_items = %d\n",ps_key,number_items);
  ps_path(2);
  if (((oldy != ybase) || (yheight != y)) && (number_items > 0))
  {
    ps_flush();  
  }
  else if (oldx != xgrx)
    ps_flush();
  xlast = oldx+x;
  fprintf(plotfile,"%d %d%% %d\n",x,shade,number_items);

  xgrx = oldx+x;
  xgry = oldy+y;
  yheight = y;
  ybase = oldy; 
  return 0;
}

void
ps_box(int x, int y) 
{
    ps_flush(); 
    if (cp_tog == 0) 
     fprintf(plotfile,"%d %d mv\n",xgrx+orgx,xgry+orgy);
    if (x == 0) x = 1; 
    if (y == 0) y = 1; 
    fprintf(plotfile,"%d %d bx\n",x,y);
    xgrx += x; xgry += y;
    cp_tog = 0;
}

#ifndef SUN
void
vms_no_raster()
{
    Werrprintf("VMS programming error:  RASTER is set\n");
}
#endif 

/****************************************************************/
/*	HPGL Support - 						*/
/****************************************************************/

/****************/
void
plot_endgraphics()
/****************/
{ 
/*
   if (is_vplot_session(0) > 0)
	return;
*/
   plot = 0;
   raster = 0;
   active_gdevsw = &(gdevsw_array[C_TERMINAL]);
}

/****************/
void
plot_graf_clear() 
/****************/
{}

/*  plot_color used to set g_color.  It doesn't now.  */
/*  curpencolor is used in its place */

/***********/
void
plot_color(int c, int pen)
/***********/
{ int cc;

  (void) c;
  cc = get_pen_color(pen);
  if (cc != curpencolor)
    { fprintf(plotfile,"SP%d;\n",cc);
      curpencolor = cc;
    }
}

/**************/   
int
plot_charsize(double f)
/**************/
{
  xcharpixels = (int)((double)xcharp1*f);
  ycharpixels = (int)((double)ycharp1*f);
  fprintf(plotfile,"SI%g,%g\n",
    (double)xcharpixels/(10.0*ppmm*1.5),
    (double)ycharpixels/(10.0*ppmm*2.0));
  fprintf(plotfile,"DI1,0\n");
  revchar = 0;
  return 0;
}

int
plot_fontsize(double f)
{
  xcharpixels = (int)((double)xcharp1*f);
  ycharpixels = (int)((double)ycharp1*f);
  return 0;
}


/*************/
void
plot_amove(int x, int y)
/*************/
{ 
  xgrx=x; 
  xgry=y;  
  fprintf(plotfile,"PU%d,%d;",x+orgx,y+orgy);
}


/**************/
void
plot_rdraw(int x, int y)
/**************/
{
   xgrx += x; xgry += y;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
	fprintf(plotfile,"PD%d,%d;\n",xgrx+orgx,xgry+orgy);
}


/**************/
void
plot_adraw(int x, int y)
/**************/
{
   xgrx = x; xgry = y;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
	fprintf(plotfile,"PD%d,%d;\n",xgrx+orgx,xgry+orgy);
}


/************/
void
plot_dchar(char ch)		 
/************/
{
   if (revchar)
      fprintf(plotfile,"DI1,0\n");
   revchar = 0;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
      fprintf(plotfile,"LB%c\003\n",ch);
   xgrx += xcharpixels;
}


/**************/
void
plot_dstring(char *s)		 
/*************/
{
   if (revchar)
      fprintf(plotfile,"DI1,0\n");
   revchar = 0;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
      fprintf(plotfile,"LB%s\003\n",s);
   xgrx += xcharpixels*strlen(s);
}


/*************/
void
plot_dvchar(char ch)		 
/*************/
{
   if (!revchar)
     fprintf(plotfile,"DI0,1\n");
   revchar = 1;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
     fprintf(plotfile,"LB%c\003\n",ch);
   xgry += xcharpixels;
}

/**************/
void
plot_dvstring(char *s)		 
/**************/
{ 
   if (!revchar)
     fprintf(plotfile,"DI0,1\n");
   revchar = 1;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
     fprintf(plotfile,"LB%s\003\n",s);
   xgry += strlen(s)*xcharpixels;
}

/************/
void
plot_vchar(char ch)		 
/************/
{
   if (revchar)
      fprintf(plotfile,"DI1,0\n");
   revchar = 0;
   if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
      fprintf(plotfile,"LB%c\003\n",ch);
   xgry -= ycharpixels;
}

/************/
void
plot_vstring(char *s)		 
/************/
{
   while (*s != '\0')
   {
      plot_vchar(*s);
      s++;
   }
}

/********************************************/
void
plot_ybars(int dfpnt, int depnt, struct ybar *out, int vertical,
           int maxv, int minv)  
/********************************************/
/* draws a spectrum */
{ register int i,j,k;
  register int down,lasty;
  void range_ybar();			
	
  /* first check vertical limits */
  if (maxv)
    range_ybar(dfpnt,depnt,out,maxv,minv);
  down = 0; /* start with pen up */
  i = dfpnt;
  lasty = -1;
  while (i<depnt)
    { /* skip any blank area */
      while ((out[i].mx<out[i].mn)&&(i<depnt))
	{ i++; down = 0; /* pen up */ }
      if (i<depnt)
	{ if (out[i].mn==out[i].mx)
	    { j=i;
	      i++;
	      /* first check for min=max and find vector */
	      /* vector is less than 45 degree up or down */
	      while ((out[i].mn==out[i].mx) && (i<depnt)
		  &&(abs(out[(i+j)>>1].mn-((out[j].mn+out[i].mn)>>1))<=1))
		  i++;
	      /* long horizontal vectors give low intensity */
	      if (i>j+5) i=j+5;
	      i--;
	      if (vertical)
		{ if (down)
		  {
		    if (out[j].mn != lasty)
		      fprintf(plotfile,"PD%d,%d;\n",
			mnumxpnts-out[j].mn,j+1);
		  }
		  else
		    fprintf(plotfile,"PU%d,%d;\n",
		      mnumxpnts-out[j].mn,j+1);
		  lasty = out[i].mn;
		  fprintf(plotfile,"PD%d,%d;\n",
		      mnumxpnts-out[i].mn,i+1);
		}
	      else
		{ if (down)
		  {
		    if (out[j].mn != lasty)
                    {
		      fprintf(plotfile,"PD%d,%d;\n",j+1,out[j].mn);
                      if (j != i)
		         fprintf(plotfile,"PD%d,%d;\n",i+1,out[i].mn);
                    }
                    else
                    {
		      fprintf(plotfile,"PD%d,%d;\n",i+1,out[i].mn);
                    }
		  }
		  else
                  {
		    fprintf(plotfile,"PU%d,%d;\n",j+1,out[j].mn);
		    fprintf(plotfile,"PD%d,%d;\n",i+1,out[i].mn);
                  }
		  lasty = out[i].mn;
		}
	    }
	  else if ((i<depnt-1) && (out[i].mx==out[i+1].mn-1))
	    /* find vector more than 45 degree up */
	    { j=i;
	      k=out[j].mx-out[j].mn;
	      i++;
	      while ((out[i-1].mx==out[i].mn-1) && (i<depnt)
		  &&(out[i].mx>=out[i].mn)
		  &&(abs(out[i].mx-out[i].mn-k)<=1))
		  i++;
	      /* long horizontal vectors give low intensity */
	      if (i>j+5) i=j+5;
	      i--;
	      if (vertical)
		{ if (down)
		  {
		    if (out[j].mn-1 != lasty)
		      fprintf(plotfile,"PD%d,%d;\n",
			mnumxpnts-out[j].mn,j+1);
		  }
		  else
		    fprintf(plotfile,"PU%d,%d;\n",
		      mnumxpnts-out[j].mn,j+1);
		  lasty = out[i].mx;
		  fprintf(plotfile,"PD%d,%d;\n",
		      mnumxpnts-out[i].mx,i+1);
		}
	      else
		{ if (down)
		  {
		    if (out[j].mn-1 != lasty)
		      fprintf(plotfile,"PD%d,%d;\n",j+1,out[j].mn);
		  }
		  else
		    fprintf(plotfile,"PU%d,%d;\n",j+1,out[j].mn);
		  lasty = out[i].mx;
		  fprintf(plotfile,"PD%d,%d;\n",i+1,out[i].mx);
		}
	    }
	  else if ((i<depnt-1) && (out[i].mn==out[i+1].mx+1))
	    /* find vector more than 45 degree down */
	    { j=i;
	      k=out[j].mx-out[j].mn;
	      i++;
	      while ((out[i-1].mn==out[i].mx+1) && (i<depnt)
		  &&(out[i].mx>=out[i].mn)
		  &&(abs(out[i].mx-out[i].mn-k)<=1))
		  i++;
	      /* long horizontal vectors give low intensity */
	      if (i>j+5) i=j+5;
	      i--;
	      if (vertical)
		{ if (down)
		  {
		    if (out[j].mx+1 != lasty)
		      fprintf(plotfile,"PD%d,%d;\n",
			mnumxpnts-out[j].mx,j+1);
		  }
		  else
		    fprintf(plotfile,"PU%d,%d;\n",
		      mnumxpnts-out[j].mx,j+1);
		  lasty = out[i].mn;
		  fprintf(plotfile,"PD%d,%d;\n",
		      mnumxpnts-out[i].mn,i+1);
		}
	      else
		{ if (down)
		  {
		    if (out[j].mx+1 != lasty)
		      fprintf(plotfile,"PD%d,%d;\n",j+1,out[j].mx);
		  }
		  else
		    fprintf(plotfile,"PU%d,%d;\n",j+1,out[j].mx);
		  lasty = out[i].mn;
		  fprintf(plotfile,"PD%d,%d;\n",i+1,out[i].mn);
		}
	    }
	  else /* none of the exceptions */
	    { if (vertical)
		{ if (down)
		  {
		    if (out[i].mn == lasty)
		      lasty = out[i].mx;
		    else if (out[i].mx == lasty)
		      lasty = out[i].mn;
		    else if ((i < depnt-1) &&
		       ((out[i].mx == out[i+1].mx) ||
			(out[i].mx == out[i+1].mn)))
		    {
			lasty = out[i].mx;
			fprintf(plotfile,"PD%d,%d;",
			  mnumxpnts-out[i].mn,i+1);
		    }
		    else if ((i < depnt-1) &&
		       ((out[i].mn == out[i+1].mx) ||
			(out[i].mn == out[i+1].mn)))
		    {
			lasty = out[i].mn;
			fprintf(plotfile,"PD%d,%d;",
			  mnumxpnts-out[i].mx,i+1);
		    }
		    else
		    {
		      if (abs(lasty-out[i].mn) <= abs(lasty-out[i].mx))
		      {
			lasty = out[i].mx;
			fprintf(plotfile,"PD%d,%d;",
			  mnumxpnts-out[i].mn,i+1);
		      }
		      else
		      {
			lasty = out[i].mn;
			fprintf(plotfile,"PD%d,%d;",
			  mnumxpnts-out[i].mx,i+1);
		      }
		    }
		  }
		  else
		  {
		    fprintf(plotfile,"PU%d,%d;",
		      mnumxpnts-out[i].mn,i+1);
		    lasty = out[i].mx;
		  }
		  fprintf(plotfile,"PD%d,%d;\n",
		    mnumxpnts-lasty,i+1);
		}
	      else
		{ if (down)
		  {
		    if (out[i].mn == lasty)
		      lasty = out[i].mx;
		    else if (out[i].mx == lasty)
		      lasty = out[i].mn;
		    else if ((i < depnt-1) &&
		       ((out[i].mx == out[i+1].mx) ||
			(out[i].mx == out[i+1].mn)))
		    {
			lasty = out[i].mx;
			fprintf(plotfile,"PD%d,%d;",i+1,out[i].mn);
		    }
		    else if ((i < depnt-1) &&
		       ((out[i].mn == out[i+1].mx) ||
			(out[i].mn == out[i+1].mn)))
		    {
			lasty = out[i].mn;
			fprintf(plotfile,"PD%d,%d;",i+1,out[i].mx);
		    }
		    else
		    {
		      if (abs(lasty-out[i].mn) <= abs(lasty-out[i].mx))
		      {
			lasty = out[i].mx;
			fprintf(plotfile,"PD%d,%d;",i+1,out[i].mn);
		      }
		      else
		      {
			lasty = out[i].mn;
			fprintf(plotfile,"PD%d,%d;",i+1,out[i].mx);
		      }
		    }
		  }
		  else
		  {
		    fprintf(plotfile,"PU%d,%d;",i+1,out[i].mn);
		    lasty = out[i].mx;
		  }
		  fprintf(plotfile,"PD%d,%d;\n",i+1,lasty);
		}
	    }
	  down = 1;
	}
      i++;
    }
}

/*************************************/
int
plot_grayscale_box(int oldx, int oldy, int x, int y, int shade)
/*************************************/
{ 
  (void) shade;
  if ((x<1)||(y<1)) return 1;
  xgrx = oldx;
  xgry = oldy;
  return 1;
}


/***********/
void
plot_box(int x, int y) 
/***********/
{ 
  if ((curpencolor!=BLACK)&&(curpencolor!=BACK))
    fprintf(plotfile,"RR%d,%d;\n",x,y);
}

/*******************/
void
plot_normalmode() {}
/*******************/

/****************/
void
plot_xormode() {}
/****************/

/****************/
int
plot_raster()
/****************/
{
   return(raster);
}

FILE
*plot_file()
{
    return (plotfile);
}

char
*plot_file_name()
{
    return(plotpath);
}

void
replace_plot_file(char *path, FILE *fd)
{
    strcpy(plotpath, path);
    plotfile = fd;
}

void
set_gray_offset(double a, double b)
{
        gray_offset = a * b / 2.0;
}

int
plot_locx(int x)
{
    return (orgx + x);
}

int
plot_locy(int y)
{
    return (orgy + y);
}

void
ps_light_color() {
    // ps_rgb_color(147, 149, 151);
   if (!plotfile)
       return;
   if (maxpen > 2)
       ps_rgb_color(140.0, 140.0, 140.0);
   else {
       ps_flush();
       fprintf(plotfile,"0.5 0.5 0.5 C\n");
       psRed = 2.0;
   }
}

void
ps_dark_color() {
   if (!plotfile)
       return;
   if (maxpen > 2)
       ps_rgb_color(0.0, 0.0, 0.0);
   else {
       ps_flush();
       fprintf(plotfile,"0.0 0.0 0.0 C\n");
       psRed = 2.0;
   }
}

void
ps_info_font() {
   if (!plotfile || !init_tog)
       return;
    ps_font("Helvetica-Bold");
    ps_charsize(infoFontSize);
}

void
ps_info_light_font() {
   if (!plotfile || !init_tog)
       return;
    ps_font("Helvetica");
    ps_charsize(infoFontSize);
}

int show_ps_fonts(int argc, char *argv[], int retc, char *retv[])
{
    int i, n, x, y;

    n = sizeof(ps_font_list) / sizeof(char *);
    setplotter();
    color(BLACK);
    x = 0;
    y = mnumypnts - ycharpixels;
    for (i = 0; i < n; i++) {
         ps_font(ps_font_list[i]);
         amove(x, y);
         dstring(ps_font_list[i]);
         dstring("HIJKhijk1234");
         y = y - (ycharpixels * 1.3);
         if (y < ycharpixels) {
             y = mnumypnts - ycharpixels;
             x = x + xcharpixels * 44;
         }
    }
    RETURN;
}

static int
new_plot_page()
{
    if (plotfile == NULL || raster < 3)
         setplotter();
    if (raster < 3)
         return(0);
    ps_flush();
    pageNo++;
//    if(pageNo > pageMax)
//         return(0);
    plotHeader();
    plotLogo();
#ifdef VNMR
    if (pageNo < 3)   // page 1
        imagefile_flush();
    else
        ps_imagefile_flush();
#endif
    fprintf(plotfile,"save restore\n");
    fprintf(plotfile,"showpage\n");
    fprintf(plotfile,"%%%%Page: %d %d\n", pageNo, pageNo);

#ifdef VNMR
    imagefile_init();
#endif

    if (raster == 3)
        fprintf(plotfile,"%d %d translate\n", transX, transY);
    else
        fprintf(plotfile,"%d %d translate 90 rotate\n",transX, transY);
    fprintf(plotfile,"%6.4f %6.4f scale\n",scaleX, scaleY);
    // fprintf(plotfile,"1 setlinewidth\n");
    // pslw = 1;
    ps_linewidth(orig_pslw);
    ps_dark_color();
    cp_tog = 0;
    init_tog = 1;
    init_new_page();
    // plotHeader();
    plotLogo();
    return(1);
}

static int
next_page_column(int newPage) {
    columnIndex++;
    if (columnIndex < pageColumns) {
        infoPlotX = infoPlotX + columnWdith + infoPlotGapX;
        columnWdith = columnWdith2;
        infoPlotY = pageHeaderB - ps_info_font_height() * 2;
        return(1);
    }
    if (newPage) {
        if (new_plot_page()) {
            infoPlotY = pageHeaderB - ps_info_font_height() * 2;
            return(1);
        }
    }
    return(0);
}

static void
plot_multi_column_header(int isPeak, int isMore) {
    int x0, x, y, w;

    ps_dark_color();
    ps_info_font();
    infoPlotY -= ycharpixels;
    x0 = infoPlotX;
    w = columnWdith;
    if (layoutMode == 2 || layoutMode == 4) {
        if (tableColumns < 5) {
            x0 = infoPlotX + xcharpixels * 2;
            w = columnWdith - xcharpixels * 4;
        }
    }
    amove(x0 + xcharpixels, infoPlotY);
    if (isPeak) {
        if (isMore)
            dstring("PEAK FREQUENCIES(CONTINUED)");
        else
            dstring("PEAK FREQUENCIES");
    }
    else {
        if (isMore)
            dstring("INTEGRAL VALUES(CONTINUED)");
        else
            dstring("INTEGRAL VALUES");
    }
    ps_light_color();
    ps_info_light_font();
    y = infoPlotY - (int) (0.5 * ycharpixels);
    amove(x0, y);
    rdraw(w, 0);
    y = infoPlotY - ycharpixels * 2;
    amove(x0, y);
    rdraw(w, 0);
    y = infoPlotY - (int) (1.5 * ycharpixels);
    infoPlotY = infoPlotY - ycharpixels * 3;
    amove(columnLocx[0], y);
    dstring(tableHeaders[0]);
    for (x = 1; x < tableColumns; x++) {
        amove(columnLocx[x], y);
        ps_rstring(tableHeaders[x]);
    }
    ps_dark_color();
    ps_info_font();
}

static void
plot_multi_column_table(char *fpath, int isPeak) {
    FILE *fd;
    int  i, n, w, x0, y;
    char *data, *v;

    if (fpath == NULL || (strlen(fpath) < 1))
        return;

    ps_info_font();
    if (columnWdith < (xcharpixels * 16))
        return;
    fd = fopen(fpath, "r");
    if (fd == NULL)
        return;
    strcpy(TmpTitle, "");
    i = ycharpixels * 5;
    if (infoPlotY < i) {
        if (next_page_column(1) < 1) {
            fclose(fd);
            return;
        }
        if (psFontSize != infoFontSize)
            ps_info_font();
        infoPlotY = infoPlotY - ycharpixels;
    }
    tableColumns = 0;
    while ((data = fgets(TmpOsPlotterName, 500, fd)) != NULL) {
        while (*data == ' ' || *data == '\t')
            data++;
        if (strlen(data) < 1)
            continue;
        v = strtok(data, " \n");
        while (v != NULL && (strlen(v) > 0)) {
            if (tableHeaders[tableColumns] == NULL) {
                 tableHeaders[tableColumns] = malloc(20);
            }
            strncpy(tableHeaders[tableColumns], v, 18);
            tableColumns++;
            if (tableColumns >= MAXCOLUMNS)
                break;
            v = strtok(NULL, " \n");
        }
        if (tableColumns > 0)
            break;
    }
    if (tableColumns < 1) {
        fclose(fd);
        return;
    }
    x0 = infoPlotX;
    w = columnWdith;
    if (layoutMode == 2 || layoutMode == 4) {
        if (tableColumns < 5) {
            x0 = infoPlotX + xcharpixels * 2;
            w = columnWdith - xcharpixels * 4;
        }
    }

    if (isPeak)
        columnLocx[0] = x0 + xcharpixels * 5;
    else
        columnLocx[0] = x0 + xcharpixels * 6;
    n = tableColumns - 1;
    columnLocx[n] = x0 + w - xcharpixels;
    i = (columnLocx[n] - columnLocx[0]) / n;
    if (i < (xcharpixels * 4))
    {
        fclose(fd);
        return;
    }
    for (n = 1; n < tableColumns - 1; n++)
        columnLocx[n] = columnLocx[n-1] + i;
    n = strlen(tableHeaders[0]);
    if (n > 4)
        columnLocx[0] = x0 + xcharpixels;
    else
        columnLocx[0] = columnLocx[0] - xcharpixels * n;
    plot_multi_column_header(isPeak, 0);

    if (isPeak)
        columnLocx[0] = x0 + xcharpixels * 5;
    else
        columnLocx[0] = x0 + xcharpixels * 6;
    y = infoPlotY;

    while ((data = fgets(TmpOsPlotterName, 500, fd)) != NULL) {
        while (*data == ' ' || *data == '\t')
            data++;
        if (strlen(data) < 1)
            continue;
        if (y < 0) {
            if (next_page_column(1) < 1)
               break;
            if (psFontSize != infoFontSize)
                ps_info_font();
            x0 = infoPlotX;
            w = columnWdith;
            if (layoutMode == 2 || layoutMode == 4) {
                if (tableColumns < 5) {
                    x0 = infoPlotX + xcharpixels * 2;
                    w = columnWdith - xcharpixels * 4;
                }
            }
            infoPlotY = infoPlotY - ycharpixels;
            if (isPeak)
                columnLocx[0] = x0 + xcharpixels * 5;
            else
                columnLocx[0] = x0 + xcharpixels * 6;
            n = tableColumns - 1;
            columnLocx[n] = x0 + w - xcharpixels;
            i = (columnLocx[n] - columnLocx[0]) / n;
            for (n = 1; n < tableColumns - 1; n++)
                columnLocx[n] = columnLocx[n-1] + i;
            columnLocx[0] = x0 + xcharpixels;
            plot_multi_column_header(isPeak, 1);
            if (isPeak)
                columnLocx[0] = x0 + xcharpixels * 5;
            else
                columnLocx[0] = x0 + xcharpixels * 6;
            y = infoPlotY;
            ps_dark_color();
        }
        for (i = 0; i < tableColumns; i++) {
            if (i == 0)
                v = strtok(data, " \n");
            else
                v = strtok(NULL, " \n");
            if (v == NULL || (strlen(v) < 1))
                 break;
            amove(columnLocx[i], y);
            ps_rstring(v);
        }
        y = y - ycharpixels;
    }

    infoPlotY = y;

    fclose(fd);
}

static long
plot_2_columns(FILE *fd, int plotValue, int *lines) {
    int leftSide, length, k1, k2, s1, s2;
    int ptr, w, x, x2, y, incr, lastY, gap, rows;
    int  leftLines, isHeader, isLabel, lineLen, toDraw;
    char *data, *name, *value;

    leftLines = *lines;
    if (leftLines < 2)
        leftLines = 2;
    ptr = 0;
    leftSide = 1;
    gap = 4;
    s1 = ps_info_font_width();
    w = (columnWdith - s1 * gap) / 2;
    if (w < s1 * 6)
        return(-1);
    if (plotValue) {
        ps_info_font();
        ps_dark_color();
    }
    else {
        ps_info_light_font();
        ps_light_color();
    }
    infoPlotY -= ycharpixels;
    if (infoPlotY < ycharpixels) {
        if (next_page_column(0) < 1)
            return(1);
        if (psFontSize != infoFontSize) {
            if (plotValue)
                ps_info_font();
            else {
                ps_info_light_font();
                ps_light_color();
            }
        }
        infoPlotY -= ycharpixels;
    }
    k1 = infoPlotY / ycharpixels;
    if (k1 < 1)
        k1 = 1;
    rows = leftLines / 2 + 1;
    if (rows > k1)
        rows = k1;

    x = infoPlotX + xcharpixels;
    x2 = infoPlotX + w - xcharpixels;
    length = w / xcharpixels - 2;

    if (tableHeaders[0] == NULL)
        tableHeaders[0] = malloc(20);
    if (tableHeaders[1] == NULL)
        tableHeaders[1] = malloc(20);
    strcpy(tableHeaders[0], " ");
    strcpy(tableHeaders[1], " ");
    lastY = infoPlotY;
    while ((data = fgets(TmpOsPlotterName, 500, fd)) != NULL) {
        lineLen = strlen(data);
        name = strtok(data, " \n");
        if (name == NULL || (strlen(name) < 1)) {
            continue;
        }
        incr = 1;
        isHeader = 0;
        isLabel = 0;
        value = strtok(NULL, "\n");
        if (name != NULL) {
            if (strcmp(name, "Label:") == 0) {
                if (value != NULL && (strlen(value) > 0)) {
                    incr = 3;
                    tableHeaderNum = 0;
                    isLabel = 1;
                    if (plotValue)
                        strcpy(TmpTitle, value);
                }
                else {
                    ptr++;
                    leftLines = leftLines - 1;
                    continue;
                }
            }
            else if (strcmp(name, "Tableheader:") == 0) {
                tableHeaderNum = 0;
                if (value == NULL || (strlen(value) < 1)) {
                     continue;
                }
                strcpy(TmpPlotterType, value);
                name = strtok(TmpPlotterType, " \n");
                value = strtok(NULL, "\n");
                if (name == NULL || (strlen(name) < 1)) {
                     continue;
                }
                strncpy(tableHeaders[0], name, 10);
                if (value != NULL)
                    strncpy(tableHeaders[1], value, 10);
                else
                    strcpy(tableHeaders[1], " ");
                tableHeaderNum = 2;
                isHeader = 1;
            }
            else if (strcmp(name, "Special:") == 0) {
                 name = value;
                 value = NULL;
                 if (name == NULL) {
                     continue;
                }
            }
        }
        k1 = ptr + incr;
        if (incr > 1)
            k1++;
        y = infoPlotY - k1 * ycharpixels;
        if (k1 > rows || y < 2) {
            k2 = 0;
            if (leftSide) {
                leftSide = 0;
                ptr = 0;
                x = infoPlotX + w + xcharpixels * gap;
                x2 = x + w - xcharpixels;
                x = x + xcharpixels;
                k2 = 1;
            }
            else if (y < 2) {
                if (next_page_column(0) < 1) {
                    fseek(fd, -lineLen, SEEK_CUR);
                    if (plotValue) {
                       if (isLabel) // label will be read again, no backup
                           strcpy(TmpTitle, "");
                       *lines = leftLines;
                    }
                    return(1);
                }
                leftSide = 1;
                ptr = 0;
                infoPlotY -= ycharpixels * 2;
                x = infoPlotX + xcharpixels;
                w = (columnWdith - xcharpixels * gap) / 2;
                x2 = infoPlotX + w - xcharpixels;
                k1 = infoPlotY / ycharpixels;
                rows = leftLines / 2 + 1;
                if (rows < 2)
                    rows = 2;
                if (rows > k1)
                     rows = k1;
                lastY = infoPlotY;
                k2 = 1;
            }
            if (k2 && (tableHeaderNum > 0)) {
                y = infoPlotY - ptr * ycharpixels;
                if (!plotValue) {
                    amove(x, y);
                    dstring(tableHeaders[0]);
                    amove(x2, y);
                    ps_rstring(tableHeaders[1]);
                }
                rows++;
                ptr++;
            }
        }
        if (incr > 1) {   // label
            if (ptr > 0)
                y = infoPlotY - (ptr + 1) * ycharpixels;
            else {
                y = infoPlotY;
                incr = 2;
            }
            if (plotValue) {
                amove(x, y);
                dstring(value);
            }
            y -= ycharpixels * 0.5;
            if (!plotValue) {
                amove(x - xcharpixels, y);
                adraw(x2 + xcharpixels, y);
            }
            if (y < lastY)
               lastY = y;
        }
        else {
            y = infoPlotY - ptr * ycharpixels;
            if (!plotValue) {
                amove(x, y);
                dstring(name);
            }
            if (value != NULL) {
                s1 = strlen(name);
                s2 = strlen(value);
                if (!isHeader)
                    toDraw = plotValue;
                else {
                    if (!plotValue)
                        toDraw = 1;
                    else
                        toDraw = 0;
                }
                if ((s1+s2) > length) {
                    k1 = 0;
                    k2 = length - s1;
                    if (k2 < 1)
                        k2 = 1;
                    while (k1 < s2) {
                        if (toDraw) {
                            strncpy(TmpAttr, value + k1, k2);
                            TmpAttr[k2] = '\0';
                            amove(x2, y);
                            ps_rstring(TmpAttr);
                        }
                        incr++;
                        y = y - ycharpixels;
                        k1 += k2;
                        k2 = s2 - k1;
                        if (k2 < 1)
                            break;
                        if (k2 > (length + 2))
                            k2 = length + 2;
                    }
                }
                else if (toDraw) {
                    amove(x2, y);
                    ps_rstring(value);
                }
            }
            if (y < lastY)
               lastY = y;
        }
        ptr += incr;
        leftLines = leftLines - incr;
    }
    infoPlotY = lastY - ycharpixels;
    if (plotValue)
        *lines = leftLines;
    return(0);
}

static void
plot_2_column_table(char *fpath) {
    FILE *fd, *fout;
    char *data, *p, *v;
    char  tmpPath[20];
    int  lines, length, max;
    int  s1, s2;
    int  old_X, old_Y, old_Width;
    int  old_colIndex;
    double dv;
    long  fdPos;

    if (fpath == NULL || (strlen(fpath) < 1))
         return;

    fd = fopen(fpath, "r");
    if (fd == NULL)
        return;
    fout = NULL;
    sprintf(tmpPath, "/tmp/plotXXXXXX");
    s1 = mkstemp(tmpPath);
    if (s1 >= 0) {
        close(s1);
        fout = fopen(tmpPath, "w+");
    }
    if (fout == NULL) {
        fclose(fd);
        return;
    }

    ps_info_font();
    max = (columnWdith - xcharpixels * 4) / 2;
    length = max / xcharpixels - 2;
    lines = 0;
    max = 7;
    while ((data = fgets(TmpOsPlotterName, 500, fd)) != NULL) {
        while (*data == ' ' || *data == '\t')
            data++;
        p = NULL;
        v = NULL;
        s2 = 0;
        if (strlen(data) > 0) {
            p = strtok(data, " \n");
            if (p != NULL) {
                s1 = strlen(p);
                v = strtok(NULL, "\n");
                if (v != NULL) {
                    while (*v == ' ' || *v == '\t')
                        v++;
                    s2 = strlen(v);
                    data = v + s2 - 1;
                    while (s2 > 0) {
                        if (*data == ' ')
                            *data = '\0';
                        else
                            break;
                        s2--;
                        data--;
                    }
                    s2 = strlen(v);
                    if (s2 < 1)
                        v = NULL;
                }
            }
        }

        if (p != NULL) {
            lines++;
            s1 = strlen(p);
            if (strcmp(p, "Label:") == 0) {
                if (v != NULL) {
                    lines += 2;
                    if (s2 > max)
                        max = s2;
                }
            }
            else {
                if (p[0] == 'S' || p[0] == 'T') {
                    if (strcmp(p, "Special:") == 0)
                        s1 = 0;
                    else if (strcmp(p, "Tableheader:") == 0)
                        s1 = 0;
                }
                if (s1 > max)
                     s1 = max;
                if ((s1 + s2) > length) {
                    s2 = s1 + s2 - length;
                    while (s2 > 0) {
                        lines++;
                        s2 = s2 - length;
                    }
                }
            }
            fprintf(fout, "%s", p);
            if (v != NULL)
                fprintf(fout, " %s", v);
            fprintf(fout, "\n");
        }
    }

    if (max >= length) {
        dv = infoFontSize;
        while (dv > 0.5) {
            dv = dv * 0.9;
            s1 = (int)((double)xcharp1 * dv * ppmm/5.91);
            s2 = (columnWdith - s1 * 4) / 2;
            length = s2 / s1 - 2;
            if (length > max)
                break;
        }
        ps_charsize(dv);
    }
    strcpy(TmpTitle, "");
    tableHeaderNum = 0;

    rewind(fout);
    while (1) {
        old_X = infoPlotX;
        old_Y = infoPlotY;
        old_Width = columnWdith;
        old_colIndex = columnIndex;
        fdPos = ftell(fout);
        plot_2_columns(fout, 0, &lines);
        fseek(fout, fdPos, SEEK_SET);

        infoPlotX = old_X; 
        infoPlotY = old_Y; 
        columnWdith = old_Width; 
        columnIndex = old_colIndex; 
        if (plot_2_columns(fout, 1, &lines) < 1)
            break;
        if (new_plot_page() < 1)
            break;
        /***
        if (strlen(TmpTitle) > 0)
            lines += 2;
        ***/
    }

    fclose(fd);
    fclose(fout);
    unlink(tmpPath);
}

static void
plot_text_file(char *fpath) {
    int  y;
    char *data;
    FILE *fd;

    if (fpath == NULL || (strlen(fpath) < 1))
         return;

    fd = fopen(fpath, "r");
    if (fd == NULL)
        return;

    ps_info_font();
    ps_dark_color();
    y = infoPlotY - ycharpixels;
    if (y < ycharpixels) {
        if (next_page_column(1) < 1) {
            fclose(fd);
            return;
        }
        if (psFontSize != infoFontSize)
            ps_info_font();
        infoPlotY = infoPlotY - ycharpixels;
        y = infoPlotY;
    }
    while ((data = fgets(TmpOsPlotterName, 200, fd)) != NULL) {
         if (y < 2) {
            if (next_page_column(1) < 1)
                break;
            if (psFontSize != infoFontSize)
                ps_info_font();
            infoPlotY = infoPlotY - ycharpixels;
            y = infoPlotY;
         }
         amove(infoPlotX, y);
         dstring(data);
         y = y - ycharpixels;
    }
    infoPlotY = y;

    fclose(fd);
}


static void
miniPlot(char *cmd) {
    double oldSc, oldSc2, oldWc, oldWc2, oldVp, oldVs;
    double oldHo, oldIs, oldIo, oldVo, oldVsproj, oldVs2d, oldLvl, oldTlt;
    double oldDss_sc, oldDss_wc, oldDss_sc2, oldDss_wc2;

    double pSc, pSc2, pWc, pWc2, pVp, pVs;
    double pHo, pIs, pIo, pVo, pVsproj, pVs2d, pLvl, pTlt;
    double pDss_sc, pDss_wc, pDss_sc2, pDss_wc2;
    double newValue;
    double pLwc2maxmax, pLwcmaxmax;
    double oldWcmax, oldWc2max, oldLwc2maxmax, oldLwcmaxmax;

    int oldPshr, oldYpnts, oldXpnts;
    int miniH, chH, y;

    if (cmd == NULL || (strlen(cmd) < 1))
         return;
    setplotter();
    if (P_getreal(GLOBAL,"wc2max",&pLwc2maxmax, 1) != 0)
       return;
    if (P_getreal(GLOBAL,"wcmax",&pLwcmaxmax, 1) != 0)
       return;

    if (P_getreal(CURRENT,"wc",&pWc, 1) != 0)
       return;
    if (P_getreal(CURRENT,"wc2",&pWc2, 1) != 0)
       return;
    if (P_getreal(CURRENT,"sc",&pSc, 1) != 0)
       return;
    if (P_getreal(CURRENT,"sc2",&pSc2, 1) != 0)
       return;

    if (P_getreal(CURRENT,"vp",&pVp, 1) != 0)
       pVp = vp;
    if (P_getreal(CURRENT,"ho",&pHo, 1) != 0)
       pHo = ho;
    if (P_getreal(CURRENT,"is",&pIs, 1) != 0)
       pIs = is;
    if (P_getreal(CURRENT,"io",&pIo, 1) != 0)
       pIo = io;
    if (P_getreal(CURRENT,"lvl",&pLvl, 1) != 0)
       pLvl = lvl;
    if (P_getreal(CURRENT,"tlt",&pTlt, 1) != 0)
       pTlt = tlt;
    if (P_getreal(CURRENT,"vo",&pVo, 1) != 0)
       pVo = vo;
    if (P_getreal(CURRENT,"vsproj",&pVsproj, 1) != 0)
       pVsproj = vsproj;
    if (P_getreal(CURRENT,"vs",&pVs, 1) != 0)
       pVs = vs;
    if (P_getreal(CURRENT,"vs2d",&pVs2d, 1) != 0)
       pVs2d = vs2d;

    if (P_getreal(CURRENT,"dss_sc",&pDss_sc, 1) != 0)
       pDss_sc = 0.0;
    if (P_getreal(CURRENT,"dss_wc",&pDss_wc, 1) != 0)
       pDss_wc = pWc;
    if (P_getreal(CURRENT,"dss_sc2",&pDss_sc2, 1) != 0)
       pDss_sc2 = 0.0;
    if (P_getreal(CURRENT,"dss_wc2",&pDss_wc2, 1) != 0)
       pDss_wc2 = pWc2;

    oldSc = sc;
    oldSc2 = sc2;
    oldWc = wc;
    oldWc2 = wc2;
    oldWcmax = wcmax;
    oldWc2max = wc2max;
    oldLwc2maxmax = Lwc2maxmax;
    oldLwcmaxmax = Lwcmaxmax;
    oldXpnts = mnumxpnts;
    oldYpnts = mnumypnts;
    oldPshr = hires_ps;
    oldVp = vp;
    oldVs = vs;
    oldHo = ho;
    oldIs = is;
    oldIo = io;
    oldVo = vo;
    oldVsproj = vsproj;
    oldVs2d = vs2d;
    oldLvl = lvl;
    oldTlt = tlt;
    oldDss_sc = dss_sc;
    oldDss_wc = dss_wc;
    oldDss_sc2 = dss_sc2;
    oldDss_wc2 = dss_wc2;

    chH = ps_info_font_height();
    y = pageHeaderB / 5;
    miniH = chH * (y / chH + 1);
    y = infoPlotY - miniH;
    if (y < 0) {
        y = -y;
        if (y > (miniH / 5)) {
            if (next_page_column(1) < 1) {
                return;
            }
        }
        else
            miniH = infoPlotY - chH;
        y = infoPlotY - miniH - chH;
    }
    newValue = (double) columnWdith / ppmm;
    wc = newValue;
    wcmax = newValue;
    Lwcmaxmax = newValue;
    P_setreal(CURRENT, "wc", newValue, 1);
    P_setreal(CURRENT, "dss_wc", newValue, 1);
    P_setreal(GLOBAL, "wcmax", newValue, 1);
    sc = 0;
    P_setreal(CURRENT, "sc", 0, 1);
    P_setreal(CURRENT, "dss_sc", 0, 1);

    newValue = (double) miniH / ppmm;
    wc2 = newValue;
    wc2max = newValue;
    Lwc2maxmax = newValue;
    P_setreal(CURRENT, "wc2", newValue, 1);
    P_setreal(CURRENT, "dss_wc2", newValue, 1);
    P_setreal(GLOBAL, "wc2max", newValue, 1);
    sc2 = 0;
    P_setreal(CURRENT, "sc2", 0, 1);
    P_setreal(CURRENT, "dss_sc2", 0, 1);
    vp = wc2max * (pVp / oldWc2max);
    P_setreal(CURRENT, "vp", vp, 1);
    vs = pVs / 5.0;
    if (vs < 1.0)
        vs = 1.0;
    P_setreal(CURRENT, "vs", vs, 1);

    if (hires_ps > 0) {
        newValue = 0.0;
        P_setreal(GLOBAL, "pshr", newValue, 1);
        hires_ps = 0;
    }

    ps_flush();
    ps_info_font();
    ps_charsize(infoFontSize * 0.6);
    inMiniPlot = 1;

    fprintf(plotfile,"gsave\n");
    fprintf(plotfile,"%d %d translate\n",infoPlotX, y);
    fprintf(plotfile,"1.0 1.0 1.0 C\n");
    fprintf(plotfile,"newpath 0 0 mv\n");
    fprintf(plotfile,"0 %d lineto\n", miniH + 2);
    fprintf(plotfile,"%d %d lineto\n", columnWdith + 2, miniH + 2);
    fprintf(plotfile,"%d %d lineto\n", columnWdith + 2, 0);
    fprintf(plotfile,"closepath clip\n");
    fprintf(plotfile,"sk\n");
    fprintf(plotfile,"0.0 0.0 0.0 C\n");
    psRed = 2.0;

    sprintf(TmpOsPlotterName, "%s\n", cmd);
    execString(TmpOsPlotterName);

    inMiniPlot = 0;
    if (plotfile != NULL)
        fprintf(plotfile,"grestore\n");

    sc = oldSc;
    sc2 = oldSc2;
    wc = oldWc;
    wc2 = oldWc2;
    wcmax = oldWcmax;
    wc2max = oldWc2max;
    Lwc2maxmax = oldLwc2maxmax;
    Lwcmaxmax = oldLwcmaxmax;
    mnumxpnts = oldXpnts;
    mnumypnts = oldYpnts;
    vp = oldVp;
    vs = oldVs;
    ho = oldHo;
    is = oldIs;
    io = oldIo;
    vo = oldVo;
    vsproj = oldVsproj;
    vs2d = oldVs2d;
    lvl = oldLvl;
    tlt = oldTlt;
    dss_sc = oldDss_sc;
    dss_wc = oldDss_wc;
    dss_sc2 = oldDss_sc2;
    dss_wc2 = oldDss_wc2;

    P_setreal(GLOBAL, "wc2max", pLwc2maxmax, 1);
    P_setreal(GLOBAL, "wcmax", pLwcmaxmax, 1);
    P_setreal(CURRENT, "wc", pWc, 1);
    P_setreal(CURRENT, "wc2", pWc2, 1);
    P_setreal(CURRENT, "sc", pSc, 1);
    P_setreal(CURRENT, "sc2", pSc2, 1);
    P_setreal(CURRENT, "dss_sc", pDss_sc, 1);
    P_setreal(CURRENT, "dss_wc", pDss_wc, 1);
    P_setreal(CURRENT, "dss_sc2", pDss_sc2, 1);
    P_setreal(CURRENT, "dss_wc2", pDss_wc2, 1);
    P_setreal(CURRENT, "vp", pVp, 1);
    P_setreal(CURRENT, "ho", pHo, 1);
    P_setreal(CURRENT, "is", pIs, 1);
    P_setreal(CURRENT, "io", pIo, 1);
    P_setreal(CURRENT, "lvl", pLvl, 1);
    P_setreal(CURRENT, "tlt", pTlt, 1);
    P_setreal(CURRENT, "vo", pVo, 1);
    P_setreal(CURRENT, "vsproj", pVsproj, 1);
    P_setreal(CURRENT, "vs", pVs, 1);
    P_setreal(CURRENT, "vs2d", pVs2d, 1);

    if (oldPshr > 0) {
        newValue = (double) oldPshr;
        P_setreal(GLOBAL, "pshr", newValue, 1);
        hires_ps = oldPshr;
    }

    if (plotDebug) {
         setplotter();
         y = y -4;
         ps_rgb_color(255, 0, 0);
         amove(infoPlotX, y);
         rdraw(columnWdith, 0);
         amove(infoPlotX, infoPlotY + 2);
         rdraw(columnWdith, 0);
         amove(infoPlotX, y);
         rdraw(0, miniH + 4);
         amove(infoPlotX + columnWdith, y);
         rdraw(0, miniH + 2);
    }
    ps_flush();
    infoPlotY = y - chH;
    psRed = 2.0;
}

int setpage(int argc, char *argv[], int retc, char *retv[])
{
    if (argc < 2)
        RETURN;

    if (strcmp(argv[1], "newpage") == 0) {
        new_plot_page();
        ps_default_font_color();
        RETURN;
    }
    if (strcmp(argv[1], "debug") == 0) {
        plotDebug = 1;
        RETURN;
    }
    if (strcmp(argv[1], "logo") == 0) {
        if (logoAssigned == 0)
            logoPath[0] = '\0';
        logoAssigned = 1;
        if (argc > 2)
        {
            strcpy(logoPath, argv[2]);
            if (argc > 4)
            {
                logoPos = 1;
                logoXpos = atof(argv[3]);
                logoYpos = atof(argv[4]);
                if (argc > 6)
                {
                   logoSize = 1;
                   logoWidth = atof(argv[5]);
                   logoHeight = atof(argv[6]);
                }
            }
        }
        RETURN;
    }
    if (strcmp(argv[1], "header") == 0) {
        if (argc > 2)
            dupHeaderFile(argv[2]);
        plotHeader();
        // ps_default_font_color();
        RETURN;
    }
    if (strcmp(argv[1], "layout") == 0) {
        layoutMode = 4;
        if (argc > 2)
           layoutMode = atoi(argv[2]);
        RETURN;
    }
    if (argc < 3)
        RETURN;
    if (plotfile == NULL || raster < 3) {
        setplotter();
        if (raster < 3)
            RETURN;
    }
    if (columnIndex < 0)
        init_new_page();
    if (strcmp(argv[1], "miniplot") == 0) {
        ps_linewidth(1);
        miniPlot(argv[2]);
        inMiniPlot = 0;
        ps_default_font_color();
        ps_linewidth(orig_pslw);
        RETURN;
    }
    if (strcmp(argv[1], "parameter") == 0) {
        ps_linewidth(1);
        plot_2_column_table(argv[2]);
        ps_default_font_color();
        ps_linewidth(orig_pslw);
        RETURN;
    }
    if (strcmp(argv[1], "integral") == 0) {
        ps_linewidth(1);
        plot_multi_column_table(argv[2], 0);
        ps_default_font_color();
        ps_linewidth(orig_pslw);
        RETURN;
    }
    if (strcmp(argv[1], "peak") == 0) {
        ps_linewidth(1);
        plot_multi_column_table(argv[2], 1);
        ps_default_font_color();
        ps_linewidth(orig_pslw);
        RETURN;
    }
    if (strcmp(argv[1], "footer") == 0) {
        RETURN;
    }
    if (strcmp(argv[1], "text") == 0) {
        ps_linewidth(1);
        plot_text_file(argv[2]);
        ps_default_font_color();
        ps_linewidth(orig_pslw);
        RETURN;
    }
    RETURN;
}

