/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <stdio.h>
#include <stdlib.h>
#include  <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>
#include "termhandler.h"


static char cr = (char) 13;
static char lf = (char) 10;
static char ack = (char) 6;
static char stx = (char) 2;
static char etx = (char) 3;
static char period = (char) 46;
static char Mesg[256];
static char Respon[512];
static char Resp[512];
static char gErrorMsge[160];
static char *ErrorMessage;

static int              timer_went_off;
static struct itimerval orig_itimer;
static struct sigaction orig_sigalrm;
static sigset_t         orig_sigmask;

/* X Y motor accel/deccel ramp in Hz/sec, 10000 Hz/s standard */
/* X motor speed in Hz, 3500 slow, 12000 standard, 18000 fast */
static int Xfreq = 3500;
/* Y motor speed in Hz, 3500 slow, 12000 standard, 15000 fast */
/* at present Y is always at 15000 Hz */

static double ISOCenter[2] = { 0.0, 0.0 };
static double xyMinMax[4],MaxXYSpeed[2];
static double SoftLimitXY[4] = { 0.0, 0.0, 0.0, 0.0 };
static double Landmark[2] = { 0.0, 0.0 };

static float XLaserRef2ISO; /* R10 */
static float YLaserRef2ISO; /* R11 */
static float XShrouldLimit;
static float XHlimitIn;
static float XHlimitOut;
static float Y_XLL1;
static float Y_XLL2;
static float Y_XLH1;
static float Y_XLH2;

/* portdev = /dev/term/b or /dev/term/a */
static int              Portfd;

#define PATIENT_TABLE 9999

/* Status bits */
#define PROG_RUN 0
#define EXTERN_EMG_STOP (1 << 1)
#define EMG_AXIS_STOP 	(1 << 2)
#define POWER_ERROR   	(1 << 3)
#define ProgramERROR  	(1 << 4)
#define TERM_MODE	(1 << 5)
#define SRQ		(1 << 6)
#define READY2RECV	(1 << 7)

#define X_AXIS	0
#define Y_AXIS	1

char *StatusVals[] = { "Program Running ", 
		"External Emergency Stop",
		"Emergency Stop of One of the Axis",
		"Power Stage Error",
		"Programming Error",
		"Terminal Mode",
		"SRQ",
		"Ready to Receive" };

int wait4Axis2Stop(char axis);
static int setup_ms_timer(int ms_interval );
static void cleanup_from_timeout();

static int verbose;
static char *statusCmd = "ST";

main (int argc, char *argv[])
{
  int done;
  int status;
  char buffer[256];
  char responce[256];
  char *bptr;
  char *cmdline;
  int buflen = 1;
  int firstentry = 1;
  int chars;
  char cmdchar;
 
  if (argc < 2)
  {
    fprintf(stdout,"usage:  %s <devicename> (i.e. /dev/term/b)\n", argv[0]);
    exit(1);
  }

  verbose = 0;

  /* initialize Serial port to Gilson */
  Portfd = initPort(argv[1],PATIENT_TABLE);
 
  /* need this info for isocenter calcualtions */
  SoftLimits();

  if (argc > 2)
  {
     cmdchar = argv[2][0];
     /* printf("2nd arg: '%s'\n",argv[2]); */
     if (cmdchar != '-')
     {
	status = 0;
        switch( toupper(cmdchar) )
        {
	   case 'M':
        	MoveAxis(&(argv[2][1]));
        	/* get status and clear up error bits */
        	Send_Cmd(statusCmd, responce);
                XYPos(X_AXIS);
                XYPos(Y_AXIS);
		break;
	  case 'X':
                XYPos(X_AXIS);
		break;

	  case 'Y':
                XYPos(Y_AXIS);
		break;

	  case 'S':
                Status();
		break;

	  default:
		status = -1;
		break;
		
        }
  	fflush(stdout);
	return(status);
     }
     else if (strcmp(argv[2],"-debug") == 0)
     {
       verbose = 1;
     }
  }
 
  if (verbose)
    fprintf(stdout,"Init Device: %s\n",argv[1]);
 
  if (verbose)
     PrintSet();

/*
  fprintf(stdout,"%lf %lf %lf %lf %lf %lf %lf %lf \n",xyMinMax[0],xyMinMax[1],
		xyMinMax[2],xyMinMax[3],xyMinMax[4],xyMinMax[5],
		MaxXYSpeed[0],MaxXYSpeed[1]);
*/

  fflush(stdout);


  done = 1;
  while (done)
  {
    if (verbose)
    {
      fprintf(stdout,"M)ove Axis (MX+100,MY-100), S)tatus \n");
      fprintf(stdout,"\nCmds:  ");
    }
    else if (buflen)
    {
      if (!firstentry)
        fprintf(stdout,"Cmds:\n");   /* The GUI expects this prompt, verbatim */
      else
	 firstentry = 0;
    }
/**********/

    fflush(stdout);
    bptr = fgets(buffer, sizeof(buffer), stdin);
    buflen = strlen(buffer);
    if (bptr == NULL)
      break;
    switch( toupper(buffer[0]) )
    {
        case 'M':
                MoveAxis(&buffer[1]);
                break;
 
        case 'S':
                Status();
                break;
        case 'X':
                XYPos(X_AXIS);
                break;
        case 'Y':
                XYPos(Y_AXIS);
                break;
   
        case 'Q':
                done = 0;
                return;
                break;
    }
  }

}
  MoveAxis(char *Cmd)
  {
     /* xMA x+100 xMD */
     char mcmd[40],wcmd[32],dcmd[32];
     char responce[255];
     char *cptr;
     char axis;
     char direction;
     int nchars;

     axis = toupper(*Cmd++);
     direction = *Cmd++;
     if ( (axis != 'X') && (axis != 'Y') )
     {
       fprintf(stdout,"Error invalid Axis '%c' specified, only X or Y valid\n",axis);
       return -1;
     }

     sprintf(wcmd,"%cMA",axis);
     sprintf(mcmd,"%cMA %c%c%s",axis,axis,direction,Cmd);
     sprintf(wcmd,"%c=H",axis);
     sprintf(dcmd,"%cMD",axis);

     /* 1st activate motor, then start the axis on it's way */
     nchars = Send_Cmd(mcmd, responce);

     /* 2nd wait until axis motor has completed */
     nchars = Send_Cmd(wcmd, responce);  /* has the axis motor stopped, N=no, E=yes */
     while(responce[0] != 'E')
     {
       delayMsec(250);
       nchars = Send_Cmd(wcmd, responce);
     }

     /* just a precaution delay before turn off motor */
     delayMsec(250);
     /* 3rd turn off motor */
     nchars = Send_Cmd(dcmd, responce);
     return ( 0 );
 }
     
PrintSet()
{
   fprintf(stdout,"\n");
   fprintf(stdout,"ISO Center:  X = %lf, Y = %lf \n",
	ISOCenter[0], ISOCenter[1]);
   fprintf(stdout,"SoftLimits X: %lf, %lf\n", SoftLimitXY[0], SoftLimitXY[1]);
   fprintf(stdout,"SoftLimits Y: %lf, %lf\n", SoftLimitXY[2], SoftLimitXY[3]);
   fprintf(stdout,"Present Landmark  X = %lf,  y = %lf\n",Landmark[0],Landmark[1]);
   fprintf(stdout,"\n");
   fflush(stdout);
}

XYPos(int axisIndex)
{
   char responce[255];
   char *cmd,*cmdlandmark;
   int nchars;
   float xpos, landmarkpos;
   float dist2ISO, LaserRef2ISO;

   if (axisIndex == X_AXIS)
   {
      cmd = "XP21R";
      cmdlandmark = "R2R";
      LaserRef2ISO = XLaserRef2ISO;
   }
   else if (axisIndex == Y_AXIS)
   {
      cmd = "YP21R";
      cmdlandmark = "R5R";
      LaserRef2ISO = YLaserRef2ISO;
   }
   else
   {
      cmd = "XP21R";    /* default X axis */
      cmdlandmark = "R2R";
      LaserRef2ISO = XLaserRef2ISO;
   }

   nchars = Send_Cmd(cmd, responce);
   xpos = atof(responce);

   nchars = Send_Cmd(cmdlandmark, responce);
   landmarkpos = atof(responce);

   dist2ISO = landmarkpos - xpos + LaserRef2ISO;

   fprintf(stdout,"%f\n",xpos);
   fprintf(stdout,"%f\n",dist2ISO);
   /* fprintf(stderr,"dist 2 ISO: %f\n",dist2ISO); */
   fflush(stdout);

   /* get status and clear up error bits */
   Send_Cmd(statusCmd, responce);

   return(0);
}

Status()
{
   char *cmd = "ST";
   char responce[255];
   int status,bit,nchars;
   float ypos, xpos, landmarkXpos, landmarkYpos, Xdist2ISO, Ydist2ISO, laser2ISO, laserYref;

   nchars = Send_Cmd(cmd, responce);
   status = atoi(responce);
   fprintf(stdout,"Status: 0x%x\n",status);
   for (bit = 0; bit < 8; bit++)
   {
      if (status & (1 << bit)) fprintf(stdout,"\t'%s'\n",StatusVals[bit]);
   }
   fprintf(stdout,"\n");
   nchars = Send_Cmd("XP21R", responce);
   xpos = atof(responce);
   nchars = Send_Cmd("YP21R", responce);
   ypos = atof(responce);
   fprintf(stdout,"X-Position (vertical)   : %f mm, %f in\n",xpos,xpos/25.4);
   fprintf(stdout,"Y-Position (horizontal) : %f mm, %f in\n",ypos,ypos/25.4);

   /*
      Oops, R15 & R16 are only updated if the key pad is used, not much use here.
   nchars = Send_Cmd("R15R", responce);
   xpos = atof(responce);
   nchars = Send_Cmd("R16R", responce);
   ypos = atof(responce);
   */
   /* calc taken from CALDST.APR, used in PT controller to calc dist to ISO center */
   nchars = Send_Cmd("R10R", responce);
   laser2ISO = atof(responce);
   nchars = Send_Cmd("R11R", responce);
   laserYref = atof(responce);

   nchars = Send_Cmd("R2R", responce);
   landmarkXpos = atof(responce);
   nchars = Send_Cmd("R5R", responce);
   landmarkYpos = atof(responce);

   Xdist2ISO = landmarkXpos - xpos + laser2ISO;
   Ydist2ISO = landmarkYpos - ypos + laserYref;

   fprintf(stdout,"laser2ISO: %f, LaserYref: %f, LandmarkX: %f, LandmarkY: %f\n",
		laser2ISO,laserYref,landmarkXpos,landmarkYpos);
   
   fprintf(stdout,"X-Position relative to ISO (vertical)   : %f mm, %f in\n",
		Xdist2ISO,Xdist2ISO/25.4);
   fprintf(stdout,"Y-Position relative to ISO (horizontal) : %f mm, %f in\n",
		Ydist2ISO,Ydist2ISO/25.4);
   fprintf(stdout,"\n\n");
   
   return(0);
}

SoftLimits()
{
   char *cmd;
   char responce[255];
   int nchars;

   /* get status and clear up error bits */
   Send_Cmd(statusCmd, responce);

   /* calc taken from CALDST.APR, used in PT controller to calc dist to ISO center */
   nchars = Send_Cmd("R10R", responce);
   XLaserRef2ISO = atof(responce);
   nchars = Send_Cmd("R11R", responce);
   YLaserRef2ISO = atof(responce);

    nchars = Send_Cmd("R20R", responce);
    XShrouldLimit = atof(responce);
    nchars = Send_Cmd("R21R", responce);
    XHlimitIn = atof(responce);
    nchars = Send_Cmd("R22R", responce);
    XHlimitOut = atof(responce);
    nchars = Send_Cmd("R23R", responce);
    Y_XLL1 = atof(responce);
    nchars = Send_Cmd("R24R", responce);
    Y_XLH1 = atof(responce);
    nchars = Send_Cmd("R25R", responce);
    Y_XLL2 = atof(responce);
    nchars = Send_Cmd("R26R", responce);
    Y_XLH2 = atof(responce);
   
  if (verbose)
  {
    printf("XShrouldLimit: %lf, XHlimitIn: %lf, XHlimitOut: %lf\n",XShrouldLimit,XHlimitIn,XHlimitOut);
    printf("Y_XLL1: %lf, Y_XLH1: %lf, Y_XLL2: %lf, Y_XLH2: %lf\n",Y_XLL1,Y_XLH1,Y_XLL2,Y_XLH2);
  }
}

/* 
   All commands to the Patient table control follow the following pattern 
  STX "Instruction" ETX CR LF , STX=2,ETX=3 
  "instruction may only be 32 char long 
 responces follow the following from 
 STX ACK "ascii value" ETX CR LF , ascii value may not be present 
 Note: Ack is sent when the unit starts to execute the command NOT
       when the moter comes to a stop !!!
*/
Send_Cmd(char *cmd, char *responce)
{
   char msg[255];
   char rchar;
   char *rptr;
   int wbyte,rbyte;
   int cnt,slen,ackrecv;

   sprintf(msg,"%c%s%c%c%c",stx,cmd,etx,cr,lf);

   rptr = responce;
   *rptr = '\000';
   rbyte = cnt = 0;
   ackrecv = 0;

   slen = strlen(msg);

   tcflush(Portfd,TCIOFLUSH);    /* clear serial port */

   wbyte = write(Portfd, msg, slen);
   tcdrain(Portfd);    /* let transmission complete */

   rchar = '\000';
   while (rchar != etx )
   {
      rbyte=read(Portfd, &rchar, 1);  /* This is a BLOCKING read  */
      if (rbyte > 0)
      {
         /* fprintf(stderr,"0x%2x\n",(rchar & 0x7F)); */

        if ( (rchar == stx) || (rchar == etx) )
	  continue;

        if (rchar == ack)
        {
	   /* fprintf(stderr,"recv ACK\n"); */
           ackrecv = 1;
           continue;
        }

        if (ackrecv == 1)
        {
	   /*  fprintf(stderr,"responce char: %c\n",rchar); */
           *rptr++ = rchar;
           cnt++;
        }
     }
   }
  *rptr = '\000';       /* null terminate string */
  return(cnt);  /* number of chars in responce */
}


delayMsec(int time)
{
    timer_went_off = 0;
    setup_ms_timer(time);  /* in msec */
    while (!timer_went_off)
       sigpause(0);
    cleanup_from_timeout();
}


wait4Axis2Stop(char axis)
{
   char cmd[25];
   char responce[40];
   int nchars;

   sprintf(cmd,"%c=H\n",axis);

   /* loop until the resonce is 'E', which means the axis motor
      has stopped, while running the responce is 'N'
   */
   nchars = Send_Cmd(cmd, responce);
   fprintf(stderr,"result: '%s'\n",responce);
   while(strcmp(responce,"E") != 0)
   {
      delayMsec(250);
      nchars = Send_Cmd(cmd, responce);
      fprintf(stderr,"result: '%s'\n",responce);
   }
}



/*********************************************************
* sigalrm_irpt
*       SIGALRM interrupt handler (used for timeout)
*
*/
static void
sigalrm_irpt()
{
        sigset_t                qmask;
        struct sigaction        sigalrm_action;
 
/*  Reregister sigalrm_irpt as the SIGALRM interrupt
    handler
    to prevent process termination due to an Alarm Clock.    */
     
        sigemptyset( &qmask );
        sigaddset( &qmask, SIGALRM );
        sigaddset( &qmask, SIGIO );
        sigaddset( &qmask, SIGCHLD );
        sigalrm_action.sa_handler = sigalrm_irpt;
        sigalrm_action.sa_mask = qmask;
        sigalrm_action.sa_flags = 0;
        sigaction( SIGALRM, &sigalrm_action, NULL );

        timer_went_off = 1;                          
}

/*************************************************************
*
*
*
*/
static int setup_ms_timer(int ms_interval )
{    
        sigset_t                qmask;
        struct sigaction        sigalrm_action;
        struct itimerval        new_itimer;
     
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
        sigalrm_action.sa_flags = 0;
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
 
/************************************************************
*
*
*
*/
static void cleanup_from_timeout()
{
        sigprocmask( SIG_SETMASK, &orig_sigmask, NULL );
        sigaction( SIGALRM, &orig_sigalrm, NULL );
        setitimer( ITIMER_REAL, &orig_itimer, NULL );
}
 
