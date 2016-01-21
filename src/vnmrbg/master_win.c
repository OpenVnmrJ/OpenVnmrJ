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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef   _BSD_COMPAT
#undef   _BSD_COMPAT
#endif 
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#ifdef SOLARIS
#include <sys/filio.h>
#endif

#include <sys/stat.h>
#include <sys/socket.h>
#ifdef __INTERIX
#include <arpa/inet.h>
typedef int socklen_t;
#else
#include <netinet/in.h>
#endif
#include <netdb.h>
#include <fcntl.h>

#ifdef MOTIF
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#else
#include "vjXdef.h"
#endif 

#include "acquisition.h"
#include "smagic.h"
#include "socket1.h"
#include "wjunk.h"
#include "master_win.h"

static int fdAcq = -1;
static int fdFG  = -1;
static int inListener = -1;  /* the socket listener */
static int xwin  = 0;  /* X window flag */
static int acceptsocketfd = -1;

#define ITIMER_NULL	((struct itimerval *)0)


#define MAX_DTABLESIZE 64

#ifdef  DEBUG
extern  int    debug;
#define DPRINT0(str) \
	if (debug) fprintf(stderr,str)

#else 
#define DPRINT0(str)
#endif 

int             acqSock;
int            *acqSockPtr = &acqSock;

int shell_count=0;
int system_count=0;
static int    TimingFlag = 0; /* debugging flag for tracing shell commands */

static double shell_time=0;
static double max_time=0;
static double dtime1,dtime2,delta;
static FILE *shell_file=0;
static char max_str[256];
static char shell_str[256];

extern int 	psg_pid;
extern int      forgroundFdW;
extern int      forgroundPid;
extern int      isMaster;
extern int      Bserver;

extern void	gotInterupt2();
extern char    *nextCommand();
extern char    *previousCommand();
extern char   	Xserver[];
extern char    	Xdisplay[];
extern void     sleepMilliSeconds(int msecs);
extern void     sendChildNewLine();
extern int      get_vnmr_handle(int index);
extern void checkPipe();
extern void forgroundExpired(int pid);
extern void getTclInfoFileName(char tmp[], char addr[]);
extern void nmr_exit(char *modeptr);
extern void chewEachChar(char *buffer, int size);


/*  Counter to indicate number of commands that the master has
    sent to the child.  Only when this counter is 0 can messages
    from Acqproc be sent to the child.				*/

extern int             fgBusy;		/* Used by socket1.c    */

Display         *main_dpy;

#define      INFDS   7

XtInputId       inputpipe;
static int	fgPid = -1;
static int	numerrLines = 0;
static int	inFdList[INFDS];
static void child_exit_process(int signum);


typedef struct _vnmr_sig_vec {
            int   pid;
            int   sig;
            void  (*exit_func)(); /*  will be called when child exit */
            void  (*kill_func)(); /*  called for killing child */
            struct _vnmr_sig_vec *next;
        }  vnmr_sig_vec;

static  vnmr_sig_vec *sigList = NULL;

#ifdef MOTIF
typedef struct _xfd_rec {
            int   fd;
            XtInputId  xfd;
            struct _xfd_rec *next;
        }  xfd_rec;

static  xfd_rec *xfdList = NULL;
#endif

static int   showVnmrjAlphaDisplay = 0;

static FILE *AlphaFile = NULL;
static char AlphaFileName[256] = "/tmp/p";


static void init_alphafile()
{
	char addr[257];
	if (strcmp(AlphaFileName,"/tmp/p") == 0)
	{
	  GET_VNMR_ADDR(addr);
	  getTclInfoFileName(AlphaFileName,addr);
	  strcat(AlphaFileName,"_text");
	}
}

static int check_alphafile()
{
	if (AlphaFile == NULL)
	{
	  init_alphafile();
          writelineToVnmrJ("vnmrjcmd alphatext",AlphaFileName);
	  if ((AlphaFile = fopen(AlphaFileName, "w")) == NULL)
	     return( 0 );
	  else
	     return( 1 );
	}
	else
	  return( 1 );
}

static void close_alphafile()
{
	if (AlphaFile != NULL)
	    fclose(AlphaFile);
	AlphaFile = NULL;
}

static void remove_alphafile()
{
	init_alphafile(); /* needed since called from child, not master */
	if (access( AlphaFileName, 0 ) == 0)
	  unlink(AlphaFileName);
}

void unlink_alphafile()
{
        remove_alphafile();
        if (inListener >= 0)
            close(inListener);
        inListener = -1;
}

static void update_vnmrj_alphadisplay()
{
	showVnmrjAlphaDisplay = 1;
}

#ifdef MOTIF
static void add_xfd_rec (fd, xfd )
int   fd;
XtInputId  xfd;
{
       xfd_rec  *node, *pnode;

       if (xfdList == NULL)
       {
           /* create a dummy head node */
           xfdList = (xfd_rec *) malloc(sizeof(xfd_rec));
           if (xfdList == NULL)
               return;
           xfdList->fd = -9; 
           xfdList->xfd = 0; 
           xfdList->next = NULL; 
       }
       node = (xfd_rec *) malloc(sizeof(xfd_rec));
       if (node == NULL)
          return;
       node->fd = fd;
       node->xfd = xfd;
       node->next = NULL;
       pnode = xfdList;
       while (pnode->next != NULL) {
           pnode = pnode->next;
       }
       pnode->next = node;
}

static XtInputId del_xfd_rec (fd)
int   fd;
{
      XtInputId  xfd;
      xfd_rec  *node, *pnode;

      xfd = 0;
      if (xfdList == NULL)
          return(xfd);
      node = xfdList->next;
      pnode = xfdList;
      while (node != NULL) {
          if (node->fd == fd) {
             pnode->next = node->next;
             break;
          }
          pnode = node;
          node = node->next;
      }
      if (node != NULL) {
          xfd = node->xfd;
          free(node);
      }
      return (xfd);
}
#endif

static void add_input_fd (int fd)
{
     int  k;

     if (fd < 0)
         return;
     for (k = 0; k < INFDS; k++) {
         if (inFdList[k] < 0) {
             inFdList[k] = fd;
             return;
         }
     }
     close(fd);
}

static void close_input_fd (int fd)
{
     int  k;
     int stat;

     if (fd < 0)
         return;
     close(fd);
     if (fd == acceptsocketfd) /* fd to java program has closed */
        for (k=0; k < 10; k++)
        {
           stat = kill( getppid(), 0);  /* If java program is gone, exit */
           if (stat)
           {
              write( forgroundFdW, "vnmrexit", 8 );
              sendChildNewLine();
              sleepMilliSeconds(1000); /* wait for SIGCHLD */
              k = 11;
           }
           sleepMilliSeconds(10);
        }
     for (k = 0; k < INFDS; k++) {
         if (inFdList[k] == fd) {
             inFdList[k] = -1;
             return;
         }
     }
}


void show_vnmrj_alphadisplay()
{
	if (showVnmrjAlphaDisplay != 0)
	{
	  showVnmrjAlphaDisplay = 0;
	  writelineToVnmrJ("pnew","1 alphatext");
	}
}

void textprintf(char *fgReaderBuffer,int len)
{
        char format[10];

        if (len < 1)
             return;
        if (Bserver) {
             fprintf(stderr, "%s",fgReaderBuffer); 
             return;
        }
        sprintf(format,"%%%ds",len);

	if (check_alphafile())
        {
              fprintf(AlphaFile,format,fgReaderBuffer); 
              fflush(AlphaFile);
	      update_vnmrj_alphadisplay();
        }
}


void dispMessage(int item_no, char *msg, int loc_x, int loc_y, int width)
{
   if ((item_no > 0) && (item_no < 11))
   {
	char key[20];
	sprintf(key,"title titletext%d",item_no);
	writelineToVnmrJ(key,msg);
   }
   else if (Bserver) {
        fprintf(stderr, "%s\n", msg); 
        return;
   }
   if (loc_y == 2)
   {
	if (numerrLines >= 1)
	{
   	    if (strlen(msg) == 0) sprintf(msg, " \n");
	    textprintf(msg, strlen(msg));
	}
   }
}


void register_child_exit_func(int signum, int pid, void (*func_exit)(), void (*func_kill)())
{
	sigset_t		 qmask;
	struct sigaction	 intserv;
        vnmr_sig_vec  		*curNode, *newNode;

        if (sigList == NULL)
        {
           newNode = (vnmr_sig_vec *) calloc(1, sizeof(vnmr_sig_vec));
           if (newNode == NULL)
              return;
           newNode->pid = -1;
           sigList = newNode;
        }
        curNode = sigList;
        while ((curNode->pid != -1) && (curNode->next != NULL))
           curNode = curNode->next;
        if (curNode->pid == -1)
        {
           newNode = curNode;
        }
        else
        {
           newNode = (vnmr_sig_vec *) calloc(1, sizeof(vnmr_sig_vec));
           if (newNode == NULL)
              return;
           curNode->next = newNode;
        }
        newNode->sig = signum;
        newNode->pid = pid;
        newNode->exit_func = func_exit;
        newNode->kill_func = func_kill;

	sigemptyset( &qmask );
	sigaddset( &qmask, SIGCHLD );
	intserv.sa_handler = child_exit_process;
	intserv.sa_mask = qmask;
	intserv.sa_flags = 0;
	sigaction( SIGCHLD, &intserv, NULL );

        /*signal(signum, child_exit_process);*/
}

/*----------------------------------------------------------------------
|
|       button_interposition_handler
|
|       This routine is called when a button is clicked.  It figures
|       out what button has been clicked and calls the correct routine
|
+---------------------------------------------------------------------*/


void but_interpos_handler_jfunc(char *jstr )
{
    char sendstr[ 4 ];
    int  but_num;

    but_num = atoi(jstr);
    if (but_num < 1) but_num=1;

/*  If child not busy, start the function by sending <ESC>N, where N
    is the button number (indexed from 1); otherwise, put the escape
    sequence on the command queue.                                      */

    sprintf( &sendstr[ 0 ], "\033%d", but_num );
    if (fgBusy == 0) {
      write( forgroundFdW, &sendstr[ 0 ], (but_num > 9) ? 3 : 2 );
      sendChildNewLine();
    }
    else
      insertAcqMsgEntry( &sendstr[ 0 ] );
}

void removeButtons()
{
      writelineToVnmrJ("button","-1 removeButtons");
}

void setErrorwindow(char *tex)
{
	if (numerrLines < 1) {
   		dispMessage(0, "", 0, 2, 80);
	}
}

void setTextwindow(char *tex)
{
     if (strcmp("clear", tex) == 0)
     {
	close_alphafile();
	if (check_alphafile())
	       update_vnmrj_alphadisplay();
     }
}


void fileDump(char *file)
{
     char    buf[4098];
     int     len;
     FILE   *text_file;

     if ((text_file = fopen(file, "r")) == NULL)
        return;
     if (check_alphafile())
     {
        len = fread(buf, 1, 4096, text_file);
        while(len > 0)
        {
                buf[len] = '\0';
                fprintf(AlphaFile,"%s", buf);
                len = fread(buf, 1, 4096, text_file);
        }
        fflush(AlphaFile);
	update_vnmrj_alphadisplay();
     }
     fclose(text_file);
}


void buildButton(int bnum, char *label)
{
      char mstr[512];

      sprintf(mstr,"%d %s",bnum,label);
      writelineToVnmrJ("button",mstr);
}


void buildMainButton(int num, char *label, void (*func)())
{}

void ttyInput(char *tex, int len)
{}

void ttyFocus(char *tex)
{}

void kill_all_childs()
{
        vnmr_sig_vec  *curNode;

        curNode = sigList;
	while (curNode != NULL)
	{
		if (curNode->kill_func != NULL && curNode->pid >= 0)
			curNode->kill_func(curNode->pid);
		curNode = curNode->next;
	}
}

static void fgExpired(int pid)
{
     if (pid != fgPid)
	 return;
     kill_all_childs();
     forgroundExpired(fgPid);
     return;
}

void raise_text_window()
{ }


/*-------------------------------------------------------------------------
|    These routines change the label on the large small button. Not to
|    big a deal except we have to do it when the child task executes
|    a large or small command.
+--------------------------------------------------------------------------*/
static int      lArge = 1;	/* Large label is default */

void setLargeSmallButton(char *size)
{
   lArge = strcmp(size,"l") ? 0 : 1;
}


/*--------------------------------------------------------------------------
|	LargeSmall
|
|	This routine sends the large or small command to Vnmr
|
+-------------------------------------------------------------------------*/
#ifdef XXX
void largeSmall()
{
   if (lArge)			/* Then send large command */
   {
      write(forgroundFdW, "large", 5);
      sendChildNewLine();
      lArge = 0;
   }
   else				/* send small command */
   {
      write(forgroundFdW, "small", 5);
      sendChildNewLine();
      lArge = 1;
   }
}
#endif


void jNextPrev(int cmd )
{
    char  *p;

    p = NULL;
    if (cmd == 1)
	p = nextCommand();
    if (cmd == -1)
	p = previousCommand();
    if (p != NULL)
        writelineToVnmrJ("hist", p);
    else
        writelineToVnmrJ("hist", "");
}

static void
Interupt()
{
    remove_alphafile();
    gotInterupt2();
}

static void ExitYourself()
{
    kill_all_childs();
    remove_alphafile();
    gotInterupt2();
}

void ignoreSigpipe()
{
   struct sigaction	intserv;
   sigset_t		qmask;

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGPIPE );
   intserv.sa_handler = SIG_IGN;
   intserv.sa_mask = qmask;
   intserv.sa_flags = 0;
   sigaction(SIGPIPE, &intserv, NULL);
}

static void
catchIntSignals()
{
   struct sigaction	intserv;
   sigset_t		qmask;

 /* --- set up signal handler --- */

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGINT );
   intserv.sa_handler = Interupt;
   intserv.sa_mask = qmask;
   intserv.sa_flags = 0;
   sigaction(SIGINT, &intserv, NULL);
   /*signal(SIGINT, gotInterupt2);*/

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGTERM );
   intserv.sa_handler = ExitYourself;
   intserv.sa_mask = qmask;
   intserv.sa_flags = 0;
   sigaction(SIGTERM, &intserv, NULL);
   /*signal(SIGTERM, ExitYourself);*/

   sigemptyset( &qmask );
   sigaddset( &qmask, SIGALRM );
   intserv.sa_handler = SIG_IGN;
   intserv.sa_mask = qmask;
   intserv.sa_flags = 0;
   sigaction(SIGALRM, &intserv, NULL);
}


/*---------------------------------------------------------------------------
|  Window Notifier function to receive SIGUSR1 & SIGUSR2 signals from acqi
+--------------------------------------------------------------------------*/
void set_acqi_signal();

static void acqi_ack(int signal)
{
     if (signal == SIGUSR2)
     {
	 interact_connect_status();
     }
     set_acqi_signal();
     return;
}

void set_acqi_signal()
{
     struct sigaction	intserv;
     sigset_t		qmask;
 
     sigemptyset( &qmask );
     sigaddset( &qmask, SIGUSR2 );
     intserv.sa_handler = acqi_ack;
     intserv.sa_mask = qmask;
     intserv.sa_flags = 0;
     sigaction( SIGUSR2, &intserv, NULL );
}


/*---------------------------------------------------------------------------
|  Window Notifier function to receive SIGQUIT from acqproc
+--------------------------------------------------------------------------*/
void nmr_quit(int signal)
{
     if (signal == SIGQUIT)
        nmr_exit("");
     return;
}


void set_nmr_quit_signal()
{
     signal(SIGQUIT, nmr_quit);
}


/*---------------------------------------------------------------------------
|  Window Notifier function to receive SIGCHLD signals from child processes
|  of vnmr (i.e., PSG, acqi)
+--------------------------------------------------------------------------*/
static void Grim_Reaper(int pid)
{
        if (pid == psg_pid)  {
            DPRINT0("PSG terminated\n");
            psg_pid = 0;
            disp_acq("        ");
        } else{
            DPRINT0("interactive program terminated\n");
            interact_obituary(pid);
        }
}

void set_wait_child(int pid)
{
    register_child_exit_func(SIGCHLD, pid, Grim_Reaper, NULL);
/*
    signal(SIGCHLD, Grim_Reaper);
*/
}

void unlockPanel()
{}

void system_call(char *s)
{
	system_count++;
    system(s);
}

static int	popencall_pid = 0;	/* would replace with d/s, as explained */

FILE *popen_call(char *cmdstr, char *mode )
{
	int	 	 pipechild;
	int	 	 pipefd[ 2 ];
#ifdef XXX
	struct rlimit	 numfiles;
#endif
	FILE		*ptr_to_pipe;
	struct timeval	clock;

/*  Verify arguments;  verify mode is read.  verify no active popen process.	*/

	if (cmdstr == NULL || mode == NULL) {
		errno = EACCES;
		return( NULL );
	}
	if (*mode != 'r') {
		errno = EINVAL;
		return( NULL );
	}
	if (popencall_pid != 0) {
/*		fprintf( stderr, "Too many calls to popen_call\n" ); */
		return( NULL );
	}

	if (pipe( &pipefd[ 0 ] )) {
		return( NULL );
	}
	if(TimingFlag>0){
		gettimeofday( &clock, NULL );
		dtime1 = ( (double) (clock.tv_sec) + ((double) (clock.tv_usec)) / 1.0e6 );
		if(TimingFlag>1)
			strncpy(shell_str,cmdstr,255);
	}
#ifdef __INTERIX
	pipechild = vfork();
#else 
	pipechild = fork();
#endif 

	if (pipechild == -1) {
		return( NULL );
	}
	else if (pipechild == 0) {

	/*  This is the child process  */

		int	iter, ival, d_tablesize;

	/* redirect stdin to instant EOF (where else!)  */

		freopen("/dev/null","r",stdin);

	/*  redirect stdout to the write end of this pipe.  */

		if (close( 1 )) {
			_exit( 1 );
		}
		if (dup2( pipefd[ 1 ], 1 ) < 0) {
			perror( "dup2" );
			_exit( 1 );
		}
		close( pipefd[ 1 ] );	/* for appearance; would be closed shortly */
					    /* the read end will be closed shortly */
	/*  Close any open file descriptors,
	    except for stdin, stdout and stderr  */

#ifdef XXX
		ival = getrlimit( RLIMIT_NOFILE, &numfiles );
		if (ival < 0) {
			perror( "getrlimit" );
			_exit( 1 );
		}

		d_tablesize = numfiles.rlim_cur;
#endif 
		d_tablesize = MAX_DTABLESIZE;
		for (iter = 3; iter < d_tablesize; iter++)
		  close( iter );

		ival = execl( "/bin/sh", "sh", "-c", cmdstr, NULL );
		_exit( 1 );
	}

	/*  Parent process programming follows.  */

	else {				/* You must close the write end of the */
		close( pipefd[ 1 ] );	      /* pipe for the read end to work */
		popencall_pid = pipechild;
		ptr_to_pipe = fdopen( pipefd[ 0 ], "r" );
		register_child_exit_func(SIGCHLD, pipechild, Grim_Reaper, NULL);
/*
		signal(SIGCHLD, Grim_Reaper);
*/
		//shell_count++;
		if(TimingFlag>0)
			shell_file=ptr_to_pipe;
		return( ptr_to_pipe );
	}
}

/*  Note:  The program uses the standard buffered I/O facilities
           on the file pointer returned by this program.	*/

int pclose_call( FILE *pfile )
{
	int	ival;
	struct timeval	clock;

#ifndef LINUX
	kill( popencall_pid, SIGHUP );
#endif
	ival = fclose( pfile );
	popencall_pid = 0;
	if(TimingFlag>0 && pfile==shell_file){
		shell_count++;
		gettimeofday( &clock, NULL );
		dtime2 = ( (double) (clock.tv_sec) + ((double) (clock.tv_usec)) / 1.0e6 );
		delta=dtime2-dtime1;
		if(delta>max_time)
			strncpy(max_str,shell_str,255);
		fflush(stdout);
		fflush(stderr);
		if(TimingFlag>2)
			Winfoprintf("%2.1f\t%s\n",delta*1000,shell_str);
		else if(TimingFlag>1)
			fprintf(stderr,"%2.1f\t%s\n",delta*1000,shell_str);
		fflush(stdout);
		fflush(stderr);

		max_time=delta>max_time?delta:max_time;
		shell_time+=delta;
	}
	return( ival );
}


/*  Concerning WIFEXITED and WIFSIGNALED:

    The parent process receives a SIGCHLD signal when a child process exits
    normally, when a child process exits as the result of receiving a signal
    or when a change in the state of a child process occurs - it is stopped
    or it continues.   The latter occurs (for example) when you press ^Z to
    stop a process.

    This program needs to proceed with its processing if the child process
    has exited.  Unfortunately the WIFEXITED only refers to "normal" exit,
    i.e., not as the result of a signal.  You must also test WIFSIGNALED
    to include the latter case.							*/

static void child_exit_process(int signum)
{
        int  child_pid, status;
        vnmr_sig_vec  *curNode;

        curNode = sigList;
        status = 0;
        if (signum == SIGCHLD)
        {
#ifdef AIX
            child_pid = wait3(&status, WNOHANG, NULL);
#else 
            child_pid = waitpid(-1, &status, WNOHANG);
#endif 
	    if (WIFEXITED( status ) || WIFSIGNALED( status )) {
                while (curNode != NULL)
                {
                     if (child_pid == curNode->pid && signum == curNode->sig)
		     {
			curNode->pid = -1;
			if (curNode->exit_func != NULL)
			     curNode->exit_func(child_pid);
                        break;
		     }
                     curNode = curNode->next;
                }
             }
             /*signal(SIGCHLD, child_exit_process);*/
        }
}

static int acceptSocket(char tbuffer[], int len)
{
   int nchr;
   struct sockaddr from;
   socklen_t fromlen = sizeof(from);
   int flags, on, new_fd;

   on = 1;
   new_fd = accept(inListener,&from,&fromlen);
   if (new_fd < 0)
   {
         fprintf(stderr,"    AsynConnect(): No connection pending\n");
         return(0);
   }
   if ((flags = fcntl( new_fd, F_GETFL, 0 )) == -1)
   {
         fprintf(stderr,"set socket nonblocking, can't get current setting" );
         close(new_fd);
         return(0);
   }
   flags |= O_NDELAY;
   if (fcntl( new_fd, F_SETFL, flags ) == -1)
   {
         fprintf(stderr,"can't set socket to nonblocking" );
         close(new_fd);
         return(0);
   }
   acceptsocketfd = new_fd;
   add_input_fd (new_fd);
   nchr =  read(new_fd,tbuffer,len);
   if ( nchr > 0)
      tbuffer[nchr] = '\0';
   return(nchr);
}

void MasterLoop()
{
    int k;
    int fd;
    int r, nfds;
    fd_set  wr, er, rds;
    char buf1[1024];

#ifdef MOTIF
     if (xwin) {
        XtMainLoop();
        exit(0);
     }
#endif
     FD_ZERO(&wr);
     FD_ZERO(&er);
     for (;;) {
          FD_ZERO(&rds);
          nfds = 0;
          for (k = 0; k < INFDS; k++) {
              if (inFdList[k] >= 0) {
                  fd = inFdList[k];
                  FD_SET (fd, &rds);
                  if (fd > nfds)
                      nfds = fd;
              }
          }
          r = select (nfds + 1, &rds, &wr, &er, NULL);

               if (r == -1 && errno == EINTR)
               {
                   continue;
               }
               if (r < 0) {
                   perror ("select()");
                   exit (1);
               }
       /* NB: read oob data before normal reads */
          for (k = 0; k < INFDS; k++) {
              fd = inFdList[k];
              if (fd >= 0 && FD_ISSET(fd, &rds)) {
                  if (fd == inListener) {
                     r = acceptSocket(buf1, 1020);
                     if (r > 0)
                     {
                          JSocketIsReadNew(buf1, r);
                     }
                  }
                  else if (fd == fdAcq) {
                     AcqSocketIsRead(receive);
                  }
                  else {
                      r = read(fd,buf1,1020);
                      if (r > 0) {
                          buf1[r] = '\0';
                          if (fd == fdFG)
                              chewEachChar(buf1, r);
                          else {
                              JSocketIsReadNew(buf1, r);
                          }
                      }
                      else if (r == 0) {
                          close_input_fd (fd);
                      }
#ifdef __INTERIX
                      /* This might be okay for Mac and Linux, but need to test it */
                      else if ( (r == -1) && (errno == ECONNRESET) ) {
                         write( forgroundFdW, "vnmrexit", 8 );
                         sendChildNewLine();
                         sleepMilliSeconds(1000); /* wait for SIGCHLD */
                      }
#endif
                  }
              }
          }
     }
}


void ring_bell()
{
#ifdef MOTIF
     if (xwin)
         XBell(main_dpy, 20);
#endif
}


#ifdef MOTIF
void
check_pipe(client_data, id)
XtPointer    client_data;
XtIntervalId  *id;
{
     checkPipe();
     return;
}

void
fgpipe_ready(client_data, fd, id)
XtPointer         client_data;
int             *fd;
XtInputId       *id;
{
     fgPipeIsReady(*fd);
     return;
}

void
bgpipe_ready(client_data, fd, id)
XtPointer         client_data;
int             *fd;
XtInputId       *id;
{
     bgPipeIsReady(*fd);
     return;
}
#endif

void setCallbackTimer(int n)
{
#ifdef MOTIF
     if (xwin)
        XtAddTimeOut((unsigned long) n, check_pipe, NULL);
#endif
}


#ifdef XXX
void clearBackground()
{
     signal(SIGCHLD, NULL);
#ifdef MOTIF
     if (xwin)
        XtRemoveInput(inputpipe);
#endif
}
#endif

void clearForground(int pipeSuspended)
{
     signal(SIGCHLD, NULL);
#ifdef MOTIF
     if (xwin)
         XtRemoveInput(inputpipe);
#endif
     if (pipeSuspended)
     {
        DPRINT0("M: ...cancel suspension!\n");
        checkPipe();
     }
}

void waitForgroundChild(int forgroundFdR, int pid)
{
        fgPid = pid;
        register_child_exit_func(SIGCHLD, pid, fgExpired, NULL);
/*
        signal(SIGCHLD, fgExpired);
*/

        inputpipe = 0;
        fdFG = forgroundFdR;
        if (xwin) {
#ifdef MOTIF
           inputpipe = XtAddInput(forgroundFdR, (XtPointer)XtInputReadMask, fgpipe_ready, NULL);
           add_xfd_rec(forgroundFdR, inputpipe);
#endif
        }
        else
           add_input_fd (forgroundFdR);
}

void createWindows2(int tclType, int noUi)
{
   int             argc;
#ifdef MOTIF
   char           *argv[10];
   char           *mainArgv = "Vnmr";
   Widget          mainShell;
#endif

   for (argc = 0; argc < INFDS; argc++)
       inFdList[argc] = -1;

   if (noUi) {
       xwin = 0;
       return;
   }
#ifdef MOTIF
   xwin = 1;
   argv[0] = mainArgv;
   argc = 1;
   if (Xserver[0] != '\0')
   {
        argv[argc++] = Xdisplay;
        argv[argc++] = Xserver;
   }

   mainShell = XtInitialize("Vnmr", "Vnmr", NULL, 0, &argc, argv);

   if (!mainShell)
   {
        fprintf(stderr, " Error: Can't open display %s, exiting...\n", Xserver);
        exit(0);
   }
   main_dpy = XtDisplay(mainShell);
#endif
}

static void
readJSocket(client_data, fd, id)
XtPointer         client_data;
int             *fd;
XtInputId       *id;
{
   openVSocket(*fd);
   return;
}

#ifdef MOTIF
static void
readJSocket2(client_data, fd, id)
XtPointer         client_data;
int             *fd;
XtInputId       *id;
{
   readVSocket(*fd);
   return;
}
#endif

static void
readAcqSocket(client_data, fd, id)
XtPointer         client_data;
int             *fd;
XtInputId       *id;
{

   AcqSocketIsRead(receive);
   return;
}


void readyCatchers()
{
   XtInputId  xfd;

   catchIntSignals();
   fdAcq = GET_VNMR_HANDLE();
  
   xfd = 0;
   if (fdAcq < 0)
   {
       setMasterInterfaceAsync(readAcqSocket);
   }
   else {
       if (xwin) {
#ifdef MOTIF
          xfd = XtAddInput(fdAcq, (XtPointer)XtInputReadMask, readAcqSocket, NULL);
          add_xfd_rec(fdAcq, xfd);
#endif
       }
       else
          add_input_fd (fdAcq);
   }

   inListener = get_vnmrj_socket();
   if (inListener < 0)
       setMasterInterfaceAsync(readJSocket);
   else {
       if (xwin) {
#ifdef MOTIF
          xfd = XtAddInput(inListener, (XtPointer)XtInputReadMask, readJSocket, NULL);
          add_xfd_rec(inListener, xfd);
#endif
       }
       else
          add_input_fd (inListener);
   }
}


void readyJCatchers(int fd )
{
   XtInputId  xfd;

   xfd = 0;
   if (fd < 0)
      setMasterInterfaceAsync(readJSocket);
   else {
      if (xwin) {
#ifdef MOTIF
          xfd = XtAddInput(fd, (XtPointer) XtInputReadMask, readJSocket2, NULL);
          add_xfd_rec(fd, xfd);
#endif
      }
      else
          add_input_fd (fd);
   }
}

void removeJCatchers(int fd )
{
#ifdef MOTIF
   XtInputId  xfd;
#endif

   if (fd < 0)
      return;

   if (xwin) {
#ifdef MOTIF
       xfd = del_xfd_rec(fd);
       if (xfd > 0)
          XtRemoveInput(xfd);
       close(fd);
#endif
   }
   else
       close_input_fd (fd);
}



int listen_socket (int portNum)
{
   struct sockaddr_in a;
   struct sockaddr *a2;
   int s;
   int yes;
   socklen_t len;
   int socketPort;
#ifdef MOTIF
   XtInputId  xfd;
#endif

   if (inListener >= 0) {
      s = get_vnmrj_port();
      if (s > 0) {
          socketPort = htons(s);
          return socketPort;
      }
   }
   if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror ("socket");
      return -1;
   }
   yes = 1;
   if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                (char *) &yes, sizeof (yes)) < 0)
   {
      perror ("setsockopt");
      close (s);
      return -1;
   }
   memset (&a, 0, sizeof (a));
   a.sin_port = portNum;
   a.sin_family = AF_INET;
   a.sin_addr.s_addr = INADDR_ANY;
   if (bind(s, (struct sockaddr *) &a, sizeof (a)) < 0)
   {
      perror ("bind");
      close (s);
      return -1;
   }
        
   len = sizeof(struct sockaddr_in);
   a2 = (struct sockaddr *) &a;
   getsockname(s, a2, &len);
   socketPort = htons(a.sin_port);
   listen (s, 10);
   inListener = s;
   if (xwin) {
#ifdef MOTIF
        xfd = XtAddInput(s, (XtPointer)XtInputReadMask, readJSocket, NULL);
        add_xfd_rec(s, xfd);
#endif
   }
   else
        add_input_fd (s);
   return socketPort;
}

int shelldebug(int t) {

	shell_count=0;
	system_count=0;
	shell_time=0;
	max_time=0;
	TimingFlag=t;
	if(t>0)
		Winfoprintf("Shell call timing turned on (%d)\n",t);
	else
		Winfoprintf("Shell call timing turned off\n");
	return(0);
}

int clrshellcnt(int argc, char *argv[], int retc, char *retv[]) {
	shell_count=0;
	system_count=0;
	shell_time=0;
	max_time=0;
	return(0);
}
int showshellcnt(int argc, char *argv[], int retc, char *retv[]) {
	//fprintf(stderr,"%*scommand %s started\n",MflagIndent,"",n);
	double avetm=shell_count>0?(shell_time/shell_count):0.0;
    switch(TimingFlag){
    case 0:
    	Winfoprintf("shell calls:%d system calls:%d",shell_count,system_count);
    	break;
    case 1:
    	Winfoprintf("shell calls:%d ave:%2.1f ms total:%-2.1f s max %2.1f ms",shell_count,avetm*1000,shell_time,max_time*1000);
    	break;
    default:
    	Winfoprintf("shell calls:%d ave:%2.1f ms total:%-2.1f s max %2.1f ms cmd:%s",shell_count,avetm*1000,shell_time,max_time*1000,max_str);
    	break;
    }
	return(0);
}

