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
struct cmxpar {
	char * name;
	union {
		double f;
		char * s;
		long h;
	} value;
	char type;
	char unit;
};

#define CMXPAR_TYPE_NULL	0x0
#define CMXPAR_TYPE_FLOAT	0x1
#define CMXPAR_TYPE_STRING	0x2
#define CMXPAR_TYPE_HEX		0x4
#define CMXPAR_TYPE_INT		0x8
#define CMXPAR_TYPE_ARRAY	0x40

struct cmxarray {
	struct cmxarray * next;
	char * name;
	int num_values;
	int dim;
	char type;
	struct cmxpar * values;
};

#define CMX_MAX_ARRAYS	40	/* 1st two can probably never be used */

#define BUFFERSIZE	256

#define NUM_VALID_UNITS	5
static char valid_units[NUM_VALID_UNITS] = { 's', 'm', 'u', 'n', 'k' };
/* units should also be implemented in apply_units() */

/* definitions of interesting bit masks in the stat parameter */
#define CMX_STAT_FLOAT		0x2	/* otherwise it's 32 bit integer */
#define CMSS_DATATYPE_INT	0
#define CMSS_DATATYPE_FLOAT	1

#define CMX_PARNAME	"cmx"		/* system name for stext */

/* temporary file to hold stext as it is being built */
#define CMX_TEMPFILE	"/tmp/cmx_stext.tmp"
