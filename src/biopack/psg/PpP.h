/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PpP.h  - ProteinPack - Pbox include file */

#include <Pbox_psg.h>

shape pbox_make(shn, wvn, pw_bw, ofs, rf_pwr, rf_pw90)
char   *shn, *wvn;
double pw_bw, ofs, rf_pwr, rf_pw90;
{
  char   txt[MAXSTR],
         cmd[MAXSTR];
  shape  sh;
       
  sprintf(txt, "Pbox %s -w \"%s %.7f %.1f\" ", shn, wvn, pw_bw, ofs);
  sprintf(txt, "%s -p %.0f -l %.2f ", txt, rf_pwr, 1.0e6*rf_pw90);
  sprintf(cmd, "%s -attn %.0f%c -maxincr 10.0\n", txt, rf_pwr, 'E');
  printf("cmd = %s\n", cmd);
  system(cmd);                                     /* execute Pbox */

  sh = getRsh(shn);

  if (sh.pwrf > 4095.0) 
  {
    sprintf(cmd, "%s -attn %.0f%c -maxincr 10.0\n", txt, rf_pwr, 'd');
    system(cmd);                                   /* execute Pbox */
    sh = getRsh(shn);
    if (sh.pwrf > 4095.0) 
    {
      printf("pbox_make : power error, pwrf = %.0f\n ", sh.pwrf);
      psg_abort(1);
    }
  }

  return sh;
}

