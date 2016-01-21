/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCconsolestat
#define INCconsolestat

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

extern void setLockLevel( int newLockLevel );
extern void setSpinAct( int newSpinAct );
extern short getLSDVword();
extern void setLSDVword( ushort newLSDVword );
extern void setLSDVbits( ushort bits2set );
extern void clearLSDVbits( ushort bits2clear );

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern void setLockLevel();
extern void setSpinAct();
extern short getLSDVword();
extern void setLSDVword();
extern void setLSDVbits();
extern void clearLSDVbits();
#endif

#ifdef __cplusplus
}
#endif

#endif
