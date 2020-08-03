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
/*    init2d.c  -  module to initialize display 	*/
/*                 and processing parameters for	*/
/*                 1D and for 2D data sets		*/
/*							*/
/********************************************************/

/******************
*  Include files  *
******************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init_display.h"
#include "init_proc.h"
#include "tools.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "vfilesys.h"
#include "wjunk.h"

/***********************
*  Global definitions  *
***********************/

#define COMPLETE	0
#define ERROR		1
#define FALSE		0
#define TRUE		1
#define WCMIN		1.0
#define MIN_NPNT        5
#define BUFWORDS	65536
#define DEFAULT_vsproj	400.0
#define DEFAULT_vs2d	1000.0

#define C_GETPAR(name,value) \
  if ( (r=P_getreal(CURRENT,name,value,1)) ) \
     { P_err(r,name,":"); return ERROR; }

#define P_GETPAR(name,value) \
  if ( (r=P_getreal(PROCESSED,name,value,1)) ) \
    { P_err(r,name,":");   return ERROR; }

#define getdatatype(status)  \
  ( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX : \
    ( (status & S_COMPLEX) ? COMPLEX : REAL ) )

/**************************
*  External declarations  *
**************************/

extern int	bufferscale;
extern void p11_saveDisCmd();
extern int new_phasefile(dfilehead *phasehead, int d2flg, int nblocks, int fn_0,
                  int fn_1, int setstatus, int hypercomplex);
extern int removephasefile();
extern int  setplotter();
extern int	dim1count();    /* located in ft2d.c  */
extern void setArraydis(int mode);
extern int expdir_to_expnum(char *expdir);

#ifdef VNMRJ
extern int frame_set_pnt();
#endif

/*************************************
*  Global definitions for debugging  *
*************************************/

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
#else 
#define DPRINT(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#endif 

/**********************************************
*  The following global variables are always  *
*  set by init2d().                           *
**********************************************/

int		revflag,	/* 2D data transpose flag		*/
		d2flag,		/* 2D data flag				*/
		ni,		/* number of t1 (t3) increments         */
		fn,		/* Fourier number for the F2 dimension  */
		fn1,		/* Fourier number for the F1 dimension  */
		specperblock,	/* number of spectra per block		*/
		nblocks,	/* number of data blocks		*/
		pointsperspec,	/* number of points per spectrum	*/
		c_first,	/* max descriptor for 2D data buffer	*/
		c_last,		/* min descriptor for 2D data buffer	*/
		c_buffer;	/* descriptor for 2D data buffer	*/

double		sw,		/* spectral width for the F2 dimension	*/
		sw1;		/* spectral width for the F1 dimension	*/

dfilehead	datahead,	/* header information for datafile	*/
		phasehead;	/* header information for phasfile	*/

dpointers	c_block;	/* pointer to PHASFILE data		*/

static int	dimension_name;	/* name of active dimension (S_NP)	*/
static int	ndflag = ONE_D; /* number of Data dimensions            */
static char     normchar,revchar;


/********************************************
*  The following global variables are only  *
*  set by init2d() for spectral data if     *
*  "dospecpars" is TRUE.                    *
********************************************/

int		normflag,       /* normalization flag                   */
                dophase,        /* flag for F2 phased display           */
                doabsval,       /* flag for F2 absolute-value display   */
                dopower,        /* flag for F2 power display            */
                dophaseangle,   /* flag for F2 phase angle display      */
                dodbm,          /* flag for F2 phase angle display      */
                dof1phase,      /* flag for F1 phased display           */
                dof1absval,     /* flag for F1 absolute-value display   */
                dof1power,      /* flag for F1 power display            */
                dof1phaseangle; /* flag for F1 phase angle display      */

double		rp,		/* zero-order phase constant (horiz.)	*/
		lp,		/* first-order phase constant (horiz.)	*/
		rp1,		/* zero-order phase constant (vert.)	*/
		lp1;		/* first-order phase constant (vert.)	*/

/***************************************************
*  The following global variables are only set by  *
*  init2d() if "dis_setup" is TRUE.                *
***************************************************/

int		newspec,	/* set by gettrace() and calc_spec()	*/
		specIndex,	/* current spectrum index		*/
                normInt,        /* normalize 1d integral flag           */
                normInt2,       /* normalize 2d integral flag           */
		dfpnt,
		dnpnt,
		dfpnt2,
		dnpnt2,
		fpnt,
		npnt,
		fpnt1,
		npnt1;

float		normalize;

double		sc,
		wc,
		sp,
		wp,
		rflrfp,
		sc2,
		wc2,
		sp1,
		wp1,
		rflrfp1,
		sfrq,
		sfrq1,
		vs,
		vs2d = DEFAULT_vs2d,
		vsproj = DEFAULT_vsproj,
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

static double	aspect_ratio;
static int	saved_dis_setup;
static char axisHoriz,axisVert;

/**************
*  FUNCTIONS  *
**************/

/*
 *  THe getnd and resetnd functions are currently used to decide if a plane
 *  index needs to be displayed when disp_specIndex() is called.  jexp and
 *  rt functions call resetnd()
 */
int getnd()
{
   return(ndflag);
}

#ifdef OLD
static int resetnd()
{
   ndflag = ONE_D;
}
#endif

int getplaneno()
{
   double tmp;

   if ( P_getreal(PROCESSED, "index2", &tmp, 1) < 0 )
   {
      return(-1);
   }
   else
   {
      return( (int) tmp);
   }
}
   

/********************/
void checkreal(double *r, double min, double max)
/********************/
{

/***************************************************
*  Check a double precision floating point number  *
*  for limits; set to limit if range is exceeded.  *
***************************************************/

  if (*r < min)
  {
     *r = min;
  }
  else
  {
     if (*r > max)
        *r = max;
  }
}

/* pts	 total number of points; generally fn/2 */
/************************************/
static void set_sf_wf(double *spval, double *wpval, double swval, int pts)
/************************************/
{
  register double hzpp;

  hzpp = swval/(double) pts;
  checkreal(wpval,(double) MIN_NPNT * hzpp,swval - hzpp );
  *wpval = (double) ((int) (*wpval/hzpp + 0.01)) * hzpp;
  checkreal(spval,0.0,swval - *wpval - hzpp);
  *spval = (double) ((int) (*spval/hzpp + 0.01)) * hzpp;
}

/************************************/
void set_sp_wp(double *spval, double *wpval, double swval, int pts, double ref)
/************************************/
{
  register double hzpp;

  hzpp = swval/(double) pts;
  checkreal(wpval,(double) MIN_NPNT * hzpp,swval - hzpp );
  *wpval = (double) ((int) (*wpval/hzpp + 0.01)) * hzpp;
            
  checkreal(spval,hzpp - ref,swval - *wpval - ref);
  *spval = (double) ((int) ((*spval + ref)/hzpp + 0.01)) * hzpp - ref;
}

/************************************/
void set_sp_wp_rev(double *spval, double *wpval, double swval, int pts, double ref)
/************************************/
{
  register double hzpp;

  hzpp = swval/(double) (pts-1);
  checkreal(wpval,(double) MIN_NPNT * hzpp,swval);
  *wpval = (double) ((int) (*wpval/hzpp + 0.01)) * hzpp;
            
  checkreal(spval,-swval - ref + *wpval, - ref);
//  *spval = (double) ((int) ((*spval)/hzpp + 0.01)) * hzpp;
}

/*---------------------------------------
|					|
|	      checkphase()/4		|
|					|
|   This function checks to see if the	|
|   data are phaseable.  If the data    |
|   are hypercomplex, only the appro-	|
|   complex part is extracted for	|
|   phasing purposes.			|
|					|
+--------------------------------------*/
int checkphase(short status)
{
   int cannotphase = FALSE;

   if (d2flag)
   {
      if (revflag)
      {
         if ( !(status & (NF_CMPLX|NI_CMPLX|NI2_CMPLX)) )
            cannotphase = TRUE;
      }
      else
      {
         if ((~status) & NP_CMPLX)
            cannotphase = TRUE;
      }
   }
   else
   {
      if ((~status) & NP_CMPLX)
         cannotphase = TRUE;
   }
   return(cannotphase);
}

int checkphase_datafile()
{
   dpointers datablock;

   if (D_getbuf(D_DATAFILE, datahead.nblocks, c_buffer, &datablock))
      return(FALSE);
   else
      return(checkphase(datablock.head->status));
}

/*******************************************
*  Check to see if the display parameters  *
*  are set for a phased display.           *
*******************************************/
int phasepars()
{
   return(!(get_phase_mode(HORIZ) || get_phaseangle_mode(HORIZ)));
}

/*---------------------------------------
|					|
|		check2d()/1		|
|					|
+--------------------------------------*/
int check2d(int get_rev)
{
  char		trace[5],
		ni0name[6];
  int		r;
  double	rni;
  int           dim0,dim1;


/******************************************************
*  Determines whether the experiment contains 1D or   *
*  2D data; sets D2FLAG, REVFLAG, and NI; if get_rev  *
*  is FALSE, REVFLAG is assumed to be set by the      *
*  calling program.                                   *
******************************************************/

  /* Defaults for 1D data */
  ndflag = ONE_D;
  dim1 = dim0 = S_NP;
  dimension_name = S_NP;
  d2flag = FALSE;
  ni = 1;
  normchar = ' ';
  revchar = '1';

  if (datahead.status & S_TRANSF)	/* 2D data */
  {
     ndflag = TWO_D;
     normchar = '2';
     if (datahead.status & S_3D)
        ndflag = THREE_D;
     if (datahead.nbheaders & ~NBMASK)
        ndflag = FOUR_D;

     if (ndflag == TWO_D)
     {
        int niVal=0, ni2Val=0;
        if ( !P_getreal(PROCESSED, "ni", &rni, 1) ) niVal = (int)rni;
        if ( !P_getreal(PROCESSED, "ni2", &rni, 1) ) ni2Val = (int)rni;
        if (datahead.status & S_NF)
        {
           strcpy(ni0name, "nf");
           dimension_name |= S_NF;
        }
        else if (datahead.status & S_NI)
        { 
           strcpy(ni0name, "ni"); 
           dimension_name |= S_NI;
	   if(ni2Val > 0) {
		normchar='3';
	      	revchar='1';
	   } 
        } 
        else if (datahead.status & S_NI2)
        {
           strcpy(ni0name, "ni2");
           dimension_name |= S_NI2;
	   if(niVal > 0) {
		normchar='3';
	      	revchar='2';
	   } 
        }
        else
        {
           Werrprintf("Invalid F1 dimension in first data block header");
           return(ERROR);
        }
     }

     if (ndflag == THREE_D)
     {
        if ((datahead.status & (S_NP|S_NF)) == (S_NP|S_NF) )
        {
           strcpy(ni0name, "nf");
           normchar = '3';
           revchar = '1';
           dimension_name |= S_NF;
        }
        else if ((datahead.status & (S_NP|S_NI)) == (S_NP|S_NI) )
        { 
           strcpy(ni0name, "ni"); 
           normchar = '3';
           revchar = '1';
           dimension_name |= S_NI;
        } 
        else if ((datahead.status & (S_NP|S_NI2)) == (S_NP|S_NI2) )
        {
           strcpy(ni0name, "ni2");
           normchar = '3';
           revchar = '2';
           dimension_name |= S_NI2;
        }
        else if ((datahead.status & (S_NI|S_NI2)) == (S_NI|S_NI2) )
        {
           strcpy(ni0name, "ni");
           normchar = '2';
           revchar = '1';
           dimension_name = S_NI|S_NI2;
        }
        else
        {
           Werrprintf("Invalid dimension in first data block header");
           return(ERROR);
        }
     }
     if (ndflag == FOUR_D)
     {
        if ((datahead.nbheaders & (ND_NP|ND_NI)) == (ND_NP|ND_NI) )
        {
           strcpy(ni0name, "ni"); 
           normchar = '4';
           revchar = '1';
           dimension_name |= S_NI;
        }
        else if ((datahead.nbheaders & (ND_NP|ND_NI2)) == (ND_NP|ND_NI2) )
        { 
           strcpy(ni0name, "ni2"); 
           normchar = '4';
           revchar = '2';
           dimension_name |= S_NI2;
        } 
        else if ((datahead.nbheaders & (ND_NP|ND_NI3)) == (ND_NP|ND_NI3) )
        {
           strcpy(ni0name, "ni3");
           normchar = '4';
           revchar = '3';
           dimension_name |= S_NI3;
        }
        else if ((datahead.nbheaders & (ND_NI|ND_NI2)) == (ND_NI|ND_NI2) )
        {
           strcpy(ni0name, "ni2");
           normchar = '1';
           revchar = '2';
           dimension_name = S_NI|S_NI2;
        }
        else if ((datahead.nbheaders & (ND_NI|ND_NI3)) == (ND_NI|ND_NI3) )
        {
           strcpy(ni0name, "ni3");
           normchar = '1';
           revchar = '3';
           dimension_name = S_NI|S_NI3;
        }
        else if ((datahead.nbheaders & (ND_NI2|ND_NI3)) == (ND_NI2|ND_NI3) )
        {
           strcpy(ni0name, "ni3");
           normchar = '2';
           revchar = '3';
           dimension_name = S_NI|S_NI3;
        }
        else
        {
           Werrprintf("Invalid dimension in first data block header");
           return(ERROR);
        }
     }

     if ( (r = P_getreal(PROCESSED, ni0name, &rni, 1)) )
     {
        Werrprintf("Unable to get %s 2D parameter for 2D data matrix\n",
			ni0name);
        d2flag = FALSE;
        return(ERROR);
     }
     else
     {
        d2flag = TRUE;
        ni = (int) (rni + 0.5);
        if (ni == 0)
           ni = 1;
     }
  }

/************************************
*  Set REVFLAG if get_rev is TRUE.  *
************************************/

  if (get_rev)
  {
     revflag = FALSE;
     if (d2flag)
     {
        if ( (r = P_getstring(CURRENT, "trace", trace, 1, 4)) )
        {
           P_err(r, "trace", ":");
           return(ERROR);
        }

        if(revchar=='2' && normchar=='3') 
	   revflag = (trace[1] == '1');
	else 
	   revflag = (trace[1] == revchar);
     }
  }

  return(COMPLETE);
}


/************************/
static void constrain_aspect_ratio()
/************************/
{
  double	wc_est,
		wc2_est,
		device_ar;

/*******************************************************
*  This algorithm uses space correctly and optimally.  *
*******************************************************/

  device_ar = ((double) (mnumxpnts - right_edge))/((double) (mnumypnts*ymultiplier));  
  device_ar *= (wc2max+BASEOFFSET)/wcmax;
  wc2_est = wc/(aspect_ratio/device_ar);
  wc_est = wc2*aspect_ratio/device_ar;

  if (wc2_est > wc2)
  {
     wc = wc_est;
     checkreal(&wc,WCMIN,wcmax-wcmax*0.1);   
  }
  else
  {
     wc2 = wc2_est;
     checkreal(&wc2,WCMIN,wc2max-wc2max*0.02);
  }
}


/**********************/
int init2d_getchartparms(int checkFreq)
/**********************/
{
  int	r;

  C_GETPAR("sc",&sc);
  C_GETPAR("wc",&wc);
  C_GETPAR("sc2",&sc2);
  C_GETPAR("wc2",&wc2);
  checkreal(&wc,WCMIN,wcmax);   
  checkreal(&sc,0.0,wcmax-wc);	       
  checkreal(&wc2,WCMIN,wc2max);
  checkreal(&sc2,0.0,wc2max-wc2);	    
  P_setreal(CURRENT,"wc",wc,0);
  P_setreal(CURRENT,"sc",sc,0);
  P_setreal(CURRENT,"wc2",wc2,0);
  P_setreal(CURRENT,"sc2",sc2,0);
  if ( checkFreq && get_axis_freq(HORIZ) && get_axis_freq(VERT) )
  {
     if (((axisHoriz == 'c') || (axisHoriz == 'm') || (axisHoriz == 'u'))
         && ((axisVert == 'c') || (axisVert == 'm') || (axisVert == 'u')))
       constrain_aspect_ratio();
  }
  return COMPLETE;
}

/*-----------------------------------------------
|						|
|	     init2d_getfidparms()/2		|
|						|
|   This function initializes various data	|
|   parameters.					|
|						|
+----------------------------------------------*/
int init2d_getfidparms(int dis_setup)
{
  char		aig[5];
  int		r;
  double	rnp;
  double	hzpp;


  d2flag = FALSE;
  revflag = FALSE;
  ni = 1;


  if ( (r=P_getstring(CURRENT, "aig", aig, 1, 4)) )
  {
     if ( expdir_to_expnum(curexpdir) > 0 )
        P_err(r, "aig", ":");
     return(ERROR);
  }

  normflag = (aig[0] == 'n');

  if (P_getreal(CURRENT, "vpf", &vp, 1))
     C_GETPAR("vp", &vp);
  if (P_getreal(CURRENT, "vpfi", &vpi, 1))
     vpi = vp;
  C_GETPAR("vf", &vs);
  if (P_getreal(CURRENT, "crf", &cr, 1))
     C_GETPAR("cr", &cr);
  if (P_getreal(CURRENT, "deltaf", &delta, 1))
     C_GETPAR("delta", &delta);

  C_GETPAR("sf", &sp);
  C_GETPAR("wf", &wp);
  P_GETPAR("at", &sw);
  P_GETPAR("np", &rnp);

  fn = (int) (rnp + 0.5);

/**************************************
*  Initialize certain 2D parameters.  *
**************************************/

  rflrfp = 0.0;
  sw1 = 0.0;
  rflrfp1 = 0.0;
  fn1 = fn;

  hzpp = sw/(double) (fn/2);
  checkreal(&wp, 4.0*hzpp, sw-hzpp);
  wp = (double) ((int) (wp/hzpp + 0.01)) * hzpp;
  checkreal(&sp, rflrfp, sw-wp-hzpp);
  sp = (double) ((int) (sp/hzpp + 0.01)) * hzpp;
  if (dis_setup)
  {
     P_setreal(CURRENT, "wf", wp, 0);
     P_setreal(CURRENT, "sf", sp, 0);
  }

  set_fid_display();
  get_sfrq(HORIZ,&sfrq);
  get_scale_axis(HORIZ,&axisHoriz);
  axisVert = '\0';

  C_GETPAR("vo", &vo);
  C_GETPAR("ho", &ho);

  DPRINT4("np=%d, at=%g, rflrfp=%g, sfrq=%g\n",
                  fn,sw,rflrfp,sfrq);

  return(COMPLETE); 
}

void setVertAxis()
{
     double axis_scl;
     int    reversed;

     get_ref_pars(VERT,&sw1,&rflrfp1,&fn1);
     get_scale_pars(VERT,&sp1,&wp1,&axis_scl,&reversed);
     if (get_axis_freq(VERT))
     {
        if (reversed)
        {
           set_sp_wp_rev(&sp1, &wp1, sw1, fn1/2, rflrfp1);
        }
        else
        {
           set_sp_wp(&sp1, &wp1, sw1, fn1/2, rflrfp1);
        }
     }
     else
     {
        set_sf_wf(&sp1, &wp1, sw1, fn1/2);
     }
     UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
     UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
}

/*---------------------------------------
|					|
|	     getspecparms()/2		|
|					|
+--------------------------------------*/
/* dis_setup,	display program flag */
/* frqdimname;	frequency dimension  */
static int getspecparms(int dis_setup, int frqdimname)
{
  char		aig[5],
		dmg[5],
		dmg1[5];
  int		dim0,dim1;
  int		r;
  double	x0;

// min, max sw values
  double minval, maxval;
  if(P_getmin(PROCESSED, "sw", &minval) ) minval = 200.0;
  if(P_getmax(PROCESSED, "sw", &maxval) ) maxval = 200.0;

  dim0 = FN0_DIM;
  dim1 = FN1_DIM;
  if (d2flag)
  {
     if (frqdimname == S_NP)
     {
        if (datahead.status & S_SPEC)
        {
           Werrprintf("Data file must first be re-initialized with FID data\n");
           return(ERROR);
        }

        d2flag = FALSE;
        dim0 = FN0_DIM;
        dim1 = FN0_DIM;
     }
     else
     {
        if (ndflag == FOUR_D)
        {
           if (frqdimname & S_NP)
           {
              dim0 = FN0_DIM;
              if (frqdimname & S_NI)
                 dim1 = FN1_DIM;
              else if (frqdimname & S_NI2)
                 dim1 = FN2_DIM;
              else
                 dim1 = FN3_DIM;
           }
           else if (frqdimname & S_NI)
           {
              dim0 = FN1_DIM;
              if (frqdimname & S_NI2)
                 dim1 = FN2_DIM;
              else
                 dim1 = FN3_DIM;
           }
           else
           {
              dim0 = FN2_DIM;
              dim1 = FN3_DIM;
           }
        }
        else if (frqdimname & S_NP)
        {
           dim0 = FN0_DIM;
           dim1 = ( (frqdimname & S_NI2) ? FN2_DIM : FN1_DIM );
        }
        else if ( (~frqdimname) & S_NF )
        {
           dim0 = FN2_DIM;
           dim1 = FN1_DIM;
        }
        else
        {   
           Werrprintf("Invalid name for 2nd dimension\n");
           return(ERROR);
        }
     }
  }

/************************************************
*  Set the normalization, processing mode, and  *
*  display flags for either 1D or 2D data.      *
************************************************/

  if ( (r = P_getstring(CURRENT, "aig", aig, 1, 4)) )
  {
     if ( expdir_to_expnum(curexpdir) > 0 )
        P_err(r, "aig", ":");
     return(ERROR);
  }

  if ( (r = P_getstring(CURRENT, "dmg", dmg, 1, 4)) )
  {
     P_err(r, "dmg", ":");
     return(ERROR);
  }

  normflag = (aig[0] == 'n');
  dophase = doabsval = dopower = dophaseangle = dodbm = FALSE;
  dof1phase = dof1absval = dof1power = dof1phaseangle = FALSE;

  dophase = ((dmg[0] == 'p') && (dmg[1] == 'h'));
  if (!dophase)
  {
     doabsval = ((dmg[0] == 'a') && (dmg[1] == 'v'));
     if (!doabsval)
     {
        dopower = ((dmg[0] == 'p') && (dmg[1] == 'w'));
        if (!dopower)
        {
           dophaseangle = ((dmg[0] == 'p') && (dmg[1] == 'a'));
           if (!dophaseangle)
           {
        	   dodbm = ((dmg[0] == 'd') && (dmg[1] == 'b'));
        	   if (!dodbm)
        	   {
        	      Werrprintf("Invalid display mode for 1D spectral data");
        	      return(ERROR);
        	   }
           }
        } 
     } 
  }

  if (d2flag)
  {
     r = P_getstring(CURRENT, "dmg1", dmg1, 1, 5);
     if (!r)
     {
        r = ( (strcmp(dmg1, "ph1") != 0) && (strcmp(dmg1, "av1") != 0) &&
		  (strcmp(dmg1, "pwr1") != 0) );
     }

     if (r)
     { /* default to "dmg" */
        if ( (r = P_getstring(CURRENT, "dmg", dmg1, 1, 4)) )
        {
           P_err(r, "dmg", ":");
           disp_status("        ");
           return(ERROR);
        }
     }

     dof1phase = ((dmg1[0] == 'p') && (dmg1[1] == 'h'));
     if (!dof1phase) 
     {
        dof1absval = ((dmg1[0] == 'a') && (dmg1[1] == 'v')); 
        if (!dof1absval)
        { 
           dof1power = ((dmg1[0] == 'p') && (dmg1[1] == 'w')); 
           if (!dof1power)
           {
              dof1phaseangle = ((dmg1[0] == 'p') && (dmg1[1] == 'a')); 
              if (!dof1phaseangle)
              {
                 Werrprintf("Invalid F1 display mode for 2D spectral data");
                 return(ERROR); 
              }
           } 
        } 
     }
  }

  if (dis_setup)
  {
/************************************
*  Get current display parameters.  *
************************************/
     P_getreal(CURRENT,"vs",&vs,1);
     P_getreal(CURRENT,"vsproj",&vsproj,1);
     P_getreal(CURRENT,"vs2d",&vs2d,1);
     C_GETPAR("is", &is);
     C_GETPAR("io", &io);
     C_GETPAR("th", &th);
     C_GETPAR("vp", &vp);
     C_GETPAR("vo", &vo);
     C_GETPAR("ho", &ho);
     C_GETPAR("lvl", &lvl);
     C_GETPAR("tlt", &tlt);
  }

  if (d2flag)
  {

/*******************************************************
*  Check whether the datafile contains FID's, half-    *
*  transformed spectra, or fully transformed spectra.  *
*******************************************************/

     if ( (~datahead.status) & S_SECND)
     { /* not fully transformed 2D spectrum */
        dim1 = ( (datahead.status & S_NI2) ? SW2_DIM : SW1_DIM );
        if ( (~datahead.status) & S_SPEC)
           dim0 = SW0_DIM;
     }
  }
  else
  {
     dim0 = FN0_DIM;
     dim1 = FN1_DIM;
  }

  if (d2flag && revflag)
  {
     set_spec_display(HORIZ,dim1,VERT,dim0);
     set_display_label(HORIZ,revchar,VERT,normchar);
     set_spec_proc(HORIZ,dim1,REVDIR, VERT,dim0,NORMDIR);
  }
  else
  {
     set_spec_display(HORIZ,dim0,VERT,dim1);
     set_display_label(HORIZ,normchar,VERT,revchar);
     set_spec_proc(HORIZ,dim0,NORMDIR, VERT,dim1,REVDIR);
  }
  get_sfrq(HORIZ,&sfrq);
  get_sfrq(VERT,&sfrq1);
  get_cursor_pars(HORIZ,&cr,&delta);
  get_phase_pars(HORIZ,&rp,&lp);
  get_ref_pars(HORIZ,&sw,&rflrfp,&fn);
  if(sw<=0 || sw>maxval) {
	Winfoprintf("Error sw out of bounds %f: 0 to %f",sw,maxval);
	return(ERROR);
  }
  get_scale_axis(HORIZ,&axisHoriz);
  get_scale_axis(VERT,&axisVert);
  {
     double axis_scl;
     int    reversed;

     get_scale_pars(HORIZ,&sp,&wp,&axis_scl,&reversed);
     if (get_axis_freq(HORIZ))
     {
       if (reversed)
          set_sp_wp_rev(&sp, &wp, sw, fn/2, rflrfp);
       else
          set_sp_wp(&sp, &wp, sw, fn/2, rflrfp);
     }
     else
       set_sf_wf(&sp, &wp, sw, fn/2);
     UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
     UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  }

  if (d2flag)
  {
     get_phase_pars(VERT,&rp1,&lp1);
     setVertAxis();
     if(dim1==FN1_DIM && (sw1<=0 || sw1>maxval)) {
	Winfoprintf("Error sw1 out of bounds %f: 0 to %f",sw1,maxval);
	return(ERROR);
     }
     x0 = 1.0;
     if ( get_axis_freq(HORIZ) && get_axis_freq(VERT) )
     {
        if (((axisHoriz == 'c') || (axisHoriz == 'm') || (axisHoriz == 'u'))
         && ((axisVert == 'c') || (axisVert == 'm') || (axisVert == 'u')))
        {
	   if (axisHoriz == 'c') x0 =  1.0;
	   if (axisHoriz == 'm') x0 = 10.0;
	   if (axisHoriz == 'u') x0 = 1e4;
	   if (axisVert == 'c') x0 /= 1.0;
	   if (axisVert == 'm') x0 /= 10.0;
	   if (axisVert == 'u') x0 /= 1e4;
        }
     }
     aspect_ratio = (wp*sfrq1)/(x0*wp1*sfrq);
  }
  else
  {
     fn1 = fn;
     sw1 = 0.0;
     rflrfp1 = 0.0;
     aspect_ratio = 100.0;
  }
  normInt = 0;
  insval = 1.0;
  if (!P_getreal(CURRENT, "ins", &insval, 1))
  {
     vInfo  info;
     double tmp;

     if (!P_getreal(CURRENT, "insref", &tmp, 1))
     {
        P_getVarInfo(CURRENT,"insref",&info);
        if (!info.active)
           normInt = 1;
        else if (tmp > 0.0)
           insval /= tmp;
     }
  }
  if (insval <= 0.0)
     insval = 1.0;
  if (!normInt)
     insval /= (double) fn;

  normInt2 = 0;
  ins2val = 1.0;
  if (!P_getreal(CURRENT, "ins2", &ins2val, 1))
  {
     vInfo  info;
     double tmp;

     if (!P_getreal(CURRENT, "ins2ref", &tmp, 1))
     {
        P_getVarInfo(CURRENT,"ins2ref",&info);
        if (!info.active)
           normInt2 = 1;
        else if (tmp > 0.0)
           ins2val /= tmp;
     }
  }
  if (ins2val <= 0.0)
     ins2val = 1.0;
  if (!normInt2)
     ins2val /= ((double)fn * (double)fn1);

  return(COMPLETE);
}

double expf_dir(int direction)
{
   if (direction == HORIZ)
      return((double)(dnpnt-1)/(double)(npnt-1));
   else
      return((double)(dnpnt2-1)/(double)(npnt1-1));
}
   
extern int get_drawVscale();
extern int xcharpixels;

/*****************/
/* spec	flag to distinguish spectra from fids */
int exp_factors( int spec)
/*****************/
{
  double	hzpp,
		hzpp1;
#ifdef VNMRJ

  if(spec) {
	frame_set_pnt();
  } else {

  dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
  dnpnt  = (int)((double)(mnumxpnts-right_edge)*wc/wcmax);
  dfpnt2 = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
  dnpnt2 = (int)((double)(mnumypnts-ymin)*wc2/wc2max);

  }

#else
  dfpnt  = (int)((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
  dnpnt  = (int)((double)(mnumxpnts-right_edge)*wc/wcmax);
  dfpnt2 = (int)((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
  dnpnt2 = (int)((double)(mnumypnts-ymin)*wc2/wc2max);
#endif

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

  DPRINT("chart parameters\n");
  DPRINT4("dfpnt=%d, dnpnt=%d, dfpnt2=%d, dnpnt2=%d\n",
                  dfpnt,dnpnt,dfpnt2,dnpnt2);

  hzpp = sw/((double)(fn/2));
  // don't know why added 1. It may make a difference for small fn
  npnt = (int)(wp/hzpp + 0.01) + 1;
  if(npnt<1) npnt = 1;

  if (get_axis_freq(HORIZ))
  {
     int rev;

     rev = get_axis_rev(HORIZ);
     if (rev)
     {
        double axis_intercept;

        get_intercept(HORIZ,&axis_intercept);
        if ( axis_intercept > 0.0)
           fpnt = (int)((sw-wp+sp+rflrfp)/hzpp + 0.01);
        else
           fpnt = (int)((-sp-rflrfp)/hzpp + 0.01);
     }
     else
        fpnt = (int)((sw-sp-rflrfp-wp)/hzpp + 0.01);
  }
  else
  {
     fpnt = (int)(sp/hzpp + 0.01);
  }
  if (fpnt < 0)
    fpnt = 0;
  if (fpnt+npnt > fn/2)
  {
    npnt = fn/2 - fpnt;
    if (npnt < 5)
    {
       npnt=5;
       fpnt = fn/2 - 5;
    }
  }

  DPRINT3("npnt=%d, fpnt=%d, hzpp=%g\n",npnt,fpnt,hzpp);

  if (d2flag)
  {
     hzpp1 = sw1/((double)(fn1/2));
    // don't know why added 1. It may make a difference for small fn
     npnt1 = (int)(wp1/hzpp1 + 0.01) + 1;
     if(npnt1<1) npnt1 = 1;

     if (get_axis_freq(VERT))
     {
        int rev;

        rev = get_axis_rev(VERT);
        if (rev)
        {
           double axis_intercept;

           get_intercept(VERT,&axis_intercept);
           if ( axis_intercept > 0.0)
              fpnt1 = (int)((sw1-wp1+sp1+rflrfp1)/hzpp1 + 0.01);
           else
              fpnt1 = (int)((-sp1-rflrfp1)/hzpp1 + 0.01);
        }
        else
           fpnt1 = (int)((sw1-sp1-rflrfp1-wp1)/hzpp1 + 0.01);
     }
     else
        fpnt1 = (int)(sp1/hzpp1 + 0.01);
     if (fpnt1 < 0)
       fpnt1 = 0;
     if (fpnt1+npnt1 > fn1/2)
     {
       npnt1 = fn1/2 - fpnt1;
       if (npnt1 < 5)
       {
          npnt1=5;
          fpnt1 = fn1/2 - 5;
       }
     }

     DPRINT3("npnt1=%d, fpnt1=%d, hzpp1=%g\n",npnt1,fpnt1,hzpp1);
  }

  return COMPLETE;
}
    

/*-----------------------------------------------
|						|
|		dataheaders()/2			|
|						|
+----------------------------------------------*/
/*  getphasefile if TRUE, get phasefile header only */
/*  checkstatus	 if TRUE, check datafile status only */
int dataheaders(int getphasefile, int checkstatus)
{
  char	filepath[MAXPATH];
  int	e;


/******************************************
*  Get header information from datafile.  *
******************************************/

  D_allrelease();

  if ( (e = D_gethead(D_DATAFILE, &datahead)) )
  {

/*******************************************
*  If the datafile is not open, open with  *
*  the filehandler routines.               *
*******************************************/

     if (e == D_NOTOPEN)
     {
        if ( (e = D_getfilepath(D_DATAFILE, filepath, curexpdir)) )
        {
           D_error(e);
           return(ERROR);
        }

        e = D_open(D_DATAFILE, filepath, &datahead);     /* open the file */
     }

     if (e)
     {
        D_error(e);
        return(ERROR);
     }
  }

/***********************************************
*  Get header information from the phasefile.  *
***********************************************/

  if (getphasefile)
  {
     if ( (e = D_gethead(D_PHASFILE, &phasehead)) )
     {

/********************************************
*  If the phasefile is not open, open with  *
*  the filehandler routines.                *
*********************************************/

        if (e == D_NOTOPEN)
        {
           if ( (e = D_getfilepath(D_PHASFILE, filepath, curexpdir)) )
           {
              D_error(e);
              return(ERROR);
           }

           e = D_open(D_PHASFILE, filepath, &phasehead);  /* open the file */
        }

        if (e && removephasefile())
        {
           D_error(e);
           return(ERROR);
        }
     }
  }

/********************************************************
*  Check to insure that there is data in the datafile.  *
********************************************************/

  if (checkstatus)
  {
     if (!(datahead.status & S_DATA))
     {
        Wseterrorkey("aw");
        Werrprintf("Data are not processed");
	Wsetgraphicsdisplay("");
        return(ERROR);
     }
  }

  return(COMPLETE);
}


/*---------------------------------------
|					|
|	      block_pars()/2		|
|					|
+--------------------------------------*/
/* datascale	hypercomplex = 4, complex = 2, real = 1 */
static int block_pars(int blocks, int datascale)
{
  int	maxspecperblock;

/***************************************************
*  Compute block parameters from datahead.nblocks  *
*  and the appropriate Fourier Number set in the   *
*  GETSPECPARMS function of init2d().              *
***************************************************/

  if (d2flag)
  {

/****************************************************
*  Compute the maximum number of spectra per block  *
*  for the host configuration.                      *
****************************************************/

     maxspecperblock = (4*BUFWORDS/fn) * (bufferscale);
     maxspecperblock /= datascale;
     specperblock = fn1/(2*blocks);

     if (specperblock > maxspecperblock)
     {
        Werrprintf("2D blocksize too large for local configuration.  Reprocess on current host.");
        return(ERROR);
     }
  }
  else
  {
      specperblock = 1;
  }

  nblocks = blocks;
  pointsperspec = (fn/2) * datascale;

  c_first = 32767;
  c_last = 0;		/* mark no buffer in workspace */
  c_buffer = -1;
  c_block.head = NULL;
  c_block.data = NULL;

  return(COMPLETE);
}

/*  verify the value of fn matches that in the data file header.
    program is more complex than expected becuase of different
    sizes of the data file for real, complex and hypercomplex.

    For now, VNMR limits itself to hypercomplex as the most
    "complex" data format.  A fully phaseable 3D data set will
    be "3-way hypercomplex" or "hyper-hyper-complex", with 3
    complex pairs for each data point, but VNMR will not see
    such a data file.  The ft3d program is expected to eliminate
    the imaginary part of the dimension normal to the plane(s)
    it is writing out, either by phasing or absolute value.

    The second problem is what to use for the value of fn.  The
    VNMR program itself works with 1D traces and 2D planes, not
    with 3D blocks, at least not directly.  It only works with
    2D planes sliced out of the original 3D block.  The init2d
    program sets two numbers, fn and fn1.  Based on the direction,
    HORIZ or VERT of the NORMDIR (REVDIR is the alternate), it
    selects fn or fn1, as shown below.				*/

static int
verify_fn_data()
{
	int	fnval, norm_direction;

	if (d2flag) {
		norm_direction = get_direction( NORMDIR );
		if (norm_direction == HORIZ)
		  fnval = fn1;
		else
		  fnval = fn;
	}
	else
	  fnval = fn;

	if (datahead.status & S_HYPERCOMPLEX) {
		if (fnval*2 != datahead.np)
		  return( -1 );
	}
	else if (datahead.status & S_COMPLEX) {
		if (fnval != datahead.np)
		  return( -1 );
	}
	else {				/* neither bit set ==> REAL */
		if (fnval != datahead.np*2)
		  return( -1 );
	}

	return( 0 );
}

/*************/
int getMaxIndex(int argc, char *argv[], int retc, char *retv[])
/*************/
{
    (void) argc;
    (void) argv;
    if (retc > 0) {
		int numtraces=nblocks * specperblock;
		if(!d2flag){
			int numarrays=dim1count();
			numtraces=numtraces>numarrays?numtraces:numarrays;
		}
    	retv[0] = intString(numtraces);
    }
    RETURN;
}

/*---------------------------------------
|					|
|	     select_init()/8		|
|					|
+--------------------------------------*/
/* get_rev	if TRUE, look up REVFLAG only	*/
/* dis_setup	display program flag			*/
/* fdimname	frequency dimension			*/
int select_init(int get_rev, int dis_setup, int fdimname, int doheaders, int docheck2d,
                int dospecpars, int doblockpars, int dophasefile)
{
  int setstatus;
  int hypercomplex;

  setArraydis(0);

/***************************************************************
*  Obtain header information from the datafile and phasefile.  *
***************************************************************/

  if (doheaders)
  {
    if (dataheaders(1, 1))
       return(ERROR);
  }

  if (docheck2d)
  {
    if (check2d(get_rev))
       return(ERROR);		/* set d2flag, revflag, ni, and
				   dimension_name under certain
				   conditions */
  }
  else
  {
     dimension_name = 0;	/* use entered value for name of
				   dimension */
  }

  if (dospecpars)
  {
     if (!fdimname)
        fdimname = dimension_name;
     if (getspecparms(dis_setup, fdimname))
        return(ERROR);
  }

  if (dis_setup == 2)
  {
     if (setplotter())
        return(ERROR);
     saved_dis_setup = 2;
  }
  else if (dis_setup)
  {
     if (setdisplay())
        return(ERROR);
     saved_dis_setup = 1;
  }

/***************************************
*  Initialize the display parameters.  *
***************************************/

  if (dis_setup)
  {
     if (init2d_getchartparms(dospecpars))
        return(ERROR);
     if (exp_factors(1))
        return(ERROR);
     normalize = 1.0;		/* preliminary value */
  }

/*******************************************
*  Compute the relevant block parameters.  *
*******************************************/

  if (doblockpars)
  {
     if ( block_pars(datahead.nblocks, getdatatype(datahead.status)) )
        return(ERROR);
  }

/*******************************************************
*  Put in new phasefile header depending upon whether  *
*  data therein can be phased in any dimension and     *
*  whether REVFLAG is TRUE or FALSE.                   *
*******************************************************/

  if (dophasefile)
  {
     if ((~phasehead.status) & S_SPEC)	     /* Is the phasefile ok? */
     {
        setstatus = datahead.status & (~(S_HYPERCOMPLEX|S_COMPLEX));
        hypercomplex = (datahead.status & S_HYPERCOMPLEX);
        if (revflag)
        {
           new_phasefile(&phasehead, d2flag, nblocks, fn1, fn,
				setstatus, hypercomplex);
        }
        else
        {
           new_phasefile(&phasehead, d2flag, nblocks, fn, fn1,
				setstatus, hypercomplex);
        }
     }

     if (verify_fn_data()) {
        Werrprintf( "mismatch between fn parameter and phase file value" );
        return( ERROR );
     }

     p11_saveDisCmd();
  }

  return(COMPLETE);
}


/*---------------------------------------
|					|
|		init2d()/2		|
|					|
|   Initialize 2D parameters for data 	|
|   processing and/or display.		|
|					|
+--------------------------------------*/
int init2d(int get_rev, int dis_setup)
{
  if (select_init(get_rev, dis_setup, 0, 1, 1, 1, 1, 1))
     return(ERROR);
  return(COMPLETE);
}


/******************/
/* dis_setup	display program flag */
int initfid(int dis_setup)
/******************/
{

  setArraydis(0);

  if (init2d_getfidparms(dis_setup))
     return ERROR;

  if (dis_setup == 2)
  {
     if (setplotter())
        return ERROR;
     saved_dis_setup = 2;
  }
  else if (dis_setup)
  {
     if (setdisplay())
        return ERROR;
     saved_dis_setup = 1;
  }

  if (dis_setup)
  {
     if (init2d_getchartparms(1))
        return ERROR;
     if (exp_factors(0))
        return ERROR;
     normalize = 1.0;		/* preliminary value */
  }

  if (block_pars(dim1count(), COMPLEX/2))
     return ERROR;

  return COMPLETE;
}

int get_dis_setup()
{
	return( saved_dis_setup );
}
