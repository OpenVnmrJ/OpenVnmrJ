/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "acodes.h"
#include "rfconst.h"
#include "group.h"
#include "acqparms.h"

#define WHOLEBODY_SHIMSET	11	/*shimset number for WHOLEBODY Shims*/

extern void initorgHSlines();

extern int  bgflag;	/* debug flag */
extern int  presHSlines;
extern int  sync_flag;	/* did we synchronize decoupler mode yet? 1=yes */
extern int  newacq;

/*------------------------------------------------------------------
|
|	clearHSlines()
|	clear the acquisitions High Speed Lines to safe state.
|				Author Greg Brissey 10/29/92
|
+-----------------------------------------------------------------*/
clearHSlines()
{
    codeint *ptr;		/* pointer for Codes array */
    /* --- initialize quiescent states --- */
    /* --- establish start and current quiescent HSline states --- */

    set_lacqvar(HSlines_ptr, 0);
    HSlines = 0;	/* Clear all High Speed line bits */
    presHSlines = 0;		/* Clear present High Speed line bits */
    statusindx = 0;		/* Zero status index  = A e.g., dm[statusindx] */
    if (newacq)
    {
	initorgHSlines(HSlines);
    }
    if (bgflag)
        fprintf(stderr,"clearHSlines(): HSlines: 0x%lx presHSlines: 0x%lx\n",
		HSlines,presHSlines);
}
/*------------------------------------------------------------------
|
|	initHSlines()
|	initialize the acquisitions High Speed Lines to their
|	initial safe state. This state is also store in the structure
|	acqparams section of code for the acquisition CPU 
|       Note: HSlines is passed to acquisition in EVENT1 & EVENT2 
|	      Acodes, only then are the lines actually set
|				Author Greg Brissey 6/26/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to calc acode offsets 
+-----------------------------------------------------------------*/
initHSlines()
{
    codeint *ptr;		/* pointer for Codes array */
    extern void init_pgd_hslines();
    extern void reset_decstatus();

    /* --- initialize quiescent states --- */
    /* --- establish start and current quiescent HSline states --- */

    init_pgd_hslines(&HSlines);

    /* init to status A conditions, but leave decoupler on only for dm='a' */
    setPostExpDecState(A,0.0);
    statusindx = A;

    sync_flag=0;
    reset_decstatus();

    if (newacq)
    {
	recon();		/* receiver on for safe state at end. */
	putcode(INITHSL);
        putcode( ((HSlines >> 16) & 0xffff) );	/* high order word */
        putcode( (HSlines & 0xffff) );		/* low order word */
	putcode(ISAFEHSL);
        putcode( ((HSlines >> 16) & 0xffff) );	/* high order word */
        putcode( (HSlines & 0xffff) );		/* low order word */
    }

    /*------------------------------------------------------------------*/
    /* if inova, turn receiver off. Yes, the receiver gate is active	*/
    /* low. The default state of the receiver is on which means the 	*/
    /* gate is off.  The current exception is for 4T & 3T systems. 	*/
    /*------------------------------------------------------------------*/
    if (newacq)
    {
	double tmpshimset;
	if ( P_getreal(GLOBAL,"shimset",&tmpshimset,1) < 0 )
	{
	   tmpshimset = 1.0;
	}
	if ((int)(tmpshimset) == WHOLEBODY_SHIMSET)
	{
	   /*---------------------------------------------------*/
	   /* default receiver on in order for WHOLEBODY T/R	*/
	   /* switch to work correctly.	Currently the only way	*/
	   /* to determine a wholebody system is the shimset.	*/
	   /*---------------------------------------------------*/
	   recon();
	}
	else
	{
	   recoff();	/* default receiver Off for all other systems */
	}

    }

    /* start quiencent state contain dm,dmm and dhp HS settings */
    set_lacqvar(HSlines_ptr, HSlines);
    if (newacq)
        init_acqvar(HSlines_ptr, HSlines); 

    if (bgflag)
        fprintf(stderr,
	 "initHSlines(): HSlines: 0x%x \n", HSlines);
}
