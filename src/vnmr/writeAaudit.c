/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* write one line "...." at a time to auditDir/aaudit/time-begin.open.aaudit */
/* or append a file to auditDir/aaudit/time-begin.open.aaudit */
/* the owner of auditDir/aaudit/time-begin.open.aaudit needs to do */
/* chmod +s writeAaudit to allow any user write to this file. */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>


#define MAXSTR 1024
#define MAXFSIZE 50000 
#define PERMS 0644

typedef char string[MAXSTR];

int debug = 0;

int getAuditDir(string auditDir);
int getCurAauditFile(string auditDir, string path);

main(int argc, char *argv[])
{
    FILE* fp;
    FILE* infp;
    string buf, auditDir, aapath, aaname, path;
    string str, tstr, closedpath;

    if(argc < 3 || (strcmp(argv[1],"-l") != 0 && strcmp(argv[1],"-f") != 0) ) {
        fprintf(stderr, "  Usage: writeAaudit -l \"......\"\n");
        fprintf(stderr, "     or: writeAaudit -f filename\n");
        return(-1);
    } 

    getAuditDir(auditDir);
    if(!fileExist(auditDir)) 
	mkdir(auditDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 

    sprintf(aapath, "%s%s", auditDir, "/aaudit");

    if(!fileExist(aapath)) 
	mkdir(aapath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 

    getCurAauditFile(aapath, aaname);
    strcat(aapath, "/");

    sprintf(path, "%s%s", aapath, aaname);
    if(debug) fprintf(stderr, " path %s\n", path);

    fp = NULL;
    if(strlen(aaname) == 0 || !fileExist(path)) {
 	getTimeString(tstr);
	sprintf(path, "%s%s%s", aapath, tstr, ".open.aaudit"); 
    	fp = fopen(path,"w");
    } else if(fileExist(path) > MAXFSIZE) {
 	getTimeString(tstr);
	strcpy(str,"");
	strncat(str, path, strlen(path)-11);
	sprintf(closedpath, "%s%s", str, tstr); 
	file_copy(path, closedpath);
	unlink(path);

	sprintf(path, "%s%s%s", aapath, tstr, ".open.aaudit"); 
    	fp = fopen(path,"w");
    } else if(fileExist(path)) {
    	fp = fopen(path,"a");
    } 

    if (fp != NULL)
    {
      if (strcmp(argv[1],"-l") == 0) fprintf(fp,"%s\n", argv[2]);
      else if((infp = fopen(argv[2], "r"))) {

        while (fgets(buf,sizeof(buf),infp)) {
	    fprintf(fp,"%s",buf);
        }
    	fclose(infp);
      }

      fclose(fp);
    }
    return(1);
}

int getAuditDir(string auditDir) {
    FILE* fp;
    string str, path, buf;

    strcpy(path, "/vnmr/p11/part11Config");
    if((fp = fopen(path, "r"))) {

          while (fgets(buf,sizeof(buf),fp)) {

	    if(strStartsWith(buf, "auditDir") != 0) {
		sprintf(str, "%s", buf+9);
		break;
	    }
          }
    }

    strcpy(auditDir, "");
    strncat(auditDir, str, strlen(str)-1);
    if(debug) fprintf(stderr, " auditDir %s\n", auditDir);

    fclose(fp);
    return(0);
}

int getCurAauditFile(string path, string fname) {
    
    DIR             *dirp;
    struct dirent   *dp;
    string      child;

        if (dirp = opendir(path)) {
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

                if(debug)
                fprintf(stderr," writeAaudit %s\n", dp->d_name);
                if (*dp->d_name != '.') {

                    sprintf(child,"%s/%s",path,dp->d_name);
                    if (strEndsWith(dp->d_name, ".open.aaudit")) {
			strcpy(fname,dp->d_name);
            		closedir(dirp);
			return(1);
                    }
                }
            }
	    strcpy(fname,"");
            closedir(dirp);
            return 0;
        }
        else {
	    strcpy(fname,"");
            return 0;
        }
}
