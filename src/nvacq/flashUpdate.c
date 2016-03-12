/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/*
modification history
--------------------
11-30-04,gmb  created 
*/

/*
DESCRIPTION


*/
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif

#define USE_DELETE_NOT_RENAME
#define USE_COMPRESSION

#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <ioLib.h>
#include <sysLib.h>
#include <msgQLib.h>

#include <stat.h>

#include "errorcodes.h"
#include "nvhardware.h"
#include "instrWvDefines.h"
#include "taskPriority.h"
#include "cntlrStates.h"
#include "flashUpdate.h"
#include "crc32.h"
#include "md5.h"

#include "vTypes.h"
/* #include "vWareLib.h" */

#include "logMsgLib.h"

/* for compress & uncompression */
#define Z_OK            (0)

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "Cntlr_Comm.h"
#ifdef NO_NDDS_YET
#include "Flash_Downld.h"

/* NDDS Primary Domain */
extern NDDS_ID NDDS_Domain;

static char *pData_Bufferr = NULL;
static MSG_Q_ID pFlashMsgQ = NULL;     /* MsgQ for buffer address of ready buffers */

#endif   /* NO_NDDS_YET */

#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];

extern int DebugLevel;

extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
static char* remoteFTPDev = "wormholeFTP:";

static int filenumber = 0;

char *taskKeepList[] = {
        "tExcTask", "tLogTask", "tShell", "tRlogind", "tMsgLogd",
        "tNetTask", "n_at7400", "n_rtu7400", "n_rtm7400", "n_dt7400",
        "rDtb00", "rEvt00", "rR0000", "rR0100", "rR0200", "rR0300",
        "tMonitor", "tRlogOutTask", "tRlogInTask", "tLogTask",
        "n_mgr7400", "tPanelLeds" , "tFFSDownld", "tFFSUpdate", /* "tAppHBPub", */ NULL };

char *taskKeepMatchList[] = {
        "rDtb00", "rEvt00", "rR0000", "rR0100", "rR0200", "rR0300", NULL };

/* now use a list of what should NOT be suspended.. */
flashSuspendTasks()
{
   int numTasks,i,j,stat,keepFlag,tid;
   int taskIdList[256];
   char *tName;

   /* obtain all running tasks */
   numTasks = taskIdListGet(taskIdList,256);
   for (i=0; i < numTasks; i++)
   {
      DPRINT1(+4,"Task: '%s'  \n", taskName(taskIdList[i]));
      keepFlag = 0;
      /* check each task to see if it's on the the keep list */
      for (j=0; taskKeepList[j] != NULL; j++)
      {  
         if (taskIdList[i] == taskNameToId(taskKeepList[j]))
         {
            DPRINT2(+4,"Keep; '%s', ID; 0x%lx,\n", taskKeepList[j],taskIdList[i]);
            keepFlag = 1;
            break;
         }
      }   

      if (keepFlag == 1)
        continue;

      /* now the for NDDS 4.x task matches, these they are dynamicly named thus we look for 
         a match in the first 6 charaters
      */
      for (j=0; taskKeepMatchList[j] != NULL; j++)
      {  
         /* name of task to check */
         tName = (char*) taskName(taskIdList[i]);
         /* DPRINT2(+4,"Task: '%s' , MatchTask: '%s' \n", taskName(taskIdList[i]),taskKeepMatchList[j]); */
         /* compare the names with in the 1st 6 char for a match */
         if (strncmp(tName,taskKeepMatchList[j],strlen(taskKeepMatchList[j])) == 0)
         {
            DPRINT2(+4,"Keep; '%s', ID; 0x%lx,\n", tName,taskIdList[i]);
            keepFlag = 1;
            break;
         }
      }   
 
      if (keepFlag == 1)
        continue;

      if ((stat = taskSuspend(taskIdList[i])) != ERROR)
      {  
          DPRINT1(+4,"Suspending '%s' Successful\n",taskName(taskIdList[i]));
      }  
      else
      {  
          errLogRet(LOGIT,debugInfo, "Suspending '%s' Failed\n",taskName(taskIdList[i]));
      }  
   }
}

/*************************************************************
*
*  flashUpdate - Wait for Message 
*
*			Author Greg Brissey 12-02-04
*/
flashUpdate(MSG_Q_ID pMsgesToFFSUpdate)
{
   void execflash();
   FLASH_UPDATE_MSG Msge;
   int bytes;

   DPRINT(-1,"flashUpdate 1:Server LOOP Ready & Waiting.\n");

   /* disable extern interrupt from the FPGA while programming FPGA */
   /* is never re-enable by this routine */
   DPRINT(-1,"flashUpdate: disable FPGA extern interrupt while Flashing.\n");
   sysUicIntDisable(25);
   haltLedShow();
   /* flashSuspendTaskList(); old way */
   flashSuspendTasks();

   FOREVER
   {
     /* if connection lost this routine send msge to phandler LOST_CONN */
     bytes = msgQReceive(pMsgesToFFSUpdate, (char*) &Msge,sizeof(FLASH_UPDATE_MSG), WAIT_FOREVER);
     DPRINT1(2,"flashUpdate: got %d bytes\n",bytes);

     execflash( &Msge );
   } 
}

MSG_Q_ID pFileTransferedQueue = NULL;

MSG_Q_ID startFlashUpdate(int taskpriority,int taskoptions,int stacksize)
{
   MSG_Q_ID pMsgesToFFSUpdate;
    pMsgesToFFSUpdate = msgQCreate(20, sizeof(FLASH_UPDATE_MSG), MSG_Q_FIFO);
    pFileTransferedQueue = msgQCreate(20, 128, MSG_Q_FIFO); /* 20 file file name upto 128 char */
    if (taskNameToId("tFFSUpdate") == ERROR)
     taskSpawn("tFFSUpdate",taskpriority,taskoptions,
                   stacksize,flashUpdate, pMsgesToFFSUpdate,2,3,4,5,6,7,8,9,10);
     return( pMsgesToFFSUpdate );
}

execflash(FLASH_UPDATE_MSG *msge)
{
   int fileSize,result,cmpltCode;
   char msgstr[COMM_MAX_STR_SIZE+2];
   char filename[256];
   char md5sig[64];
   char newname[256];
   char  updatefilename[256];
   unsigned long srcCRC;
   char *pCList;
   char *strptr;
   int flashUpdateFunc(char *filename,int size,char *md5sig);
   int flashCommitFunc(char *filename,int size,char *md5sig);

/*
    msge->cntlrId;
    msge->cmd;
    msge->errorcode;
    msge->warningcode;
    msge->arg1;
    msge->arg2;
    msge->arg3;
    msge->crc32chksum;
    msge->msgstr;
*/
    DPRINT5(+3,"execflash:  cmd: %d, filesize: %d, arg2: %d, arg3: %d, crc: 0x%lx\n",
          msge->cmd,msge->filesize, msge->arg2, msge->arg3, msge->crc32chksum);
    DPRINT1(+3,"execflash:  msge: '%s'\n",msge->msgstr);

    switch(msge->cmd)
    {
       case CNTLR_CMD_FFS_UPDATE:
       {
           fileSize = msge->filesize;
           srcCRC = msge->crc32chksum;

	        strncpy(msgstr,msge->msgstr,COMM_MAX_STR_SIZE+1);
           pCList = msgstr;

           filename[0]= 0;
           strptr = (char*) strtok_r(msgstr," ,",&pCList);
           if (strptr != NULL)
               strcpy(filename,strptr);

            md5sig[0]= 0;
            strptr = (char*) strtok_r(NULL," ,",&pCList);
            if (strptr != NULL)
               strcpy(md5sig,strptr);

            newname[0] = 0;
            strptr = (char*) strtok_r(NULL," ,",&pCList);
            if (strptr != NULL)
               strcpy(newname,strptr);
            
         
            filenumber++;

            DPRINT3(-1,"execflash: filename: '%s', size: %ld, new name: '%s'\n",
			     filename,fileSize,newname);
            DPRINT1(-1,"execflash: md5sig: '%s'\n", md5sig);
            if (msge->arg2 == 0)  /* if 1, then defered update so do NOT delete .update files */
            {
               ffdel("*.update");
               ffdel("*.update.z");
               ffdel("*.orig");
               ffdel("*.fail");
               // del unwanted icat files from non RF boards
               if (BrdType !=  RF_BRD_TYPE_ID)
               {
                  ffdel("icat*.*");
               }
            }
            result = flashUpdateFunc(filename,fileSize,md5sig);
            DPRINT(-1,"execflash: FLASH_UPDATE: Complete\n");
	         /* send2Expproc(39,((result == 0) ? 42 : 666),0,0,NULL,0); /* NO_WAIT, MSG_PRI_NORMAL); */
	         /* tell master we are done */
            cmpltCode = (result == 0) ? CNTLR_FLASH_UPDATE_CMPLT : CNTLR_FLASH_UPDATE_FAIL;
	         if (BrdType == MASTER_BRD_TYPE_ID)
                cntlrSetState("master1",cmpltCode,0);
            else
               send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);
       }
       break;

       case CNTLR_CMD_FFS_COMMIT:
       {
           int bytes;
           long getFFSize(char *filename);
           fileSize = msge->filesize;
           srcCRC = msge->crc32chksum;

	        strncpy(msgstr,msge->msgstr,COMM_MAX_STR_SIZE+1);
           pCList = msgstr;

           filename[0]= 0;
           strptr = (char*) strtok_r(msgstr," ,",&pCList);
           if (strptr != NULL)
               strcpy(filename,strptr);

            md5sig[0]= 0;
            strptr = (char*) strtok_r(NULL," ,",&pCList);
            if (strptr != NULL)
               strcpy(md5sig,strptr);

            newname[0] = 0;
            strptr = (char*) strtok_r(NULL," ,",&pCList);
            if (strptr != NULL)
               strcpy(newname,strptr);
            
            DPRINT1(-3,"filename: '%s'\n", filename);
            if (strcmp(filename,"deferedcommit") == 0) /* this is a defered commit */
            {
               result = 0; // incase there no no files to commit, it should be 'seccussful'
               while( msgQReceive(pFileTransferedQueue,(char*) filename,256,NO_WAIT) != ERROR)
               {
#ifdef USE_COMPRESSION
                  /* For compression, besides the filename we also need the uncompressed size and md5sig */
                  char fname[128],ssize[128];
                  char md5sig[64];
                  pCList = filename;
                  fname[0]=0;
                  strptr = (char*) strtok_r(filename," ,",&pCList);
                  if (strptr != NULL)
                      strcpy(fname,strptr);

                  ssize[0]= 0;
                  strptr = (char*) strtok_r(NULL," ,",&pCList);
                  if (strptr != NULL)
                     strcpy(ssize,strptr);
                  fileSize = atol(ssize);

                  md5sig[0]= 0;
                  strptr = (char*) strtok_r(NULL," ,",&pCList);
                  if (strptr != NULL)
                     strcpy(md5sig,strptr);

                  DPRINT4(-1,"execflash: filename: '%s', size: %ld, md5: '%s', new name: '%s'\n",
			             fname,fileSize,md5sig,newname);

                  result = flashCommitFunc(fname,fileSize,md5sig);
                  DPRINT(-1,"execflash: FLASH_COMMIT: Complete\n");
                  if (result != 0)
		               break;

#else    /* ! USE_COMPRESSION */

                  fileSize = getFFSize(filename);
                  DPRINT3(-1,"execflash: filename: '%s', size: %ld, new name: '%s'\n",
			             filename,fileSize,newname);
                  sprintf(updatefilename,"%s.update",filename);
                  fileSize = getFFSize(updatefilename);
                  ffmd5(updatefilename,md5sig) ;
                  DPRINT1(-1,"execflash: md5sig: '%s'\n", md5sig);
                  result = flashCommitFunc(filename,fileSize,md5sig);
                  DPRINT(-1,"execflash: FLASH_COMMIT: Complete\n");
                  if (result != 0)
		               break;

#endif  /* USE_COMPRESSION */

               }
               /* tell master we are done */
              cmpltCode = (result == 0) ? CNTLR_FLASH_COMMIT_CMPLT : CNTLR_FLASH_COMMIT_FAIL;
              if (BrdType == MASTER_BRD_TYPE_ID)
                 cntlrSetState("master1",cmpltCode,0);
              else
                 send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);
            }
            else
            {
              DPRINT3(-1,"execflash: filename: '%s', size: %ld, new name: '%s'\n",
			           filename,fileSize,newname);
              DPRINT1(-1,"execflash: md5sig: '%s'\n", md5sig);
              result = flashCommitFunc(filename,fileSize,md5sig);
              DPRINT(-1,"execflash: FLASH_COMMIT: Complete\n");
	           /* tell master we are done */
              cmpltCode = (result == 0) ? CNTLR_FLASH_COMMIT_CMPLT : CNTLR_FLASH_COMMIT_FAIL;
	           if (BrdType == MASTER_BRD_TYPE_ID)
                  cntlrSetState("master1",cmpltCode,0);
              else
                 send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);
             }
       }
       break;

       case CNTLR_CMD_FFS_DELETE:
       {
	   strncpy(msgstr,msge->msgstr,COMM_MAX_STR_SIZE+1);
           pCList = msgstr;
           filename[0]= 0;
           strptr = (char*) strtok_r(pCList," ,",&pCList);
           if (strptr != NULL)
               strcpy(filename,strptr);
           DPRINT1(-1,"execflash: CNTLR_CMD_FFS_DELETE: filepattern: '%s'\n",
			filename);

	   result = ffdel(filename);
           cmpltCode = (result < 0) ? CNTLR_FLASH_UPDATE_FAIL  : CNTLR_FLASH_UPDATE_CMPLT;
	   if (BrdType == MASTER_BRD_TYPE_ID)
                cntlrSetState("master1",cmpltCode,0);
           else
               send2Master(CNTLR_CMD_STATE_UPDATE,cmpltCode,0,0,NULL);

       }
       break;

      default:
		break;
    }
    return( result );
}

/* =============================================================================== */

/*
 * calculate the MD5 signiture of file
 *
 *   Author: Greg Brissey 12/01/04
 */
int calcMD5sig(char *buffer, long sizeBytes, char *hex_output)
{
   int di;
   md5_state_t state;
   md5_byte_t digest[16];
   /* char hex_output[16*2 + 1]; */

   /* ---------------------------*/
   /* calc MD5 checksum signiture */
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)buffer, sizeBytes);
   md5_finish(&state, digest);
   /* generate checksum string */
   for (di = 0; di < 16; ++di)   
        sprintf(hex_output + di * 2, "%02x", digest[di]);
   /* ---------------------------*/

   DPRINT1(1,"MD5 Calc Digest: '%s'\n",hex_output);
}

unsigned long ffsFileCRC(char *filename);

/* srcCRC = ffsFileCRC(filename); */

/*
 *  This assumes that the tftpd deamon is running and its root directory is where the files
 *  will be. Thus only the filename needs to be given.
 *  The file size must also be given.
 * obtain file via TFTP
 * return -1,-2,-3 for failures
 * return number of bytes copied to buffer 
 *
 *   Author: Greg Brissey 12/01/04
 */
getFileViaTFTP(char *filename, int filesize, char *fileBuffer)
{
    /* FFSHANDLE newFile; */
    char buffer[8192];
    char ffsfilename[128];
    char *memname = "flashUpdateDev:";
    char *chrptr, *mptr;
    int fd,result,bytes;
    struct stat fileStats;
    unsigned long fileSizeBytes;
    int size,retries;
 
   size = 0;
 
   if (filename == NULL)
      return(-1);
 
   fileSizeBytes = (filesize == 0) ? (1048576 * 5) : filesize;


   if ((mptr = (char*) malloc(fileSizeBytes)) == NULL)
   {
         perror("malloc:");
         return(-1);
   }

   /* in reality must know files size, since whatever we allocate
      in the memDev, will be the filesize when we read it back from
      then memDev()
   */
   if ( memDevCreate(memname,mptr,fileSizeBytes) == ERROR)
   {
        perror("memDevCreate");
        free(mptr);
        return(-2);
   }
   retries = 3;
   result = ERROR;
   while( (retries-- > 0) && (result != OK) )
   {
      fd = open(memname,O_RDWR,0);
      if (fd == ERROR)
      {
         printf("Error in open of mem device\n");
         taskDelay(calcSysClkTicks(500));   /* delay 16.6 msec , one clock tick @ 60 Hz */
         result = ERROR;
         continue;
      }
      DPRINT(-1,"getFileViaTFTP: TFTP Transfer started\n");
      result = tftpCopy("wormhole",0,filename,"get","binary",fd); 
      close(fd);
      if (result != OK)
      {
         taskDelay(calcSysClkTicks(500));   /* delay 16.6 msec , one clock tick @ 60 Hz */
      }
   }
   if ( result != OK)
   {
      memDevDelete(memname);
      free(mptr);
      printf("Error in transfer: \n");
        return(-3);
   }

   fd = open(memname,O_RDONLY,0);
   result = 1;
   bytes = 0;
   chrptr = fileBuffer;
   while( result > 0)
    {
       result = read(fd,chrptr,8192);
       bytes += result;
       chrptr += result;
    }
   DPRINT1(-1,"TFTP transfered %d bytes\n",bytes);
    
   memDevDelete(memname);
   free(mptr);
   return(bytes);
}

FFSHANDLE vfCreateFile(char *, unsigned long, unsigned long, unsigned long);
FFSHANDLE vfOpenFile(char *);
unsigned long vfFindFirst(FFSFF *, char *);
unsigned long vfFindNext(FFSFF *, char *);
unsigned long vfCloseFile(FFSHANDLE);
unsigned long vfAbortFile(FFSHANDLE);
unsigned long vfReadFile(FFSHANDLE, unsigned long, unsigned char *, unsigned long, unsigned long *);
unsigned long vfWriteFile(FFSHANDLE, unsigned char *, unsigned long, unsigned long *);
unsigned long vfDeleteFile(char *);
unsigned long vfGetLastError(void);
unsigned long vfBlockSize(void);
void vfFfsResult(char *);
unsigned long vfFilesize(FFSHANDLE);



/*
 * Utility function
 * search the Controllers Flash Flile System for any file
 * names that match the regex pattern
 * returning a list of the names and the number found
 *   Author: Greg Brissey
 */
int fffind(char *regexp, char *filenames, int filenamelen)
{
    char buffer[80];
    char *fileMask;
    int totalchars,len;
    UINT32 ffssize, x, y, rc;
    FFSFF pFF;
    FFSFRAGMENT *pFragment;
    UINT32 fSize;
    UINT32 totalfiles,totalfSize;
    // HANDLE newFile;

    if (regexp != NULL)
        fileMask = regexp;
    else
        fileMask = "*";

    if (filenames != NULL)
        memset(filenames,0,filenamelen);

   /*   Do a directory of the filesystem */
    totalchars = totalfiles = totalfSize = 0;
    rc = vfFindFirst(&pFF, fileMask);
    while (!rc) {
         pFragment = vfDecode(pFF.BlockOffset);
         strcpy(buffer, (char *)(((FFSFILEHEADER *)pFragment)->Name));
         totalfiles++;
         if (filenames != NULL)
         {
            len = strlen(buffer);
            totalchars = strlen(filenames);
            // printf("'%s', %ld chars\n", buffer, len);
            // printf("'%s', %ld chars\n", filenames,totalchars);
            totalchars = totalchars + len;
            // printf("totalchars: %ld, max: %ld\n", totalchars, filenamelen);
            if (totalchars + 2 <=  filenamelen )  // 1 for "," and 1 more for the null termination
            {
                strcat(filenames,buffer);
                strcat(filenames,",");
            }
            // printf("%25s\n", buffer);
            totalchars = strlen(filenames);
            // printf("'%s', %ld chars\n", filenames,totalchars);
         }

         rc = vfFindNext(&pFF, fileMask);
    }
    return(totalfiles);
}

/*
 * simple test routine for the fffind function above
 *  GMB
 */
/************************************************/
int tstffind(char *regex)
{
    char buffer[2048];
    int nfound;
    nfound = fffind(regex, buffer, 2048);
    printf("found: %d, names: '%s'\n", nfound,buffer);
    return nfound;
}

int tstffind2(char *regex)
{
    int nfound;
    nfound = fffind(regex, NULL, 0);
    printf("found: %d\n", nfound);
    return nfound;
}

/************************************************/



/*
 * cpBuf2FF() copies a buffer as a file onto the Flash File System
 * filename - name to use for filename
 * buffer - pointer to the buffer of data that will be the body of the file.
 * size - the number of bytes in the buffer
 *
 * Author: Greg Brissey 12/01/04
 */
cpBuf2FF(char *filename,char *buffer, unsigned long size)
{
    FFSHANDLE newFile;
    unsigned long bufCnt;
    unsigned long errorcode;
    unsigned long srcCRC, dstCRC;
    unsigned long ffsFileCRC(char *filename);
    int ffdel(char *filename);
    int ffmv(char *filename,char *newname);
    int renameFF(char *filename,char *newname);

    panelLedOn(DATA_XFER_LED);

    DPRINT3(-1,"filename: '%s', buffer: 0x%lx, size: %lu\n",filename,buffer,size);
    /*  Check if file already exist on flash */
    if ((newFile = vfOpenFile(filename)) != 0)
    {
        /* file already present */
        vfCloseFile(newFile);
        printf("File: '%s' already exist.\n",filename);
        panelLedOff(DATA_XFER_LED);
        return(-1);

        /*  Close and delete file */
        /* sysFlashRW(); */
        /* vfAbortFile(newFile); */
        /* sysFlashRO();     Flash to Read Only, via EBC  */
    }

    /*  Create the file from scratch */
    sysFlashRW();
    /* DPRINT1(-1,"vfCreateFile: '%s'\n",filename); */
    newFile = vfCreateFile(filename, -1, -1, -1);
    if (newFile == 0)
    {
       errorcode = vfGetLastError();
       if (errorcode)
          vfFfsResult(filename);
       sysFlashRO();    /* Set Flash to Read Only, via EBC  */
        panelLedOff(DATA_XFER_LED);
       return(-1);
    }  

    /* calc source CRC */
    /* srcCRC = addbfcrc(buffer, size); */
    /* DPRINT2(-1,"Src CRC32: %lu [0x%lx]\n",srcCRC,srcCRC); */

    errorcode = vfWriteFile(newFile, buffer, size, (unsigned long *) &bufCnt);
    if (errorcode != 0)
    {
       errorcode = vfGetLastError();
       if (errorcode)
            vfFfsResult(filename);
       /*  Close and delete file */
       vfAbortFile(newFile);
       sysFlashRO();    /* Flash to Read Only, via EBC  */
        panelLedOff(DATA_XFER_LED);
       return(-1);
    }
    vfCloseFile(newFile);
    sysFlashRO();    /* Set Flash to Read Only, via EBC  */

    panelLedOff(DATA_XFER_LED);
 
#ifdef XXX
    /* obtain the CRC of the flash file contents */
    dstCRC = ffsFileCRC(filename);
    if (srcCRC == dstCRC)
    {
       DPRINT1(-1,"Copy Successful, CRC match: 0x%lx\n",dstCRC);
    }
    else
    {
       DPRINT2(-1,"Copy Failure, CRC MisMatch: 0x%lx, 0x%lx\n",srcCRC,dstCRC);
       DPRINT1(-1," '%s': removed from Flash.\n",filename);
       ffdel(filename);
       return(-1);
    }
#endif
    return(0);
}

/*
 * cpFF2Buf() copies a FFS File into a buffer
 * filename - name of file on FFS 
 * buffer - pointer to the buffer that the contents of the files is placed.
 * size - pointer to size, which wiil be set be this function 
 *
 * Author: Greg Brissey 12/01/04
 */
cpFF2Buf(char *filename,char *buffer, unsigned long *size)
{
    FFSHANDLE newFile;
    unsigned long  rc;
    unsigned long bufCnt;
    unsigned long errorcode;
    unsigned long srcCRC, dstCRC;
    unsigned long ffsFileCRC(char *filename);
    unsigned long fileSizeBytes;

    if ((newFile = vfOpenFile(filename)) == 0)
    {
        /* file not present */
        printf("File: '%s' does not exist on FFS.\n",filename);
        return(-1);
     }  
       
    *size = fileSizeBytes = vfFilesize(newFile);
    rc = vfReadFile(newFile, 0, buffer, fileSizeBytes, &bufCnt);
    if (bufCnt != fileSizeBytes)
    {
       printf("Incomplete write to host, only %ld bytes of %ld bytes written.\n",
            (unsigned long) fileSizeBytes,(long) bufCnt);
       vfCloseFile(newFile);
       return(-1);
    }

    vfCloseFile(newFile);
    return(0);
}

long getFFSize(char *filename)
{
   FFSHANDLE fileHdl;
   long bytes;
    /*  Open file on flash */
    if ((fileHdl = vfOpenFile(filename)) == 0)
    {
        printf("File: '%s' not found on FFS.\n",filename);
        return(-1);
    }
    bytes = vfFilesize(fileHdl);
    vfCloseFile(fileHdl);
    return(bytes);
}

long rmFF(char *filename)
{
   FFSHANDLE fileHdl;
   long bytes;
    /*  Open file on flash */
    if ((fileHdl = vfOpenFile(filename)) == 0)
    {
        return(0);
    }
    /* make flash writable, other wise an exception is thrown */
    sysFlashRW();

    vfAbortFile(fileHdl);
    sysFlashRO();
    return(0);
}
/*
 * Ok the kernel ffmv() has been giving me trouble so here is 
 * a not so eligant way of changing the file names.
 * Unfortunately there is no file reanming mechanism for this
 * FFS so one most do a copy to change the name
 *
 *     Author:  Greg Brissey   12/10/04
 */
renameFF(char *origfile, char *newfile)
{
   int i,result;
   char *xferfileBuffer;

   tcrc srcCRC,dstCRC;
   unsigned long nSize;
   long size;

   
   size = getFFSize(origfile);
   if (size < 1)   /* return file not present */
    return(0);

   xferfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) )
       return -1;

   DPRINT3(-1,"renameFF: '%s' to '%s', size: %lu\n",origfile,newfile,size);

   /* transfer FFS into buffer */
   result =  cpFF2Buf(origfile,xferfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "renameFF():  ****** Read from Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }

   srcCRC = addbfcrc(xferfileBuffer, nSize);

   result =  cpBuf2FF(newfile,xferfileBuffer, nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "renameFF():  ****** Write to Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }

   /* clear buffer */
   for(i=0; i < size; i++)
      xferfileBuffer[i] = 0;

   /* transfer FFS into buffer */
   result =  cpFF2Buf(newfile,xferfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "renameFF():  ****** Read from Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }

   dstCRC = addbfcrc(xferfileBuffer, nSize);

   if ( srcCRC != dstCRC ) 
   {
      errLogRet(LOGIT,debugInfo, "renameFF():  ****** ERROR: Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "renameFF():  True CRC - 0x%lx\n",srcCRC);
      errLogRet(LOGIT,debugInfo, "renameFF():  Calc CRC - 0x%lx\n",dstCRC);
      free(xferfileBuffer);
      return(-1);
   }
   free(xferfileBuffer);
   rmFF(origfile);
   return(nSize);
}

#ifndef USE_COMPRESSION

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* non compression routines
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

int flashUpdateFunc(char *filename,int size,char *md5sig)
{
   int i,result;
   char  updatefilename[128];
   char hex_output[16*2 + 1];
   char fileMD5[16*2 + 1];
   char cmprsMD5[16*2 + 1];
   char *xferfileBuffer;
   tcrc calcChkSum,srcCRC,dstCRC;
   unsigned long nSize;
   unsigned long cmpLen;
   char *cmprsFileBuffer = NULL;
  
   DPRINT2(-1,"flashUpdate: file: '%s', size: %lu\n",filename,size);
   DPRINT1(-1,"flashUpdate: MD5: '%s'\n",md5sig);

   if ( (result = ffmd5(filename,fileMD5)) == 0) 
   {
      /* test present file md5 to file tobe loaded md5 */
      if (strcmp(fileMD5,md5sig) == 0)
      {
          ffsBlinkLED(5, 10, 0x20);
          DPRINT1(-1,"flashUpdate: No Need to transfer: '%s'\n",filename);
          DPRINT1(-1,"flashUpdate: Update file is same as FFS file: '%s'\n",filename);
          return(0);
      }
   }

   sprintf(updatefilename,"%s.update",filename);

   /* incase of partial failure, test for update files that are current */
   if ( (result = ffmd5(updatefilename,fileMD5)) == 0) 
   {
      /* test present update file md5 to file to be loaded md5 */
      if (strcmp(fileMD5,md5sig) == 0)
      {
          ffsBlinkLED(5, 10, 0x20);
          DPRINT1(-1,"flashUpdate: No Need to transfer: '%s'\n",filename);
          DPRINT1(-1,"flashUpdate: Current .update file is same as download file: '%s'\n",filename);
	  /* no need to transfer again, but we do need to commit it later on */
          msgQSend(pFileTransferedQueue, filename, strlen(filename)+1, NO_WAIT, MSG_PRI_NORMAL);
          return(0);
      }
      else
      {
         ffdel(updatefilename);
      }
   }

   xferfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) )
   {
       blinkLED(1,3); /* fail mode */
       return -1;
   }

   ffsBlinkLED(5, 10, 0x20);
   result = getFileViaTFTP(filename, size, xferfileBuffer);
   if ( result <= 0 )
   {
       free(xferfileBuffer);
       if (cmprsFileBuffer != NULL)
         free(cmprsFileBuffer);
       blinkLED(1,3); /* fail mode */
       return -1;
   }
   result = calcMD5sig(xferfileBuffer, size, hex_output);
   srcCRC = calcChkSum = addbfcrc(xferfileBuffer, size);
   /* DPRINT1(-1,"flashUpdate:   MD5 Calc Digest: '%s'\n",hex_output); */
   /* DPRINT2(-1,"flashUpdate: CRC32 Calc ChkSum: %lu [0x%lx]\n",calcChkSum,calcChkSum); */
   if (strcmp(hex_output, md5sig)) 
   {
      errLogRet(LOGIT,debugInfo, "flashUpdate():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "flashUpdate:  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "flashUpdate:  Calc MD5 - '%s'\n",hex_output);
      free(xferfileBuffer);
      if (cmprsFileBuffer != NULL)
         free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   DPRINT(+1,"flashUpdate: Transfer From Host to Controller Memory Successful\n");
   DPRINT(+1,"flashUpdate: Transfer File to FFS\n");
   ffsBlinkLED(5, 10, 0x30);

   result =  cpBuf2FF(updatefilename,xferfileBuffer, size);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashUpdate():  ****** Copy to Flash FileSystem failed\n");
      free(xferfileBuffer);
      if (cmprsFileBuffer != NULL)
         free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   ffsBlinkLED(5, 10, 0x38);
   /* clear buffer */
   for(i=0; i < size; i++)
      xferfileBuffer[i] = 0;

   /* transfer FFS into buffer */
   result =  cpFF2Buf(updatefilename,xferfileBuffer,&nSize);
   result = calcMD5sig(xferfileBuffer, size, hex_output);
   DPRINT1(-1,"MD5 Calc Digest: '%s'\n",hex_output);
   dstCRC = addbfcrc(xferfileBuffer, size);
   DPRINT2(+1,"srcCRC: 0x%lx, dstCRC: 0x%lx\n",srcCRC,dstCRC);
   if ( (strcmp(hex_output, md5sig)) || (srcCRC != dstCRC) ) 
   {
      errLogRet(LOGIT,debugInfo, "flashUpdate:  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "flashUpdate:  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "flashUpdate:  Calc MD5 - '%s'\n",hex_output);
      errLogRet(LOGIT,debugInfo, "flashUpdate:  True CRC - 0x%lx\n",srcCRC);
      errLogRet(LOGIT,debugInfo, "flashUpdate:  Calc CRC - 0x%lx\n",dstCRC);
      free(xferfileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   free(xferfileBuffer);


    /* save the file name with in the msgQ */
   msgQSend(pFileTransferedQueue, filename, strlen(filename)+1, NO_WAIT, MSG_PRI_NORMAL);

   DPRINT1(-1,"flashUpdate() creation of '%s' Complete\n",updatefilename);
   return(0);
}


int flashCommitFunc(char *filename,int size,char *md5sig)
{
   int i,result;
   char  updatefilename[128];
   char  origbackup[128];

   char hex_output[16*2 + 1];
   char fileMD5[16*2 + 1];
   char *xferfileBuffer;
   tcrc calcChkSum,srcCRC,dstCRC;
   unsigned long nSize,regSize;
   long cmprsSize;
   char *cmprsFileBuffer = NULL;
  
   DPRINT2(-1,"flashCommit: file: '%s', size: %lu\n",filename,size);
   DPRINT1(-1,"flashCommit: MD5: '%s'\n",md5sig);

   sprintf(updatefilename,"%s.update",filename);
   sprintf(origbackup,"%s.orig",filename);

   if ( (result = ffmd5(filename,fileMD5)) == 0) 
   {
      /* test present file md5 to file tobe loaded md5 */
      if (strcmp(fileMD5,md5sig) == 0)
      {
          ffsBlinkLED(5, 10, 0x00);
          DPRINT1(-1,"flashCommit: Skipping Commit for: '%s'\n",filename);
          DPRINT1(-1,"flashCommit: Update file is same as FFS file: '%s'\n",filename);
          return(0);
      }
   }

   xferfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) )
   {
       errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: Malloc of xfer File buffer (%ld) failed\n",size);
       blinkLED(1,3); /* fail mode */
       return -1;
   }

   /* transfer regular FFS into xfer buffer */
   ffsBlinkLED(5, 10, 0x30);
   result =  cpFF2Buf(updatefilename,xferfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** Copy of Update Flash File to Memory failed\n");
      free(xferfileBuffer);
      if (cmprsFileBuffer != NULL)
        free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }

   result = calcMD5sig(xferfileBuffer, size, hex_output);
   srcCRC = addbfcrc(xferfileBuffer, size);

   DPRINT1(-1,"flashCommit: MD5 Calc Digest: '%s'\n",hex_output);
   /* DPRINT2(-1,"flashCommit: CRC32 Calc ChkSum: %lu [0x%lx]\n",calcChkSum,calcChkSum); */
   if (strcmp(hex_output, md5sig)) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "flashCommit:  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "flashCommit:  Calc MD5 - '%s'\n",hex_output);
      free(xferfileBuffer);
      if (cmprsFileBuffer != NULL)
        free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   DPRINT(-1,"flashCommit: Transfer From FFS to Controller Memory Successful\n");

   /* got the file into memory OK, so backup origianl now */
   DPRINT2(-1,"Creating Backup of Original '%s' to '%s'\n",filename,origbackup);
   /* result = ffmv(filename,origbackup); */
   ffsBlinkLED(5, 10, 0x38);

#ifndef USE_DELETE_NOT_RENAME
   result = renameFF(filename,origbackup);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** Backup of Original File Failed.\n");
/*
      free(xferfileBuffer);
      return(-1);
*/
   }
#else  /* new way to save flash space, just delete file. */

   rmFF(filename);
   rmFF(updatefilename); // remove update file as well just about to write it out

#endif

   DPRINT(-1,"flashCommit: Transfer File to FFS\n");
   ffsBlinkLED(5, 10, 0x3E);
   result =  cpBuf2FF(filename,xferfileBuffer, size);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** Copy to Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }
   ffsBlinkLED(5, 10, 0x36);
   /* clear buffer */
   for(i=0; i < size; i++)
      xferfileBuffer[i] = 0;

   /* transfer FFS into buffer */
   result =  cpFF2Buf(filename,xferfileBuffer,&nSize);
   ffsBlinkLED(5, 10, 0x32);

   result = calcMD5sig(xferfileBuffer, size, hex_output);
   DPRINT1(-1,"MD5 Calc Digest: '%s'\n",hex_output);
   dstCRC = addbfcrc(xferfileBuffer, size);
   DPRINT2(+1,"srcCRC: 0x%lx, dstCRC: 0x%lx\n",srcCRC,dstCRC);
   if ( (strcmp(hex_output, md5sig)) || (srcCRC != dstCRC) ) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "flashCommit:  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "flashCommit:  Calc MD5 - '%s'\n",hex_output);
      errLogRet(LOGIT,debugInfo, "flashCommit:  True CRC - 0x%lx\n",srcCRC);
      errLogRet(LOGIT,debugInfo, "flashCommit:  Calc CRC - 0x%lx\n",dstCRC);
      free(xferfileBuffer);
      return(-1);
   }
   free(xferfileBuffer);
   ffsBlinkLED(5, 10, 0x30);
   rmFF(updatefilename);
   ffsBlinkLED(5, 10, 0x20);
   rmFF(origbackup);
   /* ffdel(updatefilename); */
   /* ffdel(origbackup); */
   DPRINT1(-1,"flashCommit: creation of '%s' Complete\n\n",filename);
   ffsBlinkLED(5, 10, 0x00);
   return(0);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#else // USE_COMPRESS

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* Compression routines
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
extern int compress(char *dest,   unsigned long *destLen, char *source, unsigned long sourceLen);
extern int uncompress(char *dest, unsigned long *destLen, char *source, unsigned long sourceLen);

int flashUpdateFunc(char *filename,int size,char *md5sig)
{
   int i,result;
   char  updatefilename[128];
   char hex_output[16*2 + 1];
   char fileMD5[16*2 + 1];
   char cmprsMD5[16*2 + 1];
   char *xferfileBuffer;
   tcrc calcChkSum,srcCRC,dstCRC;
   unsigned long nSize;
   unsigned long cmpLen;
   long lSize;
   char *cmprsFileBuffer;
   char msgstring[256];

   lSize = (long) size;
  
   DPRINT2(-1,"flashUpdate: file: '%s', size: %lu\n",filename,lSize);
   DPRINT1(-1,"flashUpdate: MD5: '%s'\n",md5sig);

   /* if not a RF board and file is icat type then don't transfer */
   // DPRINT2(-1,"flashUpdate: BrdType: %d (1=RF), strstr: 0x%lx \n",BrdType, strstr(filename,"icat"));
   if ((BrdType !=  RF_BRD_TYPE_ID) && (strstr(filename,"icat") != NULL))
   {
        ffsBlinkLED(5, 10, 0x20);
        DPRINT1(-1,"flashUpdate: No Need to transfer icat file to non RF Controller: '%s'\n",filename);
        return(0);
   }

   if ( (result = ffmd5(filename,fileMD5)) == 0) 
   {
      /* test present file md5 to file tobe loaded md5 */
      if (strcmp(fileMD5,md5sig) == 0)
      {
          ffsBlinkLED(5, 10, 0x20);
          DPRINT1(-1,"flashUpdate: No Need to transfer: '%s'\n",filename);
          DPRINT1(-1,"flashUpdate: Update file is same as FFS file: '%s'\n",filename);
          return(0);
      }
   }


   xferfileBuffer = (char*) malloc(lSize);
   if ( (xferfileBuffer == NULL) )
   {
       errLogRet(LOGIT,debugInfo, "flashUpdate:  ****** ERROR: Malloc of xferFile buffer (%ld) failed\n",lSize);
       blinkLED(1,3); /* fail mode */
       return -1;
   }

   cmprsFileBuffer = (char*) malloc(lSize);
   if ( (cmprsFileBuffer == NULL) )
   {
       errLogRet(LOGIT,debugInfo, "flashUpdate:  ****** ERROR: Malloc of cmprsFile buffer (%ld) failed\n",lSize);
       free(xferfileBuffer);
       blinkLED(1,3); /* fail mode */
       return -1;
   }

   memset(xferfileBuffer,0,lSize);
   memset(cmprsFileBuffer,0,lSize);

   ffsBlinkLED(5, 10, 0x20);
   result = getFileViaTFTP(filename, size, xferfileBuffer);
   if ( result <= 0 )
   {
       free(xferfileBuffer);
       free(cmprsFileBuffer);
       blinkLED(1,3); /* fail mode */
       return -1;
   }
   result = calcMD5sig(xferfileBuffer, size, hex_output);
   // srcCRC = calcChkSum = addbfcrc(xferfileBuffer, size);
   /* DPRINT1(-1,"flashUpdate:   MD5 Calc Digest: '%s'\n",hex_output); */
   /* DPRINT2(-1,"flashUpdate: CRC32 Calc ChkSum: %lu [0x%lx]\n",calcChkSum,calcChkSum); */
   if (strcmp(hex_output, md5sig)) 
   {
      errLogRet(LOGIT,debugInfo, "flashUpdate():  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "flashUpdate:  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "flashUpdate:  Calc MD5 - '%s'\n",hex_output);
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   DPRINT(+1,"flashUpdate: Transfer From Host to Controller Memory Successful\n");
   DPRINT(+1,"flashUpdate: Transfer File to FFS\n");
   ffsBlinkLED(5, 10, 0x30);

   /* compress the file for flash storage */
   DPRINT(-1,"flashUpdate: Compression Start\n");
   cmpLen = lSize;
   result = compress(cmprsFileBuffer, &cmpLen, xferfileBuffer, (unsigned long) lSize);
   if (result != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "flashUpdate():  ****** ERROR: Compression Failed: result: %d\n",result);
      blinkLED(1,3); /* fail mode */
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      return(-1);
   }
   DPRINT2(-1,"flashUpdate: Compression Complete, Deflated from %d to %d bytes\n",size,cmpLen);

   /* write compressed file onto Flash */
   sprintf(updatefilename,"%s.update.z",filename);
   result =  cpBuf2FF(updatefilename,cmprsFileBuffer,cmpLen);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashUpdate():  ****** Copy to Flash FileSystem failed\n");
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   ffsBlinkLED(5, 10, 0x38);

   // compare md5 of buffer and actual compressed file written
   calcMD5sig(cmprsFileBuffer, cmpLen, cmprsMD5);
   if ( (result = ffmd5(updatefilename,fileMD5)) == 0) 
   {
      /* test present file md5 to file tobe loaded md5 */
      if ( strcmp(fileMD5,cmprsMD5) )
      {
        errLogRet(LOGIT,debugInfo, "flashUpdate:  ****** ERROR: FFS Compress File MD5 Checksum do NOT match\n");
        errLogRet(LOGIT,debugInfo, "flashUpdate:  True MD5 - '%s'\n",cmprsMD5);
        errLogRet(LOGIT,debugInfo, "flashUpdate:  File MD5 - '%s'\n",fileMD5);
        free(xferfileBuffer);
        free(cmprsFileBuffer);
        blinkLED(1,3); /* fail mode */
        return(-1);
      }
   }

   /* save the file name with in the msgQ */
   sprintf(msgstring,"%s,%ld,%s",filename,size,md5sig);
   msgQSend(pFileTransferedQueue, msgstring, strlen(msgstring)+1, NO_WAIT, MSG_PRI_NORMAL);

   DPRINT1(-1,"flashUpdate() creation of '%s' Complete\n",updatefilename);
   free(xferfileBuffer);
   free(cmprsFileBuffer);
   return(0);
}


int flashCommitFunc(char *filename,int size,char *md5sig)
{
   int i,result;
   char  updatefilename[128];
   char  origbackup[128];

   char hex_output[16*2 + 1];
   char fileMD5[16*2 + 1];
   char *xferfileBuffer;
   tcrc calcChkSum,srcCRC,dstCRC;
   unsigned long nSize,regSize;
   long cmprsSize;
   char *cmprsFileBuffer = NULL;
  
   DPRINT2(-1,"flashCommit: file: '%s', size: %lu\n",filename,size);
   DPRINT1(-1,"flashCommit: MD5: '%s'\n",md5sig);

   /* if not a RF board and file is icat type then don't transfer */
   // DPRINT2(-1,"flashUpdate: BrdType: %d (1=RF), strstr: 0x%lx \n",BrdType, strstr(filename,"icat"));
   if ((BrdType !=  RF_BRD_TYPE_ID) && (strstr(filename,"icat") != NULL))
   {
        ffsBlinkLED(5, 10, 0x00);
        DPRINT1(-1,"flashUpdate: Skipping Commit for icat file to non RF Controller: '%s'\n",filename);
        return(0);
   }

   if ( (result = ffmd5(filename,fileMD5)) == 0) 
   {
      /* test present file md5 to file tobe loaded md5 */
      if (strcmp(fileMD5,md5sig) == 0)
      {
          ffsBlinkLED(5, 10, 0x00);
          DPRINT1(-1,"flashCommit: Skipping Commit for: '%s'\n",filename);
          DPRINT1(-1,"flashCommit: Update file is same as FFS file: '%s'\n",filename);
          return(0);
      }
   }

   xferfileBuffer = (char*) malloc(size);

   sprintf(updatefilename,"%s.update.z",filename);

   sprintf(origbackup,"%s.orig",filename);

   xferfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) )
   {
       errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: Malloc of cmprsFile buffer (%ld) failed\n",size);
       blinkLED(1,3); /* fail mode */
       return -1;
   }

   cmprsSize = getFFSize(updatefilename);
   if (cmprsSize < 1)   /* return file not present */
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: Update File '%s' is missing\n",updatefilename);
      free(xferfileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }

   cmprsFileBuffer = (char*) malloc(cmprsSize + 1024);
   if ( cmprsFileBuffer == NULL )
   {
       errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: Malloc of cmprsFile buffer (%ld) failed\n",
               cmprsSize + 1024);
      free(xferfileBuffer);
       blinkLED(1,3); /* fail mode */
       return -1;
   }

   /* transfer compressed FFS into cmprs buffer */
   ffsBlinkLED(5, 10, 0x30);
   result =  cpFF2Buf(updatefilename,cmprsFileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** Copy of Update Flash File to Memory failed\n");
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }

   DPRINT(-1,"flashUpdate: Decompression Start\n");
   regSize = size;
   result = uncompress(xferfileBuffer, &regSize, cmprsFileBuffer, nSize);
   if (result != Z_OK)
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: Decompression Failed: result: %d\n",result);
      blinkLED(1,3); /* fail mode */
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      return(-1);
   }
   DPRINT2(-1,"flashUpdate: Decompression Complete, Inflated from %d to %d bytes\n",nSize,regSize);

   result = calcMD5sig(xferfileBuffer, regSize, hex_output);

   DPRINT1(-1,"flashCommit: MD5 Calc Digest: '%s'\n",hex_output);
   /* DPRINT2(-1,"flashCommit: CRC32 Calc ChkSum: %lu [0x%lx]\n",calcChkSum,calcChkSum); */
   if (strcmp(hex_output, md5sig) != 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** ERROR: MD5 Checksum do NOT match\n");
      errLogRet(LOGIT,debugInfo, "flashCommit:  True MD5 - '%s'\n",md5sig);
      errLogRet(LOGIT,debugInfo, "flashCommit:  Calc MD5 - '%s'\n",hex_output);
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      blinkLED(1,3); /* fail mode */
      return(-1);
   }
   DPRINT(-1,"flashCommit: Transfer From FFS to Controller Memory Successful\n");

#ifdef USE_DELETE_NOT_RENAME

/* new way to save flash space, just delete file. */
   ffsBlinkLED(5, 10, 0x38);

   DPRINT1(-1,"Delete Original '%s'\n",filename);
   rmFF(filename);

   rmFF(updatefilename); // remove update file as well just about to write it out

#else

   /* got the file into memory OK, so backup origianl now */
   DPRINT2(-1,"Creating Backup of Original '%s' to '%s'\n",filename,origbackup);
   /* result = ffmv(filename,origbackup); */
   ffsBlinkLED(5, 10, 0x38);


   result = renameFF(filename,origbackup);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** Backup of Original File Failed.\n");
/*
      free(xferfileBuffer);
      return(-1);
*/
   }

#endif

   DPRINT(-1,"flashCommit: Transfer File to FFS\n");
   ffsBlinkLED(5, 10, 0x3E);
   result =  cpBuf2FF(filename,xferfileBuffer, regSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "flashCommit:  ****** Copy to Flash FileSystem failed\n");
      free(xferfileBuffer);
      free(cmprsFileBuffer);
      return(-1);
   }
   ffsBlinkLED(5, 10, 0x36);



   // compare md5 of buffer and actual file written
   if ( (result = ffmd5(filename,fileMD5)) == 0) 
   {
      ffsBlinkLED(5, 10, 0x32);
      /* test present file md5 to file that should be loaded md5 */
      if (strcmp(fileMD5,md5sig) != 0)
      {
        errLogRet(LOGIT,debugInfo, "flashUpdate:  ****** ERROR: FFS File MD5 Checksum do NOT match\n");
        errLogRet(LOGIT,debugInfo, "flashUpdate:  True MD5 - '%s'\n",md5sig);
        errLogRet(LOGIT,debugInfo, "flashUpdate:  File MD5 - '%s'\n",fileMD5);
        free(xferfileBuffer);
        free(cmprsFileBuffer);
        blinkLED(1,3); /* fail mode */
        return(-1);
      }
   }

   free(xferfileBuffer);
   free(cmprsFileBuffer);
   ffsBlinkLED(5, 10, 0x30);
   rmFF(updatefilename);
   ffsBlinkLED(5, 10, 0x20);
   rmFF(origbackup);
   /* ffdel(updatefilename); */
   /* ffdel(origbackup); */
   DPRINT1(-1,"flashCommit: creation of '%s' Complete\n\n",filename);
   ffsBlinkLED(5, 10, 0x00);
   return(0);
}

#endif /* USE_COMPRESS */

testIt(char *filename,int size)
{
   int result;
   unsigned long nSize;
   tcrc calcChkSum;
   char newfilename[128];
   char hex_output[16*2 + 1];
   char *fileBuffer;
   char *fileBuffer2;
   sprintf(newfilename,"%s.update",filename);
   fileBuffer = (char*) malloc(size);
   fileBuffer2 = (char*) malloc(size);
   result = getFileViaTFTP(filename, size, fileBuffer);
   DPRINT1(-1,"getFileViaTFTP() result: %d\n",result);
   if (result > 0)
   {
     result = calcMD5sig(fileBuffer, size, hex_output);
     DPRINT1(-1,"MD5 Calc Digest: '%s'\n",hex_output);
     calcChkSum = addbfcrc(fileBuffer, size);
     DPRINT2(-1,"CRC32 Calc ChkSum: %lu [0x%lx]\n",calcChkSum,calcChkSum);
     result =  cpBuf2FF(newfilename,fileBuffer, size);
     DPRINT1(-1,"cpBuf2FF: result: %d\n",result);
     result =  cpFF2Buf(newfilename,fileBuffer2,&nSize);
     result = calcMD5sig(fileBuffer2, size, hex_output);
     DPRINT1(-1,"MD5 Calc Digest: '%s'\n",hex_output);
     calcChkSum = addbfcrc(fileBuffer2, size);
     DPRINT2(-1,"CRC32 Calc ChkSum: %lu [0x%lx]\n",calcChkSum,calcChkSum);
   }
}


#ifdef NO_NDDS_YET

/*
 *   The NDDS callback routine, the routine is call when an issue of the subscribed topic
 *   is delivered. 
 *   called with the context of the NDDS task n_rtu7400
 *
 */
RTIBool Flash_DownldCallback(const NDDSRecvInfo *issue, NDDSInstance *instance,
                             void *callBackRtnParam)
{
    Flash_Downld *recvIssue;
    FLASHFILE_UPDATEINFO flashInfo;
    char *pDst;
    int bufcmd;
    long diff;

    if (issue->status != NDDS_FRESH_DATA)
    {
       /* DPRINT(-1,"---->  Flash_DownldCallback called, issue not fresh  <------\n"); */
       return RTI_TRUE;
    }

    /* new file */
    if (recvIssue->dataOffset == 0) 
    {
       DPRINT(-1,"New file malloc space\n");
       pData_Bufferr = ( char *) malloc(recvIssue->totalBytes);
       flashInfo.bufferAddr = (long) pData_Bufferr;
       strncpy((flashInfo.filename),recvIssue->filename,FLASH_MAX_STR_SIZE);
       strncpy((flashInfo.backupname),recvIssue->backupname,FLASH_MAX_STR_SIZE);
       strncpy((flashInfo.md5sig),recvIssue->md5sig,FLASH_MAX_STR_SIZE);
       strncpy((flashInfo.msgstr),recvIssue->msgstr,FLASH_MAX_STR_SIZE);
       flashInfo.totalBytes = recvIssue->totalBytes;
       flashInfo.crc32chksum = recvIssue->crc32chksum;
    }

     recvIssue = (Flash_Downld *) instance;
     DPRINT4(-2,"Downld CallBack:  sn: %ld, totalBytes: %ld, datalen: %d, crc: 0x%lx\n",
        recvIssue->sn, recvIssue->totalBytes, recvIssue->data.len,recvIssue->crc32chksum); 

      panelLedOn(DATA_XFER_LED);

      pDst = (char*) (((unsigned long) pData_Bufferr) +  recvIssue->dataOffset);
      diff = ((unsigned long) pDst) - ((unsigned long) pData_Bufferr);
      DPRINT3(1,"dst strt: 0x%lx, cp@: 0x%lx, diff: %d\n", pData_Bufferr, pDst, diff);
      memcpy(pDst, recvIssue->data.val,recvIssue->data.len);
      /* ack issue */
      if ((recvIssue->data.len + recvIssue->dataOffset) == recvIssue->totalBytes)      
      {
         unsigned long calccrc;
         DPRINT1(-1,"Transfer completed: %d bytes\n",recvIssue->totalBytes);
         /* calccrc = addbfcrc(pDwnLdBuf->data_array, issue->totalBytes);
	 DPRINT2(1,"recv'd crc: 0x%lx, calc crc: 0x%lx\n",issue->crc32chksum, calccrc); */
         if ( msgQSend(pFlashMsgQ, (char*) &(flashInfo), sizeof(FLASHFILE_UPDATEINFO), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
         {
		/* error */
         }
      }

      panelLedOff(DATA_XFER_LED);

   return RTI_TRUE;
}

/*
 * When we get the time this is what has to happen.
 * We need to create a reliable publication and subscript for the flash download.
 *   See flash_Downld.x
 * The call back mallocs space then fills it, when complete it sends the info msge to 
 * the awaitinf task (pend on msgQ)
 * This task will copy it into flash and check crc32 and md5 signitures
 * Send an acknowledgement with errorcode if failure
 *   Greg Brissey 12/01/2004
 */

startFlashUpdate()
{
    /* create a MsgQ to place the waiting buffer beginning address to be read */
     pFlashMsgQ = msgQCreate(20, sizeof(FLASHFILE_UPDATEINFO), MSG_Q_FIFO);
}
#endif   /* NO_NDDS_YET */

/*   
 * test routines to check out FTP file transfer
 * procedure:
 * 1st. createFTPDev() to create and appropraite FTP device
 *       otherwise rsh would be used.
 * 2nd setUserPassword() to user name and their password to allow the FTP transfer to happen
 *     FTP requires a password and user account
 * 3rd.  getFileViaFTP("/tftpboot/ddrexec.o") for transfer
 */
createFTPDev()
{
  netDevCreate(remoteFTPDev,"wormhole",1);
}

setUserPassword(char *user, char *password)
{
  iam(user,password);
}

getFileViaFTP(char *filepath)
{
    char confilepath[180];
    char buffer[8192];
    int result;
    int fd;
   struct stat fileStats;
   unsigned long fileSizeBytes,bytes;

    sprintf(confilepath,"%s%s",remoteFTPDev,filepath);
    /* fd = open(confilepath,2); */
   /* First check to see if the file system is mounted and the file     */
   /* can be opened successfully.                                       */
   fd = open(confilepath, O_RDONLY, 0444);
   if (fd < 0)
   {
        printf("input file '%s', could not be open\n",confilepath);
        return(-1);
   }
 
   if (fstat(fd,&fileStats) != OK)
   {
     printf("failed to obtain status infomation on file: '%s'\n",confilepath);
     close(fd);
     return(-1);
   } 
 
  /* make sure it's a regular file */
   if ( ! S_ISREG(fileStats.st_mode) )
   {
      printf("Not a regualr file.\n");
      close(fd);
      return(-1);
   }  
 
   fileSizeBytes = fileStats.st_size;


    result = 1;
    while( result > 0)
    {
       result = read(fd,buffer,8192);
       printf(" result = %d\n",result);
    }
    
}

int ffmd5(char *filename, char *md5str)
{
   long size,result;
   unsigned long nSize;
   char hex_output[16*2 + 1];
   char *xferfileBuffer;

   if (filename == NULL)
      return -1;

   if (strlen(filename) < 1)
      return -1;

   size = getFFSize(filename);
   if (size < 1)   /* return file not present */
    return(-1);

   xferfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) )
       return -1;

   /* transfer FFS into buffer */
   result =  cpFF2Buf(filename,xferfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "ffmd5():  ****** Read from Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }

    result = calcMD5sig(xferfileBuffer, nSize, hex_output);
    if (md5str == NULL)
    {
       printf("MD5: '%s'\n",hex_output);
    }
    else
    {
        strcpy(md5str,hex_output);
    }
    free(xferfileBuffer);
    return(0); 
}


static int delayOn = 5;
static int  delayOff = 10;
int FFUledMaskBits = 0x3e;

ffsblinker()
{
   panelLedAllOff();
   FOREVER
   {
      panelLedMaskOn(FFUledMaskBits);  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
      taskDelay(delayOn);
      /* panelLedMaskOff(FFUledMaskBits);  /* from top down, 1st is dedicated to FPGA LogicAnalyzer trigger */
      panelLedAllOff();
      taskDelay(delayOff);
   }
}

ffsBlinkLED(int on, int off, int mask)
{
   int taskPriority,taskOptions,stackSize;
   taskPriority = 65;
   taskOptions = 0;
   stackSize = 1024;

   delayOn = (on < 1) ? 1 : on;
   delayOff = (off < 3) ? 3 : off;
   FFUledMaskBits = mask;

  if (taskNameToId("tFFSBlinker") == ERROR)
     taskSpawn("tFFSBlinker",FFSUPDATE_TASK_PRIORITY-1,taskOptions,stackSize,ffsblinker,
                1,2,3,4,5,6,7,8,9,10);
}
adjFFSBlinkRate(int on, int off)
{
   delayOn = (on < 1) ? 1 : on;
   delayOff = (off < 3) ? 3 : off;
}

killFFSBlinker()
{
  int tid;

    if ((tid = taskNameToId("tFFSBlinker")) != ERROR)
      taskDelete(tid);
    panelLedAllOff();
}



/*
 * routine to copy memory from controller to host computer
 *
 *   Author: Greg Brissey     3/23/05
 */
int mem2host(char *memptr,unsigned long size_bytes,char *netname)
{
    FFSHANDLE newFile;
    UINT32 bufCnt;
    UINT32 rc;
    UINT8 buffer[1024];
    unsigned long buffersize;
    long bytesleft;
    char nfilename[128],flashname[128];

   int fd;
   unsigned long fileSizeBytes,bytes;
   unsigned long offset;
   offset = 0;

   /* First check to see if the file system is mounted and the file     */
   /* can be opened successfully.                                       */
   fd = open(netname, O_CREAT | O_RDWR, 0644);
   if (fd < 0)
   {
        printf("host file '%s', could not be created\n",netname);
        return(-1);
   }
 
   printf("Coping Memory from 0x%lx to 0x%lx,   %lu bytes to Host file '%s'\n",
        memptr,memptr+size_bytes-1,size_bytes,nfilename);
 
   bytes = write(fd,memptr,size_bytes);
   if (size_bytes != bytes)
   {
      printf("Incomplete write to host, only %ld bytes of %ld bytes written.\n",
            (unsigned long) bytes,(long) size_bytes);
      printf("Copy terminated, host file deleted.\n");
      close(fd);
      unlink(nfilename);
      return(-1);
   }
   close(fd);
   printf("Complete.\n");
   return(0);
}    
   
int cpflash2host()
{
   mem2host((char*) 0xFF000000,0x01000000,"/tftpboot/flash.image");
}

// test of compress, compresses a flash files and writes it out to FFS
// reads it back in, uncompresses it, then compares md5 to see if it all worked
// leaves compress file on FFS
tstcmprs(char* filename)
{ 
   int i;
   long size,result;
   char nfilename[128];
   char hex_output[16*2 + 1];
   char hex_output2[16*2 + 1];
   char *xferfileBuffer;
   char *cmprsfileBuffer;
   tcrc srcCRC,srccmpCRC,retvcmpCRC,retvCRC;
   unsigned long nSize,cmpLen;

   if (filename == NULL)
      return -1;

   if (strlen(filename) < 1)
      return -1;

   size = getFFSize(filename);
   if (size < 1)   /* return file not present */
    return(-1);

   xferfileBuffer = (char*) malloc(size);
   cmprsfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) || (cmprsfileBuffer == NULL))
       return -1;

   /* transfer FFS into buffer */
   result =  cpFF2Buf(filename,xferfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "ffmd5():  ****** Read from Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }
   DPRINT3(-5,"file: %s read into buffer, size: %ld, (%ld)\n",filename, size, nSize);

   srcCRC = addbfcrc(xferfileBuffer, size);
   DPRINT1(-5,"File CRC 0x%lx\n\n",srcCRC);
   calcMD5sig(xferfileBuffer, nSize, hex_output);
   DPRINT(-5,"Compression Start\n");
   cmpLen = size;
   result = compress(cmprsfileBuffer, &cmpLen, xferfileBuffer, size);
   // result = deflate(xferfileBuffer,cmprsfileBuffer, &cmpLen, xferfileBuffer, size);
   if (result != Z_OK)
   {
      DPRINT1(-5,"compression error: %d\n",result);
   }
   DPRINT2(-5,"Compression Complete, Deflated from %d to %d bytes\n",size,cmpLen);
   srccmpCRC = addbfcrc(cmprsfileBuffer, cmpLen);
   DPRINT1(-5,"Cmpressed File CRC 0x%lx\n\n", srccmpCRC);

   sprintf(nfilename,"%s.z",filename);
   result =  cpBuf2FF(nfilename,cmprsfileBuffer, cmpLen);
   DPRINT2(-5,"cpBuf2FF: result: %d, size: %ld\n",result,cmpLen);

   /* clear buffer */
   for(i=0; i < size; i++)
      xferfileBuffer[i] = cmprsfileBuffer[i] = 0;


   /* transfer FFS into buffer */
   // result =  cpFF2Buf(updatefilename,xferfileBuffer,&nSize);
   result =  cpFF2Buf(nfilename,cmprsfileBuffer,&nSize);
   DPRINT2(-5,"cpFF2Buf: result: %d, filesize: %ld\n",result,nSize);
   retvcmpCRC = addbfcrc(cmprsfileBuffer, nSize);
   DPRINT1(-5,"Cmprs CRC32: 0x%lx\n",retvcmpCRC);
   DPRINT(-5,"Decompression Start\n");
   size = nSize;
   result = uncompress(xferfileBuffer, &size, cmprsfileBuffer, nSize);
   DPRINT1(-5,"uncompress result: %d\n",result);
   if (result != Z_OK)
   {
      DPRINT1(-5,"decompression error: %d\n",result);
   }
   DPRINT2(-5,"Decompression Complete, Inflated from %d to %d bytes\n",nSize,size);
   retvCRC = addbfcrc(xferfileBuffer, size);
   DPRINT1(-5,"retvCRC: 0x%lx\n",retvCRC);
   calcMD5sig(xferfileBuffer, size, hex_output2);
   free(xferfileBuffer);
   free(cmprsfileBuffer);
   DPRINT4(-5,"CRC Orig: %lx, Cmp: %lx, RetvCmp: 0x%lx, Retv: 0x%lx\n",srcCRC,srccmpCRC,retvcmpCRC,retvCRC);
   DPRINT2(-5,"MD5 Orig: '%s, Companded MD5: '%s'\n",hex_output,hex_output2);
    return(0); 
}

// was try to compress on the host and decompress on the target
// unseccussful, probably need to remove the file headed created by deflate
tstdecmprs(char* filename)
{ 
   int i;
   long size,result;
   char nfilename[128];
   char hex_output[16*2 + 1];
   char hex_output2[16*2 + 1];
   char *xferfileBuffer;
   char *cmprsfileBuffer;
   tcrc srcCRC,srccmpCRC,retvcmpCRC,retvCRC;
   unsigned long nSize,cmpLen;

   if (filename == NULL)
      return -1;

   if (strlen(filename) < 1)
      return -1;

   size = getFFSize(filename);
   if (size < 1)   /* return file not present */
    return(-1);

   xferfileBuffer = (char*) malloc(size*4);
   cmprsfileBuffer = (char*) malloc(size);
   if ( (xferfileBuffer == NULL) || (cmprsfileBuffer == NULL))
       return -1;

   /* transfer FFS into buffer */
   result =  cpFF2Buf(filename,cmprsfileBuffer,&nSize);
   if (result < 0) 
   {
      errLogRet(LOGIT,debugInfo, "ffmd5():  ****** Read from Flash FileSystem failed\n");
      free(xferfileBuffer);
      return(-1);
   }
   DPRINT3(-5,"file: %s read into buffer, size: %ld, (%ld)\n",filename, size, nSize);
   DPRINT(-5,"Decompression Start\n");
   result = inflate(cmprsfileBuffer, xferfileBuffer, nSize);
   if (result != OK)
   {
      DPRINT1(-5,"inflate error: %d\n",result);
   }
#ifdef XXX
   result = uncompress(xferfileBuffer, &size, cmprsfileBuffer, nSize);
   DPRINT1(-5,"uncompress result: %d\n",result);
   if (result != Z_OK)
   {
      DPRINT1(-5,"decompression error: %d\n",result);
   }
   DPRINT2(-5,"Decompression Complete, Inflated from %d to %d bytes\n",nSize,size);
#endif
   calcMD5sig(xferfileBuffer, size, hex_output);
   DPRINT1(-5,"DeCompressed MD5: '%s'\n",hex_output);
   free(xferfileBuffer);
   free(cmprsfileBuffer);
   return 0;
}
