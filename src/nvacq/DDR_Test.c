/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
//=========================================================================
// FILE: DDR_Test.c
// 
// AD6634 tests
//   print_ureg            print contents of a uport register
//   print_uport_io        print out contents of all uport IO registers
//   print_uport_chnl      print out contents of uport channel registers
//   utest                 print out contents of all uport registers
//
// HPI benchmark tests
//   test_hpi_read         hpi read speed test
//   test_hpi_write        hpi read write test 
//   test_hpi_xfer         hpi transfer write, read and validate test 
//
// DATA FIFO tests
//   adfclear              clear DATA FIFO and show registers
//   adfwrite              DATA FIFO write test
//   adfread               DATA FIFO read test
//   adftest               DATA FIFO read-write test
//   acqtest               measures DATA FIFO sampling rate
//
// Output FIFO tests
//   otftest              simple FIFO stuff and run test
//
// Other tests
//   datatest             test for uplinker
//   exptest              stand-alone NMR experiment simulation
//=========================================================================

#include "DDR_Acq.h"
#include "stdio.h"
#include "msgQLib.h"
#include "DDR_Globals.h"
#include "DDR_Common.h"
#include "logMsgLib.h"
#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#include "ddr_fifo.h"
#include "FFKEYS.h"
#include "dataObj.h"

#include <math.h>

#define TMIN 50e-9
#define FLDHIGH      OTF_SIZE-32
#define FLDLOW       200

#define ALLGATES(x)      encode_DDRSetGates(0,0xfff,(x))
#define SETGATES(x)      encode_DDRSetGates(0,(x),(x))
#define CLRGATES(x)      encode_DDRSetGates(0,(x),0)
#define MSKGATES(m,x)    encode_DDRSetGates(0,(m),(x))
#define HOLDGATES        encode_DDRSetGates(0,0,0)

//#define PRINT_STATS

void ddr_set_cycle(int on, int off, int zeros);

// variables used for experiment test
volatile static int tdly=0;

extern DATAOBJ_ID pTheDataObject;

extern int hpi_two_reads;
extern int hpi_two_writes;

extern int data_stats;

extern int ddrTask(char *name, void* task, int priority);
extern void getDDRData(int adrs, unsigned int pts, float *dest, int dtype);
extern void packd(double d, int *buffer, int *iptr);
extern void ddr_calc_stages(double sr, double sw, UINT *n, UINT *a, UINT *b);
extern double ddr_calc_maxcr(double sr, int stages, int m1);

//####################### AD6634 tests ####################################

//=========================================================================
// print_ureg: read and display the value of uport register
//=========================================================================
void print_ureg(int adrs, int wrds)
{
    int value=get_AD6634_reg(adrs,wrds);
    printf("  adrs 0x%-0.2X  wrds %-2d value 0x%0.6X\n",adrs,wrds,value);
}

//=========================================================================
// print_uport_io: print out the values of all io uport registers
//=========================================================================
void print_uport_io()
{
    printf("------ input/output ------------------\n");
    //WMICRO(ACR,0x00);         // not auto-inc + not broadcast
    WMICRO(SLEEP,0x2F);         // access to I/O Ctrl Regc + sleep
    print_ureg(0x00,2); 
    print_ureg(0x01,2); 
    print_ureg(0x02,3); 
    print_ureg(0x03,1); 
    print_ureg(0x04,2); 
    print_ureg(0x05,2); 
    print_ureg(0x06,3); 
    print_ureg(0x07,1); 
    print_ureg(0x08,1); 
    print_ureg(0x09,1); 
    print_ureg(0x0A,1); 
    print_ureg(0x0B,2); 
    print_ureg(0x0C,1); 
    print_ureg(0x0D,2); 
    print_ureg(0x0E,1); 
    print_ureg(0x0F,1); 
    print_ureg(0x10,1); 
    print_ureg(0x11,2); 
    print_ureg(0x12,1); 
    print_ureg(0x13,2); 
    print_ureg(0x14,1); 
    print_ureg(0x15,2); 
    print_ureg(0x16,1); 
    print_ureg(0x17,1); 
    print_ureg(0x18,1); 
    print_ureg(0x19,2); 
    print_ureg(0x1A,1); 
    print_ureg(0x1B,1); 
    print_ureg(0x1C,1); 
    print_ureg(0x1D,1); 
    print_ureg(0x1E,1); 
    WMICRO(SLEEP,0x0F);         // select channel memory map    
}

//=========================================================================
// print_uport_chnl: print out values for uport channel registers
//=========================================================================
void print_uport_chnl(int ch)
{
    printf("------ channel %d ------------------\n",ch);

    select_chnl(ch);

    print_ureg(0x80,1); 
    print_ureg(0x81,1); 
    print_ureg(0x82,1); 
    print_ureg(0x83,2); 
    print_ureg(0x84,2); 
    print_ureg(0x85,2); 
    print_ureg(0x86,2); 
    print_ureg(0x87,2); 
    print_ureg(0x88,2); 
    print_ureg(0x90,2); 
    print_ureg(0x91,2); 
    print_ureg(0x92,2); 
    print_ureg(0x93,1); 
    print_ureg(0x94,1); 
    print_ureg(0x95,1); 
    print_ureg(0x96,1); 
    print_ureg(0xA0,1); 
    print_ureg(0xA1,1); 
    print_ureg(0xA2,2); 
    print_ureg(0xA3,1); 
    print_ureg(0xA4,2); 
    print_ureg(0xA5,2); 
    print_ureg(0xA6,2); 
    print_ureg(0xA7,3); 
    print_ureg(0xA8,1); 
    print_ureg(0xA9,2); 
}

//=========================================================================
// utest: microport test (show all uport registers)
//=========================================================================
void utest()
{
    int i;
    print_uport_io();
    for(i=0;i<4;i++){
        print_uport_chnl(i);
    }
}

//####################### HPI benchmark tests #############################

//=========================================================================
// test_hpi_read: hpi read speed test
//=========================================================================
void test_hpi_read(unsigned int adrs, int number, int autoi, int base)
{
    int i;
    volatile int dest;
    
    long long m1;
    long long m2;
    double dt;
    const char *autos=autoi?"ON":"OFF";
    
    int *src=(int*)setHPIBase(base, adrs, autoi);
    
    if(!src){
         printf("invalid HPI base index %d\n",base);
         return;
    } 
    
    markTime(&m1);     
    for (i=0; i<number; i++){
        dest += src[i];
    }   
    markTime(&m2);
    dt=diffTime(m2,m1);
    printf("HPI READ TEST %d values %3.1f ms (%2.2f MB/s) autoi: %s\n",
         number,dt/1000,number*4/dt,autos);
         
    resetHPIBase(base);
}

//=========================================================================
// test_hpi_write: hpi write speed test
//=========================================================================
void test_hpi_write(unsigned int adrs, int number, int autoi, int base)
{
    int i;
    
    long long m1;
    long long m2;
    double dt;
    const char *autos=autoi?"ON":"OFF";
    int *iptr=(int*)setHPIBase(base, adrs, autoi);
        
    markTime(&m1);     
    for (i=0; i<number*4; i++){
        iptr[i] = i;
    }   
    markTime(&m2);
    dt=diffTime(m2,m1);
    printf("HPI WRITE TEST %d values %3.1f ms (%2.2f MB/s) autoi: %s\n",
         number,dt/1000,number*4/dt,autos);
    resetHPIBase(base);
}

//=========================================================================
// test_hpi_xfer: hpi memory write,read, verify test 
//=========================================================================
void test_hpi_xfer(unsigned int adrs, int number, int autoi, int base)
{
    int i;
    u32 a1;
    u32 a2;     
    float mult=0.51;
    float f;
    float *fptr=0;
    if(autoi & 0x2)
        fptr=(float*)setHPIBase(base, adrs, 1);
    else
        fptr=(float*)setHPIBase(base, adrs, 0);
        
    for (i=0; i<number; i++){   
        fptr[i] = i*mult;
    }

    if(autoi & 0x1)
        fptr=(float*)setHPIBase(base, adrs, 1);
    else
        fptr=(float*)setHPIBase(base, adrs, 0);

    for (i=0; i<number; i++){
        f=fptr[i];
        if (f != i*mult){
            a1.f=f;
            a2.f=i*mult;
            printf("MEM[0x%0.8X]: expected 0x%0.8X read 0x%0.8X\n",
                     adrs+i*4,a2.i,a1.i);
        }
    }
    resetHPIBase(base);
}

//=========================================================================
// bramtest: fpga bram memory write,read, verify test 
//=========================================================================
void bramtest(int number,unsigned int adrs)
{
    int i,j;
    u32 a1;
    u32 a2;     
    float mult=2;
    float f;
    int errs=0;
    int max_retries=100;
    int waits=0;
    volatile float *fptr=(float*)(4*adrs+0x70002000);
    
    if((adrs+number)>2000)
        number=2000-adrs;
         
    for (i=0; i<number; i++){   
        fptr[i] = i;
    }
    ddr_test(DDR_TEST_BRAM,number,adrs);
    
    waits=ddr_wait_test(0);

    for (i=0; i<number; i++){
        f=fptr[i];
        if (f != i*mult){
            errs++;
            a1.f=f;
            a2.f=i*mult;
            f=fptr[i];
            printf("MEM[0x%0.8X]: expected 0x%0.8X read 0x%0.8X\n",
                     fptr+i*4,a2.f,a2.i,a1.f,a1.i);
            for(j=1;j<max_retries;j++){
                f=fptr[i];
                if (f == i*mult)
                    break;
            }
            if (j == max_retries)
                printf("read failed after %d retries\n",j);
            else
                printf("read passed after %d retries\n",j);
        }
    }
    printf("BRAM TEST adrs 0x%0.8X values %d errors %d waits %d\n", 
         fptr, number,errs,waits-1);
}

//####################### DATA FIFO tests #############################

//=========================================================================
// adfclear() clear DATA FIFO  
//=========================================================================
void adfclear()
{
    int pts1,pts2;
    pts1=ADF_CNT;
    clearADCFIFO();     // toggle adc_fifo_clear 0->1
    pts2=ADF_CNT;
    printf("fifoclear  start:%d end:%d pts cleared:%d\n",pts1,pts2,pts1-pts2);
}

//=========================================================================
// adfread() DATA FIFO read test  
//=========================================================================
void adfread(int rpts)
{
    int pts1,pts2;
    pts1=getADFread();
    ddr_test(DDR_TEST_FIFO,rpts,0);
    taskDelay(calcSysClkTicks(166));  /* taskDelay(10); */
    pts2=getADFread();
    printf("fiforead   start:%d end:%d pts unread:%d\n",pts1,pts2,getADFcount());
}

//=========================================================================
// adfwrite() DATA FIFO fill test  
//=========================================================================
void adfwrite(int dly)
{
    int i;
    int pts1,pts2;

    setCollectMode(0);     // set manual mode (bits 1,2=1 in ADCArm)
    setADFread(0);
    clearADCFIFO();     // toggle adc_fifo_clear 0->1
    pts1=ADF_CNT;
    enableADCFIFO();    // set adc_fifo_clear = 0
    setCollect(1);      // enable data collection (adc_collect_data=1)
    softStartAD6634();
    
    for(i=0;i<dly*5;i++) // delay to collect some points into the fifo
        tdly+=i;
        
    setCollect(0);      // stop data collection (adc_collect_data=0)
    stopAD6634();       // turn off AD6634 data stream
    pts2=ADF_CNT;
    printf("fifowrite  start:%d end:%d pts written:%d\n",pts1,pts2,pts2-pts1);
}

//=========================================================================
// adftest() DATA FIFO read-write test  
//=========================================================================
void adftest(int rpts,int show)
{
    int ptm=800;       // pin-sync high 1 us
    int pdly=1600;     // pin-sync low to acq start 2 us
    int aqtm=16000;    // number of clocks 2000 pts at 5 MHz
    unsigned int *data;
    int i,errs=0,pts,same,dcnt;
    int wpts=0,sum=0;
    siu sval;
    
    if(rpts==0)
        rpts=8000;
    pts=rpts/2;
    rpts=2*pts;
   
    if(rpts>8000)
       rpts=8000; 
    if(rpts)
       aqtm=rpts*16;    

    initAD6634();
    disableMSGInts();
    stopAD6634();
    
    // load up the OTF with a sequence that will cause exactly "rpts"
    // to be written into the data FIFO
    
    resetOTF();
    clearCumDur();

    OTF=ALLGATES(DDR_IEN|DDR_PINS);
    OTF=TIME(ptm);
    OTF=ALLGATES(DDR_IEN);
    OTF=TIME(pdly);
    OTF=ALLGATES(DDR_IEN|DDR_ACQ);
    OTF=TIME(aqtm);
    OTF=ALLGATES(DDR_IEN);
    OTF=TIME(pdly);
    OTF=ALLGATES(0);
    OTF=TIME(0);   // autostop flag
    
    ADF_RPTR=0;
    clearADCFIFO();
    setCollectMode(ADC_START|ADC_STOP); 
    setADCDebug(1);
    enableADCFIFO();    
    pinStartAD6634();    
    enableMSGInts();
    softStartOTF();   // start the timing sequence    
    taskDelay(calcSysClkTicks(166));  // taskDelay(10) wait long enough for the sequence to finish    
    stopAD6634();     // OTF will stop automatically
    
    // read how many points were actually written into the data FIFO
    wpts=ADF_CNT; 
    
    // read the actual number of clock ticks that elapsed for the test  
    dcnt=DUR1-(2*pdly+ptm); //  data collection time (clks)
    
    printf("measured acqtm clks:%d pts:%d\n",dcnt,dcnt/16);

    ddr_test(DDR_TEST_FIFO,rpts,0);  // C67 moves data from FIFO to SDRAM
    
    taskDelay(calcSysClkTicks(166));  // wait long enough for data FIFO read to complete
    
    data=(unsigned int*)malloc(rpts*4); 
    if(!data){
        printf("fifotest Error, unable to malloc memory\n");
        return;
    }
    
    // read the data over the HPI
    
    getDDRData(0x90000000,pts,(float*)data,DATA_FLOAT);
    
    for (i=0; i<rpts; i++){
        sval.i=data[i];
        sum+=sval.i;

        same=(sval.s[0] == sval.s[1]);
        if(show)
             printf("%-5d %c 0x%0.8X\n",i,same?' ':'*', sval.i);
        else if(!same)
             printf("error addr:0x%0.8X  0x%0.8X\n",i, sval.i);
        if(!same)
             errs++;
    }
    if(errs)
        printf("%d errors in %d points\n",errs,wpts);
    else
        printf("no errors in %d points\n",wpts);
        
    if(sum!=test_value1)
        printf("checksum error: 0x%0.8X:0x%0.8X  last:0x%0.8X\n",
            sum,test_value1,test_value2);
    else
        printf("checksum test passed\n"); 
        
    free(data);       
}

//=========================================================================
// acqtest: adc data fifo test (pin-sync)
// - uses fifo gates acquisition mode
// - measures DATA FIFO sampling rate (default is 5 MHz)
// - to test 10 MHz sampling first run "set10M"
//=========================================================================
void acqtest(int m, int c, int t)
{
    int cnt=0;
    double f;
    int aqtm=40000;    // 2.5 ms (exact) 2500 pts at 5 MHz
    int ptm=800;       // pin-sync high 1us
    int pdly=112;      // pin-sync low to acq start 1.4 us
    int td=160;        // cycle delay 2 us
    int dcnt;
    int i=0;
    int cycles=1;
    int tp=5;
    int tm1=0;
    int tm2=0;
        
    if(m)
        aqtm=m;
    if(t)
        tp=t;
    if(c)
        cycles=c;
       
    dcnt=cycles*(aqtm+ptm+pdly+td)+td; 
    
    C6I5_ENBL=0;    
    C6I7_ENBL=0;    
   
    stopAD6634();
    resetOTF();

    OTF=ALLGATES(DDR_IEN);
    OTF=TIME(td);

    for(i=0;i<cycles;i++){             
        OTF=ALLGATES(DDR_IEN|DDR_PINS|DDR_SCAN);
        OTF=TIME(ptm);
        OTF=ALLGATES(DDR_IEN); 
        OTF=TIME(pdly);
        OTF=ALLGATES(DDR_IEN|DDR_ACQ);
        OTF=TIME(aqtm);
        OTF=ALLGATES(0); 
        OTF=TIME(td);
    } 
    OTF=ALLGATES(0);
    OTF=TIME(0);

    setADFread(0);
    clearADCFIFO();

    setCollectMode(ADC_START|ADC_STOP); 
    setTripPoint(tp);
    
    enableADCFIFO();

    C6I5_CLR=0;
    C6I5_CLR=BLKRDY_INT;        
    C6I5_ENBL=BLKRDY_INT;
   
    pinStartAD6634();
    
    // if there are points in the ADF fifo before start-up print 
    // an error message 
    cnt=getADFcount();
    if(cnt)
       printf("ERROR %d\n",cnt); // but this never gets called

    printOTFStatus();
       
    softStartOTF();   // start the timing sequence

    // wait for duration count to reach expected end value
    // or no change
    
    tm1=DUR1;
    tm2=tm1;
    while(tm1<dcnt) {
        tm1=DUR1;
        if(tm1==tm2) // no change
            break;
        tm2=tm1;
    }
    
    // test is done - clean up
    
    stopAD6634();
    ADF_ARM=0;
    C6I5_ENBL=0;
            
    cnt=getADFcount();
    clearADCFIFO();
    
    f=80.0*cnt/aqtm/cycles;
    printf("OTF clks    %d:%d\n",dcnt,tm1);
    printf("ADF COUNT   %d:0x%-8X\n",cnt,cnt);
    printf("freq        %g:%g MHz period %g ns\n",getSamplingRate(),f,1000.0/f);
}

//####################### Output FIFO tests ###############################
//=========================================================================
// otftest: simple FIFO stuff and run test 
//=========================================================================
void otftest(int d1, int d2, int cnt)
{
    int i,t=0,icnt,dcnt;
    
    if(d1==0)
       d1=800; // 10 us
    if(d2==0)
       d2=1600; // 10 us
    if(cnt==0)
       cnt=1;

    resetOTF();
    clearCumDur();
    for(i=0;i<cnt;i++){
        OTF=ALLGATES(DDR_IEN);
        OTF=TIME(d1);
        t+=RD_TIME;
        OTF=ALLGATES(0);
        OTF=TIME(d2);       
        t+=RD_TIME;
    }
    icnt=OTF_ICNT;
    dcnt=OTF_DCNT;

    OTF=TIME(0);
    printOTFStatus();
    softStartOTF();
    taskDelay(calcSysClkTicks(166));
    printf("expected:%d measured:%d icnt:%d dcnt:%d\n",t,DUR1,icnt,dcnt);
}

//####################### data tests ###############################

//=========================================================================
// datatest() test data uplinker  
//=========================================================================
void datatest(int np,int acm)
{ 
    float *adrs=(float*)DDR_DATA+np*2*acm;

    ddr_init();  // enable C67 message passing
    if(np==0)
        np=1000;
    ddr_set_dims(1, np, 0);
    ddr_test(DDR_TEST_DATA,np,acm);
    taskDelay(calcSysClkTicks(332));  // wait long enough to finish
    
    host_data(acm, 1, test_value2,ACM_LAST);
}

//####################### experiment simulation ##########################

enum {
    ACQ_ZIEN         = 0x01,  // cycled acq. with zeros
    ACQ_WACQ         = 0x02,  // cycled acq. with packed windows
    ACQ_WIEN         = 0x03,  // cycled acq. gate input 
    ACQ_TYPE         = 0x0f,  // acq test mask
    ACQ_RAMP         = 0x10,  // cycled acq. ramped gate input %
    ACQ_CYCLE        = 0x20   // cycled acq. 
};

#define SCAN_TSK_NAME "tScanTSK"

#define C2U(x) (double)(1e6*(0x07fffff&(x))/DDR_TMR_FREQ)
#define C2M(x) (double)(1e3*(x)/DDR_TMR_FREQ)
#define C2S(x) (double)((x)/DDR_TMR_FREQ)
#define S2C(x) (long long)((x)*DDR_TMR_FREQ)
#define S2T(x) (0x84000000|(long)((x)*DDR_TMR_FREQ+0.5)) // faster
#define C2T(x) (0x84000000|(x))
#define T2C(x) (0x07fffff&(x))
static double SA=5.0;       // sampling rate (MHz)
static double SR=200e-9;    // sampling rate (MHz)
static double TD=0.0;       // sampling delay

static double SW1=0;        // stage 1 spectral width (after decimation)
static int M1=0;            // stage 1 decimation factor 
static double OS1=8;        // stage 1 oversampling factor
static double XS1=0.0;      // stage 1 bc shift
static double FW1=0.0;      // stage 1 filter width factor
static double SW2=0;        // stage 2 spectral width
static int M2=0;            // stage 2 decimation factor 
static double OS2=50;       // stage 2 oversampling factor
static double XS2=0.0;      // stage 2 bc shift
static double FW2=0.0;      // stage 2 filter width factor
static int NP=1024;         // number of points in FID
static int NT=1;            // number of transients to average
static int NC=4;            // max number abph cycles
static int NACMS=1;         // number of accumulation buffers
static int NF=1;            // number of fids
static int NW=1;            // number of freqs
static int NACQS=0;         // number of acquisition buffers
static int ND=0;            // dummy scans
static int NX=0;            // extra clock cycles in scan
static int SKIPS=0;         // skip acq. triggers
static int max_scan_frames=32;   // max scan frames (wait)
static int min_scan_frames=16;   // min scan frames (fill)
static int NB=0;            // blocksize test
static int BKPTS=1000;      // block points
static int MODE=0;          // mode
static int PFAVES=0;        // prefid averages
static int ACYCS=0;         // chop cycles
static double TR=2e-3;      // repetition delay
static double TP=1e-6;      // pin-sync high period
static double TX=4e-6;      // pin-sync low period
//static double TI=500e-6;     // pre-start initial delay
static double TI=10e-3;     // pre-start initial delay

static double TA=0;         // sampling period
static double TC=0;         // acquisition cycle time
static double TS=0;         // AD6634 IEN window left shift

static double SW=1000;
static double AQTM=20;
static double WF=0;
static double WP=0;
static int xclks=0;

static double dflt_abph[4]={0,180,90,270}; // simulated phase cycle
static double dflt_abrf[4]={200,1000,4000,10000}; // simulated freq cycle

static double abph[4];
static double abrf[4];

static int aqrf=0;
static int aqph=0;
static int push_count=0;    // pushed scans (parser)
static int acq_count=0;     // popped scans (acquire)
static int total_scans=0;   // total scans in run
static int dummy_scans=0;   // dummy scans in run
static int ct=0;            // scan count in buffer
static int src=0;
static int dst=0;
static int tclks=0;
static int scan_info=0;
static int simpeaks=0;
static int acq_test=0;
static int exact_aqtm=0;
static int ssync=1;

static void parse();

//=========================================================================
// getexpars() read experiment parameters from a file  
//=========================================================================
static int get_value(char *s, double *d)
{
    double digits=1000;
    int   i; 
    float f;
    double value;   
    if(strlen(s)==1){
        if(sscanf(s,"%d",&i)){
            *d=(double)i;
            return 1;
        }   
    }
    if(!sscanf(s,"%f",&f))
        return 0;
    value=(double)f;
    i=(int)(value*digits+0.5);
    *d=(double)(i/digits);
    return 1;
}

void getexpars()
{
    char *args[64];
    char sbuff[512];
    int i,nargs=0,ival;
    double argf;
    int info=0;
    FILE *fp=0;
    char *fn="expargs"; 

    // read experiment setup file   

    fp = fopen(fn, "rb"); 
    if(fp==NULL){
        printf("could not open exp file %s\n",fn);
    }
    else {
        sbuff[0]=0;
        while(fgets(sbuff,255,fp)){         
            char *cptr=(char*)strtok(sbuff," \t\r\n");
            if(cptr[0]=='#')
                continue;
            while(cptr){
                args[nargs++]=cptr;
                cptr=(char*)strtok(0," \t\r\n\0");
            }
            break;
        }
        fclose(fp);
        fp=0;
        
        for (i = 0; i < nargs; i++) {
          if (args[i][0]=='-'){
            switch(args[i][1]){

            case 'c':  // extra tr clocks
                tclks=args[i][2]-'0';
                break;

            case 'd': //data options
                switch(args[i][2]){
                case 'd': // double-precision signal averaging
                     BIT_MSK(MODE,DATA_TYPE,DATA_DOUBLE);
                     break;
                case 'x': // packed integer return type
                     BIT_ON(MODE,DATA_PACK);
                     break;
                case 'i': // print out data statistics
                     data_stats=1;
                     break;
               }
                break;

            case 'e':  // misc. 
                exact_aqtm=0;
                break;

            case 'i': // print out info
                scan_info=1;
                break;

            case 'j':  // misc. 
                hpi_two_reads=0;
                break;
                                                
            case 'p': // debug fid data
                BIT_ON(MODE,MODE_ADC_DEBUG);
                break;

            case 'l': // first point zero time interpolation
                MODE|=PF_QUAD;
                break;

            case 'r':  // disable clock sync
                BIT_ON(MODE,MODE_NOPS);
                break;

            case 'x':  // disable super-sync timing
                ssync=0;
                break;

            case 'y':  // disable DMA 
                BIT_OFF(MODE,MODE_DMA);           
                break;

            case 'z':  // disable checksum test 
                BIT_ON(MODE,MODE_NOCS);           
                break;

            case 'D': // disable dithering
                BIT_ON(MODE,MODE_DITHER_OFF);
                break;
                
            case 'M':  // M
                if(get_value(args[i+1],&argf)){
                    if(sizeof(args[i])>2 && args[i][2]=='2')
                        M2=argf;
                    else
                        M1=argf;
                    i++;
                }
                break;

            case 'W':  // SW
                if(get_value(args[i+1],&argf)){
                    if(sizeof(args[i])>2 && args[i][2]=='2')
                        SW2=argf;
                    else
                        SW1=argf;
                    i++;
                }
                break;

            case 'F':  // FW
                if(get_value(args[i+1],&argf)){
                    if(sizeof(args[i])>2 && args[i][2]=='2')
                        FW2=argf;
                    else
                        FW1=argf;
                    i++;
                }
                break;
                    
            case 'O':  // OS1
                if(get_value(args[i+1],&argf)){
                    if(sizeof(args[i])>2 && args[i][2]=='2')
                        OS2=argf;
                    else
                        OS1=argf;
                    i++;
                }
                break;  
                
            case 'X':  // SHIFT
                if(get_value(args[i+1],&argf)){
                    if(sizeof(args[i])>2 && args[i][2]=='2')
                        XS2=argf;
                    else
                        XS1=argf;
                    i++;
                }
                break;

            case 'B': // blocksize
                 if(get_value(args[i+1],&argf)){
                    BKPTS=(int)argf;
                    i++;
                }
                break;
                
            case 'S':  // Sampling rate
                if(get_value(args[i+1],&argf)){
                    SA=argf;
                    i++;
                }
                break;  

            case 'n':  // np
                switch(args[i][2]){
				case 'z': // nz prefid averages (0=auto, 1=disable)
                    if(get_value(args[i+1],&argf)){
                        PFAVES=(int)argf;
                        i++;
                    }
                    break;
                case 'p': // np
                    if(get_value(args[i+1],&argf)){
                        NP=(int)argf;
                        i++;
                    }
                    break;
				case 't':  // nt
                    if(get_value(args[i+1],&argf)){
                        NT=(int)argf;
                        i++;
                    }
                    break;
                case 'a':  // na (NACMS)
                    if(get_value(args[i+1],&argf)){
                        NACMS=(int)argf;
                        i++;
                    }
                    break;
                case 'w':  // nw (number of arrayed ddrfreqs)
                    if(get_value(args[i+1],&argf)){
                        NW=(int)argf;
                        i++;
                    }
                    break;
                case 'f':  // nf (number of fids - arraydim)
                    if(get_value(args[i+1],&argf)){
                        NF=(int)argf;
                        i++;
                    }
                    break;
                case 'b':  // blocksize (bs)
                    if(get_value(args[i+1],&argf)){
                        NB=(int)argf;
                        i++;
                    }
                    break;                    
                case 'q':  // number of acquisition buffers
                    if(get_value(args[i+1],&argf)){
                        NACQS=(int)argf;
                        i++;
                    }
                    break;                    
                case 'c':  // nc (abph cycles)
                    if(get_value(args[i+1],&argf)){
                        NC=(int)argf;
                        i++;
                    }
                    break;
                case 'd':  // nd (dummy scans)
                    if(get_value(args[i+1],&argf)){
                        ND=(int)argf;
                        i++;
                    }
                    break;
                case 's':  // ns (max scans in queue)
                    if(get_value(args[i+1],&argf)){
                        max_scan_frames=(int)argf;
                        i++;
                    }
                    break;
                case 'g':  // ng(min scans in queue)
                    if(get_value(args[i+1],&argf)){
                        min_scan_frames=(int)argf;
                        i++;
                    }
                    break;
                 case 'x':  // nx (max c67 scans in queue)
                    if(get_value(args[i+1],&argf)){
                        NX=(int)argf;
                        i++;
                    }
                    break;                    
                }
                break;

            case 'q': // background phase
                switch(args[i][2]){
                case '0':
                case '1':
                case '2':
                case '3':
                	ival=args[i][2]-'0';
                    if(get_value(args[i+1],&argf)){
                        abph[ival]=(double)argf;
                    	i++;
                    }
                	break;
                }
                break;
                
            case 'w': // ddr frequency/phase
                switch(args[i][2]){
                case '0': // fid array freq.
                case '1': // fid array freq.
                case '2': // fid array freq.
                case '3': // fid array freq.
                	ival=args[i][2]-'0';
                    if(get_value(args[i+1],&argf)){
                        abrf[ival]=(double)argf;
                    	i++;
                    }
                	break;
                case 'f': // post-acc constant freq shift
                    if(get_value(args[i+1],&argf)){
                        WF=argf;
                        i++;
                    }
                    break;
                case 'p': // post-acc constant phase
                    if(get_value(args[i+1],&argf)){
                        WP=argf;
                        i++;
                    }
                    break;
                }
                break;
                
            case 't': // experiment times
                switch(args[i][2]){
                case 'r': // tr (ms)
                    if(get_value(args[i+1],&argf)){
                        TR=argf*1e-3;
                        i++;
                    }
                    break;
                case 'i': // ti (ms)
                    if(get_value(args[i+1],&argf)){
                        TI=argf*1e-3;
                        i++;
                    }
                    break;
                case 'x': // tx (us)
                    if(get_value(args[i+1],&argf)){
                        TX=argf*1e-6;
                        i++;
                    }
                    break;
                case 'p': // tp (us)
                    if(get_value(args[i+1],&argf)){
                        TP=argf*1e-6;
                        i++;
                    }
                    break;
                case 'a': // ta (us)
                    if(get_value(args[i+1],&argf)){
                        TA=argf*1e-6;
                        i++;
                    }
                    break;
                case 'c': // tf (us)
                    if(get_value(args[i+1],&argf)){
                        TC=argf*1e-6;
                        i++;
                    }
                    break;
                case 's': // ts (us)
                    if(get_value(args[i+1],&argf)){
                        TS=argf*1e-6;
                        i++;
                    }
                    break;
                case 'd': // td (us)
                    if(get_value(args[i+1],&argf)){
                        TD=argf*1e-6;
                        i++;
                    }
                    break;
                }
                break;

            case 'f': // filter type
                switch(args[i][2]){
                case 'b': // Blackman filter
                    MODE|=FILTER_BLACKMAN;
                    break;
                case 'h': // Hamming filter
                    MODE|=FILTER_HAMMING;
                    break;
                case 'n': // no filter
                    MODE|=FILTER_NONE;
                    break;
                case 'X': // blackman - harris..
                    MODE|=FILTER_BLCKH;
                    break;
                case 'f': // use float coefficients
                    MODE|=MODE_CFLOAT;
                    break;
                }
                break;

            case 'v':  // acquisition cycle test
                ival=args[i][2]-'0'; 
                switch(ival){
                case 1:
                    acq_test=ACQ_ZIEN|ACQ_CYCLE;
                    break;
                case 2:
                    acq_test=ACQ_WACQ|ACQ_CYCLE;
                    break;
                case 3:
                    acq_test=ACQ_WIEN|ACQ_CYCLE;              
                    break;
                } 
                if(args[i][3]=='r')
                    acq_test|=ACQ_RAMP;        
                break;

            case 'a':  // filter option 
                ival=args[i][2]-'0'; 
                BIT_OFF(MODE,AD_FILTER);
                switch(ival){
                case 1:
                    BIT_ON(MODE,AD_FILTER1);
                    break;
                case 2:
                    BIT_ON(MODE,AD_FILTER2);
                    break;
                }
                break;

        	case 'b': // turn on autoskip
                BIT_OFF(MODE,AD_SKIP);
            	ival=args[i][2]-'0'; 
                switch(ival){
                case 1:
                    BIT_ON(MODE,AD_SKIP1);
                    break;
                case 2:
                    BIT_ON(MODE,AD_SKIP2);
                    break;
                 case 3:
                    BIT_ON(MODE,AD_SKIP3);
                    break;
                }
            	break;

            case 'P': // simulated fid peak freq.
                ival=args[i][2]-'0';
                if(ival>=0 && ival <=9){
                    if(get_value(args[i+1],&argf))
                        ddr_peak_freq(ival,(double)argf);
                    i++;
                
                }
                break;

            case 'A': // simulated fid peak amp.
                ival=args[i][2]-'0';
                if(ival>=0 && ival <=9){
                    if(get_value(args[i+1],&argf))
                       ddr_peak_ampl(ival,(double)argf);
                    i++;
                }
                break;

            case 'L': // simulated fid peak lw.
                ival=args[i][2]-'0';
                if(ival>=0 && ival <=9){
                    if(get_value(args[i+1],&argf))
                        ddr_peak_width(ival,argf);
                    i++;
                }
                break;
                
            case 's': // simulated fid
                ival=args[i][2]-'0';
                if(ival>=0 && ival <=9){
                    simpeaks=ival;
                    break;
                }
                switch(args[i][2]){
                case 'l': // line width
                    if(get_value(args[i+1],&argf)){
                        ddr_sim_lw(argf);
                        i++;
                    }
                    break;
                case 'd': // receiver delay
                    if(get_value(args[i+1],&argf)){
                        ddr_sim_rdly(argf*1e-6);
                        i++;
                    }
                    break;
                case 'f': // freq scale
                    if(get_value(args[i+1],&argf)){
                        ddr_sim_fscale(argf);
                        i++;
                    }
                    break;
                case 'a': // ampl scale
                    if(get_value(args[i+1],&argf)){
                        ddr_sim_ascale(argf);
                        i++;
                    }
                    break;
                case 'n': // noise
                    if(get_value(args[i+1],&argf)){
                        ddr_sim_noise(argf);
                        i++;
                    }
                    break;
                }
                break;
           }
         }
      }         
   }
}

//=========================================================================
// loadOTFGates() set OTF time and state
// - breaks up long times (>0.5 s) into smaller pieces
//=========================================================================
void loadOTFGates(double tm, int g)
{
    #define BIGTIMERWORD 0x3800000
    #define TIMERLIMIT   0x4000000
    
    long long ticks=S2C(tm);
    long long temp=ticks;
    int k,gates;
    int numInst, nroom;

    if(tm<TMIN) // interval too short
        return;
    gates=ALLGATES(g);
    
    while (temp > 0){
        if (temp > BIGTIMERWORD){
           // start the split but check remainder 
            if (temp > TIMERLIMIT)
                k = (int) BIGTIMERWORD;
            else
                k = temp;
        }
        else 
            k = temp;
        
        numInst =  OTF_ICNT;
        if(numInst>=FLDHIGH){
             while(numInst>FLDLOW){
                 numInst =  OTF_ICNT;
             }
        }
        
        temp -= (long long) k;
        OTF=gates;
        OTF=TIME(k);
    }
}

//=========================================================================
// parse() simulated acode parser
//=========================================================================
static void parse()
{
#define ALIGN(x) ((int)((x)*fmin+0.5))*tmin
 
    int i; 
    int flags=0;
    int delta;
    int scans=scan_count;
    int newfid=0;
    int T1,T2,T3,T4,G1,G2,icnt,test;
    int R=0,nbuffs=NACMS,bcnt=NT;
    double ta=TA,tc=TC,tmin=12.5e-9,fmin,tstep=0,duty,t1,t2;

    //DPRINT3(-3,"parse %d %d %d",ddr_error(),push_count,total_scans);

    if(ddr_error() || push_count>=total_scans)
        return;
        
    if(NB>0 && NT>NB){
        //nbuffs=NT/NB;
        bcnt=NB;
    }
           
    delta=push_count-scans;
    
    test=acq_test & ACQ_TYPE;
      
    while(push_count<total_scans&&delta<max_scan_frames){
        push_count++;
        delta=push_count-scans;

        // push new scan frame on scan queue
            
        if(dummy_scans>0){
            newfid=0;
            dummy_scans--;
        }
        else{
            flags=0;
            
            newfid=((acq_count%NT)==0)?1:0;
            if(newfid){
                if(NW>0){
                	ddr_set_rf(abrf[aqrf]); 
                	aqrf++;
                	aqrf%=NW;
                }
                ddr_push_fid(acq_count/NT,acq_count);
            }
            acq_count++;
                
            // PHASE acode
            
            if(NC>1||acq_count==1){    
                ddr_set_rp(abph[aqph]); 
                aqph++;
                aqph%=NC;
            }
            
            // ACM acode
            
            ct++;
            if(ct==bcnt && push_count<total_scans){
                if(NB>0){
                    dst=src+1;
                	if(dst==nbuffs)
                       dst=0;
                }
                else
                    dst=src;
            	ddr_set_acm(src,dst,0);
                src++;
                if(src==nbuffs)
                    src=0;
                dst=src;
                ct=0;
            }
            else{
                src=dst;
            	ddr_set_acm(src,dst,0);
            }
        }
        if(push_count>=total_scans)
            flags |= EXP_LAST_SCAN;
            
        ddr_push_scan(push_count,flags);

        // load FIFO timer states
        if(dummy_scans>0)
            OTF=ALLGATES(DDR_IEN);
        else if(newfid)
            OTF=ALLGATES(DDR_FID|DDR_PINS|DDR_IEN);
        else
            OTF=ALLGATES(DDR_SCAN|DDR_PINS|DDR_IEN);
        
        OTF=S2T(TP); 

        OTF=ALLGATES(DDR_IEN|DDR_RG);
        OTF=S2T(TX);
            
        if(ACYCS){  // optimized for fifo stuffer speed 
            switch(test){
            case ACQ_WACQ:
            	G1=SETGATES(DDR_IEN|DDR_ACQ|DDR_RG);
                //G2=CLRGATES(DDR_IEN|DDR_ACQ); // shows window glitches
                G2=CLRGATES(DDR_ACQ); 
                tmin=1.0/SR;
                break;
            case ACQ_WIEN:
                tmin=50e-9;  // deliberate fall through
            default: // -v1, -v3: toggle IEN leave ACQ on 
            	G1=SETGATES(DDR_IEN|DDR_ACQ|DDR_RG);
            	G2=CLRGATES(DDR_IEN);
                break;
            }
            fmin=1.0/tmin;
            if(acq_test & ACQ_RAMP){ 
                tc=ALIGN(tc);         
                tstep=(tc-ta)/ACYCS;
                for(i=0;i<ACYCS;i++){
                    t1=ALIGN(ta);
                    t1=t1<tmin?tmin:t1;
                    t2=tc-t1;
                    t2=t2<tmin?tmin:t2;
                    T1=S2T(t1);
                    T2=S2T(t2);
                    OTF=G1;    OTF=T1;
                    OTF=G2;    OTF=T2;
                    ta+=tstep;
                    
                    // wait for FIFO to drain down if too full
                    
                    icnt =  OTF_ICNT;
                    if(icnt>=FLDHIGH){
                        while(icnt>FLDLOW)
                            icnt = OTF_ICNT;
                    }
                }                
            }
            else {
                T1=S2T(ta);
                T2=S2T(tc-ta);
                for(i=0;i<ACYCS;i++){
                    OTF=G1;    OTF=T1;
                    OTF=G2;    OTF=T2;
                    
                    // wait for FIFO to drain down if too full
                    
                    icnt =  OTF_ICNT;
                    if(icnt>=FLDHIGH){
                        while(icnt>FLDLOW)
                            icnt = OTF_ICNT;
                    }
                }
            }
        }
        else{  // standard acquisition
             loadOTFGates(AQTM,DDR_IEN|DDR_ACQ|DDR_RG);
        }
        loadOTFGates(TR,DDR_IEN|DDR_ACQ);
      
        if(push_count==total_scans){
            OTF=S2T(TMIN);
            OTF=TIME(0);
        }
    }
}

//=========================================================================
// scan_parser_task() scan boundary parser task
//=========================================================================
void scan_parser_task()
{
    int stat,delta;
    scan_msg msg;
    
    FOREVER {   
        semTake(scan_sem,WAIT_FOREVER);
        delta=push_count-scan_count;
        if(delta<min_scan_frames)
            parse();        
    }
}

//=========================================================================
// initDDRScan: initialize scan thread
//=========================================================================
void initDDRScan()
{  
    if(ddr_run_mode==STAND_ALONE){ // stand-alone modes        
        scan_sem=semBCreate(SEM_Q_FIFO,SEM_EMPTY);
        ddrTask(SCAN_TSK_NAME, (void*)scan_parser_task, 95);
    }
}

//=========================================================================
// exptest() experiment execution test 
// - simfid peaks (P0..P9)
//     4200 -4400 3000 -3900 5700 3400 -4200 7730 -5000 6200
// - arrayed ddrfreq values (w0..w3) max=nw
//     2000 10000 40000 100000
// - arrayed abph values (p0..p3) max=nc
//     0.0 180.0 90.0 270.0
//=========================================================================
void exptest()
{        
    double xtm=0;
    int wait_fifo=100;
    int ncmax=sizeof(abph)/sizeof(double);
    long long clks;
    int dwclks,on=0,off=0;
    int nx1=0,tp1,tp2,nblks;

    double aqtm;
    long long expclks=0;
    int i = 0;
    int maxbuffs,bcnt,dbytes=4;

    if(ddr_run_mode!=STAND_ALONE){
        PRINT0("===>>>  exptest is only supported in stand-alone mode <<<===");
        return;
    }   

    // reset to default experiment parameter values
    
    SA=5.0;
    TD=0;
    
    TP=1e-7;     // pin-sync high period
    TX=2e-6;     // pin-sync low period
    TI=10e-3;    // initial predelay
    SW1=0;       // stage 1 spectral width (after decimation)
    M1=0;        // stage 1 decimation
    OS1=7;       // stage 1 oversampling factor
    XS1=0.0;     // stage 1 bc shift
    FW1=1.0;     // stage 1 filter factor
    SW2=0;       // stage 2 spectral width
    M2=0;        // stage 2 decimation
    OS2=0;       // stage 2 oversampling factor
    XS2=0.0;     // stage 2 bc shift
    FW2=1.0;     // stage 2 filter factor
    NP=1000;     // number of points in FID
    NT=1;        // number of transients to average
    NACMS=0;     // number of accumulation buffers
    NF=1;        // number of "fids" (arraydim)
    NW=0;        // number of "freqs" (arraydim)
    NACQS=0;     // number of acquisition buffers
    ND=0;
    NB=0;
    WF=0;
    WP=0;
    NC=0;
    NX=0;
    TA=TC=TS=0;
    ACYCS=0;
    
    PFAVES=1;
    BKPTS=1000;
    max_scan_frames=32;
    min_scan_frames=16;
    
    for(i=0;i<4;i++){
    	abph[i]=dflt_abph[i];
    	abrf[i]=dflt_abrf[i];
    }
    ct=src=dst=0;
    tclks=0;    
    scan_info=0;
    aqph=aqrf=0;
    simpeaks=-1;
    data_stats=0;
    acq_test=0;
    ssync=1;
    exact_aqtm=1;
    MODE=MODE_DMA;
     
    ddr_init();
    
    if(ddr_wait(EXP_READY,10)==0){
        printf("wait failed %s\n","ddr_init");
        return;
    }
    
    getexpars();   // get defaults over-rides from "expargs" file
    bcnt=NT;

    if(NACMS==0){        
    	if(NB>0 && NT>NB){
        	bcnt=NB;
        	if(NT%NB>0)
        		NACMS=(NT/NB+1)*NF;
        	else
        		NACMS=(NT/NB)*NF;
   	 	}
    	else{
        	NACMS=NF;
       	}
    }
    dbytes=(MODE&DATA_DOUBLE)?16:8;
    maxbuffs=DDR_DATA_SIZE/dbytes/NP;
    
    NACMS=NACMS>maxbuffs?maxbuffs:NACMS;
    
    dummy_scans=ND;
    total_scans=NF*NT+dummy_scans;

    min_scan_frames=max_scan_frames-min_scan_frames;
    if(min_scan_frames<0)    
        min_scan_frames=0;

    if(simpeaks>=0)
        BIT_ON(MODE,MODE_FIDSIM);
    ddr_sim_peaks(simpeaks>=0?simpeaks:0);
     
    SR=SA*1e6;
    
    if(TD>0)
        BIT_OFF(MODE,AD_SKIP);

    if(OS2==0){
       SW2=0;
       M2=0;
    }
    if(OS1==0){
       SW1=SW2=0;
       M1=M2=0;
    }
    if(M1>0){
        SW1=SR/M1;
    }
    if(M2>0)
       	SW2=SW1/M2;
       
    NC=NC>ncmax?ncmax:NC;
             
    resetOTF();  // clear any residual states from the output FIFO

    if(MODE & MODE_DITHER_OFF)
        set_dither(0);  // disable dithering
    else
        set_dither(1);  // enable dithering
            
    if(MODE & MODE_ADC_DEBUG)
        setADCDebug(1);  // use "fake" fpga generated data (saw-tooth pattern)
    else
        setADCDebug(0);  // use "real" DDR-A acquired data

    push_count=scan_count=acq_count=0;    
    
    //ddr_set_mode(MODE,BKPTS,0);
    ddr_set_dims(NACMS,NP,NP);
    ddr_set_xp(1,1,0);
    
    ddr_set_cp(WP); 
    ddr_set_cf(WF); 
    ddr_set_sr(SA,TD); 
    ddr_set_sw1(SW1);
    ddr_set_os1(OS1);
    ddr_set_xs1(XS1);
    ddr_set_fw1(FW1);
    ddr_set_sw2(SW2);
    ddr_set_os2(OS2);
    ddr_set_xs2(XS2);
    ddr_set_fw2(FW2);
    ddr_set_rp(abph[aqph]); 

    ddr_set_filter();
    ddr_set_pfaves(PFAVES);

    // determine data fifo interrupt count for first and subsequent blocks
    nx1=ddr_get_aqtm()*SA*1e6;
    if(BKPTS>=nx1){
        tp1=tp2=nx1;
        nblks=1;
    }
    else{
        int rem=nx1%BKPTS;
        nblks=nx1/BKPTS;
        tp2=BKPTS+rem/nblks;
        rem=nx1%tp2;
        tp1=tp2+rem; // first block picks up remainder
    }
    ddr_set_mode(MODE,tp1,BKPTS);
                 
    SW=ddr_get_sw();
        
    if(exact_aqtm)
       AQTM=ddr_get_aqtm();
    else
       AQTM=NP/SW;
    
    if((acq_test & ACQ_CYCLE) && TC>0 && TC>TA){
        switch(acq_test & ACQ_TYPE){
        case ACQ_ZIEN:
             ACYCS=(AQTM/TC+2.5);
             aqtm=ACYCS*TC; 
             break;       
        case ACQ_WIEN:
             TC=((int)(TC*20e6+0.5))*50e-9;  // align to 4 clock grid
        case ACQ_WACQ:
             if(acq_test & ACQ_RAMP){
                 ACYCS=2*AQTM/(TC+TA);  // ave TA
                 aqtm=ACYCS*TC; // actual length of acquisition period
             }
             else {
                ACYCS=(AQTM/TA);
                aqtm=ACYCS*TC; // actual length of acquisition period            
                SW*=TA/TC;     // scale effective sw
             }
             break;
        }
        AQTM=aqtm;
        
        on=TA*SR;
        off=(TC-TA)*SR;
        PRINT2("on %d off %d",on,off);
    }
    ddr_set_cycle(on,off,(acq_test & ACQ_WACQ)?0:1);
    
    ddr_init_exp(NF,0,0);

    if(ddr_wait(EXP_INIT,1000)==0){
        //printf("wait failed %s\n","ddr_init_exp");
        return;
    }
        
    dwclks=S2C(1.0/SW);
           
    xtm=C2M(S2C(AQTM)-NP*dwclks);
    
    clks=S2C(AQTM)+S2C(TR)+S2C(TP)+S2C(TX);
    if(ssync)
        xclks=clks%4;
    else
        xclks=0;
    
    clks=clks+tclks-xclks;
    TR=TR+C2S(tclks-xclks+NX);
    
    // predelay
    
    loadOTFGates(TI,0); 
 
    xclks=0;
     
    semGive(scan_sem);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
 
    ddr_start_exp(MODE);
    taskDelay(2);  /* taskDelay(1); */

    if(ddr_wait(EXP_RUNNING,10)==0){
        printf("wait failed %s\n","ddr_start_exp");
        return;
    }
    softStartOTF();
#ifdef PRINT_STATS
    expclks=S2C(TI)+total_scans*clks;
    PRINT4("NT %d NF %d NB %d NACMS %d",NT,NF,NB,NACMS);
    if(ACYCS)
    	PRINT6("ACYCS %d TA %g TS %g TC %g SW %g ACQTM %g ms",
                 ACYCS,TA*1e6,TS*1e6,TC*1e6,SW,1e3*aqtm);

    PRINT2("TIME: TP %g TX %g us",1e6*TP, 1e6*TX);
    PRINT2("TIME: TACQ %g TR %g ms",AQTM*1e3,1e3*TR);
    PRINT5("TIME: DW %g us AT %g XTM %g ms SCAN %g s EXP %g s",
         C2U(dwclks),C2S(NP*dwclks),xtm,C2S(clks),C2S(expclks));
    PRINT4("CLKS: TP %d TX %d TACQ %-8.0f TR %-8.0f",
         (int)S2C(TP), (int)S2C(TX),(double)S2C(AQTM),(double)S2C(TR));
    PRINT4("CLKS: DW %d SCAN %8.0f EXP %8.0f XC %d",
          dwclks,(double)clks,(double)expclks,xclks);
#endif
    //printOTFStatus();
    taskDelay(20);
    if(scan_info){
        print_exp_info();
    }
}

//=========================================================================
// writeFID:save data file 
//=========================================================================
void writeFID(char *fid,int src)
{ 
    extern void get_data_info(int s, float **data, int *size,int *type);
    float *adrs;
    int np;
    int dtype;
    get_data_info(src, &adrs, &np, &dtype);
    printf("saving %d points to file %s from adrs 0x%0.8X\n",np,fid,adrs);
    writeDDRFile(fid,adrs,np,dtype);
}

//=========================================================================
// writeHFID: save prefid data file (real data only)
//=========================================================================
void writeHFID(char *fid)
{
    int prefid;
    int pfsize;

    ddr_xin_info();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    prefid=ddr_prefid();
    pfsize=ddr_prefid_size();
    if(!pfsize)
        return;
    
    writeDDRFile(fid,prefid+4+4*pfsize,pfsize,DATA_SHORT);
}

//=========================================================================
// writePFID: save prefid data file (real and reflected)
//=========================================================================
void writePFID(char *fid)
{
    int prefid;
    int pfsize;

    ddr_xin_info();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    prefid=ddr_prefid();
    pfsize=ddr_prefid_size();
    if(!pfsize)
        return;
    writeDDRFile(fid,prefid+4,2*pfsize,DATA_SHORT);
}

//=========================================================================
// print_scan_stats: print scan counts 
//=========================================================================
void print_scan_stats()
{
    printf("scans output %d parsed %d\n",
          scan_count,push_count); 
    print_msg_stats();
}

//=========================================================================
// wait_exp_end: wait for end of exp 
//=========================================================================
void wait_exp_end()
{
    ddr_wait(EXP_DATA,-1); // wait forever
}

//-------------------------------------------------------------
// maxcr() print filter max cor
//-------------------------------------------------------------
void maxcr1(int sa, int m)
{
   double maxcr=ddr_calc_maxcr(sa*1.0e6, 1, m);
   printf("maxcr=%g (1 stage)\n",maxcr);
    if(sa>3)
       printf("maxcr=%g sr=5.0 MHz 1-stage\n",maxcr);
    else
       printf("maxcr=%g sr=2.5 MHz 1-stage\n",maxcr);
}

//-------------------------------------------------------------
// maxcr() print filter max cor
//-------------------------------------------------------------
void maxcr2(int sa, int m)
{
    double maxcr=ddr_calc_maxcr(sa*1.0e6, 2, m);
    if(sa>3)
       printf("maxcr=%g sr=5.0 MHz 2-stage\n",maxcr);
    else
       printf("maxcr=%g sr=2.5 MHz 2-stage\n",maxcr);
}
