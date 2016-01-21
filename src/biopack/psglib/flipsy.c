/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*flipsi-   presaturation with rejection of regions of poor rf homogeneity
            Neuhaus, Ismail and Chung, J. Magn. Reson.A, 118, 256-263(1995).
            Also see: Lauridsen et.al, Anal.Chem. 80, 3365-3371(2008)   

            First pulse can be 0-90 degrees and set at the Ernst angle if 
            desired.
*/

#include <standard.h>

static int 
           phs22[1] = {0},
           phs23[4] = {0,1,2,3},
           phs25[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
           phs26[8] = {0,2,0,2,2,0,2,0};
		
pulsesequence()
{
 double gstab,gzlvl1,gt1;
   gzlvl1=getval("gzlvl1");
   gt1=getval("gt1");
   gstab=getval("gstab");

  assign(ct,v17);

   settable(t2,1,phs22);
   settable(t3,4,phs23);
   settable(t5,16,phs25);
   settable(t6,8,phs26);
  
   getelem(t2,v17,v1);   /* v1 - first 90 */
   getelem(t3,v17,v2);   /* v2 - 2nd 90 */
   getelem(t5,v17,v3);   /* v3 - 3rd 90 */
   getelem(t6,v17,oph);  /* oph - receiver */

   status(A);
   obspower(satpwr);

   rgpulse(satdly,v6,rof1,rof1);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   obspower(tpwr);

   rgpulse(pw, v1, rof1, 0.0);
   rgpulse(p1, v2, 1.0e-6, 0.0);
   rgpulse(p1, v3, 1.0e-6, rof2);
status(C);
}
