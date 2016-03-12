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

/* iCAT Internal System Flash  (ISF)  */
/* Author: Greg Brissey  1/20/2010 */

/*
DESCRIPTION

   functions to handle the iCAT Internal System Flash (ISF) 
   reading and writing, etc..

*/

#ifndef  simonh
#define  simonh

#include <vxWorks.h>
#include "icatSpiFifo.h"
#include "ficl.h"

extern int simon_boot(char *);
extern void simon(void);
extern void full_simon(void);
extern char *find_and_load (const char *, unsigned *, int *);
extern ficlSystem *get_simon_system();
#endif

