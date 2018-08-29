/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************/

#include <stdio.h>
#include <string.h>

#include "data.h"
#include "ftpar.h"
#include "process.h"
#include "init2d.h"		/* Uses struct defs from data.h */
#include "graphics.h"
#include "group.h"
#include "vnmrsys.h"
#include "variables.h"
#include "pvars.h"
#include "sky.h"
#include "allocate.h"
#include "buttons.h"
#include "displayops.h"
#include "dscale.h"
#include "tools.h"
#include "fft.h"
#include "wjunk.h"
#include <math.h>

#define FALSE		0
#define TRUE		1
#define	COMPLETE	0
#define ERROR		1
#define FTNORM		5.0e-7
#define MINDEGREE	0.005
#define MAXSPECSIZE	8192
#define FID_CALIB	40000.0

#define NOTHING_ON	0
#define LB_ON		1
#define SB_ON		2
#define SBS_ON		3
#define GF_ON		4
#define GFS_ON		5
#define AWC_ON		6
#define SA_ON		7
#define SAS_ON		8

#define C_GETPAR(name, value)					\
		if ( (r = P_getreal(CURRENT, name, value, 1)) )	\
		{ P_err(r, name, ":"); return ERROR; }

#define P_GETPAR(name, value)					\
		if ( (r = P_getreal(PROCESSED, name, value, 1)) )	\
		{ P_err(r, name, ":"); return ERROR; }

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

extern int		menuflag,
			readwtflag,
			acqflag;
extern double		fpointmult;
extern double getfpmult(int fdimname, int ddr);
extern int getzfnumber(int ndpts, int fnsize);
extern int getzflevel(int ndpts, int fnsize);
extern int fnpower(int fni);
extern int  dim1count();
extern int setprocstatus(int ft_dimname, int full2d);
extern int  i_fid(dfilehead *fidhead, ftparInfo *ftpar);
extern int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead,
                int *lastfid);
extern void ls_ph_fid(char *lsfname, int *lsfval, char *phfname, double *phfval,
                      char *lsfrqname, double *lsfrqval);
extern int ds_checkinput(int argc, char *argv[], int *trace);
extern void driftcorrect_fid(float *outp, int np, int lsfid, int datamult);
extern double get_dsp_lp();
extern void rotate_fid(float *fidptr, double ph0, double ph1, int np, int datatype);
extern void getVLimit(int *vmin, int *vmax);
extern void fillPlotBox2D(); // use this because fillPlotBox() has a vertical shift. 

static char	*lpname,
		*rpname,
		*phfidname,
		*lsfidname,
		*lsfrqname,
		*dmgname;
static int	raw,
		ddrType,
		eraseraw,
		weighted,
		eraseweighted,
		spec,
		wtiErasespec,
		next,
		specflag,
		current_rawfid,
		maxspecIndex,
		interferogram,
		on_status,
		hypercomplex,
		lsfid_0,
		cfvalue,
		np,
		ifn,
		pwr,
		procstatus,
                do_sa,
                do_sas,
		ptype,
		fpnt_sp,
		npnt_sp,
		miny[3],
		maxy[3];
static float	dispcalib,
		expf_fid,
		*rawfid = NULL,		/* pointer to raw, not weighted fid */
		*wtfunc = NULL,		/* pointer to weighting function */
		*spectrum = NULL;	/* pointer to spectrum */
static double	at,
		vf,
		old_vf,
		old_vs,
                dspar_lp,
		phfid_0,
		lsfrq_0;

static struct wtparams	wtp;

static void activate_sa();
static void activate_sb();
static void activate_gf();
static void wti_displayparms1();
static void wti_displayparms2();
static void ParamValue(int line, int column, int active, int turned_on,
            double value);
static void update_vf_par();
static void update_vs_par();

/*----------------------------------------------
|						|
|	      get_interferogram()/7		|
|						|
|   This function obtains either the complex	|
|   or hypercomplex t1 interferogram.		|
|						|
+----------------------------------------------*/
static int get_interferogram(int trace, float *buffer, int nnp, int nfn, int nlsfid,
                             double nphfid, double nlsfrq)
{
   int			res,
			datasize = COMPLEX;	/* initialized to complex */
   register int		i;
   register float	*b,
			*tmp;
   dpointers		datablock;


   c_buffer = trace/specperblock;
   c_first  = c_buffer*specperblock;
   if ( (res = D_getbuf(D_DATAFILE, datahead.nblocks, c_buffer, &datablock)) )
   {
      D_error(res);
      return ERROR;
   }

   if ((~datablock.head->status) & S_DATA)
   {
      Werrprintf("no 2D interferogram in file");
      return(ERROR);
   }

   if (hypercomplex)
      datasize *= 2;    /* hypercomplex interferograms */

/******************
*  Adjust "nnp".  *
******************/

   if (nlsfid < 0)
   {
      nnp = ( (nfn < (nnp - nlsfid)) ? (nfn + nlsfid) : nnp );
      if (nnp < 2)
      {
         Werrprintf("lsfid is too large in magnitude");
         return(ERROR);
      }
   }
   else
   {
      nnp = ( (nfn < nnp) ? nfn : nnp );
      if (nlsfid >= nnp)
      {
         Werrprintf("lsfid is too large in magnitude");
         return(ERROR);
      }
   }

/***************************************************
*  Depends on "pointsperspec" being the number of  *
*  total points per "F2" spectrum; only a complex  *
*  t1 interferogram is retained after the "movmem" *
*  statement.					   *
***************************************************/

   if ( (datasize == HYPERCOMPLEX) && (datablock.head->mode & NP_PHMODE) )
   {
      int	r,
		npf2;
      double	rpf2,
		lpf2,
		tmp2;

      C_GETPAR("rp", &rpf2);
      C_GETPAR("lp", &lpf2);
      P_GETPAR("fn", &tmp2);
      npf2 = (int) (tmp2 + 0.5);

      movmem((char *) (datablock.data + (trace - c_first)*pointsperspec),
		(char *)buffer, 2*nnp*sizeof(float), 1, 4);

      if ( (fabs(rpf2) > MINDEGREE) || (fabs(lpf2) > MINDEGREE) )
      { /* phase rotate the hypercomplex t1 interferogram */
         rotate4(buffer, nnp/2, 0.0, 0.0, lpf2, rpf2, npf2/2,
		   trace, TRUE);
      }

      movmem((char *)buffer, (char *)buffer, nnp*sizeof(float), 2, 4);
   }
   else
   {
      movmem((char *) (datablock.data + (trace - c_first)*pointsperspec),
		(char *)buffer, nnp*sizeof(float), 1, 4);
   }

/***************************************
*  Left-shift, phase-rotate, and then  *
*  zero-fill the FID data.             *
***************************************/

   if (nlsfid < 0)
   {
      b = buffer + nnp;
      tmp = buffer + nnp - nlsfid;
      for (i = 0; i < nnp; i++)
         *(--tmp) = *(--b);

      for (i = 0; i < (-1)*nlsfid; i++)
         *(--tmp) = 0.0;
   }
   else if (nlsfid > 0)
   {
      tmp = buffer;
      b = buffer + nlsfid;
      for (i = 0; i < (nnp - nlsfid); i++)
         *tmp++ = *b++;
   }

   nnp -= nlsfid;
   if (ifn > nnp)
      zerofill(buffer + nnp, ifn - nnp);  /* for display of interferogram */

   if ( (fabs(nphfid) > MINDEGREE) ||
	(fabs(nlsfrq) > 1e-20) )
   {
      rotate_fid(buffer, nphfid, nlsfrq, nnp, COMPLEX);
   }

   D_release(D_DATAFILE, c_buffer);
   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		wti_showwtfunc()/0		|
|						|
|   This function calculates and displays the	|
|   weighting vector.				|
|						|
+----------------------------------------------*/
static void wti_showwtfunc()
{
   int			wtsize,
        	        npts,
			pntskip,
			realbit,
			realdata;
   double		wtvs;
   int vmin = 1;
   int vmax = mnumypnts - 3;
   getVLimit(&vmin, &vmax);


   realbit = ( (interferogram & S_NP) ? REAL_t1 : REAL_t1 );
   realdata = (procstatus & realbit);
   pntskip = 1;
   npts = ( (npnt > ifn/2) ? ifn/2 : npnt );
   wtsize = ifn/2;

   if (realdata)
   {
      wtsize *= 2;
      pntskip *= 2;
   }

   if (interferogram & S_NP)
   {
      fpointmult = getfpmult(S_NP, ddrType);
   }
   else
   {
      fpointmult = ( (interferogram & (S_NF | S_NI)) ? getfpmult(S_NI, 0) :
			getfpmult(S_NI2, 0) );
   }

   if (init_wt2(&wtp, wtfunc, wtsize, realdata, interferogram,
			fpointmult, readwtflag))
   {
      disp_status("           ");
      skyreleaseAllWithId("wti");
      return;
   }

   readwtflag = TRUE;
   wtvs = maxy[1]-miny[1]-2;

   fillPlotBox2D();
   calc_ybars(wtfunc, pntskip, wtvs, dfpnt, dnpnt, npts,
		miny[1]+1, next);
   displayspec(dfpnt, dnpnt, 0, &next, &weighted, &eraseweighted,
                maxy[1]-1, miny[1], INT_COLOR);
}


/*-----------------------------------------------
|						|
|		wti_showrawfid()/0		|
|						|
|   This function displays the raw time-domain 	|
|   data during a WTI operation.		|
|						|
+----------------------------------------------*/
static int wti_showrawfid()
{
   int		r,
		lastfid;
   double	vf1;
   double	rx;
   dfilehead	fidhead;
   ftparInfo	ftpar;
   vInfo	info;


   lastfid = maxspecIndex;
   if ( (r = P_getreal(CURRENT, "vf", &vf, 1)) )
   {
      P_err(r,"current ","vf");
      Wturnoff_buttons();
      return(ERROR);
   }

/******************************************
*  Allows 'nf'-arrayed 1D experiments to  *
*  be handled within WTI.                 *
******************************************/

   if (interferogram & S_NP)
   {
      if ( (r = D_gethead(D_USERFILE, &fidhead)) )
      {
         char	fidfilepath[MAXPATHL];
         int	e;

         
         e = D_getfilepath(D_USERFILE, fidfilepath, curexpdir);

         if ( (r == D_NOTOPEN) && (!e) )
         {
            if ( (r = D_open(D_USERFILE, fidfilepath, &fidhead)) )
            {
               D_error(r);
               disp_status("           ");
               skyreleaseAllWithId("wti");
               ABORT;
            }
         }
         else
         {
            D_error( (e ? e : r) );
            disp_status("           ");
            skyreleaseAllWithId("wti");
            ABORT;
         }
      }

      cfvalue = 1;
      if (fidhead.ntraces > 1)
      {
         if ( (r = P_getVarInfo(CURRENT, "cf", &info)) )
         {
            P_err(r, "info?", "cf:");
            return(ERROR);
         }

         if (info.active)
         {
            if ( (r = P_getreal(CURRENT, "cf", &rx, 1)) )
            {
               P_err(r, "cf", ":");
               return(ERROR);
            }

            cfvalue = (int) (rx + 0.5);	 /* starts at 1 internally */
         }

         if ( (cfvalue < 1) || (cfvalue > fidhead.ntraces) )
         {
            Werrprintf("parameter 'cf' must be between 1 and %d",
		   fidhead.ntraces);
            return(ERROR);
         }
      }
   }

/***************************************
*  Read in FID or interferogram data.  *
***************************************/

   vf1 = vf * dispcalib;
   if (interferogram & S_NP)
      vf1 /= FID_CALIB;
   if (old_vf != vf)
      update_vf_par();

   if (current_rawfid != (specIndex-1))
   {
      if (interferogram & (S_NF|S_NI|S_NI2))
      {
         if (get_interferogram(specIndex-1, rawfid, np, ifn, lsfid_0,
		phfid_0, lsfrq_0))
         {
            return(ERROR);
         }
         dspar_lp = 0.0;
      }
      else
      {
         ftpar.arraydim = dim1count();
         ftpar.cf = cfvalue;
         ftpar.nf = fidhead.ntraces;
         ftpar.np0 = np;
         ftpar.fn0 = ifn;
         ftpar.lpval = 0.0;
         ftpar.lsfid0 = lsfid_0;
         ftpar.phfid0 = phfid_0;
         ftpar.lsfrq0 = lsfrq_0;
         ftpar.offset_flag = FALSE;
         ftpar.zeroflag = FALSE;
         ftpar.t2dc = FALSE;
         ftpar.dpflag = (fidhead.status & S_32);
         ddrType = fidhead.status & S_DDR;
         
         if ( getfid(specIndex-1, rawfid, &ftpar, &fidhead, &lastfid) )
         {
            return(ERROR);
         }
         dspar_lp = get_dsp_lp();

         if ( lastfid == (specIndex - 1) )
         {
            Werrprintf("FID number %d does not exist\n", lastfid + 1);
            return(ERROR);
         }

         if (lsfid_0)
         {
            if ( (np - lsfid_0) < ifn )
               zerofill(rawfid + np - lsfid_0, ifn - np + lsfid_0);
         }

         if (!d2flag)
         {
            if ( (np - lsfid_0) < ifn )
            {
               driftcorrect_fid(rawfid, np/2, lsfid_0/2, COMPLEX);
            }
            else
            {
               driftcorrect_fid(rawfid, (ifn + lsfid_0)/2, lsfid_0/2,
			COMPLEX);
            }
         }
      }
   }

/******************************
*  Display time-domain data.  *
******************************/

   if ((current_rawfid != specIndex-1) || (old_vf != vf))
   {
      int	npts;
      int vmin = 1;
      int vmax = mnumypnts - 3;
      getVLimit(&vmin, &vmax);
      fillPlotBox2D();

      npts = ( (npnt > ifn/2) ? ifn/2 : npnt );
      expf_fid = (float)(dnpnt - 1)/(float)(npts - 1);

      fid_ybars(rawfid, vf1, dfpnt, dnpnt, npts, (miny[0]+maxy[0])/2,
			next, 0);
      displayspec(dfpnt, dnpnt, 0, &next, &raw, &eraseraw,
                        maxy[0]-1, miny[0], FID_COLOR);
   }

   current_rawfid = specIndex-1;
   old_vf = vf;

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|	       wti_showspectrum()/1		|
|						|
|   This function displays the spectrum for	|
|   WTI.  It also performs the FFT to calcu-	|
|   late the spectrum when necessary.		|
|						|
+----------------------------------------------*/
static int wti_showspectrum(int newft)
{
   char		*enddisplymes;
   int		r,
		realbit,
		realdata,
		zflevel,
		zfnumber,
		lsfidx,
		npx,
		npadj,
		wti_dophase,
		wti_doabsval;
   double	vs1;
   float	scale_ft;
   int vmin = 1;
   int vmax = mnumypnts - 3;
   getVLimit(&vmin, &vmax);


   if (!specflag)
      return(COMPLETE);

   if ( (r = P_getreal(CURRENT, "vs", &vs, 1)) )
   {
      P_err(r, "current ", "vs");
      Wturnoff_buttons();
      return(ERROR);
   }

   if ( (r = P_getreal(CURRENT, "vp", &vp, 1)) )
   {
      P_err(r, "current ", "vp");
      Wturnoff_buttons();
      return(ERROR);
   }

   if ( (r = P_getreal(CURRENT, lpname, &lp, 1)) )
   {
      P_err(r, "current ", lpname);
      Wturnoff_buttons();
      return(ERROR);
   }

   if ( (r = P_getreal(CURRENT, rpname, &rp, 1)) )
   {
      P_err(r, "current ", rpname);
      Wturnoff_buttons();
      return(ERROR);
   }

/*******************************
*  Adjust "npx" and "lsfidx".  *
*******************************/
 
   if (interferogram & S_NP)
   {
      lsfidx = lsfid_0;
      if (lsfidx < 0)
      {
         npx = ( (ifn < (np - lsfidx)) ? (ifn + lsfidx) : np );
      }
      else
      {
         npx = ( (ifn < np) ? ifn : np );
      }
   }
   else
   {
      lsfidx = lsfid_0;
      npx = ( (ifn < (np - lsfidx)) ? (ifn + lsfidx) : np );
   }

   if (lsfidx < 0)
   {
      if (npx < 2)
      {
         Werrprintf("lsfid is too large in magnitude");
         return(ERROR);
      }
   }
   else
   {
      if (lsfidx >= npx)
      {
         Werrprintf("lsfid is too large in magnitude");
         return(ERROR);
      }
   }


   npadj = npx - lsfidx;
   zflevel = getzflevel(npadj, ifn);
   zfnumber = getzfnumber(npadj, ifn);

   vs1 = vs * dispcalib;
   if (old_vs != vs)
      update_vs_par();

   if (newft)
   {
      if (interferogram & (S_NF|S_NI|S_NI2))
      {
         if (interferogram & S_NI2)
         {
            disp_status("WTI2 WFT");
            enddisplymes = "WTI2    ";
         }
         else
         {
            disp_status("WTI1 WFT");
            enddisplymes = "WTI1     ";
         }

         scale_ft = 1.0;
         realbit = REAL_t1;
         wti_dophase = dof1phase;
         wti_doabsval = dof1absval;
      }
      else
      {
         disp_status("WTI  WFT");
         enddisplymes = "WTI     ";
         scale_ft = FTNORM;
         realbit = REAL_t2;
         if (d2flag)
         {
            wti_dophase = dophase;
            wti_doabsval = doabsval;
         }
         else
         {
            wti_dophase = dophase;
            wti_doabsval = doabsval;
         }
      }

      realdata = (procstatus & realbit);
      if (realdata)
      {
         vvvrmult(wtfunc, 1, rawfid, 1, spectrum, 1, npadj);
      }
      else
      {
         vvvrmult(wtfunc, 1, rawfid, 2, spectrum, 2, npadj/2);
         vvvrmult(wtfunc, 1, rawfid + 1, 2, spectrum + 1, 2, npadj/2);
      }

      if (interferogram & (~S_NP))
      {
         if (!ptype)
            negateimaginary(spectrum, npadj/2, COMPLEX);
         if (realdata)
            preproc(spectrum, npadj/2, COMPLEX);
      }

      if (zfnumber)
         zerofill(spectrum + npadj, zfnumber);

      fft(spectrum, ifn/2, pwr, zflevel, COMPLEX, COMPLEX, -1.0,
		scale_ft, (npadj + zfnumber)/2);
      if (realdata)
         postproc(spectrum, ifn/2, COMPLEX);
 
      if (dspar_lp > 1.0)
         rotate2_center(spectrum, ifn/2, dspar_lp*
            (double) (ifn/2) / (double) (ifn/2-1), (double) 0.0);
      if (wti_dophase)
      {
         if ( (fabs(rp) > MINDEGREE) || (fabs(lp) > MINDEGREE) )
         {
            phase2(spectrum, spectrum, ifn/2, lp, rp);
         }
         else
         {
            movmem((char *)spectrum, (char *)spectrum, (ifn/2)*sizeof(float),
                   COMPLEX, sizeof(float));
         }
      }
      else if (wti_doabsval)
      {
         absval2(spectrum, spectrum, ifn/2, COMPLEX, 1, 1, FALSE);
      }
      else
      {
         pwrval2(spectrum, spectrum, ifn/2, COMPLEX, 1, 1, FALSE);
      }

      disp_status(enddisplymes);
   }

   fillPlotBox2D();
   calc_ybars(spectrum+fpnt_sp, 1, vs1, dfpnt, dnpnt, npnt_sp,
             miny[2]+(maxy[2]-miny[2])/16 + (int)(dispcalib * vp), next);
   displayspec(dfpnt, dnpnt, 0, &next, &spec, &wtiErasespec,
             maxy[2]-1, miny[2], SPEC_COLOR);

   old_vs = vs;
   return(COMPLETE);
}


/**************/
void erase_spectrum()
/**************/
{ if (wtiErasespec)
    { color(BACK);
      amove(dfpnt,miny[2]);
      box(dnpnt-1,maxy[2]-miny[2]);
    }
  color(CYAN);
  wtiErasespec = 0;
}

/******************/
static void wti_turnoff()
/******************/
{ Wgmode();
  Wturnoff_mouse();
  ParameterLine(0,0,PARAM_COLOR,"");
  ParameterLine(1,0,PARAM_COLOR,"");
  ParameterLine(2,0,PARAM_COLOR,"");
  endgraphics();
  exit_display();
  skyreleaseAllWithId("wti");
  rawfid = NULL;
  wtfunc = NULL;
  spectrum = NULL;
  D_allrelease();
  Wsetgraphicsdisplay("");
  disp_status("        ");
}

/***************/
static void incr_fid()
/***************/
{ Wgmode();
  specIndex++;
  if (specIndex>maxspecIndex) specIndex = maxspecIndex;
  disp_specIndex(specIndex);
  if (wti_showrawfid())
  {
     disp_status("           ");
     skyreleaseAllWithId("wti");
     return;
  }

  if (wti_showspectrum(TRUE))
  {
     disp_status("             ");
     skyreleaseAllWithId("wti");
     return;
  }

  Wsetgraphicsdisplay("wti");
}

/***************/
static void decr_fid()
/***************/
{ Wgmode();
  specIndex--;
  if (specIndex<1) specIndex = 1;
  disp_specIndex(specIndex);
  if (wti_showrawfid())
  {
     disp_status("           ");
     skyreleaseAllWithId("wti");
     return;
  }

  if (wti_showspectrum(TRUE))
  {
     disp_status("             ");
     skyreleaseAllWithId("wti");
     return;
  }

  Wsetgraphicsdisplay("wti");
}

/******************/
static int wti_new_wt()
/******************/
{
  if (init_wt1(&wtp, interferogram))
  {
     disp_status("              ");
     skyreleaseAllWithId("wti");
     ABORT;
  }

  Wgmode();
  wti_displayparms2();
  wti_showwtfunc();
  if (wti_showspectrum(TRUE))
  {
     disp_status("             ");
     skyreleaseAllWithId("wti");
     ABORT;
  }

  Wsetgraphicsdisplay("wti");
  RETURN;
}

/******************/
static void activate_lb()
/******************/
{
  if (wtp.lb_active && on_status==LB_ON)
    { wtp.lb_active = 0;
      P_setactive(CURRENT,wtp.lbname,0);
      on_status = NOTHING_ON;
      if (wtp.sb_active) activate_sb();
      else if (wtp.sa_active) activate_sa();
      else if (wtp.gf_active) activate_gf();
      else
        wti_new_wt();
    }
  else
    {
      if (wtp.lb_active && on_status!=NOTHING_ON)
      {
        on_status = LB_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.lb_active = 1;
        on_status = LB_ON;
        P_setactive(CURRENT,wtp.lbname,1);
        wti_new_wt();
      }
    }
#ifdef VNMRJ
      appendvarlist(wtp.lbname);
#endif 
}

/******************/
static void activate_sa()
/******************/
{
  if (wtp.sa_active && on_status==SA_ON)
    { wtp.sa_active = 0;
      P_setactive(CURRENT,wtp.saname,0);
      wtp.sas_active = 0;
      P_setactive(CURRENT,wtp.sasname,0);
      on_status = NOTHING_ON;
      if (wtp.lb_active) activate_lb();
      else if (wtp.sb_active) activate_sb();
      else if (wtp.gf_active) activate_gf();
      else
        wti_new_wt();
    }
  else
      if (wtp.sa_active && on_status!=NOTHING_ON)
      {
        on_status = SA_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.sa_active = 1;
        on_status = SA_ON;
        P_setactive(CURRENT,wtp.saname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.saname);
#endif 
}

/******************/
static void activate_sas()
/******************/
{
  if (wtp.sas_active && (on_status==SAS_ON))
    { wtp.sas_active = 0;
      P_setactive(CURRENT,wtp.sasname,0);
      on_status = NOTHING_ON;
      if (wtp.sa_active) activate_sa();
      else if (wtp.lb_active) activate_lb();
      else if (wtp.sb_active) activate_sb();
      else if (wtp.gf_active) activate_gf();
      else wti_new_wt();
    }
  else
      if (wtp.sas_active && on_status!=NOTHING_ON)
      {
        on_status = SAS_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.sas_active = 1;
        on_status = SAS_ON;
        P_setactive(CURRENT,wtp.sasname,1);
        wtp.sa_active = 1;
        P_setactive(CURRENT,wtp.saname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.sasname);
#endif 
}

/******************/
static void activate_sb()
/******************/
{
  if (wtp.sb_active && on_status==SB_ON)
    { wtp.sb_active = 0;
      P_setactive(CURRENT,wtp.sbname,0);
      wtp.sbs_active = 0;
      P_setactive(CURRENT,wtp.sbsname,0);
      on_status = NOTHING_ON;
      if (wtp.lb_active) activate_lb();
      else if (wtp.sa_active) activate_sa();
      else if (wtp.gf_active) activate_gf();
      else
        wti_new_wt();
    }
  else
      if (wtp.sb_active && on_status!=NOTHING_ON)
      {
        on_status = SB_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.sb_active = 1;
        on_status = SB_ON;
        P_setactive(CURRENT,wtp.sbname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.sbname);
#endif 
}

/******************/
static void activate_sbs()
/******************/
{
  if (wtp.sbs_active && (on_status==SBS_ON))
    { wtp.sbs_active = 0;
      P_setactive(CURRENT,wtp.sbsname,0);
      on_status = NOTHING_ON;
      if (wtp.sb_active) activate_sb();
      else if (wtp.lb_active) activate_lb();
      else if (wtp.sa_active) activate_sa();
      else if (wtp.gf_active) activate_gf();
      else wti_new_wt();
    }
  else
      if (wtp.sbs_active && on_status!=NOTHING_ON)
      {
        on_status = SBS_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.sbs_active = 1;
        on_status = SBS_ON;
        P_setactive(CURRENT,wtp.sbsname,1);
        wtp.sb_active = 1;
        P_setactive(CURRENT,wtp.sbname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.sbsname);
#endif 
}

/******************/
static void activate_gf()
/******************/
{
  if (wtp.gf == 0.0)
     wtp.gf = 0.1;
  if (wtp.gf_active && (on_status==GF_ON))
    { wtp.gf_active = 0;
      P_setactive(CURRENT,wtp.gfname,0);
      wtp.gfs_active = 0;
      P_setactive(CURRENT,wtp.gfsname,0);
      on_status = NOTHING_ON;
      if (wtp.lb_active) activate_lb();
      else if (wtp.sb_active) activate_sb();
      else if (wtp.sa_active) activate_sa();
      else wti_new_wt();
    }
  else
      if (wtp.gf_active && on_status!=NOTHING_ON)
      {
        on_status = GF_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.gf_active = 1;
        on_status = GF_ON;
        P_setactive(CURRENT,wtp.gfname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.gfname);
#endif 
}

/******************/
static void activate_gfs()
/******************/
{
  if (wtp.gfs_active && (on_status==GFS_ON))
    { wtp.gfs_active = 0;
      P_setactive(CURRENT,wtp.gfsname,0);
      on_status = NOTHING_ON;
      if (wtp.gf_active) activate_gf();
      else if (wtp.lb_active) activate_lb();
      else if (wtp.sb_active) activate_sb();
      else if (wtp.sa_active) activate_sa();
      else wti_new_wt();
    }
  else
      if (wtp.gfs_active && on_status!=NOTHING_ON)
      {
        on_status = GFS_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.gfs_active = 1;
        on_status = GFS_ON;
        P_setactive(CURRENT,wtp.gfsname,1);
        wtp.gf_active = 1;
        P_setactive(CURRENT,wtp.gfname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.gfsname);
#endif 
}

/*******************/
static void activate_awc()
/*******************/
{
  if (wtp.awc_active && (on_status==AWC_ON))
    { wtp.awc_active = 0;
      P_setactive(CURRENT,wtp.awcname,0);
      on_status = NOTHING_ON;
      if (wtp.lb_active) activate_lb();
      else if (wtp.gf_active) activate_gf();
      else if (wtp.sb_active) activate_sb();
      else if (wtp.sa_active) activate_sa();
      else wti_new_wt();
    }
  else
      if (wtp.awc_active && on_status!=NOTHING_ON)
      {
        on_status = AWC_ON;
        wti_displayparms2();
      }
      else
      {
        wtp.awc_active = 1;
        on_status = AWC_ON;
        P_setactive(CURRENT,wtp.awcname,1);
        wti_new_wt();
      }
#ifdef VNMRJ
      appendvarlist(wtp.awcname);
#endif 
}

/**********************************/
static int wti_mouse(int butnum, int updown, int x, int y0)
/**********************************/
{ float time;
  int y;
  char param[8];

  y = usercoordinate(y0);
  if (updown) return 0;
  Wgmode();
  if (butnum==0)	/* left button, weighting parameters */
    { if ((y<=miny[1]) || (y>=maxy[1])) 
        return 0;	/* outside box */
      param[0] = '\0';
      time = (x-dfpnt)/(expf_fid*wtp.sw);
      if (time>999.999) time = 999.999;
      if (time<-999.999) time = -999.999;
      switch (on_status)
        { case LB_ON:  { if (time<0)
                           { wtp.lb = -wtp.lb;
                           }
                         else
                           { if (wtp.lb<0) time = -time;
                             wtp.lb = 0.318/time;
                           }
                         P_setreal(CURRENT,wtp.lbname,wtp.lb,0);
                         P_getreal(CURRENT,wtp.lbname,&wtp.lb,1);
                         ParamValue(2,18,wtp.lb_active,1,wtp.lb);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.lbname);
                         break;
                       }
          case SA_ON:  {
                         wtp.sa = (int) ((x-dfpnt)/expf_fid);
                         if (wtp.sas_active)
                            wtp.sa -= wtp.sas;
                         if ( wtp.sa > npnt)
                            wtp.sa = npnt;
                         if ( wtp.sa < 8)
                            wtp.sa = 8;
                         P_setreal(CURRENT,wtp.saname,wtp.sa,0);
                         P_getreal(CURRENT,wtp.saname,&wtp.sa,1);
                         ParamValue(2,72,wtp.sa_active,1,wtp.sa);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.saname);
                         break;
                       }
          case SAS_ON: {
                         wtp.sas = (int) ((x-dfpnt)/expf_fid);
                         if ( ( wtp.sas + wtp.sa) > npnt)
                            wtp.sas = npnt -  wtp.sa;
                         if ( wtp.sas < 0)
                            wtp.sas = 0;
                         P_setreal(CURRENT,wtp.sasname,wtp.sas,0);
                         P_getreal(CURRENT,wtp.sasname,&wtp.sas,1);
                         ParamValue(2,81,wtp.sas_active,1,wtp.sas);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.sasname);
                         break;
                       }
          case SB_ON:  { if (time<0)
                           { wtp.sb = -wtp.sb;
                           }
                         else
                           { if (wtp.sbs_active) 
                               time -= wtp.sbs;
                             if (wtp.sb<0) time = -time;
                             wtp.sb = time;
                           }
                         P_setreal(CURRENT,wtp.sbname,wtp.sb,0);
                         P_getreal(CURRENT,wtp.sbname,&wtp.sb,1);
                         ParamValue(2,27,wtp.sb_active,1,wtp.sb);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.sbname);
                         break;
                       }
          case SBS_ON: { wtp.sbs = time - fabs(wtp.sb);
                         P_setreal(CURRENT,wtp.sbsname,wtp.sbs,0);
                         P_getreal(CURRENT,wtp.sbsname,&wtp.sbs,1);
                         ParamValue(2,36,wtp.sbs_active,1,wtp.sbs);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.sbsname);
                         break;
                       }
          case GF_ON:  { if (wtp.gfs_active)
                           { time -= wtp.gfs;
			     if (time<0) time = -time;
                           }
                         wtp.gf = time;
                         if (wtp.gf == 0.0)
                           wtp.gf = 0.1;
                         P_setreal(CURRENT,wtp.gfname,wtp.gf,0);
                         P_getreal(CURRENT,wtp.gfname,&wtp.gf,1);
                         ParamValue(2,45,wtp.gf_active,1,wtp.gf);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.gfname);
                         break;
                       }
          case GFS_ON: { wtp.gfs = time;
                         P_setreal(CURRENT,wtp.gfsname,wtp.gfs,0);
                         P_getreal(CURRENT,wtp.gfsname,&wtp.gfs,1);
                         ParamValue(2,54,wtp.gfs_active,1,wtp.gfs);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.gfsname);
                         break;
                       }
          case AWC_ON: { wtp.awc = (y-(float)(miny[1]+maxy[1])/2.0) /
                                               ((float)(maxy[1]-miny[1])/2.0);
                         P_setreal(CURRENT,wtp.awcname,wtp.awc,0);
                         P_getreal(CURRENT,wtp.awcname,&wtp.awc,1);
                         ParamValue(2,63,wtp.awc_active,1,wtp.awc);
                         wti_showwtfunc();
                         if (wti_showspectrum(TRUE))
                         {
                            disp_status("             ");
                            skyreleaseAllWithId("wti");
                            ABORT;
                         }
			 strcpy(param,wtp.awcname);
                         break;
                       }
        }
#ifdef VNMRJ
        if (strlen(param) > 0)
	   appendvarlist(param);
#endif 
    }
  else if (butnum==1)
    { if ((x<dfpnt) || (x>=dfpnt+dnpnt))
        return 0;
      if (y<=miny[0])
        return 0;
      else if (y<maxy[0])
        { /* field for raw fid */
          vf *= vs_mult(x,y,mnumypnts,(miny[0]+maxy[0])/2,dfpnt,dnpnt,raw);
          update_vf_par();
          if (wti_showrawfid())
          {
             disp_status("           ");
             skyreleaseAllWithId("wti");
             ABORT;
          }
        }
      else if (y<maxy[1])
        { /* do nothing in field for weighting function */
        }
      else if (y<maxy[2])
        { /* field for spectrum */
          if (!specflag)
            { specflag=1;
              if (wti_showspectrum(TRUE))
              {
                 disp_status("             ");
                 skyreleaseAllWithId("wti");
                 ABORT;
              }
            }
          else
            { vs *= vs_mult(x,y,mnumypnts,miny[2]+(maxy[2]-miny[2])/16,
                                                        dfpnt,dnpnt,spec);
              update_vs_par();
              if (wti_showspectrum(FALSE))
              {
                 disp_status("             ");
                 skyreleaseAllWithId("wti");
                 ABORT;
              }
            }
        }
    }
  else if (butnum==2)
    { if (specflag)
        { erase_spectrum();
          specflag = 0;
        }
      else
        { specflag = 1;
          if (wti_showspectrum(TRUE))
          {
             disp_status("             ");
             skyreleaseAllWithId("wti");
             ABORT;
          }
        }
    }
    return(0);
}

/*****************/
static void wti_return()
/*****************/
{
  Wturnoff_buttons();
  if (menuflag)
    execString("menu\n");
}


/*---------------------------------------
|					|
|	    wti_buttons()/0		|
|					|
|   This function produces the button	|
|   menu for WTI.			|
|					|
+--------------------------------------*/
static void wti_buttons()
{
#ifdef VNMRJ

  char  *labelptr[11];
  PFV    cmd[11];
  double dval;
  int    i=0;

  if ((P_getreal(CURRENT,"arraydim",&dval,1))!=0)
     dval = 0.0;

/* could use locate_iconfile() in text.c to locate icon; sprintf("%d"); 
      strcat(gifname); strcat(label); strcat(wtp.wtname); */

  if (dval > 1.5)
  {
     labelptr[i] = "3 nextfid.gif Next Fid";          cmd[i] = incr_fid; i++;
     labelptr[i] = "3 prevfid.gif Previous Fid";      cmd[i] = decr_fid; i++;
  }
  if (interferogram & S_NI2)
  {
     labelptr[i] = "3 lb.gif Line Broadening (lb2)";          cmd[i] = activate_lb;  i++;
     labelptr[i] = "3 sb.gif Sinebell (sb2)";                 cmd[i] = activate_sb;  i++;
     labelptr[i] = "3 sbs.gif Sinebell Shift (sbs2)";         cmd[i] = activate_sbs; i++;
     labelptr[i] = "3 gf.gif Gaussian (gf2)";                 cmd[i] = activate_gf;  i++;
     labelptr[i] = "3 gfs.gif Gaussian Shift (gfs2)";         cmd[i] = activate_gfs; i++;
     labelptr[i] = "3 awc.gif Additive Width Correction (awc2)"; cmd[i] = activate_awc; i++;
  }
  else if (interferogram & (S_NF|S_NI))
  {
     labelptr[i] = "3 lb.gif Line Broadening (lb1)";          cmd[i] = activate_lb;  i++;
     labelptr[i] = "3 sb.gif Sinebell (sb1)";                 cmd[i] = activate_sb;  i++;
     labelptr[i] = "3 sbs.gif Sinebell Shift (sbs1)";         cmd[i] = activate_sbs; i++;
     labelptr[i] = "3 gf.gif Gaussian (gf1)";                 cmd[i] = activate_gf;  i++;
     labelptr[i] = "3 gfs.gif Gaussian Shift (gfs1)";         cmd[i] = activate_gfs; i++;
     labelptr[i] = "3 awc.gif Additive Width Correction (awc1)"; cmd[i] = activate_awc; i++;
  }
  else
  {
     labelptr[i] = "3 lb.gif Line Broadening";                cmd[i] = activate_lb;  i++;
     labelptr[i] = "3 sb.gif Sinebell";                       cmd[i] = activate_sb;  i++;
     labelptr[i] = "3 sbs.gif Sinebell Shift";                cmd[i] = activate_sbs; i++;
     labelptr[i] = "3 gf.gif Gaussian";                       cmd[i] = activate_gf;  i++;
     labelptr[i] = "3 gfs.gif Gaussian Shift";                cmd[i] = activate_gfs; i++;
     if (P_getactive(CURRENT,"sa") >= 0)
     {
        labelptr[i] = "3 sa.gif Sampling Window";                cmd[i] = activate_sa;  i++;
        if (P_getactive(CURRENT,"sas") >= 0)
        {
           labelptr[i] = "3 sas.gif Sampling Window Shift";         cmd[i] = activate_sas; i++;
        }
     }
     labelptr[i] = "3 awc.gif Additive Width Correction";     cmd[i] = activate_awc; i++;
  }
  labelptr[i] = "3 return.gif Return";                cmd[i] = wti_return;  i++;
  Wactivate_buttons(i, labelptr, cmd, wti_turnoff, "wti");
  Wactivate_mouse(0, wti_mouse, NULL);

#else 

  char *labelptr[8];
  PFV   cmd[8];

  labelptr[0] = "next fid";	cmd[0] = incr_fid;
  labelptr[7] = "return";	cmd[7] = wti_return;
  if (interferogram & S_NI2)
  {
     labelptr[1] = "lb2";		cmd[1] = activate_lb;
     labelptr[2] = "sb2";		cmd[2] = activate_sb;
     labelptr[3] = "sbs2";		cmd[3] = activate_sbs;
     labelptr[4] = "gf2";		cmd[4] = activate_gf;
     labelptr[5] = "gfs2";		cmd[5] = activate_gfs;
     labelptr[6] = "awc2";		cmd[6] = activate_awc;
     Wactivate_buttons(8, labelptr, cmd, wti_turnoff, "wti");
  }
  else if (interferogram & (S_NF|S_NI))
  {
     labelptr[1] = "lb1";		cmd[1] = activate_lb;
     labelptr[2] = "sb1";		cmd[2] = activate_sb;
     labelptr[3] = "sbs1";		cmd[3] = activate_sbs;
     labelptr[4] = "gf1";		cmd[4] = activate_gf;
     labelptr[5] = "gfs1";		cmd[5] = activate_gfs;
     labelptr[6] = "awc1";		cmd[6] = activate_awc;
     Wactivate_buttons(8, labelptr, cmd, wti_turnoff, "wti");
  }
  else
  {
     labelptr[1] = "lb";		cmd[1] = activate_lb;
     labelptr[2] = "sb";		cmd[2] = activate_sb;
     labelptr[3] = "sbs";		cmd[3] = activate_sbs;
     labelptr[4] = "gf";		cmd[4] = activate_gf;
     labelptr[5] = "gfs";		cmd[5] = activate_gfs;
     labelptr[6] = "awc";		cmd[6] = activate_awc;
     Wactivate_buttons(8, labelptr, cmd, wti_turnoff, "wti");
  }
  Wactivate_mouse(0, wti_mouse, NULL);
#endif 
}


/*-------------------------------
|				|
|	 wti_update()/0		|
|				|
+------------------------------*/
static int wti_update()
{
  if (wti_showrawfid())
  {
     disp_status("           ");
     skyreleaseAllWithId("wti");
     ABORT;
  }

  wti_showwtfunc();
  if (wti_showspectrum(TRUE))
  {
     disp_status("             ");
     skyreleaseAllWithId("wti");
     ABORT;
  }
  RETURN;
}


/****************************/
static int getifn(int tree, char *fnname, double npx)
/****************************/
{ int r,a;
  double rval;
  vInfo info;
  if ( (r=P_getreal(tree,fnname,&rval,1)) )
    { P_err(r,"current ",fnname);
      return(ERROR);
    }
  if ( (r=P_getVarInfo(tree,fnname,&info)) )
    { P_err(r,"info?",fnname);
      return(ERROR);
    }
  if (info.active) 
    a = (int)(rval+0.5);
  else 
    a = npx;
  ifn = 32; 
  while (ifn<a) 
    ifn *= 2;
  P_setreal(tree,fnname,(double)ifn,0);
  return(COMPLETE);
}


/*---------------------------------------
|					|
|		  wti()			|
|					|
+--------------------------------------*/
int wti(int argc, char *argv[], int retc, char *retv[])
{
   char		ni0name[6],
		fn1name[6];
   int		r,
		wtsize,
		nfidbytes,
		realbit,
		arg_no;
   double	rnp;
   ftparInfo	ftpar;
   dfilehead	fidhead;


   acqflag = readwtflag = FALSE;

   if (WgraphicsdisplayValid(argv[0]) && (argc > 1) && (!isReal(argv[1])) &&
        (strcmp(argv[1], "ptype") != 0) && (strcmp(argv[1], "ntype") != 0))
   {
      procstatus = setprocstatus(interferogram, 0);
      if (interferogram & (S_NF|S_NI))
      {
         if ((strcmp(argv[1], "vf") != 0) && (strcmp(argv[1], "vs") != 0) &&
             (strcmp(argv[1], "lb1") != 0) && (strcmp(argv[1], "sb1") != 0) &&
             (strcmp(argv[1], "sbs1") != 0) && (strcmp(argv[1], "gf1") != 0) &&
             (strcmp(argv[1], "gfs1") != 0) && (strcmp(argv[1], "awc1") != 0))
         {
            return COMPLETE;
         }
      }
      else if (interferogram & S_NI2)
      {
         if ((strcmp(argv[1], "vf") != 0) && (strcmp(argv[1], "vs") != 0) &&
             (strcmp(argv[1], "lb2") != 0) && (strcmp(argv[1], "sb2") != 0) &&
             (strcmp(argv[1], "sbs2") != 0) && (strcmp(argv[1], "gf2") != 0) &&
             (strcmp(argv[1], "gfs2") != 0) && (strcmp(argv[1], "awc2") != 0))
         {
            return COMPLETE;
         }
      }
      else
      {
         if ((strcmp(argv[1], "vf") != 0) && (strcmp(argv[1], "vs") != 0) &&
             (strcmp(argv[1], "sa") != 0) && (strcmp(argv[1], "sas") != 0) &&
             (strcmp(argv[1], "lb") != 0) && (strcmp(argv[1], "sb") != 0) &&
             (strcmp(argv[1], "sbs") != 0) && (strcmp(argv[1], "gf") != 0) &&
             (strcmp(argv[1], "gfs") != 0) && (strcmp(argv[1], "awc") != 0))
         {
            return COMPLETE;
         }
      }

      Wgmode();
      if (init_wt1(&wtp, interferogram))
         ABORT;
      wti_displayparms2();
      wti_update();
      Wsetgraphicsdisplay("wti");
      return COMPLETE;
   }


   ddrType = 0;
   on_status = NOTHING_ON;
   ptype = FALSE;
   arg_no = 0;
   if (argc > 1)
   {
      if (strcmp(argv[1], "ptype") == 0)
      {
         ptype = TRUE;
         arg_no++;
      }
      else if (strcmp(argv[1], "ntype") == 0)
      {
         arg_no++;
      }
   }

   Wturnoff_buttons();
   Wturnoff_mouse();
   if (ds_checkinput(argc-arg_no, argv+arg_no, &specIndex))
      ABORT;

   if (dataheaders(0, 0))
      ABORT;

   if (check2d(0))
      ABORT;

/**********************************************
* Obtain the current acquisition time; abort  *
* if an error occurs.                         *
**********************************************/
   if ( (r = P_getreal(CURRENT, "at", &at, 1)) )
   {
      P_err(r, "current ", "at:");
      ABORT;
   }


   do_sa = (P_getactive(CURRENT,"sa") >= 0) ? 1 : 0;
   do_sas = (P_getactive(CURRENT,"sas") >= 0) ? 1 : 0;
/******************************************
*  Section for interferograms.  FID data  *
*  is not considered 2D data.             *
******************************************/

   hypercomplex = (datahead.status & S_HYPERCOMPLEX);

   if ( (d2flag) && (datahead.status & S_DATA) &&
      (datahead.status & S_SPEC) && ((~datahead.status) & S_SECND) )
   {
      revflag = TRUE;
      realbit = REAL_t1;
      interferogram = datahead.status & (S_NP|S_NF|S_NI|S_NI2);

      if (datahead.status & S_3D)
      {
         Werrprintf("`wti` cannot be used on 3D spectral data");
         ABORT;
      }

      if (interferogram & S_NF)
      {
         strcpy(ni0name, "nf");
         strcpy(fn1name, "fn1");
      }
      else if (interferogram & S_NI)
      {
         strcpy(ni0name, "ni");
         strcpy(fn1name, "fn1");
      }
      else
      {
         strcpy(ni0name, "ni2");
         strcpy(fn1name, "fn2");
      }

      if ( (r = P_getreal(CURRENT, ni0name, &rnp, 1)) )
      {
         P_err(r, "current ", ni0name);
         ABORT;
      }

      rnp *= 2.0;	/* This is a problem for partially completed data
			   sets unless FT1D properly zerofills the half-
			   transformed data.  S.F.  */

      if (getifn(PROCESSED, fn1name, rnp))
      {
         ABORT;
      }

      if (select_init(0, 1, interferogram, 0, 0, 1, 1, 0))
      {
         ABORT;
      }

      fn = fn1 = ifn;
      exp_factors(1);
      npnt_sp = npnt;
      fpnt_sp = fpnt;

      np = (int) (rnp + 0.5);
      if (np > ifn)
         np = ifn;

      if ( (r = P_getreal(PROCESSED, "fn", &rnp, 1)) )
      {
         P_err(r, "processed ", "fn");
         ABORT;
      }

      maxspecIndex = (int) (rnp + 0.5) / 2;
      fpnt = 0;
      npnt = ifn/2;
   }
   else
   {
      interferogram = S_NP;
      realbit = REAL_t2;
      if ( (r = P_getreal(CURRENT, "np", &rnp, 1)) )
      {
         P_err(r, "current ", "np:");
         ABORT;
      }

      if (getifn(CURRENT, "fn", rnp))
         ABORT;

      if (select_init(0, 1, interferogram, 0, 0, 1, 1, 0))
         ABORT;

      fn = ifn;
      exp_factors(1);
      npnt_sp = npnt;
      fpnt_sp = fpnt;
      if (initfid(1))
         ABORT;

      ftpar.np0 = (int) (rnp + 0.5);
      ftpar.fn0 = ifn;	/* added 061093 by MER so i_fid() doesn't
			   get a random value for ftpar.fn0 */
      if ( i_fid(&fidhead, &ftpar) )
         ABORT;

      np = ftpar.np0;
      maxspecIndex = nblocks;
   }


   procstatus = setprocstatus(interferogram, 0);
   if (interferogram & S_NI2)
   {
      disp_status("WTI2    ");
      lpname = "lp2";
      rpname = "rp2";
      dmgname = "dmg1";
      lsfidname = "lsfid2";
      phfidname = "phfid2";
      lsfrqname = "lsfrq2";
   }
   else if (interferogram & (S_NF|S_NI))
   {
      disp_status("WTI1    ");
      lpname = "lp1";
      rpname = "rp1";
      dmgname = "dmg1";
      lsfidname = "lsfid1";
      phfidname = "phfid1";
      lsfrqname = "lsfrq1";
   }
   else
   {
      disp_status("WTI     ");
      lpname = "lp";
      rpname = "rp";
      dmgname = "dmg";
      lsfidname = "lsfid";
      phfidname = "phfid";
      lsfrqname = "lsfrq";
   }

   ls_ph_fid(lsfidname, &lsfid_0, phfidname, &phfid_0, lsfrqname, &lsfrq_0);

   if ( (r = P_getreal(CURRENT, "vs", &vs, 1)) )
   {
      P_err(r, "current ", "vs:");
      return ERROR;
   }

   pwr = fnpower(ifn);
   dispcalib = (float) (dnpnt2/3) / (float) wc2max;
   if (specIndex < 1)
   {
      specIndex = 1;
   }
   else if (specIndex > maxspecIndex)
   {
      specIndex = maxspecIndex;
   }

   disp_specIndex(specIndex);
   miny[0] = dfpnt2 + 1;
   maxy[0] = dfpnt2 + dnpnt2/3 - 1;
   miny[1] = maxy[0] + 2;
   maxy[1] = dfpnt2 + 2*dnpnt2/3 - 1;
   miny[2] = maxy[1] + 2;
   maxy[2] = dfpnt2 + dnpnt2-1;

   nfidbytes = ifn * sizeof(float);
   if (hypercomplex)
      nfidbytes *= 2;

   if ((rawfid = skyallocateWithId(nfidbytes, "wti")) == NULL)
   {
      Werrprintf("cannot allocate buffer for FID");
      skyreleaseAllWithId("wti");
      return ERROR;
   }

   wtsize = ifn/2;
   if (procstatus & realbit)
      wtsize *= 2;

   if ((wtfunc = skyallocateWithId(sizeof(float)*wtsize, "wti")) == NULL)
   {
      Werrprintf("cannot allocate buffer for weighting vector");
      skyreleaseAllWithId("wti");
      return ERROR;
   }

   if ((spectrum = skyallocateWithId(sizeof(float)*ifn, "wti")) == NULL)
   {
      Werrprintf("cannot allocate buffer for spectrum");
      skyreleaseAllWithId("wti");
      return ERROR;
   }


   Wclear_graphics();
   Wshow_graphics();
   Wgmode();
   show_plotterbox();
   color(SCALE_COLOR);

   amove(dfpnt-1, miny[0]-1);
   rdraw(dnpnt+1, 0);
   rdraw(0, dnpnt2);
   rdraw(-dnpnt-1, 0);
   rdraw(0, -dnpnt2);
   amove(dfpnt-1, maxy[0]+1);
   rdraw(dnpnt+1, 0);
   amove(dfpnt-1, maxy[1]+1);
   rdraw(dnpnt+1, 0);

   raw = 0;
   weighted = 1;
   spec = 2;
   next = 3;
   eraseraw = 0;
   eraseweighted = 0;
   wtiErasespec = 0;
   current_rawfid = -1;
   old_vf = 0;
   old_vs = 0;

   specflag = (ifn <= MAXSPECSIZE);
   init_display();

   if (init_wt1(&wtp, interferogram))
   {
      disp_status("           ");
      skyreleaseAllWithId("wti");
      return ERROR;
   }
   if (wtp.gf == 0.0)
     wtp.gf_active = 0;

   if (wti_showrawfid())
   {
      disp_status("           ");
      skyreleaseAllWithId("wti");
      ABORT;
   }

   wti_displayparms1();
   wti_buttons();
   ParameterLine(0, 0, -PARAM_COLOR,
    "Mouse buttons: Left - weighting, Center - vf/vs, Right - spectrum on/off");

   Wsetgraphicsdisplay("wti");

   if (wtp.lb_active)
   {
      activate_lb();
   }
   else if (wtp.sb_active)
   {
      activate_sb();
   }
   else if (wtp.gf_active)
   {
      activate_gf();
   }
   else if (wtp.sa_active)
   {
      activate_sa();
   }
   else
   {
      wti_displayparms2();
      wti_showwtfunc();
      if (wti_showspectrum(TRUE))
      {
         disp_status("             ");
         skyreleaseAllWithId("wti");
         ABORT;
      }
   }
   releasevarlist();
   RETURN;
}


/*---------------------------------------
|					|
|	  wti_displayparms1()/0		|
|					|
|  This function produces the display	|
|  line for the weighting parameter	|
|  labels.				|
|					|
+--------------------------------------*/
static void wti_displayparms1()
/* display certain parameters in last two lines of display */
{
  ParameterLine(1,0,PARAM_COLOR," ");
  ParameterLine(1,3,PARAM_COLOR,"vf ");
  ParameterLine(1,12,PARAM_COLOR,"vs ");
  if (interferogram & S_NI2)
  {
    ParameterLine(1,20,PARAM_COLOR,"  lb2 ");
    ParameterLine(1,29,PARAM_COLOR,"  sb2 ");
    ParameterLine(1,38,PARAM_COLOR,"  sbs2 ");
    ParameterLine(1,47,PARAM_COLOR,"  gf2 ");
    ParameterLine(1,55,PARAM_COLOR,"  gfs2 ");
    ParameterLine(1,64,PARAM_COLOR,"  awc2 ");
/*
    ParameterLine(1, 0, PARAM_COLOR,
    "     vf       vs     2:lb2    3:sb2    4:sbs2   5:gf2   6:gfs2    7:awc2");
*/
  }
  else if (interferogram & (S_NF|S_NI))
  {
    ParameterLine(1,20,PARAM_COLOR,"  lb1 ");
    ParameterLine(1,29,PARAM_COLOR,"  sb1 ");
    ParameterLine(1,38,PARAM_COLOR,"  sbs1 ");
    ParameterLine(1,47,PARAM_COLOR,"  gf1 ");
    ParameterLine(1,55,PARAM_COLOR,"  gfs1 ");
    ParameterLine(1,64,PARAM_COLOR,"  awc1 ");
/*
    ParameterLine(1, 0, PARAM_COLOR,
    "     vf       vs     2:lb1    3:sb1    4:sbs1   5:gf1   6:gfs1    7:awc1");
*/
  }
  else
  {
    ParameterLine(1,20,PARAM_COLOR,"  lb ");
    ParameterLine(1,29,PARAM_COLOR,"  sb ");
    ParameterLine(1,38,PARAM_COLOR,"  sbs ");
    ParameterLine(1,47,PARAM_COLOR,"  gf ");
    ParameterLine(1,55,PARAM_COLOR,"  gfs ");
    ParameterLine(1,64,PARAM_COLOR,"  awc ");
    if (do_sa)
    {
       ParameterLine(1,72,PARAM_COLOR,"  sa ");
       if (do_sas)
          ParameterLine(1,81,PARAM_COLOR,"  sas ");
    }
/*
    ParameterLine(1, 0, PARAM_COLOR,
    "     vf       vs     2:lb     3:sb     4:sbs    5:gf    6:gfs     7:awc ");
*/
  }

  ParamValue(2, 0, 1, 0, vf);
  ParamValue(2, 9, 1, 0, vs);
}


/*---------------------------------------
|					|
|	  wti_displayparms2()/0		|
|					|
|  This function produces the display	|
|  line for the weighting parameter	|
|  values.				|
|					|
+--------------------------------------*/
static void wti_displayparms2()
/* display certain parameters in last two lines of display */
{
  ParamValue(2, 18, wtp.lb_active, (on_status == LB_ON), wtp.lb);
  ParamValue(2, 27, wtp.sb_active, (on_status == SB_ON), wtp.sb);
  ParamValue(2, 36, (wtp.sbs_active && wtp.sb_active), (on_status == SBS_ON),
		wtp.sbs);
  ParamValue(2, 45, wtp.gf_active, (on_status == GF_ON), wtp.gf);
  ParamValue(2, 54, (wtp.gfs_active && wtp.gf_active), (on_status == GFS_ON),
		wtp.gfs);
  ParamValue(2, 63, wtp.awc_active, (on_status == AWC_ON), wtp.awc);
  if (do_sa)
  {
    ParamValue(2, 72, wtp.sa_active, (on_status == SA_ON), wtp.sa);
    if (do_sas)
       ParamValue(2, 81, (wtp.sas_active && wtp.sa_active), (on_status == SAS_ON),
		wtp.sas);
  }
}


/***************************************************/
static void ParamValue(int line, int column, int active, int turned_on,
            double value)
/***************************************************/
{ char ln[80];
  int c;
  c = PARAM_COLOR;
  if (turned_on) c = -c;
  if (active)
    {
      if ( (column == 72) || (column == 81) )  /* sa and sas values */
        sprintf(ln,"%8d ", (int) value);
      else if ((-999.0<value && value<-0.01) || (0.001<value && value<9999.0))
        sprintf(ln,"%8.3f ",value);
      else
        sprintf(ln,"%8.1e ",value);
      ParameterLine(line,column,c,ln);
    }
  else
    ParameterLine(line,column,c,"  unused ");
}

/********************/
static void update_vf_par()
/********************/
{ ParamValue(2,0,1,0,vf);
  P_setreal(CURRENT,"vf",vf,0);
}

/********************/
static void update_vs_par()
/********************/
{ ParamValue(2,9,1,0,vs);
  P_setreal(CURRENT,"vs",vs,0);
}
