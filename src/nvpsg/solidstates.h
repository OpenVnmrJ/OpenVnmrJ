/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SOLIDSTATES_H
#define SOLIDSTATES_H

//const_state - STATE           Calculate a Rectangular Pulse
//sinc_state() - STATE          Calculate a sinc Shaped Pulse
//tanramp_state() - STATE       Calculate a Tangent-Ramped Shaped Pulse
//dfs_state() - STATE           Calculate a Double-Frequency Sweep
//sfs_state() - STATE           Calculate a Single-Frequency Sweep
//cpm_state - STATE             Calculate a Cosine Phase-Modulated Pulse
//dumbo_state - STATE		Calculate a DUMBO pulse from form coefficients

//===============================
// Calculate a Constant Amplitude
//===============================

STATE const_state(double t, SHAPE_PARS p)
{
   STATE s;
   s.a = 1023.0;
   s.p = 0.0;
   s.g = 1.0;
   return s;
}

//===========================
// Calculate a SINC Function
//===========================

STATE sinc_state(double t, SHAPE_PARS p)
{
   STATE s;
   double tzc = p.dp[1];     //zero crossing time
   double arg;
   arg = PI*(t - 0.5*p.t)/tzc;
   if (fabs(arg) < 0.001) {
      s.a = 1023.0;
      s.p = 0.0;
      s.g = 1.0;
      return s;
   }
   s.a = 1023.0*sin(arg)/arg;
   s.p = 0.0;
   s.g = 1.0;
   if (s.a < 0) {
      s.a = -1.0*s.a;
      s.p = 180.0;
      s.g = 1.0;
   }
   return s;
}

//===========================
// Calculate a DFS Function
//===========================

STATE dfs_state(double t, SHAPE_PARS p)
{
   STATE s;
   double offset1 = p.dp[0];    //initial offset
   double sweeprate = p.dp[1];  //sweeprate
   double time = t;
   //double phase1 = 2.0*3.14159265358979323846*(offset1*time + sweeprate*time*time/2.0);
   double phase1 = 2.0*M_PI*(offset1*time + sweeprate*time*time/2.0);
   double phase = 0.0;
   double amp = 1023.0*cos(phase1);
   if (amp < 0.0)
   {
      amp = -amp;
      phase = 180.0;
   }
   amp = roundamp(amp,1.0/FSD);
   phase = roundphase(phase,360.0/(8192*PSD));
   double gate = 1.0;

   s.a = amp;
   s.p = phase;
   s.g = gate;

   return s;
}

//===========================
// Calculate a SFS Function
//===========================

STATE sfs_state(double t, SHAPE_PARS p)
{
   STATE s;
   double offset1 = p.dp[0];    //initial offset
   double sweeprate = p.dp[1];  //sweeprate
   double time = t;
   double phase = 360.0*(offset1*time + sweeprate*time*time/2.0);
   double amp = 1023.0;
   amp = roundamp(amp,1.0);
   phase = roundphase(phase,360.0/8192);
   double gate = 1.0;

   s.a = amp;
   s.p = phase;
   s.g = gate;

   return s;
}

STATE sfm_state(double t, SHAPE_PARS p) 
{
   STATE s;
   double offset1 = p.dp[0];    //plus/minus offset
   double taucp = p.t; 
   double time = t;
   double phase = taucp*offset1*360.0*(1.0 - cos(TWOPI*time/taucp)); 
   double amp = 1023.0;
   amp = roundamp(amp,1.0);
   phase = roundphase(phase,360.0/8192);
   double gate = 1.0;

   s.a = amp;
   s.p = phase;
   s.g = gate;

   return s;
}

//===================================
// Calculate a Tangent-Ramp FUNCTION
//===================================

STATE tanramp_state(double t, SHAPE_PARS p)
{
   double norm,mean;
   double at;
   enum polarity {NORMAL, UP_UP, DOWN_DOWN, DOWN_UP, UP_DOWN};
   enum polarity POL = NORMAL;
   STATE s;

   if(strcmp(p.flag2,"n") == 0 ) POL = NORMAL;
   if(strcmp(p.flag2,"uu") == 0 )POL = UP_UP;
   if(strcmp(p.flag2,"dd") == 0 )POL = DOWN_DOWN;
   if(strcmp(p.flag2,"du") == 0 ) POL = DOWN_UP;
   if(strcmp(p.flag2,"ud") == 0 )POL = UP_DOWN;

   mean = p.a;
   norm = 1023.0/(mean + fabs(p.dp[0]));
   s.p = 0.0;
   s.g = 1.0; 
   switch (POL) {
      case NORMAL:
         at = atan(p.dp[0]/p.dp[1]);
         s.a = norm*(mean - p.dp[1]*tan(at*(1.0 - 2.0*t/p.t)));
         break;
      case UP_DOWN:
         at = atan(fabs(p.dp[0])/p.dp[1]);
         s.a = norm*(mean - p.dp[1]*tan(at*(1.0 - 2.0*t/p.t)));
         break;
      case DOWN_UP:
         at = atan(-1.0*fabs(p.dp[0])/p.dp[1]);
         s.a = norm*(mean - p.dp[1]*tan(at*(1.0 - 2.0*t/p.t)));
         break;
      case UP_UP:
         at = atan( fabs(p.dp[0])/p.dp[1] );
         s.a = norm*(mean + fabs(p.dp[1]*tan(at*(1.0 - 2.0*t/p.t))));
         break;
      case DOWN_DOWN:
         at = atan(fabs(p.dp[0])/p.dp[1]);
         s.a = norm*(mean - fabs(p.dp[1]*tan(at*(1.0 - 2.0*t/p.t))));
         break;
      default:
         printf("Error in tanramp_state(). Cannot determine ramp polarity \n");
         psg_abort(1);
   }
   return s;
}

//====================================
// Calculate a Cosine Phase Modulation
//====================================

STATE cpm_state(double t, SHAPE_PARS p)
{
   STATE s;
   s.a = 1023.0;
//   printf("phase = %f pw = %f\n",p.dp[1],p.dp[0]*1e6);
   s.p = p.dp[1]*cos(TWOPI*t/(2.0*p.dp[0]));
   s.g = 1.0;
//   printf("t = %f p = %f\n",t*1e6,s.p);
   return s;
}

//=======================================
// Calculate DUMBO from form coefficients
//=======================================


STATE dumbo_state(double t, SHAPE_PARS p)
{
   STATE s;
   s.a = 1023.0;
//   printf("phase = %f pw = %f\n",p.dp[13],p.dp[12]*1e6);

   double halfperiod=p.dp[12]/2.0;
   double dumboscale = 0.0;
   if (t<=halfperiod) { // if we are in the first half of DUMBO
   	dumboscale=2*M_PI*(t-p.n90*p.n90m*DTCK/2.0)/halfperiod; //dp[12] - DUMBO duration pw
	printf("1st dumboscale: %f\n",dumboscale);
   	s.p=p.dp[13]; //dp[12] - initial phase ph
   	int j=0;
   	for (j=0; j<6; j++){
		s.p+=360*p.dp[j+6]*sin((j+1)*dumboscale)+360*p.dp[j]*cos((j+1)*dumboscale);
   	}
   }
   else {
       	dumboscale=2*M_PI*(p.dp[12]-t+p.n90*p.n90m*DTCK/2.0)/halfperiod;
        printf("2nd dumboscale: %f\n",dumboscale);
	s.p=p.dp[13]; //dp[12] - initial phase ph
   	int j=0;
   	for (j=0; j<6; j++){
		s.p+=360*p.dp[j+6]*sin((j+1)*dumboscale)+360*p.dp[j]*cos((j+1)*dumboscale);
   	}
        s.p+=180.0;
   }
   s.g = 1.0;
   printf("t = %f p = %f\n",t*1e6,s.p);
   return s;
}

#endif

