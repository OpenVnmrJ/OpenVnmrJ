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

#if ( CPU!=CPU32 )
#include "autoObj.h"
extern AUTO_ID pTheAutoObject;
#endif

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

#define  SHIMS_ON_MSR   1
#define  SHIMS_ON_162   2

/* Two types of RRI shims */
#define ULTRA   5
#define ULTRA18 16

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

extern char *getstring(int, char, int, char*, int);

int	shimType = NO_SHIMS;
int	shimSet = -1;

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
	-1
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
	-1
};

const int *qspi_dac_addr = 0L;

#if ( CPU==CPU32 )
/*  Adopted from "stable" in apio.c
 *  This table conforms to the memo from Tim L. dated August 21	*
 *  It uses 7,7 as a non existing DAC				*
 */

struct timShimRef {
	unsigned char	board;
	unsigned char	dac;
};

struct timShimRef timShim1Table[] = {
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
	{ 7,	7 }		/* 47 = $2a =  */
};

struct timShimRef timShim2Table[] = {
	{ 7,	7 },		/* 0 = $ 0 = z0f */
	{ 0,	0 },		/* 1 = $ 1 = z0c */
	{ 2,	1 },		/* 2 = $ 2 = z1f   z1f and z1c are the same */
	{ 7,	7 },		/* 3 = $ 3 = z1c */
	{ 3,	0 },		/* 4 = $ 4 = z2f   z2f and z2c are the same */
	{ 7,	7 },		/*  5 = $ 5 = z2c */
	{ 3,	1 },		/*  6 = $ 6 = z3 */
	{ 3,	2 },		/*  7 = $ 7 = z4 */
	{ 7,	7 },		/*  8 = $ 8 = z5 */
	{ 7,	7 },		/*  9 = $ 9 = z6 */
	{ 7,	7 },		/* 10 = $ a = z7 */
	{ 7,	7 },		/* 11 = $ b =  */
	{ 7,	7 },		/* 12 = $ c = zx3 */
	{ 7,	7 },		/* 13 = $ d = zy3 */
	{ 7,	7 },		/* 14 = $ e = z4x, for 26 chan this is x4 */
	{ 7,	7 },		/* 15 = $ f = z4y, for 26 chan this is y4 */
	{ 2,	2 },		/* 16 = $10 = x */
	{ 2,	3 },		/* 17 = $11 = y */
	{ 4,	0 },		/* 18 = $12 = xz */
	{ 4,	2 },		/* 19 = $13 = xy */
	{ 4,	3 },		/* 20 = $14 = x2-y2 */
	{ 4,	1 },		/* 21 = $15 = yz */
	{ 7,	7 },		/* 22 = $16 = x3 */
	{ 7,	7 },		/* 23 = $17 = y3 */
	{ 5,	1 },		/* 24 = $18 = yz2 */
	{ 5,	3 },		/* 25 = $19 = zx2-y2 */
	{ 5,	0 },		/* 26 = $1a = xz2 */
	{ 5,	2 },		/* 27 = $1b = zxy */
	{ 7,	7 },		/* 28 = $1c = z3x */
	{ 7,	7 },		/* 29 = $1d = z3y */
	{ 7,	7 },		/* 30 = $1e = z2(x2-y2) */
	{ 7,	7 },		/* 31 = $1f = z2xy */
	{ 7,	7 },		/* 32 = $20 = z3x2y2 */
	{ 7,	7 },		/* 33 = $21 = z3xy */
	{ 7,	7 },		/* 34 = $22 = z2x3 */
	{ 7,	7 },		/* 35 = $23 = z2y3 */
	{ 7,	7 },		/* 36 = $24 = z3x3 */
	{ 7,	7 },		/* 37 = $25 = z3y3 */
	{ 7,	7 },		/* 38 = $26 = z5x */
	{ 7,	7 },		/* 39 = $27 = z5y */
	{ 7,	7 },		/* 40 = $28 = z4x2y2 */
	{ 7,	7 },		/* 41 = $29 = z4xy */
	{ 7,	7 },		/* 42 = $2a =  */
	{ 7,	7 },		/* 43 = $2b =  */
	{ 7,	7 },		/* 44 = $2c = x4 */
	{ 7,	7 },		/* 45 = $2d = y4 */
	{ 7,	7 },		/* 46 = $2a =  */
	{ 7,	7 }		/* 47 = $2a =  */
};

struct timShimRef *timShimTable;

#endif


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

int
qspiToVnmr( short qspinum )
{
	int	iter, translationTableSize;

	if (qspi_dac_addr == &qspi_dac_table[ 0 ]) {
		translationTableSize = sizeof( qspi_dac_table ) / sizeof( qspi_dac_table[ 0 ] );
	}
	else if (qspi_dac_addr == &imgqsp_dac_table[ 0 ]) {
		translationTableSize = sizeof( imgqsp_dac_table ) / sizeof( imgqsp_dac_table[ 0 ] );
	}
	else {
		return( qspinum );
	}

	for (iter = 0; iter < translationTableSize; iter++) {
		if (qspi_dac_addr[ iter ] == qspinum)
		  return( iter );
	}
	return( -1 );
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

	clearport(port);
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

#ifdef DIAG
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
     case 15:
	printf("(0x%05x): is Varian 15 shims\n",shimSet);
	break;
     case ULTRA18:
	printf("(0x%05x): is ULTRA 18 shims\n",shimSet);
	break;
     default:
	printf("(0x%05x): is UNKNOWN Shim Set <<<<<<<<<<\n",shimSet);
	break;
   }
}
#endif


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


	clearport( port );
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
		omtMsg = getstring(port, '\r', OMT_TIMEOUT, NULL, 0);
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
#define SERIAL_PORT_NUMBER	1
#endif
#endif

#if ( CPU==CPU32 )
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
	clearport( shimPort );
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
            {
	       shimset = atoi( &typeSerialShim[ 0 ] );
            }
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
        {
           shimSet = -1;
           if (initSerialShims() != -1)	/* check if its RRI */
           {
              shimType = RRI_SHIMS ;
              shimSet = 5;
           }
        }
        else
        {
	   if (isOmtShimset(shimSet))
	       shimType = OMT_SHIMS;
	   else
               shimType = SERIAL_SHIMS;
        }
      }
      else
      {
        shimSet = -1;
      }
   }
   else if (shimSet == 5)
   {
      shimType = RRI_SHIMS;
      shimrri = initSerialShims();		/* check if its here, -1 == no */
      if (shimrri == -1)
      {
        shimSet = -1;
        shimType = NO_SHIMS;
      }

   }
   else if ((shimSet == 1) || (shimSet == 10))   /* qspi shims on MSR */
   {
       shimType = QSPI_SHIMS;
       qspi_dac_addr = &qspi_dac_table[ 0 ];
   }
   else if (shimSet == 8)	/* Oxford shims on MSR interfaced through the QSPI */
   {
       shimType = QSPI_SHIMS;
       qspi_dac_addr = &imgqsp_dac_table[ 0 ];
   }
   /* test is ULTRA or ULTRA18 RRI shims are present */
   if (shimType ==  RRI_SHIMS)
      shimSet = initGradTest();
   if (shimSet == 15)	/* Oxford shims on MSR interfaced through the QSPI */
      timShimTable = &timShim2Table[0];
   else
      timShimTable = &timShim1Table[0];
   DPRINT2(1,"determineShimType: shimSet: %d, shimType: %d\n",shimSet,shimType);
   return(shimSet);
}

#endif
#if ( CPU != CPU32 )

determineShimType()
{
   int shimrri;

   DPRINT(0,"determineShimType: check MSR() ONLY!\n");
   shimSet = autoShimsPresentGet( pTheAutoObject );
   DPRINT1(0,"determineShimType: shimSet: %d\n",shimSet);
   /* N.B.: (a % b) is poorly defined for a<0 */
   if (shimSet > 0)
       shimSet = shimSet % 1000;
   DPRINT1(0,"determineShimType: shimSet: %d\n", shimSet);

   if (shimSet == -1) /* must not be there */
   {
          shimType = NO_SHIMS;
   }
   else if ((shimSet == 1) || (shimSet == 10))   /* qspi shims on MSR */
   {
       shimType = QSPI_SHIMS;
       qspi_dac_addr = &qspi_dac_table[ 0 ];
   }
   else if ((shimSet == ULTRA) || (shimSet == ULTRA18) ) /* RRI on MSR */
   {
       shimType = MSR_RRI_SHIMS;
       qspi_dac_addr = &qspi_dac_table[ 0 ];
   }
   else if (shimSet == 11)	/* OMT shims on MSR */
   {
       shimType = MSR_OMT_SHIMS;
   }
   else if (shimSet == 8)	/* Oxford shims on MSR interfaced through the QSPI */
   {
       shimType = QSPI_SHIMS;
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
	msg = getstring(shimPort, '\r', OMT_TIMEOUT, NULL, 0);
	type = getOmtSerialShimMsgType(msg);
    }
    return msg;
}

static void
initOmtSerialShims()
{
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
    5				/* 21 = $15 = yz */
};

setSerialOmtShim( int dac, int value )
{
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

    taskDelay(sysClkRateGet() / 10); /* Wait 100ms for any msgs to finish */
    clearport(shimPort);	/* Empty the serial FIFO */
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

#if ( CPU==CPU32 )

extern int thinShimsPresent;

int input_record[MAXSHIMS];

int setThinShim( int dac, int value )
{
   int token1, token2, m1,m2,val1;
   int ttoken1, ttoken2;
   int priorvalue;
   int retval;
   char hstr[24];
   strcpy(hstr,"bogus");
   switch (dac)
   {
         case 16: case 18:   // pair 1 - X/XZ
             m1 = 16;  m2 = 18;
             strcpy(hstr,"x/xz");
         break;
         case 17: case 21:  // pair 2 Y/YZ
             m1 = 17; m2 = 21;
             strcpy(hstr,"y/yz");
         break;
         case 20: case 25:  // pair 3 c2 zc2 
             m1 = 20; m2 = 25;
             strcpy(hstr,"c2/zc2 [x2-y2,zx2-y2]");
         break;
         case 19: case 27:  // pair 4 s2 zs2 
             m1 = 19; m2 = 27;
             strcpy(hstr,"s2/zs2 [xy,zxy]");
         break;
         case 22: case 12:  // pair 5 c3 zc3 
             m1 = 22; m2 = 12;
             strcpy(hstr,"c3/zc3 [x3,zx3]");
         break;
         case 23: case 13:  // pair 6 s3 zs3  
             m1 = 23; m2= 13;
             strcpy(hstr,"s3/zs3 [y3,zy3]");
         break;
         default:
             m1 = 0;
   }
   if (m1 == 0)
   {
	retval = setTimShim(
		shimPort,
	  (int) timShimTable[ dac ].board,
	  (int) timShimTable[ dac ].dac,
		value,
		0 );

   }
   else
   {   // since both are updated no decisions..
      input_record[dac] = value;
      ttoken1 = input_record[m1] + input_record[m2];
      ttoken2 = input_record[m1] - input_record[m2];
      if ((ttoken1 > 32767) || (ttoken1 < -32767) ||
          (ttoken2 > 32767) || (ttoken2 < -32767))
      {
         input_record[token1] = priorvalue;
         errLogRet(LOGIT,debugInfo,"Shim DAC pair at limit\n");
         //DPRINT2(-3,"index = %d reset to %d\n",token1,priorvalue);
         return(-1);  /* don't set these values*/
         /*return(HDWAREERROR+SHIMDACLIM); */ /* nothing handles this error */
      }
      DPRINT5(-2,"\n   Shim pair %s: %d <= %d , %d <= %d",hstr,m1,ttoken1,m2,ttoken2);
      retval = setTimShim( shimPort,
	  (int) timShimTable[ m1 ].board,
	  (int) timShimTable[ m1 ].dac,
		ttoken1, 0 );
      if ( ! retval )
      {
	   retval = setTimShim( shimPort,
	     (int) timShimTable[ m2 ].board,
	     (int) timShimTable[ m2 ].dac,
	   	ttoken2, 0 );
      }
   }
   return( retval );


}


setSerialShim( int dac, int value )
{
	int	retval;

	if (dac < 0)
	  return( -1 );
	if (dac >= sizeof( timShim1Table ) / sizeof( timShim1Table[ 0 ] ))
	  return( -1 );
	if (shimPort < 0)
	  return( -1 );

	if (timShimTable[ dac ].board == 7)
	  return( -1 );

        if (thinShimsPresent)
           return( setThinShim( dac, value) );
        
	retval = setTimShim(
		shimPort,
	  (int) timShimTable[ dac ].board,
	  (int) timShimTable[ dac ].dac,
		value,
		0 );

	return( retval );
}
#endif


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
	 NULL
};

static char RRIcmd = 'S';
static int shimstat = 0;
static int  gradOK[sizeof(rriShimTable)/sizeof(rriShimTable[0])];

/*
 * This routine is called only for RRI shims.  It first decides if
 * the old shimmer software or the new mxserver software is present
 * in the RRI. It does this by testing whether or not the PC (prepare
 * coarse) command is present.  It is not present in old software.
 * If the new software is present, we use the PC command to disable
 * DACs which are not present. Finally, if the z7 dac is not present,
 * we assume the ULTRA18 shims are present.
 */
initGradTest()
{
   int index;
   int stat;
  
   /* Set all dacs to present */
   for (index = 0; index < sizeof(rriShimTable)/sizeof(rriShimTable[0]);
        index++)
   {
      gradOK[index] = 1;
   }
   if (pShimMutex != NULL)
   {
      semTake(pShimMutex,WAIT_FOREVER); /* protect shim serial comm */
   }
   else
   {
      return(ULTRA);
   }
   clearport(shimPort);
   pputchr(shimPort, 'P');
   cmdecho(shimPort, ECHO);
   pputchr(shimPort, 'C');
   stat = cmddone(shimPort, 100);
   if (stat == 200)
   {
/*
 * The PC command is not recognized. This must be old RRI SHIMMER software
 */
      semGive(pShimMutex);
      return(ULTRA);
   }
   pputchr(shimPort, '\r');

/*
 * At this point an error should occur for the MXSERVER RRI software.
 * There are two cases.
 * 1. MxServer with no error handling should not give an error number
 *    but it will give an error message about "Incorrect number of
 *    arguments".
 * 2. MxServer with error handling should give error 201 ""Incorrect number of
 *    argument".
 */

   stat = cmddone(shimPort, 500);
        
   semGive(pShimMutex);
   if (stat != 201)
      return(ULTRA);
   RRIcmd = 'P';
   for (index = 0; index < sizeof(rriShimTable)/sizeof(rriShimTable[0]);
        index++)
   {
      setRRIShim(index, 0, 0);
/*
 * A return value of 202 is given by RRI's mxserver if a gradient is
 * not present
 */
      if (shimstat == 202)
      {
         DPRINT1(1,"turn off dac %d\n",index);
         gradOK[index] = 0;
      }
      else if (shimstat != 0)
      {
        DPRINT3(1,"dac %d (%s) status= %d\n",
                index,rriShimTable[index],shimstat);
      }
   }
   RRIcmd = 'S';
   /* test if z7 (dac 10) is present */
   return( (gradOK[10] == 0) ? ULTRA18 : ULTRA);
}

setRRIShim( int dac, int value, int fifoFlag/*NOTUSED*/)
{
   /* Note: "fifoFlag" arg is for compatibility with Nirvana header file. */
   char	*chrptr;

   if (dac < 0)
      return( -1 );
   if (shimPort < 0)
      return( -1 );
   if (dac >= sizeof( rriShimTable ) / sizeof( rriShimTable[ 0 ] ))
      return( -1 );
   if (gradOK[dac] == 0)
   {
      DPRINT1(1,"turned off dac %d\n",dac);
      return(0);
   }
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
   clearport(shimPort);
   pputchr(shimPort, RRIcmd);
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
      errLogRet( LOGIT, debugInfo, "set RRI shim failed setting DAC value\n" );
      semGive(pShimMutex);
      return(TIMEOUT);
   }
   pputchr(shimPort, '\r');		/* NOT <L/F> !!! */

   shimstat = cmddone(shimPort, 500);
   semGive(pShimMutex);
   return( 0 );
}

/* just a debugging aid */
setMSRserial()
{
   shimType = MSR_SERIAL_SHIMS;
}

#ifdef DIAG
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
#endif
