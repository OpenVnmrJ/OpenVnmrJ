/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "errLogLib.h"
#include "msgQLib.h"
#include "hostAcqStructs.h"
#include "chanLib.h"
#include "commfuncs.h"
#include "expentrystructs.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "acqerrmsges.h"
#include "config.h"
#include "prochandler.h"
#include "shrstatinfo.h"
#include "acqcmds.h"

#define  DELIMITER_1	' '
#define  DELIMITER_2	'\n'
#define  DELIMITER_3	','

#define MAXPATHL	128

#ifndef DEBUG_STATBLK
#define DEBUG_STATBLK	(9)
#endif

#ifndef DEBUG_AUTO_SHIM_DOWNLOAD
#define DEBUG_AUTO_SHIM_DOWNLOAD	(9)
#endif

int MasSpinAct = -1;
int MasSpinSpan = -1;
int MasSpinAdj = -1;
int MasSpinMax = -1;
int MasSpinSet = -1;
int MasSpinProfile = -1;
int MasSpinSpeedLimit = -1;
int MasSpinActSp = -1;
char MasprobeId[128] = "";

static char errormsgebuf[128]; /* use by getAcqErrMsge() via acqerrmsges.h */

extern int  SystemVersionId;  /* System Version Id, compared with Console's */

extern ExpEntryInfo ActiveExpInfo;

extern void sendRoboCommand(char *cmd);

char *getAcqErrMsge( int wcode );


locateConfFile( char *confFile )
{
	strcpy( confFile, getenv("vnmrsystem") );
	if (strlen( confFile ) < (size_t) 1)
	  strcpy( confFile, "/vnmr" );
	strcat( confFile, "/acqqueue/acq.conf" );

	return( 0 );
}

static void
locateCdbFile( char *cdbFile )
{
	strcpy( cdbFile, getenv("vnmrsystem") );
	if (strlen( cdbFile ) < (size_t) 1)
	  strcpy( cdbFile, "/vnmr" );
	strcat( cdbFile, "/acqqueue/console.debug" );
}

static void
locateCurrentShims( char *currentShimsFile )
{
	strcpy( currentShimsFile, getenv("vnmrsystem") );
	if (strlen( currentShimsFile ) < (size_t) 1)
	  strcpy( currentShimsFile, "/vnmr" );
	strcat( currentShimsFile, "/acqqueue/currentShimValues" );
}

static int
receiveConsoleDebugBlock( long *data )
{
	int	cdb_fd, ival;
	char	file[MAXPATHL];

	locateCdbFile( &file[ 0 ] );
	cdb_fd = open( &file[ 0 ], O_CREAT | O_TRUNC | O_RDWR, 0666 );
	if (cdb_fd <0) {
		errLogRet(LOGOPT,debugInfo, "Cannot open %s\n", &file[ 0 ]);
		close( cdb_fd );
		return( -1 );
	}

	ival = write( cdb_fd, data, sizeof( CDB_BLOCK ) - sizeof( long ) );
	close( cdb_fd );

	return( ival );
}

static void
updateCurrentShims()
{
	char	file[MAXPATHL];

	locateCurrentShims( &file[ 0 ] );
	writeConsolseStatusBlock( &file[ 0 ] );
}

/*  value for SHIMS_PER_CMD is derived from the limit of
    256 chars in individual messages sent to a message queue	*/

#define SHIMS_PER_CMD	(16)

/*  Download shims at console bootup by first specifying the shim set,
    as recorded in the shim stat block.  The download values for all
    64 shims.  If the hardware does not support a particular shim, the
    console will ignore an attempt to set that shim.  02/1997		*/

static void
downloadShims()
{
	char		file[ MAXPATHL ], setOneValue[ 16 ], delimiter_3[ 2 ];
	char		preambleCmd[ 20 ];
	char		downloadCmd[ CONSOLE_MSGE_SIZE ];
	int		fp, rval, shimIndex, tokenCount;
	CONSOLE_STATUS	savedShimValues;

	locateCurrentShims( &file[ 0 ] );
	fp = open( &file[ 0 ], O_RDONLY );
	if (fp < 0)
	  return;

	rval = read( fp, (char *) &savedShimValues, sizeof( savedShimValues ));
	close( fp );
	if (rval != sizeof( savedShimValues ))
	  return;

	delimiter_3[ 0 ] = DELIMITER_3;
	delimiter_3[ 1 ] = '\0';

	sprintf( &preambleCmd[ 0 ], "%d%c", XPARSER, DELIMITER_2 );
	strcpy( &downloadCmd[ 0 ], &preambleCmd[ 0 ] );
	sprintf( &setOneValue[ 0 ], "2%c %d%c %d",
    DELIMITER_3, SHIMSET, DELIMITER_3, savedShimValues.AcqShimSet
	);
	strcat( &downloadCmd[ 0 ], &setOneValue[ 0 ] );

	writeChannel( chanId, (char *) &downloadCmd[ 0 ], CONSOLE_MSGE_SIZE );
	DPRINT1( DEBUG_AUTO_SHIM_DOWNLOAD, "shimset command:\n%s\n", &downloadCmd[ 0 ] );
	downloadCmd[ 0 ] = '\0';

	for (shimIndex = 0; shimIndex < MAX_SHIMS_CONFIGURED; shimIndex++) {
		if (strlen( &downloadCmd[ 0 ] ) < (size_t) 1) {
			if (MAX_SHIMS_CONFIGURED - shimIndex > SHIMS_PER_CMD)
			  tokenCount = SHIMS_PER_CMD * 3;
			else
			  tokenCount = (MAX_SHIMS_CONFIGURED - shimIndex) * 3;
			strcpy( &downloadCmd[ 0 ], &preambleCmd[ 0 ] );
			sprintf( &setOneValue[ 0 ], "%d%c", tokenCount, DELIMITER_3 );
			strcat( &downloadCmd[ 0 ], &setOneValue[ 0 ] );
		}
		else
		  strcat( &downloadCmd[ 0 ], &delimiter_3[ 0 ] );

		sprintf( &setOneValue[ 0 ], "%d%c %d%c %d",
	    SHIMDAC, DELIMITER_3, shimIndex, DELIMITER_3, savedShimValues.AcqShimValues[ shimIndex ]
		);
		strcat( &downloadCmd[ 0 ], &setOneValue[ 0 ] );
		if (shimIndex % SHIMS_PER_CMD == SHIMS_PER_CMD - 1) {
			writeChannel( chanId, (char *) &downloadCmd[ 0 ], CONSOLE_MSGE_SIZE );
			DPRINT1( DEBUG_AUTO_SHIM_DOWNLOAD, "set shims command:\n%s\n", &downloadCmd[ 0 ] );
			downloadCmd[ 0 ] = '\0';
		}
	}
}

/*  The following define causes the name of the DSP processor
    to be encoded in the name of the DSP download file.        */

#define DSP_DOWNLOAD_FILE	"tms320dsp.ram"

/**************************************************************
*
*  processChanMsge - Routine envoked to read message Q and call parser
*
*   This Function is the Non-interrupt function called to handle the
*   Async Channel interrupt (SIGIO) as register via registerChannelAsync() call
*
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/6/94
*/
void processChanMsge(int readChanId)
{
  int   rtn,stat, shimsChanged;
  long  cmd;
  long *data;
  long  consoleMsge[ CONSOLE_MSGE_SIZE/sizeof( long ) ];

 /* Keep reading the Msg Q until no further Messages */

  do {
       /* read Async Channel don't block if nothing there 
        *   returns:  0 if channel close 
	*             -1 if not successful, with `errno' set
	*                EWOULDBLOCK  no data pending for this channel
        */
       rtn = readChannelNonblocking( readChanId, &consoleMsge[ 0 ], CONSOLE_MSGE_SIZE );
       /* if we got a message then go ahead and parse it */
       if (rtn > 0)
       {
         DPRINT2(3, "received %d bytes, command is %d\n", rtn, consoleMsge[ 0 ]);
         cmd = ntohl(consoleMsge[ 0 ]);
	 data = &consoleMsge[ 1 ];

         switch(cmd)
         {
           case PARSE:
         		parser( (char *) data );
			break;
           case CASE:
			DPRINT(2,"acqHandler\n");
         		acqHandler( data );
			break;

		/* CAUTION:  getStatAcqCtCnt and getStatCT get CT from radically
			     different places.  Do not confuse the two.		*/

	   case STATBLK:
                   {
                      CONSOLE_STATUS *csbPtr;
			DPRINT(DEBUG_STATBLK, 
		 "Status Block received from Console\n"
			);

		/*  It was requestsed that the shims only be written out when
		    the shims themselves change.  Previously any change to the
		    stat block (e.g. lock level) caused a new set of shims to
		    be written out.  As a side-effect, receiveConsoleStatusBlock
		    updates the in-memory stat block, so that afterwards
		    compareShimsConsoleStatusBlock would show no change,
		    regardless of whether a change had occurred.   July 1997  */

                        csbPtr = (CONSOLE_STATUS *) data;
#ifdef LINUX
                        CSB_CONVERT(csbPtr);
#endif

                        if (MasSpinAct != -1)
                        {
                           csbPtr->AcqSpinAct = MasSpinAct;
                           csbPtr->AcqSpinSpan = MasSpinSpan;
                           csbPtr->AcqSpinAdj = MasSpinAdj;
                           csbPtr->AcqSpinMax = MasSpinMax;
                           csbPtr->AcqSpinSet = MasSpinSet;
                           csbPtr->AcqSpinProfile = MasSpinProfile;
                           csbPtr->AcqSpinSpeedLimit = MasSpinSpeedLimit;
                           csbPtr->AcqSpinActSp =  MasSpinActSp;
                           strcpy(csbPtr->probeId1, MasprobeId);
			   DPRINT(1, "MAS SpinPars updated\n");
                        }
			shimsChanged = compareShimsConsoleStatusBlock( (CONSOLE_STATUS *) data );
			stat = receiveConsoleStatusBlock( data );
			DPRINT1(DEBUG_STATBLK,
		 "case STATBLK: statblock memcmp = %d\n",stat
			);
			if (stat != 0 && ActiveExpInfo.ShrExpInfo != NULL &&
					ACQ_ACQUIRE == getStatAcqState()) {
				long		ctCnt;

			  /*  The CT counter is now kept by the console;
			      Expproc just transfers the value so Infoproc
			      and Acqstat can get at it.  July 1997	*/

				ctCnt = getStatAcqCtCnt();
				setStatCT((unsigned long) ctCnt);
			}

			if (stat != 0) {
			   int sample;

		    /* The Gilson sample preparation system can cause
		       the sample value to take on large values.  Keep
		       this value in the range [0, 999].    July 1997	*/

			   sample = getStatAcqSample();
			   sample %= 1000;
			   setStatAcqSample( sample );

			   sigInfoproc();  /* signal Infoproc to check status */
			   if (shimsChanged)
			     DPRINT( DEBUG_STATBLK, "shims changed\n" );
			   if (shimsChanged)
			     updateCurrentShims();
			}
                       }

			break;

/*  Send a reply to TUNE_UPDATE to the monitor if it is OK for
    the console to enter tune mode.  Reuse the console message
    for this purpose.  Messages for the console are character
    strings.  Send no message if it is not OK to start tune.
    The console task will timeout.				*/

	   case TUNE_UPDATE:
			DPRINT( 1, "received tune update from the console\n" );
			if ( !isThereInteractiveAccess() && !isExpActive()) {
			   sprintf( (char *) &consoleMsge[ 0 ], "%d%c", OK2TUNE, DELIMITER_2 );
			   writeChannel( chanId, (char *) &consoleMsge[ 0 ], CONSOLE_MSGE_SIZE );
			   setStatAcqState( ACQ_TUNING );
			}
			break;

           case CONF_INFO:
		{ FILE *conf_fd;
		  char	file[MAXPATHL];
		  int	freq;
                  int   index;
		  struct _hw_config	*conf;

		   conf = (struct _hw_config *)data;
#ifdef LINUX
                   HWCONF_CONVERT(conf);
#endif
		   locateConfFile( &file[ 0 ] );
		   conf_fd = fopen( &file[ 0 ], "w");
		   if (conf_fd == (FILE *)NULL)
		   {  errLogRet(LOGOPT,debugInfo,
			"Cannot open %s\n", &file[ 0 ]);
		      fclose( conf_fd );
		   }
		   else
		   {  if ( fwrite(data,sizeof(struct _hw_config),1,
				conf_fd) != 1)
		      {  errLogRet(LOGOPT,debugInfo,
                               "Trouble writing to  %s\n", &file[ 0 ]);
		      }
		      fclose( conf_fd );
		   }
		   if (conf->valid_struct  == CONFIG_VALID)
		   {  
		      if (conf->SystemVer != SystemVersionId)
		      {
                        wallMsge("System Version Clash,  Host Ver: %d, Console Ver: %d ",
              			SystemVersionId,(int)(conf->SystemVer));
    			ShutDownProc();  /* tiddy up, any loose ends */
    			exit(1);
		      }
		      setSystemVerId((int)(conf->SystemVer));
		      setInterpVerId((int)(conf->InterpVer));
		      freq = (conf->H1freq >> 4) * 100;
		      if (freq == 700) freq=750;
                      wallMsge("Acquisition Console at %d MHz Ready.", freq);


		      for ( index = 0; index < conf->STM_present; index++)
                      {
                         void *dspDwnLdAddr;
			 dspDwnLdAddr = htonl((long) conf->dspDownLoadAddrs[index]);

		         /* if ((conf->dspDownLoadAddr != (void *) 0xFFFFFFFF) &&
		             (conf->dspDownLoadAddr != NULL))
		         */
			 /* errLogRet( LOGOPT, debugInfo, "STM[%d] dwnld addr: 0x%lx\n",index, dspDwnLdAddr); */
                         /* if ((dspDwnLdAddr != (void *) 0x01FF01FF) && (dspDwnLdAddr != NULL)) */
                         if ((dspDwnLdAddr != (void *) 0xFFFFFFFF) && (dspDwnLdAddr != NULL))
		         {
			   char msg4expproc[ MAXPATHL ];

			   /*errLogRet( LOGOPT, debugInfo, "download stuff to DSP\n" );*/
			   sprintf( &msg4expproc[ 0 ], "downloadDSP  \n%s \n0x%lx", DSP_DOWNLOAD_FILE,dspDwnLdAddr );
			   /* errLogRet( LOGOPT, debugInfo, "downld msge : '%s'\n",msg4expproc); */
			   deliverMessage( "Expproc", &msg4expproc[ 0 ] );
		         }
		         /*else
			   errLogRet( LOGOPT, debugInfo, "do not download stuff to DSP\n" );*/
                      }

		   }
		   else
		   {
                      wallMsge("Acquisition Console Ready.");
		   }
                   downloadShims();
		}
		break;

           case CDB_INFO:
                {
#ifdef LINUX
                   CONSOLE_STATUS *csbPtr;
                   csbPtr = (CONSOLE_STATUS *) data;
                   CSB_CONVERT(csbPtr);
#endif

		stat = receiveConsoleDebugBlock( data );
                }
		break;

	   default: 
			errLogRet(LOGOPT,debugInfo,
					"default functionality %d not implemented\n", cmd );
			break;
         }
       }
       else if (rtn == 0) {
	break;	/* Oh oh, channel has been closed on me */
       }
     }
     while(rtn != -1);       /* if no message continue on */
         
  return;
}

acqHandler(int *buffer)
{
  char msgestr[256],ExpN[20],resetMsg[10];
  MSG_Q_ID pVnmrMsgQ;
  int sendProcMsg(int elem, int ctval, int donecode, int errorcode);
  int arg1;

  extern rmPsgFiles(SHR_EXP_INFO ExpInfo);

   arg1 = ntohl( buffer[1] );
   switch( ntohl(buffer[0]))
   {
     /* cases now employed by the present acqprocess in acqproc */
     case PANIC:
                DPRINT(1,"PANIC\n");
		break;
		
     case EXP_CMPLT:
        DPRINT1(0,"EXP_CMPLT %d, should NEVER Happen!!!!!\n",arg1);

#ifdef XXX
     	/* Send Vnmr Done & Error Codes */
        if (isExpActive() == 1)
        {
           sendVnmrCp(EXP_COMPLETE,0,0,0);
     	   rmPsgFiles(ActiveExpInfo.ExpInfo);
	   resetState();
	}
#endif
	break;

     case WARNING_MSG:	/* donecode */
        DPRINT1(1,"WARNING_MSG: %d\n",arg1);
        if (arg1 > (WARNINGS+FIFOMISSING-1) && 
	    arg1 < (WARNINGS+AUTO_NOTBOOTED+1))
        {
	    wallMsge("Expproc: >>>>>  WARNING: %s \n", getAcqErrMsge(arg1) );
        }
        else if (isExpActive() == 1)
        {
           /* Temporarily, report error as if it occurred on fid 1 */
           sendProcMsg(1,1,WARNING_MSG,arg1);
	}
	break;

     case SOFT_ERROR:	/* donecode */
        DPRINT1(1,"SOFT_ERROR: %d\n",arg1);
        if (isExpActive() == 1)
        {
           /* Temporarily, report error as if it occurred on fid 1 */
           sendProcMsg(1,1,SOFT_ERROR,arg1);
	}
	break;

/*
	Send a reset to Sendproc, Recvproc, Procproc
        Send donecode & errcode to Vnmr
*/
     case HARD_ERROR:	/* donecode */
	DPRINT1(1,"HARD_ERROR: %d\n",arg1);

        if (isExpActive() == 1)
        {
           /* resetProcs();*//* send reset msge to Sendproc,RecvProc, & Procproc */
           /* sendVnmrCp(HARD_ERROR,arg1,0,0); */

	   /* abortSendprocXfer(); */
	   ActiveExpInfo.ExpInfo->ExpState = EXPSTATE_HARDERROR;

     	   rmPsgFiles(ActiveExpInfo.ExpInfo);
	   /* resetState();   done by SYSTEM_READY */
	}
        break;

     case EXP_ABORTED:	/* donecode */
        DPRINT(1,"EXP_ABORTED\n");


        if (isExpActive() == 1)
        {
           /* resetProcs();*/  /* send reset msge to Sendproc,RecvProc, & Procproc */
           /* sendVnmrCp(EXP_ABORTED,arg1,0,0); */
	   /* abortSendprocXfer(); */
	   ActiveExpInfo.ExpInfo->ExpState = EXPSTATE_ABORTED;

     	   rmPsgFiles(ActiveExpInfo.ExpInfo);
	   /* resetState();   done by SYSTEM_READY */
        }
	break;

     case EXP_HALTED:	/* donecode */
        DPRINT(1,"EXP_HALTED\n");
        if (isExpActive() == 1)
        {
          /* sendVnmrCp(EXP_ABORTED,arg1,0,0); */

	  /* abortSendprocXfer(); */
	   ActiveExpInfo.ExpInfo->ExpState = EXPSTATE_HALTED;

     	  rmPsgFiles(ActiveExpInfo.ExpInfo);
	  /* resetState();   done by SYSTEM_READY */
        }
	break;

     case EXP_STOPPED:
     case STOP_CMPLT:	/* donecode */
        DPRINT(1,"EXP_STOPPED\n");
        if (isExpActive() == 1)
        {
          /* sendVnmrCp(STOP_CMPLT,arg1,0,0); */

	  /* abortSendprocXfer(); */
	   ActiveExpInfo.ExpInfo->ExpState = EXPSTATE_STOPPED;

     	  rmPsgFiles(ActiveExpInfo.ExpInfo);
	  /* resetState();   done by SYSTEM_READY */
        }
	break;

     case SETUP_CMPLT:
	DPRINT(1,"SETUP_CMPLT\n");
        if (isExpActive() == 1)
        {
          /* sendVnmrCp(SETUP_CMPLT,arg1,0,0); */
     	  rmPsgFiles(ActiveExpInfo.ExpInfo);
	  resetState();
	  chkExpQ(" ");
        }
	break;

        /* received after ABORT, HALT,Etc.. to infor system is read for GO,etc.. */
    case SYSTEM_READY:
	DPRINT(1,"SYSTEM_READY\n");
        if (isExpActive() == 1)
        {
	  resetState();
	  chkExpQ(" ");  /* even though Proproc sends a chkQ msge to Expproc, 
			    the Exp. can be seen as still busy until the SYSTEM_READY
			    has been received (e.g. from an User Abort,etc..) 
			    This fixes that problem  2-12-96 GMB
			 */
        }
        break;
     
     /* -----------------  New --------------------------- */
     case GET_SAMPLE:
        {
	     char robocmd[256]; 

             DPRINT1(1,"GET_SAMPLE: %d\n",arg1);
             sprintf(robocmd,"getsmp %d",arg1);
	     roboCmd(robocmd);
        }
		break;

     case LOAD_SAMPLE:
        {
	     char robocmd[256]; 

             DPRINT1(1,"LOAD_SAMPLE: %d\n",arg1);
	     sprintf(robocmd,"putsmp %d\n",arg1);
	     roboCmd(robocmd);
        }
		break;

     case FAILPUT_SAMPLE:
        {
	     char robocmd[256]; 

             DPRINT1(1,"FAILPUT_SAMPLE: %d\n",arg1);
	     sprintf(robocmd,"failputsmp %d\n",arg1);
	     roboCmd(robocmd);
        }
		break;
     
     case HEARTBEAT_REPLY:
                DPRINT(2,"HEARTBEAT_REPLY\n");
		setHeartBeat();
		break;

     default:
                DPRINT(1,"default\n");
		break;
   }
}

roboCmd(char *cmd)
{
   DPRINT1(1,"roboCmd: '%s'\n",cmd);
   if ( startATask(ROBOPROC) )     /* if not started then start it */
   {
      /* If just started Roboproc, send command until Roboproc
       * says it is ready.
       */
      sendRoboCommand("roboready");
   }
   sendRoboCommand(cmd);
}

chngSample(char *args)
{
   /* send message to roboproc */
}

expCmplt(char *args)
{
}

/* 
*  resetProcs()
*  Send the reset message to Sendproc, RecvProc, Procproc
*
*/
resetProcs()
{
  char ExpN[20],resetMsg[10];
  MSG_Q_ID pRecvMsgQ; 
  MSG_Q_ID pSendMsgQ;
  MSG_Q_ID pProcMsgQ;
  if (isExpActive() == 1)
  {
   sprintf(resetMsg,"reset");
   pSendMsgQ = openMsgQ("Sendproc");
   if (pSendMsgQ != NULL)
   {
     DPRINT(1,"Send Sendproc: 'reset'\n");
     sendMsgQ(pSendMsgQ,resetMsg,strlen(resetMsg),MSGQ_NORMAL,
		   WAIT_FOREVER);
     closeMsgQ(pSendMsgQ);
   }

/*
   pRecvMsgQ = openMsgQ("Recvproc");
   if (pRecvMsgQ != NULL)
   {
      DPRINT(1,"Send Recvproc: 'reset'\n");
      sendMsgQ(pRecvMsgQ,resetMsg,strlen(resetMsg),MSGQ_NORMAL,
		   WAIT_FOREVER);
      closeMsgQ(pRecvMsgQ);
   }
*/

   pProcMsgQ = openMsgQ("Procproc");
   if (pProcMsgQ != NULL)
   {
      DPRINT(1,"Send Procproc: 'reset'\n");
      sendMsgQ(pProcMsgQ,resetMsg,strlen(resetMsg),MSGQ_NORMAL,
		   WAIT_FOREVER);
      closeMsgQ(pProcMsgQ);
   }
  }
}

sendProcMsg(int elemId, int ctval, int donecode, int errorcode)
{
   char msgestr[256];
   MSG_Q_ID pProcMsgQ;
   if (isExpActive() == 1)
   {
      pProcMsgQ = openMsgQ("Procproc");
      if (pProcMsgQ != NULL)
      {
         sprintf(msgestr,"seterr %s %d %d %d %d\n",
     	                   ActiveExpInfo.ExpId,
     	                   elemId, ctval,
                           donecode,errorcode);
         DPRINT1(1,"Send Procproc: '%s'\n",msgestr);
         sendMsgQ(pProcMsgQ,msgestr,strlen(msgestr),MSGQ_NORMAL,
		   WAIT_FOREVER);
         closeMsgQ(pProcMsgQ);
      }
   }
}


/*-------------------------------------------------------
|  getAcqErrMsge()/1
|   Returns a pointer the Error Message String with a
|   carriage return added.
|                       Author Greg Brissey
+-------------------------------------------------------*/
char *getAcqErrMsge( int wcode )
{
    char tbuf[ 80 ];
    int iter;

    iter = 0;
    errormsgebuf[0] = '\0';
    while (acqerrMsgTable[ iter ].code != 0)
    {
      if (acqerrMsgTable[ iter ].code == wcode)
      {
          if (acqerrMsgTable[ iter ].errmsg == NULL)
          {
              return(errormsgebuf);
          }
          strcpy(errormsgebuf,acqerrMsgTable[ iter ].errmsg);
          strcat(errormsgebuf,"\n");  /* add carriage return */
          return(errormsgebuf);
      }
      else
        iter++;
    }

    sprintf( errormsgebuf, "Code = %d\n", wcode );
    return(errormsgebuf);
}

int masPars(char *args)
{
   char	*tmpstr;

   tmpstr = strtok( NULL, " " );
   MasSpinSpan = atoi(tmpstr);
   tmpstr = strtok( NULL, " " );
   MasSpinAdj = atoi(tmpstr);
   tmpstr = strtok( NULL, " " );
   MasSpinMax = atoi(tmpstr);
   tmpstr = strtok( NULL, " " );
   MasSpinSet = atoi(tmpstr);
   tmpstr = strtok( NULL, " " );
   MasSpinProfile = atoi(tmpstr);
   tmpstr = strtok( NULL, " " );
   MasSpinSpeedLimit = atoi(tmpstr);
   tmpstr = strtok( NULL, " " );
   MasSpinActSp = atoi(tmpstr);
   tmpstr = strtok( NULL, "\n" );
   strcpy(MasprobeId, tmpstr);
   return(0);
}

int masSpeed(char *args)
{
   char	*tmpstr;

   tmpstr = strtok( NULL, "\n" );
   MasSpinAct = atoi(tmpstr);
   return(0);
}
