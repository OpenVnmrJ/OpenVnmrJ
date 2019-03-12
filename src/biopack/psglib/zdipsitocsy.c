/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* zdipsitocsy - zfiltered tocsy with dipsi2 spinlock 

        pw = 90 degree excitation pulse (at power tpwr)
    satdly = length of presaturation;
  strength = spinlock field strength in Hz
    satmode  = flag for presat control
               'yn' for during relaxation delay 
               'yy' for during both "relaxation delay" and "d2" (recommended)
     phase = 1,2: for HYPERCOMPLEX phase-sensitive F1 quadrature
     sspul = 'y':  hs-90-hs sequence initiates D1 delay
             'n':  normal D1 delay
       mix = mixing time  (can be arrayed)

   mfsat='y'
           Multi-frequency saturation.
           Requires the frequency list mfpresat.ll in current experiment
           Pbox creates (saturation) shape "mfpresat.DEC"

                  use mfll('new') to initialize/clear the line list
                  use mfll to read the current cursor position into
                  the mfpresat.ll line list that is created in the
                  current experiment.

           Note: copying pars or fids (mp or mf) from one exp to another does not copy
                 mfpresat.ll!
           Note: the line list is limited to 128 frequencies !
            E.Kupce, Varian, UK June 2005 - added multifrequency presat 
                 option to tntocsy.c
            Josh Kurutz, U. of Chicago May 2006 - added mfsat to zdipsitocsy.c


           TRANSMITTER MUST BE POSITIONED AT SOLVENT FREQUENCY 
             this pulse sequence requires a T/R switch,
             linear amplifiers and computer-controlled attenuators on the
             observe channel.
         (G.Gray, Feb 2005)
*/ 
#include <standard.h>
#include <mfpresat.h>

void dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5;
        slpw5 =(1.0/(4.0*getval("strength")*18.0));

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
/* original phase cycle*/
/*static int ph1[16]  = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
           ph2[16]  = {2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1},                 
           ph3[8]  =  {0,0,2,2,1,1,3,3},                           
           ph4[8]  =  {0,2,2,0,1,3,3,1}; */

/* new phase cycle (From John Cavanaugh's book, as suggested by Joe Kurutz, U.Chicago
   for suppression of anti-diagonals) */
static int ph1[16] = {0,2,2,0,1,3,3,1,2,0,0,2,3,1,1,3},
           ph2[8]  = {0,0,2,2,1,1,3,3},                 
           ph3[1]  = {0},                           
           ph4[16] = {0,2,0,2,0,2,0,2,2,0,2,0,2,0,2,0};

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

void pulsesequence()
{
   double         
                   slpw5,slpwr,
                   mix,compH,
                   wetpwr,dz,wetpw,gtw,gswet,
		   gzlvl3,
		   gzlvl2,
		   gt2,
		   gstab,
                   strength,
                   cycles;
   int             iphase;
   char            wetshape[MAXSTR],autosoft[MAXSTR],wet[MAXSTR],sspul[MAXSTR],mfsat[MAXSTR];

/* LOAD AND INITIALIZE VARIABLES */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
   getstr("wetshape", wetshape);
   getstr("autosoft",autosoft);
   getstr("mfsat", mfsat);
   compH=getval("compH");
   strength=getval("strength");
   dz=getval("dz");
   getstr("wet", wet);
   wetpwr=getval("wetpwr");
   mix = getval("mix");
   slpw5 = 1.0/(4.0*strength*18);
   slpwr = 4095*(compH*pw*4.0*strength);
   slpwr =(int) (slpwr+0.5);
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
   settable(t2,8,ph2);
   settable(t3,1,ph3);
   settable(t4,16,ph4);

   getelem(t1,ct,v1);
   getelem(t4,ct,oph);
   getelem(t3,ct,v3);
   
   assign(one,v2);
   add(two, v2, v4);

   if (iphase == 2)
      incr(v1);

/*   add(v1, v14, v1);*/
   add(v3, v14, v3);
   add(oph,v14,oph);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);
     obspower(tpwr);
     delay(5.0e-6);
     if (sspul[0] == 'y')
      {
       zgradpulse(2000.0,.002);
       rgpulse(pw,zero,rof1,rof1);
       zgradpulse(3000.0,0.002);
      }
     if ((satmode[A] == 'y') && (wet[A] == 'y'))
      {
       printf(" select either presat or wet, not both");
       psg_abort(1);
      }
     if (satmode[A] == 'y') 
     {
      if (d1 > satdly) delay(d1 - satdly);
      rcvroff();
      if (mfsat[A] == 'y')
       {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off(); obsblank();}
      else
       {obspower(satpwr); rgpulse(satdly,zero,rof1,rof2);}
      obspower(tpwr);
      rcvron();
     }
     else delay(d1);
     
     if (wet[A] == 'y')
      {
      if (autosoft[A] == 'y') 
       { 
           /* selective H2O one-lobe sinc pulse */
        wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
        wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
        Wet4(wetpwr,"H2Osinc",wetpw,zero,one); delay(dz); 
        obspower(tpwr);
       } 
      else
       Wet4(wetpwr,wetshape,wetpw,zero,one); delay(dz); 
       delay(1.0e-4);
       obspower(tpwr);
      }

   status(B);
      rgpulse(pw, v1, rof1, 2.0e-6);
      if (d2>0.0)
      {
        if (d2 > 0.002)
         {
          zgradpulse(gzlvl3,0.4*d2);
          delay(0.1*d2 - 2*GRADIENT_DELAY - 2e-6 - (2*pw/PI));
          zgradpulse(-gzlvl3,0.4*d2);
          delay(0.1*d2 - 2*GRADIENT_DELAY - 2e-6 - (2*pw/PI));
         }
        else
         delay(d2 -4.0e-6 -(4*pw/PI));
      }
      rgpulse(pw,t2,2.0e-6,0.5e-6);
   status(C);
      obspwrf(slpwr);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
     if (cycles > 1.0)
      {
         rcvroff();
         starthardloop(v9);
             dipsi(v2,v4); dipsi(v4,v2); dipsi(v4,v2); dipsi(v2,v4);
         endhardloop();
       }
      obspwrf(4095.0);
      zgradpulse(-gzlvl2,gt2);
      delay(gstab);
      rcvron();
      status(D);
      rgpulse(pw,v3,rof1,rof2);
}



