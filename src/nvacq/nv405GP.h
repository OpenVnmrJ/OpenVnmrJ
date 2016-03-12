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


/* from ppc405GP.h */

/*
 * The 405GPr chip uses the 405D4 processor core which has a 16KB I-cache and a
 * 16KB D-cache.  All 405 processor cores have 32 byte cache lines, and are
 * 2-way set associative.
 */

#define _ICACHE_LINE_NUM_405GPR 256                    /* 256 * 32 * 2 = 16KB */
#define _DCACHE_LINE_NUM_405GPR 256                    /* 256 * 32 * 2 = 16KB */

/*
 * Processor Version Register (PVR) values for 405GP and 405GPr
 */
 
/*#define PVR_405GP_RA  0x40110000    405GP Revision A (not supported) */
#define PVR_405GP_RB    0x40110040        /* 405GP Revision B                 */ 
#define PVR_405GP_RC    0x40110082        /* 405GP Revision C                 */ 
#define PVR_405GP_RD    0x401100C4        /* 405GP Revision D                 */ 
#define PVR_405GP_RE    0x40110145        /* 405GP Revision E                 */ 
#define PVR_405GPR_RB   0x50910951        /* 405GPr Revision B (1.1)          */ 
 
#define PVR_405GP_HI    0x4011          /* generic 405GP recognition */
 
/*
 * Base DCR address values for all perhipheral cores in the 405GP
 */
 
#define SDRAM_DCR_BASE        0x010       /* SDRAM Controller                 */ 
#define EBC0_DCR_BASE         0x012       /* External Bus Controller          */ 
#define UIC_DCR_BASE          0x0C0       /* Universal Interrupt Controller   */
#define DMA_DCR_BASE          0x100       /* DMA Controller                   */ 
#define CLKPWRCH_DCR_BASE     0x0B0       /* Clock/Power/Chip Control         */ 
#define MAL0_DCR_BASE         0x180       /* Memory Access Layer Core         */ 
#define DECOMP_DCR_BASE       0x014       /* Code Decompression Core          */ 
#define OCM_DCR_BASE          0x018       /* On-chip Memory Controller        */ 
 
/*
 * Static interrupt vectors/levels.  These also correspond to bit numbers in
 * many of the registers of the Universal Interrupt Controller.
 */
#define INT_VEC_UART0             0
#define INT_VEC_UART1             1
#define INT_VEC_IIC               2
#define INT_VEC_EXT_MASTER        3
#define INT_VEC_PCI               4
#define INT_VEC_DMA_CH0           5
#define INT_VEC_DMA_CH1           6
#define INT_VEC_DMA_CH2           7
#define INT_VEC_DMA_CH3           8
#define INT_VEC_ENET_WAKEUP       9
#define INT_VEC_MAL_SERR          10
#define INT_VEC_MAL_TXEOB         11
#define INT_VEC_MAL_RXEOB         12
#define INT_VEC_MAL_TXDE          13
#define INT_VEC_MAL_RXDE          14
#define INT_VEC_ENET_0            15
#define INT_VEC_PCI_SERR          16
#define INT_VEC_ECC_ERROR         17
#define INT_VEC_PCI_POWER         18
#define INT_VEC_EXT_IRQ_7         19                /* 405GPr only */
#define INT_VEC_EXT_IRQ_8         20                /* 405GPr only */
#define INT_VEC_EXT_IRQ_9         21                /* 405GPr only */
#define INT_VEC_EXT_IRQ_10        22                /* 405GPr only */
#define INT_VEC_EXT_IRQ_11        23                /* 405GPr only */
#define INT_VEC_EXT_IRQ_12        24                /* 405GPr only */
#define INT_VEC_EXT_IRQ_0         25
#define INT_VEC_EXT_IRQ_1         26
#define INT_VEC_EXT_IRQ_2         27
#define INT_VEC_EXT_IRQ_3         28
#define INT_VEC_EXT_IRQ_4         29
#define INT_VEC_EXT_IRQ_5         30
#define INT_VEC_EXT_IRQ_6         31
 
#define INT_LVL_UART0             0
#define INT_LVL_UART1             1
#define INT_LVL_IIC               2
#define INT_LVL_EXT_MASTER        3
#define INT_LVL_PCI               4
#define INT_LVL_DMA_CH0           5
#define INT_LVL_DMA_CH1           6
#define INT_LVL_DMA_CH2           7
#define INT_LVL_DMA_CH3           8
#define INT_LVL_ENET_WAKEUP       9
#define INT_LVL_MAL_SERR          10
#define INT_LVL_MAL_TXEOB         11
#define INT_LVL_MAL_RXEOB         12
#define INT_LVL_MAL_TXDE          13
#define INT_LVL_MAL_RXDE          14
#define INT_LVL_ENET_0            15
#define INT_LVL_PCI_SERR          16
#define INT_LVL_ECC_ERROR         17
#define INT_LVL_PCI_POWER         18
#define INT_LVL_EXT_IRQ_7         19                /* 405GPr only */
#define INT_LVL_EXT_IRQ_8         20                /* 405GPr only */
#define INT_LVL_EXT_IRQ_9         21                /* 405GPr only */
#define INT_LVL_EXT_IRQ_10        22                /* 405GPr only */
#define INT_LVL_EXT_IRQ_11        23                /* 405GPr only */
#define INT_LVL_EXT_IRQ_12        24                /* 405GPr only */
#define INT_LVL_EXT_IRQ_0         25
#define INT_LVL_EXT_IRQ_1         26
#define INT_LVL_EXT_IRQ_2         27
#define INT_LVL_EXT_IRQ_3         28
#define INT_LVL_EXT_IRQ_4         29
#define INT_LVL_EXT_IRQ_5         30
#define INT_LVL_EXT_IRQ_6         31
 
/*
/*
 * Clock, power management, chip control and strapping register defintions.
 * Each is a separate DCR register.
 */
#define CPC0_PLLMR  (CLKPWRCH_DCR_BASE+0x0)  /* PLL Mode Register             */ 
#define CPC0_CR0    (CLKPWRCH_DCR_BASE+0x1)  /* Chip control 0 Register       */ 
#define CPC0_CR1    (CLKPWRCH_DCR_BASE+0x2)  /* Chip control 1 Register       */ 
#define CPC0_PSR    (CLKPWRCH_DCR_BASE+0x4)  /* Pin Strapping Register        */ 
#define CPC0_SR     (CLKPWRCH_DCR_BASE+0x8)  /* clock/power management status */
#define CPC0_ER     (CLKPWRCH_DCR_BASE+0x9)  /* clock/power management enable */
#define CPC0_FR     (CLKPWRCH_DCR_BASE+0xa)  /* clock/power management force  */
#define CPC0_ECID0       0xA8                /* 405GPr only chip id upper     */ 
#define CPC0_ECID1       0xA9                /* 405GPr only chip id lower     */ 
#define CPC0_ECR         0xAA                /* 405GPr only edge conditioner  */
#define CPC0_EIRR   (CLKPWRCH_DCR_BASE+0x6)  /* 405GPr ext interrupt routing  */

/* Chip Control Register 0 bits */
#define CR0_PLL_MODE_REG_EN     0x80000000
#define CR0_TRE                 0x08000000
#define CR0_GPIO_10_EN          0x04000000
#define CR0_GPIO_11_EN          0x02000000
#define CR0_GPIO_12_EN          0x01000000
#define CR0_GPIO_13_EN          0x00800000
#define CR0_GPIO_14_EN          0x00400000
#define CR0_GPIO_15_EN          0x00200000
#define CR0_GPIO_16_EN          0x00100000
#define CR0_GPIO_17_EN          0x00080000
#define CR0_GPIO_18_EN          0x00040000
#define CR0_GPIO_19_EN          0x00020000
#define CR0_GPIO_20_EN          0x00010000
#define CR0_GPIO_21_EN          0x00008000
#define CR0_GPIO_22_EN          0x00004000
#define CR0_GPIO_23_EN          0x00002000
#define CR0_GPIO_24_EN          0x00001000
#define CR0_UART1_CTS_EN        0x00000800
#define CR0_UART1_DTR_EN        0x00000400
#define CR0_UART0_EXT_CLK       0x00000080
#define CR0_UART1_EXT_CLK       0x00000040
#define CR0_UART_DIV_MASK       0x0000003E


/*
 * On-chip memory controller configuration register definitions.
 */
 
#define OCM_ISARC  (OCM_DCR_BASE+0x00)    /* Instruction-side Addr range cmp  */
#define OCM_ISCNTL (OCM_DCR_BASE+0x01)    /* Instruction-side control reg     */ 
#define OCM_DSARC  (OCM_DCR_BASE+0x02)    /* Data-side Addr range cmp         */ 
#define OCM_DSCNTL (OCM_DCR_BASE+0x03)    /* Data-side control reg            */ 
 
/*
 * 405GP UART (2 of them) Base Address definitions.  Both UARTs are 16550-like.
 */
#define UART0_BASE      0xEF600300
#define UART1_BASE      0xEF600400
#define UART_REG_ADDR_INTERVAL   1
 
#define UART_MEMORY_START   0xEF600000
#define UART_MEMORY_END     0xEF600FFF
 
 
/*
 * General Purpose I/O (GPIO)
 */
 
#define GPIO_BASE          0xEF600700
 
#define GPIO_OR      (GPIO_BASE+0x00)   /* GPIO Output Register               */ 
#define GPIO_TCR     (GPIO_BASE+0x04)   /* GPIO Three-state Control Reg       */ 
#define GPIO_ODR     (GPIO_BASE+0x18)   /* GPIO Open Drain Reg                */ 
#define GPIO_IR      (GPIO_BASE+0x1c)   /* GPIO Input Register                */ 

/*
 * Flash / Boot ROM area
 */
#define FLASH_START 0xFFF80000
#define FLASH_END   0xFFFFFFFF
 
