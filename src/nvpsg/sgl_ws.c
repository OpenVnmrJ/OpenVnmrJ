/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Voxel selection gradient structs */
SLICE_SELECT_GRADIENT_T  vox1_grad, vox2_grad, vox3_grad;
REFOCUS_GRADIENT_T       vox1r_grad,vox2r_grad,vox3r_grad;
GENERIC_GRADIENT_T       wscrush_grad;  // Water Suppression crusher

/* OVS variables */
char   ovs[MAXSTR];                 /* OVS flag */
double ovsthk;                      /* OVS band thickness in read,phase,slice */
double ovsgap;                      /* OVS band gap from voxel position */
double csd_ppm;                     /* Chemical shift displacement compensation
                                       in ppm */
double ovsTime;
char   insat[MAXSTR];


double ovsflipf;
double satfpwr[6];//ovs fine power 
double tpwrdx,tpwrdy,tpwrdz;

/* Water supression variables */
char   wss[MAXSTR];                 /* water suppression scheme */
int    nwsp;                        /* # ws pulses */
double gcrushws;                    /* ws gradient spoil amplitude */
double tcrushws;                    /* ws gradient spoil duration */
double wsduration;                  /* ws RF and gradient spoil duration */
double ws_delay;                    /* ws delay for WET and DRY */
double wsdel[7];                    /* ws inter-pulse delays */
double wsfpwr[7];                   /* ws fine RF power */
double wsgspoilamp[7];              /* ws gradient spoil amplitude */
double kx[7];              /* ws gradient spoil amplitude trimmer */
double ky[7];              /* ws gradient spoil amplitude trimmer*/
double kz[7];              /* ws gradient spoil amplitude trimmer*/
double wsflipf;                     /* ws flip factor for all the pulses */
double wsflipf_last;                     /* ws flip factor for the last pulse*/
double vapor_ovs[7];                /* Flags to apply ovs between vapor pulses */
double global_ovs;                  /* Flag to switch between global ovs and 
                                       vapor inter-pulse ovs */
double wsTime;                      /* Total duration of ws module */
char   wsrf[MAXSTR];  /*flag to turn on/off rf pulse*/
double    restol;  /*local frequency offset from resto*/


void get_ovsparameters() {
  getstrnwarn("ovs",ovs);
  ovsthk  = getvalnwarn("ovsthk");
  ovsgap  = getvalnwarn("ovsgap");  /* convert from mm to cm */
  csd_ppm = getvalnwarn("csd_ppm");
  ovsflipf  = getvalnwarn("ovsflipf");

  tpwrdx  = getval("tpwrdx");
  tpwrdy  = getval("tpwrdy");
  tpwrdz  = getval("tpwrdz");
  getstrnwarn("insat",insat);
  
}


void get_wsparameters() {
  getstrnwarn("wss",wss);
  gcrushws = getvalnwarn("gcrushws");
  tcrushws = getvalnwarn("tcrushws");
  ws_delay = getvalnwarn("ws_delay");
  wsflipf  = getvalnwarn("wsflipf");
  wsflipf_last  = getval("wsflipf_last");
  getstrnwarn("wsrf", wsrf);
  restol = getvalnwarn("restol");
}


void create_ovsbands() {
  double posoff;        /* positional offset relative to voxel position */
  double csdvox,csdovs; /* chemical shift displacement errors */
//  int sglpowerSave;

  if (ovs[0] == 'y') {
  
    if (sat[0] == 'y') {/* Disallow both OVS and sat bands */
      abort_message("Can not do both OVS and Sat bands, select one or the other.");
    }
    nsat = 6;
    
    /**************************************************/
    /* The OVS functions are built to support suppression of outer-voxel signal
       and assume that you use the vox1_grad, vox2_grad, vox3_grad structs to
       hold information about the voxels. 
       Check that these have been initialized properly */
    /**************************************************/
    if ((vox1_grad.rfBandwidth <= 0) || (vox2_grad.rfBandwidth <= 0) || (vox3_grad.rfBandwidth <= 0)
      ||(vox1_grad.thickness   <= 0) || (vox2_grad.thickness   <= 0) || (vox3_grad.thickness   <= 0)) {
      abort_message("create_ovs: voxel parameters are not initialized correctly");
    }  

    init_slice(&sat_grad,"ovs",ovsthk);
    init_generic(&satcrush_grad,"satcrush",gcrushsat,tcrushsat);

   
   // sglpowerSave=sglpower;
   // sglpower=0;
   
    //this is creating variable flip ovs sat pulses
    init_rf(&sat_rf,satpat,psat,ovsflipf*flipsat,rof1,rof2);
    calc_rf(&sat_rf,"satpwr","satpwrf");
    //sat_rf.powerCoarse=tpwr1;
    //satpwr=tpwr1; //this is to automatically set saturation pulses to the same pwr as P10 pulse of 512 us
    satfpwr[0] = sat_rf.powerFine*tpwrdx;  
    satfpwr[1] = sat_rf.powerFine*tpwrdx;
    satfpwr[2] = sat_rf.powerFine*tpwrdy;  
    satfpwr[3] = sat_rf.powerFine*tpwrdy;
    satfpwr[4] = sat_rf.powerFine*tpwrdz; 
    satfpwr[5] = sat_rf.powerFine*tpwrdz;

    
   // sglpower=sglpowerSave;
    
    calc_slice(&sat_grad,&sat_rf,WRITE,"");
    calc_generic(&satcrush_grad,WRITE,"","");

    /**************************************************/
    /* Calculate positions of OVS sat bands ***********/
    /* First Dimension, pos1 */
    /* chemical shift displacement errors */
    csdvox = csd_ppm*sfrq*vox1_grad.thickness/vox1_grad.rfBandwidth;
    csdovs = csd_ppm*sfrq*sat_grad.thickness/sat_grad.rfBandwidth;
    /* positional offset of centre of OVS bands relative to slice */
    posoff = csdvox + csdovs + (vox1_grad.thickness+sat_grad.thickness)/2.0;
    if (insat[0]=='y'){
      posoff = csdvox + csdovs + (vox1_grad.thickness/2+sat_grad.thickness)/2.0;
    }
    posoff *= 0.1; /* convert from mm to cm */

    satpos[0] = pos1 + posoff + ovsgap;
    satpos[1] = pos1 - posoff - ovsgap;
    
    

    printf("satpos1 is %f \n", satpos[0]);
    printf("true is %f \n", (vox1_grad.thickness+sat_grad.thickness)/2.0);

    /* Second Dimension, pos2 */
    csdvox = csd_ppm*sfrq*vox2_grad.thickness/vox2_grad.rfBandwidth;
    csdovs = csd_ppm*sfrq*sat_grad.thickness/sat_grad.rfBandwidth;
    posoff = csdvox + csdovs + (vox2_grad.thickness+sat_grad.thickness)/2.0;
    if (insat[0]=='y'){
      posoff = csdvox + csdovs + (vox2_grad.thickness/2+sat_grad.thickness)/2.0;
    }
    posoff *= 0.1; 
    satpos[2] = pos2 + posoff + ovsgap;
    satpos[3] = pos2 - posoff - ovsgap;

    /* Third Dimension, pos3 */
    csdvox = csd_ppm*sfrq*vox3_grad.thickness/vox3_grad.rfBandwidth;
    csdovs = csd_ppm*sfrq*sat_grad.thickness/sat_grad.rfBandwidth;
    posoff = csdvox + csdovs + (vox3_grad.thickness+sat_grad.thickness)/2.0;
    if (insat[0]=='y'){
      posoff = csdvox + csdovs + (vox3_grad.thickness/2+sat_grad.thickness)/2.0;
    }
    posoff *= 0.1; 
    satpos[4] = pos3 + posoff + ovsgap;
    satpos[5] = pos3 - posoff - ovsgap;

    /**************************************************/

    /* Total duration of OVS module */
    ovsTime = 6*(sat_grad.duration + satcrush_grad.duration);
  }
  else
    ovsTime = 0;
}


void ovsbands() {
  int     amp[6][3],s,d, shape;
  double  freqlist[MAXNSAT];
  double  satamp[6];

  /* amp[satband][direction] is scaling on sat band slice gradient */
  /* For directions, 0 = Readout, 1 = Phase, 2 = Slice */
  /* Initialize all to zeros */
  for (s = 0; s < nsat; s++)  /* 6 sat bands */
    for (d = 0; d < 3; d++)   /* 3 directions: RO, PE, SS */
      amp[s][d] = 0;
      
  amp[0][0] = amp[1][0] = 1;  /* First two bands along readout */
  amp[2][1] = amp[3][1] = 1;  /* Next two bands along phase    */
  amp[4][2] = amp[5][2] = 1;  /* Last two bands along slice    */

  /* satamp is temporary array of satband gradients,
     necessary for offsetglist to work */
  for (s = 0; s < nsat; s++)
    satamp[s] = sat_grad.amp;

  if (ovs[0] == 'y') {
    offsetglist(satpos,satamp,0,freqlist,nsat,'i');
    shape = shapelist(satpat,sat_grad.duration,freqlist,nsat,sat_grad.rfFraction,'i');
    set_rotation_matrix(vpsi,vphi,vtheta);
    obspower(sat_rf.powerCoarse);
   // obspwrf(sat_rf.powerFine);
    delay(4e-6);

    /* Apply six sat bands, surrounding the voxel */
    /* The only thing changing is the orientation of the sat band gradient */
    for (s = 0; s < nsat; s++ ) { 
      obspwrf(satfpwr[s]);
      obl_shapedgradient(sat_grad.name,sat_grad.duration,
        sat_grad.amp*amp[s][0],
	sat_grad.amp*amp[s][1],
	sat_grad.amp*amp[s][2],NOWAIT);
      delay(sat_grad.rfDelayFront);
      shapedpulselist(shape,sat_grad.rfDuration,oph,rof1,rof2,'i',s);
      delay(sat_grad.rfDelayBack);

      obl_shapedgradient(satcrush_grad.name,satcrush_grad.duration,
	satcrush_grad.amp*amp[s][0],
	satcrush_grad.amp*amp[s][1],
	satcrush_grad.amp*amp[s][2],WAIT);
    }	  

  }  /* end if ovs */
}



void create_watersuppress() {
  int i;
  int sglpowerSave;

  /* Check that water suppression scheme is one of the allowed */
  if (strcmp(wss,"vapor") && 
      strcmp(wss,"wet")   && 
      strcmp(wss,"dry")   &&
      strcmp(wss,"1chess")) {
    abort_message("wss (water suppression scheme) must be vapor, wet, dry or 1chess (%s)",wss);
  }

  /* Initialize all inter-pulse ovs to zero; only modified for vapor */
  for (i = 0; i < 7; i++) vapor_ovs[i] = 0;
  global_ovs = 1;


  /***** Gradient spoil *****/
  init_generic(&wscrush_grad,"wscrush",gcrushws,tcrushws);
  calc_generic(&wscrush_grad,WRITE,"","");

  /***********************************************************/
  /** VAPOR **************************************************/
  /***********************************************************/
  if (!strcmp(wss,"vapor")) {
    nwsp = 7; /* 7 pulses */

    /* Original VAPOR (Tkac et al, MRM 1999, 41(4), 649-656) 
       has flip angle ratios 
          1 : 1 : 1.78 : 1 : 1.78 : 1 : 1.78
       and interpulse delays
         150, 80, 160, 80, 100, 30, 26 ms
       
       To allow longer (32ms gauss) RF pulses, this may be modified 
       without a significant hit in performance (Kinchesh, unpublished)
       to use flip angle ratios
          1 : 1 : 1.78 : 1 : 1.78 : 1.04 : 1.86
       and interpulse delays
         150, 80, 160, 80, 100, 40, 35 ms

       1.86 is the largest scaling factor on the flip angle;
       Calculate the power levels for the largest flip angle
       and adjust the fine power down for the rest.
       wsflipf is a fudge factor for optimization purposes. */

    sglpowerSave=sglpower;
    sglpower=0;
    

    init_rf(&ws_rf,wspat,pws,1.86*flipws,rof1,rof2);
    ws_rf.flipmult=wsflipf; //this is to be able to adjust ws
    calc_rf(&ws_rf,"wstpwr","wstpwrf");
    wsfpwr[0] = ws_rf.powerFine*1.00/1.86;  
    wsfpwr[1] = ws_rf.powerFine*1.00/1.86;
    wsfpwr[2] = ws_rf.powerFine*1.78/1.86;  
    wsfpwr[3] = ws_rf.powerFine*1.00/1.86;
    wsfpwr[4] = ws_rf.powerFine*1.78/1.86; 
    wsfpwr[5] = ws_rf.powerFine*1.04/1.86;
    wsfpwr[6] = ws_rf.powerFine*wsflipf_last;

   sglpower=sglpowerSave;

    /* Delays between pulses */
    wsduration = ws_rf.rfDuration + rof1 + rof2 + wscrush_grad.duration;
    wsdel[0] = 0.150 - wsduration;  
    wsdel[1] = 0.080 - wsduration;
    wsdel[2] = 0.160 - wsduration; 
    wsdel[3] = 0.080 - wsduration;
    wsdel[4] = 0.100 - wsduration;  
    wsdel[5] = 0.040 - wsduration;
    wsdel[6] = 0.035 - wsduration + rof1 + ws_rf.rfDuration/2 + d1;
  //wsdel[6] =  d1;  //should have the ability to have the shortest delay in vapor
      /* ideally, one would also subtract the duration of the 
         initial slice select RF pulse, but we don't know that 
	 at this point */

    /* Spoiler gradient amplitudes */
    for (i=0; i<nwsp; i++) {
      wsgspoilamp[i] = wscrush_grad.amp;
      

      if (wsdel[i] < 0)
        abort_message("Water suppression pulse (pws) or gradient crusher duration (tcrushws) too long for VAPOR");
    }

    kx[0]=0;ky[0]=1;kz[0]=0;
    kx[1]=1;ky[1]=0;kz[1]=0;
    kx[2]=0;ky[2]=0;kz[2]=1;
    kx[3]=0;ky[3]=1;kz[3]=0;
    kx[4]=1;ky[4]=0;kz[4]=0;
    kx[5]=0;ky[5]=0;kz[5]=1;
    kx[6]=0;ky[6]=1;kz[6]=0;

    
    /* Set flag for applying ovs between pulses 4-5 and after 7 */
    printf("ovsTime is %f \n", ovsTime);
    printf("wsdel3 is %f \n", wsdel[3]);
    printf("wsdel6 is %f \n", wsdel[6]);
    printf("wsdel2 is %f \n", wsdel[2]);
    printf("wsdel5 is %f \n", wsdel[5]);
    if (ovs[0] == 'y') {
      if (wsdel[2] >= ovsTime) {
        vapor_ovs[2] = 1;
	wsdel[2] -= ovsTime;
	global_ovs = 0;
      }
      

     if (wsdel[3] >= ovsTime) {
        vapor_ovs[3] = 1;
	wsdel[3] -= ovsTime;
	global_ovs = 0;
      }

      if (wsdel[4] >= ovsTime) {
        vapor_ovs[4] = 1;
	wsdel[4] -= ovsTime;
	global_ovs = 0;
      }
    }
  }  /* end of VAPOR part */

  /***********************************************************/
  /** WET ****************************************************/
  /***********************************************************/
  else {
    if (!strcmp(wss,"wet")) {
      nwsp = 4; /* 4 pulses */

      /* Original WET has flip angles 81.4 : 101.4 : 69.3 : 161.0
         Ogg et al, JMR B 1994, 104(1), 1-10  
	 
	 Calculate the power levels for the 161 degree flip
	 and adjust the rest down through the fine power */
	 
      init_rf(&ws_rf,wspat,pws,(161.0/69.3)*flipws,rof1,rof2);
      ws_rf.flipmult=wsflipf; //this is to be able to adjust ws
      calc_rf(&ws_rf,"wstpwr","wstpwrf");
      wsfpwr[0] = ws_rf.powerFine*81.4/161.0; 
      wsfpwr[1] = ws_rf.powerFine*101.4/161.0;
      wsfpwr[2] = ws_rf.powerFine*69.3/161.0; 
      wsfpwr[3] = ws_rf.powerFine*wsflipf_last;

      wsgspoilamp[0] = wscrush_grad.amp;
      wsgspoilamp[1] = wscrush_grad.amp/2.0; 
      wsgspoilamp[2] = wscrush_grad.amp/4.0; 
      wsgspoilamp[3] = wscrush_grad.amp/8.0; 

    kx[0]=1;ky[0]=1;kz[0]=1;
    kx[1]=1;ky[1]=1;kz[1]=1;
    kx[2]=1;ky[2]=1;kz[2]=1;
    kx[3]=1;ky[3]=1;kz[3]=1;
   
    }

    /***********************************************************/
    /** DRY; 3 identical CHESS pulses **************************/
    /***********************************************************/
    else {
      init_rf(&ws_rf,wspat,pws,flipws,rof1,rof2);
       ws_rf.flipmult=wsflipf; //this is to be able to adjust ws
      calc_rf(&ws_rf,"wstpwr","wstpwrf");
      if (!strcmp(wss,"dry")) {
        nwsp = 3; /* 3 pulses */
        wsgspoilamp[0] = wscrush_grad.amp/4.0;
        wsgspoilamp[1] = wscrush_grad.amp/2.0;
        wsgspoilamp[2] = wscrush_grad.amp;


       kx[0]=1;ky[0]=1;kz[0]=1;
       kx[1]=1;ky[1]=1;kz[1]=1;
       kx[2]=1;ky[2]=1;kz[2]=1;
       
      wsfpwr[0] = ws_rf.powerFine; 
      wsfpwr[1] = ws_rf.powerFine;
      wsfpwr[2] = ws_rf.powerFine*wsflipf_last; 
	}
     

      /***********************************************************/
      /** SINGLE CHESS PULSE *************************************/
      /***********************************************************/
      /* 1chess is used by some to correct for coil loading in quantitation */
      if (!strcmp(wss,"1chess")) {
        nwsp = 1; /* single pulse */
        wsgspoilamp[0] = wscrush_grad.amp;
        wsfpwr[0]      = ws_rf.powerFine;
        kx[0]=1;ky[0]=1;kz[0]=1;
   
      }
    }

    /* For both WET, DRY and Single CHESS */
    for (i=0; i<nwsp; i++) wsdel[i] = ws_delay;

    wsdel[nwsp-1] += d1;   /* d1 tweaker delay */

  }
  
  /* Total duration of water supression module */
  wsTime = nwsp*(rof1 + ws_rf.rfDuration + rof2 + wscrush_grad.duration);
  for (i=0; i<nwsp; i++) {
    wsTime += wsdel[i];
    if (vapor_ovs[i]) wsTime += ovsTime;
  }

}




void watersuppress() {
  int i;
  double ws_delta; // water suppresssion frequency, delta from transmitter

 // obspower(ws_rf.powerCoarse);
  ws_delta = getval("ws_delta"); 
  delay(4e-6);
  
   for (i=0; i<nwsp; i++) {
    obspower(ws_rf.powerCoarse);
    obspwrf(wsfpwr[i]);
  if (wsrf[0]=='y') //to play out the ws with/without the rf pulses
    {
    shapedpulseoffset(ws_rf.pulseName,ws_rf.rfDuration,zero,rof1,rof2,ws_delta);
    }
    else 
    {
    delay(ws_rf.rfDuration);
    }
    obl_shapedgradient(wscrush_grad.name,wscrush_grad.duration,
      kx[i]*wsgspoilamp[i],ky[i]*wsgspoilamp[i],kz[i]*wsgspoilamp[i],WAIT);
    delay(wsdel[i]);
    
    if (vapor_ovs[i])  /* Apply OVS after certain VAPOR pulses */
      ovsbands();

  }
}
