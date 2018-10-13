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
static char Version_ID[] = "stext.c v2.50 STM 9/11/94";
/* revision history:
   2.21	added qualifiers to string parameters for correct parsing by sread
   2.31	"correctly fakes" ACQDAT; use generic on/off qualifiers for strings
   2.40	rationalized stext_params_x_y() a bit
   2.50	added stext_text_continued()
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vdata.h"

#include "convert.h"
#include "protos.h"
#include "globals.h"
#include "stext.h"

static long dima;

/* Start the stext file by writing the essential parameters section. */
void stext_essential(esp)
struct stext_essparms * esp;
{
	/* remember total number of fids */
	dima = esp->dim1 * esp->dim2 * esp->dim3;

	fprintf(params_out, "datatype= %d\n", esp->datatype);
	fprintf(params_out, "system  = %s\n", esp->system);
	fprintf(params_out, "dim1    = %d\n", esp->dim1);
	fprintf(params_out, "dim2    = %d\n", esp->dim2);
	fprintf(params_out, "dim3    = %d\n", esp->dim3);
	fprintf(params_out, "np      = %ld\n", esp->np);
	fprintf(params_out, "sw      = %f\n", esp->sw);
	fprintf(params_out, "sfrq    = %f\n", esp->sfrq);
	fprintf(params_out, "dfrq    = %f\n", esp->dfrq);
	fprintf(params_out, "bytes   = %d\n", esp->bytes);
	fprintf(params_out, "order   = %d\n", esp->order);
	fprintf(params_out, "acqtyp  = %d\n", esp->acqtyp);
	fprintf(params_out, "abstyp  = %d\n", esp->abstyp);
	if (esp->datatype == STEXT_datatype_c2fid  ||
		esp->datatype == STEXT_datatype_p2spc)
		fprintf(params_out, "sw1     = %f\n", esp->sw1);
}

/* Use this to write the first line of text. */
void stext_text(string)
char * string;
{
	fprintf(params_out, "#TEXT:\n%s\n", string);
}

/* Use this to write all subsequent lines of text. */
void stext_text_continued(string)
char * string;
{
	fprintf(params_out, "%s\n", string);
}

/* Use this right before starting to write any parameters (other than
   stext_essential).
*/
void stext_params_begin()
{
	fprintf(params_out, "#PARAMETERS:\n");
}

/* Given a pointer to an integer, write it as an integer */
void stext_params_long_long(name, value)
char * name;
void * value;
{
	fprintf(params_out, "%-8s     =%ld\n", name, * ((long *) value) );
}

/* Given a pointer to a float, round it off and write it as an integer */
void stext_params_long_float(name, value)
char * name;
void * value;
{
	fprintf(params_out, "%-8s     =%ld\n", name, (long) (* ((float *) value) + 0.5) );
}

/* Given a pointer to a float, write it in floating point */
void stext_params_double_float(name, value)
char * name;
void * value;
{
	fprintf(params_out, "%-8s     =%lf\n", name, (double) (* ((float *) value) ) );
}

/* Given a pointer to a double, write it in floating point */
void stext_params_double_double(name, value)
char * name;
void * value;
{
	fprintf(params_out, "%-8s     =%lf\n", name, * ((double *) value) );
}

/* Given a pointer to an array of chars, not necessarily null-terminated,
   write them up to but not including the first null, or a maximum of
   "length" chars if "length" is nonzero.  No more than 255 chars will be
   written in either case. */
void stext_params_chars(name, value, length)
char * name;
void * value;
int length;
{
	char buffer[256];
	char * qualifier = STEXT_parqual_on;

	if (length == 0  ||  length > 255)
		length = 255;
	strncpy(buffer, (char *) value, length);
	buffer[length] = '\0';
	if (buffer[0] == '\0')
		qualifier = STEXT_parqual_off;
	else
		qualifier = STEXT_parqual_on;
	fprintf(params_out, "%-8s%s  = %s\n", name, qualifier, buffer);
}

/* Write the ACQDAT section, which is a series of lines of the form
	nt = scale
   one for each fid in the file.  This indicates how many transients are in
   each fid and how many times it has been right-shifted during the
   sum-to-memory process; together they are used to individually scale each
   fid during processing, to allow you to compare fids in an absolute
   intensity mode even if they have different numbers of transients in them.
   This info is not normally available to us on a fid-by-fid basis within a
   file (perhaps only VNMR actually tracks them individually), and it's not
   worth the trouble of transferring the real values for the file as a whole
   into this function since every line will end up being the same anyway.  So
   we harmlessly fake ACQDAT by using nt=1, scale=0 for every fid.  We use
   the value of dima calculated by stext_essential() as the number of fids.
*/
void stext_params_end()
{
	register int i;

	fprintf(params_out, "#ACQDAT:\n");
	for (i = 0; i < dima; i++)
		fprintf(params_out, " 1 = 0\n");
}
