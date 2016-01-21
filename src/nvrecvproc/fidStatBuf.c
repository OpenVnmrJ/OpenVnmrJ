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


#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include "rngBlkLib.h"
#include "hostAcqStructs.h"
#include "fidStatBuf.h"

/*
modification history
--------------------
4-06-05,gmb  created
*/

/*
DESCRIPTION

 FID Stat Block Buffer for NDDS call back usage and the Pipeline 

*/

FIDSTATBUF_ID fidStatBufCreate(int number)
{
  FIDSTATBUF_ID   pFidStatObj;
  char *bufAddrs;
  int i;

  pFidStatObj = (FIDSTATBUF_ID) malloc(sizeof(FIDSTAT_BLKBUF)); /* create structure */
  if (pFidStatObj == NULL) 
     return (NULL);

  memset(pFidStatObj,0,sizeof(FIDSTAT_BLKBUF));

  
  pFidStatObj->numBuffers = number;
  pFidStatObj->bufSize = sizeof(FIDSTAT_BLKBUF);;
  pFidStatObj->pBlkRng = rngBlkCreate(number,"Fid Stat Blocks", 1);
  pFidStatObj->pBuffers = (FID_STAT_BLOCK*) malloc( number * sizeof(FID_STAT_BLOCK));

  bufAddrs = (char *) pFidStatObj->pBuffers;
  
  /* fill ring buffer with free list of buffer addresses */
  for(i=0;i<number;i++)
  {
     rngBlkPut(pFidStatObj->pBlkRng,&bufAddrs,1);
     bufAddrs += sizeof(FID_STAT_BLOCK);
  }
  
  return(pFidStatObj);
}

int fidStatPut(FIDSTATBUF_ID pFidStats, FID_STAT_BLOCK* *bufAddr)
{
   /* printf("put addr: 0x%lx\n",*bufAddr); */
   return( rngBlkPut(pFidStats->pBlkRng,(long*) bufAddr,1) );
}

int fidStatGet(FIDSTATBUF_ID pFidStats, FID_STAT_BLOCK* *bufAddr)
{
   int stat;
   stat = rngBlkGet(pFidStats->pBlkRng, (long*) bufAddr,1);
   /* printf("bufaddr: 0x%lx\n",bufAddr); */
   return( stat );
}

/* #define TEST_MAIN  */
#ifdef TEST_MAIN
void *reader_routine (void *arg)
{
   FIDSTATBUF_ID pFidStats = (FIDSTATBUF_ID) arg;
   int cnt, status;
   FID_STAT_BLOCK* buffer;
   int state;

   cnt = 0;
   while(1)
   {
       printf("get buffer: %d, ",++cnt);
       status = fidStatGet(pFidStats, &buffer);
       printf("stat: %d, addr: 0x%lx \n",status,buffer);
   }
}

main()
{
   FIDSTATBUF_ID pFidStats;
   FID_STAT_BLOCK* buffer;
   pthread_t  RdthreadId;
   int status,i;
   long bufcnt;
   buffer=(FID_STAT_BLOCK*) 0;
   /* create buffer object */
   pFidStats = fidStatBufCreate(10);
   rngBlkShow(pFidStats->pBlkRng,1);
   status = pthread_create (&RdthreadId,
               NULL, reader_routine, (void*) pFidStats);
   if (status != 0)
   {
        printf("pthread_create error\n");
         exit(1);
   }
   
   while(1)
   {
     sleep(2);
     printf("put in buf: %d\n",buffer++);
     fidStatPut(pFidStats, &buffer);
   }
}
#endif
