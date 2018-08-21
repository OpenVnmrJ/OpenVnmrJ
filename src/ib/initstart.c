/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

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
*  This file contains routines related to initialization file.		*
*     - Read int window position from a file.				*
*     - Initialize environment name					*
*     - Return environment name for window.init and colormap.init.	*
*									*
*************************************************************************/
#include <stdio.h>
#ifdef LINUX
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdarg.h>
#include <sys/types.h>
#include <stdlib.h>
#include "stderr.h"

#define	MAX_CMS	256

static char *envname=NULL;	/* Pointer to environment name */

static char *next_token(char *ptr);

/************************************************************************
*									*
*  Skip a word, and return a pointer to the next word.			*
*									*/
static char *
next_token(char *ptr)
{
   while ((*ptr == ' ') || (*ptr == '\t'))
      ptr++;
   
   while ((*ptr != NULL) && ((*ptr != ' ') && (*ptr != '\t')))
      ptr++;

   return(ptr);
}

/************************************************************************
*									*
*  Get the object initialize values.					*
*  Return OK or NOT_OK.							*
*									*/
int
init_get_val(char *filename,	/* start-up filename */
	     char *objname,	/* object name */
	     char *format,	/* value formats */
	     ...)
{
   va_list vargs;	/* variable argument pointer */
   FILE *file;		/* pointer to the open file */
   char buf[128];	/* buffer to store a line of data */
   char *ptr;		/* pointer position of the buffer */
   char ch;		/* character format */
   char *err_string;	/* error string */

   /* Open initialization file */
   if ((file = fopen(filename, "r")) == NULL)
   {
      STDERR_1("init_get_val:cannot open %s.", filename);
      WARNING_OFF(Sid);
      return(NOT_OK);
   }

   while (fgets(buf, 128, file))
   {
      ptr = buf;

      while ((*ptr == ' ') || (*ptr == '\t'))
	 ptr++;

      if ((*ptr == NULL) || (*ptr == '#') || (*ptr == '\n')) /* STOP */
	 continue;

      /* If the program passes here, it means that this is an object name */

      err_string = NULL;

      /* Compare the object name */
      if (strncmp(objname, ptr, strlen(objname)) == 0)
      {
	 va_start(vargs, format);
	 while (ch = *format++)
	 {
	    ptr = next_token(ptr);
	    switch (ch)
	    {
	       case 'd':
		  if (sscanf(ptr, "%d", va_arg(vargs, int *)) != 1)
		     err_string="init_get_val:%s:%s:missing integer";
		  break;
	       case 'D':
		  if (sscanf(ptr, "%ld", va_arg(vargs, long *)) != 1)
		     err_string="init_get_val:%s:%s:missing long value";
		  break;
	       case 'u':
		  if (sscanf(ptr, "%u", va_arg(vargs, unsigned int *)) != 1)
		     err_string="init_get_val:%s:%s:missing unsigned integer";
		  break;
	       case 'U':
		  if (sscanf(ptr, "%lu", va_arg(vargs, unsigned long *)) != 1)
		     err_string="init_get_val:%s:%s:missing unsigned long";
		  break;
	       case 'o':
	       case 'O':
		  if (sscanf(ptr, "%o", va_arg(vargs, int *)) != 1)
		     err_string="init_get_val:%s:%s:missing octal";
		  break;
	       case 'x':
	       case 'X':
		  if (sscanf(ptr, "%x", va_arg(vargs, unsigned long *)) != 1)
		     err_string="init_get_val:%s:%s:missing hexadecimal";
		  break;
	       case 'f':
		  if (sscanf(ptr, "%f", va_arg(vargs, float *)) != 1)
		     err_string="init_get_val:%s:%s:missing floating";
		  break;
	       case 'F':
		  if (sscanf(ptr, "%lf", va_arg(vargs, double *)) != 1)
		     err_string="init_get_val:%s:%s:missing double";
		  break;
	       case 'c':
		  if (sscanf(ptr, "%c", va_arg(vargs, char *)) != 1)
		     err_string="init_get_val:%s:%s:missing character";
		  break;
	       case 's':
		  if (sscanf(ptr, "%s", va_arg(vargs, char *)) != 1)
		     err_string="init_get_val:%s:%s:missing string";
		  break;
	       default:
		  STDERR_1("init_get_val:no such format: %c", ch);
		  err_string = "%s:%s:Undefined character";
		  break;
	    }
	    if (err_string)
	    {
	       char temp[100];
	       (void)sprintf(temp,err_string,filename,objname);
	       STDERR(temp);
	       (void)fclose(file); 
	       va_end(vargs);
	       return(NOT_OK);
	    }
	 }
	 (void)fclose(file);
	 va_end(vargs);
	 return(OK);
      }
   }
   (void)fclose(file);
   return(NOT_OK);
}

/************************************************************************
*									*
*  Initialize the environment name you want to use.			*
*									*/
void
init_set_env_name(char *name)
{
   char *ptr;

   if (envname)
      free(envname);

   if ((ptr = getenv(name)) == NULL)
   {
      STDERR_1("Warning: Your environment %s doesn't exist.", name);
      envname = NULL;
   }
   else
   {
      if ((envname = strdup(ptr)) == NULL)
	 STDERR("init_set_env_name: Couldn't strdup");
   }
}

/************************************************************************
*									*
*  Get environment name for initialiazation.				*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return a pointer to the filename as:				*
*	$USER_ENV   ---> if you have called init_set_env_name()		*
*  or	$INITDIR    ---> if you didn't call init_set_env_name()		*
*									*/
char *
init_get_env_name(char *filename)
{
   if (envname == NULL)
   {
      char *env;           /* environment variable */
      if ((env = getenv("INITDIR")) == NULL) 
      {
         STDERR("Warning: Your default environment ``INITDIR'' is not set");
         (void)strcpy(filename, "./"); 
      }
      else
         (void)strcpy(filename, env); 
   }
   else
      (void)strcpy(filename, envname); 

   return(filename);
}

/************************************************************************
*									*
*  Get window initialization filename.					*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return pointer to the filename as:				*
*      $USER_ENV/window.init --> if you have called init_set_env_name()	*
*  or  $INITDIR/window.init  --> if you didn't call init_set_env_name()	*
*									*/
char *
init_get_win_filename(char *filename)
{
   if (envname == NULL)
   {
      char *env;           /* environment variable */
      if ((env = getenv("INITDIR")) == NULL) 
      {
         STDERR("Warning: Your default environment ``INITDIR'' is not set");
         (void)strcpy(filename, "window.init"); 
      }
      else
         (void)sprintf(filename, "%s/window.init", env);
   }
   else
      (void)sprintf(filename, "%s/window.init", envname);

   return(filename);
}

/************************************************************************
*									*
*  Get colormap initialization filename.				*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return pointer to the filename as:				*
*     $USER_ENV/colormap.init --> if you have called init_set_env_name()*
*  or $INITDIR/colormap.init  --> if you didn't call init_set_env_name()*
*									*/
char *
init_get_cmp_filename(char *filename)
{
   if (envname == NULL)
   {
      char *env;           /* environment variable */
      if ((env = getenv("INITDIR")) == NULL) 
      {
         STDERR("Warning: Your default environment ``INITDIR'' is not set");
         (void)strcpy(filename, "colormap.init"); 
      }
      else
         (void)sprintf(filename, "%s/colormap.init", env);
   }
   else
      (void)sprintf(filename, "%s/colormap.init", envname);

   return(filename);
}

/************************************************************************
*									*
*  Get CSI parameter initialization filename.				*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return pointer to the filename as:				*
*     $USER_ENV/csiparam.init --> if you have called init_set_env_name()*
*  or $INITDIR/csiparam.init  --> if you didn't call init_set_env_name()*
*									*/
char *
init_get_csiparam_filename(char *filename)
{
   if (envname == NULL)
   {
      char *env;           /* environment variable */
      if ((env = getenv("INITDIR")) == NULL) 
      {
         STDERR("Warning: Your default environment ``INITDIR'' is not set");
         (void)strcpy(filename, "csiparam.init"); 
      }
      else
         (void)sprintf(filename, "%s/csiparam.init", env);
   }
   else
      (void)sprintf(filename, "%s/csiparam.init", envname);

   return(filename);
}

/************************************************************************
*									*
*  Get initialization filename with given filename 			*
*  If the environment is not initialized, use the default name INITDIR.	*
*  Always return pointer to the filename as:				*
*     $USER_ENV/xxxx.init --> if you have called init_set_env_name()	*
*  or $INITDIR/xxxx.init  --> if you didn't call init_set_env_name()	*
*									*/
char *
init_get_filename(char *name, char *fullname)
{
   if (envname == NULL)
   {
      char *env;           /* environment variable */
      if ((env = getenv("INITDIR")) == NULL) 
      {
         STDERR("Warning: Your default environment ``INITDIR'' is not set");
         (void)sprintf( fullname, "%s", name );
       }
      else
         (void)sprintf( fullname, "%s/%s", env, name );
   }
   else
      (void)sprintf( fullname, "%s/%s", envname, name );

   return( fullname );
}


