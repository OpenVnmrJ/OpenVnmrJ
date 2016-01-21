/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <tcl.h>

#ifndef GILSCRIPT
#include "shrexpinfo.h"
#include "msgQLib.h"
#include "hrm_errors.h"
#endif

#include "errLogLib.h"
#include "errorcodes.h"
#include "gilsonObj.h"
#include "rackObj.h"
#include "gilfuncs.h"
#include "timerfuncs.h"
#include "iofuncs.h"

#define ZONE1 1
#define ZONE2 2
#define ZONE3 3

#define NOSEEK 0xffff
#define IN    1
#define OUT   2

extern GILSONOBJ_ID pGilObjId;

extern G215_BED_OBJ g215Bed;

int AbortRobo = 0;       /* SIGUSR2 will set this to abort sample change */

static Tcl_Interp *TclInterp = NULL;

static char TclResultMsges[256];

static char *ErrMsge;

static char tclErrCode[12];

static double VolumeInSyringe = 0.0;

/* the last sample move2Sample() performed */
static int lastSample = 0;
static int lastZone = 0;
static RACKOBJ_ID pLastRack;
static double lastVolume;
static double lastFlowRate;
static double lastZspeed;
static double lastVol2ZTravel;

extern char SampleInfoFile[512];   /* sourced in by tcl scripts */
extern int rackDelta[GIL_MAX_RACKS][2];

/*
 * If debug set to non zero value in TCL script 
 *   then gPuts output will appear
 */
static int DebugOutput = 1;

extern int gilGetInputs(GILSONOBJ_ID pGilId,             /* gilsonObj.c     */
                        int bits, int *on_off);
extern int gilGetZ(GILSONOBJ_ID pGilId, int *Zmm);       /* gilsonObj.c     */
extern int gilGetXY(GILSONOBJ_ID pGilId, int *XYmm);     /* gilsonObj.c     */
extern int gilSelectUnit(GILSONOBJ_ID pGilId, int unit); /* gilsonObj.c     */
extern int gilSelectPump(GILSONOBJ_ID pGilId, int unit); /* gilsonObj.c     */
extern int gilSelectInj(GILSONOBJ_ID pGilId, int unit);  /* gilsonObj.c     */
extern int gilInitSyringe(GILSONOBJ_ID pGilId, int size);/* gilsonObj.c     */
extern int gilHomeDiluter(GILSONOBJ_ID pGilId);          /* gilsonObj.c     */
extern int gilCommand(GILSONOBJ_ID pGilId, int unit,     /* gilsonObj.c     */
                      char *cmd, char *type, char *rsp);
extern int rackCenter(RACKOBJ_ID pRackId, int X, int Y); /* rackObj.c       */



/* Forward Declarations */
extern int gDisplayAbort(void);

#ifndef GILSCRIPT

/* Globals for use by 768AS Hermes */
int err768AS = 0;
int rack768AS = 0;
int zone768AS = 0;
int well768AS = 0;

#endif

/*-----------------------------------------------------------------------
|    safeflow -  checks given flow against the max and min for 
|		 syringe being used. 
|		 If Outside one of the limits then the limit is returned 
|		 otherwise the given flow is returned
|		  
+-----------------------------------------------------------------------*/
static double safeflow(double flow)
{
   double max,min;
   max =  gilsonMaxFlowRate(pGilObjId);
   min = gilsonMinFlowRate(pGilObjId);

   DPRINT3(2,"safeflow: Flow: %0.1lf, Max: %0.1lf , Min: %0.1lf\n",flow,max,min);

   if (flow > 0.005)
   {
     flow = (flow < max) ? flow : max;
     flow = (flow > min) ? flow : min;
   }

   DPRINT1(2,"safeflow: Adj. Flow: %0.1lf\n",flow);
   return(flow);
}

/*-----------------------------------------------------------------------
|    safevolume -  
|		 checks given volume against the maximum the the
|		 syringe is capable of drawing at this moment.
|		 If Outside the limit then the limit is returned 
|		 otherwise the given volume is returned
|		  
+-----------------------------------------------------------------------*/
static double safevolume(double volume, int direction)
{
    /* If Volume passed in is -1, an Async syringe operation occurred
     *    so read it after the operation has completed.
     */
    int result;

    if (VolumeInSyringe == -1.0) {
        result = gilsonStoppedAll(pGilObjId);
    }

    VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
    DPRINT1(2,"safevolume: Read current volume: %.1lf\n",VolumeInSyringe);

    if (direction == IN) {
        DPRINT4(2, "safevolume: Volume: %0.1lf, Max Vol: %d , "
                   "VolInSyringe: %0.1lf, Vol Left: %lf\n",
                   volume, g215Bed.MaxVolume, VolumeInSyringe,
                   (g215Bed.MaxVolume - VolumeInSyringe));
        volume = ( (volume < (g215Bed.MaxVolume - VolumeInSyringe)) ? 
                       (volume) :
                       (g215Bed.MaxVolume - VolumeInSyringe)
                 );
        DPRINT1(2,"safevolume: Safe Volume: %0.1lf\n", volume);
    }

    else {
        DPRINT2(2,"safevolume: Volume: %0.1lf, VolInSyringe: %0.1lf\n",
                volume, VolumeInSyringe);
        volume = (volume < (VolumeInSyringe)) ? volume : VolumeInSyringe;
        DPRINT1(2,"safevolume: Safe Volume: %0.1lf\n",volume);
    }

    return(volume);
}

/*-------------------------------------------------------------------------
|
|   InitTclInterp - initialize the TCL interpreter
|
+---------------------------------------------------------------------------*/
int InitTclInterp(void)
{
   int status;

   /* Create TCL Interpreter */
   if (TclInterp == NULL)
   {
     TclInterp = Tcl_CreateInterp(); 
     if (TclInterp == NULL)
     {
       errLogRet(LOGOPT,debugInfo,"InitTclInterp: Could NOT create TCL interpreter.\n");
       return(-1);
     }
     else
     {
       status = Tcl_AppInit(TclInterp);  /* initialize the gilson Specific TCL commands */
       if (*TclInterp->result != 0)
       {
         errLogRet(LOGOPT,debugInfo,"InitTclInterp: TCL Init Error: '%s'\n",
       		TclInterp->result);
         Tcl_DeleteInterp(TclInterp);
         TclInterp = NULL;
         return(-1);
       }
     }
   }
   return(0);
}

/*-------------------------------------------------------------------------
|
|   TclDelete - cover function to free up and delete tcl interpreter
|
+---------------------------------------------------------------------------*/
void TclDelete()
{
    if (TclInterp != NULL)
        Tcl_DeleteInterp(TclInterp);
    TclInterp = NULL;
}

/*-------------------------------------------------------------------------
|
|   InterpCommand - Interprete TCL Command
|
+---------------------------------------------------------------------------*/
char * InterpCommand(char *script)
{
    if (InitTclInterp() == -1)
    {
      return("-1");
    }

    Tcl_Eval(TclInterp, script);
    return (Tcl_GetStringResult(TclInterp));
}

/*-------------------------------------------------------------------------
|
|   InterpScript - Interprete TCL Script File
|
+---------------------------------------------------------------------------*/
int InterpScript(char *tclScriptFile)
{
    int code,errorCode;
    char *errorStr;

    DPRINT2(1,"Tcl_EvalFile: 0x%lx, Script: '%s'\n", TclInterp,tclScriptFile);
    if (InitTclInterp() == -1)
    {
      return(-1);
    }

    /* Parse TCL Script */
    code = Tcl_EvalFile(TclInterp,tclScriptFile);

    /*
     * For the TCL script to pass back an error code,
     *   it must set the TCL variable "errorCode" to the value.
     * Convert the value of errorCode now.
     */
    errorStr = (char *)(Tcl_GetVar(TclInterp,"errorCode", 0));
    if (errorStr != NULL)
    {
        errorCode = atoi(errorStr);
        DPRINT3(1,"InterpScript(): code: %d, errorCode: '%s' or %d\n",
                code, errorStr, errorCode);
    }
    else
    {
        /* If TCL script didn't set errorCode,  assume a value of 0 */
        errorCode = 0;
        DPRINT2(1,"InterpScript(): code: %d, errorCode: %d\n",
                code, errorCode);
    }

    /*
     * Check if there were errors parsing the TCL script.
     * These are likely to be programming errors in the script.
     */
    if (code != TCL_OK)
    {
        if (errorCode == 0)
        {
            errLogRet(LOGOPT, debugInfo, "TCL Script Error: '%s'\n",
                      TclInterp->result);

            /* Display Aborted on Gilson if it was aborted */
            gDisplayAbort();

	    errorCode = SMPERROR+TCLSCRIPTERROR;
        }
        return(errorCode);
    }
    return(0);
}

static int ChkLimits(Tcl_Interp *interp, char *FuncName, int rackloc, int zone, int sample) 
{
   RACKOBJ_ID pRackId;
   char errorcode[12];
   int maxzone,maxsample;

  /* 1. is rackloc a valid range */
  if ((rackloc < 1) && (rackloc > GIL_MAX_RACKS))
  {
      errLogRet(LOGOPT, debugInfo, 
          "%s: Rack Location %d is out of bounds (valid range 1 - %d)\n",
          FuncName, rackloc, GIL_MAX_RACKS);
      sprintf(tclErrCode, "%d", (SMPERROR+INVALID_RACK));
      return (-1);
  }

  /* 2. Does rackloc actually point to a Rack Object ? */
  if ( g215Bed.LoadedRacks[rackloc] == NULL)
  {
      errLogRet(LOGOPT, debugInfo,
          "%s: Rack Location %d has not been loaded with a Rack\n",
          FuncName, rackloc);
      
      sprintf(tclErrCode, "%d", (SMPERROR+INVALID_RACK));
      return -1;
  }

#ifdef XXX
  /* 3. is zone  valid for this rack */
  if ((racktype < 1) || (racktype > GIL_MAX_RACK_TYPES))
  {
      errLogRet(LOGOPT,debugInfo,"%s: Rack Type %d is invalid (valid rang 0 - %d)\n",
		FuncName,racktype,GIL_MAX_RACK_TYPES);
	return(SMPERROR+INVALID_ZONE);
  }
#endif

  pRackId = g215Bed.LoadedRacks[rackloc];

  /* 3. is zone  valid for this rack */
  if ( (maxzone = rackInvalidZone(pRackId, zone)) > 0 )
  {
      errLogRet(LOGOPT, debugInfo,
          "%s: Zone %d is invalid (valid rang 1 - %d)\n",
	  FuncName, zone, maxzone);
      sprintf(tclErrCode, "%d", (SMPERROR+INVALID_ZONE));
      return( -1 );
  }

  /* 4. is sample number  valid for this rack and zone */
  if ( (maxsample = rackInvalidSample(pRackId, zone, sample)) > 0 )
  {
      errLogRet(LOGOPT, debugInfo,
          "%s: Sample %d is invalid (valid range 1 - %d)\n",
	  FuncName, sample, maxsample);
      sprintf(tclErrCode,"%d",(SMPERROR+INVALID_SAMP));
      return(-1);
  }

/*
 * zspeed = rackFlow2Zspeed(pRackId, gilsonParmsId.SampleWellRate, zone, loc);
 * gilsonParmsId.SampZSpeed = zspeed;
 * gilsonParmsId.SampVol2ZTravel = rackVol2ZTrail(pRackId,
 *     gilsonParmsId.SampleVolume, zone, loc);
 * DPRINT2(1,"SampZSpeed: %lf, SampVol2ZTravel: %lf \n\n",
 *     gilsonParmsId.SampZSpeed, gilsonParmsId.SampVol2ZTravel);
 */

  return(0);
}

/*-------------------------------------------------------------------------
|
|   gClearVolume - Zero static volume in syringe parameter
|
+---------------------------------------------------------------------------*/
int gClearVolume(void)
{
  VolumeInSyringe = 0.0;
  return (0);
}

/*-------------------------------------------------------------------------
|
|   gDisplayAbort - display User Aborted message 
|
+---------------------------------------------------------------------------*/
int gDisplayAbort(void)
{
    if (AbortRobo != 0) 
    {
       Tcl_Eval(TclInterp,"gWriteDisplay \" Aborted\"");
    }
    return(0);
}

/*----------------------------------------------------
*	  gWriteDisplay
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gWriteDisplay(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    char message[10];

    if (argc != 2)
    {
        Tcl_AppendResult(interp,
            "gWriteDisplay wrong # args: should be \"", argv[0],
            " String (8-char max.)\"", (char *) NULL);
        return TCL_ERROR;
    }
    DPRINT1(1,"gWriteDisplay: '%s' \n",argv[1]);

    /* can only use 8 char strings */
    strncpy(message,argv[1],8);
    message[8] = '\0';

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilWriteDisplay(pGilObjId,message) == -1)
    {
        Tcl_AppendResult(interp,
            "gWriteDisplay:  Failed to Display Message: ",
            argv[1], NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL,"Display: '%s'\n",message);
    printf("Display: '%s'\n",message);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*	  gMoveZ2Top
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMoveZ2Top(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    /* check for abort, if SO then just return */
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMoveZ2Top:  User Aborted",NULL);
        return TCL_ERROR;
    }

    DPRINT(1,"gMoveZ2Top \n");
#if !defined(TESTER) && !defined(STDALONE)
    if ( gilMoveZ2Top(pGilObjId) == -1)
    {
        Tcl_AppendResult(interp,
            "gMoveZ2Top:  Failed to move Needle to Top", NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ", ErrMsge, "  ", NULL);
        return TCL_ERROR;
    }
#else
    printf("gMoveZ2Top \n");
#endif
    return TCL_OK;
}

/*----------------------------------------------------
*   gMove2RinseStation 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMove2RinseStation(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    DPRINT(1,"gMove2RinseStation \n");
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMove2RinseStation: User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilMove2Rinse(pGilObjId) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Move To Rinsing Station",NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL,"gMove2RinseStation\n");
    printf("gMove2RinseStation \n");
#endif
    return TCL_OK;
}

/*----------------------------------------------------
*   gFlush Rinse_Volume MaxFlow MaxFlow  
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gFlush(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    double RinseVol,InFlowRate,OutFlowRate,TmpVol;
    double tInFlow,tOutFLow; 

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gFlush wrong # args: should be \"", argv[0],
            " Volume(ul) InputFlow OutputFlow (ml/min)\"", (char *) NULL);
        return TCL_ERROR;
    }

    RinseVol = atof(argv[1]);
    InFlowRate = atof(argv[2]);
    OutFlowRate = atof(argv[3]);
    DPRINT4(1, "gFlush: Volume: %.1lf, InFlow: %.1lf, Out Flow: %.1lf, "
               "Cur Syringe Vol: %.1lf \n",
               RinseVol, InFlowRate, OutFlowRate, VolumeInSyringe);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gFlush:  User Aborted",NULL);
        return TCL_ERROR;
    }

    if (InFlowRate > 0.0)
        TmpVol = safevolume(RinseVol,IN);
    else
        TmpVol = safevolume(RinseVol,OUT);

    if (fabs(TmpVol- RinseVol) > 0.1)
    {
	errLogRet(LOGOPT, debugInfo, "gFlush: WARNING: requested volume,  "
                  "%.1lf > max available %.1lf, volume forced to maximum.  ",
		  RinseVol, TmpVol);
        RinseVol = TmpVol;
    }

     /* be sure flow rates are with limits of syringe in use */
     InFlowRate = safeflow(InFlowRate);
     OutFlowRate = safeflow(OutFlowRate);

#if !defined(TESTER) && !defined(STDALONE)
    if (gilsonFlush(pGilObjId, RinseVol, InFlowRate, OutFlowRate ) == -1)
    {
        Tcl_AppendResult(interp, "gFlush: Failed to Flush",NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
        return TCL_ERROR;
    }

    /* if doing a Async fill then don't read the current volume set it to -1
       to flag other routines (safevolume()) to read it before they use it
    */
    if ((OutFlowRate == 0.0) && (InFlowRate > 0.0))
       VolumeInSyringe = -1.0;
    else
       VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);

    DPRINT1(1,"gFlush: VolumeInSyringe: %.1lf\n", VolumeInSyringe);
#else
    diagPrint(NULL, "gFlush: Vol %.1lf (ul), In Flow: %.1lf (ml/min), "
                    "Out Flow: %.1lf (ml/min)\n",
                    RinseVol, InFlowRate, OutFlowRate);
    printf("gFlush: Vol %.1lf (ul), In Flow: %.1lf (ml/min), "
           "Out Flow: %.1lf (ml/min)\n",
           RinseVol, InFlowRate, OutFlowRate);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*   gMove2InjectorPort 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMove2InjectorPort(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    DPRINT(1,"gMove2InjectorPort \n");
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMove2InjectorPort:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilMove2Inj(pGilObjId) == -1)
    {
        Tcl_AppendResult(interp,
                         "Failed to move Needle to Injector Port", NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ", ErrMsge, "  ", NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL,"gMove2InjectorPort \n");
    printf("gMove2InjectorPort \n");
#endif
    return TCL_OK;
}

/*----------------------------------------------------
*   gDelayMsec
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gDelayMsec(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int count;
    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gDelayMsec wrong # args: should be \"",
            argv[0], " msec_count \"", (char *) NULL);
        return TCL_ERROR;
    }
    count = atoi(argv[1]);
    DPRINT1(1,"gDelayMsec: %d \n",count);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gDelayMsec:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    delayMsec(count);
#else
    diagPrint(NULL,"gDelayMsec: delay %d (msec) \n",count);
    printf("gDelayMsec: delay %d (msec) \n",count);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*   gSetContacts 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gSetContacts(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int relay, on;

    if (argc != 3)
    {
        Tcl_AppendResult(interp, "gSetContacts wrong # args: should be \"",
            argv[0], " relay on/off  \"", (char *) NULL);
        return TCL_ERROR;
    }
    relay = atoi(argv[1]);
    on = atoi(argv[2]);
    DPRINT2(1,"gSetContacts: Relay: %d, Onflag: %d \n",relay,on);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gSetContacts:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if (gilSetContacts(pGilObjId, relay, on ) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Switch Relay Contact",
            argv[1], NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ", ErrMsge, "  ", NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL, "gSetContacts: Relay: %d, On/Off: %d (1=on,0=off)\n",
              relay,on);
    printf("gSetContacts: Relay: %d, On/Off: %d (1=on,0=off)\n", relay, on);
#endif

    return TCL_OK;
}


/*----------------------------------------------------
*   gGetInputs 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gGetInputs(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int relay, on;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gGetContacts wrong # args: should be \"",
            argv[0], " relay on/off  \"", (char *) NULL);
        return TCL_ERROR;
    }
    relay = atoi(argv[1]);
    DPRINT2(1,"gGetInputs: Relay: %d, Onflag: %d \n",relay,on);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gGetInputs:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilGetInputs(pGilObjId, relay, &on ) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Get Inputs Contact Status",
                         argv[1], NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }

    /* use int, volume can't be fractional ul anyway */
    sprintf(interp->result,"%d",on);
    DPRINT3(1, "Input %d is '%s' (%d)\n", relay,
            ((on == 1) ? "Closed" : "Open"), on);
#else
    diagPrint(NULL, "gGetInputs: Relay: %d, On/Off: %d (1=on,0=off)\n",
              relay, 0);
    sprintf(interp->result,"%d",(int)0);  /* default to Closed */
#endif

    return TCL_OK;
}


/*----------------------------------------------------
*   gGetContacts 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gGetContacts(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int relay, on;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gGetContacts wrong # args: should be \"",
            argv[0], " relay on/off  \"", (char *) NULL);
        return TCL_ERROR;
    }
    relay = atoi(argv[1]);
    DPRINT2(1,"gGetContacts: Relay: %d, Onflag: %d \n", relay, on);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gGetContacts:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilGetContacts(pGilObjId, relay, &on ) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Get Relay Contact Status",
            argv[1], NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ", ErrMsge, "  ", NULL);
        return TCL_ERROR;
    }

    /* use int, volume can't be fractional ul anyway */
    sprintf(interp->result,"%d",on);
    DPRINT3(1,"Contact %d is '%s' (%d)\n", relay,
        ((on == 1) ? "Closed" : "Open"), on);

#else
    diagPrint(NULL, "gGetContacts: Relay: %d, On/Off: %d (1=on,0=off)\n",
              relay, 0);
    sprintf(interp->result,"%d",(int)0);  /* default to Closed */
#endif

    return TCL_OK;
}

/* ------------------------- Async functions ---------------------------*/

#define ASYNC 1
#define SYNC 0

int AspirateVol(double Volume, double FlowRate, double zSpeed,
                int AsyncFlag, char *FuncName, Tcl_Interp *interp)
{
    double zTravel;
    double TmpVol;
    int tubebottom, liqtop, depthlevel;
    int XYmm[2], xAxis, yAxis;
    int result;

    DPRINT5(1,"AspirateVol: %.1lf, %.1lf %.1lf, Async: %d, call func: '%s'\n",
	      Volume, FlowRate, zSpeed,AsyncFlag, FuncName);

    TmpVol = safevolume(Volume,IN);
    if (fabs(TmpVol- Volume) > 0.1)
    {
	errLogRet(LOGOPT,debugInfo,
	   "%s: WARNING: requested volume,  %.1lf > max available %.1lf, "
           "volume forced to maximum.  ", FuncName, Volume, TmpVol);
        Volume = TmpVol;
    }

    FlowRate = safeflow(FlowRate);

    /* flow (ml/sec) = flow (ml/min) / 60 (sec/min )
       flow (ul/sec) = flow (ml/sec) * 1000 (ul/ml)
       seconds to aspirate =  volume (ul) / flow (ul/sec) 
       distance = ZSpeed (mm/sec) * seconds to aspirate
    */
    zTravel =  zSpeed * (Volume / ( (FlowRate /60.0) * 1000.0 ) );
    DPRINT2(1, "zTravel: Zspeed: %.1lf (mm/sec), zTravel: %.1lf mm\n",
            zSpeed, zTravel);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "%s:  User Aborted",FuncName,NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    gilGetZ(pGilObjId,&liqtop);
    depthlevel = liqtop - (int) ((zTravel * 10) + .5);
    DPRINT2(1,"Present Liq Level Z: %d, End Depth Z: %d\n",
	liqtop,depthlevel);

    /* if zSpeed > 0, Will we hit bottom if we proceed ?? */
    if ( (pLastRack != NULL) && (zSpeed > 0.05) )
    {
      gilGetXY(pGilObjId, XYmm);
      xAxis = rackGetX(pLastRack, lastZone, lastSample);
      yAxis = rackGetY(pLastRack, lastZone, lastSample);
      DPRINT6(1, "%s - Location: X: %d, Y: %d, Z: %d, lastSample: X: %d, "
          "Y: %d\n", FuncName, XYmm[0], XYmm[1], liqtop, xAxis, yAxis);

       /* Test if where we are is consistent with last Sample positioning  */
       if ( (abs(XYmm[0] - xAxis) < 5)  && (abs(XYmm[1] - yAxis) < 5) )
       {
	   DPRINT1(1,"%s: At Last Sample Position Confirmed.\n",FuncName);
           tubebottom = rackSampBottom(pLastRack,lastZone,lastSample);
           DPRINT4(1,"%s: At Z: %d, End Z: %d, Bottom Z: %d\n",
	       FuncName,liqtop,depthlevel,tubebottom);
       }
       else
       {
	   errLogRet(LOGOPT, debugInfo,
	       "%s: WARNING: Aspirating From Non-Confirmable Position.\n",
               FuncName);
           depthlevel = liqtop;
           tubebottom = liqtop;
           zSpeed = 0.0;
       }

       /* If depthlevel is lower than tubebottom, max tubebottom the limit */
       if ( depthlevel < tubebottom )
       {
          errLogRet(LOGOPT, debugInfo,
             "%s: WARNING: Possibly not enough liquid in vial. "
             "Will NOT follow liquid. \n", FuncName);
          zSpeed = 0;   /* force speed to zero so needle will not go down. */
          DPRINT2(1, "depthlevel %d < %d tubebottom, will go to tubebottom "
              "with Z speed at 0\n", depthlevel,tubebottom);
          depthlevel = tubebottom;
       }

       /* Z Speed is less than 1 then just go to depthlevel
        * Also does the Z move for the ( depthlevel < tubebottom )
        *   test above
        */
       if ( zSpeed < 1.0 )
       {
          DPRINT2(1,"ZSpeed %.1lf < 1.0, just go to depth level %d with "
              "Z speed at 0\n", zSpeed,depthlevel);
          gilMoveZ(pGilObjId,depthlevel);
          zSpeed = 0.0;
       }
    }

    if (AsyncFlag == ASYNC)
    {
	result = gilsonAspirateAsync(pGilObjId, Volume, FlowRate,
                                     (int) zSpeed, depthlevel);
        /* set VolumeInSyringe to -1 to indicate Async operation on Syringe
         * volume should be read later
         */
        VolumeInSyringe = -1.0;
    }
    else
    {
	result = gilsonAspirate(pGilObjId, Volume, FlowRate,
                     (int) zSpeed, depthlevel);
        VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
    }
     
    if (result == -1)
    {
        Tcl_AppendResult(interp, "Failed to Aspirate",NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }

    DPRINT2(1, "%s: Volume In Syringe: %.1lf\n", FuncName, VolumeInSyringe);
#else
    diagPrint(NULL, "%s: Vol %.1lf (ul), Flow: %.1lf (ml/min), Z Speed: "
                    "%.1lf (mm/sec)\n",	FuncName, Volume, FlowRate, zSpeed);
    printf("%s: Vol %.1lf (ul), Flow: %.1lf (ml/min), Z Speed: "
           "%.1lf (mm/sec)\n", FuncName, Volume, FlowRate, zSpeed);
#endif

    return TCL_OK;
}


/*----------------------------------------------------
*
*  gAspirate Volume Inject_Flow 0
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gAspirate(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    double Volume,FlowRate,zSpeed,zTravel;
    int result;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gAspirate: wrong # args: should be \"",
            argv[0], " Volume Flow \"", (char *) NULL);
        return TCL_ERROR;
    }

    DPRINT4(1, "gAspirate: argv[] - Volume: '%s', Flow: '%s', Z Speed: '%s', "
        "Current Syringe Vol: %.1lf\n", argv[1], argv[2], argv[3],
        VolumeInSyringe);

    Volume = atof(argv[1]);
    FlowRate = atof(argv[2]);
    zSpeed = atof(argv[3]);
    DPRINT4(1,"gAspirate: Volume: %.1lf, Flow: %.1lf, Z Speed: %.1lf, "
            "Current Syringe Vol: %.1lf\n", Volume, FlowRate, zSpeed,
            VolumeInSyringe);

    result = AspirateVol(Volume, FlowRate, zSpeed, SYNC, "gAspirate", interp);

    return result;  /*TCL_OK or TCL_ERROR */
    /* return TCL_OK; */
}


/*----------------------------------------------------
*
*  gAspirateAsync Volume Inject_Flow 0
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gAspirateAsync(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    double Volume,FlowRate,zSpeed,zTravel;
    int result;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gAspirateAsync: wrong # args: "
            "should be \"", argv[0], " Volume Flow \"", (char *) NULL);
        return TCL_ERROR;
    }

    DPRINT4(1, "gAspirateAsync: argv[] - Volume: '%s', Flow: '%s', "
            "Z Speed: '%s', Current Syringe Vol: %.1lf\n",
            argv[1], argv[2], argv[3], VolumeInSyringe);

    Volume = atof(argv[1]);
    FlowRate = atof(argv[2]);
    zSpeed = atof(argv[3]);
    DPRINT4(1,"gAspirateAsync: Volume: %.1lf, Flow: %.1lf, "
            "Z Speed: %.1lf, Current Syringe Vol: %.1lf\n",
            Volume, FlowRate, zSpeed, VolumeInSyringe);

    result = AspirateVol(Volume, FlowRate, zSpeed, ASYNC,
                         "gAspirateAsync", interp);

    return result;  /*TCL_OK or TCL_ERROR */
    /* return TCL_OK; */
}


int DispenseVol(double Volume, double FlowRate, double zSpeed,
                int AsyncFlag, char* FuncName,Tcl_Interp *interp)
{
    double zTravel;
    int dresult;
    double TmpVol;
    int tubetop, liqtop, endheight;
    int XYmm[2], xAxis, yAxis;

    DPRINT5(1,"DispenseVol: %.1lf, %.1lf %.1lf, Async: %d, call func: '%s'\n",
              Volume, FlowRate, zSpeed,AsyncFlag, FuncName);

    TmpVol = safevolume(Volume,OUT);
    if (fabs(TmpVol- Volume) > 0.1)
    {
        errLogRet(LOGOPT,debugInfo,
           "%s: WARNING: requested volume,  %.1lf > max available %.1lf, "
           "volume forced to maximum.  ", FuncName, Volume, TmpVol);
        Volume = TmpVol;
    }

    FlowRate = safeflow(FlowRate);

    if ((zSpeed > 0.0) && (zSpeed < 1.0))
       zSpeed = 1.0;

    /* flow (ml/sec) = flow (ml/min) / 60 (sec/min )
       flow (ul/sec) = flow (ml/sec) * 1000 (ul/ml)
       seconds to dispense =  volume (ul) / flow (ul/sec) 
       distance = ZSpeed (mm/sec) * seconds to dispense
    */
    zTravel =  zSpeed * (Volume / ( (FlowRate /60.0) * 1000.0 ) );
    DPRINT2(1, "zTravel: Zspeed: %.1lf (mm/sec), zTravel: %.1lf mm\n",
            zSpeed, zTravel);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gDispense:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    gilGetZ(pGilObjId,&liqtop);
    endheight = liqtop + (int) ((zTravel * 10) + .5);
    DPRINT2(1,"Present Liq Level Z: %d, End Height Z: %d\n",
        liqtop, endheight);

    /* if zSpeed > 0, Will we overflow if we proceed ?? */
    if ( (pLastRack != NULL) && (zSpeed > 0.05) )
    {
        gilGetXY(pGilObjId, XYmm);
        xAxis = rackGetX(pLastRack, lastZone, lastSample);
        yAxis = rackGetY(pLastRack, lastZone, lastSample);
        DPRINT6(1, "%s - Location: X: %d, Y: %d, Z: %d, lastSample: X: %d, "
            "Y: %d\n", FuncName, XYmm[0], XYmm[1], liqtop, xAxis, yAxis);

        /* Test if where we are is consistent with last Sample positioning  */
        if ( (abs(XYmm[0] - xAxis) < 5)  && (abs(XYmm[1] - yAxis) < 5) )
        {
            DPRINT1(1,"%s: At Last Sample Position Confirmed.\n",FuncName);
            tubetop = rackSampTop(pLastRack,lastZone,lastSample);
            DPRINT4(1,"%s: At Z: %d, End Z: %d, Top Z: %d\n",
                FuncName, liqtop, endheight, tubetop);
        }
        else
        {
            errLogRet(LOGOPT, debugInfo,
                "%s: WARNING: Dispensing to Non-Confirmable Position.\n",
                FuncName);
            endheight = liqtop;
            tubetop = liqtop;
            zSpeed = 0.0;
        }

        /* If endheight is above the tube top, make tubetop the limit */
        if ( endheight > tubetop )
        {
            errLogRet(LOGOPT, debugInfo,
               "%s: WARNING: Dispensing may overflow vial. "
               "Will NOT follow liquid. \n", FuncName);
            zSpeed = 0;   /* force speed to zero so needle will not go up. */
            DPRINT2(1, "endheight %d > %d tubetop, will go to tubetop "
                "with Z speed at 0\n", endheight, tubetop);
            endheight = tubetop;
        }

        /* Z Speed is less than 1 then just go to endheight
         * Also does the Z move for the ( endheight > tubetop )
         *   test above
         */
        if ( zSpeed < 1.0 )
        {
           DPRINT2(1,"ZSpeed %.1lf < 1.0, just go to end height %d with "
               "Z speed at 0\n", zSpeed, endheight);
           gilMoveZ(pGilObjId, endheight);
           zSpeed = 0.0;
        }
    }

    if (AsyncFlag == ASYNC)
    {
        dresult = gilsonDispenseAsync(pGilObjId, Volume, FlowRate,
                      (int) zSpeed, endheight);
        /* set VolumeInSyringe to -1 to indicate Async operation on Syringe
           volume should be read later */
        VolumeInSyringe = -1.0;
    }
    else
    {
        dresult = gilsonDispense(pGilObjId, Volume, FlowRate,
                      (int) zSpeed, endheight);
        VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
    }
    if (dresult == -1)
    {
        Tcl_AppendResult(interp, "Failed to Dispense Sample", NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }

    DPRINT2(1,"%s: VolumeInSyringe: %.1lf\n", FuncName, VolumeInSyringe);

#else
    diagPrint(NULL, "gDispense: Vol %.1lf (ul), Flow: %.1lf (ml/min), "
                    "Z Speed: %.1lf (mm/sec)\n", Volume, FlowRate, zSpeed);
    printf("gDispense: Vol %.1lf (ul), Flow: %.1lf (ml/min), Z Speed: "
           "%.1lf (mm/sec)\n", Volume, FlowRate, zSpeed);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*   gDispense Volume FlowRate zSpeed 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gDispense(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    double Volume, FlowRate, zSpeed;
    int result;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gDispense wrong # args: should be \"",
            argv[0], " Volume Flow Zspeed\"", (char *) NULL);
        return TCL_ERROR;
    }
    DPRINT4(1, "gDispense: Volume: %s, Flow: %s, Z Speed: %s, "
            "Current Syringe Vol: %.1lf \n",
            argv[1], argv[2], argv[3], VolumeInSyringe);
    Volume = atof(argv[1]);
    FlowRate = atof(argv[2]);
    zSpeed = atof(argv[3]);
    DPRINT4(1, "gDispense: Volume: %.1lf, Flow: %.1lf, Z Speed: %.1lf, "
            "Current Syringe Vol: %.1lf \n",
            Volume, FlowRate, zSpeed, VolumeInSyringe);

    result = DispenseVol(Volume, FlowRate, zSpeed, SYNC, "gDispense",interp);

    return result;
}

/*----------------------------------------------------
*   gDispenseAsync Volume FlowRate zSpeed 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gDispenseAsync(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    double Volume,FlowRate,zSpeed;
    int result;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gDispenseAsync wrong # args: should be \"",
            argv[0], " Volume Flow Zspeed\"", (char *) NULL);
        return TCL_ERROR;
    }
    DPRINT4(1, "gDispenseAsync: Volume: %s, Flow: %s, Z Speed: %s, "
            "Current Syringe Vol: %.1lf \n",
            argv[1], argv[2], argv[3], VolumeInSyringe);
    Volume = atof(argv[1]);
    FlowRate = atof(argv[2]);
    zSpeed = atof(argv[3]);
    DPRINT4(1, "gDispenseAsync: Volume: %.1lf, Flow: %.1lf, Z Speed: %.1lf, "
            "Current Syringe Vol: %.1lf \n",
            Volume, FlowRate, zSpeed, VolumeInSyringe);

    result = DispenseVol(Volume, FlowRate, zSpeed, ASYNC,
                         "gDispenseAsync",interp);

    return result;
}

/*-----------------------------------------------------------------
*   gStopTestAll Teset that all Axis and syringe action is complete
*
+------------------------------------------------------------------*/
        			/* ARGSUSED */
int gStopTestAll(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int result;

    result = gilsonStoppedAll(pGilObjId);
    if (result == -1)
    {
        Tcl_AppendResult(interp,
            "gStopTestAll: Gilson Error in last movement", NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
        return TCL_ERROR;
    }

    return TCL_OK;
}

int gTubeX(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int xAxis,yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone,xcenter,ycenter;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gTubeX wrong # args: should be \"", argv[0],
                " RackLoc SampleZone SampleNumber \"", (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    sample = atoi(argv[3]);

    DPRINT3(1, "gTubeX: Rack Loc: %d, Zone: %d, Sample: %d \n",
            rackloc, zone, sample);

    pSampRack = g215Bed.LoadedRacks[rackloc];

    xcenter = (g215Bed.Rack1Center[0] + ((rackloc-1) * 1196)) +
              rackDelta[rackloc-1][0];
    ycenter = g215Bed.Rack1Center[1]+ rackDelta[rackloc-1][1];
    rackCenter(pSampRack, xcenter,ycenter);


    xAxis = rackGetX(pSampRack, zone, sample);
    sprintf(interp->result,"%d",xAxis);  /* return value to TCL script */
    DPRINT1(1,"gTubeX Loc: %d\n",xAxis);

    return TCL_OK;
}

int gTubeY(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int xAxis,yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gTubeY wrong # args: should be \"", argv[0],
                " RackLoc SampleZone SampleNumber \"", (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    sample = atoi(argv[3]);

    DPRINT3(1,"gTubeY: Rack Loc: %d, Zone: %d, Sample: %d \n",
            rackloc, zone, sample);

    /*pSampRack = gilsonRackLoc2RackId(rackloc); */
    pSampRack = g215Bed.LoadedRacks[rackloc];

    yAxis = rackGetY(pSampRack, zone, sample);
    sprintf(interp->result,"%d",yAxis);  /* return value to TCL script */
    DPRINT1(1,"gTubeY Loc: %d\n",xAxis);

    return TCL_OK;
}


/*----------------------------------------------------
/*   gMoveZ Z_location(mm)
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMoveZ(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int Zloc;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gMoveZ wrong # args: should be \"", argv[0],
                " Z-Location (mm)\"", (char *) NULL);
        return TCL_ERROR;
    }
    Zloc = atoi(argv[1]);
    DPRINT1(1,"gMoveZ: %d \n",Zloc);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMoveZ:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if (gilMoveZ(pGilObjId,Zloc) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Move to Z Position",argv[1],NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL, "gMoveZ: Move to Z location: %d (tenths mm)\n", Zloc);
    printf("gMoveZ: Move to Z location: %d (tenths mm)\n", Zloc);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
/*   gMoveZLQ Z_location(mm)
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMoveZLQ(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int Zloc;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gMoveZLQ wrong # args: should be \"",
            argv[0], " Z-Location (mm)\"", (char *) NULL);
        return TCL_ERROR;
    }
    Zloc = atoi(argv[1]);
    DPRINT1(1,"gMoveZLQ: %d \n",Zloc);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMoveZLQ:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if (gilMoveZLQ(pGilObjId,Zloc) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Move to Z Position",argv[1],NULL);
        ErrMsge = gilErrorMsge(pGilObjId);
        Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL, "gMoveZ: Move to Z location: %d (tenths mm)\n", Zloc);
    printf("gMoveZLQ: Move to Z location: %d (tenths mm)\n", Zloc);
#endif
    return TCL_OK;
}

static int Move2Sample( int rackloc, int zone, int sample )
{
    int xAxis,yAxis,result;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int xcenter,ycenter;

/*
 *  pSampRack = gilsonRackLoc2RackId(rackloc);
 *  DPRINT1(1,"Move2Sample()  Rack: 0x%lx\n",pSampRack);
 *  pSampRack =  gilsonParmsId.Racks[rackloc];
 *  DPRINT1(1,"Move2Sample()  Rack: 0x%lx\n",pSampRack);
 */

    pSampRack =  g215Bed.LoadedRacks[rackloc];
    if (DebugLevel > 0)
    {
        int i;
        for(i=0;i < g215Bed.NumRacks; i++)
        {
            if (g215Bed.DefinedRacks[i].pRackObj == pSampRack)
            {
                DPRINT2(1,"Move2Sample()  Rack: 0x%lx, Type:,'%s'\n",
                pSampRack,g215Bed.DefinedRacks[i].IdStr);
                break;
            }
        }
    }

    /* since 1 rack may have multiple locations on the bed 
     * we need to reset the center location for the bed location
     */
    xcenter = (g215Bed.Rack1Center[0] + ((rackloc-1) * 1196)) +
              rackDelta[rackloc-1][0];
    ycenter = g215Bed.Rack1Center[1]+ rackDelta[rackloc-1][1];
    DPRINT2(1,"Center of Rack: X = %d, Y = %d\n",xcenter,ycenter);
    rackCenter(pSampRack, xcenter,ycenter);

    xAxis = rackGetX(pSampRack, zone, sample);
    yAxis = rackGetY(pSampRack, zone, sample);

    DPRINT2(1,"Move2Sample()  X: %d, Y: %d \n",xAxis,yAxis);

    result = gilMoveXY(pGilObjId,xAxis,yAxis);
    if ( result == -1)
    {
        ErrMsge = gilErrorMsge(pGilObjId);
    }
    /* set last sample worked on */
    lastSample = sample;
    lastZone = zone;
    pLastRack = pSampRack;

#ifdef XXXX
    lastZSpeed = rackFlow2Zspeed(pSampRack, lastFlowRate, zone, sample);
    lastVol2ZTravel = rackVol2ZTrail(pSampRack, lastVolume, zone, sample);
    DPRINT2(1,"SampZSpeed: %lf, SampVol2ZTravel: %lf \n\n", lastZspeed,
		lastVol2ZTravel);
#endif

    return result;
}


/*----------------------------------------------------
*  gMove2Sample
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMove2Sample(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int xAxis,yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone,error;

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gMove2Sample wrong # args: should be \"",
            argv[0], " RackLoc SampleZone SampleNumber \"", (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    sample = atoi(argv[3]);

    DPRINT3(1, "gMove2Sample: Rack Loc: %d, Zone: %d, Sample: %d \n",
            rackloc, zone, sample);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMove2Sample:  User Aborted",NULL);
        return TCL_ERROR;
    }

    if ( (error = ChkLimits(interp,"gMove2Sample",rackloc,zone,sample)) != 0)
    {
        Tcl_SetErrorCode(interp,tclErrCode,(char *) NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( Move2Sample(rackloc, zone, sample) == -1)
    {
        Tcl_AppendResult(interp, "Failed to Move To Sample: ",
            argv[1], "  ", NULL);
        Tcl_AppendResult(interp, ", ", ErrMsge, "  ", NULL);
        return TCL_ERROR;
    }

#else
    diagPrint(NULL, "gMove2Sample: Rack Location: %d, Sample Zone: %d, "
                    "Sample Number: %d\n", rackloc, zone, sample);
    printf("gMove2Sample: Rack Location: %d, Sample Zone: %d, "
           "Sample Number: %d\n", rackloc, zone, sample);
#endif
    return TCL_OK;
}

/*----------------------------------------------------
*   gInjector2Load 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gInjector2Load(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    DPRINT(1,"gInjector2Load \n");
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gInjector2Load:  User Aborted", NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilLoad(pGilObjId) == -1)
    {
        Tcl_AppendResult(interp,
            "Failed to set Injector to Load Position", NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL,"gInjector2Load \n");
    printf("gInjector2Load \n");
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*   gInjector2Inject 
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gInjector2Inject(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    DPRINT(1,"gInjector2Inject \n");
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gInjector2Inject:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if ( gilInject(pGilObjId) == -1)
    {
        Tcl_AppendResult(interp,
            "Failed to set Injector to Inject Position", NULL);
        return TCL_ERROR;
    }
#else
    diagPrint(NULL, "gInjector2Inject \n");
    printf("gInjector2Inject \n");
#endif
    return TCL_OK;
}

/*----------------------------------------------------
*  gInitSyringe 
*  Set the syringe volume and initialize pump.
*  This is only appropriate for the 402B standalone
*  pump (pumpType = 4).
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gInitSyringe(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{
    int size, result;

    DPRINT(1,"gInitSyringe\n");
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gInitSyringe:  User Aborted",NULL);
        return TCL_ERROR;
    }

    if (pGilObjId->pumpType != 4) {
        return TCL_OK;
    }

    if (argc != 2) {
        Tcl_AppendResult(interp, "gInitSyringe wrong # args: should be \"",
            argv[0], " SyrSize\"", (char *) NULL);
        return TCL_ERROR;
    }

    /* Save current volume in size */
    size = pGilObjId->pumpVolume;

    /* Try homing dilutor with the new syringe size */
    pGilObjId->pumpVolume = atoi(argv[1]);
    result = gilHomeDiluter(pGilObjId);
    if (result == 0) {
        /* Succeeded, update NVRAM */
        gilsonSetPump(pGilObjId, 4, pGilObjId->pumpVolume);
    }
    else {
        /* Restore old pump volume */
        pGilObjId->pumpVolume = size;
        Tcl_AppendResult(interp, "Failed to initialize syringe", NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}


/*----------------------------------------------------
*   gCurrentSyrVol (ul)
*   Returns current syringe volume.
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gCurrentSyrVol(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    double Cvolume;

    DPRINT(1,"gCurrentSyrVol \n");
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gCurrentSyrVol:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    Cvolume = (double) gilsonCurrentVolume(pGilObjId);

    /* use int, volume can't be fractional ul anyway */
    sprintf(interp->result,"%d",(int)Cvolume);
    DPRINT1(1,"Current Vol: %.1lf\n",Cvolume);

    /* return value some how */
#else
    diagPrint(NULL, "gCurrentSyrVol \n");
    /* use int, volume can't be fractional ul anyway */
    sprintf(interp->result, "%d", (int)100);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*   gSetUnitId (ul)
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gSetUnitId(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int unit,oldUnit;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gSetUnitId wrong # args: should be \"",
            argv[0], " Unit Number\"", (char *) NULL);
        return TCL_ERROR;
    }
    unit = atoi(argv[1]);
    DPRINT1(1,"gSetUnitId: %d \n",unit);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gSetUnitId:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    oldUnit = gilSelectUnit(pGilObjId, unit);

    /* return previous Gilson Unit Id */
    sprintf(interp->result,"%d",oldUnit);
    DPRINT1(1,"Previous Unit: %d\n",oldUnit);
#else
    diagPrint(NULL, "gSetUnitId: %d \n",unit);
    sprintf(interp->result, "%d", (int) 22);
#endif

    return TCL_OK;
}
/*----------------------------------------------------
*   gSetPumpId (id)
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gSetPumpId(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int pump, oldPump;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gSetPumpId wrong # args: should be \"",
            argv[0], " PumpId\"", (char *) NULL);
        return TCL_ERROR;
    }
    pump = atoi(argv[1]);
    DPRINT1(1,"gSetPumpId: %d \n", pump);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gSetPumpId:  User Aborted",NULL);
        return TCL_ERROR;
    }

    oldPump = gilSelectPump(pGilObjId, pump);

    /* return previous Gilson Unit Id */
    sprintf(interp->result, "%d", oldPump);
    DPRINT1(1, "Previous Pump: %d\n", oldPump);

    return TCL_OK;
}
/*----------------------------------------------------
*   gSetInjectId (ul)
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gSetInjectId(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int unit,oldUnit;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gSetInjectId wrong # args: should be \"",
            argv[0], " Unit Number\"", (char *) NULL);
        return TCL_ERROR;
    }
    unit = atoi(argv[1]);
    DPRINT1(1,"gSetInjectId: %d \n",unit);
    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gSetInjectId:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    oldUnit = gilSelectInj(pGilObjId, unit);

    /* return previous Gilson Unit Id */
    sprintf(interp->result,"%d",oldUnit);
    DPRINT1(1,"Previous Unit: %d\n",oldUnit);
#else
    diagPrint(NULL, "gSetInjectId: %d \n",unit);
    sprintf(interp->result, "%d", (int)22);
#endif

    return TCL_OK;
}


/*----------------------------------------------------*/
static int AirGap( double volume, double flowrate)
{
    int result;

    if ( gilMoveZ2Top(pGilObjId) == -1 )
    {
        ErrMsge = gilErrorMsge(pGilObjId);
        return(-1);
    }

    result = gilsonAspirate(pGilObjId, volume, safeflow(flowrate), 0, 0);
    if ( result == -1)
    {
        ErrMsge = gilErrorMsge(pGilObjId);
    }
    return result;
}


static int Go2LiqLevel(int rackloc, int zone, int sample,
                       int height, int depth )
{
    RACKOBJ_ID pSampRack;
    int samptop, sampbot, liqtop, depthlevel;
    int result;

    result = 0;

    pSampRack = g215Bed.LoadedRacks[rackloc];

    samptop = rackSampTop(pSampRack,zone,sample);
    sampbot = rackSampBottom(pSampRack,zone,sample);
    if (DebugLevel > 0)
       errLogRet(LOGOPT, debugInfo,
              "samptop=%d sampbot=%d\n", samptop, sampbot);

    liqtop = sampbot + height;

    if (liqtop < sampbot)
    {
        liqtop = sampbot;
    }
    else if (liqtop > samptop)
    {
        liqtop = samptop;
    }

    DPRINT3(1, "Go2LiqLevel: liq level: %d = bot: %d + height: %d\n",
            liqtop, sampbot, height);
    if (depth != NOSEEK)
    {
        if ( gilMoveZ(pGilObjId, samptop) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            return(-1);
        }

        /* either at liquid surface or safety */
        if ( gilMoveZLQ(pGilObjId,liqtop) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            return(-1);
        }
        gilGetZ(pGilObjId,&liqtop);
        depthlevel = liqtop - depth;

        /* if depth is higher than where Liq was found
         *   then stay where we are at
         */
        depth = (depthlevel > liqtop) ? liqtop : depthlevel;

        DPRINT3(1, "Go2LiqLevel: liqtopat : %d, depthlevel: %d, bottom: %d\n",
                liqtop, depth, sampbot);

        /*
         * If depth is below the sample bottom then
         *   only go the sample bottom
         */
        depth = (depth < sampbot) ? sampbot : depth;
        if ( gilMoveZ(pGilObjId, depth) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            return(-1);
        }
    }
    else
    {
        if ( gilMoveZ(pGilObjId, liqtop) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            return(-1);
        }
    }

    return (0);
}

/*----------------------------------------------------
*  gMove2LiqLevel
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMove2LiqLevel(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int i, xAxis, yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone,times,height,depth;

    if (argc != 6)
    {
        Tcl_AppendResult(interp, "gMove2LiqLevel wrong # args: should be \"",
            argv[0], " RackLoc SampleZone SampleNumber Height Depth \"",
            (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    sample = atoi(argv[3]);
    height = atoi(argv[4]);

    if (strcmp(argv[5],"NOSEEK") == 0)
        depth = NOSEEK;
    else
        depth = atoi(argv[5]);

    DPRINT5(1, "gMove2LiqLevel: Rack Loc: %d, Zone: %d, Sample: %d, "
            "Height: %d, Depth: %d\n", rackloc, zone, sample, height, depth);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMove2LiqLevel:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if (Go2LiqLevel(rackloc, zone, sample, height, depth ) == -1)
    {
        Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }

#else
    diagPrint(NULL, "gMove2LiqLevel: Rack Loc: %d, Zone: %d, Sample: %d, "
              "Height: %d, Depth: %d\n",
              rackloc, zone, sample, height, depth);
    printf("gMove2LiqLevel: Rack Loc: %d, Zone: %d, Sample: %d, "
           "Height: %d, Depth: %d\n",
	   rackloc, zone, sample, height, depth);
#endif
    return TCL_OK;
}

/*----------------------------------------------------
*  gMix
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gMix(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int i, xAxis, yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone,times,height;
    double volume,flowrate;
    int error;

    if (argc != 8)
    {
        Tcl_AppendResult(interp, "gMix wrong # args: should be \"", argv[0],
          " RackLoc SampleZone SampleNumber Height Volume Times FlowRate \"",
          (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    sample = atoi(argv[3]);
    height = atoi(argv[4]);
    volume = atof(argv[5]);
    times = atoi(argv[6]);
    flowrate = atof(argv[7]);

    if ( (error = ChkLimits(interp,"gMix",rackloc,zone,sample)) != 0)
    {
        Tcl_SetErrorCode(interp,tclErrCode,(char *) NULL);
        return TCL_ERROR;
    }

    DPRINT7(1,"gMix: Rack Loc: %d, Zone: %d, Sample: %d, Height: %d, "
        "Vol: %.1lf, Times: %d, FlowRate: %.1lf \n",
        rackloc, zone, sample, height, volume, times, flowrate);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gMix:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    AirGap(30.0, 1.0);  /* 30 ul air gap for mixing */
    VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);

    /* Make sure Volume is OK */
    volume = safevolume(volume,IN);
    flowrate = safeflow(flowrate);
    gilMoveZ2Top(pGilObjId);

    if ( Move2Sample(rackloc, zone, sample) == -1)
    {
        Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }

    if (Go2LiqLevel(rackloc, zone, sample, height, NOSEEK ) == -1)
    {
        Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
        return TCL_ERROR;
    }

    for( i = 0; i < times; i++)
    {
        if( gilsonAspirate(pGilObjId, volume, flowrate, 0, 0) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
            return TCL_ERROR;
        }
        if ( gilsonDispense(pGilObjId, volume,flowrate, 0, 0) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
            return TCL_ERROR;
        }
    }
    gilMoveZ2Top(pGilObjId);
    /* get rid of air gap from mixing */
    gilsonDispense(pGilObjId, 30.0,flowrate, 0, 0);

#else
    diagPrint(NULL, "gMix: Rack Location: %d, Sample Zone: %d, Sample "
              "Number: %d\n", rackloc, zone, sample);
    printf("gMix: Rack Location: %d, Sample Zone: %d, Sample "
           "Number: %d\n", rackloc, zone, sample);
#endif

    return TCL_OK;
}


/*----------------------------------------------------
*  gTransfer
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gTransfer(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int xAxis,yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSrcRack,pDstRack;
    int racksrc,samplesrc,zonesrc,heightsrc;
    int rackdst,sampledst,zonedst,heightdst;
    double volume,volleft,flowsrc,flowdst;
    int error;

    if (argc != 12)
    {
        Tcl_AppendResult(interp, "gTransfer wrong # args: should be \"",
            argv[0],
            " Volume SrcRack SrcZone SrcSample SrcHeight SrcFlow DstRack "
            "DstZone DstSample DstHeight DstFlow\"", (char *) NULL);
        return TCL_ERROR;
    }

    volume = atof(argv[1]);

    racksrc = atoi(argv[2]);
    zonesrc = atoi(argv[3]);
    samplesrc = atoi(argv[4]);
    heightsrc = atoi(argv[5]);
    flowsrc = atof(argv[6]);

    rackdst = atoi(argv[7]);
    zonedst = atoi(argv[8]);
    sampledst = atoi(argv[9]);
    heightdst = atoi(argv[10]);
    flowdst = atof(argv[11]);

    DPRINT6(1,"gTransfer: Volume= %.1lf, Src: Loc %d, Zone %d, Sample %d, "
        "Height %d, Flow: %.1lf \n",
        volume, racksrc, zonesrc, samplesrc, heightsrc, flowsrc);
    DPRINT5(1,"                        Dst: Loc %d, Zone %d, Sample %d, "
            "Height %d, Flow: %.1lf \n",
            rackdst, zonedst, sampledst, heightdst, flowdst);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
        return TCL_ERROR;
    }

    if ((error = ChkLimits(interp,"gTransfer",racksrc,zonesrc,samplesrc))
        != 0)
    {
        Tcl_SetErrorCode(interp,tclErrCode,(char *) NULL);
        return TCL_ERROR;
    }

    if ((error = ChkLimits(interp,"gTransfer",rackdst,zonedst,sampledst))
        != 0)
    {
        Tcl_SetErrorCode(interp,tclErrCode,(char *) NULL);
        return TCL_ERROR;
    }

    volleft = volume;

#if !defined(TESTER) && !defined(STDALONE)
    while (volleft)
    {
        volume = safevolume(volleft,IN);
        gilMoveZ2Top(pGilObjId);
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if ( Move2Sample(racksrc, zonesrc, samplesrc) == -1)
        {
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if (Go2LiqLevel(racksrc, zonesrc, samplesrc, heightsrc, NOSEEK )==-1)
        {
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if (gilsonAspirate(pGilObjId, volume, safeflow(flowsrc), 0, 0) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if (gilMoveZ2Top(pGilObjId) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if ( Move2Sample(rackdst, zonedst, sampledst) == -1)
        {
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if (Go2LiqLevel(rackdst, zonedst, sampledst, heightdst, NOSEEK )==-1)
        {
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        if ( gilsonDispense(pGilObjId, volume,safeflow(flowdst), 0, 0) == -1)
        {
            ErrMsge = gilErrorMsge(pGilObjId);
            Tcl_AppendResult(interp, ", ",ErrMsge,"  ",NULL);
            VolumeInSyringe = (double) gilsonCurrentVolume(pGilObjId);
            return TCL_ERROR;
        }
        if (AbortRobo != 0) 
        {
            Tcl_AppendResult(interp, "gTransfer:  User Aborted",NULL);
            return TCL_ERROR;
        }

        volleft -= volume;
        DPRINT1(1,"gTransfer: Volleft: %0.1lf\n",volleft);
    }

    gilMoveZ2Top(pGilObjId);
#else
    diagPrint(NULL, "gTransfer: Rack Location: %d, Sample Zone: %d, Sample "
              "Number: %d\n", rackdst, zonedst, sampledst);
    printf("gTransfer: Rack Location: %d, Sample Zone: %d, Sample "
           "Number: %d\n", rackdst, zonedst, sampledst);
#endif

    return TCL_OK;
}

/*----------------------------------------------------
*  gZSpeed
*
*  For the given vial and flow rate, determine the
*  Z speed that should be used when aspirating and
*  dispensing fluids.
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gZSpeed(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    int xAxis,yAxis;
    int sampZtop,sampZbottom;
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone,error;
    double flowrate,zSpeed;

    if (argc != 5)
    {
        Tcl_AppendResult(interp, "gZSpeed wrong # args: should be \"",
          argv[0], " RackLoc SampleZone SampleNumber FlowRate\"",
          (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    sample = atoi(argv[3]);
    flowrate = atof(argv[4]);
    flowrate = safeflow(flowrate);

    DPRINT4(1,
        "gZSpeed: Rack Loc: %d, Zone: %d, Sample: %d, FLowrate: %.1lf \n",
        rackloc, zone, sample, flowrate);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gZSpeed:  User Aborted",NULL);
        return TCL_ERROR;
    }

    if ( (error = ChkLimits(interp,"gZSpeed",rackloc,zone,sample)) != 0)
    {
        Tcl_SetErrorCode(interp,tclErrCode,(char *) NULL);
        return TCL_ERROR;
    }

    pSampRack =  g215Bed.LoadedRacks[rackloc];
    zSpeed = rackFlow2Zspeed(pSampRack, flowrate, zone, sample);
    sprintf(interp->result,"%.1lf",zSpeed);
    DPRINT1(1,"gZSpeed: Z Speed  %.1lf\n",zSpeed);

    return TCL_OK;
}

#ifndef GILSCRIPT
/*****************************************************
*
*  ResumeAcq
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gResumeAcq(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    extern void reportRobotStat(int robstat);
    if (argc != 1)
    {
        Tcl_AppendResult(interp, "ResumeAcq wrong # args: should be \"",
            argv[0], " \"", (char *) NULL);
        return TCL_ERROR;
    }

    /* tell robot completed OK, then acquisition while continue */
    reportRobotStat( 0 );

    return TCL_OK;
}
#endif


/*****************************************************
*
*  gRackLocTypeMap
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gRackLocTypeMap(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    char racktype[80];
    int rackloc,i,index,failed;

    if (argc != 3)
    {
        Tcl_AppendResult(interp,
            "gRackLocTypeMape wrong # args: should be \"", argv[0],
            " RackLoc SampleZone StartLoc Pattern \"", (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    strcpy(racktype,argv[2]);

    if ((rackloc < 1) && (rackloc > GIL_MAX_RACKS))
    {
        Tcl_AppendResult(interp, "gRackLocTypeMap(): Rack Location \"",
            rackloc, " is not within ( 1 - 6 ) bounds \"", (char *) NULL);
        return TCL_ERROR;
    }

    failed = 1;
    for(i=0;i < g215Bed.NumRacks; i++)
    {
        if (strcmp(g215Bed.DefinedRacks[i].IdStr,racktype) == 0)
        {
	    g215Bed.LoadedRacks[rackloc] = g215Bed.DefinedRacks[i].pRackObj;
            failed = 0;
            break;
        }
    }
    DPRINT2(1, "gRackLocTypeMap: Rack Loc: %d, Type: %s\n",
            rackloc, racktype);
    if (failed)
    {
        Tcl_AppendResult(interp, "gRackLocTypeMape racktype \"", racktype,
              " Not Found \"", (char *) NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

static int decodeOrder(char* startloc, char* pattern)
{
    int order = NW_2_E;  /* default */
    if (strcmp(startloc,"NW") == 0)
    {
        if (strcmp(pattern,"HST") == 0)
        {
            order = NW_2_E;
        }
        else if (strcmp(pattern,"HZZ") == 0)
        {
            order = NW_2_E_ZIGZAG;
        }
        else if (strcmp(pattern,"VST") == 0)
        {
            order = NW_2_S;
        }
        else if (strcmp(pattern,"VZZ") == 0)
        {
            order = NW_2_S_ZIGZAG;
        }
    }
    else if (strcmp(startloc,"NE") == 0)
    {
        if (strcmp(pattern,"HST") == 0)
        {
            order = NE_2_W;
        }
        else if (strcmp(pattern,"HZZ") == 0)
        {
            order = NE_2_W_ZIGZAG;
        }
        else if (strcmp(pattern,"VST") == 0)
        {
            order = NE_2_S;
        }
        else if (strcmp(pattern,"VZZ") == 0)
        {
            order = NE_2_S_ZIGZAG;
        }
    }
    else if (strcmp(startloc,"SW") == 0)
    {
        if (strcmp(pattern,"HST") == 0)
        {
            order = SW_2_N;
        }
        else if (strcmp(pattern,"HZZ") == 0)
        {
            order = SW_2_N_ZIGZAG;
        }
        else if (strcmp(pattern,"VST") == 0)
        {
            order = SW_2_E;
        }
        else if (strcmp(pattern,"VZZ") == 0)
        {
            order = SW_2_E_ZIGZAG;
        }
    }
    else if (strcmp(startloc,"SE") == 0)
    {
        if (strcmp(pattern,"HST") == 0)
        {
            order = SE_2_W;
        }
        else if (strcmp(pattern,"HZZ") == 0)
        {
            order = SE_2_W_ZIGZAG;
        }
        else if (strcmp(pattern,"VST") == 0)
        {
            order = SE_2_N;
        }
        else if (strcmp(pattern,"VZZ") == 0)
        {
            order = SE_2_N_ZIGZAG;
        }
    }

    return(order);
}

/*----------------------------------------------------
*  gRackZoneSequenceOrder
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gRackZoneSequenceOrder(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    char startloc[80], pattern[80];
    RACKOBJ_ID pSampRack;
    int rackloc,sample,zone,order;

    if (argc != 5)
    {
        Tcl_AppendResult(interp, "gRackZoneSequenceOrder wrong # args: "
           "should be \"", argv[0], " RackLoc SampleZone StartLoc Pattern \"",
           (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    zone = atoi(argv[2]);
    strcpy(startloc,argv[3]);
    strcpy(pattern,argv[4]);

    DPRINT4(1,
        "gRackZoneSequenceOrder: Rack Loc: %d, Zone: %d, StartLoc: '%s', "
        "Pattern: '%s'\n", rackloc, zone, startloc, pattern);

    pSampRack = g215Bed.LoadedRacks[rackloc];

    order = decodeOrder(startloc,pattern);

    rackZoneWellOrder(pSampRack, zone, order);

    return TCL_OK;
}

/*----------------------------------------------------
*  gRackSequenceOrder
*
+----------------------------------------------------*/
        			/* ARGSUSED */
int gRackSequenceOrder(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    char startloc[80], pattern[80];
    RACKOBJ_ID pSampRack;
    int rackloc,sample,order;

    if (argc != 4)
    {
        Tcl_AppendResult(interp,
            "gRackSequenceOrder wrong # args: should be \"",
            argv[0], " RackLoc StartLoc Pattern \"", (char *) NULL);
        return TCL_ERROR;
    }

    rackloc = atoi(argv[1]);
    strcpy(startloc,argv[2]);
    strcpy(pattern,argv[3]);

    DPRINT3(1, "gRackSequenceOrder: Rack Loc: %d, StartLoc: '%s', Pattern: "
            "'%s'\n", rackloc, startloc, pattern);

    pSampRack = g215Bed.LoadedRacks[rackloc];

    order = decodeOrder(startloc,pattern);

    rackWellOrder(pSampRack, order);

    return TCL_OK;
}

int gPuts(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{ 
    char *msge;

    if (argc != 2)
    {
        Tcl_AppendResult(interp, "gPuts wrong # args: should be \"", argv[0],
             " String \"", (char *) NULL);
        return TCL_ERROR;
    }

    if (DebugOutput)
        diagPrint(NULL, "%s", argv[1]);

    return TCL_OK;
}

/*  direct commands to the gilson family of equipment */
/*  Immediate and Buffer command routines callable	*/
/*  from the tcl scripts				*/
int gCommand(dummy, interp, argc, argv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int argc;                           /* Number of arguments. */
    char **argv;                        /* Argument strings. */
{
    int UnitID;
    char *Cmd;
    char *CmdType;
    char rString[256];

    if (argc != 4)
    {
        Tcl_AppendResult(interp, "gCommand wrong # args: should be \"", 
            argv[0], " ID Number Cmd CmdType\"", (char *) NULL);
        return TCL_ERROR;
    }


    UnitID = atoi(argv[1]);
    Cmd = (char *)argv[2];
    CmdType = (char *)argv[3];

    DPRINT3(1,"gCommand: UnitId: %d, Cmd: '%s' CmdType: '%s'\n",
            UnitID, Cmd, CmdType);

    if (AbortRobo != 0) 
    {
        Tcl_AppendResult(interp, "gFlush:  User Aborted",NULL);
        return TCL_ERROR;
    }

#if !defined(TESTER) && !defined(STDALONE)
    if (gilCommand(pGilObjId, UnitID, Cmd, CmdType, rString ) == -1)
    {
        Tcl_AppendResult(interp, "gCommand: Command Error",NULL);
        /* ErrMsge = gilErrorMsge(pGilObjId); */
        /* Tcl_AppendResult(interp, ",\nGilson Error: ",ErrMsge,"  ",NULL); */
        return TCL_ERROR;
    }

    /* return what the Gilson returned */
    sprintf(interp->result,"%s",rString);
    DPRINT1(1,"returned String:'%s'\n",rString);
#else
    diagPrint(NULL,"gCommand: UnitID: %d, Cmd: '%s', CmdType: '%s'\n",
	UnitID,Cmd,CmdType);
    printf("gCommand: UnitID: %d, Cmd: '%s', CmdType: '%s'\n",
	UnitID,Cmd,CmdType);
    sprintf(interp->result,"%s","TESTER&STDALONE");
#endif

    return TCL_OK;

}
/*----------------------------------------------------*/

/* 
 * tclAppInit.c --
 *
 *	Provides a default version of the main program and Tcl_AppInit
 *	procedure for Tcl applications (without Tk).
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

#ifdef XXXXX
/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tcl_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;			/* Number of command-line arguments. */
    char **argv;		/* Values of command-line arguments. */
{
    Tcl_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{

    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*---------------------- GILSON COMMANDS -------------------------*/

    Tcl_CreateCommand(interp, "gWriteDisplay", gWriteDisplay,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMoveZ2Top", gMoveZ2Top,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMove2RinseStation", gMove2RinseStation,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gFlush", gFlush,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMove2InjectorPort", gMove2InjectorPort,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gDelayMsec", gDelayMsec,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gSetContacts", gSetContacts,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gGetContacts", gGetContacts,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gGetInputs", gGetInputs,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gAspirate", gAspirate,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gDispense", gDispense,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMove2Sample", gMove2Sample,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMoveZ", gMoveZ,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gInjector2Load", gInjector2Load,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gInjector2Inject", gInjector2Inject,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gCurrentSyrVol", gCurrentSyrVol,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "gAspirateAsync", gAspirateAsync,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gDispenseAsync", gDispenseAsync,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gStopTestAll", gStopTestAll,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gTubeX", gTubeX,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gTubeY", gTubeY,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMoveZLQ", gMoveZLQ,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gZSpeed", gZSpeed,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMix", gMix,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gTransfer", gTransfer,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gMove2LiqLevel", gMove2LiqLevel,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gRackZoneSequenceOrder", gRackZoneSequenceOrder,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gRackSequenceOrder", gRackSequenceOrder,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "gRackLocTypeMap", gRackLocTypeMap,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gPuts", gPuts,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gSetUnitId", gSetUnitId,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gSetPumpId", gSetPumpId,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gSetInjectId", gSetInjectId,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gInitSyringe", gInitSyringe,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
 
    Tcl_CreateCommand(interp, "gCommand", gCommand,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

#ifndef GILSCRIPT
    Tcl_CreateCommand(interp, "ResumeAcq", gResumeAcq,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_LinkVar(interp, "err768AS", (char *)&err768AS, TCL_LINK_INT);
    Tcl_LinkVar(interp, "rack768AS", (char *)&rack768AS, TCL_LINK_INT);
    Tcl_LinkVar(interp, "zone768AS", (char *)&zone768AS, TCL_LINK_INT);
    Tcl_LinkVar(interp, "well768AS", (char *)&well768AS, TCL_LINK_INT);
#endif
 
  /* global RinseVolume MaxFlow InjectFlow InitialProbeVolume InitialVolumeFlow ExtraVolume Flushrate */
    /* Tcl_LinkVar(interp, "varname", (char *) addr, TCL_LINK_INT); */
    /* Tcl_LinkVar(interp, "varname", (char *) addr, TCL_LINK_DOUBLE); */
    /* Tcl_LinkVar(interp, "varname", (char *) addr, TCL_LINK_BOOLEAN); */
    /* Tcl_LinkVar(interp, "varname", (char *) addr, TCL_LINK_STRING); */

    Tcl_LinkVar(interp, "AbortRobo", (char *)&AbortRobo, TCL_LINK_INT);

    /* generated from C reflected to TCL scripts */
    Tcl_LinkVar(interp, "SampleInfoFile", (char *) SampleInfoFile, TCL_LINK_STRING);

    Tcl_LinkVar(interp, "MaxVolume", (char *) &g215Bed.MaxVolume, TCL_LINK_INT);
    Tcl_LinkVar(interp, "MaxFlow", (char *) &g215Bed.MaxFlow, TCL_LINK_DOUBLE);
    Tcl_LinkVar(interp, "debug", (char *) &DebugOutput, TCL_LINK_INT);

#ifdef XXX
    /* TCL set reflect to C to calc ZSpeed and Vol2Ztravel */
    Tcl_LinkVar(interp, "SampleVolume", (char *) &lastVolume, TCL_LINK_DOUBLE);
    Tcl_LinkVar(interp, "SampleWellRate", (char *) &lastFlowRate, TCL_LINK_DOUBLE);

    /* calculated in C reflected back to TCL scripts */
    Tcl_LinkVar(interp, "SampleZSpeed", (char *) &lastZSpeed, TCL_LINK_DOUBLE);
    Tcl_LinkVar(interp, "SampleVol2ZTravel", (char *) &lastVol2ZTravel, TCL_LINK_DOUBLE);
#endif

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    /* Tcl_SetVar(interp, "tcl_rcFileName", "~/.tclshrc", TCL_GLOBAL_ONLY); */
    Tcl_SetVar(interp, "tcl_rcFileName", "/vnmr/asm/.tclrobo", TCL_GLOBAL_ONLY);
    return TCL_OK;
}




#ifdef XXXX



#define ZONE1 1
#define ZONE2 2
#define ZONE3 3

static  GILSONOBJ_ID pGilObjId;
static  RACKOBJ_ID inject,rack,rack2,rack3;

static int RackCenter[2] = { 550, 1870 };
static int InjectorCenter[2] = { 5576, 38 };
static int InjectorBot = 840;

static double Flowrate = 4.0;
static double Injectrate = 2.0;
static double Retrvrate = 0.5;
static double Flushrate = 16.0; 
static double MaxFlow = 0.0;
static int MaxVolume = 0;
static int SampStrt = 8;
static int SampEnd = 9;
static double Volume = 350.0;	/* ul */
static double RVolume = 400.0;
static double PVolume = 370.0;
static double ExtraVol = 25;

static int Xincr,Yincr,Zincr;
static int XYLoc[2],ZLoc;
static int InjectCenter[2];

main (int argc, char *argv[])
{
  int done;
  char buffer[256];
  char msge[10];
  char *bptr;
  int xAxis,yAxis;
  int i,maxflow;
  int sampZtop,sampZbottom;
  double flowrate, zspeed,ztravel;


  if (argc != 2)
  {
    fprintf(stderr,"usage:  %s <devicename> (i.e. /dev/term/b)\n", argv[0]);
    exit(1);
  }

  fprintf(stderr,"Init Device: %s\n",argv[1]);

  inject = rackCreate("./m215_inj.grk");
  rackCenter(inject, (550 + (1200 * 4) + 226 ),38);  /* 5565 */
  rackShow(inject, 1);

  rack = rackCreate("./code_205.grk");
  rackCenter(rack, 550,1870);
  rackShow(rack, 1);


  fprintf(stderr,"Init Device: %s\n",argv[1]);
  pGilObjId = gilsonCreate(argv[1],22,29,3);

  xAxis = rackGetX(inject, ZONE1,1);
  yAxis = rackGetY(inject, ZONE1,1);
  sampZtop = rackSampTop(inject,ZONE1,1);
  sampZbottom = rackSampBottom(inject,ZONE1,1);
  /* gilInitInjLoc(pGilObjId,xAxis, yAxis, bottom); */
  gilInitInjLoc(pGilObjId,xAxis, yAxis, 840);

  MaxFlow = gilsonMaxFlowRate(pGilObjId);  /* ml/min */
  MaxVolume = gilsonMaxVolume(pGilObjId); /* ul */

  PrintSet();

}
runSample()
{
  char buffer[256];
  char *bptr;
  char msge[10];
  int xAxis,yAxis;
  int i,a,maxflow;
  int sampZtop,sampZbottom;
  double zspeed,ztravel;
  double MinusVol;
  int error;

  zspeed = rackFlow2Zspeed(rack, Flowrate, ZONE1, 1);
  ztravel = rackVol2ZTrail(rack, Volume, ZONE1, 1);
  fprintf(stderr,"Z Travel: %lf (mm), Speed: %lf (mm/sec), flow time: %lf\n (sec)",
	ztravel,zspeed, Volume / ((Flowrate * 1000.0) / 60.0));

  /* ExtraVol =  5.0 * (((double)MaxVolume)/100.0);	/* 5% of maxvolume */

  error = gilSetContacts(pGilObjId, 2, 0 );
  fprintf(stderr,"Samples %d thru %d, vol: %0.1lf\n",SampStrt,SampEnd,Volume);
  fprintf(stderr,"OK to Proceed? (y or n): ");
  bptr = gets(buffer);
  if (bptr == NULL)
      return;
  if (toupper(buffer[0]) != 'Y')
     return(0);
  fprintf(stderr,"\n\nBeginning Sample Run.\n\n");
  for(i=SampStrt;i<=SampEnd;i++)
  {
      /* ----------  Rinse Needle ------------- */
      fprintf(stderr,"Sample %d\n",i);
      fprintf(stderr,"Rinse: Needle  ");
      gDisplay " Rinse "
      gMove2Top
      gMove2RinseStation
      gFlush Volume In Flow, Out Flow 
      gMove2Injector
      getContacts 2 1

      error = gilWriteDisplay(pGilObjId," Rinse "); /* max of 8 chars */
      error = gilMoveZ2Top(pGilObjId);
      error = gilMove2Rinse(pGilObjId);
      /* error = gilEmptyDiluter(pGilObjId); */
      error = gilsonFlush(pGilObjId, RVolume, Flushrate, Flushrate );
      error = gilMoveZ2Top(pGilObjId);
      if (error == -1)
          break;
 

      /* ------------ Rinse Probe ------------- */
      fprintf(stderr,"Probe  ");
      error = gilMove2Inj(pGilObjId);
      error = gilsonFlush(pGilObjId, PVolume, MaxFlow, Injectrate );
      delayMsec(300);
      error = gilSetContacts(pGilObjId, 2, 1 );
      if (error == -1)
          break;
 
      delayMsec(500);
      gilsonAspirate(pGilObjId, 100.0, Retrvrate, 0, 0);
      gilsonAspirate(pGilObjId, (PVolume+ExtraVol-100.0), Injectrate, 0, 0);
      delayMsec(1500);
      /* delayAwhile(20); */
      error = gilSetContacts(pGilObjId, 2, 0 );
      if (error == -1)
          break;
 
      delayMsec(300);
      error = gilMoveZ2Top(pGilObjId);
      error = gilMove2Rinse(pGilObjId);
      error = gilsonDispense(pGilObjId, (PVolume+ExtraVol), MaxFlow, 0, 0);
      fprintf(stderr,"Needle");
      error = gilsonFlush(pGilObjId, RVolume, Flushrate, Flushrate );
      error = gilMoveZ2Top(pGilObjId);
      fprintf(stderr,"\n");

      if (error == -1)
          break;


      sprintf(msge,"Samp: %d",i);
      gilWriteDisplay(pGilObjId,msge); /* max of 8 chars */

      /* Get Sample Location in Tray */
      xAxis = rackGetX(rack, ZONE1,i);
      yAxis = rackGetY(rack, ZONE1,i);
      sampZtop = rackSampTop(rack,ZONE1,i);
      sampZbottom = rackSampBottom(rack,ZONE1,i);


      /* ----------  Get Sample -------------- */
      /* fprintf(stderr,"xAxis: %d, yAxis: %d\n",xAxis,yAxis); */
      fprintf(stderr,"Sample: Get, ");
      gilMoveXY(pGilObjId,xAxis,yAxis);
      gilMoveZ(pGilObjId,sampZtop);
      /* gilsonAspirate(pGilObjId, 5.0, Flowrate, 0, 0); */
      gilMoveZ(pGilObjId,sampZtop-180);
      error = gilWriteDisplay(pGilObjId,"Aspirate"); /* max of 8 chars */
      if (error == -1)
          break;
 
      gilsonAspirate(pGilObjId, Volume, Flowrate, (int) zspeed, 0);
      error = gilMoveZ2Top(pGilObjId);
      if (error == -1)
          break;

      /* ----------- Inject Sample -----------  */
      fprintf(stderr,"Inject, ");
      gilWriteDisplay(pGilObjId," Inject "); /* max of 8 chars */
      error = gilMove2Inj(pGilObjId);
      if (error == -1)
          break;
 
      gilsonDispense(pGilObjId, Volume, Injectrate, 0, 0);

      /* ---------  Sit Here while Acquiring NMR Data -------------- */
      gilWriteDisplay(pGilObjId,"Data Acq"); /* max of 8 chars */
      /* delayAwhile(5); */

      /* ----------- Remove Sample ------------ */
      gilWriteDisplay(pGilObjId,"Retrieve"); /* max of 8 chars */
      fprintf(stderr,"Retrieve, ");
      gilSetContacts(pGilObjId, 2, 1 );

      gilsonAspirate(pGilObjId, 100.0, Retrvrate, 0, 0);
      gilsonAspirate(pGilObjId, (Volume+ExtraVol-100.0), Injectrate, 0, 0);
      delayMsec(1500);
      error = gilSetContacts(pGilObjId, 2, 0 );
      delayMsec(300);
      if (error == -1)
          break;
	
      /* ------------ Return Sample Back into Sample Tube */
      fprintf(stderr,"Return\n");
      gilMoveZ2Top(pGilObjId);
      gilMoveXY(pGilObjId,xAxis,yAxis);
      gilMoveZ(pGilObjId,sampZtop-100);
      error = gilsonDispense(pGilObjId, Volume+ExtraVol, Flowrate, 0, 0);
      /* error = gilsonDispense(pGilObjId, 5.0, Flowrate, 0, 0); */
      if (error == -1)
          break;
  }
  gilMoveZ2Top(pGilObjId);
  gilMove2Rinse(pGilObjId);
  gilsonFlush(pGilObjId, RVolume, Flushrate, Flushrate );
  gilMoveZ2Top(pGilObjId);
  gilMoveXY(pGilObjId,10,10); 
  gilWriteDisplay(pGilObjId,"Complete"); /* max of 8 chars */
}
 
 
#endif

