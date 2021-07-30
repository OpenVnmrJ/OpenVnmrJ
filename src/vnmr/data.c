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
|	data.c    -    data file handler	|
|						|
+----------------------------------------------*/

#define _FILE_OFFSET_BITS 64

#include "vnmrsys.h"
#include "data.h"
#include "group.h"
#include "allocate.h"
#include "wjunk.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#ifdef __INTERIX
#include <arpa/inet.h>
#endif
#ifdef LINUX
#include <netinet/in.h>
#include <inttypes.h>
#endif


#define COMPLETE	0
#define ERROR		1
#define FALSE		0
#define TRUE		1
#define EMPTY		-1
#define NOT_OPEN	-1
#define NOT_IN_USE	0
#define IN_USE		1
#define NOT_UPDATED	0
#define UPDATED		1
#define D_MAXBLOCKS	8	     /* maximum number of blocks in memory  */
#define D_MAXBYTES  	(800*1024)   /* maximum number of bytes to allocate */
#define MAXINT		0x7FFFFFFF  
#define BLOCKSIZE	65536	     /* words "per" bufferscale */


#define checkfileindex(findex)						\
	((findex < D_DATAFILE) || (findex > D_USERFILE))

#define checkfileopen(curfile_index)					\
	(curfile_index >= 0)

#define checkfileclosed(curfile_index)					\
	(curfile_index < 0)

#define checkblockindex(indx, maxfileindx)				\
	(indx >= 2*maxfileindx)

#define checkblockfind(b_index, headerpntr)				\
	((b_index < 0) || (headerpntr == NULL))

#define getdatatype(status)						\
  	( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX :			\
    	( (status & S_COMPLEX) ? COMPLEX : REAL ) )

struct _fileinfo
{
   dfilehead	datahead;		 /* current data file head  */
   dblockhead	*hpointers[D_MAXBLOCKS]; /* memory block heads	    */
   int		curfile;		 /* current file descriptor */
   int		vreadonly;		 /* read only flag 	    */
   int 	 	blockindex[D_MAXBLOCKS]; /* memory block indices    */
   int		bufsize[D_MAXBLOCKS];	 /* allocated buffer size   */
   int		umark[D_MAXBLOCKS];	 /* mark for updated	    */
   int	 	inuse[D_MAXBLOCKS];	 /* mark for in use         */
};

typedef struct _fileinfo	fileinfo;

struct _curpathInfo
{
   char	curpar[MAXPATH];
   char	procpar[MAXPATH];
   char	datafile[MAXPATH];
   char	phasefile[MAXPATH];
};

typedef struct _curpathInfo	curpathInfo;

#ifdef LINUX
struct swapbyte
{
   short s1;
   short s2;
   short s3;
   short s4;
   int  l1;
   int  l2;
   int  l3;
   int  l4;
   int  l5;
};

typedef union
{
   dblockhead *in1;
   struct swapbyte *out;
} headerUnion;

#endif

extern void initxposebuf();
extern int transpose(void *matrix, int ncols, int nrows, int datatype);
extern int p11_saveFDAfiles_processed(char* func, char* orig, char* dest);

static char			*lastfunc;
static short			fid_version_id = VERSION;
static int			freealloc;
static fileinfo			filedata[D_MAXFILES];
static curpathInfo		curfilepaths = {"curpar", "procpar",
						"data", "phasefile"};

char curexpdir[MAXPATH];       /* current experiment path */
int  bufferscale;               /* scaling factor for internal Vnmr buffers */
int  Rflag;

/* prototypes */
static void binit(int fileindex);
static int findblock(int fileindex, int index);
static int last_vendor_id = 0;

/********************
*  BEGIN FUNCTIONS  *
********************/

int D_getLastVendorId()
{
   return(last_vendor_id);
}

/*---------------------------------------
|					|
|	     show_blocks()/0		|
|					|
|   This function shows the indices	|
|   of the DATA and PHASFILE blocks	|
|   which are currently in memory.	|
|					|
+--------------------------------------*/
static int show_blocks()
{
   int i;


   for (i = 0; i < D_MAXBLOCKS; i++)
      Wscrprintf("%4d", filedata[D_DATAFILE].blockindex[i]);

   Wscrprintf(" - ");
   for (i = 0; i < D_MAXBLOCKS; i++)
      Wscrprintf("%4d", filedata[D_PHASFILE].blockindex[i]);

   Wscrprintf("\n");
   return(COMPLETE);
}


/*-------------------------------------------
|					    |
|		movmem()/5		    |
|					    |
|   This function moves data from one	    |
|   location in memory to another.	    |
|					    |
| *c1,	 pointer to source of data          |
| *c2;	 pointer to destination of data     |
| n,		 number of BYTES to move    |
| sc1,	 skip factor for source pointer     |
| bytesperelem; number of bytes per element |
+------------------------------------------*/
void movmem(void *c1, void *c2, size_t n , int sc1, int bytesperelem)
{
   register size_t	i;
   register int		skip1;


   skip1 = sc1;

   if (skip1 != 1)
   {
      switch(bytesperelem)
      {
         case sizeof(char):		/* CHAR variable type */
	 {
	    register char	*ip1,
				*ip2;

	    i = n;
	    ip1 = (char *)c1;
	    ip2 = (char *)c2;

	    while (i--)
	    {
	       *ip2++ = *ip1;
	       ip1 += skip1;
	    }

	    break;
	 }
         case sizeof(short):		/* SHORT variable type */
	 {
	    register short	*ip1,
				*ip2;

	    i = n/sizeof(short);
	    ip1 = (short *)c1;
	    ip2 = (short *)c2;

	    while (i--)
	    {
	       *ip2++ = *ip1;
	       ip1 += skip1;
	    }

	    break;
	 }
         case sizeof(int):		/* INTEGER or FLOAT variable type */
	 {
	    register int	*ip1,
				*ip2;

	    i = n/sizeof(int);
	    ip1 = (int *)c1;
	    ip2 = (int *)c2;

	    while (i--)
	    {
	       *ip2++ = *ip1;
	       ip1 += skip1;
	    }

	    break;
	 }
         case sizeof(double):		/* DOUBLE variable type */
	 {
	    register double	*ip1,
				*ip2;

	    i = n/sizeof(double);
	    ip1 = (double *)c1;
	    ip2 = (double *)c2;

	    while (i--)
	    {
	       *ip2++ = *ip1;
	       ip1 += skip1;
	    }

	    break;
	 }
         default:			/* CHAR variable type */
	 {
	    register char	*ip1,
				*ip2;

	    i = n;
	    ip1 = (char *)c1;
	    ip2 = (char *)c2;

	    while (i--)
	    {
	       *ip2++ = *ip1;
	       ip1 += skip1;
	    }
	 }
      }
   }
   else
   {
      register char	*ip1,
			*ip2;

      i = n;
      ip1 = (char *)c1;
      ip2 = (char *)c2;

      while (i--)
         *ip2++ = *ip1++;
   }
}


/*---------------------------------------
|					|
|		D_init()/0		|
|					|
|   This function initializes the data	|
|   file handler.  It should be called	|
|   on bootup.				|
|					|
+--------------------------------------*/
int D_init()
{
   int fileindex;


   if (Rflag > 2) 
   {
      Wscrprintf("D_init starts\n");
      show_blocks();
   }

   lastfunc = "init";
   for (fileindex = D_DATAFILE; fileindex <= D_USERFILE; fileindex++)
   {
      filedata[fileindex].curfile = NOT_OPEN;	/* mark as not open */
      binit(fileindex);
   }

   freealloc = D_MAXBYTES*bufferscale;
   initxposebuf();

   if (Rflag > 2) 
      Wscrprintf("D_init done\n");

   return(COMPLETE);
}


/*---------------------------------------
|					|
|		binit()/1		|
|					|
|  This function initializes the block	|
|  storage buffer.			|
|					|
+--------------------------------------*/
static void binit(int fileindex)
{
   int i;


   for (i = 0; i < D_MAXBLOCKS; i++)
   {
      filedata[fileindex].hpointers[i] = NULL;
      filedata[fileindex].blockindex[i] = EMPTY;
      filedata[fileindex].umark[i] = NOT_UPDATED;
      filedata[fileindex].inuse[i] = NOT_IN_USE;
      filedata[fileindex].bufsize[i] = 0;
   }
}


int D_fidversion()
{
   return( (int) fid_version_id);
}
/*---------------------------------------
|					|
|	     fidversion()/3		|
|					|
+--------------------------------------*/
int fidversion(void *headptr, int headertype, int version)
{
   short	old_S_complex,
		old_Vendor_id,
		version_id,
		vendor_id;
   dblockhead	*fidblockhead;
   dfilehead	*fidfilehead;


   if (headertype == FILEHEAD)
   {
      fidfilehead = (dfilehead *)headptr;
      version_id = ( (version == -1) ? (fidfilehead->vers_id & P_VERS) :
			version );
      if (version == -1)
         fid_version_id = version_id;

      switch (version_id)
      {
         case 0:   old_S_complex = 0x40;
		   old_Vendor_id = (0x800 | 0x1000 | 0x2000 | 0x4000);
                   vendor_id = fidfilehead->status & old_Vendor_id;
		   fidfilehead->nbheaders = 1;
		   break;
         default:  vendor_id = fidfilehead->vers_id & P_VENDOR_ID;
                   old_S_complex = S_COMPLEX;
		   break;
      }

      fidfilehead->vers_id = vendor_id + FID_FILE + VERSION;
      last_vendor_id = vendor_id;
      if (fidfilehead->status & old_S_complex)	/* old S_COMPLEX */
      {
         fidfilehead->status &= ~(old_S_complex);
         fidfilehead->status |= S_COMPLEX;
      }

      fidfilehead->status &= ~(S_HYPERCOMPLEX|S_SECND|S_TRANSF|S_NP|
				  S_NF|S_NI|S_NI2);
   }
   else if (headertype == BLOCKHEAD)
   {
      if ((version < 0) || (version > VERSION))
      {
         Werrprintf("Invalid version number for FID block header\n");
         return(ERROR);
      }

      fidblockhead = (dblockhead *)headptr;
      switch (version)
      {
         case 0:   old_S_complex = 0x40;
                   break;
         default:  old_S_complex = S_COMPLEX;
                   break;
      }

      if (fidblockhead->status & old_S_complex)	/* old S_COMPLEX */
      {
         fidblockhead->status &= ~(old_S_complex);
         fidblockhead->status |= S_COMPLEX;
      }

      fidblockhead->status &= ~(S_HYPERCOMPLEX|MORE_BLOCKS|NP_CMPLX|
				   NF_CMPLX|NI_CMPLX|NI2_CMPLX);
   }
   else
   {
      Werrprintf("Invalid type of header\n");
      return(ERROR);
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		   D_open()/3			|
|						|
|   This function opens a file for processing	|
|   using the data file handler.		|
|						|
+----------------------------------------------*/
int D_open(int fileindex, char *filepath, dfilehead *fhead)
{
   lastfunc = "open";
   if (Rflag > 2) 
   {
      Wscrprintf("D_open\n");
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (checkfileopen(filedata[fileindex].curfile))
      return(D_IS_OPEN);

   binit(fileindex);
   filedata[fileindex].curfile = open(filepath, O_RDWR, 0666); /* r/w open */
   filedata[fileindex].vreadonly = FALSE;

   if (checkfileclosed(filedata[fileindex].curfile))
   {
      filedata[fileindex].curfile = open(filepath, O_RDONLY, 0666);
      filedata[fileindex].vreadonly = TRUE;
   }

   if (checkfileclosed(filedata[fileindex].curfile))
      return(D_NOTOPEN);

/****************************
*  Read in datafile header  *
****************************/

   if (read(filedata[fileindex].curfile, &filedata[fileindex].datahead,
		sizeof(dfilehead)) <= 0)
   {
      return(D_READERR);
   }
#ifdef LINUX
   filedata[fileindex].datahead.nblocks = ntohl(filedata[fileindex].datahead.nblocks);
   filedata[fileindex].datahead.ntraces = ntohl(filedata[fileindex].datahead.ntraces);
   filedata[fileindex].datahead.np = ntohl(filedata[fileindex].datahead.np);
   filedata[fileindex].datahead.ebytes = ntohl(filedata[fileindex].datahead.ebytes);
   filedata[fileindex].datahead.tbytes = ntohl(filedata[fileindex].datahead.tbytes);
   filedata[fileindex].datahead.bbytes = ntohl(filedata[fileindex].datahead.bbytes);
   filedata[fileindex].datahead.vers_id = ntohs(filedata[fileindex].datahead.vers_id);
   filedata[fileindex].datahead.status = ntohs(filedata[fileindex].datahead.status);
   filedata[fileindex].datahead.nbheaders = ntohl(filedata[fileindex].datahead.nbheaders);
#endif
/***********************
*  Check for old data  *
***********************/

   switch (fileindex)
   {
      case D_DATAFILE:
      {
         if ( (filedata[fileindex].datahead.nbheaders & NBMASK) < 1)
            return(D_INVNBHDR);
         if (filedata[fileindex].datahead.vers_id == 0)
            filedata[fileindex].datahead.vers_id += (DATA_FILE + VERSION);
         break;
      }

      case D_PHASFILE:
      {
         if ( (filedata[fileindex].datahead.nbheaders & NBMASK) < 1)
            return(D_INVNBHDR);
         if (filedata[fileindex].datahead.vers_id == 0)
            filedata[fileindex].datahead.vers_id += (PHAS_FILE + VERSION);
         break;
      }

      case D_USERFILE:
      {
         if ( fidversion((char *) (&filedata[fileindex].datahead),
		FILEHEAD, -1) )
         {
            return(ERROR);
         }
         break;
      }

      default:	break;
   }

   movmem((char *) (&filedata[fileindex].datahead), (char *)fhead,
			sizeof(dfilehead), 1, 1);

   if (Rflag > 2) 
      Wscrprintf("D_open done\n");

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      blockread()/5		|
|					|
|   This function reads in a data	|
|   block from a disk file.		|
|					|
+--------------------------------------*/
static int blockread(int fileindex, char *buf, int blockno, off_t start, int length)
{
   off_t offset;

   offset = (off_t) blockno * (off_t) filedata[fileindex].datahead.bbytes;
   offset += sizeof(dfilehead) + start;
   if (lseek(filedata[fileindex].curfile, offset, SEEK_SET) < 0)
   {
      return(D_SEEKERR);
   }

   if (read(filedata[fileindex].curfile, buf, length) <= 0)
      return(D_READERR);

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     blockwrite()/5		|
|					|
|   This function writes out a data	|
|   block into a disk file.		|
|					|
+--------------------------------------*/
static int blockwrite(int fileindex, char *buf, int blockno, off_t start, int length)
{
   off_t offset;
   
   offset = (off_t) blockno * (off_t) filedata[fileindex].datahead.bbytes +
            (off_t) sizeof(dfilehead) + start;
   if (lseek(filedata[fileindex].curfile, offset, SEEK_SET) < 0)
   {
      return(D_SEEKERR);
   }

   if (write(filedata[fileindex].curfile, buf, length) <= 0)
      return(D_WRITERR);

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		  store()/4			|
|						|
|   This function stores a block in a disk	|
|   file.					|
|						|
+----------------------------------------------*/
/* 	fileindex	  specifies which file	       */
/* 	blockno		  index for internal buffer         */
/* 	index		  index into external file block    */
static int store(int fileindex, int blockno, int index, int ignoretransf)
{
   char		*buf2;		  /* pointer to actual data	       */
   int		e,		  /* I/O result variable	       */
		bb,		  /* bytes per block to be written out */
		dtype,		  /* 2 = complex    4 = hypercomplex   */
		i,
		blockheadersize,  /* total block header size in bytes  */
		cur_nblocks,	  /* no. of blocks in "fileindex" file */
		cur_ntraces,	  /* no. of traces in "fileindex" file */
		cur_np;		  /* no. of points in "fileindex" file */
#ifdef LINUX
   headerUnion hU;
   register int cnt, num;
   register short *sptr;
   register int  *lptr;
#endif


   if (Rflag > 2) 
   {
      Wscrprintf("storing block no %d\n",index);
      show_blocks();
   }

/***************************************************
*  Check that the block index is legal.  Then set  *
*  the specified internal buffer ("blockno") to    *
*  NOT_IN_USE.  Determine whether the data is      *
*  hypercomplex.  If the data is hypercomplex,     *
*  allow for two block headers per data block.     *
***************************************************/

   if (checkblockindex(index, filedata[fileindex].datahead.nblocks))
      return(D_BLINDEX);

   filedata[fileindex].umark[blockno] = NOT_UPDATED;
   blockheadersize = (filedata[fileindex].datahead.nbheaders & NBMASK) *
			  sizeof(dblockhead);
   dtype = getdatatype(filedata[fileindex].datahead.status);

/*************************************************
*  Setup "nblocks", "ntraces", and "np" for the  *
*  current "fileindex" file.                     *
*************************************************/

   cur_nblocks = filedata[fileindex].datahead.nblocks;
   cur_ntraces = filedata[fileindex].datahead.ntraces;
   cur_np = filedata[fileindex].datahead.np;


/***************************************************
*  Select how we wish the data to be written out.  *
***************************************************/

   if ((index >= cur_nblocks) && (!ignoretransf) &&
		(filedata[fileindex].datahead.status & S_TRANSF))
   {

/*****************************************************
*  Store a transposed block by transposing it back.  *
*****************************************************/

      if (Rflag > 2)
         Wscrprintf("transpose and write, reversed mode\n");

      index -= cur_nblocks;
      bb = filedata[fileindex].datahead.bbytes - blockheadersize;
      bb /= cur_nblocks;
      buf2 = (char *) (filedata[fileindex].hpointers[blockno])
		 + blockheadersize;

/********************************
*  First write out the header.  *
********************************/
#ifdef LINUX
      hU.in1 = filedata[fileindex].hpointers[blockno];
      hU.out->s1 = htons(hU.out->s1);
      hU.out->s2 = htons(hU.out->s2);
      hU.out->s3 = htons(hU.out->s3);
      hU.out->s4 = htons(hU.out->s4);
      hU.out->l1 = htonl(hU.out->l1);
      hU.out->l2 = htonl(hU.out->l2);
      hU.out->l3 = htonl(hU.out->l3);
      hU.out->l4 = htonl(hU.out->l4);
      hU.out->l5 = htonl(hU.out->l5);
#endif

      if ( (e = blockwrite(fileindex,
		   (char *) (filedata[fileindex].hpointers[blockno]),
		   index, (off_t) 0, blockheadersize)) )
      {
         return(e);
      }

      transpose(buf2, cur_ntraces*cur_nblocks,
		cur_np/(dtype*cur_nblocks),
		dtype);

      for (i = 0; i < filedata[fileindex].datahead.nblocks; i++)
      {
          off_t offset;
          offset = (off_t) index * (off_t) bb + (off_t) blockheadersize;
          transpose(buf2, cur_np/(dtype*cur_nblocks), cur_ntraces, dtype);
#ifdef LINUX
          num = bb / filedata[fileindex].datahead.ebytes;
          if (filedata[fileindex].datahead.ebytes == 2)
          {
             sptr = (short *) buf2;
             for (cnt = 0; cnt < num; cnt++)
             {
                *sptr = htons(*sptr);
                 sptr++;
             }
          }
          else
          {
             lptr = (int *) buf2;
             for (cnt = 0; cnt < num; cnt++)
             {
                *lptr = htonl(*lptr);
                 lptr++;
             }
          }
#endif
	  if ( (e = blockwrite(fileindex, buf2, i, offset, bb)) )
          {
             return(e);
          }

	  buf2 += bb;
      }

      return(COMPLETE);
   }
   else if ((filedata[fileindex].datahead.status & S_TRANSF) &&
		(!ignoretransf))
   {

/***************************************************
*  Store a block which has NOT been transposed in  *
*  the transposed format.                          *
***************************************************/

      transpose((char *) (filedata[fileindex].hpointers[blockno]) +
		blockheadersize, cur_np/dtype, cur_ntraces, dtype);
#ifdef LINUX
      hU.in1 = filedata[fileindex].hpointers[blockno];
      hU.out->s1 = htons(hU.out->s1);
      hU.out->s2 = htons(hU.out->s2);
      hU.out->s3 = htons(hU.out->s3);
      hU.out->s4 = htons(hU.out->s4);
      hU.out->l1 = htonl(hU.out->l1);
      hU.out->l2 = htonl(hU.out->l2);
      hU.out->l3 = htonl(hU.out->l3);
      hU.out->l4 = htonl(hU.out->l4);
      hU.out->l5 = htonl(hU.out->l5);
      bb = filedata[fileindex].datahead.bbytes - blockheadersize;
      num = bb / filedata[fileindex].datahead.ebytes;
      buf2 = (char *) (filedata[fileindex].hpointers[blockno])
		 + blockheadersize;
      if (filedata[fileindex].datahead.ebytes == 2)
      {
         sptr = (short *) buf2;
         for (cnt = 0; cnt < num; cnt++)
         {
            *sptr = htons(*sptr);
             sptr++;
         }
      }
      else
      {
         lptr = (int *) buf2;
         for (cnt = 0; cnt < num; cnt++)
         {
            *lptr = htonl(*lptr);
             lptr++;
         }
      }
#endif

      if ( (e = blockwrite(fileindex,
		   (char *) (filedata[fileindex].hpointers[blockno]),
	  	   index, (off_t) 0, filedata[fileindex].datahead.bbytes)) )
      {
         return(e);
      }
   }
   else
   {

/**************************
*  Store a normal block.  *
**************************/
#ifdef LINUX
      hU.in1 = filedata[fileindex].hpointers[blockno];
      hU.out->s1 = htons(hU.out->s1);
      hU.out->s2 = htons(hU.out->s2);
      hU.out->s3 = htons(hU.out->s3);
      hU.out->s4 = htons(hU.out->s4);
      hU.out->l1 = htonl(hU.out->l1);
      hU.out->l2 = htonl(hU.out->l2);
      hU.out->l3 = htonl(hU.out->l3);
      hU.out->l4 = htonl(hU.out->l4);
      hU.out->l5 = htonl(hU.out->l5);
      bb = filedata[fileindex].datahead.bbytes - blockheadersize;
      /* continuation of the acqflag trick from ftinit.c */
      if (filedata[fileindex].datahead.ebytes == 0)
         num = bb / 4;
      else
         num = bb / filedata[fileindex].datahead.ebytes;
      buf2 = (char *) (filedata[fileindex].hpointers[blockno])
		 + blockheadersize;
      if (filedata[fileindex].datahead.ebytes == 2)
      {
         sptr = (short *) buf2;
         for (cnt = 0; cnt < num; cnt++)
         {
            *sptr = htons(*sptr);
             sptr++;
         }
      }
      else
      {
         lptr = (int *) buf2;
         for (cnt = 0; cnt < num; cnt++)
         {
            *lptr = htonl(*lptr);
             lptr++;
         }
      }
#endif
      if ( (e = blockwrite(fileindex,
		   (char *) (filedata[fileindex].hpointers[blockno]),
	   	   index, (off_t) 0, filedata[fileindex].datahead.bbytes)) )
      {
         return(e);
      }
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		   D_flush()/1			|
|						|
|   This function removes all released blocks	|
|   from the current internal buffers.  The	|
|   updated and released blocks are stored on	|
|   disk.					|
|						|
+----------------------------------------------*/
int D_flush(int fileindex)
{
   int	work_to_do,
	b,
	bi=0,
	index,
	e;


   if (Rflag > 2) 
   {
      Wscrprintf("D_flush starts\n");
      show_blocks();
   }

   lastfunc = "flush";
   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);

/***************************************************
*  Store all updated AND released blocks on disk.  *
*  Remove all released blocks from the internal    *
*  buffers so that these buffers may be used by    *
*  another routine.                                *
***************************************************/


   work_to_do = TRUE;
   while (work_to_do)
   {

/**********************************************
*  First find the block with smallest index.  *
**********************************************/

      index = MAXINT;
      for (b = 0; b < D_MAXBLOCKS; b++)
      {
         if ((filedata[fileindex].blockindex[b] < index) &&
		(filedata[fileindex].inuse[b] == NOT_IN_USE) &&
		(filedata[fileindex].hpointers[b] != NULL))
         {
            index = filedata[fileindex].blockindex[b];
            bi = b;
         }
      }

      if (index == MAXINT)
      {
         work_to_do = FALSE;
      }
      else
      {

/************************************************
*  Check to see if the block is to be updated.  *
************************************************/

         if (filedata[fileindex].umark[bi] == UPDATED)
         {
	    if ( (e = store(fileindex, bi, index, 0)) )
               return(e);
         }

	 skyrelease(filedata[fileindex].hpointers[bi]);
	 filedata[fileindex].hpointers[bi] = NULL;
	 filedata[fileindex].blockindex[bi] = EMPTY;
	 freealloc += filedata[fileindex].bufsize[bi];
	 filedata[fileindex].bufsize[bi] = 0;
      }
   }

   if (Rflag > 2) 
      Wscrprintf("D_flush done\n");

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|	    D_writeallblockheaders()/1		|
|						|
|   This function writes a block header into	|
|   all the data blocks.			|
|						|
+----------------------------------------------*/
int D_writeallblockheaders(int fileindex)
{
   int		b,
		index,
		e,
		blockheadersize;
#ifdef LINUX
   headerUnion hU;
#endif


   if (Rflag > 2)
   {
      Wscrprintf("D_writeallblockheaders starts\n");
      show_blocks();
   }

/*******************************************************
*  Check the validity of the "fileindex".  Determine   *
*  if the requested file is open (which it must be!).  *
*  Then check to see if the file can be written to.    *
*******************************************************/

   lastfunc = "writeallblockheaders";
   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (filedata[fileindex].vreadonly)
      return(D_READONLY);

/*********************************************************
*  Locate internal buffer which contains the block data  *
*  for the requested block index of "fileindex".         *
*********************************************************/

   index = 0;
   b = findblock(fileindex, index);
   if (checkblockfind(b, filedata[fileindex].hpointers))
      return(D_NOTFOUND);

/*************************************************************
*  "hpointers" is a pointer to the block header information  *
*  Write the block header into each data block using the     *
*  "blockwrite()" file handler function.                     *
*************************************************************/

   blockheadersize = (filedata[fileindex].datahead.nbheaders & NBMASK) *
			 sizeof(dblockhead);

#ifdef LINUX
   hU.in1 = filedata[fileindex].hpointers[b];
   hU.out->s1 = htons(hU.out->s1);
   hU.out->s2 = htons(hU.out->s2);
   hU.out->s3 = htons(hU.out->s3);
   hU.out->s4 = htons(hU.out->s4);
   hU.out->l1 = htonl(hU.out->l1);
   hU.out->l2 = htonl(hU.out->l2);
   hU.out->l3 = htonl(hU.out->l3);
   hU.out->l4 = htonl(hU.out->l4);
   hU.out->l5 = htonl(hU.out->l5);
#endif
   for (index = 0; index < filedata[fileindex].datahead.nblocks; index++)
   {
      filedata[fileindex].hpointers[b]->index = index;
      if ( (e = blockwrite(fileindex,
		   (char *) (filedata[fileindex].hpointers[b]),
           	   index, (off_t) 0, blockheadersize)) )
      {
         return(e);
      }
   }

   skyrelease(filedata[fileindex].hpointers[b]);    /* release the memory */
   filedata[fileindex].hpointers[b] = NULL;
   filedata[fileindex].blockindex[b] = EMPTY;
   freealloc += filedata[fileindex].bufsize[b];
   filedata[fileindex].bufsize[b] = 0;

   if (Rflag>2)
      Wscrprintf("D_writeallblockheaders done\n");

   return(COMPLETE); 
}


/*-----------------------------------------------
|						|
|	      checktransposed()/2		|
|						|
|   This function stores updated blocks of	|
|   opposite transpose status on disk.		|
|						|
+----------------------------------------------*/
/* 	fileindex      defines which file to use	 */
/* 	blockno	       block number			 */
static int checktransposed(int fileindex, int blockno)
{
   int	b,
	bi,
	index,
	e,
	min,
	max;


   if (blockno >= filedata[fileindex].datahead.nblocks)
   {
      min = 0;
      max = filedata[fileindex].datahead.nblocks - 1;
   }
   else
   {
      min = filedata[fileindex].datahead.nblocks;
      max = 2 * filedata[fileindex].datahead.nblocks - 1;
   }


   for (b = 0; b < D_MAXBLOCKS; b++)
   {
      if ((filedata[fileindex].umark[b] == UPDATED) &&
		(filedata[fileindex].blockindex[b] >= min) &&
		(filedata[fileindex].blockindex[b] <= max))
      {
         index = filedata[fileindex].blockindex[b];
         bi = b; 
	 if (filedata[fileindex].inuse[b] == IN_USE)
            return(D_TRERR);
	 if ( (e = store(fileindex, bi, index, 0)) )
            return(e);

	 skyrelease(filedata[fileindex].hpointers[bi]);
	 filedata[fileindex].hpointers[bi] = NULL;
	 filedata[fileindex].blockindex[bi] = EMPTY;
	 freealloc += filedata[fileindex].bufsize[bi];
	 filedata[fileindex].bufsize[bi] = 0;
      }
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	       D_trash()/1		|
|					|
|  This function closes the file cur-	|
|  rently being processed by the data	|
|  file handler.  It does not store on	|
|  on disk the data currently in the	|
|  buffers.				|
|					|
+--------------------------------------*/
int D_trash(int fileindex)
{
   int	b;

   lastfunc = "trash";
   if (Rflag > 2) 
   {
      Wscrprintf("D_trash started\n");
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (checkfileclosed(filedata[fileindex].curfile))
      return(D_NOTOPEN);

/*********************************
*  Release all alocated buffers  *
*********************************/

   for (b = 0; b < D_MAXBLOCKS; b++)
   {
      if (filedata[fileindex].hpointers[b] != NULL)
      {
         skyrelease(filedata[fileindex].hpointers[b]);
	 filedata[fileindex].hpointers[b] = NULL;
	 filedata[fileindex].blockindex[b] = EMPTY;
	 freealloc += filedata[fileindex].bufsize[b];
	 filedata[fileindex].bufsize[b] = 0;
      }
   }

   close(filedata[fileindex].curfile);
   filedata[fileindex].curfile = NOT_OPEN;

   if (Rflag > 2) 
      Wscrprintf("D_trash done\n");

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      D_remove()/1		|
|					|
|  This function closes the file cur-	|
|  rently being processed by the data	|
|  file handler.  It does not store on	|
|  disk either the data currently in	|
|  buffers or the null file header.	|
|					|
+--------------------------------------*/
int D_remove(int fileindex)
{
   char		filepath[MAXPATH];
   int		r;
   dfilehead	datahead;

/******************************************
*  Trash any existing file of this type.  *
******************************************/

   D_trash(fileindex);

   datahead.nblocks = 0;
   datahead.ntraces = 0;
   datahead.np = 0;
   datahead.ebytes = 0;
   datahead.tbytes = 0;
   datahead.bbytes = sizeof(dblockhead);

   datahead.nbheaders = 1;
   datahead.status = 0;
   datahead.vers_id = 0;


   if (D_getfilepath(fileindex, filepath, curexpdir))
      strcpy(filepath, "");

   if (strcmp(filepath, ""))
   {
      if ( (r = D_newhead(fileindex, filepath, &datahead)) )
      {
         D_error(r);
         return(ERROR);
      }

      if ( (r = D_close(fileindex)) )
      {
         D_error(r);
         return(ERROR);
      }
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		   D_close()/1			|
|						|
|   This function closes the file currently	|
|   being processed by the data file handler.	|
|   It stores all updated data currently being	|
|   held in the internal buffers on disk.	|
|						|
+----------------------------------------------*/
int D_close(int fileindex)
{
   int	work_to_do,
	b,
	bi = 0,
	index,
	e;


   lastfunc = "close";
   if (Rflag > 2) 
   {
      Wscrprintf("D_close(%d) started\n", fileindex);
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);

/**************************************
*  Store all updated blocks on disk.  *
**************************************/

   work_to_do = TRUE;
   while (work_to_do)
   {

/******************************************
*  First find block with smallest index.  *
******************************************/

      index = MAXINT;
      for (b = 0; b < D_MAXBLOCKS; b++)
      {
         if ((filedata[fileindex].umark[b] == UPDATED) &&
		(filedata[fileindex].blockindex[b] < index))
         {
            index = filedata[fileindex].blockindex[b];
            bi = b;
         }
      }

      if (index == MAXINT)
      {
         work_to_do = FALSE;
      }
      else
      {
         if ( (e = store(fileindex, bi, index, 0)) )
            return(e);
      }
   }

/************************************
*   Release all allocated buffers.  *
************************************/

   for (b = 0; b < D_MAXBLOCKS; b++)
   {
      if (filedata[fileindex].hpointers[b] != NULL)
      {
         skyrelease(filedata[fileindex].hpointers[b]);
	 filedata[fileindex].hpointers[b] = NULL;
	 filedata[fileindex].blockindex[b] = EMPTY;
	 freealloc += filedata[fileindex].bufsize[b];
	 filedata[fileindex].bufsize[b] = 0;
      }
   }

   close(filedata[fileindex].curfile);
   filedata[fileindex].curfile = NOT_OPEN;

   if (Rflag > 2) 
      Wscrprintf("D_close done\n");

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     findblock()/2		|
|					|
|  This function locates in the buffers	|
|  the block with the requested index.	|
|					|
+--------------------------------------*/
static int findblock(int fileindex, int index)
{
   int b;


   b = 0;
   while ((b < D_MAXBLOCKS) && (filedata[fileindex].blockindex[b] != index))
   {
      b++;
   }

   if (b < D_MAXBLOCKS)
   {
      return(b);		/* index found */
   }
   else
   {
      b = 0;
      while ((b < D_MAXBLOCKS) && (filedata[fileindex].blockindex[b] >= 0))
         b++;

      if (b < D_MAXBLOCKS)
      {
         return(b);		/* empty index found */
      }
      else
      {
         return(-1);		/* no index or empty index found */
      }
   }
}


/*--------------------------------------------
|						|
|		findreleased()/2		|
|						|
|  This function finds the block with the	|
|  largest magnitude difference between its	|
|  index and the new index.  If that block	|
|  is not in use and has been updated,	its	|
|  contents are written out to the appropriate	|
|  data block on disk.				|
|						|
+----------------------------------------------*/  
static int findreleased(int fileindex, int index)
{
   int	i,
	b,
	l,
	e,
	l1;


   b = -1;
   l = 0;
   for (i = 0; i < D_MAXBLOCKS; i++)
   {
      if ((filedata[fileindex].inuse[i] == NOT_IN_USE) &&
		(filedata[fileindex].hpointers[i] != NULL))
      {
         l1 = filedata[fileindex].blockindex[i] - index;
         if (l1 < 0)
            l1 *= (-1);
         if (l1 > l)
         {
            b = i;
            l = l1;
         }
      }
   }

   if (b < 0)
      return(b);

   if (filedata[fileindex].umark[b] == UPDATED)
   {
      if ( (e = store(fileindex, b, filedata[fileindex].blockindex[b], 0)) )
         return(e);
   }

   filedata[fileindex].umark[b] = NOT_UPDATED;
   filedata[fileindex].blockindex[b] = EMPTY;

   return(b);
}


/*-----------------------------------------------
|						|
|		 D_allocbuf()/3			|
|						|
|   This function allocates memory for the	|
|   block index of the file.  If this block	|
|   is subsequently marked as updated, all	|
|   data in the disk ifle of this block will	|
|   be overwritten.  It yields a pointer to	|
|   both the header and data of the block.	|
|						|
+----------------------------------------------*/
int D_allocbuf(int fileindex, int index, dpointers *bpntrs)
{
   int		b,
		bytes,
		b1,
		blockheadersize;

   if (Rflag > 2) 
   {
      Wscrprintf("D_allocbuf, index=%d\n", index);
      show_blocks();
   }

   lastfunc = "allocbuf";
   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (checkblockindex(index, filedata[fileindex].datahead.nblocks))
      return(D_BLINDEX);

   b = findblock(fileindex, index);
   bytes = filedata[fileindex].datahead.bbytes;
   if (b < 0)
   {
      b = findreleased(fileindex, index);
      if (b < 0)
         return(D_NOBLOCK);
      filedata[fileindex].blockindex[b] = index;
   }
   else if (filedata[fileindex].hpointers[b] == NULL)
   {
      if (freealloc < bytes)			/* not enough memory */
      {
         b1 = findreleased(fileindex, index);	/* reuse a buffer space? */
         if (b1 < 0)				/* if none available */
	 {

/***************************************
*  Flush one or both sets of buffers.  *
***************************************/

            D_flush((fileindex + 1) % D_MAXFILES);
	    if (freealloc < bytes)
               D_flush((fileindex + 2) % D_MAXFILES);
            filedata[fileindex].hpointers[b] = (dblockhead *) skyallocateWithId(bytes, "data");
            if (filedata[fileindex].hpointers[b] == NULL)
               return(D_NOALLOC);

            freealloc -= bytes;
	    filedata[fileindex].bufsize[b] = bytes;
	 }
         else
         {
            b = b1;				/* ok to reuse */
         }
      }
      else					/* allocate new memory */ 
      {
         filedata[fileindex].hpointers[b] = (dblockhead *) skyallocateWithId(bytes, "data");
         if (filedata[fileindex].hpointers[b] == NULL)
         {
            b = findreleased(fileindex, index);
            if (b < 0)
               return(D_NOALLOC);
         }
         else
         {
            freealloc -= bytes;
         }

	 filedata[fileindex].bufsize[b] = bytes;
      }

      filedata[fileindex].blockindex[b] = index;
   }

   blockheadersize = (filedata[fileindex].datahead.nbheaders & NBMASK) *
			sizeof(dblockhead);
   bpntrs->head = filedata[fileindex].hpointers[b];
   bpntrs->data = (float *) ((char *) (bpntrs->head) + blockheadersize);
   filedata[fileindex].umark[b] = NOT_UPDATED;
   filedata[fileindex].inuse[b] = IN_USE;

   if (Rflag > 2) 
      Wscrprintf("D_allocbuf done\n");

   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|                D_getbuf()/4                   |
|                                               |
|   This function allocates memory for the      |
|   block with the specified index.  It then    |
|   reads the block in from the disk file.  It  |
|   yields a pointer to the both the header     |
|   and the data of this block.  It does not    |
|   allocate memory or read in the block if     |
|   the block is already in memory.  This	|
|   routine is specially tailored for reading	|
|   2D interferogram data prior to the F1 FT.   |
|                                               |
+----------------------------------------------*/
int D_getbuf(int fileindex, int nrdblocks, int index, dpointers *bpntrs)
{
   char *buf2;
   int  version_id,
	b,
        e,
	i,           
        bb,
        dtype,
        blockheadersize;
#ifdef LINUX
   headerUnion hU;
   register int cnt, num;
   register short *sptr;
   register int  *lptr;
#endif
 
 
   lastfunc = "getbuf";
   if (Rflag > 2)
   {
      Wscrprintf("D_getbuf started, index=%d\n", index);
      show_blocks();
   }
 
   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (checkblockindex(index, filedata[fileindex].datahead.nblocks))
      return(D_BLINDEX);
        
   version_id = filedata[fileindex].datahead.vers_id & P_VERS;
   blockheadersize = (filedata[fileindex].datahead.nbheaders & NBMASK) *
                        sizeof(dblockhead);
   dtype = getdatatype(filedata[fileindex].datahead.status);

 
   b = findblock(fileindex, index);
   if ((b >= 0) && (filedata[fileindex].hpointers[b] != NULL) &&
                        (filedata[fileindex].blockindex[b] == index))
   {
 
/*****************************
*  Block is still in memory  *
*****************************/
 
      if (Rflag > 2)
         Wscrprintf("D_getbuf, in memory, fileindex=%d, index=%d, b=%d\n",
                                  fileindex, index, b);
      filedata[fileindex].inuse[b] = IN_USE;
      bpntrs->head = filedata[fileindex].hpointers[b];
      bpntrs->data = (float *)((char *) (bpntrs->head) + blockheadersize);
   }
   else
   {
      if (filedata[fileindex].datahead.status & S_TRANSF)
 
/*********************************************
*  Blocks with oposite transpose status may  *
*  not have been stored yet.                 *
*********************************************/
 
      {
         if (filedata[fileindex].datahead.nblocks == 1)
         {
 
/************************************************
*  Check if block of opposite transpose status  *
*  is in memory.                                *
************************************************/
 
            if (index)
            {
               bb = 0;
            }
            else
            {
               bb = 1;
            }
 
            b = 0;
            while ((b < D_MAXBLOCKS) &&
                        (filedata[fileindex].blockindex[b] != bb))
            {
               b++;
            }

            if (b < D_MAXBLOCKS)
            {
 
/*********************************************
*  Found block of opposite transpose status  *
*  in memory.                                *
*********************************************/
 
               if (filedata[fileindex].inuse[b] == IN_USE)
               {
 
/***************************************************
*  If still in use, allocate space and transpose.  *
***************************************************/
 
                  Wscrprintf("allocate and transpose not implemented\n");
                  if ( (e = checktransposed(fileindex, index)) )
                     return(e);
               }
               else
               {
 
/***********************************
*  Transpose internally in block.  *
***********************************/
 
                  if (bb == 1)
                  {
 
/************************
*  transpose backwards  *
************************/

                     filedata[fileindex].blockindex[b] = 0;
                     filedata[fileindex].hpointers[b]->index = 0;
                     transpose((char *) (filedata[fileindex].hpointers[b]) +
                         blockheadersize, filedata[fileindex].datahead.ntraces,
                         filedata[fileindex].datahead.np/dtype, dtype);
                  }
                  else
                  {
 
/***********************
*  transpose forwards  *
***********************/
 
                     filedata[fileindex].blockindex[b] = 1;
                     filedata[fileindex].hpointers[b]->index = 1;
                     transpose((char *) (filedata[fileindex].hpointers[b]) +
                         blockheadersize, filedata[fileindex].datahead.np/dtype,                         filedata[fileindex].datahead.ntraces, dtype);
                  }
 
                  filedata[fileindex].inuse[b] = IN_USE;
                  bpntrs->head = filedata[fileindex].hpointers[b];
                  bpntrs->data = (float *)( (char *) (bpntrs->head) +
                                        blockheadersize );
                  return(COMPLETE);
               }
            }
         }
         else
         {
 
/**************************
*  Store all such blocks  *
**************************/
 
            if ( (e = checktransposed(fileindex, index)) )
               return(e);
         }
      }
 
/******************************************
*  Block is not in memory.  Data storage  *
*  must therefore be allocated.           *
******************************************/
 
      if ( (e = D_allocbuf(fileindex, index, bpntrs)) )
         return(e);
      b = findblock(fileindex,index);
      if (Rflag > 2)
         Wscrprintf("reading block no %d\n",index);
 
      if (filedata[fileindex].datahead.status & S_TRANSF)
      {
         if (index >= filedata[fileindex].datahead.nblocks)
         {
            index -= filedata[fileindex].datahead.nblocks;
            bb = (filedata[fileindex].datahead.bbytes - blockheadersize) /
                        filedata[fileindex].datahead.nblocks;
 
            buf2 = (char *) (filedata[fileindex].hpointers[b]) +
                        blockheadersize;
 
/*********************************
*  Read the block header first.  *
*********************************/
 
            if ( (e = blockread(fileindex,
                        (char *) (filedata[fileindex].hpointers[b]), index,
			(off_t) 0, blockheadersize)) )
            {
               return(e);
            }
#ifdef LINUX
            hU.in1 = filedata[fileindex].hpointers[b];
            hU.out->s1 = ntohs(hU.out->s1);
            hU.out->s2 = ntohs(hU.out->s2);
            hU.out->s3 = ntohs(hU.out->s3);
            hU.out->s4 = ntohs(hU.out->s4);
            hU.out->l1 = ntohl(hU.out->l1);
            hU.out->l2 = ntohl(hU.out->l2);
            hU.out->l3 = ntohl(hU.out->l3);
            hU.out->l4 = ntohl(hU.out->l4);
            hU.out->l5 = ntohl(hU.out->l5);
#endif
 
            for (i = 0; i < nrdblocks; i++)
            {
               off_t offset;
               offset = (off_t) index * (off_t) bb + (off_t) blockheadersize;
               if ( (e = blockread(fileindex, buf2, i, offset, bb)) )
               {
                  return(e);
               }
#ifdef LINUX
               num = bb / filedata[fileindex].datahead.ebytes;
               if (filedata[fileindex].datahead.ebytes == 2)
               {
                  sptr = (short *) buf2;
                  for (cnt = 0; cnt < num; cnt++)
                  {
                     *sptr = ntohs(*sptr);
                      sptr++;
                  }
               }
               else
               {
                  lptr = (int *) buf2;
                  for (cnt = 0; cnt < num; cnt++)
                  {
                     *lptr = ntohl(*lptr);
                      lptr++;
                  }
               }
#endif
 
               transpose(buf2, filedata[fileindex].datahead.ntraces,
                    filedata[fileindex].datahead.np /
                    (dtype * filedata[fileindex].datahead.nblocks),
                    dtype);
 
               buf2 += bb;
            }
 
            transpose((char *) (filedata[fileindex].hpointers[b]) +
                 blockheadersize, filedata[fileindex].datahead.np /
                 (dtype * filedata[fileindex].datahead.nblocks),
                 filedata[fileindex].datahead.ntraces *
                 filedata[fileindex].datahead.nblocks,
                 dtype);
         }
         else
         {
            if ( (e = blockread(fileindex,
                        (char *) (filedata[fileindex].hpointers[b]),
                        index, (off_t) 0, filedata[fileindex].datahead.bbytes)) )
            {
               return(e);
            }            
#ifdef LINUX
            hU.in1 = filedata[fileindex].hpointers[b];
            hU.out->s1 = ntohs(hU.out->s1);
            hU.out->s2 = ntohs(hU.out->s2);
            hU.out->s3 = ntohs(hU.out->s3);
            hU.out->s4 = ntohs(hU.out->s4);
            hU.out->l1 = ntohl(hU.out->l1);
            hU.out->l2 = ntohl(hU.out->l2);
            hU.out->l3 = ntohl(hU.out->l3);
            hU.out->l4 = ntohl(hU.out->l4);
            hU.out->l5 = ntohl(hU.out->l5);
            num = filedata[fileindex].datahead.np * filedata[fileindex].datahead.ntraces;
            if (filedata[fileindex].datahead.ebytes == 2)
            {
               sptr = (short *) bpntrs->data;
               for (cnt = 0; cnt < num; cnt++)
               {
                  *sptr = ntohs(*sptr);
                   sptr++;
               }
            }
            else
            {
               lptr = (int *) bpntrs->data;
               for (cnt = 0; cnt < num; cnt++)
               {
                  *lptr = ntohl(*lptr);
                   lptr++;
               }
            }
#endif
 
            transpose((char *) (filedata[fileindex].hpointers[b]) +
                  blockheadersize, filedata[fileindex].datahead.ntraces,
                  filedata[fileindex].datahead.np/dtype, dtype);
         }
      }
      else
      {
         if ( (e = blockread(fileindex,
                     (char *) (filedata[fileindex].hpointers[b]),
                     index, (off_t) 0, filedata[fileindex].datahead.bbytes)) )
         {
            return(e);
         }
#ifdef LINUX
         hU.in1 = filedata[fileindex].hpointers[b];
         hU.out->s1 = ntohs(hU.out->s1);
         hU.out->s2 = ntohs(hU.out->s2);
         hU.out->s3 = ntohs(hU.out->s3);
         hU.out->s4 = ntohs(hU.out->s4);
         hU.out->l1 = ntohl(hU.out->l1);
         hU.out->l2 = ntohl(hU.out->l2);
         hU.out->l3 = ntohl(hU.out->l3);
         hU.out->l4 = ntohl(hU.out->l4);
         hU.out->l5 = ntohl(hU.out->l5);
         num = filedata[fileindex].datahead.np * filedata[fileindex].datahead.ntraces;
         if (filedata[fileindex].datahead.ebytes == 2)
         {
            sptr = (short *) bpntrs->data;
            for (cnt = 0; cnt < num; cnt++)
            {
               *sptr = ntohs(*sptr);
                sptr++;
            }
         }
         else
         {
            lptr = (int *) bpntrs->data;
            for (cnt = 0; cnt < num; cnt++)
            {
               *lptr = ntohl(*lptr);
                lptr++;
            }
         }
#endif
      }
   }
 

   if (fileindex == D_USERFILE)
   {
      if (fidversion((char *)bpntrs->head, BLOCKHEAD, fid_version_id))
         return(ERROR);
   }

   if (Rflag > 2)
      Wscrprintf("D_getbuf done\n");
 
   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		 D_release()/2			|
|						|
|   This function releases the block with the	|
|   specified index from the internal Vnmr	|
|   memory buffers.  Once the block is re-	|
|   leased, the memory can be used again to	|
|   store other blocks.				|
|						|
+----------------------------------------------*/
int D_release(int fileindex, int index)
{
   int	b;


   lastfunc = "release";
   if (Rflag > 2)
   {
      Wscrprintf("D_release, index=%d\n", index);
      show_blocks();
   }

/*******************************************************
*  Check the validity of the "fileindex".  Determine   *
*  if the requested file is open (which it must be!).  *
*  Finally, check the validity of the specified block  *
*  index.                                              *
*******************************************************/

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (checkblockindex(index, filedata[fileindex].datahead.nblocks))
      return(D_FLINDEX);

/*********************************************************
*  Locate internal buffer which contains the block data  *
*  for the requested block index of "fileindex".         *
*********************************************************/

   b = findblock(fileindex, index);
   if (checkblockfind(b, filedata[fileindex].hpointers[b]))
      return(COMPLETE);
   filedata[fileindex].inuse[b] = NOT_IN_USE;

   if (Rflag > 2)
      Wscrprintf("D_release done\n");

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		D_allrelease()/0		|
|						|
|  This function releases all internal buffers	|
|  associated with any of the three possible	|
|  "fileindices".				|
|						|
+----------------------------------------------*/
void D_allrelease()
{
   int	fileindex,
	b;


   lastfunc = "allrelease";
   for (fileindex = D_DATAFILE; fileindex <= D_USERFILE; fileindex++)
   {
      for (b = 0; b < D_MAXBLOCKS; b++)
      {
         if (filedata[fileindex].inuse[b] == IN_USE)
            filedata[fileindex].inuse[b] = NOT_IN_USE;
      }
   }
}


/*-----------------------------------------------
|						|
|		D_markupdated()/2		|
|						|
|   This function marks the internal buffer	|
|   to indicate that the associated data block	|
|   has been updated and therefore is to be	|
|   written out to disk if the internal buffer	|
|   is released at any time.			|
|						|
+----------------------------------------------*/
int D_markupdated(int fileindex, int index)
{
   int	b,
	min,
	max;


   lastfunc = "markupdated";
   if (Rflag > 2)
   {
      Wscrprintf("D_markupdated, index=%d\n", index);
      show_blocks();
   }

/*******************************************************
*  Check the validity of the "fileindex".  Determine   *
*  if the requested file is open (which it must be!).  *
*  Then check to see if the file can be written to.    *
*  Finally, check the validity of the specified block  *
*  index.                                              *
*******************************************************/

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (filedata[fileindex].vreadonly)
      return(D_READONLY);
   if (checkblockindex(index, filedata[fileindex].datahead.nblocks))
      return(D_BLINDEX);

/*********************************************************
*  Locate internal buffer which contains the block data  *
*  for the requested block index of "fileindex".         *
*********************************************************/

   b = findblock(fileindex, index);
   if (checkblockfind(b, filedata[fileindex].hpointers[b]))
      return(D_NOTFOUND);
   filedata[fileindex].umark[b] = UPDATED;
   filedata[fileindex].inuse[b] = IN_USE;

/**************************************
*  This section is only for 2D data.  *
**************************************/

   if (filedata[fileindex].datahead.status & S_TRANSF)
   {
      if (index >= filedata[fileindex].datahead.nblocks)
      {
         min = 0;
         max = filedata[fileindex].datahead.nblocks - 1;
      }
      else
      {
         min = filedata[fileindex].datahead.nblocks;
	 max = 2 * filedata[fileindex].datahead.nblocks - 1;
      }

      for (b = 0; b < D_MAXBLOCKS; b++)
      {
         if ((filedata[fileindex].blockindex[b] >= min) &&
		(filedata[fileindex].blockindex[b] <= max) &&
		(filedata[fileindex].hpointers[b] != NULL))
	 {
            if ((filedata[fileindex].inuse[b] == IN_USE) ||
			(filedata[fileindex].umark[b] == UPDATED))
	    {
               return(D_TRERR);
	    }
            else 
	    {
               skyrelease(filedata[fileindex].hpointers[b]);
	       filedata[fileindex].hpointers[b] = NULL;
	       filedata[fileindex].blockindex[b] = EMPTY;
	       freealloc += filedata[fileindex].bufsize[b];
	       filedata[fileindex].bufsize[b] = 0;
	    }
	 }
      }
   }

   if (Rflag > 2)
      Wscrprintf("D_markupdated done\n");

/*
   if(fileindex == D_DATAFILE || fileindex == D_PHASFILE)
   	p11_saveFDAfiles_processed("D_markupdated", "-", "datdir");
*/

   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|               D_getblhead()/3                 |
|                                               |
|  This function reads the block header for     |
|  the specified data block.                    |
|                                               |
+----------------------------------------------*/
int D_getblhead(int fileindex, int index, dblockhead *blhead)
{
   int	b;
#ifdef LINUX
   headerUnion hU;
#endif


   lastfunc = "getblhead";
   if (Rflag > 2)
   {
      Wscrprintf("D_getblhead started, index=%d\n", index);
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (checkblockindex(index, filedata[fileindex].datahead.nblocks))
      return(D_BLINDEX);

 
   b = findblock(fileindex, index);
   if ((b >= 0) && (filedata[fileindex].hpointers[b] != NULL) &&
                        (filedata[fileindex].blockindex[b] == index))
   {

/*****************************
*  Block is still in memory  *
*****************************/

      if (Rflag > 2)           
      {
         Wscrprintf("D_getblhead, in memory, fileindex=%d, index=%d, b=%d\n",
			fileindex, index, b);
      }

      filedata[fileindex].inuse[b] = IN_USE;
      blhead = filedata[fileindex].hpointers[b];
   }
   else
   {
      off_t offset;
   
      offset = (off_t) index * (off_t) filedata[fileindex].datahead.bbytes +
               (off_t) sizeof(dfilehead);
      if ( lseek(filedata[fileindex].curfile, offset, SEEK_SET) < 0 )
      {
         return(D_SEEKERR);
      }

      if ( read(filedata[fileindex].curfile, (char *)blhead,
		   sizeof(dblockhead)) != sizeof(dblockhead) )
      {
         return(D_READERR);
      }
#ifdef LINUX
      hU.in1 = blhead;
      hU.out->s1 = ntohs(hU.out->s1);
      hU.out->s2 = ntohs(hU.out->s2);
      hU.out->s3 = ntohs(hU.out->s3);
      hU.out->s4 = ntohs(hU.out->s4);
      hU.out->l1 = ntohl(hU.out->l1);
      hU.out->l2 = ntohl(hU.out->l2);
      hU.out->l3 = ntohl(hU.out->l3);
      hU.out->l4 = ntohl(hU.out->l4);
      hU.out->l5 = ntohl(hU.out->l5);
#endif
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		 D_gethead()/2			|
|						|
|  This function moves the "static internal	|
|  file header" into a pointer to an external	|
|  (user-selected) data file header structure.	|
|						|
+----------------------------------------------*/
int D_gethead(int fileindex, dfilehead *fhead)
{
   lastfunc = "gethead";
   if (Rflag > 2)
   {
      Wscrprintf("D_gethead started\n");
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);

   movmem((char *) (&filedata[fileindex].datahead), (char *)fhead,
			sizeof(dfilehead), 1, 1);

   if (Rflag > 2)
      Wscrprintf("D_gethead done\n");

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		 D_newhead()/3			|
|						|
|   This function opens a disk file with a new	|
|   file header.  The current contents in mem-	|
|   mory of the	old file are lost.		|
|						|
+----------------------------------------------*/
int D_newhead(int fileindex, char *filepath, dfilehead *fhead)
{
   int	blockheadersize;
#ifdef LINUX
   dfilehead swaphead;
#endif


   lastfunc = "newhead";
   if (Rflag > 2)
   {
      Wscrprintf("D_newhead started\n");
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (checkfileopen(filedata[fileindex].curfile))
      return(D_IS_OPEN);

   blockheadersize = (fhead->nbheaders & NBMASK) * sizeof(dblockhead);

   binit(fileindex);
   if (fhead->bbytes < blockheadersize)
      return(D_INCONSI);
   if (fhead->bbytes != (fhead->tbytes * fhead->ntraces +
              blockheadersize))
   {
      return(D_INCONSI);
   }

   unlink(filepath);
   filedata[fileindex].curfile = open(filepath, O_RDWR|O_EXCL|O_CREAT, 0666);
   filedata[fileindex].vreadonly = FALSE;
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);

   movmem((char *)fhead, (char *) (&filedata[fileindex].datahead),
			sizeof(dfilehead), 1, 1);

   filedata[fileindex].datahead.vers_id &= P_VENDOR_ID;	/* preserves vendor
							   ID status */
   filedata[fileindex].datahead.vers_id += VERSION;	/* loads version no. */
   if (fileindex == D_DATAFILE)
   {

      filedata[fileindex].datahead.vers_id += DATA_FILE;
   }
   else if (fileindex == D_PHASFILE)
   {
      filedata[fileindex].datahead.vers_id += PHAS_FILE;
   }

#ifdef LINUX
   movmem((char *)(&filedata[fileindex].datahead), (char *) (&swaphead),
			sizeof(dfilehead), 1, 1);
   swaphead.nblocks = htonl(swaphead.nblocks);
   swaphead.ntraces = htonl(swaphead.ntraces);
   swaphead.np = htonl(swaphead.np);
   swaphead.ebytes = htonl(swaphead.ebytes);
   swaphead.tbytes = htonl(swaphead.tbytes);
   swaphead.bbytes = htonl(swaphead.bbytes);
   swaphead.vers_id = htons(swaphead.vers_id);
   swaphead.status = htons(swaphead.status);
   swaphead.nbheaders = htonl(swaphead.nbheaders);
   if (write(filedata[fileindex].curfile, &swaphead,
		sizeof(dfilehead)) <= 0)
   {
      return(D_WRITERR);
   }
#else
   if (write(filedata[fileindex].curfile, &filedata[fileindex].datahead,
		sizeof(dfilehead)) <= 0)
   {
      return(D_WRITERR);
   }
#endif

   if (Rflag > 2)
      Wscrprintf("D_newhead done\n");

/*
   if(fileindex == D_DATAFILE || fileindex == D_PHASFILE)
   	p11_saveFDAfiles_processed("D_newhead", "-", "datdir");
*/

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		D_updatehead()/2		|
|						|
|  This function updates the disk file header.	|
|						|
+----------------------------------------------*/
int D_updatehead(int fileindex, dfilehead *fhead)
{
   int	blockheadersize;
#ifdef LINUX
   dfilehead swaphead;
#endif


   lastfunc = "updatehead";
   if (Rflag > 2)
   {
      Wscrprintf("D_updatehead started\n");
      show_blocks();
   }

   if (checkfileindex(fileindex))
      return(D_FLINDEX);
   if (!checkfileopen(filedata[fileindex].curfile))
      return(D_NOTOPEN);
   if (filedata[fileindex].vreadonly)
      return(D_READONLY);

   blockheadersize = (fhead->nbheaders & NBMASK) * sizeof(dblockhead);

   if (fhead->bbytes < blockheadersize)
      return(D_INCONSI);
   if (fhead->bbytes != (fhead->tbytes * fhead->ntraces +
              blockheadersize))
   {
      return(D_INCONSI);
   }

   movmem((char *)fhead, (char *) (&filedata[fileindex].datahead),
			sizeof(dfilehead), 1, 1);

   if (lseek(filedata[fileindex].curfile, 0, SEEK_SET) < 0)
      return(D_SEEKERR);
#ifdef LINUX
   movmem((char *)(&filedata[fileindex].datahead), (char *) (&swaphead),
			sizeof(dfilehead), 1, 1);
   swaphead.nblocks = htonl(swaphead.nblocks);
   swaphead.ntraces = htonl(swaphead.ntraces);
   swaphead.np = htonl(swaphead.np);
   swaphead.ebytes = htonl(swaphead.ebytes);
   swaphead.tbytes = htonl(swaphead.tbytes);
   swaphead.bbytes = htonl(swaphead.bbytes);
   swaphead.vers_id = htons(swaphead.vers_id);
   swaphead.status = htons(swaphead.status);
   swaphead.nbheaders = htonl(swaphead.nbheaders);
   if (write(filedata[fileindex].curfile, &swaphead,
		sizeof(dfilehead)) <= 0)
   {
      return(D_WRITERR);
   }
#else
   if (write(filedata[fileindex].curfile, &filedata[fileindex].datahead,
		sizeof(dfilehead)) <= 0)
   {
      return(D_WRITERR);
   }
#endif

   if (Rflag > 2)
      Wscrprintf("D_updatehead done\n");

   return(COMPLETE);
}


/*---------------------------------------
|					|
|		D_error()/1		|
|					|
|   This function displays an error	|
|   message for a data file handler	|
|   error.				|
|					|
+--------------------------------------*/
void D_error(int errnum)
{
   switch (errnum)
   {
      case D_IS_OPEN:  Werrprintf("Data file is already open"); break;
      case D_NOTOPEN:  Werrprintf("Data file is not open currently"); break;
      case D_READERR:  Werrprintf("File read error"); break;
      case D_WRITERR:  Werrprintf("File write error"); break;
      case D_INCONSI:  Werrprintf("Inconsistent data file header"); break;
      case D_NOALLOC:  Werrprintf("Buffer not allocated, out of memory"); break;
      case D_NOBLOCK:  Werrprintf("Too many blocked blocks in memory"); break;
      case D_NOTFOUND: Werrprintf("Requested block not in memory"); break;
      case D_SEEKERR:  Werrprintf("File seek error"); break;
      case D_BLINDEX:  Werrprintf("Illegal block index"); break;
      case D_FLINDEX:  Werrprintf("Illegal file index"); break;
      case D_BUFERR:   Werrprintf("Internal buffer too small"); break;
      case D_TRERR:    Werrprintf("Cannot transpose block"); break;
      case D_READONLY: Werrprintf("No write permission"); break;
      case D_INVNBHDR: Werrprintf("Invalid number of block headers"); break;
      case D_INVOP:    Werrprintf("Invalid file operation"); break;
      case D_TRUNCERR: Werrprintf("File truncation error"); break;
      case D_OPENERR:  Werrprintf("Cannot open data file"); break;
      case ERROR:      break;
      default:         Werrprintf("D_error: unknown error number"); break;
   }

   if (Rflag>1)
      Wscrprintf("  Last function called = D_%s\n", lastfunc);
}

char *D_geterror(int errnum)
{
   static char msg[64];
   switch (errnum)
   {
      case D_IS_OPEN:  strcpy(msg,"Data file is already open"); break;
      case D_NOTOPEN:  strcpy(msg,"Data file is not open currently"); break;
      case D_READERR:  strcpy(msg,"File read error"); break;
      case D_WRITERR:  strcpy(msg,"File write error"); break;
      case D_INCONSI:  strcpy(msg,"Inconsistent data file header"); break;
      case D_NOALLOC:  strcpy(msg,"Buffer not allocated, out of memory"); break;
      case D_NOBLOCK:  strcpy(msg,"Too many blocked blocks in memory"); break;
      case D_NOTFOUND: strcpy(msg,"Requested block not in memory"); break;
      case D_SEEKERR:  strcpy(msg,"File seek error"); break;
      case D_BLINDEX:  strcpy(msg,"Illegal block index"); break;
      case D_FLINDEX:  strcpy(msg,"Illegal file index"); break;
      case D_BUFERR:   strcpy(msg,"Internal buffer too small"); break;
      case D_TRERR:    strcpy(msg,"Cannot transpose block"); break;
      case D_READONLY: strcpy(msg,"No write permission"); break;
      case D_INVNBHDR: strcpy(msg,"Invalid number of block headers"); break;
      case D_INVOP:    strcpy(msg,"Invalid file operation"); break;
      case D_TRUNCERR: strcpy(msg,"File truncation error"); break;
      case D_OPENERR:  strcpy(msg,"Cannot open data file"); break;
      case ERROR:      break;
      default:         strcpy(msg,"D_error: unknown error number"); break;
   }
   return(msg);
}


/*---------------------------------------
|					|
|	   D_checkfileinfo()/1		|
|					|
+--------------------------------------*/
int D_checkfileinfo(int fileindex)
{
   int	i;


   if (checkfileindex(fileindex))
      return(D_FLINDEX);

   Wscrprintf("\n\nFile Index      = %d\n", fileindex);
   Wscrprintf("File Descriptor = %d\n", filedata[fileindex].curfile);
   Wscrprintf("File Read Only  = %d\n\n", filedata[fileindex].vreadonly);

   for (i = 0; i < D_MAXBLOCKS; i++)
   {
      Wscrprintf("Block Number  = %d\n", i);
      Wscrprintf("Block Index   = %d\n", filedata[fileindex].blockindex[i]);
      Wscrprintf("Block Size    = %d\n", filedata[fileindex].bufsize[i]);
      Wscrprintf("Block In Use  = %d\n", filedata[fileindex].inuse[i]);
      Wscrprintf("Block Updated = %d\n", filedata[fileindex].umark[i]);

      if (filedata[fileindex].hpointers[i] == NULL)
      {
         Wscrprintf("Block Pointer = NULL\n\n");
      }
      else
      {
         Wscrprintf("Block Pointer = %p\n\n", filedata[fileindex].hpointers[i]);
      }
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     setfilepaths()/1		|
|					|
+---------------------------------------*/
void setfilepaths(int datano)
{
   char	fext[20],
	filepath[MAXPATH];


   if (datano == 0)
   {
      strcpy(curfilepaths.datafile, "data");
      strcpy(curfilepaths.phasefile, "phasefile");
      strcpy(curfilepaths.curpar, "curpar");
      strcpy(curfilepaths.procpar, "procpar");
      return;
   }

   sprintf(fext, ".%d", datano);

   strcpy(filepath, "data");
   strcat(filepath, fext);
   strcpy(curfilepaths.datafile, filepath);

   strcpy(filepath, "phasefile"); 
   strcat(filepath, fext); 
   strcpy(curfilepaths.phasefile, filepath);

   strcpy(filepath, "curpar");  
   strcat(filepath, fext);  
   strcpy(curfilepaths.curpar, filepath);

   strcpy(filepath, "procpar");   
   strcat(filepath, fext);   
   strcpy(curfilepaths.procpar, filepath);
}


/*---------------------------------------
|					|
|	  D_getparfilepath()/3		|
|					|
+--------------------------------------*/
int D_getparfilepath(int partree, char *filepath, char *basepath)
{
   if (filepath != basepath)
      strcpy(filepath, basepath);
   switch(partree)
   {
      case CURRENT:
#ifdef UNIX
		strcat(filepath, "/");
#endif 
		strcat(filepath, curfilepaths.curpar);
		break;
	case PROCESSED:
#ifdef UNIX
		strcat(filepath, "/");
#endif 
		strcat(filepath, curfilepaths.procpar);
		break;
	default:
		return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	   D_getfilepath()/3		|
|					|
+--------------------------------------*/
int D_getfilepath(int fileindex, char *filepath, char *basepath)
{
   if (checkfileindex(fileindex))
      return(D_FLINDEX);

   strcpy(filepath, basepath);
   switch(fileindex)
   {
       case D_PHASFILE:
#ifdef UNIX
  		strcat(filepath, "/datdir/");
#else 
		vms_fname_cat(filepath, "[.datdir]");
#endif 
		strcat(filepath, curfilepaths.phasefile);
		break;
       case D_DATAFILE:
#ifdef UNIX
  		strcat(filepath, "/datdir/");
#else 
  		vms_fname_cat(filepath, "[.datdir]");
#endif 
		strcat(filepath, curfilepaths.datafile);
		break;
       case D_USERFILE:
#ifdef UNIX
  		strcat(filepath, "/acqfil/fid");
#else 
  		vms_fname_cat(filepath, "[.acqfil]fid");
#endif 
		break;

/*  This should have been caught earlier.  */

	default:
		return(D_FLINDEX);
   }

   return( COMPLETE );
}


/*---------------------------------------
|					|
|	      D_foldt()/6		|
|					|
+--------------------------------------*/
int D_foldt(int fileindex, float *datapntr, int seekmode,
            int seekbytes, int iomode, int iobytes)
{
   int	fd;
#ifdef LINUX
   register int *lptr;
   register int cnt;
   register int num;
#endif

   if (checkfileindex(fileindex))
      return(D_FLINDEX);

   fd = filedata[fileindex].curfile;
   if (checkfileclosed(fd))
      return(D_NOTOPEN);

   if (lseek(fd, (off_t)seekbytes, seekmode) < 0)
      return(D_SEEKERR);

   switch (iomode)
   {
      case WRITE_DATA:
      {
#ifdef LINUX
         lptr = (int *) datapntr;
         num = iobytes / sizeof(float);
         for (cnt = 0; cnt < num; cnt++)
         {
            *lptr = htonl(*lptr);
             lptr++;
         }
#endif
       
         if (write(fd, (char *)datapntr, (unsigned)iobytes) != iobytes) 
            return(D_WRITERR);
         break;
      }
      case READ_DATA:
      {
         if (read(fd, (char *)datapntr, (unsigned)iobytes) != iobytes)
            return(D_READERR);
#ifdef LINUX
         lptr = (int *) datapntr;
         num = iobytes / sizeof(float);
         for (cnt = 0; cnt < num; cnt++)
         {
            *lptr = ntohl(*lptr);
             lptr++;
         }
#endif
         break;
      }
      default: return(D_INVOP);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     D_compress()/0		|
|					|
+--------------------------------------*/
int D_compress()
{
   char			datapath[MAXPATH];
   int			i,
			r,
			fd,
			nblocks,
			bbytesOrig,
			bbytesNew,
			nbheader,
			nbheaderbytes,
			setbytes,
			incrbytes;
   register int         j,
                        cnt;
   register float	*srcpntr,
			*destpntr;
   dfilehead		datafilehead;
   dpointers		datablock;


   if ( (r = D_getfilepath(D_DATAFILE, datapath, curexpdir)) )
      return(r);

   if ( (fd = open(datapath, O_RDWR, 0666)) < 0 )
      return(D_OPENERR);

/***********************************
*  Read in file header and modify  *
*  for the compressed file.        *
***********************************/

   if ( read(fd, (char *)(&datafilehead), sizeof(dfilehead))
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_READERR);
   }
#ifdef LINUX
   datafilehead.nblocks = ntohl(datafilehead.nblocks);
   datafilehead.ntraces = ntohl(datafilehead.ntraces);
   datafilehead.np = ntohl(datafilehead.np);
   datafilehead.ebytes = ntohl(datafilehead.ebytes);
   datafilehead.tbytes = ntohl(datafilehead.tbytes);
   datafilehead.bbytes = ntohl(datafilehead.bbytes);
   datafilehead.vers_id = ntohs(datafilehead.vers_id);
   datafilehead.status = ntohs(datafilehead.status);
   datafilehead.nbheaders = ntohl(datafilehead.nbheaders);
#endif

   nblocks = datafilehead.nblocks;
   nbheader = (datafilehead.nbheaders & NBMASK);
   nbheaderbytes = nbheader * sizeof(dblockhead);
   bbytesOrig = datafilehead.bbytes;

   datafilehead.status &= (~S_COMPLEX);
   datafilehead.np /= 2;
   datafilehead.tbytes /= 2;
   datafilehead.bbytes = datafilehead.tbytes*datafilehead.ntraces +
				nbheaderbytes;
   bbytesNew = datafilehead.bbytes;
   cnt = datafilehead.np*datafilehead.ntraces;

#ifdef LINUX
   datafilehead.nblocks = htonl(datafilehead.nblocks);
   datafilehead.ntraces = htonl(datafilehead.ntraces);
   datafilehead.np = htonl(datafilehead.np);
   datafilehead.ebytes = htonl(datafilehead.ebytes);
   datafilehead.tbytes = htonl(datafilehead.tbytes);
   datafilehead.bbytes = htonl(datafilehead.bbytes);
   datafilehead.vers_id = htons(datafilehead.vers_id);
   datafilehead.status = htons(datafilehead.status);
   datafilehead.nbheaders = htonl(datafilehead.nbheaders);
#endif
/************************************
*  Seek to start of file and write  *
*  out the new file header.         *
************************************/

   if ( lseek(fd, 0, SEEK_SET) < 0 )
   {
      close(fd);
      return(D_SEEKERR);
   }

   if ( write(fd, (char *)(&datafilehead), sizeof(dfilehead) )
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_WRITERR);
   }

/******************************
*  Allocate memory for data.  *
******************************/

   if ( (datablock.head = (dblockhead *) allocateWithId(bbytesOrig,
		"compress")) == NULL )
   {
      close(fd);
      return(D_NOALLOC);
   }

   datablock.data = (float *) (datablock.head + nbheader);
   setbytes = sizeof(dfilehead);
   incrbytes = bbytesNew - nbheaderbytes;


/***********************
*  Start compression.  *
***********************/

   for (i = 0; i < nblocks; i++)
   {
      if ( read(fd, (char *)datablock.head, bbytesOrig) != bbytesOrig )
      {
         close(fd);
         releaseAllWithId("compress");
         return(D_READERR);
      }

#ifdef LINUX
      datablock.head->status = ntohs(datablock.head->status);
      datablock.head->status &= (~S_COMPLEX);
      datablock.head->status = htons(datablock.head->status);
#else
      datablock.head->status &= (~S_COMPLEX);
#endif
      srcpntr = destpntr = datablock.data;

      for (j = 0; j < cnt; j++)
      {
         *destpntr++ = *srcpntr;
         srcpntr += 2;
      }

      if ( lseek(fd, (off_t)setbytes, SEEK_SET) < 0 )
      {
         close(fd);
         releaseAllWithId("compress");
         return(D_SEEKERR);
      }

      if ( write(fd, (char *)datablock.head, bbytesNew) != bbytesNew )
      {
         close(fd);
         releaseAllWithId("compress");
         return(D_WRITERR);
      }

/*  L_INCR ==> SEEK_CUR, SVR4.  Both are defined to 1,
    but L_INCR is not available on SVR4.		*/

      if ( (i+1) < nblocks )
      {
         if ( lseek(fd, (off_t) (incrbytes * (i+1)), SEEK_CUR) < 0 )
         {
            close(fd);
            releaseAllWithId("compress");
            return(D_SEEKERR);
         }
      }

      setbytes += bbytesNew;
   }

/***********************************
*  Truncate the data file down to  *
*  its actual size.                *
***********************************/

   if ( ftruncate(fd, (off_t) (nblocks*bbytesNew + sizeof(dfilehead))) )
   {
      close(fd);
      releaseAllWithId("compress");
      return(D_TRUNCERR);
   }

   close(fd);
   releaseAllWithId("compress");
   return(COMPLETE);
}

int D_downsizefid(int fp, int newnp, char *datapath)
{
   int			i,
			fd,
			nblocks,
			bbytesOrig,
			bbytesNew,
			nbheader,
			nbheaderbytes,
			setbytes,
			setbytesOrig,
                        tBytes,
                        skipBytes=0;
   register int         cnt;
   dfilehead		datafilehead;
   dpointers		datablock;


   if ( (fd = open(datapath, O_RDWR, 0666)) < 0 )
      return(D_OPENERR);

/***********************************
*  Read in file header and modify  *
*  for the downsized file.        *
***********************************/

   if ( read(fd, (char *)(&datafilehead), sizeof(dfilehead))
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_READERR);
   }
#ifdef LINUX
   datafilehead.nblocks = ntohl(datafilehead.nblocks);
   datafilehead.ntraces = ntohl(datafilehead.ntraces);
   datafilehead.np = ntohl(datafilehead.np);
   datafilehead.ebytes = ntohl(datafilehead.ebytes);
   datafilehead.tbytes = ntohl(datafilehead.tbytes);
   datafilehead.bbytes = ntohl(datafilehead.bbytes);
   datafilehead.vers_id = ntohs(datafilehead.vers_id);
   datafilehead.status = ntohs(datafilehead.status);
   datafilehead.nbheaders = ntohl(datafilehead.nbheaders);
#endif

   nblocks = datafilehead.nblocks;
   nbheader = (datafilehead.nbheaders & NBMASK);
   nbheaderbytes = nbheader * sizeof(dblockhead);
   bbytesOrig = datafilehead.bbytes;

   if (fp+newnp > datafilehead.np)
   {
      close(fd);
      return(COMPLETE);
   }

   skipBytes = fp * datafilehead.ebytes;
   datafilehead.np = newnp;
   tBytes = datafilehead.tbytes = datafilehead.ebytes*newnp;
   datafilehead.bbytes = datafilehead.tbytes*datafilehead.ntraces +
				nbheaderbytes;
   bbytesNew = datafilehead.bbytes;
   cnt = datafilehead.np*datafilehead.ntraces;

#ifdef LINUX
   datafilehead.nblocks = htonl(datafilehead.nblocks);
   datafilehead.ntraces = htonl(datafilehead.ntraces);
   datafilehead.np = htonl(datafilehead.np);
   datafilehead.ebytes = htonl(datafilehead.ebytes);
   datafilehead.tbytes = htonl(datafilehead.tbytes);
   datafilehead.bbytes = htonl(datafilehead.bbytes);
   datafilehead.vers_id = htons(datafilehead.vers_id);
   datafilehead.status = htons(datafilehead.status);
   datafilehead.nbheaders = htonl(datafilehead.nbheaders);
#endif
/************************************
*  Seek to start of file and write  *
*  out the new file header.         *
************************************/

   if ( lseek(fd, 0, SEEK_SET) < 0 )
   {
      close(fd);
      return(D_SEEKERR);
   }

   if ( write(fd, (char *)(&datafilehead), sizeof(dfilehead) )
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_WRITERR);
   }

/******************************
*  Allocate memory for data.  *
******************************/

   if ( (datablock.head = (dblockhead *) allocateWithId(bbytesOrig,
		"downsize")) == NULL )
   {
      close(fd);
      return(D_NOALLOC);
   }

   datablock.data = (float *) (datablock.head + nbheader);
   setbytes = sizeof(dfilehead);
   setbytesOrig = sizeof(dfilehead);


//  read original data and write out truncated data

   for (i = 0; i < nblocks; i++)
   {
      if ( read(fd, (char *)datablock.head, bbytesOrig) != bbytesOrig )
      {
         close(fd);
         releaseAllWithId("downsize");
         return(D_READERR);
      }
      setbytesOrig += bbytesOrig; 

      if ( lseek(fd, (off_t)setbytes, SEEK_SET) < 0 )
      {
         close(fd);
         releaseAllWithId("downsize");
         return(D_SEEKERR);
      }

      if (skipBytes == 0)
      {
         if ( write(fd, (char *)datablock.head, bbytesNew) != bbytesNew )
         {
            close(fd);
            releaseAllWithId("downsize");
            return(D_WRITERR);
         }
      }
      else
      {
         if (write(fd, (char *)datablock.head, nbheaderbytes) != nbheaderbytes)
         {
            close(fd);
            releaseAllWithId("downsize");
            return(D_WRITERR);
         }
         if (write(fd, (char *)datablock.head+nbheaderbytes+skipBytes, tBytes) != tBytes)
         {
            close(fd);
            releaseAllWithId("downsize");
            return(D_WRITERR);
         }
      }

      if ( (i+1) < nblocks )
      {
         if ( lseek(fd, (off_t) setbytesOrig, SEEK_SET) < 0 )
         {
            close(fd);
            releaseAllWithId("downsize");
            return(D_SEEKERR);
         }
      }

      setbytes += bbytesNew;
   }

//  Truncate the data file down to its actual size.

   if ( ftruncate(fd, (off_t) (nblocks*bbytesNew + sizeof(dfilehead))) )
   {
      close(fd);
      releaseAllWithId("downsize");
      return(D_TRUNCERR);
   }

   close(fd);
   releaseAllWithId("downsize");
   return(COMPLETE);
}

int D_zerofillfid(int newnp, char *datapath)
{
   char			tmpdatapath[MAXPATH];
   int			i,
			fd,
			np,
			tmpfd,
			nblocks,
			bbytesOrig,
			bbytesNew,
			nbheader,
			nbheaderbytes;
   register int         cnt;
   dfilehead		datafilehead;
   dpointers		datablock;
   float *ptr;


   if ( (fd = open(datapath, O_RDONLY)) < 0 )
      return(D_OPENERR);

/***********************************
*  Read in file header and modify  *
*  for the downsized file.        *
***********************************/

   if ( read(fd, (char *)(&datafilehead), sizeof(dfilehead))
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_READERR);
   }
#ifdef LINUX
   datafilehead.nblocks = ntohl(datafilehead.nblocks);
   datafilehead.ntraces = ntohl(datafilehead.ntraces);
   datafilehead.np = ntohl(datafilehead.np);
   datafilehead.ebytes = ntohl(datafilehead.ebytes);
   datafilehead.tbytes = ntohl(datafilehead.tbytes);
   datafilehead.bbytes = ntohl(datafilehead.bbytes);
   datafilehead.vers_id = ntohs(datafilehead.vers_id);
   datafilehead.status = ntohs(datafilehead.status);
   datafilehead.nbheaders = ntohl(datafilehead.nbheaders);
#endif

   nblocks = datafilehead.nblocks;
   nbheader = (datafilehead.nbheaders & NBMASK);
   nbheaderbytes = nbheader * sizeof(dblockhead);
   bbytesOrig = datafilehead.bbytes;
   np = datafilehead.np;
   if (newnp <= np)
   {
      close(fd);
      return(COMPLETE);
   }

   sprintf(tmpdatapath,"%s.tmp",datapath);
   if ( (tmpfd = open(tmpdatapath, O_WRONLY | O_CREAT, 0666)) < 0 )
   {
      close(fd);
      return(D_OPENERR);
   }
   datafilehead.np = newnp;
   datafilehead.tbytes = datafilehead.ebytes*newnp;
   datafilehead.bbytes = datafilehead.tbytes*datafilehead.ntraces +
				nbheaderbytes;
   bbytesNew = datafilehead.bbytes;

#ifdef LINUX
   datafilehead.nblocks = htonl(datafilehead.nblocks);
   datafilehead.ntraces = htonl(datafilehead.ntraces);
   datafilehead.np = htonl(datafilehead.np);
   datafilehead.ebytes = htonl(datafilehead.ebytes);
   datafilehead.tbytes = htonl(datafilehead.tbytes);
   datafilehead.bbytes = htonl(datafilehead.bbytes);
   datafilehead.vers_id = htons(datafilehead.vers_id);
   datafilehead.status = htons(datafilehead.status);
   datafilehead.nbheaders = htonl(datafilehead.nbheaders);
#endif
/************************************
*  Seek to start of file and write  *
*  out the new file header.         *
************************************/

   if ( write(tmpfd, (char *)(&datafilehead), sizeof(dfilehead) )
		!= sizeof(dfilehead) )
   {
      close(fd);
      close(tmpfd);
      return(D_WRITERR);
   }

/******************************
*  Allocate memory for data.  *
******************************/

   if ( (datablock.head = (dblockhead *) allocateWithId(bbytesNew,
		"downsize")) == NULL )
   {
      close(fd);
      close(tmpfd);
      return(D_NOALLOC);
   }

   datablock.data = (float *) (datablock.head + nbheader);

//  read original data and write out truncated data

   for (i = 0; i < nblocks; i++)
   {
      if ( read(fd, (char *)datablock.head, bbytesOrig) != bbytesOrig )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_READERR);
      }

      ptr= datablock.data+np;
      cnt = newnp - np;
      while (cnt--)
         *ptr++ = 0.0;
      if ( write(tmpfd, (char *)datablock.head, bbytesNew) != bbytesNew )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_WRITERR);
      }
   }
   close(fd);
   close(tmpfd);
   rename(tmpdatapath,datapath);
   releaseAllWithId("downsize");
   return(COMPLETE);
}

int D_leftshiftfid(int lsFID, char *datapath, int *newnp)
{
   char			tmpdatapath[MAXPATH];
   int			i,
			fd,
			np,
			tmpfd,
			nblocks,
			bbytesOrig,
			bbytesNew,
			nbheader,
			nbheaderbytes;
   int      zeroBytes,
            dataBytes;
   dfilehead		datafilehead;
   dpointers		datablock;
   float *ptr;
   float *zeros;


   if (lsFID == 0)
      return(COMPLETE);
   if ( (fd = open(datapath, O_RDONLY)) < 0 )
      return(D_OPENERR);

/***********************************
*  Read in file header and modify  *
*  for the downsized file.        *
***********************************/

   if ( read(fd, (char *)(&datafilehead), sizeof(dfilehead))
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_READERR);
   }
#ifdef LINUX
   datafilehead.nblocks = ntohl(datafilehead.nblocks);
   datafilehead.ntraces = ntohl(datafilehead.ntraces);
   datafilehead.np = ntohl(datafilehead.np);
   datafilehead.ebytes = ntohl(datafilehead.ebytes);
   datafilehead.tbytes = ntohl(datafilehead.tbytes);
   datafilehead.bbytes = ntohl(datafilehead.bbytes);
   datafilehead.vers_id = ntohs(datafilehead.vers_id);
   datafilehead.status = ntohs(datafilehead.status);
   datafilehead.nbheaders = ntohl(datafilehead.nbheaders);
#endif

   nblocks = datafilehead.nblocks;
   nbheader = (datafilehead.nbheaders & NBMASK);
   nbheaderbytes = nbheader * sizeof(dblockhead);
   bbytesOrig = datafilehead.bbytes;
   np = datafilehead.np;
   if (lsFID < 0)
   {
      dataBytes = datafilehead.np * datafilehead.ebytes * datafilehead.ntraces;
   }
   else
   {
      dataBytes = (datafilehead.np - (lsFID*2)) * datafilehead.ebytes * datafilehead.ntraces;
   }

   sprintf(tmpdatapath,"%s.tmp",datapath);
   if ( (tmpfd = open(tmpdatapath, O_WRONLY | O_CREAT, 0666)) < 0 )
   {
      close(fd);
      return(D_OPENERR);
   }
   datafilehead.np = np - (lsFID*2);
   datafilehead.tbytes = datafilehead.ebytes*datafilehead.np;
   datafilehead.bbytes = datafilehead.tbytes*datafilehead.ntraces +
				nbheaderbytes;
   bbytesNew = datafilehead.bbytes;
  *newnp = datafilehead.np;

#ifdef LINUX
   datafilehead.nblocks = htonl(datafilehead.nblocks);
   datafilehead.ntraces = htonl(datafilehead.ntraces);
   datafilehead.np = htonl(datafilehead.np);
   datafilehead.ebytes = htonl(datafilehead.ebytes);
   datafilehead.tbytes = htonl(datafilehead.tbytes);
   datafilehead.bbytes = htonl(datafilehead.bbytes);
   datafilehead.vers_id = htons(datafilehead.vers_id);
   datafilehead.status = htons(datafilehead.status);
   datafilehead.nbheaders = htonl(datafilehead.nbheaders);
#endif
/************************************
*  Seek to start of file and write  *
*  out the new file header.         *
************************************/

   if ( write(tmpfd, (char *)(&datafilehead), sizeof(dfilehead) )
		!= sizeof(dfilehead) )
   {
      close(fd);
      close(tmpfd);
      return(D_WRITERR);
   }

/******************************
*  Allocate memory for data.  *
******************************/

   if ( (datablock.head = (dblockhead *) allocateWithId(bbytesOrig,
		"downsize")) == NULL )
   {
      close(fd);
      close(tmpfd);
      return(D_NOALLOC);
   }

   datablock.data = (float *) (datablock.head + nbheader);
   if (lsFID < 0)
   {
      float *zptr;
      int numZeros = -2 * lsFID;
      zeroBytes = numZeros * sizeof(float);
      if ( (zeros = (float *) allocateWithId(zeroBytes, "downsize")) == NULL )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_NOALLOC);
      }
      zptr = zeros;
      while (numZeros--)
         *zptr++ = 0.0;
   }
   else
   {
      zeros = NULL;
      zeroBytes = 0;
   }

//  read original data and write out truncated data

   for (i = 0; i < nblocks; i++)
   {
      if ( read(fd, (char *)datablock.head, bbytesOrig) != bbytesOrig )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_READERR);
      }
      if ( write(tmpfd, (char *)datablock.head, sizeof(dblockhead)) != sizeof(dblockhead) )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_WRITERR);
      }
      if (lsFID < 0)
      {
         if ( write(tmpfd, (char *)zeros, zeroBytes) != zeroBytes )
         {
            close(fd);
            close(tmpfd);
            releaseAllWithId("downsize");
            return(D_WRITERR);
         }
         ptr= datablock.data;
      }
      else
      {
         ptr = datablock.data + (lsFID*2);
      }
      if ( write(tmpfd, (char *)ptr, dataBytes) != dataBytes )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_WRITERR);
      }
   }
   close(fd);
   close(tmpfd);
   rename(tmpdatapath,datapath);
   releaseAllWithId("downsize");
   return(COMPLETE);
}

int D_scalefid(double scaling, char *datapath)
{
   char			tmpdatapath[MAXPATH];
   int			i,
			fd,
			np,
			tmpfd,
			nblocks,
			bbytesOrig,
			nbheader;
   int cnt;
   int ebyte;
   short stat;
   dfilehead		datafilehead;
   dpointers		datablock;
   float *ptr;
   int *iptr;
   short *sptr;
   int ival;
   short sval;
   union u_tag {
      float fval;
      int   ival;
   } uval;
   int scale;
   double inScale;


   if ( (fd = open(datapath, O_RDONLY)) < 0 )
      return(D_OPENERR);

/***********************************
*  Read in file header and modify  *
*  for the downsized file.        *
***********************************/

   if ( read(fd, (char *)(&datafilehead), sizeof(dfilehead))
		!= sizeof(dfilehead) )
   {
      close(fd);
      return(D_READERR);
   }

   nblocks = ntohl(datafilehead.nblocks);
   bbytesOrig = ntohl(datafilehead.bbytes);
   np = ntohl(datafilehead.np);
   nbheader = ntohl(datafilehead.nbheaders);
   nbheader = nbheader & NBMASK;
   ebyte = ntohl(datafilehead.ebytes);
   stat = ntohs(datafilehead.status);

   sprintf(tmpdatapath,"%s.tmp",datapath);
   if ( (tmpfd = open(tmpdatapath, O_WRONLY | O_CREAT, 0666)) < 0 )
   {
      close(fd);
      return(D_OPENERR);
   }

/************************************
*  Seek to start of file and write  *
*  out the new file header.         *
************************************/

   if ( write(tmpfd, (char *)(&datafilehead), sizeof(dfilehead) )
		!= sizeof(dfilehead) )
   {
      close(fd);
      close(tmpfd);
      return(D_WRITERR);
   }

/******************************
*  Allocate memory for data.  *
******************************/

   if ( (datablock.head = (dblockhead *) allocateWithId(bbytesOrig,
		"downsize")) == NULL )
   {
      close(fd);
      close(tmpfd);
      return(D_NOALLOC);
   }

   datablock.data = (float *) (datablock.head + nbheader);

//  read original data and write out scaled data

   inScale = scaling;
   scale = 0;
   while (inScale >= 2.0)
   {
      scale++;
      inScale /= 2.0;
   }
   while (inScale <= 0.5)
   {
      scale--;
      inScale *= 2.0;
   }
   for (i = 0; i < nblocks; i++)
   {
      if ( read(fd, (char *)datablock.head, bbytesOrig) != bbytesOrig )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_READERR);
      }

      if (ebyte == 2)
      {

         sptr = (short *) datablock.data;
         if (inScale != 1.0)
         {
            for (cnt=0; cnt<np; cnt++)
            {
               sval = *sptr;
               sval = ntohs(sval);
               sval = (short) ((double)sval * inScale);
               sval = htons(sval);
               *sptr = sval;
               sptr++;
            }
         }
         if (scale)
         {
            sval = ntohs(datablock.head->scale);
            sval += scale;
            datablock.head->scale = ntohs(sval);
         }
      }
      else if ((stat & S_FLOAT) == 0)
      {
         iptr = (int *) datablock.data;;
         if (inScale != 1.0)
         {
            for (cnt=0; cnt<np; cnt++)
            {
               ival = *iptr;
               ival = ntohl(ival);
               ival = (int) ((double)ival * inScale);
               ival = htonl(ival);
               *iptr = ival;
               iptr++;
            }
         }
         if (scale)
         {
            sval = ntohs(datablock.head->scale);
            sval += scale;
            datablock.head->scale = ntohs(sval);
         }
         // Also handle offsets from noise check
         uval.fval = datablock.head->lvl;
         uval.ival = ntohl(uval.ival);
         uval.fval = (float) (uval.fval * scaling);
         uval.ival = htonl(uval.ival);
         datablock.head->lvl = uval.fval;
         uval.fval = datablock.head->tlt;
         uval.ival = ntohl(uval.ival);
         uval.fval = (float) (uval.fval * scaling);
         uval.ival = htonl(uval.ival);
         datablock.head->tlt = uval.fval;
      }
      else // floating point data
      {
         ptr = datablock.data;
         if (inScale != 1.0)
         {
            for (cnt=0; cnt<np; cnt++)
            {
               uval.fval = *ptr;
               uval.ival = ntohl(uval.ival);
               uval.fval = (float) (uval.fval * inScale);
               uval.ival = htonl(uval.ival);
               *ptr = uval.fval;
               ptr++;
            }
         }
         if (scale)
         {
            sval = ntohs(datablock.head->scale);
            sval += scale;
            datablock.head->scale = ntohs(sval);
         }
      }
      if ( write(tmpfd, (char *)datablock.head, bbytesOrig) != bbytesOrig )
      {
         close(fd);
         close(tmpfd);
         releaseAllWithId("downsize");
         return(D_WRITERR);
      }
   }
   close(fd);
   close(tmpfd);
   rename(tmpdatapath,datapath);
   releaseAllWithId("downsize");
   return(COMPLETE);
}

