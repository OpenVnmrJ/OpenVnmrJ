/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef RCVRFUNCS_H
#define RCVRFUNCS_H

extern void rcvron();
extern void rcvroff();
extern void recon();
extern void recoff();
extern void blankon(int device);
extern void blankoff(int device);
extern int isBlankOn(int device);
extern int parmToRcvrMask(char *parName);
extern int isRcvrActive(int n);
extern int getFirstActiveRcvr();
extern int numOfActiveRcvr();
extern double getGain(int ircvr);
extern int getGainBits(int ircvr);
extern void setMRGains();
extern void setMRFilters();
extern void startacq(double ldelay);
extern void endacq();

#endif
