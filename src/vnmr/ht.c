/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************
*  ht.c  -  Hadamard Transform program        *
*           used by ft2d, indirect dimension  *
**********************************************/


#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ftpar.h"
#include "data.h"
#include "process.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "allocate.h"
#include "pvars.h"
#include "sky.h"
#include "wjunk.h"
#include "Pbox_HT.h"


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

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

extern int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead,
                int *lastfid);
static int hm(int nij, int i1, int i2, short brev, short rdr, int nix);
htPar	htpar;


static int hps(N, f)   /* sorts the line lists */
int     N;
double  *f[];
{
  int i, j, k, ii=N-1;
  double tmp;

  if (N==1)
    return 0;

  i=1;
  while ((i<N) && ((*f)[i] < (*f)[i-1]))
    i++;

  if (i!=N)
  {
    k = (N>>1);
    for(;;)
      {
      if (k > 0)
      {
        tmp = (*f)[--k];
      }
      else
      {
        tmp = (*f)[ii];
        (*f)[ii] = (*f)[k];
        if (--ii == 0)
        {
          (*f)[k] = tmp;
          break;
        }
      }
      i=k;
      j=k+1;
      while (j<=ii)
      {
        if (j<ii && (*f)[j] < (*f)[j+1])
          j++;
        if (tmp < (*f)[j])
        {
          (*f)[i] = (*f)[j];
          i=j;
          j <<= 1;
        }
        else
          break;
      }
      (*f)[i]=tmp;
    }
  }
  else
    return 0;
  k=N/2;
  for (i=0, j=N-1; i<k; i++, j--)
  {
    tmp=(*f)[i];
    (*f)[i]=(*f)[j];
    (*f)[j]=tmp;
  }
  return 1;
}


static int bitrev(jx, nx, p2)  /* bit reversal reordering function */
int jx, nx, p2;
{
  int i=1, j=0, k=0;
  jx--;
  if (p2 == 2)
  {
    k = nx >> 1;
    while (i<k)
    {
      j = i + k;
      if (((jx&j) != j) && ((jx&i) || (jx&k)))
        jx = jx ^ j;
      i<<=1, k>>=1;
    }
  }
  jx++;
  return (nx - jx + 1);
}

static int is_hm4(nij)
int nij;
{
  int i, j;

  if (!nij)
  {
    htpar.Hx=0; htpar.H2=0;
    return 0;
  }

  i=1;
  while (i<nij) i<<=1;

  if (i==nij)
  {
    htpar.Hx = 2;
    htpar.H2 = nij;
    return 2;                  /* power of 2 */
  }
  else if (!(nij%12)) htpar.Hx=12, j = nij/htpar.Hx;
  else if (!(nij%20)) htpar.Hx=20, j = nij/htpar.Hx;
  else if (!(nij%28)) htpar.Hx=28, j = nij/htpar.Hx;
  else return 0;

  i=1;
  while (i<j) i <<= 1;

  if (i==j)
    htpar.H2=j;
  else
    return 0;

  return htpar.H2;
}

static int hm2(nij, i1, i2)
int nij, i1, i2;
{
  int  i, j, k, ij, jj, nii, icf=0;

  if (i1 < 2) return 1;
  ij=i2-1; jj=i1-1;           /* reset the indexes */
  for (k=1; k<nij; k*=2);
  nii=k;

  j=1; i=1;
  while ((j*=2) <= jj) i=j;

  icf = 2*((ij/i)%2);

  j = i>>1;
  while (j>0)
  {
    if ((i+j) < i1)
    {
      icf = (icf + 2*((ij/j)%2))%4;
      i+=j;
    }
    j >>= 1;
  }
  return 1-icf;
}

static int hm4(nij)
char nij;
{
  if (nij=='+') return 1;
  else if (nij=='-') return -1;
  else
    Werrprintf("ht (hm4) - error in Hadamard Matrix \n");
  return 0;
}

/* Common definitions of H12, H20, H28 matrices for
 *  vnmr and psg are in Pbox_HT.h.
 * Note the usage of matrices in vnmr are the same
 *  as the matrices used in psg to make shapes,
 *  e.g. H12[i][j], etc. 
 */

static int hm(int nij, int i1, int i2, short brev, short rdr, int nix)   /* ni, i */
{
  int  i, j, ii, jj;

  if (brev)
  {
    i1=bitrev(i1, nix, htpar.Hx);
    i2=bitrev(i2, nix, htpar.Hx);
  }
/*  if (rdr)
  {
    i1=htseq[i1-1];
    i2=htseq[i2-1];
  }
*/
  if (htpar.Hx == 2)
  {
    if (i1 < 2) return 1;
    else return hm2(nij, i1, i2);
  }
  else if (htpar.Hx==12)
  {
    if (htpar.H2>1)
    {
      i = (i1-1)%12;
      j = (i2-1)%12;
      ii= 1 + (i1-1)/12;
      jj= 1 + (i2-1)/12;
      return (hm2(htpar.H2, ii, jj) * hm4(H12[i][j]));
    }
    else
      return hm4(H12[i1-1][i2-1]);
  }
  else if(htpar.Hx==20)
  {
    if (htpar.H2>1)
    {
      i = (i1-1)%20;
      j = (i2-1)%20;
      ii= 1 + (i1-1)/20;
      jj= 1 + (i2-1)/20;
      return (hm2(htpar.H2, ii, jj) * hm4(H20[i][j]));
    }
    else
      return hm4(H20[i1-1][i2-1]);
  }
  else if (htpar.Hx==28)
  {
    if (htpar.H2>1)
    {
      i = (i1-1)%28;
      j = (i2-1)%28;
      ii= 1 + (i1-1)/28;
      jj= 1 + (i2-1)/28;
      return (hm2(htpar.H2, ii, jj) * hm4(H28[i][j]));
    }
    else
      return hm4(H28[i1-1][i2-1]);
  }
/* else if (htpar.Hx<0)
 *  return hm4(hmx[i1-1][i2-1]);
 *  OR return hm4(hmx[i2-1][i1-1]); ???
 * need getmtx to read hmx file if used
 */
  return(1);
}


/*----------------------------------------
|					 |
|	      htinit()/2		 |
|					 |
|  This function finds the appropriate	 |
|  hadamard frequency lists and indices. |
|					 |
+---------------------------------------*/
int htinit(ftparInfo *ftpar, int numfids)
{
  char	  frqname[8], swname[8], fnname[8], niofname[8];
  int	  i, j, index, index_bad, res, asize, maxin, fdimname;
  double  dtmp, fn, sw, delfn;
  vInfo   info;
/*  htparInfo   *htpar;
    htpar = ftpar->htpar1; */

  htpar.Hx = -1;
  htpar.brev = 0;
  htpar.rdr = 0;

  htpar.fsize = 0;
  fdimname = ftpar->D_dimname;

/***********************************************
*  Determine which frequency list, ni offset,  *
*  spectral width, and fn to use.              *
***********************************************/

  if (fdimname & S_NI2)
  {
     strcpy(frqname, "htfrq2");
     strcpy(niofname, "htofs2");
     strcpy(swname, "sw2");
     strcpy(fnname, "fn2");
     fn = ftpar->fn1;
  }
  else if (fdimname & (S_NI|S_NF))
  {
     strcpy(frqname, "htfrq1");
     strcpy(niofname, "htofs1");
     strcpy(swname, "sw1");
     strcpy(fnname, "fn1");
     fn = ftpar->fn1;
  }
  else /* if (fdimname & S_NP) */
  {
     strcpy(frqname, "htfrq");
     strcpy(niofname, "htofs");
     strcpy(swname, "sw");
     strcpy(fnname, "fn");
     fn = ftpar->fn0;
  }

/**********************************
*  Get ni/ni2/np offset.          *
**********************************/

  if ( (res = P_getreal(CURRENT, niofname, &sw, 1)) )
  {
    sw = 1.0;
  }
  htpar.niofs = (int) (sw + 0.5);

/**********************************
*  Get bit reversal flag.         *
**********************************/

  if (!P_getreal(CURRENT, "htbitrev", &sw, 1))
  {
    htpar.brev = (int) (sw + 0.5);
  }

/**********************************
*  Get spectral width.            *
**********************************/

  if ( (res = P_getreal(CURRENT, swname, &sw, 1)) )
  {
    P_err(res, "current ", swname);
    return(ERROR);
  }

/***********************************
*  Calculate Hadamard dimensions.  *
***********************************/

  htpar.dim = is_hm4(numfids);

/**********************************
*  Get values in frequency list.  *
**********************************/

  if ( (res = P_getVarInfo(CURRENT, frqname, &info)) )
  {
     P_err(res, "current ", frqname);
     return(ERROR);
  }
  asize = info.size;
  if (asize > numfids - htpar.niofs)
  {
    asize = numfids - htpar.niofs;
    Winfoprintf("Warning: using only first %d frequencies of %s",asize,frqname);
  }

  if ((htpar.freq = (double *)skyallocateWithId(sizeof(double) * asize, "ft2d")) == NULL)
  {
     Werrprintf("cannot allocate htpar buffer");
     return(ERROR);
  }
  if ((htpar.index = (int *)skyallocateWithId(sizeof(int) * asize, "ft2d")) == NULL)
  {
     Werrprintf("cannot allocate htpar buffer");
     return(ERROR);
  }
  for (i=0; i<asize; i++)
  {
     if ( (res = P_getreal(CURRENT, frqname, &dtmp, i+1)) )
     {
       P_err(res, "current ", frqname);
       return(ERROR);
     }
     htpar.freq[i] = dtmp;
  }

/***************************************
*  Sort frequencies in reverse order.  *
***************************************/

   j=0;
   while ((i = hps(asize, &(htpar.freq))) && (j<10)) j++;

/**********************************************************
*  Set frequencies within (0,sw) instead of (-sw/2,sw/2)  *
*    and reverse sign.                                    *
**********************************************************/

  for (i=0; i<asize; i++)
    htpar.freq[i] = 0.5 * sw - htpar.freq[i];

/************************
*  Check niofs limits.  *
************************/

  if (htpar.niofs < 0)
    htpar.niofs = 0;
  else if (htpar.niofs > (numfids - asize))
    htpar.niofs = numfids - asize;

/*********************************************
*  Calculate indices where to put HT traces  *
*  within 2D spectrum.                       *
*********************************************/

  delfn = 0.5 * fn / sw;
  maxin = (int)(0.5 * fn + 0.5) - 1;
  index_bad = COMPLETE;
  for (i=0; i<asize; i++)
  {
     index = 0;
     index = (int)((double)(htpar.freq[i] * delfn + 0.001));
     if (index < 0)
     {
       index = 0;
       index_bad = ERROR;
       Winfoprintf("%s[%d] out of range of -%s/2 to %s/2.\n",frqname,i+1,swname,swname);
     }
     else if (index > maxin)
     {
       index = maxin;
       index_bad = ERROR;
       Winfoprintf("%s[%d] out of range of -%s/2 to %s/2.\n",frqname,i+1,swname,swname);
     }
     htpar.index[i] = index;
     if (i > 0)
     {
       if (fabs(htpar.freq[i] - htpar.freq[i-1]) < 0.05)
       {
         Werrprintf("frequencies %s[%d], %s[%d] are equal, cannot do Hadamard transform\n",frqname,i,frqname,i+1);
         return(ERROR);
       }
       else if (htpar.index[i] == htpar.index[i-1])
       {
         if (index_bad == ERROR)
           Werrprintf("cannot do Hadamard transform.\n");
         else
         {
/* If the two freq's just happen to fall on either side of a boundary, you
   may be able to get away with much smaller fn1.  More complicated calculation.
*/ 
           res = (int)((2.0 * sw / fabs(htpar.freq[i] - htpar.freq[i-1])) + 0.999);
           index = fn;
           if (P_getmax(CURRENT,fnname,&dtmp))
             dtmp = 524288.0;
           j = (int)dtmp;
           while ((index < res) && (index < j))
             index *= 2;
           Werrprintf("minimum %s of %d recommended for Hadamard - %s[%d], %s[%d] indices are equal\n",fnname,index,frqname,i,frqname,i+1);
/* reset fn1 instead of just recommending a value? */
         }
         return(ERROR);
       }
     }
  }
  htpar.fsize = asize;

  return(COMPLETE);
}

/***************************************************************************
* Error handling of htfrq1[] in htinit()
*  (1) if htfrq1 is outside sw1, set to 0 or sw1
*  (2) if htfrq1 array size is bigger than numfids
*        (a) ignore htfrq1[i] for i>numfids, warning message
*  (3) if htfrq1 array size is bigger than fn1
*        (a) ignore htfrq1's for index bigger than fn1
*        (b) set fn1 bigger than array size, and power of 2 (not done)
*  (4) if the difference between htfrq1[i] and htfrq1[i+1]
*        is smaller than sw1/fn1
*        (a) put out a warning message, suggesting a larger
*          fn1, and (i) ignore one of the frequencies and
*          complete ft2d, or (ii) abort
*        (b) set fn1 to be larger than the difference, unless
*           the value is much too large (>256k?)  (not done)
*  (5) if htfrq1[i] = htfrq1[i+1] for any i
*        -put out a warning message, and
*          (a) abort
*          (b) ignore one of the frequencies, and complete ft2d (not done)
* Current implementation: do not reset fn1.
***************************************************************************/


static int htsum(sgn, addbuf, combinebuf, bufsize)
int sgn, bufsize;
float *addbuf, *combinebuf;
{
  int i;
  register float *src, *sum;
  src = addbuf;
  sum = combinebuf;

  if (sgn > 0)
  {
    for (i=0; i<bufsize; i++)
      *sum++ += *src++;
  }
  else
  {
    for (i=0; i<bufsize; i++)
      *sum++ -= *src++;
  }
  return(COMPLETE);
}


/****************************************************************
*  The Hadamard transform operates on the indirect dimension,   *
*  which is usually done in secondft().  We do it in firstft()  *
*  for bookkeepping reasons, and skip the fft in secondft().    *
*                                                               *
*  gethtfid() does the Hadamard summing of fids, and puts       *
*  the result into combinebuf.                                  *
****************************************************************/

/* loop over fidindex for 2D; loop over nii for 3D? */
int gethtfid(int fidindex, float *combinebuf, ftparInfo *ftpar, dfilehead *fidhead,
             int *lastfid, float *addbuf, int nii, int datatype)
{
  int ret, sgn=1, ij, jx, ix, ik, size;
  float div, *ptr;
  char pmtx[MAXPATH];

/*  if (htpar.fsize <= 0)
    return(ERROR); */

  size = ftpar->ni0;
  zerofill(combinebuf, (ftpar->fn0));
  ret = 0;

  strcpy(pmtx,"\0");
  ij = fidindex / nii;
  jx = fidindex % nii;

  for (ix=0; ix < size; ix++)
  {
/*  if 3D, need getfid(ix + nii * ni, ...)? */
    ret = getfid(ix, addbuf, ftpar, fidhead, lastfid);
    if (ret==COMPLETE)
    {
      ik = ix * size + jx;
      sgn = hm(size, ij+1, ix+1, 0, 0, size);
      ret = htsum(sgn, addbuf, combinebuf, (ftpar->fn0));
      if (size < MAXPATH)
      {
        if (sgn > 0)
          pmtx[ix]='+';
        else
          pmtx[ix]='-';
      }
    }
    else
      break;
  }
  if (ret==COMPLETE)
  {
    div = 1.0 / (float)(size);
    ptr = combinebuf;
    for (ix=0; ix < (ftpar->fn0); ix++)
      *ptr++ *= div;
  }
  if (size-1 < MAXPATH)
  {
    pmtx[size]='\0';
    DPRINT1("HT  %s\n",pmtx);
  }
  return(ret);
}
