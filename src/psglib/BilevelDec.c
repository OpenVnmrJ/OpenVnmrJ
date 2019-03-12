// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* BilevelDec.c - bilevel decoupling test sequence. 
                Eriks Kupce, Oxford, 12.07.2004
                Ref. E. Kupce, R. Freeman, G. Wider and K. Wuethrich,
                     J. Magn. Reson., Ser A, vol. 122, p. 81 (1996)

                Requires the following parameters:
                BLdec - flag for bilevel decoupling during acquisition
                jBL - J-coupling to be decoupled by bi-level decoupling
                nBL - number of high power pulses, must be a power of 2. Larger
                      nBL suppresses higher order sidebands and requires more
                      RF power. The default is 2.
                BLxpwr - extra power level for low power decoupling. This is
                         added to the internal setting of the low power
                         decoupling level to reduce the inner sidebands.
                BLorder - 1, suppresses only high order sidebands; requires
                             the shortest duration of high power decoupling.
                             The sub-harmonic sidebands can be suppressed by
                             using high peak amplitude decoupling sequences,
                             such as clean-WURST or sech. The inner sidebands
                             can be suppressed by increasing the extra power
                             level, BLxpwr for the low power  decoupling.
                          2, suppresses all outer sidebands including the
                             sub-harmonic;
                             this is the DEFAULT option. The inner sidebands
                             can be reduced by increasing the extra power
                             level, BLxpwr for low power  decoupling.
                          3, suppresses all sidebands including the inner
                             sidebands; requires the longest duration of
                             high power decoupling;

               Note: This setup uses explicit acquisition (dsp must be set to
                     dsp = 'n') and depends on the digitization rate. As a
                     result, the decoupling pulse duration depends on spectral
                     width, sw. This can have some limitations, particularly
                     for relatively large J-couplings and relatively small sw.
*/

#include <standard.h>
#include "Pbox_bld.h"


void pulsesequence()
{  
   double          decbw = getval("decbw"),        /* decoupling bandwidth */
                   ref_pwr = getval("ref_pwr"), 
                   ref_pw90 = getval("ref_pw90");
   char            BLdec[MAXSTR];   /* Bi-level decoupling flag */

   getstr("BLdec", BLdec);

   if ((dm[C] == 'y') && (BLdec[0] == 'y'))
    { printf("Parameter 'dm' is not in use!.\n");
      printf("Set dm='nnn' and BLdec='y' to turn on decoupling during acquisition.\n");
      psg_abort(1); }

/* BEGIN ACTUAL PULSE SEQUENCE CODE */

status(A);

   hsdelay(d1);

status(B);

   pulse(p1, zero);
   hsdelay(d2);
   
   rgpulse(pw, oph, rof1, rof2);

status(C);

   if (BLdec[A] == 'y')
     acquireBL(decbw, ref_pw90, ref_pwr);
}

