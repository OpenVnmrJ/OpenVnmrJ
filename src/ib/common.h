/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _COMMON_H
#define	_COMMON_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

/************************************************************************
*                                                                       *
* Swap between minimum and maximum value.  The result will guarantee 	*
* that val1 <= val2.							*
*									*/
extern void
com_minmax_swap(short &val1, short &val2);

/************************************************************************
*                                                                       *
* Return TRUE if a point is inside a rectangle or FALSE if it is not.	*
*									*/
extern int
com_point_in_rect(short x, short y,	// Point to be checked
	short x1, short y1,		// upper-left corner
	short x2, short y2);		// lower-right corner

/************************************************************************
*                                                                       *
*  Cut-off the length of the string at the fron abd add "..." at the    * 
*  front of the string.  Maximum the resulting string should not be     *
*  greater than 80 characters.                                          *
*  For example:                                                         *
*     ("this is just a test", 6) will return                            * 
*       "...a test".                                                    * 
*  Return a pointer to the resulting string.                            *
*                                                                       */ 
extern char * 
com_clip_len_front(char *str, 		// string to be clipped
		   int len);		// resulting length Maximum is 80

#endif (not) _COMMON_H
