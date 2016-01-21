/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef FFT_H
#define FFT_H
/*-----------------------------------------------
|						|
|		     fft()/9			|
|						|
|   This function performs an FFT or I-FFT on	|
|   the input data.				|
|						|
|   Ift must have 0 frequency in zero element   |
+----------------------------------------------*/
/* fn		number of complex points */
/* level	number of FT levels */
/* zfratio	zero-filling ratio */
/* skip		skip factor */
/* datatype	type of data:  complex or hypercomplex */
/* nfidpts	TEMPORARY */
extern int fft(float *data, int fn, int level, int zfratio, int skip, int datatype, double sign,
        double fnorm, int nfidpts);

#endif
