/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_DDRCONTROLLER_H
#define INC_DDRCONTROLLER_H
#include "Controller.h"

#define TIMERCLOCKFREQ 80e6
#ifndef UINT
#define UINT unsigned int
#endif

class DDRController: public Controller
{
  private:
    unsigned int filter_init;
    unsigned int acq_on;
    unsigned int acq_mode;
    unsigned int debug;
    
    double    simnoise;
    double    simfscale;
    double    simascale;
    double    simlw;
    double    simdly;
    int       simpeaks;

    void reset_all();
    
    long long acqticker;
    void clearAcqTicker();
    long long getAcqTicker();
    double sw1,os1,xs1,fw1;
    double sw2,os2,xs2,fw2;
    double cor,maxcr;
    double sr,sd;
    double cp, cf, ca, tc;
    unsigned int mode;
    unsigned int np;
    unsigned int al,dl,ns,nf,nfmod,nacms,nacqs,dbytes,ascale;
    unsigned int n1,m1,b1,n2,m2,b2,ny1,nx1,stages;
    unsigned int bkpts,nfids,bmax,nmax,amax;
    unsigned int acycles,tp1,tp2,nblks;
    long long acqTicks,scanTicks,sampleTicks;
    double pstep;
    int RF_LO_source;
    
  public:
    int acq_started;
    int acq_completed;
    int ddrActive4Exp;
    int chid;
    double aqtm;

    DDRController(char *name,int flags,int id);

    // virtual override functions

    int initializeExpStates(int setupflag); 
    int initializeIncrementStates(int num);

    // public functions

    void reset_acq();

    int findMaxBufs(int nt, int bs, int arraydim, int nf, int nmod);
    int outputDDRACode(int Code, int many, int *codes);
    void setDDRDelay(double);
    void setAllGates(int GatePattern);
    void setGates(int GatePattern);
    void clrGates(int GatePattern);
    long long Acquire();                     // implicit acquire
    long long startAcquire(double t);        // explicit acquire start
    long long startAcquireTicksOnly(double t);        // time for explicit acquire start
    long long Sample(double dw);             // explicit acquire sample
    long long endAcquire();                  // explicit acquire end
    void setAcqMode(int m);                  // set acquisition mode
    int getAcqMode();                        // get acquisition mode
    void setAcqVar(int m);                   // set acquisition rtvar
    void sendZeroFid(int ix, int arrayDim);  // send zeros instead of FID
    int getAcqVar();                         // get acquisition rtvar
    void setReceiverGate(int);
    void setRcvrPhaseVar(int rtvar);         // real time phase control
    void setRcvrPhaseStep(double);           // real time phase step size
    void setDataGate(int);
    void setAcqGate(int);
    void packd(double d, int *i, int &index);
    double unpackd(int *i,int &index);
    int nextscanDDR();
    void inactiveRcvrSetup();
    void getexpars();
    double acq_time();
    double sweep_width(); // actual sw
    void show_filter();
    void calc_filter();
    void show_exp_mode();
    long long pushloop(int);
    long long poploop(int);
    double calc_maxcr(double sr, int stages, int m1);
    void calc_stages(double sr, double sw, UINT *stages, UINT *m1, UINT *m2);
    void set_RF_LO_source(int rfch);
    int get_RF_LO_source();
    void setDDRActive4Exp(int active);
};

#endif
