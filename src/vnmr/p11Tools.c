/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#ifndef P11
#include "vnmrsys.h"

#undef MAXSTR
#endif
#define MAXSTR 1024
#define MAXWORDS 64
#define BUFSIZE 1024
#define PERMS 0644

typedef char string[MAXSTR];

static int debug = 0;

static string sysOwnedSafecp = "/vnmr/p11/bin/safecp";
static string usrOwnedSafecp = "/vnmr/bin/safecp";

#ifndef P11
extern void currentDate(char* cstr, int len );
#else
static char* systemdir;
static char* userdir;
#endif

static void initGlobal() {

#ifdef P11
    systemdir = getenv("vnmrsystem");
    userdir = getenv("vnmruser");
#endif

    if(systemdir != NULL) {
	strcpy(sysOwnedSafecp, systemdir);
	strcat(sysOwnedSafecp, "/p11/bin/safecp");
	strcpy(usrOwnedSafecp, systemdir);
	strcat(usrOwnedSafecp, "/bin/safecp");
    }

}

/******************/
void getStrValues(char* buf, string* words, int* n, char* delimiter)
/******************/
{
/* break a string "buf" into "words" separated by delimiter */
/* return both words and number of words n. */

    int size;
    char *strptr, *tokptr;
    char  str[BUFSIZE];

    size = *n;

    /* remove newline if exists */
    if(buf[strlen(buf)-1] == '\n') {
        strcpy(str, "");
        strncat(str, buf, strlen(buf)-1);
    } else
       strcpy(str, buf);

    strptr = str;
    *n = 0;
    while ((tokptr = (char*) strtok(strptr, delimiter)) != (char *) 0) {

        if(strlen(tokptr) > 0) {
            strcpy(words[*n], tokptr);
            (*n)++;
        }
        strptr = (char *) 0;
    }
}

#ifdef P11
/******************/
static void currentDate(char* cstr, int len )
/******************/
{
  char tmpstr[MAXSTR];
  struct timeval clock;
  static char t_format[] = "%\Y%\m%\dT%\H%\M%\S";

  gettimeofday(&clock, NULL);
  strftime(tmpstr, MAXSTR, t_format, localtime((long*)&(clock.tv_sec)));

  strncpy(cstr, tmpstr, len);
}
#endif


/******************/
void getTimeString(char* timestr)
/******************/
{
/* time string is formatted as D2002-01-22T16:14:38 (for example)*/

    char    ptmp[MAXSTR];

    currentDate(ptmp, MAXSTR);

    strcpy(timestr, "");
    if(strlen(ptmp) == 15)
    sprintf(timestr,"D%.4s-%.2s-%.2s%.1s%.2s:%.2s:%.2s",
        &ptmp[0], &ptmp[4], &ptmp[6], &ptmp[8], &ptmp[9], &ptmp[11], &ptmp[13]);
}

/**************************/
int setSafecpPath(char* safecpPath, char* dest)
/**************************/
{
/* call this before using safecp_file */
/* use usrOwnedSafecp only if user owns dest.*/

    string str, words[MAXWORDS];
    string destRoot;
    int i, j, nwords, ival1, ival2;
    struct stat     stat_dest, stat_user;
    FILE* fp;

    initGlobal();

    ival2 = stat( userdir, &stat_user );
    if(ival2 != 0) {
        sprintf(safecpPath, "%s", sysOwnedSafecp);
        return(0);
    }

    ival1 = 1;
    strcpy(destRoot, dest);
    nwords = MAXWORDS;
    getStrValues(destRoot, words, &nwords, "/");

    for(i=nwords; i>0; i--) {
        strcpy(str,"/");
        for(j=0; j<i; j++) {
          strcat(str, words[j]);
          strcat(str, "/");
        }

        if(debug)
        fprintf(stderr,"///safecpPath str %s\n", str);

        ival1 = stat( str, &stat_dest );
        if(ival1 == 0) break;
    }
    if(ival1 != 0) {
        sprintf(safecpPath, "%s", sysOwnedSafecp);
        return(0);
    }

    strcat(str,"/testSafecpPath");
    if(!(fp = fopen(str, "w"))) {
        sprintf(safecpPath, "%s", sysOwnedSafecp);
    } else {
        sprintf(safecpPath, "%s", usrOwnedSafecp);
    }
    if(fp)
        fclose(fp);

    unlink(str);

        if(debug)
        fprintf(stderr,"///safecpPath %s\n", safecpPath);

    return(1);
}

/**************************/
int cp_file(char* path1, char* path2)
/**************************/
{
    char cmd[MAXSTR];

    sprintf(cmd, "cp -rf %s %s", path1, path2);
    return(system(cmd));
}

/**************************/
int rm_file(char* path)
/**************************/
{
    char cmd[MAXSTR];

    sprintf(cmd, "rm -rf %s", path);
    return(system(cmd));
}

/**************************/
int fileExist(char* path)
/**************************/
{
        int ival;
        struct stat     stat_blk;

        errno = 0;
        ival = stat( path, &stat_blk );
/* ival = 0 if stat is successful, ival = -1 if failed */
/* do not use ival to determine whether file exists, */
/* because stat fail when file exists but size overflow, */
/* search permission is denied... */

/* if the file does nit exist, errno will be set to ENOENT */
/*
        fprintf(stderr,"errno %d\n", errno);
        fprintf(stderr," ENOENT %d\n", ENOENT);
        fprintf(stderr," ival %d %s\n", ival, path);
*/
        if(errno == ENOENT) return(0);
        else if(stat_blk.st_size > 0) return(stat_blk.st_size);
	else return 1;
}

int isAdirectory(char* filename)
{
        int             ival, retval;
        struct stat     buf;

        ival = stat(filename,&buf);
        if (ival != 0)
          retval = 0;
        else
          if (buf.st_mode & S_IFDIR)
            retval = 1;
          else
            retval = 0;

        return( retval );
}

/******************/
int lastSlash(char* path)
/******************/
/* if ends with '/', get second last */
{
    int i, last;

    last = strlen(path);
    if(path[strlen(path)-1] == '/') last -= 1;
    for(i=last; i>0; i--) {
        if(path[i-1] == '/') return(i);
    }
    return(-1);
}

/**************************/
int strStartsWith(char *s1, char *s2)
/**************************/
/* return whether s1 ends with s2, 0 no, 1 yes. */
{
   int l1, l2;
   l1 = strlen(s1);
   l2 = strlen(s2);

   if(l2 > l1) return(0);

   if(strncmp(s1, s2, l2) == 0) return(1);
   else return(0);
}

/**************************/
int strEndsWith(char *s1, char *s2)
/**************************/
/* return whether s1 ends with s2, 0 no, 1 yes. */
{
   char *str;
   int l1, l2;
   l1 = strlen(s1);
   l2 = strlen(s2);

   if(l2 > l1) return(0);

   str = s1;
   str += (l1 - l2);

   if(strcmp(s2, str) == 0) return(1);
   else return(0);
}

/**************************/
int strEndsWith_once(char *s1, char *s2)
/**************************/
/* return whether s1 ends with s2 and occurs only once, 0 no, 1 yes. */
{
   int l1, l2;
   l1 = strlen(s1);
   l2 = strlen(s2);

   if(l2 > l1) return(0);

   if(strstr(s1, s2) != NULL &&
        ((int)strcspn(s1, s2) + l2) == l1) return(1);
   else return(0);
}

int file_copy(string origpath, string destpath) {
    int f1, f2, n;
    char buf[BUFSIZE];

    if( (f1 = open(origpath, O_RDONLY, 0)) == -1) {
        fprintf(stderr, "writeAaudit: cannot open %s.\n", origpath);
        return(0);
    }

    if( (f2 = creat(destpath, PERMS)) == -1) {
        fprintf(stderr, "writeAaudit: cannot create %s.\n", destpath);
	close(f1);
        return(0);
    }

    while((n = read(f1, buf, BUFSIZE)) > 0)
        if(write(f2, buf, n) != n) {
            fprintf(stderr, "writeAaudit: write error on file %s.\n", destpath);
            return(0);
        }

    close(f1);
    close(f2);
    return(1);
}

/******************/
void getFilenames(string rootpath, string name, string* files, int* n, int nmax)
/******************/
{
    DIR             *dirp;
    struct dirent   *dp;
    string dir, child;

    if(rootpath[strlen(rootpath)-1] != '/')
       sprintf(dir, "%s/%s", rootpath, name);
    else
       sprintf(dir, "%s%s", rootpath, name);

 	if(debug)
	fprintf(stderr, "getFilenames %s\n", dir);

    if(strlen(dir) == 0 || !fileExist(dir)) return;

    if((*n) >= (nmax-1)) {
	return;
    } else if(!isAdirectory(dir)) {
        strcpy(files[*n], name);
        (*n)++;
    } else {
        if ( (dirp = opendir(dir)) ) {
            if(strlen(name) > 0 && name[strlen(name)-1] != '/')
                strcat(name,"/");
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

                if(debug)
                fprintf(stderr," dir %s %s\n", dir, dp->d_name);
                if (*dp->d_name != '.') {

                    sprintf(child,"%s%s",name,dp->d_name);
                    getFilenames(rootpath, child, files, n, nmax);
                }
            }

            closedir(dirp);
        }
    }
    return;
}

/******************/
int brkPath(char* path, char* root, char* name)
/******************/
/* success return 1, failed return 0 */
{
    int i;

    i = lastSlash(path);
    if(i != -1) {
        strcpy(root, "");
        strncat(root, path, i);
        strcpy(name, path+i);
        return(1);
    } else {
        strcpy(root, "");
        strcpy(name, path);
        return(0);
    }
}

