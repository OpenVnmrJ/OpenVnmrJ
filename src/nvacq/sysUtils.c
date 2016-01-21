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
8-20-03,gmb  created
*/


#ifdef VXWORKS
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <taskLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <wvLib.h>

#include <rBuffLib.h>
#include "private/wvBufferP.h" 
#include "private/wvUploadPathP.h" 
#include "private/wvSockUploadPathLibP.h"

#include <inetLib.h>
#include <time.h>

/*     VxWorks symbol table includes */
#include "sysSymTbl.h"
#include <symLib.h>
#include "logMsgLib.h"
#ifdef SILKWORM
#include "ioLib.h"
#include "stat.h"
#include "vTypes.h"
#include "md5.h"
#endif  //SILKWORM
#else
#ifndef VNMRS_WIN32
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>
#else    /* native windows */
#include <Winsock2.h>
#include <ws2tcpip.h>
#endif
#include "errLogLib.h"
#endif

#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


/*
   Obtain the the local IP number of this node
   e.g. 10.0.0.4
*/
#ifndef VXWORKS

/* default hostname, typeical installation uses wormhole for name */
static char  *defaultHostname = "wormhole";


/*
 *   Obtain the NIC or hostname that the console is attached, default is 'wormhole'
 *
 */
char *get_console_hostname(void)
{
        char    *tmpaddr;

        tmpaddr = (char *) getenv( "NIRVANA_CONSOLE" );
        if (tmpaddr == NULL) {
#ifdef DEBUG
                errLogRet(ErrLogOp,debugInfo, "Warning: NIRVANA_CONSOLE environment parameter not defined\n" );
                errLogRet(ErrLogOp,debugInfo, "Using default gateway hostname: '%s'\n" ,defaultHostname);
                errLogRet(ErrLogOp,debugInfo, "Define this parameter and restart the program if host differs from default\n" );
#endif
                tmpaddr = defaultHostname;
        }
        return(tmpaddr);
}


/*
   Solaris varient
   Obtain the IP for host given 
   e.g. getHostIP("wormhole",IPstr);
   e.g. 10.0.0.4
*/
#ifndef VNMRS_WIN32
char *getHostIP(char* hname, char *localIP)
{
   int ipval;
   struct in_addr in;
   struct hostent *hp;
   char **p;
 
   if ( (hp = gethostbyname(hname)) == NULL) {
     fprintf(stderr, "error in getting hostname\n");
     return(NULL);
   }
   p = hp->h_addr_list;
   memcpy(&in.s_addr, *p, sizeof (in.s_addr));
   strcpy(localIP,inet_ntoa(in));
   return(localIP);

}

#else   /* native Win32 */
char *getHostIP(char* hname, char *localIP)
{
   struct in_addr in;
   struct hostent *hp;
   char **p;
   int err;

   WORD wVersionRequested;
   WSADATA wsaData;

   wVersionRequested = MAKEWORD( 1,1);

   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 ) {
       /* Tell the user that we could not find a usable */
       /* WinSock DLL.                                  */
       fprintf(stderr,"error %d in WSAStartup(), misssing WinSock ddl?\n",err);
       return(NULL);
   }

   if ( (hp = gethostbyname(hname)) == NULL) {
           err = WSAGetLastError();
       fprintf(stderr, "error %d in getting hostname %s \n", err, hname);
       return(NULL);
   }
   p = hp->h_addr_list;
   memcpy(&in.s_addr, *p, sizeof (in.s_addr));
   strcpy(localIP,inet_ntoa(in));
   return(localIP);

}

#ifdef GREGWAY
char *getHostIP(char* hname, char *localIP)
{
   // Declare and initialize variables.
   struct in_addr *in;
   struct sockaddr_in *sockadr;
   char* ip = "wormhole";
   char* port = "";
   struct addrinfo aiHints;
   struct addrinfo *aiList = NULL;
   int retVal;

   //-------------------------------
   // Setup the hints address info structure
   // which is passed to the getaddrinfo() function
   memset(&aiHints, 0, sizeof(aiHints));
   aiHints.ai_family = AF_INET;
   aiHints.ai_socktype = SOCK_STREAM;
   aiHints.ai_protocol = IPPROTO_TCP;

   //--------------------------------
   // Call getaddrinfo(). If the call succeeds,
   // the aiList variable will hold a linked list
   // of addrinfo structures containing response
   // information about the host
   if ((retVal = getaddrinfo(ip, port, &aiHints, &aiList)) != 0) {
      return(NULL);
    }
    sockadr = (struct sockaddr_in *) aiList->ai_addr;
	in = &(sockadr->sin_addr);
    strcpy(localIP,inet_ntoa(*in));
    return(localIP);
}
#endif // GREGWAY

#endif   /* end of native WIN32 */

#else  /* ++++++++++++++++  VxWorks +++++++++++++++++++++++++++ */

extern int      sysTimerClkFreq;  /* Timer clock frequency e.g. 333.3 MHz = 333333333 */

/* typedefs */
typedef int (*PFIentry)();

/*
 *  Find and Exec a function in the System Symbol Table
 *
 */
int execFunc(char *funcName, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, void *arg7, void *arg8)
{
   int stat;
   char *addr,type;
   PFIentry   pFuncEntry;

   /* function name in the sysmbol table, obtain entry point if present */
   stat = symFindByName(sysSymTbl,funcName,&addr,&type);
   if ((stat != 0) || (addr == NULL))
    return(-1);

  DPRINT4(2,"execFunc()  stat: %d from sysFindByname('%s'): Addr: 0x%lx, Type: %d\n",stat,funcName,addr,type);
  pFuncEntry = (PFIentry) addr;
  stat =  (*pFuncEntry)(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
  return(stat);
}

/*
 * marksysclk() - return the value of system count up timer, 
 *                for high percision time measurements
 */
long long marksysclk()
{
  union 
  {
    long long lword;
    int  word[2];
  } test;
  vxTimeBaseGet(&(test.word[0]),&(test.word[1]));
  return(test.lword);
}

//=========================================================================
// deltatime: return delta-time (microseconds)
//=========================================================================
double deltaTime(long long end,long long start)
{  
    return ((double)(end-start))/333.3 ;
}  

static long long initialTimeStamp = 0LL;
// static long long DurationTimeStamp = 0LL;
static long long currentTimeStamp = 0LL;
static long long prevTimeStamp = 0LL;
static SEM_ID  pTimeStampMutex = NULL;;       /* Mutex Semaphore for Cntroller State Object */

void resetTimeStamp()
{
    initialTimeStamp = 0LL;
    return;
}


/*
 * updateTimeStamp(flag) - initial call pass 1 to set the initial timestamp used for duration times 
 *                         succesive call pas 0 to update the current and prev time for duration times
 *  roughly 3 usec to execute this function.
*/
void updateTimeStamp(int initflag)
{
    if (pTimeStampMutex == NULL) {
       pTimeStampMutex =  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    }
       
    if ((initflag == 1) || (initialTimeStamp == 0LL))
    {
       initialTimeStamp = currentTimeStamp = prevTimeStamp = marksysclk();
       // DurationTimeStamp = 0LL;
       // DPRINT3(-19,"updateTimeStamp(init): init: %lld, current: %lld, prev: %lld\n",initialTimeStamp,currentTimeStamp,prevTimeStamp);
    }
    else 
    {
       // don't delay the getting of the timestamp
       long long currentTempTime = marksysclk();
       semTake(pTimeStampMutex,WAIT_FOREVER);
         prevTimeStamp = currentTimeStamp;
         currentTimeStamp = currentTempTime;
         // DurationTimeStamp = DurationTimeStamp + (currentTimeStamp - prevTimeStamp); turns out to be the same as the diff from initialTimeStamp
         // currentTimeStamp = currentTempTime - correctionTime; not effective ~ 2 usec correction
       semGive(pTimeStampMutex);
       // DPRINT3(-19,"updateTimeStamp: init: %lld, current: %lld, prev: %lld\n",initialTimeStamp,currentTimeStamp,prevTimeStamp);
    }
    return;   
}

/*
 * returns the deltatime and total time from the updateTimeStamp() calls
 * roughly 4 usec to execute
*/
double getTimeStampDurations(double *deltatime, double *durationtime)
{
    
    double temp;
    *deltatime = deltaTime(currentTimeStamp,prevTimeStamp);
    *durationtime = deltaTime(currentTimeStamp,initialTimeStamp);
    // *durationtime = ((double)(DurationTimeStamp))/333.3 ;
     // diagPrint(debugInfo,"getTimeStampDurations: duration: %lf vs %lf usec\n",temp,*durationtime);
    
    return (*deltatime);
}

/*
*calibrateTS()
*{
*     int i;
*     long long start,end,delta,delta2,delta3;
*     double dtime,durtime;
*     char *strarg = "testing abcd123";
*     start = marksysclk();
*     // for(i=0;i < 100; i++)
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*        updateTimeStamp(0);
*     end = marksysclk();
*     delta = end - start;
*     // correctionTime = (long long) delta/10;
*     printf("updateTimeStamp X 10: delta time: %lld, %lf\n", delta, deltaTime(end,start));
*     start = marksysclk();
*     // for(i=0;i < 100; i++)
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*        getTimeStampDurations(&dtime,&durtime);
*     end = marksysclk();
*     delta2 = end - start;
*     printf("getTimeStampDurations X 10: delta time: %lld, %lf\n", delta2, deltaTime(end,start));
*     start = marksysclk();
*     // for(i=0;i < 100; i++)
*	     diagPrint(debugInfo,"%s delta: %lf usec, duration: %lf usec\n",strarg,dtime,durtime);
*     end = marksysclk();
*     delta3 = end - start;
*     printf("diagPrint: delta time: %lld, %lf\n", delta3, deltaTime(end,start));
*
*     start = marksysclk();
*
*     // for(i=0;i < 100; i++)
*        updateTimeStamp(0);
*        getTimeStampDurations(&dtime,&durtime);
*	     diagPrint(debugInfo,"%s delta: %lf usec, duration: %lf usec\n",strarg,dtime,durtime);
*
*     end = marksysclk();
*     delta = end - start;
*     printf("3 together delta time: %lld, %lf\n", delta, deltaTime(end,start));
*   
*     start = marksysclk();
*       TSPRINT("testing abcd123");
*     end = marksysclk();
*     delta = end - start;
*     printf("TSPRINT delta time: %lld, %lf\n", delta, deltaTime(end,start));
*
*     start = marksysclk();
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     TSPRINT("testing abcd123");
*     end = marksysclk();
*     delta = end - start;
*     printf("TSPRINT x 10 delta time: %lld, %lf\n", delta, deltaTime(end,start));
*     TSPRINT("testing marksysclk start");
*     start = marksysclk();
*     TSPRINT("testing marksysclk time");
*     start = marksysclk();
*     marksysclk();
*     end = marksysclk();
*     delta = end - start;
*     printf("marksysclk delta time: %lld, %lf\n", delta, deltaTime(end,start));
*}
*/

/*----------------------------------------------------------------------------
*    getFutureTime - get a time in usec in the future
*
*    used with  checkTime()
*
*    Used where we want a a timeout but with out giving up control
*    as would a taskDelay() might do.
*
*     Simple example
*
*     unsigned long ftop,fbot;
*
*    //  what time will it be 10 ms from now 
*    getFutureTime(10000,&ftop,&fbot);
*    while(!ready)
*    {
*        testBit(4);
*        // now avoid a hung system incase bit 4 never changes we add this 
*        // will test for 10msec then the time will be up and we will break out 
*        if (checkTime(ftop,fbot) == 1)
*            break;   // times up break out of loop 
*    }
*
*
*
*
*----------------------------------------------------------------------------*/
void getFutureTime(unsigned int usec, unsigned int *top, unsigned int *bot )
{
   /* for 405 @ 200 MHz */
   unsigned int advtime;

   advtime = usec * (sysTimerClkFreq / 1000000);
   vxTimeBaseGet(top,bot);
   *bot += advtime; 
}

/*----------------------------------------------------------------------------
*  checkTime
*      check to see if the time given has past
*      return 0 if not past
*      return 1 if time has past
*       see above for usage example
*----------------------------------------------------------------------------*/
checkTime(unsigned int top, unsigned int bot )
{
    unsigned int ptop, pbot;
    vxTimeBaseGet(&ptop,&pbot);

    /* present time top is greater than the given time then time is up */
    if (top < ptop)
	return(1);  /* time has past */

    /* present time bot is greater than the given time then time is up */
    if ( bot <= pbot)
     return(1);    /* time has past */

   /* time is not up */
   return(0);  
}

/*
 *  Simple busy loop delay, acurate +- 1 usec
 */
sysDelay(unsigned  int usecdelay)
{
     unsigned int ftop,fbot;
     getFutureTime(usecdelay, &ftop, &fbot );
     while (checkTime(ftop,fbot) != 1);  /* wait until time elapsed */
     return 0;
}

#ifdef VXWORKS

/* give time in mseconds how many tick is this equivilent to
 * for timeout and taskDelay usage
 * if msecond is below one tick one tick will be forced
 * to prevent delays of zero
 *
 *   Author: Greg Brissey 3/1/2006
 */
int calcSysClkTicks(int mseconds)
{
    int ticks;
    /* 10000 insterad of 1000 to keep resolution of 60 Hz clock of 16.6 msec */
    int msecPerTick =  10000 / sysClkRateGet();  /* mseconds per clock tick  for 60Hz clokc that 16.6 msec or 17 */
    /* printf("msecPerTick: %d\n",msecPerTick); */
    
    /*  bump mseconds by 10 to account for the 10000 above, and added 1/2 of the value to round up */
    ticks = ((mseconds*10) + (msecPerTick/2)) / msecPerTick;

    /* always return at least one tick */
    if (ticks < 1) 
       ticks = 1;
    return( ticks );
}

prtIntStack()
{
   extern char *vxIntStackBase;
   extern char *vxIntStackEnd;

   char *intStackBase = vxIntStackBase;
   char *intStackEnd  = vxIntStackEnd;
   register char  *pIntStackHigh;

   for (pIntStackHigh = intStackEnd; * (UINT8 *)pIntStackHigh == 0xee;
             pIntStackHigh ++)
             ;
   /* printf ("%-12.12s %-12.12s", "INTERRUPT", ""); */
        logMsg (" %8s %5d cur: %5d hi: %5d mar: %6d %s\n",
                 "",                                            /* task name */
                 (int)((intStackEnd - intStackBase) * -1 /* _STACK_DIR */),       /* stack size */
                 0,                                             /* current */
                 (int)((pIntStackHigh - intStackBase) * -1 /* _STACK_DIR */),
                                                          /* high stack usage */
                 (int)((intStackEnd - pIntStackHigh) * -1 /* _STACK_DIR */),     /* margin */
                 (pIntStackHigh == intStackEnd) &&              /* overflow ? */
                 (intStackEnd != intStackBase != 0) ? /* no interrupt stack ? */
                 "OVERFLOW" : "");

  return 0;
}


#ifdef SILKWORM

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
 * calculate the md5 signiture of the buffer content
 * GMB
 */
static int calcMd5(char *buffer,UINT32 len, char *md5sig)
{
   md5_state_t state;
   md5_byte_t digest[16];
   // char hex_output[16*2 + 1];
   int di;

   /* ---------------------------*/
   /* calc MD5 checksum signiture */
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)buffer, len);
   md5_finish(&state, digest);
   /* generate md5 string */
   for (di = 0; di < 16; ++di)
        sprintf(md5sig + di * 2, "%02x", digest[di]);
   /* ---------------------------*/
   return 0;
}


#define ICAT_DEV_PREFIX "icat"
#define FLASH_DEV_PREFIX "ffs:"
#define NFS_DEV_PREFIX "rsh:"

char *vsload(char *filepath, uint32_t *size, uint32_t *dataOffset )
{

  char *filename;
  char *buffer;

  *dataOffset = 0L;
   // printf("vsload: file: '%s' \n",filepath);
  if (strncmp(ICAT_DEV_PREFIX,filepath,sizeof(ICAT_DEV_PREFIX)-1) == 0)
  {
      char md5sig[36];
      int result;
      int rfType,index;
      char *colonptr;

      *dataOffset = 0L;

      rfType = execFunc("getRfType", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      if ( rfType != 1 )
      {
         printf("icat not available on this controller\n");
        *size = 0;
         return NULL;
      }

      colonptr = strstr(filepath,":");
      filename = colonptr + 1;
      // buffer = readIsfFile(filename,size,dataOffset);
      buffer = execFunc("readIsfFile", filename, size, dataOffset,NULL,NULL,NULL,NULL,NULL);
      // printf("icat- file: '%s', size: %ld, dataOffset: %ld, buffer: 0x%lx\n",filename, *size, *dataOffset, buffer);
      if (buffer == NULL)
      {
          printf("icat: '%s', could not be open\n",filename);
          return NULL;
      }
  }
  else if (strncmp(FLASH_DEV_PREFIX,filepath,sizeof(FLASH_DEV_PREFIX)-1) == 0)
  {
      FFSHANDLE fileHdl;
      unsigned int cnt;
      filename = filepath+sizeof(FLASH_DEV_PREFIX)-1;

      if ((fileHdl = vfOpenFile(filename)) == NULL)
      {
         printf("ffs: '%s', could not be open\n",filename);
        *size = 0;
        return NULL;
      }

      *size = vfFilesize(fileHdl);
      if ((buffer = (char *) malloc(*size)) == NULL)
      {
        vfCloseFile(fileHdl);
        *size = 0;
        return NULL;
      }

      vfReadFile(fileHdl,0,buffer,*size,&cnt);
      // printf("ffs:file: '%s', size: %ld, dataOffset: %ld, buffer: 0x%lx\n",filename, *size, dataOffset, buffer);
      if (cnt != *size)
      {
        printf("Incomplete read, only %ld bytes of %ld bytes read.\n",
                cnt,*size);
        free(buffer);
        vfCloseFile(fileHdl);
        *size = 0;
        return 0;
      }

      vfCloseFile(fileHdl);
  }
  else
  {
     int fd; 
     struct stat fileStats;
     unsigned long fileSizeBytes,bytes;

     if (strncmp(NFS_DEV_PREFIX,filepath,sizeof(NFS_DEV_PREFIX)-1) == 0)
       filename = filepath+sizeof(NFS_DEV_PREFIX)-1;
     else
       filename = filepath;

     *size = 0;

     /* First check to see if the file system is mounted and the file     */
     /* can be opened successfully.                                       */
     fd = open(filename, O_RDONLY, 0444);
     if (fd < 0)
     {
        printf("rsh: '%s', could not be open\n",filename);
        return(NULL);
     }

     if (fstat(fd,&fileStats) != OK)
     {
       printf("failed to obtain status infomation on file: '%s'\n",filename);
       close(fd);
       return(NULL);
     }

    /* make sure it's a regular file */
     if ( ! S_ISREG(fileStats.st_mode) )
     {
        printf("Not a regular file.\n");
        close(fd);
        return(NULL);
     }

     *size = fileStats.st_size;


     if ((buffer = malloc(*size+1)) == NULL)
     {
        printf("Memory allocation failed\n");
       *size = 0;
       close(fd);
       return NULL;
     }

     if ( (bytes = read(fd, buffer,*size)) < *size)
     {
        printf("Incomplete read, only %ld bytes of %ld bytes read.\n",
                bytes,*size);
        *size = 0;
        free(buffer);
        close(fd);
        return NULL;
     }
     // printf("rsh:file: '%s', size: %ld, dataOffset: %ld, buffer: 0x%lx\n",filename, *size, *dataOffset, buffer);
     buffer[*size] = '\0';
     close(fd);
  }
  return buffer;
}


int vswrite(char *filepath, char *buffer, uint32_t size )
{
  char *filename;
  int result;

  if (strncmp(ICAT_DEV_PREFIX,filepath,sizeof(ICAT_DEV_PREFIX)-1) == 0)
  {
      char md5sig[36],prefix[36];
      char tmpname[36],tmpmd5[36];
      int rfType,index,prefixlen,preindex,inuse,maxNumFiles;
      int maxSize;
      uint32_t tmplen;
      char *colonptr;

      index = 0;
      rfType = execFunc("getRfType", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      if ( rfType != 1 )
      {
         printf("icat not available on this controller\n");
         return -1;
      }
      colonptr = strstr(filepath,":");
      prefixlen = (int) (colonptr - filepath);
      strncpy(prefix,filepath,prefixlen);
      prefix[prefixlen]=0;
      index = execFunc("extractIndex",prefix, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      filename = colonptr + 1;
      maxNumFiles = execFunc("getISFMaxNumFiles", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      maxNumFiles = (maxNumFiles == -1) ? 5 : maxNumFiles;
      // note: index for new FFS layout will be -1 since it's not specified.
      if (index < (maxNumFiles - 1))   // really only useful for old ISF FFS layout
      {
         maxSize = execFunc("getISFFileMaxSize", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
         if (size > maxSize)
         {
             printf("file: '%s', size: %ld, exceeds max: %ld for icat file\n",filename,size,maxSize);
             return -1;
         }
      }
      calcMd5(buffer, size, md5sig);
      // check if the given index already has an file entry
      // inuse = execFunc("readConfigTableHeader",index, tmpname, tmpmd5, &tmplen,NULL, NULL, NULL, NULL);
      
      // make sure this file name is unique, or over writting a present file
      preindex = execFunc("getConfigTableByName",filename, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      // printf("vswrite-icat: preindex: %d\n",preindex);
      // printf("vswrite-icat: preindex: %d, inuse: %d\n",preindex,inuse);
      // if ((preindex != -1) || (inuse != -1))// name was found
      if (preindex != -1) // name was found
      {
         /*
         if ( (preindex != index) && (preindex != -1) ) // name exist but not for this index
         {
             printf("file: '%s', is not unique, this name is already used by index: %d\n",filename,preindex);
             printf("Copy aborted. \n");
             return(-1);
         }
         else  // over-writting
         */
         {
            char answer[32];
            int led_id, chars;
            led_id = ledOpen(fileno(stdin),fileno(stdout),10);
            /*
            if (inuse != -1) // the given index is already in use 
            {
                printf("Overwite file '%s' ? (y or n) ? ",tmpname);
            }
            else
            {
                printf("Overwite file? (y or n) ? ");
            }
            */
            printf("Overwite file? (y or n) ? ");

            chars = ledRead(led_id,answer,sizeof(answer));
            // printf("answer: %d -  '%s'\n",chars,answer);
            if (answer[0] == 'y')
            {
               // int writeIsfFile(char *name, char *md5, UINT32 len, char *buffer, int overwrite)
               result = execFunc("writeIsfFile", filename, md5sig, size, buffer, 1 , NULL, NULL, NULL);
               // result = execFunc("writeConfigTable", index, filename, md5sig, size, buffer, NULL, NULL, NULL);
               // printf("icat:file: result: %d, '%s', index: %d, md5: '%s', size: %ld, buff: 0x%lx\n", result, filename,index,md5sig,size,buffer);
            }
            else
            {
              printf("Copy aborted. \n");
              return(-1);
            }
         }
      }
      else
      {
        result = execFunc("writeIsfFile", filename, md5sig, size, buffer, 0 , NULL, NULL, NULL);
        // result = execFunc("writeConfigTable", index, filename, md5sig, size, buffer, NULL, NULL, NULL);
        // printf("icat:file: result: %d, '%s', index: %d, md5: '%s', size: %ld, buff: 0x%lx\n", result, filename,index,md5sig,size,buffer);
      }
      if (result == -2)
         printf("file: '%s', size: %ld, exceeds max: %ld for icat file\n",filename,size,maxSize);
      else if (result == -3)
         printf("No space left for file of %ld bytes\n",size);
  }
  else if (strncmp(FLASH_DEV_PREFIX,filepath,sizeof(FLASH_DEV_PREFIX)-1) == 0)
  {
     FFSHANDLE fileHdl;
     unsigned int cnt;
     uint32_t errorcode;
     uint32_t bufCnt;

     filename = filepath+sizeof(FLASH_DEV_PREFIX)-1;

     // printf("ffs:filename: '%s'\n",filename);
    /*  Check if file already exist on flash */
    if ((fileHdl = vfOpenFile(filename)) != 0)
    {
        char answer[32];
        int led_id, chars;
        led_id = ledOpen(fileno(stdin),fileno(stdout),10);
        printf("Overwite file? (y or n) ? ");
        chars = ledRead(led_id,answer,sizeof(answer));
        // printf("answer: %d -  '%s'\n",chars,answer);
        if (answer[0] == 'y')
        {
             /*  Close and delete file */
             sysFlashRW();
             vfAbortFile(fileHdl);
             sysFlashRO();    /* Flash to Read Only, via EBC  */
        }
        else /* option == 0 */
        {
           /*  Close file */
           vfCloseFile(fileHdl);
           printf("Copy aborted. \n");
           return(-1);
        }
        ledClose(led_id);
    }
    /* make flash writable, other wise an exception is thrown */
    sysFlashRW();

    /*  Create the file from scratch */
    fileHdl = vfCreateFile(filename, -1, -1, -1);
    if (fileHdl == 0)
    {
       errorcode = vfGetLastError();
       if (errorcode)
          vfFfsResult(filename);
       sysFlashRO();    /* Set Flash to Read Only, via EBC  */
       return(-1);
    }
  // unsigned long vfWriteFile(FFSHANDLE, unsigned char *, unsigned long, unsigned long *);
    errorcode = vfWriteFile(fileHdl, buffer, size, &bufCnt);
    if (errorcode != 0)
    {
        errorcode = vfGetLastError();
        if (errorcode)
          vfFfsResult(filename);
        /*  Close and delete file */
        vfAbortFile(fileHdl);
        sysFlashRO();    /* Flash to Read Only, via EBC  */
        return(-1);
    }
    if ( (bufCnt != size) )
    {
       printf("Incomplete write to flash, only %ld bytes of %ld bytes written.\n",
                   (unsigned long) bufCnt,size);
       /*  Close and delete file */
       vfAbortFile(fileHdl);
       sysFlashRO();    /* Flash to Read Only, via EBC  */
       return(-1);
    }
    vfCloseFile(fileHdl);
    sysFlashRO();    /* Set Flash to Read Only, via EBC  */
    result = 0;

    // printf("ffs:file: result: %d, '%s', size: %ld, buffer: 0x%lx\n",result, filename,size,buffer);
  }
  else 
  {
     int fd,bytes; 
     if (strncmp(NFS_DEV_PREFIX,filepath,sizeof(NFS_DEV_PREFIX)-1) == 0)
       filename = filepath+sizeof(NFS_DEV_PREFIX)-1;
     else
       filename = filepath;

     // printf("rsh:file: '%s'\n",filename);

    /* First check to see if the file system is mounted and the file     */
    /* can be opened successfully.                                       */
    fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd < 0)
    {
        printf("rsh: '%s' could not be created\n",filename);
        return(-1);
    }
    bytes = write(fd,buffer,size);
    if (bytes != bytes)
    {
       printf("Incomplete write to host, only %ld bytes of %ld bytes written.\n",
              (unsigned long) bytes,(long) size);
       close(fd);
       /*  Close and delete file */
       unlink(filename);
       return(-1);
    }
    close(fd);
    // printf("rsh:file: '%s', size: %ld, buffer: 0x%lx\n",filename,bytes,buffer);
    result = 0;
  }
  return result;
}

pcp(char *source, char *destpath)
{
   char *srcBuf, *dstBuf;
   uint32_t srcSize,dataOffset,dstSize,dstDataOffset;
   char srcMd5sig[36],dstMd5sig[36];
   int status;

   if ((source == NULL) || (destpath == NULL))
   {
      printf("Usage:  pcp \"dev:source_filename\",\"dev:dest_filename\"\n");
      printf("        where 'dev' can be: ffs - controller flash, rsh - host, icat - iCAT ISFlash]\n");
      printf("   e.g. pcp \"rsh:/home/vnmr1/myfile.txt\",\"ffs:myfile.txt\"\n");
      printf("        pcp \"rsh:/home/vnmr1/myfile.txt\",\"icat:myfile.txt\"\n");
      printf("        pcp \"ffs:myfile.txt\",\"rsh:/home/vnmr1/myfile.txt\"\n");
      printf("        pcp \"icat:myfile.txt\",\"ffs:myfile.txt\"\n");
      printf("   Note some devices may not be available on all controllers\n\n");
      return(-1);
   }
  
   srcBuf = vsload(source, &srcSize, &dataOffset );
   // printf("pcp: src: '%s', size: %ld, offset: %ld, Buff: 0x%lx\n", source,srcSize,dataOffset,srcBuf);
   if (srcBuf == NULL)
   {
     printf("Copy failed.\n");
     return -1;
   }

   status = vswrite(destpath, (srcBuf + dataOffset), srcSize);
   // printf("pcp: status: %d, dst: '%s', size: %ld, offset: %ld, Buff: 0x%lx\n", status, destpath,srcSize,dataOffset,(srcBuf+dataOffset));
   if (status != 0)
   {
     free(srcBuf);
     printf("Copy failed.\n");
     return -1;
   }

   dstBuf = vsload(destpath, &dstSize, &dstDataOffset );
   // printf("pcp: vsload: dst: '%s', size: %ld, offset: %ld, Buff: 0x%lx\n", destpath,dstSize,dstDataOffset,dstBuf);
   if (dstBuf == NULL)
   {
     free(srcBuf);
     printf("Copy failed.\n");
     return -1;
   }
   calcMd5((srcBuf + dataOffset), srcSize, srcMd5sig);
   calcMd5((dstBuf+dstDataOffset), dstSize, dstMd5sig);
   // printf("MD5 src: '%s', dst: '%s'\n",srcMd5sig,dstMd5sig);
   if ( strcmp(srcMd5sig,dstMd5sig) != 0)
   {
     free(srcBuf);
     free(dstBuf);
     printf("Verification failed.\n");
     printf("Copy failed.\n");
     return -1;
   }
   printf("Copy Successful.\n");
   free(srcBuf);
   free(dstBuf);
   return 0;
}

#endif  // SILKWORM

#ifndef WINDVIEW_33
/* use the Windview global already present, define in ../target/src/config/usrWindview.c */

/* globals */

extern int wvDefaultBufSize;           /* default size of a buffer */
extern int wvDefaultBufMax;            /* default max number of buffers */
extern int wvDefaultBufMin;            /* default min number of buffers */
extern int wvDefaultBufThresh;         /* default threshold for uploading, in bytes */
extern int wvDefaultBufOptions;        /* default rBuff options */

extern rBuffCreateParamsType   wvDefaultRBuffParams;  /* contains the above values */
/*     e.g. setup shown below */
/*    wvDefaultBufSize            = WV_DEFAULT_BUF_SIZE; 	*/
/*    wvDefaultBufMax             = WV_DEFAULT_BUF_MAX; 	*/
/*    wvDefaultBufMin             = WV_DEFAULT_BUF_MIN; 	*/
/*    wvDefaultBufThresh          = WV_DEFAULT_BUF_THRESH; 	*/
/*    wvDefaultBufOptions         = WV_DEFAULT_BUF_OPTIONS; 	*/
/*                                                          	*/
/*    wvDefaultRBuffParams.minimum          = wvDefaultBufMin; 	*/
/*    wvDefaultRBuffParams.maximum          = wvDefaultBufMax; 	*/
/*    wvDefaultRBuffParams.buffSize         = wvDefaultBufSize; */
/*    wvDefaultRBuffParams.threshold        = wvDefaultBufThresh; */
/*    wvDefaultRBuffParams.options          = wvDefaultBufOptions; */

/*    wvDefaultRBuffParams.sourcePartition  = memSysPartId; 	*/
/*    wvDefaultRBuffParams.errorHandler     = wvRBuffErrorHandler; */

extern BUFFER_ID         wvBufId;      /* event-buffer id used by wvOn/Off */
extern UPLOAD_ID         wvUpPathId;   /* upload-path id used by wvOn/Off */
extern WV_UPLOADTASK_ID  wvUpTaskId;   /* upload-task id used by wvOn/Off */
extern WV_LOG_HEADER_ID  wvLogHeader;  /* the event-log header */

TASKBUF_ID  taskBufId;

static int uploadPort = 6164;

DWvLog()
{
   printf("\nDeferred WindView logging and upLoad\n\n");
   printf("1. Initialize WindView Logging buffers: \n\t\twvDLogInitNoCon(level,enableNetworkEvts) level=1-3,enable...=0 or 1 \n");
   printf("2. Start logging: \n\t\twvDLogStrt() \n");
   printf("3. Stop logging: \n\t\twvDLogStp() \n");
   printf("4. Make Connection to Host IP & Port to upload windview data: \n\t\twvDLogConnect(int useport,char *hostIpStr) useport-6160-6199, 0=6164 default, hostIPStr e.g. \"172.16.0.1\"\n");
   printf("5. Upload data: \n\t\twvDLogSend() \n");

   /*printf("1. Initialize WindView Logging buffers: wvDLogInitNoCON(level,hostPort)\n\t\t level=1-3, hostport-6160-6199, 0=6164 default\n"); */
   return 0;
}

wvDLogInit(int level, int useport)
{
   wvDLogInitM(level, useport, 0);
   return 0;
}

wvDLogInitNoCon(int level, int netFlag)
{
   wvDLogInitM(level,0 , 1);
   if (netFlag != 0)
      wvNetEnable(0);
   return 0;
}

wvDLogInitM(int level, int useport, int delayedConnection)
{
  /* hostIP = "172.16.0.1"; */
  /* hostIP = "192.168.0.1"; */

  char hostIP[25];
  int class;
  int port;
     
   if (useport == 0)
   {
      port = 6164;
   }
   else if ( (useport < 6160) || (useport > 6199))
   {   
       printf("valid windview ports: 6160 - 6199\n");
       port = 6164;
   }   
   else
   {   
       port = useport;
   }   
   uploadPort = port;

   if ((level > 0) && (level < 4))
   {
      switch(level)
      {
         case 1:  class = WV_CLASS_1; /* (0x1); */ break;
         case 2:  class = WV_CLASS_2; /* (0x3); */ break;
         case 3:  class = WV_CLASS_3; /* (0x7); */ break;
         default: class = WV_CLASS_1; /* (0x1); */ break;
      }

   }
   else
   {
       printf("wvLog(level) level = 1,2, or 3\n");
       return 0;
   }

   if (delayedConnection == 0)
   {
     getRemoteHostIP(hostIP);
     printf("Logging on '%s' port %d\n",hostIP,port);
   }


   /* Create event buffer in memSysPart, yielding wvBufId. */ 
    wvDefaultRBuffParams.buffSize = 512*1024;
    wvDefaultRBuffParams.minimum = 4; 
    wvDefaultRBuffParams.maximum = 10;
    wvDefaultRBuffParams.options = RBUFF_WRAPAROUND | RBUFF_UP_DEFERRED; /* task names get lost */
    if ((wvBufId = rBuffCreate (& wvDefaultRBuffParams)) == NULL)
    {
      logMsg ("initWvDeferLog: error creating buffer.\n",0,0,0,0,0,0);
        return (ERROR);
    }

   if (delayedConnection == 0)
   {
     if ((wvUpPathId = sockUploadPathCreate ((char *) hostIP,

                                            (short)(htons (port)))) == NULL)
     {
        logMsg ("wvOn: error creating upload path(ip=%s port=%d).\n",
                hostIP, port,0,0,0,0);
        rBuffDestroy (wvBufId);
        return (ERROR);
     }
   }

    /*
    * Initialize the event log, and let event logging routines know which
    * buffer to log events to.
    */  
    wvEvtLogInit (wvBufId);

    taskBufId = wvTaskNamesPreserve(memSysPartId,256);

    /*
     * Capture a log header, including task names active in the system.
     */
 
    if ((wvLogHeader = wvLogHeaderCreate (memSysPartId)) == NULL)
    {
        logMsg ("wvOn: error creating log header.\n",0,0,0,0,0,0);
        rBuffDestroy (wvBufId);
        /* sockUploadPathClose (wvUpPathId); */
        return (ERROR);
    }

    wvEvtClassSet (class); /* set to log class events */

    return 0;
}

/* eases post mortem windview logging */
pmStart()
{
  
   wvDLogInitNoCon(3,1);
   wvDLogStrt(); 
}
/* arg=1 to go to 199 else normal */
pmStop(int k)
{ 
   printf("evtRecv better be active!\n");
   wvDLogStp();
   if (k != 1) 
     wvDLogConnect(6164,"172.16.0.1");
   else
     wvDLogConnect(6164,"172.16.0.199");
   wvDLogSend();
}

wvDLogStrt()
{
    wvEvtLogStart();
    return 0;
}

wvDLogStp()
{
   wvEvtLogStop ();
    return 0;
}

wvDLogConnect(int useport,char *hostIpStr)
{
  char hostIP[25];
   int port;
   if (useport == 0)
   {
      port = 6164;
   }
   else if ( (useport < 6160) || (useport > 6199))
   {   
       printf("valid windview ports: 6160 - 6199\n");
       port = 6164;
   }   
   else
   {   
       port = useport;
   }   
   uploadPort = port;

   if (hostIpStr == NULL)
   {
      getRemoteHostIP(hostIP);
   }
   else if (strlen(hostIpStr) < 7)
   {
      getRemoteHostIP(hostIP);
   }
   else
   {
      strncpy(hostIP,hostIpStr,25);
   }
   printf("Logging on '%s' port %d\n",hostIP,port);

   if ((wvUpPathId = sockUploadPathCreate ((char *) hostIP,
                                            (short)(htons (port)))) == NULL)
   {
        logMsg ("wvOn: error creating upload path(ip=%s port=%d).\n",
                hostIP, port,0,0,0,0);
        rBuffDestroy (wvBufId);
        return (ERROR);
   }

   return 0;
}

wvDLogSend()
{
   /*
    * Upload the header
    */  
    if (wvLogHeaderUpload (wvLogHeader, wvUpPathId) != OK)
    {
        logMsg ("wvDLogSend: error uploading log header.\n",0,0,0,0,0,0);
        sockUploadPathClose (wvUpPathId);
        return (ERROR);
    }
    
    if ((wvUpTaskId = wvUploadStart (wvBufId, wvUpPathId, TRUE)) == NULL)
    {
        logMsg ("wvDLogSend: error launching upload.\n",0,0,0,0,0,0);
        rBuffDestroy (wvBufId);
        sockUploadPathClose (wvUpPathId);
        return (ERROR);
    }

    /*
     * Stop continuous upload of events.  This will block until the buffer
     * is emptied.
     */
 
    if (wvUploadStop (wvUpTaskId) == ERROR)
    {
        logMsg ("wvUploadStop: error in stopping upload task.\n", 0,0,0,0,0,0);
    }
 
    if ( wvTaskNamesUpload(taskBufId, wvUpPathId) == ERROR)
    {
        logMsg ("wvUploadStop: error in task name upload.\n", 0,0,0,0,0,0);
    }

    /* Close the upload path. */
    sockUploadPathClose (wvUpPathId);
 
    /* Destroy the event buffer. */
    if (wvBufId != NULL)
        rBuffDestroy (wvBufId);

    return 0;
}

wvDLogStpSend()
{
  wvDLogStp();
  wvDLogSend();
  return 0;
}

#endif  // XXXXX

#define SNTPC

#ifdef SNTPC

/* set time via Netowrk TIme Protocol  (NTP) */
void setTimeNTP(char *NTPServerIP,int timeoutmsec)
{
    /* Assume the time is in GMT */

    struct timespec tp;
    int timeOutTicks;

    timeOutTicks = calcSysClkTicks(timeoutmsec);
    if ( sntpcTimeGet(NTPServerIP,timeOutTicks,&tp) != ERROR)
    {
       clock_settime (CLOCK_REALTIME, &tp);
       printf ("The new time is set to: ");
       date ();
    }
    else
        printf("setTimeNTP: sntpcTimeGet Timed out.\n");

   return;
}

setCntlrTime(int timeoutmsec)
{
    char hostIP[16];
    getRemoteHostIP(hostIP);
    setTimeNTP(hostIP,timeoutmsec);
}

#endif



#endif  /* VXWORKS */

/* #define APP_WRAP_MALLOC_FREE */
#ifdef APP_WRAP_MALLOC_FREE

static int mallocSuspendFlag = 0;
static int suspendTaskId = 0;

/*
*  to replace a library routine with your own you can use the wrapper
* 1) Use the linker option "--wrap symbol", you should be
*   able to link your ld() API without modifying the VxWorks
*   library.
*    Use a wrapper function for symbol. Any undefined reference to symbol will
*    be resolved to __wrap_symbol. Any undefined reference to __real_symbol will
*    be resolved to symbol.
*
*   This can be used to provide a wrapper for a system function. The wrapper
*   function should be called __wrap_symbol. If it wishes to call the system
*   function, it should call __real_symbol.
*/
/* diagnostic malloc */
void * __wrap_malloc (int c)  {
   void *pointer;
   int taskID,PC;
   taskID = taskIdSelf();  /* this task ID of the calling function */
   pointer = __real_malloc (c);
   if ((c == 48) || (c == 128))
   {
      PC = pc(taskID);
      printf("'%s': malloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
      ti(taskID);
      if (mallocSuspendFlag == 1)
      {
         suspendTaskId = taskID;
         printf("'%s': Suspending malloc task\n", taskName(taskID));
         taskSuspend(taskID);
      }
      /* tt(taskID); /* stack trace of calling function/task */
      printf("'%s': malloc called with %ld, return Addr: 0x%lx, PC=0x%lx\n", taskName(taskID), c, pointer,PC);
   }
   return pointer;
}

suspendMallocTask()
{
   mallocSuspendFlag = 1;
}

resumeMallocTask()
{
   mallocSuspendFlag = 0;
   taskResume(suspendTaskId);
}

int __wrap_free (int addr)  {
   int status;
   int taskID,PC;
   taskID = taskIdSelf();  /* this task ID of the calling function */
   status = __real_free (addr);
   PC = pc(taskID);
   printf("'%s': free called with Addr: 0x%lx, PC = 0x%lx\n", taskName(taskID), addr,PC);
   ti(taskID);
   /* tt(taskID); /* stack trace of calling function/task */
   printf("'%s': free called with Addr: 0x%lx, PC = 0x%lx\n", taskName(taskID), addr,PC);
   return status;
}
#endif

#endif
