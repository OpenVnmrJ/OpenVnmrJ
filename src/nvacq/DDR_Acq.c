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
// FILE: DDR_Acq.c
//=========================================================================
//   I. functional responsibilities of module
//     1. contains functions to send messages to the C67
//     2. contains functions to receive messages from the C67
//     3. contains NMR experiment control functions (DDR_*)
//     4. executes functions for messages sent by the C67 (HOST_*) 
//=========================================================================
#include <stdlib.h>
#ifdef INSTRUMENT
/* #include "wvLib.h"          /* wvEvent() */
#include "instrWvDefines.h"
#endif

#include "expDoneCodes.h"
#include "DDR_Common.h"
#include "DDR_Globals.h"
#include "DDR_Acq.h"
#include "logMsgLib.h"
#include "errorcodes.h"
#include "upLink.h"
#include "taskPriority.h"
#include <math.h>

#define DMSG_ISR_NAME "tDMsgIST"
#define DMSG_TSK_NAME "tDMsgTSK"

int adc_ovf_count=0;
int enable_adc_ovf=0;

extern int ddrUsePioFlag;
extern double C6Speed;
static int push_count=0;
int hpi_two_reads=0;
int hpi_two_writes=1;

extern int ddr_il_incr;   /* interleave cycle count, used by fid & ct count interrupt */

extern int ddrTask(char *name, void* task, int priority);
extern void getDDRData(int adrs, unsigned int pts, float *dest, int dtype);
void dmsg_isr();
void dmsg_exec_task();
void dmsg_reader_task();
void ddr_fpga_hw_isr();
void initDDRmsgs();
void host_data(int src, int id, int cs, int stat);
void host_error(int id, int arg1, int arg2);

// message queue properties

#define MAX_DDR_MSGS        256
#define DDR_MSG_SIZE        sizeof(msg_struct)
#define DATA_MSG_SIZE       sizeof(data_msg)
#define SCAN_MSG_SIZE       sizeof(scan_msg)

#define DFLT_DDR_DEBUG 	 DDR_DEBUG_ERRS
#define DEBUG_ALL        DFLT_DDR_DEBUG|DDR_DEBUG_HMSGS|DDR_DEBUG_DMSGS|DDR_DEBUG_SCANS

static unsigned int dl=0;
static unsigned int al=0;
static unsigned int nacms=0;
static unsigned int nf=1;
static unsigned int nfmod=1;
static unsigned int nfsize=0;
static unsigned int nt=0;
static double sr=0;
static double sa=0;
static double sw1=0,os1=0,xs1=0.5,fw1=1.0;
static double sw2=0,os2=0,xs2=0.5,fw2=1.0;
static double sd=0;
static unsigned int nx1=0,ny1=0;
static unsigned int n1=0,m1=0,b1=0;
static unsigned int n2=0,m2=0,b2=0;
static unsigned int stages=0;
static unsigned int acqstat=0;
static int watchdog_count=0;
static unsigned int mode=0;
static unsigned int xin=0;
static unsigned int prefid=0;
static unsigned int pfaves=1;
static unsigned int pfsize=0;
static unsigned int xskips=0;
static unsigned int bkpts=4000;
static unsigned int data_count=0;
static unsigned int data_tt=0;
static unsigned int data_scale=1;
static unsigned int databuffsize=0;
static unsigned int databuffs=0;

#define HOST_MSGS_ALL_LOW_PRIORITY
//#define DEBUG_SEQUENCE_ORDER

#ifdef DEBUG_SEQUENCE_ORDER
static int last_data,last_unlock,last_acm,last_push;
static int acm_errors,data_errors,unlock_errors,push_errors;
#define TEST_ORDER(type,msg) \
    if(src>last_##type+1){ \
        if(src-last_##type>2){ \
            PRINT2(msg" MISSING %d-%d",last_##type+1,src-1); \
        } \
        else{ \
            PRINT1(msg" MISSING %d",src-1); \
       } \
       type##_errors+=src-last_##type-1; \
    } \
    last_##type=src;
#define SEQUENCE_TEST_INIT \
    last_data=last_unlock=last_acm=last_push=0; \
    data_errors=unlock_errors=acm_errors=push_errors=0
#define SEQUENCE_TEST_PRINT \
    PRINT4("MISSING SEQUENCE EVENTS ddr_set_acm:%d host_data:%d ddr_unlock:%d ddr_push_scan:%d", \
       acm_errors,data_errors,unlock_errors,push_errors)
#else
#define TEST_ORDER(type,msg)
#define SEQUENCE_TEST_INIT
#define SEQUENCE_TEST_PRINT
#endif
void ddr_xin_info();

//########################  utility functions #############################

union ltw {
  unsigned long long ldur;
  unsigned int  idur[2];
};

//=========================================================================
// markTime: set system clock time marker
//=========================================================================
void markTime(long long *dat)
{ 
  union ltw TEMP;
  int top,bot;
  vxTimeBaseGet(&top,&bot);
  TEMP.idur[0] = top;
  TEMP.idur[1] = bot;
  *dat = (TEMP.ldur);
}

//=========================================================================
// diffTime: return delta-time (microseconds)
//=========================================================================
double diffTime(long long m2,long long m1)
{
    return ((double)(m2-m1))/333.3 ;
}

void packd(double d, int *buffer, int *iptr)
{
    u64 tmp;
    int index=*iptr;
    tmp.d=d;
    buffer[index]=tmp.i.h;
    buffer[index+1]=tmp.i.l;
    *iptr = index+2;
}

//=========================================================================
// unpackd: unpack double from int array
//=========================================================================
double unpackd(int *buffer,int *index)
{
    u64 tmp;
    tmp.i.h=buffer[*index];
    tmp.i.l=buffer[*index+1];
    *index += 2;
    return tmp.d;
}
//=========================================================================
// unpack: unpack double from ints 
//=========================================================================
double unpack(int a,int b)
{
    u64 tmp;
    tmp.i.h=a;
    tmp.i.l=b;
    return tmp.d;
}

//=========================================================================
// ddr_wait(stat,ms): wait for ddr status bit to be set
// - stat  bit to check
// - ms    ms to wait (x10)
//=========================================================================
int ddr_wait(int stat,int ms)
{
    int i,waits=1;
    volatile int test=acqstat;
    if(ms==0)
       return 1;
    else if(ms<0){ 
        FOREVER {
            test=acqstat;
            if(test & stat)
                return waits;
            taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
            waits++;
        }
    }
    else{
        for(i=0;i<ms;i++){
            test=acqstat;
            if(test & stat)
                return waits;
            taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
            waits++;
        }
    }
    return 0; 
}

//=========================================================================
// ddr_wait_test(max): wait for ddr status bit to be set
//=========================================================================
int ddr_wait_test(int max)
{
    int i,waits=1;
    volatile int test=acqstat;
    
    if(max<=0){ 
        FOREVER {
            test=acqstat;
            if(test & EXP_TEST)
                return waits;
            taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
            waits++;
        }
    }
    else{
        for(i=0;i<max;i++){
            test=acqstat;
            if(test & EXP_TEST)
                return waits;
            taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
            waits++;
        }
    }
    return 0; 
}

//=========================================================================
// ddr_mode: return ddr mode
//=========================================================================
int ddr_mode()
{
    return mode; 
}

//=========================================================================
// ddr_status: return ddr status
//=========================================================================
int ddr_status()
{
    return acqstat; 
}

//=========================================================================
// set_data_ready: set ddr ready status
//=========================================================================
void set_data_ready()
{
    BIT_ON(acqstat,EXP_DATA); 
}

//=========================================================================
// ddr_ready: return data ready status
//=========================================================================
int data_ready()
{
    return (acqstat&EXP_DATA)?1:0; 
}

//=========================================================================
// ddr_nacms: return ddr acq buffers
//=========================================================================
int ddr_nacms()
{
    return nacms; 
}

//=========================================================================
// ddr_error: return ddr error status
//=========================================================================
int ddr_error()
{
    return (acqstat&EXP_ERROR)?1:0; 
}

//=========================================================================
// ddr_ready: return ddr ready status
//=========================================================================
int ddr_ready()
{
    return (acqstat&EXP_READY)?1:0; 
}

//=========================================================================
// ddr_xin: return ddr input data address
//=========================================================================
int ddr_xin()
{
    return xin; 
}

//=========================================================================
// ddr_prefid: return ddr prefid data address
//=========================================================================
int ddr_prefid()
{
    return prefid; 
}

//=========================================================================
// ddr_prefid_size: return ddr prefid size
//=========================================================================
int ddr_prefid_size()
{
    return pfsize; 
}

//-------------------------------------------------------------
// show_prefid() show first part of simulated fid
//-------------------------------------------------------------
void show_prefid(int max)
{
    int i,n,pts;
    siu *data=0;
    siu  tmp;
    double maxd=1.0/(double)0xffff;
    double ampl=1,X0,X1,R,I;
    double C[2];
    double p1;

    ddr_xin_info();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    printf("XIN  X:0x%0.8X B:0x%0.8X PFSIZE:%d\n",xin,prefid,pfsize);
    
    n=pfsize;
    if(max && max<n)
        n=max;

    pts=2*pfsize;
    data=(siu*)malloc(pts*4+8);
    if(!data)
        return;
        
    getDDRData(prefid,pts,(float*)data,DATA_SHORT);

    X0=data[0].s[0];
    X1=data[0].s[1];

    X0*=maxd;
    X1*=maxd;
    
    if (X0 || X1) {
        ampl = 1.0 / (X0 * X0 + X1 * X1);
        C[0] = (float) ((X0 * X0 - X1 * X1) * ampl);
        C[1] = (float) (2 * X0 * X1) * ampl;
    } else {
        C[0] = 1;
        C[1] = 0;
    }

    printf("X0 %-1.4f X1 %-1.4f C0 %0.4f C1 %0.4f\n",X0,X1,C[0],C[1]);
       
    for(i=0;i<n;i++){
        tmp=data[pfsize-n+i+1];
        R=maxd*tmp.s[0];
        I=maxd*tmp.s[1];
        printf("B:%-5d R:%-1.4f I:%-1.4f A:%-1.4f\n",n-i-1,R,I,sqrt(R*R+I*I));
    }
    for(i=0;i<n;i++){
        tmp=data[i+pfsize+1];
        R=maxd*tmp.s[0];
        I=maxd*tmp.s[1];
        printf("X:%-5d R:%-1.4f I:%-1.4f A:%-1.4f\n",i,R,I,sqrt(R*R+I*I));
    }
    free(data);
}

//=========================================================================
// ddr_set_show: set debug bit
//=========================================================================
void ddr_set_debug(int m,int i)
{
    BIT_SET(ddr_debug,m,i);
}

//=========================================================================
// ddr_debug_default: reset debug bits
//=========================================================================
void ddr_debug_default()
{
    ddr_debug=DFLT_DDR_DEBUG;
}

//=========================================================================
// ddr_debug_none: clear debug bits (except errors)
//=========================================================================
void ddr_debug_none()
{
    ddr_debug=DDR_DEBUG_ERRS;
}

//=========================================================================
// ddrx: debug xfers no prints
//=========================================================================
void ddrx(int i)
{
    if(i)
        ddr_debug=DDR_DEBUG_XFER|DDR_DEBUG_ERRS|DDR_DEBUG_DATA;
    else
        ddr_debug=DDR_DEBUG_XFER|DDR_DEBUG_ERRS;
}

//=========================================================================
// ddr_debug_all: set debug bits
//=========================================================================
void ddr_debug_all()
{
    ddr_debug=DEBUG_ALL;
}

//=========================================================================
// ddr_debug_hmsgs: set debug bit
//=========================================================================
void ddr_debug_hmsgs(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_HMSGS,i);
}

//=========================================================================
// ddr_debug_dmsgs: set debug bit
//=========================================================================
void ddr_debug_dmsgs(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_DMSGS,i);
}

//=========================================================================
// ddr_debug_msgs: show all messages
//=========================================================================
void ddr_debug_msgs(int i)
{
    ddr_debug_hmsgs(i);
    ddr_debug_dmsgs(i);
}

//=========================================================================
// ddr_debug_post: set/clr message post debug bit
//=========================================================================
void ddr_debug_post(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_MPOST,i);
}

//=========================================================================
// ddr_debug_scans: show scan messages
//=========================================================================
void ddr_debug_scans(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_SCANS,i);
}

//=========================================================================
// ddr_debug_data: show data messages
//=========================================================================
void ddr_debug_data(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_DATA,i);
}

//=========================================================================
// ddr_debug_data: show data messages
//=========================================================================
void ddr_debug_ints(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_INTS,i);
}

//=========================================================================
// ddr_debug_xfer: show data xfer messages
//=========================================================================
void ddr_debug_xfer(int i)
{
    BIT_SET(ddr_debug,DDR_DEBUG_XFER,i);
}

//=========================================================================
// printSamplingRate: show sampling rate (MHz)
//=========================================================================
void printSamplingRate()
{
    printf("SR=%g\n",getSamplingRate());
}

//=========================================================================
// ddr_calc_delay: calculate number of autoskip points
//=========================================================================
double ddr_calc_delay()
{
    if(mode & AD_SKIP){
       if(sr>=10e6){
           return 1.9e-6;;   // 1.9 us
       }
       else if(sr>=5.0e6){
           switch(mode & AD_FILTER){
           default:
           case AD_FILTER1:  // 32 taps
               switch(mode & AD_SKIP){
               case AD_SKIP1: return 1.4e-6;
               case AD_SKIP2: return 1.8e-6;
               case AD_SKIP3: return 2.0e-6;
               }
               break;
           case AD_FILTER2:  // 64 taps
               switch(mode & AD_SKIP){
               case AD_SKIP1: return 2.1e-6;
               case AD_SKIP2: return 3.0e-6;
               case AD_SKIP3: return 3.2e-6;
               }
               break;
           }
       }
       else{  // 2.5 MHz
           switch(mode & AD_FILTER){
           case AD_FILTER1:  // 32 taps
               switch(mode & AD_SKIP){
               case AD_SKIP1: return 1.2e-6;
               case AD_SKIP2: return 1.6e-6;
               case AD_SKIP3: return 2.0e-6;
               }
               break;
           default:
           case AD_FILTER2:  // 64 taps
               switch(mode & AD_SKIP){
               case AD_SKIP1: return 2.4e-6;
               case AD_SKIP2: return 3.1e-6;
               case AD_SKIP3: return 3.2e-6;
               }
               break;
           }
       }
    }
    return 0;
}

//=========================================================================
// calc_stages: calculate filter stages and decimation factors from sw
//=========================================================================
void ddr_calc_stages(double sr, double sw, UINT *n, UINT *a, UINT *b)
{
    UINT stages=1;
    UINT m1=1;
    UINT m2=1;
    
	if (sw > 167778) {
		stages = 1;
	} else if (sw > 5000) {
		m2 = 2;
		stages = 2;
	} else {
		m2 = 4;
		stages = 2;
	}
	m1 = (int) (sr/m2/sw + 1.0/(2.0 * sr));
	*n=stages;
	*a=m1;
	*b=m2;
}

//=========================================================================
// ddr_calc_maxcr: calculate max filter cor
//=========================================================================
double ddr_calc_maxcr(double sr, int stages, int m1)
{
	double maxcr = 0;
	if (sr > 3e6) {//5 MHz sampling
		if (stages == 1) {
			if (m1 == 5)
				maxcr = 9;
			else if (m1 <= 15)
				maxcr = 0.66 * m1 + 4;
			else
				maxcr = 16;
		} else {
			if (m1 > 40)
				maxcr = 3.5 * m1 + 60;
			else
				maxcr = 7.2 * m1 - 90;
		}
	} else {  //2.5 MHz sampling
		if (stages == 1) {
			if (m1 == 2)
				maxcr = 15;
			else if (m1 == 3)
				maxcr = 25;
			else if (m1 == 4)
				maxcr = 35;
			else if (m1 == 5)
				maxcr = 40;
			else if (m1 == 6)
				maxcr = 45;
			else
				maxcr = 50;
		} else {
			if (m1 > 14)
				maxcr = 11.0 * m1 + 60;
			else
				maxcr = 23 * m1 - 90;
		}
	}
	return maxcr>1000?1000:maxcr;
}

//=========================================================================
// ddr_set_filter: set filter variables
//=========================================================================
void ddr_set_filter()
{
    if(os1==0 || sw1==0){
        stages=0;
        sw1=sr;
        os2=0;
        sw2=0;
        os1=0;
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
        b1=b1<0?0:b1;       
        b1=b1>n1-1?n1-1:b1;       
        if(sw2 && os2){
            stages=2;       
            m2=(int)(sw1/sw2+0.5);
            n2=(int)(os2*sw1/sw2+0.5);
            n2=(n2%2==0)?n2+1:n2;   
            if(xs2>=1 || xs2<=-1)       
                b2=(int)(0.5*n2-xs2);
            else
                b2=(int)(n2*(0.5-xs2));
            b2=b2<0?0:b2;
            ny1=(al-1)*m2+n2-b2;
            nx1=(ny1-1)*m1+n1-b1;   
        }
        else{
            stages=1;       
            ny1=al;
            nx1=(al-1)*m1+n1-b1;
        }
    }
}

//=========================================================================
// ddr_get_aqtm() return calculated acquisition time
//    return invalid unless called after ddr_set_filter (or ddr_init_exp)
//=========================================================================
double ddr_get_aqtm()
{
    return nx1/sr;
}

//=========================================================================
// ddr_get_sw() return calculated sweep width
//    return invalid unless called after ddr_set_filter (or ddr_init_exp)
//=========================================================================
double ddr_get_sw()
{
    if(os1==0 || sw1==0)
       return sr;
    if(os2==0 || sw2==0)
       return sr/m1;
    return sr/m1/m2;      
}

//-------------------------------------------------------------
// print_exp_status() print current ddr status
//-------------------------------------------------------------
void print_exp_status()
{
    char buff[256];
    buff[0]=0;
    if(acqstat&EXP_READY)
        strcat(buff,"|READY");
    if(acqstat&EXP_INIT)
        strcat(buff,"|INIT");
    if(acqstat&EXP_RUNNING)
        strcat(buff,"|RUNNING");
        
    if(buff[0]){
        buff[0]=' ';
    }
    printf("STATUS  0x%-0.8X  %s\n",acqstat,buff);
}

//-------------------------------------------------------------
// print_exp_mode() print ddr exp mode
//-------------------------------------------------------------
void print_exp_mode()
{
    char buff[256];
    buff[0]=0;
    if(C6Speed==300)
        strcat(buff,"|C6CPU_300");
    else
        strcat(buff,"|C6CPU_220");
    if((mode&DATA_TYPE)==DATA_DOUBLE)
        strcat(buff,"|DP");
    if(mode&DATA_PACK)
        strcat(buff,"|2x16");
    if(mode&MODE_ADC_DEBUG)
        strcat(buff,"|ADC_DEBUG");
    if(mode&MODE_DITHER_OFF)
        strcat(buff,"|DITHER_OFF");
    if(mode&MODE_NOPS)
        strcat(buff,"|NOPS");
    if(mode&MODE_FIDSIM)
        strcat(buff,"|FIDSIM");
    if(mode&MODE_CFLOAT)
        strcat(buff,"|CFLOAT");
    if(mode&MODE_DMA)
        strcat(buff,"|DMA");       
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
    else if((mode&FILTER_TYPE)==FILTER_BLCKH)
            strcat(buff,"|BLACKMAN-HARRIS");
        
    if(buff[0]){
        buff[0]=' ';
    }
    printf("MODE    0x%-0.8X  %s\n",mode,buff);
}

//-------------------------------------------------------------
// print_exp_info() print current ddr status
//-------------------------------------------------------------
void print_exp_info()
{
    double sw=ddr_get_sw();
    printf("NACMS   %-8d\n",nacms);
    printf("NF      %-8d  NFNOD %d\n",nf,nfmod);
    printf("DL      %-8d\n",dl);
    printf("AL      %-8d\n",al);
    printf("STAGES  %-8d\n",stages);
    printf("SR      %-8g\n",sr);
    printf("NX1     %-8d NY1 %d\n",nx1,ny1);
    if(stages>0){
        printf("W1      %-8g  O1 %-8g  X1 %g\n",sw1,os1,xs1);
        printf("N1      %-8d  M1 %-8d  B1 %d\n",n1,m1,b1);
        if(stages>1){
            printf("W2      %-8g  O2 %-8g  X2 %g\n",sw2,os2,xs2);
            printf("N2      %-8d  M2 %-8d  B2  %d\n",n2,m2,b2);
        }
    }
    printf("AQTM    %g ms SW %g Hz DW %g us\n",1e3*nx1/sr,sw,1e6/sw);
    print_exp_status();
    print_exp_mode();
}
//-------------------------------------------------------------
// ddr_string() return message string (HOST->DDR messages)
//-------------------------------------------------------------
void ddr_string(int id,int a,int b,int c, char *str)
{
    switch(id){
    case DDR_NOINIT:       
        sprintf(str,"DDR_NOINIT");
        break;
    case DDR_INIT:       
        sprintf(str,"DDR_INIT");
        break;
    case DDR_INIT_EXP:       
        sprintf(str,"DDR_INIT_EXP fs:%d sf:%d df:%d",a,b,c);
        break;
    case DDR_START_EXP:       
        sprintf(str,"DDR_START_EXP mode:0x%-0.8X",a);
        break;
    case DDR_STOP_EXP:       
        sprintf(str,"DDR_STOP_EXP mode:0x%-0.8X",a);
        break;
    case DDR_SET_MODE:   
        sprintf(str,"DDR_SET_MODE mode:0x%-0.8X tp1:%d tp2:%d",a,b,c);
        break;    
    case DDR_SET_DIMS:   
        sprintf(str,"DDR_SET_DIMS nacms:%d dl:%d al:%d",a,b,c);   
        break;    
    case DDR_SET_XP:     
        sprintf(str,"DDR_SET_XP nf:%d nfmod:%d ascale:%d",a,b,c);
        break;   
    case DDR_SET_SR:     
        sprintf(str,"DDR_SET_SR %g Hz", unpack(a,b));
        break; 
    case DDR_SET_SD:     
        sprintf(str,"DDR_SET_SD %g us",unpack(a,b)*1e6);
        break; 
    case DDR_SET_SW:     
        sprintf(str,"DDR_SET_SW%d %g",c,unpack(a,b));
        break; 
    case DDR_SET_OS:     
        sprintf(str,"DDR_SET_OS%d %g",c,unpack(a,b));
        break; 
    case DDR_SET_XS:     
        sprintf(str,"DDR_SET_XS%d %g",c,unpack(a,b));
        break; 
    case DDR_SET_FW:     
        sprintf(str,"DDR_SET_FW%d %g",c,unpack(a,b));
        break; 
    case DDR_SET_RP:     
        sprintf(str,"DDR_SET_RP %g",unpack(a,b));
        break; 
    case DDR_SET_RF:     
        sprintf(str,"DDR_SET_RF %g",unpack(a,b));
        break; 
    case DDR_SET_CP:     
        sprintf(str,"DDR_SET_CP %g",unpack(a,b));
        break; 
    case DDR_SET_CA:     
        sprintf(str,"DDR_SET_CA %g",unpack(a,b));
        break; 
    case DDR_SET_CF:     
        sprintf(str,"DDR_SET_CF %g",unpack(a,b));
        break; 
    case DDR_PFAVES:     
        sprintf(str,"DDR_PFAVES %d",a);
        break; 
    case DDR_SET_ACM:     
        sprintf(str,"DDR_SET_ACM src:%d dst:%d nt:%d",a,b,c);
        break; 
    case DDR_PUSH_SCAN:     
        sprintf(str,"DDR_PUSH_SCAN src:%d status:0x%-0.8X",a,b);
        break; 
    case DDR_UNLOCK:     
        sprintf(str,"DDR_UNLOCK src:%d",a);
        break; 
    case DDR_TEST:     
        sprintf(str,"DDR_TEST id:%d %d %d",a,b,c);
        break; 
    case DDR_SIM_PEAKS:     
        sprintf(str,"DDR_SIM_PEAKS %d",a);
        break; 
    case DDR_SIM_FSCALE:     
        sprintf(str,"DDR_SIM_FSCALE %g %%",unpack(a,b)*100);
        break; 
    case DDR_SIM_ASCALE:     
        sprintf(str,"DDR_SIM_ASCALE %g %%",unpack(a,b)*100);
        break; 
    case DDR_SIM_NOISE:     
        sprintf(str,"DDR_SIM_NOISE %g %%",unpack(a,b)*100);
        break; 
    case DDR_SIM_RDLY:     
        sprintf(str,"DDR_SIM_RDLY %g us",unpack(a,b)*1e6);
        break; 
    case DDR_SIM_LW:     
        sprintf(str,"DDR_SIM_LW %g Hz",unpack(a,b));
        break; 
    default:
        sprintf(str,"DDR_[%d] 0x%0.8X 0x%0.8X]",id,a,b);
        break;
    }
}

//-------------------------------------------------------------
// host_string() return message string (DDR->HOST messages)
//-------------------------------------------------------------
void host_string(int id,int a,int b,int c, char *str)
{
    switch(id){
    case HOST_NOINIT:       
        sprintf(str,"HOST_NOINIT");
        break;
    case HOST_INIT:
        sprintf(str,"HOST_INIT status: 0x%0.8X",a);
        break;
    case HOST_TEST:     
        sprintf(str,"HOST_TEST id:%d %d %d",a,b,c);
        break; 
    case HOST_ERROR:     
        sprintf(str,"HOST_ERROR id:%d %d %d",a,b,c);
        break; 
    case HOST_INIT_EXP:
        sprintf(str,"HOST_INIT_EXP status: 0x%0.8X",a);   
        break;    
    case HOST_START_EXP:
        sprintf(str,"HOST_START_EXP status: 0x%0.8X",a);
        break;   
    case HOST_STOP_EXP:
        sprintf(str,"HOST_STOP_EXP status: 0x%0.8X",a);
        break;        
    case HOST_DATA:
        sprintf(str,"HOST_DATA acm:%d id:%d cs:0x%0.8X",a, b); 
        break;  
    case HOST_END_DATA:  
        sprintf(str,"HOST_END_DATA acm:%d id:%d cs:0x%0.8X",a, b);
        break;
    case HOST_END_EXP:
        sprintf(str,"HOST_END_EXP cpu:%d tm:%d tt:0x%0.8X",a,b,c);
        break;
    default:
        sprintf(str,"HOST_[%d] 0x%0.8X 0x%0.8X]",id,a,b);
        break;
    }
}
//=========================================================================
// send_hmsg_int: send C67_INT4 interrupt to the C67
// - WARNING: C67 ISR clearing C6I4 results in a "machine check" or
//            vxWorks "lock-up". (Need to find out why this is happening)
//=========================================================================
void send_hmsg_int() 
{
    GP_INT=0;
    GP_INT=1;
    C6I4_CLR=0;
    C6I4_CLR=GP_I0_INT; 
}

//-------------------------------------------------------------
// set_msgs_base() set messages base address
//-------------------------------------------------------------
void set_msgs_base()
{
    long l=HPI_BASE_REG0;
    long a=l+(EMIF_MASK&EMIF_BASE0);

    if(a != DDR_MSGS_ADRS){
        PRINT0("setting msg base register");
        setC6xBase(DDR_MSGS_ADRS);
        taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    }
}

//-------------------------------------------------------------
// flush_msgs() initialize message pointers in C67 memory 
//-------------------------------------------------------------
void flush_msgs()
{
    set_msgs_base();
    
    VMEM(DDR_MSGS_READ)=0;
    VMEM(DDR_MSGS_WRITE)=0;
    VMEM(HOST_MSGS_READ)=0;
    VMEM(HOST_MSGS_WRITE)=0;

    //flush_hpi_write();
}

//-------------------------------------------------------------
// print_msg_addrs() show message queue addresses
//-------------------------------------------------------------
void print_msg_addrs()
{
    int host_msgs=DDR_MSGS_ADRS;
    int ddr_msgs=DDR_MSGS_ADRS+0x1000;
    
    printf("405->C67  start: 0x%-0.8X   end: 0x%-0.8X\n",
        host_msgs,host_msgs+MSG_BUFF_SIZE);
    printf("           read: 0x%-0.8X write: 0x%-0.8X\n",
        host_msgs+MSG_BUFF_SIZE,host_msgs+MSG_BUFF_SIZE+4);
    printf("C67->405  start: 0x%-0.8X   end: 0x%-0.8X\n",
        ddr_msgs,ddr_msgs+MSG_BUFF_SIZE);
    printf("           read: 0x%-0.8X write: 0x%-0.8X\n",
        ddr_msgs+MSG_BUFF_SIZE,ddr_msgs+MSG_BUFF_SIZE+4);
}

//-------------------------------------------------------------
// print_msg_stats() show message queue status
//-------------------------------------------------------------
void print_msg_stats(int n)
{
    int read,write,unread,i;
    
    set_msgs_base();

    read=VMEM(HOST_MSGS_READ);
    write=VMEM(HOST_MSGS_WRITE);
    unread=write>=read?write-read:write+MSG_BUFFER_SIZE-read;      
    
    printf("HOST->DDR total:%d read %d write %d unread:%d\n",
           total_host_msgs,read,write,unread);
           
    read=VMEM(DDR_MSGS_READ);
    write=VMEM(DDR_MSGS_WRITE);
    
    unread=write>=read?write-read:write+MSG_BUFFER_SIZE-read;      

    printf("DDR->HOST total:%d read %d write %d unread:%d\n",
           total_ddr_msgs,read,write,unread);   
}

//-------------------------------------------------------------
// print_ddr_msgs() dump DDR_* message buffer
//-------------------------------------------------------------
void print_ddr_msgs(int n)
{
    int read,write,unread,i;
    volatile int *mptr;
    volatile msg_struct msg;
    char str[256];
   
    set_msgs_base();

    read=VMEM(HOST_MSGS_READ);
    write=VMEM(HOST_MSGS_WRITE);
     
    unread=write>=read?write-read:write+MSG_BUFFER_SIZE-read;      

    printf("HOST->DDR total:%d read %d write %d unread:%d\n",
           total_host_msgs,read,write,unread);

    if(n==0){
        n=MSG_BUFFER_SIZE;
        if(n>total_host_msgs)
            n=total_host_msgs;
    }
    
    n=n>MSG_BUFFER_SIZE?MSG_BUFFER_SIZE:n;
        
    for(i=0;i<n;i++){
        write--;
        if(write<0)
            write=MSG_BUFFER_SIZE-1;
        mptr=MSG_PTR(HOST_MSGS,write);            
        msg=*((msg_struct *)mptr);
        
        ddr_string(msg.id,msg.arg1,msg.arg2,msg.arg3,str);
        printf("%-4d %s\n",i,str);
    }   
}

//-------------------------------------------------------------
// print_host_msgs() dump HOST_* message buffer
//-------------------------------------------------------------
void print_host_msgs(int n)
{
    int read,write,unread,i;
    volatile int *mptr;
    msg_struct msg;
    char str[256];
    
    set_msgs_base();

    read=VMEM(DDR_MSGS_READ);
    write=VMEM(DDR_MSGS_WRITE);
    
    unread=write>=read?write-read:write+MSG_BUFFER_SIZE-read;      

    printf("DDR->HOST total:%d read %d write %d unread:%d\n",
           total_ddr_msgs,read,write,unread);   

    if(n==0){
        n=MSG_BUFFER_SIZE;
        if(n>total_ddr_msgs)
            n=total_ddr_msgs;
    }
    n=n>MSG_BUFFER_SIZE?MSG_BUFFER_SIZE:n;
        
    for(i=0;i<n;i++){
        read--;
        if(read<0)
            read=MSG_BUFFER_SIZE-1;
        mptr=MSG_PTR(DDR_MSGS,read);
            
        msg=*((msg_struct *)mptr);
       
        host_string(msg.id,msg.arg1,msg.arg2,msg.arg3,str);
        printf("%-4d %s\n",i,str);
    }   
}

//-------------------------------------------------------------
// post_hmsg() send a host message to the DDR
//-------------------------------------------------------------
void post_hmsg(msg_struct* msg)
{
    int i,read, write,verify;
    volatile int *dptr,*mptr;
    int retries=0;
    int max_retries=100;

    if(acqstat&EXP_ERROR){
        PRINT0("!!! post_hmsg ERROR call while error active - aborting");
        return;
    }
        
#ifdef INSTRUMENT
	wvEvent(EVENT_POSTMSG_2DSP,NULL,NULL);
#endif

    semTake(pHostMsgMutex, WAIT_FOREVER);

    mptr=(int*)(msg);
        
    // first read sometimes returns the wrong value

    read=VMEM(HOST_MSGS_READ);
    write=VMEM(HOST_MSGS_WRITE);
    if(hpi_two_reads){
        read=VMEM(HOST_MSGS_READ);
        write=VMEM(HOST_MSGS_WRITE);
    }
  
    dptr=MSG_PTR(HOST_MSGS,write);
    
    if(ddr_debug & DDR_DEBUG_MPOST)
        PRINT3("post_hmsg read %d write %d data 0x%0.8X",read,write,(int)dptr);
        
    for (i=0; i<(MSG_DATA_SIZE)/sizeof(int); i++)
        dptr[i]=mptr[i];
        
    // possible fpga bug found 9/29/05 
    // - after a message is sent the C67 buffer sometimes contains an invalid 
    //   image of the message data
    // - this leads to various errors such as "locked buffer overwrite" etc.
    // possible causes
    // - maybe that the output buffer doesn't flush before this function exits ?
    // - but shouldn't the write-read-verify test for the write index insure this ?
    // work-arounds
    // - writing the message out twice seems to be effective.
    
    // 1. try writing to different address
    // 2. add incrementing count to 3rd argument in unlock 
    
    if(hpi_two_writes)
    for (i=0; i<(MSG_DATA_SIZE)/sizeof(int); i++) // out it goes again
        dptr[i]=mptr[i];

     write++;
    if(write==MSG_BUFFER_SIZE)
        write=0;

    if(write==read){  // ERROR: caught up with read pointer
        PRINT0("!!! post_hmsg ERROR: caught up with read pointer - aborting");
        host_error(DDR_ERROR_HMSG,write,scan_count);
        semGive(pHostMsgMutex);
        return;
    }
    VMEM(HOST_MSGS_WRITE)=write;   
    verify=VMEM(HOST_MSGS_WRITE);
    retries=0;
    while(verify != write && retries<max_retries){
        VMEM(HOST_MSGS_WRITE)=write;
        verify=VMEM(HOST_MSGS_WRITE);
        retries++;
    }
    if(retries>=max_retries){
        PRINT0("!!! post_hmsg ERROR: max_retries - aborting");
        host_error(DDR_ERROR_HPI,scan_count,retries);
    }
    else{
        send_hmsg_int();
        total_host_msgs++;
    }

    semGive(pHostMsgMutex);

#ifdef INSTRUMENT
	wvEvent(EVENT_POSTMSG_2DSP_CMPLT,NULL,NULL);
#endif
}
//-------------------------------------------------------------
// dmsg_reader_task()   read messages from C67
// - HW ISR or high priority vxWorks task
// - if task, unblocked by semGive(ddr_msg_sem) from hw isr (ddr_fpga_hw_isr)
// - places all unread DDR messages in C6 SDRAM IOBUF
//   onto message queue (msgsFromDDR)
//-------------------------------------------------------------
void dmsg_reader_task()
{
    msg_struct msg;
    volatile int * mptr;
    int i,j,*dptr,stat,read,write,nmsgs;
    unsigned int test;
    DSP_MSG hostdatamsg;
    FOREVER{
        semTake(ddr_msg_sem,WAIT_FOREVER);
        
#ifdef INSTRUMENT
	wvEvent(EVENT_READ_DSP_MSGS,NULL,NULL);
#endif
        if(acqstat&EXP_ERROR){
            PRINT0("!! dmsg_reader: ERROR call while error active");
            continue;
        }

        read=VMEM(DDR_MSGS_READ); 
        write=VMEM(DDR_MSGS_WRITE); 
        if(hpi_two_reads){
            read=VMEM(DDR_MSGS_READ);
            write=VMEM(DDR_MSGS_WRITE);
        }
        nmsgs=(read>write)?MSG_BUFFER_SIZE+write-read:write-read;

        // NOTE: it's quite possible to get multiple interrupts for the same message
        
        if(nmsgs==0) {
            if(ddr_debug & DDR_DEBUG_INTS){
                PRINT0("dmsg_reader_task redundant interrupt - exiting");
            }                               
            continue;
        }
        while(read != write){
            mptr=MSG_PTR(DDR_MSGS,read);
            dptr=(int*)(&msg);    
            for (i=0; i<(MSG_DATA_SIZE)/sizeof(int); i++)
                dptr[i]=mptr[i];

            if(ddr_debug & DDR_DEBUG_INTS){
                PRINT3("dmsg_reader_task id:%d read:%d write:%d",msg.id,read,write);
            }                    
             
            read++;
            if(read==MSG_BUFFER_SIZE)
                read=0;
                
            // make sure message is flushed from fpga write buffer
            
            VMEM(DDR_MSGS_READ)=read;
            test=VMEM(DDR_MSGS_READ);
            while(test != read){
                 VMEM(DDR_MSGS_READ)=read;
                 test=VMEM(DDR_MSGS_READ);
            }
            // if this queue is full then someone downsteam is blocking - catch the error there
            if(msgQSend(msgsFromDDR,(char*)&msg,sizeof(msg_struct),NO_WAIT,MSG_PRI_NORMAL)==ERROR)
                PRINT1("dmsg_reader_task: msg could not be sent (queue full) id=%d",msg.id);
        }
#ifdef INSTRUMENT
	wvEvent(EVENT_READ_DSP_MSGS_CMPLT,NULL,NULL);
#endif
    }   
}

//-------------------------------------------------------------
// post_int_hmsg() send a message to the DDR (int args)
//-------------------------------------------------------------
void post_int_hmsg(int id, int a1, int a2, int a3)
{
    msg_struct msg;    
    msg.id=id;
    msg.arg1=a1;
    msg.arg2=a2;
    msg.arg3=a3;
    post_hmsg(&msg);    
}

//=========================================================================
//  disableMSGInts: disable DDR message interrupts
//=========================================================================
void  disableMSGInts()
{
     BIT_OFF(IRQ_ENBL,IRQ_INT_MASK);
     C6I4_ENBL=0; // disable all INT4 interrupts
     C6I5_ENBL=0; // disable all INT5 interrupts
     C6I6_ENBL=0; // disable all INT6 interrupts
     C6I7_ENBL=0; // disable all INT7 interrupts
     GP_INT=0;    // clear global sw int register
     C6I4_CLR=0;
     C6I4_CLR=0xffffffff;
     C6I5_CLR=0;
     C6I5_CLR=0xffffffff;
     C6I6_CLR=0;
     C6I6_CLR=0xffffffff;
     C6I7_CLR=0;
     C6I7_CLR=0xffffffff;
     IRQ_CLR=0;
     IRQ_CLR=0xffffffff;
}

//=========================================================================
//  disableExpInts: disable experiment error interrupts
//=========================================================================
void  disableExpInts()
{
     BIT_OFF(IRQ_ENBL,ACQ_INT_MASK);
     IRQ_CLR=0;
     IRQ_CLR=ACQ_INT_MASK;
     C6I5_ENBL=0; // disable INT5 (blkrdy) interrupt
     C6I7_ENBL=0; // disable INT7 (error) interrupt
}

//=========================================================================
//  enableMSGInts: enable DDR message interrupts
//=========================================================================
void  enableMSGInts()
{
     C6I4_ENBL=GP_I0_INT;         // host->ddr gp_int bit 0
     BIT_ON(IRQ_ENBL,GP_I1_INT);
     BIT_ON(IRQ_ENBL,GP_I2_INT);
}

//=========================================================================
//  enableExpInts: enable ADC FIFO interrupts
//=========================================================================
void  enableExpInts()
{
     BIT_ON(IRQ_ENBL,ACQ_INT_MASK);
}

//############### parser interface functions ##########################
//=========================================================================
// ddr_set_exp() initialize the ddr
// caution: arguments must match up EXACTLY with those in 
//          DDRController::initializeExpStates 
//=========================================================================
int ddr_set_exp(int *args)
{
    int l1,l2,l3,nargs;
    double d1,d2;
    int i=0;
    unsigned int maxf=4;  // max fid queue size
    unsigned int maxs=100;  // max scan queue size
    unsigned int maxd=100;  // max data queue size
    int nfids=1;
    mode=0;
    
    acqstat=0;
    
    ddr_init();
    
    if(ddr_wait(EXP_READY,10)==0){
        PRINT0("wait failed: ddr_init");
        return 0;
    }
    nargs=args[i++];        // number of args to follow
    
    nfids=args[i++];
    databuffs = args[i++];
    databuffsize = args[i++];
    nf=args[i++];
    nfmod=args[i++];
    
    mode=args[i++];         // mode
    l1=args[i++];           // tp1
    l2=args[i++];           // tp2

    ddr_set_mode(mode,l1,l2);
    l1=args[i++];           // nacms
    l2=args[i++];           // dl
    l3=args[i++];           // al

    ddr_set_dims(l1,l2,l3);
    l1=args[i++];           // ascale
   
    ddr_set_xp(nf,nfmod,l1 );

    maxf=args[i++];         // acqs queue size
    maxs=args[i++];         // scan queue size
    maxd=args[i++];         // data queue size
        
    d1=unpackd(args, &i);   // cp
    ddr_set_cp(d1);
    
    d1=unpackd(args, &i);   // ca
    ddr_set_ca(d1);

    d1=unpackd(args, &i);   // cf
    ddr_set_cf(d1);

    d2=unpackd(args, &i);   // sd    
    d1=unpackd(args, &i);   // sr  
    
    ddr_set_sr(d1,d2);
    
    d1=unpackd(args, &i);   // sw1
    ddr_set_sw1(d1);
    
    d1=unpackd(args, &i);   // os1
    ddr_set_os1(d1);
    
    d1=unpackd(args, &i);   // xs1    
    ddr_set_xs1(d1);

    d1=unpackd(args, &i);   // fw1    
    ddr_set_fw1(d1);

    d1=unpackd(args, &i);   // sw2
    ddr_set_sw2(d1);
    
    d1=unpackd(args, &i);   // os2
    ddr_set_os2(d1);
    
    d1=unpackd(args, &i);   // xs2    
    ddr_set_xs2(d1);

    d1=unpackd(args, &i);   // fw2    
    ddr_set_fw2(d1);

    if((mode & MODE_FIDSIM) && (nargs-i>10)){
        int npeaks=args[i++];    // simpeaks
        double tmp;
        ddr_sim_peaks(npeaks);
        tmp=unpackd(args, &i);   // simdelay
        ddr_sim_rdly(tmp);
        tmp=unpackd(args, &i);   // simfscale
        ddr_sim_fscale(tmp);
        tmp=unpackd(args, &i);   // simnoise
        ddr_sim_noise (tmp);       
        tmp=unpackd(args, &i);   // simlw
        ddr_sim_lw(tmp);
        tmp=unpackd(args, &i);   // simascale
        ddr_sim_ascale(tmp);
    }
    else
        ddr_sim_peaks(0);

    enable_adc_ovf=(mode&MODE_ADC_OVF)?1:0;
            
    if(mode & MODE_DITHER_OFF)
        set_dither(0);  // disable dithering
    else
        set_dither(1);  // enable dithering
            
    if(mode & MODE_ADC_DEBUG)
        setADCDebug(1);  // use "fake" fpga generated data (saw-tooth pattern)
    else
        setADCDebug(0);  // use "real" DDR-A acquired data

    ddr_set_filter();

    if(mode & PF_QUAD){
        i=m1/2;
        i=i>100?100:i;
        ddr_set_pfaves(i);  // use quadratic fit for prefid phase 
    }
    else
        ddr_set_pfaves(1);  // use first point only for prefid phase 

    if(mode&MODE_DMA)
        ddrUsePioFlag = 0;
    else
        ddrUsePioFlag = 1;

    ddr_init_exp(maxf,maxs,maxd);

    // give the C67 time to calculate fake data points if
    // running in simulation mode (e.g. expargs=-s)
    // - set max time to ~15 s (1/2 sys init timout max)
    if(ddr_wait(EXP_INIT,1000)==0){ 
        PRINT0("wait failed: ddr_init_exp");
        host_error(DDR_ERROR_INIT, 1, 0);
        return 0;
    }
    return 1;   
}

//=========================================================================
// ddr_set_scan() initialize the scan
// function called from ddrFifo.c (but not in current use)
//=========================================================================
int ddr_set_scan(int *args)
{   
    return 1;   
}

//###############  405 to C67 message functions ##########################
//=========================================================================
// ddr_init() initialize the ddr
// message id : DDR_INIT
//=========================================================================
void ddr_init()
{
    unsigned int test;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_INIT");
    disableMSGInts();
    flush_msgs();
    //set_msgs_base();
    // semGive(pHostMsgMutex);
    enableMSGInts();
    acqstat=0;
    total_host_msgs=total_ddr_msgs=0;
    post_int_hmsg(DDR_INIT, 0, 0, 0);       
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
}

//=========================================================================
// ddr_set_sr() set ADC sampling rate and delay (max = 3.1875 us)
// message id : DDR_SET_SR
//=========================================================================
void ddr_set_sr(double sa, double x)
{
    u64 tmp;
 
#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_SR,NULL,NULL);
#endif
    sr=sa*1e6; // convert units from MHz to Hz
    if(x)
        sd=x;
    else  
        sd=ddr_calc_delay();
    tmp.d=sr;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT2("DDR_SET_SR sa:%d Hz sd:%1.2f us",(int)sr,sd*1e6);
    post_int_hmsg(DDR_SET_SR, tmp.i.h, tmp.i.l,0); 
    tmp.d=sd;
          
    post_int_hmsg(DDR_SET_SD, tmp.i.h, tmp.i.l,0);       

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_SR_CMPLT,NULL,NULL);
#endif
}

//=========================================================================
// ddr_set_sw1() set sw1
// message id : DDR_SET_SW
//=========================================================================
void ddr_set_sw1(double sw)
{
    u64 tmp;
#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_SW1,NULL,NULL);
#endif
    sw1=sw;
    tmp.d=sw;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_SW1 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_SW, tmp.i.h, tmp.i.l, 1);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_SW1_CMPLT,NULL,NULL);
#endif
}

//=========================================================================
// ddr_set_sw1() set sw2
// message id : DDR_SET_SW
//=========================================================================
void ddr_set_sw2(double sw)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_SW2,NULL,NULL);
#endif

    sw2=sw;
    tmp.d=sw;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_SW2 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_SW, tmp.i.h, tmp.i.l, 2);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_SW2_CMPLT,NULL,NULL);
#endif
}

//=========================================================================
// ddr_set_os1() set os1
// message id : DDR_SET_OS
//=========================================================================
void ddr_set_os1(double os)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_OS1,NULL,NULL);
#endif

    os1=os;
    tmp.d=os;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_OS1 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_OS, tmp.i.h, tmp.i.l, 1);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_OS1_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_os2() set os2
// message id : DDR_SET_OS
//=========================================================================
void ddr_set_os2(double os)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_OS2,NULL,NULL);
#endif

    os2=os;
    tmp.d=os;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_OS2 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_OS, tmp.i.h, tmp.i.l, 2);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_OS2_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_xs1() set xs1
// message id : DDR_SET_OS
//=========================================================================
void ddr_set_xs1(double xs)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_XS1,NULL,NULL);
#endif

    xs1=xs;
    tmp.d=xs;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_XS1 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_XS, tmp.i.h, tmp.i.l, 1);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_XS1_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_xs2() set xs2
// message id : DDR_SET_OS
//=========================================================================
void ddr_set_xs2(double xs)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_XS2,NULL,NULL);
#endif

    xs2=xs;
    tmp.d=xs;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_XS2 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_XS, tmp.i.h, tmp.i.l, 2);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_XS2_CMPLT,NULL,NULL);
#endif
}

//=========================================================================
// ddr_set_fw1() set fw1
// message id : DDR_SET_FW
//=========================================================================
void ddr_set_fw1(double fw)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_FW1,NULL,NULL);
#endif

    fw1=fw;
    tmp.d=fw;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_FW1 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_FW, tmp.i.h, tmp.i.l, 1);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_FW1_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_fw2() set fw2
// message id : DDR_SET_FW
//=========================================================================
void ddr_set_fw2(double fw)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_FW2,NULL,NULL);
#endif

    fw2=fw;
    tmp.d=fw;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_FW2 %-8d",(int)tmp.d);
    post_int_hmsg(DDR_SET_FW, tmp.i.h, tmp.i.l, 2);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_FW2_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_rp: set receiver phase
// message id : DDR_SET_RP
//=========================================================================
void ddr_set_rp(double p)
{    
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_RP,NULL,NULL);
#endif

    tmp.d=p;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_RP p:%d",(int)p);
    post_int_hmsg(DDR_SET_RP,tmp.i.h, tmp.i.l, 0);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_RP_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_cp: set const receiver phase
// message id : DDR_SET_CP
//=========================================================================
void ddr_set_cp(double p)
{    
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_CP,NULL,NULL);
#endif

    tmp.d=p;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_CP p:%d",(int)p);
    post_int_hmsg(DDR_SET_CP,tmp.i.h, tmp.i.l, 0);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_CP_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_rf: set receiver frequency offset
// message id : DDR_SET_RF
//=========================================================================
void ddr_set_rf(double f)
{
    u64 tmp;

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_RF,NULL,NULL);
#endif

    tmp.d=f;

    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_RF f:%d Hz",(int)f);
    post_int_hmsg(DDR_SET_RF,tmp.i.h, tmp.i.l, 0);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_RF_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_cf: set const. frequency offset
// message id : DDR_SET_CF
//=========================================================================
void ddr_set_cf(double f)
{
    u64 tmp;
    tmp.d=f;

    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_CF f:%d Hz",(int)f);
    post_int_hmsg(DDR_SET_CF,tmp.i.h, tmp.i.l, 0);

}

//=========================================================================
// ddr_set_ca: set const. amplitude scale
// message id : DDR_SET_CA
//=========================================================================
void ddr_set_ca(double a)
{
    u64 tmp;
    tmp.d=a;

    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SET_CA a:%d %%",(int)a*100);
    post_int_hmsg(DDR_SET_CA,tmp.i.h, tmp.i.l, 0);

}

//=========================================================================
// ddr_set_mode() set mode and options
// message id : DDR_SET_MODE
//=========================================================================
void ddr_set_mode(int m, int b, int s)
{
    mode=m;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_SET_MODE mode:0x%-0.8X tp1:%d tp2:%d",m,b,s);
    post_int_hmsg(DDR_SET_MODE, mode, b, s);
}

//=========================================================================
// ddr_set_dims() set data arrays dimensions
// message id : DDR_SET_DIMS
//=========================================================================
void ddr_set_dims(int n, int d, int a)
{

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_DIMS,NULL,NULL);
#endif

    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_SET_DIMS nacms:%d dl:%d al:%d ",n,d,a);

    dl=d;
    al=a;
    nacms=n;
    post_int_hmsg(DDR_SET_DIMS, n, d,  a);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_DIMS_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_xp() set transfer policy     
// message id : DDR_SET_XP
//=========================================================================
void ddr_set_xp(int n, int m, int x)
{

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_XP,NULL,NULL);
#endif

    nf=n;
    nfmod=m;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_SET_XP nf:%d nfmod:%d ascale:%d",nf,nfmod,x);
    post_int_hmsg(DDR_SET_XP, nf, nfmod, x);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_XP_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_cycle() set acq. cycle duty factor
// message id : DDR_SIM_CYCLE
//=========================================================================
void ddr_set_cycle(int on, int off, int zeros)
{

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_CYCLE,NULL,NULL);
#endif

    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_SIM_CYCLE on:%d off:%d zeros:%d",on,off,zeros);
    post_int_hmsg(DDR_SIM_CYCLE, on, off,  zeros);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_CYCLE_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_init_exp() initialize acquisition
// message id : DDR_INIT_EXP
//=========================================================================
void ddr_init_exp(int af,int sf,int df)
{
    //ddr_set_filter();
    push_count=0;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_INIT_EXP acq_fifo_size:%d scan_fifo_size:%d data_fifo_size:%d",af,sf,df);
    post_int_hmsg(DDR_INIT_EXP, af, sf, df);
}

//=========================================================================
// ddr_start_exp() start acquisition
// message id : DDR_START_EXP
//=========================================================================
void ddr_start_exp(int m)
{

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_STARTEXP,NULL,NULL);
#endif

    SEQUENCE_TEST_INIT;
    
    if(acqstat & EXP_RUNNING){
        PRINT0("WARNING: ddr_start_exp called while ddr is running");
        return;
    }
    scan_count=fid_count=fid_ct=0;
    ddr_il_incr=0;
    data_count=0;
    adc_ovf_count=0;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_START_EXP mode=0x%x",m);
    post_int_hmsg(DDR_START_EXP, m, 0, 0);

    if(ddr_wait(EXP_RUNNING,0)==0){   // N.B: set wait time to 0
        printf("wait failed %s\n","ddr_start_exp");
        return;
    }
#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_STARTEXP_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_stop_exp() stop acquisition
// message id : DDR_STOP_EXP
//=========================================================================
void ddr_stop_exp(int i)
{
    if(!(acqstat & EXP_READY)){
        PRINT0("WARNING: ddr_stop_exp called before ddr_init (aborting)");
        return;
    }
#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_STOPEXP,NULL,NULL);
#endif

	if(ddr_run_mode==STAND_ALONE){     
        disableExpInts();
		stopOTF();
	}

    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_STOP_EXP 0x%0.8X",i);
    post_int_hmsg(DDR_STOP_EXP, i,  0,  0);

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_STOPEXP_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_push_fid() push new fid onto "fid queue"
// message id : DDR_PUSH_FID
//=========================================================================
void ddr_push_fid(int id, int ac)
{
#ifdef INSTRUMENT
	//wvEvent(EVENT_DSP_PUSHFID,NULL,NULL);
#endif
    if((ddr_debug & DDR_DEBUG_SCANS) || (ddr_debug & DDR_DEBUG_DMSGS))
        PRINT2("DDR_PUSH_FID fid:%d ac:%d",id,ac);
    post_int_hmsg(DDR_PUSH_FID,id,ac,0);
#ifdef INSTRUMENT
	//wvEvent(EVENT_DSP_PUSHFID_CMPLT,NULL,NULL);
#endif
}

//=========================================================================
// ddr_push_scan() push new scan on "scan queue"
// message id : DDR_PUSH_SCAN
//=========================================================================
void ddr_push_scan(int src, int status)
{
#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_PUSHSCAN,NULL,NULL);
#endif
    TEST_ORDER(push,"DDR_PUSH_SCAN");

    push_count++;
    if(status & EXP_LAST_SCAN){
       PRINT2("DDR_PUSH_SCAN id:%d LAST count:%d",src,push_count);
    }
    else if(ddr_debug & DDR_DEBUG_DMSGS){
    
    //else if((ddr_debug & DDR_DEBUG_SCANS) || (ddr_debug & DDR_DEBUG_DMSGS)){
       PRINT2("DDR_PUSH_SCAN id:%d count:%d",src,push_count);
    }
    post_int_hmsg(DDR_PUSH_SCAN, src, status,0);

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_PUSHSCAN_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_set_acm: set accumulation buffer (dst+=src)
// message id : DDR_SET_ACM
//=========================================================================
void ddr_set_acm(int src, int dst, int nt)
{

    TEST_ORDER(acm,"DDR_SET_ACM");
#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_ACM,NULL,NULL);
#endif

//    if((ddr_debug & DDR_DEBUG_SCANS) ||(ddr_debug & DDR_DEBUG_DMSGS))
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_SET_ACM src:%d dst:%d nt:%d",src,dst,nt);
    post_int_hmsg(DDR_SET_ACM, src, dst, nt);

#ifdef INSTRUMENT
	wvEvent(EVENT_SET_DSP_ACM_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_unlock_acm: unlock accumulation buffer
// message id : DDR_UNLOCK
//=========================================================================
void ddr_unlock_acm(int src, int mode)
{
   TEST_ORDER(unlock,"DDR_UNLOCK");

#ifdef INSTRUMENT
	wvEvent(EVENT_UNLOCK_DSP_ACM,NULL,NULL);
#endif

    if((ddr_debug & DDR_DEBUG_SCANS) ||(ddr_debug & DDR_DEBUG_DMSGS))
        PRINT1("DDR_UNLOCK src:%d",src);
    post_int_hmsg(DDR_UNLOCK, src, mode, 0);

#ifdef INSTRUMENT
	wvEvent(EVENT_UNLOCK_DSP_ACM_CMPLT,NULL,NULL);
#endif

}

//=========================================================================
// ddr_version() get ddr version
// message id : DDR_VERSION
//=========================================================================
void ddr_version()
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_VERSION");
    post_int_hmsg(DDR_VERSION,  0,  0,  0);
}

//=========================================================================
// ddr_test() debug test
// message id : DDR_TEST
//=========================================================================
void ddr_test(int id, int a, int b)
{
    if(!(acqstat & EXP_READY)){
        PRINT0("WARNING: ddr_test called before ddr_init");
    }
    BIT_OFF(acqstat,EXP_TEST);
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT3("DDR_TEST id:%d a:%d b%d",id,a,b);
    post_int_hmsg(DDR_TEST, id, a, b);      
}

//=========================================================================
// ddr_test() debug test
// message id : DDR_TEST
//=========================================================================
void ddr_mem_test(int a, int b)
{
    if(!(acqstat & EXP_READY)){
        PRINT0("WARNING: ddr_test called before ddr_init");
    }
    BIT_OFF(acqstat,EXP_TEST);
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT2("DDR_TEST_DATA a:%d b%d",a,b);
    post_int_hmsg(DDR_TEST, DDR_TEST_DATA, a, b);      
}

//=========================================================================
// ddr_sim_peaks() configure simulated fid
// message id : DDR_SIM_PEAKS
//=========================================================================
void ddr_sim_peaks(int id)
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SIM_PEAKS %d",id);
    post_int_hmsg(DDR_SIM_PEAKS, id, 0, 0);     
}

//=========================================================================
// ddr_sim_ascale() configure simulated fid
// message id : DDR_SIM_ASCALE
//=========================================================================
void ddr_sim_ascale(double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SIM_ASCALE %d %%",(int)f*100);
    post_int_hmsg(DDR_SIM_ASCALE, tmp.i.h, tmp.i.l, 0);     
}

//=========================================================================
// ddr_sim_fscale() configure simulated fid
// message id : DDR_SIM_FSCALE
//=========================================================================
void ddr_sim_fscale(double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SIM_FSCALE %d %%",(int)f*100);
    post_int_hmsg(DDR_SIM_FSCALE, tmp.i.h, tmp.i.l, 0);     
}

//=========================================================================
// ddr_sim_noise() configure simulated fid
// message id : DDR_SIM_NOISE
//=========================================================================
void ddr_sim_noise(double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SIM_NOISE %d %%",(int)f*100);
    post_int_hmsg(DDR_SIM_NOISE, tmp.i.h, tmp.i.l, 0);      
}

//=========================================================================
// ddr_xin_info() input data info request 
// ddr  msg id : DDR_XIN_INFO
// host msg id : HOST_XIN_INFO
//=========================================================================
void ddr_xin_info()
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_XIN_INFO");
    post_int_hmsg(DDR_XIN_INFO,0,0,0);      
}

//=========================================================================
// ddr_sim_rdly() configure simulated fid
// message id : DDR_SIM_RDLY
//=========================================================================
void ddr_sim_rdly(double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SIM_RDLY %d us",(int)f*1e6);
    post_int_hmsg(DDR_SIM_RDLY, tmp.i.h, tmp.i.l, 0);       
}

//=========================================================================
// ddr_sim_lw() configure simulated fid
// message id : DDR_SIM_LW
//=========================================================================
void ddr_sim_lw(double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_SIM_LW %d Hz",(int)f);
    post_int_hmsg(DDR_SIM_LW, tmp.i.h, tmp.i.l, 0);     
}

//=========================================================================
// ddr_peak_freq() configure simulated fid peak
// message id : DDR_PEAK_FREQ
//=========================================================================
void ddr_peak_freq(int id, double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT2("DDR_PEAK_FREQ peak:%d freq:%d",id,(int)f);
    post_int_hmsg(DDR_PEAK_FREQ, id, tmp.i.h, tmp.i.l);     
}

//=========================================================================
// ddr_peak_ampl() configure simulated fid peak
// message id : DDR_PEAK_AMPL
//=========================================================================
void ddr_peak_ampl(int id, double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT2("DDR_PEAK_FREQ peak:%d ampl:%d %%",id,(int)f*100);
    post_int_hmsg(DDR_PEAK_AMPL, id, tmp.i.h, tmp.i.l);     
}

//=========================================================================
// ddr_peak_width() configure simulated fid peak
// message id : DDR_PEAK_WIDTH
//=========================================================================
void ddr_peak_width(int id, double f)
{
    u64 tmp;
    tmp.d=f;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT2("DDR_PEAK_FREQ peak:%d width:%d %%",id,(int)f*100);
    post_int_hmsg(DDR_PEAK_WIDTH, id, tmp.i.h, tmp.i.l);        
}

//=========================================================================
// ddr_get_count() advance ddr watchdog count value (return old value)
// message id : DDR_TEST
//=========================================================================
int ddr_get_count()
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_TEST id:DDR_TEST_ACTIVE");
    post_int_hmsg(DDR_TEST, DDR_TEST_ACTIVE, 0, 0);
    return watchdog_count;  
}

//=========================================================================
// ddr_set_fid() set ddr host fid
// message id : DDR_SETFID
//=========================================================================
void ddr_set_fid(int a, int b, int c)
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_SETFID");
    post_int_hmsg(DDR_SETFID,a, b, c);      
}

//=========================================================================
// ddr_set_pfaves() set number of prefid points to average
// message id : DDR_PFAVES
//=========================================================================
void ddr_set_pfaves(int n)
{
    if(n==0)
        pfaves=m1/2; 
    else  
        pfaves=n;
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT1("DDR_PFAVES %d",pfaves);
    post_int_hmsg(DDR_PFAVES,pfaves, 0, 0);      
}

//=========================================================================
// ddr_set_blink: enable/disable ddr led blinker
//=========================================================================
void ddr_set_blink(int a)
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_DEBUG:DDR_BLINK");
    post_int_hmsg(DDR_DEBUG,DDR_BLINK, a, 0);       
}

//=========================================================================
// ddr_set_auto: set auto test
//=========================================================================
void ddr_set_auto(int tr, int ts)
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT2("DDR_DEBUG: DDR_AUTOP %d %d",tr,ts);
    post_int_hmsg(DDR_DEBUG,DDR_AUTOP, tr, ts);     
}

//=========================================================================
// ddr_clr_debug: clear all debug flags
//=========================================================================
void ddr_clr_debug()
{
    if(ddr_debug & DDR_DEBUG_DMSGS)
        PRINT0("DDR_DEBUG: NONE");
    post_int_hmsg(DDR_DEBUG,DDR_NONE, 0, 0);        
}

//###############  HOST message handler functions ########################
//=========================================================================
// abort_exp: stop current experiment as a result of a fatal error
//=========================================================================
void abort_exp(int id)
{
    // maps ddr error id into HARDERRORS in errorcodes.h starting at 940
    if(ddr_debug & DDR_DEBUG_ERRS)
        PRINT0("ABORTING EXP");
    BIT_ON(acqstat,EXP_ERROR);
    disableExpInts();
    SW_OUTPUTS=0;
    if(ddr_run_mode!=STAND_ALONE)
        sendException(HARD_ERROR, HDWAREERROR+id, 0,0,NULL);
    else
        ddr_stop_exp(EXP_ERROR);
}

//=========================================================================
// send_warning: send warning message to vnmr
//=========================================================================
void send_warning(int type, int id)
{       
    if(ddr_run_mode!=STAND_ALONE)
    {
       /* DPRINT(-1,"send_warning: ADCOVER\n"); */
       /* Send ADC Overflow warning only once per FID */
    //   if (checkAdcWarning(id))
    //   {
           sendException(WARNING_MSG, WARNINGS + type, id,0,NULL);
    //   }
    }
}

//=========================================================================
// host_error() ddr errors central
// message id : HOST_ERROR
//   - HDWAREERROR and DDR_DSP.. error ids defined in errorcodes.h (vnmr)
//   - corresponding message strings defined in table acqerrmsges.h (vnmr)
//   - table(acqerrmsges.h) = 900(HDWAREERROR) + 40 + id(ddr_symbols.h)
//=========================================================================
void host_error(int id, int arg1, int arg2)
{

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_HOSTERR+id,NULL,NULL);
#endif

    switch(id){
    
    // Errors sent by the C67
    
    case DDR_ERROR_INIT:  // <C67> ddr_init not called before ddr_init_exp
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT0("ERROR: "DDR_ERROR_INIT_STR"");
        abort_exp(DDR_DSP_NOT_INIT);
        break;

    case DDR_ERROR_ACQ:  // <C67> re-entrant blkrdy interrupt or other error
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_ACQ_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_ACQ_ERR);
        break;

    case DDR_ERROR_PROC: // <C67> unexpected fatal processing error
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT1("ERROR: "DDR_ERROR_PROC_STR" in scan %d ",arg1);
        abort_exp(DDR_DSP_PROC_ERR);
        break;

    case DDR_ERROR_SCAN_QUE1: // <C67> scan queue failure
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_SCAN_QUE1_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_SCANQ_ERR);
        break;

    case DDR_ERROR_SCAN_QUE2: // <C67> scan queue failure
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_SCAN_QUE2_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_SCANQ_POP_ERR);
        break;

    case DDR_ERROR_MEM:   // <C67> ddr memory error
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT1("ERROR: "DDR_ERROR_MEM_STR" [%d]",arg1);
        abort_exp(DDR_DSP_MEM_ERR);
        break; 

    case DDR_ERROR_BADID: // <C67> fpga - ddr software checksum mismatch
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_BADID_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_BADID_ERR);
        break; 

    case DDR_ERROR_DATA_QUE:  // <C67> data queue error
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_DATA_QUE_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_DATAQ_ERR);
        break; 

    case DDR_ERROR_LOCK:  // <C67> attempted locked buffer overwrite
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_LOCK_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_LOCK_ERR);
        break; 

    case DDR_ERROR_NP:   // <C67> wrong dl collected in acquisition
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_NP_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_NP_ERR);
        break; 

    case DDR_ERROR_DMQ_C67: // <C67> unknown message error
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_DMQ_C67_STR" %d %d",arg1,arg2);
        abort_exp(DDR_DSP_BAD_MSG);
        break; 

    case DDR_ERROR_DMSG:  // <DDR_Init> DMSG queue full
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_DMSG_STR" %d scan %d",arg1,arg2);
        abort_exp(DDR_DSP_C67Q_OVRFLW);
        break; 

    // Warnings sent by the C67
    
    case DDR_ERROR_MAXD:  // short data max exceeded in signal averaging
       if (adc_ovf_count>0)   // send new warning once for each new fid
            break;
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("WARNING: "DDR_ERROR_MAXD_STR" acm %d value %d",arg1,arg2);
        adc_ovf_count++;
        send_warning(DATAOVF,arg1);  
        //sendException(WARNING_MSG, WARNINGS + ADCOVER, 0,0,NULL);        
        break; 

    case DDR_ERROR_DCLIP: // <C67> DDR data clip
        if (adc_ovf_count>0)   // send new warning once for each new fid
            break;
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT4("WARNING: "DDR_ERROR_DCLIP_STR" fid %d scan %d msgfid %d ovfcnt %d",
                 fid_count,fid_ct,arg2,adc_ovf_count);
        adc_ovf_count++;
        send_warning(ADCOVER,arg2);
        break; 

    // Errors generated by the 405 (DDR_Acq)

    case DDR_ERROR_HMSG:  // <DDR_Acq> HMSG queue full
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_HMSG_STR" %d scan %d",arg1,arg2);
        abort_exp(DDR_DSP_405Q_OVRFLW);
        break; 
 
    case DDR_ERROR_HPI:  // <DDR_Acq> HPI max retries exceeded
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_HPI_STR" %d scan %d",arg1,arg2);
        abort_exp(DDR_DSP_HPI_TIMEOUT);
        break; 

    // Errors sent by the 405 (DDR_Init hw isr)

    case DDR_ERROR_OVF: // <DDR_Init> DDR output fifo overflow
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT1("ERROR: "DDR_ERROR_OVF_STR" scan %d",arg1);
        if(ddr_run_mode==STAND_ALONE)
            abort_exp(DDR_DSP_FIFO_OVRFLW);
        break; 

    case DDR_ERROR_UNF: // <DDR_Init> DDR output fifo underflow
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT2("ERROR: "DDR_ERROR_UNF_STR" fid %d ct %d",arg1,arg2);
        if(ddr_run_mode==STAND_ALONE)
            abort_exp(DDR_DSP_FIFO_UNDERFLW);
        break; 

    case DDR_ERROR_ADF_OVF: // <DDR_Init> DDR data fifo overflow
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT1("ERROR: "DDR_ERROR_ADF_OVF_STR" scan %d",arg1);
        abort_exp(DDR_DSP_ADC_OVRFLW);
        break; 

    // Other errors

    case DDR_ERROR_SW:   // <DDR_Acq> unexpected sw behavior trap
        if(ddr_debug & DDR_DEBUG_ERRS)
            PRINT1("ERROR: "DDR_ERROR_SW_STR" id %d ",arg1);
        if(arg2>0) // fatal
            abort_exp(DDR_DSP_SW_TRAP);
        break;

    default: // unknown message id error (??)
        if(ddr_debug & DDR_DEBUG_ERRS)
           PRINT2("ERROR: DDR_UNKNOWN id %d %d",id,arg1);
        abort_exp(DDR_DSP_UNKNOWN_ERR);
        break; 
    }

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_HOSTERR,NULL,NULL);
#endif

}

//=========================================================================
// host_init() ready message (initial state)
// message id : HOST_INIT
//=========================================================================
void host_init(int status)
{
    acqstat=status;
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT1("HOST_INIT status:0x%0.8X",status);
}

//=========================================================================
// host_reset() reset message acknowledge
// message id : HOST_RESET
//=========================================================================
void host_reset(int status)
{
    acqstat=status;
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT1("HOST_RESET status:0x%0.8X",status);
}

//=========================================================================
// host_end_exp() end of exp event message
// message id : HOST_END_EXP
//=========================================================================
//void host_end_exp(int status, int cpu, int runtm)
void host_end_exp(int cpu, int acq, int acm)
{
    double s;
    double dur1=0;
    double dur2=0;
    int at=1e6*nx1/sr;
    
    //PRINT3("HOST_END_EXP CPU:%d ACQ:%d ACM:%d %% ",cpu,acq,acm);
    DPRINT5(-3,"HOST_END_EXP AT:%d CPU:%d us ALL:%d ACQ:%d ACM:%d %% ",at,cpu,100*cpu/at,acq,acm);
    dur1=DUR1;    
    dur2=DUR2;    
    s=(dur2*(double)(1<31)+dur1)/80.0e6;
    //PRINT2("run time: %d msecs scans: %d", (int)(1000.0*s),scan_count);
    DPRINT2(-3,"run time: %d msecs scans: %d", (int)(1000.0*s),scan_count);
 
    //acqstat=status;
    
    BIT_OFF(acqstat,EXP_RUNNING);
   
    disableExpInts();
}

//=========================================================================
// host_data_info part of data ready message
// message id : HOST_DATA_INFO
// - need additional host message to pass tt and scale information 
//=========================================================================
void host_data_info(int tt, int scale, int unassigned)
{
    data_tt=tt;
    data_scale=scale;
}

//=========================================================================
// data_info() data buffer info
//=========================================================================
void get_data_info(int src, float **data, int *size, int *dtype)
{
    int dmode=mode&DATA_TYPE;
    int stride;
    
    stride=(dmode==DATA_DOUBLE)?4:2;       // word size in C67 memory
    *size=nfmod*dl;
   
    *data=(float*)DDR_DATA+stride*dl*src*nfmod;
    *dtype=(mode&DATA_PACK)?DATA_SHORT:DATA_FLOAT;
}

//=========================================================================
// set_data_info() set data buffer info based on np
//  Used by testing routines only     1/28/07  GMB
//=========================================================================
void set_data_info(int np)
{
    mode= DATA_TYPE;
    nfmod = 1;
    dl = np / 2;
}

//=========================================================================
// host_data() data ready message
// message id : HOST_DATA
// - constructs a data_msg and places it on a message queue
//=========================================================================
void host_data(int src, int id, int cs, int stat)
{
    data_msg msg;
    DSP_MSG  dmsg;    
    float *adrs;
    int xp,dtype;
    int dsize=mode&DATA_PACK?4:8;

    get_data_info(src, &adrs, &xp, &dtype);
       
    TEST_ORDER(data,"HOST_DATA");
    
    data_count++;
    
    if((ddr_debug & DDR_DEBUG_SCANS) ||(ddr_debug & DDR_DEBUG_HMSGS))
        PRINT6("HOST_DATA src:%d dl:%d bytes:%d adr:0x%0.8X id:%d transfer#:%d",
             src,xp,xp*dsize,(int)adrs,id,data_count);
    if(stat)
        PRINT6("END_DATA  src:%d dl:%d bytes:%d adr:0x%0.8X id:%d transfers:%d",
             src,xp,xp*dsize,(int)adrs,id,data_count);
    
    if(pDspDataRdyMsgQ){
        dmsg.srcIndex=(long)src;    
        dmsg.dataAddr=(unsigned long)adrs;    
        dmsg.np=(unsigned long)xp;
        dmsg.tag=(unsigned long)id;
        dmsg.crc32chksum=(unsigned long)cs;
        if(msgQSend(pDspDataRdyMsgQ,(char*)&dmsg,sizeof(DSP_MSG),NO_WAIT,MSG_PRI_NORMAL)==ERROR){
            logMsg("\n!! ERROR host_data: could not post data ready msg (pDspDataRdyMsgQ full) tag:%d\n",
                dmsg.tag,0,0,0,0,0);
            sendException(HARD_ERROR, HDWAREERROR + DATA_UPLINK_ERR, 0,0,NULL);
            taskDelay(calcSysClkTicks(34)); // delay time for the error to kick in (avoids multiple error calls)
        }
    }
    if(pTestDataRdyMsgQ){
        msg.src=(unsigned int)src;    
        msg.adrs=(unsigned int)adrs;
        msg.np=(unsigned int)xp*dl;
        msg.id=(unsigned int)id;    
        msg.cs=(unsigned int)cs;
        msg.status=(unsigned int)(dtype|stat);
        msgQSend(pTestDataRdyMsgQ,(char*)&msg,sizeof(data_msg),WAIT_FOREVER,MSG_PRI_NORMAL);
    }    
}

//=========================================================================
// host_ss_req()  snapshot ready message
// message id : HOST_SS_REQ
//=========================================================================
void host_ss_req(int acm, int ct, int tt)
{
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT3("HOST_SS_REQ acm:%d ct:%d tt:%d ",acm,ct,tt);
}

//=========================================================================
// host_init_exp() experiment initialization ready message
// message id : HOST_INIT_EXP
//=========================================================================
void host_init_exp(int status,int m,int a)
{
    acqstat=status;
    mode=m;
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT3("HOST_INIT_EXP status:0x%0.8X mode:0x%0.8X nacqs:%d",status,m,a);
}

//=========================================================================
// host_xin_info() return prefid data pointers
// message id : HOST_XIN_INFO
//=========================================================================
void host_xin_info(int x, int b, int n)
{
    xin=x;
    prefid=b;
    pfsize=n;
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT3("HOST_XIN_INFO X:0x%0.8X  B:0x%0.8X  PFSIZE:%d",x,b,n);
}

//=========================================================================
// host_start_exp() experiment start ready message
// message id : HOST_START_EXP
//=========================================================================
void host_start_exp(int status)
{
    acqstat=status;
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT1("HOST_START_EXP status:0x%0.8X",status);
    if(acqstat & EXP_RUNNING)
        enableExpInts();
}

//=========================================================================
// host_stop_exp() stop request acknowledge
// message id : HOST_STOP_EXP
//=========================================================================
void host_stop_exp(int status,int scan)
{
    acqstat=status;
    //stopOTF();
    //if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT2("HOST_STOP_EXP status:0x%0.8X scan:%d",status,scan);
    SEQUENCE_TEST_PRINT;
}

//=========================================================================
// host_resume_exp() restart acquisition
// message id : HOST_RESUME_EXP
//=========================================================================
void host_resume_exp(int status)
{
    acqstat=status;   
    if(ddr_debug & DDR_DEBUG_HMSGS)
        PRINT1("HOST_RESUME_EXP status:0x%0.8X",status);
}
//=========================================================================
// host_test() debug test 
// message id : HOST_TEST
//=========================================================================
void host_test(int id, int b, int c)
{
    test_id=id;
   
    switch(id){
    case DDR_TEST_ACTIVE:
        watchdog_count=b;
        break;
    case DDR_TEST_DATA:
        PRINT2("DDR_TEST_DATA errors %d:%d",c,b); 
        break;
    case DDR_TEST_LIMITS:
        PRINT2("DDR_DATA_LIMITS min:%d max:%d",c,b); 
        break;
    default:
        PRINT4("HOST_TEST id:%d b:%d (0x%-0.8X) c:%d",id,b,b,c); 
        test_value1=b;
        test_value2=c;
        break;
    }
    BIT_ON(acqstat,EXP_TEST);
}

//=========================================================================
// host_version() ddr version
// message id : HOST_VERSION
//=========================================================================
void host_version(int a, int b, int c)
{
    PRINT2("DDR OS version:%d checksum:%d",a,b);
    if(b != c)
        host_error(DDR_ERROR_BADID,b,c);        
}

//=========================================================================
// host_unknown() non-standard message 
// message id : default
//=========================================================================
void host_unknown(int id, int a, int b)
{
    if(ddr_debug & DDR_DEBUG_ERRS)
        PRINT3("HOST_UNKNOWN id:%d a:%d a:%d",id,a,b);
    abort_exp(DDR_DSP_UNKNOWN_ERR);  
}

//###############  DDR message reader functions ########################

//=========================================================================
// initDDRmsgs: DDR message initialization function
// 1. First call sets up vxWorks objects to handle C67 message passing
//   - intConnect fpga_irq_hw_isr 
//      o responds to new message interrupts from C67
//      o gets messages from EMIF IOBUF & adds to message queue
//   - taskSpawn dmsg_exec_task (executes messages from queue)
// 2. called at start of each new experiment to reset message queue 
//=========================================================================
void initDDRmsgs()
{
    static int initialized=0;
    disableMSGInts();
    if(initialized)
        return;
    initialized=1;
   
    msgsFromDDR=msgQCreate(MAX_DDR_MSGS,sizeof(msg_struct),MSG_Q_FIFO);
    ddr_msg_sem=semBCreate(SEM_Q_FIFO,SEM_EMPTY);
    /* create the pub issue  Mutual Exclusion semaphore */
    pHostMsgMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);

    ddrTask(DMSG_TSK_NAME, (void*)dmsg_exec_task, DSPMSG_TASK_PRIORITY);
    ddrTask(DMSG_ISR_NAME, (void*)dmsg_reader_task, DSPMSG_IST_PRIORITY);
    ddr_debug=DFLT_DDR_DEBUG;
}

//-------------------------------------------------------------
// dmsg_exec_task() ddr->host message execution task
// - A lower priority task that executes the jobs specified in
//   the C67->405 msgs posted by ddr_reader_task
// - The following tasks may fail if resources are blocked:
//   host_data       posts data ready msg to dspDataXfer (upLink)
//   host_error      calls sendException, stops exp.
//   host_warning    calls sendException, exp continues
//-------------------------------------------------------------
void dmsg_exec_task()
{
    static msg_struct msg;
    int stat;
    FOREVER {
        stat=msgQReceive(msgsFromDDR, (char *) &msg, DDR_MSG_SIZE, WAIT_FOREVER);
        total_ddr_msgs++;
        if(stat==ERROR){
            PRINT0("HOST MSG ERROR !!!"); // shouldn't ever get here
            continue;
        }

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_MSG+msg.id,NULL,NULL);
#endif
        switch (msg.id) {
        case HOST_END_DATA:
            host_data(msg.arg1,msg.arg2,msg.arg3,ACM_LAST);
            break;
        case HOST_DATA:
            host_data(msg.arg1,msg.arg2,msg.arg3,0);
            break;
        case HOST_ERROR:
            host_error(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_INIT:
            host_init(msg.arg1);
            break;
        case HOST_END_EXP:
            host_end_exp(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_DATA_INFO:
            host_data_info(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_XIN_INFO:
            host_xin_info(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_SS_REQ:
            host_ss_req(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_INIT_EXP:
            host_init_exp(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_START_EXP:
            host_start_exp(msg.arg1);
            break;
        case HOST_STOP_EXP:
            host_stop_exp(msg.arg1,msg.arg2);
            break;
        case HOST_RESUME_EXP:
            host_resume_exp(msg.arg1);
            break;
        case HOST_TEST:
            host_test(msg.arg1,msg.arg2,msg.arg3);
            break;
        case HOST_VERSION:
            host_version(msg.arg1,msg.arg2,msg.arg3);
            break;
        default:
            host_unknown(msg.id,msg.arg1,msg.arg2);
            break;
        }

#ifdef INSTRUMENT
	wvEvent(EVENT_DSP_MSG,NULL,NULL);
#endif

    }
}
