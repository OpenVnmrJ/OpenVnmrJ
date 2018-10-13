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
/*
 */

/* #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include  <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include  <time.h>
#include <signal.h>
#include <errno.h>

#include "errLogLib.h"
#include "termhandler.h"

#define ZONE1 1
#define ZONE2 2
#define ZONE3 3
#define MAXPATHL 256

int serialPort;

static int              timer_went_off;

static struct itimerval orig_itimer;
static struct sigaction orig_sigalrm;
static sigset_t         orig_sigmask;

#define  _POSIX_SOURCE 1
#define ETX  3
#define EOM  3
#define LF 10
#define CR 13
#define PERIOD 46
#define ASMCHANGER 0
#define SMSCHANGER 1
#define GILSON_215 3
#define NMSCHANGER 4
#define ILLEGALCMD 13
#define  FALSE  0
#define  TRUE  1
#define  WAIT_FOR_TIMEOUT    40000  /* in milisecond */
#define  MSEC_IN_SEC         1000  /* msec in a second */
#define SMPTIMEOUT	98

extern int DebugLevel;

int robot(char, int);
int startRobot( char , int );
 
main (int argc, char *argv[])
{
  char buffer[256];
  char *bptr;
  int i, val;
  int buflen = 1;
  int done = 1;
  int verbose = 1;
 
  if (argc < 2)
  {
    fprintf(stdout,"usage:  %s <devicename> (i.e. /dev/term/b)\n", argv[0]);
    exit(1);
  }
 
  if (argc > 2)
  {
     verbose = 0;
  }

  /* serialPort = initPort("/dev/term/a",SMS_SERIAL_PORT); */
  serialPort = initPort(argv[1],SMS_SERIAL_PORT);

  while (done)
  {
    if (verbose)
    {
      fprintf(stdout,"D)own Probe,   U)p Probe,        S#)ample Number\n");
      fprintf(stdout,"F)lange Adj.,  P)robe Actuator,  A)djust Probe\n");
      fprintf(stdout,"T)est NMS,     R)ack Test,       C)arousel Adj,\n");
      fprintf(stdout,"Q)uit,  \n");
      fprintf(stdout,"\nEnter a command:  ");
    }
    else if (buflen)
    {
      fprintf(stdout,"CMDS:\n");
    }
   
    fflush(stdout);
    bptr = gets(buffer);
    buflen = strlen(buffer);
    if (bptr == NULL)
       break;
    switch( toupper(buffer[0]) )
    {   
        case 'D':
	   	  robot('F',0);
                  break;
        
        case 'U':
	      	  robot('M',0);
                  break;

	case 'F':
		  robot('A',1);
                  break;

	case 'P':
		  robot('A',2);
                  break;

	case 'A':
		  robot('A',3);
                  break;

	case 'T':
		  robot('A',4);
                  break;

	case 'R':
		  robot('A',5);
                  break;

	case 'C':
		  robot('A',6);
                  break;

	case 'S':
                  val = atoi(buffer+1);
		  robot('V', val);
                  break;

	case 'ETX':
                  robot('',0);
                  break;

        case 'Q':
		  robot('Q',0);
                  close(serialPort);
	          exit(0);
	          break;

        default:
	          break;
    }
  }
}    
 
robot( char cmd, int index )
{
  int stat;
  
  if (stat = startRobot( cmd, index ))
     return (stat);        /* return if stat != 0  ERROR  */
  return( robotCmplt() );
}

/************************************************************
*  robotCmplt - waits for the sample changer to complete a command
*/
robotCmplt()
{
  int stat;
  DPRINT(1,"robotCmplt: Waiting for Cmd Ack\n");

  if (stat = cmdAck(serialPort,600))   /* 10 min to timeout */
     return(stat);           /* return if stat != 0  ERROR */

  DPRINT(1," robotCmplt: cmdAck D O N E\n\n");
  return (0);             /* At this point --> nicely DONE */
}


/************************************************************
*  startRobot - send a command to the sample changer
*  returns immediately without waiting for the sample changer to complete
*/
int startRobot( char cmd, int smpnum )
{
  char *ptr;
  char  chrbuf[20];
  char  chrbuf1[128];
  int   i, wbyte, rbyte, stat;

  tcflush(serialPort,TCIFLUSH);    /* clear serial port */

  sprintf(chrbuf,"%c", cmd);
  ptr = chrbuf;

  DPRINT2(1,"startRobot: cmd: %s, Arg: %d\n",chrbuf,smpnum); 
  DPRINT1(2,"SENDcmd: 0x%2x\n",*ptr); 

  /* Sending alphabetical part of command to robot  */
  wbyte= write(serialPort, ptr, 1);  

  stat = readPort(serialPort, 0, 6);

  if (stat == SMPTIMEOUT)  /* and read the echo back till end of text "0x03" */
     return(stat);

  /*-- these commands require a parameter --*/
  if (cmd == 'F' || cmd == 'P' || cmd == 'A' || 
      cmd == 'T' || cmd == 'R' || cmd == 'C')
  {
     DPRINT1(2,"\nSENDsmpnum: %2x Hex",smpnum); 
     writePort(serialPort,smpnum);
  }

  /* DPRINT(2,"\nSEND 'cr'");  */
  write(serialPort, "\r",1);         /*  Sending "cr" , The end of command  */
  
  return (0);
}

/************************************************************
*
*  cmdAck - receives the sample changer respons
*
*  receives the character stream out put from the 
*  sample changer . And compiles any error numbers returned .
*  This routine returns when the sample changer returns
*   it's prompt ( CRLF. ) or error message ( -error EOM )
*/
cmdAck (int serialPort, int timeout)
{
  int  error = 0;
  int  sign = 1;
  int  done = FALSE;
  
  char charbuf1[128];
  int  rbyte=0, i;
  char chr, prevchr;
  
  chr = prevchr = 0;

  DPRINT1(1,"cmdAck: expecting CR NL . or (0xd 0xa 0x2e) WITHIN %d Seconds \n",
		timeout);
  DPRINT(1,"cmdAck: error expecting ## (0=0x30) EOM (0x3) then (0xd 0xa 0x2e)\n");

  while( chr != EOM && !done )
  {
    timer_went_off = 0;
    setup_ms_timer(MSEC_IN_SEC * timeout);

    rbyte=read(serialPort, charbuf1,1);

    cleanup_from_timeout();
    if (timer_went_off)
    {
      errLogRet(LOGOPT,debugInfo,"cmdAck: Sample Changer NOT Responsive");
      return (SMPTIMEOUT);
    }

    DPRINT1(1,"cmdAck: Received: %d chars\n", rbyte);
  
    for (i=0; i<rbyte; i++) 
    {
        chr = charbuf1[i];
        DPRINT3(1,"cmdAck in array charbuf1: char %d:0x%4x\t\t ==>  %c\n",i+1,chr,chr);
    
        if (chr == '-')
           sign = -1;

        if (chr >= '0' && chr <= '9')
           error = error*10 + (chr - '0');

        if (chr == PERIOD && ( prevchr == LF || prevchr == CR))
        {
           done = TRUE;
/*ccccc*/
           /*fprintf(stdout,"%c\n", chr);*/
        }

        prevchr = chr;
    }
  }

  if (chr == EOM )
  {
    DPRINT(1," ERROR\n ");
    DPRINT1(1," last chr = %d\n",chr);
    DPRINT1(1," error = %d\n",error);
    DPRINT1(1," sign = %d\n",sign);
    DPRINT1(1,"error return # %d\n",error*sign);
    fprintf(stdout,"cmdAck_EOM: may not an ERROR: %d\n", error*sign);
    return (error*sign);
  }

  fprintf(stdout,"cmdAck: may not an ERROR: %d\n", error*sign);
  return(error*sign);  
}


readPort( int serialPort, int type, int timeout )
{
  char charbuf1[128];
  int  rbyte=0, i, echo;
  char chr=0;
 
  echo = FALSE;
  while( chr != EOM && !echo )
  {
    timer_went_off = 0;
    setup_ms_timer(MSEC_IN_SEC * timeout);
 
    rbyte=read(serialPort, charbuf1, 1);
    DPRINT1(2,"echo from NMS,charbuf1: %s\n", charbuf1);
 
    cleanup_from_timeout();
    if (timer_went_off)
    {
       /*printf("timer_went_off\n");*/
       errLogRet(LOGOPT,debugInfo,"readPort: Sample Changer NOT Responsive\n");
       return (SMPTIMEOUT);
    }
    DPRINT1(2,"RECEIVED,rbyte: %d chars\n", rbyte);
 
    if (type != 0) 
       echo = TRUE;
 
    for (i=0; i<rbyte; i++)
    {
        chr = charbuf1[i];
        DPRINT3(2,"in array charbuf1:  %d:0x%4x\t\t ==>  %c\n\n",i+1,chr,chr);
    }
  }
  return (0);
}
 
writePort( int serialPort, int smpnum )
{
  char  charbuf[20];
  char   *ptr;
  int   wbyte, stat;

  sprintf(charbuf,"%d", smpnum);
  ptr = charbuf;

  while (*ptr != '\000')
  {
     wbyte = write(serialPort, ptr, 1);
     ptr++;
     stat = readPort(serialPort, 1, 6);
     if (stat == SMPTIMEOUT) /* and read the echo back till end of text "0x03" */
     return(stat);
  }
  return (0);
}

/*********************************************************
* sigalrm_irpt
*	SIGALRM interrupt handler (used for timeout)
*/
static void
sigalrm_irpt()
{
   sigset_t           qmask;
   struct sigaction   sigalrm_action;

/*  Reregister sigalrm_irpt as the SIGALRM interrupt handler
    to prevent process termination due to an Alarm Clock.    */

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGALRM );
   sigaddset( &qmask, SIGIO );
   sigaddset( &qmask, SIGCHLD );
   sigalrm_action.sa_handler = sigalrm_irpt;
   sigalrm_action.sa_mask = qmask;

#ifdef SOLARIS
   sigalrm_action.sa_flags = ~SA_RESTART;
#else
   sigalrm_action.sa_flags = SV_INTERRUPT;
#endif

   sigaction( SIGALRM, &sigalrm_action, NULL );
   timer_went_off = 1;
}

static int
setup_ms_timer( ms_interval )
int ms_interval;
{
   sigset_t           qmask;
   struct sigaction   sigalrm_action;
   struct itimerval   new_itimer;
 
   if (ms_interval < 1) {
      return( -1 );
   }
 
/* set up signal handler */
/* necessary to assert that the system call (read)
   is NOT to be restarted.
   required for the timeout to work.                    */

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGALRM );
   sigaddset( &qmask, SIGIO );
   sigaddset( &qmask, SIGCHLD );
   sigalrm_action.sa_handler = sigalrm_irpt;
   sigalrm_action.sa_mask = qmask;

#ifdef SOLARIS
   sigalrm_action.sa_flags = ~SA_RESTART;
#else
   sigalrm_action.sa_flags = SV_INTERRUPT;
#endif
 
   if (sigaction( SIGALRM, &sigalrm_action, &orig_sigalrm ) != 0) {
      perror("sigaction error");
      return( -1 );
   }
 
/*  Specify a Timer to go off in a certain number of milliseconds and assert SIGALRM  */
 
   new_itimer.it_value.tv_sec = ms_interval / 1000;
   new_itimer.it_value.tv_usec = (ms_interval % 1000) * 1000;
   new_itimer.it_interval.tv_sec = 0;
   new_itimer.it_interval.tv_usec = 0;
 
   if (setitimer( ITIMER_REAL, &new_itimer, &orig_itimer ) != 0) {
      perror("setitimer error");
      return( -1 );
   }
 
/*  Since this process may have SIGALRM blocked (for example, it is
    running a SIGALRM interrupt program), it is necessary to remove
    SIGALRM from the mask of blocked signals.                           */
 
   sigemptyset( &qmask );
   sigaddset( &qmask, SIGALRM );
   if (sigprocmask( SIG_UNBLOCK, &qmask, &orig_sigmask ) != 0) {
      perror("sigprocmask error");
      return( -1 );
   }
 
   return( 0 );
}
 
/*  Restore all values saved (and modified) in setup_ms_timeout.  */

static int
cleanup_from_timeout()
{
   sigprocmask( SIG_SETMASK, &orig_sigmask, NULL );    
   sigaction( SIGALRM, &orig_sigalrm, NULL );
   setitimer( ITIMER_REAL, &orig_itimer, NULL );
}
 
/*  End of setup timeout / cleanup timeout programs.  */
/*  Remember!  These programs may be running in response to a SIGALRM
    or may be running on a system with a separate interval timer.   */
        

delayAwhile(int time)
{
    timer_went_off = 0;
    setup_ms_timer(time*1000);  /* in msec */
    while (!timer_went_off)
       sigpause(0);
    cleanup_from_timeout();
}

delayMsec(int time)
{
    timer_went_off = 0;
    setup_ms_timer(time);  /* in msec */
    while (!timer_went_off)
       sigpause(0);
    cleanup_from_timeout();
}
