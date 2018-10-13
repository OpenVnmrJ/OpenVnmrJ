/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rngLLib.c  11.1 07/09/07 - Long Ring Buffer Object Source Modules */
/* 
 */


#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
/* #include <vxWorks.h> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rngLLib.h"

/*
modification history
--------------------
4-19-99,gmb  created 
*/

/*
DESCRIPTION

Long Ring Buffer Library
routines to create a ring buffer that access by longs no bytes
For additional description see rngLib.

*/

/**************************************************************
*
*  rngLCreate - Creates an empty ring buffer 
*
*
*  This routine creates a long ring buffer of size <n elements(longs)> 
*and initializes it. Memory for the buffer is allocated from 
*the system memory partition.
*
*
* RETURNS:
* ID of the blocking ring buffer, or NULL if memory cannot be allocated.
*
*	Author Greg Brissey 4/19/99
*/
RINGL_ID rngLCreate(int nelem,char* idstr)
/* int nelem;  size in elements of ring buffer */
{
  RINGL_ID pLRng;
  char tmpstr[80];

  pLRng = (RINGL_ID) malloc(sizeof(RING_LONG));  /* create structure */
  if (pLRng == NULL) 
     return (NULL);

  memset(pLRng,0,sizeof(RING_LONG));

  /* create Ring Buffer size buffer nelem + 1 */
  pLRng->rBuf = (long *) malloc((sizeof(long) * nelem) + (sizeof(long)) );

  if ( (pLRng->rBuf) == NULL)
  {
     free(pLRng->pRngIdStr);
     free(pLRng);
     return(NULL);
  }

  pLRng->pToBuf = pLRng->pFromBuf = 0;
  
  pLRng->maxSize = pLRng->bufSize = nelem + 1;

  return( pLRng );
}

/**************************************************************
*
* rngLDelete - Delete Ring Buffer
*
*
*  Delete Blocking Ring Buffer
*  This routine deletes a blocking ring buffer 
*
* RETURNS:
* N/A
*
*		Author Greg Brissey 5/26/94
*/
void rngLDelete(register RINGL_ID rngd)
/* RINGL_ID rngd;  long ring buffer to delete */
{
  free(rngd->rBuf);
  free(rngd);
}


/**************************************************************
*
* rngLFlush - make a ring buffer empty
*
*
*  This routine initialized a ring buffer to be empty.
*Any data in the buffer will be lost. 
*
* RETURNS:
* N/A
*
*	Author Greg Brissey 5/26/94
*/
void rngLFlush(register RINGL_ID rngd)
/* RINGL_ID rngd; Long ring buffer to initialize */
{
  int npend,pAry[4];
  rngd->pToBuf = rngd->pFromBuf = 0;
}


/**************************************************************
*
* rngLPut - put element into a blocking ring buffer
*
*
*  This routine copies n elements from <buffer> into long ring buffer
*<rngd>. The specified number of elements will be put into the ring buffer.
*
* RETURNS:
*  The number of elements actually put into ring buffer.
*
*	Author Greg Brissey 5/26/94
*/
int rngLPut(register RINGL_ID rngd,register long* buffer,register int size)
/* RINGL_ID rngd;	long ring buffer to put data into */
/* long*      buffer;   buffer to get data from */
/* int	      size;     number of elements to put */
{
   register int fromP;
   int nelem;
   int npend,pAry[4];
   register int result,i;

   nelem = 0;
   for (i = 0; i < size; i++)
   {
     /* this macro inlines the code for speed */
     nelem += RNG_LONG_PUT(rngd, (buffer[i]), fromP);
   }

   return( nelem );
}

/**************************************************************
*
* rngLGet - get elements from a blocking ring buffer 
*
*
*  This routine copies elements from long ring buffer <rngd> into <buffer>
*The specified number of elements will be put into the buffer.
*
*
* RETURNS:
*  The number of elements actually put into buffer.
*
*	Author Greg Brissey 5/26/94
*/
int rngLGet(register RINGL_ID rngd,long* buffer,int size)
/* RINGL_ID rngd;	long ring buffer to get data from */
/* char*      buffer;   point to buffer to receive data */
/* int	      size;     number of elements to get */
{
   register int fromP;
   int nelem;
   int i,items;

   /* items = RNG_LONG_GET(rngd, buffer,fromP); */
   items = 0;
   for (i = 0; i < size; i++)
   {
     /* this macro inlines the code for speed */
     items += RNG_LONG_GET(rngd, &(buffer[i]), fromP) ;
   }
   return ( items );   /* For now only get one item at a time. */
}

/**************************************************************
*
* rngLShow - show long ring buffer information 
*
*
*  This routine displays the information on a long ring buffer <rngd> 
*
* RETURNS:
*  VOID 
*
*	Author Greg Brissey 8/9/93
*/
void rngLShow(register RINGL_ID rngd,int level)
/* RINGL_ID rngd;	long ring buffer to get data from */
/* int	      level;    level of information display */
{
    int used,nfree,total;
    int npend,pAry[4];
    int i;

   used = rngLNElem (rngd);
   nfree = rngLFreeElem (rngd);
   total = used + nfree;

   printf("Blk Ring BufferID:  0x%lx\n",rngd);
   printf("Buffer Addr: 0x%lx, Size: %d (0x%x)\n",rngd->rBuf, rngd->bufSize,rngd->bufSize);
   printf("Entries  Used: %d, Free: %d, Total: %d\n", used, nfree, total);

   printf("Buffer Put Index: %d (0x%x), Get Index: %d (0x%x)\n",
		rngd->pToBuf,rngd->pToBuf,rngd->pFromBuf,rngd->pFromBuf);

   if (level > 0) 
   {
     int fromP;
     long *buff;
     fromP = (rngd)->pFromBuf;
     buff = (long*) &((rngd)->rBuf[fromP]);

     for(i=0;  i < used; i++)
     {
        printf("item[%d] = %ld (0x%lx)\n",i,buff[i],buff[i]);
     }
   }
   else if (level > 1) 
   {
     long *buff;

     buff = (long*) rngd->rBuf;
     for(i=0; i < total + 1 ; i++)
       printf("item[%d] = %ld (0x%lx)\n",i,buff[i],buff[i]);
   }

}


void rngLShwResrc(register RINGL_ID rngd, int indent )
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   /* printf("\n%sBlk Ring BufferID: '%s', 0x%lx\n",spaces,rngd->pRngIdStr,rngd); */
   printf("\n%sBlk Ring BufferID: 0x%lx\n",spaces,rngd);
}

/**************************************************************
*
* rngLFreeElem - return the number of Free Elements in the 
*		    ring buffer empty
*
*
*  This routine returns the number of free elements in the blocking ring 
*buffer. 
*
* RETURNS:
* number of free elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngLFreeElem (register RINGL_ID ringId)
{
   register int result;

   return( ( (result = ((ringId->pFromBuf - ringId->pToBuf) - 1)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngLNElem  returns number of used elements
*
*
*  This routine returns the number of used elements in the long ring 
*buffer. 
*
* RETURNS:
* number of used elements
*
*	Author Greg Brissey 5/26/94
*/
int 	rngLNElem (register RINGL_ID ringId)
{
   register int result;

   return( ( (result = (ringId->pToBuf - ringId->pFromBuf)) < 0) ? 
	   result + ringId->bufSize : result );
}

/**************************************************************
*
* rngLIsEmpty - returns 1 if ring buffer is empty
*
*
*  This routine returns 1 if ring buffer is empty
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngLIsEmpty (register RINGL_ID ringId)
{
    return ( (ringId->pToBuf == ringId->pFromBuf) ? 1 : 0 );
}

/**************************************************************
*
* rngLIsFull - returns 1 if ring buffer is full
*
*
*  This routine returns 1 if ring buffer is full
*
* RETURNS:
*  0 or 1 
*
*	Author Greg Brissey 5/26/94
*/
int 	rngLIsFull (RINGL_ID ringId)
{
    register int result;

    if ( (result = ((ringId->pToBuf - ringId->pFromBuf) + 1)) == 0)
    {
	return( 1 );
    }
    else if ( result != ringId->bufSize)
    {
	return( 0 );
    }
    else
        return( 1 );
}
void         rngLPutAhead (RINGL_ID ringId, long elem, int offset)
{
   printf("rngLPutAhead: Not implimented yet.\n");
}
