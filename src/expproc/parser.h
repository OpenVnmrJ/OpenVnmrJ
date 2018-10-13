/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCparserh
#define INCparserh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------------------------------
|       parser.h
|
|       This include file contains the structure of  parser commands
|	and the 2 functions that each seperate user of parser must provide
|  E.G.
|
|cmd table[] = { 
|    {"exp"	, strtExp, 	"Start Experiment" },
|    {"term"	, terminate, 	"Terminate Sendproc" },
|    {"debug"	, debugLevel, 	"Changed Debug Level" },
|    {"mapin"	, mapIn, 	"Map in a Shared Memory Segment" },
|    {"mapout"	, mapOut, 	"Map out a Shared Memory Segment" },
|    {"?"	, listCmds, 	"List Known Commands" },
|    {NULL	,  NULL, 	NULL    }
|              };
|
+-------------------------------------------------------------------------*/

struct _cmd { char   *n;
              int   (*f)();
	      char   *d;
            };
typedef struct _cmd cmd;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int sizeOfCmdTable(void);
extern cmd *addrOfCmdTable(void);

/* --------- NON-ANSI/C++ prototypes ------------  */

#else

extern int sizeOfCmdTable();
extern cmd *addrOfCmdTable();
 
#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* INCparserh */
