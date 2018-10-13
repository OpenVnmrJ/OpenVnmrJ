/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "abort.h"

#define INOVA_SP1	1<<21
#define INOVA_SP2	1<<22

extern int  bgflag;	/* debug flag */
extern int  ap_interface;
extern int  spare12_hs_bits;
extern int  newacq;

/*------------------------------------------------------------------
|
|	var1on()
|	 turn on var 1 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
var1on()
{
    okinhwloop();
    if (ap_interface < 4 )
       HSgate(VAR1,TRUE);
}
/*------------------------------------------------------------------
|
|	var1off()
|	 turn off var 1 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
var1off()
{
    okinhwloop();
    if (ap_interface < 4 )
       HSgate(VAR1,FALSE);
}
/*------------------------------------------------------------------
|
|	var2on()
|	 turn on var 2 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
var2on()
{
    okinhwloop();
    if (ap_interface < 4 )
       HSgate(VAR2,TRUE);
}
/*------------------------------------------------------------------
|
|	var2off()
|	 turn off var 2 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
var2off()
{
    okinhwloop();
    if (ap_interface < 4 )
       HSgate(VAR2,FALSE);
}

/*------------------------------------------------------------------
|
|	sp1on()
|	 turn on spare 1 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
sp1on()
{
    okinhwloop();
    if (ap_interface < 4)
       HSgate(SP1,TRUE);
    else
    {
	if (newacq)
	{
          HSgate(INOVA_SP1,TRUE);
	}
	else
	{
          spare12_hs_bits |= 0x10;
          putcode(SPARE12);
          putcode(spare12_hs_bits);
	}
    }
}
/*------------------------------------------------------------------
|
|	sp1off()
|	 turn off spare 1 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
sp1off()
{
    okinhwloop();
    if (ap_interface < 4)
       HSgate(SP1,FALSE);
    else
    {
	if (newacq)
	{
          HSgate(INOVA_SP1,FALSE);
	}
	else
	{
          spare12_hs_bits &= ~0x10;
          putcode(SPARE12);
          putcode(spare12_hs_bits);
	}
    }
}
/*------------------------------------------------------------------
|
|	sp2on()
|	 turn on spare 2 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
sp2on()
{
    okinhwloop();
    if (ap_interface < 4)
       HSgate(SP2,TRUE);
    else
    {
	if (newacq)
	{
          HSgate(INOVA_SP2,TRUE);
	}
	else
	{
          spare12_hs_bits |= 0x20;
          putcode(SPARE12);
          putcode(spare12_hs_bits);
	}
    }
}
/*------------------------------------------------------------------
|
|	sp2off()
|	 turn off spare 2 bit on the high speed lines.
|				Author Greg Brissey 7/10/86
+-----------------------------------------------------------------*/
sp2off()
{
    okinhwloop();
    if (ap_interface < 4)
       HSgate(SP2,FALSE);
    else
    {
	if (newacq)
	{
          HSgate(INOVA_SP2,FALSE);
	}
	else
	{
          spare12_hs_bits &= ~0x20;
          putcode(SPARE12);
          putcode(spare12_hs_bits);
	}
    }
}
/*------------------------------------------------------------------
|
|	sp_on()/1
|	 turn on spare 3-5 bit on the high speed lines.
+-----------------------------------------------------------------*/
sp_on(bitn)
int bitn;
{
    unsigned long sparelines;
    if (newacq)
    {
	if ((bitn >= 3) && (bitn <= 5))
	{
	    sparelines = 1 << (20+bitn) ;
            HSgate(sparelines,TRUE);
	}
	else
	{
            text_error( "sp_on(): Invalid bit specified = %d.\n",bitn);
	    psg_abort(1);
	}
    }
    else
    {
            text_error("sp_on() only callable on INOVA systems.\n");
	    psg_abort(1);
    }
}
/*------------------------------------------------------------------
|
|	sp_off()/1
|	 turn off spare 3-5 bit on the high speed lines.
+-----------------------------------------------------------------*/
sp_off(bitn)
int bitn;
{
    unsigned long sparelines;
    if (newacq)
    {
	if ((bitn >= 3) && (bitn <= 5))
	{
	    sparelines = 1 << (20+bitn) ;
            HSgate(sparelines,FALSE);
	}
	else
	{
            text_error("sp_off(): Invalid bit specified = %d.\n",bitn);
	    psg_abort(1);
	}
    }
    else
    {
            text_error("sp_off(): Only callable on INOVA systems.\n");
	    psg_abort(1);
    }
}
