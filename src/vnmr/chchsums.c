/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* -f checksum-file or a dir. */
/* -r dirpath, recursively search dirs of suffix .REC and */
/*              using checksum-file in subdirs acqfil and datdirNNN . */
/* -R a file containing a list of dirs, recursively search .RECs of each dir.*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include "md5global.h"
#include "md5.h"

#define MAXSTR	512 
#define MAXWORDS  32
#define BUFSIZE 1024

typedef char string[MAXSTR];

static string auditMenuSelection = "/persistence/auditMenuSelection";
static string auditTableSelection = "/persistence/auditTableSelection";
static string killprocess = "/persistence/killAuditProcess";

static int debug = 0;
static char UserName[40] = "";
static char* userdir;
static char* systemdir;

static void getStrValues(char* buf, string* words, int* n, char* delimiter);
static int get_username(char * username_addr, int username_len );
static void initGlobal();

int main(int argc, char *argv[])
{
    int bad = 0;
    FILE *fp;
    string path;
    int  nwords;
    char  buf[BUFSIZE], curDir[MAXSTR];
    string words[MAXWORDS];
    string flag, killsh;
    string rootpath;

    string str, cmd, line, cmdLine, cmdTime;
    FILE *tmpfp;
    FILE *outfp;
    string tmpfile, outpath;
    int scriptWritten=0;

    initGlobal();
    getTimeString(cmdTime);

    strcpy(cmd, "/vnmr/p11/bin/writeAaudit ");

    /* killsh is a shell script to kill this process.  It will be executed 
       via the cancel button in the panel (Audit.java) */
    sprintf(killsh, "%s%s", userdir, killprocess);
    if(fp = fopen(killsh, "w")) {
	fprintf(fp, "kill -9 %d\n", getpid());
        fclose(fp);

        sprintf(str, "%s%s", "chmod +x ", killsh);
        system(str);

        // Flag to know whether or not to try to remove this file
        scriptWritten = 1;
    }

    strcpy(outpath, "");
    if(argc > 3 && argv[1][0] == '-') {
	strcpy(flag, argv[1]);
	strcpy(path, argv[2]);
	strcpy(outpath, argv[3]);
    } else if(argc > 2 && argv[1][0] == '-') {
	strcpy(flag, argv[1]);
	if(strcmp(argv[1], "-R") == 0) {
	    sprintf(path, "%s%s", userdir, auditMenuSelection);
	    strcpy(outpath, argv[2]);
	} else strcpy(path, argv[2]);
    } else if(argc > 1 && argv[1][0] == '-') {
	strcpy(flag, "-R");
	sprintf(path, "%s%s", userdir, auditMenuSelection);
    } else if(argc > 1 && argv[1][0] != '-') {
	strcpy(flag, "-f");
	strcpy(path, argv[1]);
    } else {
        fprintf(stderr, "  Usage: chchsums <-f/r/R> <path> <outpath>\n");
        fprintf(stderr, "     -f: path is a checksum file (arbitrary name) or a dir with\n");
	fprintf(stderr, "         a checksum file of name 'checksum' containing filepaths\n");
	fprintf(stderr, "         and original checksums of the files.\n");
        fprintf(stderr, "     -r: path is a dir, recursively search subdirs of suffix .REC and \n"); 
        fprintf(stderr, "         matching checksum file in acqfil and datdirNNN.\n");
        fprintf(stderr, "     -R: path is a file containing a list of dirs, recursively search\n");
	fprintf(stderr, "         .RECs of each dir.\n");
        fprintf(stderr, "     -R: no path is specified, use userdir/persistence/auditMenuSelection\n");
	fprintf(stderr, "         as the list file.\n");
        fprintf(stderr, "         \"chchsums -R\" is used by vnmrj auditing interface.\n");
        fprintf(stderr, "default: -f \n");
        return(-1);
    }
		
    if(strlen(outpath) > 0) outfp = fopen(outpath, "w");
    else outfp = NULL;

    if(path[0] != '/' && getcwd(curDir, MAXSTR) != NULL) {
       strcpy(str,path);
       sprintf(path,"%s/%s",curDir,str);
    }

    	if(debug) fprintf(stderr, "flag, path %s %s\n", flag, path);
    if(strcmp(flag, "-f") == 0 && (strEndsWith(path,"/checksum") ||
                strEndsWith(path,"/checksum/"))) {
	/* path is checksum file, get rootpath (strip off checksum) */
	/* write message to stdout */
	strcpy(rootpath, "");
	strncat(rootpath,path,strlen(path)-9);
        bad = checkPart11Checksums0(rootpath, stdout);
	if(bad == -1) fprintf(stdout,"chchsums: checksum file %s does not exist.\n", path);
	else if(bad == 0) fprintf(stdout,"chchsums: checksum file %s successfully matched.\n", path);
	else fprintf(stdout,"chchsums: %d file(s) in %s corrupted.\n", bad, path);
    } else if(strcmp(flag, "-f") == 0 && !isAdirectory(path) ) {
        /* path is a check sum file, but is not named "checksum" */
        /* write message to stdout, time is empty */
        bad = checkChecksums("", path, stdout, "");
	if(bad == -1) fprintf(stdout,"chchsums: checksum file %s does not exist.\n", path);
        else if(bad == 0) fprintf(stdout,"chchsums: checksum file %s successfully matched.\n", path);
	else fprintf(stdout,"chchsums: %d file(s) in %s corrupted.\n", bad, path);
    } else if(strcmp(flag, "-f") == 0) {
	/* path is a dir (rootpath). assume there is a checksum in the dir */
        bad = checkPart11Checksums0(path, stdout);
	if(bad == -1) fprintf(stdout,"chchsums: checksum file %s%s does not exist.\n", path,"checksum");
	else if(bad == 0) 
	fprintf(stdout,"chchsums: checksum file %s%s successfully matched.\n", path, "checksum");
	else fprintf(stdout,"chchsums: %d file(s) in %s corrupted.\n", bad, path);
    } else if (strcmp(flag, "-r") == 0) {

        bad = checkPart11Checksums(path, outfp);

        fprintf(stdout, "chchsums: %s %s , %d %s\n", "check records", path, bad, "corrupted.");

        sprintf(line, "%s %s %s %s %s , %d %s", cmdTime,
                "aaudit", UserName, "check records", path, bad, "corrupted.");
        sprintf(cmdLine, "%s -l \"%s\"", cmd, line);
        system(cmdLine);

    } else if (strcmp(flag, "-R") == 0) {

    	tmpnam(tmpfile);
        tmpfp = fopen(tmpfile, "w");

        if((fp = fopen(path, "r"))) {
            while(fgets(buf,sizeof(buf),fp)) {

                if(buf[0] != '#') {
                    nwords = MAXWORDS;
                    getStrValues(buf, words, &nwords, " ");
                    if(strcmp(words[0], "path") == 0 && strstr(words[1], "/trash") == NULL) {

                        bad = checkPart11Checksums(words[1], outfp);

        		fprintf(stdout, "chchsums: %s %s , %d %s\n", 
				"check records", words[1], bad, "corrupted.");

                        sprintf(line, "%s %s %s %s %s , %d %s", cmdTime,
                                "aaudit", UserName, "check records", words[1], bad, "corrupted.");
                        if(tmpfp != NULL) fprintf(tmpfp, "%s\n", line);
                    }
                }
            }

            fflush(tmpfp);
            sprintf(cmdLine, "%s -f %s", cmd, tmpfile);
        if(debug) fprintf(stderr, "%s -f %s\n", cmd, tmpfile);
            system(cmdLine);

        } 
        fclose(fp);
	fclose(tmpfp);
	unlink(tmpfile);

    }

    if(outfp != NULL) fclose(outfp);

    if(scriptWritten) {
        /* Remove the script if it was written */
        sprintf(str, "%s%s", "rm -f ", killsh);
        system(str);
    }


    return (bad);
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

