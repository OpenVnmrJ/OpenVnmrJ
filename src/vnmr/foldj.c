/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************
*							*
*   foldj	- fold (symmetrize) 2D J spectra	*
*   foldt	- fold (symmetrize) triagonal for COSY 	*
*   foldcc	- fold for CCC2DQ data			*
*   proj	- project 2D spectra			*
*   dc2d	- 2d drift correction			*
*   rotate	- 2d rotation				*
*							*
********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*  VMS does not define M_PI in its math.h file */

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif 

#include "vnmrsys.h"

#include <sys/types.h>
#include <unistd.h>

#include "data.h"
#include "ftpar.h"
#include "disp.h"
#include "init2d.h"
#include "tools.h"
#include "variables.h"
#include "group.h"
#include "allocate.h"
#include "buttons.h"
#include "displayops.h"
#include "init_display.h"
#include "init_proc.h"
#include "pvars.h"
#include "sky.h"
#include "wjunk.h"

#define FALSE		0
#define TRUE		1
#define ERROR		1
#define COMPLETE	0
#define SYMM		1
#define TRIANG		2
#define COVAR		3
#define CCLVL		10000.0		/* see CLVL in "integ.c" */
#define MINDEGREE	0.005

#define C_GETPAR(name, value)					\
	if ( (r = P_getreal(CURRENT, name, value, 1)) )		\
	{ P_err(r, name, ":"); return ERROR; }

#define T_SETPAR(name, value)					\
	if ( (r = P_setreal(TEMPORARY, name, value, 0)) )	\
	{ P_err(r, name, ":"); return ERROR; }

#define T_SETSTRING(name, value)				\
	if ( (r = P_setstring(TEMPORARY, name, value, 0)) )	\
	{ P_err(r, name, ":"); return ERROR; }

#define getdatatype(status)					\
  	( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX :		\
    	( (status & S_COMPLEX) ? COMPLEX : REAL ) )

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

#define nonzerophase(arg1, arg2)                        \
        ( (fabs(arg1) > MINDEGREE) ||                   \
          (fabs(arg2) > MINDEGREE) )

extern int  check_other_experiment(char *exppath, char *exp_no, int showError);
extern void get_weightpar_names(int dim, struct wtparams *wtpar);
extern int get_weightpar_vals(int tree, struct wtparams *wtpar);
extern void set_weightpar_vals(int tree, struct wtparams *wtpar);
extern int transpose(void *matrix, int ncols, int nrows, int datatype);
extern void dodc(float *ptr, int offset, double oldlvl, double oldtlt);
extern int cexpCmd(int argc, char *argv[], int retc, char *retv[]);

extern int		debug1;			/* debug flag */

static void foldj_cleanup(char *message, int dataerror);
static int symmfunc(int operation, float *data1, float *data2, int nelems,
			int dtype, int b1scale, int b2scale);
static int i_proj(int argc, char *argv[], int retc,
                  int *newbuf, int *cmplx, int trace, int *blocks);
static void dc2d_cleanup(char *message);
static void cmplx_lvltlt(float *dataptr, int typeofdata,
                    double oldlvl, double oldtlt);
void data2d_rotate(float *datapntr, int f1phas, float *f1buffer,
                   int f2phas, float *f2buffer, int total_blocks,
                   int block_num, int nf1pnts, int nf2pnts, int hyperflag);

/*---------------------------------------
|					|
|	   removephasefile()/0		|
|					|
+--------------------------------------*/
int removephasefile()
{
   char	path[MAXPATHL];
   int	r,
	datatype;


/**********************************
*  Remove any existing phasefile  *
**********************************/

   D_trash(D_PHASFILE);

   if ( (r = D_getfilepath(D_PHASFILE, path, curexpdir)) )
   {
      D_error(r);
      return(ERROR);
   }

   datatype = getdatatype(datahead.status);

   phasehead.nblocks = datahead.nblocks;
   phasehead.ntraces = datahead.ntraces;
   phasehead.np = datahead.np/datatype;
   phasehead.ebytes = datahead.ebytes;
   phasehead.tbytes = phasehead.ebytes * phasehead.np;
   phasehead.vers_id = (datahead.vers_id & (P_VERS|P_VENDOR_ID)) + PHAS_FILE;
   phasehead.status = 0;
   phasehead.nbheaders = datahead.nbheaders;

   phasehead.bbytes = ((phasehead.nbheaders & NBMASK) * sizeof(dblockhead)) +
			   phasehead.tbytes * phasehead.ntraces;

   if ( (r = D_newhead(D_PHASFILE, path, &phasehead)) )
   {
      D_error(r);
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|		 foldj()		|
|					|
+--------------------------------------*/
int foldj(int argc, char *argv[], int retc, char *retv[])
{
   int		i,
		dtype,
		block_no,
		trace_no,
		r;
   float	*a,
		*b;


   Wturnoff_buttons();
   revflag = TRUE;		/* fold along F1 axis */
   if (init2d(0, 0))
      return ERROR;

   if (!d2flag)
   {
      Werrprintf("No 2D data in data file");
      return ERROR;
   }

   dtype = getdatatype(datahead.status);
   if (removephasefile())
      return ERROR;

/***********************************
*  Begin FOLD for J-resolved data  *
***********************************/

   disp_status("FOLDJ");
   for (block_no = 0; block_no < datahead.nblocks; block_no++)
   {
      disp_index(datahead.nblocks - block_no);
      if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, block_no, &c_block)) )
      {
         foldj_cleanup("", r);
         return ERROR;
      }

      if ((~c_block.head->status) & S_DATA)
      {
         foldj_cleanup("No data in file", 0);
	 return ERROR;
      }
      else if ((~c_block.head->status) & S_SPEC)
      {
         foldj_cleanup("No spectra in file", 0);
         return ERROR;
      }

      for (trace_no = 0; trace_no < datahead.ntraces; trace_no++)
      {

/**************************************************
*  'a' points to the first "type" element in the  *
*  trace; 'b' points to the last "type" element   *
*  in the trace.                                  *
**************************************************/

         a = c_block.data + trace_no * datahead.np;
	 b = a + (datahead.np - dtype);
         a += dtype;				/* skip first point */
	 i = datahead.np/(2*dtype) - 1;
         if (symmfunc(SYMM, a, b, i, dtype, 1, -1))
         {
            foldj_cleanup("", 0);
            return ERROR;
         }
      }

      if ( (r = D_markupdated(D_DATAFILE, block_no)) )
      {
         foldj_cleanup("", r);
         return ERROR;
      }

      if ( (r =  D_release(D_DATAFILE, block_no)) )
      {
         foldj_cleanup("", r);
         return ERROR;
      }

      if (interuption)
      {
         foldj_cleanup("", 0);
         return ERROR;
      }
   }

   foldj_cleanup("", 0);
   if (!Bnmr)
   {
      releasevarlist();
      appendvarlist("dconi");
      Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
   }

   return COMPLETE;
}


/*---------------------------------------
|					|
|	     foldj_cleanup()/2		|
|					|
+--------------------------------------*/
static void foldj_cleanup(char *message, int dataerror)
{
   if (strcmp(message, "") != 0)
      Werrprintf(message);
   if (dataerror)
      D_error(dataerror);
   disp_index(0);
   disp_status("          ");
}


/*---------------------------------------
|					|
|	     foldt_cleanup()/1		|
|					|
+--------------------------------------*/
static void foldt_cleanup(char *message)
{
   if (strcmp(message, "") != 0)
      Werrprintf(message);
   disp_status("             ");
}


/*---------------------------------------
|					|
|	      symmetrize()/6		|
|					|
+--------------------------------------*/
static void symmetrize(float *dblock1, float *dblock2, int ndpts,
                       int datatype, int scale1, int scale2)
{
   register int		i;
   register float	*buf1,
			*buf2,
			a,
			b;

   buf1 = dblock1;
   buf2 = dblock2;
   i = ndpts;

   while (i--)
   {
      a = (*buf1) * (*buf1);
      b = (*buf2) * (*buf2);

      if (datatype == COMPLEX)
      {
         a += (*(buf1 + 1)) * (*(buf1 + 1));
         b += (*(buf2 + 1)) * (*(buf2 + 1));
      }
      else if (datatype == HYPERCOMPLEX)
      {
         a += (*(buf1 + 1)) * (*(buf1 + 1));
         a += (*(buf1 + 2)) * (*(buf1 + 2));
         a += (*(buf1 + 3)) * (*(buf1 + 3));
         b += (*(buf2 + 1)) * (*(buf2 + 1));
         b += (*(buf2 + 2)) * (*(buf2 + 2));
         b += (*(buf2 + 3)) * (*(buf2 + 3));
      }

      if (a < b)
      {
         buf1 += datatype;
         a = (float) sqrt( (double) (a/b) );
         (*buf2++) *= a;

         if (datatype == COMPLEX)
         {
            (*buf2++) *= a;
         }
         else if (datatype == HYPERCOMPLEX)
         {
            (*buf2++) *= a;
            (*buf2++) *= a;
            (*buf2++) *= a;
         }
      }
      else if (a > b)
      {
         buf2 += datatype;
         b = (float) sqrt( (double) (b/a) );
         (*buf1++) *= b; 

         if (datatype == COMPLEX) 
         {
            (*buf1++) *= b;
         }
         else if (datatype == HYPERCOMPLEX)
         {
            (*buf1++) *= b;
            (*buf1++) *= b;
            (*buf1++) *= b;
         }
      }
      else
      {
         buf1 += datatype;
         buf2 += datatype;
      }

      buf1 += scale1;
      buf2 += scale2;
   }
}


/*---------------------------------------
|					|
|	    triangularize()/6		|
|					|
+--------------------------------------*/
static void triangularize(float *dblock1, float *dblock2, int ndpts,
                          int datatype, int scale1, int scale2)
{
   register int		i;
   register float	*buf1,
			*buf2,
			a,
			b,
			r;


   buf1 = dblock1;
   buf2 = dblock2;
   i = ndpts;

   while (i--)
   {
      a = (*buf1) * (*buf1);
      b = (*buf2) * (*buf2);

      if (datatype == COMPLEX)
      {
         a += (*(buf1 + 1)) * (*(buf1 + 1));
         b += (*(buf2 + 1)) * (*(buf2 + 1));
      }
      else if (datatype == HYPERCOMPLEX)
      {
         a += (*(buf1 + 1)) * (*(buf1 + 1));
         a += (*(buf1 + 2)) * (*(buf1 + 2));
         a += (*(buf1 + 3)) * (*(buf1 + 3));
         b += (*(buf2 + 1)) * (*(buf2 + 1));
         b += (*(buf2 + 2)) * (*(buf2 + 2));
         b += (*(buf2 + 3)) * (*(buf2 + 3));
      }

      r = (float) sqrt( (double) (a*b) );
      if (r > 0.0)
      {
         a = (float) sqrt( (double) (r/a) );
         b = (float) sqrt( (double) (r/b) );
      }
      else
      {
         a = 0.0;
         b = 0.0;
      }

      (*buf1++) *= a;
      (*buf2++) *= b;

      if (datatype == COMPLEX)
      {
         (*buf1++) *= a;
         (*buf2++) *= b;
      }
      else if (datatype == HYPERCOMPLEX)
      {
         (*buf1++) *= a;
         (*buf1++) *= a;
         (*buf1++) *= a;
         (*buf2++) *= b;
         (*buf2++) *= b;
         (*buf2++) *= b;
      }

      buf1 += scale1;
      buf2 += scale2;
   }
}

/*---------------------------------------
|					|
|	      covar()/6		|
|  This only works for "real" data"     |
|					|
+--------------------------------------*/
static void covar(float *dblock1, float *dblock2, int ndpts,
                       int datatype, int scale1, int scale2)
{
   register int		i;
   register float	*buf1,
			*buf2,
			a,
			b;


   buf1 = dblock1;
   buf2 = dblock2;
   i = ndpts;

   while (i--)
   {
      a = (*buf1);
      b = (*buf2);

#ifdef NO
      if (datatype == COMPLEX)
      {
         a += (*(buf1 + 1));
         b += (*(buf2 + 1));
      }
      else if (datatype == HYPERCOMPLEX)
      {
         a += (*(buf1 + 1));
         a += (*(buf1 + 2));
         a += (*(buf1 + 3));
         b += (*(buf2 + 1));
         b += (*(buf2 + 2));
         b += (*(buf2 + 3));
      }
#endif

      a *= b;
      if (a <= 0.0)
      {
         (*buf1++) = 0.0;
         (*buf2++) = 0.0;

#ifdef NO
         if (datatype == COMPLEX)
         {
            (*buf1++) = 0.0;
            (*buf2++) = 0.0;
         }
         else if (datatype == HYPERCOMPLEX)
         {
            (*buf1++) = 0.0;
            (*buf1++) = 0.0;
            (*buf1++) = 0.0;
            (*buf2++) = 0.0;
            (*buf2++) = 0.0;
            (*buf2++) = 0.0;
         }
#endif
      }
      else
      {
         b = (float) sqrt( (double) a);
         (*buf1++) = b; 
         (*buf2++) = b; 

#ifdef NO
         if (datatype == COMPLEX) 
         {
            (*buf1++) = b;
            (*buf2++) = b;
         }
         else if (datatype == HYPERCOMPLEX)
         {
            (*buf1++) = b;
            (*buf1++) = b;
            (*buf1++) = b;
            (*buf2++) = b;
            (*buf2++) = b;
            (*buf2++) = b;
         }
#endif
      }
      buf1 += scale1;
      buf2 += scale2;
   }
}



/*---------------------------------------
|					|
|	       symmfunc()/7		|
|					|
+--------------------------------------*/
static int symmfunc(int operation, float *data1, float *data2, int nelems,
			int dtype, int b1scale, int b2scale)
{
   int			skip1,
			skip2;


/* If b1scale or b2scale are 1, the skip variable is 0 
 * causing no additional steps through the vectors in the
 * symmetry functions. If b1scale or b2scale = -1, it causes
 * the symmetry functions to step backward
 */
   skip1 = dtype*(b1scale - 1);
   skip2 = dtype*(b2scale - 1);

   switch (operation)
   {
      case SYMM:
      {
         symmetrize(data1, data2, nelems, dtype, skip1, skip2);
         return(COMPLETE);
      }
      case TRIANG:
      {
         triangularize(data1, data2, nelems, dtype, skip1, skip2);
         return(COMPLETE);
      }
      case COVAR:
      {
         covar(data1, data2, nelems, dtype, skip1, skip2);
         return(COMPLETE);
      }
      default:
      {
         Werrprintf("Unsupported symmetrization function");
         return(ERROR);
      }
   }
}


/*---------------------------------------
|					|
|		foldt()/4		|
|					|
+--------------------------------------*/
/*
 * foldt used to work in the following way.
 * To illustrate what it did, take a hypothetical fn=fn1=8 2D spectrum
 * and fold it:
 *
 *   3  5  6  0
 *   2  4  0  6
 *   1  0  4  5
 *   0  1  2  3
 *
 * where the "0" values are the APPARENT diagonal, and locations with
 * equal non-zero value indicate  pair which are symmetrized (partially,
 * in the case of phase-sensitive data). However, considering the frequency
 * space, the symmetrization should occur as follows:
 *
 *	    < - - - -   sw  - - - - >
 *
 *	^   0     0     0     0     x
 *	|
 *	|   0     2     3     0     x
 *	|
 *     sw1  0     1     0     3     x
 *	|
 *	|   0     0     1     2     x
 *	|
 *	v   x     x     x     x     x
 *
 * Here, the "0" values indicate points that are NOT affected by folding,
 * "x" are "virtual" points indicating true frequency space (sw/sw1) in f1
 * and f2, and 1, 2, and 3 indicate the pairs to be folded. Note that in
 * looking from left to right, the f2 spectrum is "extended" at the "far"
 * end, while in looking from trace #1 up, f1 space is extended at the
 * "near" end - that's not paradoxical, but due to the fact that we do a
 * frequency inversion as part of the second / indirect transformation.
 * 
 * It used to foldt complex data in datafile. It now does foldt on real
 * data in phasefile.
 */
int foldt(int argc, char *argv[], int retc, char *retv[])
{
   int		i,
		j,
		res,
		symoptype;	/* type of symmetry operation		*/
   float *spec;


/*********************************************
*  Determine the type of symmetry operation  *
*  to be performed on the 2D data.           *
*********************************************/

   Wturnoff_buttons();
   if (argc > 1)
   {
      if (strcmp(argv[1], "symm") == 0)
      {
         symoptype = SYMM;
         disp_status("SYMM       ");
      }
      else if (strcmp(argv[1], "triang") == 0)
      {
         symoptype = TRIANG;
         disp_status("TRIANG     ");
      }
      else if (strcmp(argv[1], "covar") == 0)
      {
         symoptype = COVAR;
         disp_status("COVAR      ");
      }
      else
      {
         Werrprintf("Unsupported symmetry operation on 2D data");
         return ERROR;
      }
   }
   else
   {
      symoptype = SYMM;
      disp_status("SYMM       ");
   }

/**************************************************
*  Initialize 2D parameters and check processing  *
*  parameters to insure consistency with demands  *
*  of any symmetry operation.                     *
**************************************************/

   revflag = TRUE;			/* F1 axis horizontal */
   if (init2d(0, 0))
      return ERROR;

   if (!d2flag)
   {
      Werrprintf("No 2D data in data file");
      return ERROR;
   }

   if ((datahead.ntraces * datahead.nblocks) != (datahead.np/ getdatatype(datahead.status) ))
   {
      Werrprintf("FN is not equal to FN1:  data matrix is not symmetric");
      return ERROR;
   }

/*******************************************
*  Start performing symmetry operation on  *
*  the 2D data file.                       *
*******************************************/

   /* Calculate phasefile for f1 dimension revflag = TRUE */
   /* One spectrum from each block, since the entire block is calculate when needed */
   c_last=0;
   for (i = 0; i < datahead.nblocks; i++)
   {
      spec = gettrace(i*specperblock + 1, 0);
   }
   revflag = FALSE;			/* F2 axis horizontal */
   if (init2d(0, 0))
      return ERROR;
   c_last=0;
   c_buffer = -1;
   /* Calculate phasefile for f2 dimension revflag = FALSE */
   /* One spectrum from each block, since the entire block is calculate when needed */
   for (i = 0; i < datahead.nblocks; i++)
   {
      spec = gettrace(i*specperblock + 1, 0);
   }
   for (i = 0; i < datahead.nblocks*specperblock -1; i++)
   {
      float *spec1;
      float *spec2;

      if (interuption)
      {
         foldt_cleanup("");
         return ERROR;
      }
      revflag = TRUE;
      c_last = 0;		/* mark no buffer in workspace */
      c_buffer = -1;
      if (i == 0)
      {
         /* zero first trace in f1 */
         spec1 = gettrace(i, 0);
         for (j=0; j < fn/2; j++)
         {
           *spec1++ = 0.0;
         }
      }
      spec1 = gettrace(i+1, 0);
      revflag = FALSE;
      c_last = 0;		/* mark no buffer in workspace */
      c_buffer = -1;
      spec2 = gettrace(i, 0);
      if (symmfunc(symoptype, spec1, spec2+1, fn/2 - 1, 1, 1, 1))
      {
         foldt_cleanup("Symmetrization error occurred on data file");
         return ERROR;
      }
      /* zero the first point in f2 */
      *spec2 = 0.0;
      /* zero the last point in f1 */
      *(spec1+(fn/2)-1) = 0.0;
      /* Release blocks as we finish with them */
      if ( (i > specperblock) &&  ! ((i-3)  % specperblock))
      {
         int index;

         index = (i / specperblock) - 1;
         if ( (res = D_markupdated(D_PHASFILE, index)) )
         {
            D_error(res);
            return ERROR;
         }
         if ( (res = D_release(D_PHASFILE, index)) )
         {
            D_error(res);
            return ERROR;
         }
         if ( (res = D_markupdated(D_PHASFILE, index+datahead.nblocks)) )
         {
            D_error(res);
            return ERROR;
         }
         if ( (res = D_release(D_PHASFILE, index+datahead.nblocks)) )
         {
            D_error(res);
            return ERROR;
         }
      }
   }
   revflag = FALSE;
   c_last=0;
   c_buffer = -1;
   spec = gettrace( datahead.nblocks*specperblock -1, 0);
   /* zero the last line in f2 */
   for (j=0; j < fn/2; j++)
   {
      *spec++ = 0.0;
   }
   /* Release the last f1 and f2 blocks */
   if ( (res = D_markupdated(D_PHASFILE, datahead.nblocks-1)) )
   {
      D_error(res);
      return ERROR;
   }
   if ( (res = D_release(D_PHASFILE, datahead.nblocks-1)) )
   {
      D_error(res);
      return ERROR;
   }
   if ( (res = D_markupdated(D_PHASFILE, 2*datahead.nblocks -1)) )
   {
      D_error(res);
      return ERROR;
   }
   if ( (res = D_release(D_PHASFILE, 2*datahead.nblocks -1)) )
   {
      D_error(res);
      return ERROR;
   }

/***************************************
*  Symmetry operation is over.  Close  *
*  the open files.                     *
***************************************/

   foldt_cleanup("");
   if (!Bnmr)
   {
      releasevarlist();
      appendvarlist("dconi");
      Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
   }

   return COMPLETE;
}


/*---------------------------------------
|					|
|		proj()/4		|
|					|
|   This function makes a 2D skyline	|
|   projection.				|
|					|
+--------------------------------------*/
int proj(int argc, char *argv[], int retc, char *retv[])
{
   int			trace_no,
			r,
			argnumber,
			sumflag;
   int newbuf;
   int cmplx;
   int incr;
   int trace;
   int blocks;
   float		hzpp1;
   float	*phasfl;
   float	*dptr;
   dpointers		block;
   int spwp = 0;

   Wturnoff_buttons();
   argnumber = 2;
   sumflag = FALSE;
   newbuf = FALSE;
   cmplx = FALSE;
   blocks = trace = 0;
   while ( (argc > argnumber) && !isReal(argv[argnumber]) )
   {
      if (strcmp(argv[argnumber], "sum") == 0)
      {
         sumflag = TRUE;
      }
      else if (strcmp(argv[argnumber], "new") == 0)
      {
         newbuf = TRUE;
      }
      else if (strcmp(argv[argnumber], "complex") == 0)
      {
         cmplx = TRUE;
      }
      else if (strcmp(argv[argnumber], "trace") == 0)
      {
         if ( (argc > argnumber+1) && isReal(argv[argnumber+1]) )
         {
            argnumber++;
            trace = atoi(argv[argnumber]);
         }
         else
         {
             Werrprintf("proj: trace option must be followed by trace index");
	     return ERROR;
         }
         if (trace < 1)
         {
             Werrprintf("proj: trace index must be between 1 and arraydim");
	     return ERROR;
         }
      }
      argnumber++;
   }
   if (trace && newbuf)
   {
      Werrprintf("proj: trace and new options cannot both be used");
      return ERROR;
   }
   if (i_proj(argc, argv, retc, &newbuf, &cmplx, trace, &blocks))
   {
      if (retc)
      {
         retv[0] = intString(0);
         if (retc > 1)
            retv[1] = intString(blocks);
         return COMPLETE;
      }
      return ERROR;
   }

/****************************************
*  Get the input parameters "sp" and    *
*  "wp" if they exist.  Otherwise, the  *
*  full spectral width is used.         *
****************************************/

   if (argc > argnumber)
   {
      if (isReal(argv[argnumber]))
      {
	 sp1 = stringReal(argv[argnumber]);
      }
      else
      {
         Werrprintf("usage - proj(exp_no<,'sum', 'new', start, width, start2, width2>)");
	 return ERROR;
      }

      argnumber++;
   }
   else
   {
      sp1 = (-1) * rflrfp1;
   }

   if (argc > argnumber)
   {
      if (isReal(argv[argnumber]))
      {
	 wp1 = stringReal(argv[argnumber]);
      }
      else
      {
         Werrprintf("usage - proj(exp_no<,'sum', 'new', start, width, start2, width2>)");
	 return ERROR;
      }
      argnumber++;
   }
   else
   {
      wp1 = sw1 - sp1 - rflrfp1;
   }

   if (argc > argnumber)
   {
      if (isReal(argv[argnumber]))
      {
	 sp = stringReal(argv[argnumber]);
      }
      else
      {
         Werrprintf("usage - proj(exp_no<,'sum', 'new', start, width, start2, width2>)");
	 return ERROR;
      }

      argnumber++;
   }
   if (argc > argnumber)
   {
      if (isReal(argv[argnumber]))
      {
	 wp = stringReal(argv[argnumber]);
         spwp=1;
      }
      else
      {
         Werrprintf("usage - proj(exp_no<,'sum', 'new', start, width, start2, width2>)");
	 return ERROR;
      }
   }



/*********************************************
*  Check the wp1 and sp1 internal variables  *
*  for consistency with the known digital    *
*  resolution of the spectrum.               *
*********************************************/

   hzpp1 = sw1 / (double)(fn1/2 - 1);
   if (wp1 < hzpp1)
      wp1 = 0.0;
   else
      checkreal(&wp1, 2.0*sw1/(double)(fn1/2), sw1);
   checkreal(&sp1, (-1)*rflrfp1, sw1 - wp1 - rflrfp1);
   npnt1 = (int)(wp1/hzpp1) + 1;
   fpnt1 = (int)((sw1 - rflrfp1 - sp1 - wp1) / hzpp1);
   if (spwp)
   {
      float hzpp;
      hzpp = sw / (double)(fn/2 - 1);
      if (wp < hzpp)
         wp = 0.0;
      else
         checkreal(&wp, 2.0*sw/(double)(fn/2), sw);
      checkreal(&sp, (-1)*rflrfp, sw - wp - rflrfp);
      npnt = (int)(wp/hzpp) + 1;
      fpnt = (int)((sw - rflrfp - sp - wp) / hzpp);
   }

   if ( ! spwp)
   {
      fpnt = 0;
      npnt = fn/2;
   }
/********************************************
*  Allocate buffer for data and initialize  *
*  the block header.  D_USERFILE refers to  *
*  the DATAFILE in the new experiment.      *
********************************************/

   if (trace)
   {
      if ( (r = D_getbuf(D_USERFILE, trace, trace - 1, &block)) )
      {
         D_error(r);
         D_close(D_USERFILE);
         return ERROR;
      }
      incr = (cmplx) ? 2 : 1;
      dptr = allocateWithId(incr * fn/2 * sizeof(float), "proj");
      zerofill(dptr, incr * fn/2);
   }
   else
   {
      if ( (r = D_allocbuf(D_USERFILE, newbuf - 1, &block)) )
      {
         D_error(r);
         D_close(D_USERFILE);
         return ERROR;
      }

      block.head->scale = 0;
      block.head->status = (S_DATA|S_SPEC|S_FLOAT);
      block.head->index = newbuf;
      block.head->mode = 0;
      block.head->rpval = 0.0;
      block.head->lpval = 0.0;
      block.head->lvl = 0.0;
      block.head->tlt = 0.0;
      block.head->ctcount = 0;
 
      if (cmplx)
      {
         block.head->status |= S_COMPLEX;
         incr = 2;
      }
      else
      {
         incr = 1;
      }

      dptr = block.data;
      zerofill(block.data, incr * fn/2);
   }


   for (trace_no = fpnt1; trace_no < (fpnt1 + npnt1); trace_no++)
   {
      int i;
      float *in1;
      float *in2;

      if ((phasfl = gettrace(trace_no, 0)) == NULL)
      {
         D_close(D_USERFILE);
	 return ERROR;
      }

      i = npnt;
      in1 = dptr+(fpnt*incr);
      in2 = phasfl+fpnt;

      if (sumflag)
      {
         while (i--)
         {
            *in1 += *in2;
            in1 += incr;
            in2++;
         }
      }
      else
      {
         while (i--)
         {
            if (*in2 > *in1)
               *in1 = *in2;
            in1 += incr;
            in2++;
         }
      }
   }
   if (trace)
   {
      int i;
      float *in;
      float *out;
      i = incr * fn/2;
      in = dptr;
      out = block.data;
      while (i--)
      {
         *out++ += *in++;
      }
      releaseAllWithId("proj");
      newbuf = trace;
   }

/******************************************
*  The data file in the "new" experiment  *
*  does NOT contain complex pairs.        *
******************************************/

   if ( (r = D_markupdated(D_USERFILE, newbuf - 1)) )
   {
      D_error(r);
      D_close(D_USERFILE);
      return ERROR;
   }

   if ( (r = D_release(D_USERFILE, newbuf - 1)) )
   {
      D_error(r);
      D_close(D_USERFILE);
      return ERROR;
   }

   if ( (r = D_close(D_USERFILE)) )
   {
      D_error(r);
      return ERROR;
   }

   if (retc)
   {
      retv[0] = intString(1);
      if (retc > 1)
         retv[1] = intString(blocks);
   }
   return COMPLETE;
}

static int saveProjPars(char *exppath, int dim)
{
   char	path[MAXPATH];
   int r;

   T_SETPAR("arraydim", (double) dim);
   if ( (r = D_getparfilepath(CURRENT, path, exppath)) )
   {
      D_error(r);
      return(ERROR);
   }

   if ( (r = P_save(TEMPORARY, path)) )
   {
      Werrprintf("Problem storing parameters in %s", path);
      return(ERROR);
   }

   if ( (r = D_getparfilepath(PROCESSED, path, exppath)) )
   {
      D_error(r); 
      return(ERROR); 
   }

   if ( (r = P_save(TEMPORARY, path)) )
   {
      Werrprintf("Problem storing parameters in %s", path);
      return(ERROR);
   }

   P_treereset(TEMPORARY);
   return(COMPLETE);
}


/*---------------------------------------
|					|
|		i_proj()/2		|
|					|
+--------------------------------------*/
static int i_proj(int argc, char *argv[], int retc,
                  int *newbuf, int *cmplx, int trace, int *blocks)
{
   char		exppath[MAXPATH],
		path[MAXPATH],
                axisval[2];
   int          r;
   double	rfl_val,rfp_val;
   dfilehead	userhead;
   struct wtparams wtpar;


   if (argc < 2)
   {
      Werrprintf("usage - proj(exp_no<,'sum', 'new', start, width, start2, width2>)");
      return(ERROR);
   }

   *blocks = 0;
   if (check_other_experiment(exppath, argv[1], 0))
   {
      if (*newbuf)
      {
         char *argv2[3];
         char tmp[2*MAXPATH];
         char sysdir[2*MAXPATH];
         int ret __attribute__((unused));

         strcpy(tmp,argv[1]);
         argv2[0] = "cexp";
         argv2[1] = tmp;
         argv2[2] = NULL;
         cexpCmd(2,argv2,1,NULL);
         *newbuf = 0;
         sprintf(tmp,"%s/maclib/jexp%s",userdir,argv[1]);
         ret = unlink(tmp);
         sprintf(sysdir,"%s/maclib/jexp1",systemdir);
         ret = symlink(sysdir,tmp);
      }
      else
         return(ERROR);
   }
   if (init2d(1, 0))
      return(ERROR);

   if (!d2flag)
   {
      Werrprintf("No 2D data in data file");
      return(ERROR);
   }

   if ( (r = D_gethead(D_DATAFILE, &datahead)) )
   {
      Werrprintf("Cannot get datafile header for current experiment");
      return(ERROR);
   }

   D_allrelease();

/******************************************
*  Update the current parameters and the  *
*  processed acquisition parameters in    *
*  the temporary tree.                    *
******************************************/

   P_treereset(TEMPORARY);
   if ( (r = P_copy(CURRENT, TEMPORARY)) )
   {
      P_err(r, "C->T ", "copy:");
      return(ERROR);
   }

   if ( (r = P_copygroup(PROCESSED, TEMPORARY, G_ACQUISITION)) )
   {
      P_err(r, "ACQ ", "copygroup:");
      return(ERROR);
   }

   if (get_phase_mode(HORIZ))
   {
      T_SETSTRING("dmg", "ph");
   }
   else if (get_av_mode(HORIZ))
   {
      T_SETSTRING("dmg", "av");
   }
   else
   {
      T_SETSTRING("dmg", "pwr");
   }

   get_weightpar_names(get_dimension(HORIZ), &wtpar);
   get_weightpar_vals(PROCESSED, &wtpar);
   set_weightpar_vals(TEMPORARY, &wtpar);

   T_SETPAR("fn", (double) fn);
   T_SETPAR("sw", sw);
   T_SETPAR("rp", (*cmplx) ? 0.0 : rp);
   T_SETPAR("lp", (*cmplx) ? 0.0 : lp);
   get_reference(HORIZ,&rfl_val, &rfp_val);
   T_SETPAR("rfl", rfl_val);
   T_SETPAR("rfp", rfp_val);
   T_SETPAR("cr", cr);
   T_SETPAR("delta", delta);
   T_SETPAR("sp", sp);
   T_SETPAR("wp", wp);
   get_scale_axis(HORIZ, axisval);
   axisval[1] = '\0';
   T_SETSTRING("axis", axisval);
   T_SETPAR("procdim", 1.0);
   T_SETPAR("ni", 0.0);
   T_SETSTRING("array", "");

/******************************
*  Remove existing FID file.  *
******************************/

   D_close(D_USERFILE);		/* do not check for "close" error here */

   if ( ! *newbuf && (trace == 0) )
   {
      if ( (r = D_getfilepath(D_USERFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }

      if (unlink(path))   // rm acqfil/fid
      {
         if (debug1)
            Wscrprintf("cannot remove file %s\n", path);
      }

/*******************************
*  Remove existing data file.  *
*******************************/

      if ( (r = D_getfilepath(D_DATAFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }

      if (unlink(path)) // remove datdir/data
      {
         if (debug1)
            Wscrprintf("cannot remove file %s\n", path);
      }

/***********************************
*  Remove existing phase file and  *
*  prepare a new phase file.       *
***********************************/

      if ( (r = D_getfilepath(D_PHASFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }

      if (unlink(path)) // remove datdir/phasefile
      {
         if (debug1)
            Wscrprintf("cannot remove file %s\n", path);
      }

      userhead.nblocks = 1;
      userhead.ntraces = 1;
      userhead.np = fn/2;
      userhead.nbheaders = 1;
      userhead.ebytes = 4;

      userhead.tbytes = userhead.ebytes * userhead.np;
      userhead.bbytes = userhead.nbheaders * sizeof(dblockhead) +
			userhead.tbytes * userhead.ntraces;
      userhead.status = 0;
      userhead.vers_id = (P_VERS|P_VENDOR_ID);
      userhead.vers_id += PHAS_FILE;

      if ( (r = D_newhead(D_USERFILE, path, &userhead)) )
      {
         D_error(r);
         return(ERROR);
      }

      if ( (r = D_close(D_USERFILE)) )
      {
         D_error(r);
         return(ERROR);
      }

/**************************
*  Prepare new data file  *
**************************/

      if ( (r = D_getfilepath(D_DATAFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }

      userhead.status = (S_DATA|S_SPEC|S_FLOAT|S_NP);
      if (*cmplx)
      {
         userhead.np = fn;
         userhead.tbytes = userhead.ebytes * userhead.np;
         userhead.bbytes = userhead.nbheaders * sizeof(dblockhead) +
                           userhead.tbytes * userhead.ntraces;
         userhead.status |= S_COMPLEX;
      }
      userhead.vers_id &= (P_VERS|P_VENDOR_ID);
      userhead.vers_id += DATA_FILE;

      if ( (r = D_newhead(D_USERFILE, path, &userhead)) )
      {
         D_error(r);
         return(ERROR);
      }
      D_updatehead(D_USERFILE, &userhead);
      *newbuf = 1;
      *blocks = 1;
   }
   else if (*newbuf)
   {
      if ( (r = D_getfilepath(D_PHASFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }
      if ( (r = D_open(D_USERFILE, path, &userhead)) )
      {
           Werrprintf("cannot open addsub exp phase file: error = %d",r);
           return(ERROR);
      }
      if (userhead.np != fn/2)
      {
           Werrprintf("fn mismatch between current(%d) and projection(%d) phasefile",
                    fn,(int) (userhead.np*2));
           D_trash(D_USERFILE);
           return(ERROR);
      }

      userhead.nblocks += 1;
      *newbuf = userhead.nblocks;
      D_updatehead(D_USERFILE, &userhead);
      D_release(D_USERFILE, *newbuf - 1);
      if ( (r = D_close(D_USERFILE)) )
      {
         D_error(r);
         return(ERROR);
      }
      if ( (r = D_getfilepath(D_DATAFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }
      if ( (r = D_open(D_USERFILE, path, &userhead)) )
      {
           Werrprintf("cannot open addsub exp phase file: error = %d",r);
           return(ERROR);
      }
      userhead.nblocks += 1;
      D_updatehead(D_USERFILE, &userhead);
      *newbuf = userhead.nblocks;
      *cmplx = ( (userhead.status & S_COMPLEX) == S_COMPLEX) ? 1 : 0;
      *blocks = *newbuf;
      
   }
   else // add to existing trace
   {
      if ( (r = D_getfilepath(D_PHASFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }
      if ( (r = D_open(D_USERFILE, path, &userhead)) )
      {
           Werrprintf("cannot open addsub exp phase file: error = %d",r);
           return(ERROR);
      }
      if (userhead.np != fn/2)
      {
           Werrprintf("fn mismatch between current(%d) and projection(%d) phasefile",
                    fn,(int) (userhead.np*2));
           D_trash(D_USERFILE);
           return(ERROR);
      }

      if ( (r = D_close(D_USERFILE)) )
      {
         D_error(r);
         return(ERROR);
      }
      if ( (r = D_getfilepath(D_DATAFILE, path, exppath)) )
      {
         D_error(r);
         return(ERROR);
      }
      if ( (r = D_open(D_USERFILE, path, &userhead)) )
      {
           Werrprintf("cannot open addsub exp phase file: error = %d",r);
           return(ERROR);
      }
      *blocks = userhead.nblocks;
      if  (trace > userhead.nblocks)
      {
           if ( retc == 0 )
              Werrprintf("proj: trace %d does not exist",trace);
           return(ERROR);
      }
      *cmplx = ( (userhead.status & S_COMPLEX) == S_COMPLEX) ? 1 : 0;
   }
/**************************************
*  Write the temporary tree into the  *
*  requested experiment directory.    *
**************************************/

   if ( !trace)
      saveProjPars(exppath, *newbuf);

   return(COMPLETE);
}

#define FRACTION	64	/* fraction at end of spectrum
				   to average */

/*---------------------------------------
|					|
|		 dc2d()/4		|
|					|
|  This function applies a zero-order	|
|  drift correction to complex spectra. |
|					|
+--------------------------------------*/
int dc2d(int argc, char *argv[], int retc, char *retv[])
{
   int			datatype,
			quad4,
			nplinear,
			npblock,
			npf2,
			npf1,
			f1phase,
			f2phase,
			block_no,
			r,
                        rev_dir,
                        norm_dir,
			block_offset,
			quad2;
   register int		i,
			traceincr;
   float		*pf1buffer,
			*mf1buffer,
			*pf2buffer,
			*mf2buffer;
   register float	*tracepnt;
   double		rp_rev,
			lp_rev,
			rp_norm,
			lp_norm;


   Wturnoff_buttons();

   if (argc == 2)
   {
      if (strcmp(argv[1], "f1") == 0)
      {
         revflag = TRUE;
      }
      else if (strcmp(argv[1], "f2") == 0)
      {
         revflag = FALSE;
      }
      else
      {
         Werrprintf("usage - dc2d('f1') or dc2d('f2')");
         return ERROR;
      }
   }
   else
   {
      Werrprintf("usage - dc2d('f1') or dc2d('f2')");
      return ERROR;
   }

   if ( init2d(0, 0) )
      return ERROR;	/* must be done before "datahead"
			   structure is used */

   if (!d2flag)
   {
      Werrprintf("no 2D data in data file");
      return ERROR;
   }

   if (removephasefile())
      return ERROR;

   block_offset = ( revflag ? 0 : datahead.nblocks );
   datatype = getdatatype(datahead.status);
   f1phase = FALSE;
   f2phase = FALSE;
   quad2 = FALSE;
   quad4 = (datatype == HYPERCOMPLEX);
   if (!quad4)
      quad2 = (datatype == COMPLEX);

/************************************
*  Set-up the parameters for phase  *
*  and number of points.            *
************************************/

   nplinear = pointsperspec;
   npblock = specperblock * nblocks;
   if (quad4)
   {
      npblock *= 2;
      nplinear /= 2;
   }

   if (revflag)
   {
      npf1 = nplinear;
      npf2 = npblock;
   }
   else
   {
      npf2 = nplinear;
      npf1 = npblock;
   }

   pf1buffer = mf1buffer = NULL;	/* initialize pointers */
   pf2buffer = mf2buffer = NULL;	/* initialize pointers */

/************************************************
*  Setup the + and - F1 phase-rotation vectors  *
*  for phasing hypercomplex or complex 2D data. *
************************************************/

   rev_dir = get_direction(REVDIR);
   get_phase_pars(rev_dir,&rp_rev,&lp_rev);
   f1phase = ( get_phase_mode(rev_dir) && (quad2 || quad4) &&
                  nonzerophase(rp_rev, lp_rev) &&
                  get_axis_freq(rev_dir) );
   if (f1phase)
   {
      pf1buffer = allocateWithId(npf1 * sizeof(float), "dc2d");
      if (pf1buffer == NULL)
      {
         Werrprintf("Cannot allocate +F1 phase-rotation vector");
         return ERROR;
      }

      mf1buffer = allocateWithId(npf1 * sizeof(float), "dc2d"); 
      if (mf1buffer  == NULL) 
      { 
         Werrprintf("Cannot allocate -F1 phase-rotation vector");
         releaseAllWithId("dc2d");
         return ERROR;
      }

      phasefunc(pf1buffer, npf1/2, lp_rev, rp_rev);
      phasefunc(mf1buffer, npf1/2, -lp_rev, -rp_rev);
   }

/************************************************
*  Setup the + and - F2 phase-rotation vectors  *
*  for phasing hypercomplex 2D data.            *
************************************************/

   norm_dir = get_direction(NORMDIR);
   get_phase_pars(norm_dir,&rp_norm,&lp_norm);
   f2phase = ( get_phase_mode(norm_dir) && quad4 &&
               nonzerophase(rp_norm, lp_norm) );

   if (f2phase)
   {  
      pf2buffer = allocateWithId(npf2 * sizeof(float), "dc2d");
      if (pf2buffer == NULL)
      { 
         Werrprintf("Cannot allocate +F2 phase-rotation vector");
         if (f1phase)
            releaseAllWithId("dc2d");
         return ERROR;
      } 
 
      mf2buffer = allocateWithId(npf2 * sizeof(float), "dc2d");
      if (mf2buffer  == NULL) 
      {     
         Werrprintf("Cannot allocate -F2 phase-rotation vector");
         releaseAllWithId("dc2d");
         return ERROR;
      }     
 
      phasefunc(pf2buffer, npf2/2, lp_norm, rp_norm);
      phasefunc(mf2buffer, npf2/2, -lp_norm, -rp_norm);
   }

/***************************************
*  Force the drift correction to be    *
*  calculated on the entire spectrum.  *
***************************************/

   fpnt = 0;
   npnt = fn/2;
   traceincr = npnt*datatype;

/*************************************
*  Readjust "npf1" and "npf2" to be  *
*  per block.                        *
*************************************/

   if (revflag)
   {
      char charval;
      char tmp[10];

      npf2 /= nblocks;
      get_display_label(rev_dir,&charval);
      sprintf(tmp, "DC2D F%c",charval);
      disp_status(tmp);
   }
   else
   {
      char charval;
      char tmp[10];

      npf1 /= nblocks;
      get_display_label(norm_dir,&charval);
      sprintf(tmp, "DC2D F%c",charval);
      disp_status(tmp);
   }

/*********************************
*  Begin accessing data blocks.  *
*********************************/

   for (block_no = 0; block_no < datahead.nblocks; block_no++)
   {
      disp_index(datahead.nblocks - block_no);
      if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, block_no + block_offset,
		&c_block)) )
      {
         dc2d_cleanup("");
         D_error(r);
         return ERROR;
      }

      if ( !(c_block.head->status & S_DATA) )
      {
         Werrprintf("no data in block %d", block_no + 1);
         dc2d_cleanup("");
	 return ERROR;
      }
      else if (!(c_block.head->status & S_SPEC))
      {
         Werrprintf("no spectra in block %d", block_no + 1);
         dc2d_cleanup("");
         return ERROR;
      }

/*******************************************
*  Phase-rotate the 2D data if necessary.  *
*******************************************/

      if (f1phase || f2phase)
      {
         data2d_rotate(c_block.data, f1phase, pf1buffer, f2phase, pf2buffer,
			   nblocks, block_no + block_offset, npf1/2,
			   npf2/2, quad4);
      }

/*********************************************
*  Perform a DC correction on each trace in  *
*  this block.                               *
*********************************************/

      tracepnt = c_block.data;
      i = specperblock;
      while (i--)
      {
	 dodc(tracepnt, datatype, 0.0, 0.0);
         cmplx_lvltlt(tracepnt, datatype, 0.0, 0.0);
         tracepnt += traceincr;
      }

/**********************************************
*  Un-phase-rotate the 2D data if necessary.  *
**********************************************/

      if (f1phase || f2phase)
      {
         data2d_rotate(c_block.data, f1phase, mf1buffer, f2phase, mf2buffer,
			   nblocks, block_no + block_offset, npf1/2,
			   npf2/2, quad4);
      }

      if ( (r = D_markupdated(D_DATAFILE, block_no + block_offset)) )
      {
         D_error(r);
         dc2d_cleanup("");
         return ERROR;
      }

      if ( (r = D_release(D_DATAFILE, block_no+block_offset)) )
      {
         D_error(r);
         dc2d_cleanup("");
         return ERROR;
      }

      if (interuption)
      {
         dc2d_cleanup("");
         return ERROR;
      }
   }

/***********************************
*  Clean up after DC2D operation.  *
***********************************/

   dc2d_cleanup("");
   if (!Bnmr)
   {
      releasevarlist();
      appendvarlist("dconi");
      Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
   }

   return COMPLETE;
}


/*---------------------------------------
|					|
|	    data2d_rotate()/10		|
|					|
+--------------------------------------*/
void data2d_rotate(float *datapntr, int f1phas, float *f1buffer,
                   int f2phas, float *f2buffer, int total_blocks,
                   int block_num, int nf1pnts, int nf2pnts, int hyperflag)
{
   if (hyperflag)	/* hypercomplex 2D data */
   {
      if (f1phas && f2phas)
      {
         blockrotate4(datapntr, f1buffer, f2buffer, total_blocks,
			block_num, nf1pnts, nf2pnts);
      }
      else if (f1phas)
      {
         blockrotate2(datapntr, f1buffer, total_blocks, block_num,
			nf1pnts, nf2pnts, HYPERCOMPLEX, COMPLEX, TRUE);
         blockrotate2(datapntr + 1, f1buffer, total_blocks, block_num,
			nf1pnts, nf2pnts, HYPERCOMPLEX, COMPLEX, TRUE);
      }
      else if (f2phas)
      {
         blockrotate2(datapntr, f2buffer, total_blocks, block_num,
			nf1pnts, nf2pnts, HYPERCOMPLEX, REAL, FALSE);
         blockrotate2(datapntr + 2, f2buffer, total_blocks, block_num,
			nf1pnts, nf2pnts, HYPERCOMPLEX, REAL, FALSE);
      }
   }
   else if (f1phas)	/* complex 2D data */
   {
      blockrotate2(datapntr, f1buffer, total_blocks, block_num, nf1pnts,
			2*nf2pnts, COMPLEX, REAL, TRUE);
   }
}


/*---------------------------------------
|					|
|	     dc2d_cleanup()/1		|
|					|
+--------------------------------------*/
static void dc2d_cleanup(char *message)
{
   if (strcmp(message, "") != 0)
      Werrprintf(message);
   releaseAllWithId("dc2d");
   disp_status("         ");
   disp_index(0);
}


/*---------------------------------------
|					|
|	     cmplx_lvltlt()/4		|

|					|
+--------------------------------------*/
static void cmplx_lvltlt(float *dataptr, int typeofdata,
                    double oldlvl, double oldtlt)
{
   int			i;
   register int		npts,
			dtype;
   float		sv_start;
   register float	start,
			*p,
			incr;


   dtype = typeofdata;
   sv_start = ((float) (lvl - oldlvl)) / CCLVL;
   incr = ( ((float) (tlt - oldtlt)) / CCLVL ) / (float) (fn/2);

   for (i = 0; i < dtype; i++)
   {
      npts = fn/2;
      p = dataptr + i;
      start = sv_start;

      while (npts--)
      {
         *p += start;
         start += incr;
         p += dtype;
      }
   }
}


/*---------------------------------------
|					|
|	    ccrot_cleanup()/1		|
|					|
+--------------------------------------*/
static void ccrot_cleanup(char *message)
{
   if (strcmp(message, "") != 0)
      Werrprintf(message);
   disp_status("         ");
   disp_index(0);
}


/*---------------------------------------
|					|
|		rotate()/4		|
|					|
+--------------------------------------*/
int rotate(int argc, char *argv[], int retc, char *retv[])
{
   int			dtype,
			ishift,
			block_no,
			trace_no,
			r,
			ptsperspec;
   register int		i;
   register float	*a,
			*b;
   double		theta,
			shift_per_spec,
			shift;

   Wturnoff_buttons();

   if (argc > 1)
   {
      if (isReal(argv[1]))
      {
	 theta = stringReal(argv[1]);
      }
      else
      {
         Werrprintf("usage - rotate(theta)");
	 return ERROR;
      }

      if (fabs(theta) >= 89.9)
      {
         Werrprintf("rotation angle must be less than +/- 90 degrees");
	 return ERROR;
      }
   }
   else
   {
      theta = 45.0;
   }
   theta *= M_PI / 180.0;

   revflag = FALSE;			/* shift along F2 axis */
   if (init2d(0, 0))
      return ERROR;

   if (!d2flag)
   {
      Werrprintf("no 2D data in data file");
      return ERROR;
   }

   if (removephasefile())
      return ERROR;

   dtype = getdatatype(datahead.status);
   ptsperspec = datahead.ntraces * datahead.nblocks;	/* number of complex
							   points along F2 */

   shift_per_spec = (sw1/((fn1/2) - 1))*(((fn/2) - 1)/sw)*sin(theta)/cos(theta);
   shift = - (shift_per_spec * datahead.np) / (2 * dtype);

   for (block_no = 0; block_no < datahead.nblocks; block_no++)
   {
      disp_status("ROTATE");
      disp_index(datahead.nblocks - block_no);
      if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, block_no
			  + datahead.nblocks, &c_block)) )
      {
         ccrot_cleanup("");
         D_error(r);
         return ERROR;
      }

      if (!(c_block.head->status & S_DATA))
      {
         ccrot_cleanup("no data in file");
	 return ERROR;
      }
      else if (!(c_block.head->status & S_SPEC))
      {
         ccrot_cleanup("no spectra in file");
         return ERROR;
      }

      for (trace_no = 0; trace_no < datahead.np/(dtype*datahead.nblocks);
		trace_no++)
      {

/********************************************
*  "a" points to the first complex element  *
*  in the F2 trace.                         *
********************************************/
         a = c_block.data + (trace_no * ptsperspec * dtype);
         ishift = ( (shift < 0.0) ? (int) (shift - 0.5) :
			(int) (shift + 0.5) );

         if (ishift < 0)
         {
            b = a - (dtype * ishift);
	    i = dtype * (ptsperspec + ishift);
            if (i > 0)
            {
               while (i--)
                  *a++ = *b++;
            }

            i =  - (dtype * ishift);
            if (i > (dtype * ptsperspec))
               i = dtype * ptsperspec;
            if (i > 0)
            {
               while (i--)
                  *a++ = 0.0;
            }
	 }
         else if (ishift > 0)
         {
            a += ptsperspec * dtype - 1;
            b = a - (dtype * ishift);
	    i = dtype * (ptsperspec - ishift);
            if (i > 0)
            {
               while (i--)
                  *a-- = *b--;
            }

            i = dtype * ishift;
            if (i > (dtype * ptsperspec))
               i = dtype * ptsperspec;
            if (i > 0)
            {
               while (i--)
                  *a-- = 0.0;
            }
	 }
         shift += shift_per_spec;
      }

      if ( (r = D_markupdated(D_DATAFILE, block_no+datahead.nblocks)) )
      {
         ccrot_cleanup("");
         D_error(r);
         return ERROR;
      }

      if ( (r = D_release(D_DATAFILE, block_no+datahead.nblocks)) )
      {
         ccrot_cleanup("");
         D_error(r);
         return ERROR;
      }

      if (interuption)
      {
         ccrot_cleanup("");
         return ERROR;
      }
   }


   ccrot_cleanup("");
   if (!Bnmr)
   {
      releasevarlist();
      appendvarlist("dconi");
      Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
   }

   return COMPLETE;
}


/*---------------------------------------
|					|
|		foldcc()/4		|
|					|
+--------------------------------------*/
int foldcc(int argc, char *argv[], int retc, char *retv[])
{
   int			r,
			block_no,
			rr1,
			rr2;
   register int		changepertrace,
			trace_no,
			dtype,
			ff1;
   register float	*spec;


   Wturnoff_buttons();
   revflag = FALSE;			/* fold along F2 axis */
   if (init2d(0, 0))
      return ERROR;

   if (!d2flag)
   {
      Werrprintf("no 2D data in data file");
      return ERROR;
   }

   if (datahead.status & S_HYPERCOMPLEX)
   {
      dtype = HYPERCOMPLEX;
   }
   else
   {
      dtype = getdatatype(datahead.status);
   }

   changepertrace = fn/fn1;
   if (changepertrace == 0)
   {
      Werrprintf("fn >= fn1 required for 'foldcc'");
      return ERROR;
   }


   ff1 = 0;
   rr1 = 0;
   rr2 = (fn/2) - 1;
   if (removephasefile())
      return ERROR;

/********************************************
*  Force drift correction to be calculated  *
*  on the whole spectrum.                   *
********************************************/

   fpnt = 0;
   npnt = fn/2;
   disp_status("FOLDCC");

   for (block_no = datahead.nblocks; block_no < 2*datahead.nblocks; block_no++)
   {
      disp_index(2*datahead.nblocks - block_no);
      if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, block_no, &c_block)) )
      {
         ccrot_cleanup("");
         D_error(r);
         return ERROR;
      }

      if (!(c_block.head->status & S_DATA))
      {
         ccrot_cleanup("no data in file");
	 return ERROR;
      }
      else if (!(c_block.head->status & S_SPEC))
      {
         ccrot_cleanup("no spectra in file");
         return ERROR;
      }

      spec = c_block.data;
      for (trace_no = 0; trace_no < fn1/(2*datahead.nblocks); trace_no++)
      {
	 dodc(spec, dtype, 0.0, 0.0);
	 cmplx_lvltlt(spec, dtype, 0.0, 0.0);
         if (ff1 < fn/4)
         {
            symmfunc(SYMM, spec, spec + 2*ff1*dtype, ff1, dtype, 1, -1);
         }
         else
         {
            symmfunc(SYMM, spec + dtype*rr1, spec + dtype*(fn/2 - 1),
			rr2, dtype, 1, -1);
            rr1 += 2*changepertrace;
         }

         ff1 += changepertrace;
         rr2 -= changepertrace;
         spec += (fn/2)*dtype;
      }

      if ( (r = D_markupdated(D_DATAFILE, block_no)) )
      {
         ccrot_cleanup("");
         D_error(r);
         return ERROR;
      }

      if ( (r = D_release(D_DATAFILE, block_no)) )
      {
         ccrot_cleanup("");
         D_error(r);
         return ERROR;
      }

      if (interuption)
      {
         ccrot_cleanup("");
         return ERROR;
      }
   }

   ccrot_cleanup("");
   if (!Bnmr)
   {
      releasevarlist();
      appendvarlist("dconi");
      Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
   }

   return COMPLETE;
}

/*---------------------------------------
|					|
|		zeroneg()/4		|
|					|
+--------------------------------------*/
int zeroneg(int argc, char *argv[], int retc, char *retv[])
{
   register int		i,ctrace, clast, tlast;
   register float	*phasfl, thresh;


   Wturnoff_buttons();

   thresh = 0.0;
   if (init2d(1, 1))
      return ERROR;
   if (argc > 1)
   {
      if (isReal(argv[1]))
      {
	 thresh = stringReal(argv[1]);
         if (normflag)
           thresh /= normalize * vs;
         else
           thresh /= vs;
      }
      else
      {
         Werrprintf("usage - zeroneg(<level>)");
	 return ERROR;
      }
   }

   if (!d2flag)
   {
      Werrprintf("no 2D data in data file");
      return ERROR;
   }

   disp_status("ZERONEG");
   clast = fn1/2;
   tlast = fn/2;

   for (ctrace=0; ctrace < clast; ctrace++)
   { if ((phasfl = gettrace(ctrace,0)) == 0)
	 return 1;
       i = 0;
       while (i++ < tlast)	/* go though the trace */
  	{
   	  if (*phasfl < thresh)  
	       *phasfl = thresh;
  	   phasfl++;

  	} /* ------end of zero negtive points ------*/
        if (ctrace == c_last)
        {
           int r;
           if ( (r = D_markupdated(D_PHASFILE, c_buffer)) )
           {
              D_error(r);
              return ERROR;
           }
        }

   }
   if (!Bnmr)
   {
      releasevarlist();
      appendvarlist("dconi");
      Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
   }
   return COMPLETE;
}
