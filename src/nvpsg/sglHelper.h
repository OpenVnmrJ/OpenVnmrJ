/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SGLHELPERS_H
#define SGLHELPERS_H

/* mean nt and mean tr of arrayed values */
extern double ntmean;                               /* Mean nt of arrayed nt values */
extern double trmean;                               /* Mean tr of arrayed tr values */

extern int P_getVarInfo(int tree,char *name,VINFO_T *info);

extern VINFO_T       dvarinfo;           /* variable info as defined in variables.h */
extern ARRAYPARS_T   arraypars;          /* array parameters */

void   get_parameters();
void   euler_test();
void   init_mri();
void   set_images();
void   pdd_preacq();
void   pdd_postacq();
void   create_diffusion(int difftype);
void   init_diffusion(DIFFUSION_T *diffusion,GENERIC_GRADIENT_T *grad,char *name,double amp,double delta);
void   set_diffusion(DIFFUSION_T *diffusion,double Dadd,double DELTA,double techo,char mintechoflag);
void   set_diffusion_se(DIFFUSION_T *diffusion,double decho1,double decho2);
void   set_diffusion_tm(DIFFUSION_T *diffusion,double tmix,char mintmixflag);
void   calc_diffTime(DIFFUSION_T *diffusion,double *temin);
void   calc_diffTime_tm(DIFFUSION_T *diffusion,double *temin,double *tmmin);
void   calc_bvalues(DIFFUSION_T *diffusion,char *ropar,char *pepar,char *slpar);
void   set_dvalues(DIFFUSION_T *diffusion,double *roscale,double *pescale,double *slscale,int index);
void   write_bvalues(DIFFUSION_T *diffusion,char *basepar,char *tracepar,char *maxpar);
void   diffusion_dephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale);
void   diffusion_rephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale);
double bval( double g1,double d1,double D1);
double bval2( double g1,double g2,double d1,double D1);
double bval_nested( double g1,double d1,double D1,double g2,double d2,double D2);
double bval_cross( double g1,double d1,double D1,double g2,double d2,double D2);
double shapedbval(double g1,double d1,double D1,int shape1);
double shapedbval2(double g1,double g2,double d1,double D1,int shape1); 
double shapedbval_nested(double g1,double d1,double D1,int shape1,double g2,double d2,double D2,int shape2);
double shapedbval_cross(double g1,double d1,double D1,int shape1,double g2,double d2,double D2,int shape2);
void   calc_mean_nt_tr();
void   parse_array(ARRAYPARS_T *apars);
int    array_check(char *par,ARRAYPARS_T *apars);
int    get_nvals(char *par,ARRAYPARS_T *apars);
int    get_cycle(char *par,ARRAYPARS_T *apars);
void   check_nsblock();
void   init_structures();
double limitResolution( double in);
double prep_profile( char profilechar, 
                     double steps, 
                     PHASE_ENCODE_GRADIENT_T *pe,
                     PHASE_ENCODE_GRADIENT_T *per);
void   putstring( char *param, 
                  char value[]); 
void   putvalue( char *param, 
                 double value);
void   putarray(char *param, double *value, int n);
void   retrieve_parameters();
void   sgl_abort_message( char *format, ...);
void   sgl_error_check( int error_flag);

void   t_get_parameters();
void   t_euler_test();
void   t_init_mri();
void   t_set_images();
void   t_pdd_preacq();
void   t_pdd_postacq();
void   t_create_diffusion(int difftype);
void   t_init_diffusion(DIFFUSION_T *diffusion,GENERIC_GRADIENT_T *grad,char *name,double amp,double delta);
void   t_set_diffusion(DIFFUSION_T *diffusion,double Dadd,double DELTA,double techo,char mintechoflag);
void   t_set_diffusion_se(DIFFUSION_T *diffusion,double decho1,double decho2);
void   t_set_diffusion_tm(DIFFUSION_T *diffusion,double tmix,char mintmixflag);
void   t_calc_diffTime(DIFFUSION_T *diffusion,double *temin);
void   t_calc_diffTime_tm(DIFFUSION_T *diffusion,double *temin,double *tmmin);
void   t_calc_bvalues(DIFFUSION_T *diffusion,char *ropar,char *pepar,char *slpar);
void   t_set_dvalues(DIFFUSION_T *diffusion,double *roscale,double *pescale,double *slscale,int index);
void   t_write_bvalues(DIFFUSION_T *diffusion,char *basepar,char *tracepar,char *maxpar);
void   t_diffusion_dephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale);
void   t_diffusion_rephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale);
double t_bval( double g1,double d1,double D1);
double t_bval2( double g1,double g2,double d1,double D1);
double t_bval_nested( double g1,double d1,double D1,double g2,double d2,double D2);
double t_bval_cross( double g1,double d1,double D1,double g2,double d2,double D2);
double t_shapedbval(double g1,double d1,double D1,int shape1);
double t_shapedbval2(double g1,double g2,double d1,double D1,int shape1); 
double t_shapedbval_nested(double g1,double d1,double D1,int shape1,double g2,double d2,double D2,int shape2);
double t_shapedbval_cross(double g1,double d1,double D1,int shape1,double g2,double d2,double D2,int shape2);
void   t_calc_mean_nt_tr();
void   t_parse_array(ARRAYPARS_T *apars);
int    t_array_check(char *par,ARRAYPARS_T *apars);
int    t_get_nvals(char *par,ARRAYPARS_T *apars);
int    t_get_cycle(char *par,ARRAYPARS_T *apars);
void   t_check_nsblock();
void   t_init_structures();
double t_limitResolution( double in);
double t_prep_profile( char profilechar, 
                       double steps, 
                       PHASE_ENCODE_GRADIENT_T *pe,
                       PHASE_ENCODE_GRADIENT_T *per);
void   t_putstring( char *param, 
                    char value[]); 
void   t_putvalue( char *param, 
                   double value);
void   t_putarray(char *param, double *value, int n);
void   t_retrieve_parameters();
void   t_sgl_abort_message(char *format, ...);
void   t_sgl_error_check(int error_flag);

void   x_get_parameters();
void   x_euler_test();
void   x_init_mri();
void   x_set_images();
void   x_pdd_preacq();
void   x_pdd_postacq();
void   x_create_diffusion(int difftype);
void   x_init_diffusion(DIFFUSION_T *diffusion,GENERIC_GRADIENT_T *grad,char *name,double amp,double delta);
void   x_set_diffusion(DIFFUSION_T *diffusion,double Dadd,double DELTA,double techo,char mintechoflag);
void   x_set_diffusion_se(DIFFUSION_T *diffusion,double decho1,double decho2);
void   x_set_diffusion_tm(DIFFUSION_T *diffusion,double tmix,char mintmixflag);
void   x_calc_diffTime(DIFFUSION_T *diffusion,double *temin);
void   x_calc_diffTime_tm(DIFFUSION_T *diffusion,double *temin,double *tmmin);
void   x_calc_bvalues(DIFFUSION_T *diffusion,char *ropar,char *pepar,char *slpar);
void   x_set_dvalues(DIFFUSION_T *diffusion,double *roscale,double *pescale,double *slscale,int index);
void   x_write_bvalues(DIFFUSION_T *diffusion,char *basepar,char *tracepar,char *maxpar);
void   x_diffusion_dephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale);
void   x_diffusion_rephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale);
double x_bval( double g1,double d1,double D1);
double x_bval2( double g1,double g2,double d1,double D1);
double x_bval_nested( double g1,double d1,double D1,double g2,double d2,double D2);
double x_bval_cross( double g1,double d1,double D1,double g2,double d2,double D2);
double x_shapedbval(double g1,double d1,double D1,int shape1);
double x_shapedbval2(double g1,double g2,double d1,double D1,int shape1); 
double x_shapedbval_nested(double g1,double d1,double D1,int shape1,double g2,double d2,double D2,int shape2);
double x_shapedbval_cross(double g1,double d1,double D1,int shape1,double g2,double d2,double D2,int shape2);
void   x_calc_mean_nt_tr();
void   x_parse_array(ARRAYPARS_T *apars);
int    x_array_check(char *par,ARRAYPARS_T *apars);
int    x_get_nvals(char *par,ARRAYPARS_T *apars);
int    x_get_cycle(char *par,ARRAYPARS_T *apars);
void   x_check_nsblock();
void   x_init_structures();
double x_limitResolution( double in);
double x_prep_profile( char profilechar, 
                       double steps, 
                       PHASE_ENCODE_GRADIENT_T *pe,
                       PHASE_ENCODE_GRADIENT_T *per);
void   x_putstring( char *param, 
                    char value[]); 
void   x_putvalue( char *param, 
                   double value);
void   x_putarray(char *param, double *value, int n);
void   x_retrieve_parameters();
void   x_sgl_abort_message(char *format, ...);
void   x_sgl_error_check(int error_flag);



#endif
