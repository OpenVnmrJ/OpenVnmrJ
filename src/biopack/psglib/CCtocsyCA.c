/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  CCtocsyCA.c

    2D 13C observe 13C-13C TOCSY experiment.


    Uses constant time evolution for the 13C shifts. Set tn='C13', dn='H1', dn2='N15, dn3='H2'.

    Standard features include maintaining the 13C carrier in the Cab region
    throughout using off-res SLP pulses; square pulses on Cab with first
    null at 13CO.

 
    pulse sequence: A. Eletsky, O. Moreiera, H. Kovacs, and K. Pervushin
    JBNMR 26, 167-179 (2003).


    Must set phase = 1,2 for States-TPPI acquisition in t1 [13C].
    set dm2='nynn' dmm2='cgcc'  dm3='nyny' dmm3='cwcw'.

    Pulse sequence is optimized for VNMRS. Delays should be corrected for INOVA by subtracting
    WFG2_START_DELAY and WFG3_START_DELAY.

    02 November 2006
    N. Murali, Varian Palo Alto


*/



#include <standard.h>
#include "Pbox_bio.h"
  


static int  
                      

	     phi1[2]  = {0,2},
	     phi2[1]  = {0},
             phi3[1]  = {1},
             rec[2]   = {0,2};

static double   d2_init=0.0;
static double   C13ofs=46.0;
static shape    CO_dec, mix_seq;



pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */


 
int         t1_counter;  		        /* used for states tppi in t1 */


double      tau1,         				         /*  t1 delay */
	    TC = getval("TC"), 		   /* Constant delay 1/(JCC) ~ 13.5 ms */
            mix = getval("mix"),	   /* TOCSY mixing time */
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* 90 degree pulse at Cab (46ppm), first off-resonance null at CO (174ppm)    */
        pwC1,		              /* 90 degree pulse length on C13 at rf1 */
        rf1,		       /* fine power for 5.1 kHz rf for 600MHz magnet */

/* 180 degree pulse at Ca (46ppm), first off-resonance null at CO(174ppm)     */
        pwC2,		                    /* 180 degree pulse length at rf2 */
        rf2,		      /* fine power for 11.4 kHz rf for 600MHz magnet */


   compC = getval("compC"),       /* adjustment for C13 amplifier compression */




	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		     
	gzlvl1 = getval("gzlvl1"),

	gt2 = getval("gt2"),				   
	gzlvl2 = getval("gzlvl2"),
        gstab = getval("gstab"),
        ppm,
        co_ofs = getval("co_ofs"),	/* offset for C' */
        co_bw = getval("co_bw"),	/* bandwidth for C' */
        copwr = getval("copwr"),	/* power for C' decoupling. Get from CO_dec.DEC*/
	codmf = getval("codmf"),	/* dmf for C' decoupling. Get from CO_dec.DEC  */
        codres = getval("codres"), 	/* dres for C' decoupling. Get from CO_dec.DEC */
        mixbw,                          /* band width for mixing shape */
        mixpwr = getval("mixpwr"),   	/* power for CC mixing. Get from ccmix.DEC*/
        mixdmf= getval("mixdmf"),  	/* dmf for CC decoupling. Get from ccmix.DEC  */
        mixdres = getval("mixdres"); 	/* dres for CC decoupling. Get from ccmix.DEC */ 



/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
        settable(t2,1,phi2);
	settable(t3,1,phi3);
	settable(t12,2,rec);

        setautocal();           /* activate auto-calibration */

/*   INITIALIZE VARIABLES   */



    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

    /* 90 degree pulse on Cab, null at CO 128ppm away */
	pwC1 = sqrt(15.0)/(4.0*128.0*sfrq);
        rf1 = (compC*4095.0*pwC)/pwC1;
	rf1 = (int) (rf1 + 0.5);
	
    /* 180 degree pulse on Cab, null at CO 128ppm away */
        pwC2 = sqrt(3.0)/(2.0*128.0*sfrq);
	rf2 = (4095.0*compC*pwC*2.0)/pwC2;
	rf2 = (int) (rf2 + 0.5);	
	if( rf2 > 4295 )
         { printf("increase pwClvl"); psg_abort(1);}
	if(( rf2 > 4095 ) && (rf2 <4296)) rf2=4095; 



/* CHECK VALIDITY OF PARAMETER RANGES */


      if ( 0.5*ni*1/(sw1) > TC)
       { printf(" ni is too big. Make ni equal to %d or less.\n", 
         ((int)((TC)*2.0*sw1))); psg_abort(1);}


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)    tsadd(t1,1,4);  
   
    tau1 = d2;
    tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t12,2,4); }


	if (autocal[0] == 'y')
        {
         if(FIRST_FID)
         {
          ppm = getval("sfrq");
          ofs_check(C13ofs);
          co_ofs = (C13ofs+118.0)*ppm; co_bw = 20*ppm; 
          CO_dec = pbox_Dsh("CO_dec", "SEDUCE1", co_bw, co_ofs, pwC*compC, pwClvl); 
          copwr = CO_dec.pwr; codmf = CO_dec.dmf; codres = CO_dec.dres;
          mixbw = sw1;
          mix_seq = pbox_Dsh("mix_seq", "FLOPSY8", mixbw, 0.0, pwC*compC, pwClvl);
          mixpwr = mix_seq.pwr; mixdmf = mix_seq.dmf; mixdres = mix_seq.dres; 
         }
        }
/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);
        if ( dm3[B] == 'y' )
          { lk_hold(); lk_sampling_off();}  /*freezes z0 correction, stops lock pulsing*/

	rcvroff();
	obspower(pwClvl);
	decpower(tpwr);
 	dec2power(dpwr2);
        dec3power(dpwr3);
	obspwrf(rf1);			/*fine power for Cab 90 degree pulse */
	obsoffset(tof);			/*13C carrier at 46 ppm */
	txphase(zero);
   	delay(1.0e-5);

status(B);        
        rgpulse(pwC1, t1, 0.0,0.0);


/*   xxxxxxxxxxxxxxxxxxxxxx   13Cab Constant Time Evolution       xxxxxxxxxxxxxxxxxx    */

       obspower(copwr); obspwrf(rf0); txphase(zero);
       obsunblank();
       xmtron();
       obsprgon("CO_dec",1.0/codmf,codres);

       if ( dm3[B] == 'y' )   /* turns on 2H decoupling  */
            {
            dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
            dec3unblank();
            dec3phase(zero);
            delay(2.0e-6);
            setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
            delay(TC - tau1 - pwC2/2 - 1/dmf3);
            }
       else
       delay(TC - tau1 - pwC2/2);
       obsprgoff();
       xmtroff();
       obsblank();
       obspower(pwClvl); obspwrf(rf2);
       rgpulse(pwC2, zero, 0.0, 0.0);
       obspower(copwr); obspwrf(rf0); txphase(zero);
       obsunblank();
       xmtron();
       obsprgon("CO_dec",1.0/codmf,codres);

       if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           delay(TC + tau1 - pwC2/2 - 1/dmf3);
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }
       else
       delay(TC + tau1 - pwC2/2);

       obsprgoff();
       xmtroff();
       obsblank();
       obspower(pwClvl); obspwrf(rf1);

       rgpulse(pwC1,t2,0.0,0.0);
       status(C);
       zgradpulse(gzlvl1, gt1);
       delay(gstab);
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxx  FLOPSY 8 Spin lock for mixing xxxxxxxxxxxxxxxxxxxx	*/
       obspower(mixpwr); obspwrf(rf0); txphase(zero);
       obsunblank();
       xmtron();
       obsprgon("mix_seq",1.0/mixdmf,mixdres);
       delay(mix-gt1-gt2);
       obsprgoff();
       xmtroff();
       obsblank();
       obspower(pwClvl); obspwrf(rf1);
       zgradpulse(gzlvl2,gt2);
       delay(gstab);
       rgpulse(pwC1,t3,0.0,rof2);
       getelem(t3,ct,v3);
       add(v3,one,v3);
       obspower(pwClvl); obspwrf(rf0);
       delay(350e-6-rof2);
       rgpulse(pwC*2.0,v3,0.0,0.0);
       delay(350e-6);
       
       status(D);
       setreceiver(t12); 
       
}		 
