/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Stubs and other programs for NDC that replace programs in HALfuncs.c  */

/*  CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C  */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data.h"
#include "group.h"

#include "ACQ_SUN.h"

#include "acqcmds.h"

#include "shrstatinfo.h"
#include "shrexpinfo.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "expDoneCodes.h"
#include "expQfuncs.h"

extern char *ipcGetProcName();

#ifndef MAXPATHL
#define  MAXPATHL	128
#endif

#ifndef ACQI_EXPERIMENT
#define  ACQI_EXPERIMENT  "0"
#endif

#ifndef ACQI_PROGRAM_NAME
#define  ACQI_PROGRAM_NAME  "acqi"
#endif

#define ON	1
#define OFF	0

/*  neg_ct_count helps the ACQI program track neg_ct, a field in the
    VM02 stat block which fiddisplay needs to display summed data
    correctly.  It is set to 0 by start_fidexp and decremented (it's
    a negative CT count) each time shrmemToStatblk is called.

    To be sure, this is not the most optimal algorithm, yet it only
    requires that the FID display application call shrmemToStatblk
    AFTER it successfully receives data from the console.  Recall
    also that the INOVA version of ACQI sums the data itself.

    shrmemToStatblk only decrements neg_ct_count if ACQI can acquire;
    for FIDmonitor (ACQI cannot acquire), get the CT count out of the
    block header of the block from which we get data.  See
    recvInteractive.  BEWARE this assumes the application first calls
    recvInteractive and then calls shrmemToStatblk !!			*/

static int		dspGainBits = 0;
static int		neg_ct_count;
static int		signal_avg = 0;
static int		phase_cycle = 0;
static long		blocksize_val = 0;
static long		ctss_val = 0;
static long		oph_val = 0;
static MFILE_ID		ifile = NULL;
static TIMESTAMP	currentTimeStamp;
static FID_STAT_BLOCK	currentFidStatBlk;
static char		rtparsfile[ EXPINFO_STR_SIZE ] = { '\0' };
static unsigned long	rtparsize;


/*  for FID monitor  */

static char		fidFilePath[ EXPINFO_STR_SIZE ];

static SHR_MEM_ID 	ShrExpInfo;
static SHR_EXP_INFO	expInfo;

static struct {
	long	NumTrans;
	long	ctcount;
	long	elemid;
} prevFidStats;

/*  The ctcount and the element ID really are from the previous FID, the
    last one that ACQI/FIDmonitor accessed (and presumably displayed).
    Both are set initially by start_fidmonitor; both are updated by
    isAcqiDataCurrent and both are used by recvInteractData.

    NumTrans is not expected to change from one FID to the next.	*/


/*  Programs to send stuff to the New Digital Console.
    The Unity/UnityPLUS systems used sendtoHAL and DCsendtoHAL.
    The latter was based on the former, with the additional
    feature that it would call disconnect ("DC...") if the
    transfer failed.

    We want to preserve the source code in shimdisplay,
    lockdisplay, etc. which uses sendtoHAL and DCsendtoHAL,
    even though in the NDC there is no HAL.

    At this time ACQI calls upon sendtoHAL and DCsendtoHAL to
    send stuff that on the New Digital Console is sent using
    the "sethw" command.  At this time too we are not
    emulating this "disconnect on failure feature", though it
    may become a requirement later.  This is another reason
    for keeping the two separate interfaces.

    sendtoHAL and DCsendtoHAL become wrapper programs that
    each call sendtoNDC, which actually does the work.		*/

static int
sendtoNDC( dataspace, datalen )
char *dataspace;
int datalen;
{
	short	*sptr;
	char	 NDCcommand[ 1024 ], incremental[ 12 ];
	int	 iter, ival, limit;

	NDCcommand[ 0 ] = '\0';
	sptr = (short *) dataspace;
	limit = *sptr++;
	sprintf( &NDCcommand[ 0 ], "%d", limit );
	for (iter = 0; iter < limit; iter++) {
		sprintf( &incremental[ 0 ], ",%d", *sptr );
		strcat( &NDCcommand[ 0 ], &incremental[ 0 ] );
		sptr++;
	}
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
	send2Acq4Acqi( "sethw", &NDCcommand[ 0 ] );
	return( datalen );				/* Notice it always works */
}

sendtoHAL( dataspace, datalen )
char *dataspace;
int datalen;
{
	return( sendtoNDC( dataspace, datalen ) );
}

DCsendtoHAL( dataspace, datalen )
char *dataspace;
int datalen;
{
	return( sendtoNDC( dataspace, datalen ) );
}

/*  Although the New Digital Console has no HAL device, it is still useful
    for recvfmHAL to zero out the data space it was called with.		*/

recvfmHAL( dataspace, datalen )
char *dataspace;
int datalen;
{
	memset( dataspace, 0, datalen );
	return( datalen );
}


/*  New programs for the new digital console -  these have no equivalent in Unity ACQI  */
/*  CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C   CLASSIC C  */

int
setAcqiInterval()
{
	int	ival;

	ival = send2Acq4Acqi( "statintv", "200" );
	return( ival );
}

int
setDefaultInterval()
{
	int	ival;

	ival = send2Acq4Acqi( "statintv", "" );
	return( ival );
}

int
setAcqiTimeConst()
{
	char		NDCcommand[ 142 ];
	int		ival;

	sprintf( &NDCcommand[ 0 ], "2,%d,0", LKTC );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	ival = send2Acq4Acqi( "sethw", &NDCcommand[ 0 ] );
	return( ival );
}

int
setDefaultTimeConst()
{
	char		NDCcommand[ 142 ];
	int		ival;

	sprintf( &NDCcommand[ 0 ], "2,%d,0", LKACQTC );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	ival = send2Acq4Acqi( "sethw", &NDCcommand[ 0 ] );
	return( ival );
}

int
setAcqiStatus()
{
	char		NDCcommand[ 142 ];
	int		ival;

	sprintf( &NDCcommand[ 0 ], "2,%d,%d", SETSTATUS, ACQ_INTERACTIVE );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	ival = send2Acq4Acqi( "sethw", &NDCcommand[ 0 ] );
	return( ival );
}

int
setDefaultStatus()
{
	char		NDCcommand[ 142 ];
	int		ival;

	sprintf( &NDCcommand[ 0 ], "2,%d,%d", SETSTATUS, ACQ_IDLE );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	ival = send2Acq4Acqi( "sethw", &NDCcommand[ 0 ] );
	return( ival );
}


int
signal_avg_on()
{	return(signal_avg); }

void
set_signal_avg(value)
int value;
{	signal_avg = value; 
}

int
phase_cycle_on()
{	return(phase_cycle); }

void
set_phase_cycle(value)
int value;
{	phase_cycle = value; 
}


#define VT_REPORTED_OFF	30000

shrmemToStatblk( statblk, valid_data )
struct ia_stat *statblk;
int valid_data;
{
	int	iter;
	int	NDCAcqState, NDCVTact;

	NDCAcqState = getStatAcqState();

    /* valid_data is whatever the application wants.  At this time the application
       relies on other schemes involving time stamps to see if the data is valid	*/

	statblk->valid_data = valid_data;

	for (iter = 0; iter < sizeof( statblk->sh_dacs ) / sizeof( short ); iter++)
	  statblk->sh_dacs[ iter ] = getStatShimValue( iter );

	statblk->lk_lvl = getStatLkLevel();
	statblk->LSDV = getStatLSDV();
	statblk->lk_gain = getStatLkGain();
	statblk->lk_power = getStatLkPower();
	statblk->lk_phase = getStatLkPhase();

	statblk->spinspd = getStatSpinAct();

/*  Default value for ADC width or precision.  */

	statblk->adc_size = 16 + dspGainBits;

	if (can_acqi_acquire())
	{
	  if (!signal_avg_on() || (neg_ct_count < 1))
	     neg_ct_count--; 	/* please see comment where neg_ct_count is declared */
	}
        statblk->neg_ct = neg_ct_count;  /* negative ct value for FID display */

	statblk->sh_smplx = (NDCAcqState == ACQ_SHIMMING) ? WORKING : FINISHED;
	statblk->rcvr_gain = getStatRecvGain();
	NDCVTact = getStatVTAct();
	if (NDCVTact == VT_REPORTED_OFF)
	  statblk->VT_temp = 0.0;
	else
	  statblk->VT_temp = NDCVTact;

/*  You may find other statblk fields need to be filled in ...  */

}

int
update_rtparam( rtindex, rtval )
int rtindex;
long rtval;
{
	char	NDCcommand[ 142 ];
	int	rtfd;
	long	lval;

	if (strlen( &rtparsfile[ 0 ] ) < 1) {
		fprintf( stderr,
	   "error: tried to update a real-time variable when the file was not defined\n"
		);
		return( -1 );
	}

	if (rtindex < 0) {
		fprintf( stderr,
	   "attempted to modify a real-time variable using index %d\n", rtindex
		);
		return( -1 );
	}

	rtfd = open( &rtparsfile[ 0 ], O_RDWR );
	if (rtfd < 0) {
		fprintf( stderr,
	   "error: cannot open %s to modify real-time parameters\n", &rtparsfile[ 0 ]
		);
		return( -1 );
	}

	if ((rtindex + new_taboffset()) * sizeof( rtval ) > rtparsize) {
		fprintf( stderr,
	   "attempted to modify index %d when the file has only %d chars\n",
	    rtindex, rtparsize
		);
		close( rtfd );
		return( -1 );
	}

	lval = lseek( rtfd, (long)(rtindex + new_taboffset()) * sizeof(rtval), 
								SEEK_SET );
	write( rtfd, &rtval, sizeof( rtval ) );
	close( rtfd );

/*  Now update the active real-time variable in the console.  */

	sprintf( &NDCcommand[ 0 ], "1;%d,%d,1,%d", FIX_RTVARS, rtindex, rtval );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
	send2Acq4Acqi( "aupdt", &NDCcommand[ 0 ] );

	return( 0 );
}


int
retrieve_rtparam( rtindex, rtval )
int rtindex;
long *rtval;
{
	char	NDCcommand[ 142 ];
	int	rtfd;
	long	lval;

	if (strlen( &rtparsfile[ 0 ] ) < 1) {
		fprintf( stderr,
	   "error: tried to update a real-time variable when the file was not defined\n"
		);
		return( -1 );
	}

	if (rtindex < 0) {
		fprintf( stderr,
	   "attempted to modify a real-time variable using index %d\n", rtindex
		);
		return( -1 );
	}

	rtfd = open( &rtparsfile[ 0 ], O_RDWR );
	if (rtfd < 0) {
		fprintf( stderr,
	   "error: cannot open %s to modify real-time parameters\n", &rtparsfile[ 0 ]
		);
		return( -1 );
	}

	if ((rtindex + new_taboffset()) * sizeof( rtval ) > rtparsize) {
		fprintf( stderr,
	   "attempted to modify index %d when the file has only %d chars\n",
	    rtindex, rtparsize
		);
		close( rtfd );
		return( -1 );
	}

	lval = lseek( rtfd, (long)(rtindex + new_taboffset()) * sizeof(rtval), 
								SEEK_SET );
	read( rtfd, rtval, sizeof( rtval ) );
	close( rtfd );

	return( 0 );
}

static SHR_EXP_INFO
setup_lockentry(systemdir)
char *systemdir;
{
	SHR_EXP_INFO lockentry;

	lockentry = (SHR_EXP_INFO) malloc( sizeof( *lockentry ) );
	if (lockentry == NULL)
	  return( NULL );

	memset( lockentry, 0, sizeof( *lockentry ) );

	lockentry->DataPtSize = sizeof( long );
	lockentry->NumDataPts = 512;
	lockentry->FidSize = lockentry->DataPtSize * lockentry->NumDataPts;

	lockentry->DataSize = sizeof( TIMESTAMP ) + sizeof( FID_STAT_BLOCK ) +
			      lockentry->DataPtSize * lockentry->NumDataPts;
	lockentry->NumFids = 1;		/* Number of Fids (arraydim) */
	lockentry->NumTrans = 1;	/* Number of Transients (NT) */
	strcpy( &lockentry->UsrDirFile[ 0 ], getenv( "vnmruser" ) );
	strcpy( &lockentry->UserName[ 0 ], ipcGetUserName() );

	sprintf( &lockentry->DataFile[ 0 ],
                 "%s/acqqueue/%s.Data", systemdir, ipcGetProcName());
	lockentry->ExpNum = 0;	/* Experiment # to perform processing in */
	lockentry->GoFlag = ACQI_LOCK;
	lockentry->InteractiveFlag = 1;
	lockentry->Codefile[0] = '\0';
	lockentry->RTParmFile[0] = '\0';
	lockentry->TableFile[0] = '\0';
	lockentry->WaveFormFile[0] = '\0';
	lockentry->GradFile[0] = '\0';
        dspGainBits = 0;

	return( lockentry );
}


/*  This program is present in case something goes wrong
    while starting an interactive acquisition (start_lockexp
    or start_fidexp).  The corresponding reserveConsole is
    in IPCmsgqfuncs.c.  In normal operations, ACQUIRE access
    to the console is released when the interactive
    experiment completes and ACQI does nothing; in
    particular, it does not itself release the console.  See
    ExpAcqDone in msgehandler.c, SCCS category expproc.	    */

static
releaseConsole()
{
	int	iter, ival;
	char	NDCcommand[ 122 ], expprocReply[ 256 ];

	strcpy( &NDCcommand[ 0 ], "acquire" );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	ival = talk2Acq4Acqi(
   "releaseConsole", &NDCcommand[ 0 ], &expprocReply[ 0 ], sizeof( expprocReply )
	);

	return( 0 );
}


/*  The New Digital Console starts the lock by preparing a Queue Entry and submitting
    it to Expproc.  We use the High Priority Queue to designate it as an interactive
    experiment.

    In normal operations the Expproc may say NO.  For example, you can be in shim
    display and be refused lock display, since shim display is permitted during a
    non-interactive acquisition whereas lock display is not.  This contrasts with
    the Gemini and HAL based systems, where ACQI could expect to have free reign
    once Acqproc turned over the console to it.						*/

int
start_lockexp()
{
	char		tmpfilename[ MAXPATHL ];
        char		systemdir[ MAXPATHL ];
	char		params4cmd[ 122 ];
	char		expproc_reply[ 256 ];
	char		expinfo[ 16 ];
	int		fd;
	unsigned long	*iaddr;
	SHR_EXP_INFO 	lockentry;

        if (P_getstring(GLOBAL,"systemdir",systemdir,1,MAXPATHL))
          strcpy(systemdir,"/vnmr");
	lockentry = setup_lockentry(systemdir);
	if (lockentry == NULL) {
		fprintf( stderr, "cannot create a lock experiment information entry\n" );
		releaseConsole();
		return( -1 );
	}

	initExpQs( 0 );
	sprintf( tmpfilename, "%s/acqqueue/%s.Info",
		systemdir, ipcGetProcName());

	fd = open( &tmpfilename[ 0 ], O_RDWR | O_CREAT | O_TRUNC, 0666 );
	write( fd, lockentry, sizeof( *lockentry ) );
	close( fd );

	strcpy( &expinfo[ 0 ], ACQI_EXPERIMENT );
	strcat( &expinfo[ 0 ], " " );
	strcat( &expinfo[ 0 ], ipcGetUserName() );

	expQaddToTail( HIGHPRIO, &tmpfilename[ 0 ], &expinfo[ 0 ] );
	params4cmd[ 0 ] = '\0';
	insertAuth( &params4cmd[ 0 ], sizeof( params4cmd ) );
	talk2Acq4Acqi(
    "startInteract", &params4cmd[ 0 ], &expproc_reply[ 0 ], sizeof( expproc_reply )
	);
	if (strcmp( &expproc_reply[ 0 ], "NO" ) == 0) {
		releaseConsole();
		return( -1 );
	}

	ifile = mOpen( &lockentry->DataFile[ 0 ], lockentry->DataSize, O_RDONLY );
	if (ifile == NULL) {
		fprintf( stderr, "can't access data in shared memory\n" );
		releaseConsole();
		return( -1 );
	}

	iaddr = (unsigned long *) ifile->mapStrtAddr;
	currentTimeStamp = *(TIMESTAMP *) iaddr;

	return( 0 );
}

int
start_fidexp( int *datasizeptr, int *lcnpptr )
{
	char		 fidentryfile[ MAXPATHL ];
	char		 tmp2filename[ MAXPATHL ];
        char		 systemdir[ MAXPATHL ];
	char		 params4cmd[ 122 ];
	char		 expproc_reply[ EXPINFO_STR_SIZE ];
	char		 errmsg[200];
	char		 expinfo[ 40 ];
	int		 ival;
	unsigned long	*iaddr;
	struct stat	 rtparstat;
	MFILE_ID	 qfile;
	SHR_EXP_INFO	 fidentry;

	neg_ct_count = 0;			/* no FID data at all present */
	initExpQs( 0 );

        if (P_getstring(GLOBAL,"systemdir",systemdir,1,MAXPATHL))
          strcpy(systemdir,"/vnmr");
	sprintf( fidentryfile, "%s/acqqueue/%s", systemdir, ipcGetProcName());
	sprintf( tmp2filename, "%s.new", fidentryfile);
	if (access(fidentryfile,0) == 0) {
		unlink( fidentryfile );
	}
        if (link(tmp2filename,fidentryfile) != 0) {
		sprintf(errmsg,
	   "write('error','run gf or go(\\'acqi\\') before using the FID button in acqi')\n"
		);
		registerMessageForDisconnect(errmsg);  /* see interactscrn.c */
		releaseConsole();
		/* perror( "link acqi to acqi.new" ); Of no special consequence */
		return( -1 );
	}

	qfile = mOpen( &fidentryfile[ 0 ], sizeof( SHR_EXP_STRUCT ), O_RDONLY );
	if (qfile == NULL) {
		fprintf( stderr, "Cannot open queue file %s\n", &fidentryfile[ 0 ] );
		releaseConsole();
		return( -1 );
	}
	fidentry = (SHR_EXP_INFO) qfile->mapStrtAddr;
	*datasizeptr = fidentry->FidSize;
	*lcnpptr = fidentry->NumDataPts;
        dspGainBits = fidentry->DspGainBits;

	strcpy( &rtparsfile[ 0 ], &fidentry->RTParmFile[ 0 ] );
	ival = stat( &rtparsfile[ 0 ], &rtparstat );
	if (ival != 0) {
		rtparsize = 0;
	}
	else {
		rtparsize = rtparstat.st_size;
		retrieve_rtparam( new_bs_index(), &blocksize_val);
		retrieve_rtparam( new_ctss_index(), &ctss_val);
		retrieve_rtparam( new_oph_index(), &oph_val);
		if (signal_avg_on())
		   update_rtparam( new_nt_index(), get_bs_val() );
		else
		   update_rtparam( new_nt_index(), 1 );

		if (phase_cycle_on())
		   update_rtparam( new_cp_index(), 0 );
		else
		   update_rtparam( new_cp_index(), 1 );
	}

	strcpy( &expinfo[ 0 ], ACQI_EXPERIMENT );
	strcat( &expinfo[ 0 ], " " );
	strcat( &expinfo[ 0 ], ipcGetUserName() );

	expQaddToTail( HIGHPRIO, &fidentryfile[ 0 ], &expinfo[ 0 ] );
	params4cmd[ 0 ] = '\0';
	insertAuth( &params4cmd[ 0 ], sizeof( params4cmd ) );
	talk2Acq4Acqi(
    "startInteract", &params4cmd[ 0 ], &expproc_reply[ 0 ], sizeof( expproc_reply )
	);
	if (strcmp( &expproc_reply[ 0 ], "NO" ) == 0) {
		mClose( qfile );
		releaseConsole();
		return( -1 );
	}

	ifile = mOpen( &fidentry->DataFile[ 0 ], fidentry->DataSize, O_RDONLY );
	if (ifile == NULL) {
		fprintf( stderr, "can't access FID data in shared memory file %s\n",
			&fidentry->DataFile[ 0 ] );
		releaseConsole();
		mClose( qfile );
		return( -1 );
	}

	iaddr = (unsigned long *) ifile->mapStrtAddr;
	currentTimeStamp = *(TIMESTAMP *) iaddr;
	mClose( qfile );

	return( 0 );
}

int
operateFidScope( commandFidScope )
int commandFidScope;
{
	char		NDCcommand[ 122 ];

	if (commandFidScope != STARTFIDSCOPE)
	  commandFidScope = STOPFIDSCOPE;
	sprintf( &NDCcommand[ 0 ], "%d", commandFidScope );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
	send2Acq4Acqi( "transparent", &NDCcommand[ 0 ] );
}

int
startFidScope()
{
	operateFidScope( STARTFIDSCOPE );
}

int
stopFidScope()
{
	operateFidScope( STOPFIDSCOPE );
}

/*  Like most UNIX system calls, returns 0 if OK, -1 if problem, error or failure  */

int
start_fidmonitor( int *datasizeptr, int *lcnpptr )
{
	int		ival;
	char		expInfoFile[ EXPINFO_STR_SIZE ];
	struct stat	fidFileStat;

	ival = talk2Acq4Acqi( "getExpInfoFile", "", &expInfoFile[ 0 ], sizeof( expInfoFile ) );
	if (ival != 0)
	  return( -1 );
	if (strcmp( &expInfoFile[ 0 ], " " ) == 0)
	  return( -1 );

	ShrExpInfo = shrmCreate( &expInfoFile[ 0 ], 1, (unsigned long)sizeof(SHR_EXP_STRUCT) ); 
	if (ShrExpInfo == NULL) {
		fprintf(stderr,"mapIn: shrmCreate() failed in start FID monitor\n");
		return( -1 );
	}

	expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);
	if (expInfo == NULL) {
		fprintf(stderr, "shrmAddr failed after a successful shrmCreate\n" );
		shrmRelease( ShrExpInfo );
		return( -1 );
	}
#ifdef DEBUG_FIDMONITOR
    fprintf( stderr, "data size for the experiment is %d, np is %d\n",
		      expInfo->FidSize, expInfo->NumDataPts );
#endif
	*datasizeptr = expInfo->FidSize;
	*lcnpptr = expInfo->NumDataPts;
	strcpy( &fidFilePath[ 0 ], &expInfo->DataFile[ 0 ] );
	strcat( &fidFilePath[ 0 ], "/fid" );

/*  The programs that receive or read the NMR data append /fid, rather
    than those that prepare the initial experiment information.  This
    is a result of tradition going back at least to the VXR-5000.	*/

	ival = stat( &fidFilePath[ 0 ], &fidFileStat );
#ifdef DEBUG_FIDMONITOR
    fprintf( stderr, "FID file size is %d\n", fidFileStat.st_size );
    fprintf( stderr, "modification time: %d, %s",
		      fidFileStat.st_mtime, ctime( &fidFileStat.st_atime ) );
    fprintf( stderr, "current CT is %d, current block size is %d, current element is %d\n",
		      getStatCT(), expInfo->NumInBS, getStatElem() );
#endif
	ifile = mOpen( &fidFilePath[ 0 ], expInfo->DataSize, O_RDONLY );

	prevFidStats.ctcount = 0;
	prevFidStats.elemid = 1;
	prevFidStats.NumTrans = expInfo->NumTrans;

	return( 0 );
}

int
isAcqiDataCurrent()
{
	if (can_acqi_acquire()) {

		TIMESTAMP	tmpTimeStamp;

		if (ifile != NULL) {
			tmpTimeStamp = *(TIMESTAMP *) ifile->mapStrtAddr;
			if (cmpTimeStamp(
				 (TIMESTAMP *) ifile->mapStrtAddr,
					      &currentTimeStamp
			) > 0) {
				currentTimeStamp = tmpTimeStamp;
				return( 1 );
			}
			else
			  return( 0 );
		}
		else
		  return( 0 );
	}
	else {
		char		*iaddr;
		int		 ival, retval, statElem;
		dfilehead	*fhptr;
		dblockhead	*bhptr;

		retval = 0;
		iaddr = ifile->mapStrtAddr;
		fhptr = (dfilehead *) iaddr;

/*  If on the last time through, the CT count reached NT, move on to the next element  */

		if (prevFidStats.ctcount >= prevFidStats.NumTrans) {
			prevFidStats.ctcount = 0;	/* won't work for IL!! */
			statElem = getStatElem();
			prevFidStats.elemid++;
			if (statElem > prevFidStats.elemid)
			  prevFidStats.elemid = statElem;

		}

/*  Access the Block Header for the current element.  */

		bhptr = (dblockhead *) (iaddr + sizeof( dfilehead ) +
				       (prevFidStats.elemid-1) * fhptr->bbytes);

		if (bhptr->ctcount > prevFidStats.ctcount) {
			prevFidStats.ctcount = bhptr->ctcount;
			retval = 1;
		}

		return( retval );
	}
}

int
isAcquisitionOver()
{
	int	acqState;

	acqState = getStatAcqState();
	if (acqState == ACQ_IDLE || acqState == ACQ_INACTIVE) {
		return( 1 );
	}

	return( 0 );
}


/*  Three programs are present on the subject of errors in the console.
    The first checks for console errors.  At this time it is a yes/no
    test, with yes equivalent to a done code of HARD ERROR.  These are
    (currently) not recoverable.  Once one occurs, the console returns
    no more data until the host restarts the experiment.  From the
    perspective of ACQI this means disconnecting and reconnecting.

    The second stores the error and the third sends a message to VNMR
    describing the error, using the acqstatus command.  At this time
    two programs are present because the FID stat block from shared
    memory will be inaccessable after the disconnect operation
    completes.  You should not send the message before disconnecting
    since the disconnect operation itself sends more commands to VNMR
    to update parameter values and these commands will erase any
    error reported previously.						*/

int
check4ConsoleError()
{
	FID_STAT_BLOCK	*fidstataddr;

	if (ifile == NULL) {
		return( 0 );
	}

	fidstataddr = (FID_STAT_BLOCK *) (ifile->mapStrtAddr + sizeof( TIMESTAMP ));
	if (fidstataddr->doneCode == HARD_ERROR)
	  return( 131071 );

	return( 0 );
}

int
storeConsoleError()
{
	FID_STAT_BLOCK	*fidstataddr;

	if (ifile == NULL) {
		return( -1 );
	}

	fidstataddr = (FID_STAT_BLOCK *) (ifile->mapStrtAddr + sizeof( TIMESTAMP ));
	memcpy( &currentFidStatBlk, fidstataddr, sizeof( currentFidStatBlk ) );
	return( 0 );
}

/*  Note:  This program is always called after disconnect is called.
           See fiddisplay.c, fshimdisplay.c and lockdisplay.  Therefore
           it should NOT use the registerMessageAtDisconnect facility.   */

int
SendConsoleError()
{
	if (currentFidStatBlk.doneCode == HARD_ERROR) {
		char		 acqstatus4Vnmr[ 122 ];

		sprintf( &acqstatus4Vnmr[ 0 ], "acqstatus('%s',%d,%d)\n",
			  ipcGetProcName(),
        		  currentFidStatBlk.doneCode,
        		  currentFidStatBlk.errorCode
		);

		sendasync_vnmr( &acqstatus4Vnmr[ 0 ] );
	}

	return( 0 );
}

/*  Changes in the interface between ACQI, Expproc and the
    console have caused get_locksignal to become a no-op.  */

int
get_locksignal()
{
}

/*  Sending Expproc a getInteract command causes the Expproc to 
    request from the console the data that has accumulated since
    the last getInteract or the start of the current scan,
    whichever occurred later.  This is the FIDscope feature.

    This program is obsolete and is no longer to be used.  Use
    operateFidScope / startFidScope / StopFidScope.  01/1996		*/

int
get_fidscope()
{
	char	params4cmd[ 122 ];

	params4cmd[ 0 ] = '\0';
	insertAuth( &params4cmd[ 0 ], sizeof( params4cmd ) );
	send2Acq4Acqi( "getInteract", &params4cmd[ 0 ] );
}

/*  This program does not actually receive interactive data from the
    console.  It accesses the shared memory where the Expproc puts the
    data it receives from the console.  Previously isAcqiDataCurrent
    found this data to be current, so this program just copies it into
    the data space for FID display.  FID display uses a separate space
    (and not the shared memory directly) so it can modify this data if
    necessary and to help maintain compatibility with other version of
    ACQI.								*/

int
recvInteractData( dbuf, dsize )
char *dbuf;
int dsize;
{
	if (ifile == NULL)
	  return( -1 );

	if (can_acqi_acquire()) {
		char	*interactDataAddr;
		int	 interactDataHdrSize;
		FID_STAT_BLOCK	*interacqHdrAddr;

		interactDataHdrSize = sizeof( TIMESTAMP ) + sizeof( FID_STAT_BLOCK );
		interacqHdrAddr = (FID_STAT_BLOCK *)( ifile->mapStrtAddr + 
							sizeof( TIMESTAMP ));

		interactDataAddr = ifile->mapStrtAddr + interactDataHdrSize;
		if (ifile->byteLen - interactDataHdrSize < dsize) {
			dsize = ifile->byteLen - interactDataHdrSize;
			if (dsize < 0)
			  return( -1 );
			else if (dsize == 0)
			  return( 0 );
		}
		if (signal_avg_on() && (interacqHdrAddr->ct > 0))
		   neg_ct_count = interacqHdrAddr->ct;

/*  A problem appeared when FIDscope was introduced, spikes in the display.
    It was caused by data being simultaneously transferred into and out of
    the shared memory.  Data needs to be transferred atomically as long
    words to prevent this.  See recvInteract, in recvfuncs.c, SCCS category
    recvproc for more information.

    At one time I tried transferring the data here as atomic long words,
    rather than using memcpy.  This has proven (so far) to not be necessary.
    If spikes do ever appear (and after several months of product release
    that should be a remote possibility), try doing the transfer here using
    long word operation.   July 1996  ROL					*/

		memcpy( dbuf, interactDataAddr, dsize );
	}

/*  FIDmonitor:  access to the current experiment data is already present.
    Set neg_ct_count for shrmemToStatblk's use. */

	else {
		int	 	 dataOffset;
		int		 iter;
		char		*iaddr;
		int		*laddr;
		dfilehead	*fhptr;
		dblockhead	*bhptr;

		iaddr = ifile->mapStrtAddr;
		fhptr = (dfilehead *) iaddr;
		bhptr = (dblockhead *) (iaddr + sizeof( dfilehead ) +
			  (prevFidStats.elemid-1) * fhptr->bbytes);
		neg_ct_count = -(bhptr->ctcount);
		dataOffset = (sizeof( dfilehead ) +
			  (prevFidStats.elemid-1) * fhptr->bbytes +
			      sizeof( dblockhead ));
		iaddr += dataOffset;

	   /* If for some reason the file is not large enough,
	      reduce the size of the transfer accordingly.	*/

		if (ifile->byteLen - dataOffset < dsize) {
			dsize = ifile->byteLen - dataOffset;
			if (dsize < 0)
			  return( -1 );
			else if (dsize == 0)
			  return( 0 );
		}
		memcpy( dbuf, iaddr, dsize );

#ifdef DEBUG_FIDMONITOR
		laddr = (int *) iaddr;
		for (iter = 0; iter < 10; iter++)
		 fprintf( stderr, "%d ", *(laddr + iter) );
		fprintf( stderr, "\n" );
#endif
	}
	return( dsize );
}

int
reset_neg_ct()		    /* set the value to -1, since the FID display will start */
{			/* adding data to that already present on the host.  Thus on */
	neg_ct_count = -1;	  /* the first iteration in summed mode, the current */
}			 /* scan will be added to the last scan taken in single mode */

/*  Stops both lock display and FID display  */

int
stop_lockexp()
{
	char	params4cmd[ 122 ], expproc_reply[ 256 ];

	params4cmd[ 0 ] = '\0';
	insertAuth( &params4cmd[ 0 ], sizeof( params4cmd ) );
	talk2Acq4Acqi(
    "stopInteract", &params4cmd[ 0 ], &expproc_reply[ 0 ], sizeof( expproc_reply )
	);
	mClose( ifile );
	ifile = NULL;

/*  Make sure no one tries to update real time parameters  */

	rtparsize = 0;
	rtparsfile[ 0 ] = '\0';

	return( 0 );
}

stop_fidmonitor()
{
	shrmRelease( ShrExpInfo );
	expInfo = NULL;

	mClose( ifile );
	ifile = NULL;
}

/*  Special Note:  The HalFilMark program is only used by ACQI to stop autoshim.  */

HalFileMark()
{
	char	 NDCcommand[ 142 ];
	int	 ival;

	sprintf( &NDCcommand[ 0 ], "1,%d", STOP_SHIMI );
	insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );

	ival = send2Acq4Acqi( "sethw", &NDCcommand[ 0 ] );
	return( ival );
}

/* dummy HAL programs from the days of SCSI */

ResetHalLptr()
{
}

ResetHalCmd()
{
}

int get_bs_val()
{
   return(blocksize_val);
}
int get_ctss_val()
{
   return(ctss_val);
}
int get_oph_val()
{
   return(oph_val);
}
