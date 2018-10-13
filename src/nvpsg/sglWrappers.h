/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.1  2006/08/23 14:09:57  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:02  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/07/07 01:12:48  mikem
*     modification to compile with psg
*
*********************************************************************/

#ifndef SGLWRAPPERS_H
#define SGLWRAPPERS_H

extern int      sglarray;
extern int      sglpower;
extern double   glim;                              /* gradient limiting factor: % in VJ, fraction in SGL  */
extern double   glimpe;                            /* PE gradient limiting factor [%] */


void   calc_generic( GENERIC_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]);
void   calc_phase( PHASE_ENCODE_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]);
void   calc_readout( READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_sw[], char VJparam_at[]);
void   calc_readout_refocus( REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   calc_readout_rephase( REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);					
void   calc_rf ( RF_PULSE_T *rf, char VJparam_tpwr[], char VJparam_tpwrf[]);
double calc_sim_gradient( GENERIC_GRADIENT_T  *grad0, GENERIC_GRADIENT_T  *grad1,
                          GENERIC_GRADIENT_T  *grad2, double min_tpe, int write_flag);
void   calc_slice( SLICE_SELECT_GRADIENT_T *grad, RF_PULSE_T *rf, int write_flag, char VJparam_g[]);			
void   calc_slice_dephase( REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   calc_slice_refocus( REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);	

double calc_zfill_gradient(FLOWCOMP_T *grad0, GENERIC_GRADIENT_T  *grad1,GENERIC_GRADIENT_T *grad2);
double calc_zfill_gradient2(FLOWCOMP_T *grad0, FLOWCOMP_T *grad1,GENERIC_GRADIENT_T *grad2);
double calc_zfill_gradient3(FLOWCOMP_T *grad0, FLOWCOMP_T *grad1,FLOWCOMP_T *grad2);
double calc_zfill_grad(GENERIC_GRADIENT_T *grad0, GENERIC_GRADIENT_T  *grad1,GENERIC_GRADIENT_T *grad2);				

void   init_generic( GENERIC_GRADIENT_T *grad, char name[], double amp, double time); 																			
void   init_phase( PHASE_ENCODE_GRADIENT_T *grad, char name[], double lpe, double nv);
void   init_readout( READOUT_GRADIENT_T *grad, char name[], double lro, double np, double sw);
void   init_readout_butterfly( READOUT_GRADIENT_T *grad, char name[], double lro, double np, double sw, double gcrush, double tcrush);
void   init_readout_refocus( REFOCUS_GRADIENT_T *refgrad, char name[]);
void init_dephase( GENERIC_GRADIENT_T *grad, char name[] );

void calc_dephase( GENERIC_GRADIENT_T *grad, int write_flag, double moment0, char VJparam_g[], char VJparam_t[] );

void   init_rf ( RF_PULSE_T *rf, char rfName[MAX_STR], double pw, double flip, double rof1, double rof2);
void   shape_rf ( RF_PULSE_T *rf, char rfBase[MAX_STR], char rfName[MAX_STR], double pw, double flip, double rof1, double rof2);
void   init_slice_butterfly( SLICE_SELECT_GRADIENT_T *grad, char name[], double thk, double gcrush, double tcrush);
void   init_slice_refocus( REFOCUS_GRADIENT_T *refgrad, char name[]); 
void   trapezoid( GENERIC_GRADIENT_T *grad, char name[], double amp, double time, double moment, int write_flag);

/*************** declarteation of t_ functions *************/		
void   t_calc_generic( GENERIC_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]);
void   t_calc_phase( PHASE_ENCODE_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]);
void   t_calc_readout( READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_sw[], char VJparam_at[]);
void   t_calc_readout_refocus( REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   t_calc_readout_rephase( REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   t_calc_rf ( RF_PULSE_T *rf, char VJparam_tpwr[], char VJparam_tpwrf[]);
double t_calc_sim_gradient( GENERIC_GRADIENT_T  *grad0, GENERIC_GRADIENT_T  *grad1, GENERIC_GRADIENT_T  *grad2, double min_tpe, int write_flag);
void   t_calc_slice( SLICE_SELECT_GRADIENT_T *grad, RF_PULSE_T *rf, int write_flag, char VJparam_g[]);			
void   t_calc_slice_dephase( REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   t_calc_slice_refocus( REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
double t_calc_zfill_gradient(FLOWCOMP_T *grad0, GENERIC_GRADIENT_T  *grad1, GENERIC_GRADIENT_T  *grad2);
double t_calc_zfill_gradient2(FLOWCOMP_T *grad0, FLOWCOMP_T  *grad1, GENERIC_GRADIENT_T  *grad2);
double t_calc_zfill_gradient3(FLOWCOMP_T *grad0, FLOWCOMP_T  *grad1, FLOWCOMP_T  *grad2);
double t_calc_zfill_grad(GENERIC_GRADIENT_T *grad0, GENERIC_GRADIENT_T  *grad1, GENERIC_GRADIENT_T  *grad2);
void   t_init_generic( GENERIC_GRADIENT_T *grad, char name[], double amp, double time);
void   t_init_phase( PHASE_ENCODE_GRADIENT_T *grad, char name[], double lpe, double nv);
void   t_init_readout( READOUT_GRADIENT_T *grad, char name[], double lro, double np, double sw);
void   t_init_readout_butterfly( READOUT_GRADIENT_T *grad, char name[], double lro, double np, double sw, double gcrush, double tcrush);
void   t_init_readout_refocus( REFOCUS_GRADIENT_T *refgrad, char name[]);
void t_init_dephase( GENERIC_GRADIENT_T *grad, char name[] );

void t_calc_dephase( GENERIC_GRADIENT_T *grad, int write_flag, double moment0, char VJparam_g[], char VJparam_t[] );

void   t_init_rf ( RF_PULSE_T *rf, char rfName[MAX_STR], double pw, double flip, double rof1, double rof2);
void   t_shape_rf ( RF_PULSE_T *rf, 
                 char rfBase[MAX_STR],
                 char rfName[MAX_STR], 
                 double pw, 
                 double flip,
                 double rof1, 
                 double rof2);
void   t_init_slice_butterfly( SLICE_SELECT_GRADIENT_T *grad, char name[], double thk, double gcrush, double tcrush);
void   t_init_slice_refocus( REFOCUS_GRADIENT_T *refgrad, char name[]); 
void   t_trapezoid( GENERIC_GRADIENT_T *grad, char name[], double amp, double time, double moment, int write_flag);


/******************** declaration of X_ functions ***************/		
void   x_calc_generic( GENERIC_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]);
void   x_calc_phase( PHASE_ENCODE_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]);
void   x_calc_readout( READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_sw[], char VJparam_at[]);
void   x_calc_readout_refocus( REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   x_calc_readout_rephase( REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   x_calc_rf ( RF_PULSE_T *rf, char VJparam_tpwr[], char VJparam_tpwrf[]);
double x_calc_sim_gradient( GENERIC_GRADIENT_T  *grad0, GENERIC_GRADIENT_T  *grad1, GENERIC_GRADIENT_T  *grad2, double min_tpe, int write_flag);
void   x_calc_slice( SLICE_SELECT_GRADIENT_T *grad, RF_PULSE_T *rf, int write_flag, char VJparam_g[]);			
void   x_calc_slice_dephase( REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
void   x_calc_slice_refocus( REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, int write_flag, char VJparam_g[]);
double x_calc_zfill_gradient(FLOWCOMP_T *grad0 , GENERIC_GRADIENT_T  *grad1, GENERIC_GRADIENT_T  *grad2);
double x_calc_zfill_gradient2(FLOWCOMP_T *grad0 , FLOWCOMP_T  *grad1, GENERIC_GRADIENT_T  *grad2);
double x_calc_zfill_gradient3(FLOWCOMP_T *grad0 , FLOWCOMP_T  *grad1, FLOWCOMP_T  *grad2);
double x_calc_zfill_grad(GENERIC_GRADIENT_T  *grad0 , GENERIC_GRADIENT_T  *grad1, GENERIC_GRADIENT_T  *grad2);
void   x_init_generic( GENERIC_GRADIENT_T *grad, char name[], double amp, double time);
void   x_init_phase( PHASE_ENCODE_GRADIENT_T *grad, char name[], double lpe, double nv);
void   x_init_readout( READOUT_GRADIENT_T *grad, char name[], double lro, double np, double sw);
void   x_init_readout_butterfly( READOUT_GRADIENT_T *grad, char name[], double lro, double np, double sw, double gcrush, double tcrush);
void   x_init_readout_refocus( REFOCUS_GRADIENT_T *refgrad, char name[]);
void x_init_dephase( GENERIC_GRADIENT_T *grad, char name[] );

void x_calc_dephase( GENERIC_GRADIENT_T *grad, int write_flag, double moment0, char VJparam_g[], char VJparam_t[]);

void   x_init_rf ( RF_PULSE_T *rf, char rfName[MAX_STR], double pw, double flip, double rof1, double rof2);
void   x_shape_rf ( RF_PULSE_T *rf, 
                 char rfBase[MAX_STR],
                 char rfName[MAX_STR], 
                 double pw, 
                 double flip,
                 double rof1, 
                 double rof2);
void   x_init_slice_butterfly( SLICE_SELECT_GRADIENT_T *grad, char name[], double thk, double gcrush, double tcrush);
void   x_init_slice_refocus( REFOCUS_GRADIENT_T *refgrad, char name[]); 
void   x_trapezoid( GENERIC_GRADIENT_T *grad, char name[], double amp, double time, double moment, int write_flag);

#endif
