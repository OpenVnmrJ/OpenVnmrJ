/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 * persist data to flash
 */
#include <stdio.h>
#include <sys/types.h>
#include <vxWorks.h>
#include <time.h>
#include "vTypes.h"
#include "vWareLib.h"

int persistence = 0;

FFSHANDLE start_persisting(const char* key)
{
  FFSHANDLE handle;
  UINT32 error;
  if (key == NULL)
    return (FFSHANDLE) -1;

  sysFlashRW();        // make flash writeable

  if ((handle = vfOpenFile((char *) key)) == 0) {
    handle = vfCreateFile((char *) key,-1,-1,-1);
    if (handle == 0) {
      error = vfGetLastError();
      vfFfsResult((char *)key);
      sysFlashRO();    // make flash read-only

    }
    return (FFSHANDLE) -1;
  }
  return handle;
}

UINT32 stop_persisting(FFSHANDLE handle)
{
  return vfCloseFile(handle);
}

int persist(const char* key, int size, void* data)
{
#if 0
    bytesleft = vfFilesize(oldFile);
    while( bytesleft != 0)
    {
        buffersize = (bytesleft < 1024) ? bytesleft : 1024;

       /* read it in */
        errorcode = vfReadFile(oldFile, offset, buffer, buffersize, &bufCnt);
	if((errorcode != 0) || (bufCnt != buffersize))
        {
            errorcode = vfGetLastError();
            if (errorcode)
               vfFfsResult(filename);
            vfCloseFile(oldFile);
            vfAbortFile(newFile);
            sysFlashRO();    /* Flash to Read Only, via EBC  */
	    return(-1);
        }
        offset = offset + bufCnt;

        /* write it out */
        errorcode = vfWriteFile(newFile, buffer, buffersize, &bufCnt);
        if ((errorcode != 0) || (bufCnt != buffersize))
        {
            errorcode = vfGetLastError();
            if (errorcode)
	       vfFfsResult(newname);
            /*  Close and delete file */
            vfAbortFile(newFile);
            vfCloseFile(oldFile);
            sysFlashRO();    /* Flash to Read Only, via EBC  */
            return(-1);
        }
        bytesleft = bytesleft - bufCnt;
    }
#endif
}
