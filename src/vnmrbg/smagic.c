/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-------------------------------------------------------------------------
|
|       vnmr  main routine
|
|	This module contains the main() procedure of the
|	vnmr program. This main procedure figures out whether
|	we are running on a sun or a graphon terminal and
|	creates the appropriate windows.
|	This module also contains code to handle mouse
|	execution on the sun, creating the special cursor
|	on the sun, and sets up environmental variables.
|
|	NOTE WELL THAT ON THE SUN CONSOLE THIS IS THE BASE ROUTINE OF
|	THE CHILD AND IS NOT CALLED BY MASTER
|
|	Important note:  Prior to this version, the child owned the
|	numbered buttons.  This was not acceptable, as the master had
|	no way of knowing synchronously that the child was preoccupied
|	with a Button event and thus Acquisition messages should be
|	queued until the child is no longer busy.
|
|	Thus the code to create the panel and the numbered buttons
|	has been removed from this module and placed in master.c
|
/+-----------------------------------------------------------------------*/
#include "vnmrsys.h"
#include "locksys.h"
#include "allocate.h"
#include "wjunk.h"
#include <stdio.h>
#include <stdlib.h>
#if defined (__INTERIX) || (SOLARIS)
#define _REENTRANT
#include <unistd.h>
#endif
#include <string.h>
#include <sys/time.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <signal.h>
// #include <X11/Xlib.h>
#include "acquisition.h"
#include "smagic.h"

/* for openSocket() etc. */
#include <sys/socket.h>

#include <arpa/inet.h>
#ifdef __INTERIX
typedef int socklen_t;
#endif

#include <netinet/tcp.h>
#include "sockets.h"

/* for socket() etc. */
#include <unistd.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <netdb.h>

/* for fcntl() */
#include <sys/types.h>
#include <fcntl.h>

#include <sys/fcntl.h>

#ifdef SOLARIS
#include <sys/filio.h>
#endif 

#include "group.h"
/* #include "vnmrsys.h" */
#include "variables.h"
#include <errno.h>
#include "params.h"
#include "vfilesys.h"
#include "comm.h"
#include "aipCFuncs.h"
#include "tools.h"
#include "pvars.h"
#include "buttons.h"

#ifdef UNIX
#include <sys/file.h>           /*  bits for `access', e.g. F_OK */
#else 
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif 

/*for Linux only,  it's in signals.h of Solaris*/
#ifndef  MAXSIG
#define MAXSIG  46
#endif

#ifndef TRUE
#define FALSE (0)
#define TRUE (!FALSE)
#endif 

#define JEVAL "jFunc(9,"
#define JEVALLEN 8

char         datadir[MAXPATH]; 		/* path to data in automation mode */
char        *initCommand = NULL;        /* initial command */
char        *initArg = NULL;
char         serverMac[MAXPATH];
char        *graphics;			/* Type of graphical display */
char         psgaddr[MAXSTR];

char         vnMode[22];	/* can be set to background, acq, etc */
char        *vnmruser = NULL;		
char	    *Jx_string; /* pointer to list of changed variable names for VNMRJ */
char	     JtopPanelParam[MAXPATH] = ""; /* name of parameter to update VNMRJ panels */

extern int   ignoreEOL;
/* extern int   textTypeIsTcl; */
extern int   resizeBorderWidth;
extern int   Eflag;
extern int   Rflag;
extern int   debug;
extern int   MflagVJcmd;
extern char  systemdir[];
extern char  userdir[];
extern char  textFrameFds[];
extern char  mainFrameFds[];
extern char  buttonWinFds[];
extern char  Xserver[];
extern char  Xdisplay[];
extern char  fontName[];
extern char  HostName[];

extern int   VnmrJPortId;
extern int   VnmrJViewId;
extern char  VnmrJHostName[];
extern char  Jvbgname[];
extern int   isMaster;
extern int   jInput;
extern int   forgroundPid;
extern int   acqflag;
extern int        doThisCmdHistory;
int          JgetPid;
extern int autoDelExp;
extern int nextexp_d(int *expi, char *expn );
extern int cexpCmd(int argc, char *argv[], int retc, char *retv[]);

extern varInfo *findVar(char *name);
/* static void clearVar(); */
extern void  showMacros();
extern void  pushTree();
extern void  popTree();
extern void  clearVar(char *name);
extern int   aipOwnsScreen();
extern void  dontIgnoreBomb();
extern void  jset_overlayType(int);
extern void  jset_csi_overlayType(int);
extern void  jset_overlaySpecInfo(char *, int, char *, int, int, int, double,
               double, double, double, double, double, double, double,char *,char *);
extern void  jset_csi_overlaySpecInfo(char *, int, char *, int, int, int, double,
               double, double, double, double, double, double, double);
extern int   sendTripleEscToMaster(char code, char *string_to_send );
extern int   loadAndExec(char *buffer);
extern int   rightsEval(char * rightsName);
extern void setupdirs(char *cptr );
extern void stuffCommand(char *s);
extern int  arraytests();
extern int nmrExit(int argc, char *argv[], int retc, char *retv[]);
extern void checkAcqStart(char *s);
extern void jvnmr_sync(int n, int aip);
extern void set_win2_size(int x, int y, int w, int h);
extern void init_colors();
extern void make_table();
extern void processJMouse(int argc, char *argv[]);
extern void process_csi_JMouse(int argc, char *argv[]);
extern void set_jmouse_mask(int argc, char *argv[]);
extern void set_jmouse_mask2(int argc, char *argv[]);
extern void set_csi_jmouse_mask(int argc, char *argv[]);
extern void set_csi_jmouse_mask2(int argc, char *argv[]);
#ifdef MOTIF
extern void jSetGraphicsWinId2(long num );
extern void set_input_func();
#endif
extern void repaintCanvas();
extern void redrawCanvas2();
extern void set_font_size(int w, int h, int a, int d);
extern void set_frame_font_size(int id, int w, int h, int a, int d);
extern void set_vj_font_array(char *name, int n, int w, int h, int a, int d);
extern void set_csi_font_size(int w, int h, int a, int d);
extern void repaint_canvas(int csi);
extern void set_csi_mode(int on, char *orient);
extern void set_csi_display(int on);
extern void vj_set_transparency_level(int type, double level);
extern void set_transparency_level(double level);
extern void set_pen_width(int thick);
extern void set_line_width(int thick);
extern void set_spectrum_width(int thick);
extern void set_csi_opened(int n);
extern void window_redisplay();
extern void copy_from_pixmap(int x, int y, int x2, int y2, int w, int h);
extern void set_jframe_size(int id, int x, int y, int w, int h);
extern void set_jframe_loc(int id, int x, int y, int w, int h);
extern void set_pframe_size(int id, int x, int y, int w, int h);
extern void set_csi_jframe_size(int id, int x, int y, int w, int h);
extern void set_csi_jframe_loc(int id, int x, int y, int w, int h);
extern void set_csi_pframe_size(int id, int x, int y, int w, int h);
extern void toggle_graphics_debug(int a, int b);
extern void set_jframe_vertical(int id, int vertical);
extern void jvnmr_init();
extern void set_active_frame(int id);
extern void set_csi_active_frame(int id);
extern void set_jframe_status(int id, int s);
extern void set_csi_jframe_status(int id, int s);
extern int jscroll_wheel_mouse(int clicks, int csi);
extern void printjimage(int args, char *argv[]);
extern void jsendArrayMenu();
extern void jMouse(int button, int event, int x, int y);
extern void csi_jMouse(int button, int event, int x, int y);
extern void iplan_pnewUpdate(char* str);
extern void removeJCatchers(int fd );
extern void sendChildNewLineNoClear();
extern void AbortAcq();
extern void cancelCmd();
extern void release_canvas_event_lock();
extern void terminal_main_loop();
extern void smagicLoop();
extern void set_bootup_gfcn_ptrs();
extern void setUpWin(int windowRetain, int noUi);
extern void jgraphics_init();
extern void nmr_exit(char *modeptr);
extern void bootup(char *modeptr, int enumber);
extern void setWissun(int val);
extern int merge_command_table(void *aipCmd);
extern void *aipGetCommandTable();
extern void set_win_region(int order, int x, int y, int x2, int y2);
extern void set_win_size(int x, int y, int w, int h);
extern void set_main_win_size(int x, int y, int w, int h);
extern void set_csi_win_size(int x, int y, int w, int h);
extern void jSetGraphicsWinId(char *idStr, int useX );
extern void jSuspendGraphics(int s);
extern void jSetGraphicsShow(int show);
extern void readyJCatchers(int fd );
extern void set_hourglass_cursor();
extern void restore_original_cursor();
extern void exec_aip_menu(int n);
extern void exec_vj_menu(int n);
extern void clear_aip_menu();
extern int open_color_palette_from_vj(int type, int n, char *name);
extern int change_aip_image_order(int id, int order);
extern int select_aip_image(int id, int order, int mapId);
extern int set_aip_image_info(int id, int order,int mapId, int transparency, char *name, int selected);
extern void show_spec_raw_data(int n);
extern void set_ybar_style(int n);
extern int jTable_changed(int argc, char *argv[]);
extern char *getParentMacro();
extern int XParseGeometry ( const char *string, int *x, int *y, unsigned int *width, unsigned int *height);


int          Bnmr;
int          Bserver;
int          noUI; // no GUI, no Graphics
int          noGUI;
int          noGraph=0; // flag to turn on/off fraphics update 
int          Aflag;
int          Dflag;
int          Gflag;
int          Lflag;
int	     masterWinHeight = 1;
int	     masterWinWidth = 1;
int          textWinHeight = 1;
int	     mode_of_vnmr;
int	     moreFd = 0;
int          Pflag;
int          Tflag;
int          Vflag;
int          Wretain;
int          inputfd;
int          outputfd;
int 	     interuption = 0;
int 	     working     = 0;
int	     jShowInput  = 0;
int	     jShowArray  = 0;
int	     jParent  = 0;

static int  expnum;
static int  graphics_window = 0;
static int  annotate = 0;
static int  inPort = 0;  /* the input socket port of master */
static int  inPrintMode = 0;  /* print screen  */
static int  cmdLineOK = 1;  /* command line is enabled */
static char escChar = '\033';
static char *vjWinInfo = NULL;
static char evalStr[ MAXPATH ];
static char showStr[ MAXPATH ];
static char tmpStr[ MAXPATH ];
static char jexprDummyVar[ MAXPATH ] = "$VALUE";

static FILE   *xtfd = NULL;


COMM_INFO_STRUCT vnmrj_comm_addr;

#define NEWLINE         10
#define CR         13
#define VNMRJ_NUM_VIEWPORTS 4
#define NUM_INDIRECT_PARS 3

#ifndef  DEFAULT_SOCKET_BUFFER_SIZE
#define  DEFAULT_SOCKET_BUFFER_SIZE	32768
#endif

#define SIGINTX1     SIGHUP
#define SIGINTX2     SIGCONT
#define SIGINTX3     (MAXSIG-1)

static int jtesttest = 0;
static int open_terminal(int, char *);
static void removeJeventEntry(char *keyName, int len);
int returnEval(int argc, char *argv[], int retc, char *retv[]);
void insertAcqMsgEntry(char *acqmptr );
static int messageSendToVnmrJ( Socket *pSD, const char* message, int messagesize);
static int setUpEnv(char *modeptr);

static char graphOffMacro[MAXSTR] = "";

static void Wtesttest(char *key )
{
  jtesttest = atoi( key );
}

#ifdef XXX
/* compare characters of beginning of string 1 with whole of string 2 */
int begstrcmp(char *longstr, char *staticstr)
{
  int i, len, ret;
  len = (int) strlen(staticstr);
  if (len > (int) strlen(longstr))
    ret = 2;
  else
  {
    ret = 0;
    for (i=0; i<len; i++)
      if (longstr[i] != staticstr[i])
        ret = 1;
  }
  return( ret );
}
#endif

/*-------------------------------------------------------------------------
|
|     Main routine of magic
|
/+-----------------------------------------------------------------------*/

void read_jcmd()
{
    int n;
    char  td[512];

    while (1) {
       if ( (n = read(inputfd,td,510)) > 0) {
	   if (td[n-1] != '\n') {
	       td[n] = '\n';
	       td[n+1] = '\0';
	   }
	   else
	       td[n] = '\0';
           break; 
       }
    }
    if (n > 0)
       execString(td);
}

static void stop_draw2screen(int sig)
{
   (void) sig;
   jSetGraphicsShow(0);
}

static void start_draw2screen(int sig)
{
   (void) sig;
   jSetGraphicsShow(1);
}

static void start_resize(int sig)
{
   (void) sig;
   jSuspendGraphics(1);
}

static void
catch_sigint()
{
    struct sigaction	intserv;
    sigset_t		qmask;
    void		gotInterupt();
    extern void		send_hourglass_cursor();
 
    /* --- set up signal handler --- */

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    intserv.sa_handler = gotInterupt;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
    sigaction( SIGINT, &intserv, NULL );

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGALRM );
    intserv.sa_handler = send_hourglass_cursor;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
    sigaction(SIGALRM, &intserv, NULL);

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINTX1 );
    intserv.sa_handler = stop_draw2screen;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
    sigaction(SIGINTX1, &intserv, NULL);

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINTX2 );
    intserv.sa_handler = start_draw2screen;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
    sigaction(SIGINTX2, &intserv, NULL);

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINTX3 );
    intserv.sa_handler = start_resize;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
    sigaction(SIGINTX3, &intserv, NULL);
}

static char cancelLabel[MAXSTR];
static int  doInteruption;

void setCancel(int doit, char *str)
{
   doInteruption = doit;
   strcpy(cancelLabel,str);
}

void gotInterupt()
{  if (working)
      if (interuption == 0)
      {  if (strlen(cancelLabel))
            Werrprintf(cancelLabel);
         else
            Werrprintf("command canceled");
         if (doInteruption == 1)
         {
            interuption = 1;
            dontIgnoreBomb();
         }
         if (doInteruption == 0)
         {
            if ( strcmp(cancelLabel," "))
            {
               strcpy(cancelLabel,"");
            }
            doInteruption = 1;
         }
      }
      else
	 Werrprintf("already canceled");
   else
      Werrprintf("nothing to cancel");

   catch_sigint();
}

static void get_vnmrj_addr(char *addr )
{
   CommPort ptr = &vnmrj_comm_addr;
   sprintf(addr, "%s %d %d", ptr->host,htons(ptr->port),ptr->pid);
}

int get_vnmrj_port()
{
   return(vnmrj_comm_addr.port);
}

int get_vnmrj_socket()
{
   return(vnmrj_comm_addr.msgesocket);
}

/* initialize server socket to receive vnmrj commands */
int init_vnmrj_comm(char *hostname, int port_num)
{
   int buff = DEFAULT_SOCKET_BUFFER_SIZE;
   CommPort ptr = &vnmrj_comm_addr;
   int on = 1;
   socklen_t meslength;

   strcpy(ptr->host,hostname);
   ptr->pid = getpid();
   ptr->msg_uid = getuid();
   seteuid( ptr->msg_uid );
   ptr->port = port_num;

 /*===================================================================*/
 /*   ---  create the asynchronous message socket for PSG,VNMR  ---   */
 /*===================================================================*/

    strcpy(ptr->path,"");
    ptr->msgesocket = socket(AF_INET,SOCK_STREAM,0);  /* create a socket */
    if (ptr->msgesocket < 0) /* really should loop if it fails */
    {
        fprintf(stderr,"  INITSOCKET(): socket error\n");
    }
    else
    {
        /* if on same machine, do not send out ethernet */
#ifndef LINUX
        setsockopt(ptr->msgesocket,SOL_SOCKET,SO_USELOOPBACK,(char*) &on,sizeof(on));
#endif
        /* close socket immediately if close request */
        setsockopt(ptr->msgesocket,SOL_SOCKET,(~SO_LINGER),(char *)&on,sizeof(on));
        /* set send buffer size and receive buffer size */
        setsockopt(ptr->msgesocket,SOL_SOCKET, SO_SNDBUF, (char *) &(buff), sizeof(buff));
        setsockopt(ptr->msgesocket,SOL_SOCKET, SO_RCVBUF, (char *) &(buff), sizeof(buff));

        /* --- bind a name to the socket so that others may connect to it --- */

        /* Use IPPORT_RESERVED +4 for port until we setup better way */
        /* Find first available port */
        ptr->messname.sin_family = AF_INET;
        // ptr->port = ptr->messname.sin_port = 0; /* set to 0 for port search */
        // ptr->port = ptr->messname.sin_port = port_num;
        ptr->messname.sin_port = htons(port_num);
        ptr->messname.sin_addr.s_addr = INADDR_ANY;
        /* name socket */
        if (bind(ptr->msgesocket,(struct sockaddr*) &(ptr->messname),sizeof(ptr->messname)) != 0)
        {
            fprintf(stderr,"  INITSOCKET(): msgesocket bind error\n");
            ptr->msgesocket = -1;
            ptr->port = -1;
		/* really should loop if it fails?? */
        }
        else
        {
            meslength = sizeof(ptr->messname);
            getsockname(ptr->msgesocket,(struct sockaddr*) &ptr->messname,&meslength);
            ptr->port = ptr->messname.sin_port;
            listen(ptr->msgesocket,6);        /* set up listening queue ?? */
        }
    }
    return(ptr->port);
}


void openVSocket(int fd)
{
    struct sockaddr from;
    socklen_t fromlen = sizeof(from);
    int  flags, new_fd;

    if (debug)
        fprintf(stderr,"  open server socket %d  \n", fd);
    if (fd <= 0)
        return;
    new_fd = accept(fd, &from, &fromlen);
    if (debug)
        fprintf(stderr,"   new server socket fd %d \n", new_fd);
    if (new_fd == -1) {
        fprintf(stderr,"    AsynConnect(): No connection pending\n");
        return;
    }
    if ((flags = fcntl( new_fd, F_GETFL, 0 )) == -1) {
	fprintf(stderr,"set socket nonblocking, can't get current setting" );
        close(new_fd);
        return;
    }
    flags |= O_NDELAY;
    if (fcntl( new_fd, F_SETFL, flags ) == -1) {
	fprintf(stderr,"set socket nonblocking, can't change current setting" );
        close(new_fd);
        return;
    }
    readyJCatchers( new_fd );
}


int createVSocket(int type, Socket *tSocket )
/*int type - protocol type, TCP or UDP */
{

     memset( tSocket, 0, sizeof( Socket ) );
     tSocket->sd = -1;
     tSocket->protocol = type;
     return(0);
}


Socket sVnmrJaddr; /* vnmraddr */
Socket sVnmrJcom; /* common text */
Socket sVnmrJAl;  /* alpha text window */
Socket sVnmrGph;  /* graphics */
Socket sVnmrPaint;  /* graphics for print screen */
Socket sVnmrNet;  /* network  */

int Gphsd = -1;
int Gpaint = -1;  // for print screen

static int sendToVnmrJinit(int nodelay, Socket *pSD )
{
    int i, ival, on=1;
/*  int buff=DEFAULT_SOCKET_BUFFER_SIZE; */

    if (noUI) return(0);

/*    pSD = createSocket( SOCK_STREAM ); */
/* mallocs space for socket and passes me the pointer */
/*    createVSocket( SOCK_STREAM, pSD ); */

    if (pSD == NULL)
      return(-1);
    if (openSocket(pSD) == -1)
      return(-1);
    setsockopt(pSD->sd,SOL_SOCKET,(~SO_LINGER),(char *)&on,sizeof(on));
    if (nodelay==1)
      setsockopt(pSD->sd,SOL_SOCKET,TCP_NODELAY,(char *)&on,sizeof(on));
/*  setsockopt(pSD->sd,SOL_SOCKET, SO_SNDBUF, (char *) &(buff), sizeof(buff));
    setsockopt(pSD->sd,SOL_SOCKET, SO_RCVBUF, (char *) &(buff), sizeof(buff)); */
    ival = -1;
    for( i=0; i < 5; i++)
    {
        ival = connectSocket(pSD,VnmrJHostName,VnmrJPortId);
        if (ival == 0)
	    break;
        sleep(1);
    }
    if (ival == 0)
        return(0);
    return (-1);
}

int net_write(char *netAddr, char *netPort, char *message)
{
     int  bNew, port, k, len;
     char *d;
     char addr[128];

     if (netAddr == NULL || netPort == NULL || message == NULL)
        return(-1);
/*
     if (sscanf(netAddr, "%s%d", addr, &port) != 2)
        return;
*/
     d = netAddr;
     while (*d == ' ') d++;
     strcpy(addr, d);
     d = netPort;
     while (*d == ' ') d++;
     port = atoi(d);
     if ( (strlen(addr) < 2) || (port == 0) )
        return(-2);
     if (debug)
        fprintf(stderr, " net_write  host: %s   port: %d \n", addr, port);
     if (sVnmrNet.sd > 0)
        closeSocket(&sVnmrNet);
     if (openSocket(&sVnmrNet) == -1)
        return(-3);
     setsockopt(sVnmrNet.sd,SOL_SOCKET,(~SO_LINGER),(char *)&k,sizeof(k));
     setsockopt(sVnmrNet.sd,SOL_SOCKET,TCP_NODELAY,(char *)&k,sizeof(k));
     bNew = 0;
     while (bNew < 6) {
         k = connectSocket(&sVnmrNet,addr,port);
         if (k == 0)
             break;
         if (bNew > 4)
             return(-4);
         bNew++;
         sleep(1);
     }
     if (sVnmrNet.sd < 0)
         return(-5);
     len = strlen(message);
     k =  writeSocket( &sVnmrNet, message, len);
     closeSocket(&sVnmrNet);
     return(0);
}

static int getJfile(char *vstr )
{
  if (noUI) return(0);
  if (access( vstr, F_OK ) == 0)
  {
    unlink( vstr );
    return(0);
  }
  else
    return(1);
}

int smagicSendJvnmraddr(int portno) /* send Java vnmraddr */
{
    char smagicJaddr[MAXPATH];
    int val2;
    int i=1, getjfile=1;
    char mstr[1024], kstr[MAXPATH+128], kstr2[64], kstr3[64], kstr1[64];
/*  int newPort; */

    (void) portno;
    if (isMaster==0)
      return(0);
    get_vnmrj_addr(smagicJaddr); /* better than getparm, don't need global tree to be present */
    sscanf(smagicJaddr, "%s %s %s", kstr1,kstr2,kstr3);
/**
#ifndef MOTIF
    newPort = listen_socket(portno);
    sprintf(smagicJaddr,"%s %d %s",kstr1, newPort, kstr3);
#endif
**/
    sprintf(kstr, "%s/tmp/vnmr%s", systemdir, kstr3);
    if (access( Jvbgname, F_OK ) == 0)
       unlink( Jvbgname );
    unlink( kstr );
    if (noUI)
        return(0);
    while (getjfile == 1)
    { 
      i++;
      if (sVnmrJaddr.sd == -1) /* should I check this, or just send it?? */
      {
        sendToVnmrJinit(1, &sVnmrJaddr);
      }
      if (sVnmrJaddr.sd != -1) /* should be > 0?? */
      {
        sprintf(mstr, "addr %s\n", smagicJaddr);
        if ((val2=messageSendToVnmrJ( &sVnmrJaddr, mstr, strlen(mstr))) != 0)
        {
          return(-1);
        }
#ifndef __INTERIX
        sleep(1);
        getjfile = getJfile(kstr);
#else
        getjfile = 0;
#endif
        if (i > 10)
        {
	  return( -1 );
	  break;
        }
      }
      closeSocket( &sVnmrJaddr ); /* shouldn't this set sVnmrJaddr.sd = -1 ?? */
    }
    return(0);
}

static void init_win_info()
{
	char *tokptr, *strptr;
	int k, x, y;
	unsigned int w, h;

	if (vjWinInfo == NULL)
	    return;

	strptr = vjWinInfo;
	while ((tokptr = strtok(strptr, " \n")) != (char *) 0)
        {
	    k = 0;
	    strptr =  (char *) 0;
	    if (strcmp(tokptr, "-jxid") == 0)
		k = 1;
	    else if (strcmp(tokptr, "-jxgeom") == 0)
		k = 2;
	    else if (strcmp(tokptr, "-jxregion") == 0)
		k = 3;
	    tokptr = strtok(strptr, " \n");
	    if (tokptr == NULL)
		break;
	    if (k == 0)
		break;
	    if (k == 1) {
		k = atoi(tokptr);
		if (k > 0)
		   k = 1;
		jSetGraphicsWinId(tokptr, k);
	    }
	    else if (k == 2) {
		if (XParseGeometry(tokptr, &x, &y, &w, &h) > 0) {
		    set_win_size(x, y, (int) w, (int) h);
		}
	    }
	    else if (k == 3) {
		if (XParseGeometry(tokptr, &x, &y, &w, &h) > 0) {
		   set_win_region(1, x, y, x+(int) w, y+(int) h);
		}
	    }
	}
}

/*  This routine process the command line used to start Vnmr.  */

static void vnmr_argvec(int argc, char *argv[] )
{
    char *p;
    char *vnmode;
    char *moreFds;				/*  Used???  */
    int i;

/*  Parsing loop changed so that only one option can be
    specified per item in the argument vector.			*/

    vnmode = NULL;
    vjWinInfo = NULL;
    VnmrJPortId = 0;
    psgaddr[0] = '\0';
    for (i=1; i<argc; ++i)
    {
	p = argv[ i ];
	if (*p == '-')
	    switch (*(++p))
	    {
		  case 'B':     resizeBorderWidth = atoi(p+1);
                                break;
		  case 'D':	Dflag -= 1;
				break;
		  case 'd':	
                                if (!strcmp(argv[i], "-display"))
                                {
                                     sprintf(Xdisplay, "-display");
                                     i++;
                                     strcpy(Xserver, argv[i]);
                                }
				else
                                {
                                     int len;

                                     strcpy( &datadir[ 0 ], p+1 );
                                     len = strlen(datadir);
                                     if ((len > 4) && ( strcmp( &datadir[len - 4], ".fid") == 0) )
                                        datadir[len-4] = '\0';
                                }
				break;
		  case 'E':	Eflag -= 1;
				break;
		  case 'e':	Eflag += 1;
				break;
		  /* option f Does not need to be used */
		  case 'f':     moreFds = p+1;
				moreFd = atoi(moreFds);
				break;
		  /* The textsw file descriptor */
                  case 'F':
                                sprintf(textFrameFds, "%s", p+1);
                                break;
		  case 'G':	Gflag -= 1;
				break;
		  case 'g':	Gflag += 1;
				break;
		  case 'h':     
                                if (!strcmp(argv[i], "-host"))
                                {
				    i++;
				    if (strlen(argv[i]) > 0)
				    {
					strcpy(VnmrJHostName,argv[i]);
/*					  if (~isMaster)
				        printf( "smagic: VnmrJHostName %s\n", VnmrJHostName);
*/
				    }
				    continue;
				}
				else
				{
				    SET_VNMR_ADDR(p+1);
                                    strcpy(psgaddr, p+1);
				}
				break;
		  case 'H':     masterWinHeight = atoi(p+1);
				break;
		  case 'i':     SET_ACQ_ID(p+1);
				break;
		  case 'j':
				vjWinInfo = (char *) malloc(strlen(argv[i]) + 2);
			        sprintf(vjWinInfo, argv[i]);
				break;
		  case 'K':     textWinHeight = atoi(p+1);
                                break;
		  case 'L':	Lflag -= 1;
				break;
		  case 'l':	Lflag += 1;
				break;
                  case 'M':
                                sprintf(mainFrameFds, "%s", p-1);
                                break;
		  case 'm':     
                                if (!strcmp(argv[i], "-mserver"))
                                    Bserver = 1;
                                else
                                    vnmode= p+1;
				break;
		  case 'n':     if ( *(p+1) == '+' )
                                {
                                   expnum = -1;
                                }
		                else if ( *(p+1) == '-' )
                                {
                                   expnum = -2;
                                }
                                else
                                {
                                   expnum = atoi(p+1);
                                }
				break;
                  case 'O':
                                sprintf(buttonWinFds, "%s", p-1);
                                break;
		  case 'P':	jParent = atoi(p+1);
				break;
		  case 'p':
                                if (!strcmp(argv[i], "-port"))
                                {
				    i++;
				    if (strlen(argv[i]) > 0)
				    {
    				        VnmrJPortId = atoi(argv[i]);
				        if (VnmrJPortId < 0)
					    fprintf(stderr, "smagic: invalid port id %s\n", argv[i]);
				    }
				    continue;
				}
				break;
		  case 'R':	Rflag -= 1;
				break;
		  case 'r':	Rflag += 1;
				break;
		  case 'S':     masterWinWidth = atoi(p+1);
				break;
		  case 's':     i++;
                                strcpy(serverMac, argv[i]);
				break;
		  case 'T':	Tflag -= 1;
				break;
		  case 't':	Tflag += 1;
				break;
		  case 'u':	strcpy(&userdir[ 0 ],p+1);
				break;
		  case 'V':	Vflag -= 1;
				break;
		  case 'v':
                                if (!strcmp(argv[i], "-view"))
                                {
				    i++;
				    if (strlen(argv[i]) > 0)
				    {
    				        VnmrJViewId = atoi(argv[i]);
				        if (VnmrJViewId < 0 || VnmrJViewId > VNMRJ_NUM_VIEWPORTS)
					    fprintf(stderr, "smagic: invalid view id %s\n", argv[i]);
				    }
				    continue;
				}
				else
				{
				    Vflag += 1;
				}
				break;
		  case 'W':	Wretain -= 1;
				break;
		  case 'w':	Wretain += 1;
				break;
		  case 'x':
                                if (!strcmp(argv[i], "-xserver"))
                                    Bserver = 1;
                                else if (!strncmp(argv[i], "-xport_", 7)) {
                                    inPort = atoi(p+6);
                                }
				break;
                  case 'Z':
                                if (strlen(argv[i]) > 2)
                                   strcpy(&fontName[ 0 ],p+1);
                                break;
		  default:	fprintf(stderr,"magic: unknown option '%c' available options DdEeGgLlPpRrTtuVvWw\n",*p);
				break;
	}
	else
	{   initCommand = p;
            if ( ((i+1) < argc) && strlen(serverMac) )
            {
               i++;
               initArg = argv[i];
               if ((i+1) < argc)
               {
                  fprintf(stderr,"Unknown argument '%s' passed to Vnmrbg\n",argv[i+1]);
                  exit(1);
               }
            }
	    if (Tflag)
		fprintf(stderr,"Command line %s passed to vnmr\n",initCommand);
	}
    }			/*  End of for each argument loop */

    setUpEnv(vnmode);	/* setup environmental variables */
}

static void open_xterm()
{
        FILE *xfd;
        int k, ret;

        if (noUI)
           return;
	sprintf(tmpStr, "%s/.vjtty", getenv("vnmruser"));
        xfd = fopen(tmpStr, "r");
        if (xfd == NULL)
           return;
        ret = 0;
        if (fgets(tmpStr, 64, xfd) != NULL)
        {
           sscanf(tmpStr,"%[^\n\r]s",showStr );
           k = strlen(showStr);
           if (k > 2) {
                ret = open_terminal(1, showStr);
           }
        }
        fclose(xfd);
        if (ret < 1) {
	   sprintf(tmpStr, "%s/.vjtty", getenv("vnmruser"));
	   unlink(tmpStr);
        }
}


int vnmr(int argc, char *argv[])
{
    createVSocket( SOCK_STREAM, &sVnmrJaddr );
    createVSocket( SOCK_STREAM, &sVnmrJcom );
    createVSocket( SOCK_STREAM, &sVnmrJAl );
    createVSocket( SOCK_STREAM, &sVnmrGph );
    createVSocket( SOCK_STREAM, &sVnmrNet );
    createVSocket( SOCK_STREAM, &sVnmrPaint );

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
#ifdef VMS
    vf_init();				/* Initialize binary disk I/O system */
#endif 
    setCancel(1,"");
    /*signal(SIGINT,gotInterupt);*/
    catch_sigint();
    Dflag     = 0;
    Eflag     = 0;
    Gflag     = 0;
    ignoreEOL = 0;
    Aflag     = 0;
    Lflag     = 0;
    Pflag     = 0;
    Rflag     = 1;
    Tflag     = 0;
    Vflag     = 0;
    Bnmr      = 0;
    Bserver   = 0;
    noUI      = 0;
    noGUI     = 0;
    inPrintMode = 0;
    Wretain   = 1;   /* default retained */
    expnum    = 1;
    fontName[0] = '\0';
    Xserver[0] = '\0';
    Xdisplay[0] = '\0';

/* sleep(5); */

    datadir[0]= '\0';				/* No path to data */
    serverMac[0]= '\0';				/* No path to data */
    INIT_VNMR_ADDR();
    vnmr_argvec( argc, argv );
    VNMR_ADDR_OK(); /* could put before vnmr_argvec() ?? */
    sprintf(Jvbgname, "/tmp/vbgtmp%d", VnmrJPortId);

    if (Bnmr || Bserver) {
        noUI = 1;
        noGUI = 1;
    }
    open_xterm();
    if (noUI && debug)
        fprintf(stderr,"Background VNMR sys =%s user=%s expnum=%d vnMode =%s\n",systemdir,userdir,expnum,vnMode);

    if (Tflag)
    {
        if (Bnmr)
        {
            fprintf(stderr,"Background VNMR sys =%s user=%s moreFd = %d expnum=%d vnMode =%s\n",systemdir,userdir,moreFd,expnum,vnMode);
        }
        else
        {
            fprintf(stderr,"Forground VNMR sys =%s user=%s moreFd = %d expnum=%d vnMode=%s\n",systemdir,userdir,moreFd,expnum,vnMode);
        }
    }

    INIT_ACQ_COMM(systemdir);

    merge_command_table(aipGetCommandTable());

    if (Vflag)
	showMacros();

    if (Bnmr)  /* background vnmr */
    {

	if ( (mode_of_vnmr == ACQUISITION) ||
             (mode_of_vnmr == AUTOMATION) )
        {
           char logPath[MAXPATH];
           strcpy(logPath,systemdir);
           strcat(logPath,"/tmp/acqlog");
           if (access( logPath, W_OK ) == 0)
           {
              freopen(logPath, "a", stderr);
              freopen(logPath, "a", stdout);
           }
           checkAcqStart(initCommand);
        }
        INIT_VNMR_COMM(HostName);	/* To receive stuff from acq process */
        graphics = "none";
        setWissun(0);
	setupdirs( "background" );
        if (expnum < 0)
        {
           int expi;
           char expn[32];
           if (nextexp_d( &expi, expn))
           {
              char *argv2[3];
              char *retv2[2];
                    
              if (expnum == -2)
                 autoDelExp=1;
              expnum = expi;
              sprintf(expn,"%d",expi);
              argv2[0] = "cexp";
              argv2[1] = expn;
              argv2[2] = NULL;
              strcpy( curexpdir, userdir );
              strcat( curexpdir, "/exp0" );
              cexpCmd(2, argv2, 1, retv2);
   
           }
           else
           {
              expnum = 0;
           }
        }
        jgraphics_init();
	bootup(vnMode,expnum);
	if (initCommand)
	{
            char tmpStr[2048];
            if (Tflag)
		fprintf(stderr,"executing command line %s\n",initCommand);
            if (strlen(serverMac))
            {
               if (initArg)
                  sprintf(tmpStr,"%s(`%s`,`%s`)",serverMac,initCommand,initArg);
               else
                  sprintf(tmpStr,"%s(`%s`)",serverMac,initCommand);
            }
            else
            {
               strcpy(tmpStr,initCommand);
            }
            strcat(tmpStr,"\n");
            if (MflagVJcmd)
            {
              fprintf(stderr,"initCommand %s",tmpStr);
              fflush(stderr);
              MflagVJcmd = 0;
            }
            working = 1;
            loadAndExec(tmpStr);
	}
	nmr_exit(vnMode);
	exit(0);
    }

    graphics = "sun";

    jgraphics_init();

#ifdef SUN
    if (Wissun())  /* sun vnmr */
    {	setupdirs( "child" );
	graphics_window = 1;
        setUpWin(Wretain, noUI); /* setup frame */ /* calls make_table() */
	if (Tflag)
	  fprintf(stderr,"before notify set input func\n");
    /* set to yell at fd 0 */
        inputfd = 0;
#ifdef MOTIF
        set_input_func();
#endif
	if (Tflag)
	  fprintf(stderr,"after notify set input func\n");
	set_bootup_gfcn_ptrs(); /* also done in bootup() */
	jvnmr_init();
        if (!noUI)
	    init_win_info();
	bootup(vnMode,expnum);
/*	setWissun(1); */
	JgetPid = getpid();
        smagicLoop();
	if (Tflag)
	  fprintf(stderr,"after window_main_loop \n");
    }
    else  /* terminal vnmr */
#endif 
    {	setupdirs( "terminal" );
	graphics_window = 0;
	Wsetupterm(); /* setup windows */
	bootup(vnMode,expnum);
	terminal_main_loop();
    }
    return(0);
}

#ifdef XXX
/*--------------------------------------------------------------------
|    getInputBuf/2
|
|	This routine grabs a string from the input pipe and  
|	passes it back to the caller
|
/+--------------------------------------------------------------------*/
char *getInputBuf(char *buf, int len)
{   char c;
    int  i = 0;
    int  res;

    if (!len)
	return ('\0');  /* why would anyone want 0 length */
   
    res = read(0,&c,1); /* read first character */
    if (Tflag)
    {	if (res == -1)
	    perror("getInputBuf:Read");
	fprintf(stderr,"getInputBuf:res = %d read %c\n",res,c);
    }
    while (i < len)
    {	if (c == NEWLINE)
	{   buf[i] = '\0';
	    return(buf);   
	}
	buf[i++] = c;
     	res = read(0,&c,1);
	if (Tflag)
	{   if (res == -1)
		perror("getInputBuf:Read");
	    fprintf(stderr,"getInputBuf:res = %d read %c\n",res,c);
	}
    }
    buf[len-1] = '\0';
    return (buf);
}

#endif 


/*
 * This prevents mouse movements over the graphics canvas from blocking
 * all input to the Sun windows.  This is a problem which seems to be
 * new in SunOS 4.x.  This procedure is called when spectra or FIDs are
 * accessed.  (calc_spec(),  gettrace(),  getfid(),  get_one_fid(), and
 * secondft().)
 */
void long_event()
{
#ifdef SUN
   if (graphics_window)
      release_canvas_event_lock();
#endif 
}


#ifdef XXX
/*--------------------------------------------------------------------
|    macro_main_loop
|
|	This opens macro file and executes from it 
|
/+--------------------------------------------------------------------*/
void macro_main_loop(char *file)
{   char  buf[1024];
    FILE *stream;

    if ( (stream = fopen(file,"r")) )
    {	while (fgets(buf,sizeof(buf),stream))
	    loadAndExec(buf);
    }
    else
    {	fprintf(stderr,"Errors opening file %s\n",file);
	exit(1);
    }
}
#endif

/*--------------------------------------------------------------------
|    setUpEnv
|
|	This routine checks and reads in the environmental variables
|	for vnmr.  It also checks if userdir/maclib exists and 
|	creates one if it doesn't.
|
/+--------------------------------------------------------------------*/

static int setUpEnv(char *modeptr)
{
    int  mode_err;

    Bnmr = 0;
    mode_err = 0;

    /* setup vnMode */

    if (modeptr == NULL) /* if not predefined by option */
	modeptr = "foreground";
    else if (strlen(modeptr) > 20)
    {
    	fprintf(stderr,"Error: value of mode option too long\n");
	exit( 1 );
    }

/*  Mode can be:
	foreground
	background
	acquisition
	automation

    Only the minimum number of characters are required.
    If mode is not recognized, mode_of_vnmr will remain 0.	*/

    mode_of_vnmr = 0;
    if (*modeptr == 'b')
      mode_of_vnmr = BACKGROUND;
    else if (*modeptr == 'a')
    {
	modeptr++;
	if (*modeptr == 'c')
	  mode_of_vnmr = ACQUISITION;
	else if (*modeptr == 'u')
	  mode_of_vnmr = AUTOMATION;
	modeptr--;
    }
    else if (*modeptr == 'f')
      mode_of_vnmr = FOREGROUND;

    if (mode_of_vnmr == 0)
    {
	fprintf( stderr, "invalid mode option %s\n", modeptr );
	exit( 1 );
    }
    strcpy(vnMode,modeptr);
    Bnmr = (mode_of_vnmr != FOREGROUND);

    if (Tflag)
	fprintf(stderr,"setUpEnv systemdir =%s userdir=%s expnum = %d\n",systemdir,userdir,expnum);
    return (0);
}


#define  EOT 	04
#define ACQMBUFSIZE 8192
struct acqmsgentry {
	char	*nextentry;
	char	curentry[ ACQMBUFSIZE ];
};

extern int         fgBusy;
extern int         forgroundFdW;

struct acqmsgentry  *baseofqueue = NULL;
static struct acqmsgentry  *moveEntry = NULL;
static char        acqProcBuf[ACQMBUFSIZE+4];
static int         acqProcBufPtr = 0;
static char        jCmdBuf[ACQMBUFSIZE+4];
static int         jCmdBufPtr = 0;

/*
 * Put command in history list - but filter out nuisance commands
 * used for VJ <-> VBG communication.
 */
static void
jStuffCommand(char *s)
{
    if (!(*s == '\0'
	  || strncmp(s,"jFunc",5)==0
	  || strncmp(s,"readlk",6)==0
	  || strncmp(s,"acqsend",7)==0
	  || strncmp(s,"acqstat",7)==0
	  || strncmp(s,"vnmrjcmd",8)==0))
    {
	stuffCommand(s);
    }
}

static void
jCatInput(char *s) /* M@inp */
{
  int i,len,size;
  size = 5;
  len = strlen(s);
  if (len > size)
  {
    for (i=0; i<len-size; i++)
    {
      s[i] = s[i+size];
    }
  }
  if (len >= size)
    s[len-size]='\0';
}

#ifdef  DEBUG
extern int Gflag;
#define GPRINT0(str) \
        if (Gflag) fprintf(stderr,str)
#define GPRINT1(str, arg1) \
        if (Gflag) fprintf(stderr,str,arg1)
#else 
#define GPRINT0(str)
#define GPRINT1(str, arg1)
#endif 

void resetMasterSockets() /* reset master sockets */
{
	if (Bnmr) return;
	closeSocket( &sVnmrJcom );
	sVnmrJcom.sd = -1;
	closeSocket( &sVnmrJAl );
	sVnmrJAl.sd = -1;
	closeSocket( &sVnmrGph );
	sVnmrGph.sd = -1;
	closeSocket( &sVnmrJaddr );
	sVnmrJaddr.sd = -1;
        if (sVnmrPaint.sd >= 0)
	    closeSocket( &sVnmrPaint );
	sVnmrPaint.sd = -1;
}

/*---------------------------------------------------------------
|
|   exec_message()/0
|	send acquisition message to Vnmr
|
+--------------------------------------------------------------*/
static void
exec_message()
{
   int len;
   char *xstr;
   extern void jNextPrev();
   acqProcBuf[acqProcBufPtr] = '\0';
   if (acqProcBufPtr <= 0)
       return;
   GPRINT1("M: Got '%s'\n",acqProcBuf);
   if (strcmp(acqProcBuf," "))
   {
      if (Wissun())
      {
         GPRINT1("fgBusy = %d\n", fgBusy);
	 if (strcmp(acqProcBuf,"jFunc(2)")==0)
	    resetMasterSockets();
	 if ((acqProcBuf[0]=='M') && (acqProcBuf[1]=='@'))
	 {
	    len = strlen(acqProcBuf);
	    if ((acqProcBuf[len-2]=='C') && (acqProcBuf[len-1]=='@'))
	    {
		acqProcBuf[len-2] = '\0';
	    }
            if (strncmp(acqProcBuf,"M@event", 7)==0) {
	    	xstr = &acqProcBuf[8];
            	write(forgroundFdW,xstr,strlen(xstr));
            	write(forgroundFdW,"\n", 1);
   	    	acqProcBufPtr = 0;
	    	return;
	    }
            else if (strcmp(acqProcBuf,"M@cancel")==0)
	    {
		if (jInput == 1)
		{
/*		    acqProcBuf[0] = '\n';
		    acqProcBuf[1] = '\0';
		    write(forgroundFdW,acqProcBuf,strlen(acqProcBuf));
*/
		    jInput = 0;
		}
                cancelCmd();
	    }
            else if (strcmp(acqProcBuf,"M@abort")==0)
                AbortAcq();
            else if (strcmp(acqProcBuf,"M@jFunc2")==0)
	        resetMasterSockets();
	    else if (strcmp(acqProcBuf,"M@next")==0)
		jNextPrev( 1 );
	    else if (strcmp(acqProcBuf,"M@prev")==0)
		jNextPrev( -1 );
            else if ((strncmp(jCmdBuf,"M@inp", 5)==0) && (jInput == 1))
	  {    /* vnmrj input() */
	    jCatInput(acqProcBuf);
/*	    jStuffCommand(acqProcBuf); */
	    len = strlen(acqProcBuf);
	    if (len >= ACQMBUFSIZE)
	    {
	       acqProcBuf[len-1] = '\n';
	       acqProcBuf[len] = '\0';
	    }
	    else /* if (acqProcBuf[len-1] != '\n') */
	    {
	       acqProcBuf[len] = '\n';
	       acqProcBuf[len+1] = '\0';
	    }
            write(forgroundFdW,acqProcBuf,strlen(acqProcBuf));
/*		sendChildNewLine(); */
/*             sendChildNewLineNoClear(); */
	    jInput = 0;
	  }
	 }
         else if (fgBusy)
	 {
/* add C@ to start or end of acqProcBuf, then stuff() but strip extra chars first */
/*	    jStuffCommand(acqProcBuf); */
            insertAcqMsgEntry( &acqProcBuf[ 0 ] );
	 }
         else
         {
/*	    jStuffCommand(acqProcBuf); */
            write(forgroundFdW,acqProcBuf,strlen(acqProcBuf));
            sendChildNewLineNoClear();
         }
      }
      else
      {
         strcat(acqProcBuf,"\n");
         execString( &acqProcBuf[ 0 ] );
      }
   }
   acqProcBufPtr = 0;
}

#ifdef XXX
static char *
jfunc2macro(char *jcmd,		/* jFunc(9,... call to translate */
	    char *key,		/* Initial substring (like "jFunc(9,") */
	    int itr)		/* Iteration number; unique in this macro */
{
    char *pc;
    char *pc1;
    char *pc2;
    char *id;
    char expr[ACQMBUFSIZE];
    char *prec;
    char newvar[STR64];
    char buf[MAXSTR];
    char var[MAXSTR];
    char *jcall;
    char jcallbuf[ACQMBUFSIZE];
    static char rtn[ACQMBUFSIZE];

    strcpy(var, jexprDummyVar);
    strcpy(jcallbuf, jcmd);	/* Working buffer for tokenizing command */
    jcall = jcallbuf;
    jcall += (int)strlen(key);	/* Skip to "ID" argument */
    id = strtok_r(jcall, " ,", &pc);
    /* Advance to first "'" delimited string */
    if (strtok_r(NULL, "'", &pc) == NULL) {
	return NULL;
    }
    if ((pc1=strtok_r(NULL, "'", &pc)) == NULL) {
	return NULL;
    }
    *expr = '\0';
    strcat(expr, pc1);
    while (expr[strlen(expr)-1] == '\\') {
	/* We actually just found a "\'"; replace it with "'" and continue */
	expr[strlen(expr)-1] = '\'';
	if (pc1[strlen(pc1)+1] == '\'') {
	    /* We're sitting on the true end */
	    break;
	}
	if ((pc1=strtok_r(NULL, "'", &pc)) == NULL) {
	    return NULL;
	}
	strcat(expr, pc1);
    }

    /* Advance to possible second "'" delimited string */
    strtok_r(NULL, "'", &pc);
    if (!(prec = strtok_r(NULL, "'", &pc))) {
	prec = "";
    }

    /*
     * jFunc command is parsed. Now assemble the pieces.
     */
    sprintf(newvar,"%s%d", var, itr);
    /* Put in the "expr" with "newvar" substituted for "var" */
    rtn[0] = '\0';
    for (pc1 = expr; (pc2 = strstr(pc1, var)); pc1 = pc2 + strlen(var)) {
	strncat(rtn, pc1, (pc2 - pc1));
	strcat(rtn, newvar);
    }
    strcat(rtn, pc1);
    strcat(rtn, "\n");

    /* Put in a write of "newvar" value to file */
    sprintf(buf,"jwrite('fileln',$fd,'%s ','',%s,'%s')\n", id, newvar, prec);
    strcat(rtn, buf);

    return rtn;
}
#endif

#define XRESUME  80
#define XSHOW           31

/*---------------------------------------------------------------
|
|   Jexec_message()/0
|	send vnmrj message to Vnmr
|
+--------------------------------------------------------------*/
static void
Jexec_message()
{
   int len;
   int res;
   char *xstr;
   extern void jNextPrev();

   static int xDrag = 0;

   jCmdBuf[jCmdBufPtr] = '\0';

   /*{
       struct timeval tv;
       gettimeofday(&tv, 0);
       fprintf(stderr,"%d.%03d: Jexec_message: %s \n",
	       tv.tv_sec, tv.tv_usec/1000, jCmdBuf);
   } */ /*TIMING*/

   if (jCmdBufPtr <= 0)
       return;
   GPRINT1("M: Got '%s'\n",jCmdBuf);
   if (*jCmdBuf && strcmp(jCmdBuf," "))
   {
      if (Wissun())
      {
         GPRINT1("fgBusy = %d\n", fgBusy);
	 if (strcmp(jCmdBuf,"jFunc(2)")==0)
	    resetMasterSockets();
	 if ((jCmdBuf[0]=='X') && (jCmdBuf[1]=='@'))
	 {
/*
	    len = strlen(jCmdBuf);
	    jCmdBuf[len] = '\n';
	    jCmdBuf[len+1] = '\0';
*/
   	    jCmdBufPtr = 0;
            moveEntry = NULL;
            if (strncmp(jCmdBuf,"X@stop", 6)==0) {
		xDrag = 0;
		res = -1;
            	if (strncmp(jCmdBuf,"X@stop3", 7)==0) {
		    res = -20;
                    removeJeventEntry("jEvent", 6);
                    removeJeventEntry("jMove", 5);
                }
            	else if (strncmp(jCmdBuf,"X@stop2", 7)==0) {
		     res = -20;
		}
		sprintf(jCmdBuf, "jEvent(%d, %d)", XRESUME, res);
		if (fgBusy) {
            	     insertAcqMsgEntry(jCmdBuf);
		}
		else {
            	     write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
            	     sendChildNewLineNoClear();
		}
		return;
	    } 
            if (strncmp(jCmdBuf,"X@copy", 6)==0) {
	    	xstr = &jCmdBuf[7];
		if (fgBusy) {
		     if (xDrag)
			return;
            	     insertAcqMsgEntry(xstr);
		}
		else {
            	     write(forgroundFdW,xstr,strlen(xstr));
            	     sendChildNewLineNoClear();
		}
		return;
	    }
            if (strncmp(jCmdBuf,"X@redo", 6)==0) {
	    	xstr = &jCmdBuf[7];
		xDrag = 0;
		if (fgBusy) {
            	     insertAcqMsgEntry(xstr);
		}
		else {
            	     write(forgroundFdW,xstr,strlen(xstr));
            	     sendChildNewLineNoClear();
		}
		return;
	    }
            if (strncmp(jCmdBuf,"X@region", 8)==0) {
	    	xstr = &jCmdBuf[9];
                removeJeventEntry("jRegion", 7);
		if (fgBusy) {
            	     insertAcqMsgEntry(xstr);
		}
		else {
            	     write(forgroundFdW,xstr,strlen(xstr));
            	     sendChildNewLineNoClear();
		}
		return;
	    }
	    return;
	 }
	 if ((jCmdBuf[0]=='M') && (jCmdBuf[1]=='@'))
	 {
	    len = strlen(jCmdBuf);
	    if ((jCmdBuf[len-2]=='C') && (jCmdBuf[len-1]=='@'))
	    {
		jCmdBuf[len-2] = '\0';
	    }
            if (strncmp(jCmdBuf,"M@event", 7)==0) {
		jCmdBuf[len] = '\n';
		jCmdBuf[len+1] = '\0';
	    	xstr = &jCmdBuf[8];
            	write(forgroundFdW,xstr,strlen(xstr));
            	// sendChildNewLineNoClear();
   	    	jCmdBufPtr = 0;
	    	return;
	    }
	    if (strncmp(jCmdBuf,"M@xstop", 7)==0) {
                jCmdBufPtr = 0;
                if (fgBusy)
                    res = kill(forgroundPid, SIGINTX1);
                else {
		    sprintf(jCmdBuf, "jFunc(%d, 0)", XSHOW);
            	    write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
            	    sendChildNewLineNoClear();
                }
                return;
            }
            if (strncmp(jCmdBuf,"M@xstart", 8)==0) {
                jCmdBufPtr = 0;
                if (fgBusy)
                    res = kill(forgroundPid, SIGINTX2);
                else {
		    sprintf(jCmdBuf, "jFunc(%d, 1)", XSHOW);
            	    write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
            	    sendChildNewLineNoClear();
                }
                return;
            }
            if (strncmp(jCmdBuf,"M@xresize", 9)==0) {
                jCmdBufPtr = 0;
		xDrag = 1;
                moveEntry = NULL;
                removeJeventEntry("jEvent", 6);
                removeJeventEntry("jMove", 5);
                if (fgBusy)
                    res = kill(forgroundPid, SIGINTX3);
                else {
		    sprintf(jCmdBuf, "jFunc(%d, 1)", XRESUME);
            	    write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
            	    sendChildNewLineNoClear();
                }
                return;
            }
            if (strcmp(jCmdBuf,"M@cancel")==0)
	    {
		if (jInput == 1)
		{
/*		    jCmdBuf[0] = '\n';
		    jCmdBuf[1] = '\0';
		    write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
*/
		    jInput = 0;
		}
                cancelCmd();
	    }
            else if (strcmp(jCmdBuf,"M@abort")==0)
                AbortAcq();
            else if (strcmp(jCmdBuf,"M@jFunc2")==0)
	        resetMasterSockets();
	    else if (strcmp(jCmdBuf,"M@next")==0)
		jNextPrev( 1 );
	    else if (strcmp(jCmdBuf,"M@prev")==0)
		jNextPrev( -1 );
            else if (strncmp(jCmdBuf,"M@vplayout", 10)==0) {
		jCmdBuf[len] = '\0';
		if (len > 10)
		  writelineToVnmrJ( "vnmrjcmd vplayout use ", &jCmdBuf[10] );
	    }
            else if ((strncmp(jCmdBuf,"M@inp", 5)==0) && (jInput == 1))
	    {    /* vnmrj input() */
	    	jCatInput(jCmdBuf);
	    	jStuffCommand(jCmdBuf);
	    	len = strlen(jCmdBuf);
	    	if (len >= ACQMBUFSIZE)
	    	{
	       	    jCmdBuf[len-1] = '\n';
	       	    jCmdBuf[len] = '\0';
	    	}
	    	else /* if (jCmdBuf[len-1] != '\n') */
	    	{
	            jCmdBuf[len] = '\n';
	            jCmdBuf[len+1] = '\0';
	    	}
            	write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
/*		sendChildNewLine(); */
/*             sendChildNewLineNoClear(); */
	    	jInput = 0;
	    }
   	    jCmdBufPtr = 0;
	    return; /* return on M@ */
	 } 
         else if (fgBusy)
	 {
/* add C@ to start or end of jCmdBuf, then stuff() but strip extra chars first */
	    jStuffCommand(jCmdBuf);
            insertAcqMsgEntry( &jCmdBuf[ 0 ] );
	 }
         else
         {
	    jStuffCommand(jCmdBuf);
	    if (baseofqueue) { /* something wrong */
		insertAcqMsgEntry( &jCmdBuf[ 0 ] );
                sendChildNewLineNoClear();
	    }
	    else {
                write(forgroundFdW,jCmdBuf,strlen(jCmdBuf));
                sendChildNewLineNoClear();
	    }
         }
      }
      else
      {
         strcat(jCmdBuf,"\n");
         execString( &jCmdBuf[ 0 ] );
      }
   }
   jCmdBufPtr = 0;
}

/*-------------------------------------------------------------
|  JSocketIsRead(me,fd)
|
|  Modified  for added robustness see above for comments  .
|				  12/13/90  Greg Brissey
|  Modified  for elimination of EOT.
|				  2/08/91  Greg Brissey
+-------------------------------------------------------------*/
void JSocketIsReadNew(char tbuff[], int readchars)
{  
   int  index,ovrrun;
   char c;
 
      index = ovrrun = 0;
      while (index < readchars)
      {
         c = tbuff[index++];
         switch (c)
         {
         case CR :
                break;
         case '\n': 
/*	        jCmdBuf[jCmdBufPtr++] = 'C'; */ /* dummy case */
/*	        jCmdBuf[jCmdBufPtr++] = '@'; */ /* dummy case */
                Jexec_message();
		break;
         case EOT:   /* skip any EOT char, for compatiblity with previous routines */
		break;
	 default:   
		/* make sure we don't overrun buffer */
                jCmdBuf[jCmdBufPtr++] = c;
                if (jCmdBufPtr >= ACQMBUFSIZE)
                {
                   jCmdBuf[ACQMBUFSIZE - 1] = '\0';
		   ovrrun = 1;
                   fprintf(stderr, "Vnmrbg(%d) message exceeded buffer size %d \n",VnmrJViewId, ACQMBUFSIZE);
                   fprintf(stderr, "message fragment: '%s'\n",jCmdBuf);
                   jCmdBufPtr = 0;
                   jCmdBuf[jCmdBufPtr] = '\0';
                }
                break;
         }
         if (ovrrun)
            break;	/* jump out of while if overran buffer */
      }   

}

#ifdef MOTIF
#define READBUFFSIZE  512
void readVSocket(int fd)
{  
   char tbuff[READBUFFSIZE + 2]; /* two extra character may be added - \n\0 */
   int  index, ovrrun;
   int  n, nchr;
   static int  errTimes = 0;
   char c;

 
   n = 0;
   while ((nchr = read(fd, tbuff, READBUFFSIZE)) > 0)
   {
       n += nchr;
       index = ovrrun = 0;
       while (index < nchr)
       {
          c = tbuff[index++];
          switch (c)
          {
            case '\n': 
                Jexec_message();
		break;
            case EOT:   /* skip any EOT char, for compatiblity with previous routines */
		break;
	    default:   
		/* make sure we don't overrun buffer */
                jCmdBuf[jCmdBufPtr++] = c;
                if (jCmdBufPtr >= ACQMBUFSIZE)
                {
                   jCmdBuf[ACQMBUFSIZE - 1] = '\0';
		   ovrrun = 1;
                   fprintf(stderr, "Vnmrbg(%d) message exceeded buffer size %d \n",VnmrJViewId, ACQMBUFSIZE);
                   fprintf(stderr, "message fragment: '%s'\n",jCmdBuf);
                   jCmdBufPtr = 0;
                   jCmdBuf[jCmdBufPtr] = '\0';
                }
                break;
          }
          if (ovrrun)
              break;	/* jump out of while if overran buffer */
       }
   }
   if (n <= 0)
   {
       errTimes++;
       if (debug)
          fprintf(stderr, "Error: read socket fd, got 0 chars. \n");
       if (errTimes > 4) { /* this socket was broken */
          pid_t ppid;
          ppid = getppid();
          if (ppid == 1)
          {
             write(forgroundFdW, "vnmrexit", 8);
             write(forgroundFdW,"\n", 1);
          }
          removeJCatchers(fd);
          errTimes = 0;
          if (debug)
              fprintf(stderr, "   close socket fd %d \n", fd);
       }
    }
    else
       errTimes = 0;
}
#endif



/*-------------------------------------------------------------
|  AcqSocketIsRead(me,fd)
|
|  Modified  for added robustness see above for comments  .
|				  12/13/90  Greg Brissey
|  Modified  for elimination of EOT.
|				  2/08/91  Greg Brissey
+-------------------------------------------------------------*/
void AcqSocketIsRead(int (*reader)())
{  
   char tbuff[ACQMBUFSIZE];
   int  index,readchars,ovrrun;
   int  read_again, do_read;
   char c;
 
   do_read = 1;
   read_again = 0;
   while (do_read &&
          ((readchars = (*reader)(tbuff, sizeof(tbuff), &read_again)) > 0) )
   {
      index = ovrrun = 0;
      do_read = read_again;
      while (index < readchars)
      {
         c = tbuff[index++];
         switch (c)
         {
         case '\n': 
/*	        acqProcBuf[acqProcBufPtr++] = 'C'; */ /* dummy case */
/*	        acqProcBuf[acqProcBufPtr++] = '@'; */ /* dummy case */
                exec_message();
		break;
         case EOT:   /* skip any EOT char, for compatiblity with previous routines */
		break;
	 default:   
		/* make sure we don't overrun buffer */
                if (acqProcBufPtr < ACQMBUFSIZE)
                {
                   acqProcBuf[acqProcBufPtr++] = c;
                }
                else
                {
                   acqProcBuf[acqProcBufPtr++] = '\0';
		   ovrrun = 1;
                   fprintf(stderr, "Acq message exceeded buffer size %d\n", ACQMBUFSIZE);
                   fprintf(stderr, "message fragment: '%s'\n",acqProcBuf);
                   acqProcBufPtr = 0;
                   break;
                }
         }
         if (ovrrun)
            break;	/* jump out of while if overran buffer */
      }   
   }
   if (readchars <= 0)
   {
      if (acqProcBufPtr)
      {
         exec_message();
      }
   }
}

/*  No concern about concurrent execution so long as both the insert
    and the remove routines are called from the Notify dispatcher (if
    indirectly) AND the Notify dispatcher does not interrupt itself	*/

void insertAcqMsgEntry(char *acqmptr )
{
	register int			 esize, finished;
	register char			*tmpptr;
	register struct acqmsgentry	*curptr, *newptr;

/*  Be careful to leave curptr pointing to something useful, not NULL
    (unless, of course, the queue is empty)				*/

	curptr = baseofqueue;
	finished = (curptr == NULL);
	while ( !finished ) {
		tmpptr = curptr->nextentry;
		if (tmpptr != NULL)
		  curptr = (struct acqmsgentry *) tmpptr;
		else
		  finished = 131071;
	}

/*  Round entry size up to next multiple of 4.  Do not forget
    to include space for the next entry, or for the nul byte.   */

	esize = (strlen( acqmptr ) + 1 + sizeof( char * ) + 3) & ~3;
	if ( (tmpptr = allocateWithId( esize, "acqmsgqueue" )) == NULL ) {
		GPRINT0("BUG! out of memory for acqmsg queue\n" );
		return;
	}

	newptr = (struct acqmsgentry *) tmpptr;
	newptr->nextentry = NULL;
	strcpy( &newptr->curentry[ 0 ], acqmptr );

/*  An empty queue is a special case.  */

	if (baseofqueue == NULL) baseofqueue = newptr;
	else curptr->nextentry = (char *) newptr;
        if (acqmptr[0] == 'j') {
             if (strncmp(acqmptr, "jEve", 4) == 0)  /* jEvent */
                 moveEntry = NULL;
             else if (strncmp(acqmptr, "jMove", 5) == 0) {
                 if (moveEntry != NULL)
                    sprintf(moveEntry->curentry, "jMove(6)");
                 moveEntry = newptr;
             }
        }
}

/*  It is the duty of the routine that calls "removeAcqMsgEntry"
    to deallocate the space used by the queue entry.		*/

struct acqmsgentry *removeAcqMsgEntry()
{
	register struct acqmsgentry	*curptr;

	if (baseofqueue == NULL) return( NULL );	/* empty queue */

	curptr = baseofqueue;
	baseofqueue = (struct acqmsgentry *) curptr->nextentry;
        if (curptr == moveEntry)
            moveEntry = NULL;
	return( curptr );
}

static void removeJeventEntry(char *keyName, int len)
{
	register struct acqmsgentry	*curptr, *pnode, *np;
	char *d;

	if (baseofqueue == NULL) return;
	
	curptr = baseofqueue;
	pnode = baseofqueue;
	while (curptr != NULL) {
	     d = &curptr->curentry[ 0 ];
	     while (*d == ' ') d++;
	     if (strncmp(d, keyName, len) == 0) {
 		 np = (struct acqmsgentry *) curptr->nextentry;
		 if (curptr == baseofqueue) {
		      baseofqueue = np;
		      pnode = np;
		 }
 		 else {
		      pnode->nextentry = (char *) np;
		 }
		 release(curptr);
		 curptr = np;
	     }
	     else {
		 pnode = curptr;
 		 curptr = (struct acqmsgentry *) curptr->nextentry;
	     }
	}
}

void
setVjGUiPort(int on)
{
      if (on > 0)
          noGUI = 0;
      else
          noGUI = 1;
}

void
setVjUiPort(int on)
{
      if (on > 0)
          noUI = 0;
      else
          noUI = 1;
}

void
setVjPrintMode(int on)
{
      if (on > 0)
          inPrintMode = 1;
      else
          inPrintMode = 0;
}


static int messageSendToVnmrJ( Socket *pSD, const char* message, int messagesize)
{
   int bytes;

   if (noUI || inPrintMode) return(0);
   bytes = writeSocket( pSD, message, messagesize );
   return(0); /* return(bytes); */
}

int writelineToVnmrJ(const char *cmd, const char *message )
{
    int val;
    if (noUI || inPrintMode) return(0);
    if (sVnmrJcom.sd == -1)
    {
	sendToVnmrJinit(1, &sVnmrJcom);
	if (sVnmrJcom.sd != -1)
	{
	  if (isMaster)
	    messageSendToVnmrJ( &sVnmrJcom, "mcomm\n", 6);
	  else
	    messageSendToVnmrJ( &sVnmrJcom, "common\n", 7);
	}
    }
/* check length before sending? just send message? */
    if (sVnmrJcom.sd != -1)
    {
      messageSendToVnmrJ( &sVnmrJcom, cmd, strlen(cmd));
      messageSendToVnmrJ( &sVnmrJcom, " ", 1);
      messageSendToVnmrJ( &sVnmrJcom, message, strlen(message));
      val = strlen(message);
      if (message[val-1] != '\n')
        messageSendToVnmrJ( &sVnmrJcom, "\n", 1);
/*    if ((val = messageSendToVnmrJ( pVnmrJcom, mstr, strlen(mstr))) != 0)
        return( -1 ); */
    }
    return(0);
}

#ifdef XXX
int writealphaToVnmrJ( char *message ) /* write message to alpha screen */
{
    if (noUI || inPrintMode) return(0);
/*  if (isMaster==0) return(0); */
    {
      if (sVnmrJAl.sd == -1)
      {
        sendToVnmrJinit(1, &sVnmrJAl);
        if (sVnmrJAl.sd != -1)
          messageSendToVnmrJ( &sVnmrJAl, "alpha\n", 6);
      }
    }
    if (sVnmrJAl.sd != -1)
    {
      messageSendToVnmrJ( &sVnmrJAl, message, strlen(message));
    }
    return(0);
}
#endif

void openPrintPort()
{
    if (Gpaint < 0)
    {
        sendToVnmrJinit(1, &sVnmrPaint);
        Gpaint = sVnmrPaint.sd;
        if (Gpaint >= 0)
          messageSendToVnmrJ( &sVnmrPaint, "print\n", 6);
    }
}

static int graphToPrint( char *message, int messagesize )
{
    if (Gpaint < 0)
    {
        sendToVnmrJinit(1, &sVnmrPaint);
        Gpaint = sVnmrPaint.sd;
        if (Gpaint >= 0)
          messageSendToVnmrJ( &sVnmrPaint, "print\n", 6);
    }
    if (Gpaint >= 0)
        writeSocket( &sVnmrPaint, message, messagesize );
    return(0);
}

void graph_off(int mode) {
   noGraph=mode;
   if(mode) strcpy(graphOffMacro,getParentMacro());
   else strcpy(graphOffMacro,"");
}

void doGraphOff(char *macroName) {
   if(strlen(graphOffMacro) < 1) return;
   if(strcmp(macroName,graphOffMacro) != 0) return;
   if(macroName && !strcmp(macroName,graphOffMacro)) graph_off(0);
}

int graphoff(int argc, char *argv[], int retc, char *retv[]) {
  if(argc>1) graph_off(atoi(argv[1]));
  else graph_off(1);
  return(0);
}

int graphToVnmrJ( char *message, int messagesize )
{
    int val;
    if (noGraph || noGUI || isMaster) return(0);
    if (message == NULL) return(0);
    if (inPrintMode) 
    {
        return (graphToPrint(message, messagesize));
    }
    if (Gphsd < 0)
    {
        sendToVnmrJinit(1, &sVnmrGph);
        Gphsd = sVnmrGph.sd;
        if (Gphsd >= 0)
          messageSendToVnmrJ( &sVnmrGph, "graph\n", 6);
    }
    if (Gphsd >= 0)
    {
      val = writeSocket( &sVnmrGph, message, messagesize );
    //  messageSendToVnmrJ( &sVnmrGph, message, messagesize);
/*    if ((val = messageSendToVnmrJ( pVnmrGph, message, messagesize)) != 0)
      return( -1 ); */
    }
    return(0);
}

int writeToVnmrJcmd(int argc, char *argv[], int retc, char *retv[])
{
   (void) retc;
   (void) retv;
   if (argc > 2)
   {
      writelineToVnmrJ( argv[1], argv[2] );
   }
   return(0);
}

/* how to do if it's Master? */
int vnmrj_close(int argc, char *argv[], int retc, char *retv[])
{
        (void) argv;
        (void) retc;
        (void) retv;
	if (Bnmr) RETURN;
	if (argc > 1) RETURN;
/*	if (isMaster == 0)
	  sendTripleEscToMaster( 'j', 0 ); */
/*	sync(); */ /* flush(); */
	closeSocket( &sVnmrJcom );
	closeSocket( &sVnmrJAl );
	closeSocket( &sVnmrGph );
        closeSocket( &sVnmrNet );
        closeSocket( &sVnmrPaint );
        RETURN;
}

#ifdef XXX
int sendToVnmrJclose(Socket *pVnmrJ )
{
	if (Bnmr) return(0);
	closeSocket( pVnmrJ );
        return( 0 );
}
#endif

#define JTREESIZE 3
int jExpressUse;
static int jExError;
static int jtree[ JTREESIZE ] = { CURRENT, GLOBAL, SYSTEMGLOBAL };
static int jEvalGlo = 0;

void resetjEvalGlo()
{
	jEvalGlo = 0;
}

static void jsendLayoutParam(int id, char *param ) 
{
	char layout[ MAXPATH ], layoutfile[ MAXPATH+20 ];
	char keyword[11] = "layoutpar";
	if (strcmp(param,"") != 0)
	{
	  if (P_getstring(CURRENT,param,layout,1,MAXPATH) < 0) 
	  { if (P_getstring(GLOBAL,param,layout,1,MAXPATH) < 0) 
	    { if (P_getstring(SYSTEMGLOBAL,param,layout,1,MAXPATH) < 0) 
	      {
		sprintf(layoutfile, "%d default", id);
		writelineToVnmrJ(keyword,layoutfile);
/* Winfoprintf("jsendLayoutParam: %s %s\n",keyword,layoutfile); */
		return;
	      }
	    }
	  }
	  strcpy(JtopPanelParam,param);
	  sprintf(layoutfile, "%d %s", id, layout);
	  writelineToVnmrJ(keyword,layoutfile);
/* Winfoprintf("jsendLayoutParam: %s %s\n",keyword,layoutfile); */
	}
}

#ifndef P_GLO
#define P_GLO 32768 /* bit 15 - global variable not sent to multiple Vnmrbg's */
#endif 
static void jAutoSendIfGlobal(char *param )
{
        int i, j, k, l, tree = NOTREE;
        int num;
        char mstr[MAXPATH+1], paramb[MAXPATH+1];
	char *plist;
        double dval;
        vInfo info;

        if (P_getVarInfo(GLOBAL, param, &info)==0)
          tree = GLOBAL;
        else if (P_getVarInfo(SYSTEMGLOBAL, param, &info)==0)
          tree = SYSTEMGLOBAL;
        if (tree == NOTREE)
	    return;
	if (info.prot & P_GLO) /* set bit on above params */
	    return;
        if (info.size > 256)
	    return;
        num = 0;
	plist = (char *)newStringId("","vjpglo");
        if (info.basicType == T_STRING)
        {
            mstr[0] = '\0';
            if (info.size == 1)
            {
	      if (P_getstring( tree, param, mstr, 1, MAXPATH) == 0)
                  num = 1;
/*	      sprintf(paramb,"%s=\\\'%s\\\'",param,mstr); */
	      sprintf(paramb,"%s=\\\'",param);
	      l = strlen(paramb);
	      j = strlen(mstr);
	      for (i=0; i<j; i++)
	      {
		if (mstr[i] == '\'') /* check for internal quotes */
		{
		  paramb[l++] = '\\';
		  paramb[l++] = '\\';
		  paramb[l++] = '\\';
		}
		paramb[l++] = mstr[i];
	      }
	      paramb[l] = '\0';
	      strcat(paramb,"\\\'");
	      plist = (char *)newCatId(plist,paramb,"vjpglo");
            }
            else
            {
	      plist = (char *)newCatId(plist,param,"vjpglo");
              for (k=0; k<info.size; k++)
              {
	        if (P_getstring( tree, param, mstr, k+1, MAXPATH) != 0)
                     break;
		if (k != 0)
		  plist = (char *)newCatId(plist, "," ,"vjpglo");
		else
		  plist = (char *)newCatId(plist, "=" ,"vjpglo");
/*	        sprintf(paramb,"\\\'%s\\\'",mstr); */
		strcpy(paramb,"\\\'");
		l = strlen(paramb);
		j = strlen(mstr);
		for (i=0; i<j; i++)
		{
		  if (mstr[i] == '\'') /* check for internal quotes */
		  {
		    paramb[l++] = '\\';
		    paramb[l++] = '\\';
		    paramb[l++] = '\\';
		  }
		  paramb[l++] = mstr[i];
		}
		paramb[l] = '\0';
		strcat(paramb,"\\\'");
		plist = (char *)newCatId(plist,paramb,"vjpglo");
                num = num + l + 2;
                if (num > 2400)
                    break;
              }
            }
        }
        else /* info.basicType == T_REAL */
        {
            if (info.size == 1)
            {
	      P_getreal( tree, param, &dval, 1);
	      sprintf(paramb,"%s=%g",param,dval);
	      plist = (char *)newCatId(plist,paramb,"vjpglo");
              num = 1;
            }
            else
            {
	      plist = (char *)newCatId(plist,param,"vjpglo");
              for (k=0; k<info.size; k++)
              {
	        if (P_getreal( tree, param, &dval, k+1) != 0)
                   break;
		if (k != 0)
		  plist = (char *)newCatId(plist, "," ,"vjpglo");
		else
		  plist = (char *)newCatId(plist, "=" ,"vjpglo");
	        sprintf(paramb,"%g",dval);
	        plist = (char *)newCatId(plist,paramb,"vjpglo");
                num++;
              }
            }
        }
/* Winfoprintf("echo: pglo %s\n",plist); */
        if (num > 0)
	     writelineToVnmrJ("pglo",plist);
	releaseWithId("vjpglo");
}

static void addToJvarlist(char *param)
{
	if ( (param[0] != '$') && (strcmp(param,jexprDummyVar)!=0) &&
	     (strcmp(param,"parmax")!=0) && (strcmp(param,"parmin")!=0) &&
	     (strcmp(param,"parstep")!=0) )
	{
	  char paramb[ MAXPATH+2 ];
	  sprintf(paramb," %s ",param);
	  if (strstr(Jx_string,paramb)==0)
	  {
	    Jx_string = (char *)newCatId(Jx_string,param,"vjpnew");
	    Jx_string = (char *)newCatId(Jx_string," ","vjpnew");
	  }
	}
}

static void set_vnmrj_acqdim()
{
    double   adim;
    varInfo *vinfo;
    extern double get_acq_dim();

    if (P_getVarInfoAddr(CURRENT,"acqdim") == (varInfo *) -1)
    {
      P_creatvar(CURRENT,"acqdim",T_REAL);
      if ((vinfo = P_getVarInfoAddr(CURRENT,"acqdim")) != (varInfo *) -1)
        vinfo->subtype = ST_INTEGER;
    }
    adim = get_acq_dim();
    P_setreal(CURRENT, "acqdim", adim, 1);
    if (acqflag == FALSE)
        addToJvarlist("acqdim");
    //  writelineToVnmrJ("pnew","1 acqdim"); /* need to append esc and value */
/* called from within jAutoRedisplayList(), don't appendvarlist */
}
static void jAddParamEsc(char *paramb, int len)
{
  int i, j;
  char tmp[MAXPATH];

  i = 0; j = 0;
  while ((i < len) && (j < MAXPATH-3))
    tmp[j++] = paramb[i++];
  tmp[j++] = escChar;
  tmp[j++] = escChar;
  tmp[j] = '\0';
  strncpy(paramb,tmp,j+1);
}
static void jFindParamValue(char *param, char *paramb)
{
	vInfo info;
	double dval;
	int found = 0;
	int k;

	for (k=0; k<JTREESIZE; k++)
	{
	  if (P_getVarInfo(jtree[k], param, &info)==0)
	  {
	    found = 1;
	    break;
	  }
	}
	if (found == 0)
	  strcpy(paramb,"---");
        else if (info.size > 1)
	  strcpy(paramb,"array");
	else
	{
	  if (info.basicType == T_REAL)
	  {
	    if (P_getreal(jtree[k], param, &dval, 1) < 0)
	      strcpy(paramb,"E@@ERR");
	    else
	      sprintf(paramb,"%g",dval);
	  }
	  else if (info.basicType == T_STRING)
	  {
	    if (P_getstring(jtree[k], param, paramb, 1, MAXPATH) < 0)
	      strcpy(paramb,"E@@ERR");
	  }
	}
	jAddParamEsc(paramb, strlen(paramb));
}

void appendJvarlist(const char *name)
{
	int i,j=0,k,len;
	char param[ MAXPATH ];
	if (Jx_string == NULL)
	  Jx_string = (char *)newStringId("    ","vjpnew");
	len = strlen(name);
	for (i=0; i<len; i++)
	{
	  if (name[i]==',' || name[i] == ' ')
	  {
	    param[j++] = '\0';
            if (j > 1)
	       addToJvarlist( param );
            j = 0;
	    for (k=j-1; k>=0; k--)
	      param[k] = '\0';
	  }
	  else
	  {
	    param[j++] = name[i];
	  }
	}
	param[j] = '\0';
        if (j > 0)
	   addToJvarlist( param );
}
void releaseJvarlist()
{
	if (Jx_string)
	{
	  releaseWithId("vjpnew");
	  Jx_string = NULL;
	}
}
void appendTopPanelParam()
{
	appendJvarlist(JtopPanelParam);
	appendJvarlist("seqfil");
}
void jAutoRedisplayList()
{
	char param[ MAXPATH ], paramb[ MAXPATH+2 ];
	char *vlist, *pch;
	int i, j, k, len, listsize=0, sendit=0;
	int layout=0, ni_flg=0;
	char tbuf[8];
	int batchit;

	if (P_getstring( GLOBAL, "appmode", param, 1, MAXPATH ) < 0)
	{
	  Werrprintf("WARNING: Could not find parameter 'appmode'.\n");
	  releaseJvarlist();
	  return;
	}
	else
	{
	  if (strcmp(param,"imaging")==0)
	    iplan_pnewUpdate(Jx_string);
	  j = strlen(param) + 1;
	  while (j>=0) param[j--] = '\0';
	}

	len = strlen(Jx_string);
	sprintf(param,"%c%c",escChar,escChar);
	vlist = (char *)newStringId(param,"vjpnew");
	if (len > 4) /* leading spaces */
	{
	  k = 0;
	  pch = Jx_string;
	  while ((k<len) && (*pch==' ')) /* skip leading spaces */
	    { pch++; k++; }
	  while (k<len)
	  {
	    j = sscanf(pch,"%s",param); /* return value of sscanf is +1/-1 but not consistent */
	    if (param[0] != '\0')
	    {
	      jFindParamValue(param, paramb);
	      vlist = (char *)newCatId(vlist,paramb,"vjpnew");
	      sendit = 1;
	      listsize++;
	      if (jEvalGlo == 0) /* send value(s) if global */
	        jAutoSendIfGlobal( param );
	      if (strcmp(param,JtopPanelParam)==0) /* update parameter panel */
	        layout = 1;
/*	      if (strcmp(param,"array")==0)
	        jsendArrayInfo(); */
	      if ((strcmp(param,"ni")==0) || (strcmp(param,"ni2")==0) ||
	          (strcmp(param,"ni3")==0) || (strcmp(param,"nD")==0))
	      {
	        ni_flg = 1;
	      }
	    }
	    j = strlen(param) + 1;
	    pch += j;
	    k += j;
	    i = j;
	    while (i>=0)
	      param[i--] = '\0';
	  }
	}
	pch = NULL;
	jEvalGlo = 0;
	if (sendit > 0)
	{
	    /* We will have VJ batch parameter changes if we have
	     * more than 10 parameters changing at a time.
	     */
	  batchit = 0;
	  if ((listsize > 10) && (P_getstring(CURRENT,"batch",tbuf,1,sizeof(tbuf))) == 0) {
	      batchit = ((*tbuf == 'y') && (listsize > 10));
	  }
	  if (layout == 1)
	  {
/* Winfoprintf("echo layout: pnew %s",Jx_string); */ /* vnmrj handles this */
	  }
	  if (ni_flg == 1)
	    set_vnmrj_acqdim();
	  Jx_string = (char *)newCatId(Jx_string,vlist,"vjpnew");
	  param[0] = '\0';
	  sprintf(param,"%d",listsize);
	  if (listsize > 999) strcpy(param,"999");
	  for (k=0; k< (int) strlen(param); k++)
	    Jx_string[k] = param[k];
	  if (batchit)
	      writelineToVnmrJ("vnmrjcmd","batch pnew on");
	  writelineToVnmrJ("pnew",Jx_string);
	  if (batchit)
	      writelineToVnmrJ("vnmrjcmd","batch pnew off");
if (jtesttest > 1)
  Winfoprintf("echo: pnew %s\n",Jx_string);
	}
	release(vlist);
	releaseJvarlist();
}

/* always send active status w/async? */
/* sync 0=sync, 1=async */
/* no_dg 0=use Dgroup, 1=don't use Dgroup */
static void jsendParam(char *param, int sync, int no_dg )
{
	char paramval[ MAXPATH ]; /* length of string parameter */
	char mstr[ MAXPATH ];     /* tmp storage */
	char jstr[ 4*MAXPATH ];   /* output to VnmrJ */
	char mkey[ 6 ];		   /* must include '\0' */
	vInfo info;
	double dval, value;
	int i, iter, found=0;

	if (sync==0)
	  strcpy(mkey,"pars");
	else
	  strcpy(mkey,"para");
/* parse param for spaces? */
	for (i=0; i<JTREESIZE; i++)
	{
	  if (P_getVarInfo(jtree[i], param, &info)==0)
	  {
	    found = 1;
	    break;
	  }
	}
	if (found==0)
	{
	  if (sync==0)
	    strcpy(jstr, "e NOFIND");
	  else
	    sprintf(jstr, "%s e NOFIND", param);
	  writelineToVnmrJ( mkey, jstr );
	  Werrprintf("jsendParam: %s not found\n",param);
	  return;
	}
	iter = i;
	if (info.basicType == 1) /* double */
	{
/* if arrayed, send all values */ 
	  if (info.size>1)
	  {
	    if (sync==0)
	      sprintf(jstr,"t %d",info.size);
	    else
	      sprintf(jstr,"%s t %d",param,info.size);
	  }
	  for (i=0; i<info.size; i++)
	  {
	    P_getreal( jtree[iter], param, &dval, i+1);
	    if (no_dg == 0)
	    {
	      switch( info.Dgroup )
	      {
	        case 2:
	          if (strcmp(param,"temp")==0)
	            dval += 273.0;
	          else if (info.subtype==6)
	            dval *= 1e-3;
	          else
	            dval *= 1e3;
	          break;
	        case 3:
	          if (info.subtype==6)
	            dval *= 1e-6;
	          else
	            dval *= 1e6;
	          break;
	        case 4:
	          if ((P_getreal(CURRENT,"reffrq",&value,1))==0)
	            dval /= value;
	          break;
	        case 5:
	          if ((P_getreal(CURRENT,"reffrq1",&value,1))==0)
	            dval /= value;
	          break;
	        case 6:
	          if ((P_getreal(CURRENT,"reffrq2",&value,1))==0)
	            dval /= value;
	          break;
	        default:
		  break;
	      }
	    }
	    if (info.size>1)
	    {
	      sprintf(mstr," %g",dval);
	      strcat(jstr,mstr);
	    }
	  }
	  if (info.size==1)
	  {
	    if (sync==0)
	      sprintf(jstr, "r %g", dval);
	    else
	      sprintf(jstr, "%s r %g", param, dval);
	    writelineToVnmrJ( mkey, jstr );
	  }
	  else
	  {
	    writelineToVnmrJ( mkey, jstr );
	  }
	}
	else if (info.basicType == 2) /* string */
	{
	  for (i=0; i<info.size; i++)
	  {
	    P_getstring( jtree[iter], param, paramval, i+1, MAXPATH );
	    if (info.size==1)
	    {
	      if (sync==0)
	        sprintf(jstr, "s %s", paramval);
	      else
	        sprintf(jstr, "%s s %s", param, paramval);
	      writelineToVnmrJ( mkey, jstr );
	    }
	    else
	    {
	      if (sync==0)
	        sprintf(jstr, "u %d %d %s", info.size, i, paramval);
	      else
	        sprintf(jstr, "%s u %d %d %s", param, info.size, i, paramval);
	      writelineToVnmrJ( mkey, jstr );
	    }
	  }
	}
	else
	{
	  Werrprintf("jsendParam: %s in tree=%d type=%d NOT real or string",
	    param,jtree[iter],info.basicType);
	  if (sync==0)
	    strcpy(jstr, "e NOTYPE");
	  else
	    sprintf(jstr, "%s e NOTYPE", param);
	  writelineToVnmrJ( mkey, jstr );
	  return;
	}
}
/* send max, min, step of param to VnmrJ */
void jsendParamMaxMin(int num, char *param, char *tree )
{
	char jstr[ MAXPATH ];   /* output to VnmrJ */
	double maxv, minv, stepv;
	char *type;
	int i, found=0;
	vInfo info;

        (void) tree;
	for (i=0; i<JTREESIZE; i++) /* ignore tree, search in all trees */
	{
	  if (P_getVarInfo(jtree[i], param, &info)==0)
	  {
	    found = 1;
	    break;
	  }
	}
	if (found==0)
	{
	  if (num)
	    sprintf(jstr, "%d m E@@ERR E@@ERR E@@ERR", num);
	  else
	    sprintf(jstr, "m E@@ERR E@@ERR E@@ERR");
	}
	else
	{
          if (info.prot & P_MMS)
          {
            if (P_getreal( SYSTEMGLOBAL, "parmax", &maxv, (int)(info.maxVal+0.1) ))
              maxv = 1.0e+30;
            if (P_getreal( SYSTEMGLOBAL, "parmin", &minv, (int)(info.minVal+0.1) ))
              minv = -1.0e+30;
            if (P_getreal( SYSTEMGLOBAL, "parstep", &stepv, (int)(info.step+0.1) ))
              stepv = 0.0;
          }
          else
          {
            maxv = info.maxVal;
            minv = info.minVal;
            stepv = info.step;
          }
	  if (num == -1)
	    sprintf(jstr, "%s m %g %g %g", param, maxv,minv,stepv);
	  else
	  {
	    if (num)
	      sprintf(jstr, "%d m %g %g %g", num, maxv,minv,stepv);
	    else
	      sprintf(jstr, "m %g %g %g", maxv,minv,stepv);
	  }
	}
	if (num == -1)
	  type = "plim";
	else
	  type = num == 0 ? "pars" : "para";
	writelineToVnmrJ( type, jstr );
}

static void vnmrj_express_real(char *mstr, char *format, double dval )
{
  char rstr[8];
  int ig;
  if (strcmp(format,"")==0)
    sprintf(mstr, "%g", dval);
  else
  {
    if (!isReal(format))
      sprintf(mstr, "%g", dval);
    else
    {
      ig = atoi( format );
      if (ig < 0)
        sprintf(mstr, "%g", dval);
      else
      {
        sprintf(rstr, "%%.%df", ig);
/* could use sprintf(rstr, "%%.%de", ig); if > 1e5 or < 1e-4 */
        sprintf(mstr, rstr, dval);
      }
    }
  }
}
static void vnmrj_express_string(char *mstr )
{
  char mstr2[MAXPATH];
  int i, j;
  /* Substitute '\n' for any line feeds */
  for (i=j=0; i<= (int) strlen(mstr); i++, j++) {
    if (mstr[i] != '\n') {
	mstr2[j] = mstr[i];
    } else {
	mstr2[j++] = '\\';
	mstr2[j] = 'n';
    }
  }
  mstr2[sizeof(mstr2)-1] = '\0';
  strcpy(mstr, mstr2);
}

static void jsendArrayVal(char *key, int exnum, char *param, char *format )
{
	char  *jstr;
	char   rstr[ MAXPATH ];
	int    i, iter, found=0;
	double dval;
	vInfo  info;

	strcpy(rstr,"");
	jstr = (char *)newStringId("","vjarray");
/* remove spaces from param if they exist?? */
	for (i=0; i<JTREESIZE; i++)
	{
	  if (P_getVarInfo(jtree[i], param, &info)==0)
	  {
	    found = 1;
	    break;
	  }
	}
	if (found==0)
	{
	  sprintf(rstr,"%d z NOVALUE", exnum);
	  writelineToVnmrJ( key, rstr );
	  return;
	}
	iter = i;
	switch (info.basicType)
	{
	  case T_REAL:
	      {
	        sprintf(rstr, "%d r %d ", exnum, info.size);
		jstr = (char *)newCatId(jstr,rstr,"vjarray");
	        for (i=0; i<info.size; i++)
	        {
	          P_getreal( jtree[iter], param, &dval, i+1);
	          vnmrj_express_real(rstr, format, dval);
		  jstr = (char *)newCatId(jstr,rstr,"vjarray");
		  if(i<info.size-1)
		    jstr = (char *)newCatId(jstr," ","vjarray");
	        }
		writelineToVnmrJ( key, jstr );
	      }
	      break;
	  case T_STRING:
	      {
	        sprintf(rstr, "%d s %d ", exnum, info.size);
		jstr = (char *)newCatId(jstr,rstr,"vjarray");
	        for (i=0; i<info.size; i++)
	        {
	          P_getstring( jtree[iter], param, rstr, i+1, MAXPATH );
	          vnmrj_express_string( rstr );
		  jstr = (char *)newCatId(jstr,rstr,"vjarray");
		  if(i<info.size-1)
		    jstr = (char *)newCatId(jstr,"; ","vjarray");
		}
	        writelineToVnmrJ( key, jstr );
	      }
	      break;
	  default:
	      {
	        sprintf(rstr,"%d u %d NOTYPE", exnum, info.basicType);
		writelineToVnmrJ( key, rstr );
	      }
	      break;
	}
	releaseWithId("vjarray");
}

#define letter(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z'))||((c)=='_')||((c)=='$')||((c)=='#'))
#define digit(c) (('0'<=(c))&&((c)<='9'))
#define NIL 0
#define CSPACE 0x20
#define LPRIN 0x28
#define RPRIN 0x29
#define COMMA 0x2C
// this function sends "array" param info (arraylist) to vnmrj
static void jsendArrayInfo(int exnum )
{
	char param[MAXPATH];
	char arrayval[MAXPATH];
	char arraylist[MAXPATH*3 + 30];
	int i, j, found, len, rank = 1, rank_inc = 1, elem_ct = 0;
	double dval;
	vInfo info;
        int ni_rank=0, ni2_rank=0, ni3_rank=0;

	strcpy(arrayval,"");
	strcpy(arraylist,"");
	sprintf(arraylist,"%d ",exnum);
	if (P_getstring( CURRENT, "array", arrayval, 1, MAXPATH ) < 0)
	{
	  strcat(arraylist,"E@@ERR");
	  writelineToVnmrJ("ARRAY",arraylist);
	  return;
	}
	if (arraytests())
	{
	  strcat(arraylist,"E@@ERR");
	  writelineToVnmrJ("ARRAY",arraylist);
	  return;
	}

	if (strcmp(arrayval,"") != 0)
	{
	  len = strlen(arrayval);
	  arrayval[len] = '\0';
	  len++;
	  j = 0;
	  param[j] = '\0';
	  for (i=0; i<len; i++)
	  {
	    switch (arrayval[i])
	    {
		case LPRIN:
		  rank_inc = 0;
		  break;
		case RPRIN:
		  rank_inc = 1;
		  break;
		case COMMA: case NIL:
		  param[j] = '\0';
		  found = 1;
		  for (j=0; j<JTREESIZE; j++)
		  {
	  	    if (P_getVarInfo(jtree[j], param, &info)==0)
	  	    {
		      found = 0;
	  	      break;
	  	    }
		  }
		  if (found == 0)
		  {
		    int is1D=1; 
	  	    if (P_getreal(CURRENT,"ni",&dval,1)==0) is1D = (dval <= 1);

		    // if d2, d3, d4, then save rank, but don't add to arraylist
		    // d2, d3, d4 will be replaced by ni,ni2, ni3 below
		    if(!is1D && strstr(param,"d2")!=NULL) ni_rank=rank;
		    else if(!is1D && strstr(param,"d3")!=NULL) ni2_rank=rank;
		    else if(!is1D && strstr(param,"d4")!=NULL) ni3_rank=rank;
		    else if(info.size > 1) { // add only if param is arrayed
		      strcat(arraylist,param);
		      strcat(arraylist," ");
		      sprintf(param,"%d ",rank);
		      strcat(arraylist,param);
/* send size - send with parameters also */
		      sprintf(param,"%d ",(int)info.size);
		      strcat(arraylist,param);
		    }

		    rank += rank_inc;
		    elem_ct++;
		    j = 0;
		    param[j] = '\0';
		    strcpy(param,"");
		  }
		  break;
		default:
/*		  if (letter(arrayval[i]) || digit(arrayval[i])) */
		  param[j++] = arrayval[i];
		  break;
	    }
	  }
	}

/* send value instead of size for ni, etc. */
/* don't send if value is 0 or 1 */
/* do a for loop to get parameters */

// now add implicitly arrayed param ni,ni2,ni3 with rank saved above.
	for (i=0; i<NUM_INDIRECT_PARS; i++)
	{
	  switch (i)
	  {
		case 0:
	          strcpy(param,"ni3");
		  rank=ni_rank;
		  break;
		case 1:
	          strcpy(param,"ni2");
		  rank=ni2_rank;
		  break;
		case 2:
	          strcpy(param,"ni");
		  rank=ni3_rank;
		  break;
		default:
	          strcpy(param,"ni");
		  rank=ni_rank;
		  break;
          }
	  if (P_getreal(CURRENT,param,&dval,1)==0)
	    if (dval > 1.5)
	    {
	      strcat(arraylist,param);
	      sprintf(param," %d %g ",rank,dval);
	      strcat(arraylist,param);
	    }
	}

	writelineToVnmrJ( "ARRAY", arraylist );
}

// this function sets "array" param based on msg from vnmrj
static void jrecvArrayInfo(char *msg )
{
	int i, j, k, len, cmode, pdone, badparam;
	int *rlist, elemct1, elemct = 0;
	char *plist, param[MAXPATH], acmd[2*MAXPATH];
	vInfo info;
/* msg must be of the form: 'param1 rank1 param2 rank2 ...'
   e.g. 'pw 1 p1 2 d2 3'  or  'pw 1 p1 1 d2 2'
   rank MUST BE in ascending order, else it fails
*/

	len = strlen( msg );
	msg[len] = '\0';
	len++;
	pdone = 1;
	for(i=0; i<len; i++)
	{
	  switch (msg[i])
	  {
	    case CSPACE: case NIL:
		if (pdone == 1)
		{
		  pdone = 0;
		  elemct++;
		}
		break;
	    default:
		pdone = 1;
		break;
	  }
	}
	elemct /= 2;
	if (elemct > 0)
	{
	  if ((rlist = (int *)malloc(sizeof(int) * elemct))==NIL)
	  {
	    Werrprintf("jrecvArrayInfo: error allocating memory\n");
	    return;
	  }
	  if ((plist = (char *)malloc(sizeof(char) * (len-1)))==NIL)
	  {
	    Werrprintf("jrecvArrayInfo: error allocating memory\n");
	    free((char *)rlist);
	    return;
	  }
	  for (i=0; i<elemct; i++)
	    rlist[i] = -2;
	  strcpy(plist,"");
	  badparam = 0; /* whether parameter exists or not */
	  cmode = 0; /* cmode indicates whether to read param or rank */
	  pdone = 0; /* pdone indicates whether read param is done */
	  k = 0;
	  j = 0;
	  param[j] = '\0';
	  for(i=0; i<len; i++)
	  {
	    switch (msg[i])
	    {
	      case CSPACE: case NIL:
	        if (pdone == 1)
	        {
		  param[j] = '\0';
		  if (cmode==0)
		  {
		    if ((badparam = P_getVarInfo(CURRENT, param, &info)) == 0)
		    {
		      if(info.size>1 || strcmp(param,"ni")==0 ||
			strcmp(param,"ni2")==0 || strcmp(param,"ni3")==0 ) {
		        strcat(plist,param);
		        strcat(plist," ");
		      } else if (elemct > 1) { // skip if param size 1
                        elemct -= 1;
		      }
		    }
		    cmode=1;
		  }
		  else
		  {
		    if (badparam != 0)
		    {
		      badparam = 0;
		      if (elemct > 1)
		        elemct -= 1;
		    }
		    else
		    {
/* note if atoi string is 0, function returns 0, so r=-1 */
		      if (atoi(param))
		        rlist[k] = atoi(param);
		      else
		        rlist[k] = -1;
		      k++;
		    }
		    cmode=0;
		  }
		  pdone = 0;
		  j = 0;
		  param[j] = '\0';
	        }
	        break;
	      default:
	        param[j++] = msg[i];
	        pdone = 1;
	        break;
	    }
	  }
	  if (strlen(plist) > 0)
	  {
	    strcpy( acmd, "array='" );
	    elemct1 = elemct - 1;
	    cmode = 0; /* cmode indicates whether to add paren or not */
	    i = 0;
	    for (k=0; k<elemct; k++)
	    {
/* if needed, add left paren */
	      if (rlist[k] > 0)
	      {
	        if ((k < elemct1) && (cmode==0))
	        {
	          if (rlist[k] == rlist[k+1])
	          {
		    strcat( acmd, "(" );
		    cmode = 1;
	          }
	        }
	      }
/* find param, add to list */
	      while (plist[i] == CSPACE)
	        i++;
	      j = 0; /* find variable */
	      param[j] = '\0';
	      while ((plist[i] != CSPACE) && (plist[i] != NIL))
	      {
	        param[j++] = plist[i++];
	      }
	      param[j] = '\0';
	      while (plist[i] == CSPACE)
	        i++;
	      if (rlist[k] > 0) { // add param to "array" if rank>0
		// replace implicitly arrayed param ni, ni2, ni3 with d2, d3, d4
		if(strcmp(param,"ni")==0) strcat( acmd, "d2" );
		else if(strcmp(param,"ni2")==0) strcat( acmd, "d3" );
		else if(strcmp(param,"ni3")==0) strcat( acmd, "d4" );
	        else strcat( acmd, param );
	      }
/* if needed, add right paren */
	      if ((rlist[k] > 0) && (cmode == 1))
	      {
	        if (k == elemct1)
	        {
	          strcat( acmd, ")" );
	          cmode = 0;
	        }
	        else if (rlist[k+1] != rlist[k])
	        {
	          strcat( acmd, ")" );
	          cmode = 0;
	        }
	      }
/* if needed, add comma */
	      if ((rlist[k] > 0) && (k < elemct1))
	        if (rlist[k+1] != -1)
	          strcat( acmd, "," );
	    }
	    strcat( acmd, "'\n" );
	    execString( acmd );
	  }
	  free((char *)rlist);
	  free((char *)plist);
	}
}

/* execute expression ($VALUE = ...) or any command */
static void jShowExpression(int exnum, char *express, char *format )
{
	char jstr[ 8*MAXPATH ];
/*
	char rstr[ MAXPATH ];
*/
	char key[ 8 ] = "shcond";
	varInfo *vinfo;
	int varokay;

	jExpressUse = 1;
	jExError = 0;
	sprintf(showStr, "%d E@@ERR", exnum);
	strcpy(jstr, express);
	strcat(jstr, "\n");
        pushTree();
	execString( jstr );
	if (jExError == 0)
	{
	  varokay = 0;
	  if ( (vinfo=findVar("$SHOW"))  == NULL)
          {
	    if ( (vinfo=findVar("$ENABLE"))  == NULL)
	    {
	      if ( (vinfo=findVar(jexprDummyVar))  == NULL)
	      {
		varokay = 1;
		if (jEvalGlo == 0)
		    Winfoprintf("Warning: %s not found: %s\n", jexprDummyVar, express);
		sprintf(showStr, "%d E@@NO_EXEC", exnum);
	      }
	    }
	  }
	  if (varokay == 0)
	  {
		switch (vinfo->T.basicType)
		{
		  case T_REAL:
		    { double dval;
		      char mstr[MAXPATH];
		    /* if (info.size>1) {...} */
		    dval = vinfo->R->v.r;
		    vnmrj_express_real(mstr, format, dval);
		    sprintf(showStr, "%d %s", exnum, mstr);
		    }
		    break;
		  case T_STRING:
		    { char mstr[MAXPATH];
		      /* if (info.size>1) {...} */
		      strncpy(mstr, vinfo->R->v.s, MAXPATH);
		      vnmrj_express_string(mstr);
		      sprintf(showStr, "%d %s", exnum, mstr);
		    }
		    break;
		  default:
		    sprintf(showStr, "%d  ", exnum);
		    break;
		}
	  }
	}
	else
	{
	  sprintf(showStr, "%d 0", exnum);
	}
        popTree();
 if (jtesttest > 0)
  Winfoprintf("echo: %s %s\n", key, showStr);
        if (!annotate)
	    writelineToVnmrJ( key, showStr );
	jExpressUse = 0;
}

/* execute expression ($VALUE = ...) or any command */
static void jExecExpression(char *key, char *exnum, char *express, char *format )
{
    char jstr[ 8*MAXPATH ];
    char dumstr[ 8*MAXPATH ];
/*
    char rstr[ MAXPATH ];
*/
    varInfo *vinfo = NULL;
    int simpleExpress = 0;
    int ret;
    int doPop = 0;

    /*{
	struct timeval tv;
	gettimeofday(&tv, 0);
	fprintf(stderr,"%d.%03d: jExecExpression start\n",
		tv.tv_sec, tv.tv_usec/1000);
    } */ /*TIMING*/

    jExError = 0;
    ret = sscanf(express,"%[^ =]%*[ =]%s%s",evalStr,jstr,dumstr);
    if ( (ret == 2) && !strcmp(evalStr,jexprDummyVar) && ( (vinfo=findVar(jstr))  != NULL) )
    {
       simpleExpress = 1;
    }
    else
    {
       if ((ret == 2) && !strcmp(evalStr,jexprDummyVar))
       {
          char tstStr[8*MAXPATH ];
          char *tstPtr;

          strcpy(tstStr,jstr);
          tstPtr = strtok(tstStr, " */+-()");
          while ((tstPtr != NULL) && !simpleExpress )
          {
             if ( ((vinfo=findVar(tstPtr)) != NULL) && (vinfo->T.size > 1) )
             {
                simpleExpress = 1;
             }
             else
             {
                tstPtr = (char*) strtok(NULL," */+-()");
             }
          }
       }
       if ( ! simpleExpress)
       {
          jExpressUse = 1;
          sprintf(evalStr, "%s E@@ERR", exnum);
          clearVar(jexprDummyVar);
          strcpy(jstr, express);
          strcat(jstr, "\n");
    /*    Winfoprintf("execString: %s", jstr); */
          pushTree();
          execString( jstr );
          doPop = 1;
       }
    }

    /* if jexprDummyVar exists while Syntax error, set error flag */
    if (jExError)
    {
      if (strcmp(key,"vloc")==0)
	sprintf(evalStr, "%s ", exnum);
      else
	sprintf(evalStr, "%s ---", exnum);
    } else {
	if (!simpleExpress && (vinfo=findVar(jexprDummyVar))  == NULL) {
	    if (jEvalGlo == 0) {
		Winfoprintf("Warning: %s not found: %s\n",
			    jexprDummyVar, express);
	    }
	    sprintf(evalStr, "%s E@@NO_EXEC", exnum);
	} else {

	    /* aval doesn't quite work, what we need is to pass
	     * parameter name also, have it execute an expression
	     * with $param[i] substituted.  plus using it with
	     * vnmrunits macro won't work */
	    if (strcmp(key,"aval")==0)
	    {
	        switch (vinfo->T.basicType)
	        {
		  case T_REAL:
		      { double dval;
		      char mstr[MAXPATH];
		      int i;
		      Rval *p;
		      int size;

		      size = vinfo->T.size;
		      sprintf(evalStr, "%s r %d", exnum, size);
		      /* if (size>1) {...} */
		      for (i=0, p=vinfo->R; i<size && p; i++, p=p->next)
		      {
			  dval = p->v.r;
			  vnmrj_express_real(mstr, format, dval);
			  /* conversion routine does not work on arrayed
			   * variables with expressions, such as
			   * $VALUE=nt/2, since $VALUE does not end up
			   * being an arrayed parameter even if nt is
			   * arrayed */
			  strcat(evalStr," ");
			  strcat(evalStr,mstr);
		      }
		      /* Winfoprintf("echo: %s %s\n", key, evalStr); */
		      }
		      break;
		  case T_STRING:
		      { char mstr[MAXPATH];
		      char tmpstr[MAXPATH];
		      int i;
		      Rval *p;
		      int size;

		      size = vinfo->T.size;
		      /* if (size>1) {...} */
		      strcpy(tmpstr, "");
		      for (i=0, p=vinfo->R; i<size && p; i++, p=p->next)
		      {
			  strncpy(mstr, p->v.s, MAXPATH);
			  vnmrj_express_string(mstr);
			  strcat(tmpstr, mstr);
			  if(i<size-1) strcat(tmpstr, "; ");
		      }
		      sprintf(evalStr, "%s s %d %s", exnum, size, tmpstr);
                      if (!annotate)
		         writelineToVnmrJ( key, evalStr );
		      /* Winfoprintf("echo: %s %s\n", key, evalStr); */
		      }
		      clearVar(jexprDummyVar);
		      jExpressUse = 0;
                      if (doPop)
                         popTree();
		      return;
		      break;
		  default:
		    sprintf(evalStr, "%s  ", exnum);
		    break;
		}
	    }
	    else /* key != "aval" (array value) */
	    {
		switch (vinfo->T.basicType)
		{
		  case T_REAL:
		      { double dval;
		      char mstr[MAXPATH];
		      if (vinfo->T.size>1)
                      {
                         sprintf(evalStr, "%s array", exnum);
                      }
                      else
                      {
		         dval = vinfo->R->v.r;
		         vnmrj_express_real(mstr, format, dval);
		         sprintf(evalStr, "%s %s", exnum, mstr);
                      }
		      }
		      break;
		  case T_STRING:
		      { char mstr[MAXPATH];
		      if (vinfo->T.size>1)
                      {
                         sprintf(evalStr, "%s array", exnum);
                      }
                      else
                      {
		         strncpy(mstr, vinfo->R->v.s, MAXPATH);
		         vnmrj_express_string(mstr);
		         sprintf(evalStr, "%s %s", exnum, mstr);
                      }
		      }
		      break;
		  default:
		    sprintf(evalStr, "%s  ", exnum);
		    break;
		}
	    }
	}
    }
    if (doPop)
        popTree();
    if (!simpleExpress)
       clearVar(jexprDummyVar);

    if (jtesttest > 0)
	Winfoprintf("echo: %s %s\n", key, evalStr);
    if (!annotate)
        writelineToVnmrJ( key, evalStr );
    jExpressUse = 0;

    /*{
	struct timeval tv;
	gettimeofday(&tv, 0);
	fprintf(stderr,"%d.%03d: jExecExpression done: %s %s\n",
		tv.tv_sec, tv.tv_usec/1000, key, evalStr);
    } */ /*TIMING*/

}

/* evaluate real expression, set equal to $VALUE */
static void jEvaluateExpression(int exnum, char *express )
{
	char jstr[ 8*MAXPATH ];
	char rstr[ MAXPATH ];

	jExpressUse = 1;
	jExError = 0;
	sprintf(rstr, "%d E@@ERR", exnum);
	  sprintf(jstr, "%s=", jexprDummyVar);
	  strcat(jstr, express);
	  strcat(jstr, "\n");
          pushTree();
	  execString( jstr );
	  if (jExError == 0)
	  {
	    varInfo *vinfo;
	    if ((vinfo=findVar(jexprDummyVar))  != NULL)
	    {
	      if (vinfo->T.basicType == T_REAL)
	      { /* if (info.size>1) {...} */
		double dval;
		dval = vinfo->R->v.r;
		sprintf(rstr, "%d %g", exnum, dval);
	      }
	      else if (vinfo->T.basicType == T_STRING)
	      { /* if (info.size>1) {...} */
		char mstr[MAXPATH];
		strncpy(mstr, vinfo->R->v.s, MAXPATH-6);
		sprintf(rstr, "%d %s", exnum, mstr);
	      }
	    }
	    else
	    {
	      sprintf(rstr, "%d E@@NO_EXEC", exnum);
	    }
	  }
	  else
	  {
	    Werrprintf("Error in template: %s", express);
	    sprintf(rstr, "%d E@@ERR", exnum);
	  }
        popTree();
	writelineToVnmrJ( "expr", rstr );
	jExpressUse = 0;
}

void
jExpressionError() /* if jexprDummyVar exists, error from evaluating dg-expr */
{		   /* if jExpressUse==1, error from evaluating dg-expr */
/*	vInfo info;
	if (P_getVarInfo(CURRENT,jexprDummyVar,&info)==0)
	  jExError = 1;
	else
	  jExError = 0;
*/
	if (jExpressUse == 1)
	  jExError = 1;
	else
	  jExError = 0;
}

static void flushGlobal(int ret)
{
	char parampath[MAXPATH];
	int diskIsFull, ival;
	strcpy(parampath,userdir);
	strcat(parampath,"/global");
	if (P_save(GLOBAL,parampath))
	{
	  ival = isDiskFullFile( userdir, parampath, &diskIsFull );
	  if (ival == 0 && diskIsFull)
	    Werrprintf("problem saving global parameters: disk is full");
	  else
	    Werrprintf("problem saving global parameters");
	  return;
	}
/* also write out conpar if vnmr1? */
        if (ret > 0)
	   writelineToVnmrJ("flush","startv");
/* Winfoprintf("echo: flush startv\n"); */
}

static int open_terminal(int to_open, char *f)
{
	if (xtfd != NULL) {
	    fclose(xtfd);
	    xtfd = NULL;
	}
        if (f == NULL)
	    return(0);
        if (strncmp(f, "/dev/", 5) == 0) {
            if (access(f, W_OK) != 0) {
               Werrprintf("Could not open %s.", f);
               return (0);
            }
        }
	if (to_open <= 0) {
            if (access("/dev/null", W_OK) != 0)
	       return(0);
            sendTripleEscToMaster( 'X', "/dev/null");
	    xtfd = fopen("/dev/null", "a");
        }
        else {
            sendTripleEscToMaster( 'X', f);
	    xtfd = fopen(f, "w");
        }
	if (xtfd != NULL) {
	    dup2(fileno(xtfd), 2);
            fprintf(stderr, "terminal %s opened. \n", f);
            return (1);
	}
        Werrprintf("Could not open %s.", f);
        return (0);
}

int vnmrjcmd(int argc, char *argv[], int retc, char *retv[])
{
	char *jstr;
	int   i;

        (void) retc;
        (void) retv;
	if (argc < 2)
            RETURN;
        if (strcmp(argv[1], "pnew")==0)
        {
            /*********
	    if (argc > 2)
	    {
	      char tmpstr[ MAXPATH ];
	      sprintf(tmpstr, "%d ", argc - 2);
	      jstr = (char *)newCatId(jstr,tmpstr,"vjcmd");
	      for (i=2; i<argc; i++)
	      {
	        jstr = (char *)newCatId(jstr,argv[i],"vjcmd");
	        jstr = (char *)newCatId(jstr," ","vjcmd");
	      }
	      writelineToVnmrJ("pnew", jstr);
	    }
            *********/
	    for (i=2; i<argc; i++)
                appendJvarlist(argv[i]); 
            RETURN;
	}

	if (strcmp(argv[1], "tty")==0)
	{
	    if (argc > 2) /* same as jFunc(55,..) */
	      open_terminal(1, argv[2] );
	    else
	      open_terminal(0, "null" );
            RETURN;
        }

	jstr = (char *)newStringId("","vjcmd");
	  // If updateexpsel, use "," as delimiter between args
	if (argc > 2 && strcmp(argv[2], "updateexpsel")==0) {
	        for (i=1; i<argc; i++)
	        {
	          jstr = (char *)newCatId(jstr,argv[i],"vjcmd");
	          jstr = (char *)newCatId(jstr,",","vjcmd");
	        }	      
	        writelineToVnmrJ( argv[0], jstr );
	}
	else /* not pnew */
	{
	    for (i=1; i<argc; i++)
	    {
	      jstr = (char *)newCatId(jstr,argv[i],"vjcmd");
	      jstr = (char *)newCatId(jstr," ","vjcmd");
	    }
            /*  if (begstrcmp(argv[1],"window")==0)
	           flushGlobal(1); */
	    writelineToVnmrJ( argv[0], jstr );
            /* Winfoprintf("echo: %s %s\n",argv[0],jstr); */
	}
	releaseWithId("vjcmd");
	return 0;
}


int isimagebrowser(int argc, char *argv[], int retc, char *retv[])
{
   char jstr[128];
   int aip;

   (void) argc;
   (void) argv;
   aip = (aipOwnsScreen()) ? 1 : 0;
   if (retc)
   {
      retv[0] = realString((double) aip);
   }
   else
   {
      sprintf(jstr,"isimagebrowser %d",aip);
      writelineToVnmrJ( "vnmrjcmd", jstr );
   }
   RETURN;
}

/* start jFunc, could be separate file */

/* static int  debug = 0; */
#define PEXIT     	1
#define PCLOSE    	2
#define PDEBUG    	3
#define GSYNC     	4
#define WSIZE     	5
#define JMOUSE    	6
#define JMENU     	7
#define JCOLORS   	8
#define JEXECVAL	9
#define JEXPRESS  	10
#define JPARMAXMIN	11
#define JPARAM    	12
#define JPARAMDG	13
#define JSHOWCOND       14
#define JEXECVALGLOBAL  15
#define XWINID          16
#define WEXECDISP	17
#define FLUSHVNMR       18
#define UPDATEPANEL     19
#define SETVIEWID       20
#define JPAINT 		21
#define FONTSIZE 	22
#define REDRAW 		23
#define XEVENT 		24
#define JSENDARRAYINFO	25
#define JRECVARRAYINFO  26
#define JSENDARRAYMENU  27
#define JEXECARRAYVAL	28
#define SHOWINPUT	29
#define XMOVE           30
#define XREGION         32
#define XWINID2         33
#define JPAINT2         34
#define WSIZE2     	35
#define JSHOWARRAY	36
#define XMASK		37
#define XMASK2		38
#define XCOPY		39
#define PANELPARAM	40
#define JPNEWLOC	41
#define XREGION2        42
#define FRMSIZE         43
#define FRMID           44
#define FRMLOC          45
#define OVLYTYPE        46
#define SCRNDPI         47
#define JPRINT          48
#define JSYNC           49
#define PSIZE           50
#define FRMSTATUS       51
#define JSCROLLWHEEL    52
#define AIPMENU         53
#define APPLYCMP        54
#define XTERM     	55
#define IMGORDER     	57
#define SELECTIMG     	58
#define SETIMGINFO     	59
#define TABLEACT     	60  // vj canvas table changed
#define PENTHICK     	62
#define LINEWIDTH     	63
#define SPECWIDTH     	64
#define XDEBUG     	66
#define JPAINTALL 	67
#define CSIDISP 	68
#define TRANSPARENT     69
#define TRANSPARENT2    70
#define CSIOPENED     	71
#define SAVESPECDATA   	72
#define SPECSTYLE   	77
#define CSIWINDOW 	88
#define FRM_FONTSIZE 	89
#define FONTARRAY 	90
/* #define JACQDONETEST	97 */
#define JTESTTEST	98
#define ERRORKEYTEST	99

#define G3D  100
#define CMDLINEOK  101
#define RETEVAL  102
#define FVERTICAL 103

#define JGRAPH 	1
#define VPORT 	2
#define VHOST 	3
#define GDONE   5

#define CLOSEALL     0
#define CLOSEGRAPH   1
#define CLOSECOMMON  2
#define CLOSEALPHA   3
#define SENDVNMRADDR 4

struct _attr {
              char   *name;
              int    type;
              char   *sval;
            };

static struct _attr pattrs[] = {
        { "printdpi", ST_STRING, "300"},
        { "printsavedpi", ST_STRING, "150"},
        { "printlw", ST_STRING, "1" },
        { "printfile", ST_STRING, NULL },
        { "printsend", ST_STRING, "file" },
        { "papersize", ST_STRING, "letter" },
        { "printformat", ST_STRING, "ps" },
        { "printsaveformat", ST_STRING, "jpg" },
        { "printlayout", ST_STRING, "portrait" },
        { "printcolor", ST_STRING, "mono" },
        { "printsavecolor", ST_STRING, "color" },
        { NULL, ST_STRING, NULL }
      };

static int jxFunc(int func, int argc, char *argv[])
{
        int     i, k, x, y, w, h;
	double  fv;

       switch (func) {
               case JPNEWLOC:  // 41x
                    if (argc > 2) {
                         char mstr[MAXPATH], key[6]="vloc";
                         if (argc > 4)
                         {
                            sprintf(mstr, "%s=%s", jexprDummyVar, argv[3]);
                            jExecExpression( key, argv[2], mstr, argv[4] );
                         }
                         else if (argc > 3)
                         {
                            sprintf(mstr, "%s=%s", jexprDummyVar, argv[3]);
                            jExecExpression( key, argv[2], mstr, "" );
                         }
                         else if (argc > 2)
                         {
                            sprintf(mstr, "%s=%s", jexprDummyVar, argv[2]);
                            jExecExpression( key, argv[2], mstr, "" );
                         }
                     }
		     break;
               case XREGION2:  // 42x
                     if (argc > 5) {
                          x = atoi(argv[2]);
                          y = atoi(argv[3]);
                          i = atoi(argv[4]);
                          k = atoi(argv[5]);
                          set_win_region(2, x, y, i, k);
                     }
		     break;
               case FRMSIZE:  // 43x
                     if (argc > 6) {
                          i = atoi(argv[2]);
                          x = atoi(argv[3]);
                          y = atoi(argv[4]);
                          w = atoi(argv[5]);
                          h = atoi(argv[6]);
                          set_hourglass_cursor();
                          set_jframe_size(i, x, y, w, h);
                          restore_original_cursor();
                     }
		     break;
               case FRMID:  // 44x
                     if (argc > 2) {
                          i = atoi(argv[2]);
                          set_active_frame(i);
                     }
		     break;
               case FRMLOC:  // 45x
                     if (argc > 6) {
                          i = atoi(argv[2]);
                          x = atoi(argv[3]);
                          y = atoi(argv[4]);
                          w = atoi(argv[5]);
                          h = atoi(argv[6]);
                          set_jframe_loc(i, x, y, w, h);
                     }
		     break;
               case OVLYTYPE:  // 46x
                     k = 0;
                     if (argc > 17) {
                          jset_overlaySpecInfo(argv[2], atoi(argv[3]), argv[4], atoi(argv[5]), atoi(argv[6]),
                                atoi(argv[7]), atof(argv[8]), atof(argv[9]), atof(argv[10]), atof(argv[11]),
                                atof(argv[12]), atof(argv[13]), atof(argv[14]), atof(argv[15]),argv[16],argv[17]);
                     } else if (argc > 2) {
                          k = atoi(argv[2]);
                          jset_overlayType(k);
                     }
		     break;
               case SCRNDPI:  // 47x
                     if (argc < 3)
                          return(1);
                     k = atoi(argv[2]);
                     if (k < 0)
                          return(1);
                     if (P_getreal(GLOBAL, "screendpi", &fv, 1) < 0) {
                           P_creatvar(GLOBAL,"screendpi",ST_REAL);
                           P_setlimits(GLOBAL,"screendpi", (double)1200, 0.0, 1.0);
                     }
                     P_setreal(GLOBAL,"screendpi",(double)k, 1);
                     i = 0;
                     while (pattrs[i].name != NULL) {
                        if (P_getstring(GLOBAL,pattrs[i].name,tmpStr,1,MAXPATH) < 0)
                        {
                             if (P_getreal(GLOBAL, pattrs[i].name, &fv, 1) >= 0)
                                 P_deleteVar(GLOBAL, pattrs[i].name);
                             P_creatvar(GLOBAL, pattrs[i].name, ST_STRING);
                             if (pattrs[i].sval != NULL)
                                 P_setstring(GLOBAL,pattrs[i].name, pattrs[i].sval,0);
                        }
                        i++;
                     }
		     break;
               case JPRINT:  // 48x
                     printjimage(argc, argv);
		     break;
               case JSYNC:  // 49x
                     k = 0;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     jvnmr_sync(k, 0);
		     break;
               case PSIZE:  // 50x
                     if (argc > 6) {
                         i = atoi(argv[2]);
                         x = atoi(argv[3]);
                         y = atoi(argv[4]);
                         w = atoi(argv[5]);
                         h = atoi(argv[6]);
                         set_pframe_size(i, x, y, w, h);
                     }
		     break;
               case FRMSTATUS:  // 51x
                     if (argc > 3) {
                         i = atoi(argv[2]);
                         k = atoi(argv[3]);
                         set_jframe_status(i, k);
                     }
		     break;
               case JSCROLLWHEEL:  // 52x
                     if (argc > 2) {
                         set_hourglass_cursor();
                         jscroll_wheel_mouse( atoi(argv[2]), 0);
                         restore_original_cursor();
                     }
		     break;
               case AIPMENU:  // 53x
                     if (argc > 2)
                         exec_aip_menu(atoi(argv[2]));
		     break;
               case APPLYCMP:  // 54x
                     k = 0;
                     if (argc < 5)
                         return(1);
                     k = atoi(argv[2]);
                     i = atoi(argv[3]);
                     open_color_palette_from_vj(k,i, argv[4]);
		     break;
               case XTERM:  // 55x
                     if (argc > 2)
                         open_terminal(1, argv[2] );
                     else
                         open_terminal(0, "null" );
		     break;
               case IMGORDER:  // 57x
                     if (argc < 4)
                        return(1);
                     i = atoi(argv[2]);
                     k = atoi(argv[3]);
                     change_aip_image_order(i, k);
		     break;
               case SELECTIMG:  // 58x
                     if (argc < 5)
                        return(1);
                     i = atoi(argv[2]);
                     k = atoi(argv[3]);
                     w = atoi(argv[4]);
                     select_aip_image(i, k, w);
		     break;
               case SETIMGINFO:  // 59x
                     if (argc < 8)
                        return(1);
                     x = atoi(argv[2]);
                     y = atoi(argv[3]);
                     k = atoi(argv[4]);
                     w = atoi(argv[5]);
                     set_aip_image_info(x, y, k, w, argv[6],atoi(argv[7]));
		     break;
               case TABLEACT:  // 60x
                     if (argc < 5)
                        return(1);
                     jTable_changed(argc - 1, argv+1);
		     break;
               case PENTHICK:  // 62x
                     k = 0;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     set_pen_width(k);
		     break;
               case LINEWIDTH:  // 63x
                     k = 0;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     set_line_width(k);
		     break;
               case SPECWIDTH:  // 64x
                     k = 0;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     set_spectrum_width(k);
		     break;
               case XDEBUG:  // 66x   graphics debug
                     i = 0;
                     k = 0;
                     if (argc > 2)
                        i = atoi(argv[2]);
                     if (argc > 3)
                        k = atoi(argv[3]);
                     toggle_graphics_debug(i, k);
		     break;
               case JPAINTALL:  // 67x
                     repaint_canvas(0);
		     break;
               case CSIDISP:  // 68x
                     k = 1;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     set_csi_display(k);
		     break;
               case TRANSPARENT:  // 69x
                     if (argc > 3) {
                         k = atoi(argv[2]);
                         fv = atof(argv[3]);
                         vj_set_transparency_level(k, fv);
                     }
		     break;
               case TRANSPARENT2:  // 70x
                     if (argc > 2) {
                         fv = atof(argv[2]);
                         set_transparency_level(fv);
                     }
		     break;
               case CSIOPENED:  // 71x
                     k = 0;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     set_csi_opened(k);
		     break;
               case SAVESPECDATA:  // 72x
                     x = 1;
                     if (argc > 2)
                        x = atoi(argv[2]);
                     show_spec_raw_data(x);
		     break;
               case SPECSTYLE:  // 77x
                     x = 1;
                     if (argc > 2)
                        x = atoi(argv[2]);
                     set_ybar_style(x);
		     break;
	       case XRESUME:  // 80x
                     if (argc > 2)
			  jSuspendGraphics( atoi(argv[2]) );
		     break;
               case CSIWINDOW:  // 88x
                     k = 1;
                     if (argc > 2)
                         k = atoi(argv[2]);
                     clear_aip_menu();
                     if (argc > 3)
                         set_csi_mode(k, argv[3]);
                     else
                         set_csi_mode(k, "h");
		     break;
               case FRM_FONTSIZE:  // 89x
                     if (argc > 6)
                         set_frame_font_size(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
		     break;
               case FONTARRAY:  // 90x
                     if (argc > 7)
                         set_vj_font_array(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
		     break;
               case JTESTTEST:  // 98x just for testing
                     if (argc > 2)
                         Wtesttest( argv[2] );
                     else
                         Wtesttest( "0" );
		     break;
               case ERRORKEYTEST:  // 99x
                     if (argc > 2)
                        Wseterrorkey( argv[2] );
                     else
                        Wseterrorkey( "oi" );
		     break;
               case G3D:  // 100x
                     {
                        extern void g3dcmd(int,int);
                        int value=0;
                        int code=0;
                        if (argc > 2)
                             code=atoi(argv[2]);
                        if (argc > 3){
                             if(strcmp(argv[3],"true")==0 || strcmp(argv[3],"yes")==0)
                                        value=1;
                             else if(strcmp(argv[3],"false")==0 || strcmp(argv[3],"no")==0)
                                        value=0;
                             else
                                        value=atoi(argv[3]);
                         }
                         g3dcmd(code,value);
                     }
		     break;
               case CMDLINEOK:  // 101x
                     if (argc > 2)
                         cmdLineOK = atoi(argv[2]);
		     break;
               case FVERTICAL:  // 103x
                     if (argc < 3)
                        return(1);
                     i = atoi(argv[2]);
                     k = 0;
                     if (argc > 3)
                        k = atoi(argv[3]);
                     set_jframe_vertical(i, k);
		     break;
               default:
                     return (0);
       }
       return(1);
}

int jFunc(int argc, char *argv[], int retc, char *retv[])
{
        int     i, k, x, y, w, h, kcmd;

       (void) retc;
       (void) retv;
#if defined (IRIX) || (AIX)
	Werrprintf("jFunc not enabled for SGI and IBM ");
	RETURN;
#else 
        doThisCmdHistory = 0;
        if (argc < 2)
            RETURN;
        kcmd = atoi(argv[1]);
 	if (kcmd <= 0)
		RETURN;
 	if (kcmd >= JPNEWLOC) {
            if (jxFunc(kcmd, argc, argv))
	        RETURN;
        }

        switch (kcmd) {
                case JMOUSE:  // 6
			processJMouse(argc, argv);
			jvnmr_sync(0, 0);
			break;
                case PEXIT:  // 1
			nmrExit(0,NULL,0,NULL);
			break;
                case PCLOSE:  // 2
/*			sync(); */ /* flush(); */
			i = CLOSEALL;
			if (argc > 2)
			{
			  i = atoi(argv[2]);
			  if (isMaster == 0)
			    sendTripleEscToMaster( 'j', "0" ); /* does not work?? */
			}
			if ((i < CLOSEALL) || (i > CLOSEALPHA))
			  i = CLOSEALL;
			switch (i)
			{
			  case CLOSEGRAPH:
			    closeSocket( &sVnmrGph );
			    sVnmrGph.sd = -1;
                            if (sVnmrPaint.sd >= 0)
			        closeSocket( &sVnmrPaint );
			    sVnmrPaint.sd = -1;
			    break;
			  case CLOSECOMMON:
			    closeSocket( &sVnmrJcom );
			    sVnmrJcom.sd = -1;
			    break;
			  case CLOSEALPHA:
			    closeSocket( &sVnmrJAl );
			    sVnmrJAl.sd = -1;
			    break;
			  case SENDVNMRADDR:
			    break;
			  case CLOSEALL: default:
			    closeSocket( &sVnmrJcom );
			    closeSocket( &sVnmrJAl );
			    closeSocket( &sVnmrGph );
			    closeSocket( &sVnmrJaddr );
                            closeSocket( &sVnmrNet );
                            closeSocket( &sVnmrPaint );
			    break;
			}
			break;
                case PDEBUG:  // 3
			if (debug > 0)
			   debug = 0;
			else
			   debug = 1;
			break;
                case GSYNC:  // 4
			jvnmr_sync(0, 0);
			break;
                case WSIZE:   // 5
                case WSIZE2:  // 35
			if (argc > 3)
			{
			  x = 0;
			  y = 0;
			  w = atoi(argv[2]);
			  h = atoi(argv[3]);
			  if (argc > 5) {
			     x = atoi(argv[4]);
			     y = atoi(argv[5]);
			  }
                          set_hourglass_cursor();
			  if (kcmd == WSIZE)
			     set_main_win_size(x, y, w, h);
			  else
			     set_win2_size(x, y, w, h);
                          restore_original_cursor();
			}
			break;
		case JMENU:   // 7
			if (argc > 2)
			  exec_vj_menu(atoi(argv[2]));
                        /****
			if (argc > 2)
			if (isMaster == 0)
			  sendTripleEscToMaster( 'J', argv[2] );
                        **/
/*			but_interpos_handler_jfunc(argv[2]); */
			break;
		case JCOLORS:   // 8   make color table
			init_colors();
			make_table();
			break;
		case JEXECVAL:  // 9
			jEvalGlo = 0;
			if (argc > 4)
			  jExecExpression( "eval", argv[2], argv[3], argv[4] );
			else if (argc > 3)
			  jExecExpression( "eval", argv[2], argv[3], "" );
			else if (argc > 2)
			  jExecExpression( "eval", "0", argv[2], "" );
			break;
		case JEXPRESS:  // 10
			if (argc > 3)
			  jEvaluateExpression( atoi(argv[2]), argv[3] );
			else if (argc > 2)
			  jEvaluateExpression( 0, argv[2] );
			break;
		case JPARMAXMIN:  // 11
			if (argc > 4)
			  jsendParamMaxMin(atoi(argv[2]),argv[3],argv[4]);
			else if (argc > 3)
			  jsendParamMaxMin(atoi(argv[2]),argv[3],NULL);
			break;
		case JPARAM:  // 12
			if (argc > 2)
			  jsendParam(argv[2], 0, 1);
			break;
		case JPARAMDG:  // 13
			if (argc > 2)
			  jsendParam(argv[2], 0, 0);
			break;
		case JSHOWCOND:  // 14
			jEvalGlo = 0;
			if (argc > 3)
			  jShowExpression( atoi(argv[2]), argv[3], "" );
			else if (argc > 2)
			  jShowExpression( 0, argv[2], "" );
			break;
		case JEXECVALGLOBAL:  // 15
                        /* send this one if sending global params to multiple Vnmr's */
			jEvalGlo = 1;
			if (argc > 4)
			  jExecExpression( "eval", argv[2], argv[3], argv[4] );
			else if (argc > 3)
			  jExecExpression( "eval", argv[2], argv[3], "" );
			else if (argc > 2)
			  jExecExpression( "eval", "0", argv[2], "" );
                       /* jEvalGlo = 0; */
			break;
		case XWINID:  // 16
			k = 0;
			if (argc > 3)
			  k = atoi(argv[3]);
			if (argc > 2)
			  jSetGraphicsWinId( argv[2], k );
			break;
		case XWINID2:  // 33
#ifdef MOTIF
			if (argc > 2) {
			  k = atoi(argv[2]);
			  jSetGraphicsWinId2((long)k);
			}
#endif
			break;
		case WEXECDISP: // 17
			Wexecgraphicsdisplay();
			break;
		case FLUSHVNMR:  // 18
			k = 1;
			if (argc > 2)
			  k = atoi(argv[2]);
			flushGlobal(k);
			break;
		case UPDATEPANEL:  // 19
                        /* Not used any more */
			break;
		case PANELPARAM:  // 40
			if (argc > 3)
			  jsendLayoutParam( atoi(argv[2]), argv[3]);
			else if (argc > 2)
			  jsendLayoutParam( 0, argv[2]);
			break;
		case SETVIEWID: // 20
			if (argc > 2)
			  VnmrJViewId = atoi( argv[2] );
			break;
		case JPAINT:  // 21
			repaintCanvas();
/*
			redrawCanvas();
*/
			break;
		case JPAINT2:  // 34
			redrawCanvas2();
			break;
		case FONTSIZE:  // 22
			if (argc > 5)
			  set_font_size( atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
			else if (argc > 3)
			  set_font_size( atoi(argv[2]), atoi(argv[3]), 0, 0);
			break;
		case REDRAW:  // 23
			window_redisplay();
			break;
		case XEVENT:  // 24
			if (argc < 6)
			    RETURN;
			jMouse(atoi(argv[2]), atoi(argv[3]),
				atoi(argv[4]), atoi(argv[5]));
			break;
		case JSENDARRAYINFO:  // 25
			if (argc > 2) {
			  jsendArrayInfo( atoi(argv[2]) );
			} else
			  jsendArrayInfo( 0 );
			break;
		case JRECVARRAYINFO:  // 26
			if (argc > 2) {
			  jrecvArrayInfo( argv[2] );
			}
			break;
		case JSENDARRAYMENU:   // 27
			jsendArrayMenu();
			break;
		case JEXECARRAYVAL:  // 28
			if (argc > 4) {
			  jsendArrayVal( "aval", atoi(argv[2]), argv[3], argv[4] );
			} else if (argc > 3) {
			  jsendArrayVal( "aval", atoi(argv[2]), argv[3], "" );
			} else if (argc > 2) {
			  jsendArrayVal( "aval", 0, argv[2], "" );
			}
			break;
		case SHOWINPUT:  // 29
			if (argc > 2)
			{
			  if (atoi(argv[2]))
			    jShowInput = atoi(argv[2]);
			  else
			    jShowInput = 0;
			}
			else
			  jShowInput = 0;
			break;
		case XSHOW:  // 31
			k = 1;
			if (argc > 2)
			  k = atoi(argv[2]);
			jSetGraphicsShow(k);
			break;
                case XCOPY:  // 39
			if (argc > 5)
			{
			  x = atoi(argv[2]);
			  y = atoi(argv[3]);
			  i = atoi(argv[4]);
			  k = atoi(argv[5]);
			  copy_from_pixmap(x, y, x, y, i, k);
			}
			break;
                case XREGION:  // 32
			if (argc > 5)
			{
			  x = atoi(argv[2]);
			  y = atoi(argv[3]);
			  i = atoi(argv[4]);
			  k = atoi(argv[5]);
			  set_win_region(1, x, y, i, k);
			}
			break;
		case JSHOWARRAY:  // 36
			if (argc > 2)
			{
			  jShowArray = atoi( argv[2] );
			  if (jShowArray != 0) jShowArray = 1;
			}
			else
			  jShowArray = 0;
			break;
/*		case JACQDONETEST:  // 97
			set_vnmrj_acqdone_times();
			break; */
 		case G3D:  // 100
		    {
 		    	extern void g3dcmd(int,int);
 		    	int value=0;
 		    	int code=0;
 			    if (argc > 2)
 			        code=atoi(argv[2]);
 			    if (argc > 3){
 			    	if(strcmp(argv[3],"true")==0 || strcmp(argv[3],"yes")==0)
 			    		value=1;
 			        else if(strcmp(argv[3],"false")==0 || strcmp(argv[3],"no")==0)
 			        	value=0;
 			        else
 			        	value=atoi(argv[3]);
 			    }
 			    g3dcmd(code,value);
 		    }
 			break;
                case XMASK:  // 37
			set_jmouse_mask(argc, argv);
			break;
                case XMASK2:  // 38
			set_jmouse_mask2(argc, argv);
			break;
                case RETEVAL:  // 38
			returnEval(argc, argv, retc, retv);
			break;
		default:
                        Werrprintf("Error: unknown jFunc key %d.\n", kcmd);
			break;
	}

	     /* {
		 struct timeval tv;
		 gettimeofday(&tv, 0);
		 fprintf(stderr," jFunc done: %d.%03d\n",
			 tv.tv_sec, tv.tv_usec/1000);
	     }TIMING */
#endif 
        RETURN;
}

// jFunc2 comes from VJ csicanvas
int jFunc2(int argc, char *argv[], int retc, char *retv[])
{
        int     i, k, x, y, w, h, kcmd;

       (void) retc;
       (void) retv;
#if defined (IRIX) || (AIX)
	Werrprintf("jFunc not enabled for SGI and IBM ");
	RETURN;
#else 
        doThisCmdHistory = 0;
        if (argc < 2)
            RETURN;
        kcmd = atoi(argv[1]);
 	if (kcmd <= 0)
		RETURN;
        switch (kcmd) {
                case PDEBUG:
			if (debug > 0)
			   debug = 0;
			else
			   debug = 1;
			break;
                case GSYNC:
			jvnmr_sync(0, 1);
			break;
                case WSIZE:
			if (argc > 3)
			{
			  x = 0;
			  y = 0;
			  w = atoi(argv[2]);
			  h = atoi(argv[3]);
			  if (argc > 5) {
			     x = atoi(argv[4]);
			     y = atoi(argv[5]);
			  }
			  if (kcmd == WSIZE)
			     set_csi_win_size(x, y, w, h);
			}
			break;
                case JMOUSE:
			process_csi_JMouse(argc, argv);
			jvnmr_sync(0, 1);
			break;
		case JCOLORS:
			break;
		case JEXPRESS:
			if (argc > 3)
			  jEvaluateExpression( atoi(argv[2]), argv[3] );
			else if (argc > 2)
			  jEvaluateExpression( 0, argv[2] );
			break;
		case JPAINT:
			// repaintCanvas();
			break;
		case JPAINT2:
			break;
		case JPAINTALL:
                        repaint_canvas(1);
			break;
		case FONTSIZE:
			if (argc > 5)
			  set_csi_font_size( atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
			else if (argc > 3)
			  set_csi_font_size( atoi(argv[2]), atoi(argv[3]), 0, 0);
			break;
		case REDRAW:
			// window_redisplay();
			break;
		case XEVENT:
			if (argc < 6)
			    RETURN;
			csi_jMouse(atoi(argv[2]), atoi(argv[3]),
				atoi(argv[4]), atoi(argv[5]));
			break;
		case XSHOW:
			k = 1;
			if (argc > 2)
			  k = atoi(argv[2]);
			// jSetGraphicsShow(k);
			break;
                case XCOPY:
			if (argc > 5)
			{
			  x = atoi(argv[2]);
			  y = atoi(argv[3]);
			  i = atoi(argv[4]);
			  k = atoi(argv[5]);
			  // copy_from_pixmap(x, y, x, y, i, k);
			}
			break;
                case FRMSIZE:
			if (argc > 6)
			{
			  i = atoi(argv[2]);
			  x = atoi(argv[3]);
			  y = atoi(argv[4]);
			  w = atoi(argv[5]);
			  h = atoi(argv[6]);
			  set_csi_jframe_size(i, x, y, w, h);
                        }
			break;
                case FRMLOC:
			if (argc > 6)
			{
			  i = atoi(argv[2]);
			  x = atoi(argv[3]);
			  y = atoi(argv[4]);
			  w = atoi(argv[5]);
			  h = atoi(argv[6]);
			  set_csi_jframe_loc(i, x, y, w, h);
                        }
			break;
                case FRMID:
			if (argc > 2)
			{
			  i = atoi(argv[2]);
			  set_csi_active_frame(i);
                        }
			break;
                case FRMSTATUS:
			if (argc > 3)
			{
			  i = atoi(argv[2]);
			  k = atoi(argv[3]);
			  set_csi_jframe_status(i, k);
                        }
			break;
                case JSCROLLWHEEL:
			if (argc > 2)
                        {
                          set_hourglass_cursor();
			  jscroll_wheel_mouse( atoi(argv[2]), 1);
                          restore_original_cursor();
                        }
			break;
		case XRESUME:
			break;
                case XMASK:
			set_csi_jmouse_mask(argc, argv);
			break;
                case XMASK2:
			set_csi_jmouse_mask2(argc, argv);
			break;
                case OVLYTYPE:
			k = 0;
			if (argc > 15) {
			  jset_csi_overlaySpecInfo(argv[2], atoi(argv[3]), argv[4], atoi(argv[5]), atoi(argv[6]), 
				atoi(argv[7]), atof(argv[8]), atof(argv[9]), atof(argv[10]), atof(argv[11]), 
				atof(argv[12]), atof(argv[13]), atof(argv[14]), atof(argv[15])); 
			} else if (argc > 2) { 
			  k = atoi(argv[2]);
			  jset_csi_overlayType(k);
  			}
			break;
                case SCRNDPI:
			break;
                case XDEBUG:
			i = 0;
			k = 0;
			if (argc > 2)
			  i = atoi(argv[2]);
			if (argc > 3)
			  k = atoi(argv[3]);
			toggle_graphics_debug(i, k);
			break;
                case JPRINT:
                        break;
                case JSYNC:
			k = 0;
			if (argc > 2)
			    k = atoi(argv[2]);
			jvnmr_sync(k, 1);
                        break;
                case PSIZE:
                        if (argc > 6)
                        {
                          i = atoi(argv[2]);
                          x = atoi(argv[3]);
                          y = atoi(argv[4]);
                          w = atoi(argv[5]);
                          h = atoi(argv[6]);
                          set_csi_pframe_size(i, x, y, w, h);
                        }
                        break;
		default:
			break;
	}

#endif 
        RETURN;
}

int jEvent(int argc, char *argv[], int retc, char *retv[])
{
   return(jFunc(argc, argv, retc, retv));
}

int jEvent2(int argc, char *argv[], int retc, char *retv[])
{
   return(jFunc2(argc, argv, retc, retv));
}

int jMove(int argc, char *argv[], int retc, char *retv[])
{
   (void) retc;
   (void) retv;
   if (argc > 3) {
      jvnmr_sync(0, 0);
      processJMouse(argc, argv);
      // jvnmr_sync(0, 0);
   }
   RETURN;
}

int jMove2(int argc, char *argv[], int retc, char *retv[])
{
   (void) retc;
   (void) retv;
   if (argc > 3) {
      jvnmr_sync(0, 1);
      process_csi_JMouse(argc, argv);
      // jvnmr_sync(0, 1);
   }
   RETURN;
}

int jMove3(int argc, char *argv[], int retc, char *retv[])
{
   return(jFunc(argc, argv, retc, retv));
}

int jRegion(int argc, char *argv[], int retc, char *retv[])
{
   return(jFunc(argc, argv, retc, retv));
}

int jRegion2(int argc, char *argv[], int retc, char *retv[])
{
   return(jFunc2(argc, argv, retc, retv));
}

int returnEval(int argc, char *argv[], int retc, char *retv[])
{
    if (argc != 3)
      RETURN;
    annotate = 1;
    jExecExpression( "eval", " ", argv[2], "" );
    annotate = 0;
    if (retc)
      retv[0] = newString(evalStr);
    else
      Wscrprintf("VALUE: %s",evalStr);
    RETURN;
}

char *AnnotateExpression(char *exp, char *format)
{
    annotate = 1;
    if (format == NULL) {
       jExecExpression( "eval", "00", exp, "" );
    }
    else {
       jExecExpression( "eval", "00", exp, format );
    }
    annotate = 0;
    return (evalStr);
}

char *AnnotateShow(char *exp)
{
    jEvalGlo = 0;
    annotate = 1;
    jShowExpression(0, exp, "");
    annotate = 0;
    return (showStr);
}

int serverport(int argc, char *argv[], int retc, char *retv[])
{
   (void) argc;
   (void) argv;
   if (retc > 0)
      retv[0] = intString(inPort);
   else
      Wscrprintf("serverport  %d\n", inPort);

   RETURN;
}

int cmdlineOK(int argc, char *argv[], int retc, char *retv[])
{
   (void) argc;
   (void) argv;

   // 3/31/10 the cmdline enable/disable was put into the "rights"
   // like other similar items.  For backwards compatibility, this
   // function is being changed to use the "right" enablecmdline
   int enablecmdline = rightsEval("enablecmdline");
   if (retc > 0)
   {
      if (enablecmdline)
         retv[0] = intString(enablecmdline);
      else if (argc > 1)
         retv[0] = newString(argv[1]);
      else
         retv[0] = intString(-1);
   }
   else
   {
      Winfoprintf("Command line is %s\n", (enablecmdline) ? "enabled" : "disabled");
   }

   RETURN;
}
