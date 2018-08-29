/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vnmrsys.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <limits.h>

#include "allocate.h"
#include "group.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"
#define _REENTRANT
#include <string.h>

#ifdef __INTERIX
#include <interix/interix.h>
#endif
static char *appdirPaths;
#ifndef PSG_LC
static char *appdirLabels;
static int appdirNumPaths = 0;
static int  appdirRight = -1;  /* can an operator use the appdir command */
static char lastOperator[MAXPATH] = "";
extern char UserName[];
extern char RevTitle[];
extern int rightsEval(char * rightsName);
#endif

#ifdef VNMRJ
extern int VnmrJViewId;
#endif

#ifdef __INTERIX
//#define TEST_PATH
void winPathToUnix(char *path,char *buff, int maxlength)
{
 	winpath2unix(path, 0, buff, maxlength);
#ifdef DEBUG_PATHS
	Winfoprintf("winpath2unix %s -> [%s]",path,buff);
#endif
}
void unixPathToWin(char *path,char *buff,int maxlength)
{
	char *ptr;
 	unixpath2win(path, 0, buff, maxlength);
	// c has issues with back-slashes in strings so convert them to forward slashes
	while ( (ptr = strchr(buff,'\\')) )
		*ptr = '/';
#ifdef TEST_PATH
	Winfoprintf("unixPathToWin %s -> [%s]",path,buff);
	char tmpPath[MAXPATH];
	winpath2unix(buff, 0, tmpPath, maxlength);
	Winfoprintf("winpath2unix %s -> [%s]",buff,tmpPath);
#endif
}
#else
void unixPathToWin(char *path,char *buff,int maxlength) {
	strncpy(buff,path,maxlength);
}
void winPathToUnix(char *path,char *buff,int maxlength) {
	strncpy(buff,path,maxlength);
}
#endif
#ifndef PSG_LC
/************************************************/
/*  getLineFromFile returns the next line from a file	*/
/************************************************/
static int getLineFromFile(FILE *path, char line[], int limit)
/**********************************/
{
  int ch,i;

  line[0] = '\0';
  i = 0;
  ch = '\0';
  while ((i < limit -1) && ((ch = getc(path)) != EOF) && (ch != '\n'))
    line[i++] = ch;
  line[i] = '\n';
  line[i+1] = '\0';
  if ((i == 0) && (ch == EOF))
     return(0);
  else
     return(1);
}

static void getAppdirTemplate(char *filepath)
{
   char buf[1024];
   FILE *path;
   int  found = 0;
   char *ptr;
   char defTemplate[MAXPATH];

   sprintf(filepath,"%s/adm/users/profiles/user/%s", systemdir, UserName);
   path = fopen(filepath,"r");
   if (path != NULL)
   {
      while (getLineFromFile(path, buf, sizeof(buf)))
      {
         if (strncmp(buf,"appdir",6) == 0)
         {
            ptr = &buf[6];
            while ( (*ptr == ' ') || (*ptr == '\t') )
               ptr++;
            if (strlen(ptr))
            {
               if (*(ptr+strlen(ptr)-1) == '\n')
               {
                  *(ptr+strlen(ptr)-1) = '\0';
               }
               sprintf(filepath,"%s/adm/users/userProfiles/appdir%s.txt", systemdir, ptr);
               if ( ! access(filepath,R_OK) )
               {
                  /* found an existing template name */
                  fclose(path);
                  return;
               }
            }
         }
         else if (  ! found && (strncmp(buf,"itype",5) == 0) )
         {
            ptr = &buf[5];
            while ( (*ptr == ' ') || (*ptr == '\t') )
               ptr++;
            if (strlen(ptr))
            {
               if (*(ptr+strlen(ptr)-1) == '\n')
                  *(ptr+strlen(ptr)-1) = '\0';
               /* use itype as default if appdir entry does not exist */
               if ( ! strcmp(ptr,"LC-NMR/MS") )
                  sprintf(defTemplate,"%s/adm/users/userProfiles/appdirLcNmrMs.txt", systemdir);
               else
                  sprintf(defTemplate,"%s/adm/users/userProfiles/appdir%s.txt", systemdir, ptr);
               found = 1;
            }
         }
      }
      fclose(path);
   }
   sprintf(filepath,"%s/adm/users/profiles/system/%s", systemdir, UserName);
   path = fopen(filepath,"r");
   if (path != NULL)
   {
      while (getLineFromFile(path, buf, sizeof(buf)))
      {
         if (strncmp(buf,"appdir",6) == 0)
         {
            ptr = &buf[6];
            while ( (*ptr == ' ') || (*ptr == '\t') )
               ptr++;
            if (strlen(ptr))
            {
               if (*(ptr+strlen(ptr)-1) == '\n')
                  *(ptr+strlen(ptr)-1) = '\0';
               sprintf(filepath,"%s/adm/users/userProfiles/appdir%s.txt", systemdir, ptr);
               if ( ! access(filepath,R_OK) )
               {
                  fclose(path);
                  return;
               }
            }
         }
         else if (  ! found && (strncmp(buf,"itype",5) == 0) )
         {
            ptr = &buf[5];
            while ( (*ptr == ' ') || (*ptr == '\t') )
               ptr++;
            if (strlen(ptr))
            {
               /* use itype as default if appdir entry does not exist */
               if (*(ptr+strlen(ptr)-1) == '\n')
                  *(ptr+strlen(ptr)-1) = '\0';
               if ( ! strcmp(ptr,"LC-NMR/MS") )
                  sprintf(defTemplate,"%s/adm/users/userProfiles/appdirLcNmrMs.txt", systemdir);
               else
                  sprintf(defTemplate,"%s/adm/users/userProfiles/appdir%s.txt", systemdir, ptr);
               found = 1;
            }
         }
      }
      fclose(path);
   }
   if (found)
   {
      strcpy(filepath,defTemplate);
   }
}
#endif


static void getappdirPaths()
{
#ifndef PSG_LC
   char filepath[MAXPATH];
   char buf[1024];
   FILE *path;
   int  onOff;
   char sPath[1024];
   char sLabel[1024];
   char operator[MAXPATH];
   int  systemInList = 0;

   if (P_getstring(GLOBAL,"operator",operator,1,MAXPATH-1))
   {
      strcpy(operator,UserName);
   }
   if ( (lastOperator[0] != '\0') && ! strcmp(operator,lastOperator) )
   {
      /* operator has not changed. */
      return;
   }
   strcpy(lastOperator, operator);
   path = NULL;
   if (appdirRight == -1)
      appdirRight = rightsEval("caneditappdir");
   if (appdirRight)
   {
      sprintf(filepath,"%s/persistence/appdir_%s", userdir, operator);
      if ( ! access(filepath,R_OK) )
      {
         path =  fopen(filepath,"r");
      }
   }
   /* Check for an owner based appdir file */
   if ( (path == NULL) && strcmp(UserName,operator) )
   {
      /* rightsEval is based on the operator parameter */
      P_setstring(GLOBAL,"operator",UserName,1);
      appdirRight = rightsEval("caneditappdir");
      if (appdirRight)
      {
         sprintf(filepath,"%s/persistence/appdir_%s", userdir, UserName);
         if ( ! access(filepath,R_OK) )
         {
            path =  fopen(filepath,"r");
         }
      }
      P_setstring(GLOBAL,"operator",operator,1);
      appdirRight = rightsEval("caneditappdir");
   }
   if (path == NULL)
   {
      getAppdirTemplate(filepath);
      path = fopen(filepath,"r");
   }
   if (path != NULL)
   {
      releaseWithId("app");
      appdirPaths = NULL;
      appdirLabels = NULL;
      appdirNumPaths = 0;
      while (getLineFromFile(path, buf, sizeof(buf)))
      {
         if ( (buf[0] == '0') || (buf[0] == '1') )
         {
            sLabel[0] = '\0';
            sscanf(buf,"%d;%[^;];%[^\n]\n", &onOff, sPath, sLabel);
            if (onOff)
            {
               ++appdirNumPaths;
               if ( ! strcmp(sPath,"USERDIR") )
                  strcpy(sPath, userdir);
#ifdef __INTERIX
               //winPathToUnix(sPath);
#endif
               if ( ! strcmp(sPath, systemdir) )
                  systemInList = 1;
               if (appdirPaths == NULL)
               {
                  appdirPaths = newStringId(sPath,"app");
                  appdirLabels = newStringId(sLabel,"app");
               }
               else
               {
                  appdirPaths = newCatId(appdirPaths,";","app");
                  appdirPaths = newCatId(appdirPaths,sPath,"app");
                  appdirLabels = newCatId(appdirLabels,";","app");
                  appdirLabels = newCatId(appdirLabels,sLabel,"app");
               }
            }
         }
      }
      fclose(path);
   }
   if ( ! systemInList )
   {
      char title[STR64];
      if (appdirPaths == NULL)
      {
         appdirPaths = newStringId(systemdir,"app");
         sprintf(title,"%s system",RevTitle);
         appdirLabels = newStringId(title,"app");
      }
      else
      {
         appdirPaths = newCatId(appdirPaths,";","app");
         appdirPaths = newCatId(appdirPaths,systemdir,"app");
         sprintf(title,";%s system",RevTitle);
         appdirLabels = newCatId(appdirLabels,title,"app");
      }
      ++appdirNumPaths;
   }
#endif
}

#ifndef PSG_LC
static char *appdirVal(char *info, int index)
{
   int  findIndex = 1;
   register char *infoPtr;
   static char label[MAXPATH];
   char *labelPtr;

   infoPtr = info;
   while ( (findIndex < index) && *infoPtr)
   {
      if ( *infoPtr == ';' )
      {
         ++findIndex;
      }
      ++infoPtr;
   }
   findIndex = 0;
   labelPtr = label;
   while ( *infoPtr && (*infoPtr != ';') )
   {
      *labelPtr++ = *infoPtr++;
   }
   *labelPtr = '\0';
   labelPtr = label;
   return(labelPtr);
}

#ifdef VNMRJ
static void sendAppdirs()
{
   int  findIndex = 0;
   char info[MAXPATH];
   char cmd[MAXPATH*2];

   if (appdirPaths == NULL)
      getappdirPaths();
   while (findIndex < appdirNumPaths)
   {
      findIndex++;
      sprintf(cmd,"vnmrjcmd appdir %d",findIndex);
      strcpy(info, appdirVal(appdirPaths,findIndex) );
#ifdef __INTERIX
      //unixPathToWin(info);
#endif
      strcat(info, ";" );
      strcat(info, appdirVal(appdirLabels,findIndex) );
      writelineToVnmrJ(cmd, info);
   }
   writelineToVnmrJ("vnmrjcmd", "rebuild");
   return;
}
#endif
#endif

/*  Search appdir defined directories for filename
    filename is the name of the file to look for
    lib is the directory within the appdirs to search for filename
    fullpath is the returned pathname, if the filename is found.
    If fullpath is NULL, the pathname is not returned
    suffix is a secondary search for filename. The search order is:
      1. filename is searched in the first appdir
      2. filename.suffix is searched in the first appdir
      3. filename is searched in the second appdir
      4. filename.suffix is searched in the second appdir.
      5. etc.
    If suffix is supplied as NULL or a null string, the suffix searches are
    skipped.
    perm is the permissions checked for accessing the file.
 */
int appdirFind(const char *filename, const char *lib, char *fullpath,
               const char *suffix, int perm)
{
   int  findIndex = 0;
   char delimiters[] = ";";
   char *ptr;
   char searchPath[MAXPATH];
   char currentPath[MAXPATH];

   if (appdirPaths == NULL)
      getappdirPaths();
   ptr = strwrd(appdirPaths, currentPath, MAXPATH, delimiters);
   while ( currentPath[0] != '\0' )
   {
      findIndex++;
      sprintf(searchPath,"%s/%s/%s",currentPath,lib,filename);
      if (!access( searchPath, perm))
      {
         if (fullpath != NULL)
            strcpy(fullpath,searchPath);
         return(findIndex);
      }
      if ( (suffix != NULL) && (suffix[0] != '\0') )
      {
         strcat(searchPath,suffix);
         if (!access( searchPath, perm))
         {
            if (fullpath != NULL)
               strcpy(fullpath,searchPath);
            return(findIndex);
         }
      }
      ptr = strwrd(ptr, currentPath, MAXPATH, delimiters);
   }
   return(0);
}

/*  Search appdir defined directories for access rights
    lib is the directory within the appdirs to check for access rights.
    fullpath is the returned pathname of the first directory with
    access rights.
    If fullpath is NULL, the pathname is not returned
    The directory is checked for read, write, execute permission.
 */
int appdirAccess(const char *lib, char *fullpath)
{
   int  findIndex = 0;
   char delimiters[] = ";";
   char *ptr;
   char searchPath[MAXPATH];
   char currentPath[MAXPATH];

   if (appdirPaths == NULL)
      getappdirPaths();
   ptr = strwrd(appdirPaths, currentPath, MAXPATH, delimiters);
   while ( currentPath[0] != '\0' )
   {
      findIndex++;
      sprintf(searchPath,"%s/%s",currentPath,lib);
      if (!access( searchPath, R_OK | X_OK | W_OK))
      {
         if (fullpath != NULL)
            strcpy(fullpath,searchPath);
         return(findIndex);
      }
      ptr = strwrd(ptr, currentPath, MAXPATH, delimiters);
   }
   return(0);
}

/*  Initialize appdirs. Called from bootup.c */
#ifndef PSG_LC
void setAppdirs()
{

   lastOperator[0] = '\0';
   getappdirPaths();
#ifdef VNMRJ
   /* This is the signal to VJ to reset its appdirs 
    * It will send a appdir('send') when it is ready to
    * receive the new appdirs
    */
   writelineToVnmrJ("vnmrjcmd appdir", "0");
#endif
}
#endif

char *getAppdirValue()
{
   return(appdirPaths);
}

/* This function is used by PSG.  It should not be used by Vnmr */
void setAppdirValue(char *path)
{
#ifdef PSG_LC
   appdirPaths = path;
#else
   (void) path;
#endif
}

#ifndef PSG_LC
int appdir(int argc, char *argv[], int retc, char *retv[])
{
   static char resetCmd[MAXPATH];
   static int sendCmd = 0;
   static int sendDirs = 1;

   if (argc > 1)
   {
      if ( ! strcmp(argv[1],"file"))
      {
         char filepath[MAXPATH];
         char operator[MAXPATH];

         /* Reset appdirs */
         lastOperator[0] = '\0';
         appdirRight = -1;
         getappdirPaths();
         filepath[0] = '\0';
         if (P_getstring(GLOBAL,"operator",operator,1,MAXPATH-1))
         {
            strcpy(operator,UserName);
         }
         if (appdirRight)
         {
            sprintf(filepath,"%s/persistence/appdir_%s", userdir, operator);
            if ( access(filepath,R_OK) )
            {
               filepath[0] = '\0';
            }
         }
         if ( (filepath[0] == '\0') && strcmp(operator,UserName) )
         {
            P_setstring(GLOBAL,"operator",UserName,1);
            appdirRight = rightsEval("caneditappdir");
            if (appdirRight)
            {
               sprintf(filepath,"%s/persistence/appdir_%s", userdir, UserName);
               if ( access(filepath,R_OK) )
               {
                  filepath[0] = '\0';
               }
            }
            P_setstring(GLOBAL,"operator",operator,1);
            appdirRight = rightsEval("caneditappdir");
         }
         if (filepath[0] == '\0')
         {
            getAppdirTemplate(filepath);
         }
         if (retc >= 1)
            retv[0] = newString(filepath);
         else
            Winfoprintf("application directory file is %s",filepath);
      }
      else if  ( ! strcmp(argv[1],"index"))
      {
         if (argc == 3)
         {
            int index;
            int res = 0;
            for (index=1; index <= appdirNumPaths; index++)
            {
               if ( ! strcmp(argv[2], appdirVal(appdirPaths,index) ) )
               {
                  res = index;
                  break;
               }
            }
            if (retc >= 1)
               retv[0] = intString(res);
            else if (res == 0)
               Winfoprintf("%s is not an appdir",argv[2]);
            else
               Winfoprintf("%s is appdir number %d of %d",argv[2],res,appdirNumPaths);
         }
         else
         {
            Werrprintf("Usage: %s('index',directory)", argv[0]);
            ABORT;
         }
      }
      else if  ( ! strcmp(argv[1],"info"))
      {
         if (argc == 2)
         {
            if (retc >= 1)
               retv[0] = intString(appdirNumPaths);
            else
               Winfoprintf("There are %d application directories",appdirNumPaths);
         }
         else 
         {
            int index = atoi(argv[2]);

            if ( (index <= 0) || (index > appdirNumPaths) )
            {
               Werrprintf("Application directory %s does not exist. Second argument must be between 1 and %d",
                           argv[2],appdirNumPaths);
               ABORT;
            }
            if (retc)
            {
               retv[0] = newString( appdirVal(appdirLabels,index) );
               if (retc >= 2)
               {
                  retv[1] = newString( appdirVal(appdirPaths,index) );
               }
            }
            else
            {
               Winfoprintf("Label for application directory %d is %s",
                           index, appdirVal(appdirLabels,index));
            }
         }
      }
#ifdef TODO
      else if  ( ! strcmp(argv[1],"infoall"))
      {
          /* Return onOff, path and label */
      }
#endif
      else if  ( ! strcmp(argv[1],"reset"))
      {
         char *saveAppdirPaths;

         lastOperator[0] = '\0';
         saveAppdirPaths = newStringId(appdirPaths,"apptmp");
         if (argc > 2)
         {
            /* With more than 1 argument, it is called from the login panel */
            P_setstring(GLOBAL,"operator",argv[2],1);
#ifdef VNMRJ
            {
              int num, index;
              double dval;
              char msg[MAXSTR];
              if (!P_getreal(GLOBAL, "jviewports", &dval, 1))
              {
                 num = (int) (dval+0.1);
                 for (index=1; index <= num; index++)
                 {
                    if (index != VnmrJViewId)
                    {
                       sprintf(msg,"VP %d setvalue('operator', '%s', 'global') vnmrjcmd('pnew','operator') appdir('update')\n",index,argv[2]);
                       writelineToVnmrJ("vnmrjcmd",msg);
                    }
                 }
              }
            }
            appendJvarlist("operator");
            // writelineToVnmrJ("pnew","1 operator");
#endif
            appdirRight = -1; /* check rights on next call to appdirFind */
            getappdirPaths();
            sprintf(resetCmd,"operatorlogin('%s','%s')\n",argv[3],argv[4]);
            sendCmd = 1;
         }
         else
         {
            /* With only 1 argument, it is called from the appdir editor */
            getappdirPaths();
         }
#ifdef VNMRJ
         if (strcmp(saveAppdirPaths, appdirPaths) )
         {
            /* Only update VJ panels if the appdirs has changed
             * Java will send an appdir('send') when it is ready
             * for the new appdir directories
             */
            writelineToVnmrJ("vnmrjcmd appdir", "0");
            sendDirs = 1;
         }
         if (sendCmd && ! sendDirs)
         {
            execString(resetCmd);
            sendCmd = 0;
         }
         {
            /* Update other viewports */
            int num, index;
            double dval;
            char msg[MAXSTR];
            if (!P_getreal(GLOBAL, "jviewports", &dval, 1))
            {
               num = (int) (dval+0.1);
               for (index=1; index <= num; index++)
               {
                  if (index != VnmrJViewId)
                  {
                     sprintf(msg,"VP %d appdir('update')\n",index);
                     writelineToVnmrJ("vnmrjcmd",msg);
                  }
               }
            }
         }
#endif
         releaseWithId("apptmp");
      }
#ifdef VNMRJ
      else if  ( ! strcmp(argv[1],"send"))
      {
         /* VJ has requested the new appdirs */
         if (sendDirs)
         {
            sendAppdirs();
            appendJvarlist("operator");
            sendDirs = 0;
         }
         // writelineToVnmrJ("pnew","1 operator");
         if (sendCmd)
         {
            execString(resetCmd);
            sendCmd = 0;
         }
      }
      else if  ( ! strcmp(argv[1],"update"))
      {
         /* get current appdirs */
         lastOperator[0] = '\0';
         getappdirPaths();
      }
#endif
   }
   RETURN;
}
#endif

/************************************************************************
      isDiskFullSize  determine if the disk is full based on size
      isDiskFullFile  determine if the disk is full based on a file

      Both program require a path which must exist.  Each uses this
      path to determine the correct file partition.  Both have a
      "default" test, used if the size is less than or equal to 0;
      or if the file cannot be accessed.  The default test reports
      the disk is full if 10 blocks or fewer are free.	
************************************************************************/

int isDiskFullSize(char *path, int size, int *resultadr )
{
	int		ival;
        unsigned int    usize;
	struct statvfs	freeblocks_buf;

	if (resultadr == NULL) {
		return( -1 );
	}

	ival = statvfs( path, &freeblocks_buf );
	if (ival != 0) {
		char	perror_msg[ MAXPATHL+16 ];

		strcpy( &perror_msg[ 0 ], "Problem with " );
		strcat( &perror_msg[ 0 ], path );
		perror( &perror_msg[ 0 ] );
		return( -1 );
	}

/*  If size is zero, we try to establish if the disk is full independently.
 *  If less than 10 blocks are available then the disk is full.
 */
        usize = (size == 0) ? 10 : size;
	if (freeblocks_buf.f_bavail <= usize)
	  *resultadr = 1;
	else
	  *resultadr = 0;

	return( 0 );
}

int isDiskFullFile(char *path, char *path_2, int *resultadr )
{
	int		ival;
	unsigned int	bs_factor;
	struct stat	stat_blk;
	struct statvfs	freeblocks_buf;

	if (resultadr == NULL) {
		return( -1 );
	}

	ival = statvfs( path, &freeblocks_buf );
	if (ival != 0) {
		char	perror_msg[ MAXPATHL+16 ];

		strcpy( &perror_msg[ 0 ], "Cannot access " );
		strcat( &perror_msg[ 0 ], path );
		perror( &perror_msg[ 0 ] );
		return( -1 );
	}

/*  If we can't get a stat block on the 2nd file, report the disk is full
    if less than 0.1% is free; that is, the number of blocks required is 0.1%
    of the number of blocks total in the partition.				*/

	ival = stat( path_2, &stat_blk );
	if (ival != 0) {
		stat_blk.st_blocks = freeblocks_buf.f_blocks / 1000;
		if (stat_blk.st_blocks < 10)
		  stat_blk.st_blocks = 10;
	}
        /* st_blocks are in size of 512 byte blocks
         * f_bavail are in size of f_bsize blocks
         * which is typically 4096.
         * bs_factor is f_bsize / 512
         * dividing st_blocks by bs_factor wil convert it into
         * number of blocks of f_bsize blocks, which can then
         * be compared to f_bavail
         */

        bs_factor = freeblocks_buf.f_bsize / 512;
        if (bs_factor < 1)
           bs_factor = 1;
	if (freeblocks_buf.f_bavail <= stat_blk.st_blocks / bs_factor)
	  *resultadr = 1;
	else
	  *resultadr = 0;

	return( 0 );
}


/************************************************************************/
/*   follow_link( start, final, flen )
 *
 *   start -  initial file or path
 *   final -  resulting link, or initial file/path if not a link
 *   flen  -  space available in final					*/
/************************************************************************/

int follow_link(char *start, char *final, int flen )
{
   int		 initial_is_relative, ival, link_is_absolute, slen;
   char		 linkpath[ PATH_MAX ], linkpath2[ PATH_MAX ];
   char		*qaddr;
   struct stat	 statblk;

   slen = strlen( start );
   if (slen < 1 || slen >= PATH_MAX) {
   	return( -1 );
   }

   strcpy( &linkpath[ 0 ], start );

/*  Keep following the symbolic link until we either come to a file
    which is not a symbolic link or an entry that does not exist.	*/

   for (;;) {
   	ival = lstat( &linkpath[ 0 ], &statblk );
   	if (ival != 0) {
   		if ( (errno == ENOENT) &&
                              ( (int) strlen( &linkpath[ 0 ] ) < flen) )
                {
			strcpy( final, &linkpath[ 0 ] );
			return( 0 );		/* normal return */
		}			/* for non-existant file */
		return( -1 );
	}

	if ((statblk.st_mode & S_IFMT) != S_IFLNK) {
		if ( (int) strlen( &linkpath[ 0 ] ) >= flen)
			  return( -1 );
		strcpy( final, &linkpath[ 0 ] );
			return( 0 );			/* normal return */
	}

	ival = readlink( &linkpath[ 0 ], &linkpath2[ 0 ], sizeof( linkpath2 ) - 1 );
	if (ival <= 0) {
		return( -1 );
	}
	else
	  linkpath2[ ival ] = '\0';	/* readlink program doesn't */
						/* put in a NUL character   */

	initial_is_relative = linkpath[ 0 ] != '/';
	link_is_absolute = linkpath2[ 0 ] == '/';
	if (initial_is_relative || link_is_absolute) {
		strcpy( &linkpath[ 0 ], &linkpath2[ 0 ] );
	}
	else {

	/*  initial is absolute (not relative) and link is relative
	    (not absolute).  Append the resulting link to the parent
	    directory of the initial path.				*/

		qaddr = strrchr( &linkpath[ 0 ], '/' );
		if (qaddr == NULL) {
			return( -1 );	/* should not happen */
		}			/* initial is not relative */
		qaddr++;
		strcpy( qaddr, &linkpath2[ 0 ] );

	   /* an enhancement would be to eliminate /a/../b constructs */
	}
   }

  /* Computer program never comes here ... */

}

/*  only s1 can have a wildcard character.
    unlike strcmp(), this function returns 1 if the
    two strings match and 0 if they do not match.	*/

int nscmp(char *s1, char *s2 )
{
	char		c1, c2;
	int		i1, i2, l1, l2;

/*  Preliminaries:  Establish length of each string.  If both strings are
    null, this counts as a match.  If the first string is null and the
    second is not, this is not a match.  If the second string is null and
    the first one is "*", this counts as a match; otherwise, a null second
    string cannot match.

    When all of this is complete, we know both strings are not null.	*/

	l1 = strlen( s1 );
	l2 = strlen( s2 );

	if (l1 <= 0 && l2 <= 0)
           return( 1 );
	if (l1 <= 0)
           return( 0 );
	if (l2 <= 0)
        {
	   if (strcmp( s1, "*" ) == 0)
              return( 1 );
	   else
              return( 0 );
        }

	i1 = 0;
	i2 = 0;
	do {
		c1 = *(s1+i1);
		c2 = *(s2+i2);

/*  Following check may be superfluous, but I feel better if it is done.  */

		if ( c1 == '\0' && c2 == '\0' ) return( 1 );

/*  Eliminate distinction, upper-case vs. lowercase.  */

		if ( c1 >= 'A' && c1 <= 'Z') c1 |= 0x20;
		if ( c2 >= 'A' && c2 <= 'Z') c2 |= 0x20;

/*  Check for 'placeholder' in first string.  */

		if (c1 == '%') {
			if (c2 == '\0') return( 0 );
			i1++;
			i2++;
		}

/*  Now check for wildcard in first string  */

		else if ( c1 == '*' ) {
			i1++;
			if (i1 >= l1) return( 1 );
			while ( l2-i2 > 0 )
			  if (nscmp( s1+i1, s2+i2 ))
			    return( 1 );
			  else
			    i2++;
			return( 0 );
		}
		else if (c1 != c2)
		 return( 0 );
		else {
			i1++;
			i2++;
		}
	} while (i1 <= l1 && i2 <= l2);

	return( 1 );
}
