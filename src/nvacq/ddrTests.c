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
=========================================================================
FILE: ddrTests.c 4.1 03/21/08
=========================================================================
PURPOSE:
	Provide a file for board bringup and other misc. test  routines 

Externally available modules in this file:

Internal support modules in this file:

COMMENTS:

AUTHOR:	

*/
#include <vxWorks.h>
#include <stdlib.h>
#include <msgQLib.h>
#include "nvhardware.h"
#include "dmaDrv.h"
#include "ddr.h"
#include "ddr_fifo.h"

#define LATCHKEY (1 << 31)
#define DURATIONKEY (1 << 26)
#define GATEKEY (2 << 26)
#define DDRGAINKEY (3 << 26)
#define DDRAUXKEY (9 << 26)

static int *srcDataBufPtr = NULL;
static int *cpybkDataBufPtr = NULL;
static int *dstdataBufPtr = NULL;
static char *bufferZone = NULL;
static unsigned int calcChksum;

/*
 * build some data buffers for the tests
 */
builddmabuf()
{
     int i;
     char *bufferZone;
      int size;

     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     srcDataBufPtr = (int*) malloc(1024*1024); /* max DMA transfer in single shot */
     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     cpybkDataBufPtr = (int*) malloc(1024*1024); /* max DMA transfer in single shot */
     dstdataBufPtr = (int*) malloc(1024*1024); /* max DMA transfer in single shot */
     bufferZone = (char*) malloc(1024); /* max DMA transfer in single shot */
     printf("bufaddr: 0x%08lx\n",srcDataBufPtr);
     size = (1024*1024)/4;
     for (i=0;i<size; i++)
     {
	srcDataBufPtr[i] = i;
	cpybkDataBufPtr[i] = 0;
        dstdataBufPtr[i] = 0;
     }
     /* calcChksum = addbfcrc((char*)srcDataBufPtr,64000); */
}

/*
 * routine to zero values in the buffer that the data is copied back to
 */
clrcpyback()
{
     int i,size;
     size = (1024*1024)/4;
     for (i=0;i<size; i++)
     {
	cpybkDataBufPtr[i] = 0;
     }
}
/*
 * zero the values in the local Destination buffer
 */
clrlocaldstbuf(int size)
{
   int i;
   long *dstAddr;
   dstAddr = (long *) dstdataBufPtr;
   for(i=0;i<size;i++)
     *dstAddr++ = 0;
}

/*
 * fill data space with address of memory location on the C67 side of the HPI
 */
filldspwaddr()
{
   int i,size;
   long *dstAddr;
   size = 0x1000000;    /* 64MB in words, 64MB = 0x4000000 */

   dstAddr = (long *) 0x90000000;
   for(i=0;i<size;i++,dstAddr++)
     *dstAddr = dstAddr;
}


/*
 * zero values on the C67 side of the HPI
 */
clrdspbuf(int size)
{
   int i;
   long *dstAddr;
   dstAddr = (long *) 0x90000000;
   for(i=0;i<size;i++)
     *dstAddr++ = 0;
}

/*
 * zero values on the FPGA memory buffer 
 */
clrbufdspbuf(int size)
{
   int i;
   long *dstAddr;
   dstAddr = (long *) 0x70002000;
   for(i=0;i<size;i++)
     *dstAddr++ = 0;
}
/*
 * just print what was copied back 
 */
prtcpyback(int size)
{
     int i;
     for (i=0;i<size; i++)
     {
	printf("cpyback[%d] = %d\n",i,cpybkDataBufPtr[i]);
     }
}


/*
 * Simple programmed IO for sanity check 
 */
ddrpio(int size)
{
   int i;
   long *srcAddr;
   long *dstAddr;

   if (srcDataBufPtr == NULL)
       builddmabuf();
      
   clrdspbuf(size);

   srcAddr = (long *) srcDataBufPtr;
   dstAddr = (long *) 0x90000000;
   for(i=0;i<size;i++)
     *dstAddr++ = *srcAddr++;
   
   clrcpyback();

   srcAddr = (long *) 0x90000000;
   dstAddr = (long *) cpybkDataBufPtr;

   for(i=0;i<size;i++)
     *dstAddr++ = *srcAddr++;

   for(i=0;i<size;i++)
   {
      if (srcDataBufPtr[i] != cpybkDataBufPtr[i])
      {
          printf("Orig: %d, 0x%lx, CopyBack: %d 0x%lx\n",srcDataBufPtr[i],srcDataBufPtr[i],cpybkDataBufPtr[i],cpybkDataBufPtr[i]);
      }
   }

}


/*
 * Simple programmed IO for sanity check 
 */
ddrbufpio(int size)
{
   int i;
   long *srcAddr;
   long *dstAddr;

   if (srcDataBufPtr == NULL)
       builddmabuf();
      
   clrbufdspbuf(size);

   srcAddr = (long *) srcDataBufPtr;
   dstAddr = (long *) 0x70002000;
   for(i=0;i<size;i++)
     *dstAddr++ = *srcAddr++;
   
   clrcpyback();

   srcAddr = (long *) 0x70002000;
   dstAddr = (long *) cpybkDataBufPtr;

   for(i=0;i<size;i++)
     *dstAddr++ = *srcAddr++;

   for(i=0;i<size;i++)
   {
      if (srcDataBufPtr[i] != cpybkDataBufPtr[i])
      {
          printf("Orig: %d, 0x%lx, CopyBack: %d 0x%lx\n",srcDataBufPtr[i],srcDataBufPtr[i],cpybkDataBufPtr[i],cpybkDataBufPtr[i]);
      }
   }

}

/* 
 * paced = 0 standard memory-to-memory dma, paced = 1 paced memory-to-memory dma
 * size in words
 * dpsmemflag = 0 use local 405 memory as dstAddr, dspmemflag = 1 use DSP memory address as distination 
 */
ddrdma(int paced,int size, int dspmemflag)
{
   int i,status,cntdown;
   int dmaChannel;
   long *srcAddr;
   long *dstAddr;

   /* int xfersize = 64000; */
   int xfersize;

   if (srcDataBufPtr == NULL)
       builddmabuf();

   clrdspbuf(size);
   clrlocaldstbuf(size);
   clrbufdspbuf(size);
      
   dmaChannel = getDataDmaChan();
   srcAddr = (long *) srcDataBufPtr;
   if (dspmemflag == 2)
     dstAddr = (long *) 0x70002000;
   else if (dspmemflag == 1)
     dstAddr = (long *) 0x90000000;
   else
     dstAddr = (long *) dstdataBufPtr;

   if(paced)
     status = dmaXfer(dmaChannel, MEMORY_TO_MEMORY_DST_PACED /* MEMORY_TO_MEMORY ,FPGA_TO_MEMORY */, NO_SG_LIST, 
		(UINT32) srcAddr, (UINT32) dstAddr, size, NULL, NULL);
   else
     status = dmaXfer(dmaChannel, MEMORY_TO_MEMORY /* MEMORY_TO_MEMORY ,FPGA_TO_MEMORY */, NO_SG_LIST, 
		(UINT32) srcAddr, (UINT32) dstAddr, size, NULL, NULL);

   clrcpyback();

   if (dspmemflag == 2)
     srcAddr = (long *) 0x70002000;
   else if (dspmemflag == 1)
      srcAddr = (long *) 0x90000000;
   else
      srcAddr = (long *) dstdataBufPtr;

   dstAddr = (long *) cpybkDataBufPtr;

   if(paced)
      status = dmaXfer(dmaChannel, MEMORY_TO_MEMORY_SRC_PACED /* MEMORY_TO_MEMORY ,FPGA_TO_MEMORY */, NO_SG_LIST, 
		(UINT32) srcAddr, (UINT32) dstAddr, size, NULL, NULL);
   else
      status = dmaXfer(dmaChannel, MEMORY_TO_MEMORY /* MEMORY_TO_MEMORY ,FPGA_TO_MEMORY */, NO_SG_LIST, 
		(UINT32) srcAddr, (UINT32) dstAddr, size, NULL, NULL);

   cntdown = 20;
   while ( (dmaGetDeviceActiveStatus(dmaChannel) == 1) && (cntdown-- > 0) )
     taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */

  if (cntdown < 1)
    printf("\n=====>>>  Timed out waiting for DMA to complete. <<<========\n\n\n");
  else 
  {
   printf("Comparing src to copy back values.\n");
   for(i=0;i<size;i++)
   {
      if (srcDataBufPtr[i] != cpybkDataBufPtr[i])
      {
          printf("Orig: %d, 0x%lx, CopyBack: %d 0x%lx\n",srcDataBufPtr[i],srcDataBufPtr[i],cpybkDataBufPtr[i],cpybkDataBufPtr[i]);
      }
   }
  }
}

/*  Uses the  DMARequest reigster to enable or disable DMA
    via the Device paced feature of DMA on the 405   
 */
dmaa()
{
    int *dmaReg;
    dmaReg = (int*) get_pointer(DDR,DMARequest);
    printf("dmaReqAddr: 0x%lx\n",dmaReg);
}
dmaon()
{
    int *dmaReg;
    dmaReg = (int*) get_pointer(DDR,DMARequest);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    set_register(DDR,DMARequest,1);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    /* set_field(DDR,dma_request,0); */
   /* printf("DMA_Request Reg: %d\n",get_register(DDR,dma_request)); */
   printf("DMA_Request Reg: %d\n",get_register(DDR,DMARequest));
}

dmaoff()
{
    int *dmaReg;
    dmaReg = (int*) get_pointer(DDR,DMARequest);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    set_register(DDR,DMARequest,0);
    printf("dmaReqAddr: 0x%lx, value: %d\n",dmaReg,*dmaReg);
    /* set_field(DDR,dma_request,1); */
   /* printf("DMA_Request Reg: %d\n",get_register(DDR,dma_request)); */
   printf("DMA_Request Reg: %d\n",get_register(DDR,DMARequest));
}

dmatgl()
{
    set_register(DDR,DMARequest,1);   /* on */
    set_register(DDR,DMARequest,0);   /* off */
}

/*
 * Everything you wanted to know and MORE
 */
chkdma()
{
   int dmaChannel, dmaReqsQd, free2Q;
   dmaChannel = getDataDmaChan();
   dmaReqsQd = dmaReqsInQueue(dmaChannel);
   free2Q = dmaReqsFreeToQueue(dmaChannel);
   printf("Data Transfer: DMA Channel: %d\n",dmaChannel);
   printf("Data Transfer: DMA_Request Reg: %d\n",get_field(DDR,dma_request));
   printf("Data Transfer: DMA device paced pended: %d\n",dmaGetDevicePacingStatus(dmaChannel));
   printf("Data Transfer: DMA Request Queued: %d, Remaining Queue Space: %d\n",dmaReqsQd, free2Q);
   printf("\n\n");
   dmaDebug(dmaChannel);
}

long *pLocOrgBuf = NULL;
long *pLocDsp2PccBuf = NULL;
long *pDspBuf = NULL;
long dbytesize = 0;
MSG_Q_ID  pTstDataAddrMsgQ = NULL;

dataxfrtst(int times)
{
   unsigned long orgCRC, dspCRC, dmaXfrCRC;
   int i,itr,status;
   int dmaChannel;

   long *tmpptr;
   long *srcptr;
   long *dstptr;
   long *rtnAddr;

   dmaChannel = getDataDmaChan();

   dbytesize = 20*64000*4;
   printf("Create a data buffer of %d bytes or %d words\n",dbytesize,dbytesize/4);

   if (pTstDataAddrMsgQ == NULL)
      pTstDataAddrMsgQ = msgQCreate(10,4,MSG_Q_FIFO);

   if (pLocOrgBuf == NULL)
      pLocOrgBuf = (long *) malloc(20*64000*4);

   if (pLocDsp2PccBuf == NULL)
      pLocDsp2PccBuf = (long *) malloc(20*64000*4);

   pDspBuf = (long *) 0x90000000;

   printf("pLocOrgBuf: 0x%lx, pLocDsp2PccBuf: 0x%lx, pDspBuf: 0x%lx\n",pLocOrgBuf,pLocDsp2PccBuf,pDspBuf);

   printf("\nFill pLocOrgBuf with ramp, %ld bytes, %ld words\n", dbytesize, dbytesize/4);
   for (i=0, tmpptr = pLocOrgBuf; i < dbytesize/4; i++)
      *tmpptr++ = i; 

   orgCRC = addbfcrc(pLocOrgBuf,dbytesize);
   printf("pLocOrgBuf Data CRC: 0x%lx\n\n",orgCRC);

   printf("Transfer data via PIO to DSP memory 0x%lx, standby.. ",pDspBuf);
   for (i=0, srcptr = pLocOrgBuf, dstptr = pDspBuf; i < dbytesize/4; i++)
      *dstptr++ = *srcptr++; 
    printf(" done\n");

   printf("Dsp CRC: ");
   dspCRC = addbfcrc(pDspBuf,dbytesize);
   printf(" 0x%lx\n\n",dspCRC);
   if ( orgCRC != dspCRC)
   {
      printf(" -----> CRC Error, transfer to DSP memory failed!!!!   <----------\n");
      printf(" org: 0x%lx != dsp: 0x%lx\n",orgCRC,dspCRC);
      return -1;
   }
   
   printf("Dma from DSP to 405 memory, %d words\n",dbytesize/4);

   for (itr=0; itr < times; itr++)
   {
      printf("%d: DMA from DSP to PPC405",itr+1);
      memset(pLocDsp2PccBuf,0,dbytesize);  /* zero dst memory */
      status = dmaXfer(dmaChannel, MEMORY_TO_MEMORY, NO_SG_LIST, 
		(UINT32) pDspBuf, (UINT32) pLocDsp2PccBuf, dbytesize/4, NULL, pTstDataAddrMsgQ);

      printf(" Standby...");
      msgQReceive(pTstDataAddrMsgQ, (char*) &rtnAddr, 4, WAIT_FOREVER);

      printf("  Finished...");


      dmaXfrCRC = addbfcrc(pLocDsp2PccBuf,dbytesize);
      printf("  CRC: 0x%lx\n", dmaXfrCRC);
      if (dmaXfrCRC != orgCRC)
      {
          printf("CRC Error: iteration: %d, org CRC - dma CRC: 0x%lx  0x%lx\n",itr+1,orgCRC,dmaXfrCRC);
      }

      /* if ((itr+1) == 20)    just to force an error 
         *pDspBuf = 42; */

  }
     
 return 0;

}
      
tstBadOpCode(int opcode)
{
    int badopcode;
    badopcode = (opcode << 26);
    setFifoOutputSelect( 1 ); /* switch to FIFO control */
    cntrlFifoReset();
    cntrlClearInstrCountTotal();
    cntrlFifoCumulativeDurationClear();
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(badopcode);  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,320));  /* 2usec */
    cntrlFifoPut(encode_DDRSetDuration(1,0));  /* 2usec */

    taskDelay(calcSysClkTicks(83));  /* taskDelay(5); */
    printf("fifo instructions reciev'd: %d\n",cntrlInstrCountTotal());
    cntrlFifoStart();
}


struct DDRHOLD {
  int duration;
  int gates;
  int gain;
  int aux;
}  DDR_state;
 
unsigned long totalduration;
extern unsigned long long cumduration;
extern unsigned long long fidduration;

fifoDecode(unsigned int word,int noprt)
{
   void ddrdecode(unsigned int word,int noprt);
   ddrdecode(word,noprt);
}

void ddrdecode(unsigned int word,int noprt)
{
  int tmp,tdata,latched;
  tmp = word;

   if (noprt != 1)
      printf(" 0x%8lx : ",word);
   if (tmp & LATCHKEY)
      latched = 1;
   else
      latched = 0;
   tdata = tmp & 0x3ffffff; /* 26 bits of data */
   tmp &= 0x7C000000;
   switch (tmp)
   {
               case DURATIONKEY:
   		 if (noprt != 1)
                    printf("duration of %7.4f usec",((float) tdata)/80.0);
                 DDR_state.duration = tdata;  break;
               case GATEKEY:
   		 if (noprt != 1)
                    printf("mask/gate set to %x",tdata&0xfff);
                 DDR_state.gates = tdata; break;
               case DDRGAINKEY:
   		 if (noprt != 1)
                     printf("rf phase pattern set to %x",tdata&0xffff);
                  DDR_state.gain = tdata & 0x1F; break;
               case DDRAUXKEY:
   		 if (noprt != 1)
                    printf("aux addr,data = %x,%x\n",(tdata&0xf)>>8, \
                        tdata & 0xff); break;
            default:
                  printf("don't recognize key!! %x\n",tmp);
           }
          if (latched)
          {
   	     if (noprt != 1)
                printf(" fifo word latched\n");
             if (tmp != DDRAUXKEY)
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of %9.4lf usec  GATE = %4x  ", \
                    ((float) DDR_state.duration)/80.0,DDR_state.gates);
               totalduration = totalduration + DDR_state.duration;
             }
             else
             {
   	       if (noprt != 1)
                  printf("OUTPUT STATE of 0.050 usec AUX  GATE = %4x  ", \
                   DDR_state.gates);
               totalduration = totalduration + 4;
             }
	/*
             cumduration += (unsigned long long) totalduration;
             fidduration += (unsigned long long) totalduration;
             printf("cumduration: %llu, fidduration: %llu, duration: %lu\n",cumduration,fidduration,totalduration);
	*/
          }
          else { 
   	     if (noprt != 1)
               printf("\n");
           }
}

long getDecodedDuration()
{
   return(totalduration); 
} 
int clrCumTime()
{
   cumduration = 0LL;
}
int clrFidTime()
{
   fidduration = 0LL;
}
int clrBufTime()
{
   totalduration = 0L;
}

int prtFifoTime(char *idstr)
{
   if (idstr != NULL)
      printf("----->>  '%s'\n",idstr);
   printf("----->>  Durations: Cum: %llu ticks, %18.4f,   \n----->>             FID: %llu ticks, %18.4f,   Buffer: %lu ticks, %7.4f usec\n\n",
		cumduration, ((double) cumduration) / 80.0, fidduration, ((double) fidduration) / 80.0, totalduration, 
	        ((float) totalduration)/80.0);
}




#define INCLUDE_TESTING
#ifdef INCLUDE_TESTING
/*  test routines */

#endif
