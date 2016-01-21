/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _FRAMELIST_H
#define _FRAMELIST_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Doug Landau
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*  Short description of functions in this file:
*
*  framelist_win_create
*	create a frame-browser window. (Only called once)
*  framelist_win_show
*	pop-up the frame-browser window
*  framelist_notify_func
*	register callback functions
*  framelist_win_destroy
*	destroy frame_browser window
*  framelist_select    
*	select a list entry to be highlighted
*
*	
*************************************************************************/

#include <xview/frame.h>


/************************************************************************
*                                                                       *
*  Create frame list handlers.                                        	*
*  Return OK or NOT_OK.                                                 *
*                                                                       */
extern int
framelist_win_create(Frame owner,
	int xpos, int ypos,	/* Position of the popup window (in pixel)*/
	int framename_wd,	/* the maximum width of framename (in pixel)*/
	int framelist_num);	/* maximum number of viewable frames */

/************************************************************************
*                                                                       *
*  Pop up the file list frame.                                          *
*  The "id" is used to indicate which caller program wants to use the	*
*  frame browser.							*
*                                                                       */
extern void   
framelist_win_show(char *title);    /* Specify the title window label */
				    /*  If NULL, label "Playlist"     */

/************************************************************************
*                                                                       *
*  User register function to call when a new frame is added to the      *
*  playlist.                                                            *
*                                                                       */
void
framelist_notify_func(long addr_okfunc);      /* Function address */


/************************************************************************
*                                                                       *
*  Destroys the frame browser.                                          *
*                                                                       */
void
framelist_win_destroy(void);


/************************************************************************
*                                                                       *
*  Destroys the frame browser.                                          *
*                                                                       */
extern void
framelist_select(int);

void
frame_replace_vs(int, double);

#endif _FRAMELIST_H
