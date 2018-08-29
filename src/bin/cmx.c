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
/* static char Version_ID[] = "cmx.c v2.75 STM 7/13/98"; */
/* revision history:
   2.1	rewrote cmx_parse() to make it quicker and smarter (doesn't choke
	on "1puls" etc.); added precision to "u" and "n" unit printout.
   2.11	minor change to cmx.h to suit Sun compiler
   2.2	cmx_pb() detects float data in stat parameter; cmx_file() alters and
	returns "type" accordingly
   2.30 adapt to Varian data.h v6.12 data structure elements, cosmetic (2/93)
   2.31	change to generic on/off qualifiers for strings
   2.42	handles the few differences between Intel-based (spec) and Sun-based
	(CMXW) data, based on the flag bit in the "type" variable.
   2.60	added Spinsight (CMSS) conversion
   2.71	use Spinsight current_sizeN params to detect data size
   2.72 better handling of Spinsight array vs. 2-D question
   2.73	observe Spinsight use_array value
   2.74	Y2K handling of Spinsight dates
   2.75	bugfix of Y2K handling of Spinsight dates
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "vdata.h"

#include "convert.h"
#include "protos.h"
#include "globals.h"
#include "cmx.h"
#include "stext.h"			/* stext file definition */

static struct stext_essparms esspar = {
	STEXT_datatype_c1fid,		/* datatype */
	1, 1, 1,			/* dim1, dim2, dim3 */
	4,				/* bytes */
	STEXT_order_mcsparc,		/* order (will be reversed) */
	STEXT_acqtyp_simul,		/* acqtyp */
	STEXT_abstyp_absval,		/* abstyp */
	CMX_PARNAME,			/* system */
	0,				/* np */
	0, 0,				/* sw, sw1 */
	0, 0				/* sfrq, dfrq */
};
static struct cmxarray array_head = {
	NULL,				/* next */
	"",				/* name */
	0,				/* num_values */
	0,				/* dim */
	CMXPAR_TYPE_NULL,		/* type */
	NULL				/* values */
};
/* static int is_2D = FALSE; */	/* reset to TRUE if 2-D, not 1-D/array */
static int arraydim = 1;	/* counts [] arrays found in acq or pb */
static int save_current_size2 = 0; /* hang onto current_size2 (CMSS), al2, */
static int save_al2 = 0;	/* and dw2 to help decide about 2-D */
static int dw2_present = FALSE;
static int use_array = TRUE;	/* if FALSE, only writes 1st value of arrays */
static int channel1 = 1;
static int channel2 = 2;
static int channel3 = 3;
int hyper = 0;
static double acq_time = 0.0;

/* prototypes of local functions */
#ifdef __STDC__

static long cmx_pb(long type, FILE * file);
static void cmx_parse(struct cmxpar * par, char * buffer);
static void cmx_init_array(char * fullname, char * parvalue);
static struct cmxarray * cmx_get_array(char * parname, int create);
static void cmx_record_array_val(struct cmxpar * par, int index);
static void cmx_write_arrays(long type);
static void cmx_destroy_arrays(void);
static void apply_units(struct cmxpar * par);
static void cmx_stext(long type, struct cmxpar * par);
static void cmx_array_stext(long type, int count_arrays);
static void check_for_essparm(long type,struct cmxpar * par,
			      struct stext_essparms * esp, char * commentbuf);
static void write_any_parm(long type, struct cmxpar * par);
static void write_stext_par(char * name, struct cmxpar * par, int truetype);
static int cmx_is_2D(long type);
static char * date_numeric_to_english(char * numdate);
static char * freqName(int index);

#else /* __STDC__ */

static long cmx_pb();
static void cmx_parse();
static void cmx_init_array();
static struct cmxarray * cmx_get_array();
static void cmx_record_array_val();
static void cmx_write_arrays();
static void cmx_destroy_arrays();
static void apply_units();
static void cmx_stext();
static void cmx_array_stext();
static void check_for_essparm();
static void write_any_parm();
static void write_stext_par();
static int cmx_is_2D();
static char * date_numeric_to_english();
static char * freqName();

#endif /* __STDC__ */

/* CMX doesn't really have a "header"; its "files" are directories like
   Varian's.  For a basic 1-D file, the parameters are in the file "pb" and
   the data is in "d", both under the directory by which the whole collection
   is referred.  2-D datasets have a few parameters defining the nature of
   the 2nd dimension in the file "p2".  Arrayed datasets have a "pe" file
   which documents each change of parameters from array element to array
   element in rather repetitive fashion.  "pe" is also generated for 2-D
   datasets, which it is technically correct but superfluous since this is
   handled much more compactly by "p2".  Currently the "p2" file if present,
   is opened as params2_in; if this file pointer is non-NULL, the dataset
   must be 2-D.  The "pe" file is ignored for now.
   Since the "stat" parameter in the pb file may decide how the fid is to be
   interpreted (float vs. integer etc.), we are passed the variable "type",
   modify it as necessary, and return the modified version.  The calling
   function must catch this change and act appropriately.
*/
/* Changes for Spinsight: fid data "d" is now named "data".  Parameters that
   used to be all in "pb" are now split into "acq", for original acquisition
   parameters, "acq_2", for acquisition parameters that change during 1-D
   acquisition (mainly "ac", the scan count), "proc", for latest processing
   parameters, and "proc_setup", for original processing parameters.  The
   "stat" parameter has disappeared and all fid data is 32-bit integer.
   Arrayed datasets have a file "apnd" which gives a history similar to the
   old "pe" and is similarly ignored here.
*/
long cmx_file(type)
    long type;
{
    struct cmxpar param;
    
    in_fhead.nblocks = 1;
    in_fhead.ntraces = 1;
    in_fhead.ebytes = 4;
    in_fhead.vers_id = 0;
    in_fhead.status = S_DATA | S_COMPLEX;
    in_fhead.nbheaders = 1;
    out_fhead.nblocks = 1;
    out_fhead.ntraces = 1;
    out_fhead.ebytes = 4;
    out_fhead.vers_id = 0;
    out_fhead.status = S_DATA | S_32 | S_COMPLEX;
    out_fhead.nbheaders = 1;
    
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
    
    if (type & IN_CMSS) {
	open_params_in(&params_in, "acq", TRUE);
	type = cmx_pb(type, params_in);
	open_params_in(&params_in, "acq_2", FALSE);
	if (params_in)
	    type = cmx_pb(type, params_in);
	open_params_in(&params_in, "proc", FALSE);
	if (params_in)
	    type = cmx_pb(type, params_in);
    }
    else {
	open_params_in(&params_in, "pb", TRUE);
	type = cmx_pb(type, params_in);
	open_params_in(&params_in, "p2", FALSE);
	if (params_in)
	    type = cmx_pb(type, params_in);
    }
    cmx_write_arrays(type);
    if (esspar.np / 2 == (int) (acq_time * esspar.sw) * 2) {
       param.name = "phase";
       param.type = CMXPAR_TYPE_FLOAT;
       param.value.f = 10.0;
       write_stext_par("phase", &param, CMXPAR_TYPE_INT);
       hyper = 1;
       esspar.np /= 2;
    }
    
    /* write np, which was deferred until the end */
    param.name = "np";
    param.type = CMXPAR_TYPE_FLOAT;
    param.value.f = (double) esspar.np;
    write_stext_par("np", &param, CMXPAR_TYPE_INT);
    
    /* Decide whether the data is 1-D/array or 2-D and write out ni
       appropriately.  If 2-D, the number of fids is taken directly from
       current_size2 (CMSS) or al2 (CMX/CMXW).  If 1-D/array, it comes
       from arraydim.  It is unnecessary to write arraydim itself since
       VNMR sread calculates it on the fly.  Note that it is impossible to
       have an arrayed 2-D in this scheme.  There is also no provision for
       factors-of-two caused by hypercomplex 2-D etc.
       */
    if (cmx_is_2D(type)) {
	param.name = "ni";
	if (type & IN_CMSS)
	    param.value.h = save_current_size2;
	else
	    param.value.h = save_al2;
	param.type = CMXPAR_TYPE_HEX;
	param.unit = '\0';
	cmx_stext(type, &param);
	esspar.dim1 = param.value.h;
	in_fhead.ntraces = esspar.dim1;
    }
    else {
        struct stat buf;
        int size;

	param.name = "ni";
	param.value.h = 0;
	param.type = CMXPAR_TYPE_HEX;
	param.unit = '\0';
	cmx_stext(type, &param);
        fstat(fileno(fid_in), &buf);
        size = in_fhead.np * in_fhead.ebytes;
        /* If arraydim does not match actual data size, reset it */
        if ((size*arraydim != (int) buf.st_size) && (size == (int) buf.st_size) )
        {
           arraydim = 1;
        }
	esspar.dim1 = arraydim;
	in_fhead.ntraces = arraydim;
    }

    /* complex points -> total points */
    in_fhead.tbytes = in_fhead.np * in_fhead.ebytes;
    in_fhead.bbytes = in_fhead.ntraces*in_fhead.tbytes;
    out_fhead.np = in_fhead.np;
    if (hyper)
    {
       out_fhead.np /= 2;
       out_fhead.nblocks *= 2;
       esspar.dim1 *= 2;
    }
    out_fhead.tbytes = out_fhead.np * out_fhead.ebytes;
    out_fhead.bbytes = out_fhead.ntraces*out_fhead.tbytes + sizeof(struct datablockhead);

    in_bhead.scale = 0;
    out_bhead.scale = in_bhead.scale;
    out_bhead.ctcount = in_bhead.ctcount;
    
    if (type & OUT_STEXT)
	cmx_stext(type, NULL);		/* finishes stext file */

    return(type);
}

/* Interprets a CMX parameter file line by line.  Calls the appropriate
   function to write each parameter out as it is read.  CMSS arrays, which
   span several lines, are accumulated in a special list and written out all
   at once after the rest of the file is processed.  Checks for the parameter
   "stat" and if found, checks it to see if data is integer or floating point;
   in the latter case, the variable "type" is altered to reflect this.  "type"
   (possibly altered) is returned to the caller.  A consequence of this design
   is that the parameter "stat" cannot be successfully arrayed; however,
   "stat" does not seem to be present in CMSS data, and it is questionable why
   one would want to array it anyway.
*/
static long cmx_pb(type, file)
long type;
FILE * file;
{
	char buffer[BUFFERSIZE];
	struct cmxpar param;

	for (;;) {
		if (fgets(buffer, BUFFERSIZE, file) == NULL)
			break;
		cmx_parse(&param, buffer);
		if (param.type != CMXPAR_TYPE_NULL  &&
		    param.type != CMXPAR_TYPE_ARRAY) {	/* do arrays later */
			if (type & OUT_STEXT)
				cmx_stext(type, &param);
		}
	}
	return(type);
}

char *freqName(index)
int index;
{
   if (channel1 == index)
      return("sfrq");
   if (channel2 == index)
      return("dfrq");
   if (channel3 == index)
      return("dfrq2");
   return("dfrq3");
}
/* Parse a line from a CMX parameter file.  Recognizes the parameter name, 
   value, and units, if any.  Knows about floating point, string, and hex
   variables; integers are handled as floating point.  Parameters are stored
   in a cmxpar structure, which contains name, type, value, and units.  If
   the line cannot be interpreted, the cmxpar.type will be 0.
*/
static void cmx_parse(par, buffer)
struct cmxpar * par;
char * buffer;
{
	int i, words;
	char * valuestart;	/* string pointer for value of parameter */
	int is_array = 0;
	char * indexstart = NULL;	/* string pointer for value of array index */
	int index;
	static char param_name[BUFFERSIZE];
	static char param_string[BUFFERSIZE];

	par->type = CMXPAR_TYPE_NULL;	/* signals parameter not valid yet */

	/* Find "=" sign and replace it with a null to split the string.  If
	 this is a value of an arrayed parameter, we detect that from the
	 presence of '[' and ']' characters.  We then capture the array index
	 also. */
	for (i = 0; i < BUFFERSIZE; i++) {
		/* If end-of-line is found before '=', ignore the line.  This
		   rejects blank lines as well as malformed lines that don't
		   look like parameters. */
		if (buffer[i] == '\n'  ||  buffer[i] == '\0')
			return;
		else if (buffer[i] == '[') {		/* array index */
			buffer[i] = '\0';		/* terminate name */
			indexstart = &buffer[i+1];	/* remember where */
			is_array = 1;			/* mark as array */
		}
		else if (buffer[i] == ']')
			buffer[i] = '\0';		/* terminate index */
		else if (buffer[i] == '=') {
			buffer[i] = '\0';	/* terminate name (usually) */
			valuestart = &buffer[i+1];	/* remember where */
			break;
		}
	}
	if (i >= BUFFERSIZE)	/* loop ended on count, couldn't find = */
		return;

	/* If it's an array initialization parameter, process it and return. */
	if (!strncmp(buffer, "array_", 6)) {
		cmx_init_array(buffer, valuestart);
		return;
	}

	/* Determine the array index, if any. */
	if (is_array) {
		if (sscanf(indexstart, "%d", &index) != 1) {
			/* In an attempt to recreate the original parameter
			   line for the error message, try to replace the '=',
			   '[', and ']' that were replaced with nulls
			   previously.  This may give a garbled result if
			   there was embedded whitespace. */
			buffer[strlen(buffer)] = '[';
			indexstart[strlen(indexstart)] = ']';
			indexstart[strlen(indexstart)+1] = '=';
			errmsg(RC_BADPARAMLINE, __FILE__, __LINE__, (long) buffer);
			cleanup(RC_BADPARAMLINE);
		}
	}

	/* Set up buffers for name and value. */
	strcpy(param_name, buffer);  /* copy name, since null-terminated now */
	par->name = param_name;
	*param_string = '\0';

	/* String at valuestart can be 5 things (from most to least common):
		a decimal number:  1.2 (decimal point optional)
		a decimal number with a unit:  1.2u (decimal point optional)
		a string:  fred
		a string that just looks like a number plus unit:  1puls
		a hex number:  0x12
	   We use a sscanf() call to read the string as a leading number
	   followed by a non-numeric part.  If only the number part succeeds,
	   it's just a decimal number.  If the numeric part is a single '0',
	   the non-numeric part is a single 'x', and the following char is a
	   valid hex digit, we reread as a hex number and break.  If the
	   non-numeric part is a single char which is one of an array of
	   recognized units (valid_units[]), we apply the units to the number
	   and break.  Otherwise we treat the whole thing as a string by
	   doing something clever but unfriendly:  we don't break but instead
	   drop through into the default case, which treats the whole thing
	   as a string.
	*/

	while (isspace(*valuestart))
		++valuestart;
	words = sscanf(valuestart, "%lf%s", &par->value.f, param_string);

	switch (words) {
	case 1:		/* decimal, no units */
		par->type = CMXPAR_TYPE_FLOAT;
		par->unit = '\0';
		break;
	case 2:		/* hex, or decimal with units, or funny string */
		if (par->value.f == 0  &&  *param_string == 'x') {    /* hex? */
			if (isxdigit(param_string[1])) {	  /* it's hex */
				words = sscanf(&param_string[1], "%lx", &par->value.h);
				par->type = CMXPAR_TYPE_HEX;
				par->unit = '\0';
				break;
			}
		}
		else if (strlen(param_string) == 1) {	/* decimal plus units? */
			for (i = 0; i < NUM_VALID_UNITS; i++) {		 /* one char only */
				if (*param_string == valid_units[i]) {	 /* unit OK? */
					par->type = CMXPAR_TYPE_FLOAT;	/* yes, store unit */
					par->unit = *param_string;
					apply_units(par);  /* modify value to reflect unit */
					break;		    /* just leaves for() loop */
				}
			}
			if (i < NUM_VALID_UNITS)	/* loop broke, DIDN'T end on count */
				break;			/* leave switch() */
		}
		/* Any strings like "1puls" mistakenly caught in this case drop
		   through here into the default case.  Don't change the order of
		   case evaluation! */
	default:
		strcpy(param_string, valuestart);	/* copy value of parameter */
		if (*param_string != '\0') {		/* remove \n if last char of string */
			i = strlen(param_string) - 1;
			if (param_string[i] == '\n')
				param_string[i] = '\0';
		}
		par->value.s = param_string;
		par->type = CMXPAR_TYPE_STRING;
		par->unit = '\0';
		break;
	}

	/* If it's a value of an arrayed parameter, record it in the array
	   which has been preallocated for it.  Change the type field so our
	   caller knows the parameter needs special processing.  The type is
	   still available in the array itself.
	*/
	if (is_array) {
		cmx_record_array_val(par, index);
		par->type = CMXPAR_TYPE_ARRAY;	/* notes that we did this */
	}
}

/* Process the three initialization parameters which define an array:
   array_num_values_xxx, array_dim_xxx, and array_type_xxx.  Arguments
   provided are the full init-parameter name and the string which represents
   its value.  We first determine the parameter's real name from the end of
   fullname; then we locate the cmxarray structure for the parameter, creating
   a new one if it doesn't exist yet.  Then we fill in the given value.
   Filling in the num_values field also causes allocation of memory to hold
   the parameter's values (as an array of cmxpars).  This step must occur
   before any of the parameter values are encountered.  The other
   initialization steps might possibly occur after the first values are
   encountered; however in practice this has not been seen yet.
*/
static void cmx_init_array(fullname, parvalue)
char * fullname;
char * parvalue;
{
	char * parname;
	struct cmxarray * array;
	int intvalue;

	/* Convert the value string into an integer. */
	if (sscanf(parvalue, "%d", &intvalue) != 1) {
		/* In an attempt to recreate the original parameter line for
		   the error message, try to replace the '=' that was replaced
		   with a null at the end of fullname. */
		fullname[strlen(fullname)] = '=';
		errmsg(RC_BADPARAMLINE, __FILE__, __LINE__, (long) fullname);
		cleanup(RC_BADPARAMLINE);
	}

	/* Process the parameter. */
	if (!strncmp(fullname, "array_num_values_", 17)) {
		parname = &fullname[17];
		array = cmx_get_array(parname, TRUE);
		if (array->num_values != 0) {
			errmsg(RC_BADARRAY, __FILE__, __LINE__,
			       (long) parname);
			cleanup(RC_BADARRAY);
		}
		array->num_values = intvalue;
		array->values = (struct cmxpar *) calloc(array->num_values,
							  sizeof(struct cmxpar));
		if (array->values == NULL) {
			errmsg(RC_NOMEMORY, __FILE__, __LINE__,
			       array->num_values * sizeof(struct cmxpar));
			cleanup(RC_NOMEMORY);
		}
	}
	else if (!strncmp(fullname, "array_dim_", 10)) {
		parname = &fullname[10];
		array = cmx_get_array(parname, TRUE);
		array->dim = intvalue;
	}
	else if (!strncmp(fullname, "array_type_", 11)) {
		parname = &fullname[11];
		array = cmx_get_array(parname, TRUE);
		array->type = intvalue;
	}
}

/* Find the cmxarray structure for the given parameter name.  If there is no
   cmxarray for it yet, a new one is optionally created, linked into the
   cmxaray list, and returned.  Otherwise NULL is returned.
*/
static struct cmxarray * cmx_get_array(parname, create)
char * parname;
int create;
{
	struct cmxarray * array;
	struct cmxarray * ptr;

	/* Find the array with the given name. */
	for (array = array_head.next; array; array = array->next)
		if (!strcmp(array->name, parname))
			return(array);

	/* If we got to this point, an array with the specified name was not
	   found.  If "create" is true, we must allocate a new array. */
	if (!create)
		return(NULL);
	array = (struct cmxarray *) calloc(1, sizeof(struct cmxarray));
	if (array == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, sizeof(struct cmxarray));
		cleanup(RC_NOMEMORY);
	}
	array->name = malloc(strlen(parname)+1);
	if (array->name == NULL) {
		errmsg(RC_NOMEMORY, __FILE__, __LINE__, strlen(parname)+1);
		cleanup(RC_NOMEMORY);
	}
	strcpy(array->name, parname);

	/* Link it into the list. */
	ptr = &array_head;
	while (ptr->next)
		ptr = ptr->next;
	ptr->next = array;
	array->next = NULL;

	return(array);
}

/* Copy the values from a cmxpar structure into an array element.  The array
   name is taken from the cmxpar structure.
*/
static void cmx_record_array_val(par, index)
struct cmxpar * par;
int index;
{
	struct cmxarray * array;

	/* Find the array with the given name. */
	if ((array = cmx_get_array(par->name, FALSE)) == NULL) {
		errmsg(RC_BADARRAY, __FILE__, __LINE__, (long) par->name);
		cleanup(RC_BADARRAY);
	}

	/* Make sure it's already initialized. */
	if (index >= array->num_values  ||  array->num_values == 0) {
		errmsg(RC_BADARRAY, __FILE__, __LINE__, (long) par->name);
		cleanup(RC_BADARRAY);
	}

	/* Copy the info from the given cmxpar structure into the cmxpar
	   already allocated as part of the array.  The "name" field is
	   pointed at the name in the cmxarray structure since the one passed
	   to us will become invalid.
	*/
	array->values[index].name = array->name;
	array->values[index].value.h = par->value.h;
	array->values[index].type = par->type;
	array->values[index].unit = par->unit;
}

/* Write arrayed variables to the output file, then release their memory.
   Only implemented for OUT_STEXT; other output types will give an error if
   any arrays are present.
*/
static void cmx_write_arrays(long type)
{
    if (array_head.next) {
	if (type & OUT_STEXT) {
	    cmx_array_stext(type, TRUE);
	}
	else {
	    errmsg(RC_ONOTYET, __FILE__, __LINE__, 0);
	    cleanup(RC_ONOTYET);
	}
	cmx_destroy_arrays();		/* release array memory and list */
    }
}

/* Free all memory associated with the list of arrayed variables and restore
   the array_head so it's ready to use with any arrays we encounter in future
   parameter files.
*/
static void cmx_destroy_arrays()
{
	struct cmxarray * array;
	struct cmxarray * nextarray;

	array = array_head.next;
	while (array) {
		nextarray = array->next;
		free(array->name);
		free(array->values);
		free(array);
		array = nextarray;
	}
	array_head.next = NULL;
}

/* Multiply floating point parameter by a scale factor to convert from
   modified units (msec, usec) to base units (sec).  Knows about the units
   s (*1), m (/1000), u (/1e6), n (/1e9), and k (*1024).  Supply a cmxpar
   structure pointer.  The structure's value field will be modified, but the
   units will be left unchanged.
*/
static void apply_units(par)
struct cmxpar * par;
{
	switch (par->unit) {
	case 's':
		break;
	case 'm':
		par->value.f *= 1.0e-3;
		break;
	case 'u':
		par->value.f *= 1.0e-6;
		break;
	case 'n':
		par->value.f *= 1.0e-9;
		break;
	case 'k':
		par->value.f *= 1024.0;
		break;
	}
}

/* Writes an stext file from CMX parameters.  Since a few params like sweep
   width must go in the essential parameters section at the top of the file,
   we have to write the bottom part into a temporary file, watching for
   essential params as we go, then write the essential part to the real
   output file and copy the temporary one into it.  To write a parameter,
   just send the address of a cmxpar structure.  To cause the final output
   file to be written, call with NULL.  Careful:  any calls after the NULL
   one will reopen the temporary file and toss new data into it.  Calling
   with NULL twice will append a second whole stext onto the first one.
*/
static void cmx_stext(type, par)
long type;
struct cmxpar * par;
{
	int c;
	static char commentbuf[BUFFERSIZE];

	if (temp_out == NULL) {
		temp_out = fopen(CMX_TEMPFILE, "w");
		if (temp_out == NULL) {
			errmsg(RC_NOTMPFILE, __FILE__, __LINE__, (long) CMX_TEMPFILE);
			cleanup(RC_NOTMPFILE);
		}
	}

	if (par) {
		check_for_essparm(type, par, &esspar, commentbuf);
		write_any_parm(type, par);
	}
	else {					/* time to finish up */
		fclose(temp_out);
		temp_out = fopen(CMX_TEMPFILE, "r");
		if (temp_out == NULL) {
			errmsg(RC_NOTMPFILE, __FILE__, __LINE__, (long) CMX_TEMPFILE);
			cleanup(RC_NOTMPFILE);
		}
		stext_essential(&esspar);
		stext_text(commentbuf);
		stext_params_begin();
		while ((c = fgetc(temp_out)) != EOF)
			fputc(c, params_out);
		stext_params_end();
		fclose(temp_out);
                temp_out = NULL;
		DELETE(CMX_TEMPFILE);
	}
}

/* Write out all the arrays in stext format.  If count_arrays is true, also
   store the total array dimension in the global arraydim.  CMSS co-arrays
   (like VNMR array='(x,y)') are handled correctly in the array dimension
   calculation, but sread will ignore one of the co-arrayed parameters (in
   effect, treating it like an arrayed processing parameter).  This occurs no
   matter how the "arrayn" keywords are set in the stext file.
   The global variable use_array contains the value of the Spinsight acq_2
   parameter use_array (default TRUE for non-Spinsight).  If it is FALSE, we
   write out a single non-arrayed parameter set using just the first value of
   each arrayed variable, and leave 1 in arraydim.
*/
static void cmx_array_stext(type, count_arrays)
long type;
int count_arrays;
{
	struct cmxarray * array;
	int i;
	int array_sizes[CMX_MAX_ARRAYS];
	int local_arraydim = 1;

	/* Set the sizes of all possible dimensions to 0. */
	for (i = 0; i < CMX_MAX_ARRAYS; i++)
	    array_sizes[i] = 0;

	/* Walk through the list of arrays, recording their sizes and writing
	   them to the stext file. */
	for (array = array_head.next; array; array = array->next) {
	    if (use_array) {
		if (array_sizes[array->dim] == 0) {	/* new dim */
		    array_sizes[array->dim] = array->num_values;
		    local_arraydim *= array->num_values;
		}
		else {					/* old dim */
		    if (array->num_values != array_sizes[array->dim]) {
			errmsg(RC_BADARRAY, __FILE__, __LINE__,
			       (long) array->name);
			cleanup(RC_BADARRAY);
		    }
		}
		fprintf (temp_out, "%s\"array%d\"=\n",
			 array->name, array->dim - 1);
		for (i = 0; i < array->num_values; i++)
		    cmx_stext(type, &array->values[i]);
	    }
	    else {
		    cmx_stext(type, &array->values[0]);
	    }
	}
	
	if (count_arrays)
	    arraydim = local_arraydim;
}

/* Examine a parameter (a pointer to a cmxpar structure) to see if it is one
   of the essential params needed at the top of the stext file.  If so, puts
   it in the essparms structure.  Also recognizes the file comment and saves
   it in a comment buffer.  The np, ntraces, scale, and ctcount variables of
   the datafilehead structure are recognized and placed in in_fhead (NOT in
   out_fhead, nor are in_fhead.tbytes and .ebytes changed).
*/
static void check_for_essparm(type, par, esp, commentbuf)
long type;
struct cmxpar * par;
struct stext_essparms * esp;
char * commentbuf;
{
	/* The original number of complex points at acquisition time is in
	   "al" (found in the pb file in CMX/CMXW, or acq in Spinsight).  If
	   the data was zero-filled after acquisition, then saved, "al" will
	   remain the same but the actual data will be larger.  The parameter
	   "current_size1" (only in Spinsight, in the proc file) gives the
	   actual size taking into account any zero-filling.  Since we scan
	   proc after acq, the safe way to handle this is to accept both, so
	   that the last one wins.  */
	if (!strcmp(par->name, "al")) {
		esp->np = par->value.f * 2 + 0.5;
		in_fhead.np = esp->np;
	}
	else if (!strcmp(par->name, "current_size1")) {
		esp->np = par->value.f * 2 + 0.5;
   		in_fhead.np = esp->np;
	}
	else if (!strcmp(par->name, "al2")) {
		/* al2 seems to be the CMX/CMXW equivalent of VNMR "ni".
		   In VNMR, "ni" is only used for 2-D and is otherwise 0 or 1.
		   In CMSS, it seems to do the same in 2-D.  But in 1-D, it
		   seems to take on random small integer values greater than
		   or equal to the total array dimension (VNMR arraydim).  So
		   a simple non-arrayed 1-D experiment can have al2=4, for
		   example.  For now, we just remember what al2 was and decide
		   what to do with it later, after we know more about arrays
		   and 2-D.  We do the same for current_size2 (CMSS), which
		   always gives the correct number of fids. */
		save_al2 = (int) (par->value.f + 0.5);
	}
	else if (!strcmp(par->name, "current_size2")) {
		save_current_size2 = (int) (par->value.f + 0.5);
	}
	else if (!strcmp(par->name, "ac"))
		in_bhead.ctcount = par->value.f + 0.5;
	else if (!strcmp(par->name, "dw"))
		esp->sw = 1.0 / par->value.f;
	else if (!strcmp(par->name, "dw2")) {
		if (par->value.f > 0) {
			esp->sw1 = 1.0 / par->value.f;
			if (esp->datatype == STEXT_datatype_c1fid)
				esp->datatype = STEXT_datatype_c2fid;
			else if (esp->datatype == STEXT_datatype_p1spc)
				esp->datatype = STEXT_datatype_p2spc;
		}
	}
	else if (!strcmp(par->name, "sf")) {
		if (type & IN_CMX)
			esp->sfrq = par->value.f;
	}
	else if (!strcmp(par->name, "sf1")) {
		if (type & IN_CMX)
			esp->dfrq = par->value.f;
		else if (type & (IN_CMXW | IN_CMSS))
			esp->sfrq = par->value.f;
	}
	else if (!strcmp(par->name, "sf2")) {
		if (type & (IN_CMXW | IN_CMSS))
			esp->dfrq = par->value.f;
	}
	else if (!strcmp(par->name, "aqtm")) {
		acq_time = par->value.f;
	}
	else if (!strcmp(par->name, "ch1")) {
		channel1 = (int) (par->value.f+0.1);
	}
	else if (!strcmp(par->name, "ch2")) {
		channel2 = (int) (par->value.f+0.1);
	}
	else if (!strcmp(par->name, "ch3")) {
		channel3 = (int) (par->value.f+0.1);
	}
	else if (!strcmp(par->name, "com"))
		strcpy(commentbuf, par->value.s);
	/* CMSS can store transformed spectra as well as fids.  VNMR sread
	   will probably fail if given a transformed spectrum.  We ignore
	   domain2 as we hope it will track domain1.  This won't be true for
	   half-transformed 2-D data but sread can't read that anyway. */
	else if (!strcmp(par->name, "domain1")) {
		if (par->value.f != 0.0) {
			if (esp->datatype == STEXT_datatype_c1fid)
				esp->datatype = STEXT_datatype_p1spc;
			else if (esp->datatype == STEXT_datatype_c2fid)
				esp->datatype = STEXT_datatype_p2spc;
		}
	}
}

/* Write out a cmxpar under any name.  If nonzero, truetype overrides type
   stored in cmxpar structure.  A truetype value of 'i' causes the value
   (assumed double) to be rounded off to a long.
*/
static void write_stext_par(name, par, truetype)
char * name;
struct cmxpar * par;
int truetype;
{
	char * formatstring;
	char * qualifier;

	if (truetype == CMXPAR_TYPE_NULL)
		truetype = par->type;
	switch (truetype) {
	case CMXPAR_TYPE_FLOAT:
		if (par->unit == 'u')
			formatstring = "%-8s     =%.9lf\n";
		else if (par->unit == 'n')
			formatstring = "%-8s     =%.12lf\n";
		else
			formatstring = "%-8s     =%lf\n";
		fprintf(temp_out, formatstring, name, par->value.f);
		break;
	case CMXPAR_TYPE_HEX:
		fprintf(temp_out, "%-8s     =%ld\n", name, par->value.h);
		break;
	case CMXPAR_TYPE_INT:
		fprintf(temp_out, "%-8s     =%ld\n", name, (long) (par->value.f + 0.5));
		break;
	case CMXPAR_TYPE_STRING:
		if (*(par->value.s) == '\0')
			qualifier = STEXT_parqual_off;
		else
			qualifier = STEXT_parqual_on;
		fprintf(temp_out, "%-8s%s  = %s\n", name, qualifier, par->value.s);
		break;
	}
}

/* Translate any CMX params we know about into Varian terms, otherwise just
   write most of them out as is.  A few parameters conflict with Varian params
   of very different meaning, so they are excluded.  VNMR "sread" will ignore
   any stext parameters with names longer than 8 characters.  Note:  due to the
   way arrayed parameters are handled, do not write out two stext parameters
   for any CMX parameter is it's likely to be arrayed.
*/
static void write_any_parm(type, par)
    long type;
    struct cmxpar * par;
{
    static struct cmxpar tempfloat;	/* initialize to 0's, only use value */
    static struct cmxpar tempstr;	/* initialize to 0's, only use value */
    
    if (!strcmp(par->name, "na"))
	write_stext_par("nt", par, CMXPAR_TYPE_INT);
    else if (!strcmp(par->name, "ac"))
	write_stext_par("ct", par, CMXPAR_TYPE_INT);
    else if (!strcmp(par->name, "stat")) {
	if ( type & (IN_CMX | IN_CMXW)  &&
	     par->value.h & CMX_STAT_FLOAT )
	    type |= IEEE_FLOAT;
	write_stext_par("stat", par, CMXPAR_TYPE_HEX);
    }
    else if (!strcmp(par->name, "datatype")) {
	if ( type & IN_CMSS  &&
	     (int) (par->value.f + 0.5) == CMSS_DATATYPE_FLOAT )
	    type |= IEEE_FLOAT;
	write_stext_par("datatype", par, CMXPAR_TYPE_INT);
    }
    else if (!strcmp(par->name, "dl")) {
	tempfloat.value.f = par->value.f * 2;
	write_stext_par("fn", &tempfloat, CMXPAR_TYPE_INT);
    }
    else if (!strcmp(par->name, "dl2")) {
	tempfloat.value.f = par->value.f * 2;
	write_stext_par("fn1", &tempfloat, CMXPAR_TYPE_INT);
    }
    else if (!strcmp(par->name, "sf")) {
	if (type & IN_CMX)
	    write_stext_par("sfrq", par, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "sf1")) {
	if (type & IN_CMX)
	    write_stext_par("dfrq", par, CMXPAR_TYPE_FLOAT);
	else if (type & (IN_CMXW | IN_CMSS))
	    write_stext_par(freqName(1), par, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "sf2")) {
	if (type & (IN_CMXW | IN_CMSS))
	    write_stext_par(freqName(2), par, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "sf3")) {
	if (type & (IN_CMXW | IN_CMSS))
	    write_stext_par(freqName(3), par, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "dw")) {
	tempfloat.value.f = 1.0 / par->value.f;
	write_stext_par("sw", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "dw2")) {
	if (par->value.f > 0.0) {
	    tempfloat.value.f = 1.0 / par->value.f;
	    write_stext_par("sw1", &tempfloat, CMXPAR_TYPE_FLOAT);
	    dw2_present = TRUE;
	}
    }
    else if (!strcmp(par->name, "sw"))	/* in KHz, and duplicates dw */
	write_stext_par("swKHz", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "sw2"))
	write_stext_par("sw1KHz", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "aqtm"))
	write_stext_par("at", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "gain"))
	write_stext_par("gain", par, CMXPAR_TYPE_INT);  /* same but write as int */
    else if (!strcmp(par->name, "dp"))
	write_stext_par("ss", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "multiplier_for_1st_pt1"))
	write_stext_par("fpmult", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "multiplier_for_1st_pt2"))
	write_stext_par("fpmult1", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "ff")) {
	tempfloat.value.f = par->value.f * 1000.0;
	write_stext_par("fb", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "ds_num1")) {
	tempfloat.value.f = par->value.f * -1.0;
	write_stext_par("lsfid", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "pw")) {/* VNMR wants pulses in usec */
	tempfloat.value.f = par->value.f * 1.0e6;
	write_stext_par("pw", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "pw1")) {
	tempfloat.value.f = par->value.f * 1.0e6;
	write_stext_par("pw1", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "pw2")) {
	tempfloat.value.f = par->value.f * 1.0e6;
	write_stext_par("pw2", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "rd")) {/* do rd and ad like pulses */
	tempfloat.value.f = par->value.f * 1.0e6;
	write_stext_par("rd", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "ad")) {/* do rd and ad like pulses */
	tempfloat.value.f = par->value.f * 1.0e6;
	write_stext_par("ad", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "pred")) {
	tempfloat.value.f = par->value.f * 1.0e6;
	write_stext_par("pred", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    else if (!strcmp(par->name, "ct"))	/* collides with VNMR "ct" */
	write_stext_par("contact", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "dp"))	/* collides with VNMR "dp" */
	write_stext_par("cmx_dp", par, CMXPAR_TYPE_INT);
    else if (!strcmp(par->name, "ppfn")) {
	write_stext_par("seqfil", par, CMXPAR_TYPE_STRING);
	write_stext_par("pslabel", par, CMXPAR_TYPE_STRING);
    }
    else if (!strcmp(par->name, "em_lb1"))
	write_stext_par("lb", par, CMXPAR_TYPE_FLOAT);
    else if (!strcmp(par->name, "em_lb2"))
	write_stext_par("lb1", par, CMXPAR_TYPE_FLOAT);
    /* This section is incomplete.  We detect two of the many axis units
       CMSS uses (0=virtual, 1=ppm, 2=KHz, 4=points, 5=Hz) and set VNMR's
       axis variable from it.  We ignore any 2nd dimension axis for now. */
    else if (!strcmp(par->name, "freq_units1")) {
	if (par->value.f == 1.0) {
	    tempstr.value.s = "p";
	    write_stext_par("axis", &tempstr, CMXPAR_TYPE_STRING);
	}
	else if (par->value.f == 5.0) {
	    tempstr.value.s = "h";
	    write_stext_par("axis", &tempstr, CMXPAR_TYPE_STRING);
	}
    }
    /* Converting the referencing parameters assumes we have already
       discovered the sw and sfrq values; OK assumption for CMSS if these
       are in acq and reference params are in proc.  rmp1 is the reference
       point in absolute MHz, given center of the spectrum at sf1.  rmv1
       is the chemical shift to assign at that position, expressed in
       units described in rmvunits1 (normally only ppm and KHz found). */
    else if (!strcmp(par->name, "rmp1")) {
	tempfloat.value.f = esspar.sw/2 - (esspar.sfrq - par->value.f)*1.0e6;
	write_stext_par("rfl", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    /* This assumes rmvunits1=1 (units of ppm); usually but not
       necessarily true. */
    else if (!strcmp(par->name, "rmv1")) {
	tempfloat.value.f = esspar.sfrq * par->value.f;
	write_stext_par("rfp", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    /* This is for homonuclear 2-D.  For heteronuclear, should use dfrq;
       but there's no way to know whether this is homo or hetero. */
    else if (!strcmp(par->name, "rmp2")) {
	tempfloat.value.f = esspar.sw1/2 - (esspar.sfrq - par->value.f)*1.0e6;
	write_stext_par("rfl1", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    /* This assumes rmvunits2=1 (units of ppm); usually but not
       necessarily true. */
    else if (!strcmp(par->name, "rmv2")) {
	tempfloat.value.f = esspar.sfrq * par->value.f;
	write_stext_par("rfp1", &tempfloat, CMXPAR_TYPE_FLOAT);
    }
    /* Spinsight use_array is a Boolean which disables array-panel arrays.
       We use it to set our use_array global.  This is checked later by
       cmx_write_arrays() etc. */
    else if (!strcmp(par->name, "use_array")) {
	use_array = (int) (par->value.f + 0.5);
	write_stext_par("use_array", par, CMXPAR_TYPE_INT);
    }
    /* Only Spinsight is known to provide a date parameter.  It must be
       translated from MM/DD/YY format to MMM DD YYYY for VNMR >6.1a. */
    else if (!strcmp(par->name, "date")) {
	tempstr.value.s = date_numeric_to_english(par->value.s);
	write_stext_par("date", &tempstr, CMXPAR_TYPE_STRING);
	free(tempstr.value.s);
    }
    /* from here on down we are excluding parameters */
    else {
	if (strcmp(par->name, "wc"))
	    write_stext_par(par->name, par, CMXPAR_TYPE_NULL);
    }
}

static int cmx_is_2D(type)
long type;
{
	if (type & IN_CMSS) {
		if (save_current_size2 > 1) {
			if (arraydim == 1)
				return(TRUE);
			else
				return(FALSE);
		}
		else
			return(FALSE);
	}
	else {
		if (dw2_present)
			return(TRUE);
		else
			return(FALSE);
	}
}

/* Given a string containing a date in typical US numeric format (MM/DD/YY,
   e.g. 5/31/98, 12/2/98) returns a string in US English format ("Mmm DD YYYY",
   e.g. "May 31 1998", "Dec  2 1998").  The returned string is allocated by
   this function, and should be freed by the caller when no longer needed.  If
   the input year has 2 or 3 digits, it is treated like this:
	if <  Y2K_CUTOFF, add 2000
	if >= Y2K_CUTOFF, add 1900
   4-digit years are not altered.  This is the only explicit manipulation of
   dates in the "convert" package.  US numeric dates with 2-digit years are
   found in Spinsight data, and US English dates with 4-digit years are found
   in Varian native and sread formats, as of mid-1998 (VNMR 6.1a). */

#define Y2K_CUTOFF 80	/* 2-digit years lower than this assumed to be in 21st
			   century; higher assumed to be in 20th */

static char * date_numeric_to_english(numdate)
    char * numdate;
{
    int mm;
    int dd;
    int yy;
    int yyyy = 0;
    char * engdate;
    static char * monthnames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    /* Extract the numeric values. */

    if (sscanf(numdate, "%d/%d/%d", &mm, &dd, &yy) != 3) {
	errmsg(RC_BADDATE, __FILE__, __LINE__, (long) numdate);
	cleanup(RC_BADDATE);
    }

    /* Range checking for month and day. */

    if (mm < 1  ||  mm > 12) {
	errmsg(RC_BADDATE, __FILE__, __LINE__, (long) numdate);
	cleanup(RC_BADDATE);
    }

    if (dd < 1  ||  dd > 31) {
	errmsg(RC_BADDATE, __FILE__, __LINE__, (long) numdate);
	cleanup(RC_BADDATE);
    }

    /* Year checking.  Y2K cutoff code is here. */

    if (yy < 1) {
	errmsg(RC_BADDATE, __FILE__, __LINE__, (long) numdate);
	cleanup(RC_BADDATE);
    }
    else if (yy < Y2K_CUTOFF)
	yyyy = yy + 2000;
    else if (yy < 1000)
	yyyy = yy + 1900;
    else if (yy < 10000)
	yyyy = yy;
    else {
	errmsg(RC_BADDATE, __FILE__, __LINE__, (long) numdate);
	cleanup(RC_BADDATE);
    }

    /* Allocate a string to hold the return value. */

    engdate = malloc(12);
    if (engdate == NULL) {
	errmsg(RC_NOMEMORY, __FILE__, __LINE__, 12);
	cleanup(RC_NOMEMORY);
    }

    /* Fill the string. */

    sprintf(engdate, "%3s %2d %4d", monthnames[mm-1], dd, yyyy);

    return(engdate);
}

