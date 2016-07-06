/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#ifndef gradientSPI_h
#define gradientSPI_h

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern void initSPI();
extern int  setspi(int chip_select,int data_value);
extern void setAxisAtten(int channel, int value);
extern void setEccScaling(int value);
extern void ZeroEccRam();
extern void loadEccAmpTerms(signed int *eccAmpTermsArray);
extern void loadEccTimeConstTerms(signed long long *eccTCTermsArray);

#ifdef __cplusplus
}
#endif
 
#endif

