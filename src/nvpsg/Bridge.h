/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_BRIDGE_H
#define INC_BRIDGE_H


/* the extern "C" effectively prevents name mangling by C++ compiler */

#ifdef __cplusplus
#include "PSGFileHeader.h"
#define CEXTERN extern "C"
CEXTERN void InitAcqObject_writecode(int word);
CEXTERN void InitAcqObject_startTableSection(ComboHeader ch);
CEXTERN void InitAcqObject_endTableSection();
CEXTERN void AcodeManager_startSubSection(int header);
CEXTERN void AcodeManager_endSubSection();
CEXTERN int  AcodeManager_getAcodeStageWriteFlag(int stage);
CEXTERN void postInitScanCodes();
CEXTERN void func4sfrq(double value);
CEXTERN void func4dfrq(double value);
CEXTERN void func4dfrq2(double value);
CEXTERN void func4dfrq3(double value);
CEXTERN void func4dfrq4(double value);

// names assigned to these two functions must not duplicate existing patterns
CEXTERN void userRFShape(char *name, struct _RFpattern *pa, int nsteps, int channel);
CEXTERN void userDECShape(char *name, struct _DECpattern *pa, double dres, int mode, int steps, int channel);

#else
#define CEXTERN extern
#endif

/* H1 is just until map observe done */
// most user calls are in macros.h

#define  MAXSTATUSES    255            /* Maximum size of flag arrays */
CEXTERN int validrtvar(int);
CEXTERN void delay(double);
CEXTERN double calcDelay(double);
CEXTERN double pwCorr(double);
CEXTERN void hsdelay(double);
CEXTERN void statusDelay(int, double);
CEXTERN void preacqdelay(int);
CEXTERN void status(int);
CEXTERN void startacq(double);
CEXTERN void startacq_obs(double);
CEXTERN void startacq_rcvr(double);
CEXTERN void setacqvar(int);
CEXTERN void sendzerofid(int);
CEXTERN void setacqmode(int);
CEXTERN void acquire(double,double);
CEXTERN void parallelacquire_obs(double,double,double);
CEXTERN void parallelacquire_rcvr(double,double,double);
CEXTERN void sample(double tm);
CEXTERN void acquire_obs(double,double);
CEXTERN void acquire_rcvr(double,double);
CEXTERN void setRcvrPhaseVar(int var);
CEXTERN void dfltacq();
CEXTERN void dfltstdacq();
CEXTERN void dflthomoacq();
CEXTERN void dfltwfghomoacq();
CEXTERN void endacq();
CEXTERN void endacq_obs();
CEXTERN void endacq_rcvr();
CEXTERN void rcvron();
CEXTERN void rcvroff();
CEXTERN void recon();
CEXTERN void recoff();
CEXTERN void setactivercvrs(char *str);
CEXTERN void genRFPulse(double, int, double, double, int);
CEXTERN void gensim_pulse(double width1, double width2, int phase1,
		  int phase2, double rx1, double rx2, int rfdevice1,int rfdevice2);
CEXTERN void gensim_pulse(double width1, double width2, int phase1,
		  int phase2, double rx1, double rx2,
                  int rfdevice1,int rfdevice2);

CEXTERN void gensim3_pulse(double width1, double width2, double width3,
		     int phase1, int phase2, int phase3,
                     double rx1, double rx2,
                     int rfdevice1,int rfdevice2, int rfdevice3);

CEXTERN void gensim4_pulse(double width1, double width2, double width3, double width4,
		     int phase1, int phase2, int phase3, int phase4,
                     double rx1, double rx2,
                     int rfdevice1,int rfdevice2, int rfdevice3, int rfdevice4);

CEXTERN void genRFShapedPulse(double, char *, int, double, double, double, double, int);
CEXTERN void genRFshaped_rtamppulse(char *nm, double pw, int rtTpwrf, int rtphase,
   double rx1, double rx2, double g1, double g2, int rfdevice);

CEXTERN void gen_offsetlist(double *, double, double, double *, double, char, int);

CEXTERN void gen_offsetglist(double *, double *, double, double *, double, char, int);

CEXTERN int  gen_shapelist_init(char *, double, double *, double, double, char, int);

CEXTERN double gen_shapelistpw(char  *baseshape, double pw, int rfch);

CEXTERN void gen_shapedpulselist(int, double, int, double, double, char, int, int);

CEXTERN void gen_shapedpulseoffset(char *name, double pw, int rtvar, double rof1, double rof2, double offset, int rfch);

CEXTERN void swift_acquire(char *shape, double pwon, double preacqdelay);

CEXTERN double gen_poffset(double pos, double grad, int rfch);

CEXTERN double nuc_gamma();

CEXTERN void gensim2shaped_pulse(char *name1, char *name2, double width1,
  double width2, int phase1, int phase2, double rx1, double rx2,
  double g1, double g2, int rfdevice1, int rfdevice2);

CEXTERN void genspinlock(char *name, double pw_90, double deg_res,
   int phsval, int ncycles,int rfdevice);

CEXTERN void gensim3shaped_pulse(char *name1, char *name2, char *name3, double width1, double width2,
                 double width3, int phase1, int phase2, int phase3, double rx1, double rx2,
			 double g1, double g2, int rfdevice1, int rfdevice2, int rfdevice3);

CEXTERN void prg_dec_on(char *name, double pw_90, double deg_res, int rfdevice);
CEXTERN void prg_dec_on_offset(char *name, double pw_90, double des_res, double foff, int rfdevice);
CEXTERN void prg_dec_off(int rfdevice);
CEXTERN void setstatus(int channel, int state, char mode, int sync, double modfrq);
CEXTERN void resolve_endofscan_actions();
CEXTERN void resolve_endofexpt_actions();
CEXTERN void resolve_initscan_actions();

CEXTERN void genFullPhase(int vvar, int rfch);
CEXTERN void definePhaseStep(double step, int rfch);

CEXTERN void zgradpulse(double amp, double duration);

CEXTERN void genBlankOnOff(int state, int rfch);

CEXTERN void genTxGateOnOff(int state, int rfch);

CEXTERN void genPower(double power, int rfch);

CEXTERN void genPowerF(double finepower, int rfch);

CEXTERN void genVPowerF(int vindex, int rfch);

CEXTERN void defineVAmpStep(double vstep, int rfch);

CEXTERN void genPhase(int phase, int rfch);

CEXTERN void genOffset(double offsetValue, int rfch);

CEXTERN void genXmtr(int state, int rfch);

CEXTERN void ifzero_bridge(int rtvar);
CEXTERN void ifmod2zero_bridge(int rtvar);
CEXTERN void elsenz_bridge(int rtvar);
CEXTERN void endif_bridge(int rtvar);

CEXTERN void nowait_loop(double loops, int rtvar1, int rtvar2);
CEXTERN void nowait_endloop(int rtvar);

CEXTERN void xgate(double events);

CEXTERN void splineon(int whichone);
CEXTERN void splineoff(int whichone);

CEXTERN void setTempComp(int state);

CEXTERN void showpowerintegral();

CEXTERN void notImplemented();

CEXTERN void broadcastCodes(int code, int many, int *codeptr);
CEXTERN void broadcastLoopCodes(int code, int many, int *codeptr,
                                int count, long long ticks);

CEXTERN void lk_sample(void);

CEXTERN void lk_hold(void);

CEXTERN void lk_autotrig(void);

CEXTERN void lk_sampling_off(void);

CEXTERN void arrayedshims();

CEXTERN void set4Tune(int chan, double gain);

CEXTERN void setRcvrLO(int chan);

CEXTERN void XmtNAcquire(double pw,int phase, double rof1);

CEXTERN void ShapedXmtNAcquire(char *shape, double pw, int phase, double rof1, int chan);

CEXTERN void MultiChanSweepNAcquire(double fstartMhz, double fendMHz,
                                    int nsteps, unsigned int chanmap,
                                    double offset_sec);

CEXTERN void SweepNOffsetAcquire(double fstartMhz, double fendMHz,
                                 int nsteps, int chan, double offset_sec);

CEXTERN void SweepNAcquire(double fstartMhz, double fendMHz,
                           int nsteps,int chan);

CEXTERN void genRFShapedPulseWithOffset(double pw, char *name, int rtvar, \
  double rof1, double rof2, double offset, int rfch);

CEXTERN void vdelay(int timecode, int rtvar);

CEXTERN int genCreate_delay_list(double *list, int nvals);

CEXTERN void vdelay_list(int listId, int rtvar);

CEXTERN void startTicker(int);

CEXTERN long long stopTicker(int);

CEXTERN void diplexer_override(int state);

CEXTERN void rotorsync(int);

CEXTERN void rotorperiod(int);

CEXTERN double getTimeMarker();

CEXTERN int parmToRcvrMask(const char *parName, int calltype);
CEXTERN int isRcvrActive(int n);
CEXTERN char *RcvrMapStr(const char *parmname, char *mapstr);
CEXTERN int getNumActiveRcvrs();

CEXTERN void triggerSelect(int);

CEXTERN void readMRIUserByte(int rtvar, double delay);
CEXTERN void readMRIUserBit(int rtvar, double delay);
CEXTERN void writeMRIUserByte(int rtvar);

CEXTERN int initRFGroupPulse(double pw, char *name, char mode, double cpwr, double fpwr, double sphase, double *fa, int num);
CEXTERN void modifyRFGroupName(int pulseid,int num, char *name);
CEXTERN void modifyRFGroupPwrf(int pulseid,int num, double pwrf);
CEXTERN void modifyRFGroupPwrC(int pulseid,int num, double pwr);
CEXTERN void modifyRFGroupOnOff(int pulseid,int num, int flag);
CEXTERN void modifyRFGroupSPhase(int pulseid,int num, double ph);
CEXTERN void modifyRFGroupFreqList(int pulseid,int num, double *fl);
CEXTERN void GroupPulse(int pulseid, double rof1, double rof2,  int vphase, int vselect);
CEXTERN void TGroupPulse(int pulseid, double rof1, double rof2,  int vphase, int vselect);
CEXTERN void GroupXmtrPhase(int rtvar);
CEXTERN void GroupPhaseStep(double step);
CEXTERN void MagnusSelectFPower();

CEXTERN void ifrtGT(int rtarg1, int rtarg2, int rtarg3);
CEXTERN void ifrtGE(int rtarg1, int rtarg2, int rtarg3);
CEXTERN void ifrtLT(int rtarg1, int rtarg2, int rtarg3);
CEXTERN void ifrtLE(int rtarg1, int rtarg2, int rtarg3);
CEXTERN void ifrtEQ(int rtarg1, int rtarg2, int rtarg3);
CEXTERN void ifrtNEQ(int rtarg1, int rtarg2, int rtarg3);
CEXTERN void enableHDWshim();
CEXTERN void hdwshiminit();
CEXTERN void setAmpBlanking();
CEXTERN void mgain(int which, double fgain);
CEXTERN void dfltwfghomoacq();
CEXTERN void create_offset_list(double *off, int num, int device, int list);
CEXTERN void voffsetch(int vvar,int list, int device);
CEXTERN void parallelstart(const char *chanType);
CEXTERN double parallelend();
CEXTERN void parallelsync();
CEXTERN void parallelshow();
CEXTERN int rlloop(int count, int rtcount, int rtcounter);
CEXTERN void rlendloop(int rtcounter);
CEXTERN int kzloop(double duration, int rtcount, int rtcounter);
CEXTERN void kzendloop(int rtcounter);
CEXTERN void setKzDuration(double duration);
CEXTERN long long stopTickerKzLoop( double *remTime);
#endif
