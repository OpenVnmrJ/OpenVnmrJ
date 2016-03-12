/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/*
*/
/* Automation QSM control program for the shim serial EEprom. */

#include "qspiObj.h"
#include "m68332_nmri.h"

extern QSPI_ID pTheQspiObject;
extern int shimDebug;

/*
/*	To read status from the Serial EEPROM
/*  	Select the EEPROM, send RDSR (0x05) and read 1 word of data & deselect.
/*		status byte is 1,1,1,1,BP1,BP0,WEL,WIP
/*		WIP = 1 = Write in progress
/*		WEL = 1 = Write Enable Latch set for write function.
/*		BP1,BP0 = 00 No Write protect
/*		        = 01 0x180 - 0x1ff protected
/*		        = 10 0x100 - 0x1ff protected
/*		        = 11 0x000 - 0x1ff protected
*/

#define	RDSRwip  0x1  /* WIP status bit */
#define	RDSRwel  0x2  /* WEL status bit */
#define	RDSRbp   0xC  /* mask for BP1,BP0 */
#define	RDSRrdok 0x3  /* mask for OK to read */

/*
/*	To write data to the Serial EEPROM:
/* 1)	Check status to insure a Write In Progress is NOT taking place.
/* 2)	Select the EEPROM, send 1 byte WREN (0x06) with deselect at end of byte.
/* 3)	Select the EEPROM and sent the following bytes with one deselect at end.
/* 4)	WRITE0 (0x02) or Write1 (0x0A), an Address byte, and one or sixteen
/*			bytes of data.
/* 5)	Wait till read of status indicates write is finished.
/* ### For now assume 16 byte blocks on aligned addresses */

/********************************************************
*
* shimEEpromWrite - Write to the Shim EEprom
*     blkAddr is the byte address in EEprom to start.
*
*     blkSize is the number of bytes to process.
*      IF blkSize would place end of operation out of bounds
*         then operation will be truncated to end of EEprom
*         and the number of blocks actually processed returned.
*
*     EEpromData is a pointer to take/put data/status.
*
*     IF blkAddr is out of bounds No operation will take place and
*        ERROR is returned.
*
* RETURNS:
*  No bytes processed or status, unless blkAddr is out of bounds then ERROR.
*/

int shimEEpromWrite(int blkAddr, int NoBlks, char *EEpromData)
{
register QSPI_OBJ *pQspiObj;
short wrcmd;
int NotReady;
int stat;
int i;

register char  *xmitptr,*prtptr;
unsigned char  *QSM_COMD_RAM;
unsigned short *QSM_TX_RAM;
unsigned short *QSM_RX_RAM;

   if (shimDebug > 10) printf("shimEEpromWrite:\n");
   if (shimDebug > 20)
      printf("shimEEpromWrite: BlkAddr = %2d, NoBlks = %2d, data = \n\'%s\'\n",
		blkAddr, NoBlks, EEpromData);

   pQspiObj = pTheQspiObject;

   /* for now blkAddr is 16byte blocks and blkSize the same */
   if(blkAddr > 496) return(ERROR);

   if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR)
	   return(10);

   if (pQspiObj->qspiconfig != SPCR0SEinit) 
      /* CLK=hi/932 Khz, CAP LE, CHG TE, 16bits 0x8209 */
      qspiInit(SPCR0SEinit); 

   for(i=0;i<NoBlks;i++)  /*process each block till done */
   {
      NotReady = TRUE;
      while(NotReady)
      {
         stat = (int)qspiStatus(EEpromPCSno,SPCR0SEinit);
         if(shimDebug > 11) printf(";w;");
         /* ### Should be a watchdog timer here for safety */
         /* If Not ready, could be a write still in progress, wait. */
         if( (stat & RDSRrdok) == 0) NotReady = FALSE;
         /* If only Write Enable, We should not be here */
         if( (stat & RDSRrdok) == RDSRwel)
         {
            if(shimDebug > 11) printf("bad status\n");
            semGive(pQspiObj->pQspiSEMid);
            return(ERROR);
         }
      }
  
      qspiIO16(blkAddr, EEpromData, FALSE, EEpromPCSno);

      EEpromData += 16;
      blkAddr    += 16; /* advance blkAddr to next address */
   
   } /* end for(i=0;i<NoBlks;i++) */
   
   semGive(pQspiObj->pQspiSEMid);

   return(NoBlks);

} /* end shimEEpromWrite() */


/*
/* 	To read data from the Serial EEPROM:
/* 1)	Check status to insure a Write In PRogress is NOT taking place.
/* 2)	Select the EEPROM and send the following bytes with one deselect at end.
/* 	READ0 (0x03) or READ1 (x83) and Address byte and 1 or more byte/word
/* 		clocks. data transfer is terminated with a deselect.
*/

/* ### For now assume 16 byte blocks on aligned addresses */

int shimEEpromRead (int blkAddr, int NoBlks, char *EEpromData)
{
register QSPI_OBJ *pQspiObj;
short rdcmd;
int NotReady;
int stat;
int i;

register char  *recptr,*prtptr;
unsigned char  *QSM_COMD_RAM;
unsigned short *QSM_TX_RAM;
unsigned short *QSM_RX_RAM;

   pQspiObj = pTheQspiObject;

   /* for now blkAddr is 16byte blocks and blkSize the same */
   if (blkAddr > 496) return(ERROR);

   if (shimDebug > 10) printf("shimEEpromRead1:\n");
   if (shimDebug > 20)
      printf("shimEEpromRead: BlkAddr = %2d, NoBlks = %2d\n", blkAddr, NoBlks);

   if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR )
	   return(10);

   if (pQspiObj->qspiconfig != SPCR0SEinit) 
      /* CLK=hi/932 Khz, CAP LE, CHG TE, 16bits 0x8209 */
      qspiInit(SPCR0SEinit); 
   
   for (i=0;i<NoBlks;i++)  /*process each block till done */
   {
      int failcount = 0;
      NotReady = TRUE;
      while(NotReady)
      {
         stat = (int)qspiStatus(EEpromPCSno,SPCR0SEinit);
      	 if(shimDebug > 11) printf(":r:");
         /* ### Should be a watchdog timer here for safety */
      	 /* If Not ready, could be a write still in progress, wait. */
      	 if( (stat & RDSRrdok) == 0)
      	 {
      	    NotReady = FALSE;
      	    if(shimDebug > 11) printf(":F:");
      	 }
      	 /* If only Write Enable, We should not be here */
      	 if( (stat & RDSRrdok) == RDSRwel)
      	 {
      	    if(shimDebug > 11) printf("bad status\n");
      	    semGive(pQspiObj->pQspiSEMid);
      	    return(ERROR);
      	 }
         failcount++;
         if (failcount > 10)
         {
      	    if(shimDebug > 11) printf("bad status 2\n");
      	    semGive(pQspiObj->pQspiSEMid);
      	    return(ERROR);
         }
      }
   
      if (shimDebug > 10) printf("shimEEpromRead2:\n");

      qspiIO16(blkAddr, EEpromData, TRUE, EEpromPCSno);

      EEpromData += 16;
      blkAddr    += 16; /* advance blk to next address */

   } /* end for(i=0;i<NoBlks;i++) */

   semGive(pQspiObj->pQspiSEMid);

   return(FALSE);

} /* end shimEEpromRead() */

/********************************************************
*   shimEEpromPresent
*
*   Changed to return type of shims, as read from the PROM
*   (assuming they are present)  Non-zero value means some
*   kind of shim set was found; zero value means the shim
*   EEPROM were not found.
*/
  
int shimEEpromPresent()
{
register QSPI_OBJ	*pQspiObj;
char			shimEEprom[ 34 ];
int			shimType;

   pQspiObj = pTheQspiObject;

   if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR)
      return(10);

   if ( qspiStatus( EEpromPCSno, SPCR0SEinit ) == 0)
   {  semGive(pQspiObj->pQspiSEMid);
      return( 0 );			/* not present */
   }

/*  1st argument to shimEEpromRead specifies the starting block.
/*  They are numbered starting at 0.
/*
/*  2nd argument specifies how many blocks to read.  Each block in
/*  the EEPROM is 16 bytes long.  We read 2, so we expect 32 bytes
/*  back.  We are interested in bytes 29 and 30.			*/

   shimEEpromRead( 0, 2, &shimEEprom[ 0 ] );
   shimType = ((int) (shimEEprom[ 29 ] - '0') * 10) +
	       (int) (shimEEprom[ 30 ] - '0');

   semGive(pQspiObj->pQspiSEMid);
   return( shimType );
}

/* end qShimEEprom.c */
