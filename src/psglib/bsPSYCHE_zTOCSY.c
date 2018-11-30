// Chempacker modified sequence
/*   PSYCHE_zTOCSY
     F1-PSYCHE-TOCSY experiment
 
  Reference:
	Foroozandeh, M; Adams RW; Nilsson, M; Morris, GA, J. Am. Chem. Soc. 2014, 136, 11867.

  Spectrum is Pure Shift in F1
  Additional indirect covariance processing (foldt('covar') in vnmrj) yields double Pure Shift spectrum

*/

#include <standard.h>
#include <chempack.h>

static int	ph1[2] = {0,2},
		ph2[1] = {0},
		ph3[8] = {0,0,1,1,2,2,3,3},
		ph4[1] = {0},
		ph5[1] = {0},
		ph6[1] = {0},
		ph7[1] = {0},
		ph8[1] = {0},
		ph9[8] = {2,0,0,2,2,0,0,2};

pulsesequence()
{

   double	   gt1 = getval("gt1"),
		   gzlvl1=getval("gzlvl1"),
		   gt2 = getval("gt2"),
		   gzlvl2=getval("gzlvl2"),
		   gt3 = getval("gt3"),
		   gzlvl3=getval("gzlvl3"),
		   gt4 = getval("gt4"),
		   gzlvl4=getval("gzlvl4"),
		   selpwrPS = getval("selpwrPS"),
		   selpwPS = getval("selpwPS"),
		   gzlvlPS = getval("gzlvlPS"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
		   slpwrT = getval("slpwrT"),
                   slpwT = getval("slpwT"),
                   mixT = getval("mixT"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   zqfpw2 = getval("zqfpw2"),
                   zqfpwr2 = getval("zqfpwr2"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   gzlvlzq2 = getval("gzlvlzq2"),
		   gstab = getval("gstab");

   int 		   prgcycle=(int)(getval("prgcycle")+0.5),
	           phase1 = (int)(getval("phase")+0.5);

   char		   selshapePS[MAXSTR],
		   slpatT[MAXSTR],
                   zqfpat1[MAXSTR],
                   zqfpat2[MAXSTR],
                   selshapeA[MAXSTR],
                   selshapeB[MAXSTR];

/* LOAD AND INITIALIZE VARIABLES */
   getstr("slpatT",slpatT);
   getstr("zqfpat1",zqfpat1);
   getstr("zqfpat2",zqfpat2);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("selshapePS",selshapePS);

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);


//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt1 = syncGradTime("gt1","gzlvl1",1.0);
        gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
	gt2 = syncGradTime("gt2","gzlvl2",1.0);
        gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   assign(ct,v17);
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   settable(t1,2,ph1);
   settable(t2,1,ph2);
   settable(t3,8,ph3);
   settable(t4,1,ph4);
   settable(t5,1,ph5);
   settable(t6,1,ph6);
   settable(t7,1,ph7);
   settable(t8,1,ph8);
   settable(t9,8,ph9);

   getelem(t1,v17,v1);
   getelem(t2,v17,v2);
   getelem(t3,v17,v3);
   getelem(t4,v17,v4);
   getelem(t5,v17,v5);
   getelem(t6,v17,v6);
   getelem(t7,v17,v7);
   getelem(t8,v17,v8);
   getelem(t9,v17,v9);
   assign(v9,oph);

   assign(v1,v11);
   add(v11,two,v12);

   if (phase1 == 2)
      {incr(v1); }

   add(v1, v14, v1);
   add(oph,v14,oph);

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

      delay(d1);

   status(B);
      obspower(tpwr);
      rgpulse(pw, v1, rof1, rof1);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v11,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v12,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

	delay(d2/2.0); 

	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	rgpulse(2*pw,v2,rof1,rof1);
	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	zgradpulse(gzlvl2,gt2);
	delay(2.0*gstab);
	obspower(selpwrPS);
	rgradient('z',gzlvlPS);
	shaped_pulse(selshapePS,selpwPS,v3,rof1,rof1);
	rgradient('z',0.0);
	delay(gstab);
	obspower(tpwr);
	zgradpulse(gzlvl2,gt2);
	delay(gstab);

	if (d2>0)
	delay(d2/2.0-2.0*pw/PI-2.0*rof1); 

	rgpulse(pw,v5,rof1,rof1);

        if (mixT > 0.0)
        {
           if (getflag("Gzqfilt"))
           {
            obspower(zqfpwr1);
            rgradient('z',gzlvlzq1);
            delay(100.0e-6);
            shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
            delay(100.0e-6);
            rgradient('z',0.0);
            delay(100e-6);
           }
           obspower(slpwrT);
           zgradpulse(gzlvl3,gt3);
           delay(gt3);
        
           if (dps_flag)
             rgpulse(mixT,v4,0.0,0.0);
           else
             SpinLock(slpatT,mixT,slpwT,v4);

           if (getflag("Gzqfilt"))
           {
            obspower(zqfpwr2);
            rgradient('z',gzlvlzq2);
            delay(100.0e-6);
            shaped_pulse(zqfpat2,zqfpw2,zero,rof1,rof1);
            delay(100.0e-6);
            rgradient('z',0.0);
            delay(100e-6);
           }
           obspower(tpwr);
           zgradpulse(gzlvl4,gt4);
           delay(gt4);
         }

        rgpulse(pw,v8,rof1,rof2);

   status(C);
}
