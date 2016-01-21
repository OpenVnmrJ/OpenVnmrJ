/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------
|  HSgate()
|	sets the bit in the HSlines to the given state .
|				Author Greg Brissey  5/13/86
+-----------------------------------------------------------------------*/
#include "acodes.h"

extern int HSlines;
extern int presHSlines;
/* extern int fifolpsize; */
extern int SkipHSlineTest;
extern int newacq;

HSgate(bit,state)
int bit;	/* bit to set */
int state;	/* state to set bit, (0 or 1) */
{

    if (state)
	HSlines |= bit;
    else
	HSlines &= ~bit;

    if (newacq)
    {
	if (state)
	   putcode(IMASKON);
	else
	   putcode(IMASKOFF);
	putcode( ((bit >> 16) & 0xffff) );	/* high order word */
	putcode( (bit & 0xffff) );		/* low order word */
	return;
    }

/*
/* Remove output board differences
/*
/* if ( fifolpsize > 65)
 */
    /* There maybe more subtle ways but its fast and sure */
    /* Well almost, I forgot ifzero() constructs in the uses pulse sequence */
    /* so upon entering the PS SkipHSlineTest is set to 1 thus avoiding the */
    /* problem.	*/
    if ( (HSlines != presHSlines) || SkipHSlineTest )
    {
      putcode(HSLINES);
      putcode( ((HSlines >> 16) & 0xffff) );	/* high order word */
      putcode( (HSlines & 0xffff) );		/* low order word */
      presHSlines = HSlines;
    }
}
