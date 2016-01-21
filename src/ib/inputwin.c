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
*									*
*  Routine to pop-up input window so that the user can type in a string.*
*  Then it will return the string to the user's callback function	*
*  which it registered.  It is used as a command-line interface.	*
*  Its usage is very limited, and basically used to change the value	*
*  of a variable.							*
*									*
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include "initstart.h"
#include "stderr.h"

typedef struct _inputwin
{
   Frame popup;			/* popup frame */
   Panel panel;			/* popup panel */
   Panel_item msg;		/* message panel item */
   Panel_item text;		/* message panel text */
   int user_id;			/* user ID */
   int (*callback_func)(int, char *);/* callback function */
} Inputwin;

static Inputwin inputwin;

static void input_notify_proc(Panel_item item);

/************************************************************************
*                                                                       *
*  Create user input window.                                         	*
*                                                                       */
void
inputwin_create(Frame owner, char *objname)
{
   int x_pos, y_pos;    /* position of the window */
   int wd, ht;          /* width and height of the window */
   char initname[128];  /* initialization filename */
 
   /* Get the initialized file name for window position */
   (void)init_get_win_filename(initname);
 
   /* Get the position of error-message window and its size */
   if ((objname == NULL) || (init_get_val(initname, objname, "dddd",
                    &x_pos, &y_pos, &wd, &ht)) == NOT_OK)
   {
      /* Defaults */
      x_pos = 500;
      y_pos = 100;
      wd = 300;
      ht = 50;
   }

   inputwin.callback_func = NULL;
   inputwin.popup = xv_create(owner,	FRAME_CMD,
	FRAME_LABEL,	"Input-Window",
	XV_X,		x_pos,
	XV_Y,		y_pos,
	XV_WIDTH,	wd,
	XV_HEIGHT,	ht,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);

   inputwin.panel = xv_get(inputwin.popup, FRAME_CMD_PANEL);

   inputwin.msg = xv_create(inputwin.panel,	PANEL_MESSAGE,
	XV_X,			4,
	XV_Y,			4,
	PANEL_LABEL_STRING,	"<Message>",
   	NULL);

   inputwin.text = 
   xv_create(inputwin.panel,	PANEL_TEXT,
	XV_X,			4,
	XV_Y,			(int)xv_get(inputwin.msg, XV_HEIGHT) + 11,
	PANEL_LABEL_STRING,	"Input:",
	PANEL_VALUE,		"",
	PANEL_NOTIFY_PROC,	input_notify_proc,
	NULL);
}

/************************************************************************
*                                                                       *
*  Show input window.							*
*  Note that it also registers the callback function, and set input	*
*  window header.							*
*									*/
void
inputwin_show(int user_id, int (*func)(int, char *), char *header)
{
   if (xv_get(inputwin.popup, XV_SHOW) == FALSE)
      xv_set(inputwin.popup, XV_SHOW, TRUE, NULL);

   inputwin.user_id = user_id;
   inputwin.callback_func = func;
   xv_set(inputwin.popup, FRAME_LABEL, header, NULL);
}

/************************************************************************
*                                                                       *
*  Display something in the input space.				*
*  Put here for use by Vs.              				*
*									*/
void
inputwin_set(float value)
{
    char buf[100];
    sprintf(buf,"%.4g", value);
    xv_set(inputwin.text, PANEL_VALUE, buf, NULL);
}

/************************************************************************
*                                                                       *
*  Hide input window.							*
*  Note that it also registers the callback function, and set input	*
*  window header.							*
*									*/
void
inputwin_hide(void)
{
   if (xv_get(inputwin.popup, XV_SHOW) == TRUE)
      xv_set(inputwin.popup, XV_SHOW, FALSE, NULL);
}


/************************************************************************
*                                                                       *
*  User has enterred an input value, notify this function.		*
*
*  TO DO: It would be better for this function to check the pushpin
*	  position, and , if it is out, close the window on finding a
*	  valid value.
*									*/
static void
input_notify_proc(Panel_item item)
{
   if (inputwin.callback_func)
   {
      char *tbuf = (char *)xv_get(item, PANEL_VALUE);
      // This string may be deallocated by future xv_ calls, so copy it.
      char *in_val = new char[strlen(tbuf) + 1];
      strcpy(in_val, tbuf);
      char buf[128];

      if (in_val == NULL)
      {
         (void)sprintf(buf, "Error: No value specified");
         xv_set(inputwin.msg, PANEL_LABEL_STRING, buf, NULL);
	 return;
      }

      while (*in_val == ' ')
	 in_val++;

      if (*in_val == NULL)
	 return;

      if ((*inputwin.callback_func)(inputwin.user_id, in_val) == OK)
      {
         (void)sprintf(buf, "Accept: %s", in_val);
         xv_set(inputwin.msg, PANEL_LABEL_STRING, buf, NULL);

         xv_set(item, PANEL_VALUE, "", NULL);
      }
      else
      {
         (void)sprintf(buf, "Error: %s", in_val);
         xv_set(inputwin.msg, PANEL_LABEL_STRING, buf, NULL);
      }
      delete[] in_val;
   }
}
