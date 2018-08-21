/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* The experiment allows to measure cross-correlated 
relaxation rate between NH DD and 15N CSA. It is intended
for large molecules or high fields where interference
effect is significant. For CCR < 6 s-1, traditional 
indirect methods may be better.

"Delta" and "Tct" should be created(create('Delta','delay'), 
create('Tct','delay')) Tct sets the constant-time delay. 
Delta sets the proton inversion pulse position 
( 0 =< Delta <= Tct ).A proper value of Tct can be 
chosen by setting Delta =0,Tct. Optimal performance 
is expected when the intensity at Delta=Tct is around 
1/3 of that at Delta=0. This criteria should be applied to 
signals of interest rather than to the slowly relaxing 
terminal residues. Delta is to be arrayed, e.g. using a
pseudo-code like: array('Delta',n,0,Tct/(n-1))

15N chem shift evolves during Tct. If Tct is set too 
short in cases where CCR is large, a semi-constant time 
setup takes effect automatically. The user doesn't have 
to do anything specific.

This pulse sequence also allows to measure CCR by 
TROSY-antiTROSY method. In this case,
"overide_kappa" flag needs to be created and set to 'y'. 
i.e., create('overide_kappa', 'flag'), and overide_kappa=y. 
"kappa" should be created and set to 0. i.e., create('kappa',
'real'), and kappa=0. To measure TROSY relaxation rates, 
set Delta=0 and vary Tct, or co-vary Delta and Tct (at 
equal values) to measure anti-TROSY rates. For more infor. 
please refer to: Liu Y & Prestegard JH, 
J Magn Reson. 2008 193(1):23-31

Revised version, 8may2009, GG, Varian


 */

#include <standard.h>
  

	     
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                      

	     phi1[2]  = {1,3},	
             phi2[8]  = {0,0,2,2},
 		phi3[1] = {0},		
             rec[4]   = {0,2};	     


static double   d2_init=0.0, relaxTmax;


pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    NHonly[MAXSTR],		       /* spectrum of only NH groups  */
	    NH2only[MAXSTR],		       /* spectrum of only NH2 groups */
	    T1[MAXSTR],				/* insert T1 relaxation delay */
	    T1rho[MAXSTR],		     /* insert T1rho relaxation delay */
	    T2[MAXSTR],				/* insert T2 relaxation delay */
		overide_kappa[MAXSTR], /*whether to overide kappa */
	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
	    rTnum,			/* number of relaxation times, relaxT */
	    rTcounter;		    /* to obtain maximum relaxT, ie relaxTmax */

double 
		Tct = getval("Tct"), /*relaxation delay */     
	Delta = getval("Delta"), 
	kappa  = getval("kappa"), 

		tau1,         				         /*  t1 delay */

	    lambda = 0.91/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
	    tNH = 1.0/(4.0*getval("JNH")),	  /* 1/4J N15 evolution delay */
	    relaxT = getval("relaxT"),		     /* total relaxation time */
	    rTarray[1000], 	    /* to obtain maximum relaxT, ie relaxTmax */
            maxrelaxT = getval("maxrelaxT"),    /* maximum relaxT in all exps */
	    ncyc,			 /* number of pulsed cycles in relaxT */
        
/* the sech/tanh pulse is automatically calculated by the macro "proteincal", */
/* and is called directly from your shapelib.                  		      */

   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compN = getval("compN"),       /* adjustment for N15 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

	calH = getval("calH"), /* multiplier on a pw pulse for H1 calibration */

  tpwrsf_t = getval("tpwrsf_t"), /* fine power adustment for first soft pulse(TROSY=n)*/
  tpwrsf_n = getval("tpwrsf_n"), /* fine power adustment for first soft pulse(TROSY=y)*/
  tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for second soft pulse(TROSY=y)*/
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	calN = getval("calN"),   /* multiplier on a pwN pulse for calibration */
	slNlvl,					   /* power for N15 spin lock */
        slNrf = 1500.0,        /* RF field in Hz for N15 spin lock at 600 MHz */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac to G/cm conversion      */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("C13refoc",C13refoc);
    getstr("NH2only",NH2only);
    getstr("NHonly", NHonly);
    getstr("T1",T1);
    getstr("T1rho",T1rho);
    getstr("T2",T2);
	getstr("overide_kappa",overide_kappa);
    getstr("TROSY",TROSY);



/*   LOAD PHASE TABLE    */

    TROSY[A]='y';
	NHonly[A]='n';
/* always use TROSY */

	settable(t1,2,phi1);
	settable(t2,4,phi2);
	settable(t3,1,phi3);
	settable(t4,2,rec);
 

/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }}

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */

/* power level for N15 spinlock (90 degree pulse length calculated first) */
	slNlvl = 1/(4.0*slNrf*sfrq/600.0) ;
	slNlvl = pwNlvl - 20.0*log10(slNlvl/(pwN*compN));
	slNlvl = (int) (slNlvl + 0.5);

/* use 1/8J times for relaxation measurements of NH2 groups */
  if ( (NH2only[A]=='y') && ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )	
     {  tNH = tNH/2.0;  }

/* reset calH and calN for 2D if inadvertently left at 2.0 */
  if (ni>1.0) {calH=1.0; calN=1.0;}



/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((TROSY[A]=='y') && (NHonly[A]=='y'))
  { text_error( "incorrect NHonly flag ! Should be 'n' \n"); psg_abort(1); }

  if ((TROSY[A]=='y') && (gt1 < -gstab + pwHs + 1.0e-4 + 2.0*POWER_DELAY))
  { text_error( " gt1 is too small. Make gt1 equal to %f or more.\n",    
    (-gstab + pwHs + 1.0e-4 + 2.0*POWER_DELAY) );		    psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }

if(Delta >Tct ){
        text_error("Decrease Delta or increase Tct!");
        psg_abort(1);
     }
if(Tct == 0){
	text_error("Set Tct to a very small value such as 1usec if you want it to be zero.");
	psg_abort(1);
}


if ((TROSY[A]=='y') && (dm2[C] == 'y'))
{ text_error("Choose either TROSY='n' or dm2='n' ! ");              psg_abort(1); }


/* set kappa */
	if(overide_kappa[A] == 'y'){ }
	else{
		kappa = Tct>ni/sw1? 1:Tct/(ni/sw1);
		kappa = ((double)((int)(kappa*1000)))/1000; /* 3 digits after the point, may cause a small decrease in value for safe boundary */
	}

	printf("kappa=%f\n",kappa);
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

	if (phase1 == 1)   	 icosel = -1;
        else 	  {  tsadd(t3,2,4);  icosel = +1;  }


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t4,2,4); }




/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rf0);
 	dec2power(pwNlvl);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);

	delay(d1);

 
/*  xxxxxxxxxxxxxxxxx  CONSTANT SAMPLE HEATING FROM N15 RF xxxxxxxxxxxxxxxxx  */

 

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
status(B);
        rcvroff();

	zgradpulse(gzlvl0,0.5e-3);
	delay(gstab);
	zgradpulse(gzlvl0,0.5e-3);
	delay(gstab);
	decpwrf(rfst);
	

	delay(50.0e-6);

 
   	rgpulse(calH*pw,zero,0.0,0.0);                 /* 1H pulse excitation */

	txphase(zero);
   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);


   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);


   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, three, 0.0, 0.0);
	txphase(two);
    
         obspower(tpwrs+6.0);
           obspwrf(tpwrsf_n);

   	   shaped_pulse("H2Osinc_n", pwHs, zero, 5.0e-5, 0.0);


	 obspower(tpwr); obspwrf(4095.0);

	zgradpulse(gzlvl3, gt3);
	dec2phase(t1);
	delay(gstab - 10.0e-6);

        initval(45.0,v1);
        dec2stepsize(1.0);
        dcplr2phase(v1);
        delay(10.0e-6);

   	dec2rgpulse(calN*pwN, t1, 0.0, 0.0);
	dcplr2phase(zero);
	txphase(zero);
	decphase(zero);

	delay(lambda/0.91/2.0-gt4-pwN-gstab);
        zgradpulse(gzlvl4*0.7,gt4);
        delay(gstab - GRADIENT_DELAY);
        sim3pulse(2.0*pw, 0.0,2.0*pwN,zero,zero,zero,0.0,0.0);
        zgradpulse(gzlvl4*0.7,gt4);
        delay(lambda/0.91/2.0 -gt4 -pwN - GRADIENT_DELAY);
        dec2rgpulse(pwN, t1, 0.0,0.0);



/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */
	

		if( C13refoc[A]=='y' && Delta*2.0*kappa*tau1/Tct + 2.0*pw > 1.0e-3 + WFG2_START_DELAY ){ 
				delay( Delta*kappa*tau1/Tct + pw - 0.5e-3 - WFG2_START_DELAY );
				decshaped_pulse("stC200",1.0e-3,zero, 0.0, 0.0);
				delay( Delta/2.0 + pw - 0.5e-3 );
		}
		else{
        		delay(Delta*(Tct/2.0 + kappa*tau1)/Tct + 2.0*pw);
		}

                zgradpulse(gzlvl4, gt4);
                delay(gstab- GRADIENT_DELAY);
                dec2rgpulse(2.0*pwN,zero,0.5e-6,0.5e-6); /*treat this as a unit */
                zgradpulse(gzlvl4, gt4);
                delay(gstab-GRADIENT_DELAY);


       delay (Delta*(Tct/2.0 - kappa*tau1)/Tct );

                rgpulse(pw,zero,0.0,0.0);
                rgpulse(2.0*pw,one,0.1e-6,0.1e-6);
                rgpulse(pw,zero,0.0,0.0);


       delay ( (Tct-Delta)*(Tct/2.0 - kappa*tau1)/Tct );

        	zgradpulse(gzlvl4*1.4, gt4);
        	delay(gstab- GRADIENT_DELAY);
        	dec2rgpulse(2.0*pwN,zero,0.5e-6,0.5e-6);
        	zgradpulse(gzlvl4*1.4, gt4);
        	delay(gstab- GRADIENT_DELAY);

				        
		if( C13refoc[A]=='y' && 2*tau1*(1-Delta*kappa/Tct) + 2.0*pw > 1.0e-3 + WFG2_START_DELAY ) {
				delay( (1-kappa)*tau1 + (Tct - Delta)/2.0 + pw - 0.5e-3 - WFG2_START_DELAY );
				decshaped_pulse("stC200",1.0e-3,zero, 0.0, 0.0);
				delay( tau1*(1-Delta*kappa/Tct) + pw - 0.5e-3 );
		}
		else{
				delay( (Tct-Delta)*(Tct/2.0 + kappa*tau1)/Tct + 2.0*pw) ;
				delay( (1-kappa)*2*tau1 ); /*semi constant delay*/
		}


    	zgradpulse(gzlvl1, gt1);   
	delay(gstab - GRADIENT_DELAY);

	dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
	
	delay(gt1+ gstab );

	txphase(three);
     

	txphase(t4);



/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	sim3pulse(pw,0.0,pwN, zero,zero, t3, 0.0, 0.0);

        if (tpwrsf_d<4095.0)
        {
         obspwrf(tpwrsf_d); obspower(tpwrs+6.0);
   	 shaped_pulse("H2Osinc_d", pwHs, zero, 5.0e-5, 0.0);
	 obspower(tpwr); obspwrf(4095.0);
        }
        else
        {   
         obspower(tpwrs);
   	 shaped_pulse("H2Osinc_d", pwHs, zero, 5.0e-5, 0.0);
	 obspower(tpwr);
        }	

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 0.65*(pw + pwN) - gt5 - pwHs - 2.0*POWER_DELAY -2.0*PWRF_DELAY -50.0e-6);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(zero);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, zero, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5 );

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(one);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.6*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 0.0); 

	delay((gt1/10.0) + 1.0e-4 +gstab - 0.65*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
        
      zgradpulse(icosel*gzlvl2, 0.1*gt1);		/* 2.0*GRADIENT_DELAY */
        delay(gstab);
        rcvron();
statusdelay(C,1.0e-4);		


	setreceiver(t4);
}		 
