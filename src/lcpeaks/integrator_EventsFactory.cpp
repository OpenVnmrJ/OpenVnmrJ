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

// $History: integrator_EventsFactory.cpp $
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:56 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:57 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include "integrator_EventsFactory.h"
#include "integrator_classes.h"
#include <string.h>

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

TIntegrationEventsFactory::TIntegrationEventsFactory()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Constructor
 *           Build the prototypes of the handled events
 * History :
 * -------------------------- */
{
    FExistingEvents[INTEGRATIONEVENT_SETPEAKWIDTH] =
        new TIntegrationEvent_VALUE1((char *)"Set Peak Width",
        (char *)"SPW",
        0, 0,
        INTEGRATIONEVENT_SETPEAKWIDTH,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_SETTHRESHOLD] =
        new TIntegrationEvent_VALUE1((char *)"Set Threshold",
        (char *)"STH",
        0, 10,
        INTEGRATIONEVENT_SETTHRESHOLD,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT );

    FExistingEvents[INTEGRATIONEVENT_DBLEPEAKWIDTH] =
        new TIntegrationEvent_Now((char *)"Double Peak Width",
        (char *)"DPW",
        0,
        INTEGRATIONEVENT_DBLEPEAKWIDTH,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_HLVEPEAKWIDTH] =
        new TIntegrationEvent_Now((char *)"'Halve Peak Width",
        (char *)"HPW",
        0,
        INTEGRATIONEVENT_HLVEPEAKWIDTH,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_ESTIMTHRESHOLD] =
        new TIntegrationEvent_Now((char *)"Estimate Threshold",
        (char *)"ETH",
        0,
        INTEGRATIONEVENT_ESTIMTHRESHOLD,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_COMPUTENOISE] =
        new TIntegrationEvent_OnOff((char *)"Compute Noise",
        (char *)"CPN",
        0,
        true,
        INTEGRATIONEVENT_COMPUTENOISE,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_ADDTHRESHOLD] =
        new TIntegrationEvent_VALUE1((char *)"Add to Threshold",
        (char *)"ATH",
        0, 10,
        INTEGRATIONEVENT_ADDTHRESHOLD,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_MIN_HEIGHT] =
        new TIntegrationEvent_VALUE1((char *)"Set Minimal Height",
        (char *)"SMH",
        0, 100,
        INTEGRATIONEVENT_MIN_HEIGHT,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_MINVALUES);

    FExistingEvents[INTEGRATIONEVENT_MIN_AREA] =
        new TIntegrationEvent_VALUE1((char *)"Set Minimal Area",
        (char *)"SMA",
        0, 100,
        INTEGRATIONEVENT_MIN_AREA,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_MINVALUES);

    FExistingEvents[INTEGRATIONEVENT_MIN_HEIGHT_PERCENT] =
        new TIntegrationEvent_VALUE1((char *)"Set Minimal Height %",
        (char *)"SMH%",
        0, 0.1,
        INTEGRATIONEVENT_MIN_HEIGHT_PERCENT,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_MINVALUES);

    FExistingEvents[INTEGRATIONEVENT_MIN_AREA_PERCENT] =
        new TIntegrationEvent_VALUE1((char *)"Set Minimal Area %",
        (char *)"SMA%",
        0, 0.1,
        INTEGRATIONEVENT_MIN_AREA_PERCENT,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_MINVALUES);

    FExistingEvents[INTEGRATIONEVENT_INTEGRATION] =
        new TIntegrationEvent_OnOff((char *)"Turn Integration",
        (char *)"TI",
        0, false,
        INTEGRATIONEVENT_INTEGRATION,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_FORCEDPEAKS);

    FExistingEvents[INTEGRATIONEVENT_STARTPEAK_NOW]=
        new TIntegrationEvent_Now((char *)"Start Peak Now",
        (char *)"SPN",
        0,
        INTEGRATIONEVENT_STARTPEAK_NOW,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_FORCEDPEAKS);

    FExistingEvents[INTEGRATIONEVENT_ENDPEAK_NOW]=
        new TIntegrationEvent_Now((char *)"Stop Peak Now",
        (char *)"EPN",
        0,
        INTEGRATIONEVENT_ENDPEAK_NOW,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_FORCEDPEAKS);

    FExistingEvents[INTEGRATIONEVENT_SPLITPEAK_NOW] =
        new TIntegrationEvent_Now((char *)"Split Peak Now",
        (char *)"TPN",
        0,
        INTEGRATIONEVENT_SPLITPEAK_NOW,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_FORCEDPEAKS);

    FExistingEvents[INTEGRATIONEVENT_SLICE_INTEGRATION] =
        new TIntegrationEvent_OnOff(
        (char *)"Slice Integration",
        (char *)"SLI", 
        0, true,
        INTEGRATIONEVENT_SLICE_INTEGRATION,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

    FExistingEvents[INTEGRATIONEVENT_DETECTNEGPEAKS] =
        new TIntegrationEvent_OnOff((char *)"Detect Negative Peaks",
        (char *)"DNP",
        0, true,
        INTEGRATIONEVENT_DETECTNEGPEAKS,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_NEGPEAK);

    FExistingEvents[INTEGRATIONEVENT_ADDPEAKS] =
        new TIntegrationEvent_OnOff((char *)"Add Peaks",
        (char *)"ADP",
        0, true,
        INTEGRATIONEVENT_ADDPEAKS,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_FORCEDPEAKS);

    FExistingEvents[INTEGRATIONEVENT_VALTOVAL] =
        new TIntegrationEvent_OnOff((char *)"Baseline Valley-to-Valley",
        (char *)"VTV",
        0, true,
        INTEGRATIONEVENT_VALTOVAL,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_HORIZLINE] =
        new TIntegrationEvent_OnOff((char *)"Horizontal Baseline",
        (char *)"HB",
        0, true,
        INTEGRATIONEVENT_HORIZLINE,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_HORIZLINE_BYPEAK] =
        new TIntegrationEvent_OnOff((char *)"Horizontal Baseline by peak",
        (char *)"HBP",
        0, true,
        INTEGRATIONEVENT_HORIZLINE_BYPEAK,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_HORIZLINE_BACK] =
        new TIntegrationEvent_OnOff((char *)"Backward Horizontal Baseline",
        (char *)"BHB",
        0, true,
        INTEGRATIONEVENT_HORIZLINE_BACK,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_HORIZLINE_BACK_BYPEAK] =
        new TIntegrationEvent_OnOff((char *)"Backward Horizontal Baseline by peak",
        (char *)"BHBP",
        0, true,
        INTEGRATIONEVENT_HORIZLINE_BACK_BYPEAK,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_COMMONLINE] =
        new TIntegrationEvent_OnOff((char *)"Force Baseline",
        (char *)"FB",
        0, true,
        INTEGRATIONEVENT_COMMONLINE,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_COMMONLINE_BYPEAK] =
        new TIntegrationEvent_OnOff((char *)"Force Baseline by peak",
        (char *)"FBP",
        0, true,
        INTEGRATIONEVENT_COMMONLINE_BYPEAK,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_BASELINE_NOW] =
        new TIntegrationEvent_Now((char *)"Baseline Now",
        (char *)"BN",
        0,
        INTEGRATIONEVENT_BASELINE_NOW,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_BASELINE_NEXTVALLEY] =
        new TIntegrationEvent_Now((char *)"Baseline Next Valley",
        (char *)"BNV",
        0,
        INTEGRATIONEVENT_BASELINE_NEXTVALLEY,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_BASELINE);

    FExistingEvents[INTEGRATIONEVENT_SETSKIMRATIO] =
        new TIntegrationEvent_VALUE1((char *)"Set Skim Ratio",
        (char *)"SKR",
        0, 4,
        INTEGRATIONEVENT_SETSKIMRATIO,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_TANGENTSKIM] =
        new TIntegrationEvent_OnOff((char *)"Tangent Skim Next Peaks",
        (char *)"TSNP",
        0, true,
        INTEGRATIONEVENT_TANGENTSKIM,
        INTEGRATION_PRIORITY_LOW,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_TANGENTSKIM_REAR] =
        new TIntegrationEvent_OnOff((char *)"Tangent Skim Rear",
        (char *)"TSR",
        0, true,
        INTEGRATIONEVENT_TANGENTSKIM_REAR,
        INTEGRATION_PRIORITY_LOW,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_TANGENTSKIM_FRONT] =
        new TIntegrationEvent_OnOff((char *)"Tangent Skim Front",
        (char *)"TSF",
        0, true,
        INTEGRATIONEVENT_TANGENTSKIM_FRONT,
        INTEGRATION_PRIORITY_LOW,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_TANGENTSKIM_EXP] =
        new TIntegrationEvent_OnOff((char *)"Exponential Skim Next Peaks",
        (char *)"ESNP",
        0,true,
        INTEGRATIONEVENT_TANGENTSKIM_EXP,
        INTEGRATION_PRIORITY_LOW,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_TANGENTSKIM_REAR_EXP] =
        new TIntegrationEvent_OnOff((char *)"Exponential Skim Rear",
        (char *)"ESR",
        0, true,
        INTEGRATIONEVENT_TANGENTSKIM_REAR_EXP,
        INTEGRATION_PRIORITY_LOW,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_TANGENTSKIM_FRONT_EXP] =
        new TIntegrationEvent_OnOff((char *)"Exponential Skim Front",
        (char *)"ESF",
        0, true,
        INTEGRATIONEVENT_TANGENTSKIM_FRONT_EXP,
        INTEGRATION_PRIORITY_LOW,
        IEC_SKIMMING);

    FExistingEvents[INTEGRATIONEVENT_SETTHRESHOLD_SOLVENT] =
        new TIntegrationEvent_VALUE1((char *)"Set Solvent Threshold",
        (char *)"SST",
        0, 1000,
        INTEGRATIONEVENT_SETTHRESHOLD_SOLVENT,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);

}

TIntegrationEventsFactory::~TIntegrationEventsFactory()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Destructor
 * History :
 * -------------------------- */
{
    for (TIntegrationEvent_map::iterator item  = FExistingEvents.begin();
        item != FExistingEvents.end();
        ++item)
    {
        delete (*item).second ;
    }
}

TIntegrationEvent* TIntegrationEventsFactory::CreateEvent(const long code)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Instanciates an event object
 * History :
 * -------------------------- */
{
    if (FExistingEvents.find(code)!= FExistingEvents.end())
    {
        return FExistingEvents[code]->CloneSelf() ;
    }
    else
        {
        return NULL ;
    }
}

TIntegrationEvent* TIntegrationEventsFactory::CreateEvent(const long code, INT_FLOAT time, INT_FLOAT fvalue, bool bvalue)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Instanciates an event object
 * History :
 * -------------------------- */
{
    TIntegrationEvent* newevent = CreateEvent(code);
    if (newevent != NULL) newevent->initialize(time,fvalue,bvalue);
    return newevent ;
}

int TIntegrationEventsFactory::NumberOfEvents() 
{
    return FExistingEvents.size() ;
}

void TIntegrationEventsFactory::DumpIntoEventList(INT_EVENT_INFO* EvtList)
{
    unsigned int nbevt = 0;
    for (TIntegrationEvent_map::iterator item  = FExistingEvents.begin();
        item != FExistingEvents.end();
        item++)
    {
        TIntegrationEvent* pCurEvent = (*item).second ;
        INT_EVENT_INFO& CurEventInfo = EvtList[nbevt] ;

        strcpy(CurEventInfo.name, pCurEvent->Name());
        strcpy(CurEventInfo.shortname, pCurEvent->ShortName());
        CurEventInfo.id = pCurEvent->Code();
        CurEventInfo.category = pCurEvent->Category();
        CurEventInfo.valuetype = pCurEvent->ValueType() ;

        if (pCurEvent->ValueType() == INTEGRATION_EVTVALTYPE_VALUE)
        {
            CurEventInfo.defaultvalue = ((TIntegrationEvent_VALUE1*)pCurEvent)->UserValue() ;
        } 
        else
        {
            CurEventInfo.defaultvalue = 0.0 ;
        }

        ++nbevt;
    }

}
