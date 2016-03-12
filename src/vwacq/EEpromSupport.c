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
/* EEpromSupport.c */

#include <vxWorks.h>
#include <stdio.h>
#include <ctype.h>
#include <msgQLib.h>
#include "logMsgLib.h"
#if CPU==CPU32
#include "qspiObj.h"
#endif
#include "EEpromSupport.h"

int EEdiagFlag;

/* Keep these elements is order */
/* The integer is used in a case statement */
char HELP[] = "help";
#define help 0
char DTM16[] = "dtm16";
#define dtm16 1
char DTM64[] = "dtm64";
#define dtm64 2
char DTM4[] = "dtm4";
#define dtm4 3
char ADC[] = "adc";
#define adc 4
char AUTO[] = "auto";
#define auto 5
char ACQ[] = "acq";
#define acq 6
char SHIM[] = "shim";
#define shim 7
char IMAGE[] = "image";
#define image 8
char RRI[] = "rri";
#define rri 9

#define NoCpu 0
#define VME 162
#define QSPI 332

#ifndef INCqspih
#define	ThisType VME
#else
#define ThisType QSPI
#endif

struct EEpromType
{
	int code;
	char *EEname;
	int cpu;
};
typedef struct EEpromType EEpT;

EEpT	EEproms [] =
{
	help,  HELP,  NoCpu,
	dtm16, DTM16,  VME,
	dtm64, DTM64,  VME,
	dtm4,  DTM4,   VME,
	adc,   ADC,    VME,
	auto,  AUTO,   VME,
	acq,   ACQ,    VME,
	shim,  SHIM,  QSPI,
	image, IMAGE, QSPI,
	rri,   RRI,   QSPI,
};

#define EEpromTypeSize (sizeof(EEproms)/sizeof(EEpT))

void boardHelp(char *rtnname)
{
int i;

	printf("\n%s",rtnname);
	for(i = 1; i < EEpromTypeSize; i++)
	{
		if(EEproms[i].cpu == ThisType)
			printf(" [%s]",EEproms[i].EEname);
	}
	/* calling routine will terminate line with unique params and newline */
}


int identifyBoard(char *Board)
{
int i;
int j;
char lcboard[10];
int lcboardchrs;

	if(Board == NULL) return(0);
	else
	{
		for(i=1;i<EEpromTypeSize;i++)
		{
			if( (lcboardchrs = strlen(Board)) >= (sizeof(lcboard) -1) )
				lcboardchrs = sizeof(lcboard) - 1;
			for(j=0;j<lcboardchrs;j++) lcboard[j] = tolower(Board[j]);
			lcboard[j] = '\0';
			if( (strcmp(lcboard,EEproms[i].EEname)) == 0 ) return(i);
		}
		return(help);	/* fake help if no match */
	}

} /* end int identifyBoard(char *Board) */


#ifndef INCqspih
int getParams(char *Board, volatile unsigned long **fetch_addr, int *numberOfChars, char **TestPattern)
{
#else
int getParams(char *Board, volatile unsigned char **fetch_addr, int *numberOfChars, char **TestPattern)
{
#endif

static char   AcqPattern[] =
	"ACQ CTL                   1001 Board=0190201000Z,PWB=0190201100F 12Oct95,Schem=0190201300F 11Oct1995,U2K=0190249700A,U3D=0190249600A,U15B=0190249800A.";

static char Dtm16Pattern[] =
	"DTM 16 Meg.               2101 Board=0190202201B,PWB=0190202300G 19Aug95,Schem=0190202500G 18Aug1995,U1C=0190260200B,U3C=0190250800D,U3H=0190251001B,U4B=0190250700B,U5A=0190251400A,U5C=0190250900B,U6B=0190250501A,U6C=0190250300B,U8C=0190250400A.";

static char Dtm64Pattern[] =
	"DTM - 64 Meg.             2201 Board=0190202202B,PWB=0190202300G 19Aub95,Schem=0190202500G 18Aug1995,U1C=0190260200B,U3C=0190250800D,U3H=0190251001B,U4B=0190250700B,U5A=0190251400A,U5C=0190250900B,U6B=0190250501A,U6C=0190250300B,U8C=0190250400A.";

static char  Dtm4Pattern[] =
	"DTM 4 Meg.                2301 Board=0190202203B,PWB=0190202300G 19Aug95,Schem=0190202500G 18Aug1995,U1C=0190260200B,U3C=0190250800D,U3H=0190251001B,U4B=0190250700B,U5A=0190251400A,U5C=0190250900B,U6B=0190250501A,U6C=0190250300B,U8C=0190250400A.";

static char   AdcPattern[] =
	"NMR-DATA ADC DUAL 500Kc   3001 Board=0190202601B,PWB=0190202701G,Schem=0190202900G,U10C=0190251400A,U10E=0190251500A,U12B=0190260201B,U14C=0190251203A.";

static char  AutoPattern[] =
	"MAG-REG                   4001 Board=0190203001A,PWB=0190203100F 14Sep95,Schem=0190203300F 06Sep1995,U3B=0190250100A,U7A=0190250000A,U7C=0190249900A.";

static char EEprom00Data[] =
	"SHIM Single QSPI Standard   10 Board=0190229200A 11Jul95,Schem=0190229500A 11Jun1995,U4J=01902561A 21Mar95.";

static char EEprom01Data[] =
	"SHIM Single QSPI Imaging    08 Board=0190229201A 11Jul95,Schem=0190229500A 11Jun1995,U4J=01902561A 21Mar95.";

static char EEprom02Data[] =
	"SHIM Single QSPI RRI        05 Board=0190229202A 11Jul95,Schem=0190229500A 11Jun1995,U4J=01902561A 21Mar95.";

static char  GDPattern[] =
	"GD Test                   0001 Board=0190203001A,PWB=0190203100F 14Sep95,Schem=0190203300F 06Sep1995,U3B=0190250100A,U7A=0190250000A,U7C=0190249900A.";

	switch(identifyBoard(Board))
	{
		case help:
			boardHelp(*TestPattern);
			return(help);

		case dtm16:
#ifndef	INCqspih
			*fetch_addr = STM_EEP_BASE;
#else
			printf("DTM16 EEprom is NOT accessable from Automation board!\n");
			return(ERROR);
#endif
			*TestPattern = Dtm16Pattern;
			*numberOfChars = sizeof(Dtm16Pattern) -1;	/* Don't write trailing Zero!! */
			return(dtm16);

		case dtm64:
#ifndef	INCqspih
			*fetch_addr = STM_EEP_BASE;
#else
			printf("DTM64 EEprom is NOT accessable from Automation board!\n");
			return(ERROR);
#endif
			*TestPattern = Dtm64Pattern;
			*numberOfChars = sizeof(Dtm64Pattern) -1;	/* Don't write trailing Zero!! */
			return(dtm64);

		case dtm4:
#ifndef	INCqspih
			*fetch_addr = STM_EEP_BASE;
#else
			printf("DTM4 EEprom is NOT accessable from Automation board!\n");
			return(ERROR);
#endif
			*TestPattern = Dtm4Pattern;
			*numberOfChars = sizeof(Dtm4Pattern) -1;	/* Don't write trailing Zero!! */
			return(dtm4);

		case adc:
#ifndef	INCqspih
			*fetch_addr = ADC_EEP_BASE;
#else
			printf("ADC EEprom is NOT accessable from Automation board!\n");
			return(ERROR);
#endif
			*TestPattern = AdcPattern;
			*numberOfChars = sizeof(AdcPattern) -1;	/* Don't write trailing Zero!! */
			return(adc);

		case auto:
#ifndef	INCqspih
			*fetch_addr = AUTO_EEP_BASE;
#else
			printf("Automation EEprom is NOT accessable from Automation board!\n");
			return(ERROR);
#endif
			*TestPattern = AutoPattern;
			*numberOfChars = sizeof(AutoPattern) -1;	/* Don't write trailing Zero!! */
			return(auto);

		case acq:
#ifndef	INCqspih
			*fetch_addr = OUTPUT_EEP_BASE;
#else
			printf("ACQ CTL EEprom is NOT accessable from Automation board!\n");
			return(ERROR);
#endif
			*TestPattern = AcqPattern;
			*numberOfChars = sizeof(AcqPattern) -1;	/* Don't write trailing Zero!! */
			return(acq);

		case shim:
#ifndef	INCqspih
			printf("Serial EEprom is ONLY accessable from Automation board!\n");
			return(ERROR);
#else
			*fetch_addr = QSPI_SHIM_EEP_BASE;
			*TestPattern = EEprom00Data;
			*numberOfChars = sizeof(EEprom00Data) -1;	/* Don't write trailing Zero!! */
#endif
			return(shim);

		case image:
#ifndef	INCqspih
			printf("Serial EEprom is ONLY accessable from Automation board!\n");
			return(ERROR);
#else
			*fetch_addr = QSPI_SHIM_EEP_BASE;
			*TestPattern = EEprom01Data;
			*numberOfChars = sizeof(EEprom01Data) -1;	/* Don't write trailing Zero!! */
#endif
			return(image);

		case rri:
#ifndef	INCqspih
			printf("Serial EEprom is ONLY accessable from Automation board!\n");
			return(ERROR);
#else
			*fetch_addr = QSPI_SHIM_EEP_BASE;
			*TestPattern = EEprom02Data;
			*numberOfChars = sizeof(EEprom02Data) -1;	/* Don't write trailing Zero!! */
#endif
			return(rri);
	}
}


int dumpEEprom(char *Board, int charsToDump, int startAddr)
{
#ifndef INCqspih
volatile unsigned long *fetch_addr;
#else
volatile unsigned char *fetch_addr;
#endif
char *TestPattern;
char routine[] = "dumpEEprom";
int numberOfChars;
int bdStat;
int rdStat;
int i;
int readchars;
int charsRead;
char databuf[26];

	TestPattern = routine;	/* So getParams can identify routine */
	bdStat = getParams(Board, &fetch_addr, &numberOfChars, &TestPattern);
	if(bdStat <= 0)
	{
		if(bdStat < 0)return(bdStat);
		printf(" charsToDump [startAddr]\n");
		return(bdStat);
	}

	if(charsToDump == NULL)
	{
		printf("\n  *** Need charsToDump!! ***\n");
		return(ERROR);
	}
	else
	{
		if(charsToDump > 512)
		{
			printf("\n Current limit of dump is 512 bytes!\n");
			return(ERROR);
		}
	}


	/* loop on Read till numberOfChars has been displayed. */

	fetch_addr += startAddr;	/* adjust start */
	rdStat = 0;	/* for first time thru loop, set=0. */
	for(charsRead = 0; charsRead < charsToDump;)
	{
		/* fetch_addr now holds fwa EEprom for selected board. add startAddr and begin. */
		if(EEdiagFlag & 0x100)
			printf("\ndumpEEprom: fa=0x%08x, sa=%d,",fetch_addr,startAddr);
		fetch_addr += rdStat;

		if( (readchars = (charsToDump - charsRead) ) > 24 ) readchars = 24;
		if(EEdiagFlag & 0x100)
			printf(" fa+sc=0x%08x, rc=%d\n",fetch_addr,readchars);

		if( readchars <= 0)
		{
			printf("\ndumpEEprom: internal ERROR readchars <= 0 rc=%d, ctd=%d, cr=%d\n",
				readchars, charsToDump, charsRead);
			return(ERROR);
		}

#ifndef INCqspih
		rdStat = EEpromRead(fetch_addr,readchars,databuf);
#else
		rdStat = shimEEpromRead(fetch_addr,readchars,databuf);
#endif

		if(rdStat == ERROR)
		{
			printf("\n Error on Read EEprom\n");
			return(ERROR);
		}

		charsRead += rdStat;

		printf("\ndumpEEprom: from=%d, to=%d, of=%d. HEX\n",
			startAddr,startAddr + rdStat,charsToDump);
		for(i=0;i<rdStat;i++) printf(" %02x",( databuf[i] & 0x000000ff) );

		printf("\ndumpEEprom: from=%d, to=%d, of=%d. ASCII\n",
			startAddr,startAddr + rdStat,charsToDump);
		for(i=0;i<rdStat;i++) printf(" %c",( databuf[i] & 0x000000ff) );
		printf("\n");

		startAddr += rdStat;
	
	}	/* end for(charsRead = 0; charsRead < charsToDump) */

	printf("\ndumpEEprom: charsRead = %d\n",charsRead);
	return(charsRead);

} /* end int dumpEEprom(char *Board, int charsToDump, int startAddr) */


#define DATAsize 64
#define PRINTsize 80

int chkEEprom(char *Board)
{
#ifndef INCqspih
volatile unsigned long *fetch_addr;
long EEpromAddr;
#else
volatile unsigned char *fetch_addr;
int EEpromAddr;
#endif
int charsRead;
int RdStat;
char *TestPattern;
char routine[] = "chkEEprom";
int numberOfChars;
int bdStat;
int i;
int AllDone;	/* When FALSE, done. Return */
int chkPN;           /* TRUE check next char for U */
char databuf[DATAsize + 2];
int dataindex;
char printbuf[PRINTsize + 2];
int printindex;

	TestPattern = routine;
	bdStat = getParams(Board, &fetch_addr, &numberOfChars, &TestPattern);
	if(bdStat <= 0)
	{
		if(bdStat < 0)return(bdStat);
		printf("\n");
		return(bdStat);
	}

	for(i=0;i<DATAsize;i++) databuf[i]='\0';

  printf("%s: fetch_addr = 0x%08x\n",routine,fetch_addr);
#ifndef INCqspih
	RdStat = EEpromRead(fetch_addr,32,databuf);
#else
	RdStat = shimEEpromRead(fetch_addr,32,databuf);
#endif
	if(RdStat == ERROR) return(ERROR);
	charsRead = RdStat;

	if(EEdiagFlag & 0x20)
	{
		printf("%s21:\n",routine);
		for(i=0;i<32;i++) printf(" %02x",( databuf[i] & 0x000000ff) );
	}

	if( (databuf[0] == (char)0xff) || (databuf[0] == '\0') )
	{
		printf("\n%s: EEprom has NOT been programmed!\n",routine);
		return(0);
	}

	/* Check if valid data in default record */
	for(i=0; i<32;i++)
	{
		if( (databuf[i] == (char)0xff) || (databuf[i] == '\0') )
		{
			printf("%s: Default first 32 characters are invalid!\n",routine);
			printf("\n\"%s\"\n",databuf);	/* print partial data */
			return(ERROR);
		}
	}

	databuf[32] = '\0';
	printf("\n\"%s\"\n",databuf);	/* default 32 characters */

	chkPN = TRUE;
	dataindex = printindex = 0;
	AllDone = FALSE;
	EEpromAddr = 32;

	while(AllDone == FALSE)
	{
		if(dataindex == 0)	/* read one buffer full */
		{
#ifndef INCqspih
			RdStat = EEpromRead(fetch_addr + EEpromAddr,DATAsize,databuf);
#else
			RdStat = shimEEpromRead(fetch_addr + EEpromAddr,DATAsize,databuf);
#endif
			if(RdStat == ERROR)
			{
				printf("%s: error reading EEprom at address = %d\n",routine,EEpromAddr);
				return(ERROR);
			}

			if(EEdiagFlag & 0x40)
			{
				printf("%s41:\n",routine);
				for(i=0;i<DATAsize;i++) printf(" %02x",( databuf[i] & 0x000000ff) );
				printf("\n%s: addr=%d\n",routine,EEpromAddr);
			}
		}

		if( (databuf[dataindex] == (char)0xff) || (databuf[dataindex] == '\0') )
		{
			printf("\n%s: End of data before terminal period or comma.\n",routine);
			printf("addr = %d, chars read = %d\n",EEpromAddr,charsRead);
			return(ERROR);
		}

		if(chkPN)
		{
			if(EEdiagFlag & 0x80) printf("chkPN81:\n");
			if(databuf[dataindex] == '=')
			{
				if(EEdiagFlag & 0x80) printf("chkPN82: = TRUE\n");
				chkPN = 2;
				printbuf[printindex++] = ' ';
				printbuf[printindex++] = databuf[dataindex++];
				printbuf[printindex++] = ' ';
				charsRead++;
			}
			else
			{
				if(EEdiagFlag & 0x80) printf("chkPN83: else not =\n");
				if( (databuf[dataindex] > '9') && (chkPN == 2) )
				{
					printbuf[printindex++] = ' ';
					printbuf[printindex++] = 'R';
					printbuf[printindex++] = 'e';
					printbuf[printindex++] = 'v';
					printbuf[printindex++] = '.';
					chkPN = FALSE;
				}

				printbuf[printindex++] = databuf[dataindex++];
				charsRead++;
			}
		}
		else
		{
			if(EEdiagFlag & 0x80) printf("chkPN84: else\n");
			printbuf[printindex] = databuf[dataindex++];
			charsRead++;

			if( (printbuf[printindex] == ',') || (printbuf[printindex] == '.') )
			{
				if(EEdiagFlag & 0x80) printf("chkPN85: else - comma or period\n");
				/* end of statement, time to print? */

				chkPN = TRUE;
				if( ( printindex > ( (PRINTsize * 2) / 3) ) || (printbuf[printindex] == '.') )
				{
					if(EEdiagFlag & 0x80) printf("chkPN86: index> or period\n");
					if(printbuf[printindex] == '.') AllDone = TRUE;
					printindex++;
					printbuf[printindex] = '\0';	/* terminate print string */
					printf("%s\n\n",printbuf);
					printindex = 0;	/* reset print buffer */
				}
				else
				{
					if(EEdiagFlag & 0x80) printf("chkPN87: index> or period - else\n");
					printindex++;
					printbuf[printindex++] = ' ';
				}

			}
			else
			{
				if(EEdiagFlag & 0x80) printf("chkPN88: else - comma or period - else\n");
				printindex++;
			}

		}	/* end if(chkPN) */

		if(EEdiagFlag & 0x80) printf("%s81: dataindex = %d, printindex = %d\n",routine,dataindex,printindex);
		if(dataindex >= DATAsize)
		{
			EEpromAddr += DATAsize;	/* Read next part of EEprom */
			dataindex = 0;
		}
		if(EEdiagFlag & 0x100) printf("%s82: dataindex = %d, EEpromAddr = %d\n",routine,dataindex,EEpromAddr);

	}	/* end while(AllDone == FALSE) */
		if(EEdiagFlag & 0x200) printf("%s82: dataindex = %d, EEpromAddr = %d\n",routine,dataindex,EEpromAddr);

	return(charsRead);
}	/* end int chkEEprom(char *Board) */


int getEEpromInquiry(char *Board)
{
#ifndef INCqspih
volatile unsigned long *fetch_addr;
#else
volatile unsigned char *fetch_addr;
#endif
char *TestPattern;
char routine[] = "getEEpromInquiry";
int numberOfChars;
int bdStat;
int RdStat;
char EEpromData[33];

	TestPattern = routine;	/* So getParams can identify routine */
	bdStat = getParams(Board, &fetch_addr, &numberOfChars, &TestPattern);
	if(bdStat <= 0)
	{
		if(bdStat < 0)return(bdStat);
		printf("\n");
		return(bdStat);
	}

#ifndef INCqspih
	RdStat = EEpromRead(fetch_addr + 0, 32, EEpromData);
#else
	RdStat = shimEEpromRead(fetch_addr + 0, 32,EEpromData);
#endif
	if(RdStat > 0) printf("%s\n.",EEpromData);
	return(RdStat);

}	/* end int getEEpromInquiry(char *Board) */

#ifndef INCqspih
int getEEpromBrdId(unsigned long BoardEEBaseAddr)
{
  char chr;
  int IdNum,sign,i;
  int numberOfChars;
  int bdStat;
  int RdStat;
  int CntrCFound;
  char EEpromData[65];

  DPRINT1(2,"getEEpromBrdId: EEBaseAddr: 0x%lx\n",BoardEEBaseAddr);
  RdStat = EEpromRead(BoardEEBaseAddr, 64, EEpromData);
  if(RdStat > 0) 
  {
    IdNum = CntrCFound = 0;
    for(i=0;i<35;i++)
    {
       chr = EEpromData[i];
       if ( chr == '' )
       {
	     IdNum = 0;
	     CntrCFound = 1;
       }
       /* DPRINT3(5,"char[%d] = '%c', 0x%x\n",i,chr,chr); */
       if ( !(chr >= '0' && chr <= '9') && (IdNum > 0) && (CntrCFound == 1))
	  break;
       if (chr == '-') sign = -1;  /* sign of value neg. */
       if (chr >= '0' && chr <= '9')  IdNum = IdNum*10 + (chr - '0');
       DPRINT2(4,"getEEpromBrdID: chr: '%c', ID: %d\n",chr,IdNum);
    }
  }
  else
  {
    IdNum = -1;
  }
  DPRINT1(2,"Board Id Number: %d\n",IdNum);
  return(IdNum);
}

int getEEpromBrdID(char *Board)
{
  volatile unsigned long *fetch_addr;
  char *TestPattern;
  char routine[] = "getEEpromBrdID";
  char chr;
  int IdNum,sign,i;
  int numberOfChars;
  int bdStat;
  int RdStat;
  char EEpromData[33];

  TestPattern = routine;	/* So getParams can identify routine */
  bdStat = getParams(Board, &fetch_addr, &numberOfChars, &TestPattern);
  if(bdStat <= 0)
  {
    return(-1);
  }
  RdStat = EEpromRead(fetch_addr + 0, 32, EEpromData);
  if(RdStat > 0) 
  {
    IdNum = 0;
    for(i=0;i<32;i++)
    {
       chr = EEpromData[i];
       if ( chr == '' )
	  IdNum = 0;
       if (chr == '-') sign = -1;  /* sign of value neg. */
       if (chr >= '0' && chr <= '9')  IdNum = IdNum*10 + (chr - '0');
       DPRINT2(4,"getEEpromBrdID: chr: '%c', ID: %d\n",chr,IdNum);
    }
  }
  else
  {
    IdNum = -1;
  }
  return(IdNum);
}
#endif

int getEEpromHex(char *Board, int NoBytes, int EEpromAddr)
{
#ifndef INCqspih
volatile unsigned long *fetch_addr;
#else
volatile unsigned char *fetch_addr;
#endif
char *TestPattern;
char routine[] = "getEEpromHex";
int numberOfChars;
int bdStat;
int RdStat;
register int i;
unsigned char EEpromData[515];

	TestPattern = routine;	/* So getParams can identify routine */
	bdStat = getParams(Board, &fetch_addr, &numberOfChars, &TestPattern);
	if(bdStat <= 0)
	{
		if(bdStat < 0)return(bdStat);
		printf(" charsToGet [startAddr]\n");
		return(bdStat);
	}

	if(NoBytes == NULL)
	{
		printf("\n  *** Need charsToGet!! ***\n");
		return(ERROR);
	}
	else
	{
		if(NoBytes > 512)
		{
			printf("\n Current limit of dump is 512 bytes!\n");
			return(ERROR);
		}
	}

#ifndef INCqspih
	RdStat = EEpromRead(fetch_addr + EEpromAddr, 32, EEpromData);
#else
	RdStat = shimEEpromRead(fetch_addr + EEpromAddr, 32,EEpromData);
#endif
	if(RdStat > 0)
	{
		printf("EEpromData @ Addr %d =\n",EEpromAddr);
		for(i=0;i<NoBytes;i++)
		{
			printf(" %02x",EEpromData[i]);
			if( ( (i+1)%32) == 0) printf("\n");
		}
		printf("\n");
	}
	return(RdStat);
}	/* end int getEEpromHex(char *Board, int NoBytes, int EEpromAddr) */


int getEEpromData(char *Board, int NoBytes, int EEpromAddr)
{
#ifndef INCqspih
volatile unsigned long *fetch_addr;
#else
volatile unsigned char *fetch_addr;
#endif
char *TestPattern;
char routine[] = "getEEpromData";
int numberOfChars;
int bdStat;
int RdStat;
register int i;
unsigned char EEpromData[515];

	TestPattern = routine;	/* So getParams can identify routine */
	bdStat = getParams(Board, &fetch_addr, &numberOfChars, &TestPattern);
	if(bdStat <= 0)
	{
		if(bdStat < 0)return(bdStat);
		printf(" charsToGet [startAddr]\n");
		return(bdStat);
	}

	if(NoBytes == NULL)
	{
		printf("\n  *** Need charsToDump!! ***\n");
		return(ERROR);
	}
	else
		if(NoBytes > 512)
		{
			printf("\n Current limit of dump is 512 bytes!\n");
			return(ERROR);
		}

#ifndef INCqspih
		RdStat = EEpromRead(fetch_addr + EEpromAddr, NoBytes, EEpromData);
#else
		RdStat = shimEEpromRead(fetch_addr + EEpromAddr, NoBytes, EEpromData);
#endif
		if(RdStat > 0) printf("EEpromData = \"%s\"\n",EEpromData);
	return(RdStat);

}	/* end int getEEpromData(char *Board, int NoBytes, int EEpromAddr) */

/* end EEpromSupport.c */
