/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
	static char Sid[] = "@(#)nfgets.c 18.1 03/21/08 20:01:35 Dave Woodworth's perversion of fgets()";
#endif (not) lint

#include <stdio.h>

/****************************************************************************
 nfgets

 This function mimics the standard library function "fgets()", except that
 it removes any trailing newline and returns the number of characters read.

 INPUT ARGS:
   s           A pointer to a buffer for storing the characters read.
   max         The maximum number of characters to be returned, including
               a NULL terminator for the string.
   stream      A pointer to the input stream to be read.
 OUTPUT ARGS:
   s           The characters read.
 RETURN VALUE:
   (int)       The number of characters read.
   EOF         End-of-File read on "stream".
 GLOBALS USED:
   none
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   If any characters (other than a newline) are read before EOF is reached,
   the EOF is pushed back into the input stream for the next read.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

int nfgets (char *s, int max, FILE *stream)

#else

int nfgets (s, max, stream)
 char *s;
 int   max;
 FILE *stream;

#endif
{
   /**************************************************************************
   LOCAL VARIABLES:

   i     A counter for the number of characters read from "stream".
   c     The character read from "stream".
   */
   register int i;
   register int c;

   /* reduce the allowed length by one, for the NULL terminator */
   --max;

   /* read characters until a newline, EOF, or allowed size of buffer */
   for (i = 0; i < max && (c = getc (stream)) != EOF && c != '\n'; ++i)
      *s++ = c;

   /* terminate the string: removes any newline present */
   *s = '\0';

   /* check for EOF; if other characters were read, save it for next time */
   if (c == EOF)
   {
      /* if nothing else was read, return End-of-File */
      if (i == 0)
         return (EOF);

      /* save the EOF for next time */
      (void)putc (c, stream);
   }
   return i;
}
