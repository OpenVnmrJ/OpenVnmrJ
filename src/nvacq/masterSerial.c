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
DESCRIPTION

This library contains board-specific routines for nirvana master serial devices.
There are four serial ports  use the open-core UART IP ns16550.  These
devices are 16550-like. The ns16550Sio driver is used for all devices.
*/

#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "sysLib.h"
#include "drv/sio/ns16552Sio.h"
#include "errno.h"
#include "ioLib.h"
#include "nvhardware.h"

#include "master.h"
#include "fpgaBaseISR.h"

#include "logMsgLib.h"

#define FPGA_UART0_BASE 0x70001000
#define FPGA_UART1_BASE 0x70002000
#define FPGA_UART2_BASE 0x70003000
#define FPGA_UART3_BASE 0x70004000
#define UART_REG_ADDR_INTERVAL   1

LOCAL NS16550_CHAN  ns16550Chan0;
LOCAL NS16550_CHAN  ns16550Chan1;
LOCAL NS16550_CHAN  ns16550Chan2;
LOCAL NS16550_CHAN  ns16550Chan3;

LOCAL NS16550_CHAN *ns16550Chans[4];

#define FpgaFormatStr "/TyMaster/%d"

SIO_CHAN * masterFpgaSerialChanGet ( int channel );   /* serial channel */

createMasterTtyDevs()
{
   STATUS ttyDevCreate ( char * name, SIO_CHAN * pSioChan, int rdBufSize, int wrtBufSize ) ;
   int i, stat;
   char ttyName[32];

   for(i=0; i < 4; i++)
   {
      sprintf(ttyName,FpgaFormatStr,i);
//      logMsg("create: '%s'\n",ttyName);
      if ((stat = ttyDevCreate(ttyName, masterFpgaSerialChanGet(i), 512,512)) != OK)
          errLogSysRet(LOGIT,debugInfo,"createMasterTtyDevs: Could not create : '%s'",ttyName);
   }
}

/***********************************************************************
*
* masterFpgaSerialInit - initialize the Master FPGA serial devices to a quiescent state
*
* This routine initializes the Master FPGA serial device descriptors and puts the
* devices in a quiescent state.  It is called from bringup() with
* interrupts locked.
*
* RETURNS: N/A
*
*/
void masterFpgaSerialInit(void)
{
    /*
     * intialize serial device 0 descriptors (S0)
     */

    ns16550Chan0.regs = (UINT8*) FPGA_UART0_BASE;
    ns16550Chan0.channelMode = SIO_MODE_INT;
    ns16550Chan0.level = (UINT8) FPGA_EXT_INT_IRQ;  /* a wild guess , this looks good  25 */
    ns16550Chan0.regDelta  = UART_REG_ADDR_INTERVAL;
    ns16550Chan0.xtal  = 66666666;    /* 66 MHz */
    ns16550Chan0.baudRate  = 9600;

    /*
     * intialize serial device 1 descriptors (S1)
     */
    ns16550Chan1.regs = (UINT8*) FPGA_UART1_BASE;
    ns16550Chan1.channelMode = SIO_MODE_INT;
    ns16550Chan1.level = (UINT8) FPGA_EXT_INT_IRQ;  /*a wild guess, this looks good  25 */
    ns16550Chan1.regDelta  = UART_REG_ADDR_INTERVAL;
    ns16550Chan1.xtal  = 66666666;
    ns16550Chan1.baudRate  = 9600;

    /*
     * intialize serial device 2 descriptors (S2)
     */
    ns16550Chan2.regs = (UINT8*) FPGA_UART2_BASE;
    ns16550Chan2.channelMode = SIO_MODE_INT;
    ns16550Chan2.level = (UINT8) FPGA_EXT_INT_IRQ;  /*a wild guess, this looks good  25 */
    ns16550Chan2.regDelta  = UART_REG_ADDR_INTERVAL;
    ns16550Chan2.xtal  = 66666666;
    ns16550Chan2.baudRate  = 9600;

    /*
     * intialize serial device 3 descriptors (S3)
     */
    ns16550Chan3.regs = (UINT8*) FPGA_UART3_BASE;
    ns16550Chan3.channelMode = SIO_MODE_INT;
    ns16550Chan3.level = (UINT8) FPGA_EXT_INT_IRQ;  /*a wild guess, this looks good  25 */
    ns16550Chan3.regDelta  = UART_REG_ADDR_INTERVAL;
    ns16550Chan3.xtal  = 66666666;
    ns16550Chan3.baudRate  = 9600;

    /* initialize chan pointer array for interrupt service routine */
    ns16550Chans[0] = &ns16550Chan0;
    ns16550Chans[1] = &ns16550Chan1;
    ns16550Chans[2] = &ns16550Chan2;
    ns16550Chans[3] = &ns16550Chan3;
    /*
     * reset both devices
     */
    ns16550DevInit(&ns16550Chan0);
    ns16550DevInit(&ns16550Chan1);
    ns16550DevInit(&ns16550Chan2);
    ns16550DevInit(&ns16550Chan3);
}

/*
 * The Pete's way to clear the FPGA interrupt
 */
void fpgaClearUartInt(int uart_num)
{
   *((volatile unsigned int *)(MASTER_BASE+MASTER_uart_int_clear_addr)) = 0;
   *((volatile unsigned int *)(MASTER_BASE+MASTER_uart_int_clear_addr)) = 1<<(MASTER_uart_int_clear_pos + uart_num);
}

/*
 *  This interrupt ISR maybe one of several registered via fpgaIntConnect() the the FPGA
 *  external interrupt.
 * There is a Base ISR that handles reading the interrupt status register
 * it passes this as the 1st argument to all the register ISRs, the 2nd argument 
 * the the argument specified in the fpgaInitConnect call
 * As such the 1st thing it checks is if and uarts need to be serviced, if not it returns
 * immediately so the next ISR on the chain can be called.
 * The base ISR clear the interrupts, Do NOT clear them in register ISRs
 *  FPGA serial port  ISR, all serialport plus all the other interrupt are generated
 *  via the one 405 external interrupt IRQ0.
 *  This routine decide which if any serial ports need servicing add call the Sio
 *  ISR to handle that serial port channel.
 *
 *   Author: greg Brissey  5/7/04
 */
void masterFpgaNs16550Isr(int pendingItrs, int dummy)
{
    extern void ns16550Int ( NS16550_CHAN * pChan);     /* pointer to channel */
    int uarts;
    /* its passed so don't read it */
    /* uarts = get_field(MASTER,uart_int_status); */
    uarts = mask_value(MASTER,uart_int_status,pendingItrs);
//    logMsg("fpga serial ISR: pending: 0x%lx, uarts: 0x%x\n",pendingItrs,uarts,3,4,5,6);

    /* No UARTs need servicing then just return */
    if (uarts == 0)
	return;
    if (uarts & 1)
    {
          /* logMsg("ISR: chan 0\n"); */
          ns16550Int(ns16550Chans[0]);
	  /* fpgaClearUartInt(0); */
    }
    if (uarts & 2)
    {
          /* logMsg("ISR: chan 1\n"); */
          ns16550Int(ns16550Chans[1]);
	  /* fpgaClearUartInt(1); */
    }
    if (uarts & 4)
    {
          /* logMsg("ISR: chan 2\n"); */
          ns16550Int(ns16550Chans[2]);
	  /* fpgaClearUartInt(2); */
    }
    if (uarts & 8)
    {
          /* logMsg("ISR: chan 3\n"); */
          ns16550Int(ns16550Chans[3]);
	  /* fpgaClearUartInt(3); */
    }
    return;
}
/***********************************************************************
*
* masterFpgaSerialInit2 - connect serial device interrupts
*
* This routine connects the serial device interrupts.  It is called from
* sysHwInit2().
*
* RETURNS: N/A
*
*/
void masterFpgaSerialInit2 ( void )
{
    SIO_CHAN * masterFpgaSerialChanGet(int);
    int intLevel = intLock ();


    /*
     * Connect serial interrupt handlers for S0 (UART 0)
     * to the FPGA Base ISR and enable serial interrupts for the
     * FPGA ns16550 serial ports
     */
    (void) fpgaIntConnect ( masterFpgaNs16550Isr, (int) masterFpgaSerialChanGet(0), set_field_value(MASTER,uart_int_status,0xf));

    /* enable serial port interrupts */
    set_field(MASTER,uart_int_enable,0xf); /* enable all 4 uarts */
    /* clear the interrupts */
    *((volatile unsigned int *)(MASTER_BASE+MASTER_uart_int_clear_addr)) = 0;
    *((volatile unsigned int *)(MASTER_BASE+MASTER_uart_int_clear_addr)) = 0xf << MASTER_uart_int_clear_pos;

    intUnlock (intLevel);
}

/***********************************************************************
*
* fpgaSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* This routine returns a pointer to the SIO_CHAN device associated
* with a specified serial channel.  It is called by usrRoot() to obtain
* pointers when creating the system serial devices, `/tyCo/x'.  It
* is also used by the WDB agent to locate its serial channel.
*
* RETURNS: A pointer to the SIO_CHAN structure for the channel, or ERROR
* if the channel is invalid.
*
*/
SIO_CHAN * masterFpgaSerialChanGet ( int channel )   /* serial channel */
{
    switch (channel)
    {
        case 0:                                 /* S0 */
            return ((SIO_CHAN *)&ns16550Chan0);

        case 1:                                 /* S1 */
            return ((SIO_CHAN *)&ns16550Chan1);

        case 2:                                 /* S2 */
            return ((SIO_CHAN *)&ns16550Chan2);

        case 3:                                 /* S3 */
            return ((SIO_CHAN *)&ns16550Chan3);

        default:
            return ((SIO_CHAN *)ERROR);
    }
}

testSerial(int port1, int port2)
{
  int i,sport1,sport2,bytes;
  char tty1Name[32], tty2Name[32];
  char rbuf[128];
  char *tststr = "This is a test. 1234567890\n";
  if (port1 == port2)
  {
      printf("ports must be different.\n");
      return;
  }
  sprintf(tty1Name,FpgaFormatStr,port1);
  sprintf(tty2Name,FpgaFormatStr,port2);
//  printf("opening dev: '%s', and '%s'\n",tty1Name,tty2Name);
  sport1 = open(tty1Name, O_RDWR, 0);
  if(sport1 < 1) 
  {
     perror("open:");
     printf("failed to open: '%s'\n",tty1Name);
     return(-1);
  }
  sport2 = open(tty2Name, O_RDWR, 0);
  if(sport2 < 1) 
  {
     perror("open:");
     printf("failed to open: '%s'\n",tty2Name);
     return(-1);
  }
//   printf("writing to tty %d\n",sport1);
  bytes = write(sport1,tststr,strlen(tststr));
//  printf("wrote %d bytes\n",bytes);
//  printf("reading from tty %d\n",sport2);
  bytes = read(sport2,rbuf,bytes);
  
  for (i=0; i<bytes; i++)
  { printf("%c", rbuf[i]);
  }
  printf("'\n");
  if (strcmp(rbuf,tststr) == 0)
      printf("      ===>> PASSED\n");
  else
      printf("      ===>> FAILED\n");
  close(sport1);
  close(sport2);
}

void mttyInit()
{
void initTty();
    initTty();	/* for backwards compatibility */
}

void initTty()
{
    masterFpgaSerialInit();
    masterFpgaSerialInit2();
    createMasterTtyDevs();
}

#define CR 13
#define EOM 3
#define PERIOD 46
#define LF 10

sspew(int times)
{
    int fd,i,len,port;
    char ttyName[16];
    char *str = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    port = 0;
    sprintf(ttyName,FpgaFormatStr,port);
    printf("opening dev: '%s'\n",ttyName);
    fd = open(ttyName, O_RDWR, 0);
    if(fd < 1)
    {
       perror("open:");
       printf("failed to open: '%s'\n",ttyName);
       return(-1);
    }
    len = strlen(str);
    for(i=0; i < times; i++)
      write(4,str,len);

    close(fd);
}

int sfd;
tpmode(int port)
{
    char ttyName[16];
    char chrStr[80],replyStr[80];
    int len,i;

    sprintf(ttyName,FpgaFormatStr,port);
    printf("opening dev: '%s'\n",ttyName);
    sfd = open(ttyName, O_RDWR, 0);
    if(sfd < 1) 
    {
       perror("open:");
       printf("failed to open: '%s'\n",ttyName);
       return(-1);
    }
    sprintf(ttyName,FpgaFormatStr,1);
    sprintf(ttyName,FpgaFormatStr,2);
    sprintf(ttyName,FpgaFormatStr,3);
    while(1)
    {
    printf("string to issue: ");
    fioRdString (STD_IN, &chrStr, 80);
    printf("sending: '%s'\n",chrStr);
    len = strlen(chrStr);
    for(i=0;i<len;i++)  
        printf("char: '%c', 0x%x\n",chrStr[i],chrStr[i]);
    printf("strlen: %d\n",len);
    chrStr[len] = 13; 
    chrStr[len+1] = 10;
    chrStr[len+2] = 0;
    write(sfd,chrStr,len+2);
    fioRdString(sfd,replyStr,80);
    len = strlen(replyStr);
    taskDelay(calcSysClkTicks(500));  /* 1/2 sec, taskDelay(30); */
    printf("reply: %d bytes  '%s' \n",len,replyStr);
    }
}
tpclose()
{
   close(sfd);
}

polltx()
{
    int i,len,doit;
    SIO_CHAN *pSioChan, *pSioChan2; /* serial I/O channel */
    char *text = "0123456789abcdefghijklmnopqurstuwxyz";
    pSioChan = masterFpgaSerialChanGet(2);
    len = strlen(text);
    doit = 100;
    while(doit--)
    {
    for (i = 0; i < len; i++)
    {
        while (sioPollOutput (pSioChan, text[i]) == EAGAIN);
    }
    }
}
pollTest()
{
    int i;
    char Ichar;
    SIO_CHAN *pSioChan, *pSioChan2; /* serial I/O channel */
    char *text = "0123456789abcdefghijklmnopqurstuwxyz";
    pSioChan = masterFpgaSerialChanGet(0);
    /* ns16550InitChannel(pSioChan);  */
    for (i = 0; i < 4; i++)
    {
	printf("output: %c, 0x%x\n",text[i],text[i]);
        while (sioPollOutput (pSioChan, text[i]) == EAGAIN);
        taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
        if ( sioPollInput (pSioChan, &Ichar) != EAGAIN)
           printf("got: '%c',  0x%x\n",Ichar,Ichar);
    }
/*
    for (i = 0; i < 4; i++)
    {
        while (sioPollInput (pSioChan, &Ichar) == EAGAIN);
	printf("got: %c\n",Ichar);
    }
*/
}
pollTest2()
{
    int i;
    char Ichar;
    SIO_CHAN *pSioChan, *pSioChan2; /* serial I/O channel */
    char *text = "test";
    pSioChan = masterFpgaSerialChanGet(2);
    pSioChan2 = masterFpgaSerialChanGet(3);
    /* ns16550InitChannel(pSioChan);  */
    for (i = 0; i < 4; i++)
    {
	printf("output: %c, 0x%x\n",text[i],text[i]);
        while (sioPollOutput (pSioChan, text[i]) == EAGAIN);
        taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
        if ( sioPollInput (pSioChan2, &Ichar) != EAGAIN)
           printf("got: '%c',  0x%x\n",Ichar,Ichar);
    }
/*
    for (i = 0; i < 4; i++)
    {
        while (sioPollInput (pSioChan, &Ichar) == EAGAIN);
	printf("got: %c\n",Ichar);
    }
*/
}

rs232test()
{
int answer;
      while (1)
      {
         printf(" The test should produce:\n");
         printf(" 'This is a test. 1234567890\\n'\n\n");
         printf("1 Port 0 is connected to port 2 via a NullModem\n");
         printf("2 Port 1 is connected to port 3 via a NullModem\n");
         printf("0 exit\n");

         printf("\nEnter selection: ");
         answer = getchar() - '0';
         printf("\n\n");
         switch (answer) 
         {
         case 0:
	      printf("Goodbye\n\n");
              return;
	      break;
         case 1:
              printf("Transmitting from port 0 to port2\n");
              testSerial(0,2);
              printf("Transmitting from port 2 to port0\n");
              testSerial(2,0);
	      break;
         case 2:
              printf("Transmitting from port 1 to port3\n");
              testSerial(1,3);
              printf("Transmitting from port 3 to port1\n");
              testSerial(3,1);
	      break;
         default:
	      break;
         }
         getchar();
      }
}
