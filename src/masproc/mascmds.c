/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* robocmds.c 12.1 03/21/08 - Table of commands & function calls */
/* 
 */

#include <stdio.h>
#include "parser.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Commands MUST be alphabetized */
extern int debugLevel();
extern int listCmds();
extern int terminate();
extern int roboReady();
extern int userCmd();
extern int initMas();
 
cmd table[] = { 
    {"term"     , terminate,    "Terminate Sendproc" },
    {"debug"    , debugLevel,   "Changed Debug Level" },
    {"roboready", roboReady,    "Initializing message queues from Expproc" },
    {"initMas",   initMas,      "Initializing Mas from Expproc" },
    {"userCmd"  , userCmd,      "Command from sethw" },
    {"?"        , listCmds,     "List Known Commands" },
    {NULL       , NULL,         NULL }
};

/************************************************************************
 *
 * sizeOfCmdTable - return size of Cmd Table 
 *
 * RETURNS:
 * size of cmd table
 *
 * Author Greg Brissey 10/5/94
 *
 ************************************************************************/
int sizeOfCmdTable(void)
{
    return(sizeof(table));
}

/************************************************************************
 *
 * addrOfCmdTable - returns address of Cmd Table 
 *
 * RETURNS:
 * address of cmd table
 *
 * Author Greg Brissey 10/5/94
 *
 ************************************************************************/
cmd *addrOfCmdTable(void)
{
    return(table);
}

#ifdef __cplusplus
}
#endif
