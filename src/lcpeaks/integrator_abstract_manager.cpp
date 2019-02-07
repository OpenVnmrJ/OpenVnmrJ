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

#include "common.h"
#include "integrator_abstract_manager.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

void TAbstract_Integration_Manager::Process_Integration()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{ 
    FIntegrator.Initialize() ; // smooth signal and compute derivatives
    FIntegrator.FindPeaksAverageSlope() ;
    FIntegrator.ComputeNoiseFromProcessedIntegrationEvents() ;
}

void TAbstract_Integration_Manager::AllocateSmoothedSignal()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FreeSmoothedSignal(); //works even if not allocated
    FIntegrator.SignalSmooth = new INT_FLOAT[signal_size()];
}

void TAbstract_Integration_Manager::FreeSmoothedSignal()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    delete [] FIntegrator.SignalSmooth;
    FIntegrator.SignalSmooth = NULL ;
}

void TAbstract_Integration_Manager::ClearEventList()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.ClearEventList();
}

INT_SIGNAL TAbstract_Integration_Manager::signal()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.Signal ;
}

INT_SIGNAL TAbstract_Integration_Manager::signal_smoothed()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.SignalSmooth;
}

int TAbstract_Integration_Manager::signal_size()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.SignalSize;
}

TAbstract_Integration_Manager::TAbstract_Integration_Manager(INT_SIGNAL _signal,
    int _nbpoints)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
//setup internal integrator
    FIntegrator.Signal     = _signal ;
    FIntegrator.SignalSize = _nbpoints ;
    FIntegrator.SignalSmooth = NULL;
    AllocateSmoothedSignal();

}

TAbstract_Integration_Manager::~TAbstract_Integration_Manager()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FreeSmoothedSignal();
}

bool TAbstract_Integration_Manager::ReduceNoise()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.FReduceNoise;
}
void TAbstract_Integration_Manager::ReduceNoise(bool val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.FReduceNoise = val ;
}

INT_FLOAT TAbstract_Integration_Manager::DeadTime() 
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.DeadTime ;
}

void TAbstract_Integration_Manager::DeadTime(INT_FLOAT val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.DeadTime = val ;
}

INT_FLOAT TAbstract_Integration_Manager::PeakSat_Min()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.PeakSat_min ;
}

void TAbstract_Integration_Manager::PeakSat_Min(INT_FLOAT val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.PeakSat_min = val ;
}

INT_FLOAT TAbstract_Integration_Manager::PeakSat_Max()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.PeakSat_max ;
}

void TAbstract_Integration_Manager::PeakSat_Max(INT_FLOAT val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.PeakSat_max = val ;
}

bool TAbstract_Integration_Manager::UseRelativeThreshold()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.FUseRelativeThreshold;
}

void TAbstract_Integration_Manager::UseRelativeThreshold(bool val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.FUseRelativeThreshold = val ;
}

bool TAbstract_Integration_Manager::ComputeBaselines()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.FComputeBaseLines;
}

void TAbstract_Integration_Manager::ComputeBaselines(bool val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.FComputeBaseLines = val ;
}

INT_FLOAT TAbstract_Integration_Manager::SpikeReduction()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.FSpikeReduceParameter;
}

void TAbstract_Integration_Manager::SpikeReduction(INT_FLOAT val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.FSpikeReduceParameter = val ;
}

INT_FLOAT TAbstract_Integration_Manager::DeltaTime()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.DeltaTime;
}

void TAbstract_Integration_Manager::DeltaTime(INT_FLOAT val)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FIntegrator.DeltaTime = val ;
}

TIntegrationEventList& TAbstract_Integration_Manager::events()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    return FIntegrator.Events ;
}

