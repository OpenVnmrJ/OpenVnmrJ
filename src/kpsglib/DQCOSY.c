// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* DQCOSY - Double Quantum Filtered COSY (DQCOSY)

        Paramters:
                sspul :         selects magnetization randomization option

KrishK  -       last revision:  June 1997
KrishK	-	Revised		: July 2004

*/


#include <standard.h>
#include <chempack.h>

static int 	ph1[8]  = {0,0,0,0,1,1,1,1},
		ph2[16] = {1,1,1,1,0,0,0,0,3,3,3,3,2,2,2,2},
		ph3[16]	= {1,2,3,0,2,3,0,1,3,0,1,2,0,1,2,3},
		ph4[8]  = {0,3,2,1,3,2,1,0},
		ph6[1]  = {0};

void pulsesequence()
{

  int   phase1 = (int)(getval("phase")+0.5);

	settable(t1,8,ph1);
	settable(t2,16,ph2);
	settable(t3,16,ph3);
	settable(t4,8,ph4);
	settable(t6,1,ph6);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t6,ct,v6);
	getelem(t3,ct,v3);
	getelem(t4,ct,oph);

	if (phase1 == 2)
	 {incr(v1); incr(v6);}
/*
	mod2(id2,v10);
	dbl(v10,v10);
*/
  	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

		add(v1,v10,v1);
		add(v6,v10,v6);
		add(oph,v10,oph);

status(A);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        satpulse(satdly,zero,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

status(B);

	rgpulse(pw, v1, rof1, 2.0e-6);
	if (d2 > 0.0)
	 delay(d2 - (4*pw/PI) - 4.0e-6);
	else
	 delay(d2); 
	rgpulse(pw, v2, 2.0e-6, rof1);
	rgpulse(pw, v3, rof1, rof2);

status(C);
}

