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

#ifndef _TABSTRACT_INTEGRATION_MANAGER
#define _TABSTRACT_INTEGRATION_MANAGER

#include "integrator_types.h"
#include "integrator_classes.h"
#include "integrator_calcul.h"

class TAbstract_Integration_Manager {
protected:
    TInternalIntegrator FIntegrator ;

    void Process_Integration() ;

    void AllocateSmoothedSignal();
    void FreeSmoothedSignal();
    void ClearEventList();

public:

    TAbstract_Integration_Manager(INT_SIGNAL _signal, int _nbpoints);
    virtual ~TAbstract_Integration_Manager();

    virtual void integrate() = 0 ;
    virtual void RecomputePeakProperties() = 0;
//signal arrays
    INT_SIGNAL signal();
    INT_SIGNAL signal_smoothed();
    int signal_size();

//event list
    TIntegrationEventList& events() ;

//Global integration parameters
    bool ReduceNoise();
    void ReduceNoise(bool val);

    bool UseRelativeThreshold() ;
    void UseRelativeThreshold(bool val);

    bool ComputeBaselines();
    void ComputeBaselines(bool val);

    INT_FLOAT SpikeReduction();
    void SpikeReduction(INT_FLOAT val);

    INT_FLOAT DeltaTime() ;
    void DeltaTime(INT_FLOAT val);

    INT_FLOAT DeadTime() ;
    void DeadTime(INT_FLOAT val);

    INT_FLOAT PeakSat_Min();
    void PeakSat_Min(INT_FLOAT val);

    INT_FLOAT PeakSat_Max();
    void PeakSat_Max(INT_FLOAT val);

    int NbPeaks() { return FIntegrator.Peaks().size() ;}
    int NbLines() { return FIntegrator.Lines().size() ;} 
    int NbAutoThresholds() { return FIntegrator.AutoThresholdResults.size() ;}
};

#endif
