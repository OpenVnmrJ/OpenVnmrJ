// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*
KrishK - includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
*/


#include <standard.h>
#include <chempack.h>

static int phs1[4] = {0,2,1,3},
	   phs3[4] = {0,0,1,1},
           phs2[8] = {0,0,1,1,2,2,3,3};

pulsesequence()
{
  double satdly = getval("satdly");
  int prgcycle=(int)(getval("prgcycle")+0.5);
  char	satmode[MAXSTR],
	sspul[MAXSTR];

  getstr("satmode",satmode);
  getstr("sspul",sspul);

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

  settable(t1,4,phs1);
  settable(t2,8,phs2);
  settable(t3,4,phs3);
  getelem(t1,v17,v1);
  getelem(t2,v17,v2);
  getelem(t3,v17,v3);
  assign(v1,oph);
  add(oph,v18,oph);
  add(oph,v19,oph);

  if (getflag("prgflg") && (satmode[0] == 'y'))
	assign(v3,v2);

   /* equilibrium period */
   status(A);

   delay(5.0e-5);
   if (sspul[0] == 'y')
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
	if (getflag("slpsat"))
	   {
		shaped_satpulse("relaxD",satdly,v2);
                if (getflag("prgflg"))
                   shaped_purge(v1,v2,v18,v19);
	   }
	else
	   {
        	satpulse(satdly,v2,rof1,rof1);
		if (getflag("prgflg"))
		   purge(v1,v2,v18,v19);
	   }
     }
   else
	delay(d1);

   status(B);

   pulse(p1,zero); 
   hsdelay(d2); 
   if (getflag("cpmgflg"))
      {
         rgpulse(pw, v1, rof1, 0.0);
	 cpmg(v1, v15);
      }
   else
      rgpulse(pw, v1, rof1, rof2);

   status(C);
}
