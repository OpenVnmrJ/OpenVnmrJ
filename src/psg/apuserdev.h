/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Used for ap user device functions.  */

#ifndef APUSERDEV_H
#define APUSERDEV_H

extern void inituserapobjects();
extern void setBOB(int value, int reg);
extern void vsetBOB(int rtparam, int reg);
extern void vreadBOB(int rtparam, int reg);
extern void apset(int value, int addrreg);
extern void apsetbyte(int value, int addrreg);
extern void vapset(int rtparam, int addrreg);
extern void vapread(int rtparam, int addrreg, double apsyncdelay);
extern void signal_acqi_updt_cmplt();
extern void start_acqi_updt(double delaytime);
extern void setnumpoints(codeint rtindex);
extern void setdataoffset(codeint rtdataoffset);

#endif
