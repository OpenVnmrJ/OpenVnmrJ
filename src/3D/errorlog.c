/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include "constant.h"

#define FT3D
#include "command.h"
#undef FT3D


static FILE	*logfd = NULL,
		*logfdmaster = NULL;

extern int	master_ft3d;		/* Master ft3d flag	*/


/*---------------------------------------
|					|
|	     Wlogprintf()/n		|
|					|
+--------------------------------------*/
void Wlogprintf(char *format, ...)
{
   char		str[1024];
   va_list	vargs;


   if (logfd != NULL)
   {
      va_start(vargs, format);
      (void) vsprintf(str, format, vargs);
      va_end(vargs);
      (void) fprintf(logfd, "%s\n", str);
      (void) fflush(logfd);
   }
}


/*---------------------------------------
|					|
|	     Wlogdateprintf()/n		|
|					|
+--------------------------------------*/
void Wlogdateprintf(char *format, ...)
{
   char         	str[1024],
			datetim[26],
			*chrptr;
   long			timedate;
   va_list		vargs;
   struct tm		*tmtime;
   struct timeval	clock;


   if (logfd != NULL)
   {
      va_start(vargs, format);
      (void) vsprintf(str, format, vargs);
      va_end(vargs);

      if ( !gettimeofday(&clock, NULL) )
      {
         timedate = clock.tv_sec;
         tmtime = localtime( &(timedate) );
         chrptr = asctime(tmtime);
         (void) strcpy(datetim, chrptr);
         (void) strcat(str, "\t");
         (void) strcat(str, datetim);
      }

      (void) fprintf(logfd, "%s", str);
      (void) fflush(logfd);

      if (logfdmaster != NULL)
      {
         (void) fprintf(logfdmaster, "%s", str);
         (void) fflush(logfdmaster);
      }
   }
}


/*---------------------------------------
|					|
|	     Werrprintf()/n		|
|					|
+--------------------------------------*/
void Werrprintf(char *format, ...)
{
   char		str[1024];
   va_list	vargs;


   va_start(vargs, format);
   (void) vsprintf(str, format, vargs);
   va_end(vargs);

   (void) printf("%s\n", str);
   Wlogprintf("%s", str);
}


/*---------------------------------------
|                                       |
|	 closemasterlogfile()/0		|
|					|
+--------------------------------------*/
void closemasterlogfile()
{
   if (logfdmaster != NULL)
      fclose(logfdmaster);
}


/*---------------------------------------
|                                       |
|	  openmasterlogfile()/3		|
|					|
+--------------------------------------*/
void openmasterlogfile(pinfo)
comInfo	*pinfo;
{
   char		logpath[MAXPATHL];


   if (!pinfo->logfile.ival)
      return;

   (void) strcpy(logpath, pinfo->datadirpath.sval);
   (void) strcat(logpath, "/log");

   if ( access(logpath, F_OK) )
   { /* make the directory if it does not already exist */
      if ( mkdir(logpath, 0777) )
      {
         if (errno != EEXIST)
            return;
      }
   }

   (void) strcat(logpath, "/master");
   if ( (logfdmaster = fopen(logpath, "w")) == NULL )
   {
      Werrprintf("\nopenmasterlogfile():  cannot open master log file `%s`",
			logpath);
   }
}


/*---------------------------------------
|                                       |
|	    closelogfile()/0		|
|					|
+--------------------------------------*/
void closelogfile()
{
   if (logfd != NULL)
      fclose(logfd);
}


/*---------------------------------------
|                                       |
|	     openlogfile()/3		|
|					|
+--------------------------------------*/
void openlogfile(pinfo, dimen, filenum)
int	dimen,
	filenum;
comInfo	*pinfo;
{
   char		logpath[MAXPATHL],
		fext[15];


   if (!pinfo->logfile.ival)
      return;

   (void) strcpy(logpath, pinfo->datadirpath.sval);
   (void) strcat(logpath, "/log");

   if ( access(logpath, F_OK) )
   { /* make the directory if it does not already exist */
      if ( mkdir(logpath, 0777) )
      {
         if (errno != EEXIST)
            return;
      }
   }

   (void) strcat(logpath, "/f");

   switch (dimen)
   {
      case F1_DIMEN:	(void) strcat(logpath, "1."); break;
      case F2_DIMEN:	(void) strcat(logpath, "2."); break;
      case F3_DIMEN:	(void) strcat(logpath, "3"); break;
      default:		Werrprintf("\nopenlogfile():  invalid dimension");
			break;
   }

   if (dimen != F3_DIMEN)
   {
      sprintf(fext, "%d", filenum);
      (void) strcat(logpath, fext);
   }

   if ( (logfd = fopen(logpath, "w")) == NULL )
      Werrprintf("\nopenlogfile():  cannot open log file `%s`", logpath);
}
