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

// integrator_calcul.cpp
//-------------------------------------------------------------
// Implementation of classes involved in the computations
//-------------------------------------------------------------
// $History: integrator_calcul.cpp $
/*  */
/* *****************  Version 16  ***************** */
/* User: Go           Date: 8/11/02    Time: 17:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* C++ translation of the integration algorithm.  */
/* All main translation bugs fixed after having performed the official */
/* integration tests. */
/*  */
/* *****************  Version 15  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 14  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 13  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 12  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 11  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 9  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 24/06/02   Time: 11:30 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 12/06/02   Time: 10:34 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 23/05/02   Time: 18:02 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* BACKUP */

#include "common.h"
#include <assert.h>
#include <cmath>
#include <algorithm>
#include "integrator_types.h"
#include "integrator_calcul.h"
#include "integrator_groups.h"
#include "integrator_classes.h"
#include "integrator_peakProperties.h"
#include "GeneralLibrary.h"
#include "LineFitter.h"
#include "Wavelets.h"
#include "median.h"
#include "owninglist.h"

#ifdef DEBUG
#include "debug.h"
#include <fstream>
#include <stdio.h>
#endif

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

//---------------------------------------------------

int  CheckLastAdded(int previous, int candidat)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    int result ;
    if (candidat <= previous)
    {
        result = previous ;
    }
    else
        {
        result = candidat ;
    }

    return result ;
}

//---------------------------------------------------
CLASSIFY_RESULT Classify (INT_FLOAT p0x,
    INT_FLOAT p1x,
    INT_FLOAT p2x,
    INT_FLOAT p0y,
    INT_FLOAT p1y,
    INT_FLOAT p2y)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT EPSILON = 1E-5 ;
    INT_FLOAT ax = p1x - p0x ;
    INT_FLOAT ay = p1y - p0y ;
    INT_FLOAT bx = p2x - p0x ;
    INT_FLOAT by = p2y - p0y ;
    INT_FLOAT sa = ax * by - bx * ay ;

    CLASSIFY_RESULT result;
    if (sa > EPSILON)
    {
        result = LEFT;
    }
    else
        if (sa < -EPSILON)
    {
        result = RIGHT;
    }
    else
        if ((ax * bx < -EPSILON) || (ay * by < -EPSILON))
    {
        result = BEHIND;
    }
    else
        if ((ax * ax + ay * ay) < (bx * bx + by * by))
    {
        result = BEYOND;
    } 
    else
        if ((fabs(p0x - p2x) < EPSILON) && (fabs(p0y - p2y) < EPSILON))
    {
        result = ORIGIN;
    }
    else
        if ((fabs(p1x - p2x) < EPSILON) && (fabs(p1y - p2y) < EPSILON))
    {
        result = DESTINATION;
    }
    else
        {
        result = BETWEEN;
    }
    return result;
}
//---------------------------------------------------

void FindTestLine(
    INT_INDEXLIST& halfhull,
    int apexIndex,
    int& StartTestLine,
    int& EndTestLine)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(apexIndex >= 0);
    unsigned int i = 1 ;
    bool stop = false ;
    while ( (!stop)  && (i<halfhull.size()) )
    {
        stop = apexIndex < halfhull[i];
        i++;
    }
    if (stop)
    {
        StartTestLine = halfhull[i-2] ;
        EndTestLine   = halfhull[i-1] ;
    }
    else
        {
        StartTestLine = 0 ;
        EndTestLine   = 0 ;
    }
}

//---------------------------------------------------

/*
function FindIntersectLineCurve_NotContact
------------------------------------------
AUTHOR:  BO, Sept 30, 1998  
         GO, May  22, 2002 : C++ translation
PURPOSE: Try to find the first intersection (starting from left to right)
         between the line (I1,Y1)-(I2,Y2) and the curve (a list of values).
         But a simple contact will not be considered as an intersection
         contrary to the first version above.
         The procedure identifies the following situations:
         - no intersection
         - TRUE intersection located between two data points

REMARK: the caller of this procedure MUST check that the line is not reduced
        to a single point (I1=I2)

	During the translation into C++, I changed the type of some parameters from int to double
	to avoid integer division in the formula. GO.
*/

INT_FLOAT Local_LineValueAt(INT_FLOAT I,
    INT_FLOAT line_I1,
    INT_FLOAT line_I2,
    INT_FLOAT line_Y1,
    INT_FLOAT line_Y2 )
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT result =  line_Y1 + (I - line_I1) / (line_I2 - line_I1) * (line_Y2 - line_Y1) ;
    return result ;
}

INT_FLOAT Local_CurveAt(INT_SIGNAL Curve, int I)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return Curve[I];
//!! WARNING !!
//   Changed the I+1 into I during translation
//   because TDynarrayligth indexes are 1-based
}

void FindIntersectLineCurve_NotContact(INT_SIGNAL Curve,
    int line_I1,
    int line_I2,
    INT_FLOAT line_Y1,
    INT_FLOAT line_Y2,
    TLineCurveIntersectType& IntersectType,
    int& prevI,
    int& nextI)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(line_I2 > line_I1);

//assert(line_I2 < Curve.len); not translated because no equivalent for curve.len

    IntersectType = INTERSECT_NONE;
    prevI = line_I1 + 1;
    INT_FLOAT prevLineValue = Local_LineValueAt(prevI, line_I1, line_I2, line_Y1, line_Y2);
    INT_FLOAT prevDelta = Local_CurveAt(Curve,prevI) - prevLineValue;

// skip initial contact points
    while ( (fabs(prevDelta) < EPSILON_INTERSECTION) && (prevI < line_I2 - 1) )
    {
        prevI++;
        prevLineValue = Local_LineValueAt(prevI, line_I1, line_I2, line_Y1, line_Y2);
        prevDelta = Local_CurveAt(Curve, prevI) - prevLineValue;
    }

    nextI = prevI + 1;

    while ( (IntersectType == INTERSECT_NONE) && (nextI <= line_I2 - 1) )
    {
        INT_FLOAT nextLineValue = Local_LineValueAt(nextI, line_I1, line_I2, line_Y1, line_Y2);
        INT_FLOAT nextDelta = Local_CurveAt(Curve, nextI) - nextLineValue;
// skip current contact points
        while ( (fabs(nextDelta) < EPSILON_INTERSECTION) && (nextI <= line_I2 - 1) )
        {
            nextI++;
            nextLineValue = Local_LineValueAt(nextI, line_I1, line_I2, line_Y1, line_Y2);
            nextDelta = Local_CurveAt(Curve, nextI) - nextLineValue;
        }

        if ( (fabs(prevDelta) >= EPSILON_INTERSECTION) &&
            (fabs(nextDelta) >= EPSILON_INTERSECTION) &&
            (prevDelta * nextDelta < 0) )

        {
            IntersectType = INTERSECT_BETWEEN ;
        }
        else
            {
            prevI = nextI;
            prevDelta = nextDelta;
            nextI++;
        }
    }
}

//----------------  TPeakDetector -------------------

TPeakDetector::TPeakDetector()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FSlopeSensitivity = 0;
    FSignalMaxValue   = INT_MAXFLOAT ;
    BM1 = 0;
    B0  = 0;
    B1  = 0;
    B2  = 0;
    B3  = 0;
    NegPeaksAllowed = false;

    ShoulderReset();
}

TPeakDetector::~TPeakDetector()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{

}

void TPeakDetector::Push(INT_FLOAT val)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    BM1 = B0  ;
    B0  = B1  ;
    B1  = B2  ;
    B2  = B3  ;
    B3  = val ;
}

bool TPeakDetector::PotentialStart()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT S1 = B3 - B2 ;
    INT_FLOAT S2 = B2 - B1 ;
    INT_FLOAT AverageSlope = (S1 + S2) / 2.0;
    bool result;

#ifdef DEBUG
    {
        char tmpstr[100];
        int nc = sprintf(tmpstr,"Potential start ? S1=%g  ave=%g  sensitivity=%g",S1,AverageSlope,FSlopeSensitivity);
        assert(nc<=100);
        OptionalDebug(tmpstr,3);
    }
#endif
    if (S1 >= FSlopeSensitivity)
    {
        result =
            (AverageSlope >= FSlopeSensitivity) &&
            (fabs(B3 - FSignalMaxValue) > INTEGRATOR_SIGNALMAXVALUE_TOLERANCE * FSignalMaxValue);
    }
    else
        {
        if ( (S1 <= -FSlopeSensitivity) &&
            NegPeaksAllowed )
        {
            result =
                (AverageSlope <= -FSlopeSensitivity) &&
                (fabs(B3 - FSignalMaxValue) > INTEGRATOR_SIGNALMAXVALUE_TOLERANCE * FSignalMaxValue) ;
        }
        else
            {
            result = false;
        }
    }

#ifdef DEBUG
    if (result)
        OptionalDebug("Potential start : yes",3);
    else
        OptionalDebug("Potential start : no",3);

#endif
    return result;
}

bool TPeakDetector::PotentialEnd()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool result =
        (fabs(B2 - B1) <= FSlopeSensitivity) &&
        (fabs(B3 - B2) <= FSlopeSensitivity) &&
        (fabs(B1 - B0) <= FSlopeSensitivity) ;
    return result ;
}

bool TPeakDetector::PotentialApex()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool result1 = ((B1 - BM1) * (B3 - B1) < 0);
    bool result ;
    if (result1)
        if (!NegPeaksAllowed && (B3 - B1 > 0))
    {
        result = false ;
    }
    else
        {
        result = true;
    }
    else
        {
        result = false;
    }
    return result ;
}

bool TPeakDetector::PotentialValley()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool result =
        ((B1 - BM1) * (B3 - B1) < 0) &&
        (fabs(B3 - FSignalMaxValue) > INTEGRATOR_SIGNALMAXVALUE_TOLERANCE * FSignalMaxValue) ;
    return result ;
}

bool TPeakDetector::PotentialShoulder()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TSlopeType curSlope = SlopeType();
    bool result ;

    if (curSlope == ZeroSlope)
    {
        ZeroSlopeFound = true;
        result = false;
    }
    else
        {
        switch (ZeroSlopeFound) {
            case true :
                {
                    ZeroSlopeFound = false;
                    if (curSlope == PreviousNonZeroSlope)
                    {
                        result = true;
                    }
                    else
                        {
                        result = false;
                        PreviousNonZeroSlope = curSlope;
                    }
                    break;
                }

            case false :
                {
                    PreviousNonZeroSlope = curSlope;
                    result = false;
                    break;
                }

        }
    }
    return result;
}

TSlopeChangeType TPeakDetector::SlopeChange()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(PotentialApex() || PotentialValley());
    TSlopeChangeType result ;

    if ( (B1 - BM1 > 0) &&
        (B3 - B1  < 0) )
    {
        result = PosThenNeg ;
    }
    else
        {
        result = NegThenPos ;
    }
    return result;
}

TSlopeType TPeakDetector::SlopeType()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT delta = B3 - B2;
    TSlopeType result ;

    if (delta > FSlopeSensitivity)
    {
        result = PositiveSlope ;
    }
    else
        {
        if (delta < -FSlopeSensitivity)
        {
            result = NegativeSlope;
        }
        else
            {
            result = ZeroSlope;
        }
    }
    return result;
}

TSlopeType TPeakDetector::SlopeTypeNoZero()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// like function SlopeType but here we try to avoid a ZeroSlope Result
    INT_FLOAT delta = B3 - B2;
    TSlopeType result ;
    if (delta > 0)
    {
        result = PositiveSlope;
    }
    else
        {
        if (delta < 0)
        {
            result = NegativeSlope;
        }
        else
            {
            result = ZeroSlope;
        }
    }
    return result;
}

INT_FLOAT TPeakDetector::EstimatedDeriv2()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT result = B3 - 2.0 * B1 + BM1; // could be divided by 4*Deltat for more accuracy
    return result;
}

INT_FLOAT TPeakDetector::EstimatedDeriv1()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT result = B2 - B0; // could be divided by 2*Deltat for more accuracy
    return result;
}

void TPeakDetector::ShoulderReset()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    PreviousNonZeroSlope = ZeroSlope;
    ZeroSlopeFound       = false;
}

#ifdef TESTING
static void TPeakDetector::Test(){

}
#endif

//------------------- TGeneralState -----------------------

void TGeneralState::UpdateFromEvents(INT_FLOAT curTime, TIntegrationEventList& aEvents )
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT EPSILON_ZERO_TIME = 0.001;

// events must have been sorted by time before calling this function
    bool stop = false;
    while ((aEvents.size() > 0) && !stop)
    {
        TIntegrationEvent& curEvent = *(aEvents.front());
        stop =
            (curEvent.Time() > curTime) &&
            (fabs(curEvent.Time() - curTime) > EPSILON_ZERO_TIME);	// or added for issue 547.
// This is not a general solution however
// because we work with integer indexes
// while users see real values
        if (!stop)
        {
            ProcessEvent(curEvent);
            aEvents.erase(aEvents.begin()); // not too efficient. We could also sort the events in reverse order.
        }
    }
}

#ifdef TESTING
static void TGeneralState::Test(){

}
#endif

//------------------- TRejectionState --------------------

TRejectionState::TRejectionState()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FMinHeight = 0 ;
    FMinArea   = 0 ;
    FTotArea   = 0 ;
    FTotHeight = 0 ;
}

bool TRejectionState::KeepThePeak(TCandidatePeak& thePeak)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
//  with thePeak do
    bool result =
        (fabs(thePeak.Area) > FMinArea)           &&
        (fabs(thePeak.Height) > FMinHeight)       &&
        (thePeak.StartIndex < thePeak.EndIndex)   ||  // rejection of peaks reduced to a single point
        thePeak.FisMother                                  ||
        thePeak.FisDaughter; // deletion of skimmed peaks is not allowed
// for the moment because we have to study
// the consequences of such deletions
    return result ;
}

void TRejectionState::ProcessEvent(TIntegrationEvent& theEvent)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    switch (theEvent.Code()) {

        case INTEGRATIONEVENT_MIN_AREA :
            {
                FMinArea = fabs( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue());
                break;
            }

        case INTEGRATIONEVENT_MIN_HEIGHT :
            {
                FMinHeight = fabs( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue());
                break;
            }

        case INTEGRATIONEVENT_MIN_AREA_PERCENT :
            {
                FMinArea = 0.01 * fabs( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue()) * FTotArea;
                break;
            }

        case INTEGRATIONEVENT_MIN_HEIGHT_PERCENT :
            {
                FMinHeight = 0.01 * fabs( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue()) * FTotHeight;
                break;
            }
    }
}

void TRejectionState::SetTotAmounts(INT_FLOAT  totarea, INT_FLOAT totheight)
{
    FTotArea   = totarea;
    FTotHeight = totheight;
}

#ifdef TESTING
static void TRejectionState::Test(){

}
#endif

//------------------ TAlgoState -----------------

INT_FLOAT TAlgoState::ProcessCurrentData(INT_FLOAT data)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT result ;
    switch (FDetectNegativePeaks) { //s1

        case NEGPEAKS_DETECTION_ON :
            {
                FThresholdForNegPeaks = data;
                FDetectNegativePeaks = NEGPEAKS_DETECTION_RUNNING;
                result = data;
                break;
            }

        case NEGPEAKS_DETECTION_RUNNING :
            {
                result = data;
                break;
            }

        case NEGPEAKS_DETECTION_OFF :
            {
                switch (FInvertNegativePeaks) { //s2

                    case NEGPEAKS_INVERSION_ON :
                        {
                            FThresholdForNegPeaks = data;
                            FInvertNegativePeaks = NEGPEAKS_INVERSION_RUNNING;
                            result = data;
                            break;
                        }

                    case NEGPEAKS_INVERSION_RUNNING :
                        {
                            result = FThresholdForNegPeaks + fabs(data - FThresholdForNegPeaks);
                            break;
                        }

                    case NEGPEAKS_INVERSION_OFF :
                        {
                            switch (FClampNegativePeaks) { //s3

                                case NEGPEAKS_CLAMPING_ON :
                                    {
                                        FThresholdForNegPeaks = data;
                                        FClampNegativePeaks = NEGPEAKS_CLAMPING_RUNNING;
                                        result = data;
                                        break;
                                    }

                                case NEGPEAKS_CLAMPING_RUNNING :
                                    {
                                        if (data < FThresholdForNegPeaks)
                                        { 
                                            result = FThresholdForNegPeaks;
                                        }
                                        else
                                            {
                                            result = data;
                                        }
                                        break;
                                    }

                                case NEGPEAKS_CLAMPING_OFF :
                                    {
                                        result = data;
                                        break;
                                    }

                                default :
                                    {
                                        assert(false);
                                        result = 0 ;
                                        break;
                                    }
                            }
                            break;
                        }

                    default :
                        {
                            assert(false);
                            result = 0 ;
                            break;
                        }
                }
                break;
            }
        default :
            {
                assert(false);
                result = 0 ;
                break;
            }
    }
    return result ;
}

TAlgoState::TAlgoState()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FEstimateThresholdNow = false;
    FInvertNegativePeaks  = NEGPEAKS_INVERSION_OFF ;
    FClampNegativePeaks   = NEGPEAKS_CLAMPING_OFF  ;
    FDetectNegativePeaks  = NEGPEAKS_DETECTION_OFF ;
}

TAlgoState::~TAlgoState()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{

}

void TAlgoState::ProcessEvent(TIntegrationEvent& theEvent)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    switch (theEvent.Code()) {

        case INTEGRATIONEVENT_ESTIMTHRESHOLD :
            {
                FEstimateThresholdNow = true;
                break;
            }

        case INTEGRATIONEVENT_INVERTNEGPEAKS :
            {
                if ( ((TIntegrationEvent_OnOff*) &theEvent)->On())
                {
                    FInvertNegativePeaks = NEGPEAKS_INVERSION_ON ;
                }
                else
                    {
                    FInvertNegativePeaks = NEGPEAKS_INVERSION_OFF;
                }
                break;
            }

        case INTEGRATIONEVENT_CLAMPNEGPEAKS :
            {
                if ( ((TIntegrationEvent_OnOff*) &theEvent)->On())
                {
                    FClampNegativePeaks = NEGPEAKS_CLAMPING_ON ;
                }
                else
                    {
                    FClampNegativePeaks = NEGPEAKS_CLAMPING_OFF;
                }
                break;
            }

        case INTEGRATIONEVENT_DETECTNEGPEAKS :
            {
                if ( ((TIntegrationEvent_OnOff*) &theEvent)->On())
                {
                    FDetectNegativePeaks = NEGPEAKS_DETECTION_ON ;
                }
                else
                    {
                    FDetectNegativePeaks = NEGPEAKS_DETECTION_OFF;
                }
                break;
            }

        default:
            {
    #ifdef DEBUG
                dbgstream() << theEvent.Name() << " not taken into account. " << endl ;
    #endif
            }
    }
}

#ifdef TESTING
static void TAlgoState::Test(){

}
#endif

TParameterState::TParameterState(INT_FLOAT deltat)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
//ne pas oublier le constructeur de la classe de base
    FPeakWidth = 0;
    FbunchSize = 1;
    FThres = 0;
    FThresSolvent = INT_MAXFLOAT ;
    FMinHeight = 0;
    FMinArea = 0;
    FDeltaTime = deltat;
    FIntegrationOn = true;

    pFSkimRatios =  new TTimeAndValue(INTEGRATOR_DEFAULT_SKIM_RATIO);
}

TParameterState::~TParameterState()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    delete pFSkimRatios;
}

void TParameterState::SetPeakWidth(INT_FLOAT peakwidth)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FPeakWidth = peakwidth;
    FbunchSize = max(1, round(peakwidth / (FDeltaTime * INTEGRATOR_OPTIMAL_POINTS_PER_PEAK)));
#ifdef DEBUG
    dbgstream() << "New peak width : " << FPeakWidth << endl ;
#endif

}

void TParameterState::SetThreshold(INT_FLOAT thres, bool ReScale)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (ReScale)
        FThres = thres * FThresScale ;
    else
        FThres = thres ;
#ifdef DEBUG
    dbgstream() << "New threshold : " << FThres << endl;
#endif
}

void TParameterState::SetThresholdSolvent(INT_FLOAT thres)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FThresSolvent = thres ;
}

void TParameterState::AddThreshold(INT_FLOAT thres, bool Rescale)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (Rescale)
    {
        FThres += thres * FThresScale ;
    }
    else
        {
        FThres += thres;
    }
}

void TParameterState::SetThresScale(INT_FLOAT scale)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FThresScale = scale;
}

void TParameterState::ProcessEvent(TIntegrationEvent& theEvent)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    switch (theEvent.Code()) {
        case INTEGRATIONEVENT_SETPEAKWIDTH :
            {
                SetPeakWidth( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue());
                break;
            }
        case INTEGRATIONEVENT_DBLEPEAKWIDTH :
            {
                SetPeakWidth(FPeakWidth * 2.0);
                break;
            }
        case INTEGRATIONEVENT_HLVEPEAKWIDTH :
            {
                SetPeakWidth(FPeakWidth / 2.0);
                break;
            }
        case INTEGRATIONEVENT_SETTHRESHOLD :
            {
                SetThreshold( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue(), true);
                break;
            }
        case INTEGRATIONEVENT_ADDTHRESHOLD :
            {
                AddThreshold( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue(), true);
                break;
            }
        case INTEGRATIONEVENT_SETSKIMRATIO :
            {
                TIntegrationEvent_VALUE1* tmpEvt = (TIntegrationEvent_VALUE1*) &theEvent ;
                pFSkimRatios->Add(tmpEvt->Time(), tmpEvt->UserValue());
                break;
            }
        case INTEGRATIONEVENT_SETTHRESHOLD_SOLVENT :
            {
                SetThresholdSolvent( ((TIntegrationEvent_VALUE1*) &theEvent)->UserValue());
                break;
            }
        case INTEGRATIONEVENT_INTEGRATION :
            {
                FIntegrationOn = ((TIntegrationEvent_OnOff*) &theEvent)->On();
                break;
            }
        default:
            {
    #ifdef DEBUG
                dbgstream() << theEvent.Name() << "not yet taken into account." << endl ;
    #endif
            }
    }
}

#ifdef TESTING
static void Test(){

}
#endif

//---------------- TPeakState ------------------

TPeakState::TPeakState()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    PeakInProgress  = false;
    NbPeaks         = 0;
    NbNegativePeaks = 0;

    FMustStartNow = false;
    FMustEndNow   = false;
    FMustSplitNow = false;
    FSlicing      = false;
    FStartingSlice= false;
}

TPeakState::~TPeakState()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{

}

void TPeakState::FoundPeakStart(TSlopeType ch,  bool PreserveOldState)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    PeakInProgress = true ;
    FMustStartNow  = false; // cause it has been done
    FMustSplitNow  = false; // id
    FSlicing       = FStartingSlice;
    FStartingSlice = false;

// old state must sometimes be preserved, in case of Split Peak for example
    if (!PreserveOldState)
    {
        ApexFound = false;
        if ( (ch == PositiveSlope) ||
            (ch == ZeroSlope) ) // pas terrible, à améliorer. Ajouté à cause d'un début de pic forcé dans une zone de pente quasi-nulle
        {
            Change   = NegThenPos;
            peaktype = PositivePeak;
        }
        else
            {
            Change   = PosThenNeg;
            peaktype = NegativePeak;
        }
    }
}

void TPeakState::FoundApex(TSlopeChangeType ch)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    ApexFound = true;
    Change    = ch;
    NbPeaks++;
    if (ch == NegThenPos) NbNegativePeaks++;
}

void TPeakState::FoundValley(TSlopeChangeType ch)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    ApexFound = false ;
    Change    = ch    ;
}

void TPeakState::FoundPeakEnd()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    PeakInProgress = false;
    FMustEndNow    = false; // cause it has been done
}

void TPeakState::ProcessEvent(TIntegrationEvent& theEvent)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    switch (theEvent.Code()) {
        case INTEGRATIONEVENT_STARTPEAK_NOW :
            {
                FMustStartNow = true ;
                FMustEndNow   = false;
                FMustSplitNow = false;
                break;
            }
        case INTEGRATIONEVENT_ENDPEAK_NOW :
            {
                FMustStartNow = false;
                FMustEndNow   = true ;
                FMustSplitNow = false;
                break;
            }
        case INTEGRATIONEVENT_SPLITPEAK_NOW :
            {
                FMustStartNow = true ;
                FMustEndNow   = false;
                FMustSplitNow = true ;
                break;
            }
        case INTEGRATIONEVENT_SLICE_INTEGRATION :
            {
                bool boolval = ((TIntegrationEvent_OnOff*) &theEvent)->On() ;
                FMustStartNow = boolval;
                FMustEndNow = true; 
                FMustSplitNow = false;
                FStartingSlice = boolval;
                FSlicing = false ;
                break ;
            }
        default :
            {
    #ifdef DEBUG
                dbgstream() << theEvent.Name() << " not yet taken into account." << endl ;
    #endif
            }
    }
}

#ifdef TESTING
static void TPeakState::Test(){

}
#endif

//-------------- TNoiseComputer -----------------

void TNoiseComputer::ComputeNoiseValues(int i1, int i2)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT sumsqr = 0;
    int nbPoints = i2 - i1 + 1;
    LINE_FITTER LineFitter ;

    for (int j=i1; j<=i2; j++) LineFitter.AddPoint(j,FSignal[j]);

    INT_FLOAT a,b ;
    if (LineFitter.GetCoefs(b,a))
    {
// now we have y = ax+b
        INT_FLOAT vmin = INT_MAXFLOAT ;
        INT_FLOAT vmax = INT_MINFLOAT ;
        for (int j=i1; j<=i2; j++)
        {
            INT_FLOAT v = FSignal[j] - (a * j + b);
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
            sumsqr += v*v;
        }
        FNoise = vmax - vmin;
        FDrift = a / FDeltaTime;
        FRMSNoise = sqrt(sumsqr / (INT_FLOAT)nbPoints);
        FWasSuccessful = true;
    }
    else
        {
        FWasSuccessful = false;
    }
}

void TNoiseComputer::SetNoiseIntervalStart(int iStart)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FiStart = iStart;
}

void TNoiseComputer::SetNoiseIntervalEnd(int iEnd)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FiEnd = iEnd;
}

bool TNoiseComputer::ComputationIsFeasible()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool result =
        (FiStart >= 0) &&
        (FiEnd >= 0)   &&
        (FiStart < FSignalSize) &&
        (FiEnd < FSignalSize)   &&
        (FiStart < FiEnd - 1)   ; // we need at least two points

    return result ;
}

TNoiseComputer::TNoiseComputer()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FWasSuccessful = false;
    FiStart        = 0;
    FiEnd          = 0;
}

TNoiseComputer::~TNoiseComputer()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{

}

void TNoiseComputer::Initialize(INT_SIGNAL Signal, int SignalSize, INT_FLOAT DeltaTime)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(Signal);
    assert(DeltaTime > 0);

    FSignal     = Signal;
    FSignalSize = SignalSize ;
    FDeltaTime  = DeltaTime;
}

bool TNoiseComputer::ComputeNoiseInInterval()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (ComputationIsFeasible())
    {
        ComputeNoiseValues(FiStart, FiEnd);
    }
    else
        {
        FWasSuccessful = false;
    }
    return FWasSuccessful;
}

INT_FLOAT TNoiseComputer::GetNoise()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (FWasSuccessful)
        return FNoise ;
    else
        return 0;
}

INT_FLOAT TNoiseComputer::GetDrift()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (FWasSuccessful)
        return FDrift;
    else
        return 0;
}

INT_FLOAT TNoiseComputer::GetRMSNoise()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (FWasSuccessful)
        return FRMSNoise;
    else
        return 0;
}

void TNoiseComputer::ProcessEvent(TIntegrationEvent& theEvent)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(theEvent.Code() == INTEGRATIONEVENT_COMPUTENOISE);
    TIntegrationEvent_OnOff* pEvent =(TIntegrationEvent_OnOff*) &theEvent ;
    if (pEvent->On())
        SetNoiseIntervalStart(max(0, round(pEvent->Time() / FDeltaTime))) ;
    else
        SetNoiseIntervalEnd(min(FSignalSize - 1, round(pEvent->Time() / FDeltaTime)));
}

#ifdef TESTING
static void TNoiseComputer::Test(){

}

#endif

//------------------- TInternalIntegrator ----------------------

void TInternalIntegrator::Initialize()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
#ifdef DEBUG
    dbgstream() << "Initializing integrator" << endl ;
#endif
// the calling order must be strictly respected
    FNoiseComputer.Initialize(Signal, SignalSize, DeltaTime);

    SmoothWithWaveletsProc(Signal, SignalSize,SignalSmooth, FSpikeReduceParameter, FNoiseValue); // computes FNoiseValue as a side-effect, and in a rigourous way
    FnoiseSDev = FNoiseValue;

#ifdef DEBUG
    dbgstream() << "Smoothing done." << endl ;

//    ofstream file_signal("smooth") ;
//    for (int i=0; i<SignalSize; ++i)
//    {
//        file_signal << Signal[i] << "  " << SignalSmooth[i] << endl ;
//    }
#endif

    if (!FReduceNoise)
    {
// overwrite smoothed signal with raw signal
        for (int i=0;  i<SignalSize; i++) SignalSmooth[i] = Signal[i];
// slightly reduce noise std deviation, so that the user sees more peaks
        FNoiseValue /= 2.0;
    }

// this array of boolean will memorize indexes that correspond to integration off
// this will helps to avoid breaking the limits when moving the markers
    FOnIndexes.resize(SignalSize);
    SplitEvents();
}

void TInternalIntegrator::FindPeaksAverageSlope()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT curSample ;

    TCandidatePeak
    *prevPeak,
    *curPeak,
    *newPeak ;

    int 
        idxStartPeak ;

#ifdef DEBUG
    dbgstream() << "---------------- STARTING PEAK SEARCH ---------------" << endl ;
    dbgstream() << "DeltaT="<< DeltaTime<<"[min]  " ;
    dbgstream() << "Signal size=" << SignalSize << endl ;

    if (FReduceNoise)
        dbgstream() << "Noise Reduction Enabled" << endl ;
    else
        dbgstream() << "Noise Reduction Disabled" << endl ;

    if (FUseRelativeThreshold)
        dbgstream() << "Relative Threshold Enabled" << endl ;
    else
        dbgstream() << "Relative Threshold Disabled" << endl ;
#endif

    TAlgoState algoState ;
    TPeakDetector Detector ;
    TPeakState peakState ;
    TParameterState paramState(DeltaTime) ;

// to do below: clarify the computation of FSignalMaxValue. The trick is to avoid
// to compute the max twice by reusing the NormalizingValue, but it makes the code unclear.
    if (FUseRelativeThreshold)
    {
        paramState.SetThresScale(NormalizingValue());
        Detector.FSignalMaxValue = paramState.FThresScale * INTEGRATOR_REFSCALE;
    }
    else
        {
        paramState.SetThresScale(NORMALIZINGVALUE_ABSOLUTE_THRESHOLD);
        Detector.FSignalMaxValue = NormalizingValue() * INTEGRATOR_REFSCALE;
    }

// this array of boolean will memorize indexes that correspond to integration off
// this will helps to avoid breaking the limits when moving the markers
    FOnIndexes.resize(SignalSize);
    INT_FLOAT bunch = 0;
    int NbSamplesInBunch = 0;

    int iLastAdded = -1;
    int iLastAddedOld = -1;

    INT_FLOAT curTime = 0.0;

    for (int i = 0 ;
    i < SignalSize;
    i++)
    { // main loop on signal points
// processEvents
        paramState.UpdateFromEvents(curTime, FParameterEvents);
        algoState.UpdateFromEvents(curTime, FAlgoEvents);
        peakState.UpdateFromEvents(curTime, FPeakEvents);
        FNoiseComputer.UpdateFromEvents(curTime, FNoiseComputer.Events);//

// handle changes caused by particular events
// event? maybe we must estimate the threshold
        if (algoState.FEstimateThresholdNow)
        {
            INT_FLOAT tmpdble = paramState.FPeakWidth / DeltaTime ; //DT5803
            INT_FLOAT LocThresholdEstimate = ComputeLocalThreshold(i, paramState.FbunchSize, ceil(tmpdble));
            paramState.SetThreshold(LocThresholdEstimate, false);
            TAutoThresholdResult curresult ;
            curresult.RelativeThreshold =  paramState.FThres / NormalizingValue();
            curresult.Threshold = paramState.FThres ;
            curresult.Time = curTime ;
            AutoThresholdResults.push_back(curresult);
            algoState.FEstimateThresholdNow = false;
#ifdef DEBUG
            dbgstream() << "Autothreshold - " ;
            dbgstream() << "t = " << curTime << "  " ;
            dbgstream() << "Internal Threshold = " <<  paramState.FThres << "  " ;
            dbgstream() << "User Threshold = " << paramState.FThres / paramState.FThresScale ;
            dbgstream() << endl ;
#endif

        }
// event? maybe the threshold has changed (a value was provided by the user or estimated)
        Detector.FSlopeSensitivity = paramState.FThres;
        curSample = SignalSmooth[i];

// event? maybe events on negative peaks have something to say
        curSample = algoState.ProcessCurrentData(curSample);
// it is necessary to memorize inverting and clamping so that future operations
// (baseline computations in particular) will take them into account
// we do this in a rather crude way, by correcting the signals.
// This will not give good results
// in case the smoothed signal and the signal are very different.
// to be corrected in this happens in practice
// THIS APPROCH DOES NOT WORK WELL ENOUGH BECAUSE
// PEAK STARTS AND ENDS ARE NOT CORRECTLY PLACED. THEY SHOULD OCCUR
// AT THE POINTS WHERE THE SIGNAL CROSSES THE SPECIFIED LEVEL

// IMPORTANT WARNING: this approach modifies the signal that are passed
// to the DLL. This is OK for the moment, since the DLL receives copies of
// the signal, but may become wrong in the future.
        if ( (algoState.FInvertNegativePeaks != NEGPEAKS_INVERSION_OFF) ||
            (algoState.FClampNegativePeaks != NEGPEAKS_CLAMPING_OFF) )
        {
            assert(false); //this is some experimental code. Should not be used.
            SignalSmooth[i] = curSample;
            Signal[i] = curSample;
        }

// these events on negative peaks influence the detector
        Detector.NegPeaksAllowed = (algoState.FDetectNegativePeaks != NEGPEAKS_DETECTION_OFF);
        if (Detector.NegPeaksAllowed && (pCurNegLine == NULL))
        {
// creates a new line
//delete pCurNegLine; //commented during translation : should not be necessary because pCurNegLine is NULL
            int IndexEndNegLine = LocateEndNegLine(i, FAlgoEvents);
            if (IndexEndNegLine > i)
            {
                pCurNegLine = new TStraightLineTool_Integer(i, curSample, IndexEndNegLine, SignalSmooth[IndexEndNegLine]) ;
            }
            else
                {
                pCurNegLine = new TStraightLineTool_Integer(i, curSample, i + 1, curSample); //i ou i+1 ??
            }
        }

        if ( (Detector.NegPeaksAllowed) &&
            (peakState.PeakInProgress) )
        {
//assert(curNegLine);
            if ( (curPeak->FisPositive) &&
                (curSample <= pCurNegLine->ValueAt(i)) ||
                (! curPeak->FisPositive) &&
                (curSample >= pCurNegLine->ValueAt(i)) )
            {
                peakState.FMustStartNow = ! peakState.PeakInProgress; // true replaced by BO 22/9/99, due to Issue #1451
            }
        }

// special case : the last sample should end any peak in progress
// note that we do not handle this via FmustEndNow since this would prevent
// the algorithm to remove the possible last peak
        bool LastDataPoint = (i == (SignalSize-1));

// now let us go with the peak detection business
        bunch = bunch + curSample;
        NbSamplesInBunch++;

        if ( ((i % paramState.FbunchSize) == 0) ||
            (peakState.FMustStartNow) ||
            (peakState.FMustEndNow) ||
            (peakState.FMustSplitNow) ||
            LastDataPoint )  // these events break the bunching process temporarily
        { //new bunch

            assert(NbSamplesInBunch > 0);

            Detector.Push(bunch / NbSamplesInBunch);
            bunch = 0;
            NbSamplesInBunch = 0;

            if (! peakState.PeakInProgress )
            { //no peak in progess
                if (peakState.FMustEndNow)
                {
// this situation entails that previous peak must be extended
                    if (FCandidPeaks.size() > 0)
                    {
                        if (! FCandidPeaks.back().FForcedEnd)
                        {
                            iLastAdded = AdjustLimit(CheckLastAdded(iLastAdded, i), IS_RIGHTLIMIT);
// update peak
                            curPeak->EndIndex = iLastAdded ;
                            curPeak->FForcedEnd = peakState.FMustEndNow;
// reset state

                            peakState.FoundPeakEnd();
                        }
                    }
                }

                if (ThereIsaPeakStart(Detector, paramState, algoState, peakState, curSample))
                {
                    iLastAddedOld = iLastAdded;
                    iLastAdded = AdjustLimit(CheckLastAdded(iLastAdded, i - 3 * paramState.FbunchSize + 1), IS_LEFTLIMIT);
// create peak
                    if (peakState.FSlicing) 
                        idxStartPeak = i ;
                    else
                        idxStartPeak = iLastAdded;

                    curPeak = &(FCandidPeaks.push_back()); //creates the peak inside the owning list

                    curPeak->isUserSlice = peakState.FStartingSlice ;
                    curPeak->StartIndex = idxStartPeak ;
                    curPeak->BunchSize = paramState.FbunchSize ;
                    curPeak->FForcedStart = (peakState.FMustStartNow || peakState.FMustSplitNow);
                    curPeak->FDropLineAtStart = peakState.FMustSplitNow;
                    if (peakState.FMustSplitNow)
                    {
                        if (FCandidPeaks.size() > 1)
                        {
                            curPeak->FisPositive = FCandidPeaks[FCandidPeaks.size()-2].FisPositive ;// same sign as the previous peak
                        }
                        else
                            {
                            curPeak->FisPositive = true ;// default value, this case should not occur
                        }
                    }
                    else
                        {
                        curPeak->FisPositive = (Detector.SlopeTypeNoZero() == PositiveSlope);
                    }

// new: try to improve peak location
                    if ( (! curPeak->FForcedStart) && LastTwoPeaksHaveSameSign())
                    {
                        iLastAdded = FindLocalExtrema( max(max(0, iLastAddedOld), i - 5 * paramState.FbunchSize) ,
                            max(i, iLastAddedOld),
                            curPeak->FisPositive);
                        iLastAdded = AdjustLimit(iLastAdded, IS_LEFTLIMIT);
                        curPeak->StartIndex = iLastAdded;
                    }

#ifdef DEBUG
                    dbgstream() << "t=" << curTime
                    << " : Found peak start"
                    << "i=" << i << " "
                    << "EndIndex=" << iLastAdded
                    << endl ;
#endif
                    AddCandidatePeak(*curPeak);
                    peakState.FoundPeakStart(Detector.SlopeTypeNoZero(), peakState.FMustSplitNow);
                    Detector.ShoulderReset();
//(peak start)
                }//peak in progress
            }
            else
                { // peak in progress
// memorize max slope for solvent peak identification and removal
                curPeak->UpdateMaxSlope(Detector.B3 - Detector.B2);

// 17-07-1999 BO. I realize that do not know whether it is better to detect
// "peak end" before "peak valleys". This should be investigated later.
                if ( ThereIsaPeakEnd(Detector, paramState, peakState) || // and not FlatPeakTop(i, ceil(paramState.FPeakWidth/DeltaTime), paramState.FthresScale)
                    LastDataPoint )
                {
                    iLastAddedOld = iLastAdded;
                    iLastAdded = AdjustLimit(CheckLastAdded(iLastAdded, i), IS_RIGHTLIMIT);
// update peak
                    curPeak->EndIndex = iLastAdded;
                    curPeak->FForcedEnd = peakState.FMustEndNow || peakState.FMustSplitNow;
// new: improve peak end
// cette technique pose le problème suivant: iLastAdded peut  devenir > i
// ce qui contredit une hypothèse implicite dans l'algorithme de détection
// Les effets sont donc à examiner très sérieusement; il pourra être nécessaire
// de désactiver cette Options

                    if (! curPeak->FForcedEnd)
                    {
                        iLastAdded = FindLocalExtrema(max(i, iLastAddedOld), min(i + paramState.FbunchSize * 2, SignalSize - 1), curPeak->FisPositive);
                        iLastAdded = AdjustLimit(iLastAdded, IS_RIGHTLIMIT);
                        curPeak->EndIndex = iLastAdded;
                    };
#ifdef DEBUG
                    dbgstream() << "t="
                    << curTime
                    << " : peak end found"
                    << "i=" << i << " "
                    << "EndIndex=" << iLastAdded
                    << endl ;
#endif
                    if (CurrentPeakIsGood(paramState))
                    {
                        HandleFusedPeaks();
                    }
                    else // remove peak
                        {
                        FCandidPeaks.pop_back() ;
                        if (FCandidPeaks.size() >= 1)
                        {
                            prevPeak = &(FCandidPeaks.back()) ;
                            prevPeak->FEndIsValley = false;
                        }
                    }//else currentPeakIsGood

// reset state
                    peakState.FoundPeakEnd();

                    goto EndPeakInProgress;
                }//end : there is a peak end

                if (ThereIsaPeakApex(Detector, paramState, peakState))
                {
                    Detector.ShoulderReset();
/* the iLastAdded mechanism below may be too simple. It causes
		   situations like in issue #2294, where FApexIndex = FStartIndex;
		   Such a situation entails problems when FStartIndex is moved (due to baseline corrections)
		   but not FApexIndex: the data coherence is corrupted, because we have
		   FApexIndex < FStartIndex !
		   There are several options:
		   1- keep the mechanism, and reject such peaks
		   2- find a better mechanism
		   3- (with 1 or 2) : check the coherence when peak start/end are moved toward the apex:
		      they should not pass the apex

		   On 02/02/2000, I think that 1- is enough because the problem appears (in my opinion) only
		   with thin peaks and with a PeakWidth detection parameter too large to correctly detect
		   these peaks. Thus with 1- these peaks will just disappear. 3- is probably necessary but
		   asks for refactoring: do not move directly the start/end but call a method in order
		   to perform a few checks. I may refactor this later if necessary.
		*/
#ifdef DEBUG
                    dbgstream() << "t=" << curTime << " : peak apex found  " ;
                    dbgstream() << "i=" << i << " " ;
                    dbgstream() << "EndIndex=" << iLastAdded << endl ;
#endif
                    iLastAdded  = CheckLastAdded(iLastAdded, i - 2 * paramState.FbunchSize); // no limit check since we should be inside an integration zone, by definition
                    peakState.FoundApex(Detector.SlopeChange());
// update peak
                    curPeak->ApexIndex = iLastAdded;
                    goto EndPeakInProgress;
                }//ThereIsAPeakApex

                if (ThereIsaValley(Detector, paramState, peakState))
                {
                    Detector.ShoulderReset();
                    iLastAddedOld = iLastAdded;
                    iLastAdded = FindLocalExtrema(max(iLastAddedOld, i - 2 * paramState.FbunchSize), min(i, SignalSize - 1), curPeak->FisPositive);
                    peakState.FoundValley(Detector.SlopeChange());
// update peak and create new peak
                    curPeak->EndIndex = iLastAdded;
                    bool curPeakOk = CurrentPeakIsGood(paramState);
#ifdef DEBUG
                    dbgstream() << "t=" << curTime << "  " ;
                    dbgstream() << " : peak valley found " ;
                    dbgstream() << "i=" << i << "  " ;
                    dbgstream() << "EndIndex=" << iLastAdded ;
                    dbgstream() << endl ;
#endif
                    if (curPeakOk)
                    {
                        curPeak->FEndIsValley = true;
                        HandleFusedPeaks();
                    }
                    else  // remove peak
                        {
                        FCandidPeaks.pop_back() ;
                        if (FCandidPeaks.size() >= 1)
                        {
                            prevPeak = &(FCandidPeaks.back()) ;
                            prevPeak->FEndIsValley = false;
                        }
                    }
                    newPeak = &(FCandidPeaks.push_back());
                    newPeak->StartIndex = iLastAdded ;
                    newPeak->BunchSize = paramState.FbunchSize ;
                    newPeak->FStartIsValley = curPeakOk; // false if curPeak was removed
                    newPeak->FisPositive = (Detector.SlopeChange() != PosThenNeg);
                    AddCandidatePeak(*newPeak);

                    curPeak = newPeak;
                    goto EndPeakInProgress;
                }//ThereIsAValley

                if ( (Detector.PotentialShoulder()) && (paramState.FIntegrationOn) )
                {
                    iLastAdded = CheckLastAdded(iLastAdded, i - 2 * paramState.FbunchSize);
#ifdef DEBUG
                    dbgstream() << "t=" << curTime << "  " ;
                    dbgstream() << " : shoulder found " ;
                    dbgstream() << "i=" << i << "  " ;
                    dbgstream() << "EndIndex=" << iLastAdded ;
                    dbgstream() << endl ;
#endif
                    goto EndPeakInProgress;
                }

                EndPeakInProgress: {}
            }
        }
        curTime += DeltaTime ;
    }//for

// the algorithm above can sometimes have a peak in progress. This peak
// must be ended
    if (peakState.PeakInProgress)
    {
        curPeak->EndIndex = SignalSize-1 ;
        curPeak->FForcedEnd = peakState.FMustEndNow;
        if (CurrentPeakIsGood(paramState))
        { 
            HandleFusedPeaks();
        }
        else
            {
            FCandidPeaks.pop_back();
            if (FCandidPeaks.size() >= 1)
            {
                prevPeak = &(FCandidPeaks.back());
                prevPeak->FEndIsValley = false;
            }
        }
        peakState.FoundPeakEnd();
    }//PeakInProgress

#ifdef DEBUG
    dbgstream() << "-- DETECTION FINISHED "
    << FCandidPeaks.size() << "  candidate peaks "
    << endl ;

    for (int ipeak = 0 ;
    ipeak < FCandidPeaks.size();
    ++ipeak)
    {
        TCandidatePeak& curpeak = FCandidPeaks[ipeak] ;
        dbgstream() << "#" << ipeak
        << " " << curpeak.StartIndex
        << " " << curpeak.EndIndex
        << endl ;
    }
#endif

// the order of the following operations is MANDATORY
//0
// default baseline that help to place the peak markers
    CreateDefaultBaseLines();

// will also delete all default baselines. It would be better to delete
// only the default lines which overlaps event lines (and thus skip step 3),
// but this is too complex with the current data structures

    CreateEventBaselines(*(paramState.pFSkimRatios));
    if (FComputeBaseLines) CleanCandidateLines();

// now we create the final default baselines
    if (FComputeBaseLines) CreateDefaultBaseLines();

// BO 13/03/01 DT 3924. Added CorrectSkimming that was forgotten after INUS fixes in version 57 of this file.
    CorrectSkimming();

//3
    CreateResultLines(); // prepare the result lines and associates them to the candidate peaks (link is a pointer)

//4
    CreateResultPeaks(); // prepare the result peaks and associates them to the lines (link is an integer)

    ComputeNoiseFromProcessedIntegrationEvents();

#ifdef DEBUG
    dbgstream() << "---------------- PEAK SEARCH END ---------------" << endl ;
#endif
}

void TInternalIntegrator::ComputeNoiseFromProcessedIntegrationEvents()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FNoiseComputer.ComputeNoiseInInterval();
}
int TInternalIntegrator::LocateEndNegLine(int i,
    TIntegrationEventList& AlgoEvents)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// locate the first event Detect Negative Peaks Off after t=i*DeltaTime
// and returns the corresponding index
    unsigned int  idxEvent = 0;
    bool found    = false;
    INT_FLOAT itime = i * DeltaTime;

    TIntegrationEvent* event;
    while ( (!found) && (idxEvent < AlgoEvents.size()) )
    { 
        event = AlgoEvents[idxEvent];
        if ( (event->Time() > itime) &&
            (event->Code() == INTEGRATIONEVENT_DETECTNEGPEAKS) )
        {
            found = !( ((TIntegrationEvent_OnOff*) event)->On() ) ;
            idxEvent++;
        }
    }

    if (found)
        return AdjustIndexToSignal(round(event->Time() / DeltaTime)) ;
    else
        return i;
}

TInternalIntegrator::TInternalIntegrator()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FReduceNoise = false;
    FSpikeReduceParameter = 1;
    pCurNegLine = NULL;
}

void TInternalIntegrator::ClearEventList()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    for(TIntegrationEventList::iterator evtit = Events.begin();
    evtit!=Events.end();
    ++evtit)
    {
        delete *evtit ;
    }
    Events.clear();
}

TInternalIntegrator::~TInternalIntegrator()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (pCurNegLine != NULL) delete pCurNegLine ;
    ClearEventList();
}
void TInternalIntegrator::PropagateValleyChanges(TCandidatePeak& FirstPeak, TCandidatePeak&  LastPeak)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// valley status of the peak has just changed and must
// be propagated to neighbors

// NB: FirstPeak and LastPeak can be the same peak
    assert(&LastPeak!=NULL);
    assert(&FirstPeak!=NULL);

    if (FirstPeak.myIndexInTheWholeList > 0)
    {
        TCandidatePeak& prevPeak = FCandidPeaks[FirstPeak.myIndexInTheWholeList - 1];
        prevPeak.FEndIsValley = FirstPeak.FStartIsValley;
        prevPeak.FFusedEnd = (prevPeak.FFusedEnd && FirstPeak.FFusedStart);
    }

    if (LastPeak.myIndexInTheWholeList < (FCandidPeaks.size()-1))
    {
        TCandidatePeak& curPeak = FCandidPeaks[LastPeak.myIndexInTheWholeList + 1];
        curPeak.FStartIsValley = LastPeak.FEndIsValley;
        curPeak.FFusedStart = (curPeak.FFusedStart && LastPeak.FFusedEnd);
    }
}
void TInternalIntegrator::SplitEvents()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(&FParameterEvents!=NULL);
    assert(&FAlgoEvents!=NULL);
    assert(&FPeakEvents!=NULL);
    assert(&FBaseLineEvents!=NULL);
    assert(&FAddPeaksEvents!=NULL);
    assert(&FRejectionEvents!=NULL);
    assert(&Events!=NULL);

// to the developper: if you add a FxxxxEvent
// then do not forget to sort it before exiting the procedure
    for (TIntegrationEventList::iterator curEvent_it = Events.begin();
    curEvent_it != Events.end();
    curEvent_it++)
    {
        TIntegrationEvent* pCurEvent = (*curEvent_it);
#ifdef DEBUG
        dbgstream() << "Event "  << pCurEvent->Name()
        << " (code " << pCurEvent->Code()
        << ") at time " << pCurEvent->Time()
        << endl ;
#endif
        if (pCurEvent->Code() < INTEGRATIONEVENT_PARAM_EVENT_MAX)
            FParameterEvents.push_back(pCurEvent);
        else
            if (pCurEvent->Code() < INTEGRATIONEVENT_ALGO_EVENT_MAX)
                FAlgoEvents.push_back(pCurEvent);
        else
            if (pCurEvent->Code() < INTEGRATIONEVENT_PEAK_EVENT_MAX)
                FPeakEvents.push_back(pCurEvent);
        else
            if (pCurEvent->Code() < INTEGRATIONEVENT_BASELINE_EVENT_MAX)
                FBaseLineEvents.push_back(pCurEvent);
        else
            if (pCurEvent->Code() < INTEGRATIONEVENT_REJECTION_MAX)
                FRejectionEvents.push_back(pCurEvent);
        else
            if (pCurEvent->Code() == INTEGRATIONEVENT_ADDPEAKS)
                FAddPeaksEvents.push_back(pCurEvent);
        else
            if (pCurEvent->Code() == INTEGRATIONEVENT_COMPUTENOISE)
                FNoiseComputer.Events.push_back(pCurEvent);
    } //loop on events

// sorting the events by time is MANDATORY
    sort(FParameterEvents.begin(), FParameterEvents.end(),TIntegrationEvent::lessTime());
    sort(FAlgoEvents.begin(), FAlgoEvents.end(),TIntegrationEvent::lessTime());
    sort(FPeakEvents.begin(), FPeakEvents.end(),TIntegrationEvent::lessTime());
    sort(FBaseLineEvents.begin(), FBaseLineEvents.end(),TIntegrationEvent::lessTime());
    sort(FAddPeaksEvents.begin(), FAddPeaksEvents.end(),TIntegrationEvent::lessTime());
    sort(FRejectionEvents.begin(), FRejectionEvents.end(),TIntegrationEvent::lessTime());

// additional work on events
    ExtractOffIndexes();
}

void TInternalIntegrator::ExtractOffIndexes()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool lastState = true; // we start implicitly with integration ON
    bool curState ;
    int  lastIdx = -1;
    int  curIdx ;

    for (TIntegrationEventList::iterator event_it = FParameterEvents.begin();
    event_it != FParameterEvents.end();
    event_it++)
    {
        TIntegrationEvent* pCurEvent = (*event_it) ;
        if (pCurEvent->Code() == INTEGRATIONEVENT_INTEGRATION)
        {
            curIdx = min(SignalSize-1, round(pCurEvent->Time() / DeltaTime));
            curState = ((TIntegrationEvent_OnOff*) pCurEvent)->On() ;

            for (int i=lastIdx+1; i<=curIdx; i++) FOnIndexes[i] = lastState;
#ifdef DEBUG
            if (lastState)
                dbgstream() << "OnIndexes: TRUE between ["
            << (lastIdx+1)*DeltaTime
            << ","
            << curIdx * DeltaTime
            << "]" << endl;
            else
                dbgstream() << "OnIndexes: FALSE between ["
            << (lastIdx+1)*DeltaTime
            << ","
            << curIdx * DeltaTime
            << "]" << endl;
#endif
            lastState = curState;
            lastIdx   = curIdx;
        }
    }

    curIdx = SignalSize - 1;
    for (int i=lastIdx+1; i<=curIdx; i++) FOnIndexes[i]=lastState;

#ifdef DEBUG
    if (lastState)
        dbgstream() << "OnIndexes: TRUE between ["
    << (lastIdx+1)*DeltaTime
    << ","
    << curIdx * DeltaTime
    << "]" << endl;
    else
        dbgstream() << "OnIndexes: FALSE between ["
    << (lastIdx+1)*DeltaTime
    << ","
    << curIdx * DeltaTime
    << "]" << endl;
#endif
}

int TInternalIntegrator::AdjustLimit(int OriginalLimit, int inLimitType)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(OriginalLimit < SignalSize);
    int result ;
    int OriginalLimitCorrected = max(OriginalLimit, 0);

    if (FOnIndexes[OriginalLimitCorrected])
        result = OriginalLimitCorrected ;
    else
        {
        bool stopSearch = false;
        int idxSearch = OriginalLimitCorrected;
        while ( (!stopSearch)    &&
            (idxSearch >= 0) &&
            (idxSearch < SignalSize) )
        {
            stopSearch = FOnIndexes[idxSearch];
            idxSearch += inLimitType;
        }
        result = idxSearch - inLimitType;
    }
    return result ;
}

int TInternalIntegrator::AdjustIndexToSignal(int index)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    int result ;
    if (index < 0)
        result = 0 ;
    else
        if (index >= SignalSize)
            result = SignalSize - 1 ;
    else
        result = index;
    return result;
}

INT_FLOAT TInternalIntegrator::NormalizingValue()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT result ;
    INT_FLOAT Max = -1;
    for (int i=0; i<SignalSize; i++)
    {
        INT_FLOAT val = fabs(Signal[i]);
        if (val > Max) Max = val;
    }
    if (Max > 0)
        result = Max / INTEGRATOR_REFSCALE ;
    else
        result = 1; // case of a null signal

    return result ;
}

void TInternalIntegrator::ComputeLocalThresholdWithNoise(int curPos,
    int bunchSize,
    int searchWidth,
    INT_FLOAT& Thresh,
    INT_FLOAT& Noise)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT sumThres2 = 0;
    INT_FLOAT sumThres  = 0;
    int nbval = 0;

    int maxindex = min(SignalSize-1,curPos+9) ;
    for (int inPos=curPos; inPos<=maxindex; inPos++)
    {
        INT_FLOAT delta = Signal[inPos] - Signal[inPos-1] ;
        sumThres  += delta;
        sumThres2 += delta*delta ;
        nbval++;
    }

    if (nbval > 0)
    {
        Thresh = sumThres / nbval /DeltaTime;
        Noise  = sumThres2 / nbval / (DeltaTime*DeltaTime);
        INT_FLOAT tmp = Noise-(Thresh*Thresh) ;
        if (tmp>0)
            Noise = sqrt(tmp);
        else
            Noise = sqrt(-tmp); //in that case there is only a floating point precision problem, tmp is very small
    }
    else
        {
        Thresh = 0 ;
        Noise  = 0 ;
    }

    Noise = 5*Noise;
}

INT_FLOAT TInternalIntegrator::ComputeLocalThreshold(int curPos,
    int bunchSize,
    int searchWidth)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
#ifdef DEBUG
    dbgstream() << "Computing threshold "
    << "pos:" << curPos << " "
    << "bch:" << bunchSize << " "
    << "wdt:" << searchWidth << " - "
    << flush ;
#endif

    INT_FLOAT sumThres = 0;
    int nbVal = 0;

    int maxindex = min(SignalSize-1, curPos + searchWidth);
    for ( int inPos = (curPos + 2 * bunchSize);
    inPos<=maxindex;
    inPos++)
    {
        sumThres += fabs(Signal[inPos] - Signal[inPos - 2 * bunchSize]);
        nbVal++;
    }

#ifdef DEBUG
    dbgstream() << "sum:" << sumThres << " nval:" << nbVal << endl;
#endif

    INT_FLOAT result ;
    if (nbVal > 0)
        result = sumThres / (INT_FLOAT) nbVal ;
    else
        result = 0;

    return result ;
}

void TInternalIntegrator::HandleFusedPeaks()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(&FCandidPeaks!=NULL);
    int newlimit ;

    if (FCandidPeaks.size() >= 2)
    {
        TCandidatePeak& curPeak = FCandidPeaks.back();
        TCandidatePeak& prevPeak = FCandidPeaks[FCandidPeaks.size()-2];

        assert(curPeak.EndIndex > 0);
        assert(prevPeak.EndIndex > 0);

        int w1 = curPeak.EndIndex  - curPeak.StartIndex;
        int w2 = prevPeak.EndIndex - prevPeak.StartIndex;
        int dist = curPeak.StartIndex - prevPeak.EndIndex;

// in the case where one extremity is fixed, my initial idea was
// to move the valley to this fixed point, but this give baselines
// that are not very natural.
// Thus if one extremity is fixed, no valley will be added.
        if (!(curPeak.FForcedStart || prevPeak.FForcedEnd))
        {
            INT_FLOAT ratio ;
            if (dist > 0)
                ratio = max(w1, w2) / dist ;
            else
                ratio = INTEGRATOR_FUSED_PEAKS_RATIO;

// we must check that there is no integration-off region between the two peaks
            bool PossibleFuse = true;
            int idx = prevPeak.StartIndex + 1;
            while ( (PossibleFuse) && (idx < curPeak.StartIndex) )
            {
                PossibleFuse = FOnIndexes[idx];
                idx++;
            }

            if ( (ratio >= INTEGRATOR_FUSED_PEAKS_RATIO) && PossibleFuse)
            { // the peaks are fused (not fully resolved)

                assert(prevPeak.EndIndex <= curPeak.StartIndex);
// previous version: search was performed between prevPeak.FEndIndex and curPeak.FStartIndex
// but this led to misplaced valleys in case prevPeak.FEndIndex is detected to late (due to
// a high threshold for instance)
// newLimit := FindLocalExtrema(prevPeak.FEndIndex, curPeak.FStartIndex, curPeak.FisPositive) ;

// new version 18/07/1999 (the search limits are chosen rather empirically. to be studied)
// Remark: it is maybe not necessary to handle this case separately from the case where
// there is a valley between the peaks (to be studied)
// BO 05/01/2001 Addex min/max in order to respect the assumptions of  FindLocalExtrema (DT3623)
                newlimit = FindLocalExtrema( max(prevPeak.EndIndex - 2 * prevPeak.BunchSize, 0),
                    min(curPeak.StartIndex + curPeak.BunchSize, SignalSize - 1),
                    curPeak.FisPositive);

#ifdef DEBUG
                dbgstream() << "Adding a valley at t="
                << newlimit * DeltaTime
                << " min." << endl ;
#endif
// modify the peaks according to the new limit
                prevPeak.EndIndex   = newlimit;
                prevPeak.FFusedEnd  = true;
                curPeak.StartIndex  = newlimit;
                curPeak.FFusedStart = true;
            }
        }
// if there is a valley between the peaks
// we try to improve its location
        else
            {
            if (curPeak.FStartIsValley &&
                prevPeak.FEndIsValley &&
                (!curPeak.FForcedStart) &&
                (!prevPeak.FForcedEnd) )
            {
// now we try to improve the valley
// by searching between the apexes of the neighbors
                if (prevPeak.ApexIndex == 0) // no apex
                    newlimit = FindLocalExtrema((prevPeak.EndIndex + prevPeak.StartIndex) / 2, curPeak.ApexIndex, curPeak.FisPositive);
                else
                    if (curPeak.ApexIndex == 0) // no apex
                        newlimit = FindLocalExtrema(prevPeak.ApexIndex, (curPeak.EndIndex + curPeak.StartIndex) / 2, curPeak.FisPositive);
                else
                    newlimit = FindLocalExtrema(prevPeak.ApexIndex, curPeak.ApexIndex, curPeak.FisPositive);

#ifdef DEBUG
                dbgstream() << "Moving a valley to t="
                << newlimit * DeltaTime
                << " min" << endl ;
#endif
// modify the peaks according to the new limit
                prevPeak.EndIndex  = newlimit;
                curPeak.StartIndex = newlimit;
            }
            else
                {
                if ( curPeak.FDropLineAtStart &&
                    curPeak.FForcedStart &&
                    prevPeak.FForcedEnd)
                {
                    prevPeak.FFusedEnd  = true;
                    curPeak.FFusedStart = true;
                }
            }
        }
    }
}

bool TInternalIntegrator::CurrentPeakIsGood(TParameterState& paramState)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT AVEHEIGHTFACTOR = 4.0  ; // value chosen empirically
    INT_FLOAT NBSIGNFACTOR    = 0.35 ; // value chosen empirically

// check whether the last peak added is a good peak (ie different from noise,
// no strange shape, etc.)
    assert((FCandidPeaks.size() >= 1));
    TCandidatePeak& CurPeak = FCandidPeaks.back();

// validity checks
    bool seindex = (CurPeak.EndIndex > CurPeak.StartIndex);
    bool apexvalid = CurPeak.ApexIndexIsValid() ;
    bool PeakIsOk = (  seindex && apexvalid  &&   // added 02/02/2000 by BO. cf issue #2294.
        ( (CurPeak.ApexIndex > CurPeak.StartIndex) ||
        paramState.FIntegrationOn ) );  // in case integration is turned off before a peak apex, the peak is discarded

// is it a solvent peak ?
    PeakIsOk = PeakIsOk && (CurPeak.MaxSlope() < paramState.FThresSolvent);

// we do not check and thus do not remove user-defined peaks
    if (PeakIsOk && (!CurPeak.FForcedStart) && (!CurPeak.FForcedEnd) )
    {
// compute a few criteria with respect to a pseudo baseline
// drawn from the start to the end of the peak using Signal values
// and not SignalSmooth (does not seem so useful in the tests we have
// performed)

// main weakness of this approach: using this baseline does not give
// an accurate estimate of the true heigth. In particular we can not
// remove peaks with a negative heigth when negative peaks are not allowed

        assert((CurPeak.EndIndex < SignalSize));
        TStraightLineTool_Integer defaultLine(CurPeak.StartIndex,
            SignalSmooth[CurPeak.StartIndex],
            CurPeak.EndIndex,
            SignalSmooth[CurPeak.EndIndex]);
        INT_FLOAT maxHeight = 0;
        INT_FLOAT maxHeightPosition = -1;
        INT_FLOAT maxHeightNeg = 0;
        INT_FLOAT maxHeightPos = 0;
        INT_FLOAT aveHeight = 0;
        INT_FLOAT v = 0;
        int nbSignChange = 0;

        for (int i=CurPeak.StartIndex;
        i<=CurPeak.EndIndex;
        i++)
        {
            INT_FLOAT pv = v;
            v = Signal[i] - defaultLine.ValueAt(i);
            aveHeight += v;
            if (fabs(v) >= maxHeight)
            {
                maxHeight = fabs(v);
                maxHeightPosition = i;
            }
            if ( (v > 0) && (v > maxHeightPos))
            { 
                maxHeightPos = v;
            }
            else
                {
                if ( (v < 0) && (v < maxHeightNeg))
                    maxHeightNeg = v;
            }

            if (v * pv < 0) nbSignChange++;
        }

        aveHeight /=  (CurPeak.EndIndex - CurPeak.StartIndex);

// detect strange peaks
// ex: peaks whose apex is extremely close to their extremities
        bool StrangePeak ;
        if (maxHeightPosition > 0)
        {
            StrangePeak = (fabs(maxHeightPosition - (INT_FLOAT)CurPeak.EndIndex) < INTEGRATOR_MIN_POINTS_APEX_EXTREM)
            || (fabs(maxHeightPosition - (INT_FLOAT)CurPeak.StartIndex) < INTEGRATOR_MIN_POINTS_APEX_EXTREM);
        }
        else
            {
            StrangePeak = false;
        }

        bool NoisyFlatPeak = ((INT_FLOAT)nbSignChange / ((INT_FLOAT)CurPeak.EndIndex - (INT_FLOAT)CurPeak.StartIndex)) > NBSIGNFACTOR;

        bool WrongSign = (CurPeak.FisPositive && (fabs(maxHeightNeg) > maxHeightPos * 1.25)) ||
            (!CurPeak.FisPositive && (fabs(maxHeightNeg) < maxHeightPos * 1.25));

        PeakIsOk =
            ((fabs(maxHeightNeg) > FNoiseValue) || (fabs(maxHeightPos) > FNoiseValue))
        && !StrangePeak && !NoisyFlatPeak
        && (AVEHEIGHTFACTOR * fabs(aveHeight) > FNoiseValue)
        && !WrongSign;

#ifdef DEBUG
//        dbgstream() << "Strange Shape params :  "
//        << "maxpos:" << maxHeightPosition << "  "
//        << "start:"  << CurPeak.StartIndex << "  "
//        << "end:"    << CurPeak.EndIndex
//        << endl;
        if (!PeakIsOk)
        {
            dbgstream() << "Removing peak starting at "
            << CurPeak.StartIndex * DeltaTime
            << " : " ;
            if (StrangePeak)
                dbgstream() << "Strange Shape  "
            << "maxpos:" << maxHeightPosition << "  "
            << "start:"  << CurPeak.StartIndex << "  "
            << "end:"    << CurPeak.EndIndex;
            else
                if (NoisyFlatPeak)
                    dbgstream() << "Noisy Flat Peak";
            else
                if (WrongSign)
                dbgstream() << "Wrong Peak Sign"
            << " positive : " << CurPeak.FisPositive
            << " maxHeightNeg : "<< maxHeightNeg
            << " maxHeightPos : "<< maxHeightPos;
            else
                dbgstream() << "Not high enough/noise ["
            << FNoiseValue <<"]";
            dbgstream() << endl ;
        }
#endif
    }
    else
    {
#ifdef DEBUG
        if (!PeakIsOk) dbgstream() << "The peak is not valid. Removed.  "<<endl;
#endif

    }
    return PeakIsOk;
}

int TInternalIntegrator::FindLocalExtrema(int startIndex,
    int endIndex,
    bool searchTheMin)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// we use the smoothed signal for the moment
// we may use the original signal if markers are misplaced due to smoothing
// but I have not yet seen this problem in practice. BO 14/07/1999

    assert(startIndex <= endIndex);
    assert(startIndex >= 0);
    assert(endIndex < SignalSize);

    int extremPos = startIndex;
    INT_FLOAT extrem = SignalSmooth[extremPos];

    for (int curPos=startIndex+1;
    curPos<=endIndex;
    curPos++)
    {
        if ( (searchTheMin) &&
            (SignalSmooth[curPos] < extrem) ||
            (!searchTheMin) &&
            (SignalSmooth[curPos] > extrem) )
        {
            extremPos = curPos;
            extrem = SignalSmooth[extremPos];
        }
    }
    assert((extremPos >= startIndex) && (extremPos <= endIndex));
    return extremPos;
}

void TInternalIntegrator::CreateResultPeaks()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * Comment : Seen a potential bug during the translation :
 *           the height of the mother peak could be wrong in the case where
 *           one skimmed peak is higher than the mother peak.
 * History :
 * -------------------------- */
{
    INT_FLOAT totArea=0;  
    INT_FLOAT totHeight=0;
    ComputePeaksProperties(totArea, totHeight) ;
    RejectPeaks(totArea, totHeight);
    SetupPeaksCode();
    CorrectForSkimming();
    RejectLinesWithNoPeak();
}

void TInternalIntegrator::ComputePeaksProperties(INT_FLOAT& totArea, INT_FLOAT& totHeight)
{
    PEAK_COMPUTER peakComputer(Signal, SignalSize, SignalSmooth, DeltaTime);

    totArea   = 0;
    totHeight = 0;
    TCandidatePeak* pPrevpeak = NULL ;
    for(int peakidx = 0; peakidx<FCandidPeaks.size(); ++peakidx)
    {
        TCandidatePeak& curCandidPeak = FCandidPeaks[peakidx] ;
        peakComputer.Integrate(curCandidPeak);

        if (curCandidPeak.isUserSlice) 
            curCandidPeak.RetentionTime = (curCandidPeak.StartIndex + curCandidPeak.EndIndex) * DeltaTime/2.0 ;
      
        totArea += fabs(curCandidPeak.Area);
        totHeight += fabs(curCandidPeak.Height);

        if ( ( (curCandidPeak.code.find("FS")>0) || (curCandidPeak.code.find("FB")>0) )
            && (curCandidPeak.Height > EPSILON_ZERO) )
            curCandidPeak.Resolution_Valleys = peakComputer.PrevValleyHeight / curCandidPeak.Height; // formula changed 26/10/99 by BO. Asked by NB.

        if (pPrevpeak != NULL)
        {
            if (fabs(pPrevpeak->RetentionTime - DeadTime) > EPSILON_ZERO) 
                curCandidPeak.Selectivity = (curCandidPeak.RetentionTime - DeadTime) / (pPrevpeak->RetentionTime - DeadTime);
            else 
                curCandidPeak.Selectivity = 0.0;

            if (curCandidPeak.Width_50p + pPrevpeak->Width_50p > EPSILON_ZERO)
                curCandidPeak.Resolution_HalfWidth = 1.18 * (curCandidPeak.RetentionTime - pPrevpeak->RetentionTime) / (curCandidPeak.Width_50p + pPrevpeak->Width_50p);
            else 
                curCandidPeak.Resolution_HalfWidth = 0.0;

            if (curCandidPeak.Width_TgBase + pPrevpeak->Width_TgBase > EPSILON_ZERO)
                curCandidPeak.Resolution_USP = 2.0 * (curCandidPeak.RetentionTime - pPrevpeak->RetentionTime) / (curCandidPeak.Width_TgBase + pPrevpeak->Width_TgBase);
            else 
                curCandidPeak.Resolution_USP = 0.0;

             if (fabs(DeadTime) > EPSILON_ZERO) 
                 curCandidPeak.Capacity = (curCandidPeak.RetentionTime - DeadTime) / DeadTime ;
             else 
                 curCandidPeak.Capacity = 0.0;
        }
        else
        {
            curCandidPeak.Selectivity = 0.0;
            curCandidPeak.Resolution_HalfWidth = 0.0;
            curCandidPeak.Resolution_USP = 0.0;
            curCandidPeak.Resolution_Valleys = 0.0;
        }
        if (fabs(DeadTime) > EPSILON_ZERO)
          curCandidPeak.Capacity = (curCandidPeak.RetentionTime - DeadTime) / DeadTime ;
        else 
          curCandidPeak.Capacity = 0.0;

        pPrevpeak = &curCandidPeak ;
    }
}

void TInternalIntegrator::RejectPeaks(const INT_FLOAT totArea, const INT_FLOAT totHeight)
{

    TRejectionState rejectState;
    rejectState.SetTotAmounts(totArea, totHeight);

    for(int peakidx = 0; peakidx<FCandidPeaks.size(); ++peakidx)
    {
        TCandidatePeak& curCandidPeak = FCandidPeaks[peakidx] ;
        rejectState.UpdateFromEvents( curCandidPeak.StartIndex * DeltaTime,
                                      FRejectionEvents);
        if (!rejectState.KeepThePeak(curCandidPeak))
        {
            PropagatePeakDeletion(curCandidPeak, false);
            FCandidPeaks.erase(peakidx);
            --peakidx;
        }
        //CAUTION: MyIndexInTheWholeList not valid after this point;
        //call PropagatePeakDeletion with TRUE if validity is still needed
    }
}

void TInternalIntegrator::SetupPeaksCode()
{
    for(int peakidx = 0; peakidx<FCandidPeaks.size(); ++peakidx)
    {
        TCandidatePeak& curCandidPeak = FCandidPeaks[peakidx] ;

        curCandidPeak.SetDefaultCodes();
        curCandidPeak.StartTime = curCandidPeak.StartIndex * DeltaTime ;
        curCandidPeak.EndTime   = curCandidPeak.EndIndex   * DeltaTime ;

        if ( (curCandidPeak.FFusedStart || curCandidPeak.FStartIsValley)
            &&
            (curCandidPeak.FFusedEnd   || curCandidPeak.FEndIsValley) )
        {
            curCandidPeak.code = string("FB") ;
        }
        else
            if (curCandidPeak.FFusedStart || curCandidPeak.FStartIsValley)
        {
            curCandidPeak.code = string("FS");
        }
        else
            if (curCandidPeak.FFusedEnd || curCandidPeak.FEndIsValley)
        {
            curCandidPeak.code = string("FE");
        }
        curCandidPeak.code =
            curCandidPeak.code
            + string(1,INTEGRATION_CODE_DELIMITER)
            + string(1,curCandidPeak.FStartCode)
            + string(1,curCandidPeak.FEndCode) ;

    }
}

void TInternalIntegrator::CorrectForSkimming()
{
    for(int peakidx = 0; peakidx<FCandidPeaks.size(); ++peakidx)
    {
        TCandidatePeak& curCandidPeak = FCandidPeaks[peakidx] ;
        if (curCandidPeak.FisMother)
        {
            assert(curCandidPeak.pFDaughters!=NULL);
            for( TCandidatePeakList::iterator daughter_it = curCandidPeak.pFDaughters->begin();
            daughter_it != curCandidPeak.pFDaughters->end();
            daughter_it++)
            {
                TCandidatePeak& curDaughter = *(*daughter_it) ;
                curCandidPeak.Area -= curDaughter.Area;
                curDaughter.MotherIndex = peakidx;
            }
            // The following quantities are not
            // defined for skimmed peaks.
            curCandidPeak.Moment1  = 0;
            curCandidPeak.Moment2  = 0;
            curCandidPeak.Moment3  = 0;
            curCandidPeak.Moment4  = 0;
            curCandidPeak.Skew     = 0;
            curCandidPeak.Kurtosis = 0;
        }
    }
    // IMPORTANT: no peak should be deleted from now, since FMotherIndex is the
    // index of a peak in the final list
}


void TInternalIntegrator::RejectLinesWithNoPeak()
{
    for (int lineidx=0; lineidx<FCandidLines.size(); lineidx++)
    {
        TCandidateBaseLine& curline = FCandidLines[lineidx] ;
        if (curline.FAssociatedPeaks.size()==0)
        {
            FCandidLines.erase(lineidx) ;
            --lineidx;
        }
    }
}

//------------------------------------------------------

void TInternalIntegrator::Local_ComputeLimits(int idxStartPeak, int& leftLimit, int& rightLimit)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TCandidatePeak& theCandidPeak = FCandidPeaks[idxStartPeak] ;
    if (theCandidPeak.FForcedStart)
    {
        leftLimit = theCandidPeak.StartIndex ;
    }
    else
        {
        if (idxStartPeak==0)
            leftLimit=0;
        else
            leftLimit=FCandidPeaks[idxStartPeak-1].EndIndex ;
    }  

    if (theCandidPeak.FForcedEnd)
    {
        rightLimit = theCandidPeak.EndIndex;
    }
    else
        {
        if (idxStartPeak == FCandidPeaks.size()-1)
            rightLimit = SignalSize-1;
        else
            rightLimit = FCandidPeaks[idxStartPeak + 1].StartIndex;
    }
}

void TInternalIntegrator::CreateDefaultBaseLines()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TCandidateBaseLine* pNewLine ;
    int leftLimit, rightLimit;
    int idxStartPeak;

    for(int idxpeak=0; idxpeak<FCandidPeaks.size(); ++idxpeak)
    {
        TCandidatePeak& CurPeak = FCandidPeaks[idxpeak]; ;

#ifdef DEBUG
        dbgstream() << "Computing peak s:"
        << CurPeak.StartIndex*DeltaTime
        << "e:"
        << CurPeak.EndIndex*DeltaTime
        << endl ;
#endif
        if (!(CurPeak.FHasEventBaseLine))
        {
            if ( !(CurPeak.FFusedStart || CurPeak.FStartIsValley) &&
                !(CurPeak.FFusedEnd   || CurPeak.FEndIsValley) )
            {
                pNewLine = &(FCandidLines.push_back());
                pNewLine->FStartIndex = CurPeak.StartIndex ;
                pNewLine->FEndIndex = CurPeak.EndIndex ;
                pNewLine->FAssociatedPeaks.push_back(&CurPeak);
                idxStartPeak = idxpeak;
                Local_ComputeLimits(idxStartPeak,leftLimit,rightLimit);
#ifdef DEBUG
                dbgstream() << "New line : no baseline event" << endl;
#endif
                AddCandidateBaseLine(*pNewLine, leftLimit, rightLimit, 0);
//TRANSLATION : To avoid object copies and resources wasting, I instanciates
// the new line directly in the list in which it will eventually be stored.
// It is mandatory to call AddCandidateBaseLine, because it could split the line
// into smaller ones. GO.
            }
            else
                {
                if ( !(CurPeak.FFusedStart || CurPeak.FStartIsValley))
                {
                    pNewLine = &(FCandidLines.push_back());
                    pNewLine->FStartIndex = CurPeak.StartIndex ;
                    pNewLine->FEndIndex = 0 ;
                    pNewLine->FAssociatedPeaks.push_back(&CurPeak);
                    idxStartPeak = idxpeak;
#ifdef DEBUG
                    dbgstream() << "New line : baseline event" << endl;
#endif
                }
                else
                    {
                    if (!(CurPeak.FFusedEnd || CurPeak.FEndIsValley))
                    {
                        assert(pNewLine!=NULL);
                        pNewLine->FEndIndex = CurPeak.EndIndex;
                        pNewLine->FAssociatedPeaks.push_back(&CurPeak);
                        Local_ComputeLimits(idxStartPeak,leftLimit, rightLimit);
                        AddCandidateBaseLine(*pNewLine, leftLimit, rightLimit, 0);

//TRANSLATION : To avoid object copies and resources wasting, I instanciates
// the new line directly in the list in which it will eventually be stored.
// It is mandatory to call AddCandidateBaseLine, because it could split the line
// into smaller ones. GO.

#ifdef DEBUG
                        dbgstream() << "Updating current line / peak belongs to it" << endl ;
#endif

                    }
                    else
                        {
                        assert(pNewLine != NULL);
                        pNewLine->FAssociatedPeaks.push_back(&CurPeak);
#ifdef DEBUG
                        dbgstream() << "Current peak belongs to current line" << endl ;
#endif

                    }
                }
            }
        }
    }

#ifdef DEBUG
    dbgstream() << "After CreateDefaultBaselines : "
    << FCandidLines.size()
    << " candidate lines"
    << endl ;

    for (int lineidx=0; lineidx<FCandidLines.size(); ++lineidx)
    {
        TCandidateBaseLine& curline = FCandidLines[lineidx] ;
        dbgstream() << "#" << lineidx << " "
        << "s:" << curline.FStartIndex * DeltaTime << " "
        << "e:" << curline.FEndIndex * DeltaTime
        << endl ;
    }
#endif

}

//------------------------------------------------------

void TInternalIntegrator::CreateEventBaselines(TTimeAndValue& SkimRatios)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FCandidLines.clear(); //this calls object destructors

// events will be associated to their peaks and then processed
// (NB: some events will not be associated to a peak and must be processed
// separately, ex: Manual Baseline)

// IMPORTANT: ADDPEAK events must be considered first, since they may delete
// existing peaks: they influence other associations
// ADDPEAK could have been handled during peak detection but we have chosen
// this other way in order to avoid complexifying the peak detection algorithm.

    unsigned int idxEvent ;
// processing addpeaks events
    for (idxEvent=0;
    idxEvent < FAddPeaksEvents.size();
    idxEvent++)
//not using the iterator here because of the original delphi code.
//the following function calls need idxEvent. GO 31.05.02
    {
        TEventAndPeaks* assoc = AssociateEventsAndPeaks(idxEvent, FAddPeaksEvents);
        if (assoc!=NULL)
        {
#ifdef DEBUG
            dbgstream() << "Now processing ADD PEAKS "
            << assoc->pFEvent->Name() << "  "
//<< assoc.pFEvent->DescribeArgs
            << endl ;
#endif
            if (assoc->FPeaks.size() > 0) ProcessEventAndPeaks(*assoc, SkimRatios);
            delete assoc;
        }
    }

// processing other events

    TEventAndPeaksList assocList;
// was a owning list in the pascal code. translated in c++ by a list of pointers
// because elements are instanciated by a function. Like that, we are avoiding some
// unecessary calls to the copy constructor.
// The elements of this list should be destroyed "by hand" at the end of this
// function.   GO 31.05.02 (during C++ translation)

    for (idxEvent=0;
    idxEvent < FBaseLineEvents.size();
    idxEvent++)
//not using the iterator here because of the original delphi code.
//the following function calls need idxEvent. GO 31.05.02
    {
        TEventAndPeaks* assoc = AssociateEventsAndPeaks(idxEvent, FBaseLineEvents);
        if (assoc!=NULL)
        {
            assocList.push_back(assoc);
#ifdef DEBUG
            dbgstream() << "Queuing event "
            << assoc->pFEvent->Name() << endl ;
//with assoc.fevent do OptionalDebug(format('queuing event %s %s', [Fname, DescribeArgs]));
#endif
        }
    }

if (assocList.size() > 1)
        sort(assocList.begin(),assocList.end(),SortAssocsByPriority);

    TEventAndPeaksList::iterator assoc_it ;
    for(assoc_it = assocList.begin();
    assoc_it != assocList.end();
    assoc_it++)
    {
        TEventAndPeaks* assoc = *assoc_it;
#ifdef DEBUG
        dbgstream() << "Processing event "
        << assoc->pFEvent->Name() << endl ;
#endif
        if ((assoc->FPeaks.size() > 0) && FComputeBaseLines) //INUS_TEST. to be improved later so that we can process some events.
            ProcessEventAndPeaks(*assoc, SkimRatios);
    }

//See the comment at the assocList initialization.
    for(assoc_it = assocList.begin();
    assoc_it != assocList.end();
    assoc_it++)
    {
        delete *assoc_it ;
    }
}

TEventAndPeaks* TInternalIntegrator::AssociateEventsAndPeaks(int idxEvent,
    TIntegrationEventList& EventList)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
//assert(event!=NULL);

    TEventAndPeaks* assoc = NULL;
    TIntegrationEvent& event = *(EventList[idxEvent]) ;
    if ( (event.Code() >= INTEGRATIONEVENT_HORIZLINE) &&
        (event.Code() <= INTEGRATIONEVENT_VALTOVAL) ||
        (event.Code() == INTEGRATIONEVENT_ADDPEAKS) )
    { 
// standard On/Off event - or skimming event
        TIntegrationEvent_OnOff& event_onoff = (TIntegrationEvent_OnOff&) event;
        if (event_onoff.On())  //TIntegrationEvent& event = *EventList[idxEvent];

            {
// 1- creation of assoc depends on the event
// test skimming
            if ( (event.Code() >= INTEGRATIONEVENT_TANGENTSKIM) &&
                (event.Code() <= INTEGRATIONEVENT_VALTOVAL) )
            {
                assoc = new TEventAndPeaks_Skimming(idxEvent, event_onoff, DeltaTime);
            }
// then test common line
            else
                {
                if ( (event.Code() == INTEGRATIONEVENT_COMMONLINE_BYPEAK) ||
                    (event.Code() == INTEGRATIONEVENT_COMMONLINE) )
                {
                    assoc = new TEventAndPeaks_CommonLine(idxEvent, event_onoff, DeltaTime);
                }
// else standard case
                else
                    {
                    assoc = new TEventOnOffAndPeaks(idxEvent, event_onoff, DeltaTime);
                }
            }

// 2- now call a virtual method
            assoc->GetAssociatedPeaks(EventList, FCandidPeaks);
        }
    }
    else
        {
        if ( (event.Code() >= INTEGRATIONEVENT_BASELINE_NOW) &&
            (event.Code() <= INTEGRATIONEVENT_BASELINE_NEXTVALLEY) )
        {
// standard TEvent_Now processing
            TIntegrationEvent_Now& Event_now = (TIntegrationEvent_Now&) event;
            assoc = new TEventNowAndPeaks(Event_now, DeltaTime);
            assoc->GetAssociatedPeaks(EventList, FCandidPeaks);
        }
    }
    return assoc;
}

void TInternalIntegrator::ProcessEventAndPeaks(TEventAndPeaks& assoc,
    TTimeAndValue& SkimRatios)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(&assoc!=NULL);

    switch (assoc.pFEvent->Code())
    {
        case INTEGRATIONEVENT_HORIZLINE :
            { 
                ProcessHorizontalBaseLine(assoc, true, false);
                break;
            }
        case INTEGRATIONEVENT_HORIZLINE_BACK :
            {
                ProcessHorizontalBaseLine(assoc, false, false);
                break;
            }
        case INTEGRATIONEVENT_HORIZLINE_BYPEAK :
            {
                ProcessHorizontalBaseLine(assoc, true, true);
                break;
            }
        case INTEGRATIONEVENT_HORIZLINE_BACK_BYPEAK :
            {
                ProcessHorizontalBaseLine(assoc, false, true);
                break;
            }
        case INTEGRATIONEVENT_COMMONLINE :
            {
                TEventAndPeaks_CommonLine* assoc_cl = (TEventAndPeaks_CommonLine*) &assoc ;
                ProcessCommonBaseLine(assoc, assoc_cl->FStartIsByPeak, assoc_cl->FEndIsByPeak);
                break;
            }

        case INTEGRATIONEVENT_COMMONLINE_BYPEAK :
            {
                TEventAndPeaks_CommonLine* assoc_cl = (TEventAndPeaks_CommonLine*) &assoc ;
                ProcessCommonBaseLine(assoc, assoc_cl->FStartIsByPeak, assoc_cl->FEndIsByPeak);
                break;
            }

        case INTEGRATIONEVENT_BASELINE_NOW :
            {
                ProcessBaseLineNow(assoc);
                break;
            }

        case INTEGRATIONEVENT_BASELINE_NEXTVALLEY :
            {
                ProcessCommonBaseLine(assoc, true, true);
                break;
            }

        case INTEGRATIONEVENT_ADDPEAKS :
            {
                ProcessAddPeaks(assoc);
                break;
            }

        case INTEGRATIONEVENT_VALTOVAL :
            {
                ProcessValleyToValley(assoc);
                break;
            }

        case INTEGRATIONEVENT_TANGENTSKIM :
            {
                ProcessSkimming(assoc, BASELINE_STRAIGHT);
                break;
            }

        case INTEGRATIONEVENT_TANGENTSKIM_FRONT :
            {
                ProcessSkimming_FrontOrRear(assoc, false, SkimRatios, BASELINE_STRAIGHT);
                break;
            }

        case INTEGRATIONEVENT_TANGENTSKIM_REAR :
            {
                ProcessSkimming_FrontOrRear(assoc, true, SkimRatios, BASELINE_STRAIGHT);
                break;
            }

        case INTEGRATIONEVENT_TANGENTSKIM_EXP :
            {
                ProcessSkimming(assoc, BASELINE_EXPONENTIAL);
                break;
            }

        case INTEGRATIONEVENT_TANGENTSKIM_FRONT_EXP :
            {
                ProcessSkimming_FrontOrRear(assoc, false, SkimRatios, BASELINE_EXPONENTIAL);
                break;
            }

        case INTEGRATIONEVENT_TANGENTSKIM_REAR_EXP :
            {
                ProcessSkimming_FrontOrRear(assoc, true, SkimRatios, BASELINE_EXPONENTIAL);
                break;
            }
    }
}

void TInternalIntegrator::ProcessHorizontalBaseLine(TEventAndPeaks& assoc,
    bool goForward,
    bool byPeak)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// for each peak, create a new horiz line at the level determined by event start
// we do not try to create a single line because we want to avoid signal/line intersection

    int StartMeasureIndex ;
    int EndMeasureIndex ;

    if (byPeak)
    {
        StartMeasureIndex = assoc.FPeaks.front()->StartIndex;
        EndMeasureIndex   = assoc.FPeaks.back()->EndIndex;
    }
    else
        {
// use AdjustIndextoSignal since the limits
// are provided by the user and may exceed the signal range
        StartMeasureIndex = AdjustIndexToSignal(round(assoc.FStartTime / DeltaTime));
        EndMeasureIndex   = AdjustIndexToSignal(round(assoc.FEndTime / DeltaTime));
    }

    for(TCandidatePeakList::iterator curpeak_it = assoc.FPeaks.begin() ;
    curpeak_it != assoc.FPeaks.end();
    curpeak_it++)
    {
        TCandidatePeak& curPeak = **curpeak_it ;
        if ( (!curPeak.FHasEventBaseLine) && (!curPeak.FisDaughter))
        {
//creates a new element in the list
            TCandidateBaseLine& newLine = FCandidLines.push_back();
            newLine.FStartIndex = curPeak.StartIndex ;
            newLine.FEndIndex = curPeak.EndIndex ;
            newLine.FisHorizontalFromStart = goForward;
            newLine.FisHorizontalFromEnd = !goForward;

            if (goForward)
                newLine.FMeasureStartAtIndex = StartMeasureIndex;
            else
                newLine.FMeasureEndAtIndex   = EndMeasureIndex;

            curPeak.FHasEventBaseLine = true;
            curPeak.FEventLineIsHorizontal = true; // thus skimming can apply
            newLine.FAssociatedPeaks.push_back(&curPeak);
            newLine.FisEventLine = true;

// if the first or last peak was inside a group, the group is broken - see DT4882
            if ( (curPeak.myIndexInTheWholeList > 0) &&
                (curpeak_it == assoc.FPeaks.begin()) )
            {
                TCandidatePeak& predPeak = FCandidPeaks[curPeak.myIndexInTheWholeList-1];
                predPeak.FFusedEnd = false;
                predPeak.FEndIsValley = false;
                curPeak.FFusedStart = false;
                curPeak.FStartIsValley = false;
            }

            if ( (curPeak.myIndexInTheWholeList < (FCandidPeaks.size()-1)) &&
                (curpeak_it == assoc.FPeaks.end()-1) )
            {
                TCandidatePeak& succPeak = FCandidPeaks[curPeak.myIndexInTheWholeList+1];
                curPeak.FFusedEnd = false;
                curPeak.FEndIsValley = false;
                succPeak.FFusedStart = false;
                succPeak.FStartIsValley = false;
            }

            curPeak.SetHorizontalCodes();
        }
    }
}

void TInternalIntegrator::ProcessCommonBaseLine(TEventAndPeaks& assoc,
    bool StartIsByPeak,
    bool EndIsByPeak)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(assoc.FPeaks.size() > 0);

// only group of peaks that are completely free (of any other baseline)
// will be processed
    bool OtherBaseline = false;
    unsigned int idxPeak = 0;
    while ( (!OtherBaseline) && (idxPeak < assoc.FPeaks.size()))
    {
// daughter or mother peaks will cancel the operation
// (in order to simplify for the moment)
        OtherBaseline = (assoc.FPeaks[idxPeak]->FHasEventBaseLine ||
            assoc.FPeaks[idxPeak]->FisDaughter ||
            assoc.FPeaks[idxPeak]->FisMother);
        idxPeak++;
    }

    if (!OtherBaseline)
    {
        TCandidatePeak& firstPeak = *(assoc.FPeaks.front()) ;
        TCandidatePeak& lastPeak = *(assoc.FPeaks.back()) ;

// first/last peak can not belong to other groups
        firstPeak.FStartIsValley = false;
        firstPeak.FFusedStart = false;
        lastPeak.FEndIsValley = false;
        lastPeak.FFusedEnd = false;
        PropagateValleyChanges(firstPeak, lastPeak);

// use AdjustIndextoSignal since the limits
// are provided by the user and may exceed the signal range
        int startIndex;
        if (StartIsByPeak)
            startIndex = firstPeak.StartIndex;
        else
            startIndex = AdjustIndexToSignal(round(assoc.FStartTime / DeltaTime));

        int endIndex;
        if (EndIsByPeak)
            endIndex = lastPeak.EndIndex ;
        else
            endIndex = AdjustIndexToSignal(round(assoc.FEndTime / DeltaTime));

        ;
        TCandidateBaseLine* pNewLine = &(FCandidLines.push_back());
        pNewLine->FStartIndex = startIndex ;
        pNewLine->FEndIndex = endIndex ;

// correction of the peak preceding the first peak
// if the baseline overrides it (in order to avoid an intersection of two baselines)
// for the moment we do not change the beginning of firstpeak (to be corrected later
// if necessary)
        if ( (!StartIsByPeak) && (firstPeak.myIndexInTheWholeList > 0) )
        {
            TCandidatePeak& curPeak = FCandidPeaks[firstPeak.myIndexInTheWholeList - 1];
            if ( (!curPeak.FForcedEnd) &&
                (curPeak.EndIndex > pNewLine->FStartIndex) )
            {
                curPeak.EndIndex = pNewLine->FStartIndex;
            }
        }

        for(TCandidatePeakList::iterator peak_it = assoc.FPeaks.begin();
        peak_it!=assoc.FPeaks.end();
        peak_it++)
        {
            TCandidatePeak& curPeak = **peak_it;
            pNewLine->FAssociatedPeaks.push_back(&curPeak);
            curPeak.FHasEventBaseLine = true;
        }

// now fusion the peaks (cf PowerTrack issue #296)
        if (assoc.FPeaks.size() > 1)
            for(TCandidatePeakList::iterator peak_it = assoc.FPeaks.begin();
        peak_it!=assoc.FPeaks.end()-1;
        peak_it++)
        {
            TCandidatePeak& curPeak  = **peak_it;
            TCandidatePeak& nextPeak = **(peak_it+1);
// fusion peaks only if they have the same sign
            if (curPeak.FisPositive == nextPeak.FisPositive)
            {
                curPeak.FEndIsValley = true;
                nextPeak.FStartIsValley = true;
// now finds the valley
                int valleyPosition = FindLocalExtrema(curPeak.EndIndex, nextPeak.StartIndex, curPeak.FisPositive);
                curPeak.EndIndex = valleyPosition;
                nextPeak.StartIndex = valleyPosition;
            }
// else ... what we should do is not clear, thus we do nothing, waiting for specifications...
        }

        AddCandidateBaseLine(*pNewLine, pNewLine->FStartIndex, pNewLine->FEndIndex, 0);

    }
}

void TInternalIntegrator::ProcessAddPeaks(TEventAndPeaks& assoc)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(assoc.FPeaks.size() > 0);

// only group of peaks that are completely free (of any other baseline)
// will be processed
    bool OtherBaseline = false;
    unsigned int idxPeak = 0;
    while ( (!OtherBaseline) && (idxPeak < assoc.FPeaks.size()) )
    {
// daughter or mother peaks will cancel the operation
// (in order to simplify for the moment)
        OtherBaseline = ( assoc.FPeaks[idxPeak]->FHasEventBaseLine ||
            assoc.FPeaks[idxPeak]->FisDaughter ||
            assoc.FPeaks[idxPeak]->FisMother );
        idxPeak++;
    }

// otherBaseline should always be false since AddPeaks has the highest priority
    assert(!OtherBaseline);
//TRANSLATION : is the previous loop (while) necessary since we know
//              that OtherBaseline is false ? I do not understand the
//              meaning of this code... GO.

    if (!OtherBaseline) //always true (see the previous assertion)!!
    {
        TCandidatePeak& firstPeak = *(assoc.FPeaks.front());
        TCandidatePeak& lastPeak  = *(assoc.FPeaks.back());

// first/last peak can not belong to other groups
        firstPeak.FStartIsValley = false;
        firstPeak.FFusedStart = false;
        lastPeak.FEndIsValley = false;
        lastPeak.FFusedEnd = false;
        PropagateValleyChanges(firstPeak, lastPeak);

        TCandidatePeak& curPeak = firstPeak;
// instead of creating a totally new peak we extend firt peak
// because it is already inserted at the right place in FCandidPeaks

        curPeak.EndIndex = lastPeak.EndIndex;
        curPeak.FHasEventBaseLine = false; // since future events may define a baseline

// delete the component peaks (except the first of them)
// not very optimized (because PropagePeakDeletion will loop several times over the peaks), to improve later
        for(TCandidatePeakList::iterator peak_it = assoc.FPeaks.begin();
        peak_it!=assoc.FPeaks.end();
        peak_it++)
        {
            TCandidatePeak& curPeak = **peak_it;
            PropagatePeakDeletion(curPeak, true);
            FCandidPeaks.erase(FCandidPeaks[curPeak.myIndexInTheWholeList]);
//FCandidPeaks owns the peak, the constructor is being called.
        }
    }
}

void TInternalIntegrator::ProcessBaseLineNow(TEventAndPeaks& assoc)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(assoc.FPeaks.size() == 1);

    TCandidatePeak& curPeak = *(assoc.FPeaks.front());

    int newLine_StartIndex = AdjustIndexToSignal(round(assoc.FStartTime / DeltaTime));

    if (curPeak.ApexIndex > 0)
    {
        if ( (curPeak.ApexIndex <= newLine_StartIndex) &&
            (!curPeak.FForcedEnd) )
        {
            curPeak.EndIndex = newLine_StartIndex;
            curPeak.FEndIsValley = false;
            curPeak.FFusedEnd = false;
            PropagateValleyChanges(curPeak, curPeak);
        }
        else
            {
            if ( (curPeak.ApexIndex > newLine_StartIndex) &&
                (!curPeak.FForcedStart) )
            {
                curPeak.StartIndex = newLine_StartIndex;
                curPeak.FStartIsValley = false;
                curPeak.FFusedStart = false;
                PropagateValleyChanges(curPeak, curPeak);
            }
        }
    }
}

void TInternalIntegrator::ProcessValleyToValley(TEventAndPeaks& assoc)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// if the first peak was inside a group, the group is broken
// if the last  peak was inside a group, the group is broken
    if (assoc.FPeaks.size() > 0)
    {
        TCandidatePeak& firstPeak = *(assoc.FPeaks.front()) ;
        if ( (firstPeak.myIndexInTheWholeList > 0) &&
            (!firstPeak.FHasEventBaseLine) &&
            (!firstPeak.FisDaughter) )
        {
            TCandidatePeak& predPeak = FCandidPeaks[firstPeak.myIndexInTheWholeList-1];
            predPeak.FFusedEnd = false;
            predPeak.FEndIsValley = false;
            firstPeak.FFusedStart = false;
            firstPeak.FStartIsValley = false;
        }

        TCandidatePeak& lastPeak = *(assoc.FPeaks.back());
        if ( (lastPeak.myIndexInTheWholeList < (FCandidPeaks.size()-1) ) &&
            (!lastPeak.FHasEventBaseLine) &&
            (!lastPeak.FisDaughter) )
        {
            TCandidatePeak& succPeak = FCandidPeaks[lastPeak.myIndexInTheWholeList+1];
            lastPeak.FFusedEnd = false;
            lastPeak.FEndIsValley = false;
            succPeak.FFusedStart = false;
            succPeak.FStartIsValley = false;
        }
    }

// for each peak draw a line from its start to its end
    for(TCandidatePeakList::iterator peak_it = assoc.FPeaks.begin();
    peak_it != assoc.FPeaks.end();
    peak_it++)
    {
        TCandidatePeak& curPeak = **peak_it ;
        if ( (!curPeak.FHasEventBaseLine) &&
            (!curPeak.FisDaughter) )
        {
            TCandidateBaseLine* pNewLine = &(FCandidLines.push_back());
            pNewLine->FStartIndex = curPeak.StartIndex ;
            pNewLine->FEndIndex = curPeak.EndIndex;
//allocating directly in the list. Do not forget to call addCandidateBaseLine
//because it may be needed to split the line.

            curPeak.FHasEventBaseLine = true;
            pNewLine->FAssociatedPeaks.push_back(&curPeak);

// améliorer ligne
            //assert(&curPeak == &(FCandidPeaks[curPeak.myIndexInTheWholeList]) );

            int limitleft ;
            if (curPeak.FForcedStart)
                limitleft = curPeak.StartIndex ;
            else
                if (curPeak.myIndexInTheWholeList == 0)
                    limitleft = 0 ;
            else
                limitleft = FCandidPeaks[curPeak.myIndexInTheWholeList - 1].EndIndex;

            int limitright ;
            if (curPeak.FForcedEnd)
                limitright = curPeak.EndIndex;
            else
                if ( curPeak.myIndexInTheWholeList == (FCandidPeaks.size() - 1) )
                    limitright = SignalSize ;
            else
                limitright = FCandidPeaks[curPeak.myIndexInTheWholeList + 1].StartIndex;

            AddCandidateBaseLine(*pNewLine, limitleft, limitright, 0);
        }
    }
}

void TInternalIntegrator::ProcessSkimming(TEventAndPeaks& assoc,
    TBaseLineType linetype)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : default behavior: the mother peak is the first peak in the fused group
             and no ratio check is performed
 * History : 15/05/00 BO Fixed DT 1703 integration codes
 * -------------------------- */
{
#ifdef DEBUG
    dbgstream() << "Processing skimming for assoc (s:"
    << assoc.FStartTime
    << " e:"
    << assoc.FEndTime
    << ")"
    << endl;
#endif
// for each group of peaks, marks the mother peak and its daughters
    TCandidatePeakList* pCurGroup = NULL;
    for(TCandidatePeakList::iterator peak_it = assoc.FPeaks.begin();
    peak_it != assoc.FPeaks.end();
    peak_it++)
    {
        TCandidatePeak& curPeak = **peak_it ;

#ifdef DEBUG
        dbgstream() << " - current peak s:"
        << curPeak.StartIndex * DeltaTime
        << " e:"
        << curPeak.EndIndex * DeltaTime
        << " " ;
#endif

        if ( (curPeak.FFusedEnd || curPeak.FEndIsValley) &&
            (!(curPeak.FFusedStart || curPeak.FStartIsValley)) )
        {
#ifdef DEBUG
            dbgstream() << " is mother";
#endif
            delete pCurGroup;
            pCurGroup = new TCandidatePeakList;
            pCurGroup->push_back(&curPeak);
        }
        else
            if ( ((curPeak.FFusedEnd || curPeak.FEndIsValley) &&
            (curPeak.FFusedStart || curPeak.FStartIsValley))
            && (pCurGroup!=NULL) ) // incomplete groups will be ignored
            {
#ifdef DEBUG
                dbgstream() << " included to the current group";
#endif
                pCurGroup->push_back(&curPeak);
            }
            else
                if ( (curPeak.FFusedStart || curPeak.FStartIsValley)
                && (pCurGroup!=NULL) ) // incomplete groups will be ignored
                { // process the group
#ifdef DEBUG
                    dbgstream() << " included to the current group, "
                                << "first is mother ";
#endif
                    pCurGroup->push_back(&curPeak);
// in this simplified case, first peak is the mother peak
                    TCandidatePeak& firstPeak = *(pCurGroup->front());
                    firstPeak.FisMother = true;
                    firstPeak.FFusedEnd = false;
                    firstPeak.FEndIsValley = false;
                    TCandidatePeak& MotherPeak = firstPeak;
// next peak is no more fused
                    (*(pCurGroup->begin()+1))->FFusedStart = false;
                    (*(pCurGroup->begin()+1))->FStartIsValley = false;

                    MotherPeak.pFDaughters = new TCandidatePeakList;

// next and following peaks are daughter peaks
                    for (TCandidatePeakList::iterator group_it = pCurGroup->begin()+1;
                    group_it!=pCurGroup->end();
                    group_it++)
                    {
                        TCandidatePeak& curitem = **group_it ;
                        curitem.FisDaughter = true;
                        curitem.FmyLineType = linetype;
                        curitem.SetSkimmingCodes(linetype);
                        MotherPeak.pFDaughters->push_back(&curitem);
                    }
// extend mother peak
                    MotherPeak.EndIndex = pCurGroup->back()->EndIndex;
                }
#ifdef DEBUG
                dbgstream() << endl;
#endif
    }
//TRANSLATION : It seems that there is a potential memory leak
//              Who is owning curGroup ? GO.
}

void TInternalIntegrator::ProcessSkimming_FrontOrRear(TEventAndPeaks& assoc,
    bool rear,
    TTimeAndValue& SkimRatios,
    TBaseLineType linetype)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// builds and processes all peak groups in the assoc list
// a group end is determined by a peak whose end is not fused nor a valley
// or whose baseline number is different from the line number of the group
    TPeaksToSkim curGroup(rear, SignalSmooth, DeltaTime, linetype, this);

    int increment ;
    int idxNoMorePeaks;
    int idxPeak;
    if (rear)
    {
        increment = 1;
        idxNoMorePeaks = assoc.FPeaks.size();
        idxPeak = 0;
    }
    else
        {
        increment = -1;
        idxNoMorePeaks = -1;
        idxPeak = assoc.FPeaks.size()-1;
    }

    while (idxPeak != idxNoMorePeaks)
    {
        TCandidatePeak& curPeak = *(assoc.FPeaks[idxPeak]);
        curGroup.Add(curPeak);

        TCandidatePeak* pNextPeak ;
        if ( (idxPeak+increment)!=idxNoMorePeaks )
            pNextPeak = assoc.FPeaks[idxPeak + increment] ;
        else
            pNextPeak = NULL;

        if (pNextPeak!=NULL)
            if ( (rear && !(curPeak.EndIndex == pNextPeak->StartIndex))
                ||
                (!rear && !(curPeak.StartIndex == pNextPeak->EndIndex)) )
        {
            curGroup.Process(SkimRatios);
            curGroup.Clear();
        }
        idxPeak += increment;
    }

    curGroup.Process(SkimRatios);

}

void TInternalIntegrator::CleanCandidateLines()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// clean the lines that have been modified due to skimming
// = break the link between such a line and the daughter peaks
// and then remove lines with no peaks
    INT_INDEXLIST linesToRemove ;
    for (int lineidx=0; lineidx<FCandidLines.size(); ++lineidx)
    {
        TCandidateBaseLine& curline = FCandidLines[lineidx] ;

//do with FCandidLines.ItemIs(idxLines) do
//begin
        vector<TCandidatePeakList::iterator> peaksToRemove ;
        for(TCandidatePeakList::iterator peak_it = curline.FAssociatedPeaks.begin();
        peak_it!=curline.FAssociatedPeaks.end();
        peak_it++)
        {
            TCandidatePeak& curPeak = **peak_it;

            if (curPeak.FisDaughter)
                peaksToRemove.push_back(peak_it);
//FAssociatedPeaks.Items[idxPeaks] := nil ; // this peak does not belong anymore to the line
            else
                if (curPeak.FisMother && curPeak.FHasEventBaseLine)
            {
// line must be corrected due to curPeak extension
                if ((curline.FisHorizontalFromStart || curline.FisHorizontalFromEnd))
                    if (curline.FStartIndex > curPeak.StartIndex)
                        curline.FStartIndex = curPeak.StartIndex ;
                else
                    if (curline.FEndIndex < curPeak.EndIndex)
                        curline.FEndIndex = curPeak.EndIndex;
            }

        }
//now removing peaks
        for (vector<TCandidatePeakList::iterator>::reverse_iterator toremove = peaksToRemove.rbegin();
        toremove!=peaksToRemove.rend();
        toremove ++)
        {
            curline.FAssociatedPeaks.erase(*toremove);
        }

        if (curline.FAssociatedPeaks.size() == 0)
            linesToRemove.push_back(lineidx);
    }
//now removing marked lines
//By construction, the linesToRemove list is sorted by increasing order.
//Deleting elements in the reverse order to avoid tricky things with
//index computations.
    for(INT_INDEXLIST::reverse_iterator toremove = linesToRemove.rbegin();
    toremove!=linesToRemove.rend();
    ++toremove)
    {
        FCandidLines.erase(*toremove);
    }
}

void TInternalIntegrator::CorrectSkimming()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// here we are looking for daughter peaks that have extended outside their mother peaks,
// and for mother peaks that have been shrinked
    for (int idxpeak=0; idxpeak<FCandidPeaks.size(); ++idxpeak)
    {
        TCandidatePeak& curPeak = FCandidPeaks[idxpeak] ;
        if (curPeak.FisMother)
        {
            vector<TCandidatePeakList::iterator> daughtersToRemove ;
            for(TCandidatePeakList::iterator daughter_it = curPeak.pFDaughters->begin();
            daughter_it!=curPeak.pFDaughters->end();
            daughter_it++)
            {
                TCandidatePeak& curDaughter = **daughter_it ;
                if  ( !((curDaughter.StartIndex >= curPeak.StartIndex) &&
                    (curDaughter.EndIndex <= curPeak.EndIndex)) )
                {
// we have a problem ! daughter is not inside its mother...
// 1- store daughter iterator for removing
                    daughtersToRemove.push_back(daughter_it);
// 2- correct Daughter
                    curDaughter.FisDaughter = false;
// 3- daughter can no more have an exponential line (DT 3924)
                    curDaughter.FmyLineType = BASELINE_STRAIGHT;
                }
// peut etre un peu brutal... a voir
/*
		curDaughter.FStartIsValley := false ;
		curDaughter.FEndIsValley := false ;
		curDaughter.FFusedStart := false ;
		curDaughter.FFusedEnd   := false ;
		PropagateValleyChanges(curDaughter,curDaughter) ;
	      */
            }
//removing stored daughters
            for(vector<TCandidatePeakList::iterator>::reverse_iterator toremove = daughtersToRemove.rbegin();
            toremove!=daughtersToRemove.rend();
            toremove++)
            {
                curPeak.pFDaughters->erase(*toremove);
            }

// check whether we have removed all daughters
            curPeak.FisMother = (curPeak.pFDaughters->size() > 0);
            if (curPeak.FisMother)
            {
                TCandidatePeak& tmpPeak = *(curPeak.pFDaughters->front()) ;
                bool WasFrontSkim = (curPeak.ApexIndex > tmpPeak.ApexIndex);
                if (WasFrontSkim)
                {
                    TCandidatePeak& curDaughter = *(curPeak.pFDaughters->back()) ;
                    curDaughter.FStartIsValley = false;
                    curDaughter.FFusedStart = false;
                    PropagateValleyChanges(curDaughter, curDaughter);
                }
                else
                    {
                    TCandidatePeak& curDaughter = *(curPeak.pFDaughters->back()) ;
                    curDaughter.FEndIsValley = false;
                    curDaughter.FFusedEnd = false;
                    PropagateValleyChanges(curDaughter, curDaughter);
                }
            }
        }
    }
}

void TInternalIntegrator::CreateResultLines()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    for(int lineidx=0; lineidx<FCandidLines.size(); ++lineidx)
    {
        TCandidateBaseLine& curline = FCandidLines[lineidx] ;
        curline.StartTime = curline.FStartIndex * DeltaTime ;
        curline.EndTime   = curline.FEndIndex * DeltaTime;
        if (!(curline.FisHorizontalFromStart || curline.FisHorizontalFromEnd))
        {
            curline.StartValue = SignalSmooth[curline.FStartIndex] ;
            curline.EndValue   = SignalSmooth[curline.FEndIndex]   ;
        }
        else
            if (curline.FisHorizontalFromStart)
        {
            curline.StartValue = SignalSmooth[curline.FMeasureStartAtIndex] ;
            curline.EndValue   = curline.StartValue ;
            CorrectPeaksOfHorizLine(curline,curline.FAssociatedPeaks);
        }
        else
            if (curline.FisHorizontalFromEnd)
        {
            curline.EndValue = SignalSmooth[curline.FMeasureEndAtIndex] ;
            curline.StartValue = curline.EndValue ;
            CorrectPeaksOfHorizLine(curline,curline.FAssociatedPeaks);
        }

// directly associate peaks with the new line
// in order to speed up the computation of height and area
// + use the loop to determine the type of the line
// (it is exponential if all the peaks want an exponential line)
        bool LineIsExponential = true;
        for(TCandidatePeakList::iterator peak_it = curline.FAssociatedPeaks.begin();
        peak_it != curline.FAssociatedPeaks.end();
        peak_it++)
        {
            TCandidatePeak& curpeak = **peak_it ;
            assert(&curpeak!=NULL);

            curpeak.pFmyLine = &curline ;
            LineIsExponential = LineIsExponential && (curpeak.FmyLineType == BASELINE_EXPONENTIAL);
        }
        assert(!(LineIsExponential && (curline.FisHorizontalFromStart || curline.FisHorizontalFromEnd)));

// exponential type is accepted only if exponential  equation can be solved
// (heights must have the same sign and be non null and be different)
        LineIsExponential = LineIsExponential && (curline.StartValue != curline.EndValue);

        if (LineIsExponential)
            curline.type = BASELINE_EXPONENTIAL ;
        else
            curline.type = BASELINE_STRAIGHT ;

    }
}

void TInternalIntegrator::Local_HandleLeftIntersection( TCandidateBaseLine& baseline,
    TCandidatePeak& thePeak,
    int intersecIndex)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(intersecIndex >= thePeak.StartIndex);
    assert(intersecIndex <= thePeak.ApexIndex);
    if (!thePeak.FForcedStart)
    {
        int OrigIndex = thePeak.StartIndex;
        thePeak.StartIndex = min(intersecIndex, thePeak.GetMinDaughterIndex());
// correction of the baseline is possible only because
// each peak has its own baseline. This may change in the future.
// also baseline correction will be made after small peaks removal
        baseline.StartTime = thePeak.StartIndex * DeltaTime;
        if (OrigIndex!=thePeak.StartIndex)
        {
//DT4341. It is necessary to break any valley. This is the code originally written in ProcessHorizontalBaseLine
            if (thePeak.myIndexInTheWholeList > 0)
            {
                TCandidatePeak& predPeak = FCandidPeaks[thePeak.myIndexInTheWholeList-1];
                predPeak.FFusedEnd = false;
                predPeak.FEndIsValley = false;
                thePeak.FFusedStart = false;
                thePeak.FStartIsValley = false;
            }
        }
    }
}

void TInternalIntegrator::Local_HandleRightIntersection( TCandidateBaseLine& baseline,
    TCandidatePeak& thePeak,
    int intersecIndex)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(intersecIndex >= thePeak.ApexIndex);
    assert(intersecIndex <= thePeak.EndIndex);

    if (!thePeak.FForcedEnd)
    {
        int OrigIndex = thePeak.EndIndex;
        thePeak.EndIndex = max(intersecIndex, thePeak.GetMaxDaughterIndex());
// same remark as above on baseline correction
        baseline.EndTime = thePeak.EndIndex * DeltaTime ;
        if (OrigIndex!=thePeak.StartIndex)
        {
            if (thePeak.myIndexInTheWholeList < (FCandidPeaks.size()-1))
            {
                TCandidatePeak& succPeak = FCandidPeaks[thePeak.myIndexInTheWholeList+1];
                thePeak.FFusedEnd = false;
                thePeak.FEndIsValley = false;
                succPeak.FFusedStart = false;
                succPeak.FStartIsValley = false;
            }
        }
    }
}

void TInternalIntegrator::CorrectPeaksOfHorizLine(TCandidateBaseLine& baseline,
    TCandidatePeakList& AssociatedPeaks)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    int i1,i2;
    TLineCurveIntersectType IntersectType;

    assert(AssociatedPeaks.size() == 1); // each peak has a baseline. Assumption made in order to correct the lines start/end

    INT_FLOAT YValue = baseline.EndValue;
    for(TCandidatePeakList::iterator peak_it = AssociatedPeaks.begin();
    peak_it != AssociatedPeaks.end();
    peak_it++)
    {
        TCandidatePeak& curPeak = **peak_it ;
/*
	BO 30/01/01 DT3794
	Removed the assertion (curPeak.FApexIndex > 0) and replaced with a test,
	since peaks with FApexIndex are tolerated (peaks with a forced start or end).
	This means that we do not try to correct such peaks. To be done in the future if such
	a correction appears necessary.
      */

        if (curPeak.ApexIndex > 0)
        {
// 1, look for left intersection
            FindIntersectLineCurve_NotContact(SignalSmooth, curPeak.StartIndex, curPeak.EndIndex, YValue, YValue, IntersectType, i1, i2);
            if (IntersectType!=INTERSECT_NONE)
                if (i2 <= curPeak.ApexIndex)
            {
                Local_HandleLeftIntersection(baseline, curPeak, i2);
// 2, look for a right intersection
                FindIntersectLineCurve_NotContact(SignalSmooth, curPeak.ApexIndex, curPeak.EndIndex, YValue, YValue, IntersectType, i1, i2);
                if (IntersectType!=INTERSECT_NONE)
                    Local_HandleRightIntersection(baseline, curPeak, i1);
            }
            else // this is a right intersection
                {
                Local_HandleRightIntersection(baseline, curPeak, i1);
            }
        }
    }
}

void  TInternalIntegrator::AddCandidateBaseLine(TCandidateBaseLine& NewLine,
    int leftLimit,
    int rightLimit,
    int level)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */

//TRANSLATION : This function should be called after having added (or created) a new line
// in the FCandidLines list. IT WILL NOT ADD THE LINE IN THE LIST BUT ONLY SPLIT IT IF NEEDED.

{
    assert(NewLine.FAssociatedPeaks.size() > 0);
    assert( FCandidLines.indexof(NewLine) != -1 ); //pNewLine is already in the list.

    TLineCurveIntersectType IntersectType ;
    int iStart, iEnd;

#ifdef DEBUG
    dbgstream() << "Line from "<< NewLine.FStartIndex * DeltaTime ;
    dbgstream() << " to " << NewLine.FEndIndex * DeltaTime;
    dbgstream() << " with " << NewLine.FAssociatedPeaks.size() << " peaks " ;
//sprintf(linedescr,"Line from %.3f to %.3f min, with %d peaks", [NewLine.FStartIndex * DeltaTime, NewLine.FEndIndex * DeltaTime, NewLine.FassociatedPeaks.size()]);
#endif

    if (NewLine.FEndIndex!=NewLine.FStartIndex)
    {
        FindIntersectLineCurve_NotContact(SignalSmooth, NewLine.FStartIndex, NewLine.FEndIndex,
            SignalSmooth[NewLine.FStartIndex],
            SignalSmooth[NewLine.FEndIndex],
            IntersectType, iStart, iEnd);
    }
    else
        {
        IntersectType = INTERSECT_NONE;
        ++(NewLine.FEndIndex); // temporary solution for PowerTrack Issue #64
// This solution is OK since peaks associated to the line will be deleted later, and thus the line itself will be deleted
// Better solution: the caller of AddCandidateBaseLine should check that the line is not reduced to
// a single point and take appropriate decisions (ie delete the peaks and baseline)
    }

    if (IntersectType == INTERSECT_NONE)
    {
//TRANSLATION : The new line has already been instanciated in the list, so nothing
//              has to be done here.
#ifdef DEBUG
        dbgstream() << " is good." << endl ;
#endif
    }
    else
        switch(NewLine.FAssociatedPeaks.size()){
        case 1 :
            {
    #ifdef DEBUG
                dbgstream() << " is wrong : intersection btw [" << iStart * DeltaTime ;
                dbgstream() << " , " << iEnd*DeltaTime << "] min." << endl ;
    #endif
                ImproveLine(NewLine, iStart, iEnd, level + 1);
                break;
        }
    default:
        {
#ifdef  DEBUG
            dbgstream() << " is wrong : intersection btw [" << iStart * DeltaTime ;
            dbgstream() << " , " << iEnd*DeltaTime << "] min." << endl ;
#endif
            BreakTheCandidateLine(NewLine, leftLimit, rightLimit, level + 1);
        }
    }
}

void TInternalIntegrator::AddCandidatePeak(TCandidatePeak& peak)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */

//TRANSLATION : Do not add the peak in the list. Only manage
//              things related to this peaks when is is being added
//              in the list.
{  
    peak.myIndexInTheWholeList = FCandidPeaks.indexof(peak);
}

void TInternalIntegrator::PropagatePeakDeletion(TCandidatePeak& curCandidPeak,
    bool UpdateIndexes)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History : During translation : remove the peak from the
 *           list associated to the baseline.
 * -------------------------- */
{
// Remove the peak from its baseline list :
    if (curCandidPeak.pFmyLine!=NULL)
    {
        TCandidateBaseLine& theline = *(curCandidPeak.pFmyLine) ;
        theline.FAssociatedPeaks.erase(find(theline.FAssociatedPeaks.begin(),
            theline.FAssociatedPeaks.end(),
            &curCandidPeak)
            );
    }

// 0) shorten baseline if peak lies at the beginning or end of a group
// 1) peak can no more be fused with other peaks
    if (curCandidPeak.myIndexInTheWholeList > 0)
    {
        TCandidatePeak& tmpPeak = FCandidPeaks[curCandidPeak.myIndexInTheWholeList - 1];
        tmpPeak.FFusedEnd = false;
        tmpPeak.FEndIsValley = false;
    }

    if ( curCandidPeak.myIndexInTheWholeList < (FCandidPeaks.size()-1) )
    {
        TCandidatePeak& tmpPeak = FCandidPeaks[curCandidPeak.myIndexInTheWholeList + 1];
        tmpPeak.FFusedStart = false;
        tmpPeak.FStartIsValley = false;
    }

// if the peak is really removed from the list,  FmyIndexInTheWholeList must be
// updated for all following peaks
    if (UpdateIndexes)
        for (int idxPeak = curCandidPeak.myIndexInTheWholeList + 1 ;
    idxPeak < FCandidPeaks.size();
    idxPeak++)
    {
        FCandidPeaks[idxPeak].myIndexInTheWholeList--;
    }
}

void TInternalIntegrator::ImproveLine(TCandidateBaseLine& newLine,
    int intersecStart,
    int intersecEnd,
    int level)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert((newLine.FAssociatedPeaks.size() == 1));
    assert((intersecStart <= intersecEnd));

    int StartTestLine, EndTestLine;

    TCandidatePeak& thePeak = *(newLine.FAssociatedPeaks.front());

    INT_INDEXLIST halfhull;

    FindConvexHalfHull(SignalSmooth, SignalSize, newLine.FStartIndex, newLine.FEndIndex, thePeak.FisPositive, halfhull);

//  with thePeak do
    int _apexIndex = (thePeak.ApexIndex == 0) ? ((thePeak.StartIndex + thePeak.EndIndex) / 2) : thePeak.ApexIndex ;
    FindTestLine(halfhull, _apexIndex, StartTestLine, EndTestLine);

// no limit check since we do not go outside of the limits of the line
// (and theses limits should already be OK)
    int curPositionLeft  = (thePeak.FForcedStart) ? newLine.FStartIndex : max(newLine.FStartIndex, StartTestLine) ;
    int curPositionRight = (thePeak.FForcedEnd)   ? newLine.FEndIndex   : min(newLine.FEndIndex, EndTestLine) ;

// change extremities
#ifdef DEBUG
//with newline do
//OptionalDebugShift(level, format('[Improve Mode] Line corrected after %f %% left/right move', [100 -(curPositionRight - curPositionLeft) / (FendIndex - Fstartindex) * 100]));
    dbgstream() << "[Improve Mode] Line corrected after "
    << 100 - fabs(curPositionRight - curPositionLeft) / (newLine.FEndIndex - newLine.FStartIndex) * 100
    << "% left/right move" << endl ;
#endif

// report changes to the associated peak
    thePeak.FStartIsValley = (thePeak.FStartIsValley && (newLine.FStartIndex == curPositionLeft));
    thePeak.FFusedStart = (thePeak.FFusedStart && (newLine.FStartIndex == curPositionLeft));

    thePeak.FEndIsValley = (thePeak.FEndIsValley && (newLine.FEndIndex == curPositionRight));
    thePeak.FFusedEnd = (thePeak.FFusedEnd && (newLine.FEndIndex = curPositionRight));

    newLine.FStartIndex = curPositionLeft;
    newLine.FEndIndex = curPositionRight;

    thePeak.StartIndex = newLine.FStartIndex;
    thePeak.EndIndex = newLine.FEndIndex;

    PropagateValleyChanges(thePeak, thePeak);
}

void TInternalIntegrator::BreakTheCandidateLine(TCandidateBaseLine& NewLine,
    int leftliSmoothWithWaveletsProcmitOrig,
    int rightlimitOrig,
    int level)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(NewLine.FAssociatedPeaks.size() >= 2);

    INT_INDEXLIST halfhull ;
    TCandidateBaseLineList Lines;

#ifdef DEBUG
// check whether all the peaks have the same sign
// if not we will have to do something, but what ? --> to do later
//  for idxCurPeak := 1 to pred(newline.FAssociatedPeaks.count)
//    do assert(newLine.FAssociatedPeaks.ItemIs(idxCurPeak).FisPositive = newLine.FAssociatedPeaks.ItemIs(0).FisPositive) ;
#endif

    FindConvexHalfHull(SignalSmooth, SignalSize, NewLine.FStartIndex, NewLine.FEndIndex,
        (NewLine.FAssociatedPeaks.front()->FisPositive), halfhull);
// associate peaks and lines
    for(TCandidatePeakList::iterator peak_it = NewLine.FAssociatedPeaks.begin();
    peak_it != NewLine.FAssociatedPeaks.end();
    peak_it++)
    {
        TCandidatePeak& curPeak = **peak_it;

        int apexIndex = (curPeak.ApexIndex == 0) ? ((curPeak.StartIndex + curPeak.EndIndex) / 2) : curPeak.ApexIndex ;
        Local_FindLine mysearch; //instanciates the search function
        FindTestLine(halfhull, apexIndex, mysearch.StartTestLine, mysearch.EndTestLine);
        TCandidateBaseLineList::iterator curLine_it = find_if(Lines.begin(),Lines.end(),mysearch);

        TCandidateBaseLine* pCurLine ;
        if (curLine_it == Lines.end()) //not found
        {
            pCurLine = &(FCandidLines.push_back()) ;//creates a new line in the list
            pCurLine->FStartIndex = mysearch.StartTestLine ;
            pCurLine->FEndIndex = mysearch.EndTestLine;
            Lines.push_back(pCurLine); // stored just for easier retrieval in next loop
        }
        else
            {
            pCurLine = *curLine_it  ;
        }
        pCurLine->FAssociatedPeaks.push_back(*peak_it);
    }

#ifdef DEBUG
    dbgstream() << "Now improving peak extremities" << endl ;
#endif
// improve peak extremities thanks to baselines
    for(TCandidateBaseLineList::iterator line_it = Lines.begin();
    line_it != Lines.end();
    line_it++)
    {
        TCandidateBaseLine* pcurLine = *line_it ;
#ifdef DEBUG
        dbgstream() << "[Break Mode] Line from "
        << pcurLine->FStartIndex *DeltaTime
        << " to "
        << pcurLine->FEndIndex*DeltaTime
        << " with "
        << pcurLine->FAssociatedPeaks.size()
        << " peaks is good"
        << endl ;
#endif

        TCandidatePeak& firstPeak = *(pcurLine->FAssociatedPeaks.front()) ;
        TCandidatePeak& lastPeak  = *(pcurLine->FAssociatedPeaks.back() ) ;

// reconcile peak/line extremities
        firstPeak.StartIndex = pcurLine->FStartIndex;
        lastPeak.EndIndex    = pcurLine->FEndIndex;
// last peak on the line can not have a fused end  end;
        lastPeak.FFusedEnd = false;
// and can not end with a valley
        lastPeak.FEndIsValley = false;
// first peak on the line can not have a fused start
        firstPeak.FFusedStart = false;
// and can not start with a valley
        firstPeak.FStartIsValley = false;
    }

    FCandidLines.erase(NewLine);
}

bool TInternalIntegrator::ThereIsaPeakStart( TPeakDetector& aDetector,
    TParameterState& paramState,
    TAlgoState& algoState,
    TPeakState& peakState,
    INT_FLOAT curSample)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool pst = aDetector.PotentialStart() ;
    bool slt = (aDetector.SlopeType() == PositiveSlope);
    bool fnp = (curSample < algoState.FThresholdForNegPeaks);
    bool msn = (peakState.FMustStartNow);
    bool spn = (peakState.FMustSplitNow);
    bool ito = paramState.FIntegrationOn ;
    bool result =  (  ( pst && ( slt || fnp ) || msn || spn  ) && ito) ;

#ifdef DEBUG
    if (result)
        OptionalDebug("There is a peak start !",3);
    else
        {
        if (pst) OptionalDebug("+PST",3); else OptionalDebug("-pst",3) ;
        if (slt) OptionalDebug("+SLT",3); else OptionalDebug("-slt",3) ;
        if (fnp) OptionalDebug("+FNP",3); else OptionalDebug("-fnp",3) ;
        if (msn) OptionalDebug("+MSN",3); else OptionalDebug("-msn",3) ;
        if (spn) OptionalDebug("+SPN",3); else OptionalDebug("-spn",3) ;
        if (ito) OptionalDebug("+ITO",3); else OptionalDebug("-ito",3) ;

    }
#endif

    return result ;

}
bool TInternalIntegrator::ThereIsaPeakEnd( TPeakDetector& Detector,
    TParameterState& paramState,
    TPeakState& peakState)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TCandidatePeak& curpeak = FCandidPeaks.back() ;
    bool apx = peakState.ApexFound ;
    bool fcd = curpeak.FForcedStart ;
    bool pte = Detector.PotentialEnd();

#ifdef DEBUG
//   dbgstream() << "   -- test peak end -- (apx:"<<apx<<" || fcd:"<<fcd<<") && pte:"<<pte
// 	      << " || men:"<< peakState.FMustEndNow
// 	      << " || msn:"<< peakState.FMustStartNow
// 	      << " || spl:"<< peakState.FMustSplitNow;
#endif

    bool result =
        (( apx || fcd) && pte
        && (Detector.EstimatedDeriv1() * Detector.EstimatedDeriv2() < 0)
        || !paramState.FIntegrationOn // integration OFF must end the current peak
        || peakState.FMustEndNow
        || peakState.FMustStartNow
        || peakState.FMustSplitNow ) && (!peakState.FSlicing);
#ifdef DEBUG
//   dbgstream() << "  peak end : " << result << endl;
#endif

    return result ;
}

bool TInternalIntegrator::ThereIsaPeakApex( TPeakDetector& Detector,
    TParameterState& paramState,
    TPeakState& peakState)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool check1 =
        (!peakState.ApexFound) && Detector.PotentialApex()
    && ((peakState.peaktype == PositivePeak) && (Detector.EstimatedDeriv2() < 0)
        ||
        (peakState.peaktype == NegativePeak) && (Detector.EstimatedDeriv2() > 0)
        )
    && (paramState.FIntegrationOn && (!peakState.FSlicing));

    if (check1)
        return (peakState.Change != Detector.SlopeChange());
    else
        return false;

}
bool TInternalIntegrator::ThereIsaValley( TPeakDetector& Detector,
    TParameterState& paramState,
    TPeakState& peakState)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool check1 =
        peakState.ApexFound && Detector.PotentialValley() 
        && paramState.FIntegrationOn && !peakState.FSlicing;
    if (check1)
        return (peakState.Change != Detector.SlopeChange());
    else
        return false;

}
bool TInternalIntegrator::LastTwoPeaksHaveSameSign()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (FCandidPeaks.size() < 3)
        return true ;
    else
        {
        int lastidx = FCandidPeaks.size()-2;
        return FCandidPeaks[lastidx].FisPositive == FCandidPeaks[lastidx-1].FisPositive;
    }
}

#ifdef TESTING
static void TInternalIntegrator::Test(){

}
#endif

void SmoothWithWaveletsProc_Standard(INT_SIGNAL Signal,
    int SignalSize,
    INT_SIGNAL SignalSmooth,
    INT_FLOAT ThresFactor,
    INT_FLOAT& NoiseStdDev)
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    const INT_FLOAT EPSILON_THRES = 1.0E-5;
    Wavefilt wfilter(DAUBECHIES_DEFAULT_ORDER);
    int ClosestPowerOf2 = 1;
    int idxClosestPowerOf2 = 0;

    while (ClosestPowerOf2 < SignalSize)
    {
        ClosestPowerOf2 = ClosestPowerOf2 * 2;
        idxClosestPowerOf2++;
    }

    INT_FLOAT coef1, coef2;
    if (idxClosestPowerOf2 <= 6) { coef1 = 1.678; coef2 = 8.004; }
    else
        if (idxClosestPowerOf2 >= 16) { coef1 = 3.640; coef2 = 6.748; }
    else
        switch (idxClosestPowerOf2) {
        case 7 :  { coef1 = 1.893 ; coef2 = 7.980 ;  break;}
        case 8 :  { coef1 = 2.116 ; coef2 = 7.549 ;  break;}
        case 9 :  { coef1 = 2.331 ; coef2 = 7.259 ;  break;}
        case 10 : { coef1 = 2.538 ; coef2 = 7.069 ;  break;}
        case 11 : { coef1 = 2.737 ; coef2 = 6.939 ;  break;}
        case 12 : { coef1 = 2.930 ; coef2 = 6.848 ;  break;}
        case 13 : { coef1 = 3.116 ; coef2 = 6.799 ;  break;}
        case 14 : { coef1 = 3.296 ; coef2 = 6.760 ;  break;}
        case 15 : { coef1 = 3.471 ; coef2 = 6.748 ;  break;}
        default:  { coef1 = 0     ; coef2 = 0     ;  break;}// this case will not occur
        }

    #ifdef DEBUG
//OptionalDebug("Wavelets smoothing...");
//cout<<"#points="<<SignalSize<<", closest power of 2 is "<<ClosestPowerOf2<<"=2^"<<idxClosestPowerOf2<<endl;
//OptionalDebug(format('#points=%d, closest power of 2 is %d=2^%d', [Signal.len, ClosestPowerOf2, idxClosestPowerOf2]));
    #endif

        INT_SIGNAL wavedata = new INT_FLOAT[ClosestPowerOf2+1] ;
        int i ;
        for(i=1; i<=SignalSize; i++) wavedata[i] = Signal[i-1];
        for(i=SignalSize+1; i<=ClosestPowerOf2; i++) wavedata[i] = 0;

        int nn = ClosestPowerOf2;
        do {
            wfilter.pwt(wavedata, nn, 1);
            nn /= 2; //shr 1 translated by /=2. GO.
        } while (nn >= 4);

// computation of noise variance by the method described
// in the book "a wavelet tour of signal processing" p.447
        int medianSize = SignalSize/2 ;
        INT_SIGNAL tmp_mediandata = new INT_FLOAT[medianSize] ;
        for(i=0; i<=(SignalSize / 2 - 1); i++) tmp_mediandata[i] = fabs(wavedata[i + 1 + (ClosestPowerOf2 / 2)]);

//assert(length(tmp_mediandata) > 0);
        INT_FLOAT ThresComputed = ComputeMedian(tmp_mediandata,medianSize) / 0.6745;
        delete [] tmp_mediandata ;

        INT_FLOAT Thres = (ThresFactor > 0) ? (ThresComputed * ThresFactor) : ThresComputed ;
        NoiseStdDev = Thres;

    #ifdef DEBUG
//OptionalDebug(format('Smoothing with threshold %f (user defined factor %f, true threshold %f)', [Thres, ThresFactor, ThresComputed]));
    #endif

        if (Thres > EPSILON_THRES)
            for(int i=1; i<=ClosestPowerOf2; i++)
        wavedata[i] = semisoftshrink(coef1, coef2, wavedata[i] / Thres) * Thres;

        nn = 4;
        do {
            wfilter.pwt(wavedata, nn, -1);
            nn *= 2;//translated shl 1 by *=2. GO.
        } while(nn <= ClosestPowerOf2);

    #ifdef DEBUG
//   sx := 0; sx2 := 0;
//   for i := 1 to Signal.len
//     do begin
//     v := wavedata[i] - Signal.data[i];
//     sx := sx + v;
//     sx2 := sx2 + v * v;
//   end;
//   variance := sx2 / signal.len - sqr(sx / Signal.len);
//   if variance > 0 then std := sqrt(variance) else std := 0;

//   OptionalDebug('std deviation signal-smoothed ' + FloatToStr(std));
    #endif

        for(i=1; i<=SignalSize; i++) SignalSmooth[i-1] = wavedata[i];

        delete [] wavedata ;
    }

    void SmoothWithWaveletsProc(INT_SIGNAL  Signal,
        int SignalSize,
        INT_SIGNAL SignalSmooth,
        INT_FLOAT ThresFactor,
        INT_FLOAT&  NoiseStdDev)
    /* --------------------------
         * Author  : Bruno Orsier : Original Delphi code
                     Gilles Orazi : C++ translation
         * Created : 06/2002
         * Purpose :
         * History :
         * -------------------------- */
    {
        if (SignalSize > 1)
            SmoothWithWaveletsProc_Standard(Signal, SignalSize, SignalSmooth, ThresFactor, NoiseStdDev);
        else
            {
            NoiseStdDev = 0; // not defined
            if (SignalSize == 1) SignalSmooth[0] = Signal[0] ;
        }
    }

    void FindConvexHalfHull(INT_SIGNAL Signal,
        int SignalSize,
        int idxOrigin,
        int idxExtr,
        bool bottom,
        INT_INDEXLIST& HalfHull)
    /* --------------------------
         * Author  : Bruno Orsier : Original Delphi code
                     Gilles Orazi : C++ translation
         * Created : 06/2002
         * Purpose :
         * History :
         * -------------------------- */
    {
//assert(Signal <> nil);
//assert((idxOrigin >= 0) && (idxOrigin < Signal.len));
//assert((idxExtr >= 0) and (idxExtr < Signal.len));
//assert(HalfHull <> nil);
//
//assertions not translated because of signal.len has no equivalent
//should we change the signal class for a vector.
//GO. (during translation into C++)

        HalfHull.push_back(idxOrigin);

        for (int idxCurPoint = idxOrigin+1;
        idxCurPoint <= idxExtr;
        idxCurPoint++)
        {
            INT_FLOAT curPointX = idxCurPoint ;
            INT_FLOAT curPointY = Signal[idxCurPoint];

            bool stop = false ;
            int idxPrevPoints = HalfHull.size()-1;
            CLASSIFY_RESULT side ;
            if (bottom)
            {
                side = LEFT;
        }
        else
            {
            side = RIGHT;
        }

        while ( (!stop) && (idxPrevPoints>0) )
        {
            INT_FLOAT prevPointX = HalfHull[idxPrevPoints];
            INT_FLOAT prevPointY = Signal[HalfHull[idxPrevPoints]];
            INT_FLOAT neighborX  = HalfHull[idxPrevPoints-1];
            INT_FLOAT neighborY  = Signal[HalfHull[idxPrevPoints-1]];

            CLASSIFY_RESULT prevPointType = Classify(curPointX, prevPointX, neighborX, curPointY, prevPointY, neighborY);
            stop =
                (prevPointType != side) &&
                (prevPointType != BEYOND) &&
                (prevPointType != BETWEEN);
            idxPrevPoints-- ;
        }

        if (stop)
        {
            while ( ((int)HalfHull.size()) >idxPrevPoints+2)
            {
                HalfHull.pop_back();
            }
        }
        else
            {
            while (HalfHull.size()>1)
            {
                HalfHull.pop_back();
            }
        }
        HalfHull.push_back(idxCurPoint);
    }

}


