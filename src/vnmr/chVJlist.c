/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAXSTR 1024
#define MAXFILES 10000 

typedef char string[MAXSTR];
int debug = 0;
string flag ;   /* -l generate a list, -v validate the list, -x validate and report extra files*/

void validFileList(string root, string name, string* files, int nfiles, int* nextra);

main(int argc, char *argv[])
{

    FILE* fp;
    FILE* fptmp;
    int i, nfiles;
    string files[MAXFILES];
    string str, path, listfile, buf;
    int nextra, nmiss;

    if(argc > 3 && argv[1][0] == '-') {
	strcpy(flag, argv[1]); 
	strcpy(path, argv[2]);
	strcpy(listfile, argv[3]);
    } else if(argc > 2 && strcmp(argv[1], "-l") == 0) {
	strcpy(flag, argv[1]); 
	strcpy(path, argv[2]);
	strcpy(listfile, "");
    } else if(argc > 2) {
	strcpy(flag, argv[1]); 
	strcpy(listfile, argv[2]);
	strcpy(path, "/vnmr");
    } else if(argc > 1 && strcmp(argv[1], "-l") == 0) {
	strcpy(flag, argv[1]); 
	strcpy(path, "/vnmr");
	strcpy(listfile, "");
    } else if(argc > 1 && argv[1][0] != '-') {
	strcpy(flag, "-l");
	strcpy(path, argv[1]); 
	strcpy(listfile, "");
    } else if(argc > 1) {
	strcpy(flag, argv[1]); 
	strcpy(path, "/vnmr");
	strcpy(listfile, "/vnmr/adm/p11/sysListAll");
    } else {
        fprintf(stderr, "  Usage: chVJlist <-l/v> <path> <listfile> <-x>\n");
        fprintf(stderr, "     -l: make a list of (max 9999) files of given path,\n");
        fprintf(stderr, "         and write the list to listfile if specified.\n");
        fprintf(stderr, "     -v: validate (max 9999) files in given path from given listfile.\n");
        fprintf(stderr, "     -x: validate. report extra files when validating.\n");
        fprintf(stderr, "defualt: -l /vnmr /vnmr/adm/p11/sysListAll.\n");

	return(-1);
    }

    if(debug)
	fprintf(stderr, "%s %s %s\n", flag, path, listfile);

    if(path[strlen(path)-1] != '/') strcat(path,"/");

    if(strcmp(flag, "-l") == 0) {

        nfiles = 0;
        /* getFilenames() adds a '/' to path, beware when testing below */
        getFilenames(path, "", files, &nfiles, MAXFILES); 

        if(strlen(listfile) == 0 || !(fp = fopen(listfile, "w")))
            fp = stdout;

        /* Allow /vnmr with or without a trailing '/' */
        /* This section is to skip directories that change in normal use */
        if(strncmp(path, "/vnmr", 5) == 0) {
            for(i=0; i<nfiles; i++) {
                if( (strstr(files[i], "adm/users/profiles/") == NULL) &&
                    (strstr(files[i], "adm/patch/") == NULL) &&
                    (strstr(files[i], "acqqueue/") == NULL) &&
                    (strstr(files[i], "tmp/") == NULL) &&
                    (strstr(files[i], "pgsql/data/") == NULL) &&
                    (strstr(files[i], "pgsql/persistence/") == NULL) )
                 {
                    char tmpFile[1024];
                    sprintf(tmpFile,"%s%s",path,files[i]);
                    if ( strcmp(tmpFile,"/vnmr/bin/convert") )
                       fprintf(fp, "%s%s\n", path, files[i]);
                 }
            }
        } 
        else {
            for(i=0; i<nfiles; i++) {
                sprintf(str, "%s%s", path, files[i]);
                if((fptmp = fopen(str, "r")) != NULL) {
                    fprintf(fp, "%s\n", str);
                    fclose(fptmp);
                }
            }
        }

        fclose(fp);
    } 
    else if(strcmp(flag, "-v") == 0 && strlen(listfile) > 0 && (fp = fopen(listfile, "r"))){
 	struct stat fstat;	
        while (fgets(buf,sizeof(buf),fp) && i < MAXFILES) {
            if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
		buf[strlen(buf)-1]='\0';
		if (stat(buf, &fstat) != 0)	
			fprintf(stdout, "%s is missing.\n",buf); 
            }
        }
        fclose(fp);
    } 
    else if(strlen(listfile) > 0 && (fp = fopen(listfile, "r"))){

        i = 0;
        while (fgets(buf,sizeof(buf),fp) && i < MAXFILES) {
            if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
                strcpy(files[i], "");
                strncat(files[i], buf, strlen(buf)-1);
                i++;
            }
        }
 
        fclose(fp);

        nfiles = i;
        nextra = 0;
        validFileList(path, "", files, nfiles, &nextra); 

        nmiss = 0;
        for(i=0; i<nfiles; i++) {
	    if(strlen(files[i]) > 0) { 
		fprintf(stdout, "%s is missing.\n", files[i]);
		nmiss++;
	    }
        }

	if(debug) fprintf(stderr, "extra, miss %d %d\n", nextra, nmiss); 
        if(nextra == 0 && nmiss == 0) 
            fprintf(stdout, "chVJlist: all files in %s matched.\n", path);
    }

    return(0);
}

void validFileList(string rootpath, string name, string* files, int nfiles, int* nextra) {

    DIR             *dirp;
    struct dirent   *dp;
    string dir, child;
    int i, ind;

    if(rootpath[strlen(rootpath)-1] != '/') strcat(rootpath,"/");

    sprintf(dir, "%s%s", rootpath, name);

    if(strlen(dir) == 0 || !fileExist(dir)) return;

    if(!isAdirectory(dir)) {
	
	ind = -1;
	for(i=0; i<nfiles; i++) {
	    if(strlen(files[i]) > 0 && strcmp(files[i], dir) == 0) {
		strcpy(files[i], "");
		ind = i;
	    	break;
	    } 
	}

        if(strcmp(flag, "-x") == 0 && ind == -1) {
	    fprintf(stdout, "%s is not in the list.\n", dir);
	    (*nextra)++;
	}

    } else {
        if (dirp = opendir(dir)) {
            if(strlen(name) > 0 && name[strlen(name)-1] != '/')
                strcat(name,"/");
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

                if(debug)
                    fprintf(stderr," dir %s %s\n", dir, dp->d_name);
                if (*dp->d_name != '.') {

                    sprintf(child,"%s%s",name,dp->d_name);
                    validFileList(rootpath, child, files, nfiles, nextra);
                }
            }

            closedir(dirp);
        }
    }
    return;	
}
