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
/*
/*
DESCRIPTION

   Lock specific routines

*/

// #ifndef   PFG_CNTLR

#ifndef ALLREADY_POSIX
#  define _POSIX_SOURCE         /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <msgQLib.h>

#include "logMsgLib.h"
#include "taskPriority.h"
#include "nvhardware.h"
#include "fpgaBaseISR.h"
#include "lockcomm.h"
#ifdef PFG_LOCK_COMBO_CNTLR
  #include "lock_reg.h"
  #include "lpfg_top.h"     // for lock DDS definitions
#else
  #include "lock.h"
#endif
#include "Lock_FID.h"
#include "Lock_Stat.h"

#define VPINT(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)

#define VPSHORT(arg) volatile short *p##arg = (short *) (FPGA_BASE_ADR + arg)


VPINT(LOCK_Locked);

#define p2LOCK_INTERRUPT_STATUS  	\
		((volatile int *) (LOCK_BASE + LOCK_InterruptStatus))
#define p2LOCK_INTERRUPT_ENABLE  	\
		((volatile int *) (LOCK_BASE + LOCK_InterruptEnable))
#define p2LOCK_INTERRUPT_CLEAR   	\
		((volatile int *) (LOCK_BASE + LOCK_InterruptClear))
#define p2LOCK_INTERRUPT_LAST    	\
		((volatile int *) (LOCK_BASE + LOCK_Last))

/* DO NOT Alter these pointers !!!!!! */
extern Lock_FID *pLkFIDIssue;
extern Lock_Stat *pLkStatIssue;

extern char fpgaLoadStr[40];

char msg1[20] = "0";
char msg2[20] = "1";

MSG_Q_ID pMsgesToLockMon = NULL;
MSG_Q_ID pMsgesToLL = NULL;
int LLTid = 0;

/* sending status 2000/sec is a little much */
static int count = 0;
static int countLimit = 500;
static int AbsRollAve, DisRollAve;

struct _LockMessage
{
   unsigned int action;
   int status;
   int arg1;
   int arg2;
   int partial;
};

struct _LLMessage
{
   int action;
   char filen[80];
};

volatile short *p2Buffer0 = (short *) REG_ADDR(LOCK_real_data0_addr);//was hardcoded as (0x70008000);
volatile short *p2Buffer1 = (short *) REG_ADDR(LOCK_real_data1_addr);//was hardcoded as (0x70010000);

int nerk = 0;
int acc = 0;

#ifdef PFG_LOCK_COMBO_CNTLR
volatile unsigned char *ddsprogram = (unsigned char *) REG_ADDR(LPFG_LOCK_DDS_BASE);
#else
volatile unsigned char *ddsprogram = (unsigned char *) REG_ADDR(0x20000); 
#endif

/* used for 9852 DDS */
union ltw
{
   unsigned long long ldur;
   unsigned int idur[2];
   unsigned char cdur[8];
};

/* these are the locations for lock search */
volatile unsigned int XBuffer0[5000], XBuffer1[5000];
/* static */
/* modes might be LockLevel = 1, LockSearch = 2 else nop */
int modeSpecifier = 0;
/* fix acq rate at 5 khz?? */
int pointsPerShot = 2;          /* mode = 1 only */
/* this is internal */
int AbsorptionModeBoxcar[2048];
int DispersionModeBoxcar[2048];
int LiveIndex = 0;
int BoxCarSize = 256;
/* these are the outputs for lockLevel */
int LockedFlag = 0;
int AbsorptionAverage = 0;
int DispersionAverage = 0;
int AborptionTrend = 0;
int DispersionTrend = 0;


/*====================================================================================*/
/*====================================================================================*/

void resetDDS()
{
   *REG_ADDR(LOCK_DDSReset) = 0;
   *REG_ADDR(LOCK_DDSReset) = 1;
   *REG_ADDR(LOCK_DDSReset) = 0;
}

void resetADC()
{
   *REG_ADDR(LOCK_ADCSpare) = 0;
   *REG_ADDR(LOCK_ADCSpare) = 1;
   *REG_ADDR(LOCK_ADCSpare) = 0;
}

void enableADC()
{
   *REG_ADDR(LOCK_ADCSpare) = 1;
}

int getLockGain()
{
   /* this routine does not convert the linear value back to dB */
   return (get_field(LOCK, rx_gain));
}


static const u_char lin_db[] = {
   0,                           /*  0dB */
   1,                           /*  1 dB */
   1,                           /*  2 dB */
   1,                           /*  3 dB */
   2,                           /*  4 dB */
   2,                           /*  5 dB */
   2,                           /*  6 dB */
   2,                           /*  7 dB */
   3,                           /*  8 dB */
   3,                           /*  9 dB */
   3,                           /* 10 dB */
   4,                           /* 11 dB */
   4,                           /* 12 dB */
   4,                           /* 13 dB */
   5,                           /* 14 dB */
   6,                           /* 15 dB */
   6,                           /* 16 dB */
   7,                           /* 17 dB */
   8,                           /* 18 dB */
   9,                           /* 19 dB */
   10,                          /* 20 dB */
   11,                          /* 21 dB */
   13,                          /* 22 dB */
   14,                          /* 23 dB */
   16,                          /* 24 dB */
   18,                          /* 25 dB */
   20,                          /* 26 dB */
   23,                          /* 27 dB */
   25,                          /* 28 dB */
   28,                          /* 29 dB */
   32,                          /* 30 dB */
   36,                          /* 31 dB */
   40,                          /* 32 dB */
   45,                          /* 33 dB */
   51,                          /* 34 dB */
   57,                          /* 35 dB */
   64,                          /* 36 dB */
   72,                          /* 37 dB */
   80,                          /* 38 dB */
   90,                          /* 39 dB */
   101,                         /* 40 dB */
   113,                         /* 41 dB */
   128,                         /* 42 dB */
   143,                         /* 43 dB */
   161,                         /* 44 dB */
   180,                         /* 45 dB */
   202,                         /* 46 dB */
   227,                         /* 47 dB */
   255,                         /* 48 dB */
};

void setLockGain(int gain)
{
   int lin_gain;
   if (gain < 0)
      gain = 0;
   else if (gain > 48)
      gain = 48;
   lin_gain = lin_db[gain];
   DPRINT2(-1, "gain=%d,lingain=%d\n", gain, lin_gain);
   set_field(LOCK, rx_gain, lin_gain);
   if (pLkStatIssue != NULL)
      pLkStatIssue->lkgain = gain;
   pubLkStat();
}

/* power can run from 0-48 dB. 
/* If lockpower is 0, no lock pulses, but gate for lockdisplay
/* If lockpower is -1, then rcvr on only. Occurs when tn='lk'
/* This is set in lockRate based on the duty cycle
/*  */
void setLockPower(int power)
{
   int dbval, tblsize;
   void lockRate(double, double);
   if (pLkStatIssue != NULL)
   {
      if (power >= 0)
         pLkStatIssue->lkpower = power;
   }
   if (power > 48)
   {
      /* the other 20 dB is set on the NSR by the master */
      /* it is subtracted here, so puLkStat() returns the true value */
      power -= 20;
   }
   if (power <= 0)
   {
      if (modeSpecifier > 1)
         lockRate(20.0, (double) power);        //power is 0 or -1
      else
         lockRate(2000.0, (double) power);      //power is 0 or -1
      power = 0;
   }
   else
   {
      if (modeSpecifier > 1)
         lockRate(20.0, 12.0);
      else
         lockRate(2000.0, 12.0);
      tblsize = sizeof(lin_db) / sizeof(lin_db[0]);
      if (power >= tblsize)
         power = tblsize - 1;
   }
   dbval = lin_db[power];

   set_field(LOCK, tx_power, dbval);
   DPRINT2(3,"lock tx power set to %d (actual %d)\n", dbval, get_field(LOCK, tx_power));

   pubLkStat();
}

int getLockPower()
{
   /* this routine does not convert the linear value back to dB */
   return (get_field(LOCK, tx_power));
}

void setLockPhase(int phase)
{
   int tmp;
   /* Phase 0-360 --> 0-255, hardware 1.41 degree interval or 360/255 = 1.41 */
   tmp = phase * 255 / 360;
   set_field(LOCK, tx_phase, tmp);
   if (pLkStatIssue != NULL)
      pLkStatIssue->lkphase = phase;
   pubLkStat();
   DPRINT2(3,"lock tx phase set to %d (actual %d)\n", phase, get_field(LOCK, tx_phase));
}

int getLockPhase()
{
   return (get_field(LOCK, tx_phase));
}

int getLockLevel()
{
   return (pLkStatIssue->lkLevelR);
}

void pulser_on()
{
   set_field(LOCK, seq_gen_enable, 1);
   if (pLkStatIssue != NULL)
      pLkStatIssue->lkon = 1;
   pubLkStat();

}

void pulser_off()
{
   set_field(LOCK, seq_gen_enable, 0);
   if (pLkStatIssue != NULL)
      pLkStatIssue->lkon = 0;
   pubLkStat();
}

int get_pulser_on()
{
   return (get_field(LOCK, seq_gen_enable));
}

get_interrupt_status()
{
   printf("%x is int 0 status\n", get_field(LOCK, adc_buffer_complete_int_0_status));
   printf("%x is int 1 status\n", get_field(LOCK, adc_buffer_complete_int_1_status));
}

/*
   <-------   rx_stop ---------------------------------------->
   <--------  rx_start ---->
   <--> tx_start
   <---------> tx_stop
*/
/* if its running stop it and restart else leave alone */

set_pulser(int rept, int rof1, int pw, int rof2, int dwell, int ncp)
{
   int state;
   state = get_field(LOCK, seq_gen_enable);
   if (state != 0)
      pulser_off();
   set_field(LOCK, rx_stop, rept);
   set_field(LOCK, rx_start, rof1 + pw + rof2);
   set_field(LOCK, tx_start, rof1);
   set_field(LOCK, tx_stop, rof1 + pw);
   set_field(LOCK, ctc_period, dwell);
   set_field(LOCK, num_ctc_pulses, ncp);
   if (state != 0)
      pulser_on();
}

void lockRate(double hz, double duty)
{
   int repTicks, rxOnTicks, rof, pw, rxOffTicks;
   double repTime, rxOffTime, dwellTicks;
   int npairs, spare;
   int dutyInt;
   /* duty == -12 is a special key to test lock power= 0 */
   if (duty == -12.0)
   {
      if (pLkStatIssue && (pLkStatIssue->lkpower == 0) )
         duty = 0.0;
      else
         duty = 12.0;
   }
   if (duty < -0.2)
      dutyInt = -1;
   else if (duty < .2)
      dutyInt = 0;
   else
      dutyInt = (int) (duty + .1);
   if (hz < 0.0)
      hz = 0.3;
   if (hz > 1000000.0)
      hz = 1000000.0;
   countLimit = hz / 4;         // force par update once every 1/4 second

   if (duty < 0)
      duty = 0;
   if (duty > 80.0)
      duty = 80.0;

   repTime = (80000000.0 / hz);
   repTicks = (int) repTime;
   rxOffTime = repTime * duty / 100.0;
   rxOffTicks = (int) rxOffTime;

   rof = (int) repTime *0.01;   /* 4 % fixed by spec.. */
   /* 5.0 kHz acq => 200.00 usec dwell * 80 ticks/ usec = 16000 ticks */
   /* 6.4 kHz acq => 156.25 usec dwell * 80 ticks/ usec = 12500 ticks */
   if (hz > 100)
      dwellTicks = 12500.0;
   else
      dwellTicks = 12500.0;
   spare = 0;
   npairs = (int) ((repTime - rxOffTime) / dwellTicks) + 1;
   if (npairs < 0)
      npairs = 4;
   if (npairs > 700)
   {
      spare = npairs - 700;
      spare *= dwellTicks;
      npairs = 700;
   }

   printf("rep tick=%d   rof=%d  pw=%d npairs=%d, dwell=%d spares=%d\n",
          repTicks, rof, rxOffTicks - 2 * rof, npairs, (int) dwellTicks, spare);
   pointsPerShot = npairs;
   if (hz > 100.0)
      modeSpecifier = 1;        /* average/copy flag */
   else
      modeSpecifier = 2;        /* lock display */
   if (dutyInt < 0)             // rcvr on always, tn='lk'
   {
      set_pulser(40000, 0, 0, 0, 3000, 12);
      *pLOCK_Locked = 0;        // turn lock status LED off
   }
   else if (dutyInt == 0)       // lockpower = 0, no pulses
      set_pulser(repTicks, rof + spare, 0, rof, (int) dwellTicks, npairs);
   else
      set_pulser(repTicks, rof + spare, rxOffTicks - 2 * rof, rof, (int) dwellTicks, npairs);
}

/***************************************************
goal - log the re/im raw pairs into a large buffer 
       structed as fill and stop OR ring and continue
       control - no log, log to fill, ring buffer log
       default no log
*/
int lockLogCntrl = 0;
int lockLogNum = 0;
int lockLogState = 0;
short *logBase = 0;
short *logCurrent = 0;
short *logMax = 0;
short *logP = 0;

void logger_clear()
{
   int i;
   logCurrent = logBase;
   for (i = 0; i < lockLogNum; i++)
      *logCurrent++ = (short) 0;
   logCurrent = logBase;
}

int logger_init(numb)
{
   logBase = (short *) malloc(numb * sizeof(short));
   if (logBase == 0)
   {
      printf("malloc failed - do NOT use!\n");
      return (0);
   }
   logMax = logBase + numb;
   logP = logBase + numb / 2;
   lockLogNum = numb;
   if (pointsPerShot > 5)
      printf("you are in scan mode??");
   printf("buffer should hold %d seconds\n", numb / (4000));
   logger_clear();
   return (numb);
}

int secret1 = 0;
void loggerStart(int mode)
{
   struct _LLMessage aMessage;
   if (logBase == 0)
   {
      printf("no memory!\n");
      return;
   }
   logCurrent = logBase;
   lockLogCntrl = mode;
   lockLogState = 1;
   if ((mode == 2) && (pMsgesToLL > 0))
   {
      aMessage.action = 1;
      sprintf(aMessage.filen, "/vnmr/tmp/LockL%d.dat", secret1++);
      printf("starting %s for data\n", aMessage.filen);
      msgQSend(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), 0, 0);
   }
   printf("logging ....\n");
}

void loggerStop()
{
   struct _LLMessage aMessage;
   printf("stopped logging\n");
   if ((lockLogCntrl == 2) && (pMsgesToLL > 0))
   {
      aMessage.action = 5;
      printf("closing file in /vnmr/tmp");
      msgQSend(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), 0, 0);
   }
   lockLogCntrl = 0;
}

void logger1(int Reacc, int Imacc)
{
   int i, shift;
   if (logBase == 0)
      return;
   if ((lockLogCntrl == 1) || (lockLogCntrl == 2))
   {
      if (pointsPerShot < 4)
         shift = 2;
      else
         shift = 8;
      *logCurrent++ = (short) (Reacc >> shift);
      *logCurrent++ = (short) (Imacc >> shift);
      if ((logCurrent == logP) && (lockLogCntrl == 2))
      {
         printf("send a message to write buffer 1\n");
         if (LLTid != 0)
            testb1();
      }
      if (logCurrent >= logMax)
      {
         if (lockLogCntrl == 1)
         {
            lockLogCntrl = 0;   /* done - filled one buffer */
            printf("log done\n");
            return;             /* exit */
         }
         if (lockLogCntrl == 2)
         {
            if (LLTid != 0)
               testb2();
            printf("send a message to write buffer 2\n");
            logCurrent = logBase;       /* back to start - circular buffer */
            lockLogState++;
         }
      }
   }
   /* otherwise ignore */
}

printL(int mode)
{
   int i, j;
   short s, *lp;
   lp = logBase;
   if (lockLogState < 2)
      mode = 1;
   if (mode == 0)
   {
      printf("%7d,%2d\n", lockLogNum, mode);
      while (lp < logMax)
         printf("%6d, %6d\n", (int) *lp++, (int) *lp++);
   }
   if (mode == 1)
   {
      printf("%7d,%2d\n", logCurrent - logBase, mode);
      while (lp < logCurrent)
         printf("%6d, %6d\n", (int) *lp++, (int) *lp++);
   }
   if (mode == 2)
   {
      printf("%7d,%2d\n", lockLogNum, mode);
      lp = logCurrent;
      while (lp < logMax)
         printf("%6d, %6d\n", (int) *lp++, (int) *lp++);
      lp = logBase;
      while (lp < logCurrent)
         printf("%6d, %6d\n", (int) *lp++, (int) *lp++);
   }
}

void fileL(char *hostfile)
{
   int fid;
   fid = open(hostfile, 0x202, 0644);
   if (fid == ERROR)
   {
      printf("no file opened");
      return;
   }
   printf("file id = %x\n", fid);
   write(fid, (char *) logBase, (int) (logMax - logBase) * sizeof(short));
   close(fid);
}

void logMe()
{
   char rep;
   int il, num, state, j;
   il = 1;
   state = 0;
   if (logBase == 0)
   {
      if (logger_init(1000000) == 0)
      {
         printf("initialize failed\n");
         return;
      }
   }
   while (state == 0)
   {
      if (il == 1)
      {
         printf("c)lear buffer, l)og one buffer\n");
         printf("r)unning buffer - keep last buffer time, s)top logger\n");
         printf("p)rint from start to last valid, d)ump in order,h)elp\n");
         il = 0;
      }
      printf("::");
      rep = getchar();
      switch (rep)
      {
        case 'c':
           logger_clear();
           printf("cleared\n");
           break;
        case 'l':
           loggerStart(1);
           break;
        case 'r':
           loggerStart(2);
           printf("ring buffer\n");
           break;
        case 's':
           loggerStop();
           break;
        case 'f':
           fileL("/home/vnmr1/log1");
           break;
        case 'p':
           printL(1);
           break;
        case 'd':
           printL(0);
           break;
        case 'h':
           il = 0;
           break;
        case 'x':
        case 'q':
        case 'Q':
        case 'X':
           state = 1;
           break;
      }
   }
}

void tLLogger();

void startLLMon()
{
   if (pMsgesToLL == 0)
      pMsgesToLL = msgQCreate(200, sizeof(struct _LLMessage), 0);
   LLTid = taskSpawn("LockLogger", 200, 0, 40000, (void *) tLLogger, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   if (LLTid == ERROR)
   {
      perror("taskSpawn");
   }
}

void tLLogger()
{
   struct _LLMessage aMessage;
   int fid = 0;
   int stat;

   while (1)
   {
      msgQReceive(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), -1);
      printf("LLogger got message\n");
      if (aMessage.action == 1)
      {
         // open the file for writing (send it a name);
         fid = open(aMessage.filen, 0x202, 0644);
         if (fid < 1)
            printf("file open failed");
      }
      if ((aMessage.action == 2) && (fid > 0))
      {
         close(fid);
         fid = 0;
      }
      if ((aMessage.action == 3) && (fid > 0))
      {
         stat = write(fid, (char *) logBase, (int) (logP - logBase) * sizeof(short));
         printf("status 1 = %d\n", stat);
      }
      if ((aMessage.action == 4) && (fid > 0))
      {
         write(fid, (char *) logP, (int) (logMax - logP) * sizeof(short));
         printf("status 2 = %d\n", stat);
      }
      if ((aMessage.action == 5) && (fid > 0))
      {
         if (logCurrent < logP)
            write(fid, (char *) logBase, (int) (logCurrent - logBase) * sizeof(short));
         else
            write(fid, (char *) logP, (int) (logCurrent - logP) * sizeof(short));
         close(fid);
         printf("closing\n");
         fid = 0;
      }
   }
}

openfile()
{
   struct _LLMessage aMessage;
   aMessage.action = 1;
   strcpy(aMessage.filen, "/home/vnmr1/myLogPhil");
   msgQSend(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), 0, 0);
}

closefile()
{
   struct _LLMessage aMessage;
   aMessage.action = 2;
   strcpy(aMessage.filen, "");
   msgQSend(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), 0, 0);
}

testb1()
{
   struct _LLMessage aMessage;
   aMessage.action = 3;
   strcpy(aMessage.filen, "");
   msgQSend(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), 0, 0);
}

testb2()
{
   struct _LLMessage aMessage;
   aMessage.action = 4;
   strcpy(aMessage.filen, "");
   msgQSend(pMsgesToLL, (char *) &aMessage, sizeof(aMessage), 0, 0);
}


/***************************************************/
/* Lock data management task..  
   filters in 2KHz lock mode 
   copies in big block slow search/adjust modes 
   Phil Hornung
   May 7, 2004
*/
void LockMon()
{
   volatile unsigned int *pntr, *tpntr, i;
   volatile short *spntr;
   int AbsAcc;
   int DisAcc;
   struct _LockMessage aMessage;
   while (1)
   {
      msgQReceive(pMsgesToLockMon, (char *) &aMessage, sizeof(aMessage), -1);
      if (aMessage.action == 1)
      {
         tpntr = 0;
         if (aMessage.status == 0x8)
         {
            tpntr = XBuffer0;
            /* wvEvent(111,0,0); */
            pntr = (unsigned int *) p2Buffer0;
            DPRINT2(4,"LockMon buffer 0@0x%x %x\n",p2Buffer0,aMessage.arg1);
         }
         if (aMessage.status & 0x10)
         {
            tpntr = XBuffer1;
            /* wvEvent(123,0,0); */
            pntr = (unsigned int *) p2Buffer1;
            DPRINT2(4,"LockMon buffer 1@0x%x %x\n",p2Buffer1,aMessage.arg1);
         }
         if (aMessage.status == 0x18)
            printf("LockMon buffer 18 \n");
         if (modeSpecifier >= 1)        /* boxcar re/im */
         {
            AbsAcc = 0;
            DisAcc = 0;
            spntr = (short *) pntr;
            for (i = 0; i < pointsPerShot; i++)
            {
               AbsAcc += (int) *spntr++;
               DisAcc += (int) *spntr++;
            }
            logger1(AbsAcc, DisAcc);
            if (modeSpecifier < 2)
            {
               /* average per shot */
               /* boxcar on the fly */

               AbsRollAve += AbsorptionModeBoxcar[LiveIndex];
               DisRollAve += DispersionModeBoxcar[LiveIndex];
               AbsorptionModeBoxcar[LiveIndex] = AbsAcc / pointsPerShot;
               DispersionModeBoxcar[LiveIndex] = DisAcc / pointsPerShot;
               AbsRollAve -= AbsAcc / pointsPerShot;
               DisRollAve -= DisAcc / pointsPerShot;
               LiveIndex++;
               /* don't run outta buffer */
               if (LiveIndex >= BoxCarSize)
                  LiveIndex = 0;
               /* update the structure */
               AbsorptionAverage = AbsRollAve / BoxCarSize;
               DispersionAverage = DisRollAve / BoxCarSize;
            }
            else
            {
               /* At 2 kHz the filters average the locklevel between
                * rcvr on/off, when 20 Hz, only when receiver on. 
                * This gives a lock level change, so we multiply by 44/50
                * The receiver is off 6ms, on 44ms
                */
               AbsorptionAverage = (-AbsAcc * 44) / (50 * pointsPerShot);
               DispersionAverage = (-DisAcc * 44) / (50 * pointsPerShot);
            }
            count++;
            if (count > countLimit)
            {
               count = 0;
               if (pLkStatIssue != NULL)
               {
                  if (AbsorptionAverage > 3100)
                  {
                     pLkStatIssue->locked = 1;
                     *pLOCK_Locked = 1;
                  }
                  else
                  {
                     pLkStatIssue->locked = 0;
                     *pLOCK_Locked = 0;
                  }
               }

               AbsAcc = (int) (AbsorptionAverage / 16.0);
               DisAcc = (int) (DispersionAverage / 16.0);
               pubLockLevel(AbsAcc, DisAcc, 0, 0);
            }
         }
         /* we copy int's even though they are pairs of shorts */
         if (modeSpecifier == 2)
         {
            pubLockFID((short *) (pntr), pointsPerShot);
         }
      }
      else
      {
         printf("BOGUS\n");
      }
   }
}

int LockTid = 0;

lkDisp()
{
   nerk = 1;
   modeSpecifier = 2;
}

void startLockMon(int priority, int taskOptions, int stackSize)
{
   AbsRollAve = DisRollAve = 0;
   if (pMsgesToLockMon == 0)
      pMsgesToLockMon = msgQCreate(200, sizeof(struct _LockMessage), 0);
   LockTid = taskSpawn("LockMon", priority, taskOptions, stackSize, (void *) LockMon, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   if (LockTid == ERROR)
   {
      perror("taskSpawn");
   }
}

void lockISR(unsigned int int_status, int dummy)
{
   int lastActive, mask, bufintr;
   char buffer[256];
   struct _LockMessage aMessage;
   bufintr = mask_value(LOCK, adc_buffer_complete_int_0_status, int_status) |
             mask_value(LOCK, adc_buffer_complete_int_1_status, int_status);

   if (DebugLevel > 7)
     logMsg("lock ISR: pending: 0x%lx, buffer int: 0x%x\n",int_status,bufintr,3,4,5,6);

   if (bufintr == 0)
      return;

   lastActive = get_field(LOCK, sample_block_select);
   if (int_status == 0x10)
   {
      panelLedOff(5);
      panelLedOn(6);
   }
   else
   {
      panelLedOff(6);
      panelLedOn(5);
   }

   if (nerk == 1)
   {
      aMessage.action = 1;
      aMessage.status = int_status;
      aMessage.arg1 = mask;
      msgQSend(pMsgesToLockMon, (char *) &aMessage, sizeof(aMessage), 0, 0);
   }
   else
   {
      strcpy(buffer, "buffer ");
      if (int_status & 0x8)
      {
         strcat(buffer, msg1);
      }
      if (int_status & 0x10)
      {
         strcat(buffer, msg2);
      }
      DPRINT3(3, "%s  %x  %x\n", buffer, int_status, lastActive);
   }
}

int isrIL = 0;
/* revise theory .. if the pulser is ON, the initialization screws up */
/*  great care applies here ..  
    shut off pulser..
    enable interrupts - then clear them!  */

void initIntr(int slice)
{
   int state, mask;
   /* initial the FPGA Base ISR */
   initFpgaBaseISR(); /* :TODO: remove call - already handled in BringUp!? */

   /* be sure pulser is off */
   state = get_field(LOCK, seq_gen_enable);
   if (state != 0)
   {
      pulser_off();
      taskDelay(calcSysClkTicks(100));  /* 100 ms, taskDelay(6); */
   }
   /* register ADC buffer ISR to FPGA Base ISR */
   fpgaSliceIntConnect(slice, lockISR, 0, NO_MASKING);

   // :TODO: replace the code below with
   //   set_field(LOCK,adc_buffer_complete_int_0_enable,1);
   //   set_field(LOCK,adc_buffer_complete_int_1_enable,1);
   //   set_field(LOCK,adc_buffer_complete_int_0_enable,0);
   //   set_field(LOCK,adc_buffer_complete_int_1_enable,0);
   //   set_field(LOCK,adc_buffer_complete_int_0_enable,1);
   //   set_field(LOCK,adc_buffer_complete_int_1_enable,1);
   //   set_field(LOCK,adc_buffer_complete_int_0_enable,0);
   //   set_field(LOCK,adc_buffer_complete_int_1_enable,0);
   /* enable both buffer0 & 1 interrupts */
   *REG_ADDR(LOCK_InterruptEnable) = 0x18;

   /* clear interrupts */
   *REG_ADDR(LOCK_InterruptClear) = 0;
   *REG_ADDR(LOCK_InterruptClear) = 0x18;
   *REG_ADDR(LOCK_InterruptClear) = 0;
}

bufsIntOn()
{
   /* enable both buffer0 & 1 interrupts */
   *REG_ADDR(LOCK_InterruptEnable) = 0x18;
}

bufsIntOff()
{
   /* disable both buffer0 & 1 interrupts */
   *REG_ADDR(LOCK_InterruptEnable) = *REG_ADDR(LOCK_InterruptEnable) & ~(0x18);
}


void printADCBuffer(int which, int many)
{
   short *pntr;
   int i;
   if (which == 0)
      pntr = (short *) p2Buffer0;
   else
      pntr = (short *) p2Buffer1;
   for (i = 0; i < many; i++)
      printf("%8x  %8x\n", *(pntr + 2 * i), *(pntr + 2 * i + 1));
}

union TypeTransformer
{
   unsigned long long lword;
   unsigned int iword[2];
   unsigned char cword[8];
   double dword;
};

double prevFreq = 0;
void setLockDDS(double freq)
{
   union ltw test;
   int i;
   unsigned char t;
   volatile unsigned char *cpntr;
   if (freq == prevFreq)
      return;                   //don't program unless changed, phase changes
   prevFreq = freq;
   test.ldur = (long long) freq *3518437.2089L;
   printf("%llx ..\n", test.ldur);
   cpntr = (unsigned char *) (ddsprogram + 4);
   for (i = 0; i < 6; i++)
   {
      *cpntr++ = test.cdur[2 + i];
   }
   cpntr = (unsigned char *) (ddsprogram + 0x1d);
   /*  *cpntr = 0x24140100; */
   *cpntr++ = 0x14;
   *cpntr++ = 0x24;
   *cpntr++ = 0x01;
   *cpntr = 0x00;
   if (pLkStatIssue != NULL)
      pLkStatIssue->lkfreq = freq;
   pubLkStat();
}

void setDLockFreq(int a, int b)
{
   union TypeTransformer Freq;
   Freq.iword[0] = a;
   Freq.iword[1] = b;
   setLockDDS(Freq.dword);
}

void dumpDDS()
{
   volatile unsigned char *cpntr;
   int i;
/*  cpntr = (volatile unsigned char *) 0x70020000; */
   cpntr = ddsprogram;
   for (i = 0; i < 0x21; i++)
      printf("%x  %x\n", i, *(cpntr + i));
}

void DDSup()
{
   set_field(LOCK, dds_reset, 1);
}

void DDSdown()
{
   set_field(LOCK, dds_reset, 0);
}

void start()
{
   startLockMon(120, 0, 12000);
   startLLMon();
}

static int lktoggle = 0;
static int lkfidtoggle = 0;
pubLockLevel(levelR, levelI, slopeR, slopeI)
{
   if (pLkStatIssue != NULL)
   {
      pLkStatIssue->lkLevelR = levelR;
      pLkStatIssue->lkLevelI = levelI;
      pLkStatIssue->lkSlopeR = slopeR;
      pLkStatIssue->lkSlopeI = slopeI;
      pubLkStat();
      /* DPRINT1(-1,"pubLockLevel: tog: %d\n",lktoggle); */
      if (lktoggle)
      {
         panelLedOn(3);
         lktoggle = 0;
      }
      else
      {
         panelLedOff(3);
         lktoggle = 1;
      }
   }
}

pubLockFID(short *pFid, int sizeInShorts)
{
   if (pLkFIDIssue != NULL)
   {
#ifndef RTI_NDDS_4x
      pLkFIDIssue->lkfid.len = sizeInShorts;
      pLkFIDIssue->lkfid.val = pFid;
      pubLkFID();
#else /* RTI_NDDS_4x */
      pubLkFID(pFid, sizeInShorts);
#endif /* RTI_NDDS_4x */
   }
}

/*============================================================================*/
/*
 * all board type specific initializations done here. 
 * PFG+Lock combo board calls initLock separately.
 */
initLock(int slice)
{
   resetDDS();
   resetADC();
   enableADC();
   initIntr(slice);
   setLockPower(60);
   setLockDDS(7678000.0);
   setLockGain(48);
   lockRate(1.0,12.0);
   pulser_on(); 
   nerk=1;
   startLockStatPub(120, STD_TASKOPTIONS, STD_STACKSIZE);
   startLockParser(LKPARSER_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
   startLockMon(120,0,4000);
   initialLockComm();
}
