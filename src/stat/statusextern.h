/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|
|	File of global externals
|
+----------------------------------------------------------------------------*/
 

extern  int debug;

extern  int StartY;

extern  int VTpan,VTpan1,VTpan2,SPNpan2;
 
#define HOSTLEN 40
extern  char User[HOSTLEN];		/* The users login Name */
extern  char LocalHost[HOSTLEN];	/* The Host machine Name */
extern  int  Procpid; 		/* This process's PID */
extern  long PresentTime;
extern  long IntervalTime;

/*
extern  Rect *rect;
*/
