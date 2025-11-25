/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* sendcmds.c - Table of commands & function calls */
#ifndef LINT
#endif
/* 
 */

#include <stdio.h>
#include "parser.h"


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* commands MUST be alphabetized */
extern int abortCodes();
extern int debugLevel();
extern int listCmds();
extern int mapIn();
extern int mapOut();
extern int sendCmd();
extern int sendCodes();
extern int sendTables();	
extern int sendDelTables();	
extern int sendVME();
extern int terminate();
 
cmd table[] = { 
    {"?"	, listCmds, 	"List Known Commands" },
    {"command"	, sendCmd, 	"Pass a command to another parser" },
    {"debug"	, debugLevel, 	"Changed Debug Level" },
    {"mapin"	, mapIn, 	"Map in a Shared Memory Segment" },
    {"mapout"	, mapOut, 	"Map out a Shared Memory Segment" },
    {"send"	, sendCodes, 	"Download acodes" },
    {"tables"	, sendTables, 	"Download named buffers" },
    {"tabledel"	, sendDelTables,  "Download and delete named buffers" },
    {"term"	, terminate, 	"Terminate Sendproc" },
    {"vme"	, sendVME,	"send stuff to a location on the VME" },
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
   return((int) sizeof(table));
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

