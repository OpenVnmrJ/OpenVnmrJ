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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <grp.h>
#include "shrexpinfo.h"

#define MAXPATHL	256

extern void unlockit(const char *lockPath, const char *idPath);
extern int lockit(const char *lockPath, const char *idPath, time_t timeout);

void bill_started(SHR_EXP_INFO);
void bill_done(SHR_EXP_INFO);


#ifdef INCLUDE_MAIN

static char	vnmrsys[MAXPATHL];

void fill(int);
SHR_EXP_STRUCT	mapExpInfo;
int   gstage;


int main(int argc, char argv[])
{
int i;
   strcpy(vnmrsys, getenv("vnmrsystem") );
   i=0;
   while (i<1000)
   {
       printf("filling ... %d\n",i);
       fill(i);
       i++;

       sleep(2);
   }
}

void fill(int stage)
{
char   tmpStr[80],*tmpPtr;
time_t tmpT;
    tmpStr[0]=0;
    gstage = stage;
    switch (stage%3)
    {
    case 0:	// filled in by PSG
        mapExpInfo.Billing.submitTime = time(0);
        mapExpInfo.SampleLoc = 0;
        mapExpInfo.GoFlag    = stage%4;
        strcpy(mapExpInfo.Billing.Operator,"johnny");
        strcpy(mapExpInfo.Billing.account,"011-31-50");
        strcpy(mapExpInfo.Billing.seqfil,"s2pul");
        break;
    case 1:	// added at start time by expproc
        bill_started(&mapExpInfo);
        break;
    case 2:	// added and written in rmPsgFiles
        bill_done(&mapExpInfo);
        break;
   }
}
#endif

void bill_started(SHR_EXP_INFO  ExpInfo)
{
   ExpInfo->Billing.startTime = time(0);
   ExpInfo->Billing.wroteRecord = 0;
}

void bill_done(SHR_EXP_INFO  ExpInfo)
{
    const char root_end[]="</accounting>\n";
    char   tmpStr[80],*tmpPtr;
    char   inputPath[MAXPATHL*2];
    int    bufSize=4095;
    char   buf[bufSize+1];
    int    i,n;
    FILE   *inputFd=NULL;
    char    lockPath[MAXPATHL];
    char    idPath[MAXPATHL];
    char    idFile[MAXPATHL];
    char    accFile[MAXPATHL];
    time_t lockSecs = 2; /* default lock timeout */
    char    t_format[] = "%a %b %d %T %Z %Y";
    FILE    *xmlFd;
    time_t  secs;

    if (ExpInfo->Billing.wroteRecord != 0) return;

    strcpy(lockPath, getenv("vnmrsystem") );
    strcat(lockPath,"/adm/accounting/");
    strcpy(accFile, lockPath);
    strcat(accFile,"acctLog.xml");
    if ( access(accFile,F_OK) )
       return;

    ExpInfo->Billing.doneTime = time(0);

    /* Lock the Log file so other instances of vnmrj cannot write at the 
       same time. This is to lock the acctLog file. */
    strcpy(idPath, lockPath);
    strcat(lockPath, "acctLogLock");
    sprintf(idFile, "acctLogLockId_%d", getpid());
    strcat(idPath, idFile);
    lockit(lockPath, idPath, lockSecs);
    xmlFd = fopen(accFile,"r+");
    if (xmlFd == NULL) 
    {
       unlockit(lockPath,idPath);
       printf("Cannot (re-re)open '%s'\n",accFile);
       return;
    }

    /* find the closing /> at the end and put log info above that */
    fseek(xmlFd,-strlen(root_end),SEEK_END);

    fprintf(xmlFd,"<entry type=\"gorecord\" account=\"%s\" operator=\"%s\"\n",
            ExpInfo->Billing.account, ExpInfo->Billing.Operator);
    fprintf(xmlFd,"       goflag=\"%d\" loc=\"%d\" result=\"%d\"\n",
            ExpInfo->GoFlag, ExpInfo->SampleLoc, ExpInfo->ExpState);
    fprintf(xmlFd,"       seqfil=\"%s\"\n",ExpInfo->Billing.seqfil);

    i=0;
    n=sizeof(tmpStr);

//    strncpy(tmpStr,ctime(&(ExpInfo->Billing.submitTime)),n-1);
    secs = ExpInfo->Billing.submitTime;
    strftime(tmpStr,n-1, t_format, localtime(&secs));
    tmpPtr = tmpStr;		// remove <lf> from submitTime
    while ( *tmpPtr != '\0' && i<n)
    {  if ( *tmpPtr == '\n')
           *tmpPtr='\0';
       tmpPtr++;
       i++;
    }
    fprintf(xmlFd,"       submit=\"%s\"\n",tmpStr);

//    strncpy(tmpStr,ctime(&(ExpInfo->Billing.startTime)),n-1);
    secs = ExpInfo->Billing.startTime;
    strftime(tmpStr,n-1, t_format, localtime(&secs));
    tmpPtr = tmpStr;
    // remove <lf> from startTime
    i=0;
    while ( *tmpPtr != '\0' && i<n)
    {  if ( *tmpPtr == '\n')
           *tmpPtr='\0';
       tmpPtr++;
       i++;
    }
    fprintf(xmlFd,"       start=\"%s\"\n",tmpStr);
    fprintf(xmlFd,"       startsec=\"%d\"\n",ExpInfo->Billing.startTime);

//    strncpy(tmpStr,ctime( &(ExpInfo->Billing.doneTime)),n-1);
    secs = ExpInfo->Billing.doneTime;
    strftime(tmpStr,n-1, t_format, localtime(&secs));
    tmpPtr = tmpStr;		// remove <lf> from doneTime
    i=0;
    while ( *tmpPtr != '\0' && i<n )
    {  if ( *tmpPtr == '\n') *tmpPtr='\0';
       tmpPtr++;
       i++;
    }
    fprintf(xmlFd,"       end=\"%s\"\n",tmpStr);
    fprintf(xmlFd,"       endsec=\"%d\"\n",ExpInfo->Billing.doneTime);

    // Write optional params
    /* - Open file .../[go_id].loginfo */
    sprintf(inputPath, "/vnmr/acqqueue/%s.loginfo", ExpInfo->Billing.goID);
    inputFd = fopen(inputPath, "r");
    if(inputFd != NULL) {
      /* Get the optional param info */
      int size = fread(buf, 1, bufSize, inputFd);
      /* Terminate the buffer */
      buf[size] = 0;
      /* Write to the log file */
      fprintf(xmlFd,"%s",buf);
      fclose(inputFd);
    }

    fprintf(xmlFd,"/>\n");
    fprintf(xmlFd,"%s",root_end);
    fflush(xmlFd);
    fclose(xmlFd);
    unlink(inputPath);
    ExpInfo->Billing.wroteRecord = 1;
    unlockit(lockPath,idPath);
}

