// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* vast1d (subset of lc1d) */
/* modified for t1b1 case, dz, and waltz   */

#include <standard.h>

static void composite_pulse(double width, codeint phasetable,
                            double rx1, double rx2, codeint phase)
{
  getelem(phasetable,ct,phase); /* Extract observe phase from table */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
}

void pulsesequence()
{
  /* DECLARE & READ IN NEW PARAMETERS */
  
  char   compshape[MAXSTR];
  getstr("compshape",compshape);    /* Composit pulse shape  */

  loadtable("lc1d");              /* Phase table                   */

  /* PULSE SEQUENCE */

  status(A);

    hsdelay(d1);

  status(B);

    if (getflag("wet")) wet4(t1,t2);
  status(C); 

    if (getflag("composit")) 
    {
       if (rfwg[OBSch-1] == 'y')
          shaped_pulse(compshape,4.0*pw+0.8e-6,t3,rof1,rof2);
       else
          composite_pulse(pw,t3,rof1,rof2,v1);
    }
    else
       rgpulse(pw,t3,rof1,rof2);
    setreceiver(t4);
}
