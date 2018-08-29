/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCexpcmdsh
#define INCexpcmdsh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------------------------------
|       expcmds.h
|
|       This include file contains the names, addresses, and other
|       information about all commands.
+-------------------------------------------------------------------------*/
#include <stdio.h>

struct _cmd { char   *n;
              int   (*f)();
	      char   *d;
            };
typedef struct _cmd cmd;

/* commands MUST be alphabetized */
extern int abortCodes();
extern int debugLevel();
extern int listCmds();
extern int mapIn();
extern int mapOut();
extern int strtExp();
extern int terminate();
 
cmd table[] = { 
    {"exp"	, strtExp, 	"Start Experiment" },
    {"term"	, terminate, 	"Terminate Sendproc" },
    {"debug"	, debugLevel, 	"Changed Debug Level" },
    {"mapin"	, mapIn, 	"Map in a Shared Memory Segment" },
    {"mapout"	, mapOut, 	"Map out a Shared Memory Segment" },
    {"?"	, listCmds, 	"List Known Commands" },
    {NULL	,  NULL, 	NULL    }
              };

#ifdef __cplusplus
}
#endif

#endif /* INCexpcmdsh */
