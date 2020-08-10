/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
modification history
--------------------
6-12-06,gmb  created
7-18-94,gmb  created
2-22-05,gmb  mod to more thread safe, not 100% though
6-12-06,gmb  port to native windows, uses printf as a temporary measure for now
	     conditional compile with VNMRS_WIN32
*/


/*
Description:
 Three classes of Messages
 1. Informational Application Specific (i.e. I am doing this now, etc. )
 2. Diagnostic Messages for debugging, application specific
 3. Errors:
    A. Application Errors
    B. System Call Errors

   Class 1. maybe be just printf(s) or they maybe wish to be logged

   Class 2. Present for debugging but gone in non-debugging version,
   	    are just fprintf(s).

   Clase 3. A. Should be logged
	    B. Most Definitely should be logged.

 This Library provides facilities for Class 2 & 3 messages.

 Debugging messages use the DPRINT style macros to allow
 conditional compiling of print statment and also provides
 a level of verbosity in debug printing.

 The Error routines may log or not log based user preference.
 None logged messags are printed on stderr. Log messages
 are log and printed according the /etc/syslog.conf file
and the syslog facility parameter.
 There is a global parmater ErrLogOp that can be used in 
 errLogXXX() calls for the logopt. Then a users may change 
 this parameter (LOGIT/NOLOG) to start or stop all error logging.

 logSysInit() initialize syslog with your application ID of LOG_LOCAL0-7
and reports message with the ID string and pid of our process.

   For LOG_LOCAL0 modify /etc/syslog.conf to include
   local0.err;local0.info;local0.emerg;local0.debug        /var/log/vnmr
   local0.err;local0.info;local0.emerg;local0.debug        /dev/console

   This tells syslogd to write your messages to the console as well
as the logfile /var/log/vnmr.

  After modifing syslog.conf execute a "kill -HUP  syslog's pid" 
(found in /etc/syslog.pid)
This will result in syslogd reading the modified syslog.conf file.

   For different processes one could use LOG_LOCAL0-7 to have different
log files.

   include "errLogLib.h"

*/

#ifndef VNMRS_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>

#else   /* Win32 Native */

#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#endif   /* VNMRS_WIN32 */

#ifndef NULL
#define NULL ((void *)0)
#endif 

#define LOGIT 1
#define NOLOG 2

int DebugLevel = 0;	/* used with DPRINT */
int ErrLogOp = NOLOG;	/* used with errLogXXX  */

/*
 * static char ErrMsgStr[8192] = { 0 };
 * static char DiagMsgStr[8192] = { 0 };
*/
static char infoStr[80] = { 0 };
static char outputDevice[ 64 ] = { 0 };
static char programName[ 64 ] = { 0 };

int errPrtMsg(char *ErrMsgStr);

/**************************************************************
*
*  fileNline - returns pointer to File name & Line Number string
*
* This routine is used via the infoDebug cpp macro and is 
* not used directly by the user.
*
* RETURNS:
* char* 
*
*       Author Greg Brissey 7/18/94
*/
char *fileNline(char* filename, int linenum)
{
   sprintf(infoStr,"'%s' line: %d, ",filename,linenum);
   return(infoStr);
}


/*  Use this program to write text to a log file in /vnmr/tmp  */

#ifndef VNMRS_WIN32

void logprintf( char *fmt, ... )
{
	va_list	 vargs;
	char	 outputBuffer[ 122 ];
	char	 logFileName[ 32 ];
	FILE	*logfile;

	va_start(vargs, fmt);
	vsprintf(&outputBuffer[ 0 ], fmt, vargs);
	va_end(vargs);

	strcpy( &logFileName[ 0 ], "/vnmr/tmp/" );
	strcat( &logFileName[ 0 ], &programName[ 0 ] );
	strcat( &logFileName[ 0 ], ".log" );
	logfile = fopen( &logFileName[ 0 ], "a+" );
	fprintf( logfile, "%s", &outputBuffer[ 0 ] );
	fclose( logfile );
}


/**************************************************************
 *
 *  outputDeviceInit - select /dev/tty or /dev/console
 *
 */

static void
initOutputDevice()
{
	FILE	*console;

	console = fopen( "/dev/tty", "a+" );
	if (console == NULL) {
		console = fopen( "/dev/console", "a+" );
		if (console == NULL)
		  outputDevice[ 0 ] = '\0';
		else
		  strcpy( &outputDevice[ 0 ], "/dev/console" );
	}
	else {
		strcpy( &outputDevice[ 0 ], "/dev/tty" );
	}
	if (console != NULL)
	  fclose( console );
}

/**************************************************************
*
*  logSysInit - Initialize Syslog for application.
*
* This routine initializes the syslog log file for the calling
* application process. 
*  
* idstr  is typically the process name, e.g. Acqproc, Autoproc, etc...
* facility is the facility parameter assiociated with syslog. For our
* applications this should be LOG_LOCAL0-7 (Reserved for local use.)
* see syslog for further details.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void logSysInit(char* idstr, int facility)
/* char* idstr - Id string preappend to Syslog messages */
/* int facility - facility parameter assigned to all messages (syslog) */
{
   int iter;
   long openmax = sysconf(_SC_OPEN_MAX);

   for (iter = 0; iter < openmax; iter++)
     close(iter);			/* close any inherited open file descriptors */
   openlog(idstr, (LOG_PID | LOG_CONS | LOG_NOWAIT), facility);
   initOutputDevice();
   strncpy( &programName[ 0 ], idstr, sizeof( programName ) - 1 );
   programName[ sizeof(programName ) - 1 ] = '\0';
   return;
}

/**************************************************************
*
*  logSysClose - CLose Syslog for application.
*
* This routine closes the syslog log file for the 
* application process. 
*  
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void logSysClose()
{
   closelog();
   return;
}




/**************************************************************
*
*  diagPrint - Diagnostic Print routine not called directly but used via DPRINT1-5(...) cpp Macros.
*
* This routine prints the given string on the standard output
*  
* Typical usage:
*   DPRINT1(level,"Message String %d\n",arg1);
*   if DEBUG is defined during compilation then the print statement
*is expanded to the form: 
*     if(DebugLevel>=level)diagPrint(debugInfo,string,arg1)
*  otherwise the print statement is not present at all.
*  Level is a debugging level at which one wishes the message to be printed.
*This is compared to the global debug level parameter "DebugLevel".
*It up to the user to set DebugLevel to some level (0-#).
*Thus if DebugLevel = 0, level = 1 then no message is printed,
*however if DebugLevel = 1 or greater then the Message would be printed.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void diagPrint(char* fileNline, char *fmt, ...)
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   int len;
   FILE *console;
   time_t  timet;
   struct tm tmtime;
   char timestamp[40];

   char DiagMsgStr[8192] = { 0 };
   

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(DiagMsgStr,"%s",fileNline);
      len = strlen(DiagMsgStr);
   }

   va_start(vargs, fmt);
   vsprintf((DiagMsgStr + len), fmt, vargs);
   va_end(vargs);

   initOutputDevice();
   if (strlen( &outputDevice[ 0 ] ) > 0)
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         /* -------- ANSI way of getting time of day */
         /* size = sizeof(timestamp); */
         timet = time(NULL);
         localtime_r(&timet,&tmtime);
         strftime(timestamp,sizeof(timestamp),"%H:%M:%S",&tmtime);
         /* ------------------------------------ */

         fprintf(console,"%s: ",timestamp);
         fprintf(console,"%s",DiagMsgStr);
         fclose( console );
      }
   }
   else
   {
      syslog(LOG_ERR, "%s", DiagMsgStr);
   }

   return;
}


/**************************************************************
*
*  errLogRet - Print/Log Application Error
*
* This routine prints and optionally logs an application error 
*  then returns to the calling function.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogRet(int logopt, char *fileNline, char *fmt, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   int len;
   FILE *console;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fmt);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, "%s", ErrMsgStr);
   else
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }

   return;
}

/**************************************************************
*
*  errLogQuit - Print/Log Application Error
*
* This routine prints and optionally logs an application error 
*  then quits the application via exit(1).
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogQuit(int logopt, char *fileNline, char *fmt, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   int len;
   FILE *console;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fmt);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, "%s", ErrMsgStr);
   else
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }

   exit(1);
}

/**************************************************************
*
*  errLogSysRet - Print/Log Application Error
*
* This routine prints and optionally logs a system error,
*the system errno message is appended, then returns to the
*calling function.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogSysRet(int logopt, char *fileNline, char *fmt, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   int len;
   FILE *console;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fmt);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   errPrtMsg(ErrMsgStr);   /* appends system Error to message */

   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, "%s", ErrMsgStr);
   else
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }

   return;
}

/**************************************************************
*
*  errLogSysQuit - Print/Log Application Error
*
* This routine prints and optionally logs a System error,
*the system errno message is appended, 
*then quits the application via exit(1).
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogSysQuit(int logopt, char *fileNline, char *fmt, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
    va_list vargs;
    int len;
    FILE *console;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fmt);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);


   errPrtMsg(ErrMsgStr);		/* appends system Error to message */

   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, "%s", ErrMsgStr);
   else
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }

   exit(1);
}

/**************************************************************
*
*  errLogSysDump - Print/Log Application Error then dumps core
*
* This routine prints and optionally logs a System error,
*the system errno message is appended, 
*then dumps core via abort().
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogSysDump(int logopt, char *fileNline, char *fmt, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   int len;
   FILE *console;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fmt);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   errPrtMsg(ErrMsgStr);		/* appends system Error to message */

   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, "%s", ErrMsgStr);
   else
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }

   abort();
   exit(1);
}

/**************************************************************
*
*  errPrtMsg - appends errno system message to end of Error String
*
* This routine is for internal library use.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
int errPrtMsg(char *ErrMsgStr)
{
   register int len;
   char *sysErrStr();

   len = strlen(ErrMsgStr);
   sprintf(ErrMsgStr + len, " %s", sysErrStr());
   return(0);
}

/**************************************************************
*
*  sysErrStr - return pointer to System errno message
*
* This routine is for internal library use.
*
* RETURNS:
* char*  
*
*       Author Greg Brissey 7/18/94
*/
char* sysErrStr()
{
   static char mstr[200];
   char *str_err;

   if (errno != 0)
   {
      if ( (str_err = strerror(errno) ) != NULL )
	     sprintf(mstr,"(%s)", str_err);
      else
	     sprintf(mstr,"(errno = %d)", errno);
   }
   else
   {
      mstr[0] = '\0';
   }

   return(mstr);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

#else  /* WIN32 native */

/* *************************************************************** */
/* *************************************************************** */

/**************************************************************
*
*  errPrtMsg - appends errno system message to end of Error String
*
* This routine is for internal library use.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
int errPrtMsg(char *ErrMsgStr, DWORD errorNum)
{
   register size_t len;
   LPVOID lpMsgBuf;
   DWORD  nchars;

   nchars = FormatMessage( 
                (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                 FORMAT_MESSAGE_FROM_SYSTEM | 
                 FORMAT_MESSAGE_IGNORE_INSERTS) ,
                 NULL,
                 errorNum,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                ( LPTSTR) &lpMsgBuf,
                 0,
                NULL );

	if (nchars == 0)
    {       
	    DWORD fmtErr;
		fmtErr = GetLastError();
		// Display the string.
        // MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );

		printf("errPrtMsg(): FormatMessage failed, error %d\n",fmtErr);
	   // Free the buffer.
	   LocalFree( lpMsgBuf );
	   return(0);
	}
	len = strlen(ErrMsgStr);
    sprintf(ErrMsgStr + len, " %s", lpMsgBuf);
	//MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error: ", MB_OK | MB_ICONINFORMATION );
	//MessageBox( NULL, (LPCTSTR)ErrMsgStr, "Error: ", MB_OK | MB_ICONINFORMATION );

   return(0);
}

/**************************************************************
*
*  sysErrStr - return pointer to System errno message
*
* This routine is for internal library use.
*
* RETURNS:
* char*  
*
*       Author Greg Brissey 7/18/94
*/
char* sysErrStr(int errorNum)
{
   static char mstr[200];
  
   if (!FormatMessage( 
                (FORMAT_MESSAGE_FROM_SYSTEM | 
                 FORMAT_MESSAGE_IGNORE_INSERTS) ,
                 NULL,
                 errorNum,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                ( LPTSTR) mstr,
                 200,
                NULL ))
    {       
	   DWORD fmtErr;
	   fmtErr = GetLastError();
	   printf("errPrtMsg(): FormatMessage failed, error: %d\n",fmtErr);
	   mstr[0] = '\0';
	   
	}
   return(mstr);
}



/**************************************************************
*
*  diagPrint - Diagnostic Print routine not called directly but used via DPRINT1-5(...) cpp Macros.
*
* This routine prints the given string on the standard output
*  
* Typical usage:
*   DPRINT1(level,"Message String %d\n",arg1);
*   if DEBUG is defined during compilation then the print statement
*is expanded to the form: 
*     if(DebugLevel>=level)diagPrint(debugInfo,string,arg1)
*  otherwise the print statement is not present at all.
*  Level is a debugging level at which one wishes the message to be printed.
*This is compared to the global debug level parameter "DebugLevel".
*It up to the user to set DebugLevel to some level (0-#).
*Thus if DebugLevel = 0, level = 1 then no message is printed,
*however if DebugLevel = 1 or greater then the Message would be printed.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void diagPrint(char* fileNline, ...)
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   char *fmt;
   size_t len;
   __time64_t long_time;
   struct tm *tmtime;
   char timestamp[40];
   char am_pm[] = "AM";

   char DiagMsgStr[8192] = { 0 };
   

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(DiagMsgStr,"%s",fileNline);
      len = strlen(DiagMsgStr);
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((DiagMsgStr + len), fmt, vargs);
   va_end(vargs);

   _time64( &long_time );                /* Get time as long integer. */
   tmtime = _localtime64( &long_time ); /* Convert to local time. */

   //if( tmtime->tm_hour > 12 )        /* Set up extension. */
   //    strcpy( am_pm, "PM" );
   //if( tmtime->tm_hour > 12 )        /* Convert from 24-hour */
   //    tmtime->tm_hour -= 12;    /*   to 12-hour clock.  */
   //if( tmtime->tm_hour == 0 )        /*Set hour to 12 if midnight. */
   //    tmtime->tm_hour = 12;
   //printf( "%.19s %s\n", asctime( tmtime ), am_pm );  /* Tue Feb 12 10:05:58 AM */

   strftime(timestamp,sizeof(timestamp),"%H:%M:%S",tmtime);
   printf("%s: ",timestamp);
   printf("%s",DiagMsgStr);

   return;
}

/**************************************************************
*
*  errLogRet - Print/Log Application Error
*
* This routine prints and optionally logs an application error 
*  then returns to the calling function.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogRet(int logopt, char *fileNline, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   char *fmt;
   size_t len;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   printf("%s",ErrMsgStr);
/*
   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, ErrMsgStr);
   else
   {
      //console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }
*/

   return;
}


/**************************************************************
*
*  errLogSysRet - Print/Log Application Error
*
* This routine prints and optionally logs a system error,
*the system errno message is appended, then returns to the
*calling function.
*
* RETURNS:
* void 
*
*       Author Greg Brissey 7/18/94
*/
void errLogSysRet(int logopt, char *fileNline, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   char *fmt;
   size_t len;
   DWORD errNum;
   char ErrMsgStr[8192] = { 0 };

   errNum = GetLastError();

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   errPrtMsg(ErrMsgStr, errNum);   /* appends system Error to message */

   printf("%s",ErrMsgStr);

/*   initOutputDevice();
   if (logopt == LOGIT || strlen( &outputDevice[ 0 ] ) < 1)
      syslog(LOG_ERR, ErrMsgStr);
   else
   {
      console = fopen( &outputDevice[ 0 ], "a+" );
      if (console != NULL)
      {
         fprintf(console,"%s",ErrMsgStr);
         fclose( console );
      }
   }
*/

   return;
}

void errLogQuit(int logopt, char *fileNline, ...)
{
	va_list vargs;
   char *fmt;
   size_t len;
   char ErrMsgStr[8192] = { 0 };

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   printf("%s",ErrMsgStr);

   exit(1);
}

void errLogSysQuit(int logopt, char *fileNline, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   char *fmt;
   size_t len;
   DWORD errNum;
   char ErrMsgStr[8192] = { 0 };

   errNum = GetLastError();

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   errPrtMsg(ErrMsgStr, errNum);   /* appends system Error to message */

   printf("%s",ErrMsgStr);

   exit(1);
}
void errLogSysDump(int logopt, char *fileNline, ...)
/* int logopt - flag that indicates to log message or not */
/* char* fileNline - String containing file names & line number */
{
	va_list vargs;
   char *fmt;
   size_t len;
   DWORD errNum;
   char ErrMsgStr[8192] = { 0 };

   errNum = GetLastError();

   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      len = strlen(ErrMsgStr);
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   errPrtMsg(ErrMsgStr, errNum);   /* appends system Error to message */

   printf("%s",ErrMsgStr);

   abort();
   exit(1);
}
#endif  /* VNMRS_WIN32 */
