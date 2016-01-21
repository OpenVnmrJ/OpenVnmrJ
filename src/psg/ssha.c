/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <string.h>

#include "acodes.h"
#include "group.h"
#include "pvars.h"
#include "abort.h"
#include "ssha.h"

#ifndef MAXSTR
#define MAXSTR 256
#endif

#define SSHA_Z1_INDEX	2
#define SSHA_Z1C_INDEX	3
#define SSHA_Z2_INDEX	4
#define SSHA_Z2C_INDEX	5
#define SSHA_X1_INDEX	6   /* different from shims.c */
#define SSHA_Y1_INDEX	7   /* different from shims.c */
#define SSHA_MAX_SHIMS	12  /* or 13, 0-12 */
#define SSHA_OFF	0
#define SSHA_ON		1

extern int	newacq;
extern int putcode();

/*  Configured status is based on hdwshim.  Currently it is on or off,
    but it could get more values if the hdwshim parameter is asked to
    do more than just show on or off.  If there is no hdwshim parameter,
    then SSHA will be configured off.

    Current status is either off or on.  On means at this point in the
    pulse sequence SSHA would be active in the console.  Similarly off
    means is has been turned off.  SSHA can be configured on but turned
    off; the opposite, configured off but turn on, is not allowed.	*/

static struct {
	int	configuredStatus;
	int	presatConfiguredStatus;
	int	currentStatus;
	int	doItNow; /* set only at beginning of pulse sequence
				(unless explicitly set later) */
	int	delayTooShort; /* flagged if delay is too short */
	int	presatInit;
	int	hardloopDisable; /* disable ssha during hardloop */
} sshaStatus;


int initSSHA()
{
	char	hdwshim[ 4 ];

	sshaStatus.currentStatus = SSHA_OFF;
	sshaStatus.hardloopDisable = SSHA_OFF;

	if (newacq) 
	{
		if (P_getstring(GLOBAL, "hdwshim", hdwshim, 1, 3) < 0) 
		{
		  text_error( "global parameter 'hdwshim' not found\n" );
		  sshaStatus.configuredStatus = SSHA_OFF;
		}
		else 
		{
		  if (hdwshim[ 0 ] == 'y' || hdwshim[ 0 ] == 'Y')
		    sshaStatus.configuredStatus = SSHA_ON;
		  else
		    sshaStatus.configuredStatus = SSHA_OFF;
		  if (hdwshim[ 0 ] == 'p' || hdwshim[ 0 ] == 'P')
		    sshaStatus.presatConfiguredStatus = SSHA_ON;
		  else
		    sshaStatus.presatConfiguredStatus = SSHA_OFF;
		}
	}
	else	/* not INOVA */
	{
	  sshaStatus.configuredStatus = SSHA_OFF;
	  sshaStatus.presatConfiguredStatus = SSHA_OFF;
	}
	sshaStatus.doItNow = SSHA_OFF;
	sshaStatus.delayTooShort = SSHA_ON;
	sshaStatus.presatInit = SSHA_OFF;

	return( 0 );
}

void initSSHAshimmethod()
{
	char hdwshimmethod[ MAXSTR ];
	int shimMask, rval;

	if (newacq && (isSSHAselected() || isSSHAPselected()))
	{
	  shimMask = (short) (1 << SSHA_Z1_INDEX); /* max 12 shims */
	  strcpy( hdwshimmethod, "" );
	  if (P_getstring(GLOBAL, "hdwshimlist", hdwshimmethod, 1, MAXSTR) == 0) 
	  {
	    rval = translateSSHAmethod( hdwshimmethod );
	    if (rval > 0) shimMask = rval;
	  }
	  putcode( SSHA );
	  putcode( 3 );
	  putcode( shimMask ); /* max 16-bit 32767 */
	}
}


int translateSSHAmethod(char hdwshimmethod[] )
{
	double dshimset;
	int rval, found, i, coarseflag;
	char tmpSSHAmethod[MAXSTR], hdwshimmethod_name[MAXSTR], errstr[MAXSTR];

	strcpy( hdwshimmethod_name, "hdwshimlist" );
	coarseflag = 0;   /* flag if z1c,z2c can be used */
	if (P_getreal(GLOBAL, "shimset", &dshimset, 1) < 0) 
	{
	  text_error("PSG: global parameter 'shimset' not found.\n");
	  psg_abort(1);
	}
	else
	{
	  switch ((int)(dshimset+0.5))
	  {
	    case 1: case 2: case 5: case 10: coarseflag = 1; break;
	    default: coarseflag = 0; break;
	  }
	}
	rval = 0;
	found = 0;
	i = 0;
	strcpy( tmpSSHAmethod, hdwshimmethod );
	strcpy( errstr, "" );
	while (tmpSSHAmethod[i] != '\0' && i<100)
	{
	  if (tmpSSHAmethod[i] == '\0')
	  {
	    text_error("%s=%s not valid, using default 'z1'\n", hdwshimmethod_name, hdwshimmethod);
	    rval = -1;
	  }
	  if (tmpSSHAmethod[i]=='z' && tmpSSHAmethod[i+1]=='1' && tmpSSHAmethod[i+2]=='c')
	  {
	    if (coarseflag == 1)
	      rval |= (1 << SSHA_Z1C_INDEX);
	    else
	      text_error("%s error: z1c not available with shimset=%g\n", hdwshimmethod_name, dshimset);
	    i += 3;
	  }
	  else if (tmpSSHAmethod[i]=='z' && tmpSSHAmethod[i+1]=='2' && tmpSSHAmethod[i+2]=='c')
	  {
	    if (coarseflag == 1)
	      rval |= (1 << SSHA_Z2C_INDEX);
	    else
	      text_error("%s error: z2c not available with shimset=%g\n", hdwshimmethod_name, dshimset);
	    i += 3;
	  }
	  else if (tmpSSHAmethod[i]=='z' && tmpSSHAmethod[i+1]=='1')
	  {
	    rval |= (1 << SSHA_Z1_INDEX);
	    i += 2;
	  }
	  else if (tmpSSHAmethod[i]=='z' && tmpSSHAmethod[i+1]=='2')
	  {
	    rval |= (1 << SSHA_Z2_INDEX);
	    i += 2;
	  }
	  else if (tmpSSHAmethod[i]=='x' && tmpSSHAmethod[i+1]=='1')
	  {
	    rval |= (1 << SSHA_X1_INDEX);
	    i += 2;
	  }
	  else if (tmpSSHAmethod[i]=='y' && tmpSSHAmethod[i+1]=='1')
	  {
	    rval |= (1 << SSHA_Y1_INDEX);
	    i += 2;
	  }
	  else
	  {
	    if ((tmpSSHAmethod[i] != ',') && (tmpSSHAmethod[i] != ' '))
	      sprintf(errstr,"%s%c",errstr,tmpSSHAmethod[i]);
	    i++;
	    continue;
	  }
	}
	if (strcmp(errstr,"") != 0)
	  text_error("%s: extra characters '%s' ignored\n", hdwshimmethod_name, errstr);
	if (rval==0)
	{
	  rval = -1;
	  if (strcmp(errstr,"") != 0)
	    text_error("%s='%s' not valid, using default 'z1'\n", hdwshimmethod_name, tmpSSHAmethod);
	}
	return( rval );
}


void hdwshiminit()
{
	if (sshaStatus.configuredStatus == SSHA_ON) 
	{
	  sshaStatus.doItNow = SSHA_ON;
	}
	if (sshaStatus.presatConfiguredStatus == SSHA_ON) 
	{
	  sshaStatus.presatInit = SSHA_ON;
	}

}

void hdwshiminitPresat()
{
	if (sshaStatus.presatConfiguredStatus == SSHA_ON) 
	{
	  if (sshaStatus.presatInit == SSHA_ON)
	  {
	    sshaStatus.doItNow = SSHA_ON;
	    sshaStatus.presatInit = SSHA_OFF;
	  }
	}
}

void setSSHAdelayTooShort()
{
	sshaStatus.delayTooShort = SSHA_ON;
}

void setSSHAdelayNotTooShort()
{
	sshaStatus.delayTooShort = SSHA_OFF;
}

void setSSHAdisable()
{
	if (sshaStatus.doItNow == SSHA_ON)
	{
	  sshaStatus.doItNow = SSHA_OFF;
	  sshaStatus.hardloopDisable = SSHA_ON;
	}
}
void setSSHAreenable()
{
	if (sshaStatus.hardloopDisable == SSHA_ON)
	{
	  sshaStatus.doItNow = SSHA_ON;
	  sshaStatus.hardloopDisable = SSHA_OFF;
	}
}

void setSSHAoff()
{
	sshaStatus.doItNow = SSHA_OFF;
	sshaStatus.presatInit = SSHA_OFF;
}

void turnOnSSHA(double delaytime)
{
	int inttime;
	if ( sshaStatus.configuredStatus == SSHA_ON || 
		sshaStatus.presatConfiguredStatus == SSHA_ON )
	{
	  if (sshaStatus.doItNow == SSHA_ON) 
	  {
	    if (sshaStatus.currentStatus == SSHA_OFF) 
	    {
		sshaStatus.currentStatus = SSHA_ON;
		if (delaytime > 32767) delaytime = 32767;
		inttime = (short)( delaytime );
		if (inttime < 0) inttime = 0;
		putcode( SSHA );
		putcode( 1 );
		putcode( inttime ); /* max 16-bit */
	    }
	  }
	}
}

void turnOffSSHA()
{
	if ( sshaStatus.configuredStatus == SSHA_ON ||
		sshaStatus.presatConfiguredStatus == SSHA_ON )
	{
	  sshaStatus.doItNow = SSHA_OFF;
	  if (sshaStatus.currentStatus != SSHA_OFF) 
	  {
		sshaStatus.currentStatus = SSHA_OFF;
		putcode( SSHA );
		putcode( 0 );
		putcode( 0 ); /* dummy value */
	  }
	}
}

int isSSHAselected()
{
	return( sshaStatus.configuredStatus == SSHA_ON );
}

int isSSHAPselected()
{
	return( sshaStatus.presatConfiguredStatus == SSHA_ON );
}

int isSSHAactive()
{
	return( sshaStatus.currentStatus == SSHA_ON );
}

int isSSHAstillDoItNow()
{
	return( sshaStatus.doItNow == SSHA_ON );
}

int isSSHAdelayTooShort()
{
	return( sshaStatus.delayTooShort == SSHA_ON );
}

int isSSHAPresatInit()
{
	return( sshaStatus.presatInit == SSHA_ON );
}
