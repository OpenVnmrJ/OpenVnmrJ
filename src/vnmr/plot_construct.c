/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/socket.h>

#ifdef __INTERIX
#include <arpa/inet.h>
#else
#ifdef SOLARIS
#include <sys/types.h>
#endif
#include <netinet/in.h>
#ifdef SOLARIS
#include <inttypes.h>
#endif
#endif

//#define DEBUG_JPLOT

#include <netdb.h>
#include <unistd.h>
#include "group.h"
#include "pvars.h"
#include "wjunk.h"
#include "vnmrsys.h"
#include "variables.h"
#include "graphics.h"
#include "sockets.h"
#ifndef VNMRJ
#include <errno.h>
#endif

#ifdef VNMRJ
extern char  VnmrJHostName[];
#endif
extern char     *get_cwd();
extern int  setplotter();
extern void set_ploter_ppmm(double p);
extern void ps_set_linewidth(int w);
extern int get_plot_planes();
extern int loadPlotInfo(char *pltype);
extern void disp_plot(char *t);
extern void imagefile_flush();
extern void ps_flush();
extern void set_ploter_coordinate(int x, int y);
extern void jplot_charsize(double f);
extern void register_child_exit_func(int signum, int pid,
             void (*func_exit)(), void (*func_kill)() );
extern void image_convert(char *options, char *fmt1, char *file1, char *fmt2, char *file2);

extern char     PlotterType[];
extern char     PlotterHost[];
extern char     LplotterPort[];
extern char     plotpath[];
extern int	plot;
extern int	in_plot_mode;
extern int	init_tog;
extern int	raster;
extern int	ymin;
extern int	ymultiplier;
extern int	raster_resolution;
extern int	mnumxpnts, mnumypnts;
extern double	wc, wc2, wc2max, wcmax;
extern double	vs;

extern FILE     *plotfile;
static FILE     *org_plotfile;


static int  vplot_session = 0;
static int  jportFd = -1;
static int  jportNum = -1;
static char data[512];

static int  plot_pid = -1;
static int  debug = 0;
static int  org_raster = 0;
static int  my_raster = 0;
static int  org_ymin;
static int  org_ymultiplier;
static int  org_resolution = 300;
static int  region_cmd = 0;
static int  preview = 0;
static int  view_text = 0;
static int  do_page = 0;
static int  from_jdesign = 0;
static int  from_jplot = 0;
static int  jfinished = 0;
static int  need_ret = 0;
static int  set_exit_func = 1;
static int  color_plot = 1;
static int  line_width = 1;
static int  org_xpnts, org_ypnts;
static int  win_width, win_height;
static int  xsize, ysize;
static int  myxpnts, myypnts;
static int  mycharx, mychary;
static int  textH = 0;
static int  save_data = 0;
static int  save_resolution = 300;
static int  save_format = 0;
static int  out_format = 0;
static int  out_dpi = 0;
static int  isDicom = 0;
static char out_file[MAXPATHL];
static int  plot_rx, plot_ry;
static int  do_unscale = 0;
static char plot_template[MAXPATHL];
static char org_plotpath[MAXPATHL];
static char plot_out[MAXPATHL];
static char save_out[MAXPATHL];
#ifndef VNMRJ
static char LocalHost[128];
#endif
static double paper_w = 7.5;
static double paper_h = 10.0;
static double screen_res = 90.0;
static double myppmm, org_ppmm;
static double ps_ppmm;
static double org_wcmax, org_wc2max;
static double my_wcmax, my_wc2max;
static double org_wc = 0.0;
static double org_wc2 = 0.0;
static char *dformats[] =
 { "AVS", "BMP", "BMP24", "DCX", "DIB", "EPI", "EPS", "EPS2", "EPSF",
   "EPSI", "EPT", "FAX", "FITS", "GIF", "GIF87", "HTML", "JPEG", "MAP",
   "MATTE", "MIFF", "MPEG", "MTV", "M2V", "NETSCAPE", "PBM", "PCD", "PCDS",
   "PCL", "PCX", "PDF", "PGM", "PICT", "PNG", "PNM", "PPM", "PS",
   "PSD", "PS2", "RAD", "SGI", "SUN", "TGA", "TIFF", "VIFF", "XBM",
   "XPM", "XWD", "AVI" };

#define GIF	14
#define PCL	28
#define PDF	30
#define PGM	31
#define PS	36
#define PS2	38
#define dformats_num	48

#define SIZE	1
#define START	2
#define PSTART	3
#define END	4
#define PEND	5
#define COLOR	6
#define LINEW	7
#define FONT	8
#define WSIZE	9
#define DTEXT	10	
#define DLINE	11	
#define DSQR	12	
#define DARROW	13	

static char *plot_funcs[] =
	{ "size", "start", "pstart", "end", "pend", "color", "linewidth",
	  "font", "window", "text", "line", "square", "garrow" };

#define MACRO	1
#define SETUP	2
#define REGION	3
#define DRAW	4
#define PREVIEW	5
#define PLOTTER	6
#define PSAVE	7
#define PSCREEN	8
#define PFORMAT	9
#define PPAGE	10
#define PPROC	11
#define PCLOSE	12
#define PEXIT	13
#define DEBUG	14	
#define JPORT	15
#define VTEXT	16
#define JTEXT	17
#define JDPI	18
#define JFILE	19
#define LASTCMD	JFILE

static char *plot_cmds[] =
	{ "-macro", "-setup", "-region", "-draw", "-preview", "-plotter",
	  "-save", "-screen", "-format", "-page", "-proc", "-close",
	  "-exit", "-debug", "-port", "-viewtext", "-jtext", "-dpi",
	  "-file" };

extern double get_ploter_ppmm();
extern void systemPlot(char *filename, int retc, char *retv[]);

static int
get_plot_cmd(char *cmd)
{
	int	ret;

	ret = 0;
	for (ret = 0; ret < LASTCMD; ret++)
	{
	    if (strcmp(cmd, plot_cmds[ret]) == 0)
		return(ret+1);
	}
	return (0);
}

static int
get_plot_func(char *cmd)
{
	int	ret;

	ret = 0;
	for (ret = 0; ret < 13; ret++)
	{
	    if (strcmp(cmd, plot_funcs[ret]) == 0)
		return(ret+1);
	}
	return (0);
}

static void
un_scale()
{
	if (debug)
	     fprintf(stderr, " un_scale: %d \n", do_unscale);
	if (do_unscale == 0)
	     return;
	execString("jplotunscale\n");
	do_unscale = 0;
	sprintf(data, "wc=%g  wc2=%g \n", org_wc, org_wc2);
	execString(data);
}

static void
reset_2_orginal()
{
	if (debug)
	    fprintf(stderr, "  reset_2_orginal... ");
	if (org_wc2max <= 0.0 || org_wcmax <= 0.0)
	    return;
	if (debug)
	    fprintf(stderr, "  reset_2_orginal... \n");
	P_setreal(GLOBAL,"wc2max", org_wc2max, 1);
	P_setreal(GLOBAL,"wcmax", org_wcmax, 1);
/*
	P_setreal(CURRENT,"wc", org_wc, 1);
	P_setreal(CURRENT,"wc2", org_wc2, 1);
	P_setreal(CURRENT,"vs", org_vs, 1);
	wc2 = org_wc2;
	wc = org_wc;
	vs = org_vs;
*/
	wc2max = org_wc2max;
	wcmax = org_wcmax;
	ppmm = org_ppmm;
	raster = org_raster;
	line_width = 1;
	ps_set_linewidth(line_width);
	vplot_session = 0;
	loadPlotInfo(PlotterType);
	un_scale();
	if (debug)
	    fprintf(stderr, " reset  wc: %g  wc2: %g \n", wc, wc2);
}

static void
init_params()
{
	region_cmd = 0;
	preview = 0;
	save_data = 0;
	do_page = 0;
	view_text = 0;
	save_resolution = 0;
}

static int
set_v_plotter(char *pname)
{
	if (P_getreal(GLOBAL,"wc2max" ,&org_wc2max, 1))
	    org_wc2max = 155;
	if (P_getreal(GLOBAL,"wcmax" ,&org_wcmax, 1))
	    org_wcmax = 250;
	if (P_getreal(CURRENT,"wc" ,&org_wc, 1))
	    org_wc = wcmax;
	if (P_getreal(CURRENT,"wc2" ,&org_wc2, 1))
	    org_wc2 = wc2max;
	if (debug) {
	    fprintf(stderr, " org wc: %g  wcmax %g  wc2: %g  wc2max %g \n", org_wc, org_wcmax, org_wc2, org_wc2max);
	}
	if(strcmp(PlotterType, "DICOM") == 0)
	    isDicom = 1;
	else
	    isDicom = 0;
	org_ymin = ymin;
	do_unscale = 0;
	vplot_session = 1;
	setplotter();
/*
	if (P_getreal(CURRENT,"vs" ,&org_vs, 1))
	     org_vs = vs;
*/
 	org_raster = raster;
	org_ppmm = ppmm;
 	org_resolution = raster_resolution;
	if (org_raster <= 0)
	    org_raster = 1;
	if (org_resolution <= 0)
	    org_resolution = 150;
	if (!loadPlotInfo(pname))
	{
	    loadPlotInfo(PlotterType);
	    return(0);
	}
	ps_ppmm = (double) get_ploter_ppmm();
	org_plotfile = plotfile;
	strcpy(org_plotpath, plotpath);
	if (need_ret)
	   vplot_session = 2;
	else
	   vplot_session = 1;
	init_tog = 0;
	if (plotfile != NULL)
	{
	   fclose(plotfile);
	   unlink(plotpath);
	}
	plotfile = NULL;
	if (!preview) {
	   if(out_dpi != 0) {
	        myppmm = out_dpi / 25.4;
		set_ploter_ppmm(myppmm);
	    }
	    else if (save_resolution > 0) {
	        myppmm = save_resolution / 25.4;
		set_ploter_ppmm(myppmm);
	    }
	    else if (isDicom) {
		set_ploter_ppmm(org_ppmm);
	    }
	}
	setplotter();
	if (plotfile == NULL || plot <= 0)
	{
	   reset_2_orginal();
	   return(0);
	}
	my_raster = raster;
	myppmm = ppmm;

	releasevarlist();
	return(1);
}


static void
plot_start_func(int argc, char **argv)
{
	double  xpnt, ypnt, my, mx;
	int	k;

	vplot_session = 3;
	ps_flush();

	org_ymin = ymin;
	org_xpnts = mnumxpnts;
	org_ypnts = mnumypnts;
	org_ymultiplier = ymultiplier;
	mycharx = xcharpixels;
	mychary = ycharpixels;
	win_width = 100;
	win_height = 100;
	if (debug)
	   fprintf(stderr, " region: %s  %s  %s  %s\n", argv[3], argv[4], argv[5], argv[6]);
	mx = atof(argv[5]);
	my = atof(argv[6]);
	if (mx <= 0.0)
	   mx = 0.5;
	if (my <= 0.0)
	   my = 0.5;
	if (preview)
	{
	   xsize = (int) (paper_w * screen_res * mx);
	   ysize = (int) (paper_h * screen_res * my);
	}
	else
	{
	   xsize = (int) (paper_w * 72.0 * mx);
	   ysize = (int) (paper_h * 72.0 * my);
	}
	xpnt = paper_w * 25.4 * myppmm;
	ypnt = paper_h * 25.4 * myppmm;
	plot_rx = (int) (xpnt * atof(argv[3]));
	plot_ry = (int) (ypnt * atof(argv[4]));
	if (debug)
	   fprintf(stderr, "  paper wh: %g %g  %g %g \n", paper_w, paper_h, xpnt, ypnt);
	mnumxpnts = (int) (xpnt * mx);
	mnumypnts = (int) (ypnt * my);
	wc2max = (double) mnumypnts / myppmm - (double) BASEOFFSET;
	wcmax = (double) mnumxpnts / myppmm;
/*
	wc2 = org_wc2 * (wc2max / org_wc2max);
	wc = org_wc * (wcmax / org_wcmax);
	vs = org_vs * (wc2max / org_wc2max);
*/
	P_setreal(GLOBAL,"wc2max", wc2max, 1);
	P_setreal(GLOBAL,"wcmax", wcmax, 1);
	mx = wcmax / org_wcmax;
	my = wc2max / org_wc2max;
	k = (int) (mx * 1000.0);
	if (k <= 0)
	    k = 1;
	mx = (double)k / 1000.0;
	k = (int) (my * 1000.0);
	if (k <= 0)
	    k = 1;
	my = (double)k / 1000.0;
	P_setreal(GLOBAL,"plotscalex", mx, 1);
	P_setreal(GLOBAL,"plotscaley", my, 1);
	mnumypnts = (int)((wc2max+BASEOFFSET) * myppmm); 
	myxpnts = mnumxpnts;
	myypnts = mnumypnts;
	my_wc2max = wc2max;
	my_wcmax = wcmax;
	ppmm = myppmm;
	if (preview)
	   fprintf(plotfile,"%%%%BoundingBox:  0  0 %4d %4d\n", xsize, ysize);
               
	in_plot_mode = 1;
	plot = 1;
	if (preview)
	{
/*
	    if (raster == 3)
		fprintf(plotfile,"0 0 translate\n");
	    else
		fprintf(plotfile,"0 0 translate 90 rotate\n");
*/
	}
	else
	{
	    if (my_raster == 3)
	    {
		if ((org_raster == 1) || (org_raster == 2))
		    plot_ry = (int)ypnt - mnumypnts - plot_ry - (int)myppmm * 8;
		else
		    plot_ry = (int)ypnt - mnumypnts - plot_ry;
	        set_ploter_coordinate(plot_rx, plot_ry );
	    }
	    else
	    {
		if ((org_raster == 1) || (org_raster == 2))
		{
		    plot_rx = plot_rx - (int) myppmm * 8;
		    plot_ry = (int)ypnt - mnumypnts - plot_ry - (int) myppmm * 22;
		}
		else
		{
		    plot_rx = plot_rx - (int) myppmm * 2;
		    plot_ry = (int)ypnt - mnumypnts - plot_ry - (int) myppmm * 20;
		}
	        set_ploter_coordinate(plot_rx, plot_ry );
	    }
	}
	if (debug)
	{
	    fprintf(stderr, "   ppmm: %g -> %g\n", ppmm, myppmm);
	    fprintf(stderr, "   xywh:  %d %d  %d %d \n", plot_rx, plot_ry, mnumxpnts, mnumypnts);
	    fprintf(stderr, " new wcmax %g wc %g  wc2max %g wc2 %g \n", wcmax, wc, wc2max, wc2 );
	}
	if (region_cmd)
	    active_gdevsw = &(gdevsw_array[C_TERMINAL]);
	else
	    active_gdevsw = &(gdevsw_array[C_PSPLOT]);

	wc = org_wc * mx;
	if (wc < 5.0)
	    wc = 5.0;
	wc2 = org_wc2 * my;
	if (wc2 < 5.0)
	    wc2 = 5.0;
	P_setreal(CURRENT,"wc", wc, 1);
	P_setreal(CURRENT,"wc2", wc2, 1);
	execString("jplotscale\n");
	do_unscale = 1;
}

static void
plot_end_func(int argc, char **argv)
{
	if (!preview && debug)
	{
	    active_gdevsw = &(gdevsw_array[C_PSPLOT]);
	    amove(0,0);
	    rdraw(0, mnumypnts - 2);
	    rdraw(mnumxpnts - 2, 0);
	    rdraw(0, 2 - mnumypnts);
	    rdraw(2 - mnumxpnts, 0);
	}
	ps_flush();
        P_setreal(GLOBAL,"wc2max", org_wc2max, 1);
        P_setreal(GLOBAL,"wcmax", org_wcmax, 1);
	mnumxpnts = org_xpnts;
	mnumypnts = org_ypnts;
	ymultiplier = org_ymultiplier;
	wc2max = org_wc2max;
	wcmax = org_wcmax;
	un_scale();
}

static void
writeToJProc(char *str)
{
        int k;
	char data[512];

	if (jportFd < 0)
	   return;
	sprintf(data, "%s\n", str);
#ifdef DEBUG_JPLOT
		Winfoprintf("writeToJProc: %s",str);
#endif
        k = write(jportFd, data, strlen(data));
}

static void
preview_page()
{
	struct stat     f_stat;

	if (plotfile == NULL)
	{
	   if (need_ret)
              writeToJProc("null");
	   return;
	}
#ifdef VNMRJ
	imagefile_flush();
#endif
	fprintf(plotfile, "showpage grestore\n%%%%Trailer\n%%%%EOF\n");
	fclose(plotfile);
	plotfile = 0;
	plot = 0;
	init_tog = 0;
        active_gdevsw = &(gdevsw_array[C_TERMINAL]);
        disp_plot("    "); /* clear PLOT on status screen */

       /* modified for new gs */
/*
	sprintf(data, "vnmr_gs -sDEVICE=ppmraw -preview -gifFile %s -sOutputFile=%s.ppm -r%gx%g -g%dx%d -q - %s", plot_out, plotpath, screen_res, screen_res, xsize, ysize, plotpath);
*/
	if (!view_text) {
	    sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=png256 -sOutputFile=%s.png -r%gx%g -g%dx%d -q %s", plotpath, screen_res, screen_res, xsize, ysize, plotpath);
	    system(data);
		sprintf(data, "%s.png", plotpath);
		image_convert("-transparent white", "PNG", data, "GIF", plot_out);
	}
	if (stat(plotpath, &f_stat) == 0)
	{
	   unlink(plotpath);
	}
	sprintf(data, "%s.png", plotpath);
	if (stat(data, &f_stat) == 0)
	   unlink(data);
	if (need_ret && jportFd > 0)
	{
	   if (stat(plot_out, &f_stat) == 0)
              writeToJProc(plot_out);
	   else
              writeToJProc("null");
	}
}


static void
plot_page()
{
	int isPcl, dpi, port;

	if (plotfile == NULL)
	{
	   reset_2_orginal();
	   return;
	}
	ps_flush();
#ifdef VNMRJ
	imagefile_flush();
#endif
	fprintf(plotfile, "showpage grestore\n%%%%Trailer\n%%%%EOF\n");
	fclose(plotfile);
	plotfile = 0;
	plot = 0;
	init_tog = 0;
	isDicom = 0;
        active_gdevsw = &(gdevsw_array[C_TERMINAL]);
        disp_plot("    "); /* clear PLOT on status screen */
	dpi = org_ppmm * 25.4;
	if ((org_raster == 1) || (org_raster == 2))
	   isPcl = 1;
	else {
	   isPcl = 0;
	   if(strcmp(PlotterType, "DICOM") == 0)
	      isDicom = 1;
	}
	if (isPcl) {
	  if (color_plot)
	   sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=cljet5c -sOutputFile=%spcl -r%d -q %s", plotpath, org_resolution, plotpath);
	  else
	   sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=ljet4 -sOutputFile=%spcl -r%d -q %s", plotpath, org_resolution, plotpath);

	  system(data);

	  sprintf(data, "%spcl", plotpath);
	  unlink(plotpath);
	  strcpy(plotpath, data);
	}
	if (isDicom) {
	  port = atoi(LplotterPort);
	  if (port <= 0)
		port = 104;
	  sprintf(data, "%s/bin/createdicom -infile %s -tag %s/user_templates/plot/dicom.default -dpi %d -outfile %s.dcm", systemdir, plotpath, systemdir, dpi, plotpath );
	  system(data);
	  sprintf(data, "%s.dcm", plotpath);
	  if (! access(data, R_OK)) {
	     sprintf(data, "%s/bin/dicomlpr -file %s.dcm -port %d -host %s", systemdir, plotpath, port, PlotterHost);
	      system(data);
	  }
	  unlink(plotpath);
	  sprintf(data, "%s.dcm", plotpath);
	  if (! access(data, R_OK)) {
	      unlink(data);
	  }
	}
	else
	   systemPlot("",0,NULL);
	reset_2_orginal();
}

static void
save_plot_data()
{
	int	to_ppm;
	char	format[12];

	if (plotfile == NULL)
	{
	   vplot_session = 0;
	   return;
	}
	ps_flush();
#ifdef VNMRJ
	imagefile_flush();
#endif
	fprintf(plotfile, "showpage grestore\n%%%%Trailer\n%%%%EOF\n");
	fclose(plotfile);
	plotfile = 0;
	plot = 0;
	init_tog = 0;
	to_ppm = 0;
        active_gdevsw = &(gdevsw_array[C_TERMINAL]);
        disp_plot("    "); /* clear PLOT on status screen */
	if (save_format <= 0)
	   return;
	switch (save_format) {
	  case GIF: 
		 sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=png256 -sOutputFile=%s.png -r%d  -q %s", plotpath, save_resolution, plotpath);
		 system(data);
		 sprintf(data, "%s.png", plotpath);
		 image_convert("-transparent white", "PNG", data, "GIF", save_out);
		 unlink(plotpath);
		 unlink(data);
		 return;
		 break;
	  case PGM:
		 sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=pgmraw -sOutputFile=%s -r%d  -q %s", save_out, save_resolution, plotpath);
		 break;
	  case PS:
		 sprintf(data, "mv %s %s", plotpath, save_out);
		 system(data);
		 return;
		 break;
	  case PCL:
		 if (color_plot)
		    sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=cljet5c -sOutputFile=%s -r%d -q %s", save_out, save_resolution, plotpath);
		 else
		    sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=ljet4 -sOutputFile=%s -r%d -q %s", save_out, save_resolution, plotpath);
		 break;
	  case PDF:
                 if (access ("/usr/bin/ps2pdf", X_OK) == 0)
		     sprintf(data, "/usr/bin/ps2pdf %s %s", plotpath, save_out);
                 else
		     sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile=%s -r72 -q %s", save_out, plotpath);
		 break;
	  default:
		 to_ppm = 1;
		 strcpy(format, dformats[save_format-1]);
		 break;
	}
	if (to_ppm)
	{
	    sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=png16m -sOutputFile=%s.png -r%d  -q %s", plotpath, save_resolution, plotpath);
	    system(data);
	    sprintf(data, "%s.png", plotpath);
		image_convert("", "PNG", data, format, save_out);
	    unlink(plotpath);
	    unlink(data);
	}
	else
	{
	    system(data);
	    unlink(plotpath);
	}
}

static void
plot_macro_func(int argc, char **argv)
{
	int	cmd;
	char	*retStr;
	char	macro[MAXPATHL];

	if (jportFd < 0)
	    return;
	cmd = atoi(argv[2]);
	/* now, there is only one cmd */
	switch (cmd) {
	  case  1:
		if (argc < 8)
		{
              	    writeToJProc("null");
		    return;
		}
		need_ret = atoi(argv[7]);
		strcpy(plot_out, argv[5]);
		retStr = rindex(argv[4], '/');
		if (retStr != NULL)
		    strcpy(macro, retStr+1);
		else
		    strcpy(macro, argv[4]);
		save_out[0] = '\0';
		jfinished = 0;
		init_params();
		sprintf(data,"macrold('%s'):$dum %s\n",argv[4], macro);
		execString(data);
		sprintf(data,"purge('%s')\n", macro);
		execString(data);
		if (debug == 0)
		    unlink (argv[4]);
		ps_set_linewidth(1);
		if (preview || need_ret) /* preview failed */
		{
              	    writeToJProc("null");
		}
		reset_2_orginal();
		break;
	}
}

#ifdef OLD
static void
draw_arrow(int argc, char **argv)
{
	int	type, x, y, w, h, x1, y1, wa, wb;

	type = atoi(argv[3]);
	if (type < 1 || type > 8)
	    return;
	x = (int) (mnumxpnts * atof(argv[4]));
	y = (int) (mnumypnts * (1.0 - atof(argv[5])));
	if (mnumxpnts > mnumypnts)
	   w = mnumxpnts * 0.02;
	else
	   w = mnumypnts * 0.02;
	h = w;
	wa = (int) (w * 0.6);
	wb = (int) (w * 0.2);

	ps_flush();
	fprintf(plotfile,"newpath\n");
	switch (type) {
	  case 1: /* arrow up */
		 w = w / 2;
		 y1 = y + h;
		 x1 = x + w / 2;
		 ps_flush();
		 amove (x1, y1);
		 fprintf(plotfile,"%d %d rlineto\n", -wb, -wa);
		 fprintf(plotfile,"%d %d rlineto\n", wb * 2, 0);
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x1, y);
		 rdraw(0, h);
		 break;
	  case 2: /* arrow down */
		 w = w / 2;
		 y1 = y;
		 x1 = x + w / 2;
		 amove (x1, y1);
		 fprintf(plotfile,"%d %d rlineto\n", -wb, wa);
		 fprintf(plotfile,"%d %d rlineto\n", wb * 2, 0);
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x1, y);
		 rdraw(0, h);
		 break;
	  case 3: /* arrow left */
		 h = h / 2;
		 y1 = y + h / 2;
		 amove (x, y1);
		 fprintf(plotfile,"%d %d rlineto\n", wa, -wb);
		 fprintf(plotfile,"%d %d rlineto\n", 0, wb * 2);
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x, y1);
		 rdraw(w, 0);
		 break;
	  case 4: /* arrow right */
		 h = h / 2;
		 y1 = y + h / 2;
		 x1 = x + w;
		 amove (x1, y1);
		 fprintf(plotfile,"%d %d rlineto\n", -wa, wb);
		 fprintf(plotfile,"%d %d rlineto\n", 0, -(wb * 2));
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x, y1);
		 rdraw(w, 0);
		 break;
	  case 5: /* arrow right down */
		 w = (int) (w * 0.7);
		 h = (int) (h * 0.7);
		 wa = (int) (w * 0.7);
		 wb = (int) (w * 0.3);
		 x1 = x + w;
		 amove (x1, y);
		 fprintf(plotfile,"%d %d rlineto\n", -wb, wa);
		 fprintf(plotfile,"%d %d rlineto\n", -(wa - wb), -(wa-wb));
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x1, y);
		 rdraw(-w, h);
		 break;
	  case 6: /* arrow left down */
		 w = (int) (w * 0.7);
		 h = (int) (h * 0.7);
		 wa = (int) (w * 0.7);
		 wb = (int) (w * 0.3);
		 x1 = x + w;
		 amove (x, y);
		 fprintf(plotfile,"%d %d rlineto\n", wb, wa);
		 fprintf(plotfile,"%d %d rlineto\n", wa - wb, -(wa-wb));
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x, y);
		 rdraw(w, h);
		 break;
	  case 7: /* arrow right up */
		 w = (int) (w * 0.7);
		 h = (int) (h * 0.7);
		 wa = (int) (w * 0.7);
		 wb = (int) (w * 0.3);
		 x1 = x + w;
		 y1 = y + h;
		 amove (x1, y1);
		 fprintf(plotfile,"%d %d rlineto\n", -wa, -wb);
		 fprintf(plotfile,"%d %d rlineto\n", wa - wb, -(wa-wb));
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x, y);
		 rdraw(w, h);
		 break;
	  case 8: /* arrow left up */
		 w = (int) (w * 0.7);
		 h = (int) (h * 0.7);
		 wa = (int) (w * 0.7);
		 wb = (int) (w * 0.3);
		 x1 = x;
		 y1 = y + h;
		 amove (x, y1);
		 fprintf(plotfile,"%d %d rlineto\n", wb, -wa);
		 fprintf(plotfile,"%d %d rlineto\n", wa - wb, wa-wb);
		 fprintf(plotfile,"closepath fill\n");
		 ps_flush();
		 amove (x, y1);
		 rdraw(w, -h);
		 break;
	}
}
#endif

static void
draw_garrow(int argc, char **argv)
{
	int	x, y, x2, y2, w, dx, dy, dx2, dy2;
	double  angle, angle2, pi, len;

	x = (int) (mnumxpnts * atof(argv[3]));
	y = (int) (mnumypnts * (1.0 - atof(argv[4])));
	x2 = (int) (mnumxpnts * atof(argv[5]));
	y2 = (int) (mnumypnts * (1.0 - atof(argv[6])));
	if (mnumxpnts > mnumypnts)
	   w = mnumxpnts * 0.012;
	else
	   w = mnumypnts * 0.012;
	if (line_width > 2)
	   w = w + line_width;
	ps_flush();
	fprintf(plotfile,"newpath\n");
	dx = x2 - x;
        dy = y - y2;
        if ((dx == 0) && (dy == 0))
             return;
	pi = 3.14159265358979323846;
        angle = 0.0;
        len = sqrt((double)(dx * dx) + (double)(dy * dy));
        if (dx == 0)
        {
             if (y2 < y)
                angle = 0.5 * pi;
             else
                angle = -0.5 * pi;
        }
        else if (dy == 0)
        {
             if (x < x2)
                angle = 0.0;
             else
                angle = pi;
        }
        else
        {
             angle = acos((double)dx / len);
             if (dy < 0)
                angle = -angle;
        }
        angle2 = angle - pi;
	amove(x2, y2);
	dx = (int)(cos(angle2 + 0.314) * w);
        dy = (int)(sin(angle2 + 0.314) * w);
	fprintf(plotfile,"%d %d rlineto\n", dx, -dy);
	dx2 = (int)(cos(angle2 - 0.314) * w);
        dy2 = (int)(sin(angle2 - 0.314) * w);
	fprintf(plotfile,"%d %d rlineto\n", dx2 - dx, dy - dy2);
	fprintf(plotfile,"closepath fill\n");
	ps_flush();
	amove(x, y);
	if (line_width > 2)
	{
	    w -= 2;
	    x2 = x2 + (int)(cos(angle2) * w);
            y2 = y2 - (int)(sin(angle2) * w);
	}
	adraw(x2, y2);
}

static void
set_save_info(int argc, char **argv)
{
	int	k;

	save_format = 0;
	if (argc < 4)
	   return;
	for (k = 0; k < dformats_num; k++)
	{
	    if (strcmp(argv[2], dformats[k]) == 0)
	    {
		save_format = k + 1;
		break;
	    }
	}
	save_resolution = atoi(argv[3]);
}

/***
static
plot_draw_func(argc, argv)
int	argc;
char    **argv;
{
	double  f1, f2, f3;
	int	v1, v2, v3, v4;

	if (strcmp(argv[2], "font") == 0)
	{
	    v1 = atoi(argv[4]);
	    f1 = (double)v1 * 72.0 / screen_res;
	    v2 = (int) (3.880 * f1+0.5) + 3;
	    fprintf(plotfile,"/%s findfont %d scalefont setfont\n",argv[3], v2);
	    return;
	}
	if (strcmp(argv[2], "text") == 0)
	{
	    f1 = atof(argv[3]);
	    f2 = 1.0 - atof(argv[4]);
	    v1 = mnumxpnts * f1;
	    v2 = mnumypnts * f2;
	    amove(v1, v2);
	    dstring(argv[5]);
	    return;
	}
	if (strcmp(argv[2], "color") == 0)
	{
	    f1 = atof(argv[3]) / 255.0;
	    f2 = atof(argv[4]) / 255.0;
	    f3 = atof(argv[5]) / 255.0;
	    ps_flush();
	    fprintf(plotfile,"%4.3f %4.3f %4.3f setrgbcolor\n",f1,f2,f3);
	    return;
	}
	if (strcmp(argv[2], "line") == 0)
	{
	    f1 = atof(argv[3]);
	    f2 = 1.0 - atof(argv[4]);
	    v1 = mnumxpnts * f1;
	    v2 = mnumypnts * f2;
	    f1 = atof(argv[5]);
	    f2 = 1.0 - atof(argv[6]);
	    v3 = mnumxpnts * f1;
	    v4 = mnumypnts * f2;
	    amove(v1, v2);
	    adraw(v3, v4);
	    return;
	}
	if (strcmp(argv[2], "square") == 0)
	{
	    f1 = atof(argv[3]);
	    f2 = 1.0 - atof(argv[4]);
	    v1 = mnumxpnts * f1;
	    v2 = mnumypnts * f2;
	    f1 = atof(argv[5]);
	    f2 = atof(argv[6]);
	    v3 = mnumxpnts * f1;
	    v4 = mnumypnts * f2;
	    amove(v1, v2);
	    rdraw(v3, 0);
	    rdraw(0, -v4);
	    rdraw(-v3, 0);
	    rdraw(0, v4);
	    return;
	}
	if (strcmp(argv[2], "garrow") == 0)
	{
	    draw_garrow(argc, argv);
	    return;
	}
	if (strcmp(argv[2], "arrow") == 0)
	{
	    draw_arrow(argc, argv);
	    return;
	}
}

***/

void
kill_jplot(int pid)
{
         writeToJProc("exit_jplot");
}


#ifdef OLD
static void
clear_plot(int pid)
{
	if (jportFd > 0)
           close (jportFd);
	jportFd = -1;
        return;
}
#endif

void
jplot_page()
{
	if (!from_jplot) {
	    return;
	}
	from_jplot = 0;
	if ((int)strlen(out_file) <= 0) {
	    plot_page();
	    return;
	}
	strcpy(save_out, out_file);
	if (out_dpi == 0) {
	    out_dpi = 72;
	    if (out_format == PCL || out_format == PS || out_format == PS2)
 		out_dpi = org_resolution;
	}
	save_resolution = out_dpi;
	save_format = out_format;
	save_plot_data();
	reset_2_orginal();
}

int
is_vplot_page()
{
	return(do_page);
}

int
is_vplot_session(int queryOnly)
{
	if (debug)
	    fprintf(stderr, "  wc: %g  wc2: %g \n", wc, wc2);
	if (queryOnly)
	    return(vplot_session);
	if (vplot_session > 2)
	{
	    mnumxpnts = myxpnts;
	    mnumypnts = myypnts;
	    ppmm = myppmm;
	    wc2max = my_wc2max;
	    wcmax = my_wcmax;
	    xcharpixels = mycharx;
	    ycharpixels = mychary;
	    if (!preview)
	        set_ploter_coordinate(plot_rx, plot_ry);
	    if (get_plot_planes() > 1)
	    {
		if (org_raster < 3)
		{
		   if (preview || color_plot)
		        vplot_session = 8;
		}
		else
		{
		   if (preview || color_plot)
		        vplot_session = 9;
		}
	    }
	    active_gdevsw = &(gdevsw_array[C_PSPLOT]);
	}
	return(vplot_session);
}


static void
run_plot_constructor (int argc, char **argv)
{
	char    addr[MAXPATHL];
        char    cmd[512];

	if (jportFd >= 0)
	     return;
	if (set_exit_func > 0) {
            set_exit_func = 0;
            register_child_exit_func(SIGCHLD, 999, NULL, kill_jplot);
        }
	if (P_getstring(GLOBAL,"vnmraddr", data, 1, sizeof(data)-1))
        {
             Werrprintf("jplot: cannot get vnmraddr");
             return;
        }
#ifdef LINUX
        {
           int portTmp;
           int pidTmp;
           sscanf(data,"%s %d %d",cmd, &portTmp, &pidTmp);
	   sprintf(addr, "-h%s %d %d", cmd, htons(portTmp), pidTmp);
        }
#else
	sprintf(addr, "-h%s", data);
#endif
	sprintf(cmd, "vnmr_jplot  \"%s\"",   addr);
	writelineToVnmrJ("vnmrjcmd",cmd);
   	//system(cmd);
}

static void
exec_jtext_func(int argc, char **argv)
{
	char  *psFile;
	char  *pngFile;
	char  *gifFile;
	struct stat     f_stat;

	if (argc < 7)
        writeToJProc("null");
	psFile = argv[2];
	pngFile = argv[3];
	gifFile = argv[4];

    sprintf(data, "vnmr_gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=png256 -sOutputFile=%s.png -r%s -g%s  -q %s", psFile, argv[5], argv[6], psFile);

    system(data);
	sprintf(data, "%s.png", psFile);
	image_convert("-transparent white", "PNG", data, "GIF", gifFile);

//  sprintf(data, "-transparent white -size %s",argv[6]);
//	image_convert(data, "PS", psFile, "GIF", gifFile);
	if (stat(psFile, &f_stat) == 0)
	{
	   unlink(psFile);
	}

	sprintf(data, "%s.png", pngFile);
	if (stat(data, &f_stat) == 0)
	   unlink(data);

	if (jportFd > 0)
	{
	   if (stat(gifFile, &f_stat) == 0)
              writeToJProc(gifFile);
	   else
              writeToJProc("null");
	}
}


static void
exec_plot_func(int argc, char **argv)
{
	int	cmd, v1, v2, v3, v4;
	double  f1, f2, f3;

	if (plotfile == NULL)
	   return;
	cmd = get_plot_func(argv[2]);
	switch (cmd) {
	  case START:
	  case PSTART:
			plot_start_func(argc, argv);
			fprintf(plotfile,"0.0 0.0 0.0 setrgbcolor\n");
			break;
	  case END:
	  case PEND:
			plot_end_func(argc, argv);
			break;
	  case COLOR:
			if ((!preview) && (!color_plot))
			    return;
			f1 = atof(argv[3]) / 255.0;
			f2 = atof(argv[4]) / 255.0;
			f3 = atof(argv[5]) / 255.0;
			ps_flush();
			fprintf(plotfile,"%4.3f %4.3f %4.3f setrgbcolor\n",
				f1,f2,f3);
			break;
	  case LINEW:
			ps_flush();
			line_width = atoi(argv[3]);
			if (!preview)
			     line_width = line_width * myppmm / ps_ppmm;
			if (line_width < 1)
			   line_width = 1;
	    		fprintf(plotfile,"%d setlinewidth\n", line_width);
			ps_set_linewidth(line_width);
			break;
	  case FONT:
			v1 = atoi(argv[4]);
            		f1 = (double)v1 * 72.0 / screen_res;
           		textH = (int) (3.880 * f1+0.5) + 3;
/*
			if (preview)
            		   fprintf(plotfile,"/%s findfont %d scalefont setfont\n",argv[3], textH);
*/
			if (region_cmd || !preview)
			{
			/* if (region_cmd) */
			     f1 = (double) v1 / 14.0;
			     v1 = (int) (3.880*myppmm*f1+0.5);
            		     fprintf(plotfile,"/%s findfont %d scalefont setfont\n",argv[3], v1);
			     jplot_charsize (f1);
			     mycharx = xcharpixels;
			     mychary = ycharpixels;
			}
			else
            		   fprintf(plotfile,"/%s findfont %d scalefont setfont\n",argv[3], textH);
			break;
	  case WSIZE:
			win_width = atoi(argv[3]);
			win_height = atoi(argv[4]);
			if (preview)
                        {
                            xsize = win_width;
                            ysize = win_height;
                        }
			break;
	  case DTEXT:
			f1 = atof(argv[3]);
			f2 = 1.0 - atof(argv[4]);
			v1 = mnumxpnts * f1;
			if (preview)
			v2 = mnumypnts * f2 + textH / 4;
			else
			v2 = mnumypnts * f2 + (textH / 4) * (ppmm / ps_ppmm);
			amove(v1, v2);
			dstring(argv[5]);
			break;
	  case DLINE:
			v1 = (int) (atof(argv[3]) * mnumxpnts);
			v2 = (int) ((1.0 - atof(argv[4])) * mnumypnts);
			v3 = (int) (atof(argv[5]) * mnumxpnts);
			v4 = (int) ((1.0 - atof(argv[6])) * mnumypnts);
			amove(v1, v2);
			adraw(v3, v4);
			break;
	  case DSQR:
			ps_flush();
			v1 = (int) (atof(argv[3]) * mnumxpnts);
			v2 = (int) ((1.0 - atof(argv[4])) * mnumypnts);
			v3 = (int) (atof(argv[5]) * mnumxpnts);
			v4 = (int) (atof(argv[6]) * mnumypnts);
			amove(v1, v2);
			fprintf(plotfile,"%d 0 rlineto\n", v3);
			fprintf(plotfile,"0 %d rlineto\n", -v4);
			fprintf(plotfile,"%d 0 rlineto\n", -v3);
		 	fprintf(plotfile,"closepath stroke\n");
			ps_flush();
			break;
	  case DARROW:
			draw_garrow(argc, argv);
			break;
	}
}

static void
get_default_template()
{
	FILE  *fd;

	sprintf(data, "%s/templates/plot/.default", userdir);
	if ((fd = fopen(data, "r")) == NULL)
	{
	   sprintf(data, "%s/user_templates/plot/.default", systemdir);
	   if ((fd = fopen(data, "r")) == NULL)
		return;
	}
	while (fgets(data, 510, fd) != NULL)
        {
	   if (sscanf(data, "%s", plot_template) == 1)
	   {
		if (strlen(plot_template) > 0)
		   break;
	   }
	}
	fclose(fd);
	if (strlen(plot_template) <= 0)
	    return;
	if (plot_template[0] == '/')
	    strcpy(data, plot_template);
	else
	    sprintf(data, "%s/templates/plot/%s", userdir, plot_template);
        if (access (data, R_OK) == 0)
	{
	    strcpy(plot_template, data);
            return;
	}
	if (plot_template[0] != '/')
	    sprintf(data, "%s/user_templates/plot/%s", systemdir, plot_template);
        if (access (data, R_OK) == 0)
	{
	    strcpy(plot_template, data);
            return;
	}
	plot_template[0] = '\0';
}

static int access_template()
{
	if (plot_template[0] == '/')
	    strcpy(data, plot_template);
	else
	    sprintf(data, "%s/templates/plot/%s", userdir, plot_template);
        if (access (data, R_OK) == 0)
	{
	    strcpy(plot_template, data);
            return(1);
	}
	if (plot_template[0] == '/')
            return(0);
	sprintf(data, "%s/user_templates/plot/%s", systemdir, plot_template);
        if (access (data, R_OK) == 0)
	{
	    strcpy(plot_template, data);
            return(1);
	}
	return(0);
}

static void
exec_template()
{
	char    *d;
	char	macro[MAXPATHL];

	d = rindex(plot_template, '/');
	if (d != NULL)
	    strcpy(macro, d+1);
	else
	    strcpy(macro, plot_template);
	jfinished = 0;
	init_params();
	sprintf(data,"macrold('%s'):$dum %s\n",plot_template, macro);
	execString(data);
	sprintf(data,"purge('%s')\n", macro);
	execString(data);
	ps_set_linewidth(1);
/*
	reset_2_orginal();
*/
	if (!jfinished)
             Werrprintf("jplot failed.");
}

static void
create_new_socket(int port)
{
	int result;
	int i;
	int on = 1;
#ifdef VNMRJ
        Socket jSocket;
#else
	int  new_fd;
	struct hostent *hp;
	static struct sockaddr_in  msgsockname;
#endif

	if (jportFd >= 0)
	     close(jportFd);
	jportFd = -1;
	jportNum = port;
#ifdef VNMRJ

        memset(&jSocket, 0, sizeof( Socket ) );
        jSocket.sd = -1;
        jSocket.protocol = SOCK_STREAM;
        if (openSocket(&jSocket) < 0)
             return;
        setsockopt(jSocket.sd,SOL_SOCKET,(~SO_LINGER),(char *)&on,sizeof(on));
        result = -1;
        for( i=0; i < 5; i++)
        {
            result = connectSocket(&jSocket,VnmrJHostName,port);
            if (result == 0)
                break;
            sleep(1);
        }
        if (result != 0)
            return;
	jportFd = jSocket.sd;
        return;
#else
        gethostname(LocalHost,sizeof(LocalHost));

        hp = (struct hostent *)gethostbyname(LocalHost);
        if (hp == NULL)
        {
            return;
        }
        memcpy((char *)&msgsockname.sin_addr, hp->h_addr, hp->h_length);
        msgsockname.sin_family = hp->h_addrtype;
        msgsockname.sin_port = htons(port);

        for (i=0; i < 5; i++)
        {
           new_fd = socket(AF_INET,SOCK_STREAM,0);
           if (new_fd == -1)
           {
                break;
           }
           setsockopt(new_fd,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));
           if ((result = connect(new_fd,(struct sockaddr *) &msgsockname,sizeof(msgsockname))) != 0)
           {
                if (errno != ECONNREFUSED && errno != ETIMEDOUT)
                {
                        close(new_fd);
                        new_fd = -1;
                        break;
                }
           }
           else
                break;
           close(new_fd);
           new_fd = -1;
           sleep(1);
        }
	jportFd = new_fd;;
#endif
}

static void
discard_all()
{
	if (plotfile == NULL)
           return;
        fclose(plotfile);
        plotfile = 0;
        plot = 0;
        init_tog = 0;
	unlink(plotpath);
	active_gdevsw = &(gdevsw_array[C_TERMINAL]);
        disp_plot("    ");
        reset_2_orginal();
}

static void
init_jplot(int argc, char **argv)
{
        int     i, k;
	char    *cstr;

	if (from_jplot) {
	    discard_all();
	    from_jplot = 0;
	}
	plot_template[0] = '\0';
	out_file[0] = '\0';
	debug = 0;
	out_dpi = 0;
	out_format = 0;
	preview = 0;
	if (argc > 1)
	{
	    for (i = 1; i < argc; i++)
	    {
		if (argv[i][0] == '-')
		{
		    if (strcmp(argv[i], "-debug") == 0)
		   	debug = 1;
		    else if (strcmp(argv[i], "-dpi") == 0) {
			if (i < (argc-1)) {
			    out_dpi = atoi(argv[i+1]);
			}
			i++;
		    }
		    else if (strcmp(argv[i], "-file") == 0) {
			if (i < (argc-1)) {
			    strcpy(out_file, argv[i+1]);
			}
			i++;
		    }
		    else if (strcmp(argv[i], "-format") == 0) {
	     		cstr = argv[i+1];
			if (i < argc-1) {
			    for (k = 0; k < strlen(cstr); k++) {
				data[k] = toupper(cstr[k]);
			    }
			    data[k] = '\0';
			   for (k = 0; k < dformats_num; k++)
			   {
	    			if (strcmp(data, dformats[k]) == 0)
	    			{
				   out_format = k + 1;
				   break;
				}
			   }
			}
			i++;
			if (out_format == 0) {
             		    Werrprintf("jplot: format '%s' is not supported ", data);
			    return;
			}
		    }
		}
		else {
		   if ((int) strlen(plot_template) <= 0)
		   	sprintf(plot_template, argv[i]);
		}
	    }
	}
	if (out_format == 0) {
	    if ((org_raster == 1) || (org_raster == 2))
		out_format = PCL;
	    else
		out_format = PS;
	}
	else {
	    if ((int) strlen(out_file) <= 0) {
               Werrprintf("jplot: the argument of output file is missing ");
		return;
	    }
	}
	if ((int) strlen(plot_template) <= 0)
	    get_default_template();
	if ((int) strlen(plot_template) <= 0)
	{
             Werrprintf("jplot: there is no default template, try jdesign");
	     return;
	}
        if (access_template () < 1)
	{
             Werrprintf("jplot: could not access template '%s'", plot_template);
	     return;
	}
	from_jplot = 1;
	exec_template();
}


int
jplot(int argc, char **argv, int retc, char **retv)
{
        int     i, k;
#ifdef DEBUG_JPLOT
        int     j;
#endif
	char    *cstr = NULL;

#if defined (IRIX) || (AIX)
        Werrprintf("jplot does not work for SGI and IBM ");
	RETURN;
#else 
	if (argc > 1)
	{
	     cstr = argv[1];
	     while ((*cstr == ' ') || (*cstr == '\t'))
		cstr++;
	}
        color_plot = 1;
	if ((argc > 1) && (*cstr == '-'))
	{
	     k = get_plot_cmd(cstr);
#ifdef DEBUG_JPLOT
	     char tmps[256]={0};
	     for(j=1;j<argc;j++){
	    	 strcat(tmps,argv[j]);
	    	 strcat(tmps," ");
	     }
		 Winfoprintf("jplot: %s",tmps);
#endif
	     switch (k) {
		case MACRO:
			   if (from_jplot) {
				discard_all();
				from_jplot = 0;
			   }
			   from_jdesign = 1;
			   plot_macro_func(argc, argv);
			   from_jdesign = 0;
			   break;
		case SETUP:
			   if (jportFd < 1) {
                                run_plot_constructor(argc, argv);
                           }
                           else
                                writeToJProc("open_jplot");
			   break;
		case REGION:
			   region_cmd = 1;
			   exec_plot_func(argc, argv);
			   break;
		case DRAW:
			   region_cmd = 0;
			   exec_plot_func(argc, argv);
			   break;
		case PREVIEW:
			   preview = atoi(argv[2]);
			   break;
		case VTEXT:
			   view_text = atoi(argv[2]);
			   break;
		case JTEXT:
			   exec_jtext_func(argc, argv);
			   break;
		case PLOTTER:
			   vplot_session = 0;
			   if (argc < 5)
				RETURN;
			   paper_w = atof(argv[3]);
			   paper_h = atof(argv[4]);
			   if (argc > 5)
			      color_plot = atoi(argv[5]);
			   set_v_plotter(argv[2]);
			   break;
		case PSAVE:
			   save_data = 1;
			   sprintf(save_out, argv[2]);
			   break;
		case PSCREEN:
			   screen_res = atof(argv[2]);
			   break;
		case PPAGE:
			   jfinished = 1;
			   if (from_jplot) {
				jplot_page();
			   }
			   else {
				if (preview)
			       	   preview_page();
				else if (save_data)
				    save_plot_data();
				else
			       	    plot_page();
			   }
			   vplot_session = 0;
			   preview = 0;
			   view_text = 0;
			   do_page = 0;
			   need_ret = 0;
			   out_dpi = 0;
			   break;
		case PFORMAT:
			   if (!from_jdesign) {
				init_jplot(argc, argv);
			   }
			   else
			        set_save_info(argc, argv);
			   break;
		case JDPI:
			   if (!from_jdesign) {
				init_jplot(argc, argv);
			   }
			   break;
		case JFILE:
			   if (!from_jdesign) {
				init_jplot(argc, argv);
			   }
			   break;
		case PPROC:
			   plot_pid = atoi(argv[2]);
			   break;
		case PCLOSE:
		case PEXIT:
			   i = atoi(argv[2]);
			   if (i == jportNum) {
				if (jportFd >= 0)
				    close(jportFd);
			    	jportFd = -1;
				jportNum = -1;
			   }
			   break;
		case DEBUG:
			   if (debug == 0)
			      debug = 1;
			   else
			      debug = 0;
			   break;
		case JPORT:
			   create_new_socket(atoi(argv[2]));
			   break;

		default:
             		   Werrprintf("jplot: unknown argument '%s'", argv[1]);
			   break;
	     }
	     RETURN;
	}
	init_jplot(argc, argv);
        RETURN;
#endif 
}

int
get_plot_linewidth()
{
	return(line_width);
}

int
plot_active()
{
	if (jportFd < 0)
	    return(0);
	else
	    return(1);
}

