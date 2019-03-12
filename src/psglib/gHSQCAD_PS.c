// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* gHSQCAD_PS - Gradient Selected phase-sensitive HSQC
		Adiabatic version
                With real-time PureShift acquisition option developed by the
                         group of Prof. G.A. Morris at Manchester University

The sequence is based on the following publications:
         1, L Paudel et al. Angewandte Chemie Int. Ed. 2013, 52, 11616-11619
                     (real-time pure shift acquisition)
         2, R.D. Boyer et al. JMR 2003, 165, 253-259
                     (CRISIS-gHSQCAD)
         3, Garbow et al. Chem. Phys. Letter, 1982, 93, 504-509
                     (BIRD)

        Paramters:
                sspul  	   :  selects magnetization randomization option
                nullflg	   :  selects TANGO-Gradient option
                hsglvl     :   Homospoil gradient level (DAC units)
                hsgt       :  Homospoil gradient time
                gzlvlE     :  encoding Gradient level
                gtE        :  encoding gradient time
                EDratio    :  Encode/Decode ratio
                gstab      :  recovery delay
                j1xh       :  One-bond XH coupling constant
                mult       :  multiplicity editing
				0 - no editing
				1 - XH only
				2 - XH/XH3 up, XH2 down
                BIRD	   :  'y' - real time PureShift acquisition
			      'n' - impicit acquition, no decoupling
		BIRDmode   :  'h' - rectangulat refocussing BIRD pulses
			      'b' - BIP refocusing pulses (user enterable)
		shp_XBIRD  :  shape of the X-BIRD inversion pulse
                pwr_XBIRD  :  power level of the shp_XBIRD inversion pulse
                pw_XBIRD   :  ipulse width of the shp_XBIRD inversion pulse
                shp_HBIRD  :  shape of the H-BIRD inversion pulse
                pwr_HBIRD  :  power level of the shp_HBIRD inversion pulse
                pw_HBIRD   :  ipulse width of the shp_HBIRD inversion pulse
		npoints    :  number of data points acquired in a chunk
                                  (np must be an integer multiple of npoints)
		chunkwidth :  lenght of a single chunk
		tauA       :  compensation for tauB and tauD
                tauB       :  effect of rof2
                tauD       :  effect of alfa
                gtcr       :  crusher gradient duration
                gzlvlcr    :  crusher gradient amplitude
                nt         :  number of transient per increment
                              2 recommended minimun
                              4 for proper F1 quad-image cancellation

Please note that this sequence during acquisition mixes C12 and C13 attached 
protons, therefore for proper spectral cleanness nt=4 is recommended.
The sequence is fully compatible with NUS.

KrishK  -       Last revision   : June 1997
KrishK  -       Revised         : July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

/*------------------------------------------
Phase tables for gHSQC
--------------------------------------------*/
static int ph1[4]  = {1,1,3,3},  //v1 - proton 90 at the end of first inept
           ph2[2]  = {0,2},   //v2 - X 90 at the end of first inept
           ph3[8]  = {0,0,0,0,2,2,2,2},  //v3 - proton 90 in 2nd inept
           ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2}, //v4 - X 90 in 2nd inept
           ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1}; //oph

/*------------------------------------------
Phase tables for rtgHSQC-BIRD
---------------------------------------------*/
static int      ph11[8]  = {1,1,1,1,3,3,3,3},  //v1
                ph12[2]  = {0,2},              //v2
                ph13[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},  //v3
                ph14[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},  //v4
                ph15[32] = {1,3,1,3,3,1,3,1,3,1,3,1,1,3,1,3,
                            3,1,3,1,1,3,1,3,1,3,1,3,3,1,3,1},  //oph  
                ph17[4]  = {0,0,1,1},  //v7 - 1st 90 of bird and the hard 180
                ph18[4]  = {1,1,2,2},  //v8 - simpulse 180 of bird
                ph19[4]  = {2,2,3,3};  //v9 - 2nd 90 of bird 

void pulsesequence()
{
double  gzlvlE = getval("gzlvlE"),
        gtE = getval("gtE"),
        EDratio = getval("EDratio"),
        gstab = getval("gstab"),
        mult = getval("mult"),
	pwx180 = getval("pwx180"),
	pwxlvl180 = getval("pwxlvl180"),
	pwx180r = getval("pwx180r"),
	pwxlvl180r = getval("pwxlvl180r"),
        hsglvl = getval("hsglvl"),
        hsgt = getval("hsgt"),
        tauA=getval("tauA"),        //compensation for tauB and tauD
        tauB=getval("tauB"),        //effect of rof2
        tauD=getval("tauD"),        //effect of alfa
        tBal=getval("tBal"),        //supports inova console if ~1/(fb*1.3)
        pwr_XBIP = getval("pwr_XBIP"),
        pw_XBIP = getval("pw_XBIP"),
        pwr_HBIP = getval("pwr_HBIP"),
        pw_HBIP = getval("pw_HBIP"),
        gzlvlcr = getval("gzlvlcr"),
        gtcr = getval("gtcr"),        //crusher gradient in BIRD
        npoints = getval("npoints"),  // npoints should be an integer multiple of np
        tau, evolcorr, taug,cycles;
int     prgcycle = (int)(getval("prgcycle")+0.5),
	phase1 = (int)(getval("phase")+0.5),
	icosel, ZZgsign;
char	pwx180ad[MAXSTR], pwx180adR[MAXSTR], pwx180ref[MAXSTR],
        shp_XBIP[MAXSTR], shp_HBIP[MAXSTR], BIRD[MAXSTR], BIRDmode[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

  getstr("pwx180ad", pwx180ad);
  getstr("pwx180adR", pwx180adR);
  getstr("pwx180ref", pwx180ref);
  getstr("shp_XBIP",shp_XBIP);
  getstr("shp_HBIP",shp_HBIP);
  getstr("BIRD",BIRD);
  getstr("BIRDmode",BIRDmode);

  tau = 1 / (4*(getval("j1xh")));
  evolcorr = (4*pwx/PI)+2*pw+8.0e-6;
  cycles=np/npoints;
  cycles = (double)((int)((cycles)));
  initval(cycles,v20);

  if (mult > 0.5)
    taug = 2*tau + getval("tauC");
  else
    taug = gtE + gstab + 2 * GRADIENT_DELAY;
  ZZgsign=-1; 
  if (mult == 2) ZZgsign=1;
  icosel = 1;
 
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
 
     if (BIRD[0]=='n')          //gHSQC phases
        {
                settable(t1,4,ph1);
                settable(t2,2,ph2);
                settable(t3,8,ph3);
                settable(t4,16,ph4);
                settable(t5,16,ph5);
        }
     else                      //rtgHSQC-BIRD phases
        {
                settable(t1,8,ph11);
                settable(t2,2,ph12);
                settable(t3,16,ph13);
                settable(t4,32,ph14);
                settable(t5,32,ph15);
                settable(t7,4,ph17);
                settable(t8,4,ph18);
                settable(t9,4,ph19);
                getelem(t7, v17, v7);
                getelem(t8, v17, v8);
                getelem(t9, v17, v9);
        }

     getelem(t1, v17, v1);
     getelem(t3, v17, v3);
     getelem(t4, v17, v4);
     getelem(t2, v17, v2);
     getelem(t5, v17, oph);

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

  if ((phase1 == 2) || (phase1 == 5))
    icosel = -1;

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
  add(v2, v14, v2);
  add(oph, v14, oph);

status(A);
   obspower(tpwr); decpower(pwxlvl);
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
                shaped_satpulse("relaxD",satdly,zero);
                if (getflag("prgflg"))
                   shaped_purge(v6,zero,v18,v19);
           }
        else
           {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v6,zero,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

  status(B);
              /****** null flag starts here *****/
    if (getflag("nullflg"))
    {
      rgpulse(0.5 * pw, zero, rof1, rof1);
      delay(2 * tau);
      decpower(pwxlvl180);
      decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
      rgpulse(2.0 * pw, zero, rof1, rof1);
      delay(2 * tau + 2 * POWER_DELAY);
      decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
      decpower(pwxlvl);
      rgpulse(1.5 * pw, two, rof1, rof1);
      zgradpulse(hsglvl, hsgt);
      delay(1e-3);
    }

/*********gHSQC or gHSQC part of pure shift starts here *****/

    if (getflag("cpmgflg"))
    {
      rgpulse(pw, v6, rof1, 0.0);
      cpmg(v6, v15);
    }
    else
      rgpulse(pw, v6, rof1, rof1);
    delay(tau);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    rgpulse(2.0 * pw, zero, rof1, rof1);
    delay(tau + 2 * POWER_DELAY);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(pwxlvl);
    rgpulse(pw, v1, rof1, rof1);
    zgradpulse(hsglvl, 2 * hsgt);
    delay(1e-3);
    decrgpulse(pwx, v2, rof1, 2.0e-6);

      delay(d2 / 2);
    rgpulse(2 * pw, zero, 2.0e-6, 2.0e-6);
      delay(d2 / 2);

    delay(taug - POWER_DELAY);
    if (mult > 0.5)
    {
    decpower(pwxlvl180r);
    decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
    rgpulse(mult * pw, zero, rof1, rof1);
    delay(taug - mult * pw - 2 * rof1 + POWER_DELAY - gtE - gstab - 2 * GRADIENT_DELAY+evolcorr);
    zgradpulse(gzlvlE, gtE);
    delay(gstab);
    decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
    }
    else
    {
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    delay(taug + POWER_DELAY - gtE - gstab - 2 * GRADIENT_DELAY+evolcorr);
    zgradpulse(gzlvlE, gtE);
    delay(gstab);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    }

    decpower(pwxlvl);

    decrgpulse(pwx, v4, 2.0e-6, rof1);
    zgradpulse(ZZgsign*0.6 * hsglvl, 1.2 * hsgt);
    delay(1e-3);
    rgpulse(pw, v3, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau - (2 * pw / PI) - 2*rof1);
    rgpulse(2 * pw, zero, rof1, rof2);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    zgradpulse(icosel * 2.0*gzlvlE/EDratio, gtE/2.0);
    delay(tau - gtE/2.0 - 2 * GRADIENT_DELAY);

/********gHSQC part stops and BIRD Acquisition starts here*************/
    // delay(tBal);
        //filter delay (Hoult) for inova; adjust tBal manually for the same effect
        //delay(1.0/(getval("fb")*1.3))

        if (BIRD[0]=='y')
        {
#ifdef NVPSG
                setacqmode(WACQ|NZ);    //use this line only for vnmrs console; comment this out in inova
#endif
                obsblank();
//                delay(rof2);
                startacq(alfa);
        }

/*----------------------------------------------------------------------------
         Observe the 1st half chunk
-----------------------------------------------------------------------------*/

        if (BIRD[0]=='y')
        {
                status(C);
                acquire(npoints/2.0,1.0/sw);
                rcvroff();                                            
                status(B);
                obspower(tpwr);

/*------------------------------------------------------------------------
         Using hard 13C inversion pulse in BIRD
--------------------------------------------------------------------------*/

                if (BIRDmode[0]== 'h')
                {
                        rgpulse(pw,v7,rof1,rof1);
                        decpower(pwxlvl);
                        delay(tau);        
                        zgradpulse(gzlvlcr,gtcr);
                        delay(tau-gtcr);
                        simpulse(2.0*pw,2.0*pwx,v8,v8,rof1,rof1);
                        decpower(dpwr);
                        delay(tau);
                        zgradpulse(gzlvlcr,gtcr);
                        delay(tau-gtcr);
                        rgpulse(pw,v9,rof1,rof1);
                }

/*---------------------------------------------------------------------------
        Using BIP 13C inversion pulse in BIRD
----------------------------------------------------------------------------*/

                if (BIRDmode[0]== 'b')
                {
                  rgpulse(pw,v7,rof1,rof1);                     
                  if (pwr_XBIP!=pwxlvl) decpower(pwr_XBIP); else decpower(pwxlvl);
                  if (pwr_HBIP!=tpwr)  obspower(pwr_HBIP);
                  delay(tau); 
                  zgradpulse(gzlvlcr,gtcr);
                  delay(tau-gtcr);                              
                  simshaped_pulse(shp_HBIP,shp_XBIP,2.0*pw,pw_XBIP,v8,v8,rof1,rof1);
                  if (pwr_HBIP!=tpwr) obspower(tpwr);
                  decpower(dpwr);
                  delay(tau);
                  zgradpulse(gzlvlcr,gtcr);
                  delay(tau-gtcr);
                  rgpulse(pw,v9,rof1,rof1);
                }
                
                delay(tauA);
                rgpulse(pw*2.0,v7,rof1,rof1);  // hard 180 degree refocusing pulse
                obsblank();
                delay(tauB);
                rcvron();       //this includes rof3
                delay(tauD);
                decr(v20);

/*------------------------------------------------------------------------------
                Loops for more chunks
------------------------------------------------------------------------------*/

                starthardloop(v20);
                status(C);
                        acquire(npoints,1.0/sw);
                        rcvroff();                                    
                status(B);
                        obspower(tpwr);                               

/*------------------------------------------------------------------------                                              
Using hard 13C inversion pulse in BIRD
--------------------------------------------------------------------------*/
                if (BIRDmode[0]== 'h')
                {
                        rgpulse(pw,v7,rof1,rof1);
                        decpower(pwxlvl);                             
                        delay(tau);         
                        zgradpulse(gzlvlcr,gtcr);
                        delay(tau-gtcr);                      
                        simpulse(2.0*pw,2.0*pwx,v8,v8,rof1,rof1);
                        decpower(dpwr);
                        delay(tau);
                        zgradpulse(gzlvlcr,gtcr);
                        delay(tau-gtcr);
                        rgpulse(pw,v9,rof1,rof1);
                }
/*---------------------------------------------------------------------------
Using BIP 13C inversion pulse in BIRD
----------------------------------------------------------------------------*/
//                if (BIRDmode[0]== 'b')
                if (BIRDmode[0]== 'b')
                {
                        rgpulse(pw,v7,rof1,rof1);                     
                        if (pwr_XBIP!=pwxlvl) decpower(pwr_XBIP); else decpower(pwxlvl);
                        delay(tau);      
                        zgradpulse(gzlvlcr,gtcr);
                        delay(tau-gtcr);                         
                        simshaped_pulse(shp_HBIP,shp_XBIP,2*pw,pw_XBIP,v8,v8,rof1,rof1);
                        decpower(dpwr);
                        delay(tau);
                        zgradpulse(gzlvlcr,gtcr);
                        delay(tau-gtcr);
                rgpulse(pw,v9,rof1,rof1);
                }

                delay(tauA);
                rgpulse(pw*2.0,v7,rof1,rof1);            // hard 180 degree refocusing pulse
                obsblank();
                delay(tauB);
                rcvron();       //this includes rof3
                delay(tauD);

                endhardloop();

/*----------------------------------------------------------------
                Acquisition of last half chunk
----------------------------------------------------------------*/
                status(C);
                        acquire(npoints/2.0,1.0/sw);
                        rcvroff();                                    
                        endacq();
                        incr(v20);
        }
                 /****** BIRD ends here for all *****/
/***************** ACQ for conventional gHSQC ******************************/
        else
        status(C);
}
/**************** PULSE SEQUENCE ENDS HERE**********************************/

