/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCcommondefsh
#define INCcommondefsh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */
typedef int (*PFI)();
typedef long (*PFL)();

#ifdef __INCvxWorksh
# define TIME_OUT (-2)
# define DIRECT_READ (-3)
#else
# define NO_WAIT 0
# define WAIT_FOREVER (-1)
# define TIME_OUT (-2)
# define DIRECT_READ (-3)
#endif

/* dummy args */
#define ARG1 0
#define ARG2 0
#define ARG3 0
#define ARG4 0
#define ARG5 0
#define ARG6 0
#define ARG7 0
#define ARG8 0
#define ARG9 0
#define ARG10 0

/* vxMemProbe Access Sizes */
#define BYTE	1
#define SHORT	2
#define WORD	2
#define LONG	4

#ifdef __cplusplus
}
#endif

#endif
/* INCcommondefsh */
