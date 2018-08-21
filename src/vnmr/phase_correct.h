/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**************************************************/
/* definitions for phase correction algorithms    */
/**************************************************/
#ifndef PHASE_CORRECT
#define PHASE_CORRECT

#define PC_FIT_WIDTH 20


/* definitions for phase map generation options   */
#define POINTWISE 0
#define LINEAR 1
#define QUADRATIC 2
#define CENTER_PAIR 3
#define PAIRWISE 4
#define FIRST_PAIR 5
#define TRIPLE_REF 6
#define OFF -1
#define MIN_OPTION POINTWISE
#define MAX_OPTION TRIPLE_REF


#endif 
