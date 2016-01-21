/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
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
*/

/* IST VxWorks Task Priority & Stack Size */
#define FIFO_MT_IST_PRIORTY 1
#define FIFO_FULL_IST_PRIORTY 1
#define FIFO_STOP_IST_PRIORTY 1
#define FIFO_STRT_ERR_IST_PRIORTY 1
#define FIFO_PRGITR_IST_PRIORTY 1
#define FIFO_IST_STACK_SIZE 2048
#define FIFO_IST_TASK_OPTIONS 0

/* IST VxWorks Task Priority & Stack Size */
#define STM_DATRDY_IST_PRIORTY 1
#define STM_MAXTRN_IST_PRIORTY 1
#define STM_DATERR_IST_PRIORTY 1
#define STM_IST_STACK_SIZE 2048
#define STM_IST_TASK_OPTIONS 0

#define SIB_IST_PRIORITY 20

/* -----------  Console Tasks ------------ */

/*
   tPHandler: PENDED on its message queue.
    The phandler must have higher priority han the stuffer so that it 
    can preempt the stuffer and all other application so it can 
    perform its function immediately.

   tFFStuffer: PENDED on Semaphores taken and given by fifo interrupt routines 
	       & its Blocking ring buffer (fifo  words).
    The Fifo stuffer Is The next Highest, thus when ever the FIFO needs 
    stuffing it will be serviced immediately.

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

   conBroker: PENDED on the system select() call
    The conBroker handles the channel connection requests between the 
    host and console.  This typically happpens during the initial 
    booting sequence and is inactive for the mojority of time.

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
#define STD_STACKSIZE 3000
#define CON_STACKSIZE 4000
#define XSTD_STACKSIZE 5000
#define MED_STACKSIZE 10000
#define BIG_STACKSIZE 20000
#define STD_TASKOPTIONS 0

#define PHANDLER_PRIORITY       5        /* was 9 */
#define FIFO_STUFFER_PRIORITY  10        /* Only the Phandler Task should be */
				         /* of a higher priority than the stuffer */
#define MONITOR_TASK_PRIORITY	      60 /* 51 */
#define HOSTAGENT_TASK_PRIORITY	      65 /* 51 */
#define STATAGENT_TASK_PRIORITY	      70 /* 52 */
/* #define STATMON_TASK_PRIORITY	      53  changed to 203, see below */
#define CONBROKER_TASK_PRIORITY	      80 /* 100 */
#define DOWNLINKER_TASK_PRIORITY      90 /* 198  */
/* Dwnldr should it be < or > parser ? */
/* > parser then all dwnlding will happen prior
     to parsing
     < parser then dnwldr will not finish and parser
     will start when it pends for more acodes then
     then downldr will be able to run
     Maybe this is a canidate for a dynamic changed
     priority. Initially higher than parser to get
     a minimum set of codes down then sets its
     priority lower to allow parser to start up..
 */
#define SHANDLER_PRIORITY             100	/* 199 */
#define APARSER_TASK_PRIORITY	      110	/* 200 */
#define XPARSER_TASK_PRIORITY	      120	/* 200 */
#define UPLINKER_TASK_PRIORITY	      130	/* 201 */
#define UPLINKERI_TASK_PRIORITY	      140	/* 201 */
#define AUPDT_TASK_PRIORITY           150	/* 201 */
#define STATMON_TASK_PRIORITY	      160	/* 203 */
#define VT_TASK_PRIORITY	      170	/* 205 */
#define TUNE_TASK_PRIORITY            210

#ifdef __cplusplus
}
#endif

#endif
