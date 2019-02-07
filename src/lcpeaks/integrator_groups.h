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

// File     : integrator_groups.h
// Author   : Bruno Orsier : original Delphi code
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration algorithm
//            Class to handle skimming
//
// $History: integrator_groups.h $
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
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef _INTEGRATOR_GROUPS_H
#define _INTEGRATOR_GROUPS_H

#include "integrator_util_events.h"
#include "integrator_util_peaks.h"
#include "integrator_classes.h"
#include "integrator_calcul.h"

class TPeaksToSkim {
private:
    int FIncrement;
    TCandidatePeakList FPeaks ;
    INT_FLOAT FDeltaTime ;
    INT_SIGNAL FSignal ;
    TBaseLineType FlineType ;
    TInternalIntegrator* pFIntegrator ;

    bool TestDaughterPeak(TCandidatePeak& MotherPeak,
        TCandidatePeak& DaughterPeak,
        INT_FLOAT ratio) ;

    bool TestPlausibleDaughterPeak(TCandidatePeak& DaughterPeak);

    INT_FLOAT MaxHeight(TCandidatePeak& peak,
        INT_FLOAT base) ;

    void ProcessSubGroup(TCandidatePeak& MotherPeak,
        TCandidatePeakList& Daughters) ;

    bool CheckSubGroup(TCandidatePeak& MotherPeak,
        TCandidatePeakList& Daughters);

public:
    TPeaksToSkim(bool directionIsRight,
        INT_SIGNAL Signal,
        INT_FLOAT deltatime,
        TBaseLineType linetype,
        TInternalIntegrator* integrator);

    ~TPeaksToSkim();

    void Add(TCandidatePeak& aPeak);
    void Clear();
    void Process(TTimeAndValue& SkimRatios);
};

#endif
