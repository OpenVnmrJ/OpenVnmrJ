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

// File     : debug.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : Debugging stream support
//
// $History: debug.h $
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
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef __VISUAL_CPP__
#include <ostream>
#else
// #include <iostream.h>
#include <iostream>
#endif

extern "C" {
    void DbgOutputInFile(char* filename) ;
    void DbgCloseFile() ;
}

void setDbgStream(std::ostream* astream) ;
std::ostream& dbgstream() ;

void OptionalDebug(char* message, int priority = 1);
void SetVerboseLevel(int priority);

#endif
