/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "errLogLib.h"
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "Monitor_Cmd.h"

extern void initiatePubSub();
extern NDDS_ID initiateNDDS(int);
extern int wait4Master2Connect(int timeout);
extern int send2Monitor(int, int, int,int, char *, int);
extern void MonitorReply(int *, int *, int *, int *, char *);
extern void DestroyDomain();
extern int initConfSub();

int onestep();
int twostep(int deferedFlag);
int rebootem();
int deleteFF();
int readMd5(char *filename, char *md5signature);
int getFileSize(char *filename);

char ProcName[80];
int multicast = 1;  /* enable mulsticasting for NDDS */
int ndds_debuglevel = 1;

char *ConsoleHostName = "wormhole";

char *ConsoleNicHostname = "wormhole";

char *cntlrNames[11] = { "master1", "rf1", "rf2", "rf3", "rf4", "rf5", "pfg1", "lpfg1", "grad1", "lock1", "ddr1" };


int main(void)
{
    int cmd;
    int ret __attribute__((unused));
    strcpy(ProcName,"testflash");
    initiateNDDS(0);   /* NDDS_Domain is set */
    initiatePubSub();  /* monitor command pub/sub */

    if ( wait4Master2Connect(10) == -1)  /* wait for 10 sec for Master to connect */
    {
       fprintf(stderr,"Master Failed to connect to publication, aborting.\n");
    }
    while(1)
    {
       fprintf(stdout,"Menu:\n");
       fprintf(stdout,"0 - Exit\n");
       fprintf(stdout,"1 - Two Steps Interactive\n");
       fprintf(stdout,"2 - One step Interactive\n");
       fprintf(stdout,"3 - Reboot controllers\n");
       fprintf(stdout,"4 - Delete From Flash \n");
       fprintf(stdout,"5 - Deferred Update \n");
       fprintf(stdout,"Selection: ");
       ret = fscanf(stdin,"%d",&cmd);
       switch(cmd)
       {
          case 1:
		  twostep(0);
		 break;
          case 2:  
                 onestep();
		 break;

          case 3:  
                 rebootem();
		 break;
          case 4:  
                 deleteFF();
		 break;
          case 5:  
                 twostep(1);
		 break;
          case 0:
          default:
        DestroyDomain();  // make sure to notify console this pub is going away
		  exit(0);
       }
     }
     DestroyDomain();  // make sure to notify console this pub is going away
     exit(0);
}

int rebootem()
{
  send2Monitor(ABORTALLACQS, 0, 0, 0, NULL, 0);
  return(0);
}

int twostep(int deferedFlag)
{
    int cmd,timeoutSec;
    int replycmd,replyarg1,replyarg2,replyarg3;
    char replymsge[512];
    int result __attribute__((unused));
    char answer[80];
    char filename[80];
    char xfername[80];
    char md5sigstr[80];
    char infostr[512];
    char *strptr;
    int fileSize;
    int ret __attribute__((unused));

   cmd = FLASH_UPDATE ;
   timeoutSec = 60;
   fprintf(stdout,"\nfilename: ");
   ret = fscanf(stdin,"%s",filename);
   printf("download file: '%s'\n\n",filename);
   fileSize = getFileSize(filename);
   readMd5(filename, md5sigstr);
   strptr = strrchr(filename,'/');
   if (strptr == NULL)
   {
       strcpy(xfername,filename);
   }
   else
   {
       strcpy(xfername,(strptr+1));
   }
   printf("file: '%s', size: %d, md5: '%s'\n\n",xfername,fileSize,md5sigstr);
   if ( (fileSize > 0) && (strlen(md5sigstr) > 5))
   {
          cmd = FLASH_UPDATE ;
          sprintf(infostr,"%s,%s",xfername,md5sigstr);
          result = send2Monitor(cmd, timeoutSec, fileSize, deferedFlag, infostr, (strlen(infostr)+1));
          /* wait for reply */
          MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
          printf("%d Cntlr Responding FLASH FS Update Rollcall: '%s'\n\n",replyarg1,replymsge);
          MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
          printf("reply - cmd: %d, result: %d, arg2: %d, arg3: %d, msge: '%s'\n\n",
		replycmd,replyarg1,replyarg2,replyarg3,replymsge);
          if (replyarg1 > 0)
          {
             printf("Flash Updated:  Failed...\n");
             printf("Controllers: '%s'  Failed to Complete.\n",replymsge);
             return(-1);
          }
          else
          {
             printf("Flash Updated:  All Controllers were Successful.\n");
          }
   }
   else
   {
         printf("Bad file, skipped\n");
         return(-1);
   }
   fprintf(stdout,"\nCommit (y or n): ");
   ret = fscanf(stdin,"%s",answer);
   if (strcmp(answer,"y") == 0)
   {
          printf("Commit Update files to Flash\n\n");
          printf("Commit: '%s', size: %d, md5: '%s'\n\n",xfername,fileSize,md5sigstr);
          cmd = FLASH_COMMIT;
          timeoutSec = 120;
          if (deferedFlag == 0)
             sprintf(infostr,"%s,%s",xfername,md5sigstr);
          else
             sprintf(infostr,"deferedcommit,%s",md5sigstr);
          result = send2Monitor(cmd, timeoutSec, fileSize, deferedFlag, infostr, (strlen(infostr)+1));
          MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
          if (replyarg1 > 0)
          {
             printf("Flash Commit:  Failed...\n");
             printf("Controllers: '%s'  Failed to Complete.\n",replymsge);
             return(-1);
          }
          else
          {
             printf("Flash COmmit:  All Controllers were Successful.\n");
          }
   }
   return(0);
}

int onestep()
{
    char filename[80];
    char xfername[80];
    char md5sigstr[80];
    char *strptr;
    int fileSize;
    int downLoad(char *filename, int filesize, char *md5sigstr);
    int ret __attribute__((unused));

   fprintf(stdout,"\nfilename: ");
   ret = fscanf(stdin,"%s",filename);
   printf("download file: '%s'\n\n",filename);
   fileSize = getFileSize(filename);
   readMd5(filename, md5sigstr);
   strptr = strrchr(filename,'/');
   if (strptr == NULL)
   {
       strcpy(xfername,filename);
   }
   else
   {
       strcpy(xfername,(strptr+1));
   }
   downLoad(xfername,fileSize,md5sigstr);
   return(0);
}

int deleteFF()
{
    int cmd,timeoutSec;
    int replycmd,replyarg1,replyarg2,replyarg3;
    char replymsge[512];
    int result __attribute__((unused));
    char filename[80];
    int ret __attribute__((unused));

   fprintf(stdout,"\nDelete filepattern: ");
   ret = fscanf(stdin,"%s",filename);
   printf("delete file: '%s'\n\n",filename);

   cmd = FLASH_DELETE ;
   timeoutSec = 80;
   result = send2Monitor(cmd, timeoutSec, 0 , 0, filename, (strlen(filename)+1));
   /* wait for reply */
   MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
   printf("%d Cntlr Responding FLASH FS Delete Rollcall: '%s'\n\n",replyarg1,replymsge);
   MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
   printf("reply - cmd: %d, result: %d, arg2: %d, arg3: %d, msge: '%s'\n\n",
		replycmd,replyarg1,replyarg2,replyarg3,replymsge);
   if (replyarg1 > 0)
   {
       printf("Flash Delete:  Failed...\n");
       printf("Controllers: '%s'  Failed to Complete.\n",replymsge);
       return(-1);
   }
   else
   {
             printf("Flash Delete:  All Controllers were Successful.\n");
   }
   return(0);
}

int downLoad(char *filename, int filesize, char *md5sigstr)
{
    int replycmd,replyarg1,replyarg2,replyarg3;
    int result __attribute__((unused));
    int timeoutSec;
    char replymsge[512];
    char infostr[512];

       printf("file: '%s', size: %d, md5: '%s'\n\n",filename,filesize,md5sigstr);
       if ( (filesize > 0) && (strlen(md5sigstr) > 5))
       {
          sprintf(infostr,"%s,%s",filename,md5sigstr);
          timeoutSec = 60;
          result = send2Monitor(FLASH_UPDATE, timeoutSec, filesize, 0, infostr, (strlen(infostr)+1));
          /* wait for reply */
          printf("Waiting for reply\n");
          MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
          printf("%d Cntlr Responding FLASH FS Update Rollcall: '%s'\n\n",replyarg1,replymsge);
          printf("Waiting for phase one completion\n");
          MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
          printf("reply - cmd: %d, result: %d, arg2: %d, arg3: %d, msge: '%s'\n\n",
		replycmd,replyarg1,replyarg2,replyarg3,replymsge);
          if (replyarg1 > 0)
          {
             printf("Flash Updated:  Failed...\n");
             printf("Controllers: '%s'  Failed to Complete.\n",replymsge);
             return(-1);
          }
          else
          {
             printf("Flash Updated:  All Controllers were Successful.\n");
          }
          printf("Commit Update file to Flash\n\n");
          /* sprintf(infostr,"%s,%s",xfername,md5sigstr); */
          timeoutSec = 120;
          result = send2Monitor(FLASH_COMMIT, timeoutSec, filesize, 0, infostr, (strlen(infostr)+1));
          MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
          if (replyarg1 > 0)
          {
             printf("Flash Commit:  Failed...\n");
             printf("Controllers: '%s'  Failed to Complete.\n",replymsge);
             return(-1);
          }
          else
          {
             printf("Flash Commit:  All Controllers were Successful.\n");
          }
       }
       return(0);
}
/*
 * Obtain the size in bytes of the files to be transfered to the controllers flash
 *
 */
int getFileSize(char *filename)
{
    int fd;
    struct stat fileStats;
 
   /* First check to see if the file system is mounted and the file     */
   /* can be opened successfully.                                       */
   fd = open(filename, O_RDONLY, 0444);
   if (fd < 0)
   {
        printf("input file '%s', could not be open\n",filename);
        return(-1);
   }
 
   if (fstat(fd,&fileStats) != 0)
   {
     printf("failed to obtain status infomation on file: '%s'\n",filename);
     close(fd);
     return(-1);
   }

   return( (int) fileStats.st_size );
}

/*
 *  read in the MD5 signature of the file being transfer to the controllers 
 *  flash FS.
 *
 */
int readMd5(char *filename, char *md5signature)
{
   char md5filename[60];
   char execname[60];
   char *strptr;
   FILE *sfd;
   int ret __attribute__((unused));

   strcpy(md5filename,filename);
   strptr = strstr(md5filename,".o"); 
   if (strptr != NULL)
   {
     strcpy(strptr,".md5");
   }
   else
   {
     strcat(md5filename,".md5");
   }

   /* printf("md5file: '%s'\n",md5filename); */
   sfd = fopen(md5filename,"r");
   if (sfd == NULL)
   {
      printf("could not open: '%s'\n",md5filename);
      md5signature[0] = 0;
      return -1;
   }
   ret = fscanf(sfd,"%s %s",md5signature,execname);
   fclose(sfd);
   /* printf("md5: '%s', exec: '%s'\n",md5signature,execname); */
   return 0;
}
