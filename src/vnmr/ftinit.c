/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "data.h"
#include "allocate.h"
#include "ftpar.h"
#include "process.h"
#include "group.h"
#include "variables.h"
#include "vfilesys.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"
#include "tools.h"


#define COMPLETE	0
#define ERROR		1
#define FALSE		0
#define TRUE		1
#define MAX2DFTSIZE	16384		/* 2D (max FT number)/bufferscale  */
#define MAX1DFTSIZE	(512*1024)	/* 1D (max FT number)/bufferscale  */
#define BUFWORDS	65536		/* words/bufferscale in buffer	   */
#define MINLSFRQ        0.01	        /* in Hz			   */

#define MAX_SSORDER	20		/* maximum value		   */
#define MIN_SSFILTER	10.0		/* minimum value		   */

#define DFLT_SSFILTER	100.0		/* default value		   */
#define DFLT_SSNTAPS	121		/* default value		   */
#define DFLT_SSORDER	7		/* default value		   */

#define MAX_DSFACTOR	500		/* maximum downsampling factor */
#define MIN_DSNTAPS	3		/* minimum number of coefficients */
#define MAX_DSNTAPS	50000		/* maximum number of coefficients */
#define DFLT_DSNTAPS	61		/* default number of coefficients */

int		acqflag;		/* acquisition flag		   */

extern int	bufferscale,		/* scaling factor for Vnmr buffers */
		specIndex,
		c_first,
		c_last,
		c_buffer;

extern int  check_other_experiment(char *exppath, char *exp_no);
extern void ds_digfilter(double *dbuffer, double decfactor, int ntaps, int norm);
extern int  init_downsample_files(ftparInfo *ftpar);
extern int  dim1count();
extern int p11_saveFDAfiles_processed(char* func, char* orig, char* dest);



extern coefs	arraycoef[MAX2DARRAY];
static int getmultipliers(int argc, char *argv[], int arg_no, int dfflag,
                         ftparInfo *ftpar);
static int loadcoef(char *stringval, float *coefval);
static int set_offsets(int argus[], ftparInfo *ftpar);
static void clearFidProcInfo(ftparInfo *ftpar);
static int getFidProcInfo(dfilehead *fidhead, ftparInfo *ftpar);

int i_fid(dfilehead *fidhead, ftparInfo *ftpar);
int fidss_par(ssparInfo *sspar, int ncp0, int memalloc, int dimen);
int downsamp_par(ftparInfo *ftpar);
extern void clearMspec();
extern void set_dpf_flag(int b, char *cmd);
extern void set_dpir_flag(int b, char *cmd);

/*-----------------------------------------------
|						|
|		new_phasefile()/7		|
|						|
+----------------------------------------------*/
int new_phasefile(dfilehead *phasehead, int d2flg, int nblocks, int fn_0,
                  int fn_1, int setstatus, int hypercomplex)
{
   char	filepath[MAXPATHL];
   int  e;

/***********************************************
*  Open phase file with data handler.  First,  *
*  close any open file.                        *
***********************************************/

   D_trash(D_PHASFILE);

   if ( (e = D_getfilepath(D_PHASFILE, filepath, curexpdir)) )
   {
      D_error(e);
      return(ERROR);
   }

   phasehead->status = (short)setstatus;
   phasehead->status &= (~S_TRANSF);
   phasehead->nblocks = nblocks;
   phasehead->ebytes = 4;
   phasehead->vers_id = 0;

   if (d2flg)
   {
      phasehead->ntraces = fn_0/(2*nblocks);
      phasehead->np = fn_1/2;
   }
   else
   {
      phasehead->ntraces = 1;
      phasehead->np = fn_0/2;
   }

   phasehead->tbytes = phasehead->ebytes * phasehead->np;
   phasehead->bbytes = phasehead->tbytes * phasehead->ntraces +
		          sizeof(dblockhead);

   if (d2flg && hypercomplex)
   {
      phasehead->bbytes += sizeof(dblockhead);
      phasehead->nbheaders = 2;
   }
   else
   {
      phasehead->nbheaders = 1;
   }

   if ( (e = D_newhead(D_PHASFILE, filepath, phasehead)) )
   {
      D_error(e);
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      copypar()/0		|
|					|
|   This function copies processing	|
|   and acquisition parameters from	|
|   one tree to another tree.		|
|					|
+--------------------------------------*/
static int copypar()
{
  int	r;

  if ( (r = P_copygroup(CURRENT, PROCESSED, G_PROCESSING)) )
  {
     P_err(r, "PROCESSING ", "copygroup:");
     return(ERROR);
  }

  P_setstring(PROCESSED, "ptspec3d", "nnn", 1);

  if ( (r = P_copygroup(PROCESSED, CURRENT, G_ACQUISITION)) )
  {
     P_err(r, "ACQUISITION ", "copygroup:");
     return(ERROR);
  }

  p11_saveFDAfiles_processed("ftinit","-", "datdir");
  return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|                   i_ft()/9                    |
|                                               |
|   This function initializes the 1D and 2D     |
|   FT programs.                                |
|                                               |
+----------------------------------------------*/
int i_ft(int argc, char *argv[], int setstatus, int checkstatus, int flag2d,
         ftparInfo *ftpar, dfilehead *fidhead, dfilehead *datahead,
         dfilehead *phasehead)
{
    int copyPars = TRUE;

   char         filepath[MAXPATHL],
                ni0name[6],
                ni1name[6],
                fn1name[6],
                dmg[5],
                dcrmv[4],
                pmode[10],
		partspec[10];
   int          dfflag,
		arrayflag,
                fn_active,
                fn1_active = 0,
                nfflag = FALSE,
                niflag = FALSE,
                ni2flag = FALSE,
                d2_cmplx,
                d2_avmode,
                d2_phmode,
                d2_pwrmode,
                d2_pamode,
                r,
		tmpi,
                arg_no;
   double       rval,
                oldfn0,
                oldfn1;
   vInfo        info;
   extern void	resetf3();
 
   clearMspec();
   set_dpf_flag(0,"");
   set_dpir_flag(0,"");
   
  if (argc > 1 && strcmp(argv[1],"nopars") == 0) {
      copyPars = FALSE;
  }

   D_allrelease();

   if ( !P_getstring(PROCESSED, "ptspec3d", partspec, 1, 10) )
   {
      if (partspec[0] == 'y')
         resetf3();
   }
 
/*******************************************
*   provision for baseline offset removal  *
*   using numbers reported by noise check  *
*******************************************/
                 
   ftpar->offset_flag = FALSE;
   if (!P_getstring(CURRENT, "dcrmv", dcrmv, 1, 4))
   {
     if (dcrmv[0] == 'y')
        ftpar->offset_flag = TRUE;
   }


/**************************************
*  Initialize certain processing and  *
*  display parameters for 1D and 2D   *
*  FT processing.                     *
**************************************/

   ftpar->dophase = FALSE;
   ftpar->doabsval = FALSE;
   ftpar->dopower = FALSE;
   ftpar->dophaseangle = FALSE;
   ftpar->dof1phase = FALSE;
   ftpar->dof1absval = FALSE;
   ftpar->dof1power = FALSE;
   ftpar->dof1phaseangle = FALSE;
   ftpar->hypercomplex = FALSE;
   ftpar->combineflag = FALSE;
   ftpar->D_dsplymode = 0;
   ftpar->D_cmplx = 0;
   ftpar->lpval = 0;
   ftpar->daslp = 0;
   ftpar->ftarg.fidsPerSpec = 1;
 
   dfflag = (argv[0][0] == 'd');                 /* df2d */

/************************
*  2D status bit setup  *
************************/
   arg_no = 1;
 
   if (flag2d)
   {
 
/**************************
*  Initialize parameters  *
**************************/
 
      ftpar->ptype = FALSE;	/* defaults to N-type processing	*/
      ftpar->f2select = FALSE;	/* defaults to full F1 processing	*/
      ftpar->t2dc = FALSE;	/* defaults to no t2 DC correction	*/
      ftpar->t1dc = FALSE;	/* defaults to no t1 DC correction	*/

      ftpar->sspar.zfsflag = FALSE;
      ftpar->sspar.lfsflag = FALSE;
      ftpar->sspar1.zfsflag = FALSE;
      ftpar->sspar1.lfsflag = FALSE;
      ftpar->dspar.dsflag = FALSE;
      ftpar->dspar.fileflag = FALSE;
      ftpar->dspar.newpath[0] = '\0';

      ftpar->D_dimen = F1F2_DIM;
      ftpar->D_dimname = (S_NI|S_NP);
				/* defaults to NI t1 dimension		*/

      niflag = TRUE;
      d2_cmplx = NI_CMPLX;
      d2_phmode = NI_PHMODE;
      d2_avmode = NI_AVMODE;
      d2_pwrmode = NI_PWRMODE;
      d2_pamode = NI_PAMODE;
      strcpy(ni0name, "ni");
      strcpy(fn1name, "fn1");

      ftpar->t1_offset = 0;	/* default value	*/
      ftpar->t2_offset = 0;	/* default value	*/
      ftpar->cfstep = 1;	/* default value	*/

/*************************************
*  Parse all string arguments first  *
*************************************/

      while ( (argc > arg_no) && (!isReal(argv[arg_no])) )
      {
         if (strcmp(argv[arg_no], "nf") == 0)
         {
            ftpar->D_dimname = (S_NF|S_NP);
            ftpar->D_dimen = F1F2_DIM;
            nfflag = TRUE;
            niflag = FALSE;
            d2_cmplx = NF_CMPLX;
            d2_phmode = NF_PHMODE;
            d2_avmode = NF_AVMODE;
            d2_pwrmode = NF_PWRMODE;
            d2_pamode = NF_PAMODE;
            strcpy(ni0name, "nf");
            strcpy(fn1name, "fn1");
         }
         else if (strcmp(argv[arg_no], "ni2") == 0)
         {
            ftpar->D_dimname = (S_NI2|S_NP);
            ftpar->D_dimen = F3F2_DIM;
            ni2flag = TRUE;
            niflag = FALSE;
            d2_cmplx = NI2_CMPLX;
            d2_phmode = NI2_PHMODE;
            d2_avmode = NI2_AVMODE;
            d2_pwrmode = NI2_PWRMODE;
            d2_pamode = NI2_PAMODE;
            strcpy(fn1name, "fn2");
            strcpy(ni0name, "ni2");
         }
         else if (strcmp(argv[arg_no], "ptype") == 0)
         {
            ftpar->ptype = TRUE;
         }
         else if (strcmp(argv[arg_no], "ntype") == 0)
         {
            ftpar->ptype = FALSE;
         }
         else if (strcmp(argv[arg_no], "f2sel") == 0)
         {
            ftpar->f2select = TRUE;
         }
         else if (strcmp(argv[arg_no], "t2dc") == 0)
         {
            ftpar->t2dc = TRUE;
         }   
         else if (strcmp(argv[arg_no], "t1dc") == 0)
         {
            ftpar->t1dc = TRUE;
         }
         else if (strcmp(argv[arg_no], "nopars") == 0)
         {
            copyPars = FALSE;
         }
         else if ( (strcmp(argv[arg_no], "noop") != 0) &&
		   (strcmp(argv[arg_no], "ni") != 0)  &&
		   (strcmp(argv[arg_no], "noft") != 0)  &&
		   (strcmp(argv[arg_no], "nods") != 0) )
         {
            Werrprintf("invalid string argument to FT program");
            return(ERROR);
         }
 
         arg_no++;
      }

/************************************************
*  Get the "pmode" parameter which governs the  *
*  number of 2D quadrants in the DATA file.     *
************************************************/
 
      if (dfflag || (r = P_getstring(CURRENT, "pmode", pmode, 1, 10)))
         strcpy(pmode, "partial");       /* default to "partial" */
 
      if (strcmp(pmode, "full") == 0)
      {
         ftpar->D_cmplx = NP_CMPLX + d2_cmplx;
         ftpar->hypercomplex = TRUE;
      }
      else if (strcmp(pmode, "partial") == 0)
      {
         ftpar->D_cmplx = d2_cmplx;
      }
      else if (strcmp(pmode, "") != 0)
      {
         Werrprintf("invalid selection for pmode");
         return(ERROR);
      }
 
/************************************
*  Obtain the display mode for the  *
*  F1 dimension.                    *
************************************/
 
      r = P_getstring(CURRENT, "dmg1", dmg, 1, 5);
      if (!r)
      {
         r = ( (strcmp(dmg, "ph1") != 0) && (strcmp(dmg, "av1") != 0) &&
                   (strcmp(dmg, "pwr1") != 0) && (strcmp(dmg, "pa1") != 0) );
      }
 
      if (r)
      { /* default to "dmg" */
         if ( (r = P_getstring(CURRENT, "dmg", dmg, 1, 4)) )
         {
            P_err(r, "dmg", ":");
            return(ERROR);
         }
      }
 
      if ( (dmg[0] == 'p') && (dmg[1] == 'h') )
      {
         ftpar->D_dsplymode = d2_phmode;
         ftpar->dof1phase = TRUE;
      }
      else if ( (dmg[0] == 'a') && (dmg[1] == 'v') )
      {
         ftpar->D_dsplymode = d2_avmode;
         ftpar->dof1absval = TRUE;
      }
      else if ( (dmg[0] == 'p') && (dmg[1] == 'w') )
      {  
         ftpar->D_dsplymode = d2_pwrmode;
         ftpar->dof1power = TRUE;
      }
      else if ( (dmg[0] == 'p') && (dmg[1] == 'a') )
      {  
         ftpar->D_dsplymode = d2_pamode;
         ftpar->dof1phaseangle = TRUE;
      }
      else
      {
         Werrprintf("invalid display mode for the F1 dimension");
         return(ERROR);
      }

    ftpar->daslp = 0;
    if (!P_getVarInfo(CURRENT, "daslp", &info))
      if (info.active)
        if (P_getreal(CURRENT, "daslp", &(ftpar->daslp), 1))
        {
	  ftpar->daslp = 0;
        }
   }
 
/************************
*  1D status bit setup  *
************************/

   else
   {
      ftpar->D_dimname = S_NP;		/* NP t2 dimension for 1D	*/
      ftpar->D_cmplx = NP_CMPLX;	/* only complex 1D spectra data	*/
   }


/************************************
*  Obtain the display mode for the  *
*  F2 dimension.                    *
************************************/

   if ( (r = P_getstring(CURRENT, "dmg", dmg, 1, 4)) )
   {
      P_err(r, "dmg", ":");
      return(ERROR);
   }

   if ( (dmg[0] == 'p') && (dmg[1] == 'h') )
   {   
      ftpar->D_dsplymode |= NP_PHMODE;
      ftpar->dophase = TRUE;
   }
   else if ( (dmg[0] == 'a') && (dmg[1] == 'v') )
   {
      ftpar->D_dsplymode |= NP_AVMODE;
      ftpar->doabsval = TRUE;
   }
   else if ( (dmg[0] == 'p') && (dmg[1] == 'w') )
   {
      ftpar->D_dsplymode |= NP_PWRMODE;
      ftpar->dopower = TRUE;
   }
   else if ( (dmg[0] == 'p') && (dmg[1] == 'a') )
   {
      ftpar->D_dsplymode |= NP_PAMODE;
      ftpar->dophaseangle = TRUE;
   }
   else if ( (dmg[0] == 'd') && (dmg[1] == 'b') )
   {
      ftpar->D_dsplymode |= NP_PAMODE|NP_PWRMODE;
      ftpar->dopower = TRUE;  /* I think ftpar->dopower is unused!! */
   }
   else
   {
      Werrprintf("invalid display mode for the F2 dimension");
      return(ERROR);
   }
 
/***************************************
*  Get 2D increment and FN parameters  *
***************************************/
 
   ftpar->wtflag = (argv[0][0] == 'w');		/* wft, wft1d, or wft2d	*/
 
   if (flag2d)
   {
      if ( (r = P_getreal(PROCESSED, "fn", &oldfn0, 1)) )
      {
         P_err(r, "processed ", "fn:");
         return(ERROR);
      }
 
      if ( (r = P_getreal(PROCESSED, fn1name, &oldfn1,1)) )
      {
         if (strcmp(fn1name, "fn2") != 0)
         {
            P_err(r, "processed ", fn1name);
            return(ERROR);
         }
         else
         {
            oldfn1 = -2;         /* default setting for `fn2` */
         }
      }

      if ( (r = P_getreal(PROCESSED, ni0name, &rval, 1)) )
      {
         Werrprintf("Not a 2D experiment in %s", ni0name);
         return(ERROR);
      }
      else
      {
         if ( (r = P_getVarInfo(PROCESSED, ni0name, &info)) )
         {
            P_err(r, "processed ", ni0name);
            return(ERROR);
         }
         else if (!info.active)
         {
            Werrprintf("Not a 2D experiment in %s", ni0name);
            return(ERROR);
         }
         else
         {
            ftpar->ni0 = (int) (rval + 0.5);
            if (ftpar->ni0 < 2)
            {
               Werrprintf("Not a 2D experiment in %s", ni0name);
               return(ERROR);
            }
         }
      }
   }
   else
   {
      ftpar->ni0 = 1;
      ftpar->ni1 = 1;
   }

   ftpar->np1 = (ftpar->ni0) * 2;


/**********************************
*  Get 1D time-domain parameters  *
**********************************/
 
   if ( (r = P_getreal(PROCESSED, "np", &rval, 1)) )
   {
      P_err(r, "processed ", "np:");
      return(ERROR);
   }
   else
   {
      ftpar->np0 = (int) (rval + 0.5);
   }

/****************************************************
*  Get downsampling parameters.                     *
****************************************************/

   if (downsamp_par( ftpar))
     return(ERROR);

/*************************************
*  Setup 1D and 2D array parameters  *
*************************************/

   ftpar->arraydim = dim1count();	/* this is 1 for NF-2D processing */
   if (flag2d)
   {
      if (niflag || nfflag)
      {
         strcpy(ni1name, "ni2");
      }
      else
      {   
         strcpy(ni1name, "ni");		/* could be `nf`?  SF */
      }
 
      ftpar->ni1 = 1;
      if ( !(r = P_getreal(PROCESSED, ni1name, &rval, 1)) )
      {
         if ( (r = P_getVarInfo(PROCESSED, ni1name, &info)) )
         {
            P_err(r, "processed ", ni1name);
            return(ERROR);
         }
         else if (info.active)
         {
            if (rval > 1.0)
               ftpar->ni1 = (int) (rval + 0.5);
         }
      }
 
      ftpar->arraydim /= ftpar->ni1;
      if (!nfflag)
         ftpar->arraydim /= ftpar->ni0;

      if (ftpar->sspar1.zfsflag || ftpar->sspar1.lfsflag) /* fidss_par not called yet */
      {
         ftpar->t1dc = FALSE;	/* required */
      }
      ftpar->combineflag = (argc > (arg_no + 3));	/* > 3 more arguments */
   }
 
 
/***************************************
*  Get coefficients for 2D processing  *
***************************************/
 
   arrayflag = ( (ftpar->arraydim > 1) || (ftpar->combineflag)
			|| (ftpar->ni1 > 1) );

   if ( (arrayflag || ftpar->hypercomplex) && flag2d )
   {
      if ( getmultipliers(argc, argv, arg_no, dfflag, ftpar) )
         return(ERROR);
   }
   else
   {
      ftpar->arrayindex = 0;
   }
 
/******************************************
*  Get appropriate 1D or 2D-F2 FN values  *
******************************************/
 
   if ( (r = P_getreal(CURRENT, "fn", &rval,1)) )
   {
      P_err(r, "current ", "fn:");
      return(ERROR);
   }     
   else if ( (r = P_getVarInfo(CURRENT, "fn", &info)) )
   {
      P_err(r, "info?", "fn:");
      return(ERROR);
   }
   else
   {
      if (ftpar->dspar.dsflag)
      {
        r = ( (info.active) ? ((int) (rval + 0.5)) : ftpar->dspar.finalnp );
      }
      else
      {
        r = ( (info.active) ? ((int) (rval + 0.5)) : ftpar->np0 );
      }
   }
 
   fn_active = info.active;
   ftpar->fn0 = 32;
   while (ftpar->fn0 < r)
      ftpar->fn0 *= 2;


/****************************************************
*  Set up FID file if 'downsamp' option specified
****************************************************/

    if (ftpar->dspar.fileflag)
      if (init_downsample_files(ftpar))
      {
        ftpar->dspar.dsflag = FALSE;
        ftpar->dspar.fileflag = FALSE;
        return(ERROR);
      }
 
/************************************
*  Open FID file with data handler  *
************************************/
 
   if ( i_fid(fidhead, ftpar) )
      return(ERROR);

/************************************
*  Get appropriate 2D-F1 FN values  *
************************************/
 
   clearFidProcInfo(ftpar);
   if (flag2d)
   {
      if ((nfflag) && (ftpar->ni0 != fidhead->ntraces))
      {
         ftpar->ni0 = fidhead->ntraces;
         ftpar->np1 = (ftpar->ni0) * 2;
      }
      if ( (r = P_getreal(CURRENT, fn1name, &rval, 1)) )
      {
         P_err(r, "current ", fn1name);
         return(ERROR);
      }
      else if ( (r = P_getVarInfo(CURRENT, fn1name, &info)) )
      {
         P_err(r, "info? ", fn1name);
         return(ERROR);
      }
      else
      {
         r = ( (info.active) ? ((int) (rval + 0.5)) : ftpar->np1 );
      }
 
      fn1_active = info.active;
      ftpar->fn1 = 32;
      while (ftpar->fn1 < r)
         ftpar->fn1 *= 2;
      if (ftpar->fn1 < (2*ftpar->ni0))
      {
         ftpar->ni0 = ftpar->fn1/2;
         ftpar->np1 = (ftpar->ni0) * 2;
      }
   }
   else if ( ftpar->ftarg.useFtargs )
   {
      getFidProcInfo(fidhead, ftpar);
   }
 
 
/**************************************
*  Adjust FN and FN1 values for DF2D  *
**************************************/

   if (dfflag)
   {
      fn_active = 0;
      fn1_active = 0;
      while (ftpar->fn0 >= (2*ftpar->np0))
         ftpar->fn0 /= 2;
      if (ftpar->fn0 < 32)
         ftpar->fn0 = 32;
 
      while (ftpar->fn1 >= 4*ftpar->ni0)
         ftpar->fn1 /= 2;
      if (ftpar->fn1 < 32)
         ftpar->fn1 = 32;
   }
 
 
/******************************************
*  Check FN and FN1 values against their  *
*  respective maximum values.             *
******************************************/

   if (flag2d)
   {
      if (ftpar->fn0 > (MAX2DFTSIZE*bufferscale))
      {
         Werrprintf("fn too large,  max = %d", bufferscale*MAX2DFTSIZE);
         return(ERROR);
      }
      else if (ftpar->fn1 > (MAX2DFTSIZE*bufferscale))
      {
         Werrprintf("%s too large,  max = %d", fn1name,
			bufferscale*MAX2DFTSIZE);
         return(ERROR);
      }
 
      if (!dfflag)
         P_setreal(CURRENT, fn1name, (double) (ftpar->fn1), 0);
   }
   else if (ftpar->fn0 > (bufferscale*MAX1DFTSIZE))
   {
      Werrprintf("fn too large,  max = %d", bufferscale*MAX1DFTSIZE);
      return(ERROR);
   }
 
   if (!dfflag)
      P_setreal(CURRENT, "fn", (double) (ftpar->fn0), 0);

/****************************************************
*  Get time-domain solvent subtraction parameters.  *
****************************************************/

   tmpi = ftpar->np0/2;
   if (ftpar->fn0/2 < tmpi) tmpi = ftpar->fn0/2; 
   if ( fidss_par( &(ftpar->sspar), tmpi, TRUE, ftpar->D_dimname & S_NP ) )
   {
      return(ERROR);
   }

   if (flag2d) {
      tmpi = ftpar->np1/2;
      if (ftpar->fn1/2 < tmpi) tmpi = ftpar->fn1/2;
      if ( fidss_par( &(ftpar->sspar1), tmpi, TRUE, ftpar->D_dimname & (S_NI2|S_NI|S_NF) ) )
      {
         return(ERROR);
      }
   }

   if ((ftpar->dspar.dsflag) && ((ftpar->sspar.lfsflag) || (ftpar->sspar.zfsflag)))
   {
      Werrprintf("cannot use downsamp and ssfilter together");
      return(ERROR); 	/* use npx=ftpar.dspar.finalnp in fidss() in ft.c, not quite right? */
   }			/* ssfilter1 and downsamp ok? */
 
 
/*******************************************************
*  Copy current parameters into processed parameters.  *
*  Deactivate all weighting parameters and put the     *
*  correct FN and FN1 values into the processed tree   *
*  if these parameters are set to 'n' in the current   *
*  tree.                                               *
*******************************************************/

   if ( flag2d )
   {
      P_setreal(CURRENT, "lvl", (double) 0.0, 0);
      P_setreal(CURRENT, "tlt", (double) 0.0, 0);
   }
   if (copyPars) {
       if ( copypar() )
           return(ERROR);
   }
 
   if (!ftpar->wtflag)
   {
      P_setactive(PROCESSED, "lb", ACT_OFF);
      P_setactive(PROCESSED, "sb", ACT_OFF);
      P_setactive(PROCESSED, "gf", ACT_OFF);
      P_setactive(PROCESSED, "sa", ACT_OFF);
      P_setactive(PROCESSED, "awc", ACT_OFF);
      P_setactive(PROCESSED, "sbs", ACT_OFF);
      P_setactive(PROCESSED, "gfs", ACT_OFF);
      P_setactive(PROCESSED, "sas", ACT_OFF);
 
      if (ni2flag)
      {
         P_setactive(PROCESSED, "lb2", ACT_OFF);
         P_setactive(PROCESSED, "sb2", ACT_OFF);
         P_setactive(PROCESSED, "gf2", ACT_OFF);
         P_setactive(PROCESSED, "awc2", ACT_OFF);
         P_setactive(PROCESSED, "sbs2", ACT_OFF);
         P_setactive(PROCESSED, "gfs2", ACT_OFF);
      }
      else if (niflag || nfflag)
      {
         P_setactive(PROCESSED, "lb1", ACT_OFF);
         P_setactive(PROCESSED, "sb1", ACT_OFF);
         P_setactive(PROCESSED, "gf1", ACT_OFF);
         P_setactive(PROCESSED, "awc1", ACT_OFF);
         P_setactive(PROCESSED, "sbs1", ACT_OFF);
         P_setactive(PROCESSED, "gfs1", ACT_OFF);
      }
   }
 
   if (!fn_active)
      P_setreal(PROCESSED, "fn", (double) (ftpar->fn0), 0);
   if ( flag2d && (!fn1_active) )
      P_setreal(PROCESSED, fn1name, (double) (ftpar->fn1), 0);
 
 
/******************************************
*  Check data file status:  FT2D may not  *
*  have to do FT(t2).                     *
******************************************/

   if ( (r = D_getfilepath(D_DATAFILE, filepath, curexpdir)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   ftpar->dofirstft = TRUE;
   specIndex = 0;	/* tell programs that new data exist */

   if (checkstatus || acqflag)
   {
      r = D_gethead(D_DATAFILE, datahead);
      if (r)
         r = D_open(D_DATAFILE, filepath, datahead);   /* open the data file */
           
      if (r == COMPLETE)
      {
         if (acqflag)
         {
            if ( (datahead->status & (S_DATA|S_SPEC|S_SECND))
		     == (S_DATA|S_SPEC) )
            {
               if ( ( (int)(datahead->np) == ftpar->fn0 ) &&
                         ((~datahead->status) & S_TRANSF) &&
                         (datahead->ebytes == 0) )
               {
                  specIndex = datahead->nblocks;
               }
            }
         }
         else
         {
            ftpar->dofirstft = ( (datahead->status & S_HYPERCOMPLEX) &&
                                 (!(ftpar->D_cmplx & NP_CMPLX)) );
            if (!ftpar->dofirstft)
               ftpar->dofirstft = ( ((~datahead->status) & S_HYPERCOMPLEX)
                                 && (ftpar->D_cmplx & NP_CMPLX) );
            if (!ftpar->dofirstft)
            {
               ftpar->dofirstft = ((datahead->status & (S_DATA|S_SPEC|S_SECND))
                                         != (S_DATA|S_SPEC));
               if (!ftpar->dofirstft)
               {
                  ftpar->dofirstft = (!( ((int)(oldfn0) == ftpar->fn0) &&
                                 ((int)(oldfn1) == ftpar->fn1) &&
                                 (datahead->status & S_TRANSF) ));
               }
            }
         }
      }
   }
 
 
/*****************************************
*  Compute block parameters if necessary *
*****************************************/
 
   if (flag2d)
   {
      ftpar->sperblock0 = (BUFWORDS/ftpar->fn0) * bufferscale;
      if ( ftpar->sperblock0 > (ftpar->fn1/2) )
         ftpar->sperblock0 = ftpar->fn1/2;
      if ( ftpar->sperblock0 < 4 )
         ftpar->sperblock0 = 4;

      ftpar->nblocks = ftpar->fn1/(2 * ftpar->sperblock0);

      if ( ftpar->hypercomplex && (!dfflag) )
      {
         ftpar->sperblock0 /= 2;
         ftpar->nblocks *= 2;
      }
 
      if (ftpar->nblocks == 0)          /* must be at least one block */
         ftpar->nblocks = 1;
 
/*****************************************
*  If two blocks, make one larger block  *
*  for in-core transposition.            *
*****************************************/
 
      if (ftpar->nblocks == 2)
      {
         ftpar->nblocks = 1;
         ftpar->sperblock0 *= 2;
      }
 
      ftpar->sperblock1 = ftpar->fn0/(2*ftpar->nblocks);
   }
   else
   {
      ftpar->sperblock0 = 1;
      if (ftpar->nblocks > (ftpar->arraydim*ftpar->nf) / ftpar->ftarg.fidsPerSpec )  /* handles 1D NF */
      {
         ftpar->nblocks = (ftpar->arraydim * ftpar->nf) / ftpar->ftarg.fidsPerSpec;
      }
      else
      {
         if (ftpar->nblocks < 1)
             ftpar->nblocks = 1;
      }
   }


/********************************************
*  Setup DATA file header and block header  *
*  parameters.                              *
********************************************/
 
   setstatus |= (short)ftpar->D_dimname;   /* set processing dimensions */
 
   datahead->status = (setstatus|S_COMPLEX);
   datahead->nblocks = ftpar->nblocks;
   /* This is a little hack so that ft('acq') can determine if some other ft
    * has changed the data file.  For example, ga, then x FIDs are automatically
    * fted, then user tpes ft(3).  Now spectra 1 and 2 are absent.  By examining
    * ebytes, go('acq') can determine that the datafile may be incomplete and fix it
    */
   datahead->ebytes = (acqflag) ? 0 : 4;

   if (flag2d)          
   {
      datahead->status |= S_TRANSF;        /* transposed data is stored */
      datahead->ntraces = ftpar->sperblock1;
      datahead->np = ftpar->fn1;

      if ( ftpar->hypercomplex && (!dfflag) )
         datahead->np *= 2;
   }
   else
   {
      datahead->status &= (~S_TRANSF);
      datahead->ntraces = 1;
      datahead->np = ftpar->fn0;
   }
   if (fidhead->status & S_DDR)
      datahead->status |= S_DDR;
 
   datahead->vers_id = 0;
   datahead->tbytes = sizeof(float) * datahead->np;
   datahead->bbytes = (datahead->tbytes * datahead->ntraces) +
                              sizeof(dblockhead);
 
/**********************************************
*  If hypercomplex 2D spectral data is being  *
*  created, one extra data block header is    *
*  added.                                     *
**********************************************/
 
   if ( flag2d && ftpar->hypercomplex && (!dfflag) )
   {
      datahead->bbytes += sizeof(dblockhead);
      datahead->nbheaders = 2;
      datahead->status |= S_HYPERCOMPLEX;
   }
   else
   {
      datahead->nbheaders = 1;
   }
 
   c_first = 32767;
   c_last = 0;
   c_buffer = -1;


   if (ftpar->dofirstft && ( (acqflag && (specIndex == 0)) || (!acqflag) ))
   {
      D_trash(D_DATAFILE);       /* instead of closing, trash it */
      if ( (r = D_newhead(D_DATAFILE, filepath, datahead)) )
      {
         D_error(r);
         return(ERROR);
      }
   }
   else
   {
      if ( (r = D_updatehead(D_DATAFILE, datahead)) )
      {
         D_error(r);
         return(ERROR);
      }
   }
 
 
/***************************************
*  Create new phasefile and load file  *
*  header information.                 *
***************************************/
 
   if ( new_phasefile(phasehead, flag2d, ftpar->nblocks, ftpar->fn0,
			  ftpar->fn1, setstatus, ftpar->hypercomplex) )
   {
      return(ERROR);
   }

   disp_status("       ");
   return(COMPLETE);             /* successfull initialization */
}


/*---------------------------------------
|                                       |
|            fidss_par()/4              |
|                                       |
+--------------------------------------*/
int fidss_par(ssparInfo *sspar, int ncp0, int memalloc, int dimen)
{
   char		swname[10],
		ssfiltname[20],
		ssntapsname[20],
		ssordname[20];
   int		i, 
		ssorder;
   double	sw,
		ssfilter,
		tmp;
   vInfo	info;
   extern void	set_calcfidss();
   extern void	calc_digfilter();


   sspar->zfsflag = FALSE;
   sspar->lfsflag = FALSE;
   switch (dimen)
   {
      case S_NP:	strcpy(swname, "sw");
			strcpy(ssfiltname, "ssfilter");
			strcpy(ssordname, "ssorder");
			strcpy(ssntapsname, "ssntaps");
			break;
      case S_NF:
      case S_NI:	strcpy(swname, "sw1");
                        strcpy(ssfiltname, "ssfilter1"); 
                        strcpy(ssordname, "ssorder1"); 
			strcpy(ssntapsname, "ssntaps1");
                        break;
      case S_NI2:
      default:		strcpy(swname, "sw2");
                        strcpy(ssfiltname, "ssfilter2");  
                        strcpy(ssordname, "ssorder2"); 
			strcpy(ssntapsname, "ssntaps2");
   }

   if ( !P_getVarInfo(CURRENT, ssfiltname, &info) )
   {  
      if (info.active)
      {
         if ( P_getreal(PROCESSED, swname, &sw, 1) )
         {
            Werrprintf("cannot get %s for time-domain solvent subtraction", swname);
            return(ERROR);
         }
         ssfilter = DFLT_SSFILTER;		/* default value */
         if ( !P_getreal(CURRENT, ssfiltname, &ssfilter, 1) )
         {
            if (ssfilter > sw)
            {
               ssfilter = sw;
            }
            else if (ssfilter < MIN_SSFILTER)
            {
               ssfilter = MIN_SSFILTER;
            }
         }
      }
      else
         return(COMPLETE);
   }
   else
      return(COMPLETE);


   ssorder = DFLT_SSORDER;		/* default value */

   if ( !P_getVarInfo(CURRENT, ssordname, &info) )
   {
      if (info.active)
      {
         sspar->zfsflag = TRUE;
         if ( !P_getreal(CURRENT, ssordname, &tmp, 1) )
         {
            ssorder = (int) (tmp + 0.5);
            if (ssorder < 1)
            {
               ssorder = 1;
            }
            else if (ssorder > MAX_SSORDER)
            {
               ssorder = MAX_SSORDER;
            }
         }
      }
      else
         sspar->lfsflag = TRUE;
   }
   else
      sspar->lfsflag = TRUE;

   sspar->matsize = ssorder + 1;

   sspar->decfactor = (int) ( (sw/ssfilter) + 0.5 );
   if (sspar->decfactor < 2)
   {
      sspar->zfsflag = FALSE;
      sspar->lfsflag = FALSE;
      return(COMPLETE);
   }

   sspar->ntaps = DFLT_SSNTAPS;		/* default value */

   if ( !P_getVarInfo(CURRENT, ssntapsname, &info) )
   {  
      if (info.active)
      {
         if ( !P_getreal(CURRENT, ssntapsname, &tmp, 1) )
         {
            sspar->ntaps = (int) (tmp + 0.5);
            if (sspar->ntaps < 1)
               sspar->ntaps = DFLT_SSNTAPS;
         }
      }
   }

   if ( sspar->ntaps > (ncp0/2) )
   {
      sspar->ntaps = ncp0/2;
      if ( (sspar->ntaps % 2) == 0 )
      {
         sspar->ntaps += 1;
/*         P_setreal(CURRENT, "ssntaps", (double) (sspar->ntaps), 0); */
      }
   }

   sspar->membytes = ncp0 + (sspar->matsize * sspar->matsize) +
			5*sspar->matsize + sspar->ntaps + 1;
   sspar->membytes *= sizeof(double);
   sspar->buffer = NULL;

   if (memalloc)
   {
      if ( (sspar->buffer = (double *) allocateWithId(sspar->membytes,
                   "ft2d" )) == NULL )
      {
         Werrprintf("insufficient memory for time-domain solvent subtraction");
         return(ERROR);
      }

      calc_digfilter(sspar->buffer, sspar->ntaps, sspar->decfactor);
   }

   sspar->sslsfrqsize = 1;
   sspar->sslsfrq[0] = 0.0;
   if ( !P_getVarInfo(CURRENT, "sslsfrq", &info) )
      if (info.active)
      {
         if (info.size > SSLSFRQSIZE_MAX)
            sspar->sslsfrqsize = SSLSFRQSIZE_MAX;
         else
            sspar->sslsfrqsize = info.size;
         for (i=0; i<sspar->sslsfrqsize; i++)
            if ( !P_getreal(CURRENT, "sslsfrq", &tmp, i+1))
               sspar->sslsfrq[i] = tmp*(-180.0)/sw;
      }

   set_calcfidss(TRUE);
   return(COMPLETE);
}

/*---------------------------------------
|                                       |
|            downsamp_par()/1           |
|                                       |
+--------------------------------------*/
int downsamp_par(ftparInfo *ftpar)
{
    vInfo	info;
    char	filename[MAXPATHL];
    double	tmp,
		sw;
    dsparInfo   *dspar = &(ftpar->dspar);
    double	*readDFcoefs();

/**********************************************
* init dspar struct, except for fileflag
*	and newpath which are set when parsing
*	the arguments to ft
**********************************************/
    dspar->lp           = 0;
    dspar->dslsfrq      = 0;
    dspar->filter       = NULL;
    dspar->buffer       = NULL;
    dspar->data         = NULL;
    dspar->dsfactor     = 1;
    dspar->dsfiltfactor = 1.0;
    dspar->dscoeff      = 0;
    dspar->finalnp      = 0;
    dspar->dp           = TRUE;

/************************************************
* set value of dspar->dsflag and dspar->dsfactor
************************************************/
    if (!P_getVarInfo(CURRENT, "downsamp", &info))
      if (info.active)
        if (!P_getreal(CURRENT, "downsamp", &tmp, 1))
	  if (tmp > 0.999)
	  {
	    dspar->dsflag = TRUE;
	    dspar->dsfactor = (int)(tmp+0.5);
	  }

    if (dspar->dsfactor > MAX_DSFACTOR)
      dspar->dsfactor = MAX_DSFACTOR;
    if (dspar->dsfactor < 1)
      dspar->dsfactor = 1;

/*****************************************************
* continue only if dspar->dsflag set
*****************************************************/
    if (!dspar->dsflag)
    {
      dspar->fileflag = FALSE;
      return(COMPLETE);
    }

/**********************************************
* set value of dspar->dslsfrq
**********************************************/
    if (!P_getVarInfo(CURRENT, "dslsfrq", &info))
      if (info.active)
        if (!P_getreal(CURRENT, "dslsfrq", &tmp, 1))
	{
           if ( P_getreal(PROCESSED, "sw", &sw, 1) )
           {
              Werrprintf("cannot get sw for digital filtering");
              dspar->dsflag = FALSE;
              return(ERROR);
           }
	  dspar->dslsfrq = tmp*(-180.0)/sw;
	}

/**********************************************
* set value of dspar->dscoeff
**********************************************/
    if (!P_getVarInfo(CURRENT, "dscoef", &info))
      if (info.active)
        if (!P_getreal(CURRENT, "dscoef", &tmp, 1))
	{
	  dspar->dscoeff = (int)(tmp+0.5);
	}

    if (dspar->dscoeff == 0)
      dspar->dscoeff = DFLT_DSNTAPS;

/*    dspar->dscoeff = (int)((double)(dspar->dscoeff)*dspar->dsfactor/2.0); */
    if (dspar->dscoeff > MAX_DSNTAPS)
      dspar->dscoeff = MAX_DSNTAPS;
    if (dspar->dscoeff < MIN_DSNTAPS)
      dspar->dscoeff = MIN_DSNTAPS;
    if (dspar->dscoeff > ftpar->np0/4)
      dspar->dscoeff = ftpar->np0/4-1;

    if (dspar->dscoeff%2 == 0)
      dspar->dscoeff++;

/**********************************************
* set value of dspar->dsfiltfactor
**********************************************/
    dspar->dsfiltfactor = dspar->dsfactor;
    if (!P_getVarInfo(CURRENT, "dsfb", &info))
      if (info.active)
        if (!P_getreal(CURRENT, "dsfb", &tmp, 1))
	{
          if ( P_getreal(PROCESSED, "sw", &sw, 1) )
          {
            Werrprintf("cannot get sw for digital filtering");
            dspar->dsflag = FALSE;
            return(ERROR);
          }
	  dspar->dsfiltfactor = sw/(2.0*tmp);
	}
    if (dspar->dsfiltfactor <= 1.0)
      dspar->dsfiltfactor = dspar->dsfactor;

/**********************************************
* read or calculate coefficients for filter
**********************************************/

    filename[0] = '\0';
    if (!P_getVarInfo(CURRENT, "filtfile", &info))
      if (info.active)
        if (P_getstring(CURRENT, "filtfile", filename, 1, MAXPATHL))
          filename[0] = '\0';

    if ((dspar->filter = readDFcoefs(filename, dspar->dsfiltfactor,
		&(dspar->dscoeff), MIN_DSNTAPS, MAX_DSNTAPS)) == NULL)
    {
      dspar->filter = (double *)allocateWithId(sizeof(double)*(dspar->dscoeff+1),"ft2d");
      if (!dspar->filter)
      {
        Werrprintf("Error allocating filter buffer");
        dspar->dsflag = FALSE;
        dspar->fileflag = FALSE;
        return(ERROR);
      }
      ds_digfilter(dspar->filter, dspar->dsfiltfactor, dspar->dscoeff, TRUE);
    }
    else
    {
      P_setreal(CURRENT,"dscoef",(double)(dspar->dscoeff*2/dspar->dsfactor),1);
      P_setactive(CURRENT,"dscoef",ACT_OFF);
    }

/**********************************************
* set value of dspar->lp
**********************************************/
    dspar->lp = ((dspar->dscoeff/2)/(dspar->dsfactor))*360.0;

/**********************************************
* set value of dspar->finalnp
**********************************************/
    dspar->finalnp=(int)(((double)ftpar->np0/2)/dspar->dsfactor);
    dspar->finalnp *= 2;

    return(COMPLETE);
}


/*---------------------------------------
|                                       |
|               i_ift()/5               |
|                                       |
+--------------------------------------*/
int i_ift(int argc, char *argv[], int arg_no, ftparInfo *ftpar,
          dfilehead *fidhead, dfilehead *datahead)
{
   char         exppath[MAXPATHL],
                path[MAXPATHL],
                sval[16];
   short        status_mask;
   int          r,
                efactor;
   double       v,
                sw;


   if ( (argc != (arg_no + 2)) || (!isReal(argv[arg_no])) )
   {
      Werrprintf("usage - ft(<options>,'inverse',exp_number,expansion_factor)");      return(ERROR);
   }

   if (check_other_experiment(exppath, argv[arg_no]))
      return(ERROR);
 
   if (isReal(argv[arg_no+1]))
   {
      v = stringReal(argv[arg_no + 1]);
      if ((v < 1.0) || (v > 32.0))
      {
         Werrprintf("expansion factor should be between 1 and 32");
         return(ERROR);
      }
      else
      {
         efactor = 1;
         while (efactor < (int)v)
            efactor *= 2;
      }
   }
   else
   {
      Werrprintf("usage - ft(<options>,'inverse',exp_number,expansion_factor)");      return(ERROR);
   }
 
   if (P_getreal(PROCESSED, "ni", &v, 1) == 0)
   {
      if (v > 1.0)
      {
         Werrprintf("inverse ft only implemented for 1D data");
         return(ERROR);
      }
   }   
   else if (P_getreal(PROCESSED, "ni2", &v, 1) == 0)
   {
      if (v > 1.0)
      {
         Werrprintf("inverse ft only implemented for 1D data");
         return(ERROR);
      }
   }   

   D_allrelease();
   if ( (r = P_getreal(PROCESSED, "fn", &v, 1)) )
   {
      P_err(r, "processed", "fn:");
      return(ERROR);
   }

   ftpar->hypercomplex = FALSE;
   ftpar->np0 = (int)v;                /* size of input data (spectrum) */
   ftpar->fn0 = efactor * ftpar->np0;  /* size of output data (FID)     */

/* update the current acquisition parameters */
   if ( (r = P_copygroup(PROCESSED, CURRENT, G_ACQUISITION)) )
   {
      P_err(r, "ACQUISITION ", "copygroup:");
      return(ERROR);
   }

/* change parameters np, sw, and dp; store parameters in other experiment */
   if ( (r = P_getreal(PROCESSED, "sw", &sw, 1)) )
   {
      P_err(r, "processed ", "sw: ");
      return(ERROR);
   }

   if ( (r = P_setreal(CURRENT, "sw", sw*efactor, 1)) )
   {
      P_err(r, "current ", "sw: ");
      return(ERROR);
   }

   if ( (r = P_setreal(CURRENT, "np", (double) (ftpar->fn0), 1)) )
   {
      P_err(r, "current ", "np: ");
      return(ERROR);
   }

   if ( (r = P_setstring(CURRENT, "dp", "y", 1)) )
   {
      P_err(r, "current ", "dp: ");
      return(ERROR);
   }

/* setup parameters in target experiment */
   if ( (r = D_getparfilepath(CURRENT, path, exppath)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   if ( (r = P_save(CURRENT, path)) )
   {
      Werrprintf("problem storing parameters in %s", path);
      return(ERROR);
   }
 
   if ( (r = D_getparfilepath(PROCESSED, path, exppath)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   if ( (r = P_save(CURRENT, path)) )
   {
      Werrprintf("problem storing parameters in %s", path);
      return(ERROR);
   }
 
/* restore proper parameters in original experiment */
   if ( (r = P_getreal(PROCESSED, "np", &v, 1)) )
   {
      P_err(r, "processed ", "np: ");
      return(ERROR);
   }

   if ( (r = P_setreal(CURRENT, "np", v, 1)) )
   {
      P_err(r, "current ", "np: ");
      return(ERROR);
   }
 
   if ( (r = P_setreal(CURRENT, "sw", sw, 1)) )
   {
      P_err(r, "current ", "sw: ");
      return(ERROR);
   }
 
   if ( (r = P_getstring(PROCESSED, "dp", sval, 1, 15)) )
   {
      P_err(r, "processed ", "dp: ");
      return(ERROR);
   }
 
   if ( (r = P_setstring(CURRENT, "dp", &sval[0], 1)) )
   {
      P_err(r, "current ", "dp: ");
      return(ERROR);
   }
 
/* set the status of the phasefile in the target
   experiment to 0 */
   D_close(D_USERFILE);

   if ( (r = D_getfilepath(D_PHASFILE, path, exppath)) )
   {
      D_error(r);
      return(ERROR);
   }

   fidhead->status = 0;
   fidhead->vers_id = 0;
   fidhead->np = ftpar->fn0;
   fidhead->ntraces = 1;
   fidhead->nblocks = 1;
   fidhead->nbheaders = 1;
   fidhead->ebytes = 4;
   fidhead->tbytes = fidhead->np * sizeof(int);
   fidhead->bbytes = sizeof(dblockhead) +
			fidhead->tbytes * fidhead->ntraces;

   if ( (r = D_newhead(D_USERFILE, path, fidhead)) )
   {
      D_error(r);
      return(ERROR);
   }

   D_close(D_USERFILE);

/* access the file containing the original spectrum */
   if ( (r = D_gethead(D_DATAFILE, fidhead)) )
   {
      if (r == D_NOTOPEN)
      {
         if ( (r = D_getfilepath(D_DATAFILE, path, curexpdir)) )
         {
            D_error(r);
            return(ERROR);
         }

         r = D_open(D_DATAFILE, path, fidhead); /* open the file */
      }

      if (r)
      {
         D_error(r);
         return(ERROR);
      }
   }
 
   status_mask = (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX);
   if ( (fidhead->status & status_mask) != status_mask )
   {
      Werrprintf("no spectrum in file, status = %d", fidhead->status);
      return(ERROR);
   }
 
   if (ftpar->np0 != fidhead->np)
   {
      Werrprintf("size of data inconsistent, np0 = %d, head.np = %d",
                        ftpar->np0, fidhead->np);
      return(ERROR);
   }
 
/* prepare the file to contain the FID from the inverse FT */
   D_close(D_USERFILE);

   if ( (r = D_getfilepath(D_USERFILE, path, exppath)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   unlink(path);

   datahead->nblocks = fidhead->nblocks;
   datahead->ntraces = 1;
   datahead->np      = ftpar->fn0;      /* size of the expanded fid */
 
   datahead->status  &= (~S_TRANSF);
   datahead->ebytes  = 4;
   datahead->tbytes  = datahead->ebytes * datahead->np;
   datahead->bbytes  = datahead->tbytes * datahead->ntraces +
                              sizeof(dblockhead);
   datahead->status = (S_DATA|S_FLOAT|S_COMPLEX);
   if (fidhead->status & S_DDR)
      datahead->status |= S_DDR;
   datahead->nbheaders = 1;
   datahead->vers_id = 0;
 
   if ( (r = D_newhead(D_USERFILE, path, datahead)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   disp_status("  ");
   return(COMPLETE);
}
    

static void clearFidProcInfo(ftparInfo *ftpar)
{
   ftpar->ftarg.fidsPerSpec = 1;
   ftpar->ftarg.multfid = 0;
   ftpar->ftarg.numPhase = 0;
   ftpar->ftarg.curPhase = 0;
   ftpar->ftarg.autoPhaseInit = 0;
   ftpar->ftarg.phasePnts = 0;
   ftpar->ftarg.phaseSkipPnts = 0;
   ftpar->ftarg.phaseRmsPnts = 0;
   ftpar->ftarg.phaseRmsMult = 0;
   ftpar->ftarg.eccPnts = 0;
   ftpar->ftarg.eccLsfid = 0;
   ftpar->ftarg.numFreq = 0;
   ftpar->ftarg.curFreq = 0;
   ftpar->ftarg.initFreq = 0;
   ftpar->ftarg.incrFreq = 0;
   ftpar->ftarg.numSa = 0;
   ftpar->ftarg.curSa = 0;
   ftpar->ftarg.numSas = 0;
   ftpar->ftarg.curSas = 0;
   ftpar->ftarg.numShift = 0;
   ftpar->ftarg.curShift = 0;
   ftpar->ftarg.initShift = 0;
   ftpar->ftarg.incrShift = 0;
   ftpar->ftarg.numAmp = 0;
   ftpar->ftarg.curAmp = 0;
   ftpar->ftarg.saveT2dc = 0;
   ftpar->ftarg.infoFD = NULL;
   strcpy(ftpar->ftarg.autoPhasePar,"");
}


static int getFidProcInfo(dfilehead *fidhead, ftparInfo *ftpar)
{
   char sval[MAXSTR];
   char lval[MAXSTR];
   int args;
   int arg_no;

   args = P_getsize(CURRENT, "ftarg", NULL);
   arg_no = 1;
   while (arg_no <= args)
   {
      P_getstring(CURRENT, "ftarg", sval, arg_no, MAXSTR-1);
#ifdef TODO
      if ( ! strcmp(sval,"file") )
      {
      }
      else if  ( ! strcmp(sval,"parameter") )
      {
      }
      else if  ( ! strcmp(sval,"list") )
      {
         if ( (ftpar->fidcoefs = (coefs *) allocateWithId(ftpar->arraydim * 4 * sizeof(double),"ft2d")) == NULL )
         {
            Werrprintf("insufficient memory for FID coefs");
            return(ERROR);
         }
      }
      else
#endif
      if  ( ! strcmp(sval,"phaseParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            int index;
            double *ptr;
            double tmp;
            num = P_getsize(CURRENT, lval, NULL);
            ftpar->ftarg.phase = (double *) allocateWithId(num* sizeof(double), "ft2d");
            ftpar->ftarg.numPhase = num;
            ptr = ftpar->ftarg.phase;
            for (index=0; index < num; index++)
            {
               P_getreal(CURRENT, lval, &tmp, index+1);
               *ptr = tmp;
               ptr++;
            }
            ftpar->ftarg.phasePnts = 0; /* turn off auto phase parameter */
         }
      }
      else if  ( ! strcmp(sval,"autoPhaseParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            double tmp;
            if (4 == P_getsize(CURRENT, lval, NULL))
            {
               P_getreal(CURRENT, lval, &tmp, 1);
               ftpar->ftarg.phasePnts = (int) tmp;
               P_getreal(CURRENT, lval, &tmp, 2);
               ftpar->ftarg.phaseSkipPnts = (int) tmp;
               P_getreal(CURRENT, lval, &tmp, 3);
               ftpar->ftarg.phaseRmsPnts = (int) tmp;
               P_getreal(CURRENT, lval, &tmp, 4);
               ftpar->ftarg.phaseRmsMult = tmp;
               if (ftpar->ftarg.phasePnts > 500)
               {
                  Werrprintf("Warning: Number of phase calulation points is too high. Reset to max = 500.");
                  ftpar->ftarg.phasePnts = 500;
               }
               if (ftpar->ftarg.phaseSkipPnts > 500)
               {
                  Werrprintf("Warning: Number of phase calulation skip points is too high. Reset to max = 500.");
                  ftpar->ftarg.phaseSkipPnts = 500;
               }
               if (ftpar->ftarg.phaseRmsPnts > ftpar->np0 / 8)
               {
                  Werrprintf("Warning: Number of phase points for RMS calculation is too high. Reset to max = np/8.");

                  ftpar->ftarg.phaseRmsPnts = ftpar->np0 / 8;
               }
               if (ftpar->ftarg.phasePnts > 0)
               {
                  ftpar->ftarg.autophase = (float *) allocateWithId(
                                                     2 * ftpar->ftarg.phasePnts * sizeof(float), "ft2d");
               }
               ftpar->ftarg.numPhase = 0; /* turn off phase parameter */
            }
         }
      }
      else if  ( ! strcmp(sval,"autoPhaseInfo") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            char parname[MAXSTR];
            if ( !P_getstring(CURRENT, lval, parname, 1, MAXSTR-1) &&
                 (strlen(parname) > 1) )
               strcpy(ftpar->ftarg.autoPhasePar,parname);
         }
      }
      else if  ( ! strcmp(sval,"eccParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            FILE *infile;
            char filename[MAXSTR];
            int np0, i;
            double tmp;
            double *ptr;

            if ( !P_getstring(CURRENT, lval, filename, 1, MAXSTR-1) )
            {
               if ( (infile = fopen(filename,"r") ))
               {
                  if (fread(&tmp,sizeof(double),1,infile) == 1)
                  {
                     strcpy(ftpar->ftarg.eccFile,filename);
                     ftpar->ftarg.eccPnts = np0 = (int) tmp;
                     ftpar->ftarg.ecc = (double *) allocateWithId(np0* sizeof(double), "ft2d");
                     ptr = ftpar->ftarg.ecc;
                     for (i=0; i<np0; i++)
                     {
                         fread(&tmp,sizeof(double),1,infile);
                         *ptr++ = tmp;
                     }
                  }
                  fclose(infile);
               }
            }
         }
      }
      else if  ( ! strcmp(sval,"eccLsParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
                double tmp;
                P_getreal(CURRENT, lval, &tmp, 1);
                ftpar->ftarg.eccLsfid = tmp;
         }
      }
      else if  ( ! strcmp(sval,"ampParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            int index;
            double *ptr;
            double tmp;
            num = P_getsize(CURRENT, lval, NULL);
            ftpar->ftarg.amp = (double *) allocateWithId(num* sizeof(double), "ft2d");
            ftpar->ftarg.numAmp = num;
            ptr = ftpar->ftarg.amp;
            for (index=0; index < num; index++)
            {
               P_getreal(CURRENT, lval, &tmp, index+1);
               *ptr = tmp;
               ptr++;
            }
         }
      }
      else if  ( ! strcmp(sval,"saParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            int index;
            int *ptr;
            double tmp;
            num = P_getsize(CURRENT, lval, NULL);
            ftpar->ftarg.sa = (int *) allocateWithId(num* sizeof(int), "ft2d");
            ftpar->ftarg.numSa = num;
            ptr = ftpar->ftarg.sa;
            for (index=0; index < num; index++)
            {
               P_getreal(CURRENT, lval, &tmp, index+1);
               *ptr = (int) (tmp+0.1);
               ptr++;
            }
         }
      }
      else if  ( ! strcmp(sval,"sasParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            int index;
            int *ptr;
            double tmp;
            num = P_getsize(CURRENT, lval, NULL);
            ftpar->ftarg.sas = (int *) allocateWithId(num* sizeof(int), "ft2d");
            ftpar->ftarg.numSas = num;
            ptr = ftpar->ftarg.sas;
            for (index=0; index < num; index++)
            {
               P_getreal(CURRENT, lval, &tmp, index+1);
               *ptr = (int) (tmp+0.1);
               ptr++;
            }
         }
      }
      else if  ( ! strcmp(sval,"autoShiftParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            double tmp;

            if (P_getsize(CURRENT, lval, NULL) == 2)
            {
               /* Use -1 as a key. This also de-activates the non-incrementing shift adjustment. */
               ftpar->ftarg.numShift = -1;
               P_getreal(CURRENT, lval, &tmp, 1);
               if (tmp < 0)
                  ftpar->ftarg.initShift = (int) (tmp-0.1);
               else
                  ftpar->ftarg.initShift = (int) (tmp+0.1);
               P_getreal(CURRENT, lval, &tmp, 2);
               if (tmp < 0)
                  ftpar->ftarg.incrShift = (int) (tmp-0.1);
               else
                  ftpar->ftarg.incrShift = (int) (tmp+0.1);
            }
            else
            {
                Wscrprintf("Parameter %s requires two values for auto-incrementing the shift", lval);
            }
         }
      }
      else if  ( ! strcmp(sval,"shiftParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            int index;
            int *ptr;
            double tmp;
            num = P_getsize(CURRENT, lval, NULL);
            ftpar->ftarg.shift = (int *) allocateWithId(num* sizeof(int), "ft2d");
            ftpar->ftarg.numShift = num;
            ptr = ftpar->ftarg.shift;
            for (index=0; index < num; index++)
            {
               P_getreal(CURRENT, lval, &tmp, index+1);
               if (tmp < 0)
                  *ptr = (int) (tmp-0.1);
               else
                  *ptr = (int) (tmp+0.1);
               ptr++;
            }
         }
      }
      else if  ( ! strcmp(sval,"freqParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            int index;
            double *ptr;
            double tmp;
            num = P_getsize(CURRENT, lval, NULL);
            ftpar->ftarg.freq = (double *) allocateWithId(num* sizeof(double), "ft2d");
            ftpar->ftarg.numFreq = num;
            ptr = ftpar->ftarg.freq;
            for (index=0; index < num; index++)
            {
               P_getreal(CURRENT, lval, &tmp, index+1);
               if (fabs(tmp) < MINLSFRQ)
                  *ptr = 0.0;
               else
               {
                  double tmp2;
                  if (!P_getreal(PROCESSED, "sw", &tmp2, 1))
                     *ptr = -180.0*tmp/tmp2;
                  else
                     *ptr = 0.0;
               }
               ptr++;
            }
         }
      }
      else if  ( ! strcmp(sval,"autoFreqParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            double tmp;
            double tmp2;

            if (P_getsize(CURRENT, lval, NULL) == 2)
            {
               /* Use -1 as a key. This also de-activates the non-incrementing freq. adjustment. */
               ftpar->ftarg.numFreq = -1;
               if (P_getreal(PROCESSED, "sw", &tmp2, 1))
                  tmp2 = 0.0;
               P_getreal(CURRENT, lval, &tmp, 1);
               if ( (fabs(tmp) < MINLSFRQ) || (tmp2 < 0.1) )
                  ftpar->ftarg.initFreq = 0.0;
               else
                  ftpar->ftarg.initFreq = -180.0*tmp/tmp2;
               P_getreal(CURRENT, lval, &tmp, 2);
               if ( (fabs(tmp) < MINLSFRQ) || (tmp2 < 0.1) )
                  ftpar->ftarg.incrFreq = 0.0;
               else
                  ftpar->ftarg.incrFreq = -180.0*tmp/tmp2;
            }
            else
            {
                Wscrprintf("Parameter %s requires two values for auto-incrementing the phase", lval);
            }
         }
      }
      else if  ( ! strcmp(sval,"addParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            int num;
            double tmp;
            P_getreal(CURRENT, lval, &tmp, 1);
            ftpar->ftarg.fidsPerSpec = (int) tmp;
            if (ftpar->ftarg.fidsPerSpec == 0)
               ftpar->ftarg.fidsPerSpec = ftpar->arraydim;
            ftpar->ftarg.multfid = 1;
            num = P_getsize(CURRENT, lval, NULL);
            if (num == 5)
            {
                P_getreal(CURRENT, lval, &tmp, 2);
                ftpar->ftarg.rr = tmp;
                P_getreal(CURRENT, lval, &tmp, 3);
                ftpar->ftarg.ir = tmp;
                P_getreal(CURRENT, lval, &tmp, 4);
                ftpar->ftarg.ri = tmp;
                P_getreal(CURRENT, lval, &tmp, 5);
                ftpar->ftarg.ii = tmp;
            }
            else
            {
                ftpar->ftarg.rr = 1.0;
                ftpar->ftarg.ir = 0.0;
                ftpar->ftarg.ri = 0.0;
                ftpar->ftarg.ii = 1.0;
            }
         }
      }
      else if  ( ! strcmp(sval,"infoParameter") )
      {
         arg_no++;
         if ( !P_getstring(CURRENT, "ftarg", lval, arg_no, MAXSTR-1) 
            &&  (ACT_ON == P_getactive(CURRENT, lval)) )
         {
            char filename[MAXSTR];
            if ( !P_getstring(CURRENT, lval, filename, 1, MAXSTR-1) &&
                 (strlen(filename) > 2) )
               ftpar->ftarg.infoFD = fopen(filename,"w");
         }
      }
      arg_no++;
   }
   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|             getmultipliers()/5                |
|                                               |
|   This function parses the 2D processing      |
|   coefficients or 2D array selector.          |
|                                               |
+----------------------------------------------*/
static int getmultipliers(int argc, char *argv[], int arg_no, int dfflag,
                         ftparInfo *ftpar)
{
   int  i,
        *tmpint,
        coef_no,
        args[3],
        arg_rem,
        arg_init,
	numcoefs = 0;
 
 
/********************************
*  Initialization of variables  *
********************************/

   ftpar->arrayindex = 0;
   ftpar->t1_offset = 0;
   ftpar->t2_offset = 0;
   ftpar->cfstep = 1;

   args[0] = args[1] = args[2] = 1;
   arg_init = arg_no;

/***********************************************
*  For getmultipliers() to function properly,  *
*  d2, which is controlled by ni, must incre-  *
*  mented faster than d3, which is controlled  *
*  by ni2.                                     *
***********************************************/

  if (argc == arg_no)
  {
     if ( dfflag )
     {
        Werrprintf("%s(arrayindex): no array index specified", argv[0]);
        return(ERROR);
     }
     else
     {
        Werrprintf("%s requires array index or coefficients. Also, check pmode parameter.", argv[0]);
        return(ERROR);
     }
  }
  else if ( (argc == (arg_no + 1)) && (ftpar->ni1 == 1) )
  {
 
/*************************************************
*  One argument only:  select 2D-ni (or 2D-ni2)  *
*  element from an arrayed 2D data set.          *
*************************************************/
 
     ftpar->combineflag = FALSE;
 
     if (isReal(argv[arg_no]))
     {
        ftpar->arrayindex = (int) (stringReal(argv[arg_no++])) - 1;
					/* start at 0 */
        if ( (ftpar->arrayindex > (ftpar->arraydim - 1)) ||
	     (ftpar->arrayindex < 0) )
        {
           Werrprintf("%s(arrayindex): illegal array index", argv[0]);
           return(ERROR);
        }
     }
     else
     {
        Werrprintf("%s(arrayindex): index syntax error", argv[0]);
        return(ERROR);
     }
  }
  else if ( (argc == (arg_no + 1)) && (ftpar->ni1 > 1) )
  {

/*************************************************
*  One argument only:  select 2D-ni (or 2D-ni2)  *
*  element from a non-arrayed 2D data set.       *
*************************************************/
 
     ftpar->combineflag = FALSE;
 
     if (ftpar->arraydim > 1)
     {
        Werrprintf("%s(arrayindex):  insufficient index specification",
                        argv[0]);
        return(ERROR);
     }
     else if (isReal(argv[arg_no]))
     {
        tmpint = ( (ftpar->D_dimname & S_NI) ? &(ftpar->t2_offset)
			: &(ftpar->t1_offset) );
        *tmpint = (int) (stringReal(argv[arg_no++])) - 1;
 
        if ( (*tmpint < 0) || (*tmpint > (ftpar->ni1 - 1)) )
        {
           Werrprintf("%s(arrayindex): illegal array index", argv[0]);
           return(ERROR);
        }
     }
     else
     {
        Werrprintf("%s(arrayindex): index syntax error", argv[0]);
        return(ERROR);
     }
  }
  else if ( (argc == (arg_no + 2)) || (argc == (arg_no + 3)) )
  {
 
/*************************************************
*  Two or three arguments:  select 2D-ni (or     *
*  2D-ni2) element from an arrayed 3D data set.  *
*************************************************/
 
     ftpar->combineflag = FALSE;
 
     if ( (ftpar->arraydim == 1) || (ftpar->ni1 == 1) )
     {
        Werrprintf("%s(arrayindices): too many indices specified", argv[0]);
        return(ERROR);
     }
 
     for (i = 0; i < (argc - arg_init); i++)
     {
        if (isReal(argv[arg_no]))
        {
           args[i] = (int) (stringReal(argv[arg_no++]));
        }
        else
        {
           Werrprintf("%s(arrayindices): illegal index", argv[0]);
           return(ERROR);
        }
     }
 
     if ( set_offsets(args, ftpar) )
     {
        Werrprintf("%s(arrayindices): illegal index", argv[0]);
        return(ERROR);
     }
  }
  else
  {
     int nphase;
 
/***************************************************
*  Setup coefficient arrays either for an arrayed  *
*  2D data set or for a 2D-ni (or 2D-ni2) array    *
*  in an arrayed 3D data set.                      *
***************************************************/
 
     arg_rem = (argc - arg_init) % 4;
     P_getsize(PROCESSED,"phase", &nphase);
     if ( (nphase == 2) && (ftpar->ni1 == 1) && ((argc - arg_no) == 9) )
     {
        /* States case of arrayed 2D
         * First arg is index, next 8 are combination coefficients
         */
        if (isReal(argv[arg_no]))
        {
           ftpar->arrayindex = (int) (stringReal(argv[arg_no++])) - 1; /* start at 0 */
           ftpar->t2_offset = ftpar->arraydim / 2; /* Number of arrayed 2D data sets */
           ftpar->arraydim = 2;                  /* Size of arrayed phase parameter. */
           if ( (ftpar->arrayindex > (ftpar->t2_offset - 1)) ||
	        (ftpar->arrayindex < 0) )
           {
              Werrprintf("%s(arrayindex): illegal array index", argv[0]);
              return(ERROR);
           }
        }
        else
        {
           Werrprintf("%s(arrayindex): index syntax error", argv[0]);
           return(ERROR);
        }
     }
     else if ( ((arg_rem != 1) && (ftpar->ni1 > 1)) ||
           (arg_rem && (ftpar->ni1 == 1)) )
     {
        Werrprintf("%s(arrayindices): incorrect number of indices specified",
                           argv[0]);
        return(ERROR);
     }
 
     if (ftpar->ni1 > 1)
     {
        if (isReal(argv[arg_no]))
        {
           args[1] = (int) (stringReal(argv[arg_no++]));
        }
        else
        {
           Werrprintf("%s(arrayindices): illegal index", argv[0]);
           return(ERROR);
        }
 
        if ( set_offsets(args, ftpar) )
        {
           Werrprintf("%s(arrayindices): illegal index", argv[0]);
           return(ERROR);
        }
     }
 
     if (ftpar->D_dimname & S_NF)
     {
        int nfdiv = 0;

        numcoefs = (argc - arg_no)/4;
        if (numcoefs > 0)
           nfdiv = ftpar->nf/numcoefs;
 
        if ( (argc - arg_no) & 3 )
        {
           Werrprintf("%s(...) expecting a multiple of 4 arguments", argv[0]);
           return(ERROR);
        }
        else if ( ftpar->nf != (nfdiv * numcoefs) )
        {
           Werrprintf("%s(...) invalid number of arguments", argv[0]);
           return(ERROR);
        }
        else if (numcoefs > MAX2DARRAY)
        {
           Werrprintf("%s(...) too many individual fid's to combine",
                argv[0]);
           return(ERROR);
        }
     }   
     else
     {
        numcoefs = ftpar->arraydim;
        if ((argc - arg_no) != 4*numcoefs)
        {
           Werrprintf("%s(...) expecting %d, not %d arguments", argv[0],
                arg_no + 4*numcoefs - arg_init, argc - arg_init);
           return(ERROR);
        }
        else if (numcoefs > MAX2DARRAY)
        {
           Werrprintf("%s(...) too many individual fid's to combine",
                argv[0]);
           return(ERROR);
        }
     }

     for (coef_no = 0; coef_no < numcoefs; coef_no++)
     {  
        if (loadcoef(argv[arg_no++], &arraycoef[coef_no].re2d.re))
           return(ERROR);
        if (loadcoef(argv[arg_no++], &arraycoef[coef_no].re2d.im))
           return(ERROR);
     }
 
     for (coef_no = 0; coef_no < numcoefs; coef_no++)
     {
        if (loadcoef(argv[arg_no++], &arraycoef[coef_no].im2d.re))
           return(ERROR);
        if (loadcoef(argv[arg_no++], &arraycoef[coef_no].im2d.im))
           return(ERROR);
     }
  }

/*****************************************************
*  This is needed to deal with hypercomplex 2D data  *
*  processing both of non-arrayed data sets and for  *
*  non-arrayed FT2D transforms of arrayed data sets. *
*****************************************************/
 
  if ( (numcoefs == 0) && ftpar->hypercomplex && (!dfflag) )
  {
     ftpar->combineflag = TRUE;
     arraycoef[0].re2d.re = 1.0;
     arraycoef[0].re2d.im = 0.0;
     arraycoef[0].im2d.re = 0.0;
     arraycoef[0].im2d.im = 1.0;
  }
 
  return(COMPLETE);
}


/*---------------------------------------
|                                       |
|             loadcoef()/2              |
|                                       |
|  This function loads the FT2D input   |
|  into a processing coefficient.       |
|                                       |
+--------------------------------------*/
static int loadcoef(char *stringval, float *coefval)
{
   if (isReal(stringval))
   {
      *coefval = stringReal(stringval);
   }
   else
   {
      Werrprintf("expecting real argument for 2D coefficient");
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|            set_offsets()/2            |
|                                       |
+--------------------------------------*/
static int set_offsets(int argus[], ftparInfo *ftpar)
{
   if ( (ftpar->D_dimname & S_NI) && (ftpar->ni1 > 1) )
   {
      ftpar->arrayindex = argus[0] - 1;
      ftpar->t2_offset = argus[1] - 1;

      if ( (ftpar->t2_offset > (ftpar->ni1 - 1)) ||
	   (ftpar->t2_offset < 0) || (ftpar->arrayindex < 0) ||
	   (ftpar->arrayindex > (ftpar->arraydim - 1)) )
      {
         return(ERROR);
      }
   }
   else if ( (ftpar->D_dimname & S_NI2) && (ftpar->ni1 > 1) )
   {
      ftpar->arrayindex = argus[0] - 1;
      ftpar->t1_offset = argus[1] - 1;

      if ( (ftpar->t1_offset > (ftpar->ni1 - 1)) ||
	   (ftpar->t1_offset < 0) || (ftpar->arrayindex < 0) ||
	   (ftpar->arrayindex > (ftpar->arraydim - 1)) )
      {
         return(ERROR);
      }
   }
   else if (ftpar->D_dimname & S_NF)
   {
      if (ftpar->ni1 > 1)
      {
         ftpar->arrayindex = argus[0] - 1;
         ftpar->t1_offset = argus[1] - 1;
         /* ftpar->cfstep = argus[2] - 1; */

         if ( (ftpar->t1_offset > (ftpar->ni1 - 1)) ||
	      (ftpar->t1_offset < 0) || (ftpar->arrayindex < 0) ||
	      (ftpar->arrayindex > (ftpar->nf - 1)) ||
	      (ftpar->cfstep < 0) )
         {
            return(ERROR);
         }
      }
      else
      {
         ftpar->arrayindex = argus[0] - 1;
         /* ftpar->cfstep = argus[1] - 1; */

         if ( (ftpar->arrayindex < 0) || (ftpar->cfstep < 0) ||
	      (ftpar->arrayindex > (ftpar->nf - 1)) )
         {
            return(ERROR);
         }
      }
   }
   else
   {
      return(ERROR);
   }
 
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|               i_fid()/2               |
|                                       |
|  This function opens the FID file     |
|  with the data handler. 		|
|                                       |
+--------------------------------------*/
int i_fid(dfilehead *fidhead, ftparInfo *ftpar)
{
   char         filepath[MAXPATHL],
                sval[16];
   int          res;
   double       rx;
   vInfo        info;
   void		ls_ph_fid();
 
 
   D_trash(D_USERFILE);          /* instead of closing, trash it */

   if ( (res = D_getfilepath(D_USERFILE, filepath, curexpdir)) )
   {
      D_error(res);
      return(ERROR);
   }

   if ( (res = D_open(D_USERFILE, filepath, fidhead)) )
   {
      D_error(res);
      return(ERROR);
   }
 
/*******************************
*  Check NF and CF parameters  *
*******************************/
 
   if (fidhead->ntraces > 1)
   {
      if ( (res = P_getVarInfo(CURRENT, "cf", &info)) )
      {
         P_err(res, "info?", "cf:");
         return(ERROR);
      }
      else if (!info.active)
      {
         Werrprintf("parameter 'cf' must be active for this data set");
         return(ERROR);
      }
 
      if ( (res = P_getreal(CURRENT, "cf", &rx, 1)) )
      {
         P_err(res, "cf", ":");
         return(ERROR);
      }

      ftpar->cf = (int) (rx + 0.5);             /* starts at cf = 1 */
      if ( (res = P_getreal(PROCESSED, "nf", &rx, 1)) )
      {
         P_err(res, "nf", ":");
         return(ERROR);
      }

      ftpar->nf = (int) (rx + 0.5);
      if (ftpar->nf > fidhead->ntraces)
        ftpar->nf = fidhead->ntraces;
      if ( (ftpar->cf < 1) || (ftpar->cf > ftpar->nf) )
      {
         Werrprintf("parameter 'cf' must be between 1 and %d", ftpar->nf);
         return(ERROR);
      }
   }
   else
   {
      ftpar->cf = 1;
      ftpar->nf = 1;
   }
 
 
/*************************************
*  Check FID file header parameters  *
*************************************/

   if (ftpar->np0 != fidhead->np)
   {
      Werrprintf("np = %d is inconsistent with data; actual np = %d",
                         ftpar->np0, fidhead->np);
      ftpar->np0 = fidhead->np;
      P_setreal(CURRENT, "np", (double) (ftpar->np0), 0);
      P_setreal(PROCESSED, "np",(double) (ftpar->np0), 0);
   }
 
   ftpar->dpflag = (fidhead->ebytes == 4);
   if (!P_getstring(PROCESSED, "dp", sval, 1, 15))
   {              
      if (ftpar->dpflag && (sval[0] != 'y'))
      {
         P_setstring(PROCESSED, "dp", "y", 1);
      }
      else if (!ftpar->dpflag && (sval[0] != 'n'))
      {
         P_setstring(PROCESSED, "dp", "n", 1);
      }
   }
 
   ls_ph_fid("lsfid", &(ftpar->lsfid0), "phfid", &(ftpar->phfid0),
		"lsfrq", &(ftpar->lsfrq0));

   if (ftpar->lsfid0 < 0)
   {
      int  npx;

      npx = ( (ftpar->fn0 < (ftpar->np0 - ftpar->lsfid0)) ?
		(ftpar->fn0 + ftpar->lsfid0) : ftpar->np0 );

      if (npx < 2)
      {
         Werrprintf("lsfid is too large in magnitude");
         return(ERROR);
      }
   }
   else
   {
      if (ftpar->lsfid0 >= ftpar->np0)
      {
         Werrprintf("lsfid is too large in magnitude");
         return(ERROR);
      }
   }
 
   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|                ls_ph_fid()/6                  |
|                                               |
|   This function returns the values for the    |
|   requested "lsfid#", "phfid#" and "fshift#"  |
|   parameters. 				|
|                                               |
+----------------------------------------------*/
void ls_ph_fid(char *lsfname, int *lsfval, char *phfname, double *phfval,
               char *lsfrqname, double *lsfrqval)
{
   double       tmp,
		tmp2;
   vInfo        info;


   *lsfval = 0;
   *phfval = 0.0;
   *lsfrqval = 0.0;

   if (lsfname != NULL)
   {
      if (!P_getreal(CURRENT, lsfname, &tmp, 1))
      {
         if (!P_getVarInfo(CURRENT, lsfname, &info))
         {
            if (info.active)
               *lsfval = (int) (2*tmp);
         }
      }
   }
 
   if (phfname != NULL)
   {
      if (!P_getreal(CURRENT, phfname, &tmp, 1))
      {
         if (!P_getVarInfo(CURRENT, phfname, &info))
         {
            if (info.active)
               *phfval = tmp;
         }
      }
   }

   if (lsfrqname != NULL)
   {
      if (!P_getreal(CURRENT, lsfrqname, &tmp, 1))
      {
         if (fabs(tmp) < MINLSFRQ)
            return;

         if (!P_getVarInfo(CURRENT, lsfrqname, &info))
         {
            if (info.active)
            {
               if ( strcmp(lsfrqname, "lsfrq") == 0 )
               {
                  if (!P_getreal(PROCESSED, "sw", &tmp2, 1))
                     *lsfrqval = -180.0*tmp/tmp2;
               }
               else if ( strcmp(lsfrqname, "lsfrq1") == 0 )
               {
                  if (!P_getreal(PROCESSED, "sw1", &tmp2, 1))
                     *lsfrqval = -180.0*tmp/tmp2;
               }
               else
               {
                  if (!P_getreal(PROCESSED, "sw2", &tmp2, 1))
                     *lsfrqval = -180.0*tmp/tmp2;
               }
            }
         }
      }
   }
}
