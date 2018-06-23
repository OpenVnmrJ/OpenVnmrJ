/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCrackh
#define INCrackh

/* #define NEW_TRAY_ROUTINES  to compile the new geometry file reader and routines */
/* when fully implimented should provide a direct compaible Gilson grk files
   with no modification
*/
#define NEW_TRAY_ROUTINES

#define BACKLEFT2RIGHT 		0
#define BACKLEFT2RIGHTZIGZAG 	1
#define BACKRIGHT2LEFT 		2
#define BACKRIGHT2LEFTZIGZAG	3
#define LEFTBACK2FRONT		4
#define LEFTBACK2FRONTZIGZAG	5
#define LEFTFRONT2BACK		6
#define LEFTFRONT2BACKZIGZAG	7
#define FRONTLEFT2RIGHT 	8
#define FRONTLEFT2RIGHTZIGZAG 	9
#define FRONTRIGHT2LEFT 	10
#define FRONTRIGHT2LEFTZIGZAG	11
#define RIGHTBACK2FRONT		12
#define RIGHTBACK2FRONTZIGZAG	13
#define RIGHTFRONT2BACK		14
#define RIGHTFRONT2BACKZIGZAG	15

#define NW_2_E 		BACKLEFT2RIGHT
#define NW_2_E_ZIGZAG 	BACKLEFT2RIGHTZIGZAG
#define NW_2_S		LEFTBACK2FRONT
#define NW_2_S_ZIGZAG	LEFTBACK2FRONTZIGZAG
#define NE_2_W 		BACKRIGHT2LEFT
#define NE_2_W_ZIGZAG	BACKRIGHT2LEFTZIGZAG
#define NE_2_S		RIGHTBACK2FRONT
#define NE_2_S_ZIGZAG	RIGHTBACK2FRONTZIGZAG
#define SW_2_N 		LEFTFRONT2BACK
#define SW_2_N_ZIGZAG	LEFTFRONT2BACKZIGZAG
#define SW_2_E 		FRONTLEFT2RIGHT
#define SW_2_E_ZIGZAG 	FRONTLEFT2RIGHTZIGZAG
#define SE_2_W 		FRONTRIGHT2LEFT
#define SE_2_W_ZIGZAG	FRONTRIGHT2LEFTZIGZAG
#define SE_2_N		RIGHTFRONT2BACK
#define SE_2_N_ZIGZAG	RIGHTFRONT2BACKZIGZAG

#define ORDERINGMAX		15

/* #define NEW_TRAY_ROUTINES */

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif




/* ------------------- Rack Object Structure ------------------- */


/* Geometery line within the grk file */
/*    e.g. 0.0  -13.5  6.0  79.5  119.3 or 0.0  -35.9  [Third Row] */
typedef struct _geo_line_ {
		int xval;
		int yval;
		int diameter;
		int bottom;
		int top;
                int zval;       /* 3rd value in a line referencing another block */
                int thetaAngle;      /* 4th value in a line referencing another block */
                char blockRef[128];
                struct  _geoblock_ *geoBlockRef;
} GEO_LINE;

typedef GEO_LINE *GEO_LINE_ID;
		
/* Geometry Block, define by a label i.e. [Microplate] which can contain several
   geometry lines of x,y, etc. values with a possible reference to another Block
   rows & columns are a guess, and are deduced by lines having X values and no Y or
   visavirsa
*/
typedef struct  _geoblock_ {
		char Label[80];
		int rows;   /*  set to nentries if all gelLine X values are equal */ 
                int columns;/* set to nentries if all gelLine  Y values are equal */
                int  leafGeoLines;   /* true is geoline are leafs (no references) */
		int  nentries;
                GEO_LINE_ID geoLine[20];
} BLOCK_OBJ;

typedef BLOCK_OBJ *BLOCK_ID;

/* A list of all the blocks that were within the grk file 
   the block with no name is label "RackZones" and is a key word used
   within the program
*/
typedef struct  {
                 int nBlocks;
		 BLOCK_ID geoblocks[30];
} BLOCK_LIST ;

/* one for each zone on tray/rack */
/* the rows and columns are guesses based on lines that have only X or only Y values */
/* X = columns, Y = rows */
typedef struct _zoneInfo_ {
		int totalSamples;
		int rows;
		int columns;
                int     Ordering;
		GEO_LINE_ID zoneLine;
} ZONE_INFO_OBJ;
typedef ZONE_INFO_OBJ *ZONE_INFO_ID;



struct  _label_obj {
		char Label[80];
		int  nentries;
		int  xyloc[20][5];
		char RefLab[20][80];
		struct _label_obj *RefLabel;
};

typedef struct  _label_obj LABEL_OBJ;


typedef struct {
	char 	ZoneIdStr[40];	/* user identifier string */
	LABEL_OBJ *pLabelId;
	int	Columns;	/* Column */
	int	Rows;		/* Rows */
        int     Ordering;
} ZONE_OBJ;
 

typedef struct {
	char 	pRackIdStr[40];	/* user identifier string */
	char* 	pSID; 		/* SCCS ID string */
	int	numZones;
	ZONE_OBJ *pZones[5];
        int	rackCenter[2]; /* absolute center location of complete rack */
/* for new rack routines */
#ifdef NEW_TRAY_ROUTINES
        BLOCK_ID  ZoneBlock;
        BLOCK_LIST *blockList;
        int      numZoneInfo;
        ZONE_INFO_OBJ zoneInfo[20];
#endif
} RACK_OBJ;

typedef RACK_OBJ *RACKOBJ_ID;



/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern RACKOBJ_ID rackCreate(char *filename);
extern int rackGetX(RACKOBJ_ID pRackId, int Zone,int sample);
extern int rackGetY(RACKOBJ_ID pRackId, int Zone,int sample);
extern double rackFlow2Zspeed(RACKOBJ_ID pRackId, double Flow, int Zone, int sample);
extern double rackVol2ZTrail(RACKOBJ_ID pRackId, double Vol, int Zone, int sample);
extern int rackSampTop(RACKOBJ_ID pRackId, int Zone, int sample);
extern int rackSampBottom(RACKOBJ_ID pRackId, int Zone, int sample);
extern int rackInvalidZone(RACKOBJ_ID pRackId, int zone);
extern int rackInvalidSample(RACKOBJ_ID pRackId, int zone,int sample);
extern int rackWellOrder(RACKOBJ_ID pRackId, int Order);
extern int rackZoneWellOrder(RACKOBJ_ID pRackId, int Zone,int Order);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif

