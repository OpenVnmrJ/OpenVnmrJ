/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	tools.c
|
|	These procedures and functions form a kit of useful tools.
|	This module contains general purpose procedures and 
|	functions that are use tools. Some of them are:
|
|	newString         - allocates and returns a pointer to a string 
|	newStringId       - allocates with ID and returns a pointer to a string 
|	newCat            - allocates and concatinates two strings 
|  	newCatId          - allocates with ID and concatinates two strings,
|			  the first string is released. 
|	newCatDontTouch   - allocates and concatinates two strings with
|			    out releasing first string.
|	newCatIdDontTouch - allocates with ID  and concatinates two strings 
|			    with out releasing first string.
|	intString         - contvert integer to string (allocates string)
|	rtoa              - real to string 
|	realString        - real to string with allocation 
|	stringReal        - string to real 
|	isReal            - function to determine if string can be
|			       converted to real 
|
|	isSymLink	  - returns 0 if file is a Symbolic Link
|	isHardLink	  - returns 0 if file is a Hard Link
|	move_file	  - rename a file
|	verify_copy	  - verify a copy-file operation worked
|	copy_file_verify  - copy a file and verify the operation
|	verify_fname	  - verify a file name is acceptable
|
+-----------------------------------------------------------------------------*/
#include "vnmrsys.h"
#include "tools.h"
#include "allocate.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#ifdef UNIX
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <sys/file.h>
#else 
#include  <stat.h>
#endif 

#ifndef MAXPATH
#define MAXPATH 256
#endif 

#ifdef VNMRJ
#include "vfilesys.h"
#include "wjunk.h"
#endif
/*------------------------------------------------------------------------------
|
|	newString/1
|	newStringId/2
|
|	This function is used to copy a character string into new storage.
|	A pointer to the newly allocated storage is returned.
|
+-----------------------------------------------------------------------------*/

char *newStringId(const char *p, const char *id)
{   char *q;

    if ( (q=allocateWithId(strlen(p)+1,id)) )
    {	strcpy(q,p);
	return(q);
    }
    else
    {	fprintf(stderr,"FATAL! out of memory\n");
	exit(1);
    }
}

char *newString(const char *p)
{  return(newStringId(p,"newString"));
}

/*------------------------------------------------------------------------------
|
|	newCat/2
|	newCatId/3
|	newCatDontTouch/2
|	newCatIdDontTouch/3
|
|	These functions return a new string formed by concatenating the
|	two given strings.  The first two will release (free) up the
|	malloced (or allocatedwithId) space of the first string.  The
|	routines with DontTouch will not free up any space.
|
+-----------------------------------------------------------------------------*/

char *newCatIdDontTouch(const char *a, const char *b, const char *id)
{   char *c;

    if ( (c=allocateWithId(strlen(a)+strlen(b)+1,id)) )
    {	strcpy(c,a);

#ifdef UNIX
	strcat(c,b);
#else 
	vms_fname_cat(c,b);
#endif 

	return(c);
    }
    else
    {	fprintf(stderr,"tools:out of memory (newCat) \n");
	exit(1);
    }
}

char *newCatDontTouch(const char *a, const char *b)
{  return(newCatIdDontTouch(a,b,"newCatDontTouch"));
}

char *newCatId(char *a, const char *b, const char *id)
{   char *c;

    if ( (c=allocateWithId(strlen(a)+strlen(b)+1,id)) )
    {	strcpy(c,a);

#ifdef UNIX
	strcat(c,b);
#else 
	vms_fname_cat(c,b);
#endif 

	release(a);  /* release memory for string a */
	return(c);
    }
    else
    {	fprintf(stderr,"tools:out of memory (newCat) \n");
	exit(1);
    }
}

char *newCat(char *a, const char *b)
{  return(newCatId(a,b,"newCat"));
}

/*------------------------------------------------------------------------------
|
|	intString/1
|
|	This function is used to convert a integer to a string.
|
+-----------------------------------------------------------------------------*/

char *intString(int i)
{   char tmp[32];

    sprintf(tmp,"%d",i);
    return(newStringId(tmp,"intString"));
}

/*------------------------------------------------------------------------------
|
|	rtoa/3
|
|	This function is used to convert a double to a string.
|
+-----------------------------------------------------------------------------*/

char *rtoa(double d, char *buf, int size)
{   char temp[128];

    sprintf(temp,"%13g",d);
    strncpy(buf,temp,size);
    buf[size-1]= '\0';
    if (strlen(temp) > size)
	fprintf(stderr,"rtoa: Warning,  value '%s' truncated to '%s'.\n",
	temp,buf);
    return (buf);
}

/*------------------------------------------------------------------------------
|
|	realString/1
|
|	This function is used to convert a real to a string.
|
+-----------------------------------------------------------------------------*/

char *realString(double d)
{   char tmp[32];

    sprintf(tmp,"%.13g",d);
    return(newStringId(tmp,"realString"));
}

/*------------------------------------------------------------------------------
|
|	stringReal/1
|
|	This function is used to convert a string to a real.
|
+-----------------------------------------------------------------------------*/

double stringReal(char *s)
{   double d;

    if (sscanf(s,"%lf",&d) == 1)
	return(d);
    else
    {	fprintf(stderr,"magic: can't convert \"%s\" to a real value, zero assummed\n",s);
	return(0.0);
    }
}

/*------------------------------------------------------------------------------
|
|	space/2
|
|	This procedure is used to output a given number of spaces to a given
|	stream.
|
+-----------------------------------------------------------------------------*/

void space(FILE *f, int n)
{   while (0 < n--)
	fprintf(f," ");
}

/*------------------------------------------------------------------------------
|
|	msg/1
|
|	This function returns a pointer to a printable message, if a NULL
|	pointer is passed in, a pointer to a zero length string is returned.
|	If a non-NULL pointer is given, it is simply returned.
|
+-----------------------------------------------------------------------------*/

const char *msg(const char *m)
{   if (m)
	return(m);
    else
	return("");
}

/*------------------------------------------------------------------------------
|
|	isReal/1
|
|	This function returns true (non-zero) if the given string can be
|	converted to a real.  It returns false (zero) otherwise.
|       The string 'inf' returns true.
|
+-----------------------------------------------------------------------------*/

int isReal(char *s)
{   double tmp;
    char *end;
    register char *end2;

    errno = 0;
    tmp = strtod(s, &end);
    if ((end == s) || (errno != 0))
	return(0);
    end2 = s + strlen(s);
    while ((end < end2) && isspace((unsigned char) *end)) {
        end++;
    }
    if (end != end2) {
	return(0);
    }

    return(! isnan(tmp));
}

/*------------------------------------------------------------------------------
|
|	isFinite/1
|
|	This function returns true (non-zero) if the given string can be
|	converted to a real.  It returns false (zero) otherwise.
|       The string 'inf' returns false.
|
+-----------------------------------------------------------------------------*/

int isFinite(char *s)
{   double tmp;
    char *end;
    register char *end2;

    errno = 0;
    tmp = strtod(s, &end);
    if ((end == s) || (errno != 0))
	return(0);
    end2 = s + strlen(s);
    while ((end < end2) && isspace((unsigned char) *end)) {
        end++;
    }
    if (end != end2) {
	return(0);
    }

    return(isfinite(tmp));
}

/****************************************************/
/*  Returns 0 if indicated file is a symbolic link  */
/****************************************************/

int isSymLink(char *lptr)
{
   int             ival;
   struct stat     unix_fab;

   ival = lstat(lptr, &unix_fab);
   if (ival != 0)
      return (131071);
   if ((unix_fab.st_mode & S_IFLNK) == S_IFLNK)
      return (0);
   else
      return (131071);
}

/****************************************************/
/*  Returns 0 if indicated file is a hard link      */
/****************************************************/

int isHardLink(char *lptr)
{
   int             ival;
   struct stat     unix_fab;

   ival = lstat(lptr, &unix_fab);
   if (ival != 0)
      return (131071);
   if (unix_fab.st_nlink > 1)
      return (0);
   else
      return (131071);
}

#ifdef VNMRJ
int make_copy_fidfile(char *prog, char *dir, char *msg)
{
   int     diskFull, ival;
   char    fidpath[ MAXPATH ], tmp[ MAXPATH*2 ];
   struct stat     unix_fab;
   strcpy(fidpath,dir);
   strcat(fidpath,"/acqfil/fid");
   if ( lstat(fidpath, &unix_fab) )
   {
      /* If there is no fid file, just return */
      if (access( fidpath, F_OK ))
         return(0);
      sprintf(tmp,"%s: FID file %s does not exist or is not readable",
                  prog, fidpath);
      if (msg == NULL)
         Werrprintf(tmp);
      else
         strcpy(msg,tmp);
      return(-1);
   }
   if ( (unix_fab.st_nlink > 1) || S_ISLNK(unix_fab.st_mode) )
   {
      ival = access( fidpath, R_OK );
      if (ival != 0) {
         sprintf(tmp,"%s: FID file %s does not exist or is not readable",
            prog, fidpath);
         if (msg == NULL)
            Werrprintf(tmp);
         else
            strcpy(msg,tmp);
         return( -1 );
      }
      ival = isDiskFullFile( dir, fidpath, &diskFull );
      if (diskFull) {
         sprintf(tmp,"%s: insufficent disk space to make a copy of the FID file",
            prog);
         if (msg == NULL)
            Werrprintf(tmp);
         else
            strcpy(msg,tmp);
         return( -1 );
      }

      strcpy(tmp, fidpath);
      strcat(tmp, "tmp" );

      ival = copy_file_verify(fidpath, tmp);
      if (ival != 0) {
         sprintf(tmp,"%s: error making a copy of the FID file",prog);
         if (msg == NULL)
            Werrprintf(tmp);
         else
            strcpy(msg,tmp);
         return( -1 );
      }
      unlink(fidpath);
      rename(tmp,fidpath);
   }
   RETURN;
}
#endif

/************************************************************************/
/*  A customer reported a bug in the SVF command which caused only the
    "procpar" file to be written out.  The "text" and "fid" were not
    present.  We believe this is due to SunOS not being able to fork
    a new process, since "procpar" is written out directly by the VNMR
    process while "text" and "fid" are wriiten using the UNIX command
    "cp" in a subprocess.

    To address this problem, we are added a new capability to let the
    current process verify the success of a copy operation.  Because
    of the concern expressed previously, this capability cannot rely
    on a child process to do its work.

    Eventually we would like to improve the interface between VNMR and
    its subprocesses so VNMR can check for errors better, including
    failure to spawn a subprocess.

    Pease notice that while one can copy a file to a directory, calling
    this program with the two parameters to that copy command will
    return a failure status; the file and the directory will surely be
    of different sizes.

    Expected errors:
	file a does not exist
	file b does not exist
	both files exist, but are of different sizes.			*/
/************************************************************************/

int verify_copy(char *file_a, char *file_b )
{
	int		ival;
	struct stat	stat_a, stat_b;

	ival = stat( file_a, &stat_a );
	if (ival != 0)
	  return( NO_FIRST_FILE );
	ival = stat( file_b, &stat_b );
	if (ival != 0)
	  return( NO_SECOND_FILE );
	if (stat_a.st_size != stat_b.st_size)
	  return( SIZE_MISMATCH );

	return( 0 );
}

int copyFile(const char *fromFile, const char *toFile, mode_t mode)
{
   int f1, f2;
   int n;
   char buffer[32768];

   if ( (f1=open(fromFile,O_RDONLY)) == -1 )
   {
      return(NO_FIRST_FILE);
   }
   if (mode == 0)
      mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
   if ( (f2=open(toFile,O_WRONLY|O_CREAT|O_TRUNC,mode)) == -1 )
   {
      close(f1);
      return(NO_SECOND_FILE);
   }
   while ( (n=read(f1, buffer, sizeof(buffer))) > 0 )
   {  if ( write(f2,buffer,n) != n )
      {
         close ( f1 );
         close ( f2 );
         return(SIZE_MISMATCH);
      }
   }
   close(f1);
   close(f2);
   return(0);
}

int copy_file_verify(char *file_a, char *file_b )
{
   return( copyFile(file_a, file_b, 0) );
}

/********************************************************************/
/* check_spaces - add escape characters before spaces in filenames  */
/*   note: filename length is compared with MAXPATH in verify_fname */
/********************************************************************/

char *check_spaces(char *inptr, char *outptr, int maxlen )
{
	char tchar, space_char = ' ';
	int  iter, jter, len;

	len = strlen( inptr );
	if (maxlen > MAXPATH+2) maxlen=MAXPATH+2;
	if (len < 0 || len > maxlen) 
	{	/* no substitutions */
		return(inptr);
	}
	jter = 0;
	for (iter = 0; iter<len; iter++) {
		tchar = *(inptr+iter);
		if (tchar == space_char) jter++;
	}
	if (jter <= 0)
	{	/* if no spaces, no substitutions */
		return(inptr);
	}
	else if ((len+jter) < maxlen)
	{	/* substitute '\' before space */
		jter = 0;
		for (iter = 0; (iter<len) && (jter<maxlen); iter++) {
		  tchar = *(inptr+iter);
		  if ((tchar == space_char) && ((jter-iter+len) < maxlen))
		  {
		    outptr[jter++] = '\\';
		  }
		  outptr[jter++] = tchar;
		}
		outptr[jter++] = '\0';
	}
	else if ((len+2) < maxlen)
	{ /* surround with "", fails with wildcards '*' '?' */
		strcpy(outptr,"\0");
		strncat(outptr,"\"",maxlen - 1);
		strncat(outptr,inptr,maxlen - strlen(outptr) - 1);
		strncat(outptr,"\"",maxlen - strlen(outptr) - 1);
		len = strlen( outptr );
		outptr[len] = '\0';
	}
	else /* no substitutions */
	{
		return(inptr);
	}
	return(outptr);
}

/************************************************************************/
/*  The routine "verify_fname" was written to help fix a problem
    where the 'svf' command would create file names with obscure
    characters, such as ' ', causing problems with UNIX.

    If the argument contains a character listed in the table or
    is a control character (ADE < 32), the routine returns -1;
    if no problems are detected, it returns 0.

    Each character in the table below is disallowed for one of
    the following reasons:
        It leads to inconvenient file names, e. g. ' '
	The shell uses the character for its own reasons, e. g. '<', '|'
	The character is a quotation character - ', ", `
	The file-name interpreter uses the character, e. g. '*', '?', '~'

    The NUL character serves to terminate this table.			*/
/************************************************************************/

#if defined(__INTERIX) || defined(MACOS)
/*  Space character allowed for Interix or MACOS */
static char illegal_fchars[] = {
	     '!', '"', '$', '&', '\'', '(', ')', '*', ';', '<', '>', '?',
	'\\', '[', ']', '^', '`', '{', '}', '|', ',', '\0' };
#else
static char illegal_fchars[] = {
        ' ', '!', '"', '$', '&', '\'', '(', ')', '*', ';', '<', '>', '?',
        '\\', '[', ']', '^', '`', '{', '}', '|', ',', '\0' };
#endif

/*  Returns 0 is supplied character is valid for a file name
 *  returns -1 if supplied character is not valid for a file name
 */
int verify_fnameChar(char tchar)
{
   char lchar;
   int  jter;
   if (tchar < 32 || tchar > 126)
      return( -1 );
   jter = 0;
   while ((lchar = illegal_fchars[ jter++ ]) != '\0')
      if (lchar == tchar)
         return( -1 );
   return(0);
}

int verify_fname(char *fnptr )
{
	char lchar;
        unsigned int tchar;
	int  iter, jter, len;

	len = strlen( fnptr );
	if (len < 0 || len > MAXPATH) return( -1 );

	for (iter = 0; iter < len; iter++) {
		tchar = *(fnptr+iter);
		if (tchar < 32 )
		  return( -1 );
		jter = 0;
		while ((lchar = illegal_fchars[ jter++ ]) != '\0')
		  if (lchar == tchar)
		    return( -1 );
	}

	return( 0 );
}

/* Same as verify_fname except it allows the space ' ' character */
int verify_fname2(char *fnptr )
{
	char lchar;
        unsigned int tchar;
	int  iter, jter, len;

	len = strlen( fnptr );
	if (len < 0 || len > MAXPATH) return( -1 );

	for (iter = 0; iter < len; iter++) {
		tchar = *(fnptr+iter);
		if (tchar < 32 )
		  return( -1 );
      if (tchar != ' ')
      {
		   jter = 0;
		   while ((lchar = illegal_fchars[ jter++ ]) != '\0')
		     if (lchar == tchar)
		       return( -1 );
      }
	}
	return( 0 );
}

/*
 * Alternative to strtok and strtok_r
 * s is string to search
 * buf is buffer to put the next word or token
 * len is length of buf
 * delim is the word or token delimiter
 */
char *strwrd(char *s, char *buf, size_t len, char *delim)
{
   size_t n;

   if ( s == NULL )
   {
      buf[0] = '\0';
      return ( NULL );
   }
   s += strspn(s, delim);
   n = strcspn(s, delim);
   if (len -1 < n)
      n = len -1;
   memcpy(buf, s, n);
   buf[n] = '\0';
   s += n;
   return ( (*s == '\0') ? NULL : s );
}
