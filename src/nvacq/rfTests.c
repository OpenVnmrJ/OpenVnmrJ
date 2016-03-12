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
/*
=========================================================================
FILE: rfTests.c 4.1 03/21/08
=========================================================================
PURPOSE:
	Provide a file for board bringup and other misc. test  routines 

Externally available modules in this file:

Internal support modules in this file:

COMMENTS:

AUTHOR:	

*/
#include <vxWorks.h>
#include <stdlib.h>
#include "logMsgLib.h"
#include "rf.h"
#include "rf_fifo.h"
#include "nvhardware.h"
#include "dmaDrv.h"
#include "rfinfo.h"
#include "FFKEYS.h"

/* #define RF_CNTLR  */
#ifdef RF_CNTLR
#include "cntrlFifoBufObj.h"
#endif

#define INCLUDE_TESTING
#ifdef INCLUDE_TESTING

#define VPSYN(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)

VPSYN(RF_AMTRead);

#ifdef RF_CNTLR
extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
#endif

extern struct _RFInfo rfInfo;  // was originally defined here, moved to A32qInterp.c, now an exern here

/*  test routines */
void switch2sw()
{
   set_field(RF,fifo_output_select,1);
   set_field(RF,fifo_output_select,0);
}

void switch2fifo()
{ 
  set_field(RF,fifo_output_select,0); 
  set_field(RF,fifo_output_select,1);
}
/* experimental */
int swReadAux(int reg)
{
  reg &= 7;
  reg |= 8;  /* set the read back bit */
  switch2sw(); /* arbitrarily */
  set_field(RF,sw_aux,(reg<<8));
  set_field(RF,sw_aux_strobe,1); 
  set_field(RF,sw_aux_strobe,0);
  switch2fifo();
  return(get_field(RF,aux_read));
}
/* the 8 is the read commond */  
void swWriteAux(int reg,int data)
{
  switch2sw(); /* arbitrarily */
  reg &= 7; 
  data &= 0xff;
  set_field(RF,sw_aux,(((reg)<<8) | data));
  set_field(RF,sw_aux_strobe,1); 
  set_field(RF,sw_aux_strobe,0);
  switch2fifo();
}
auxReset()
{
   set_field(RF,sw_aux_reset,0);
   set_field(RF,sw_aux_reset,1);
   set_field(RF,sw_aux_reset,0);
}

//----
union ltrans {
  unsigned long long lword;
  unsigned int  iword[2];
  unsigned char cword[8];
};

unsigned long long toBCD(double f)
{
  unsigned long long binary,bcd;
  int i;
 
  binary = (unsigned long long) f;
  bcd = 0L; 
   
  for (i = 0; i < 16; i++, binary = binary / 10)
  {
     bcd |= (binary % 10) << (i*4);
  }
  printf("f = %f bcd = %llx\n",f,bcd);
  return(bcd);
}

/* latest PTS IF */
auxPTS(double freq)
 {
   double yy;
   union ltrans temp;
   if (freq > 2000.0) { printf("input in MHz\n");  exit(1);}
   /* implicit address zero.. */
     yy = freq * 10000000.0L+0.5;  /* 1/10 hz resolution */
     temp.lword = (~toBCD(yy));  
     printf("update test lword = %llx\n",temp.lword);  
     swWriteAux(0,0x6);  /* dummy byte  - must be six*/
     swWriteAux(0,(temp.cword[7] & 0xff)); 
     swWriteAux(0,(temp.cword[6] & 0xff)); 
     swWriteAux(0,(temp.cword[5] & 0xff)); 
     swWriteAux(0,(temp.cword[4] & 0xff)); 
     swWriteAux(0,(temp.cword[3] & 0xff));
     swWriteAux(1,0xff);  // additional
  }

int setAttenuator(int atten)
{
  int val,outword;
  val = 63 - atten; /* power -> attenuator */
  if (val > 79) val = 79; 
  if (val <  0) val = 0;
  outword = 0;
  if (val > 63) 
  {
    outword = 0x80;  /* 16 db atten */
    val -= 16;
  }
 
  if ( val & 0x20)  outword |= 0x2;
  if ( val & 0x10)  outword |= 0x4;
  if ( val & 0x08)  outword |= 0x8;
  if ( val & 0x04)  outword |= 0x10;
  if ( val & 0x02)  outword |= 0x20;
  if ( val & 0x01)  outword |= 0x40;
  outword = (~outword) & 0xff;
  swWriteAux(6,outword);
  return(4);
}

int swGates(int word)
{
  switch2sw();
  set_field(RF,sw_gates,word);
}

int swAmp(int word)
{
  switch2sw();
  set_field(RF,sw_amp,word);
}

int getRFGates()
{
  return(get_field(RF,sw_gates));
}

int TuneON(int power)
{
  int i;
DPRINT3(-1,"TuneOn: power=%d band=%d  mixer=%d\n",
                      power-16,rfInfo.ampHiLow,rfInfo.xmtrHiLow);
  setAttenuator(power-16);
  switch2sw();  /* this is dangerous */
  set_field(RF,sw_amp,getTuneLevel());	// how safe is this???
  i = get_field(RF,sw_gates);
  if (rfInfo.xmtrHiLow)
     set_field(RF,sw_gates,0xfe | i);	// set xmtr mixer bit
  else
     set_field(RF,sw_gates,0xbe | i);	// clear xmtr mixer bit
}

int TuneOFF()
{
  int i;
  switch2sw();
  i = get_field(RF,sw_gates);
  set_field(RF,sw_gates,i&0x40); 
  set_field(RF,sw_amp,0);
  setAttenuator(-16);
}

/* this routine attempts to put 10 dBm out of the amplifiers */
/* Note that TuneON is called with 5xswitchValue or 45 dB    */
/* maximum for the coarse attn, ie, 34 dB below max already  */
/*   50W = 47 dBm  -> attn_out=-37, xmtr_out=-3   2899 = 0xb53 */
/*  100W = 50 dBm  -> attn_out=-40, xmtr_out=-6   2047 = 0x7ff */
/*  300W = 55 dBm  -> attn_out=-45, xmtr_out=-11  1154 = 0x482 */
/*  400W = 56 dBm  -> attn_out=-46, xmtr_out=-12  1023 = 0x3ff */
/* other = 60 dBm  -> attn_out=-50, xmtr_out=-16   649 = 0x289 */
int getTuneLevel()
{
int ampType;
int xmtrLevel;
   if (rfInfo.tunePwr > -1) 
      xmtrLevel =  rfInfo.tunePwr;
   else
   {
      ampType = (*pRF_AMTRead)&0xf;	// see VSYN
      switch (ampType) {
      case 0x0:	// 300W-300W
           xmtrLevel = 1154;
           break;
      case 0x2:	//  50W-----
      case 0x1:	//  50W-300W
           if (rfInfo.ampHiLow) xmtrLevel = 2899; else xmtrLevel = 1154;
           break;
      case 0x4:	// -----400W
           xmtrLevel=1023;
           break;
      case 0x6:	// 100W-100W
           xmtrLevel=2047;
           break;
      case 0x8:	// -----300W
           xmtrLevel=1154;
           break;
      case 0xa:	// 100W-----
           xmtrLevel=2047;
           break;
      case 0xb:	//  50W-400W
           if (rfInfo.ampHiLow) xmtrLevel = 2899; else xmtrLevel = 1023;
           break;
      case 0xc:	// 100W-300W
      case 0xd:	// 100W-300W
           if (rfInfo.ampHiLow) xmtrLevel = 2047; else xmtrLevel = 1154;
           break;
      case 0x3:
      case 0xe:
      default:	//dont know, could be 1 kW
           xmtrLevel=649;
           break;
      }
   }
DPRINT3(-1,"getTuneLevel: amptpe=%d(0x%x) returns %d",ampType,ampType,xmtrLevel);
   return(xmtrLevel<<4);  // left shift 4, 12-bit DAC, 16 bit word
}


AMTbits()
{
int ampType;
   ampType = *(int *)(FPGA_BASE_ADR + RF_AMTRead);
   ampType &= 0xF;
   DPRINT2(-1,"AMT = %x (%d)",ampType,ampType);
   if      (ampType==0x0) printf("AMT 3900-11  Lo-Lo band\n");
   else if (ampType==0x1) printf("AMT 3900-12  Hi-Lo band\n");
   else if (ampType==0x2) printf("AMT 3900-2   Hi    band\n");
   else if (ampType==0x4) printf("AMT 3900-1S  Lo band\n");
   else if (ampType==0x6) printf("AMT 3900-55  Hi band(100/100W)\n");
   else if (ampType==0x8) printf("AMT 3900-1   Lo band\n");
   else if (ampType==0xa) printf("AMT 3900-7   Hi band\n");
   else if (ampType==0xb) printf("AMT 3900-1S4 Hi-Lo band\n");
   else if (ampType==0xc) printf("AMT 3900-15B Hi-Lo band(100W)\n");
   else if (ampType==0xd) printf("AMT 3900-1S7 Hi-Lo band(100W)\n");
   else if (ampType==0xf) printf("None\n");
   else                   printf("Unknown\n");

}


#ifdef RF_CNTLR
tstRfBadOpCode(int opcode)
{
    int badopcode,badopcode2;
    badopcode = (opcode << 26);
    cntrlFifoCumulativeDurationClear();
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(badopcode);
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,320));
    cntrlFifoPut(encode_RFSetDuration(1,0));
    taskDelay(calcSysClkTicks(83));  /* 83 ms, taskDelay(5); */
    cntrlFifoStart();
}

struct RFHOLD {
  int duration;
  int gates;
  int amp;
  int amps;
  int phase;
  int phasec;
}  RF_state;
 
unsigned long totalduration;

fifoDecode(unsigned int word,int noprt)
{
   void rfdecode(unsigned int word,int noprt);
   rfdecode(word,noprt);
}

void rfdecode(unsigned int word,int noprt)
{
  int tmp,tdata,latched;
  tmp = word;

   if (noprt != 1)
      printf(" 0x%8lx : ",word);
   if (tmp & LATCHKEY)
            latched = 1;
          else
            latched = 0;
          tdata = tmp & 0x3ffffff; /* 26 bits of data */
          tmp &= 0x7C000000;
          switch (tmp)
            {
               case DURATIONKEY:
   		 if (noprt != 1)
                    printf("duration of %7.4f usec",((float) tdata)/80.0);
                 RF_state.duration = tdata;  break;
               case GATEKEY:
   		 if (noprt != 1)
                    printf("mask/gate set to %x",tdata&0xfff);
                 RF_state.gates = tdata; break;
               case RFPHASEKEY:
   		 if (noprt != 1)
                     printf("rf phase pattern set to %x",tdata&0xffff);
                  RF_state.phase = tdata; break;
               case RFPHASECYCLEKEY:
   		 if (noprt != 1)
                     printf("rf phase cycle set to %x",tdata&0xffff);
                  RF_state.phasec = tdata; break;
               case RFAMPKEY:
   		 if (noprt != 1)
                     printf("rf Amp pattern set to %x",tdata&0xffff);
                  RF_state.amp = tdata; break;
               case RFAMPSCALEKEY:
   		 if (noprt != 1)
                     printf("rf Amp Scale set to %x",tdata&0xffff);
                  RF_state.amps = tdata; break;
               case USER:
   		 if (noprt != 1)
                    printf("user data = %x\n",(tdata&0x3)); break;
               case AUX:
   		 if (noprt != 1)
                    printf("aux addr,data = %x,%x\n",(tdata&0xfff)>>6, \
                        tdata & 0xff); break;
            default:
                  printf("don't recognize key!! %x\n",tmp);
           }
          if (latched)
          {
   	     if (noprt != 1)
                printf(" fifo word latched\n");
             if (tmp != AUX)
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of %9.4lf usec  GATE = %4x  ", \
                    ((float) RF_state.duration)/80.0,RF_state.gates);
               totalduration = totalduration + RF_state.duration;
             }
             else
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of 0.050 usec AUX  GATE = %4x  ", \
                   RF_state.gates);
               totalduration = totalduration + 4;
             }

   	     if (noprt != 1)
                printf("AMP = %6.1lf  PHASE = %7.4lf\n",\
                    ((RF_state.amp*RF_state.amps)>>16)/6.5535+0.05,\
                    ((RF_state.phase+RF_state.phasec)&0xffff)*0.005493);
          }
          else { 
   	     if (noprt != 1)
               printf("\n");
           }
}

int prtRfTime()
{
   printf("----->>  Total duration: %lu ticks, %7.4f usec  <<--------\n\n",totalduration, ((float) totalduration)/80.0);
   totalduration = 0L;
}
void prtFifoTime()
{
   prtRfTime();
}

#endif

int setPhaseBias(int j) { set_field(RF,phase_bias,j); return(get_field(RF,phase_bias));  }
int setAmPBias(int j)    { set_field(RF,amp_bias,j); return(get_field(RF,amp_bias));  }

setAmpSkew(int j)
{ 
  set_field(RF,amp_delay,j);
}

setQuadPhaseSkew(int j)
{ 
  set_field(RF,phase_delay,j);
}

setGateSkew(int j)
{ 
  set_field(RF,xout_delay,j);
}

printSkewInfo()
{
   printf("delays are 12.5ns ticks\n");
   printf("quad phase     delayed by %d ticks\n",get_field(RF,phase_delay));
   printf("amplitude bits delayed by %d ticks\n",get_field(RF,amp_delay));
   printf("xout ONLY bit  delayed by %d ticks\n",get_field(RF,xout_delay));
}


int write_ftws(int ftw1, int ftw2, int freq)
{
     unsigned char darray[18];
     int header;
     int cmd;
     unsigned long rfswitch;
     int i;
     set_field(RF,fifo_output_select,0);

     if (freq > 316)
       rfswitch = 0x03555555;
     else
       rfswitch = 0x00AAAAAA;

     header = 0x12;           /* first 5 bits need to be size == 18  */
     cmd = 0x01;

     darray[0] = header & 0xff;
     darray[1] = cmd & 0xff;
     /* these lines represent sending ftw1 LSB first */
     darray[2] = 0x00;
     darray[3] = 0x00;
     darray[4] = ftw1 & 0xff;
     darray[5] = (ftw1 >>  8) & 0xff;
     darray[6] = (ftw1 >> 16) & 0xff;
     darray[7] = (ftw1 >> 24) & 0xff;

     darray[8] =  (ftw2 & 0xff);
     darray[9] =  (ftw2 >>  8) & 0xff;
     darray[10] = (ftw2 >> 16) & 0xff;
     darray[11] = (ftw2 >> 24) & 0xff;
     darray[12] = rfswitch & 0xff;
     darray[13] = (rfswitch >> 8) & 0xff;
     darray[14] = (rfswitch >> 16) & 0xff;
     darray[15] = (rfswitch >> 24) & 0xff;
     darray[16] = 0x00;
     darray[17] = 0x00;
     for (i=0;i<18;i=i+1)
     {
       set_field(RF,sw_aux,(0 << 8 ) | darray[i]);
       printf("dds darray[%d]: 0x%x\n",i,darray[i]);
       taskDelay(calcSysClkTicks(17));  /* 16 ms, taskDelay(1); */
       set_field(RF,sw_aux_strobe,1);
       set_field(RF,sw_aux_strobe,0);
       taskDelay(calcSysClkTicks(17));  /* 16 ms, taskDelay(1); */
     }
     set_field(RF,sw_aux,(1 << 8 ) | 0xff);	
     taskDelay(calcSysClkTicks(17));  /* 16 ms, taskDelay(1); */
     set_field(RF,sw_aux_strobe,1);
     set_field(RF,sw_aux_strobe,0);
     taskDelay(calcSysClkTicks(17));  /* 16 ms, taskDelay(1); */
     return(1);
}


int FTW1, FTW2;

void getFTW(double frequency) {
    double fc;
    /* int FTW1;
       int FTW2; */

    if (frequency >= 325e6) {
        frequency = 720e6 - frequency;
    }

    FTW2 = getFTW2(frequency);

    if (FTW2 != -1) {
        fc = frequency * 65536.0 / ( (double) FTW2);
        FTW1 = getFTW1(fc);
    } else {
        FTW1 = FTW2;
    }
    FTW2 <<= 16;
}


int getFTW2(double frequency) {
    double FTWfloat;
    int    FTWfixed;
    double fc;
    double f0;
    int    i;

    f0 = frequency;
    fc = 782.5e6;

    FTWfloat = 65536.0 * f0 / fc;
    FTWfixed = ( (int) FTWfloat ) ;
    FTWfixed &= 0xff00;  

    FTWfloat = (double) FTWfixed;
    fc = 65536.0 * f0 / FTWfloat;

    i = 0;    
    while (fc > 819.0e6)
   {        
        FTWfixed += 0x0010;
        FTWfloat = (double) FTWfixed;
        fc = 65536.0 * f0 / FTWfloat;
        i++;
        if (i > 15) {
            FTWfixed = -1;
            break;
        }
    }    
    return (FTWfixed);

}

/* April 29 revision.. */
int getFTW1(double fc) {
    double ftw1Raw;
    double fcLocal;
    int temp; 

    /* Take fc "back" through the tripler and mixer.*/
    fcLocal = ( fc / 3.0 ) - 240.0e6;
    ftw1Raw = ( fcLocal / 240.0e6 ) * 4294967296.0L;
    temp = (int) ftw1Raw; 
    printf("ftw1 res = %x\n",temp);
    return( temp );
}

setDDS(double f)
{
  int t;
  if (f > 1000.0) 
    {
      printf("frequency input in MHZ\n");
	return(-1);
    }
  t = (int) f;
  f *= 1000000.0;
  getFTW(f);
  printf("write command 1 = 0x%x, 2 = 0x%x  3=0x%x\n",FTW1,FTW2,t);
  write_ftws(FTW1,FTW2,t);
} 


#ifdef RF_CNTLR

execpat(int pat_index,int mode)
{
    int i,steps,result;
    int dmaFifoChan;
    long long duration;
    unsigned long l;
    int *patternList;
    int patternSize;

    setFifoOutputSelect(1);  /* fifo control outputs */
    cntrlFifoCumulativeDurationClear();
    clearSystemSyncFlag();
    clearStart4ExpFlag();

    dmaFifoChan = cntrlFifoDmaChanGet();

    result = getPattern(pat_index, &patternList, &patternSize);
    printf("dma pattern size: %d\n",patternSize);
    if (result == 0) 
    {
       /* prtPat(pat_index); */
       if (patternSize > 0)
       {
          if (mode == 0)
            sendCntrlFifoList(patternList, patternSize, 1);
          else if (mode==1)
            dmaXfer( dmaFifoChan, MEMORY_TO_PERIPHERAL, NO_SG_LIST, patternList, 0x70000108, patternSize, NULL, NULL);
          else
            cntrlFifoXferSGList(pCntrlFifoBuf, (long*) patternList, patternSize, 1, 0, 0);

          taskDelay(calcSysClkTicks(166));  /* 166 ms, taskDelay(10); */
          writeCntrlFifoWord(encode_RFSetDuration(1,0));
          flushCntrlFifoRemainingWords();
        }
        if ( ! cntrlFifoRunning() )
        {
           taskDelay(calcSysClkTicks(166));  /* 166 ms, taskDelay(10); */
           startFifo4Exp();   /* startCntrlFifo(); */
        }
        cntrlFifoWait4StopItrp();
    } 
    else 
    {
        DPRINT(-1,"Pattern not found\n");
    }
    return 0;
}
#endif
#endif
