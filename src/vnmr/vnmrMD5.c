/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "md5global.h"
#include "md5.h"
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define MAXSTR	512 
#define MAXFILES	10000 
#define MAXWORDS  32
#define BUFSIZE 1024

typedef char string[MAXSTR];

/* checksum of a directory is a "group checksum" in a file named "checksum" */
/* in that directory, with the format: */
/*                  			*/
/* rootpath:dirName */
/* n_files:#   (files are recursively searched from root dir) */  
/* sum:filename:checksum */
/*                       */
/* checksum file cannot be modified once genrated by this program */
/* the checksum of a file in the given dir depends not only on that file. */
/*			*/
/* usage: vnmrMD5 <-f/l> filename <rootpath>	*/ 
/* -f filename, if a file, print checksum to stdout, */
/*		if a dir, create group checksum file (or stdout if not permitted) */
/* -l filename, file is a list of files/dirs whose group checksum will be determined */
/* and printed to stdout */
/* default is -f	*/  

void getFilenames(string rootpath, string name, string* files, int* n, int nmax);
void writeChecksum(FILE* fp, char* rootpath, char* file, int nfiles);

int debug = 0;

int main (argc, argv)
int argc;
char *argv[];
{
    FILE *fp;
    string path, str, curDir, rootpath;
    int i, nfiles;
    char  buf[BUFSIZE];
    string files[MAXFILES];
    int list = 0; /* 0 is f, 1 is l */

    if(getcwd(curDir, MAXSTR) == NULL)
	strcpy(curDir,"");

    if(argc > 3 && strcmp(argv[1],"-l") == 0 ) {
	list = 1;
	strcpy(path, argv[2]);
	strcpy(rootpath, argv[3]);
    } else if(argc > 3) {
	list = 0;
	strcpy(path, argv[2]);
	strcpy(rootpath, argv[3]);
    } else if(argc > 2) {
	if(strcmp(argv[1],"-l") == 0 && isAdirectory(argv[2])) {
	    fprintf(stderr,"vnmrMD5 error: %s should be a checksum file.\n", argv[2]); 
	    return(-1);
	} else if(strcmp(argv[1],"-l") == 0) list = 1;
	strcpy(path, argv[2]);
	strcpy(rootpath, curDir);
    } else if (argc > 1) {
	strcpy(path, argv[1]);
	strcpy(rootpath, curDir);
    } else {
	fprintf(stderr, "  Usage: vnmrMD5 <-f/l> path <rootpath> \n");
	fprintf(stderr, "     -f: path is a file or dir \n");
	fprintf(stderr, "     -l: path is a file contains a list of files \n");
	fprintf(stderr, "default: -f \n");
	return(-1);
    }

    if(path[0] != '/') {
	strcpy(str, path);
	sprintf(path,"%s/%s", curDir,str);
    } 

    if(debug)
	fprintf(stderr,"input path %s\n", path);
    	
    if (list) {
	
        if((fp = fopen(path, "r"))) {
	   nfiles = 0;
           while(fgets(buf,sizeof(buf),fp)) {

                if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
		    strcpy(files[nfiles], "");
		    strncat(files[nfiles], buf, strlen(buf)-1);
		    nfiles++;
		}
	   }
	   fclose(fp);

           if(argc > 4) sprintf(str, "%s", argv[4]);
	   else strcpy(str,"");
    	   if(nfiles > 0 && argc > 4 && (fp = fopen(str, "w"))) {
	    fprintf(fp, "rootpath:%s\n", rootpath);
	    fprintf(fp, "n_files:%d\n",nfiles);
    	    for(i=0; i<nfiles; i++) {
               writeChecksum(fp, "", files[i], nfiles);
    	    }
	    fclose(fp);
	   } else if(nfiles > 0) {
	    fprintf(stdout, "rootpath:%s\n", rootpath);
	    fprintf(stdout, "n_files:%d\n",nfiles);
    	    for(i=0; i<nfiles; i++) {
               writeChecksum(stdout, "", files[i], nfiles);
    	    }
	   }
	}

    } else if(!isAdirectory(path)) {
        if(argc > 4) sprintf(str, "%s", argv[4]);
	else strcpy(str,"");
    	if(argc > 4 && (fp = fopen(str, "w"))) {
	    fprintf(fp, "rootpath:%s\n",rootpath);
	    fprintf(fp, "n_files:%d\n",1);
	    writeChecksum(fp, "", path, 1);
	    fclose(fp);
	} else {
	    fprintf(stdout, "rootpath:%s\n",rootpath);
	    fprintf(stdout, "n_files:%d\n",1);
	    writeChecksum(stdout, "", path, 1);
	}
    } else {

	if(path[strlen(path)-1] != '/') strcat(path, "/");
        if(argc > 4) sprintf(str, "%s", argv[4]);
	else strcpy(str,"");

	nfiles = 0;
	getFilenames(path, "", files, &nfiles, MAXFILES);

    	if(argc > 4 && (fp = fopen(str, "w"))) {
	    fprintf(fp, "rootpath:%s\n",path);
	    fprintf(fp, "n_files:%d\n",nfiles);

    	    for(i=0; i<nfiles; i++) {
        	writeChecksum(fp, path, files[i], nfiles);
    	    }
	    fclose(fp);
	} else {

	    fprintf(stdout, "rootpath:%s\n",path);
            fprintf(stdout, "n_files:%d\n",nfiles);

            for(i=0; i<nfiles; i++) {
                writeChecksum(stdout, path, files[i], nfiles);
            }
	}
    } 
    return(0);
}

/**************************/
void writeChecksum(FILE* fp, char* rootpath, char* file, int nfiles)
/**************************/
{
    int last;
    char str[MAXSTR], sum[MAXSTR];
    string filename;

    sprintf(filename, "%s%s", rootpath, file);
    if(debug)fprintf(stderr, "writeChecksum %s\n", filename);
    if(getChecksum(filename, sum)) {

        if(isdigit(sum[strlen(sum)-1])) {
          sprintf(str,"%c",sum[strlen(sum)-1]);
          last = atoi(str);
        } else last = 0;

        sprintf(str, "%d", last+nfiles);
        strcat(sum, str);
        fprintf(fp, "sum:%s:%s\n",file, sum);
    }
}

