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
/* demo.c */

#include <vxWorks.h>
#include <stdioLib.h>
#include <stdio.h>
#include <ioLib.h>
#include <semLib.h>
#include "quart.h" /* AUTO specific info */
#include "sc68c94.h" /* quad definitions */

IMPORT TYQ_CO_DEV tyQCoDv [];	/* device descriptors */

extern setupQ(int port,int Onflag);


/* quartTm Transparent Mode from stdin/stdout to "port" */

int quartTm(int porta, int chEcho, int portb)
{
int sPort,dPort;
int QdemoInt;
int XitCode;
int sread0,swrite0,sreadd,swrited;
char sChrBuf,dChrBuf,dChrCr,dChrNl;
char sChrPer;
unsigned char *sStat;
unsigned char *sIO;
unsigned char *dStat;
unsigned char *dIO;

	QdemoInt = tyQCoDv[0].created;

	dChrNl = '\n';
	dChrCr = '\r';

	/* Resolve ambiguity of ports! if porta=0 then if portb=0 make it 4.
	*  Port 4 is to be understood as stdin.
	*/

	if( (porta == 0) && (portb == 0) ) portb = 4;

	/* check if illegal port requested */
	if( (porta == portb) || (porta > 3) || (portb > 4) ||
		 (porta < 0) || (portb < 0) )
	{
		printf("\nquartTm Qporta(0-3),echo,portb(0-3,4{4=stdin})\n");
		return(1);
	}

	printf("\nquartTm: portA=%d, portB=%d, Int=%d\n",porta,portb,QdemoInt);
	taskDelay(60);

	if(portb == 4)
	{
		sPort = open("/tyCo/0", O_RDWR, 0777);	/* open stdin */
		if(sPort < 1) return(2);
		/* Clear buffer */
		while(pifchr( sPort )) sread0 = read( sPort, &sChrBuf, 1);
	}
	else
	{
		if(QdemoInt)
		{
			sPort = initSerialPort( portb );	/* open target QUART port */
			if(sPort < 1) return(3);
			while(pifchr( sPort )) sread0 = read( sPort, &sChrBuf, 1);
		}
		else
		{
			switch(portb)
			{
				case 0:
					sStat = (unsigned char *) 0x200002;
					sIO = (unsigned char *) 0x200006;
					break;
				case 1:
					sStat = (unsigned char *) 0x200012;
					sIO = (unsigned char *) 0x200016;
					break;
				case 2:
					sStat = (unsigned char *) 0x200022;
					sIO = (unsigned char *) 0x200026;
					break;
				case 3:
					sStat = (unsigned char *) 0x200032;
					sIO = (unsigned char *) 0x200036;
					break;
	
			}
			setupQ(portb,TRUE);
			while( *sStat & 0x1 ) sChrBuf = *dIO;
		}
	}

	if(QdemoInt)
	{
		dPort = initSerialPort( porta );	/* open target QUART port */
		if(dPort < 1)
		{
			close(sPort);
			return(2);
		}
		while(pifchr( sPort )) sread0 = read( sPort, &sChrBuf, 1);
	}
	else
	{
		switch(porta)
		{
			case 0:
				dStat = (unsigned char *) 0x200002;
				dIO = (unsigned char *) 0x200006;
				break;
			case 1:
				dStat = (unsigned char *) 0x200012;
				dIO = (unsigned char *) 0x200016;
				break;
			case 2:
				dStat = (unsigned char *) 0x200022;
				dIO = (unsigned char *) 0x200026;
				break;
			case 3:
				dStat = (unsigned char *) 0x200032;
				dIO = (unsigned char *) 0x200036;
				break;

				dStat = (unsigned char *) 0x200002;
				dIO = (unsigned char *) 0x200006;
		}
		setupQ(porta,TRUE);
		while( *dStat & 0x1 ) dChrBuf = *dIO;
	}

	FOREVER
	{
		if(QdemoInt)
		{
			if(pifchr( sPort ))
			{
				sread0 = read( sPort, &sChrBuf, 1);
    		if(sread0 < 1)
				{
					XitCode = 0x24;
					break;
				}
			}
		}
		else
		{
			if( *sStat & 0x1 )
			{
				sChrBuf = *sIO;
				sread0 = 1;
			}
		}
		if(sread0 > 0)
		{
			sread0 = 0;
			if(sChrBuf == '!')
			{
				XitCode = 0x10;
				break;
			}
			if(chEcho != 0)
			{
				if(QdemoInt)
				{
					swrite0 = write( sPort, &sChrBuf, 1 );
					if(swrite0 != 1)
					{
						XitCode = 0x11;
						break;
					}
					if(sChrBuf == '\r')
					{
						swrite0 = write( sPort, &dChrNl, 1 );
						if(swrite0 != 1)
						{
							XitCode = 0x12;
							break;
						}
					}
					if(sChrBuf == '\n')
					{
						swrite0 = write( sPort, &dChrCr, 1 );
						if(swrite0 != 1)
						{
							XitCode = 0x13;
							break;
						}
					}
				}
				else
				{
					*sIO = sChrBuf;
					if(sChrBuf == '\n') *sIO = '\r';
					if(sChrBuf == '\r') *sIO = '\n';
				}
			}

			if(QdemoInt)
			{
				swrite0 = write( dPort, &sChrBuf, 1 );
				if(swrite0 != 1)
				{
					sChrPer = '0' + swrite0;
					swrite0 = write( sPort, &sChrPer, 1 );
					if(swrite0 != 1)
					{
						XitCode = 0x31;
						break;
					}
				}
				if(sChrBuf == '\r')
				{
					swrite0 = write( dPort, &dChrNl, 1 );
					if(swrite0 != 1)
					{
						XitCode = 0x15;
						break;
					}
				}
				if(sChrBuf == '\n')
				{
					swrite0 = write( dPort, &dChrCr, 1 );
					if(swrite0 != 1)
					{
						XitCode = 0x16;
						break;
					}
				}
			}
			else
			{
				*dIO = sChrBuf;
				if(sChrBuf == '\n') *dIO = '\r';
				if(sChrBuf == '\r') *dIO = '\n';
			}
		}
		
		/* Here check for character on portb */

		if(QdemoInt)
		{
			if(pifchr( dPort ))
			{
				sreadd = read( dPort, &dChrBuf, 1);
    		if(sreadd < 1)
				{
					XitCode = 0x24;
					break;
				}
			}
		}
		else
		{
			if( *dStat & 0x1 )
			{
				dChrBuf = *dIO;
				sreadd = 1;
			}
		}
		if(sreadd > 0)
		{
			sreadd = 0;
			if(dChrBuf == '!')
			{
				XitCode = 0x20;
				break;
			}

			if(chEcho != 0)
			{
				if(QdemoInt)
				{
					swrited = write( dPort, &dChrBuf, 1 );
					if(swrited != 1)
					{
						XitCode = 0x23;
						break;
					}
					if(dChrBuf == '\n')
					{
						swrited = write( dPort, &dChrCr, 1 );
						if(swrited != 1)
						{
							XitCode = 0x21;
							break;
						}
					}
					if(dChrBuf == '\r')
					{
						swrited = write( dPort, &dChrNl, 1 );
						if(swrited != 1)
						{
							XitCode = 0x22;
							break;
						}
					}
				}
				else
				{
					*dIO = dChrBuf;
					if(dChrBuf == '\n') *dIO = '\r';
					if(dChrBuf == '\r') *dIO = '\n';
				}
			}

			/* Here echo portb to porta */

			if(QdemoInt)
			{
				swrited = write( sPort, &dChrBuf, 1 );
				if(swrited != 1)
				{
					XitCode = 0x23;
					break;
				}
				if(dChrBuf == '\n')
				{
					swrited = write( sPort, &dChrCr, 1 );
					if(swrited != 1)
					{
						XitCode = 0x21;
						break;
					}
				}
				if(dChrBuf == '\r')
				{
					swrited = write( sPort, &dChrNl, 1 );
					if(swrited != 1)
					{
						XitCode = 0x22;
						break;
					}
				}
			}
			else
			{
				*sIO = dChrBuf;
				if(dChrBuf == '\n') *sIO = '\r';
				if(dChrBuf == '\r') *sIO = '\n';
			}
		} /* end if (sreadd > 0) */
		taskDelay(2);
	} /* end FOREVER */

	if( (QdemoInt) || (portb == 4) ) close(sPort);
	else setupQ(portb,FALSE);

	if(QdemoInt) close(dPort);
	else setupQ(porta,FALSE);

	return(XitCode);
} /* end quartTm */


int startTm(int dPort, int dEcho, int sPort)
{
int tid;

	if(dPort < 0) dPort = 0;
	if(dPort > 3) dPort = 3;
	if(sPort < 0) sPort = 0;
  if(sPort > 4) sPort = 4;
	tid = taskSpawn("tquartTm",55,0,8192,quartTm,
		dPort,dEcho,sPort,0,0,0,0,0,0,0);
	return(tid);
}


testICR(int pattern)
{
unsigned char orig;
unsigned char patt;
unsigned char now;

	orig = *(unsigned char *) 0x200058;
	patt = (unsigned char)pattern;
	*(unsigned char *) 0x200058 = patt;
	now = *(unsigned char *) 0x200058;
	printf("testICR: orig=0x%02x, patt=0x%02x, now=0x%02x\n",orig,patt,now);
}


int setupQ(int port,int OnFlag)
{
unsigned char *dMr;
unsigned char *dCr;
unsigned char *dCsr;
unsigned char *dImr;

	*(unsigned char *) 0x20100b = 0x01; /* kill 68332 interrupt inhibit */

	switch(port)
	{
		case 0:
			dMr = (unsigned char *) 0x200000;
			dCr = (unsigned char *) 0x200004;
			dCsr = (unsigned char *) 0x200002;
			dImr = (unsigned char *) 0x20000a;
			break;
		case 1:
			dMr = (unsigned char *) 0x200010;
			dCr = (unsigned char *) 0x200014;
			dCsr = (unsigned char *) 0x200012;
			dImr = (unsigned char *) 0x20001a;
			break;
		case 2:
			dMr = (unsigned char *) 0x200020;
			dCr = (unsigned char *) 0x200024;
			dCsr = (unsigned char *) 0x200022;
			dImr = (unsigned char *) 0x20002a;
			break;
		case 3:
			dMr = (unsigned char *) 0x200030;
			dCr = (unsigned char *) 0x200034;
			dCsr = (unsigned char *) 0x200032;
			dImr = (unsigned char *) 0x20003a;
			break;

			dMr = (unsigned char *) 0x200000;
			dCr = (unsigned char *) 0x200004;
			dCsr = (unsigned char *) 0x200002;
			dImr = (unsigned char *) 0x20000a;
	}

	*dCr = 0x20; /* reset receiver */
	*dCr = 0x30; /* reset transmitter */
	*dCr = 0x40; /* reset error status */
	*dCr = 0x50; /* reset break status */
	*dCr = 0x10; /* reset MR pointer to reg 1 */
	*dMr = 0x13; /* MR1 = No parity 8 bits */
	*dMr = 0x07; /* MR2 = 1 stop bit */
	*dCsr = 0xbb; /* 9600 baud */
	*(unsigned char *) 0x200052 = 0x80; /* set IVR */
	*dImr = 0x00; /* No interrupts */
	if(OnFlag >= 1) *dCr = 0x05; /* enable receiver and transmitter */

	return(port);
}


void setQuartReg(int Qreg, int Qdata)
{
unsigned char *QuartReg;

	QuartReg = (unsigned char *)0x200000 + Qreg;
	*QuartReg = (char)Qdata & (char)0xff;
}


int getQuartReg(int Qreg)
{
int Qdata;
unsigned char *QuartReg;

	QuartReg = (unsigned char *)0x200000 + Qreg;
	Qdata = *QuartReg;
	return(Qdata);
}


void updateCIR()
{
	*(unsigned char *) 0x200054 = 0x05;
}


/*******************************************************************************
*
* quartShow - report register status for the QUART
*
* RETURNS: N/A
*/

void quartshow()
{
int ii;			/* scratch */
char mrReg0[4];
char mrReg1[4];
char mrReg2[4];
char srReg[4];
char ipcr_oprReg[4];
char isr_iprReg[4];
char bcrReg[4];
char imrReg[4];
char cirReg;
char gicrReg;
char icrReg;
int heartbeatReg[4];
char baudReg[4];

int lock = intLock ();	/* LOCK INTERRUPTS */

	for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
	{
		*tyQCoDv [ii].cr  = SET_MR_PTR_ZRO;
		mrReg0[ii] = *tyQCoDv [ii].mr; /* read contents of reg */
		mrReg1[ii] = *tyQCoDv [ii].mr;
		mrReg2[ii] = *tyQCoDv [ii].mr;
		*tyQCoDv [ii].cr  = RST_MR_PTR_CMD;
		srReg[ii] = *tyQCoDv [ii].sr;
		ipcr_oprReg[ii] = *tyQCoDv [ii].ipcr;
		isr_iprReg[ii] = *tyQCoDv [ii].isr;
		bcrReg[ii] = *tyQCoDv [ii].bcr;
		imrReg[ii] = *tyQCoDv [ii].kimr;
		heartbeatReg[ii] = tyQCoDv [ii].heartbeat;
		baudReg[ii] = tyQCoDv [ii].baud;
	}

	cirReg = *QSC_CIR;
	gicrReg = *QSC_IVR;
	icrReg = *QSC_ICR;

	intUnlock (lock);				/* UNLOCK INTERRUPTS */

/**********************************
* Now print the results out
*/

	printf("\n");

	for (ii = 0; ii < tyQCoDv [0].numChannels; ii ++)
	{
		printf("Q%d mr0=0x%02x, mr1=0x%02x, mr2=0x%02x, sr=0x%02x, bcr=0x%02x, imr=0x%02x, baud=%d, hb=%d\n",
			ii, (mrReg0[ii] & 0xff), (mrReg1[ii] & 0xff), (mrReg2[ii] & 0xff),
			(srReg[ii] & 0xff), (bcrReg[ii] & 0xff), (imrReg[ii] & 0xff),
			(baudReg[ii]), heartbeatReg[ii]);

		if(ii & 1)
			printf("ipcr=0x%02x, isr=0x%02x, opr=0x%02x, ipr=0x%02x\n",
				(ipcr_oprReg[ii-1] & 0xff), (isr_iprReg[ii-1] & 0xff),
				(ipcr_oprReg[ii] & 0xff), (isr_iprReg[ii] & 0xff) );
	}
	printf("cir=0x%02x, gicr=0x%02x, icr=0x%02x\n",
		(cirReg & 0xff), (gicrReg & 0xff), (icrReg & 0xff) );

}

/* end serialdemo.c */
