/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _WIN_PROCESS_H
#define _WIN_PROCESS_H

/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

/* Process statistics and show the result */
extern void winpro_statistics_show();

/* Show the histogram window controller */
extern void winpro_histenhance_show(void);

/* Show the image rotation window controller */
extern void winpro_rotation_show(void);

/* Show the arithmetic window controller */
extern void winpro_arithmetic_show(void);

/* Show the filter window controller */
extern void winpro_filter_show(void);

/* Show the math window controller */
extern void winpro_math_show(void);

/* Insert text into math expression */
void winpro_math_insert(char *text, int isimage);
#define MATH_NOTIMAGE 0
#define MATH_ISIMAGE 1

#endif (_WIN_PROCESS_H)
