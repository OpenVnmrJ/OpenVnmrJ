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

/* Commands MUST be alphabetized */
extern int debugLevel();
extern int listCmds();
extern int mapIn();
extern int mapOut();
extern int resetRoboproc();
extern int terminate();
extern int getSample();
extern int putSample();
extern int reputSample();
extern int failPutSample();
extern int queueSample();
extern int roboClear();
extern int roboReady();
extern int userCmd();
extern int check_timers();
 
cmd table[] = { 
    {"getsmp"   , getSample,    "Get Sample from Magnet" },
    {"putsmp"   , putSample,    "Put Sample into Magnet" },
    {"reputsmp" , reputSample,  "Retry put Sample" },
    {"failputsmp" , failPutSample, "Put Sample failed" },
    {"queuesmp" , queueSample,  "Schedule Sample for NMR" },
    {"roboclear", roboClear,    "Clear all samples from sample changer" },
    {"reset"    , resetRoboproc,"Reset Roboproc, Clear Qs,"
                                " Abort BG processing" },
    {"term"     , terminate,    "Terminate Sendproc" },
    {"debug"    , debugLevel,   "Changed Debug Level" },
    {"chktimers", check_timers, "Internal msg: check for scheduler events" },
    {"mapin"    , mapIn,        "Map in a Shared Memory Segment" },
    {"mapout"   , mapOut,       "Map out a Shared Memory Segment" },
    {"roboready", roboReady,    "Initializing command from Expproc" },
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
