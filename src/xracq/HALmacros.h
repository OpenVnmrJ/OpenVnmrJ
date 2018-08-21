/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* miscellaneous */
#define ERROR	-1
#define OK	0
#define TRUE	1
#define FALSE	0
#define NULL	0L


/* macro definitions */

/* enable/disable mfp and dma interrupts */
#define enable_interrupts	asm("andi.w	#$f0ff,sr")
#define disable_interrupts	asm("ori.w	#$0600,sr")

/* get/set contents of mfp registers */
#define get_mfp(A)		*((char *)A + MFP_BASE)
#define set_mfp(A,B)		*((char *)A + MFP_BASE) = B

/* get/set contents of dma registers */
#define get_dma(A)		*((char *)A + DMA_BASE)
#define set_dma(A,B)		*((char *)A + DMA_BASE) = B

/* set/clear selected bit in control register */
#define set_cxr(A,B)		*((char *)A + CXR_BASE) = B

/* convert long bytes to minimum long bytes that is a multiple of 512 */
#define r512(A)		(((A) % 512L) == 0) ? (A) : (A) + 512L - (A) % 512L

/* convert long bytes to integer minimum number of blocks of size 512L */
#define blks(A)			(int)(((A) + 511L)/512L)

/* convert long bytes to long minimum number of blocks of size 512L */
#define lblks(A)		(((A) + 511L)/512L)

/* convert long bytes to integer minimum number of blocks of size A_BLK_SIZE */
#define vblks(A)		(int)(((A) + A_BLK_SIZE - 1L)/A_BLK_SIZE)

/* convert long bytes to long minimum number of blocks of size A_BLK_SIZE */
#define lvblks(A)		(((A) + A_BLK_SIZE - 1L)/A_BLK_SIZE)

/* convert long bytes to integer minimum number of blocks of size D_BLK_SIZE */
#define dblks(A)		(int)(((A) + D_BLK_SIZE - 1L)/D_BLK_SIZE)

/* convert long bytes to long minimum number of blocks of size D_BLK_SIZE */
#define ldblks(A)		(((A) + D_BLK_SIZE - 1L)/D_BLK_SIZE)

/* convert long bytes to integer minimum number of blocks of size S_BLK_SIZE */
#define sblks(A)		(int)(((A) + S_BLK_SIZE - 1L)/S_BLK_SIZE)

/* convert long bytes to long minimum number of blocks of size S_BLK_SIZE */
#define lsblks(A)		(((A) + S_BLK_SIZE - 1L)/S_BLK_SIZE)

/* convert a DRAM HALbus address to a DRAM VERSAbus address */
#define vbus(A)	(((dram_diff) && (A) >= (WDRAM+dram_diff) && \
(A) <= (d_ram.end+dram_diff)) ? (A) - (dram_diff) : (A))

/* convert a DRAM VERSAbus address to a DRAM HALbus address */
#define hbus(A)	(((dram_diff) && (A) >= WDRAM && \
(A) <= d_ram.end) ? (A) + (dram_diff) : (A))

/* convert a vm02 RAM VERSAbus address to a vm02 RAM ACQbus address */
#define abus(A)	((A) >= WARAM && (A) <= WATOP) ? (A) - WARAM : (A)

/* convert a vm02 RAM ACQbus address to a vm02 RAM VERSAbus address */
#define Vbus(A)	((A) >= AARAM && (A) <= AATOP) ? (A) + WARAM : (A)

/* clear bit pattern in an integer lsb */
/* used on sblock[2] */
#define clr_lo(A,B)	(((A) & ~(B) & 0x00ff) == 0) ? (A) & 0xff00 : (A) & ~(B) | 0x0080

/* clear bit pattern in an integer msb */
/* used on sblock[2] */
#define clr_hi(A,B)	(((A) & ~(B) & 0xff00) == 0) ? (A) & 0x00ff : (A) & ~(B) | 0x8000
