/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef INC_GRADCONTROLLER_H
#define INC_GRADCONTROLLER_H

#include "GradientBase.h"

#define GRADMAXPLUS (32767.0)

extern "C" {
void warn_message(const char *format, ...) __attribute__((format(printf,1,2)));
}


class GradientController: public GradientBase
{
   private:
     // there probably should be some duty cycle
     // things here...
     int userToScale(double xx);
     int ampTo16Bits(double xx);
     int validControllers;
     int usageFlag;
     long long DelayAccumulator;
     int GridTicks;
     int DelayMode;
     int doRepeatedDelay(long long dur);
   public:
     GradientController(const char *name,int flags):GradientBase(name,flags)
     {  
        patternDataStore = (int *) malloc(4000*sizeof(int));
        patternDataStoreSize = 4000;
        patternDataStoreUsed = 0;
        validControllers = flags & 0xf;
        usageFlag = flags & 0xf0;
        kind = GRAD_TAG;
        DelayMode = 0; 
        DelayAccumulator = 0L;
        GridTicks = 320;
        if ( pAcodeBuf == NULL )
            warn_message((char*)"AcodeBuffer is null in GradientController object\n");
     }; 

     int initializeExpStates(int setupflag);
     int initializeIncrementStates(int num);
     virtual int getDACLimit();
     void setEnable(char *cmd);
     void setGates(int GatePattern);
     virtual void setGrad(const char *which, double value);
     virtual void setGradScale(const char *which, double value);
     void setVGrad(char *which, double step, int rtvar);
     virtual int errorCheck(int checkType, double g1, double g2, double g3, double s1, double s2, double s3);
     // virtual base class allows override. 
     // ONLY setTickDelay & outputAcode
     int setTickDelay(long long ticks);
     int outputACode(int Code, int many, int *stream);
     //  remember math a(1,1) is FIRST - no zero index...
     void setRotArrayElement(int index1, int index2, double value);
#ifdef NEXT
     void setRotation(double ang1, double ang2, double ang3);
#endif
     virtual cPatternEntry *resolveGrad1Pattern(char *nm, int flag, char *emsg, int action);
     virtual cPatternEntry *resolveOblShpGrdPattern(char *nm, int flag, const char *emsg, int action);
     int flushGridDelay();
     int setNoDelay(double dur);
     void getPowerIntegral(double *powerarray);
};
#endif
