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
/* An M-100 float.  Convert to double using mfloat_to_double(). */
typedef unsigned long mfloat;

/* This is the format of the first 2481 bytes of the M-100 file, where the
   prolog containing the filename and size has been included in the file.
   Also included in this structure is 1 stray byte, undocumented in the
   sources I have access to, which occurs between the header and the fid.
   Since we are declaring this as a structure, we must make sure that the
   alignment rules of the processor do not cause the compiler to insert pad
   bytes into the structure.  For Sparc and most 32-bit processors, this means
   we have to keep the ints and longs on 4-byte boundaries.  In the offsets
   below, "na" indicates offsets not aligned on 4-byte boundaries.

   Note that the "dp" element is a char (it's true; the M-100 software
   restricts it to 0-255).  Even though the previous 3 bytes seem to be
   unused, we must not treat it as a long, since the result would not be
   4-byte aligned.
*/
struct m100_header {
	char filename[12];			/*    0 */
	long filesize;				/*   12 */
	long dl;				/*   16 */
	long dl_bytes;				/*   20 */
			char unused0[4];	/*   24 */
	long al;				/*   28 */
	long al_bytes;				/*   32 */
			char unused1[140];	/*   36 */
	char seqfil[12];			/*  176 */
			char unused2[4];	/*  188 */
	char pscomment[64];			/*  192 */
			char unused3[1192];	/*  256 */
	mfloat pw1;				/* 1448 */
	mfloat pw2;				/* 1452 */
			char unused4[12];	/* 1456 */
	mfloat pd;				/* 1468 */
			char unused5[4];	/* 1472 */
	mfloat pred;				/* 1476 */
	mfloat ct;				/* 1480 */
	mfloat psd;				/* 1484 */
	mfloat act;				/* 1488 */
	mfloat tau;				/* 1492 */
			char unused6[16];	/* 1496 */
	mfloat tm[20];				/* 1512 */
			char unused7[120];	/* 1592 */
	long na;				/* 1712 */
			char unused8[16];	/* 1716 */
	long ac;				/* 1732 */
			char unused9[13];	/* 1736 */
	char dp;				/* 1749 */
			char unused10[8];	/* 1750 */
	char comment[178];			/* 1758na */
	long dw;				/* 1936 */
			char unused11[4];	/* 1940 */
	long fobs;				/* 1944 */
			char unused12[4];	/* 1948 */
	long fdec;				/* 1952 */
	mfloat lb;				/* 1956 */
			char unused13[40];	/* 1960 */
	mfloat rm;				/* 2000 */
			char unused14[476];	/* 2004 */
	char			stray;		/* 2480 */
};

/* The number of bytes to read from the file to fill an m100_header.  This is
   not the same as sizeof(struct m100_header) because the structure as written
   is 2481 bytes long, but GCC/SPARC alignment rules (and probably rules of
   other 32-bit compiler/CPU combinations) pad the actual structure to 2844
   bytes. */
#define M100_HEADER_SIZE	2481

#define M100_PARNAME	"m100"		/* system name for stext */

