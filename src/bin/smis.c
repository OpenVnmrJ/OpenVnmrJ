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
static char Version_ID[] = "smis.c v2.41 STM 6/1/93";
/* revision history:
   2.30	original, for simple 1-fid native-format files only
   2.31 1-D array and 2-D support; generic on/off qualifiers for strings
   2.32	better scaling of SMIS parameters, and basenaming of strings
   2.33	fixed np counting bug
   2.41	allow oversized VAR_ARRAYs, fix basename bug
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vdata.h"

#include "convert.h"
#include "protos.h"
#include "globals.h"
#include "smis.h"
#include "stext.h"			/* stext file definition */

static struct stext_essparms esspar = {
	STEXT_datatype_c1fid,		/* datatype */
	1, 1, 1,			/* dim1, dim2, dim3 */
	4,				/* bytes */
	STEXT_order_mcsparc,		/* order (will be reversed) */
	STEXT_acqtyp_simul,		/* acqtyp */
	STEXT_abstyp_absval,		/* abstyp */
	SMIS_PARNAME,			/* system */
	0,				/* np */
	0, 0,				/* sw, sw1 */
	0, 0				/* sfrq, dfrq */
};

/* prototypes of local functions */
#ifdef __STDC__

long smis_mrd_header (long type);
void fix_smis_header (struct SMIS_header * header);
int smis_make_tokenvec (char * buf, char ** tokenv);
long smis_mrd_footer (long type);
int smis_mrd_smpfile (struct smispar * par, char * buffer, long * typep);
int smis_parse (struct smispar * par, char * buffer, long * typep);
int smis_fill_array (struct smispar * par, char * buffer, int num_tokens, char ** tokenv, int arrayelts, int storeelts);
double smis_translate_float_par (struct smispar * par);
char * smis_translate_string_par (struct smispar * par);
char * smis_basename (char * fullname);
void smis_stext (struct smispar * par, int num_pars);
void write_smispar_stext (struct smispar * par, int num_pars);

#else /* __STDC__ */

long smis_mrd_header ();
void fix_smis_header ();
int smis_make_tokenvec ();
long smis_mrd_footer ();
int  smis_mrd_smpfile ();
int smis_parse ();
int smis_fill_array ();
double smis_translate_float_par ();
char * smis_translate_string_par ();
char * smis_basename ();
void smis_stext ();
void write_smispar_stext ();

#endif /* __STDC__ */

/* Reads all the parameters out of a .MRD file and fills the datafileheads
   and datablockheads.  File must start out positioned at the beginning.
   Since the parameter information is found at both the beginning and end,
   with fid data in between, a seek is performed to reach the parameter
   "footer" after reading the header.  After reading the footer, the file is
   seeked back to right after the header, at the beginning of the data.  So
   this function will fail on nonseekable inputs.
*/
long smis_file (type)
long type;
{
	long start_of_data;

	type = smis_mrd_header (type);
	start_of_data = ftell (fid_in);
	if (fseek (fid_in, in_fhead.bbytes, SEEK_CUR) != 0) {
		errmsg (RC_SKIPERR, __FILE__, __LINE__, in_fhead.bbytes);
		cleanup (RC_SKIPERR);
	}
	type = smis_mrd_footer (type);
	if (fseek (fid_in, start_of_data, SEEK_SET) != 0) {
		errmsg (RC_SKIPERR, __FILE__, __LINE__, start_of_data);
		cleanup (RC_SKIPERR);
	}
	if (type & OUT_STEXT)
		smis_stext (NULL, 0);
	return (type);
}

long smis_mrd_header (type)
long type;
{
	struct smispar par[NUMSMISPAR];		/* holds pars found in header */
	int is_2d;

	in_header = (unsigned char *) malloc (sizeof (struct SMIS_header));
	if (in_header == NULL) {
		errmsg (RC_NOMEMORY, __FILE__, __LINE__, sizeof (struct SMIS_header));
		cleanup (RC_NOMEMORY);
	}

	/* read header */
	if (fread (in_header, sizeof (struct SMIS_header), 1, fid_in) != 1 ) {
		errmsg (RC_READERR, __FILE__, __LINE__, sizeof (struct SMIS_header));
		free (in_header);
		in_header = NULL;
		cleanup (RC_READERR);
	}

	fix_smis_header ((struct SMIS_header *) in_header);

	in_fhead.nblocks = 1;
	in_fhead.vers_id = 0;
	in_fhead.status = S_DATA | S_COMPLEX;
	in_fhead.nbheaders = 1;

	out_fhead.nblocks = 1;
	out_fhead.ebytes = 4;
	out_fhead.vers_id = 0;
	out_fhead.status = S_DATA | S_32 | S_COMPLEX;
	out_fhead.nbheaders = 1;

	in_bhead.scale = 0;
	in_bhead.status = S_DATA | S_COMPLEX;
	in_bhead.index = 1;
	in_bhead.mode = 0;
	in_bhead.lpval = 0;
	in_bhead.rpval = 0;
	in_bhead.lvl = 0;
	in_bhead.tlt = 0;

	out_bhead.scale = 0;
	out_bhead.status = S_DATA | S_32 | S_COMPLEX;
	out_bhead.index = 1;
	out_bhead.mode = 0;
	out_bhead.lpval = 0;
	out_bhead.rpval = 0;
	out_bhead.lvl = 0;
	out_bhead.tlt = 0;

	/* reject non-complex data, whatever that is */
	if (! ( ((struct SMIS_header *) in_header)->datatypecode & SMIS_DTC_COMPLEX ) ) {
		fprintf (stderr, "file conversion not implemented yet for non-complex SMIS data.\n");
		cleanup (RC_INOTYET);
	}

	/* validate and set up incoming word size and integer/float status */
	switch (((struct SMIS_header *) in_header)->datatypecode & SMIS_DTC_SIZE_MASK) {
	case SMIS_DTC_UCHAR:	/* take your own chances with this one */
	case SMIS_DTC_SCHAR:
		in_fhead.ebytes = 1;
		break;
	case SMIS_DTC_SHORT:
	case SMIS_DTC_INT:
		in_fhead.ebytes = 2;
		break;
	case SMIS_DTC_LONG:
		in_fhead.ebytes = 4;
		break;
	case SMIS_DTC_FLOAT:
		in_fhead.ebytes = 4;
		type |= IEEE_FLOAT;
		break;
	case SMIS_DTC_DOUBLE:	/* error out since can't contract this yet */
	default:
		fprintf (stderr, "file conversion not implemented yet for double precision data.\n");
		cleanup (RC_INOTYET);
		break;
	}

	/* ncp1 is half of what Varian calls np, since it measures complex points */
	in_fhead.np  = 2 * ((struct SMIS_header *) in_header)->ncp1;
	out_fhead.np = in_fhead.np;
	in_fhead.tbytes  = in_fhead.ebytes  * in_fhead.np;
	out_fhead.tbytes = out_fhead.ebytes * out_fhead.np;

	/* ncp2 and ncp3 are roughly what Varian would call ni and ni2, plus
	   or minus a possible factor of 2 for hypercomplex or
	   superhypercomplex pairing.  nslice is presumably equivalent to the
	   "ni3" of 4-D data.  So for an absolute value 2-D, the number of
	   words present is 2*ncp1*ncp2.  I have not seen a phase-sensitive
	   2-D yet.  nsarray appears to be Varian's arraydim, except that
	   unlike arraydim it is 1 for 2-D experiments (unless they're
	   arrayed?).  So we use this scheme to figure out ntraces.  Assume
	   that nblocks is always 1.
	*/
	if ( ((struct SMIS_header *) in_header)->ncp2   > 1  ||
	     ((struct SMIS_header *) in_header)->ncp3   > 1  ||
	     ((struct SMIS_header *) in_header)->nslice > 1  )
	 	is_2d = 1;		/* really means is 2- through 4-D */
	else
	 	is_2d = 0;
	 
	if (is_2d) {			/* require that it's not arrayed! */
		if ( ((struct SMIS_header *) in_header)->nsarray > 1 ) {
			fprintf (stderr, "file conversion not implemented yet for arrayed 2-D data.\n");
			cleanup (RC_INOTYET);
		}
		in_fhead.ntraces = out_fhead.ntraces =
			((struct SMIS_header *) in_header)->ncp2 *
			((struct SMIS_header *) in_header)->ncp3 *
			((struct SMIS_header *) in_header)->nslice;
	}
	else
		in_fhead.ntraces = out_fhead.ntraces =
			((struct SMIS_header *) in_header)->nsarray;

	in_fhead.bbytes  = in_fhead.tbytes  * in_fhead.ntraces;	/* no datablockheads */
	out_fhead.bbytes = out_fhead.tbytes * out_fhead.ntraces + sizeof (struct datablockhead);

	/* Fill in essential parameters structure for stext, if needed.  Then
	   send the params we've found to the stext file.  */
	if (type & OUT_STEXT) {
		esspar.np    = out_fhead.np;
		esspar.dim1  = ((struct SMIS_header *) in_header)->ncp2;
		esspar.dim2  = ((struct SMIS_header *) in_header)->ncp3;
		esspar.dim3  = ((struct SMIS_header *) in_header)->nslice;
		if (!is_2d)
			esspar.dim1 *= ((struct SMIS_header *) in_header)->nsarray;
		esspar.bytes = out_fhead.ebytes;
		/* sw, sw1, sfrq, dfrq must be discovered later in smis_parse() */
		par[0].name = "np";
		par[0].value.l = out_fhead.np;
		par[0].type = SMISPAR_LONG;
		par[1].name = "ni";
		par[1].value.l = ((struct SMIS_header *) in_header)->ncp2;
		par[1].type = SMISPAR_LONG;
		par[2].name = "ni2";
		par[2].value.l = ((struct SMIS_header *) in_header)->ncp3;
		par[2].type = SMISPAR_LONG;
		par[3].name = "ni3";
		par[3].value.l = ((struct SMIS_header *) in_header)->nslice;
		par[3].type = SMISPAR_LONG;
		par[4].name = "arraydim";
		par[4].value.l = out_fhead.ntraces;
		par[4].type = SMISPAR_LONG;

		/* Varian convention is that unused "ni" dimensions are set to
		   0 rather than 1. */
		if (par[1].value.l == 1)
			par[1].value.l = 0;
		if (par[2].value.l == 1)
			par[2].value.l = 0;
		if (par[3].value.l == 1)
			par[3].value.l = 0;

		smis_stext (par, 5);
	}

	return (type);
}

/* Converts the little-endian data in the header to the same data in our
   native format.
*/
void fix_smis_header (header)
struct SMIS_header * header;
{
	/* Most data in the header is either long or short ints, which need
	   to be reversed separately, but there are some unused items
	   interspersed which out of propriety we leave untouched.
	*/
	reverse_bytes ((unsigned char *) &header->ncp1,         4, 4);
	reverse_bytes ((unsigned char *) &header->datatypecode, 1, 2);
	reverse_bytes ((unsigned char *) &header->nsarray,      1, 4);
}

/* Preprocess a parameter line.  Validate that it has the
   ":KEY word [word]..." format, then break it up into individual tokens by
   replacing the word separators by nulls.  Doublequoted strings are treated
   as single tokens for this purpose, but the " characters themselves are
   treated as separators.  The addresses of each token found are placed in
   the argv-like vector tokenv.  The vector tokenv is allocated by the caller
   and must have at least TOKENVSIZE elements.  By analogy with argv, the
   tokenv element immediately after the last one filled will be set to NULL;
   this means at most TOKENVSIZE-1 words can be recovered from the line.  The
   number of tokens found (corresponding to argc) is returned to the caller;
   the final NULL token is not included in this count.  A return value of 0
   means the line was empty or invalid; a return of TOKENVSIZE means that the
   line contained more words than could be recovered, but the first
   TOKENVSIZE-1 of them were placed in tokenv, followed by NULL.  Remember
   not to dereference the NULL!
*/
int smis_make_tokenvec (buf, tokenv)
char * buf;
char ** tokenv;
{
	register int tokenc;	/* tokens found so far, also next tokenv to fill */
	register char * p;	/* buffer-walking pointer */
	register int state;	/* we're a state machine, states are:
					0	before token
					1	in regular token
					2	in quoted token
				*/

	if (buf[0] != ':') {	/* parameter lines all start with : */
		tokenv[0] = NULL;
		return (0);
	}

	/* walk through buffer until null, breaking out tokens */
	for (p = &buf[1], state = 0, tokenc = 0; *p; p++) {
		if (state == 2) {		/* only " and \0 break this state */
			if (*p == '"') {		/* the closing quote? */
				*p = '\0';		/* terminate token */
				state = 0;		/* look for next token */
			}
		}
		else if (strchr (SEPARATORS, *p)) {	/* a separator! */
			if (state == 1) {		/* are we in a token? */
				*p = '\0';		/* terminate token */
				state = 0;		/* look for next token */
			}
		}
		else {					/* non-separator, a token! */
			if (state == 0) {		/* looking for a token? */
				if (tokenc >= TOKENVSIZE-1)	/* any room left? */
					break;			/* one too many */
				if (*p == '"') {		/* quoted? */
					tokenv[tokenc++] = p + 1;   /* yes, skip " */
					state = 2;
				}
				else {
					tokenv[tokenc++] = p;	    /* no */
					state = 1;
				}
			}
		}
	}
	
	tokenv[tokenc] = NULL;
	return (tokenc);
}

/* Interprets the footer at the end of the .MRD file.  This consists of a
   fixed-size block containing the filename where the sample info is located
   on the SMIS, followed by a variable number of lines of formatted ASCII
   parameter name/value sets which are specific to the .PPL program which
   acquired the data.
*/
long smis_mrd_footer (type)
long type;
{
	char buffer[BUFFERSIZE];		/* holds one line */
	struct smispar * par;			/* smispars for line */
	int smispar_array_size;			/* smispar structs allocated */
	register int numpars;			/* how many we got */

	/* Allocate enough smispars to hold as many parameters as there are
	   likely to be on the line.  For arrayed variables, this will be a few
	   more than the arraydim. */
	if ( ((struct SMIS_header *) in_header)->ncp2   > 1  ||
	     ((struct SMIS_header *) in_header)->ncp3   > 1  ||
	     ((struct SMIS_header *) in_header)->nslice > 1  )
		smispar_array_size = NUMSMISPAR;	/* arrayed 2-D not allowed */
	else if ( ((struct SMIS_header *) in_header)->nsarray > 1 )
		/* arrayed 1-D; need extra smispars to hold array values */
		smispar_array_size = NUMSMISPAR + ((struct SMIS_header *) in_header)->nsarray;
	else
		smispar_array_size = NUMSMISPAR;	/* single 1-D */

	par = (struct smispar *) malloc (smispar_array_size * sizeof (struct smispar));
	if (par == NULL) {
		errmsg (RC_NOMEMORY, __FILE__, __LINE__, smispar_array_size * sizeof (struct smispar));
		return (RC_NOMEMORY);
	}

	/* Read the sample file name into the buffer, then maybe into a
	   smispar. */
	numpars = smis_mrd_smpfile (par, buffer, &type);
	if (numpars > 0) {
		if (type & OUT_STEXT)
			smis_stext (par, numpars);
	}

	/* Read the whole rest of the file, interpreting each line as a
	   parameter line which is parsed into a number of smispars. */
	for (;;) {
		if (fgets (buffer, BUFFERSIZE, fid_in) == NULL)
			break;
		numpars = smis_parse (par, buffer, &type);
		if (numpars > 0) {
			if (type & OUT_STEXT)
				smis_stext (par, numpars);
		}
	}
	free ((char *) par);
	return (type);
}

/* In "well-formed" SMIS files I have seen, a block of chars following the
   data contains the name of the sample text file, padded out to a total of
   120 (SMIS_FOOTER_SMPLEN) bytes with nulls.  However I have seen 1 file (a
   suspicious-looking 2-D which SPECANA nevertheless accepted) which lacked
   this whole block, and in addition had lines of text interspersed between
   the parameters that followed.  This function reads the smpfile out of the
   first form and will unreasonably choke and trash the conversion of the
   second form.  If the second form turns out not to be anomalous, this
   function and smis_parse() will have to become very much more complicated.
   It leaves the file positioned before the ':' of the first real parameter
   that follows.
*/
int smis_mrd_smpfile (par, buffer, typep)
struct smispar * par;
char * buffer;
long * typep;
{
	int num_pars = 0;

	if (fread (buffer, SMIS_FOOTER_SMPLEN, 1, fid_in) != 1 ) {
		errmsg (RC_READERR, __FILE__, __LINE__, SMIS_FOOTER_SMPLEN);
		cleanup (RC_READERR);
	}
	if (isprint (buffer[0])) {
		buffer[SMIS_FOOTER_SMPLEN-1] = '\0';	/* null terminate */
		par[0].name = "smpfile";
		par[0].type = SMISPAR_STRING;
		par[0].value.s = buffer;
		par[0].value.s = smis_translate_string_par (&par[0]);
		num_pars = 1;
	}
	return (num_pars);
}

/* Parse a line from a SMIS parameter block.  Recognizes the parameter name, 
   value, and any necessary conversions.  Knows about floating point, string,
   and integer variables; all integers are handled as longs.  Some SMIS
   parameter line may contain several items which should be interpreted as
   separate parameters (e.g. the equivavents of Varian tn and sfrq are both
   found in one OBSERVE_FREQUENCY line); in addition, one SMIS parameter may
   generate several outside-world parameters (e.g. a SAMPLE_PERIOD line has
   only one SMIS parameter but generates both "dwell" and "sw".  Parameters
   are stored in smispar structures, which contain name, type, and value.  An
   array of NUMSMISPAR of these must be provided.  The number successfully
   filled is returned.  0 means no parameters were found on the line;
   -NUMSMISPAR means that NUMSMISPAR were found and placed in par, but there
   are more on the line that there was not room for in par.  The caller can
   choose whether to make this a fatal error.
   
   This function may modify the caller's "type" variable through the pointer
   provided.

   If the output format is OUT_STEXT, the sw, sfrq, and dfrq parameters are
   captured and filled into the esspar structure.  Currently we dont' know
   how to detect sw1 so that must remain as its initialized value.
   
   NOTE:  smispar.name and smispar.value.s are char pointers and in some
   cases may be filled with the *addresses* of the original token strings,
   which are part of the buffer.  Therefore these addresses only remain valid
   as long as the caller does not destroy the buffer.
   
   NOTE:  the size of the par array is assumed to be at least 2 (or
   2+arraydim for arrayed 1-D's).
*/
int smis_parse (par, buffer, typep)
struct smispar * par;
char * buffer;
long * typep;
{
	register int num_pars = 0;
	register int num_tokens;
	char * tokenv[TOKENVSIZE];
	int arrayelts;
	
	num_tokens = smis_make_tokenvec (buffer, tokenv);
	
	if (num_tokens == 0)
		return (0);
	if (num_tokens >= TOKENVSIZE)
		fprintf (stderr, "smis_parse:  found too many tokens on '%s' line, proceeding anyway...\n", tokenv[0]);

	/* num_tokens is at least 1.  Processing of line depends on first token. */
	if (!strcmp (tokenv[0], "END")) {	/* end of parameter block */
		return (0);
	}
	else if (!strcmp (tokenv[0], "PPL")) {
		if (num_tokens != 2) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "pplfile";
		par[0].type = SMISPAR_STRING;
		par[0].value.s = tokenv[1];
		par[0].value.s = smis_translate_string_par (&par[0]);
		num_pars = 1;
	}
	else if (!strcmp (tokenv[0], "DSP_ROUTINE")) {
		if (num_tokens != 2) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "dspfile";
		par[0].type = SMISPAR_STRING;
		par[0].value.s = tokenv[1];
		par[0].value.s = smis_translate_string_par (&par[0]);
		num_pars = 1;
	}
	else if (!strcmp (tokenv[0], "NO_SAMPLES")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "np";
		par[0].type = SMISPAR_LONG;
		if (sscanf (tokenv[2], "%ld", &par[0].value.l) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].value.l *= 2;		/* since was complex pairs */
		num_pars = 1;
	}
	else if (!strcmp (tokenv[0], "SAMPLE_PERIOD")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		/* This is really a dwell time in units of 0.1 usec.  Convert to
		   Hz (for a simultaneously sampled scheme).  Also set "at". */
		par[0].name = "sw";
		par[0].type = SMISPAR_DOUBLE;
		if (sscanf (tokenv[2], "%lf", &par[0].value.d) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].value.d = 1.0e7 / par[0].value.d;
		par[1].name = "at";
		par[1].type = SMISPAR_DOUBLE;
		/* np/2 since in VNMR definition it refers to reals + imags */
		par[1].value.d = in_fhead.np/2 / par[0].value.d;
		num_pars = 2;
		if (*typep & OUT_STEXT)
			esspar.sw = par[0].value.d;
	}
	else if (!strcmp (tokenv[0], "SAMPLE_PERIOD_2")) {	/* for 2-D */
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		/* This is really a dwell time in units of 0.1 usec.  Convert to
		   Hz (for a simultaneously sampled scheme).  Also set at1. */
		par[0].name = "sw1";
		par[0].type = SMISPAR_DOUBLE;
		if (sscanf (tokenv[2], "%lf", &par[0].value.d) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].value.d = 1.0e7 / par[0].value.d;
		par[1].name = "at1";
		par[1].type = SMISPAR_DOUBLE;
		par[1].value.d = ((struct SMIS_header *) in_header)->ncp2 / par[0].value.d;
		num_pars = 2;
		if (*typep & OUT_STEXT)
			esspar.sw1 = par[0].value.d;
	}
	else if (!strcmp (tokenv[0], "OBSERVE_FREQUENCY")) {
		if (num_tokens != 7) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "tn";
		par[0].type = SMISPAR_STRING;
		par[0].value.s = tokenv[1];
		par[1].name = "sfrq";
		par[1].type = SMISPAR_DOUBLE;
		if (sscanf (tokenv[2], "%lf", &par[1].value.d) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[1].value.d *= 1.0e-6;		/* convert to MHz */
		num_pars = 2;
		if (*typep & OUT_STEXT)
			esspar.sfrq = par[1].value.d;
	}
	else if (!strcmp (tokenv[0], "DECOUPLE_FREQUENCY")) {
		if (num_tokens != 7) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "dn";
		par[0].type = SMISPAR_STRING;
		par[0].value.s = tokenv[1];
		par[1].name = "dfrq";
		par[1].type = SMISPAR_DOUBLE;
		if (sscanf (tokenv[2], "%lf", &par[1].value.d) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[1].value.d *= 1.0e-6;		/* convert to MHz */
		num_pars = 2;
		if (*typep & OUT_STEXT)
			esspar.dfrq = par[1].value.d;
	}
	else if (!strcmp (tokenv[0], "NO_AVERAGES")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "nt";
		par[0].type = SMISPAR_LONG;
		if (sscanf (tokenv[2], "%ld", &par[0].value.l) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		num_pars = 1;
	}
	else if (!strcmp (tokenv[0], "VIEW_BLOCK")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "bs";
		par[0].type = SMISPAR_LONG;
		if (sscanf (tokenv[2], "%ld", &par[0].value.l) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		num_pars = 1;
	}
	else if (!strcmp (tokenv[0], "PHASE_CYCLE")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "phasecyc";
		par[0].type = SMISPAR_LONG;
		if (sscanf (tokenv[2], "%ld", &par[0].value.l) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		num_pars = 1;
	}
	else if (!strcmp (tokenv[0], "DISCARD")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "discard";
		par[0].type = SMISPAR_LONG;
		if (sscanf (tokenv[2], "%ld", &par[0].value.l) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		num_pars = 1;
	}
	/* EXPERIMENT_ARRAY and VAR_ARRAY are for arrayed experiments.  We do
	   extra validation on EXPERIMENT_ARRAY to make sure it conforms to
	   the data size (nsarray or ntraces) we already know from the header
	   (remember for now we only do 1-D arrays).  VAR_ARRAY may contain
	   more elements than the experiment actually used, so its size is
	   greater than or equal to the value in EXPERIMENT_ARRAY. */
	else if (!strcmp (tokenv[0], "EXPERIMENT_ARRAY")) {
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = "arraydim";
		par[0].type = SMISPAR_LONG;
		if (sscanf (tokenv[2], "%ld", &par[0].value.l) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		if (par[0].value.l != in_fhead.ntraces) {
			fprintf (stderr, "EXPERIMENT_ARRAY size does not match data size.\n");
			cleanup (RC_BADPARAMLINE);
		}
		num_pars = 1;
	}
	/* VAL_ARRAY's tokens are the actual arrayed variable name, the
	   number of values it takes on, and the values.  Unlike all other
	   parameter types, this one can span multiple lines, with values
	   separated by newlines, commas, and spaces.  We cannot predict in
	   advance how many lines, or whether the total will overflow the
	   buffer.  So instead we require that at least the value count must
	   be on the first line.  Then we absorb values from the rest of that
	   line and we keep reading more lines one by one until we have
	   enough values.  This is the only section that either destroys the
	   original buffer (apart from tokenizing) or moves the file
	   position.
	*/
	else if (!strcmp (tokenv[0], "VAR_ARRAY")) {
		if (num_tokens < 3) {		/* must be at least 3 */
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		/* Token 2 is the arraydim, same as in EXPERIMENT_ARRAY. */
		if (sscanf (tokenv[2], "%ld", &arrayelts) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		if (arrayelts < in_fhead.ntraces) {
			fprintf (stderr, "VAL_ARRAY size is less than data size.\n");
			cleanup (RC_BADPARAMLINE);
		}
		/* This does all the dirty work.  The VNMR "array" parameter
		   is left unfilled since sread handles that. */
		num_pars = smis_fill_array (par, buffer, num_tokens, tokenv, arrayelts, in_fhead.ntraces);
	}
	else if (!strcmp (tokenv[0], "VAR")) {
		/* VAR can be anything:  tokenv[1] will be the name of a
		   variable whose definition is known only to the PPL
		   program.  Try to be smart and test tokenv[2]; if it can be
		   scanned by sscanf() as a valid floating point number, it
		   is treated as one; otherwise it is treated as a string.
		   Call either smis_translate_string_par()  or
		   smis_translate_float_par() to see if it is a common one
		   for which a conventional conversion is known.  Beware:
		   parameter names longer than 8 chars are legal in SMIS but
		   maybe not in other software; we take a chance and leave
		   them untouched though.
		*/
		if (num_tokens != 3) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].name = tokenv[1];
		if (sscanf (tokenv[2], "%lf", &par[0].value.d) == 1) {
			par[0].type = SMISPAR_DOUBLE;
			par[0].value.d = smis_translate_float_par (&par[0]);
		}
		else {					/* ack, it's a string */
			par[0].type = SMISPAR_STRING;
			par[0].value.s = tokenv[2];
			par[0].value.s = smis_translate_string_par (&par[0]);
		}
		num_pars = 1;
	}
	else if (tokenv[0][0] == '_') {
		/* Deal with a tiresome series of special parameters */
		if (num_tokens != 2) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		par[0].type = SMISPAR_DOUBLE;
		if (sscanf (tokenv[1], "%lf", &par[0].value.d) != 1) {
			errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
			cleanup (RC_BADPARAMLINE);
		}
		if      (!strcmp (tokenv[0], "_ObserveTransmitGain"))
			par[0].name = "obstgain";
		else if (!strcmp (tokenv[0], "_DecoupleTransmitGain"))
			par[0].name = "dectgain";
		else if (!strcmp (tokenv[0], "_ObserveReceiverGain"))
			par[0].name = "obsrgain";
		else if (!strcmp (tokenv[0], "_ObserveReceiverAttenuation"))
			par[0].name = "obsrattn";
		else if (!strcmp (tokenv[0], "_ObserveModulatorOffset"))
			par[0].name = "obsmoff";
		else if (!strcmp (tokenv[0], "_ObserveModulatorFineOffset"))
			par[0].name = "obsmfoff";
		else if (!strcmp (tokenv[0], "_DecoupleModulatorOffset"))
			par[0].name = "decmoff";
		else if (!strcmp (tokenv[0], "_DecoupleModulatorFineOffset"))
			par[0].name = "decmfoff";
		else {		/* take our chances */
			strncpy (par[0].name, &tokenv[0][1], 8);
			(par[0].name)[7] = '\0';
		}
		num_pars = 1;
	}
	else {
		errmsg (RC_BADPARAMLINE, __FILE__, __LINE__, (long) tokenv[0]);
		cleanup (RC_BADPARAMLINE);
	}
	
	return (num_pars);
}

/* A SMIS parameter array starts on a line with VAR_ARRAY as token 0, the
   name of the arrayed variable in token 1, the number of elements in token
   2.  (Note:  the number of elements in the VAR_ARRAY frequently exceeds the
   number of traces actually acquired--the latter value obtained from the
   EXPERIMENT_ARRAY or the nsarray field of the MRD header; the remaining
   elements were not used for this acquisition.)  In the arrays I have seen,
   token 3 is the first value.  The remaining values span additional lines
   following this, with normal ", " token separators between them as well as
   preceding the first token on every line.  To read the values, we take what
   we can from a line, then read a new line into the same buffer, tokenize
   it, and repeat.  We require that the first character of continuation lines
   not be ':' (signaling a new parameter--the premature end of the array).
   If the last line of values contains too many tokens, we warn of that and
   continue.  We do not try any more lines after we get the right number of
   values, but if the next line does turn out to be more array values (i.e.
   starts with ", "), it will later be caught and rejected by
   smis_make_tokenvec().
   The smispar storage format for arrays starts with a dummy "intro"
   parameter containing the array name and dimension, followed by the values,
   all in one contiguous array of smispar structs.  The "intro" smispar has
   the arrayed parameter's name in smispar.name, the dimension in
   smispar.value.l, and SMISPAR_ARRAYELTS in smispar.type.  The "value"
   smispars have the same name in smispar.name (although this is not checked)
   and the value and type as usual.  The elements do NOT have to be the same
   type, although a mixed array may be illegal to VNMR.
   This function returns the number of smispars filled, including the intro
   smispar; normally this will be storeelts+1 (NOT arrayelts+1, which may be
   larger).
*/
int smis_fill_array (par, buffer, num_tokens, tokenv, arrayelts, storeelts)
struct smispar * par;	/* array of at least storeelts+1 smispars */
char * buffer;		/* pretokenized buffer, which we may overwrite */
int num_tokens;		/* tokens in pretokenized buffer */
char ** tokenv;		/* initially buffer's tokens; size TOKENVSIZE */
int arrayelts;		/* number of values expected */
int storeelts;		/* number of values to store (read but ignore the rest) */
{
	static char savedname[12];	/* parameter name */
	int cur_token;	/* index of next token to eat (0...num_tokens-1) */
	int cur_value;	/* index of next smispar to fill (1...arrayelts) */

	/* must remember param name in static memory (for caller's use) since
	   we'll trash this buffer */
	strncpy (savedname, tokenv[1], 12);
	savedname[11] = '\0';

	/* fill the "intro" smispar */
	par[0].name = savedname;
	par[0].value.l = storeelts;
	par[0].type = SMISPAR_ARRAYELTS;
	cur_value = 1;		/* since we just filled #0 */
	cur_token = 3;		/* #2 was arrayelts; caller handled that */
	while (cur_value <= arrayelts) {
		if (cur_token < num_tokens) {		/* eat another token */
			if (cur_value <= storeelts) {	/* enough values stored yet? */
				par[cur_value].name = par[0].name;
				if (sscanf (tokenv[cur_token], "%lf", &par[cur_value].value.d) == 1) {
					par[cur_value].type = SMISPAR_DOUBLE;
					par[cur_value].value.d = smis_translate_float_par (&par[cur_value]);
				}
				else {				/* ack, it's a string */
					par[cur_value].type = SMISPAR_STRING;
					par[cur_value].value.s = tokenv[cur_token];
					par[cur_value].value.s = smis_translate_string_par (&par[cur_value]);
				}
			}
			++cur_value;
			++cur_token;
		}
		else {					/* read another line */
			if (fgets (buffer, BUFFERSIZE, fid_in) == NULL) {
				fprintf (stderr, "unexpected end of file while reading array %s (%s:%d)\n", par[0].name, __FILE__, __LINE__);
				cleanup (RC_READERR);
			}
			if (buffer[0] == ':') {
				fprintf (stderr, "premature end of array while reading array %s (%s:%d)\n", par[0].name, __FILE__, __LINE__);
				cleanup (RC_BADPARAMLINE);
			}
			/* but now fake a ':' as 1st char so
			   smis_make_tokenvec() won't bounce it */
			buffer[0] = ':';
			num_tokens = smis_make_tokenvec (buffer, tokenv);
			cur_token = 0;
			/* now go around again */
		}
	}

	/* Only way out of loop is to get enough values.  But were there too
	   many tokens? */
	if (cur_token < num_tokens)
		fprintf (stderr, "warning:  too many elements in array %s;\nignoring extras and continuing anyway (%s:%d)...\n", par[0].name, __FILE__, __LINE__);

	return (cur_value + 1);		/* number of smispars filled */
}

/* Do various conversions of "conventional" SMIS floating point parameters.
   Supply a smispar with name and SMIS value filled in; returns a "converted"
   value, which the caller *may* choose to install in the smispar.
   SMIS parameters as stored in the file are integers that reflect the actual
   values that are provided to PPL library functions.  Depending on the
   function and the whim of the programmer they may represent any of at least
   4 different time units (sec, msec, usec, or "PPL units" of 0.1 usec), or
   plain integers with various bounds (e.g. attenuator settings, 0-2047;
   number of transients, 0-32767).  Here we translate at least the time units
   into those used by the most comparable VNMR parameters, which come in only
   two kinds, generally usec for pulses and sec for delays.
*/
double smis_translate_float_par (par)
struct smispar * par;
{
	double value;

	if (!strcmp (par->name, "p90"))			/* 0.1 usec -> usec */
		value = par->value.d * 0.1;
	else if (!strcmp (par->name, "p180"))		/* 0.1 usec -> usec */
		value = par->value.d * 0.1;
	else if (!strcmp (par->name, "tr"))		/* msec -> sec */
		value = par->value.d * 0.001;
	else if (!strcmp (par->name, "t1array"))	/* msec -> sec */
		value = par->value.d * 0.001;
	else if (!strcmp (par->name, "trdd"))		/* usec -> usec */
		value = par->value.d;
	else if (!strcmp (par->name, "rdd"))		/* usec -> usec */
		value = par->value.d;
	else
		value = par->value.d;

	return (value);
}

/* Do any necessary transformations of string parameters.  Only one is
   (reluctantly) done now; this is because VNMR sread truncates all incoming
   string parameters after 15 non-nulls.  Some parameters which we know to be
   filenames can be longer than 15 characters, so to preserve their most
   useful part we strip them to their basenames if they exceed 15 chars.
*/
char * smis_translate_string_par (par)
struct smispar * par;
{
	char * value = par->value.s;	/* default is don't change it */

	if (!strcmp (par->name, "pplfile")  ||
	    !strcmp (par->name, "dspfile")  ||
	    !strcmp (par->name, "smpfile")    ) {
	    	if (strlen (par->value.s) > 15)
			value = smis_basename (par->value.s);
	}

	return (value);
}

/* Returns the basename at the end of a filename-like string, where the
   basename is defined as the rightmost sequence of characters which do not
   include any of the characters '/', '\', or ':'.  The contents of the
   original string are not altered and the result is a pointer to one the
   chars within the original string.  If no '/', '\', or ':' are present, the
   original string (i.e. a pointer to its first char) is returned.
*/
char * smis_basename (fullname)
char * fullname;
{
	register int i;

	for (i = strlen (fullname) - 1; i >= 0; i--) {
		if (fullname[i] == '\\'  ||
		    fullname[i] == '/'   ||  
		    fullname[i] == ':'     )
			return (&fullname[i+1]);
	}
	return (fullname);
}

/* Writes an stext file from SMIS parameters.  Since a few params like sweep
   width must go in the essential parameters section at the top of the file,
   and we may not discover those values until we have read many other
   parameters, we have to write the bottom part into a temporary file, then
   when we've read all the parameters and presumably found the essential
   ones, wewrite the essential part to the real output file and copy the
   temporary one into it.  To write parameters, just send the address of an
   array of smispar structures and the number of filled structures in the
   array.  To cause the final output file to be written, call with (NULL,
   0).  Careful:  any calls after the NULL one will reopen the temporary file
   and toss new data into it.  Calling with NULL twice will append a second
   whole stext onto the first one.
*/
void smis_stext (par, num_pars)
struct smispar * par;
int num_pars;
{
	register int c;

	if (temp_out == NULL) {
		temp_out = fopen (SMIS_TEMPFILE, "w");
		if (temp_out == NULL) {
			errmsg (RC_NOTMPFILE, __FILE__, __LINE__, (long) SMIS_TEMPFILE);
			cleanup (RC_NOTMPFILE);
		}
	}

	if (par)
		write_smispar_stext (par, num_pars);
	else {					/* time to finish up */
		fclose (temp_out);
		temp_out = fopen (SMIS_TEMPFILE, "r");
		if (temp_out == NULL) {
			errmsg (RC_NOTMPFILE, __FILE__, __LINE__, (long) SMIS_TEMPFILE);
			cleanup (RC_NOTMPFILE);
		}
		stext_essential (&esspar);
		stext_text (((struct SMIS_header *) in_header)->text);
		stext_params_begin ();
		while ((c = fgetc (temp_out)) != EOF)
			fputc (c, params_out);
		stext_params_end ();
		fclose (temp_out);
		DELETE (SMIS_TEMPFILE);
	}
}

/* Write out a smispar in stext format. */
void write_smispar_stext (par, num_pars)
struct smispar * par;
int num_pars;
{
	char * qualifier;
	register int i;
	static int num_arrays = 0;	/* number seen so far */

	for (i = 0; i < num_pars; i++) {
		switch (par[i].type) {
		case SMISPAR_DOUBLE:
			fprintf (temp_out, "%-8s     =%lf\n", par[i].name, par[i].value.d);
			break;
		case SMISPAR_LONG:
			fprintf (temp_out, "%-8s     =%ld\n", par[i].name, par[i].value.l);
			break;
		case SMISPAR_STRING:
			if (*(par[i].value.s) == '\0')
				qualifier = STEXT_parqual_off;
			else
				qualifier = STEXT_parqual_on;
			fprintf (temp_out, "%-8s%s  = %s\n", par[i].name, qualifier, par[i].value.s);
			break;
		case SMISPAR_ARRAYELTS:
			++num_arrays;
			fprintf (temp_out, "%s\"array%d\"=\n", par[i].name, num_arrays);
			/* Succeeding elements of the current vector of
			   smispars will fill in the values automatically.  We
			   assume the caller has made sure there are the right
			   number of them. */
			break;
		}
	}
}
