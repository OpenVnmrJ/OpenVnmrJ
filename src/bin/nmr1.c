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
static char Version_ID[] = "nmr1.c v2.71 STM 3/13/98";
/* Revision history:
   2.30 adapt to Varian data.h v6.12 data structure elements, cosmetic (2/93)
   2.40 cosmetic change of stext function names
   2.71	casting changes to please Sun C compiler
*/

#include <stdio.h>
#include <stdlib.h>
#include "vdata.h"

#include "convert.h"
#include "protos.h"
#include "globals.h"
/* The following header comes with NMR1.  It's a pretty old version. */
#include "fdatadef.h"			/* NMR1 data file definition */
#include "stext.h"			/* stext file definition */

/* local prototypes */
#ifdef __STDC__
void fix_nmr1_header (unsigned char * header);
void nmr1_stext (void);
void nmr1_vtext (void);

#else /* __STDC__ */

void fix_nmr1_header ();
void nmr1_stext ();
void nmr1_vtext ();

#endif /* __STDC__ */

#define NMR1_HEADSIZE	4*FDATASIZE		/* 512 4-byte words */
#define NMR1_PARNAME	"nmr1"			/* name of parlib file */

/* convert from float to long, given generic pointer pointing to float */
#define FIX(f)	((long) (* ((float *) (&(f)))))

/* convert from float to double, given generic pointer pointing to float */
#define DOUBLE(f)	((double) (* ((float *) (&(f)))))

void nmr1_file (type)
long type;
{
	in_header = (unsigned char *) malloc (NMR1_HEADSIZE);
	if (in_header == NULL) {
		errmsg (RC_NOMEMORY, __FILE__, __LINE__, NMR1_HEADSIZE);
		cleanup (RC_NOMEMORY);
	}

	/* read header */
	if (fread (in_header, NMR1_HEADSIZE, 1, fid_in) != 1 ) {
		errmsg (RC_READERR, __FILE__, __LINE__, NMR1_HEADSIZE);
		free (in_header);
		in_header = NULL;
		cleanup (RC_READERR);
	}

	fix_nmr1_header (in_header);

	in_fhead.nblocks = 1;
	in_fhead.ntraces = 1;
	in_fhead.ebytes = 4;
	in_fhead.vers_id = 0;
	in_fhead.status = S_DATA | S_FLOAT | S_COMPLEX;
	in_fhead.nbheaders = 1;
	out_fhead.nblocks = 1;
	out_fhead.ntraces = 1;
	out_fhead.ebytes = 4;
	out_fhead.vers_id = 0;
	out_fhead.status = S_DATA | S_32 | S_COMPLEX;
	out_fhead.nbheaders = 1;

	/* complex points ->total points */
	in_fhead.np = FIX (in_header[4*FDRealData]) * 2;
	in_fhead.tbytes = in_fhead.np * in_fhead.ebytes;
	in_fhead.bbytes = in_fhead.ntraces*in_fhead.tbytes;
	out_fhead.np = in_fhead.np;
	out_fhead.tbytes = out_fhead.np * out_fhead.ebytes;
	out_fhead.bbytes = out_fhead.ntraces*out_fhead.tbytes + sizeof (struct datablockhead);

	in_bhead.status = S_DATA | S_FLOAT | S_COMPLEX;
	in_bhead.index = 0x3020;		/* don't understand this */
	in_bhead.mode = 0;
	in_bhead.lpval = 0;
	in_bhead.rpval = 0;
	in_bhead.lvl = 0;
	in_bhead.tlt = 0;
	out_bhead.status = S_DATA | S_32 | S_COMPLEX;
	out_bhead.index = 0x3020;		/* don't understand this */
	out_bhead.mode = 0;
	out_bhead.lpval = 0;
	out_bhead.rpval = 0;
	out_bhead.lvl = 0;
	out_bhead.tlt = 0;

	in_bhead.scale = FIX (in_header[4*AbsScale]);
	in_bhead.ctcount = FIX (in_header[4*nscan]);
	out_bhead.scale = in_bhead.scale;
	out_bhead.ctcount = in_bhead.ctcount;

	if (type & OUT_STEXT)
		nmr1_stext ();
	if (type & OUT_VTEXT)
		nmr1_vtext ();
}

/* Converts the Vax-format data in the header to the same data in our native
   format.
*/
void fix_nmr1_header (header)
unsigned char * header;
{
	register int f;

	/* All data in the header is either Vax D_floating or ascii strings. 
	   Both need to be byte-reversed at the shortword level, so just reverse
	   the whole header.
	*/
	reverse_bytes (header, FDATASIZE * 2, 2);

	/* Final conversion step for floating point is to divide by 4 to
	   compensate for the Vax exponent being biased by 129 instead of 127 as
	   in IEEE floating point.  The complicated condition is needed to avoid
	   all those locations that hold ascii strings instead of floats.
	*/
	for (f = 0; f < FDATASIZE; f++) {
		if (  ( (f < 179)               ||	/* first, avoid ranges of ascii */
				(f > 186  &&  f < 279)  ||
				(f > 280  &&  f < 286)  ||
				(f > 289  &&  f < 297)  ||
				(f > 351)
			  ) &&
			  (	f != 114  &&	/* then check for individual ascii longwords */
				f != 120  &&
				f != 141  &&
				f != 202
			  )
			)
			((float *) header)[f] /= 4.0;
	}
}

void nmr1_stext ()
{
	struct stext_essparms esspar;
	float temp;

	if (in_header[4*FtFlag] == 0)
		esspar.datatype = STEXT_datatype_c1fid;
	else
		esspar.datatype = STEXT_datatype_p1spc;
	esspar.dim1 = 1;			/* no arrays or 2-d for now */
	esspar.dim2 = 1;
	esspar.dim3 = 1;
	esspar.bytes = 4;
	esspar.order = STEXT_order_mcsparc;		/* will be reversed */
	esspar.acqtyp = STEXT_acqtyp_simul;
	esspar.abstyp = STEXT_abstyp_absval;
	esspar.system = NMR1_PARNAME;
	esspar.np = in_fhead.np;				/* already filled in */
	esspar.sw = DOUBLE (in_header[4*SweepWidth]);
	esspar.sw1 = DOUBLE (in_header[4*SecondDimSw]);
	esspar.sfrq = DOUBLE (in_header[4*SpecFreq]);
	esspar.dfrq = DOUBLE (in_header[4*DecouplerFreq]);

	stext_essential (&esspar);

	stext_text ((char *) (&in_header[4*comment + 2]));
	fprintf (params_out, "%s\n", &in_header[4*title]);

	/* N.A.V.P. = Not A Varian Parameter, just added since it's available */
	stext_params_begin ();
	temp = (* ((float *) &in_header[4*FDRealData])) * 2;
	stext_params_long_float ("np", &temp);
	stext_params_double_float ("dwell", &in_header[4*dwell]);	/* N.A.V.P. */
	stext_params_double_float ("sw", &in_header[4*SweepWidth]);
	stext_params_double_float ("at", &in_header[4*AcqTime]);
	stext_params_long_float ("ct", &in_header[4*nscan]);
	stext_params_long_float ("nt", &in_header[4*nscan]);
	stext_params_double_float ("d1", &in_header[4*ScanInterval]);
/*	stext_params_double_float ("lp", &in_header[4*FirstPhase]);
	stext_params_double_float ("rp", &in_header[4*ZeroPhase]);*/
	stext_params_double_float ("lb", &in_header[4*lb]);
	stext_params_double_float ("alfa", &in_header[4*PulseAcqDly]);	/* ? */
	temp = (* ((float *) &in_header[4*filter])) * 1000;
	stext_params_double_float ("fb", &temp);
	if (in_header[4*exname])
		stext_params_chars ("seqfil", &in_header[4*exname], 4);
	stext_params_double_float ("dfrq", &in_header[4*DecouplerFreq]);
	stext_params_double_float ("sfrq", &in_header[4*SpecFreq]);
	stext_params_double_float ("pw", &in_header[4*PulseWidth]);
	stext_params_double_float ("temp", &in_header[4*temperature]);
	if (in_header[4*Spectrometer])
		stext_params_chars ("SystemID", &in_header[4*Spectrometer], 8);
	stext_params_end ();
}

void nmr1_vtext ()
{
	fprintf (text_out, "%s\n", &in_header[4*comment + 2]);
	fprintf (text_out, "%s\n", &in_header[4*title]);
}
