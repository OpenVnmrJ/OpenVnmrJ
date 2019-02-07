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

// $History: LineFitter.cpp $
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include "LineFitter.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/* --------------------------
     * LINE_FITTER::LINE_FITTER
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Constructor
 * History :
 *-------------------------- */
LINE_FITTER::LINE_FITTER()
{
    ClearPoints();
}

/* --------------------------
     * LINE_FITTER::ClearPoints
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Restart with no point
 * History :
 *-------------------------- */
void LINE_FITTER::ClearPoints()
{
    S = Sx = Sy = Sxx = Sxy = 0 ;
}

/* --------------------------
     * LINE_FITTER::AddPoint
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Add a point in the set
 *           of points to be fitted
 * History :
 *-------------------------- */
void LINE_FITTER::AddPoint(INT_FLOAT x, INT_FLOAT y)
{
    S   += 1   ;
    Sx  += x   ;
    Sy  += y   ;
    Sxx += x*x ;
    Sxy += x*y ;
}

/* --------------------------
     * LINE_FITTER::GetCoefs
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Get the result of the fit
 * History :
 *-------------------------- */
bool LINE_FITTER::GetCoefs(double& a, double& b)
{
    INT_FLOAT delta = S*Sxx-Sx*Sx;
    if (delta!=0)
    {
        a = (Sxx * Sy  - Sx * Sxy ) / delta ;
        b = (S   * Sxy - Sx * Sy  ) / delta ;
        return true;
    }
    else
        {
        a = 0 ;
        b = 0 ;
        return false;
    }
}
