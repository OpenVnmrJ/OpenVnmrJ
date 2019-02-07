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

// File     : integrator_calcul.h
// Author   : Bruno Orsier : original Delphi code
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration algorithm
//            main processing classes
//
// $History: integrator_calcul.h $
/*  */
/* *****************  Version 15  ***************** */
/* User: Go           Date: 8/11/02    Time: 17:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* C++ translation of the integration algorithm.  */
/* All main translation bugs fixed after having performed the official */
/* integration tests. */
/*  */
/* *****************  Version 14  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 13  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 12  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 11  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 10  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 9  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef __INTEGRATOR_CALCUL_H
#define __INTEGRATOR_CALCUL_H

#include <vector>
#include "integrator_types.h"
#include "integrator_classes.h"
#include "integrator_util_peaks.h"
#include "integrator_util_events.h"
#include "integrator_util_misc.h"

using namespace std;

#define  INTEGRATOR_MIN_POINTS_APEX_EXTREM    3 // roughly 1/3 of INTEGRATOR_OPTIMAL_POINTS_PER_PEAK
#define  INTEGRATOR_REFSCALE                  100000 // in order to normalize the threshold
#define  INTEGRATOR_FUSED_PEAKS_RATIO         3.0
#define  INTEGRATOR_DEFAULT_SKIM_RATIO        4.0
#define  INTEGRATOR_SIGNALMAXVALUE_TOLERANCE  0.01 // must be in [0.0..1.0]. use to forbid peak detection

#define  DAUBECHIES_DEFAULT_ORDER             20    // ONLY 4 or 12 or 20 ... See Numerical Recipes function pwtset
#define  EPSILON_INTERSECTION                 1E-5  // should maybe depend on the chromato range
#define  EPSILON_ZERO                         1E-8

// do not change these constants
#define  IS_LEFTLIMIT                         1
#define  IS_RIGHTLIMIT                       -1
#define  NORMALIZINGVALUE_ABSOLUTE_THRESHOLD  1.0

typedef enum{
    PosThenNeg = 0,
    NegThenPos = 1,
    DontKnow   = 2} TSlopeChangeType;

typedef enum{
    PositiveSlope = 0,
    NegativeSlope = 1,
    ZeroSlope     = 2} TSlopeType;

typedef enum{
    PositivePeak = 0,
    NegativePeak = 1} TPeakType;

typedef enum{
    BeginShape    = 0,
    EndShape      = 1,
    TopShape      = 2,
    ValleyShape   = 3,
    ShoulderShape = 4} TShapeType;

typedef enum{
    NEGPEAKS_INVERSION_ON       = 0,
    NEGPEAKS_INVERSION_RUNNING  = 1,
    NEGPEAKS_INVERSION_OFF      = 2} TNegPeaksInversion;

typedef enum{
    NEGPEAKS_CLAMPING_ON       = 0,
    NEGPEAKS_CLAMPING_RUNNING  = 1,
    NEGPEAKS_CLAMPING_OFF      = 2} TNegPeaksClamping;

typedef enum{
    NEGPEAKS_DETECTION_ON      = 0,
    NEGPEAKS_DETECTION_RUNNING = 1,
    NEGPEAKS_DETECTION_OFF     = 2} TNegPeaksDetection;

typedef enum{
    LEFT        = 0,
    RIGHT       = 1,
    BEYOND      = 2,
    BEHIND      = 3,
    BETWEEN     = 4,
    ORIGIN      = 5,
    DESTINATION = 6} CLASSIFY_RESULT ;

typedef enum{
    INTERSECT_NONE     = 0,
    INTERSECT_EXACT    = 1,
    INTERSECT_BETWEEN  = 2} TLineCurveIntersectType;

class  TPeakDetector {
public:
    INT_FLOAT
    FSlopeSensitivity,
    FSignalMaxValue,
    BM1, B0, B1, B2, B3 ;
    bool NegPeaksAllowed ;
    TSlopeType PreviousNonZeroSlope ;
    bool ZeroSlopeFound ;

public:
    TPeakDetector() ;
    ~TPeakDetector() ;

    void Push(INT_FLOAT val);
    bool PotentialStart() ;
    bool PotentialEnd() ;
    bool PotentialApex() ;
    bool PotentialValley() ;
    bool PotentialShoulder() ;
    TSlopeChangeType SlopeChange() ;
    TSlopeType SlopeType() ;
    TSlopeType SlopeTypeNoZero() ;
    INT_FLOAT EstimatedDeriv1() ;
    INT_FLOAT EstimatedDeriv2() ;
    INT_FLOAT EstiprocedurematedDeriv1() ;
    void ShoulderReset() ;
#ifdef TESTING
    static void Test();
#endif
};

class  TGeneralState{ // implements the "Strategy" Design pattern
public:
// general strategy, will call ProcessEvent
    virtual void UpdateFromEvents(INT_FLOAT curTime,
        TIntegrationEventList& aEvents );

// procedure to be implemented by derived classes
    virtual void ProcessEvent(TIntegrationEvent& theEvent) {};
#ifdef TESTING
    static void Test();
#endif
};

class  TRejectionState : public TGeneralState {
private:
    INT_FLOAT
    FMinHeight,
    FMinArea,
    FTotArea,
    FTotHeight ;
public:
    TRejectionState();
    bool KeepThePeak(TCandidatePeak& thePeak);
    void ProcessEvent(TIntegrationEvent& theEvent);
    void SetTotAmounts(INT_FLOAT  totarea,
        INT_FLOAT totheight);
#ifdef TESTING
    static void Test();
#endif
};

class TAlgoState : public TGeneralState {
public:
    TNegPeaksInversion FInvertNegativePeaks  ;
    TNegPeaksClamping  FClampNegativePeaks   ;
    TNegPeaksDetection FDetectNegativePeaks  ;
    INT_FLOAT          FThresholdForNegPeaks ;
public:
    bool FEstimateThresholdNow ;
    TAlgoState();
    virtual ~TAlgoState();
    INT_FLOAT ProcessCurrentData(INT_FLOAT data) ;
    void ProcessEvent(TIntegrationEvent& theEvent);
#ifdef TESTING
    static void Test();
#endif
};

class  TParameterState : public TGeneralState {
public:
    int 
        FbunchSize;
    bool
        FIntegrationOn ;
    INT_FLOAT
        FPeakWidth,
        FDeltaTime,
        FThres, FThresSolvent,
        FThresScale ;
    TTimeAndValue*
        pFSkimRatios;
public:
    INT_FLOAT
    FMinHeight,
    FMinArea ;

    TParameterState(INT_FLOAT deltat);
    virtual ~TParameterState();
    void SetPeakWidth(INT_FLOAT peakwidth);
    void SetThreshold(INT_FLOAT thres, bool ReScale);
    void SetThresholdSolvent(INT_FLOAT thres);
    void AddThreshold(INT_FLOAT thres, bool ReScale);
    void SetThresScale(INT_FLOAT scale);
    void ProcessEvent(TIntegrationEvent& theEvent);
#ifdef TESTTING
    static void Test();
#endif
};

class  TPeakState : public  TGeneralState {
public:
    bool
        PeakInProgress,
        ApexFound,
        FMustStartNow,
        FMustEndNow,
        FMustSplitNow;
    TSlopeChangeType
        Change ;
    TPeakType
        peaktype ;
    int
        NbPeaks,
        NbNegativePeaks;
    bool 
        FStartingSlice, 
        FSlicing ;

    TPeakState();
    virtual ~TPeakState();
    void FoundPeakStart(TSlopeType ch,  bool PreserveOldState);
    void FoundApex(TSlopeChangeType ch);
    void FoundValley(TSlopeChangeType ch);
    void FoundPeakEnd();
    void ProcessEvent(TIntegrationEvent& theEvent);
#ifdef TESTING
    static void Test();
#endif
};

class  TNoiseComputer : public TGeneralState {
protected:
    INT_SIGNAL
    FSignal;
    int
    FSignalSize;
    INT_FLOAT
    FDeltaTime,
    FNoise,
    FDrift,
    FRMSNoise;
    bool
    FWasSuccessful;
    int
    FiStart,
    FiEnd;
    void ComputeNoiseValues(int i1, int i2);
    void SetNoiseIntervalStart(int iStart);
    void SetNoiseIntervalEnd(int iEnd);
    bool ComputationIsFeasible();

public:
    TIntegrationEventList Events ;

    TNoiseComputer();
    virtual ~TNoiseComputer();

    void Initialize(INT_SIGNAL Signal, int SignalSize, INT_FLOAT DeltaTime);
    bool ComputeNoiseInInterval();
    INT_FLOAT GetNoise();
    INT_FLOAT GetDrift();
    INT_FLOAT GetRMSNoise();
    void ProcessEvent(TIntegrationEvent& theEvent);
#ifdef TESTING
    static void Test();
#endif
};

class  TInternalIntegrator {
private:
    void Local_ComputeLimits(int idxStartPeak, int& leftLimit, int& rightLimit) ;
// Was locally declared inside CreateDefaultBaseLines in the Delphi code.
// I declared it in private during the translation in C++. GO.

    void Local_HandleLeftIntersection( TCandidateBaseLine& baseline,
        TCandidatePeak& thePeak,
        int intersecIndex);

    void Local_HandleRightIntersection( TCandidateBaseLine& baseline,
        TCandidatePeak& thePeak,
        int intersecIndex);

    struct Local_FindLine {
        int StartTestLine ;
        int EndTestLine ;
        bool operator() (TCandidateBaseLine* x) {
        return ((StartTestLine == x->FStartIndex) && (EndTestLine == x->FEndIndex)); }
    };

public:
    INT_SIGNAL
        Signal,
        SignalSmooth;

    int
        SignalSize ;

    INT_FLOAT
        DeltaTime,
        DeadTime,
        PeakSat_min,
        PeakSat_max,
        FNoiseValue,
        FnoiseSDev;

    TIntegrationEventList
        Events;  //the events are owned


    vector<TAutoThresholdResult>  AutoThresholdResults ;

    bool
        FReduceNoise,
        FComputeBaseLines,     // added BO 25/09/00 for INUS style of integration (with a baseline at level 0)
        FUseRelativeThreshold; // added BO 02/12/00 for DT3425

    INT_FLOAT
        FSpikeReduceParameter;

    TNoiseComputer
        FNoiseComputer;

    void Initialize();
    void FindPeaksAverageSlope();
    void ComputeNoiseFromProcessedIntegrationEvents();
    int LocateEndNegLine(int i, TIntegrationEventList& AlgoEvents);

    //Peak properties computations :
    void RejectLinesWithNoPeak();
    void CorrectForSkimming();
    void SetupPeaksCode();
    void RejectPeaks(const INT_FLOAT totArea, const INT_FLOAT totHeight);
    void ComputePeaksProperties(INT_FLOAT& totArea, INT_FLOAT& totHeight);
    //---

    void ClearEventList() ;

    TInternalIntegrator();
    ~TInternalIntegrator();

// made public so that group processing can access it
    void PropagateValleyChanges(TCandidatePeak& FirstPeak, TCandidatePeak& LastPeak);

    inline TCandidatePeakOwningList& Peaks() {return FCandidPeaks;}
    inline TCandidateBaseLineOwningList& Lines() {return FCandidLines;}

protected:
    TIntegrationEventList
    FParameterEvents,
    FAlgoEvents,
    FPeakEvents,
    FBaseLineEvents,
    FAddPeaksEvents,
    FRejectionEvents ;

    TCandidatePeakOwningList
    FCandidPeaks ; // CAUTION: do not add peaks directly, use the procedure ADDCANDIDATEPEAK instead
// + if a peak is removed from this list, all indexes must be recomputed
    TCandidateBaseLineOwningList
    FCandidLines ;

    INT_BOOLVECTOR
    FOnIndexes;

    TStraightLineTool_Integer*
    pCurNegLine ; //put a pointer on object or object ??


    void SplitEvents(); // be to called during initialization

    void ExtractOffIndexes();
    int AdjustLimit(int OriginalLimit,
        int inLimitType) ;
    int AdjustIndexToSignal(int index) ;
    INT_FLOAT NormalizingValue() ;

    void ComputeLocalThresholdWithNoise(int curPos,
        int bunchSize,
        int searchWidth,
        INT_FLOAT& Thresh,
        INT_FLOAT& Noise);
    INT_FLOAT ComputeLocalThreshold(int curPos,
        int bunchSize,
        int searchWidth);

    void HandleFusedPeaks();
    bool CurrentPeakIsGood(TParameterState& paramState);
    int FindLocalExtrema(int startIndex,
        int endIndex,
        bool searchTheMin);

    void CreateResultPeaks();

    void CreateResultPeaks_old();
    void CreateDefaultBaseLines();
    void CreateEventBaselines(TTimeAndValue& SkimRatios);
    TEventAndPeaks* AssociateEventsAndPeaks(int idxEvent,
        TIntegrationEventList& EventList);
    void ProcessEventAndPeaks(TEventAndPeaks& assoc,
        TTimeAndValue& SkimRatios);
    void ProcessHorizontalBaseLine(TEventAndPeaks& assoc,
        bool goForward,
        bool byPeak);
    void ProcessCommonBaseLine(TEventAndPeaks& assoc,
        bool StartIsbyPeak,
        bool EndIsByPeak);
    void ProcessAddPeaks(TEventAndPeaks& assoc);
    void ProcessBaseLineNow(TEventAndPeaks& assoc);
    void ProcessValleyToValley(TEventAndPeaks& assoc);
    void ProcessSkimming(TEventAndPeaks& assoc,
        TBaseLineType linetype);
    void ProcessSkimming_FrontOrRear(TEventAndPeaks& assoc,
        bool rear,
        TTimeAndValue& SkimRatios,
        TBaseLineType linetype);
    void CleanCandidateLines();
    void CorrectSkimming();
    void CreateResultLines();
    void CorrectPeaksOfHorizLine(TCandidateBaseLine& baseline,
        TCandidatePeakList& AssociatedPeaks);
    void  AddCandidateBaseLine(TCandidateBaseLine& NewLine,
        int leftLimit,
        int rightLimit,
        int level);
    void AddCandidatePeak(TCandidatePeak& peak);
    void PropagatePeakDeletion(TCandidatePeak& curCandidPeak,
        bool UpdateIndexes);

    void ImproveLine(TCandidateBaseLine& newLine,
        int intersecStart,
        int intersecEnd,
        int level);
    void BreakTheCandidateLine(TCandidateBaseLine& NewLine,
        int leftlimitOrig,
        int rightlimitOrig,
        int level);

    bool ThereIsaPeakStart( TPeakDetector& Detector,
        TParameterState& paramState,
        TAlgoState& algoState,
        TPeakState& peakState,
        INT_FLOAT curSample);

    bool ThereIsaPeakEnd( TPeakDetector& Detector,
        TParameterState& paramState,
        TPeakState& peakState);

    bool ThereIsaPeakApex( TPeakDetector& Detector,
        TParameterState& paramState,
        TPeakState& peakState);

    bool ThereIsaValley( TPeakDetector& Detector,
        TParameterState& paramState,
        TPeakState& peakState);

    bool LastTwoPeaksHaveSameSign();
#ifdef TESTING
    static void Test();
#endif
};

void SmoothWithWaveletsProc(INT_SIGNAL Signal,
    int SignalSize,
    INT_SIGNAL SignalSmooth,
    INT_FLOAT ThresFactor,
    INT_FLOAT&  NoiseStdDev);

void FindConvexHalfHull(INT_SIGNAL Signal,
    int SignalSize,
    int idxOrigin,
    int idxExtr,
    bool bottom,
    INT_INDEXLIST& HalfHull);

#endif
