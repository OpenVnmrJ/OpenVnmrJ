/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "parser.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern int abortCodes();
extern int debugLevel();
extern int chkQueue();
extern int listCmds();
extern int mapIn();
extern int mapOut();
extern int expId();
extern int wError();
extern int wExp();
extern int wNt();
extern int wBs();
extern int FGcomplt();
extern int qError();
extern int qStatus();
extern int resetProcproc();
extern int terminate();
 
cmd table[] = { 
    {"chkQ"	, chkQueue, 	"Processing Q Entry has been added (via another process)" },
    {"expid"	, expId, 	"Active Exp. mappin ExpInfo Dbm" },
    {"werror"	, wError,	"Error Processing" },
    {"wexp"	, wExp,		"Experiment Complete Processing" },
    {"wnt"	, wNt,		"FID Complete Processing" },
    {"wbs"	, wBs,		"BlockSize Processing" },
    {"fgcmplt"	, FGcomplt, 	"Foreground Processing Complete Msge from Vnmr" },
    {"seterr"	, qError, 	"Queue an Acqusition Error" },
    {"status"	, qStatus, 	"Print Procproc Q status" },
    {"reset"	, resetProcproc,"Reset Procproc, Clear Qs, Abort BG processing" },
    {"term"	, terminate, 	"Terminate Sendproc" },
    {"debug"	, debugLevel, 	"Changed Debug Level" },
    {"mapin"	, mapIn, 	"Map in a Shared Memory Segment" },
    {"mapout"	, mapOut, 	"Map out a Shared Memory Segment" },
    {"?"	, listCmds, 	"List Known Commands" },
    {NULL	,  NULL, 	NULL    }
              };

/**************************************************************
*
*  sizeOfCmdTable - return size of Cmd Table 
*
* RETURNS:
* size of cmd table
*
*	Author Greg Brissey 10/5/94
*/
int sizeOfCmdTable()
{
   return(sizeof(table));
}

/**************************************************************
*
*  addrOfCmdTable - returns address of Cmd Table 
*
* RETURNS:
* address of cmd table
*
*	Author Greg Brissey 10/5/94
*/
cmd *addrOfCmdTable()
{
   return(table);
}

#ifdef __cplusplus
}
#endif

