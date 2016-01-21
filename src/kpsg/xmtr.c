/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
#include "rfconst.h"

#define CTXON   16                     /* 13C tx gate line */
#define HTXON   128                    /*  1H tx gate line */
extern int	cardb;
extern int	indirect;

/* the following functions work only if they are followed by a delay */
/* they are here in HMQC					     */
decon()
{
   if (!indirect)
      gate(HTXON,TRUE);
   else
      gate(CTXON,TRUE);
}

xmtron()
{
   if (cardb)
      gate(CTXON,TRUE);
   else
      gate(HTXON,TRUE);
}

decoff()
{
   if (!indirect)
      gate(HTXON,FALSE);
   else
      gate(CTXON,FALSE);
}

xmtroff()
{
   if (cardb)
      gate(CTXON,FALSE);
   else
      gate(HTXON,FALSE);
}

