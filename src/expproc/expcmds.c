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

/* commands MUST be alphabetized */
extern int abortCodes();
extern int acqAbort();
extern int acqDebug();
extern int acqHalt();
extern int acqHalt2();
extern int acqHwSet();
extern int acqStop();
extern int atCmd();
extern int call_aupdt();
extern int call_jupdt();
extern int chkExpQ();
extern int downloadDSP();
extern int dwnldComplete();
extern int ExpAcqDone();
extern int vnmrProcAck();
extern int debugLevel();
extern int getExpInfoFile();
extern int getInteract();
extern int getLockS();
extern int getStatBlock();
extern int ipctst();
extern int ipccontst();
extern int listCmds();
extern int mapIn();
extern int mapOut();
extern int parmChg();
extern int queryStatus();
extern int rebootConsole();
extern int setStatBlockIntv();
extern int reserveConsole();
extern int releaseConsole();
extern int relayCmd();
extern int startInteract();
extern int stopInteract();
extern int strtExp();
extern int terminate();
extern int transparent();
extern int qQuery();
extern int wallUsers();
extern int acqRobotCmdAck();
extern int autoMode();
extern int normalMode();
extern int autoResume();
extern int autoOk2Die();
extern int queueSample();
extern int roboClear();
extern int robotMessage();
extern int acqDequeue();
extern int autoqMsg();
extern int add2QHead();
extern int add2QTail();
#ifdef NIRVANA
extern int rebooted();
#else
extern int masSpeed(); /* this is in conhandler.c */
extern int masPars();  /* this is in conhandler.c */
#endif

 
cmd table[] = { 
    {"atcmd"	, atCmd, 	"Check for atcmds" },
    {"auto"	, autoMode, 	"Change to Automation Mode of Operation, start Autoproc" },
    {"normal"	, normalMode, 	"Change to Normal Mode of Operation" },
    {"autoRq2Die", autoOk2Die, 	"Autoproc request to terminate itself" },
    {"chkExpQ"	, chkExpQ, 	"Check Experiment Queue & Start Experiment" },
    {"exp"	, strtExp, 	"Start Experiment" },
    {"expdone"	, ExpAcqDone, 	"Experiment is Finished Acquiring" },
    {"fgcmplt"	, vnmrProcAck, 	"Vnmr Acknowledge of Procproc processing request" },
    {"getLockS" , getLockS,	"Get Lock Signal" },
    {"ipctst"	, ipctst, 	"Msge Loopback Vnmr->Expproc->Vnmr" },
    {"ipccontst", ipccontst, 	"Msge Loopback Vnmr->Expproc->console->Expproc->Vnmr" },
    {"term"	, terminate, 	"Terminate Sendproc" },
    {"debug"	, debugLevel, 	"Changed Debug Level" },
    {"mapin"	, mapIn, 	"Map in a Shared Memory Segment" },
    {"mapout"	, mapOut, 	"Map out a Shared Memory Segment" },
    {"parmchg"	, parmChg, 	"Set processing mask" },
    {"relay"	, relayCmd, 	"Relay command to the named proc" },
    {"resume"	, autoResume, 	"Send resume command to Autoproc, start automation" },
    {"roboack"	, acqRobotCmdAck,"Sample Changer Command Acknowledge for Console" },
    {"sethw"	, acqHwSet,	"Set values for Hardware in the Console" },
    {"halt"	, acqHalt,      "Halt acquisition, with wexp processing" },
    {"halt2"	, acqHalt2,     "Halt acquisition, with no processing" },
    {"abort"	, acqAbort,	"Abort acquisition, with error processing" },
    {"stop"	, acqStop,	"stop acquisition, as directed" },
    {"acqdebug"	, acqDebug,	"Set Debugging Level 0-9:host 10-19:console" },
    {"superabort", rebootConsole, "reboot digital console" },
    {"statintv" , setStatBlockIntv,
			"set rate (interval) at which console sends status blocks" },
    {"reserveConsole", reserveConsole,
			"reserve console for exclusive interactive use" },
    {"releaseConsole", releaseConsole,
			"release console from exclusive interactive use" },
    {"startInteract",  startInteract, "start an interactive experiment" },
    {"getInteract",    getInteract,   "get current interactive data" },
    {"stopInteract",   stopInteract,  "stop an interactive experiment" },
    {"getStatBlock",   getStatBlock,  "get stat block from console immediately" },
    {"queuequery",   qQuery,  "get list of experiments" },
    {"wall"	, wallUsers,  "Write all Users (unix wall)" },
    {"aupdt"	, call_aupdt,	 "Set values for Real-Time Variables in the Console" },
    {"jupdt"	, call_jupdt,	 "Set values for Real-Time Vars/Tables in the Console" },
    {"transparent", transparent, "Send an arbitrary command to the console" },
    {"downloadDSP", downloadDSP, "Download data over the VMS bus to the DSP board" },
    {"dwnldComplete", dwnldComplete,
			"Sendproc sends this when it completes a VME download" },
    {"getExpInfoFile", getExpInfoFile,
			"return name of current experiment information file" },
    {"queryStatus", queryStatus, "Get current status" },
    {"queueSample", queueSample, "Queue sample with autochanger scheduler" },
    {"roboclear", roboClear, "Tell roboproc to clear sample changer" },
    {"robot", robotMessage, "Send message to roboproc" },
    {"?"	, listCmds, 	"List Known Commands" },
    {"autoqmsg" , autoqMsg,  "send message to registered listeners" },
    {"dequeue"  , acqDequeue,	"Dequeue acquisition, as directed" },
    {"add2Qhead", add2QHead, "Add to Experiment Queue's Head" },
    {"add2Qtail", add2QTail, "Add to Experiment Queue's Tail" },
#ifdef NIRVANA
    {"rebooted", rebooted, "Console rebooted. Download shims, etc" },
#else
    {"masspeed"	, masSpeed, 	"MAS speed change" },
    {"maspars"	, masPars, 	"MAS parameter change" },
#endif
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
int sizeOfCmdTable(void)
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
