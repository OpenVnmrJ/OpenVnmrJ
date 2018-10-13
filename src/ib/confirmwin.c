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
*									*
*  Routine related to popup confirmer window.				*
*									*
*************************************************************************/
#include "stderr.h"
#include <xview/xview.h>
#include <xview/notice.h>

#ifdef __OBJECTCENTER__
/* Note that xview.h uses K&R C varargs.h which        */
/* is not compatible with stdarg.h for C++ or ANSI C.	*/
#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#endif
#include <stdarg.h>
/* #include <varargs.h> */
#endif

#include "confirmwin.h"

/************************************************************************
*									*
*  Popup notice window to ask the confirmation from the user.  		*
*  It blocks all window activities until the user responds to the 	*
*  confirmer window.							*
*									*/
int
confirmwin_popup(char *true_button, char *false_button, ...)
{
   va_list vargs;       /* variable argument pointer */
   int result;		/* result to be returned */
   Frame frame;		/* frame created as the owner of popup-notice */
   char *str[80];	/* Maximum of 80 line messages */
   int argc=0;		/* number of message string */

   WARNING_OFF(Sid);

   /* Get arguments into the argument list */
   va_start(vargs, false_button);
   while (str[argc++] = va_arg(vargs, char *));
   va_end(vargs);

   frame = xv_create(XV_NULL, FRAME, NULL);

   result = notice_prompt(frame,	NULL,
	NOTICE_BUTTON_YES,	true_button,
	NOTICE_BUTTON_NO,	false_button,
	NOTICE_MESSAGE_STRINGS_ARRAY_PTR, str,
	NULL);

   xv_destroy_safe(frame);
   return(result);
}
