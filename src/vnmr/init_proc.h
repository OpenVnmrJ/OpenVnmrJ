/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INIT_PROC_H
#define INIT_PROC_H

extern int set_mode(char *m_name);
extern void set_fid_proc();
extern void set_spec_proc(int direction0, int proc0, int revnorm0,
                   int direction1, int proc1, int revnorm1);
extern void get_ref_pars(int direction, double *swval, double *ref, int *fnval);
extern int get_av_mode(int direction);
extern int get_phase_mode(int direction);
extern int get_phaseangle_mode(int direction);
extern int get_dbm_mode(int direction);
extern int get_mode(int direction);
extern int get_dimension(int direction);
extern int get_direction(int revnorm);
extern int get_direction_from_id(char *id_name);

#endif
