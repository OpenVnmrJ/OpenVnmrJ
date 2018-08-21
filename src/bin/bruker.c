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
static char Version_ID[] = "bruker.c v2.71 STM 3/13/98";
/* This module converts data acquired by a Bruker Aspect computer running the
   DISNMR program under the ADAKOS operating system.  The DISNMR version it's
   based on is 880101.1; I do not know how applicable it is to other
   versions, particularly earlier ones.  The data it expects is in the form
   seen after transfer to a Sun by recent (1993) versions of Bruknet; the
   header format may be different for other transfer mechanisms so the
   program may not work for those.

   At present there is NO support here for Bruker X-32 (AMX etc.) data which
   is a vastly different format, directory-based rather than single
   file-based.  I would attempt a conversion of X-32 data if someone would
   send me some; I have no access to any myself.
   STM
*/
/* Revision history:
   2.30 adapt to Varian data.h v6.12 data structure elements, cosmetic (2/93)
   2.34	add blind reads
   2.35	replace Bruker defaults with ones for Bruknet-to-Sun data; better 6bit
   2.36	write STEXT_acqtyp_seqen and proc='rft'
   2.40	"final" revised Bruker translation
   2.71	casting changes to please Sun C compiler
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vdata.h"

#include "convert.h"
#include "protos.h"
#include "globals.h"
#include "bruker.h"
#include "stext.h"				/* stext file definition */

/* local prototypes */
#ifdef __STDC__

long bruker_header (long type);
long bruker_blind (long type);
void bruker_read_FDT (void);
void bruker_stext (void);
double getbits (void * object,
                int size,
				int lowbit,
				int numbits);
double doubleBFLOAT (unsigned char * bfloat);
void Bsixbit (unsigned char * outarray,
              unsigned char * inarray);

#else /* __STDC__ */

long bruker_header ();
long bruker_blind ();
void bruker_read_FDT ();
void bruker_stext ();
double getbits ();
double doubleBFLOAT ();
void Bsixbit ();

#endif /* __STDC__ */

extern long blind_skip, blind_length;

static char filename[16];

long bruker_file (type)
long type;
{
	if (type & BLIND_READ)
		type = bruker_blind (type);
	else
		type = bruker_header (type);

	if (type & OUT_STEXT)
		bruker_stext ();

	return (type);
}

long bruker_header (type)
long type;
{
	in_header = (unsigned char *) malloc (BRUKER_HEADSIZE);
	if (in_header == NULL) {
		errmsg (RC_NOMEMORY, __FILE__, __LINE__, BRUKER_HEADSIZE);
		cleanup (RC_NOMEMORY);
	}

	/* skip directory info at beginning of file */
	bruker_read_FDT ();

	/* read next part as header */
	if (fread (in_header, BRUKER_HEADSIZE, 1, fid_in) != 1 ) {
		errmsg (RC_READERR, __FILE__, __LINE__, BRUKER_HEADSIZE);
		free (in_header);
		in_header = NULL;
		cleanup (RC_READERR);
	}

	/* Aspect uses little-endian 3-byte ints, must byte-reverse header */
	reverse_bytes (in_header, BRUKER_HEADSIZE/3, 3);

	in_fhead.nblocks = 1;
	in_fhead.ntraces = 1;
	in_fhead.ebytes = 3;
	in_fhead.vers_id = 0;
	in_fhead.status = S_DATA | S_COMPLEX;
	in_fhead.nbheaders = 1;

	out_fhead.nblocks = 1;
	out_fhead.ntraces = 1;
	out_fhead.ebytes = 4;
	out_fhead.vers_id = 0;
	out_fhead.status = S_DATA | S_32 | S_COMPLEX;
	out_fhead.nbheaders = 1;

	intBINT (in_fhead.np, &in_header[B_tdsize]);
	in_fhead.tbytes = in_fhead.np * in_fhead.ebytes;
	in_fhead.bbytes = in_fhead.ntraces * in_fhead.tbytes;
	out_fhead.np = in_fhead.np;
	out_fhead.tbytes = out_fhead.np * out_fhead.ebytes;
	out_fhead.bbytes = out_fhead.ntraces * out_fhead.tbytes + sizeof (struct datablockhead);

	in_bhead.status = S_DATA | S_COMPLEX;
	in_bhead.index = 1;
	in_bhead.mode = 0;
	in_bhead.lpval = 0;
	in_bhead.rpval = 0;
	in_bhead.lvl = 0;
	in_bhead.tlt = 0;

	out_bhead.status = S_DATA | S_32 | S_COMPLEX;
	out_bhead.index = 1;
	out_bhead.mode = 0;
	out_bhead.lpval = 0;
	out_bhead.rpval = 0;
	out_bhead.lvl = 0;
	out_bhead.tlt = 0;

	intBINT (in_bhead.scale, &in_header[B_globex]);
	intBINT (in_bhead.ctcount, &in_header[B_swpcom]);
	out_bhead.scale = in_bhead.scale;
	out_bhead.ctcount = in_bhead.ctcount;

	return (type);
}

/* Here we blindly seek past what the user tells us is the header, blind_skip
   bytes, then fudge the in_header and out_header to look like it probably
   should given that the data is blind_length long.  Recommended only for
   debugging.
*/
long bruker_blind (type)
long type;
{
	/* discard blind_skip bytes */
	if (fseek (fid_in, blind_skip, SEEK_SET) != 0) {
		errmsg (RC_SKIPERR, __FILE__, __LINE__, blind_skip);
		cleanup (RC_SKIPERR);
	}

	in_fhead.nblocks = 1;
	in_fhead.ntraces = 1;
	in_fhead.ebytes = 3;
	in_fhead.vers_id = 0;
	in_fhead.status = S_DATA | S_COMPLEX;
	in_fhead.nbheaders = 1;

	out_fhead.nblocks = 1;
	out_fhead.ntraces = 1;
	out_fhead.ebytes = 4;
	out_fhead.vers_id = 0;
	out_fhead.status = S_DATA | S_32 | S_COMPLEX;
	out_fhead.nbheaders = 1;

	in_fhead.np = blind_length / 3;
	in_fhead.tbytes = in_fhead.np * in_fhead.ebytes;
	in_fhead.bbytes = in_fhead.ntraces * in_fhead.tbytes;
	out_fhead.np = in_fhead.np;
	out_fhead.tbytes = out_fhead.np * out_fhead.ebytes;
	out_fhead.bbytes = out_fhead.ntraces * out_fhead.tbytes + sizeof (struct datablockhead);

	in_bhead.scale = 0;
	in_bhead.status = S_DATA | S_COMPLEX;
	in_bhead.index = 1;
	in_bhead.mode = 0;
	in_bhead.ctcount = 1;
	in_bhead.lpval = 0;
	in_bhead.rpval = 0;
	in_bhead.lvl = 0;
	in_bhead.tlt = 0;

	out_bhead.scale = 0;
	out_bhead.status = S_DATA | S_32 | S_COMPLEX;
	out_bhead.index = 1;
	out_bhead.mode = 0;
	out_bhead.ctcount = 1;
	out_bhead.lpval = 0;
	out_bhead.rpval = 0;
	out_bhead.lvl = 0;
	out_bhead.tlt = 0;

	return (type);
}

/* Reads over the ADAKOS directory header (a file description table and
   commentary, the latter unused).  The FDT contains the ADAKOS filename so
   this is stashed in the lgobal variable filename for later use.
*/
void bruker_read_FDT ()
{
	unsigned char dirblock[BRUKER_DIRSIZE];
	char tmpname[16];

	if (fread (dirblock, BRUKER_DIRSIZE, 1, fid_in) != 1 ) {
		errmsg (RC_READERR, __FILE__, __LINE__, BRUKER_DIRSIZE);
		cleanup (RC_READERR);
	}
	reverse_bytes (dirblock, BRUKER_DIRSIZE/3, 3);
	Bsixbit ((unsigned char *) &tmpname[0],
		 (unsigned char *) &dirblock[B_FDT_filename  ]);
	Bsixbit ((unsigned char *) &tmpname[4],
		 (unsigned char *) &dirblock[B_FDT_filename+3]);
	Bsixbit ((unsigned char *) &tmpname[9],
		 (unsigned char *) &dirblock[B_FDT_filename+6]);
	tmpname[8]  = '.';
	tmpname[13] = '\0';
	/* quick, very dirty way to remove internal nulls */
	strcpy (filename, &tmpname[0]);
	if (tmpname[3] == '\0')		/* 1st word was short */
		strcat (filename, &tmpname[4]);
	if (tmpname[7] == '\0')		/* 2nd word was short */
		strcat (filename, &tmpname[8]);
}

void bruker_stext ()
{
	struct stext_essparms esspar;
	double tempr, tof, sp, lp;
	long tempi;
	char seqfil[14], tmpseqfil[14];

	/* don't know how to recognize transformed spectrum */
	esspar.datatype = STEXT_datatype_c1fid;	
	esspar.dim1 = 1;			/* no arrays or 2-d for now */
	esspar.dim2 = 1;
	esspar.dim3 = 1;
	esspar.bytes = 4;			/* since we'll expand it */
	esspar.order = STEXT_order_mcsparc;	/* will be reversed */
	/* We will "Redfield" the data to prepare it for the "real FT" that
	   VNMR is supposed to do when it sees acqtyp = STEXT_acqtyp_seqen.
	   Unfortunately however, sread currently ignores acqtyp. */
	esspar.acqtyp = STEXT_acqtyp_seqen;
	esspar.abstyp = STEXT_abstyp_absval;
	esspar.system = BRUKER_PARNAME;
	esspar.np = in_fhead.np;	/* already filled in, from B_tdsize */
	esspar.sw = doubleBFLOAT (&in_header[B_spwidth]);
	esspar.sw1 = 0;				/* todo */
	esspar.sfrq = doubleBFLOAT (&in_header[B_sfreq]);
	esspar.dfrq = doubleBFLOAT (&in_header[B_fx2]);

	stext_essential (&esspar);

	stext_text (BRUKER_TEXT);

	/* N.A.V.P. = Not A Varian Parameter, just added since it's available */
	stext_params_begin ();

	/* sread won't use "file", so we choose another name too */
	if (*filename) {
		stext_params_chars ("file", filename, 0);
		stext_params_chars ("brukfile", filename, 0);
	}

	/* write proc='rft' to force VNMR to learn it should do a real FT */
	stext_params_chars ("proc", "rft", 0);

	intBINT (tempi, &in_header[B_asize]);
	stext_params_long_long ("fn", &tempi);

	intBINT (tempi, &in_header[B_swpcom]);
	stext_params_long_long ("ct", &tempi);

	intBINT (tempi, &in_header[B_tdsize]);
	stext_params_long_long ("np", &esspar.np);

	intBINT (tempi, &in_header[B_dwellt]);
	stext_params_long_long ("dwell", &tempi);		/* N.A.V.P. */

	intBINT (tempi, &in_header[B_fwidth]);
	tempi >> 1;				/* in VNMR it's a half-width */
	stext_params_long_long ("fb", &tempi);

	intBINT (tempi, &in_header[B_vdelay]);
	stext_params_long_long ("vdelay", &tempi);

	intBINT (tempi, &in_header[B_dscans]);
	stext_params_long_long ("ss", &tempi);

	stext_params_double_double ("sw", &esspar.sw);
	tempr = esspar.np / esspar.sw / 2.0;
	stext_params_double_double ("at", &tempr);

	intBINT (tempi, &in_header[B_aqmode]);
	stext_params_long_long ("aqmode", &tempi);

	intBINT (tempi, &in_header[B_texp]);
	stext_params_long_long ("texp", &tempi);

	tof = doubleBFLOAT (&in_header[B_o1]);
	stext_params_double_double ("tof", &tof);

	tempr = doubleBFLOAT (&in_header[B_o2]);
	stext_params_double_double ("dof", &tempr);

	tempr = doubleBFLOAT (&in_header[B_nthpt]);
	stext_params_double_double ("nthpt", &tempr);

	intBINT (tempi, &in_header[B_crdel]);	/* timer word for d1 */
	stext_params_long_long ("crdel", &tempi);

	intBINT (tempi, &in_header[B_cpulse]);	/* timer word for pw */
	stext_params_long_long ("cpulse", &tempi);

	intBINT (tempi, &in_header[B_cdelay]);	/* timer word for pad */
	stext_params_long_long ("cdelay", &tempi);

	intBINT (tempi, &in_header[B_csweep]);
	stext_params_long_long ("nt", &tempi);

	intBINT (tempi, &in_header[B_drstr]);
	stext_params_long_long ("drstr", &tempi);

	intBINT (tempi, &in_header[B_drend]);
	stext_params_long_long ("drend", &tempi);

	intBINT (tempi, &in_header[B_drset]);
	stext_params_long_long ("drset", &tempi);

	intBINT (tempi, &in_header[B_cspdel]);	/* really a timer word */
	stext_params_long_long ("cspdel", &tempi);

	intBINT (tempi, &in_header[B_phzflg]);
	stext_params_long_long ("phzflg", &tempi);

	stext_params_double_double ("sfrq", &esspar.sfrq);

	intBINT (tempi, &in_header[B_quaflg]);
	stext_params_long_long ("quaflg", &tempi);

	intBINT (tempi, &in_header[B_datten]);
	stext_params_long_long ("datten", &tempi);

	intBINT (tempi, &in_header[B_deutl]);
	stext_params_long_long ("deutl", &tempi);

	intBINT (tempi, &in_header[B_temper]);
	tempi -= 273;		/* convert to degrees C */
	stext_params_long_long ("temp", &tempi);

	intBINT (tempi, &in_header[B_rgain]);
	stext_params_long_long ("gain", &tempi);

	intBINT (tempi, &in_header[B_pd0inc]);
	stext_params_long_long ("pd0inc", &tempi);

	tempr = doubleBFLOAT (&in_header[B_sfreq0]);
	stext_params_double_double ("sfreq0", &tempr);

	tempr = doubleBFLOAT (&in_header[B_fx2]);
	stext_params_double_double ("dfrq", &tempr);

	Bsixbit ((unsigned char *) &tmpseqfil[0],
		 (unsigned char *) &in_header[B_saunm1]);
	Bsixbit ((unsigned char *) &tmpseqfil[4],
		 (unsigned char *) &in_header[B_saunm2]);
	Bsixbit ((unsigned char *) &tmpseqfil[9],
		 (unsigned char *) &in_header[B_sauext]);
	tmpseqfil[8]  = '.';
	tmpseqfil[13] = '\0';
	/* quick, very dirty way to remove internal nulls */
	strcpy (seqfil, &tmpseqfil[0]);
	if (tmpseqfil[3] == '\0')	/* 1st word was short */
		strcat (seqfil, &tmpseqfil[4]);
	if (tmpseqfil[7] == '\0')	/* 2nd word was short */
		strcat (seqfil, &tmpseqfil[8]);
	stext_params_chars ("seqfil",  seqfil, 0);
	stext_params_chars ("pslabel", seqfil, 0);

	intBINT (tempi, &in_header[B_rofreq]);
	stext_params_long_long ("spin", &tempi);

	intBINT (tempi, &in_header[B_fmflag]);
	stext_params_long_long ("fmflag", &tempi);

	intBINT (tempi, &in_header[B_fixflg]);
	stext_params_long_long ("fixflg", &tempi);

	intBINT (tempi, &in_header[B_acqdate]);
	stext_params_long_long ("acqdate", &tempi);

	intBINT (tempi, &in_header[B_acqtime]);
	stext_params_long_long ("acqtime", &tempi);

	/* These two don't seem to be exactly sp and wp */
	sp = doubleBFLOAT (&in_header[B_freq2]);	/* right edge */
	sp *= esspar.sfrq;				/* convert to Hz */
	stext_params_double_double ("freq2", &sp);

	tempr = doubleBFLOAT (&in_header[B_freq1]);	/* left edge */
	tempr *= esspar.sfrq;			/* convert to Hz */
	tempr -= sp;				/* convert to plot width */
	stext_params_double_double ("freq1", &tempr);

	tempr = doubleBFLOAT (&in_header[B_offset]);
	stext_params_double_double ("offset", &tempr);

	tempr = doubleBFLOAT (&in_header[B_rfreq]);
	/* Bruker SR is O1 value that puts carrier at 0 Hz */
	tempr = esspar.sw/2 + tempr - tof;
	stext_params_double_double ("rfl", &tempr);
	tempr = 0;
	stext_params_double_double ("rfp", &tempr);

	intBINT (tempi, &in_header[B_minint]);
	stext_params_long_long ("th", &tempi);

	intBINT (tempi, &in_header[B_ymax]);
	stext_params_long_long ("ymax", &tempi);

	intBINT (tempi, &in_header[B_aiflag]);
	stext_params_long_long ("aiflag", &tempi);

	intBINT (tempi, &in_header[B_ainorm]);
	stext_params_long_long ("ainorm", &tempi);

	lp = doubleBFLOAT (&in_header[B_fp1k]);
	/* in VNMR lp is right phase minus left phase, same for Bruker? */
	stext_params_double_double ("lp", &lp);

	tempr = doubleBFLOAT (&in_header[B_fp0k]);
	tempr += lp/2;
	/* in VNMR rp is right phase, in Bruker is center phase */
	stext_params_double_double ("rp", &tempr);

	tempr = doubleBFLOAT (&in_header[B_lbroad]);
	stext_params_double_double ("lb", &tempr);

	tempr = doubleBFLOAT (&in_header[B_gbroad]);
	if (tempr != 0.0) {
		/* convert gb to gf; 0.530020727 is 2sqrt(ln(2))/pi */
		tempr = 0.530020727 / tempr;
		stext_params_double_double ("gf", &tempr);
	}

	intBINT (tempi, &in_header[B_sbshft]);		/* this is wrong? */
	stext_params_long_long ("sbs", &tempi);

	intBINT (tempi, &in_header[B_zpnum]);
	stext_params_long_long ("lsfid", &tempi);

	intBINT (tempi, &in_header[B_t1pt]);
	stext_params_long_long ("t1pt", &tempi);

	intBINT (tempi, &in_header[B_t2pt]);
	stext_params_long_long ("t2pt", &tempi);

	intBINT (tempi, &in_header[B_smctr]);
	stext_params_long_long ("smctr", &tempi);

	tempr = doubleBFLOAT (&in_header[B_gamma]);
	stext_params_double_double ("gamma", &tempr);

	tempr = doubleBFLOAT (&in_header[B_alpha]);
	/* not same as VNMR alfa */
	stext_params_double_double ("alpha", &tempr);

	tempr = doubleBFLOAT (&in_header[B_poycm]);
	tempr *= 10;		/* convert to mm */
	stext_params_double_double ("vo", &tempr);

	tempr = doubleBFLOAT (&in_header[B_poxcm]);
	tempr *= 10;		/* convert to mm */
	stext_params_double_double ("ho", &tempr);

	intBINT (tempi, &in_header[B_dpopt]);
	stext_params_long_long ("dpopt", &tempi);

	tempr = doubleBFLOAT (&in_header[B_dcon]);
	/* this relates to VNMR addi, I think, not awc */
	stext_params_double_double ("dcon", &tempr);

	stext_params_end ();
}


/* Utility function to extract the value of a given bit range of a multi-byte
   object, not limited by byte boundaries.  object is a pointer to the
   object, size is the size of the object in bytes, lowbit is the number of
   the lowest-order bit wanted (lowest in object is bit 0), and numbits is
   the length in bits which should be extracted.  The result is returned as a
   double to avoid integer overflow errors.
*/
double getbits (object, size, lowbit, numbits)
void * object;
int size, lowbit, numbits;
{
	double result = 0;
	double addend;
	register int i, j, bit, elt;
	
	for (i = 0, bit = lowbit; i < numbits; i++, bit++) {
		elt = size - 1 - bit/8;
		if ( ((unsigned char *) object)[elt] & (1 << (bit % 8)) ) {
			addend = 1.0;
			for (j = 0; j < i; j++)
				addend *= 2.0;
			result += addend;
		}
	}
	return (result);
}

/* Given a pointer to a 6-byte Bruker floating point number, converts it to a
   double, which is returned.  The format of the Bruker float seems to be:
   sign:  + if bits 47-46 = 01, - if bits 47-46 = 10 (undefined otherwise?);
   exponent:  two's-complement signed 11-bit integer in bits 23-13 (23 is the
              sign bit) biased by +1;
   mantissa:  35-bit fraction of 2^35, additional 1 implied, with high 22
              bits found in bits 45-24 and low 13 bits in bits 12-0.
   0 is represented as all 0's.
   I'm 90% sure about the signs of negative numbers and not at all sure of
   what happens with negative exponents (haven't seen one).
   Really, I'm not making this up.
*/
double doubleBFLOAT (bfloat)
unsigned char * bfloat;
{
	long sign, exp;
	double mant;
	double number;
	register int i;
	register unsigned char signbits;

	signbits = bfloat[0] & 0xC0;
	if (signbits == 0x40)			/* + */
		sign = 0;
	else if (signbits == 0x80)		/* - */
		sign = 1;
	else if (signbits == 0x00)		/* 0 */
		return (0.0);
	else {				/* 0xC0, don't know what that is */
		errmsg (RC_BADBFLOAT, __FILE__, __LINE__, (long) bfloat);
		cleanup (RC_BADBFLOAT);
	}

	exp = (long) (getbits (bfloat, 6, 13, 11) + 0.5);
	if (exp & (1 << 10))		/* negative, sign-extend to long */
		exp |= 0xFFFFF800;
	--exp;					/* de-bias */

	mant = getbits (bfloat, 6, 24, 22);	/* top 22 bits */
	mant = mant * 8192;			/* like left shift of 13 bits */
	mant += getbits (bfloat, 6, 0, 13);	/* bottom 13 bits */
	
	number = mant / (1024.0 * 1024.0 * 1024.0 * 32.0);  /* divide by 2^35 */
	++number;
	if (exp >= 0) {				/* implement exponent (+) */
		for (i = 0; i < exp; i++)
			number *= 2.0;
	}
	else {					/* implement exponent (-) */
		for (i = 0; i < -exp; i++)
			number /= 2.0;
	}
	if (sign)				/* implement sign */
		number = -number;
	
	return (number);
}

/* Utility function to convert a 3-byte-word of Bruker sixbit into 4
   bytes of ASCII, possibly NOT null-terminated.  Supply a pointer to each
   array.  Currently incomplete; it is not known how characters other than
   null, 26 letters, and 10 digits are stored.  The following is used for
   now:
	Bruker		translated to
	dec	hex	dec	hex	char
	0	0	0	0	null (terminates strings)
	1-26	1-1a	97-122	61-7a	a-z
	27-41	1b-29	33-47	21-2f	!"#$%&'()*+,-./
	42	2a	64	40	@
	43	2b	91	5b	[
	44-47	2c-2f	93-96	5d-60	]^_`     (to avoid '\'!)
	48-57	30-39	48-57	30-39	0-9
	58-63	3a-3f	58-63	3a-3f	:;<=>?
*/
void Bsixbit (outarray, inarray)
unsigned char * outarray, * inarray;
{
	register int j;

	outarray[0] = (inarray[0] & (unsigned char) 0xFC) >> 2;
	outarray[1] = (inarray[0] & (unsigned char) 0x3) << 4 |
	              (inarray[1] & (unsigned char) 0xF0) >> 4;
	outarray[2] = (inarray[1] & (unsigned char) 0xF) << 2 |
	              (inarray[2] & (unsigned char) 0xC0) >> 6;
	outarray[3] = (inarray[2] & (unsigned char) 0x3F);
	for (j = 0; j < 4; j++) {
		/* if outarray[j] == 0, leave it untouched */
		/* if outarray[j] == 48 to 63, leave it untouched */
		if (outarray[j] >= 1  &&  outarray[j] <= 26)
			outarray[j] += 'a' - 1;		/* 'a' is 0x61 */
		/* for now, map the unknowns onto puctuation characters */
		else if (outarray[j] >= 27  &&  outarray[j] <= 41)
			outarray[j] += '!' - 27;	/* '!' is 0x21 */
		else if (outarray[j] == 42)
			outarray[j] += '@';		/* '@' is 0x40 */
		else if (outarray[j] == 43)
			outarray[j] += '[';		/* '[' is 0x5b */
		else if (outarray[j] >= 44  &&  outarray[j] <= 47)
			outarray[j] += ']' - 44;	/* ']' is 0x5d */
	}
}
