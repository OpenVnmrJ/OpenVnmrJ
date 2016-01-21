/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

satpulse(duration,phase,rx1,rx2)
double duration, rx1, rx2;
codeint phase;
{
  double satpwr,
	 satfrq;
  satpwr = getval("satpwr");
  satfrq = getval("satfrq");

       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(duration,phase,rx1,rx2);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);

}
