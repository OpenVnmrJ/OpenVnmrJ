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

// File     : integrator_util_peaks.h
// Author   : Bruno Orsier : original code in Delphi
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration algorithm
//            Peaks and baselines handling
// $History: integrator_util_peaks.h $
/*  */
/* *****************  Version 11  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 10  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 9  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 8  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef __INTEGRATOR_UTIL_PEAKS_H
#define __INTEGRATOR_UTIL_PEAKS_H

#include "integrator_types.h"
#include "owninglist.h"
#include <string>

#ifdef INTEGR_DEBUG_MEMORY
int  Glob_inCandidPeaksCount,
Glob_inCandidPeaksTotal,
Glob_inCandidBlinesCount,
Glob_inCandidBlinesTotal;
#endif

extern "C" {
void Integr_ComputeBaselineExponentialCoefficients( 
    const INT_TIME start_time,
    const INT_TIME end_time,
    const INT_FLOAT start_value,
    const INT_FLOAT end_value,
    INT_FLOAT &A,
    INT_FLOAT &B,
    INT_FLOAT &t0);

INT_FLOAT Integr_ComputeBaselineExponentialValue(
    const INT_FLOAT t, 
    const INT_FLOAT A, 
    const INT_FLOAT B, 
    const INT_FLOAT t0);
}

    
class TCandidatePeak     ; //forward declaration
class TCandidateBaseLine ; //forward declaration

typedef OwningList<TCandidatePeak>  TCandidatePeakOwningList;
typedef vector<TCandidatePeak*>  TCandidatePeakList;

class  TCandidatePeak{
private:
    void _defaultvalues();
    INT_FLOAT
    FMaxSlope;
public:
    int
        ApexIndex,
        BunchSize,
        EndIndex,
        MotherIndex,
        myIndexInTheWholeList,
        StartIndex;

    INT_FLOAT 
        StartTime,
        RetentionTime,
        EndTime,
        Area, pcArea,
        Height, pcHeight,
        MaxHeight, MinHeight;

    //StartTime (resp. EndTime) is not always equal to StartIndex*DeltaT
    //There are some cases where the peak is starting between 2 data points
    //(e.g. split peak event).
    //GO

    INT_FLOAT LeftInflexionPointTime ;
    INT_FLOAT RightInflexionPointTime ;
    int LeftInflexionPointErrorCode ;
    int RightInflexionPointErrorCode ;
    INT_FLOAT LeftTgCoeffA ;
    INT_FLOAT LeftTgCoeffB ;
    INT_FLOAT RightTgCoeffA ;
    INT_FLOAT RightTgCoeffB ;

    INT_FLOAT InterTgLb_X1;
    INT_FLOAT InterTgLb_X2;

    INT_FLOAT PrevValleyHeight;

    INT_FLOAT
        ASYMETRY_PHARMACOP_EUROP,
        ASYMETRY_USP_EMG_EP_ASTM,
        ASYMETRY_HALF_WIDTHS_44,
        ASYMETRY_HALF_WIDTHS_10;

    INT_FLOAT
        THPLATES_SIGMA2,
        THPLATES_SIGMA3,
        THPLATES_SIGMA4,
        THPLATES_SIGMA5,
        THPLATES_USP,
        THPLATES_EP_ASTM,
        THPLATES_EMG,
        THPLATES_AREAHEIGHT;

    INT_FLOAT
        Width_TgBase,
        Width_4p4,
        Width_5p,
        Width_10p,
        Width_13p4,
        Width_32p4,
        Width_50p,
        Width_60p7;

    INT_FLOAT
        Moment1,
        Moment2,
        Moment3,
        Moment4,
        Skew,
        Kurtosis;

    INT_FLOAT
        Left_Width_4p4,
        Left_Width_5p,
        Left_Width_10p,
        Left_Width_13p4,
        Left_Width_32p4,
        Left_Width_50p,
        Left_Width_60p7;

    INT_FLOAT
        Resolution_Valleys,
        Resolution_HalfWidth,
        Resolution_USP,
        Capacity,
        Selectivity;

    INT_FLOAT
        BaseLineHeight_AtRetTime;

    TCandidatePeakList*
        pFDaughters ;

    bool
        FDropLineAtEnd,
        FDropLineAtStart,
        FEndIsValley,
        FForcedEnd,
        FForcedStart,
        FFusedEnd,
        FFusedStart,
        FHasEventBaseLine,
        FEventLineIsHorizontal, // in order to determine whether skimming can apply
        FisDaughter,
    //      FisKept,
        FisMother,
        FisPositive,
        FStartIsValley;

    TCandidateBaseLine*
        pFmyLine ;  // stores the baseline of the candidate peak, but only inside CreateResultLines.

    TBaseLineType FmyLineType;

    string
        code ;

    char
        FStartCode,
        FEndCode;
    bool 
        isUserSlice ; 

    TCandidatePeak();
    TCandidatePeak(
        int StartIndex,
        int EndIndex,
        int ApexIndex,
        int BunchSize);
    ~TCandidatePeak() ;

    void  UpdateMaxSlope(INT_FLOAT slope);
    bool ApexIndexIsValid() ;
    void SetDefaultCodes() ;
    void SetHorizontalCodes() ;
    void SetSkimmingCodes(TBaseLineType linetype);
    int GetMaxDaughterIndex() ;
    int GetMinDaughterIndex();

    CLASS_READONLY_PROPERTY(MaxSlope, INT_FLOAT, return FMaxSlope);
};

class TCandidateBaseLine{
private:
    void __initialize();
public:
    TCandidatePeakList
    FAssociatedPeaks;

    int
    FEndIndex,
    FMeasureEndAtIndex,
    FMeasureStartAtIndex,
    FStartIndex;

    bool
    FForcedEnd,
    FForcedStart,
    FisEventLine,
    FisHorizontalFromEnd,
    FisHorizontalFromStart;

    INT_FLOAT
    StartTime, StartValue,
    EndTime,   EndValue ;

    TBaseLineType
        type ;

    TCandidateBaseLine(int startindex, int endindex) ;
    TCandidateBaseLine();
    ~TCandidateBaseLine() ;

    void GetExponentialCoefficients(INT_FLOAT &A, INT_FLOAT &B, INT_FLOAT &t0);

    friend bool operator==(const TCandidateBaseLine& lhs, const TCandidateBaseLine& rhs)
    {return ( (lhs.FStartIndex==rhs.FStartIndex) &&
        (lhs.FEndIndex==rhs.FEndIndex) );}
};

typedef OwningList<TCandidateBaseLine>  TCandidateBaseLineOwningList ;
/*    constructor Create;
      procedure Add(item : TCandidateBaseLine);
      procedure Insert(Index: Integer; item: TCandidateBaseLine);
      function ItemIs(const index: Integer): TCandidateBaseLine;
      procedure Remove(item : TCandidateBaseLine);
*/

typedef vector<TCandidateBaseLine*>  TCandidateBaseLineList ;
/*
  constructor Create;
  procedure Add(item : TCandidateBaseLine);
  procedure Insert(Index: Integer; item: TCandidateBaseLine);
  function ItemIs(const index: Integer): TCandidateBaseLine;
  procedure Remove(item : TCandidateBaseLine);
*/

typedef struct {
    INT_FLOAT Time ;
    INT_FLOAT Threshold;
    INT_FLOAT RelativeThreshold ;
} TAutoThresholdResult ;

#endif

