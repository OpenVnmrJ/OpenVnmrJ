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
/* usrTime.c  - Unix style date & time interface Source Library */
/*
	vxWorks User's Group (Warren Jasper, wjasper@tx.ncsu.edu)
*/
#include <vxWorks.h>
#include <time.h>
#include <timers.h>
#include <socket.h>
#include <sockLib.h>
#include <in.h>
#include <stdio.h>
#include <string.h>
#include <hostLib.h>
#include <ioLib.h>
#define IPPORT_TIMESERVER 37

/*
modification history
--------------------
5-26-94,gmb  created 
7-01-2004,gmb  created for Nirvana controllers, must pass time server to rdate
*/


/*
DESCRIPTION

Provides a Library of Date & Time of Day function for VxWorks.
Routines to Obtain the date from the boot host computer, set the date
and print the date are provided in UNIX style.

E.G.
   1. putenv("TIMEZONE=PDT::420:040102:100102")   <--- TimeZone
   2. _SERVER = &sysBootParams + 20  <- Who the boot host is
   3. ld < usrTime.o
   4. rdate <-- get date from boot host and set VxWorks Clock
   5. date  <-- output i.e.  FRI JUL 01 11:59:34 1994

*/

extern char sysBootParams[];
static char *SERVER=&sysBootParams[20];

/**************************************************************
*
*  date - prints date and time (similar to UNIX date)
*
*
*  This routine prints the date and time. 
*
*
* RETURNS:
* void
*
*/
void date ()
    {
    time_t tp;
    tp = time (0);
    printf (ctime (&tp));
    }

/**************************************************************
*
*  set_date - sets the vxWorks date & time clock
*
*
*  This routine sets the VxWorks date & time clock. 
*
*
* RETURNS:
* void
*
*/
void set_date (long unsigned int t)
    {

    /* Assume the time is in GMT */

    struct timespec tp;

    tp.tv_sec = t;
    tp.tv_nsec = 0;

    clock_settime (CLOCK_REALTIME, &tp);
    printf ("The new time is set to: ");
    date ();
    }

/**************************************************************
*
*  rdate - get the unix time of day from our boot host
*
*
*  This routine get the unix time of day from our boot host and
* then sets the VxWorks date & time clock. 
*
*
* RETURNS:
* void
*
*/
void nvrdate (char *server)
{

    /* rdate - get the unix time of day from our boot host.  */

    struct sockaddr_in sockaddr;
    struct timespec tp;
    int s;
    int ipval;
    unsigned long t;
    char *buffer;
    int nbytes,
        nread;

    bzero ((char *) (&sockaddr), sizeof (sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons (IPPORT_TIMESERVER);

    sockaddr.sin_addr.s_addr = ipval; /* hostGetByName (SERVER); */
    if (server != NULL)
    {
       if( (sockaddr.sin_addr.s_addr = hostGetByName(server)) == ERROR)
       {
           printf("Unknown Time Server: '%s'\n",server);
	   return;
       }
    }
    else
    {
       if( (sockaddr.sin_addr.s_addr = hostGetByName("wormhole")) == ERROR)
       {
           printf("Unknown Time Server: 'wormhole'\n");
	   return;
       }
    }

    if ((s = socket (AF_INET, SOCK_STREAM, 0)) == ERROR)
	{
	perror ("rdate: socket failed.");
	return;
	}

    if (connect (s, &sockaddr, sizeof(sockaddr)) == ERROR)
	{
	close (s);
	perror ("rdate: connect failed.");
	return;
	}
    buffer = (char *) &t;
    nbytes = sizeof (t);
    while (nbytes)
	{
	if ((nread = read (s, buffer, nbytes)) == ERROR)
	    {
	    close (s);
	    perror ("rdate: read failed.");
	    return;
	    }
	nbytes -= nread;
	buffer += nread;
	}
    close (s);

    /*
     * There is a 2208988800 sec (about 70 years) difference between 0 in UNIX
     * time, and what the inetd time service gives.  Hence the use of this
     * magic number
     */

    tp.tv_sec = (ntohl (t) - 2208988800L);
    tp.tv_nsec = 0;
    clock_settime (CLOCK_REALTIME, &tp);
}
