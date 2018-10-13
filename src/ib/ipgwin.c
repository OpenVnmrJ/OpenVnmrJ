/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *Sid(){
    return "@(#)ipgwin.c 18.1 03/21/08 (c)1991-92 SISCO";
}

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
*  This file contains routines for process communication using RPC, and	*
*  routine for calling user function in a certain interval.		*
*									*
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <sys/types.h>
typedef u_long ulong ;
#ifdef LINUX
#define PORTMAP
#include <rpc/rpc.h>
#include <rpc/svc.h>
#include <rpc/svc_auth.h>
#elif SOLARIS
#define PORTMAP
#include <rpc/rpc.h>
#include <rpc/svc_soc.h>
#else
#include "rpc.h"
typedef bool_t (*resultproc_t)(char *, struct in_addr *);
#endif
#include <rpc/pmap_clnt.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef SOLARIS
#include <time.h>
#else /* (not) SOLARIS */
#include <sys/time.h>
#endif /* (not) SOLARIS */
#include <sys/resource.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#ifdef LINUX 
#elif SOLARIS
#include <netconfig.h>
#endif
#include "stderr.h"
#include "ipgwin.h"

#define	IPG_MAX_BYTES	BUFSIZ

/* Loop delay to avoid chewing up all CPU time (microseconds) */
#define	LOOP_DELAY	40

/* Structure for RPC (TCP) SERVER */
typedef struct _svcmsg
{
   SVCXPRT *xprt;	/* server information */
   u_long prognum;	/* program number (range 0x20000000 - 0x5fffffff) */
   u_long versnum;	/* version number (any number) */
   void (*msgfunc)(u_long procnum, Ipgmsg *msg);/* user function to call */
   struct _svcmsg *next;
} Svcmsg;

/* Structure for RPC CLIENT */
typedef struct _cltmsg
{
   u_long client_id;	/* user sender ID */
   CLIENT *client;	/* client information */
   char *hostname;	/* hostname to send to */
   u_long prognum;	/* program number (should be the same as destination
			   host) */
   u_long versnum;	/* version number (should be the same as destination
			   host) */
   struct _cltmsg *next;
} Cltmsg;

/* Structure for IPG user-function */
typedef struct _useritem
{
   int userid;		/* user ID */
   long delay_sec;	/* delay in second */
   long delay_usec;	/* delay in micro second */
   struct timeval tp;	/* current time */
   void (*userfunc)(int);/* user function to call */
   struct _useritem *next;
} Useritem;

static Svcmsg *svchead=NULL;	/* header of the RPC SERVER list */
static Cltmsg *clthead=NULL;	/* header of the RPC CLIENT list */
static Useritem *ufhead=NULL;	/* header of IPG user-function list */
static int not_quit;	/* Flag to indicate TRUE of FALSE for looping */
static int uid;		/* User ID of the process */

static void ipgwin_check_message(int);
static void ipgwin_priv_msg_receive(struct svc_req *, SVCXPRT *);
static void ipgwin_notify_user(void);
static bool_t xdr_ipgmsg(XDR *xdrs, Ipgmsg *msg);

/************************************************************************
*									*
*  This function provides a platform independent way to get the
*  time of day.
*									*/
void
ipg_gettime(struct timeval *tp)
{
#ifdef LINUX
    gettimeofday(tp, NULL);
#elif SOLARIS
    struct timespec temp;
    clock_gettime(CLOCK_REALTIME, &temp);
    tp->tv_sec = temp.tv_sec;
    tp->tv_usec = temp.tv_nsec / 1000;
#else /* (not) SOLARIS */
    gettimeofday(tp, NULL);
#endif /* (not) SOLARIS */
}

/************************************************************************
*									*
*  This function is provided to support all events which cannot be 	*
*  detected by xv_main_loop.  This routine will go through all register *
*  event functions for each loop as well as Xview events.		*
*									*/
void
ipgwin_main_loop(Frame frame)
{
   Display *display;		/* display handler */
   int num_filed;		/* number of file descriptor */
   struct rlimit rlp;

   /* Get program UID */
   uid = getuid();

   /* Get the number of file descriptors */
   //num_filed = getdtablesize();
   if (getrlimit(RLIMIT_NOFILE,&rlp) == 0){
      num_filed = rlp.rlim_cur;
   }else{
      num_filed = 0;
   }

   display = (Display *)xv_get(frame, XV_DISPLAY);
   xv_set(frame, WIN_SHOW, TRUE, 0);
   XFlush(display);

   not_quit = TRUE;
   while (not_quit)
   {
      /* Process XView event */
      notify_dispatch();

      /* Check if there is a message coming in. Process it if there is */ 
      ipgwin_check_message(num_filed);

      /* Check if any user-function is registered. Process it if there is */
      ipgwin_notify_user();

      XFlush(display);

#ifdef SOLARIS
      struct timespec delay;
      delay.tv_sec = 0;
      delay.tv_nsec = LOOP_DELAY * 1000;
      nanosleep(&delay, NULL);
#else /* (not) SOLARIS */
      usleep(LOOP_DELAY);
#endif /* (not) SOLARIS */
   }  
}

/************************************************************************
*									*
* Terminate sis_main_loop.						*
*									*/
void
ipgwin_exit_loop()
{
   not_quit = FALSE;
}

/************************************************************************
*									*
*  Check if there is incoming message.  This routine is similar to	*
*  svc_run, but we don't want to block (wait).				*
*									*
*  In RPC term, SERVER polls a message.					*
*									*/
static void
ipgwin_check_message(int num_filed)	/* number of file decriptor */
{
#ifdef LINUX
   fd_set readfds;	   /* bit mask for file descriptor */
#else
   struct fd_set readfds;	   /* bit mask for file descriptor */
#endif
   struct timeval timeout; /* Time to wait for message */

   /* No message server is registered */
   if (svchead == NULL)
      return;

   /* IMPORTANT: fd_set is struct { u_long fds_bits[8] }     */
   /* svc_fds is a global external variable from RPC routine */
   /* Its value should be copied to another variable to      */
   /* prevent "select" function changing its value.          */ 
   readfds = svc_fdset; 

   /* No waiting because we want to do polling */
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   switch (select(num_filed, &readfds, NULL, NULL, &timeout))
   {
      case -1:
	 if (errno == EBADF)
	 {
	    PERROR("ipgwin_check_message:select");
	 }
	 break;

      case 0:		/* No message */
	  break;

      default:		/* There is a message */
	 svc_getreqset(&readfds);
	 break;
   }
}

/************************************************************************
*									*
*  Unregister all receiver message ports.				*
*									*/
void
ipgwin_unregister_msg(void)
{
   Svcmsg *temp;

   while (svchead)
   {
      svc_unregister(svchead->prognum, svchead->versnum);
      svc_destroy(svchead->xprt);
      temp = svchead->next;
      (void)free((char *)svchead);
      svchead = temp;
   }
}

/************************************************************************
*									*
*  This routine is used to register a specific function which will be	*
*  called when there is a message.					*
*  User function to be called can be NULL, indicating unregistering     *
*  message, or user-func address, which takes a form of                 *
*  func(u_long proc_id,  where proc_id is ID number.                    *
*       Ipgmsg *msg)       where msg is a message.                        *
*									*
*  Return program number for success and 0 for failure.			*
*  									*
*  In RPC term, SERVER registers/unregisters program and version number	*
*									*/
u_long
ipgwin_register_msg_receive(
      u_long prognum,		/* program ID */
      u_long versnum,		/* program version */
      void (*userfunc)(u_long, Ipgmsg*))
{
   Svcmsg *current, *prev;	/* ptrs used for message item */
   Svcmsg *newitem;		/* new created item */
   SVCXPRT *transp;		/* transport info */
   int protocol;		/* protocol */

   /* Check if the msgid has been registered or not.  If the message  */
   /* has been registered, only change the user function.  If the user */
   /* function is NULL, delete the message item from the list.  If the */
   /* message has not been registered, add the new message item at the */
   /* first of the list.					       */
   for (current=prev=svchead; current; prev=current, current=current->next)
   {
      if ((current->prognum == prognum) &&
          (current->versnum == versnum))   /* Found */
      {
         svc_unregister(current->prognum, current->versnum);
	 svc_destroy(current->xprt);

         if (current == prev)  /* Only one item on the list */
	    svchead = NULL;
         else
            prev->next = current->next;
         (void)free((char *)current);

	 if (userfunc == NULL) 
	    return(prognum) ;
	 else
	    break;
      }
   }

   /* Create an RPC with TCP socket */
   if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL)
   {
      STDERR("Can't create RPC");
      return(0);
   }

   /* Get the "transient" program number if prognum is zero */
   if (prognum == 0)
   {
      /* The "transient" program number starts at 0x40000000 */
      /* Get the unused number.				     */
      prognum = 0x40000000;
      while (!pmap_set(prognum, versnum, IPPROTO_TCP, transp->xp_port))
	 prognum++;

      /* We don't need to register it at svc_register since pmap_set */
      /* has done it.						     */
      protocol = 0;
   }
   else
   {
      /* Erase the pormapper's table */
      pmap_unset(prognum, versnum);

      protocol = IPPROTO_TCP;
   }

   /* Register the portmapper. */
   /* Note that Sun suggests that the program number ranges from */
   /* 0x20000000 to 0x5fffffff.  However, our routine doesn't    */
   /* check for this limit.		        */
#ifdef SOLARIS
   if (!svc_register(transp, prognum, versnum,
		     ipgwin_priv_msg_receive, protocol))
#elif LINUX 
   if (!svc_register(transp, prognum, versnum,
		     ipgwin_priv_msg_receive, protocol))
#else
   if (!svc_register(transp, prognum, versnum, 
       (void (*)(DOTDOTDOT))ipgwin_priv_msg_receive, protocol))
#endif
   {
      STDERR("ipgwin_register_msg_receive:Can't register RPC");
      svc_destroy(transp);
      return(0);
   }

   /* Create new item */
   if ((newitem = (Svcmsg *)malloc(sizeof(Svcmsg))) == NULL)
   {
      PERROR("ipgwin_register_message_receive:malloc:Message is not registered");
      return(0);
   }
   newitem->xprt = transp;
   newitem->prognum = prognum;
   newitem->versnum = versnum;
   newitem->msgfunc = userfunc;

   /* Add a new item at the front of the list */
   newitem->next = svchead;
   svchead = newitem; 
   return(prognum);
}

/************************************************************************
*									*
*  This routine is used to register a message for sending. After	*
*  registering,  the user will refer to "send_id" when sending a 	*
*  message.								*
*  The message can be sent to any host by specifying the hostname.  If	*
*  hostname is NULL,  this routine will unregister the message.		*
*									*
*  Return program number for success and 0 for failure.			*
*									*
*  In RPC term, CLIENT registers program and version numbers.  The	*
*  server should have already registered these numbers.			*
*									*/
u_long
ipgwin_register_msg_send(
      u_long send_id,		/* user sender ID */
      u_long prognum,		/* program ID */
      u_long versnum,		/* program version */
      char *hostname)		/* hostname where the message delivers to */ 
{
   struct hostent *hp;		/* database information for hostname */
   struct sockaddr_in svc_addr; /* server spcket address */
   CLIENT *client;		/* Client info */
   Cltmsg *current, *prev;	/* ptrs used for message item */
   Cltmsg *newitem;		/* new item */
   int sock = RPC_ANYSOCK;	/* any socket */

   /* Check if the sender ID has been registered. */
   for (current=prev=clthead; current; prev=current, current=current->next)
   {
      if (current->client_id == send_id)   /* Found */
      {
	 auth_destroy(current->client->cl_auth);
	 clnt_destroy(current->client);
	 (void)free(current->hostname);

         if (current == prev)  /* Only one item on the list */
	    svchead = NULL;
         else
            prev->next = current->next;
         (void)free((char *)current);

	 if (hostname == NULL)
	    return(prognum);
	 else
	    break;
      }
   }

   /* Get hostname info */
   if ((hp = gethostbyname(hostname)) == NULL)
   {
      STDERR_1("ipgwin_register_msg_send:Can't get address for %s", hostname);
      return(0);
   }

   /* Set socket address */
   memcpy( (caddr_t)&svc_addr.sin_addr, hp->h_addr_list[0], hp->h_length);
   svc_addr.sin_family = AF_INET;
   svc_addr.sin_port = 0;

   /* Create RPC client using TCP */
   if ((client = clnttcp_create(&svc_addr, prognum, versnum,
       &sock, IPG_MAX_BYTES, IPG_MAX_BYTES)) == NULL)
   {
      STDERR("ipgwin_register_msg_send:clnttcp_create");
      clnt_pcreateerror("clnttcp_create");
      return(0);
   }

   /* Create Authentication.  Server will check the UID everytime */
   /* it receives a message.					  */
   client->cl_auth = authunix_create_default();

   /* Create new item */
   if ((newitem = (Cltmsg *)malloc(sizeof(Cltmsg))) == NULL)
   {
      PERROR("ipgwin_register_msg_send:malloc:Message is not registered");
      return(0);
   }
   newitem->client = client;
   newitem->client_id = send_id;
   newitem->prognum = prognum;
   newitem->versnum = versnum;
   newitem->hostname = strdup(hostname);
   newitem->next = NULL;

   /* Add a new item at the front of the list */
   newitem->next = clthead;
   clthead = newitem; 
   return(prognum);
}

/************************************************************************
*									*
*  This routine will notify the appropriate user function when the	*
*  message has arrived.  This routine is shielded from the user.	*
*  									*
*  In RPC term, SERVER has received a message. Process it.		*
*									*/
static void
ipgwin_priv_msg_receive(
	struct svc_req *rqstp,	/* Request info */
	SVCXPRT *transp)	/* transport info */
{
   Svcmsg *current;		/* traversal pointer */
   char buf[IPG_MAX_BYTES];	/* message string buffer */
   Ipgmsg msg;			/* IPG message */

   /* Return if no request */
   if (rqstp->rq_proc == NULLPROC)
      return;

   /* Check for the same UID */
   if (((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid != uid)
   {
      STDERR("Sorry ! CLIENT doesn't have the same UID as SERVER");
      return;
   }

   /* Check for all user register transport and find the correct */
   /* program number and version number.			 */
   for (current=svchead; current; current=current->next)
   {
      if ((current->prognum == rqstp->rq_prog) &&
          (current->versnum == rqstp->rq_vers))
      {
	 msg.msgbuf = buf;
#ifdef SOLARIS
	 if (!svc_getargs(transp, (xdrproc_t)xdr_ipgmsg, (caddr_t)(&msg)) )
#elif LINUX 
	 if (!svc_getargs(transp, (xdrproc_t)xdr_ipgmsg, (caddr_t)(&msg)) )
#else
	 if (!svc_getargs(transp, xdr_ipgmsg, &msg))
#endif
	 {
	    STDERR("ipgwin_priv_msg_receive:can't decode arguments");
	    return;
	 }

	 /* Make sure that the request procedure number is the same as */
	 /* major ID.						       */
	 if (rqstp->rq_proc != msg.major_id)
	 {
	    STDERR("ipgwin_priv_msg_receive:Undefined major ID:");
	    STDERR_2("  major ID should be %d, but it receives %d",
		  msg.major_id, rqstp->rq_proc);
	    return;
	 }

	 /* Call the user function */
	 current->msgfunc(rqstp->rq_proc, &msg);
	 return;
      }
   }

   /* ERROR if the code passes here */
   STDERR("ipgwin_priv_msg_receive:No receiver for this message");
}

/************************************************************************
*									*
*  This routine is used to send a message to the server.  In order to	*
*  send a message, the user should have registered sender ID.		*
*									*
*  In RPC term, CLIENT sends a message to server.			*
*									*
*   Return OK or NOT_OK.						*
*									*/
int
ipgwin_send(u_long send_id,		/* sender ID */
	    Ipgmsg *msg)		/* message */
{
   Cltmsg *ptr;			/* pointer to find the corrent sender ID */
   struct timeval timeout;	/* timeout to wait for reply */
   int clnt_status;		/* return status from clnt_call */

   /* No waiting for reply */
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   /* Finf the correct sender ID which have been registered */
   for (ptr=clthead; ptr; ptr=ptr->next)
   {
      if (ptr->client_id == send_id)
      {
#ifdef SOLARIS
	  if ( (clnt_status = clnt_call(ptr->client,
					msg->major_id, 
					(xdrproc_t)xdr_ipgmsg,
					(caddr_t)msg,
					(xdrproc_t)xdr_void,
					NULL, 
					timeout)
		) != RPC_SUCCESS)
#elif LINUX 
	  if ( (clnt_status = clnt_call(ptr->client,
					msg->major_id, 
					(xdrproc_t)xdr_ipgmsg,
					(caddr_t)msg,
					(xdrproc_t)xdr_void,
					NULL, 
					timeout)
		) != RPC_SUCCESS)
#else
	  if ( (clnt_status = clnt_call(ptr->client,
					msg->major_id,
					xdr_ipgmsg,
					msg,
					xdr_void,
					NULL,
					timeout)
		) != RPC_SUCCESS)
#endif
	  {
	      /* Ignore error due to timeout */
	      if (clnt_status != (int)RPC_TIMEDOUT)
	      {
		  STDERR("ipgwin_send:clnt_call");
		  clnt_perror(ptr->client, "clnt_call");
		  return(NOT_OK);
	      }
	  }
	  return(OK);
      }
  }
   STDERR_1("ipgwin_send: No such sender ID (%d) registered", send_id);
   return(NOT_OK);
}

/************************************************************************
*									*
*  Check if there is any registered user function.  			*
*  The user function will be called if the delay time exceeded.		*
*									*/
static void
ipgwin_notify_user(void)
{
   Useritem *ptr;  	/* pointer to the current user function*/
   struct timeval temp;	/* time structure */
   long exec_sec;	/* When to service--sec */
   long exec_usec;	/* When to service--usec */

   /* Check each registered user function */
   for (ptr=ufhead; ptr; ptr=ptr->next)
   {
       /* Calculate next execution time */
       exec_sec = ptr->tp.tv_sec + ptr->delay_sec;
       exec_usec = ptr->tp.tv_usec + ptr->delay_usec;
       if (exec_usec >= 1000000){
	   exec_usec -= 1000000;
	   exec_sec += 1;
       }

       /* See if it's time yet */
       ipg_gettime(&temp);
       if ( (temp.tv_sec > exec_sec) ||
           ((temp.tv_sec == exec_sec) && (temp.tv_usec >= exec_usec)) )
       {
	   /* Time to do it--update reference time */
	   ptr->tp.tv_sec = exec_sec;
	   ptr->tp.tv_usec = exec_usec;

	   /* Execute user function */
	   ptr->userfunc(ptr->userid);
       }
   }
}

/************************************************************************
*									*
*  This routine is used to register a specific function which will be   * 
*  called at regular intervals time until the user unregisters it.
*  User function to be called can be NULL, indicating unregistering     *
*  user-function, or user-func address, which takes a form of           *
*  func(int id)              						*
*									*/
void
ipgwin_register_user_func(int userid,
	long delay_sec,		/* delay second */
	long delay_usec,	/* delay micro-second */
	void (*userfunc)(int))
{
   Useritem *current, *prev;	/* item pointers */

   /* Check if the id has been registered or not.  If the id has  */
   /* been registered, only change the user function.  If the user */
   /* function is NULL, delete this item from the list.  If the    */
   /* id has not been registered, add the new item at the 	   */
   /* end of the list.						   */
   
   for (current=prev=ufhead; current; prev=current, current=current->next)
   {
      if (current->userid == userid)   /* Found */
	 break;
   }

   if (current)		/* We found this ID */
   {
      if (userfunc == NULL)	/* delete this ID */
      {
	 if (current == prev)	/* Only one item on the list */
	    ufhead = NULL;
	 else
	    prev->next = current->next;
	 (void)free((char *)current);
      }
      else
      {
	 current->userfunc = userfunc;
	 current->delay_sec = delay_sec;
	 current->delay_usec = delay_usec;
	 ipg_gettime(&(current->tp));
      }
   }
   else			/* Add the user function on the list */
   {
      Useritem *newitem;
      if ((newitem = (Useritem *)malloc(sizeof(Useritem))) == NULL)
      {
	 PERROR("ipgwin_notify_user_func:malloc:Message is not registered");
	 return;
      }
      
      newitem->userid = userid;
      newitem->userfunc = userfunc;
      newitem->delay_sec = delay_sec;
      newitem->delay_usec = delay_usec;
      ipg_gettime(&(newitem->tp));
      newitem->next = NULL;

      if (ufhead == NULL)
	 ufhead = newitem;
      else
	 prev->next = newitem;
   }
}

/************************************************************************
*									*
*  Convert the Ipgmsg into XDR format.					*
*									*/
static bool_t
xdr_ipgmsg(XDR *xdrs, Ipgmsg *msg)
{
   if (!xdr_u_long(xdrs, &msg->major_id) ||
       !xdr_u_long(xdrs, &msg->minor_id) ||
       !xdr_string(xdrs, &msg->msgbuf, IPG_MAX_BYTES))
      return(FALSE);
   return(TRUE);
}
