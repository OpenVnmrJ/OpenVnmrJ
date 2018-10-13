/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* write one line "...." at a time to /vnmrp11/trash/username/time-begin.open.trash */
/* or append a file to /vnmrp11/trash/username/time-begin.open.trash */
/* the owner of /vnmrp11/trash/ needs to do */
/* chmod +s writeTrash to allow any user write to this file. */

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

int debug = 1;

int getTrashDir(string trashDir);
int getCurTrashFile(string trashDir, string path);

int main(int argc, char *argv[])
{
    FILE* fp;
    FILE* infp;
    string buf, trashDir, trashpath, trashname, path;
    string str, tstr, closedpath;

    if(argc < 4 || (strcmp(argv[1],"-l") != 0 && strcmp(argv[1],"-f") != 0) ) {
        fprintf(stderr, "  Usage: writeTrash -l \"......\" username\n");
        fprintf(stderr, "     or: writeTrash -f filename username\n");
        return(-1);
    } 

    getTrashDir(trashDir);
    if(!fileExist(trashDir)) 
	mkdir(trashDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 

    sprintf(trashpath, "%s/%s", trashDir, argv[3]);

    if(!fileExist(trashpath)) 
	mkdir(trashpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); 

    getCurTrashFile(trashpath, trashname);
    strcat(trashpath, "/");

    sprintf(path, "%s%s", trashpath, trashname);
    if(debug) fprintf(stderr, " path %s\n", path);

    fp = NULL;
    if(strlen(trashname) == 0 || !fileExist(path)) {
 	getTimeString(tstr);
	sprintf(path, "%s%s%s", trashpath, tstr, ".open.trash"); 
    	fp = fopen(path,"w");
    } else if(fileExist(path) > MAXFSIZE) {
 	getTimeString(tstr);
	strcpy(str,"");
	strncat(str, path, strlen(path)-11);
	sprintf(closedpath, "%s%s", str, tstr); 
	file_copy(path, closedpath);
	unlink(path);

	sprintf(path, "%s%s%s", trashpath, tstr, ".open.trash"); 
    	fp = fopen(path,"w");
    } else if(fileExist(path)) {
    	fp = fopen(path,"a");
    } 

    if (fp != NULL)
    {
       if(strcmp(argv[1],"-l") == 0) fprintf(fp,"%s\n", argv[2]);
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

int getTrashDir(string trashDir) {
    FILE* fp;
    string str, path, buf;

    strcpy(path, "/vnmr/p11/part11Config");
    if((fp = fopen(path, "r"))) {

          while (fgets(buf,sizeof(buf),fp)) {

	    if(strStartsWith(buf, "part11Dir") != 0) {
		sprintf(str, "%s", buf+10);
		break;
	    }
          }
    }

    strcpy(trashDir, "");
    strncat(trashDir, str, strlen(str)-1);
    strcat(trashDir, "/trash");
    if(debug) fprintf(stderr, " trashDir %s\n", trashDir);

    fclose(fp);
    return(0);
}

int getCurTrashFile(string path, string fname) {
    
    DIR             *dirp;
    struct dirent   *dp;
    string      child;

        if (dirp = opendir(path)) {
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

                if(debug)
                fprintf(stderr," writeTrash %s\n", dp->d_name);
                if (*dp->d_name != '.') {

                    sprintf(child,"%s/%s",path,dp->d_name);
                    if (strEndsWith(dp->d_name, ".open.trash")) {
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
