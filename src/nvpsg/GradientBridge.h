/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_GRADIENTBRIDGE_H
#define INC_GRADIENTBRIDGE_H

#ifdef __cplusplus
#define CEXTERN extern "C"
CEXTERN void vgradient(char axis, double value, int rtvar);
#else
#define CEXTERN extern
#endif

CEXTERN void rgradientCodes(char axis, double value);
CEXTERN void rgradient(char axis, double value);
CEXTERN void settmpgradtype(char *axis);
CEXTERN void zero_all_gradients(void);

/* axis = "yyy" "nny" x, then y, then z 
 * no error if disabling a non-configrured shim
 */
CEXTERN void pfgenable(char *axis);

CEXTERN int  getorientation(char *c1,char *c2,char *c3,char *orientname);

CEXTERN void grad_limit_checks(double xgrad,double ygrad,double zgrad,char *routinename);

CEXTERN void set_rotation_matrix(double ang1, double ang2, double ang3);

CEXTERN int create_rot_angle_list(char *nm, double* angle_set, int num_sets);

CEXTERN void rot_angle_list(int listId, char mode, int index);

CEXTERN int create_angle_list(char *nm, double* angle_set, int num_sets);

CEXTERN void set_angle_list(int angleListId, char *logAxis, char mode, int rtvar);

CEXTERN void exe_grad_rotation();

CEXTERN void shapedgradient(char *name,double width,double amp,char which,
		      int loops,int wait_4_me);

CEXTERN void phase_encode3_gradient(double stat1,double stat2,double stat3,
                            double step1,double step2,double step3,
                            int   vmult1,int   vmult2,int   vmult3,
                            double  lim1,double  lim2,double  lim3);

CEXTERN void phase_encode3_oblshapedgradient(char *pat1, char *pat2, char *pat3, double width,
                                     double stat1, double stat2, double stat3,
                                     double step1, double step2, double step3,
                                     int vmult1, int vmult2, int vmult3,
                                     double lim1, double lim2, double lim3,
                                     int loops, int wait4me, int tag);
CEXTERN void d_phase_encode3_oblshapedgradient(char *pat1, char *pat2, char *pat3, char *pat4, char *pat5, char *pat6,          \
                                     double stat1, double stat2, double stat3, double stat4, double stat5, double stat6,     \
                                     double step1, double step2, double step3, double step4, double step5, double step6,     \
                                     int vmult1, int vmult2, int vmult3, int vmult4, int vmult5, int vmult6,                 \
                                     double lim1, double lim2, double lim3, double lim4, double lim5, double lim6,            \
                                     double width, int loops, int wait4me, int tag);


CEXTERN void phase_encode3_gradpulse(double stat1, double stat2, double stat3,
     double duration, double step1, double step2, double step3,
     int vmult1, int vmult2, int vmult3, double lim1, double lim2, double lim3);

CEXTERN void oblique_gradpulse(double level1,double level2,double level3,double ang1,double ang2,double ang3,double width);

CEXTERN void magradientpulse(double gradlvl, double gradtime);

CEXTERN void putEccAmpsNTimes(int n, int amps[], long long times[]);
CEXTERN void putSdacScaleNLimits(int n, int vlaues[]);
CEXTERN void putDutyLimits(int n, int values[]);

CEXTERN void setMRIUserGates(int rtvar);
CEXTERN int  grad_advance(double dur);
CEXTERN void getgradpowerintegral(double *powerarray);

#endif
