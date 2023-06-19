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
#include "PFGController.h"
#include "FFKEYS.h"
#include "ACode32.h"
#include "cpsg.h"

extern "C" {

#include "safestring.h"

}

//
#define WriteWord( x )  pAcodeBuf->putCode( x )
#define putPattern( x ) pWaveformBuf->putCode( x )

#define DBG if (bgflag) printf

extern double gxmax;
extern double gymax;
extern double gzmax;
extern double gmax;
extern double gtotlimit;
extern double gxlimit;
extern double gylimit;
extern double gzlimit;
extern double gradstepsz;
extern double psi,phi,theta;
extern int    bgflag;

// - the math layer does 
//   16 X 16 multiply and shifts down by 15!
//   this allows for an exact full scale + and -
//   at the cost of resolution on the scaling only
//   
//   in addition, we cut the Triax in half.
//   
int PFGController::userToScale(double xx)

{
  double t;
  int l;
  // t = xx*16384.0/32767.0;
  t = xx;
  l = ((int) floor(t+0.49));
  if ((strstr(gradType,"t")) || (strstr(gradType,"u"))) l >>= 1; 

  l &= 0xffff; // 16 bits only.
  return(l);
}

int PFGController::ampTo16Bits(double xx)
{
  int temp;
  if (strstr(gradType,"l"))
  {
      if ((xx > 2047.5) || (xx < -2047.5))
        abort_message("low power PFG (%g) out of range (+- 2047)",xx);
      xx *= 16;
  }
  temp = ((int) floor(xx));
  temp &= 0xffff;

  return(temp);
}

int PFGController::getDACLimit()
{
  return dacLimit;
}

int PFGController::isPresent(char x)
{
  if ((x == 'x')  || (x == 'X'))
    return(usageFlag & XPFGPRESENT); 
  if ((x == 'y')  || (x == 'Y'))
    return(usageFlag & YPFGPRESENT);
  if ((x == 'z')  || (x == 'Z'))
    return(usageFlag & ZPFGPRESENT);
  return(0);  // it failed ...
}

int PFGController::setEnable(char *key)
{
  int buffer[4];
  char errorstring[128];
  OSTRCPY( errorstring, sizeof(errorstring), "");
  
  buffer[0] = 0;  /* everyone off */
  if (tolower(key[0]) == 'y') 
    {
      if (!isPresent('x'))
        OSTRCAT( errorstring, sizeof(errorstring), "no X gradient ");
	 buffer[0] |= XPFGPRESENT;
    }
  if (tolower(key[1]) == 'y') 
    {
      if  (!isPresent('y'))
        OSTRCAT( errorstring, sizeof(errorstring), "no Y gradient ");
	 buffer[0] |= YPFGPRESENT;
    }
  if (tolower(key[2]) == 'y') 
    {
      if  (!isPresent('z'))
        OSTRCAT( errorstring, sizeof(errorstring), "no Z gradient ");
	 buffer[0] |= ZPFGPRESENT;
    }
  if (strlen(errorstring) > 1) 
    {
      abort_message("bad gradient configuration %8s\n",errorstring);
    }
  // power watch gate on or
  outputACode(PFGENABLE,1,buffer);
  return(0);
} 

void PFGController::setExpireTime(long long expT)
{
  wait4meExpireTicker = expT;
} 
//
// This overrides base class ..
// the implementation only works for one axis currently...
//
int PFGController::setTickDelay(int ticks)
{
  long long dif;
  // all paths do this... 
  bigTicker += ticks;
  smallTicker += ticks;
  if (wait4meExpireTicker == 0L)
  {
   add2Stream(DURATIONKEY | LATCHKEY | ticks);
   return(DURATIONKEY | LATCHKEY | ticks);
  }
  else
  // untested ....
  if ((bigTicker + ticks) > wait4meExpireTicker)
  {
	  dif = bigTicker - wait4meExpireTicker;
          if ((dif > 0L) && (dif < 4L)) 
            exit(-1);  // IMPROVE 
          if (dif != 0L)   // don't add a 0...
            add2Stream(DURATIONKEY | LATCHKEY | dif);
          // we are done clean up...
          wait4meExpireTicker = 0L;  
          setSync();
  }
  return(0);
}

//
//  usage still TBD
//
void PFGController::setGates(int GatePattern)
{
    add2Stream(PFGGATESKEY | (GatePattern & 0x0f));    
}
/* by convention, patterns use amp and are scaled by ampscale..
   single events use ampscale and set amp to full 
*/

void PFGController::setGrad(const char *which, double value)
{
   // config info to check valid TBD
   long long ago = getBigTicker();
   int k, encoding;

   switch (*which) {
     case 'X': case 'x': 
        k = XPFGAMPKEY; 
        Xpower.eventFinePowerChange(value/32767.0,ago); 
       break;
     case 'Y': case 'y': 
        k = YPFGAMPKEY; 
        Ypower.eventFinePowerChange(value/32767.0,ago); 
        break;
     case 'Z': case 'z': 
        k = ZPFGAMPKEY; 
        Zpower.eventFinePowerChange(value/32767.0,ago); 
    break;
    default: k = 0;
   }
   if (k == 0)
     exit(-1); // needs improvement...
   encoding = k | ampTo16Bits(value);
   outputACode(PFGSETZLVL, 1, &encoding);
}

int PFGController::setPfgCorrection()
{
  char gradcorr[MAXSTR];
  double corr_dbl[2];
  int corr[2] = {0,0};
  int err;
  if ((err = P_getstring(CURRENT,"veripulse_gradcorr", gradcorr, 1, MAXSTR)) < 0)
  {
    DBG("veripulse_gradcorr not in current tree (%d)\n", err);
    if ((err = P_getstring(SYSTEMGLOBAL,"gradcorr", gradcorr, 1, MAXSTR)) < 0) {
      DBG("gradcorr not in systemglobal tree (%d)\n", err);
      return -1;
    }
  }
  DBG("gradcorr = %s\n", gradcorr);

  if (!strcmp(gradcorr,"y")) {
    if ( P_getreal(CURRENT,"veripulse_gzlvlcorr_hi",&corr_dbl[0], 1) < 0 )
      if ( P_getreal(SYSTEMGLOBAL,"gzlvlcorr_hi",&corr_dbl[0], 1) < 0 )
	corr_dbl[0] = 0.0;
    corr[0] = (int) corr_dbl[0];
    if ( P_getreal(CURRENT,"veripulse_gzlvlcorr_lo",&corr_dbl[1], 1) < 0 )
      if ( P_getreal(GLOBAL,"gzlvlcorr_lo",&corr_dbl[1], 1) < 0 )
	corr_dbl[1] = 0.0;
    corr[1] = (int) corr_dbl[1];
    DBG("corr = [%d,%d]\n", corr[0], corr[1]);
    outputACode(PFGSETZCORR, 2, corr);
    return 0;
  }
  return 1;
}

void PFGController::setGradScale(const char *which, double value)
{
   // config info to check valid TBD
   int k;
   long long ago;
   ago = getBigTicker();
   switch (*which) {
     case 'X': case 'x': 
        k = XPFGAMPSCALEKEY; 
        Xpower.eventFineCalibrateUpdate(value/16384.0, ago);
     break;
     case 'Y': case 'y': 
        k = YPFGAMPSCALEKEY;
        Ypower.eventFineCalibrateUpdate(value/16384.0, ago);
     break;
     case 'Z': case 'z': 
       k = ZPFGAMPSCALEKEY;
       Zpower.eventFineCalibrateUpdate(value/16384.0, ago);
     break;
     default: k = 0;
   }
   if (k == 0)
     exit(-1); // needs improvement...
   add2Stream(k | userToScale(value));
}

void PFGController::setVGrad(char *which, double step, int rtvar)
{
   int buffer[4];
   
   int k;
// config info to check valid TBD
   switch (*which) {
    case 'X': case 'x':   isPresent('x');  k = XPFGAMPSCALEKEY; break;
    case 'Y': case 'y':   isPresent('y');  k = YPFGAMPSCALEKEY; break;
    case 'Z': case 'z':   isPresent('z');  k = ZPFGAMPSCALEKEY; break;
    default: k = 0;
   }
   if (k == 0)
     exit(-1); // needs improvement...
   buffer[0] = k;       // the amp multiplier...
   buffer[1] = rtvar;     // the rtvar to use
   buffer[2] = ampTo16Bits(step);
   outputACode(PFGVGRADIENT,3,buffer); // out they go..
}
  
//
int PFGController::errorCheck(int checkType, double g1, double g2, double g3, double s1, double s2, double s3)
{
  return (0);
}

// what about which channel??? --- flag does the channel...
cPatternEntry *PFGController::resolveGrad1Pattern(char *nm, int flag, char *emsg, int action)
{
    int i,l;
    double f1,f2,f3,f4,f5,scalef;
    int wcount, eventcount, repeats;
    int gradampmask;
    char tname[400];
    // do we already know the pattern??
    cPatternEntry *tmp = find(nm, flag);
    scalef = PFGMAXPLUS;
    if (tmp != NULL)
      return(tmp);
    OSTRCPY( tname, sizeof(tname), nm);
    OSTRCAT( tname, sizeof(tname), ".GRD");
    wcount = 0; eventcount = 0;

    i = findPatternFile(tname);
     if (i != 1)
	 {
	   abort_message("pfg pattern %s.GRD not found. abort",nm);
         }

     patternCount++;
     pWaveformBuf->startSubSection(PATTERNHEADER);

       // read in the file
     switch (flag)
       {
          case 1: gradampmask = XPFGAMPKEY; break;
          case 2: gradampmask = YPFGAMPKEY; break;
          case 3: gradampmask = ZPFGAMPKEY; break;
          default:  abort_message("bad pfg gradient specified %d. abort.",flag);
	 exit(-1);
       }
       
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
          int value = GATEKEY | (((int) f3) & 0xf) ;
          putPattern( value );
          wcount++;
       }
       repeats = 1;
       if (l > 1)
           repeats = ((int) f2) & 0xff;
       if (l > 0)
       {
           // adjust this to the scalef
          putPattern(gradampmask | ampTo16Bits(f1*PFGMAXPLUS/scalef) | LATCHKEY);
           wcount++;
           eventcount++;
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


cPatternEntry *PFGController::resolveOblShpGrdPattern(char *nm, int flag, const char *emsg, int action)
{
    int i,l;
    double f1,f2,f3,f4,f5,scalef;
    double grad_rms_value;
    int wcount, eventcount, repeats;
    char tname[400];
    // do we already know the pattern??
    cPatternEntry *tmp = find(nm, flag);
    scalef = PFGMAXPLUS;
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
        abort_message("waveform buffer is null for PFGController. abort!\n");

    patternCount++;
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
       ival = (int) (floor(f1*PFGMAXPLUS/scalef));
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
         cout << "PFGController pattern name " << nm << " eventcount = " << eventcount << " word count = " << wcount << endl;
      }
    return(tmp);

}



int PFGController::initializeExpStates(int setupflag)
{
  char estring[42];

  P_getstring(GLOBAL,"pfgon",estring,1,40);
  /* set enable is at parse time - */
  setEnable(estring);

  if (gradType[2] == 'l')
    dacLimit = 0x7ff;
  else
    dacLimit = 0x7fff;

  setPfgCorrection();

    /* Get gmax or gxmax, gymax, gzmax */
    if ( P_getreal(CURRENT,"gxmax",&gxmax,1) < 0 )
    {
        if ( P_getreal(CURRENT,"gmax",&gmax,1) < 0 )
           gmax = 0;
        gxmax = gmax;
    }
    if ( P_getreal(CURRENT,"gymax",&gymax,1) < 0 )
    {
        if ( P_getreal(CURRENT,"gmax",&gmax,1) < 0 )
           gmax = 0;
        gymax = gmax;
    }
    if ( P_getreal(CURRENT,"gzmax",&gzmax,1) < 0 )
    {
        if ( P_getreal(CURRENT,"gmax",&gmax,1) < 0 )
           gmax = 0;
        gzmax = gmax;
    }
    if (gxmax < gzmax)
        if (gxmax < gymax)
           gmax = gxmax;
        else
           gmax = gymax;
    else
        if (gzmax < gymax)
           gmax = gzmax;
        else
           gmax = gymax;

  if (gxmax > gzmax)
     if (gxmax > gymax)
        GMAX = gxmax;
     else
        GMAX = gymax;
  else
     if (gzmax > gymax)
        GMAX = gzmax;
     else
        GMAX = gymax;

  GMAX_TO_DAC = 32768.0/gmax;

  if ((gxmax != 0.0) && (gmax != 0.0))
     GXSCALE = (int)((gmax/gxmax)*0x4000);
  else
     GXSCALE = 0x4000;
  if ((gymax != 0.0) && (gmax != 0.0))
     GYSCALE = (int)((gmax/gymax)*0x4000);
  else
     GYSCALE = 0x4000;
  if ((gzmax != 0.0) && (gmax != 0.0))
     GZSCALE = (int)((gmax/gzmax)*0x4000);
   else
     GZSCALE = 0x4000;

  if (bgflag)
  {
     printf("setting GXSCALE=%d  GYSCALE=%d  GZSCALE=%d\n",GXSCALE,GYSCALE,GZSCALE);
     printf("gmax=%g  gxmax=%g  gymax=%g  gzmax=%g\n",gmax,gxmax,gymax,gzmax);
  }

    /* --- Get gradient limit parameters --- */
    if ( P_getreal(CURRENT,"gtotlimit",&gtotlimit,1) < 0 )
        gtotlimit = gxmax + gymax + gzmax;
    if (gtotlimit <= 0.0) gtotlimit = gxmax + gymax + gzmax;

    if ( P_getreal(CURRENT,"gxlimit",&gxlimit,1) < 0 )
        gxlimit = gxmax;
    if (gxlimit <= 0.0) gxlimit = gxmax;

    if ( P_getreal(CURRENT,"gylimit",&gylimit,1) < 0 )
        gylimit = gymax;
    if (gylimit <= 0.0) gylimit = gymax;

    if ( P_getreal(CURRENT,"gzlimit",&gzlimit,1) < 0 )
        gzlimit = gzmax;
    if (gzlimit <= 0.0) gzlimit = gzmax;

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

  setGrad("X",0.0);
  setGrad("Y",0.0);
  setGrad("Z",0.0);
  setGradScale("X",0x4000);   /* set to default max here */
  setGradScale("Y",0x4000);   /* set to default max here */
  setGradScale("Z",0x4000);   /* set to default max here */

  setTickDelay((long long) 240);  /* enforced minimum time */

  psi   = getvalnwarn("psi");
  phi   = getvalnwarn("phi");
  theta = getvalnwarn("theta");
  set_rotation_matrix(psi,phi,theta);

  // there's a note about 40 joules total power 
  // that must ignore the leak because 1.1 joule is
  // the full power 10 ms limit..
#ifdef POWERLIMIT 
  Xpower.setAlarmLevel(1.1,POWER_ACTION_ABORT,"X pfg ");
  Ypower.setAlarmLevel(1.1,POWER_ACTION_ABORT,"Y pfg ");
  Zpower.setAlarmLevel(1.1,POWER_ACTION_ABORT,"Z pfg ");
#endif
  return 0;
}

// num > 240 ticks!!! else pfg throws a interrupt.
//
int PFGController::initializeIncrementStates(int num)
{ 
  setTickDelay((long long) num);
  return(0);
}

//
//
//
void PFGController::showPowerIntegral()
{
  double xpower, xfraction, ypower, yfraction, zpower, zfraction;
  long long now = getBigTicker();
  double ntval = getval("nt");
 
  if (isPresent('x') && (&Xpower != NULL))
  {
    xpower = Xpower.getPowerIntegral();
    xfraction = xpower*100.0/(now*12.5e-9*ntval);
    printf("%6s: X axis Energy=%10.4g\n",getName(),xpower);
  }

  if (isPresent('y') && (&Ypower != NULL))
  {
    ypower = Ypower.getPowerIntegral();
    yfraction = ypower*100.0/(now*12.5e-9*ntval);
    printf("%6s: Y axis Energy=%10.4g\n",getName(),ypower);
  }

  if (isPresent('z') && (&Zpower != NULL))
  {
    zpower = Zpower.getPowerIntegral();
    zfraction = zpower*100.0/(now*12.5e-9*ntval);
    printf("%6s: Z axis Energy=%10.4g\n",getName(),zpower);
  }
}
//
//
//
void PFGController::showEventPowerIntegral(const char *comment)
{
  printf("Power info: %s %5s Energy   ",comment,getName());
  if (isPresent('x') && (&Xpower != NULL)) printf(" X=%8.4g ",Xpower.getEventPowerIntegral());
  if (isPresent('y') && (&Ypower != NULL)) printf(" Y=%8.4g ",Ypower.getEventPowerIntegral());
  if (isPresent('z') && (&Zpower != NULL)) printf(" Z=%8.4g ",Zpower.getEventPowerIntegral());
  printf("\n");
}
//
//
//
void PFGController::getPowerIntegral(double *gradpowerarray)
{
	if (isPresent('x'))
		gradpowerarray[0] = Xpower.getPowerIntegral();
	else
		gradpowerarray[0] = 0.0;
	if (isPresent('y'))
		gradpowerarray[1] = Ypower.getPowerIntegral();
	else
		gradpowerarray[1] = 0.0;
	if (isPresent('z'))
		gradpowerarray[2] = Zpower.getPowerIntegral();
	else
		gradpowerarray[2] = 0.0;
}
//
//
//
#ifdef BOGUS
//  - preliminary multi axes support using the shapedgradient nowait..  
  shaped gradient nowait call 
  read the pattern as normal..
--force memory copy if host based..
  if (list == null) create list
  if (branch == null) create branch
  add to branch a structure -
  pattern reference.
  element time mark bigTicker recommended..
  pattern dwell (time per state)
  cur state = -1 indicates not in use
  current time ??  
  


#endif
