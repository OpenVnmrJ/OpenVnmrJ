// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* COSY - homonuclear correlation spectroscopy (absolute value)

	Paramters:
		sspul   :	selects magnetization randomization option
		cmult   :	selects COSY-45/COSY-90/COSY-135 (0.5/1/1.5)	

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK  - 	Includes purge option : Aug. 2006
BHeise  - 	Includes COSY-beta/COSY-45 option
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/


#include <standard.h>
#include <chempack.h>

static int 	ph1[8] = {0, 0, 1, 1, 2, 2, 3, 3},
		ph2[8] = {0, 1, 3, 0, 2, 3, 1, 2},
		ph3[8] = {0, 2, 1, 3, 2, 0, 3, 1};

void pulsesequence()
{
        int  prgcycle = (int)(getval("prgcycle")+0.5);
        double cmult = getval("cmult");

  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }

	assign(zero,v4);
	settable(t1,8,ph1);
	settable(t2,8,ph2);
	settable(t3,8,ph3);

	getelem(t1,v17,v1);
	getelem(t2,v17,v2);
	getelem(t3,v17,oph);

        if (getflag("prgflg") && (satmode[0] == 'y'))
	   assign(v1,v4);

   add(oph,v18,oph);
   add(oph,v19,oph);

/*
	mod2(id2,v10);
	dbl(v10,v10);
*/
  	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

	add(v1,v10,v1);
	add(v4,v10,v4);
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
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v4);
                if (getflag("prgflg"))
                   shaped_purge(v1,v4,v18,v19);
           }
        else
           {
                satpulse(satdly,v4,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v4,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);


status(B);
	if (getflag("cpmgflg"))
	{
		rgpulse(pw, v1, rof1, 0.0);
		cpmg(v1, v15);
	}
	else
		rgpulse(pw, v1, rof1, rof1);
	delay(d2); 
	rgpulse(cmult*pw, v2, rof1, rof2);

status(C);
}
