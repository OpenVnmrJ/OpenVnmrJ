/*
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/* DISCLAIMER :
     * ------------
     *
 * This is a beta version of the GALAXIE integration library.
 * This code is under development and is provided for information purposes.
 * The classes names and interfaces as well as the file names and
 * organization is subject to changes. Moreover, this code has not been
 * fully tested.
 *
 * For any bug report, comment or suggestion please send an email to
 * gilles.orazi@varianinc.com
 *
 * Copyright Varian JMBS (2002)
 */

#ifndef _INTEGRATOR_LINETOOLS_H
#define _INTEGRATOR_LINETOOLS_H

#include "integrator_types.h"

class TLineTool {
public:
    virtual INT_FLOAT ValueAtTime(INT_FLOAT t) = 0;
};

class TStraightLineTool : public TLineTool {
private:
    INT_FLOAT
    FTime1,
    FValue1,
    FTime2,
    FValue2;
public:
    TStraightLineTool(INT_FLOAT Time1,
        INT_FLOAT Value1,
        INT_FLOAT Time2,
        INT_FLOAT Value2);
    virtual INT_FLOAT ValueAtTime(INT_FLOAT t);
};

class  TExpLineTool : public TLineTool{
private:
    INT_FLOAT
    FA,
    FB,
    Ft0 ;
public:
    TExpLineTool(INT_FLOAT A,
        INT_FLOAT B,
        INT_FLOAT t0);
    virtual INT_FLOAT ValueAtTime(INT_FLOAT t);
};

#endif
