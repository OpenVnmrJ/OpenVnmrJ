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

// $History: integrator_util_misc.cpp $
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
#include <math.h>
#include <stdlib.h>
#include "integrator_util_misc.h"
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

TSpiralingCounter::TSpiralingCounter()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Reset();
}

void TSpiralingCounter::NextValue()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if ((deltai == deltaj) && (deltai <= 0))
    {
        iplus = 1 ;
        jplus = 0 ;
        irange++ ;
    }
    else
        if (abs(deltai) == irange)
        if (abs(deltaj) < irange)
    {
        iplus = 0 ;
        jplus = sgn(deltai) ;
    }
    else
        if (deltai*deltaj > 0)
    {
        iplus = -sgn(deltai) ;
        jplus = 0 ;
    }
    else
        {
        iplus = 0 ;
        jplus = -sgn(deltaj) ;
    }
    deltai += iplus ;
    deltaj += jplus ;

}

void TSpiralingCounter::Reset()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    iplus  = 1 ;
    jplus  = 0 ;
    irange = 0 ;
    deltai = 0 ;
    deltaj = 0 ;
}

int TSpiralingCounter::sgn(int x)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    int result ;
    if (x>0)
        result = 1;
    else
        if (x<0)
            result =-1;
    else
        result = 0;
    return result ;
}

TStraightLineTool_Integer::TStraightLineTool_Integer(int line_I1,
    INT_FLOAT line_Y1,
    int       line_I2,
    INT_FLOAT line_Y2)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Fline_I1 = line_I1 ;
    Fline_I2 = line_I2 ;
    Fline_Y1 = line_Y1 ;
    Fline_Y2 = line_Y2 ;

// check that we have a true line
    assert( (line_I1 != line_I2) || (line_Y1 != line_Y2) ) ;
}

INT_FLOAT TStraightLineTool_Integer::ValueAt(int I)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT denom = (Fline_I2-Fline_I1) ;
    INT_FLOAT num = (I-Fline_I1) ;
    return Fline_Y1 +  num / denom *(Fline_Y2-Fline_Y1) ;
}

TExpLineTool_NotUsed::TExpLineTool_NotUsed(INT_FLOAT A,
    INT_FLOAT B,
    INT_FLOAT DeltaT)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FA = A ;
    FB = B ;
    FDeltaT = DeltaT ;
}

INT_FLOAT TExpLineTool_NotUsed::ValueAt(int I)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT result = FA*exp(FB*I*FDeltaT) ;
    return result;
}
