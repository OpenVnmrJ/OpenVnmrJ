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

#include <msgQLib.h>
#include <string.h>
#include <semLib.h>
#include "logMsgLib.h"
#include "Lock_Cmd.h"
#include "X_interp.h"
#include "acqcmds.h"
#include "Console_Stat.h"
#include "serialShims.h"
#include "master.h"
#include "masterAux.h"
#include "nvhardware.h"
#include "instrWvDefines.h"
#include "spinner.h"
#include "nsr.h"

#ifndef VTOFF
#define VTOFF		30000
#endif

#ifndef NOCMD
#define  NOCMD  	-1
#endif

#ifndef MAXNUMARGS
#define  MAXNUMARGS     30
#endif

#ifndef MAXSHIMS
#define  MAXSHIMS       48
#endif

#define VPSYN(arg) volatile unsigned int *p##arg = (unsigned int *) (FPGA_BASE_ADR + arg)

VPSYN(MASTER_SoftwareGates);

extern Console_Stat	*pCurrentStatBlock;	/* Acqstat-like status block */
extern SEM_ID		pSemOK2Tune;
extern int		globalLocked;
extern int		host_abort;

int			shimType=0;
int			spinnerType = LIQUIDS_SPINNER;  /* default to liquid spinner */
int			auxLockByte = 0;	/* Lock loop starts closed */
union int2double {
   signed int	i[2];
   double	d;
   };
MSG_Q_ID		pMsgesToXParser;

static struct _tcValue 
{
   int	tc;	// fast time constant
   int  acqtc;	// slow time constant
} tcValues;

/* 
static struct {
   int    changecode;
   int    argcount;
   int  (*handler)();
   char  *text;
} Xcmd[] = {
x	{ LKPOWER,	1,	ladcHandler,   "Set lock power" },
x	{ LKGAIN,	1,	ladcHandler,   "Set lock gain"  },
x	{ LKPHASE,	1,	ladcHandler,   "Set lock phase"  },
x	{ LKRATE,	1,	ladcHandler,   "20/2k Hz rep rate" },
x	{ SHIMDAC,	2,	shimHandler,   "Set shim DAC (alias 1)"  },
x	{ SET_DAC,	2,	shimHandler,   "Set shim DAC (alias 2)"  },
x	{ LKMODE,	1,	ladcHandler,   "Lock Mode, on, off or auto" },
x	{ LKTC,		1,	ladcHandler,   "Set lock tc not acquiring" },
x	{ LKACQTC,	1,	ladcHandler,   "Set lock tc for acquiring" },
x	{ BEAROFF,	0,	spnrHandler,   "spinner Bearing air off" },
x	{ BEARON,	0,	spnrHandler,   "spinner Bearing air on" },
	{ SETRATE,	1,	spnrHandler,   "set spinner air rate" },
	{ SETSPD,	1,	spnrHandler,   "set spinner speed" },
x	{ EJECT,	0,	spnrHandler,   "eject sample" },
x	{ INSERT,	0,	spnrHandler,   "insert sample" },
  y	{ GETSTATUS,	0,	statHandler,   "return console status block" },
  y	{ RTN_SHLK,	0,	statHandler,   "return console status block" },
	{ SYNC_FREE,	0,	syncHandler,   "Restart the acode parser" },
	{ WSRAM,	1,	nvRamHandler,  "write NvRam offset by 0x500"},
  y	{ SHIMI,	6,	autshmHandler, "start up autoshim" },
  y	{ STOP_SHIMI,	0,	autshmHandler, "start up autoshim" },
  y	{ RCVRGAIN,	1,	rcvrgainHandler, "set receiver gain" },
	{ FIX_ACODE,	2,	fixAcodeHandler, "modify A-codes" },
	{ SET_ATTN,	2,	setAttnHandler, "for acqi IPA RF attenuation" },
        { SET_TUNE,    14,      tuneHandler,   "for tune command" },
x       { RESET_VT,     0,      spnrHandler,   "Reset VT controller" },
x       { SETLOC,       1,      spnrHandler,   "Set Sample Number" },
x       { SETMASTHRES,  1,      spnrHandler,   "Set MAS Threshold (Hz)" },
x       { BUMPSAMPLE,   0,      spnrHandler,   "bump the sample" },
x       { SETTEMP,      2,      setvtHandler,  "Set temp in VT controller" },
x       { SHIMSET,      1,      shmsetHandler, "Specify type of shims" },
X       { SETSTATUS,    1,      statHandler,   "Set console acq status" },
x        { LKFREQ,       2,      lkfreqHandler, "Specify lock frequency" },
  y	{ STATRATE,	1,	statTimer,     "set statblock update rate" },
	{ GTUNE_PX,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_IX,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_PY,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_IY,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_PZ,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GTUNE_IZ,	1,	gpaHandler,    "set GPA tune parameters" },
	{ GPAENABLE,	1,	gpaHandler,    "set GPA enable/disable (1/0)" },
};

New:      PNEUFAULT,    1,                      "clear/set level for pneumatics"
*/

startXParser(int priority, int taskoptions, int stacksize)
{
   if (pMsgesToXParser == NULL)
      pMsgesToXParser = msgQCreate(100,300,MSG_Q_PRIORITY);
   if (pMsgesToXParser == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,"could not create X Parser MsgQ, ");
      return;
   }
   
   if (taskNameToId("tXParser") == ERROR)
      taskSpawn("tXParser",priority,0,stacksize,Xparser,pMsgesToXParser,
						2,3,4,5,6,7,8,9,10);
}

killXParser()
{
   int tid;
   if ( (tid = taskNameToId("tXParser")) != ERROR)
      taskDelete(tid);
}

cleanXParser()
{
  int tid;
  if (pMsgesToXParser != NULL)
  {
    msgQDelete(pMsgesToXParser);
    pMsgesToXParser = NULL;
  }
  if ( (tid = taskNameToId("tXParser")) != ERROR)
     taskDelete(tid);
}

void Xparser()
{
char	msg[ CONSOLE_MSGE_SIZE ];
int	ival;

   FOREVER {
      ival = msgQReceive(pMsgesToXParser,msg,CONSOLE_MSGE_SIZE,WAIT_FOREVER);
      DPRINT1( 2, "Xparse:  msgQReceive returned %d\n", ival );
      if (ival == ERROR)
      {
         printf("X PARSER Q ERROR\n");
         return;
      }
      else
      {
         X_interp(msg);
      }
   }
}

void X_interp(char *cmdstring)
{
char	*currentTag, *paramptr;
int	paramvec[MAXNUMARGS],index;
int	count;
int	token;
int	tmpInt;
   DPRINT1(-1,"X_interp(): cmdstring='%s'\n",cmdstring);
   paramptr = strtok_r(cmdstring,",",&currentTag);
   count = atoi(paramptr);
   DPRINT1(-1,"X_interp(): count=%d\n",count);
   index=0;
   while ( (currentTag != NULL) && (count > index) )
   {
      paramvec[index] = getNextToken( &currentTag ); index++;
   }
   if (index < count)
   {
      DPRINT2(-1,"fewer tokens (%d) then advertized (%d)",index,count);
   }
   count = index; /* reset to lowest of the two */
   index = 0;
   while (index < count) 
   {
      token = paramvec[index]; 
      DPRINT1(1,"token = %d\n",paramvec[index]);
      switch(token)
      {
      case  BEAROFF:
      case  BEARON:
      case  BEARING:
      case  BUMPSAMPLE:
      case  EJECT:
      case  INSERT:
      case  EJECTOFF:
      case  SETMASTHRES:
      case  SETRATE:
      case  SETSPD:
      case  SETBEARSPAN:
      case  SETBEARADJ:
      case  SETBEARMAX:
      case  SETASP:
      case  SETPROFILE:
      case  MASOFF:
      case  MASON:
          /* exit from autolock or autoshim if sample is ejected */
          if (token == EJECT)
             host_abort = 1;
          spinnerHandler(paramvec, &index, count);
          break; 
      case IPII:
        { int token;
           index++;
           token = paramvec[index++];
           DPRINT(-1, "IPII\n");
           if ( token == -1 )
              sibReset();
           break;
        }
      case LKACQTC:
          index++;
          DPRINT1(-1,"LKACQTC %d\n",paramvec[index]);
          lockacqtc();
          index++;
	  break;
      case LKFREQ:
        { union int2double freq;
          index++;
          freq.i[0] = paramvec[index]; index++;
          freq.i[1] = paramvec[index]; index++;
          DPRINT3(-1,"LKFREQ = %f 0x%x 0x%x\n",freq.d, freq.i[0], freq.i[1]);
          send2Lock(LK_SET_FREQ, freq.i[0], freq.i[1], freq.d, 0.0);
          break;
        }
      case LKGAIN:
	  index++;
          DPRINT1(-1,"LKGAIN %d\n",paramvec[index]);
          setgain(paramvec[index]);
          index++;
          break;
      case LKMODE:
        {
          int maxCount = 0;
	  index++;
          DPRINT1(-1,"LKMODE %d\n",paramvec[index]);
          while ( ((tmpInt = semTake(pSemOK2Tune,NO_WAIT)) != OK) && (maxCount < 10))
          {
             maxCount++;
             taskDelay(calcSysClkTicks(500));
          }
          if (tmpInt == OK)
          {
             if (maxCount)
                DPRINT1(-1,"LKMODE set on try %d\n",maxCount+1);
             setmode(paramvec[index]);
             semGive(pSemOK2Tune); // so we can 'go' again
          }
          index++;
          break;
        }
      case LKPHASE:
	  index++;
	  DPRINT1(-1,"LKPHASE %d\n",paramvec[index]);
          setphase(paramvec[index]);
          index++;
          break;
      case LKPOWER:
	  index++;
	  DPRINT1(-1,"LKPOWER %d\n",paramvec[index]);
          setpower(paramvec[index]);
          index++;
          break;
      case LKRATE:
	  index++;
          DPRINT1(-1,"LKRATE %d\n",paramvec[index]);
          send2Lock(LKRATE, 0, 0, (double)paramvec[index], 0.0);
/*
    For bug 8654 the following four lines fix the problem with acqstate.
    However, calling lock_scan / locki('stop') during an acquisition
    seems to cause the EXP_COMPLETE interrupt to be lost.

          if (paramvec[index] < 1000)
          {
             if (pCurrentStatBlock->Acqstate == ACQ_IDLE)
                pCurrentStatBlock->Acqstate = ACQ_INTERACTIVE;
          }
          else
          {
             if (pCurrentStatBlock->Acqstate == ACQ_INTERACTIVE)
                pCurrentStatBlock->Acqstate = ACQ_IDLE;
          }


 */
          if (paramvec[index] < 1000)
             pCurrentStatBlock->Acqstate = ACQ_INTERACTIVE;
          else
             pCurrentStatBlock->Acqstate = ACQ_IDLE;
          index++;
          break;
      case LKTC:
        { int newBits;
	  index++;
          DPRINT1(-1,"LKTC %d\n",paramvec[index]);
          locktc();
          index++;
          break;
        }
      case PNEUFAULT:
        { int token;
           index++;
           token = paramvec[index++];
           DPRINT(-1, "PNEUFAULT\n");
           if ( token == -1 )
              clearPneuFault();
           else
              setPneuFaultInterlock(token);
           break;
        }
      case PNEUMATICS:
          index++;
          DPRINT(-1, "PNEUMATICS\n");
          setBearingLevel(paramvec[index]);  index++;
          setVTAirFlow(paramvec[index]);  index++;
          setVTAirLimits(paramvec[index]);  index++;
          setSpinnerType(paramvec[index]); index++;
          testAndClearPneuFault();
          break;
      case RESET_VT:
	  index++;
          DPRINT(-1, "RESET_VT\n");
          resetVT();
          break;
      case VTINFO:
        {
          int token;
          index++;
          token = paramvec[index++];
          DPRINT2(-1, "VTINFO %d (vtc %d)\n", token, paramvec[index]);
          setVTInfo(token, paramvec[index]); index++;
          break;
        }
      case SETLOC:
	  index++;
          DPRINT1(-1,"SETLOC %d\n",paramvec[index]);
          pCurrentStatBlock->AcqSample = (long) paramvec[index];
          index++;
          break;
      case SETSTATUS:
	  index++;
          DPRINT1(-1,"SETSTATUS %d\n",paramvec[index]);
          if (paramvec[index])
             setAcqState(paramvec[index]);   /* status update to Vnmrj */
          else
             pCurrentStatBlock->Acqstate = ACQ_INACTIVE;
          index++;
          break;
      case SETTEMP:
          index++;
          DPRINT1(-1,"SETTEMP %d\n",paramvec[index]);
          token = paramvec[index];         index++;
          setVT(token,paramvec[index],-1); index++;
          break;
      case SET_DAC:
      case SHIMDAC:
          shimHandler(paramvec, &index, count, 0);
//          DPRINT2( 1,"returned: tag='%s', count=%d\n", currentTag, count);
          break;
      case SHIMSET:
	  index++;
	  establishShimType(paramvec[index]); index++;
          break;
      case STATRATE:
          index++;
          if ((tmpInt = semTake(pSemOK2Tune,NO_WAIT)) == OK)
          {
             DPRINT(-1,"STATRATE");
             if ( paramvec[index] > 1000) 
                lockacqtc();
             else
                locktc();
             semGive(pSemOK2Tune); // so we can 'go' again
          }
          else
             DPRINT(-1,"No STATRATE");
          index++;
          break;

      case SYNC_FREE:   /* Imaging Prep */
	  index++;
          DPRINT(-1, "Imaging Prep, SYNC_FREE\n");
          givePrepSem();
          break;

      case VTAIRFLOW:
          index++;
          newVTAirFlow(paramvec[index]); index++;
          break;
      case VTAIRLIMITS:
          index++;
          setVTAirLimits(paramvec[index]); index++;
          break;
      case WSRAM:
          DPRINT2(-20, "WSRAM %d pv[2]=%d\n",count, paramvec[2]);
          if (paramvec[2]<0) 
          {
             sibSendDutyCycle(6,&paramvec[3]);
          }
          // else
          // {
          //    writeConfig2FF(&paramvec[1],count);
          // }
          index = count;	//ignore this one completely
          break;
      default:
	  DPRINT1(-1,"%d not supported (yet)\n",paramvec[index]);
          index = count;  /* skip any remaining arguments */
          break;
      }
   }
//   sendConsoleStatus();
}

int getNextToken( char **str )
{
int token;
   token = atoi( *str );
   strtok_r( *str, ",", str );
   return(token);
}

int TomySwitch = 0;

void shimHandler( int *paramvec, int *index, int count , int fifoFlag)
{
   DPRINT( 1,"shimHandler() called\n" );
//   DPRINT2( 1,"index=%d count=%d fifoFlag=%d\n", *index, count, fifoFlag);

#ifdef INSTRUMENT
   wvEvent(EVENT_XPARSER_SHIMHDLR,NULL,NULL);
#endif

   if (shimType == NO_SHIMS)
   {
      determineShimType();
      /*  If it doesn't work, we'll find out in good time... */
   }

/*   shimType = SPI_SHIMS;
/* */
   switch (shimType)
   {
      case NO_SHIMS:
            errLogRet( LOGIT, debugInfo,
                     "shim handler cannot establish type of shims\n");
            (*index) += count;
            break;

      case QSPI_SHIMS:
            qspiShimInterface( paramvec, index, count, fifoFlag);
            break;

      case RRI_SHIMS:
            rriShimInterface( paramvec, index, count, fifoFlag);
            break;

      case SPI_SHIMS:
            spiShimInterface( paramvec, index, count, fifoFlag);
            break;

      case SPI_M_SHIMS:
	    spi1ShimInterface( paramvec, index, count, fifoFlag);
            break;

      case SPI_THIN_SHIMS:
            spi2ShimInterface( paramvec, index, count, fifoFlag);
            break;
      default:
            errLogRet(LOGIT,debugInfo,"unknown shims set %d\n",shimType);
            return;
   }
}

void spinnerHandler(int *paramvec, int *index, int count )
{
   DPRINT(1,"spinnerHandler() called\n" );
   DPRINT2(-1,"index=%d count=%d\n", *index, count);

#ifdef INSTRUMENT
   wvEvent(EVENT_XPARSER_SHIMHDLR,NULL,NULL);
#endif
/* We will have to determine the spinner type some where during
/* the configuration discovery, for now I hard code it
/* Something like...check for FtCollinsSpinner presence (RS232)
/*                  check for PaloAltoSpinner presence (SPI)
/*     if (spinnerType=NO_SPINNER) return # for imagers
/*     if (spinspeed < MASTHRESH) spinnerType=LIQUIDS_SPINNTER
/*     else      if (ftc_present) spinnerType=MAS_SPINNER
/*               else             spinnerType=SOLIDS_SPINNER
/*                  
/* for now it is hard coded below
/* */
   switch (spinnerType)
   {
      case NO_SPINNER:
            errLogRet( LOGIT, debugInfo,
                     "spinnerHandler(): could not find a spinner\n");
            break;

      case SOLIDS_SPINNER:
      case LIQUIDS_SPINNER:
            liquidsSpinner(paramvec, index, count );
            break;

      case MAS_SPINNER:
            masSpinner(paramvec, index, count );
            break;

      default:
            errLogRet(LOGIT,debugInfo,"unknown spinner type %d\n",spinnerType);
            return;
   }
}

void set2sw_fifo(int i)
{
   DPRINT(+1,"set2sw() called\n");
   if (i==0) *pMASTER_SoftwareGates = 0;
   set_register(MASTER,FIFOOutputSelect,i);
}


int auxReadReg(int reg)
{
int i;
  set_field(MASTER,sw_aux,( ((reg&7) << AUX_REG_POS) | AUX_READ_BIT ));
  set_field(MASTER,sw_aux_strobe,1); 
  set_field(MASTER,sw_aux_strobe,0);
  i = get_field(MASTER,aux_read);
  DPRINT1(+1,"auxReadRef read: %x\n",i);
  return(i);
}
 
void auxWriteReg(int reg, int value)
{
int i;
  i = ((reg&7) << AUX_REG_POS) | (value & 0xFF);
  DPRINT1(+1,"auxWriteReg write: %x\n",i);
  set_field(MASTER,sw_aux,(i));
  set_field(MASTER,sw_aux_strobe,1); 
  set_field(MASTER,sw_aux_strobe,0);
}

/*------------------------------*/
/* lock control routines        */
/*------------------------------*/
int chlock()				// Is the system locked?
{
   return(globalLocked);
}

int get_lock_offset()			// return z0 for now
{
   return(pCurrentStatBlock->AcqShimValues[1]);
}

int get_limit_lock()			// more needed
{
int shimset,daclimit;
   shimset=pCurrentStatBlock->AcqShimSet;
   if ( (shimset <=2) || (shimset == 10) )
      daclimit = 2047;
   else
      daclimit = 32767;
   return(daclimit); 
}

int getgain()
{
   return(pCurrentStatBlock->AcqLockGain);
}

int getpower()
{
   return(pCurrentStatBlock->AcqLockPower);
}

int getphase()
{
   return(pCurrentStatBlock->AcqLockPhase);
}

int getmode()
{
   return( ! (auxLockByte & AUX_LOCK_LOCKON) );
}

double get_lkfreq_ap()
{
   return( pCurrentStatBlock->AcqLockFreqAP );
}


void set_lock_offset( int newz0 )
{
int oneDac[5];
int zero;
   zero=0;
   oneDac[0] = 13;
   oneDac[1] = 1;
   oneDac[2] = newz0;
   shimHandler(oneDac,&zero,3,0);   // count=1, fifoFlag=false
}

void set_lk_freq_ap( double newfreq )
{
union int2double freq;
   freq.d = newfreq;
   send2Lock(LK_SET_FREQ, freq.i[0], freq.i[1], freq.d, 0.0);
}

void setgain(int newgain)
{
   send2Lock(LK_SET_GAIN, newgain, 0, 0.0, 0.0);
}

void setpower(int newpower)
{
          if (newpower > 48) {
             /* clear the lock preamp attn bit */
             hsspi(1, ((0xd<<11) | 0x1) );
          }
          else {
             /* set the lock preamp attn bit */
             hsspi(1, ((0xd<<11) | 0x0) );
          }
          /* the lock cntlr will deduct the 20 dB as needed */
          /* it sends the status back, so needs the true value */
          send2Lock(LK_SET_POWER, newpower, 0, 0.0, 0.0);
}

void setphase(int newphase)
{
   send2Lock(LK_SET_PHASE, newphase, 0, 0.0, 0.0);
}

void setmode(int newmode)
{
   set2sw_fifo(0);
   auxLockByte = auxReadReg(AUX_LOCK_REG);
   // newmode=1=on, newmode=0=off
   if ( ! newmode) {
      auxLockByte |= AUX_LOCK_LOCKON;    //set to open loop
   }
   else {
      auxLockByte &= ~AUX_LOCK_LOCKON;   //clear to close loop
   }
   auxWriteReg(AUX_LOCK_REG, auxLockByte);
   set2sw_fifo(1);
}

void lk2kcf()
{
   locktc();					// fast responds
   send2Lock(LKRATE, 0, 0, (double)2000.0, 0.0);// 2 kHz 
}

void lk2kcs()
{
   lockacqtc();					// slow responds
   send2Lock(LKRATE, 0, 0, (double)2000.0, 0.0);// 2 kHz 
}

void lk20Hz()
{
   locktc();					// fast responds
   send2Lock(LKRATE, 0, 0, (double)20.0, 0.0);	// 2 kHz 
}

/*------------------------------*/
/* lock time constant routines  */
/*------------------------------*/
void locktc()
{
   setTc(tcValues.tc);
}

void lockacqtc()
{
   setTc(tcValues.acqtc);
}

setTc(int tc)
{
int tmpInt, newBits;
   set2sw_fifo(0);
   tmpInt = auxReadReg(AUX_LOCK_REG);
   switch( tc & 0x3 )
   {
   case 0:
        newBits=0;
        break;
   case 1:
        newBits = AUX_LOCK_LLF1;
        break;
   case 2:
        newBits = AUX_LOCK_LLF2;
        break;
   case 3:
        newBits = AUX_LOCK_LLF1 + AUX_LOCK_LLF2;
        break;
   }
   tmpInt &= ~(AUX_LOCK_LLF1+AUX_LOCK_LLF2);     //clear loop tc
   tmpInt |= newBits;                            //set new tc
   auxWriteReg(AUX_LOCK_REG, tmpInt);
   set2sw_fifo(1);
}

void saveTcValues( int tcSlow, int tcFast )
{
    tcValues.tc = tcFast & 0x3;
    tcValues.acqtc = tcSlow & 0x3;
}

tcShow()
{
  printf("tcValues.tc    =  %d\n", tcValues.tc);
  printf("tcValues.acqtc =  %d\n", tcValues.acqtc);
}

void lockHoldOff()		// assumes fifo is not running
{
int tmpInt, newBits;
   set2sw_fifo(0);
   tmpInt = auxReadReg(AUX_LOCK_REG);
   tmpInt &= ~AUX_LOCK_HOLD;     //clear hold bit
   auxWriteReg(AUX_LOCK_REG, tmpInt);
   set2sw_fifo(1);
}


/*--------------------------------------------------*/
/*  Support for eject switch on top of upper barrel
/*--------------------------------------------------*/
int sample_where=0;
enable_UB_ISR(int enable)
{
void UB_ISR(void);
   if (enable)  // enable the interrupt
   {
      DPRINT(-1,"Connecting Upper Barrel Eject to interrupt service list\n");
      fpgaIntConnect(UB_ISR,0,1<<MASTER_eject_switch_int_enable_pos);
      set_field(MASTER,eject_switch_int_enable,1);
      set_field(MASTER,eject_switch_int_clear,0);
      set_field(MASTER,eject_switch_int_clear,1);
      sample_where=0;
   }
   else		// disable the interrupt
   {
      DPRINT(-1,"Removing Upper Barrel Eject from interrupt service list\n");
      fpgaIntRemove(UB_ISR,0);
      set_field(MASTER,eject_switch_int_enable,0);
   }
}

UB_ISR()	// eject the sample, NO QUESTIONS ASKED!
{
int out_not_in;
   out_not_in = get_field(MASTER,eject_switch);
   if (out_not_in)
      eject();
   else
      insert();
   sample_where ^= 1;
}

/*--------------------------------------------------*/
/*  Test Stubs
/*--------------------------------------------------*/
insert()
{
char cmd[80];
   sprintf(cmd,"1,%d,",INSERT);
   msgQSend(pMsgesToXParser, cmd, 5, NO_WAIT, MSG_PRI_NORMAL);
}

eject()
{
char cmd[80];
   sprintf(cmd,"1,%d,",EJECT);
   msgQSend(pMsgesToXParser, cmd, 5, NO_WAIT, MSG_PRI_NORMAL);
}

setlkmode(int i)
{
char cmd[80];
   sprintf(cmd,"2,%d,%d,",LKMODE,i);
   X_interp(cmd);
}

setlocktc(int i)
{
char cmd[80];
   sprintf(cmd,"2,%d,%d,",LKTC,i);
   X_interp(cmd);
}


checkShim()
{
char msg[40];
int  len;
int i;
   strcpy(msg,"2, 13, 1, 8000,");
   len = strlen(msg);
   msg[len]='\0';
   msg[len+1]='\0';
   for (i=0; i<1000000; i++)
   {
      msgQSend(pMsgesToXParser, msg, len, NO_WAIT, MSG_PRI_NORMAL);
      taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
   }
}

#include "FFKEYS.h"
fifo2spi(int port, int value)
{
long fWord[30];
   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();
   fWord[0] = (           DURATIONKEY | 8000);
   fWord[1] = (LATCHKEY | (1<<30) | (port<<28) | value);
   fWord[2] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[3] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[4] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[5] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[6] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[7] = (LATCHKEY | DURATIONKEY | 0);
   cntrlFifoPIO(fWord,8);
   startCntrlFifo();	/* Start the fifo running */
   wait4CntrlFifoStop();
   DPRINT(-1,"Fifo Done");
}

fifo2aux(int reg, int value)
{
long fWord[30];
long tmp;

   cntrlFifoReset();
   cntrlFifoCumulativeDurationClear();
   tmp = (LATCHKEY | AUX | (reg<<8) | value);
   DPRINT3(-1,"aux reg %d, value %d, fifo %x\n",reg, value, tmp);
   fWord[0] = tmp;
   fWord[1] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[2] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[3] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[4] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[5] = (LATCHKEY | DURATIONKEY | 0xFFFFFF);
   fWord[6] = (LATCHKEY | DURATIONKEY | 0);
   cntrlFifoPIO(fWord,7);
   startCntrlFifo();	/* Start the fifo running */
   wait4CntrlFifoStop();
   DPRINT(-1,"Fifo Done");
}
