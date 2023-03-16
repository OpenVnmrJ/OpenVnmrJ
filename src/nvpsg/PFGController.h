/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INC_PFGCONTROLLER_H
#define INC_PFGCONTROLLER_H

#include "GradientBase.h"

#define XPFGPRESENT  1
#define YPFGPRESENT  2
#define ZPFGPRESENT  4


#define PFGMAXPLUS (32767.0)
#define PFGUNITYPLUS (0x4000)
#define PFGUNITYMINUS (0xC000)

class PFGController: public GradientBase
{
   private:
     // there probably should be some duty cycle
     // things here...
     int usageFlag;  // uses the present & enable bits..
     char gradType[16];
     long long wait4meExpireTicker;
     //
     //PowerMonitor Xpower,Ypower,Zpower;

   public:
     //  this is the only constructor...
     //PFGController(char *name,char *gradtype):GradientBase(name,0),Xpower(1.0e6,10.0,name),Ypower(1.0e6,10.0,name),Zpower(1.0e6,10.0,name)
     PFGController(const char *name,const char *gradtype):GradientBase(name,0)
     {
        patternDataStoreSize = 4000;
        patternDataStore = (int *) malloc(patternDataStoreSize * sizeof(int));
        patternDataStoreUsed = 0;
        usageFlag = 0; 
        if (tolower(gradtype[0] != 'n'))  usageFlag |= XPFGPRESENT;
        if (tolower(gradtype[1] != 'n'))  usageFlag |= YPFGPRESENT;
	if (tolower(gradtype[2] != 'n'))  usageFlag |= ZPFGPRESENT;
        kind = PFG_TAG;
        strncpy(gradType,gradtype,10);  // why is strncpy() only using 10 of 16 chars?
        // BDZ: gradType is size 16 and strncpy() might not have null terminated.
        gradType[10] = 0;
        wait4meExpireTicker = 0L;
        if ( pAcodeBuf == NULL)
        {  cout << "PFGController constr(): pAcodeBuf is NULL \n" ; }
     };  
  
     int isPresent(char x);
     int setEnable(char *cstring);
     int initializeExpStates(int setupflag);
     int initializeIncrementStates(int num);
     virtual int getDACLimit();
     void setExpireTime(long long expT);
     int setTickDelay(int ticks);  // overrides base method for wait for me
     void setGates(int GatePattern);
     int setPfgCorrection();
     virtual void setGrad(const char *which, double value);
     virtual void setGradScale(const char *which, double value);
     void setVGrad(char *which, double step, int rtvar); 
     virtual int errorCheck(int checkType, double g1, double g2, double g3, double s1, double s2, double s3);
     virtual cPatternEntry *resolveGrad1Pattern(char *nm, int flag, char *emsg, int action);
     virtual cPatternEntry *resolveOblShpGrdPattern(char *nm, int flag, const char *emsg, int action);
     virtual int userToScale(double xx);
     virtual int ampTo16Bits(double xx);
     void showPowerIntegral();
     void showEventPowerIntegral(const char *);
     void getPowerIntegral(double *powerarray);


};
#endif
