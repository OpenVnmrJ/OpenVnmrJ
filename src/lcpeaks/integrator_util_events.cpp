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

// $History: integrator_util_events.cpp $
/*  */
/* *****************  Version 11  ***************** */
/* User: Go           Date: 8/11/02    Time: 17:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* C++ translation of the integration algorithm.  */
/* All main translation bugs fixed after having performed the official */
/* integration tests. */
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
/* User: Go           Date: 9/07/02    Time: 14:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include "integrator_util_events.h"
#include "integrator_types.h"
#include <assert.h>
#include "owninglist.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif


#define  TIMEANDVALUE_INITIAL_SIZE  20 ;
#define  TIMEANDVALUE_AUGMENT_SIZE  20 ;
#define  LARGEST_POSSIBLE_TIME      1.0E5 ; // any max value for the temporal range
// can not be Maxdouble because this would
// cause a floating-point exception later
// 1.0E10 not OK for the same reason

//---------------------------------------------------------------------

TEventNowAndPeaks::TEventNowAndPeaks(TIntegrationEvent_Now& event,
    INT_FLOAT deltatime):
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
TEventAndPeaks( event, deltatime, event.Time() )
{
}

//---------------------------------------------------------------------

void TEventNowAndPeaks::CollectOnePeak(TCandidatePeakOwningList& CandidPeaks)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// collect one or zero peak
// the peak limits must contain the desired index
// peaks are supposed to be sorted by time

    INT_FLOAT index1 = FStartTime/FDeltaTime ;

    assert(FPeaks.size() == 0 ) ;

    bool exitloop = false;
    for(int peakidx=0; peakidx<CandidPeaks.size(); ++peakidx)
    {
        TCandidatePeak& curPeak = CandidPeaks[peakidx] ;
        if ( (curPeak.StartIndex < index1) && (curPeak.EndIndex > index1) )
        {
            FPeaks.push_back(&curPeak) ;
            exitloop = true ;
        }
        else
            {
            exitloop = (curPeak.StartIndex > index1) ;
        }//if
    }//for
}

//---------------------------------------------------------------------

void TEventNowAndPeaks::GetAssociatedPeaks(TIntegrationEventList& EventList,
    TCandidatePeakOwningList& CandidPeaks)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    CollectOnePeak(CandidPeaks) ;
}

//---------------------------------------------------------------------

TEventAndPeaks_CommonLine::TEventAndPeaks_CommonLine(int idxEvent,
    TIntegrationEvent_OnOff& event,
    INT_FLOAT deltatime) :
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
TEventOnOffAndPeaks(idxEvent, event, deltatime)
{
    FStartIsByPeak = (event.Code() == INTEGRATIONEVENT_COMMONLINE_BYPEAK) ;
    FEndIsByPeak   = false ;
}

//---------------------------------------------------------------------

INT_FLOAT TEventAndPeaks_CommonLine::FindClosingTime(TIntegrationEventList& EventList)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT time = INT_MINFLOAT ;

    for (unsigned int idxEvent = FidxEvent+1 ;
        (idxEvent<EventList.size()) && (time<0);
        ++idxEvent)
        {

        TIntegrationEvent* pOtherEvent = EventList[idxEvent] ;
        if ((pOtherEvent->Code() == pFEvent->Code()) ||
            ( (pOtherEvent->Code() == INTEGRATIONEVENT_COMMONLINE_BYPEAK) &&
            (pFEvent->Code()     == INTEGRATIONEVENT_COMMONLINE) ) ||
            ( (pOtherEvent->Code() == INTEGRATIONEVENT_COMMONLINE) &&
            (pFEvent->Code()     == INTEGRATIONEVENT_COMMONLINE_BYPEAK) ))
        {
            if (FAnyOtherWillClose)
            { 
                time = pOtherEvent->Time() ;
            }
            else
                {
//What a typecast !! Can we do something simpler ?
                TIntegrationEvent_OnOff* otherEvent_OnOff = (TIntegrationEvent_OnOff*) pOtherEvent ;
                if ( ! otherEvent_OnOff->On() )
                {
                    time = pOtherEvent->Time() ;
                }
            }
        }

        if (time > 0)
//in pascal original source test if otherevent is assigned.
//not tested here because it seems to be unuseful.
            {
            FEndIsByPeak = (pOtherEvent->Code() == INTEGRATIONEVENT_COMMONLINE_BYPEAK) ;
        }
    }
    return time ;
}  

//---------------------------------------------------------------------
INT_FLOAT TEventAndPeaks_Skimming::FindClosingTime(TIntegrationEventList& EventList)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT time = INT_MINFLOAT ;

    for (TIntegrationEventList::iterator it_otherEvent = EventList.begin()+FidxEvent+1;
    ( (it_otherEvent != EventList.end() ) && (time<0));
    it_otherEvent++)
    {
        TIntegrationEvent* pOtherEvent = *it_otherEvent ;
        if (pOtherEvent->Code() == pFEvent->Code())
        {
            if (FAnyOtherWillClose)
            {
                time = pOtherEvent->Time() ;
            }
            else
                {
//What a typecast !! Can we do something simpler ?
                TIntegrationEvent_OnOff* otherEvent_OnOff = (TIntegrationEvent_OnOff*) pOtherEvent ;
                if ( ! otherEvent_OnOff->On() )
                {
                    time = pOtherEvent->Time();
                }
            }
        }

        if (time < 0)
        {
//What a typecast !! Can we do something simpler ?
            TIntegrationEvent_OnOff* otherEvent_OnOff = (TIntegrationEvent_OnOff*) pOtherEvent ;
            if ( (pFEvent->Code() >= INTEGRATIONEVENT_TANGENTSKIM) &&
                (pFEvent->Code() <= INTEGRATIONEVENT_TANGENTSKIM_FRONT_EXP) &&
                (pOtherEvent->Code() >= INTEGRATIONEVENT_TANGENTSKIM) &&
                (pOtherEvent->Code() <= INTEGRATIONEVENT_TANGENTSKIM_FRONT_EXP) &&
                otherEvent_OnOff->On() )
            {
                time = pOtherEvent->Time() ;
            }
        }
    }

    return time ;
}

//---------------------------------------------------------------------

TTimeAndValue::TTimeAndValue(INT_FLOAT V0)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Add(0,V0) ;
}

//---------------------------------------------------------------------

void TTimeAndValue::Add(const INT_TIME& t, const INT_FLOAT& v)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FTime.push_back(t);
    FValues.push_back(v);
}

//---------------------------------------------------------------------

INT_FLOAT TTimeAndValue::ValueAt(const INT_TIME& t)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(t>=0);

    int i = 0 ;
    bool stop = false ;

    for (FValuesList::iterator time_it = FTime.begin();
    (time_it != FTime.end()) || stop;
    time_it++ )
    {
        INT_FLOAT& curTime = *time_it ;
        stop = (t < curTime) ;
        i++;
    }

    INT_FLOAT result ;
    if (stop)
        result = FValues[i-2];
    else
        result = FValues[i-1];
    return result ;
}

//---------------------------------------------------------------------

TEventAndPeaks::TEventAndPeaks( TIntegrationEvent& event,
    INT_FLOAT DeltaTime,
    INT_FLOAT StartTime)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(DeltaTime  > 0 ) ;

    FStartTime = StartTime ;
    FEndTime = -1 ;
    FDeltaTime = DeltaTime ;
    pFEvent = &event ;
}

//---------------------------------------------------------------------

TEventOnOffAndPeaks::TEventOnOffAndPeaks(int idxEvent,
    TIntegrationEvent_OnOff& event,
    INT_FLOAT deltatime):
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
TEventAndPeaks(event, deltatime, event.Time())
{
    FidxEvent = idxEvent ;

// for several kinds of event, any other On of the same kind
// will close the current event
    FAnyOtherWillClose =
        (event.Code() == INTEGRATIONEVENT_HORIZLINE)
    || (event.Code() == INTEGRATIONEVENT_HORIZLINE_BACK)
    || (event.Code() == INTEGRATIONEVENT_HORIZLINE_BYPEAK)
    || (event.Code() == INTEGRATIONEVENT_HORIZLINE_BACK_BYPEAK)
    || (event.Code() == INTEGRATIONEVENT_COMMONLINE)
    || (event.Code() == INTEGRATIONEVENT_COMMONLINE_BYPEAK)
    || (event.Code() == INTEGRATIONEVENT_ADDPEAKS) ;
}

//---------------------------------------------------------------------
void  TEventOnOffAndPeaks::CollectPeaks(INT_FLOAT time1,
    INT_FLOAT time2,
    TCandidatePeakOwningList& CandidPeaks)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT index1 = time1 / FDeltaTime ;
    INT_FLOAT index2 = time2 / FDeltaTime ;
    bool stop = false ;
    for(int peakidx=0; peakidx<CandidPeaks.size() && !stop ; ++peakidx)
    {
        TCandidatePeak& curPeak = CandidPeaks[peakidx] ;
        if (curPeak.EndIndex > index2)
            stop = true;
        if ( ((curPeak.ApexIndex >= index1) ||
            (curPeak.StartIndex >= index1))
            &&
            ((curPeak.EndIndex <= index2) ||
            (curPeak.ApexIndex <= index2)) )
        {
            FPeaks.push_back(&curPeak);
        }
    }
}

//---------------------------------------------------------------------

INT_FLOAT TEventOnOffAndPeaks::FindClosingTime(TIntegrationEventList& EventList )
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT time = INT_MINFLOAT ;
    for (TIntegrationEventList::iterator otherEvent_it = EventList.begin()+FidxEvent+1;
    (time<0) && (otherEvent_it != EventList.end());
    otherEvent_it++)
    {
        TIntegrationEvent* pOtherEvent = *otherEvent_it ;
        if ( (pOtherEvent->Code() == pFEvent->Code())
            || CompatibleEvents(pOtherEvent->Code(), pFEvent->Code()) )
        {
            if (FAnyOtherWillClose)
            {
                time = pOtherEvent->Time() ;
            }
            else
                {
                TIntegrationEvent_OnOff *otherEvent_OnOff = (TIntegrationEvent_OnOff*) pOtherEvent;
                if (! otherEvent_OnOff->On() )
                {
                    time = pOtherEvent->Time() ;
                }
            }
        }
    }
    return time;
}

//---------------------------------------------------------------------

void TEventOnOffAndPeaks::GetAssociatedPeaks(TIntegrationEventList& EventList,
    TCandidatePeakOwningList& candidPeaks)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FEndTime = FindClosingTime(EventList);
    if (FEndTime<0)
    {
        FEndTime = LARGEST_POSSIBLE_TIME ;
    }
    CollectPeaks(FStartTime, FEndTime, candidPeaks);
}

//---------------------------------------------------------------------

bool test1(int code)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool tmp =
        (code == INTEGRATIONEVENT_HORIZLINE) ||
        (code == INTEGRATIONEVENT_HORIZLINE_BYPEAK ) ;
    return tmp;
}

bool test2(int code)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool tmp =
        (code == INTEGRATIONEVENT_HORIZLINE_BACK) ||
        (code == INTEGRATIONEVENT_HORIZLINE_BACK_BYPEAK ) ;
    return tmp;
}

bool TEventOnOffAndPeaks::CompatibleEvents(int Code1,
    int Code2)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    bool tmp =
        test1(Code1) &&
        test1(Code2) &&
        test2(Code1) &&
        test2(Code2) ;
    return tmp ;
}

//---------------------------------------------------------------------

bool SortAssocsByPriority(TEventAndPeaks* Item1, TEventAndPeaks* Item2)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (Item1->pFEvent->Priority() == Item2->pFEvent->Priority())
    {
        return SortEventsByTime(*(Item1->pFEvent), *(Item2->pFEvent)); // both events have the same priority
    }
    else
        {
        return (Item1->pFEvent->Priority() > Item2->pFEvent->Priority());
        //Events with high priority should be processed first.
    }
}

//---------------------------------------------------------------------
