/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*  HISTORY:
*
*  Revision 1.5  2010/8/30 23:13:39  deans
*  modified sequence to use tpwr and tpwrf

*  Revision 1.4  2006/10/18 23:13:39  mikem
*  replaced RF shape file with fine-phase control in sequence
*
*  Revision 1.3  2006/10/17 23:26:48  mikem
*  Added fine control for phase
*
*  Revision 1.2  2006/10/13 00:33:20  mikem
*  fixed bug, finished program
*
*  Revision 1.1  2006/10/12 00:23:56  mikem
*  Initial check in
*
* Author: Michael R. Meiler
* Copyright (c) 2005 Varian, Inc.  All Rights Reserved
*
* Varian, Inc. and its contributors. Use, disclosure and reproduction
*********************************************************************/

/***********************************************************************
loopback sequence
************************************************************************/
#include <standard.h>

/* floating point comparison macros */
#define EPSILON 1e-9      /* largest allowable deviation due to floating */
                                        /* point storage */
#define FP_GT(A,B) (((A) > (B)) && (fabs((A) - (B)) > EPSILON)) /* A greater than B */

void pulsesequence()
{
	/* Internal variable declarations *********************/
	char txphase[MAXSTR];
	char rxphase[MAXSTR];
    char blankmode[MAXSTR]={0};
	double  postDelay;
	double rfDuration;
	double acqt;
	int i,ret=-1;
	static int phs1[4] = {0,2,1,3}; /* from T1meas.c */

	/*************************************************/                                     
	/*  Initialize paramter **************************/
	i                = 0;
	postDelay        = 0.5;
	acqt             = 0.0;
	getstr("rxphase",rxphase);
	getstr("txphase",txphase);  

	ret = P_getstring(GLOBAL,"blankmode",blankmode,1,MAXSTR);
    //getparm("blankmode","string",GLOBAL,blankmode,MAXSTR);
	postDelay = tr - at;

   //printf("blankmode=%s\n",blankmode);
                        
	/*************************************************/
	/* check phase setting ***************************/
	if ( (txphase[0] =='n')   && (rxphase[0] =='n') )
	{
		abort_message("ERROR - Select at least one phase [Tx or Rx]\n");   
	}

	/**************************************************/
	/* check pulse width  *****************************/
	rfDuration = shapelistpw(p1pat, p1);     /* assign exitation pulse  duration */
	acqt = rfDuration + rof1 - alfa;
	if (FP_GT(acqt, at))
	{
		abort_message("Pulse duration too long. max [%.3f]    ms\n",(at-rof1+alfa)*1000.0);   
	}
    if(ret==0 && blankmode[0]=='u')
    	obsunblank();
	delay(postDelay);
    
	settable(t1,4,phs1); /*from T1meas.c */
	getelem(t1,ct,v11);  /*from T1meas.c */
	setreceiver(t1);                    
	/*==============================================*/
	/*  START LOOPBACK PULSE SEQUENCE               */
	/*==============================================*/
	status(A);
	obsoffset(resto);

	/* TTL trigger to scope sequence ****************************/       
	sp1on();             

	/* Relaxation delay ***********************************/       
    xgate(ticks);

	/* RF pulse *******************************************/ 
	obspower(tpwr);
	obspwrf(tpwrf);
	ShapedXmtNAcquire(p1pat, rfDuration, v11, rof1, OBSch);

	endacq();
	sp1off();
    if(ret==0 && blankmode[0]=='u')
 		obsunblank();
}

