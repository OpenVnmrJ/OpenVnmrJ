/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DSCALE_H
#define DSCALE_H

extern void init_proc2d();
extern void activate_mouse();
extern int fid_ybars(float *phasfl, double scale, int df, int dn,
              int n, int off, int newspec, int dotflag);
extern int fill_ybars(int df, int dn, int off, int newspec);
extern int abs_ybars(float *phasfl, double scale, int df, int dn, int n,
              int off, int newspec, int dotflag, int sgn, int env);
extern void envelope(short *buf, int n, int sgn);
extern int envelope_ybars(float *phasfl, double scale, int df, int dn,
                   int n, int off, int newspec, int dotflag, int sgn);
extern int calc_ybars(float *phasfl, int skip, double scale, int df, int dn,
               int n, int off, int newspec);
extern int calc_plot_ybars(float *phasfl, int skip, double scale, int df,
                    int dn, int n, int off, int newspec);
extern int datapoint(double freq, double sw, int rfn);
extern int int_ybars(float *integral, double scale, int df, int dn, int fpt,
              int npt, int off, int newspec, int resets, double sw, int fn);
extern int displayspec(int df, int dn, int vertical, int *newspec, int *oldspec,
                int *erase, int max, int min, int dcolor);
extern void erasespec(int specindex, int vertical, int dcolor );
extern int set_cursors(int two_cursors, int either_cr, int *newpos,
                int oldpos, int oldpos2, int *idelta, int df, int dn);
extern float vs_mult(int x_pos, int y_pos, int max, int min, int df,
                     int dn, int index);
extern struct ybar *outindex(int index);
extern void exit_display();
extern void init_display();
extern void dpreal(double v, int dez, char *nm);
extern void erase_dcon_box();
extern void scale2d(int drawbox, int yoffset, int drawscale, int dcolor);
extern int dscale_on();
extern int dscale_onscreen();
extern int new_dscale(int erase, int draw);
extern void clear_dscale();
extern void reset_dscale();
extern void addi_dscale();
extern void getFrameScalePars(double *sp, double *wp, double *scl, double *vp_off,
                  int *rev, int *color, int *vp, int *df, int *dn, int *df2,
                  int *axis, int *flag);
extern void setFrameScalePars(double sp, double wp, double scl, double vp_off,
                  int rev, int color, int vp, int df, int dn, int df2,
                  int axis, int flag);

#define MAYBE 100

#endif
