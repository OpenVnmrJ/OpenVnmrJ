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
#include <vxWorks.h>
#include <iv.h>
#include <intLib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semLib.h>
#include "nvhardware.h"
#include "logMsgLib.h"
#include "gradient.h"
#include "gradient_fifo.h"
#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#include "errorcodes.h"
#include "math.h"

FIFO_REGS masterFifo_regs = { 
   				get_pointer(GRADIENT,FIFOInstructionWrite),
				get_pointer(GRADIENT,FIFOControl),
				get_pointer(GRADIENT,FIFOStatus),
        			get_pointer(GRADIENT,InterruptStatus),
        			get_pointer(GRADIENT,InterruptEnable),
        			get_pointer(GRADIENT,InterruptClear),
        			get_pointer(GRADIENT,ClearCumulativeDuration),
        			get_pointer(GRADIENT,CumulativeDurationLow),
        			get_pointer(GRADIENT,CumulativeDurationHigh),
        			get_pointer(GRADIENT,InstructionFIFOCount),
        			get_pointer(GRADIENT,DataFIFOCount),
        			get_pointer(GRADIENT,InvalidOpcode),
        			get_pointer(GRADIENT,InstructionFifoCountTotal),
        			get_pointer(GRADIENT,ClearInstructionFifoCountTotal) 
			    };



/** read these from pRF_InterruptStatus / interrupt status..*/
#define  UNDERFLOW_FLAG    (0x00000002)
#define  OVR_FLAG     (0x00000001)

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */

extern MSG_Q_ID pMsgs2FifoIST;     /* msgQ for harderror resulting from failure assertion */

extern int SafeGateVal;         /* Safe Gate values for Abort or Exp End, in globals.c */
extern int SafeXAmpVal;	        /* PFG or Gradient X Amp settings */ 
extern int SafeYAmpVal;		/* PFG or Gradient Y Amp settings */ 
extern int SafeZAmpVal;		/* PFG or Gradient Z Amp settings */ 
extern int SafeB0AmpVal;	/* Gradient B0 Amp settings */ 
extern int SafeXEccVal;		/* Gradient X ECC settings */ 
extern int SafeYEccVal;		/* Gradient Y ECC settings */ 
extern int SafeZEccVal;		/* Gradient Z ECC settings */ 
extern int SafeB0EccVal;	/* Gradient B0 ECC settings */ 

initFifo()
{
  unsigned int fifoIntMask;
    int instrfifosize;
    int newhiwatermark;

  fifoIntMask = get_mask(GRADIENT,fifo_overflow_status) | get_mask(GRADIENT,fifo_underflow_status) | 
		get_mask(GRADIENT,fifo_finished_status) | get_mask(GRADIENT,fifo_started_status)  |
                get_mask(GRADIENT,invalid_opcode_int_status) |
                get_mask(GRADIENT,fail_int_status) | get_mask(GRADIENT,warn_int_status);

  cntrlFifoInit(&masterFifo_regs, fifoIntMask, get_instrfifosize(GRADIENT));

    /* +++++++ set the high and low device-paced FIFO DMA water marks +++++++  */
    /* size - headroom equal new high water mark for FIFO DMA device-paced */
    /* 8192 - 6 = 8186, so at 8186 words the DMA is told to stop, 
    /*    usually two more words get in */
    instrfifosize = getfifosize();
    newhiwatermark = instrfifosize - FIFO_HIWATER_MARK_HEADROOM;   /* 6 */
    setfifohimark(newhiwatermark);
    /* setfifolowmark( ..... );  leave as default of 512 or half the instruction FIFO size */

  set2gradfifo();

  initialBrdSpecificFailItr();

  /* set values in the SW FIFO Control registers to appropriate failsafe values */
  /* typically zero, when fail line is assert these values are presented to the output */
  /* of the controller, (e.g. high speed gates)  Values that need to be serialized */
  /* must strobed output by software however.	*/
  /*            Greg Brissey 9/16/04  */
  preSetAbortStates();




  /* cntlrFifoBufCreate(unsigned long numBuffers, unsigned long bufSize, int dmaChannel, volatile unsigned int* const pFifoWrite ) */
  if ((cntrlFifoDmaChan = dmaGetChannel(2)) == -1)
  { printf("No dma channel available for FIFO \n"); return; }

  pCntrlFifoBuf =  cntlrFifoBufCreate(GRADIENT_NUM_FIFO_BUFS, GRADIENT_SIZE_FIFO_BUFS, cntrlFifoDmaChan, 
                            (volatile unsigned int*) get_pointer(GRADIENT,FIFOInstructionWrite));

}

/*
 * Rotorsync - the cntlr variant of rotor sync 
 *   place halt into FIFO so the controller can wait for the
 *   rotosync to complete.  
 *   the master assert the sync line when rotor sync is complete
 *   allowing the other controllers to continue
 *
 *    Author: Greg Brissey  4/05/2005
 */
int RotorSync(int count, int postdelay) 
{
   int instrwords[20];
   int num;
   int fifoEncodeSystemSync(int postdelay, int *instrwords);

   num = fifoEncodeSystemSync(postdelay, instrwords);
   cntrlFifoBufPut(pCntrlFifoBuf, (long *) instrwords, num );
   return(num);
}

/*
 * fifoEncodeSWItr(), return a encoded stream of FIFO instructions to
 * the selected Software FIFO interrupt to occur.
 *
 *    Author: Greg Brissey  8/11/04
 */
int fifoEncodeSWItr(int SWitr, int *instrwords)
{
   int i = 0;
   instrwords[i++] = encode_GRADIENTSetGates(0,0xf00,0);  /* only effect SW bits */
   /* PHIL CHECKS */
   DPRINT(1,"320 INTERRUPT \n");
   instrwords[i++] = encode_GRADIENTSetDuration(0,320); /* duration of asserted interrupt bit, 1us */
   switch(SWitr)
   {
 
      case 1: instrwords[i++] = encode_GRADIENTSetGates(1,0xf00,0x100); break;
      case 2: instrwords[i++] = encode_GRADIENTSetGates(1,0xf00,0x200); break;
      case 3: instrwords[i++] = encode_GRADIENTSetGates(1,0xf00,0x400); break;
      case 4: instrwords[i++] = encode_GRADIENTSetGates(1,0xf00,0x800); break;
   }
   instrwords[i++] = encode_GRADIENTSetGates(1,0xf00,0x000);
   return(i);
}

/* 
 *  Time Duration of Zero will pause FIFO, followed by another post delay 
 *  given by postdelay argument, approx. 100nsec
 *
 *      Author: Greg brissey 8/12/2004
 */
int fifoEncodeSystemSync(int postdelay,int *instrwords)
{
   int i = 0;
   /*  TEMP */
   if ((postdelay < 320) && (postdelay > 0))
   {
      DPRINT1(-2,"1 postDelay UP'd to 320!! was %d\n",postdelay);
      postdelay = 320;
   }
   instrwords[i++] = encode_GRADIENTSetDuration(0,0); /* duration of Zero will pause FIFO waiting for Sync bit */
   instrwords[i++] = encode_GRADIENTSetGates(1,FF_GATE_SW4,0);
   instrwords[i++] = encode_GRADIENTSetDuration(0,postdelay); /* duration of post delay */
   instrwords[i++] = encode_GRADIENTSetGates(1,FF_GATE_SW4,0);
   return(i);
}

int fifoEncodeDuration(int write, int duration,int *instrwords)
{
    int i = 0;
if ((duration  < 320) && (duration > 0)) DPRINT1(-2,"2 THIS IS AN ERROR postDelay BEEP %d \n",duration);
    instrwords[i++] = encode_GRADIENTSetDuration(write,duration);
    return i;
}


int fifoEncodeGates(int write, int mask,int gates, int *instrwords)
{
    int i = 0;
    instrwords[i++] = encode_GRADIENTSetGates(write,mask,gates);
    return i;
}

set2gradfifo()
{
   set_register(GRADIENT,FIFOOutputSelect,1);
}

/* output controlled via SW registers (0) or FIFO output (1) */
void setFifoOutputSelect(int SwOrFifo)
{
   set_register(GRADIENT,FIFOOutputSelect,SwOrFifo);
   return;
}


int getfifosize()
{
  return( get_instrfifosize(GRADIENT) );
}

prtfifohilowmarks()
{
   int himark,lowmark;

   himark = get_field(GRADIENT,instruction_fifo_high_threshold);
   lowmark = get_field(GRADIENT,instruction_fifo_low_threshold);
   printf("FIFO DMA device-paced hi watermark: %d, low water mark: %d\n",himark,lowmark);
}

int getfifohimark()
{
   int himark;
   himark = get_field(GRADIENT,instruction_fifo_high_threshold);
   return( himark );
}

int setfifohimark(int hi)
{
   set_field(GRADIENT,instruction_fifo_high_threshold,hi);
   return(0);
}

int getfifolowmark()
{
   int lowmark;
   lowmark = get_field(GRADIENT,instruction_fifo_low_threshold);
   return( lowmark );
}

int setfifolowmark(int low)
{
   set_field(GRADIENT,instruction_fifo_low_threshold,low);
   return(0);
}

preSetAbortStates()
{
   SafeGateVal = SafeXAmpVal = SafeYAmpVal = SafeZAmpVal = 0;		
   SafeB0AmpVal	= SafeXEccVal = SafeYEccVal = SafeZEccVal = SafeB0EccVal = 0;

/* there are various other AUX & SPI devices but at present it has been decided that */
   set_register(GRADIENT,SoftwareGates,0);
   set_register(GRADIENT,SoftwareUser,0);
   set_register(GRADIENT,SoftwareXAmp,0);
   set_register(GRADIENT,SoftwareYAmp,0);
   set_register(GRADIENT,SoftwareZAmp,0);
   set_register(GRADIENT,SoftwareB0Amp,0);
   set_register(GRADIENT,SoftwareXEcc,0);
   set_register(GRADIENT,SoftwareYEcc,0);
   set_register(GRADIENT,SoftwareZEcc,0);
   set_register(GRADIENT,SoftwareB0Ecc,0);
}

/*
 * Enable Serialization for FPGA to serialize the safe values out to hardware
 *  On Fail
 */
void serializeSafeVals()
{
    // nothing to do, ho, hum...
}

resetFifoHoldRegs()
{
   int rep,duration;
   int xamp,yamp,zamp,b0amp,xamp_scale,yamp_scale,zamp_scale,b0amp_scale;
   int xecc,yecc,zecc,b0ecc,shim,user;
   int clear_zero, count_zero;
   int clear_one, count_one;

    duration = rep = 0;
    xamp = yamp = zamp = b0amp = 0;
    xamp_scale = yamp_scale = zamp_scale = b0amp_scale = 0;
    xecc = yecc = zecc = b0ecc = shim = user = 0;
    clear_zero = count_zero = 0;
    clear_one = count_one = 1;

   /* clear the FIFO gate holding register */

   cntrlFifoPut(encode_GRADIENTSetGates(0,0xfff,0x000));
   cntrlFifoPut(encode_GRADIENTSetDuration(0,duration));
   cntrlFifoPut(encode_GRADIENTSetRepeatedDuration(0,rep,duration));

   cntrlFifoPut(encode_GRADIENTSetXAmp(0,clear_zero,count_one,xamp));
   cntrlFifoPut(encode_GRADIENTSetXAmp(0,clear_one,count_zero,yamp));
   cntrlFifoPut(encode_GRADIENTSetXAmpScale(0,clear_zero,count_one,xamp_scale));
   cntrlFifoPut(encode_GRADIENTSetXAmpScale(0,clear_one,count_zero,xamp_scale));

   cntrlFifoPut(encode_GRADIENTSetYAmp(0,clear_zero,count_one,yamp));
   cntrlFifoPut(encode_GRADIENTSetYAmp(0,clear_one,count_zero,yamp));
   cntrlFifoPut(encode_GRADIENTSetYAmpScale(0,clear_zero,count_one,yamp_scale));
   cntrlFifoPut(encode_GRADIENTSetYAmpScale(0,clear_one,count_zero,yamp_scale));

   cntrlFifoPut(encode_GRADIENTSetZAmp(0,clear_zero,count_one,zamp));
   cntrlFifoPut(encode_GRADIENTSetZAmp(0,clear_one,count_zero,zamp));
   cntrlFifoPut(encode_GRADIENTSetZAmpScale(0,clear_zero,count_one,zamp_scale));
   cntrlFifoPut(encode_GRADIENTSetZAmpScale(0,clear_one,count_zero,zamp_scale));

   cntrlFifoPut(encode_GRADIENTSetB0Amp(0,clear_zero,count_one,b0amp));
   cntrlFifoPut(encode_GRADIENTSetB0Amp(0,clear_one,count_zero,b0amp));
   cntrlFifoPut(encode_GRADIENTSetB0AmpScale(0,clear_zero,count_one,b0amp_scale));
   cntrlFifoPut(encode_GRADIENTSetB0AmpScale(0,clear_one,count_zero,b0amp_scale));

   cntrlFifoPut(encode_GRADIENTSetXEcc(0,xecc));
   cntrlFifoPut(encode_GRADIENTSetYEcc(0,yecc));
   cntrlFifoPut(encode_GRADIENTSetZEcc(0,zecc));
   cntrlFifoPut(encode_GRADIENTSetB0Ecc(0,b0ecc));
   cntrlFifoPut(encode_GRADIENTSetShim(0,shim));
   cntrlFifoPut(encode_GRADIENTSetUser(0,user));

   return 0;
}

prtholdregs()
{
   printf("GRADIENT_Duration: %ld\n", get_register(GRADIENT,Duration));
   printf("GRADIENT_DurationCount: %ld\n", get_register(GRADIENT,DurationCount));
   printf("GRADIENT_XAmp: %ld\n", get_register(GRADIENT,XAmp));
   printf("GRADIENT_XAmpIncrement: %ld\n", get_register(GRADIENT,XAmpIncrement));
   printf("GRADIENT_XAmpCount: %ld\n", get_register(GRADIENT,XAmpCount));
   printf("GRADIENT_XAmpClear: %ld\n", get_register(GRADIENT,XAmpClear));
   printf("GRADIENT_YAmp: %ld\n", get_register(GRADIENT,YAmp));
   printf("GRADIENT_YAmpIncrement: %ld\n", get_register(GRADIENT,YAmpIncrement));
   printf("GRADIENT_YAmpCount: %ld\n", get_register(GRADIENT,YAmpCount));
   printf("GRADIENT_YAmpClear: %ld\n", get_register(GRADIENT,YAmpClear));
   printf("GRADIENT_ZAmp: %ld\n", get_register(GRADIENT,ZAmp));
   printf("GRADIENT_ZAmpIncrement: %ld\n", get_register(GRADIENT,ZAmpIncrement));
   printf("GRADIENT_ZAmpCount: %ld\n", get_register(GRADIENT,ZAmpCount));
   printf("GRADIENT_ZAmpClear: %ld\n", get_register(GRADIENT,ZAmpClear));
   printf("GRADIENT_B0Amp: %ld\n", get_register(GRADIENT,B0Amp));
   printf("GRADIENT_B0AmpIncrement: %ld\n", get_register(GRADIENT,B0AmpIncrement));
   printf("GRADIENT_B0AmpCount: %ld\n", get_register(GRADIENT,B0AmpCount));
   printf("GRADIENT_B0AmpClear: %ld\n", get_register(GRADIENT,B0AmpClear));
   return 0;
}

/*====================================================================================*/
/*
 *   Disable the Slew rate Exceeded Interrupt
 *
 *  Author Greg Brissey 10/25/05
 *
 */
disableSlewExceededItr()
{
      unsigned int *pItrEnable = (unsigned int *) get_pointer(GRADIENT,InterruptEnable);
      /* logMsg("pItrEnable: 0x%lx, *pItrEnable: 0x%lx\n",pItrEnable,*pItrEnable); */
      /* logMsg("*pItrEnable: 0x%lx & 0x%lx\n",*pItrEnable, ~(get_mask(GRADIENT,slew_limit_exceeded_status))); */
      
       *pItrEnable = *pItrEnable & ( ~(get_mask(GRADIENT,slew_limit_exceeded_status)) );
      /* logMsg("*pItrEnable: 0x%lx \n",*pItrEnable); */
}

/*
 *   Enable the Slew rate Exceeded Interrupt
 *
 *  Author Greg Brissey 10/25/05
 *
 */
enableSlewExceededItr()
{
      unsigned int *pItrEnable = (unsigned int *) get_pointer(GRADIENT,InterruptEnable);
      /* logMsg("pItrEnable: 0x%lx, *pItrEnable: 0x%lx\n",pItrEnable,*pItrEnable); */
      /* logMsg("*pItrEnable: 0x%lx | 0x%lx\n",*pItrEnable, (get_mask(GRADIENT,slew_limit_exceeded_status))); */
       *pItrEnable = *pItrEnable | ( (get_mask(GRADIENT,slew_limit_exceeded_status)) );
      /* logMsg("*pItrEnable: 0x%lx \n",*pItrEnable); */
}


/*
 *     Board Specific ISR for harderrors of the system 
 *
 *  Author Greg Brissey 9/23/04
 *
 */
static void Brd_Specific_Fail_ISR(int int_status, int errorcode) 
{
   /* we only allow one slew rate waring per Experiment, if we get then disable this interrupt */

   if (errorcode == (WARNINGS + GRAD_SLEW_EXCEEDED))
   {
      int maxx,maxy,maxz,maxb0,rx,ry,rz,rb0;

      disableSlewExceededItr();
      /* as of 11/16/2006 these registers are gone */
      /*
      * rx = get_field(GRADIENT,xslewdiffval);
      * ry = get_field(GRADIENT,yslewdiffval);
      * rz = get_field(GRADIENT,zslewdiffval);
      * rb0 = get_field(GRADIENT,b0slewdiffval);
      */
      /* correct for 16 bit natural of value >= 0x8000 is negative */
      /* say the register reads 0xfea7 - 
       *  this must be converted to a positive number and put a negative
       *  sign in front of it ..
       *  0xfea7 - 1111 1110 1010 0111
       * first you take the complement = 0x0158 - 0000 0001 0101 1000
       * then add 1 --> 0x0159 == 0000 0001 0101 1001
       * and put negative sign in front for display ...
       * ==> -345.
       */
       /*
       * rx = (rx >= 0x8000) ? (((~rx & 0xFFFF) + 1) * -1) : rx;
       * ry = (ry >= 0x8000) ? (((~ry & 0xFFFF) + 1) * -1) : ry;
       * rx = (rx >= 0x8000) ? (((~rx & 0xFFFF) + 1) * -1) : rx;
       * rb0 = (rb0 >= 0x8000) ? (((~rb0 & 0xFFFF) + 1) * -1) : rb0;
       */

       maxx = get_field(GRADIENT,xslewlimit);
       maxy = get_field(GRADIENT,yslewlimit);
       maxz = get_field(GRADIENT,zslewlimit);
       maxb0 = get_field(GRADIENT,b0slewlimit);
       /* logMsg(" ===========  SlewRate - X,Y,Z,B0 MAX: %d,%d,%d,%d, req: %d,%d,%d,%d \n",
	         maxx,maxy,maxz,maxb0, rx,ry,rz,rb0); */
       logMsg(" ===========  SlewRate - X,Y,Z,B0 MAX: %d,%d,%d,%d\n",
	         maxx,maxy,maxz,maxb0);
   }
   if (DebugLevel > -1)
     logMsg("Brd_Specific_Fail_ISR: status: 0x%lx, errorcode: %d\n",int_status, errorcode); 

   msgQSend(pMsgs2FifoIST,(char*) &errorcode, sizeof(int), NO_WAIT, MSG_PRI_NORMAL);
 
   return;
}

/*====================================================================================*/
/*
 * enable all board specific failures interrupts
 */
initialBrdSpecificFailItr()
{
  unsigned int failureMask,intMask;

  failureMask =  get_mask(GRADIENT,slew_limit_exceeded_status) |
	         get_mask(GRADIENT,spi_failed_busy_status) ;

  intMask =  get_mask(GRADIENT,slew_limit_exceeded_status); 
  /* DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask); */
  fpgaIntConnect( Brd_Specific_Fail_ISR, (WARNINGS + GRAD_SLEW_EXCEEDED), intMask );

  intMask =  get_mask(GRADIENT,spi_failed_busy_status); 
  /* DPRINT1(-1,"----------------> Failure Masks: 0x%lx\n",intMask); */
  fpgaIntConnect( Brd_Specific_Fail_ISR, (HDWAREERROR + GRADIENT_SPI_OVRRUN), intMask );

  /* disbale slew interrupt until the once per Exp code is inplace */
  cntrlFifoIntrpSetMask(failureMask);    /* enable interrupts */
}

char taga[52][16] = {"\nx->x#1","x->x#2","x->x#3",
		     "\nx->x#4","x->x#5","x->x#6",
		     "\nx->y#1","x->y#2","x->y#3",
		     "\nx->z#1","x->z#2","x->z#3",
		     "\nx->b0#1","x->b0#2","x->b0#3","x->b0#4",
                     "\n\ny->y#1","y->y#2","y->y#3",
		     "\ny->y#4","y->y#5","y->y#6",
		     "\ny->z#1","y->z#2","y->z#3",
		     "\ny->x#1","y->x#2","y->x#3",
		     "\ny->b0#1","y->b0#2","y->b0#3","y->b0#4",
		     "\n\nz->z#1","z->z#2","z->z#3",
		     "\nz->z#4","z->z#5","z->z#6",
		     "\nz->X#1","z->X#2","z->X#3",
		     "\nz->y#1","z->y#2","z->y#3",
		     "\nz->b0#1","z->b0#2","z->b0#3","z->b0#4",
		     "\n\nb0->b0#1","b0->b0#2","b0->b0#3","b0->b0#4"};
/*  phil stub only for testing */
showECC()
{
   int i,j;
   long long *pdl,idl;
   int  *pamp,iamp;
   float xamp;
   long double xecc, yecc;
   pdl =  (long long *) get_pointer(GRADIENT,ECCTimeConstant);
   pamp = (int *)  get_pointer(GRADIENT,ECCAmp);
   printf("ECC    Amp    TC\n");
   for (i = 0; i < 52; i++)
   {
       idl = *(pdl + i);
       iamp = *(pamp + i);
       //printf("%d :  %llx, %x\n",i,idl,iamp);
       /* the amplitude is easy */
       /* sign extend amplitude */
       if (iamp & 0x8000) iamp |= 0xffff0000;
       xamp = ((float) iamp) / 327.670;
       if (idl > 0x10000)
       xecc = 42.97512519471660918365155109591L - log((double) idl); 
       else 
       xecc = -1; /* off */
       if (xecc > 0.00000001)  xecc = 0.004/xecc;
       printf("%8s (%2d): %4.2f %6.1lf   ",taga[i],i,xamp,xecc);
   }
   printf("\n\nSlew Limits are :%x   %x   %x    %x\n\n",
    *get_pointer(GRADIENT,XSlewLimit), *get_pointer(GRADIENT,YSlewLimit),
    *get_pointer(GRADIENT,ZSlewLimit), *get_pointer(GRADIENT,B0SlewLimit)); 
/*
  printf("Duty Cycle Limits are :%x   %x   %x    %x\n",
    *get_pointer(GRADIENT,XDutyCycleLimit), *get_pointer(GRADIENT,YDutyCycleLimit),
    *get_pointer(GRADIENT,ZDutyCycleLimit), *get_pointer(GRADIENT,B0DutyCycleLimit));
*/
}   

int setECCd(int a1,double a2,double amp);

int setECC(int choice)
{
  double ampl,timeConstant;
  char buffer[256];
  int k;
  printf("ECC %d is %s\ntime Constant ms =",choice,taga[choice]);
  gets(buffer);
  k = sscanf(buffer,"%lf",&timeConstant);
  if (k < 1) return(-1);
  printf("ECC %d is %s time Constant ms = %f\namplitue %%?",choice,
    taga[choice],timeConstant);
  gets(buffer);
  k = sscanf(buffer,"%lf",&ampl);
  if (k < 1) return(-2);
   printf("ECC %d is %s time Constant ms = %f  amplitue %f%%\n",choice,
    taga[choice],timeConstant,ampl);
   setECCd(choice,timeConstant,ampl);
}

  /* this does not work from shell??? */
int setECCd(int choice,double Gtc, double Gampf)
{
   double ratio, expterm;
   long long tcCode, *tcPointer;
   int amp, *ampp;
   ampp = (int *) get_pointer(GRADIENT,ECCAmp);
   amp = ((int)(327.67*Gampf)) & 0xffff;
   tcPointer = (long long *) get_pointer(GRADIENT,ECCTimeConstant);
   if (Gtc > 0.01) ratio = 0.004L/Gtc; else ratio = 1.0;
   expterm = exp(-ratio);
   tcCode = (long long) (((double) 0x3FFFFFFFFFFFFFFFL) * expterm);
   *(ampp+choice) = amp;
   *(tcPointer+choice)  = tcCode;
   return(0);
}

