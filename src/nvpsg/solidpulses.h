/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SOLIDPULSES_H
#define SOLIDPULSES_H

// Contents:
// getpulse() - SHAPE       Build a Generic Pulse
// getdfspulse - SHAPE      Build a Double-Frequency Sweep Pulse
// getsfspulse - SHAPE      Build a Single-Frequency Sweep Pulse
// getsinc() - SHAPE        Build a SINC Shaped Pulse
// gettanramp() - SHAPE     Build a Tangent-Ramped Shaped Pulse
// getcpm() - SHAPE         Build a Cosine Phase-Modulated Pulse
// getdumbogenshp() - SHAPE Build a DUMBO pulse from coefficients

//================================
// Get a Constant Amplitude Pulse
//================================

SHAPE getpulse(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = const_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getpulse(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

//pw (pulse length)

   var = getname0("pw",s.pars.seqName,"");
   double lpw = getval(var);
   lpw = roundoff(lpw,s.pars.n90*DTCK);
   s.pars.t = lpw;
   s.pars.array = disarry(var, s.pars.array);

//q (number of elements)

   s.pars.nelem = 1;

//ch (channel)

   var = getname0("ch",s.pars.seqName,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

   s = genericInitShape(s,seqName,p,phint,iRec);
   return s;
}

//=====================
// get a DFS Function
//=====================

SHAPE getdfspulse(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = dfs_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getdfspulse(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes
  
   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

//pwXdfs (dfs pulse length)

   var = getname0("pw",s.pars.seqName,"");
   double lpw = getval(var);
   lpw = roundoff(lpw,s.pars.n90*DTCK);
   s.pars.t = lpw;
   s.pars.array = disarry(var, s.pars.array);

//of1Xdfs

   var = getname0("of1",s.pars.seqName,"");
   double loffset1 = getval(var);
   s.pars.dp[0] = loffset1;
   s.pars.array = disarry(var, s.pars.array);

//of2Xdfs

   var = getname0("of2",s.pars.seqName,"");
   double loffset2 = getval(var);
   s.pars.array = disarry(var, s.pars.array);

   double sweeprate = 0.0;
   if (lpw > 0.0) sweeprate = (loffset2 - loffset1)/lpw;
   s.pars.dp[1] = sweeprate;

//q (number of elements)

   s.pars.nelem = 1;

//chXdfs

   var = getname0("ch",s.pars.seqName,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

   s = genericInitShape(s,seqName,p,phint,iRec);
   return s;
}

//=====================
// get an SFS Function
//=====================

SHAPE getsfspulse(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = sfs_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getsfspulse(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

//pwXsfs (dfs pulse length)

   var = getname0("pw",s.pars.seqName,"");
   double lpw = getval(var);
   lpw = roundoff(lpw,s.pars.n90*DTCK);
   s.pars.t = lpw;
   s.pars.array = disarry(var, s.pars.array);

//of1Xsfs

   var = getname0("of1",s.pars.seqName,"");
   double loffset1 = getval(var);
   s.pars.dp[0] = loffset1;
   s.pars.array = disarry(var, s.pars.array);

//of2Xsfs

   var = getname0("of2",s.pars.seqName,"");
   double loffset2 = getval(var);
   s.pars.array = disarry(var, s.pars.array);

   double sweeprate = 0.0;
   if (lpw > 0.0) sweeprate = (loffset2 - loffset1)/lpw;
   s.pars.dp[1] = sweeprate;

//q (number of elements)

   s.pars.nelem = 1;

//chXdfs
   
   var = getname0("ch",s.pars.seqName,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

   s = genericInitShape(s,seqName,p,phint,iRec);
   return s;
}

//=====================
// get an SFM Function
//=====================

SHAPE getsfmpulse(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = sfm_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getsfmpulse(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

//pwXsfs (dfs pulse length)

   var = getname0("pw",s.pars.seqName,"");
   double lpw = getval(var);
   lpw = roundoff(lpw,s.pars.n90*DTCK);
   s.pars.t = lpw;
   s.pars.array = disarry(var, s.pars.array);

//of1Xsfs

   var = getname0("of1",s.pars.seqName,"");
   double loffset1 = getval(var);
   s.pars.dp[0] = loffset1;
   s.pars.array = disarry(var, s.pars.array);

//chXdfs
   
   var = getname0("ch",s.pars.seqName,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

   s = genericInitShape(s,seqName,p,phint,iRec);
   return s;
}

//=======================
// get a SINC Function
//=======================

SHAPE getsinc(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = sinc_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getsinc(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

//bandwidth

   var = getname0("bw",s.pars.seqName,"");
   s.pars.dp[0] = getval(var);
   if ( s.pars.dp[0] < 10 ) {
      printf("Error in getsinc. Bandwidth < 10! \n");
      psg_abort(1);
   }
   s.pars.array = disarry(var, s.pars.array);

//zero crossings

   var = getname0("nzc",s.pars.seqName,"");
   s.pars.ip[0] = getval(var);
   if ( s.pars.ip[0]%2) {
      printf("Error in getSINC. Number of zero crossings must be even! \n");
      psg_abort(1);
   }
   s.pars.array = disarry(var, s.pars.array);

//zero-crossing time

   s.pars.dp[1] = 1.0/s.pars.dp[0];

//time

   var = getname0("t",s.pars.seqName,"");
   s.pars.t = (double)s.pars.ip[0]/s.pars.dp[0];
   s = genericInitShape(s,s.pars.seqName,p,phint,iRec);
   return s;
}

//=============================
// get a Tangent-Ramp Function
//=============================

SHAPE gettanramp(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = tanramp_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getsfmpulse(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }
//delta

   var = getname0("d",s.pars.seqName,"");
   s.pars.dp[0] = getval(var);
   s.pars.array = parsearry(s.pars.array);

//beta

   var = getname0("b",s.pars.seqName,"");
   s.pars.dp[1] = getval(var);
   s.pars.array = parsearry(s.pars.array);

//time

   var = getname0("t",s.pars.seqName,"");
   s.pars.t = getval(var);
   s.pars.array = parsearry(s.pars.array);

//polarity

   var = getname0("pol",s.pars.seqName,"");
   Getstr(var,s.pars.flag2,sizeof(s.pars.flag2));
   s.pars.array = parsearry(s.pars.array);

//q (number of elements)

   s.pars.nelem = 1;

//shape : s.pars.flag1

   var = getname0("sh",s.pars.seqName,"");
   Getstr(var,s.pars.flag1,sizeof(s.pars.flag1));
   s.pars.array = parsearry(s.pars.array);

   if (strcmp(s.pars.flag1,"c") == 0) {
      s.pars.dp[1] = 1.0e12;
      s.pars.dp[0] = 0.0;
   }

   else if (strcmp(s.pars.flag1,"l") == 0) s.pars.dp[1] = 1.0e12;
   s = genericInitShape(s,s.pars.seqName,p,phint,iRec);
   return s;
}
//================================
// Get a CPM Pulse
//================================

SHAPE getcpm(char *seqName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = cpm_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getcpm(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

//pw (pulse length)

   var = getname0("pw",s.pars.seqName,"");
   double lpw = getval(var);
   lpw = roundoff(lpw,s.pars.n90*DTCK);   
   s.pars.dp[0] = lpw;
   s.pars.t = 2.0*lpw;
   s.pars.array = disarry(var, s.pars.array);

//ph (phase excursion)

   var = getname0("ph",s.pars.seqName,"");
   double lph = getval(var);
   s.pars.dp[1] = lph;
   s.pars.array = disarry(var, s.pars.array);

//q (number of elements)

   var = getname0("q",s.pars.seqName,"");
   s.pars.nelem = getval(var);
   s.pars.array = disarry(var, s.pars.array);

//ch (channel)

   var = getname0("ch",s.pars.seqName,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

   s = genericInitShape(s,seqName,p,phint,iRec);
   return s;
}

//================================
// Get a DUMBO Pulse
//================================

SHAPE getdumbogenshp(char *seqName, char *coeffName, double p, double phint, int iRec, int calc)
{
   SHAPE s;
   char *var;
   s.get_state = dumbo_state;

   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getdumbogenshp(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   sprintf(s.pars.seqName,"%s",seqName);
   s.pars.calc = calc;
   s.pars.array = parsearry(s.pars.array);

// Set the Step Sizes

   s.pars.n90 = VNMRSN90;
   s.pars.n90m = VNMRSN90M;
   s.pars.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      s.pars.n90 = INOVAN90;
      s.pars.n90m = INOVAN90M;
      s.pars.trap = INOVATRAP;
   }

   var = getname0("sc",coeffName,"");
   s.pars.array = disarry(var, s.pars.array);

//get Fourier coefficients
   char *coef[12]={"ca1","ca2","ca3","ca4","ca5","ca6","cb1","cb2","cb3","cb4","cb5","cb6"};
   int i;
   for (i=0; i<12; i++){
   	var = getname0(coef[i],coeffName,"");
   	s.pars.dp[i] = (double) getval(var);
	s.pars.array = disarry(var, s.pars.array);
   }
  

//pw (pulse length)

   var = getname0("pw",s.pars.seqName,"");
   double lpw = getval(var);
   lpw = roundoff(lpw,s.pars.n90*DTCK);   
   s.pars.dp[12] = lpw;
   s.pars.t = lpw;
   s.pars.array = disarry(var, s.pars.array);

//ph (phase excursion)

   var = getname0("ph",s.pars.seqName,"");
   double lph = getval(var);
   s.pars.dp[13] = lph;
   s.pars.array = disarry(var, s.pars.array);

//q (number of elements)

   var = getname0("q",s.pars.seqName,"");
   s.pars.nelem = getval(var);
   s.pars.array = disarry(var, s.pars.array);

//ch (channel)

   var = getname0("ch",s.pars.seqName,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

   s = genericInitShape(s,seqName,p,phint,iRec);
   return s;
}


#endif

