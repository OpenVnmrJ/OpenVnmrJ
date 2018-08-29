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
static char Version_ID[] = "m100.c v2.50 STM 9/11/94";
/* revision history:
   2.50	original
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vdata.h"

#include "convert.h"
#include "protos.h"
#include "globals.h"
#include "m100.h"
#include "stext.h"			/* stext file definition */

static struct stext_essparms esspar = {
	STEXT_datatype_c1fid,		/* datatype */
	1, 1, 1,			/* dim1, dim2, dim3 */
	4,				/* bytes */
	STEXT_order_mcsparc,		/* order */
	STEXT_acqtyp_simul,		/* acqtyp */
	STEXT_abstyp_absval,		/* abstyp */
	M100_PARNAME,			/* system */
	0,				/* np */
	0, 0,				/* sw, sw1 */
	0, 0				/* sfrq, dfrq */
};

/* prototypes of local functions */
#ifdef __STDC__

long m100_header(long type);
void m100_stext();
char * contract_m100_filename(char * filename, int * namepart);
char * fix_m100_comment(char * comment, int * length);
double mfloat_to_double(mfloat mf);

#else /* __STDC__ */

long m100_header();
void m100_stext();
char * contract_m100_filename();
char * fix_m100_comment();
double mfloat_to_double();

#endif /* __STDC__ */

/* Reads all the parameters out of an M-100 file and fills the datafileheads
   and datablockheads.  File must start out positioned at the beginning.
*/
long m100_file(type)
long type;
{
	type = m100_header(type);

	if (type & OUT_STEXT)
		m100_stext();

	return(type);
}

long m100_header(type)
long type;
{
	struct m100_header * m100h;

	m100h = (struct m100_header *) malloc(sizeof(struct m100_header));
	in_header = (unsigned char *) m100h;
	if (m100h == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, sizeof(struct m100_header));
		cleanup(RC_NOMEMORY);
	}

	/* M100_HEADER_SIZE is <= sizeof(struct m100_header); see m100.h. */
	if (fread(m100h, M100_HEADER_SIZE, 1, fid_in) != 1 ) {
		errmsg(RC_READERR, __FILE__, __LINE__, M100_HEADER_SIZE);
		free(in_header);
		in_header = NULL;
		cleanup(RC_READERR);
	}

	in_fhead.nblocks = 1;
	in_fhead.ntraces = 1;
	in_fhead.ebytes = 4;
	in_fhead.np  = m100h->al;
	in_fhead.tbytes  = in_fhead.ebytes  * in_fhead.np;
	/* no datablockheads */
	in_fhead.bbytes  = in_fhead.tbytes  * in_fhead.ntraces;
	in_fhead.vers_id = 0;
	in_fhead.status = S_DATA | S_COMPLEX;
	in_fhead.nbheaders = 1;

	out_fhead.nblocks = 1;
	out_fhead.ntraces = 1;
	out_fhead.ebytes = 4;
	out_fhead.np = in_fhead.np;
	out_fhead.tbytes = out_fhead.ebytes * out_fhead.np;
	out_fhead.bbytes = out_fhead.tbytes * out_fhead.ntraces + sizeof(struct datablockhead);
	out_fhead.vers_id = 0;
	out_fhead.status = S_DATA | S_32 | S_COMPLEX;
	out_fhead.nbheaders = 1;

	in_bhead.scale = 0;
	in_bhead.status = S_DATA | S_COMPLEX;
	in_bhead.index = 1;
	in_bhead.mode = 0;
	in_bhead.ctcount = m100h->ac;
	in_bhead.lpval = 0;
	in_bhead.rpval = 0;
	in_bhead.lvl = 0;
	in_bhead.tlt = 0;

	out_bhead.scale = 0;
	out_bhead.status = S_DATA | S_32 | S_COMPLEX;
	out_bhead.index = 1;
	out_bhead.mode = 0;
	out_bhead.ctcount = in_bhead.ctcount;
	out_bhead.lpval = 0;
	out_bhead.rpval = 0;
	out_bhead.lvl = 0;
	out_bhead.tlt = 0;
}

/* Fill in essential parameters structure for stext.  Then send the params
   we've found to the stext file.  */
void m100_stext()
{
	int length;
	int i;
	char * string;
	long templ;
	double tempd;
	struct m100_header * m100h = (struct m100_header *) in_header;
	char tmname[8];

	esspar.np = out_fhead.np;
	esspar.sw = 1.0e7 / m100h->dw;
	esspar.sfrq = m100h->fobs / 1.0e6;
	esspar.dfrq = m100h->fdec / 1.0e6;

	stext_essential (&esspar);

	length = sizeof(m100h->comment);
	string = fix_m100_comment(m100h->comment, &length);
	stext_text(string);
	free(string);

	length = sizeof(m100h->pscomment);
	string = fix_m100_comment(m100h->pscomment, &length);
	stext_text_continued(string);
	free(string);

	stext_params_begin();

	stext_params_long_long("fn", &m100h->dl);
	stext_params_long_long("np", &m100h->al);

	string = contract_m100_filename(m100h->seqfil, &length);
	stext_params_chars("seqfil",  string, length);
	stext_params_chars("pslabel", string, length);

	/* Scale pw1 and pw2 to represent microsec rather than sec.  All
	   other times will be left unscaled. */
	tempd = mfloat_to_double(m100h->pw1) * 1.0e6;
	stext_params_double_double("pw1", &tempd);
	tempd = mfloat_to_double(m100h->pw2) * 1.0e6;
	stext_params_double_double("pw2", &tempd);

	tempd = mfloat_to_double(m100h->pd);
	stext_params_double_double("pd", &tempd);
	tempd = mfloat_to_double(m100h->pred);
	stext_params_double_double("pred", &tempd);

	/* must rename ct since VNMR already uses that name */
	tempd = mfloat_to_double(m100h->ct);
	stext_params_double_double("contact", &tempd);
	tempd = mfloat_to_double(m100h->psd);
	stext_params_double_double("psd", &tempd);
	tempd = mfloat_to_double(m100h->act);
	stext_params_double_double("act", &tempd);
	tempd = mfloat_to_double(m100h->tau);
	stext_params_double_double("tau", &tempd);

	for (i = 0; i < 20; i++) {
		tempd = mfloat_to_double(m100h->tm[i]);
		sprintf(tmname, "tm%d", i+1);
		stext_params_double_double(tmname, &tempd);
	}

	stext_params_long_long("nt", &m100h->na);
	stext_params_long_long("ct", &m100h->ac);

	templ = (long) m100h->dp;
	stext_params_long_long("ss", &templ);

	stext_params_double_double("sw", &esspar.sw);
	stext_params_double_double("sfrq", &esspar.sfrq);
	stext_params_double_double("dfrq", &esspar.dfrq);

	tempd = mfloat_to_double(m100h->lb);
	stext_params_double_double("lb", &tempd);

	tempd = mfloat_to_double(m100h->rm);
	stext_params_double_double("rfp", &tempd);
	tempd = esspar.sw / 2.0;
	stext_params_double_double("rfl", &tempd);

	stext_params_end ();
}

/* Removes embedded spaces and nonprintable chars from a filename.  Returns
   the address of a statically declared string containing the new filename,
   which will be null-terminated.  The integer pointed to by namepart will be
   set to the number of characters in the first part of the new name, up to
   but not including the '.'.
*/
char * contract_m100_filename(filename, namepart)
char * filename;
int * namepart;
{
	int i, j;
	static char newname[13];

	for (i = 0, j = 0; i < 8; i++) {
		if (isalnum(filename[i]))
			newname[j++] = filename[i];
	}
	*namepart = j;
	newname[j++] = '.';
	for (i = 9; i < 12; i++) {
		if (isalnum(filename[i]))
			newname[j++] = filename[i];
	}
	newname[j] = '\0';
	return(newname);
}

/* Scan a text comment in the M-100 header and copy it into a new buffer,
   null-terminating it by replacing the first nonprintable character (usually
   a 0xd or 0x4) with 0x0.  The original comment need not be null-terminated,
   so if all "length" chars turn out to be printable, the new buffer will turn
   out to be 1 char long than the original.  The length of the new comment, as
   would be returned by strlen(), overwrites the contents of "length".  The
   return value is the new string, which is stored in memory malloc'ed by this
   function.  Therefore the caller should free() the string after it is no
   longer needed.
*/
char * fix_m100_comment(comment, length)
char * comment;
int * length;
{
	int i;
	char * new;

	new = malloc(*length + 1);
	if (new == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, *length + 1);
		cleanup(RC_NOMEMORY);
	}
	for (i = 0; i < *length; i++) {
		if (isprint(comment[i]))
			new[i] = comment[i];
		else
			break;
	}

	*length = i;
	comment[i] = '\0';
	return(new);
}

double mfloat_to_double(mf)
mfloat mf;
{
	unsigned long exponent;
	union {
		float f;
		unsigned long ul;
	} result;
	/* Union is used for manipulating result as a bit pattern, then
	   returning it as floating point. */

	/* We must redo the exponent by expanding it from 7 to 8 bits and
	   changing the bias from 64 to 127.  The brute-force way is to mask
	   it off (using & 0x7f000000), right-shift it 24 places to put it at
	   the least-significant end of the word, deduct 64 and add 127 in its
	   place, then left-shift the result (now up to 8 bits) by 23 places:
	       exponent = (((*m100 & 0x7f000000) >> 24) - 64 + 127) << 23;
	   All of this can be simplified since the shift operations are
	   distributive.  In the equation below, 0x1f800000 is 63<<23. */
	   
	exponent = ( (mf & 0x7f000000) >> 1 ) + 0x1f800000;

	/* Now substitute the new exponent for bits 30 through 23 of the
	   original.  Note that bit 23 was formerly a mantissa bit, but it was
	   superfluous. */

	result.ul = (mf & 0x807fffff) | exponent;

	return((double) result.f);
}
