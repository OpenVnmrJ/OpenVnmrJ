/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include	<string.h>
#include	"acodes.h"
#include	"acqparms.h"
extern void   getstr(const char *variable, char buf[]);

#define	SMPL	0
#define	HOLD	1
#define TRUE	1
#define FALSE	0

#ifndef NVPSG
#include	"abort.h"
extern int putcode();
extern void HSgate(int ch, int state);
extern int	ap_interface;
void lk_sample()
{
   if (ap_interface!=4)
   { 
      /* an error message could be displayed but they probably already
       * got one from using lk_hold()
       */
      return;
   }
   putcode(SMPL_HOLD);
   putcode(SMPL);
}

void lk_hold()
{
   static int errorsent = 0;
   if (ap_interface!=4)
   { 
      if (!errorsent)
      {
         text_error("lk_hold is not valid with this hardware configuration\n");
         text_error("lk_hold is ignored\n");
         errorsent = 1;
      }
      return;
   }
   putcode(SMPL_HOLD);
   putcode(HOLD);
}

void lk_hslines()
{
   HSgate(0x2000000,TRUE);	/* Set LK RCVR line to 1 = OFF */
   putcode(APBOUT);
   putcode(1);
   putcode(0xab51);
   putcode(0xbb3a);		/* follow chan 6 HS lines for XMTR and RCVR */
}

void lk_autotrig()
{
   HSgate(0x2000000,FALSE);	/* Reset LK RCVR line to 0, as always */
   putcode(APBOUT);
   putcode(1);
   putcode(0xab51);
   putcode(0xbb0a);		/* do 2 kHz auto mode for XMTR and RCVR */
}

void lk_sampling_off()		/* turn off lock sampling */
{
   putcode(APBOUT);
   putcode(1);
   putcode(0xab51);
   putcode(0xbb4a);
}
#endif

void init_lkrelay()
{
    char tmpstr[MAXSTR];
    getstr("tn", tmpstr);
    if (strcmp(tmpstr,"lk")==0)
    {
      lk_hold();
      lk_sampling_off();
    }
}
