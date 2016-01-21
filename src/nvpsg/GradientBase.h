/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* GradientBase.h */

#ifndef INC_GRADIENTBASE_H
#define INC_GRADIENTBASE_H
#include "Controller.h"

class GradientBase: public Controller
{
   protected:
     int rotm_11, rotm_12, rotm_13;
     int rotm_21, rotm_22, rotm_23;
     int rotm_31, rotm_32, rotm_33;
     PowerMonitor Xpower,Ypower,Zpower;
     int dacLimit;

   public:
     GradientBase(const char *name,int flags):Controller(name,flags),Xpower(1.0e18,10.0,"Grad X"),Ypower(1.0e18,10.0,"Grad Y"),Zpower(1.0e18,10.0,"Grad Z")
     {
       Xpower.setGateONOFF(1.0);
       Ypower.setGateONOFF(1.0);
       Zpower.setGateONOFF(1.0);
     };

     double GMAX_TO_DAC;
     double GMAX;
     int    GXSCALE;
     int    GYSCALE;
     int    GZSCALE;

     int initializeExpStates(int setupflag);
     void setEnable(char *cmd);
     void setGates(int GatePattern);
     virtual void setGrad(const char *which, double value);
     virtual void setGradScale(const char *which, double value);
     void setVGrad(char *which, double step, int rtvar);
     void setRotArrayElement(int index1, int index2, double value);
     int initializeIncrementStates(int num);
     virtual int getDACLimit();
#ifdef NEXT
     void setRotation(double ang1, double ang2, double ang3);
#endif
     virtual cPatternEntry *resolveGrad1Pattern(char *nm, int flag, char *emsg, int action);
     virtual cPatternEntry *resolveOblShpGrdPattern(char *nm, int flag, const char *emsg, int action);
     virtual int userToScale(double xx);
     virtual int ampTo16Bits(double xx);
     virtual int errorCheck(int checkType, double g1, double g2, double g3, double s1, double s2, double s3);
     void set_rotation_matrix(double ang1, double ang2, double ang3);
     int create_rotation_list(char *name, double* angle_set, int num_sets);
     void set_angle_list(int angleList, char *angleName, char mode, int rtvar);
     int create_angle_list(char *name, double *angle_set, int num_sets);
     void calc_obl_matrix(double ang1,double ang2,double ang3,double *tm11,double *tm12,   \
                          double *tm13,double *tm21,double *tm22,double *tm23,             \
                          double *tm31,double *tm32,double *tm33);
     void oblpegrad_limit_check(int Gx, int Gy, int Gz, int peR_step, int peP_step, int peS_step, \
                                                    int peR_lim, int peP_lim, int peS_lim);
     void calcGradientRotation(int Gr, int Gp, int Gs, int peR_step, int peP_step, int peS_step,  \
                                                    int peR_mult, int peP_mult, int peS_mult,     \
                                                    int Gr_rms, int Gp_rms, int Gs_rms, int *G_array);
     int estimatePELoopCount(int vmult, double numpe);
     void computeOBLPESHPGradPower(int when, cPatternEntry *Gr_wfg, cPatternEntry *Gp_wfg, cPatternEntry *Gs_wfg, \
                                        int whichones, int Gr, int Gp, int Gs, int Gr_step, int Gp_step, int Gs_step, \
                                        int Gr_mult, int Gp_mult, int Gs_mult,                                    \
                                        int Gr_lim, int Gp_lim, int Gs_lim, int GradMaskBit);
     void powerMonLoopStartAction();
     void powerMonLoopEndAction();
     void PowerActionsAtStartOfScan();
     void computePowerAtEndOfScan();
     virtual void showPowerIntegral();
     virtual void resetPowerEventStart();
     virtual void getPowerIntegral(double *powerarray);
     virtual void showEventPowerIntegral(const char *);
     void eventStartAction();
};
#endif
