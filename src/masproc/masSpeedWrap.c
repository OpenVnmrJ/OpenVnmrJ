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

#include <time.h>
#include <sys/ioctl.h>

#include "masSpeedWrap.h"
#include "errLogLib.h"


Console_Stat StatBlock;

Console_Stat *pCurrentStatBlock = &StatBlock;

static int SpinSpan = -1;
static int SpinAdj = -1;
static int SpinMax = -1;
static int SpinSet = -1;
static int SpinProfile = -1;
static int SpinSpeedLimit = -1;
static int SpinActSp = -1;
static char probeId[128] = "";

char *MASPORTNAME = "/dev/ttyS0";
unsigned int MASTER_SpinPulseLow;
unsigned int *pMASTER_SpinPulseLow = &MASTER_SpinPulseLow;

extern int masSpeedPort;
extern float clkRate;

void setMasSpeedPort(int fd)
{
   masSpeedPort = fd;
   clkRate = 4.0;
}

int sysClkRateGet()
{
  return 1;
}

int calcSysClkTicks(int msec)
{
  return(msec);
}

void taskDelay(int msec)
{
   struct timespec del;
   int stat;

   del.tv_sec = 0;
   del.tv_nsec = msec * 1000000;
   stat = nanosleep(&del, NULL);
}

void hsspi(int dum, int dum2)
{
  return;
}

int taskSpawn(char *name, int p, int t, int s, void *(func)(), int a1,
             int a2, int a3, int a4, int a5, int a6, int a7, int a8,
             int a9, int a10)
{
   return 1;
}

int msgQMasCreate(int a, int b, int c)
{
  return 1;
}

int msgQMasReceive(int a, char *buf, int len, int delay)
{
   return 1;
}

void sendConsoleStatus()
{
  return;
}

int clearport(int port)
{
   return (flushPort( port ) );
}

int pifchr(int port)
{
   int  status;
   int  nBytesUnread;
   status = ioctl( port, FIONREAD, (int) &nBytesUnread );
   /*printf( "ARE THERE ANYTHING ON THE PORT?: %d\n", nBytesUnread );*/
   return( (status) ? 0 : nBytesUnread );
}

void msgQMasSend(int id, char *msg, int len, int a1, int a2)
{
   DPRINT2(3,"msgQSEnd: Send '%s' len= %d\n", msg, len);
   processMasMessage(msg);
}

void selectSpinner(int val)
{
   /* Do nothing */
   return;
}

int getMasSpeed()
{
   int status;
   int response;

   sendToMASSpeed("S\n");
   status = getMASSpeedResponseInt(&response);

   if(status == -1 && response == -2)
   {
                DPRINT(0, "*** Trying to set speed higher than limit.\n");
   }
   else if(status == -1)
   {
                DPRINT(0, "*** Speed Controller had problem with 'S' command.\n");
   }
   else {
                // Set the new response into the NDDS structure
                pCurrentStatBlock->AcqSpinAct = response;

                // Tell it we have changed a value
                sendConsoleStatus();

                DPRINT1(3, "Current Speed = %d\n", response);
   }
   return(response);
}

void showMasPars()
{
    DPRINT1(1,"AcqSpinSpan:       %d\n", pCurrentStatBlock->AcqSpinSpan);
    DPRINT1(1,"AcqSpinAdj:        %d\n", pCurrentStatBlock->AcqSpinAdj);
    DPRINT1(1,"AcqSpinMax:        %d\n", pCurrentStatBlock->AcqSpinMax);
    DPRINT1(1,"AcqSpinSet:        %d\n", pCurrentStatBlock->AcqSpinSet);
    DPRINT1(1,"AcqSpinProfile:    %d\n", pCurrentStatBlock->AcqSpinProfile);
    DPRINT1(1,"AcqSpinSpeedLimit: %d\n", pCurrentStatBlock->AcqSpinSpeedLimit);
    DPRINT1(1,"AcqSpinActSp:      %d\n", pCurrentStatBlock->AcqSpinActSp);
    DPRINT1(1,"AcqSpinprobeId1:   %s\n", pCurrentStatBlock->probeId1);
}

int masParsChanged(char *cmd)
{
   showMasPars();
   if ( strcmp(probeId, pCurrentStatBlock->probeId1)  ||
        (SpinSpan != pCurrentStatBlock->AcqSpinSpan) ||
        (SpinAdj != pCurrentStatBlock->AcqSpinAdj) ||
        (SpinMax != pCurrentStatBlock->AcqSpinMax) ||
        (SpinSet != pCurrentStatBlock->AcqSpinSet) ||
        (SpinProfile != pCurrentStatBlock->AcqSpinProfile) ||
        (SpinSpeedLimit != pCurrentStatBlock->AcqSpinSpeedLimit) ||
        (SpinActSp != pCurrentStatBlock->AcqSpinActSp) )
   {
      strcpy(probeId, pCurrentStatBlock->probeId1);
      SpinSpan = pCurrentStatBlock->AcqSpinSpan;
      SpinAdj = pCurrentStatBlock->AcqSpinAdj;
      SpinMax = pCurrentStatBlock->AcqSpinMax;
      SpinSet = pCurrentStatBlock->AcqSpinSet;
      SpinProfile = pCurrentStatBlock->AcqSpinProfile;
      SpinSpeedLimit = pCurrentStatBlock->AcqSpinSpeedLimit;
      SpinActSp = pCurrentStatBlock->AcqSpinActSp;
      sprintf(cmd,"maspars %d %d %d %d %d %d %d %s;\n",
              SpinSpan, SpinAdj, SpinMax, SpinSet,
              SpinProfile, SpinSpeedLimit, SpinActSp, probeId);
      return(1);
   }
   return(0);
}

