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

// File     : integrator_classes.h
// Author   : Bruno Orsier                : original code in Delphi
//            Diego Segura / Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration
//            Define the class which describe the integration
//            events, peaks and baselines and their related
//            constants
//
// $History: integrator_classes.h $
/*  */
/* *****************  Version 16  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 15  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 14  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 13  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 12  ***************** */
/* User: Go           Date: 9/07/02    Time: 14:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 11  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/*  */
/* *****************  Version 10  ***************** */
/* User: Go           Date: 25/06/02   Time: 8:59 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 9  ***************** */
/* User: Go           Date: 24/06/02   Time: 11:30 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 8  ***************** */
/* User: Go           Date: 12/06/02   Time: 10:34 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 5/06/02    Time: 11:57 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
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

#ifndef __INTEGRATOR_CLASSES_H
#define __INTEGRATOR_CLASSES_H

#include <string.h>
#include <vector>
#include <string>
#include "integrator_types.h"
#include "LineFitter.h"
#include "owninglist.h"
#include "integrator_ioStructures.h"
#include "integrator_events_code.h"

using namespace std;

#define   INTEGRATOR_OPTIMAL_POINTS_PER_PEAK  15
#define   INTEGR_EXPBL_COEFF  0.25
#define   INTEGR_NBPOINTS_TANGENTES  2 // nb points at the right and left of inflexion points that should be used for tangent fitting

//------

#define   INTEGRATION_COMPUT_RESULT_NOTDEFINED  0.0

// NB: priorities are now used only to separate
// skimming events from the rest. Thus only two priority levels
// are really necessary.
#define   INTEGRATION_PRIORITY_LOW      0
#define   INTEGRATION_PRIORITY_NORMAL   1
#define   INTEGRATION_PRIORITY_HIGH     2
#define   INTEGRATION_PRIORITY_HIGHEST  3

#define   INTEGRATION_CODE_DELIMITER    '|'

#define   INTEGRATION_VALLEY_CODE       'V'
#define   INTEGRATION_SIGNAL_CODE       'S'
#define   INTEGRATION_HORIZ_CODE        'H'
#define   INTEGRATION_FORCED_CODE       'F'
#define   INTEGRATION_TANGENT_CODE      'T'
#define   INTEGRATION_EXP_CODE          'E'
#define   INTEGRATION_NO_CODE           '-'
#define   INTEGRATION_MANUAL_CODE       'M'
#define   INTEGRATION_SATURATED_CODE    'OR'

#ifdef INTEGR_DEBUG
int
Glob_InPeaksCount,
Glob_InBaseLineCount,
Glob_InPeaksTotal,
Glob_InBaseLineTotal,
Glob_InEventsCount,
Glob_InEventsTotal,
Glob_FinalizCount;
#endif

typedef enum {INFLEXP_NOERROR=0} TInflexionPointErrorType;

class TIntegrationEvent {
protected:
    int m_priority;
    char m_name[INT_MAX_STRING_LENGTH];
    char m_short_name[INT_MAX_SHORT_STRING_LENGTH];
    INT_TIME m_time;
    long m_code;
    TIntegrationEventCategory m_category;

public:
    TIntegrationEvent(	char *name,
        char *short_name,
        INT_TIME time,
        long code,
        int priority,
        TIntegrationEventCategory category);
    TIntegrationEvent( const TIntegrationEvent& );
    TIntegrationEvent() {} ;
    virtual ~TIntegrationEvent();

    virtual void initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue) ;

    TIntegrationEvent& operator=(const TIntegrationEvent&);

    virtual TIntegrationEvent *CloneSelf() = 0;

    CLASS_READONLY_PROPERTY(Priority, int, return m_priority);
    CLASS_PROPERTY(Name, char *, return const_cast<char *>(m_name), strcpy(m_name, v));
    CLASS_PROPERTY(ShortName, char *, return const_cast<char *>(m_short_name), strcpy(m_short_name, v));
    CLASS_PROPERTY(Time, INT_TIME, return m_time, m_time = v );
    CLASS_PROPERTY(Code, long, return m_code, m_code = v );
    CLASS_PROPERTY(Category, TIntegrationEventCategory, return m_category, m_category = v );
    virtual int ValueType() = 0 ;

    //These 2 following functions are defined to use the 'sort' STL algorithm.
    friend int operator<(const TIntegrationEvent& left,
        const TIntegrationEvent& right);
    friend int operator>(const TIntegrationEvent& left,
        const TIntegrationEvent& right);

    //To use the STL sort algorithm with containers of pointers on objects
    struct lessTime : public binary_function<TIntegrationEvent*, TIntegrationEvent*, bool> {
        bool operator()(TIntegrationEvent* lhs, TIntegrationEvent* rhs) { return  (*lhs).Time()<(*rhs).Time() ;}
    };

#ifdef TESTING
    static void Test();
#endif
};

typedef vector<TIntegrationEvent*> TIntegrationEventList ;
typedef OwningList<TIntegrationEvent> TIntegrationEventOwningList ;

class TIntegrationEvent_OnOff : public TIntegrationEvent {
private:
    bool m_on;

public:
    TIntegrationEvent_OnOff(	
        char *name,
        char *short_name,
        INT_TIME time,
        bool on_now,
        long code,
        int priority,
        TIntegrationEventCategory category);
    TIntegrationEvent_OnOff( TIntegrationEvent_OnOff& );

    TIntegrationEvent_OnOff& operator=(const TIntegrationEvent_OnOff&);

    TIntegrationEvent *CloneSelf();

    virtual void initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue) ;

    int ValueType() {return INTEGRATION_EVTVALTYPE_ONOFF ;}

    CLASS_PROPERTY(On, bool, return m_on, m_on = v )

#ifdef TESTING
    static void Test();
#endif
};

class TIntegrationEvent_Now : public TIntegrationEvent {
public:
    TIntegrationEvent_Now(	char *name,
        char *short_name,
        INT_TIME time,
        long code,
        int priority,
        TIntegrationEventCategory category);
    TIntegrationEvent_Now(TIntegrationEvent_Now& );

    TIntegrationEvent_Now& operator=(const TIntegrationEvent_Now&);

    int ValueType() {return INTEGRATION_EVTVALTYPE_NOW ;}
    TIntegrationEvent *CloneSelf();

#ifdef TESTING
    static void Test();
#endif
};

class TIntegrationEvent_Duration : public TIntegrationEvent {
private:
    INT_TIME m_endTime;

public:
    TIntegrationEvent_Duration( char *name,
        char *short_name,
        INT_TIME start_time,
        INT_TIME end_time,
        long code,
        int priority,
        TIntegrationEventCategory category);

    TIntegrationEvent_Duration( const TIntegrationEvent_Duration& );

    TIntegrationEvent_Duration& operator=(const TIntegrationEvent_Duration&);

    TIntegrationEvent *CloneSelf();

    virtual void initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue) ;

    int ValueType() {return INTEGRATION_EVTVALTYPE_DURATION ;}
    CLASS_PROPERTY(EndTime, INT_TIME, return m_endTime, m_endTime = v )

#ifdef TESTING
    static void Test();
#endif
};

class TIntegrationEvent_VALUE1 : public TIntegrationEvent {
private:
    INT_FLOAT m_userValue;

public:
    TIntegrationEvent_VALUE1(	char *name,
        char *short_name,
        INT_TIME time,
        INT_FLOAT value,
        long code,
        int priority,
        TIntegrationEventCategory category);
    TIntegrationEvent_VALUE1( const TIntegrationEvent_VALUE1& );

    TIntegrationEvent_VALUE1& operator=(const TIntegrationEvent_VALUE1&);

    TIntegrationEvent *CloneSelf();

    virtual void initialize(INT_TIME time, INT_FLOAT fvalue, bool bvalue) ;

    int ValueType() {return INTEGRATION_EVTVALTYPE_VALUE ;}

    CLASS_PROPERTY(UserValue, INT_FLOAT, return m_userValue, m_userValue = v )

#ifdef TESTING
    static void Test();
#endif
};

bool SortEventsByTime(TIntegrationEvent& evt1, TIntegrationEvent& evt2) ;

#endif


