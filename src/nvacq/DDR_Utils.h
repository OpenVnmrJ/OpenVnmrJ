/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
 *
 */ 
//=========================================================================
// DDR_Utils.h:  DDR_Utils.c header file
//=========================================================================
#ifndef DDR_UTILS_H
#define DDR_UTILS_H

#ifndef C67
union ltw {
  unsigned long long ldur;
  unsigned int  idur[2];
};

void markTime(long long *dat);
double diffTime(long long m2,long long m1);
#endif

double calc_acqtm(double SR, int NP, 
		double SW1, double OS1, double XS1,
		double SW2, double OS2, double XS2
		);
double calc_sw(double SR, double SW1, double SW2);
		
#endif
