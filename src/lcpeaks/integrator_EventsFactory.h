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

// File     : integrator_EventsManager.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : Integration data structures
//            Factory to create integration event object
//
// $History: integrator_EventsFactory.h $
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
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef _INTEGRATOR_EVENTSFACTORY_H
#define _INTEGRATOR_EVENTSFACTORY_H

#include "integrator_ioStructures.h"
#include "integrator_classes.h"
#include <map>

typedef map<long, TIntegrationEvent*> TIntegrationEvent_map ;


/*
 * This class should maybe converted to a singleton
 */
class TIntegrationEventsFactory {
protected:
    TIntegrationEvent_map FExistingEvents ;
public:
    TIntegrationEventsFactory();
    ~TIntegrationEventsFactory();
    TIntegrationEvent* CreateEvent(const long code);
    TIntegrationEvent* CreateEvent(const long code, INT_TIME time, INT_FLOAT fvalue, bool bvalue);
    
    int NumberOfEvents() ;
    void DumpIntoEventList(INT_EVENT_INFO*); 
    //The array has to be allocated by the caller
};

#endif
