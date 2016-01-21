// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* ATgNhmqc.c
    Gradient HMQC for N15 in D2O
    Modified from rna_gNhqmc.c from RnaPack .
*/

#include <standard.h>

static int	phi1[8]	= {0,0,1,1,2,2,3,3},
		phi2[2]	= {0,2},
		phi3[8]	= {2,2,3,3,0,0,1,1},
		phi4[1] = {0},
		rec[8]	= {0,2,2,0,0,2,2,0};

pulsesequence()

{

/* DECLARE VARIABLES */	

double
	taunh,		/* 1/4J(NH)			*/
	pwxlvl,		/* power level for 15N hard pulse */
	pwx,		/* pulse width for 15N hard pulse */
	j;		/* coupling for NH		*/

	j	= getval("j");
	pwx	= getval("pwx");
	pwxlvl	= getval("pwxlvl");
	at	= getval("at");

/* LOAD PHASE PARAMETERS */

	settable(t1, 8, phi1);
	settable(t2, 2, phi2);
	settable(t3, 8, phi3);
	settable(t4, 1, phi4);
	settable(t5, 8, rec);

/* INITIALIZE VARIABLES */

	taunh =  1/(4.0*j);

	decpower(pwxlvl);	/* Set DEC power to pwxlvl	*/
	obspower(tpwr);
	delay(d1);

        decphase(t2);

        rgpulse(pw, zero, rof1, rof1);

        delay(taunh );	/* delay = 1.0/2J(NH) */
        delay(taunh);
	decrgpulse(pwx, t2, 0.0, 0.0);
	decphase(t4);


       rgpulse(2*pw, t1, rof1, rof1);
      txphase(zero);
	 
	decrgpulse(pwx, t4, 0.0, 0.0);

	delay(taunh );		/* delay = 1/2J(NH) */
        delay(taunh);
	setreceiver(t5);
}
