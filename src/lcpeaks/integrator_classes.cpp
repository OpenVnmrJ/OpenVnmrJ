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

// integrator_classes.cpp
//-------------------------------------------------------------
// Implementation of classes used by the main integrator object
//-------------------------------------------------------------
// $History: integrator_classes.cpp $
/*  */
/* *****************  Version 15  ***************** */
/* User: Go           Date: 8/11/02    Time: 17:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* C++ translation of the integration algorithm.  */
/* All main translation bugs fixed after having performed the official */
/* integration tests. */
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
/* *****************  Version 10  ***************** */
/* User: Go           Date: 9/07/02    Time: 14:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 9  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/*  */
/* *****************  Version 8  ***************** */
/* User: Go           Date: 24/06/02   Time: 11:30 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 12/06/02   Time: 10:34 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 3/06/02    Time: 15:29 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 23/05/02   Time: 18:02 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* BACKUP */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 30/04/02   Time: 9:13 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 12/04/02   Time: 11:01 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include <iostream>
#include <assert.h>
#include <cmath>
#include "integrator_types.h"
#include "integrator_classes.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#ifdef INTEGR_DEBUG
int Glob_InPeaksCount = 0;
int Glob_InBaseLineCount = 0;
int Glob_InPeaksTotal = 0;
int Glob_InBaseLineTotal = 0;
int Glob_InEventsCount = 0;
int Glob_InEventsTotal = 0;
int Glob_FinalizCount = 0;
#endif

/////////////////// TIntegrationEvent /////////////////////

int operator<(const TIntegrationEvent& left,
    const TIntegrationEvent& right)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return left.Time() < right.Time() ;
}
int operator>(const TIntegrationEvent& left,
    const TIntegrationEvent& right)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return left.Time() > right.Time() ;
}

TIntegrationEvent::TIntegrationEvent(	char *name,
    char *short_name,
    INT_TIME time,
    long code,
    int priority,
    TIntegrationEventCategory category)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(strlen(name) < INT_MAX_STRING_LENGTH);
    assert(strlen(short_name) < INT_MAX_SHORT_STRING_LENGTH);
    Name(name);
    Time(time);
    Code(code);
    m_priority = priority;
    ShortName(short_name);
    Category(category);
#ifdef INTEGR_DEBUG
    ++Glob_InEventsCount;
    ++Glob_InEventsTotal;
#endif
}

TIntegrationEvent::TIntegrationEvent( const TIntegrationEvent &source )
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Name(source.Name());
    Time(source.Time());
    Code(source.Code());
    m_priority = source.Priority();
    ShortName(source.ShortName());
    Category(source.Category());
#ifdef INTEGR_DEBUG
    ++Glob_InEventsCount;
    ++Glob_InEventsTotal;
#endif
}

TIntegrationEvent::~TIntegrationEvent()
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
#ifdef INTEGR_DEBUG
    --Glob_InEventsCount;
#endif
}

void TIntegrationEvent::initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue)
/* --------------------------
     * Author  :Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    m_time = time;
}

TIntegrationEvent& TIntegrationEvent::operator=(const TIntegrationEvent& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if( this != &source )
    {
        Name(source.Name());
        Time(source.Time());
        Code(source.Code());
        m_priority = source.Priority();
        ShortName(source.ShortName());
        Category(source.Category());
    }

    return *this;
}

/////////////////// TIntegrationEvent_OnOff /////////////////////

void TIntegrationEvent_OnOff::initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TIntegrationEvent::initialize(time, fvalue, bvalue);
    m_on = bvalue ;
}

TIntegrationEvent_OnOff::TIntegrationEvent_OnOff( char *name,
    char *short_name,
    INT_TIME time,
    bool on_now,
    long code,
    int priority,
    TIntegrationEventCategory category)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(name, short_name, time, code, priority, category), m_on(on_now)
{
}

TIntegrationEvent_OnOff::TIntegrationEvent_OnOff(TIntegrationEvent_OnOff& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(source)
{
    m_on = source.On();
}

TIntegrationEvent_OnOff& TIntegrationEvent_OnOff::operator=(const TIntegrationEvent_OnOff& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if( this != &source )
    {
        TIntegrationEvent::operator=(source);
        m_on = source.On();
    }
    return *this;
}

TIntegrationEvent *TIntegrationEvent_OnOff::CloneSelf()
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return new TIntegrationEvent_OnOff(*this);
}

/////////////////// TIntegrationEvent_Now /////////////////////

TIntegrationEvent_Now::TIntegrationEvent_Now(	char *name,
    char *short_name,
    INT_TIME time,
    long code,
    int priority,
    TIntegrationEventCategory category)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(name, short_name, time, code, priority, category)
{
}

TIntegrationEvent_Now::TIntegrationEvent_Now(TIntegrationEvent_Now& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(source)
{}

TIntegrationEvent_Now& TIntegrationEvent_Now::operator=(const TIntegrationEvent_Now& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if( this != &source )
    {
        TIntegrationEvent::operator=(source);
    }
    return *this;
}

TIntegrationEvent *TIntegrationEvent_Now::CloneSelf()
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return new TIntegrationEvent_Now(*this);
}

/////////////////// TIntegrationEvent_Duration /////////////////////

void TIntegrationEvent_Duration::initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TIntegrationEvent::initialize(time,fvalue,bvalue);
    m_endTime = fvalue ;
}

TIntegrationEvent_Duration::TIntegrationEvent_Duration( char *name,
    char *short_name,
    INT_TIME start_time,
    INT_TIME end_time,
    long code,
    int priority,
    TIntegrationEventCategory category)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
 *           Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(name, short_name, start_time, code, priority, category), m_endTime(end_time)
{
}

TIntegrationEvent_Duration::TIntegrationEvent_Duration( const TIntegrationEvent_Duration& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(source)
{
    m_endTime = source.EndTime();
}

TIntegrationEvent_Duration& TIntegrationEvent_Duration::operator=(const TIntegrationEvent_Duration& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if( this != &source )
    {
        TIntegrationEvent::operator=(source);
        m_endTime = source.EndTime();
    }
    return *this;
}

TIntegrationEvent *TIntegrationEvent_Duration::CloneSelf()
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return new TIntegrationEvent_Duration(*this);
}

/////////////////// TIntegrationEvent_VALUE1 /////////////////////

void TIntegrationEvent_VALUE1::initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TIntegrationEvent::initialize(time,fvalue,bvalue);
    m_userValue = fvalue ;
}

TIntegrationEvent_VALUE1::TIntegrationEvent_VALUE1( char *name,
    char *short_name,
    INT_TIME time,
    INT_FLOAT value,
    long code,
    int priority,
    TIntegrationEventCategory category)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(name, short_name, time, code, priority, category), m_userValue(value)
{
}

TIntegrationEvent_VALUE1::TIntegrationEvent_VALUE1( const TIntegrationEvent_VALUE1& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
: TIntegrationEvent(source)
{
    m_userValue = source.UserValue();
}

TIntegrationEvent_VALUE1& TIntegrationEvent_VALUE1::operator=(const TIntegrationEvent_VALUE1& source)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if( this != &source )
    {
        TIntegrationEvent::operator=(source);
        m_userValue = source.UserValue();
    }
    return *this;
}

TIntegrationEvent *TIntegrationEvent_VALUE1::CloneSelf()
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return new TIntegrationEvent_VALUE1(*this);
}

bool SortEventsByTime(TIntegrationEvent& evt1, TIntegrationEvent& evt2)
/* --------------------------
     * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT t1 = evt1.Time();
    INT_FLOAT t2 = evt2.Time();
    return (t1 < t2);
}
////////////////////////////////////////////////////////////////
///////////////////   TEST PROCEDURES   ////////////////////////
////////////////////////////////////////////////////////////////

#ifdef TESTING

void TIntegrationEvent::Test()
/* --------------------------
     * Author  : Diego Segura
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// Construction test
    TIntegrationEvent evt("test event", "evt", 1.5, 1, 5, IEC_FORCEDPEAKS);

    assert( strcmp(evt.Name(), "test event") == 0 );
    assert( strcmp(evt.ShortName(), "evt") == 0 );
    assert( evt.Time() == 1.5 );
    assert( evt.Code() == 1 );
    assert( evt.Priority() == 5 );
    assert( evt.Category() == IEC_FORCEDPEAKS );

// Copy construction test
    TIntegrationEvent evt2(evt);

    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );

// CloneSelf test
    TIntegrationEvent *evt3 = evt2.CloneSelf();

    assert( strcmp(evt3->Name(), evt2.Name()) == 0 );
    assert( strcmp(evt3->ShortName(), evt2.ShortName()) == 0 );
    assert( evt3->Time() == evt2.Time() );
    assert( evt3->Code() == evt2.Code() );
    assert( evt3->Priority() == evt2.Priority() );
    assert( evt3->Category() == evt2.Category() );

    delete evt3;

// Affectation
    evt.Name("other name");
    evt.ShortName("oth");
    evt.Time(5.0);
    evt.Code(3);
    evt.Category(IEC_SKIMMING);

    evt2 = evt;
    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );

    cout << "TIntegrationEvent class works fine !" << endl;
}

void TIntegrationEvent_OnOff::Test()
/* --------------------------
     * Author  : Diego Segura
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// Construction test
    TIntegrationEvent_OnOff evt("test event", "evt", 1.5, true, 1, 5, IEC_FORCEDPEAKS);

    assert( strcmp(evt.Name(), "test event") == 0 );
    assert( strcmp(evt.ShortName(), "evt") == 0 );
    assert( evt.Time() == 1.5 );
    assert( evt.Code() == 1 );
    assert( evt.Priority() == 5 );
    assert( evt.Category() == IEC_FORCEDPEAKS );
    assert( evt.On() == true);

// Copy construction test
    TIntegrationEvent_OnOff evt2(evt);

    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );
    assert( evt2.On() == evt.On());

// CloneSelf test
    TIntegrationEvent *evt3 = evt2.CloneSelf();

    assert( strcmp(evt3->Name(), evt2.Name()) == 0 );
    assert( strcmp(evt3->ShortName(), evt2.ShortName()) == 0 );
    assert( evt3->Time() == evt2.Time() );
    assert( evt3->Code() == evt2.Code() );
    assert( evt3->Priority() == evt2.Priority() );
    assert( evt3->Category() == evt2.Category() );
    assert( ((TIntegrationEvent_OnOff *)evt3)->On() == evt2.On());

    delete evt3;

// Affectation
    evt.Name("other name");
    evt.ShortName("oth");
    evt.Time(5.0);
    evt.Code(3);
    evt.Category(IEC_SKIMMING);
    evt.On(false);

    evt2 = evt;
    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );
    assert( evt2.On() == evt.On());

    cout << "TIntegrationEvent_OnOff class works fine !" << endl;
}

void TIntegrationEvent_Now::Test()
/* --------------------------
     * Author  : Diego Segura
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// Construction test
    TIntegrationEvent_Now evt("test event", "evt", 1.5, 1, 5, IEC_FORCEDPEAKS);

    assert( strcmp(evt.Name(), "test event") == 0 );
    assert( strcmp(evt.ShortName(), "evt") == 0 );
    assert( evt.Time() == 1.5 );
    assert( evt.Code() == 1 );
    assert( evt.Priority() == 5 );
    assert( evt.Category() == IEC_FORCEDPEAKS );

// Copy construction test
    TIntegrationEvent_Now evt2(evt);

    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );

// CloneSelf test
    TIntegrationEvent *evt3 = evt2.CloneSelf();

    assert( strcmp(evt3->Name(), evt2.Name()) == 0 );
    assert( strcmp(evt3->ShortName(), evt2.ShortName()) == 0 );
    assert( evt3->Time() == evt2.Time() );
    assert( evt3->Code() == evt2.Code() );
    assert( evt3->Priority() == evt2.Priority() );
    assert( evt3->Category() == evt2.Category() );

    delete evt3;

// Affectation
    evt.Name("other name");
    evt.ShortName("oth");
    evt.Time(5.0);
    evt.Code(3);
    evt.Category(IEC_SKIMMING);

    evt2 = evt;
    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );

    cout << "TIntegrationEvent_Now class works fine !" << endl;
}

void TIntegrationEvent_Duration::Test()
/* --------------------------
     * Author  : Diego Segura
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// Construction test
    TIntegrationEvent_Duration evt("test event", "evt", 1.5, 10.6, 1, 5, IEC_FORCEDPEAKS);

    assert( strcmp(evt.Name(), "test event") == 0 );
    assert( strcmp(evt.ShortName(), "evt") == 0 );
    assert( evt.Time() == 1.5 );
    assert( evt.Code() == 1 );
    assert( evt.Priority() == 5 );
    assert( evt.Category() == IEC_FORCEDPEAKS );
    assert( evt.EndTime() == 10.6);

// Copy construction test
    TIntegrationEvent_Duration evt2(evt);

    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );
    assert( evt2.EndTime() == evt.EndTime());

// CloneSelf test
    TIntegrationEvent *evt3 = evt2.CloneSelf();

    assert( strcmp(evt3->Name(), evt2.Name()) == 0 );
    assert( strcmp(evt3->ShortName(), evt2.ShortName()) == 0 );
    assert( evt3->Time() == evt2.Time() );
    assert( evt3->Code() == evt2.Code() );
    assert( evt3->Priority() == evt2.Priority() );
    assert( evt3->Category() == evt2.Category() );
    assert( ((TIntegrationEvent_Duration *)evt3)->EndTime() == evt2.EndTime());

    delete evt3;

// Affectation
    evt.Name("other name");
    evt.ShortName("oth");
    evt.Time(5.0);
    evt.Code(3);
    evt.Category(IEC_SKIMMING);
    evt.EndTime(25.1);

    evt2 = evt;
    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );
    assert( evt2.EndTime() == evt.EndTime());

    cout << "TIntegrationEvent_Duration class works fine !" << endl;
}

void TIntegrationEvent_VALUE1::Test()
/* --------------------------
     * Author  : Diego Segura
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// Construction test
    TIntegrationEvent_VALUE1 evt("test event", "evt", 1.5, 123.123, 1, 5, IEC_FORCEDPEAKS);

    assert( strcmp(evt.Name(), "test event") == 0 );
    assert( strcmp(evt.ShortName(), "evt") == 0 );
    assert( evt.Time() == 1.5 );
    assert( evt.Code() == 1 );
    assert( evt.Priority() == 5 );
    assert( evt.Category() == IEC_FORCEDPEAKS );
    assert( evt.UserValue() == 123.123);

// Copy construction test
    TIntegrationEvent_VALUE1 evt2(evt);

    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );
    assert( evt2.UserValue() == evt.UserValue());

// CloneSelf test
    TIntegrationEvent *evt3 = evt2.CloneSelf();

    assert( strcmp(evt3->Name(), evt2.Name()) == 0 );
    assert( strcmp(evt3->ShortName(), evt2.ShortName()) == 0 );
    assert( evt3->Time() == evt2.Time() );
    assert( evt3->Code() == evt2.Code() );
    assert( evt3->Priority() == evt2.Priority() );
    assert( evt3->Category() == evt2.Category() );
    assert( ((TIntegrationEvent_VALUE1 *)evt3)->UserValue() == evt2.UserValue());

    delete evt3;

// Affectation
    evt.Name("other name");
    evt.ShortName("oth");
    evt.Time(5.0);
    evt.Code(3);
    evt.Category(IEC_SKIMMING);
    evt.UserValue(1.6);

    evt2 = evt;
    assert( strcmp(evt2.Name(), evt.Name()) == 0 );
    assert( strcmp(evt2.ShortName(), evt.ShortName()) == 0 );
    assert( evt2.Time() == evt.Time() );
    assert( evt2.Code() == evt.Code() );
    assert( evt2.Priority() == evt.Priority() );
    assert( evt2.Category() == evt.Category() );
    assert( evt2.UserValue() == evt.UserValue());

    cout << "TIntegrationEvent_VALUE1 class works fine !" << endl;
}

#endif
