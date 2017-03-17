/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vnmrsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "data.h"
#include "process.h"
#include "group.h"
#include "buttons.h"
#include "pvars.h"
#include "wjunk.h"
#include "tools.h"
#include "fdatap.h"
extern float getParm( float fdata[], int parmCode, int origDimCode );

#define BUFWORDS 65536
extern int Bnmr;
extern int start_from_ft;
extern int bufferscale;
static float *data;
static int jeolFlag;


/* check to make sure value is a power of two */
static int checkFnSize( int fnSize, const char *msg )
{
   int testFn;
   if (fnSize < 16) /* value is too small */
   {
      if (msg)
         Werrprintf("%s data size (%d) is too small",msg,fnSize);
      return(-1);
   }
   else if (fnSize > (2 << 29)) /* value is too large */
   {
      if (msg)
         Werrprintf("%s data size (%d) is too large",msg,fnSize);
      return(-2);
   }
   if (jeolFlag)
      return(0);

   testFn = 16;
   while (testFn < fnSize)
      testFn *= 2;
   if (msg && (testFn != fnSize))
      Werrprintf("%s data size (%d) is not a power of 2",msg,fnSize);
   return( (testFn == fnSize) ? 0 : -3);
   
}

static int getBuffer2D(int index, int stat)
{
  dpointers block;
  int r;

  if ( (r=D_allocbuf(D_DATAFILE,index,&block)) )
    { D_error(r); ABORT;
    }

  block.head->scale  = 0;
  block.head->status = stat;
  block.head->mode   = 0;
  block.head->rpval  = 0;
  block.head->lpval  = 0;
  block.head->lvl    = 0;
  block.head->tlt    = 0;
  block.head->ctcount= 1;
  data = block.data;
  RETURN;
}

/**********************/
static int setDataFile2D3D(int fn0, int f2realOnly, int fn1, int f1realOnly,
              int *blockStat, int *blocks2D, int *traces2D, int *np2D,
              int flag3d, int flag4d)
/**********************/
{ char path[MAXPATH];
  dfilehead datahead;
  double rfn;
  int r;
  int stat;
  int nblocks;
  int sperblock0;
  char *fnx;
  char *fny;
  int  stat2d3d;

/*
  fprintf(stderr,"setDataFile2D3D fn0= %d f2realOnly= %d fn1= %d f1realOnly= %d\n",
                  fn0, f2realOnly, fn1, f1realOnly);
 */
  
  fnx = "fn";
  fny = "fn1";
  stat2d3d = (S_NP|S_NI);
  if (flag4d)
  {
     char plane[STR64];

     if ( (r=P_getstring(CURRENT,"plane",plane,1,STR64-1)) )
     { P_err(r,"plane",":");
       ABORT;
     }
     if ( ! strcmp(plane,"f1f4") )
     {
        fnx = "fn";
        fny = "fn1";
        stat2d3d = ND_NP|ND_NI;
     }
     if ( ! strcmp(plane,"f1f3") )
     {
        fnx = "fn3";
        fny = "fn1";
        stat2d3d = ND_NI3|ND_NI;
     }
     else if ( ! strcmp(plane,"f1f2") )
     {
        fnx = "fn2";
        fny = "fn1";
        stat2d3d = ND_NI2|ND_NI;
     }
     else if ( ! strcmp(plane,"f2f4") )
     {
        fnx = "fn";
        fny = "fn2";
        stat2d3d = ND_NP|ND_NI2;
     }
     else if ( ! strcmp(plane,"f2f3") )
     {
        fnx = "fn3";
        fny = "fn2";
        stat2d3d = ND_NI3|ND_NI2;
     }
     else if ( ! strcmp(plane,"f3f4") )
     {
        fnx = "fn";
        fny = "fn3";
        stat2d3d = ND_NP|ND_NI3;
     }
  }
  else if (flag3d)
  {
     char plane[STR64];

     if ( (r=P_getstring(CURRENT,"plane",plane,1,STR64-1)) )
     { P_err(r,"plane",":");
       ABORT;
     }
     if ( ! strcmp(plane,"f1f3") )
     {
        fnx = "fn";
        fny = "fn1";
        stat2d3d = (S_3D|S_NP|S_NI);
     }
     else if ( ! strcmp(plane,"f1f2") )
     {
        fnx = "fn2";
        fny = "fn1";
        stat2d3d = (S_3D|S_NI2|S_NI);
     }
     else if ( ! strcmp(plane,"f2f3") )
     {
        fnx = "fn";
        fny = "fn2";
        stat2d3d = (S_3D|S_NP|S_NI2);
     }
  }
  if ( (r=P_getreal(CURRENT,fnx,&rfn,1)) )
    { P_err(r,fnx,":");
      ABORT;
    }
  if ( f2realOnly && ((int) rfn != (fn0*2)))
  {
     Werrprintf("%s mismatch for real data (%d)",fnx, 2*fn0);
     ABORT;
  }
  else if ( ! f2realOnly && ((int) rfn != fn0) )
  {
     Werrprintf("%s mismatch for complex data (%d)", fnx, fn0);
     ABORT;
  }
  P_setreal(PROCESSED,fnx,rfn,0);
  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);
  fn0 = (int) rfn;
  if ( (r=P_getreal(CURRENT,fny,&rfn,1)) )
    { P_err(r,fny,":");
      ABORT;
    }
  if ( f1realOnly && ((int) rfn != (fn1*2)))
  {
     Werrprintf("%s mismatch for real data (%d)", fny, 2*fn1);
     ABORT;
  }
  else if ( ! f1realOnly && ((int) rfn != fn1) )
  {
     Werrprintf("%s mismatch for complex data (%d)", fny, fn1);
     ABORT;
  }

  if ( (r = D_getfilepath(D_DATAFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  sperblock0 = (BUFWORDS/fn0) * bufferscale;
  if (sperblock0 > fn1/2)
     sperblock0 = fn1/2;
  if (sperblock0 < 4)
     sperblock0 = 4;
  nblocks = fn1/(2 * sperblock0);
  if ( ! f1realOnly && ! f2realOnly)
  {
     sperblock0 /= 2;
     nblocks *= 2;
  }
  if (nblocks == 0)
     nblocks = 1;
  if (nblocks == 2)
  {
     nblocks = 1;
     sperblock0 *= 2;
  }

  datahead.nblocks = nblocks;
  datahead.ntraces = fn0/(2*nblocks);
  datahead.np      = fn1;
  datahead.vers_id = VERSION;
  datahead.vers_id += DATA_FILE;
  datahead.nbheaders = 1;
  datahead.ebytes  = 4;
  datahead.tbytes  = datahead.ebytes * datahead.np;
  datahead.bbytes  = datahead.tbytes * datahead.ntraces +
                       sizeof(dblockhead);
  datahead.status  = S_SECND|S_TRANSF;
  if ( ! f1realOnly && ! f1realOnly)
  {
     datahead.bbytes += sizeof(dblockhead);
     datahead.nbheaders = 2;
     datahead.status  |= S_HYPERCOMPLEX;
  }

  if (f1realOnly)
    stat  = (S_DATA|S_SPEC|S_FLOAT);
  else
    stat  = (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX|NP_CMPLX);
  if (flag4d)
  {
     datahead.nbheaders |= stat2d3d;
     datahead.status  |= stat;
  }
  else
  {
     datahead.status  |= (stat|stat2d3d);
  }

  if ( (r=D_newhead(D_DATAFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  *blockStat = stat;
  *blocks2D = datahead.nblocks;
  *traces2D = datahead.ntraces;
  *np2D = datahead.np;

  if ( (r = D_getfilepath(D_PHASFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  datahead.vers_id += PHAS_FILE - DATA_FILE;
  if ( (r=D_newhead(D_PHASFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  RETURN;
}

/**********************/
static int setFidFile1D(int npval)
/**********************/
{ char path[MAXPATH];
  dfilehead datahead;
  dpointers block;
  int r;

  P_setreal(CURRENT,"arraydim",1.0,0);
  P_setreal(PROCESSED,"arraydim",1.0,0);
  P_setreal(CURRENT,"np",(double) npval,0);
  P_setreal(PROCESSED,"np",(double) npval,0);

  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);
  D_trash(D_USERFILE);

  if ( (r = D_getfilepath(D_USERFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  datahead.nblocks = 1;
  datahead.ntraces = 1;
  datahead.np      = npval;
  datahead.vers_id = VERSION;
  datahead.vers_id += FID_FILE;
  datahead.vers_id |= S_JEOL;
  datahead.vers_id |= S_MAKEFID;
  datahead.nbheaders = 1;
  datahead.ebytes  = 4;
  datahead.tbytes  = datahead.ebytes * datahead.np;
  datahead.bbytes  = datahead.tbytes * datahead.ntraces +
                       sizeof(dblockhead);
  datahead.status  = S_DATA|S_FLOAT|S_COMPLEX;

  if ( (r=D_newhead(D_USERFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  if ( (r=D_allocbuf(D_USERFILE,0,&block)) )
    { D_error(r); ABORT;
    }

  block.head->index  = 1;
  block.head->scale  = 0;
  block.head->status = datahead.status;
  block.head->mode   = 0;
  block.head->rpval  = 0;
  block.head->lpval  = 0;
  block.head->lvl    = 0;
  block.head->tlt    = 0;
  block.head->ctcount= 1;
  data = block.data;

  RETURN;
}

/**********************/
static int getFidFile1D(int npval, int elem)
/**********************/
{ char path[MAXPATH];
  dfilehead datahead;
  dpointers block;
  double rnp;
  int e;

  if ( (e=P_getreal(CURRENT,"np",&rnp,1)) )
  {
      P_err(e,"np",":");
      ABORT;
  }
  if ( (int) rnp != npval)
  {
     Werrprintf("np mismatch currrent np is %d. New element np is %d", (int) rnp, npval);
     ABORT;
  }
  D_trash(D_PHASFILE);

  if ( (e = D_gethead(D_USERFILE, &datahead)) )
  {
     if ( (e = D_getfilepath(D_USERFILE, path, curexpdir)) )
     {
        D_error(e);
        ABORT;
     }

     e = D_open(D_USERFILE, path, &datahead);     /* open the file */
     if (e)
     {
        D_error(e);
        ABORT;
      }
  }

  if (npval != datahead.np)
  {
     if (elem == 1)
        return(setFidFile1D(npval));
     Werrprintf("data block np mismatch (%d)", npval);
     ABORT;
  }
  if (elem > datahead.nblocks+1)
  {
     Werrprintf("Cannot add element %d to data block that only has %d elements",
                 elem, datahead.nblocks);
     ABORT;
  }
  if (elem > datahead.nblocks)
  {
     datahead.nblocks = elem;
      D_updatehead(D_USERFILE, &datahead);
     if ( (e=D_allocbuf(D_USERFILE,elem-1,&block)) )
     { D_error(e); ABORT;
     }
  block.head->index  = elem;
  block.head->scale  = 0;
  block.head->status = datahead.status;
  block.head->mode   = 0;
  block.head->rpval  = 0;
  block.head->lpval  = 0;
  block.head->lvl    = 0;
  block.head->tlt    = 0;
  block.head->ctcount= 1;
  }
  else
  {
     D_getbuf(D_USERFILE, datahead.nblocks, elem-1, &block);
  }
  data = block.data;

  RETURN;
}

/**********************/
static int setDataFile1D(int fnval, int realOnly)
/**********************/
{ char path[MAXPATH];
  dfilehead datahead;
  dpointers block;
  double rfn;
  int r;
  int stat;

  if ( (r=P_getreal(CURRENT,"fn",&rfn,1)) )
    { P_err(r,"fn",":");
      ABORT;
    }
  if ( realOnly && ((int) rfn != (fnval*2)))
  {
     Werrprintf("fn mismatch for real data (%d)", 2*fnval);
     ABORT;
  }
  else if ( ! realOnly && ((int) rfn != fnval) )
  {
     Werrprintf("fn mismatch for complex data (%d)", fnval);
     ABORT;
  }
  P_setreal(PROCESSED,"fn",rfn,0);
  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);

  if ( (r = D_getfilepath(D_DATAFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  datahead.nblocks = 1;
  datahead.ntraces = 1;
  datahead.np      = fnval;
  datahead.vers_id = VERSION;
  datahead.vers_id += DATA_FILE;
  datahead.nbheaders = 1;
  datahead.ebytes  = 4;
  datahead.tbytes  = datahead.ebytes * datahead.np;
  datahead.bbytes  = datahead.tbytes * datahead.ntraces +
                       sizeof(dblockhead);
  if (realOnly)
    stat  = (S_DATA|S_SPEC|S_FLOAT);
  else
    stat  = (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX|NP_CMPLX);
  datahead.status  = (stat|S_NP);

  if ( (r=D_newhead(D_DATAFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  if ( (r=D_allocbuf(D_DATAFILE,0,&block)) )
    { D_error(r); ABORT;
    }

  block.head->scale  = 0;
  block.head->status = stat;
  block.head->mode   = 0;
  block.head->rpval  = 0;
  block.head->lpval  = 0;
  block.head->lvl    = 0;
  block.head->tlt    = 0;
  block.head->ctcount= 1;
  data = block.data;

  if ( (r = D_getfilepath(D_PHASFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  datahead.vers_id += PHAS_FILE - DATA_FILE;
  if ( (r=D_newhead(D_PHASFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  RETURN;
}

/**********************/
static int getDataFile1D(int fnval, int realOnly, int elem)
/**********************/
{ char path[MAXPATH];
  dfilehead datahead;
  dpointers block;
  double rfn;
  int e;

  if ( (e=P_getreal(CURRENT,"fn",&rfn,1)) )
    { P_err(e,"fn",":");
      ABORT;
    }
  if ( realOnly && ((int) rfn != (fnval*2)))
  {
     Werrprintf("fn mismatch for real data (%d)", 2*fnval);
     ABORT;
  }
  else if ( ! realOnly && ((int) rfn != fnval) )
  {
     Werrprintf("fn mismatch for complex data (%d)", fnval);
     ABORT;
  }
  P_setreal(PROCESSED,"fn",rfn,0);
  D_trash(D_PHASFILE);

  if ( (e = D_gethead(D_DATAFILE, &datahead)) )
  {
     if ( (e = D_getfilepath(D_DATAFILE, path, curexpdir)) )
     {
        D_error(e);
        ABORT;
     }

     e = D_open(D_DATAFILE, path, &datahead);     /* open the file */
     if (e)
     {
        D_error(e);
        ABORT;
      }
  }

  if ( (realOnly && (datahead.status & S_COMPLEX) == S_COMPLEX) ||
       ( ! realOnly && ((datahead.status & S_COMPLEX) != S_COMPLEX) ) )
  {
     if (elem == 1)
        return(setDataFile1D(fnval,realOnly));
     Werrprintf("data block real - complex mismatch");
     ABORT;
  }
  if (fnval != datahead.np)
  {
     if (elem == 1)
        return(setDataFile1D(fnval,realOnly));
     Werrprintf("data block fn mismatch for real data (%d)", fnval);
     ABORT;
  }
  if (elem > datahead.nblocks+1)
  {
     Werrprintf("Cannot add element %d to data block that only has %d elements",
                 elem, datahead.nblocks);
     ABORT;
  }
  if (elem > datahead.nblocks)
  {
     datahead.nblocks = elem;
      D_updatehead(D_DATAFILE, &datahead);
     if ( (e=D_allocbuf(D_DATAFILE,elem-1,&block)) )
     { D_error(e); ABORT;
     }
  block.head->scale  = 0;
  block.head->status = datahead.status;
  block.head->status &= (~S_NP);
  block.head->mode   = 0;
  block.head->rpval  = 0;
  block.head->lpval  = 0;
  block.head->lvl    = 0;
  block.head->tlt    = 0;
  block.head->ctcount= 1;
  }
  else
  {
     D_getbuf(D_DATAFILE, datahead.nblocks, elem-1, &block);
  }
  data = block.data;

  if ( (e = D_getfilepath(D_PHASFILE, path, curexpdir)) )
  {
     D_error(e);
     ABORT;
  }

  datahead.vers_id += PHAS_FILE - DATA_FILE;
  if ( (e=D_newhead(D_PHASFILE,path,&datahead)) )
    { D_error(e); ABORT;
    }
  RETURN;
}

/****************************/
int pipeRead(int argc, char *argv[], int retc, char *retv[])
/****************************/
{ 
   char path[MAXPATH];
   int elem;
   float fdata[FDATASIZE];
   int fd;
   int dimCount;
   int num;
   int xSize, ySize;
   int xFtSize, xCenter;
   int xDomain, yDomain, zDomain, aDomain;
   int xMode, yMode;
   int blockStat;
   int blocks2D, traces2D, np2D;
   int block2D;
   int totSize;
   int zeroTraces;
   int firstTrace;
   int r;
   float *ptr;
   float *start;
   float *dptr;
   float multRe, multIm;

   if (argc<2)
   {
      Werrprintf("usage - %s('filename'<,index>)",argv[0]);
      ABORT;
   }
   jeolFlag = (strcmp(argv[0],"jread")) ? 0 : 1;
   Wturnoff_buttons();
   D_allrelease();
   if (argv[1][0] == '/')
      strcpy(path,argv[1]);
   else
      sprintf(path,"%s/%s",curexpdir,argv[1]);
   elem = 1;
   if (jeolFlag)
   {
      multRe = multIm = 100.0;
   }
   else
   {
      multRe = multIm = FTNORM;
   }
   if (argc >= 3)
   {
      if (isReal(argv[2]))
      {
         elem = atoi(argv[2]);
         if (elem < 1)
            elem = 1;
      }
      else if ( ! strcmp(argv[2],"rev") )
      {
         multIm = -multRe;
      }
      if (argc >= 4)
      {
         if (isReal(argv[3]))
         {
            elem = atoi(argv[3]);
            if (elem < 1)
               elem = 1;
         }
         else if ( ! strcmp(argv[3],"rev") )
         {
            multIm = -multRe;
         }
      }
   }

   if ( access(path,R_OK) )
   {
      Werrprintf("%s: cannot access %s",argv[0],path);
      ABORT;
   }
   if ( (fd = open(path,O_RDONLY)) < 0)
   {
      Werrprintf("%s: unable to open %s",argv[0],path);
      ABORT;
   }

   if ( read(fd, fdata, sizeof(float)*FDATASIZE) != sizeof(float)*FDATASIZE)
   {
      Werrprintf("%s: unable to read %s",argv[0],path);
      close(fd);
      ABORT;
   }

   /* 1D Data */
   if ( (dimCount = (int) getParm( fdata, FDDIMCOUNT, 0 )) == 1)
   {
      int realOnly;

      realOnly = getParm( fdata, FDQUADFLAG, 0 );
      xSize = (int) getParm( fdata, NDSIZE, CUR_XDIM );
      if ( (r = checkFnSize(xSize, "1D")) )
      {
         close(fd);
         ABORT;
      }
      if ( ! realOnly )  /* Complex counts complex pairs */
        xSize *= 2;
      if (jeolFlag)
      {
         if ( (elem == 1) && setFidFile1D(xSize))
         {
            close(fd);
            ABORT;
         }
         if ( (elem != 1)  && getFidFile1D(xSize,elem))
         {
            close(fd);
            ABORT;
         }
      }
      else
      {
         if ( (argc != 3) && setDataFile1D(xSize,realOnly))
         {
            close(fd);
            ABORT;
         }
         if ( (argc == 3)  && getDataFile1D(xSize,realOnly,elem))
         {
            close(fd);
            ABORT;
         }
      }
      start = (float *) mmap(0,sizeof(float) * (xSize+FDATASIZE), PROT_READ,
                             MAP_PRIVATE, fd, 0);
      ptr = start + FDATASIZE;
      dptr = data;
      if (realOnly)
      {
         num = xSize;
         while ( num-- )
         {
            *dptr++ = *ptr++ * multRe;
         }
      }
      else
      {
         num = xSize / 2;
         while ( num-- )
         {
            *dptr++ = *ptr * multRe;
            *dptr++ = *(ptr+xSize/2) * multIm;
             ptr++;
         }
      }
      munmap(start, sizeof(float) * (xSize+FDATASIZE) );
      close(fd);

      if (jeolFlag)
      {
         if ( (r=D_markupdated(D_USERFILE,elem-1)) )
         { D_error(r);
           ABORT;
         }
         if ( (r=D_flush(D_USERFILE)) )
         { D_error(r);
           ABORT;
         }
         if ( (r=D_release(D_USERFILE,elem-1)) )
         { D_error(r);
           ABORT;
         }
         D_close(D_USERFILE);
         Wsetgraphicsdisplay("");		/* activate the ds program */
      }
      else
      {
         if ( (r=D_markupdated(D_DATAFILE,elem-1)) )
         { D_error(r);
           ABORT;
         }
         if ( (r=D_release(D_DATAFILE,elem-1)) )
         { D_error(r);
           ABORT;
         }
         if (!Bnmr)
         {
            releasevarlist();
            appendvarlist("cr");
            Wsetgraphicsdisplay("ds");		/* activate the ds program */
            start_from_ft = 1;
         }
      }
      RETURN;
   }

   /* 2D data */

   xDomain = (int) getParm( fdata, NDFTFLAG, CUR_XDIM );
   yDomain = (int) getParm( fdata, NDFTFLAG, CUR_YDIM );
   zDomain = (int) getParm( fdata, NDFTFLAG, CUR_ZDIM );
   aDomain = (int) getParm( fdata, NDFTFLAG, CUR_ADIM );
#ifdef XXX
   /* For 4D testing only */
   if ( strstr(path,".ft4") )
   {
      aDomain=1;
   }
#endif
   /* zDomain is the 3D flag; aDomain is 4D flag */
   if ( (dimCount < 1) || (xDomain + yDomain != 2) )
   {
      Winfoprintf("%s: file %s not 1D nor 2D data",argv[0], path);
      close(fd);
      RETURN;
   }
   xMode = (int) getParm( fdata, NDQUADFLAG, CUR_XDIM );
   yMode = (int) getParm( fdata, NDQUADFLAG, CUR_YDIM );
   xSize = (int) getParm( fdata, NDSIZE, CUR_XDIM );
   ySize = (int) getParm( fdata, NDSIZE, CUR_YDIM );
   xFtSize = (int) getParm( fdata, NDFTSIZE, CUR_XDIM );
   xCenter = (int) getParm( fdata, NDCENTER, CUR_XDIM );
   zeroTraces = 0;
   if (xSize != xFtSize)
   {
      zeroTraces = xFtSize/2 - xCenter + 1;
   }
   if ( (r = checkFnSize(xFtSize, "2D x")) )
   {
      close(fd);
      ABORT;
   }
   if ( (r = checkFnSize(ySize, "2D y")) )
   {
      close(fd);
      ABORT;
   }

   blocks2D = 0;
   totSize = 0;
   if (xMode && yMode) /* Real Real */
   {
      if ( setDataFile2D3D(xFtSize, xMode, ySize, yMode,
                        &blockStat, &blocks2D, &traces2D, &np2D, zDomain, aDomain) )
      {
         close(fd);
         ABORT;
      }
/*
      Winfoprintf("%s: Real Real 2D data from %s",argv[0], path);
      Winfoprintf("%s: blocks= %d traces= %d np= %d",argv[0], blocks2D, traces2D, np2D);
 */
      totSize = traces2D * np2D;
      totSize = xSize * ySize;
   }
   else
   {
      Werrprintf("%s: Only Real Real 2D data is currently handled",argv[0]);
      close(fd);
      ABORT;
   }
   start = (float *) mmap(0,sizeof(float) * (totSize+FDATASIZE), PROT_READ,
                             MAP_PRIVATE, fd, 0);
   ptr = start + FDATASIZE;
   close(fd);
   firstTrace = 0;
   for (block2D=0; block2D < blocks2D; block2D++)
   {
      int tracesPerBlk = traces2D / blocks2D;
      int trace = 0;
      int tracesDone = 0;
      int pt;

      if (getBuffer2D(block2D, blockStat))
      {
         munmap(start, sizeof(float) * (totSize+FDATASIZE) );
         ABORT;
      }
      dptr = data;
      if (zeroTraces)
      {
         tracesDone = (zeroTraces < tracesPerBlk) ? zeroTraces : tracesPerBlk;
         for (trace = 0; trace < tracesDone; trace++)
         {
            for (pt=0; pt < np2D; pt++)
              *dptr++ = 0.0;
         }
         zeroTraces -= tracesDone;
      }
      if ( !zeroTraces && (tracesDone < tracesPerBlk) )
      {
         while ( (firstTrace < xSize) && (tracesDone < tracesPerBlk) )
         {
            int toffset = firstTrace + (tracesPerBlk * block2D);
            for (pt=0; pt < np2D; pt++)
              *dptr++ = *(ptr + toffset + pt*xSize) * multRe; 
            firstTrace++;
            tracesDone++;
         }
         while (tracesDone < tracesPerBlk)
         {
            for (pt=0; pt < np2D; pt++)
               *dptr++ = 0.0;
            tracesDone++;
         }
      }
      if ( (r=D_markupdated(D_DATAFILE,block2D)) )
      {
         D_error(r);
         munmap(start, sizeof(float) * (totSize+FDATASIZE) );
         ABORT;
      }
      if ( (r=D_release(D_DATAFILE,block2D)) )
      {
         D_error(r);
         munmap(start, sizeof(float) * (totSize+FDATASIZE) );
         ABORT;
      }
   }
   munmap(start, sizeof(float) * (totSize+FDATASIZE) );

   RETURN;
}
