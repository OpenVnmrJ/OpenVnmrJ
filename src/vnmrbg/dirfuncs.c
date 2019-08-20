/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|
|
|    This module contains functions that recursively scan a directory
|
|    It would have gone into shellcmds.c but the fts.h include
|    file is not compatable with -D_FILE_OFFSET_BITS==64
|
+----------------------------------------------------------------------------*/

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fts.h>
#include <sys/stat.h>
#include <regex.h>

#include "vnmrsys.h"
#include "variables.h"
#include "tools.h"
#include "wjunk.h"

extern mode_t str_to_octal(char *ptr );
extern int assignString(const char *s, varInfo *v, int i);

/*---------------------------------------------------------------------------
|
|    Chmod
|
|    This routine does almost the equivalent of chmod
|
+----------------------------------------------------------------------------*/
int Chmod(int argc, char *argv[], int retc, char *retv[])
{
   char *argv2[2];
   int recurse;
   mode_t mode = 0;
   int modeMask = 0;
   int user, group, other;
   int addRemove = 0;
   int index;
   int ch;
   struct stat buf;

   if (argc != 3)
   {
      Werrprintf("%s: two arguments must be supplied",argv[0]);
      ABORT;
   }

   if (stat(argv[2], &buf))
   {
      if (retc > 0)
         retv[0] = intString(0);
      else
         Werrprintf("%s: cannot access %s",argv[0],argv[2]);
      RETURN;
   }

   recurse = (strstr(argv[1],"-R") == NULL) ? 0 : 1;
   argv2[0] = argv[2];
   argv2[1] = NULL;

   user = group = other = 0;
   addRemove = 0;
   index = (recurse) ? 2 : 0;
   if (recurse && ! S_ISDIR(buf.st_mode) )
      recurse = 0;
   while ( (ch = argv[1][index]) != '\0')
   {
      if ((ch >= '0') && (ch <= '7'))
      {
         if (recurse)
         {
            Werrprintf("%s: cannot use -R and numerical mode",argv[0]);
            ABORT;
         }
         if ( (argv[1][index] < '0') || (argv[1][index] > '7') ||
              (argv[1][index+1] < '0') || (argv[1][index+1] > '7') ||
              (argv[1][index+2] < '0') || (argv[1][index+2] > '7') ||
              (argv[1][index+3] != '\0') )
         {
            Werrprintf("%s: in numerical mode, three characters must be between 0 and 7 (%s)",
                       argv[0], &(argv[1][index]) );
            ABORT;
         }
         mode = str_to_octal(&(argv[1][index]));
         chmod(argv[2], mode);
         if (retc > 0)
            retv[0] = intString(1);
         RETURN;
      }
      if (ch == 'u')
         user = 1;
      else if (ch == 'g')
         group = 1;
      else if (ch == 'o')
         other = 1;
      else if (ch == 'a')
         user = group = other = 1;
      else if (ch == '+')
         addRemove = 1;
      else if (ch == '-')
         addRemove = -1;
      else if (ch == 'r')
      {
         if (user)
            modeMask |= S_IRUSR;
         if (group)
            modeMask |= S_IRGRP;
         if (other)
            modeMask |= S_IROTH;
      }
      else if (ch == 'w')
      {
         if (user)
            modeMask |= S_IWUSR;
         if (group)
            modeMask |= S_IWGRP;
         if (other)
            modeMask |= S_IWOTH;
      }
      else if (ch == 'x')
      {
         if (user)
            modeMask |= S_IXUSR;
         if (group)
            modeMask |= S_IXGRP;
         if (other)
            modeMask |= S_IXOTH;
      }
      else if (ch != ' ')
      {
         Werrprintf("%s: illegal character '%c'",argv[0],ch);
         ABORT;
      }
      index++;
   }

   if ( ((user == 0) && (group == 0) && (other == 0)) ||
        (addRemove == 0) || (modeMask == 0) )
   {
      Werrprintf("%s: illegal mode %s",argv[0],argv[1]);
      ABORT;
   }
   if ( ! recurse)
   {
      if (addRemove == 1)
         mode = buf.st_mode | modeMask;
      else
         mode = buf.st_mode & (~modeMask);
      chmod(argv[2], mode);
      if (retc > 0)
         retv[0] = intString(1);
      RETURN;
   }
   FTS *tree = fts_open(argv2, FTS_NOCHDIR|FTS_PHYSICAL, NULL);
   if (!tree)
   {
      if (retc > 0)
         retv[0] = intString(0);
      else
         Werrprintf("%s: cannot open %s",argv[0],argv[2]);
      RETURN;
   }

   errno = 0;
   FTSENT *node;
   while ((node = fts_read(tree)))
   {
      if (addRemove == 1)
         mode = (node->fts_statp)->st_mode | modeMask;
      else
         mode = (node->fts_statp)->st_mode & (~modeMask);
      chmod(node->fts_path, mode);
   }

   if (fts_close(tree))
   {
      if (retc > 0)
         retv[0] = intString(0);
      else
         Werrprintf("%s: cannot close %s",argv[0],argv[2]);
      RETURN;
   }
   if (errno)
   {
      if (retc > 0)
         retv[0] = intString(0);
      else
         Werrprintf("%s: cannot read %s",argv[0],argv[2]);
      RETURN;
   }
   if (retc > 0)
      retv[0] = intString(1);
   RETURN;
}

// The calling routine should make sure the directory dirname
// exists before calling this function
int Rmdir(char *dirname, int rmParent)
{
   char *argv2[2];
   argv2[0] = dirname;
   argv2[1] = NULL;

   errno = 0;
   FTS *tree = fts_open(argv2, FTS_NOCHDIR|FTS_PHYSICAL, NULL);
   if (!tree)
      return(-1);

   FTSENT *node;
   while ((node = fts_read(tree)))
   {
      if ( (node->fts_info == FTS_F) || (node->fts_info == FTS_SL) || (node->fts_info == FTS_SLNONE) )
      {
//         fprintf(stderr,"fts_path: %s\n",node->fts_path);
//         fprintf(stderr,"fts_name %s\n",node->fts_name);
//         fprintf(stderr,"rm %s\n",node->fts_path);
         unlink(node->fts_path);
      }
      else if (node->fts_info == FTS_DP)
      {
//         fprintf(stderr,"supplied dirname: %s\n",dirname);
//         fprintf(stderr,"fts_path: %s\n",node->fts_path);
//         fprintf(stderr,"fts_name %s\n",node->fts_name);
         if (rmParent || strcmp(dirname,node->fts_path) )
         {
//         fprintf(stderr,"rmdir : %s\n",node->fts_path);
            rmdir(node->fts_path);
         }
      }
   }
   if (fts_close(tree))
      return(-3);
   if (errno)
      return(-2);
   return(0);
}

// The calling routine should make sure the directories
// exists before calling this function
int Cpdir(char *fromdir, char *todir)
{
   char toPath[MAXPATH];
   char *argv2[2];

   argv2[0] = fromdir;
   argv2[1] = NULL;

   errno = 0;
   FTS *tree = fts_open(argv2, FTS_NOCHDIR|FTS_PHYSICAL, NULL);
   if (!tree)
   {
      Werrprintf("cp: trouble opening %s",fromdir);
      return(-1);
   }

   FTSENT *node;
   while ((node = fts_read(tree)))
   {
      if ( (node->fts_info == FTS_F) || (node->fts_info == FTS_SL) )
      {
         strcpy(toPath,todir);
         strcat(toPath,"/");
         strcat(toPath,node->fts_name);
         copyFile(node->fts_path,toPath,0);
      }
   }
   if (fts_close(tree))
      return(-3);
   if (errno)
      return(-2);
   return(0);
}

/*------------------------------------------------------------------------
|
|	This module returns the name of the n th file
|	in the selected directory
|
+-----------------------------------------------------------------------*/
int getitem(char *dirname, int index, char *filename, char *fileCmp, int recurse,
                   int regExp, int saveInPar, varInfo *v1)
{
   regex_t regex;
   regmatch_t matches;
   int reti;
   int cnt;
   size_t len = 0;

   char *argv2[2];
   argv2[0] = dirname;
   argv2[1] = NULL;

   if (recurse)
   {
      len = strlen(dirname);
      if ( dirname[len-1] != '/')
         len++;
   }
   errno = 0;
   FTS *tree = fts_open(argv2, FTS_NOCHDIR|FTS_PHYSICAL, NULL);
   if (!tree)
   {
      Werrprintf("getfile: trouble opening %s",dirname);
      return(-1);
   }

   FTSENT *node = NULL;
   if (regExp)
   {
      reti = regcomp(&regex, fileCmp, REG_EXTENDED|REG_NOSUB);
      if (reti)
      {
         char buf[MAXPATH];
         regerror(reti, &regex , buf, sizeof(buf) );
         fts_close(tree);
         Werrprintf("regular expression '%s' has error '%s'",fileCmp,buf);
         return(-1);
      }
   }
   cnt = 0;
   while ( (node = fts_read(tree)) )
   {
      int found;

      found = 0;
      if ( (*node->fts_name != '.') || (regExp == 2) )  /* no . files unless regular expressions are used */
      {
         if ( (node->fts_info == FTS_F) || (node->fts_info == FTS_SL) )
         {
         
            if ( regExp )
            {
               if (regexec(&regex, node->fts_name, 0, &matches, 0) == 0)
               {
                  cnt++;
                  found = 1;
               }
            }
            else if ( ! strlen(fileCmp)  || (strstr(node->fts_name, fileCmp) != NULL) )
            {
               cnt++;
               found = 1;
            }
         }
         else if (node->fts_info == FTS_D)
         {
            if ( regExp )
            {
               if (regexec(&regex, node->fts_name, 0, NULL, 0) == 0)
                  if ( strcmp(dirname,node->fts_path) )
                  {
                     cnt++;
                     found = 1;
                  }
            }
            else if ( ! strlen(fileCmp)  || (strstr(node->fts_name, fileCmp) != NULL) )
            {
               if ( strcmp(dirname,node->fts_path) )
               {
                  cnt++;
                  found = 1;
               }
            }
            if (( ! recurse ) && strcmp(dirname,node->fts_path) )
               fts_set(tree, node, FTS_SKIP);
         }
      }
      if (found && saveInPar)
      {
         if (cnt == 1)
            assignString("",v1,0);
         if (recurse)
            assignString(&node->fts_path[len],v1,cnt);
         else
            assignString(node->fts_name,v1,cnt);
      }
      if (index && (index == cnt))
      {
         if (recurse)
            strcpy(filename,&node->fts_path[len]);
         else
            strcpy(filename,node->fts_name);
         break;
      }
   }
   if (regExp)
   {
      regfree(&regex);
   }
   if (fts_close(tree))
   {
      Werrprintf("fts close error");
      return(-1);
   }
   if (index && (index != cnt))
   {
        Werrprintf("cannot get item %d from %s",index,dirname);
	return(-1);
   }	
   if (errno)
   {
      Werrprintf("errno error %d",errno);
      return(-1);
   }
   return(cnt);
}

/*------------------------------------------------------------------------
|
|	This module returns the name of the n th file
|	in the selected directory, sorted
|
+-----------------------------------------------------------------------*/
struct sort {
   char path[MAXPATH];
};
static int doSort(const void *s1, const void *s2)
{
   struct sort *si1 = (struct sort *) s1;
   struct sort *si2 = (struct sort *) s2;
   return( strcmp( si1->path, si2->path) );
}

int getitem_sorted(char *dirname, int index, char *filename, char *fileCmp, int recurse,
                   int regExp, int saveInPar, varInfo *v1)
{
   regex_t regex;
   regmatch_t matches;
   int reti;
   int cnt;
   size_t len = 0;

   char *argv2[2];
   argv2[0] = dirname;
   argv2[1] = NULL;

   if (recurse)
   {
      len = strlen(dirname);
      if ( dirname[len-1] != '/')
         len++;
   }
   errno = 0;
   FTS *tree = fts_open(argv2, FTS_NOCHDIR|FTS_PHYSICAL, NULL);
   if (!tree)
   {
      Werrprintf("getfile: trouble opening %s",dirname);
      return(-1);
   }

   FTSENT *node = NULL;
   if (regExp)
   {
      reti = regcomp(&regex, fileCmp, REG_EXTENDED|REG_NOSUB);
      if (reti)
      {
         char buf[MAXPATH];
         regerror(reti, &regex , buf, sizeof(buf) );
         fts_close(tree);
         Werrprintf("regular expression '%s' has error '%s'",fileCmp,buf);
         return(-1);
      }
   }
   cnt = 0;
   while ( (node = fts_read(tree)))
   {
      if ( (*node->fts_name != '.') || (regExp == 2) )  /* no . files unless regular expressions are used */
      {
         if ( (node->fts_info == FTS_F) || (node->fts_info == FTS_SL) )
         {
         
            if ( regExp )
            {
               if (regexec(&regex, node->fts_name, 0, &matches, 0) == 0)
                  cnt++;
            }
            else if ( ! strlen(fileCmp)  || (strstr(node->fts_name, fileCmp) != NULL) )
            {
               cnt++;
            }
         }
         else if (node->fts_info == FTS_D)
         {
            if ( regExp )
            {
               if (regexec(&regex, node->fts_name, 0, NULL, 0) == 0)
                  if ( strcmp(dirname,node->fts_path) )
                  {
                     cnt++;
                  }
            }
            else if ( ! strlen(fileCmp)  || (strstr(node->fts_name, fileCmp) != NULL) )
            {
               if ( strcmp(dirname,node->fts_path) )
               {
                  cnt++;
               }
            }
            if (( ! recurse ) && strcmp(dirname,node->fts_path) )
               fts_set(tree, node, FTS_SKIP);
         }
      }
   }

   struct sort sortPath[cnt+1];
   fts_close(tree);
   tree = fts_open(argv2, FTS_NOCHDIR|FTS_PHYSICAL, NULL);
   cnt = 0;
   while ( (node = fts_read(tree)))
   {
      int found;

      found = 0;
      if ( (*node->fts_name != '.') || (regExp == 2) )
      {
         if ( (node->fts_info == FTS_F) || (node->fts_info == FTS_SL) )
         {
            if ( regExp )
            {
               if (regexec(&regex, node->fts_name, 0, &matches, 0) == 0)
               {
                  cnt++;
                  found = 1;
               }
            }
            else if ( ! strlen(fileCmp)  || (strstr(node->fts_name, fileCmp) != NULL) )
            {
               cnt++;
               found = 1;
            }
         }
         else if (node->fts_info == FTS_D)
         {
            if ( regExp )
            {
               if  (regexec(&regex, node->fts_name, 0, NULL, 0) == 0)
                  if ( strcmp(dirname,node->fts_path) )
                  {
                     cnt++;
                     found = 1;
                  }
            }
            else if ( ! strlen(fileCmp)  || (strstr(node->fts_name, fileCmp) != NULL) )
            {
               if ( strcmp(dirname,node->fts_path) )
               {
                  cnt++;
                  found = 1;
               }
            }
            if (( ! recurse ) && strcmp(dirname,node->fts_path) )
               fts_set(tree, node, FTS_SKIP);
         }
      }
      if (found)
      {
         if (recurse)
            strcpy(sortPath[cnt-1].path, &node->fts_path[len]);
         else
            strcpy(sortPath[cnt-1].path, node->fts_name);
      }
   }
   qsort( &sortPath[0], cnt, sizeof(struct sort), doSort);
   if (regExp)
   {
      regfree(&regex);
   }
   if (index)
      strcpy(filename, sortPath[index-1].path );
   else if (saveInPar)
   {
      assignString("",v1,0);
      for (index=1; index <= cnt; index++)
         assignString(sortPath[index-1].path,v1,index);
   }
   if (fts_close(tree))
   {
      Werrprintf("fts close error");
      return(-1);
   }
   if (errno)
   {
      Werrprintf("errno error %d",errno);
      return(-1);
   }
   return(cnt);;
}

