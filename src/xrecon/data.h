/* data.h */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* data.h: Stripped down data file handler                           */
/*                                                                           */
/* This file is part of Xrecon.                                              */
/*                                                                           */
/* Xrecon is free software: you can redistribute it and/or modify            */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* Xrecon is distributed in the hope that it will be useful,                 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with Xrecon. If not, see <http://www.gnu.org/licenses/>.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/


/*****************/
struct datafilehead
/*****************/
/* Used at the beginning of each data file (fid's, spectra, 2D) */
{
   int     nblocks;      /* number of blocks in file			*/
   int     ntraces;      /* number of traces per block			*/
   int     np;           /* number of elements per trace		*/
   int     ebytes;       /* number of bytes per element			*/
   int     tbytes;       /* number of bytes per trace			*/
   int     bbytes;       /* number of bytes per block			*/
   short   vers_id;      /* software version and file_id status bits	*/
   short   status;       /* status of whole file			*/
   int	   nbheaders;	 /* number of block headers			*/
};


/*******************/
struct datablockhead
/*******************/
/* Each file block contains the following header */
{
   short   scale;	/* scaling factor                   */
   short   status;	/* status of data in block          */
   short   index;	/* block index                      */
   short   mode;	/* mode of data in block	    */
   int	   ctcount;	/* ct value for FID		    */
   float   lpval;	/* F2 left phase in phasefile       */
   float   rpval;	/* F2 right phase in phasefile      */
   float   lvl;		/* F2 level drift correction        */
   float   tlt;		/* F2 tilt drift correction         */
};


/*********************/
struct hypercmplxbhead
/*********************/
/* Additional datafile block header for hypercomplex 2D data */
{
   short   s_spare1;	/* short word:  spare		*/
   short   status;	/* status word for block header	*/
   short   s_spare2;	/* short word:  spare		*/
   short   s_spare3;	/* short word:  spare		*/
   int	   l_spare1;	/* int word:  spare		*/
   float   lpval1;	/* additional left phase	*/
   float   rpval1;	/* additional right phase 	*/
   float   f_spare1;	/* float word:  spare		*/
   float   f_spare2;	/* float word:  spare		*/
};


/*-----------------------------------------------
|						|
|     Data file header status bits  (0-9) 	|
|						|
+----------------------------------------------*/

/* Bits 0-6:  file header and block header status bits (bit 6 = unused) */
#define S_DATA		0x1	/* 0 = no data      1 = data       */
#define S_SPEC		0x2	/* 0 = fid	    1 = spectrum   */
#define S_32		0x4	/* 0 = 16 bit	    1 = 32 bit	   */
#define S_FLOAT 	0x8	/* 0 = integer      1 = fltng pt   */
#define S_COMPLEX	0x10	/* 0 = real	    1 = complex	   */
#define S_HYPERCOMPLEX	0x20	/* 1 = hypercomplex		   */

/* Bits 7-10:  file header status bits (bit 10 = unused) */
#define S_DDR		0x80    /* 0 = not DDR acq  1 = DDR acq    */
#define S_SECND		0x100	/* 0 = first ft     1 = second ft  */
#define S_TRANSF	0x200	/* 0 = regular      1 = transposed */
#define S_3D		0x400	/* 1 = 3D data			   */

/* Bits 11-14:  status bits for processed dimensions (file header) */
#define S_NP		0x800	/* 1 = np  dimension is active	*/
#define S_NF		0x1000	/* 1 = nf  dimension is active	*/
#define S_NI		0x2000	/* 1 = ni  dimension is active	*/
#define S_NI2		0x4000	/* 1 = ni2 dimension is active	*/


/*-------------------------------------------------------
|							|
|     Main data block header status bits  (7-15)	|
|							|
+------------------------------------------------------*/

/* Bit 7 */
#define MORE_BLOCKS	0x80	/* 0 = absent	    1 = present	*/

/* Bits 8-11:  bits 12-14 are currently unused */
#define NP_CMPLX	0x100	/* 1 = complex     0 = real	*/
#define NF_CMPLX	0x200	/* 1 = complex     0 = real	*/
#define NI_CMPLX	0x400	/* 1 = complex     0 = real	*/
#define NI2_CMPLX	0x800   /* 1 = complex     0 = real	*/


/*-----------------------------------------------
|						|
|  Main data block header mode bits  (0-13)	|
|						|
+----------------------------------------------*/

/* Bits 0-3:  bit 3 is used for pamode */
#define NP_PHMODE	0x1	/* 1 = ph mode  */
#define NP_AVMODE	0x2	/* 1 = av mode	*/
#define NP_PWRMODE	0x4	/* 1 = pwr mode */
#define NP_PAMODE	0x8	/* 1 = pa mode  */

/* Bits 4-7:  bit 7 is used for pamode */
#define NF_PHMODE	0x10	/* 1 = ph mode	*/
#define NF_AVMODE	0x20	/* 1 = av mode	*/
#define NF_PWRMODE	0x40	/* 1 = pwr mode	*/
#define NF_PAMODE	0x80	/* 1 = pa mode  */

/* Bits 8-10 */
#define NI_PHMODE	0x100	/* 1 = ph mode	*/
#define NI_AVMODE	0x200	/* 1 = av mode	*/
#define NI_PWRMODE	0x400	/* 1 = pwr mode	*/

/* Bits 11-13:  bits 14 and 15 are used for pamode */
#define NI2_PHMODE	0x800	/* 1 = ph mode	*/
#define NI2_AVMODE	0x1000	/* 1 = av mode	*/
#define NI2_PWRMODE	0x2000	/* 1 = pwr mode	*/

/* Bits 14-15 */
#define NI_PAMODE	0x4000	/* 1 = pa mode  */
#define NI2_PAMODE	0x8000	/* 1 = pa mode  */

/*-------------------------------
|				|
|    Software Version Status 	|
|				|
+------------------------------*/

/* Bits 0-5:  31 different software versions; 32-63 are for 3D */

/*-------------------------------
|				|
|	 File ID Status		|
|				|
+------------------------------*/

/* Bits 6-10:  31 different file types (64-1984 in steps of 64) */
#define FID_FILE	0x40
#define DATA_FILE	0x80
#define PHAS_FILE	0xc0
