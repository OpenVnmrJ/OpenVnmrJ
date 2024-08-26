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
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifdef USE_RPC
#include <rpc/types.h>
#include <rpc/rpc.h>
#include "acqinfo.h"
#endif

#include "ACQPROC_strucs.h"
#include "STAT_DEFS.h"

#define ERROR 1
#define MAXRETRY 8
#define SLEEP 4
#define REGISTERPORT 2
#define UNREGPORT 3
#define DISCONNECTSIZ 7		/* disconnect window size */
#define CONNECTSIZ 14		/* connect window size */
#define DATAGRAM 0		/* DataGram type socket  */
#define STREAM 1		/* Stream type socket  */

int	acq_ok;
int	debug;

#define HOSTLEN 122
char User[HOSTLEN];
char LocalHost[HOSTLEN];
char RemoteHost[HOSTLEN];
int  Procpid;

/*  local_addr_list and local_addr are required to support local_entry.
    You must create a new copy of the h_addr_list and the h_addr fields
    (see netdb.h); else when you call gethostbyname again, these will
    be replaced, even if you make a separate copy of the hostent itself.  */

struct hostent	 local_entry;
static char	*local_addr_list[ 2 ] = { NULL, NULL };
static int	 local_addr;

static int statussocket = -1;
static struct sockaddr_in	statussockname,msgsockname;

static int  Acqpid;    /* acquisitions process ID number for async usage */
static int  Acqrdport; /* acquisition's read stream socket port */
static int  Acqwtport; /* acquisition's write stream socket port */
static int  Acqmsgport; /* acquisition's async message socket port */
static char AcqHost[HOSTLEN]; /* acquisition's machine name */


#ifdef USE_RPC
static CLIENT *client = NULL;  /* RPC client handle */

/*-------------------------------------------------------------
|  initrpctcp()/1 - initialize client handle for 
|		    Remote Procedure Call (RPC) using TCP protocal
|                       Author:  Greg Brissey 9/12/89
+-------------------------------------------------------------*/
initrpctcp(hostname)
char *hostname;
{
	char			 errmsg[256];
	struct sockaddr_in	 server_addr;
	int			 socket = RPC_ANYSOCK;
	enum clnt_stat		 clnt_stat;
	struct hostent		*hp;

	if ( (hp = gethostbyname(hostname)) == NULL) {
		/*sprintf(errmsg,"can't obtain information on host computer %s",hostname);
		perror(errmsg);*/
	   /* testing showed that perror never gives a useful error here, so we
	      just report the failure and leave it to the user to figure it out.  */

		fprintf(stderr,"can't obtain information on host computer %s\n",hostname);
		return(-1);
	}

	/*bcopy(hp->h_addr, (caddr_t) &server_addr.sin_addr,hp->h_length);*/
	memcpy( (caddr_t) &server_addr.sin_addr, hp->h_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = 0;
	if ((client = clnttcp_create(
	   &server_addr,
	    ACQINFOPROG,
	    ACQINFOVERS,
           &socket,
	    BUFSIZ,
	    BUFSIZ)) == NULL) {
		sprintf(errmsg,"Remote Host: %s, acqinfo service request ",hostname);
		clnt_pcreateerror(errmsg);
		return(-1);
	}

	return(0);
}

/*-------------------------------------------------------------
|  killrpctcp()/1 - destroy client handle for 
|		    Remote Procedure Call (RPC) using TCP protocal
|                       Author:  Greg Brissey 9/12/89
+--------------------------------------------------------------*/
killrpctcp()
{
	clnt_destroy(client);
}

/*-------------------------------------------------------------
|  callrpctcp()/8 - Remote Procedure Call (RPC) using TCP protocal
|                       Author:  Greg Brissey 8/15/89
| 9/12/89 - split this routine into 2 additional routines
|	    initrpctcp() & killrpctcp().    GMB
+-------------------------------------------------------------*/
callrpctcp(host,prognum,versnum,procnum,inproc,in,outproc,out)
char *host, *in, *out;
xdrproc_t inproc,outproc;
{
	char		errmsg[256];
	enum clnt_stat	clnt_stat;
	struct timeval	total_timeout;

	total_timeout.tv_sec = 20;
	total_timeout.tv_usec = 0;
	clnt_stat = clnt_call(client,procnum,inproc,in,outproc,out,total_timeout);
	if (clnt_stat  != RPC_SUCCESS) {
		sprintf(errmsg,"Remote Host: %s, acqinfo service request ",host);
		clnt_perror(client,errmsg);
		/* clnt_destroy(client); */
		return(-1);
	}

	return( (int) clnt_stat);
}

/*-------------------------------------------------------------
|  getinfo()/1 - use RPC to regisitered server on remote host
|	         to obtain the IPC socket information
|
|		 Returns -  if the pid is running.
|			Author:  Greg Brissey 8/15/89
+-------------------------------------------------------------*/
getinfo(hostname)
char *hostname;
{
	static acqdata	info;
	int		svc_debug;

	svc_debug = debug;
	if (callrpctcp(
	    hostname,
	    ACQINFOPROG,
	    ACQINFOVERS,
	    ACQINFO_GET,
	    xdr_int,
	   &svc_debug,
	    xdr_acqdata,
	   &info) != 0) {
		fprintf(stderr,"error: callrpctcp, cannot obtain acqinfo, aborting.\n");
		return(0);
	}

/*
	strcpy(AcqHost,info.host);
*/
	strcpy(AcqHost, hostname);
	Acqpid = info.pid;
	Acqrdport=info.rdport;
	Acqwtport=info.wtport;
	Acqmsgport=info.msgport;

	if (debug) {
		fprintf(stderr,"host: '%s'\n",AcqHost);
		fprintf(stderr,"Pid: %d, rdPort: %d, wtPort: %d, msgPort: %d\n",
			info.pid,info.rdport,info.wtport,info.msgport);
	}

	return(info.pid_active);
}
#endif // USE_RPC

/*--------------------------------------------------------------------*/

main( argc, argv )
int argc;
char *argv[];
{
	int		 ival;
	struct passwd	*getpwuid();
	struct passwd	*pasinfo;
	struct hostent	*local_hp;

	acq_ok = 0;
	debug = 0;

    /* get process id */

	Procpid = getpid();
	if (debug)
	  fprintf(stderr,"Process ID: %d\n",Procpid);
 
    /* get Host machine name */

	gethostname(LocalHost,sizeof(LocalHost));
	if (debug)
          fprintf(stderr,"Local Host Name: %s\n",LocalHost);

	local_hp = gethostbyname( LocalHost );
	if (local_hp->h_length > sizeof( int )) {
		fprintf(stderr, "programming error, size of host address is %d, expected %d\n",
				 local_hp->h_length, sizeof( int )
		);
		exit( 1 );
	}
	memcpy( &local_entry, local_hp, sizeof( local_entry ) );
	memcpy( &local_addr, local_entry.h_addr, local_entry.h_length );
	local_entry.h_addr_list = &local_addr_list[ 0 ];
	local_entry.h_addr = (char *) &local_addr;
 
/*  WARNING:  any address fields in local_entry not explicitly set
              may be reset the next time you call gethostbyname.	*/

#ifdef USE_RPC
	if (argc > 1) {
		strcpy( RemoteHost, argv[ 1 ] );
		if (initrpctcp( RemoteHost ) < 0)
		  exit( 1 );
	}
	else
#endif
	  strcpy(RemoteHost,LocalHost);

    /* --- get user's name --- */
    /*        get the password file information for user */

	pasinfo = getpwuid((int) getuid());
	strcpy(User,pasinfo->pw_name); /* Store user name */
	if (debug)
	  fprintf(stderr,"User Name: %s\n",User);

	acq_ok = initIPCinfo(RemoteHost);
	if (acq_ok) {
		if (debug)
		  fprintf(stderr,"DoTheChores initializing socket\n");
		initsocket();
		acqregister();	/* register status port with acquisition */
		DoTheChores();	/* receive status information, print it out */
		unregister();	/* unregister status port with acquisition */

		exit( 0 );
	}
	else {
		fprintf( stderr, "Acqproc does not appear to be active on %s\n", RemoteHost );
		exit( 1 );
	}
}

DoTheChores()
{
	int	ret;

	if (debug)
	  fprintf(stderr,"DoTheChores acq_ok = %d\n",acq_ok);
	if (acq_ok) {
		AcqStatBlock statbuf;	/* acq update status buffer */

		ret = readacqstatblock(&statbuf);
		if (ret < 0) {
			printf( "could not get Acqstat information\n" );
		}
		else {
			if (debug)
			  fprintf(stderr,"DoTheChores updating screen");

			updatestatscrn(&statbuf);
		}
	}
	else {
		fprintf( stderr, "IPC info not initialized\n" );
	}
}

CreateSocket(type)
int type;
{
	int on = 1;
	int sd;	/* socket descriptor */
	int flags;

	if (type == STREAM) {
		sd = socket(AF_INET,SOCK_STREAM,0);  /* create a stream socket */
	}
	else {
		sd = socket(AF_INET,SOCK_DGRAM,0);  /* create a datagram socket */
	}
	if (sd == -1) {
		perror("CreateSocket(): socket error");
		return(-1);
	}

    /* --- set up the socket options --- */

#ifndef LINUX
	setsockopt(sd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif
	/*setsockopt(sd,SOL_SOCKET,(~SO_LINGER),&on,sizeof(on));*/

    /* --- We explicitly setup the descriptor as desired because ----*/
    /*     the notifier remembers the old settings for a file descriptor
    /* --- even if it was closed, so we take no chances ---- */

        if ((flags = fcntl(sd,F_GETFL,0)) == -1) {	/* get mode bits */
		perror("CreateSocket():  fcntl error, can't get flags");
		return(-1);
	}    

	flags &=  ~FNDELAY;
	if (fcntl(sd,F_SETFL,flags) == -1) {	/* set to blocking */
		perror("CreateSocket(): fcntl error ");
		return(-1);
	}

	return(sd);
}

initsocket()    
{
	struct hostent *hp;

    /* --- name of the socket so that we may connect to them --- */
    /* read in acquisition host and port numbers to use */

	hp = gethostbyname(AcqHost);
	if (hp == NULL) {
		fprintf(stderr,"Acqproc():, Unknown Host\n");
		exit(1); 
	}
 
	memset((char *) &msgsockname,0,sizeof(msgsockname));  /* clear socket info */
 
    /*bcopy(hp->h_addr,(char *)&msgsockname.sin_addr,hp->h_length);*/

	memcpy((char *)&msgsockname.sin_addr, hp->h_addr, hp->h_length);
	msgsockname.sin_family = hp->h_addrtype;
	msgsockname.sin_port = Acqmsgport;
}

initIPCinfo(remotehost)
char *remotehost;
{
	char filepath[128];
	char *tmpptr;
	FILE *stream;
	extern char *getenv();
 
#ifdef USE_RPC
	if ( (remotehost != NULL) &&
	     (remotehost[0] != '\0') &&
	     (strcmp(remotehost,LocalHost) != 0) )
	  return(getinfo(remotehost));
#endif

	tmpptr = (char *)getenv("vnmrsystem");            /* vnmrsystem */
	strcpy(filepath,tmpptr);
	strcat(filepath,"/acqqueue/acqinfo");
	if (stream = fopen(filepath,"r")) {
		if (fscanf(stream,"%d%s%d%d%d",
			  &Acqpid,AcqHost,&Acqrdport,&Acqwtport,&Acqmsgport
		) != 5) {
			fclose(stream);
			return(0);
		}
		else {
			if (debug) {
				fprintf(stderr,"Acq Hostname: '%s'\n",AcqHost);
				fprintf(stderr,"Acq PID = %d, Read Write Port = %d, %d,Msge %d\n",
					Acqpid,Acqrdport,Acqwtport,Acqmsgport);
			}
			fclose(stream);
			return(Ping_Pid());
		}
	}
	else
	  return(0);
}

int
Ping_Pid()
{
	int ret;

	ret = kill(Acqpid,0);           /*  check if Acqpid is active */
	if ((ret == -1) && (errno == ESRCH)) {
		errno = 0;          /* reset errno */
		if (debug)
		  fprintf( stderr, "Ping PID failed for PID = %d\n", Acqpid );
		return(0);
	}
	else {
		if (debug)
		  fprintf( stderr, "Ping PID succeeded for PID = %d\n", Acqpid );
		return(1);
	}
}

acqregister()
{
	char	message[100];
	int	localaddr, namlen;
 
	statussockname.sin_family = AF_INET;
	statussockname.sin_addr.s_addr = INADDR_ANY;
    /*statussockname.sin_port = IPPORT_RESERVED + 15;*/
	statussockname.sin_port = 0;
    /* --- create the connectless socket for Acq status display --- */
	if (statussocket != -1)
	  close(statussocket);
	statussocket = CreateSocket(DATAGRAM);

	if (statussocket == -1) {
		perror("Register(): error");
		exit(1);
	}

	if (bind(statussocket,(caddr_t)&statussockname,sizeof(statussockname)) != 0)    {
		perror("Register(): bind error");
		exit(0);
	}
	namlen = sizeof(statussockname);
	getsockname(statussocket,&statussockname,&namlen);
	if (debug)
	  fprintf(stderr,"Status Port Number: %d\n",statussockname.sin_port);
 
 
	if (local_entry.h_length > sizeof( int )) {
		fprintf( stderr, "Error: length of host address is %d, expected %d\n",
				  local_entry.h_length, sizeof( int )
		);
		exit(0);
	}
	else
	  memcpy( (caddr_t) &localaddr, local_entry.h_addr, local_entry.h_length);

    /* ---- register this status socket with the acquisition process --- */

	sprintf(message,"%d,%s,%d,%d,%s,%d,%d,%d,%d,",
			 REGISTERPORT,LocalHost,
			 statussockname.sin_port,Procpid,User,Procpid,
			 localaddr,local_entry.h_addrtype,local_entry.h_length);
	if (sendacq(Acqpid,message))
	  fprintf(stderr,"register message unable to be sent\n");
}

unregister()
{
	char message[100];
 
    /* ---- register this status socket with the acquisition process --- */

	sprintf(message,"%d,%s,%d,%d,",UNREGPORT,LocalHost,statussockname.sin_port,
			Procpid);
	if (sendacq(Acqpid,message))
	  fprintf(stderr,"unregister message unable to be sent\n");
}
/*------------------------------------------------------------
|
|    sendacq()/2
|       connect to Acquisition's Async Process's Socket
|       then transmit a message to it and disconnect.
|
+-----------------------------------------------------------*/
sendacq(acqpid,message)
char *message;
int acqpid;
{
	int fgsd;   /* socket discriptor */
	int result;
	int i;
	int on = 1;
 
    /* --- try several times then fail --- */

	for (i=0; i < MAXRETRY; i++) {
		fgsd = socket(AF_INET,SOCK_STREAM,0);        /* create a socket */

		if (fgsd == -1) {
			perror("sendacq(): socket");
			fprintf(stderr,"sendacq(): Error, could not create socket\n");
			exit(0);
		}
		if (debug)
		  fprintf(stderr,
			 "sendacq(): socket create for async trans %d\n",fgsd
		);

#ifndef LINUX
		setsockopt(fgsd,SOL_SOCKET,SO_USELOOPBACK,&on,sizeof(on));
#endif

		if (debug) {
			fprintf(stderr,"sendacq(): socket process group: %d\n",acqpid);
			fprintf(stderr,"sendacq(): socket created fd=%d\n",fgsd);
		}
 
       /* --- attempt to connect to the named socket --- */

		if (debug)
		  printf("sendacq(): send signal that pipe connection is requested.\n");
		if ((result = connect(fgsd,&msgsockname,sizeof(msgsockname))) != 0) {
          /* --- Is Socket queue full ? --- */
			if (errno != ECONNREFUSED && errno != ETIMEDOUT) { /* NO, some other error */
				perror("sendacq():aborting,  connect error");
				fprintf(stderr,"sendacq(): errno: %d ECONN %d\n",
						errno,ECONNREFUSED);
				exit(ERROR);
			}
		}             /* Yes, try MAXRETRY times */
		else {     /* connection established */
			break;
		}

		fprintf(stderr,"sendacq(): Socket queue full, will retry %d\n",i);
		close(fgsd);
		sleep(SLEEP);
	}
	if (result != 0) {   /* tried MAXRETRY without success  */
		fprintf(stderr,"Sendacq(): Max trys Exceeded, aborting send\n");
		exit(ERROR);
	}
	if (debug)
	  fprintf(stderr,"Sendacq(): Connection Established \n");
	write(fgsd,message,strlen(message));

	shutdown(fgsd,2);
	close(fgsd);
	return(0);
}

readacqstatblock(statblock)
AcqStatBlock *statblock;
{
	int bytes;

	if (debug)
	  fprintf(stderr,"readacqstatblock(): \n");

	bytes = read(statussocket,statblock,sizeof(AcqStatBlock));
	if (debug)
	  fprintf(stderr,"returned %d \n",bytes);

	return(bytes);
}

/* defined this program before it is used - prevent compiler
   confusion over the value returned by this program.		*/

static char *
makeStateString( acqstate )
int acqstate;
{
	char *tmpaddr, *retaddr;
 
	switch(acqstate) {
          case ACQ_REBOOT:
                        tmpaddr ="Rebooted";
                        break;
          case ACQ_IDLE:
                        tmpaddr ="Idle";
                        break;
          case ACQ_ACQUIRE:
                        tmpaddr="Acquiring";
                        break;
          case ACQ_PARSE:
                        tmpaddr="Active";
                        break;
          case ACQ_PREP:
                        tmpaddr="Working";
                        break;
          case ACQ_SYNCED:
                        tmpaddr="Ready";
                        break;
          case ACQ_VTWAIT:
                        tmpaddr="VT Regulation";
                        break;
          case ACQ_SPINWAIT:
                        tmpaddr="Spin Regulation";
                        break;
          case ACQ_AGAIN:
                        tmpaddr="Auto Set Gain";
                        break;
          case ACQ_ALOCK:
                        tmpaddr="Auto Locking";
                        break;
          case ACQ_AFINDRES:
                        tmpaddr="Lock: Find Res.";
                        break;
          case ACQ_APOWER:
                        tmpaddr="Lock: Adj. Power";
                        break;
          case ACQ_APHASE:
                        tmpaddr="Lock: Adj. Phase";
                        break;
          case ACQ_FINDZ0:
                        tmpaddr="Find Z0";
                        break;
          case ACQ_SHIMMING:
                        tmpaddr="Shimming";
                        break;
          case ACQ_SMPCHANGE:
                        tmpaddr="Changing Sample";
                        break;
          case ACQ_RETRIEVSMP:
                        tmpaddr="Retrieving Sample";
                        break;
          case ACQ_LOADSMP:
                        tmpaddr="Loading Sample";
                        break;
          case ACQ_ACCESSSMP:
                        tmpaddr="Sample: Access open";
                        break;
          case ACQ_ESTOPSMP:
                        tmpaddr="Sample: ESTOP";
                        break;
          case ACQ_MMSMP:
                        tmpaddr="Sample: Magnet motion";
                        break;
          case ACQ_HOMESMP:
                        tmpaddr="Sample: Initializing";
                        break;
	  case ACQ_INTERACTIVE:
                        tmpaddr="Interactive";
                        break;
	/*
          case ACQ_FID:
                        tmpaddr="FID Display";
                        break;
       */
          case ACQ_TUNING:
                        tmpaddr="Tuning";
          case ACQ_PROBETUNE:
                        tmpaddr="Probe Tuning";
                        break;
	  default:
                        tmpaddr="Inactive";
			break;
	}

	retaddr = (char *) malloc( strlen( tmpaddr ) + 1 );
	if (retaddr == NULL)
	  return( NULL );

	strcpy( retaddr, tmpaddr );

/*  It is upto the calling program to free
    the memory allocated by this program.	*/

	return( retaddr );
}    

/*  i32Log10 serves to count the number of digits in a positive integer  */

static int
i32Log10( ivalue )
long ivalue;
{
	if (ivalue < 1)
	  return( 0 );
	if (ivalue < 10)
	  return( 1 );
	if (ivalue < 100)
	  return( 2 );
	if (ivalue < 1000)
	  return( 3 );
	if (ivalue < 10000)
	  return( 4 );
	if (ivalue < 100000)
	  return( 5 );
	if (ivalue < 1000000)
	  return( 6 );
	if (ivalue < 10000000)
	  return( 7 );
	if (ivalue < 100000000)
	  return( 8 );
	if (ivalue < 1000000000)
	  return( 9 );
	else
	  return( 10 );
}

#define ABS_TIME_ACQSTAT_INDEX	11
#define ABS_TIME_ACQSTAT_LENGTH  8

static char *
formatAbsTimeAcqstat( absTime )
int absTime;
{
	int	 localAbsTime;
	char	*tmpaddr, *retaddr;

/* If Acqproc sends back an absolute time of 0 or less,
   format it as the 0-length string.			*/

	if (absTime <= 0) {
		retaddr = (char *) malloc( 1 );
		if (retaddr == NULL)
		  return( NULL );
		*retaddr = '\0';
		return( retaddr );
	}

	localAbsTime = absTime;
	tmpaddr = (char *) ctime( &localAbsTime );
	retaddr = (char *) malloc( ABS_TIME_ACQSTAT_LENGTH + 1 );
	if (retaddr == NULL)
	  return( NULL );
	strncpy( retaddr, tmpaddr + ABS_TIME_ACQSTAT_INDEX, ABS_TIME_ACQSTAT_LENGTH );
	*(retaddr + ABS_TIME_ACQSTAT_LENGTH) = '\0';

	return( retaddr );
}

#define SECS_PER_HOUR	3600
#define SECS_PER_MIN	60
#define LENGTH_MIN_SECS	5
#define MIN_HOUR_LEN	2

static char *
formatDeltaTimeAcqstat( deltaTime )
int deltaTime;					/* in seconds */
{
	int	 hours, mins, secs;
	int	 hlen;
	char	*retaddr;

	if (deltaTime <= 0) {
		retaddr = (char *) malloc( 1 );
		if (retaddr == NULL)
		  return( NULL );
		*retaddr = '\0';
		return( retaddr );
	}

	hours = deltaTime / SECS_PER_HOUR;
	if (hours > 0) {
		hlen = i32Log10( hours );
		if (hlen < MIN_HOUR_LEN)
		  hlen = MIN_HOUR_LEN;
		deltaTime = deltaTime - hours * SECS_PER_HOUR;
	}
	else {
		hours = 0;			/* confirm the value */
		hlen = 2;			/* to get 00:mm:ss */
	}

	mins = deltaTime / SECS_PER_MIN;
	secs = deltaTime % SECS_PER_MIN;

	retaddr = (char *) malloc( hlen + 1 + LENGTH_MIN_SECS + 1 );
	if (retaddr == NULL)
	  return( NULL );

/*  First arg to sprintf specifies the field width
    (actually precision) for the hour value.		*/

	sprintf( retaddr, "%0.*d:%02d:%02d", hlen, hours, mins, secs );
	return( retaddr );
}

static int
cvtLockLevelToPercent( lockLevel )
int lockLevel;
{
	int	lockPercent;

	if (lockLevel > 1300) {
		lockPercent = (int) (1300.0 / 16.0 + (lockLevel - 1300) / 15.0);
		if (lockPercent > 100 ) lockPercent = 100;
	}
	else
	  lockPercent = lockLevel / 16;

	return( lockPercent );
}

#define  VT_NOT_VALID	30000

/*  We convert the VT value into a Character String because a VT value
    of 30000 (base 10) implies the VT is not turned on or for some other
    reason the value is not valid.  In that case we want to return the
    0-length string instead of a numeric value.				*/

static char *
getVTvalue( vt_val )
int vt_val;
{
	int	 tmplen;
	char	*retaddr, tmpbuf[ 20 ];

	if (vt_val == VT_NOT_VALID) {
		retaddr = (char *) malloc( 1 );
		if (retaddr == NULL)
		  return( NULL);
		*retaddr = '\0';
	}
	else {
		sprintf( &tmpbuf[ 0 ], "%.1f", (double) vt_val / 10.0 );
		tmplen = strlen( &tmpbuf[ 0 ] );
		retaddr = (char *) malloc( tmplen + 1 );
		if (retaddr == NULL)
		  return( NULL);
		strcpy( retaddr, &tmpbuf[ 0 ] );
	}

	return( retaddr );
}

#define OFF 0
#define REGULATED 1
#define NOTREG 2
#define NOTPRESENT 3

static char *
getmode( statval )
int statval;
{
	int	 msgelen;
	char	*msgeptr, *retaddr;

	switch( statval ) {
	  case OFF:
		msgeptr = "Off";
		break;

	  case REGULATED:
		msgeptr = "Regulated";
		break;

	  case NOTREG:
		msgeptr = "Not Reg.";
		break;

	  case NOTPRESENT:
		msgeptr = "Not Present.";
		break;

	  default:
		msgeptr = "";
		break;
	}

	msgelen = strlen( msgeptr );
	retaddr = (char *) malloc( msgelen + 1 );
	if (retaddr == NULL)
	  return( NULL );

	strcpy( retaddr, msgeptr );
	return( retaddr );
}

#define  BASIC_MASK	0x03
#define  LOCK_SHIFT	2
#define  SPIN_SHIFT	4
#define  DEC_SHIFT	9
#define  VT_SHIFT	11

static char *
getLockFromLSDV( lsdv_val )
int lsdv_val;
{
	int	lock_val;

	lock_val = ((lsdv_val >> LOCK_SHIFT) & BASIC_MASK);
	return( getmode( lock_val ) );
}

static char *
getSpinFromLSDV( lsdv_val )
int lsdv_val;
{
	int	spin_val;

	spin_val = ((lsdv_val >> SPIN_SHIFT) & BASIC_MASK);
	return( getmode( spin_val ) );
}

static char *
getDecFromLSDV( lsdv_val )
int lsdv_val;
{
	int	 dec_val, msgelen;
	char	*message, *retaddr;

	dec_val = ((lsdv_val >> DEC_SHIFT) & BASIC_MASK);
        switch( dec_val ) {
	  case 0:
		message = "Off";
		break;
	  case 1:
		message = "On";
		break;
	  case 2:
		message = "Gated";
		break;
	  default:
		message = "";
		break;
	}

	msgelen = strlen( message );
	retaddr = (char *) malloc( msgelen + 1 );
	if (retaddr == NULL)
	  return( NULL );

	strcpy( retaddr, message );
	return( retaddr );
}

static char *
getVTFromLSDV( lsdv_val )
int lsdv_val;
{
	int	vt_val;

	vt_val = ((lsdv_val >> VT_SHIFT) & BASIC_MASK);
	return( getmode( vt_val ) );
}

static char *
getAirFromLSDV( lsdv_val )
int lsdv_val;
{
	int	 air_val, msgelen;
	char	*message, *retaddr;

	air_val = (lsdv_val >> 6) & 0x0001;
        switch( air_val ) {
	  case 0:
		message = "Eject";
		break;
	  case 1:
		message = "Insert";
		break;
	  default:
		message = "";
		break;
	}

	msgelen = strlen( message );
	retaddr = (char *) malloc( msgelen + 1 );
	if (retaddr == NULL)
	  return( NULL );

	strcpy( retaddr, message );
	return( retaddr );
}

updatestatscrn(statbuf)
AcqStatBlock *statbuf;
{
	char	*state, *VTactual, *VTsetting;
	char	*completeTime, *dataStoreTime, *remainTime;
	char	*lockFromLSDV, *spinFromLSDV, *VTFromLSDV, *decFromLSDV;
	char	*airFromLSDV;

/*  Make adjustments based on conditions.  */

	if ( strlen( &statbuf->AcqUserID[ 0 ] ) < 1) {
        	statbuf->AcqFidElem = 0L;
        	statbuf->AcqCT = 0L;
	}

/*  Get special values */

	state = makeStateString( statbuf->Acqstate );
	completeTime = formatAbsTimeAcqstat( statbuf->AcqCmpltTime );
	remainTime = formatDeltaTimeAcqstat( statbuf->AcqRemTime );
	dataStoreTime = formatAbsTimeAcqstat( statbuf->AcqDataTime );
	VTactual = getVTvalue( statbuf->AcqVTAct );
	VTsetting = getVTvalue( statbuf->AcqVTSet );

	printf( "acq state: %s\n", state );
	printf( "sample number: %d\n", statbuf->AcqSample );
	printf( "User: %s\n", &statbuf->AcqUserID[ 0 ] );
	printf( "Exp ID: %s\n", &statbuf->AcqExpID[ 0 ] );
	printf( "FID element: %d\n", statbuf->AcqFidElem );
	printf( "CT: %d\n", statbuf->AcqCT );
	printf( "Completion time: %s\n", completeTime );
	printf( "remaining time: %s\n", remainTime );
	printf( "data stored at time: %s\n", dataStoreTime );
	printf( "acq exp in queue: %d\n", statbuf->AcqExpInQue );
	printf(
	   "lock level: %d\n", cvtLockLevelToPercent( statbuf->AcqLockLevel )
	);
	printf( "spinner set at: %d\n", statbuf->AcqSpinSet );
	printf( "spinner currently at: %d\n", statbuf->AcqSpinAct );
    /* Only output MAS items if ones seems to exist. */
    if(statbuf->AcqSpinSpeedLimit > 0) {
        printf( "MAS spinner bearing Span: %d Adj: %d Max: %d\n", 
                statbuf->AcqSpinSpan, statbuf->AcqSpinAdj, statbuf->AcqSpinMax);
        printf( "MAS spinner Module ID: %s\n", &statbuf->probeId1[ 0 ]);
        printf( "MAS spinner Speed Limit: %d\n", statbuf->AcqSpinSpeedLimit);
    }
        printf( "Gradient Coil ID: %s\n", &statbuf->gradCoilId[ 0 ]);
	printf( "VT set: %s\n", VTsetting );
	printf( "VT active: %s\n", VTactual );
      /*printf( "su flag: %d\n", statbuf->AcqSuFlag );
	printf( "LDSV: 0x%04x\n", statbuf->AcqLSDV );
	printf( "\n" );					Dan Iverson says No */

/*  This output extracted from the LSDV word  */

	lockFromLSDV = getLockFromLSDV( statbuf->AcqLSDV );
	spinFromLSDV = getSpinFromLSDV( statbuf->AcqLSDV );
	VTFromLSDV = getVTFromLSDV( statbuf->AcqLSDV );
	decFromLSDV = getDecFromLSDV( statbuf->AcqLSDV );
	airFromLSDV = getAirFromLSDV( statbuf->AcqLSDV );
	printf( "Lock: %s\n", lockFromLSDV );
	printf( "Spin: %s\n", spinFromLSDV );
	printf( "Air: %s\n", airFromLSDV );
	printf( "VT: %s\n", VTFromLSDV );
	printf( "Decoupler: %s\n", decFromLSDV );

	free( state );
	free( completeTime );
	free( remainTime );
	free( dataStoreTime );

	free( lockFromLSDV );
	free( spinFromLSDV );
	free( VTFromLSDV );
	free( decFromLSDV );
	free( airFromLSDV );
}
