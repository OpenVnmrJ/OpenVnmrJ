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
/*  init_display.c -	procedures for localizing information about       */
/*                      display parameters.  All display details of which */
/*                      axis is Horizontal and which is Vertical          */
/*                      is maintained here.                               */
/*  entry points include the procedures set_spec_display() and            */
/*  set_fid_display(),  reset_disp_pars(), get_sfrq(),                    */
/*  get_phase_pars(), get_sw_par(),                                       */
/*  get_scale_pars(), get_scalesw(),                                      */
/*  get_axis_freq(), get_mark2d_info()                                    */
/*  get_reference(), and get_rflrfp().                                    */
/*						                          */
/*  set_spec_display() maps selectable domains (e.g.,  FN0, or SW1) onto  */
/*  the Horizontal and Vertical axes.                                     */
/*  set_fid_display() maps the SW0 domain onto both the Horizontal and    */
/*  Vertical axes.  There are no other options.                           */
/*  reset_disp_pars() corrects parameters when using a two dimensional    */
/*  display of FIDs or interferrograms.  This is because of the current   */
/*  restriction that these displays do not allow zooming and because the  */
/*  spectral width is calculated differently depending on the display     */
/*  get_sfrq() returns a frequency or time domain scaling factor to       */
/*  convert values based on the value of axis for either the horizontal   */
/*  or vertical direction.                                                */
/*  get_axis_freq() returns true or false depending on whether the        */
/*  the requested axis (horizontal or vertical) is a frequency (TRUE) or  */
/*  time (FALSE) domain axis.                                             */
/*  get_phase_pars() return the rp and lp paramters for the requested     */
/*  axis (horizontal or vertical).  If the requested axis is a time       */
/*  domain axis,  zeros are returned.                                     */
/*  get_sw_pars() return the sw paramter for the requested axis           */
/*  (horizontal or vertical).                                             */
/*  get_display_label() returns the character id for the requested axis   */
/*  (horizontal or vertical). For example,  returns '2' for the f2 axis.  */
/*  get_mark2d_info() returns aixs information needed by mark.  It is a   */
/*  little weird.                                                         */
/*  get_scale_pars() return the sp, wp, and frequency scaling paramters   */
/*  for the requested axis (horizontal or vertical).  Also returns        */
/*  whether the scale increases to the left (normal for frequency domain) */
/*  or to the right (normal for time domain).                             */
/*  get_scalesw() return the spectral window scaling factor for the       */
/*  requested axis (horizontal or vertical).                              */
/*  get_reference() return the individual referencing parameters for the  */
/*  requested axis (horizontal or vertical).                              */
/*  get_rflrfp() return the commonly used rflrfp variable for the         */
/*  requested axis (horizontal or vertical).                              */
/*						                          */
/*  Parameter labelling procedures are also included in this file.        */
/*  The botton two lines of the graphics display is divided into six      */
/*  fields which can each display one parameter.  Applications programs   */
/*  such as ds,  dfid,  dconi,  etc  select which parameter should be     */
/*  in which field.  The include file disp.h has the necessary definitions*/
/*  There should be no need for the application program know whether the  */
/*  appropriate parameter name is,  for example,  sp1 or sp.              */
/*						                          */
/**************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "disp.h"
#include "group.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"

#define FALSE	0
#define TRUE	1

#ifdef VNMRJ
extern int getFrameID();
#endif
extern void ParameterLine(int line, int column, int scolor, char *string);
extern int getUserUnit(char *unit, char *label, double *slope, double *intercept);

#ifdef  DEBUG
extern int debug1;
#define DPRINT0(str) \
	if (debug1) fprintf(stderr,str)
#define DPRINT1(str, arg1) \
	if (debug1) fprintf(stderr,str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3,arg4,arg5)
#else 
#define DPRINT0(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5)
#endif 

struct fr_dimension
{
   double    cr_val;
   double    delta_val;
   double    sp_val;
   double    wp_val;
   double    sw_val;
   double    frq_slope;
   double    frq_intercept;
   int       axis_elem;
   int       axis_freq;
   int       axis_rev;
   int       axis_scaled;
   char      axis_val[16];
   char      axis_label[16];
   char     *axis_name;
   char     *cr_name;
   char     *delta_name;
   char      dimen_name[10];
   char     *fn_name;
   char     *fov_name;
   char     *lp_name;
   char     *lvl_name;
   char     *ref_name;
   char     *rfl_name;
   char     *rfp_name;
   char     *rp_name;
   char     *scalesw_name;
   char     *sp_name;
   char     *sw_name;
   char     *tlt_name;
   char     *wp_name;
   char     *nuc_param;
};

typedef struct fr_dimension	dim_;

static dim_ *dim_x = NULL, *dim_y = NULL;

static dim_ dim_fn0 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   0,			/* int       axis_elem      */
   TRUE,		/* int       axis_freq      */
   FALSE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "h",			/* char      axis_val[16]   */
   "Hz",		/* char      axis_label[16] */
   "axis",		/* char     *axis_name      */
   "cr",		/* char     *cr_name        */
   "delta",		/* char     *delta_name     */
   "F2",		/* char      dimen_name[10] */
   "fn",		/* char     *fn_name        */
   "lro",		/* char     *fov_name       */
   "lp",		/* char     *lp_name        */
   "lvl",		/* char     *lvl_name       */
   "reffrq",		/* char     *ref_name       */
   "rfl",		/* char     *rfl_name       */
   "rfp",		/* char     *rfp_name       */
   "rp",		/* char     *rp_name        */
   "scalesw",		/* char     *scalesw_name   */
   "sp",		/* char     *sp_name        */
   "sw",		/* char     *sw_name        */
   "tlt",		/* char     *tlt_name       */
   "wp",		/* char     *wp_name        */
   "tn"			/* char     *nuc_param      */
   };

static dim_ dim_fn1 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   1,			/* int       axis_elem      */
   TRUE,		/* int       axis_freq      */
   FALSE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "h",			/* char      axis_val[16]   */
   "Hz",		/* char      axis_label[16] */
   "axis",		/* char     *axis_name      */
   "cr1",		/* char     *cr_name        */
   "delta1",		/* char     *delta_name     */
   "F1",		/* char      dimen_name[10] */
   "fn1",		/* char     *fn_name        */
   "lpe",		/* char     *fov_name       */
   "lp1",		/* char     *lp_name        */
   "lvl",		/* char     *lvl_name       */
   "reffrq1",		/* char     *ref_name       */
   "rfl1",		/* char     *rfl_name       */
   "rfp1",		/* char     *rfp_name       */
   "rp1",		/* char     *rp_name        */
   "scalesw1",		/* char     *scalesw_name   */
   "sp1",		/* char     *sp_name        */
   "sw1",		/* char     *sw_name        */
   "tlt",		/* char     *tlt_name       */
   "wp1",		/* char     *wp_name        */
   "dn"			/* char     *nuc_param      */
   };

static dim_ dim_fn2 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   2,			/* int       axis_elem      */
   TRUE,		/* int       axis_freq      */
   FALSE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "h",			/* char      axis_val[16]   */
   "Hz",		/* char      axis_label[16] */
   "axis",		/* char     *axis_name      */
   "cr2",		/* char     *cr_name        */
   "delta2",		/* char     *delta_name     */
   "F1",		/* char      dimen_name[10] */
   "fn2",		/* char     *fn_name        */
   "lss",		/* char     *fov_name       */
   "lp2",		/* char     *lp_name        */
   "lvl",		/* char     *lvl_name       */
   "reffrq2",		/* char     *ref_name       */
   "rfl2",		/* char     *rfl_name       */
   "rfp2",		/* char     *rfp_name       */
   "rp2",		/* char     *rp_name        */
   "scalesw2",		/* char     *scalesw_name   */
   "sp2",		/* char     *sp_name        */
   "sw2",		/* char     *sw_name        */
   "tlt",		/* char     *tlt_name       */
   "wp2",		/* char     *wp_name        */
   "dn2"		/* char     *nuc_param      */
   };

static dim_ dim_fn3 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   3,			/* int       axis_elem      */
   TRUE,		/* int       axis_freq      */
   FALSE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "h",			/* char      axis_val[16]   */
   "Hz",		/* char      axis_label[16] */
   "axis",		/* char     *axis_name      */
   "cr3",		/* char     *cr_name        */
   "delta3",		/* char     *delta_name     */
   "F3",		/* char      dimen_name[10] */
   "fn3",		/* char     *fn_name        */
   "lss",		/* char     *fov_name       */
   "lp3",		/* char     *lp_name        */
   "lvl",		/* char     *lvl_name       */
   "reffrq3",		/* char     *ref_name       */
   "rfl3",		/* char     *rfl_name       */
   "rfp3",		/* char     *rfp_name       */
   "rp3",		/* char     *rp_name        */
   "scalesw3",		/* char     *scalesw_name   */
   "sp3",		/* char     *sp_name        */
   "sw3",		/* char     *sw_name        */
   "tlt",		/* char     *tlt_name       */
   "wp3",		/* char     *wp_name        */
   "dn2"		/* char     *nuc_param      */
   };

static dim_ dim_sw0 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   0,			/* int       axis_elem      */
   FALSE,		/* int       axis_freq      */
   TRUE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "s",			/* char      axis_val[16]   */
   "sec",		/* char      axis_label[16] */
   "axisf",		/* char     *axis_name      */
   "crf",		/* char     *cr_name        */
   "deltaf",		/* char     *delta_name     */
   "t2",		/* char      dimen_name[10] */
   "np",		/* char     *fn_name        */
   "",			/* char     *fov_name       */
   "",			/* char     *lp_name        */
   "",			/* char     *lvl_name       */
   "",			/* char     *ref_name       */
   "",			/* char     *rfl_name       */
   "",			/* char     *rfp_name       */
   "phfid",		/* char     *rp_name        */
   "",			/* char     *scalesw_name   */
   "sf",		/* char     *sp_name        */
   "sw",		/* char     *sw_name        */
   "",			/* char     *tlt_name       */
   "wf",		/* char     *wp_name        */
   "tn"			/* char     *nuc_param      */
   };

static dim_ dim_sw1 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   1,			/* int       axis_elem      */
   FALSE,		/* int       axis_freq      */
   TRUE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "s",			/* char      axis_val[16]   */
   "sec",		/* char      axis_label[16] */
   "axisf",		/* char     *axis_name      */
   "cr1",		/* char     *cr_name        */
   "delta1",		/* char     *delta_name     */
   "t1",		/* char      dimen_name[10] */
   "ni",		/* char     *fn_name        */
   "",			/* char     *fov_name       */
   "",			/* char     *lp_name        */
   "",			/* char     *lvl_name       */
   "",			/* char     *ref_name       */
   "",			/* char     *rfl_name       */
   "",			/* char     *rfp_name       */
   "phfid1",		/* char     *rp_name        */
   "",			/* char     *scalesw_name   */
   "sf1",		/* char     *sp_name        */
   "sw1",		/* char     *sw_name        */
   "",			/* char     *tlt_name       */
   "wf1",		/* char     *wp_name        */
   "dn"			/* char     *nuc_param      */
   };

static dim_ dim_sw2 = {
   1.0,			/* double    cr_val         */
   1.0,			/* double    delta_val      */
   1.0,			/* double    sp_val         */
   1.0,			/* double    wp_val         */
   1.0,			/* double    sw_val         */
   1.0,			/* double    frq_slope      */
   0.0,			/* double    frq_intercept  */
   2,			/* int       axis_elem      */
   FALSE,		/* int       axis_freq      */
   TRUE,		/* int       axis_rev       */
   FALSE,		/* int       axis_scaled    */
   "s",			/* char      axis_val[16]   */
   "sec",		/* char      axis_label[16] */
   "axisf",		/* char     *axis_name      */
   "cr2",		/* char     *cr_name        */
   "delta2",		/* char     *delta_name     */
   "t2",		/* char      dimen_name[10] */
   "ni2",		/* char     *fn_name        */
   "",			/* char     *fov_name       */
   "",			/* char     *lp_name        */
   "",			/* char     *lvl_name       */
   "",			/* char     *ref_name       */
   "",			/* char     *rfl_name       */
   "",			/* char     *rfp_name       */
   "phfid2",		/* char     *rp_name        */
   "",			/* char     *scalesw_name   */
   "sf2",		/* char     *sp_name        */
   "sw2",		/* char     *sw_name        */
   "",			/* char     *tlt_name       */
   "wf2",		/* char     *wp_name        */
   "dn2"		/* char     *nuc_param      */
   };

static void
set_display_name(int dimension, char *dim_name)
{
   switch (dimension)
   {
     case FN0_DIM:
                 DPRINT1("FN0_DIM name is %s\n",dim_name);
                 strcpy(dim_fn0.dimen_name, dim_name);
                 break;
     case FN1_DIM:
                 DPRINT1("FN1_DIM name is %s\n",dim_name);
                 strcpy(dim_fn1.dimen_name, dim_name);
                 break;
     case FN2_DIM:
                 DPRINT1("FN2_DIM name is %s\n",dim_name);
                 strcpy(dim_fn2.dimen_name, dim_name);
                 break;
     case FN3_DIM:
                 DPRINT1("FN3_DIM name is %s\n",dim_name);
                 strcpy(dim_fn3.dimen_name, dim_name);
                 break;
     case SW0_DIM:
                 DPRINT1("SW0_DIM name is %s\n",dim_name);
                 strcpy(dim_sw0.dimen_name, dim_name);
                 break;
     case SW1_DIM:
                 DPRINT1("SW1_DIM name is %s\n",dim_name);
                 strcpy(dim_sw1.dimen_name, dim_name);
                 break;
     case SW2_DIM:
                 DPRINT1("SW2_DIM name is %s\n",dim_name);
                 strcpy(dim_sw2.dimen_name, dim_name);
                 break;
   }
}

/******************************/
void getunits(dim_ *dimen, char *units, int ltype)
/******************************/
{
  /*   label type 1 :      ppm (sc)  :  used for dscale and pscale  */
  /*   label type 2 :     (ppm)(sc)  :  used for vertical 2d axis   */
  /*   label type 3 :     (ppm (sc)) :  used for horizontal 2d axis */
  /*   label type 4 :     (ppm)      :  used for parameter label fields */
   if (ltype == UNIT1)
      *units = '\0';
   else
      strcpy(units,"(");
   strcat(units,dimen->axis_label);
   if (ltype == UNIT4)
   {
      strcat(units,")");
   }
   else
   {
      if (ltype == UNIT2)
         strcat(units,")");
      if (dimen->axis_scaled)
         strcat(units," (sc)");
      if (ltype == UNIT3)
         strcat(units,")");
   }
}

/*****************************************/
static int
sw_scaled(dim_ *dimen, double *value)
/*****************************************/
{
   int scaled = FALSE;
   double value2;

   *value = 1.0;
   if (dimen->axis_freq)  /*  only spectra can have scaled axes */
   {
      if (!P_getreal(CURRENT,dimen->scalesw_name,&value2,1))
      {
         vInfo info;

         if ((P_getVarInfo(CURRENT,dimen->scalesw_name,&info) == 0) &&
              info.active)
            if ((value2 > 0.0) && (value2 != 1.0))
            {
               scaled = TRUE;
	       *value *= value2;
            }
      }
      if (strcmp(dimen->sw_name,"sw") == 0) /* directly acquired dimension */
      {
        if (!P_getreal(CURRENT,"downsamp",&value2,1))
        {
          vInfo info;

          if ((P_getVarInfo(CURRENT,"downsamp",&info) == 0) &&
              info.active)
            if ((value2 > 0.0) && (value2 != 1.0))
            {
               scaled = TRUE;
               *value /= value2;
            }
        }
      }
   }
   if (!scaled)
      *value = 1.0;
   return(scaled);
}

/*****************************************/
static void
set_frq(dim_ *dimen)
/*****************************************/
{
  double value;
  double freq = 1.0;
  double inter = 0.0;

  if(!getUserUnit(dimen->axis_val, dimen->axis_label, &freq, &inter))
  {
    switch (dimen->axis_val[0])
    {
     /*  seconds or hertz  */
     case 'x':
                strcpy(dimen->axis_label,"I");
                break;
     case ' ':
                strcpy(dimen->axis_label,"");
                break;
     case 's':
                strcpy(dimen->axis_label,"sec");
                break;
     case 'h':
                strcpy(dimen->axis_label,"Hz");
                break;
     case 'D':
                strcpy(dimen->axis_label,"D");
                break;

     /*  kilohertz axis */
     case 'k':  freq = 1000.0;
                strcpy(dimen->axis_label,"kHz");
                break;

     /*  cm axis */
     case 'c':  if (P_getreal(PROCESSED,dimen->fov_name,&value,1))
                {
                   strcpy(dimen->axis_val,"h");
                   strcpy(dimen->axis_label,"Hz");
/*
                   Werrprintf("%s not found; axis assumed to be 'h'",
                               dimen->fov_name);
 */
                }
                else
                {
                   freq = (value > 0.0) ? dimen->sw_val/value : 1.0;
                   strcpy(dimen->axis_label,"cm");
                }
		break;

     /*  decoupler freq ppm */
     case '1':
     case 'd':
                if (!P_getreal(PROCESSED,"dreffrq",&freq,1) && (freq > 1.0))
                   strcpy(dimen->axis_label,"ppm");
                else if (P_getreal(PROCESSED,"dfrq",&freq,1) || (freq < 1.0))
                {
                   freq = 1.0;
                   strcpy(dimen->axis_val,"h");
                   strcpy(dimen->axis_label,"Hz");
                }
                else
                {
                   strcpy(dimen->axis_label,"ppm");
                }
		break;

     /* 2nd decoupler freq ppm */
     case '2':
		if (!P_getreal(PROCESSED,"dreffrq2",&freq,1) && (freq > 1.0))
                   strcpy(dimen->axis_label,"ppm");
                else if (P_getreal(PROCESSED,"dfrq2",&freq,1) || (freq < 1.0))
		{
		   freq = 1.0;
                   strcpy(dimen->axis_val,"h");
                   strcpy(dimen->axis_label,"Hz");
		}
                else
                {
                   strcpy(dimen->axis_label,"ppm");
                }
		break;

     /* 3rd decoupler freq ppm */
     case '3':
		if (!P_getreal(PROCESSED,"dreffrq3",&freq,1) && (freq > 1.0))
                   strcpy(dimen->axis_label,"ppm");
                else if (P_getreal(PROCESSED,"dfrq3",&freq,1) || (freq < 1.0))
		{
		   freq = 1.0;
                   strcpy(dimen->axis_val,"h");
                   strcpy(dimen->axis_label,"Hz");
		}
                else
                {
                   strcpy(dimen->axis_label,"ppm");
                }
		break;

     /*  mm axis or msec axis */
     case 'm':  if (dimen->axis_freq)
                {
                   if (P_getreal(PROCESSED,dimen->fov_name,&value,1))
                   {
                      strcpy(dimen->axis_val,"h");
                      strcpy(dimen->axis_label,"Hz");
/*
                      Werrprintf("%s not found; axis assumed to be 'h'",
                                  dimen->fov_name);
 */
                   }
                   else
                   {
                      freq = (value > 0.0) ? dimen->sw_val/value : 1.0;
                      freq /= 10.0;
                      strcpy(dimen->axis_label,"mm");
                   }
                }
                else                       /*  msec axis */
                {
                   freq = 1e-3;
                   strcpy(dimen->axis_label,"msec");
                }
		break;

     /*  um axis or usec axis */
     case 'u':  if (dimen->axis_freq)
                {
                   if (P_getreal(PROCESSED,dimen->fov_name,&value,1))
                   {
                      strcpy(dimen->axis_val,"h");
                      strcpy(dimen->axis_label,"Hz");
/*
                      Werrprintf("%s not found; axis assumed to be 'h'",
                                  dimen->fov_name);
 */
                   }
                   else
                   {
                      freq = (value > 0.0) ? dimen->sw_val/value : 1.0;
                      freq /= 1e4;
                      strcpy(dimen->axis_label,"um");
                   }
                }
                else                       /*  usec axis */
                {
                   freq = 1e-6;
                   strcpy(dimen->axis_label,"usec");
                }
		break;
     case 'S':  {
                   char tmp[64];
                   char label[16];
                   double left;
                   double right;

                   sprintf(label,"%s_left",dimen->fn_name);
                   if (P_getreal(CURRENT,label,&left,1))
                      left=0.0;
                   sprintf(label,"%s_right",dimen->fn_name);
                   if (P_getreal(CURRENT,label,&right,1))
                      right=1.0;
                   sprintf(label,"%s_label",dimen->fn_name);
                   strcpy(tmp,"");
                   P_getstring(CURRENT,label,tmp,1,15);
                   strcpy(dimen->axis_label,tmp);
                   if (left > right)
                   {
                      dimen->sw_val = left - right;
                      dimen->axis_rev = 0;
                      dimen->axis_freq = 1;
                   }
                   else
                   {
                      dimen->sw_val = right - left;
                      dimen->axis_rev = 1;
		      inter = dimen->sw_val;
		      dimen->axis_freq = 1;
                   }
                }
		break;
     case '0':  break;   /* Assume axis_label is already set */

     default:   if (dimen->axis_freq)
                {
		   if (P_getreal(PROCESSED,"sreffrq",&freq,1) || (freq < 1.0))
                      P_getreal(PROCESSED,"sfrq",&freq,1);     /*  ppm */
                   if ((dimen->axis_val[0]!='p') && (dimen->axis_val[0]!='n'))
                   {
                      strcpy(dimen->axis_val,"p");
                      strcpy(dimen->axis_label,"ppm");
                   }
                }
                else
                {
                   strcpy(dimen->axis_val,"s");
                   strcpy(dimen->axis_label,"sec");
                }
		break;
    }
  }
  dimen->axis_scaled = FALSE;
  if (sw_scaled(dimen,&value))
  {
     freq /= value;
     dimen->axis_scaled = TRUE;
  }
  dimen->frq_slope = freq;
  dimen->frq_intercept = inter;
}

static void
CalcDispVals(dim_ *dimen)
{
   char axis[10];

   P_getreal(CURRENT,dimen->cr_name,&dimen->cr_val,1);
   P_getreal(CURRENT,dimen->delta_name,&dimen->delta_val,1);
   P_getreal(CURRENT,dimen->sp_name,&dimen->sp_val,1);
   P_getreal(CURRENT,dimen->wp_name,&dimen->wp_val,1);
   P_getreal(PROCESSED,dimen->sw_name,&dimen->sw_val,1);
   DPRINT2("%s = %g\n",dimen->sw_name,dimen->sw_val);
   P_getstring(CURRENT,dimen->axis_name,axis,1,8);
   DPRINT2("%s = %s\n",dimen->axis_name,axis);
   dimen->axis_val[0] = axis[dimen->axis_elem];
   dimen->axis_val[1] = '\0';
   dimen->axis_rev = !dimen->axis_freq;
   DPRINT3("axis elem %d = %s axis freq (vs time) = %d\n",
            dimen->axis_elem,dimen->axis_val,dimen->axis_freq);
   set_frq(dimen);
   DPRINT1("freq scaling = %g\n",dimen->frq_slope);
}

static void
set_display_vals(int dimension)
{
   switch (dimension)
   {
     case FN0_DIM:  CalcDispVals(&dim_fn0);
                 break;
     case FN1_DIM:  CalcDispVals(&dim_fn1);
                 break;
     case FN2_DIM:  CalcDispVals(&dim_fn2);
                 break;
     case FN3_DIM:  CalcDispVals(&dim_fn3);
                 break;
     case SW0_DIM:  CalcDispVals(&dim_sw0);
                 break;
     case SW1_DIM:  CalcDispVals(&dim_sw1);
                 break;
     case SW2_DIM:  CalcDispVals(&dim_sw2);
                 break;
   }
}

static void
map_display(int dimension, int direction)
{
   if (direction == HORIZ)
      switch (dimension)
      {
        case FN0_DIM:  dim_x = &dim_fn0;
                    break;
        case FN1_DIM:  dim_x = &dim_fn1;
                    break;
        case FN2_DIM:  dim_x = &dim_fn2;
                    break;
        case FN3_DIM:  dim_x = &dim_fn3;
                    break;
        case SW0_DIM:  dim_x = &dim_sw0;
                    break;
        case SW1_DIM:  dim_x = &dim_sw1;
                    break;
        case SW2_DIM:  dim_x = &dim_sw2;
                    break;
      }
   else
      switch (dimension)
      {
        case FN0_DIM:  dim_y = &dim_fn0;
                    break;
        case FN1_DIM:  dim_y = &dim_fn1;
                    break;
        case FN2_DIM:  dim_y = &dim_fn2;
                    break;
        case FN3_DIM:  dim_y = &dim_fn3;
                    break;
        case SW0_DIM:  dim_y = &dim_sw0;
                    break;
        case SW1_DIM:  dim_y = &dim_sw1;
                    break;
        case SW2_DIM:  dim_y = &dim_sw2;
                    break;
      }
}

void set_fid_display()
{
   set_display_vals(SW0_DIM);
   DPRINT0("mapping:   SW0 to HORIZ\n");
   map_display(SW0_DIM,HORIZ);
   map_display(SW0_DIM,VERT);
   set_display_name(SW0_DIM,"t2");
}

void reset_disp_pars(int direction, double swval, double wpval, double spval)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->sw_val = swval;
   dim->sp_val = spval;
   dim->wp_val = wpval;
}

void set_spec_display(int direction0, int dim0, int direction1, int dim1)
{
   set_display_vals(dim0);
   map_display(dim0,direction0);
   set_display_vals(dim1);
   map_display(dim1,direction1);
   set_display_name(FN0_DIM,"F2");
   set_display_name(FN1_DIM,"F1");
   set_display_name(FN2_DIM,"F2");
   set_display_name(SW0_DIM,"t2");
   set_display_name(SW1_DIM,"t1");
   set_display_name(SW2_DIM,"t1");
}

void set_display_label(int direction0, char char0, int direction1, char char1)
{
   if (direction0 == HORIZ)
      dim_x->dimen_name[1] = char0;
   else
      dim_y->dimen_name[1] = char0;
   if (direction1 == HORIZ)
      dim_x->dimen_name[1] = char1;
   else
      dim_y->dimen_name[1] = char1;
}

void get_nuc_name(int direction, char *nucname, int n) {
   if (dim_x == NULL)
      map_display(SW0_DIM, HORIZ);
   if (dim_y == NULL)
      map_display(SW0_DIM, VERT);
   if (direction == HORIZ) {
      if(dim_x->axis_val[0] == 'd') P_getstring(CURRENT,"dn",nucname,1,n);
      else if(dim_x->axis_val[0] == '2') P_getstring(CURRENT,"dn2",nucname,1,n);
      else if(dim_x->axis_val[0] == '3') P_getstring(CURRENT,"dn3",nucname,1,n);
      else P_getstring(CURRENT,"tn",nucname,1,n);

   } else {
      if(dim_y->axis_val[0] == 'd') P_getstring(CURRENT,"dn",nucname,1,n);
      else if(dim_y->axis_val[0] == '2') P_getstring(CURRENT,"dn2",nucname,1,n);
      else if(dim_y->axis_val[0] == '3') P_getstring(CURRENT,"dn3",nucname,1,n);
      else P_getstring(CURRENT,"tn",nucname,1,n);
   }
}

void get_display_label(int direction, char *charval)
{
   if (direction == HORIZ)
      *charval = dim_x->dimen_name[1];
   else
      *charval = dim_y->dimen_name[1];
}

void get_sfrq(int direction, double *val)
{
   if (direction == HORIZ)
      *val = dim_x->frq_slope;
   else
      *val = dim_y->frq_slope;
}

void get_sw_par(int direction, double *val)
{
   if (direction == HORIZ)
      *val = dim_x->sw_val;
   else
      *val = dim_y->sw_val;
}

void get_reference(int direction, double *rfl_val, double *rfp_val)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   if (dim->axis_val[0] == 'S')
   {
      char label[16];

      sprintf(label,"%s_right",dim->fn_name);
      if (P_getreal(CURRENT,label,rfp_val,1))
         *rfp_val = 0.0;
      *rfl_val = 0.0;
   }
   else if (dim->axis_freq)
   {
      if (P_getreal(CURRENT,dim->rfl_name,rfl_val,1))
         *rfl_val = 0.0;
      if (P_getreal(CURRENT,dim->rfp_name,rfp_val,1))
         *rfp_val = 0.0;
   }
   else
   {
      *rfl_val = 0.0;
      *rfp_val = 0.0;
   }
}

void get_rflrfp(int direction, double *val)
{
   double rfl_val,rfp_val;

   get_reference(direction,&rfl_val,&rfp_val);
   *val = rfl_val - rfp_val;
}

int get_axis_freq(int direction)
{
   return((direction == HORIZ) ? dim_x->axis_freq : dim_y->axis_freq);
}

int get_axis_rev(int direction)
{
   return((direction == HORIZ) ? dim_x->axis_rev : dim_y->axis_rev);
}

void get_mark2d_info(int *first_ch, int *last_ch, int *first_direction)
{
   *first_ch = '2';
   *last_ch = '1';
   if (dim_x == &dim_fn0)
      *first_direction = HORIZ;
   else
      *first_direction = VERT;
}

/********************************************/
/*                                          */
/*   Parameter Labelling                    */
/*                                          */
/********************************************/
struct par_label
{
   int par_dec;
   int par_scl;
   int par_color;
   int name_color;
   int name_index;
   dim_ *ptr;
   int line1_color,line2_color;
   char line1_str[16],line2_str[16];
};

struct par_label fields[10];

static void
get_name(dim_ *dimen, int n_index, char *n_label)
{
   switch (n_index)
   {
      case CR_NAME :  strcpy(n_label,dimen->cr_name);
                      break;
      case DELTA_NAME :  strcpy(n_label,dimen->delta_name);
                      break;
      case LP_NAME :  strcpy(n_label,dimen->lp_name);
                      break;
      case RP_NAME :  strcpy(n_label,dimen->rp_name);
                      break;
      case SP_NAME :  strcpy(n_label,dimen->sp_name);
                      break;
      case WP_NAME :  strcpy(n_label,dimen->wp_name);
                      break;
   }
}

static int
get_pos(dim_ *dimen, int n_index)
{
   int pos,found;

   DPRINT0("Calling get_pos\n");
   pos = 0;
   found = FALSE;
   while (!found && (++pos <= 6))
      found = ((fields[pos].name_index == n_index) &&
               (dimen == fields[pos].ptr));
   DPRINT2("found= %d pos= %d\n",found,pos);
   return((found) ? pos : 0);
}

extern int d2flag;
void DispCursor(int pos, int color, char *label, char *axisName) {
   double d;
   char tmp[15];

   if(P_getreal(GLOBAL, "mfShowFields", &d, 1)) d = 1.0; 
#ifdef VNMRJ
   if(d < 1.0 || getFrameID() > 1) return;
#endif

   if(strstr(label,"cr")!=NULL) {
     if (d2flag)
	sprintf(tmp,"%-s%-s",axisName,"_cursor"); 
     else
	sprintf(tmp,"%-11.11s","cursor"); 
   } else if(strstr(label,"delta")!=NULL) {
     if (d2flag)
	sprintf(tmp,"%-s%-s",axisName,"_delta"); 
     else
	sprintf(tmp,"%-11.11s","delta"); 
   }

   if ((color != fields[pos].line1_color) ||
        strcmp(tmp,fields[pos].line1_str))
   {
      fields[pos].line1_color = color;
      strcpy(fields[pos].line1_str,tmp);
      ParameterLine(1,COLUMN(pos),color,tmp);
   }
}

void
DispField1(int pos, int color, char *label)
{
   double d;
   char tmp[15];

   if(P_getreal(GLOBAL, "mfShowFields", &d, 1)) d = 1.0; 
#ifdef VNMRJ
   if(d < 1.0 || getFrameID() > 1) return;
#endif

   //sprintf(tmp,"%-11.11s",label);
   if(strstr(label,"cr1")!=NULL) sprintf(tmp,"%-11.11s","F1_cursor"); 
   else if(strstr(label,"delta1")!=NULL) sprintf(tmp,"%-11.11s","F1_delta"); 
   else if(strstr(label,"vs2d")!=NULL) sprintf(tmp,"%-11.11s","2D scale"); 
   else if(strstr(label,"vsproj")!=NULL) sprintf(tmp,"%-11.11s","trace scale"); 
   else if(strstr(label,"vpf")!=NULL) sprintf(tmp,"%-11.11s","V_position"); 
   else if(strstr(label,"cr")!=NULL) {
     if (d2flag)
	sprintf(tmp,"%-11.11s","F2_cursor"); 
     else
	sprintf(tmp,"%-11.11s","cursor"); 
   } else if(strstr(label,"delta")!=NULL) {
     if (d2flag)
	sprintf(tmp,"%-11.11s","F2_delta"); 
     else
	sprintf(tmp,"%-11.11s","delta"); 
   }
   else if(strstr(label,"vs")!=NULL) sprintf(tmp,"%-11.11s","V_scale"); 
   else if(strstr(label,"vp")!=NULL) sprintf(tmp,"%-11.11s","V_position"); 
   else if(strstr(label,"th")!=NULL) sprintf(tmp,"%-11.11s","threshold"); 
   else if(strstr(label,"vf")!=NULL) sprintf(tmp,"%-11.11s","V_scale"); 
   else if(strstr(label,"io")!=NULL) sprintf(tmp,"%-11.11s","integ offset"); 
   else if(strstr(label,"is")!=NULL) sprintf(tmp,"%-11.11s","integ scale"); 
   else if(strstr(label,"sc")!=NULL) sprintf(tmp,"%-11.11s","start"); 
   else if(strstr(label,"wc")!=NULL) sprintf(tmp,"%-11.11s","width"); 
   else if(strstr(label,"lvl")!=NULL) sprintf(tmp,"%-11.11s","level"); 
   else if(strstr(label,"tlt")!=NULL) sprintf(tmp,"%-11.11s","tilt"); 
   else if(strstr(label,"phfid")!=NULL) sprintf(tmp,"%-11.11s","rotation"); 
   else if(strstr(label,"rp")!=NULL) sprintf(tmp,"%-11.11s","0 order"); 
   else if(strstr(label,"lp")!=NULL) sprintf(tmp,"%-11.11s","1st order"); 
   else sprintf(tmp,"%-11.11s",label);

   if ((color != fields[pos].line1_color) ||
        strcmp(tmp,fields[pos].line1_str))
   {
      fields[pos].line1_color = color;
      strcpy(fields[pos].line1_str,tmp);
      ParameterLine(1,COLUMN(pos),color,tmp);
   }
}

void
DispField2(int pos, int color, double val, int dez)
{
   double tmp;
   char  ln[16],format[8];

   if(P_getreal(GLOBAL, "mfShowFields", &tmp, 1)) tmp = 1.0; 
#ifdef VNMRJ
   if(tmp < 1.0 || getFrameID() > 1) return;
#endif

   if(color < 0) color = -color;
   DPRINT1("pos= %d\n",pos);
   tmp = fabs(val);
   if ((tmp < 0.1) || (tmp > 1e6))
   {
      sprintf(format,"%%-8.%dg",dez);
   }
   else
      sprintf(format,"%%-8.%df",dez);
   DPRINT2("format= %s value= %g\n",format,val);
   sprintf(ln,format,val);
   if ((color != fields[pos].line2_color) ||
        strcmp(ln,fields[pos].line2_str))
   {
      fields[pos].line2_color = color;
      strcpy(fields[pos].line2_str,ln);
      ParameterLine(2,COLUMN(pos),color,ln);
   }
}

static void
DisplayVal(dim_ *dimen, int n_index, double val, double intercept)
{
   int pos;

   if ( (pos = get_pos(dimen,n_index)) )
   {
      if (fields[pos].par_scl)
      {
/* The unit conversion is y = mx + b
 * For ppm y = 1P means y = reffrq * 1 + 0
 * For this display, y is being passed, and we want to
 * display x, therefore, x = (y - b)/m
 */
         val -= intercept;
         val /= dimen->frq_slope;
      }
      DPRINT2("scaling= %d factor= %g\n",fields[pos].par_scl,
               dimen->frq_slope);
      DispField2(pos,fields[pos].par_color,val,fields[pos].par_dec);
   }
}

void ResetLabels()
{
   fields[1].name_index = fields[1].line1_color = fields[1].line2_color = 0;
   fields[2].name_index = fields[2].line1_color = fields[2].line2_color = 0;
   fields[3].name_index = fields[3].line1_color = fields[3].line2_color = 0;
   fields[4].name_index = fields[4].line1_color = fields[4].line2_color = 0;
   fields[5].name_index = fields[5].line1_color = fields[5].line2_color = 0;
   fields[6].name_index = fields[6].line1_color = fields[6].line2_color = 0;
}

void EraseLabels()
{
   ResetLabels();
   ParameterLine(2,0,0,"");
   ParameterLine(1,0,0,"");
}

void InitVal(int pos, int direction, int n_index, int n_color, int n_style,
             int p_index, int p_color, int p_scale_it, int p_decimal)
{
   char *blanks = "           ";

   DPRINT1("calling InitVal  pos= %d\n",pos);
   fields[pos].name_index = n_index;
   fields[pos].name_color = n_color;
   if (n_index == BLANK_NAME)
   {
      DispField1(pos,n_color,blanks);
      ParameterLine(2,COLUMN(pos),n_color,blanks);
      fields[pos].line2_color = n_color;
      strcpy(fields[pos].line2_str,blanks);
   }
   else
   {
      char t_label[15];
      double val;

      fields[pos].par_color = p_color;
      fields[pos].par_dec = p_decimal;
      fields[pos].par_scl = p_scale_it;
      fields[pos].ptr = (direction == HORIZ) ? dim_x : dim_y;
      get_name(fields[pos].ptr,n_index,t_label);
      if (n_style != NOUNIT)
      {
         char u_label[15];
         char tmp[40];

         getunits(fields[pos].ptr,u_label,n_style);
         sprintf(tmp,(n_style == UNIT4) ? "%s%s" : "%s %s",t_label,u_label);
	 if(strstr(t_label,"cr") != NULL || strstr(t_label,"delta") != NULL) 
	   DispCursor(pos,n_color,t_label, fields[pos].ptr->dimen_name);
	 else
           DispField1(pos,n_color,tmp);
      }
      else if(strstr(t_label,"cr") != NULL || strstr(t_label,"delta") != NULL) {
         DispCursor(pos,n_color,t_label, fields[pos].ptr->dimen_name);
      } else {
         DispField1(pos,n_color,t_label);
      }
      if (p_index == BLANK_NAME)
      {
         ParameterLine(2,COLUMN(pos),n_color,blanks);
         fields[pos].line2_color = n_color;
         strcpy(fields[pos].line2_str,blanks);
      }
      else if (p_index != NO_NAME)
      {
         double intercept = 0.0;
         if (p_index == CR_NAME)
         {
            val = fields[pos].ptr->cr_val;
            if ( ! fields[pos].ptr->axis_rev )
               intercept = fields[pos].ptr->frq_intercept;
         }
         else if (p_index == DELTA_NAME)
            val = fields[pos].ptr->delta_val;
         else if (p_index == SP_NAME)
         {
            val = fields[pos].ptr->sp_val;
            if ( ! fields[pos].ptr->axis_rev )
               intercept = fields[pos].ptr->frq_intercept;
         }
         else if (p_index == WP_NAME)
            val = fields[pos].ptr->wp_val;
         else if (p_index == SW_NAME)
            val = fields[pos].ptr->sw_val;
         else
            P_getreal(CURRENT,t_label,&val,1);
         DisplayVal(fields[pos].ptr,p_index,val,intercept);
      }
   }
}

void UpdateVal(int direction, int n_index, double val, int displ)
{
   char n_label[15];
   dim_ *dim;
   double intercept = 0.0;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   DPRINT2("calling UpdateVal  index= %d  display= %d\n",n_index,displ);
   get_name(dim,n_index,n_label);
   P_setreal(CURRENT,n_label,val,1);
   if (n_index == CR_NAME)
   {
      dim->cr_val = val;
      if ( ! dim->axis_rev )
         intercept = dim->frq_intercept;
   }
   else if (n_index == DELTA_NAME)
      dim->delta_val = val;
   else if (n_index == SP_NAME)
   {
      dim->sp_val = val;
      if ( ! dim->axis_rev )
         intercept = dim->frq_intercept;
   }
   else if (n_index == WP_NAME)
      dim->wp_val = val;
   if (displ == SHOW)
      DisplayVal(dim,n_index,val,intercept);
}

void DispField(int pos, int color, char *n_name, double val, int decimal)
{
   DispField1(pos,color,n_name);
   DispField2(pos,color,val,decimal);
}

void get_label(int direction, int style, char *label)
{
   dim_ *dim;
   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   if (style != NOUNIT)
   {
      if ( dim->axis_val[0] == 'S')
         strcpy(label,dim->axis_label);
      else
      {
         getunits(dim,label,style);
         if (style != UNIT1)
         {
            char tmp[15];

            strcpy(tmp,label);
            sprintf(label,(style == UNIT4) ? "%s%s" : "%s %s",dim->dimen_name,tmp);
         }
      }
   }
}

void set_scale_label(int direction, char *label)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   strcpy(dim->dimen_name,label);
}

void set_scale_axis(int direction, char axisv)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->axis_val[0] = axisv;
   dim->axis_val[1] = '\0';
   set_frq(dim);
}

void set_scale_rev(int direction, int rev)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->axis_rev = rev;  
}

void set_scale_start(int direction, double start)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->sp_val = start;
}

void get_axis_label(int direction, char *label)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   strcpy(label,dim->axis_label);
}

void set_axis_label(int direction, char *label)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   strcpy(dim->axis_label,label);
   dim->axis_val[0] = '0';
   dim->axis_val[1] = '\0';
}

void set_scale_len(int direction, double len)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->wp_val = len;
   dim->frq_slope = 1.0;
}

void get_scale_axis(int direction, char *axisv)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   *axisv = dim->axis_val[0];
}

void set_cursor_pars(int direction, double crval, double deltaval)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->cr_val = crval;
   dim->delta_val = deltaval;
}

void get_cursor_pars(int direction, double *crval, double *deltaval)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   *crval = dim->cr_val;
   *deltaval = dim->delta_val;
}

void set_scale_pars(int direction, double start, double len, double scl, int rev)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   dim->sp_val = start;
   dim->wp_val = len;
   dim->frq_slope = scl;
   dim->axis_rev = rev;  
}

void get_scale_pars(int direction, double *start, double *len, double *scl, int *rev)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   *start = dim->sp_val;
   *len   = dim->wp_val;
   *scl   = dim->frq_slope;
   *rev   = dim->axis_rev;  
}

void get_intercept(int direction, double *intercept)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   *intercept = dim->frq_intercept;
}

void get_scalesw(int direction, double *scaleswval)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   sw_scaled(dim,scaleswval);
}

void get_phase_pars(int direction, double *rpval, double *lpval)
{
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   if (dim->axis_freq)
   {
      P_getreal(CURRENT,dim->rp_name,rpval,1);
      P_getreal(CURRENT,dim->lp_name,lpval,1);
   }
   else
   {
      *rpval = 0.0;
      *lpval = 0.0;
   }
}

double convert2Hz(int direction, double val)
{
   dim_ *dim;
   double freq;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;

   if(dim == NULL) return 0.0;

/* The unit conversion is y = mx + b
 * For ppm y = 1P means y = reffrq * 1 + 0
 * For this display, y is being passed, and we want to
 * display x, therefore, x = (y - b)/m
 */
     freq = val * dim->frq_slope + dim->frq_intercept;
     return freq;
}

// val is in Hz or ppm depending on axis display
// frq_slope is 1.0 when Hz is displayed, and is reffrq if ppm is displayed
double convert2ppm(int direction, double val)
{
   dim_ *dim;
   double ref;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;

   if(dim == NULL) return val;

   if (P_getreal(CURRENT,dim->ref_name,&ref,1))
         ref = 1.0;

   if(ref > 0 && dim->frq_slope >0) return val*dim->frq_slope/ref;
   else return val;
}

// convert from ppm. val is ppm, return depending on axis display
double convert4ppm(int direction, double val)
{
   dim_ *dim;
   double ref;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;

   if(dim == NULL) return val;

   if (P_getreal(CURRENT,dim->ref_name,&ref,1))
         ref = 1.0;

   if(ref > 0 && dim->frq_slope > 0) return val*ref/dim->frq_slope;
   else return val;
}
// val is in ppm
double ppm2Hz(int direction, double val)
{
   dim_ *dim;
   double ref;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;

   if(dim == NULL || !dim->axis_freq) return val;

   if (P_getreal(CURRENT,dim->ref_name,&ref,1))
         ref = 1.0;

   if(ref > 0) return val*ref;
   else return val; 
}

// val is in Hz 
double Hz2ppm(int direction, double val)
{
   dim_ *dim;
   double ref;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;

   if(dim == NULL || !dim->axis_freq) return val;

   if (P_getreal(CURRENT,dim->ref_name,&ref,1))
         ref = 1.0;

   if(ref > 0) return val/ref;
   else return val; 
}

void getReffrq(int direction, double *val) {
   dim_ *dim;

   if (direction == HORIZ)
      dim = dim_x;
   else
      dim = dim_y;
   if (dim->axis_freq)
   {
      if (dim->axis_val[0] == 'S')
      {
         *val = 1.0;
      }
      else
      {
         if (P_getreal(CURRENT,dim->ref_name,val,1))
            *val = 1.0;
      }
   }
   else
   {
      *val = 1.0;
   }
}
