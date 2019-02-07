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

// File     : integrator_manager_Cstruct.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : Main user class, i/o C structures
//            input  : signal, parameters
//            output : peaks and baselines
//
// $History: integrator_manager_Cstruct.h $
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:57 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 9/07/02    Time: 14:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef _INTEGRATOR_MANAGER_CSTRUCT_H
#define _INTEGRATOR_MANAGER_CSTRUCT_H

#include "integrator_types.h"
#include "integrator_ioStructures.h"
#include "integrator_abstract_manager.h"
#include "integrator_EventsFactory.h"

/************************************************************************************/
extern "C" {
    INT_RESULTS* Integrate(INT_SIGNAL signal, int signalsize, INT_PARAMETERS* params);
    void UpdatePeakProperties(INT_SIGNAL signal, int signalsize, INT_PARAMETERS* params, INT_RESULTS* res) ;
    bool CheckLimits(int EventCode , INT_FLOAT value , char* Error) ;
    void Smooth(INT_SIGNAL signal, int size, INT_SIGNAL smoothed, INT_FLOAT factor) ;
}
/************************************************************************************/

class TIntegration_Manager_Cstruct : public TAbstract_Integration_Manager {
protected:
    TIntegrationEventsFactory FEventsFactory;
    INT_PARAMETERS* FParams ;

    void StoreEvent(INT_EVENT& aevent);
    void GetParameters();

public:
    TIntegration_Manager_Cstruct(INT_SIGNAL _signal,
        int _signalsize,
        INT_PARAMETERS* _params);
    void integrate();
    void RecomputePeakProperties() ;

    void SetResults(INT_RESULTS* pResults);
    void FillResults(INT_RESULTS* pResults);
    static void DeleteResults(INT_RESULTS* pResults) ;
};

#endif
