/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  cbcaco.c

    3D 13C observe CACO experiment with IPAP option. 
    IPAP='i' for inphase splitting and IPAP='a' for anti-phase splitting.

    Set tn='C13', dn='H1', dn2='N15, dn3='H2'.
    
    Offsets:
    CO          = 174ppm    13C carrier position
    CAB		= 44.2 ppm    shifted for CACB pulses
    CA		= 57.7ppm   shifted for CA pulses
    N15         = 120ppm
    H1          = 4.7ppm

    pulse sequence: 
    from review of  W. Bermel  et al Concepts in Magnetic Resonance Part A,
    2008 vol 32 183-200
    11 September 2006, N.Murali, Varian
    Modified  June 2011, E. Tischenko, Agilent to use BIP pulses

                  BIP (broadband inversion Pulses )
                 Mari A. Smith, Haitao Hu, and A. J. Shaka
                Journal of Magnetic Resonance 151, 269 283 (2001)

    Must set phase = 1,2 phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C] and t2[C13].
    set dm2='nyy' dmm2='cgg', or 'ccp'  dm='nyy' dmm='cww',or 'cgg'.
*/
#include <standard.h>
static int  
                      
	     phi1[]  = {0,2,0,2},
	     phi2[]  = {1,1,3,3},
	     phi3[]  = {0,0,0,0},
	     phi4[]  = {0,0,0,0},
             phi5[]  = {0,0,0,0},
             rec[]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0, C13ofs=174.0;
 

pulsesequence()
{
 
int         t1_counter, t2_counter;	/* used for states tppi in t1 & t2*/
char	    IPAP[MAXSTR],Hstart[MAXSTR],
            f1180[MAXSTR], H2dec[MAXSTR],
            shCACB_90[MAXSTR],shCACB_90r[MAXSTR],
	    shCACB_180[MAXSTR], 
            shCACB_180off[MAXSTR],
            shCBIP[MAXSTR],
	    shCO_90[MAXSTR],            
	    shCO_180[MAXSTR],
            shCO_180off[MAXSTR];
double      
             ni2=getval("ni2"),
       	     tau1,			/*  t1 delay */
             tau2,			/*  t2 delay */
             pwCBIP  =  getval("pwCBIP"),	
             pwCO_90  =  getval("pwCO_90"), /* 90 degree pulse on C13 */
             pwCO_90phase_roll = getval("pwCO_90phase_roll") ,
                   /* fraction of CACB pulse to compensate for phase roll */
             pwrCO_90 =  getval("pwrCO_90"),  	/*power  */
             pwrfCO_90 = getval("pwrfCO_90"),
 	     TC = getval("TC"), 		/* delay 1/(2JCACB) ~  7.0ms in Ref. */ 
             del = getval("del"),	/* delay del = 1/(2JC'C) ~ 9.0ms in Ref. */         
             pwClvl = getval("pwClvl"), 	/* coarse power for C13 pulse */
             pwC = getval("pwC"),        /* C13 90 degree pulse length at pwClvl */
             compC = getval("compC"),    /* adjustment for C13 amplifier compression */
	     pwClvlF=getval("pwClvlF"),  /* maximum fine power when using pwC pulses */
	     pwHlvl = getval("pwHlvl"),
             pwH = getval("pwH"),	    
	     TCH = getval("TCH"), 
        
/* 180 degree pulse at CO (174ppm)  */

             pwCO_180   =  getval("pwCO_180"),
			/* 180 degree pulse length on C13  */
             pwrCO_180  =  getval("pwrCO_180"),/*power  */
             pwrfCO_180 =  getval("pwrfCO_180"),

/* 90 degree pulse at CAB (57.7ppm)  */

             tofCACB    =  getval("tofCACB"), 
             pwCACB_90  =  getval("pwCACB_90"),
             pwCACB_90phase_roll=getval("pwCACB_90phase_roll") ,
              /* fraction of CACB pulse to compensate for phase roll */

             pwrCACB_90 =  getval("pwrCACB_90"),	/*power  */
             pwrfCACB_90 = getval("pwrfCACB_90"),

/* 180 degree pulse at CA (57.7ppm)  */
             pwCACB_180   =  getval("pwCACB_180"),	
		/* 180 degree pulse length on C13  */
             pwrCACB_180  =  getval("pwrCACB_180"),   	/*power  */
             pwrfCACB_180 =  getval("pwrfCACB_180"),

             pwCB_180   =  getval("pwCB_180"),
		/* 180 degree pulse length on C13  */
             pwrCB_180  =  getval("pwrCB_180"),
 

             sw1 = getval("sw1"),
             sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		     
	gzlvl1 = getval("gzlvl1"),

	gt2 = getval("gt2"),				   
	gzlvl2 = getval("gzlvl2"),
        gt3 = getval("gt3"),
        gzlvl3 = getval("gzlvl3"),
        gstab = getval("gstab");

        getstr("H2dec",H2dec); 
        getstr("IPAP",IPAP); 
        getstr("shCBIP",shCBIP);
        getstr("f1180",f1180);
        getstr("shCACB_90",shCACB_90); 
        getstr("shCACB_90r",shCACB_90r);
        getstr("shCACB_180",shCACB_180); 
        getstr("shCACB_180off",shCACB_180off);
        getstr("shCO_90",shCO_90);
        getstr("shCO_180",shCO_180); 
        getstr("shCO_180off",shCO_180off);
	getstr("Hstart",Hstart);


/*   LOAD PHASE TABLE    */
	settable(t1,4,phi1);
        settable(t2,4,phi2);
	settable(t3,4,phi3);
	settable(t4,4,phi4);
        settable(t5,4,phi5);
	settable(t12,4,rec);
        

         

	 

/*   INITIALIZE VARIABLES   */
if( (IPAP[A] != 'i') &&  (IPAP[A] != 'a')&&  (IPAP[A] != 't'))
  { text_error("IPAP flag either i or a, exiting "); psg_abort(1); }

 if( (Hstart[A]=='y') && ((dmm[A]!='c' || dm[A]=='y')) ) 
{ 
 text_error("Incorrect combination of dm, dmm and Hstart.  "); 
                         psg_abort(1);};


if(ni/sw1 > TC) { 

 text_error("too many increments in CAB t1 evolution, should be less than  %d\n",(int)( (TC)*sw1 +0.5)); 
                         psg_abort(1);};
if(ni2/sw2 > TC-2.0*(gstab-gt2) ) { 

 text_error("too many increments in CA t2 evolution, should be less than  %d\n",(int)( (TC-2.0*(gstab-gt2))*sw2 +0.5)); 
                         psg_abort(1);};


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)    tsadd(t1,1,4);  
    if (phase2 == 2) tsadd(t2,1,4); 
    
    tau1 = d2;
    if(f1180[A]=='y') {tau1+=0.5/sw1;}
    tau1 = tau1/2.0;

    tau2 = d3;
    tau2 = tau2/2.0;
    


    
/* Calculate modifications to phases for States-TPPI acquisition          */
if(IPAP[A]=='a'){tsadd(t4,1,4);};

   if(d2_index % 2) 	{ tsadd(t1,2,4); tsadd(t12,2,4); }
   if(d3_index % 2)     { tsadd(t2,2,4); tsadd(t12,2,4); }

/*
   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t2,2,4); tsadd(t12,2,4); }

*/


	
/* BEGIN PULSE SEQUENCE */

status(A);
   	 delay(10.0e-6);
        obspower(pwClvl);
        obspwrf(4095.0);
        dec2power(dpwr2); dec2pwrf(4095.0);
   	obsoffset(tofCACB);
        delay(d1);

/* option to start from H magnetization */
if(Hstart[A]=='y') 
     {
	   decpower(pwHlvl); decpwrf(4095.0);
	   decrgpulse(pwH, zero, 0.0, 0.0);
	   delay(TCH);
	   simpulse(2.0*pwC,2.0*pwH, zero,zero, 0.0, 0.0);
	   delay(TCH);
	   decrgpulse(pwH, one, 0.0, 0.0);
	   zgradpulse(gzlvl3*0.6,gt3);
	   delay(gstab);
 	   rgpulse(pwC, two, 0.0, 0.0);
 	   delay(TCH);
 	   simpulse(2.0*pwC,2.0*pwH, zero,zero, 0.0, 0.0);
 	   delay(TCH);
  	   rgpulse(pwC, one, 0.0, 0.0);
	   zgradpulse(gzlvl3,gt3);
	   delay(gstab*2.0);
	   decpower(dpwr); decpwrf(4095.0);
}
if (H2dec[A] == 'y') lk_hold();
status(B);

/* CBCACO experiment */     

/************optional deuterium decoupling**************************/
if(H2dec[A] == 'y')
 {  dec3unblank();
    if(1.0/dmf3>900.0e-6) 
    {
     dec3power(dpwr3+6.0);
     dec3rgpulse(0.5/dmf3, one, 1.0e-6, 0.0e-6);
     dec3power(dpwr3);
    }
   else dec3rgpulse(1.0/dmf3, one, 1.0e-6,0.0e-6);
   dec3phase(zero);   
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
}	
/**************************************/

/* begin CA/CB  t1 evolution and transfer to CA */

 obspwrf(4095.0);  obspower(pwClvl);
 obspower(pwrCACB_90);
 shapedpulse(shCACB_90,pwCACB_90,t1,0.0,0.0);

 delay(tau1);

 obspower(pwrCO_180);
 shapedpulse(shCO_180off,pwCO_180,zero,0.0,0.0);

 delay(TC/2.0);
         	  
 obspower(pwrCACB_180);
 shapedpulse(shCACB_180,pwCACB_180,zero,0.0,0.0);

 delay(TC/2.0-tau1);

 obspower(pwrCO_180);
 shapedpulse(shCO_180off,pwCO_180,zero,0.0,0.0);
 rgpulse(pw,one,0.0,0.0);
 obspower(pwrCACB_90);
 shapedpulse(shCACB_90,pwCACB_90,one,0.0,0.0);

 /* on CAy + CBzCAx  now */
 /* CAy -> CAxCOz   CBzCAx -> CAy -> CAxCOz transfer  together with CA t2 evolution */
 

 obspower(pwrCO_180);
 shapedpulse(shCO_180off,pwCO_180,zero,0.0,0.0);
 
 delay(TC/2.0+ pwCACB_90phase_roll*pwCACB_90 -gstab-gt2 -tau2);

 zgradpulse(gzlvl2,gt2);
 delay(gstab);
 obspower(pwrCACB_180);
 shapedpulse(shCACB_180,pwCACB_180,zero,0.0,0.0);

 obspower(pwrCO_180);
 shapedpulse(shCO_180off,pwCO_180,zero,0.0,0.0);

 zgradpulse(gzlvl2,gt2);
 delay(gstab);
      
 delay(TC/2.0  -gstab-gt2 +tau2);

 obspower(pwrCACB_90);
 shapedpulse(shCACB_90,pwCACB_90,t2,0.0,0.0);

/*************************************************************/
 if(H2dec[A] == 'y') 
 {                     
  setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
  if(1.0/dmf3>900.0e-6)
  {
   dec3power(dpwr3+6.0);
   dec3rgpulse(0.5/dmf3, three, 1.0e-6, 0.0e-6);
   dec3power(dpwr3);
  }
  else dec3rgpulse(1.0/dmf3, three, 1.0e-6, 0.0e-6);	
 dec3blank();  
 }
/*************************************************************/
 /* CAzCOz */
  delay(10e-6);
  obsoffset(tof);
  delay(10e-6);
  zgradpulse(gzlvl1,gt1);
  delay(gstab);
status(C);
 /* CAzCOz ->  CO or CACO */

  obspwrf(pwrfCO_90);  obspower(pwrCO_90);
  shapedpulse(shCO_90,pwCO_90,t4,0.0,0.0);
        /* ghost 180 on CA*/

  if(IPAP[A]=='i')  
  {
   obspwrf(pwClvlF);  obspower(pwClvl);
   delay(10.0e-6);
   shapedpulse(shCBIP,pwCBIP,zero,0.0,0.0);
   delay(10.0e-6);
   delay(del/2.0 + pwCO_90phase_roll*pwCO_90);
   obspwrf(pwrfCACB_180); obspower(pwrCACB_180);
   delay(10.0e-6);
   shapedpulse(shCACB_180off,pwCACB_180,zero,0.0,0.0);
   obspwrf(pwClvlF);  obspower(pwClvl);
   delay(10.0e-6);
   shapedpulse(shCBIP,pwCBIP,zero,0.0,0.0);
   obspwrf(pwrfCACB_180); obspower(pwrCACB_180);
   delay(10.0e-6);
   shapedpulse(shCACB_180off,pwCACB_180,zero,0.0,0.0);
   delay(10.0e-6);
   delay(del/2.0 );
  }

/***>>>>>>>>>>>**TEST*********/
  if(IPAP[A]=='t') 
  {
    obspwrf(pwrfCACB_180); obspower(pwrCACB_180);
    delay(10.0e-6);
    shapedpulse(shCACB_180off,pwCACB_180,zero,0.0,0.0);
    delay(10.0e-6);
    obspwrf(pwrfCO_180); obspower(pwrCO_180);
    delay(del/2.0);
    shapedpulse(shCO_180,pwCO_180,zero,0.0,0.0);
    obspwrf(pwrfCACB_180); obspower(pwrCACB_180);
    delay(10.0e-6);
    shapedpulse(shCACB_180off,pwCACB_180,zero,0.0,0.0);
    delay(10.0e-6);
    obspwrf(pwrfCO_90);  obspower(pwrCO_90);
    delay(del/2.0);
  }
/********<<<<<<<<<<<<<**TEST*********/

  if(IPAP[A]=='a')   
  {
   obspwrf(pwClvlF);  obspower(pwClvl);
   delay(10.0e-6);
   shapedpulse(shCBIP,pwCBIP,zero,0.0,0.0);
   delay(10.0e-6);
   delay(del/4.0 + pwCO_90phase_roll*pwCO_90);
   obspwrf(pwrfCACB_180); obspower(pwrCACB_180);
   delay(10.0e-6);
   shapedpulse(shCACB_180off,pwCACB_180,zero,0.0,0.0);
   obspwrf(pwClvlF);  obspower(pwClvl);
   delay(10.0e-6);
   delay(del/4.0 );
   shapedpulse(shCBIP,pwCBIP,zero,0.0,0.0);
   delay(del/4.0);
   obspwrf(pwrfCACB_180); obspower(pwrCACB_180);
   delay(10.0e-6);
   shapedpulse(shCACB_180off,pwCACB_180,zero,0.0,0.0);
   delay(10.0e-6);
   delay(del/4.0 );
  }
if (H2dec[A]=='y') lk_sample();
status(D);
	setreceiver(t12);
	
}
