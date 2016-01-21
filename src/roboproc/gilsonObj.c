/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* --------------------- gilsonObj.c ----------------------------- */

/* #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>

#include "errLogLib.h"
#include "termhandler.h"
#include "timerfuncs.h"
#include "iofuncs.h"

#define TRUE 1
#define FALSE 0

#include "gilsonObj.h"

extern int AbortRobo;

#ifdef GILALIGN
    extern int verbose;
#endif

int bypassInit = 0;

/*
modification history
--------------------
3-05-97,gmb  created 

*/

/*
DESCRIPTION
*/
/* ------------------- Rack Object Structure ------------------- */
static char cr = (char) 13; 
static char lf = (char) 10;
static char ack = (char) 6;
static char eom = (char) 3;
static char period = (char) 46;
static char Mesg[256];
static char Respon[512];
static char Resp[512];
static char gErrorMsge[512];
static char *ErrorMessage;

/* if set to some other than -1, then gilErrorMsge() dosn't read the Gilson
   for an error, it uses the Error Msge in gErrorMsge instead
*/
static int  gilerrno;

/* Last attempted x/y/z positions change, for error 24-29 */
static int  Xsetpos,Ysetpos,Zsetpos;

/* gilson uses 150msec delay between checks */
static int  TimeDelay = 150;

static int gilsonGetRStation(GILSONOBJ_ID pGilId);
static int setNVaddr(GILSONOBJ_ID pGilId, short unitId,int address);
static int getNVint(GILSONOBJ_ID pGilId, short unitId);
static int setNVvalue(GILSONOBJ_ID pGilId, short unitId,
                      int address, int value);
static int gilsonICmd(GILSONOBJ_ID pGilId, short unitId,
                      char cmd, char *resp);
static int gilsonBCmd(GILSONOBJ_ID pGilId, short unitId,
                      char *cmd, char *resp);


/* Global Declarations - Should be defined in a .h file somewhere */
int gilInitSyringe(GILSONOBJ_ID pGilId, int size);
int gilSelectUnit(GILSONOBJ_ID pGilId, int gilunit);
int gilSelectInj(GILSONOBJ_ID pGilId, int injunit);
int gilSelectPump(GILSONOBJ_ID pGilId, int pumpunit);


/*****************************************************************************
                        g i l S e l e c t U n i t
    Description:

*****************************************************************************/
int gilSelectUnit(GILSONOBJ_ID pGilId, int gilunit)
{
    int oldId = pGilId->gilsonID;

    pGilId->gilsonID = gilunit + 128;   /* Gilson ID */
    return(oldId);
}

/*****************************************************************************
                        g i l S e l e c t I n j
    Description:

*****************************************************************************/
int gilSelectInj(GILSONOBJ_ID pGilId, int injunit)
{
    int oldId = pGilId->injectID;

    pGilId->injectID = injunit + 128;   /* Gilson ID */
    return(oldId);
}

/*****************************************************************************
                       g i l S e l e c t P u m p
    Description:

*****************************************************************************/
int gilSelectPump(GILSONOBJ_ID pGilId, int pumpunit)
{
    int oldId = pGilId->pumpID;

    pGilId->pumpID = pumpunit + 128;   /* Gilson ID */
    return(oldId);
}

/*****************************************************************************
                        g i l s o n R e s e t
    Description:
    Resets Gilson.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonReset(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonICmd(pGilId, pGilId->gilsonID,'$',resp);
    delayAwhile(1);

    return(result);
}

/*****************************************************************************
                     g i l s o n G e t X Y Z M i n M a x
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97
    Modified:   Debby Sillman 7/2004
                Modified to notice and throw away bogus responses
                since it was observed that the Gilson sometimes
                returns 0,0,0 value as a response to the 'Q' command.

*****************************************************************************/
int gilsonGetXYZMinMax(GILSONOBJ_ID pGilId)
{
    char resp[256];
    char *answer, *strval1,*strval2;
    int stat, data, complete, axisflag, cnt, min, max;

    if (pGilId == NULL)
        return(-1);

    complete = axisflag = cnt = 0;
    while (!complete && !AbortRobo)
    {
        stat = gilsonICmd(pGilId, pGilId->gilsonID,'Q',resp);
        /*
         * Returned string
         * E.G.: "X= 0000/ 5903", or
         *       "Y= 0000/ 3304" or
         *       "Z= 0000/ 1250"
         */

        DPRINT1(1, "gilsonGetXYZMinMax: response: '%s'\n",resp);

        strval1 = strtok(&resp[2],"/");  
        strval2 = strtok(NULL,"/");  

        DPRINT2(1, "gilsonGetXYZMinMax: strval1: '%s', strval2: '%s'\n",
               strval1, strval2);

        switch(resp[0])
        {
        case 'X':
        case 'x':
            min = atoi(strval1);
            max = atoi(strval2);
            if ((min != max) && (max > min)) {
                pGilId->X_MinMax[0] = min;
                pGilId->X_MinMax[1] = max;
                axisflag |= 0x1;
            }
            break;
        case 'Y':
        case 'y':
            min = atoi(strval1);
            max = atoi(strval2);
            if ((min != max) && (max > min)) {
              pGilId->Y_MinMax[0] = min;
              pGilId->Y_MinMax[1] = max;
              axisflag |= 0x2;
            }
            break;
        case 'Z':
        case 'z':
            min = atoi(strval1);
            max = atoi(strval2);
            if ((min != max) && (max > min)) {
              pGilId->Z_MinMax[0] = min;
              pGilId->Z_MinMax[1] = max;
              axisflag |= 0x4;
            }
            break;
        default:
            break;
        }

        /* printf("gilsonGetXYZMinMax: cnt = %d, axisflag = %x\n",cnt,
         *        axisflag);
         */

        if ((cnt++ > 6) || (axisflag == 0x7))
            complete = 1;

    } /* end while */

    DPRINT2(1,"gilsonGetXYZMinMax: XminMax: %d, %d\n",pGilId->X_MinMax[0],
            pGilId->X_MinMax[1]);
    DPRINT2(1,"gilsonGetXYZMinMax: YminMax: %d, %d\n",pGilId->Y_MinMax[0],
            pGilId->Y_MinMax[1]);
    DPRINT2(1,"gilsonGetXYZMinMax: ZminMax: %d, %d\n",pGilId->Z_MinMax[0],
            pGilId->Z_MinMax[1]);

    return 0;
}
    
/*****************************************************************************
                        g i l s o n D e l e t e
    Description:
    Closes the i/o device and frees the gilson object.

*****************************************************************************/
void gilsonDelete(GILSONOBJ_ID pGilId)
{
    if (pGilId != NULL) {
        if ((pGilId->liqDevEntry != NULL) && (pGilId->liqDev != 0))
            (pGilId->liqDevEntry)->close(pGilId->liqDev);
        free(pGilId);
        pGilId = NULL;
    }
}

/*****************************************************************************
                        g i l s o n C r e a t e
    Description:
    Create the Gilson 215 Object. Opens and initializes the i/o channel.
    Resets and Homes the Gilson and gets valus from the Gilson
      non-volatile RAM.

    Input Parameters:
      devName = "GIL_TTYA", "GIL_TTYB", or "HRM_GILSON"
      gilunit = 22;
      injunit = 29;

    Returns:
    GILSONOBJ_ID       - Success
    NULL               - Error

    Author             Greg Brissey 3/5/97

*****************************************************************************/
GILSONOBJ_ID gilsonCreate(char *devName,
                          int gilunit, int injunit, int pumpunit)
{
    GILSONOBJ_ID pGilsonObj;
    int result, powered;

    int    liqDev;
    ioDev *liqDevEntry;

    gilerrno = -1;

    /* ------- malloc space for STM Object --------- */
    if ( (pGilsonObj = (GILSON_OBJ *) malloc( sizeof(GILSON_OBJ)) ) == NULL )
    {
        errLogSysRet(ErrLogOp, debugInfo, "gilsonCreate: ");
        return(NULL);
    }

    /* Zero out structure so we don't free something by mistake */
    memset(pGilsonObj, 0, sizeof(GILSON_OBJ));

    pGilsonObj->pSID = "";
    gilSelectUnit(pGilsonObj, ((gilunit == 0) ? 22 : gilunit));
    gilSelectInj(pGilsonObj, ((injunit == 0) ? 29 : injunit));
    gilSelectPump(pGilsonObj, ((pumpunit == 0) ? 3 : pumpunit));

    /* Initialize i/o port to Gilson */
    liqDev = -1;
    liqDevEntry = &devTable[0];
    while (liqDevEntry->devName != NULL) {
        if (strcmp(liqDevEntry->devName, devName) == 0) {
            liqDev = liqDevEntry->open(devName);
            liqDevEntry->ioctl(liqDev, IO_FLUSH);
            break;
        }
        liqDevEntry++;
    }
    if (liqDev < 0) {
        errLogRet(LOGOPT, debugInfo,
                  "gilsonCreate: unable to init Gilson i/o channel.\n");
        gilsonDelete(pGilsonObj);
        return NULL;
    }

    pGilsonObj->liqDevEntry = liqDevEntry;
    pGilsonObj->liqDev = liqDev;

    /* Regular Gilson 215 has an integrated pump */
    if (strncmp(devName, "HRM_GILSON", 10)) {
        /* Regular 215: Get pump type and volume from NV RAM */
        setNVaddr(pGilsonObj,pGilsonObj->gilsonID, NV_PUMP);
        pGilsonObj->pumpType =  getNVint(pGilsonObj, pGilsonObj->gilsonID);
        pGilsonObj->pumpVolume =  getNVint(pGilsonObj, pGilsonObj->gilsonID);
    }
    else {
        /* Hermes 215SW: always 402B pump, only get syringe size from NVRAM */
        setNVaddr(pGilsonObj,pGilsonObj->gilsonID, 10);
        pGilsonObj->pumpVolume =  getNVint(pGilsonObj, pGilsonObj->gilsonID);

        /* Default is 1ml syringe */
        if (pGilsonObj->pumpVolume == 0)
            pGilsonObj->pumpVolume = 1000;

        /* Update NVRAM */
        gilsonSetPump(pGilsonObj, 4, pGilsonObj->pumpVolume);

        /* Make sure NV_ZSENSE is zero (=1 for hi-power Z arm) */
        setNVvalue(pGilsonObj, pGilsonObj->gilsonID, NV_ZSENSE, 0);
    }

    /* Set Z Arm Height */
    setNVaddr(pGilsonObj,pGilsonObj->gilsonID, NV_ZTRAVEL);
    pGilsonObj->Ztravel =
        ( (getNVint(pGilsonObj, pGilsonObj->gilsonID) == 0) ? 125 : 175 );

    /* Read max Z motor position */
    setNVaddr(pGilsonObj, pGilsonObj->gilsonID, NV_ZTOPCLAMP);
    pGilsonObj->ZTopClamp =  getNVint(pGilsonObj, pGilsonObj->gilsonID);

    /* Get XYZ min/max motor positions */
    gilsonGetXYZMinMax(pGilsonObj);

    /* Get rinse station X/Y/Z coord */
    gilsonGetRStation(pGilsonObj);

    if (!bypassInit) {
        powered = gilsonPowered(pGilsonObj);

#ifdef GILALIGN
        if (verbose)
            DPRINT1(0, "Gilson motors are%spowered.\r\n",
                    powered ? " ":" not ");
#endif

        /* Reset gilson, if any axis or pump is not powered up and ready */
        if (!powered && !AbortRobo)
        {
            result = gilsonReset(pGilsonObj);
            if (result == -1)
            {
                errLogRet(ErrLogOp, debugInfo, "gilsonCreate: Reset Failed. "
                          "Gilson not responding to Cmds.\n");
                gilsonDelete(pGilsonObj);
                return(NULL);
            }
            else
            {
                /* Home gilson */
                result = gilsonHome(pGilsonObj);
                if (result < 0) {
                    errLogRet(ErrLogOp, debugInfo, "gilsonCreate: "
                              "Home Failed.\n");
                    gilsonDelete(pGilsonObj);
                    return(NULL);
                }
            }
        }
    }

    return(pGilsonObj);
}

/*****************************************************************************
                        g i l I n i t I n j L o c
    Description:
    Set the Inject Loction X/Y/Z.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilInitInjLoc(GILSONOBJ_ID pGilId, int X, int Y, int Z)
{
    if (pGilId == NULL)
        return(-1);

    pGilId->InjectStation[0] = X;
    pGilId->InjectStation[1] = Y;
    pGilId->InjectStation[2] = Z;
    return(0);
}

/*****************************************************************************
                        g i l s o n S t o p p e d
    Description:

    Returns:
     1          - Powered
     0          - if any axis or pump is unpowered or in error state
    -1          - Error

    Author:    Greg Brissey 5/8/97

*****************************************************************************/
int gilsonStopped(GILSONOBJ_ID pGilId,int X,int Y,int Z,int P)
{
    char resp[256];
    int  Xstop,Ystop,Zstop,Pstop,stop;
    int  Xerr,Yerr,Zerr,Perr,err;
    int  stat;

    if (pGilId == NULL)
        return(-1);

    stat = gilsonICmd(pGilId, pGilId->gilsonID,'M',resp);
/*
 *  resp[4] = 0;
 *  fprintf(stderr,"gilsonStopped: response: '%s'\n", resp);
 */

    if (stat < 0)
        return(-1);  /* TIMEOUT */

    /*
     * R = running,
     * P = powered,
     * U = Unpowered,
     * I = Uninitialized,
     * E = Error
     */

#ifdef GILALIGN
    if (verbose) {
        if (X)
            DPRINT2(0, "X is %s (%c)\r\n",
                     ((resp[0] == 'P') ? "stopped" :
                     ((resp[0] == 'R') ? "running" :
                     ((resp[0] == 'I') ? "not initialized" :
                     ((resp[0] == 'U') ? "unpowered" : "error")))),
                     resp[0]);
        if (Y)
            DPRINT2(0, "Y is %s (%c)\r\n",
                     ((resp[1] == 'P') ? "stopped" :
                     ((resp[1] == 'R') ? "running" :
                     ((resp[1] == 'I') ? "not initialized" :
                     ((resp[1] == 'U') ? "unpowered" : "error")))),
                     resp[1]);
        if (Z)
            DPRINT2(0, "Z is %s (%c)\r\n",
                     ((resp[2] == 'P') ? "stopped" :
                     ((resp[2] == 'R') ? "running" :
                     ((resp[2] == 'I') ? "not initialized" :
                     ((resp[2] == 'U') ? "unpowered" : "error")))),
                     resp[2]);
    }
#endif

    if (pGilId->pumpType == 4) {
      char pumpResp[16];
      int idx = 0;

      stat = gilsonICmd(pGilId, pGilId->pumpID, 'M', pumpResp);
      pumpResp[15] = '\0';
      if (pumpResp[0] == 'M')  /* 'M' = pump motor not installed */
          idx = 7;             /*   check the other pump         */
      resp[3] = pumpResp[idx];

#ifdef GILALIGN
      if (verbose)
          DPRINT2(0, "Pump is %s (%c)\r\n",
                      (((pumpResp[idx] == 'N') ? "stopped" :
                       ((pumpResp[idx] == 'R') ? "running" :
                       ((pumpResp[idx] == 'O') ? "overload" :
                       ((pumpResp[idx] == 'I') ? "not initialized" :
                       ((pumpResp[idx] == 'M') ? "motor missing" :
                       ((pumpResp[idx] == 'H') ? "motion incomplete" :
                       ((pumpResp[idx] == 'W') ? "waiting" : "error")))))))),
                       pumpResp[idx]);
#endif
      if (resp[3] == 'N')
          resp[3] = 'P';       /* 'N' = no errors */
      else if (resp[3] != 'R')
          resp[3] = 'E';       /* 'R' = running   */
    }

#ifdef GILALIGN
    else if (verbose && P) {
        DPRINT2(0, "Pump is %s (%c)\r\n",
                         ((resp[3] == 'P') ? "stopped" :
                         ((resp[3] == 'R') ? "running" :
                         ((resp[3] == 'I') ? "not initialized" :
                         ((resp[3] == 'U') ? "unpowered" : "error")))),
                         resp[3]);
    }
#endif

    Xstop = (resp[0] == 'P');
    Ystop = (resp[1] == 'P');
    Zstop = (resp[2] == 'P');
    Pstop = (resp[3] == 'P');

    Xerr = ((resp[0] != 'R') && (resp[0] != 'P'));
    Yerr = ((resp[1] != 'R') && (resp[1] != 'P'));
    Zerr = ((resp[2] != 'R') && (resp[2] != 'P'));
    Perr = ((resp[3] != 'R') && (resp[3] != 'P'));

    if ( (Xstop || !X ) && 
         (Ystop || !Y ) &&
         (Zstop || !Z ) &&
         (Pstop || !P ) )
        stop = 1;
    else
        stop = 0;

    if ( (Xerr && X ) || 
         (Yerr && Y ) ||
         (Zerr && Z ) ||
         (Perr && P ) )
    {
        /* DPRINT(0, "Error on one or more of the Axis\n"); */

        /* kill power to all axes */
        gilsonBCmd(pGilId, pGilId->gilsonID, "E000", resp);

        DPRINT1(0, "Gilson215 Error: %s\n", gilErrorMsge(pGilId));
        stop = -1;
    }

    return(stop);
}

/*****************************************************************************
                      g i l s o n S t o p p e d A l l
    Description:
    Waits indefinitely for all motors to either be stopped or
    in an error condition.

    Returns:
    0           - Success (all motors stopped)
    -1          - Error (at least one motor is in an error state)

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonStoppedAll(GILSONOBJ_ID pGilId)
{
    int result = -1;

    while (!AbortRobo) {
        result = gilsonStopped(pGilId,TRUE,TRUE,TRUE,TRUE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                        g i l s o n P o w e r e d
    Description:
    Returns the  status of power to axis and pump.

    Returns:
     1         - Powered
     0         - if any axis or pump is unpowered or in error state
    -1         - Error

    Author:    Greg Brissey 5/8/97

*****************************************************************************/
int gilsonPowered(GILSONOBJ_ID pGilId)
{
    int stat;

    if (pGilId == NULL)
        return(-1);
    stat = gilsonStopped(pGilId, 1, 1, 1, 1);
    return( ((stat < 0) ? 0 : 1 ));
}

/*****************************************************************************
                          g i l H o m e X Y Z
    Description:
    Resets & Homes Gilson.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int gilHomeXYZ(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonBCmd(pGilId, pGilId->gilsonID, "H", resp);
    if (result == 0) {
        delayAwhile(3);
        while (!AbortRobo) {
            result = gilsonStopped(pGilId,TRUE,TRUE,TRUE,FALSE);
            if ((result == 1 /* stopped */) || (result < 0) /* error */) {
                break;
            }
            delayMsec(250);
        }
    }

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                       g i l H o m e D i l u t e r
    Description:
    Moves to the rinse station, lowers needle.
    Puts valve in the needle position and sends syringe piston
    to the upper position.  Then raises needle to the Z top.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilHomeDiluter(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int result;

    if (pGilId == NULL)
        return(-1);

    result = -1;
    if (pGilId->pumpType > 0)
    {
        if (pGilId->pumpType != 4)
        {
            result = gilMoveZ(pGilId,pGilId->ZTopClamp);
            if ((result < 0) || AbortRobo)
                return result;

            result = gilMove2Rinse(pGilId);
            if ((result < 0) || AbortRobo)
                return result;

            gilerrno = -1;
            result = gilsonBCmd(pGilId, pGilId->gilsonID, "d", resp);
            if ((result < 0) || AbortRobo)
                return result;

            delayMsec(500);
            while (!AbortRobo)
            {
                result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
                if ((result == 1 /* stopped */) || (result < 0) /* error */)
                    break;
                delayMsec(250);  /* Gilson uses 500 msec here */
            }

            if ((result < 0) || AbortRobo)
                return result;

            result = gilMoveZ(pGilId,pGilId->ZTopClamp);
        }
        else
        {
            result = gilMoveZ(pGilId,pGilId->ZTopClamp);
            if ((result < 0) || AbortRobo)
                return result;

            result = gilMove2Rinse(pGilId);
            if ((result < 0) || AbortRobo)
                return result;

            gilerrno = -1;
            result = gilInitSyringe(pGilId, pGilId->pumpVolume);
            if ((result < 0) || AbortRobo)
                return result;

            result = gilMoveZ(pGilId,pGilId->ZTopClamp);
        }
    }
    return( ((result != -1) ? 0 : -1) );
}

/*****************************************************************************
                       g i l E m p t y D i l u t e r
    Description:
    Puts valve in the needle position and sends syringe piston
    to the upper position.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilEmptyDiluter(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int result;

    if (pGilId == NULL)
        return(-1);

    result = -1;
    if (pGilId->pumpType > 0)
    {
        if (pGilId->pumpType != 4)
        {
            result = gilsonBCmd(pGilId, pGilId->gilsonID, "d", resp);
            if ((result < 0) || AbortRobo)
                return result;

            delayMsec(500);
            while (!AbortRobo) {
                result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
                if ((result == 1 /* stopped */) || (result < 0) /* error */)
                    break;
                delayMsec(250); /* Gilson uses 500 msec here */
            }
        }
        else
        {
            result = gilInitSyringe(pGilId, pGilId->pumpVolume);
        }
    }
    return( ((result != -1) ? 0 : -1) );
}

/*****************************************************************************
                          g i l s o n H o m e
    Description:
    Resets & Homes Gilson & Diluter.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonHome(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int result;
    int Xpos, Ypos;

    Xpos = pGilId->X_MinMax[0] + 20;
    Ypos = pGilId->Y_MinMax[0] + 20;

    if (pGilId == NULL)
        return(-1);

    result = gilWriteDisplay(pGilId," Homing ");  /* max of 8 chars */
    if ((result < 0) || AbortRobo)
        return result;

    result = gilHomeXYZ(pGilId); 
    if ((result < 0) || AbortRobo)
        return result;

    result = gilHomeDiluter(pGilId);
    if ((result < 0) || AbortRobo)
        return result;

    result = gilMoveXY(pGilId,Xpos,Ypos);
    if ((result < 0) || AbortRobo)
        return result;

    result = gilWriteDisplay(pGilId,"");  /* max of 8 chars */
    return( result );
}

/*****************************************************************************
                        g i l X Y Z M i n M a x
    Description:
    Returns the Min & Max of the X, Y & Z axis.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilXYZMinMax(GILSONOBJ_ID pGilId,int *AxisMinMax)
{
    /* Set up some default values */
    if (pGilId->pumpType == 4) {
        AxisMinMax[0] = -329; AxisMinMax[1] = 4974;  /* min max X */
        AxisMinMax[2] = -3;   AxisMinMax[3] = 3291;  /* min max Y */
        AxisMinMax[4] = 550;  AxisMinMax[5] = 2300;  /* min max Z */
    }
    else {
        AxisMinMax[0] = 0; AxisMinMax[1] = 5800;  /* min max X */
        AxisMinMax[2] = 0; AxisMinMax[3] = 3300;  /* min max Y */
        AxisMinMax[4] = 0; AxisMinMax[5] = 1210;  /* min max Z */
    }

    if (pGilId == NULL)
        return(-1);

    /* Min & Max as read by the Q command */
    AxisMinMax[0] = pGilId->X_MinMax[0]; AxisMinMax[1] = pGilId->X_MinMax[1];
    AxisMinMax[2] = pGilId->Y_MinMax[0]; AxisMinMax[3] = pGilId->Y_MinMax[1];
    AxisMinMax[4] = pGilId->Z_MinMax[0]; AxisMinMax[5] = pGilId->Z_MinMax[1];

    return(0);
}

/*****************************************************************************
                             g i l M o v e Z
    Description:
    Moves to the Z coordinates given in tenths of mm.
    E.G.   Z=1000  == Z 100.0 mm
    Larger numbers move the probe higher and further away
    from the Gilson bed.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMoveZ(GILSONOBJ_ID pGilId, int Ztmm)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return -1;

    if ((Ztmm < pGilId->Z_MinMax[0]) || (Ztmm > pGilId->Z_MinMax[1]))
        return -1;

    Zsetpos = Ztmm;

    if (!strncmp(pGilId->liqDevEntry->devName, "HRM_GILSON", 10))
    {
       sprintf(cmd,"v%04d,050.00", Ztmm);
    }
    else
    {
       sprintf(cmd,"Z%d",Ztmm);
    }
    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    delayMsec(250);
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,TRUE,FALSE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                           g i l M o v e Z L Q
    Description:
    Seeks the liquid level till it detects or reaches specified limit.
    E.G.   gilMoveZLQ(1000) seeks liquid till reaches Z 100.0 mm

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMoveZLQ(GILSONOBJ_ID pGilId,int Zmm)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    Zsetpos = Zmm;

    sprintf(cmd,"z%d,1",Zmm);
    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    delayMsec(300);
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,TRUE,FALSE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                        g i l M o v e Z 2 T o p
    Description:
    Moves Z to Top.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMoveZ2Top(GILSONOBJ_ID pGilId)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    Zsetpos = pGilId->ZTopClamp;

    if (!strncmp(pGilId->liqDevEntry->devName, "HRM_GILSON", 10))
    {
       sprintf(cmd,"v%04d,050.00", pGilId->ZTopClamp);
    }
    else
    {
       sprintf(cmd,"Z%d",pGilId->ZTopClamp);
    }
    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    delayMsec(200);
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,TRUE,FALSE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                           g i l R e l a x Z
    Description:
    Frees the Z Coordinate.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilRelaxZ(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonBCmd(pGilId, pGilId->gilsonID, "Exx0", resp);
    return( result );
}

/*****************************************************************************
                           g i l M o v e X Y
    Description:
    Moves to the X/Y Coordinates Given in tenths of mm.
    E.G.   X=1228 Y=38 == X 122.8 mm, Y 3.8mm

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMoveXY(GILSONOBJ_ID pGilId,int Xmm, int Ymm)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    Xsetpos = Xmm;
    Ysetpos = Ymm;

    sprintf(cmd,"X%d/%d",Xmm,Ymm);
    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    delayMsec(200);
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,TRUE,TRUE,FALSE,FALSE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                       g i l M o v e X Y A s y n c
    Description:
    gilMoveXY but don't wait for completion.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMoveXYAsync(GILSONOBJ_ID pGilId,int Xmm, int Ymm)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    Xsetpos = Xmm;
    Ysetpos = Ymm;

    sprintf(cmd,"X%d/%d",Xmm,Ymm);
    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                        g i l M o v e 2 R i n s e
    Description:
    Moves to the  Rinse Station.
    RinseStation[3]; X/Y/Z coordinates in mm of Rinse Station 

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMove2Rinse(GILSONOBJ_ID pGilId)
{
    char cmd[30];
    char resp[256];
    int  result;
    int  Xmax,Xmin,Ymax,Ymin;

    if (pGilId == NULL)
        return(-1);

    Xmin = pGilId->X_MinMax[0];
    Xmax = pGilId->X_MinMax[1];
    Ymin = pGilId->Y_MinMax[0];
    Ymax = pGilId->Y_MinMax[1];

    /* Be sure the needle is at the top before moving in the x/y direction */
    result = gilMoveZ(pGilId,pGilId->ZTopClamp);
    if ((result < 0) || AbortRobo)
        return result;
    delayMsec(200);

    /*
     * RinseStation[3]; X/Y/Z coordinates in mm of Rinse Station
     * Only go there if position is reasonable !
     */
    /* DPRINT4(-1,"gilMove2Rinse: Gilson Min/Max, X= %d / %d, Y = %d / %d\n",
               Xmin,Xmax,Ymin,Ymax); */
    /* DPRINT2(-1,"gilMove2Rinse: Gilson Rinse X/Y Pos: %d / %d\n", 
        pGilId->RinseStation[0],pGilId->RinseStation[1]); */

    if ( ((pGilId->RinseStation[0] >= Xmin) &&
          (pGilId->RinseStation[0] < Xmax)) &&
         ((pGilId->RinseStation[1] >= Ymin) &&
          (pGilId->RinseStation[1] < Ymax)) ) {

        result = gilMoveXY(pGilId, pGilId->RinseStation[0],
                           pGilId->RinseStation[1]);
        if ((result < 0) || AbortRobo)
            return result;

        delayMsec(200);
        result = gilMoveZ(pGilId, pGilId->RinseStation[2]);
    }
    else
    {
        /* Move to default station location */
        result = gilMoveXY(pGilId,Xmin+20,Ymin+20);
        sprintf(gErrorMsge, "Rinse Station X/Y: %d/%d beyond "
                            "Min/Max X:%d/%d, Y: %d/%d .",
                pGilId->RinseStation[0], pGilId->RinseStation[1],
                Xmin, Xmax, Ymin, Ymax);
        gilerrno = 1000;

        Xsetpos = pGilId->RinseStation[0];
        Ysetpos = pGilId->RinseStation[1];

        result = -1;
    }

    return( result );
}

/*****************************************************************************
                         g i l M o v e 2 I n j
    Description:
    Moves to the  Injection Port.
    InjectStation[3];  X/Y/Z coordinates in mm of Injector Station 

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilMove2Inj(GILSONOBJ_ID pGilId)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    /* Be sure the needle is at the top before moving in the x/y direction */
    result = gilMoveZ(pGilId,pGilId->ZTopClamp);
    if ((result < 0) || AbortRobo)
        return result;

    /* Move to Injector X/Y Position */
    delayMsec(200);
    result = gilMoveXY(pGilId, pGilId->InjectStation[0],
                       pGilId->InjectStation[1]);
    if ((result < 0) || AbortRobo)
        return result;

    /* Now, Lower Needle */
    delayMsec(200);
    result = gilMoveZ(pGilId, pGilId->InjectStation[2]);

    return( result );
}

/*****************************************************************************
                           g i l R e l a x X Y
    Description:
    Frees the X/Y Coordinates.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilRelaxXY(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonBCmd(pGilId, pGilId->gilsonID, "E00x", resp);
    return( result );
}

/*****************************************************************************
                        g i l I n i t S y r i n g e
    Description:
    Applicable to the standalone gilson 402 pump.  Used to initialize the
    syringe.  Resets pump.  Selects syringe size and puts valve in the needle
    position.  Sends piston to the upper position.  Liquid may be dispensed
    during this operation so the caller must be sure the needle is in an
    appropriate location (such as the rinse station) before calling.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int gilInitSyringe(GILSONOBJ_ID pGilId, int size)
{
    char resp[16], pumpCmd[16], side='L', stat;
    int  result, i;

    if (pGilId == NULL)
        return(-1);

    /* Set Syringe Size */
    pGilId->pumpVolume = size;

    /* Perform Master Reset on the Pump */
    result = gilsonICmd(pGilId, pGilId->pumpID, '$', resp);
    if ((result < 0) || AbortRobo)
        return result;

    /* Check which side we are on */
    result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if (resp[0] == 'M') side = 'R';
    stat = resp[0+(7*((side == 'R') ? 1 : 0))];
    if (stat != 'I')
        return -1;

    /* Put Valve in the Needle Position */
    sprintf(pumpCmd, "V%cN", side);
    result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0+(7*((side == 'R') ? 1 : 0))]) != 'I') return -1;
    result = gilsonICmd(pGilId, pGilId->pumpID, 'S', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0] != '0') || (resp[1] != '0')) return -1;

    /* Select syringe size */
    sprintf(pumpCmd, "P%c%05d", side, pGilId->pumpVolume);
    result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0+(7*((side == 'R') ? 1 : 0))]) != 'I') return -1;
    result = gilsonICmd(pGilId, pGilId->pumpID, 'S', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0] != '0') || (resp[1] != '0')) return -1;

    /* Now Initialize the Syringe - Liquid May be Dispensed! */
    sprintf(pumpCmd, "O%c", side);
    result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
    if ((result < 0) || AbortRobo)
        return result;
    result = gilsonICmd(pGilId, pGilId->pumpID, 'S', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0] != '0') || (resp[1] != '0')) return -1;

    /* Wait for Status to be Ready */
    for (i=0; i<20; i++) {
        result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
        if ((result < 0) || AbortRobo)
            return result;
        stat = resp[0+(7*((side == 'R') ? 1 : 0))];
        if ((stat != 'R') && (stat != 'I')) break;
    }

    if (stat != 'N') {
        errLogRet(LOGOPT, debugInfo, "Pump Op Failed, stat= %c\r\n", stat);
        return -1;
    }

    return 0;
}

/*****************************************************************************
                           g i l P u m p 2 1 5 S W
    Description:
    Version of the gilPump() routine that works on the 215SW with
    standalone 402B pump (pumpType == 4).

    Direction: valve pos: 'N'=needle position; 'R'=reservoir position
    Volume: negative numbers for dispensing liquid
            positive numbers for aspirating liquid
    flow: flow rate at which to draw-in or expel liquid
    ZSpeed: mm/s to raise/lower needle while performing operation

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilPump215SW(GILSONOBJ_ID pGilId, char Dir, double vol,
                 double flow, int Zspeed, int Zlimit)
{
    char pumpCmd[30], g215Cmd[30];
    char resp[256], side, stat;
    int  result, pumpResult, g215Result=1;
    int  volume, i;

    /* Volume is a whole number of ul, sorry no factional amounts allowed */
    volume = (vol > 0.0) ? ((int) (vol + 0.5)) : ((int) (vol - 0.5));

    /* What side are we on? */
    result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if (resp[0] == 'M') side = 'R'; else side = 'L';
    stat = resp[0+(7*((side == 'R') ? 1 : 0))];

    /* Is pump ready to go? */
    if ((stat == 'O') || (stat == 'H')) {
        result = gilsonICmd(pGilId, pGilId->pumpID, '$', resp);
        if ((result < 0) || AbortRobo)
            return result;
        result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
        if ((result < 0) || AbortRobo)
            return result;
        stat = resp[0+(7*((side == 'R') ? 1 : 0))];
    }
    if (stat == 'I') {
        /* Put Valve in the Needle Position */
        sprintf(pumpCmd, "V%cN", side);
        result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
        if ((result == -1) || AbortRobo)
            return -1;

        /* Select syringe size */
        sprintf(pumpCmd, "P%c%05d", side, pGilId->pumpVolume);
        result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
        if ((result == -1) || AbortRobo)
            return -1;

        /* Now Initialize the Syringe - Liquid May be Dispensed! */
        sprintf(pumpCmd, "O%c", side);
        result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
        if ((result == -1) || AbortRobo)
            return -1;

        /* Wait for Status to be Ready */
        for (i=0; i<20; i++) {
            result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
            if ((result < 0) || AbortRobo)
                return result;
            stat = resp[0+(7*((side == 'R') ? 1 : 0))];
            if ((stat != 'R') && (stat != 'I')) break;
        }
    }
    if (stat != 'N') {
        errLogRet(LOGOPT,debugInfo, "Pump Op Failed, stat= %c\r\n", stat);
        return -1;
    }

    /* Select Correct Valve Position */
    sprintf(pumpCmd, "V%c%c", side, Dir);
    result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0+(7*((side == 'R') ? 1 : 0))]) != 'N') return -1;
    result = gilsonICmd(pGilId, pGilId->pumpID, 'S', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0] != '0') || (resp[1] != '0')) return -1;

    /* Select the flow rate */
    if (flow > 0) {
        sprintf(pumpCmd, "S%c%f", side, (float)flow);
        result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
        if ((result < 0) || AbortRobo)
            return result;
        result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
        if ((result < 0) || AbortRobo)
            return result;
        if ((resp[0+(7*((side == 'R') ? 1 : 0))]) != 'N') return -1;
        result = gilsonICmd(pGilId, pGilId->pumpID, 'S', resp);
        if ((result < 0) || AbortRobo)
            return result;
        if ((resp[0] != '0') || (resp[1] != '0')) return -1;
    }

    /* Select volume to be dispensed/aspirated */
    sprintf(pumpCmd, "%c%c%d",
            ((volume < 0) ? 'D' : 'A'), side, abs(volume));
    result = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
    if ((result < 0) || AbortRobo)
        return result;

    result = gilsonICmd(pGilId, pGilId->pumpID, 'M', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0+(7*((side == 'R') ? 1 : 0))]) != 'H') return -1;
    result = gilsonICmd(pGilId, pGilId->pumpID, 'S', resp);
    if ((result < 0) || AbortRobo)
        return result;
    if ((resp[0] != '0') || (resp[1] != '0')) return -1;

    /* If Z-Axis motion is wanted, prepare the command */
    if (Zspeed > 0) {
        sprintf(g215Cmd, "v%04d,%d", Zlimit, Zspeed);
    }
    else
        g215Cmd[0] = '\0';

    /* Ready, Set... */
    sprintf(pumpCmd, "B%c", side);
    pumpResult = gilsonBCmd(pGilId, pGilId->pumpID, pumpCmd, resp);
    if ((pumpResult != 0) || AbortRobo)
        return -1;

    if (Zspeed > 0) {
        g215Result = gilsonBCmd(pGilId, pGilId->gilsonID, g215Cmd, resp);
        if ((g215Result < 0) || AbortRobo)
            return g215Result;
    }

    /* Wait for the pump to stop */
    delayMsec(500);
    while (!AbortRobo) {
        pumpResult = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
        if ((pumpResult == 1 /* stopped */) || (pumpResult < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    /* Wait for the z-travel to stop */
    if ((Zspeed > 0) && (pumpResult == 1)) {
        while (!AbortRobo) {
            g215Result = gilsonStopped(pGilId,FALSE,FALSE,TRUE,FALSE);
            if ((g215Result == 1) /* stopped */ ||
                (g215Result < 0) /* error */)
                break;
            delayMsec(TimeDelay);
        }
    }

    return( ((pumpResult == 1) && (g215Result == 1)) ? 0 : -1);
}


/*****************************************************************************
                                 g i l P u m p
    Description:
    Speed 1 - 16 for 500 ul.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilPump(GILSONOBJ_ID pGilId, char Dir, double vol,
            double speed, int Zspeed, int Zlimit)
{
    char cmd[30];
    char tmp[30];
    char resp[256];
    int  result;
    int  volume;

    if (pGilId == NULL)
        return(-1);

    if (pGilId->pumpType == 4)
        return(gilPump215SW(pGilId, Dir, vol, speed, Zspeed, Zlimit));

    /* Volume is a whole number of ul, sorry no factional amounts allowed */
    volume = ( vol > 0.0) ? ((int) ( vol + 0.5 )) : ((int) (vol - 0.5));

    if (Dir == 'R')
    {
        /* sprintf(tmp,"DR%0.1lf",vol); */
        sprintf(tmp,"DR%d",volume);
    }

    else /* Dir = 'N'; */
    {
        /* sprintf(tmp,"DN%0.1lf",vol); */
        sprintf(tmp,"DN%d",volume);
    }
    strcpy(cmd,tmp);
  
    if (speed > 0)
    {
        sprintf(tmp,",%0.1lf",speed);
        strcat(cmd,tmp);
    }

    if (Zspeed > 0)
    {
        sprintf(tmp,",Z%d",Zspeed);
        strcat(cmd,tmp);
    }

    /* Be Sure Pump is Ready for Command */
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    if ((result != 1) || AbortRobo)
        return result;

    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    return( ((result != -1) ? 0 : -1) );
}

/*****************************************************************************
                   g i l s o n C u r r e n t V o l u m e
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonCurrentVolume(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int  result,value;
    char *val;

    /* Wait for pump to stop before getting the volume */
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    if ((result != 1) || AbortRobo)
        return result;

    if (pGilId->pumpType == 4) {
        char pumpResp[16];
        int idx = 1;

        result = gilsonICmd(pGilId, pGilId->pumpID, 'M', pumpResp);
        if (result < 0)
            return result;
        if (pumpResp[0] == 'M')  /* 'M' = pump motor not installed */
            idx = 8;             /*   check the other pump         */
        pumpResp[idx+6] = '\0';
        value = atoi(&(pumpResp[idx]));
    }
    else {
        result = gilsonICmd(pGilId, pGilId->gilsonID,'D',resp);
        if (result < 0)
            return result;

        /*
         * Response = vxxx, v= N or R, xxx will be volume in syringe
         * Skip N or R in resp string
         */
        val = strtok(&resp[1]," ");
        value = atoi(val);
    }

    /* fprintf(stderr,"gilsonCurrentVolume: '%s', %d ul\n",resp,value); */
    return(value);
}

/*****************************************************************************
                         g i l s o n A s p i r a t e
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonAspirate(GILSONOBJ_ID pGilId, double volume /*ul */,
                   double Speed, int Zspeed, int Zlimit)
{
    int Zmm,result;

    if (pGilId == NULL)
        return(-1);

/*
 *  gilGetZ(pGilId,&Zmm);
 *  fprintf(stderr,"Z at %d\n",Zmm);
 */

    result = gilPump(pGilId, 'N', volume, Speed, Zspeed, Zlimit);
    if ((result < 0) || AbortRobo)
        return result;

    delayMsec(500);
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    if ((result != 1) || AbortRobo)
        return result;
/*
 *  gilGetZ(pGilId,&Zmm);
 *  fprintf(stderr,"Z at %d after pump\n",Zmm);
 */

    return( ((result < 0) ? -1 : 0));
}


/*****************************************************************************
                    g i l s o n A s p i r a t e A s y n c
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonAspirateAsync(GILSONOBJ_ID pGilId, double volume /*ul */,
                        double Speed, int Zspeed, int Zlimit)
{
    int Zmm,result;

    if (pGilId == NULL)
        return(-1);

    result = gilPump(pGilId, 'N', volume, Speed, Zspeed, Zlimit);
    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                        g i l s o n D i s p e n s e
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonDispense(GILSONOBJ_ID pGilId, double volume /*ul */,
                   double Speed, int Zspeed, int Zlimit)
{
    int Zmm,result;

    if (pGilId == NULL)
        return(-1);

/*
 *  gilGetZ(pGilId,&Zmm);
 *  fprintf(stderr,"Z at %d\n",Zmm);
 */

    result = gilPump(pGilId, 'N', 0-volume, Speed, Zspeed, Zlimit);
    if ((result < 0) || AbortRobo)
        return result;

    delayMsec(500);
    while (!AbortRobo) {
        result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
        if ((result == 1 /* stopped */) || (result < 0) /* error */)
            break;
        delayMsec(TimeDelay);
    }

    if ((result != 1) || AbortRobo)
        return result;
/*
 *  gilGetZ(pGilId,&Zmm);
 *  fprintf(stderr,"Z at %d after pump\n",Zmm);
 */

    return( ((result < 0) ? -1 : 0));
}

/*****************************************************************************
                    g i l s o n D i s p e n s e A s y n c
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97
*****************************************************************************/
int gilsonDispenseAsync(GILSONOBJ_ID pGilId, double volume /*ul */,
                        double Speed, int Zspeed, int Zlimit)
{
    int result;

    if (pGilId == NULL)
        return(-1);

    result = gilPump(pGilId, 'N', 0-volume, Speed, Zspeed, Zlimit);

    return( ((result != -1) ? 0 : -1) );
}

/*****************************************************************************
                          g i l s o n F l u s h
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonFlush(GILSONOBJ_ID pGilId, double volume /*ul */,
                double inSpeed, double outSpeed )
{
    int result = 0;

    if (pGilId == NULL)
        return(-1);

    if (inSpeed > 0.0)
        result = gilPump(pGilId, 'R', volume, inSpeed, 0, 0);

    if ((result >= 0) && (outSpeed > 0.0) && !AbortRobo)
    {
        result = gilPump(pGilId,'N',0-volume,outSpeed, 0, 0);
        if ((result < 0) || AbortRobo)
            return result;

        delayMsec(500);
        while (!AbortRobo) {
            result = gilsonStopped(pGilId,FALSE,FALSE,FALSE,TRUE);
            if ((result == 1 /* stopped */) || (result < 0) /* error */)
               break;
            delayMsec(TimeDelay);
        }

        if ((result != 1) || AbortRobo)
            return result;
    }

    return((result < 0) ? -1 : 0);
}

/*****************************************************************************
                       g i l s o n M a x V o l u m e
    Description:
    Max Volume for syringe in ul.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonMaxVolume(GILSONOBJ_ID pGilId)
{
    if (pGilId == NULL)
        return(-1);

    return(pGilId->pumpVolume);
}

/*****************************************************************************
                     g i l s o n M a x F l o w R a t e
    Description:
    Max Flow rate for syringe in ml/min.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
double gilsonMaxFlowRate(GILSONOBJ_ID pGilId)
{
    double flowrate;
    int pumpvol;

    if (pGilId == NULL)
        return(-1);

    pumpvol = pGilId->pumpVolume;

    /* 0-none,
     * 1-Cavro,
     * 2-gilson 402,
     * 3-gilson Peristaltic,
     * 4-gilson standalone 402 module
     */
    switch(pGilId->pumpType)
    {
        case 0:
            flowrate = 0;
            break;
        case 1:
            if ( (pumpvol >= 0) && (pumpvol <= 350) )
            {
                flowrate = 0.0;
            }
            else if ( (pumpvol >= 351) && (pumpvol <= 750) )
            {
                flowrate = 25.0;
            }
            else if ( (pumpvol >= 751) && (pumpvol <= 2000) )
            {
                flowrate = 50.0;
            }
            else if ( (pumpvol >= 2001) && (pumpvol <= 3500) )
            {
                flowrate = 125.0;
            }
            else if ( (pumpvol >= 3501) && (pumpvol <= 7500) )
            {
                flowrate = 0.0;
            }
            else if ( (pumpvol >= 7501) && (pumpvol <= 15000) )
            {
                flowrate = 500.0;
            }
            else if ( (pumpvol >= 15001) && (pumpvol <= 30000) )
            {
                flowrate = 0.0;
            }
            else
                flowrate = 0.0;
            break;
        case 2:
            if ( (pumpvol >= 0) && (pumpvol <= 75) )
            {
                flowrate = 2.0;
            }
            else if ( (pumpvol >= 76) && (pumpvol <= 200) )
            {
                flowrate = 3.0;
            }
            else if ( (pumpvol >= 201) && (pumpvol <= 350) )
            {
                flowrate = 8.0;
            }
            else if ( (pumpvol >= 351) && (pumpvol <= 750) )
            {
                flowrate = 16.0;
            }
            else if ( (pumpvol >= 751) && (pumpvol <= 2000) )
            {
                flowrate = 33.0;
            }
            else if ( (pumpvol >= 2001) && (pumpvol <= 3500) )
            {
                flowrate = 0.0;
            }
            else if ( (pumpvol >= 3501) && (pumpvol <= 7500) )
            {
                flowrate = 100.0;
            }
            else if ( (pumpvol >= 7501) && (pumpvol <= 15000) )
            {
                flowrate = 100.0;
            }
            else if ( (pumpvol >= 15001) && (pumpvol <= 30000) )
            {
                flowrate = 100.0;
            }
            else
                flowrate = 0.0;
            break;

        case 4:
            {
                switch (pumpvol) {
                case   100:   flowrate = 6;   break;
                case   250:   flowrate = 15;  break;
                case   500:   flowrate = 30;  break;
                case  1000:   flowrate = 60;  break;
                case  5000:   flowrate = 120; break;
                case 10000:   flowrate = 240; break;
                case 25000:   flowrate = 240; break;
                default:      flowrate = 6;   break;
                }
            }
            break;

        default:
            flowrate = 0.0;
            break;
    }

    return(flowrate);
}

/*****************************************************************************
                     g i l s o n M i n F l o w R a t e
    Description:
    Min Flow rate for syringe in ml/min.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
double gilsonMinFlowRate(GILSONOBJ_ID pGilId)
{
    double flowrate;
    int pumpvol;

    if (pGilId == NULL)
        return(-1);

    pumpvol = pGilId->pumpVolume;
    if (pGilId->pumpType == 0)
        return(0.0);

    else if (pGilId->pumpType == 4) {
        switch (pumpvol) {
        case   100:   flowrate = 0.001; break;
        case   250:   flowrate = 0.001; break;
        case   500:   flowrate = 0.001; break;
        case  1000:   flowrate = 0.01;  break;
        case  5000:   flowrate = 0.01;  break;
        case 10000:   flowrate = 0.02;  break;
        case 25000:   flowrate = 0.04;  break;
        default:      flowrate = 0.001; break;
        }
    }
    else {
        switch(pumpvol)
        {
        case    50 :    flowrate = 0.1;  break;
        case   100 :    flowrate = 0.1;  break;
        case   250 :    flowrate = 0.1;  break;
        case   500 :    flowrate = 0.1;  break;
        case  1000 :    flowrate = 0.1;  break;
        case  5000 :    flowrate = 0.1;  break;
        case 10000 :    flowrate = 0.2;  break;
        case 25000 :    flowrate = 0.5;  break;
        default:        flowrate = 0.1;  break;
        }
    }

    return(flowrate);
}

/*****************************************************************************
                       g i l S e t C o n t a c t s
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilSetContacts(GILSONOBJ_ID pGilId, int bits, int on_off)
{
    char cmd[30];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    if ((bits > 0) && (bits < 6))
    {
        strcpy(cmd,"Jxxxxx");
        cmd[bits] = (on_off == 1) ? '1' : '0';
        result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    }
    else
        result = 1;

    return(result);
}

/*****************************************************************************
                         g i l G e t I n p u t s
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilGetInputs(GILSONOBJ_ID pGilId, int bits, int *on_off)
{
    char resp[256];

    *on_off = -1;
    if (pGilId == NULL)
        return(-1);

    if ((bits > 0) && (bits < 6))
    {
        gilsonICmd(pGilId, pGilId->gilsonID,'I',resp);
/*
 *      resp[6] = 0;
 *      fprintf(stderr,"gilGetContacts: resp: '%s'\n",resp);
 */

        *on_off = (resp[bits - 1] == '1') ? 1 : 0;
    }

    return(0);
}

/*****************************************************************************
                        g i l G e t C o n t a c t s
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilGetContacts(GILSONOBJ_ID pGilId, int bits, int *on_off)
{
    char resp[256];

    *on_off = -1;
    if (pGilId == NULL)
        return(-1);

    if ((bits > 0) && (bits < 6))
    {
        gilsonICmd(pGilId, pGilId->gilsonID,'J',resp);
/*
 *      resp[6] = 0;
 *      fprintf(stderr,"gilGetContacts: resp: '%s'\n",resp);
 */

        *on_off = (resp[bits - 1] == '1') ? 1 : 0;
    }

    return(0);
}

/*****************************************************************************
                        i n j e c t S t o p p e d
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int injectStopped(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int  stop, result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonICmd(pGilId, pGilId->injectID,'V',resp);
    if (result < 0)
        return result;

    /*
     * M = Moving,
     * I = Inject Pos,
     * L = Load Pos,
     * E = Error
     */

    stop = (resp[0] != 'M');
    return(stop);
}

/*****************************************************************************
                              g i l I n j e c t
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilInject(GILSONOBJ_ID pGilId)
{
    char  resp[256];
    int result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonBCmd(pGilId, pGilId->injectID, "VI", resp);
    if ((result < 0) || AbortRobo)
        return result;

    while (!injectStopped(pGilId) && !AbortRobo)
        delayMsec(TimeDelay);

    return(0);
}

/*****************************************************************************
                               g i l L o a d
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilLoad(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonBCmd(pGilId, pGilId->injectID, "VL", resp);
    if ((result < 0) || AbortRobo)
        return result;

    while (!injectStopped(pGilId) && !AbortRobo)
        delayMsec(TimeDelay);

    return(0);
}

/*****************************************************************************
                   g i l G e t I n j e c t V a l v e L o c
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilGetInjectValveLoc(GILSONOBJ_ID pGilId,char *loc)
{
    char resp[256];
    int stop, result;

    *loc = 'E';
    if (pGilId == NULL)
        return(-1);

    result = gilsonICmd(pGilId, pGilId->injectID,'V',resp);
    if (result < 0)
        return result;

    /*
     * M = Moving,
     * I = Inject Pos,
     * L = Load Pos,
     * E = Error
     */

    if ((resp[0] != 'I') && (resp[0] != 'L'))
        *loc = 'E';
    else
        *loc = resp[0];

    return(0);
}
    
/*****************************************************************************
                             g i l G e t X Y
    Description:
    Gets the X/Y Coordinates in tenths of mm.
    E.G.   X=1228 Y=38 == X 122.8 mm, Y 3.8mm

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilGetXY(GILSONOBJ_ID pGilId,int *XYmm)
{
    char cmd[30];
    char resp[256];
    char *val;
    int  result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonICmd(pGilId, pGilId->gilsonID,'X',resp);

    if (result == 0)
    {
        val = strtok(resp,"/");
        XYmm[0] = atoi(val);
        val = strtok(NULL," ");
        XYmm[1] = atoi(val);
        result = 0;
    }
    else
    {
        XYmm[0] = 0;
        XYmm[1] = 0;
        result = -1;
    }

    return(result);
}

/*****************************************************************************
                               g i l G e t Z
    Description:
    Gets the Z Coordinates in tenths of mm.
    E.G.   Z=1000 == Z 100.0 mm

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilGetZ(GILSONOBJ_ID pGilId,int *Zmm)
{
    char cmd[30];
    char resp[256];
    char *val;
    int  result;

    if (pGilId == NULL)
        return(-1);

    result = gilsonICmd(pGilId, pGilId->gilsonID,'Z',resp);

    if ( result == 0)
    {
        val = strtok(resp," ");
        *Zmm = atoi(val);
        result = 0;
    }
    else
    {
        *Zmm = 0;
        result = -1;
    }

    return(result);
}

/*****************************************************************************
                      g i l s o n G e t R S t a t i o n
    Description:
    Obtains the X/Y/Z coord of Rinse Station.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int gilsonGetRStation(GILSONOBJ_ID pGilId)
{
    int rX,rY,rZ,stat;

    if (pGilId == NULL)
        return(-1);

    stat = setNVaddr(pGilId,pGilId->gilsonID,NV_RINSEX);

    if (stat == 0)
    {
        rX = getNVint(pGilId, pGilId->gilsonID);
        if ((rX == -1) || AbortRobo)
            return -1;

        rY = getNVint(pGilId, pGilId->gilsonID);
        if ((rY == -1) || AbortRobo)
            return -1;

        rZ = getNVint(pGilId, pGilId->gilsonID);
        if ((rZ == -1) || AbortRobo)
            return -1;

        pGilId->RinseStation[0] = rX;
        pGilId->RinseStation[1] = rY;
        pGilId->RinseStation[2] = rZ;
        stat = 0;
        DPRINT3(1,"gilsonGetRStation: X/Y/Z: %d/%d/%d\n",
                pGilId->RinseStation[0], pGilId->RinseStation[1],
                pGilId->RinseStation[2]);
    }

    return(stat);
}

/*****************************************************************************
                     g i l s o n S e t R S t a t i o n
    Description:
    Sets the X/Y/Z coord of Rinse Station.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonSetRStation(GILSONOBJ_ID pGilId,int X, int Y, int Z)
{
    int rX,rY,rZ,stat;

    if (pGilId == NULL)
        return(-1);

    stat =  setNVvalue(pGilId,pGilId->gilsonID,NV_RINSEX,X);
    if ((stat < 0) || AbortRobo)
        return stat;
    stat =  setNVvalue(pGilId,pGilId->gilsonID,NV_RINSEY,Y);
    if ((stat < 0) || AbortRobo)
        return stat;
    stat =  setNVvalue(pGilId,pGilId->gilsonID,NV_RINSEZ,Z);
    if ((stat < 0) || AbortRobo)
        return stat;
    stat = gilsonGetRStation(pGilId);
    return(stat);
}

/*****************************************************************************
                        g i l s o n S e t Z T o p
    Description:
    Set the ZTop value into Gilson EEprom.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonSetZTop(GILSONOBJ_ID pGilId,int ZTop)
{
    int stat;

    if (pGilId == NULL)
        return(-1);

    stat =  setNVvalue(pGilId, pGilId->gilsonID, NV_ZTOPCLAMP, ZTop);
    if ((stat < 0) || AbortRobo)
        return stat;

    stat = setNVaddr(pGilId, pGilId->gilsonID, NV_ZTOPCLAMP);
    if ((stat < 0) || AbortRobo)
        return stat;

    stat = getNVint(pGilId, pGilId->gilsonID);
    if ((stat < 0) || AbortRobo)
        return stat;

    pGilId->ZTopClamp = stat;
    return (0);
}

/*****************************************************************************
                        g i l s o n G e t Z T o p
    Description:
    Get the ZTop value from Gilson EEprom.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonGetZTop(GILSONOBJ_ID pGilId)
{
    int ztop, result;

    if (pGilId == NULL)
        return(-1);

    result = setNVaddr(pGilId, pGilId->gilsonID,NV_ZTOPCLAMP);
    if (result < 0)
        return result;
    ztop = getNVint(pGilId, pGilId->gilsonID);
    return(ztop);
}


/*****************************************************************************
                        g i l G e t Z T o p
    Description:
    Return the ZTop value.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilGetZTop(GILSONOBJ_ID pGilId)
{
    int ztop;

    if (pGilId == NULL)
        return(-1);

    return(pGilId->ZTopClamp);
}


/*****************************************************************************
                       g i l s o n T e s t M o d e
    Description:
    Set the Gilson TestMode.
    1- XYZ test
    2- Dilutor test
    3- Display liquid level sensor Frequency
    8- Disable XY phase checking
    9- Reset NV-RAM and initialize to defaults

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonTestMode(GILSONOBJ_ID pGilId,int mode)
{
    char cmd[10];
    char resp[256];
    int  result;

    if (pGilId == NULL)
        return(-1);

    sprintf(cmd,"~%d",mode);
    result = gilsonBCmd(pGilId, pGilId->gilsonID, cmd, resp);
    if (result < 0)
        return(-1);
    else
        return(0);
}

/*****************************************************************************
                        g i l s o n S e t P u m p
    Description:
    Set the Pump Type & Volume into EEprom.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilsonSetPump(GILSONOBJ_ID pGilId, int PumpType, int PumpVolume)
{
    int stat;

    if (pGilId == NULL)
        return(-1);

    /*
     * Hermes has a standalone pump, - only storing the
     *  syringe size in the NVRAM of the liquid handler.
     */
    if (!strncmp(pGilId->liqDevEntry->devName, "HRM_GILSON", 10)) {
        pGilId->pumpType = PumpType;
        pGilId->pumpVolume = PumpVolume;
        stat =  setNVvalue(pGilId, pGilId->gilsonID, 10, PumpVolume);
        return stat;
    }

    stat =  setNVvalue(pGilId, pGilId->gilsonID, NV_PUMP, PumpType);
    if (stat < 0)
        return stat;
    stat =  setNVvalue(pGilId, pGilId->gilsonID, NV_SYRINGE, PumpVolume);
    if (stat < 0)
        return stat;

    /* get pump type and volume from NV RAM */
    stat = setNVaddr(pGilId, pGilId->gilsonID, NV_PUMP);
    if (stat < 0)
        return stat;
    stat =  getNVint(pGilId, pGilId->gilsonID);
    if (stat < 0)
        return stat;
    else
        pGilId->pumpType = stat;

    stat = getNVint(pGilId, pGilId->gilsonID);
    if (stat < 0)
        return stat;
    else
        pGilId->pumpVolume = stat;

    return(stat);
}
/*****************************************************************************
                            s e t N V a d d r
    Description:
    Set Non-volatile RAM Address.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int setNVaddr(GILSONOBJ_ID pGilId, short unitId,int address)
{
    char cmd[256];
    char resp[256];
    int  result;

    sprintf(cmd,"@%d",address);
    result = gilsonBCmd(pGilId, unitId, cmd, resp);
    return(result);
}

/*****************************************************************************
                         s e t N V v a l u e
    Description:
    Set Non-volatile Value at RAM Address.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int setNVvalue(GILSONOBJ_ID pGilId, short unitId,int address,int value)
{
    char cmd[256];
    char resp[256];
    int  result;

    sprintf(cmd,"@%d=%d",address,value);
    result = gilsonBCmd(pGilId, unitId, cmd, resp);
    return(result);
}


/*****************************************************************************
                           g e t N V i n t
    Description:
    Gets Assumed Int value from Non-volatile RAM Address.
    Obtain the integer value from the previously set NV Address.

    Returns:
    int value

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int getNVint(GILSONOBJ_ID pGilId, short unitId)
{
    char resp[32];
    int  data, stat, i;

    stat = gilsonICmd(pGilId, unitId,'@',resp);
    if (stat < 0)
        return(-1);    /* TIMEOUT */

    /* fprintf(stderr,"'%s'\n",resp); */

    for (i=0; (resp[i] != '\0') && (resp[i] != '='); i++) { ; }
    if (resp[i] == '=')
        data = atoi(&resp[i+1]);
    else {
        data = 0;
        DPRINT1(0, "Unable to parse gilson response to \'@\' command (%s)\n",
            resp);
    }

    /* fprintf(stderr,"data; '%s', valus: %d\n",&resp[3],data); */

    return(data);
}


#ifdef XXX
/*****************************************************************************
                       g i l s o n X Y Z P o w e r e d
    Description:
    Returns the  status of power to the X, Y and Z axis.

    Returns:
     1         - Powered
     0         - if any axis or is unpowered or in error state
    -1         - Error

    Author:    Greg Brissey 5/8/97

*****************************************************************************/
int gilsonXYZPowered(GILSONOBJ_ID pGilId)
{
    int stat;

    if (pGilId == NULL)
        return(-1);
    stat = gilsonStopped(pGilId, 1, 1, 1, 0);
    return( ((stat < 0) ? 0 : 1 ));
}

/*****************************************************************************
                     g i l s o n P u m p P o w e r e d
    Description:
    Returns the  status of power to the pump.

    Returns:
     1         - Powered
     0         - if pump is unpowered or in error state
    -1         - Error

    Author:    Greg Brissey 5/8/97

*****************************************************************************/
int gilsonPumpPowered(GILSONOBJ_ID pGilId)
{
    int stat;

    if (pGilId == NULL)
        return(-1);
    stat = gilsonStopped(pGilId, 0, 0, 0, 1);
    return( ((stat < 0) ? 0 : 1 ));
}
#endif

/*****************************************************************************
                       g i l W r i t e D i s p l a y
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilWriteDisplay(GILSONOBJ_ID pGilId,char *msge)
{
    int  result,len;
    char msg[24];
    char resp[256];

    if (pGilId == NULL)
        return(-1);

    if (!strncmp(pGilId->liqDevEntry->devName, "HRM_GILSON", 10)) {
        strcpy(msg, "W1=");
        strncat(msg, msge, 20);
        msg[23] = '\000';
    }
    else {
        strcpy(msg,"W");
        strncat(msg,msge,9);
        msg[9] = '\000';
    }
    result = gilsonBCmd(pGilId, pGilId->gilsonID, msg, resp);
    return(result);
}

/*****************************************************************************
                        g i l C l e a r E r r o r
    Description:

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
int gilClearError(GILSONOBJ_ID pGilId)
{
    char resp[256];
    int  result;
    result = gilsonBCmd(pGilId, pGilId->gilsonID, "e", resp);
    return result;
}

/*****************************************************************************
                        g i l E r r o r M s g e
    Description:

*****************************************************************************/
char *gilErrorMsge(GILSONOBJ_ID pGilId)
{
    char cmd[30];
    char resp[256];
    char *tmp;
    int  result,errorcode;

    if (gilerrno == -1)
    {
        /* xXXxx/xxxx or xxXX */
        result = gilsonICmd(pGilId, pGilId->gilsonID,'e',resp);
        if (result == 0)
        {
            /* fprintf(stderr,"error read: '%s'\n",resp); */

            tmp = strtok(resp,"/ ");

            /* fprintf(stderr,"token: '%s'\n",tmp); */

            errorcode = atoi(tmp);
            switch(errorcode)
            {
            case 0:    
                ErrorMessage = "None";
                break;
            case 10:   
                if (pGilId->pumpType == 4)
                    ErrorMessage = "Slave absent.";
                else
                    ErrorMessage = "Invalid Pump Type.";
                break;
            case 11:   
                ErrorMessage = "Undefined syringe size.";
                break;
            case 12:   
                ErrorMessage = "Pump not found.";
                break;
            case 13:   
                ErrorMessage = "Syringe speed out of range.";
                break;
            case 14:   
                ErrorMessage = "Invalid syringe volume.";
                break;
            case 15:   
                ErrorMessage = "NV-RAM checksum is invalid.";
                break;
            case 16:   
                ErrorMessage = "X scale factor is invalid.";
                break;
            case 17:   
                ErrorMessage = "Y scale factor is invalid.";
                break;
            case 18:   
                ErrorMessage = "Z scale factor is invalid.";
                break;
            case 20:   
                ErrorMessage = "X motor position error.";
                break;
            case 21:   
                ErrorMessage = "Y motor position error.";
                break;
            case 22:   
                ErrorMessage = "Z motor position error.";
                break;
            case 23:
                ErrorMessage = "Homing error.";
                break;
            case 24:
                /* ErrorMessage = "X target less than minimum X."; break; */
                sprintf(gErrorMsge,"X target %d less than minimum X %d\n",
                Xsetpos,pGilId->X_MinMax[0]);
                ErrorMessage = gErrorMsge;
                break;
            case 25:
                /* ErrorMessage = "X target more than maximum X."; break; */
                sprintf(gErrorMsge,"X target %d more than maximum X %d\n",
                Xsetpos,pGilId->X_MinMax[1]);
                ErrorMessage = gErrorMsge;
                break;
            case 26:
                /* ErrorMessage = "Y target less than minimum Y."; break; */
                sprintf(gErrorMsge,"Y target %d less than minimum Y %d\n",
                Ysetpos,pGilId->Y_MinMax[0]);
                ErrorMessage = gErrorMsge;
                break;
            case 27:
                /* ErrorMessage = "Y target more than maximum Y."; break; */
                sprintf(gErrorMsge,"Y target %d more than maximum Y %d\n",
                Ysetpos,pGilId->Y_MinMax[1]);
                ErrorMessage = gErrorMsge;
                break;
            case 28:
                /* ErrorMessage = "Z target less than minimum Z."; break; */
                sprintf(gErrorMsge,"Z target %d less than minimum Z %d\n",
                Zsetpos,pGilId->Z_MinMax[0]);
                ErrorMessage = gErrorMsge;
                break;
            case 29:
                /* ErrorMessage = "Z target more than maximum Z."; break; */
                sprintf(gErrorMsge,"Z target %d more than maximum Z %d\n",
                Zsetpos,pGilId->Z_MinMax[1]);
                ErrorMessage = gErrorMsge;
                break;
            case 30:   
                ErrorMessage = "X encoder inactive.";
                break;
            case 31:   
                ErrorMessage = "Y encoder inactive.";
                break;
            case 32:   
                ErrorMessage = "Z position sensor inactive.";
                break;
            case 33:   
                ErrorMessage = "Safety contact activated.";
                break;
            case 34:   
                ErrorMessage = "X home phase is invalid.";
                break;
            case 35:   
                ErrorMessage = "Y home phase is invalid.";
                break;
            case 36:
                ErrorMessage = "X and Y home phases are invalid.";
                break;
            case 39:   
                if (pGilId->pumpType == 4)
                    ErrorMessage = "Stop button has been pressed.";
                else
                    ErrorMessage = "Dilutor not initialized.";
                break;
            case 40:
                ErrorMessage = "Gilson m402 invalid valve position.";
                break;
            case 41:   
                ErrorMessage = "Gilson m402 valve missing.";
                break;
            case 42:   
                ErrorMessage = "Gilson m402 undefined valve command.";
                break;
            case 43:   
                ErrorMessage = "Gilson m402 valve communication error.";
                break;
            case 44:   
                ErrorMessage = "Gilson m402 valve unit busy.";
                break;
            case 45:  
                ErrorMessage = "Gilson m402 syringe overload.";
                break;
            case 46:
                ErrorMessage = "Gilson m402 syringe missing.";
                break;
            case 47:
                ErrorMessage = "Gilson m402 undefined syringe command.";
                break;
            case 48:   
                ErrorMessage = "Gilson m402 syringe communication error.";
                break;
            case 49:   
                ErrorMessage = "Gilson m402 valve unit busy.";
                break;
            case 50:   
                ErrorMessage = "Cavro XL-3000 initialization error.";
                break;
            case 51:   
                ErrorMessage = "Cavro XL-3000 invalid command.";
                break;
            case 52:   
                ErrorMessage = "Cavro XL-3000 invalid operand.";
                break;
            case 53:   
                ErrorMessage = "Cavro XL-3000 communication error.";
                break;
            case 54:   
                ErrorMessage = "Cavro XL-3000 not initialized.";
                break;
            case 55:   
                ErrorMessage = "Cavro XL-3000 plunger overload.";
                break;
            case 56:   
                ErrorMessage = "Cavro XL-3000 valve overload.";
                break;
            case 57:   
                ErrorMessage = "Cavro XL-3000 plunger move not allowed.";
                break;
            case 58:   
                ErrorMessage = "Cavro XL-3000 command overflow.";
                break;
            case 59:   
                ErrorMessage = "Cavro XL-3000 invalid echo.";
                break;
            case 60:   
                ErrorMessage = "Cavro XL-3000 unit busy.";
                break;
            default:
                sprintf(gErrorMsge,"Undescribed error number '%d'.",
                        errorcode);
                ErrorMessage = gErrorMsge;
                break;
            } /* end switch */
        } /* if result == 0 */

        else
        {
            ErrorMessage = "Error reading m215 error.";
        }

    } /* if gilerrno == -1 */
    else
        ErrorMessage = gErrorMsge;

    gilerrno = -1;
    return(ErrorMessage);
}

/*****************************************************************************
                            C h a r _ C m d
    Description:
    Sends & Receives Characters from the Gilson 215.

    Returns:
    int           - # of char(s) received
    -1            - Error (timeout)

    Author:       Greg Brissey 3/5/97

*****************************************************************************/
static int Char_Cmd(GILSONOBJ_ID pGilId, char *cmd, char *resp)
{
    char recvbuf[20];
    char *sptr,*rptr;
    int wbyte,rbyte;
    int cnt, max, tmout;

    /*  if (*(cmd+2) == '\n')
     *      errLogRet(LOGOPT,debugInfo, ">> %s\n", cmd+2);
     *  else
     *      errLogRet(LOGOPT,debugInfo, ">> %s\n", cmd+1);
     */

    /* Clear i/o channel */
    pGilId->liqDevEntry->ioctl(pGilId->liqDev, IO_FLUSH);    

    tmout = 10;
    max = 128;
    sptr = cmd;
    rptr = resp;
    *rptr = '\000';
    cnt=0;
    rbyte=0;

    /* Write out the command to Gilson, getting the echoed characters */
    while ((*sptr != '\000') && (tmout > 0) && !AbortRobo)
    {
        wbyte = pGilId->liqDevEntry->write(pGilId->liqDev, sptr, 1);
        if (AbortRobo) break;

        /* fprintf(stderr,"sent: 0x%2x, '%c' %d byte\n",
         *       (*sptr) & 0xff, *sptr, wbyte);
         */
        sptr++;

        /* let transmission complete */
        pGilId->liqDevEntry->ioctl(pGilId->liqDev, IO_DRAIN);
        if (AbortRobo) break;

        /* This is a BLOCKING read  */
        rbyte = pGilId->liqDevEntry->read(pGilId->liqDev, recvbuf, 1);
        if (AbortRobo) break;

        if (rbyte > 0)
        {
            /* fprintf(stderr,"   read back: 0x%2x, '%c'\n",
             *           (recvbuf[0]& 0x7F),(recvbuf[0]& 0x7F));
             */
            if ((recvbuf[0]& 0x7F) >= 0x20) /* strip non-printable char */
            {
                cnt++;
                *rptr = recvbuf[0] & 0x7F;

                /* fprintf(stderr,"   saved char: 0x%2x, '%c'\n",
                 *          recvbuf[0],recvbuf[0]);
                 */
                rptr++;
            }
            else
                tmout--;
        }
        else
        {
            cnt = -1;
            *rptr = '\000';    /* null terminate string */

            /* fprintf(stderr,"   nothing read back, timeout.\n"); */

            return(cnt);
        }

    } /* end while */

    /*
     * Now write acks until the '#' character is returned,
     * reading the returned characters
     */
    tmout = 10;
    while( (tmout > 0) && !AbortRobo )
    {
        wbyte = pGilId->liqDevEntry->write(pGilId->liqDev, &ack, 1);
        if (AbortRobo) break;

        /* fprintf(stderr,"sent: 0x%2x, '%c' %d byte\n",ack,ack,wbyte); */

        /* let transmission complete */
        pGilId->liqDevEntry->ioctl(pGilId->liqDev, IO_DRAIN);
        if (AbortRobo) break;

        /* This is a BLOCKING read  */
        rbyte = pGilId->liqDevEntry->read(pGilId->liqDev, recvbuf, 1);
        if (AbortRobo) break;

        if (rbyte > 0)
        {

            /* fprintf(stderr,"   read back: 0x%2x, '%c'\n",
             *          (recvbuf[0]& 0x7F),(recvbuf[0]& 0x7F));
             */
            /*
             * Break out when find # character, or
             *   read more than 256 character,
             * Avoid endless loop
             */

            if (( (recvbuf[0]& 0x7F) == '#') || (cnt > max) || (tmout <= 0))
            {
                /* fprintf(stderr,"   found #, breakout of loop\n"); */

                break;
            }

            if ((recvbuf[0]& 0x7F) >= 0x20) /* strip non-printable char */
            {
                cnt++;
                *rptr = recvbuf[0] & 0x7F;

                /* fprintf(stderr,"   saved char: 0x%2x, '%c'\n",
                 *          recvbuf[0],recvbuf[0]);
                 */
                rptr++;
            }
            else
                tmout--;
        }
        else
        {
            *rptr = '\000';    /* null terminate string */

            /* fprintf(stderr,"   nothing read back, timeout.\n"); */

            return(cnt);
        }

    } /* end while */

    *rptr = '\000';   /* null terminate string */

    /* fprintf(stderr,"returning: '%s'\n",resp);
     * fprintf(stderr,"\n");
     */


    /* errLogRet(LOGOPT,debugInfo, "<< %s\n", resp); */


    return(cnt);  /* number of chars in response */
}

/*****************************************************************************
                          g i l s o n I C m d
    Description:
    Send an Immediate Command to the Gilson.
    Immediate Commands Mode, form: 'unit id, char cmd, ack(s)'

    Returns:
    0           - Success
    -1          - Error (timeout)

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int gilsonICmd(GILSONOBJ_ID pGilId, short unitId,
                      char cmd, char *resp)
{
    char msg[255];
    char *mptr;
    int i,nAcks,stat;;

    mptr = msg;

    /* fprintf(stderr,"CMD: '%c'\n",cmd); */

    *mptr++ = unitId;
    *mptr++ = cmd;
    *mptr = '\000';

    stat = Char_Cmd(pGilId, msg, resp);

    return( ((stat > 0) ? 0 : -1) );
}

/*****************************************************************************
                           g i l s o n B C m d
    Description:
    Send a Buffered Command to the Gilson.
    Buffer Commands Mode, form: 'unit id, lf, char cmd, cr'

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 3/5/97

*****************************************************************************/
static int gilsonBCmd(GILSONOBJ_ID pGilId, short unitId,
                      char *cmd, char *resp)
{
    char msg[255];
    int stat;

    /* fprintf(stderr,"CMD: '%s'\n",cmd); */

    sprintf(msg,"%c%c%s%c",unitId,lf,cmd,cr);

    /* fprintf(stderr,"Response Addr: 0x%lx\n",resp); */

    stat = Char_Cmd(pGilId, msg, resp);

    return( ((stat > 0) ? 0 : -1));
}

/*****************************************************************************
                             g i l C o m m a n d
    Description:
    Send an Immediate or Buffered Command to the Gilson Device.
    Since we always can't foresee what devices a customer might put on the
    Gilson-215, this command allows direct access to any device until we
    add a more appropriate command for the user.

    E.G. TCL scripts:

        gCommand 22 "X0150/0150" "B"
        (buffered command to send gilson to set X=150,Y=150)

        set loc [gCommand 22 "X" "I"]
        (loc = X & Y locations 0150/0150 )

    Input Parameters:
      Unit ID = ID of gilson device, 22 Gilson-215, 29 for Value
      Cmd     - a single character for an Immediated command
                a string for a buffered command
      CmdType - Command type:  Immediate = 'I', Buffered = 'B'
      resp    - updated with the string returned as a result of the command

    Returns:
    0           - Success
    -1          - Error (timeout)

    Author:     Greg Brissey 9/28/2001

*****************************************************************************/
int gilCommand(GILSONOBJ_ID pGilId, int UnitId, char *Cmd,
               char *CmdType, char *resp)
{
    char  cmd[30];
    int result;
    short unit;

    /* Null terminate that string */
    resp[0] = '\0';

    if (pGilId == NULL)
        return(-1);

    /* Add the magic 128 to the unit id */
    unit = UnitId + 128;

    if ((CmdType[0] == 'I') || (CmdType[0] == 'i'))
    {
        result = gilsonICmd(pGilId, unit, Cmd[0], resp);
    }

    else if ((CmdType[0] == 'B') || (CmdType[0] == 'b'))
    {
        result = gilsonBCmd(pGilId, unit, Cmd, resp);
    }

    else
    {
        result = -1;
    }

    return( ((result != -1) ? 0 : -1) );
}


#ifdef XXX
/*****************************************************************************
                                  m a i n
    Description:

*****************************************************************************/
main (int argc, char *argv[])
{
  GILSONOBJ_ID pGilObjId;

  pGilObjId = gilsonCreate("/dev/term/b",22,29);
    
}
#endif
#ifdef XXXX
#define SOLARIS
main (int argc, char *argv[])
{
  char buffer[256];
  char *bptr;
  char  cmd;
  int   smpnum;
  int   immediate;
  int done;
  extern int initPort(char *);

  if (argc != 2)
  {
    fprintf(stderr,"usage:  %s <devicename>\n", argv[0]);
    exit(1);
  }

  fprintf(stderr,"Init Device: %s\n",argv[1]);
  Portfd = initPort(argv[1]);
  /* Portfd = portFdGet(); */
  /* printf("Portfd: %d, Strm: 0x%lx\n",Portfd,sPort); */
  fprintf(stderr,"Portfd: %d\n",Portfd);

  done = 1;
  while (done)
  {
    fprintf(stderr,"\nMesg Addr: 0x%lx, Respon Addr: 0x%lx\n",Mesg,Respon);
    fprintf(stderr,"Immediate Cmd or Buffered Cmd (i or b): \n");
    bptr = gets(buffer);
    if (bptr == NULL)
      break;
    fprintf(stderr,"Type - '%s'\n", buffer);
    switch( toupper(buffer[0]) )
    {
        case 'I':
        immediate = TRUE;
        break;
        case 'B':
        immediate = FALSE;
        break;
        case 'Q':
        done = FALSE;
        restorePort(Portfd);
        close(Portfd);
        return;
        break;
    }
 
    memset(Respon,0,256);
    fprintf(stderr,"Cmd: ");
    gets(buffer);
    fprintf(stderr,"\n");
    fprintf(stderr,"Cmd: '%s' \n",buffer);
    if (immediate)
    gilsonICmd(buffer[0],&Respon[0]);
     else
    gilsonBCmd(buffer,&Respon[0]);
    fprintf(stderr,"\nMain Respon: '%s'\n",Respon);
  }
 
  restorePort(Portfd);
 
}
#endif
