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
static char Version_ID[] = "unity.c v2.71 STM 3/13/98 STM";
/* revision history:
   2.70	created, initially with just Unity fid file input/output
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

/* local prototypes */
#ifdef __STDC__

void unity_read_fid_header(long type);

#else /* __STDC__ */

void unity_read_fid_header();

#endif /* __STDC__ */

/* Currently the only input supported is fid file (no parameters) within fid
   dir, and output is fid file.
*/
long unity_file(type)
long type;
{
    unity_read_fid_header(type);

    /* temporary:  only support 1 block, 1 trace, long integers */
    if (in_fhead.nblocks != 1  ||
	in_fhead.ntraces != 1  ||
	in_fhead.ebytes != 4) {
	errmsg (RC_ONOTYET, __FILE__, __LINE__, 0);
	cleanup (RC_ONOTYET);
    }

    /* For now just copy the input headers to the output headers. */
    memcpy((char*) &out_fhead, (char*) &in_fhead, sizeof (struct datafilehead));
    memcpy((char*) &out_bhead, (char*) &in_bhead, sizeof (struct datablockhead));
}

void unity_read_fid_header(type)
long type;
{
    if (fread (&in_fhead, sizeof (struct datafilehead), 1, fid_in) != 1 ) {
	errmsg (RC_READERR, __FILE__, __LINE__, sizeof (struct datafilehead));
	cleanup (RC_READERR);
    }
    if (fread (&in_bhead, sizeof (struct datablockhead), 1, fid_in) != 1 ) {
	errmsg (RC_READERR, __FILE__, __LINE__, sizeof (struct datablockhead));
	cleanup (RC_READERR);
    }
}

