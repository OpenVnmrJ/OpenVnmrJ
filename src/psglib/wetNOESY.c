// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* wetNOESY - Gradient Selected through-space correlation experiment */

#include <standard.h>

static int	phs1[16] = {0,2,0,2,0,2,0,2,1,3,1,3,1,3,1,3},
		phs2[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                            2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
		phs3[16] = {0,0,1,1,2,2,3,3,1,1,2,2,3,3,0,0},
		phs4[32] = {0,2,1,3,2,0,3,1,1,3,2,0,3,1,0,2,
                            2,0,3,1,0,2,1,3,3,1,0,2,1,3,2,0};
		
pulsesequence()
{
   double          mix,
   		   pwwet,
		   gzlvlmix,
		   gsmix,
   		   gtw,
   		   gswet,
                   hsglvl,
                   hsgt;
   int             iphase;
   char            sspul[MAXSTR],
		   composit[MAXSTR],
		   compshape[MAXSTR],
		   wet[MAXSTR];

/* LOAD VARIABLES */
   mix = getval("mix");
   gzlvlmix = getval("gzlvlmix");
   gsmix = getval("gsmix");
   pwwet = getval("pwwet");
   gtw = getval("gtw");
   gswet = getval("gswet");
   iphase = (int) (getval("phase") + 0.5);
   getstr("sspul", sspul);
   getstr("wet",wet);
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   getstr("composit",composit);
   getstr("compshape",compshape);

   settable(t1,16,phs1);
   settable(t2,32,phs2);
   settable(t3,16,phs3);
   settable(t4,32,phs4);
   
   getelem(t1,ct,v1);
   getelem(t4,ct,oph);
   getelem(t2,ct,v2);
   getelem(t3,ct,v3);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (iphase == 2)                                        /* hypercomplex */
      incr(v1);

   add(v1, v14, v1);
   add(oph,v14,oph);
    

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   if (sspul[0] == 'y')
   {
         zgradpulse(hsglvl,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
   }

   delay(d1);

   if (wet[0] == 'y')
     wet4(zero,one);

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      if (d2 > (2.0*rof1 +4.0*pw/PI))
       delay(d2- 2.0*rof1 -(4.0*pw/PI));  /*corrected evolution time */
       
      else {
	if (ix == 1)
	  dps_show("delay",d2);
	else if ((ix > 2) && (iphase < 2))
	  text_error("increment %d cannot be timed properly\n", (int) ix/2);
      }
      rgpulse(pw, v2, rof1, rof1);

   status(C);
     if  (wet[2] == 'y')
      {
	zgradpulse(gzlvlmix,(mix - 4*pwwet - 4*gtw - 4*gswet - gsmix));
	delay(gsmix);
        wet4(zero,one);
      }
      else
        delay(mix);
   status(D);
      if (composit[0] == 'y')
    {
       if (rfwg[OBSch-1] == 'y')
          shaped_pulse(compshape,4.0*pw+0.8e-6,v3,rof1,rof2);
       else
          comp90pulse(pw,v3,rof1,rof2);
    }
      else
       rgpulse(pw, v3, rof1, rof2);

}
