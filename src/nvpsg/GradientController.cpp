/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include <string.h>
#include "GradientController.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "cpsg.h"

extern "C" {

#include "safestring.h"

}

//
#define WriteWord( x ) pAcodeBuf->putCode( x )
#define putPattern( x ) pWaveformBuf->putCode( x )

extern "C" void download_master_decc_values(const char *);

extern double gxmax;
extern double gymax;
extern double gzmax;
extern double gmax;
extern double gtotlimit;
extern double gxlimit;
extern double gylimit;
extern double gzlimit;
extern double gradstepsz;
extern int    gxFlip;
extern int    gyFlip;
extern int    gzFlip;
extern double psi,phi,theta;
extern int    bgflag;

//#define DEBUG_DELAYS

//
// because the scaling is done before rotation, the 
// scale registers are not utilized with patterns etc.
// the application software is oblique ...
//
int GradientController::userToScale(double xx)
{
  double t;
  int l;
  // t = xx*16384.0/32767.0;
  t = xx;
  l = ((int) floor(t+0.49));
  l &= 0xffff; // 16 bits only.
  return(l);
}

int GradientController::ampTo16Bits(double xx)
{
  int temp;
  temp = ((int) floor(xx));
  temp &= 0xffff;
  return(temp);
}

int GradientController::getDACLimit()
{
  return dacLimit;
}

int GradientController::flushGridDelay()
{
  long long repeats,dur;
  int extra,trepeats;
  double x;
  // logic
  dur =  DelayAccumulator;
  // dur = 0 is a stop they are intercepted one level up..
  if (dur == 0L) 
    return(1);
  if (dur < (long long) GridTicks)
  {
     x = dur * 0.0125;
     //cout<< "Gradient Grid Timing Violation of " << x << " usec " << endl; 
     abort_message("Gradient time too small %8.3f microseconds. abort!\n",x);
     exit(-1);
  }
  repeats = dur / GridTicks;
  extra = dur % GridTicks; 
  if (extra > 0) 
    {
      //cout << "bozo nono" << endl; 
      repeats--;
      extra += GridTicks;
    }
  if (repeats == 1) 
  {
      add2Stream(LATCHKEY | DURATIONKEY | GridTicks);
      repeats = 0;
  }
  while (repeats > 0)
    {
      trepeats = repeats;
      if (repeats > 65530)   
         trepeats = 65530;
      add2Stream(LATCHKEY | (17 << 26) | ((trepeats) << 10) | GridTicks);
      //add2Stream(LATCHKEY | 0 ); /* a nop to flush */
      repeats -= trepeats;
    }
  if (extra > 0) 
      add2Stream(LATCHKEY | DURATIONKEY | extra);
  DelayAccumulator = 0L;
  return(1);
}

int GradientController::setTickDelay(long long ticks)
{
  DelayAccumulator += ticks;
  bigTicker   += ticks;
  smallTicker += ticks;

  return(1);
}
//  dangerous used to advance the gradient versus other
//  controllers
int GradientController::setNoDelay(double dur)
{
  long long ticks;
  ticks = calcTicks(dur);
  // omitting this advances the Gradient
  // DelayAccumulator += ticks;
  bigTicker   += ticks;
  smallTicker = ticks;  // seves as clear and add...
  return(1);
}

int GradientController::outputACode(int Code, int many, int *stream)
{  
  // the default clause does all the work - made for easy edits..
  switch (Code) {
    case (int) DLOAD:  case (int) RTOP: case (int) RT2OP: case (int) RT3OP:  break; 
    case (int) TASSIGN: case (int) TABLE: case (int) TPUT:  break; 
    default:  flushGridDelay(); // 
  }
  Controller::outputACode(Code,many,stream);
  return 0;
}


void GradientController::setGates(int GatePattern)
{
    flushGridDelay();
    add2Stream(GATEKEY | (GatePattern & 0x0f));    
}

void GradientController::setGrad(const char *which, double value)
{
   // config info might guide - set errors ..
   // gxFlip, gyFlip & gzFlip are used to negate gradients for wiring differences

   flushGridDelay();
   long long ago;
   ago = getBigTicker();

   int k;
   switch (*which) {
    case 'X':  case 'x': k = XGRADKEY; value = value * gxFlip; 
               Xpower.eventFinePowerChange(value/32767.0,ago);
               break;

    case 'Y':  case 'y': k = YGRADKEY; value = value * gyFlip; 
               Ypower.eventFinePowerChange(value/32767.0,ago);
               break;

    case 'Z':  case 'z': k = ZGRADKEY; value = value * gzFlip; 
               Zpower.eventFinePowerChange(value/32767.0,ago);
               break;

    case 'B':  case 'b': k = B0GRADKEY; break;

    default: k = 0;
   }
   if (k == 0)
     exit(-1); // needs improvement...
   add2Stream(k | ampTo16Bits(value));
}

void GradientController::setGradScale(const char *which, double value)
{
   // config info might guide - set errors ..
   int k;
   flushGridDelay();
   long long ago;
   ago = getBigTicker();

   switch (*which) {
    case 'X':  case 'x': k = XGRADSCALEKEY; 
               Xpower.eventFineCalibrateUpdate(value/16384.0, ago);
               break;

    case 'Y':  case 'y': k = YGRADSCALEKEY; 
               Ypower.eventFineCalibrateUpdate(value/16384.0, ago);
               break;

    case 'Z':  case 'z': k = ZGRADSCALEKEY; 
               Zpower.eventFineCalibrateUpdate(value/16384.0, ago);
               break;

    case 'B':  case 'b': k = B0GRADSCALEKEY; break;

    default: k = 0;
   }
   if (k == 0)
     exit(-1); // needs improvement...
   add2Stream(k | userToScale(value));
}

void GradientController::setVGrad(char *which, double step, int rtvar)
{
   int buffer[4];
   
   int k;
   flushGridDelay();
   switch (*which) {
    case 'x':   k = XGRADKEY; break;
    case 'y':   k = YGRADKEY; break;
    case 'z':   k = ZGRADKEY; break;
    default: k = 0;
   }
   if (k == 0)
     exit(-1); // needs improvement...
   buffer[0] = k;       // the amp multiplier...
   buffer[1] = rtvar;     // the rtvar to use
   buffer[2] = ampTo16Bits(step);
   outputACode(VGRADIENT,3,buffer); // out they go..
}

//
int GradientController::errorCheck(int checkType, double g1, double g2, double g3, double s1, double s2, double s3)
{
  return (0);
}

//
void GradientController::setRotArrayElement(int index1, int index2, double value)
{
  cout << "NOT YET IMPLEMENTED" << endl;
  return;
}
  
  
// what about which channel??? --- flag does the channel...
cPatternEntry *GradientController::resolveGrad1Pattern(char *nm, int flag, char *emsg, int action)
{
    int i,l;
    double f1,f2,f3,f4,f5,scalef;
    int wcount, eventcount, repeats;
    // do we already know the pattern??
    cPatternEntry *tmp = find(nm, flag);
    scalef = GRADMAXPLUS;
    if (tmp != NULL)
      return(tmp);
    wcount = 0; eventcount = 0;

    i = findPatternFile(nm);
     if (i != 1)
     {
	   if (action > -1)
             abort_message("gradient pattern %s.GRD not found. abort",nm);
           else
             text_message("gradient pattern %s.GRD not found. abort",nm);

           return(NULL);  
     }

     if (pWaveformBuf == NULL)
         abort_message("waveform buffer is null for GradientController. abort!\n");

     pWaveformBuf->startSubSection(PATTERNHEADER);

       // read in the file
    do
    {  //  grad == amp dwell gates...
       l = scanLine(&f1,&f2,&f3,&f4,&f5);
       if (l <= 0) break;
       // l = 0 skip, l = -1 done.. or error..

       if ((l > 1) && (f2 < 0.5))    // verify custom....
       {
          scalef = f1;
          continue;
       }
       if (l == 3)
       {
          putPattern(GATEKEY | (((int) f3) & 0xf));
          wcount++;
       }
       repeats = 1;
       if (l > 1)
           repeats = ((int) f2) & 0xff;
 
       while (repeats >= 1)
       {
          // adjust this to the scalef
          putPattern(GRADAMPMASK | ampTo16Bits(f2*GRADMAXPLUS/scalef) | LATCHKEY);
          wcount++;
          eventcount++;
          repeats--;
       }

      }  while (l > -1);
     if (eventcount > 0)
     {
        tmp = new cPatternEntry(nm,flag,eventcount,wcount);
        addPattern(tmp);
        sendPattern(tmp);
      }
    return(tmp);
}

//
//
//
cPatternEntry *GradientController::resolveOblShpGrdPattern(char *nm, int flag, const char *emsg, int action)
{
    int i,l;
    double f1,f2,f3,f4,f5,scalef;
    double grad_rms_value;
    int wcount, eventcount, repeats;
    char tname[400];
    // do we already know the pattern??
    cPatternEntry *tmp = find(nm, flag);
    scalef = GRADMAXPLUS;
    if (tmp != NULL)
      return(tmp);
    OSTRCPY( tname, sizeof(tname), nm);
    OSTRCAT( tname, sizeof(tname), ".GRD");
    wcount = 0; eventcount = 0;
    grad_rms_value = 0.0;

    i = findPatternFile(tname);
     if (i != 1)
     {
           abort_message("gradient pattern %s.GRD not found. abort",nm);
     }

    if (pWaveformBuf == NULL)
        abort_message("waveform buffer is null for GradientController. abort!\n");

    pWaveformBuf->startSubSection(PATTERNHEADER);


    // read in the file
    int ival = 0;

    do
    {  //  grad == amp dwell gates...
       l = scanLine(&f1,&f2,&f3,&f4,&f5);
       if (l <= 0) break;
       // l = 0 skip, l = -1 done.. or error..

       if ((l > 1) && (f2 < 0.5))    // verify custom....
       {
          scalef = f1;
          continue;
       }
       if (l == 3)
         abort_message("invalid gradient waveform file format. abort!\n");

       repeats = 1;
       if (l > 1)
           repeats = ((int) f2) & 0xff;

       // adjust this to the scalef
       ival = (int) (floor(f1*GRADMAXPLUS/scalef));
       if ( (ival < -32767 ) || (ival > 32767) )
           abort_message("amplitude value outside +/-32767 range in gradient shape %s. abort!\n",tname);

       while (repeats >= 1)
       {
          putPattern(ival);
          grad_rms_value += ival*ival/1.0;   // divide by 1073676289 = 32767*32767 ?
          wcount++;
          eventcount++;
          repeats--;
       }

    }  while (l > -1);
     if (eventcount > 0)
     {
        tmp = new cPatternEntry(nm,flag,eventcount,wcount);
        tmp->setPowerFraction(sqrt(grad_rms_value/(double)eventcount));
        tmp->setLastElement(ival);
        addPattern(tmp);
        sendPattern(tmp);
        if (bgflag)
         cout << "GradientController pattern name " << nm << " eventcount = " << eventcount << " word count = " << wcount << endl;
      }
    return(tmp);

}
//
//
//
void GradientController::setEnable(char *cmd)
{
  //cout << "GradientController set enable not implemented " << endl;
}

//
//
//
int GradientController::initializeExpStates(int setupflag) {
	if (var_active("gpropdelay", CURRENT) == 1) {
		int buffer[4];
	        double gdelays[4] = { 0 };
		double max_delay = 0;
		double gprop = 0;
		P_getreal(CURRENT, "gxdelay", &gdelays[0], 1);
		P_getreal(CURRENT, "gydelay", &gdelays[1], 1);
		P_getreal(CURRENT, "gzdelay", &gdelays[2], 1);
		P_getreal(CURRENT, "b0delay", &gdelays[3], 1);
		P_getreal(CURRENT, "gpropdelay", &gprop, 1);
		for (int i = 0; i < 4; i++) {
			gdelays[i] = (gdelays[i] < 0) ? 0 : gdelays[i];
			max_delay = gdelays[i] > max_delay ? gdelays[i] : max_delay;
		}
                if (max_delay > gprop) {
			text_message("Advisory: gxdelay, gydelay, gzdelay, or b0delay is larger than gpropdelay");
                }
#ifdef DEBUG_DELAYS
		printf("prop-delays grad:%g X:%g Y:%g Z:%g B0:%g max:%g\n",
				gprop*1e6,gdelays[0]*1e6,gdelays[1]*1e6,gdelays[2]*1e6,gdelays[3]*1e6,max_delay*1e6);
#endif
		for (int i = 0; i < 4; i++) {
			buffer[i] = (int) (gdelays[i] * 80e6 + 0.5); // convert us to ticks
		}
#ifdef DEBUG_DELAYS
		printf("delta delays X:%g(%d) Y:%g(%d) Z:%g(%d) B0:%g(%d)\n",
				buffer[0]/80.0,buffer[0],
				buffer[1]/80.0,buffer[1],
				buffer[2]/80.0,buffer[2],
				buffer[3]/80.0,buffer[3]);
#endif
		outputACode(GRAD_DELAYS, 4, buffer);
	}
#ifdef DEBUG_DELAYS
	else
		printf("grad_delay array not created\n");
#endif

	dacLimit = 0x7fff;

	char estring[42];
	P_getstring(GLOBAL, "pfgon", estring, 1, 40);
	/* set enable is at parse time - */
	setEnable(estring);

	/* Get gmax or gxmax, gymax, gzmax */
	if (P_getreal(CURRENT, "gxmax", &gxmax, 1) < 0) {
		if (P_getreal(CURRENT, "gmax", &gmax, 1) < 0)
			gmax = 0;
		gxmax = gmax;
	}
	if (P_getreal(CURRENT, "gymax", &gymax, 1) < 0) {
		if (P_getreal(CURRENT, "gmax", &gmax, 1) < 0)
			gmax = 0;
		gymax = gmax;
	}
	if (P_getreal(CURRENT, "gzmax", &gzmax, 1) < 0) {
		if (P_getreal(CURRENT, "gmax", &gmax, 1) < 0)
			gmax = 0;
		gzmax = gmax;
	}
	if (gxmax < gzmax)
		if (gxmax < gymax)
			gmax = gxmax;
		else
			gmax = gymax;
	else if (gzmax < gymax)
		gmax = gzmax;
	else
		gmax = gymax;

	GMAX = gmax;

	GMAX_TO_DAC = 32768.0 / GMAX;

	if ((gxmax != 0.0) && (GMAX != 0.0))
		GXSCALE = (int) ((GMAX / gxmax) * 0x4000);
	else
		GXSCALE = 0x4000;
	if ((gymax != 0.0) && (GMAX != 0.0))
		GYSCALE = (int) ((GMAX / gymax) * 0x4000);
	else
		GYSCALE = 0x4000;
	if ((gzmax != 0.0) && (GMAX != 0.0))
		GZSCALE = (int) ((GMAX / gzmax) * 0x4000);
	else
		GZSCALE = 0x4000;

	if (bgflag) {
		cout << "setting GXSCALE=" << GXSCALE << " GYSCALE=" << GYSCALE
				<< " GZSCALE=" << GZSCALE << endl;
		cout << "gmax=" << gmax << " gxmax=" << gxmax << " gymax=" << gymax
				<< " gzmax=" << gzmax << endl;
	}

	/* --- Get gradient limit parameters --- */
	if (P_getreal(CURRENT, "gtotlimit", &gtotlimit, 1) < 0)
		gtotlimit = gxmax + gymax + gzmax;
	if (gtotlimit <= 0.0)
		gtotlimit = gxmax + gymax + gzmax;

	if (P_getreal(CURRENT, "gxlimit", &gxlimit, 1) < 0)
		gxlimit = gxmax;
	if (gxlimit <= 0.0)
		gxlimit = gxmax;

	if (P_getreal(CURRENT, "gylimit", &gylimit, 1) < 0)
		gylimit = gymax;
	if (gylimit <= 0.0)
		gylimit = gymax;

	if (P_getreal(CURRENT, "gzlimit", &gzlimit, 1) < 0)
		gzlimit = gzmax;
	if (gzlimit <= 0.0)
		gzlimit = gzmax;

	Xpower.setMainCalibrate(1.0);
	Xpower.setMainPowerFactor(0.0);
	Xpower.setTimeConstant(1.0e6);
	Ypower.setMainCalibrate(1.0);
	Ypower.setMainPowerFactor(0.0);
	Ypower.setTimeConstant(1.0e6);
	Zpower.setMainCalibrate(1.0);
	Zpower.setMainPowerFactor(0.0);
	Zpower.setTimeConstant(1.0e6);
	Xpower.setGateONOFF(1);
	Ypower.setGateONOFF(1);
	Zpower.setGateONOFF(1);

	setGrad("X", 0.0);
	setGrad("Y", 0.0);
	setGrad("Z", 0.0);
	setGrad("B", 0.0);
	setGradScale("X", GXSCALE); // 0x4000
	setGradScale("Y", GYSCALE); // 0x4000
	setGradScale("Z", GZSCALE); // 0x4000
	setGradScale("B", 0x4000); // limited use
	setTickDelay((long long) 320); /* enforced minimum time */
	download_master_decc_values("grad1");
	psi = getvalnwarn("psi");
	phi = getvalnwarn("phi");
	theta = getvalnwarn("theta");
	set_rotation_matrix(psi, phi, theta);

	return 0;
}
//
//
//
int GradientController::initializeIncrementStates(int num)
{ 
  setTickDelay((long long) num);
  return(num);
}

void GradientController::getPowerIntegral(double *gradpowerarray)
{
	gradpowerarray[0] = Xpower.getPowerIntegral();
	gradpowerarray[1] = Ypower.getPowerIntegral();
	gradpowerarray[2] = Zpower.getPowerIntegral();
}
