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

// File     : integrator_types.h
// Author   : Diego Segura / Gilles Orazi
// Created  : 06/2002
// Comments : Base types used by the integration algorithm
//
// $History: integrator_types.h $
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef _INTEGRATOR_TYPES_H
#define _INTEGRATOR_TYPES_H

#include <vector>
#include <set>
//#include "integrator_CTypes.h"
using namespace std;

//---------- CLASS PROPERTIES ----------------

#define CLASS_PROPERTY(name, type, read_action, write_action)	type name() const { read_action; } \
    void name(const type v) {write_action;}

#define CLASS_READONLY_PROPERTY(name, type, read_action)	type name() const { read_action; }

//--------------------------------------------

typedef double              INT_FLOAT;	// double precision floating point (8 bytes)
typedef INT_FLOAT         *INT_SIGNAL;	// array of SIGNALPOINTs
typedef INT_FLOAT            INT_TIME;	// type used for time specification

typedef long			INT_EVENTTYPE;	// Type of event
#define INT_MINFLOAT            -1E38  //very small number for initializations
#define INT_MAXFLOAT            +1E38  //very high  number for initializations

#define EVTYPE_ONOFF	        0
#define EVTYPE_NOW		        1
#define EVTYPE_DURATION	        2
#define EVTYPE_VALUE1           3

typedef enum{  
    IEC_PARAMDETECT	= 0,
    IEC_MINVALUES	= 1,
    IEC_FORCEDPEAKS	= 2,
    IEC_NEGPEAK	    = 3,
    IEC_BASELINE	= 4,
    IEC_SKIMMING	= 5 
}	TIntegrationEventCategory;	// Category of event

#define INT_MAX_STRING_LENGTH			256
#define INT_MAX_SHORT_STRING_LENGTH		16

//------------- PEAKS -------------------------

// -- Constants for code field
#define PEAK_CODE_FUSED_END	"FE"
#define PEAK_CODE_FUSED_START	"FS"
#define PEAK_CODE_FUSED_BOTH	"FB"

// -- Special Baseline id
#define PEAK_NO_BASELINE	-1

// -- Special mother peak index
#define PEAK_NO_MOTHER_PEAK	-1

//------------ BASELINES ---------------------

// -- baseline types
typedef enum {	
    BASELINE_STRAIGHT       =0,
    BASELINE_EXPONENTIAL    =1 
} TBaseLineType;

/* This header file adds some C++ types to the C-types
 * defined in integrator_CTypes.h
 */

typedef vector<bool>            INT_BOOLVECTOR;
typedef vector<int>             INT_INDEXLIST;
typedef set<int>                INT_INDEXSET;

#endif
