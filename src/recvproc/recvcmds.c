/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* recvcmds.c - Table of commands & function calls */
/* 
 */

#include <stdio.h>
#include "parser.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern int abortCodes();
extern int debugLevel();
extern int listCmds();
extern int mapIn();
extern int mapOut();
extern int recvData();
extern int recvInteract();
extern int resetState();
extern int startInteract();
extern int stopInteract();
extern int terminate();
 
cmd table[] = { 
    {"recv"	, recvData, 	"Receive Data" },
    {"reset"	, resetState, 	"Reset Recvproc State " },
    {"term"	, terminate, 	"Terminate Sendproc" },
    {"debug"	, debugLevel, 	"Changed Debug Level" },
    {"mapin"	, mapIn, 	"Map in a Shared Memory Segment" },
    {"mapout"	, mapOut, 	"Map out a Shared Memory Segment" },
    {"startI"   , startInteract,"Prepare to Receive Interactive Data" },
    {"recvI"    , recvInteract, "Receive Single Element of Interactive Data" },
    {"stopI"    , stopInteract, "Stop Receiving Interactive Data" },
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
sizeOfCmdTable()
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

