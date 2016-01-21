/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*  Miscellaneous routines.						*
*									*
*************************************************************************/
#include <string.h>
#include "stderr.h"

/************************************************************************
*									*
*  Swap two values for if val1 > val2.					*
*									*/
void
com_minmax_swap(short &val1, short &val2)
{
   if (val1 > val2)
   {
      short temp;
      temp = val1;
      val1 = val2;
      val2 = temp;
   }
   else
      WARNING_OFF(Sid);
}

/************************************************************************
*									*
*  Return TRUE if a point is in the rectangle else FALSE.		*
*									*/
int
com_point_in_rect(short x, short y,	// a point to be checked
	short x1, short y1,		// upper left corner
	short x2, short y2)		// upper right corner
{
   if ((x < x1) || (x > x2) || (y < y1) || (y > y2))
      return (FALSE);
   else
      return (TRUE);
}

/************************************************************************
*									*
*  Cut-off the length of the string at the fron abd add "..." at the	*
*  front of the string.  Maximum the resulting string should not be 	*
*  greater than 80 characters.						*
*  For example:								*
*     ("this is just a test", 6) will return 				*
*	"...a test".							*
*  Return a pointer to the resulting string.				*
*									*/
char *
com_clip_len_front(char *str, int len)
{
   static char strbuf[85];	// buffer for resulting string
   int slen = strlen(str);

   // Check the limit
   if (len < 0)
      len = 0;
   else if (len > 80)
      len = 80;
      
   if (slen > len)
      (void)sprintf(strbuf,"...%s", str + slen - len);
   else
      (void)strcpy(strbuf, str);
   return(strbuf);
}
