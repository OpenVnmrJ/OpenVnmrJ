/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------
|  You are responsible for any any fifo words generated to be outputed 
|  failure to do so may result in unpredictable and possibly destructive
|  settings of the hardware!!!!  see pre_exp_seq.c for example
+----------------------------------------------------------------------*/
/*------------------------------------------------
|  pre_expsequence():
|  This routine is for users that want to preform
|  functions once prior to the experiment.
+------------------------------------------------*/
pre_expsequence()
{
  extern int ap_interface;

  if (ap_interface < 4)
  {
  /* set spectrometer for reverse mode, i.e, decoupler to observe channel. */
  /* included in pulse sequence by  #include "revmode.c" */
     OBSch = DODEV;
     DECch = TODEV;
     set_observech(DODEV);
  /* now set them back so pulse elements are not switched, that is, */
  /* decrgpulse still comes out on decoupler                        */
     OBSch = TODEV;
     DECch = DODEV;
  }
  return;
}
