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

#include "common.h"
#include "integrator_lineTools.h"
#include "math.h"
#include <cassert>

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

TStraightLineTool::TStraightLineTool(INT_FLOAT Time1,
    INT_FLOAT Value1,
    INT_FLOAT Time2,
    INT_FLOAT Value2)
{
    assert(Time2 > Time1); // no vertical lines allowed
    FTime1  = Time1  ;
    FTime2  = Time2  ;
    FValue1 = Value1 ;
    FValue2 = Value2 ;
}

INT_FLOAT TStraightLineTool::ValueAtTime(INT_FLOAT t)
{
    return FValue1 + (t - FTime1) / (FTime2 - FTime1) * (FValue2 - FValue1);
}

TExpLineTool::TExpLineTool(INT_FLOAT A,
    INT_FLOAT B,
    INT_FLOAT t0)
{
    FA = A;
    FB = B;
    Ft0 = t0;
}

INT_FLOAT TExpLineTool::ValueAtTime(INT_FLOAT t)
{
    INT_FLOAT result ;
    try
    {
        result = Ft0 + FA * exp(FB * t) ;
    }
    catch (...)
    {
        result = Ft0;
    }
    return result ;
}


