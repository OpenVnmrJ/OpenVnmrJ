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

/*-------------------------------------------------------------------------
|       expcmds.h
|
|       This include file contains the names, addresses, and other
|       information about all commands.
+-------------------------------------------------------------------------*/

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* commands MUST be alphabetized */
extern int debugLevel();
extern int listCmds();
extern int Resume();
extern int Ignore();
extern int Listen();
extern int markDoneQcmplt();
extern int resetAutoproc();
extern int terminate();
extern int Ok2Die();
extern int SampleId();
 
cmd table[] = { 
    {"cmplt"	, markDoneQcmplt, 	"Resume, start next Exp in Queue" },
    {"ignore"	, Ignore, 	"Ignore Resume" },
    {"listen"	, Listen, 	"Listen to Resume" },
    {"resume"	, Resume, 	"Resume, start next Exp in Queue" },
    {"reset"	, resetAutoproc,"Reset Autoproc, Clear Qs, Abort BG processing" },
    {"term"	, terminate, 	"Terminate Nautoproc" },
    {"Ok2Die"	, Ok2Die, 	"Expproc's Ok to Terminate Nautoproc" },
    {"sampleid" , SampleId,     "Save sample id/barcode for current sample" },
    {"debug"	, debugLevel, 	"Changed Debug Level" },
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

