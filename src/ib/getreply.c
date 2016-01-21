/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "boolean.h"
#include "error.h"

/****************************************************************************
 get_reply

 This function prints a prompt message at "stderr", then reads the user's
 response into the provided buffer, and returns the number of characters
 in the reply (or EOF when encountered).  Leading white space is removed.

 INPUT ARGS:
   prompt      A pointer to a string to be printed at stderr, to prompt the
               user for a reply.
   reply       A pointer to a buffer to hold the user's reply, with leading
               white space and trailing newline removed, and NULL-terminated.
   max         The allowed length of the response, including NULL-terminator.
   required    A flag: TRUE  => the user must enter non-white-space text
                       FALSE => white space is accepted for the reply
 OUTPUT ARGS:
   reply       The user's reply, with leading white space and trailing newline
               removed, and NULL-terminated.
 RETURN VALUE:
   n           The number of characters, less leading white space, in the
               reply. "n" can be zero only if "required" is FALSE.
   EOF         End-of-File read on input.
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function uses "nfgets()", a version of the standard library function
   "fgets()" that removes trailing newlines from its input, and returns the
   number of characters it has read.
   If any characters (other than newline) have been read when EOF is read,
   the EOF is pushed back onto the input for the next read.
   Replies that exceed the allowed length will be read until a newline or EOF
   is encountered.
   File I/O errors are not checked.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#define INPUT_BUFFER_SIZE 200  /* size of local buffer for editing input */

/* prototype for the function that reads user input from stdin */
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
extern int nfgets (char *s, int max, FILE *stream);
#else
extern int nfgets ();
#endif

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

int get_reply (char *prompt, char *reply, int max, int required)

#else

int get_reply (prompt, reply, max, required)
 char *prompt;
 char *reply;
 int   max;
 int   required;

#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   buffer   A local buffer for the user's reply; needed because leading white
            space is removed.
   p        A pointer into "buffer", for removing leading white space.
   n        A count for the number of characters read from function "nfgets()".
   c        An int, for reading the rest of too-long replies.
   */
   char  buffer [INPUT_BUFFER_SIZE];
   char *p;
   int   n;
   int   c;

   /* if needed, adjust the limit to match the local input buffer */
   if (INPUT_BUFFER_SIZE < max)
      max = INPUT_BUFFER_SIZE;

   /* print the prompt at stderr, and get a reply from stdin */
   for (;;)
   {
      /* print the prompt message */
      (void)fprintf (stderr, "%s ", prompt);

      /* get the user's reply */
      if ( (n = nfgets (p = buffer, max, stdin)) != EOF && n > 0)
      {
         /* check to see if an entire line was read */
         if (n == max - 1)
         {
            /* read and discard everything up to the next end-of-line */
            while ( (c = getchar()) != EOF && c != '\n')
               ;
            /* save the EOF for the next call */
            if (c == EOF)
               putchar (c);
         }
         /* strip off leading white space in the reply */
         while (isspace (*p))
         {
            ++p;
            --n;
         }
         /* check for something left */
         if (*p != '\0')
         {
            /* put the reply into the requested buffer */
            (void)strcpy (reply, p);

            return (n);
         }
      }
      if (required == TRUE)
         error ("Input required");
      else
         return (n);
   }
}  /* end of function "get_reply" */
