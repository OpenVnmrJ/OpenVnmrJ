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
#include "logMsgLib.h"
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
/*
/*	To write data to the Serial EEPROM:
/* 1)	Check status to insure a Write In Progress is NOT taking place.
/* 2)	Select the EEPROM, send 1 byte WREN (0x06) with deselect at end of byte.
/* 3)	Select the EEPROM and sent the following bytes with one deselect at end.
/* 4)	WRITE0 (0x02) or Write1 (0x0A), an Address byte, and one or sixteen
/*			bytes of data.
/* 5)	Wait till read of status indicates write is finished.
/* ### For now assume 16 byte blocks on aligned addresses
/* */


#define	RDSRwip  0x1  /* WIP status bit */
#define	RDSRwel  0x2  /* WEL status bit */
#define	RDSRbp   0xC  /* mask for BP1,BP0 */
#define	RDSRrdok 0x3  /* mask for OK to read */

	char	probeId[34];
static	int	ProbeIdPCSno = 8;

/********************************************************
*
* probeIdWrite - Write to the ProbeId EEprom
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

int probeIdWrite(int blkAddr, int NoBlks, char *EEpromData)
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

   if (shimDebug > 10) printf("probeIdWrite:\n");
   if (shimDebug > 20)
      printf("probeIdWrite: BlkAddr = %2d, NoBlks = %2d, data = \n\'%s\'\n",
		blkAddr, NoBlks, EEpromData);

   pQspiObj = pTheQspiObject;

   /* for now blkAddr is 16byte blocks and blkSize the same */
   if(blkAddr > 496) return(ERROR);

   if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR)
	   return(10);

   if (pQspiObj->qspiconfig != SPCR0Sinit|0xFF) 
      /* CLK=hi/31 Khz, CAP LE, CHG TE, 16bits 0x8209 */
      qspiInit(SPCR0Sinit | 0xFF); 

   for(i=0;i<NoBlks;i++)  /*process each block till done */
   {
      NotReady = TRUE;
      while(NotReady)
      {
         stat = (int)qspiStatus(ProbeIdPCSno,SPCR0Sinit | 0xFF);
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
  
      qspiIO16(blkAddr, EEpromData, FALSE, ProbeIdPCSno);

      EEpromData += 16;
      blkAddr    += 16; /* advance blkAddr to next address */
   
   } /* end for(i=0;i<NoBlks;i++) */
   
   semGive(pQspiObj->pQspiSEMid);

   return(NoBlks);

} /* end probeIdWrite() */


/*
/* 	To read data from the Serial EEPROM:
/* 1)	Check status to insure a Write In PRogress is NOT taking place.
/* 2)	Select the EEPROM and send the following bytes with one deselect at end.
/* 	READ0 (0x03) or READ1 (x83) and Address byte and 1 or more byte/word
/* 		clocks. data transfer is terminated with a deselect.
*/

/* ### For now assume 16 byte blocks on aligned addresses */

int probeIdRead (int blkAddr, int NoBlks, char *EEpromData)
{
register QSPI_OBJ *pQspiObj;
short rdcmd;
int NotReady;
int stat;
int i,j;

register char *recptr,*prtptr;
unsigned char *QSM_COMD_RAM;
unsigned short *QSM_TX_RAM;
unsigned short *QSM_RX_RAM;

   pQspiObj = pTheQspiObject;

   /* for now blkAddr is 16byte blocks and blkSize the same */
   if (blkAddr > 496) return(ERROR);

   if (shimDebug > 10) printf("probeIdRead:\n");
   if (shimDebug > 20)
      printf("probeIdRead: BlkAddr = %2d, NoBlks = %2d\n", blkAddr, NoBlks);

   if ( semTake(pQspiObj->pQspiSEMid,WAIT_FOREVER) == ERROR )
	   return(10);

   if (pQspiObj->qspiconfig != SPCR0Sinit | 0xFF) 
      /* CLK=hi/932 Khz, CAP LE, CHG TE, 16bits 0x8209 */
      qspiInit(SPCR0Sinit | 0xFF); 
   
   for (i=0;i<NoBlks;i++)  /*process each block till done */
   {
      NotReady = TRUE;
      while(NotReady)
      {
         stat = (int)qspiStatus(ProbeIdPCSno,SPCR0Sinit | 0xFF);
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
      }
   
      if (shimDebug > 10) printf("probeIdRead:\n");

      qspiIO16(blkAddr, EEpromData, TRUE, ProbeIdPCSno);

      EEpromData += 16;
      blkAddr    += 16; /* advance blk to next address */

   } /* end for(i=0;i<NoBlks;i++) */

   semGive(pQspiObj->pQspiSEMid);

   return(FALSE);

} /* end probeIdRead() */

/*----------------------------------------------------
 | ProbeId ISR
 -----------------------------------------------------*/
char	probeId[34];

void qspiProbeISR(QSPI_ID pQspiId)
{
   semGive(pQspiId->pQspiSEMBplug);	/* give semaphore, releasing task */
}

void probeIdTask()
{

QSPI_ID	pQspiId;
   pQspiId = pTheQspiObject;


   printf("Startted probeIdTask()\n");
   /* Even though it was created SEM_FULL,                */
   /* just to be sure and for clarity we take it here too */
   semTake(pQspiId->pQspiSEMBplug,NO_WAIT);

   FOREVER {
      semTake(pQspiId->pQspiSEMBplug,WAIT_FOREVER);/* now wait for interrupt */

 printf("probeIdTask(): taskDelay(%d)\n",sysClkRateGet() * 1);
       /* First wait for the board to power up */
      taskDelay(sysClkRateGet() * 1);

      probeIdRead( 0, 2, probeId);
      logMsg("qspiProbeIdISR(): `%s`\n",probeId);


   }

}

/* end qProbeId.c */
