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

// File     : integrator_ioFunctions.h
// Author   :Gilles Orazi
// Created  : 06/2002
// Comments : Utilitary functions
//             - Read integration parameters from file
//             - Print integration results in an output stream
//
// $History: integrator_ioFunctions.h $
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 9/07/02    Time: 14:41 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */

#ifndef _INTEGRATOR_IOFUNCTIONS_H
#define _INTEGRATOR_IOFUNCTIONS_H

#include <string>
#include "integrator_ioStructures.h"

#ifdef __VISUAL_CPP__
#include <ostream>
#else
// #include <iostream.h>
#include <iostream>
#endif

INT_PARAMETERS* CreateParametersFromFile(string filename) ;
void FormatResults(INT_RESULTS& results, ostream& out);

#endif
