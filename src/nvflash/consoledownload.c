/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */

#ifdef VNMRS_WIN32
#include <Windows.h>
#include <Winnls.h>
#include <io.h>
#endif

#include <stdio.h>
#ifdef RTI_NDDS_4x
#include <stdarg.h>  // compile without normal but for some readon with 4x it's needed
#endif  /* RTI_NDDS_4x */
#include <stdlib.h>
#include <string.h>
#ifndef VNMRS_WIN32
#include <unistd.h>
#endif //VNMRS_WIN32
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef VNMRS_WIN32
#include <dirent.h>
#endif //VNMRS_WIN32
#include <ctype.h>
#include "errLogLib.h"
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "Monitor_Cmd.h"

#ifdef RTI_NDDS_4x
#include "Monitor_CmdPlugin.h"
#include "Monitor_CmdSupport.h"
#endif /* RTI_NDDS_4x */

#ifdef VNMRS_WIN32
#define W_OK 02
#define R_OK 04
#endif


extern NDDS_ID pMonitorPub, pMonitorSub;
extern int send2Monitor(int, int, int,int, char *, int);
extern void MonitorReply(int *, int *, int *, int *, char *);
extern void initiatePubSub();
extern NDDS_ID initiateNDDS(int);
extern int wait4Master2Connect(int timeout);
extern void DestroyDomain();

// char *ConsoleHostName = "wormhole";
// char *ConsoleNicHostname = "wormhole";

char *ConsoleHostName = "wormhole";
char *ConsoleNicHostname = "wormhole";

int * __errno()
{return 0;
}

unsigned short * _ictype;
FILE  __sF[20];

int multicast = 1;  /* enable multicasting for NDDS */
static int debuglevel = 0;
static int testlevel = 0;
static int topDirExist = 0;
static int nddOk = 0;
static char deftftpDir[256];
static char tftpDir[256];
static char objDir[256*2];
static char sysDir[256];
static char logpath[256];
static FILE *infoFd = NULL;
static FILE *histFd = NULL;

typedef struct  _obj_node {
        char *name;
        char *base;
        char *md5Str;
        int fSize;
        int  valid;
        struct _obj_node *next;
      } OBJ_NODE;

static OBJ_NODE *objList = NULL;

static void
dprint(char *format, ...)
{
     va_list   vargs;

     if (debuglevel) {
        va_start(vargs,format);
        vfprintf(stdout, format, vargs);
        va_end(vargs);
     }
}

static void
printInfo(char *format, ...)
{
     va_list   vargs;

     if (infoFd != NULL)
     {
        va_start(vargs,format);
        vfprintf(infoFd, format, vargs);
        va_end(vargs);
     }
     if (histFd != NULL)
     {
        va_start(vargs,format);
        vfprintf(histFd, format, vargs);
        va_end(vargs);
     }
}


int getFileSize(char *filename)
{
//    int fd;
#ifdef VNMRS_WIN32
    struct _stat fileStats;
#else
   struct stat fileStats;
#endif
 
   /* First check to see if the file system is mounted and the file     */
   /* can be opened successfully.                                       */

#ifdef VNMRS_WIN32
   if (_access(filename, R_OK) != 0) {
#else
    if (access(filename, R_OK) != 0) {
#endif
        dprint( "Error: open file '%s' failed.\n",filename);
        return(-1);
    }

#ifdef VNMRS_WIN32
   if (_stat(filename, &fileStats) != 0) {
#else
    if (stat(filename, &fileStats) != 0) {
#endif
        dprint("Error: failed to obtain status infomation on file: '%s'\n",filename);
        return(-1);
    }

    return( (int) fileStats.st_size );
}

int
readMd5(char *filename, char *md5signature)
{
   char md5filename[256];
   char execname[60];
   char *strptr;
   FILE *sfd;
   int ret __attribute__((unused));

   strcpy(md5filename, filename);
   strptr = strstr(md5filename,".o");
   if (strptr != NULL)
   {
     strcpy(strptr,".md5");
   }
   else
   {
     strcat(md5filename,".md5");
   }

   md5signature[0] = '\0';
   sfd = fopen(md5filename,"r");
   if (sfd == NULL)
   {
      printInfo("Error: Could not open file %s\n", md5filename);
      return (0);
   }
   ret = fscanf(sfd,"%s %s",md5signature, execname);
   fclose(sfd);
   return (1);
}


static int
download(OBJ_NODE *node, int deferedFlag)
{
    int replycmd,replyarg1,replyarg2,replyarg3;
    int cmd, timeoutSec ;
    int result;
    int fsize;
    char *fname, *md5str;
    char infostr[512];
    char replymsge[512];


    fname = node->name;
    md5str = node->md5Str;
    fsize = (int) node->fSize;
    if (fname == NULL || md5str == NULL)
   return (1);
    sprintf(infostr,"%s,%s",fname, md5str);
    dprint( "file info: %s\n", infostr);
    if (fsize <= 1) {
   dprint("file size: %d\n", fsize);
   return (1);
    }
    if (!nddOk)   return (1);
    cmd = FLASH_UPDATE;
    timeoutSec = 180;
    dprint("download cmd: %d, timeout: %d  size: %d\n",cmd, timeoutSec, fsize);
    result = send2Monitor(cmd, timeoutSec,fsize,deferedFlag,infostr,(strlen(infostr)+1));
           /* wait for reply */
    if (result < 0)
        return(1);

    dprint("wait for console reply ... \n");

    MonitorReply(&replycmd,&replyarg1,&replyarg2,&replyarg3, replymsge);
    dprint(" %d Cntlr Responding FLASH FS Update Rollcall: '%s'\n\n",replyarg1,replymsge);
    MonitorReply(&replycmd,&replyarg1,&replyarg2,&replyarg3, replymsge);
    dprint(" reply - cmd: %d, result: %d, arg2: %d, arg3: %d, msge: '%s'\n\n",
      replycmd,replyarg1,replyarg2,replyarg3,replymsge);
    if (replyarg1 > 0) {
       printInfo("File Transfer:  Failed...\n");
       printInfo("Controllers: '%s'  Failed to Complete.\n",replymsge);
       return(replyarg1);
    }
    dprint("File transfer %s Successful.\n", fname);

    if (deferedFlag == 0)
    {
       dprint("Commit Update files to Flash\n\n");
       dprint("Commit: '%s', size: %lu, md5: '%s'\n\n", fname,fsize,md5str);

       cmd = FLASH_COMMIT;
       timeoutSec = 180;
       dprint("commit cmd: %d, timeout: %d  size: %d\n",cmd, timeoutSec, fsize);
       sprintf(infostr,"%s,%s",fname, md5str);
       send2Monitor(cmd, timeoutSec, fsize,0,infostr,(strlen(infostr)+1));
       MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
       if (replyarg1 > 0) {
          printInfo("Flash Commit:  Failed...\n");
          printInfo("Controllers: '%s'  Failed to Complete.\n",replymsge);
       }
       else {
         dprint("Flash Commit:  All Controllers were Successful.\n");
       }
    }
    else
    {
       dprint("Will Commit after all files downloaded to Flash\n\n");
    }
    return(replyarg1);
}

static int deferedCommit()
{
    int replycmd,replyarg1,replyarg2,replyarg3;
    int cmd, timeoutSec ;
    int fsize = 0;
    char infostr[512];
    char replymsge[512];

       dprint("Defered Commit of Update files to Flash\n\n");
       cmd = FLASH_COMMIT;
       timeoutSec = 600;
       dprint("Commit: '%s', size: %lu, md5: '%s'\n\n","deferedcommit" ,0,"NA");
       sprintf(infostr,"deferedcommit,");
       send2Monitor(cmd, timeoutSec, fsize,1,infostr,(strlen(infostr)+1));
       MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
       if (replyarg1 > 0) {
          printInfo("Flash Commit:  Failed...\n");
          printInfo("Controllers: '%s'  Failed to Complete.\n",replymsge);
       }
       else {
         dprint("Flash Commit:  All Controllers were Successful.\n");
       }
    return(replyarg1);
}


int deleteAlldotOs()
{
    int cmd,timeoutSec;
    int replycmd,replyarg1,replyarg2,replyarg3;
    char replymsge[512];
    int result __attribute__((unused));
    char *cmdstr;

   // delete the nvScript file from flash
   cmd = FLASH_DELETE ;
   cmdstr = "nvScript";
   timeoutSec = 60;
   result = send2Monitor(cmd, timeoutSec, 0 , 0, cmdstr, (strlen(cmdstr)+1));
   /* wait for reply */
   MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
   printf("%d Cntlr Responding FLASH FS Delete Rollcall: '%s'\n\n",replyarg1,replymsge);
   MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
   printf("reply - cmd: %d, result: %d, arg2: %d, arg3: %d, msge: '%s'\n\n",
      replycmd,replyarg1,replyarg2,replyarg3,replymsge);

   // delete all the .o files from flash
   cmd = FLASH_DELETE ;
   cmdstr = "*.o";
   timeoutSec = 150;
   result = send2Monitor(cmd, timeoutSec, 0 , 0, cmdstr, (strlen(cmdstr)+1));
   /* wait for reply */
   MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
   printf("%d Cntlr Responding FLASH FS Delete Rollcall: '%s'\n\n",replyarg1,replymsge);
   MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
   printf("reply - cmd: %d, result: %d, arg2: %d, arg3: %d, msge: '%s'\n\n",
      replycmd,replyarg1,replyarg2,replyarg3,replymsge);

/*
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
*/
   return(0);
}


static void rebootConsole()
{
  send2Monitor(ABORTALLACQS, 0, 0, 0, NULL, 0);
  return;
}

void
validateMd5()
{
    int       k, pri;
    char     filename[512+16];
    char     mdStr[512*2];
    char     *ptr;
    OBJ_NODE *node, *node2;
    int ret __attribute__((unused));
    
    node = objList;
    while (node != NULL) {
       node->valid = 0;
       sprintf(filename, "%s/%s", objDir, node->name);
       node->fSize = getFileSize(filename);
       if (node->fSize > 1) {
      ptr = strrchr(node->name, '.');
      pri = 1;
      if (ptr != NULL) {
      k = ptr - node->name;
      node->base = (char *) malloc(k+2);
      strncpy(node->base, node->name, k);
      node->base[k] = '\0';
      if (strcmp(ptr, ".o") == 0)
         pri = 3;
      }
      else {
      node->base = (char *) malloc(strlen(node->name) + 2);
      strcpy(node->base, node->name);
      pri = 2;
      }
           sprintf(filename, "%s/%s", objDir, node->base);
      mdStr[0] = '\0';
           readMd5(filename, mdStr);
      k = strlen(mdStr);
      if (k > 5) {
      node->md5Str = (char *) malloc(k+2);
      strcpy(node->md5Str, mdStr);
      node->valid = pri;
      }
       }
       node = node->next;
    }

    /* remove duplicated names */
    node = objList;
    while (node != NULL) {
   if (node->valid > 0) {
       node2 = node->next;
       while (node2 != NULL) {
          if (node2->valid > 0) {
         if (strcmp(node->base, node2->base) == 0) {
         if (node2->valid > node->valid)
             node->valid = 0;
         else
             node2->valid = 0;
         }
          }
               node2 = node2->next;
       }
   }
   node = node->next;
    }
    dprint("download objects: \n");
    node = objList;
    k = 0;
    while (node != NULL) {
        if (node->valid > 0) {
          if (topDirExist) {
           sprintf(mdStr, "cp %s/%s %s", objDir, node->name, tftpDir); 
           dprint("%s \n", mdStr);
      ret = system(mdStr);
      k++;
       }
   }
   node = node->next;
    }
    if (k > 0)
      fprintf(stdout, "downloading %d files to the acquisition console. \n", k);
}


static void saveResult(int result, int num, OBJ_NODE *node)
{
    char mess[52];

#ifdef VNMRS_WIN32
   _strtime(mess);
#else
   struct timeval clock;
    gettimeofday(&clock, NULL);
    strftime(mess, 50, " %H:%M:%S ", localtime(&(clock.tv_sec)));
#endif
    printInfo(" %d. %s %s %s ", num, mess, node->name, node->md5Str);
    if (result == 0)
       printInfo(" successful\n");
    else
       printInfo(" failed\n");
}

static void
stopExpproc()
{
#ifdef VNMRS_WIN32
   char *result;
#else
    int   fd0;
#endif
   int expProc;
    FILE  *fd;
    char data[256*2];
    char tmpfile[26];
   int ret __attribute__((unused));

    sprintf(data, "%s/bin/acqcomm stop", sysDir);
#ifdef VNMRS_WIN32
   if (_access(data, R_OK) != 0) {  // was asking for execute permmission
#else
    if (access(data, X_OK) != 0) {
#endif
   dprint(" couldn't execute %s\n", data);
   return;
    }

#ifdef VNMRS_WIN32
   strcpy(tmpfile,getenv("SFUDIR"));
   strcat(tmpfile,"\\tmp\\tmpXXXXXX");
   result = _mktemp(tmpfile);
   if (result == NULL) return;
   sprintf(data,"ps -e > %s", result);
#else
   sprintf(tmpfile, "/tmp/tmpXXXXXX");
    fd0 = mkstemp(tmpfile);
    if (fd0 < 0)
   return;
    close(fd0);
    sprintf(data, "ps -e > %s", tmpfile);
#endif
    ret = system(data);
    fd = fopen(tmpfile, "r");
    if (fd == NULL)
   return;
    expProc = 0;
    while (fgets(data, 250, fd) != NULL) {
       if (strstr(data, "Expproc") != NULL) {
           expProc = 1;
       }
       if (expProc > 0) {
           dprint("  %s\n", data);
           break;
       }
   }
   fclose(fd);
#ifdef VNMRS_WIN32
    _unlink(tmpfile);
#else  //VNMRS_WIN32
   unlink(tmpfile);
#endif  //VNMRS_WIN32
    if (expProc == 0)
   return;
    dprint("there are Expproc family programs running, kill them... \n"); 
    sprintf(data, "%s/bin/acqcomm stop", sysDir);
    dprint("executing %s \n", data);
    ret = system(data);
}

/* this is the preferred order to download */
static char *md5Files[] = {"vxWorks405gpr", "nddslib", "nvlib", "masterexec",
        "rfexec", "pfgexec", "lpfgexec", "gradientexec", "lockexec", "ddrexec","icat_top",
        "icat_config", "icat_config00", "icat_config01",  "icat_config02",  "icat_config03", 
        "icat_config04",  "icat_config05", "rf_amrs", "nvScript" };
   

static void
sortListOrder()
{
    int      mdNum, k;
    OBJ_NODE *node, *p1;
    OBJ_NODE *newList, *p2;

    mdNum = sizeof(md5Files) / sizeof(char *);
    newList = NULL;
    p2 = NULL;
    for (k = 0; k < mdNum; k++) {
       node = objList;
      p1 = objList;
       while (node != NULL) {
         if (strcmp(node->base, md5Files[k]) == 0) {
            if (node == objList)
              objList = node->next;
           else
              p1->next = node->next;
           node->next = NULL;
           if (newList == NULL)
              newList = node;
           else
              p2->next = node;
           p2 = node;
           break;
         }
         p1 = node;
         node = node->next;
      }
   }
    if (newList == NULL)
      return;
    p2->next = objList;
    objList = newList;
}

static void
exit_close()
{
    if (histFd != NULL)
        fclose(histFd);
    if (infoFd != NULL) {
        fclose(infoFd);
        sprintf(objDir, "%s/loadinfo", logpath);
      dprint( "remove file  %s\n", objDir);
#ifdef VNMRS_WIN32
   _unlink(objDir);
#else
   unlink(objDir);
#endif
    }
    exit(0);
}

#define  DEBUG   1
#define  NOLOAD    2
#define  SRCDIR  3
#define  TFTPDIR  4
#define  TEST    5
#define  LOGDIR  6
#define  DELAY   7
#define  DEFERRED_COMMIT  11
#define  DELETE_DOT_O  12
#define  DOWNLOADDIR 13

/* make sure the number matches the true number of options in list below */
#define  OPTNUM  13
static char *options[] =
     {"-debug", "-d", "-nodwnld", "-objdir", "-test", "-t", "-tftpdir", 
   "-logdir", "-delay", "-sleep", "-deferred", "-deleteo", "-dwnld3x" };

static int optId[] = {DEBUG, DEBUG, NOLOAD, SRCDIR, TEST, TEST, TFTPDIR,
   LOGDIR, DELAY, DELAY, DEFERRED_COMMIT, DELETE_DOT_O, DOWNLOADDIR };

int main(int argc, char **argv)
{
    int  i, fcount, ftotalcount, scount;
    int  result, cmd, failedNum;
    int  downldflag;
    int  delayTime;
    int deferredCommitFlag, deleteDotOsFlag, downld3xFlag;
    char cmdStr[512];
    OBJ_NODE *objNode = NULL;
    OBJ_NODE *newNode;
    char *fname;
    char *bname;
#ifdef VNMRS_WIN32
   FILE *pOpen;
   char *vnmrsys;
#else
    struct timeval clock;
    DIR            *dirp;
    struct dirent  *dp;
#endif

#ifdef LINUX
    delayTime = 5;
#else
    delayTime = 2;
#endif
    DebugLevel= -3;
    downldflag = 1;
    deferredCommitFlag = 0; /* old style by default */
    deleteDotOsFlag = 0; /* old style by default */
    downld3xFlag = 0; /* download from download3x  */
    objDir[0] = '\0';
    tftpDir[0] = '\0';
    logpath[0] = '\0';
    for (i = 1; i < argc; i++) {
   cmd = 0;
   for (scount = 0; scount < OPTNUM; scount++) {
       /* printf("argv[%d]: '%s', options[%d]: '%s'\n",i, argv[i], scount, options[scount]); */
       if (strcmp(argv[i], options[scount]) == 0) {
      cmd = optId[scount];
      break;
       }
   }
   switch (cmd) {
      case DEBUG:
               debuglevel = 1;   
               DebugLevel = 2;   
            if (i < argc - 1) {
         if (isdigit(argv[i+1][0])) {
             i++;
             DebugLevel = atoi(argv[i]);
         }
            }
            break;
      case NOLOAD:
               debuglevel = 1;   
               downldflag = 0;   
            break;
      case SRCDIR:
            i++;
            if (i < argc)
                    strcpy(objDir, argv[i]);
            break;
      case TFTPDIR:
            i++;
            if (i < argc)
                    strcpy(tftpDir, argv[i]);
            break;
      case LOGDIR:
            i++;
            if (i < argc)
                    strcpy(logpath, argv[i]);
            break;
      case TEST:
               testlevel = 1; 
            break;
      case DELAY:
            i++;
            if (i < argc)
            delayTime = atoi(argv[i]);
            break;
      case DEFERRED_COMMIT:
            i++;
                      deferredCommitFlag = 1; /* old stle by default */
            break;
      case DELETE_DOT_O:
            i++;
                      deleteDotOsFlag = 1; /* delete all the .o files from flash prior to dowloads, 
                                               prevent FFS from running out of space in some situations */
            break;
      case DOWNLOADDIR:
            i++;
                      downld3xFlag = 1; /* download from download3x  */
            break;
            
   }
    }
    dprint("consoledownload... \n");
    dprint(" debug level %d\n", debuglevel);
    dprint(" deferred Commit: %d\n", deferredCommitFlag);
    dprint(" delete .o: %d\n",deleteDotOsFlag);
    dprint(" download3x: %d\n",downld3xFlag);
#ifdef VNMRS_WIN32
   strcpy(deftftpDir,getenv("SFUDIR"));
   strcat(deftftpDir,"tftpboot");
#else // VNMRS_WIN32
   strcpy(deftftpDir,"/tftpboot");
#endif // VNMRS_WIN32
    if (tftpDir[0] == '\0')
       strcpy(tftpDir, deftftpDir);
   dprint("tftpDir='%s'\n",tftpDir);

    topDirExist = 1;
#ifdef VNMRS_WIN32
   if (_access(tftpDir, W_OK) != 0) {
#else // VNMRS_WIN32
    if (access(tftpDir, W_OK) != 0) {
#endif // VNMRS_WIN32
        topDirExist = 0;
        downldflag = 0;
       fprintf(stdout, "Couldn't access directory %s\n", tftpDir);
       if (!testlevel)
          exit(0);
    }

#ifdef VNMRS_WIN32
   strcpy(sysDir,getenv("vnmrsystem_win"));  // e.g.: C:\SFU\vnmr_2.1C
   strcpy(objDir,sysDir);
   if (downld3xFlag == 1)
      strcat(objDir,"\\acq\\download3x");    // e.g.: C:\SFU\vnmr_2.1C\acq\download3x
   else
      strcat(objDir,"\\acq\\download");      // e.g.: C:\SFU\vnmr_2.1C\acq\download

   strcpy(logpath, objDir);
   strcpy(cmdStr,objDir);
   strcat(cmdStr,"\\loadinfo");        // e.g.: C:\SFU\vnmr_2.1C\acq\download\loadinfo
#else // VNMRS_WIN32
    if (getenv("vnmrsystem") == NULL)
   strcpy(sysDir, "/vnmr");
    else
   sprintf(sysDir, "%s", getenv("vnmrsystem"));

    if (objDir[0] == '\0')
    {
       if (downld3xFlag == 1)
          sprintf(objDir, "%.64s/acq/download3x", sysDir);
       else
          sprintf(objDir, "%.64s/acq/download", sysDir);
    }
    if (logpath[0] == '\0')
        strcpy(logpath, objDir);
    
    sprintf(cmdStr, "%s/loadinfo", logpath);
#endif // VNMRS_WIN32
    dprint("objDir='%s'\n",objDir);
#ifdef VNMRS_WIN32
   if (_access(cmdStr, R_OK) == 0) {
#else // VNMRS_WIN32
    if (access(cmdStr, R_OK) == 0) {
#endif // VNMRS_WIN32
       if (downldflag && !testlevel) {
           dprint("It had been downloaded already, exit.\n");
           exit(0);
       }
    }

#ifdef VNMRS_WIN32
   objList = NULL;
   vnmrsys = getenv("vnmrsystem_win");
   strcpy(cmdStr,"dir ");          // cmdStr = "dir "
   strcat(cmdStr,objDir);         // cmdStr = "dir C:\SFU\vnmrj_2.1C\acq\download"
    pOpen = _popen(cmdStr,"rt");
   while ( ! feof(pOpen) ) {
       fgets(tmpBuf, 120, pOpen);
//     printf("pOpen read: '%s'\n",tmpBuf);
      tmpN = strlen(tmpBuf);
      if ( (tmpN>39) && (tmpBuf[2]=='/') && (tmpBuf[5]=='/') ) {
          tmpBuf[tmpN-1]='\0';
          fname = &tmpBuf[39];
//        printf("pOpen file: '%s'\n",fname);
          i = strlen(fname);
           if (i > 4) {
               bname = fname + i - 4;
               if (strcmp(bname, ".md5") == 0)
                  i = 0;
           }
           if (i > 0) {
               if ((strcmp(fname, ".") == 0) || (strcmp(fname, "..") == 0))
                   i = 0;
               else if (strcmp(fname, "loadinfo") == 0)
                   i = 0;
               else if (strcmp(fname, "loadhistory") == 0)
                   i = 0;
           }

           if (i > 0) {
                newNode = (OBJ_NODE *) malloc(sizeof(OBJ_NODE));
                newNode->name = (char *) malloc(i + 2);
                strcpy(newNode->name, fname);
                newNode->next = NULL;
                if (objList == NULL)
                   objList = newNode;
                else
                   objNode->next = newNode;
                objNode = newNode;
          }
       }
   }
   _pclose(pOpen);
#else // VNMRS_WIN32
    dirp = opendir(objDir);
    if (dirp == NULL) {
   fprintf(stdout, "Couldn't access directory %s\n", objDir);
   exit(0);
    }

    objList = NULL;
    dp = readdir(dirp);
    while (dp != NULL) { 
       fname = dp->d_name;
       i = strlen(fname);
       if (i > 4) {
           bname = fname + i - 4;
           if (strcmp(bname, ".md5") == 0)
              i = 0;
       }
       if (i > 0) {
          if ((strcmp(fname, ".") == 0) || (strcmp(fname, "..") == 0))
              i = 0;
          else if (strcmp(fname, "loadinfo") == 0)
              i = 0;
          else if (strcmp(fname, "loadhistory") == 0)
              i = 0;
       }

       if (i > 0) {
            newNode = (OBJ_NODE *) malloc(sizeof(OBJ_NODE));
            newNode->name = (char *) malloc(i + 2);
            strcpy(newNode->name, fname);
            newNode->next = NULL;
            if (objList == NULL)
               objList = newNode;
            else
               objNode->next = newNode;
            objNode = newNode;
       }
        dp = readdir(dirp);
    }
    closedir(dirp);
#endif // VNMRS_WIN32

    if (objList == NULL) {
        printf("Error: no files found for download.\n");
        exit(0);
    }

    if (downldflag)
        stopExpproc();

   if (downldflag)
   {
    dprint( " initiateNDDS... \n");
    nddOk = 1;

    if (initiateNDDS(testlevel) == NULL)   /* NDDS_Domain is set */
    {
       dprint("  initiateNDDS failed.\n");
       if (!testlevel)
       {
           DestroyDomain();  // make sure to notify console this pub is going away
           exit(0);
       }
       nddOk = 0;
    }

    dprint(" initiatePubSub... \n");
    initiatePubSub();  /* monitor command pub/sub */
    if (pMonitorPub == NULL || pMonitorSub == NULL) 
    {
       dprint("  initiatePubSub failed.\n");
       if (!testlevel)
       {
          DestroyDomain();  // make sure to notify console this pub is going away
          exit(0);
       }
       nddOk = 0;
    }
   }
   else
   {
    nddOk = 0;
   }

    validateMd5();

    sortListOrder();

    if (topDirExist == 0)
        nddOk = 0;

    objNode = objList;

    if (testlevel || (downldflag && nddOk)) 
    {
        sprintf(cmdStr, "%s/loadinfo", logpath);
        infoFd = fopen(cmdStr,"w");
        if (infoFd == NULL)
            dprint("Error: couldn't open file %s \n", cmdStr);
        sprintf(cmdStr, "%s/loadhistory", logpath);
        histFd = fopen(cmdStr,"a+");
        if (histFd == NULL)
            dprint("Error: couldn't open file %s \n", cmdStr);
#ifdef VNMRS_WIN32
        strcpy(cmdStr,"Date:  ");
        _strdate( tmpBuf );
        strcat(cmdStr, tmpBuf);
        strcat(cmdStr,"\n");
#else // VNMRS_WIN32
        gettimeofday(&clock, NULL);
        strftime(cmdStr, 126, "Date:  %D\n",
        localtime((long*) &(clock.tv_sec)));
#endif // VNMRS_WIN32
        printInfo(cmdStr);
    }
    fcount = 0;
    objNode = objList;
    while (objNode != NULL) {
      if (objNode->valid > 0) {
      fcount++;   
      }
      objNode = objNode->next;
    }
    ftotalcount = fcount;
    printInfo(" ------ the number of files to be download is %d ------\n", fcount);
    if (fcount <= 0) {
       dprint("Error: no files to be download.\n");
       DestroyDomain();  // make sure to notify console quickly that this pub is gone 
       exit_close();
    }

    if (nddOk)
    {
#ifdef VNMRS_WIN32
        Sleep(5000); /* wait 5000 msec to allow NDDS to settle down */
#else // VNMRS_WIN32
        sleep(5);  /* wait a sec to allow NDDS to settle down */
#endif // VNMRS_WIN32
        if ( wait4Master2Connect(20) == -1)  /* wait for 10 sec for Master to connect */
        {
           dprint("  wait4Master2Connect(): ");
           printf("Timed out, Master Controller not connecting.\n");
           printInfo("  Master Controller connection failed.\n");
           DestroyDomain();  // make sure to notify console quickly that this pub is gone 
           exit_close();
        }
    }

    objNode = objList;
    fcount = 0;
    scount = 0;
    failedNum = 0;

    if (deleteDotOsFlag > 0)
    {
      dprint("Delete the '.o' files from flash 1st\n"); 
      deleteAlldotOs();  // we need all the room we can get..
    }
    
    while (objNode != NULL) 
    {
      if (objNode->valid > 0) 
      {
         fcount++;
         if (downldflag)
         {
             fprintf(stdout, "download file %d of %d\n", fcount,ftotalcount);
#ifdef VNMRS_WIN32
             Sleep(delayTime*1000);   // in msec
#else //VNMRS_WIN32
             sleep(delayTime);
#endif //VNMRS_WIN32
             result = download(objNode,deferredCommitFlag);
             saveResult(result, fcount, objNode); 
             if (result != 0) {
                 fprintf(stdout, "download failed.\n");
                failedNum++;
                break;
             }
          }
          else
          {
             fprintf(stdout, "would of downloaded file %d of %d\n", fcount,ftotalcount);
             fprintf(stdout, "filename: '%s', size: %d, md5: '%s'\n", objNode->name, objNode->fSize, objNode->md5Str);
             result = 1;
          }

          if (result == 0)
             scount++;
      }
      else 
      {
          dprint("  Bad file '%s',  skipped\n", objNode->name);
      }
   
      objNode = objNode->next;
    } /* while loop */

    if (deferredCommitFlag == 1)
    {
       if (failedNum > 0)
       {
           fprintf(stdout, "Failed to download all files, Will NOT Commit to Flash\n");
       }
       else
       {
         fprintf(stdout, "Commit all downloaded files\n");
         deferedCommit();
       }
    }

    dprint( "total download file number: %d\n", scount);
    printInfo(" ****** the number of downloaded files is %d ******\n\n", scount);

    if (topDirExist && (testlevel == 0)) 
    {
        objNode = objList;
        while (objNode != NULL) 
        {
           if (objNode->valid > 0) 
           {
              sprintf(cmdStr, "%s/%s", tftpDir, objNode->name);
#ifdef VNMRS_WIN32
              _unlink(cmdStr);
#else // VNMRS_WIN32
              unlink(cmdStr);
#endif // VNMRS_WIN32
              dprint("remove file  %s\n", cmdStr);
           }
           objNode = objNode->next;
        }
    }

    /* reboot Console if downloading to controllers */
    if ( (nddOk) && (downldflag))
    {
       dprint("  Rebooting Console Controllers\n");
#ifdef VNMRS_WIN32
       Sleep(1000);
#else
       sleep(1);
#endif
       rebootConsole();
    }

    if (histFd != NULL)
        fclose(histFd);
    if (infoFd != NULL) 
    {
        fclose(infoFd);
        if (testlevel || (failedNum > 0)) 
        {
            sprintf(cmdStr, "%s/loadinfo", logpath);
            dprint( "remove file  %s\n", cmdStr);
#ifdef VNMRS_WIN32
            _unlink(cmdStr);
#else //VNMRS_WIN32
            unlink(cmdStr);
#endif //VNMRS_WIN32
        }
    }
    DestroyDomain();  // make sure to notify console quickly that this pub is gone 
}

