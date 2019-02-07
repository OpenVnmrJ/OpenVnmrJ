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

// File     : integrator_peakProperties.h
// Author   : Bruno Orsier : original code in Delphi
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration algorithm
//            Peak properties computations
//
//            The PEAK_COMPUTER class was originally declared in
//            integrator_classes. During C++ translation, I moved
//            it into a separated .h file. GO.
//
// $History: integrator_peakProperties.h $
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 9/07/02    Time: 14:41 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */

#ifndef _INTEGRATOR_PEAKPROPERTIES_H
#define _INTEGRATOR_PEAKPROPERTIES_H

#include "integrator_classes.h"
#include "integrator_util_peaks.h"
#include "integrator_types.h"

class PEAK_COMPUTER {
private:
    INT_SIGNAL m_signal, m_signalSmooth;
    int m_signalsize ;
    INT_FLOAT m_deltaTimeMinutes;
    INT_SIGNAL m_firstDeriv;

    LINE_FITTER LineFitter ;

    void Initialize();
    void ComputeAsymetry();
    void ComputePlates();
    void Reset();
    void LocateInflexionPoints();

    int LocateMaxDeriv(int idxfrom,
        int idxto,
        int step,
        int& errCode);

    INT_FLOAT ComputeTgIntersectionWithBL(INT_FLOAT TgCoeffA,
        INT_FLOAT TgCoeffB);

    void ComputeWidthsAtHeight(
        INT_FLOAT HeightRatio,    // absolute height (i.e above 0, not above the baseline)
        INT_FLOAT& HalfWidthLeft,
        INT_FLOAT& HalfWidthRight);

    TCandidatePeak* pCurpeak ;

public:

    /*
    INT_FLOAT
        FbaseLineHeight_AtRetTime;
    */

    PEAK_COMPUTER(
        INT_SIGNAL Signal,
        int SignalSize,
        INT_SIGNAL SignalSmooth,
        INT_FLOAT DeltaTimeMinutes);

    ~PEAK_COMPUTER();

    void Integrate(TCandidatePeak& aPeak) ;
    
    INT_FLOAT
        PrevValleyHeight ;

// void  SetForbiddenHeightIndexes(Const Indexes: TBits); //not used for peak detection

//    CLASS_READONLY_PROPERTY(StartTime,INT_FLOAT,return m_startTime)
//    CLASS_READONLY_PROPERTY(EndTime,INT_FLOAT,return m_endTime)

};

#endif
