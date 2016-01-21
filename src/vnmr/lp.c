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
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "data.h"
#include "ftpar.h"
#include "process.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "allocate.h"
#include "buttons.h"
#include "displayops.h"
#include "fft.h"
#include "pvars.h"
#include "sky.h"
#include "tools.h"
#include "params.h"
#include "wjunk.h"

#ifdef  DEBUG
extern int      debug1;
#define DPRINT(str) \
        if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1) \
        if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
        if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
        if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
        if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4)
#else 
#define DPRINT(str)
#define DPRINT1(str, arg2)
#define DPRINT2(str, arg1, arg2)
#define DPRINT3(str, arg1, arg2, arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4)
#endif 
 

#define COMPLETE	0
#define ERROR		1
#define MAXINT		0x7FFFFFFF	/* maximum integer	*/
#define APODIZE		8
#define FALSE		0
#define TRUE		1
#define CMPLX_DC	8
#define MAXSTR		256

#define zerofill(data_pntr, npoints_to_fill)                    \
                        datafill(data_pntr, npoints_to_fill,    \
                                0.0)

static lpinfo	lpparams;

extern int	interuption,
		Bnmr,
		specIndex,
		c_first,
		c_last,
		c_buffer;

extern double	fpointmult;
extern double getfpmult(int fdimname, int ddr);
extern int setlppar(lpstruct *parLPinfo, int dimen, int pstatus,
             int nctdpts, int ncfdpts, int mode, char *memId);
extern int disp_current_seq();
extern void setheader(dpointers *block, int status_value, int mode_value,
               int index_value, int hypercomplex);
extern int i_ft(int argc, char *argv[], int setstatus, int checkstatus, int flag2d,
                ftparInfo *ftpar, dfilehead *fidhead, dfilehead *datahead,
                dfilehead *phasehead);
extern int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead,
                int *lastfid);
extern int lpz(int trace, float *data, int nctdpts, lpinfo lpparvals);
extern void currentDate(char* cstr, int len );
extern int saveProcpar(char *path); 

/*-----------------------------------------------
|						|
|		    lpcmd()			|
|						|
|  This function performs 1D LP on a single	|
|  or arrayed FID data set. A new fid file      |
|  is write to argv[1] or curexpdir/lp.fid      |
|  if not specified.                            |
|						|
+----------------------------------------------*/
int lpcmd(int argc, char *argv[], int retc, char *retv[])
{
  char		filepath[MAXPATHL];
  int		status,
		res,
		cblock,
		lastcblock,
		blocksdone,
		lsfidx,
		fidnum = 0,
		arg_no,
		npx,
		npadj,
		ftflag,
		noreal,
		element_no,
		lastfid,
		first,
		last,
		step,
		i,
		ctcount,
		realt2data;
  float		*outp;
  dpointers	inblock;
  dpointers	outblock;
  dfilehead	fidhead,
		datahead,
		phasehead;
  lpstruct	parLPinfo;
  ftparInfo	ftpar;
  char		newfidpath[MAXPATHL];

  Wturnoff_buttons();
  ftpar.procstatus = (CMPLX_t2|LP_F2PROC);

/************************************
*  Initialize all parameterizeable  *
*  variables                        *
************************************/

  arg_no = first = step = element_no = 1;
  ftpar.nblocks = MAXINT;
  last = MAXINT;

  noreal = ftflag = TRUE;
  ftpar.t2dc = -1;
  ftpar.zeroflag = FALSE;
  ftpar.sspar.lfsflag = ftpar.sspar.zfsflag = FALSE;
  ftpar.dspar.dsflag = FALSE;
  ftpar.dspar.fileflag = FALSE;
  ftpar.dspar.newpath[0] = '\0';
  ftpar.ftarg.useFtargs = 0;
 
  // default newfidpath
  sprintf(newfidpath,"%s/lp.fid",curexpdir);

/*********************************
*  Parse STRING arguments first  *
*********************************/

  while ( (argc > arg_no) && (noreal = !isReal(argv[arg_no])) )
  {
     if(strcmp(argv[arg_no],"rlp") == 0) { // e.g., lp('/tmp/lp.fid')
       ftpar.procstatus = (REAL_t2|LP_F2PROC);
     } else if(argv[arg_no][0] == '/') { // e.g., lp('/tmp/lp.fid')
        strcpy(newfidpath,argv[arg_no]);
     } else {
        sprintf(newfidpath,"%s/%s",curexpdir,argv[arg_no]);
     }
     arg_no++;
  }
  if(strcmp(newfidpath+strlen(newfidpath)-4,".fid") != 0 &&
     strcmp(newfidpath+strlen(newfidpath)-5,".fid/") != 0) strcat(newfidpath,".fid");
  
/******************************
*  Initialize data files and  *
*  FT parameters.             *
******************************/

  if ( i_ft(argc, argv, (S_DATA | S_FLOAT),
            0, 0, &ftpar, &fidhead, &datahead, &phasehead) )
  {
      disp_status("        ");
      ABORT;
  }
  
  if ( (res = D_getbuf(D_USERFILE, fidhead.nblocks,  0, &inblock)) ) { 
    ctcount=1;
  } else ctcount = inblock.head->ctcount; 

  if(ctcount<1) ctcount=1;

  if (ftpar.t2dc == -1)
  {
      ftpar.t2dc = (fidhead.status & S_DDR) ? FALSE : TRUE;
  }
  disp_current_seq();

  ftpar.cf = first;

     specIndex = first;		/* tells interactive programs
				   that new data exist */
/***************************************************
*  np0  =  total number of points in the fid       *
*  npx  =  used number of points in the fid        *
*                                                  *
*  Adjust "npx" and "lsfidx".                      *
***************************************************/

  lsfidx = ftpar.lsfid0;

    npx = ftpar.np0;
    npadj = npx - lsfidx;     /* adjusted number of FID data points */

  if (lsfidx < 0)
  {
     if (npx < 2)
     {
        Werrprintf("lsfid is too large in magnitude");
        ABORT;
     }
  }
  else
  {
     if (lsfidx >= npx)
     {
        Werrprintf("lsfid is too large in magnitude");
        ABORT;
     }
  }

  realt2data = (ftpar.procstatus & REAL_t2);
  fpointmult = getfpmult(S_NP, fidhead.status & S_DDR);

  // note, limit for forward LP is npadj (5th arg)
  if ( setlppar(&parLPinfo, S_NP, ftpar.procstatus, npadj/2, MAXINT,
		  LPALLOC, "ft2d") )
  {
     disp_status("        ");
     releaseAllWithId("ft2d");
     Wsetgraphicsdisplay("");		/* TRY THIS!  SF */
     ABORT;
  }

  if (parLPinfo.sizeLP)
  {
     int	maxlpnp,
		nptmp;

     if (realt2data)
     {
        Werrprintf("LP analysis is not supported for real t2 data");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     maxlpnp = npadj;

     for (i = 0; i < parLPinfo.sizeLP; i++)
     {
        lpparams = *(parLPinfo.parLP + i);

        if (lpparams.status & FORWARD)
        {
           nptmp = 2*(lpparams.startextpt + lpparams.ncextpt - 1);
           if (nptmp > maxlpnp)
              maxlpnp = nptmp;
        }
     }

     npadj = maxlpnp;
  }

/**************************
*  Start loop over FIDs.  *
**************************/

  lastfid = ftpar.ni0 * ftpar.ni1 * ftpar.arraydim;
     if (lastfid > ftpar.nblocks)
        lastfid = ftpar.nblocks;
     if (first > lastfid)
     {
        last = lastfid;
     }

  D_trash(D_DATAFILE);
  if(access(newfidpath,F_OK) != 0) {
   if(mkdir(newfidpath,0777)) {
     Winfoprintf("cannot create %s",newfidpath);
     ABORT;
   }
  }

  status = (S_DATA | S_FLOAT | S_COMPLEX | ftpar.D_cmplx);

  // now write out fid to newfidpath
  strcpy(filepath,newfidpath); 
  strcat(filepath,"/fid");
  
  datahead.status = fidhead.status;
  datahead.vers_id = fidhead.vers_id;
  datahead.nbheaders = fidhead.nbheaders;
  datahead.nblocks = fidhead.nblocks;
  datahead.ntraces = 1;
  datahead.np = npadj; 
  datahead.ebytes = fidhead.ebytes;
  datahead.tbytes = npadj*fidhead.ebytes;
  datahead.bbytes = datahead.tbytes + sizeof(dblockhead);;
   
  if (D_newhead(D_DATAFILE, filepath, &datahead) )
  {
     Werrprintf("cannot open file %s", filepath);
     ABORT;
  }

/***********************************************
*  Necessary until I can devise a function to  *
*  read only the FID block header in order to  *
*  determine if FID data exists in that block. *
***********************************************/

  blocksdone = FALSE;
  lastcblock = ftpar.nblocks;

  cblock = 0;

  while ((cblock < lastcblock) && (!blocksdone))
  {
     DPRINT1("block %d\n", cblock);
     if ( (res = D_allocbuf(D_DATAFILE, cblock, &outblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }
     
     outblock.head->ctcount = ctcount;  /* default setting */
     outblock.head->scale = 0;    /* default setting */

       outp = (float *)outblock.data;

/**********************************************************
*  Start filling at the start of the output data buffer.  *
*  This facilitates the automatic array-like processing   *
*  of 'cf' and 'nf' in 1D.                                *
**********************************************************/

        fidnum = cblock;
     
     if (interuption)
     {
        releaseAllWithId("ft2d");
        D_trash(D_PHASFILE);
        D_trash(D_DATAFILE);
        disp_status("        ");
        ABORT;
     }

     if ((cblock + 1) == first)
     {
        if ( ! ((cblock+1) & 15) )
           disp_index(cblock + 1);
        if ( getfid(fidnum, outp, &ftpar, &fidhead, &lastfid) )
	{

	   Werrprintf("Unable to get FID data");
           releaseAllWithId("ft2d");
	   disp_index(0);
           disp_status("        ");
           ABORT;
        }

	if (lastfid == 0)
	{
           releaseAllWithId("ft2d");
	   disp_index(0);
	   disp_status("        ");
           ABORT;
        }
        else if (cblock == lastfid)
	{
           status = 0;
           blocksdone = TRUE;
           if (last == MAXINT)
           {
              last = cblock;
           }
	}
        else
        {
           if (parLPinfo.sizeLP)
           {
              for (i = 0; i < parLPinfo.sizeLP; i++)
              {
                 lpparams = *(parLPinfo.parLP + i);
                 disp_status(lpparams.label);

                 if (lpz(fidnum, outp, (npx - lsfidx)/2, lpparams))
                 {
                    Werrprintf("LP analysis failed");
                    releaseAllWithId("ft2d");
                    disp_index(0);
                    disp_status("       ");
                    ABORT;
                 }
              }
           }

           if (fpointmult != 1.0)
           {
              *outp *= fpointmult;
              *(outp + 1) *= fpointmult;
           }

           //zerofill(outp + npadj, ftpar.fn0 - npadj);

           last = cblock + 1;
        }

        // multiply outp by ctcount
        outp = (float *)outblock.data;
        outp += npadj;
        for(i = 0; i < npadj; i++)
	  *(--outp) *= ctcount;
       
        setheader(&outblock, status, 0, cblock+1,
			ftpar.hypercomplex);
        first += step;
     }
     else
     {
        setheader(&outblock, 0, 0, cblock, ftpar.hypercomplex);
     }

     if ( (res = D_markupdated(D_DATAFILE, cblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     if ( (res = D_release(D_DATAFILE, cblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     cblock++;
     if (!blocksdone)
        blocksdone = (first > lastcblock);
  }

  if ( (last != ftpar.nblocks) && (last != MAXINT) )
  {
     if ( (res = D_gethead(D_DATAFILE, &datahead)) )
     {
        D_error(res);
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     datahead.nblocks = last;
     if ( (res = D_updatehead(D_DATAFILE, &datahead)) )
     {
        D_error(res);
        disp_index(0);
        disp_status("        ");
        ABORT;
     }
  }

  releasevarlist();
  releaseAllWithId("ft2d");
  disp_index(0);
  disp_status("    ");
  D_close(D_USERFILE);
  D_flush(D_DATAFILE);
  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);

  // save procpar
  strcpy(filepath,newfidpath); 
  strcat(filepath,"/procpar");
  if(npx == npadj) {
     saveProcpar(filepath);
  } else {
     
     double sw,at, oldat;
     P_getreal(PROCESSED,"at", &oldat, 1);
     P_setreal(PROCESSED,"np",(double)npadj,1);
     if(!P_getreal(PROCESSED,"sw",&sw,1) && sw > 0) {
        at = npadj/(2*sw);
        P_setreal(PROCESSED,"at",at,1);	
     }  
     saveProcpar(filepath);
     P_setreal(PROCESSED,"np",(double)npx,1);
     P_setreal(PROCESSED,"at",oldat,1);
  }

  RETURN;
}
