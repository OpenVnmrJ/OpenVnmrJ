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

// File     : GeneralLibrary.h
// Author   : Gilles Orazi
// Created  : 06/2002
// Comments : Define some very useful functions
//
// $History: GeneralLibrary.h $
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

#ifndef _GENERAL_LIBRARY_H
#define _GENERAL_LIBRARY_H

//------- TEMPORARY IMPLEMENTATION -------------
// TO BE CHANGED INTO INLINE TEMPLATE FUNCTIONS
//----------------------------------------------

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define round(x) ( (int)( ((x) < 0) ? ((x) - 0.5) : ((x) + 0.5) ) )

template<class T> int sign(T x) {if (x>0) return 1 ;  else if (x<0) return -1 ; else return 0 ;}

#endif
