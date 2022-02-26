/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************/
/* clradd - clear the addsub experiment					*/
/* add    - add FID to the addsub FID					*/
/* sub    - subtract FID from the addsub FID				*/
/* spadd  - add a spectrum to the addsub spectrum			*/
/* spsub  - subtract a spectrum from the addsub spectrum		*/
/* addi   - interactive combination a spectrum with the addsub spectrum */
/************************************************************************/

#include "vnmrsys.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "disp.h"
#include "data.h"
#include "group.h"
#include "init2d.h"
#include "allocate.h"
#include "tools.h"
#include "pvars.h"
#include "sky.h"
#include "wjunk.h"
#include "variables.h"

#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define ADD_MODE        0
#define SUB_MODE        1
#define MIN_MODE        2
#define MAX_MODE        3

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

extern int	debug1;
extern float	*calc_user(int trace, int fpoint, int dcflag,
                           int normok, int *newspec, int file_id);
extern void     get_ref_pars(int direction, double *swval, double *ref,
                             int *fnval);
extern int      datapoint(double freq, double sw, int rfn);
extern int      start_addi(float *ptr2, float *ptr3, double scl, char *path);
extern void     Wturnoff_buttons();
extern int      cexpCmd(int argc, char *argv[], int retc, char *retv[]);
extern int      delexp(int argc, char *argv[], int retc, char *retv[]);
extern float *get_one_fid(int curfid, int *np, dpointers *c_block, int dcflag);
extern double get_dsp_lp();
extern int      expdir_to_expnum(char *expdir);

static int	addsub_complex,
		newbuf,
		save_complex,
		shift,
		range,
                replaceData,
		trace;			/* trace number in addsub exp */
static int      old_blocks, fidhead_status;
static int	interactive_addi = FALSE;
static float    *addi_cur_imag;
static double	multiplier;
static char *rangeHighField;
static char *rangeLowField;

static int save_addsub_pars();
static int addsub_pars(char [], int);

/*---------------------------------------
|					|
|	      open_file()/3		|
|					|
+--------------------------------------*/
static int open_file(int np, char *filepath, int stat, int vendor)
{
  int	r;

  datahead.np = np;
  datahead.ntraces = 1;
  datahead.nblocks = 1;
  datahead.nbheaders = 1;
  datahead.status = stat;
  datahead.ebytes = 4;
  datahead.tbytes = datahead.ebytes * datahead.np;
  datahead.bbytes = datahead.tbytes + sizeof(dblockhead);
  datahead.vers_id &= ~P_VENDOR_ID;
  datahead.vers_id |= vendor;

  if ( (r = D_newhead(D_USERFILE, filepath, &datahead)) )
  {
     Werrprintf("cannot define addsub exp file: error = %d", r);
     return(ERROR);
  }

  return(COMPLETE);
}


/*---------------------------------------
|					|
|	     add_new_file()/4		|
|					|
+--------------------------------------*/
static int add_new_file(dpointers *header, int index, int stat, int mode)
{
  int	r;

  if ( (r = D_allocbuf(D_USERFILE, index, header)) )
  {
     Werrprintf("cannot allocate addsub exp buffer: error = %d", r);
     D_close(D_USERFILE);
     return(ERROR);
  }

  header->head->index   = index+1;
  header->head->scale   = 0;
  header->head->ctcount = 0;

  if (save_complex)
     stat |= NP_CMPLX;

  header->head->status  = stat;
  header->head->mode    = mode;
  header->head->rpval   = 0.0;
  header->head->lpval   = 0.0;
  header->head->lvl     = 0.0;
  header->head->tlt     = 0.0;

  return(COMPLETE);
}


/*---------------------------------------
|					|
|	     new_datafile()/1		|
|					|
+--------------------------------------*/
static int new_datafile(char *exppath)
{
  char	path[MAXPATH];

  D_close(D_DATAFILE);
  strcpy(path, exppath);
#ifdef UNIX
  strcat(path, "/datdir/data");
#else 
  vms_fname_cat(path, "[.datdir]data");
#endif 

  if (D_open(D_DATAFILE, path, &datahead))
  {
     Werrprintf("cannot open file %s", path);
     return(ERROR);
  }

  return(COMPLETE);
}


/*---------------------------------------
|					|
|	   get_addsub_data()/8		|
|					|
+--------------------------------------*/
static int get_addsub_data(int newexp, dpointers *new_file, char *exppath,
                           int status1, int status2, int mode1, int mode2,
                           int fid, int range)
{
  char	path[MAXPATH];
  int	r,
	file_id;

  D_trash(D_USERFILE); 
  strcpy(path, exppath);
  strcat(path, "/datdir/phasefile");

  if (newexp)
  {
     int vendor_id = 0;
     dpointers	dummy;

/**************************************************
*  PHASEFILE in addsub experiment is done first.  *
**************************************************/

     trace = 0;
     open_file(fn/2, path, (S_DATA|S_FLOAT|S_NP|status1),vendor_id);
     add_new_file(&dummy, trace, (S_DATA|S_FLOAT|status1), mode1);
     D_release(D_USERFILE, trace);
     D_close(D_USERFILE);

/********************************************
*  Either FID or DATA in addsub experiment  *
*  is done next.  The file is associated    *
*  with D_USERFILE.                         *
********************************************/

     strcpy(path, exppath);
#ifdef UNIX
     strcat(path, (fid) ? "/acqfil/fid" : "/datdir/data");
#else 
     vms_fname_cat(path, (fid) ? "[.acqfil]fid" : "[.datdir]data");
#endif 

     if (fid)
     {
        vendor_id = D_getLastVendorId();
     }
     open_file(fn, path, (S_DATA|S_FLOAT|S_NP|status2),vendor_id);
     if (fid)
     {
         add_new_file(new_file, trace, (S_DATA|S_FLOAT|status2), 0);
     }
     else
     {
        add_new_file(new_file, trace, (S_DATA|S_FLOAT|status2), mode2);
     }

     newbuf = TRUE;
  }
  else
  {

/**************************************************
*  D_USERFILE is PHASEFILE in addsub experiment.  *
**************************************************/

   if (!fid)
   {
     if ( (r = D_open(D_USERFILE, path, &datahead)) )
     {
        Werrprintf("cannot open addsub exp phase file: error = %d",r);
        return(ERROR);
     }
     if ( ! range && (datahead.np != fn/2) )
     {
        Werrprintf("fn mismatch between current(%d) and addsub(%d) phasefile",
                    fn,(int) (datahead.np*2));
        D_trash(D_USERFILE);
        return(ERROR);
     }

     if (replaceData && (trace == datahead.nblocks) )
        newbuf = 1;
     if (newbuf)
     {
        trace = datahead.nblocks;
        datahead.nblocks += 1;
        D_updatehead(D_USERFILE, &datahead);
        add_new_file(new_file, trace, S_DATA|S_FLOAT|status1, mode1);
     }
     else
     { /* get phasfl block */
        if (trace >= datahead.nblocks)
        {
           Werrprintf("Trace %d does not exist", trace+1);
           D_trash(D_USERFILE);
           return(ERROR);
        }
        D_getbuf(D_USERFILE, 1, trace, new_file);
        new_file->head->status = 0;
        D_markupdated(D_USERFILE, trace);
     }

     file_id = (newbuf || fid) ? D_USERFILE : D_DATAFILE;
     D_release(file_id, trace);
     D_close(file_id);
   }

/**************************************************
*  Either the FID or the DATA file is now opened  *
*  in the current experiment directory.           *
**************************************************/

     file_id = (newbuf || fid) ? D_USERFILE : D_DATAFILE;
     strcpy(path, exppath);
     strcat(path, (fid) ? "/acqfil/fid" : "/datdir/data");

     if ( (r = D_open(file_id, path, &datahead)) )
     {
        Werrprintf("cannot open addsub exp data file: error = %d", r);
        return(ERROR);
     }
     old_blocks = datahead.nblocks;
     fidhead_status = datahead.status;
     if ( ! range && (datahead.np != fn) )
     {
        Werrprintf("fn mismatch between current(%d) and addsub(%d) data",
                    fn,(int) datahead.np);
        D_close(file_id);
        return(ERROR);
     }

     if (trace == datahead.nblocks)
        newbuf = 1;
     if ( newbuf )
     {
        trace = datahead.nblocks;
        datahead.nblocks += 1;

        D_updatehead(file_id, &datahead);
        add_new_file(new_file, trace, (datahead.status & 0x3f), mode1);

		/* the 0x3f bit mask keeps only the status bits
		   which both file headers and block headers have
		   in common */
     }
     else if ( (r = D_getbuf(file_id, datahead.nblocks, trace, new_file)) )
     {
        Werrprintf("cannot get addsub exp data file: error = %d", r);
        D_close(file_id);
        return(ERROR);
     }
  }

  return(COMPLETE);
}


/***************************/
static void new_exp(int expn, int retc, char *retv[])
/***************************/
{ int lsfid_on,phfid_on;
  int d2flag;
  double rni;
  vInfo dummy;
  char *argv2[3];
  char expnum[32];

  sprintf(expnum,"%d",expn);
  argv2[0] = "cexp";
  argv2[1] = expnum;
  argv2[2] = NULL;

  newbuf = TRUE;
  lsfid_on = ((P_getVarInfo(CURRENT,"lsfid",&dummy)==0)  && (dummy.active));
  if (lsfid_on)
    P_setactive(CURRENT,"lsfid",ACT_OFF);
  phfid_on = ((P_getVarInfo(CURRENT,"phfid",&dummy)==0)  && (dummy.active));
  if (phfid_on)
    P_setactive(CURRENT,"phfid",ACT_OFF);
  d2flag = ((P_getreal(PROCESSED,"ni",&rni,1) == 0) && (rni>1.5));
  if (d2flag)		/* What is going on here?  SF */
  {
    P_setreal(PROCESSED,"ni",0.0,0);
    P_setreal(CURRENT,"ni",0.0,0);
  }
  cexpCmd(2,argv2,retc,retv);
  if (lsfid_on)
    P_setactive(CURRENT,"lsfid",ACT_ON);
  if (phfid_on)
    P_setactive(CURRENT,"phfid",ACT_ON);
  if (d2flag)
  {
    P_setreal(PROCESSED,"ni",rni,0);
    P_setreal(CURRENT,"ni",rni,0);
  }
}

/**********************************************************/
static void combine_data(float *out1, float *in1, float mult1, float *in2,
                         float mult2, int shift, int incr, int pts, int mode)
/**********************************************************/
{
  if (debug1)
    Wscrprintf("starting spectral combination\n");
  if (shift>0)
    while (shift)
    {
      *out1 = *in1++ * mult1;
      out1 += incr;
      shift--;
    }
  else if (shift<0)
    in2 -= shift;
  if (mode == MIN_MODE)
  { register float temp1,temp2;

    while (pts--)
    {
      temp1 = mult1 * *in1++;
      temp2 = mult2 * *in2++;
      *out1 = (fabs((double) temp1) < fabs((double) temp2)) ? temp1 : temp2;
      out1 += incr;
    }
  }
  else if (mode == MAX_MODE)
  { register float temp1,temp2;

    while (pts--)
    {
      temp1 = mult1 * *in1++;
      temp2 = mult2 * *in2++;
      *out1 = (fabs((double) temp1) > fabs((double) temp2)) ? temp1 : temp2;
      out1 += incr;
    }
  }
  else
  {
    if (replaceData)
    {
       while (pts--)
       {
         *out1 = *in2++ * mult2;
         out1 += incr;
       }
    }
    else
    {
       while (pts--)
       {
         *out1 = *in2++ * mult2 + *in1++ * mult1;
         out1 += incr;
       }
    }
  }
  if (shift<0)
    while (shift++)
    {
      *out1 = *in1++ * mult1;
      out1 += incr;
    }
  if (debug1)
    Wscrprintf("finish spectral combination\n");
}

/********************************************/
void store_pars(double *rp, double *lp, double *lvl, double *tlt,
                int *norm, int *phase, int *absval, int *power, int save)
/********************************************/
{
  static double rp_save,lp_save,lvl_save,tlt_save;
  static int    dophase_save,norm_save;
  static int	doabsval_save,dopower_save;

  if (save)
  {
    rp_save = *rp;
    lp_save = *lp;
    lvl_save = *lvl;
    tlt_save = *tlt;
    norm_save = *norm;
    dophase_save = *phase;
    doabsval_save = *absval;
    dopower_save = *power;
  }
  else
  {
    *rp = rp_save;
    *lp = lp_save;
    *lvl = lvl_save;
    *tlt = tlt_save;
    *norm = norm_save;
    *phase = dophase_save;
    *absval = doabsval_save;
    *power = dopower_save;
  }
}


/*---------------------------------------
|					|
|	     save_range_data()/8		|
|					|
+--------------------------------------*/
/* spec1    real data from "exp5/datdir/phasefile"	   */
/* spec2    real, absorptive data from "curexp/datdir/data" */
/* tmp_spec real, dispersive data from "curexp/datdir/data" */
/* if interactive_addi, tmp_spec is empty buffer   */
static int save_range_data(char *exppath, float *spec1, double mult1, float *spec2,
              double mult2, float *tmp_spec, int shift, int addsub_mode, int pts)
{
  int		dummy,
		r;
  float		*newptr,
		*as_real;
  dfilehead	tmphead;
  dpointers	new_spec;


  if (!new_datafile(exppath))
  {
/* DATAFILE now refers to the "exp5/datdir/data" file.  We will not
   actually use this data but rather use the reserved memory as a
   place to put the addsub + curexp_data result. */
     if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, trace, &new_spec)) )
     {
        Werrprintf("cannot get addsub experiment data file:  error = %d", r);
        D_close(D_DATAFILE);
        D_trash(D_USERFILE);
        return(ERROR);
     }
  }
  else
  {
     Werrprintf("cannot get addsub experiment data file");
     D_trash(D_USERFILE);
     return(ERROR);
  }

/* freeze the display mode of the addsub data
   file if any non-phaseable data is added in */
  newptr = (float *)new_spec.data;
  new_spec.head->status &= (~NP_CMPLX);
  if (save_complex)
  {
     new_spec.head->status |= NP_CMPLX;

     as_real = (float *) allocateWithId((fn/2) * sizeof(float), "addi");
     if (as_real == NULL)
     {
        Werrprintf("cannot allocate temporary buffer");
        D_close(D_DATAFILE);
        D_trash(D_USERFILE);
        return(ERROR);
     }
     movmem((char *)spec1, (char *)as_real, (fn/2)*sizeof(float), 1, 1);
  }
  else
  {
     as_real = spec1;
  }

  addsub_pars(exppath, FALSE);

  if (save_complex)
  {
     int save_d2flag = d2flag;
     int save_specperblock = specperblock;
     float *imag_ptr;

     specperblock = 1;		/* added for crude 2D */
     d2flag = FALSE;		/* added for crude 2D */
     rp += 90.0;	/* get the imaginary part of the addsub DATA */
     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;
     if ((spec1 = calc_user(trace, 0, FALSE, FALSE, &dummy,D_USERFILE)) == NULL)
     {
        Werrprintf("cannot get imaginary part of addsub data");
        D_close(D_DATAFILE);
        D_trash(D_USERFILE);
        return(ERROR);
     }

     if (interactive_addi)
        imag_ptr = addi_cur_imag;
     else
        imag_ptr = tmp_spec;
     combine_data(newptr + shift*2 + 1, spec1+shift, (float)mult1, imag_ptr, (float)mult2,
		    0, 2, pts, addsub_mode);
     rp -= 90.0;	/* reset internal value of "rp" */
     d2flag = save_d2flag;
     specperblock = save_specperblock;
  }

  combine_data(newptr + shift*2, as_real+shift, (float)mult1, spec2, (float)mult2,
		 0, 2, pts, addsub_mode);

  store_pars(&rp, &lp, &lvl, &tlt, &normflag, &dophase, &doabsval,
		&dopower, FALSE);
  if ( (r = D_markupdated(D_DATAFILE, trace)) )
  {
     Werrprintf("cannot update addsub experiment data file: error = %d", r);
     D_close(D_DATAFILE);
     D_trash(D_USERFILE);
     return(ERROR);
  }

/* USERFILE refers to the "exp5/datdir/phasefile" file. */
  r = D_gethead(D_USERFILE, &tmphead);
  tmphead.status = 0;
  if ( (r=D_getbuf(D_USERFILE,1,trace,&c_block)) ) /* get a spectrum */
  {
    if ( (r=D_allocbuf(D_USERFILE,trace,&c_block)) )
    {
      Werrprintf("cannot get addsub exp spectrum: error = %d",r);
      return(ERROR);
    }
  }
  c_block.head->status = 0;
  if (!r)
     r = D_updatehead(D_USERFILE, &tmphead);
  if (r)
  {
     Werrprintf("cannot update addsub experiment phase file header");
     D_close(D_DATAFILE);
     D_trash(D_USERFILE);
     return(ERROR);
  }
  if ( (r = D_markupdated(D_USERFILE, trace)) )
  {
     Werrprintf("cannot update fid file: error = %d", r);
  }
  else if ( (r = D_release(D_USERFILE, trace)) )
  {
     D_error(r);
  }


  D_release(D_DATAFILE, trace);
  D_close(D_USERFILE);
  save_addsub_pars();
  return(COMPLETE);
}

/*---------------------------------------
|					|
|	     save_data()/8		|
|					|
+--------------------------------------*/
/* spec1    real data from "exp5/datdir/phasefile"	   */
/* spec2    real, absorptive data from "curexp/datdir/data" */
/* tmp_spec real, dispersive data from "curexp/datdir/data" */
/* if interactive_addi, tmp_spec is empty buffer   */
int save_data(char *exppath, float *spec1, double mult1, float *spec2,
              double mult2, float *tmp_spec, int shift, int addsub_mode)
{
  int		dummy,
		pts,
		r;
  float		*newptr,
		*as_real;
  dfilehead	tmphead;
  dpointers	new_spec;


  pts = (fn/2) - abs(shift);
  if (!new_datafile(exppath))
  {
/* DATAFILE now refers to the "exp5/datdir/data" file.  We will not
   actually use this data but rather use the reserved memory as a
   place to put the addsub + curexp_data result. */
     if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, trace, &new_spec)) )
     {
        Werrprintf("cannot get addsub experiment data file:  error = %d", r);
        D_close(D_DATAFILE);
        D_trash(D_USERFILE);
        return(ERROR);
     }
  }
  else
  {
     Werrprintf("cannot get addsub experiment data file");
     D_trash(D_USERFILE);
     return(ERROR);
  }

/* freeze the display mode of the addsub data
   file if any non-phaseable data is added in */
  newptr = (float *)new_spec.data;
  new_spec.head->status &= (~NP_CMPLX);
  if (save_complex)
  {
     new_spec.head->status |= NP_CMPLX;

     as_real = (float *) allocateWithId((fn/2) * sizeof(float), "addi");
     if (as_real == NULL)
     {
        Werrprintf("cannot allocate temporary buffer");
        D_close(D_DATAFILE);
        D_trash(D_USERFILE);
        return(ERROR);
     }
     movmem((char *)spec1, (char *)as_real, (fn/2)*sizeof(float), 1, 1);
  }
  else
  {
     as_real = spec1;
  }

  addsub_pars(exppath, FALSE);

  if (save_complex)
  {
     int save_d2flag = d2flag;
     int save_specperblock = specperblock;
     float *imag_ptr;

     specperblock = 1;		/* added for crude 2D */
     d2flag = FALSE;		/* added for crude 2D */
     rp += 90.0;	/* get the imaginary part of the addsub DATA */
     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;
     if ((spec1 = calc_user(trace, 0, FALSE, FALSE, &dummy,D_USERFILE)) == NULL)
     {
        Werrprintf("cannot get imaginary part of addsub data");
        D_close(D_DATAFILE);
        D_trash(D_USERFILE);
        return(ERROR);
     }

     if (interactive_addi)
        imag_ptr = addi_cur_imag;
     else
        imag_ptr = tmp_spec;
     combine_data(newptr + 1, spec1, (float)mult1, imag_ptr, (float)mult2,
		    shift, 2, pts, addsub_mode);
     rp -= 90.0;	/* reset internal value of "rp" */
     d2flag = save_d2flag;
     specperblock = save_specperblock;
  }

  combine_data(newptr, as_real, (float)mult1, spec2, (float)mult2,
		 shift, 2, pts, addsub_mode);

  store_pars(&rp, &lp, &lvl, &tlt, &normflag, &dophase, &doabsval,
		&dopower, FALSE);
  if ( (r = D_markupdated(D_DATAFILE, trace)) )
  {
     Werrprintf("cannot update addsub experiment data file: error = %d", r);
     D_close(D_DATAFILE);
     D_trash(D_USERFILE);
     return(ERROR);
  }

/* USERFILE refers to the "exp5/datdir/phasefile" file. */
  r = D_gethead(D_USERFILE, &tmphead);
  tmphead.status = 0;
  if ( (r=D_getbuf(D_USERFILE,1,trace,&c_block)) ) /* get a spectrum */
  {
    if ( (r=D_allocbuf(D_USERFILE,trace,&c_block)) )
    {
      Werrprintf("cannot get addsub exp spectrum: error = %d",r);
      return(ERROR);
    }
  }
  c_block.head->status = 0;
  if (!r)
     r = D_updatehead(D_USERFILE, &tmphead);
  if (r)
  {
     Werrprintf("cannot update addsub experiment phase file header");
     D_close(D_DATAFILE);
     D_trash(D_USERFILE);
     return(ERROR);
  }
  if ( (r = D_markupdated(D_USERFILE, trace)) )
  {
     Werrprintf("cannot update fid file: error = %d", r);
  }
  else if ( (r = D_release(D_USERFILE, trace)) )
  {
     D_error(r);
  }


  D_release(D_DATAFILE, trace);
  D_close(D_USERFILE);
  save_addsub_pars();
  return(COMPLETE);
}

static void updateBlocks(int blocks, int status, int trace)
{
     int i;
     int ival __attribute__((unused));
     dpointers fid_block;
     for (i=0; i< blocks; i++)
     {
        if (i != trace)
        {
           ival = D_getbuf( D_USERFILE, 1, i, &fid_block );
           fid_block.head->status = status;
           ival = D_markupdated( D_USERFILE, i );
           ival = D_release( D_USERFILE, i );
        }
     }
}

static int keepFids(char *exppath, int lastFid, char *msg)
{
   char	path[MAXPATH];
   int	r;
   off_t length = 0;
   int updateFhead = 0;

   D_trash(D_USERFILE); 
   strcpy(path, exppath);
   strcat(path,"/acqfil/fid");

   if (make_copy_fidfile("clradd", exppath, msg))
      return(-1);
   if ( (r=D_open(D_USERFILE,path,&datahead)) )
   {
       sprintf(msg,"cannot open addsub fid file: %s",D_geterror(r));
       return(-1);
   }
   if (lastFid > datahead.nblocks)
      lastFid = datahead.nblocks;
   if (datahead.ebytes == 4)
   {
      int i;
      int ival __attribute__((unused));
      dpointers fid_block;
      int np;
      
      if ( (datahead.status & (S_DATA|S_FLOAT|S_COMPLEX)) == (S_DATA|S_FLOAT|S_COMPLEX) )
      {
         for (i=0; i< lastFid; i++)
         {
            ival = D_getbuf( D_USERFILE, 1, i, &fid_block );
            if ( fid_block.head->scale || (fid_block.head->ctcount > 1) )
            {
               int shift;
               float rmult = 1.0;
               float *ptr;

               shift = 1 << abs(fid_block.head->scale);
               if (fid_block.head->scale < 0)
                  rmult = 1.0/(float)(shift);
               else
                  rmult = (float) shift;
               rmult /= (float)(fid_block.head->ctcount);
               fid_block.head->ctcount = 1;
               fid_block.head->scale = 0;
               np = datahead.np;
               ptr = (float *)fid_block.data;
               while (np--)
                  *ptr++ *= rmult;
            
               ival = D_markupdated( D_USERFILE, i );
            }
            ival = D_release( D_USERFILE, i );
         }
      }
      else
      {
         datahead.status = (S_DATA|S_FLOAT|S_COMPLEX);
         updateFhead = 1;
         for (i=0; i< lastFid; i++)
         {
            int shift;
            float rmult = 1.0;
            float *ptr;
            int *iptr;

            ival = D_getbuf( D_USERFILE, 1, i, &fid_block );
            shift = 1 << abs(fid_block.head->scale);
            if (fid_block.head->scale < 0)
               rmult = 1.0/(float)(shift);
            else
               rmult = (float) shift;
            rmult /= (float)(fid_block.head->ctcount);
            fid_block.head->ctcount = 1;
            fid_block.head->scale = 0;
            fid_block.head->status = datahead.status;
            np = datahead.np;
            ptr = (float *)fid_block.data;
            iptr = (int *)fid_block.data;
            while (np--)
               *ptr++ = (float) (*iptr++) * rmult;
            ival = D_markupdated( D_USERFILE, i );
            ival = D_release( D_USERFILE, i );
         }
      }
   }
   else
   {
      char tmppath[MAXPATH];
      dfilehead new_datahead;
      dpointers new_block;
      dpointers fid_block;
      int r;
      int i;
      int np;

      strcpy(tmppath,path);
      strcat(tmppath,"tmp");
      new_datahead.nblocks = lastFid;
      new_datahead.ntraces = datahead.ntraces;
      new_datahead.np      = datahead.np;
      new_datahead.vers_id = datahead.vers_id;
      new_datahead.nbheaders = 1;
      new_datahead.ebytes  = 4;
      new_datahead.tbytes  = new_datahead.ebytes * new_datahead.np;
      new_datahead.bbytes  = new_datahead.tbytes * new_datahead.ntraces +
                           sizeof(dblockhead);
      new_datahead.status  = S_DATA|S_FLOAT|S_COMPLEX;
      D_trash(D_PHASFILE);

      if ( (r=D_newhead(D_PHASFILE,tmppath,&new_datahead)) )
      { D_error(r); return(1);
      }
      for (i=0; i< lastFid; i++)
      {
         int shift;
         float rmult = 1.0;
         float *ptr;
         short *iptr;

         r = D_getbuf( D_USERFILE, 1, i, &fid_block );
         shift = 1 << abs(fid_block.head->scale);
         if (fid_block.head->scale < 0)
            rmult = 1.0/(float)(shift);
         else
            rmult = (float) shift;
         rmult /= (float)(fid_block.head->ctcount);
         np = datahead.np;
         iptr = (short *)fid_block.data;
         if ( (r=D_allocbuf(D_PHASFILE,i,&new_block)) )
         { D_error(r); return(1);
         }
         new_block.head->index  = i+1;
         new_block.head->scale  = 0;
         new_block.head->status = new_datahead.status;
         new_block.head->mode   = 0;
         new_block.head->rpval  = 0;
         new_block.head->lpval  = 0;
         new_block.head->lvl    = 0;
         new_block.head->tlt    = 0;
         new_block.head->ctcount= 1;
         ptr = (float *)new_block.data;
         while (np--)
            *ptr++ = (float) (*iptr++) * rmult;
         r = D_markupdated( D_PHASFILE, i );
         r = D_release( D_PHASFILE, i );
         r = D_release( D_USERFILE, i );
      }
      D_trash(D_USERFILE);
      D_close(D_PHASFILE);
      unlink(path);
      rename(tmppath,path);
      lastFid = datahead.nblocks;
      length = 0;
   }
   if ( (lastFid < datahead.nblocks) || updateFhead )
   {
      datahead.nblocks = lastFid;
      r = D_updatehead(D_USERFILE, &datahead);
      length = datahead.nblocks * datahead.bbytes + sizeof(dfilehead);
      if ( D_fidversion() == 0)
      {
         updateBlocks(datahead.nblocks, datahead.status, lastFid);
      }
   }
   D_close(D_USERFILE);
   if (length > 0)
      truncate(path,length);
   return(0);

}

/*---------------------------------------
|					|
|		addfid()/6		|
|					|
+--------------------------------------*/
static int addfid(int argc, char *argv[], int retc, char *retv[],
                  char *exppath, int newexp, int inCurexp)
{
  int		r,
		np,
		do_sub;
  float		*fidptr,
		*newptr;
  dpointers	old_fid,
		new_fid;


  (void) argc;
  (void) retc;
  (void) retv;
  do_sub = (strcmp(argv[0], "sub") == 0);
  revflag = FALSE;
  if (initfid(1))
     return(ERROR);

  if (specIndex < 1)
     specIndex = 1;
  np = -fn;
  if ((fidptr =  get_one_fid(specIndex-1, &np, &old_fid, FALSE)) == NULL)
     return(ERROR);
  if (debug1)
    Wscrprintf("first fid point is %g\n", *fidptr);

  save_complex = TRUE;
  if (do_sub)
  {
     disp_status("FID SUB ");
  }
  else
  {
     disp_status("FID ADD ");
  }

  if (get_addsub_data(newexp, &new_fid, exppath, 0, S_COMPLEX, 0, 0, TRUE, FALSE))
  {
     return(ERROR);
  }
  if (inCurexp && ( D_fidversion() == 0) )
  {
     updateBlocks(old_blocks, fidhead_status, trace);
  }
  newptr = (float *)new_fid.data;
  if (newbuf)
  {
     if (new_fid.head->ctcount == 0)
        new_fid.head->ctcount = 1;
     new_fid.head->lpval = get_dsp_lp();
  }
  if (debug1)
     Wscrprintf("first of %d fid points are %g and %g\n",np,*fidptr,*newptr);

  if (do_sub)
     multiplier *= -1.0;
  if (newbuf || replaceData)
  {
     new_fid.head->ctcount = 1;
     new_fid.head->scale = 0;
     if ( (new_fid.head->status & (S_DATA|S_FLOAT|S_COMPLEX) ) ==
           (S_DATA|S_FLOAT|S_COMPLEX) )
     {
        while (np--)
           *newptr++ = *fidptr++ * multiplier;
     }
     else
     {
        if (datahead.ebytes == 2)
        {
           short *inp16;
           inp16 = (short *) new_fid.data;
           while (np--)
              *inp16++ = (short) (*fidptr++ * multiplier);
        }
        else
        {
           int *inp32;
           inp32 = (int *) new_fid.data;
           while (np--)
              *inp32++ = (int) (*fidptr++ * multiplier);
        }
     }
  }
  else
  {
     int shift;
     float rmult = 1.0;
     if ( new_fid.head->scale || (new_fid.head->ctcount > 1) )
     {
        shift = 1 << abs(new_fid.head->scale);
        if (new_fid.head->scale < 0)
           rmult = 1.0/(float)(shift);
        else
           rmult = (float) shift;
        rmult /= (float)(new_fid.head->ctcount);
        new_fid.head->ctcount = 1;
        new_fid.head->scale = 0;
        shift = 1;
     }
     else
     {
        shift = 0;
     }
     if ( (new_fid.head->status & (S_DATA|S_FLOAT|S_COMPLEX) ) ==
           (S_DATA|S_FLOAT|S_COMPLEX) )
     {
        if ( shift )
        {
           float *ptr;
           int tmpNp;

           ptr = newptr;
           tmpNp = np;
           while (tmpNp--)
              *ptr++ *= rmult;
        }
        while (np--)
           *newptr++ += *fidptr++ * multiplier;
     }
     else
     {
        if (datahead.ebytes == 2)
        {
           short *inp16;
           inp16 = (short *) new_fid.data;
           if ( shift )
           {
              short *ptr;
              int tmpNp;
              float tmp;

              ptr = inp16;
              tmpNp = np;
              while (tmpNp--)
              {
                 tmp = (float) *ptr;
                 tmp *= rmult;
                 *ptr++ = (short) tmp;
              }
           }
           while (np--)
              *inp16++ += (short) (*fidptr++ * multiplier);
        }
        else
        {
           int *inp32;
           inp32 = (int *) new_fid.data;
           if ( shift )
           {
              int *ptr;
              int tmpNp;
              float tmp;

              ptr = inp32;
              tmpNp = np;
              while (tmpNp--)
              {
                 tmp = (float) *ptr;
                 tmp *= rmult;
                 *ptr++ = (int) tmp;
              }
           }
           while (np--)
              *inp32++ += (int) (*fidptr++ * multiplier);
        }
     }
  }

  if ( (r = D_markupdated(D_USERFILE, trace)) )
  {
     Werrprintf("cannot update fid file: error = %d", r);
  }
  else if ( (r = D_release(D_USERFILE, trace)) )
  {
     D_error(r);
  }

  D_close(D_USERFILE);
  D_trash(D_PHASFILE);
  return(COMPLETE);
}


/**********************************/
static int save_addsub_pars()
/**********************************/
{
  int  r;

  if ( (r=P_setreal(TEMPORARY,"rp",0.0,1)) )
    { P_err(r,"rp",":"); return(ERROR); }
  if ( (r=P_setreal(TEMPORARY,"lp",0.0,1)) )
    { P_err(r,"lp",":"); return(ERROR); }
  if ( (r=P_setreal(TEMPORARY,"lvl",0.0,1)) )
    { P_err(r,"lvl",":"); return(ERROR); }
  if ( (r=P_setreal(TEMPORARY,"tlt",0.0,1)) )
    { P_err(r,"tlt",":"); return(ERROR); }
  return(COMPLETE);
}

/*****************************/
int addsub_pars(char exppath[],int read_tree)
/*****************************/
{
  char path[MAXPATH];
  char aig[5],dmg[5];
  int  r;

  if (read_tree)
  {
    strcpy(path,exppath);
#ifdef UNIX
    strcat(path,"/curpar");
#else 
    strcat(path,"curpar");
#endif 
    P_treereset(TEMPORARY);		/* clear the tree first */
    if (P_read(TEMPORARY,path))
    {
      Werrprintf("error reading %s parameters",path);
      return(ERROR);
    }
  }
  if ( (r=P_getstring(TEMPORARY,"aig",aig,1,4)) )
    { P_err(r,"aig",":"); return(ERROR); }
  normflag = (aig[0] == 'n');
  if ( (r=P_getstring(TEMPORARY,"dmg",dmg,1,4)) )
    { P_err(r,"dmg",":"); return(ERROR); }
  dophase = ((dmg[0] == 'p') && (dmg[1] == 'h'));
  doabsval = ((dmg[0] == 'a') && (dmg[1] == 'v'));
  dopower = ((dmg[0] == 'p') && (dmg[1] == 'w'));
  if ( (r=P_getreal(TEMPORARY,"rp",&rp,1)) )
    { P_err(r,"rp",":"); return(ERROR); }
  if ( (r=P_getreal(TEMPORARY,"lp",&lp,1)) )
    { P_err(r,"lp",":"); return(ERROR); }
  if ( (r=P_getreal(TEMPORARY,"lvl",&lvl,1)) )
    { P_err(r,"lvl",":"); return(ERROR); }
  if ( (r=P_getreal(TEMPORARY,"tlt",&tlt,1)) )
    { P_err(r,"tlt",":"); return(ERROR); }
  return(COMPLETE);
}

static int correct_fn_sw(char *exppath )
{
	char	path[ MAXPATH ];
	int	fnparam, fnval, r;
	double	qval, ref, swval;

	strcpy(path,exppath);
#ifdef UNIX
	strcat(path,"/procpar");	/* required to work with processed params */
#else 
	strcat(path,"procpar");
#endif 
	P_treereset( TEMPORARY );	/* clear the tree first */
	if (P_read( TEMPORARY, path )) {
		Werrprintf( "error reading %s parameters", path );
		return( ERROR );
	}

	get_ref_pars( HORIZ, &swval, &ref, &fnval );

        fnparam = 0;
	r = P_getreal( TEMPORARY, "fn", &qval, 1 );
	if (r == 0)
	  fnparam = (int) (qval+0.5);

	if (r != 0 || fnval != fnparam) {
		P_setreal( TEMPORARY, "fn", (double) fnval, 1 );
		P_setreal( TEMPORARY, "sw", swval, 1 );
		if (P_save( TEMPORARY, path )) {
			Werrprintf( "error writing %s parameters", path );
			return( ERROR );
		}
	}
        return(COMPLETE);
}

/*---------------------------------------
|					|
|		addspec()/6		|
|					|
+--------------------------------------*/
static int addspec(int argc, char *argv[], int retc, char *retv[],
                   char *exppath, int newexp)
{
  char          *dsplymessage,
                path[MAXPATH];
  int           r,
                dummy,
                addsub_mode,
		display_mode,
		// orig_d2flag,
                pts,
		stat;
  float         *spectrum,
                *newptr,
		*cur_real,
		*cur_imag,
		*as_real;
  double        scale,
                addsub_scale;
  dpointers     new_spec;
  int           rangePt1 = 0;
  int           rangePt2 = 0;


  (void) argc;
  (void) retc;
  (void) retv;
  dsplymessage = "";	/* initialization */
  addsub_mode = ADD_MODE;
  cur_imag = NULL;

  if (strcmp(argv[0], "spsub") == 0)
  {
     addsub_mode = SUB_MODE;
     dsplymessage = "SPEC SUB";
  }
  else if (strcmp(argv[0], "spadd") == 0)
  {
     addsub_mode = ADD_MODE;
     dsplymessage = "SPEC ADD";
  }
  else if (strcmp(argv[0], "spmin") == 0)
  {
     addsub_mode = MIN_MODE;
     dsplymessage = "SPEC MIN";
  }
  else if (strcmp(argv[0], "spmax") == 0)
  {
     addsub_mode = MAX_MODE;
     dsplymessage = "SPEC MAX";
  }

  revflag = FALSE;	/* necessary if data turn out to be 1D */
  if (init2d(1, 1))
     return(ERROR);	/* d2flag, revflag, fn, fn1, and the DATAFILE
			   file header for the current experiment are
			   now properly set.  "dophase", "doabsval",
			   and "dopower" refer to the current experi-
			   ment at this time.  */

  if (datahead.status & S_HYPERCOMPLEX)
  {
     Werrprintf("addsub currently unsupported for hypercomplex data");
     disp_status("        ");
     return(ERROR);
  }

/*  addsub only works with 1D traces, even in 2D planes.
    If the alternate direction was chosen (trace = 'f1', for example),
    the values for fn and sw will not be correct in the add/sub data
    buffer.  The correct_fn_sw program will substitute the right values
    for these two parameters if necessary in the add/sub buffer.  This
    operation is only necessary for a newly created add/sub data buffer
    (newexp == TRUE).  In subsequent add/sub operations, the new trace
    must be the same size as the older trace.				*/

  if (d2flag && newexp)
    correct_fn_sw( exppath );

  disp_status(dsplymessage);
  if (specIndex < 1)
    specIndex = 1;

  if (dophase)
  {
     display_mode = NP_PHMODE;
  }
  else if (doabsval)
  {
     display_mode = NP_AVMODE;
  }
  else	/* default to "power" */
  {
     display_mode = NP_PWRMODE;
  }

  save_complex = (dophase && (datahead.status & S_COMPLEX));
  if (save_complex)
  {
     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;
     rp += 90.0;
  }

/* "spectrum" points to the dispersive data in the current
   experiment if one is saving complex data; otherwise, it
   points to the absorptive (or displayed) data in the cur-
   rent experiment. */

  if ((spectrum = calc_spec(specIndex-1, 0, FALSE, TRUE, &dummy)) == NULL)
     return(ERROR);

  if (save_complex)
  {
     cur_imag = (float *) allocateWithId(sizeof(float) * fn/2, "addi");
     if (cur_imag == NULL)
     {
        Werrprintf("cannot allocate temporary buffer");
        return(ERROR);
     }

     movmem((char *)spectrum, (char *)cur_imag, (fn/2)*sizeof(float), 1, 1);

     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;
     rp -= 90.0;

     if ((spectrum = calc_spec(specIndex-1, 0, FALSE, TRUE, &dummy))
	     == NULL)
     {
        releaseAllWithId("addi");
        return(ERROR);
     }

/* If one is saving complex data, "spectrum" now points to the
   real data and "cur_imag", to the imaginary data. */
  }


  scale = ( (normflag) ? normalize : 1.0 );
  stat = (S_SPEC | S_COMPLEX);
  if (get_addsub_data(newexp, &new_spec, exppath, S_SPEC, stat,
			display_mode, display_mode, FALSE, range))
  {
     releaseAllWithId("addi");
     return(ERROR);
  }

  addsub_complex = (new_spec.head->status & S_COMPLEX);  /* always TRUE now */
  specperblock = 1;		/* added for crude 2D */
  // orig_d2flag = d2flag;
  d2flag = FALSE;		/* added for crude 2D */

  newptr = (float *)new_spec.data;
  multiplier *= scale;
  if (addsub_mode == SUB_MODE)
     multiplier *= -1.0;

  if (range)
  {
     double rPt1, rPt2;
     if (rangeHighField == NULL)
     {
        rPt2 = sp;
     }
     else
     {
        rPt2 = stringReal(rangeHighField);
     }
     if (rangeLowField == NULL)
     {
        rPt1 = sp + wp;
     }
     else
     {
        rPt1 = stringReal(rangeLowField);
     }
     rangePt1 =  datapoint(rPt1 + rflrfp, sw, fn/2);
     if (rangePt1 > fn/2)
       rangePt1 = fn/2;
     else if (rangePt1 < 0)
       rangePt1 = 0;
     rangePt2 =  datapoint(rPt2 + rflrfp, sw, fn/2);
     if (rangePt2 > fn/2)
       rangePt2 = fn/2;
     else if (rangePt2 < 0)
       rangePt2 = 0;
     /* supplied in wrong order */
     if (rangePt1 > rangePt2)
     {
        int tmp;
        tmp = rangePt1;
        rangePt1 = rangePt2;
        rangePt2 = tmp;
     }
  }

  if (newbuf)
  {
     if (range)
     {
        zerofill(newptr,datahead.np );
        spectrum += rangePt1;
        cur_imag += rangePt1;
        pts = rangePt2 - rangePt1;
        if (shift < 0)
        {
            rangePt1 += shift;
            if (rangePt1 < 0)
               rangePt1 = 0;  
        }
        else if (shift > 0)
        {
            rangePt1 += shift;
            if (rangePt1 + pts > datahead.np/2)
               rangePt1 = datahead.np/2 - pts;  
        }
        newptr += 2*rangePt1;

        if (save_complex)
        {
           new_spec.head->status |= NP_CMPLX;
           while (pts--)
           {
              *newptr++ = (*spectrum++) * multiplier;
              *newptr++ = (*cur_imag++) * multiplier;
           }
        }
        else
        {
           new_spec.head->status &= (~NP_CMPLX);
           while (pts--)
           {
              *newptr++ = (*spectrum++) * multiplier;
              *newptr++ = 0.0;
           }
        }
     }
     else
     {
        if (shift > 0)
        {
           zerofill(newptr, 2*shift);
           newptr += 2*shift;
        }
        else if (shift < 0)
        {
           spectrum -= shift;
           if (save_complex)
              cur_imag -= shift;
        }
        pts = (fn/2) - abs(shift);
        if (save_complex)
        {
           new_spec.head->status |= NP_CMPLX;
           while (pts--)
           {
              *newptr++ = (*spectrum++) * multiplier;
              *newptr++ = (*cur_imag++) * multiplier;
           }
        }
        else
        {
           new_spec.head->status &= (~NP_CMPLX);
           while (pts--)
           {
              *newptr++ = (*spectrum++) * multiplier;
              *newptr++ = 0.0;
           }
        }

        if (shift < 0)
           zerofill(newptr, (-2)*shift);
     }

     if ( (r = D_markupdated(D_USERFILE, trace)) )
     {
        Werrprintf("cannot update data file: error = %d", r);
     }
     else if ( (r = D_release(D_USERFILE, trace)) )
     {
        D_error(r);
     }
  }
  else
  {
     pts = (fn/2) - abs(shift);
     addsub_scale = normflag ? normalize : 1.0;
     store_pars(&rp, &lp, &lvl, &tlt, &normflag, &dophase, &doabsval,
			&dopower, TRUE);
     addsub_pars(exppath, TRUE);
     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;

     cur_real = (float *) allocateWithId(sizeof(float) * fn/2, "addi");
     if (cur_real == NULL)
     {
        Werrprintf("cannot allocate temporary buffer");
        return(ERROR);
     }

     movmem((char *)spectrum, (char *)cur_real, (fn/2)*sizeof(float), 1, 1);
     if (range)
     {
        cur_real += rangePt1;
        cur_imag += rangePt1;
        pts = rangePt2 - rangePt1;
        fn = datahead.np;
        shift += rangePt1;
        if (shift < 0)
        {
            shift = 0;  
        }
        else if (shift + pts > fn/2)
        {
            shift = fn/2 - pts;  
        }
     }
     if ( (as_real = calc_user(trace, 0, FALSE, FALSE, &dummy,D_USERFILE)) == NULL )
     {
        Werrprintf("cannot get addsub real data");
        D_trash(D_USERFILE);
        D_close(D_DATAFILE);
        releaseAllWithId("addi");
        return(ERROR);
     }

     if (range)
     {
        if (save_range_data(exppath, as_real, addsub_scale, cur_real,
		multiplier, cur_imag, shift, addsub_mode, pts))
        {
           releaseAllWithId("addi");
           D_trash(D_USERFILE);
           D_close(D_DATAFILE);
           return(ERROR);
        }
     }
     else
     {
        if (save_data(exppath, as_real, addsub_scale, cur_real,
		multiplier, cur_imag, shift, addsub_mode))
        {
           releaseAllWithId("addi");
           D_trash(D_USERFILE);
           D_close(D_DATAFILE);
           return(ERROR);
        }
     }

     strcpy(path,exppath);
#ifdef UNIX
     strcat(path,"/curpar");
#else 
     strcat(path,"curpar");
#endif 

     if (P_save(TEMPORARY, path))
     {
        Werrprintf("error writing addsub parameters");
        D_trash(D_USERFILE); 
        D_close(D_DATAFILE);
        releaseAllWithId("addi");
        return(ERROR);
     }

     P_treereset(TEMPORARY);		/* clear the tree again */
   }

   D_close(D_USERFILE);
   D_close(D_DATAFILE);
   releaseAllWithId("addi");
   return(COMPLETE);
}


/**************************************/
static int checkinput(int argc, char *argv[], int newexp)
/**************************************/
{ int do_shift = FALSE;

  trace = 1;
  range = 0;
  rangeHighField = rangeLowField = NULL;
  while (--argc)
    if (isReal(*++argv))
      if (do_shift)
        shift = (int) stringReal(*argv);
      else
      {
        multiplier = stringReal(*argv);
        do_shift = TRUE;
      }
    else if ((!newexp) && (strcmp(*argv,"new") == 0))
      newbuf = 1;
    else if ( (strcmp(*argv,"trace") == 0) || (strcmp(*argv,"newtrace") == 0) )
    {
       if (strcmp(*argv,"newtrace") == 0)
          replaceData = 1;
       argc--;
       if (argc && isReal(*++argv))
       {
          trace = (int) stringReal(*argv);
          if (trace < 1)
            trace = 1;
       }
       else
       {
          Werrprintf("trace option for add/sub requires a numeric index\n");
          return(-1);
       }
    }
    else if (strcmp(*argv,"range") == 0)
    {
       range = 1;
       if ( (argc > 1) && isReal(*(argv+1)))
       {
          argv++;
          argc--;
          rangeHighField = *argv;
          if ( (argc > 1) && isReal(*(argv+1)))
          {
             argv++;
             argc--;
             rangeLowField = *argv;
          }
       }
    }
  if (debug1)
    Wscrprintf("selected trace is %d\n",trace);
  trace--;
  return(0);
}

/***************************/
static int addi(int argc, char *argv[],int retc, char *retv[],
                char *exppath)
/***************************/
{ int  r;
  float *newptr,*add_spec;
  char path[MAXPATH];
  double scale;
  int save_d2flag;
  int save_specperblock;
  dpointers     new_spec;

  (void) argc;
  (void) argv;
  (void) retc;
  (void) retv;
  revflag = FALSE;
  if(init2d(1, 1))
     return(ERROR);

  interactive_addi = TRUE;
  if (datahead.status & S_HYPERCOMPLEX)
  {
     Werrprintf("addsub currently unsupported for hypercomplex data");
     return(ERROR);
  }

  if (specIndex < 1)
    specIndex = 1;
  save_complex = (dophase && (datahead.status & S_COMPLEX));
  if (save_complex)
  {
     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;
     rp += 90.0;

     if ((newptr = calc_spec(specIndex-1, 0, FALSE, TRUE, &r)) == NULL)
        return(ERROR);

     addi_cur_imag = (float *) allocateWithId(sizeof(float) * fn/2, "addi");
     if (addi_cur_imag == NULL)
     {
        Werrprintf("cannot allocate temporary buffer");
        return(ERROR);
     }

     movmem((char *)newptr, (char *)addi_cur_imag, (fn/2)*sizeof(float), 1, 1);

     c_first = 32767;	/* mark no buffer in workspace */
     c_last = 0;
     c_buffer = -1;
     rp -= 90.0;

  }

  store_pars(&rp,&lp,&lvl,&tlt,&normflag,&dophase,&doabsval,&dopower,TRUE);
  addsub_pars(exppath,TRUE);
  D_trash(D_USERFILE);
  strcpy(path,exppath);
#ifdef UNIX
  strcat(path,"/datdir/phasefile");
#else 
  vms_fname_cat(path,"[.datdir]phasefile");
#endif 
  if ( (r=D_open(D_USERFILE,path,&datahead)) )
  {
    Werrprintf("cannot open addsub exp phase file: error = %d",r);
    return(ERROR);
  }
  if (datahead.np != fn/2)
  {
     Werrprintf("fn mismatch between current(%d) and addsub(%d) phasefile",
                 fn,(int) (datahead.np*2));
     D_trash(D_USERFILE);
     return(ERROR);
  }
  if ( (r=D_getbuf(D_USERFILE,1,trace,&c_block)) ) /* get a spectrum */
  {
    if ( (r=D_allocbuf(D_USERFILE,trace,&c_block)) )
    {
      Werrprintf("cannot get addsub exp spectrum: error = %d",r);
      return(ERROR);
    }
    c_block.head->status = 0;
  }
  if (new_datafile(exppath))
    return(ERROR);
  if (datahead.np != fn)
  {
     Werrprintf("fn mismatch between current(%d) and addsub(%d) data",
                 fn,(int) datahead.np);
     D_close(D_DATAFILE);
     return(ERROR);
  }
  if (D_getbuf(D_DATAFILE, datahead.nblocks, trace, &new_spec))
  {
     Werrprintf("cannot get addsub exp data file: error = %d", r);
     D_close(D_DATAFILE);
     return(ERROR);
  }
  addsub_complex = (new_spec.head->status & S_COMPLEX);  /* always TRUE now */
  save_complex = (save_complex && addsub_complex);
  c_first = 32767;	/* mark no buffer in workspace */
  c_last = 0;
  c_buffer = -1;
  save_d2flag = d2flag;
  save_specperblock = specperblock;
  specperblock = 1;		/* added for crude 2D */
  d2flag = FALSE;		/* added for crude 2D */
  if ((newptr = calc_user(trace,0,FALSE,TRUE,&r,D_USERFILE))==0)
  {
    Werrprintf("cannot get addsub exp spectral pointer: error = %d",r);
    return(ERROR);
  }
  d2flag = save_d2flag;
  specperblock = save_specperblock;
  scale = normflag ? normalize : 1.0;
  if (new_datafile(curexpdir))
    return(ERROR);
  c_first = 32767; c_last = 0; /* mark no buffer in workspace */
  c_buffer = -1;
  store_pars(&rp,&lp,&lvl,&tlt,&normflag,&dophase,&doabsval,&dopower,FALSE);
  if ((add_spec = (float *) allocateWithId(sizeof(float) * fn/2,"addi"))==0)
  {
    Werrprintf("cannot allocate phasing buffer");
    D_release(D_USERFILE,trace);
    D_close(D_USERFILE);
    return(ERROR);
  }
  start_addi(newptr,add_spec,scale,exppath);
  Wsetgraphicsdisplay("addi");
  return(COMPLETE);
}


/*---------------------------------------
|					|
|		chpar()/2		|
|					|
+--------------------------------------*/
static int chpar(int getflag, int specadd)
{
   static char		dpsave[4];
   static double	rpsave,
			lpsave;
   char			*parname;
   int			r,
			i;
   double		tmp;


   if (getflag)
   {
      if ( (r = P_getstring(PROCESSED, "dp", &dpsave[0], 1, 3)) )
      {
         P_err(r, "dp", ":");
         return(ERROR);
      }

      if ( (r = P_setstring(CURRENT, "dp", "y", 1)) )
      { 
         P_err(r, "dp", ":"); 
         return(ERROR); 
      }

      i = 2;
      while (i)
      {
         switch (i)
         {
            case 2:	parname = "rp"; break;
            case 1:	parname = "lp"; break;
            default:	parname = "rp";
         }

         if ( (r = P_getreal(CURRENT, parname, &tmp, 1)) )
         {
            P_err(r, parname, ":"); 
            return(ERROR); 
         }

         if (specadd)
         {
            if ( (r = P_setreal(CURRENT, parname, 0.0, 1)) )
            {
               P_err(r, parname, ":"); 
               return(ERROR); 
            }
         }

         switch (i)
         {
            case 2:	rpsave = tmp; break;
            case 1:	lpsave = tmp; break;
            default:	rpsave = tmp;
         }

         i -= 1;
      }
   }
   else
   {
      if ( (r = P_setstring(CURRENT, "dp", &dpsave[0], 1)) )
      {  
         P_err(r, "dp", ":"); 
         return(ERROR);  
      }

      i = 2;
      while (i)
      {
         switch (i)
         {
            case 2:	tmp = rpsave; parname = "rp"; break;
            case 1:	tmp = lpsave; parname = "lp"; break;
            default:	tmp = rpsave; parname = "rp";
         }

         if ( (r = P_setreal(CURRENT, parname, tmp, 1)) )
         {
            P_err(r, parname, ":"); 
            return(ERROR); 
         }

         i -= 1;
      }
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|		addsub()/4		|
|					|
|   Main entry point for the ADDSUB	|
|   functions.				|
|					|
+--------------------------------------*/
int addsub(int argc, char *argv[], int retc, char *retv[])
{
  char	exppath[MAXPATH];
  int	newexp,
	fidadd,
	specadd,
	clradd;
  double tmp;
  int   addsubExp;
  int   inCurexp;


  Wturnoff_buttons();
  D_allrelease();
#ifdef XXX
  /* Preliminary definition is to only check a global parameter */
  if ( P_getreal(CURRENT,"addsubexp",&tmp,1))
#endif
    if ( P_getreal(GLOBAL,"addsubexp",&tmp,1)) 
       tmp = 5.0;
  addsubExp = (int) (tmp+0.1);
  inCurexp = (addsubExp == expdir_to_expnum(curexpdir));

       
  sprintf(exppath, "%s/exp%d", userdir, addsubExp);
  newbuf = newexp = (access(exppath, 6));

  interactive_addi = FALSE;
  multiplier = 1.0;
  shift = 0;
  if (debug1)
     Wscrprintf("addsub exp %s\n", (newexp) ? "does not exist" : "exists");

  fidadd = specadd = FALSE;	/* initialization */
  replaceData = 0;
  clradd = ( strcmp(argv[0], "clradd") == 0 );

  if (!clradd)
  {
     fidadd = ( (strcmp(argv[0], "add") == 0) ||
		(strcmp(argv[0], "sub") == 0) );
     if (!fidadd)
     {
        specadd = ( (strcmp(argv[0], "spadd") == 0) ||
		    (strcmp(argv[0], "spsub") == 0) ||
		    (strcmp(argv[0], "spmax") == 0) ||
		    (strcmp(argv[0], "spmin") == 0) );
     }
  }

  if (!clradd)
  {
     if (checkinput(argc, argv, newexp))
        return(ERROR);
     if (newexp)
     {
        chpar(1, !fidadd);
        new_exp(addsubExp, retc, retv);
        chpar(0, !fidadd);
     }
  }

  if (clradd)
  {
     if (argc > 1)
     {
        int res;
	char msg[MAXPATH];

        trace = 0;
        if ( (argc == 3) && !strcmp(argv[1],"keep") )
        {
           if (isReal(argv[2]))
              trace = (int) stringReal(argv[2]);
        }
        res = -1;
        strcpy(msg,"");
        if ( ! newexp && (trace > 0) )
           res = keepFids(exppath,trace,msg);
        if (res == 0)
        {
           if (retc)
           {
              retv[ 0 ] = intString( 1 );
              if (retc > 1)
              {
                 sprintf(msg,"clradd kept %d fids",trace);
                 retv[ 1 ] = newString( msg );
              }
           }
        }
        else
        {
           if (trace <= 0)
              strcpy(msg,"clradd keep option requires an index greater than 0\n");
           else
              sprintf(msg,"cannot delete experiment %d, does not exist", addsubExp);
           if (retc)
           {
              retv[ 0 ] = intString( 0 );
              if (retc > 1)
              {
                 retv[ 1 ] = newString( msg );
              }
           }
        }
     }
     else if (!newexp && !inCurexp)
     {
        char *argv2[3];
        char expnum[32];

        sprintf(expnum,"%d",addsubExp);
        argv2[0] = "delexp";
        argv2[1] = expnum;
        argv2[2] = NULL;
        delexp(2,argv2,retc,retv);
     }
     else if (retc)
     {
        retv[ 0 ] = intString( 0 );
        if (retc > 1)
        {
	   char msg[MAXPATH];

           if (inCurexp)
              strcpy(msg,"cannot use clradd on currently active experiment");
           else
              sprintf(msg,"cannot delete experiment %d, does not exist", addsubExp);
           retv[ 1 ] = newString( msg );
        }
     }
     else
     {
           if (inCurexp)
              Werrprintf("cannot use clradd on currently active experiment");
           else
              Werrprintf("cannot delete experiment %d, does not exist", addsubExp);
     }
  }
  else if (fidadd)
  { /* add the current fid into the addsub buffer */
     if ( ! newexp )
     {
        make_copy_fidfile(argv[0], exppath, NULL);
     }
     addfid(argc, argv, retc, retv, exppath, newexp, inCurexp);
  }
  else if (specadd)
  { /* add the current spectrum into the addsub buffer */
     addspec(argc, argv, retc, retv, exppath, newexp);
  }
  else if (newbuf || newexp)
  { /* addi command */
     addspec(argc, argv, retc, retv, exppath, newexp);
  }
  else
  {
     addi(argc, argv, retc, retv, exppath);
  }

  return(COMPLETE);
}
