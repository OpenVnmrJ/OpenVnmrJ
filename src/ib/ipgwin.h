/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _IPGWIN_H
#define _IPGWIN_H

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

#include <sys/types.h>

/* Structure for message ID */
/* Note that this structure can be changed.  However, changing this */
/* structure would require to change the XDR represantation of this */
/* structure.							    */
typedef struct _ipgmsg
{
   u_long major_id;	/* Major message ID */
   u_long minor_id;	/* Minor message ID */
   char *msgbuf;	/* message string terminated with a NULL */
} Ipgmsg;

/************************************************************************
*                                                                       *
* As a substitute for xv_main_loop, this routine will detect XView	*
* events,  IPG message arrival, and user-registered function.  When it	*
* detects any of these events,  it will call an approprite function (if	*
* the user has registered a function).					*
* This routine will never return until it is terminated.		*
*                                                                       */
extern void
ipgwin_main_loop(unsigned long);

/************************************************************************
*                                                                       *
* Terminate ipgwin_main_loop.                                           *  
*                                                                       */
extern void
ipgwin_exit_loop(void);

/************************************************************************
*                                                                       *
*  This routine is to register a callback user-function every interval	*
*  time as specified.  							*
*  Assume that no window event occurs,  the shortest possible of the 	*
*  delay time is 100 usec.						*
*  User function to be called can be NULL, indicating unregistering     *
*  user-function, or user-function address which takes a form of        *
*  func(int id)                                                         *
*									*
*  Parameters:								*
*	user_id: user sender ID.					*
*	delay_sec, delay_usec: interval time to call a function.	*
*	func : user function to be called.				*
*                                                                       */
extern void
ipgwin_register_user_func(
	int user_id,		/* User ID */
	long delay_sec,		/* delay time in second */
	long delay_usec,	/* delay time in micro-second */
	void (*func)(int));	/* User function to be called */

/************************************************************************
*                                                                       *
*  This routine is used to register a specific function which will be   * 
*  called when a message arrives.  The user needs a unique program-	*
*  number with its version-number so that the sender knows this 	*
*  destination.  (The sender will use the same program-number and 	*
*  version number as these numbers.)					*
*  User function to be called can be NULL, indicating unregistering 	*
*  message, or user-function address.					*
*									*
*  Parameters:								*
*	prognum: is suggested to be in the range 0x20000000 - 0x5fffffff*
*		 If it is 0, the routine will select a number for it.	*
*	versnum: version number 					*
*       func(u_long proc_id,  where proc_id is ID number.		*
*	     Ipgmsg *msg)     where msg is a message.			*
*                                                                       * 
*  Return program number (prognum) for success and 0 for failure.       *
*                                                                       */
extern u_long
ipgwin_register_msg_receive(
	u_long prognum, 		/* program number*/
    	u_long versnum,			/* version number */
 	void (*func)(u_long, Ipgmsg *));	/* User function */

/************************************************************************
*                                                                       *
*  Unregister all receiver message ports.                               *
*                                                                       */
extern void
ipgwin_unregister_msg(void);

/************************************************************************
*                                                                       *
*  This routine is used to register a message for sending. After        *
*  registering,  the user will refer to "send_id" when sending a        *
*  message.                                                             *
*  The message can be sent to any host by specifying the hostname.  If  *
*  hostname is NULL,  this routine will unregister the message.         *
*									*
*  Parameters:								*
*	send_id: user sender ID.  The user will refer to this ID when 	*
*	         sending a message.					*
* 	prognum: program number should be exactly the same as that of	*
*                receiver.						*
* 	versnum: version number should be exactly the same as that of	*
*		 receiver.						*
*									*
*  Return program number (prognum) for success and 0 for failure.       *
*                                                                       */
extern u_long
ipgwin_register_msg_send(
      u_long send_id,           /* user sender ID */
      u_long prognum,           /* program ID */
      u_long versnum,           /* program version */
      char *hostname);         /* hostname where the message delivers to */
 
/************************************************************************
*                                                                       *
*  Send a message.                                                      *
*  Return OK or NOT_OK.                                                 *
*                                                                       */
extern int
ipgwin_send(u_long send_id, 	/* sender ID */
	    Ipgmsg *msg);	/* message string (actual message)*/

#endif _IPGWIN_H
