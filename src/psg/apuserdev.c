/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Used for ap user device functions.  */

#include <stdio.h>
#include <sys/types.h>

#include "oopc.h"
#include "acodes.h"
#include "acqparms.h"
#include "macros.h"
#include "abort.h"
#include "delay.h"
#include "apuserdev.h"

#ifdef DEBUG

#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stdout,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stdout,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif


extern int      bgflag;
extern int 	newacq;
extern codeint	dtmcntrl;

extern Object ObjectNew();
extern int AP_Device();
extern int putcode();
extern int SetAPAttr(Object attnobj, ...);

Object APbyte = NULL;

#define BOB_READ_SYNC_DELAY	0.005 	/* 5 msec */

/*--------------------------------------------------------------*/
/* The following routines support reading and writing for 	*/
/* general or user apbus devices.				*/
/* The BreakOut Board is supported here.			*/
/*--------------------------------------------------------------*/

void inituserapobjects()
{
    if (newacq)
    {
	BOBreg0  = ObjectNew(AP_Device,   "BreakOut Board register 0");
	BOBreg1  = ObjectNew(AP_Device,   "BreakOut Board register 1");
	BOBreg2  = ObjectNew(AP_Device,   "BreakOut Board register 2");
	BOBreg3  = ObjectNew(AP_Device,   "BreakOut Board register 3");
	APall    = ObjectNew(AP_Device,   "Generic APdevice object");
	APbyte   = ObjectNew(AP_Device,   "Generic APdevice object");

	SetAPAttr(BOBreg0,
		     SET_APADR,			BOB_APADDR,
		     SET_APREG,			BOB_REG,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);
	SetAPAttr(BOBreg1,
		     SET_APADR,			BOB_APADDR,
		     SET_APREG,			BOB_REG+1,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);
	SetAPAttr(BOBreg2,
		     SET_APADR,			BOB_APADDR,
		     SET_APREG,			BOB_REG+2,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);
	SetAPAttr(BOBreg3,
		     SET_APADR,			BOB_APADDR,
		     SET_APREG,			BOB_REG+3,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);
	SetAPAttr(APall,
		     SET_APADR,			0,
		     SET_APREG,			0,
		     SET_APBYTES,		2,
		     SET_APMODE,		1,
		NULL);
	SetAPAttr(APbyte,
		     SET_APADR,			0,
		     SET_APREG,			0,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);
    }

}

void setBOB(int value, int reg)
{
   char            msge[128];

    if (!newacq)
    {
	text_error("Warning: setuserap available only on Inova systems.\n");
	text_error("         setuserap ignored.\n");
	return;
    }
    switch (reg)
    {
	case 0:
		SetAPAttr(BOBreg0, SET_MASK, (c68int)(value), 
							SET_VALUE, NULL);
		break;
	case 1:
		SetAPAttr(BOBreg1, SET_MASK, (c68int)(value),
							SET_VALUE, NULL);
		break;
	case 2:
		SetAPAttr(BOBreg2, SET_MASK, (c68int)(value),
							SET_VALUE, NULL);
		break;
	case 3:
		SetAPAttr(BOBreg3, SET_MASK, (c68int)(value),
							SET_VALUE, NULL);
		break;
	default:
		sprintf(msge,"setuserap: Invalid register = %d.\n",reg);
		text_error(msge);
        	psg_abort(1);
    }

}

void vsetBOB(int rtparam, int reg)
{
   char            msge[128];

    if (!newacq)
    {
	text_error("Warning: vsetuserap available only on Inova systems.\n");
	text_error("         vsetuserap ignored.\n");
	return;
    }
    switch (reg)
    {
	case 0:
		SetAPAttr(BOBreg0, SET_RTPARAM, (c68int)(rtparam), NULL);
		break;
	case 1:
		SetAPAttr(BOBreg1, SET_RTPARAM, (c68int)(rtparam), NULL);
		break;
	case 2:
		SetAPAttr(BOBreg2, SET_RTPARAM, (c68int)(rtparam), NULL);
		break;
	case 3:
		SetAPAttr(BOBreg3, SET_RTPARAM, (c68int)(rtparam), NULL);
		break;
	default:
		sprintf(msge,"vsetuserap: Invalid register = %d.\n",reg);
		text_error(msge);
        	psg_abort(1);
    }

}

void vreadBOB(int rtparam, int reg)
{
    int addrreg;
    char msge[128];

    if (!newacq)
    {
	text_error("Warning: readuserap available only on Inova systems.\n");
	text_error("         readuserap ignored.\n");
	return;
    }
    addrreg = (BOB_APADDR << 8) | (BOB_REG);
    switch (reg)
    {
	case 0:
		vapread(rtparam,addrreg,BOB_READ_SYNC_DELAY);
		break;
	case 1:
		vapread(rtparam,addrreg+1,BOB_READ_SYNC_DELAY);
		break;
	case 2:
		vapread(rtparam,addrreg+2,BOB_READ_SYNC_DELAY);
		break;
	case 3:
		vapread(rtparam,addrreg+3,BOB_READ_SYNC_DELAY);
		break;
	default:
		sprintf(msge,"readuserap: Invalid register = %d.\n",reg);
		text_error(msge);
        	psg_abort(1);
    }

}

void apset(int value, int addrreg)
{
    if (!newacq)
    {
	text_error("Warning: apset available only on Inova systems.\n");
	text_error("         apset ignored.\n");
	return;
    }
    SetAPAttr(APall,
	SET_APADR,		((addrreg >> 8) & 0x0f),
	SET_APREG,		(addrreg & 0x0ff),
	SET_MASK, 		(c68int)(value),
	SET_VALUE,
	NULL);
}

void apsetbyte(int value, int addrreg)
{
    if (!newacq)
    {
	text_error("Warning: apsetbyte available only on Inova systems.\n");
	text_error("         apsetbyte ignored.\n");
	return;
    }
    SetAPAttr(APbyte,
	SET_APADR,		((addrreg >> 8) & 0x0f),
	SET_APREG,		(addrreg & 0x0ff),
	SET_MASK, 		(c68int)(value & 0xff),
	SET_VALUE,
	NULL);
}

void vapset(int rtparam, int addrreg)
{
    if (!newacq)
    {
	text_error("Warning: apset available only on Inova systems.\n");
	text_error("         apset ignored.\n");
	return;
    }
    SetAPAttr(APall,
	SET_APADR,		((addrreg >> 8) & 0x0f),
	SET_APREG,		(addrreg & 0x0ff),
	SET_RTPARAM, 		(c68int)(rtparam),
	NULL);
}

void vapread(int rtparam, int addrreg, double apsyncdelay)
{
    int ticks,secs;
    short *ptr;

    if (!newacq)
    {
	text_error("Warning: vapread available only on Inova systems.\n");
	text_error("         vapread ignored.\n");
	return;
    }
    
    timerwords(apsyncdelay,&ticks,&secs);

    putcode((c68int)IAPREAD);
    putcode((c68int)addrreg);
    putcode((c68int)rtparam);
    ptr = (short *) &secs;
    putcode((codeint) *ptr);	/* timerword one */
    ptr++;
    putcode((codeint) *ptr);	/* timerword one */
    ptr = (short *) &ticks;
    putcode((codeint) *ptr);	/* timerword one */
    ptr++;
    putcode((codeint) *ptr);	/* timerword one */
	
}

/*--------------------------------------------------------------*/
/* signal_acqi_updt_cmplt 					*/
/*	Inserts a interrupt to tell the parser that the update	*/
/* 	interval has completed and it is time to start parsing	*/
/* 	acodes.							*/
/*--------------------------------------------------------------*/
void signal_acqi_updt_cmplt()
{
    if (!newacq)
    {
	text_error("Warning: signal_acqi_updt_cmplt available only on Inova systems.\n");
	text_error("         signal_acqi_updt_cmplt ignored.\n");
	return;
    }
    putcode((c68int)IACQIUPDTCMPLT);
}

/*--------------------------------------------------------------*/
/* start_acqi_updt	 					*/
/*	Starts the update interval.  signal_acqi_updt_cmplt	*/
/*	will signal the acq computer that the update interval	*/
/*	has finished.  The parser then has the time between  	*/
/*	the signal_acqi_updt_cmplt element and this element to	*/
/*	parse and stuff enough acodes to prevent FIFO Underflow.*/
/*	This assumes the parser	can parse faster than the 	*/
/* 	sequence can execute.				 	*/
/*--------------------------------------------------------------*/
void start_acqi_updt(double delaytime)
{
    if (!newacq)
    {
      text_error("Warning: start_acqi_updt available only on Inova systems.\n");
      text_error("         start_acqi_updt ignored.\n");
      return;
    }
    delay(delaytime);
    putcode((c68int)ISTARTACQIUPDT);
}

/*--------------------------------------------------------------*/
/* setnumpoints	 						*/
/*	Updates number of points to acquire given by the  	*/
/*	real-time variables.  Resets dtmcntrl value after	*/
/*	setting new number of points.				*/
/*--------------------------------------------------------------*/
void setnumpoints(codeint rtindex)
{
   putcode(ISETNP);	/*  */
   putcode((codeint)rtindex);
   putcode((codeint)dtmcntrl);
}

/*--------------------------------------------------------------*/
/* setdataoffset	 					*/
/*	Offsets the data location from the beginning of scan.  	*/
/*	The data offset must be in bytes.			*/
/*--------------------------------------------------------------*/
void setdataoffset(codeint rtdataoffset)
{
   putcode(ISETDATAOFFSET);	/*  */
   putcode((codeint)rtdataoffset);
   putcode((codeint)rtdataoffset);
   putcode((codeint)dtmcntrl);
}
