/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***************************************************************************
* File boolean.h: defines for Boolean constants FALSE and TRUE             *
*      ---------                                                           *
***************************************************************************/

#ifdef HEADER_ID

#ifndef lint
	static char BOOLEAN_H_ID[]="@(#)boolean.h 18.1 03/21/08 20:00:44 Copyright 1992 Spectroscopy Imaging Systems";
#endif /* (not) lint */

#else /* (not) HEADER_ID */

#ifndef BOOLEAN_H
#define BOOLEAN_H

/* make sure the fundamental values TRUE and FALSE are defined correctly */

#if !defined(FALSE)
#define FALSE 0
#else
#if FALSE != 0
#undef FALSE
#endif
#endif

#if !defined(TRUE)
#define TRUE 1
#else
#if TRUE != 1 && TRUE != !FALSE
#undef TRUE
#endif
#endif

#endif /* (not) BOOLEAN_H */

#endif /* HEADER_ID */
