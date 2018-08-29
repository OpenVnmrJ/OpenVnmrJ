/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef CONVERT_H
#define CONVERT_H

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

/************************************************************************
*                                                                       *
*  Convert type of floating point data to type of char                  *
*  The input data must be a floating point absolute value, and the      *
*  resulting output data will range from 0 to max_value.                *
*                                                                       */
extern void
convert_float_to_char(float *indata,    /* input data */
                char *outdata,          /* output result */
                int numdata,            /* number of data */
                float vs,               /* vertical scale */
                int max_value);         /* maximum data value */

/************************************************************************
*                                                                       *
*  Convert type of floating point data to type of short                 *
*  The input data must be a floating point absolute value, and the      *
*  resulting output data will range from 0 to max_value.                *
*                                                                       */
extern void
convert_float_to_short(float *indata,   /* input data */
                short *outdata,         /* output result */
                int numdata,            /* number of data */
                float vs,               /* vertical scale */
                int max_value);         /* maximum data value */

/************************************************************************
*                                                                       *
*  Convert type of floating point data to type of int                   *
*  The input data must be a floating point absolute value, and the      *
*  resulting output data will range from 0 to max_value.                *
*                                                                       */
extern void
convert_float_to_int(float *indata,     /* input data */
                int *outdata,           /* output result */
                int numdata,            /* number of data */
                float vs,               /* vertical scale */
                int max_value);         /* maximum data value */
#endif CONVERT_H
