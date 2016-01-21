/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#include <stdlib.h>
#endif

#define OUT_FP stderr

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "error.h"

/****************************************************************************
 sys_error

 This function prints, on OUT_FP, the system error message that corresponds
 to the system error number in "errno",  then clears error number "errno".
 It returns the value of "errno".

 INPUT ARGS:
   message     A format string for the message to be printed.
   ...         Arguments for the format string, as in printf().
 OUTPUT ARGS:
   none
 RETURN VALUE:
   (errno)     The number of the system error.
 GLOBALS USED:
   errno       The number of the system error.
 GLOBALS CHANGED:
   errno       The number of the system error: set to zero.
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

int sys_error (char *message, ...)
{
#ifndef LINUX
   extern int   errno;
#endif
   /**************************************************************************
   LOCAL VARIABLES:

   p_args      Pointer to the (variable) list of arguments for the print.
   save_errno  Save the system error number from being changed by fprintf calls.
   */
   va_list p_args;
   int     save_errno = errno;

   if (save_errno > 0)
      (void)fprintf (OUT_FP, "SYSTEM-LEVEL ERROR: ");

   /* print the message to accompany the error */
   va_start (p_args, message);
   (void)vfprintf (OUT_FP, message, p_args);
   va_end (p_args);

   if (save_errno > 0)
   {
      (void)fprintf (OUT_FP, " (%d: %s)", save_errno, strerror(save_errno) );
      errno = 0;
   }
   (void)fprintf (OUT_FP, "\n");

   return save_errno;

}  /* end of function "sys_error" */

/****************************************************************************
 error

 This function prints an error message on OUT_FP.

 INPUT ARGS:
   message     A format string for the message to be printed.
   ...         Arguments for the format string, as in printf().
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void error (char *message, ...)
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_args      Pointer to the (variable) list of arguments for the print.
   */
   va_list p_args;

   va_start (p_args, message);
   (void)vfprintf (OUT_FP, message, p_args);
   va_end (p_args);

   (void)fprintf (OUT_FP, "\n");

}  /* end of function "error" */

/****************************************************************************
 fatal

 This function prints an error message on OUT_FP, then exits to the operating
 system via "error_exit()".

 INPUT ARGS:
   exit_code   An integer to be passed to the operating system by "error_exit()".
   message     A format string for the message to be printed.
   ...         Arguments for the format string, as in printf().
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void fatal (int exit_code, char *message, ...)
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_args      Pointer to the (variable) list of arguments for the print.
   */
   va_list p_args;

   va_start (p_args, message);
   (void)vfprintf (OUT_FP, message, p_args);
   va_end (p_args);

   (void)fprintf (OUT_FP, "\n");

   error_exit (exit_code);
}

/****************************************************************************
 error_exit

 This function prints an exit message on OUT_FP, then exits to the operating
 system.

 INPUT ARGS:
   exit_code   An integer to be passed to the operating system by "exit()".
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   exit_code   An integer to be passed to the operating system.
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void error_exit (int exit_code)
{
   (void)fprintf (OUT_FP, "Exiting program: exit code is %d\n", exit_code);
   exit (exit_code);

}  /* end of function "error_exit" */

#else /* not __STDC__ && not __cplusplus && not c_plusplus */

#include <varargs.h>

#include "error.h"

/*VARARGS0*/

int sys_error (va_alist)
 va_dcl
{
#ifndef LINUX
   extern int errno;
   extern int sys_nerr;
   extern char *sys_errlist[];
#endif
   va_list p_args;
   char *format;
   /* save the system error number from being changed by fprintf calls */
   int save_errno = errno;

   if (save_errno > 0)
      (void)fprintf (OUT_FP, "SYSTEM-LEVEL ERROR: ");

   va_start (p_args);
   format = va_arg (p_args, char *);
   (void)vfprintf (OUT_FP, format, p_args);
   va_end (p_args);

   if (save_errno > 0)
   {
      (void)fprintf (OUT_FP, " (%d", save_errno);
      if (save_errno < sys_nerr &&
          sys_errlist [save_errno] != NULL &&
          *sys_errlist [save_errno] != '\0')

         (void)fprintf (OUT_FP, ": %s", sys_errlist [save_errno]);

      (void)fprintf (OUT_FP, ")");
      errno = 0;
   }
   (void)fprintf (OUT_FP, "\n");

   return (save_errno);

}  /* end of function "sys_error" */

/*VARARGS0*/

void error (va_alist)
 va_dcl
{
   va_list p_args;
   char *format;

   va_start (p_args);
   format = va_arg (p_args, char *);
   (void)vfprintf (OUT_FP, format, p_args);
   va_end (p_args);

   (void)fprintf (OUT_FP, "\n");

}  /* end of function "error" */

/*VARARGS1*/

void fatal (exit_code, va_alist)
 int exit_code;
 va_dcl
{
   va_list p_args;
   char *format;

   va_start (p_args);
   format = va_arg (p_args, char *);
   (void)vfprintf (OUT_FP, format, p_args);
   va_end (p_args);

   (void)fprintf (OUT_FP, "\n");

   error_exit (exit_code);

}  /* end of function "fatal" */

void error_exit (exit_code)
 int exit_code;
{
   (void)fprintf (OUT_FP, "Exiting program: exit code is %d\n", exit_code);
   exit(exit_code);

}  /* end of function "error_exit" */

#endif
