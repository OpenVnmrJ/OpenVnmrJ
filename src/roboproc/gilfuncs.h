/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCgilfuncsh
#define INCgilfuncsh

#define GIL_MAX_SAMP_TYPES 6
#define GIL_MAX_RACKS 6
#define GIL_MAX_RACK_TYPES 36
 
/* define STDALONE for standalone debugging */
/* #define STDALONE */

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/* ------------------- GILSON 215 Automation Exp.  Structure ------------------- */
#define IDSTRLEN 80

typedef struct {
		  char IdStr[IDSTRLEN];
	       }  TYPE_IDSTR;

typedef struct {
		 RACKOBJ_ID  pRackObj;
		 char  IdStr[IDSTRLEN];  /* racktype Id Strings, (209,205,etc) */
		 int   type;		
	       } DEFINED_RACKS;

typedef struct {
        int	NumRacks;
 DEFINED_RACKS  DefinedRacks[GIL_MAX_RACK_TYPES];/* The Array of Rack Objects (e.g. code_205.grk, 
				     	   code_205H.grk, etc) Loaded from racksetup */	
   RACKOBJ_ID   LoadedRacks[GIL_MAX_RACKS];

        int     Rack1Center[2];		/* Center location of 1st Rack */
        int     InjectorCenter[2];	/* Center of Injector */
 	int 	InjectorBot;	/* Bottom of Injector */
	double  MaxFlow;
	int     MaxVolume;
} G215_BED_OBJ;

typedef G215_BED_OBJ *G215_BED_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

  extern int initGilson215(char *comdev, int gilId, int injId, int pumpId);
  extern int gilSampleSetup(unsigned int sampleloc, char *sampleDir);
  extern int gilPowerUp();
  extern int initRacks(void);
  extern int readRacks(char *rackpath);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif

