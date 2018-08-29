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
#define IN_BRUKER		(1<<0)		/* DISNMR */
#define IN_M100			(1<<1)		/* Chemagnetics M series */
#define IN_CMX			(1<<2)		/* Chemagnetics CMX (Intel/spec) */
#define IN_CMXW			(1<<3)		/* Chemagnetics CMX (Sun/cmxw) */
#define IN_CMSS			(1<<4)		/* Chemagnetics CMX (Sun/spinsight) */
#define IN_NMR1			(1<<5)		/* NMR1, transferred from Vax */
#define IN_VARIAN		(1<<6)		/* Varian VXR-S/Unity */
#define IN_SREAD		(1<<7)		/* Varian "sread" data transfer format */
#define IN_SMIS			(1<<8)		/* SMIS native .MRD format */
#define MAKE_FILE		(1<<10)		/* make just a single file */
#define MAKE_DIR		(1<<11)		/* make a directory full of files */
#define OUT_VFID		(1<<12)		/* write a Varian-format "fid" file */
#define OUT_VPARAMS		(1<<13)		/* write a Varian-format "procpar" file */
#define OUT_VTEXT		(1<<14)		/* write a Varian-format "text" file */
#define OUT_SDATA		(1<<15)		/* write an sread-format "sdata" file */
#define OUT_STEXT		(1<<16)		/* write an sread-format "stext" file */
#define READ_FILE		(1<<17)		/* input from a single file */
#define READ_DIR		(1<<18)		/* input from a directory */
#define NO_HEADER		OUT_SDATA	/* really the same thing */
#define EXPERIMENTAL_METHOD	((unsigned int)1<<31) /* controlled by exptl_type */
#define BLIND_READ		(1<<30)		/* read fid but ignore params */
#define SCALE_REALS		(1<<29)		/* multiply reals by rfactor */
#define SCALE_IMAGS		(1<<28)		/* multiply imags by ifactor */
#define ZERO_REALS		(1<<27)		/* replace reals with zeroes */
#define ZERO_IMAGS		(1<<26)		/* replace imags with zeroes */
#define SPECTRUM_REVERSE	(1<<25)		/* negate imaginary data */
#define BYTE_REVERSE		(1<<24)		/* reverse byte order within word */
#define SHUFFLE			(1<<23)		/* shuffle reals and imags */
#define VAX_FLOAT		(1<<22)		/* convert Vax D_floating to integer */
#define IEEE_FLOAT		(1<<21)		/* convert IEEE 32-bit float to integer */
#define ALTERNATE_SIGNS		(1<<20)		/* negate every 2nd complex point */

#define ALL_IN		(IN_BRUKER|IN_M100|IN_CMX|IN_CMXW|IN_CMSS|IN_NMR1|IN_VARIAN|IN_SREAD|IN_SMIS)
#define ALL_OUT		(OUT_VFID|OUT_VPARAMS|OUT_VTEXT|OUT_SDATA|OUT_STEXT)
#define ALL_MAKE	(MAKE_FILE|MAKE_DIR)
#define ALL_READ	(READ_FILE|READ_DIR)
#define ALL_TRANS	(SCALE_REALS|SCALE_IMAGS|ZERO_REALS|ZERO_IMAGS|SPECTRUM_REVERSE|BYTE_REVERSE|SHUFFLE|VAX_FLOAT|IEEE_FLOAT|ALTERNATE_SIGNS|EXPERIMENTAL_METHOD)

#define RC_SUCCESS		0
#define RC_NOINFILE		1
#define RC_NOOUTFILE		2
#define RC_NOMEMORY		3
#define RC_BADSWITCH		4
#define RC_BADARG		5
#define RC_MISSINGARG		6
#define RC_BADNUMBER		7
#define RC_NOFORMAT		8
#define RC_INOTYET		10
#define RC_ONOTYET		11
#define RC_EOF			12
#define RC_READERR		13
#define RC_WRITERR		14
#define RC_SKIPERR		15
#define RC_NOMATH		16
#define RC_NOOUTDIR		17
#define RC_BADBFLOAT		18
#define RC_NOTMPFILE		19
#define RC_BADPARAMLINE		20
#define RC_BADARRAY		21
#define RC_BADDATE		22
#define RC_INTERNAL		100		/* programming error, "never happens" */

#ifdef MSDOS
#	define SLASH '\\'
#else
#	define SLASH '/'
#endif

#ifndef unix		/* for Amiga or MS-DOS, don't know about Mac etc. */
#	define DELETE remove
#	define SIGNEDCHAR signed char
#else
#	define DELETE unlink
#	define SIGNEDCHAR char
#endif

#define TRUE	1
#define FALSE	0
