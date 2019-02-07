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

#ifndef _INTEGRATOR_BASE_MANAGER
#define _INTEGRATOR_BASE_MANAGER

#include "integrator_types.h"

class BASE_INTEGRATION_MANAGER {
protected:
    TInternalIntegrator FIntegrator ;
    TIntegrationEventOwningList FEvtlist ;
    TIntegrationEventsManager FEventsManager;

    INT_SIGNAL FSignal, FSmoothedSignal ;
    int FSignalSize;

    virtual void AddMandatoryEvents();
    virtual void Process_Integration();
    virtual void FreeSmoothedSignal();
    virtual ...* peaklist();
    virtual ...* baselinelist();

public:

    BASE_INTEGRATION_MANAGER(INT_SIGNAL _signal, int nbpoints);

    virtual void integrate() = 0 ;

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

};

#endif
