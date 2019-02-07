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

// File     : EventsReader.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : Integration utilitary class
//            Input of integration parameters from a stream
//            (Can be used to read a configuration file)
//
// $History: EventsReader.h $
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef _EVENTS_READER_H
#define _EVENTS_READER_H

#include "integrator_classes.h"
#ifdef __VISUAL_CPP__
#include <istream>
#else
#include <istream>
#endif

//The following class fills an event list from a text-formatted stream.
class TEventReader {
protected:
    TIntegrationEventOwningList* _EventList  ;
    bool ReadNextEvent(istream& input);
public:
    TEventReader(TIntegrationEventOwningList* theEventList);
    void ReadEvents(istream& input_stream);
    TEventReader& operator<<(istream& input);
};

#endif
