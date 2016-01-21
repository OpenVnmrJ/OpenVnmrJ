/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rna_zdipsitocsy - zfiltered tocsy with dipsi2 spinlock */

#include <standard.h>

/* Chess - CHEmical Shift Selective Suppression */
static void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
  double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}


/* Wet4 - Water Elimination */
static void Wet4(pulsepower,wetshape,duration,phaseA,phaseB)
  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* wetshape;
{
  double wetpw,finepwr,gzlvlw,gtw,gswet;
  gzlvlw=getval("gzlvlw"); gtw=getval("gtw"); gswet=getval("gswet");
  wetpw=getval("wetpw");
  finepwr=pulsepower-(int)pulsepower;     /* Adjust power to 152 deg. pulse*/
  pulsepower=(double)((int)pulsepower);
  if (finepwr == 0.0) {pulsepower=pulsepower+5; finepwr=4095.0; }
  else {pulsepower=pulsepower+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(pulsepower);         /* Set to low power level        */
  Chess(finepwr*0.6452,wetshape,wetpw,phaseA,20.0e-6,rof1,gzlvlw,gtw,gswet);
  Chess(finepwr*0.5256,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4928,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
}


dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5;
        slpw5 = getval("pw")/18.0;

        rgpulse(64*slpw5,phse1,0.0,0.0);
        rgpulse(82*slpw5,phse2,0.0,0.0);
        rgpulse(58*slpw5,phse1,0.0,0.0);
        rgpulse(57*slpw5,phse2,0.0,0.0);
        rgpulse(6*slpw5,phse1,0.0,0.0);
        rgpulse(49*slpw5,phse2,0.0,0.0);
        rgpulse(75*slpw5,phse1,0.0,0.0);
        rgpulse(53*slpw5,phse2,0.0,0.0);
        rgpulse(74*slpw5,phse1,0.0,0.0);

}
/*******************************************************************/

static int ph1[16]  = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
           ph2[16]  = {2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1},                 
           ph3[8]  =  {0,0,2,2,1,1,3,3},                           
           ph4[8]  =  {0,2,2,0,1,3,3,1};

pulsesequence()
{
   double          p1lvl,
                   slpw5,
                   mix,compH,
                   wetpwr,dz,wetpw,gtw,gswet,
		   gzlvl3,
		   gzlvl2,
		   gt2,
		   gstab,
                   cycles;
   int             iphase;
   char            wetshape[MAXSTR],autosoft[MAXSTR],wet[MAXSTR],sspul[MAXSTR];

/* LOAD AND INITIALIZE VARIABLES */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
   getstr("wetshape", wetshape);
   getstr("autosoft",autosoft);
   compH=getval("compH");
   dz=getval("dz");
   getstr("wet", wet);
   wetpwr=getval("wetpwr");
   mix = getval("mix");
   p1lvl = getval("p1lvl");
   slpw5 = pw/18;
   gzlvl3 = getval("gzlvl3");
   gzlvl2 = getval("gzlvl2");
   gt2 = getval("gt2");
   gstab=getval("gstab");

   getstr("sspul",sspul);
   iphase = (int) (getval("phase") + 0.5);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
   cycles = (mix/(2072*slpw5));
   cycles = 2.0*(double)(int)(cycles/2.0);
   initval(cycles, v9);                      /* V9 is the MIX loop count */
 
   settable(t1,16,ph1);
   settable(t2,16,ph2);
   settable(t3,8,ph3);
   settable(t4,8,ph4);

   getelem(t1,ct,v1);
   getelem(t4,ct,oph);
   getelem(t3,ct,v3);
   
   assign(one,v2);
   add(two, v2, v4);

   if (iphase == 2)
      incr(v1);

   add(v1, v14, v1);
   add(oph,v14,oph);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);
     obspower(p1lvl);
     delay(5.0e-6);
     if (sspul[0] == 'y')
      {
       zgradpulse(2000.0,.002);
       rgpulse(p1,zero,rof1,rof1);
       zgradpulse(3000.0,0.002);
      }
     delay(d1);
     if ((satmode[A] == 'y') && (wet[A] == 'y'))
      {
       printf(" select either presat or wet, not both");
       psg_abort(1);
      }
     if (satmode[A] == 'y') 
      {
       obspower(satpwr);
       rgpulse(satdly,zero,rof1,rof1);
       obspower(p1lvl);
      }
     if (wet[A] == 'y')
      {
      if (autosoft[A] == 'y') 
       { 
           /* selective H2O one-lobe sinc pulse */
        wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
        wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
        Wet4(wetpwr,"H2Osinc",wetpw,zero,one); delay(dz); 
        obspower(p1lvl);
       } 
      else
       Wet4(wetpwr,wetshape,wetpw,zero,one); delay(dz); 
       delay(1.0e-4);
       obspower(p1lvl);
      }

   status(B);
      rgpulse(p1, v1, rof1, 2.0e-6);
      if (d2 > 0.001)
       {
        zgradpulse(gzlvl3,0.4*d2);
        delay(0.1*d2 - 2*GRADIENT_DELAY - 2e-6 - (2*p1/PI));
        zgradpulse(-gzlvl3,0.4*d2);
        delay(0.1*d2 - 2*GRADIENT_DELAY - 2e-6 - (2*p1/PI));
       }
      else
       delay(d2);
      rgpulse(p1,t2,2.0e-6,0.5e-6);
      obspower(tpwr);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
     if (cycles > 1.0)
      {
         rcvroff();
         starthardloop(v9);
             dipsi(v2,v4); dipsi(v4,v2); dipsi(v4,v2); dipsi(v2,v4);
         endhardloop();
       }
      obspower(p1lvl);
      zgradpulse(-gzlvl2,gt2);
      rcvron();
statusdelay(C,gstab);
      rgpulse(p1,v3,rof1,rof2);
}

