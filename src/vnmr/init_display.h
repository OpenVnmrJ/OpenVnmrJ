/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INIT_DISPLAY_H
#define INIT_DISPLAY_H

extern void set_fid_display();
extern void reset_disp_pars(int direction, double swval, double wpval,
                            double spval);
extern void set_spec_display(int direction0, int dim0, int direction1,
                            int dim1);
extern void set_display_label(int direction0, char char0, int direction1,
                            char char1);
extern void get_display_label(int direction, char *charval);
extern void get_sfrq(int direction, double *val);
extern void get_sw_par(int direction, double *val);
extern void get_reference(int direction, double *rfl_val, double *rfp_val);
extern void get_rflrfp(int direction, double *val);
extern int get_axis_freq(int direction);
extern void get_mark2d_info(int *first_ch, int *last_ch, int *first_direction);
extern void DispField1(int pos, int color, char *label);
extern void DispField2(int pos, int color, double val, int dez);
extern void ResetLabels();
extern void EraseLabels();
extern void InitVal(int pos, int direction, int n_index, int n_color,
                    int n_style, int p_index, int p_color, int p_scale_it,
                    int p_decimal);
extern void UpdateVal(int direction, int n_index, double val, int displ);
extern void DispField(int pos, int color, char *n_name,
                      double val, int decimal);
extern void get_label(int direction, int style, char *label);
extern void set_scale_label(int direction, char *label);
extern void set_scale_axis(int direction, char axisv);
extern void set_scale_rev(int direction, int rev);
extern void set_scale_start(int direction, double start);
extern void get_axis_label(int direction, char *label);
extern void set_axis_label(int direction, char *label);
extern void set_scale_len(int direction, double len);
extern void get_scale_axis(int direction, char *axisv);
extern void set_cursor_pars(int direction, double crval, double deltaval);
extern void get_cursor_pars(int direction, double *crval, double *deltaval);
extern void set_scale_pars(int direction, double start, double len,
                    double scl, int rev);
extern void get_scale_pars(int direction, double *start, double *len,
                    double *scl, int *rev);
extern void get_intercept(int direction, double *intercept);
extern void get_scalesw(int direction, double *scaleswval);
extern void get_phase_pars(int direction, double *rpval, double *lpval);
extern double convert2Hz(int direction, double val);
extern double convert2ppm(int direction, double val);
extern double dss_sc, dss_wc, dss_sc2, dss_wc2;
void get_nuc_name(int direction, char *nucname, int n);
void getReffrq(int direction, double *val);

#endif
