/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef part11_header_included
#define part11_header_included

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "vnmrsys.h"
#include "group.h"
#include "variables.h"
#include "tools.h"
#include "shims.h"
#include "md5global.h"
#include "md5.h"

#define MAXWORDS  32 
#define MAXFILES  10000 
#define BUFSIZE 1024

#define FALSE           0
#define TRUE            1

typedef char string[MAXSTR];

typedef struct {
    int fid;
    int procpar;
    int log;
    int text;
    int global;
    int conpar;
    int usermaclib;
    int shims;
    int waveforms;
    int data;
    int phasefile;
    int fdf;
    int snapshot;
    int cmdHistory;
    int pulseSequence;

} part11_standard_files;

typedef struct {
    int numOfFiles;
    string *fullpaths;

} part11_additional_files;

extern string bootuptime;
extern FILE    *auditTrailFp;
extern FILE    *cmdHistoryFp;
extern int part11System;
extern int part11Data;
extern string auditDir;
extern string part11RootPath;
extern string currentPart11Path;
extern part11_standard_files* part11_std_files;
extern part11_additional_files* part11_add_files;

void p11_writeCmdHistory(char* str);
int p11_init();
void p11_flush();

void displayPart11Config(FILE *fp);
int readPart11Config();
int safecp_file_verify(char* safecpDir, char* file_a, char* file_b );
void createSignature(char *path1, char *path2);
void getVersion(char* path, char* prefix, char* version);
int  copy_file(char* origpath, char* destpath);
int overwrite_file(char* filepath);
int processed();
int makeChecksums(char* rootpath,char *checksumFile);
void p11_createpars();
void p11_saveDisCmd();
void p11_setcurrfidid();
#endif
