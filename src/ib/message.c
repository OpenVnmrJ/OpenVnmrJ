/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

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
*  This file contains routines to send and receive a message to/from	*
*  other program.  It assignes a program and version numbers (act as	*
*  a server) and receive a program and version numbers from other 	*
*  process (act as a client).						*
*									*
*************************************************************************/
#include <string.h>
// #include <stream.h>
#include <iostream>
#include <stdarg.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
/* #include <sysent.h> */
#include "message.h"

// Initialize static class members
u_long Message::msgid = 0;	
u_long Message::prognum = 0;
u_long Message::versnum = 0; 
u_long Message::o_prognum = 0;
u_long Message::o_versnum = 0;
void (*Message::msg_func)(Ipgmsg *) = 0;

/************************************************************************
*									*
*  Create a message server so that it can receive a message.		*
*									*/
void
Message::create(u_long callback_func, u_long id, char *key_string) 
{
   char initname[128];	// buffer fpr initialization filename

   WARNING_OFF(Sid);

   // Assign called back function and ID
   msg_func = (void (*)(Ipgmsg *))callback_func;
   msgid = id;

   // Get the initialized file for program number and version number
   (void)init_get_win_filename(initname);

   // Get the program and version number (I am a server)
   if (init_get_val(initname, key_string, "xU", &prognum, &versnum) ==
       NOT_OK)
   {
      std::cerr << "\n\tUser needs to supply RPC Program number. Look at"
           << "\t\"Network Programming Guide\" at Introduction to"
           << "\tRPC under section Assigning Program Numbers";
      exit(1);
   }
   
   // Check for the program number limitation 
   if ((prognum < 0x20000000) || (prognum > 0x5fffffff))
   {
      fprintf(stderr, "\tWARNNING!!!\n");
      fprintf(stderr, "\tYou have assigned program number at 0x%x\n", prognum);
      fprintf(stderr, "\twhereas SUN suggests to use the number in the\n");
      fprintf(stderr, "\trange of 0x20000000 - 0x5fffffff.\n");
   }
   
   // Register to receive a message from another process
   if (!ipgwin_register_msg_receive(prognum, versnum, 
	&Message::msg_receive))
   {
      STDERR("Create:Cannot register message server");
      exit(1);
   }
}

/************************************************************************
*									*
*  Create a message client to send a message (to a hostname).  Then,	*
*  create a server and sends my program number, version number, and	*
*  hostname so that another (client) process can talk to me.		*
*									*/
void
Message::create(u_long callback_func, u_long id, char *key_string,
	       char *receiver_host)
{
   char this_host[128];         /* this hostname */
   char initname[128];          /* initialized window file name */

   // Assign called back function and ID
   msg_func = (void (*)(Ipgmsg *))callback_func;
   msgid = id;

   // Get the initialized file for program number and version number
   (void)init_get_win_filename(initname);

   // Get the program and version number (I am a client)
   if (init_get_val(initname, key_string, "xU", &o_prognum, &o_versnum) ==
       NOT_OK)
   {
      std::cerr << "\n\tUser needs to supply RPC Program number. Look at"
           << "\t\"Network Programming Guide\" at Introduction to"
           << "\tRPC under section Assigning Program Numbers";
      exit(1);
   }
   
   // Check for the program number limitation 
   if ((o_prognum < 0x20000000) || (o_prognum > 0x5fffffff))
   {
      fprintf(stderr,"\tWARNNING!!!\n");
      fprintf(stderr,"\tYou have assigned program number at 0x%x\n", prognum);
      fprintf(stderr,"\twhereas SUN suggests to use the number in the\n");
      fprintf(stderr,"\trange of 0x20000000 - 0x5fffffff.\n");
   }

   // Register to send a message to another process
   if (!ipgwin_register_msg_send(msgid, o_prognum, o_versnum, receiver_host))
   {
      std::cerr << "\tNo server is running\n";
      exit(1);
   }
   
   // Get the default hostname
   /*gethostname(this_host, 128);*/
   struct utsname names;
   uname(&names);
   strncpy(this_host, names.nodename, 128);

   // Register to receive a message from another process (as a server)
   // Note that the IPG library routine will choose program-number
   // for me.  After that, send my program number, version number, and
   // hostname to the other process so that process can talk to me.
   versnum = 1;
   if (prognum = ipgwin_register_msg_receive(0, versnum,
       &Message::msg_receive))
      msg_send(prognum, versnum, this_host);
   else
   {
      STDERR("create:ipgwin_register_msg_receive");
      exit(1);
   }
}

/************************************************************************
*									*
*  Unregistering all messages.						*
*									*/
void
Message::destroy(void)
{
   ipgwin_unregister_msg();
}

/************************************************************************
*									*
*  Send a message to another process.					*
*									*/
void
Message::msg_send(u_long major_id,	// major ID 
		u_long minor_id,	// minor ID
		char *format ...)	// any string format
{
   va_list vargs;	// variable argumment pointer
   char buf[128];	// message buffer
   Ipgmsg msg;		// IPG message

   buf[0] = 0;
   if (format)
   {
      va_start(vargs, format);
      (void)vsprintf(buf, format, vargs);
      va_end(vargs);
   }

   msg.major_id = major_id; 
   msg.minor_id = minor_id;
   msg.msgbuf = buf;

   (void)ipgwin_send(msgid, &msg);
}

/************************************************************************
*									*
*  Send a message to another process.					*
*									*/
void
Message::msg_send(Ipgmsg *msg) 
{
   (void)ipgwin_send(msgid, msg);
}

/************************************************************************
*									*
*  Receive a message from another process and call a user's callback	*
*  function.								*
*  (Note that this function also requests a connection to the server	*
*   if its major_id is greater than 0x3fffffff.)			*
*									*/
void
Message::msg_receive(u_long progid, Ipgmsg *msg)
{
   // Note that user callback function will be called if the major ID is
   // less than 0x3fffffff.
   if (msg->major_id > 0x3fffffff)
   {
      WARNING_OFF(progid);

      // Another process is ready to talk to me.  I register the 
      // program number of another process with my ID msgid so  
      // that I can send message to another process.
      if (!ipgwin_register_msg_send(msgid,	// our own ID
	   o_prognum = msg->major_id,	// program number
	   o_versnum = msg->minor_id,	// version number
		       msg->msgbuf))	// hostname
      {
	 STDERR("msg_receive:Message receiver is not running");
	 exit(1);
      }
   }
   else if (msg_func)
      msg_func(msg);
}
