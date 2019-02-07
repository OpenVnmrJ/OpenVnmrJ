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

// File     : Wavelets.h
// Author   : Bruno Orsier : Original Delphi code
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration computation class and functions
//            Performs wavelets transform and denoising
//            by wavelets shrinkage.
//
// $History: Wavelets.h $
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "integrator_types.h"

class Wavefilt {
public:
    unsigned long
    ncof,
    ioff,
    joff ;
    double* cc ;
    double* cr ;
    Wavefilt(int n) ;
    ~Wavefilt() ;

    void pwt(double* a, unsigned long n, int isign) ;
//WARNING : indexes are 1-based.

private:
    double* wksp;
};

//--------------- Wavelet shrinkage functions -------------

INT_FLOAT softshrink(INT_FLOAT x, INT_FLOAT s);
INT_FLOAT semisoftshrink(INT_FLOAT L1, INT_FLOAT L2, INT_FLOAT x);
INT_FLOAT semisoftshrink2(INT_FLOAT L1, INT_FLOAT L2, INT_FLOAT x);
