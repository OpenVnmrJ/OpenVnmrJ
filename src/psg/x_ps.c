/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
/*---------------------------------------------------------
| x_pulsesequence()
|	dummy dps pulse sequence
|	This sequence is defaulted to if the pulse sequence
|	fails the dps_ps_gen step of seqgen.
+----------------------------------------------------------*/
void
x_pulsesequence()
{
   fprintf(stdout, "This pulse sequence was not prepared for 'dps' display.\n");
}

void
t_pulsesequence()
{
   fprintf(stdout, "This pulse sequence was not prepared for timer.\n");
}

