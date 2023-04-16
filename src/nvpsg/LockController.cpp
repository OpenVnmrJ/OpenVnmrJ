/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <math.h>
#include "LockController.h"
#include "ACode32.h"
//
//  the lock channel is alway usage = OFF 
//  
#define LOCKGAIN  501
#define LOCKPOWER 502
#define LOCKPHASE 503
#define LOCKFREQ  504

LockController::LockController(const char *name, int flags):Controller(name,flags)
{
  setOff();
  kind = LOCK_TAG;
  lockGain = 0;
  lockPower = 0;
  lockPhase = 0;
  lockOffsetFreq = 0; 
}


void LockController::setActive()
{
  return;
}

void LockController::setOff()
{ 
  usage = OFF;
}

void LockController::setSync()
{
  return;
}

void LockController::setAsync()
{
  return;
}

void LockController::setLockGain(double xx)
{
  int k;
  if (xx > 63.0) xx = 63.0;
  if (xx < 0.0)  xx = 0.0;
  k = (int) floor(xx+0.5);
  lockGain = k;
  outputACode(LOCKGAIN,1,&k);
}

int LockController::getLockGainI()
{
  return(lockGain);
}

void LockController::setLockPower(double xx){
  int k;
  if (xx > 63.0) xx = 63.0;
  if (xx < 0.0)  xx = 0.0;
  k = (int) floor(xx+0.5);
  lockPower = k;
  outputACode(LOCKPOWER,1,&k);
}
  
int  LockController::getLockPowerI(){
  return(lockPower);
}
   
void LockController::setLockPhase(double xx)
{
  int k;
  double y;
  y = fmod(xx,360.0);
  if (y < 0.0) y += 360.0;
  y /= 256.0;
  k = (int) floor(y);
  lockPhase = k;
  outputACode(LOCKPHASE,1,&k);
}
   
int  LockController::getLockPhaseI()
{
  return(lockPhase);
}
   
void LockController::setLockOffsetFrequency(double xx)
{
  int k;

  k = (int) floor(xx+0.5);
  lockGain = k;
  outputACode(LOCKFREQ,1, &k);
}
   
int  LockController::getLockOffsetFrequencyI()
{
  return(lockOffsetFreq);
}


int LockController::initializeExpStates(int setupflag)
{
   return 0;
}

