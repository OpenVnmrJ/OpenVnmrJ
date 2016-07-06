/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */ 
#ifndef DDR_COMMON_H
#define DDR_COMMON_H

// ------------- Make C header file C++ compliant -------------------

#ifdef __cplusplus
extern "C" {
#endif

#include "ddr.h"
#include "ddr_fifo.h"
#include "ddr_symbols.h"

#ifdef DDR_ScratchRAM
//#define USE_FPGA_MSG_BUFFER
#define FPGA_BRAM  FPGA_BASE+DDR_ScratchRAM
#endif

//============= function prototypes =================================

//------------- HPI -----------------------------------------

extern void *setHPIBase(int base, int adrs, int autoi);
extern void *getHPIBase(int base);
extern void *setC6xBase(int adrs);
extern void *resetHPIBase(int base);
extern void resetHPIRegs();

//------------- OutPut FIFO -----------------------------------------

void clearCumDur();
void softStartOTF();
void syncStartOTF();
void stopOTF();
void resetOTF();

//------------- ADC data FIFO ---------------------------------------

void setCollectMode(int m);
void setCollect(int m);
int getADFcount();
void setADFread(int i);
int getADFread();
void setTripPoint(int m);
int getTripPoint();
void setADCDebug(int m);
void clearADCFIFO();
void enableADCFIFO();

//------------- AD6634 ---------------------------------------
void setSamplingRate(double sa);
double getSamplingRate();
void softStartAD6634();
void pinStartAD6634();
void clearStartAD6634();
void stopAD6634();
void pinStartAD6634();
void setSoftwareGates(int i);
int getSoftwareGates();
void select_chnl(int ch);
void set_NCO_freq(double val);
double get_NCO_freq();
void set_NCO_phase(double val);
double get_NCO_phase();
void set_NCO_dither(int val);
int get_NCO_dither();
void set_NCO_clear(int val);
int get_NCO_clear();
void set_IEN_mode(int val);
int get_IEN_mode();
void load_coeffs(int *table, int size);
int get_coeffs(int *table);
int getClockPhase();
void set_filter_option(int);
int get_filter_option();

//======================= utilities =================================

#ifndef BIT_ON
#define BIT_ON(a,b)     (a) |= (b)
#define BIT_OFF(a,b)    (a )&= ~(b)
#define BIT_TST(a,b)    a&b?1:0
#define BIT_SET(a,b,f)  if(f) BIT_ON(a,b); else BIT_OFF(a,b)
#define BIT_MSK(a,b,f)  BIT_OFF(a,b); BIT_ON(a,f)

#endif

#define LSFT(x)   (1<<(x))        // convert pos to bitfield
#define BMASK(x)  ((1<<(x))-1)    // convert width to bitmask
#define WSIZE(x)  ((1<<(x)))      // convert width bits to size

#define VUIPTR   (volatile unsigned int *)
#define VREG(x)  *(VUIPTR(FPGA_BASE+(x)))
#define VMEM(x)  *(VUIPTR(x))

//======================= misc DDR =================================
#define DDR_TMR_FREQ 80e6   // C67 cpu clock

#define ADC_FIFO ((unsigned int *)(0xB0000000))
#ifndef BLKSIZE
#define BLKSIZE 1000
#endif

//======================= C67 address map   =====================

// C67 EMIF memory map

#define DDR_IRAM_ADRS     0x00000000
#define DDR_MSGS_ADRS     0x00000200
#define DDR_REGS_ADRS     0x01800000
#define DDR_HPIC_ADRS     0x01880000
#define DDR_CODE_ADRS     0x80000000
#define DDR_ACQ_ADRS      0x81000000
#define DDR_DRAM_ADRS     0x90000000
#define DDR_ADC_ADRS      0xB0000000
#define DDR_PLL_REGS      0x01B7C000
#define DDR_CACHE_REGS    0x01840000
#define DDR_EMIF_REGS     0x01800000

#define CCFG              *VUIPTR(0x01840000)

// nmr data buffer in SDRAM (includes optional snapshot buffer)

#define DDR_DATA          (DDR_DRAM_ADRS)    // 0x90000000

//++++++++++++++++++++ FPGA registers ++++++++++++++++++++++++++++++

#ifdef C67
#define FPGA_BASE 0xA0000000
#else  // vxWorks
#define FPGA_BASE 0x70000000
#endif

#ifndef DDR_BASE
#define DDR_BASE FPGA_BASE
#endif

#define EMIF_BASE0 0x74000000
#define EMIF_BASE1 0x80000000
#define EMIF_BASE2 0x90000000
#define EMIF_BASE  EMIF_BASE0  // default
#define EMIF_MASK  0x07ffffff

#define C6MEM(x)        (*(VUIPTR(EMIF_BASE0+(x))))

//################### FPGA status ###################################

#define FPGA_ID         VREG(DDR_ID)
#define FPGA_LEDS       VREG(DDR_LED)

#define EXP_RUNNING_LED     0x10   // on when exp running, off when stopped
#define DDR_ACTIVE_LED      0x02   // C67 blinks when running
#define HOST_ACTIVE_LED     0x04   // 405 blinks when running

#define SET_LEDS(x)         FPGA_LEDS=0x3f&(~(x))
#define GET_LEDS            0x3f&(~FPGA_LEDS)
   
#define CHECKSUM_MASK BMASK(DDR_checksum_width)
#define FPGA_CHECKSUM ((FPGA_ID>>DDR_checksum_pos)&CHECKSUM_MASK)

//################### FPGA C67 ###################################

#define C67_RESET       VREG(DDR_C67Reset)
#define C67_CONFIG      VREG(DDR_C67Configuration)

//################### FPGA HPI ###################################

#define HPI_BASE_REG0   VREG(DDR_HPIBaseAddress0)
#define HPI_BASE_REG1   VREG(DDR_HPIBaseAddress1)
#define HPI_BASE_REG2   VREG(DDR_HPIBaseAddress2)

#define HPI_TIMING      VREG(DDR_HPITiming)
#define HPI_TIMEOUT     VREG(DDR_HPITimeout)
#define FPGA_TIMEOUT    VREG(DDR_EBCTimeout)
#define EMIF_STATE      VREG(DDR_EMIFState)
#define EMIF_CLK_STAT   VREG(DDR_EMIFClockStatus)
#define EMIF_CLK_CTRL   VREG(DDR_EMIFClockControl)
#define SCLK_CLK_CTRL   VREG(DDR_SCLKClockControl)
#define SCLK_CLK_STAT   VREG(DDR_SCLKClockStatus)
#define FPGA_HPI_AUTO   VREG(DDR_HPIAutoIncrement)
#define FPGA_HPIC       VREG(DDR_HPIControlAccess)
#define HPI_BUSY        VREG(DDR_HPIBusy)

#define HPIC_HWOB       0x00010001
#define HPIC_DSPINT     0x00020002
#define HPIC_HINT       0x00040004

//################### FPGA interrupts ################################

#define OTF_OVF_INT    LSFT(DDR_fifo_overflow_status_0_pos)  // OTFIFO overflow
#define OTF_UNF_INT    LSFT(DDR_fifo_underflow_status_0_pos) // OTFIFO underflow
#define OTF_DONE_INT   LSFT(DDR_fifo_finished_status_0_pos)  // program exp done
#define SYS_FAIL_INT   LSFT(DDR_fail_int_status_0_pos)       // system failure
#define SYS_WARN_INT   LSFT(DDR_warn_int_status_0_pos)       // system warning
#define HINT_INT       LSFT(DDR_host_int_status_0_pos)       // hpi interrupt
#define ADF_OVF_INT    LSFT(DDR_adc_overflow_int_status_0_pos)  // ADF overflow
#define ADF_UNF_INT    LSFT(DDR_adc_underflow_int_status_0_pos) // ADF underflow
#define BLKRDY_INT     LSFT(DDR_adc_threshold_int_status_0_pos) // ADF threshold
#define DATA_OVF_INT   LSFT(DDR_AD6634_OVF_int_status_0_pos)    // data clipped
#define SYNC_INT       LSFT(DDR_sync_int_status_0_pos)          // sync flag
#define GP_I0_INT      LSFT(DDR_gp_int_status_0_pos)            // gp int 0 bit 
#define GP_I1_INT      LSFT(DDR_gp_int_status_0_pos+1)          // gp int 1 bit
#define GP_I2_INT      LSFT(DDR_gp_int_status_0_pos+2)          // gp int 2 bit
#define GP_I3_INT      LSFT(DDR_gp_int_status_0_pos+3)          // gp int 3 bit
#define SYNC_I0_INT    LSFT(DDR_sync_int_status_0_pos)          // sync 0
#define SYNC_I1_INT    LSFT(DDR_sync_int_status_0_pos+1)        // sync 1
#define SYNC_I2_INT    LSFT(DDR_sync_int_status_0_pos+2)        // sync 2
#define SYNC_I3_INT    LSFT(DDR_sync_int_status_0_pos+3)        // sync 3

#define ACQ_INT_MASK   SYNC_I0_INT|SYNC_I1_INT|ADF_OVF_INT|OTF_OVF_INT|OTF_UNF_INT
#define IRQ_INT_MASK   GP_I2_INT|GP_I1_INT|ACQ_INT_MASK

#define GP_INT         VREG(DDR_GPInterrupt)

#define IRQ_STAT       VREG(DDR_InterruptStatus_0)
#define IRQ_ENBL       VREG(DDR_InterruptEnable_0)
#define IRQ_CLR        VREG(DDR_InterruptClear_0)

#define NMI_STAT       VREG(DDR_InterruptStatus_1)
#define NMI_ENBL       VREG(DDR_InterruptEnable_1)
#define NMI_CLR        VREG(DDR_InterruptClear_1)

#define C6I4_STAT      VREG(DDR_InterruptStatus_2)
#define C6I4_ENBL      VREG(DDR_InterruptEnable_2)
#define C6I4_CLR       VREG(DDR_InterruptClear_2)

#define C6I5_STAT      VREG(DDR_InterruptStatus_3)
#define C6I5_ENBL      VREG(DDR_InterruptEnable_3)
#define C6I5_CLR       VREG(DDR_InterruptClear_3)

#define C6I6_STAT      VREG(DDR_InterruptStatus_4)
#define C6I6_ENBL      VREG(DDR_InterruptEnable_4)
#define C6I6_CLR       VREG(DDR_InterruptClear_4)

#define C6I7_STAT      VREG(DDR_InterruptStatus_5)
#define C6I7_ENBL      VREG(DDR_InterruptEnable_5)
#define C6I7_CLR       VREG(DDR_InterruptClear_5)

//################### AD6634 microport ############################

#define AD6634_RST     VREG(DDR_AD6634Reset)
#define AD6634_DIT     VREG(DDR_DitherControl)

#define MICRO(a)   (*((volatile unsigned char*)(FPGA_BASE+0x00001000+a)))
#define WMICRO(a,b)  MICRO(a)=(unsigned char)(b)
#define RMICRO(a)    (unsigned char)MICRO(a)

// AD6634 external uport addresses

#define DR0     0
#define DR1     1
#define DR2     2
#define ACR     7
#define CAR     6
#define PSYNC   4  // write only
#define SLEEP   3  // write only
#define SSYNC   5  // write only

//################### ADC Data FIFO ##################################

#define ADF_CTL        VREG(DDR_ADCFifoControl)
#define ADF_ARM        VREG(DDR_ADCArmFIFO)
#define ADF_CNT        VREG(DDR_ADCCount)
#define ADF_RPTR       VREG(DDR_ADCReadPointer)
#ifdef DDR_ADCFifoDelay
#define ADF_DELAY      VREG(DDR_ADCFifoDelay)
#define ADF_MAXDELAY   BMASK(DDR_adc_collect_data_delay_width)
#endif

// ADCArmFIFO  register bitfields

#define ADC_COLLECT LSFT(DDR_adc_collect_data_pos)   // manual collect bit
#define ADC_START   LSFT(DDR_adc_arm_fifo_start_pos) // collection start mode
#define ADC_STOP    LSFT(DDR_adc_arm_fifo_stop_pos)  // collection stop mode
#define ADC_DEBUG   LSFT(DDR_adc_debug_mode_pos)     // collect fake data

// ADCFifoControl register bitfields

#define ADF_SIZE   BMASK(DDR_adc_trip_point_width) // depth of data fifo
#define TRIP_BITS  BMASK(DDR_adc_trip_point_width) // trip point bitmask
#define CLR_ADF    LSFT(DDR_adc_fifo_clear_pos)    // fifo clear bit in ADC_CTL 

//################### Output/Timer FIFO ##############################

// CumulativeDuration registers and bitfields

#define DUR_CLR         VREG(DDR_ClearCumulativeDuration)
#define DUR2            VREG(DDR_CumulativeDurationHigh)
#define DUR1            VREG(DDR_CumulativeDurationLow)
 
#define DUR_CLR_SET     LSFT(DDR_clear_cumulative_duration_pos)

//  FIFOControl register and bitfields

#define OTF_STATUS      VREG(DDR_FIFOStatus)
#define OTF_ICNT        VREG(DDR_InstructionFIFOCount)
#define OTF_DCNT        VREG(DDR_DataFIFOCount)
#define OTF_SIZE        WSIZE(DDR_instruction_fifo_count_width)

#define OTF             VREG(DDR_FIFOInstructionWrite)

//  DDR_FIFOControl register and bitfields

#define OTF_CTL         VREG(DDR_FIFOControl)

#define OTF_SOFT_START  LSFT(DDR_fifo_start_pos)
#define OTF_SYNC_START  LSFT(DDR_fifo_sync_start_pos)
#define OTF_RESET       LSFT(DDR_fifo_reset_pos)

//  FIFOOutputSelect register and bitfields

#define OTF_OUT_SEL     VREG(DDR_FIFOOutputSelect)
#define OTF_FIFO_OUT    LSFT(DDR_fifo_output_select_pos)
#define OTF_SOFT_OUT    0

//  Software Gates register and bitfields

#define SWGATES         VREG(DDR_SoftwareGates)

//  FIFO Gates instruction register and bitfields

#define RD_GATES        VREG(DDR_Gates)
#define RD_TIME         VREG(DDR_Duration)
#define TMAX            (0x03ffffff)        // time value bits
#define TMCNT(t)        (((t)&(~TMAX))>>26) // long times repeat tmax

#define TIME(x)          encode_DDRSetDuration(1,(x))
#define Time(x)          TIME(((x)*80.0e6)) // convert secs to clock ticks
#define GAIN(x)          encode_DDRSetGain(0,x)

//################### Other FPGA registers ##############################

//  Software controlled outputs

#define SW_OUTPUTS       VREG(DDR_SoftwareOutputs)

#define ACQUIRE  (1<<DDR_acquire_pos)
#define OVERLOAD (1<<DDR_overload_pos)

#define SWOUT1 (1<<DDR_sw_control_gpoutputs_pos)
#define SWOUT2 (2<<DDR_sw_control_gpoutputs_pos)
#define SWOUT3 (4<<DDR_sw_control_gpoutputs_pos)
#define SWOUT4 (8<<DDR_sw_control_gpoutputs_pos)

//  backplane output control

#define J1_OUTPUT       VREG(DDR_J1COutput)
#define J1_INPUT        VREG(DDR_J1CInput)
#define J1_ENABLE       VREG(DDR_J1CEnable)
#define J2_OUTPUT       VREG(DDR_J2COutput)
#define J2_INPUT        VREG(DDR_J2CInput)
#define J2_ENABLE       VREG(DDR_J2CEnable)

//++++++++++++++++++++ end FPGA registers +++++++++++++++++++++++++

//=======================   Message objects  =============================

typedef struct msg_struct 
{
    int     id;   // command ID 
    int   arg1;   // command argument 
    int   arg2;   // command argument 
    int   arg3;   // command argument 
} msg_struct;

#define MSG_BUFFER_SIZE         128      // max messages in shared fifos
#define MSG_GAP                 128     // max messages in shared fifos

#define MSG_DATA_SIZE           sizeof(msg_struct)
#define MSG_BUFF_SIZE           (MSG_DATA_SIZE*MSG_BUFFER_SIZE)

#ifdef C67
#define MSGS_BASE               (DDR_MSGS_ADRS)
#else
#define MSGS_BASE               (EMIF_BASE)
#endif

#ifdef USE_FPGA_MSG_BUFFER     

#define HOST_MSGS_WRITE         (MSGS_BASE)
#define DDR_MSGS_READ           (MSGS_BASE+4)
#define HOST_MSGS_READ          (MSGS_BASE+MSG_GAP)
#define DDR_MSGS_WRITE          (MSGS_BASE+MSG_GAP+4)
#define HOST_MSGS               (FPGA_BRAM)
#define DDR_MSGS                (FPGA_BRAM+0x500)

#else
#define HOST_MSGS               (MSGS_BASE)
#define HOST_MSGS_WRITE         (HOST_MSGS+MSG_BUFF_SIZE)
#define HOST_MSGS_READ          (HOST_MSGS+MSG_BUFF_SIZE+4)
#define DDR_MSGS                (MSGS_BASE+0x1000)
#define DDR_MSGS_WRITE          (DDR_MSGS+MSG_BUFF_SIZE)
#define DDR_MSGS_READ           (DDR_MSGS+MSG_BUFF_SIZE+4)
#endif
#define MSG_PTR(base,idx)        VUIPTR(base+MSG_DATA_SIZE*(idx))

#ifdef __cplusplus
}
#endif

#endif
