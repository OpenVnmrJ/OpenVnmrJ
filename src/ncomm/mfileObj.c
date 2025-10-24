/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define _FILE_OFFSET_BITS 64

#ifndef VNMRS_WIN32    /* Defined for Native Windows compilation */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>

/* for routines that seek then use the internal mfile struct to obtain the offeset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 */
#ifdef THREADED
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>
#endif

#else    /* native Win32 includes */

#define THREADED
#include <Windows.h>
#include <stdio.h>
#include <fcntl.h>

#endif

#include "errLogLib.h"
#include "mfileObj.h"

static long sysPageSize(void);
/*
modification history
--------------------
12-21-93,gmb  created 
*/

/*
DISCRIPTION

MMAP file Library, provides facilities to handle files as if they were
in memory. The library provides a simplified interface to the OS VM
mmap facilities. These facilities consist of mOpen(), mClose(), mAdvise()
mShow().

The files mapped are handle as if they are in memory, the hooks to accessing
the file are provided by the parameter members of the structure pointed to
via the map descriptor (md). This structure consists of:

  md->mapStrtAddr - this is the starting address of 
		    the file. Do NOT alter this value.
                    It is needed to unmap the file.

  md->offsetAddr - this is the address that MAY be 
		   altered to the users needs.
		   It is initial set to the beginning of 
		   the file.

  md->byteLen - This is the byte length of the file when 
		it was open. 
		If it was a new file the size argument to
                mOpen is used to set this.

  md->mapLen - This is the mapped length of the file, 
	       mapped files MUST be mulitplies of the 
	       VM system page size.

  md->newByteLen - This is the new byte length of a file 
		   being written to.
		   It is the User's responcibility to 
		   update this to the correct value. This
		   value is used by mClose() to resize
		   the file to it's correct size.

  md->filePath - File path of file mmapped.

  md->multiMap - mmap can only map 2 Gbytes of a file. For files
                 larger than 2 Gbytes, they must be mapped in
                 sections.  This flag indicates whether multiple
                 mmaps will be needed.

  md->firstByte - offset to first byte of the current section of
                  file mapped in.  Only needed if md->multiMap=1.

To mmap a file:

  1. Open file(s) just as you would with open() with the 
 addition argument of size, see open() for options 
 (O_RDONLY,etc)
  2. Use Madvise() if accesses are random or sequencial, 
 normal is the standard access mode.
  3. Access file via offsetAddr parameter, updating newByteLen if needed.
  4. close file(s) when done.

E.G.

 MFILE_ID inmd,outmd;

 inmd = mOpen("inputfile",0,O_RDONLY);
 outmd = mOpen("outputfile",filesize,O_RDWR | O_CREAT);

 ---  access a structure from in -----------

 strtptr = (struct blockhead *) inmd->offsetAddr;
 strtptr->member_name = etc.

 ----- copy structure from in to out -------- 

 memcpy(outmd->offsetAddr,inmd->offsetAddr,(sizeof(struct blockheader)));

 ----- move past blockheader and update size ------ 
 inmd->offsetAddr += sizeof((struct blockheader));
 outmd->offsetAddr += sizeof((struct blockheader));
 outmd->newByteLen += sizeof((struct blockheader));

 --------- alter data ----------

 ptr1 = inmd->offsetAddr;
 ptr2 = outmd->offsetAddr;

 for (i=0; i < npts; i++)
   *ptr2++ = *ptr1++ / factor;

 ------ update size and address  -----------
 inmd->offsetAddr += sizeof(int) * npts;
 outmd->offsetAddr += sizeof(shorts) * npts;
 outmd->newByteLen += sizeof(shorts) * npts;
 

etc., etc., etc.,

For further details see: mmap(), madvise(), minmap(), msync(), mctl(), 
mprotect(), mincore()
*/


/*-------------------------------------------------------------
| MMAP file Object Public Interfaces
+-------------------------------------------------------------*/
/**
 * #define O_RDONLY        0                +1 == FREAD
 * #define O_WRONLY        1                +1 == FWRITE
 * #define O_RDWR          2                +1 == FREAD|FWRITE
 * #define O_APPEND        _FAPPEND
 * #define O_CREAT         _FCREAT
 * #define O_TRUNC         _FTRUNC
 * #define O_EXCL          _FEXCL
 *         O_SYNC          _FSYNC          not posix, defined below
 *         O_NDELAY        _FNDELAY        set in include/fcntl.h
 *         O_NDELAY        _FNBIO          set in 5include/fcntl.h
 * #define O_NONBLOCK      _FNONBLOCK
 * #define O_NOCTTY        _FNOCTTY
 **/
 
#define ROOT_UID 0
static  int  old_euid = -1;
static  int  old_uid = -1;

#ifndef VNMRS_WIN32  /* Varian's native windows define */

#define MAXMAP ((uint64_t) 1 << 31)

/*
MFILE_ID mOpen(char* filename,uint64_t size, int permit)
*/
/**************************************************************
*
*  mOpen - open a file and returns map discriptor (md)
*
*  mOpen - is used just like the open system call but instead a
* file descriptor (fd) a map descriptor is returned.
* This is actually a pointer to the mmapped file 
* Object Data Structure.
* Any file open for writing is expanded to the maximum
* file size for the OS (this does not use disk space). 
*  mClose() call will resize file based on newByteLen 
* parameter.
*
* RETURNS:
* MFILE_ID - if no error, NULL - if mallocing or open failed
*
*/ 
MFILE_ID mOpen(char *filename, uint64_t size, int permit)
{
   void *p;
   MFILE_ID mfileObj;
   int fd;
   struct stat s;
   mode_t old_umask;
   long pagesize;
   int ret __attribute__((unused));

   if (permit != O_RDONLY)
   {
      if (old_uid == -1)
      {
         old_uid = getuid();
      }
      if (old_uid == ROOT_UID)
      {
         if (old_euid == -1)
            old_euid = geteuid();
         ret = seteuid( old_uid );
      }
   }
   /* open file, thus checking most error possibilities of file create, etc. */
   old_umask = umask(000); /* clear file creation mode mask,so that open has control */
   if ((fd = open(filename,permit,0666)) < 0)
   {
// There are cases where the file may not exist and we don't want a
// confusing error message. Calling routine should handle NULL return
//
//     errLogSysRet(ErrLogOp,debugInfo,"mOpen: open of %s failed (%s)\n",
//                  filename,strerror(errno) );
     umask(old_umask);  /* restore umask control over open. */
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
       ret = seteuid( old_euid );
     return (NULL);
   }
   umask(old_umask);  /* restore umask control over open. */

   /* get file size */
   if (fstat(fd, &s))
   {
     errLogSysRet(ErrLogOp,debugInfo,"mOpen: stat of %s failed (%s)\n",
                  filename,strerror(errno) );
     close(fd);
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
        ret = seteuid( old_euid );
     return (NULL);
   }

   /* ------- malloc space for adc Object --------- */
   if ( (mfileObj = (MFILE_OBJ *) malloc( sizeof(MFILE_OBJ) ) ) == NULL )
   {
     errLogSysRet(ErrLogOp,debugInfo,"mOpen: malloc of Object struct failed\n");
     close(fd);
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
        ret = seteuid( old_euid );
     return(NULL);
   }

   mfileObj->byteLen = (uint64_t)(s.st_size);		/* actual file size */
   mfileObj->fileAccess = permit;	/* file permission */
   if ( (mfileObj->filePath = (char *) (malloc(strlen(filename) + 1))) == NULL )
   {
     close(fd);
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
        ret = seteuid( old_euid );
     free(mfileObj);
     return(NULL);
   }
   strcpy(mfileObj->filePath,filename);
   mfileObj->pageSize = pagesize = sysPageSize();

   /* Read Only file will stay same size */
   if (permit == O_RDONLY)
   {
      size = s.st_size;
   }
   else /* file may become bigger therefore mmap a large size */
   {
     /* Map to a 2 Gbyte file if size was 0 */
     if (size == (uint64_t) 0)
        size = MAXMAP - s.st_size - pagesize;
   }
   if (size <= MAXMAP)
   {
        mfileObj->mapLen = (size + pagesize - 1) & ~(pagesize - 1);
        mfileObj->multiMap = 0;
        mfileObj->fd = -1;
   }
   else
   {
        mfileObj->mapLen = (MAXMAP + pagesize - 1) & ~(pagesize - 1);
        mfileObj->multiMap = 1;
        mfileObj->fd = fd;
   }
   mfileObj->firstByte = 0;

   if (permit == O_RDONLY)
   {
     mfileObj->newByteLen = s.st_size; /* Read Only File stays same size */
     mfileObj->mmapProt = PROT_READ;
   }
   else
   {
     mfileObj->newByteLen = (uint64_t) 0;
     mfileObj->mmapProt = PROT_READ | PROT_WRITE;

     /* change file size, to accomodate writes that increase size */
     if ((off_t)size != s.st_size)
     {
       if (ftruncate(fd,(off_t)size) != 0)
       {
         errLogSysRet(ErrLogOp,debugInfo,"mOpen: sizing failed for %s (%s)\n",
                      filename,strerror(errno) );
         close(fd);
         if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
            ret = seteuid( old_euid );
         free(mfileObj->filePath);
         free(mfileObj);
         return(NULL);
       }
     }
     mfileObj->byteLen = size;		/* actual file size */
   }

/* for routines that seek then use the internal mfile struct to obtain the offeset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 */
#ifdef THREADED

   /* initialize the thread mutex */
   if ( pthread_mutex_init(&mfileObj->mutex, NULL) != 0)
   {
        free(mfileObj->filePath);
        free(mfileObj);
        if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
           ret = seteuid( old_euid );
        return(NULL);
   }
  
#endif

   if ( (p = mmap(0,mfileObj->mapLen,mfileObj->mmapProt,MAP_SHARED,
                  fd,(off_t) 0)) == (void *) -1 )
   {
     errLogSysRet(ErrLogOp,debugInfo,"mOpen: mmap of %s failed (%s)\n",
                  filename,strerror(errno) );
     close(fd);
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
        ret = seteuid( old_euid );
     free(mfileObj->filePath);
     free(mfileObj);
     return(NULL);
   }
   mfileObj->mapStrtAddr = p;
   mfileObj->offsetAddr = p;

   if (mfileObj->fd == -1)
      close(fd);
   if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
      ret = seteuid( old_euid );

   return( mfileObj );
}

/*
int mClose(MFILE_ID *mFileId)
*/
/************************************************
*
*  mClose - close the mmapped file 
*
*  mClose - if the file was open for writing then the member 
* parameter newByteLen (new_byte_len) is used to 
* truncate the file back down to the correct size.
* The file is then unmapped.
*
*   Note: It is the user's responsibility to correctly set 
*         the value of newByteLen.
*
* RETURNS:
*   0 - if no error, -1 - if error occurred
*
*/
int mClose(MFILE_ID mFileId)
{
   int fd,access;
   mode_t old_umask;
   int ret __attribute__((unused));

   if (mFileId == NULL)
      return(-1);

   if (mFileId->filePath == NULL)
      return(-1);

   /* Resize file to actual byte size (newByteLen) */
   if ( (mFileId->fileAccess != O_RDONLY) ) 
   {
     if (old_uid == -1)
        old_uid = getuid();
     if (old_uid == ROOT_UID)
     {
        if (old_euid == -1)
           old_euid = geteuid();
        ret = seteuid( old_uid );
     }
     if (mFileId->newByteLen != mFileId->byteLen)
     {
       /* clear file creation mode mask,so that open has control */
       old_umask = umask(000);
       if (mFileId->fd == -1)
       {
          /* better strip out O_TRUNC and O_CREAT before resizing since
	     1. We know the file is here
	     2. We sure don't want to trunc the file to Zero NOW!
          */
          access = mFileId->fileAccess & (~(O_CREAT | O_TRUNC));
          if (( fd = open(mFileId->filePath, access,  0666)) < 0)
          {
            errLogSysRet(ErrLogOp,debugInfo,"mClose: failed for %s (%s)\n",
                         mFileId->filePath,strerror(errno) );
          }
       }
       else
       {
          fd = mFileId->fd;
       }
       /* size file to amount written */
       if ((fd >= 0) && ftruncate(fd,mFileId->newByteLen))
       {
         errLogSysRet(ErrLogOp,debugInfo,"mClose: sizing failed for %s (%s)\n",
                      mFileId->filePath,strerror(errno) );
       }
       umask(old_umask);  /* restore umask control over open */
       close(fd);
       mFileId->fd = -1;
     }
   }
   if (mFileId->fd >= 0)
      close(mFileId->fd);

   (void) munmap(mFileId->mapStrtAddr,mFileId->mapLen);

   if ( (mFileId->fileAccess != O_RDONLY) && (old_uid == ROOT_UID) )
      ret = seteuid( old_euid );
   free(mFileId->filePath);
   free(mFileId);
   return(0);
}

/***************************************************
*
*  sysPageSize() - get the page size of the Operating
* 		   Systems VM
*/
static long sysPageSize()
{
#ifndef SOLARIS
     return((long) getpagesize());	/* BCD way */
#else 
     /* a POSIX call with a non standard arg */
     return( sysconf(_SC_PAGESIZE ));   /* Solaris way */
#endif 
}

/* Windows Cygwin doens't appear to have madvise() as a supported feature
 * since it is not used at this point just comment it out.
 * if this is alos true for INTERIX use the following construct
 * #if (!defined(CYGWIN) && !defined(__INTERIX)) 
*/

#ifndef CYGWIN

/*
int mAdvise(MFILE_ID md,int advice)
*/
/************************************************
*
*  mAdvise - Advise VM system about the mmapped file 
*	     access usage.
*
*  mAdvise - Advise the VM system about file access 
* to improve system performance.
*     MF_NORMAL - read data as it is accessed with read ahead, 
*	         cache data for a period of time.
*     MF_RANDOM - read in minimum amount of data 
*		  (no read ahead).
*     MF_SEQUENTIAL - Data likely to be accessed only once, 
*		  so free data as quickly as possible.
*
*  Note: This affect the entire file, if only parts of the file
*	 need to be altered then use madvise() directly.
*
* RETURNS:
*   0 - if no error, -1 - if error occurred
*
*/
int mAdvise(md,advice)
MFILE_ID md;
int advice;
{
  int stat;

  if (md != NULL)
     stat = madvise(md->mapStrtAddr,md->mapLen,advice); /* returns 0 if OK */
  else
     stat = -1;

  return(stat);
}

/************************************************
*
*  mAsync - schedules async writes all modified  copies  of  pages
*           over  the  range  [addr,  addr  +  len)  to  the  underlying 
*           hardware for the mmapped file.
*
*  Note: This affect the entire file, if only parts of the file
*	 need to be sync then use msync() directly.
*        However only modified page with be scheduled for writing.
*
* RETURNS:
*   0 - if no error, -1 - if error occurred
*
*/   

int mAsync(MFILE_ID md)
{
  int stat;

  if (md != NULL)
     stat = msync(md->mapStrtAddr,md->mapLen,MS_ASYNC); /* returns 0 if OK */
  else
     stat = -1;

  return(stat);
}

#endif

/************************************************
*
*  mSync - schedules sync writes all modified  copies  of  pages
*           over  the  range  [addr,  addr  +  len)  to  the  underlying 
*           hardware for the mmapped file.
*           does NOT return until all pages wrtten.
*
*  Note: This affect the entire file, if only parts of the file
*	 need to be sync then use msync() directly.
*        However only modified page with be scheduled for writing.
*
* RETURNS:
*   0 - if no error, -1 - if error occurred
*
*  maybe a future implimentation below
* int mSync(void *addr, size_t len, int flags) return ( msync(addr, len, MS_SYNC)) ;
*/   
int mSync(MFILE_ID md)
{
  int stat;
  if (md != NULL)
     stat = msync(md->mapStrtAddr,md->mapLen,MS_SYNC); /* returns 0 if OK */
  else
     stat = -1;

  return(stat);
}

/************************************************
*
*  mFidSeek - Set offsetAddr to the beginning of the
*	      blockheader for the requested FID.
*
*  md         - pointer to the mmaped file information.
*  fidNum     - requested FID number
*  headerSize - size, in bytes of the FID file header.
*               will be sizeof(struct datafilehead)
*  bbytes     - size, in bytes, of a FID and its block
*               header.
*
*  RETURNS:
*  0 for success, -1 for failure.
*/
int mFidSeek(MFILE_ID md, int fidNum, int headerSize, unsigned long bbytes)
{
    uint64_t fidoffset;
 
    if (fidNum < 1L)
    {
      errLogRet(ErrLogOp,debugInfo, "mFidSeek: Fid number < 1.\n");
      return(-1);
    }
    fidoffset = headerSize + ((uint64_t) bbytes * ((uint64_t) (fidNum - 1L)));
    /* test to make sure we're not pointer off the end of the file */
    if (fidoffset + (uint64_t) bbytes > md->byteLen )
    {
       errLogRet(ErrLogOp,debugInfo, 
	  "mFidSeek: Fid Number %d,  resulted in seek past end of file.\n",
	  fidNum);
       return(-1);
    }
    if ( fidoffset + (uint64_t) bbytes > md->newByteLen)
       md->newByteLen = fidoffset + (uint64_t) bbytes;

    if (!md->multiMap)
    {
       md->offsetAddr = md->mapStrtAddr + fidoffset;
    }
    else
    {
       if ((fidoffset + (uint64_t)bbytes > md->firstByte +
            md->mapLen) || (fidoffset < md->firstByte) )
       {
          void *p;

          (void) munmap(md->mapStrtAddr,md->mapLen);
          md->firstByte = fidoffset & ~(md->pageSize - 1);
          if ( (p = mmap(0,md->mapLen,md->mmapProt,MAP_SHARED,
                         md->fd, md->firstByte)) == (void *) -1 )
          {
             return(-1);
          }
          md->mapStrtAddr = (char *) p;
       }
       md->offsetAddr = md->mapStrtAddr;
       md->offsetAddr += fidoffset - md->firstByte;
    }

//    DPRINT4(1,"mFidSeek(): fid# = %d, offset = %lu(0x%lx), addr = 0x%lx\n",
//		fidNum, fidoffset, fidoffset, md->offsetAddr);
    return(0);
}

/* for routines that seek then use the internal mfile struct to obtain the offset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 *
 *         Author:  Greg Brissey    4/19/2005
 */
int mMutexLock(MFILE_ID md)
{
#ifdef THREADED
    int status;
    status = pthread_mutex_lock (&md->mutex);
    if (status != 0)
      return(-1);
    else
      return(0);
#else
      (void) md;
      DPRINT(-3,"mMutexLock(): dummy routine, recompile with THREADED define for real function\n");
      return(0);
#endif
}

int mMutexUnlock(MFILE_ID md)
{
#ifdef THREADED
    int status;
    status = pthread_mutex_unlock (&md->mutex);
    if (status != 0)
      return(-1);
    else
      return(0);
#else
      (void) md;
      DPRINT(-3,"mMutexUnlock(): dummy routine, recompile with THREADED define for real function\n");
      return(0);
#endif
}

/*
void mShow(MFILE_ID md)
*/
/************************************************
*
*  mShow - display the Stats on the mmapped file 
*
*
* RETURNS:
*   VOID
*
*/
void mShow(MFILE_ID md)
{
   if (md == NULL)
     return;

#ifdef DEBUG
   DPRINT1(-1,"File Mapped: %s\n",md->filePath);
//   DPRINT1(-1,"File Size on Open: %d\n",md->byteLen);
//   DPRINT3(-1,"Mapped Address and Size: 0x%lx, 0x%lx (%ld)\n",
//		   md->mapStrtAddr,md->mapLen,md->mapLen);
//   DPRINT2(-1,"Present Location with file: 0x%lx, Page: %ld\n",
//		   md->offsetAddr, (diff / psize) );
//   DPRINT1(-1,"New File Size: %ld\n",md->newByteLen);
#endif
}

/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/
#else /* win32 native */
/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/


static long sys_pagesize = 0;
static long sys_regionsize = 0;

// #define MAXMAP 2147483648  // 2GB, this cause problem with right shift by 31, values are not right
// #define MAXMAP 0x7FFFFFFF // 2147483648 , mapping work but view cannot view a file this large
#define MAXMAP 0x70000000   // 1 GB

/***************************************************
*
*  sysPageSize() - get the page size of the Operating
* 		   Systems VM
*/
long getPageSize(void)
{
	if (sys_pagesize == 0)
	{
	   SYSTEM_INFO systemInfo;
	   GetSystemInfo(&systemInfo);
	   sys_pagesize = systemInfo.dwPageSize;
	   sys_regionsize = systemInfo.dwAllocationGranularity;
	}
	return sys_pagesize;
}


/***************************************************
*
*  sysRageSize() - get the allocation granularity of the Operating
* 		   Systems (Win32)
*/
long getRegionSize(void)
{
	if (sys_regionsize == 0)
	{
	   SYSTEM_INFO systemInfo;
	   GetSystemInfo(&systemInfo);
	   sys_pagesize = systemInfo.dwPageSize;
	   sys_regionsize = systemInfo.dwAllocationGranularity;
	}
	return sys_regionsize;
}

/**************************************************************
*
*  freeResources - Frees and close handles to open resources
*
* RETURNS:
* void
*
*/ 
static void freeResources(MFILE_ID mFileId)
{
	if (mFileId == NULL)
		return;
    if (mFileId->mapStrtAddr != NULL)
	{
	    UnmapViewOfFile(mFileId->mapStrtAddr);
        mFileId->mapStrtAddr = NULL;
	}

    if (mFileId->mapHandle != NULL)
	{
      CloseHandle(mFileId->mapHandle);
	  mFileId->mapHandle = NULL;
	}

    if (mFileId->fileHandle != NULL)
	{
	   CloseHandle(mFileId->fileHandle);
	   mFileId->fileHandle = NULL;
	}

   free(mFileId->filePath);
   free(mFileId);
   mFileId = NULL;
   return;
}
/**************************************************************
*
*  mOpen - open a file and returns map discriptor (md)
*
*  mOpen - is used just like the open system call but instead a
* file descriptor (fd) a map descriptor is returned.
* This is actually a pointer to the mmapped file 
* Object Data Structure.
* Any file open for writing is expanded to the maximum
* file size for the OS (this does not use disk space). 
*  mClose() call will resize file based on newByteLen 
* parameter.
*
* RETURNS:
* MFILE_ID - if no error, NULL - if mallocing or open failed
*
*/ 
MFILE_ID mOpen(char *filename, uint64_t size, int permit)
{
   DWORD accessRights;
   DWORD shareRights;
   DWORD creationMode;
   DWORD fileAttributes;
   DWORD sizeLSW,sizeMSW;
   DWORD dwError;
   DWORD mapMode;
   DWORD viewAccess;
   size_t viewSize;
   LPVOID lpMapAddress;
   MFILE_ID mfileObj;
   HANDLE hFile;
   long pagesize;

   /* ------- malloc space for mFile Object --------- */
   if ( (mfileObj = (MFILE_OBJ *) malloc( sizeof(MFILE_OBJ) ) ) == NULL )
   {
     errLogSysRet(ErrLogOp,debugInfo,"mOpen: malloc of Object struct failed, ");
     return(NULL);
   }
   memset(mfileObj,0,sizeof(MFILE_OBJ));

   mfileObj->fileAccess = permit;	/* file permissions */
   if ( (mfileObj->filePath = (char *) (malloc(strlen(filename) + 1))) == NULL )
   {
     freeResources(mfileObj);
     return(NULL);
   }
   strcpy(mfileObj->filePath,filename);
   mfileObj->pageSize = pagesize = getPageSize();  /* not used to map files under windows */


   /* not sure what to do in Win32 for getuid,seteuid & umask, see UNIX routines above */
 
   if (permit != _O_RDONLY)
   {
	 accessRights = GENERIC_WRITE | FILE_ALL_ACCESS;
	 shareRights = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
   }
   else
   {
	 accessRights = GENERIC_READ;
	 shareRights = FILE_SHARE_READ;
   }

   if ( (permit & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
	 creationMode = CREATE_NEW;  /* fails if already present */
   else if ( (permit & (_O_CREAT | _O_TRUNC )) == (_O_CREAT | _O_TRUNC))
     creationMode = OPEN_ALWAYS | TRUNCATE_EXISTING;
   else
     creationMode = OPEN_ALWAYS;


   /* note: there are also FILE_FLAG_RANDOM_ACCESS & FILE_FLAG_SEQUENTIAL_SCAN & */
   /*        FILE_FLAG_OVERLAPPED attributes */
   fileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS /* case sensitive */;

   hFile = CreateFile(filename,accessRights, shareRights, NULL /* no security */, creationMode,
				   fileAttributes, NULL);

   if (hFile == INVALID_HANDLE_VALUE) 
   { 
	errLogSysRet(ErrLogOp,debugInfo,"mOpen: CreateFile of %s failed, ",
                  filename);
	freeResources(mfileObj);
	return NULL;
   }
   mfileObj->fileHandle = hFile;

   // Try to obtain hFile's huge size. 
   sizeLSW = GetFileSize (hFile, &sizeMSW); 
 
   // If we failed ... 
   if (sizeLSW == INVALID_FILE_SIZE && 
      (dwError = GetLastError()) != NO_ERROR )
   { 
      errLogSysRet(ErrLogOp,debugInfo,"mOpen: GetFileSize of %s failed (%s)\n",
                  filename);
	  freeResources(mfileObj);
      return(NULL);
   }

   /* not sure this will work in all cases were size are 2 Gb or larger */
   /* shiting by 32 is a no-op */
   mfileObj->byteLen = (uint64_t)((((__int64)sizeMSW) << 31) | (__int64) sizeLSW); /* actual file size */


   /* Read Only file will stay same size */
   if ( (permit == _O_RDONLY) ) 
   {
      size = mfileObj->byteLen;
	  mfileObj->mapLenHW = 0L;
	  mfileObj->mapLenLW = 0L;   /* causes filemapping to map in all of file */
   }
   else /* file may become bigger therefore mmap a large size */
   {
     /* Map to a 2 Gbyte file if size was 0 */
     if (size == (uint64_t) 0)
        size = MAXMAP; //  - mfileObj->byteLen - pagesize;
   }
   if (size <= MAXMAP)
   {
        // mfileObj->mapLen = (unsigned int) (size + pagesize - 1) & ~(pagesize - 1);
		mfileObj->mapLen = (int) size;
		mfileObj->mapLenHW = (mfileObj->mapLen >> 31) & 0xFFFFFFFFL; /* for 0x8000000, mapLenHW = 1, not right */
		mfileObj->mapLenLW = mfileObj->mapLen & 0xFFFFFFFFL;
        mfileObj->multiMap = 0;
        mfileObj->fd = -1;
   }
   else
   {
        // mfileObj->mapLen = (MAXMAP + pagesize - 1) & ~(pagesize - 1);
		//mfileObj->mapLenHW = (mfileObj->mapLen >> 31) & 0xFFFFFFFFL;
		//mfileObj->mapLenLW = mfileObj->mapLen & 0xFFFFFFFFL;
        //mfileObj->multiMap = 1;
        // mfileObj->fd = -1;
		errLogRet(ErrLogOp,debugInfo,"mOpen: filesize greater than 2GB not supported.\n");
		freeResources(mfileObj);
        return(NULL);
   }

   mfileObj->firstByte = 0L;

   if ( (permit == _O_RDONLY) ) 
   {
     mfileObj->newByteLen = mfileObj->byteLen; /* Read Only File stays same size */
     mfileObj->mmapProt = mapMode = PAGE_READONLY;  /* fileMapping mode */
   }
   else
   {
     mfileObj->newByteLen = (uint64_t) 0;
     mfileObj->mmapProt = mapMode = PAGE_READWRITE;  /* fileMapping mode */
	 /* ftruncate() use to increase filesize in UNIX, not need nder Win32 */
     mfileObj->byteLen = size;		/* actual file size */
   }

/* for routines that seek then use the internal mfile struct to obtain the offeset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 */
#ifdef THREADED
  /* initialize the thread mutex */
  InitializeCriticalSection(&(mfileObj->CriticalSection));
  
#endif
 
   /* 1st create file mapping, then MapView, the equiv of mmap in UNIX */
   /* To share among different processing we must give the FileMapping a Name 
      (last arg) here we just give it the same name as the filepath
   */
   mfileObj->mapHandle = CreateFileMapping( mfileObj->fileHandle, NULL, mapMode, 
	                        mfileObj->mapLenHW, mfileObj->mapLenLW,mfileObj->filePath);



   if (mfileObj->mapHandle == NULL) 
   { 
     errLogSysRet(ErrLogOp,debugInfo,"mOpen: CreateFileMapping of '%s' failed, ",
                  filename);
	 freeResources(mfileObj);
	 return(NULL);
   } 

   /* 2nd create of View of Mapping */
   // Map the view of the entier file or more
   if (permit == _O_RDONLY)
   {
	 viewAccess = FILE_MAP_READ;
	 viewSize = 0L;
   }
   else
   {
	 viewAccess = FILE_MAP_ALL_ACCESS;
	 viewSize = (size_t) mfileObj->mapLenLW;
   }

  /* Note:  A map view must start at an offset into the file that is a multiple 
            of the file allocation granularity (e.g. getRegionSize) */
  lpMapAddress = MapViewOfFile(mfileObj->mapHandle, 
                               viewAccess , // read/write permission 
                               0, // high-order 32 bits of file offset
                               0, // low-order 32 bits of file offset
							      // Note: offset within the file must matches the system's 
								  // memory allocation granularity (e.g. getRegionSize)
                               viewSize); // number of bytes to map 
  if (lpMapAddress == NULL) {
    printf("lpMapAddress is NULL: last error: %d\n", GetLastError());
	errLogSysRet(ErrLogOp,debugInfo,"mOpen: MapViewOfFile of %s failed, ",
                  filename);
	freeResources(mfileObj);
    return NULL;
  }

  // Calculate the pointer to the data.
   mfileObj->mapStrtAddr = (char *) lpMapAddress;
   mfileObj->offsetAddr = (char *) lpMapAddress;

   return( mfileObj );
}


/************************************************
*
*  mClose - close the mmapped file 
*
*  mClose - if the file was open for writing then the member 
* parameter newByteLen (new_byte_len) is used to 
* truncate the file back down to the correct size.
* The file is then unmapped.
*
*   Note: It is the user's responsibility to correctly set 
*         the value of newByteLen.
*
* RETURNS:
*   0 - if no error, -1 - if error occurred
*
*/
int mClose(MFILE_ID mFileId)
{
//   int fd,access;
//   mode_t old_umask;

   if (mFileId == NULL)
      return(-1);

   if (mFileId->filePath == NULL)
      return(-1);


   //if (!FlushViewOfFile(mFileId->mapStrtAddr, mFileId->mapLenLW)) 
  // {
   //  ErrorHandler("Could not flush memory to disk."); 
   //}

   UnmapViewOfFile(mFileId->mapStrtAddr);
   mFileId->mapStrtAddr = NULL;

   if (mFileId->mapHandle != NULL)
   {
      CloseHandle(mFileId->mapHandle);
	  mFileId->mapHandle = NULL;
   }
   /* Resize file to actual byte size (newByteLen) */
   /* note: you must first call UnmapViewOfFile to unmap all views and call 
    * CloseHandle to close the file mapping object before you can call 
	* SetEndOfFile().
	*/

   if ( (mFileId->fileAccess != _O_RDONLY) ) 
   {
	   /* truncate file to proper size */
	   if (mFileId->newByteLen != mFileId->byteLen)
       {
		   LARGE_INTEGER offset;  /* don't be fooled this is a structure/union */
		   offset.QuadPart = mFileId->newByteLen; /* 64 bit union memeber */
		   SetFilePointerEx(mFileId->fileHandle,offset,NULL,FILE_BEGIN);
		   SetEndOfFile(mFileId->fileHandle);
	   }
   }
   if (mFileId->fileHandle != NULL)
   {
	   CloseHandle(mFileId->fileHandle);
	   mFileId->fileHandle = NULL;
   }

   free(mFileId->filePath);
   mFileId->filePath = NULL;
   free(mFileId);
   mFileId = NULL;
   return(0);
}
/* for routines that seek then use the internal mfile struct to obtain the offset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 *
 *         Author:  Greg Brissey    4/19/2005
 */
int mMutexLock(MFILE_ID md)
{
#ifdef THREADED

   // Request ownership of the critical section.
    EnterCriticalSection(&(md->CriticalSection)); 

#else
      DPRINT(-3,"mMutexLock(): dummy routine, recompile with THREADED define for real function\n");
      
#endif
	  return(0);
}

int mMutexUnlock(MFILE_ID md)
{
#ifdef THREADED
   // Release ownership of the critical section.
    LeaveCriticalSection(&(md->CriticalSection));

#else
      DPRINT(-3,"mMutexUnlock(): dummy routine, recompile with THREADED define for real function\n");
#endif
  return(0);
}
/************************************************
*
*  mFidSeek - Set offsetAddr to the beginning of the
*	      blockheader for the requested FID.
*
*  md         - pointer to the mmaped file information.
*  fidNum     - requested FID number
*  headerSize - size, in bytes of the FID file header.
*               will be sizeof(struct datafilehead)
*  bbytes     - size, in bytes, of a FID and its block
*               header.
*
*  RETURNS:
*  0 for success, -1 for failure.
*/
int mFidSeek(MFILE_ID md, int fidNum, int headerSize, unsigned long bbytes)
{
    uint64_t fidoffset;
 
    if (fidNum < 1L)
    {
      errLogRet(ErrLogOp,debugInfo, "mFidSeek: Fid number < 1.\n");
      return(-1);
    }
    fidoffset = headerSize + ((uint64_t) bbytes * ((uint64_t) (fidNum - 1L)));
    /* test to make sure we're not pointer off the end of the file */
    if (fidoffset + (uint64_t) bbytes > md->byteLen )
    {
       errLogRet(ErrLogOp,debugInfo, 
	  "mFidSeek: Fid Number %lu,  resulted in seek past end of file.\n",
	  fidNum);
       return(-1);
    }
    if ( fidoffset + (uint64_t) bbytes > md->newByteLen)
       md->newByteLen = fidoffset + (uint64_t) bbytes;

    if (!md->multiMap)
    {
       md->offsetAddr = md->mapStrtAddr + fidoffset;
    }
    else
    {
       errLogRet(ErrLogOp,debugInfo, "mFidSeek: files greater than 2GB not supported.\n");
    }

    DPRINT4(1,"mFidSeek(): fid# = %lu, offset = %lu(0x%lx), addr = 0x%lx\n",
		fidNum, fidoffset, fidoffset, md->offsetAddr);
    return(0);
}

#endif /* VNMR_WIN32 */
