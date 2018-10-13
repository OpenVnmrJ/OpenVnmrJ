/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>

#include "STAT_DEFS.h"

/**********************************************************************
 *
 * associative database to connect possible Expproc (and Acqproc)
 * replies with more useful messages for our users
 *
 */

#define  sizeofArray( a )	(sizeof( a ) / sizeof( a[ 0 ] ))

static struct {
	int	 index;
	int	 constant;
	char	*expprocReply;
	char	*userMessage;
} acqAssociativeDBase[] = {
	{ 0,  ACQ_INTERACTIVE, "INTERACTIVE",
	     "console in interactive mode" },
	{ 1,  ACQ_INACTIVE,    "DOWN",
	     "console powered down or not connected to the host" },
	{ 2,  ACQ_TUNING,      "TUNING",
	     "console in tune mode" },
	{ 3,  ACQ_ACQUIRE,     "NO",
	     "acquisition in progress" },
	{ 4,  ACQ_AUTO,        "auto",
	     "automation run in progress" },
	{ 5,  ACQ_IDLE,        "OK1",
	     "console available" },
	{ 6,  ACQ_NEEDSU,      "OK2",
	     "please enter the VNMR command su before proceeding" },
	{ 7,  ACQ_IDLE,        "OK",
	     "console available" },
	{ 8,  ACQ_ACQUIRE,     "ACQUIRING",  
	     "acquisition in progress" },
	{ -1, ACQ_UNKNOWN,     "",
	     "console not available for an unknown reason" },
};

/*  OK1 is actually an Acqproc (UnityPLUS) message;
    put here for reference and convenience		*/

/*  The constant field allows the application to get a result
    expressed as a constant, with possible values obtained from
    an include file.						*/

/*  The last entry is a default, to be used if the expproc reply
    can't be found.  Since index2UserMessage relies on this one
    being the last one, please leave it that way.		*/

/*  If a longer string matches a shorter string, put the longer
    string first.  Otherwise the search algorithm will match the
    shorter one without even looking at the longer one.		*/

int
expprocReply2Index( expprocReply )
char *expprocReply;
{
	int	entrylen, iter, minlen, replylen;

	if (expprocReply == NULL)
	  return( -1 );
	replylen = strlen( expprocReply );
	if (replylen < 1)
	  return( -1 );

	for (iter = 0; iter < sizeofArray( acqAssociativeDBase ); iter++) {
		entrylen = strlen( acqAssociativeDBase[ iter ].expprocReply );
		if (entrylen < 1)
		  continue;
		if (entrylen < replylen)
		  minlen = entrylen;
		else
		  minlen = replylen;
		if (strncmp( expprocReply, acqAssociativeDBase[ iter ].expprocReply, minlen ) == 0)
		  return( iter );
	}

	return( -1 );
}

char *
index2UserMessage( index )
int index;
{
	int	iter;

	if (index > sizeofArray( acqAssociativeDBase ) - 2 || index < 0)
	  return( &acqAssociativeDBase[ sizeofArray( acqAssociativeDBase ) -1 ].userMessage[ 0 ] );
	else
	  return( &acqAssociativeDBase[ index ].userMessage[ 0 ] );
}

int
index2Constant( index )
int index;
{
	int	iter;

	if (index > sizeofArray( acqAssociativeDBase ) - 2 || index < 0)
	  return( acqAssociativeDBase[ sizeofArray( acqAssociativeDBase ) -1 ].constant );
	else
	  return( acqAssociativeDBase[ index ].constant );
}

