/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*   Modified 1/10/05 for the npsg.
     dec3 ('4') add on 1/21/05. 

     The function tancp provides a variable amplitude cross polarization 
     with either a tangent ramp, a linear ramp or a constant value for one
     of the two channels involved in CP. The other channel has a constant 
     amplitude.  This function is written for Varian NMR spectrometer
     systems only.   

     lcntct  -  is the full duration of the CP shape. Use the parameter 
                "cntct".  

     level   -  is the average and median amplitude of the shape. Choose 
                the parameter (c.f. "tpwrm") that sets the appropriate 
                modulator.

     level1  -  is the amplitude of the constant channel (c.f. "crossp")
                if the constant channel is the decoupler.                
      
     lwidth  -  is the amplitude width (amp(end) - amp(beginning)) of the 
                shape.  If lwidth < 0.0,  the slope of the ramp is reversed. 
                lwidth is usually set from the parameter "width".  The shape 
                has a maxiumum amplitude excursion of +/- lwidth/2.0 about 
                the value "level". 

     lcurve  -  is a curvature parameter for tangent ramps.  If lcurve
                is greater than 1.0e4, the ramp is nearly linear.  If 
                lcurve is < 1.0e-4 the ramp is nearly constant amplitude 
                with sharp excursions at the ends to +/- lwidth/2.0. If 
                lcurve is approximately 1.0 the amplitude has a tangent 
                shape. lcurve is usually set from the parameter "curve".

   lmaxsteps -  is the maximum number of steps in the shape.  It can be set 
                directly as an argument of the function, without defining 
                a parameter. A good value is 250, but greater numbers can
                be used.  If lmaxsteps is too large for the hardware a "FIFO
                Underflow" error" will occur. 

     ltype   -  is 'l' for linear, 'c' for constant and 't' for a tangent. 
                lcurve is usually set from the parameter "curve".  'l' forces
                lcurve = 1.0e4 in the equation for the tangent ramp.  

     lchnl   -  A two character string (c.f. "12" - note the double quotes)
                that designates the two pulse-program channels involved in CP. 
                The first character designates the shaped channel and the 
                second character designates the constant channel where '1' 
                is observe (obs), '2' is the first decoupler (dec), '3' is 
                the second decoupler (dec2) and '4' is the third decoupler 
                (dec2). 

     The parameters that provide level and level1 must be the amplitudes for 
     the pulse-program channels designated by "lchnl".  The pulse program 
     channels obs, dec, dec2 and dec3 may be assigned to any transmitter 
     with the parameter "rfchannel".    */

/*Beginning of the tancp function*/
      		      
void tancp(double lcntct, double level, double level1,
           int    lmaxsteps,
           char  *lchnl,
           double lcurve, double lwidth,
           char  *ltype)
{
   double     time,lastamp,duration,tduration,mintimestep,
              timestep,amp,cntct2,dcntct,aphh_dur[2048],
              aphh_amp[2048];

   int        index,nsteps,wstep,ticks,minstepticks,
              stepticks,amp1;

   lwidth=lwidth/2.0;

/*set lcurve or lwidth for linear or constant CP - default is a tangent CP*/

   if (ltype[0]=='l') lcurve=1e6;
 
   if (ltype[0]=='c') {
      lwidth=0.0; 
      lcurve=1e6;}

/*determine number of steps in the calculation and the minimum duration*/

   ticks = lcntct/12.5e-9;
   cntct2 = ticks*12.5e-9;
   dcntct = lcntct - cntct2;
   if (dcntct > 6.25e-9) ticks = ticks + 1;

   minstepticks = ticks/lmaxsteps; 
   mintimestep = minstepticks*12.5e-9;
   if (mintimestep < 1.0e-6) mintimestep=1.0e-6;

   stepticks = 8;
   timestep = stepticks*12.5e-9;
   nsteps = ticks/stepticks; 

/*initialize for amplitude calculation*/
    
   wstep=0;
   lastamp=0.0;
   duration=0.0;
   tduration=0.0;
   time = timestep - mintimestep/2.0;

/*calculate the amplitudes*/

   for (index=1; index <= nsteps; index++) {  
      amp=lcurve*tan(atan(lwidth/lcurve)*(1.0-2.0*time/lcntct));
      amp = level - amp;
     
/*round the amplitudes to the nearest unit of 1.0*/ 

      amp1 = (int) amp;
      if ((amp - (double) amp1)<0.5)
         amp = (double) amp1;
      else
         amp = ((double) amp1) + 1.0;

/*amplitudes are between 0 and 4095.0*/

      if (amp < 0.0) amp = 0.0;
      if (amp > 4095.0) amp = 4095.0;

/*increase the duration if the amp has not changed - write a step if it has*/

      if (duration >= mintimestep) { 
         if ((amp != lastamp) && (duration > 0.0)) {
            wstep = wstep + 1; 
            aphh_dur[wstep] = duration;
            aphh_amp[wstep] = lastamp;

            tduration = tduration + duration;
            duration = timestep;}
            
         else {
            duration = duration + timestep;}}
            
      else {
       duration = duration + timestep;}  
          
      lastamp = amp;
      time = time + timestep;}

/*write the last step*/

   wstep = wstep + 1;
   aphh_dur[wstep] = duration;
   aphh_amp[wstep] = lastamp;
   tduration = tduration + duration;

/*write waveform properties to the text window

   fprintf(stdout,"number of steps = %d steps.\n",nsteps);
   fprintf(stdout,"total waveform steps = %d steps.\n",wstep);
   fprintf(stdout,"width = %f microseconds\n",lcntct*1e6);
   fprintf(stdout,"total duration = %f microseconds\n",tduration*1e6);

/*begin pulse sequence statements*/ 

/* 1 - variable obs - constant dec*/
    
   if ((lchnl[0]=='1') && (lchnl[1]=='2')) {       
     
      decpwrf(level1);
      obspwrf(aphh_amp[1]);
      dps_off();
      xmtron(); 
      decon();  
      for (index=1; index <=wstep; index++) {
         obspwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      xmtroff(); 
      decoff();
      obspwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);}

/*#2 - variable obs - constant dec2*/

   if ((lchnl[0]=='1') && (lchnl[1]=='3')) {
         
      dec2pwrf(level1); 
      obspwrf(aphh_amp[1]);
      dps_off();
      xmtron();
      dec2on();      
      for (index=1; index <=wstep; index++) {
         obspwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      xmtroff(); 
      dec2off();
      obspwrf(level); 
      dps_on();     
      if (ltype[0]=='c')
         dps_show("sim3pulse", "cntct", lcntct, "no_pulse", lcntct, "cntct", lcntct);  
      else 
         dps_show("sim3shaped_pulse", "cntct", lcntct, "no_pulse", lcntct, "cntct", lcntct);} 
      
/*#3 - variable obs - constant dec3*/

   if ((lchnl[0]=='1') && (lchnl[1]=='4')) {
         
      dec3pwrf(level1); 
      obspwrf(aphh_amp[1]);
      dps_off();
      xmtron(); 
      dec3on();      
      for (index=1; index <=wstep; index++) {
         obspwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      xmtroff(); 
      dec3off();
      obspwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);}   

/*#4 - variable dec - constant obs*/

   if ((lchnl[0]=='2') && (lchnl[1]=='1')) {     
      
      obspwrf(level1);
      decpwrf(aphh_amp[1]); 
      dps_off(); 
      decon(); 
      xmtron();               
      for (index=1; index <=wstep; index++) {
         decpwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      decoff(); 
      xmtroff();
      decpwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);} 

/*#5 - variable dec - constant dec2*/

   if ((lchnl[0]=='2') && (lchnl[1]=='3')) {
      
      dec2pwrf(level1); 
      decpwrf(aphh_amp[1]);
      dps_off();
      decon(); 
      dec2on();           
      for (index=1; index <=wstep; index++) {
         decpwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      decoff(); 
      dec2off();
      decpwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("sim3pulse", "no_pulse", lcntct, "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("sim3shaped_pulse", "no_pulse", lcntct, "cntct", lcntct, "cntct", lcntct);} 
      
/*#6 - variable dec - constant dec3*/

   if ((lchnl[0]=='2') && (lchnl[1]=='4')) {
      
      dec3pwrf(level1); 
      decpwrf(aphh_amp[1]);
      dps_off();
      decon(); 
      dec3on();           
      for (index=1; index <=wstep; index++) {
         decpwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      decoff(); 
      dec3off();
      decpwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);} 

/*#7 - variable dec2 - constant obs*/

   if ((lchnl[0]=='3') && (lchnl[1]=='1')) {      
      
      obspwrf(level1); 
      dec2pwrf(aphh_amp[1]);
      dps_off();
      dec2on(); 
      xmtron();    
      for (index=1; index <=wstep; index++) {
         dec2pwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      dec2off(); 
      xmtroff();
      dec2pwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("sim3pulse", "cntct", lcntct, "no_pulse", lcntct, "cntct", lcntct);  
      else 
         dps_show("sim3shaped_pulse", "cntct", lcntct, "no_pulse", lcntct, "cntct", lcntct);} 

/*#8 - variable dec2 - constant dec*/

   if ((lchnl[0]=='3') && (lchnl[1]=='2')) {     
      
      decpwrf(level1);
      dec2pwrf(aphh_amp[1]);
      dps_off();  
      dec2on(); 
      decon();      
      for (index=1; index <=wstep; index++) {
         dec2pwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      dec2off(); 
      decoff();
      dec2pwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("sim3pulse", "no_pulse", lcntct, "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("sim3shaped_pulse", "no_pulse", lcntct, "cntct", lcntct, "cntct", lcntct);}
      
/*#9 - variable dec2 - constant dec3*/

   if ((lchnl[0]=='3') && (lchnl[1]=='4')) {     
      
      dec3pwrf(level1);
      dec2pwrf(aphh_amp[1]);
      dps_off();  
      dec2on(); 
      dec3on();      
      for (index=1; index <=wstep; index++) {
         dec2pwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      dec2off(); 
      dec3off();
      dec2pwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);}
      
/*#10 - variable dec3 - constant obs*/

   if ((lchnl[0]=='4') && (lchnl[1]=='1')) {     
      
      obspwrf(level1);
      dec3pwrf(aphh_amp[1]);
      dps_off();  
      dec3on(); 
      xmtron();      
      for (index=1; index <=wstep; index++) {
         dec3pwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      dec3off(); 
      xmtroff();
      dec3pwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);}
      
/*#11 - variable dec3 - constant dec*/

   if ((lchnl[0]=='4') && (lchnl[1]=='2')) {     
      
      decpwrf(level1);
      dec3pwrf(aphh_amp[1]);
      dps_off();  
      dec3on(); 
      decon();      
      for (index=1; index <=wstep; index++) {
         dec3pwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      dec3off(); 
      decoff();
      dec3pwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);}
      
/*#12 - variable dec3 - constant dec2*/

   if ((lchnl[0]=='4') && (lchnl[1]=='3')) {     
      
      dec2pwrf(level1);
      dec3pwrf(aphh_amp[1]);
      dps_off();  
      dec3on(); 
      dec2on();      
      for (index=1; index <=wstep; index++) {
         dec3pwrf(aphh_amp[index]);
         delay(aphh_dur[index]);}
         
      dec3off(); 
      dec2off();
      dec3pwrf(level);
      dps_on();
      if (ltype[0]=='c')
         dps_show("simpulse", "cntct", lcntct, "cntct", lcntct);  
      else 
         dps_show("simshaped_pulse", "cntct", lcntct, "cntct", lcntct);}   
}

/*finish the tancp function*/ 






