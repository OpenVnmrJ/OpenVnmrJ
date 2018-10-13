/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _WINROTATION_H
#define _WINROTATION_H

#include "aipImgInfo.h"

class WinRotation
{
private:
    static WinRotation *winRotation; // Support one instance

    WinRotation();
    static void rotate(char * rottype);
    static bool calcBodyToPixRotation(int slice, int policy, double b2p[3][3]);
    static bool calcMagnetToBodyRotation(int loc1, int loc2, double m2b[3][3]);
    static bool snapRotationTo90(double src[3][3], double dst[3][3]);

public:
    static WinRotation *get(); // Returns instance; may call constructor.
    static int aipRotate(int argc, char *argv[], int retc, char *retv[]);
    static int calcRotation(spDataInfo_t di);
};

#endif /* _WINROTATION_H */
