/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "rfconst.h"
#include "acodes.h"
#include "acqparms.h"
/*-------------------------------------------------------------------
|
|	setPTS()/3 
|	set PTS for old style RF scheme
|				Author Greg Brissey  6/30/86
+------------------------------------------------------------------*/
extern char syn[];	/* syn[0] = PTS for trans(y or n) syn[1] = PTS for dec*/
extern int  bgflag;	/* debug flag */
extern int  curfifocount;
extern int  H1freq;	/* instrunment proton frequency */
extern int  ptsval[];	/* PTS type for trans & dec */

extern char ptsoff[];	/* NEW PTS type for trans & dec */
int  setptsflag = 1;	/* debug flag */

setPTS(ptsvalue,device)
double ptsvalue;	/* value to set PTS */	
int device;
{

    	int bcd;
    	int digit;
    	int digoffset;	/* reg. offset on XL interface */
    	int ival;
	int num;
    	int ptstype;
    	int sisptsflag;	/* required by new SIS offset scheme */

        sisptsflag=((ptsoff[device - 1]=='y') || (ptsoff[device - 1]=='Y'));
	if (bgflag)
	    fprintf(stderr,
		"setPTS(): pts value = %lg, device=%d sisflag=%d\n",
	  	ptsvalue,device,sisptsflag);
	/* --- create APBUSS words to control PTS --- */
    					/* not rounding, making sure 30.0=30*/
	if (!sisptsflag) ival = (int) ((ptsvalue*10.0) + 0.0005);
	if (bgflag)
	    fprintf(stderr,
	     "setPTS(): pts value = %d(100KHz)\n",ival);
	ptstype = ptsval[device-1];		/* device PTS: 160,200,etc.*/
	if (ptstype < 160) 	/* if true there is NO PTS for this device */
	{
            if (device == TODEV)
	      text_error("ptsval has an invalid value for transmitter\n");
            else
	      text_error("ptsval has an invalid value for decoupler\n");
	    abort(1);
	}
	if (device == TODEV)
	    digoffset = PTS1OFFSET;	/* reg. offset on XL interface */
	else
	    digoffset = PTS2OFFSET;
	if ( ptstype == 160 )
	    num = 3;			/* number of apbuss words to create */
	else
	    num = 4;			/* number of apbuss words to create */
        for(digit = (0 + digoffset); digit < (digoffset + num); digit++)
        {
	    /* --- test for PTS160 case --- */
	    if ( (ptstype == 160) && (digit >= (digoffset + (num-1))) )
	    {
	        bcd = ival % 16;	/* Hex not BCD */
	        ival /= 16;
	    }
	    else
	    {  bcd = ival % 10;	/* BCD */
	       ival /= 10;
	    }
	    bcd = 0x8000 | (PTSDEV << DEVPOS) | (digit << DIGITPOS) | bcd ;
	    putcode((codeint) bcd);
	    curfifocount++;		/* increment # fifo words */
	}
}

/*------------------------------------------------------------------
: setSISPTS(mainpts,offpts,device,setpts)
------------------------------------------------------------------*/

/* Program to drive PTS-Interface*/
/* by Jerry Signer*/

# define	ap_init 0xa000	/*AP S2 Phase*/
# define	ap_seq	0x9000	/*AP S1 Phase*/
# define	obspts	1
# define	obsofs	0
# define	dcplpts	3
# define	dcplofs	2
# define	ptsboar	7

setSISPTS(base,offset,device,setoop)
int base,offset;	/*base in 100KHz, offset in .1Hz*/
int setoop,device;	/*setoop: set offset or both: 0 only offset*/
			/*1 Main PTS + Offset*/
			/*device: 1=trans, 2=decoupler*/
{
    if (bgflag)
      fprintf(stderr, "setSISPTS: mainpts = %d, ifreq = %d , device = %d, setpts = %d\n",
	      base , offset, device, setoop);
	if (device == 1) {
		apcodes(ptsboar,obsofs,ptsofs(offset/10));
		if (setoop == 1)
		apcodes(ptsboar,obspts,~ptsconv(base));
		}
	if (device == 2) {
		apcodes(ptsboar,dcplofs,ptsofs(offset/10));
		if (setoop == 1)
		apcodes(ptsboar,dcplpts,~ptsconv(base));
		}
}

apcodes(boardadd,breg,longw)
int boardadd, breg;
int longw; /*is already the 32 bits to write into AP-Chip*/
{
int init, seq1, seq2, seq3, seq4;

init = ap_init | (boardadd << 8) | (breg << 2) | 0x0003;
seq1 = ap_seq | (boardadd << 8) | (longw & 0x000000ff);
seq2 = ap_seq | (boardadd << 8) | ((longw >> 8) & 0xff);
seq3 = ap_seq | (boardadd << 8) | ((longw >> 16) & 0xff);
seq4 = ap_seq | (boardadd << 8) | ((longw >> 24) & 0xff);
   if (bgflag)
      fprintf(stderr, "setSISPTSjerry: init = %x, seq1 = %x, seq2 = %x, seq3 = %x, seq4 = %x\n",
	      init,seq1,seq2,seq3,seq4);
putcode((codeint) init);
putcode((codeint) seq1);
putcode((codeint) seq2);
putcode((codeint) seq3);
putcode((codeint) seq4);
curfifocount += 5;

return;
}	/*end setacodes*/

/* 1.0	10/7/87 */

int ptsconv(int value)
/* conversion program to convert a int number (10 <= x =< 4999) into
   the form for the pts output. Returned value is of type long, BCD coded
   32bit coded
   word: 	
		bit	value
		16-19	100kHz
		20-23	1MHz
		24-27	10MHz
		28-31	100MHz
*/
{
int II, valueint;
int ptsbcd;
	{
		ptsbcd = 0;
		for (II=0; II<=3; II++) {
		ptsbcd = ptsbcd + (value % 10) * lngpow(2,(16+4*II));
			value = value / 10;
			}
	return(ptsbcd);
}}

int ptsofs(int ofsint)
/* subroutine to convert an integer number in Hz (offset) into a bcd AP
   Bus chip form. Returned value is long.
   The bits have to be set as followed:
		bits	value
		0-3	1Hz
		4-7	10Hz
		8-11	100Hz
		12-15	1kHz
		16-19	10kHz
		20-23	100kHz
		24	1MHz
		25	2MHz
*/
{
int II;
int ptsobcd = 0;
	{
		for (II=0; II<=6; II++) {
		ptsobcd = ptsobcd + (ofsint%10) * lngpow(2,(II*4));
		ofsint = ofsint / 10;
		}
		return(ptsobcd);
	}
}

int lngpow(int a, int b)
/* function to calculate power (pow)
*/
{
int respow = 1;
int II;
	if (b==0) return(respow);
	for (II=1; II <= b; II++) 
		respow= respow * 2;
	return(respow);
}

/*------------------------------------------------------------------*/

