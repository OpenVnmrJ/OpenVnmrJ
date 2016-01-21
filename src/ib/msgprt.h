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

#ifndef _MSGPRT_H
#define	_MSGPRT_H
/*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/
#include <xview/xview.h>

/************************************************************************
*									*
*  Create message windows msgerr for error message, msginfo for
*  information message, and msgmacro for macro editing.
*									*/
extern void msgerr_win_create(Frame owner, /* Owner (Should not be NULL) */
	int x, int y,		/* starting position of message window */
	int wd, int ht);	/* width and height of window */
extern void msginfo_win_create(Frame owner, /* Owner (Should not be NULL) */
	int x, int y,		/* starting position of message window */
	int wd, int ht);	/* width and height of window */
extern void msgmacro_win_create(Frame owner, /* Owner (Should not be NULL) */
	int x, int y,		/* starting position of message window */
	int wd, int ht);	/* width and height of window */

/************************************************************************
*									*
*  Output the error/info message into the window.  It follows the 	*
*  standard "printf" format.  Note that it always output the 'newline' 	*
*  and a "bell" sound for every "msgerr_print", and it doesn't for	*
*  "msginfo_print".							*
*									*/
extern void msgerr_print_string(char *);
extern void msgerr_print(char *, ...);
extern void msginfo_print_string(char *);
extern void msginfo_print(char *, ...);
extern void msgmacro_print(char *, ...);

/************************************************************************
*                                                                       *
*  Write messages into a file and clear the window.               	*
*  If filename is NULL, it doesn't write messages into filename.        *
*  Return OK for success or NOT_OK for failure.                        	*
*                                                                       */
extern int msgerr_write(char *filename);
extern int msginfo_write(char *filename);
extern int msgmacro_write(char *filename);


/*********************************************************************
*
*   Macro for magerr_print & msginfo_print
*
*/
#define	MSGINFO(str)\
	   (void)msginfo_print(" %s\n", str);

#define	MSGERR(str)\
	   (void)msgerr_print(" %s\n", str);

#define	MSGERR_0(str)\
	   (void)msgerr_print("%s: %d: %s\n",__FILE__, __LINE__, str);
#define	MSGERR_1(str,arg1)\
	   (void)msgerr_print("%s: %d: "str"\n",__FILE__, __LINE__, arg1);
#define	MSGERR_2(str,arg1,arg2)\
	   (void)msgerr_print("%s: %d: "str"\n",__FILE__, __LINE__, arg1, arg2);
#define	MSGERR_3(str,arg1,arg2,arg3)\
	   (void)msgerr_print("%s: %d: "str"\n",__FILE__, __LINE__, arg1, arg2, arg3);

#endif	_MSGPRT_H
