/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**************************************************************************/
/*						                          */
/*  init_proc.c -	procedures for localizing information about       */
/*                      processing parameters.  All details for           */
/*                      processing the Horizontal axis and the Vertical   */
/*                      axis is maintained here.                          */
/*  entry points include the procedures set_spec_proc() and               */
/*  set_fid_proc(),  and get_ref_pars()                                   */
/*						                          */
/*  set_spec_proc() maps selectable domains (e.g.,  FN0, or SW1) onto     */
/*  the Horizontal and Vertical axes.                                     */
/*  set_fid_proc() maps the SW0 domain onto both the Horizontal and       */
/*  Vertical axes.  There are no other options.                           */
/*  get_ref_pars() returns sw, fn, and rflrfp for a selected axis.        */
/*  For time domain axes,  the sw value is inverted and returned as time  */
/*  get_phase_mode() returns TRUE if the selected axis is in PHMODE.      */
/*  get_phaseangle_mode() returns TRUE if the selected axis is in PAMODE. */
/*  get_av_mode() returns TRUE if the selected axis is in AVMODE.         */
/*  get_mode() returns the phasefile "mode-bit" for the selected axis.    */
/*  get_direction() returns whether the "rev" or "norm" dimension is      */
/*                  in the HORIZ or VERT direction See disp.h for more    */
/*                  information                                           */
/*  get_direction_from_id() is supplied an fn_name and returns what       */
/*                  direction (HORIZ or VERT) that that dimension         */
/*                  coresponds to                                         */
/*  get_dimension() is supplied a direction (HORIZ or VERT) and returns   */
/*                  what dimension (DIM_FN0, DIM_FN1, etc) that that      */
/*                  coresponds to                                         */
/*						                          */
/**************************************************************************/

#include <math.h>
#include <string.h>
#include "disp.h"
#include "group.h"
#include "variables.h"
#include "process.h"
#include "pvars.h"
#include "wjunk.h"
#include "init_display.h"

#define FALSE	0
#define TRUE	1

#define MODE_BIT(mode, dim)	((mode) << (dim))

#ifdef  DEBUG
extern int debug1;
#define DPRINT0(str) \
	if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1) \
	if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4)
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4,arg5)
#else 
#define DPRINT0(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5)
#endif 

struct proc_dimension
{
   double    rp_val;
   double    lp_val;
   int       fn_val;
   int       mode_val;
   int       mode_shift;
   int       rev_norm;
   int       dim_id;
   char     *fn_name;
   char     *mode_name;
   char     *np_name;
   char     *lp_name;
   char     *rp_name;
};

typedef struct proc_dimension	proc_;

static proc_ *proc_x,*proc_y;

static proc_ proc_fn0 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   0,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   FN0_DIM,		/* int       dim_id         */
   "fn",		/* char     *fn_name        */
   "dmg",		/* char     *mode_name      */
   "np",		/* char     *np_name        */
   "lp",		/* char     *lp_name        */
   "rp",		/* char     *rp_name        */
   };

static proc_ proc_fn1 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   4,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   FN1_DIM,		/* int       dim_id         */
   "fn1",		/* char     *fn_name        */
   "dmg1",		/* char     *mode_name      */
   "ni",		/* char     *np_name        */
   "lp1",		/* char     *lp_name        */
   "rp1",		/* char     *rp_name        */
   };

static proc_ proc_fn2 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   8,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   FN2_DIM,		/* int       dim_id         */
   "fn2",		/* char     *fn_name        */
   "dmg2",		/* char     *mode_name      */
   "ni2",		/* char     *np_name        */
   "lp2",		/* char     *lp_name        */
   "rp2",		/* char     *rp_name        */
   };

static proc_ proc_fn3 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   8,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   FN3_DIM,		/* int       dim_id         */
   "fn3",		/* char     *fn_name        */
   "dmg3",		/* char     *mode_name      */
   "ni3",		/* char     *np_name        */
   "lp3",		/* char     *lp_name        */
   "rp3",		/* char     *rp_name        */
   };

static proc_ proc_sw0 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   0,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   SW0_DIM,		/* int       dim_id         */
   "fn",		/* char     *fn_name        */
   "dmg",		/* char     *mode_name      */
   "np",		/* char     *np_name        */
   "lp",		/* char     *lp_name        */
   "rp",		/* char     *rp_name        */
   };

static proc_ proc_sw1 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   4,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   SW1_DIM,		/* int       dim_id         */
   "fn1",		/* char     *fn_name        */
   "dmg1",		/* char     *mode_name      */
   "ni",		/* char     *np_name        */
   "lp",		/* char     *lp_name        */
   "rp",		/* char     *rp_name        */
   };

static proc_ proc_sw2 = {
   0.0,			/* double    rp_val         */
   0.0,			/* double    lp_val         */
   0,			/* int       fn_val         */
   0,			/* int       mode_val       */
   8,			/* int       mode_shift     */
   0,			/* int       rev_norm       */
   SW2_DIM,		/* int       dim_id         */
   "fn2",		/* char     *fn_name        */
   "dmg2",		/* char     *mode_name      */
   "ni2",		/* char     *np_name        */
   "lp",		/* char     *lp_name        */
   "rp",		/* char     *rp_name        */
   };

int set_mode(char *m_name)
{
   int r;
   int d_phase, d_absval, d_power, d_phaseangle,d_dbm;
   char dmgval[8];

   r = P_getstring(CURRENT, m_name, dmgval, 1, 5);
   d_phase = d_absval = d_power = d_dbm = d_phaseangle = 0;
   if (!r)
   {
      d_phase = ((dmgval[0] == 'p') && (dmgval[1] == 'h'));
      d_absval = ((dmgval[0] == 'a') && (dmgval[1] == 'v')); 
      d_power = ((dmgval[0] == 'p') && (dmgval[1] == 'w')); 
      d_phaseangle = ((dmgval[0] == 'p') && (dmgval[1] == 'a'));
      d_dbm = ((dmgval[0] == 'd') && (dmgval[1] == 'b'));
      r = (!d_phase && !d_absval && !d_power && !d_phaseangle && !d_dbm);
   }

   if (r)     /* default to "dmg" */
   {
      if (!P_getstring(CURRENT, "dmg", dmgval, 1, 4))
      {
         d_phase = ((dmgval[0] == 'p') && (dmgval[1] == 'h'));
         d_absval = ((dmgval[0] == 'a') && (dmgval[1] == 'v')); 
         d_power = ((dmgval[0] == 'p') && (dmgval[1] == 'w')); 
         d_phaseangle = ((dmgval[0] == 'p') && (dmgval[1] == 'a'));
         d_dbm = ((dmgval[0] == 'd') && (dmgval[1] == 'b'));
         r = (!d_phase && !d_absval && !d_power && !d_phaseangle && !d_dbm);
      }
   }

   if (r)
   {
      d_absval = 1;
      Werrprintf("Parameter dmg does not exist.  Default to absolute value mode");
   }
   if (d_phase)
      return(PHMODE);
   else if (d_absval)
      return(AVMODE);
   else if (d_phaseangle)
      return(PAMODE);
   else if (d_dbm) 
	  return(PAMODE|PWRMODE);
   else
      return(PWRMODE);
}

static void
CalcDispVals(proc_ *dimen, int revnorm)
{
   double value;

   P_getreal(PROCESSED,dimen->fn_name,&value,1);
   dimen->fn_val = (int) (value + 0.5);
   dimen->rev_norm = revnorm;
   P_getreal(CURRENT,dimen->rp_name,&dimen->rp_val,1);
   P_getreal(CURRENT,dimen->lp_name,&dimen->lp_val,1);
   dimen->mode_val = set_mode(dimen->mode_name);
}

static void
set_display_vals(int dimension, int revnorm)
{
   switch (dimension)
   {
     case FN0_DIM:  CalcDispVals(&proc_fn0,revnorm);
                 break;
     case FN1_DIM:  CalcDispVals(&proc_fn1,revnorm);
                 break;
     case FN2_DIM:  CalcDispVals(&proc_fn2,revnorm);
                 break;
     case FN3_DIM:  CalcDispVals(&proc_fn3,revnorm);
                 break;
     case SW0_DIM:  CalcDispVals(&proc_sw0,revnorm);
                 break;
     case SW1_DIM:  CalcDispVals(&proc_sw1,revnorm);
                 break;
     case SW2_DIM:  CalcDispVals(&proc_sw2,revnorm);
                 break;
   }
}

static void
map_display(int dimension, int direction)
{
   if (direction == HORIZ)
      switch (dimension)
      {
        case FN0_DIM:  proc_x = &proc_fn0;
                    break;
        case FN1_DIM:  proc_x = &proc_fn1;
                    break;
        case FN2_DIM:  proc_x = &proc_fn2;
                    break;
        case FN3_DIM:  proc_x = &proc_fn3;
                    break;
        case SW0_DIM:  proc_x = &proc_sw0;
                    break;
        case SW1_DIM:  proc_x = &proc_sw1;
                    break;
        case SW2_DIM:  proc_x = &proc_sw2;
                    break;
      }
   else
      switch (dimension)
      {
        case FN0_DIM:  proc_y = &proc_fn0;
                    break;
        case FN1_DIM:  proc_y = &proc_fn1;
                    break;
        case FN2_DIM:  proc_y = &proc_fn2;
                    break;
        case FN3_DIM:  proc_y = &proc_fn3;
                    break;
        case SW0_DIM:  proc_y = &proc_sw0;
                    break;
        case SW1_DIM:  proc_y = &proc_sw1;
                    break;
        case SW2_DIM:  proc_y = &proc_sw2;
                    break;
      }
}

void set_fid_proc()
{
   set_display_vals(SW0_DIM,NORMDIR);
   DPRINT0("mapping:   SW0 to HORIZ\n");
   map_display(SW0_DIM,HORIZ);
   map_display(SW0_DIM,VERT);
}

void set_spec_proc(int direction0, int proc0, int revnorm0,
                   int direction1, int proc1, int revnorm1)
{
   set_display_vals(proc0,revnorm0);
   map_display(proc0,direction0);
   set_display_vals(proc1,revnorm1);
   map_display(proc1,direction1);
}

void get_ref_pars(int direction, double *swval, double *ref, int *fnval)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   get_sw_par(direction,swval);
   get_rflrfp(direction,ref);
   *fnval = dim->fn_val;
   if (!get_axis_freq(direction))
   {
      *swval = (*fnval/2) / *swval;
   }
}

int get_av_mode(int direction)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   return(dim->mode_val == AVMODE);
}

int get_phase_mode(int direction)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   return(dim->mode_val == PHMODE);
}

int get_phaseangle_mode(int direction)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   return(dim->mode_val == PAMODE);
}

int get_dbm_mode(int direction)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   /* Winfoprintf("mode val = %x\n",dim->mode_val); phil */
   return(dim->mode_val == (PAMODE|PWRMODE));
}

int get_mode(int direction)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   return(MODE_BIT(dim->mode_val,dim->mode_shift));
}

int get_dimension(int direction)
{
   proc_ *dim;

   if (direction == HORIZ)
      dim = proc_x;
   else
      dim = proc_y;
   return(dim->dim_id);
}

int get_direction(int revnorm)
{
   return((proc_x->rev_norm == revnorm) ? HORIZ : VERT);
}

int get_direction_from_id(char *id_name)
{
   if (strcmp(proc_x->fn_name,id_name) == 0)
      return(HORIZ);
   else if (strcmp(proc_y->fn_name,id_name) == 0)
      return(VERT);
   else
      return(DIRECTION_ERROR);
}
