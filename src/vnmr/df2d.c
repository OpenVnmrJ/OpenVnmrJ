/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "data.h"
#include "ftpar.h"
#include "process.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "sky.h"
#include "tools.h"
#include "pvars.h"
#include "wjunk.h"

#define COMPLETE	0
#define ERROR		1
#define FALSE		0
#define TRUE		1
#define DF2D_SCALE	5.0e-5

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

extern int	acqflag,
                Bnmr,           /* background flag			*/
		interuption;

extern int  i_fid(dfilehead *fidhead, ftparInfo *ftpar);
extern int  dim1count();    /* located in ft2d.c  */
extern void ls_ph_fid(char *lsfname, int *lsfval, char *phfname, double *phfval,
                      char *lsfrqname, double *lsfrqval);
extern int  new_phasefile(dfilehead *phasehead, int d2flg, int nblocks, int fn_0,
                          int fn_1, int setstatus, int hypercomplex);
extern void long_event();
extern void driftcorrect_fid(float *outp, int np, int lsfid, int datamult);
extern int i_ft(int argc, char *argv[], int setstatus, int checkstatus, int flag2d,
                ftparInfo *ftpar, dfilehead *fidhead, dfilehead *datahead,
                dfilehead *phasehead);
extern int setheader(dpointers *block, int status_value, int mode_value, int index_value,
                     int hypercomplex);
extern void rotate_fid(float *fidptr, double ph0, double ph1, int np, int datatype);
extern void zeroimag(float *dataptr, int np, int dispflag);

static int convertfids(ftparInfo *ftpar, dfilehead *fidhead);
static int convertblock(int *curfid, int blocknum, int *lastfid, int fidincr,
                ftparInfo *ftpar, dfilehead *fidhead, int nf_firstfid);
int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead, int *lastfid);

/*---------------------------------------
|                                       |
|                df2d()                 |
|                                       |
|   This function displays a 2D FID     |
|   data set.                           |
|                                       |
+--------------------------------------*/
int df2d(int argc, char *argv[], int retc, char *retv[])
{
  ftparInfo     ftpar;
  dfilehead     fidhead,
                datahead,
                phasehead;
 
                  
  (void) retc;
  (void) retv;
  if ( i_ft(argc, argv, (S_DATA|S_FLOAT), 0, 1, &ftpar, &fidhead,
                &datahead, &phasehead) )
  {
     ABORT;
  }
 
  acqflag = FALSE;
  ftpar.zeroflag = FALSE;
 
  if (ftpar.combineflag)
  {
     Werrprintf("%s(...) cannot combine fid's:  too many arguments", argv[0]);
     ABORT;
  }
 
  if (convertfids(&ftpar, &fidhead))
  {
     disp_index(0);
     ABORT;
  }
 
  D_close(D_USERFILE);
  disp_index(0);
 
  if (!Bnmr)
  {
     releasevarlist();
     appendvarlist("dconi");
     Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
  }
  RETURN;
}


/*-----------------------------------------------
|                                               |
|               convertfids()/2                 |
|                                               |
|  This function converts FID's from 16 or 32   |
|  bit acquisition format to DATA file format.  |
|  The input is a set of individual FID's from  |
|  ACQFIL/FID.  The output is placed into the   |
|  file DATDIR/DATA.  This function operates    |
|  on the complete data set by calling the      |
|  function "convertblock()".  This function    |
|  is used in 'df2d'.                           |
|                                               |
+----------------------------------------------*/
static int convertfids(ftparInfo *ftpar, dfilehead *fidhead)
{
  int fidskip,
        curblock,
        firstfid,
        lastfid,
	nf_firstfid;


  nf_firstfid=0;
  if (ftpar->D_dimname & S_NF)
  {
     firstfid = ftpar->arrayindex + (ftpar->t1_offset*ftpar->arraydim);
     nf_firstfid = firstfid;
     fidskip = ftpar->cfstep;
     ftpar->cf = 1;
     if (ftpar->D_dimname & S_NI) /* 3D */
	firstfid += (ftpar->t2_offset*ftpar->ni0*ftpar->arraydim);

     lastfid = firstfid + (ftpar->ni0 * fidskip);
  }
  else
  { /* this handles (ni,ni2) double internal arrays for 3D */
     firstfid = ftpar->arrayindex + (ftpar->t1_offset*ftpar->arraydim)
                + (ftpar->t2_offset*ftpar->ni1*ftpar->arraydim);
     fidskip = ftpar->arraydim;
     if (ftpar->D_dimname & S_NI2)
	fidskip *= ftpar->ni1;

     lastfid = firstfid + (ftpar->ni0 * fidskip);
  }
  for (curblock = 0; curblock < ftpar->nblocks; curblock++)
  {
     disp_index(ftpar->nblocks - curblock);
     if ( convertblock(&firstfid, curblock, &lastfid, fidskip,
                ftpar, fidhead, nf_firstfid) )
     {
        return(ERROR);
     }
 
     if (interuption)
     {
        D_trash(D_DATAFILE);
        D_trash(D_PHASFILE);
        return(ERROR);
     }
  }
 
  return(COMPLETE);
}

/*---------------------------------------
|                                       |
|              scalefid()/0             |
|                                       |
+--------------------------------------*/
static void scalefid(register float *outp, register int pts)
{
   while (pts--)
      *outp++ *= DF2D_SCALE;
}


/*-----------------------------------------------
|                                               |
|               convertblock()/6                |
|                                               |
|  This function converts FID's from 16 or 32   |
|  bit acquisition format into the DATA file    |
|  format.  The ;input is a set of individual   |
|  FID's from ACQFIL/FID.  The output is put    |
|  into the file DATDIR/DATA.  This routine     |
|  does one block in DATDIR/DATA.               |
|                                               |
+----------------------------------------------*/
/*  curfid      number of current fid to start conversion    */
/*  fidincr     fid increment factor                         */
/*  blocknum    DATA block number to store converted data    */
static int convertblock(int *curfid, int blocknum, int *lastfid, int fidincr,
                ftparInfo *ftpar, dfilehead *fidhead, int nf_firstfid)
{
  int                   status,
                        res,
                        outfile,
                        npadj,
			fidnum,
			zfnumber;
  register int          fidcnt;
  register float        *outp;
  dpointers             outblock;
 
 
  outfile = D_DATAFILE;
 
/****************************************
*  Block "blocknum+nblocks" is used in  *
*  D_DATAFILE to transpose data.        *
****************************************/
 
  if ( (res = D_allocbuf(outfile, blocknum + ftpar->nblocks, &outblock)) )
  {
     D_error(res);
     return(ERROR);
  }
 
  outp = (float *)outblock.data;
/*************************************************
*  Start filling at the beginning of the output  *
*  data buffer.                                  *
*                                                *
*  np0 = the number of actual points in the FID  *
*  fn0 = the number of actual points in the      *
*        converted file                          *
*************************************************/
 
  npadj = ( (ftpar->lsfid0 < 0) ? ftpar->np0 : (ftpar->np0 - ftpar->lsfid0) );
  if (npadj > ftpar->fn0)
     npadj = ftpar->fn0;
  zfnumber = ftpar->fn0 - npadj;
 
/*********************************************
*  DF2D of NF-arrayed 2D data will not work  *
*  at this time.                             *
*********************************************/
  *lastfid = *curfid + ftpar->sperblock0*fidincr;
  for (fidcnt = *curfid; fidcnt < *lastfid; fidcnt += fidincr)
  {
     fidnum = ( (ftpar->D_dimname & S_NF) ? nf_firstfid : fidcnt );
     if ( getfid(fidnum, outp, ftpar, fidhead, lastfid) )
        return(ERROR);
 
     if (*lastfid == 0)
        return(ERROR);
 
     if (fidcnt < (*lastfid))
     {
        if (zfnumber > 0)
           zerofill(outp + npadj, zfnumber);
        scalefid(outp,npadj);
	if (ftpar->D_dimname & S_NF)
           ftpar->cf += ftpar->cfstep;/* increment cf for NF processing */
     }
     outp += ftpar->fn0;
  }
 
  fidnum = fidcnt;
  for (fidcnt = *lastfid + fidincr; fidcnt < *curfid + ftpar->sperblock0*fidincr;
       fidcnt += fidincr)
  {
     zerofill(outp, ftpar->fn0);
     outp += ftpar->fn0;
  }
  *curfid = fidnum;
  status = (S_DATA|S_FLOAT|S_COMPLEX);
     
  setheader(&outblock, status, ftpar->D_dsplymode, blocknum,
                ftpar->hypercomplex);
  if ( (res = D_markupdated(outfile, blocknum + ftpar->nblocks)) )
  {
     D_error(res);
     return(ERROR);
  }
 
  if ( (res = D_release(outfile, blocknum + ftpar->nblocks)) )
  {
     D_error(res);
     return(ERROR);
  }
 
  return(COMPLETE);
}

static double oversamp_lp = 0.0;

double get_dsp_lp()
{
   return(oversamp_lp);
}

/*---------------------------------------
|                                       |
|              getfid()/5               |
|                                       |
+--------------------------------------*/
int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead, int *lastfid)
{
   short                *data,
                        *inp16;
   int                 *inp32;
   register int         i,
                        npx;
   int                  shift,
                        res;
   register float       *inpfloat,
                        rmult;
   register float       *tmp;
   dpointers            inblock;
   static float		xoff,
			yoff;
   int                  showMsg = 1;


/*  i_ft sets the correct values for cf and nf fields in ftpar  */

   if (curfid >= (*lastfid) || ftpar->cf > ftpar->nf)
   {
      zerofill(outp, ftpar->fn0);
   }
   else
   {
      if ( (res = D_getbuf(D_USERFILE, fidhead->nblocks,  curfid, &inblock)) )
      {
         *lastfid = curfid;
         if (ftpar->arraydim > 0)
         {
            if ( ftpar->np1 > 2*(curfid/ftpar->arraydim) )
               ftpar->np1 = 2*(curfid/ftpar->arraydim);
                                /* adjusts number of t1 points */
         }
 
         zerofill(outp, ftpar->fn0);
         D_close(D_USERFILE);

         if (*lastfid == 0)   
         {
            Werrprintf("No data in FID file");
         }
         else
         {               
            if (!acqflag)
               Winfoprintf("number of FID's used = %d", *lastfid);
         }
 
         return(COMPLETE);
      }

/************************************************************
*  Convert the data of each FID.  Check to see if FID data  *
*  are double precision or single precision and convert     *
*  appropriately.  Correct FID size for CT and scaling.     *
************************************************************/
      if (inblock.head->lpval != 0)
        ftpar->lpval = inblock.head->lpval;
/* Save this so other programs like addsub and wti can get it */
      oversamp_lp = inblock.head->lpval;
      ftpar->dspar.lvl = inblock.head->lvl;
      ftpar->dspar.tlt = inblock.head->tlt;
      ftpar->dspar.scale = inblock.head->scale;
      ftpar->dspar.ctcount = inblock.head->ctcount;

      data = (short *) (inblock.data);                        
      if (ftpar->cf > 1)
      {
         data += (ftpar->cf - 1)*ftpar->np0;
         if (ftpar->dpflag)
            data += (ftpar->cf - 1)*ftpar->np0;
      }

      if ( (ftpar->np0 - ftpar->lsfid0) > ftpar->fn0 )
         ftpar->np0 = ftpar->fn0 + ftpar->lsfid0;


      if (inblock.head->status == (S_DATA|S_FLOAT|S_COMPLEX))
      {
         inpfloat = (float *) (data);

         if (ftpar->lsfid0 < 0)
         {
            inpfloat += ftpar->np0;
            tmp = outp + ftpar->np0 - ftpar->lsfid0;
            npx = ftpar->np0;
            if ((inblock.head->ctcount > 1) || (inblock.head->scale) )
            {
               shift = 1 << abs(inblock.head->scale);
               if (inblock.head->scale < 0)
                  rmult = 1.0/(float)(shift);
               else
                  rmult = (float) shift;
               rmult /= (float)(inblock.head->ctcount);
               for (i = 0; i < npx; i++)
                  *(--tmp) = *(--inpfloat) * rmult;
            }
            else
            {
               for (i = 0; i < npx; i++)
                  *(--tmp) = *(--inpfloat);
            }

            npx = (-1)*ftpar->lsfid0;
            for (i = 0; i < npx; i++)
               *(--tmp) = 0.0;
         }
         else
         {
            inpfloat += ftpar->lsfid0;
            tmp = outp;
            npx = ftpar->np0 - ftpar->lsfid0;
            if ((inblock.head->ctcount > 1) || (inblock.head->scale) )
            {
               shift = 1 << abs(inblock.head->scale);
               if (inblock.head->scale < 0)
                  rmult = 1.0/(float)(shift);
               else
                  rmult = (float) shift;
               rmult /= (float)(inblock.head->ctcount);
               for (i = 0; i < npx; i++)
                  *tmp++ = *inpfloat++ * rmult;
            }
            else
            {
               for (i = 0; i < npx; i++)
                  *tmp++ = *inpfloat++;
            }
         }
      }
      else
      {
         if (inblock.head->ctcount == 0)
         {
            inblock.head->ctcount = 1;
            inblock.head->status = 0;
            *lastfid = curfid;
            if (ftpar->arraydim > 0)
            {
               if ( ftpar->np1 > 2*(curfid/ftpar->arraydim) )
                  ftpar->np1 = 2*(curfid/ftpar->arraydim);
            }
 
            if (!acqflag)
            {
               if ( *lastfid )
                  Winfoprintf("number of FID's used = %d", *lastfid);
               else
                  Winfoprintf("No data in FID file");
               showMsg = 0;
            }
         }

         inp16 = (short *) (inp32 = (int *) (data));
         shift = 1 << abs(inblock.head->scale);
         if (inblock.head->scale < 0)
            rmult = 1.0/(float)(shift);
         else
            rmult = (float) shift;
         rmult /= (float)(inblock.head->ctcount);
         if (inblock.head->status == (S_DATA|S_32|S_COMPLEX))
         {
            cnvrts32(rmult, inp32, outp, ftpar->np0, ftpar->lsfid0);
         }
         else if (inblock.head->status == (S_DATA|S_COMPLEX))
         {
            cnvrts16(rmult, inp16, outp, ftpar->np0, ftpar->lsfid0);
         }
         else
         {
            if (inblock.head->status != 0)
            {
               Wscrprintf("status of FID %d incorrect, status = %d\n",
                     curfid + 1, inblock.head->status);
            }
 
            zerofill(outp, ftpar->fn0);
            if (ftpar->arraydim > 0)
            {
               if ( ftpar->np1 > 2*(curfid/ftpar->arraydim) )
                  ftpar->np1 = 2*(curfid/ftpar->arraydim);
            }
 
            *lastfid = curfid;
            if ( !acqflag && showMsg )
            {
               if ( *lastfid )
                  Winfoprintf("number of FID's used = %d", *lastfid);
               else
                  Winfoprintf("No data in FID file");
            }
         }
      }

/*---------------------------------------------
|     if requested remove the baseline        |
|     supplied by noise check                 |
---------------------------------------------*/
 
     if ( ftpar->offset_flag && (inblock.head->ctcount == 1) )
     {
        if (curfid == 0)
        {
           xoff = inblock.head->lvl;
           yoff = inblock.head->tlt;
        }
 
        if (ftpar->lsfid0 < 0)
        {
           tmp = outp - ftpar->lsfid0;
           npx = ftpar->np0;
        }
        else
        {
           tmp = outp;
           npx = ftpar->np0 - ftpar->lsfid0;
        }

        i = 0;
        while (i < npx)
        {  
           *tmp++ -= xoff;
           *tmp++ -= yoff;
           i += 2;
        }
      }
/*    end of prototype baseline removal */

#ifdef XXX
      if ( !P_getreal(CURRENT, "rlmult", &rlmult, 1) )
      {
         if ( !P_getreal(CURRENT, "immult", &immult, 1) )
         {
            if (ftpar->lsfid0 < 0)
            {
               tmp = outp - ftpar->lsfid0;
               npx = ftpar->np0;
            }
            else
            {   
               tmp = outp;
               npx = ftpar->np0 - ftpar->lsfid0;
            }

            i = 0;
            while (i < npx)
            {
               *tmp++ *= rlmult;
               *tmp++ *= immult;
               i += 2;
            }
         }
      }
#endif
/* end of prototype differential channel scaling */

      if (ftpar->t2dc)
      {
        driftcorrect_fid(outp, ftpar->np0/2, ftpar->lsfid0/2, COMPLEX);
      }
 
      if ( (fabs(ftpar->phfid0) > MINDEGREE) ||
	   (fabs(ftpar->lsfrq0) > 1e-20) )
      {
         rotate_fid(outp, ftpar->phfid0, ftpar->lsfrq0,
			ftpar->np0 - ftpar->lsfid0, COMPLEX);
      }

      if (ftpar->zeroflag)
      {
         if (ftpar->zeroflag > 0)
            zeroimag(outp, ftpar->np0 - ftpar->lsfid0, TRUE);
         else
            negateimaginary(outp, (ftpar->np0 - ftpar->lsfid0) / 2, COMPLEX);
      }

      if ( (res = D_release(D_USERFILE, curfid)) )
      {
         D_error(res);
         D_close(D_USERFILE);
         return(ERROR);
      }
   }

   long_event();
   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|               get_one_fid()/3                 |
|                                               |
|    This function returns a pointer to the     |
|    requested FID in a 1D, arrayed, or 2D      |
|    experiment.                                |
|                                               |
+----------------------------------------------*/
float *get_one_fid(int curfid, int *np, dpointers *c_block, int dcflag)
{
  char          filepath[MAXPATHL],
                dcrmv[4];
  int           res,
                lastfid,
                force_getfid,
                headok;
  float         *fidptr;
  int           cftemp;
  int           cttemp;
  double        tmp;
  vInfo         info;
  ftparInfo	ftpar;
  dfilehead	fidhead,
		phasehead;
 
 
  force_getfid = (*np < 0);
  if (force_getfid)
     *np = -(*np);
  acqflag = FALSE;
  ftpar.np0 = *np;
  ftpar.fn0 = *np;
  ftpar.hypercomplex = FALSE;	/* ==> will not work for hypercomplex
				   2D interferograms */
  D_allrelease();
 
  if ( (res = D_gethead(D_PHASFILE, &phasehead)) )
  {
     if (res == D_NOTOPEN)
     {
        if ( (res = D_getfilepath(D_PHASFILE, filepath, curexpdir)) )
        {
           D_error(res);
           return(NULL);
        }

        res = D_open(D_PHASFILE, filepath, &phasehead); /* open the file */
     }
 
     if (res)
     {
        if ( new_phasefile(&phasehead, 0, 0, 0, 0, 0, ftpar.hypercomplex) )
           return(NULL);
     }
  }
 
  cftemp = 1;
  if (!P_getreal(CURRENT, "cf", &tmp, 1))
  {
     if (!P_getVarInfo(CURRENT, "cf", &info))
     {
        if (info.active)
           cftemp = (int) tmp;
     }
  }
  cttemp = 0;
  if (!P_getreal(PROCESSED, "ct", &tmp, 1))
     cttemp = (int) (tmp + 0.5);

  ls_ph_fid("lsfid", &(ftpar.lsfid0), "phfid", &(ftpar.phfid0), "lsfrq",
		&(ftpar.lsfrq0));
  headok = ( (phasehead.status == (S_DATA|S_FLOAT|S_COMPLEX)) &&
            (phasehead.ntraces == 1) && (phasehead.np == ftpar.np0) );
 
  if (headok)
  { /* if phase file does contain fid data, open the file */
     res = D_getbuf(D_PHASFILE, phasehead.nblocks, curfid, c_block);
 
     if (!res)
     {
        if ( (c_block->head->status == (S_DATA|S_FLOAT|S_COMPLEX)) &&
             (c_block->head->rpval == (float) (ftpar.phfid0)) &&
             (c_block->head->lpval == (float) (ftpar.lsfid0/2)) &&
             (c_block->head->lvl   == (float) (cftemp)) &&
             (c_block->head->tlt   == (float) (cttemp)) &&
             !force_getfid )
        {
           long_event();
           return((float *)c_block->data);
        }
     }
  }
 
/********************************************
*  If phasefile does not contain FID data,  *
*  open phasefile with the data handler.    *
********************************************/

  ftpar.zeroflag = FALSE;
  ftpar.arraydim = dim1count();
  lastfid = ftpar.arraydim;
  ftpar.fn0 = ftpar.np0;
  if ( i_fid(&fidhead, &ftpar) )   /* open fid file with data handler */
     return(NULL);
  if (ftpar.fn0 != ftpar.np0)
     headok = 0;

  *np = ftpar.np0;
  ftpar.fn0 = ftpar.np0;
 
  if (!headok)
  {
     if (new_phasefile(&phasehead, 0, ftpar.arraydim, 2*ftpar.np0, 1,
                    (S_DATA|S_FLOAT|S_COMPLEX), ftpar.hypercomplex))
     {
        return(NULL);
     }
  }
 
  if ( (res = D_allocbuf(D_PHASFILE, curfid, c_block)) )
  {
     D_error(res);
     return(NULL);
  }
 
  fidptr = (float *)c_block->data;
 
  /*******************************************
  *   provision for baseline offset removal  *
  *   using numbers reported by noise check  *
  *******************************************/

  ftpar.offset_flag = FALSE;
  if (!P_getstring(CURRENT,"dcrmv",dcrmv,1,4))
  {
    if (dcrmv[0] == 'y')
    {
      ftpar.offset_flag = TRUE;
    }
  }
  ftpar.t2dc = dcflag;
     
  if ( getfid(curfid, fidptr, &ftpar, &fidhead, &lastfid) ||
        (lastfid <= curfid) )
  {
     return(NULL);
  }
 
  if (ftpar.lsfid0 > 0)
     zerofill(fidptr + ftpar.np0 - ftpar.lsfid0, ftpar.lsfid0);
 
  D_close(D_USERFILE);
  setheader(c_block, (S_DATA|S_FLOAT|S_COMPLEX), NP_PHMODE, curfid,
                ftpar.hypercomplex);
  c_block->head->rpval = (float) (ftpar.phfid0);
  c_block->head->lpval = (float) (ftpar.lsfid0/2);
  c_block->head->lvl   = (float) (ftpar.cf);
  c_block->head->tlt   = (float) (ftpar.dspar.ctcount);
 
  if ( ftpar.cf != cftemp)
  {  /* cf is misset or inactive - just set it = 1 */
     Werrprintf("cf = %d is inconsistent with data",cftemp);
     P_setreal(CURRENT,  "cf", 1.0, 0);
     P_setreal(PROCESSED,"cf", 1.0, 0);
  }
  if ( (res = D_markupdated(D_PHASFILE, curfid)) )
  {
     D_error(res);
     return(NULL);
  }
 
  return(fidptr);
}
