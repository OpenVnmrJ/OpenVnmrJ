/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define SELECT          0
#define SET             1
#define EITHER		2

#ifdef FT3D			/* for 3D processing program		*/
#define AUTO_FILE	10
#define INP_COEF_FILE   11
#define EXP_NUM         12
#define OVER_WRITE      13
#define INP_FID_DIR     14
#define LOG_FILE	15
#define INP_INFO_DIR    16
#define OUT_DATA_DIR    17
#define REDUCE		18
#define PROC_MODE	19
#define PROCF3_ACQ	20
#define MULTI_FILE	21
#define FID_MAP		22
#define X_RPC		23
#define GET_PLANE	24
#define FDF_OUTPUT	25
#define IOHOSTADDR	26
#endif

#ifdef PLEXTRACT		/* for 3D plane extraction program	*/
#define EXP_NUM		10
#define OVER_WRITE      11
#define INP_DATA_DIR    12
#define OUT_DATA_DIR	13
#define PLANE_SELECT	14
#endif


struct _ipar
{
   int	pmode;
   int	vset;
   int	ival;
};

typedef struct _ipar ipar;

struct _spar 
{ 
   int  pmode;
   int	vset;
   char	sval[MAXPATHL];
}; 

typedef struct _spar spar;
 
#ifdef FT3D
struct _comInfo
{
   ipar	curexp;
   ipar	reduce;
   ipar overwrite;
   ipar	procf3acq;
   ipar multifile;
   ipar xrpc;
   ipar logfile;
   spar	fiddirpath;
   spar datadirpath;
   spar autofilepath;
   spar coeffilepath;
   spar info3Ddirpath;
   spar fidmapfilepath;
   spar	procmode;
   spar getplane;
   spar fdfheaderpath;
   spar hostAddr;
};
#endif

#ifdef PLEXTRACT
struct _comInfo
{
   ipar	curexp;
   ipar overwrite;
   spar	indirpath;
   spar outdirpath;
   spar planeselect;
};
#endif
 
typedef struct _comInfo comInfo;
