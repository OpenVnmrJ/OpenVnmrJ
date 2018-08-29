/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Usage: auditcp <-f/l> <origpath> <destpath> */
/*	-f origpath is a file or dir to be copied. */ 
/*	-l origpath is a file containing a list of files(or dirs) to be copied. */ 
/*	default is -l*/
/*	default origpath is userdir/persistence/auditTableSelection */
/*	default destpath is userdir/p11/copies */
/*	if only one path is specified, it is taken as as destpath. */
/*	if no path is specified, both default paths are used. */ 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h> 
#include <unistd.h>
#include "md5global.h"
#include "md5.h"

#define MAXSTR  1024
#define MAXWORDS  32
#define BUFSIZE 1024
#define MAXFILE 50000 

typedef char string[MAXSTR];

static string auditTableSelection = "/persistence/auditTableSelection";
static string killprocess = "/persistence/killAuditProcess";

static int debug = 0;
static char UserName[40] = "";
static char* userdir;
static char* systemdir;

static void getStrValues(char* buf, string* words, int* n, char* delimiter);
static int get_username(char * username_addr, int username_len );
static void initGlobal();

int main (argc, argv)
int argc;
char *argv[];
{
    FILE *fp;
    string cmd, flag, orig, dest;
    int  nwords;
    char  buf[BUFSIZE];
    string words[MAXWORDS];
    string line, cmdLine, cmdTime;
    FILE *tmpfp;
    string tmpfile, killsh, str;
    int scriptWritten=0;

    initGlobal();
    getTimeString(cmdTime);

    /* killsh is a shell script to kill this process.  It will be executed 
       via the cancel button in the panel (Audit.java) */
    sprintf(killsh, "%s%s", userdir, killprocess);
    if(fp = fopen(killsh, "w")) {
        fprintf(fp, "kill -9 %d\n", getpid());
        if(debug)fprintf(stderr, "kill -9 %d\n", getpid());
        fclose(fp);

        sprintf(str, "%s%s", "chmod +x ", killsh);
        system(str);

        // Flag to know whether or not to try to remove this file
        scriptWritten = 1;
    }

    if(argc > 3 && argv[1][0] == '-') {
        strcpy(flag, argv[1]);
        strcpy(orig, argv[2]);
        strcpy(dest, argv[3]);
    } else if (argc > 2 && argv[1][0] == '-') {
        strcpy(flag, argv[1]);
    	sprintf(orig, "%s%s", userdir, auditTableSelection);
        strcpy(dest, argv[2]);
    } else if (argc > 2) {
        strcpy(flag, "-l");
        strcpy(orig, argv[1]);
        strcpy(dest, argv[2]);
    } else if (argc > 1 && argv[1][0] == '-') {
        strcpy(flag, argv[1]);
    	sprintf(orig, "%s%s", userdir, auditTableSelection);
        sprintf(dest, "%s%s", userdir, "/p11");
        if(!fileExist(dest)) mkdir(dest, 0777);
        sprintf(dest, "%s%s", userdir, "/p11/copies");
        if(!fileExist(dest)) mkdir(dest, 0777);
    } else if (argc > 1) {
        strcpy(flag, "-l");
    	sprintf(orig, "%s%s", userdir, auditTableSelection);
        if(strstr(argv[1], "/") == NULL) {
            sprintf(dest, "%s%s", userdir, "/p11");
            if(!fileExist(dest)) mkdir(dest, 0777);
            sprintf(dest, "%s%s", userdir, "/p11/copies");
            if(!fileExist(dest)) mkdir(dest, 0777);
            sprintf(dest, "%s%s%s", userdir, "/p11/copies/", argv[1]);
        } else strcpy(dest, argv[1]);
    } else {
        fprintf(stderr, "  Usage: auditcp <-f/l> <origpath> <destpath>\n");
        fprintf(stderr, "     -f: path is a file or directory. \n");
        fprintf(stderr, "     -l: path is a file containing files and/or directories. \n");
        fprintf(stderr, "         default is -l.\n");
        fprintf(stderr, "         default origpath is userdir/persistence/auditTableSelection.\n");
        fprintf(stderr, "         default destpath is userdir/p11/copies.\n");
        fprintf(stderr, "         if only one path is given, it is taken as destpath.\n");
        fprintf(stderr, "         \"auditcp -l\" is used by vnmrj auditing interface. \n");
        return(-1);
    }

    if(isAdirectory(orig)) strcpy(flag, "-f");

	if(debug) fprintf(stderr, "flag orig dest %s %s %s\n", flag, orig, dest);

    if(strcmp(flag, "-f") == 0) {

        sprintf(line, "%s %s %s %s %s to %s", cmdTime,
                "aaudit", UserName, "copied records", orig, dest);
        strcpy(cmd,"/vnmr/p11/bin/writeAaudit -l ");
        sprintf(cmdLine, "%s\"%s\"", cmd, line);
        system(cmdLine);

        sprintf(cmd, "cp -rf %s %s", orig, dest);
	if(fileExist(orig)) system(cmd);

	fprintf(stdout, "auditcp: %s %s\n","copied selected file(s) to ", dest);

    } else if((fp = fopen(orig, "r"))) {

    	tmpnam(tmpfile);
    	tmpfp = fopen(tmpfile, "w");

        while(fgets(buf,sizeof(buf),fp)) {

          if(buf[0] != '#') {
            nwords = MAXWORDS;
            getStrValues(buf, words, &nwords, " ");
            if(strcmp(words[0], "path") == 0) {

                sprintf(line, "%s %s %s %s %s to %s", cmdTime,
                        "aaudit", UserName, "copied records", words[1], dest);
                if(tmpfp != NULL) fprintf(tmpfp, "%s\n", line);
                sprintf(cmd, "cp -rf %s %s", words[1], dest);
		if(fileExist(words[1])) system(cmd);
            }
          }
        }
    	fclose(tmpfp);
        strcpy(cmd,"/vnmr/p11/bin/writeAaudit -f ");
        if(debug) fprintf(stderr, "%s%s\n", cmd, tmpfile);
        sprintf(cmdLine, "%s%s", cmd, tmpfile);
        system(cmdLine);

	fprintf(stdout, "auditcp: %s %s\n","copied selected file(s) to ", dest);

	unlink(tmpfile);
    }

    fclose(fp);

    if(scriptWritten) {
        /* Remove the script if it was written */
        sprintf(str, "%s%s", "rm -f ", killsh);
        system(str);
    }

    return(1);
}

/******************/
static void getStrValues(char* buf, string* words, int* n, char* delimiter)
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

int get_username(char * username_addr, int username_len )
{
        int              ulen;
        struct passwd   *getpwuid();
        struct passwd   *pasinfo;

        pasinfo = getpwuid( getuid() );
        if (pasinfo == NULL)
          return( -1 );
        ulen = strlen( pasinfo->pw_name );

        if (ulen >= username_len) {
                strncpy( username_addr, pasinfo->pw_name, username_len-1 );
                username_addr[ username_len-1 ] = '\0';
        }
        else
          strcpy(username_addr, pasinfo->pw_name);

    return(1);
}

static void initGlobal() {

    systemdir = (char*)getenv("vnmrsystem");
    userdir = (char*)getenv("vnmruser");

    get_username(UserName,sizeof(UserName));
}

