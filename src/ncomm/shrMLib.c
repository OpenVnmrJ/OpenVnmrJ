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
#include <sys/file.h>
#include <fcntl.h>
#include <sys/ipc.h>

#include <errno.h>

#include "errLogLib.h"
#include "shrMLib.h"
#include "semLib.h"

/*
modification history
--------------------
6-29-94,gmb  created
*/


/*
DESCRIPTION
This library provides a shared memory interface with a mutual
excusion semaphore.

Interface provided:

  SHR_MEM_ID  shrmCreate(char *filename,int keyid, unsigned long size);
  void shrmRelease(SHR_MEM_ID shrmid);
  char *shrmAddr(SHR_MEM_ID shrmid);
  void shrmTake(SHR_MEM_ID shrmid);
  void shrmTake(SHR_MEM_ID shrmid);
  void  shrmShow(SHR_MEM_ID shrmid);
 
 In creating a share memory segment between two processes, the
SAME filename and keyid MUST BE Used by both processes.

  Typical scenerio:

     shrmid = shrmCreate("/home/MyShrMemImage", 42,
	                   sizeof(my_struct_to_be_shared));
     shared_struct=(my_struct_to_be_shared)shrmAddr(shrmid);

     -- don't need to lock when reading single element --
     val = shared_struct->a_member_of_struct;

     -- but I do when writing to struct ---
     shrmTake(shrmid);  take mutex
     shared_struct->another_member_of_struct = newValue;
     shrmGive(shrmid);  give mutex back


*/

/**************************************************************
*
*  shrmCreate - Create and initialize Shared Memory.
*
* This routine creates a Shared Memory Segment with
* a mutual exclusion semaphore.  This shared memory is
* based on the mmap facilities of the OS's VM (virtual memory).
* As a result a filename is required which is where a 
* persistent memory image is retained.  Thus it is possible
* to retain the contents of the shared memory between 
* executions of a process using it.
* If the file is not to be retained, then deleted it after
* shrmRelease().
*  
*
* RETURNS:
* Shared Memory ID, else ERROR NULL
*
*       Author Greg Brissey 6/29/94
*/
SHR_MEM_ID  shrmCreate(char *filename,int keyid, unsigned long size)
/* char *filename - filename for the persistent memory image */
/* int keyid - key id used to generate System V key. */
/* unsigned long memSize - Memory Segment size needed. */
{
   register SHR_MEM_ID shrmid;
   key_t key;
   int perm;

   /* malloc space for SHR_MEM_ID object */
   if ( (shrmid = (SHR_MEM_ID) (malloc(sizeof(SHR_MEM_OBJ)))) == NULL )
   {
      errLogRet(ErrLogOp,debugInfo,"shrmCreate: SHR_MEM_OBJ malloc error\n");
      return(NULL);
   }

   perm = (keyid == SHR_EXP_INFO_KEY) ? O_RDONLY : (O_RDWR | O_CREAT);
   if ( (shrmid->shrmem = mOpen(filename,size,perm)) == NULL)
   {
// There are cases where the file may not exist and we don't want a
// confusing error message. Calling routine should handle NULL return
//
//    errLogRet(ErrLogOp,debugInfo,"shrmCreate: mOpen failed\n");
      free(shrmid);
      return(NULL);
   }
   shrmid->shrmem->newByteLen = size;   /* file size when closed */

   /* bring important parameters up on structure for easier access */
   shrmid->MemAddr = shrmid->shrmem->mapStrtAddr;  /* mmap file starting address */
   shrmid->MemPath = shrmid->shrmem->filePath;  /* mmap file path name */

   if ( (keyid == SHR_EXP_INFO_KEY) || (keyid == SHR_EXP_INFO_RW_KEY) )
   {
      shrmid->mutexid = -1;
   }
   else
   {
      key = 342500000 + keyid; /* obtain unique key for semiphore */

      if ( (shrmid->mutexid = semOpen(key, 1)) < 0)
      {
         errLogRet(ErrLogOp,debugInfo,"shrmCreate: semOpen failed\n");
         mClose(shrmid->shrmem);  /* unmap shared memory */
         free(shrmid);
         return(NULL);
      }
   }

   return(shrmid);
}

/**************************************************************
*
*  shrmRelease - Release a shared memory segment from process.
*
* This routine releases the shared memory from the process. The
* file of the shared memory image is retained. Delete file
* if it is not needed.
*
* RETURNS:
* VOID 
*
*       Author Greg Brissey 6/29/94
*/
void shrmRelease(SHR_MEM_ID shrmid)
/* MSG_Q_ID mid - Message Q ID */
{

   if (shrmid != NULL)
   {
      if (shrmid->mutexid > 0)
      {
         mClose(shrmid->shrmem);  /* unmap shared memory */
         semClose(shrmid->mutexid); /* close semaphore */
      }
      else
      {
         /* trick mClose into not trying to truncate the file */
         shrmid->shrmem->fileAccess = O_RDONLY;
         mClose(shrmid->shrmem);  /* unmap shared memory */
      }
      free(shrmid);
   }
   return;
}


/**************************************************************
*
*  shrmAddr - return start address of shared memory.
*
* This routine returns the start address of the shared memory.
* Cast the address returned to the desired structure type.
* 
* RETURNS:
* Shared Memory (char*) Address, or ERROR -1
*
*       Author Greg Brissey 6/29/94
*/
char *shrmAddr(SHR_MEM_ID shrmid)
/* SHR_MEM_ID shrmid - shared memory id */
{
    return(shrmid->shrmem->mapStrtAddr);
}



/**************************************************************
*
*  shrmTake - Take mutex semaphore for shared memory 
*
* This routine takes the semaphore for the shared memory.
* If the mutex semaphore has already been taken the process
* will block until it is released. Any process that has taken
* the mutex semaphore and prematurely exits will release it.
*
*
* RETURNS:
*  void
*
*       Author Greg Brissey 6/29/94
*/
void shrmTake(SHR_MEM_ID shrmid)
/* SHR_MEM_ID shrmid - shared memory id */
{
    semTake(shrmid->mutexid);
    return;
}
/**************************************************************
*
*  shrmGive - Give mutex semaphore for shared memory 
*
* This routine gives the semaphore for the shared memory.
*
*
* RETURNS:
*  void
*
*       Author Greg Brissey 6/29/94
*/
void shrmGive(SHR_MEM_ID shrmid)
/* SHR_MEM_ID shrmid - shared memory id */
{
    semGive(shrmid->mutexid);
    return;
}
/**************************************************************
*
*  shrmShow - show information about shared memory 
*
* This routine display the system state information of 
* the shared memory.
*
* RETURNS:
* void
*
*       Author Greg Brissey 6/29/94
*/
void  shrmShow(SHR_MEM_ID shrmid)
/* SHR_MEM_ID shrmid - Shared Memory Id */
{

   if (shrmid != NULL)
   {
      mShow(shrmid->shrmem);
   }

   return;
}



