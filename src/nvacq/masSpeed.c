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

#ifdef MASPROC

#include "masSpeedWrap.h"
#include "errLogLib.h"
#define TRUE 1
#define FALSE 0
#define ERROR -1
MSG_Q_ID msgToMASSpeedCtl = 0;

#else
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>
#include "ioLib.h"

#include "taskPriority.h"
#include "logMsgLib.h"
#include "Console_Stat.h"
#include "acqcmds.h"
#include "spinner.h"
#include "spinObj.h"



/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#define MASPORTNAME "/TyMaster/3"
MSG_Q_ID msgToMASSpeedCtl = NULL;
extern SPIN_ID pTheSpinObject;

#endif


/* Delay to use as msgQ timeout so that we sent the current speed
   to vnmrj on each timeout.  
*/
#define CYCLE_DELAY 3  // in seconds
#define MAX_MSGS 20
#define MAX_MSG_LEN 64

#define SpinLED_STOP 0	/* Console Display LED OFF */
#define SpinLED_BLINKshort 1	/* Console Display LED BLINK ON short */
#define SpinLED_BLINKlong 2	/* Console Display LED BLINK ON long */
#define SpinLED_OK 3	/* Console Display LED ON "Regulating" */

void MASSpeed();
void sendToMASSpeed(char *);
void checkModuleChange();
void getAllParams();
void getAllParamsCmd();
static void checkSpinLEDMas ();
int getMASSpeedResponseString(char *result);
int getMASSpeedResponseInt(int *result);
int ckMASSpeedInternal();


extern void msgQMasSend(int id, char *msg, int len, int a1, int a2);
extern Console_Stat *pCurrentStatBlock;


int masSpeedTaskId = 0;
int masSpeedPort = 0;
float clkRate;      // Created as float for calculations
char module[32]="EMPTY";
int  setPoint=-1;
int  exitMas=FALSE;
// Init curMasSpeed to -1 so first time through, it will show a change
// from the new curMasSpeed to cause LED to be set.
int  curMasSpeed=-1;
int  prevMasSpeed=-1;
int  MasSpeedTaskSleeping=FALSE;

/*************************************************************************
   Utility Commands
      exitMASSpeed - Sets a variable that will cause the task to exit at
                     a safe time.
      initMASSpeed - Start or restart the MASSSpeed task.  If this is
                     executed while the task is running, it will simply
                     get all of the parameters from the controller to be
                     sure things are up to date.
      getModuleID - Gets the Probe ID from the controller and sends it
                     up the chain.
      getAllParamsCmd - Gets all parameters from the controller and sends them
                     up the chain.
      putMASSpeedToSleep - Puts the task into a sleep state.  Every 3 seconds,
                     it checks to see if it is supposed to stay asleep.
      wakeUpMASSpeed - Takes the task out of the sleep state.  At that point,
                     it gets all parameters from the controller to get
                     everything up to date.
      ckMASSpeedInternal -   Checks in with the controller to see if it is still
                     available.  Set DebugLevel=2 to see the text results.
      ckMASSpeed -   Check controller status and output with DebugLevel = 0
                     This is for manually checking the controller.

*************************************************************************/


/* VxWorks cmd to exit the task.  It will exit after finishing the command
   it may currently be involved with, when no communications is going on. */
void exitMASSpeed() {
    exitMas = TRUE;
}


#ifndef MASPROC

/* Setup and Start the task */
void startMASSpeed(int priority, int taskOptions, int stackSize)
{
    int  status;
    int msgQMaxNum=20;    /* max number of msg in queue */
    int msgQMaxSize=256;  /* char per msg */

    exitMas = FALSE;

    // Get the rate of the clock for setting absolute times on delays
    // and timeouts
    clkRate = (float)sysClkRateGet();

    /* If the task is already running, Report this fact, and getAllParams 
       then just return. */
    if(masSpeedTaskId != 0) {
        /* Pause the task so we don't get into trouble sending concurrent
           commands to the controller by calling getAllParamsCmd().
        */
        MasSpeedTaskSleeping = TRUE;
        // Give the running task time to complete anything it might be doing
        // and go to sleep
        // taskDelay((int)(clkRate * 0.2));
        taskDelay(calcSysClkTicks(200)); /* 200 ms, or 60 * .2 = 12 */

        DPRINT(-1, "Mas Speed Task is Already Running\n");

        /* Forward the params update through the existing task */
        getAllParamsCmd();

        MasSpeedTaskSleeping = FALSE;
        return;
    }

    /* Open the serial port to the CM Speed Controller */
    masSpeedPort = open(MASPORTNAME, O_RDWR, 0);
    DPRINT1(0,"Opened MAS Port = %d\n", masSpeedPort);
    /* Check to see if the CM speed controller is up and running and
       able to respond properly.  If not, bail out. */
    status = ckMASSpeedInternal();
    if(status < 1) {
        errLogRet(LOGIT,debugInfo, "*** MAS Controller Not Ready, Aborting\n");
        return;   /* Bail Out */
    }
    DPRINT      (0, "*** MAS Controller Found\n");

    // This is a flag telling everyone what  controller is available
//    spinnerType = MAS_SPINNER;
    
    if (pTheSpinObject != NULL)
       pTheSpinObject->SPNtype = MAS_SPINNER;


    hsspi(2,0x1000000);  // Clear this for MAS, it is used for liq and tach

    /* Spawn the task that will keep running to monitor requests from
       vnmrj, and to routinely send the current speed back to vnmrj.
    */
    masSpeedTaskId = taskSpawn("MASSpeed",priority, taskOptions, stackSize,
                        (void *) MASSpeed,0,0,0,0,0,0,0,0,0,0);

    if (masSpeedTaskId == ERROR) {
        perror("taskSpawn");
    }
}


/* Non argument entry point */
void initMASSpeed ()
{
    startMASSpeed(MAS_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
}
#endif

void processMasMessage(char *msgBuf)
{
   char strRes[80];
   int len;
   int status;
   int response;

   /* For testing.  when a msg is sent to the msgQ manually, it
      does not have the \n, so add one. */
   len = strlen(msgBuf);
   if(msgBuf[len-1] != '\n') {
      strcat(msgBuf, "\n");
   }

   DPRINT1(1, "Message received for MAS Controller: %s\n", msgBuf);

   // The msg should start with "SET" or "GET".  The real difference
   // between these two, is that "SET" does not expect a return
   // value, and "GET" does expect a return.  The command
   // GET BEARING is special in that it needs to return 3 values.
   // It is the only cmd to return the name of a value followed
   // by an equal sign and then a value.  The general form of the
   // incoming message here should be "GET/SET CMD [value]"
   
   // Check first 3 char of msg for GET/SET
   if(strncmp(msgBuf, "SET", 3) == 0)
   {
      /* The controller return from all set commands should simply
         be the controller prompt of "\n>".
         Cmd strings received here (msgBuf) should be SET followed
         by the controller cmd string such as ACTSP which is followed
         by the value or values for that cmd.  It should also
         already have a \n at the end of the string so that it
         is ready to send to the controller after removing the
         leading SET in the string.  The msgBuf[4] is taking the
         string starting after the SET.
      */
      sendToMASSpeed(&msgBuf[4]);
      // The controller should just return its prompt.
      status = getMASSpeedResponseString(strRes);

      if(status != 1  ||  strRes[1] != '>')
      {
         if(strncmp(strRes, "SPEED LIMIT", 11) == 0)
         {
            errLogRet(LOGIT,debugInfo, "Trying to set speed over speed limit\n");
         }
         else
         {
            errLogRet(LOGIT,debugInfo, "Problem with controller sending cmd: %s\n    %s\n", 
                               &msgBuf[4], strRes);
         // Try clearing out controller and hope for better
         // luck next time.
            ckMASSpeedInternal();
         }
      }
      else
      {
         if(strncmp(msgBuf, "SET S", 5) == 0)
         {
            // If we set a new speed, ask the controller for
            // its current value to be sure it know what we ask for.
            sendToMASSpeed("SETP\n");
            status = getMASSpeedResponseInt(&response);
            if(status == 1)
            {
               // Set the current value into the NDDS structure
               pCurrentStatBlock->AcqSpinSet = response;
               sendConsoleStatus();

               // Save cur set point for test now and then
               setPoint = response;
            }
         }
      }
   }
   else if(strncmp(msgBuf, "GET", 3) == 0)
   {
      /* [4] means look at string after the "GET " in the command. */
      if(strncmp(&msgBuf[4], "BEARING", 7) == 0)
      {
         /* BEARING command returns results in the form:
            SPAN = 100
            ADJ = 6
            MAX = 26\n>
         */
         sendToMASSpeed("BEARING\n");
         status = getMASSpeedResponseString(strRes);
         if(status != 1) {
            errLogRet(LOGIT,debugInfo, "Problem with Bearing command\n   %s\n", strRes);
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }
      }
      /* MODULE  returns a text string */
      else if(strncmp(&msgBuf[4], "MODULE", 6) == 0) {
         /* The module ID will not be sent unless it has changed.
            set to empty and then to the received value.
         */
         pCurrentStatBlock->probeId1[0] = '\0';
         sendConsoleStatus();
         taskDelay(calcSysClkTicks(830)); /* 830 ms, or taskDelay(50); */

         sendToMASSpeed(&msgBuf[4]);
         status = getMASSpeedResponseString(strRes);
         if(status != 1) {
            errLogRet(LOGIT,debugInfo, "Problem with command: %s\n    %s\n", 
                               &msgBuf[4], strRes);
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }

         // Put the result in place to be sent up
         strcpy(pCurrentStatBlock->probeId1, strRes);
         // Tell it we have changed values
         sendConsoleStatus();

         // Save current module
         strcpy(module, strRes);
         DPRINT1(1, "MAS Probe Module: %s\n", module);
      }
      /* PROFILE returns a text string */
      else if(strncmp(&msgBuf[4], "PROFILE", 7) == 0) {
         sendToMASSpeed(&msgBuf[4]);
         status = getMASSpeedResponseString(strRes);
         if(status != 1) {
            errLogRet(LOGIT, debugInfo, 
                                  "Problem with command: %s\n    %s\n", 
                                  &msgBuf[4], strRes);

            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }

      }
      /* SETP returns the current set point value in Hz. Set with S */
      else if(strncmp(&msgBuf[4], "SETP", 4) == 0) {
         sendToMASSpeed(&msgBuf[4]);
         status = getMASSpeedResponseInt(&response);
         if(status == 1) {
            // Set the current value into the NDDS structure
            pCurrentStatBlock->AcqSpinSet = response;
            sendConsoleStatus();

            // Save cur set point for test now and then
            setPoint = response;
         }
         else {
            errLogRet(LOGIT,debugInfo, 
                                  "Problem with command: %s\n    %s\n", 
                                  &msgBuf[4], strRes);
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }
      }
      /* ACTSP returns 0, 1 or 2.  Just forward the value. 
         This is the active setpoint in case the user is wanting
         to change between set points without changing SETP */
      else if(strncmp(&msgBuf[4], "ACTSP", 5) == 0) {
         sendToMASSpeed(&msgBuf[4]);
         status = getMASSpeedResponseString(strRes);
         if(status != 1) {
            errLogRet(LOGIT,debugInfo, "Problem with command: %s\n    %s\n", 
                               &msgBuf[4], strRes);
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }

      }
      /* SPEED LIMIT returns the current value as
         'MAX SPEED = value' */
      else if(strncmp(&msgBuf[4], "SPEED LIMIT", 11) == 0) {
         sendToMASSpeed(&msgBuf[4]);
         status = getMASSpeedResponseString(strRes);
         if(status != 1) {
            errLogRet(LOGIT,debugInfo, "Problem with command: %s\n    %s\n", 
                               &msgBuf[4], strRes);
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }

      }
      /* S stands for speed.  This gets the current speed in Hz.
         This test must be after all other S commands so that we
         do not catch the first letter of another command. */
      else if(strncmp(&msgBuf[4], "S", 1) == 0) {
         sendToMASSpeed(&msgBuf[4]);
         status = getMASSpeedResponseString(strRes);
         if(status != 1) {
            errLogRet(LOGIT, debugInfo,
                                  "Problem with command: %s\n    %s\n", 
                                  &msgBuf[4], strRes);
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
         }

      }             
      else if(strncmp(&msgBuf[4], "ALLPARAMS", 9) == 0) {
         getAllParams();
      }
      else {
         // Error
         errLogRet(LOGIT, debugInfo,
                           "Unknown speed controller command %s.  Skipping.\n",
                           msgBuf);
      }

   }
   else
   {
      // Error
      errLogRet(LOGIT, debugInfo,
                       "Problem with speed controller command %s.  Skipping.\n",
                       msgBuf);
   }
}

/* Task to be spawned by startMASSpeed.  This task it to use a msgQ for
   a dual purpose.  #1 is to have waiting on the msgQ time out every few
   seconds so we can get the current speed and put it in the NDDS structure
   for sending to vnmrj.  #2 is that any request coming from vnmrj will
   be put into the msgQ and promptly dealt with.

   Thus, the same task sitting on a single msgQ will be waiting for 
   requests from vnmrj and basically looping to get the current speed
   and send back to vnmrj.
*/
void MASSpeed()
{
    int  response;
    int  status, mStatus;
    char msgBuf[MAX_MSG_LEN];
    long cycle=0;
    int  delay;

    DPRINT(1, "*** Starting MASSpeed in Sleep State\n");
    // Start up in sleep state.  It will be awaken when spintype is set
    MasSpeedTaskSleeping = TRUE;

    msgToMASSpeedCtl = msgQCreate(MAX_MSGS, MAX_MSG_LEN, MSG_Q_FIFO);
    checkModuleChange();

    // Get all param values and set into pCurrentStatBlock
    getAllParams();

    // convert CYCLE_DELAY in sec to clock ticks
    delay = clkRate * CYCLE_DELAY;
    while(1) {
        // Now and then, check to see if the probe module has changed.
        // If it has,  get all info and send back to vnmrj.
        if(!MasSpeedTaskSleeping) {
            if(cycle >= 20) {
                checkModuleChange();
                cycle = 0;
            }
            cycle++;
        }

        /* Either catch a message from vnmrj, or timeout and get the
           current speed and send it.
        */
        mStatus = msgQReceive(msgToMASSpeedCtl, msgBuf, MAX_MSG_LEN, delay);

        // If sleeping, just continue for another cycle of delay until
        // we are not sleeping.  Ignore all commands that come in.
        if(MasSpeedTaskSleeping)
            continue;

        /* Timeout of msgQReceive will return ERROR.  In our case,
           this simply means to get the speed and send it on. 
        */
        if(mStatus == ERROR) { 
            /* See if we need to exit this task */
            if(exitMas) {
                DPRINT(0, "Exiting MASSpeed Task\n");
                masSpeedTaskId = 0;
                close(masSpeedPort);
                return;
            }

            sendToMASSpeed("S\n");
            status = getMASSpeedResponseInt(&response);

            if(status == -1 && response == -2)
                errLogRet(LOGIT,debugInfo, 
                          "*** Trying to set speed higher than limit.\n");
            else if(status == -1)
                errLogRet(LOGIT,debugInfo, 
                        "*** Speed Controller had problem with 'S' command.\n");
            else {
                // Set the new response into the NDDS structure
                pCurrentStatBlock->AcqSpinAct = response;

                // Tell it we have changed a value
                sendConsoleStatus();

                DPRINT1(3, "MAS Current Speed = %d\n", response);

                // Save current and prev for setting LED
                prevMasSpeed = curMasSpeed;
                curMasSpeed = response;
                // Set the LED as appropriate
                checkSpinLEDMas ();

            }

        }
        /* If msgQReceive does not return ERROR, then we received a message
           which we need to process.
        */
        else {
           processMasMessage(msgBuf);
        }
    }
}


/* Write one char at a time with a delay between char.
   The controller has a 1 char input buffer.  We must not over run it.
   The spinsight/acc code notes that it was found that 80ms was safe.
   I have seen a couple of misses with 66ms.  Thus I am going with 120ms.
*/
void sendToMASSpeed(char *msg) {
    int i, msgSize;
    int ret __attribute__((unused));
    
    // Be sure nothing is waiting from or to the controller
    clearport(masSpeedPort);

    msgSize = strlen(msg);
    for(i=0; i < msgSize; i++) {
        ret = write(masSpeedPort, &msg[i], 1);
        taskDelay(calcSysClkTicks(117)); /* 117 ms, or 60 * .12 = 7.2, taskDelay(7); */
    }
}

/* Check to see if the CM speed controller is up and running and
   able to respond properly.  Return 1 if all is okay, return -1 for failure.
*/
int ckMASSpeedInternal()
{
    char  str[80];
    int  status;

    /* If there was a char in the controller already.  I will get
       either an error or perhaps a numeric response from the first \n.  
       I need to send a \n, then read and dump the response, then send 
       another \n and see if all I get is a '\n>' from the second one.
    */
    sendToMASSpeed("\n");
    status = getMASSpeedResponseString(str);
    sendToMASSpeed("\n");
    status = getMASSpeedResponseString(str);

    if(status == 1) {
        DPRINT(1, "MAS Controller is Okay\n");
    }
    else {
        DPRINT(1, "MAS Controller is Not Available\n");
    }
    
    return status;

}

/* Check to see if the CM speed controller is up and running and
   able to respond properly. This is for manual use.  Type this command
   into the master1 login shell.
*/
int ckMASSpeed()
{
    char  str[80];
    int  status;

    /* If there was a char in the controller already.  I will get
       either an error or perhaps a numeric response from the first \n.  
       I need to send a \n, then read and dump the response, then send 
       another \n and see if all I get is a '\n>' from the second one.
    */
    sendToMASSpeed("\n");
    status = getMASSpeedResponseString(str);
    sendToMASSpeed("\n");
    status = getMASSpeedResponseString(str);

    if(status == 1 && MasSpeedTaskSleeping) {
        DPRINT(-1, "MAS Controller is Okay and Task is Sleeping\n");
    }
    else if(status == 1 && !MasSpeedTaskSleeping) {
        DPRINT(-1, "MAS Controller is Okay and Task is Awake\n");
    }
    else {
        DPRINT(-1, "MAS Controller is Not Responding\n");
    }
    
    return status;

}


/* Read one character from the controller.  Wait a clock tick if nothing is
   there yet.  Timeout if nothing is available before timeout.
   Return -1 for timeout.
*/
int readCharWithTimeout(char *chr) 
{
    int i;
    int nBytesUnread;

    // Delay one clock tick each time we check to see if anything is
    // ready to read.  After 1 second, timeout.
    for(i=0; i < clkRate * 1; i++) {
        nBytesUnread = pifchr(masSpeedPort);
        if(nBytesUnread > 0) {
            // Read one char even if more are available
            if (read(masSpeedPort, chr, 1) > 0) {
                return 1;
            }
        }
        taskDelay(calcSysClkTicks(17)); /* taskDelay(1); */
    }

    // We must have timed out
    return -1;

}

/* Get a response from the CM speed controller.
    The mas speed controller terminates all messages with its prompt: "\n>"

    Successful communications return a 1, failures return -1.

    If a speed limit error is detected from the mas, then
    -1 is returned and result is set to SPEED LIMIT
*/
int getMASSpeedResponseString(char *result)
{
    char in_str[80];
    long cnt=0;
    long break_out=0, place;

    /* clear and terminate the input string in case we have an error
       and don't set the result string.
    */
    result[0] = '\0';

    while (cnt < 80) {
        if (readCharWithTimeout(&in_str[cnt]) > 0) {
            if (in_str[cnt] == '>') {   // end of response
                break;
            }
            if (in_str[cnt] == '?') {  // error delimiter, '??'
                break_out++;
                if (break_out == 2)
                    break;
            }
            cnt++;
        }
        else {
            // Must have timed out
            errLogRet(LOGIT, debugInfo,
                      "Timed Out trying to read the CM speed controller\n");
            return -1;
        }
    }
    // in_str[cnt+1]=0;
    // printf("'%s'\n",in_str);
    /* catch error of setting speed above speed limit */
    if (break_out == 2) {
        place = 0;
        while (in_str[place++] != '?');
        if (!strncmp(&in_str[place], "SPEED LIMIT", 11)) {
            // speed limit error detected
            strcpy(result, "SPEED LIMIT\0");
            return -1;
        }
        if (!strncmp(&in_str[place], "RS-232 OVER-RUN ERROR", 21)) {
            // over run error detected
            strcpy(result, "RS 232 OverRun\0");
            
            return -1;
        }

        else {  // Some other error
            strcpy(result, in_str);
            result[cnt+1] = '\0';    // Terminate the error string
            return -1;
        }
    }

    // if just prompt returned, return ok
    if (cnt == 2 && in_str[cnt] == '>') {
        result[0] = '\n';
        result[1] = '>';
        result[2] = '\0';
        return 1;
    }

	// If ending > is found, replace it with a termination
	if (in_str[cnt] == '>' && cnt > 1) 
		in_str[cnt-1] = '\0';
	else 
		return -1;   // not a valid response

    /* Put the string obtained into the callers 'result'. */
    strcpy(result, in_str);
    return 1;

}

/* Return an integer response from the MAS controller.  Only call this
   when an integer is expected.  Returns -1 for error.  No known responses
   from the controller should be negative.
*/
int getMASSpeedResponseInt(int *result) 
{
    int status;
    char str[80];
    int  value=0;

    /* Get the response from the controller as a string */
    status = getMASSpeedResponseString(str);

    /* catch error of setting speed above speed limit */
    if(status == -1) {
        if (strcmp(str, "SPEED LIMIT") == 0) {
             // -2 specifically denotes speed limit error detected
            *result = -2; 
            return status;
        }
        else {
            errLogRet(LOGIT, debugInfo,"Error from controller: %s\n", str);
            *result = -1;
            return status;
        }
    }

    // if just prompt returned, return ok
    if (str[1] == '>') {
        *result = -1;
        return 1;
    }

    // try to convert returned string into a number
    if (sscanf(str, "%d", &value) != 1) {   
        *result = -1;    // error occured in scan
        return -1;
    }
    else {
        *result = value;
        return 1;
    }
    return status;
}

/* Get the current Module ID and see if it has changed since we last
   checked and updated things.  If it has changed, get numerous values
   from the controller and send to vnmrj to get it up to date.
   If the probe is unplugged then the same probe is plugged in again,
   all params will be set to defaults including a set point of 0.
   I will check for a change in set point from non-zero to zero also.

   checkModuleChange() MUST ONLY BE CALLED FROM WITHIN MASSpeed(),
   else it can be trying to send cmds to the controller at the same time
   MASSpeed() is sending cmds, and the return values get out of sync.
*/
void checkModuleChange() 
{
    char strRes[80];
    int  status;
    int  curSetPoint;

    sendToMASSpeed("MODULE\n");
    status = getMASSpeedResponseString(strRes);
    if(status == 1) {
        if(strcmp(strRes, module) != 0) {
            // New module name found, update the one we keep around
            strcpy(module, strRes);
            DPRINT1(1,"Found new Module %s\n", module);
            // Need to get all new params
            getAllParams();
            // No need to check set point below, our work is done.
            return;
        }
    }
    else {
        // Try clearing out controller and hope for better
        // luck next time.
        ckMASSpeedInternal();
    }

    // The module did not change.  However, if they unplugged and replugged,
    // the connector, the controller will reset itself.  Try looking at
    // the set point also if it was non-zero and see if it has changed
    // to zero.
    if(setPoint != 0) {
        sendToMASSpeed("SETP\n");
        status = getMASSpeedResponseInt(&curSetPoint);
        if(status == 1) {
            if(setPoint != curSetPoint) {
                setPoint = curSetPoint;
            // Need to get all new params
            getAllParams();
            }
        }
        else {
            // Try clearing out controller and hope for better
            // luck next time.
            ckMASSpeedInternal();
        }
    }
}


/* VxWorks command to get probe module ID.
   It must go through MASSpeed().
*/
void getModuleID() {
    msgQSend(msgToMASSpeedCtl, "GET MODULE\n", 12, 0, 0);
}


/* VxWorks command to get all parameters.
   Since getAllParams() can only be called from within MASSpeed(), this is
   to send the request through MASSpeed() which will take care of the request
   when it is free.
*/
void getAllParamsCmd() {
    msgQSend(msgToMASSpeedCtl, "GET ALLPARAMS\n", 15, 0, 0);
}



/* When the controller has been reset, the module has been changed or we
   are just starting up, we need to get all of the params from the
   speed controller that are needed for the GUI.  That list would be
   Module, Speed Limit, Span, Adj, Max, ActSp, Profile, SetPoint

   getAllParams() MUST ONLY BE CALLED FROM WITHIN MASSpeed() or if it is asleep,
   else it can be trying to send cmds to the controller at the same time
   MASSpeed() is sending cmds, and the return values get out of sync.
*/
void getAllParams()
{
    char strRes[80];
    int  result;
    int  status;

    /* Init all values to -1 so that when they are changed to the correct
       values, they will update the panel.
    */
    pCurrentStatBlock->AcqSpinSpan = -1;
    pCurrentStatBlock->AcqSpinAdj = -1;
    pCurrentStatBlock->AcqSpinMax = -1;
    pCurrentStatBlock->AcqSpinSet = -1;
    pCurrentStatBlock->AcqSpinProfile = -1;
    pCurrentStatBlock->AcqSpinSpeedLimit = -1;
    pCurrentStatBlock->AcqSpinActSp = -1;
    pCurrentStatBlock->probeId1[0] = '\0';

    // Tell it we have changed values
    sendConsoleStatus();

    taskDelay(calcSysClkTicks(17)); /* taskDelay(1); */

    /* Now get current values from the controller. */
    sendToMASSpeed("BEARING SPAN\n");
    status = getMASSpeedResponseInt(&result);
    if(status == 1) {
        pCurrentStatBlock->AcqSpinSpan = result;
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with Bearing command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }
    sendToMASSpeed("BEARING ADJ\n");
    status = getMASSpeedResponseInt(&result);
    if(status == 1) {
        pCurrentStatBlock->AcqSpinAdj = result;
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with Bearing command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }
    sendToMASSpeed("BEARING MAX\n");
    status = getMASSpeedResponseInt(&result);
    if(status == 1) {
        pCurrentStatBlock->AcqSpinMax = result;
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with Bearing command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }

    sendToMASSpeed("MODULE\n");
    status = getMASSpeedResponseString(strRes);
    if(status == 1) {
        strcpy(module, strRes);
        strcpy(pCurrentStatBlock->probeId1, strRes);
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with Module command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }

    sendToMASSpeed("SPEED LIMIT\n");
    status = getMASSpeedResponseString(strRes);
    if(status == 1) {
        // Since this returns 'MAX SPEED = ', we need to
        // take the string after that in strRes
        if (sscanf(&strRes[12], "%d", &result) == 1) {   
            pCurrentStatBlock->AcqSpinSpeedLimit = result;
        }

    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with SPEED LIMIT command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }

    sendToMASSpeed("SETP\n");
    status = getMASSpeedResponseInt(&result);
    if(status == 1) {
        setPoint = result;
        pCurrentStatBlock->AcqSpinSet = result;
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with SETP command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }

    sendToMASSpeed("PROFILE\n");
    status = getMASSpeedResponseString(strRes);
    if(status == 1) {
        if(strncmp(strRes, "ACTIVE", 6) == 0) {
            pCurrentStatBlock->AcqSpinProfile = 1;
        }
        else {
            pCurrentStatBlock->AcqSpinProfile = 0;
        }
 
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with PROFILE command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }
  
    sendToMASSpeed("ACTSP\n");
    status = getMASSpeedResponseInt(&result);
    if(status == 1) {
        pCurrentStatBlock->AcqSpinActSp = result;
    }
    else {
        errLogRet(LOGIT, debugInfo,"Problem with ACTSP command\n");
        // Try clearing out controller and hope for better luck next time.
        ckMASSpeedInternal();
    }

    // Tell it we have changed values
    sendConsoleStatus();
}

/* 
   masSpinner()
   
   Come here when commands come into X_interp for spinning and the active
   controller is the MAS controller.  Since this is called outside of
   MASSpeed(), masSpinner() MUST NOT send commands directly to the
   controller, but MUST go through MASSpeed() by sending a msg.

*/

void masSpinner(int *paramvec, int *index, int count)
{
    int  token;
    char str[32];
    int  slen;

    while (*index < count) {
        token = paramvec[*index]; (*index)++;
        /* For some reason, I get a 0 cmd before I get real commands
           just continue if 0 */
        if(token == 0)
            continue;
        switch(token) {
            /* BEAROFF is used to not only turn bearing air on/off for 
               the liquids spinner, but for spinner on/off, so in our 
               context, it has NOTHING to do with bearing air, but is 
               ONLY for spinning on/off.
            */
          case MASOFF: 
            /* Create a cmd and put into the msgQ */
            msgQSend(msgToMASSpeedCtl, "SET STOP\n", 10, 0, 0);
            break;
            /* BEARON is used to not only turn bearing air on/off for 
               the liquids spinner, but for spinner on/off, so in our 
               context, it has NOTHING to do with bearing air, but is 
               ONLY for spinning on/off.
            */
          case MASON:
            msgQSend(msgToMASSpeedCtl, "SET START\n", 11, 0, 0);
            break;
          case SETSPD:
            /* Get the next arg which should be the speed */
            token = paramvec[*index]; (*index)++;

            pCurrentStatBlock->AcqSpinSet = token;
            sprintf(str, "SET S %d\n", token);
            slen = strlen(str);
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);
            break;
          case SETBEARMAX:
            /* Get the next arg which should be the max */
            token = paramvec[*index]; (*index)++;

            pCurrentStatBlock->AcqSpinMax = token;
            sprintf(str, "SET BEARING MAX %d\n", token);
            slen = strlen(str);
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);
            break;
          case SETBEARSPAN:
            /* Get the next arg which should be the span */
            token = paramvec[*index]; (*index)++;

            pCurrentStatBlock->AcqSpinSpan = token;
            sprintf(str, "SET BEARING SPAN %d\n", token);
            slen = strlen(str);
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);
            break;
          case SETBEARADJ:
            /* Get the next arg which should be the adj */
            token = paramvec[*index]; (*index)++;

            pCurrentStatBlock->AcqSpinAdj = token;
            sprintf(str, "SET BEARING ADJ %d\n", token);
            slen = strlen(str);
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);
            break;
          case SETASP:
            /* Get the next arg which should be the active set point 0, 1, 2 */
            token = paramvec[*index]; (*index)++;

            pCurrentStatBlock->AcqSpinActSp = token;
            sprintf(str, "SET ACTSP %d\n", token);
            slen = strlen(str);
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);

            // Get the new set speed so the panel stays up to date.
            sprintf(str, "GET SETP\n");
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);
            break;
          case SETPROFILE:
            /* Get the next arg which should be the 0/1 for on/off */
            token = paramvec[*index]; (*index)++;

            pCurrentStatBlock->AcqSpinProfile = token;
            if(token == 1) 
                sprintf(str, "SET PROFILE ON\n");
            else
                sprintf(str, "SET PROFILE OFF\n");
            slen = strlen(str);
            msgQSend(msgToMASSpeedCtl, str, slen+1, 0, 0);
            break;
          // Ignore BEAROFF and BEARON, these are the ones used for liq
          case BEAROFF: 
          case BEARON:
            break;

          default:
            errLogRet(LOGIT, debugInfo,"masSpinner: cmd %d not supported by MAS Spin Controller (yet)\n",token);
            break;

        } /* end of switch */
    } /* end of while */
} /* masSpinner() */

/* Get a response from the CM speed controller.
    The mas speed controller terminates all messages with its prompt: "\n>"

    Successful communications return a 1, timeout failures return -1.
   
    similar to getMASSpeedResponseString(), but without various testing and string mods
    just the recieved string is return without modifications

    Author: Greg Brissey      8/26/08

*/
int getMASResponseString(char *result, int len)
{
    char in_str[1028];
    int status = 0;
    long cnt=0,prevchr;
    long break_out=0;

    /* clear and terminate the input string in case we have an error
       and don't set the result string.
    */
    result[0] = '\0';

    // printf("reading: \n");
    while (cnt < 1027) {
        if (readCharWithTimeout(&in_str[cnt]) > 0) 
        {
            // printf("%c",in_str[cnt]);
            prevchr = (cnt > 0) ? cnt-1 : 0; 
            // if (in_str[cnt] == '>') {   // end of response
            //   printf("\nprev char: '%c', 0x%x\n", in_str[prevchr],in_str[prevchr]);
            // }

            if ((in_str[prevchr] == '\n') && (in_str[cnt] == '>')) {
                // printf("got NL and >\n");
                break;
            }
            if (in_str[cnt] == '?') {  // error delimiter, '??'
                break_out++;
                if (break_out == 2)
                    break;
            }
            cnt++;
        }
        else {
            // Must have timed out
            // printf("\nTimeout\n");
            status = -1;
            break;
        }
    }
    in_str[cnt+1]=0;
    // printf("\n\ncnt: %d, last char: '%c'\n",cnt,in_str[cnt]);
    // printf("in_str: '%s'\n",in_str);
    /* Put the string obtained into the callers 'result'. */
    strncpy(result, in_str, len);
    return status;
}

#ifndef MASPROC
/*
 * tipmas()
 * Function to give a transparent mode to the MAS controller for the User
 * Once this cmd is entered, the infinite loop reads the user input and sends it
 * on to the MAS controller, printing for the user the response of the MAS controller.
 * To exit this function the user types: quit
 *   Author Greg Brissey   8/8/08
 */
/* was TMAS */
tipmas()
{
   void putMASSpeedToSleep();
   void wakeUpMASSpeed();
   char inputbuf[128];
   char strRes[1024];
   char *strptr;
   int status,len,i,badinput;


   initMASSpeed(); /* start and init the MAS if not already */
   putMASSpeedToSleep();   /* put MAS task to sleep so our commands won't clash with it */
   taskDelay(calcSysClkTicks(500)); /* 500 ms, or 60 * .2 = 12 */
   status = ckMASSpeedInternal();   /* check and clear MAS com link */
   if(status < 1) {
        printf("*** MAS Controller Not Ready, Aborting\n");
        return 0;   /* Bail Out */
   }
   printf("Entering MAS Transparent Mode\n");
   printf("Enter MAS command strings, To end transparent mode session type: quittip \n>");
   while ( 1 )
   {
        strptr = gets(inputbuf);
        if (strcmp("quittip",inputbuf) == 0)   // if 'quit' then exit
        {
           break;
        }
        len = strlen(inputbuf);
        inputbuf[len] = '\n';    // gets() strips out '\n' char, so we must put it back in
        inputbuf[len+1] = 0;
        badinput = 0;
        // printf("input chars: \n");
        for (i = 0; i < len; i++)  // don't test inputbuf[len]=\n or it will fail aways
        {
            // printf(" '%c', 0x%x\n ", inputbuf[i], inputbuf[i]);
            // if there are esc or ~ or DEL char skip, input and retry, found that these odd 
            // characters combine with number arg causes MAS controller to go bye-bye 
            if ( ! ((inputbuf[i] >  0x1F /* DEL */) && (inputbuf[i] < 0x7E /* ~ */)) )
            {
               // printf("invalid input.\n>");
               printf("TYPE: <COMMAND><CR>\n>");
               badinput = 1;
               break;
            } 
        }
        if (badinput == 1)
        {
           continue;
        }
        // sprintf(cmdbuf,"%s\n",inputbuf);  // gets() strip out \n, so we must put it back in
        // sendToMASSpeed(cmdbuf);           // send the cmd to MAS
        sendToMASSpeed(inputbuf);           // send the cmd to MAS
        status = getMASResponseString(strRes,1024);  // get the response  from MAS
        if (status == -1)
            printf("%s\n>",strRes);   // let the user see MAS response.
        else
            printf("%s",strRes);   // let the user see MAS response.
   }

   wakeUpMASSpeed();       /* wake the MAS task */
   printf("\nExiting MAS Transparent Mode\nAt the prompt, logoff the controller by typing: logout \n\n");
   return 0;
}
#endif


/* The definition in the firmware for the MAS controller for its lock light
   to come on is for the current speed to be within the larger of 5Hz or
   0.1% of the target.
*/
#define LSDV_SPIN_OFF	0x00
#define LSDV_SPIN_REG   0x10
#define LSDV_SPIN_UNREG 0x20
#define LSDV_SPIN_MASK  0x30

extern volatile unsigned int *pMASTER_SpinPulseLow;

void setSpinLEDMas(int state) {
    switch(state) {
      case SpinLED_STOP:
        if (curMasSpeed == 0) {
            *pMASTER_SpinPulseLow = 40000000; /* low full period of 1 sec */
        }
        else {
            *pMASTER_SpinPulseLow = 26600000; /* low for 2/3, high 1/3 */
        }
        pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
        break;

      case SpinLED_BLINKshort:
        *pMASTER_SpinPulseLow = 26600000;     /* low for 2/3, high 1/3 */
        pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
        pCurrentStatBlock->AcqLSDVbits |=  LSDV_SPIN_UNREG;
        break;

      case SpinLED_BLINKlong:
        *pMASTER_SpinPulseLow = 13300000;     /* low for 1/3, high 2/3 */
        pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
        pCurrentStatBlock->AcqLSDVbits |=  LSDV_SPIN_UNREG;
        break;

      case SpinLED_OK:
        *pMASTER_SpinPulseLow =        0;    /* low for 0, high 1 sec */
        pCurrentStatBlock->AcqLSDVbits &= ~LSDV_SPIN_MASK;
        pCurrentStatBlock->AcqLSDVbits |=  LSDV_SPIN_REG;
        break;
    }
    return;

}


/* Set the LED as appropriate.
   LED on if within tolerance of +/- 5Hz or +/- 0.1%, whichever is more.
   If speed increasing then fast blinking, slowing slow blinking
   If speed is zero, LED off.
*/
static void checkSpinLEDMas ()
{
    int absSpeedDeviation;  // Deviation from setPoint
    int speedChangeSign;    // Speeding up = pos #, slowing down = neg #


    // If no change, just bail out
    if(curMasSpeed == prevMasSpeed)
        return;

    if(curMasSpeed == 0) {
        // Stopped, turn LED off
        setSpinLEDMas(SpinLED_STOP);
    }
    else {
        absSpeedDeviation = abs(setPoint - curMasSpeed);
        if(absSpeedDeviation <= 5 || absSpeedDeviation <= (0.001 * setPoint)) {
            // In regulation
            setSpinLEDMas(SpinLED_OK);
        }
        else {
            // Not stopped, and not in regulation
            speedChangeSign = curMasSpeed - prevMasSpeed;
            if(speedChangeSign < 0) {
                // speeding up, blink short blinks
                setSpinLEDMas(SpinLED_BLINKshort);
            }
            else {
                // slowing down, blink long blinks
                setSpinLEDMas(SpinLED_BLINKlong);
            }
        }
    }
}


/* putMASSpeedToSleep
   We want to keep the task running, but sometimes we want the task to
   not do anything.  This can be if the liq controller is to be used for
   awhile, or if we need to send cmds to the controller from this code and
   do not want to collide with the task loop.
*/
void putMASSpeedToSleep() {
    if(MasSpeedTaskSleeping) {
        DPRINT(0, "MASSpeed staying asleep\n");
    }
    else {
        MasSpeedTaskSleeping = TRUE;
        DPRINT(0, "MASSpeed going to sleep\n");
    }
}


/* Wake the task back up. */
void wakeUpMASSpeed() {

    MasSpeedTaskSleeping = FALSE;
    DPRINT(0, "MASSpeed waking up\n");

    hsspi(2,0x1000000);  // Clear this for MAS, it is used for liq and tach

    /* need to do this for rotorsync to work with Varian/ChemMagnets MAS controller, GMB  */
    selectSpinner( 0 );   /* select spinner pulse mux to Solids input 0-solids, 1-liquids */


    /* Send command to update all params to current. */
    getAllParamsCmd();
}
