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

// File     : integrator_util_misc.h
// Author   : Bruno Orsier : original code in Delphi
//            Gilles Orazi : C++ translation
// Created  : 06/2002
// Comments : Integration algorithm
//
// $History: integrator_util_misc.h $
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

#ifndef __INTEGRATOR_UTIL_MISC_H
#define __INTEGRATOR_UTIL_MISC_H

#include "integrator_types.h"

/*
  Class TSpiralingCounter
  AUTHOR: BO, Sept 29, 1998 / translated in C++ by GO 17.05.02
  PURPOSE: helps to browse
           the set { (i,j), i,j in -INF..INF} in a spiraling way,
	   starting from the values closest to zero. See example
	   below :
	   NextValue   Result
	   call #
	   1          (1 0)
	   2          (1 1)
	   3          (0 1)
	   4          (-1 1)
	   5          (-1 0)
	   6          (-1 -1)
	   7          (0 -1)
	   8          (1 -1)
	   9          (2 -1)
	   10         (2 0)
	   11         (2 1)
	   12         (2 2)
	   13         (1 2)
	   14         (0 2)
	   15         (-1 2)
	   16         (-2 2)
	   17         (-2 1)
	   18         (-2 0)
	   19         (-2 -1)
	   20         (-2 -2)
	   etc.
*/

class  TSpiralingCounter {
private:
    int
    deltai,
    deltaj,
    iplus,
    jplus,
    irange;

    int sgn(int x);

public:
    TSpiralingCounter();

    void NextValue() ;
    void Reset() ;

    CLASS_READONLY_PROPERTY(CurrentI, int, return deltai);
    CLASS_READONLY_PROPERTY(CurrentJ, int, return deltaj);
};

class TLineTool_Integer {
public:
    virtual INT_FLOAT ValueAt(int I) = 0 ;
};

class TStraightLineTool_Integer : public TLineTool_Integer {
private:
    int
    Fline_I1,
    Fline_I2;
    INT_FLOAT
    Fline_Y1,
    Fline_Y2 ;

public:
    TStraightLineTool_Integer(int       line_I1,
        INT_FLOAT line_Y1,
        int       line_I2,
        INT_FLOAT line_Y2) ;
    virtual INT_FLOAT ValueAt(int I);
};

class TExpLineTool_NotUsed : public TLineTool_Integer {
private:
    INT_FLOAT
    FA,
    FB,
    FDeltaT;
public:
    TExpLineTool_NotUsed(INT_FLOAT A,
        INT_FLOAT B,
        INT_FLOAT DeltaT) ;
    virtual INT_FLOAT ValueAt(int I);
};

#endif
