/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef WETFUNCS_H
#define WETFUNCS_H

#include "acqparms.h"

/* wet4 - Water Elimination */
void wet4(codeint phaseA, codeint phaseB);
/* chess - CHEmical Shift Selective Suppression */
extern void chess(double pulsepower, char *pulseshape, double duration,
      codeint phase, double rx1, double rx2, double gzlvlw,
      double gtw, double gswet, int c13wet);
extern int getflag(char *str);
extern void comp90pulse(double width, codeint phase, double rx1, double rx2);

#endif
