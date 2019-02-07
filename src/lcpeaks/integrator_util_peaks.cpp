#ifndef LINT
static char SCCSid[] = "%Z%%M% %I% %G% Copyright (c) 2000-2002 Varian,Inc. All Rights Reserved";
#endif
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

// $History: integrator_util_peaks.cpp $
/*  */
/* *****************  Version 9  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 8  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include <cmath>
#include <cassert>
#include "integrator_util_peaks.h"
#include "integrator_classes.h"
#include "GeneralLibrary.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

////////////// CONSTANTS
#define INTEGR_EXPBL_COEFF		0.25

TCandidatePeak::TCandidatePeak()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    _defaultvalues();
}

TCandidatePeak::TCandidatePeak(int aStartIndex,
    int aEndIndex,
    int aApexIndex,
    int aBunchSize)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    _defaultvalues();
    StartIndex  = aStartIndex ;
    EndIndex    = aEndIndex ;
    ApexIndex   = aApexIndex ;
    BunchSize = aBunchSize ;

}

void TCandidatePeak::_defaultvalues()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    StartIndex  = 0;
    EndIndex    = 0;
    ApexIndex   = 0;
    BunchSize   = 0;
    FForcedStart = false ;
    FForcedEnd   = false ;
    FFusedStart  = false ;
    FFusedEnd    = false ;
    FEndIsValley = false ;
    FStartIsValley = false ;
    FisPositive = true ;
    FHasEventBaseLine = false ;
    FEventLineIsHorizontal = false ;
    myIndexInTheWholeList = -1 ;
    pFmyLine = NULL ;
    FisMother = false ;
    FisDaughter = false ;
    MotherIndex = -1 ;
    pFDaughters = NULL ;
    RetentionTime = 0 ;
    Area    = 0 ;
    Height  = 0 ;
    FmyLineType = BASELINE_STRAIGHT ;
    FDropLineAtEnd = false ;
    FDropLineAtStart = false ;
    FMaxSlope        = 0 ;
    char tmpchar = INTEGRATION_NO_CODE ;
    strncpy(&FStartCode,&tmpchar,1) ;
    strncpy(&FEndCode  ,&tmpchar,1) ;
    code = string("");
    isUserSlice = false ; 

#ifdef INTEGR_DEBUG_MEMORY
    Glob_inCandidPeaksCount++ ;
    Glob_inCandidPeaksTotal++ ;
#endif

}

TCandidatePeak::~TCandidatePeak()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
#ifdef INTEGR_DEBUG_MEMORY
    dec(Glob_inCandidPeaksCount) ;
#endif
    if (pFDaughters != NULL) delete pFDaughters ;

}

void  TCandidatePeak::UpdateMaxSlope(INT_FLOAT slope)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FMaxSlope = max(FMaxSlope,fabs(slope)) ;
}

bool TCandidatePeak::ApexIndexIsValid()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool result = ( ( (ApexIndex > StartIndex) &&
        (ApexIndex < EndIndex) )
        || (ApexIndex==0)
        );
    return result;
} 

void TCandidatePeak::SetDefaultCodes()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    switch (FStartCode) {

        case INTEGRATION_NO_CODE :   // - is always replaced
            {
                if (FFusedStart || FStartIsValley)
                {
                    FStartCode = INTEGRATION_VALLEY_CODE ;
                }
                else
                    {
                    FStartCode = INTEGRATION_SIGNAL_CODE ;
                }
                break;
            }

        case INTEGRATION_EXP_CODE : break;
        case INTEGRATION_TANGENT_CODE : // 'E','T' can be replaced by 'V'
            {
                if (FFusedStart || FStartIsValley)
                {
                    FStartCode = INTEGRATION_VALLEY_CODE;
                }
                break;
            }
    }

    switch (FEndCode) {

        case INTEGRATION_NO_CODE : // - is always replaced
            {
                if (FFusedEnd || FEndIsValley)
                {
                    FEndCode = INTEGRATION_VALLEY_CODE ;
                }
                else
                    {
                    FEndCode = INTEGRATION_SIGNAL_CODE ;
                }
                break;
            }

        case INTEGRATION_EXP_CODE : break;
        case INTEGRATION_TANGENT_CODE : // 'E','T' can be replaced by 'V'
            {
                if (FFusedEnd || FEndIsValley)
                {
                    FEndCode = INTEGRATION_VALLEY_CODE ;
                }
                break;
            }

    }

// a forced start/end overwrites everything
    if (FForcedStart) FStartCode = INTEGRATION_FORCED_CODE ;
    if (FForcedEnd)   FEndCode   = INTEGRATION_FORCED_CODE ;

}

void TCandidatePeak::SetHorizontalCodes()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FStartCode = INTEGRATION_HORIZ_CODE ;
    FEndCode   = INTEGRATION_HORIZ_CODE ;
}

void TCandidatePeak::SetSkimmingCodes(TBaseLineType linetype)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    switch (linetype) {
        case BASELINE_STRAIGHT :
            {
                FStartCode = INTEGRATION_TANGENT_CODE ;
                FEndCode   = INTEGRATION_TANGENT_CODE ;
                break;
            }

        case BASELINE_EXPONENTIAL :
            {
                FStartCode = INTEGRATION_EXP_CODE ;
                FEndCode   = INTEGRATION_EXP_CODE ;
                break;
            }
    }
}

/*------------------------------------------------------------
      FUNCTION:    TCandidatePeak.GetMaxDaughterIndex
  AUTHOR:    Bruno Orsier
  DATE:      23/10/2001 08:49:19
             translated into C++ by GO 23.05.02
  PURPOSE:   returns the max right index of all daughter peaks.
             Computing the max is not really necessary since the peaks
             are sorted, but I do not have the time to study carefully the issue,
             thus I have just implemented the safest method. It would be necessary
             to check in which order peaks are added into the list of daughters.
             Default value when there is no daughter peak is the peak start index
  HISTORY:
  ------------------------------------------------------------*/
int TCandidatePeak::GetMaxDaughterIndex()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    int theMax = StartIndex ;
    if (pFDaughters != NULL)
    {
        for (TCandidatePeakList::iterator aDaughter_it = pFDaughters->begin();
        aDaughter_it != pFDaughters->end();
        aDaughter_it++)
        {
            TCandidatePeak& aDaughter = *(*aDaughter_it);
            theMax = max(theMax, aDaughter.EndIndex) ;
        }
    }
    assert(theMax <= EndIndex) ; // there should be no wrong daughter peaks
    return theMax ;
}

int TCandidatePeak::GetMinDaughterIndex()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    int theMin = EndIndex ;
    if (pFDaughters != NULL)
    {
        for (TCandidatePeakList::iterator aDaughter_it = pFDaughters->begin();
        aDaughter_it != pFDaughters->end();
        aDaughter_it++)
        {
            TCandidatePeak& aDaughter = *(*aDaughter_it);
            theMin = min(theMin, aDaughter.StartIndex) ;
        }
    }
    assert(theMin >= StartIndex) ; // there should be no wrong daughter peaks
    return theMin ;
}

TCandidateBaseLine::TCandidateBaseLine()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    __initialize();
}

TCandidateBaseLine::TCandidateBaseLine(int startindex,
    int endindex)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    __initialize();
    FStartIndex = startindex ;
    FEndIndex   = endindex ;
}

void TCandidateBaseLine::__initialize()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FStartIndex = -1 ;
    FEndIndex   = -1 ;
//FAssociatedPeaks := TCandidatePeakList.CReate ;
    FForcedStart = false ;
    FForcedEnd   = false ;
    FisHorizontalFromStart = false ;
    FisHorizontalFromEnd   = false ;
    FMeasureStartAtIndex   = -1 ;
    FMeasureEndAtIndex     = -1 ;
    FisEventLine = false ;
#ifdef INTEGR_DEBUG_MEMORY
    Glob_inCandidBlinesCount++ ;
    Glob_inCandidBlinesTotal++ ;
#endif

}

TCandidateBaseLine::~TCandidateBaseLine()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
#ifdef INTEGR_DEBUG_MEMORY
    Glob_inCandidBlinesCount-- ;
#endif
}

void Integr_ComputeBaselineExponentialCoefficients( 
    const INT_TIME start_time,
    const INT_TIME end_time,
    const INT_FLOAT start_value,
    const INT_FLOAT end_value,
    INT_FLOAT &A,
    INT_FLOAT &B,
    INT_FLOAT &t0)
/* --------------------------
 * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    t0 = (end_value < start_value) ? end_value : start_value;

    if( t0 >= 0.0 )
        t0 *= (1.0 - INTEGR_EXPBL_COEFF);
    else
        t0 *= (1.0 + INTEGR_EXPBL_COEFF);

    B = log((end_value - t0) / (start_value - t0)) / (end_time - start_time);
    try
    {
        A = (start_value - t0) * exp(-B * start_time);
    } catch(...)
    {
        A = 0.0;
    }
}

void TCandidateBaseLine::GetExponentialCoefficients(INT_FLOAT &A, INT_FLOAT &B, INT_FLOAT &t0)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Integr_ComputeBaselineExponentialCoefficients(StartTime, EndTime, StartValue, EndValue,
        A, B, t0);
}

/*------------------------------------------------------------
 FUNCTION:    .Integr_ComputeBaselineExponentialValue
 AUTHOR:    Bruno Orsier
            Gilles Orazi : C++ translation
 DATE:      25/04/2001 16:49:34
 PURPOSE:   - compute the exponential baseline value given the time and the coefficients
            - factorize a computation that was made in two different files. This entailed DT4007.
            Note: this function must be synchronized with Integr_ComputeBaselineExponentialCoefficients (inverse function)
 HISTORY:
------------------------------------------------------------*/
INT_FLOAT Integr_ComputeBaselineExponentialValue(
    const INT_FLOAT t, 
    const INT_FLOAT A, 
    const INT_FLOAT B, 
    const INT_FLOAT t0)
{
    INT_FLOAT result ;
    try {
        result = t0 + A * exp(B * t) ;
    }
    catch(...)
    {
        result = t0 ;
    }
    return result ;
}
