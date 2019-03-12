// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* pansy_cosy.c - PANSY COSY, homo/hetero-nuclear correlation spectroscopy, 
   written by Eriks Kupce, Oxford, April 2006
 
  Processing for H-H cosy:
  ------------------------
  av axis='pp' b=-at/2 sbs='n' sb1=-0.5*ni/sw1 sbs1='n' lsfrq1=-sw1/2
  wft2d(1,0,0,0,0,0,0,0,0,0,0,0,0,-1,0,0)

  Processing for H-C COSY:
  ------------------------
  axis='dp' lsfrq1='n'
  wft2d(0,0,1,0,0,0,0,0,0,0,0,0,0,0,-1,0)

  Referencing:
  ------------
  the setref macro currently fails for the PANSY data sets. Hence the 
  referencing should be done manually, by setting the reference using a cursor.

*/


#include <standard.h>

void pulsesequence()
{
  	double  hsglvl,
		hsgt,
		pwC = getval("pwC"),
		pwClvl = getval("pwClvl");
	char    sspul[MAXSTR];

  	hsglvl = getval("hsglvl");
	hsgt = getval("hsgt");
	getstr("sspul",sspul);

	dbl(ct,v1);
	mod4(v1,v1);      /* v1  = 02 */
	assign(v1, oph);  /* oph = 02 */

	assign(one, v2);  /* v2  = 11 */
        if(phase1 == 2) 
          add(three,v2,v2);
        
	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
		add(v2,v10,v2);
		add(oph,v10,oph);

  status(A);

 	delay(5.0e-5);
 	decpower(pwClvl);

        if (sspul[0] == 'y')
        {
          zgradpulse(hsglvl,hsgt);
          simpulse(pw, pwC, zero, zero, rof1, rof1);
          zgradpulse(hsglvl,hsgt);
        }

        pre_sat();
        obspower(tpwr);
        
        zgradpulse(hsglvl,hsgt);
        decrgpulse(pwC, zero, rof1, rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1.0e-3);

  status(B);
	rgpulse(pw, v1, rof1, rof1);
	delay(d2); 
	simpulse(pw, pwC, v2, zero, rof1, rof2);

  status(C);
}
