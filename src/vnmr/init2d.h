/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------
|						|
|    init2d.h   - defines global 2D variables	|
|						|
+----------------------------------------------*/
#ifndef INIT2D_H
#define INIT2D_H

/*-----------------------------------------------------------------------
|									|
|   Definitions for arguments in calls to init2d and select_init	|
|   A blank line separates definitions for different arguments in	|
|   the call to select_init / init2d					|
|									|
-----------------------------------------------------------------------*/

#define  GET_REV	1

#define  GRAPHICS	1
#define  PLOTTER	2
#define  NO_DISPLAY	0

#define  NO_FREQ_DIM	0

#define  NO_HEADERS	0
#define  DO_HEADERS	1

#define  NO_CHECK2D	0
#define  DO_CHECK2D	1

#define  NO_SPECPARS	0
#define  DO_SPECPARS	1

#define  NO_BLOCKPARS	0
#define  DO_BLOCKPARS	1

#define  NO_PHASEFILE	0
#define  DO_PHASEFILE	1

/*-------------------------------
|				|
|   Display Global Variables	|
|				|
+------------------------------*/

extern char	nucHoriz[8], 
		nucVert[8];

extern int	dfpnt,
		dnpnt,
		dfpnt2,
		dnpnt2,
		fpnt,
		npnt,
		fpnt1,
		npnt1,
                normInt,        /* normalize 1d integral flag           */
                normInt2;       /* normalize 2d integral flag           */

extern float	normalize;

extern double	sc,
		wc,
		sp,
		wp,
		sw,
		rflrfp,
		sc2,
		wc2,
		sp1,
		wp1,
		sw1,
		rflrfp1,
		sfrq,
		sfrq1,
		vs,
		vs2d,
		vsproj,
		is,
		insval,
		ins2val,
		io,
		th,
		vp,
		vpi,
		vo,
		ho,
		delta,
		cr,
		lvl,
		tlt;


/*-------------------------------
|				|
|  Processing Global Variables	|
|				|
+------------------------------*/

extern int	fn,		/* Fourier number for the F2 dimension	*/
		fn1,		/* Fourier number for the F1 dimension	*/
		ni,		/* number of t1 (t3) increments		*/
		c_first,	/* first buffer number for PHASFILE	*/
		c_last,		/* last buffer number for PHASFILE	*/
		c_buffer,	/* current buffer number for PHASFILE	*/
		revflag,	/* flag for trace = 'f2' display	*/
		dophase,	/* flag for F2 phased display		*/
		doabsval,	/* flag for F2 absolute-value display	*/
		dopower,	/* flag for F2 power display		*/
		dophaseangle,	/* flag for F2 phase angle display	*/
		dof1phase,	/* flag for F1 phased display		*/
		dof1absval,	/* flag for F1 absolute-value display	*/
		dof1power,	/* flag for F1 power display		*/
		dof1phaseangle,	/* flag for F1 phase angle display	*/
		d2flag,		/* flag for 2D data			*/
		normflag,	/* flag for normalization of display	*/
		specperblock,	/* number of spectra per block		*/
		nblocks,	/* number of data blocks		*/
		pointsperspec,	/* number of points per spectrum	*/
		specIndex;	/* current spectrum index		*/


extern double	lp,		/* first-order phase constant (F1,F3)	*/
		rp,		/* zero-order phase contant (F1,F3)	*/
		lp1,		/* first-order phase constant (F2)	*/
		rp1;		/* zero-order phase contant (F2)	*/

extern dpointers	c_block;	/* data pointer for PHASFILE	*/
extern dfilehead	datahead,	/* header for DATA		*/
			phasehead;	/* header for PHASFILE		*/

extern float *gettrace(int trace, int fpnt);
extern float *calc_spec(int trace, int fpnt, int dcflag, int normok, int *newspec);
extern float *get_data_buf(int trace);

extern int getnd();
extern int getplaneno();
extern void checkreal(double *r, double min, double max);
extern void set_sp_wp(double *spval, double *wpval, double swval, int pts, double ref);
extern void set_sp_wp_rev(double *spval, double *wpval, double swval, int pts, double ref);
extern void setVertAxis();
extern int checkphase(short status);
extern int checkphase_datafile();
extern int phasepars();
extern int check2d(int get_rev);
extern int init2d_getchartparms(int checkFreq);
extern int init2d_getfidparms(int dis_setup);
extern double expf_dir(int direction);
extern int exp_factors( int spec);
extern int dataheaders(int getphasefile, int checkstatus);
extern int select_init(int get_rev, int dis_setup, int fdimname, int doheaders, int docheck2d,
                int dospecpars, int doblockpars, int dophasefile);
extern int init2d(int get_rev, int dis_setup);
extern int initfid(int dis_setup);
extern int get_dis_setup();

#endif
