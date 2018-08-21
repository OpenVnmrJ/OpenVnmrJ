/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.2  2006/08/29 20:43:54  mikem
*     Removed doubled defined functions and variables
*
*     Revision 1.1  2006/08/23 14:09:58  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:04  deans
*     *** empty log message ***
*
*     Revision 1.3  2006/07/07 20:10:19  deans
*     sgl library changes
*     --------------------------
*     1.  moved most core functions in sgl.c to sglCommon.c (new).
*     2. sgl.c now contains only pulse-sequence globals and
*          parameter initialization functions.
*     3. sgl.c is not built into the nvpsg library but
*          is compiled in with the user sequence using:
*          include "sgl.c"
*        - as before ,so sequences don't need to be modified
*     4. sgl.h is no longer used and has been removed from the project
*
*     Revision 1.2  2006/07/07 01:10:48  mikem
*     modification to compile with psg
*
*   
* 
*
* Author: Michael L. Gyngell
* Contains additions to SGL by Michael L. Gyngell.
* 1) Utilities for extracting general information from SGL structures
*
***************************************************************************/
#ifndef SGLADDITIONS_H
#define SGLADDITIONS_H

#include "sglCommon.h"

int countListNodes( struct SGL_LIST_T *aList );
struct SGL_LIST_T *headOfList( struct SGL_LIST_T *aList );


char *allocateString( int aSize );
void freeString( char **aString );
char *duplicateString( char *aString );
void replaceString( char **aString, char *aNewString );
char *concatenateStrings( char *aString, char *a2ndString );
void appendString( char **aString, char *aAddString );
void appendFormattedString( char **aString, char *aAddString, ... );

int    set_pe_table();
void   write_tab_file( char *tableFileName, 
                       int numTables, ...);
double getResolution( SGL_GRADIENT_T *grad );
void   setName( SGL_GRADIENT_T *grad, 
                char name[] );
char   *getName( SGL_GRADIENT_T *grad );
double getAmplitude( SGL_GRADIENT_T *grad );
double getMaxAmplitude( GENERIC_GRADIENT_T  *gradient, double startTime, double duration, int *numPoints );
double getDuration( SGL_GRADIENT_T	*grad );
long   getNumPoints( SGL_GRADIENT_T	*grad );
double *getDataPoints( SGL_GRADIENT_T	*grad );
void   initOutputGradient( GENERIC_GRADIENT_T *outputGradient, char name[], double duration, double resolution );
void   copyToOutputGradient( GENERIC_GRADIENT_T *outputGradient, double *gradientPoints, long numPoints, double startTime, SGL_CHANGE_POLARITY_T invert );
double writeOutputGradientDBStr( SGL_GRADIENT_T *outputGradient, double startTime, double duration, char *gradParams );
double writeOutputGradient( SGL_GRADIENT_T *outputGradient, double startTime, double duration);
double write_gradient( SGL_GRADIENT_T *outputGradient );
double write_gradient_snippet( SGL_GRADIENT_T *outputGradient, double startTime, double duration );

int t_countListNodes( struct SGL_LIST_T *aList );
struct SGL_LIST_T *t_headOfList( struct SGL_LIST_T *aList );

char *t_allocateString( int aSize );
void t_freeString( char **aString );
char *t_duplicateString( char *aString );
void t_replaceString( char **aString, char *aNewString );
char *t_concatenateStrings( char *aString, char *a2ndString );
void t_appendString( char **aString, char *aAddString );
void t_appendFormattedString( char **aString, char *aAddString, ... );

int    t_set_pe_table();
void   t_write_tab_file( char *tableFileName, 
                       int numTables, ...);
double t_getResolution( SGL_GRADIENT_T *grad );
void   t_setName( SGL_GRADIENT_T *grad, 
                char name[] );
char   *t_getName( SGL_GRADIENT_T *grad );
double t_getAmplitude( SGL_GRADIENT_T *grad );
double t_getMaxAmplitude( GENERIC_GRADIENT_T  *gradient, double startTime, double duration, int *numPoints );
double t_getDuration( SGL_GRADIENT_T	*grad );
long   t_getNumPoints( SGL_GRADIENT_T	*grad );
double *t_getDataPoints( SGL_GRADIENT_T	*grad );
void   t_initOutputGradient( GENERIC_GRADIENT_T *outputGradient, char name[], double duration, double resolution );
void   t_copyToOutputGradient( GENERIC_GRADIENT_T *outputGradient, double *gradientPoints, long numPoints, double startTime, SGL_CHANGE_POLARITY_T invert );
double t_writeOutputGradientDBStr( SGL_GRADIENT_T *outputGradient, double startTime, double duration, char *gradParams );
double t_writeOutputGradient( SGL_GRADIENT_T *outputGradient, double startTime, double duration);
double t_write_gradient( SGL_GRADIENT_T *outputGradient );
double t_write_gradient_snippet( SGL_GRADIENT_T *outputGradient, double startTime, double duration );

int x_countListNodes( struct SGL_LIST_T *aList );
struct SGL_LIST_T *x_headOfList( struct SGL_LIST_T *aList );

char *x_allocateString( int aSize );
void x_freeString( char **aString );
char *x_duplicateString( char *aString );
void x_replaceString( char **aString, char *aNewString );
char *x_concatenateStrings( char *aString, char *a2ndString );
void x_appendString( char **aString, char *aAddString );
void x_appendFormattedString( char **aString, char *aAddString, ... );

int    x_set_pe_table();
void   x_write_tab_file( char *tableFileName, 
                       int numTables, ...);
double x_getResolution( SGL_GRADIENT_T *grad );
void   x_setName( SGL_GRADIENT_T *grad, 
                char name[] );
char   *x_getName( SGL_GRADIENT_T *grad );
double x_getAmplitude( SGL_GRADIENT_T *grad );
double x_getMaxAmplitude( GENERIC_GRADIENT_T  *gradient, double startTime, double duration, int *numPoints );
double x_getDuration( SGL_GRADIENT_T	*grad );
long   x_getNumPoints( SGL_GRADIENT_T	*grad );
double *x_getDataPoints( SGL_GRADIENT_T	*grad );
void   x_initOutputGradient( GENERIC_GRADIENT_T *outputGradient, char name[], double duration, double resolution );
void   x_copyToOutputGradient( GENERIC_GRADIENT_T *outputGradient, double *gradientPoints, long numPoints, double startTime, SGL_CHANGE_POLARITY_T invert );
double x_writeOutputGradientDBStr( SGL_GRADIENT_T *outputGradient, double startTime, double duration, char *gradParams );
double x_writeOutputGradient( SGL_GRADIENT_T *outputGradient, double startTime, double duration);
double x_write_gradient( SGL_GRADIENT_T *outputGradient );
double x_write_gradient_snippet( SGL_GRADIENT_T *outputGradient, double startTime, double duration );



#endif
