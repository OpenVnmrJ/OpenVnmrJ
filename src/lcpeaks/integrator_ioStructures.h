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

// File     : integrator_ioStructures.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : integration data structures
//            Define structures for input and output
//
// $History: integrator_ioStructures.h $
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
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
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef _INTEGRATOR_IOSTRUCTURES_H
#define _INTEGRATOR_IOSTRUCTURES_H

#include "integrator_types.h"

#define  INTEGRATION_EVTVALTYPE_ONOFF      0 
#define  INTEGRATION_EVTVALTYPE_NOW        1 
#define  INTEGRATION_EVTVALTYPE_VALUE      2 
#define  INTEGRATION_EVTVALTYPE_DURATION   3 

typedef struct {
    char*       name;
    char*       shortname;
    int         id;
    int         category;
    int         valuetype; //const : INTEGRATION_EVTVALTYPE_*
    INT_FLOAT   defaultvalue ;
} INT_EVENT_INFO ;

typedef struct {
    INT_FLOAT time;
    int id;
    INT_FLOAT value;
    bool on;
} INT_EVENT ;

typedef struct {
    int NumberOfEvents ;
    INT_EVENT* events;

    bool ReduceNoise;
    bool UseRelativeThreshold ;
    bool ComputeBaselines ;
    INT_FLOAT SpikeReduction;
    INT_FLOAT DeltaTime ;
    INT_FLOAT DeadTime  ;
    INT_FLOAT PeakSat_Min ;
    INT_FLOAT PeakSat_Max ;
} INT_PARAMETERS ;

typedef struct {
    INT_FLOAT StartTime ;
    INT_FLOAT RetentionTime ; //TODO
    INT_FLOAT EndTime ;
    INT_FLOAT Area, pcArea ;
    INT_FLOAT Height, pcHeight ;
    INT_FLOAT LeftInflexionPointTime ;
    INT_FLOAT RightInflexionPointTime ;

    bool isUserSlice ;

    int       LeftInflexionPointErrorCode ;
    int       RightInflexionPointErrorCode ;
    INT_FLOAT LeftTgCoeffA ;
    INT_FLOAT LeftTgCoeffB ;
    INT_FLOAT RightTgCoeffA ;
    INT_FLOAT RightTgCoeffB ;

    INT_FLOAT InterTgLb_X1;
    INT_FLOAT InterTgLb_X2;

    INT_FLOAT PrevValleyHeight;

    INT_FLOAT
        ASYMETRY_PHARMACOP_EUROP,
        ASYMETRY_USP_EMG_EP_ASTM,
        ASYMETRY_HALF_WIDTHS_44,
        ASYMETRY_HALF_WIDTHS_10;

    INT_FLOAT
        THPLATES_SIGMA2,
        THPLATES_SIGMA3,
        THPLATES_SIGMA4,
        THPLATES_SIGMA5,
        THPLATES_USP,
        THPLATES_EP_ASTM,
        THPLATES_EMG,
        THPLATES_AREAHEIGHT;

    INT_FLOAT
        Width_TgBase,
        Width_4p4,
        Width_5p,
        Width_10p,
        Width_13p4,
        Width_32p4,
        Width_50p,
        Width_60p7;

    INT_FLOAT
        Moment1,
        Moment2,
        Moment3,
        Moment4,
        Skew,
        Kurtosis;

    INT_FLOAT
        Left_Width_4p4,
        Left_Width_5p,
        Left_Width_10p,
        Left_Width_13p4,
        Left_Width_32p4,
        Left_Width_50p,
        Left_Width_60p7;

    INT_FLOAT
        Resolution_Valleys,
        Resolution_HalfWidth,
        Resolution_USP,
        Capacity,
        Selectivity;
    
    INT_FLOAT
        BaseLineHeight_AtRetTime;

    char*
        code ;

    int
        BaselineIdx,
        MotherIdx;

} INT_RESULT_PEAK ;

typedef struct {
    INT_FLOAT StartTime ;
    INT_FLOAT EndTime ;
    INT_FLOAT StartValue ;
    INT_FLOAT EndValue ;
    int LineType ;
} INT_RESULT_LINE ;

typedef struct {
    INT_FLOAT Time ;
    INT_FLOAT Threshold;
    INT_FLOAT RelativeThreshold ;
} INT_RESULT_AUTOTHRESHOLD ;


typedef struct {
    int NumberOfPeaks ;
    INT_RESULT_PEAK* Peaks ;
    int NumberOfLines ;
    INT_RESULT_LINE* Lines ;
    INT_FLOAT
        NoiseSdev,
        Noise,
        RMSNoise,
        Drift ;
    int NumberOfThresholds ;
    INT_RESULT_AUTOTHRESHOLD*
        AutoThresholds;

} INT_RESULTS ;

extern "C" {

// Memory allocation functions :
// -----------------------------

    INT_RESULTS* Alloc_Results(int NbLines, int NbPeaks, int NbAutoThreshold) ;
    void Free_Results(INT_RESULTS* res);

    INT_PARAMETERS* Alloc_Parameters(int NbEvents) ;
    void Free_Parameters(INT_PARAMETERS* params) ;

    char* Alloc_String(int NbChar) ;
    void Free_String(char* astr);


// Events handled by the integration library :
// -------------------------------------------
    
    void Integration_Event_List(INT_EVENT_INFO* &evtlist, int& nbevts) ;
    void Free_Event_List(INT_EVENT_INFO* evtlist, const int nbevts);

}

#endif
