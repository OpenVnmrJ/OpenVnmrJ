// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* DEPT - distortionless enhancement by polarization transfer
	- Option to retain quaternary carbon
		Broadband inversion version option (using BIP or adiabatic) 
		in 13C channel

  REF:  MRC, 45, 469 (2007)

	Parameters:
	   j1xh : 	CH coupling constant
	   mult :	Multiplication factor for the theta pulse
	   		 (set as an array = 0.5,1,1,1.5 for editing)
	   qphase:	set as an array of -1,+1 for quaternary edit
			set as 0 to suppress quaternary carbons
	   qtip:	tip angle (in degrees) for the first Carbon pulse
			  [Ernst angle - affects quaternary carbons only)
	   qrelax:	Relaxation delay for quat carbons.  This preceeds the
			usual d1 value and the decoupler is turned ON
	   sbbiflg:	Shaped BB inversion flag - turns on/off wideband shaped inversion

Krishk	-			: Jan 2008
*/


#include <standard.h>
#include <chempack.h>

static int   phi3[2] = {1,3},
	     phi4[8] = {1,1,2,2,3,3,0,0},
	     rec_1[16] = {1,3,2,0,3,1,0,2,3,1,0,2,1,3,2,0},
	     phi2[16] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
	     
void pulsesequence()
{
   double          mult = getval("mult"),
                   pp = getval("pp"),
		   pplvl = getval("pplvl"),
		   qphase = getval("qphase"),
		   qtip = getval("qtip"),
		   qrelax = getval("qrelax"),
		   gzlvl5 = getval("gzlvl5"),
		   gt5 = getval("gt5"),
		   tpwr180 = getval("tpwr180"),
		   pw180 = getval("pw180"),
		   qpw,
                   tau;
   char		   pw180ad[MAXSTR],
		   sbbiflg[MAXSTR],
		   bipflg[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt5 = syncGradTime("gt5","gzlvl5",1.0);
        gzlvl5 = syncGradLvl("gt5","gzlvl5",1.0);

  getstr("pw180ad", pw180ad);
  getstr("bipflg", bipflg);
  getstr("sbbiflg",sbbiflg);

  if (bipflg[0] == 'y')
  {
        tpwr180=getval("tnbippwr");
        pw180=getval("tnbippw");
        getstr("tnbipshp",pw180ad);
  }

   tau    = 1.0 / (2.0 * (getval("j1xh")));
  if (getflag("PFGflg"))
	tau = tau - gt5;
   qpw = qtip*pw/90.0;

/* CHECK CONDITIONS */

   if ((dm[0] == 'y') || (dm[1] == 'y'))
   {
      fprintf(stdout, "Decoupler must be set as dm='nny'.\n");
      psg_abort(1);
   }
   if ((dmm[0] != 'c') || (dmm[1] != 'c'))
   {
      fprintf(stdout, "Decoupler must be set as dmm='ccf' or dmm='ccw'.\n");
      psg_abort(1);
   }


/* PHASECYCLE CALCULATIONS */
   settable(t3,2, phi3);
   settable(t4,8, phi4);
   settable(t9,16, rec_1);
   settable(t2,16,phi2);

   assign(zero,v1);		/* proton 90 */
   getelem(t2,ct,v2);		/* proton 180 */
   getelem(t3,ct,v3);		/* proton theta pulse */
   getelem(t4,ct,v4);		/* carbon 90 */
   getelem(t9,ct,oph);          /* Receiver */

   add(v4,one,v7);		/* 1st pair of carbon 180 */
   assign(v4,v5);		/* 2nd pair of carbon 180 */
   add(oph,one,v6);		/* carbon alpha pulse qphase=1 */

   if (qphase < -0.5)
	add(v6,two,v6);		/* carbon alpha phase qphase=-1 */

   add(one,v5,v10);
   add(one,v7,v11);

   add(oph,two,oph);
   if (getflag("sbbiflg"))
	sub(oph,two,oph);
	
/* ACTUAL PULSESEQUENCE BEGINS */
   status(A);
      decpower(pplvl);
      obspower(tpwr);

      if ((qphase != 0.0) && (qrelax > d1))
	{ 
	    decpower(dpwr);
	    status(C);
		delay(qrelax - d1);
	    status(A);
	    decpower(pplvl);
	}
      delay(d1);

   status(B);
	if (getflag("sbbiflg"))
	{
      	   if (qphase != 0.0) 
	   {
	   	rgpulse(qpw,v6,rof1,rof1);
	   	obspower(tpwr180);
	   	shaped_pulse(pw180ad,pw180,v7,rof1,rof1);
	   	obspower(tpwr);
	   	if (getflag("PFGflg"))
			zgradpulse(gzlvl5,gt5);
	   	delay(tau + pp + 2*rof1 + (2*qpw/PI));
	   	obspower(tpwr180);
	   	shaped_pulse(pw180ad,pw180,v7,rof1,rof1);
	   	obspower(tpwr);
	   }
	
           decrgpulse(pp, v1,rof1,rof1);
	   if (getflag("PFGflg"))
		zgradpulse(gzlvl5,gt5);
	   delay(tau - pp - 2*rof1);
           simpulse(pw, 2.0 * pp, v4, v2, rof1, rof1);
	   if (getflag("PFGflg"))
		zgradpulse(gzlvl5,gt5);
	   delay(tau - pp - 2*rof1 - mult*pp);
	   decrgpulse(mult*pp,v3,rof1,rof1);

	   obspower(tpwr180);
	   shaped_pulse(pw180ad,pw180,v5,rof1,rof1);
	   obspower(tpwr);
	   if (getflag("PFGflg"))
           	zgradpulse(gzlvl5,gt5);
	   decpower(dpwr);
	   delay(tau - POWER_DELAY);
	   obspower(tpwr180);
	   shaped_pulse(pw180ad,pw180,v5,rof1,rof2);
	   obspower(tpwr);
	}
	else
	{
	   if (qphase != 0.0)
	   {
           	rgpulse(qpw,v6,rof1,rof1);
           	if (getflag("PFGflg"))
                    zgradpulse(gzlvl5,gt5);
           	delay(tau - rof1 - (2*qpw/PI));
           	rgpulse(pw,v7,rof1,rof1);
           	simpulse(2*pw,pp,v11,v1,rof1,rof1);
           	rgpulse(pw,v7,rof1,rof1);
	   }
	   else
	      	decrgpulse(pp,v1,rof1,rof1);
           if (getflag("PFGflg"))
                zgradpulse(gzlvl5,gt5);
	   if (qphase !=0.0)
           	delay(tau - pp - rof1);
	   else
		delay(tau - pp - (2*pp/PI) - 2*rof1);
           simpulse(pw, 2.0 * pp, v4, v2, rof1, rof1);
           if (getflag("PFGflg"))
                zgradpulse(gzlvl5,gt5);
           delay(tau - pp - rof1 - (2*pw/PI));
           rgpulse(pw,v5,rof1,rof1);
           simpulse(2*pw,mult*pp,v10,v3,rof1,rof1);
           rgpulse(pw,v5,rof1,rof1);
           if (getflag("PFGflg"))
                zgradpulse(gzlvl5,gt5);
           decpower(dpwr);
           delay(tau - POWER_DELAY + rof2);
	}

   status(C);
}
