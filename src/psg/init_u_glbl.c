/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "oopc.h"
#include  "acqparms.h"

extern int bgflag;

/*-------  Global Parameter ------------------
| Define any global parameters here, then
| define them as extern in our pulse sequence
| to use them.
+---------------------------------------------*/

/*--------------------------------------------
| init_user_globals():
|    This routine is for users that want to 
|  initialize global parameters and/or Objects
|  for the main pulse sequence.
|  Change the follow function to suit your needs.
+--------------------------------------------*/
int init_user_globals()
{
  return(0);
}
