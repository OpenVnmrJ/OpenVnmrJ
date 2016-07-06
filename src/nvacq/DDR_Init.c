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

#define VERIFY_OS // verify os code transfer
//#define DEBUG_DDR_INIT   // print debug messages

#define LOW_SPEED_HPI   0x07070710
//#define HIGH_SPEED_HPI  0x01010103
//#define HIGH_SPEED_HPI  0x00000002
#define HIGH_SPEED_HPI  0x00000003		/* this was deem incorrect, see bug #348 */
//#define HIGH_SPEED_HPI  0x01010104		/* see bug #348 for reference */

#define OS_WRITE_HPI_BASE  1  
double C6Speed=220e6;
double EMIFSpeed=88e6;


//=========================================================================
// FILE: DDR_Init
//=========================================================================
#include <stdlib.h>
#include <cacheLib.h>
#include <iv.h>

#include "math.h"
#ifdef INSTRUMENT
#include "wvLib.h"          /* wvEvent() */
#endif
#include "vTypes.h"
#include <arch/ppc/ivPpc.h> /* INUM_TO_IVEC */
#include <cacheLib.h>
#include <intLib.h>
#include <stdio.h>
#include "logMsgLib.h"
#include "DDR_Common.h"
#include "DDR_Globals.h"
#include "fpgaBaseISR.h"
#include "DDR_Acq.h"
#include "fifoFuncs.h"
#include "cntrlFifoBufObj.h"
#include "nvhardware.h"
#include "FFKEYS.h"
#include "dmaMsgQueue.h"
#include "errorcodes.h"
#include "AParser.h"


extern ACODE_ID pTheAcodeObject;
extern int il_flag;

#define DEFAULT_DDROS_FILENAME "ddr.bin"

static char osname[256];
static char basepath[256];

static int last_il_fid_ct = 0;
extern int ddr_il_incr;

//#define ENABLE_ADC_OVF   // enable to send new warning for each fid

//###################  FPGA interrupt ISR #############################

//=========================================================================
// ddr_fpga_hw_isr: external interrupt hw ISR (405gp)
//   - handles non-system interrupts from the FPGA
// 1. DDR messages (GP_I1_INT)
//   - signaled by host_int in FPGA interrupt register unit 0
//   - gives semaphore to DDR message reader (dmsg_reader_task)
// 2.  SCAN boundary interrupt (SYNC_I0_INT)
//   - generated from output fifo SCAN bit (rising edge)
//   - gives semaphore (scan_sem) to real or emulated parser task
//=========================================================================
void ddr_fpga_hw_isr(int I_status, int userID) 
{ 
    extern int adc_ovf_count,enable_adc_ovf; 
    static msg_struct msg;
    int retry=0;
    //if(ddr_debug & DDR_DEBUG_INTS)
    //    logMsg("ddr_fpga_hw_isr: 0x%0.8X\n",I_status,0,0,0,0,0);
    if(I_status & GP_I1_INT){
        semGive(ddr_msg_sem);  // unblock dmsg_reader_task
    }
    if(I_status & SYNC_I1_INT){  // new FID
        scan_count++;
        /* logMsg("il_flag: %d, fid_count: %d fid_ct: %d, nfids: %d\n", il_flag,fid_count,fid_ct,pTheAcodeObject->num_acode_sets,0,0); */
        /* for Interleave acquisition, fid_count and fid_ct must be set at 
         * each interleave cycle with the proper values */
        if ((il_flag == 1) && (fid_count+1 > pTheAcodeObject->num_acode_sets - 1))
        {
	     fid_count = 0;   /* go back to 1st FID */
	     ddr_il_incr++; 
             last_il_fid_ct = fid_ct + 1;   /* this will be the start CT count on the next interleave cycle */
             /* logMsg("@ il_cycle - fid_count: %d, last_il_fid_ct: %dd\n", fid_count,last_il_fid_ct,0,0,0,0); */
        }

        /* each FID in the interleave cycle must set the CT back to the starting CT of the FID */
        if ( (il_flag == 1) && (ddr_il_incr > 0) )
        {
           fid_ct = last_il_fid_ct;
           /* logMsg("il - fid_count: %d, last_il_fid_ct: %d,  fid_ct: %d, nfids: %d\n", 
            * fid_count,last_il_fid_ct,fid_ct,pTheAcodeObject->num_acode_sets,0,0); */
        }
        else
        {
          fid_ct=1;
        }
        fid_count++;
        if(enable_adc_ovf)
            adc_ovf_count=0;
        if (ddr_run_mode != STAND_ALONE)
            sendFidCtStatus();
        if(ddr_debug & DDR_DEBUG_INTS)
            logMsg("FID INT fid %d ct %d scan %d ovfcnt %d\n",
                fid_count,fid_ct,scan_count,adc_ovf_count,0,0);
        if(scan_sem) // test mode parser
            semGive(scan_sem);     // unblock scan flag task
    }
    if(I_status & SYNC_I0_INT){  // new CT
        scan_count++;
        fid_ct++;
        if (ddr_run_mode != STAND_ALONE)
           sendFidCtStatus();
        if(ddr_debug & DDR_DEBUG_INTS)
            logMsg("SCAN INT fid %d ct %d scan: %d\n",
                fid_count,fid_ct,scan_count,0,0,0);
        if(scan_sem) // test mode parser
            semGive(scan_sem);     // unblock scan flag task
    }
    if(I_status & GP_I2_INT){  // C67->405 message queue overflow error
        logMsg("ERROR C67->405 message queue overflow scan:%d\n",scan_count,0,0,0,0,0);
        disableMSGInts();
        abort_exp(DDR_DSP_C67Q_OVRFLW); // can't use msgsFromDDR queue 'cause it's probably full 
    }
    if(I_status & ADF_OVF_INT){
        disableExpInts();
        msg.id=HOST_ERROR;
        msg.arg1=DDR_ERROR_ADF_OVF;
        msg.arg2=scan_count;
        msg.arg3=0;
        msgQSend(msgsFromDDR,(char*)&msg,sizeof(msg_struct),NO_WAIT,MSG_PRI_NORMAL);
    }
    if(I_status & OTF_OVF_INT){
        msg.id=HOST_ERROR;
        msg.arg1=DDR_ERROR_OVF;
        msg.arg2=scan_count;
        msg.arg3=0;
        msgQSend(msgsFromDDR,(char*)&msg,sizeof(msg_struct),NO_WAIT,MSG_PRI_NORMAL);
     }
    if(I_status & OTF_UNF_INT){
        msg.id=HOST_ERROR;
        msg.arg1=DDR_ERROR_UNF;
        msg.arg2=fid_count;
        msg.arg3=fid_ct;
        msgQSend(msgsFromDDR,(char*)&msg,sizeof(msg_struct),NO_WAIT,MSG_PRI_NORMAL);
    }
}

//###################  printf utilities #############################

//=========================================================================
// printID: print the FPGA id value
//=========================================================================
void printID()
{
    int id = get_field(DDR,ID);
    int rev = get_field(DDR,revision);
    int csum = get_field(DDR,checksum);

    printf("DDR FPGA ID:%d REV:%d CHECKSUM:%d\n",id,rev,csum);
}

//=========================================================================
// printEBCerr: show EBC error registers
//=========================================================================
void printEBCerr()
{ 
   printf("EBC error address 0x%0.8x\n",sysDcrEbcGet(0x20));       
   printf("EBC error status  0x%0.8x\n",sysDcrEbcGet(0x21));       
}

//=========================================================================
// printOTFStatus: show OTF status
//=========================================================================
void printOTFStatus()
{
    printf("OTF STAT  0x%-8X CTL 0x%-8X  SEL 0x%-8X\n",
        OTF_STATUS, OTF_CTL, OTF_OUT_SEL);
    printf("OTF ICNT    %-8d DCNT  %-8d\n", OTF_ICNT, OTF_DCNT);
    printf("OTF DUR1    %-8d DUR2  %-8d\n", DUR1, DUR2);
}

//=========================================================================
// printHPIBase: show HPI base register status
//=========================================================================
void printHPIBase(int i)
{
    long l,a,b;
  
    switch(i){
    case 0:
        l=HPI_BASE_REG0;
        a=l+(EMIF_MASK&EMIF_BASE0);
        b=EMIF_BASE0;
        break;
    case 1:
        l=HPI_BASE_REG1;
        a=l+(EMIF_MASK&EMIF_BASE1);
        b=EMIF_BASE1;
        break;
    case 2:
        l=HPI_BASE_REG2;
        a=l+(EMIF_MASK&EMIF_BASE2);
        b=EMIF_BASE2;
        break;
    default:
        printf("HPI BASE %d <not supported>\n",i);
        return;
    }
    printf("HPI BASE %d   0x%0.8X C6:0x%0.8X 405:0x%0.8X\n",i,l,a,b);
}

//=========================================================================
// printHPIStatus: show register status
//=========================================================================
void printHPIStatus()
{
    printHPIBase(0);
    printHPIBase(1);
    printHPIBase(2);
    printf("HPI RETRIES  %d\n",HPI_TIMEOUT);
    printf("HPI AUTOINC  %s\n",FPGA_HPI_AUTO?"YES":"NO");
    printf("HPI BUSY     %s\n",HPI_BUSY?"YES":"NO");
    printf("HPI TIMING   0x%0.8X\n",HPI_TIMING);
}

//=========================================================================
// printFPGAStatus: show register status
//=========================================================================
void printFPGAStatus()
{
    printf("EMIF STATE   0x%0.8X\n",EMIF_STATE);
    printf("EMIF CLK     0x%0.8X\n",EMIF_CLK_STAT);
#ifdef DDR_SCLKClockStatus
    printf("SCLK CLK     0x%0.8X\n",SCLK_CLK_STAT);
#endif
    printf("FPGA TIMEOUT %d\n",FPGA_TIMEOUT);
}

//=========================================================================
// printADCregs: show register status
//=========================================================================
void printADCregs()
{
    printf("ARM     0x%0.8X\n",ADF_ARM);
    printf("CTRL    0x%0.8X\n",ADF_CTL);
    printf("COUNT   %d\n",ADF_CNT);
    printf("READ    %d\n",ADF_RPTR);
}

//=========================================================================
// printIntStats: print FPGA interrupt register's status
//=========================================================================
void printIntStats()
{
   printf("GP  INT        0x%-0.8X\n",GP_INT);
   printf("404 IRQ  enbl: 0x%-0.8X stat: 0x%-0.8X\n",IRQ_ENBL,IRQ_STAT);
   printf("C67 INT4 enbl: 0x%-0.8X stat: 0x%-0.8X\n",C6I4_ENBL,C6I4_STAT);
   printf("C67 INT5 enbl: 0x%-0.8X stat: 0x%-0.8X\n",C6I5_ENBL,C6I5_STAT);
   printf("C67 INT6 enbl: 0x%-0.8X stat: 0x%-0.8X\n",C6I6_ENBL,C6I6_STAT);
   printf("C67 INT7 enbl: 0x%-0.8X stat: 0x%-0.8X\n",C6I7_ENBL,C6I7_STAT);
   printf("C67 NMI  enbl: 0x%-0.8X stat: 0x%-0.8X\n",NMI_ENBL,NMI_STAT);
}

//###################  405 utilities #############################

//=========================================================================
// setEBCTimeout: set EBC timeout register
//=========================================================================
void setEBCTimeout(int m)
{
    if(m==0)
        sysDcrEbcSet(0x23,0xF8480000);  // no timeout
    else if(m==1)
        sysDcrEbcSet(0x23,0xa0400000);  // 256 cycles
    else
        sysDcrEbcSet(0x23,0xB8480000);  // 2048 cycles
}

//=========================================================================
// getEBCTimeout: get EBC timeout register
//=========================================================================
int getEBCTimeout()
{        
   return sysDcrEbcGet(0x23);
}

//=========================================================================
// setJ1enable: enable J1 outputs
//=========================================================================
void setJ1enable(int enbl)
{        
    set_field(DDR,J1_C_en,enbl);
}
//=========================================================================
// getJ1enable: return J1 enable state
//=========================================================================
int getJ1enable()
{
   return get_field(DDR,J1_C_en);
}

//=========================================================================
// setJ2enable: enable J2 outputs
//=========================================================================
void setJ2enable(int enbl)
{        
    set_field(DDR,J2_C_en,enbl);
}

//=========================================================================
// getJ2enable: return J2 enable state
//=========================================================================
int getJ2enable()
{
   return get_field(DDR,J2_C_en);
}

//=========================================================================
// setGatesPolarity: set J2 gates polarity
//=========================================================================
void setGatesPolarity(int pol)
{ 
#ifdef DDR_FIFOGatesOutputPolarity       
    set_field(DDR,fifo_gates_output_polarity,pol);
#endif
}

//=========================================================================
// setGatesPolarity: get J2 gates polarity
//=========================================================================
int getGatesPolarity()
{        
#ifdef DDR_FIFOGatesOutputPolarity       
    return get_field(DDR,fifo_gates_output_polarity);
#else
    return 0;
#endif
}

//###################  HPI utilities #############################

//=========================================================================
// setHPIBase: set indexed HPI base address register
//=========================================================================
void *setHPIBase(int base, int adrs, int autoi)
{
    if(autoi)
        FPGA_HPI_AUTO=1;
    else
        FPGA_HPI_AUTO=0;
    switch(base){
    case 0:
        HPI_BASE_REG0=adrs-(EMIF_MASK&EMIF_BASE0);
        return (void*)EMIF_BASE0;
    case 1:
        HPI_BASE_REG1=adrs-(EMIF_MASK&EMIF_BASE1);
        return (void*)EMIF_BASE1;
    case 2:
        HPI_BASE_REG2=adrs-(EMIF_MASK&EMIF_BASE2);
        return (void*)EMIF_BASE2;
    default:
        return (void*)0;
    }
}

//=========================================================================
// resetHPIBase: reset indexed HPI base address register
//=========================================================================
void *resetHPIBase(int base)
{
    FPGA_HPI_AUTO=1;
    switch(base){
    case 0:
        return setHPIBase(0,DDR_MSGS_ADRS,1);
    case 1:
        return setHPIBase(1,DDR_CODE_ADRS,1);
    case 2:
        return setHPIBase(2,DDR_DRAM_ADRS,1);
    default:
        return (void*)0;
    }
}

//=========================================================================
// resetHPIRegs: reset HPI registers
//=========================================================================
void resetHPIRegs()
{
    FPGA_HPI_AUTO=1;
    setHPIBase(0,DDR_MSGS_ADRS,1);
    setHPIBase(1,DDR_CODE_ADRS,1);
    setHPIBase(2,DDR_DRAM_ADRS,1);
}

//=========================================================================
// getHPIBase: get indexed HPI base address register
//=========================================================================
void *getHPIBase(int base)
{
    switch(base){
    case 0:
        return (void*)EMIF_BASE0;
    case 1:
        return (void*)EMIF_BASE1;
    case 2:
        return (void*)EMIF_BASE2;
    default:
        return (void*)0;
    }
}

//=========================================================================
// setC6xBase: get indexed HPI base address register
//=========================================================================
void  *setC6xBase(int adrs)
{
     return setHPIBase(0,adrs,1);
}

//=========================================================================
// getHPITiming(): return HPI timing register 
//=========================================================================
int getHPITiming()
{
     return HPI_TIMING;
}

//=========================================================================
// setHPITiming(): set HPI timing register 
//=========================================================================
void setHPITiming(int  v)
{
     HPI_TIMING=v;
}

//=========================================================================
// slowHPITiming(): set HPI timing register 
//=========================================================================
void slowHPITiming()
{
     HPI_TIMING=LOW_SPEED_HPI;
}

//=========================================================================
// fastHPITiming(): set HPI timing register 
//=========================================================================
void fastHPITiming()
{
     HPI_TIMING=HIGH_SPEED_HPI;
}

//=========================================================================
// getEMIFState(): return EMIF state register 
//=========================================================================
int getEMIFState()
{
     return EMIF_STATE;
}

//=========================================================================
// resetSCLKClock(): enable SCLK clock DLL
//=========================================================================
void resetSCLKClock()
{
#ifdef DDR_SCLKClockControl
     SCLK_CLK_CTRL=1;
     SCLK_CLK_CTRL=0;
#endif
     taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
}

//=========================================================================
// resetEMIFClock(): enable EMIF clock DLL
//=========================================================================
void resetEMIFClock()
{
     EMIF_CLK_CTRL=1;
     EMIF_CLK_CTRL=0;
     taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    // printf("EMIF CLK     %s\n",EMIF_CLK_STAT?"LOCKED":"UNLOCKED");
}

//=========================================================================
// setFPGATimeout: set FPGA timeout register
//=========================================================================
void setFPGATimeout(int m)
{
   FPGA_TIMEOUT=m;
}

//=========================================================================
// getFPGATimeout: get FPGA timeout register
//=========================================================================
int getFPGATimeout()
{        
   return FPGA_TIMEOUT;
}

//=========================================================================
// resetHPI: reset the HPI
//=========================================================================
void resetHPI()
{
    setC6xBase(0);
    FPGA_HPI_AUTO=0;
    FPGA_HPIC=1;
    C6MEM(0x200) = HPIC_HWOB;
    FPGA_HPIC=0;
}

//=========================================================================
// resetC67: reset the C67 (toggle the reset line)
//=========================================================================
void resetC67()
{
#ifdef DEBUG_DDR_INIT
    printf("C67 reset\n");
#endif
    C67_RESET=0;
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    C67_RESET=1;
    
    slowHPITiming();
    
    resetHPI();
}
//=========================================================================
// setHPIretries: set number of HPI retries
//=========================================================================
void setHPIretries(int m)
{
    HPI_TIMEOUT=m;
}

//=========================================================================
// getHPIretries: get number of HPI retries
//=========================================================================
int getHPIretries()
{
   return HPI_TIMEOUT;
}


//=========================================================================
// ddrTask: DDR task initialization function utility
//=========================================================================
int ddrTask(char *name, void* task, int priority)
{
    int tid=taskNameToId(name);
    
    if(tid != ERROR)
        taskDelete(tid);
    
    if(tid=taskSpawn(name,priority,0,10000,
        task,0,0,0,0,0,0,0,0,0,0)==ERROR)
            //perror("ddrTask:%s",name); 
            perror("ddrTask"); 
    return tid;
}

//=========================================================================
// blinkLED: routine that will blink an led when vxWorks is running
//=========================================================================
//  from winSH:  -> show_active starts the thread
//               -> td "show_active" kills the thread
//=========================================================================
static void blinkLED()
{ 
    static int count=0;
    int leds=GET_LEDS;
  
    while (1)
    {
        if(count)
            BIT_ON(leds,HOST_ACTIVE_LED);
        else
            BIT_OFF(leds,HOST_ACTIVE_LED);
        SET_LEDS(leds);
        count=count?0:1;
        taskDelay(calcSysClkTicks(83));  /* taskDelay(5); */
    }
}
void show_active()
{
    if (taskSpawn("show_active",120,0,1000, 
         (void *) blinkLED,0,0, 0,0,0,0,0,0,0,0) == ERROR)
              perror("taskSpawn");
}

//###################  DDR specific section ###############################

//=========================================================================
//  initialize the C67 PLL 
//  1. C67 CLKIN = 40 MHz
//  2. C67 CPUCLK=220 MHz
//  3. EMIF clk = 88 MHz
//=========================================================================
#define PLL_PID         *((volatile unsigned int *)(EMIF_BASE + 0x000))
#define PLL_CSR         *((volatile unsigned int *)(EMIF_BASE + 0x100))
#define PLL_MULT        *((volatile unsigned int *)(EMIF_BASE + 0x110))
#define PLL_DIV0        *((volatile unsigned int *)(EMIF_BASE + 0x114))
#define PLL_DIV1        *((volatile unsigned int *)(EMIF_BASE + 0x118))
#define PLL_DIV2        *((volatile unsigned int *)(EMIF_BASE + 0x11C))
#define PLL_DIV3        *((volatile unsigned int *)(EMIF_BASE + 0x120))
#define PLL_OSCDIV1     *((volatile unsigned int *)(EMIF_BASE + 0x124))

#define CSR_PLLEN          0x00000001
#define CSR_PLLPWRDN       0x00000002
#define CSR_PLLRST         0x00000008 
#define CSR_PLLSTABLE      0x00000040
#define DIV_ENABLE         0x00008000

//=========================================================================
// getC6Speed(): return C67 CPU speed
//=========================================================================
double getC6Speed()
{
    int m,d;
    double g;
    setC6xBase(DDR_PLL_REGS);
    m=PLL_MULT;
    d=PLL_DIV1;    
    d&=0xFF;
    d+=1;
    g= 40.0e6*m/d;
    PRINT3("MULT:%d DIV1:%d CPU:%d MHz",m,d,(int)g/1e6);
    return g;
}

//=========================================================================
// getEMIFSpeed(): return C67 EMIF speed
//=========================================================================
double getEMIFSpeed()
{
    int m,d;
    double g;
    setC6xBase(DDR_PLL_REGS);
    m=PLL_MULT;
    d=PLL_DIV3;    
    d&=0xFF;
    d+=1;
    g= 40.0e6*m/d;
    PRINT3("MULT:%d DIV3:%d EMIF:%d MHz",m,d,(int)g/1e6);
    return g;
}

//=========================================================================
// printPLLregs(): show the current C67 PLL register values
//=========================================================================
void printPLLregs()
{
    setC6xBase(DDR_PLL_REGS);    
    printf("---- C67 PLL registers --------------\n");
    printf(" PLL_PID   0x%-0.8X\n",PLL_PID);
    printf(" PLL_CSR   0x%-0.8X\n",PLL_CSR);
    printf(" PLL_MULT  0x%-0.8X\n",PLL_MULT);
    printf(" PLL_DIV0  0x%-0.8X\n",PLL_DIV0);
    printf(" PLL_DIV1  0x%-0.8X\n",PLL_DIV1);
    printf(" PLL_DIV2  0x%-0.8X\n",PLL_DIV2);
    printf(" PLL_DIV3  0x%-0.8X\n",PLL_DIV3);
    printf(" PLL_OSC1  0x%-0.8X\n",PLL_OSCDIV1);
}

//=========================================================================
// initPLL(): initialize the C6 PLL clocks (220 MHz cpu, 88 MHz ECLKOUT)
// - note: use this function only as a last resort since TI's documentation
//         specifically says not to set the PLL through the HPI
//=========================================================================
void initPLL()
{
    int csr;
    setC6xBase(DDR_PLL_REGS);
    
    // note: without this delay get a "machine check". why ??
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    csr=PLL_CSR;    
    csr  &= ~CSR_PLLEN;
    PLL_CSR     = csr;

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    // Reset the pll.  PLL takes 125ns to reset. 
    csr  |= CSR_PLLRST;
    PLL_CSR  = csr;

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
 
    // PLLOUT = CLKIN/(DIV0+1) * PLLM
    // 440    = 40/1 * 11
  
    PLL_DIV0    = DIV_ENABLE;
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    PLL_MULT    = 11;   // for C67 clock frequency=220 MHz  (EMIF=88MHz)

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    PLL_OSCDIV1 = DIV_ENABLE + 4;
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
   
    // Program in reverse order. 
    // DSP requires that pheriheral clocks be less than
    // 1/2 the CPU clock at all times.

    PLL_DIV3    = DIV_ENABLE + 4;  // set the EMIF clock
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    PLL_DIV2    = DIV_ENABLE + 3;   // set the peripheral clock
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
   
    PLL_DIV1    = DIV_ENABLE + 1;   // set the DSP clock
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    // take the PLL out of reset
    
    csr  &= ~CSR_PLLRST;
    PLL_CSR  = csr;
    taskDelay(calcSysClkTicks(34));  /* taskDelay(2); */

    // enable the PLL

    csr  |= CSR_PLLEN;
    PLL_CSR  = csr;
#ifdef DEBUG_DDR_INIT
    printPLLregs();
#endif
    fastHPITiming();
}

//=========================================================================
//  initialize the C67 EMIF 
//=========================================================================
#define EMIF_GCTL         *((volatile unsigned int *)(EMIF_BASE+0x00))
#define EMIF_CE0          *((volatile unsigned int *)(EMIF_BASE+0x08))
#define EMIF_CE1          *((volatile unsigned int *)(EMIF_BASE+0x04))
#define EMIF_CE2          *((volatile unsigned int *)(EMIF_BASE+0x10))
#define EMIF_CE3          *((volatile unsigned int *)(EMIF_BASE+0x14))
#define EMIF_SDRAMCTL     *((volatile unsigned int *)(EMIF_BASE+0x18))
#define EMIF_SDRAMTIM     *((volatile unsigned int *)(EMIF_BASE+0x1C))
#define EMIF_SDRAMEXT     *((volatile unsigned int *)(EMIF_BASE+0x20))

//=========================================================================
// printEMIFregs: show the current C67 EMIF register values
//=========================================================================
void printEMIFregs()
{
    setC6xBase(DDR_EMIF_REGS);
    printf("---- C67 EMIF registers --------------\n");
    printf(" EMIF_GCTL 0x%-0.8X\n",EMIF_GCTL);
    printf(" EMIF_CE0  0x%-0.8X\n",EMIF_CE0);
    printf(" EMIF_CE1  0x%-0.8X\n",EMIF_CE1);
    printf(" EMIF_CE2  0x%-0.8X\n",EMIF_CE2);
    printf(" EMIF_CE3  0x%-0.8X\n",EMIF_CE3);
    printf(" EMIF_CTL  0x%-0.8X\n",EMIF_SDRAMCTL);
    printf(" EMIF_TIM  0x%-0.8X\n",EMIF_SDRAMTIM);
    printf(" EMIF_EXT  0x%-0.8X\n",EMIF_SDRAMEXT);
}

//=========================================================================
// initEMIF: initialize the EMIF registers
//=========================================================================
void initEMIF()
{
    setC6xBase(DDR_EMIF_REGS);
    EMIF_GCTL=0x00003068;
    EMIF_CE0=0xFFFFFF33;
    EMIF_CE1=0xFFFFFF33;
    EMIF_CE2=0x00404120;
    EMIF_CE3=0x00404140;
    EMIF_SDRAMTIM=0x000002B0;
    EMIF_SDRAMEXT=0x00054529;
    EMIF_SDRAMCTL=0x63115000;  // set this last !  
#ifdef DEBUG_DDR_INIT
    printEMIFregs();
#endif
}

//=========================================================================
// writeDDRdata: copy data from 405 to C67 memory 
//=========================================================================
void writeDDRdata(unsigned int *src, unsigned int adrs, int size)
{
    int i;  
    UINT32 *dst;
    dst=(UINT32*)setHPIBase(OS_WRITE_HPI_BASE,adrs,1);
    for(i=0;i<size;i++)
        dst[i]=src[i];       
}

//=========================================================================
// writeDDRdata: copy buffer data from 405 to C67 memory 
//=========================================================================
int writeDDRbuffer(char *memptr, UINT32 fSize)
{
    UINT32 *src;
    char *sptr,c,*smax,cval,*cptr;
    int status,ccnt,aflag,i,dptr,adrs,wcnt,*iptr,test,errs=0;
    int total_size=0,total_errs=0;

    /* printf("------------>>>  writeBuffer(): 0x%lx, size: %lu\n",memptr,fSize); */
    iptr = (int *) memptr;
    smax=memptr+fSize-2;
    while (iptr < (int*)smax) 
    {
        adrs = iptr[0];
        wcnt = iptr[1];
        if(wcnt==0)
            break;
        writeDDRdata(iptr+2,adrs,wcnt);

#ifdef DEBUG_DDR_INIT
        printf("OS WRITE  adrs:%-0.8lX size:%d\n",adrs,wcnt);
#endif
        iptr += wcnt + 2;
        if(!wcnt || !iptr[1])
            break;
    }

#ifdef VERIFY_OS

    iptr = (int *) memptr;

    smax=memptr+fSize-2;
    while (iptr < (int*)smax) 
    {
        adrs = iptr[0];
        wcnt = iptr[1];
        if(wcnt==0)
            break;
        
        src=(UINT32*)setHPIBase(OS_WRITE_HPI_BASE,adrs,1);  

        total_size+=wcnt;
        total_errs+=errs;
#ifdef DEBUG_DDR_INIT
        printf("OS VERIFY adrs:%-0.8lX size:%d\n",adrs,wcnt);
#endif      
        errs=0;
        for(i=0;i<wcnt;i++)
        {
            test=src[i];
            if(iptr[i+2]!=test)
                errs++;
        }   
        if(errs)   
           printf("%d ERRORS in OS block %-0.8lX\n",errs,adrs);
        iptr += wcnt + 2;
        if(!wcnt || !iptr[1])
            break;
    }
#endif        
    return 1;
}

//=========================================================================
// writeDDROS: copy a DDR operating system from flash to C67 memory
//     1. data format: ADRS CNT DATA....ADRS CNT DATA.... ...0 0
//            ADRS = C67 mapped address for base of block (e.g. 0x81000100)
//            CNT  = number of words (32 bits) in block.
//            DATA = 32-bit data values
//
//      2. sequence ends when CNTi+1=ADRSi+CNTi+1=0 (next block is zero size)
//      3. file conversion
//         -".obj" file generated by CCS converted to ".hex" using TI's
//           "hex6x" utility. (conversion runs a dos script, "makehex.com")
//         - ".hex" file converted to binary (.bin) using hex2bin,
//           a new unix based utility program (see hex2bin.c in nvddr)    
//=========================================================================
int writeDDROS(char *name)
{
    char *memptr=0,*sptr,c,*smax,cval,*cptr;
    char *mbuf;
    UINT32 fSize;
    UINT32 bufCnt;
    UINT32 rc;
    int status,ccnt,aflag,i,dptr,adrs,wcnt,*iptr,test,errs=0;
    int total_size=0,total_errs=0;
    UINT32 *src;
    char file_name[256];
    int fd;
    char memname[80]; 
    static int memKey = 0; 
    if(basepath[0])    
        sprintf(file_name,"%s/%s",basepath,name);
    else
        strcpy(file_name,name);
        
    sprintf(memname,"/mem/ddr%u",memKey++); 
    fd = getFile_fd( file_name, &fSize, memname, &mbuf ); 
    if(fd<=0){
        printf("File: '%s' not found\n",file_name);
        return 0;
    }
    memptr = malloc(fSize);
    if(!memptr){
        printf("could not malloc data buffer for %s\n",file_name);
        return 0;
    }
    read(fd,memptr,fSize);

    /* write boot or DDR file over HPI to DSP */
    status = writeDDRbuffer(memptr, fSize);

    // close file
    closeFile_fd(fd, memname, mbuf); 
    free(memptr);
#ifdef DEBUG_DDR_INIT
        printf("OS TRANSFER COMPLETE WRDS: %d ERRS:%d\n",total_size,total_errs);
#endif
    return 1;
}

//=========================================================================
// HPIboot: start the DDR os (HPI boot)
//=========================================================================
void HPIboot()
{
    // note: The DDR os code must be bootable, i.e. it must have an instruction
    //       at EMIF address 0x00000000 that jumps to BIOS start.
    FPGA_HPIC=1;
    C6MEM(0) = HPIC_DSPINT;
    FPGA_HPIC=0;
}

//=========================================================================
// loadBoot: DDR initialization function
//=========================================================================
int loadBoot()
{
    resetC67();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
        
    resetSCLKClock();
    resetEMIFClock();  // sync on 20 MHz ECLKOUT

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
        
    // copy the boot program from flash to C6 memory

#ifdef XXXXX
#ifndef DSP_BIN_COMPILED_IN

    if(!writeDDROS("boot.bin")){
        DPRINT(-1,"ERROR: could not copy boot.bin into C6 memory");
        return 0;
    } 

#else

    loadDspBootArray();

#endif
#endif

    if( writeDDROS("boot.bin") == 1)
    {
        DPRINT(-1,"boot.bin copied into C6 memory");
    } 
    else
    {
        DPRINT(-1,"Loading DSP Boot Array into C6 memory");
        if ( loadDspBootArray() == -1)
        {
            DPRINT(-1,"ERROR: could not load boot array into C6 memory");
            return 0;
        }
    }

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    
    // HPI boot  (boot program sets EMIF and PLL registers)
    
    HPIboot();  // start boot program (sets PLL and EMIF regs)
    taskDelay(calcSysClkTicks(166));  /* taskDelay(10); */
    
    fastHPITiming();
    resetSCLKClock();
    resetEMIFClock();  // sync on 88 (80) MHz ECLKOUT 
    return 1; // success
}

//=========================================================================
// startDDR: DDR initialization function
//=========================================================================
int startDDR()
{
#define MAXRETRIES 10
    int retries=0;
    int ticks;
    ticks = calcSysClkTicks(17);
    GP_INT=0;
    GP_INT=0x08;
    while(retries<MAXRETRIES){
        if(!(GP_INT & 0x08)){
            DPRINT(-1,"DDROS started");
            return 1;
        }
        taskDelay(ticks);  /* taskDelay(1); */
        retries++;
    } 
    DPRINT(-1,"ERROR: DDROS failed to start");
    return 0;   
}

//=========================================================================
// resetSDRAM: DDR initialization function
//=========================================================================
void resetSDRAM()
{
    setC6xBase(DDR_EMIF_REGS);
    EMIF_SDRAMCTL=0x63115000;
}

//=========================================================================
// loadDDR: DDR initialization function
//=========================================================================
int loadDDR(char *name)
{
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    if(!loadBoot())
        return 0;   
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

    resetSDRAM(); // without this boot fails (mem errors)

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    
#ifdef XXXXX
#ifndef DSP_BIN_COMPILED_IN

    writeDDROS(name);   // copy the DDR os to C6 memory

#else

    loadDspDDRArray();

#endif
#endif


    if( writeDDROS(name) == 1)
    {
        DPRINT1(-1,"'%s': copied into C6 memory",name);
    } 
    else
    {
        DPRINT(-1,"Loading DSP DDR Array into C6 memory");
        if ( loadDspDDRArray() == -1)
        {
            DPRINT(-1,"ERROR: could not load DSP DDR array into C6 memory");
            return 0;
        }
    }

    strcpy(osname,name);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    
    // send a signal to bootup program (causes a jump to adrs:0)
    total_ddr_msgs=total_host_msgs=0;
    return startDDR();
}

//=========================================================================
// reloadDDR: DDR initialization function
//=========================================================================
int reloadDDR()
{
    if(osname[0])
        return loadDDR(osname);
    else
        return loadDDR(DEFAULT_DDROS_FILENAME);
}

//=========================================================================
// resetEMIF set EMIF and PLL registers. reset C67
//=========================================================================
void resetEMIF()
{
    resetC67();  
    
    resetEMIFClock();

    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    initPLL();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    resetEMIFClock();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    initEMIF();
}

//##################### required by external functions ####################

extern enableMSGInts();
extern set_msgs_base();

//=========================================================================
// initDDR: DDR initialization function
//=========================================================================
int initDDR(int mode,char *basePath)
{
    osname[0]=0;
    basepath[0]=0;
 
    ddr_run_mode=mode;
    if(basePath)
        strcpy(basepath,basePath);
        
    setEBCTimeout(2);
    fpgaIntConnect(ddr_fpga_hw_isr,IRQ_INT_MASK,IRQ_INT_MASK);
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    
    reloadDDR();
    resetHPIRegs();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    initAD6634();
    initDDRmsgs();
    initDDRData();
    initDDRScan();
    taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
    enableMSGInts();
    
    setGatesPolarity(DDR_RG); // invert receiver gate polarity

    C6Speed=getC6Speed();
    EMIFSpeed=getEMIFSpeed();
    return 1;
}
