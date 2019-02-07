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

// $History: integrator_ioStructures.cpp $
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

#include "common.h"
#include "integrator_ioStructures.h"
#include "integrator_EventsFactory.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

char* Alloc_String(int NbChar)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose : String memory management
             Intended for strings initialized by
             the caller to setup the io structures.
 * History :
 * -------------------------- */
{
    return new char[NbChar] ;
}

void Free_String(char* astr)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    delete [] astr ;
}

INT_RESULTS* Alloc_Results(int NbLines, int NbPeaks, int NbAutoThreshold)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{ 
    //Memory allocation
    INT_RESULTS* Results = new INT_RESULTS ;
    Results->NumberOfPeaks = NbPeaks ;
    Results->NumberOfLines = NbLines ;
    Results->Peaks = new INT_RESULT_PEAK[NbPeaks] ;
    Results->Lines = new INT_RESULT_LINE[NbLines] ;
    Results->NumberOfThresholds = NbAutoThreshold ;
    Results->AutoThresholds = new INT_RESULT_AUTOTHRESHOLD[NbAutoThreshold];

    //Pointers intializations
    for (int idxpeak=0; idxpeak<NbPeaks; ++idxpeak)
        Results->Peaks[idxpeak].code = NULL ;

    return Results ;
}

void Free_Results(INT_RESULTS* res)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    for (int i=0; i<res->NumberOfPeaks; ++i) {
        delete [] res->Peaks[i].code ;
    }
    delete [] res->Peaks ;
    delete [] res->Lines ;
    delete [] res->AutoThresholds ;
    delete res ;
}

INT_PARAMETERS* Alloc_Parameters(int NbEvents)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_PARAMETERS* params = new INT_PARAMETERS ;
    params->NumberOfEvents = NbEvents ;
    if (NbEvents>0)
        params->events = new INT_EVENT[NbEvents] ;
    else
        params->events = NULL;
    return params ;
}

void Free_Parameters(INT_PARAMETERS* params)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    delete [] params->events ;
    delete params ;
}

void Integration_Event_List(INT_EVENT_INFO* &evtlist, int& nbevts) 
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose : Returns the list of available integration events
 * History :
 * -------------------------- */
{
    TIntegrationEventsFactory evtFactory  ;
    nbevts  = evtFactory.NumberOfEvents() ;
    evtlist = new INT_EVENT_INFO[nbevts];
    for (int evtidx=0; evtidx < nbevts; ++evtidx)
    {
        INT_EVENT_INFO* pCurEventInfo = evtlist+evtidx ;
        pCurEventInfo->name      = new char[INT_MAX_STRING_LENGTH] ;
        pCurEventInfo->shortname = new char[INT_MAX_SHORT_STRING_LENGTH];
    }
    evtFactory.DumpIntoEventList(evtlist);

}

void Free_Event_List(INT_EVENT_INFO* evtlist, const int nbevts)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose : Free the structure returned by Integration_Event_List
 * History :
 * -------------------------- */
{
    for (int evtidx=0; evtidx<nbevts; ++evtidx)
    {
        INT_EVENT_INFO& CurEventInfo = evtlist[evtidx] ;
        delete [] CurEventInfo.name  ;
        delete [] CurEventInfo.shortname ;
    }
    delete [] evtlist ;
}

