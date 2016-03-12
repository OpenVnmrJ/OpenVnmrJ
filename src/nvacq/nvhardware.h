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
*
*     Updated to Pete's RF COntroller Register   V1.4  7/7/2003
*           10/20/03
*
*/
#ifndef INCnvhardwareh
#define INCnvhardwareh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#include "fpga.h"

#define REG_ADDR(offset)  ( (volatile unsigned long* const) (FPGA_BASE_ADR + (offset)))

/* Duff Device Macro */
#define DUFF_DEVICE_8(countArg,actionArg) \
do {                                \
     int count_ = (countArg);       \
     int times_ = (count_ + 7) >>3; \
     switch(count_ & 7) {           \
      case 0: do { actionArg;       \
      case 7:      actionArg;       \
      case 6:      actionArg;       \
      case 5:      actionArg;       \
      case 4:      actionArg;       \
      case 3:      actionArg;       \
      case 2:      actionArg;       \
      case 1:      actionArg;       \
         } while (--times_ > 0);    \
     }                              \
   } while(0)



/* 405 GPIO Control register Addresses */
#define IBM405_GPIO_CR     ( (volatile unsigned long* const) CPC0_CR0 )
#define IBM405_GPIO0_OR    ( (volatile unsigned int* const) 0xEF600700 ) /* 24 bits R/W 405 GPIO Output */
#define IBM405_GPIO0_TCR   ( (volatile unsigned int* const) 0xEF600704 ) /* 24 bits R/W 405 GPIO 3-State Cntrl*/
#define IBM405_GPIO0_ODR   ( (volatile unsigned int* const) 0xEF600718 ) /* 24 bits R/W 405 GPIO Open Drain */
#define IBM405_GPIO0_IR    ( (volatile unsigned int* const) 0xEF60071C ) /* 24 bits RO 405 GPIO Input */

/* System Addresses of Interests */
/* from chamleon boot.h VWare ) */
#define FLASHSTART      0xFF000000              /* Main Flash Address */
#define FLASHSIZE       0x01000000              /* Main Flash Size    */
#define BOOTFLASHSTART  0xFFF80000              /* Boot Flash Start Address */
#define BOOTFLASHSIZE   0x00080000              /* Boot Flash Size */
#define FFSFLASHSTART   (FLASHSTART)    	/* Flash File System Start Address */
#define FFSFLASHSIZE    0x00F00000              /* Flash File System Size  */

/* FPGA Base Address */
#define FPGA_PRGM_BASE_ADR   0xF0000000L
#define FPGA_PRGM_CR         0x00000008L  	/* offset to FPGA Control register */
#define FPGA_PRGM_DR         0x00000009L  	/* offset to FPGA Programming Data Register */
#define FPGA_PRGM_SR         0x0000000CL  	/* offset to FPGA Status Register */

#define FPGA_EXT_INT_IRQ  25                    /* FPGA interrupt 405 - INT_LVL_EXT_IRQ_0=25 */

/* Basicly Follows Pete's FIFO Control register document, starting page 32 */

/*  Base Address from which all address offsets are added  */
#define FPGA_BASE_ADR   0x70000000L
#define MASTER_BASE     FPGA_BASE_ADR
#define RF_BASE         FPGA_BASE_ADR
#define LPFG_BASE       FPGA_BASE_ADR
#define LOCK_BASE       FPGA_BASE_ADR
#define PFG_BASE        FPGA_BASE_ADR
#define GRADIENT_BASE   FPGA_BASE_ADR
#ifndef DDR_BASE
#define DDR_BASE        FPGA_BASE_ADR
#endif

/* Board Type Defines */
#define MASTER_BRD_TYPE_ID 0
#define RF_BRD_TYPE_ID 1
#define PFG_BRD_TYPE_ID 2
#define GRAD_BRD_TYPE_ID 3
#define LOCK_BRD_TYPE_ID 4
#define DDR_BRD_TYPE_ID 5
#define LPFG_BRD_TYPE_ID 6
#define UNDEF2_BRD_TYPE_ID 7

/* Panel FPGA LEDs defines */
#define FPGA_PANEL_LED_REG ( (volatile unsigned long* const) (FPGA_BASE_ADR + 0x4))
#define LAN_TRIGGERED 1    /* FPGA directly controlled */
#define DSP_ACTIVE_LED 2
#define WAIT4SYNC_LED  3
#define FIFO_RUNNING_LED 4
#define DSP_TRIGGER_LED 5
#define DATA_XFER_LED 6


#define FPGA_ID_REG	0x00000000L     /* offfset to FIFO Id register read only */
/* --- FIFO ID definitions  ---- */
#define FPAG_CHIP_ID_MASK  0x0000000F   /* Chip ID bits 0-3 */
#define FPGA_CHIP_REV_MASK 0x000000F0   /* Chip Rev vits 4-7 */

#define FPGA_FIFO_CR	0x00000004L     /* offfset to FIFO Control register wonly */
/* --- FIFO CR Definition of Bits --- */
#define FIFO_START      0x0000001L      /* Start FIFO without waiting for ext sync */
#define FIFO_SYNC_START 0x0000002L      /* Start FIFO  on next rising edge of ext_sync */
#define FIFO_RESET      0x0000004L      /* Reset FIFO */


/* place FIFO high water mark for FIFO DMA device-paced DMA 6 words below full */
#define FIFO_HIWATER_MARK_HEADROOM  6  

/*------  SCLKReset Register ------ */
#define FPGA_SCLKRESET_REG   0x00000008L
#define SCLKDCM_RST      0x1		/* Reset the SCLK DCM  bit 0 */


/*------  SCLKLocked Register ------ */
#define FPGA_SCLKLOCKED_REG   0x0000000CL
#define SCLKDCM_LOCKED      0x1		/* Is SCLK DCM Locked? 1=yes  (bit-0) */

/*------  PerClkReset  Register ------ */
#define FPGA_PERCLKRESET_REG   0x00000010L
#define PERCLKDCM_RST 0x1		/* Reset the PerClk DCM (bit-0) */


/*------  PerClkLocked Register ------ */
#define FPGA_PERCLKLOCKED_REG   0x00000014L
#define PERCLKDCM_LOCKED      0x1	/* Is PERCLK DCM Locked? 1=yes  (bit-0) */


/*------  FPGA GPIO  Registers ------ */
#define GPIO_OUTPUT_REG	  0x00000018L	/* GPIO Output Register, 32-bit  */
#define GPIO_ENABLE_REG	  0x0000001CL	/* GPIO Enable Register, GPIO active high output enable. 32-bit */
#define GPIO_INPUT_REG    0x00000020L	/* GPIOInput Register , 32-bit */ 

/*------- LED Register --------------*/
#define  FPGA_LED_REG     0x00000024L  /* bits 7-0 of 32-bits */

/*------- GeoAddress  Register --------------*/
#define  FPGA_GEOADDR_REG     0x00000028L  /* Geographic Address bits 4-0 of 32-bits */
#define  GEO_ADDR_MASK		0xF


/*------- Type Address  Register --------------*/
#define  FPGA_TYPEADDR_REG     0x0000002CL  /* Specifies the Type or Persona of controller bits 2-0 of 32-bits */
#define  TYPE_ADDR_MASK		0x7

/*------- Interrupt Status Register --------------*/
#define  FPGA_INTRPSTATUS_REG     0x00000030L

       /* Interrupt Status Bits */
#define  FIFO_OVERFLOW_STATUS	0x1	/* FIFO overflow intterupt condition */
#define  FIFO_UNDERFLOW_STATUS	0x2	/* FIFO underflow intterupt condition (FOO) */
#define  FIFO_FINISHED_STATUS	0x4     /* FIFO executed halt instruction */
#define  FIFO_SW_INT0_STATUS	0x8    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT1_STATUS	0x10    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT2_STATUS	0x20    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT3_STATUS	0x40    /* Software Interrupt  bits 3-6 */
#define  FAIL_INT_STATUS        0x80    /* System Failure */
#define  WARN_INT_STATUS        0x100   /* System Warning */

/*------- Interrupt Enable Register --------------*/
#define  FPGA_INTRPENABLE_REG     0x00000034L

       /* Interrupt Enable Bits */
#define  FIFO_OVERFLOW_ENABLE	0x1	/* FIFO overflow intterupt condition */
#define  FIFO_UNDERFLOW_ENABLE	0x2	/* FIFO underflow intterupt condition (FOO) */
#define  FIFO_FINISHED_ENABLE	0x4     /* FIFO executed halt instruction */
#define  FIFO_SW_INT0_ENABLE	0x8    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT1_ENABLE	0x10    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT2_ENABLE	0x20    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT3_ENABLE	0x40    /* Software Interrupt  bits 3-6 */
#define  FAIL_INT_ENABLE        0x80    /* System Failure */
#define  WARN_INT_ENABLE        0x100   /* System Warning */

/*------- Interrupt Clear Register --------------*/
#define  FPGA_INTRPCLEAR_REG     0x00000038L

       /* Interrupt Clear Bits */
#define  FIFO_OVERFLOW_CLEAR	0x1	/* FIFO overflow intterupt condition */
#define  FIFO_UNDERFLOW_CLEAR	0x2	/* FIFO underflow intterupt condition (FOO) */
#define  FIFO_FINISHED_CLEAR	0x4     /* FIFO executed halt instruction */
#define  FIFO_SW_INT0_CLEAR	0x8    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT1_CLEAR	0x10    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT2_CLEAR	0x20    /* Software Interrupt  bits 3-6 */
#define  FIFO_SW_INT3_CLEAR	0x40    /* Software Interrupt  bits 3-6 */
#define  FAIL_INT_CLEAR        0x80    /* System Failure */
#define  WARN_INT_CLEAR        0x100   /* System Warning */


/* --------- Miscellaneous Registers ------- */
/* ------- Ophaned ?????? ---- */
/* #define MIXER_SELECT      0x00000010L    Mixer Select Register, 1-bit */
#define AMT_READ_REG         0x0000003CL	/* AMT Amplifier Read, 3-0 bit */
#define AMT_READ_MASK     0xF	        /* AMT Amplifier Read mask , 4-bits */

/* --------- Software Controlled FIFO Outputs ----------------- */
#define SW_PHASE_REG	0x00000040L	/* Software controlled value for fifo_phase output., 16-bits */
#define SW_AMP_REG	0x00000044L	/* Software controlled value for fifo_amp output., 14-bits */
#define SW_GATES_REG	0x00000048L	/* Software controlled value for fifo_gates output., 8 bits */
#define SW_USER_REG	0x0000004CL	/* Software controlled value for fifo_user output., 3-bits */
#define SW_AUX_REG 	0x00000050L 	/* Software controlled value for fifo_aux output., 12-bits */
#define SW_AU_RESETX_REG    0x00000054L     /* Software controlled value for fifo_aux_reset, 3-bits */
#define SW_AUX_STROBE_REG   0x00000058L     /* Software controlled value for fifo_aux_strobe, 1-bit */
#define FIFO_OUTPUT_SELECT_REG  0x0000005CL  /* Select software or FIFO controlled FIFO outputs, 1-bit. */

/* ------------- FIFO Readback Register --------------------- */
#define FIFO_PHASE_REG 	0x00000060L	/* Fifo controlled value for fifo_phase output., 16-bits */
#define FIFO_AMP_REG 	0x00000064L	/* Fifo controlled value for fifo_amp output., 14-bits */
#define FIFO_GATES_REG 	0x00000068L	/* Fifo controlled value for fifo_gates output., 8-bits */
#define FIFO_USER_REG 	0x0000006CL	/* Fifo controlled value for fifo_user output., 8-bits */
#define FIFO_AUX_REG 	0x00000070L	/* Fifo controlled value for fifo_aux output., 8-bits */

#define FIFO_GPIO_REG   0x00000074L	/* FIFO Controlled genreal purpose outputs, 31-0, 32-bits */
#define CLR_CUM_DUR_REG 0x00000078L     /* Clear Cumulative duration, rising edge resets , bit-0*/
#define CUM_DUR_LOW_REG 0x0000007CL
#define CUM_DUR_HI_REG  0x00000080L

#define INSTR_FIFO_CT   0x00000084L   /* instruction Fifo count */
#define DATA_FIFO_CT    0x00000088L   /* instruction Fifo count */

/* Holding Registers These registers allow software to read the value of the FIFO holding registers. */

#define HOLDING_DURATION  0x0000008CL	/* The value of the Duration holding register, 26-bits */
#define HOLDING_GATES 	  0x00000090L	/* The value of the Gates holding register, 8-bits */
#define HOLDING_EXT_TIME  0x00000094L	/* The value of the ExtTime holding register, 16-bits */

#define HOLDING_PHASE      0x00000098L	/* The value of the Phase holding register, 16-bits */
#define HOLDING_PHASE_INCR 0x0000009CL   /* The value of the Phase increment holding register, 16-bits */
#define HOLDING_PHASE_CNT  0x000000A0L   /* The value of the Phase count holding register, 9-bits */
#define HOLDING_PHASE_CLR  0x000000A4L   /* The value of the Phase clear holding register, 1-bits */

#define HOLDING_PHASE_C      0x000000A8L	/* The value of the Phase Constant holding register, 16-bits */
#define HOLDING_PHASE_C_INCR 0x000000ACL  /* The value of the Phase Constant increment holding register, 16-bits */
#define HOLDING_PHASE_C_CNT  0x000000B0L   /* The value of the Phase Constant count holding register, 9-bits */
#define HOLDING_PHASE_C_CLR  0x000000B4L   /* The value of the Phase COnstant clear holding register, 1-bits */

#define HOLDING_AMP       0x000000B8L	/* The value of the Amplitude holding register, 16-bits */
#define HOLDING_AMP_INCR  0x000000BCL   /* The value of the Amplitude increment holding register, 16-bits */
#define HOLDING_AMP_CNT   0x000000C0L   /* The value of the Amplitude count holding register, 9-bits */
#define HOLDING_AMP_CLR   0x000000C4L   /* The value of the Amplitude clear holding register, 1-bits */

#define HOLDING_AMP_SCALE       0x000000C8L  /* The value of the Amplitude Scaling holding register, 16-bits */
#define HOLDING_AMP_SCALE_INCR  0x000000CCL  /* The value of the Amp Scaling increment holding register, 16-bits */
#define HOLDING_AMP_SCALE_CNT   0x000000D0L  /* The value of the Amp Scaling count holding register, 9-bits */
#define HOLDING_AMP_SCALE_CLR   0x000000D4L  /* The value of the Amp Scaling clear holding register, 1-bits */

#define HOLDING_USER      0x000000D8L	/* The value of the User holding register, 3-bits */


/* FIFO Contents These registers allow reading of the FIFO contents. */
/* #define FIFO_WRITE_PTR     0x0000006CL	 FIFO write pointer,, 10-bits */
/* #define FIFO_READ_PTR      0x00000070L	 FIFO read pointer,, 10-bits */
#define FIFO_INSTRUCTION   0x000000DCL	/* FIFO instruction port, 32-bits */

/* RF_FIFO Register (Address 256 - 1280) Bits Field Description Default  */
/* 68 aux_select Duration value is AUX N/A */
/* 67:42 duration Timer duration or AUX value.  (note spans two words) */
/* 40:33 gates RF Gates */
/* 32:17 phase Phase  */
/* 16:3 amp Amplitude  */
/* 2:0 user User controls */
/* */
#define FIFO_OUTPUT_LSW  0x00000400L	/* LSW of FIFO Output, 32:17 phase, 16:3 Amplitude, 2:0 User controls */
#define FIFO_OUTPUT_MSW  0x00000404L    /*  40:33 RF Gates, 41 External Timebase select, 64:42 Time Duration  */
#define FIFO_OUTPUT_HSW  0x00000408L    /*  67:65 Time Duration, 68 aux_select Duration value is AUX data */
/**********************************************************8
* hardware macros
*/
#if 0
#define FPGA_PGRM_CNTRL ( (volatile unsigned long* const) (FPGA_PRGM_BASE_ADR + FPGA_PRGM_CR))
#define FPGA_PGRM_DATA ( (volatile unsigned long* const) (FPGA_PRGM_BASE_ADR + FPGA_PRGM_DR))
#define FPGA_PGRM_STAT ( (volatile unsigned long* const) (FPGA_PRGM_BASE_ADR + FPGA_PRGM_SR))
#endif

#define FF_CNTRL     ( (volatile unsigned long* const) (FPGA_BASE_ADR + FPGA_FIFO_CR))
#define FF_STATR     ( (volatile unsigned long* const) (FPGA_BASE_ADR + FPGA_FIFO_SR))
#define GPIO_OUT     ( (volatile unsigned long* const) (FPGA_BASE_ADR + GPIO_OUTPUT_REG))
#define GPIO_READ    ( (volatile unsigned long* const) (FPGA_BASE_ADR + GPIO_INPUT_REG))
#define GPIO_DIR_CNTRL   ( (volatile unsigned long* const) (FPGA_BASE_ADR + GPIO_ENABLE_REG))


#define GPIO_CR ( (volatile unsigned long* const) CPC0_CR0 )




/* some basic casting typed predefined */
#define VOL_CHAR_PTR    (volatile unsigned char* const)
#define VOL_SHORT_PTR   (volatile unsigned short* const)
#define VOL_INT_PTR     (volatile unsigned int* const)
#define VOL_LONG_PTR    (volatile unsigned long* const)
 
#ifdef __cplusplus
}
#endif
 
#endif
