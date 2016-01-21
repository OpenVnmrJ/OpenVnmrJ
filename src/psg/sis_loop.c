/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************************
                        sis_loop
        
        SEQD file for switchable loop procedures
********************************************************************/

#include <stdio.h>
#include <math.h>
#include "oopc.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"
#include "acodes.h"
#ifndef PSG_LC
#define PSG_LC
#endif
#include "lc.h"

extern double getval();
extern Acqparams *Alc;
extern int bgflag;    /* debugging flag */
extern int newacq;

static codeint	rtnsc_ptr;	/* offset to user controlled (rt) NSC acode */


/**********************************************************
                        msloop()

        Procedure to provide a sequence switchable
        multislice loop header.

        The two APV variables hold the values of the 
        maximum count (apv1) and the current counter
        value (apv2).
***********************************************************/
msloop(state,max_count,apv1,apv2)
char    state;
double  max_count;
codeint apv1,apv2;
{
    int k;                  /*Slice step index*/

/*-----------------------------------------------------------
    state='c' designates compressed mode.  Set up the loop()
    of a loop/endloop pair in the sequence.  APV1 is used
    internally to hold the maximium count for the loop.
    APV2 counts 0 to maxcount-1.
-----------------------------------------------------------*/
    if (state == 'c') {
        if (max_count > 0.5) {
            initval(max_count,apv1);
            loop(apv1,apv2);
        }
        else {
            initval(1.0,apv1);
            loop(apv1,apv2);
        }
    }

/*-----------------------------------------------------------
    state='s' designates standard arrayed mode. APV1 is
    used to hold the max count. APV2 counts 0 up.
-----------------------------------------------------------*/
    else if (state == 's') {
	if (max_count > 1) {
            text_error("msloop: Illegal std multislice array\n");
            psg_abort(1);
	}
        k=0;
        initval(max_count,apv1);
        initval((double)k,apv2);
    }

/*-----------------------------------------------------------
    Illegal state
-----------------------------------------------------------*/
    else {
        text_error("msloop: Illegal looping state\n");
        psg_abort(1);
    }       
}






/*******************************************************
                       endmsloop()

    Procedure to close switchable multislice loop.
********************************************************/
endmsloop(state,apv1)
char    state;
codeint apv1;
{
    if (state == 'c') {
        endloop(apv1);
    }
}




/********************************************************
                         peloop()

    Procedure to provide a switchable phase encode loop header.
    The APV variables hold the max count and the current count
    for the loop.
*********************************************************/
peloop(state,max_count,apv1,apv2)
char    state;
double  max_count;
codeint apv1,apv2;
{
    switch(state) {
        /*-------------------------------------------------
         Standard encode loop: Put the current 2D
	 increment value into APV2.  APV2 counts 0 up to
	 maxcount-1. APV1 holds the maximum count.
        -------------------------------------------------*/
        case 's':
            if (max_count > 0.5) {
                initval(max_count,apv1);
                initval((double)(nth2D-1),apv2);
            }
            else {
                assign(zero,apv1);
                assign(zero,apv2);
            }
            break;

        /*-------------------------------------------------
         Compressed Encode Loop: APV2 counts 0 up because
	 of loop(). APV1 holds the maximum count.      
        -------------------------------------------------*/
        case 'c':
            if (max_count > 0.5) {
                initval(max_count,apv1);
                loop(apv1,apv2);
            }
            else {
                initval(1.0,apv1);
                loop(apv1,apv2);
            }
            break;


        /*-------------------------------------------------
         Any other state is invalid
        -------------------------------------------------*/
        default:
            text_error("peloop: Illegal loop state\n");
            psg_abort(1);
    }
}



/********************************************************
              endpeloop()

    Procedure to close switchable phase encode loop
*********************************************************/
endpeloop(state,apv1)
char    state;
codeint apv1;
{
    if (state == 'c') {
        endloop(apv1);
    }
}



/*****************************************************
    loop_check()

    Has the interface set the values of ni,nf
    nv,nv2,nv3,ne & ns correctly. If not abort.
******************************************************/
loop_check()
{
    int    nfp;             /*nf product*/
    int    counts[5];       /*values of loop count variables*/
    int    k;               /*loop index counter*/

    counts[0] = IRND(ne);      /*Number of echoes*/
    counts[1] = IRND(ns);      /*Number of slices*/
    counts[2] = IRND(nv);      /*Number of 2D views*/
    counts[3] = IRND(nv2);     /*Number of 3D views*/
    counts[4] = IRND(nv3);     /*Number of 4D views*/

/*--------------------------------------------------------
    nfp is the product of all compressed loop counts
    present in seqcon.
---------------------------------------------------------*/
    nfp = 1;
    for (k=0; k<5; k++) {
        if (seqcon[k] == 'c'  &&  counts[k] > 0) {
            nfp = nfp*counts[k];
        }
    }

    if ((int)(nf + 0.5) != nfp) {
        printf("loop_check: nf set incorrectly.\n");
        printf("Use setloop command to reset nf.\n");
        psg_abort(1);
    }
}

/*-------------------------------------------------------------------
|
|	init_vscan(rtvar,num_points)
|	To be used with vscan(rtvar).  Initializes variables that will
|	used by vscan(rtvar), in particular it sets 'ct' to 0.
|	 
|	usage:
|		loop(...);
|		   init_vscan(v1,np*nv);
|		   .
|		   .
|		   acquire(...);
|		   vscan(v1);
|		endloop(..);
|	 
|				Author Matt Howitt 9/28/92
+------------------------------------------------------------------*/
Init_vscan(rtvar,npts)
codeint rtvar;
double npts;
{
    if (newacq)
    {
	if ( getIlFlag() )
	{
	   text_error("Error: vscan will not work with interleaving.");
	   psg_abort(1);		/* abort process */
	}
	init_acqvar(npr_ptr,(int)(npts + 0.0005));
	putcode(INITVSCAN);
	assign(ntrt,rtvar);
    }
    else
    {
   	Alc->acqnp = (codelong) (npts + 0.0005); 	/* np       loc 13*/
	putcode(INITVSCAN);
	if ((nt > 0.0) && (nt < 32767.5))
    	{
	   assign(ntrt,rtvar);
    	}
    	else
    	{
	   text_error("Error: nt is less than zero or greater than 32767.");
	   psg_abort(0);		/* abort process */
    	}
    }

    rtnsc_ptr = (int) (Codeptr - Aacode); 	/* offset of start of scan */
    if (bgflag)
	fprintf(stderr,"rtnsc_ptr offset: %d \n",rtnsc_ptr);
    putcode(NSC);	
}
/*-------------------------------------------------------------------
|
|	vscan(rtvar)
|	To be used with init_vscan(rtvar) 
|
|	increments data pointer 
|	zeros data table for next set of points
|	Performs housekeeping chores 
|				Author Matt Howitt 9/28/92
+------------------------------------------------------------------*/
vscan(rtvar)
codeint rtvar;
{
    putcode(STFIFO);	/* start fifo if it already hasn't */
    putcode(HKEEP);		/* do house keeping */
    decr(rtvar);
    ifzero(rtvar);
    elsenz(rtvar);
    	putcode(BRANCH);	/* brach back to start of scan (NSC) */
    	putcode(rtnsc_ptr);	/* pointer to nsc */
    endif(rtvar);
}
