/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include "constant.h"
#include "coef3d.h"
#include "process.h"
#include "struct3d.h"
#include "lock3D.h"

#define FT3D
#include "command.h"
#include "data.h"
#include "wjunk.h"
#ifdef LINUX
#include "datac.h"
#endif
#undef FT3D

#include "fileio.h"


#define WORDS_PER_BUFUNIT	65536
#define OLD_S_COMPLEX		0x40	/* see "data.c" in Vnmr */


static off_t	compstbyte = 0;
static int      firstFID = TRUE;

extern char	*userdir;
extern int	maxfn12;	/* maximum F1-F2 real Fourier number	*/
extern int      mapFIDblock(int block_no);

/*---------------------------------------
|                                       |
|           readFIDdata()/12            |
|                                       |
+--------------------------------------*/
int readFIDdata(fd, data, wspace, filehead, nfidbytes, npadj, lsfval,
                        lastfid, fid_nbr, dpflag, fidmap, pf3acq, lpval)
char            *wspace;	/* pointer to workspace memory		*/
int             fd,		/* FID file descriptor			*/
		pf3acq,		/* Acq-F3proc flag			*/
                nfidbytes,	/* bytes per t3 FID			*/
                *lastfid,	/* last FID currently processed		*/
		fid_nbr,	/* which FID to get (first=0) */
                npadj,		/* number of adjusted complex td points	*/
                lsfval,		/* complex points to be left-shifted	*/
                dpflag,		/* double-precision flag for FID data	*/
		fidmap;		/* flag for using FID map		*/
float           *data;		/* pointer to converted FID data	*/
dfilehead	*filehead;	/* pointer to FID file header		*/
float		*lpval;		/* lpval from FID block header (DSP)	*/
{
   int skip_blks;		/* Nbr of blocks before FID we want */
   int skip_fids;		/* Nbr of FIDs to skip at front of block */
   int                  shift,
			diff_block = 0;
   float                rmult;
   dblockhead	        fidblockhead;
   off_t                fidOffset;
   extern void		cnvrts1632fl();
#ifdef LINUX
   extern void		swap1632fl();
#endif

   *lpval = 0.0;
   if (fidmap)
   { /* the `lastfid` scheme does not generally work with FID mapping!  SF */
      if ( (fid_nbr = mapFIDblock(fid_nbr)) == ERROR )
      {
         Werrprintf("readFIDdata:  error ocurred in FID translation");
         return(ERROR);
      }

      skip_blks = fid_nbr / filehead->ntraces;
      skip_fids = fid_nbr % filehead->ntraces;
      fidOffset = sizeof(struct datafilehead);
      fidOffset += (off_t) skip_blks * (off_t) filehead->bbytes;
      if ( lseek(fd, fidOffset, SEEK_SET) < 0L )
      {
         Werrprintf("readFIDdata:  error ocurred in FID seek operation");
         return(ERROR);
      }
   }
   else
   {
      if (firstFID && pf3acq)
      {
         firstFID = FALSE;
         diff_block = fid_nbr;
      }
      else
      {
         diff_block = fid_nbr - (*lastfid) - 1;
      }

      skip_blks = fid_nbr / filehead->ntraces;
      skip_fids = fid_nbr % filehead->ntraces;
      fidOffset = sizeof(struct datafilehead);
      fidOffset += (off_t) skip_blks * (off_t) filehead->bbytes;
      if ( lseek(fd, fidOffset, SEEK_SET) < 0L )
      {
	  return(LASTFID);
      }
   }

   if ( read(fd, (char *) (&fidblockhead), sizeof(dblockhead)) !=
                sizeof(dblockhead) )
   {
      return(LASTFID);
   }

#ifdef LINUX
   DBH_CONVERT( fidblockhead );
#endif

   *lpval = fidblockhead.lpval;

   if ( !(fidblockhead.status & S_DATA) )
   {
      return(LASTFID);
   }
   else
   {
      fidOffset = (off_t)skip_fids * (off_t) filehead->tbytes;
      if (lseek(fd, fidOffset, SEEK_CUR) < 0L
	  || read(fd, wspace, nfidbytes) != nfidbytes )
      {
         Werrprintf("\nreadFIDdata():  read error on FID %d", *lastfid + 1);
         return(ERROR);
      }
 
      rmult = 1.0 / (float)fidblockhead.ctcount;
      for (shift = 0; shift < fidblockhead.scale; shift++)
         rmult *= 2.0;
 
#ifdef LINUX
      swap1632fl(wspace, npadj, lsfval, fidblockhead.status);
#endif
      cnvrts1632fl(rmult, wspace, data, npadj, lsfval, fidblockhead.status);
      *lastfid = fid_nbr;
      return(COMPLETE);
   }
}
 
 
/*---------------------------------------
|                                       |
|         closeDATAfiles()/2            |
|                                       |
+--------------------------------------*/
void closeDATAfiles(datafinfo, pinfo)
filedesc	*datafinfo;
comInfo		*pinfo;
{
   char	maindatafilepath[MAXPATHL],
	datafilepath[MAXPATHL],
	fext[10];
   int	i,
	ndatafiles,
	*fdlist,
	*lklist;

   
   if (datafinfo == NULL)
      return;

   if ( (fdlist = datafinfo->dfdlist) == NULL )
      return;

   if ( (lklist = datafinfo->dlklist) == NULL )
      return;

   (void) strcpy(maindatafilepath, pinfo->datadirpath.sval);
   (void) strcat(maindatafilepath, "/data");
   ndatafiles = ( pinfo->multifile.ival ? datafinfo->ndatafd : 1 );

   for (i = 0; i < ndatafiles; i++)
   {
      if ( (*fdlist) != FILE_CLOSED )
      {
         (void) close( *fdlist );
         ( (i > 8) ? (void)sprintf(fext, "%2d", i+1) :
			(void)sprintf(fext, "%1d", i+1) );
         (void) strcpy(datafilepath, maindatafilepath);
         (void) strcat(datafilepath, fext);

         if (*lklist)
            removelock(datafilepath);
      }

      fdlist += 1;
      lklist += 1;
   }

   free( (char *) (datafinfo->dfdlist) );
/* free( (char *) (datafinfo->dlklist) );  Not explictly malloc'ed; do not free  */
   free( (char *) datafinfo );
}


/*---------------------------------------
|                                       |
|         writeDATAheader()/2           |
|                                       |
+--------------------------------------*/
int writeDATAheader(dfd, dataheader)
int		dfd;
datafileheader	*dataheader;
{
   int	nbytes;

   nbytes = sizeof(dfilehead) + sizeof(f3blockpar) + sizeof(phasepar);
   if ( lseek(dfd, (off_t) 0L, SEEK_SET) < 0L )
   {
      Werrprintf("\nwriteDATAheader():  seek error");
      return(ERROR);
   }

   if ( write(dfd, (char *) (&(dataheader->Vfilehead)), nbytes)
		 != nbytes )
   {
      Werrprintf("\nwriteDATAheader():  write error 1");
      return(ERROR);
   }

   nbytes = sizeof(datafileheader) - nbytes - sizeof(float *);
   if ( write(dfd, (char *) (&(dataheader->maxval)),
	  nbytes) != nbytes )
   {
      Werrprintf("\nwriteDATAheader():  write error 2");
      return(ERROR);
   }

   if ( write(dfd, (char *) (dataheader->coefvals),
	  dataheader->ncoefbytes) != dataheader->ncoefbytes )
   {
      Werrprintf("\nwriteDATAheader():  write error 3");
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|          readDATAheader()/2           |
|                                       |
+--------------------------------------*/
datafileheader *readDATAheader(fd, coef)
int	fd;
coef3D	*coef;
{
   int			i,
			nbytes;
   float		*srccoef,
			*destcoef;
   datafileheader	*filehead;

   if (!fd){
       return NULL;
   }

   if ( (filehead = (datafileheader *) malloc( (unsigned)
		(sizeof(datafileheader)) ))
		== NULL )
   {
      Werrprintf("\nreadDATAheader():  insufficient memory");
      return(NULL);
   }

   nbytes = sizeof(dfilehead) + sizeof(f3blockpar) + sizeof(phasepar);
   if ( read(fd, (char *)filehead, nbytes) != nbytes )
   {
      Werrprintf("\nreadDATAheader():  read error 1");
      return(NULL);
   }

   nbytes = sizeof(datafileheader) - nbytes - sizeof(float *);
   if ( read(fd, (char *) (&(filehead->maxval)), nbytes)
		!= nbytes )
   {
      Werrprintf("\nreadDATAheader():  read error 2");
      return(NULL);
   }

   if ( coef && filehead->ncoefbytes != coef->ncoefs * sizeof(float) )
   {
      Werrprintf("\nreadDATAheader():  coefficient mismatch");
      return(NULL);
   }

   if ( (filehead->coefvals = (float *) malloc( (unsigned)
		(filehead->ncoefbytes) ))
		== NULL )
   {
      Werrprintf("\nreadDATAheader():  insufficient memory for coefficients");
      return(NULL);
   }

   if (coef){
       srccoef = coef->f3t2.coefval;
       destcoef = filehead->coefvals;
       for (i = 0; i < (coef->ncoefs - 4*COMPLEX); i++)
	   *destcoef++ = *srccoef++;

       srccoef = coef->f2t1.coefval;
       for (i = 0; i < (4*COMPLEX); i++)
	   *destcoef++ = *srccoef++;
   }else{
       /* No coef values passed, get them from the header */
       if (read(fd, (char *)&filehead->coefvals, filehead->ncoefbytes)
	   != filehead->ncoefbytes)
       {
	   Werrprintf("\nreadDATAheader():  read error 3");
	   return(NULL);
       }
   }

   return(filehead);
}


/*---------------------------------------
|					|
|	   initDATAheader()/4		|
|					|
+--------------------------------------*/
datafileheader *initDATAheader(f3block, coef, p3Dinfo, pinfo)
f3blockpar	*f3block;
coef3D		*coef;
proc3DInfo	*p3Dinfo;
comInfo		*pinfo;
{
   int			i;
   float		*srccoef,
			*destcoef;
   datafileheader	*filehead;


   if ( (filehead = (datafileheader *) malloc( (unsigned)
		(sizeof(datafileheader)) ))
		== NULL )
   {
      Werrprintf("\ninitDATAheader():  insufficient memory");
      return(NULL);
   }

   filehead->mode = (p3Dinfo->f3dim.scdata.dsply ?
			p3Dinfo->f3dim.scdata.dsply : PHMODE) & NP_DSPLY;
   filehead->mode |= ( (p3Dinfo->f2dim.scdata.dsply ?
			p3Dinfo->f2dim.scdata.dsply : PHMODE) << 11 )
			   & NI2_DSPLY;
   filehead->mode |= ( (p3Dinfo->f1dim.scdata.dsply ?
			p3Dinfo->f1dim.scdata.dsply : PHMODE) << 8 )
			   & NI_DSPLY;

   filehead->lastfid = -1;
   filehead->maxval = 0.0;
   filehead->minval = 1000000.0;
   filehead->version3d = FT3D_VERSION;
   filehead->ncoefbytes = coef->ncoefs * sizeof(float);
   filehead->ndatafiles = pinfo->multifile.ival;
   filehead->nheadbytes = sizeof(datafileheader) + filehead->ncoefbytes
				- sizeof(float *);

   filehead->phaseinfo.rp  = p3Dinfo->f3dim.scdata.rp;
   filehead->phaseinfo.lp  = p3Dinfo->f3dim.scdata.lp;
   filehead->phaseinfo.rp2 = p3Dinfo->f2dim.scdata.rp;
   filehead->phaseinfo.lp2 = p3Dinfo->f2dim.scdata.lp;
   filehead->phaseinfo.rp1 = p3Dinfo->f1dim.scdata.rp;
   filehead->phaseinfo.lp1 = p3Dinfo->f1dim.scdata.lp;

   filehead->f3blockinfo.hcptspertrace = f3block->hcptspertrace;
   filehead->f3blockinfo.bytesperfid   = f3block->bytesperfid;
   filehead->f3blockinfo.dpflag        = f3block->dpflag;

   filehead->Vfilehead.nblocks   = p3Dinfo->f2dim.scdata.fn/COMPLEX;
   filehead->Vfilehead.ntraces   = p3Dinfo->f1dim.scdata.fn/COMPLEX;
   filehead->Vfilehead.np        = HYPERCOMPLEX * f3block->hcptspertrace *
					filehead->ndatafiles;
   filehead->Vfilehead.ebytes    = sizeof(float);
   filehead->Vfilehead.tbytes	 = filehead->Vfilehead.np *
					filehead->Vfilehead.ebytes;
   filehead->Vfilehead.bbytes    = filehead->Vfilehead.ntraces *
					filehead->Vfilehead.tbytes;

   filehead->Vfilehead.vers_id   = DATA3D_FILE + VNMR_VERSION;
   filehead->Vfilehead.status    = 0;
   filehead->Vfilehead.nbheaders = 0;

   if ( (filehead->coefvals = (float *) malloc( (unsigned)
		(filehead->ncoefbytes) ))
		== NULL )
   {
      Werrprintf("\ninitDATAheader():  insufficient memory for coefficients");
      return(NULL);
   }

   srccoef = coef->f3t2.coefval;
   destcoef = filehead->coefvals;
   for (i = 0; i < (coef->ncoefs - 4*COMPLEX); i++)
      *destcoef++ = *srccoef++;

   srccoef = coef->f2t1.coefval;
   for (i = 0; i < (4*COMPLEX); i++)
      *destcoef++ = *srccoef++;

   return(filehead);
}


/*---------------------------------------
|                                       |
|           readFIDheader()/1           |
|                                       |
+--------------------------------------*/
dfilehead *readFIDheader(fd)
int     fd;		/* file descriptor for FID file	*/
{
   dfilehead        *fidfilehead;
 
 
   if ( (fidfilehead = (dfilehead *) malloc( (unsigned)
           sizeof(dfilehead) )) == NULL )
   {
      Werrprintf("\nreadFIDheader():  insufficient memory for FID file header");      return(NULL);
   }
 
   if ( read(fd, (char *)fidfilehead, sizeof(dfilehead)) !=
		sizeof(dfilehead) )
   {
      Werrprintf("\nreadFIDheader():  read error on FID file header");
      return(NULL);
   }

#ifdef LINUX
   DFH_CONVERT( fidfilehead );
#endif

   return(fidfilehead);
}
 
 
/*---------------------------------------
|                                       |
|          openDATAfiles()/3            |
|                                       |
+--------------------------------------*/
filedesc *openDATAfiles(pinfo, whichtoopen, option)
int	option,		/* file opening option			*/
	whichtoopen;	/* which data files to open		*/
comInfo *pinfo;		/* pointer to command line structure	*/
{
   char		maindatafilepath[MAXPATHL],
		datafilepath[MAXPATHL],
		fext[10];
   int		i,
		ndatafiles,
		mode,
		prot,
		*fdlist,
		*lklist;
   filedesc	*datafinfo;


/**********************************
*  Allocate memory for DATA file  *
*  information structure.         *
**********************************/

   if ( (datafinfo = (filedesc *) malloc( (unsigned)sizeof(filedesc) ))
 	  == NULL)
   {
      Werrprintf("\nopenDATAfiles():  insufficient memory for `fd` structure");
      return(NULL);
   }

/**********************************
*  Allocate memory for the array  *
*  of file descriptors.           *
**********************************/

   ndatafiles = ( pinfo->multifile.ival ? pinfo->multifile.ival : 1 );

   if ( (datafinfo->dfdlist = (int *) malloc( (unsigned) (2 * ndatafiles *
	  sizeof(int)) )) == NULL )
   {
      Werrprintf("\nopenDATAfiles():  insufficient memory for `fd` structure");
      free((char *)datafinfo);
      return(NULL);
   }

   datafinfo->dlklist = datafinfo->dfdlist + ndatafiles;

/*******************************
*  Initialize these elements.  *
*******************************/

   datafinfo->ndatafd = ndatafiles;
   fdlist = datafinfo->dfdlist;
   lklist = datafinfo->dlklist;

   for (i = 0; i < ndatafiles; i++)
   {
      *fdlist++ = FILE_CLOSED;
      *lklist++ = FALSE;
   }

   (void) strcpy(maindatafilepath, pinfo->datadirpath.sval);
   (void) strcat(maindatafilepath, "/data");
   fdlist = datafinfo->dfdlist;
   lklist = datafinfo->dlklist;
   datafinfo->dataexists = FALSE;
   datafinfo->result = 0;
   mode = (O_RDWR|O_CREAT|O_TRUNC);
   prot = 0666;

   
/*********************************************
*  Start scanning through all possible DATA  *
*  files to find the first one which can     *
*  be opened for further processing.         *
*********************************************/

   for (i = 0; i < ndatafiles; i++)
   {
      ( (i > 8) ? (void)sprintf(fext, "%2d", i+1) :
		(void)sprintf(fext, "%1d", i+1) );
      (void) strcpy(datafilepath, maindatafilepath);
      (void) strcat(datafilepath, fext);

      if (option & CHECK)
      {
         if (i == 0)
         {
            if ( access(datafilepath, (R_OK|W_OK)) == 0 )
            {
               datafinfo->dataexists = TRUE;
               mode = O_RDWR;
            }
         }
         else
         {
            if ( access(datafilepath, (R_OK|W_OK)) &&
		    datafinfo->dataexists )
            {
               datafinfo->result = NOMORE_DFILES;
               return(datafinfo);
            }
         }   
      }

      if (option & READONLY)
      {
         mode = O_RDONLY;
         prot = 0440;
      }

      if ( (i == whichtoopen) || (whichtoopen == ALL) )
      {
         if ( (~option) & READONLY )
         {
            if ( (datafinfo->result = createlock(datafilepath,
		   DATA_LFILE)) )
            {
               return(datafinfo);
            }

            *lklist = TRUE;
         }

         if ( (*fdlist = open(datafilepath, mode, prot)) < 0 )
         {
            Werrprintf("\nopenDATAfiles():  cannot open DATA file %s",
	        datafilepath);
            datafinfo->result = ERROR;
            if (*lklist)
               removelock(datafilepath);

            return(datafinfo);
         }
      }

      fdlist += 1;
      lklist += 1;
   }


   return(datafinfo);
}


/*---------------------------------------
|                                       |
|           setf3blockpar()/4           |
|                                       |
+--------------------------------------*/
f3blockpar *setf3blockpar(info3D, pinfo, maxwords, fidheader)
int             *maxwords;	/* pointer to size of data buffer	*/
dfilehead	*fidheader;	/* pointer to FID file header		*/
comInfo		*pinfo;		/* pointer to command structure		*/
proc3DInfo      *info3D;	/* pointer to 3D information structure	*/
{
   int          i,
		hcptspertrace,
                nblocks,
                bytesperfid,
		nhcF3pts;
   f3blockpar   *f3block;


   if ( (f3block = (f3blockpar *) malloc( (unsigned) sizeof(f3blockpar) ))
             == NULL )
   {         
      Werrprintf("\nsetf3blockpar():  insufficient memory for block parameters");
      return(NULL);
   }

   nhcF3pts = (info3D->f3dim.scdata.reginfo.endptzero -
		  info3D->f3dim.scdata.reginfo.stptzero) / COMPLEX;
 
   hcptspertrace = (*maxwords) / (2 * maxfn12);
   if (hcptspertrace < 1)
   {
      Werrprintf("\nsetf3blockpar():  `memsize` is too small");
      return(NULL);
   }
   else if (hcptspertrace > nhcF3pts)
   {
      
      hcptspertrace = nhcF3pts;
      nblocks = ( pinfo->multifile.ival ? pinfo->multifile.ival : 1 );
      hcptspertrace /= nblocks;
      *maxwords = 2 * hcptspertrace * maxfn12;
   }
   else
   {
      nblocks = nhcF3pts / hcptspertrace;
      if (nblocks < pinfo->multifile.ival)
         nblocks = pinfo->multifile.ival;

      hcptspertrace = nhcF3pts / nblocks;
      if (hcptspertrace < 1)
      {
         Werrprintf("\nsetf3blockpar():  too many data files for data size");
         return(NULL);
      }

      *maxwords = 2 * hcptspertrace * maxfn12;
   }

   pinfo->multifile.ival = nblocks;
   bytesperfid = fidheader->tbytes;

   f3block->dpflag = (fidheader->status & S_32);
   f3block->hcptspertrace = hcptspertrace;
   f3block->bytesperfid = bytesperfid;

   return(f3block);
}


/*---------------------------------------
|                                       |
|            calcmaxwords()/0           |
|                                       |
+--------------------------------------*/
int calcmaxwords()
{   
   char 	*memsize;
   int  	bufscale,
       		tmpivar;
   extern char	*getenv();
 
 
   if ( (memsize = getenv("memsize")) == NULL )
   {
      bufscale = 2;
   }
   else
   {
      bufscale = atoi(memsize) / 4;
      if (bufscale < 1)
      {
         bufscale = 1;
      }
      else
      {
         tmpivar = 1;
         while (tmpivar < bufscale)
            tmpivar *= 2;
         if (tmpivar > bufscale)
            tmpivar /= 2;
         bufscale = tmpivar;
      }
   }

   return(WORDS_PER_BUFUNIT * bufscale);
}


/*--------------------------------------------
|                                            |
|             f3block_wr()/4                 |
| data	    pointer to F3 frequency data     |
| stbyte    starting byte in DATA file	     |
| iobytes   number of bytes to write	     |
| datafinfo pointer to DATA file ID structure|
|                                            |
+--------------------------------------------*/
int f3block_wr(filedesc *datafinfo, char *data, off_t stbyte, int iobytes)
{
   int  i,
	seekflag;
 

   seekflag = ( (stbyte - compstbyte) > 0 );
   compstbyte = stbyte + (off_t) iobytes;

   for (i = 0; i < datafinfo->ndatafd; i++)
   {
      if (seekflag)
      {
         if ( lseek( *(datafinfo->dfdlist + i), stbyte, SEEK_SET) < 0L )
         {
            Werrprintf("\nf3block_wr():  seek error on 3D data file %d", i+1);
            return(ERROR);
         }
      }

      if ( write( *(datafinfo->dfdlist + i), data, iobytes) != iobytes )
      {
         Werrprintf("\nf3block_wr():  write error on 3D data file %d", i+1);
         return(ERROR);
      }

      data += iobytes;
   }

   return(COMPLETE);
}
 
 
/*---------------------------------------
|                                       |
|            f21block_io()/10           |
|                                       |
+--------------------------------------*/
int f21block_io(fd, data, wspace, f12block, nf3pts, ioop, dimen,
                        datatype, p3Dinfo, nfheadbytes)
int             fd,             /* file descriptor for 3D data file     */
                f12block,       /* (t1,f1) or (t2,f2) block number      */
                nf3pts,         /* number of hypercomplex F3 points     */
                ioop,           /* I/O operation:  read or write        */
                dimen,          /* F1 or F2 dimension                   */
		datatype,	/* type of data to read or write	*/
		nfheadbytes;	/* size of the DATA file header (bytes)	*/
float           *data,          /* pointer to t2 interferogram data     */
                *wspace;        /* pointer to work space                */
proc3DInfo      *p3Dinfo;       /* pointer to 3D processing information */
{
   int                  nf21pts,
                        iobytes;
   off_t                stbytes;
   off_t                skbytes;
   register int         i,
                        j,
                        k,
                        xposeskp;
   register float       *tmpwspace,
                        *tmpdata;
 
 
   if (ioop == READ)
   {
      if (dimen == F1_DIMEN)
      {
         nf21pts = ( (p3Dinfo->f1dim.scdata.np <
			   p3Dinfo->f1dim.scdata.fn/COMPLEX)
				? p3Dinfo->f1dim.scdata.np
				: p3Dinfo->f1dim.scdata.fn/COMPLEX );
      }
      else
      {
         nf21pts = ( (p3Dinfo->f2dim.scdata.np <
			   p3Dinfo->f2dim.scdata.fn/COMPLEX)
				? p3Dinfo->f2dim.scdata.np
				: (p3Dinfo->f2dim.scdata.fn/COMPLEX) );
      }
   }
   else
   {
      nf21pts = ( (dimen == F1_DIMEN)
			? (p3Dinfo->f1dim.scdata.fn/COMPLEX)
                        : (p3Dinfo->f2dim.scdata.fn/COMPLEX) );
   }
 

   tmpwspace = wspace;
   tmpdata = data;
   iobytes = nf3pts * sizeof(float) * datatype;


   if (dimen == F2_DIMEN)
      skbytes = (off_t) ( (p3Dinfo->f1dim.scdata.fn/COMPLEX) - 1 ) * (off_t) iobytes;
 
   if (dimen == F1_DIMEN)
   {
      xposeskp = HYPERCOMPLEX * (p3Dinfo->f1dim.scdata.fn/COMPLEX) - datatype;
      stbytes = (off_t) (p3Dinfo->f1dim.scdata.fn/COMPLEX) * (off_t) f12block * (off_t) iobytes;
   }
   else
   {
      xposeskp = datatype * ( (p3Dinfo->f2dim.scdata.fn/COMPLEX) - 1 );
      stbytes = (off_t) f12block * (off_t) iobytes;
   }

   stbytes += (off_t) nfheadbytes;
   if ( lseek(fd, stbytes, SEEK_SET) < 0L )
   {
      Werrprintf("\nf21block_io():  seek error occurred on 3D data file");
      return(ERROR);
   }
 
 
   for (i = 0; i < nf21pts; i++)
   {
      if (ioop == READ)
      {
         if ( read(fd, (char *)wspace, iobytes) != iobytes )
         {
            Werrprintf("\nf21block_io():  read error occurred on 3D data file");
            return(ERROR);
         }
      }
 
      data += HYPERCOMPLEX;
      if (ioop == READ)
      { /* when reading t2 or t1 data:  always hypercomplex for now */
         for (j = 0; j < nf3pts; j++)
         {
            for (k = 0; k < datatype; k++)
               *tmpdata++ = *tmpwspace++;
 
            tmpdata += xposeskp;
         }
      }
      else
      { /* when writing f2 or f1 data */
         for (j = 0; j < nf3pts; j++)
         {
            for (k = 0; k < datatype; k++)
               *tmpwspace++ = *tmpdata++;

            tmpdata += xposeskp;
         }
      }   
       
      if (ioop == WRITE)
      {
         if ( write(fd, (char *)wspace, iobytes) != iobytes )
         {
            Werrprintf("\nf21block_io():  write error occurred on 3D data file");
            return(ERROR);
         }
      }
 
      if (dimen == F2_DIMEN)
      {
         if ( lseek(fd, skbytes, SEEK_CUR) < 0L )
         {
            Werrprintf("\nf21block_io():  seek error occurred on 3D data file");
            return(ERROR);
         }
      }

      tmpdata = data;
      tmpwspace = wspace;
   }

   return(COMPLETE);
}
