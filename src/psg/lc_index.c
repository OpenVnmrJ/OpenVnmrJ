/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#ifndef PSG_LC
#define PSG_LC
#endif
#include "lc_index.h"


extern int newacq;


/* These programs were put in to help out ACQI on the new digital console.
   Add TABOFFSET to get the correct offset into the real time parameter file.
   Add more programs as required.					*/

int
new_recgain_index()
{
	if (newacq)
	  return( RECGAININDEX );
	else
	  return( -1 );
}

int
new_nt_index()
{
	if (newacq)
	  return( NTINDEX );
	else
	  return( -1 );
}

int
new_bs_index()
{
	if (newacq)
	  return( BSINDEX );
	else
	  return( -1 );
}

int
new_cp_index()
{
	if (newacq)
	  return( CPFINDEX );
	else
	  return( -1 );
}

int
new_ctss_index()
{
	if (newacq)
	  return( CTSSINDEX );
	else
	  return( -1 );
}

int
new_oph_index()
{
	if (newacq)
	  return( OPHINDEX );
	else
	  return( -1 );
}

int
new_taboffset()
{
	if (newacq)
	  return( TABOFFSET );
	else
	  return( -1 );
}
