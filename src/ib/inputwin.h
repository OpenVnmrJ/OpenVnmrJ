/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/* 
 */

#ifndef _INPUTWIN_H
#define _INPUTWIN_H
/************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/
#include <xview/xview.h>

/************************************************************************
*                                                                       *
*  Create user input window.  It creates a window which the user can	*
*  type in an input string.						*
*                                                                       */
extern void inputwin_create(
	Frame owner,		/* owner of input window */
	char *objname);		/* Object name in initialization file */

/************************************************************************
*                                                                       *
*  Show input window.                                                   *
*  Note that it also registers the callback function, and sets input    *
*  window header.                                                       *
*  The callback_func will return the user's ID and type-in input value 	*
*  in its arguments.  Then the user can return OK or NOT_OK from the	*
*  callback-function.							*
*  (Look at stderr.h for value OK and NOT_OK).				*
*                                                                       */
extern void inputwin_show(
	int user_id,		/* user ID to be returned in callback func */
	int (*func)(int, char *),/* callback function */
	char *header);		/* window label */

/************************************************************************
*                                                                       *
*  Show something in the input window.                                  *
*                                                                       */
extern void inputwin_set(float);     


/************************************************************************
*                                                                       *
*  Hide inputwindow.							*
*									*/
extern void inputwin_hide(void);

#endif _INPUTWIN_H
