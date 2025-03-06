/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <stdio.h>
#include <stdlib.h>
#include  <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "gilsonObj.h"
#include "rackObj.h"
#include "termhandler.h"
#include "iofuncs.h"

extern int DebugLevel;

/******************* TYPE DEFINITIONS AND CONSTANTS *************************/
#define ZONE1 1
#define ZONE2 2
#define ZONE3 3
#define MAXPATHL 256

/**************************** GLOBAL VARIABLES ******************************/
extern int   AbortRobo;
char  systemdir[MAXPATHL];       /* vnmr system directory */
ioDev *smsDevEntry = NULL;
int   smsDev = -1;

/****************************** STATIC GLOBALS ******************************/
static  GILSONOBJ_ID pGilObjId;
static  RACKOBJ_ID inject;
static  RACKOBJ_ID rack = NULL;
static  RACKOBJ_ID rack2,rack3;

static int RackCenter[2] = { 546, 1872 };
static int SampleTop = 1013;
static int InjectorCenter[2] = { 5582, 38 };
static int InjectorBot = 840;

static int RackLoc1Delta[2] = { 0, 0 };
static int RackLoc1Top = 1210;
static int InjectorDelta[2] = { 0, 0 };
static int InjectorPort1Bot = 840;
static int RinseStation[2] = { 0, 0 };
static int RinseStationZ = 1210;

static double MaxFlow = 0.0;
static int MaxVolume = 0;

static int Xincr,Yincr,Zincr;
static int XYLoc[2],ZLoc;

int verbose = 1;

static int xyzMinMax[6];

static char path[MAXPATHL];

main (int argc, char *argv[])
{
    int done;
    char buffer[256];
    char msge[10];
    char *bptr, *tmpptr;
    char Current_InjectValve_Loc;
    int  CurrentXYmm[2],CurrentZmm,Current_AirValve_Loc,CurrentVol;
    int xAxis,yAxis;
    int i,maxflow;
    int sampZtop,sampZbottom;
    /* int xyzMinMax[6]; */
    double flowrate, zspeed,ztravel;
    int buflen = 1;

    if (argc < 2)
    {
        fprintf(stdout, "usage:  %s <devicename> (i.e. /dev/term/b)\n",
                argv[0]);
        exit(1);
    }

    if (argc > 2)
    {
        verbose = 0;
    }

    /* initialize environment parameter vnmrsystem value */
    tmpptr = getenv("vnmrsystem");      /* vnmrsystem */
    if (tmpptr != (char *) 0) {
        strcpy(systemdir,tmpptr);       /* copy value into global */
    }
    else {   
        strcpy(systemdir,"/vnmr");      /* use /vnmr as default value */
    }

    if (verbose)
        fprintf(stdout,
                "Init Default Injector 'm215_inj.grk' at 558.2, 3.8\n");

    strcpy(path,systemdir);
    strcat(path,"/asm/racks/m215_inj.grk");
    inject = rackCreate(path);
    if (inject == NULL)
    {
        fprintf(stderr,
                "\nSystem failed to init '%s'\n", path);
        fprintf(stderr,"\ngilalign Aborted.\n");
        exit(1);
    }

    rackCenter(inject, 5582 ,38);  /* 558.2, 3.8,  84.2 Gilson position */
    if (verbose)
        rackShow(inject, 1);

    if (verbose)
        fprintf(stdout,"Init Default Rack 'code_205.grk' at 54.6, 187.2\n");

    strcpy(path,systemdir);
    strcat(path,"/asm/racks/code_205.grk");
    rack = rackCreate(path);
    if (rack == NULL)
    {
        fprintf(stderr,"\nSystem failed to init '%s'\n", path);
        fprintf(stderr,"\ngilalign Aborted.\n");
        exit(1);
    }
    rackCenter(rack, 546,1872);
    if (verbose)
        rackShow(rack, 1);

    if (verbose)
        fprintf(stdout,"Init Device: %s\n",argv[1]);

    /*
     * Convert old style comm description to new style:
     *   Old Style: /dev/term/a
     *   New Style: GIL_TTYA
     */
    if ( (!strcmp(argv[1], "/dev/term/a")) ||
         (!strcmp(argv[1], "/dev/ttya")) )
        pGilObjId = gilsonCreate("GIL_TTYA", 22, 29, 3);

    else if ( (!strcmp(argv[1], "/dev/term/b")) ||
              (!strcmp(argv[1], "/dev/ttyb")) )
        pGilObjId = gilsonCreate("GIL_TTYB", 22, 29, 3);

    else if (!strcmp(argv[1], "/dev/ttyS0"))
        pGilObjId = gilsonCreate("GIL_COM1", 22, 29, 3);

    else
        pGilObjId = gilsonCreate(argv[1], 22, 29, 3);

    if (pGilObjId == NULL)
    {
        fprintf(stderr,"\nFailure to initialize Gilson '%s'\n", argv[1]);
        fprintf(stderr,"\nCheck that the i/o cable is properly attached.\n");
        fprintf(stderr,"\ngilalign Aborted.\n");
        exit(1);
    }


    /* Initialize Rinse Station Location */
    RinseStation[0] = pGilObjId->RinseStation[0];
    RinseStation[1] = pGilObjId->RinseStation[1];
    RinseStationZ = pGilObjId->RinseStation[2];

    xAxis = rackGetX(inject, ZONE1,1);
    yAxis = rackGetY(inject, ZONE1,1);
    sampZtop = rackSampTop(inject,ZONE1,1);
    sampZbottom = rackSampBottom(inject,ZONE1,1);
    /* gilInitInjLoc(pGilObjId,xAxis, yAxis, bottom); */
    gilInitInjLoc(pGilObjId,xAxis, yAxis, InjectorBot);

    MaxFlow = gilsonMaxFlowRate(pGilObjId);  /* ml/min */
    MaxVolume = gilsonMaxVolume(pGilObjId); /* ul */
    gilGetContacts(pGilObjId, 2, &Current_AirValve_Loc);
    gilGetInjectValveLoc(pGilObjId,&Current_InjectValve_Loc);
    gilGetXY(pGilObjId,CurrentXYmm);
    gilGetZ(pGilObjId,&CurrentZmm);
    CurrentVol = gilsonCurrentVolume(pGilObjId);

    if (verbose)
        PrintSet();

    done = 1;
    fprintf(stdout,"\n\n");

    gilXYZMinMax(pGilObjId,xyzMinMax);

    /**********/
    fprintf(stdout,"%d %d %d %d %d %d %d %lf %d %c %d %d %d %d\n",
            xyzMinMax[0],xyzMinMax[1],
            xyzMinMax[2],xyzMinMax[3],
            xyzMinMax[4],xyzMinMax[5],
            MaxVolume,MaxFlow,
            Current_AirValve_Loc,Current_InjectValve_Loc,
            CurrentXYmm[0],CurrentXYmm[1],
            CurrentZmm,CurrentVol);
    fflush(stdout);

    while (done)
    {
        if (verbose)
        {
            fprintf(stdout,"T)ray Alignment, R)inse Station Alignment, "
                           "I)njector Alignment,  \n");
            fprintf(stdout,"N)ew Syringe, C) Set Injector/Rack Centers, "
                           "L)ist Present Settings \n");
            fprintf(stdout,"K - Reset&Home Gilson, V)alve, "
                           "inJ)ector Valve,  \n");
            fprintf(stdout,"D)efine Rack, A)rm Hight, P)rime pump, "
                           "S)ave Parameters, Q)uit,  \n");
            fprintf(stdout,"F)req Liq Detect X)GoToHome "
                           "Z)Test Rack Definition  \n");
            fprintf(stdout,"O)utputs - sets Gilson contact states, "
                           "E)xternal Inputs, Y) Transparent Mode \n");
            fprintf(stdout,"\nCmds:  ");
        }
        else if (buflen)
        {
            fprintf(stdout,"Cmds:\n");
        }

        /**********/

        fflush(stdout);
        bptr = fgets(buffer, sizeof(buffer), stdin);
        buflen = strlen(buffer);
        if (bptr == NULL)
            break;
        switch( toupper(buffer[0]) )
        {
            case 'D':
                DefineRack();
                break;

            case 'R':
                RinseAlignment();
                break;

            case 'I':
                InjectAlignment();
                break;

            case 'T':
                TrayAlignment();
                break;

            case 'N':
                PumpReplace();
                break;

            case 'C':                /* set rack centers */
                setCenters();
                break;

            case 'L':
                PrintSet();
                break;

            case 'O':
                setOutputs();
                break;

            case 'E':
                getInputs();
                break;

            case 'P':
                PrimePump();
                break;

            case 'V':
                setValve();
                break;

            case 'J':
                setInjector();
                break;

            case 'K':
                gilsonReset(pGilObjId);
                gilsonHome(pGilObjId);
                break;

/*
 *          case 'B':
 *              saveAlignments();
 *              break;
 */

            case 'A':
                setArmHeight();
                break;

            case 'Q':
                done = 0;
                gilsonDelete(pGilObjId);
                return;
                break;

            case 'S':       /* Store alignment parameters */
                SaveParameter();
                break;

            case 'X':       /* Move to Home Position */
                gotoHome();
                break;

            case 'F':
                chkLQZ();
                break;

            case 'Y':
                GilsonTransparentMode();
                break;

            case 'Z':
                chkRack();
                break;

        }
    }
}

/*--------------------------------------------------------
  Set the Arm Height as read by the scale into the Non-Volatile Memory
  in the Gilson 215 Liquid Handler
*/
setArmHeight()
{
    char buffer[256];
    char *bptr;
    int height,storedheight;

    if (verbose)
    {
        fprintf(stdout,"Arm Height (mm): ");
        fflush(stdout);
    }

    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    height = atoi(buffer);
    if (verbose)
    {
        fprintf(stdout,"Set Arm Height to (mm): %d\n",height);
        fflush(stdout);
    }
    if ((height >= 50) && (height <= 2300))
        gilsonSetZTop(pGilObjId,height);
    else
        fprintf(stderr,"height error: %d\n",height);
  
    if (verbose)
    {
        storedheight = gilsonGetZTop(pGilObjId);
        fprintf(stdout,"Stored Arm Height is (mm): %d\n",storedheight);
        fflush(stdout);
    }
}

DefineRack()
{
    char buffer[256];
    char *bptr;

    if (verbose)
    {
        fprintf(stdout,"Rack Definition File Path: ");
        fflush(stdout);
    }

    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

/*
 *  if (rack != NULL)
 *  {
 *     rackDelete(rack);
 *     rack = NULL;
 *  }
 */

    rack = rackCreate(bptr);
    rackCenter(rack, RackCenter[0],RackCenter[1]);
}

SaveParameter()
{
    char buffer[256];
    char *bptr;

    InjectorCenter[0] = InjectorCenter[0]+ InjectorDelta[0];
    InjectorCenter[1] = InjectorCenter[1]+ InjectorDelta[1];
    InjectorBot = InjectorPort1Bot;
    /* pGilObjId->InjectStation[2] = LastZ; */

    RackCenter[0] = RackCenter[0]+ RackLoc1Delta[0];
    RackCenter[1] = RackCenter[1]+ RackLoc1Delta[1];
    SampleTop = RackLoc1Top;

    if (verbose)
    {
        fprintf(stdout,
                "Save Rinse Station LOcation: X = %d, Y = %d, Z =  %d ?",
                RinseStation[0], RinseStation[1], RinseStationZ);
        fprintf(stdout,
                "Injector Center Location :  X = %d, Y = %d, Z =  %d \n",
                InjectorCenter[0], InjectorCenter[1], InjectorBot);
        fprintf(stdout,
                "Rack Center Location :  X = %d, Y = %d, Z =  %d \n",
                RackCenter[0], RackCenter[1], SampleTop);
    }
    else
    {
      fprintf(stdout," %d  %d  %d \n",
          RinseStation[0], RinseStation[1], RinseStationZ);
      fprintf(stdout," %d  %d  %d \n",
         InjectorCenter[0], InjectorCenter[1], InjectorBot);
      fprintf(stdout,"%d  %d  %d \n",
         RackCenter[0], RackCenter[1], SampleTop);
    }
    fflush(stdout);

    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    if ((toupper(buffer[0])) == 'Y' )
        gilsonSetRStation(pGilObjId, RinseStation[0], 
                          RinseStation[1], RinseStationZ);
}

PrintSet()
{
    fprintf(stdout,"\n");
    fprintf(stdout,"Center of Rack:  X = %d, Y = %d Z = %d\n",
            RackCenter[0], RackCenter[1], SampleTop);
    fprintf(stdout,"Center of Injector:  X = %d, Y = %d, Bottom: %d \n",
            InjectorCenter[0], InjectorCenter[1],InjectorBot);
    fprintf(stdout,"MaxFlow: %0.1lf ml/min, Max Syringe Volume: %d\n",
            MaxFlow,MaxVolume);
    fprintf(stdout,"\n");
    fflush(stdout);
}

setValve()
{
    char buffer[256];
    char *bptr;
    int setting;
    while(1)
    {
/***** CHIN *****/
       if (verbose)
           fprintf(stdout,"Air Valve to Probe, pressure=1 vent=0, q-quit: ");
       else
           fprintf(stdout,"Cmd:\n");

       fflush(stdout);

       bptr = fgets(buffer, sizeof(buffer), stdin);
       if (bptr == NULL)
           return;

       if ((buffer[0] != '0') && (buffer[0] != '1'))
           return;

       setting = atoi(strtok(bptr," "));
       gilSetContacts(pGilObjId, 2, setting );
    }
}


setOutputs()
{
    char buffer[256];
    char *bptr;
    int contact,state;

    while(1)
    {
/***** CHIN *****/
        if (verbose)
            fprintf(stdout,"Contact # and state one=1 off=0, q-quit: ");
        else
            fprintf(stdout,"Cmd:\n");

        fflush(stdout);

        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;

        if (*bptr == 'q')
            return;

        contact = atoi(strtok(bptr," ,"));
        state = atoi(strtok(NULL," ,"));
        /* fprintf(stdout,"Contact # %d,  state %d\n",contact,state); */
        if ((contact < 1) || (contact > 4))
            return;
        if ((state != 0) && (state != 1))
            return;

        gilSetContacts(pGilObjId, contact, state );
    }
}

getInputs()
{
    char buffer[256];
    char *bptr;
    int contact,state;
    while(1)
    {
/***** CHIN *****/
        if (verbose)
            fprintf(stdout,"External Input #  q-quit: ");
        else
            fprintf(stdout,"Cmd:\n");

        fflush(stdout);

        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;

        if (*bptr == 'q')
            return;

        contact = atoi(strtok(bptr," ,"));
        /* fprintf(stdout,"Input # %d \n",contact); */
        if ((contact < 1) || (contact > 4))
            return;

        gilGetInputs(pGilObjId, contact, &state );
     
        fprintf(stdout,"External Input %d, state is: %d\n",contact,state);
    }
}

setInjector()
{
    char buffer[256];
    char *bptr;

    while(1)
    {
        if (verbose)
        {
            fprintf(stdout,
                    "Injector Valve to Probe, (Inject=1 Load=0), q-quit: ");
        }
        else
            fprintf(stdout,"Cmd:\n");

        fflush(stdout);

        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;

        if ((buffer[0] != '0') && (buffer[0] != '1'))
            return;

        if (buffer[0] == '1')
            gilInject(pGilObjId);
        else
            gilLoad(pGilObjId);

    }
}

/*
 * Allow TCL front end to send Injector & Rack Center information
 *   to override defaults
 */
setCenters()
{
    char buffer[256];
    char *bptr;
    char *cmd;

    if (verbose)
    {
        fprintf(stdout,"i)injector or r)ack X Y Z: ");
        fflush(stdout);
    }

    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    cmd = strtok(bptr," ,");

    if ((toupper(cmd[0])) == 'I' )
    {
        int xAxis,yAxis;
        int portnum = 1;

        InjectorCenter[0] = atoi(strtok(NULL," ,"));
        InjectorCenter[1] = atoi(strtok(NULL," ,"));
        InjectorBot = atoi(strtok(NULL," ,"));
        rackCenter(inject, InjectorCenter[0],InjectorCenter[1]);
        xAxis = rackGetX(inject, ZONE1,portnum);
        yAxis = rackGetY(inject, ZONE1,portnum);
        gilInitInjLoc(pGilObjId,xAxis, yAxis, InjectorBot);
    }
    else if ((toupper(cmd[0])) == 'R' )
    {
        RackCenter[0] = atoi(strtok(NULL," ,"));
        RackCenter[1] = atoi(strtok(NULL," ,"));
        SampleTop = atoi(strtok(NULL," ,"));
        rackCenter(rack, RackCenter[0],RackCenter[1]);
    }
}

PrimePump()
{
    int sampZtop,sampZbottom,i;

    /* Rinse Needle */
    sampZtop = rackSampTop(rack,ZONE1,1);
    gilWriteDisplay(pGilObjId,"Pri Pump"); /* max of 8 chars */
    gilMoveZ2Top(pGilObjId);
    gilMove2Rinse(pGilObjId);

      /* gilMoveXY(pGilObjId,20,20);
       * gilMoveXY(pGilObjId,xyzMinMax[0]+20,xyzMinMax[2]+20);
       * gilMoveZ(pGilObjId,sampZtop-130);
       */

    for (i=0;i < 10; i++)
        gilsonFlush(pGilObjId, MaxVolume, MaxFlow, MaxFlow );

    gilMoveZ2Top(pGilObjId);
    gilWriteDisplay(pGilObjId,""); /* max of 8 chars */
}

gotoHome()
{
    gilMoveZ2Top(pGilObjId);
    /* gilMoveXY(pGilObjId,20,20); */
    gilMoveXY(pGilObjId,xyzMinMax[0]+20,xyzMinMax[2]+20);
}
 
PumpReplace()
{
    char buffer[256];
    char resp[256];
    char *bptr;
    char *cmd;
    double maxflow;
    int result;
    int pumptype,pumpvol;

    if (verbose)
    {
        fprintf(stdout,"Pump Type: %d (2=gilson),  Pump Volume: %d ul\n",
                pGilObjId->pumpType, pGilObjId->pumpVolume);
        fprintf(stdout,"Ready to Position Plunger for Replacement? "
                       "(y or n): ");
    }
    else
        fprintf(stdout,"%d %d\n",pGilObjId->pumpType,pGilObjId->pumpVolume);

    fflush(stdout);
    maxflow = gilsonMaxFlowRate(pGilObjId)/2.0;

    while(1)
    {
        fprintf(stdout,"Cmd:\n");
        fflush(stdout);
        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;
        switch( toupper(buffer[0]) )
        {
            case 'N':
                cmd = strtok(bptr," ,");
                pumptype = atoi(strtok(NULL," ,"));
                pumpvol = atoi(strtok(NULL," ,"));
                gilsonSetPump(pGilObjId,pumptype,pumpvol);
                maxflow = gilsonMaxFlowRate(pGilObjId)/2.0;
                fprintf(stdout,"%d %lf\n",pumpvol,maxflow);
                fflush(stdout);
                break;

            case 'U':
                result = gilHomeDiluter(pGilObjId);
                break;

            case 'D':
                result = gilPump(pGilObjId, 'R', (double)
                                 pGilObjId->pumpVolume, maxflow, 0);
                result = gilPump(pGilObjId, 'N',(double) -1.0, maxflow, 0);
                break;
            case 'O':
                result = gilInitSyringe(pGilObjId);
                break;
            case 'Q':
                return;

        } /* end switch */

    } /* end while */

#ifdef XXXX

          
    if (toupper(buffer[0]) != 'Y')
        return(0);

    maxflow = gilsonMaxFlowRate(pGilObjId)/2.0;

  /* down */
    result = gilPump(pGilObjId, 'R',(double) pGilObjId->pumpVolume,
                     maxflow, 0);
    result = gilPump(pGilObjId, 'N',(double) -1.0, maxflow, 0);
  
    if (verbose)
    {
        fprintf(stdout,"Replace Syringe NOW, press CR when done.: ");
        bptr = fgets(buffer, sizeof(buffer), stdin);
        fprintf(stdout,"OK to Home Syringe NOW, press CR: ");
    }
    bptr = fgets(buffer, sizeof(buffer), stdin);
    /* Up */
    result = gilHomeDiluter(pGilObjId);

#ifdef XXX
    fprintf(stdout,"Volume in ul of new Pump: \n");
    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;
    volume  = atoi(bptr);

    fprintf(stdout,"New Volume: %d ul\n",volume);
    gilsonSetPump(pGilObjId,2/* pump_type*/,volume);
    maxflow = gilsonMaxFlowRate(pGilObjId)/2.0;
    fprintf(stdout,"Pump Type: %d (2=gilson),  Pump Volume: %d ul\n",
            pGilObjId->pumpType, pGilObjId->pumpVolume);

#endif

    gilMoveZ2Top(pGilObjId);
    /* gilMoveXY(pGilObjId,20,20); */
    gilMoveXY(pGilObjId,xyzMinMax[0]+20,xyzMinMax[2]+20);
    /* result = gilsonBCmd(pGilObjId->gilsonID,"d",resp); */
#endif
}

RinseAlignment()
{
    char buffer[256];
    char *bptr;
    int LastZ;

    if (verbose)
    {
        fprintf(stdout,"Rinse Station Location :  X = %d, Y = %d, Z =  %d \n",
                pGilObjId->RinseStation[0], pGilObjId->RinseStation[1],
                pGilObjId->RinseStation[2]);
    }
/*CHIN
 *  else
 *  {
 *      fprintf(stdout,"%d %d %d\n", pGilObjId->RinseStation[0], 
 *              pGilObjId->RinseStation[1], pGilObjId->RinseStation[2]);
 *  }
 */

    fflush(stdout);
    LastZ = pGilObjId->RinseStation[2];

    /* Don't want needle to come down yet */
    pGilObjId->RinseStation[2] = pGilObjId->ZTopClamp;
    gilMoveZ2Top(pGilObjId);
    gilMove2Rinse(pGilObjId);
    delayMsec(250);
    gilGetXY(pGilObjId,XYLoc);
    gilGetZ(pGilObjId,&ZLoc);
    Xincr = 1;
    Yincr = 1;
    Zincr = 2;
    AjustXYZ();
    delayMsec(250);
    gilGetXY(pGilObjId,XYLoc);
    gilGetZ(pGilObjId,&ZLoc);
    if (verbose)
        fprintf(stdout,"Present Location :  X = %d, Y = %d, Z =  %d \n",
                XYLoc[0], XYLoc[1],ZLoc);
    else
        fprintf(stdout,"%d %d %d\n", XYLoc[0], XYLoc[1],ZLoc);

    fflush(stdout);

    RinseStation[0] = XYLoc[0];
    RinseStation[1] = XYLoc[1];
    if (ZLoc != pGilObjId->ZTopClamp)
        RinseStationZ = ZLoc;
    if (verbose)
        fprintf(stdout,"Rinse Station Location :  X = %d, Y = %d, Z =  %d \n",
                pGilObjId->RinseStation[0], pGilObjId->RinseStation[1],
                pGilObjId->RinseStation[2]);
/*CHIN
 *  else
 *      fprintf(stdout,"%d %d %d\n", pGilObjId->RinseStation[0],
 *              pGilObjId->RinseStation[1], pGilObjId->RinseStation[2]);
 */

    fflush(stdout);

    while(1)
    {
        fprintf(stdout,"Cmd:\n");
        fflush(stdout);
        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;
        switch( toupper(buffer[0]) )
        {
            case 'N':
                gilsonSetRStation(pGilObjId, RinseStation[0],
                                  RinseStation[1], RinseStationZ);
                break;

            case 'Q':
                gilMoveZ2Top(pGilObjId);
                /* gilMoveXY(pGilObjId,20,20); */
                gilMoveXY(pGilObjId,xyzMinMax[0]+20,xyzMinMax[2]+20);
                return;
        }
    }
}

InjectAlignment()
{
    char buffer[256];
    char *bptr;
    int X,Y,Z;
    int LastX,LastY,LastZ;
    int Xdelta,Ydelta,Zdelta;
    int xAxis,yAxis;
    int portnum = 1;

    if (verbose)
    {
        fprintf(stdout,"Injector Rack Center Location :  X = %d, Y = %d \n",
                InjectorCenter[0],InjectorCenter[1]);
        fprintf(stdout,"Injector Port Location :  X = %d, Y = %d, Z =  %d \n",
                pGilObjId->InjectStation[0], pGilObjId->InjectStation[1],
                pGilObjId->InjectStation[2]);
    }
/*CHIN
 *  else
 *  {
 *      fprintf(stdout,"%d %d %d\n",
 *              pGilObjId->InjectStation[0], pGilObjId->InjectStation[1],
 *              pGilObjId->InjectStation[2]);
 *  }
 */

    LastX = pGilObjId->InjectStation[0];
    LastY = pGilObjId->InjectStation[1];
    LastZ = pGilObjId->InjectStation[2];

    fflush(stdout);

    /* Don't want needle to come down yet */
    pGilObjId->InjectStation[2] = pGilObjId->ZTopClamp;

    gilMoveZ2Top(pGilObjId);
    gilMove2Inj(pGilObjId);
    Xincr = 1;
    Yincr = 1;
    Zincr = 2;
    delayMsec(250);
    gilGetXY(pGilObjId,XYLoc);
    gilGetZ(pGilObjId,&ZLoc);
    AjustXYZ();
    delayMsec(250);
    gilGetXY(pGilObjId,XYLoc);
    gilGetZ(pGilObjId,&ZLoc);
    if (verbose)
        fprintf(stdout,"Present Location :  X = %d, Y = %d, Z =  %d \n",
                XYLoc[0], XYLoc[1],ZLoc);
    else
       fprintf(stdout,"%d %d %d\n", XYLoc[0], XYLoc[1],ZLoc);

    fflush(stdout);

    Xdelta = XYLoc[0] - LastX;
    Ydelta = XYLoc[1] - LastY;
    if (verbose)
        fprintf(stdout,"Delta changes: X = %d, Y = %d \n",
                XYLoc[0] - LastX, XYLoc[1] - LastY);

    InjectorDelta[0] = Xdelta;
    InjectorDelta[1] = Ydelta;
    if (ZLoc != pGilObjId->ZTopClamp)
        /* InjectorPort1Bot = ZLoc; */
        InjectorBot = ZLoc;

    while(1)
    {
        fprintf(stdout,"Cmd:\n");
        fflush(stdout);
        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;
        switch( toupper(buffer[0]) )
        {
            case 'N':
                InjectorCenter[0] += InjectorDelta[0];
                InjectorCenter[1] += InjectorDelta[1];

                rackCenter(inject, InjectorCenter[0],InjectorCenter[1]);
                xAxis = rackGetX(inject, ZONE1,portnum);
                yAxis = rackGetY(inject, ZONE1,portnum);
                gilInitInjLoc(pGilObjId,xAxis, yAxis, InjectorBot);
                fprintf(stdout," %d  %d  %d \n",
                        InjectorCenter[0], InjectorCenter[1], InjectorBot);
                fflush(stdout);
                break;

            case 'Q':
                gilMoveZ2Top(pGilObjId);
                gilMoveXY(pGilObjId,xyzMinMax[0]+20,xyzMinMax[2]+20);
                /* gilMoveXY(pGilObjId,20,20); */
                return;
        }
    }
}

TrayAlignment()
{
    int X,Y,Z;
    int LastX,LastY;
    int Xdelta,Ydelta;
    char buffer[256];
    char *bptr;

    if (verbose)
        fprintf(stdout,"Rack Center Location :  X = %d, Y = %d \n",
                RackCenter[0],RackCenter[1]);
/*CHIN
 *  else
 *      fprintf(stdout,"%d %d \n", RackCenter[0],RackCenter[1]);
 */

    fflush(stdout);

    LastX = X = rackGetX(rack, ZONE1,1);
    LastY = Y = rackGetY(rack, ZONE1,1);
    if (verbose)
        fprintf(stdout,"Sample 1 Location :  X = %d, Y = %d \n", X,Y);
/*CHIN
 *  else
 *      fprintf(stdout,"%d %d\n", X,Y);
 */

    gilMoveZ2Top(pGilObjId);
    gilMoveXY(pGilObjId,X,Y);
    Xincr = 1;
    Yincr = 1;
    Zincr = 2;
    delayMsec(250);
    gilGetXY(pGilObjId,XYLoc);
    gilGetZ(pGilObjId,&ZLoc);
    AjustXYZ();
    delayMsec(250);
    gilGetXY(pGilObjId,XYLoc);
    gilGetZ(pGilObjId,&ZLoc);

/*CHIN*/
    if (verbose)
        fprintf(stdout,"Present Location :  X = %d, Y = %d, Z =  %d \n",
                XYLoc[0], XYLoc[1],ZLoc);
    else
        fprintf(stdout,"%d %d %d\n", XYLoc[0], XYLoc[1],ZLoc);

    fflush(stdout);

    Xdelta = XYLoc[0] - LastX;
    Ydelta = XYLoc[1] - LastY;
    if (verbose)
        fprintf(stdout,"Delta changes: X = %d, Y = %d \n",
                Xdelta,Ydelta);

    RackLoc1Delta[0] = Xdelta;
    RackLoc1Delta[1] = Ydelta;
    RackLoc1Top = ZLoc;

    gilMoveZ2Top(pGilObjId);
    while(1)
    {
        fprintf(stdout,"Cmd:\n");
        fflush(stdout);
        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            return;
        switch( toupper(buffer[0]) )
        {
            case 'N':
                RackCenter[0] += RackLoc1Delta[0];
                RackCenter[1] += RackLoc1Delta[1];
                fprintf(stdout,"%d  %d\n",
                        RackCenter[0], RackCenter[1]);
                fflush(stdout);
                break;

            case 'Q':
                gilMoveZ2Top(pGilObjId);
                gilMoveXY(pGilObjId,xyzMinMax[0]+20,xyzMinMax[2]+20);
                /* gilMoveXY(pGilObjId,20,20); */
                return;
        }
    }
}

AjustXYZ()
{
    char buffer[256];
    char *bptr;
    int XY[2],Z;
    int X,Y;
    int incrVal;
    int len = 1;
    float vol;
    int done = 0;

    if (verbose)
        fprintf(stdout,"Adjust Position via keys u)p, d)own, k&l - "
                       "left & right, bf-back-front (q-quits):  \n");

    while(!done)
    {

        if (verbose)
        {
            fprintf(stdout,"Present Location :  X = %d, Y = %d, Z =  %d \n",
                    XYLoc[0], XYLoc[1],ZLoc);
            fprintf(stdout,"Cmd: ");
        }
        else if (len)
        {
            fprintf(stdout," %d %d %d\n", XYLoc[0], XYLoc[1],ZLoc);
            fprintf(stdout,"Cmd:\n");
        }
        fflush(stdout);

        bptr = fgets(buffer, sizeof(buffer), stdin);
    
/*CHIN*/
        if (verbose)
        {
            fprintf(stdout,"char = 0x%x\n",buffer[0]);
            fflush(stdout);
        }

        if (bptr == NULL)
            return;
        len = strlen(buffer);
        if (len > 1)
            incrVal = atoi(strtok(&(buffer[1])," "));
        else
            incrVal = 0;
        switch( toupper(buffer[0]) )
        {
            case 'q':
            case 'Q':
                done = 1;
                break;
        
            case 'U':
                if ( (incrVal <= 0) || (incrVal > 100))
                    ZLoc = ZLoc + Zincr;
                else
                    ZLoc = ZLoc + incrVal;

                /* pGilId->ZTopClamp; Max Z, as read from gilson's EEprom */
                if (ZLoc > xyzMinMax[5] )
                    ZLoc = xyzMinMax[5];
                gilMoveZ(pGilObjId,ZLoc);
                break;

            case 'D':
                if ( (incrVal <= 0) || (incrVal > 100))
                    ZLoc = ZLoc - Zincr;
                else
                    ZLoc = ZLoc - incrVal;
                if (ZLoc < xyzMinMax[4])
                    ZLoc = xyzMinMax[4];
                gilMoveZ(pGilObjId,ZLoc);
                break;

            case 'K':
                if ( (incrVal <= 0) || (incrVal > 100))
                    XYLoc[0] = XYLoc[0] - Xincr;
                else
                    XYLoc[0] = XYLoc[0] - incrVal;
                if (XYLoc[0] < xyzMinMax[0])        /* X min */
                    XYLoc[0] = xyzMinMax[0];
                gilMoveXY(pGilObjId,XYLoc[0],XYLoc[1]);
                break;

            case 'L':
                if ( (incrVal <= 0) || (incrVal > 100))
                    XYLoc[0] = XYLoc[0] + Xincr;
                else
                    XYLoc[0] = XYLoc[0] + incrVal;
                if (XYLoc[0] > xyzMinMax[1])        /* X Max */
                    XYLoc[0] = xyzMinMax[1];
                gilMoveXY(pGilObjId,XYLoc[0],XYLoc[1]);
                break;

            case 'B':
                if ( (incrVal <= 0) || (incrVal > 100))
                    XYLoc[1] = XYLoc[1] - Yincr;
                else
                    XYLoc[1] = XYLoc[1] - incrVal;
                if (XYLoc[1] < xyzMinMax[2])     /* Y Min */
                    XYLoc[1] = xyzMinMax[2];
                gilMoveXY(pGilObjId,XYLoc[0],XYLoc[1]);
                break;

            case 'F':
                if ( (incrVal <= 0) || (incrVal > 100))
                    XYLoc[1] = XYLoc[1] + Yincr;
                else
                    XYLoc[1] = XYLoc[1] + incrVal;
                if (XYLoc[1] > xyzMinMax[3])        /* Y Max */
                    XYLoc[1] = xyzMinMax[3];
                gilMoveXY(pGilObjId,XYLoc[0],XYLoc[1]);
                break;

            default:
/*CHIN*/
                if (verbose)
                {
                    fprintf(stdout,"char = 0x%x\n",buffer[0]);
                    fflush(stdout);
                }
                break;

        } /* switch */
    } /* while */
}


chkRack()
{
    char buffer[256],filepath[256];
    char *bptr;
    char *cmd;
    char msge[10];
    int xAxis,yAxis;
    int i,a,j,maxflow;
    int sampZtop,sampZbottom;
    int error;
    int startZone,endZone,SampStrt,SampEnd;
    int startOrder,endOrder,o;
    int lower,Delay;
    RACKOBJ_ID tstrack;

    fprintf(stdout,"Rack File Path (eg. ./code_205.grk): ");
    fflush(stdout);

    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    cmd = strtok(bptr," ,");
    strcpy(filepath,cmd);

    fprintf(stdout,"Opening Rack: '%s'\n",filepath);
    fflush(stdout);

    DebugLevel= 4;
    tstrack = rackCreate(filepath);
    rackCenter(tstrack, 546,1872);
    if (verbose)
        rackShow(tstrack, 5);

    fprintf(stdout, "Well Ordering: 0-BL2R, 1-BL2RZ, 2-BR2L, "
                    "3-BR2LZ, 4-LB2F, 5-LB2FZ, 6-LF2B, 7-LF2BZ\n ");
    fprintf(stdout, "     8-FL2R, 9-FL2RZ, 10-FR2L, 11-FR2LZ, "
                    "12-RB2F, 13-RB2FZ, 14-RF2B, 15-RF2BZ\n ");
    fprintf(stdout,"Select Range(e.g. 0 0, or 4 4, or 0 15): ");
    fflush(stdout);
    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    startOrder = atoi(strtok(bptr," ,"));
    endOrder = atoi(strtok(NULL," ,"));

    fprintf(stdout,"Start & End Zone#s & Sample#s (eg. 1 1 1 96): ");
    fflush(stdout);

    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    startZone = atoi(strtok(bptr," ,"));
    endZone = atoi(strtok(NULL," ,"));
    SampStrt = atoi(strtok(NULL," ,"));
    SampEnd = atoi(strtok(NULL," ,"));

    fprintf(stdout,"Lower Needle into Sample Well? (y or n): ");
    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    cmd = strtok(bptr," ,");
    if (strcmp(cmd,"y") == 0)
        lower = 1;
    else
        lower = 0;

    fprintf(stdout,"Delay (msec) between axis movement (1000 = 1 sec): ");
    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;

    Delay = atoi(strtok(bptr," ,"));

    fprintf(stderr,"Well Sequence Order %d thru %d\n",startOrder,endOrder);
    fprintf(stderr,"Zones %d thru %d\n",startZone,endZone);
    fprintf(stderr,"Samples %d thru %d\n",SampStrt,SampEnd);
    fprintf(stderr,"Needle '%s' Lowered into Sample Well.\n",
            ((lower==1) ? "Will Be" : "Will NOT Be"));
    fprintf(stderr,"Delay between Axises Movements %d \n",Delay);
    fprintf(stderr,"OK to Proceed? (y or n): ");
    bptr = fgets(buffer, sizeof(buffer), stdin);
    if (bptr == NULL)
        return;
    if (toupper(buffer[0]) != 'Y')
        return(0);
    fprintf(stderr,"\n\nRunning through Rack Positions for "
                   "Verification.\n\n");
    for(o=startOrder; o <= endOrder; o++)
    {
        rackWellOrder(tstrack, o);
        for(j=startZone;j<=endZone;j++)
        {
            for(i=SampStrt;i<=SampEnd;i++)
            {
                /* ----------  Rinse Needle ------------- */
                fprintf(stderr,"Sample %d\n",i);
                error = gilMoveZ2Top(pGilObjId);

                sprintf(msge,"Samp: %d",i);
                gilWriteDisplay(pGilObjId,msge); /* max of 8 chars */

                /* Get Sample Location in Tray */
                xAxis = rackGetX(tstrack, j,i);
                yAxis = rackGetY(tstrack, j,i);
                sampZtop = rackSampTop(tstrack,j,i);
                sampZbottom = rackSampBottom(tstrack,j,i);

                /* ----------  Get Sample -------------- */
                fprintf(stderr,"Zone: %d, xAxis: %d, yAxis: %d\n",
                        j,xAxis,yAxis);
                fprintf(stderr,"Sample Top: %d, Bottom: %d\n",
                        sampZtop,sampZbottom);
                gilMoveXY(pGilObjId,xAxis,yAxis);
                if (Delay != 0)
                    delayMsec(Delay);
                if (lower == 1)
                {
                    gilMoveZ(pGilObjId,sampZtop);
                    if (Delay != 0)
                        delayMsec(Delay);
                    /* gilMoveZ(pGilObjId,sampZtop-180); */
                    gilMoveZ(pGilObjId,sampZbottom);
                    if (Delay != 0)
                        delayMsec(Delay);
                    gilMoveZ2Top(pGilObjId);
                }
            }
        }
        gilMoveZ2Top(pGilObjId);
    }
    gilMoveZ2Top(pGilObjId);
    gilMoveXY(pGilObjId,15,15); 
    gilWriteDisplay(pGilObjId,"Complete"); /* max of 8 chars */
}

chkLQZ()
{
    /* display Liquid Sensing Frequency on the Gilson Display */
    gilsonTestMode(pGilObjId,3);
}


/* dummy function to satisfy tclfuncs.c */
reportRobotStat(int stat)
{
    return(0);
}

GilsonTransparentMode()
{
    /* guide: any single letter command is an Immediate command except
     *        '9' can be both, does different things depending on mode used
     *        'd' is buffer and homes diluter
     *        'e' either reads or clears errors on usage
     *        'H' home XYZ
     *        any multi letter command is a Buffered command
     *
     * use the commented out main in gilsonObj.c for transparent mode.
     */
    char buffer[256],response[256];
    char command[40];
    char *bptr;
    char *cmd;
    char *unitId, *type,*stype;
    int unit, len;

    fprintf(stdout,"\n\n");
    fprintf(stdout,"Entering Transparent Mode to the Gilson-215 "
                   "(D A N G E R !)\n");
    fprintf(stdout,"You may enter 'GSIOC' Immediate or Buffered "
                   "Commands to the Gilson\n");
    fprintf(stdout,"  See 'Gilson 215 User's Guide, Appendix D.' for "
                   "the Valid GSIOC Commands\n");
    fprintf(stdout,"\n");
    fprintf(stdout,"Enter the command in the following format: "
                   "Command_Arguments(if any)  UnitId (Optional)\n");
    fprintf(stdout,"Notes: \n");
    fprintf(stdout,"  Commands:  \n");
    fprintf(stdout,"       Single letter commands are interpreted as "
                   "Immediate commands, all\n");
    fprintf(stdout,"       Multi-character Commands  are interpreted as "
                   "Buffered commands.\n");
    fprintf(stdout,"       For the few single character buffered commands "
                   "add the character 'B'.\n");
    fprintf(stdout,"       E.G. for the buffered 'd' command use 'dB' "
                   "rather than just 'd'.\n");
    fprintf(stdout,"       \n");
    fprintf(stdout,"  UnitID: \n");
    fprintf(stdout,"       Varian uses the default UnitId of 22 for the "
                   "Gilson-215 and\n");
    fprintf(stdout,"       29 for the Gilson 819 value.\n");
    fprintf(stdout,"       The default Unit Id for all commands is 22, "
                   "unless a Unit Id is specified\n");
    fprintf(stdout,"       E.G. 'Q' issues the imediate command 'Q' to "
                   "Unit Id 22 (the 215)\n");
    fprintf(stdout,"       E.G. 'V 29' issues the immediate command 'V' "
                   "to Unit Id 29 (the 819)\n");
    fprintf(stdout,"       \n");
    fprintf(stdout,"\n");
    fprintf(stdout,"   Be sure to follow the syntax shown "
                   "in the appendix.   \n");
    fprintf(stdout,"       E.G. X0200/3000   215 goes to X 200 and Y 300\n");
    fprintf(stdout,"       E.G. X            returns the present X Y "
                   "location '0200/0300' \n");
    fprintf(stdout,"\n");
    fprintf(stdout,"       Note: Commands return immediately even if "
                   "Gilson has not completed the action.\n");
    fprintf(stdout,"             It's up to the user to wait when "
                   "appropriate prior to issuing another command.\n");
    fprintf(stdout,"\n");
    fprintf(stdout," W A R N I N G ! ! , This is a diagnostic code , "
                   "for use by Varian Personnel\n");
    fprintf(stdout,"                     knowledgeable in the GSIOC "
                   "commands.\n");
    fprintf(stdout,"                     There are Commands that can "
                   "wipe the Gilson Non-Volatile Memory.\n");
    fprintf(stdout,"                     In such a case the a Gilson Field "
                   "Service Engineer would be required\n");
    fprintf(stdout,"                     to restore the system.\n");
    fflush(stdout);


    while (1)
    {
        fprintf(stdout,"\nCmd (QQ to quit): ");
        fflush(stdout);
        bptr = fgets(buffer, sizeof(buffer), stdin);
        if (bptr == NULL)
            continue;

        cmd = strtok(bptr," ,");

        len = strlen(cmd);
 
        /*
         * fprintf(stdout,"cmd: '%s', len: %d\n",cmd,len);
         * fflush(stdout);
         */

        /* Check for Immediate cmd */
        if (len == 1)
        {
            type = "I";
            stype = "Immediate";
            /* fprintf(stdout,"cmd: '%s', len: %d, type: '%s' \n",
                       cmd,len,stype); */
        }

        /* Check for 'B' as 2nd char or quit */
        else if (len == 2)
        {
            if ( ((cmd[0] == 'Q') || (cmd[0] == 'q')) &&
                 ((cmd[1] == 'Q') || (cmd[1] == 'q')) )
            {
                break;
            }
            else if ((cmd[1] == 'B') || (cmd[1] == 'b'))
            {
                type = "B";
                stype = "Buffered";
                cmd[1] = '\0'; /* null out B */
                /* fprintf(stdout,"cmd: '%s', len: %d, type: '%s' \n",
                           cmd,len,stype); */
            }
            else
            {
                type = "B";
                stype = "Buffered";
                /* fprintf(stdout,"cmd: '%s', len: %d, type: '%s' \n",
                           cmd,len,stype); */
            }
        }
        else
        {
            type = "B";
            stype = "Buffered";
        }

        strcpy(command,cmd);

        unitId = strtok(NULL," ,");

        if (unitId != NULL)
            unit = atoi(unitId);
        else
            unit = 22;

        /* fprintf(stdout," Sending the '%s' command: '%s' to Unit Id: %d\n",
                   stype,command,unit); */
        fprintf(stdout,"\nCmd: '%s',  '%s' to UId: %d, ",command,stype,unit);
        fflush(stdout);

        gilCommand(pGilObjId, unit, command, type, response);

        fprintf(stdout,"reply: '%s'\n",response);
        fflush(stdout);
    }
}
