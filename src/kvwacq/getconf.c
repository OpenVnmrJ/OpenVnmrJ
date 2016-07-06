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
#include <vxWorks.h>
#include <msgQLib.h>
#include <vme.h>
#include <semLib.h>
#include "config.h"
#include "hardware.h"
#include "timeconst.h"
#include "adcObj.h"
#include "autoObj.h"
#include "fifoObj.h"
#include "stmObj.h"
#include "vtfuncs.h"
#include "spinObj.h"
#include "hostAcqStructs.h"
#include "logMsgLib.h"

#define TIMEOUT 98
#define AP_ADDR_UNKNOWN -1		
#define NOT_CONFIGURED	0x1ff		/* unknown board config value	*/
					/* inserted in status block	*/

/*  Offsets into non-volatile RAM  */

#define  CONFIG_OFFSET	0x500
#define  DEBUG_OFFSET	0x800


extern ADC_ID	pTheAdcObject;
extern AUTO_ID	pTheAutoObject;
extern FIFO_ID	pTheFifoObject;
extern SPIN_ID	pTheSpinObject;
extern MSG_Q_ID	pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */

/* Revision Number for Interpreter & System */
extern int SystemRevId; 
extern int InterpRevId;

extern int shimSet;

struct	_conf_msg {
	long	msg_type;
	struct	_hw_config hw_config;
} conf_msg;

static unsigned int rf_wfg_addr[4] = {
	 0xc10, 0xc18, 0xc48, 0xc40,
	} ;

#define NUM_GRAD_DEVS	4
#define NUM_GRAD_CONFIGS 	NUM_GRAD_DEVS+2
#define NUM_GRAD_AXIS	4

static unsigned int grad_addr[NUM_GRAD_DEVS][NUM_GRAD_AXIS] = {
	{ 0xC50, 0xC54, 0xC58, 0xC5C},
	{ 0xC60, 0xC64, 0xC68, 0xC6C},
	{ 0xC88, 0xC88, 0xC88, 0xC8C},
	{ 0xC20, 0xC28, 0xC30, 0xC38},
	} ;

static char *grad_set_name[NUM_GRAD_DEVS] = {
	"Performa II",
	"Performa I ",
	"L200       ",
	"WFG        "
	} ;

static char grad_config[NUM_GRAD_CONFIGS] = {
	'p',
	'l',
	't',
	'w',
	'q',
	'u'
	} ;

static char grad_axis[NUM_GRAD_AXIS] = { 'X', 'Y', 'Z', 'R'};
/**********************************************************************
*
*/
void getConfig()
{
char	*probePtr;
char	char_value;
short	short_value;
short	*tmp2;
short	nvRamShort[2];	/* sysNvRamGet append a NULL so we need 2 */
int	i;
int	int_value;
int	tmp_apbyte;
int	tptime;
int	temp1,temp2,temp3;
long	*lockfreq;

/* INITIALIZE */
/* initialize all integers of the hw_config structure to unknown/not present */
   tmp2 = (short *) &conf_msg.hw_config;
   for (i=0; i<sizeof( struct _hw_config ) / sizeof( short ); i++)
        *tmp2++ = 0x1ff;                /* signefies unknown/not present */

   conf_msg.hw_config.valid_struct = CONFIG_INVALID;
   conf_msg.hw_config.size   = 1;
   conf_msg.hw_config.system = 7;		/* changed from 6, 3/7/1997 */
   conf_msg.hw_config.SystemVer = (short) SystemRevId;
   conf_msg.hw_config.InterpVer   = (short) InterpRevId;
   conf_msg.hw_config.sram   = PRESENT;

/* DIGITAL */
/* Check if FIFO is present */
   probePtr = (char *) (FIFO_BASE_ADR + FIFO_SR);
   if ( vxMemProbe((char*) (probePtr), VX_READ, 2, &short_value) == ERROR)
   {  conf_msg.hw_config.fifolpsize = NOT_PRESENT;
      printf("FIFO: >>> NOT <<< Present\n");
   }
   else
   {  conf_msg.hw_config.fifolpsize = 2048;
      printf("FIFO: Present\n");
   }

/* Check if DTM is present */
   probePtr = (char *) (STM_BASE_ADR + STM_SR);
   if ( vxMemProbe((char*) (probePtr), VX_READ, 1, &short_value) == ERROR)
   {  conf_msg.hw_config.STM_present = NOT_PRESENT;
      printf("STM: >>> NOT <<<  Present\n");
   }
   else
   {  conf_msg.hw_config.STM_present = PRESENT;
      printf("STM: Present\n");
   }

/* Check if ADC is present */
   probePtr = (char *) (ADC_BASE_ADR + ADC_SR);
   if ( vxMemProbe((char*) (probePtr), VX_READ, 2, &short_value) == ERROR)
   {  conf_msg.hw_config.ADCsize = 0;
      printf("ADC: >>> NOT <<<  Present\n");
   }
   else
   {  conf_msg.hw_config.ADCsize = 0x100 + 16; /* 0x100 is speed, 16 is size */
      adcConfigDsp( pTheAdcObject );
      printf("ADC: Present\n");
   }
      
/* Check for Wide Line ADC (> 500 kHz) */
/* As soon as address is known, make this a real entry */
   conf_msg.hw_config.WLadc_present= NOT_PRESENT;

/* Check if VT is present */
/* Done with routine in vtfuncs.c, timeout may take 3-6 secs */
   printf("VT: ");
   if (test4VT() < 0)
   {
      conf_msg.hw_config.VTpresent = NOT_PRESENT;
      conf_msg.hw_config.dspDownLoadAddrs[0] = (void *)0xFFFFFFFF;
      printf(">>> NOT <<< Present\n");
   }
   else
   {
      conf_msg.hw_config.VTpresent = PRESENT;
      conf_msg.hw_config.dspDownLoadAddrs[0] = (void *)0xFFFFFFFF;
      printf("Present\n");
   }

/* Check if Spinner is present */
/* Done with routine in spinObj.c, timeout may take 3-6 secs */
   printf("SPIN: ");
   if (spinCreate() == 0)
   {
      conf_msg.hw_config.spin = NOT_PRESENT;
      printf(">>> NOT <<< Present\n");
   }
   else
   {
      if (pTheSpinObject->spinPort == -1)
      {
         conf_msg.hw_config.spin = NOT_PRESENT;
         printf(" Manual Present\n");
      }
      else
      {
         conf_msg.hw_config.spin = PRESENT;
         printf("Automated Present\n");
      }
   }

/* RF/ANALOG */
/* get the system 'H1freq' from the lock xmtr board, APbus = AA33 */
 
   tmp_apbyte=readApbusRegister(0xA33);
   printf("0x%05x ",tmp_apbyte);
   if (tmp_apbyte == AP_ADDR_UNKNOWN)
   {  printf("Cannot determine 1H frequency, no lock transmitter\n");
   }  
   else
   {  tmp_apbyte &= 0xff;
      if (tmp_apbyte == 0x0f)
      {  printf("Cannot determine 1H frequency from lock xmtr, bits not set\n");
      } 
      else
      {  printf("System 1H frequency is %d Mhz\n",(tmp_apbyte)*100);
      }
      conf_msg.hw_config.H1freq=(tmp_apbyte & NOT_CONFIGURED)<<4;
   }

   tmp_apbyte=readApbusRegister(0xA2B);
   printf("0x%05x ",tmp_apbyte);
   if (tmp_apbyte == AP_ADDR_UNKNOWN)
      printf("BB Obs Rcvr: Unknown\n");
   else
   {  if (tmp_apbyte & 0x80)
         printf("BB Obs Rcvr: with 10 dB pad\n");
      else
         printf("BB Obs Rcvr: no 10 dB pad\n");
   }

   tmp_apbyte=readApbusRegister(0xA13);
   if (tmp_apbyte != AP_ADDR_UNKNOWN)
   {  printf("0x%05x ",tmp_apbyte);
      if (tmp_apbyte < 4 )
         printf("RefGen: 4-Nucleus\n");
      else
         printf("RefGen: 5-Nucleus\n");
   }
   else
   {  tmp_apbyte=readApbusRegister(0xA0B);
      printf("0x%05x ",tmp_apbyte);
      if (tmp_apbyte != AP_ADDR_UNKNOWN)
         printf("RefGen: Broadband\n");
      else
         printf("RefGen: Unknown\n");
   }
   
   tmp_apbyte=readApbusRegister(0xA53);
   printf("0x%05x ",tmp_apbyte);
   if (tmp_apbyte == AP_ADDR_UNKNOWN)
   {  printf("Homo Decoupler NOT PRESENT\n");
      conf_msg.hw_config.homodec = 0;
   }  
   else
   {  printf("Homo Decoupler PRESENT\n");
      conf_msg.hw_config.homodec = 1;
   }

   tmp_apbyte=readApbusRegister(0xA43);
   printf("0x%05x ",tmp_apbyte);
   if (tmp_apbyte == AP_ADDR_UNKNOWN)
   {  printf("Shims NOT PRESENT\n");
   }  
   else
   {  printf("Shims PRESENT\n");
   }

    for (i=0; i<4; i++)
    {  if (tmp_apbyte & (1<<i) )
      {  if (sysNvRamGet(nvRamShort, 2, (PTS_OFFSET + i)*2 + 0x500) != OK)
	    printf("Trouble reading NvRam for PTS values\n");
/*         printf(" %d ",nvRamShort[0]); */
         conf_msg.hw_config.sram_val[ PTS_OFFSET+i ] = nvRamShort[0];
      }
    } 
   conf_msg.hw_config.PTSes_present = NOT_CONFIGURED;

   printf("0x%05x Amplifiers: ",conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]);
   if (conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]/10 == 1)
      printf(" 35 W-Hi ");
   else if (conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]/10 == 2)
      printf(" 75 W-Hi ");
   else if (conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]/10 == 3)
      printf(" 125 W-Hi ");
   if (conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]%10 == 1)
      printf(" 35 W-Lo ");
   else if (conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]%10 == 2)
      printf(" 125 W-Lo ");
   else if (conf_msg.hw_config.sram_val[ PTS_OFFSET+2 ]%10 == 3)
      printf(" 300 W-Lo ");
   printf("\n");

/* next we check for xmtrs present by reading AP bus AA1B, AA03 */
   tmp_apbyte  = ((i=readApbusRegister(0xa1B)) < 0)?  0:1; /* hb-xmtr 1*/
   if ( ! tmp_apbyte)
   {  tmp_apbyte  = ((i=readApbusRegister(0x1FF)) < 0)?  0:1;
      if (tmp_apbyte) i |= 0x10;	/* if response, force to new xmtr */
   }
   conf_msg.hw_config.xmtr_stat[0] = i & NOT_CONFIGURED;
   tmp_apbyte |= ((i=readApbusRegister(0xa03)) < 0)?  0:2; /* lb_xmtr 2*/
   conf_msg.hw_config.xmtr_stat[1] = i & NOT_CONFIGURED;
   conf_msg.hw_config.xmtr_present  = tmp_apbyte;

/* Check Transmitters and RF wfgs */
   for (i=0; i<2; i++)
   {  printf(" XMTR %d:",i);
      if (tmp_apbyte & (1<<i) )
      {
	 if (conf_msg.hw_config.xmtr_stat[i] & 0x10)
         {  printf(" 79 dB ");
	    if (!(conf_msg.hw_config.xmtr_stat[i] & 0x80))
               printf(" WFG  ");
            else
               printf(" No-WFG  ");
         }
	 else
	    printf(" 63 dB  ");
      }
      else
         printf(" Unknown  ");
   } 
   printf("\n");

/* next we check the type of shim PS */
   printf("0x%05x Shimtype is ",tmp_apbyte);
   conf_msg.hw_config.shimtype  = shimSet;
   tmp_apbyte = shimSet;
   if (tmp_apbyte == 1) tmp_apbyte=10;  /* ? */
   switch (tmp_apbyte)
   { case  1:
	printf("Varian 13 shims\n");
	break;
     case  2:
	printf("Oxford 18 shims\n");
	break;
     case  3:
	printf("Varian 23 shims\n");
	break;
     case  4:
	printf("Varian 28 shims\n");
	break;
     case  5:
	printf("Ultra Shims\n");
	break;
     case  6:
	printf("Varian 18 shims\n");
	break;
     case  7:
	printf("Varian 20 shims\n");
	break;
     case  8:
	printf("Oxford 15 shims\n");
	break;
     case  9:
	printf("Varian 40 shims\n");
	break;
     case 10:
	printf("Varian 14 shims\n");
	break;
     case 11:
	printf("Whole Body shims\n");
	break;
     default:
	printf("UNKNOWN or TIMEOUT <<<<<<<<<<\n");
	break;
   }


   /* Gradient Info */
   printf("Gradients:");
   for (i=0; i<2; i++)		/* Only test for Performa I for MERCURY */
   {
	int j;
   	printf("\n  %s: ",grad_set_name[i]);
	for (j=0; j<NUM_GRAD_AXIS; j++)
	{
	   tmp_apbyte = readApbusRegister(grad_addr[i][j]);
	   if (tmp_apbyte<0)
	      printf ("%c=No(  %4d)  ",grad_axis[j],tmp_apbyte);
	   else
	   {  printf ("%c=Yes(0x%04x) ",grad_axis[j],tmp_apbyte);
	      if (conf_msg.hw_config.gradient[j] == 'p' && 
						grad_config[i] == 'w')
	      {
	        /* Configuration 'q' = pfg + wfg  */
	      	conf_msg.hw_config.gradient[j]='q';
	      }
	      else if (conf_msg.hw_config.gradient[j] == 't' && 
						grad_config[i] == 'w')
	      {
	        /* Configuration 'u' = L200pfg + wfg  */
	      	conf_msg.hw_config.gradient[j]='u';
	      }
	      else
	      	conf_msg.hw_config.gradient[j]=grad_config[i];
	   }
	}
   }
   printf("\n");
  
/* get 'lockfreq' from SRAM */
   if (sysNvRamGet(nvRamShort, 2, LF_OFFSET*2 + 0x500) != OK)
      printf("Trouble reading NvRam for LK values\n");
   conf_msg.hw_config.sram_val[ LF_OFFSET ] = nvRamShort[0];
   if (sysNvRamGet(nvRamShort, 2, (LF_OFFSET + 1)*2 + 0x500) != OK)
      printf("Trouble reading NvRam for LK + 1 values\n");
   conf_msg.hw_config.sram_val[ LF_OFFSET + 1] = nvRamShort[0];
   lockfreq = (long *) &conf_msg.hw_config.sram_val[LF_OFFSET];
   temp1 = *lockfreq/1000;
   temp1 = temp1/1000;
   temp2 = *lockfreq - (temp1*1000*1000);
   temp3 = temp2;
   temp2 = temp2/1000;
   temp3 = temp3 - (temp2 * 1000);
   printf("lockfreq = %d.%03d%03d MHz\n",(int)temp1,(int)temp2,(int)temp3);
 
   conf_msg.hw_config.dspDownLoadAddr = pTheAdcObject->dspDownLoadAddr;
   conf_msg.hw_config.dspPromType = (short) pTheAdcObject->dspPromType;

   conf_msg.hw_config.valid_struct = CONFIG_VALID;
   conf_msg.msg_type = CONF_INFO;
   msgQSend(pMsgesToHost, (char *) &conf_msg, sizeof( conf_msg ),
			 NO_WAIT, MSG_PRI_NORMAL);
}

/**********************************************************************
*
*/
/*check_amt(i)
/*int i;
/*{
/*int tmp_apbyte;
/*   tmp_apbyte=wr_ApbusRegister(0xb37,i);
/*   printf("0x%05x ",tmp_apbyte);
/*   printf("Amplifier %d: ",i);
/*   if (tmp_apbyte == AP_ADDR_UNKNOWN)
/*   {
/*      printf(">>> NO CONTROLLER <<< \n");
/*   }
/*   else
/*   {  tmp_apbyte &= 0xff;
/*      if      (tmp_apbyte==0x0) printf("AMT 3900-11  Lo-Lo band\n");
/*      else if (tmp_apbyte==0x1) printf("AMT 3900-12  Hi-Lo band\n");
/*      else if (tmp_apbyte==0x4) printf("AMT 3900-1S  Lo band\n");
/*      else if (tmp_apbyte==0x8) printf("AMT 3900-1   Lo band\n");
/*      else if (tmp_apbyte==0xb) printf("AMT 3900-1S4 HI-Lo band\n");
/*      else if (tmp_apbyte==0xc) printf("AMT 3900-15  Hi-Lo band(100W)\n");
/*      else if (tmp_apbyte==0xf) printf("None\n");
/*      else                      printf("Unknown\n");
/*   }  
/*   return (tmp_apbyte & NOT_CONFIGURED);
/*}
/* NOMERCURY */
/**********************************************************************
*
*/
startGetConfig( int taskpriority, int taskoptions, int stacksize )
{
void getConfig();

   if (taskNameToId("tGetConf") == ERROR)
     taskSpawn("tGetConf",taskpriority,taskoptions,stacksize,getConfig,
		1,2,3,4,5,6,7,8,9,10);
}


/**********************************************************************
*
*  These programs work together to store key information about the
*  hardware of the MERCURY console in the Non-Volatile RAM of the 162.
*  The console programs can store this information at selected times
*  (for example, before rebooting itself).  At system startup the
*  information stored in the non-volatile RAM can then be passed on
*  to the host computer for future referece.		August 1996
*
*
*  When an application requests that console hardware debug data be
*  stored, it is required to specify an index.  This is so multiple
*  copies can be stored in NVRAM, with individual copies referenced
*  by their index.  The index implies under what context the data
*  was stored.  See hostAcqStructs.h for some symbolic defines.
*
*  Most of the entry points are declared as statics, since there is
*  no need for them to be referenced from outside this file.
*
*  storeConsoleDebug is used by other console applications to
*  store this information in the NVRAM.
*
*  sendConsoleDebug is used to send information previously stored
*  in the NVRAM to the host computer, using the "connection handler"
*  channel.
*
*  Use testConsoleDebug to collect current information from the
*  hardware and display it on the console monitor.
*
*  Use showConsoleDebugNVRAM (with an index) to display the
*  information currently stored in NVRAM.
*							September 1996
*/

#define  CONSOLE_DEBUG_LEVEL	9

static CDB_BLOCK  cdb_msg;


/*  This is a special program for use with the Console Debug stuff only.  */

static int
getLongFromTwoShortRegs( volatile short *highAddr, volatile short *lowAddr, long *valAddr )
{
	short		 lOrder, hOrder;

/*  Beware:  byte order requires these short words registers
             be read into short word variables.			*/

	if (vxMemProbe( (char *) highAddr, VX_READ, 2, &hOrder ) == ERROR)
	  return( -1 );
	if (vxMemProbe( (char *) lowAddr, VX_READ, 2, &lOrder ) == ERROR)
	  return( -1 );

	*valAddr = (hOrder << 16) | lOrder;
	return( 0 );
}

static int
getStmDebug( STM_DEBUG *stmdbp, int index )
{
	volatile short	*tmpsAddr, *highAddr, *lowAddr;
	short		 lOrder, hOrder;
	STMOBJ_ID	 pStmId;

	pStmId = stmGetStmObjByIndex( index );
	if (pStmId == NULL || pStmId->stmBaseAddr == 0xFFFFFFFF) {
		DPRINT1( CONSOLE_DEBUG_LEVEL, "STM %d not present\n", index );
		return( 0 );
	}

	tmpsAddr = (short *)STM_STATR(pStmId->stmBaseAddr);
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 2, &stmdbp->stmStatus) == ERROR)
	  return( 0 );
	DPRINT2( CONSOLE_DEBUG_LEVEL,
	   "STM %d Status register: 0x%04x\n", index, stmdbp->stmStatus );

	tmpsAddr = (short *)STM_TAG(pStmId->stmBaseAddr);
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 2, &stmdbp->stmTag) == ERROR)
	  return( 0 );
	DPRINT2( CONSOLE_DEBUG_LEVEL, "STM %d Tag register: %d\n", index, stmdbp->stmTag );

/*  For some reason the long-word registers on the STM must be accessed as 2 short words  */

	highAddr = (short *)STM_NPHW(pStmId->stmBaseAddr);
	lowAddr = (short *)STM_NPLW(pStmId->stmBaseAddr);
	if (getLongFromTwoShortRegs( highAddr, lowAddr, &stmdbp->stmNpReg) != 0)
	  return( 0 );
	DPRINT2( CONSOLE_DEBUG_LEVEL, "STM %d NP counter: %d\n", index, stmdbp->stmNpReg );

	highAddr = (short *)STM_CTHW(pStmId->stmBaseAddr);
	lowAddr = (short *)STM_CTLW(pStmId->stmBaseAddr);
	if (getLongFromTwoShortRegs( highAddr, lowAddr, &stmdbp->stmNtReg) != 0)
	  return( 0 );
	DPRINT2( CONSOLE_DEBUG_LEVEL, "STM %d NT counter: %d\n", index, stmdbp->stmNtReg );

	highAddr = (short *)STM_SRCHW(pStmId->stmBaseAddr);
	lowAddr = (short *)STM_SRCLW(pStmId->stmBaseAddr);  
	if (getLongFromTwoShortRegs( highAddr, lowAddr, (long *) &stmdbp->stmSrcAddr) != 0)
	  return( 0 );
	DPRINT2( CONSOLE_DEBUG_LEVEL,
	   "STM %d Source address: 0x%08lx\n", index, stmdbp->stmSrcAddr );

	highAddr = (short *)STM_DSTHW(pStmId->stmBaseAddr);
	lowAddr = (short *)STM_DSTLW(pStmId->stmBaseAddr);  
	if (getLongFromTwoShortRegs( highAddr, lowAddr, (long *) &stmdbp->stmDstAddr) != 0)
	  return( 0 );
	DPRINT2( CONSOLE_DEBUG_LEVEL,
	   "STM %d Destination address: 0x%08lx\n", index, stmdbp->stmDstAddr );

	return( 1 );
}

static int
getFifoDebug( CONSOLE_DEBUG *cdbp )
{
	volatile long	*tmplAddr;
	volatile short	*tmpsAddr;

	if (pTheFifoObject == NULL || pTheFifoObject->fifoBaseAddr == 0xFFFFFFFF)
	  return( 0 );

	tmplAddr = (long *) FF_STATR( pTheFifoObject->fifoBaseAddr); 
	if ( vxMemProbe((char*) (tmplAddr), VX_READ, 4, &cdbp->fifoStatus) == ERROR)
	  return( 0 );
	else
	  DPRINT1( CONSOLE_DEBUG_LEVEL, "FIFO status: 0x%08lx\n", cdbp->fifoStatus );

	tmpsAddr = (short *) FF_IMASK( pTheFifoObject->fifoBaseAddr); 
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 2, &cdbp->fifoIntrpMask) == ERROR)
	  return( 0 );
	else
	  DPRINT1( CONSOLE_DEBUG_LEVEL, "FIFO Interrupt Mask: 0x%04x\n", cdbp->fifoIntrpMask );

	tmplAddr = FF_LASTWRD( pTheFifoObject->fifoBaseAddr); 
	if (vxMemProbe((char*) (tmplAddr), VX_READ, 4, &cdbp->lastFIFOword[ 0 ]) == ERROR)
	  return( 0 );
	else if (vxMemProbe((char*) (tmplAddr), VX_READ, 4, &cdbp->lastFIFOword[ 1 ]) == ERROR)
	  return( 0 );
	else if (pTheFifoObject->optionsPresent & HSLINEMEZZ)
	  if (vxMemProbe((char*) (tmplAddr), VX_READ, 4, &cdbp->lastFIFOword[ 2 ]) == ERROR)
	    return( 0 );

	if (pTheFifoObject->optionsPresent & HSLINEMEZZ)
	  DPRINT3( CONSOLE_DEBUG_LEVEL, "FIFO Last Word: 0x%08lx 0x%08lx 0x%08lx",
		 cdbp->lastFIFOword[ 0 ], cdbp->lastFIFOword[ 1 ], cdbp->lastFIFOword[ 2 ] );
	else
	  DPRINT2( CONSOLE_DEBUG_LEVEL, "FIFO Last Word: 0x%08lx 0x%08lx",
		 cdbp->lastFIFOword[ 0 ], cdbp->lastFIFOword[ 1 ] );

	return( 1 );
}

static int
getAutomationDebug( CONSOLE_DEBUG *cdbp )
{
	volatile char	*tmpcAddr;
	volatile short	*tmpsAddr;
	char		 tmpchar;

	if (pTheAutoObject == NULL || pTheAutoObject->autoBaseAddr == 0xFFFFFFFF)
	  return( 0 );

	tmpsAddr = AUTO_STATR(pTheAutoObject->autoBaseAddr);
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 2, &cdbp->autoStatus) == ERROR)
	  return( 0 );
	else
	  DPRINT1( CONSOLE_DEBUG_LEVEL, "Automation Status: 0x%x\n", cdbp->autoStatus );

/*  This HSR register on the automation board may not be readable ...  */

	tmpcAddr = AUTO_HSR_SR(pTheAutoObject->autoBaseAddr);
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 1, &tmpchar) != ERROR)
	  DPRINT1( CONSOLE_DEBUG_LEVEL, "Automation H S & R Status: 0x%x\n", tmpchar );
	cdbp->autoHSRstatus = (short) tmpchar;		     /* avoids byte-order problems */
				 /* which would result if we read 1 char directly into the */
	return( 1 );					/* 16 bit data field autoHSRstatus */
}

static int
getAdcDebug( CONSOLE_DEBUG *cdbp )
{
	volatile short	*tmpsAddr;

	if (pTheAdcObject == NULL || pTheAdcObject->adcBaseAddr == 0xFFFFFFFF)
	  return( 0 );

	tmpsAddr = (short *) ADC_STATR(pTheAdcObject->adcBaseAddr);
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 2, &cdbp->adcStatus) == ERROR)
	  return( 0 );
	else
	  DPRINT1( CONSOLE_DEBUG_LEVEL, "ADC Status: 0x%x\n", cdbp->adcStatus );

	tmpsAddr = ADC_ISTATR(pTheAdcObject->adcBaseAddr);
	if (vxMemProbe((char*) (tmpsAddr), VX_READ, 2, &cdbp->adcIntrpMask) == ERROR)
	  return( 0 );
	else
	  DPRINT1( CONSOLE_DEBUG_LEVEL, "ADC Interrupt Mask: 0x%x\n", cdbp->adcIntrpMask );

	return( 1 );
}

static int
getConsoleDebugFromHardware( CONSOLE_DEBUG *cdbp )
{
	int 	adcOk, automationOk, fifoOk;
	int	iter;
	long	currentSysConf;

	cdbp->timeStamp = time( NULL );
	cdbp->Acqstate = get_acqstate();
	cdbp->revNum = SystemRevId; 
	cdbp->startupSysConf = getStartupSysConf();
	cdbp->magic = CONSOLE_DEBUG_MAGIC;

	DPRINT1( CONSOLE_DEBUG_LEVEL, "Time stamp: %d\n", cdbp->timeStamp );
	DPRINT1( CONSOLE_DEBUG_LEVEL, "Acquisition state: %d\n", cdbp->Acqstate );
	DPRINT1( CONSOLE_DEBUG_LEVEL, "Revision number: %d\n", cdbp->revNum );
	DPRINT1( CONSOLE_DEBUG_LEVEL, "startup system configuration: 0x%08lx\n",
		 cdbp->startupSysConf );

	fifoOk = getFifoDebug( cdbp );
	adcOk = getAdcDebug( cdbp );
	automationOk = getAutomationDebug( cdbp );

	currentSysConf = 0;
	if (adcOk)
	  currentSysConf |= ADC_PRESENT(0);
	if (automationOk)
	  currentSysConf |= MSR_PRESENT;
	if (fifoOk)
	  currentSysConf |= FIFO_PRESENT;

	for (iter = 0; iter < MAX_STM_OBJECTS; iter++) {
		if (getStmDebug( &cdbp->stmHwRegs[ iter ], iter ))
        	  currentSysConf |= STM_PRESENT( iter );
	}
	cdbp->currentSysConf = currentSysConf;
	DPRINT1( CONSOLE_DEBUG_LEVEL, "current system configuration: 0x%08lx\n",
		 cdbp->currentSysConf );

	return( 0 );
}

storeConsoleDebug( int index )
{
	int	 cdbSize, iter, numberLongWordsCdb, nvRamOffset, nvRamResult;
	long	*cdbaddr;

	if (index < 0 || index > MAX_CONSOLE_DEBUG_INDEX)
	  return( -1 );

/*  Only compute the number of long words in a console debug block
    if we are working with the 2nd (or later) copy in NVRAM.		*/

	numberLongWordsCdb = ((sizeof( cdb_msg.cdb ) + sizeof( long ) - 1) / sizeof( long ));
	if (index > 0)
	  cdbSize = numberLongWordsCdb * sizeof( long );
	else
	  cdbSize = 0;

	getConsoleDebugFromHardware( &cdb_msg.cdb );

	nvRamOffset = 0x800 + (index * cdbSize);
	DPRINT2( CONSOLE_DEBUG_LEVEL,
		"store console debug: index: %d, NVRAM offset: %d\n", index, nvRamOffset );
	cdbaddr = (long *) &cdb_msg.cdb;

	for (iter = 0; iter < numberLongWordsCdb; iter++) {
		nvRamResult = sysNvRamSet( cdbaddr, sizeof( long ), nvRamOffset );
		if (nvRamResult != OK) {
			errLogSysRet( LOGIT,
		   "failed to write console debug status to NVRAM at offset %d\n", nvRamOffset
			);
			return( -1 );
		}
		cdbaddr++;
		nvRamOffset += sizeof( long );
	}

	return( 0 );
}

static int
readConsoleDebugFromNVRAM( CONSOLE_DEBUG *cdbp, int index )
{
	int	cdbSize, iter, numberLongWordsCdb, nvRamOffset, nvRamResult;
	long	xferPoint[ 2 ], *cdbaddr;

	if (index < 0 || index > MAX_CONSOLE_DEBUG_INDEX)
	  return( -1 );

/*  Only compute the number of long words in a console debug block
    if we are working with the 2nd (or later) copy in NVRAM.		*/

	numberLongWordsCdb = ((sizeof( cdb_msg.cdb ) + sizeof( long ) - 1) / sizeof( long ));
	if (index > 0)
	  cdbSize = numberLongWordsCdb * sizeof( long );
	else
	  cdbSize = 0;

	nvRamOffset = 0x800 + (index * cdbSize);
	DPRINT2( CONSOLE_DEBUG_LEVEL,
		"read console debug: index: %d, NVRAM offset: %d\n", index, nvRamOffset );
	cdbaddr = (long *) cdbp;

	for (iter = 0; iter < numberLongWordsCdb; iter++) {
		nvRamResult = sysNvRamGet( &xferPoint[ 0 ], sizeof( long ), nvRamOffset );
		if (nvRamResult != OK) {
			errLogSysRet( LOGIT,
		   "failed to read console debug status from NVRAM at offset %d\n", nvRamOffset
			);
			return( -1 );
		}
		*cdbaddr++ = xferPoint[ 0 ];
		nvRamOffset += sizeof( long );
	}

	return( 0 );
}

static void
showStmDebug( STM_DEBUG *stmdbp, int index )
{
	STMOBJ_ID	 pStmId;

	pStmId = stmGetStmObjByIndex( index );
	if (pStmId == NULL || pStmId->stmBaseAddr == 0xFFFFFFFF) {
		DPRINT1( -1, "STM %d not present\n", index );
		return;
	}

	DPRINT2( -1, "STM %d Status register: 0x%04x\n", index, stmdbp->stmStatus );
	DPRINT2( -1, "STM %d Tag register: %d\n", index, stmdbp->stmTag );
	DPRINT2( -1, "STM %d NP counter: %d\n", index, stmdbp->stmNpReg );
	DPRINT2( -1, "STM %d NT counter: %d\n", index, stmdbp->stmNtReg );
	DPRINT2( -1, "STM %d Source address: 0x%08lx\n", index, stmdbp->stmSrcAddr );
	DPRINT2( -1, "STM %d Destination address: 0x%08lx\n", index, stmdbp->stmDstAddr );
}

static void
showConsoleDebugStruct( CONSOLE_DEBUG *cdbp )
{
	int	iter;

	DPRINT1( -1, "Time stamp: %d\n", cdbp->timeStamp );
	DPRINT1( -1, "Acquisition state: %d\n", cdbp->Acqstate );
	DPRINT1( -1, "Revision number: %d\n", cdbp->revNum );
	DPRINT1( -1, "startup system configuration: 0x%08lx\n", cdbp->startupSysConf );

	DPRINT1( -1, "FIFO status: 0x%08lx\n", cdbp->fifoStatus );
	DPRINT1( -1, "FIFO Interrupt Mask: 0x%04x\n", cdbp->fifoIntrpMask );
	if (pTheFifoObject->optionsPresent & HSLINEMEZZ)
	  DPRINT3( -1, "FIFO Last Word: 0x%08lx 0x%08lx 0x%08lx",
		 cdbp->lastFIFOword[ 0 ], cdbp->lastFIFOword[ 1 ], cdbp->lastFIFOword[ 2 ] );
	else
	  DPRINT2( -1, "FIFO Last Word: 0x%08lx 0x%08lx",
		 cdbp->lastFIFOword[ 0 ], cdbp->lastFIFOword[ 1 ] );

	DPRINT1( -1, "ADC Status: 0x%x\n", cdbp->adcStatus );
	DPRINT1( -1, "ADC Interrupt Mask: 0x%x\n", cdbp->adcIntrpMask );

	DPRINT1( -1, "Automation Status: 0x%x\n", cdbp->autoStatus );
	DPRINT1( -1, "Automation H S & R Status: 0x%x\n", cdbp->autoHSRstatus );

	for (iter = 0; iter < MAX_STM_OBJECTS; iter++) {
		showStmDebug( &cdbp->stmHwRegs[ iter ], iter );
	}

	DPRINT1( -1, "current system configuration: 0x%08lx\n", cdbp->currentSysConf );
}

showConsoleDebugNVRAM( int index )
{
	if (index < 0 || index > MAX_CONSOLE_DEBUG_INDEX)
	  return( -1 );

	readConsoleDebugFromNVRAM( &cdb_msg.cdb, index );
	showConsoleDebugStruct( &cdb_msg.cdb );

	taskDelay( 300 );

	return( 0 );
}

testConsoleDebug()
{
	DPRINT1( CONSOLE_DEBUG_LEVEL,
		"size of the console debug structure: %d\n", sizeof( cdb_msg.cdb ) );
	getConsoleDebugFromHardware( &cdb_msg.cdb );
	showConsoleDebugStruct( &cdb_msg.cdb );
	taskDelay( 300 );
}

sendConsoleDebug( int index )
{
	if (index < 0 || index > MAX_CONSOLE_DEBUG_INDEX)
	  return( -1 );

	readConsoleDebugFromNVRAM( &cdb_msg.cdb, index );
	cdb_msg.msg_type = CDB_INFO;
	DPRINT2( -1, "send console debug is sending block %d (%d chars) to the host\n", index, sizeof( cdb_msg ) );
	msgQSend(pMsgesToHost, (char *) &cdb_msg, sizeof( cdb_msg ),
			 NO_WAIT, MSG_PRI_NORMAL);
}
