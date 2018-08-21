/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 * inttypes.h 
 *
 */
#ifndef _INTTYPES_H    /* defined in <inttypes.h> on Solaris and Linux OSes */

#ifndef INTTYPES_H
#define INTTYPES_H

#ifdef __INTERIX  /* automatically defined in SFU compilations */
  typedef unsigned long long uint64_t;
  typedef unsigned int uintptr_t;
#endif

#ifdef VNMRS_WIN32  /* defined in makefiles for windows compilations */
  typedef unsigned __int64 uint64_t;
  typedef unsigned __int32 uintptr_t;
  typedef unsigned __int32 mode_t;
  typedef unsigned __int32 key_t;
#endif

#endif

#endif
