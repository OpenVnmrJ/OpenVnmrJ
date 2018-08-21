/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------*/
/* File mfdata.c:  							*/
/*	Contains routines that implement moving (copying) data from	*/
/*	one fid file to another.  The following commands are 		*/
/*	implemented in this file.					*/
/*	(already implemented w/mf) - mvfid(<source_expno>,<dest_expno>)	*/
/*	- mfopen([[<source_expno>],<dest_expno>])			*/
/*	- mfclose()							*/
/*	- mfblk(<source_expno>,<blk_no>,<dest_expno>,<blk_no>)		*/
/*	- mftrace(<source_expno>,<blk_no>,<trace_no>,			*/
/*				<dest_expno>,<blk_no>,<trace_no>)	*/
/*	- mfdata(<source_expno>,<blk_no>,<start_loc>,			*/
/*			<dest_expno>,<blk_no>,<start_loc>,<np>)		*/

/*  Added calls to reverse fid data to support epi.			*/
/*	- rfblk(<source_expno>,<blk_no>,<dest_expno>,<blk_no>)		*/
/*	- rftrace(<source_expno>,<blk_no>,<trace_no>,			*/
/*				<dest_expno>,<blk_no>,<trace_no>)	*/
/*	- rfdata(<source_expno>,<blk_no>,<start_loc>,			*/
/*			<dest_expno>,<blk_no>,<start_loc>,<np>)		*/

/*  Added calls to table convert fid data to support fast imaging.	*/
/*	- tcfopen(<tablename>)						*/
/*	- tcfblk(<tablename>,<source_expno>,<blk_no>,			*/ 
/*					<dest_expno>,<blk_no>)		*/
/*----------------------------------------------------------------------*/

#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "mfileObj.h"
#include "data.h"
#include "group.h"
#include "tools.h"
#include "vnmrsys.h"
#include "variables.h"
#include "wjunk.h"

extern void Vperror (char *usr_string );
extern int expdir_to_expnum(char *expdir);

#define MOVEFID 	0
#define MOVEBLK 	1
#define MOVETRACE 	2
#define MOVEDATA 	3
#define MOVEOPEN 	4

#define OLD_S_COMPLEX	0x40	/* Complex data status bit for old fid data */

#define TRUE	1
#define FALSE	0

#define OK	0
#define ERROR		1

static  MFILE_ID source_md = NULL, dest_md = NULL;
static  int explicit_open = FALSE;
static int movefid(MFILE_ID source_md, MFILE_ID dest_md);
static int moveblk(MFILE_ID source_md, int source_blk, MFILE_ID *dest_md,
                   int dest_blk, char *dest_fid, int reverseflag);
static int movetrace(MFILE_ID source_md, int source_blk, int source_trace,
	 MFILE_ID dest_md, int dest_blk, int dest_trace, int reverseflag);
static int movedata(MFILE_ID source_md, int source_blk, int source_data,
	MFILE_ID dest_md, int dest_blk, int dest_data, int num_points,
        int reverseflag);
static int access_exp(char exppath[], char estring[]);
static int tabcfblk(MFILE_ID source_md, int source_blk, MFILE_ID *dest_md,
                    int dest_blk, char *dest_fid, int tblsize);

static int fiddataopen(char *source_fid, char *dest_fid);
static int revcpy(char *destaddr,char *sourceaddr,int numbytes,
           int datasize,int datatype);

/******************************************************************************/
int mfdata (int argc, char *argv[], int retc, char *retv[] )
{ /* Begin function */
   /*
   Local Variables:
   ---------------
   source_fid	:  Complete file path to the source fid file .
   dest_fid  	:  Complete file path to the destination fid file .
   source_blk	:
   dest_blk	:
   source_trace :
   dest_trace	:
   source_data  :
   dest_data 	:
   num_points	:
   a_opt	: Defines source exp option 1=arg specified 0=not specfied
   destsize	: Size value to use when opening destination file.
   */
   char   source_fid[MAXPATHL], dest_fid[MAXPATHL], exppath[MAXPATHL];
   int	source_blk,dest_blk;
   int	source_trace,dest_trace;
   int	source_data,dest_data,num_points;
   int	a_opt, action, errstatus, reverseflag, permit;
   char esstring[8],edstring[8];

   /*---------------------------*/
   /* Begin Executable Code:	*/
   /*---------------------------*/
   source_blk = 0;  dest_blk = 0;
   source_trace = 0;  dest_trace = 0;
   source_data = 0;  dest_data = 0;
   num_points = 0;
   /* Verify that the user has passed correct number of arguments */
   a_opt = 0;
   reverseflag = 0;
   if ( !strcmp(argv[0],"mfblk") ) 
   {
	if ((argc < 4) || (argc > 5))
	{ 
	   Werrprintf("Usage: mfblk([<source_expno>,]<blk_no>,<dest_expno>,<blk_no>).");
	   ABORT;
	}
	action = MOVEBLK;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if (argc < 5)
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
     	   strcpy(edstring, argv[2]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
     	   strcpy(edstring, argv[3]);
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  source_blk=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "mfblk: Illegal source block number.");
	   ABORT;
	}
	if (isReal(argv[3+a_opt]))  dest_blk=(int)stringReal(argv[3+a_opt]);
	else {
	   Werrprintf ( "mfblk: Illegal dest block number.");
	   ABORT;
	}
   }
   else if ( !strcmp(argv[0],"mftrace") )
   {
	if ((argc < 6) || (argc > 7))
	{ 
	   Werrprintf ( "Usage: mftrace([<source_expno>,]<blk_no>,<trace_no>,<dest_expno>,<blk_no>,<trace_no>)." );
	   ABORT;
	}
	action = MOVETRACE;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if (argc < 7)
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
     	   strcpy(edstring, argv[3]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
     	   strcpy(edstring, argv[4]);
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  source_blk=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "mftrace: Illegal source block number.");
	   ABORT;
	}
	if (isReal(argv[2+a_opt]))  source_trace=(int)stringReal(argv[2+a_opt]);
	else {
	   Werrprintf ( "mftrace: Illegal source trace number.");
	   ABORT;
	}
	if (isReal(argv[4+a_opt]))  dest_blk=(int)stringReal(argv[4+a_opt]);
	else {
	   Werrprintf ( "mftrace: Illegal dest block number.");
	   ABORT;
	}
	if (isReal(argv[5+a_opt]))  dest_trace=(int)stringReal(argv[5+a_opt]);
	else {
	   Werrprintf ( "mftrace: Illegal dest trace number.");
	   ABORT;
	}
   }
   else if ( !strcmp(argv[0],"mfdata") )
   {
	if ((argc < 7) || (argc > 8))
	{ 
	   Werrprintf ( "Usage: mfdata([<source_expno>,]<blk_no>,<start_loc>,<dest_expno>,<blk_no>,<start_loc>,<num_points>)." );
	   ABORT;
	}
	action = MOVEDATA;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if (argc < 3)
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
     	   strcpy(edstring, argv[3]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
     	   strcpy(edstring, argv[4]);
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  source_blk=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "mfdata: Illegal source block number.");
	   ABORT;
	}
	if (isReal(argv[2+a_opt]))  source_data=(int)stringReal(argv[2+a_opt]);
	else {
	   Werrprintf ( "mfdata: Illegal source data number.");
	   ABORT;
	}
	if (isReal(argv[4+a_opt]))  dest_blk=(int)stringReal(argv[4+a_opt]);
	else {
	   Werrprintf ( "mfdata: Illegal dest block number.");
	   ABORT;
	}
	if (isReal(argv[5+a_opt]))  dest_data=(int)stringReal(argv[5+a_opt]);
	else {
	   Werrprintf ( "mfdata: Illegal dest data number.");
	   ABORT;
	}
	if (isReal(argv[6+a_opt]))  num_points=(int)stringReal(argv[6+a_opt]);
	else {
	   Werrprintf ( "mfdata: Illegal dest data number.");
	   ABORT;
	}
   }
   else if ( !strcmp(argv[0],"mfopen") )
   {
	if ((argc < 1) || (argc > 3))
	{ 
	   Werrprintf ( "Usage: mfopen([[<source_expno>,]<dest_expno>])." );
	   ABORT;
	}
	explicit_open = TRUE;
	action = MOVEOPEN;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if (argc < 2)
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
     	   strcpy(edstring, esstring);
	}
	else if (argc < 3)
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
     	   strcpy(edstring, argv[1]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
     	   strcpy(edstring, argv[2]);
	   a_opt = 1;
	}
   }
   else if (  !strcmp(argv[0],"mfclose") )
   {
	if (argc > 1)
	{ 
	   Werrprintf ( "Usage: mfclose()." );
	   ABORT;
	}

	/* make sure if a file is opened it is written out again */
	if (dest_md->newByteLen < dest_md->byteLen)
		dest_md->newByteLen = dest_md->byteLen;
	mClose(source_md);
	mClose(dest_md);
	source_md = 0;
	dest_md = 0;
	explicit_open = FALSE;
   	/* Normal, successful return */
   	RETURN;
   }
   else
   {
	   Werrprintf ( "mfdata: Incorrect command." );
	   ABORT;
   }

  if ( (atoi(esstring) < 1) || (atoi(esstring) > MAXEXPS) )
  {
     Werrprintf("Illegal source experiment number"); 
     ABORT; 
  }
  if ( (atoi(edstring) < 1) || (atoi(edstring) > MAXEXPS) )
  {
     Werrprintf("illegal destination experiment number"); 
     ABORT; 
  }

  if ( access_exp(exppath, esstring) )	/* If this fails, it complains */
     ABORT;
  if ( access_exp(exppath, edstring) )	/* If this fails, it complains */
     ABORT;

   if (a_opt)
       (void)strcat (source_fid , esstring );
   (void)strcat (source_fid , "/acqfil/fid" );
   (void)strcat (dest_fid , edstring );
   (void)strcat (dest_fid , "/acqfil/fid" );

   errstatus = 0; 		/* initialize to successful */

   /*---------------------------*/
   /* Open the two files 	*/
   /*---------------------------*/

   /* Only open if file pointers are NULL otherwise assume they are 	*/
   /* already open.							*/
   if ((source_md == NULL) && (dest_md == NULL))
   {
      if ( ( source_md = mOpen ( source_fid,0,O_RDONLY) ) == NULL )
      {  Vperror ( "mfdata: mOpen (source fid)" );
	ABORT;
      }

      if (strcmp(source_fid,dest_fid))  /* filenames not equal = TRUE */
      {
	int bytelen;
	struct stat st;
	if (stat(dest_fid,&st) == 0)
	{
	   bytelen = st.st_size;
	}
	else
   	{ 
	   if (errno == ENOENT)		/* no file */
	   {
		bytelen = source_md->byteLen;
	   }
	   else
	   {
		Vperror ( "mfdata: stat (dest fid)" );
	   	if (source_md) 
	   	{
		   mClose(source_md);
		   source_md = NULL;
	   	}
   	   	ABORT;
	   }
   	}
   	permit = O_RDWR | O_CREAT;
   	if ( ( dest_md = mOpen ( dest_fid,bytelen,permit)) 
								== NULL )
   	{  Vperror ( "mfdata: mOpen (dest fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}

      }
      else
      {
   	permit = O_RDWR;
   	if ( ( dest_md = mOpen ( dest_fid,source_md->byteLen,permit)) 
								== NULL )
   	{  Vperror ( "mfdata: mOpen (dest fid,source fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}
      }
   }

   /* Check that both files are open */
   if ((source_md == NULL) || (dest_md == NULL))
   {
      Vperror ( "mfdata: Error in opening files." );
      if (source_md) 
      {
	mClose(source_md);
	source_md = NULL;
      }
      if (dest_md) 
      {
	mClose(dest_md);
	dest_md = NULL;
      }
      ABORT;
   }

   switch (action) {

	case MOVEFID:
	   errstatus = movefid(source_md,dest_md);
	   break;
	case MOVEBLK:
	   /* Since moveblk can append data the dest file may  	*/
	   /* have to be remmapped so the address and the  	*/
	   /* file name of the destination file are passed.	*/
	   errstatus = moveblk(source_md,source_blk,&dest_md,dest_blk,
						dest_fid,reverseflag);
	   break;
	case MOVETRACE:
	   errstatus = movetrace(source_md,source_blk,source_trace,
		     dest_md,dest_blk,dest_trace,reverseflag);
	   break;
	case MOVEDATA:
	   errstatus = movedata(source_md,source_blk,source_data,
		    dest_md,dest_blk,dest_data,num_points,reverseflag);
	   break;
	case MOVEOPEN:
	   break;
	default:
	   Werrprintf( "mfdata: Should not get to this default switch stmt." );
	   errstatus = 1;
	   break;
   }

   if (!explicit_open)
   {
   	mClose(source_md);
   	mClose(dest_md);
	source_md = NULL;
	dest_md = NULL;
   }
   

   /* Normal, successful return */
   return(errstatus);

/* Since this item was defined within this routine, get rid of it now */
#undef P_SETPAR

} /* End function mfdata */


/******************************************************************************/
int rfdata (int argc, char *argv[], int retc, char *retv[] )
{ /* Begin function */
   /*
   Local Variables:
   ---------------
   source_fid	:  Complete file path to the source fid file .
   dest_fid  	:  Complete file path to the destination fid file .
   source_blk	:
   dest_blk	:
   source_trace :
   dest_trace	:
   source_data  :
   dest_data 	:
   num_points	:
   a_opt	: Defines source exp option 1=arg specified 0=not specfied
   reverseflag	: Reverse data points flag.
   destsize	: Size value to use when opening destination file.
   */
   char   source_fid[MAXPATHL], dest_fid[MAXPATHL], exppath[MAXPATHL];
   int	source_blk,dest_blk;
   int	source_trace,dest_trace;
   int	source_data,dest_data,num_points;
   int	a_opt, action, errstatus, reverseflag, permit;
   char esstring[8],edstring[8];

   /*---------------------------*/
   /* Begin Executable Code:	*/
   /*---------------------------*/
   source_blk = 0;  dest_blk = 0;
   source_trace = 0;  dest_trace = 0;
   source_data = 0;  dest_data = 0;
   num_points = 0;
   /* Verify that the user has passed correct number of arguments */
   a_opt = 0;
   reverseflag = 1;
   if ( !strcmp(argv[0],"rfblk") )
   {
	if ((argc < 2) || (argc > 5))
	{ 
	   Werrprintf("Usage: rfblk([<source_expno>,]<blk_no>[,<dest_expno>,<blk_no>]).");
	   ABORT;
	}
	action = MOVEBLK;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if ((argc == 2) || (argc == 4))
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
	   if (argc == 2)
		strcpy(edstring,esstring);
	   else
     	        strcpy(edstring, argv[2]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
	   if (argc < 4)
		strcpy(edstring,esstring);
	   else
     	   	strcpy(edstring, argv[3]);
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  source_blk=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "rfblk: Illegal source block number.");
	   ABORT;
	}
	if (argc > 3)
	{
	   if (isReal(argv[3+a_opt]))  dest_blk=(int)stringReal(argv[3+a_opt]);
	   else {
	      Werrprintf ( "rfblk: Illegal dest block number.");
	      ABORT;
	   }
	}
	else dest_blk = source_blk;
   }
   else if ( !strcmp(argv[0],"rftrace") )
   {
	if ((argc != 3) && (argc != 4) && (argc != 6) && (argc != 7))
	{ 
	   Werrprintf ( "Usage: rftrace([<source_expno>,]<blk_no>,<trace_no>[,<dest_expno>,<blk_no>,<trace_no>])." );
	   ABORT;
	}
	action = MOVETRACE;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if ((argc == 3) || (argc == 6))
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
	   if (argc < 5)
		strcpy(edstring,esstring);
	   else
     	   	strcpy(edstring, argv[3]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
	   if (argc < 5)
		strcpy(edstring,esstring);
	   else
     	   	strcpy(edstring, argv[4]);
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  source_blk=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "rftrace: Illegal source block number.");
	   ABORT;
	}
	if (isReal(argv[2+a_opt]))  source_trace=(int)stringReal(argv[2+a_opt]);
	else {
	   Werrprintf ( "rftrace: Illegal source trace number.");
	   ABORT;
	}
	if (argc > 4)
	{
	   if (isReal(argv[4+a_opt]))  dest_blk=(int)stringReal(argv[4+a_opt]);
	   else {
	      Werrprintf ( "rftrace: Illegal dest block number.");
	      ABORT;
	   }
	   if (isReal(argv[5+a_opt]))  
				dest_trace=(int)stringReal(argv[5+a_opt]);
	   else {
	      Werrprintf ( "rftrace: Illegal dest trace number.");
	      ABORT;
	   }
	}
	else {
	   dest_blk = source_blk;
	   dest_trace = source_trace;
	}
   }
   else if ( !strcmp(argv[0],"rfdata") )
   {
	if ((argc != 4) && (argc != 5) && (argc != 7) && (argc != 8))
	{ 
	   Werrprintf ( "Usage: rfdata([<source_expno>,]<blk_no>,<start_loc>[,<dest_expno>,<blk_no>,<start_loc>],<num_points>)." );
	   ABORT;
	}
	action = MOVEDATA;
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	if ((argc == 4) || (argc == 7))
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
	   if (argc < 6)
		strcpy(edstring,esstring);
	   else
     	   	strcpy(edstring, argv[3]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[1]);
	   if (argc < 6)
		strcpy(edstring,esstring);
	   else
     	   	strcpy(edstring, argv[4]);
	   a_opt = 1;
	}
	if (isReal(argv[1+a_opt]))  source_blk=(int)stringReal(argv[1+a_opt]);
	else {
	   Werrprintf ( "rfdata: Illegal source block number.");
	   ABORT;
	}
	if (isReal(argv[2+a_opt]))  source_data=(int)stringReal(argv[2+a_opt]);
	else {
	   Werrprintf ( "rfdata: Illegal source data number.");
	   ABORT;
	}
	if (argc > 5)
	{
	   if (isReal(argv[4+a_opt]))  dest_blk=(int)stringReal(argv[4+a_opt]);
	   else {
	      Werrprintf ( "rfdata: Illegal dest block number.");
	      ABORT;
	   }
	   if (isReal(argv[5+a_opt]))  dest_data=(int)stringReal(argv[5+a_opt]);
	   else {
	      Werrprintf ( "rfdata: Illegal dest data number.");
	      ABORT;
	   }
	   if (isReal(argv[6+a_opt]))  
				num_points=(int)stringReal(argv[6+a_opt]);
	   else {
	      Werrprintf ( "rfdata: Illegal dest data number.");
	      ABORT;
	   }
	}
	else {
	   dest_blk = source_blk;
	   dest_data = source_data;
	   if (isReal(argv[4+a_opt]))  
				num_points=(int)stringReal(argv[4+a_opt]);
	   else {
	      Werrprintf ( "rfdata: Illegal dest data number.");
	      ABORT;
	   }
	}
   }
   else
   {
	   Werrprintf ( "rfdata: Incorrect command." );
	   ABORT;
   }

  if ( (atoi(esstring) < 1) || (atoi(esstring) > MAXEXPS) )
  {
     Werrprintf("Illegal source experiment number"); 
     ABORT; 
  }
  if ( (atoi(edstring) < 1) || (atoi(edstring) > MAXEXPS) )
  {
     Werrprintf("illegal destination experiment number"); 
     ABORT; 
  }

  if ( access_exp(exppath, esstring) )	/* If this fails, it complains */
     ABORT;
  if ( access_exp(exppath, edstring) )	/* If this fails, it complains */
     ABORT;

   if (a_opt)
       (void)strcat (source_fid , esstring );
   (void)strcat (source_fid , "/acqfil/fid" );
   (void)strcat (dest_fid , edstring );
   (void)strcat (dest_fid , "/acqfil/fid" );

   errstatus = 0; 		/* initialize to successful */
   /*---------------------------*/
   /* Open the two files 	*/
   /*---------------------------*/

   /* Only open if file pointers are NULL otherwise assume they are 	*/
   /* already open.							*/
   if ((source_md == NULL) && (dest_md == NULL))
   {
      if ( ( source_md = mOpen ( source_fid,0,O_RDONLY) ) == NULL )
      {  Vperror ( "rfdata: mOpen (source fid)" );
	ABORT;
      }

      if (strcmp(source_fid,dest_fid))  /* filenames not equal = TRUE */
      {
	int bytelen;
	struct stat st;
	if (stat(dest_fid,&st) == 0)
	{
	   bytelen = st.st_size;
	}
	else
   	{ 
	   if (errno == ENOENT)		/* no file */
	   {
		bytelen = source_md->byteLen;
	   }
	   else
	   {
		Vperror ( "mfdata: stat (dest fid)" );
	   	if (source_md) 
	   	{
		   mClose(source_md);
		   source_md = NULL;
	   	}
   	   	ABORT;
	   }
   	}
   	permit = O_RDWR | O_CREAT;
   	if ( ( dest_md = mOpen ( dest_fid,bytelen,permit)) 
								== NULL )
   	{  Vperror ( "rfdata: mOpen (dest fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}

      }
      else
      {
   	permit = O_RDWR;
   	if ( ( dest_md = mOpen ( dest_fid,source_md->byteLen,permit)) 
								== NULL )
   	{  Vperror ( "rfdata: mOpen (dest fid,source fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}
      }
   }

   /* Check that both files are open */
   if ((source_md == NULL) || (dest_md == NULL))
   {
      Vperror ( "rfdata: Error in opening files." );
      if (source_md) 
      {
	mClose(source_md);
	source_md = NULL;
      }
      if (dest_md) 
      {
	mClose(dest_md);
	dest_md = NULL;
      }
      ABORT;
   }

   switch (action) {

	case MOVEFID:
	   errstatus = movefid(source_md,dest_md);
	   break;
	case MOVEBLK:
	   /* Since moveblk can append data the dest file may  	*/
	   /* have to be remmapped so the address and the  	*/
	   /* file name of the destination file are passed.	*/
	   errstatus = moveblk(source_md,source_blk,&dest_md,dest_blk,
							dest_fid,reverseflag);
	   break;
	case MOVETRACE:
	   errstatus = movetrace(source_md,source_blk,source_trace,
		     dest_md,dest_blk,dest_trace,reverseflag);
	   break;
	case MOVEDATA:
	   errstatus = movedata(source_md,source_blk,source_data,
		    dest_md,dest_blk,dest_data, num_points,reverseflag);
	   break;
	default:
	   Werrprintf( "rfdata: Should not get to this default switch stmt." );
	   errstatus=1;
	   break;
   }

   if (!explicit_open)
   {
   	mClose(source_md);
   	mClose(dest_md);
	source_md = NULL;
	dest_md = NULL;
   }
   

   /* Normal, successful return */
   return(errstatus);

/* Since this item was defined within this routine, get rid of it now */
#undef P_SETPAR

} /* End function rfdata */

static int movefid(MFILE_ID source_md, MFILE_ID dest_md)
{
   struct datafilehead *source_file_head;
   struct datafilehead *dest_file_head;

   if (dest_md->byteLen == 0)
   {
   	memcpy(dest_md->offsetAddr,source_md->offsetAddr,
					source_md->byteLen);
   	dest_md->newByteLen = source_md->byteLen;
   }
   else
   {
   	/* Set source file header */
   	source_file_head = (struct datafilehead *)source_md->mapStrtAddr;

   	/* Set dest file header */
  	 dest_file_head = (struct datafilehead *)dest_md->mapStrtAddr;

	/* For now just copy the source file over the dest file.	*/
	/* Later we may want to append the source file to the dest file.*/

   	memcpy(dest_md->offsetAddr,source_md->offsetAddr,
					source_md->byteLen);
   	dest_md->newByteLen = source_md->byteLen;
   }
   RETURN;
}

/*----------------------------------------------------------------------*/
/* moveblk								*/
/*	Copies specified block from source fid file to destination fid	*/
/*	file.  Blocks are specified starting from 1.  If the 		*/
/*	destination block number is equal to or greater than the number	*/
/*	of blocks in the destination file, the block will be appended 	*/
/*	to the destination file.					*/
/*      If data is appended to the destination file and increases the 	*/
/*	file past its mmaped size, remmap the file			*/
/*----------------------------------------------------------------------*/
static int moveblk(MFILE_ID source_md, int source_blk, MFILE_ID *dest_md,
                   int dest_blk, char *dest_fid, int reverseflag)
{
   struct datafilehead *source_file_head;
   struct datafilehead *dest_file_head;
   struct datablockhead *block_head;
   int destsize,datasize,datatype;
   int blocks, inBytes, outBytes;

   /*-------------------------------------------------------------------*/
   /* Check source_blk,dest_blk are greater than or equal to one.	*/
   /* This is to be consistent with other commands. So their input	*/
   /* range is 1 to n.  Then subtract one from them so their actual 	*/
   /* range is 0 to n-1.						*/
   /*-------------------------------------------------------------------*/
   if ((source_blk < 1) || (dest_blk < 1))
   {
	Werrprintf("mfblk: block numbers range from 1 to n." );
	ABORT;
   }
   source_blk = source_blk - 1;
   dest_blk = dest_blk - 1;

   /* Set source file header */
   source_file_head = (struct datafilehead *)source_md->mapStrtAddr;
   source_md->offsetAddr = source_md->mapStrtAddr;

   /* Set dest file header */
   dest_file_head = (struct datafilehead *)(*dest_md)->mapStrtAddr;
   (*dest_md)->offsetAddr = (*dest_md)->mapStrtAddr;
   if ((*dest_md)->newByteLen < (*dest_md)->byteLen)
	(*dest_md)->newByteLen = (*dest_md)->byteLen;

   blocks = ntohl(dest_file_head->nblocks);

   if ((*dest_md)->newByteLen == 0)
   {
	if (dest_blk == 0)
	{
   	   memcpy((*dest_md)->offsetAddr,source_md->offsetAddr,
					(sizeof(struct datafilehead)));
	   blocks = 0;

	}
	else
	{
	   Werrprintf( "mfblk: Destination file does not exist." );
	   ABORT;
	}
   }
   if (dest_blk >= blocks) /* check for final filesize */
   {
	int permit;
        int bytes;

        bytes = ntohl(dest_file_head->bbytes);
	destsize = sizeof(struct datafilehead) +
		((blocks+1)*bytes) ;
	if ( (*dest_md)->mapLen < destsize)
	{
	    mClose(*dest_md);

   	    permit = O_RDWR;
   	    if ( ( *dest_md = mOpen ( dest_fid,destsize,permit)) == NULL )
   	    {  Vperror ( "mfdata: mOpen (moveblk)" );
   	       ABORT;
   	    }
	    /* Reset destination file header */
	    dest_file_head = (struct datafilehead *)(*dest_md)->mapStrtAddr;
	}
   }


   /* advance pointer locations to start of data */
   source_md->offsetAddr += sizeof(struct datafilehead);
   (*dest_md)->offsetAddr += sizeof(struct datafilehead);


   /* Error checking */
   inBytes = ntohl(source_file_head->bbytes);
   outBytes = ntohl(dest_file_head->bbytes);

   if (inBytes != outBytes)
   {
	Werrprintf("mfblk: Source, Destination block lengths are not equivalent." );
	ABORT;
   }
   blocks = ntohl(source_file_head->nblocks);
   if (source_blk >= blocks)
   {
	Werrprintf("mfblk: Source block out of range." );
	ABORT;
   }
   datasize=4;
   datatype=0;
   if (reverseflag)
   {
	datasize = ntohl(source_file_head->ebytes);
	if (ntohs(source_file_head->vers_id) == 0)
	{
	   if (ntohs(source_file_head->status) & (short)OLD_S_COMPLEX) 
		datatype = COMPLEX;
	}
	else {
	   if (ntohs(source_file_head->status) & (short)S_COMPLEX) 
		datatype = COMPLEX;
	}
   }



   /* advance pointers to correct block positions and set block ptr */
   source_md->offsetAddr += (source_blk*inBytes);
   (*dest_md)->offsetAddr += (dest_blk*outBytes);
   block_head = (struct datablockhead *)(*dest_md)->offsetAddr;

   /* copy block and update index */
   if (reverseflag)
   {
        int numBytes;

        numBytes = ntohl(source_file_head->tbytes) *
                   ntohl(source_file_head->ntraces);
	memcpy((*dest_md)->offsetAddr,source_md->offsetAddr,
					sizeof(struct datablockhead));
	source_md->offsetAddr += sizeof(struct datablockhead);
	(*dest_md)->offsetAddr += sizeof(struct datablockhead);
   	if (revcpy((*dest_md)->offsetAddr,source_md->offsetAddr,
	 	numBytes, datasize,datatype) )
	    ABORT;
   }
   else
	memcpy((*dest_md)->offsetAddr,source_md->offsetAddr,inBytes);

   block_head->index = htons(dest_blk+1);

   blocks = ntohl(dest_file_head->nblocks);
   /* If the block is greater than num of blocks in the destination	*/
   /*  file, the source block will be appended to the dest file.	*/
   if (dest_blk >= blocks)
   {
        blocks++;
	dest_file_head->nblocks = htonl(blocks);
	(*dest_md)->newByteLen = sizeof(struct datafilehead) + 
                            (blocks*outBytes);
   }
   RETURN;
}

/*----------------------------------------------------------------------*/
/* movetrace								*/
/*	Copies specified trace from source fid file to destination fid	*/
/*	file.  Blocks and traces are specified starting from 1.  If the	*/
/*	destination block or trace number is equal to or greater than 	*/
/*	the number of blocks or traces in the destination file; unlike  */
/*	the moveblk routine, an error will be reported.  In other words */
/*	the routine only replaces data, it does not append data.	*/
/*----------------------------------------------------------------------*/
static int movetrace(MFILE_ID source_md, int source_blk, int source_trace,
	 MFILE_ID dest_md, int dest_blk, int dest_trace, int reverseflag)
{
   struct datafilehead *source_file_head;
   struct datafilehead *dest_file_head;
   int datasize,datatype;


   /*-------------------------------------------------------------------*/
   /* Check blocks and traces are greater than or equal to one.		*/
   /* This is to be consistent with other commands. So their input	*/
   /* range is 1 to n.  Then subtract one from them so their actual 	*/
   /* range is 0 to n-1.						*/
   /*-------------------------------------------------------------------*/
   if ((source_blk < 1) || (dest_blk < 1) || 
	(source_trace < 1) || (dest_trace < 1))
   {
	Werrprintf("mfblk: block and trace numbers range from 1 to n." );
	ABORT;
   }
   source_blk = source_blk - 1;
   dest_blk = dest_blk - 1;
   source_trace = source_trace - 1;
   dest_trace = dest_trace - 1;
   datasize=4;
   datatype=0;

   /* Set source file header */
   source_file_head = (struct datafilehead *)source_md->mapStrtAddr;
   source_md->offsetAddr = source_md->mapStrtAddr;

   /* Set dest file header */
   dest_file_head = (struct datafilehead *)dest_md->mapStrtAddr;
   dest_md->offsetAddr = dest_md->mapStrtAddr;

   dest_md->newByteLen = dest_md->byteLen;

   if (dest_md->byteLen == 0)
   {
	Werrprintf( "mfblk: Destination file does not exist." );
	ABORT;
   }
   else
   {
	/* advance pointer locations to start of data */
 	source_md->offsetAddr += sizeof(struct datafilehead);
 	dest_md->offsetAddr += sizeof(struct datafilehead);
   }

   /* Error checking */

   if (source_file_head->tbytes != dest_file_head->tbytes)
   {
	Werrprintf("mfblk: Source, Destination block lengths are not equivalent." );
	ABORT;
   }
   if (source_blk >= ntohl(source_file_head->nblocks))
   {
	Werrprintf("mfblk: Source block out of range." );
	ABORT;
   }
   if (dest_blk >= ntohl(dest_file_head->nblocks))
   {
	Werrprintf("mfblk: destination block out of range." );
	ABORT;
   }
   if (source_trace >= ntohl(source_file_head->ntraces))
   {
	Werrprintf("mfblk: Source trace out of range." );
	ABORT;
   }
   if (dest_trace >= ntohl(dest_file_head->ntraces))
   {
	Werrprintf("mfblk: destination trace out of range." );
	ABORT;
   }
   if (reverseflag)
   {
	datasize = ntohl(source_file_head->ebytes);
	if (ntohs(source_file_head->vers_id) == 0)
	{
	   if (ntohs(source_file_head->status) & (short)OLD_S_COMPLEX) 
		datatype = COMPLEX;
	}
	else {
	   if (ntohs(source_file_head->status) & (short)S_COMPLEX) 
		datatype = COMPLEX;
	}
   }

   /* advance pointers to correct block positions */
   source_md->offsetAddr += (source_blk*ntohl(source_file_head->bbytes));
   dest_md->offsetAddr += (dest_blk*ntohl(dest_file_head->bbytes));

  /* skip block header */
   source_md->offsetAddr += sizeof(struct datablockhead);
   dest_md->offsetAddr += sizeof(struct datablockhead);

   /* advance pointers to correct trace positions */
   source_md->offsetAddr += (source_trace*ntohl(source_file_head->tbytes));
   dest_md->offsetAddr += (dest_trace*ntohl(dest_file_head->tbytes));

   /* copy trace */
   if (reverseflag)
   {
   	if (revcpy(dest_md->offsetAddr,source_md->offsetAddr,
		ntohl(source_file_head->tbytes),datasize,datatype) )
	   ABORT;
   }
   else
	memcpy(dest_md->offsetAddr,source_md->offsetAddr,
					ntohl(source_file_head->tbytes));

   RETURN;
}
/*----------------------------------------------------------------------*/
/* movedata								*/
/*	Copies specified data from source fid file to destination fid	*/
/*	file.  Blocks are specified starting from 1.  Data is 		*/
/*	specified starting from 0.  If the destination block or 	*/
/*	datasize is equal to or greater than the number of blocks or 	*/
/*	trace size in the destination file; unlike the moveblk routine,	*/
/*	an error will be reported.  In other words the routine only 	*/
/*	replaces data, it does not append data.				*/
/*	The source_data and dest_data locations are specified by data	*/
/*	points. (not bytes,not complex points)				*/
/*----------------------------------------------------------------------*/
static int movedata(MFILE_ID source_md, int source_blk, int source_data,
	MFILE_ID dest_md, int dest_blk, int dest_data, int num_points,
        int reverseflag)
{
   int datatype,datasize;
   struct datafilehead *source_file_head;
   struct datafilehead *dest_file_head;
   int bytes;

   /*-------------------------------------------------------------------*/
   /* Check source_blk,dest_blk are greater than or equal to one.	*/
   /* This is to be consistent with other commands. So their input	*/
   /* range is 1 to n.  Then subtract one from them so their actual 	*/
   /* range is 0 to n-1.						*/
   /*-------------------------------------------------------------------*/
   if ((source_blk < 1) || (dest_blk < 1))
   {
	Werrprintf("mfblk: block numbers range from 1 to n." );
	ABORT;
   }
   source_blk = source_blk - 1;
   dest_blk = dest_blk - 1;

   /* Set source file header */
   source_file_head = (struct datafilehead *)source_md->mapStrtAddr;
   source_md->offsetAddr = source_md->mapStrtAddr;

   /* Set dest file header */
   dest_file_head = (struct datafilehead *)dest_md->mapStrtAddr;
   dest_md->offsetAddr = dest_md->mapStrtAddr;

   dest_md->newByteLen = dest_md->byteLen;
   datasize=4;
   datatype=0;

   if (dest_md->byteLen == 0)
   {
	Werrprintf( "mfblk: Destination file does not exist." );
	ABORT;
   }
   else
   {
	/* advance pointer locations to start of data */
 	source_md->offsetAddr += sizeof(struct datafilehead);
 	dest_md->offsetAddr += sizeof(struct datafilehead);
   }

   /* Error checking */

   if (source_blk >= ntohl(source_file_head->nblocks))
   {
	Werrprintf("mfblk: Source block out of range." );
	ABORT;
   }
   if (dest_blk >= ntohl(dest_file_head->nblocks))
   {
	Werrprintf("mfblk: destination block out of range." );
	ABORT;
   }
   if ((source_data+num_points) > 
	  (ntohl(source_file_head->ntraces)*ntohl(source_file_head->np)))
   {
	Werrprintf("mfblk: Source data out of range." );
	ABORT;
   }
   if ((dest_data+num_points) >
          (ntohl(dest_file_head->ntraces)*ntohl(dest_file_head->np)))
   {
	Werrprintf("mfblk: destination data out of range." );
	ABORT;
   }
   if (reverseflag)
   {
	datasize = ntohl(source_file_head->ebytes);
	if (ntohs(source_file_head->vers_id) == 0)
	{
	   if (ntohs(source_file_head->status) & (short)OLD_S_COMPLEX) 
		datatype = COMPLEX;
	}
	else {
	   if (ntohs(source_file_head->status) & (short)S_COMPLEX) 
		datatype = COMPLEX;
	}
   }

   /* advance pointers to correct block positions */
   source_md->offsetAddr += (source_blk*ntohl(source_file_head->bbytes));
   dest_md->offsetAddr += (dest_blk*ntohl(dest_file_head->bbytes));

   /* skip block header */
   source_md->offsetAddr += sizeof(struct datablockhead);
   dest_md->offsetAddr += sizeof(struct datablockhead);

   /* advance pointers to correct trace positions */
   bytes = ntohl(source_file_head->ebytes);
   source_md->offsetAddr += (source_data*bytes);
   dest_md->offsetAddr += (dest_data*ntohl(dest_file_head->ebytes));

   /* copy data */
   if (reverseflag)
   {
   	if (revcpy(dest_md->offsetAddr,source_md->offsetAddr,
		(num_points*bytes),datasize,datatype) )
	    ABORT;
   }
   else
   	memcpy(dest_md->offsetAddr,source_md->offsetAddr,
				(num_points*bytes));

   RETURN;
}

/*--------------------------------------------------------------*/
/* revcpy							*/
/*	Reverses the order of the data as it copies it from 	*/
/*	source to destination.					*/
/*	Returns 0: if successful 1: unsuccessful		*/
/*--------------------------------------------------------------*/
static int revcpy(char *destaddr,char *sourceaddr,int numbytes,
           int datasize,int datatype)
{
   int data2move,i,numshort,numlong;
   void *tmpbuf;
   short *srcshort,*destshort;
   int	*srclong,*destlong;
   /* check for correct numbytes */
   if (datatype == COMPLEX)
	data2move = 2;
   else
	data2move = 1;
   if ( (numbytes%(data2move*datasize)) != 0)
   {
     Werrprintf("revcpy: Inconsistent datalength %d datasize %d datalen %d.",
		 numbytes,datasize,data2move);
     ABORT;
   }
   tmpbuf = (char *)malloc(numbytes);
   if (tmpbuf == NULL)
   {
     Werrprintf("revcpy: Not able to allocate temporary storage.");
     ABORT;
   }

   memcpy(tmpbuf,sourceaddr,numbytes);
   switch (datasize) {
	case 2:
		numshort = numbytes/datasize;
		srcshort = (short *)tmpbuf+(numshort-1);
		destshort = (short *)destaddr;
		for (i=0; i<numshort; i=i+data2move)
		   if (data2move == 1) 
			*destshort++ = *srcshort--;
		   else	/* 2 */
		   {
			*destshort++ = *(srcshort-1);
			*destshort++ = *srcshort--;
			srcshort--;
		   }
		break;
	case 4:
		numlong = numbytes/datasize;
		srclong = (int *)tmpbuf+(numlong-1);
		destlong = (int *)destaddr;
		for (i=0; i<numlong; i=i+data2move)
		   if (data2move == 1) 
			*destlong++ = *srclong--;
		   else	/* 2 */
		   {
			*destlong++ = *(srclong-1);
			*destlong++ = *srclong--;
			srclong--;
		   }
		break;


	default:
     		Werrprintf("revcpy: Unimplemented datasize %d.",datasize);
     		ABORT;
   }
   free(tmpbuf);
   RETURN;
}


/*************************************/
static int access_exp(char exppath[], char estring[])
/*************************************/
{

/* Only verifies the existance of an experiment. */
  strcpy(exppath, userdir);
  strcat(exppath, "/exp");
  strcat(exppath, estring);
  if ( access(exppath, 6) )
  {
     Werrprintf("experiment %s is not accessible", exppath);
     ABORT;
  }
  RETURN;
}
#define TABSCOPEN	1
#define TABSCAPPLY	2

#define NOMINALTABLESIZE	1024
#define MAXTABLESIZE		1024*1

#define getdatatype(status)						\
  	( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX :			\
    	( (status & S_COMPLEX) ? COMPLEX : REAL ) )

static int intcompare(const void *i, const void *j);
static int *index_array = NULL;
static int *table = NULL;
static int tablesize = 0;
static  int tabexplicit_open = FALSE;
/******************************************************************************/
int tcfdata (int argc, char *argv[], int retc, char *retv[] )
{ /* Begin function */
   /*
   Local Variables:
   ---------------
   tabsc_file	:  Complete file path to the source fid file .
   dest_fid  	:  Complete file path to the destination fid file .
   a_opt	: Defines source exp option 1=arg specified 0=not specfied
   destsize	: Size value to use when opening destination file.
   */
   FILE    *table_ptr, *fopen();
   char   tabsc_file[MAXPATHL];
   char   source_fid[MAXPATHL], dest_fid[MAXPATHL], exppath[MAXPATHL];
   char esstring[8],edstring[8];
   int	source_blk,dest_blk;
   int	a_opt, action, status;
   int  i,tempsize,done;


   /*---------------------------*/
   /* Begin Executable Code:	*/
   /*---------------------------*/
   source_blk = 0;  dest_blk = 0;
   /* Verify that the user has passed correct number of arguments */
   a_opt = 0;
   if ( !strcmp(argv[0],"tcfblk") )
   {
	if ((argc < 2) || (argc > 6))
	{ 
	   Werrprintf("Usage: tcfblk(<tabfile>,[<source_expno>,]<blk_no>[,<dest_expno>,<blk_no>]).");
	   ABORT;
	}
	(void)strcpy ( tabsc_file, userdir );
	(void)strcat ( tabsc_file, "/tablib/" );
	(void)strcpy ( dest_fid, userdir );
	(void)strcat ( dest_fid, "/exp" );
	(void)strcat ( tabsc_file, argv[1] );
	if ((argc == 3) || (argc == 5))
	{
	   (void)strcpy ( source_fid, curexpdir );
	   sprintf(esstring, "%d", expdir_to_expnum(curexpdir));
	   if (argc == 3)
		strcpy(edstring,esstring);
	   else
     	        strcpy(edstring, argv[3]);
	}
	else
	{
	   (void)strcpy ( source_fid, userdir );
	   (void)strcat ( source_fid, "/exp" );
     	   strcpy(esstring, argv[2]);
	   if (argc < 5)
		strcpy(edstring,esstring);
	   else
     	   	strcpy(edstring, argv[4]);
	   a_opt = 1;
	}
	if (isReal(argv[2+a_opt]))  source_blk=(int)stringReal(argv[2+a_opt]);
	else {
	   Werrprintf ( "tcfblk: Illegal source block number.");
	   ABORT;
	}
	if (argc > 4)
	{
	   if (isReal(argv[4+a_opt]))  dest_blk=(int)stringReal(argv[4+a_opt]);
	   else {
	      Werrprintf ( "tcfblk: Illegal dest block number.");
	      ABORT;
	   }
	}
	else dest_blk = source_blk;
  	if ( (atoi(esstring) < 1) || (atoi(esstring) > MAXEXPS) )
  	{
  	   Werrprintf("Illegal source experiment number"); 
  	   ABORT; 
  	}
  	if ( (atoi(edstring) < 1) || (atoi(edstring) > MAXEXPS) )
  	{
  	   Werrprintf("illegal destination experiment number"); 
  	   ABORT; 
  	}

  	if ( access_exp(exppath, esstring) ) /* If this fails, it complains */
  	   ABORT;
  	if ( access_exp(exppath, edstring) ) /* If this fails, it complains */
  	   ABORT;

   	if (a_opt)
   	    (void)strcat (source_fid , esstring );
   	(void)strcat (source_fid , "/acqfil/fid" );
   	(void)strcat (dest_fid , edstring );
   	(void)strcat (dest_fid , "/acqfil/fid" );

	action = TABSCAPPLY;

   }
   else if ( !strcmp(argv[0],"tcfopen") )
   {
	if (argc != 2) 
	{ 
	   Werrprintf ( "Usage: tcfopen(<filename>)." );
	   ABORT;
	}
	tabexplicit_open = TRUE;
	(void)strcpy ( tabsc_file, userdir );
	(void)strcat ( tabsc_file, "/tablib/" );
	(void)strcat ( tabsc_file, argv[1] );
 	action = TABSCOPEN;
  }
   else if (  !strcmp(argv[0],"tcfclose") )
   {
	if (argc > 1)
	{ 
	   Werrprintf ( "Usage: tcfclose." );
	   ABORT;
	}
	tabexplicit_open = FALSE;
	free(index_array);
	index_array = NULL;
   	/* Normal, successful return */
   	RETURN;
   }
   else
   {
	   Werrprintf ( "tabsc: Incorrect command." );
	   ABORT;
   }



   /*------------------------------------*/
   /* Open and Read in Table file	 */
   /*------------------------------------*/

    /****
    * Open table file:
    ****/
   if (index_array == NULL)
   {
    if ((table_ptr = fopen(tabsc_file, "r")) == NULL) {
	(void)strcpy ( tabsc_file, systemdir );
	(void)strcat ( tabsc_file, "/tablib/" );
	(void)strcat ( tabsc_file, argv[1] );
    	if ((table_ptr = fopen(tabsc_file, "r")) == NULL) {
           perror("Open");
           printf("tabsc: can't open table file in user or system dir.\n");
           ABORT;   
    	}
    }

    /****
    * Read in the table.  A separate index array is constructed at this
    * time.  This index array is sorted by qsort, using as criteria the
    * values pointed to in the table array by the entries in index.
    ****/
    tempsize = NOMINALTABLESIZE;
    table = (int *)malloc(tempsize*sizeof(int));
    index_array = (int *)malloc(tempsize*sizeof(int));
    while (fgetc(table_ptr) != '=') ;
    i = 0;
    done = 0;
    while ((i<MAXTABLESIZE) && !done)
    {
	while ((i < tempsize) && (fscanf(table_ptr, "%d", &table[i]) == 1)) 
        {
	   index_array[i] = i;
	   i++;
	}
 	if (i == tempsize)
	{
	    tempsize += NOMINALTABLESIZE;
	    table = (int *)realloc(table,tempsize*sizeof(int));
	    index_array = (int *)realloc(index_array,tempsize*sizeof(int)); 
	}
	else done = 1;
    }
    if (i >= MAXTABLESIZE)
    {
        perror("fscanf");
	printf("tcfdata: table length overrun.\n");
	ABORT;   
    }
    tablesize = i;
    if (fclose(table_ptr)) {
        perror("fclose");
	printf("tabc: trouble closing table file.\n");
	ABORT;   
    }
    qsort(index_array, tablesize, sizeof(int), intcompare);
    free(table);
    table = NULL;
   }

    /****
    * At this point the index array has been rearranged so that its
    * entries, in ascending order, hold the index values required to 
    * unscramble the table array, and therefore the data array.
    *
    * Example:           table =  2   0  -1   1  -2
    * index starts as:   index =  0   1   2   3   4
    * After sorting,     index =  4   2   1   3   0
    *
    * Now, we can write out the 4th element of data, followed by the
    * 2nd, 1st, 3rd, and 0th, resulting in data which appears as if it 
    * had been acquired in the order: -2  -1  0  1  2.
    ****/

   status = OK;
   switch (action) {

	case TABSCAPPLY:
	  {
	   /*---------------------------*/
	   /*  Process data blocks.	*/
	   /*---------------------------*/

	   fiddataopen(source_fid,dest_fid);
	   status = tabcfblk(source_md,source_blk,&dest_md,dest_blk,
					dest_fid,tablesize);
	   }
	   break;
	case TABSCOPEN:
	   break;
	default:
	   Werrprintf( "tabsc: Should not get to this default switch stmt." );
	   status = ERROR;
	   break;
   }

   if ((!tabexplicit_open) || (status == ERROR))
   {
	free(index_array);
	index_array = NULL;
	tablesize = 0;
   }
   if (!explicit_open)
   {
   	mClose(source_md);
   	mClose(dest_md);
	source_md = NULL;
	dest_md = NULL;
   }
   

   /* Normal, successful return */
   return(status);


} /* End function tabsc */

/*----------------------------------------------------------------------*/
/* tabcfblk								*/
/*	Performs a table convert of the specified block.		*/
/*	Blocks are specified starting from 1.  If the 			*/
/*	destination block number is equal to or greater than the number	*/
/*	of blocks in the destination file, the block will be appended 	*/
/*	to the destination file.					*/
/*      If data is appended to the destination file and increases the 	*/
/*	file past its mmaped size, remmap the file			*/
/*----------------------------------------------------------------------*/
static int tabcfblk(MFILE_ID source_md, int source_blk, MFILE_ID *dest_md,
                    int dest_blk, char *dest_fid, int tblsize)
{
   struct datafilehead *source_file_head;
   struct datafilehead *dest_file_head;
   struct datablockhead *block_head;
   int destsize,datasize,datatype,malloc_size,trace_size,j;
   char *sdata,*data;
   int blocks;
   int inBytes;

   /*-------------------------------------------------------------------*/
   /* Check source_blk,dest_blk are greater than or equal to one.	*/
   /* This is to be consistent with other commands. So their input	*/
   /* range is 1 to n.  Then subtract one from them so their actual 	*/
   /* range is 0 to n-1.						*/
   /*-------------------------------------------------------------------*/
   if ((source_blk < 1) || (dest_blk < 1))
   {
	Werrprintf("mfblk: block numbers range from 1 to n." );
	ABORT;
   }
   source_blk = source_blk - 1;
   dest_blk = dest_blk - 1;

   /* Set source file header */
   source_file_head = (struct datafilehead *)source_md->mapStrtAddr;
   source_md->offsetAddr = source_md->mapStrtAddr;

   /* Set dest file header */
   dest_file_head = (struct datafilehead *)(*dest_md)->mapStrtAddr;
   (*dest_md)->offsetAddr = (*dest_md)->mapStrtAddr;

   /* check for final filesize */
   blocks = ntohl(dest_file_head->nblocks);
   if (dest_blk >= blocks)
   {
	int permit;
	destsize = sizeof(struct datafilehead) +
		((blocks+1)*ntohl(dest_file_head->bbytes)) ;
	if ( (*dest_md)->byteLen < destsize)
	{
   	    (*dest_md)->newByteLen = (*dest_md)->byteLen;
	    mClose(*dest_md);

   	    permit = O_RDWR;
   	    if ( ( *dest_md = mOpen ( dest_fid,destsize,permit)) == NULL )
   	    {  Vperror ( "mfdata: mOpen (moveblk)" );
   	       ABORT;
   	    }
	}
	/* Reset destination file header */
	dest_file_head = (struct datafilehead *)(*dest_md)->mapStrtAddr;
   }


   (*dest_md)->newByteLen = (*dest_md)->byteLen;

   if ((*dest_md)->byteLen == 0)
   {
	if (dest_blk == 0)
	{
   	   memcpy((*dest_md)->offsetAddr,source_md->offsetAddr,
					(sizeof(struct datafilehead)));
   	   (*dest_md)->newByteLen += sizeof(struct datafilehead);
	   source_md->offsetAddr += sizeof(struct datafilehead);
	   (*dest_md)->offsetAddr += sizeof(struct datafilehead);
	   dest_file_head->nblocks = htonl(1);
           blocks = 1;

	}
	else
	{
	   Werrprintf( "mfblk: Destination file does not exist." );
	   ABORT;
	}
   }
   else
   {
	/* advance pointer locations to start of data */
 	source_md->offsetAddr += sizeof(struct datafilehead);
 	(*dest_md)->offsetAddr += sizeof(struct datafilehead);
   }

   /* Error checking */
   if (source_file_head->bbytes != dest_file_head->bbytes)
   {
	Werrprintf("tcfblk: Source, Destination block lengths are not equivalent." );
	ABORT;
   }
   if (source_blk >= ntohl(source_file_head->nblocks))
   {
	Werrprintf("tcfblk: Source block out of range." );
	ABORT;
   }
   if (tblsize != ntohl(source_file_head->ntraces))
   {
	Werrprintf("tcfblk: tablesize %d not equal to num traces %d.",
			tblsize,ntohl(source_file_head->ntraces) );
	ABORT;
   }
   datasize=4;
   datatype=0;
   datasize = source_file_head->ebytes;
   if (ntohs(source_file_head->vers_id) == 0)
   {
	if (ntohs(source_file_head->status) & (short)OLD_S_COMPLEX) 
		datatype = COMPLEX;
   }
   else {
	if (ntohs(source_file_head->status) & (short)S_COMPLEX) 
		datatype = COMPLEX;
   }



   inBytes = ntohl(source_file_head->bbytes);
   /* advance pointers to correct block positions and set block ptr */
   source_md->offsetAddr += (source_blk*inBytes);
   (*dest_md)->offsetAddr += (dest_blk*ntohl(dest_file_head->bbytes));
   block_head = (struct datablockhead *)(*dest_md)->offsetAddr;

   /* Allocate temp buffer */
   trace_size = ntohl(source_file_head->tbytes);
   malloc_size = trace_size * ntohl(source_file_head->ntraces);
   sdata = (char *) malloc(malloc_size);
   

   memcpy((*dest_md)->offsetAddr,source_md->offsetAddr,
					sizeof(struct datablockhead));
   source_md->offsetAddr += sizeof(struct datablockhead);
   data = source_md->offsetAddr;
   /*----------------------*/
   /*  Sort Traces.  	*/
   /*----------------------*/
   for (j=0; j<tblsize; j++) {
	memcpy(&sdata[j*trace_size],&data[index_array[j]*trace_size],
					trace_size);
   }
   /*------------------------------*/
   /*  Copy sorted data back.  	*/
   /*------------------------------*/
   (*dest_md)->offsetAddr += sizeof(struct datablockhead);
   data = (*dest_md)->offsetAddr;
   memcpy(data,sdata,malloc_size);

   block_head->index = htonl(dest_blk+1);

   /* If the block is greater than num of blocks in the destination	*/
   /*  file, the source block will be appended to the dest file.	*/
   if (dest_blk >= ntohl(dest_file_head->nblocks))
   {
        blocks = ntohl(dest_file_head->nblocks);
        blocks++;
	dest_file_head->nblocks = htonl(blocks);
	(*dest_md)->newByteLen += inBytes;
   }
   RETURN;
}

/*****************************************************************
*  This routine is used by the qsort function to provide the
*  comparison for sorting.  It compares the two entries in
*  "table" array which have indeces defined by the values of the 
*  elements in the "index" array.
*****************************************************************/
static  int intcompare(const void *i, const void *j)
{
    return(table[*((int *)i)] - table[*((int *)j)]);
}



static int fiddataopen(char *source_fid, char *dest_fid)
{
   int permit;
   /*---------------------------*/
   /* Open the two files 	*/
   /*---------------------------*/

   /* Only open if file pointers are NULL otherwise assume they are 	*/
   /* already open.							*/
   if ((source_md == NULL) && (dest_md == NULL))
   {
      if ( ( source_md = mOpen ( source_fid,0,O_RDONLY) ) == NULL )
      {  Vperror ( "fiddataopen: mOpen (source fid)" );
	ABORT;
      }

      if (strcmp(source_fid,dest_fid))  /* filenames not equal = TRUE */
      {
	struct stat st;
	if (stat(dest_fid,&st) != 0)
   	{  Vperror ( "fiddataopen: stat (dest fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}
   	permit = O_RDWR | O_CREAT;
   	if ( ( dest_md = mOpen ( dest_fid,st.st_size,permit)) 
								== NULL )
   	{  Vperror ( "fiddataopen: mOpen (dest fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}

      }
      else
      {
   	permit = O_RDWR;
   	if ( ( dest_md = mOpen ( dest_fid,source_md->byteLen,permit)) 
								== NULL )
   	{  Vperror ( "fiddataopen: mOpen (dest fid,source fid)" );
	   if (source_md) 
	   {
		mClose(source_md);
		source_md = NULL;
	   }
   	   ABORT;
   	}
      }
   }

   /* Check that both files are open */
   if ((source_md == NULL) || (dest_md == NULL))
   {
      Vperror ( "fiddataopen: Error in opening files." );
      if (source_md) 
      {
	mClose(source_md);
	source_md = NULL;
      }
      if (dest_md) 
      {
	mClose(dest_md);
	dest_md = NULL;
      }
      ABORT;
   }
   RETURN;
}
