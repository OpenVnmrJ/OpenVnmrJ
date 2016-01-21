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
*************************************************************************/
#include <stdio.h>
#include <pwd.h>
#ifdef LINUX
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdlib.h>

/* Shut off warining message */
static void dummy()
{
   Sid = Sid;
}

/************************************************************************
*									*
*  This routine will expand the filename if the (1st charcater) name    *
*  starting with '~' or '$'.  If it is not, it just duplicates the 	*
*  filename.								*
*  Retrun a pointer to the newname or NULL.				*
*									*/
char *
expand_filename(char *oldname, char *newname)
{
   char buf[128] ;		/* temporary buffer */

   /* Expand the filename if the first character is ~ or $ */
   if (*oldname == '~')
   {
      char *home;       /* pointer of environment variable */
      struct passwd *pw;/* password data stucture */
      int i;            /* loop counter */

      if (*(oldname+1) == '/')  /* Get the current user home directory */
      {
         if ((home = (char *)getenv("HOME")) == (char *)NULL)
            home = "/" ;
         (void)sprintf(newname,"%s%s",home,oldname+1);
      }
      else if (*(oldname+1) == NULL)
      {
         if ((home = (char *)getenv("HOME")) == (char *)NULL)
            home = "/" ;
	 (void)strcpy(newname,home);
      }
      else
      {
         /* Copy the username to user buffer */
         i = 0;
         oldname++;  /* Skip ~ */
         while ((*oldname != '/') && (*oldname != NULL))
            buf[i++] = *oldname++ ;
         buf[i] = 0;
       
         /* Find the user from password entry file */
         if ((pw = getpwnam(buf)) == NULL)
            return(NULL);

         (void)sprintf(newname,"%s%s", pw->pw_dir, oldname);
      }
   }
   else if (*oldname == '$')
   {   
      int i;            /* loop counter */
      char *home;       /* pointer of environment variable */
       
      /* Copy the environment variable into a buffer */
      i = 0;
       oldname++;  /* Skip $ */
       while ((*oldname != '/') && (*oldname != NULL))
          buf[i++] = *oldname++ ;
       buf[i] = 0;
       
       /* Get the environment name */
       if ((home = (char *)getenv(buf)) == (char *)NULL)
          return(NULL);

       (void)sprintf(newname,"%s%s",home, oldname);
   }   
   else
      (void)strcpy(newname, oldname); 

   return(newname);
}
