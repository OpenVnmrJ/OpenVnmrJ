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
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <termios.h>
#include  <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>

#ifndef GILSCRIPT
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "msgQLib.h"
#include "acquisition.h"
#include "acqcmds.h"
#include "errorcodes.h"
#endif

#include "errLogLib.h"
#include "rackObj.h"
#include "gilsonObj.h"
#include "gilfuncs.h"
#include "iofuncs.h"

#define  ERROR -1

extern char systemdir[];    /* vnmr system directory */

int rackDelta[GIL_MAX_RACKS][2] = {
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

G215_BED_OBJ    g215Bed;
GILSONOBJ_ID pGilObjId = NULL;

/* needs to be a ptr to malloc space for tcl */
char *SampleInfoFile = NULL;
static time_t MDate = 0;

/*****************************************************************
*
*  initialize Gilson Liquid Handler and Comm Port
*
*/
int initGilson215(char *devName, int gilunit, int injunit, int pumpunit)
{
#ifndef STDALONE
    if (pGilObjId == NULL)
    {
        DPRINT2(1,"Initializing Gilson: Id: 0x%lx, Dev: '%s'\n",
                pGilObjId, devName);
        pGilObjId = gilsonCreate(devName, gilunit, injunit, pumpunit);
        if (pGilObjId == NULL)
            return(-1);
    }
    g215Bed.MaxFlow = gilsonMaxFlowRate(pGilObjId); /* double ml/min */
    g215Bed.MaxVolume = gilsonMaxVolume(pGilObjId); /* int  ul */
#else
    g215Bed.MaxFlow = 16.0; /* double ml/min */
    g215Bed.MaxVolume = 500; /* int  ul */
#endif

#ifndef GILALIGN
    gClearVolume();
#endif
    return(0);
}

/******************************************************************
*
*   File Contains the Enter 
* Sample#   Rack# Zone# TrayIndex Volume Flow 
*     e.g.
*   sampleloc is an encoded number
*    lsw, 10k-1k digits are sample type
*         100-1  Tray Location (loc)
*
*    hsw,  1k digit rack zone (1-3)
*	   100 digit rack location on Gilson
*	   10-1 racktype 1-36  
*         
*    long  100 - 1 	Location
*          10k - 1k 	Sample Type
*           1m - 100k 	Rack Type
*          	  10m 	Rack Position
*         	 100m 	Zone of rack
*
*/
int gilSampleSetup(unsigned int sampleloc, char *sampleDir)
{
   
    int racktype,zone,rackLoc,sampType,loc;
    int sampIndex,rackIndex;
    int maxzone,maxsample;

    double zspeed;

    /* if haven't malloc space for SampleInfoFile yet then do so */
    if (SampleInfoFile == NULL)
        SampleInfoFile = (char *) malloc(512);

    /* gilShow(1); */

    /* Decode sample value into gilson rack & sample parameters */
    /* rrzzllll */

    /* Decode sample value into gilson rack & sample parameters */
    loc = sampleloc % 10000;  /* get location */

    /* rr000000 */
    rackLoc = (sampleloc / 1000000) % 100L;

    /* rrzz0000 */
    zone = (sampleloc / 10000L) % 100L;

    sprintf(SampleInfoFile, "%s%s%s%d_%d_%d", systemdir, "/asm/info/",
            sampleDir, rackLoc, zone, loc);
    DPRINT4(1, "\ngilSampleSetup: sampdecode: rackloc: %d,  zone: %d,  "
               "sample: %d,  samplefile: '%s'\n",
            rackLoc, zone, loc, SampleInfoFile);

    return(0);
}


/*--------------------------------------------------------------------
| gilPowerUp
|	reset and home gilson, set syringe volume to 0 (unknown)
+--------------------------------------------------------------------*/
int gilPowerUp()
{
#ifndef STDALONE
    if ( (!gilsonPowered(pGilObjId)) )
    {
        DPRINT(1,"gilPowerUp(): one axis is not powered, reset Gilson\n");
        gilsonReset(pGilObjId);
	gilsonHome(pGilObjId);
#ifndef GILALIGN
        gClearVolume();
#endif
    }
#endif
    return 0;
}

/*--------------------------------------------------------------------
| initRacks
|	initialize index and arrays 
+--------------------------------------------------------------------*/
int initRacks()
{
   int j;

   g215Bed.NumRacks = 0;	/* Number of Racks on Gilson */
   for (j=0; j < GIL_MAX_RACK_TYPES; j++)
      g215Bed.DefinedRacks[j].pRackObj = NULL;

   for (j=0; j < GIL_MAX_RACKS; j++)
      g215Bed.LoadedRacks[j] = NULL;

   return 0;
}

/*--------------------------------------------------------------------
| racksBeenRead
|	determine if racks have been loaded in yet or not
+--------------------------------------------------------------------*/
int racksBeenRead()
{
    /* If injector (type 0) has not been setup then assume the racksetup files
     * have not been read yet
     */
     /* return(  (gilsonParmsId.RackTypeIndex[0] == -1) ? 0 : 1 ); */
     return(  (g215Bed.DefinedRacks[0].pRackObj == NULL) ? 0 : 1 );
}

/*****************************************************************************
 * rack file
 *
 * rack_type rack_pathname injector_port/rack_index X-center Y-center Z-bottom
 *
 * e.g.:
 *  injector /vnmr/asm/racks/m215_inj.grk 1 Center X, Center Y (5576 38)
 *  rack /vnmr/asm/racks/code_205.grk  1 546 1872
 *  rack /vnmr/asm/racks/code_201.grk  2 546+(1*1196) 1872
 *  rack /vnmr/asm/racks/code_204.grk  3 546+(2*1196) 1872
 *  rack /vnmr/asm/racks/code_202.grk  4 546+(3*1196) 1872
 *  rack /vnmr/asm/racks/code_209.grk  5 546+(4*1196) 1872
 *
 ****************************************************************************/
int readRacks(char *rackpath)
{
    FILE *stream;
    struct stat s;
    char eolch;
    char textline[256],buffer[256],path[256];
    char *rackstype,*racktype,*filepath,*port_rack,*X,*Y,*Z;
    char *rackIdStr,*chrptr;
    int status,line,j,t,typefound,len;
    int portnum, xcenter, ycenter, bot;
    int xAxis,yAxis;
    int rackType,rackIndex;
    int sampType,sampIndex;
    int numSRacks;
    

    j=0;
    line = 0;
    numSRacks = 0;

    stat(rackpath, &s);
    DPRINT3(1,"'%s': MDate: %ld, File: %ld \n",rackpath,MDate,s.st_mtime);
    if (s.st_mtime <= MDate) {
        DPRINT(1,"File has not changed.\n");
        return(0);
    }
    DPRINT(1,"File has Changed.\n");

    stream = fopen(rackpath,"r");

    /* does file exist? */
    if (stream == NULL) {
        errLogSysRet(LOGOPT,debugInfo,
           "initRacks: setup file '%s' could not be opened.", rackpath);
        return(ERROR);
    }
 
    while ( ( chrptr = fgets(textline, 256, stream) )  != NULL) {

        /* note: the CR is included in textline */
        j++;

        if ( (len = strlen(textline)) < 15)
            continue;

        if (textline[0] == '#')  /* # then it's a comment */
            continue;

        /* extract the string tokens, Prompt and Text */
        rackstype = (char*) strtok(textline," \n");
        if ( rackstype == NULL )
            continue;

        racktype = (char*) strtok(NULL," \n");
        if ( racktype == NULL )
            continue;

        filepath = (char*) strtok(NULL," \n");
        if ( filepath == NULL )
            continue;

        DPRINT4(1,"\n\n Nracks: %d, Rack: '%s', Type: '%s', filepath: '%s'\n",
                g215Bed.NumRacks,rackstype,racktype,filepath);

        X = (char*) strtok(NULL," \n");
        Y = (char*) strtok(NULL," \n");
        Z = (char*) strtok(NULL," \n");
        port_rack = (char*) strtok(NULL," \n");

        rackType = atoi(racktype);
        if ((rackType < 0) || (rackType > 37)) {
            errLogRet(LOGOPT, debugInfo, "initRacks: Rack Type of "
                                         "Range(0-37): %d \n", rackType);
            fclose(stream);
            return(ERROR);
        }

        strcpy(buffer, filepath);
        rackIdStr = strtok(buffer,"_");
        /* DPRINT1(1,"Id prefix: '%s'\n",rackIdStr); */
        rackIdStr = strtok(NULL,".");
        /* DPRINT1(1,"Id string: '%s'\n",rackIdStr); */
   
        for (typefound=rackIndex=0; rackIndex<g215Bed.NumRacks; rackIndex++) {

            DPRINT4(1,"Index: %d, Type: %d, Defined Type: %d, '%s'\n",
                    rackIndex, rackType, g215Bed.DefinedRacks[rackIndex].type,
                    g215Bed.DefinedRacks[rackIndex].IdStr);

            if (g215Bed.DefinedRacks[rackIndex].type == rackType) {
                typefound = 1;
                break;
            }
        } 

        /* Centers depths etc. for Injector or rack */
        if (rackType == 0)  /* Injector */ {

            portnum = atoi(port_rack);
            xcenter = atoi(X);
            ycenter = atoi(Y);
            bot = atoi(Z);

            DPRINT4(1,"Port: %d, center: X=%d, Y=%d, Bottom: %d\n",
                    portnum, xcenter, ycenter, bot);

            g215Bed.InjectorCenter[0] = xcenter; /* Center of Injector */
            g215Bed.InjectorCenter[1] = ycenter; /* Center of Injector */
            g215Bed.InjectorBot = bot;           /* Bottom of Injector */

            if (typefound) {
                rackCenter(g215Bed.DefinedRacks[rackIndex].pRackObj,
                           xcenter, ycenter); /* 5565 */
                xAxis = rackGetX(g215Bed.DefinedRacks[rackIndex].pRackObj,
                                 1, portnum);
                yAxis = rackGetY(g215Bed.DefinedRacks[rackIndex].pRackObj,
                                 1, portnum);
                DPRINT3(1,"Inject Loc: X=%d, Y=%d, Bottom: %d\n",
                        xAxis,yAxis,bot);

                gilInitInjLoc(pGilObjId,xAxis, yAxis, bot);
            }
        }
        else {
            numSRacks++;

            if ( X != NULL )
                xcenter = atoi(X);
            else
                xcenter = -1;
   
            if ( Y != NULL )
                ycenter = atoi(Y);
            else
                ycenter = -1;

            DPRINT4(1,"NumRacksRead: %d, rackIndex: %d, Center: X=%d, Y=%d\n",
                    numSRacks, rackIndex, xcenter, ycenter);

            if ((xcenter != -1) && (ycenter != -1)) {
                g215Bed.Rack1Center[0] = xcenter; /* Center of Well */
                g215Bed.Rack1Center[1] = ycenter; /* Center of Well */
            }
        }

        if (typefound != 1) {
            /* rack IdStr e.g. 205, 205h, adpt2, etc. */
            strncpy(g215Bed.DefinedRacks[g215Bed.NumRacks].IdStr,
                    rackIdStr, 80);
            g215Bed.DefinedRacks[g215Bed.NumRacks].type = rackType;

            DPRINT4(1, "Adding racktype: '%s', Idstr: '%s', type: %d, "
                       "path: '%s'\n", rackstype, 
                    g215Bed.DefinedRacks[g215Bed.NumRacks].IdStr, 
                    g215Bed.DefinedRacks[g215Bed.NumRacks].type, filepath);

            DPRINT1(1,"rack definition file: '%s'\n", filepath);
         
            if ( (g215Bed.DefinedRacks[g215Bed.NumRacks].pRackObj =
                  rackCreate(filepath)) == NULL) {

                DPRINT1(1, "initRacks: Unable to Initialize Rack: '%s'\n",
                          filepath);
                fclose(stream);
                return(-1);
            }

            if (rackType == 0)  /* Injector */ {

                rackCenter(g215Bed.DefinedRacks[g215Bed.NumRacks].pRackObj,
                           xcenter, ycenter);
                xAxis = rackGetX(
                            g215Bed.DefinedRacks[g215Bed.NumRacks].pRackObj,
                            1, portnum);
                yAxis = rackGetY(
                            g215Bed.DefinedRacks[g215Bed.NumRacks].pRackObj,
                            1, portnum);
                DPRINT3(1,"Inject Loc: X=%d, Y=%d, Bottom: %d\n",
                        xAxis, yAxis, bot);

                gilInitInjLoc(pGilObjId, xAxis, yAxis, bot);
            }

            g215Bed.NumRacks++;
        }
    }

    fclose(stream);
    /* gilShow(0); */
    MDate = s.st_mtime;

    /*------------ HERMES - READ RACK DELTAS FROM CONFIG FILE -------------*/
    sprintf(path, "%s/asm/info/liqhandlerInfo", systemdir);
    stream = fopen(path, "r");
    if (stream != NULL) {
        int x1,y1,x2,y2,x3,y3,x4,y4;
        if ((fscanf(stream, "%[^\n]\n", textline) != 1) ||
            (fscanf(stream, "X1Delta:  %d\r\n", &x1) != 1) ||
            (fscanf(stream, "Y1Delta:  %d\r\n", &y1) != 1) ||
            (fscanf(stream, "X2Delta:  %d\r\n", &x2) != 1) ||
            (fscanf(stream, "Y2Delta:  %d\r\n", &y2) != 1) ||
            (fscanf(stream, "X3Delta:  %d\r\n", &x3) != 1) ||
            (fscanf(stream, "Y3Delta:  %d\r\n", &y3) != 1) ||
            (fscanf(stream, "X4Delta:  %d\r\n", &x4) != 1) ||
            (fscanf(stream, "Y4Delta:  %d\r\n", &y4) != 1)) {
            DPRINT3(0, "\r\nError reading 768AS liquid handler "
                    "setup file %s %d %s\r\n", path, errno,
                    strerror(errno));
        }
        else {
            rackDelta[0][0] = x1; rackDelta[0][1] = y1;
            rackDelta[1][0] = x2; rackDelta[1][1] = y2;
            rackDelta[2][0] = x3; rackDelta[2][1] = y3;
            rackDelta[3][0] = x4; rackDelta[3][1] = y4;
        }

        fclose(stream);
    }

    return (0);
}

int gilShow(int level)
{
    int location, type;
    int i,j;

    DPRINT(0,"Defined Racks: \n");
    for(j=0;j<g215Bed.NumRacks;j++)
    {

        DPRINT4(0,"Rack[%d]: 0x%lx, Id: '%s', Type: %d\n", j,
            g215Bed.DefinedRacks[j].pRackObj,
            g215Bed.DefinedRacks[j].IdStr,
            g215Bed.DefinedRacks[j].type);
    }

    DPRINT(0,"\n\nCenters: \n");
    DPRINT3(0,"Injector: X: %d, Y: %d, Z: %d\n",
        g215Bed.InjectorCenter[0],
        g215Bed.InjectorCenter[1],
        g215Bed.InjectorBot);
    DPRINT2(0,"1st Rack: X: %d, Y: %d\n",
	g215Bed.Rack1Center[0],
        g215Bed.Rack1Center[1]);

    DPRINT(0,"Loaded Racks: \n");
    for(j=0;j<GIL_MAX_RACKS;j++)
    {

        if (g215Bed.LoadedRacks[j] != NULL)
            DPRINT2(0,"Rack[%d]: 0x%lx \n", j, g215Bed.LoadedRacks[j]);
    }

    return 0;
}
