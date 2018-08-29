// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* wetTOCSY - MLEV17c spinlock */

#include <standard.h>

mleva()
{
   double slpw;
   slpw = getval("slpw");
   txphase(v2); delay(slpw);
   xmtroff(); delay(slpw); xmtron();
   txphase(v3); delay(2*slpw); 
   xmtroff(); delay(slpw); xmtron();
   txphase(v2); delay(slpw);
}

mlevb()
{
   double slpw;
   slpw = getval("slpw");
   txphase(v4); delay(slpw);
   xmtroff(); delay(slpw); xmtron();
   txphase(v5); delay(2*slpw);
   xmtroff(); delay(slpw); xmtron();
   txphase(v4); delay(slpw);
}

static int ph1[4] = {0,0,1,1},   		/* presat */
           ph2[4] = {1,3,2,0},                  /* 90 */
           ph3[8] = {1,3,2,0,3,1,0,2},          /* receiver */
           ph4[4] = {0,0,1,1},                  /* trim */
           ph5[4] = {1,1,2,2},                  /* spin lock */
	   ph7[8] = {1,1,2,2,3,3,0,0},
	   ph8[4] = {3,3,0,0};
	   
pulsesequence()
{
   double          slpwr,
                   slpw,
                   trim,
                   mix,
		   hsglvl,
		   hsgt,
		   gzlvlz,
		   gtz,
		   zfphinc,
		   zfpwr,
		   zfpw,
                   cycles,
                   phase;
   int             iphase;
   char            sspul[MAXSTR],
                   zfilt[MAXSTR],
		   zfshp[MAXSTR],
		   composit[MAXSTR],
		   compshape[MAXSTR];


/* LOAD AND INITIALIZE VARIABLES */
   mix = getval("mix");
   slpwr = getval("slpwr");
   slpw = getval("slpw");
   trim = getval("trim");
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   gzlvlz = getval("gzlvlz");
   gtz = getval("gtz");
   zfpwr = getval("zfpwr");
   zfpw = getval("zfpw");
   zfphinc = getval("zfphinc");
   getstr("composit",composit);
   getstr("compshape",compshape);
   getstr("zfshp",zfshp);
   phase = getval("phase");
   iphase = (int) (phase + 0.5);
   getstr("sspul", sspul);
   getstr("zfilt",zfilt);
   
   if (zfphinc < 0) zfphinc=360+zfphinc;
   initval(zfphinc,v10);
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
  
      cycles = (mix - trim ) / (96.66*slpw);
      cycles = 2.0*(double) (int) (cycles/2.0);
      initval(cycles, v9);                      /* V9 is the MIX loop count */
 
   			sub(ct,ssctr,v12);
   settable(t1,4,ph1); 	getelem(t1,v12,v6);
   settable(t2,4,ph2); 	getelem(t2,v12,v1);
   settable(t3,8,ph3); 	getelem(t3,v12,oph);
   settable(t4,4,ph4); 	getelem(t4,v12,v13);
   settable(t5,4,ph5); 	getelem(t5,v12,v2);
   settable(t7,8,ph7); 	getelem(t7,v12,v7);
   settable(t8,4,ph8); 	getelem(t8,v12,v8);   

   if (zfilt[0] == 'n') assign(v1,oph);
      
   sub(v2, one, v3);
   add(two, v2, v4);
   add(two, v3, v5);

   if (iphase == 2)
      {incr(v1); incr(v6);}

   add(v1, v14, v1);
   add(v6, v14, v6);
   add(oph,v14,oph);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
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
      obspower(slpwr);
      txphase(v13);
      if (d2 > (rof1 + POWER_DELAY + 2.0*pw/PI))
       delay(d2 - rof1 - POWER_DELAY - (2.0*pw/PI));
	
      else {
	if (ix == 1)
	  dps_show("delay",d2);
	else if ((ix > 2) && (phase < 1.5))
	  text_error("increment %d cannot be timed properly\n", (int) ix/2);
      }

      if (cycles > 1.0)
      { 
	obsunblank(); xmtron();
	delay(trim);
         starthardloop(v9);
            mleva(); mleva(); mlevb(); mlevb();
            mlevb(); mleva(); mleva(); mlevb();
            mlevb(); mlevb(); mleva(); mleva();
            mleva(); mlevb(); mlevb(); mleva();
            txphase(v3); delay(0.66*slpw);
         endhardloop();
	xmtroff(); obsblank();
       }
	
       if (zfilt[0] == 'y')
        {
           obspower(tpwr);
           rgpulse(pw,v7,1.0e-6,rof1);
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
                   shaped_pulse(compshape,4.0*pw+0.8e-6,v8,rof1,rof2);
                  else
                   comp90pulse(pw,v8,rof1,rof2);
           }
           else
             rgpulse(pw,v8,rof1,rof2);
        }
       else
           delay(rof2);
           
   status(C);
}
