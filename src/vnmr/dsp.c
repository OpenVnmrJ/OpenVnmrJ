/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*************************************************/
/*						 */
/* dsp - calculate a spectrum from a stored list */
/*       of lines. Lorenzian and Gaussian line   */
/*       shapes and combinations between the two */
/*       are supported.				 */
/*       The format of the input file is as      */
/*       follows:				 */
/*	   frequency,intensity,width,%gaussian	 */
/*       Each signal line starts on a new line	 */
/*         frequency: in Hz, required		 */
/*	   intensity: default=1			 */
/*	   width:     default=1			 */
/*         gaussian fraction: default=0,	 */
/*              (100% Lorenzian)		 */
/*						 */
/* fitspec - spectrum deconvolution		 */
/*						 */
/* spins - spin simulation			 */
/*                                               */
/* spa(transition index, experimental line index)*/ 
/* 	  calculated transition assignment 	 */
/* dlalong - places a table of assign transitions*/
/*	  in spins.la of current experiment.     */
/*************************************************/

#include "vnmrsys.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "data.h"
#include "group.h"
#include "init2d.h"
#include "variables.h"
#include "tools.h"
#include "allocate.h"
#include "pvars.h"
#include "sky.h"
#include "wjunk.h"

#define MAXLINES 2048
#define MAXSPINS 8
#define ITERATION 1

#define SETVAL 1
#define SETARR 2
#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

#define MAX2DFTSIZE     16384           /* 2D (max FT number)/bufferscale  */
#define BUFWORDS        65536           /* words/bufferscale in buffer     */

extern int      bufferscale;            /* scaling factor for Vnmr buffers */

static float frequency[MAXLINES];	/* list of frequencies */
static float intensity[MAXLINES];	/* list of intensities */
static float width[MAXLINES];		/* list of line widths */
static float gaussian[MAXLINES];	/* %gaussian */
static int nlines;
static float *data;
static char do_freq[MAXLINES],do_int[MAXLINES],
        do_width[MAXLINES],do_gauss[MAXLINES];

static float intensmax = 0.0;
extern char *Wgettextdisplay(char *buf, int max1);
extern void  Wturnoff_buttons();
extern int P_deleteGroupVar(int tree, int group);
extern void disp_specIndex(int i);
extern int datapoint(double freq, double sw, int fn);
extern int Rflag;
extern int start_from_ft;

static char spins_parlist[256] = "";
static char spins_dmenu_par[4] = "";
static char spins_jmenu_par[4] = "";

#ifdef CLOCKTIME
extern int fitspec_timer_no, spins_timer_no;
#endif 

static int spins_makepar(char *varname, double value, int pbits, int type);
static int spins_param(char *choice, char *value, int retc, char *retv[]);
int getsize(int tree, char *name, int *Size);
int rundla(char *tdisplay);

/****************************/
static int readinput(char *filename)
/****************************/
{
  FILE *inputfile;
  int i,ch;
  double slw;
  if (P_getreal(GLOBAL,"slw",&slw,1))
    slw = 0.5;
  if ( (inputfile=fopen(filename,"r")) )
    { i = 0;
      do
        { if (fscanf(inputfile,"%f",&frequency[i])!=1)
	    { ch = fgetc(inputfile);
              while (ch==' ') ch = fgetc(inputfile);
              if (ch==EOF)
                { nlines = i;
                  if (nlines<1)
                    { Werrprintf("No lines in input file %s",filename);
                      fclose(inputfile);
                      ABORT;
                    }
                  fclose(inputfile);
                  RETURN;
                }
	      else
                { Werrprintf("Syntax error in input file %s",filename);
	          fclose(inputfile);
		  ABORT;
                }
            }
          ch = fgetc(inputfile);
          while (ch==' ') ch = fgetc(inputfile);
	  if (ch=='*')
	    { do_freq[i] = '*';
	      ch = fgetc(inputfile);
              while (ch==' ') ch = fgetc(inputfile);
	    }
          else
	    do_freq[i] = ' ';
	  if (ch==',')
	    { if (fscanf(inputfile,"%f",&intensity[i])!=1)
                { Werrprintf("Syntax error in input file %s",filename);
	          fclose(inputfile);
		  ABORT;
                }
	      if (intensity[i]>intensmax) intensmax = intensity[i];
              ch = fgetc(inputfile);
              while (ch==' ') ch = fgetc(inputfile);
	      if (ch=='*')
	        { do_int[i] = '*';
		  ch = fgetc(inputfile);
                  while (ch==' ') ch = fgetc(inputfile);
		}
              else
		do_int[i] = ' ';
              if (ch==';')
                /* skip over line assignments */
                while ((ch!=EOF)&&(ch!='\n'))
                  ch = fgetc(inputfile);
	      if (ch==',')
	        { if (fscanf(inputfile,"%f",&width[i])!=1)
                    { Werrprintf("Syntax error in input file %s",filename);
		      fclose(inputfile);
		      ABORT;
                    }
                  ch = fgetc(inputfile);
                  while (ch==' ') ch = fgetc(inputfile);
		  if (ch=='*')
		    { do_width[i] = '*';
		      ch = fgetc(inputfile);
                      while (ch==' ') ch = fgetc(inputfile);
		    }
                  else
		    do_width[i] = ' ';
		  if (ch==',')
		    { if (fscanf(inputfile,"%f",&gaussian[i])!=1)
                        {Werrprintf("Syntax error in input file %s",filename);
		          fclose(inputfile);
		          ABORT;
                        }
                      ch = fgetc(inputfile);
                      while (ch==' ') ch = fgetc(inputfile);
		      if (ch=='*')
		        { do_gauss[i] = '*';
		          ch = fgetc(inputfile);
                          while (ch==' ') ch = fgetc(inputfile);
		        }
                      else
		        do_gauss[i] = ' ';
		    }
                  else
		    { gaussian[i] = 0.0;
		      do_gauss[i] = '*';
                    }
		}
              else
		{ width[i]    = slw;
		  do_width[i] = ' ';
		  gaussian[i] = 0.0;
		  do_gauss[i] = '*';
		}
	    }
          else
	    { intensity[i] = 1.0;
	      do_int[i]    = ' ';
	      width[i]     = slw;
	      do_width[i]  = ' ';
	      gaussian[i]  = 0.0;
	      do_gauss[i]  = '*';
            }
          if (ch!='\n')
            { Werrprintf("Syntax error in input file %s",filename);
	      fclose(inputfile);
	      ABORT;
	    }
          i++;
          if (i>=MAXLINES)
            { Werrprintf("Too many lines in input file %s",filename);
              fclose(inputfile);
              ABORT;
            }
        }
      while (1);
    }
  else
    { Werrprintf("Cannot open input file %s",filename);
      ABORT;
    }
}

/**********************/
static int set2Ddatafile(int fn0, int fn1Val)
/**********************/
{ char path[MAXPATHL];
  dfilehead datahead;
  dpointers block;
  double rfn;
  int r;
  int sperblock0;
//  int sperblock1;
  int nblks;

  if (fn0 > (MAX2DFTSIZE*bufferscale))
  {
         Werrprintf("fn too large,  max = %d", bufferscale*MAX2DFTSIZE);
         ABORT;
  }
  else if (fn1Val > (MAX2DFTSIZE*bufferscale))
  {
         Werrprintf("fn1 too large,  max = %d", bufferscale*MAX2DFTSIZE);
         ABORT;
  }

  if ( (r=P_getreal(CURRENT,"fn",&rfn,1)) )
    { P_err(r,"fn",":");
      ABORT;
    }
  P_setreal(PROCESSED,"fn",rfn,0);
  if ( (r=P_getreal(CURRENT,"fn1",&rfn,1)) )
    { P_err(r,"fn1",":");
      ABORT;
    }
  P_setreal(PROCESSED,"fn1",rfn,0);
  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);

  if ( (r = D_getfilepath(D_DATAFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  sperblock0 = (BUFWORDS/fn0) * bufferscale;
      if ( sperblock0 > (fn1Val/2) )
         sperblock0 = fn1Val/2;
      if ( sperblock0 < 4 )
         sperblock0 = 4;

      nblks =fn1Val/(2 *sperblock0);

      if (nblks == 0)          /* must be at least one block */
         nblks = 1;

/*****************************************
 * *  If two blocks, make one larger block  *
 * *  for in-core transposition.            *
 * *****************************************/

      if (nblks == 2)
      {
        nblks = 1;
        sperblock0 *= 2;
      }

//    sperblock1 = fn0/(2*nblks);

// fprintf(stderr,"sperblock0=%d specblock1= %d nblks= %d\n",sperblock0, sperblock1, nblks);

  

  datahead.nblocks = 1;
  datahead.ntraces = fn0;
  datahead.np      = fn1Val;
  datahead.vers_id = VERSION;
  datahead.vers_id += DATA_FILE;
  datahead.vers_id |= S_MAKEFID;
  datahead.nbheaders = 1;
  datahead.ebytes  = 4;
  datahead.tbytes  = datahead.ebytes * datahead.np;
  datahead.bbytes  = datahead.tbytes * datahead.ntraces +
                       sizeof(dblockhead) * datahead.nbheaders;
  datahead.status  = (S_DATA|S_SPEC|S_FLOAT|S_NP|S_NI|S_TRANSF|S_SECND);

#ifdef DEBUG
     Wscrprintf("\nDATA FILE HEADER:\n");
     Wscrprintf("  status  = %8x,  nbheaders       = %8x\n", datahead.status,
        datahead.nbheaders);
     Wscrprintf("  nblocks = %8d,  bytes per block = %8d\n", datahead.nblocks,
        datahead.bbytes);
     Wscrprintf("  ntraces = %8d,  bytes per trace = %8d\n", datahead.ntraces,
        datahead.tbytes);
     Wscrprintf("  npoints = %8d,  bytes per point = %8d\n", datahead.np,
        datahead.ebytes);
     Wscrprintf("  vers_id = %8d\n", datahead.vers_id);
#endif

  if ( (r=D_newhead(D_DATAFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  if ( (r=D_allocbuf(D_DATAFILE,0,&block)) )
    { D_error(r); ABORT;
    }

  block.head->scale  = 0;
  block.head->status = (S_DATA|S_SPEC|S_FLOAT);
  block.head->index  = 1;
  block.head->mode   = 0;	/* COULD BE A PROBLEM HERE??   S.F. */
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
static int setdatafile(int *fnval)
/**********************/
{ char path[MAXPATHL];
  dfilehead datahead;
  dpointers block;
  double rfn;
  int r;

  if ( (r=P_getreal(CURRENT,"fn",&rfn,1)) )
    { P_err(r,"fn",":");
      ABORT;
    }
  P_setreal(PROCESSED,"fn",rfn,0);
  *fnval = (int) rfn;
  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);

  if ( (r = D_getfilepath(D_DATAFILE, path, curexpdir)) )
  {
     D_error(r);
     ABORT;
  }

  datahead.nblocks = 1;
  datahead.ntraces = 1;
  datahead.np      = *fnval / 2;
  datahead.vers_id = VERSION;
  datahead.vers_id += DATA_FILE;
  datahead.nbheaders = 1;
  datahead.ebytes  = 4;
  datahead.tbytes  = datahead.ebytes * datahead.np;
  datahead.bbytes  = datahead.tbytes * datahead.ntraces +
                       sizeof(dblockhead) * datahead.nbheaders;
  datahead.status  = (S_DATA|S_SPEC|S_FLOAT|S_NP);

  if ( (r=D_newhead(D_DATAFILE,path,&datahead)) )
    { D_error(r); ABORT;
    }
  if ( (r=D_allocbuf(D_DATAFILE,0,&block)) )
    { D_error(r); ABORT;
    }

  block.head->scale  = 0;
  block.head->status = (S_DATA|S_SPEC|S_FLOAT);
  block.head->mode   = 0;	/* COULD BE A PROBLEM HERE??   S.F. */
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

/**************************/
int readspectrum(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
   int i,r,argnum;
   float scalefactor;
   int   fntmp;
   int   do_ds = 1;
   FILE *f1;
   register float *rdata;
   struct stat buf;
   int ibuf;
   float fbuf;
   int wordSize;
   int intType;  // 1 for integer, 0 for float
   int fnVal;
   int fn1Val = 0;
   int f2 = 1;
   size_t ret __attribute__((unused));

   Wturnoff_buttons();
   D_allrelease();
   argnum = 1;
   if (argc < 2)
   {
      Werrprintf("%s: requires file name as first argument",argv[0]);
      ABORT;
   }
   argnum = 2;
   scalefactor = 1.0;
   wordSize = sizeof(int);
   intType=1; // defines integer type
   while (argc>argnum)
   {
      if (isReal(argv[argnum]))
      { scalefactor = stringReal(argv[argnum]);
      }
      else if (strcmp(argv[argnum],"int") == 0)
      {
         intType=1; // defines integer type
      }
      else if (strcmp(argv[argnum],"float") == 0)
      {
         intType=0; // defines float type
      }
      else if (strcmp(argv[argnum],"nods") == 0)
      {
         do_ds = 0;
      }
      else if ( ! strcmp(argv[argnum],"f1") )
         f2 = 0;
      else if ( ! strcmp(argv[argnum],"f2") )
         f2 = 1;
      else if (strcmp(argv[argnum],"incr") == 0)
      {
         if ( (argnum+1 < argc)  && isReal(argv[argnum+1]) ) 
         {
            argnum++;
            fn1Val = atoi(argv[argnum]);
         }
         else
         {
            Werrprintf("%s: fn1 argument must be followed by the number of f1 traces",argv[0]);
            ABORT;
         }
      }
      argnum++;
   }
   if (!(f1 = fopen(argv[1],"r")))  {
      Werrprintf("%s: Error - file \"%s\" not found",argv[0], argv[1]);
      ABORT;
   }
   stat(argv[1],&buf);
   fnVal = buf.st_size / wordSize;
   if (fn1Val)
   {
      int j;
      if (fnVal % fn1Val)
      {
         Werrprintf("%s: Error - fn1 value inconsistent with data",argv[0]);
         fclose(f1);
         ABORT;
      }
      fnVal /= fn1Val;
      P_setreal(CURRENT,"fn1", (double) fn1Val*2, 0);
      P_setreal(CURRENT,"fn", (double) fnVal*2, 0);
      P_setreal(CURRENT,"procdim", (double) 2, 0);
      P_setreal(PROCESSED,"procdim", (double) 2, 0);
      if (set2Ddatafile(fnVal, fn1Val))
      {
         fclose(f1);
         ABORT;
      }
      rdata = data;
      if (intType)
      {
         for (j=0; j<fnVal; j++)
         for (i=0; i<fn1Val; i++)
         {
            if (f2)
            {
               fseek(f1,(long) (j+i*fnVal)*sizeof(int), SEEK_SET);
            }
            ret = fread(&ibuf,sizeof(int),1,f1);
            *rdata++ = (float) ibuf * scalefactor;
         }
      }
      else
      {
         for (j=0; j<fnVal; j++)
         for (i=0; i<fn1Val; i++)
         {
            if (f2)
            {
               fseek(f1,(long) (j+i*fnVal)*sizeof(float), SEEK_SET);
            }
            ret = fread(&fbuf,sizeof(float),1,f1);
            *rdata++ = fbuf * scalefactor;
         }
      }
   }
   else
   {
   P_setreal(CURRENT,"fn", (double) fnVal*2, 0);
      P_setreal(CURRENT,"procdim", (double) 1, 0);
      P_setreal(PROCESSED,"procdim", (double) 1, 0);
   if (setdatafile(&fntmp))
   {
      fclose(f1);
      ABORT;
   }
   rdata = data;
   if (intType) // integer
   {
      for (i=0; i<fnVal; i++)
      {
         ret = fread(&ibuf,sizeof(int),1,f1);
         *rdata++ = (float) ibuf * scalefactor;
      }
   }
   else  // float
   {
      for (i=0; i<fnVal; i++)
      {
         ret = fread(&fbuf,sizeof(float),1,f1);
         *rdata++ = fbuf * scalefactor;
      }
   }
   }
   fclose(f1);
   if ( (r=D_markupdated(D_DATAFILE,0)) )
   { D_error(r); ABORT;
   }
   if ( (r=D_release(D_DATAFILE,0)) )
   { D_error(r); ABORT;
   }
   P_setstring(CURRENT,"file",argv[0],0);
   P_setstring(PROCESSED,"file",argv[0],0);
   if (!Bnmr && do_ds)
   {
      releasevarlist();
      appendvarlist("cr");
      Wsetgraphicsdisplay("ds");		/* activate the ds program */
      start_from_ft = 1;
   }
   RETURN;
}

/*******************************/
static int calculate(double scalefactor, int fitspecflag, int fnval)
/*******************************/
/* fixed for proper calculation of gaussian linewidth                   */
/* according to suggestions by Frank Heatley, University of Manchester  */
/* r.kyburz, 1/21/92 */
{ register int i,pl,pr,delta,lw40;
  register double lorenzian,vs0,vs1,lw_gfc,lwsq_2gfc;
  double sw,rfl,rfp,vs;
  float pointsperhz;
  register float *rdata;
  char aig[10];
  int r;

  rdata = data;
  if ( (r=P_getreal(CURRENT,"sw",&sw,1)) )
    { P_err(r,"sw",":");
      ABORT;

    }
  P_setreal(PROCESSED,"sw",sw,0);
  if ( (r=P_getreal(CURRENT,"rfl",&rfl,1)) )
    { P_err(r,"rfl",":");
      ABORT;
    }
  if ( (r=P_getreal(CURRENT,"rfp",&rfp,1)) )
    { P_err(r,"rfp",":");
      ABORT;
    }
  if ( (r=P_getreal(CURRENT,"vs",&vs,1)) )
    { P_err(r,"vs",":");
      ABORT;
    }
  if (fitspecflag) 
      vs = scalefactor/vs;
  else
  {  
      if ( (r=P_getstring(CURRENT,"aig",aig,1,10)) )
      { P_err(r,"aig",":");
        ABORT;
      }
      if (strcmp(aig,"nm") == 0)
        r = P_setreal(CURRENT,"vs",100.0,1);
      else
        r = P_setreal(CURRENT,"vs",1000.0,1);
      if (r)
      { P_err(r,"vs",":");
        ABORT;
      }
      vs = scalefactor/100.0;
  }
  pointsperhz = fnval/(2*sw);
  zerofill(data,fnval/2);
  for (i=0; i<nlines; i++)
    {
      lorenzian = 1.0 - gaussian[i];
      lw40 = (int)(40.0 * width[i] * pointsperhz);
      if (lorenzian>0.0)
        { /* calculate lorenzian line shape */
          vs0 = intensity[i] * lorenzian * vs;
          lw_gfc = width[i] * pointsperhz / 2.0;
          lwsq_2gfc = lw_gfc * lw_gfc;
          pr = pl = datapoint((double) frequency[i] - rfp + rfl, sw, fnval/2);
          if ((pl>=0) && (pl<fnval/2))
            rdata[pl] += vs0;
          delta = 0;
          do
            { pl--; pr++; delta++;
              vs1 = vs0 * lwsq_2gfc / (lwsq_2gfc + ((double) delta* (double) delta));
              if ((pl>=0) && (pl<fnval/2))
              {
                rdata[pl] += (float) vs1;
              }
              if ((pr>=0) && (pr<fnval/2))
              {
                rdata[pr] += (float) vs1;
              }
            }
            while (delta<lw40);
        }
      if (gaussian[i]>0.0)
        { /* calculate gaussian line shape */
          vs0 = intensity[i] * gaussian[i] * vs;
          lw_gfc = 1.665109 / (width[i] * pointsperhz);
          lw_gfc = lw_gfc * lw_gfc;
          pr = pl = datapoint((double) frequency[i] - rfp + rfl, sw, fnval/2);
          if ((pl>=0) && (pl<fnval/2))
            rdata[pl] += vs0;
          vs1 = vs0;
          delta = 0;
          do
            { pl--; pr++; delta++;
              vs1 = vs0 * exp(0.0 - lw_gfc * (double) delta * (double) delta);
              if ((pl>=0) && (pl<fnval/2))
                rdata[pl] += vs1;
              if ((pr>=0) && (pr<fnval/2))
                rdata[pr] += vs1;
            }
            while ((delta<lw40) && (lw_gfc<1));
        }
    }
  RETURN;
}

/**************************/
int dsp(int argc, char *argv[], int retc, char *retv[])
/**************************/
{ int i,j,r,argnum,fitspecflag;
  float scalefactor;
  char  filename[MAXPATHL];
  int   fnval;
  int   do_ds = 1;

  Wturnoff_buttons();
  D_allrelease();
  argnum = 1;
  if ((argc>argnum)&&(!isReal(argv[argnum])) &&
      (strcmp(argv[argnum],"spins.outdata")!=0) && 
      (strcmp(argv[argnum],"nods")!=0) && 
      (strcmp(argv[argnum],"intensmax")!=0))
    {
      strcpy(filename,argv[argnum]);
      if (readinput(filename)) ABORT;
      argnum++;
      for (j=0; (filename[j]!='\0' && j<MAXPATHL-1); j++);
#ifdef UNIX
      for (i=j-1;(i>0 && filename[i]!='/'); i--); 
      if (filename[i]=='/') i++;
#else 
      for (i=j-1;(i>0 && filename[i]!=']'); i--); 
      if (filename[i]==']') i++;
#endif 
      fitspecflag =
	 (filename[i]=='f' && filename[i+1]=='i' && filename[i+2]=='t');
    }
  else
    {
      strcpy(filename,curexpdir);
#ifdef UNIX
      strcat(filename,"/spins.outdata");
#else 
      strcat(filename,"spins.outdata");
#endif 
      if (readinput(filename)) ABORT;
      fitspecflag = 0;
    }
  if ( (r  = P_setreal(GLOBAL,"svs",intensmax,1)) )
        Werrprintf("error saving svs\n");
  if ((argc>argnum) && (strcmp(argv[argnum],"intensmax") == 0)) RETURN;
  scalefactor = 1.0;
  while (argc>argnum)
  {
    if (isReal(argv[argnum]))
    { scalefactor = stringReal(argv[argnum]);
    }
    else if (strcmp(argv[argnum],"nods") == 0)
    {
       do_ds = 0;
    }
    argnum++;
  }
  if (setdatafile(&fnval))
    ABORT;
  if (calculate(scalefactor,fitspecflag,fnval))
    ABORT;
  if ( (r=D_markupdated(D_DATAFILE,0)) )
    { D_error(r); ABORT;
    }
  if ( (r=D_release(D_DATAFILE,0)) )
    { D_error(r); ABORT;
    }
  if (!Bnmr && do_ds)
  {
     releasevarlist();
     appendvarlist("cr");
     Wsetgraphicsdisplay("ds");		/* activate the ds program */
     start_from_ft = 1;
  }
  RETURN;
}

/************************/
static int datawrite(char *filename)
/************************/
{
  FILE *datafile;
  register int i,r;
  float *data;
  double vs;
  Wturnoff_buttons();
  D_allrelease();
  d2flag = 0;
  if(select_init(1,1,0,0,0,1,1,1)) ABORT;
  if ((specIndex < 1) || (specIndex > nblocks * specperblock))
    { Werrprintf("spectrum %d does not exist",specIndex);
      specIndex = 1;
    }
  if ( (r=P_getreal(CURRENT,"vs",&vs,1)) )
    { P_err(r,"vs",":");
      ABORT;
    }
  disp_specIndex(specIndex);
  data = gettrace(specIndex-1,fpnt);
  if (data==0)
    ABORT;
  if ( (datafile=fopen(filename,"w+")) )
    { fprintf(datafile,"%d\n",npnt);
      fprintf(datafile,"%g\n",sp);
      fprintf(datafile,"%g\n",wp);
      for (i=0; i<npnt; i++)
        fprintf(datafile,"%g\n",*data++ * vs);
      fclose(datafile);
      chmod(filename,0666);
    }
  else
    Werrprintf("cannot open file %s",filename);
  D_allrelease();
  RETURN;
}

/*************************/
static int display_parameters()
/*************************/
/* fixed for proper calculation of gaussian linewidth                   */
/* according to suggestions by Frank Heatley, University of Manchester  */
/* r.kyburz, 1/21/92 */
{ int i;
 Wscrprintf(
"line   frequency    intensity    integral  line width   gaussian fraction\n");
  for (i=0; i<nlines; i++)
    Wscrprintf("%3d%12.3f%c%12.3f%c%12.3f %12.3f%c%12.3f%c\n",
      i+1,frequency[i],do_freq[i],intensity[i],do_int[i], width[i] *
       intensity[i] * (1.064467*gaussian[i] + 1.570796*(1.0-gaussian[i])),
      width[i],do_width[i],gaussian[i],do_gauss[i]);
  RETURN;
}

/****************************/
static int fitspec_setresult()
/****************************/
{ int i;
  char filename[MAXPATHL];

  Wscrprintf("Parameters:\n");
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fitspec.outpar");
#else 
  strcat(filename,"fitspec.outpar");
#endif 
  if (readinput(filename))
    ABORT;
  display_parameters();
  P_setreal(GLOBAL,"slfreq",0.0,0);
  for (i=0; i<nlines; i++)
    { P_setreal(GLOBAL,"slfreq",frequency[i],i+1);
    }
  RETURN;
}

/******************************/
int fitspec(int argc, char *argv[], int retc, char *retv[])
/******************************/
{
  FILE *inputfile;
  FILE *outputfile;
  char  filename[MAXPATHL];
  char execstring[256];
  int error,count1,count2;
  int i,j,more_to_do,r,stop;
  double llfrq,llamp,vs,rfl,rfp,rflrfp,slw;
  float chisq;
  double value;
  int ret __attribute__((unused));

#ifdef CLOCKTIME
  /* Turn on a clocktime timer */
  (void)start_timer ( fitspec_timer_no );
#endif 

  if (P_getreal(GLOBAL,"slfreq",&value,1))
    if (spins_makepar("slfreq",0.0,SETVAL&SETARR,ST_REAL))
      ABORT;
  stop = 0;
  if (argc>1) 
  { if (strcmp(argv[1],"usell")==0)
      { stop = 1;
        if ( (r=P_getreal(CURRENT,"vs",&vs,1)) )
          { P_err(r,"vs",":");
            ABORT;
          }
        if ( (r=P_getreal(CURRENT,"rfl",&rfl,1)) )
          { P_err(r,"rfl",":");
            ABORT;
          }
        if ( (r=P_getreal(CURRENT,"rfp",&rfp,1)) )
          { P_err(r,"rfp",":");
            ABORT;
          }
        if ( (r=P_getreal(GLOBAL,"slw",&slw,1)) )
          { P_err(r,"slw",":");
            ABORT;
          }
        rflrfp = rfl - rfp;
        strcpy(filename,curexpdir);
#ifdef UNIX
        strcat(filename,"/fitspec.inpar");
#else 
        strcat(filename,"fitspec.inpar");
#endif 
        if ( (outputfile=fopen(filename,"w+")) )
          { i = 1;
            j = 0;
            more_to_do = 1;
            while (more_to_do)
              { more_to_do = 0;
                if (P_getreal(CURRENT,"llfrq",&llfrq,i)==0)
                  more_to_do += 1;
                if (P_getreal(CURRENT,"llamp",&llamp,i)==0)
                  more_to_do += 2;
                if (more_to_do==3)
                  { fprintf(outputfile,"%12g,%12g,%12g\n",
                      llfrq-rflrfp,vs*llamp,slw);
                    j++;
                  }
                i++;
              }
            fclose(outputfile);
            if (j<1)
              { Werrprintf("no lines in line listing");
                ABORT;
              }
          }
        else
          { Werrprintf("cannot open file %s",filename);
            ABORT;
          }
      }
    else if (strcmp(argv[1],"setslfreq")==0)
      { Wturnoff_buttons();
        Wsettextdisplay("fitspec");
        Wclear_graphics();
        Wshow_text();
        fitspec_setresult();
        RETURN;
      }
   else
     { Werrprintf("usage - fitspec<('usell' or 'setslfreq')>");
       ABORT;
     }
  }
  Wturnoff_buttons();
  D_allrelease();
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fitspec.data");
#else 
  strcat(filename,"fitspec.data");
#endif 
  datawrite(filename);
  Wsettextdisplay("fitspec");
  Wclear_graphics();
  Wshow_text();
  Wscrprintf("Curve fitting: input parameters:\n");
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fitspec.inpar");
#else 
  strcat(filename,"fitspec.inpar");
#endif 
  readinput(filename);
  display_parameters();
  if (stop)
    RETURN;
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fitspec.outpar");
#else 
  strcat(filename,"fitspec.outpar");
#endif 
  unlink(filename);
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/fitspec.stat");
#else 
  strcat(filename,"fitspec.stat");
#endif 
  unlink(filename);
  sprintf(execstring,"fitspec %s",curexpdir);
  ret = system(execstring);
  if ( (inputfile=fopen(filename,"r")) )
    { if (fscanf(inputfile,"%d\n",&error)!=1)
        { Werrprintf("problem reading from fitspec.stat,line 1");
          ABORT;
        }
      switch (error)
        { case 0: break;
          case 1: Werrprintf("inproper selection of parameters to fit");
                  fclose(inputfile);
                  ABORT;
	  case 2: Werrprintf("singular matrix");
                  fclose(inputfile);
                  ABORT;
          case 3: Werrprintf("problem reading file 'fitspec.inpar'");
                  fclose(inputfile);
                  ABORT;
          case 4: Werrprintf("too many parameters");
                  fclose(inputfile);
                  ABORT;
          case 5: Werrprintf("unable to write to file 'fitspec.outpar'");
                  fclose(inputfile);
                  ABORT;
          case 6: Werrprintf("problem reading file 'fitspec.dat'");
                  fclose(inputfile);
                  ABORT;
          case 7: Werrprintf("not enough data points, increase fn or wp");
                  fclose(inputfile);
                  ABORT;
          case 8: Werrprintf("too many data points, decrease fn or wp");
                  fclose(inputfile);
                  ABORT;
          default:Werrprintf("unidentified error during fitting");
                  fclose(inputfile);
                  ABORT;
        }
      if (fscanf(inputfile,"%f\n",&chisq)!=1)
        { Werrprintf("problem reading from fitspec.stat, line 2");
          ABORT;
        }
      if (fscanf(inputfile,"%d\n",&count1)!=1)
        { Werrprintf("problem reading from fitspec.stat, line 3");
          ABORT;
        }
      if (fscanf(inputfile,"%d\n",&count2)!=1)
        { Werrprintf("problem reading from fitspec.stat, line 4");
          ABORT;
        }
      fclose(inputfile);
      Wscrprintf("Number of data points:      %8d\n",npnt);
      Wscrprintf("Final chi square:       %12.3f\n",chisq);
      Wscrprintf("Total number of iterations: %8d\n",count2);
      Wscrprintf("Successful iterations:      %8d\n",count1);
      Wscrprintf("Digital resolution:     %12.3f Hz/point\n",wp/npnt);
      if (count1<2)
       Wscrprintf("NO GOOD FIT OBTAINED\n");
      else
       Wscrprintf("ITERATION HAS CONVERGED\n");
      fitspec_setresult();
    }
  else
    Werrprintf("problem running program 'fitspec', no status available");

#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  (void)stop_timer ( fitspec_timer_no );
#endif 

  RETURN;
}

/*********************************************/
static int spins_makepar(char *varname, double value, int pbits, int type)
/*********************************************/
{ int r;
  if ( (r=P_creatvar(GLOBAL,varname,type)) )
    { P_err(r,varname,"(createvar):");
      ABORT;
    }
  if ( (r=P_setgroup(GLOBAL,varname,G_SPIN)) )
    { P_err(r,varname,"(setgroup):");
      ABORT;
    }
  if ( (r=P_setprot(GLOBAL,varname,pbits)) )
    { P_err(r,varname,"(setprot):");
      ABORT;
    }
  if ((type==ST_REAL)||(type==ST_INTEGER))
    { if ( (r=P_setlimits(GLOBAL,varname,1e10,-1e10,0.0)) )
        { P_err(r,varname,"(setreal):");
          ABORT;
        }
      if ( (r=P_setreal(GLOBAL,varname,value,0)) )
        { P_err(r,varname,"(setreal):");
          ABORT;
        }
    }
  RETURN;
}

/*******************************/
static int spins_init()
/*******************************/
{ int r;
  if (P_getactive(GLOBAL,"spinsys") < 0)
  {
     if (spins_makepar("spinsys",0.0,0,ST_STRING))
       ABORT;
     if ( (r=P_setlimits(GLOBAL,"spinsys",20.0,0.0,0.0)) )
     { P_err(r,"spinsys","(setlimits):");
         ABORT;
     }
     if ( (r=P_setprot(GLOBAL,"spinsys",0)) )
     { P_err(r,"spinsys","(setprot):");
         ABORT;
     }
  }
  if (P_getactive(GLOBAL,"dga") < 0)
  {
     if (spins_makepar("dga",0.0,0,ST_STRING))
       ABORT;
     P_setlimits(GLOBAL,"dga",1024.0,0.0,0.0);
  }
  if (P_getactive(GLOBAL,"dla") < 0)
  {
     if (spins_makepar("dla",0.0,0,ST_STRING))
       ABORT;
     P_setlimits(GLOBAL,"dla",1024.0,0.0,0.0);
  }

  if (P_getactive(GLOBAL,"sminf") < 0)
    if (spins_makepar("sminf",0.0,SETVAL,ST_REAL))
      ABORT;
  if (P_getactive(GLOBAL,"smaxf") < 0)
    if (spins_makepar("smaxf",1000.0,SETVAL,ST_REAL))
      ABORT;
  if (P_getactive(GLOBAL,"svs") < 0)
    if (spins_makepar("svs",150.0,SETVAL,ST_REAL))
      ABORT;
  if (P_getactive(GLOBAL,"sth") < 0)
    { if (spins_makepar("sth",0.01,SETVAL,ST_REAL))
        ABORT;
      P_setlimits(GLOBAL,"sth",1.0,0.0,0.001);
    }
  if (P_getactive(GLOBAL,"slw") < 0)
    { if (spins_makepar("slw",0.5,SETVAL,ST_REAL))
        ABORT;
      P_setlimits(GLOBAL,"slw",1e6,0.01,0.01);
    }
  if (P_getactive(GLOBAL,"clamp") < 0)
    { if (spins_makepar("clamp",0.0,SETVAL&SETARR,ST_INTEGER))
        ABORT;
      P_setlimits(GLOBAL,"clamp",4096.0,0.0,1.0);
    }
  if (P_getactive(GLOBAL,"clindex") < 0)
  {
     if (spins_makepar("clindex",0.0,SETVAL&SETARR,ST_INTEGER))
       ABORT;
     P_setlimits(GLOBAL,"clindex",4096.0,0.0,1.0);
  }
  if (P_getactive(GLOBAL,"cla") < 0)
  {
     if (spins_makepar("cla",0.0,SETVAL&SETARR,ST_INTEGER))
       ABORT;
     P_setlimits(GLOBAL,"cla",4096.0,0.0,1.0);
  }
  if (P_getactive(GLOBAL,"slfreq") < 0)
    if (spins_makepar("slfreq",0.0,SETVAL&SETARR,ST_REAL))
      ABORT;
  if (P_getactive(GLOBAL,"clfreq") < 0)
    if (spins_makepar("clfreq",0.0,SETVAL&SETARR,ST_REAL))
      ABORT;
  if (P_getactive(GLOBAL,"niter") < 0)
    { if (spins_makepar("niter",0.0,SETVAL,ST_INTEGER))
      ABORT;
      P_setlimits(GLOBAL,"niter",9999.0,0.0,1.0);
    }
  if (P_getactive(GLOBAL,"iterate") < 0)
  {
     if (spins_makepar("iterate",0.0,SETVAL,ST_STRING))
       ABORT;
     P_setlimits(GLOBAL,"iterate",256.0,0.0,0.0);
  }
  RETURN;
}

/*******************************/
static int spins_system(char *systemstring)
/*******************************/
{ int i,j,nspins,r;
  FILE *fp;
  char paramfile[MAXSTR];
  char nuctable[8]; 
  char varname[4];
  // int  nucnumber[8];
  char dgastring[1024];
  char dlastring[512];
  double value,sw;
  i = 0;
  nspins = 0;
  while (systemstring[i])
    { /* capitalize, if necessary */
      if ((systemstring[i]>='a') && (systemstring[i]<='z'))
        systemstring[i] -= 32;
      if ((systemstring[i]>='A') && (systemstring[i]<='Z'))
        { if (nspins>=MAXSPINS)
            { Werrprintf("Too many non equivalent spins, maximum=%d",MAXSPINS);
              ABORT;
            }
          nuctable[nspins] = systemstring[i];
          // nucnumber[nspins] = 1;
          i++;
          if ((systemstring[i]>='1') && (systemstring[i]<='9'))
            { // nucnumber[nspins] = systemstring[i]-'0';
              i++;
            }
          nspins++;
        }
      else
        { Werrprintf("Illegal character &c in spin system string");
          ABORT;
        }
    }
  P_deleteGroupVar(GLOBAL,G_SPIN);
  if (spins_makepar("spinsys",0.0,0,ST_STRING))
    ABORT;
  if ( (r=P_setlimits(GLOBAL,"spinsys",20.0,0.0,0.0)) )
    { P_err(r,"spinsys","(setlimits):");
      ABORT;
    }
  if ( (r=P_setprot(GLOBAL,"spinsys",0)) )
    { P_err(r,"spinsys","(setprot):");
      ABORT;
    }
  if ( (r=P_setstring(GLOBAL,"spinsys",systemstring,0)) )
    { P_err(r,"spinsys","(setstring):");
      ABORT;
    }
  strcpy(spins_parlist,"");
  strcpy(paramfile,userdir);
  strcat(paramfile,"/persistence/spins_pard");
  unlink(paramfile);
  if (! (fp=fopen(paramfile,"w")))
    ABORT;
  strcpy(dlastring,"1:EXPERIMENTAL LINES:[slfreq:11:2];");
  strcat(dlastring,"3:LINES CALCULATED:[clindex:4:0,clfreq:11:2];");
  strcpy(dgastring,
    "1:SPIN SYSTEM:spinsys,sminf:2,smaxf:2,sth:4,slw:3,niter:0,iterate,svs:3;");
  strcat(dgastring,"2:CHEM SHIFTS:");
  for (i=0; i<nspins; i++)
    { varname[0] = nuctable[i];
      varname[1] = 0;
      if (spins_makepar(varname,0.0,SETVAL,ST_REAL))
      {
        fclose(fp);
        ABORT;
      }
      P_setlimits(GLOBAL,varname,1.0e10,-1.0e10,0.0);
      strcat(dgastring,varname);
      strcat(dgastring,":2");
      if (i<nspins-1)
        strcat(dgastring,",");
      else
        strcat(dgastring,";");
      strcat(spins_parlist,varname);
      strcat(spins_parlist," ");
      if (i==0)
        strcpy(spins_dmenu_par,varname);
      fprintf(fp,"%s %s\n",varname,varname);
    }
  fclose(fp);
  strcpy(paramfile,userdir);
  strcat(paramfile,"/persistence/spins_parj");
  unlink(paramfile);
  if (! (fp=fopen(paramfile,"w")))
    ABORT;
  strcat(dgastring,"3:COUPLING CONSTANTS:");
  for (i=0; i<nspins-1; i++)
    for (j=i+1; j<nspins; j++)
      { varname[0] = 'J';
        varname[1] = nuctable[i];
        varname[2] = nuctable[j];
        varname[3] = 0;
        if (spins_makepar(varname,0.0,SETVAL,ST_REAL))
        {
          fclose(fp);
          ABORT;
        }
        P_setlimits(GLOBAL,varname,1.0e10,-1.0e10,0.0);
        strcat(dgastring,varname);
        strcat(dgastring,":2");
        if ((i<nspins-2)||(j<nspins-1))
          strcat(dgastring,",");
        else
          strcat(dgastring,";");
        strcat(spins_parlist,varname);
        strcat(spins_parlist," ");
        if (i==0 && j==i+1)
          strcpy(spins_jmenu_par,varname);
        fprintf(fp,"%s %s\n",varname,varname);
      }
  fclose(fp);
  if (spins_makepar("dga",0.0,0,ST_STRING))
    ABORT;
  P_setlimits(GLOBAL,"dga",1024.0,0.0,0.0);
  if ( (r=P_setstring(GLOBAL,"dga",dgastring,0)) )
    { P_err(r,"dga","(setstring):");
      ABORT;
    }
  if (spins_makepar("dla",0.0,0,ST_STRING))
    ABORT;
  P_setlimits(GLOBAL,"dla",1024.0,0.0,0.0);
  if ( (r=P_setstring(GLOBAL,"dla",dlastring,0)) )
    { P_err(r,"dla","(setstring):");
      ABORT;
    }

  if (P_getreal(GLOBAL,"sminf",&value,1))
    if (spins_makepar("sminf",0.0,SETVAL,ST_REAL))
      ABORT;
  if (P_getreal(GLOBAL,"smaxf",&value,1))
    if (spins_makepar("smaxf",1000.0,SETVAL,ST_REAL))
      ABORT;
  if (P_getreal(GLOBAL,"svs",&value,1))
    if (spins_makepar("svs",150.0,SETVAL,ST_REAL))
      ABORT;
  if (P_getreal(GLOBAL,"sth",&value,1))
    { if (spins_makepar("sth",0.01,SETVAL,ST_REAL))
        ABORT;
      P_setlimits(GLOBAL,"sth",1.0,0.0,0.001);
    }
  if (P_getreal(GLOBAL,"slw",&value,1))
    { if (spins_makepar("slw",0.5,SETVAL,ST_REAL))
        ABORT;
      P_setlimits(GLOBAL,"slw",1e6,0.01,0.01);
    }
  if (P_getreal(GLOBAL,"clamp",&value,1))
    { if (spins_makepar("clamp",0.0,SETVAL&SETARR,ST_INTEGER))
        ABORT;
      P_setlimits(GLOBAL,"clamp",4096.0,0.0,1.0);
    }
  if (spins_makepar("clindex",0.0,SETVAL&SETARR,ST_INTEGER))
    ABORT;
  P_setlimits(GLOBAL,"clindex",4096.0,0.0,1.0);
  if (spins_makepar("cla",0.0,SETVAL&SETARR,ST_INTEGER))
    ABORT;
  P_setlimits(GLOBAL,"cla",4096.0,0.0,1.0);
  if (P_getreal(GLOBAL,"slfreq",&value,1))
    if (spins_makepar("slfreq",0.0,SETVAL&SETARR,ST_REAL))
      ABORT;
  if (spins_makepar("clfreq",0.0,SETVAL&SETARR,ST_REAL))
    ABORT;
  if (P_getreal(GLOBAL,"niter",&value,1))
    { if (spins_makepar("niter",0.0,SETVAL,ST_INTEGER))
      ABORT;
      P_setlimits(GLOBAL,"niter",9999.0,0.0,1.0);
    }
  if (spins_makepar("iterate",0.0,SETVAL,ST_STRING))
    ABORT;
  P_setlimits(GLOBAL,"iterate",256.0,0.0,0.0);
  P_setstring(GLOBAL,"iterate","",0);
  if (P_getreal(CURRENT,"sw",&sw,1)==0)
    { P_setreal(GLOBAL,"sminf",0.0,0);
      P_setreal(GLOBAL,"smaxf",sw,0);
    }
  execString("dga\n");
  spins_param("show","",0,NULL);
#ifdef VNMRJ
  appendJvarlist("spinsys");
  // writelineToVnmrJ("pnew 1","spinsys");
#endif
  RETURN;
}

/*****************************/
static int spins_calculate(char *options, int iterateflag)
/*****************************/
{
  FILE *outputfile,*inputfile;
  char  filename[MAXPATHL];
  char systemstring[2*MAXSPINS+2];
  double value;
  char nuctable[MAXSPINS];
  // int nucnumber[MAXSPINS];
  int i,j,nspins,r;
  int *clindex, clindexsize, clindexvalue;
  int *saveclindex;
  int  found;
  int *cla, clasize;
  int *savecla;
  int a, error;
  double cladoub;
  char varname[4];
  char execstring[256];
  float f1,f2;
  int ret __attribute__((unused));

  if (getsize(GLOBAL,"clindex",&clindexsize))
      ABORT;
  if (getsize(GLOBAL,"cla",&clasize))
      ABORT;
  cla = NULL;
  clindex = NULL;
  if (iterateflag || clindexsize == clasize)
  {
    /* keeps same line assignment when order of transitions may have changed */
    clindex = (int *)allocateWithId(clindexsize*sizeof(int),"spins");
    cla     = (int *)allocateWithId(clasize*sizeof(int),"spins");

    saveclindex = clindex;
    savecla     = cla;
    for (i=1; i<=clindexsize; i++)
    {
        if ( (r = P_getreal(GLOBAL,"clindex",&cladoub,i)) )
        {   P_err(r,"clindex",":");
            releaseAllWithId("spins");
            ABORT;
        }
        else
	{
            *clindex = (int) (cladoub + 0.1);
	    clindex++;
	}
    }
    for (i=1; i<=clasize; i++)
    {
        if ( (r = P_getreal(GLOBAL,"cla",&cladoub,i)) )
        {   P_err(r,"cla",":");
            releaseAllWithId("spins");
            ABORT;
        }
        else
	{
            *cla = (int) (cladoub + 0.1);
	    cla++;
        }
    }
    clindex = saveclindex;
    cla     = savecla;
  }
  else
  {
    P_setreal(GLOBAL,"clindex",(double)0.0,0);
  }

  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spins.inpar");
#else 
  strcat(filename,"spins.inpar");
#endif 
  if ( (r=P_getstring(GLOBAL,"spinsys",systemstring,1,2*MAXSPINS+1)) )
    { Werrprintf("no spin system defined, r=%d",r);
      ABORT;
    }
  i = 0;
  nspins = 0;
  while (systemstring[i])
    { /* capitalize, if necessary */
      if ((systemstring[i]>='a') && (systemstring[i]<='z'))
        systemstring[i] -= 32;
      if ((systemstring[i]>='A') && (systemstring[i]<='Z'))
        { if (nspins>=MAXSPINS)
            { Werrprintf("Too many non equivalent spins, maximum=%d",MAXSPINS);
              ABORT;
            }
          nuctable[nspins] = systemstring[i];
          // nucnumber[nspins] = 1;
          i++;
          if ((systemstring[i]>='1') && (systemstring[i]<='9'))
            { // nucnumber[nspins] = systemstring[i]-'0';
              i++;
            }
          nspins++;
        }
      else
        { Werrprintf("Illegal character &c in spin system string");
          ABORT;
        }
    }
  if ( (outputfile=fopen(filename,"w+")) )
    { if ( (r=P_getreal(GLOBAL,"sth",&value,1)) )
        { P_err(r,"sth",":");
          fclose(outputfile);
          ABORT;
        }
      fprintf(outputfile,"%g,",value);
      if ( (r=P_getreal(GLOBAL,"sminf",&value,1)) )
        { P_err(r,"sminf",":");
          fclose(outputfile);
          ABORT;
        }
      fprintf(outputfile,"%g,",value);
      if ( (r=P_getreal(GLOBAL,"smaxf",&value,1)) )
        { P_err(r,"smaxf",":");
          fclose(outputfile);
          ABORT;
        }
      fprintf(outputfile,"%g\n",value);
      fprintf(outputfile,"%s\n",systemstring);
      for (i=0; i<nspins; i++)
        { varname[0] = nuctable[i];
          varname[1] = 0;
          if ( (r=P_getreal(GLOBAL,varname,&value,1)) )
            { P_err(r,varname,":");
              fclose(outputfile);
              ABORT;
            }
          fprintf(outputfile,"%g\n",value);
        }
      for (i=0; i<nspins-1; i++)
        { for (j=i+1; j<nspins; j++)
            { varname[0] = 'J';
              varname[1] = nuctable[i];
              varname[2] = nuctable[j];
              varname[3] = 0;
              if ( (r=P_getreal(GLOBAL,varname,&value,1)) )
                { P_err(r,varname,":");
                  fclose(outputfile);
                  ABORT;
                }
              fprintf(outputfile,"%g",value);
              if (j<nspins-1)
                fprintf(outputfile,",");
              else
                fprintf(outputfile,"\n");
            }
        }
      fclose(outputfile);
    }
  else
    { Werrprintf("cannot open file %s",filename);
      ABORT;
    }
  Wsettextdisplay("spins");
  Wclear_graphics();
  Wshow_text();
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spins.outdata");
#else 
  strcat(filename,"spins.outdata");
#endif 
  unlink(filename);
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spini.outpar");
#else 
  strcat(filename,"spini.outpar");
#endif 
  unlink(filename);
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spins.list");
#else 
  strcat(filename,"spins.list");
#endif 
  unlink(filename);
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spins.stat");
#else 
  strcat(filename,"spins.stat");
#endif 
  unlink(filename);
#ifdef UNIX
  sprintf(execstring,"spins %s %s > %s/spins.list",curexpdir,options,curexpdir);
#else 
  sprintf(execstring,"spins %s %s > %sspins.list",curexpdir,options,curexpdir);
#endif 
  ret = system(execstring);
  if ( (inputfile=fopen(filename,"r")) )
    { if (fscanf(inputfile,"%d\n",&error)!=1)
        { Werrprintf("problem reading from fitspec.stat,line 1");
          ABORT;
        }
      if (error)
        { Werrprintf("error during spin simulation");
          ABORT;
        }
      fclose(inputfile);
    }
  else
    { Werrprintf("problem running program 'spins'");
      ABORT;
    }
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spins.outdata");
#else 
  strcat(filename,"spins.outdata");
#endif 
  if ( (inputfile=fopen(filename,"r")) )
    { i = 1;
      P_setreal(GLOBAL,"cla",0.0,0);
      P_setreal(GLOBAL,"clfreq",0.0,0);
      P_setreal(GLOBAL,"clamp",0.0,0);
      while (fscanf(inputfile,"%f,%f;L%d",&f1,&f2,&a)==3)
        { int ch=' ';
          ch = fgetc(inputfile);
          while (ch==' ') ch = fgetc(inputfile);
	  if (ch=='*')
	  {
	      ch = fgetc(inputfile);
              while (ch==' ') ch = fgetc(inputfile);
	  }
	  /* ch should = '\n') */
	  P_setreal(GLOBAL,"cla",(double)a,i);
          P_setreal(GLOBAL,"clfreq",f1,i);
          P_setreal(GLOBAL,"clamp",f2,i);
          if (iterateflag || clindexsize == clasize)
 	    {
 	      found=0;
	      clindexvalue = 0;
	      j=-1;
	      /* Find cla in old cla , find corresponding clindex */
 	      while ( (j< clasize) && !found)
	        {
 	          j++;
	          found = (a == cla[j]);
	 	}
	      if (found)
		{
		  clindexvalue = clindex[j];
		}
              P_setreal(GLOBAL,"clindex",(double)clindexvalue,i);
	    }
          i++;
        }
      releaseAllWithId("spins");
      fclose(inputfile);
    }
  else
    { Werrprintf("problem running program 'spins'");
      ABORT;
    }
  strcpy(filename,curexpdir);
#ifdef UNIX
  strcat(filename,"/spins.stat");
#else 
  strcat(filename,"spins.stat");
#endif 
  unlink(filename);
  RETURN;
}

/***************************/
static int spins_iterate(char *options)
/***************************/
{
  FILE *inputfile;
  FILE *outputfile;
  FILE *savelafile;
  char  filename[MAXPATHL];
  char  savelaname[MAXPATHL];
  char  expdir[MAXPATHL];
  char iterate[256];
  int i,j,r;
  int clasize,clindexsize,slfreqsize;
  double clindex,cla,slfreq;
  char execstring[256];
  double value;
  double niter;
  float fvalue;
  char parname[6];

  if ( (r=P_getstring(GLOBAL,"iterate",iterate,1,255)) )
    { P_err(r,"iterate",":");
      ABORT;
    }
  else if (strlen( &iterate[ 0 ] ) < 1)
    {
      Werrprintf( "spins('iterate'): null value for 'iterate' parameter" );
      Wscrprintf( "Have you assigned spins to lines in the spectrum?\n" );
      ABORT;
    }
  strcpy(expdir,curexpdir);
#ifdef UNIX
  strcat(expdir,"/");
#endif 
  strcpy(filename,expdir);
  strcat(filename,"spini.inpar");
  if ( (outputfile=fopen(filename,"w+")) )
    {
      int equalflag = 0;

      i = 0;
      j = 0;
      while ((iterate[i]) && (i<256))
        {
          if (iterate[i]==',')
	    {
              parname[j] = '\0'; 
              if (P_getreal(GLOBAL,parname,&value,1))
                { P_err(r,parname,":");
      		  ABORT;
    		}
              fprintf(outputfile," %14.4f\n",value);
	      j = 0;
              equalflag = 0;
	    }
          else if (iterate[i]==' ')
            { Werrprintf("blank illegal in 'iterate' string");
              ABORT;
            }
          else
	    {
              if (iterate[i] == '=')
                 equalflag= 1;
              fprintf(outputfile,"%c",iterate[i]);
              if (!equalflag)
              {
	         parname[j] = iterate[i];
                 j++;
              }
  	    }
          i++;
        }
      parname[j] = '\0'; 
      if (P_getreal(GLOBAL,parname,&value,1))
        { P_err(r,parname,":");
          ABORT;
      	}
      fprintf(outputfile," %12.4f\n",value);
      fclose(outputfile);
      if (i==0)
        { Werrprintf("iteration parameter string 'iterate' empty");
          ABORT;
        }
    }
  else
    { Werrprintf("cannot open file %s",filename);
      ABORT;
    }
  strcpy(filename,expdir);
  strcpy(savelaname,expdir);
  strcat(filename,"spini.indata");
  strcat(savelaname,"spini.savela");
  if (getsize(GLOBAL,"clindex",&clindexsize))
    ABORT;
  if (getsize(GLOBAL,"cla",&clasize))
    ABORT;
  if (getsize(GLOBAL,"slfreq",&slfreqsize))
    ABORT;
  if (clindexsize != clasize)
    {
      Werrprintf("cannot save line assignments");
      ABORT;
    }
  if ( (savelafile=fopen(savelaname,"w+")) )
    { fprintf(savelafile,"%d %d\n",clasize,slfreqsize);
    }
  else
    { Werrprintf("cannot open file %s",savelaname);
      ABORT;
    }
  if ( (outputfile=fopen(filename,"w+")) )
    {
      j=0;
      for (i=1;i<=(int)clindexsize; i++)
        {
              if ( (r=P_getreal(GLOBAL,"clindex",&clindex,i)) ==0)
		{
		  if (clindex>0.5)
                    if (P_getreal(GLOBAL,"cla",&cla,i)==0)
                    {
                      if (P_getreal(GLOBAL,"slfreq",&slfreq,(int)clindex)==0)
		        {
                          fprintf(outputfile,"%d,%g\n",(int)cla,slfreq);
                          fprintf(savelafile,"%d %d %d\n",i,(int)cla,(int)clindex);
			  j++;
		        }
		      else
                	{ P_err(r,"slfreq",":");
		    	  ABORT;
			}
                    }
		}
		else
                {   P_err(r,"clindex",":");
		    ABORT;
		}
        }
      fclose(outputfile);
      fclose(savelafile);
      if (j<2)
        { Werrprintf("less than two lines assigned");
          ABORT;
        }
    }
  else
    { Werrprintf("cannot open file %s",filename);
      ABORT;
    }
  if (P_getreal(GLOBAL,"niter",&niter,1))
    niter = 0.0;
  if (niter==0.0)
    niter = 32.0;
  strcpy(filename,expdir);
  strcat(filename,"spini.outpar");
  unlink(filename);
  sprintf(execstring,"iterate %d %s",(int)niter,options);
  if (spins_calculate(execstring,ITERATION))
    ABORT;
  strcpy(filename,expdir);
  strcat(filename,"spini.outpar");
  if ( (inputfile=fopen(filename,"r")) )
    {
      while (fscanf(inputfile,"%s %f\n",parname,&fvalue) == 2)
        {
          if (P_setreal(GLOBAL,parname,(double)fvalue,1))
          { P_err(r,parname,"(setreal):");
            ABORT;
    	  }
        }
      fclose(inputfile);
    }
  strcpy(filename,expdir);
  strcat(filename,"spini.indata");
  unlink(filename);
  strcpy(filename,expdir);
  strcat(filename,"spins.stat");
  unlink(filename);
  spins_param("show","",0,NULL);
#ifdef VNMRJ
  appendJvarlist("spinsys");
  // writelineToVnmrJ("pnew 1","spinsys");
#endif
  RETURN;
}

/********************/
static int spins_display()
/********************/
{ double vs,svs;
  int r;
  char s[80];
  if ( (r=P_getreal(CURRENT,"vs",&vs,1)) )
    { P_err(r,"vs",":");
      ABORT;
    }
  if ( (r=P_getreal(GLOBAL,"svs",&svs,1)) )
    { P_err(r,"svs",":");
      ABORT;
    }
  sprintf(s,"dsp('spins.outdata',%g)\n",svs);
  execString(s);
  RETURN;
}

/********************/
static int spins_param(char *choice, char *value, int retc, char *retv[])
/********************/
{
  double dvalue = 0.0;

  if (retc>0) retv[0] = newString("");

  if (strcmp(choice, "setdmenu")==0)
  {
    strcpy(spins_dmenu_par,value);
#ifdef VNMRJ
    appendJvarlist("A");
    // writelineToVnmrJ("pnew 1","A");
#endif
  }
  else if (strcmp(choice, "getdmenu")==0)
  {
    if (retc>0) retv[0] = newString(spins_dmenu_par);
  }
  else if (strcmp(choice, "setjmenu")==0)
  {
    strcpy(spins_jmenu_par,value);
#ifdef VNMRJ
    appendJvarlist("JAB");
    // writelineToVnmrJ("pnew 1","JAB");
#endif
  }
  else if (strcmp(choice, "getjmenu")==0)
  {
    if (retc>0) retv[0] = newString(spins_jmenu_par);
  }
  else if (strcmp(choice, "setdvalue")==0)
  {
    dvalue = atof(value);
    P_setreal(GLOBAL,spins_dmenu_par,dvalue,1);
    spins_param("show","",retc,retv);
#ifdef VNMRJ
    appendJvarlist("B");
    appendJvarlist(spins_dmenu_par);
    // writelineToVnmrJ("pnew 2 B",spins_dmenu_par);
#endif
  }
  else if (strcmp(choice, "getdvalue")==0)
  {
    if (P_getreal(GLOBAL,spins_dmenu_par,&dvalue,1)==0)
    {
      if (retc>0) retv[0] = realString(dvalue);
    }
  }
  else if (strcmp(choice, "setjvalue")==0)
  {
    dvalue = atof(value);
    P_setreal(GLOBAL,spins_jmenu_par,dvalue,1);
    spins_param("show","",retc,retv);
#ifdef VNMRJ
    appendJvarlist("JAX");
    appendJvarlist(spins_jmenu_par);
    // writelineToVnmrJ("pnew 2 JAX",spins_jmenu_par);
#endif
  }
  else if (strcmp(choice, "getjvalue")==0)
  {
    if (P_getreal(GLOBAL,spins_jmenu_par,&dvalue,1)==0)
    {
      if (retc>0) retv[0] = realString(dvalue);
    }
  }
  else if (strcmp(choice, "show")==0) /* output dga information to file */
  { /* get and display chem shifts, coupling constants */
    FILE *fp;
    char *ch, param[4], parfile[MAXSTR];
    int j, r, cctitle=1;
    if (strlen(spins_parlist) > 0)
    {
      strcpy(parfile,curexpdir);
      strcat(parfile,"/spinj.par");
      unlink(parfile);
      if (! (fp=fopen(parfile,"w")))
        ABORT;
      fprintf(fp," CHEMICAL SHIFTS\n");
      ch = spins_parlist; j=0;
      while ( sscanf(ch,"%s",param) )
      {
        j++;
        if ((param[0] == '\0') || (j > 100))
          break;
        if ((param[0] == 'J') && (cctitle==1))
        {
          fprintf(fp,"COUPLING CONSTANTS\n");
          cctitle = 0;
        }
        if ((r=P_getreal(GLOBAL,param,&dvalue,1))==0)
        {
          if (param[0] == 'J')
            fprintf(fp,"%s    %11.4f\n",param,dvalue);
          else
            fprintf(fp,"%s    %13.2f\n",param,dvalue);
        }
        else
        {
          P_err(r,param,"(getreal):");
          fclose(fp);
          ABORT;
        }
        ch += (strlen(param)+1);
        param[3]='\0'; param[2]='\0'; param[1]='\0'; param[0]='\0';
      }
      fclose(fp);
#ifdef VNMRJ
      appendJvarlist("spinsys");
      // writelineToVnmrJ("pnew 1","spinsys");
#endif
    }
  }
  else
    Werrprintf("Usage: spins('param','show') spins('param','getdmenu|getjmenu|getdvalue|getjvalue'):$val spins('param','setdmenu|setjmenu|setdvalue|setjvalue',$val)");
  RETURN;
}

/****************************/
int spins(int argc, char *argv[], int retc, char *retv[])
/****************************/
{ 

#ifdef CLOCKTIME
  /* Turn on a clocktime timer */
  (void)start_timer ( spins_timer_no );
#endif 

  Wturnoff_buttons();
  D_allrelease();
  if (argc<2)
    { /* calculate energy levels and frequencies */
      return (spins_calculate("",0));
    }
  else if (strcmp(argv[1],"init")==0)
    { /* create all parameters */
      if (argc!=2)
        { Werrprintf("usage - spins('init')");
          ABORT;
        }
      return (spins_init());
    }
  else if (strcmp(argv[1],"system")==0)
    { /* define spin system */
      if (argc!=3)
        { Werrprintf("usage - spins('system','abc...')");
          ABORT;
        }
      return (spins_system(argv[2]));
    }
  else if (strcmp(argv[1],"calculate")==0)
    { /* calculate energy levels and frequencies */
      if (argc>2)
        return (spins_calculate(argv[2],0));
      else
        return (spins_calculate("",0));
    }
  else if (strcmp(argv[1],"iterate")==0)
    { /* iterate spin system parameters */
      if (argc>2)
        spins_iterate(argv[2]);
      else
        spins_iterate("");
    }
  else if (strcmp(argv[1],"display")==0)
    { /* calculate and display spectrum */
      spins_display();
    }
  else if (strcmp(argv[1],"param")==0)
    { /* get and set spin system parameters */
      if (argc>3)
        return (spins_param(argv[2],argv[3],retc,retv));
      else if (argc>2)
        return (spins_param(argv[2],"",retc,retv));
      else
        { Werrprintf("Usage: spins('param','show') spins('param','getdmenu|getjmenu|getdvalue|getjvalue'):$val spins('param','setdmenu|setjmenu|setdvalue|setjvalue',$val)");
          ABORT;
        }
    }
  else
    { Werrprintf(
        "usage - spins('key'), where key=system,calculate,iterate,display,param");
      ABORT;
    }

#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  (void)stop_timer ( spins_timer_no );
#endif 

  RETURN;
}

int getsize(int tree, char *name, int *Size)
{   vInfo info;
    int tmp;

    if ( (tmp = P_getVarInfo(tree,name,&info)) )
    {   P_err(tmp,name,":");
        ABORT;
    }
    else
       *Size = info.size;
    return(tmp);
}

/*
 * spa - spins assign  
 *	assign a calculated transition to an expermental line.
 *      usage: spa(calculated transition index, experimental line index)");
 *	spa(calculated transition index,0) unassigns an experimental line.
 */
int spa(int argc, char *argv[], int retc, char *retv[])
/****************************/
{ 
  int cindex, sindex;
  int c_size, s_size, clindex_size;
  int i, r;
  char str[32];
  char textcmd[20];
  char *tdisplay;

  tdisplay = newString(Wgettextdisplay(textcmd,20));
  if (argc == 3)
  {
    cindex = atoi(argv[1]);
    sindex = atoi(argv[2]);
    if (getsize(GLOBAL,"slfreq",&s_size))
        ABORT;
    if (getsize(GLOBAL,"clfreq",&c_size))
        ABORT;
    if (getsize(GLOBAL,"clindex",&clindex_size))
        ABORT;
    if (clindex_size < c_size)
    {
        for (i=clindex_size+1; i<=cindex; i++)
            if ( (r = P_setreal(GLOBAL,"clindex",(double) 0.0,i)) )
            {   P_err(r,"clindex",":");
                ABORT;
            }
    }
    if (sindex > s_size  || sindex < 0 || cindex <= 0 || cindex>c_size)
    {
        if (sindex > s_size  || sindex < 0)
	    strcpy(str,"Experimental line");
        else 
	    strcpy(str,"Calculated transition");
        Werrprintf("%s index out of range",str);
        ABORT;
    }
    else
        if (sindex == 0)
        {
            if ( (r = P_setreal(GLOBAL,"clindex",(double) 0.0,cindex)) )
            {   P_err(r,"clindex",":");
                ABORT;
            }
	    else
	    {   rundla(tdisplay);
                RETURN;
	    }
        }
  }
  else 
  { Werrprintf(
      "usage: spa(calculated transition index, experimental line index)");
    ABORT;
  }

  if ( (r = P_setreal(GLOBAL,"clindex",(double) (sindex), cindex)) )
  { P_err(r,"clindex",":");
    ABORT;
  }
    
  rundla(tdisplay);
  RETURN;
}

int rundla(char *tdisplay)
{
  if (strcmp(tdisplay,"dla") == 0)
  {
      execString("dla\n");
  }
  RETURN;
}

/*
 * dlalong - makes a table of spin simulation assignments
 * dlalong('cat') also displays the file in the text window
 * dlalong('print') prints the file
 * dlalong('returnla') returns line assignments from
 *	  the file 'spini.savela'
 * dlalong('short') same output as "dla" to file, text window
 *
 * For now though, it accepts no arguments; always
 * assumes the cat option and none of the others.
 */

#define DLALONG_DEFAULT  { printflag = 0; catflag = 1; returnla = 0; }

int dlalong(int argc, char *argv[], int retc, char *retv[])
{
    FILE           *outputfile;
    char            filename[MAXPATHL];
    char            s[120];
    int             found, i, j, r;
    int             printflag = 0;
    int             catflag = 0;
    int             returnla = 0;
    int             shortflag = 0;
    int             clindexsize, clfreqsize, slfreqsize;
    int            *clindex,	*saveclindex;
    int            *cla,	*savecla;
    int             claval,	 clindexval;
    int             indx;
    double          cladoub;
    double         *clfreq,	*slfreq,	*clamp;
    double         *saveclfreq,	*saveslfreq,    *saveclamp;
    int ret __attribute__((unused));

    if (argc > 1) {
        if (strcmp( argv[ 1 ], "returnla" ) == 0)
	  returnla = 1;
        else
          DLALONG_DEFAULT
        if (strcmp( argv[ 1 ], "short" ) == 0)
	  shortflag = 1;
    }
    else
      DLALONG_DEFAULT

/* Later perhaps we will remove the print stuff.
   For now we keep it though.				*/

    strcpy(filename,curexpdir);
#ifdef UNIX
    strcat(filename,"/");
#endif 
    if (returnla)
    {
        strcat(filename,"spini.savela");
        if (!(outputfile=fopen(filename,"r")))
        {   Werrprintf("cannot open file %s",filename);
            ABORT;
        }
    }
    else
    {
        strcat(filename,"spini.la");
        if ( (outputfile=fopen(filename,"w")) )
        {
          if (shortflag)
	  {
            fprintf(outputfile,
	    "EXPERIMENTAL LINES      LINES CALCULATED\n");
            fprintf(outputfile,
	    " i      slfreq          i clindex  clfreq\n");
	  }
	  else
	  {
            fprintf(outputfile,
	    "                     Table of Frequency Assignments\n\n");
            fprintf(outputfile,
	    "    slfreq[i]     frequency of experimental line\n");
            fprintf(outputfile,
            "    clindex[i]    index to an experimental line for an assigned transition\n");
            fprintf(outputfile,
	    "    clamp[i]      amplitude of calculated transition\n");
            fprintf(outputfile,
	    "    clfreq[i]     frequency of calculated transition\n\n");
            fprintf(outputfile,
            "  i    slfreq[i]       i clindex  clamp[i] clfreq[i]   assigned frequency\n");
	  }
        }
        else
        {   Werrprintf("cannot open file %s",filename);
            ABORT;
        }
    }
    if (getsize(GLOBAL,"clindex",&clindexsize))
        ABORT;
    if (getsize(GLOBAL,"slfreq",&slfreqsize))
        ABORT;
    if (getsize(GLOBAL,"clfreq",&clfreqsize))
        ABORT;
    if (clindexsize>clfreqsize)
      clindexsize=clfreqsize; /*additional values of clindex would be meaningless */
    clindex  = (int *)allocateWithId(clfreqsize*sizeof(int),"dlalong");
    cla      = (int *)allocateWithId(clfreqsize*sizeof(int),"dlalong");
    clfreq   = (double *)allocateWithId(clfreqsize*sizeof(double),"dlalong");
    clamp    = (double *)allocateWithId(clfreqsize*sizeof(double),"dlalong");
    slfreq   = (double *)allocateWithId(slfreqsize*sizeof(double),"dlalong");

    saveclindex = clindex;
    savecla	= cla;
    saveclfreq  = clfreq;
    saveclamp   = clamp;
    saveslfreq  = slfreq;
    for (i=1; i<=clindexsize; i++)
    {
        if ( (r = P_getreal(GLOBAL,"clindex",&cladoub,i)) )
        {   P_err(r,"clindex",":");
            releaseAllWithId("dlalong");
	    fclose(outputfile);
            ABORT;
        }
        else
	{
            *clindex = (int) (cladoub + 0.1);
	    clindex++;
	}
    }
    if (returnla && clindexsize <clfreqsize)
        for (i=clindexsize+1; i<=clfreqsize; i++)
	{
            if ( (r = P_setreal(GLOBAL,"clindex",0.0,i)) )
            {   P_err(r,"clindex",":");
                releaseAllWithId("dlalong");
	        fclose(outputfile);
                ABORT;
            }
            *clindex = 0;
	    clindex++;
	}
    for (i=1; i<=clfreqsize; i++)
    {
        if ( (r = P_getreal(GLOBAL,"cla",&cladoub,i)) )
        {   P_err(r,"cla",":");
            releaseAllWithId("dlalong");
	    fclose(outputfile);
            ABORT;
        }
        else
	{
            *cla = (int) (cladoub + 0.1);
	    cla++;
	}
    }
    for (i=1; i<=clfreqsize; i++)
    {
        if ( (r = P_getreal(GLOBAL,"clfreq",clfreq,i)) )
        {   P_err(r,"clfreq",":");
            releaseAllWithId("dlalong");
	    fclose(outputfile);
            ABORT;
        }
        else
	{
	    clfreq++;
        }
        if ( (r = P_getreal(GLOBAL,"clamp",clamp,i)) )
        {   P_err(r,"clamp",":");
            releaseAllWithId("dlalong");
	    fclose(outputfile);
            ABORT;
        }
        else
	{
	    clamp++;
        }
    }
    for (i=1; i<=slfreqsize; i++)
    {
        if ( (r = P_getreal(GLOBAL,"slfreq",slfreq,i)) )
        {   P_err(r,"slfreq",":");
            releaseAllWithId("dlalong");
	    fclose(outputfile);
            ABORT;
        }
        else
	{
	    slfreq++;
	}
    }
    clindex  = saveclindex;
    cla	     = savecla;
    clfreq   = saveclfreq;
    clamp    = saveclamp;
    slfreq   = saveslfreq;


    if (returnla)
    {
	    ret = fscanf(outputfile,"%d %d\n",&claval,&clindexval);
	    while (fscanf(outputfile,"%d %d %d\n",&indx,&claval,&clindexval)==3)
	    {
	        j=1;
	        while (claval != cla[j-1] && j<clfreqsize) j++;
    	        if (claval==cla[j-1])
	        {
  		    if ( (r = P_setreal(GLOBAL,"clindex",(double) clindexval,j)) )
  		    {   P_err(r,"CLindex",":");
                        releaseAllWithId("dlalong");
	     	        fclose(outputfile);
    			ABORT;
  		    }
	        }
	        else
		{
		    Werrprintf("CANNOT return line assignments");
                    releaseAllWithId("dlalong");
	     	    fclose(outputfile);
            	    ABORT;
		}
 	    }
	    fclose(outputfile);
            releaseAllWithId("dlalong");
            RETURN;
    }
    i=-1;
    do
    {
      if (shortflag)
      {
        i++;
        found = (clindex[i] > 0);
        if (found)
         if (i < slfreqsize) 
            fprintf(outputfile,"%2d   %9.2f    %7d  %3d  %9.2f\n",
	       i+1,slfreq[i],i+1,clindex[i],clfreq[i]);
         else
            fprintf(outputfile,"                  %7d  %3d  %9.2f\n",
	       i+1,clindex[i],clfreq[i]);
        else
         if (i < slfreqsize) 
            fprintf(outputfile,"%2d   %9.2f    %7d       %9.2f\n",
	       i+1,slfreq[i],i+1,clfreq[i]);
         else
            fprintf(outputfile,"                  %7d       %9.2f\n",
	       i+1,clfreq[i]);
      }
      else
      {
        i++;
        found = (clindex[i] > 0);
        if (found)
         if (i < slfreqsize) 
            fprintf(outputfile,"%3d    %9.2f %7d %7d %9.2f %9.2f %9.2f\n",
	       i+1,slfreq[i],i+1,clindex[i],clamp[i],clfreq[i],slfreq[clindex[i]-1]);
         else
            fprintf(outputfile,"                 %7d %7d %9.2f %9.2f %9.2f\n",
	       i+1,clindex[i],clamp[i],clfreq[i],slfreq[clindex[i]-1]);
        else
         if (i < slfreqsize) 
            fprintf(outputfile,"%3d    %9.2f %7d         %9.2f %9.2f \n",
	       i+1,slfreq[i],i+1,clamp[i],clfreq[i]);
         else
            fprintf(outputfile,"                 %7d         %9.2f %9.2f \n",
	       i+1,clamp[i],clfreq[i]);
      }
    }
    while (i<clfreqsize - 1);
    if (slfreqsize>clfreqsize) do
    {
      if (shortflag)
      {
        i++;
        fprintf(outputfile,"%2d   %9.2f\n", i+1,slfreq[i]);
      }
      else
      {
        i++;
        fprintf(outputfile,"%3d    %9.2f \n", i+1,slfreq[i]);
      }
    }
    while (i<slfreqsize - 1);
   

    releaseAllWithId("dlalong");
    fclose(outputfile);

    if (printflag)
    {
        strcpy(s,"printon cat('");
        strcat(s,filename);
        strcat(s,"') printoff\n");
        execString(s);
    }
    else if (catflag)
    {
	Wclear_text();
        strcpy(s,"cat('");
        strcat(s,filename);
        strcat(s,"')\n");
        execString(s);
    }
#ifdef VNMRJ
    appendJvarlist("slfreq");
    // writelineToVnmrJ("pnew 1","slfreq");
#endif
    RETURN;
}
