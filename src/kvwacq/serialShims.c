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
#include <stdio.h>
#include <string.h>
#include <vxWorks.h>
#include <semLib.h> 
#include <ioLib.h>

#include "logMsgLib.h"
#include "hardware.h"
#include "fifoObj.h"
#include "AParser.h"
#include "acqcmds.h"

#if ( CPU!=CPU32 )
#include "autoObj.h"
extern AUTO_ID pTheAutoObject;
#endif

extern FIFO_ID  pTheFifoObject;  /* FIFO Object */
extern ACODE_ID pTheAcodeObject;	/* ACODE object */

#define CMD 0
#define TIMEOUT 98
#define ECHO 1


static int	shimPort = -1;
static SEM_ID	pShimMutex = NULL;

#define  NO_SHIMS	0
#define  QSPI_SHIMS	1
#define  SERIAL_SHIMS	2
#define  RRI_SHIMS	3
#define  MSR_SERIAL_SHIMS 4
#define  MSR_RRI_SHIMS	5
#define  OMT_SHIMS	6
#define  MSR_OMT_SHIMS	7
#define  APBUS_SHIMS    8

#define  SHIMS_ON_MSR   1
#define  SHIMS_ON_162   2

/* OMT Serial Shim Status Bit Definitions */
#define OMTSTS_RUN (1<<0)
#define OMTSTS_COIL (1<<1)
#define OMTSTS_PHASE (1<<2)
#define OMTSTS_CABINET_FAULT (1<<3)
#define OMTSTS_CHAN_OVERPOWER (1<<4)
#define OMTSTS_INPUT_OVERPOWER (1<<5)
#define OMTSTS_OUTPUT_OVERPOWER (1<<6)
#define OMTSTS_ADC_FAULT (1<<7)
#define OMTSTS_MODULE_FAULT (1<<8)
#define OMTSTS_REMOTE (1<<9)

/* OMT Serial Shim Message Types */
#define OMT_NO_MSG 0
#define OMT_VERSION_MSG 1
#define OMT_ERROR_MSG 2
#define OMT_STATUS_MSG 3
#define OMT_ACTUAL_MSG 4
#define OMT_NULL_MSG 5
#define OMT_UNKNOWN_MSG 6

#define OMT_TIMEOUT 100		/* 1 sec timeout */

extern char *getstring(int, char, int);

int	shimType = NO_SHIMS;
int	shimSet = -1;
int	shimLoc = -1;

/*  The index matches the index in Dan's table on the host
    The value represents what GlenD's drivers will expect.	*/

#ifndef MAXSHIMS
#define  MAXSHIMS	48
#endif


static const int
qspi_dac_table[ MAXSHIMS ] = {
	-1,
	4,
	0,
	1,
	2,
	3,			/* 5 */
	5,
	6,
	7,
	-1,
	-1,			/* 10 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 15 */
	8,
	9,
	10,
	13,
	12,			/* 20 */
	11,
	14,
	15,
	-1,
	-1,			/* 25 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 30 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 35 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 40 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 45 */
	-1,
	-1,
};

static const int
imgqsp_dac_table[ MAXSHIMS ] = {
	-1,
	4,
	0,
	-1,
	2,
	-1,			/* 5 */
	5,
	6,
	-1,
	-1,
	-1,			/* 10 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 15 */
	8,
	9,
	10,
	12,
	13,			/* 20 */
	11,
	-1,
	-1,
	15,
	1,			/* 25 */
	14,
	3,
	-1,
	-1,
	-1,			/* 30 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 35 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 40 */
	-1,
	-1,
	-1,
	-1,
	-1,			/* 45 */
	-1,
	-1,
};

const int *qspi_dac_addr = 0L;

setApbusShims2( int dac, int value, char *msgptr)
{

int     dacspot,tmp,tmp2;
   dacspot = qspi_dac_table[ dac ];
/* errLogRet(LOGIT,debugInfo,"spot=%d value=%d\n",dacspot,value); */
   if (dacspot == -1) return;   /* non-existing DAC */

   tmp  = 0xA00 | ((dacspot&0x3)<<6) | ((value>>8)&0xF);
   tmp2 = 0xA00 | (value & 0xFF);
   sprintf(msgptr,"3;%d,%d,1,%d;%d,%d,1,%d;%d,%d,1,%d;",
	FIX_APREG, 0x0A40, 0xA00 | (dacspot/4),
	FIX_APREG, 0x0A41, tmp,
	FIX_APREG, 0x0A41, tmp2);
}

int
setApbusShims( int dac, int value)
{
int     dacspot,tmp,tmp2;
   dacspot = qspi_dac_table[ dac ];
/* errLogRet(LOGIT,debugInfo,"spot=%d value=%d\n",dacspot,value); */
   if (dacspot == -1) return;   /* non-existing DAC */

   tmp  = 0xA00 | ((dacspot&0x3)<<6) | ((value>>8)&0xF);
   tmp2 = 0xA00 | (value & 0xFF);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_SLCT, 0xA40);
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_WRT,  0xA00 | (dacspot/4) );
   fifoStuffCmd(pTheFifoObject, 0,              0x00A);  /* 1 usec delay */
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_INCWR,tmp);
   fifoStuffCmd(pTheFifoObject, 0,              0x00A);  /* 1 usec delay */
   fifoStuffCmd(pTheFifoObject, CL_AP_BUS_WRT,  tmp2);
/*   if (pTheAcodeObject == NULL) */
   {  fifoStuffCmd(pTheFifoObject,HALTOP,0);      /* xhaltop(0); */
      fifoStart(pTheFifoObject);                  /* ff_start(); */
      fifoWait4StopItrp(pTheFifoObject);
   }
}

int
areSerialShimsSetup()
{
	return( shimPort >= 0);
}

getnGiveShimMutex()
{
   if (pShimMutex == NULL)
     return;
   semTake(pShimMutex,WAIT_FOREVER);
   semGive(pShimMutex);
}

/* For now Just for MSR board */
#if ( CPU==CPU32 )
int setQspiShim(short dacnum,short SHIMvalue)
{
    short qspi_dac;
    /* Standard Qspi or Imaging QSpi ShimSet */
    /* qspi_dac_addr set in determineShimType() */
	
     /* DPRINT3(-1,"setQspiShim: DAC[%d] = %d, qspi_dac_addr: 0x%lx\n",
		dacnum,SHIMvalue,qspi_dac_addr); */
     /* make sure address is valid before proceeding */
     if ((qspi_dac_addr == qspi_dac_table) || (qspi_dac_addr == imgqsp_dac_table))
     {
         /* Decode and set proper dac */
         qspi_dac = (short) qspi_dac_addr[ dacnum ];
         /* DPRINT2(-1,"setQspiShim: shimDacSet(%d,%d)\n",qspi_dac,SHIMvalue); */
         shimDacSet(qspi_dac,SHIMvalue);
     }
}
#endif

setTimShim( int port, int board, int dac, int value, int prtmod )
{
	int localValue, stat;

	if (port < 0)
	  return( -1 );

    if (pShimMutex != NULL)
    {
        semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"setTimShim: Mutex Pointer NULL\n");
      return(-1);
    }

/*  For some reason on the U+ system a positive shim parameter value
    resulted in a negative voltage in the shim coil.  On the inova
    system a positive value sent to the shim computer causes a positive
    voltage in the shim coil.  This change was ajudged to be not
    acceptable so we change the sign of the parameter here.		*/

	localValue = -value;

	clearinput(port);
	pputchr(port, 'S');
	if (cmdecho(port, CMD)) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed on the S command\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}

	pputchr(port, 'D');
        if (cmdecho(port,ECHO)) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed on the D command\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}

	pputchr(port, ' ');
        if (cmdecho(port,ECHO)) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed on the ' ' command\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}
	if (echoval(port, board )) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed selecting the board\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}

	pputchr(port, ' ');
        if (cmdecho(port,ECHO)) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed on the 2nd ' '\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}
	if (echoval(port, dac )) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed selecting the DAC\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}

	pputchr(port, ' ');
        if (cmdecho(port,ECHO)) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed on the 3rd ' '\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}
	if (echoval(port, localValue )) {
		errLogRet( LOGIT, debugInfo, "set tim shim failed setting the DAC value\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}
	pputchr(port, '\n');

	stat = cmddone(port, 500);
	semGive(pShimMutex);
	return( 0 );
}

int
serialShimsShow()
{
   printf("\n\n---------------------------------------------------------\n\n");
   printf("serial port is %d, at memory location 0x%x\n", shimPort, &shimPort );
   printf( "serial port Mutex: 0x%lx\n", pShimMutex);
   if (pShimMutex != NULL)
      semShow(pShimMutex,1);
   return( 0 );
}

shimPrintSet()
{
   switch (shimSet)
   { case  1:
	printf("(0x%05x): is Varian 13 shims\n",shimSet);
	break;
     case  2:
	printf("(0x%05x): is Oxford 18 shims\n",shimSet);
	break;
     case  3:
	printf("(0x%05x): is Varian 23 shims\n",shimSet);
	break;
     case  4:
	printf("(0x%05x): is Varian 28 shims\n",shimSet);
	break;
     case  5:
	printf("(0x%05x): is Ultra Shims\n",shimSet);
	break;
     case  6:
	printf("(0x%05x): is Varian 18 shims\n",shimSet);
	break;
     case  7:
	printf("(0x%05x): is Varian 20 shims\n",shimSet);
	break;
     case  8:
	printf("(0x%05x): is Oxford 15 shims\n",shimSet);
	break;
     case  9:
	printf("(0x%05x): is Varian 40 shims\n",shimSet);
	break;
     case 10:
	printf("(0x%05x): is Varian 14 shims\n",shimSet);
	break;
     case 12:
	printf("(0x%05x): is Varian 26 shims\n",shimSet);
	break;
     case 13:
	printf("(0x%05x): is Varian 29 shims\n",shimSet);
	break;
     case 14:
	printf("(0x%05x): is Varian 35 shims\n",shimSet);
	break;
     default:
	printf("(0x%05x): is UNKNOWN Shim Set <<<<<<<<<<\n",shimSet);
	break;
   }
}


static
testSerialShims( int port )
{
    	char *omtMsg = 0;
	int rtn = 0;
	int	ival;

    if (pShimMutex != NULL)
    {
        semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"testSerialShims: Mutex Pointer NULL\n");
      return(-1);
    }


	clearinput( port );
	/* printf("testSerialShims: put B, wait for responce\n"); */
	 pputchr(port, 'B');
	if (cmdecho(port, CMD)) 
        {   /* if command times out, try again */
	    pputchr( port, 'B');    /* RRI peripheral responds on the 2nd try */
	    /* printf("testSerialShims: again put B, wait for responce\n"); */
	    if (cmdecho(port, CMD))    /* even though RRI calls it an error. */
	    {
		/* No response yet */
		pputchr(port, '\r'); /* Make OMT serial shims respond */
		omtMsg = getstring(port, '\r', OMT_TIMEOUT);
		if (!omtMsg){
		    semGive(pShimMutex);
		    return( TIMEOUT );
		}
	    }
	}

	if (!omtMsg) ival = cmddone(port, 300 );

	semGive(pShimMutex);
	return( 0 );
}

#ifndef SERIAL_PORT_NUMBER
#if ( CPU==CPU32 )
#define SERIAL_PORT_NUMBER	0
#else
#define SERIAL_PORT_NUMBER	3
#endif
#endif

initSerialShims()
{
	int	ival;

	if (shimPort >= 0) {
		close( shimPort );
	}

	shimPort = initSerialPort( SERIAL_PORT_NUMBER );
        setSerialTimeout(shimPort,25);	/* set timeout to 1/4 sec */

	if (shimPort < 0)
	  return( -1 );

	if (pShimMutex == NULL)  
        {
           pShimMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);
        }

	ival = testSerialShims( shimPort );
	if ((ival == -1) || (ival == TIMEOUT))
        {
		close( shimPort );
		shimPort = -1;
		return( -1 );
	}

	return( 0 );
}

static int
readSerialShimType()
{
	char	typeSerialShim[ 12 ];
	int	ival, shimset;

	memset( &typeSerialShim[ 0 ], 0, sizeof( typeSerialShim ) );

	if (shimPort < 0) {
		ival = initSerialShims();
		if (ival < 0 || ival == TIMEOUT)
		  return( -1 );
	}

    if (pShimMutex != NULL)
    {
        semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"readSerialShimType: Mutex Pointer NULL\n");
      return(-1);
    }
	shimset = 0;
	clearinput( shimPort );
	pputchr( shimPort, 'I');
	if (cmdecho( shimPort, CMD))
	{
	    /* No resposnse--must be different type serial shim */
	    shimset = readOmtSerialShimType(); /* Try OMT shims */
	}else{
	    /* Responded to "I" cmd */
	    ival = readreply( shimPort, 300,
			     &typeSerialShim[ 0 ], sizeof( typeSerialShim ) );
	    if (ival == 0)
	       shimset = atoi( &typeSerialShim[ 0 ] );
	    else
	       shimset = -1;
	}

	semGive(pShimMutex);
	return( shimset );
}

/*  This program attempts to reset the RRI or other serial shims when the
    shims are not responding becasue the Shim CPU and the console/MSR CPU
    are out of phase in the command sequence.	*/

resetSerialShims()
{
	int	stat;

	pputchr(shimPort, '\n');
	stat = cmddone(shimPort, 500);
	DPRINT1( -1, "cmddone returned %d in reset serial shims\n", stat );

	pputchr(shimPort, '\r');
	stat = cmddone(shimPort, 500);
	DPRINT1( -1, "cmddone returned %d in reset serial shims\n", stat );
}

#if ( CPU==CPU32 )

determineShimType()
{
   int shimrri;

   shimSet = shimEEpromPresent();
   if ( (shimSet < 1) )
   {
      if( tyQCoPresent() ) /* is local QUART active ? */
      {  
        DPRINT(0,"GET_SHIMS_PRESENT: Local QUART active.\n");
        /* check for someone on the serial port */
        /* get ID from the someone on the serial port */
        shimSet = readSerialShimType();
        if (shimSet == TIMEOUT || shimSet < 0)
           shimSet = -1;
        else
        {
	   if (isOmtShimset(shimSet))
	       shimType = OMT_SHIMS;
	   else
               shimType = SERIAL_SHIMS;
           shimLoc = SHIMS_ON_MSR ;
        }
      }
      else
        shimSet = -1;
   }
   else if (shimSet == 5)
   {
      shimType = RRI_SHIMS ;
      shimrri = initSerialShims();		/* check if its here, -1 == no */
      if (shimrri != -1)
         shimLoc = SHIMS_ON_MSR ;
      else 
         shimLoc = SHIMS_ON_162 ;
   }
   else if ((shimSet == 1) || (shimSet == 10))   /* qspi shims on MSR */
   {
       shimType = QSPI_SHIMS;
       shimLoc = SHIMS_ON_MSR;
       qspi_dac_addr = &qspi_dac_table[ 0 ];
   }
   else if (shimSet == 8)	/* Oxford shims on MSR interfaced through the QSPI */
   {
       shimType = QSPI_SHIMS;
       shimLoc = SHIMS_ON_MSR;
       qspi_dac_addr = &imgqsp_dac_table[ 0 ];
   }
   DPRINT3(1,"determineShimType: shimSet: %d, shimType: %d, shimLoc: %d\n",shimSet,shimType,shimLoc);
   return(shimSet);
}

#else	/* CPU 68040 */

determineShimType()
{
   int shimrri;

   DPRINT(0,"determineShimType: check MSR()\n");
   shimSet = autoShimsPresentGet( pTheAutoObject );
   DPRINT1(0,"determineShimType: shimSet: %d\n",shimSet);
   if (shimSet > 1000)
      shimLoc = SHIMS_ON_MSR;
   else
      shimLoc = SHIMS_ON_162;
     
   DPRINT2(0,"determineShimType: shimSet: %d, shimLoc: %d\n", shimSet,shimLoc);

   shimSet = shimSet % 1000;
   DPRINT1(0,"determineShimType: shimSet: %d\n", shimSet);

   if (shimSet == -1) /* must be serial shims on 162 side */
   {
        DPRINT(1,"determineShimType: readSerialShimType\n");
	shimSet = readSerialShimType();  /* get shimset from serial shims, -1 error */
	if (isOmtShimset(shimSet))
	  shimType = OMT_SHIMS;
	else if (shimSet > 0)
	  shimType = SERIAL_SHIMS;
        else
	{  int tmp_apbyte;
           tmp_apbyte=readApbusRegister(0xA43);
           if (tmp_apbyte != -1)
           {  shimSet = 10;
              shimType = APBUS_SHIMS;
           }
           else
              shimType = NO_SHIMS;
        }
   }
   else if ((shimSet == 1) || (shimSet == 10))   /* qspi shims on MSR */
   {
       shimType = QSPI_SHIMS;
       shimLoc = SHIMS_ON_MSR;
       qspi_dac_addr = &qspi_dac_table[ 0 ];
   }
   else if (shimSet == 5)  /* RRI on MSR */
   {
       if (shimLoc == SHIMS_ON_MSR)
       {
          shimType = MSR_RRI_SHIMS;
       }
       else
       {
          DPRINT(1,"determineShimType: initSerialShims\n");
          shimrri = initSerialShims();		/* check if its there, -1 == no */
          if (shimrri > -1)
	     shimType = RRI_SHIMS;
          else
	     shimType = NO_SHIMS;
       }
       qspi_dac_addr = &qspi_dac_table[ 0 ];
   }
   else if (shimSet == 11)	/* OMT shims on MSR */
   {
       shimType = MSR_OMT_SHIMS;
       shimLoc = SHIMS_ON_MSR;
   }
   else if (shimSet == 8)	/* Oxford shims on MSR interfaced through the QSPI */
   {
       shimType = QSPI_SHIMS;
       shimLoc = SHIMS_ON_MSR;
       qspi_dac_addr = &imgqsp_dac_table[ 0 ];
   }
   else
   {
       shimType = MSR_SERIAL_SHIMS;
   }
   return(shimSet);
}
#endif

const int *shimGetQspiTab()
{
   return(&qspi_dac_table[0]);
}
const int *shimGetImgQspiTab()
{
   return(&imgqsp_dac_table[0]);
}

isOmtShimset(int shimno)
{
    return shimno == 11;
}

static int
getOmtSerialShimMsgType(char *msg)
{
    int type;
    int len;

    if (!msg){
	type = OMT_NO_MSG;
    }else if ((len=strlen(msg)) == 0){
	type = OMT_NULL_MSG;
    }else if (strcmp(msg, "!") == 0){
	type = OMT_ERROR_MSG;
    }else if (strstr(msg, "2358 Shim")){
	type = OMT_VERSION_MSG;
    }else if (strspn(msg, ".+") == len && len >= 10){
	type = OMT_STATUS_MSG;
    }else if (strspn(msg, "+-0123456789.;") == len && len >= 10){
	type = OMT_ACTUAL_MSG;
    }else{
	type = OMT_UNKNOWN_MSG;
    }
    return type;
}

static char *
getOmtSerialShimMsg(int msg_type)
{
    int type;
    char *msg;

    for (type=-1; type != msg_type && type != OMT_NO_MSG; ){
	msg = getstring(shimPort, '\r', OMT_TIMEOUT);
	type = getOmtSerialShimMsgType(msg);
    }
    return msg;
}

static void
initOmtSerialShims()
{
    int i;
    int status;
    char *msg;

    if ((status=getOmtShimStatus()) == -1){
	return;
    }

    /* Make sure shims are in REMOTE mode */
    if (!(status & OMTSTS_REMOTE)){
	putstring(shimPort, "REM\r");
	msg = getOmtSerialShimMsg(OMT_STATUS_MSG); /* Flush status msg */
    }

    /* Make sure shims are in RUN mode */
    if (!(status & OMTSTS_RUN)){
	putstring(shimPort, "RUN\r");
	msg = getOmtSerialShimMsg(OMT_STATUS_MSG); /* Flush status msg */
    }
}

readOmtSerialShimType()
{
    int type = -1;
    char *msg;

    putstring(shimPort, "\rVER\r"); /* Ask for version string*/
    msg = getOmtSerialShimMsg(OMT_VERSION_MSG);
    if (msg && strstr(msg, "2358")){
	if (strstr(msg, "v2.")){
	    type = 11;
	}
	initOmtSerialShims();
    }
    return type;
}

static short omtShimTable[] = {
    -1,				/*  0 = $ 0 = z0f (not present) */
    -1,				/*  1 = $ 1 = z0c (not present) */
    0,				/*  2 = $ 2 = z1f */
    -1,				/*  3 = $ 3 = z1c (not present) */
    3,				/*  4 = $ 4 = z2f */
    -1,				/*  5 = $ 5 = z2c (not present) */
    -1,				/*  6 = $ 6 = z3 (not present) */
    -1,				/*  7 = $ 7 = z4 (not present) */
    -1,				/*  8 = $ 8 = z5 (not present) */
    -1,				/*  9 = $ 9 = z6 (not present) */
    -1,				/* 10 = $ a = z7 (not present) */
    -1,				/* 11 = $ b =  */
    -1,				/* 12 = $ c = zx3 (not present) */
    -1,				/* 13 = $ d = zy3 (not present) */
    -1,				/* 14 = $ e = z4x (not present) */
    -1,				/* 15 = $ f = z4y (not present) */
    2,				/* 16 = $10 = x */
    1,				/* 17 = $11 = y */
    4,				/* 18 = $12 = xz */
    7,				/* 19 = $13 = xy */
    6,				/* 20 = $14 = x2-y2 */
    5,				/* 21 = $15 = yz */
};

setSerialOmtShim( int dac, int value )
{
	int i;
	char *msg;
	int smvalue;		/* Value in sign/magnitude format */
	char buf[32];

	if (dac < 0)
	  return( -1 );
	if (dac >= sizeof( omtShimTable ) / sizeof( omtShimTable[ 0 ] ))
	  return( -1 );
	if (shimPort < 0)
	  return( -1 );
	if (omtShimTable[ dac ] < 0 )
	  return( -1 );

	if (pShimMutex != NULL)
	{
	    semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
	}
	else
	{
	    errLogRet(LOGIT,debugInfo,
		      "setSerialOmtShim: Mutex Pointer NULL\n");
	    return(-1);
	}

	smvalue = value < 0 ? 0x8000 - value : value;
	sprintf(buf,"DEM;%02d;%d\r", omtShimTable[dac], smvalue);
	putstring(shimPort, buf);
	msg = getOmtSerialShimMsg(OMT_NULL_MSG); /* Wait for confirmation */
	if (!msg){
	    /* No confirmation */
	    initOmtSerialShims(); /* Reinitialize shims ... */
	    putstring(shimPort, buf); /* and try again */
	    getOmtSerialShimMsg(OMT_NULL_MSG); /* Wait for confirmation */
	    /* Silently give up on failure */
	}

	semGive(pShimMutex);

	return( 0 );
}

int getOmtShimStatus()
{
    int rtn = 0;
    char *buf;
    int i;

/*    taskDelay(sysClkRateGet() / 10); /* Wait 100ms for any msgs to finish */

    clearinput(shimPort);	/* Empty the serial FIFO */
    putstring(shimPort,"\rSTS\r"); /* Ask for status */
    buf = getOmtSerialShimMsg(OMT_STATUS_MSG); /* Read status */
    if (!buf){
	errLogRet(LOGIT,debugInfo,"getOmtShimStatus: No status returned\n");
	rtn = -1;
    }else{
	for (rtn=i=0; i<(8*sizeof(int)); i++, buf++){
	    if (*buf == '+'){
		rtn |= 1 << i;
	    }else if (*buf == '\0'){
		break;
	    }else if (*buf != '.'){
		errLogRet(LOGIT,debugInfo,
			  "getOmtShimStatus: Non-status message received\n");
		break;
	    }
	}
    }
    return rtn;
}


/*  Adopted from "stable" in apio.c
 *  This table conforms to the memo from Tim L. dated August 21	*
 *  It uses 7,7 as a non existing DAC				*
 */

struct timShimRef {
	unsigned char	board;
	unsigned char	dac;
} timShimTable[] = {
	{ 7,	7 },		/* 0 = $ 0 = z0f */
	{ 0,	0 },		/* 1 = $ 1 = z0c */
	{ 1,	0 },		/* 2 = $ 2 = z1f   z1f and z1c are the same */
	{ 7,	7 },		/* 3 = $ 3 = z1c */
	{ 2,	0 },		/* 4 = $ 4 = z2f   z2f and z2c are the same */
	{ 7,	7 },		/*  5 = $ 5 = z2c */
	{ 2,	1 },		/*  6 = $ 6 = z3 */
	{ 2,	2 },		/*  7 = $ 7 = z4 */
	{ 2,	3 },		/*  8 = $ 8 = z5 */
	{ 2,	4 },		/*  9 = $ 9 = z6 */
	{ 2,	5 },		/* 10 = $ a = z7 */
	{ 7,	7 },		/* 11 = $ b =  */
	{ 5,	0 },		/* 12 = $ c = zx3 */
	{ 5,	1 },		/* 13 = $ d = zy3 */
	{ 5,	2 },		/* 14 = $ e = z4x, for 26 chan this is x4 */
	{ 5,	3 },		/* 15 = $ f = z4y, for 26 chan this is y4 */
	{ 3,	0 },		/* 16 = $10 = x */
	{ 3,	1 },		/* 17 = $11 = y */
	{ 3,	2 },		/* 18 = $12 = xz */
	{ 3,	5 },		/* 19 = $13 = xy */
	{ 3,	4 },		/* 20 = $14 = x2-y2 */
	{ 3,	3 },		/* 21 = $15 = yz */
	{ 4,	2 },		/* 22 = $16 = x3 */
	{ 4,	3 },		/* 23 = $17 = y3 */
	{ 3,	7 },		/* 24 = $18 = yz2 */
	{ 4,	0 },		/* 25 = $19 = zx2-y2 */
	{ 3,	6 },		/* 26 = $1a = xz2 */
	{ 4,	1 },		/* 27 = $1b = zxy */
	{ 4,	4 },		/* 28 = $1c = z3x */
	{ 4,	5 },		/* 29 = $1d = z3y */
	{ 4,	6 },		/* 30 = $1e = z2(x2-y2) */
	{ 4,	7 },		/* 31 = $1f = z2xy */
	{ 5,	4 },		/* 32 = $20 = z3x2y2 */
	{ 5,	5 },		/* 33 = $21 = z3xy */
	{ 6,	0 },		/* 34 = $22 = z2x3 */
	{ 6,	1 },		/* 35 = $23 = z2y3 */
	{ 6,	4 },		/* 36 = $24 = z3x3 */
	{ 6,	5 },		/* 37 = $25 = z3y3 */
	{ 5,	6 },		/* 38 = $26 = z5x */
	{ 5,	7 },		/* 39 = $27 = z5y */
	{ 6,	2 },		/* 40 = $28 = z4x2y2 */
	{ 6,	3 },		/* 41 = $29 = z4xy */
	{ 7,	7 },		/* 42 = $2a =  */
	{ 7,	7 },		/* 43 = $2b =  */
	{ 6,	6 },		/* 44 = $2c = x4 */
	{ 6,	7 },		/* 45 = $2d = y4 */
	{ 7,	7 },		/* 46 = $2a =  */
	{ 7,	7 },		/* 47 = $2a =  */
};


setSerialShim( int dac, int value )
{
	int	retval;

	if (dac < 0)
	  return( -1 );
	if (dac >= sizeof( timShimTable ) / sizeof( timShimTable[ 0 ] ))
	  return( -1 );
	if (shimPort < 0)
	  return( -1 );

	if (timShimTable[ dac ].board == 7 && timShimTable[ dac ].dac == 7)
	  return( -1 );

	retval = setTimShim(
		shimPort,
	  (int) timShimTable[ dac ].board,
	  (int) timShimTable[ dac ].dac,
		value,
		0 );

	return( retval );
}

/* adopted from rrifuncs.c */
/* WARNING do not changed order, order is the dacnum index */
/* E.G. rriShimTable[1] = z0c */
/* 48 shim DACs in all */
/* no DAC in RRI box -> NULL for entry */

char  *rriShimTable[] = { 
	 "Fz0" , 	/* 0 = z0f */
	 "Cz0" , 	/* 1 = z0c */
	 "Fz" ,  	/* 2 = z1f */
	 "Cz" , 	/* 3 = z1c */
	 "Fz2" ,	/* 4 = z2f */
	 "Cz2" , 	/* 5 = z2c */
	 "Fz3" , 	/* 6 = z3f */
	 "Fz4" , 	/* 7 = z4f */
	 "Cz5" ,	/* 8 = z5 */
	 "Cz6" , 	/* 9 = z6 */
	 "Cz7" , 	/* 10 = z7 */
	 "Cz8" , 	/* 11 = z8 */  /* new DAC definition */
	 "Czc3" , 	/* 12 = zx3 */
	 "Czs3" , 	/* 13 = zy3 */
	 "Cz4x" ,	/* 14 = z4x */
	 "Cz4y" , 	/* 15 = z4y */
	 "Cx" , 	/* 16 = x */
	 "Cy" , 	/* 17 = y */
	 "Czx" ,	/* 18 = xz */
	 "Cs2" , 	/* 19 = xy */
	 "Cc2" , 	/* 20 = x2-y2 */
	 "Czy" , 	/* 21 = yz */
	 "Cc3" , 	/* 22 = x3 */
	 "Cs3" , 	/* 23 = y3 */
	 "Cz2y" , 	/* 24 = yz2 */
	 "Czc2" , 	/* 25 = zx2-y2 */
	 "Cz2x" , 	/* 26 = xz2 */
	 "Czs2" , 	/* 27 = zxy */
	 "Cz3x" , 	/* 28 = z3x */
	 "Cz3y" , 	/* 29 = z3y */
	 "Cz2c2" , 	/* 30 = z2(x2-y2) */
	 "Cz2s2" , 	/* 31 = z2xy */
	 "Cz3c2" , 	/* 32 = z3x2-y2 */  /* all new DAC defs RRI */
	 "Cz3s2" , 	/* 33 = z3xy */
	 "Cz2c3" , 	/* 34 = z2x3 */
	 "Cz2s3" , 	/* 35 = z2y3 */
	 "Cz3c3" , 	/* 36 = z3x3 */
	 "Cz3s3" , 	/* 37 = z3y3 */
	 "Cz5x" , 	/* 38 = z5x */
	 "Cz5y" , 	/* 39 = z5y */
	 "Cz4c2" , 	/* 40 = z4x2-y2 */
	 "Cz4s2" , 	/* 41 = z4xy */
	 "Cz3" , 	/* 42 = z3c */
	 "Cz4" , 	/* 43 = z4c */
	 NULL,
};

setRRIShim( int dac, int value )
{
	int	stat;
	char	*chrptr;

	if (dac < 0)
	  return( -1 );
	if (shimPort < 0)
	  return( -1 );
	if (dac >= sizeof( rriShimTable ) / sizeof( rriShimTable[ 0 ] ))
	  return( -1 );
	chrptr = rriShimTable[dac];
	if (chrptr == NULL)
	  return( -1 );

    if (pShimMutex != NULL)
    {
        semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
    }
    else
    {
      errLogRet(LOGIT,debugInfo,"setRRIShim: Mutex Pointer NULL\n");
      return(-1);
    }
	clearinput(shimPort);
	pputchr(shimPort, 'S');
	if (cmdecho(shimPort, ECHO)) {
		errLogRet( LOGIT, debugInfo, "set RRI shim failed on the S command\n" );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}

/*  First character is C(oarse) or F(ine).  RRI echoes it with an EOM (^C).
    CMD for 2nd argument to cmdecho tells cmdecho to expect EOM.		*/

	pputchr(shimPort, *chrptr);
	if (cmdecho(shimPort, CMD)) {
		errLogRet( LOGIT, debugInfo,
				 "set RRI shim failed on the %c command\n", *chrptr );
	        semGive(pShimMutex);
		return(TIMEOUT);
	}
	chrptr++;

	while (*chrptr != '\0') {
		pputchr(shimPort, *chrptr);
		if (cmdecho(shimPort, ECHO)) {
			errLogRet( LOGIT, debugInfo,
				  "set RRI shim failed on the %c command\n", *chrptr );
	                semGive(pShimMutex);
			return(TIMEOUT);
		}
		chrptr++;
	}

	pputchr(shimPort, ' ');
        if (cmdecho(shimPort,ECHO)) {
		errLogRet( LOGIT, debugInfo, "set RRI shim failed on the ' '\n" );
	  	semGive(pShimMutex);
		return(TIMEOUT);
	}

	if (echoval(shimPort, value )) {
		errLogRet( LOGIT, debugInfo, "set RRI shim failed setting the DAC value\n" );
	  	semGive(pShimMutex);
		return(TIMEOUT);
	}
	pputchr(shimPort, '\r');		/* NOT <L/F> !!! */

	stat = cmddone(shimPort, 500);
	semGive(pShimMutex);
	return( 0 );
}

/* just a debugging aid */
setMSRserial()
{
   shimType = MSR_SERIAL_SHIMS;
}

void
shimSetShow()
{
        printf("\n--------------------------------------------------------------\n");
        shimPrintSet();
        printf("\n");
	switch (shimType) {
	  case NO_SHIMS:
		printf("shim set not established\n" );
		break;

	  case SERIAL_SHIMS:
#if ( CPU==CPU32 )
		printf("serial shims interfaced through the MSR CPU\n" );
#else
		printf("serial shims interfaced through the console CPU\n" );
#endif
		serialShimsShow();
		break;

	  case OMT_SHIMS:
#if ( CPU==CPU32 )
		printf("Whole Body shims interfaced via the MSR CPU\n" );
#else
		printf("Whole Body shims interfaced via the console CPU\n" );
#endif
		serialShimsShow();
		break;

	  case QSPI_SHIMS:
		if (qspi_dac_addr == &qspi_dac_table[ 0 ])
		  printf("qspi shims interfaced through the automation board\n" );
		else if (qspi_dac_addr == &imgqsp_dac_table[ 0 ])
		  printf("imaging shims interfaced through the automation board\n" );
		else
		  printf("unknown shims interfaced through the automation board\n" );
		break;

	  case RRI_SHIMS:
#if ( CPU==CPU32 )
		printf("RRI shims interfaced through the MSR CPU\n" );
#else
		printf("RRI shims interfaced through the console CPU\n" );
#endif
		break;

	  case APBUS_SHIMS:
                printf("Mercury shims interfaced through the AP bus\n");
                break;

	  case MSR_SERIAL_SHIMS:
		printf("serial shims interfaced through the MSR  CPU\n" );
		serialShimsShow();
		break;

	  case MSR_RRI_SHIMS:
		printf("RRI shims interfaced through the MSR CPU\n" );
		break;

	  case MSR_OMT_SHIMS:
		printf("Whole Body shims interfaced through the MSR CPU\n" );
		break;

	  default:
		errLogRet( LOGIT, debugInfo, "unknown shims set %d\n", shimType );
		break;
	}
        shimPrintSet();
}
