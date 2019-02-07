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

// File     : integrator_util_events.h
// Author   : Bruno Orsier : original code in Delphi
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration algorithm
//            Events handling classes
//
// $History: integrator_util_events.h $
/*  */
/* *****************  Version 8  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef __INTEGRATOR_UTIL_EVENTS_H
#define __INTEGRATOR_UTIL_EVENTS_H

#include <vector>
#include "integrator_types.h"
#include "integrator_classes.h"
#include "integrator_util_peaks.h"

using namespace std;

class  TEventAndPeaks {
// defines an association between a baseline event and a list of peaks
// located in the temporal range of this event
protected:
    INT_FLOAT FDeltaTime;
public:
    TIntegrationEvent*  pFEvent;
    INT_FLOAT          FEndTime;
    TCandidatePeakList FPeaks;
    INT_FLOAT          FStartTime;

    TEventAndPeaks( TIntegrationEvent& event,
        INT_FLOAT DeltaTime,
        INT_FLOAT StartTime);
    virtual ~TEventAndPeaks() {};
    virtual void GetAssociatedPeaks(TIntegrationEventList& EventList,
        TCandidatePeakOwningList& candidPeaks) {};
};

bool SortAssocsByPriority(TEventAndPeaks* Item1, TEventAndPeaks* Item2) ;

class TEventOnOffAndPeaks : public TEventAndPeaks {
protected:
    bool FAnyOtherWillClose;
    int FidxEvent;
    void CollectPeaks(INT_FLOAT time1,
        INT_FLOAT time2,
        TCandidatePeakOwningList& candidPeaks);
public:
    TEventOnOffAndPeaks(int idxEvent,
        TIntegrationEvent_OnOff& event,
        INT_FLOAT deltatime);
    virtual ~TEventOnOffAndPeaks() {} ;

    virtual INT_FLOAT FindClosingTime(TIntegrationEventList& EventList );
    virtual void GetAssociatedPeaks(TIntegrationEventList& EventList,
        TCandidatePeakOwningList& candidPeaks);
    bool CompatibleEvents(int Code1,
        int Code2);
};

class TEventNowAndPeaks : public TEventAndPeaks {
protected:
    void CollectOnePeak(TCandidatePeakOwningList& candidPeaks);
public:
    TEventNowAndPeaks(TIntegrationEvent_Now& event,
        INT_FLOAT deltatime);
    virtual  ~TEventNowAndPeaks() {}

    virtual void GetAssociatedPeaks(TIntegrationEventList& EventList,
        TCandidatePeakOwningList& CandidPeaks);
};

class  TEventAndPeaks_CommonLine : public TEventOnOffAndPeaks {
public:
    bool FStartIsByPeak;
    bool FEndIsByPeak;
    INT_FLOAT FindClosingTime(TIntegrationEventList& EventList);
    TEventAndPeaks_CommonLine(int idxEvent,
        TIntegrationEvent_OnOff& event,
        INT_FLOAT deltatime);
    virtual ~TEventAndPeaks_CommonLine() {} ;
};

class TEventAndPeaks_Skimming : public TEventOnOffAndPeaks {
public:
    TEventAndPeaks_Skimming(int idxEvent,
        TIntegrationEvent_OnOff& event,
        INT_FLOAT deltatime):
    TEventOnOffAndPeaks(idxEvent,event,deltatime){ };

    virtual ~TEventAndPeaks_Skimming() {} ;

    INT_FLOAT FindClosingTime(TIntegrationEventList& EventList);
};

typedef vector<TEventAndPeaks*> TEventAndPeaksList ;
//typedef vector<TEventAndPeaks> TEventAndPeaksOwningList ;
//WARNING: cannot declare vectors of abstract types !!!

class TTimeAndValue {
protected:
    typedef vector<INT_FLOAT> FValuesList ;
    typedef vector<INT_TIME> FTimeList ;

    FValuesList
    FValues;
    FTimeList
    FTime;

public:
    TTimeAndValue(INT_FLOAT V0);
    ~TTimeAndValue() {} ;
    void Add(const INT_TIME& t, const INT_FLOAT& v);
    INT_FLOAT ValueAt(const INT_TIME& t);
};

#endif
