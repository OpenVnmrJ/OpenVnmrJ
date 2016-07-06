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

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */

#ifndef  icatSpih
#define  icatSpih

#include <vxWorks.h>
#include "ficl.h"
#include "icatSpiFifo.h"

void icat_set_base_addr();
void spi_isf_csb_low();
void spi_isf_csb_high();
unsigned spi_isf_send(unsigned value, int bits);
int spi_isf_status();
int spi_isf_complete();
ficlUnsigned rf_spi_init(ficlVm *vm);

#endif /* icatSpih */

