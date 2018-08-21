/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//
//  THIS FILE NEEDS TO BE INCLUDED TO BUILD VNMRBG UNDER SFU
//  Problem: 
//    Under sfu, inttypes.h does not exist in the supplied include directories,
//    (despite the fact that this file was added to the "must support" list for 
//    ansi c in 1999)
//  Work-around:
//   copy this file ("sfutypes.h") to C:/SFU/usr/include/inttypes.h 
//   (or run the install_types make target in Makefile)

#ifndef _INTTYPES_H
#define _INTTYPES_H
#ifdef __INTERIX
#include <sys/types.h>
typedef unsigned long long int  uint64_t;
typedef int  type_t;
#ifndef _UINTPTR_T_DEFINED
typedef unsigned int*  uintptr_t;
// typedef size_t   uintptr_t;
#define _UINTPTR_T_DEFINED
#endif/*_UINTPTR_T_DEFINED*/
#endif
#endif
