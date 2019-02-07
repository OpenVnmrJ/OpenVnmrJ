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

// $History: EventsReader.cpp $
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
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

#include "common.h"
#include "EventsReader.h"
#include <iostream>

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/* --------------------------
     * TEventReader::TEventReader
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Constructor
 * History :
 *-------------------------- */
TEventReader::TEventReader(TIntegrationEventOwningList* theEventList) {
    _EventList = theEventList ;
}

/* --------------------------
     * TEventReader::ReadEvents
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Read events from input_stream
 * History :
 *-------------------------- */
void TEventReader::ReadEvents(istream& input_stream) {
    while(ReadNextEvent(input_stream)) {} ;
}

/* --------------------------
     * TEventReader::operator<<
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Read events from input
 * History :
 *-------------------------- */
TEventReader& TEventReader::operator<<(istream& input) {
    ReadEvents(input);
    return *this;
}

/* --------------------------
     * TEventReader::ReadEvents
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose : Read the next event in the stream
 * History :
 *-------------------------- */
bool TEventReader::ReadNextEvent(istream& input) {
    string evtstr ;
    input>>evtstr ;
    bool result = input.good() ;
    cout << evtstr << "  --  " ;
    return result ;
}

