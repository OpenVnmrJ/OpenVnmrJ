/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <netinet/in.h>
#include "data.h"
#include "acqparms2.h"
#include "lc_gem.h"
#include "shrexpinfo.h"
#include "mfileObj.h"

#define PRTLEVEL 1
#define DEBUG
#ifdef  DEBUG
#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#endif

typedef	struct _aprecord {
        int		preg;	/* this is not saved */
        short		*apcarray /*[MAXARRAYSIZE+1] */;
} aprecord;

extern int           bgflag;
extern unsigned long start_elem;     /* elem for acquisition to (re)start on */
extern unsigned long completed_elem; /* total number of completed elements */
extern Acqparams     *Alc;
extern aprecord      apc;
extern SHR_EXP_STRUCT	ExpInfo;

static int                  acqfd;
static struct datafilehead  acqfileheader;
static struct datablockhead acqblockheader;
static int                  acqparmsize = sizeof(autodata) + sizeof(Acqparams);

static long max_ct;

static MFILE_ID ifile = NULL;	/* mercury datafile for ra */
static char *savedoffsetAddr;   /* saved mercury offset header */

/*-----------------------------------------------------------------
|	ra_initacqparms(fid#)
+---------------------------------------------------------------*/
ra_initacqparms(fidn)
unsigned long fidn;   /* fid number to obtain the acqpar parameters for */
{
   Acqparams *lc,*ra_lc;
   autodata *autoptr;
   char msge[256];
   int len1,len2;
   codeint acqbuffer[512];
   struct datablockhead acqblockheader;

   if (fidn == 1)
   {
     max_ct = 0L;
   }

   acqpar_seek(fidn);	/* move to proper fid entry in acqpar */
   len1 = read(acqfd,&acqblockheader,sizeof(struct datablockhead));
   len2 = read(acqfd,acqbuffer,acqparmsize);
   DPRINT3(PRTLEVEL,"ra_init: fid %lu, blockhdr %d, lc & auto %d\n",
		fidn,len1,len2);

   if ( (len1 == 0) || (len2 == 0) )
   {
      if (max_ct != -1L)  /* no start selected, 1st none acquire fid is start */
      {
        start_elem = fidn;
        max_ct = -1L;           /* only set start_elem once */
        DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	  fidn,start_elem,max_ct);
      }
      return(0);	/* no further data */
   }
 
   ra_lc = (Acqparams *) acqbuffer; /* ra lc parameters */
/*   lc = (Acqparams *) codestadr; /* initialize lc parameters */
   lc = (Acqparams *) apc.apcarray; /* initialize lc parameters */

   DPRINT2(PRTLEVEL,"lc-> 0x%lx, ra_lc-> 0x%lx\n",lc,ra_lc);
   DPRINT2(PRTLEVEL,"ct=%ld, ra ct=%ld\n",lc->ct,ra_lc->ct);
   DPRINT2(PRTLEVEL,"np=%ld, ra np=%ld\n",lc->np,ra_lc->np);
   DPRINT2(PRTLEVEL,"nt=%ld, ra nt=%ld\n",lc->nt,ra_lc->nt);

   /* find the maximum ct acquired without actually completing the FID */
   /* the assumption here is that fids are acquired 1 to arraydim */
   /* i.e., 1st fid ct>0 is max ct acquired so far */
   if ( (ra_lc->ct != ra_lc->nt) && (max_ct == 0L) )
   {
      max_ct = ra_lc->ct;
      start_elem = fidn;        /* maybe start_elem */
   }

   DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	fidn,start_elem,max_ct);
 
   DPRINT1(PRTLEVEL,"il='%s'\n",il);
   if ( (max_ct != 0L) && (max_ct != -1L) && (ra_lc->ct != ra_lc->nt) )
   {
     /* first elem with bs!=bsct or ct < max_ct , if il!='n' is the start_elem */
     if ( (il[0] != 'n') && (il[0] != 'N') )
     {
       if ( (ra_lc->bsct != ra_lc->bs) || (ra_lc->ct < max_ct) )
       {
          start_elem = fidn;
          max_ct = -1L;           /* only set start_elem once */
       }
     }
     else
     {
       start_elem = fidn;
       max_ct = -1L;           /* only set start_elem once */
     }
   }

   DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	fidn,start_elem,max_ct);
 

   if (ra_lc->ct == ra_lc->nt)
   {
      lc->nt = ra_lc->nt; /* nt can not be change for completed fids */
      completed_elem++;         /* total number of completed elements (FIDs)  (RA) */
   }
   else if ( lc->nt <= ra_lc->ct )
    {
      lc->nt = ra_lc->nt; /* nt can not be made <= to ct , for now. */
      sprintf(msge,"WARNING: FID:%d  'nt' <= 'ct', original 'nt' used.\n",
		fidn);
      text_error(msge);
    }
 
   DPRINT2(PRTLEVEL,"nt=%ld, ra nt=%ld\n",lc->nt,ra_lc->nt);
   if (ra_lc->np != lc->np)
   {
      sprintf(msge,"WARNING 'np' has changed from %ld to %ld.\n",
		ra_lc->np,lc->np);
      text_error(msge);
   }
   if (ra_lc->dpf != lc->dpf)
   {
      abort_message("data precision changed, PSG aborting.");
   }
   lc->ct = ra_lc->ct;
   lc->isum = ra_lc->isum;
   lc->rsum = ra_lc->rsum;
   lc->stmar = ra_lc->stmar;
   lc->stmcr = ra_lc->stmcr;
   DPRINT2(PRTLEVEL,"maxscale %hd, ra %hd\n",lc->maxscale,ra_lc->maxscale);
   lc->maxscale = ra_lc->maxscale;
   lc->icmode = ra_lc->icmode;
   lc->stmchk = ra_lc->stmchk;
   lc->nflag = ra_lc->nflag;
   DPRINT2(PRTLEVEL,"scale %hd, ra %hd\n",lc->scale,ra_lc->scale);
   lc->scale = ra_lc->scale;
   lc->check = ra_lc->check;
   lc->oph = ra_lc->oph;
   lc->bsct = ra_lc->bsct;
   lc->ssct = ra_lc->ssct;
   lc->ctcom = ra_lc->ctcom;
   lc->tablert = ra_lc->tablert;
   lc->v1 = ra_lc->v1;
   lc->v2 = ra_lc->v2;
   lc->v3 = ra_lc->v3;
   lc->v4 = ra_lc->v4;
   lc->v5 = ra_lc->v5;
   lc->v6 = ra_lc->v6;
   lc->v7 = ra_lc->v7;
   lc->v8 = ra_lc->v8;
   lc->v9 = ra_lc->v9;
   lc->v10 = ra_lc->v10;
   lc->v11 = ra_lc->v11;
   lc->v12 = ra_lc->v12;
   lc->v13 = ra_lc->v13;
   lc->v14 = ra_lc->v14;
   lc->rtvptr = (lc->nt - (lc->ssct % lc->nt) + lc->ct) % lc->nt;

   return(1); /* data found and lc updated */
}
/*---------------------------------------------------------------------
|       open_acqpar(path)/1
|
|       Position the disk read/write heads to the proper block offset
|       for the acqpar file (lc,auto structure parameters.
+-------------------------------------------------------------------*/
open_acqpar(filename)
char *filename;
{
   int len;
   if (bgflag)
     fprintf(stderr,"Opening Acqpar file: '%s' \n",filename);
   acqfd = open(filename,O_EXCL | O_RDONLY ,0666);
   if (acqfd < 0)
   { abort_message("Cannot open acqpar file: '%s', RA not possible.\n",filename);
   }
   len = read(acqfd,&acqfileheader,sizeof(struct datafilehead));
   if (len < 1)
   { abort_message("Cannot read acqpar file: '%s'\n",filename);
   }
      
}

/*---------------------------------------------------------------------
|       acqpar_seek(index)/1
|
|       Position the disk read/write heads to the proper block offset
|       for the acqpar file (lc,auto structure parameters.
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.    1. Routine now returns the (long) position of seek
+-------------------------------------------------------------------*/
acqpar_seek(elemindx)
unsigned long elemindx;
{
    int acqoffset;
    long pos;

    acqoffset = sizeof(acqfileheader) +
        (acqfileheader.bbytes * ((unsigned long) (elemindx - 1)))  ;
    if (bgflag)
        fprintf(stderr,"acqpar_seek(): fid# = %d,acqoffset = %d \n",
           elemindx,acqoffset);
    pos = lseek(acqfd, acqoffset,L_SET);
    if (pos == -1)
    {
        char *str_err;

        if ( (str_err = strerror(errno) ) != NULL )
        {
           fprintf(stdout,"acqpar_seek Error: offset %ld,: %s\n",
               acqoffset,str_err);
        }
        return(-1);
    }    
    return(pos);
}

int ra_mercacqparms(fidn)
unsigned long fidn;
{
   char fidpath[512];
   int curct;

   sprintf(fidpath,"%s/fid",ExpInfo.DataFile);
   if (fidn == 1)
   {
     ifile = mOpen(fidpath,ExpInfo.DataSize,O_RDONLY );
     if (ifile == NULL) 
     {
	abort_message("Cannot open data file: '%s', RA aborted.\n",
		ExpInfo.DataFile);
     }
     /* read in file header */
     memcpy((void*) &acqfileheader, (void*) ifile->offsetAddr,
					sizeof(acqfileheader));
     if(ntohl(acqfileheader.np) != ExpInfo.NumDataPts)
     {
	abort_message("FidFile np: %d not equal NumDataPts: %d, RA aborted.\n",
		ntohl(acqfileheader.np),ExpInfo.NumDataPts);
     }
     if (ntohl(acqfileheader.ebytes) != ExpInfo.DataPtSize)
     {
	abort_message("FidFile dp: %d not equal DataPtSize: %d, RA aborted.\n",
		ntohl(acqfileheader.ebytes),ExpInfo.DataPtSize);
     }
     if (ntohl(acqfileheader.ntraces) != ExpInfo.NumFids)
     {
	abort_message("FidFile nf: %d not equal NumFids: %d, RA aborted.\n",
		ntohl(acqfileheader.ntraces),ExpInfo.NumFids);
     }
     ifile->offsetAddr += sizeof(acqfileheader);  /* move my file pointers */
     savedoffsetAddr = ifile->offsetAddr;
   }
   /* offset to fid location */
   if (fidn <= getStartFidNum())
   {
	ifile->offsetAddr = savedoffsetAddr +
		((ExpInfo.DataPtSize * ExpInfo.NumDataPts *
			ExpInfo.NumFids) + sizeof(acqblockheader)) * (fidn-1);
	memcpy((void*) &acqblockheader, (void*) ifile->offsetAddr,
					sizeof(acqblockheader));
	curct = ntohl(acqblockheader.ctcount);
	if (getIlFlag())
	{
	   if (fidn < getStartFidNum())
	   {
	      if (curct != (((ExpInfo.CurrentTran/ExpInfo.NumInBS)+1) * 
							ExpInfo.NumInBS))
	    text_error("Warning: File_ct(%d): %d not equal ct+bs: %d\n",
		fidn,curct,ExpInfo.CurrentTran+ExpInfo.NumInBS);
	   }
	   else
	   {
	      if (curct != ExpInfo.CurrentTran)
	    text_error("Warning: File_ct(%d): %d not equal ct: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
	   }
	}
	else
	{
	   if (fidn < getStartFidNum())
	   {
	      if (curct != ExpInfo.NumTrans)
	    text_error("Warning: File_ct(%d): %d not equal nt: %d\n",
		fidn,curct,ExpInfo.NumTrans);
	   }
	   else
	   {
	      if (curct != ExpInfo.CurrentTran)
	    text_error("Warning: File_ct(%d): %d not equal ct: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
	   }
	}

   }
   else if (getIlFlag() && (fidn > getStartFidNum()) && 
		(ExpInfo.CurrentTran >= ExpInfo.NumInBS))
   {
	/* Only when interleaving after 1st blocksize */
	ifile->offsetAddr = savedoffsetAddr +
		((ExpInfo.DataPtSize * ExpInfo.NumDataPts *
			ExpInfo.NumFids) + sizeof(acqblockheader)) * (fidn-1);
	memcpy((void*) &acqblockheader, (void*) ifile->offsetAddr,
					sizeof(acqblockheader));
	curct = ntohl(acqblockheader.ctcount);
	if (curct != ExpInfo.CurrentTran)
	  text_error("Warning: File_ct(%d): %d not equal CurrentTran: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
   }
   else curct = 0;

   if (fidn >= ExpInfo.ArrayDim)
        mClose(ifile);
   Alc->ct = curct;
}

