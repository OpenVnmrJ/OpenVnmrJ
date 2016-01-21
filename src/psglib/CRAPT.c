// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*
CRAPT - CRISIS APT
KrishK  -       Includes slp saturation option : July 2005
KrishK - 	includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/


#include <standard.h>
#include <chempack.h>

static int phs1[4] = {0,2,1,3},
           phs3[4] = {0,0,1,1},
           phs2[8] = {0,0,1,1,2,2,3,3};

pulsesequence()
{
  double   tpwr180r = getval("tpwr180r"),
           pw180r = getval("pw180r"),
	   pwx180 = getval("dnbippw"),
	   pwxlvl180 = getval("dnbippwr"),
           pp = getval("pp"),
           pplvl = getval("pplvl"),
	   tauC = getval("tauC"),
	   tau;
  int      prgcycle=(int)(getval("prgcycle")+0.5);
  char     pw180ref[MAXSTR],
	   dnbipshp[MAXSTR],
	   bipflg[MAXSTR];

  getstr("pw180ref", pw180ref);
  getstr("dnbipshp", dnbipshp);
  getstr("bipflg",bipflg);
  tau    = 1.0 / (2.0 * (getval("j1xh")));

  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && getflag("satmode") && (prgcycle > 1.5))
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

  if (getflag("prgflg") && getflag("satmode"))
        assign(v3,v2);

   /* equilibrium period */
   status(A);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (getflag("satmode"))
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

   if (getflag("wet"))
     wet4(zero,one);

   status(B);

	obspower(tpwr);
	if (bipflg[1] == 'y')
	    decpower(pwxlvl180);
	else
	    decpower(pplvl);

        rgpulse(pw,v1,rof1,rof1);
	if (bipflg[1] == 'y')
	    { 
		decshaped_pulse(dnbipshp,pwx180,zero,2.0e-6,2.0e-6);
		delay(tau+tauC);
	    }
	else
	    {
	    	decrgpulse(2*pp,zero,2.0e-6,2.0e-6);
		delay(tau);
	    }
	obspower(tpwr180r);
	shaped_pulse(pw180ref,pw180r,v1,rof1,rof1);
        if (bipflg[1] == 'y')
            { 
                decshaped_pulse(dnbipshp,pwx180,zero,2.0e-6,2.0e-6);
                delay(tau+tauC);
            }
        else
            {
                decrgpulse(2*pp,zero,2.0e-6,2.0e-6);
                delay(tau);
            }

        shaped_pulse(pw180ref,pw180r,v1,rof1,rof2);
	decpower(dpwr);
	obspower(tpwr);

   status(C);
}
