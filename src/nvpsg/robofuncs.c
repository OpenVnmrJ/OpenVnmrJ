/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* do to lack of time and resources, all this source goes back into MasterController.cpp   */
/* these routines tested and known to work  3/7/05 gmb */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#ifdef NOT_YET   /* ifdef out entire file */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#include "group.h"
#include "ACode32.h"
#include "acqparms.h"
#include "expDoneCodes.h"

extern int loc;
extern int locActive;
extern int ok2bumpflag;
extern int spinactive;


/********************************/
/*       Get Sample             */
/********************************/
getsamp()
{
  int buffer[16];

  if ( (locActive != 0) && ((setupflag == GO) || (setupflag >= CHANGE)) )
  {
    double traymax, tmpval;
    buffer[0] = loc;
    if (getparmd("traymax","real",GLOBAL,&tmpval,1))
    {
      traymax=0;
    }
    else
    {
      traymax= (int) (tmpval + 0.5);
    }
    buffer[1] = ((traymax == 48) || (traymax == 96)) ? 1 : 0;
    buffer[2] = ok2bumpflag;

    broadcastCodes(GETSAMP,3,buffer);
    /* delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
  }

  return 0;

}


/********************************/
/*       Load Sample            */
/********************************/
loadsamp()
{
  int buffer[16];

  if ( (locActive != 0) && ((setupflag == GO) || (setupflag >= CHANGE)) )
  {
    double traymax, tmpval;

    buffer[0] = loc;

    if (getparmd("traymax","real",GLOBAL,&tmpval,1))
    {
      traymax=0;
    }
    else
    {
      traymax= (int) (tmpval + 0.5);
    }
    buffer[1] = ((traymax == 48) || (traymax == 96)) ? 1 : 0;

    buffer[2] = spinactive;
    buffer[3] = ok2bumpflag;
    broadcastCodes(LOADSAMP,4,buffer);
    /* delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
  } 
  return 0;
}
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#endif
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
