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
* Info message windows.  (Most of the codes are borrowed from Vnmr.)	*
*   Error message window						*
*   User information message window					*
*									*
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
/* Note that xview.h uses K&R C varargs.h which      */
/* is not compatible with stdarg.h for C++ or ANSI C. */

#ifdef __OBJECTCENTER__
#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#endif
#include <stdarg.h>
/* #include <varargs.h> */
#endif

#include <stdlib.h>
#include "stderr.h"

typedef struct _wmsgprt
{
   Frame owner;
   Frame popup;
   Textsw textsw;
   int x, y;		/* starting position */
   int wd, ht;		/* width and height */
} Wmsgprt;

static Wmsgprt *werrmsg=NULL;
static Wmsgprt *winfomsg=NULL;
static Wmsgprt *wmacromsg=NULL;

void msgerr_win_create(Frame, int, int, int, int);
void msginfo_win_create(Frame, int, int, int, int);
void msgmacro_win_create(Frame, int, int, int, int);
void msgerr_print(char *, ...);
void msginfo_print(char *, ...);
void msgmacro_print(char *, ...);
int msgerr_write(char *);
int msginfo_write(char *);
int msgmacro_write(char *);
static void msgprt_win_create(Wmsgprt *);
static int remove_old_msg(Textsw, int, int, int);
static int reset_textsw(Textsw, Textsw_index, int);
static void show_msg_text(Textsw, char *);

/************************************************************************
*									*
*  Create error message window. It actually creates it when the first	*
*  message is displayed.
*									*/
void
msgerr_win_create(Frame owner, int x, int y, int wd, int ht)
{
   if (werrmsg)
   {
      STDERR("msgerr_win_create:Window has been created");
      return;
   }

   if ((werrmsg = (Wmsgprt *)malloc(sizeof(Wmsgprt))) == NULL)
   {
      PERROR("msgerr_win_create:Cannot malloc");
      return;
   }

   werrmsg->owner = owner;
   werrmsg->popup = NULL;
   werrmsg->textsw = NULL;
   werrmsg->x = x;
   werrmsg->y = y;
   werrmsg->wd = wd;
   werrmsg->ht = ht;

   /* Since there is a BUG in Xview, we cannot wait to create a window. */
   /* Until the bug is fixed, we create a window at start-up time (NOW). */
   msgprt_win_create(werrmsg);
   xv_set(werrmsg->popup, FRAME_LABEL, "Error Messages", NULL);
}

/************************************************************************
*									*
*  Create info message window. It actually creates it when the first   	*
*  time message is to be displyed.                                      *
*									*/
void
msginfo_win_create(Frame owner, int x, int y, int wd, int ht)
{
   if (winfomsg)
   {
      STDERR("msginfo_win_create:Window has been created");
      return;
   }

   if ((winfomsg = (Wmsgprt *)malloc(sizeof(Wmsgprt))) == NULL)
   {
      PERROR("msginfo_win_create:Cannot malloc");
      return;
   }
   winfomsg->owner = owner;
   winfomsg->popup = NULL;
   winfomsg->textsw = NULL;
   winfomsg->x = x;
   winfomsg->y = y;
   winfomsg->wd = wd;
   winfomsg->ht = ht;

   /* Since there is a BUG in Xview, we cannot wait to create a window. */
   /* Until the bug is fixed, we create a window at start-up time (NOW). */
   msgprt_win_create(winfomsg);
   xv_set(winfomsg->popup, FRAME_LABEL, "Info Messages", NULL);
   xv_set(winfomsg->textsw,
	  TEXTSW_BROWSING, FALSE,
	  WIN_KBD_FOCUS, TRUE,
	  NULL);
}

/************************************************************************
*									*
*  Create macro display window. It actually creates it when the first
*  message is displayed.
*									*/
void
msgmacro_win_create(Frame owner, int x, int y, int wd, int ht)
{
   if (wmacromsg)
   {
      STDERR("msgmacro_win_create:Window has been created");
      return;
   }

   if ((wmacromsg = (Wmsgprt *)malloc(sizeof(Wmsgprt))) == NULL)
   {
      PERROR("msgmacro_win_create:Cannot malloc");
      return;
   }
   wmacromsg->owner = owner;
   wmacromsg->popup = NULL;
   wmacromsg->textsw = NULL;
   wmacromsg->x = x;
   wmacromsg->y = y;
   wmacromsg->wd = wd;
   wmacromsg->ht = ht;

   /* Since there is a BUG in Xview, we cannot wait to create a window. */
   /* Until the bug is fixed, we create a window at start-up time (NOW). */
   msgprt_win_create(wmacromsg);
   xv_set(wmacromsg->popup,
	  FRAME_LABEL, "Macro Edit",
	  NULL);
   xv_set(wmacromsg->textsw,
	  TEXTSW_BROWSING, FALSE,
	  WIN_KBD_FOCUS, TRUE,
	  NULL);
}

/************************************************************************
*									*
*  Show single string error message.
*  Note that it always prefix the message with a newline.		*
*									*/
void
msgerr_print_string(char *str)
{
    msgerr_print("%s", str);
}

/************************************************************************
*									*
*  Show Error message.							*
*  Note that it always prefix the message with a newline.		*
*									*/
void
msgerr_print(char *format, ...)
{
   char msgbuf[128];	/* message buffer */
   va_list vargs;	/* variable argument pointer */

   /* Create window message if it is not created */
   if (werrmsg->popup == NULL)
   {
      msgprt_win_create(werrmsg);
      xv_set(werrmsg->popup, FRAME_LABEL, "Error Messages", NULL);
   }

   if ( (xv_get(werrmsg->popup, XV_SHOW) == FALSE) || (format == NULL) ){
      xv_set(werrmsg->popup,	
		FRAME_CMD_PUSHPIN_IN,	TRUE,
		XV_SHOW,		TRUE,
		NULL);
   }

   if (format == NULL)
      return;
   
   /* Always prefix the message with a newline */
   msgbuf[0] = '\n';

   va_start(vargs, format);
   (void)vsprintf(msgbuf+1, format, vargs);
   va_end(vargs);

   XBell((Display *)xv_get(werrmsg->popup, XV_DISPLAY), 0);

   int point = (int)xv_get(werrmsg->textsw, TEXTSW_INSERTION_POINT);
   show_msg_text(werrmsg->textsw, msgbuf);
   if (point == 0){
       xv_set(werrmsg->textsw, TEXTSW_FIRST, point, NULL);
   }
}

/************************************************************************
*									*
*  Show single string info message.
*  Note that it DOES NOT prefix the message with a newline.		*
*									*/
void
msginfo_print_string(char *str)
{
    msginfo_print("%s", str);
}

/************************************************************************
*									*
*  Show Info message.							*
*  Note that it DOES NOT prefix the message with a newline.		*
*									*/
void
msginfo_print(char *format, ...)
{
   char msgbuf[128];	/* message buffer */
   va_list vargs;	/* variable argument pointer */

   /* Create window message if it is not created */
   if (winfomsg->popup == NULL)
   {
      msgprt_win_create(winfomsg);
      xv_set(winfomsg->popup, FRAME_LABEL, "Info Messages", NULL);
   }

   if ( (xv_get(winfomsg->popup, XV_SHOW) == FALSE) || (format == NULL) ){
      xv_set(winfomsg->popup, XV_SHOW, TRUE, NULL);
      xv_set(winfomsg->popup, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
   }

   if (format == NULL)
      return;
   
   va_start(vargs, format);
   (void)vsprintf(msgbuf, format, vargs);
   va_end(vargs);

   int point = (int)xv_get(winfomsg->textsw, TEXTSW_INSERTION_POINT);
   show_msg_text(winfomsg->textsw, msgbuf);
   if (point == 0){
       xv_set(winfomsg->textsw, TEXTSW_FIRST, point, NULL);
   }
}

/************************************************************************
*									*
*  Put a line in the macro window without making it visible.
*  If "format" is NULL, just make it visible.
*									*/
void
msgmacro_print(char *format, ...)
{
   char msgbuf[128];	/* message buffer */
   va_list vargs;	/* variable argument pointer */

   /* Create window message if it is not created */
   if (wmacromsg->popup == NULL)
   {
      msgprt_win_create(wmacromsg);
      xv_set(wmacromsg->popup,
	     FRAME_LABEL, "Macro Edit",
	     NULL);
      xv_set(wmacromsg->textsw,
	     TEXTSW_BROWSING, FALSE,
	     WIN_KBD_FOCUS, TRUE,
	     NULL);
  }

   if (format == NULL){
      xv_set(wmacromsg->popup, XV_SHOW, TRUE, NULL);
      xv_set(wmacromsg->popup, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
   }

   if (format == NULL)
      return;
   
   va_start(vargs, format);
   (void)vsprintf(msgbuf, format, vargs);
   va_end(vargs);

   int point = (int)xv_get(wmacromsg->textsw, TEXTSW_INSERTION_POINT);
   show_msg_text(wmacromsg->textsw, msgbuf);
   if (point == 0){
       xv_set(wmacromsg->textsw, TEXTSW_FIRST, point, NULL);
   }
}

/************************************************************************
*									*
*  Write error messages into a file and clear the window.		*
*  If filename is NULL, it doesn't write messages into filename.	*
*  Return OK or NOT_OK.							*
*									*/
int
msgerr_write(char *filename)
{
   if (filename)
   {
      if (textsw_store_file(werrmsg->textsw, filename, 0, 0) != 0)
	 return(NOT_OK);
   }
   textsw_reset(werrmsg->textsw, 0, 0);
   return(OK);
}

/************************************************************************
*									*
*  Write info messages into a file and clear the window.		*
*  If filename is NULL, it doesn't write messages into filename.	*
*  Return OK or NOT_OK.							*
*									*/
int
msginfo_write(char *filename)
{
   if (filename)
   {
      if (textsw_store_file(winfomsg->textsw, filename, 0, 0) != 0)
	 return(NOT_OK);
   }
   textsw_reset(winfomsg->textsw, 0, 0);
   return(OK);
}

/************************************************************************
*									*
*  Create info print-message window.					*
*									*/
static void
msgprt_win_create(Wmsgprt *wmsg)
{
   if (wmsg->owner == NULL)
   {
      STDERR("msgprt_win_create:Need a parent for this window");
      return;
   }

   if (wmsg->wd == 0)
      wmsg->wd = 300;
   if (wmsg->ht == 0)
      wmsg->ht = 100;

   wmsg->popup = xv_create(wmsg->owner,	FRAME_CMD,
	FRAME_LABEL,	"Message-Window",
	XV_X,		wmsg->x,
	XV_Y,		wmsg->y,
	XV_WIDTH,	wmsg->wd,
	XV_HEIGHT,	wmsg->ht,
	FRAME_SHOW_RESIZE_CORNER, TRUE,
	NULL);

   /* Destroy the default created panel */
   // (This causes an Xview warning when we set XV_SHOW)
   //xv_destroy_safe(xv_get(wmsg->popup,FRAME_CMD_PANEL));

   wmsg->textsw = xv_create (wmsg->popup, TEXTSW,
                TEXTSW_INSERT_MAKES_VISIBLE, TEXTSW_ALWAYS,
                TEXTSW_DISABLE_LOAD, TRUE,
                TEXTSW_DISABLE_CD, TRUE,
                TEXTSW_BROWSING, TRUE,
		TEXTSW_MEMORY_MAXIMUM,1000000,
                /*TEXTSW_AGAIN_RECORDING, FALSE,*/
                TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
                WIN_X, 0,
		WIN_Y, 0,
		WIN_WIDTH,	WIN_EXTEND_TO_EDGE,
		WIN_HEIGHT,	WIN_EXTEND_TO_EDGE,
                WIN_KBD_FOCUS, FALSE,
                /*WIN_IGNORE_KBD_EVENT, WIN_NO_EVENTS,*/
                0);
}

/**********************************************************************
 remove_old_msg

 Remove the oldest newline-delimited message(s) from the specified text
 sub-window, in order to free enough memory to hold a new message.
 (DEW 890525)

 INPUT ARGS:
   msg_textsw  A SunView window handle for the text sub-window where the
               message is to be displayed.
   msg_len     The length of the message to be inserted in the text window;
               that is, the amount of text sub-window memory that needs to
               be freed.
   text_len    The number of characters currently used by the text sub-
               window's displayed text.
   max_len     The number of characters available in the text sub-window's
               memory allocation.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   FALSE       Not enough memory could be freed by removing old message(s).
   reset_textsw   The result of the text sub-window reset operation:
                  TRUE  => reset succeeded;
                  FALSE => reset failed.
 ERRORS:
   no memory   If there isn't enough memory (remaining) in the text sub-
               window to display the message string, then an error message
               is printed at stderr.
 GLOBALS:
   none
***********************************************************************/
static int
remove_old_msg(Textsw msg_textsw,
 	int    msg_len,
 	int    text_len,
 	int    max_len)
{
   /********************************************************************
   LOCAL VARIABLES:
 
   current  The index of the start of the current oldest message in the text.
   search   The index in the text where the search for the end of the current
            message is to begin.
   next     The start of the next message following the current one; that
	    is, the next character past the newline delimiting the current
	    message.
   avail    The amount of memory in the text sub-window available for
	    inserting new messages; this is incremented for each old message
	    that would be eliminated.
   *********************************************************************/
 
   Textsw_index current,
                search,
                next;
   int avail;
 
   /* starting with the first character in the text sub-window, find the
      oldest message(s) until enough memory will be freed to hold the
      requested message */
 
   for (avail = max_len - text_len, current = (Textsw_index)0;
        msg_len >= avail;
        avail += (int)(next - current), current = next) {
 
      /* find a newline in the text */
 
      search = current;
      if (textsw_find_bytes (msg_textsw, &search, &next, "\n", 1, 0) == -1 ||
          search <= current)
         return (FALSE);
   }
 
   /* reset the text sub-window, eliminating the oldest message(s) in the
      process */
 
   return (reset_textsw (msg_textsw, next, text_len - (int)next));
}

/**********************************************************************
 reset_textsw
 
 Reset the text sub-window; this keeps the edit log buffer, which has the
 same memory limitation as the sub-window (???), from filling up and causing
 a "text insertion failed" message. Any text which must be restored after the
 reset has to be saved, since the SunView reset function clears the sub
 -window memory. (DEW 890525)
 
 INPUT ARGS:
   msg_textsw  The handle for the message text sub-window.
   beg_index   The text index of the start of the text to be restored.
   text_len    The length of the text to be restored.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   TRUE        Reset succeeded.
   FALSE       Reset failed because of error.
 ERRORS:
   FALSE       Couldn't allocate sufficient memory to save text (malloc); a
               message is sent to stderr, since the usual window for
	       displaying error messages may be the one that this routine
	       is trying to reset.
               Couldn't insert saved text back into sub-window
	       (textsw_insert).
 GLOBALS:
   none
 ***********************************************************************/
 
static int
reset_textsw (Textsw msg_textsw,
 	Textsw_index beg_index,
 	int text_len)
{
   /********************************************************************
   LOCAL VARIABLES:
 
   p_text         Pointer to space allocated for saving the text to be
                  restored after the reset.
   ret_val        Contains the result of the operation: TRUE or FALSE.
   *********************************************************************/
 
   char *p_text;
   int   ret_val = TRUE;
 
   /* allocate sufficient memory to store the text currently in the window */
 
   if ( (p_text = (char *)malloc (text_len + 1)) == NULL) {
      STDERR ("reset_text_window: malloc error");
      return (FALSE);
   }
 
   /* get the text from the window; the extra character in the length insures
      that the string will be NULL-terminated */
 
   xv_get (msg_textsw, TEXTSW_CONTENTS, beg_index, p_text, text_len+1);
 
   /* reset the text sub-window, which clears the edit log buffer */
 
   textsw_reset (msg_textsw, 0, 0);
 
   /* restore the message text */
 
   if (textsw_insert (msg_textsw, p_text, text_len) != text_len)
      ret_val = FALSE;
 
   /* free the memory allocated */
 
   free (p_text);
 
   return (ret_val);
}
 
/**********************************************************************
 show_msg_text
 
 Display a message string in the specified text sub-window. (DEW 890525)

 INPUT ARGS:
   msg_textsw  A SunView window handle for the text sub-window where the
               message is to be displayed.
   msg_text    A pointer to the message string to be displayed.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 ERRORS:
   no memory   If there isn't enough memory (remaining) in the text sub-
               window to display the message string, then an error message
               is printed at stderr.
 GLOBALS:
   none
***********************************************************************/
#define MAX_MSG_LEN 240
#define WIN_OVERHEAD 30

static void
show_msg_text ( Textsw msg_textsw,
 	char  *msg_text)
{
   /********************************************************************
   LOCAL VARIABLES:

   msg            A buffer for prefixing the message text with a newline.
   max_len        The number of characters available in the text sub-window's
                  memory allocation.
   text_len       The number of characters currently used by the text sub-
                  window's displayed text.
   msg_len        The length of the message to be displayed, including the
                  added newline if present.
   insert         The insertion point of the new message in the text sub-
                  window; used to scroll the sub-window, if necessary, to
                  display (at least the start of) the new message. This needs
                  worrying about since the sub-window will NOT automatically
                  scroll up to display the new message, if it is already
                  scrolled up so that the current last line is not displayed
                  in the window!
   ***********************************************************************/
   char msg[MAX_MSG_LEN + 1];
   int  max_len,
        text_len,
        msg_len;
   Textsw_index insert;

   /* get the number of characters available in the text sub-window's memory
      allocation, and the number of characters already in the window */

   max_len  = (int)xv_get (msg_textsw, TEXTSW_MEMORY_MAXIMUM) - WIN_OVERHEAD;
   text_len = (int)xv_get (msg_textsw, TEXTSW_LENGTH);

   /* format the message for display: be sure that message */
   /* is NULL-terminated */

   msg [MAX_MSG_LEN] = '\0';

   /* The first message should not start with a new-line */
   if (text_len == 0) {
      if (msg_text[0] == '\n')
         (void)strncpy (msg, msg_text+1, MAX_MSG_LEN);
      else
         (void)strncpy (msg, msg_text, MAX_MSG_LEN);
   }
   else
      strncpy (msg, msg_text, MAX_MSG_LEN);

   /* if there is enough memory available in the text sub-window for this
      message, or room can be made by removing the oldest message(s) ... */

   if ( (msg_len = strlen (msg)) < (max_len - text_len) ||
       remove_old_msg (msg_textsw, msg_len, text_len, max_len) == TRUE) {

      /* save the index of the current insertion point in the text; the
         message will be put AFTER this location */

      insert = (Textsw_index)xv_get (msg_textsw, TEXTSW_INSERTION_POINT);

      /* move past the new line, if present, in order to display (at least
         the start of) the new message */

      if (msg[0] == '\n')
         ++insert;
 
      /* display this message in the text sub-window */

      textsw_insert (msg_textsw, msg, msg_len);

      /* normalize, if needed, the display of the messages in the window
         so that (at least the start of) the new message is displayed */

      /*
#define	_OTHER_TEXTSW_FUNCTIONS
      textsw_possibly_normalize (msg_textsw, insert);
      */
   }
   else {
      STDERR_1("Message window won't hold the following:%s", msg);
   }
}
