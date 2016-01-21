// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* wetROESY - transverse cross-relaxation experiment in rotating frame */

#include <standard.h>

static int ph1[4] = {1,3,2,0},
           ph2[8] = {1,1,2,2,3,3,0,0},
           ph3[8] = {1,3,2,0,3,1,0,2},
           ph4[8] = {1,1,2,2,3,3,0,0},
           ph5[4] = {3,3,0,0},
           ph6[8] = {1,1,2,2,3,3,0,0};

pulsesequence()
{
   double          slpwr,
                   slpw,
                   mix,
                   hsglvl,
                   hsgt,
                   gzlvlz,
                   gtz,
		   zfpw,
		   zfpwr,
                   cycles;
   int             iphase;
   char            sspul[MAXSTR],
		   composit[MAXSTR],
		   compshape[MAXSTR],
		   zfilt[MAXSTR],
		   zfshp[MAXSTR];


/* LOAD AND INITIALIZE PARAMETERS */
   mix = getval("mix");
   iphase = (int) (getval("phase") + 0.5);
   slpwr = getval("slpwr");
   slpw = getval("slpw");
   getstr("sspul", sspul);
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   gzlvlz = getval("gzlvlz");
   gtz = getval("gtz");
   zfpwr = getval("zfpwr");
   zfpw = getval("zfpw");
   getstr("zfshp",zfshp);
   getstr("zfilt",zfilt);
   getstr("composit",composit);
   getstr("compshape",compshape);

   sub(ct,ssctr,v7);

   settable(t1,4,ph1);	getelem(t1,v7,v1);
   settable(t2,8,ph2);	getelem(t2,v7,v2);	add(v2,two,v3);
   settable(t3,8,ph3);	getelem(t3,v7,oph);
   settable(t4,8,ph4);	getelem(t4,v7,v4);
   settable(t5,4,ph5);	getelem(t5,v7,v5);
   settable(t6,8,ph6);	getelem(t5,v7,v6);
   
   if (zfilt[0] == 'n') assign(v1,oph);

   if (iphase == 2)
      {incr(v1); incr(v6);}			/* hypercomplex method */

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v13);
       add(v1,v13,v1);
       add(v6,v13,v6);
       add(oph,v13,oph);

   cycles = mix / (4.0 * slpw);
   initval(cycles, v10);	/* mixing time cycles */


/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);
      delay(5.0e-6);
   if (sspul[0] == 'y')
   {
         zgradpulse(hsglvl,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
   }

   delay(d1);

	if (getflag("wet"))
		wet4(zero,one);
   status(B);
      rgpulse(pw, v1, rof1, rof1);
      if (d2 > (POWER_DELAY + (2.0*pw/PI) + rof1))
       delay(d2 - POWER_DELAY - (2.0*pw/PI) - rof1);
      
      else {
	if (ix == 1)
	  dps_show("delay",d2);
	else if ((ix > 2) && (iphase < 2))
	  text_error("increment %d cannot be timed properly\n", (int) ix/2);
      }

      obspower(slpwr);

      if (cycles > 1.5000)
       {
	 obsunblank(); xmtron();
         starthardloop(v10);
		txphase(v2);
		delay(2*slpw);
		txphase(v3);
		delay(2*slpw);
         endhardloop();
	 xmtroff(); obsblank();
       }

       if (zfilt[0] == 'y')
        {
           obspower(tpwr);
           rgpulse(pw,v4,1.0e-6,rof1);
           zgradpulse(gzlvlz,gtz);
           delay(gtz/3);
           obspower(zfpwr);
           shaped_pulse(zfshp,zfpw,zero,2.0e-6,2.0e-6);
           zgradpulse(gzlvlz/4,gtz/3);
           obspower(tpwr);
           delay(gtz/8);
	   if (composit[0] == 'y')
    	   {
       		  if (rfwg[OBSch-1] == 'y')
       		   shaped_pulse(compshape,4.0*pw+0.8e-6,v5,rof1,rof2);
       		  else
       		   comp90pulse(pw,v5,rof1,rof2);
    	   }
           else
           	  rgpulse(pw,v5,rof1,rof2);
        }
       else
           delay(rof2);

   status(C);
}
