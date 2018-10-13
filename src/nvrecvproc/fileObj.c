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
/* fileObj.c  fileObj.c  */

#ifdef LINUX
#define _XOPEN_SOURCE 600      /* needed for pwrite and pread */
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdlib.h>

#include "errLogLib.h"
#include "fileObj.h"


/*
modification history
--------------------
02-05-2007,gmb  created 
*/

/*
DISCRIPTION


*/

#define ROOT_UID 0
static  int  old_euid = -1;
static  int  old_uid = -1;

/* if Ext3 file system then do not use posix_fallocate(), takes minutes to complete.
   Checkout fo ext4 file system using df -T
    GMB 8/7/2012  
*/
static int Ext4 = -1;


// these 2 routine ripped-off from shellcmd.c
static char * fgets_nointr(char *datap, int count, FILE *filep )
{
   char  *qaddr;

   while ( (qaddr = fgets( datap, count, filep )) == NULL) {
                if (errno != EINTR || (feof(filep)))
        return( NULL );
   }

   return( qaddr );
}

// execute thread-safe shell command
static int exec_shell(char *cmd)
{
   FILE *stream;
   int num_chars_out=0;
   char data[1024];
   data[0] = 0;
   DPRINT1(2,"exec_shell() cmd: '%s'\n",cmd);
   if ((stream = popen(cmd, "r")) != NULL) {
      // wait for shell to exit
      char *p = fgets_nointr(data, 1024, stream);
      while (p != NULL) {
         int len = strlen(data);
         if (len > 0) {
            // display any echo or print statement issued from shell
            if (data[len - 1] == '\n')
               data[len - 1] = '\0';
            DPRINT1(2,"Line: '%s' \n", data);
            if ( strstr(data,"ext4") != NULL)
               Ext4 = 1;
            else
               Ext4 = 0;
            DPRINT1(2,"Ext4: %d \n",Ext4);
         }
         num_chars_out+=len;
         p = fgets_nointr(data, 1024, stream);
      }
      pclose(stream);
   }
   return num_chars_out; // return number of chars printed from shell
}




static void determineExt()
{
      int chars;
     // exec_shell("/bin/df -T | /bin/grep '^/dev'");
     chars = exec_shell("/bin/df -T | /bin/grep 'ext4'");
     DPRINT1(2,"determineExt() returned chars: %d\n",chars);
     DPRINT1(2,"Ext4: %d \n",Ext4);
     // if no charaters return and Ext4 has not been set then no ext4 were found in grep
     // thus it is not Ext4
     if ((Ext4 == -1) && (chars == 0))
        Ext4 = 0;

   return;
}


/**************************************************************
*
*  fFileOpen - open a file and returns map discriptor (md)
*
*  mOpen - is a wrapper, but instead of using MMAp regualr file I/O is used 
*
* RETURNS:
* FILE_ID - if no error, NULL - if mallocing or open failed
*
*/ 
FILE_ID fFileOpen(char *filename, uint64_t size, int permit)
{
   FILE_ID fileObj;
   int fd,result;
   mode_t old_umask;

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
         seteuid( old_uid );
      }
   }
   /* open file, thus checking most error possibilities of file create, etc. */
   old_umask = umask(000); /* clear file creation mode mask,so that open has control */
   DPRINT2(2,"fOpen: open('%s',0x%x)\n", filename,permit);
   if ((fd = open(filename,permit,0666)) < 0)
   {
     errLogSysRet(ErrLogOp,debugInfo,"fOpen: open of %s failed (%s)\n",
                  filename,strerror(errno) );
     umask(old_umask);  /* restore umask control over open. */
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
       seteuid( old_euid );
     return (NULL);
   }

   // if Ext4 not set then do so.
   if (Ext4 == -1)
      determineExt();

  // If not Ext4 do not use posix_fallocate, fixes Bug 9578
   if ( Ext4 == 1 )
   {
      DPRINT2(2,"fOpen: posix_fallocate(%d, 0, %ld) \n", fd, (off_t) size);
      result = posix_fallocate(fd, (off_t) 0, (off_t) size);
      if (result != 0)
      {
         close(fd);
         unlink(filename);
         errLogSysRet(ErrLogOp,debugInfo,"fOpen: posix_fallocate(%d, 0, %ld) of %s failed (%s)\n",fd,(off_t) size,
                     filename,strerror(errno) );
         return (NULL);
      }
   }
   else
   {
      DPRINT(2,"fOpen: Not Ext4 skip posix_fallocate() \n");
   }
   /* DPRINT1(-11,"fd = %d\n", fd); */
   umask(old_umask);  /* restore umask control over open. */

   /* ------- malloc space for adc Object --------- */
   if ( (fileObj = (FILE_OBJ *) malloc( sizeof(FILE_OBJ) ) ) == NULL )
   {
     errLogSysRet(ErrLogOp,debugInfo,"fOpen: malloc of Object struct failed\n");
     close(fd);
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
        seteuid( old_euid );
     return(NULL);
   }

   fileObj->fd = fd;	/* file permission */
   fileObj->offsetAddr = (off_t) 0;	/* file permission */
   /* DPRINT2(-12,"sizeof(off_t) = %d, offset; %d\n", sizeof(off_t), sizeof(fileObj->offsetAddr)); */
   fileObj->fileAccess = permit;	/* file permission */
   if ( (fileObj->filePath = (char *) (malloc(strlen(filename) + 1))) == NULL )
   {
     close(fd);
     if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
        seteuid( old_euid );
     free(fileObj);
     return(NULL);
   }
   strcpy(fileObj->filePath,filename);


   if ( (permit != O_RDONLY) && (old_uid == ROOT_UID) )
      seteuid( old_euid );

   DPRINT2(2,"fFileOpen(): fd = %d, filepath: '%s' \n", fileObj->fd,fileObj->filePath);

   /* if this file is being create then use the passed filesize to write to the end 
      resulting in a an empty file that shows the size it will be 
      added in reponce to a recon problem, it  MMAPs the file as read only 
      MMAP fstats the file, if it still being written by recvproc the MMAP see doesn't
      get the true file size resulting in it go off the end as data is being 
      accessed for the new data written to the file */
    /* DPRINT3(-5, "Permit: 0x%lx, permit & 0x%lx,  result %d\n", permit,
     *       (O_RDWR | O_CREAT | O_TRUNC),permit & (O_RDWR | O_CREAT | O_TRUNC));
     *  DPRINT1(-5,"filesize: %llu\n",size); 
     */
   if ( (permit & (O_RDWR | O_CREAT | O_TRUNC)) > 0 )
   {
       /* DPRINT(-5,"seek to filesize end\n"); */
       lseek(fd,size-4,SEEK_SET);
       write(fd,&fd,4);
       lseek(fd,0,SEEK_SET);
   }

   return( fileObj );
}


/************************************************
*
*  fFileClose - close the file 
*
*  fFileClose - iNow warpper for regular File I/O
*
*
* RETURNS:
*   0 - if no error, -1 - if error occurred
*
*/
int fFileClose(FILE_ID fFileId)
{
   if (fFileId == NULL)
      return(-1);

   if (fFileId->filePath == NULL)
      return(-1);

   if (fFileId->fd >= 0)
      close(fFileId->fd);


   free(fFileId->filePath);
   free(fFileId);
   return(0);
}

/************************************************
*
*  fFidSeek - Set offsetAddr to the beginning of the
*	      blockheader for the requested FID.
*
*  md         - pointer to the file information.
*  fidNum     - requested FID number
*  headerSize - size, in bytes of the FID file header.
*               will be sizeof(struct datafilehead)
*  bbytes     - size, in bytes, of a FID and its block
*               header.
*
*  RETURNS:
*  0 for success, -1 for failure.
*/
long long fFidSeek(FILE_ID md, int fidNum, int headerSize, unsigned long bbytes)
{
    uint64_t fidoffset;
 
    if (fidNum < 1L)
    {
      errLogRet(ErrLogOp,debugInfo, "mFidSeek: Fid number < 1.\n");
      return(-1);
    }
    fidoffset = headerSize + ((uint64_t) bbytes * ((uint64_t) (fidNum - 1L)));

    md->offsetAddr = fidoffset;

    DPRINT3(2,"fFidSeek(): fid# = %lu, offset = %llu, 0x%llx\n",
		fidNum, fidoffset, fidoffset);

    return(fidoffset);
}

/*
 * write buffer out to File using Posix Thread Safe pwrite() routine
 *
 *      Author:  Greg Brissey
 */
int fFileWrite(FILE_ID md,char *data, int bytelen, long long offset)
{
   ssize_t bytes;
   if (md == NULL)
      return(-1);

   if (md->fd < 0)
     return(-1);

   /* bytes = pwrite(md->fd,(const void*) data,(size_t) bytelen, (off_t) md->offsetAddr); */
   DPRINT4(1,"fFileWrite(fd: %d, dataAddr: 0x%lx, len: %d, offset: %lld\n",
		md->fd,data,bytelen, offset);

   bytes = pwrite(md->fd,(const void*) data,(size_t) bytelen, offset);
 
   return((int) bytes);
}

/*
 * read file data into  buffer using Posix Thread Safe pread() routine
 *
 *      Author:  Greg Brissey
 */
int fFileRead(FILE_ID md,char *data, int bytelen, long long offset)
{
   ssize_t bytes;
   if (md == NULL)
      return(-1);

   if (md->fd < 0)
     return(-1);

   /* bytes = pread(md->fd,(void*) data,(size_t) bytelen, (off_t) md->offsetAddr); */
   DPRINT4(1,"fFileRead(fd: %d, dataAddr: 0x%lx, len: %d, offset: %lu\n",
		md->fd,data,bytelen,(unsigned long) offset);

   bytes = pread(md->fd,(void*) data,(size_t) bytelen, offset);
 
   return((int) bytes);
}


void fFileShow(md)
FILE_ID md;
{
   return;
}
