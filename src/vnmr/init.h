/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------
|	init.h
|
|	This include file contains the intial values for a variable
|	during creation.
+------------------------------------------------------------------------*/
struct value
{
  int   type;
  float maxVal;
  float minVal;
  float step;
  int   Ggroup;
  int   Dgroup;
  int   Prot;
}preset[] ={ 
    {T_UNDEF,  1e18, -1e18, 0.0,  G_ACQUISITION, 1, 0}, /* undefined variable */
    {T_REAL,   1e18, -1e18, 0.0,  G_ACQUISITION, 1, 0}, /* real variable      */
    {T_STRING, 8,     0,    0,    G_ACQUISITION, 1, 0}, /* string variable    */
    {T_REAL,   14,   14,    14,   G_ACQUISITION, 1, P_MMS}, /* delay variable */
    {T_STRING, 4,     0,    0,    G_ACQUISITION, 1, 0}, /* flag variable      */
    {T_REAL,   1e9,   -1e9, 0,    G_ACQUISITION, 1, 0}, /* freq variable      */
    {T_REAL,   13,    13,   13,   G_ACQUISITION, 1, P_MMS}, /* pulse variable */
    {T_REAL,   32767, 0,    0,    G_ACQUISITION, 1, 0}, /* integer variable   */
           };
