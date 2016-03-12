/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#ifndef INCtaskPriorh
#define INCtaskPriorh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*
DISCRIPTION

 Task priorities and associated parameters 


 changed priorities of task but maintained relative order
 except those task of equal priority now have different ones.
 Speading out priorites.

  10/16/01  GMB

  change for Nirvana   1/14/2005  GMB
*/

/* -----------  Inova Console Tasks ------------ */

/*
   tPHandler: PENDED on its message queue.
    The phandler must have higher priority han the stuffer so that it 
    can preempt the stuffer and all other application so it can 
    perform its function immediately.

   tMonitor: PENDED on read of the Expproc Channel
    The Monitor is next in priority so that commands for the Expproc are 
    dealt with in a timely manner.
   
   tHostAgent: PENDED on its message queue
    The Host Agent is next to insure prompt deliever of messages from the 
    console to Expproc.

   tStatAgent: PENDED on Semaphore which it times out on giving its 
	 	periodic nature.
    The StatAgent sends periodic stat-block to Expproc, which are 
    displayed via Acqstat on the Host computer.

   tStatMon:  Suspended via taskDelay(some_interval) giving its 
	      periodic nature.
    The Statmon inquires on status of various perpherals that might 
    take a long to respond or might pend this task. 
    (e.g. VT, Spinspeed, locklevel, etc..) These values are placed into 
    the stat-block that Statagent sends to Expproc.

   tDownLnker: PENDED on read of Sendproc channel
    The DownLinker receives the RT_parms, Xcodees, Acodes, Tables from 
    the host and places them in appropiate buffers on the console. 

   tSHandler: PENDED on its message queue.
    The shandler receives infomation from the FIFO TAG ISR, the tag
    value indicate some synchronise operation that must be 
    performed. One set of actions are the GO or Setup complete message
    back to HOST.  The other style of actions are those that need to be
    synchronise within the experiment e.g. set VT, set spinner, autolock,
    autoshim, etc.

    
   Aparser: PENDED on its message queue.
    The acode parser receives infomation from the monitor about an 
    experiment, attempts to find the appropiate buffers and begins parsing. 
    Parsing result in FIFO Words being placed into the fifo buffer which 
    starts the fifo stuffer to stuff the FIFO words. When the
    fifo is almost full the FIFO is automatically started.

   Xparser: PENDED on its message queue.
   The xcode parser receives commands via its message queue, typical 
   commands involve hardware setting, updating parameters and/or acodes, etc..

   
   Uplinker: PENDED on its Message Queue.
    The upLinker receives data interrupt message via STM, data ready or 
    data error.  The interrupt message indicates the data to be sent to 
    the Host. UpLinker writes the data statblock and then the Data to 
    Recvproc channel.

   UplinkerI: PENDED on its Message Queue.
    The Interactive upLinker receives data interrupt message via STM, 
    The interrupt message indicates the data to be sent to the Host. upLinker
    writes the Data to Recvproc? channel.

   tAupdt: PENDED on its message queue
    The Update task handles the updating of hardware,etc...

*/
#define STD_STACKSIZE 4096
#define CON_STACKSIZE 4096
#define XSTD_STACKSIZE 8192
#define MED_STACKSIZE 10000
#define BIG_STACKSIZE 20000
#define STD_TASKOPTIONS 0

/* 
Priorities of Nirvana running system  7/7/2006

  NAME        PRI   
----------    --- 
tExcTask       0 
t_endDMA       1    DMA interrupt task (FIFO Words into FIFO, FID Data transfer from DSP)
tFifoIST       1    FIFO Interrupt task (errors, SW_itrs, sync)
tShell         1 
tRlogind       2 
---------------------   present only if actualy rlogin to controller 
tRlogOutTas    2 
tRlogInTask    2 
---------------------
tWdbTask       3 
tMsgLogd       4     DPRINT, errLog(), etc. printing task
tPHandlr       5     Error Handler
tLogTask       6     VxWorks logMsg() Printing task
tDMsgIST       10   -- DDR Only,   Interrupt task for TI DSP Msgs
---------------------
tSHandlr       40    Only during MRIUSERBYTE 
tParser        45    Only during MRIUSERBYTE
---------------------
tNetTask       50    Incoming ethernet packet Task
---------------------
tParser        52    DDR Acode Parser Task
---------------------
tPortmapd      54    RPC for NFS mounting, etc.
tDhcpcState    56    DHCP
tDhcpcReadT    56    DHCP
n_at7400       60    NDDS Alarm Task
n_rtu7400      70    NDDS Unicast Receive Task
n_rtm7400      70    NDDS Multicast Receive Task
tMonitor       80  --  Master Only,  recieves Expproc cmds
tFFSUpdate     85  --   Only present when update the Flash File System 
tDMsgTSK       90  -- DDR Only, interpretes DPS Msgs
tSHandlr       100   Handles, Sync (via FIFO SW Itrs) operations
tPneuFault     100 -- Master Only, Pneumatic Fault reporter
tWvRBuffMgr    100  -- present only if collecting Windview log
tParser        110   Acode Parser Task
tXParser       120  -- Master Only,  The X Paser, interactive lock, shim setting, VnmrJ sethw commands.
tDspXfr        120  -- DDR Only,  Task to Queue DMA Data transfer from DSP Memory
tGradParser    120  -- Grad Only
tFidPubshr     135  -- DDR Only, Transfers FID Data to Recvproc
tAppHBPub      160   Controller HeartBeat Publication
tStatPub       190  -- Master Only, Exp. Status Publication to Infoproc
tSPNspeed      200  -- Master Only, Controls Spinner speed
tFidCtPub      200  -- DDR Only,  Publishes to Master the present CT or FID acquired
blafTask       202  -- Master Only, Background Shimming of sorts
tTune          210   Nucleus Tuning
n_dt7400       250   NDDS database and Request Ack for publish issue control
n_mgr7400      251   NDDS manager
tPanelLeds     255   the Cylon and Error Front panel LEDs Pattern  Task

*/

/* FIFO Iinterrupt service task, handles Error reporting */
#define FIFO_IST_PRIORTY         1
#define FIFO_IST_TASK_OPTIONS    0
#define FIFO_IST_STACK_SIZE   2048

#define END_DMA_TASK_PRIORITY   1
#define MSGLOG_TASK_PRIORITY    4
#define PHANDLER_PRIORITY       5        /* was 9 */
#define LOGMSG_TASK_PRIORITY    6
#define FIFO_STUFFER_PRIORITY  10        /* Only the Phandler Task should be */
				         /* of a higher priority than the stuffer */
#define DSPMSG_IST_PRIORITY    10

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* DANGER!!!!!  DO NOT alter order of priority of NDDS Tasks !!!! */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#define NDDS_ALARM_TASK_PRIORITY      60 /* n_at7400, must below tNetTask task */
#define NDDS_RECEIVER_TASK_PRIORITY   70 /* n_rtu7400 & n_rtm7400, Unicast  & Multicast Reciever tasks */
#define NDDS_SENDER_TASK_PRIORITY     75 /* n_st7400, Publisher Task */
#define NDDS_DBM_TASK_PRIORITY        250 /* n_dt7400, Database Task */
#define NDDS_MANAGER_TASK_PRIORITY    251 /* n_mgr7400, Manager task */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#define MONITOR_TASK_PRIORITY	      80 /* Master Monitor, comm link to Expproc */

#define FFSUPDATE_TASK_PRIORITY       85  /* Flash Update Task */

#define BUFREQ_TASK_PRIORITY          90
#define DOWNLINKER_TASK_PRIORITY       90 /* Handles queues from Sendproc about buffers for downlaoding, etc.  */

#define DSPMSG_TASK_PRIORITY           90

#define SHANDLER_PRIORITY             100	/* 199 */
#define MRIUSERBYTE_SHANDLER_PRIORITY  40

#define PNEUFAULT_PRIORITY            100

#define APARSER_TASK_PRIORITY	      110	/* 200 */
#define DDR_APARSER_TASK_PRIORITY      52
#define MRIUSERBYTE_APARSER_PRIORITY   45	

#define XPARSER_TASK_PRIORITY	      120	/* 200 */
#define LKPARSER_TASK_PRIORITY	      120	/* 200 */
#define GRADPARSER_TASK_PRIORITY      120	/* 200 */
#define ALOCK_TASK_PRIORITY	      130	/* 201 */
#define UPLINKER_TASK_PRIORITY	      130	/* Data Publisher, data to Recvproc  */
#define UPLINKERI_TASK_PRIORITY	      140	/* 201 */
#define AUPDT_TASK_PRIORITY           150	/* 201 */
#define HEARTBEAT_TASK_PRIORITY	      160	/* NDDS Heartbeat publication task */
#define STATMON_TASK_PRIORITY	      190	/* 203 */
#define VT_TASK_PRIORITY	      200	/* 205 */
#define SPIN_TASK_PRIORITY	      200
#define MAS_TASK_PRIORITY	      200
#define BLAF_TASK_PRIORITY	      202
#define TUNE_TASK_PRIORITY            210
#define PANEL_LED_TASK_PRIORITY       255

#ifdef __cplusplus
}
#endif

#endif
