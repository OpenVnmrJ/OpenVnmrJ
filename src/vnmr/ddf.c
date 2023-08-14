/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************************************/
/* ddf.c  -  display data file                                         */
/* command:                                                            */
/*   ddf <blocknumber <trace <firstelement | 'max'>>><:max>            */
/*     display data file                                               */
/*     blocknumber     starting block number for display               */
/*     trace           trace for display                               */
/*     firstelement    starting element number for display             */
/*     'max'           keyword to find maximum data value              */
/*                       if max return argument given, do not display  */
/*   ddfp(blocknumber <trace <firstelement>>>                          */
/*     display phase file                                              */
/*   ddff(blocknumber <trace <firstelement>>>                          */
/*     display fid file                                                */
/*   noise                                                             */
/*     measure the rms noise level of a fid                            */
/*   averag                                                            */
/*     calculate the average and standard deviation of a set of numbers*/
/*   ernst                                                             */       
/*     calculate the ernst angle                                       */
/*   pw/p1                                                             */
/*     calculate the flip time                                         */
/*   ln/sin/cos/tan/atan/exp(value <'Rn'>)<:Rn>                        */
/*     math functions                                                  */
/*     Rn is parameter where value is stored                           */ 
/*   ilfid                                                             */
/*     combines multiple fids into a single fid (nf>1)                 */
/*   quadtt                                                            */
/*     quadrature test of receiver channels in widleline systems       */
/*   makefid(input file [,element number, format ] )                   */
/*     write FID element using numeric input from a text file          */
/*   writefid(output file, [, element number ] )                       */
/*     write numeric text file using an element from the current FID   */
/*     resulting text file suitable as input to makefid                */
/***********************************************************************/

#include "vnmrsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>              /*  to define iswspace macro */
#include <unistd.h>             /*  bits for 'access' in SVR4 */
#include <string.h>
#include <errno.h>

#include "data.h"
#include "group.h"
#include "tools.h"
#include "variables.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"
#include "vfilesys.h"

#define COMPLETE        0
#define ERROR           1

extern int debug1;
extern int Bnmr;

extern void Wturnoff_buttons();
extern FILE *popen_call(char *cmdstr, char *mode);
extern int  pclose_call(FILE *pfile);
extern int  D_fidversion();

static int count_lines(char *fn_addr );
static void make_std_bhead(dblockhead *bh_ref, short b_status, short b_index );
static void make_empty_fhead(dfilehead *fh_ref );
static int makefid_args(int argc, char *argv[], int *element_addr, int *format_addr,
                    int *addFlag, int *phaseInvert, int *revFlag, int *indexOffset );
static int report_data_format_inconsistent(char *cmd_name, int old_format );
static int makefid_getfhead(char *cmd_name, dfilehead *fh_ref, int *update_fh_ref, int force );
static int load_ascii_numbers(char *cmd_name, char *fn_addr, void *mem_buffer,
                              int max_lines, int cur_format, int revFlag );
static int writefid_args(int argc, char *argv[], int *element_addr );
static int writefid_getfhead(char *cmd_name, dfilehead *fh_ref );
static int write_numbers_ascii(char *cmd_name, char *fn_addr, void *mem_buffer,
                               int np, int cur_format );
/*----------------------------------------------------------------------
|
|       ddf
|       Display data file
|
+---------------------------------------------------------------------*/
int ddf(int argc, char *argv[], int retc, char *retv[])
{
  int i,e,blocknumber,trace,getmax=0;
  int li,firstelement=0,lastelement;
  int doPeek = 0;
  dfilehead dhd;
  dpointers block;
  dblockhead    *tmpbhead;
  char outfidpath[MAXPATH], dataname[10];
  int f;
  int btype = 0; /* This flags that a vers_id is requested */
  int jtype = 0; /* This asks if vers_id is S_JEOL */
  int qtype = 0; /* This asks if vers_id is S_QONE */
  int mtype = 0; /* This asks if vers_id is S_MAG */
  int otype = 0; /* This asks if vers_id is S_OX */
  int vtype = 0; /* This asks if vers_id is S_VAR */
  int ctype = 0; /* This asks if vers_id is S_MAKEFID */
  int itype = 0; /* This asks for vers_id */
  int atype = 0; /* This asks for vers_id return */

  if ((argc >= 2) && ! strcmp(argv[1],"B") )
  {
    btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"J") )
  {
    jtype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"Q") )
  {
    qtype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"C") )
  {
    ctype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"M") )
  {
    mtype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"O") )
  {
    otype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"V") )
  {
    vtype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"?") )
  {
    atype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else if ((argc >= 2) && ! strcmp(argv[1],"vers_id") )
  {
    itype = btype=1;
    if (argc == 3)
    {
      strcpy(outfidpath, argv[2]);
      btype = 2;     
    }
  }
  else
  {
     Wturnoff_buttons();
     D_allrelease();
     Wshow_text();
     if (argc<2)
       { printf("usage - ddf<p>(block<,trace<,firstelement | \'max\'>>)<:max>\n");
         blocknumber = 1;
       }
     else
       { sscanf(argv[1],"%d",&blocknumber);
         blocknumber--;
       }
     if (argc>=3)
       { sscanf(argv[2],"%d",&trace);
         trace--;
       }
     else trace = 0;
     if (argc>=4)
       {
         if (strcmp(argv[3],"max")==0)
           getmax=1;
         else
         {
           sscanf(argv[3],"%d",&firstelement);
           firstelement--;
         }
       }
     else firstelement = 0;
  }
  if ((argc == 4) && !getmax && (retc >= 1) )
      doPeek = 1;

  if (argv[0][3]=='p') f=D_PHASFILE;
  else if (argv[0][3]=='f') { f=D_USERFILE; D_close(D_USERFILE); }
  else f=D_DATAFILE;

  if ( (e = D_gethead(f,&dhd)) )
    {if (e==D_NOTOPEN)
       {
         if ( (btype != 2) &&  (e = D_getfilepath(f, outfidpath, curexpdir)) )
         {
            D_error(e);
            if ( (getmax || btype) && retc>0)
            {
              retv[0] = realString( -1.0 );
              return COMPLETE;
            }
            else
              return ERROR;
         }

         e = D_open(f,outfidpath,&dhd); /* open the file */
       }
      if (e)
      { D_error(e);
        if ( (getmax || btype) && retc>0)
        {
          retv[0] = realString( -1.0 );
          return COMPLETE;
        }
        else
          return ERROR;
      }
    }

  if (btype)
  {
     if (retc)
     {
         int id;

         id = dhd.vers_id & P_VENDOR_ID;
         if (itype)
            retv[0] = intString( dhd.vers_id );
         else if (jtype)
            retv[0] = intString( (id ==  S_JEOL) ? 1 : 0 );
         else if (qtype)
            retv[0] = intString( (id ==  S_QONE) ? 1 : 0 );
         else if (ctype)
            retv[0] = intString( (id ==  S_MAKEFID) ? 1 : 0 );
         else if (mtype)
            retv[0] = intString( (id ==  S_MAG) ? 1 : 0 );
         else if (otype)
            retv[0] = intString( (id ==  S_OX) ? 1 : 0 );
         else if (vtype)
            retv[0] = intString( (id ==  S_VAR) ? 1 : 0 );
         else if (!atype)
            retv[0] = intString( (id ==  S_BRU) ? 1 : 0 );
         else
         {
            char type[2];

            if (id == S_JEOL)
               type[0] = 'J';
            else if (id == S_QONE)
               type[0] = 'Q';
            else if (id == S_MAKEFID)
               type[0] = 'C';
            else if (id == S_MAG)
               type[0] = 'M';
            else if (id == S_OX)
               type[0] = 'O';
            else if (id == S_VAR)
               type[0] = 'V';
            else // if (id == S_BRU)
               type[0] = 'B';
            type[1] = '\0';
            retv[0] = newString(type);
         }
     }

     D_close(D_USERFILE);
     return COMPLETE;
  }
/****************************
*  Display the file header  *
****************************/

  switch (f)
  {
     case D_DATAFILE:   strcpy(dataname, "DATA");  break;
     case D_PHASFILE:   strcpy(dataname, "PHASE"); break;
     case D_USERFILE:   strcpy(dataname, "FID");   break;
     default:           strcpy(dataname, "DATA");  break;
  }
  
  if (!getmax && !doPeek)
  {
     Wscrprintf("\n%s FILE HEADER:\n", dataname);
     Wscrprintf("  status  = %8x,  nbheaders       = %8x\n", dhd.status,
        dhd.nbheaders);
     Wscrprintf("  nblocks = %8d,  bytes per block = %8d\n", dhd.nblocks,
        dhd.bbytes);
     Wscrprintf("  ntraces = %8d,  bytes per trace = %8d\n", dhd.ntraces,
        dhd.tbytes);
     Wscrprintf("  npoints = %8d,  bytes per point = %8d\n", dhd.np,
        dhd.ebytes);
     Wscrprintf("  vers_id = %8d\n", dhd.vers_id);
  }

/********************************************
*  Check requested block and trace numbers  *
********************************************/

  if (argc < 2)                 /* no block display requested */
  {
     if (getmax && retc>0)
       retv[0] = realString( -1.0 );
     return COMPLETE;
  }

  if ((blocknumber < 0) || (blocknumber >= dhd.nblocks))
  {
     Werrprintf("illegal block number");
     if (getmax && retc>0)
     {
       retv[0] = realString( -1.0 );
       return COMPLETE;
     }
     else
       return ERROR;
  }

  if ((trace < 0) || (trace >= dhd.ntraces))
  {
     Werrprintf("illegal trace number");
     if (getmax && retc>0)
     {
       retv[0] = realString( -1.0 );
       return COMPLETE;
     }
     else
       return ERROR;
  }

/******************************
*  Display the block headers  *
******************************/

  if ( (e = D_getbuf(f, dhd.nblocks,  blocknumber, &block)) )
  {
     D_error(e);
     if (getmax && retc>0)
     {
       retv[0] = realString( -1.0 );
       return COMPLETE;
     }
     else
       return ERROR;
  }

  i = 0;
  if (!getmax && !doPeek)
  {
     while (i < (dhd.nbheaders & NBMASK))
     {
        tmpbhead = block.head + i;
        Wscrprintf("\n%s BLOCK HEADER %1d:\n", dataname, i+1);
        Wscrprintf("  index   = %8d,  status          = %8x,  mode = %8x\n",
           tmpbhead->index, tmpbhead->status, tmpbhead->mode);
        Wscrprintf("  scale   = %8d,  ctcount         = %8d\n",
           tmpbhead->scale, tmpbhead->ctcount);
        Wscrprintf("  lpval   = %8g,  rpval           = %8g\n",
           tmpbhead->lpval, tmpbhead->rpval);
        Wscrprintf("  lvl     = %8g,  tlt             = %8g\n",
           tmpbhead->lvl, tmpbhead->tlt);
        i += 1;
     }
  }

/***************************
*  Display the block data  *
***************************/

  lastelement = firstelement + 36;
  if (lastelement > dhd.np)
     lastelement = dhd.np;

  if (!getmax && !doPeek)
  {
     for (li = firstelement; li<lastelement; li++)
       { if ((6*((li-firstelement)/6)) == (li-firstelement))
           Wscrprintf("\n[%6d] ",li+1);
         if (dhd.ebytes==2)
           Wscrprintf(" %10d",((short *)block.data)[li + dhd.np * trace]);
         else if ((block.head->status & S_FLOAT) == 0)
           Wscrprintf(" %10d",((int *)block.data)[li + dhd.np * trace]);
         else
           Wscrprintf(" %9g",((float *)block.data)[li + dhd.np * trace]);
       }
     Wscrprintf("\n");
  }
  else if (getmax) /* get maximum data point */
  {
     float tmpval, max=0.0;
     int shift;
     for (i=0; i < dhd.np * dhd.ntraces; i++)
     {
        if (dhd.ebytes==2)
           tmpval = fabs((float) ((short *)block.data)[i]);
        else if ((block.head->status & S_FLOAT) == 0)
           tmpval = fabs((float) ((int *)block.data)[i]);
        else
           tmpval = fabs((float) ((float *)block.data)[i]);

        if (tmpval > max) max = tmpval;
     }
     shift = 1 << abs(block.head->scale);
     if (block.head->scale < 0)
        tmpval = 1.0/(float)(shift);
     else
        tmpval = (float) shift;
     tmpval /= (float)(block.head->ctcount);
     if (retc > 0)
     {
        retv[0] = realString( max );
        if (retc > 1)
           retv[1] = realString( max*tmpval );
     }     
     else
     {
        Wscrprintf("%s FILE maximum absolute value = %g\n",dataname,max);
        if ( block.head->scale || (block.head->ctcount > 1) )
           Wscrprintf("%s FILE maximum scaled absolute value = %g\n",dataname,max*tmpval);
     }
  }
  else /* peek data point */
  {
     double val;
     int index;
     int args = 0;
      
     index = firstelement + dhd.np * trace; 
     while (args < retc) 
     {
        if (dhd.ebytes==2)
           val = (double) ((short *)block.data)[index];
        else if ((block.head->status & S_FLOAT) == 0)
           val = (double) ((int *)block.data)[index];
        else
           val = (double) ((float *)block.data)[index];
        retv[args] = realString( val );
        index++;
        args++;
     }
  }

  if ( (e = D_release(f, blocknumber)) )
  {
     D_error(e);
     if (getmax && retc>0)
       return COMPLETE;
     else
       return ERROR;
  }

  return COMPLETE;
}

/*----------------------------------------------------------------------
|
|       getdatadim
|       Get dimension of data file
|
+---------------------------------------------------------------------*/
int getdatadim(int argc, char *argv[], int retc, char *retv[])
{
  int e;
  dfilehead dhd;
  char outfidpath[MAXPATH];
  int f = D_USERFILE;
  int ret = 0;

  D_allrelease(); /* necessary? */

  if (argc > 1)
    if (argv[1][0]=='s') /* spectrum */
      f = D_DATAFILE;
  if (f==D_USERFILE)
    D_close(D_USERFILE); /* necessary? */

  if ( (e = D_gethead(f,&dhd)) )
  {
    if (e==D_NOTOPEN)
    {
      if ( (e = D_getfilepath(f, outfidpath, curexpdir)) )
      {
        ret = -1;
      }
      e = D_open(f,outfidpath,&dhd); /* open the file */
    }
    if (e)
    {
      ret = -2;
    }
  }
  if (ret > -1)
    ret = dhd.nblocks * dhd.ntraces;
  else
    ret = 0;

  if (retc > 0)
    retv[0] = intString( ret );
  else
  {
    if (f==D_USERFILE)
      Winfoprintf("Number of fid's = %d\n",ret);
    else
      Winfoprintf("Number of spectra = %d\n",ret);
  }

  return COMPLETE;
}

/*----------------------------------------------------------------------
|
|       noise (<excess noise,<last measured noise,<block<,trace>>>>)
|
|         Measure the rms nose level of a fid.
|         If two input arguments are added, they are used             
|           to calculate the noise figure - the first input         
|           argument is the excess noise, the second is the        
|           last measured mean square noise.               
|         Output to parameters:
|           real/imaginary dc
|           real/imaginary rms noise
|           average rms noise
|           channel imbalance (%)
|           db noise
|           noise figure (if input arguments)
|      
|
+---------------------------------------------------------------------*/
int noise(int argc, char *argv[], int retc, char *retv[])
{
  int e;
  int blocknumber = 0;
  int trace = 0;
  int li;
  dfilehead dhd;
  dpointers block;
  char outfidpath[MAXPATH];
  int calcflag = 0;
  int f;
  float dataval;
  float resum = 0.0;
  float imsum = 0.0;
  float exnoise = 0.0;
  float reav,imav;
  float rems,imms;
  float repoint,impoint;
  float db,ms,ms0,ms1,ms2,nf;
  float rerms,imrms,avrms;
  float imbalance;

  Wturnoff_buttons();
  D_allrelease();
  Wshow_text();
  if (argc<2)
  {/* printf("Usage - noise(<excess noise,<last measured noise,<block>>>)\n\n");
   */
  }
  else
  {   sscanf(argv[1],"%f",&exnoise);
  }
  if (argc >2)
      sscanf(argv[2],"%f",&ms0);
  if (argc >3)
  {   sscanf(argv[3],"%d",&blocknumber);
      blocknumber--;
  }
  if (argc >4)
  {   sscanf(argv[4],"%d",&trace);
      trace--;
  }

  f=D_USERFILE;
  D_close(D_USERFILE); 

  if ( (e = D_gethead(f,&dhd)) )
    {if (e==D_NOTOPEN)
       {
         if ( (e = D_getfilepath(D_USERFILE, outfidpath, curexpdir)) )
         {
            D_error(e);
            return 1;
         }

         e = D_open(f,outfidpath,&dhd); /* open the file */
       }
      if (e) { D_error(e); return 1; }
    }

  if ((blocknumber<0) || (blocknumber>=dhd.nblocks))
    {
      Werrprintf("illegal block number");
      return 1;
    }
  if ((trace<0) || (trace>=dhd.ntraces))
    {
      Werrprintf("illegal trace number");
      return 1;
    }

  if ( (e = D_getbuf(f, dhd.nblocks, blocknumber, &block)) )
  {
     D_error(e);
     return ERROR;
  }

  for (li = 0; li<dhd.np; li++) 
  {
      if (dhd.ebytes==2)
          dataval = ((short *)block.data)[li + dhd.np * trace];
      else if ((block.head->status & S_FLOAT) == 0)
          dataval = ((int *)block.data)[li + dhd.np * trace];
      else
          dataval = ((float *)block.data)[li + dhd.np * trace];

      if ((li%2) == 0) resum += dataval;
      else imsum += dataval;
  }
  reav = 2.0*resum/dhd.np;
  imav = 2.0*imsum/dhd.np;
  resum = 0.0;
  imsum = 0.0;
  for (li = 0; li<dhd.np; li++) 
  {
      if (dhd.ebytes==2)
          dataval = ((short *)block.data)[li + dhd.np * trace];
      else if ((block.head->status & S_FLOAT) == 0)
          dataval = ((int *)block.data)[li + dhd.np * trace];
      else
          dataval = ((float *)block.data)[li + dhd.np * trace];

      if ((li%2) == 0) 
      {
          repoint = dataval-reav;
          resum += repoint*repoint;
      }
      else
      {
          impoint = dataval-imav;
          imsum += impoint*impoint;
      }
  }
  rems = 2.0*resum/dhd.np;
  imms = 2.0*imsum/dhd.np;

  ms = (rems + imms)/2.0;
  db = 10.0*log(ms)/2.302585;
  rerms = sqrt(rems);
  imrms = sqrt(imms);
  avrms = sqrt(ms);
  imbalance = ((rerms/imrms) - 1.0)*100.0;
  Wscrprintf("\n                    CHANNEL A   CHANNEL B     AVERAGE\n");
  Wscrprintf("DC Offset        %12.2f%12.2f\n",reav,imav);
  Wscrprintf("Mean Square Noise%12.4f%12.4f%12.4f\n",rems,imms,ms);
  Wscrprintf("RMS Noise        %12.2f%12.2f%12.2f\n\n",rerms,imrms,avrms);
  Wscrprintf("%-4.2f%% Imbalance Between Channels\n",imbalance);
  Wscrprintf("%-4.2f DB\n",db);

  if (retc >= 2  && retc <10)
  {   
      retv[0] = realString(reav);
      retv[1] = realString(imav);
      if (retc>=3) retv[2] = realString(rerms);
      if (retc>=4) retv[3] = realString(imrms);
      if (retc>=5) retv[4] = realString(avrms);
      if (retc>=6) retv[5] = realString(imbalance);
      if (retc>=7) retv[6] = realString(db);
  }
  calcflag = (argc>2 && exnoise != 0.0 && ms0 != 0.0);
  if (calcflag)
  {
      if (ms0<ms)
      {   ms1 = ms0;
          ms2 = ms;
      }
      else
      {   ms1 = ms;
          ms2 = ms0;
      }
      nf = exnoise -10.0*log((ms2/ms1) - 1.0)/2.302585;
      Wscrprintf("\nExcess Noise    = %-4.2f\n",exnoise);
      Wscrprintf("50 0hm Noise    = %-8.4f\n",ms1);
      Wscrprintf("Increased Noise = %-8.4f\n",ms2);
      Wscrprintf("Noise Figure    = %-4.2f\n",nf);
      if (retc >= 8  && retc <10) retv[7] = realString(nf);
  }
  if ( (e = D_release(f,blocknumber)) )
  { D_error(e); return 1; }
  else RETURN;
}

/*----------------------------------------------------------------------
|
|       averag
|       Place a series of numbers in the input parameters and produce 
|               the average and the standard deviation.
|       Output to parameters:
|           r1: average
|           r2: standard deviation
|           r3: argument count
|           r4: sum
|           r5: sum of squares
|
+---------------------------------------------------------------------*/
int averag(int argc, char *argv[], int retc, char *retv[])
{
    double sum = 0.0;
    double sumsquares = 0.0;
    double average; 
    double stddev; 
    double v; 
    int i;

    argc--;
    for (i=1; i<=argc; ++i)
    { 
        v = stringReal(argv[i]);
        sum += v;
        sumsquares += v*v;
    }
    if (argc < 2)
    {   Werrprintf(
          "averag requires at least 2 input numbers, e.g averag(3.2,3.1)");
        ABORT;
    }
    else
    {
        average = sum/((double) argc);
        stddev = sqrt(fabs(sumsquares - sum*sum/argc)/(argc - 1));
        if (debug1) Wscrprintf("argc=%d\n",argc);
        if (retc >= 1  && retc <10)
        {   
            retv[0] = realString(average);
            if (retc>=2) retv[1] = realString(stddev);
            if (retc>=3) retv[2] = realString((double) argc);
            if (retc>=4) retv[3] = realString(sum);
            if (retc>=5) retv[4] = realString(sumsquares);
        }       
        else
        {
           Winfoprintf("average=%f, stddev=%f",average,stddev);
        }       
    }
    RETURN;

}       

/*----------------------------------------------------------------------
|
|       datafit
|       Fit a file of data points to zero (average), first (linear)
|       order polynomial, or exponential
|
+---------------------------------------------------------------------*/
int datafit(int argc, char *argv[], int retc, char *retv[])
{
    int fitType=0;
    int col[5];
    double val[5];
    FILE *fd = NULL;
    symbol **root;
    varInfo *v1 = NULL;
    varInfo *v2 = NULL;
    Rval *r1 = NULL;
    Rval *r2 = NULL;
    double sumx = 0.0;
    double sumx2 = 0.0;
    double sumy = 0.0;
    double sumxy = 0.0;
    double sumy2 = 0.0;
    int index;
    int ret __attribute__((unused));
    char line[512];
    int moreToDo = 1;

    if (argc < 3)
    {   Werrprintf("usage: %s(<fitType>,'file',<datafile>) or", argv[0]);
        Werrprintf("usage: %s(<fitType>,'parX',<'parY'>)", argv[0]);
        ABORT;
    }
    if ( ! strcmp(argv[1],"poly0")  )
       fitType = 0;
    else if ( ! strcmp(argv[1],"poly1") )
       fitType = 1;
    else if ( ! strcmp(argv[1],"exp") )
       fitType = 2;
    else
    {  Werrprintf("%s: first argument must be 'poly0', 'poly1', or 'exp'",argv[0]);
       ABORT;
    }
    for (index=0; index < (int)(sizeof(col)/sizeof(int)); index++)
       col[index] = index;
    if ( ! strcmp(argv[2],"file") )
    {
       if ( (fd = fopen( argv[3], "r" )) == NULL)
       {
          Werrprintf( "%s:  problem opening data file %s", argv[0], argv[2]);
          ABORT;
       }
       index = 5;
       while (index <= argc)
       {
          col[index-5] =  (int) stringReal(argv[index-1]) -1;
          index++;
       }
    }
    else
    {
       if ( argv[2][0] == '$' )
       {
          if ((root=selectVarTree(argv[2])) == NULL)
          {
             Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[2]);
             ABORT;
          }
       }
       else
       {
          root = getTreeRoot("current");
       }
       if ((v1 = rfindVar(argv[2],root)) == NULL)
       {   Werrprintf("%s: x variable \"%s\" doesn't exist",argv[0],argv[2]);
           ABORT;
       }
       r1 = v1->R;
       if (fitType >= 1)
       {
          if (argc < 4)
          {
             Werrprintf("usage: %s('<poly1|exp>','parX','parY')", argv[0]);
             ABORT;
          }
          if ( argv[3][0] == '$' )
          {
             if ((root=selectVarTree(argv[3])) == NULL)
             {
                Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[2]);
                ABORT;
             }
          }
          else
          {
             root = getTreeRoot("current");
          }
          if ((v2 = rfindVar(argv[3],root)) == NULL)
          {   Werrprintf("%s: y variable \"%s\" doesn't exist",argv[0],argv[3]);
              ABORT;
          }
          if ( v1->T.size != v2->T.size )
          {   Werrprintf("%s: number of x variables (%d) not the same as y variables (%d)",
                             argv[0],v1->T.size,v2->T.size);
              ABORT;
          }
          r2 = v2->R;
       }
    }
//    for (index=0; index<sizeof(col)/sizeof(int); index++)
//        fprintf(stderr,"col[%d]: %d\n",index, col[index]);
    index = 0;
    while ( moreToDo )
    {
       if (fd)
       {
          moreToDo = (fscanf(fd, "%[^\n]\n", line) != EOF );
          if ( ! moreToDo )
             break;
          ret = sscanf(line, "%lg %lg %lg %lg %lg\n", &(val[0]), &(val[1]), &(val[2]), &(val[3]), &(val[4]));
       }
       else
       {
          moreToDo = (index < v1->T.size);
          if ( ! moreToDo )
             break;
          val[0] = r1->v.r;
          r1 = r1->next; 
          if (fitType >= 1)
          {
             val[1] = r2->v.r;
             r2 = r2->next; 
             if (fitType == 2)
                val[1] = log(val[1]);
          }
       }
       sumx += val[col[0]];
       sumx2 += (val[col[0]]*val[col[0]]);
       if (fitType >= 1)
       {
          sumy += val[col[1]];
          sumxy += (val[col[0]]*val[col[1]]);
          sumy2 += (val[col[1]]*val[col[1]]);
       }
       index++;
    }
    if (fd)
       fclose(fd);
    if (fitType == 0)
    {
       double average;
       double stddev;
       double N;

       N = (double) index;
       average = sumx/N;
       stddev = sqrt(fabs(sumx2 - sumx*sumx/N)/(N - 1.0));
       if (retc)
       {
          retv[0] = realString(average);
          if (retc>=2)
          {
             retv[1] = realString(stddev);
             if (retc>=3)
             {
                double msd;
                msd = fabs(sumx2 - sumx*sumx/N)/N;
                retv[2] = realString(msd);
                if (retc>=4)
                   retv[3] = realString(sqrt(msd));
             }
          }
       }
       else
          Winfoprintf("average: %g  std. dev.: %g",average,stddev);
    }
    else
    {
       double denom;
       double slope;
       double intercept;
       double rSquared;
       double N;

       N = (double) index;
       denom = N*sumx2 - (sumx*sumx);
       slope = ( N*sumxy - sumx*sumy ) / denom;
       intercept = ( sumx2*sumy - sumx*sumxy ) / denom;
       if (fitType == 2)
          intercept = exp(intercept);
       rSquared = (sumxy -sumx*sumy/N) / sqrt((sumx2 - sumx*sumx/N) * (sumy2 - sumy*sumy/N));
       rSquared *= rSquared;
       if (retc)
       {
          retv[0] = realString(slope);
          if (retc>=2) retv[1] = realString(intercept);
          if (retc>=3) retv[2] = realString(rSquared);
       }
       else if (fitType == 1)
          Winfoprintf("slope: %g  intercept: %g R squared: %g",slope,intercept,rSquared);
       else
          Winfoprintf("exponent: %g  coefficent: %g R squared: %g",slope,intercept,rSquared);
    }
    RETURN;
}       

/*----------------------------------------------------------------------
|
|       ernst(t1,<90degree>)
|       Calculate the ernst angle pulse with a guess at t1 
|               and the 90-degree pulse calibration.
|
|       pw/p1(pwdegrees,<90degrees>)
|       Calculate the flip time in microseconds given a desired 
|               flip angle in degrees and the 90-degree pulse in
|               microswconds.  pw or p1 enters the value into the
|               approiate pulse width parameter.
|
|       If there is a parameter pw90 and no second parameter is entered,
|               pw90 is taken as the 90-degree pulse.  An entered 2nd
|               argument resets pw90.
|
+---------------------------------------------------------------------*/
int ernst(int argc, char *argv[], int retc, char *retv[])
{
    char pulsename[9];
    double degrees,pw,t,tangent;
    double pw90 = 90.0;
    double arg1 = 0.0;
    double t1   = 0.0;
    double acqtim,delay;
    int pres __attribute__((unused));

    (void) retc;  /* suppress warning message */
    (void) retv;  /* suppress warning message */

    if (argc<2)
    {   if (argv[0][0] == 'e') 
            Wscrprintf("usage - ernst(estimated t1(sec), 90 degree pw(usec)\n");
        else
            Wscrprintf(
                "usage - %s(flip angle(degrees), 90 degree pw(usec)\n",argv[0]);
        RETURN;
    }
    if (argc>=2) arg1 = stringReal(argv[1]);
    t1 = arg1;
    if (argc>=3) 
        pw90 = stringReal(argv[2]);
    else
    {
        if (P_getreal(CURRENT,"pw90",&pw90,1))
        {   Werrprintf("parameter pw90 not found");
            ABORT;
        }
        argc = 3;
        argv[2] = realString(pw90);
    }
    if (debug1) Wscrprintf("t1=%5.1f, pw90=%5.1f\n",t1,pw90);
    if (strcmp(argv[0],"ernst") == 0)
    {
        strcpy(pulsename,"pw");
        pres=P_getreal(CURRENT,"at",&acqtim,1);
        pres=P_getreal(CURRENT,"d1",&delay,1);
        t = acqtim + delay;
        if (debug1) Wscrprintf("t/t1=%f\n ",t/t1);
        if (t/t1>40.0)
            degrees = 90.0;
        else
        {
            double u = exp(-t/t1);
            tangent = sqrt(1.0 -u*u)/u;
            degrees = 180*atan(tangent)/3.141592;
            if (debug1) Wscrprintf("u=%f, tangent=%5.1f\n",u,tangent); 
        }
        pw = pw90 * degrees/90.0;
        Winfoprintf("estimated ernst angle %5.1f degrees, %5.1f usec",
                degrees,pw);
    }
    else /* flptim*/
    {
        strcpy(pulsename,argv[0]);
        degrees = arg1;
        pw = pw90 * degrees/90.0;
    }
    /* pres=P_getreal(CURRENT,name,&value,0); */
    P_setreal(CURRENT,pulsename,pw,0);
    appendvarlist(pulsename);
    execString("dg\n");
    RETURN;
}       


/******************************/
static void mathfuncerr(int errmes)
/******************************/
{ switch (errmes)
    { case 1: Werrprintf(
             "Usage: sin(angle)<:n> radians, n is destination parameter");
              break;
      case 2: Werrprintf(
             "Usage: cos(angle)<:n>, radians, n is destination parameter");
              break;
      case 3: Werrprintf(
             "Usage: tan(angle)<:n>, radians, n is destination parameter");
              break;
      case 4: Werrprintf(
             "Usage: atan(value)<:n>, pi/2 to -pi/2n, n is destination parameter");
              break;
      case 5: Werrprintf(
             "Usage: exp(value)<:n>, n is destination parameter");
              break;
      case 6: Werrprintf(
             "Usage: ln(value)<:n>, n is destination parameter");
              break;
      case 7: Werrprintf(
             "Usage: sqrt(value)<:n>, n is destination parameter");
              break;
      case 8: Werrprintf(
             "Usage: acos(value)<:n>, n is destination parameter");
              break;
      case 9: Werrprintf(
             "Usage: asin(value)<:n>, n is destination parameter");
              break;
      case 10: Werrprintf(
             "Usage: abs(value)<:n>, n is destination parameter");
              break;
      case 11: Werrprintf(
             "Usage: atan2(y,x)<:n>, n is destination parameter");
              break;
      case 12: Werrprintf(
             "Usage: pow(x,y)<:n>, n is destination parameter");
              break;
     default: Werrprintf("Undefined Math Function");
              break;
    }
}

/*************************/
int ln(int argc, char *argv[], int retc, char *retv[])
/*  math functions       */
/*************************/
{
  int func;

  if      (strcmp(argv[0],"sin")==0)  func=1;
  else if (strcmp(argv[0],"cos")==0)  func=2;
  else if (strcmp(argv[0],"tan")==0)  func=3;
  else if (strcmp(argv[0],"atan")==0) func=4;
  else if (strcmp(argv[0],"exp")==0)  func=5;
  else if (strcmp(argv[0],"ln")==0)   func=6;
  else if (strcmp(argv[0],"sqrt")==0) func=7;
  else if (strcmp(argv[0],"acos")==0) func=8;
  else if (strcmp(argv[0],"asin")==0) func=9;
  else if (strcmp(argv[0],"abs")==0) func=10;
  else if (strcmp(argv[0],"atan2")==0) func=11;
  else if (strcmp(argv[0],"pow")==0) func=12;
  else func=13;
  if ((argc==2) && (func < 11))
  {   if (isReal(argv[1]))
      {  
         double inval = stringReal(argv[1]);
         double funcval;

         if (((func==6) && (inval <= 0.0)) || ((func==7) && (inval < 0.0)))
         {
            Werrprintf("argument to %s must be positive",argv[0]);
            ABORT;
         }
         if (((func==8) || (func==9)) && ((inval > 1.0) || (inval < -1.0)))
         {
            Werrprintf("argument to %s out of range (-1 < x < 1)",argv[0]);
            ABORT;
         }
         if      (func==1) funcval=sin(inval);
         else if (func==2) funcval=cos(inval);
         else if (func==3) funcval=tan(inval);
         else if (func==4) funcval=atan(inval);
         else if (func==5) funcval=exp(inval);
         else if (func==6) funcval=log(inval);
         else if (func==7) funcval=sqrt(inval);
         else if (func==8) funcval=acos(inval);
         else if (func==9) funcval=asin(inval);
         else /* if (func==10) */ funcval=fabs(inval);
         
         if (retc==0)
           Winfoprintf("%f",funcval);
         else
           retv[0] = realString(funcval);
         RETURN;
      }
      else
        Werrprintf("argument to %s must be a real number",argv[0]);
  }
  else if ((argc==3) && (func == 11))
  {
      if ((isReal(argv[1])) && (isReal(argv[2])))
      {  
         double inval1 = stringReal(argv[1]);
         double inval2 = stringReal(argv[2]);
         double funcval;

         if (inval1 == 0.0 && inval2 == 0.0) {
            Werrprintf("at least one of the two arguments to %s must be non-zero", argv[ 0 ] );
            ABORT;
         }
         funcval=atan2(inval1,inval2);

         if (retc==0)
           Winfoprintf("%f",funcval);
         else
           retv[0] = realString(funcval);
         RETURN;
      }
      else
        Werrprintf("arguments to %s must be real numbers",argv[0]);

  }
  else if ((argc==3) && (func == 12))
  {
      if ((isReal(argv[1])) && (isReal(argv[2])))
      {  
         double inval1 = stringReal(argv[1]);
         double inval2 = stringReal(argv[2]);
         double funcval;

         errno=0;
         funcval=pow(inval1,inval2);
         if (errno )
         {
            Werrprintf("illegal arguments (%g,%g) to the pow command", inval1, inval2 );
            ABORT;
         }

         if (retc==0)
           Winfoprintf("%f",funcval);
         else
           retv[0] = realString(funcval);
         RETURN;
      }
      else
        Werrprintf("arguments to %s must be real numbers",argv[0]);

    /* fall through to ABORT */
  }
  else mathfuncerr(func);
  ABORT;
}

/*****************/
static void parerr(int r, char *parname)
/*****************/
{   Werrprintf("cannot set %s, error %d",parname,r);
}

#define NPUSED 1024
/*----------------------------------------------------------------------
|
|       ilfid()
|       Interleave mulltiple fid into a single fid
|       Uses fid in current experiment. If preservation of original data
|       before interleave is desired, it must be saved in a seperate file.
|
+---------------------------------------------------------------------*/
int ilfid(int argc, char *argv[], int retc, char *retv[])
{
  char arrayname[3][17];
  char na[17];
  char oldfidpath[MAXPATH];
  int arraynum,arraysize;
  int dpflag = 0;
  int e,i,r,blocknumber;
  int f;
  int *dpoint = NULL;
  double *dpoint_double = NULL;
  int li,lj;
  int datapairs;
  dfilehead dhd;
  dpointers block;
  vInfo info;

  (void) argc;  /* suppress warning message */
  (void) argv;  /* suppress warning message */
  (void) retc;  /* suppress warning message */
  (void) retv;  /* suppress warning message */

  Wturnoff_buttons();
  D_allrelease();
  
  /*if (argc<2) blocknumber = 1;
    else { sscanf(argv[1],"%d",&blocknumber); blocknumber--; }*/

  for (i=0; i<3; ++i) arrayname[i][0] = '\0';
  P_getVarInfo(CURRENT,"array",&info);
  arraynum = info.size;
  if (arraynum > 3.01)
    {   Werrprintf("only 3 arrays allowed in interleaved data");
        ABORT;
    }
  arraysize = 1;
  for (i=0; i < arraynum; ++i) 
    {
        P_getstring(CURRENT,"array",na,i+1,8);
        strcpy(arrayname[i],na);
        P_getVarInfo(CURRENT,na,&info);
        arraysize *= info.size;
    }
  if (arraysize >= 20) printf("arraysize=%d\n",arraysize);

  f=D_USERFILE;
  D_close(D_USERFILE); 

  if ( (e = D_gethead(f,&dhd)) )
    {if (e==D_NOTOPEN)
       {
         if ( (e = D_getfilepath(D_USERFILE, oldfidpath, curexpdir)) )
         {
            D_error(e);
            return 1;
         }

         e = D_open(f,oldfidpath,&dhd); /* open the file */
       }
      if (e) { D_error(e); return 1; }
    }

  if (dhd.ntraces < 2) 
  {
    Werrprintf("not a multi-fid experiment");
    ABORT;
    /*for testing: dhd.ntraces *=2; dhd.np /= 2; dhd.tbytes /=2; */
  }
  datapairs =dhd.np/2;
  dpflag = (dhd.ebytes > 2);


  if (dpflag)
  {   dpoint_double = 
             ( double *) allocateWithId(sizeof(double) *
                 (datapairs * dhd.ntraces), "ilfid");
      if (dpoint_double == 0)
      {
          Werrprintf("cannot allocate fid buffer");
          ABORT;
      }
      printf("buffer %zd\n", sizeof(double) * (datapairs * dhd.ntraces) );
  }
  else 
  {   dpoint = ((int *) allocateWithId(sizeof(int) * (datapairs * dhd.ntraces),
                "ilfid"));
      if (dpoint == 0)
      {
          Werrprintf("cannot allocate fid buffer");
          ABORT;
      }
  }

  for (blocknumber = 0; blocknumber<arraysize; blocknumber++) 
  {   /* start for blocknumber= */
      if ( (e = D_getbuf(f, dhd.nblocks, blocknumber, &block)) )
      {
         D_error(e);
         return ERROR;
      }

      /* printf(" dpflag=%d,ebytes=%d,dhd.nt=%d,datapairs=%d\n",
                dpflag,dhd.ebytes, dhd.ntraces,datapairs); */

      /* alternate transpose */
      /* if (!dpflag) 
         for (i=0; i<dhd.ntraces*datapairs; i++)
           dpoint[i] =
            ((int*) block.data)[(i/dhd.ntraces) + (i%dhd.ntraces) * datapairs];
      */
      if (!dpflag)
        for (li = 0; li<dhd.ntraces; li++) 
          for (lj = 0; lj <datapairs; lj++)
            dpoint[li + lj*dhd.ntraces]= ((int*) block.data)[lj + li*datapairs];

      if (dpflag)
        for (li = 0; li<dhd.ntraces; li++) 
          for (lj = 0; lj <datapairs; lj++)
            dpoint_double[li + lj*dhd.ntraces]=
                 ((double*) block.data)[lj + li*datapairs];
      /* for (i=0; i<4; i++) printf("data(%d)=%5d  ", */
      /*   i,((short *)block.data)[i]); printf("\n"); */

      if (dpflag)
        movmem((char *)dpoint_double, (char *)block.data,
                dhd.np*dhd.ntraces*sizeof(int),1,4);
      else 
        movmem((char *)dpoint,(char *)block.data,
                datapairs*dhd.ntraces*sizeof(int),1,2);

      if ( (e = D_markupdated(f,blocknumber)) ) { D_error(e); return 1; }
      if ( (e = D_release(f,blocknumber)) ) { D_error(e); return 1; }
  } /* end for blocknumber= */

  dhd.np     *= dhd.ntraces;
  dhd.tbytes *= dhd.ntraces;
  dhd.ntraces = 1;
  if ( (e=D_updatehead(f,&dhd)) ) { D_error(e); ABORT; }

  r = P_setreal(PROCESSED,"nf",1.0,1);
  if (!r) r = P_setactive(PROCESSED,"nf",ACT_OFF);
  if (r) parerr(r,"nf");

  r = P_setactive(PROCESSED,"cf",ACT_OFF);
  if (r) parerr(r,"cf");

  r = P_setreal(PROCESSED,"np",(double) dhd.np,1);
  if (r) parerr(r,"np");

  if ( (e = D_close(f)) ) { D_error(e); return 1; }
  releaseAllWithId("ilfid"); 
  RETURN;
}

/*----------------------------------------------------------------------
|
|       quadtt()
|       for quadrature testing of receiver components
|       offset determined from average of all points
!       amplitude determined from rms value of points
|               corrcted for offset.
!       phase error determined from product of points in the two
|               channels divide by sum of squares of the A channel.
|       use with an np of 512 or more.
|
+---------------------------------------------------------------------*/
int quadtt(int argc, char *argv[], int retc, char *retv[])
{
   dfilehead dhd;
   dpointers block;
   register int   *datapntr;
   int             e,
                   f,
                   i,
                   npval,
                   first = 0,
                   last = 0,
                   nw = 0,
                   index = -1,
                   blocknumber = 0,
                   onecycle,
                   pointspercyc,
                   phival,
                   rval,
                   ival,
                   trytwice = 0;
   float           phivalue,
                   rvalue,
                   ivalue,
                   rsum = 0.0,
                   isum = 0.0,
                   rsquare = 0.0,
                   isquare = 0.0,
                   phi = 0.0,
                   roffset,
                   ioffset,
                   rpointspercyc,
                   ramp,
                   iamp,
                   unbalance,
                   firstfrac,
                   lastfrac,
                   newfrac = 0.0,
                   length;
   char            oldfidpath[MAXPATH];

   (void) retc;  /* suppress warning message */
   (void) retv;  /* suppress warning message */

   Wturnoff_buttons();
   D_allrelease();
   Wshow_text();

   f = D_USERFILE;
   D_close(D_USERFILE);

   if ( (e = D_gethead(f, &dhd)) )
   {
      if (e == D_NOTOPEN)
      {
         if ( (e = D_getfilepath(D_USERFILE, oldfidpath, curexpdir)) )
         {
            D_error(e);
            return 1;
         }

         e = D_open(f, oldfidpath, &dhd);       /* open the file */
      }

      if (e)
      {
         D_error(e);
         return 1;
      }
   }

   if ( (e = D_getbuf(f, dhd.nblocks, blocknumber, &block)) )
   {
      D_error(e);
      return 1;
   }
   datapntr = (int *) block.data;

   /* Wscrprintf("first point=%d\n", *datapntr); */
   /* real points" */
   if (dhd.ebytes < 4)
   {
      Werrprintf("Double precision data required for quadtt");
      return 1;
   }
   npval = dhd.np;
   if (npval > NPUSED)
      npval = NPUSED;
   if (npval < NPUSED)
   {
      Werrprintf("Adjust for np to be at least %d",NPUSED);
      return 1;
   }
   nw = 0;
   while (nw < npval - 2)
   {
      rval = *(datapntr + nw);
      ival = *(datapntr + nw + 2);
      /* if (*(datapntr + nw) <= 0 && *(datapntr + nw + 2) > 0) */
      if (rval < 0 && ival >= 0)
      {
         index++;
         if (index == 0)
            first = nw;
         last = nw;
      }
      nw += 2;
   }
   /**** if (index <= 0)
   {
      Wscrprintf("TRYTWICE\n");
      nw = 0;
      trytwice = 1;
      while (nw < npval - 2)
      {
         rval =  - *(datapntr + nw);
         ival =  - *(datapntr + nw + 2);
         if (rval < 0 && ival >= 0)
         {
         index++;
         if (index == 0)
            first = nw;
            last = nw;
         }
         nw += 2;
      }
   } ****/
   if (index <= 0)
   {
      if (dhd.np > 2048)
         Werrprintf("Reduce np to between 1024 and 2048 points");
      else
         Werrprintf("More sine wave cycles needed in fid display");
      return 1;
   }
   pointspercyc = (last - first) / index;
   rpointspercyc = (float) (last - first)  /2.0  /(float) (index);
   onecycle = (pointspercyc >= 32);
   if (argc>1) onecycle = ((pointspercyc /2) > atoi(argv[1]));
   if (onecycle)
   {
      int             dp0,
                      dp2;

      npval = pointspercyc;
      last = first + pointspercyc + (100 / pointspercyc) * pointspercyc;
      dp2 = *(datapntr + first + 2);
      dp0 = *(datapntr + first);
      if (trytwice)
      {
         dp2 = -dp2;
         dp0 = -dp0;
      }
      /* Wscrprintf("dp2=%d, dp0=%d",dp2,dp0); */
      firstfrac = (float) dp0 / (float) (dp0 - dp2);
      dp2 = *(datapntr + last + 2);
      dp0 = *(datapntr + last);
      if (trytwice)
      {
         dp2 = -dp2;
         dp0 = -dp0;
      }
      /* Wscrprintf(" last dp2=%d, last dp0=%d\n",dp2,dp0); */
      lastfrac = (float) dp0 / (float) (dp0 - dp2);
      newfrac = lastfrac - firstfrac;
      if (lastfrac < firstfrac)
      {
         last = last - 2;
         newfrac = 1.0 + newfrac;
      }
   }
   /*
    * Wscrprintf("firstfrac = %g, lastfrac =%g, newfrac=%g \n",firstfrac,
    *    lastfrac,newfrac);
    * Wscrprintf("first = %d, last =%d, index=%d \n",first, last,index);
    */
   for (i = first; i < last; i = i + 2)
   {
      rval = *(datapntr + i);
      ival = *(datapntr + i + 1);
      if (trytwice)
      {
         rval = -rval;
         ival = -ival;
      }
      phival = ival;
      rvalue = (float) rval;
      ivalue = (float) ival;
      phivalue = (float) phival;
      rsum += rvalue;
      isum += ivalue;
      phi += phivalue * rvalue;
      rsquare += rvalue * rvalue;
      isquare += ivalue * ivalue;
   }
   /* Wscrprintf("rsum=%g,isum=%g,rsquare=%g,phi=%g\n",rsum,isum,rsquare,phi);*/
   if (onecycle)
   {
      rval = *(datapntr + last);
      ival = *(datapntr + last + 1);
      if (trytwice)
      {
         rval = -rval;
         ival = -ival;
      }
      rvalue = (float) rval;
      ivalue = (float) ival;
      /*
       * Wscrprintf("rval=%g, ival=%g,newfrac=%g,phi=%g\n", rvalue, ivalue,
       * newfrac,phi);
       */
      phivalue = ivalue;
      rsum += rvalue * newfrac;
      isum += ivalue * newfrac;
      phi += phivalue * rvalue * newfrac;
      rsquare += rvalue * rvalue * newfrac;
      isquare += ivalue * ivalue * newfrac;
   }
   /* Wscrprintf("rsum=%g,isum=%g,rsquare=%g,phi=%g\n",rsum,isum,rsquare,phi);*/
   length = (float) (last - first) + 2.0 * newfrac;
   roffset = 2.0 * rsum / length;
   ioffset = 2.0 * isum / length;
   rsquare -= 2.0 * roffset * rsum;
   isquare -= 2.0 * ioffset * isum;
   ramp = 2.0 * sqrt(rsquare / length);
   iamp = 2.0 * sqrt(isquare / length);
   phi = -57.29577951308232087679 * phi / rsquare;
   /* Wscrprintf("length=%g, phi = %g\n",length,phi); */
   /* Make correction for offsets in the channels. */
   phi += 57.29577951308232087679 * 2.0 * roffset * ioffset / ramp / iamp;
   // amplitude = ramp;
   unbalance = 100.0 * (iamp / ramp - 1.0);
   Wclear_text();
   Wscrprintf("Points per Cycle                 = %7.2f\n", rpointspercyc);
   Wscrprintf("Amplitude of channel A           = %7.2f\n", ramp);
   Wscrprintf("Amplitude of channel B           = %7.2f\n", iamp);
   Wscrprintf("Amplitude Unbalance (percent)    = %7.2f\n", unbalance);
   Wscrprintf("Offset of channel A (real)       = %7.2f\n", roffset);
   Wscrprintf("Offset of channel B (imaginary)  = %7.2f\n", ioffset);
   Wscrprintf("Phase of (A - B - 90) (degrees)  = %7.2f\n", phi);
   if ( (e = D_release(f, blocknumber)) )
   {
      D_error(e);
      return 1;
   }
   else
      RETURN;
}

/* calc fid with summing into buffer */

static void calc_FID(void *data, double freq, double amp, double decay, double phase, double dwell, int npnts)
{
   register int i;
   register double arg;
   register double time;
   register float *ptr;
   register double ph;
   register double eval;

   ptr = (float *)data;

   ph = (phase/360.0) * 2.0 * M_PI;
   for (i=0; i < npnts/2; i++)
   {
      time = i * dwell;
      arg = time*freq*2.0*M_PI + ph;
      eval = amp * exp(-time/decay);
      *ptr++ += (float) (eval * cos(arg));
      *ptr++ += (float) (-eval * sin(arg));
   }
}

/* calc fid with setting buffer */

static void calc_FID2(void *data, double freq, double amp, double decay, double phase, double dwell, int npnts)
{
   register int i;
   register double arg;
   register double time;
   register float *ptr;
   register double ph;
   register double eval;

   ptr = (float *)data;

   ph = (phase/360.0) * 2.0 * M_PI;
   for (i=0; i < npnts/2; i++)
   {
      time = i * dwell;
      arg = time*freq*2.0*M_PI + ph;
      eval = amp * exp(-time/decay);
      *ptr++ = (float) (eval * cos(arg));   // calc_FID uses += here
      *ptr++ = (float) (-eval * sin(arg));  // calc_FID uses += here
   }
}

static int zeroElems(int element_number, int old_nblocks, dfilehead *fid_fhead)
{
   int tmp_elem;
   int ival;
   dpointers fid_block;

   for (tmp_elem = old_nblocks; tmp_elem < element_number; tmp_elem++)
   {
      if ( (ival = D_allocbuf( D_USERFILE, tmp_elem, &fid_block )) )
         return(ival);
      memset( fid_block.data, 0, fid_fhead->tbytes );
      make_std_bhead( fid_block.head, fid_fhead->status, (short) tmp_elem+1 );
      ival = D_markupdated( D_USERFILE, tmp_elem );
      ival = D_release( D_USERFILE, tmp_elem );
   }
   return(0);
}

static void cvrtReal(int npVal, int single_prec, void *mem_buffer)
{
   if (single_prec)
   {
      register short *optr = (short *) mem_buffer;
      register float *iptr = (float *) mem_buffer;
      while (npVal--)
         *optr++ = (short) *iptr++;
   }
   else
   {
      register int *optr = (int *) mem_buffer;
      register float *iptr = (float *) mem_buffer;
      while (npVal--)
         *optr++ = (int) *iptr++;
   }
}

#define  UNDEFINED      1
#define  SINGLE_PREC    2
#define  DOUBLE_PREC    3
#define  REAL_NUMBERS   4
#define  CALC_FILE      5
#define  CALC_STR       6
#define  CALC_INDEX     7

static void updateFidHead(dfilehead *fid_fhead, int blks, int pnts, int prec)
{
// fprintf(stderr,"updateFidHead blks= %d\n",blks);
   fid_fhead->nblocks = blks;
   fid_fhead->ntraces = 1;
   fid_fhead->np      = pnts;
   if (prec == SINGLE_PREC)
      fid_fhead->ebytes  = sizeof( short );
   else                                    /* sizeof double precision */
      fid_fhead->ebytes  = sizeof( float );  /* same as sizeof float */
   fid_fhead->tbytes  = pnts * fid_fhead->ebytes;
   fid_fhead->bbytes  = fid_fhead->tbytes + sizeof( struct datablockhead );
   fid_fhead->status  = (S_DATA | S_COMPLEX | S_DDR);
   fid_fhead->vers_id &= ~P_VENDOR_ID;
   fid_fhead->vers_id |= S_MAKEFID;

   if (prec == REAL_NUMBERS)
      fid_fhead->status |= S_FLOAT;
   else if (prec == DOUBLE_PREC)
      fid_fhead->status |= S_32;
}

static int mergeData(void *mem_buffer, dblockhead *this_bh, int element_number, int addFlag,
              int npVal, int new_fid_format, int bytes)
{
   dpointers fid_block;
   int ival;

// fprintf(stderr,"merge addFlag= %d element_number= %d\n",addFlag, element_number);
   if (addFlag)
      ival = D_getbuf( D_USERFILE, 1, element_number, &fid_block );
   else
      ival = D_allocbuf( D_USERFILE, element_number, &fid_block );
   if (ival) {
// fprintf(stderr,"merge Error addFlag= %d ival= %d element_number= %d\n",addFlag, ival, element_number);
      return(ival);
   }

   if (addFlag)
   {
      if (new_fid_format == SINGLE_PREC)
      {
         register short *optr = (short *) fid_block.data;
         register short *iptr = (short *) mem_buffer;
         while (npVal--)
            *optr++ += *iptr++;
      }
      else if (new_fid_format == DOUBLE_PREC)
      {
         register int *optr = (int *) fid_block.data;
         register int *iptr = (int *) mem_buffer;
         while (npVal--)
            *optr++ += *iptr++;
      }
      else
      {
         register float *optr = (float *) fid_block.data;
         register float *iptr = (float *) mem_buffer;
         while (npVal--)
            *optr++ += *iptr++;
      }
   }
   else
   {
      memcpy( fid_block.data, mem_buffer, bytes );
   }
   memcpy( fid_block.head, this_bh, sizeof( dblockhead ) );
   ival = D_markupdated( D_USERFILE, element_number );
   ival = D_release( D_USERFILE, element_number );
   return(0);
}

static int calc_fid_from_values(char *cmd_name, char *fn_addr, void *mem_buffer,
                              int npnts, double dwell, int fromFile, int phInvert )
{
        char     cur_line[ 122 ];
        int      index, len, this_line;
        double   freq, amp, decay, phase;
        FILE    *tfile;

        memset( mem_buffer, 0, npnts*sizeof(float) );
        if (fromFile == 2)  /* values are passed as an argument */
        {
           if (sscanf( fn_addr, "%lg %lg %lg %lg", &freq, &amp, &decay, &phase ) != 4)
           {
                        Werrprintf( "%s:  argument (%s) does not have 4 values",
                            cmd_name, fn_addr);
                        return( -1 );
           }
           if (phInvert)
              phase += 180.0;
           calc_FID(mem_buffer, freq, amp, decay, phase, dwell, npnts);
           return( npnts );
        }
        tfile = fopen( fn_addr, "r" );
        if (tfile == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, fn_addr );
                return( -1 );
        }
        this_line = 0;
        while (fgets( &cur_line[ 0 ], sizeof( cur_line ) - 1, tfile ) != NULL) {
                len = strlen( &cur_line[ 0 ] );
                this_line++;

/*
 *  First check is not likely to succeed; its main purpose
 *  is to eleminate lines not containing any input.
 *
 *  Second test serves to remove the new-line charater; if this
 *  renders the line blank, skip to the next line.
 *
 *  Third test eliminates lines that have only space characters.
 *
 *  Fourth test eliminates those lines with the first non-space
 *  character the pound sign (#)
 */
                if (len < 1)
                  continue;
                if (cur_line[ len - 1 ] == '\n') {
                        if (len == 1)
                          continue;
                        cur_line[ len - 1 ] = '\0';
                        len--;
                }
                index = 0;
                while (index < len) {
                        if (!isspace( cur_line[ index ] ))
                          break;
                        else
                          index++;
                }

                if (index >= len)
                  continue;

                if (cur_line[ index ] == '#')
                  continue;

                if (fromFile == 3) // File has five values per line. Read and ignore the index
                {
                   int dummyIndex;
                   if (sscanf( &cur_line[ index ], "%d %lg %lg %lg %lg",
                         &dummyIndex, &freq, &amp, &decay, &phase ) != 5)
                   {
                        Werrprintf( "%s:  problem reading %s at line %d",
                            cmd_name, fn_addr, this_line);
                        fclose( tfile );
                        return( -1 );
                   }
                }
                else  // File has 4 values per line
                {
                   if (sscanf( &cur_line[ index ], "%lg %lg %lg %lg",
                         &freq, &amp, &decay, &phase ) != 4)
                   {
                        Werrprintf( "%s:  problem reading %s at line %d",
                            cmd_name, fn_addr, this_line);
                        fclose( tfile );
                        return( -1 );
                   }
                }
                if (phInvert)
                   phase += 180.0;
                calc_FID(mem_buffer, freq, amp, decay, phase, dwell, npnts);
        }

        fclose( tfile );
        return( npnts );
}

static int calc_indexed_fid(char *cmd_name, char *fn_addr, void *mem_buffer,
                              int npnts, double dwell, int fid_format, int phInvert,
                              int indexOffset, int *replaced, dfilehead *fid_fhead )
{
        char     cur_line[ 122 ];
        int      index, len, this_line;
        double   freq, amp, decay, phase;
        FILE    *tfile;
        int      fidIndex;
        dblockhead this_bh;
        int old_nblocks;
        int doAdd;
        int numReplace = fid_fhead->nblocks;

//        fprintf(stderr,"addFlag = %d\n", (replaced == NULL) ? 1 : 0);
        
        tfile = fopen( fn_addr, "r" );
        if (tfile == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, fn_addr );
                return( -1 );
        }
        this_line = 0;
        while (fgets( &cur_line[ 0 ], sizeof( cur_line ) - 1, tfile ) != NULL) {
                len = strlen( &cur_line[ 0 ] );
                this_line++;

/*
 *  First check is not likely to succeed; its main purpose
 *  is to eleminate lines not containing any input.
 *
 *  Second test serves to remove the new-line charater; if this
 *  renders the line blank, skip to the next line.
 *
 *  Third test eliminates lines that have only space characters.
 *
 *  Fourth test eliminates those lines with the first non-space
 *  character the pound sign (#)
 */
                if (len < 1)
                  continue;
                if (cur_line[ len - 1 ] == '\n') {
                        if (len == 1)
                          continue;
                        cur_line[ len - 1 ] = '\0';
                        len--;
                }
                index = 0;
                while (index < len) {
                        if (!isspace( cur_line[ index ] ))
                          break;
                        else
                          index++;
                }

                if (index >= len)
                  continue;

                if (cur_line[ index ] == '#')
                  continue;

                if (sscanf( &cur_line[ index ], "%d %lg %lg %lg %lg",
                         &fidIndex, &freq, &amp, &decay, &phase ) != 5)
                {
                   Werrprintf( "%s:  problem reading %s at line %d",
                            cmd_name, fn_addr, this_line);
                   fclose( tfile );
                   return( -1 );
                }
                if (phInvert)
                   phase += 180.0;
                fidIndex += indexOffset;
                if ( fidIndex < 1 )
                {
                   if (indexOffset == 0) 
                      Werrprintf("%s: Index less than 1 read from makefid file '%s' at line %d",
                            cmd_name, fn_addr, this_line);
                   else
                      Werrprintf("%s: Index+indexOffset(%d) less than 1 read from makefid file '%s' at line %d",
                            cmd_name, indexOffset, fn_addr, this_line);
                   fclose( tfile );
                   return( -1 );
                }
                fidIndex--;
// fprintf(stderr,"calc for index %d\n",fidIndex);
                calc_FID2(mem_buffer, freq, amp, decay, phase, dwell, npnts);
                if (fid_format != REAL_NUMBERS)
                   cvrtReal(npnts, (fid_format == SINGLE_PREC), mem_buffer);
                old_nblocks = fid_fhead->nblocks;
                doAdd = (replaced == NULL) ? 1 : 0;
                if ((replaced != NULL) && (fidIndex < numReplace))
                {
                   if ( *(replaced+fidIndex) == 1)
                      doAdd = 1;
                   else
                      *(replaced+fidIndex) = 1;
                   
                }
//fprintf(stderr,"fidIndex: %d doAdd= %d\n",fidIndex, doAdd);
                if (fid_fhead->nblocks < fidIndex+1) {
                   int ival __attribute__((unused));
                   fid_fhead->nblocks = fidIndex+1;
                   ival = D_updatehead( D_USERFILE, fid_fhead );
                   doAdd = 0;
                   make_std_bhead( &this_bh, fid_fhead->status, (short) fidIndex+1 );
                }
                else
                {
                   int ival;
                   ival = D_getblhead( D_USERFILE, fidIndex, &this_bh );
                   if (ival != 0)
                      make_std_bhead(&this_bh, fid_fhead->status, (short) fidIndex+1);
                }
                if (fidIndex > old_nblocks)
                   if (zeroElems(fidIndex, old_nblocks, fid_fhead))
                   {
                      Werrprintf("%s:  error zeroing elements", cmd_name);
                      fclose( tfile );
                      return(-1);
                   }
// fprintf(stderr,"merge npnts= %d tbytes= %d ntraces= %d doAdd= %d addFlag= %d\n",
//                 npnts, fid_fhead->tbytes,fid_fhead->ntraces, doAdd, addFlag);
                if (mergeData(mem_buffer, &this_bh, fidIndex, doAdd, npnts,
                     fid_format, fid_fhead->tbytes*fid_fhead->ntraces))
                {
                   Werrprintf("%s:  error allocating space for block %d", cmd_name, fidIndex+1);
                   fclose( tfile );
                   return(-1);
                }
        }

        fclose( tfile );
        return( npnts );
}

/*------------------------------------------------------
|                                                       |
|       makefid                                         |
|                                                       |
-------------------------------------------------------*/

#define  FILE_NAME_ARG                  1       /* for `makefid' and `writefid' */
#define  COUNT_REQUIRED_ARGS            2       /* for `makefid' and `writefid' */

#define  exceeds_16bits( ival )  (((ival) > 32767) || ((ival) < -32768))

int makefid(int argc, char *argv[], int retc, char *retv[])
{
   char            *cmd_name, *fn_addr;
   void            *mem_buffer;
   int              bytes_to_allocate, cur_fid_format, element_number, ival,
                    new_fid_format, nlines, np_makefid, old_nblocks,
                    update_fhead;
   dfilehead        fid_fhead;
   dblockhead       this_bh;
   int              calcFID = 0;
   int              forceUpdate = 0;
   int              addFlag = 0;
   int              phaseInvert = 0;
   int              revFlag = 0;  /* Flag for spectral reverse */
   int              indexOffset = 0;

   cmd_name = argv[ 0 ];
   if (argc < COUNT_REQUIRED_ARGS) {
      Werrprintf( "Usage:  %s( 'file_name' [, element number, format ] )",
                   cmd_name);
      ABORT;
   }

   element_number = -1;             /* default values */
   new_fid_format = UNDEFINED;
   if (makefid_args( argc, argv, &element_number, &new_fid_format,
                    &addFlag, &phaseInvert, &revFlag, &indexOffset ) != 0)
      ABORT;                        /* `makefid_args' reports error */
//fprintf(stderr,"element_number:%d new_fid_format:%d addFlag:%d phaseInvert:%d revFlag:%d indexOffset:%d\n",
//                element_number, new_fid_format, addFlag, phaseInvert, revFlag, indexOffset);
   if (new_fid_format == CALC_FILE)
   {
      new_fid_format = REAL_NUMBERS;
      calcFID = 1;
   }
   if (new_fid_format == CALC_STR)
   {
      new_fid_format = REAL_NUMBERS;
      calcFID = 2;
   }
   if (new_fid_format == CALC_INDEX)
   {
      new_fid_format = REAL_NUMBERS;
      calcFID = 3;
   }
   if (element_number == 0)
   {
      forceUpdate = 1;
      addFlag = 0;
      P_setreal(CURRENT,"arraydim",1.0,1);
      P_setreal(PROCESSED,"arraydim",1.0,1);
   }
   else if (element_number > 0 )
      element_number--;               /* VNMR numbers elements starting at 0 */

   if (calcFID == 2)
   {
      fn_addr = argv[ 2 ];        /* Pointer to line values */
   }
   else
   {
      fn_addr = argv[ FILE_NAME_ARG ];        /* address of (input) file name */
      nlines = strlen( fn_addr );             /* borrow `nlines' */
      if (nlines >= MAXPATH) {
         Werrprintf("%s:  too many characters in file name", cmd_name);
         ABORT;
      }
      else if (nlines < 1) {
         Werrprintf("%s:  problem with file name argument", cmd_name);
         ABORT;
      }                       /* finished with borrowing `nlines' */

      if (access( fn_addr, F_OK )) {
         Werrprintf( "%s:  file %s not present", cmd_name, fn_addr );
         ABORT;
      }
   }

/*  This program displays an error message if it encounters a problem  */

   if (make_copy_fidfile(cmd_name, curexpdir, NULL) != 0)
      ABORT;

/*  Start with 0-length file header, in case new file header required.  */

   make_empty_fhead( &fid_fhead );

/*  `makefid_getfhead' displayes error message if it encounters a problem.  */

   if (makefid_getfhead( cmd_name, &fid_fhead, &update_fhead, forceUpdate ) != 0)
      ABORT;

// fprintf(stderr,"update_fhead= %d\n",update_fhead);
   if ( !update_fhead ) {
      if ((fid_fhead.status & (S_32 | S_FLOAT)) == 0)
         cur_fid_format = SINGLE_PREC;
      else if (fid_fhead.status & S_FLOAT)
         cur_fid_format = REAL_NUMBERS;
      else
         cur_fid_format = DOUBLE_PREC;

/*  Verify data format only if called out in the command arguments.  */

      if (calcFID)
      {
         double npnts;
         double swval;

         P_getreal(PROCESSED,"np",&npnts,1);
         P_setreal(CURRENT,"np",npnts,1);
         P_getreal(PROCESSED,"sw",&swval,1);
         P_setreal(CURRENT,"sw",swval,1);
         new_fid_format = cur_fid_format;
      }
      else if (new_fid_format != UNDEFINED)
      {
         if (new_fid_format != cur_fid_format) {
            report_data_format_inconsistent(
                                argv[ 0 ], cur_fid_format);
            ABORT;
         }
         else ;                /* do nothing if the two values match */
      }
      else                    /* new fid format not defined */
         new_fid_format = cur_fid_format;
   }
   else                                    /* no data present */
   {
      addFlag = 0;
      if (new_fid_format == UNDEFINED)      /* default to dp=y */
         new_fid_format = DOUBLE_PREC;
   }

   if ( (calcFID == 3) && (element_number == -1) )
   {
      double npnts;
      double swval;
      int *replaceList = NULL;
      P_getreal(CURRENT,"np",&npnts,1);
      P_setreal(PROCESSED,"np",npnts,1);
      P_getreal(CURRENT,"sw",&swval,1);
      P_setreal(PROCESSED,"sw",swval,1);
      if ( update_fhead )
      {
         updateFidHead(&fid_fhead, 1, npnts, new_fid_format);
         D_updatehead( D_USERFILE, &fid_fhead );
      }
      mem_buffer = allocateWithId( npnts*sizeof(float), "makefid" );
      if ( ! addFlag)
      {
         int i;
         replaceList = allocateWithId(fid_fhead.nblocks*sizeof(int), "makefid" );
         for (i=0; i<fid_fhead.nblocks; i++)
            *(replaceList+i) = 0;
      }
      np_makefid = calc_indexed_fid(cmd_name, fn_addr, mem_buffer,
                               (int) npnts, 1.0/swval, new_fid_format, phaseInvert,
                               indexOffset, replaceList, &fid_fhead );
      if (replaceList)
         release(replaceList);
      calcFID = 4;
      P_setreal(CURRENT,"arraydim",(double) fid_fhead.nblocks,1);
      P_setreal(PROCESSED,"arraydim",(double) fid_fhead.nblocks,1);
   }
   else if (calcFID)
   {
      double npnts;
      double swval;
      P_getreal(CURRENT,"np",&npnts,1);
      P_setreal(PROCESSED,"np",npnts,1);
      P_getreal(CURRENT,"sw",&swval,1);
      P_setreal(PROCESSED,"sw",swval,1);
      mem_buffer = allocateWithId( npnts*sizeof(float), "makefid" );
      np_makefid = calc_fid_from_values(cmd_name, fn_addr, mem_buffer,
                               (int) npnts, 1.0/swval, calcFID, phaseInvert );
      if (new_fid_format != REAL_NUMBERS)
         cvrtReal(np_makefid, (new_fid_format == SINGLE_PREC), mem_buffer);

      if ( !update_fhead && ( element_number == 0) )
      {
         if (fid_fhead.np != np_makefid)
         {
            update_fhead = 1;
            addFlag = 0;
         }
      }
   }
   else
   {
/*  Count number of lines in the input file; then allocate buffer space.  */

      nlines = count_lines( fn_addr );
      if (nlines < 0) {
         Werrprintf( "%s:  problem accessing %s", cmd_name, fn_addr );
         ABORT;
      }
      else if (nlines == 0) {
         Werrprintf( "%s:  There are 0 lines in %s ", cmd_name, fn_addr );
         ABORT;
      }

      if (new_fid_format == SINGLE_PREC)
         bytes_to_allocate = nlines*4;
      else
         bytes_to_allocate = nlines*8;

      mem_buffer = allocateWithId( bytes_to_allocate, "makefid" );
      np_makefid = load_ascii_numbers( cmd_name, fn_addr, mem_buffer,
                nlines, new_fid_format, revFlag);
   }
   if (np_makefid < 0) {           /* load_ascii_numbers */
      release( mem_buffer );  /* displays the error */
      ABORT;
   }

   if (calcFID != 4)
   {
      if ( update_fhead ) {
         updateFidHead(&fid_fhead, 1, np_makefid, new_fid_format);
      }
      else {
         if (np_makefid != fid_fhead.np*fid_fhead.ntraces) {
            Werrprintf("%s:  number of points in input file not equal to number in FID",
                cmd_name);
            Wscrprintf("number of points in input file: %d\n", np_makefid);
            Wscrprintf("number of points in FID: %d\n", fid_fhead.np);
            release( mem_buffer );
            ABORT;
         }
      }

/*  Only update the file header if required.  */

      old_nblocks = fid_fhead.nblocks;
      if (forceUpdate)
          old_nblocks=0;
      if (fid_fhead.nblocks < element_number+1) {
         update_fhead = 1;
         fid_fhead.nblocks = element_number+1;
      }
      if (update_fhead)
      {
         ival = D_updatehead( D_USERFILE, &fid_fhead );
         addFlag = 0;
      }

      if (update_fhead && ( D_fidversion() == 0) )
      {
         int i;
         int ival __attribute__((unused));
         dpointers fid_block;
         for (i=0; i< old_nblocks; i++)
         {
            ival = D_getbuf( D_USERFILE, 1, i, &fid_block );
            fid_block.head->status = fid_fhead.status;
            ival = D_markupdated( D_USERFILE, i );
            ival = D_release( D_USERFILE, i );
         }
      }

/*  Now fill in intervening blocks with zeros.
    First intervening element is old value for nblocks.
    Last is element_number-1.  Block element_number is
    written out later.  Of course, there may be no such
    intervening elements.                               */

      if (element_number > old_nblocks)
      {
         if (zeroElems(element_number, old_nblocks, &fid_fhead))
         {
            Werrprintf("%s:  error zeroing elements", cmd_name);
            release( mem_buffer );
            ABORT;
         }
      }
/*  Try to get the original block header, if possible.  */

      if (element_number < old_nblocks)
      {
         if (calcFID && update_fhead)
         {
            make_std_bhead(&this_bh, fid_fhead.status, (short) element_number+1);
         }
         else
         {
            ival = D_getblhead( D_USERFILE, element_number, &this_bh );
            if (ival != 0)
               make_std_bhead(&this_bh, fid_fhead.status, (short) element_number+1);
         }
      }
      else
      {
         make_std_bhead( &this_bh, fid_fhead.status, (short) element_number+1 );
      }

/*  Now write the computed element out.  */

      if (mergeData(mem_buffer, &this_bh, element_number, addFlag, np_makefid,
                    new_fid_format, fid_fhead.tbytes*fid_fhead.ntraces))
      {
         Werrprintf("%s:  error allocating space for block %d", cmd_name, element_number);
         release( mem_buffer );
         ABORT;
      }
   }
   
   release( mem_buffer );
   D_close( D_USERFILE );
   D_trash(D_PHASFILE);
   D_trash(D_DATAFILE);
        
   if (element_number > 0)
   {
      double tmp;
      P_getreal(CURRENT,"arraydim",&tmp,1);
      if (element_number+1 > (int) (tmp+0.1) )
      {
         P_setreal(CURRENT,"arraydim",(double) (element_number+1),1);
         P_setreal(PROCESSED,"arraydim",(double) (element_number+1),1);
      }
   }

   if (retc > 0)
      retv[ 0 ] = realString( (double) np_makefid );
   RETURN;
}

#define FIRST_PARSED_ARG        COUNT_REQUIRED_ARGS

static struct format_table_entry {
        char    *user_value;
        int     internal_value;
} format_table[] = {
        { "dp=n",       SINGLE_PREC },
        { "dp=y",       DOUBLE_PREC },
        { "16-bit",     SINGLE_PREC },
        { "32-bit",     DOUBLE_PREC },
        { "float",      REAL_NUMBERS },
        { "calc",       CALC_FILE },
        { "calcIndex",  CALC_INDEX },
        { "calcstr",    CALC_STR }
};

static int makefid_args(int argc, char *argv[], int *element_addr, int *format_addr,
                    int *addFlag, int *phaseInvert, int *revFlag, int *indexOffset )
{
        int     iter, ival, jter;
        int     size_of_table;
        int     firstArg;

/*  Assume the file name is the first argument;
    this routine works with the 2nd and 3rd arguments.  */

        *element_addr = -1;                      /* default value */
        *format_addr = UNDEFINED;               /* default to what's in the file */
// fprintf(stderr,"argc= %d\n",argc);
// for (iter=0; iter < argc; iter++)
//    fprintf(stderr,"arg:%d %s\n",iter, argv[iter]);

        firstArg = 2;
        if ( ! strcmp(argv[1],"calcstr") )
        {
           *format_addr = CALC_STR;
           if (argc < 3)
           {
              Werrprintf( "%s: calcstr option requires frequency argument", argv[ 0 ]);
              return( -1 );
           }
           firstArg = 3;
        }

        size_of_table = sizeof( format_table ) / sizeof( struct format_table_entry );
        for (iter = firstArg; iter < argc; iter++)
        {
// fprintf(stderr,"check %s\n",argv[iter]);
           if (*format_addr == UNDEFINED)
           {
              for (jter = 0; jter < size_of_table; jter++)
                 if (strcasecmp(argv[ iter ], format_table[ jter ].user_value) == 0) {
                    *format_addr = format_table[ jter ].internal_value;
                    jter = size_of_table;
                 }
              if (*format_addr != UNDEFINED)
                 continue;
           }
           if ( ! strcasecmp(argv[iter],"rev") )
              *revFlag = 1;
           else if ( ! strcasecmp(argv[iter],"add") )
              *addFlag = 1;
           else if ( ! strcasecmp(argv[iter],"sub"))
           {
              *addFlag = 1;
              *phaseInvert = 1;
           }
           else if (strcasecmp(argv[ iter ], "indexOffset")  == 0)
           {
                   if ( (iter + 1 < argc) && isReal( argv[iter+1] ))
                   {
                      iter++;
                      *indexOffset = (int) stringReal( argv[iter] );
                   }
                   else
                   {
                      Werrprintf("indexOffset option for makefid requires a numeric index\n");
                      return(-1);
                   }
           }
           else if (isReal( argv[ iter ] )) {
                   ival = atoi( argv[ iter ] );
                   if (ival < 0) {
                      Werrprintf( "%s:  invalid element number %d", argv[ 0 ], ival);
                      return( -1 );
                   }
                   *element_addr = ival;
           }
           else
           {
                   Werrprintf( "Invalid argument '%s' to %s", argv[ iter ], argv[ 0 ]);
                   return( -1 );
           }
        }
        return( 0 );
}

static int report_data_format_inconsistent(char *cmd_name, int old_format )
{
        if (old_format == REAL_NUMBERS)
          Werrprintf( "%s:  current FID data is in floating point format", cmd_name );
        else if (old_format == SINGLE_PREC)
          Werrprintf( "%s:  current FID data is single precision (dp=n)", cmd_name );
        else
          Werrprintf( "%s:  current FID data is double precision (dp=y)", cmd_name );

        return( 0 );
}

static void make_std_bhead(dblockhead *bh_ref, short b_status, short b_index )
{
        bh_ref->status = b_status;
        bh_ref->scale  = 0;
        bh_ref->index  = b_index;
        bh_ref->mode   = 0;
        bh_ref->ctcount = 1;
        bh_ref->lpval  = 0.0;
        bh_ref->rpval  = 0.0;
        bh_ref->lvl    = 0.0;
        bh_ref->tlt    = 0.0;
}

static void make_empty_fhead(dfilehead *fh_ref )
{
        fh_ref->nblocks = 0;
        fh_ref->ntraces = 1;
        fh_ref->np      = 0;
        fh_ref->ebytes  = 0;
        fh_ref->tbytes  = 0;
        fh_ref->bbytes  = sizeof( struct datablockhead );
        fh_ref->vers_id = VERSION;
        fh_ref->status  = 0;
        fh_ref->nbheaders = 1;
}

/*
 *  retuns 0, if successful, -1 if error.
 *  reports error if it returns -1.
 */

static int makefid_getfhead(char *cmd_name, dfilehead *fh_ref, int *update_fh_ref, int force )
{
        int     ival;
        char    fidpath[ MAXPATH ];

/*  Close user file if previous application left it open.  */

        D_close( D_USERFILE );

        ival = D_getfilepath( D_USERFILE, &fidpath[ 0 ], curexpdir );
        if (ival != 0) {
                Werrprintf( "%s:  internal error #1", cmd_name );
                return( -1 );
        }

/*  If FID file does not exist, open it, using the (presumably
    empty) file header referenced in the argument list.  Tell
    the calling program it can update the FID file header.      */

        if (force)
        {
           unlink(fidpath);
           ival = 1;
        }
        else
        {
           ival = access( &fidpath[ 0 ], F_OK );
        }
        if (ival != 0) {
// fprintf(stderr,"call newhead\n");
                ival = D_newhead( D_USERFILE, &fidpath[ 0 ], fh_ref );
// fprintf(stderr,"newhead nblocks= %d ival= %d\n", fh_ref->nblocks,ival);
                if (ival != 0) {
                        Werrprintf(
            "%s:  cannot create current experiment's FID file", cmd_name
                        );
                        return( -1 );
                }
                else
                  *update_fh_ref = 1;

                return( 0 );
        }

/*  Preexisting FID file must be writeable...  */

        ival = access( &fidpath[ 0 ], W_OK );
        if (ival != 0) {
                Werrprintf(
            "%s:  no write access to current experiment's FID file", cmd_name
                );
                Wscrprintf( "Check %s for a symbolic link\n", &fidpath[ 0 ] );
                return( -1 );
        };

/*  Open the file, obtaining the File Header in the process.  */

        ival = D_open( D_USERFILE, &fidpath[ 0 ], fh_ref);
        if (ival != 0) {
                Werrprintf(
            "%s:  cannot create current experiment's FID file", cmd_name
                );
                return( -1 );
        }

#if 0
        if (fh_ref->ntraces != 1) {
                Werrprintf(
            "%s:  current FID data has multiple traces per element", cmd_name
                );
                return( -1 );
        }
#endif 
        if (fh_ref->nblocks == 0)
          *update_fh_ref = 1;
        else
        *update_fh_ref = 0;

        if (fh_ref->ntraces != 1) {
                Winfoprintf(
            "%s:  current FID data has %d traces per element", cmd_name, fh_ref->ntraces
                );
        }

        return( 0 );
}

/*
 *  retuns 0, if successful, -1 if error.
 *  no message if it returns -1.
 */

static int count_lines(char *fn_addr )
{
        int      ival;
        FILE    *pfile;
        int ch = 0;
        int lines = 0;

/*  Calling routine is expected to check the string length, so
    this check should be successful.  Done here to be complete.  */

        ival = strlen( fn_addr );
        if (ival < 0 || ival >= MAXPATH)
        {
          return( -1 );
        }
        pfile = fopen(fn_addr,"r");
        if (pfile == NULL)
        {
           return(-1);
        }
        while ( !feof(pfile) )
        {
           ch = fgetc(pfile);
           if (ch == '\n')
              lines++;
        }
        fclose(pfile);
        return( lines );

}

/*  mem_buffer expected to be the address of
 *  max_lines*2*sizeof( datatype ) where the datatype is implied
 *  by the value of cur_format
 */
static int load_ascii_numbers(char *cmd_name, char *fn_addr, void *mem_buffer,
                              int max_lines, int cur_format, int revFlag )
{
        char     cur_line[ 122 ];
        short   *short_addr = NULL;
        int      ival, index, jval1, jval2, len, np_loaded, this_line;
        int     *int_addr = NULL;
        float   *float_addr = NULL;
        double   dval1, dval2;
        FILE    *tfile;
        double   drevMult = 1.0;
        int      revMult = 1;

/*  Transfer buffer address to selected type so address arithmetic works right.  */

        if (revFlag)
        {
           drevMult = -1.0;
           revMult = -1;
        }
        if (cur_format == REAL_NUMBERS)
          float_addr = (float *)mem_buffer;
        else if (cur_format == SINGLE_PREC)
          short_addr = (short *)mem_buffer;
        else
          int_addr = (int *)mem_buffer;

        tfile = fopen( fn_addr, "r" );
        if (tfile == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, fn_addr );
                return( -1 );
        }

        np_loaded = 0;
        this_line = 0;
        while (fgets( &cur_line[ 0 ], sizeof( cur_line ) - 1, tfile ) != NULL) {
                len = strlen( &cur_line[ 0 ] );
                this_line++;

/*
 *  First check is not likely to succeed; its main purpose
 *  is to eleminate lines not containing any input.
 *
 *  Second test serves to remove the new-line charater; if this
 *  renders the line blank, skip to the next line.
 *
 *  Third test eliminates lines that have only space characters.
 *
 *  Fourth test eliminates those lines with the first non-space
 *  character the pound sign (#)
 */
                if (len < 1)
                  continue;
                if (cur_line[ len - 1 ] == '\n') {
                        if (len == 1)
                          continue;
                        cur_line[ len - 1 ] = '\0';
                        len--;
                }
                index = 0;
                while (index < len) {
                        if (!isspace( cur_line[ index ] ))
                          break;
                        else
                          index++;
                }

                if (index >= len)
                  continue;

                if (cur_line[ index ] == '#')
                  continue;

/*  Now try to read 2 numbers from the string.  You have to tell `scanf'
    whether the floating point numbers have 32 bits or 64 bits.  If you
    read 32 bits into a 64 bit cell, only the low-order 32 bits are set.
    The high-order bits (and the exponent) are NOT modified, due to the
    ordering of data on the 68020 and the SPARC systems.  On the VAX,
    the situation is reversed (and somewhat better).                    */

                if (cur_format == REAL_NUMBERS)
                  ival = sscanf( &cur_line[ index ], "%lg %lg", &dval1, &dval2 );
                else
                  ival = sscanf( &cur_line[ index ], "%d %d", &jval1, &jval2 );

                if (ival != 2) {
                        Werrprintf(
            "%s:  problem reading %s at line %d", cmd_name, fn_addr, this_line
                        );
                        fclose( tfile );
                        return( -1 );
                }

                if (cur_format == REAL_NUMBERS) {
                        *(float_addr++) = (float) dval1;
                        *(float_addr++) = (float) (dval2 * drevMult);
                }
                else if (cur_format == SINGLE_PREC) {
                        if (exceeds_16bits( jval1 ) || exceeds_16bits( jval2 )) {
                                Werrprintf(
                    "%s:  value overflows single precision in %s at line %d",
                     cmd_name, fn_addr, this_line
                                );
                                fclose( tfile );
                                return( -1 );
                        }
                        *(short_addr++) = (short) jval1;
                        *(short_addr++) = (short) (jval2 * revMult);
                }
                else {
                        *(int_addr++) = jval1;
                        *(int_addr++) = jval2 * revMult;
                }

                np_loaded += 2;

                if (np_loaded >= max_lines*2)
                  break;
        }

        fclose( tfile );
        return( np_loaded );
}

#define MAX_NUMBER_WRITEFID_ARGS        3

int writefid(int argc, char *argv[], int retc, char *retv[])
{
        char            *cmd_name, *fn_addr;
        int              cur_fid_format, element_number, ival;
        int              tmpval __attribute__((unused));
        dfilehead        fid_fhead;
        dpointers        fid_block;

        (void) retc;  /* suppress warning message */
        (void) retv;  /* suppress warning message */
        cmd_name = argv[ 0 ];
        if (argc < COUNT_REQUIRED_ARGS || argc > MAX_NUMBER_WRITEFID_ARGS) {
                Werrprintf(
            "Usage:  %s( 'file_name' [, element number ] )", cmd_name
                );
                ABORT;
        }

        fn_addr = argv[ FILE_NAME_ARG ];        /* address of (output) file name */
        element_number = 1;                     /* default values */
        if (writefid_args( argc, argv, &element_number ) != 0)
          ABORT;                                /* `writefid_args' reports error */

        if (writefid_getfhead( cmd_name, &fid_fhead ) != 0)
          ABORT;

        if (fid_fhead.nblocks < element_number) {
                Werrprintf( "%s:  FID %d does not exist", cmd_name, element_number );
                ABORT;
        }
        element_number--;               /* VNMR numbers elements starting at 0 */

        if ((fid_fhead.status & (S_32 | S_FLOAT)) == 0)
          cur_fid_format = SINGLE_PREC;
        else if (fid_fhead.status & S_FLOAT)
          cur_fid_format = REAL_NUMBERS;
        else
          cur_fid_format = DOUBLE_PREC;

        ival = D_getbuf( D_USERFILE, fid_fhead.nblocks, element_number, &fid_block );
        if (ival != 0) {
                Werrprintf(
            "%s:  problem accessing data in current experiment's FID file", cmd_name
                );
                return( -1 );
        }

/*  Write out the data in the FID, release the buffer
    and close the FID file; then check the result of the
    write operation and RETURN or ABORT as appropriate.  */

        ival = write_numbers_ascii(
                cmd_name,
                fn_addr,
                fid_block.data,
                fid_fhead.np * fid_fhead.ntraces,
                cur_fid_format
        );
        tmpval = D_release( D_USERFILE, element_number );
        tmpval = D_close( D_USERFILE );

        if (ival == 0)
          RETURN;
        else
          ABORT;
}

static int writefid_args(int argc, char *argv[], int *element_addr )
{
        int     ival;

        *element_addr = 1;                      /* default value */

/*  What follows is not really required now,
    but is designed for future expansion.       */

        if (argc == COUNT_REQUIRED_ARGS)
          return( 0 );
        else if (argc > MAX_NUMBER_WRITEFID_ARGS)
          argc = MAX_NUMBER_WRITEFID_ARGS;

/*  Only one addition argument to `writefid' at this time.  */

        if (isReal( argv[ FIRST_PARSED_ARG ] )) {
                ival = atoi( argv[ FIRST_PARSED_ARG ] );
                if (ival < 1) {
                        Werrprintf(
            "%s:  invalid element number %d", argv[ 0 ], ival
                        );
                        return( -1 );
                }
                *element_addr = ival;
        }
        else {
                Werrprintf(
            "Invalid argument '%s' to %s", argv[ FIRST_PARSED_ARG ], argv[ 0 ]
                );
                return( -1 );
        }

        return( 0 );
}

/*
 *  retuns 0, if successful, -1 if error.
 *  reports error if it returns -1.
 *
 *  Unlike the `makefid' version, this one insists on the FID file
 *  already being present with at least one block.  But only read
 *  access is required.
 */

static int writefid_getfhead(char *cmd_name, dfilehead *fh_ref )
{
        int     ival;
        char    fidpath[ MAXPATH ];

/*  Close user file if previous application left it open.  */

        D_close( D_USERFILE );

        ival = D_getfilepath( D_USERFILE, &fidpath[ 0 ], curexpdir );
        if (ival != 0) {
                Werrprintf( "%s:  internal error #3", cmd_name );
                return( -1 );
        }

/*  The FID file must already exist and be readable.  */

        ival = access( &fidpath[ 0 ], R_OK );
        if (ival != 0) {
                Werrprintf(
            "%s:  cannot access current experiment's FID file", cmd_name
                );
                return( -1 );
        }

/*  Open the file, obtaining the File Header in the process.  */

        ival = D_open( D_USERFILE, &fidpath[ 0 ], fh_ref);
        if (ival != 0) {
                Werrprintf(
            "%s:  cannot access current experiment's FID file", cmd_name
                );
                return( -1 );
        }

#if 0
        if (fh_ref->ntraces != 1) {
                Werrprintf(
            "%s:  current FID data has multiple traces per element", cmd_name
                );
                return( -1 );
        }
#endif 
        if (fh_ref->nblocks == 0) {
                Werrprintf(
            "%s:  current experiment's FID file contains no data", cmd_name
                );
                return( -1 );
        }

        if (fh_ref->ntraces != 1) {
                Winfoprintf(
            "%s:  current FID data has %d traces per element", cmd_name, fh_ref->ntraces
                );
        }

        return( 0 );
}

static int write_numbers_ascii(char *cmd_name, char *fn_addr, void *mem_buffer,
                               int np, int cur_format )
{
        char     over_write_prompt[ MAXPATH + 20 ], over_write_ans[ 4 ];
        int      iter, ival;
        short   *short_addr = NULL;
        int     *int_addr = NULL;
        float   *float_addr = NULL;
        FILE    *tfile;

/*  Transfer buffer address to selected type so address arithmetic works right.  */

        if (cur_format == REAL_NUMBERS)
          float_addr = (float *)mem_buffer;
        else if (cur_format == SINGLE_PREC)
          short_addr = (short *)mem_buffer;
        else
          int_addr = (int *)mem_buffer;

/*  Handle situation where the output file is already present.  */

        if (access( fn_addr, F_OK ) == 0) {
                if (!Bnmr) {
                        sprintf(
                                &over_write_prompt[ 0 ],
                                "OK to overwrite %s? ",
                                 fn_addr
                        );
                        W_getInputCR(
                                &over_write_prompt[ 0 ],
                                &over_write_ans[ 0 ],
                                 sizeof( over_write_ans ) - 1
                        );
                        if (over_write_ans[ 0 ] != 'Y' && over_write_ans[ 0 ] != 'y') {
                                Winfoprintf( "%s:  operation aborted", cmd_name );
                                ABORT;
                        }
                }

                ival = unlink( fn_addr );
                if (ival != 0) {
                        Werrprintf( "%s:  cannot remove %s", cmd_name, fn_addr );
                        ABORT;
                }
        }

        tfile = fopen( fn_addr, "w" );
        if (tfile == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, fn_addr );
                return( -1 );
        }

        for (iter = 0; iter < np; iter += 2) {
                if (cur_format == REAL_NUMBERS) {
                        fprintf( tfile, "%g  %g\n", float_addr[ 0 ], float_addr[ 1 ] );
                        float_addr += 2;
                }
                else if (cur_format == SINGLE_PREC) {
                        fprintf( tfile, "%d  %d\n", short_addr[ 0 ], short_addr[ 1 ] );
                        short_addr += 2;
                }
                else {
                        fprintf( tfile, "%d  %d\n", int_addr[ 0 ], int_addr[ 1 ] );
                        int_addr += 2;
                }
        }

        fclose( tfile );
        return( 0 );
}

/*************/
int fidarea(int argc, char *argv[], int retc, char *retv[])
{
    int li;
    int e;
    int f;
    int blocknumber = 0;
    int trace = 0;
    char outfidpath[MAXPATH];
    char dcrmv[16];
    double real;
    double imag;
    double dcreal;
    double dcimag;
    double area;
    dfilehead dhd;
    dpointers block;

    (void) argc;  /* suppress warning message */
    (void) argv;  /* suppress warning message */
    f=D_USERFILE;
    D_close(D_USERFILE);

    Wturnoff_buttons();
    D_allrelease();
    Wshow_text();

    if ( (e = D_gethead(f, &dhd)) ) {
        if (e==D_NOTOPEN) {
            if ( (e = D_getfilepath(f, outfidpath, curexpdir)) ) {
                D_error(e);
                return 1;
            }
            e = D_open(f, outfidpath, &dhd); /* open the file */
        }
        if (e) {
            D_error(e);
            return 1;
        }
    }
    /*Wscrprintf("fidarea(): fidpath=%s\n", outfidpath);*/

    if ( (e = D_getbuf(f, dhd.nblocks,  blocknumber, &block)) ) {
        D_error(e);
        return ERROR;
    }

    dcreal = dcimag = 0;
    if (!P_getstring(CURRENT, "dcrmv", dcrmv, 1, sizeof(dcrmv))) {
        if (dcrmv[0] == 'y')
        {
            dcreal = block.head->lvl;
            dcimag = block.head->tlt;
        }
    }

    area = 0;
    for (li = 0; li < dhd.np; li += 2) {
        if (dhd.ebytes==2) {
            real = ((short *)block.data)[li + dhd.np * trace] - dcreal;
            imag = ((short *)block.data)[li+1 + dhd.np * trace] - dcimag;
        } else if ((block.head->status & S_FLOAT) == 0) {
            real = ((int *)block.data)[li + dhd.np * trace] - dcreal;
            imag = ((int *)block.data)[li+1 + dhd.np * trace] - dcimag;
        } else {
            real = ((float *)block.data)[li + dhd.np * trace] - dcreal;
            imag = ((float *)block.data)[li+1 + dhd.np * trace] - dcimag;
        }
        area += real*real + imag*imag;
    }
    area = sqrt(area / (dhd.np / 2)) / block.head->ctcount;
    if (retc > 0) {
        retv[0] = realString(area);
    } else {
        Wscrprintf("FID Area=%g\n", area);
    }

    if ( (e = D_release(f, blocknumber)) ) {
        D_error(e);
        return ERROR;
    }

    return COMPLETE;
}
