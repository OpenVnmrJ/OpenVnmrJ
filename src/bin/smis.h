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
/* This is the format of the first 512 bytes of .MRD files as of software
   version x.xx (late 1992).  It is NOT the same as what is described in the
   Spectroscopy Data Processing Manual v1.0 p. 20.  nsarray is found at
   offset 0x9c, not 0x80.  Some of the "unused" locations tend to have
   mysterious data in them, e.g. (the following in their original byte
   order):  00fe ff46 0000 00c7 starting at offset 0x30 in most fids, though
   0000 8045 0000 0000 was found there in one arrayed 1-D; 01 at offset 0x98
   in most fids; but a 2-D fid had all zeroes starting at offset 0x13.
   
   Single and arrayed 1-D fids have the number of complex points per trace
   (half the Varian "np") in ncp1; 1 in ncp2, ncp3, and nslice; and 1 or the
   arraydim in nsarray (never 0).  The one 2-D fid I have seen (probably an
   absolute value COSY) had th number of traces (not half that) in ncp2; i.e.
   the number of floating point numbers in the file was 2 x ncp1 x ncp2.  It
   had 1 for ncp3 and nslice, and 0 in nsarray.

   The structure paradigm and element names are my invention for more
   rational programming; the manual uses no names and only byte offsets.
   Beware that "lint" tells us this is non-portable to processors with wierd
   alignment rules.  For now this structure survives without padding the
   alignment rules of Motorola 680x0, Intel 80x86, and SPARC processors.
*/

struct SMIS_header {
	long ncp1;		/* number of complex points in dim 1 */
	long ncp2		/* number of complex points in dim 2 */;
	long ncp3;		/* number of complex points in dim 3 */
	long nslice;		/* number of slices in multislice image data */
	char unused1[2];
	short datatypecode;	/* element size */
	char unused2[136];
	long nsarray;		/* number of spectra in arrayed experiments */
	char unused3[96];
	char text[256];		/* sample description or dummy text */
};

#define SMIS_DTC_UCHAR		0x00	/* unsigned, 8 bits */
#define SMIS_DTC_SCHAR		0x01	/* signed, 8 bits */
#define SMIS_DTC_SHORT		0x02	/* signed, 16 bits */
#define SMIS_DTC_INT		0x03	/* IBM PC, same as short */
#define SMIS_DTC_LONG		0x04	/* signed, 32 bits */
#define SMIS_DTC_FLOAT		0x05	/* IEEE 32-bit single precision */
#define SMIS_DTC_DOUBLE		0x06	/* IEEE 64-bit double precision */
#define SMIS_DTC_SIZE_MASK	0x0f	/* includes all of the above */
#define SMIS_DTC_COMPLEX	0x10	/* shuffled (r,i) pairs */

#define SMIS_FOOTER_SMPLEN	120	/* length of sample file name */

struct smispar {
	char * name;
	union {
		double d;	/* type 'd', one double */
		char * s;	/* type 's', one string */
		long l;		/* type 'l', one long */
	} value;
	char type;
};

#define SMISPAR_DOUBLE		'd'
#define SMISPAR_STRING		's'
#define SMISPAR_LONG		'l'
#define SMISPAR_ARRAYELTS	'a'

#define BUFFERSIZE	256		/* must be > SMIS_FOOTER_SMPLEN */
#define TOKENVSIZE	16		/* max words on a parameter line */
#define SEPARATORS	", \t\n\r"	/* these can separate words on line */
#define NUMSMISPAR	8		/* max smispars from one line (min 5) */
			/* more added automatically for arrayed variables */

#define SMIS_PARNAME	"smis"		/* system name for stext */

/* temporary file to hold stext as it is being built */
#define SMIS_TEMPFILE	"/tmp/smis_stext.tmp"
