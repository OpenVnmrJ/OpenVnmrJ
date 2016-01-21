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
/*****************************************************************************/
/*        ECC1: SEQUENCE TO MAP EDDY CURRENT BEHAVIOUR                       */
/*              USING BALANCED GRADIENTS                                     */
 /*             VERSION 2: JULY 24, 1989, H. EGLOFF                       */
/*****************************************************************************/


/* [1] STANDARD INCLUDE FILES */
#include "standard.h"


/* [2] EXTERNAL DECLARATIONS */
extern int bgflag;
#define GRISE  0.002
#define RISERATE 0.0000005


/* [3] PULSE SEQUENCE CODE */

void pulsesequence()
{
        /* [3.1] DECLARATIONS */
 
        double absgro;
	double gtime;
	int    igro;
        char   gread,gslice,gphase;

	initparms_sis();

	griserate = trise/gradstepsz;

        /* [3.2] PARAMETER READ IN FROM EXPERIMENT */
        gro=getval("gro");
	gtime=getval("gtime");

        gread = 'z';
        getorientation(&gread,&gphase,&gslice,"orient");

 
        /* [3.3] CALCULATIONS */
        igro=(int)gro;
	if (gro < 0.0) {absgro = 0.0-gro;}
	  else {absgro=gro;}
	
    

        /* [3.4] SEQUENCE ELEMENTS */
/*        SetRFChanAttr(RF_Channel[OBSERVE], SET_SPECFREQ,sfrq,NULL); */
/*	offset(tof,TODEV); */

	xgate(ticks);

        status(A);
        mod4(ct,oph);
        delay(d1);

	gradient(gread,-igro);
	delay(gtime);
	gradient(gread,0);

	delay(d1);
        
        gradient(gread,igro);
        delay(gtime);
	gradient(gread,0);
	delay(griserate * absgro);		/* wait for gradient ramp */
	delay(d2);
	rgpulse(pw,ct,rof1,rof2);

   startacq(alfa);
   acquire(np,1.0/sw);
   endacq();

}

