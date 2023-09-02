/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//  DDR gate symbols
//  ------------
//  DDR_SCAN - causes scan boundary interrupts
//  DDR_FID  - causes fid boundary interrupts
//  DDR_PINS - DDR AD6634 Pin Sync (resets ddr receiver phase)
//  DDR_RG   - receiver gate 0:off 1:on  (need to invert in hardware)
//  DDR_IEN  - AD6634 Input Enable (0:input forced to zero 1:input valid)
//  DDR_ACQ  - enables/disables data capture into DATA FIFO
//=========================================================================
#define PSOFFMIN 2e-6    // minimum pinsync off time
#define TMIN     50e-9   // minimum time
#define PSON     50e-9   // minimum pinsync on time

#include <unistd.h>
#include <string.h>
#include "Console.h"
#include "DDRController.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "cpsg.h"
#include "ddr_symbols.h"
#include "ddr_fifo.h"
#include "stdio.h"
#include "acqparms.h"

#define WriteWord( x ) pAcodeBuf->putCode( x )

//#define USE_AQTM   // undefine to disable filter reflection at end of fid
#define ANALOGPLUS 7.0
#define BRICKWALL  75.0
#define MINSW      10e3
#define MAXSW      5e6
#define MAXOVS     10
#define DFLTSW1    100e3

#ifndef MAXSTR
#define MAXSTR     256  // needed for getstr
#endif

#define ACQ_GATES DDR_RG|DDR_IEN|DDR_ACQ|DDR_PINS|DDR_SCAN|DDR_FID
#define S2T(x)   calcTicks(x)

#define ALLGATES(x)      encode_DDRSetGates(0,ACQ_GATES,x)
#define SETGATES(x)      encode_DDRSetGates(0,(x),(x))
#define CLRGATES(x)      encode_DDRSetGates(0,(x),0)
#define MSKGATES(m,x)    encode_DDRSetGates(0,(m),(x))
#define HOLDGATES        encode_DDRSetGates(0,0,0)

extern int rcvr_is_on_now;
extern int rcvroff_flag;
extern int dps_flag;
extern "C" {
extern int DPSprint(double, const char *, ...);
}

//=========================================================================
// DDRController::DDRController constructor
//=========================================================================
DDRController::DDRController(char *name,int flags,int id):Controller(name,flags)
{
    chid=id;
    kind = DDR_TAG;
    ddrActive4Exp = flags;
    reset_all();
    debug=0;
    // the follwoing were missing
    RF_LO_source = 0;
}

//=========================================================================
//  DDRController::initializeIncrementStates new fid (array)
//=========================================================================
int DDRController::initializeIncrementStates(int num)
{
  setTickDelay((long long) num);
  clearSmallTicker(); // this should be unnecessary
  return(0);
}

//=========================================================================
//  DDRController::setGates set DDR gates
//=========================================================================
void DDRController::setGates(int state)
{
    if(ddrActive4Exp){
        if(debug>2)
            printf("DDRController<%d>::setGates(0x%X)\n",chid,state);
        add2Stream(SETGATES(state));
    }
}

//=========================================================================
//  DDRController::clrGates clear DDR gates
//=========================================================================
void DDRController::clrGates(int state)
{
    if(ddrActive4Exp){
        if(debug>2)
            printf("DDRController<%d>::clearGates(0x%X)\n",chid,state);
        add2Stream(CLRGATES(state));
    }
}

//=========================================================================
//  DDRController::setAllGates set DDR gates (all enabled)
//=========================================================================
void DDRController::setAllGates(int GatePattern)
{
    add2Stream( encode_DDRSetGates(0,ACQ_GATES,GatePattern) );
}

//=========================================================================
//  DDRController::outputDDRACode send off an acode
//=========================================================================
int DDRController::outputDDRACode(int code, int n, int *codes)
{
    if(ddrActive4Exp){
        if(debug>2)
            printf("DDRController<%d>::outputDDRACode(id=%d,args=%d)\n",
              chid,code&(~ACDKEY),n);
        return outputACode(code, n, codes);
    }
    else
        return 0;
}

//=========================================================================
//  DDRController::setDDRDelay set DDR delay
//=========================================================================
void DDRController::setDDRDelay(double dly)
{
    if(debug>2)
        printf("DDRController<%d>::setDelay(%g)\n",chid,dly);
    setDelay(dly);
}

//=========================================================================
// DDRController::clearAcqTicker reset private ticker
//=========================================================================
void DDRController::clearAcqTicker()
{
    acqticker=getBigTicker(); // get absolute tick counter
}

//=========================================================================
// DDRController::getAcqTicker get private ticker
//=========================================================================
long long DDRController::getAcqTicker()
{
   return getBigTicker()-acqticker;
}

//=========================================================================
// DDRController::reset_acq reset acquisition parameters
//=========================================================================
void DDRController::reset_acq()
{
    acqTicks=0;
    acq_on=0;
    acycles=1;
    acq_started=0;
    acq_completed=0;
    clearAcqTicker();
}

//=========================================================================
// DDRController::reset_all reset all parameters
//=========================================================================
void DDRController::reset_all()
{
    sw1=DFLTSW1;
    os1=os2=7;
    xs1=xs2=0;
    sw2=0;
    sr=5.0e6;
    n1=m1=b1=n2=m2=b2=ny1=nx1=stages=0;
    fw1=fw2=1;
    filter_init=0;
    scanTicks=sampleTicks=0;
    mode=MODE_DMA|AD_FILTER2|AD_SKIP3;
    dbytes=8;
    aqtm=0;
    nacms=nacqs=ns=1;
    debug=0;
    bkpts=0;
    reset_acq();
    sd=0;
    cf=cp=0;
    ca=1;
    maxcr=70;
    cor=7;
    acq_mode=0;
    pstep=1;
    nf=1;
    ascale=0;
    nfmod=0;
    simlw=50;
    simdly=0;
    simnoise=0.01;
    simascale=0.9;
    simfscale=1.0;
    simpeaks=6;
    // These following were missing and added by BDZ on 4-16-23.
    // Apparently they all are set by intializeExpStates() as many of
    // above are too.
    tc = 0;
    np = 0;
    al = 0;
    dl = 0;
    nfids = 0;
    amax = 0;
    bmax = 0;
    nmax = 0;
    tp1 = 0;
    tp2 = 0;
    nblks = 0;
}

//=========================================================================
//  DDRController::unpackd pack a double from two integers
//=========================================================================
double DDRController::unpackd(int *buffer,int &index)
{
    union64 tmp;
    tmp.w2.word0 = buffer[index];
    tmp.w2.word1 = buffer[index+1];
    index += 2;
    return tmp.d;
}

//=========================================================================
//  DDRController::packd pack a double into two integers
//=========================================================================
void DDRController::packd(double d, int *buffer, int &index)
{
    union64 tmp;
    tmp.d=d;
    buffer[index] = tmp.w2.word0;
    buffer[index+1] = tmp.w2.word1;
    index += 2;
}

//=========================================================================
//  DDRController::findMaxBufs return max data buffers needed
//=========================================================================
int DDRController::findMaxBufs(int ntv, int bsv, int fids, int nfv, int nm)
{
    // determine the maximum number of fid buffers required for Experiment

    int ntval, nbsval, ndim, ntmax;
    int bufs;

    ntval = ntv;
    nbsval = bsv;
    ndim = fids;

    // calculate data buffers from nt, bs, and acqcycles
    // buffer after calculation

    ntmax = getmaxval("nt");
    if (ntmax > ntval)
        ntval = ntmax;
    if ((nbsval > 0) && ((ntval/nbsval) >= 1)) {
        if ( (ntval%nbsval) > 0)
            bufs = (((ntval/nbsval)+1)*ndim);
        else
            bufs = ((ntval/nbsval)*ndim);
    }
    else if (ndim > 1)
        bufs = ndim;
    else
        bufs = 1;    /* add 1 if even decide to do noise check */

    if (nfv > 1)
       bufs = bufs * nfv/nm;
    //printf("ndim %d nbsval: %d ntval: %d nf: %d ntmax %d\n",
    //           ndim,nbsval,ntval,nf,ntmax);
    return(bufs);
}

//=========================================================================
// calc_filter: calculate filter variables
//=========================================================================
void DDRController::calc_filter()
{
    int l1=(mode & MODE_L1)?2:1;
    double sw=getval("sw");
    if(filter_init)
        return;
    if(stages==0){
        nx1=ny1=al;
    }
    else{
        m1=(int)(sr/sw1+0.5);
        n1=(int)(os1*sr/sw1+0.5);
        n1=(n1%2==0)?n1+1:n1;
        if(xs1>=1 || xs1<=-1)
            b1=(int)(0.5*n1-xs1);
        else
            b1=(int)(n1*(0.5-xs1));
        b1= (b1<0) ? 0 : b1;
        if(stages==2){
            m2=(int)(sw1/sw2+0.5);
            n2=(int)(os2*sw1/sw2+0.5);
            n2=(n2%2==0)?n2+1:n2;
            if(xs2>=1 || xs2<=-1)
                b2=(int)(0.5*n2-xs2);
            else
                b2=(int)(n2*(0.5-xs2));
            b2= (b2<0) ? 0 : b2;
            ny1=(al-1)*m2+n2-b2;
            nx1=(ny1-1)*m1/l1+n1-b1;
        }
        else{
            ny1=al;
            nx1=(al-1)*m1/l1+n1-b1;
        }
        if(mode & MODE_ATAQ){
            nx1=(int)(al*sr/sw);
        }
    }
    filter_init=1;
}

//=========================================================================
// calc_stages: calculate filter stages and decimation factors from sw
//=========================================================================
void DDRController::calc_stages(double sr, double sw, UINT *n, UINT *a, UINT *b)
{
    UINT stages=0;
    UINT m1=1;
    UINT m2=1;
    int l1=(mode & MODE_L1)?2:1;
    if(sw>1.25e6){
       stages=0;
    }
	else if (sw >= 100000) {
		stages = 1;
	} else if (sw > 50000) {
		m2 = 2;
		stages = 2;
	} else if (sw > 5000) {
		m2 = 4;
		stages = 2;
	} else {
		m2 = 8;
		stages = 2;
	}
	m1 = (int) (sr*l1/m2/sw + 0.5);
	*n=stages;
	*a=m1;
	*b=m2;
}

//=========================================================================
// calc_maxcr: calculate max filter cor
//=========================================================================
double DDRController::calc_maxcr(double sr, int stages, int m1)
{
	double cr = 0;
	if(mode & MODE_L1)
	   m1/=2;
	if(stages==0){
	    cr=0;
	}
	else if (sr > 3e6) {//5 MHz sampling
		if (stages == 1) {
			if (m1 == 5)
				cr = 6;
			else if (m1 <= 15)
				cr = 0.7 * m1 + 1;
			else
				cr = 14;
		} else {
			if (m1 > 130)
				cr = 1.17 * m1 + 400;
			if (m1 > 50)
				cr = 3.6 * m1 + 100;
			else
				cr = 6.8 * m1 - 80;
		}
	} else {  //2.5 MHz sampling
		if (stages == 1) {
			if (m1 == 2)
				cr = 7;
			else if (m1 == 3)
				cr = 20;
			else if (m1 == 4)
				cr = 30;
			else if (m1 <=10)
				cr = 35;
			else
				cr = 40;
		} else {
			if (m1 > 14)
				cr = 11.0 * m1 + 60;
			else
				cr = 23 * m1 - 90;
		}
	}
	return cr>1000?1000:cr;
}

//=========================================================================
// acq_time() return calculated acquisition time
//=========================================================================
double DDRController::acq_time()
{
    calc_filter();

    double at=nx1/sr;      // at needed for filter in scan

    // additional time needed for post processing latency

    return at;
}

//=========================================================================
// sweep_width() return calculated sweep width
//=========================================================================
double DDRController::sweep_width()
{
    calc_filter();
    if(os1==0 || sw1==0)
       return sr;
    if(os2==0 || sw2==0)
       return sr/m1;
    return sr/m1/m2;
}

//-------------------------------------------------------------
// DDRController::show_exp_mode() print ddr exp mode
//-------------------------------------------------------------
void DDRController::show_exp_mode()
{
    char buff[256];
    buff[0]=0;
    if((mode&DATA_TYPE)==DATA_DOUBLE)
        strcat(buff,"|DP");
    if(mode&DATA_PACK)
        strcat(buff,"|2x16");
    if(mode&MODE_L1)
        strcat(buff,"|L1");
    if(mode&MODE_L2)
        strcat(buff,"|L2");
    if(mode&MODE_ATAQ){
        if(mode&MODE_ATAQ_DR)
            strcat(buff,"|ATAQ<DR>");
        else
            strcat(buff,"|ATAQ");
    }
    if(mode&MODE_DMA)
        strcat(buff,"|DMA");
    if(mode&MODE_ADC_OVF)
        strcat(buff,"|ADCOVF");
    if(mode&MODE_PHASE_CONT)
        strcat(buff,"|PHASE_CONT");
    if(mode&MODE_ADC_DEBUG)
        strcat(buff,"|ADC_DEBUG");
    if(mode&MODE_DITHER_OFF)
        strcat(buff,"|DITHER_OFF");
    if(mode&MODE_NOPS)
        strcat(buff,"|NOPS");
    if(mode&MODE_HWPF)
        strcat(buff,"|HWPF");
    if(mode&MODE_FIDSIM)
        strcat(buff,"|FIDSIM");
    if(mode&MODE_CFLOAT)
        strcat(buff,"|CFLOAT");
    if((mode&PF_TYPE)==PF_QUAD)
        strcat(buff,"|PF_QUAD");

    if((mode&AD_FILTER)==AD_FILTER1)
        strcat(buff,"|FILTER1");
    else if((mode&AD_FILTER)==AD_FILTER2)
        strcat(buff,"|FILTER2");

    if((mode&AD_SKIP)==AD_SKIP1)
        strcat(buff,"|SKIP1");
    else if((mode&AD_SKIP)==AD_SKIP2)
        strcat(buff,"|SKIP2");
    else if((mode&AD_SKIP)==AD_SKIP3)
        strcat(buff,"|SKIP3");

    if((mode&FILTER_TYPE)==FILTER_HAMMING)
        strcat(buff,"|HAMMING");
    else if((mode&FILTER_TYPE)==FILTER_BLACKMAN)
        strcat(buff,"|BLACKMAN");
    else if((mode&FILTER_TYPE)==FILTER_NONE)
        strcat(buff,"|NO_FILTER");

    if(buff[0]){
        buff[0]=' ';
    }
    printf("MODE    0x%-.8X  %s\n",mode,buff);
}

//-------------------------------------------------------------
// DDRController::show_filter() print filter parameters
//-------------------------------------------------------------
void DDRController::show_filter()
{
    double sw=sweep_width();
    if(!ddrActive4Exp){
        printf("----------------- DDR %d <inactive> ----------\n",chid);
        return;
    }
    printf("----------------- DDR %d <active> ------------\n",chid);
    show_exp_mode();
    printf("NT      %-8d\n",ns);
    printf("DL      %-8d\n",dl);
    printf("AL      %-8d\n",al);
    printf("STAGES  %-8d\n",stages);
    printf("SR      %-8g\n",sr);
    printf("NX1     %-8d  NY1 %d\n",nx1,ny1);
    if(stages>0){
        printf("SW1     %-8g  OS1 %-8g XS1 %g\n",sw1,os1,xs1);
        printf("N1      %-8d  M1  %-8d B1  %d\n",n1,m1,b1);
        if(stages>1){
            printf("SW2     %-8g  OS2 %-8g XS2 %g\n",sw2,os2,xs2);
            printf("N2      %-8d  M2  %-8d B2  %d\n",n2,m2,b2);
        }
    }
    printf("FIDS    %-9d BFRS   %d\n",nfids,bmax);
    printf("NF      %-9d NFMOD  %d\n",nf,nfmod);
    printf("SW      %-9g DW     %g u\n",sw,1000/sw);
    printf("AT      %-9g AQTM   %g m\n",1000*al/sw,aqtm*1000);
    printf("CR      %-9g MAX    %g\n",cor,maxcr);
    printf("TC      %-9g MAX    %g u\n",tc*1e6,stages>0?0.5*1e6/sw:0);
    printf("ACMS    %-9d MAX    %d\n",nacms,nmax);
    printf("ACQS    %-9d MAX    %d\n",nacqs,amax);
    printf("BKPTS   %-9d 1RST   %d  NBLKS %d\n",tp2,tp1,nblks);

 }

//=========================================================================
//  DDRController::initializeExpStates DDR experiment setup
//=========================================================================
//  ACODES set : INITDDR
//  processing:
//   1. obtain all required exp setup parameters from vnmr params data base
//   2. from sw, ovs etc. determine the type DDR filter needed (1-stage,2-stage)
//   3. generate DDR filter params (sr,sw1,ov1 etc.)
//   4. pack exp setup data into acode argument list
//   5. output Acodes ...
//   6. ddrFifo routes acode arguments to ddr_set_exp in DDR_Acq.c
//   7. ddr_set_exp unpacks arguments and sends exp init messages to C67
//
// caution: arguments must match up EXACTLY with those in
//          DDR_Acq.c ddr_set_exp
//=========================================================================
int DDRController::initializeExpStates(int setupflag)
{
    char cbuff[MAXSTR];
    int nt=0,bs=0;
    double tmp,sw;
    int args[256];
    int i = 0;
    int ticks = 0;
    double ca=1.0;
    long fidsize;

    np = 0;

    reset_all();

    sw = getval("sw");
    np = (int)getval("np");
    nt = (int)getval("nt");

    if (var_active("bs",CURRENT)==1) {
        bs = (int)getval("bs");
        if (bs > nt)
            bs = nt;
    }
    else
        bs = 0;

    if (var_active("nf",CURRENT)==1) {
        tmp = getval("nf");
        if (tmp < 1.0)
            nf = 1;
        else
            nf=(int)tmp;
    }

    if (var_active("nfmod",CURRENT)==1)
        nfmod = (int)getval("nfmod");
    if(nfmod<=0 || nfmod>nf)
        nfmod=nf;
    if (nf/nfmod != (double)nf/(double)nfmod)
        abort_message("nf not an integral multiple of nfmod");

    if (var_active("ddrsd",CURRENT)==1)
        sd = getvalnwarn("ddrsd");    // receiver group delay

    // numerical corrections applied after data accumulation

    cbuff[0]='y';
    getstr("dp",cbuff);    // data type flag

    if(cbuff[0]=='n')
        mode|=DATA_PACK;  //  data transfer format mode

    // Does the FID data exceed the DDR DRAM size of 64 MBytes ?.
    // note: DDR always stores data in float format

    fidsize = np * 4 * nfmod;

    if (fidsize > 67108864L) {// > 64 MB
       abort_message("FID Size %lu Bytes has exceeded DDR internal buffer limits by %lu bytes, abort.\n",
        	fidsize, fidsize - 67108864L );
    }

    if (var_active("ddrtc",CURRENT)==0)
         mode&=~MODE_ATAQ_DR; // disable end reflection if no prefid

    getexpars();

    cbuff[0]='n';
    getstrnwarn("interp",cbuff);    // interpolation flag

    if(cbuff[0]=='y')
        mode|=MODE_L1;

    // construct a filter
 	sr=5e6;
    if(var_active("ddrsr",CURRENT)==1){
        sr=getvalnwarn("ddrsr");
		if(sw>5e6)
			sr=10e6;
		else if(sw>2.5e6)
			sr=5e6;
		else if(sw>1.25e6)
			sr=2.5e6;
    }
    else{
	    if(sw>5e6)
	        sr=10e6;
	    else if(sw>2.5e6)
	        sr=5e6;
	    else if(sw>50000)
	        sr=2.5e6;
    }

    os1=os2=ANALOGPLUS;
    cor=BRICKWALL;
    if (var_active("ddrcr",CURRENT)==1)
        cor = getvalnwarn("ddrcr");

    m1=m2=1;

    if (var_active("ddrfw1",CURRENT)==1)
        fw1 = getvalnwarn("ddrfw1");
    if (var_active("ddrfw2",CURRENT)==1)
        fw2 = getvalnwarn("ddrfw2");

    int usepars=var_active("ddrstages",CURRENT);
    if (usepars==1){
        stages=(int)getvalnwarn("ddrstages");
        if(stages==1)
            sw=sr;
        if (var_active("ddro1",CURRENT)==1)
            os1 = getvalnwarn("ddro1");
        if (var_active("ddro2",CURRENT)==1)
            os2 = getvalnwarn("ddro2");
        if (var_active("ddrx1",CURRENT)==1)
            xs1 = getvalnwarn("ddrx1");
        if (var_active("ddrx2",CURRENT)==1)
            xs2 = getvalnwarn("ddrx2");
        if (var_active("ddrm1",CURRENT)==1)
            m1 = (int)getvalnwarn("ddrm1");
        if (var_active("ddrm2",CURRENT)==1)
            m2 = (int)getvalnwarn("ddrm2");
    }
    else{
        calc_stages(sr,sw,&stages,&m1,&m2);
    }

    sw=sr/m1/m2;
    // if ddrtc exists but is set to 'n' disable baseline correction
    if (var_active("ddrtc",CURRENT)==0){
        int N1=(int)(os1*sr/sw1+0.5);
        xs1=0.5;
        int XS1=(int)(0.5*N1-xs1);
        tc = -XS1/sr;
        if(debug)
            printf("Baseline correction disabled\n");
    }
    else if (var_active("ddrtc",CURRENT)==1){
        tc = getvalnwarn("ddrtc");
        xs1=(-tc*sr);
    }
    else /* ddrtc does not exist, calculate it */
    {
        tc = alfa;
        xs1=(-tc*sr);
    }

    // estimate max cor from sr, m1 and stages

    int usemaxcr=1;
    maxcr=calc_maxcr(sr, stages, m1);
    if (var_active("ddrmaxcr",CURRENT)==0){
        if(debug)
            printf("maxcr:%g <disabled>\n",maxcr);
        usemaxcr=0;
    }
    else if(debug)
        printf("maxcr:%g <enabled>\n",maxcr);

    if(usemaxcr && cor>maxcr)
        cor=maxcr;
    if(stages==0){
        sw1=sw;
        os1=os2=0;
    }
    else if(stages==2){
        sw1=sr/m1+0.5;
        sw2=sr/m1/m2+0.5;
        sw=sw2;
        os2=cor;
     }
     else if(stages==1){
        sw1=sr/m1+0.5;
        sw=sw1;
        os1=cor;
    }

    // calculate DDR parameters(dl,nacms,..) from vnmr parameters (nt,bs,..)

    ns=(int) (bs?bs:nt);

    dl = al = (int) (np / 2.0); // make all the same for the time being

    // evaluate expargs string

    // NB: always use aqtm='n' case now so that aqtm=at
#ifdef USE_AQTM
    int active=var_active("aqtm",CURRENT);
    if(active>=0){
        if(active==0){
            aqtm=getval("at");
            mode|=MODE_ATAQ;
        }
        else
            aqtm=acq_time();

        if (P_setreal(CURRENT,"aqtm",aqtm,1)<0)
            printf("DDRController<%d>::initializeExpStates error setting aqtm\n",chid);
    }
    else
        aqtm=acq_time();
#else
    aqtm=getval("at");
    mode|=MODE_ATAQ;
#endif
    nfids=(int)getval("arraydim"); // number of arrayed elements

    nacms=bmax=findMaxBufs(nt, bs, nfids, nf, nfmod);
    nmax = DDR_DATA_SIZE/dbytes/dl/nfmod;  // max C67 data buffers available

    nacms=nmax<nacms?nmax:nacms;

    calc_filter();

    if(bkpts==0){
        if(nx1>5000)
            bkpts=1000;
        //else if(nx1<1000)
        //    bkpts=250;
        else
            bkpts=500;
    }
    if(bkpts>=nx1){
        tp1=tp2=nx1;
        nblks=1;
    }
    else{
        int rem=nx1%bkpts;
        nblks=nx1/bkpts;
        tp2=bkpts+rem/nblks;
        rem=nx1%tp2;
        tp1=tp2+rem;
    }
    int smem=(mode & MODE_FIDSIM)?4*nx1:0;
    int acqsize=dl*2*(stages>0?4:2);

    nacqs=amax=(DDR_ACQ_SIZE-smem)/acqsize;

    if(nacqs>nfids*nt)
        nacqs=nfids*nt;

    sampleTicks=(long long)(TIMERCLOCKFREQ/sr+0.5);

    scanTicks=(long long)(aqtm*TIMERCLOCKFREQ); // total ticks in scan
    acqTicks=0;

    if(debug)
        show_filter();
    if(debug>1)
        printf("DDRController<%d>::initializeExpStates(%d)\n",chid,setupflag);

    if (var_active("rcvrf",GLOBAL)==1){
        if(P_getreal(GLOBAL, "rcvrf", &tmp, chid)==0){
            rcvrf[chid-1]=tmp;
            if(debug)
                printf("rcvrf[%d]=%g\n",chid,rcvrf[chid-1]);
        }
    }
    if (var_active("rcvra",GLOBAL)==1){
        if(P_getreal(GLOBAL, "rcvra", &tmp, chid)==0){
            ca=tmp;
            rcvra[chid-1]=tmp;
            if(debug)
                printf("rcvra[%d]=%g\n",chid,rcvra[chid-1]);
        }
    }
    if (var_active("rcvrp",GLOBAL)==1){
        if(P_getreal(GLOBAL, "rcvrp", &tmp, chid)==0){
           cp=tmp;
           rcvrp[chid-1]=tmp;
           if(debug)
                printf("rcvrp[%d]=%g\n",chid,rcvrp[chid-1]);
        }
    }
    if (var_active("rcvrp1",GLOBAL)==1){
        if(P_getreal(GLOBAL, "rcvrp1", &tmp, chid)==0){
            rcvrp1[chid-1]=tmp;
            if(debug)
                printf("rcvrp1[%d]=%g\n",chid,rcvrp1[chid-1]);
        }
    }

    if (var_active("ascale",CURRENT)==1)
        ascale = (int)getvalnwarn("ascale");

    args[i++] = 0;              // place holder for number of args

    int xwrd=(mode & DATA_PACK)?4:8;

    args[i++] = nfids;          // number of fids in experiment
    args[i++] = nacms;          // number of 405 data buffers
    args[i++] = xwrd*dl*nfmod;  // bytes per buffer (uplinker data)
    args[i++] = nf;             // nf
    args[i++] = nfmod;          // nfmod

    args[i++] = mode;
    args[i++] = tp1;            // number of points to process per interrupt (first)
    args[i++] = tp2;            // number of points to process per interrupt
    args[i++] = nacms;          // number of C67 data buffers
    args[i++] = dl;             // data length (default=np/2)
    args[i++] = al;             // acquisition length (default=np/2)
    args[i++] = ascale;         // amplitude scale factor 1/2^scale for dp='n'

    args[i++] = 8;              // acq queue size
    args[i++] = 200;            // scan queue size
    args[i++] = 200;            // data_ready message queue size

    packd(cp,args,i);           // constant phase correction
    packd(ca,args,i);           // constant amplitude correction
    packd(cf,args,i);           // constant freq correction
    packd(sd,args,i);           // FPGA acq. trigger group delay

    packd(sr*1e-6,args,i);      // sampling rate (MHz)
    packd(sw1,args,i);
    if(stages==0)
        packd(0.0,args,i);
    else
        packd(os1,args,i);
    packd(xs1,args,i);
    packd(fw1,args,i);
    packd(sw2,args,i);
    if(stages>1)
        packd(os2,args,i);
    else
        packd(0.0,args,i);
    packd(xs2,args,i);
    packd(fw2,args,i);
    if(mode&MODE_FIDSIM){
        args[i++] = simpeaks;
        packd(simdly*1e-6,args,i);
        packd(simfscale,args,i);
        packd(simnoise,args,i);
        packd(simlw,args,i);
        packd(simascale,args,i);
        if(debug>0){
             printf("simpeaks=%d\n",simpeaks);
             printf("simdly=%g\n",simdly);
             printf("simfscale=%g\n",simfscale);
             printf("simnoise=%g\n",simnoise);
             printf("simlw=%g\n",simlw);
             printf("simascale=%g\n",simascale);
        }
    }
    args[0] = i;               // set number of args

    outputDDRACode(INITDDR, i, args);
   // for some reason on pinsync, if IEN is low the NCO phase is 180 degrees
   // different in MODE_WIEN than when IEN is high (??)

    add2Stream(ALLGATES(DDR_IEN));
    setDDRDelay(LATCHDELAY);   /* Latch delay */
    ticks += LATCHDELAYTICKS;
    rcvr_is_on_now = 0;
    rcvroff_flag   = 1;

    clearSmallTicker();
    clearAcqTicker();
    acycles=1;
    return ticks;
}

//=========================================================================
//  DDRController::inactiveRcvrSetup
//  ACODES set : PINSYNC_DDR for parse ahead control side-effect
//=========================================================================
void DDRController::inactiveRcvrSetup()
{
  int args[2];
  args[0]=chid;
  outputACode(PINSYNC_DDR, 1, args);
}

//=========================================================================
//  DDRController::nextscanDDR  next scan setup
//  ACODES set : NEXTSCANDDR
//=========================================================================
int DDRController::nextscanDDR()
{
    int i = 0;
    int args[32];
    int expmode=0;
    long long ticks=calcTicks(PSON);
    extern int clr_at_blksize_mode;

    if (ddrActive4Exp == 0){
        args[0]=chid;
        outputACode(PINSYNC_DDR, 1, args);
        setDDRDelay(PSON);
        return 0;
    }

    if(acq_mode & WIEN)
        expmode=EXP_WIEN;
    else if(acq_mode & WACQ)
        expmode=EXP_WACQ;

    if(clr_at_blksize_mode)
        expmode|=EXP_CLR_DATA; //  force dst=src to clear acm at each bs

    args[i++] = 0;      		// place holder for number of args
    args[i++] = ix;   			// fidnum
    args[i++] = expmode;   	    // exp mode options
    args[i++] = DDR_PINS;   	// pin sync gates
    args[i++] = (int)(ticks&0x3FFFFFF);  // pin sync ticks
    packd(roff+rcvrf[chid-1],args,i);      // frequency offset: roff+rcvrf[chid-1]
    packd(pstep+rcvrp1[chid-1],args,i);    // rt phase step: rcvrstepsize+rcvp1[chid-1]
    args[0] = i;      			// number of args

    outputDDRACode(NEXTSCANDDR, i, args);

    if(debug>1){
        printf("DDRController<%d>::nextscanDDR mode=0x%0X pstep=%g rcvrp1=%g roff=%g rcvrf=%g\n",
                chid,expmode,pstep,rcvrp1[chid-1],roff,rcvrf[chid-1]);
    }
    noteDelay(calcTicks(PSON));

    return 0;
}

//=========================================================================
//  DDRController::setAcqVar set acquisition control variable
//  ACODES set : SETAVAR_DDR
//=========================================================================
void DDRController::setAcqVar(int a)
{
    int args[2];
    args[0]=a;
    if(debug>1)
        printf("DDRController<%d>::setAcqVar(%d)\n",chid,a);
    outputDDRACode(SETAVAR_DDR, 1, args);
}

//=========================================================================
//  DDRController::sendZeroFid send zeros instead of FID
//  ACODES set : SEND_ZERO_FID
//=========================================================================
void DDRController::sendZeroFid(int ix, int arrayDim)
{
    int args[3];
    args[0]=ix;
    args[1]= (ix == arrayDim) ? 1 : 0;
    if(debug>1)
        printf("DDRController<%d>::sendZeroFid(%d)\n",chid,ix);
    outputDDRACode(SEND_ZERO_FID, 2, args);
}

//=========================================================================
//  DDRController::setRcvrPhaseVar set real time phase var
//  ACODES set : SETPVAR_DDR
//=========================================================================
void DDRController::setRcvrPhaseVar(int p)
{
    int args[2];
    args[0]=p;
    if(debug>1)
        printf("DDRController<%d>::setRcvrPhaseVar(%d)\n",chid,p);
    outputDDRACode(SETPVAR_DDR, 1, args);
}

//=========================================================================
//  DDRController::setRcvrPhaseStep set real time phase step
//=========================================================================
void DDRController::setRcvrPhaseStep(double p)
{
    if(debug>1)
        printf("DDRController<%d>::setRcvrPhaseStep(%g)\n",chid,p);
    pstep=p;
}

//=========================================================================
//  DDRController::pushloop() called at start of hardware loop
//=========================================================================
long long DDRController::pushloop(int n)
{
    long long pticks;
    if(acq_started){
        pticks=acycles*getAcqTicker();
        if(debug>1)
            printf("DDRController<%d>::pushloop(%d) dt=%g us\n",
                chid,n,10e6*pticks/TIMERCLOCKFREQ);
        acqTicks+=pticks;
        clearAcqTicker();
    }
    acycles=n;
    return 0;
}

//=========================================================================
//  DDRController::poploop() called at end of hardware loop
//=========================================================================
long long DDRController::poploop(int n)
{
    long long ticks=0;
    long long pticks;
    if(acq_started){
        pticks=getAcqTicker();
        if(debug>1)
            printf("DDRController<%d>::poploop() dt=%g s\n",
                chid,ticks/TIMERCLOCKFREQ);
        if((acq_mode & (WIEN|WACQ))==0)
            acqTicks+=acycles*pticks;
        clearAcqTicker();
    }
    acycles=n;
    return ticks;
}

//=========================================================================
//  DDRController::setAcqGate set/clr DDR_ACQ gate
//  ACODES set : SETACQ_DDR (acq_mode & WACQ)
//=========================================================================
void DDRController::setAcqGate(int state)
{
    int args[2];
    args[0]=state;
    if(acq_mode & WACQ){
        if(state)
            setGates(DDR_ACQ);
        else
            clrGates(DDR_ACQ);
    }
    else if(!acq_on)
        outputDDRACode(SETACQ_DDR, 1, args);
    if(debug>1 && !acq_on)
         printf("DDRController<%d>::setAcqGate(%d)\n",chid,state);
}

//=========================================================================
//  DDRController::setReceiverGateOn set/clr DDR_RG
//=========================================================================
void DDRController::setReceiverGate(int state)
{
    if(debug>1)
         printf("DDRController<%d>::setReceiverGate(%d)\n",chid,state);
    if(state)
        setGates(DDR_RG);
    else
        clrGates(DDR_RG);
}

//=========================================================================
//  DDRController::setDataGate set/clr DDR_IEN
//=========================================================================
void DDRController::setDataGate(int state)
{
    if(debug>1)
         printf("DDRController<%d>::setDataGate(%d)\n",chid,state);
    if(state)
        setGates(DDR_IEN);
    else
        clrGates(DDR_IEN);
}

//=========================================================================
//  DDRController::setAcqMode set acquire options
//=========================================================================
void DDRController::setAcqMode(int m)
{
    if(debug>1)
         printf("DDRController<%d>::setAcqMode(0x%X)\n",chid,m);
    acq_mode=m;
}

//=========================================================================
//  DDRController::getAcqMode get acquire options
//=========================================================================
int DDRController::getAcqMode()
{
    return acq_mode;
}

//=========================================================================
//  DDRController::startAcquire(double sample) start explicit acquire
//=========================================================================
long long DDRController::startAcquire(double alfa)
{
    long long ticks = 0;
    double rgoff;
    double rgon=0;
    double rof3=2e-6;

    if (var_active("rof3",CURRENT)==1)
        rof3 = getvalnwarn("rof3");

    rgon=alfa-rof3;
    if(rgon<0)
        rgon=0;

    clearAcqTicker();
    //flushDload();
    acqTicks=0;

    rgoff=rof3;

    if(debug>1)
        printf("DDRController<%d>::startAcquire(%g)\n",chid,alfa);

    if(!acq_started){ // need a pinsync
        nextscanDDR();
        rgoff-=PSON;
        if(rgoff<0)
            rgoff=0;
       if(debug>2)
           printf("DDRController<%d>::startAcquire TG->RG = %g RG->ACQ = %g us\n",
            chid,rgoff*1e6,rgon*1e6);
        clrGates(DDR_PINS|DDR_IEN|DDR_ACQ);
    	if(dps_flag  && chid==1){
       	   DPSprint(alfa, "48 startacq 1 0 1 1 rgon %.9f rgoff %.9f 1 zero %d startacq %.9f\n",
       	       (float)(rgon), (float)(rgoff), 0, (float)(alfa));
    	}
    }

    if(rgoff>TMIN)
        setDDRDelay(rgoff);

    setGates(DDR_RG|DDR_IEN);

    if(rgon>TMIN)
        setDDRDelay(rgon);

    // set up gates

    ticks=getAcqTicker();
    clearAcqTicker();

    acq_started=1;
    return ticks;  // should be alfa ticks
}

long long DDRController::startAcquireTicksOnly(double alfa)
{
    long long ticks = 0;
    double rgoff;
    double rgon=0;
    double rof3=2e-6;

    if (var_active("rof3",CURRENT)==1)
        rof3 = getvalnwarn("rof3");

    rgon=alfa-rof3;
    if(rgon<0)
        rgon=0;

    rgoff=rof3;

    if(debug>1)
        printf("DDRController<%d>::startAcquireTicksOnly(%g)\n",chid,alfa);

    if(dps_flag  && chid==1){
       	   DPSprint(alfa, "48 startacq 1 0 1 1 rgon %.9f rgoff %.9f 1 zero %d startacq %.9f\n",
       	       (float)(rgon), (float)(rgoff), 0, (float)(alfa));
    }

    if(rgoff>TMIN)
        ticks += calcTicks(rgoff);


    if(rgon>TMIN)
        ticks += calcTicks(rgon);

    return ticks;  // should be alfa ticks
}

//=========================================================================
//  DDRController::Sample(tm) explicit acquire (alternate form)
//=========================================================================
long long DDRController::Sample(double atm)
{
    long long ticks = 0;  // what to return for all channels
    long long aticks = 0; // acqTicks to add for this call
    long long pticks = 0; // acqTicks to before this call

    pticks=getAcqTicker(); // get delays since startAcq or startloop


    if(debug>3 || (debug>1 && !acq_on))
        printf("DDRController<%d>::Sample(%g us)\n",chid,atm*1e6);
    if(!acq_started){
        double alfa=3e-6;
#ifdef CHECK_LOOPS  // try to second-guess how loops will play out in real time
        if(acycles>1){
            char msge[100];
            sprintf(msge,"startacq must be called before sample a hardloop\n");
            text_error(msge);
            psg_abort(1);
        }
#endif
        if (var_active("alfa",CURRENT)==1)
            alfa = getvalnwarn("alfa");
        ticks+=startAcquire(alfa);
    }

    if(dps_flag && chid==1){
        DPSprint(atm, "1 sample 1 0 2 2.0 %.9f sample %.9f \n", (float)(2.0), (float)(atm));
        return ticks;
    }

    aticks=calcTicks(atm);
    ticks+=aticks;

    if(!(acq_mode & NZ) || !acq_on)
        setGates(DDR_IEN);
    setAcqGate(1);
    if(acq_mode & RG)
        setGates(DDR_RG);

    if(atm>0)
        setDDRDelay(atm);
    if(!(acq_mode & NZ))
        clrGates(DDR_IEN);
    if(acq_mode & RG)
        clrGates(DDR_RG);
    if(acq_mode & WACQ)
        clrGates(DDR_ACQ);

    acqTicks+=acycles*aticks;
    if(acq_on)
        acqTicks+=acycles*pticks;
    else
        acqTicks+=(acycles-1)*pticks;
    acq_on=1;

    clearAcqTicker();
    return ticks;
}

//=========================================================================
//  DDRController::endAcquire() end explicit acquire
//=========================================================================
long long DDRController::endAcquire()
{
    long long ticks=0;
    long long aticks=0;
    acqTicks+= acycles*getAcqTicker();
    double tm=(scanTicks-acqTicks)/TIMERCLOCKFREQ;

    if(acqTicks-scanTicks){
        if(debug>1)
            printf("DDRController<%d>::endAcquire excess aqtm = %g us\n",
               chid,-1e6*tm);
    }
    else if(tm>0){ // need more time to finish
        if(debug>1)
            printf("DDRController<%d>::endAcquire extra time needed = %g us\n",
              chid,tm*1e6);
        clearAcqTicker();
        aticks=getAcqTicker();
        acqTicks+=aticks;
    }
    else {
        if(debug>1)
           printf("DDRController<%d>::endAcquire residual=0\n",chid);
    }
    acq_on=0;

    clrGates(DDR_ACQ|DDR_RG|DDR_IEN);

    // in windowed acq modes, continue sampling to allow filter to finish

    if(debug>2)
        printf("DDRController<%d>::endAcquire mode=0x%X\n",chid,mode);

    clearAcqTicker();
    reset_acq();
    acq_completed=1;
    return ticks;
}

//=========================================================================
//  DDRController::Acquire() implicit acquire (dfltacq in Bridge.cpp)
//=========================================================================
long long DDRController::Acquire()
{
    double alfa=6e-6;
    long long ticks = 0;

    if (acq_completed){
        if(debug>1)
            printf("DDRController<%d>::Acquire() <acquire-endacq exists in user sequence, exiting>\n",chid);
        reset_acq();
        return ticks;
    }

    if(acq_started) { // finish off explicit acquisition
        if(debug>1)
            printf("DDRController<%d>::Acquire() <acquire exists in user sequence, adding endacq>\n",chid);
        if(!acq_completed)
            ticks=endAcquire();
        reset_acq();
        return ticks;
    }

    if(debug>1)
        printf("DDRController<%d>::Acquire() <acquire not in user sequence, executing implicit acquire>\n",chid);

    clearAcqTicker();

    if (var_active("alfa",CURRENT)==1)
       alfa = getvalnwarn("alfa");

    ticks=startAcquire(alfa);

    setGates(DDR_RG|DDR_IEN);
    setAcqGate(1);

    setDDRDelay(aqtm);  // use exact aqtm calculated from filter parameters

    clrGates(DDR_RG|DDR_IEN|DDR_ACQ);
    acq_on=0;
    rcvr_is_on_now = 0;
    ticks += getAcqTicker();
    reset_acq();
    return ticks;
}

//=========================================================================
//  DDRController::getexpars  get ddr overrides from exparg string
//=========================================================================
static int get_value(char *s, float *f)
{
    int   d;
    if(strlen(s)==1){
        if(sscanf(s,"%d",&d)){
            *f=(float)d;
            return 1;
        }
    }
    return sscanf(s,"%f",f);
}
void DDRController::getexpars()
{
    char *args[64];
    char sbuff[MAXSTR];
    int i,nargs=0,ival;
    float argf;

    sbuff[0]=0;
    getstrnwarn("expargs",sbuff);    // get expargs string

    if(sbuff[0]==0)
        return;
    char *cptr=(char*)strtok(sbuff," \t");
    while(cptr){
        args[nargs++]=cptr;
        cptr=(char*)strtok(0," \t");
    }

    for (i = 0; i < nargs; i++) {
        if (args[i][0]!='-')
            continue;
        switch(args[i][1]){

        case 'B': // blocksize
             if(get_value(args[i+1],&argf)){
                bkpts=(int)argf;
                i++;
            }
            break;

        case 'D': // disable dithering
            mode|=MODE_DITHER_OFF;
            break;

        case 'f': // filter type
            switch(args[i][2]){
            case 'b': // Blackman filter
                mode|=FILTER_BLACKMAN;
                break;
            case 'h': // Hamming filter
                mode|=FILTER_HAMMING;
                break;
            case 'n': // no filter
                mode|=FILTER_NONE;
                break;
            case 'f': // use float coefficients
                mode|=MODE_CFLOAT;
                break;
            case 1:   // set tighter filter
                mode&=~AD_FILTER;
                mode|=AD_FILTER1;
                break;
            case 2:  // set default filter
                mode&=~AD_FILTER;
                mode|=AD_FILTER2;
                break;
            }
            break;

        case 'd': // double precision signal-averaging
            mode|=DATA_DOUBLE;
            dbytes=16;
            break;

        // debug and test mode options

        case 'p': // sawtooth fid data (ala Pete)
            mode|=MODE_ADC_DEBUG;
            break;

        case 'r': // turn off phase sync
            mode|=MODE_NOPS;
            break;

        case 'q': // turn on prefid phase averaging
            mode|=PF_QUAD;
            break;

        case 'c': // disable all checksum tests
            mode|=MODE_NOCS;
            break;

        case 'h': // use NCO constant for phase and freq
            mode|=MODE_HWPF;
            break;

        case 'x': // reflect end points if aqtm='y'
            mode|=MODE_ATAQ_DR;
            break;

        case 'w': // use calculation vs nco for freq shift
            mode|=MODE_FSHIFT;
            break;

        case 'o': // enable new adc ovf warning for each fid
            mode|=MODE_ADC_OVF;
            break;

        case 'm': // use programmed i/o vs. dma for hpi data xfers
            mode&=~MODE_DMA;
            break;

        case 'l': // set L1 L2
            ival=args[i][2]-'0';
            switch(ival){
            case 1:
                mode|=MODE_L1;
                break;
            case 2:
                mode|=MODE_L2;
                break;
            }
            break;

        case 'b': // turn on autoskip
            mode&=~AD_SKIP;
            ival=args[i][2]-'0';
            switch(ival){
            case 1:
                mode|=AD_SKIP1;
                break;
            case 2:
                mode|=AD_SKIP2;
                break;
            case 3:
                mode|=AD_SKIP3;
                break;
            }
            sd=0;
            break;

        case 'i':
            if(!ddrActive4Exp)
                break; // else fall through
        case 'I': // turn on debugging output
            ival=args[i][2]-'0';
            switch(ival){
            default:
               debug=1;
               break;
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                debug=ival+1;
                break;
            };
            break;

         case 's': // simulated fid
            mode|=MODE_FIDSIM|MODE_NOPS;
            ival=args[i][2]-'0';
            if(ival>=0 && ival <=9){
                simpeaks=ival;
                break;
            }
            switch(args[i][2]){
            case 'l': // line width
                if(get_value(args[i+1],&argf)){
                    simlw=argf;
                    i++;
                }
                break;
            case 'd': // receiver delay
                if(get_value(args[i+1],&argf)){
                    simdly=argf;
                    i++;
                }
                break;
            case 'f': // freq scale
                if(get_value(args[i+1],&argf)){
                    simfscale=argf;
                    i++;
                }
                break;
            case 'a': // ampl scale
                if(get_value(args[i+1],&argf)){
                    simascale=argf;
                    i++;
                }
                break;
            case 'n': // noise
                if(get_value(args[i+1],&argf)){
                    simnoise=argf;
                    i++;
                }
                break;
            }
            break;
       }
    }
}
//=========================================================================
// DDRController::set_RF_LO_source(int rfch)
//=========================================================================
void DDRController::set_RF_LO_source(int rfch)
{
    RF_LO_source = rfch;
}
//=========================================================================
// DDRController::get_RF_LO_source()
//=========================================================================
int DDRController::get_RF_LO_source()
{
    return RF_LO_source;
}
//=========================================================================
// DDRController::setDDRActive4Exp(int active)
//=========================================================================
void DDRController::setDDRActive4Exp(int active)
{
  ddrActive4Exp = active;
}
