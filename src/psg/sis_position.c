/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************
                        sis_position
        
        SEQD file for position control sequence elements.

        from VERSION 9.dev      5th April 1991

************************************************************************/

#include <stdio.h>
#include <math.h>
#include "oopc.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"
#include "abort.h"

extern int offset(double off, int dev);
	

/******************************************************
            nucleus_gamma()/1

    Return the value of the nucleus gyromagnetic
    ratio expressed in Hz/G (gamma over two pi)
    as a double. Use the transmitter device and
    static field value to calculate this value.
*******************************************************/

double nucleus_gamma(int device)
{
    double nucgamma = 0.0;

    if (B0 <= 0.0) {
        text_error("nucleus_gamma: B0 (field) value <= zero\n");
        psg_abort(1);
    }

    if (device == DODEV)
        nucgamma=MHZ_HZ*dfrq/B0;
    else if (device == TODEV)
        nucgamma=MHZ_HZ*sfrq/B0; 
    else {
        text_error("nucleus_gamma: Bad transmitter device\n");
        psg_abort(1);
    }

    return(nucgamma);
}


/******************************************************
            position_offset()

    Procedure to set RF frequency from position
    and conjugate gradient values.
*******************************************************/

void S_position_offset(double pos, double grad, double resfrq, int device)
{
    double nucgamma;        /*Nucleus gyromagnetic ratio in Hz/G*/

    nucgamma = nucleus_gamma(device);
    offset((resfrq+nucgamma*grad*pos),device);
}



/******************************************************
              position_offset_list()

    Procedure to set RF frequency from a position
    list, conjugate gradient value and dynamic
    math selector.

    The index APV (apv1) holds the index for required 
    slice offset value as stored in the array.

    NOTE: The arrays provided to this element must 
    count zero up. ie array[0] must have the first 
    slice position and array[ns-1] the last.
*******************************************************/

void S_position_offset_list(double posarray[], double grad, double nslices,
                            double resfrq, int device, int listno, codeint apv1)
{
    int     k;
    double  freqarray[MAXSLICE], nucgamma;
        
    if (nslices <= 1)
        position_offset(posarray[0],grad,resfrq,device);
    else {
        nucgamma = nucleus_gamma(device);
	for (k=0; k<(int)nslices; k++) {
	    freqarray[k] = resfrq + nucgamma*grad*posarray[k];
	}
	create_offset_list(freqarray,(int)nslices,device,listno);
        voffset(listno,apv1);
    }
}
