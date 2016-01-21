/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _MSG_UI_H
#define _MSG_UI_H
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
#include "ipgwin.h"
#include "initstart.h"
#include "stderr.h"
#include "define_ui.h"

// Note that this class is designed to recieve 1 message and send 1
// message. 
class Message
{
   private:
      // message ID (for sending). Since this object can send only 1 
      // message, it has only 1 ID
      static u_long msgid;	

      // Server program number and version number
      static u_long prognum, versnum; 

      // Client program number and server number
      static u_long o_prognum, o_versnum;

      // User callback function when message is arrived.  If NULL, 
      // all messages are ignored.				
      static void (*msg_func)(Ipgmsg *);

      // This function will receive a message when there is a message. 
      // Then it will initialize a program for sending a message 
      // (depending on the major_id e.g. greater than 0x3fffffff) or
      // it calls a user's callback function.
      static void msg_receive(u_long, Ipgmsg *);

   public:
      // Creation. Parameter is a callback-function address, ID, and
      // string-command user to search for sprogram/version number
      // in initialization file
      static void create(u_long addr, u_long id, char *str_name);

      // Creation. Parameter is a callback-function address, ID,
      // string-command user to search for sprogram/version number
      // in initialization file, and hostname to a message send to
      static void create(u_long addr, u_long id, char *str_name,
		       char *host_name);

      // User should use these functions to send a message 
      static void msg_send(u_long maj_id, u_long min_id, char * ...);
      static void msg_send(Ipgmsg *msg);

      static void destroy(void);		// Destruction
};

#endif _MSG_UI_H
