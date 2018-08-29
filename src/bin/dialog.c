/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/Exclusives.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <X11/Xatom.h>

#include "dialog.h"
#include "vglide.h"
#include "dialog.icon"


char   systemdir[MAXPATHL] = "";  /* vnmr system directory */
char   userdir[MAXPATHL] = "";    /* user directory */
char   expdir[MAXPATHL] = "";    /* vnmr exp directory */
char   defpath[MAXPATHL];
char   UserName[40] = "";    /* login name of user */
char   tmpstr[512];
char   tmpstr2[256];
char   tmpstr3[128];
char   confirmStr[128];
char   mainDim[24];
char   topClassName[] = "Dialog";
char   defFile[MAXPATHL];
char   outFile[MAXPATHL];
char   tmpDir[MAXPATHL];
char   outDir[MAXPATHL];
char   tmpFile[MAXPATHL];
char   file_dir[MAXPATHL];
char   file_name[MAXPATHL];
char   outStr[512];
char   outStr2[512];
char   *defName;
char   *outName;

Widget  topShell;
Display *dpy;
Window  topShellId;
Widget scrolledwindow, dispWin;
Widget refWidget;
Widget previewShell = NULL;
Widget previewButton = NULL;
Widget outTitle;
static XrmDatabase  dbase = NULL;
static Dimension    viewWidth;
static Dimension    viewHeight;
static int   diffW = 0;
static int   diffH = 0;
static int   disp_preview = 0;
static int   show_remark = 0;
TextEditWidget infoWindow, outWindow;

#define ANYVALUE	999

GC	gc;
int	proc_pid;
int     n;
int     out_ok = -1;
int     in_pipe = -1;
int     out_pipe = -1;
int     isBusy;
int	vnmrRet;
int	debug;
int	do_preview = 0;
int	win_x, win_y;
Arg     args[20];
char	*vnmrMess;

static int      vargc;
static int      vnmr_wait = 0;
static int      choiceExec = 0;

static Window   vnmrShell;
static XtIntervalId   timerId = NULL;
static XtInputId  pipeid;

typedef struct _cmd_node {
		int	in_use;
		char    *cmdStr;
		struct _cmd_node *prev;
		struct _cmd_node *next;
		} CMD_NODE;

static CMD_NODE *cmd_list = NULL;

char  *VCmds[] = { "up", "exit", "down", "done", "exec", "close", "debug",
		   "updown", "-debug", "opendef", "readdef", "acqdone",
		   "readdefandopen", "readdefandshow", "setexp",
		   "setexpandopen", "setexpandshow" };
int   VTypes[] = { UP, GEXIT, DOWN, DONE, VEXEC, GCLOSE, DEBUG,
		   UPDN, XDEBUG, OPENDEF, SETEXP, DONE,
		   OPENEXP, SHOWEXP, SETEXP, OPENEXP, SHOWEXP, 0 };
char  *VArgs[9];


void  exit_cb()
{
	cmd_to_vnmr(GEXIT);
	if (in_pipe > 0)
	    XtRemoveInput(pipeid);
	XCloseDisplay(dpy);
	exit(0);
}


static void
OlitExit(w, client_data, event )
Widget w;
char *client_data;
void *event;
{
        OlWMProtocolVerify      *olwmpv;

        olwmpv = (OlWMProtocolVerify *) event;
        if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
                exit_cb();
        }
}



init_dialog_resource()
{
	XrmDatabase  tbase;

	dbase = XtDatabase(dpy);
	sprintf (tmpstr, "%s/app-defaults/Dialog", userdir);
	if (access (tmpstr, R_OK) == 0)
	{
           tbase = XrmGetFileDatabase(tmpstr);
           XrmMergeDatabases(tbase, &dbase);
        }
	else
	{
	   sprintf (tmpstr, "%s/app-defaults/Dialog", systemdir);
           if (access (tmpstr, R_OK) == 0)
           {
              tbase = XrmGetFileDatabase(tmpstr);
              XrmMergeDatabases(tbase, &dbase);
           }
	}
	if (debug > 2)
	   XrmPutFileDatabase(dbase, "/tmp/dialog");
}


char *get_x_resource(class, name)
 char   *class, *name;
{
	char  *type[6];
	XrmValue  retval;

	if (dbase == NULL)
		return( (char *)NULL);
		
	if (XrmGetResource (dbase, name, class, type, &retval) == FALSE)
	{
	    if (debug > 2)
		fprintf(stderr, " %s:  null\n", name);
	    return( (char *)NULL);
	}
	if (debug > 2)
	    fprintf(stderr, " %s: %s\n", name, (char *)retval.addr);
	return( (char *) retval.addr);
}


void  main(argc, argv)
int     argc;
char    *argv[];
{
     char   	  *resource;
     Window 	  win;
     int	  rx, ry;

     parse_argvs(argc, argv);
     if (defFile[0] == '\0')
	exit(0);
     topShell = XtInitialize(topClassName, topClassName, 
                 	(XrmOptionDescRec *) NULL, 0, (Cardinal *)&argc, argv);
     dpy = XtDisplay(topShell);
  
     isBusy = 0;
     init_dialog_resource();
     set_up_defaults();
     set_pipe_in_func();

     if ((defName = strrchr(defFile, '/')) != NULL)
	defName++;
     else
	defName = defFile;
     if (outFile[0] == '\0')
	sprintf(outFile, "%s_out", defName);
     if (outFile[0] == '/')
     {
        outName = strrchr(outFile, '/');
	strncpy(outDir, outFile, outName - outFile);
	outDir[outName - outFile] = '\0';
	outName++;
     }
     else
     {
        sprintf(outDir, "%s/tmp", systemdir);
	outName = outFile;
     }

     n = 0;
     XtSetArg (args[n], XtNtitle, defName);  n++;
     sprintf(tmpstr, "*%s*geometry", topClassName);
     if ((resource = get_x_resource(topClassName, tmpstr)) != NULL)
     {
	strcpy(mainDim, resource);
        XtSetArg (args[n], XtNgeometry, mainDim);  n++;
     }
     sprintf(tmpstr, "*%s*x", topClassName);
     if ((resource = get_x_resource(topClassName, tmpstr)) != NULL)
	win_x = atoi(resource);
     sprintf(tmpstr, "*%s*y", topClassName);
     if ((resource = get_x_resource(topClassName, tmpstr)) != NULL)
	win_y = atoi(resource);
     if (vnmrShell > 0 && (win_x < 0 || win_y < 0))
     {
	XTranslateCoordinates(dpy, vnmrShell, DefaultRootWindow(dpy),
                        0, 0, &rx, &ry, &win);
	if (win_x < 0)
	    win_x = rx + 140;
	if (win_y < 0)
	    win_y = ry + 30;
     }
     if (win_x >= 0)
     {
	XtSetArg(args[n], XtNx, win_x);  n++;
     }
     if (win_y >= 0)
     {
	XtSetArg(args[n], XtNy, win_y);  n++;
     }
     XtSetArg(args[n], XtNiconPixmap, XCreateBitmapFromData (dpy,
                      XtScreen(topShell)->root, icon_bits,
                      icon_width, icon_height));
     n++;
     XtSetValues (topShell, args, n);

     open_def_win();
     
     topShellId = XtWindow(topShell);
     OlAddCallback(topShell, XtNwmProtocol, OlitExit, NULL );
     XtMainLoop();
}


get_username()
{

        int              ulen;
        struct passwd   *getpwuid();
        struct passwd   *pasinfo;

        pasinfo = getpwuid( getuid() );
        if (pasinfo == NULL)
	{
   		strcpy(UserName, "");
		return;
	}
        ulen = strlen( pasinfo->pw_name );
        if (ulen >= 40) {
                strncpy( UserName, pasinfo->pw_name, 40-1 );
                UserName[ 40-1 ] = '\0';
        }
        else
          strcpy(UserName, pasinfo->pw_name);
}


set_up_defaults()
{
     char   *ptr;

     gc = DefaultGC(dpy, 0);
     ptr = getenv("vnmrsystem");
     if (ptr)
     {  if ( (int)strlen(ptr) < MAXPATHL - 32)
            strcpy(systemdir,ptr);
        else
        {   printf("Error:value of environment variable 'vnmrsystem' too long\n");
            exit(1);
        }
     }
     else
	strcpy(systemdir,"/vnmr");
     ptr = getenv("vnmruser");
     if (ptr)
     {  if ((int) strlen(ptr) < MAXPATHL - 32)
	strcpy(userdir,ptr); 
	else
	{ printf("Error:value of environment variable 'vnmruser' too long\n");
 		 exit(1);
	}
     }
     else
     {
	printf("Error: environment variable 'vnmruser' missing\n");
        exit(1);
     }

     proc_pid = getpid();
     get_username();
}


cmd_to_vnmr(cmd)
int  cmd;
{
     char   cmdstr[64];

     if (out_ok >= 0)
     {
	if (debug)
	    fprintf(stderr, " send command %d to vnmr\n", cmd);
	if (vnmr_wait && out_pipe)
	{
	   sprintf(cmdstr, "3 ,\"dialog\" ,\"SYNCFUNC\" ,\"%d\" ,\"%d\"", cmd, proc_pid);
	   write (out_pipe, cmdstr, strlen(cmdstr));
	}
	else
	{
	   sprintf(cmdstr, "dialog('SYNCFUNC',%d, %d)\n", cmd, proc_pid);
           sendToVnmr(cmdstr);
	}
     }
}



macro_to_vnmr(macroDir, macroName)
char *macroDir;
char *macroName;
{
     int   	   n, k;
     struct stat   f_stat;
     char macroPath[MAXPATHL];
     char vnmrCmd[2 * MAXPATHL];

     if (macroName[0] != '/')
         sprintf(macroPath,"%s/%s",macroDir,macroName);
     else
         strcpy(macroPath, macroName);
     if ((out_ok < 0) || ( (int)strlen(macroPath) <= 0) )
	return;
     if (stat(macroPath, &f_stat) == -1)
     {
	k = 0;
	/* if file is not ready, wait for a while */
	while (k < 1000)
	{
	   fflush((FILE *) NULL);
	   n = 0;
	   while (n < 100000) n++;
           if (stat(macroPath, &f_stat) == 0)
		break;
	   k++;
	}
     }
     sprintf(vnmrCmd,"macrold('%s'):$dum %s\n",macroPath,macroName);
     if (debug)
	fprintf(stderr, "macro to vnmr: %s", vnmrCmd);
     if (do_preview)
	disp_macro_info(macroPath);

     if (vnmr_wait && out_pipe)
     {
	sprintf(vnmrCmd, "5 ,\"dialog\" ,\"SYNCFUNC\" ,\"%d\" ,\"%d\" ,\"macrold('%s'):$dum %s\n\"",
		 VEXEC, proc_pid, macroPath, macroName);
	write (out_pipe, vnmrCmd, strlen(vnmrCmd));

     	if (debug)
            sprintf(vnmrCmd,"5 ,\"dialog\" ,\"SYNCFUNC\" ,\"%d\" ,\"%d\" ,\"purge('%s')\n\"", VEXEC, proc_pid, macroName);
        else
            sprintf(vnmrCmd,"5 ,\"dialog\" ,\"SYNCFUNC\" ,\"%d\" ,\"%d\" ,\"purge('%s') delete('%s')\n\"", VEXEC, proc_pid, macroName, macroPath);
	write (out_pipe, vnmrCmd, strlen(vnmrCmd));
     }
     else
     {
     	sendToVnmr(vnmrCmd);
     	if (debug)
            sprintf(vnmrCmd,"purge('%s')\n",macroName);
        else
            sprintf(vnmrCmd,"purge('%s') delete('%s')\n",macroName,macroPath);
        sendToVnmr(vnmrCmd);
     }
}


sync_macro_to_vnmr(macroDir, macroName)
char *macroDir;
char *macroName;
{
     XEvent	event;
     int   	   n, k;
     struct stat   f_stat;
     char macroPath[MAXPATHL];
     char vnmrCmd[2 * MAXPATHL];

     vnmrMess = NULL;
     if (out_ok < 0)
	 return;
     vnmrRet = 0;
     sprintf(macroPath,"%s/%s",macroDir,macroName);
     if (stat(macroPath, &f_stat) == -1)
     {
	k = 0;
	while (k < 1000)
	{
	   fflush((FILE *) NULL);
	   n = 0;
	   while (n < 100000) n++;
           if (stat(macroPath, &f_stat) == 0)
		break;
	   k++;
	}
     }
     sprintf(vnmrCmd,"macrold('%s'):$dum %s\n",macroPath,macroName);
     if (debug)
	fprintf(stderr, "macro to vnmr: %s", vnmrCmd);
     if (do_preview)
	disp_macro_info(macroPath);
     sendToVnmr(vnmrCmd);
     sprintf(vnmrCmd,"purge('%s')\n",macroName);
     sendToVnmr(vnmrCmd);
     while( !vnmrRet)
     {
	  XtNextEvent(&event);
          XtDispatchEvent(&event);
	  if (vnmrRet)
	     break;
     }
}


sync_read_vnmr(cmd)
int   cmd;
{
     XEvent	event;

     vnmrMess = NULL;
     if (out_ok >= 0)
     {
	vnmrRet = 0;
	cmd_to_vnmr(cmd);
	while( !vnmrRet)
	{
	     XtNextEvent(&event);
             XtDispatchEvent(&event);
	     if (vnmrRet)
		break;
	}
     }
     else if (debug)
        fprintf(stderr, " error: couldn't send cmd %d to Vnmr\n", cmd);
}



sync_query_vnmr(param)
char  *param;
{
	XEvent	event;
	char   	cmdstr[128];
	int	len, k;

	vnmrMess = NULL;
	if (out_ok < 0)
	   return;
	vnmrRet = 0;
	if (debug)
	   fprintf(stderr, " query vnmr of %s\n", param);
	len = strlen(param);
	k = 0;
	while (len > 0)
	{
	     if (*param == '\'')
	     {
	        tmpstr[k] = '\\';
		k++;
	     }
	     tmpstr[k] = *param;
	     k++;
	     len--;
	     param++;
	}
	tmpstr[k] = '\0';
	if (vnmr_wait && out_pipe)
	{
	   sprintf(cmdstr, "4 ,\"dialog\" ,\"SYNCFUNC\" ,\"%d\" ,\"%d\" ,\"%s\"", GQUERY, 
			proc_pid, tmpstr);
	   write (out_pipe, cmdstr, strlen(cmdstr));
	}
	else
	{
	   sprintf(cmdstr, "dialog('SYNCFUNC', %d, %d,'%s')\n", GQUERY,
			proc_pid, tmpstr);
           sendToVnmr(cmdstr);
	}
	while( !vnmrRet)
	{
	     XtNextEvent(&event);
             XtDispatchEvent(&event);
	     if (vnmrRet)
		break;
	}
}


int
check_show_status(showStr)
char    *showStr;
{
        int     len;

        while (*showStr == ' ' || *showStr == '\t')
                showStr++;
        len = strlen(showStr);
	if (len <= 0)
		return(1);
        if (len == 2)
        {
           if (strcmp(showStr, "no") == 0 || strcmp(showStr, "No") == 0)
                return(0);
        }
        if (len == 3)
        {
           if (strcmp(showStr, "yes") == 0 || strcmp(showStr, "Yes") == 0)
                return(1);
        }
	sync_query_vnmr(showStr);
        if (vnmrMess == NULL)
           return(0);
        return(atoi(vnmrMess));
}


timerExecProc()
{
	CMD_NODE *clist;

	if (isBusy && cmd_list)
	{
            timerId = XtAddTimeOut(1500, timerExecProc, NULL);
	    return;
	}
	if (cmd_list == NULL)
	    return;
	clist = cmd_list;
	cmd_list = clist->next;
	isBusy = 1;
	exec_input(clist, clist->cmdStr);
}


appendExecStr(command)
char  *command;
{
	int	  len;
	CMD_NODE  *clist, *plist;

	len = strlen(command);
	if (len <= 0)
	    return;
	clist = (CMD_NODE *) calloc(1, sizeof(CMD_NODE));
	clist->cmdStr = malloc(strlen(command) +1);
	strcpy(clist->cmdStr, command);
	if (cmd_list == NULL)
	   cmd_list = clist;
	else
	{
	   plist = cmd_list;
	   while (plist->next != NULL)
		plist = plist->next;
	   plist->next = clist;
	}
	if(timerId != NULL)
    	{
           XtRemoveTimeOut(timerId);
           timerId = NULL;
    	}
        timerId = XtAddTimeOut(1200, timerExecProc, NULL);
}


read_from_vnmr(c_data, fd, id)
 XtPointer  c_data;
 int        *fd;
 XtInputId  id;
{
     int     n;
     XEvent  event;

     static char  str[256];

     if (in_pipe < 0)
	return;
     if ((n = read(*fd, str, 256)) > 1)
     {
	str[n] = '\0';
	if (str[n-1] == '\n')
	   str[n-1] = '\0';
	if (debug)
	   fprintf(stderr, "  get from vnmr: '%s' \n", str);
	if (*str == ESC)
	{
	   if (isBusy)
	      appendExecStr(str+2);
	   else
	   {
	      isBusy = 1;
	      exec_input(NULL, str+2);
	   }
	}
	else
  	{
	   vnmrMess = str;
	   vnmrRet = 1;

	   /* send false event to activate event loop */
	   event.type = MotionNotify;
	   event.xmotion.send_event = TRUE;
	   event.xmotion.display = dpy;
	   event.xmotion.window = topShellId;
	   event.xmotion.root = DefaultRootWindow(dpy);
	   event.xmotion.time = CurrentTime;
	   event.xmotion.same_screen = TRUE;
	   XSendEvent (dpy, topShellId, TRUE, NoEventMask,
                &event);
  	}
     }
}


set_pipe_in_func()
{
     if (in_pipe > 0)
        pipeid = XtAddInput(in_pipe, (caddr_t)XtInputReadMask, 
		(XtInputCallbackProc)read_from_vnmr, (XtPointer)NULL);
}


int
parse_vnmr_cmd(cmdStr)
char	 *cmdStr;
{
	int     m, argc;

     	if (sscanf(cmdStr, "%d", &argc) < 1)
	   return(0);
	vargc = 0;
	while (*cmdStr != '\0')
	{
	   if ((*cmdStr == ',') && (*(cmdStr+1) == '\"'))
	   {
		cmdStr += 2;
		while (*cmdStr == ' ' || *cmdStr == '\t')
		     cmdStr++;
	 	VArgs[vargc] = cmdStr;
		vargc++;
		if (vargc > 8)
		     break;
		while (*cmdStr != '\0')
		{
		     if ((*cmdStr == '\"') && (*(cmdStr - 1) != '\\'))
		     {
			  *cmdStr = '\0';
		          cmdStr++;
		          break;
		     }
		     else
		          cmdStr++;
		}
	   }
	   else
	        cmdStr++;
	}
	if (vargc <= 0)
	   return(0);
	for (m = 0; m < sizeof(VCmds) / sizeof(char *); m++)
	{
	   if (strcmp(VArgs[0], VCmds[m]) == 0)
		return(VTypes[m]);
	}
	return(0);
}


exec_input(clist, cmdStr)
CMD_NODE *clist;
char	 *cmdStr;
{
     int    cmd;

     cmd = parse_vnmr_cmd (cmdStr);
     if (debug)
	 fprintf(stderr, " command from vnmr: %d %s\n", cmd, cmdStr);
     switch (cmd) {
	case  UP:
	     	XtMapWidget(topShell);
	     	XMapRaised(dpy, topShellId);
		break;
	case  GCLOSE:
		XtUnmapWidget (topShell);
		break;
	case  GEXIT:
		exit_cb();
		break;
	case  OPENDEF:
		if (vargc >= 3)
		{
		    strcpy(tmpstr, VArgs[1]);
		    strcpy(tmpstr2, VArgs[2]);
		    if (vargc > 3)
		        strcpy(tmpstr3, VArgs[3]);
		    else
		 	strcpy(tmpstr3, "user");
		    if (debug)
			fprintf(stderr, " opendef : %s  %s\n", tmpstr, tmpstr2);
		}
		break;
	case  VEXEC:
		if (vargc >= 2)
		{
		    if (debug)
			fprintf(stderr, " exec: %s\n", VArgs[1]);
		    button_exec_proc(NULL, VArgs[1]);
		}
		break;
	case  DEBUG:
		debug = 1;
		break;
	case  XDEBUG:
		debug = 0;
		break;
     }
     if (clist)
     {
	free(clist->cmdStr);
	free(clist);
     }
     isBusy = 0;
}


parse_argvs(argc, argv)
int argc;
char *argv[];
{
	int   i;
	char  *p;
	int    rx, ry, bx, by, gh;
        Window  win;
	XWindowAttributes win_attr;

	vnmrShell = 0;
	win_x = -1;
	win_y = -1;
	defFile[0] = '\0';
	outFile[0] = '\0';
	for(i = 0; i < argc; i++)
	{
	    p = argv[i];
	    if (*p == '-')
            switch (*(++p))
            {
                 case 'O':
                          out_pipe = atoi(p+1);
                          break;
                 case 'S':
                          vnmrShell = (Window)atoi(p+1);
                          break;
                 case 'h':
                          initVnmrComm(p+1);
                          out_ok = 1;
                          break;
                 case 'I':
                          in_pipe = atoi(p+1);
                          break;
                 case 'Q':
                          if (i < argc - 1)
			  {
				i++;
				strcpy(defFile, argv[i]);
			  }
			  break;
                 case 'P':
                          if (i < argc - 1)
			  {
				i++;
				strcpy(outFile, argv[i]);
			  }
			  break;
                 case 'W':
			  vnmr_wait = 1;
			  break;
                 case 'x':
                          if (i < argc - 1)
			  {
				i++;
				win_x = atoi(argv[i]);
			  }
			  break;
                 case 'y':
                          if (i < argc - 1)
			  {
				i++;
				win_y = atoi(argv[i]);
			  }
			  break;
                 case 'd':
			  if (*(p+1) != '\0')
                                debug = atoi(p+1);
			  if (debug > 1)
				debug--;
			  else
				debug = 0;
     			  do_preview = 1;
                          break;
	     }
	}
}


execute_exec_cmd (wentry, code, param)
subWin_entry  *wentry;
int	code;
char    *param;
{
	int	parLen;
	FILE    *fd;
	char   outPath[MAXPATHL];
	ROW_NODE     *rownode;

	if (debug)
	    fprintf(stderr, " button cmd: %d ", code);
	parLen = 0;
	if (param != NULL)
	{
	    while (*param == ' ' || *param == '\t')
	  	param++;
	    parLen = strlen(param) - 1;
	    while (parLen >= 0)
	    {
		if (param[parLen] == ' ' || param[parLen] == '\t')
		    param[parLen] = '\0';
		else
		    break;
		parLen--;
	    }
	    parLen = strlen(param);
	    if (debug)
	        fprintf(stderr, "  -%s-", param);
	}
	if (debug)
	    fprintf(stderr, " \n");

	switch (code) {
	  case BUT_CLOSE:
	  case BUT_EXIT:
			  exit_cb();
		  	  break;
	  case BUT_DO:
			  output_def_file(wentry, 0);
			  break;
	  case BUT_RESET:
			  reset_def_list(wentry);
			  break;
	  case BUT_SAVE:
			  output_def_file(wentry, 1);
			  break;
	  case BUT_VIEW:
			  defWin_preview_proc(NULL, wentry, NULL);
			  break;
	  case BUT_VEXEC:
			  if (parLen <= 0)
                		return;
			  sync_read_vnmr(EXPDIR);
        		  if (vnmrMess == NULL)
                		return;
        		  strcpy(expdir, vnmrMess);
			  sprintf(outPath, "%s/eou_rt", expdir);
			  if ((fd = fopen(outPath, "w")) == NULL)
                                return;
			  fprintf(fd, "%s\n", param);
			  fclose(fd);
			  sync();
			  macro_to_vnmr(expdir, "eou_rt");
			  break;
	  case BUT_MASK:
	  case BUT_UNMASK:
			  if (parLen <= 0)
                		return;
			  rownode = wentry->vnode;
		  	  while (rownode != NULL)
			  {
	    		     if (rownode->show && rownode->name)
	    		     { 
				if (strcmp(rownode->name, param) == 0)
			  	    mask_def_row(rownode, code);
	    		     } 
			     rownode = rownode->next;
			  }
			  break;
	}
}


static int
get_cmd_code(data)
char	*data;
{
	int	code;

	code = -1;
	while (*data == ' ' || *data == '\t')
		data++;
	switch (*data) {
	  case 'C':
		   if (strcmp(data, "CLOSE") == 0)
			code = BUT_CLOSE;
		   else if (strcmp(data, "CALL") == 0)
			code = BUT_CALL;
		   break;
	  case 'D':
		   if (strcmp(data, "DEF") == 0)
			code = BUT_DEF;
		   else if (strcmp(data, "DO") == 0)
			code = BUT_DO;
		   break;
	  case 'E':
		   if (strcmp(data, "EXIT") == 0)
			code = BUT_EXIT;
		   break;
	  case 'M':
		   if (strcmp(data, "MASK") == 0)
			code = BUT_MASK;
		   else if (strcmp(data, "MENU") == 0)
			code = BUT_MENU;
		   break;
	  case 'O':
		   if (strcmp(data, "OPEN") == 0)
			code = BUT_OPEN;
		   if (strcmp(data, "OPENDEF") == 0)
			code = BUT_OPENDEF;
		   break;
	  case 'P':
		   if (strcmp(data, "PREVIEW") == 0)
			code = BUT_VIEW;
		   break;
	  case 'R':
		   if (strcmp(data, "RESET") == 0)
			code = BUT_RESET;
		   break;
	  case 'S':
		   if (strcmp(data, "SHOW") == 0)
			code = BUT_SHOW;
		   else if (strcmp(data, "SAVE") == 0)
			code = BUT_SAVE;
		   else if (strcmp(data, "SETVAL") == 0)
			code = BUT_SETVAL;
		   break;
	  case 'U':
		   if (strcmp(data, "UNMASK") == 0)
			code = BUT_UNMASK;
		   break;
	  case 'V':
		   if (strcmp(data, "VNMREXEC") == 0)
			code = BUT_VEXEC;
		   break;
	}
	return(code);
}


static int
parse_cmd (cmdStr, retParam)
char	*cmdStr;
char	*retParam;
{
	static  char	cmdLine[256], *dtr;
	static  int	ptr, curCmd, lp;
	char	data[128];

	if (cmdStr != NULL)
	{
	   strcpy(cmdLine, cmdStr);
	   lp = 0;
	   dtr = cmdLine;
	}
	while (*dtr == ' ' || *dtr == '\t')
	   dtr++;
	retParam[0] = '\0';
	curCmd = 0;
	ptr = 0;
	data[0] = '\0';
	while (1)
	{
	   switch (*dtr) {
	    case  '\0':
	    case  '\n':
			if (lp == 0)
			{
			   if (ptr > 0)
			   {
			      data[ptr] = '\0';
			      curCmd = get_cmd_code(data);
			      ptr = 0;
			   }
			}
			else
			   curCmd = 0;
			return(curCmd);
			break;
	    case  '\t':
	    case  ',':
	    case  ' ':
			if (lp == 0)
			{
			    if (ptr > 0)
			    {
			         data[ptr] = '\0';
			         curCmd = get_cmd_code(data);
			         ptr = 0;
			    }
			}
			else
			{
			    
			    data[ptr] = *dtr;
			    ptr++;
			}
			dtr++;
			break;
	    case  '(':
			lp++;
			if (lp == 1)
			{
			    if (ptr > 0)
			    {
			         data[ptr] = '\0';
			         curCmd = get_cmd_code(data);
			         ptr = 0;
			    }
			}
			else
			{
			    data[ptr] = *dtr;
			    ptr++;
			}
			dtr++;
			break;
	    case  ')':
			lp--;
			if (lp == 0)
			{
			   if (ptr > 0)
			   {
			      data[ptr] = '\0';
		 	      strcpy(retParam, data);
			   }
			   dtr++;
			   if (curCmd > 0)
			      return(curCmd);
			   ptr = 0;
			}
			else
			{
			   data[ptr] = *dtr;
			   ptr++;
			   dtr++;
			}
			break;
	    case  '"':
			if (lp > 0)
			{
			   data[ptr] = *dtr;
			   ptr++;
			}
			dtr++;
			break;
	     default:
			if (curCmd > 0 && lp == 0)
			   return(curCmd);
			data[ptr] = *dtr;
			ptr++;
			dtr++;
			break;
	    }
	}
}


/*********************************************************************
 commands are:  DO, CLOSE, DEF, EXIT, VNMREXEC, MASK, UNMASK,
		CALL, RESET, OPEN, SHOW, SETVAL
**********************************************************************/
button_exec_proc(wentry, cmd)
subWin_entry *wentry;
char   *cmd;
{
	int	gotCmd;
	char	param[128];

	if (debug)
	    fprintf(stderr, " exec: %s\n", cmd);
	gotCmd = parse_cmd(cmd, param);
	while (gotCmd != 0)
	{
	    if (gotCmd > 0)
	        execute_exec_cmd(wentry, gotCmd, param);
	    gotCmd = parse_cmd(NULL, param);
	}
}

row_exec_proc(rnode, cmd)
ROW_NODE   *rnode;
char   *cmd;
{
	int	gotCmd;
	char	param[128];
	subWin_entry *wentry;

	if (debug)
	    fprintf(stderr, " exec: %s\n", cmd);
	wentry = (subWin_entry *) rnode->win_entry;
	if (wentry == NULL)
	    return;
	gotCmd = parse_cmd(cmd, param);
	while (gotCmd != 0)
	{
	    if (gotCmd == BUT_SETVAL)
		set_item_value(rnode, param);
	    if (gotCmd > 0)
	        execute_exec_cmd(wentry, gotCmd, param);
	    gotCmd = parse_cmd(NULL, param);
	}
}


extern char	 *get_token_and_id();
extern ROW_NODE  *exit_row;


char 
*find_def_file(where)
int	   where;
{
	struct   stat   f_stat;


	if (debug)
	    fprintf(stderr, " def_file(%d):  %s\n", where, defFile);
	file_name[0] = '\0';
	if (defFile[0] == '/')
	    strcpy(file_name, defFile);
	else if (where == USERDIR)
	{
            sprintf(file_name, "%s/app_defaults/%s", userdir, defFile);
	}
	else
	{
            sprintf(file_name, "%s/app_defaults/%s", systemdir, defFile);
	}
	if (stat(file_name, &f_stat) == 0)
	{
	    if (debug)
		fprintf(stderr, "  file %s is available\n", file_name);
	    return(file_name);
	}
	else
	{
	    if (debug)
		fprintf(stderr, "  file %s does not exist.\n", file_name);
	    return((char *) NULL);
	}
}


static int
query_show(node, id)
ITEM_NODE   *node;
int	    id;
{
	XVALUE_NODE  *xvnode;

	while (node != NULL)
	{
	     if (node->type == XSHOW && node->id == id)
	     {
		 xvnode = (XVALUE_NODE *) node->data_node;
              	 while (xvnode != NULL)
              	 {
                    if (xvnode->data != NULL)
		    {
                        if (check_show_status(xvnode->data) == 0)
			    return(0);
		    }
                    xvnode = xvnode->next;
		 }
             }
	     node = node->next;
	}
	return(1);
}



create_def_list(entry, where)
subWin_entry *entry;
int	     where;
{
	ROW_NODE      *rnode;
	ITEM_NODE     *cnode;
	char	      *fileName;
	
	fileName = find_def_file(where);
	if (fileName == NULL)
	      return;
        parse_new_def(entry, fileName);
	if (entry->vnode)
	{
	    rnode = entry->vnode;
	    while (rnode != NULL)
	    {
		if (rnode->checkShow)
		   rnode->show = query_show(rnode->item_list, -1);
		if (rnode->show && rnode->checkItem)
		{
		   cnode = (ITEM_NODE *) rnode->item_list;
		   while (cnode != NULL)
		   {
		      if (cnode->checkShow)
			cnode->show = query_show(rnode->item_list, cnode->id);
		      cnode = cnode->next;
		   }
		}
		rnode = rnode->next;
	    }
	}
}


open_def_win()
{
	subWin_entry  *def_entry;

        def_entry = (subWin_entry *) calloc(1, sizeof(subWin_entry));
        if (def_entry == NULL)
	    exit(0);
        create_def_list(def_entry, USERDIR);
	if (def_entry->vnode == NULL)
	    create_def_list(def_entry, GLOBAL);
	if (def_entry->vnode == NULL)
	    exit(0);

	build_def_window(def_entry);
}



static char
*get_value_id (sptr, retid)
char  *sptr;
int   *retid;
{
	char  *dptr;
	char  numStr[12];
	int   len, ok;

	*retid = -1;
	dptr = sptr;
	while (*dptr == ' ' || *dptr == '\t')
	   dptr++;
	if (*dptr != '(')
	   return(sptr);
	len = 0;
	ok = 0;
	dptr++;
	while (*dptr == ' ' || *dptr == '\t')
	   dptr++;
	while (*dptr != '\0')
	{
	   if (*dptr == ')')
	   {
		ok = 1;
		dptr++;
		numStr[len] = '\0';
		break;
	   }
	   if (*dptr < '0' || *dptr > '9')
		break;
	   else
	   {
		numStr[len] = *dptr;
		len++;
		if (len > 10)
		   break;
	   }
	   dptr++;
	}
	if (!ok || len <= 0)
	   return(sptr);
	*retid = atoi(numStr);
	return(dptr);
}


static char
*put_one_row_text(w, dest)
Widget   w;
char     *dest;
{
        int     len, q_mark;
        char    *data, *d2;

        XtSetArg (args[0], XtNstring, &data);
        XtGetValues(w, args, 1);
        if (data == NULL)
             return(dest);
	strcpy(tmpstr2, data);
	XtFree(data);
	data = tmpstr2;
        while (*data == ' ' || *data == '\t')
                data++;
        len = strlen(data);
	d2 = data + len - 1;
        while (len > 0)
        {
             if (*d2 == ' ' || *d2 == '\t')
		*d2 = '\0';   
	     else
		break;
	     d2--;
	     len--;
        }
	q_mark = 0;
	while (*data != '\0')
	{
             if (*data == '\\')
	     {
		  if (q_mark)
		     q_mark = 0;
		  else
		     q_mark = 1;
	     }
             else if (*data == '\'')
             {
		  if (!q_mark)
		  {
		     *dest = '\\';
		     *dest++;
		  }
		  else
		     q_mark = 0;
	     }
	     else
		  q_mark = 0;
	     *dest = *data;
	     dest++;
	     data++;
	}
        return(dest);
}


static char
*put_multi_row_text(w, dest, cols)
Widget   w;
char     *dest;
int	 cols;
{
        int     len;
	char	*text, *data;

	OlTextEditCopyBuffer((TextEditWidget)w, &text);
        if (text == NULL || (int)strlen(text) <= 0)
	    return(dest);
        len = 0;
        data = text;
        while (*data != '\0')
        {
             if (*data == '\n' || len >= cols)
             {
                  if (*data != '\n')
		  {
			*dest = *data;
			dest++;
		  }
		  *dest = '\\';
		  *dest++;
		  *dest = 'n';
		  *dest++;
                  len = 0;
             }
             else if (*data == '\'')
             {
		  *dest = '\\';
		  *dest++;
		  *dest = *data;
		  *dest++;
                  len += 2;
             }
             else
             {
		  *dest = *data;
		  *dest++;
                  len++;
             }
             data++;
        }
        XtFree(text);
	return(dest);
}


retrieve_row_data(pnode, source, dest)
 ITEM_NODE  *pnode;
 char       *source, *dest;
{
	int	   k, type, id;
	ITEM_NODE    *node, *dnode;
	XINPUT_NODE  *innode;
	XCHOICE_NODE *chnode;
	XRADIO_NODE  *rdnode;
	XVALUE_NODE  *xvnode;
	XPANE_NODE   *pane;
	

	*dest = '\0';
	while (*source != '\0')
        {
            if (*source != '$')
	    {
		*dest = *source;
                dest++;
                source++;
		continue;
	    }
	    type = 0;
	    if (strncmp(source, "$input", 6) == 0)
            {
		source += 6;
		type = XINPUT;
	    }
	    else if (strncmp(source, "$choice_value", 13) == 0)
            {
		source += 13;
		type = XMENU;
	    }
	    else if (strncmp(source, "$choice", 7) == 0)
            {
		source += 7;
		type = XMENU;
	    }
	    else if (strncmp(source, "$menu_value", 11) == 0)
            {
		source += 11;
		type = XMENU;
	    }
	    else if (strncmp(source, "$menu", 5) == 0)
            {
		source += 5;
		type = XMENU;
	    }
	    else if (strncmp(source, "$value", 6) == 0)
            {
		source += 6;
		type = ANYVALUE;
	    }
	    else if (strncmp(source, "$chcek_set_value", 16) == 0)
            {
		source += 16;
		type = XSETVAL;
	    }
	    else if (strncmp(source, "$check_unset_value", 18) == 0)
            {
		source += 18;
		type = XUNSETVAL;
	    }
	    if (type <= 0)  /* not any keyword */
	    {
		*dest = *source;
                source++;
                dest++;
		continue;
	    }
	    source = get_value_id (source, &id);
	    if (type == ANYVALUE && id <= 0)
		continue;

	    node = pnode;
	    while (node != NULL)
	    {
		switch (node->type) {
		 case XINPUT:
			if (node->id != id)
			    break;
			if (type == ANYVALUE || type == XINPUT)
			{
			    innode = (XINPUT_NODE *) node->data_node;
			    if (innode->widget != NULL)
			    {
			       if (innode->rows <= 1)
			          dest = put_one_row_text(innode->widget, dest);
			       else
			          dest = put_multi_row_text(innode->widget,dest,
						 innode->cols);
			    }
			    else
			    {
				if (innode->data)
				{
				    k = strlen(innode->data);
				    *dest = '\0';
				    strcat(dest, innode->data);
				    dest += k;
				}
			    }
			}
			break;
		 case XMENU:
		 case XCHOICE:
		 case XABMENU:
			if (node->id != id)
			    break;
			if (type == ANYVALUE || type == XMENU)
			{
			    pane = (XPANE_NODE *) node->data_node;
			    chnode = (XCHOICE_NODE *) pane->wlist;
			    while (chnode != NULL)
			    {
				if (chnode->bid == pane->val)
				{
				   if (chnode->data != NULL)
				   {
					k = strlen(chnode->data);
					*dest = '\0';
				        strcat(dest, chnode->data);
					dest += k;
				   }
				   break;
				}
				chnode = chnode->next;
			    }
			}
			break;
		 case XRADIO:
			if (node->id != id)
			    break;
			rdnode = (XRADIO_NODE *) node->data_node;
			if (type == ANYVALUE)
			{
			    if (rdnode->set)
				type = XSETVAL;
			    else
				type = XUNSETVAL;
			}
			if (type != XSETVAL && type != XUNSETVAL)
			    break;
			dnode = pnode;
			while (dnode != NULL)
			{
			    if (dnode->type == type && dnode->id == id)
			    {
				xvnode = (XVALUE_NODE *) dnode->data_node;
				while (xvnode != NULL)
				{
				    if (xvnode->data != NULL)
				    {
					k = strlen(xvnode->data);
					*dest = '\0';
				        strcat(dest, xvnode->data);
					dest += k;
				     }
				     xvnode = xvnode->next;
				}
			    }
			    dnode = dnode->next;
			}
			break;
		}  /* end of switch */
		node = node->next;
	    }
	}
	*dest = '\0';
}


static
char *fetch_row_data (head, type, id)
ITEM_NODE *head;
int	  type, id;
{
	static ITEM_NODE    *node = NULL;
	static ITEM_NODE    *hnode = NULL;
	static XVALUE_NODE  *xvnode = NULL;

	if (head != NULL)
	{
	    hnode = head;
	    node = head;
	    xvnode = NULL;
	}
	while (1)
	{
	    while (xvnode != NULL)
	    {
		if (xvnode->data)
		{
		    retrieve_row_data(hnode, xvnode->data, outStr2);
		    retrieve_row_data(hnode, outStr2, outStr);
		    xvnode = xvnode->next;
		    return(outStr);
		}
	    }
	    while (node != NULL)
	    {
	        if (node->type == type && node->id == id)
		    xvnode = (XVALUE_NODE *) node->data_node;
		node = node->next;
		if (xvnode)
		    break;
	    }
	    if (xvnode == NULL)
		break;
	}
	return((char *)NULL);
}


output_row_item (fd, rownode)
FILE	   *fd;
ROW_NODE   *rownode;
{
	ITEM_NODE    *hnode;
	char	     *data;
	
	if (rownode->mask)
	    return;
	hnode = (ITEM_NODE *) rownode->item_list;
	data = fetch_row_data (hnode, XOUTPUT, -1);
	while (data != NULL)
	{
	    fprintf(fd, "%s\n", data);
	    data = fetch_row_data (NULL, XOUTPUT, -1);
	}
}


rtput_row_item (rownode, node)
ROW_NODE  *rownode;
ITEM_NODE *node;
{
	FILE  *fout;
	ITEM_NODE    *hnode;
	XRADIO_NODE  *rdnode;
	int	     rdtype;
	char	     *data;

	if (rownode->mask)
	    return;
	sprintf(tmpstr, "%s_%s", UserName, defName);
	sprintf(tmpFile, "%s/%s", outDir, tmpstr);
        if ((fout = fopen(tmpFile, "w")) == NULL)
	{
	    sprintf(outDir, "/tmp");
	    sprintf(tmpFile, "%s/%s", outDir, tmpstr);
            if ((fout = fopen(tmpFile, "w")) == NULL)
	    	return;
	}
	hnode = (ITEM_NODE *) rownode->item_list;
        if (rownode->rtcmd)
	{
	    data = fetch_row_data (hnode, XRTOUT, -1);
	    while (data != NULL)
	    {

	    	fprintf(fout, "%s\n", data);
	    	data = fetch_row_data (NULL, XRTOUT, -1);
	    }
	}
        if (node->rtcmd)
	{
	    data = fetch_row_data (hnode, XRTOUT, node->id);
	    while (data != NULL)
	    {
	    	fprintf(fout, "%s\n", data);
	    	data = fetch_row_data (NULL, XRTOUT, node->id);
	    }
	}
	if ((node->type == XRADIO) && node->data_node)
	{
	    rdnode = (XRADIO_NODE *) node->data_node;
	    rdtype = 0;
	    if (rdnode->set && rdnode->set_rtout)
		rdtype = XSETRT;
	    else if (rdnode->set == 0 && rdnode->unset_rtout)
		rdtype = XUNSETRT;
	    if (rdtype)
	    {
	        data = fetch_row_data (hnode, rdtype, node->id);
	        while (data != NULL)
	        {
	            fprintf(fout, "%s\n", data);
	            data = fetch_row_data (NULL, rdtype, node->id);
		}
	    }
	}
	fclose(fout);
	sync();
	sprintf(tmpstr, "%s_%s", UserName, defName);
	macro_to_vnmr(outDir, tmpstr);
}


set_item_value (rownode, cmd)
ROW_NODE *rownode;
char     *cmd;
{
	char	*data;
	char	token[32];
	int	type, id, num;
	ITEM_NODE    *node, *hnode;
        XPANE_NODE   *pane;
        XINPUT_NODE  *in_node;
        XCHOICE_NODE  *chnode;
        XRADIO_NODE   *rdnode;

	data = get_token_and_id (cmd, token, &id, 0);
	if (data == NULL)
	    return;
	while (*data == ' ' || *data == '\t')
	    data++;
	hnode = (ITEM_NODE *) rownode->item_list;
	type = get_token_type (token);
	switch (type) {
	  case  XINPUT:
		    retrieve_row_data(hnode, data, outStr2);
		    retrieve_row_data(hnode, outStr2, outStr);
		    node = hnode;
		    while (node != NULL)
		    {
			if (node->type == XINPUT && node->id == id)
			{
			    in_node = (XINPUT_NODE *) node->data_node;
                            if (in_node->widget)
                            { 
			       if (in_node->rows == 1)
              			   XtSetArg (args[0], XtNstring, outStr);
			       else
	    		           XtSetArg(args[0],XtNsource, outStr);
			       XtSetValues(in_node->widget, args, 1);
			    }
			    break;
			}
			node = node->next;
		    }
		    break;
	  case  XCHOICE:
		    retrieve_row_data(hnode, data, outStr2);
		    retrieve_row_data(hnode, outStr2, outStr);
		    if ((int) strlen(outStr) <= 0)
			return;

		    num = atoi(outStr);
		    node = hnode;
		    while (node != NULL)
		    {
			if (node->type == type && node->id == id)
			{
			    pane = (XPANE_NODE *) node->data_node;
			    chnode = (XCHOICE_NODE *) pane->wlist;
			    while (chnode != NULL)
			    {
				if (chnode->bid == num && chnode->widget)
				{
	    		           XtSetArg(args[0],XtNset, True);
			           XtSetValues(chnode->widget, args, 1);
				   break;
				}
				chnode = chnode->next;
			    }
			    break;
			}
			node = node->next;
		    }
		    break;
		
	  case  XRADIO:
		    retrieve_row_data(hnode, data, outStr2);
		    retrieve_row_data(hnode, outStr2, outStr);
		    if ((int) strlen(outStr) <= 0)
			return;
		    num = atoi(outStr);
		    node = hnode;
		    while (node != NULL)
		    {
			if (node->type == type && node->id == id)
			{
			    rdnode = (XRADIO_NODE *) node->data_node;
			    if (rdnode->widget)
			    {
				if (num > 0)
	    		           XtSetArg(args[0],XtNset, True);
			        else
	    		           XtSetArg(args[0],XtNset, False);
			        XtSetValues(rdnode->widget, args, 1);
				break;
			    }
			}
			node = node->next;
		    }
		    break;
	}
}


exec_row_item (rownode, node)
ROW_NODE  *rownode;
ITEM_NODE *node;
{
	ITEM_NODE    *hnode;
	XVALUE_NODE  *xvnode;
	subWin_entry *wentry;
	XRADIO_NODE  *rdnode;
	int	     rdtype;
	char	     *data;

	if (rownode->mask)
	    return;
	wentry = (subWin_entry *) rownode->win_entry;
	if (wentry == NULL)
	    return;
	hnode = (ITEM_NODE *) rownode->item_list;
	if (rownode->execmd)
	{
	    data = fetch_row_data (hnode, XEXEC, -1);
	    while (data != NULL)
	    {
		row_exec_proc (rownode, data);
	    	data = fetch_row_data (NULL, XEXEC, -1);
	    }
	}
	if (node->execmd)
	{
	    data = fetch_row_data (hnode, XEXEC, node->id);
	    while (data != NULL)
	    {
		row_exec_proc (rownode, data);
	    	data = fetch_row_data (NULL, XEXEC, node->id);
	    }
	}
	if (node->type == XRADIO)
	{
	    rdnode = (XRADIO_NODE *) node->data_node;
	    rdtype = 0;
	    if (rdnode->set && rdnode->set_exec)
		rdtype = XSETEXEC;
	    else if (rdnode->set == 0 && rdnode->unset_exec)
		rdtype = XUNSETEXEC;
	    if (rdtype)
	    {
	        data = fetch_row_data (hnode, rdtype, node->id);
	        while (data != NULL)
	        {
		    row_exec_proc (rownode, data);
	    	    data = fetch_row_data (NULL, rdtype, node->id);
	        }
	    }
	}
}


output_def_file (wentry, save)
subWin_entry  *wentry;
int	      save;
{
	FILE      *fout;
	char      outPath[MAXPATHL];
	ROW_NODE  *rownode;

	sprintf(outPath, "%s/%s", outDir, outName);
	if (debug)
	    fprintf(stderr, " output macro: %s\n", outPath);
	if ((fout = fopen(outPath, "w")) == NULL)
	{
	    if (debug)
	        fprintf(stderr, " Could not open file %s\n", outPath);
            return;
	}
	rownode = wentry->vnode;
	while (rownode != NULL)
	{
	    if (rownode->show && rownode->output && rownode->item_list)
		output_row_item (fout, rownode);
	    rownode = rownode->next;
	}
	fclose(fout);
	if (!save)
	{
	    macro_to_vnmr(outDir, outName);
	}
	else
	    disp_macro_info (outPath);
}

defWin_preview_proc(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	int	k;
	char    *data;
	XVALUE_NODE  *xvnode;
	ITEM_NODE    *node, *hnode;
	ROW_NODE     *rownode;
	subWin_entry *def_entry;

	if (client_data == NULL)
	    return;
	def_entry = (subWin_entry *) client_data;
	if (previewShell == NULL)
	    create_preview_window();
	if (previewShell == NULL)
	    return;
	XtPopup(previewShell, XtGrabNone);

	sprintf(tmpstr, "Output file: %s/%s", outDir, outName);
        XtSetArg(args[0], XtNstring, tmpstr);
	XtSetValues(outTitle, args, 1);
	OlTextEditClearBuffer(outWindow);
	OlTextEditClearBuffer(infoWindow);
	rownode = def_entry->vnode;
	while (rownode != NULL)
	{
	    if (rownode->show && rownode->output && (!rownode->mask) && rownode->item_list)
	    {
		hnode = (ITEM_NODE *) rownode->item_list;
		data = fetch_row_data (hnode, XOUTPUT, -1);
		while (data != NULL)
		{
		    OlTextEditInsert(outWindow, data, strlen(data));
		    OlTextEditInsert(outWindow, "\n", 2);
	    	    data = fetch_row_data (NULL, XOUTPUT, -1);
		}
	    }
	    rownode = rownode->next;
	}
}


void
def_help_proc(w, c_data, x_event)
Widget  w;
caddr_t c_data;
XEvent  *x_event;
{
        subWin_entry   *entry;
        BUTTON_NODE    *bnode;

        if (c_data == NULL)
            return;
        entry = (subWin_entry *) c_data;
        switch (x_event->type)  {
          case LeaveNotify:
                XtSetArg(args[0], XtNstring, " ");
                XtSetValues(entry->footer, args, 1);
                break;
          case EnterNotify:
                XtSetArg (args[0], XtNuserData, &bnode);
                XtGetValues(w, args, 1);
                if (bnode != NULL && bnode->help != NULL)
                {
                   XtSetArg(args[0], XtNstring, bnode->help);
                   XtSetValues(entry->footer, args, 1);
                }
                break;
        }
}


void
clean_def_footer(w, client_data, event)
  Widget          w;
  caddr_t         client_data;
  XEvent         *event;
{
        subWin_entry   *entry;

        if (client_data == NULL)
            return;
        entry = (subWin_entry *) client_data;
        XtSetArg (args[0], XtNstring, " ");
        XtSetValues(entry->footer, args, 1);
}

void main_resize(w,data,event)
Widget    w;
caddr_t   data;
XEvent    *event;
{
	Dimension      width, height, bw, bh;

	if(event->type != ConfigureNotify)
            return;
        XtSetArg (args[0], XtNheight, &height);
        XtSetArg (args[1], XtNwidth, &width);
	XtGetValues(w, args, 2);
	bh = height - diffH;
	bw = width - diffW;
	
	n = 0;
	if (bh > viewHeight)
	{
            XtSetArg (args[n], XtNheight, bh); n++;
	}
	if (width > viewWidth)
	{
            XtSetArg (args[n], XtNwidth, bw); n++;
	}
	if (n > 0)
	    XtSetValues(dispWin, args, n);
}


build_def_window(entry)
subWin_entry   *entry;
{
     Position  posx, posy;
     Widget    form, ywidget, exitWin;
     Widget    footer;
     int       n, hlen, scroll, num;
     char	    *resource, *class;
     Dimension      width, height, bw, sw, sh;
     ITEM_NODE      *cnode;
     XBUTTON_NODE   *bnode;

     class = topClassName;
     if (entry->alignment <= 0)
     {
	entry->alignment = RIGHT;
	entry->maxRow = 12;
        sprintf(tmpstr, "%s*label*alignment", class);
        resource = get_x_resource(class, tmpstr);
        if (resource != NULL)
        {
	    if (strcmp(resource, "left") == 0)
		entry->alignment = LEFT;
	    else if (strcmp(resource, "none") == 0)
		entry->alignment = 9;
	}
        sprintf(tmpstr, "%s*visibleLines", class);
        resource = get_x_resource(class, tmpstr);
        if (resource != NULL)
        {
	    entry->maxRow = atoi(resource);
	    if (entry->maxRow > 60)
		entry->maxRow = 60;
	}
     }
     hlen = 38;
     n = 0;
     form = XtCreateManagedWidget("", formWidgetClass, topShell, args, n);

     scrolledwindow = NULL;
     if (entry->itemNum > entry->maxRow)
     {
	XtSetArg (args[0], XtNmappedWhenManaged, FALSE);
	XtSetValues(topShell, args, 1);
       
	n = 0;
        XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
        XtSetArg (args[n], XtNxResizable, TRUE);  n++;
        XtSetArg (args[n], XtNyResizable, TRUE);  n++;
        XtSetArg (args[n], XtNxOffset, 4);  n++;
        XtSetArg (args[n], XtNyOffset, 4);  n++;
	scrolledwindow = XtCreateManagedWidget("",
                scrolledWindowWidgetClass, form, args, n);
	n = 0;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNlayout, OL_NONE);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	XtSetArg (args[n], XtNvSpace, 0);  n++;
        dispWin = XtCreateManagedWidget ("", controlAreaWidgetClass,
                        scrolledwindow, args, n);
	ywidget = scrolledwindow;
	scroll = 1;
     }
     else
     {
	n = 0;
	XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyResizable, FALSE);  n++;
	XtSetArg (args[n], XtNxOffset, 0);  n++;
	XtSetArg (args[n], XtNyOffset, 4);  n++;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDCOLS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
	XtSetArg (args[n], XtNvSpace, 0);  n++;
        dispWin = XtCreateManagedWidget ("", controlAreaWidgetClass,
                        form, args, n);
	ywidget = dispWin;
	scroll = 0;
     }
 
     disp_def_list(dispWin, entry, &refWidget, scroll);

     n =0;
     XtSetArg (args[n], XtNborderWidth, 1);  n++;
     XtSetArg (args[n], XtNgravity, WestGravity);  n++;
     XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
     XtSetArg (args[n], XtNwrap, FALSE);  n++;
     XtSetArg (args[n], XtNrecomputeSize, FALSE);  n++;
     XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
     XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
     XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
     XtSetArg (args[n], XtNxResizable, TRUE);  n++;
     XtSetArg (args[n], XtNyResizable, FALSE);  n++;
     XtSetArg (args[n], XtNyAttachBottom, FALSE);  n++;
     XtSetArg (args[n], XtNcharsVisible, hlen + 1); n++;
     XtSetArg (args[n], XtNyRefWidget, ywidget); n++;
     XtSetArg (args[n], XtNxOffset, 4);  n++;
     XtSetArg (args[n], XtNyOffset, 6);  n++;
     footer = XtCreateManagedWidget ("info",
                     staticTextWidgetClass, form, args, n);

     entry->footer = footer;
     entry->shell = topShell;

     if (exit_row)
     {
	n = 0;
	XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
	XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
	XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg (args[n], XtNxResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyResizable, FALSE);  n++;
        XtSetArg (args[n], XtNyAttachBottom, TRUE);  n++;
	XtSetArg (args[n], XtNxOffset, 0);  n++;
	XtSetArg (args[n], XtNyOffset, 4);  n++;
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
	XtSetArg (args[n], XtNmeasure, 1);  n++;
        XtSetArg (args[n], XtNyRefWidget, footer); n++;
        exitWin = XtCreateManagedWidget ("", controlAreaWidgetClass,
                        form, args, n);
	create_exit_row(exitWin, exit_row);
     }

     if ( !show_remark )
     {
         XtSetArg (args[0], XtNmappedWhenManaged, FALSE);
	 XtSetValues(footer, args, 1);
     }
     XtAddEventHandler (dispWin, LeaveWindowMask, FALSE,
                                clean_def_footer, entry);
     XtRealizeWidget (topShell);

     XtSetArg (args[0], XtNwidth, &viewWidth);
     XtSetArg (args[1], XtNheight, &viewHeight);
     XtGetValues(dispWin, args, 2);
     num = 0;
     bw = 0;
     width = 40;
     if (exit_row)
     {
        XtSetArg (args[0], XtNwidth, &width);
        cnode = (ITEM_NODE *) exit_row->item_list;
	while (cnode != NULL)
	{
	    if (cnode->type == XBUTTON)
	    {
	        bnode = (XBUTTON_NODE *) cnode->data_node;
	        if (bnode->widget)
	    	{
	    	    XtGetValues(bnode->widget, args, 1);
	    	    bw = bw + width;
	    	    num++;
		}
	    }
	    cnode = cnode->next;
	}
	width = (int) (viewWidth - bw) / (num + 1);
	if (width > 0)
	{
            XtSetArg (args[0], XtNhPad, width);
            XtSetArg (args[1], XtNhSpace, width);
            XtSetValues(exit_row->rowidget, args, 2);
	}
     }
     if (scrolledwindow)
     {
        XtSetArg (args[0], XtNviewHeight, &height);
        XtSetArg (args[1], XtNviewWidth, &width);
	XtGetValues(scrolledwindow, args, 2);

        XtSetArg (args[0], XtNheight, &sh);
        XtSetArg (args[1], XtNwidth, &sw);
	XtGetValues(topShell, args, 2);

	diffW = sw - width;
	diffH = sh - height;
        XtSetArg (args[0], XtNheight, &height);
	XtGetValues(refWidget, args, 1);

        XtSetArg(args[0], XtNvStepSize, height);
        XtSetValues (scrolledwindow, args, 1);

	hlen = (int) height;
	height = height * entry->maxRow;
	sw = viewWidth + diffW;
	sh = height + diffH;
        XtSetArg (args[0], XtNheight, sh);
        XtSetArg (args[1], XtNwidth, sw);
	XtSetValues(topShell, args, 2);
	XtMapWidget(topShell);

	XtAddEventHandler(topShell,StructureNotifyMask,False,main_resize,NULL);
     }
     if (choiceExec)
  	exec_entry_choice(entry);
}


exec_entry_choice(wentry)
subWin_entry  *wentry;
{
	int	      type;
	ROW_NODE      *rnode;
	ITEM_NODE     *cnode;
	XPANE_NODE    *pane;
	XCHOICE_NODE  *chnode;

        rnode = wentry->vnode;
        while (rnode != NULL)
        {
            if (rnode->show && rnode->choiceExec)
	    {
		cnode = (ITEM_NODE *) rnode->item_list;
		while (cnode != NULL)
		{
		    type = cnode->type;
		    if (type == XCHOICE || type == XMENU || type == XABMENU)
		    {
        		pane = (XPANE_NODE *) cnode->data_node;
                        chnode = (XCHOICE_NODE *) pane->wlist;
                        while (chnode != NULL)
                        {
			    if (chnode->bid == 1)
			    {
			        if (chnode->exec)
	    			    row_exec_proc(rnode, chnode->exec);
			        break;
			    }
			    chnode = chnode->next;
			}
		    }
		    cnode = cnode->next;
		}
	    }
	    rnode = rnode->next;
	}
}


void
def_choice_proc(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	ITEM_NODE     *cnode;
	XPANE_NODE    *pane;
	ROW_NODE      *rownode;
	XCHOICE_NODE  *chnode;
	int	      val;

	if (client_data == NULL)
	    return;
        cnode = (ITEM_NODE *) client_data;
        XtSetArg (args[0], XtNuserData, &chnode);
	XtGetValues( w, args, 1);
        pane = (XPANE_NODE *) cnode->data_node;
	pane->val = chnode->bid;
	rownode = (ROW_NODE *) cnode->row_node;
        if (rownode->rtcmd || cnode->rtcmd)
	    rtput_row_item (rownode, cnode);
	if (rownode->execmd || cnode->execmd)
	    exec_row_item (rownode, cnode);
	if (chnode->exec)
	    row_exec_proc(rownode, chnode->exec);
}


void
def_menu_proc(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	ITEM_NODE     *cnode;
	ROW_NODE      *rownode;
	XPANE_NODE    *pane;
	XCHOICE_NODE  *chnode;
	int	      val;

	if (client_data == NULL)
	    return;
        cnode = (ITEM_NODE *) client_data;
        XtSetArg (args[0], XtNuserData, &val);
	XtGetValues( w, args, 1);
        pane = (XPANE_NODE *) cnode->data_node;
	pane->val = val;
	if (pane->lwidget)
	{
	    chnode = pane->wlist;
            while (chnode != NULL)
            {
	        if (chnode->bid == val)
	    	{
		   if (chnode->label != NULL)
	    	      XtSetArg(args[0], XtNstring, chnode->label);
		   else
	    	      XtSetArg(args[0], XtNstring, "  ");
        	   XtSetValues(pane->lwidget, args, 1);
		   break;
	    	}
	        chnode = chnode->next;
	    }
	}
	rownode = (ROW_NODE *) cnode->row_node;
        if (rownode->rtcmd || cnode->rtcmd)
	    rtput_row_item (rownode, cnode);
	if (rownode->execmd || cnode->execmd)
	    exec_row_item (rownode, cnode);
}


void
def_button_proc(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	ITEM_NODE     *cnode;
	ROW_NODE      *rownode;

	if (client_data == NULL)
	    return;
        cnode = (ITEM_NODE *) client_data;
	rownode = (ROW_NODE *)cnode->row_node;
        if (rownode->rtcmd || cnode->rtcmd)
	    rtput_row_item (rownode, cnode);
	if (rownode->execmd || cnode->execmd)
	    exec_row_item (rownode, cnode);
}


void
def_check_set_proc(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	ITEM_NODE     *cnode;
	ROW_NODE      *rownode;
	XRADIO_NODE   *rdnode;

	if (client_data == NULL)
	    return;
        cnode = (ITEM_NODE *) client_data;
	rdnode = (XRADIO_NODE *) cnode->data_node;
	rdnode->set = 1;
	rownode = (ROW_NODE *)cnode->row_node;
        if (rownode->rtcmd || cnode->rtcmd)
	    rtput_row_item (rownode, cnode);
	if (rownode->execmd || cnode->execmd)
	    exec_row_item (rownode, cnode);
}


void
def_check_unset_proc(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
	ITEM_NODE     *hnode, *cnode;
	ROW_NODE      *rownode;
	XRADIO_NODE   *rdnode;

	if (client_data == NULL)
	    return;
        cnode = (ITEM_NODE *) client_data;
	rownode = (ROW_NODE *)cnode->row_node;
	rdnode = (XRADIO_NODE *) cnode->data_node;
	rdnode->set = 0;
        if (rownode->rtcmd || cnode->rtcmd)
	    rtput_row_item (rownode, cnode);
	if (rownode->execmd || cnode->execmd)
	    exec_row_item (rownode, cnode);
}


static
prnt_item_name(node)
ITEM_NODE  *node;
{
	switch (node->type) {
	 case XMENU:
		strcpy (tmpstr, "Menu");
		break;
	 case XCHOICE:
		strcpy (tmpstr, "Choice");
		break;
	 case XBUTTON:
		strcpy (tmpstr, "Button");
		break;
	 case XABMENU:
		strcpy (tmpstr, "Abbreviate Menu");
		break;
	 case XRADIO:
		strcpy (tmpstr, "Check Button");
		break;
	 default:
		return;
		break;
	}
	if (node->id > 0)
	{
	     sprintf (tmpstr2, " (%d):\n", node->id);
	     strcat (tmpstr, tmpstr2);
	}
	else
	     strcat (tmpstr, ":\n");
   	OlTextEditInsert(infoWindow, tmpstr, strlen(tmpstr));
}


static
prnt_item_data(head, id, type)
ITEM_NODE  *head;
int	   id, type;
{
	char	*data;
	int	prnt_title;

	prnt_title = 1;
	data = fetch_row_data (head, type, id);
	while (data != NULL)
	{
	   if (prnt_title)
	   {
		prnt_title = 0;
		switch (type) {
		  case XRTOUT:
	   		OlTextEditInsert(infoWindow, " rtoutput: ", 11); 
			break;
		  case XEXEC:
	   		OlTextEditInsert(infoWindow, " exec:     ", 11); 
			break;
		  case XOUTPUT:
	   		OlTextEditInsert(infoWindow, " output:   ", 11); 
			break;
		  case XSETRT:
	   		OlTextEditInsert(infoWindow, " set rtout: ", 12); 
			break;
		  case XUNSETRT:
	   		OlTextEditInsert(infoWindow, " unset rtout: ", 14); 
			break;
		  case XSETEXEC:
	   		OlTextEditInsert(infoWindow, " set ecec: ", 11); 
			break;
		  case XUNSETEXEC:
	   		OlTextEditInsert(infoWindow, " unset exec: ", 13); 
			break;
		}
	    }
	    else
		OlTextEditInsert(infoWindow, "           ", 11);
	    OlTextEditInsert(infoWindow, data, strlen(data));
	    OlTextEditInsert(infoWindow, "\n", 2);
	    data = fetch_row_data (NULL, type, id);
	}
}



void  
def_footer_proc(w, client_data, event)
  Widget          w;
  caddr_t         client_data;
  XEvent         *event;
{
        subWin_entry   *entry;
	ROW_NODE       *rownode;

        if (client_data == NULL)
            return;
        entry = (subWin_entry *) client_data;
        XtSetArg (args[0], XtNuserData, &rownode);
        XtGetValues(w, args, 1);
        if (rownode == NULL || rownode->remark == NULL)
            XtSetArg (args[0], XtNstring, " ");
        else
            XtSetArg (args[0], XtNstring, rownode->remark);
        XtSetValues(entry->footer, args, 1);
}


void  
def_item_info_proc(w, client_data, event)
  Widget          w;
  caddr_t         client_data;
  XEvent         *event;
{
	if (client_data == NULL)
	    return;
	if (!disp_preview)
            return;
	disp_item_info((ROW_NODE *) client_data);
}


disp_item_info(rownode)
ROW_NODE  *rownode;
{
	XVALUE_NODE    *xvnode;
	XRADIO_NODE    *rdnode;
	ITEM_NODE      *node, *hnode;
	int	       new_item;
	char	       *data;

	OlTextEditClearBuffer(infoWindow);
	OlTextEditInsert(infoWindow,rownode->label,strlen(rownode->label));
	OlTextEditInsert(infoWindow, "\n", 2);
	if (rownode->mask)
	    return;
	hnode = (ITEM_NODE *) rownode->item_list;
	if (rownode->rtcmd)
	   prnt_item_data(hnode, -1, XRTOUT);
	if (rownode->execmd)
	   prnt_item_data(hnode, -1, XEXEC);
	if (rownode->output)
	   prnt_item_data(hnode, -1, XOUTPUT);
	node = hnode;
	while (node != NULL)
	{
	   if (node->show == 0)
	   {
		node = node->next;
		continue;
	   }
	   new_item = 1;
	   if (node->rtcmd)
	   {
		if (new_item)
		{
		    prnt_item_name (node);
		    new_item = 0;
		}
		prnt_item_data (hnode, node->id, XRTOUT);
	   }
	   if (node->execmd)
	   {
		if (new_item)
		{
		    prnt_item_name (node);
		    new_item = 0;
		}
		prnt_item_data (hnode, node->id, XEXEC);
	   }
	   if (node->type == XRADIO)
	   {
		rdnode = (XRADIO_NODE *) node->data_node;
		if ( rdnode->set_exec || rdnode->unset_exec || 
			 rdnode->set_rtout || rdnode->unset_rtout )
		{
		    if (new_item)
		    {
		    	prnt_item_name (node);
		        new_item = 0;
		    }
		    if (rdnode->set_rtout)
			prnt_item_data (hnode, node->id, XSETRT);
		    if (rdnode->unset_rtout)
			prnt_item_data (hnode, node->id, XUNSETRT);
		    if (rdnode->set_exec)
			prnt_item_data (hnode, node->id, XSETEXEC);
		    if (rdnode->unset_exec)
			prnt_item_data (hnode, node->id, XUNSETEXEC);
		}
	   }
	   node = node->next;
	}
}


void  
set_item_footer(w, client_data, event)
  Widget          w;
  caddr_t         client_data;
  XEvent         *event;
{
        subWin_entry   *entry;
	ITEM_NODE      *node;

        if (client_data == NULL)
            return;
        entry = (subWin_entry *) client_data;
        XtSetArg (args[0], XtNuserData, &node);
        XtGetValues(w, args, 1);
        if ((node != NULL) && (node->remark != NULL))
	{
            XtSetArg (args[0], XtNstring, node->remark);
            XtSetValues(entry->footer, args, 1);
	}
}


disp_macro_info(file_name)
char  *file_name;
{
	FILE   *fd;

	if (!disp_preview)
	    return;
        if ((fd = fopen(file_name, "r")) == NULL)
	    return;

	OlTextEditInsert(infoWindow, file_name, strlen(file_name));
	OlTextEditInsert(infoWindow, ":\n", 3);
	while (fgets(tmpstr, 256, fd) != NULL)
	{
	    OlTextEditInsert(infoWindow, tmpstr, strlen(tmpstr));
	}
	fclose(fd);
}


void  
clear_item_footer(w, client_data, event)
  Widget          w;
  caddr_t         client_data;
  XEvent         *event;
{
        subWin_entry   *entry;
	ITEM_NODE      *node;
	ROW_NODE       *rownode;

        if (client_data == NULL)
            return;
        entry = (subWin_entry *) client_data;
        XtSetArg (args[0], XtNuserData, &node);
        XtGetValues(w, args, 1);
	if (node == NULL)
	    return;
	rownode = (ROW_NODE *) node->row_node;
        if (rownode == NULL || rownode->remark == NULL)
            XtSetArg (args[0], XtNstring, " ");
        else
            XtSetArg (args[0], XtNstring, rownode->remark);
        XtSetValues(entry->footer, args, 1);
}


make_def_label(pwidget, rownode, maxlen, alignment)
Widget	pwidget;
ROW_NODE *rownode;
int	maxlen, alignment;
{
	int	len, k, n;
	char	*label;
	Widget  lwidget;

	label = rownode->label;
	len = strlen(label);
	k = maxlen - len;
	if (k < 0)
	    k = 0;
	if (alignment == LEFT)
	{
	    strcpy(tmpstr, label);
	    while (k > 0)
	    {
		tmpstr[len] = ' ';
		len++;
		k--;
	    }
	    tmpstr[len] = '\0';
	}
	else if (alignment == RIGHT)
	{
	     tmpstr[k] = '\0';
	     while (k > 0)
	     {
		 k--;
		 tmpstr[k] = ' ';
	     }
	     strcat(tmpstr, label);
	     strcat(tmpstr, " ");
	}
	else
	     strcpy(tmpstr, label);
        n = 0;
        XtSetArg (args[n], XtNgravity, WestGravity);  n++;
        XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
        XtSetArg (args[n], XtNstring, tmpstr);  n++;
	XtSetArg(args[n], XtNstrip, FALSE); n++;
        rownode->lwidget = XtCreateManagedWidget("",
                        staticTextWidgetClass, pwidget, args, n);
        XtAddEventHandler (rownode->lwidget, ButtonPressMask, FALSE, 
			def_item_info_proc, (XtPointer) rownode);
}


make_def_input(pwidget, wentry, node)
Widget  	pwidget;
subWin_entry    *wentry;
ITEM_NODE  	*node;
{
	int	n;
	XINPUT_NODE  *in_node;

	if (node->data_node == NULL)
	    return;
	in_node = (XINPUT_NODE *) node->data_node;
	if (in_node->rows <= 1)
	{
           n = 0;
	   XtSetArg(args[n], XtNcharsVisible, in_node->cols + 1); n++;
	   if (in_node->data)
              XtSetArg (args[n], XtNstring, in_node->data);
	   else
              XtSetArg (args[n], XtNstring, "  ");
	   n++;
           in_node->widget = XtCreateManagedWidget("text",
                        textFieldWidgetClass, pwidget, args, n);
	}
	else
	{
	   n = 0;
	   XtSetArg(args[n], XtNlinesVisible, in_node->rows); n++;
	   XtSetArg(args[n], XtNcharsVisible, in_node->cols); n++;
	   XtSetArg(args[n], XtNborderWidth, 1); n++;
	   if (in_node->data)
	   {
	        XtSetArg(args[n], XtNsource, in_node->data); n++;
	   }
	   in_node->widget =  XtCreateManagedWidget("",
                 textEditWidgetClass, pwidget, args, n);
	}
	if (node->remark != NULL)
	{
	    show_remark = 1;
            XtSetArg (args[0], XtNuserData, (XtPointer) node);
            XtSetValues(in_node->widget, args, 1);
            XtAddEventHandler (in_node->widget, EnterWindowMask, FALSE,
			 set_item_footer, wentry);
            XtAddEventHandler (in_node->widget, LeaveWindowMask, FALSE,
			 clear_item_footer, wentry);
	}
}


make_def_menu(snode)
ITEM_NODE    *snode;
{
	int	 k, len, n, first_time;
	Widget   menuBut, menuPane, widget;
	Widget   pwidget;
	ROW_NODE   *rownode;
	XPANE_NODE *pane;
	XCHOICE_NODE  *chnode;
	WidgetList    clist;

	if (snode->data_node == NULL)
	    return;
	rownode = (ROW_NODE *)snode->row_node;
	pwidget = rownode->rowidget;
        first_time = 1;
	pane = (XPANE_NODE *) snode->data_node;
	if (pane->abWidget != NULL)
        {
             first_time = 0;
             menuBut = pane->abWidget;
        }
	else
        {
             n = 0;
             menuBut = XtCreateManagedWidget ("menu",
                        abbrevMenuButtonWidgetClass, pwidget, args, n);
             pane->abWidget = menuBut;
        }
        n = 0;
        XtSetArg (args[n], XtNmenuPane, (XtArgVal)&menuPane); n++;
        XtGetValues(menuBut, args, n);
	if (!first_time)
	{
            n = 0;
            XtSetArg(args[n], XtNchildren, &clist); n++;
            XtSetArg(args[n], XtNnumChildren, &k); n++;
            XtGetValues (menuPane, args, n);
            if (debug)
                fprintf(stderr, " menu: %d items\n", k);
            for (n = 0; n < k; n++)
            {
                XtDestroyWidget (*clist);
                clist++;
            }
        }

	pane->val = 1;
	chnode = pane->wlist;
	k = 0;
	len = 0;
        while (chnode != NULL)
        {
	    if (chnode->label != NULL)
	    {
	       if ((int)strlen(chnode->label) > len)
		   len = strlen(chnode->label);
               k++;
	    }
	    else
	       chnode->bid = -1;
            chnode = chnode->next;
	}
	if (k <= 0)
	{
	    if (!first_time)
            {
		XtUnmapWidget (menuBut);
                if (pane->lwidget != NULL)
		   XtUnmapWidget (pane->lwidget);
	    }
	    else
	    {
		n = 0;
                XtSetArg (args[n], XtNmappedWhenManaged, FALSE); n++;
                XtSetValues(menuBut, args, n);
	    }
	    return;
	}

	chnode = pane->wlist;
	k = 1;
        while (chnode != NULL)
        {
	    if (chnode->label != NULL)
	    {
	       chnode->bid = k;
               n = 0;
               XtSetArg (args[n], XtNlabel, chnode->label); n++;
               XtSetArg (args[n], XtNuserData, (XtPointer)k); n++;
               chnode->widget = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, menuPane, args, n);
               XtAddCallback(chnode->widget, XtNselect, def_menu_proc, snode);
               k++;
	    }
            chnode = chnode->next;
	}
	if ((pane->lwidget == NULL) && (snode->type != XABMENU))
	{
	    chnode = pane->wlist;
	    while (chnode != NULL)
	    {
		if (chnode->bid == 1)
		    break;
		chnode = chnode->next;
	    }
	    k = strlen(chnode->label);
	    strcpy(tmpstr, chnode->label);
	    while (k < len)
	    {
	        tmpstr[k] = ' ';
	        k++;
	    }
	    tmpstr[k] = '\0';
            n = 0;
            XtSetArg (args[n], XtNgravity, WestGravity);  n++;
            XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
            XtSetArg(args[n], XtNstring, tmpstr); n++;
            XtSetArg(args[n], XtNrecomputeSize, TRUE); n++;
	    XtSetArg(args[n], XtNstrip, FALSE); n++;
            pane->lwidget = XtCreateManagedWidget("",
                        staticTextWidgetClass, pwidget, args, n);
	}
	if (snode->remark != NULL)
	{
	    show_remark = 1;
            XtSetArg (args[0], XtNuserData, (XtPointer) snode);
            XtSetValues(menuBut, args, 1);
            XtAddEventHandler (menuBut, EnterWindowMask, FALSE,
			 set_item_footer, (XtPointer) rownode->win_entry);
            XtAddEventHandler (menuBut, LeaveWindowMask, FALSE,
			 clear_item_footer, (XtPointer) rownode->win_entry);
	}
	if (!first_time)
	{
	    XtMapWidget (menuBut);
            if (pane->lwidget != NULL)
	   	XtMapWidget (pane->lwidget);
	}
}


make_def_choice(pwidget, wentry, snode)
Widget  	pwidget;
subWin_entry    *wentry;
ITEM_NODE  	*snode;
{
	int	   k;
	Widget     exwidget;
	XPANE_NODE *pane;
	XCHOICE_NODE  *chnode;
	ROW_NODE   *rnode;

	if (snode->data_node == NULL)
	    return;
	pane = (XPANE_NODE *) snode->data_node;

	n = 0;
        exwidget = XtCreateManagedWidget ("choice",
                        exclusivesWidgetClass, pwidget, args, n);
	pane->val = 1;
	chnode = pane->wlist;
	k = 1;
        while (chnode != NULL)
        {
	  if (chnode->label != NULL)
	  {
	     chnode->bid = k;
             n = 0;
             XtSetArg (args[n], XtNuserData, (XtPointer) chnode); n++;
             XtSetArg (args[n], XtNlabel, chnode->label); n++;
             chnode->widget = XtCreateManagedWidget ("",
                        rectButtonWidgetClass, exwidget, args, n);
             XtAddCallback(chnode->widget, XtNselect, def_choice_proc, snode);
	     if (chnode->exec != NULL)
	     {
		 choiceExec = 1;
		 rnode = (ROW_NODE *) snode->row_node;
		 rnode->choiceExec = 1;
	     }
             k++;
          }
	  else
	     chnode->bid = -1;
	  chnode = chnode->next;
        }
	if (snode->remark != NULL)
	{
	    show_remark = 1;
            XtSetArg (args[0], XtNuserData, (XtPointer) snode);
            XtSetValues(exwidget, args, 1);
            XtAddEventHandler (exwidget, EnterWindowMask, FALSE,
			 set_item_footer, wentry);
            XtAddEventHandler (exwidget, LeaveWindowMask, FALSE,
			 clear_item_footer, wentry);
	}
}

make_def_text(pwidget, wentry, node)
Widget  	pwidget;
subWin_entry    *wentry;
ITEM_NODE  	*node;
{
	int	n;
	XTEXT_NODE  *txnode;

	if (node->data_node == NULL)
	    return;
	txnode = (XTEXT_NODE *) node->data_node;
	if (txnode->data == NULL)
	    return;
	n = 0;
	XtSetArg (args[n], XtNgravity, WestGravity);  n++;
	XtSetArg (args[n], XtNstrip, False);  n++;
 	XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
	XtSetArg(args[n], XtNstring, txnode->data); n++;
	txnode->widget = XtCreateManagedWidget("",
                        staticTextWidgetClass, pwidget, args, n);
	if (node->remark != NULL)
	{
	    show_remark = 1;
            XtSetArg (args[0], XtNuserData, (XtPointer) node);
            XtSetValues(txnode->widget, args, 1);
            XtAddEventHandler (txnode->widget, EnterWindowMask, FALSE,
			 set_item_footer, wentry);
            XtAddEventHandler (txnode->widget, LeaveWindowMask, FALSE,
			 clear_item_footer, wentry);
	}
}


make_def_button(pwidget, node)
Widget  	pwidget;
ITEM_NODE  	*node;
{
	int	n;
	XBUTTON_NODE  *bnode;
	ROW_NODE      *rownode;

	if (node->data_node == NULL)
	    return;
	bnode = (XBUTTON_NODE *) node->data_node;
	if (bnode->label == NULL)
	    return;
	rownode = (ROW_NODE *)node->row_node;
        n = 0;
        XtSetArg (args[n], XtNlabel, bnode->label); n++;
        bnode->widget = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, pwidget, args, n);
        XtAddCallback(bnode->widget, XtNselect, def_button_proc, node);
	if (node->remark != NULL)
	{
	    show_remark = 1;
            XtSetArg (args[0], XtNuserData, (XtPointer) node);
            XtSetValues(bnode->widget, args, 1);
            XtAddEventHandler (bnode->widget, EnterWindowMask, FALSE,
			 set_item_footer, (XtPointer)rownode->win_entry);
            XtAddEventHandler (bnode->widget, LeaveWindowMask, FALSE,
			 clear_item_footer, (XtPointer)rownode->win_entry);
	}
}


make_def_checkbutton(pwidget, wentry, node)
Widget  	pwidget;
subWin_entry    *wentry;
ITEM_NODE  	*node;
{
	int	n;
	XRADIO_NODE  *rdnode;

	rdnode = (XRADIO_NODE *) node->data_node;
	if (rdnode == NULL || rdnode->label == NULL)
	   return;
	n = 0;
        XtSetArg (args[n], XtNlabel, rdnode->label); n++;
        rdnode->widget = XtCreateManagedWidget ("",
                        rectButtonWidgetClass, pwidget, args, n);
        XtAddCallback(rdnode->widget, XtNselect, def_check_set_proc, node);
        XtAddCallback(rdnode->widget, XtNunselect, def_check_unset_proc, node);
        XtSetArg (args[0], XtNuserData, (XtPointer) node);
        XtSetValues(rdnode->widget, args, 1);
	if (node->remark != NULL)
	{
	    show_remark = 1;
            XtAddEventHandler (rdnode->widget, EnterWindowMask, FALSE,
			 set_item_footer, wentry);
            XtAddEventHandler (rdnode->widget, LeaveWindowMask, FALSE,
			 clear_item_footer, wentry);
	}
}



disp_def_list(parent, win_entry, retWidget, scroll)
 Widget  parent, *retWidget;
 subWin_entry    *win_entry;
 int		 scroll;
{
        Widget       w1;
        int          k, maxlen, m;
	ROW_NODE     *rownode, *row2;
	ITEM_NODE    *cnode, *dnode;


        *retWidget = NULL;
        rownode = win_entry->vnode;
	maxlen = 0;
        while (rownode != NULL)
        {
            if (rownode->show && rownode->label != NULL)
	    {
		if ((int) strlen(rownode->label) > maxlen)
		    maxlen = strlen(rownode->label);
	    }
	    rownode = rownode->next;
	}
	if (maxlen > MAXPATHL)
	    maxlen = MAXPATHL;
        rownode = win_entry->vnode;
	choiceExec = 0;
        while (rownode != NULL)
        {
            if (!rownode->show)
            {
                rownode = rownode->next;
                continue;
            }
/**
            if (scroll && rownode == exit_row)
**/
            if (rownode == exit_row)
            {
                rownode = rownode->next;
                continue;
            }
	    /* To check if there is anything can be displayed */
	    k = 0;
	    if (rownode->label != NULL)
		k = 2;
	    cnode = (ITEM_NODE *) rownode->item_list;
	    while (cnode != NULL)
	    {
		switch (cnode->type)  {
		 case XINPUT:
		 case XMENU:
		 case XCHOICE:
		 case XRADIO:
		 case XABMENU:
		 case XTEXT:
			   if (cnode->show && (cnode->data_node != NULL))
			       k = 2;
			   break;
		 case XBUTTON:
			   if (cnode->show && (cnode->data_node != NULL))
			   {
			       if (k == 0)
			          k = 1;
			   }
			   break;
		}
		if (k)
		    break;
		cnode = cnode->next;
	    }
	    if (k == 0) /* nothing needs to be displayed */
	    {
                rownode = rownode->next;
                continue;
	    }

            n = 0;
            XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
            XtSetArg (args[n], XtNmeasure, 1);  n++;
            XtSetArg (args[n], XtNvSpace, 0);  n++;
            XtSetArg (args[n], XtNvPad, 4);  n++;
            w1 = XtCreateManagedWidget("",
                        controlAreaWidgetClass, parent, args, n);
	    rownode->rowidget = w1;
	    if (rownode->label)
	       make_def_label(w1,rownode, maxlen,win_entry->alignment);
	    cnode = (ITEM_NODE *) rownode->item_list;
	    while (cnode != NULL)
	    {
		switch (cnode->type)  {
		  case XINPUT:
			  if (cnode->show)
			     make_def_input(w1, win_entry, cnode);
			  break;
		  case XMENU:
		  case XABMENU:
			  if (cnode->show)
			     make_def_menu(cnode);
			  *retWidget = w1;
			  break;
		  case XCHOICE:
			  if (cnode->show)
			     make_def_choice(w1, win_entry, cnode);
			  if (*retWidget == NULL)
			     *retWidget = w1;
			  break;
		  case XTEXT:
			  if (cnode->show)
			     make_def_text(w1, win_entry, cnode);
			  break;
		  case XBUTTON:
			  if (cnode->show)
			     make_def_button(w1, cnode);
			  break;
		  case XRADIO:
			  if (cnode->show)
			     make_def_checkbutton(w1, win_entry, cnode);
			  break;
		}
		cnode = cnode->next;
	    }

            XtSetArg (args[0], XtNuserData, (XtPointer) rownode);
            XtSetValues(w1, args, 1);
            XtAddEventHandler (w1, EnterWindowMask, FALSE, def_footer_proc,
				 win_entry);
	    if (rownode->remark)
	    	show_remark = 1;
            rownode = rownode->next;
        }
        if (*retWidget == NULL)
            *retWidget = w1;
}


create_exit_row(parent, rnode)
 Widget     parent;
 ROW_NODE   *rnode;
{
	ITEM_NODE    *cnode;

	cnode = (ITEM_NODE *) rnode->item_list;
	while (cnode != NULL)
	{
	   switch (cnode->type)  {
	     case XBUTTON:
		  	if (cnode->show)
		     	    make_def_button(parent, cnode);
			break;
	   }
	   cnode = cnode->next;
	}
	rnode->rowidget = parent;
        XtSetArg (args[0], XtNuserData, (XtPointer) rnode);
        XtSetValues(parent, args, 1);
        XtAddEventHandler (parent, EnterWindowMask, FALSE, def_footer_proc,
				(XtPointer)rnode->win_entry);
}



reset_row_item(rownode)
ROW_NODE   *rownode;
{
	ITEM_NODE    *node;
        XPANE_NODE  *pane;
        XINPUT_NODE *in_node;
        XCHOICE_NODE  *chnode;
        XBUTTON_NODE  *xbutton;
        XRADIO_NODE   *rdbutton;


	node = (ITEM_NODE *) rownode->item_list;
	while (node != NULL)
	{
	    switch (node->type) {
	     case XINPUT:
			in_node = (XINPUT_NODE *) node->data_node;
                        if (in_node->widget)
                        {
			    if (in_node->rows == 1)
			    {
	   		        if (in_node->data)
              			   XtSetArg (args[0],XtNstring,in_node->data);
	   		        else
              			   XtSetArg (args[0], XtNstring, "");
			    }
			    else
			    {
	   		       if (in_node->data)
	    		           XtSetArg(args[0],XtNsource,in_node->data);
			       else
	    		           XtSetArg(args[0], XtNsource, "");
			    }
			    XtSetValues(in_node->widget, args, 1);
			}
			break;
	     case XMENU:
			pane = (XPANE_NODE *) node->data_node;
			pane->val = 1;
                        chnode = (XCHOICE_NODE *) pane->wlist;
                        while (chnode != NULL)
                        {
			    if (chnode->bid == 1)
			    {
			       XtSetArg (args[0],XtNstring,chnode->label);
			       if (pane->lwidget)
			  	   XtSetValues(pane->lwidget, args, 1);
			       break;
			    }
			    chnode = chnode->next;
			}
			break;
	     case XCHOICE:
			pane = (XPANE_NODE *) node->data_node;
			pane->val = 1;
                        chnode = (XCHOICE_NODE *) pane->wlist;
                        while (chnode != NULL)
                        {
			    if (chnode->bid == 1)
			    {
			  	XtSetArg (args[0], XtNset, TRUE);
			  	XtSetValues(chnode->widget, args, 1);
			       break;
			    }
			    chnode = chnode->next;
			}
			break;
	     case XRADIO:
			rdbutton = (XRADIO_NODE *) node->data_node;
                        if (rdbutton->widget)
			{
			    if (rdbutton->init_set)
			  	XtSetArg (args[0], XtNset, True);
			    else
			  	XtSetArg (args[0], XtNset, False);
			    XtSetValues(rdbutton->widget, args, 1);
			    rdbutton->set = rdbutton->init_set;
			}
			break;
	    }
	    node = node->next;
	}
}



reset_def_list(entry)
subWin_entry  *entry;
{
	ROW_NODE     *rownode;

	rownode = entry->vnode;
	while (rownode != NULL)
	{
	    if (rownode->show && rownode->item_list)
		reset_row_item (rownode);
	    rownode = rownode->next;
	}
}


mask_def_row(rnode, act)
ROW_NODE     *rnode;
int	     act;
{
	ITEM_NODE     *node;
        XPANE_NODE    *pane;
        XINPUT_NODE   *in_node;
        XCHOICE_NODE  *chnode;
        XBUTTON_NODE  *xbutton;
        XRADIO_NODE   *rdbutton;
	Widget	      twidget;

	if (act == BUT_MASK)
	    rnode->mask = 1;
	else
	    rnode->mask = 0;
	if (rnode->lwidget)
	{
	    if (rnode->mask)
                XtSetArg(args[0], XtNsensitive, FALSE);
            else
                XtSetArg(args[0], XtNsensitive, TRUE);
	    XtSetValues(rnode->lwidget, args, 1);
	}
	node = (ITEM_NODE *) rnode->item_list;
	while (node != NULL)
	{
	    twidget = NULL;
	    switch (node->type) {
	     case XINPUT:
			in_node = (XINPUT_NODE *) node->data_node;
			if (in_node->widget && in_node->rows > 1)
			    twidget = in_node->widget;
			else if (in_node->widget) /* textFieldWidgetClass */
			{
			    XtSetArg (args[0], XtNtextEditWidget, &twidget);
            		    XtGetValues(in_node->widget, args, 1);
			}
			break;
	     case XMENU:
			pane = (XPANE_NODE *) node->data_node;
			twidget = pane->abWidget;
			break;
	     case XCHOICE:
			pane = (XPANE_NODE *) node->data_node;
                        chnode = (XCHOICE_NODE *) pane->wlist;
			if (act == BUT_MASK)
     			   XtSetArg(args[0], XtNsensitive, FALSE);
			else
     			   XtSetArg(args[0], XtNsensitive, TRUE);
                        while (chnode != NULL)
                        {
			    XtSetValues(chnode->widget, args, 1);
			    chnode = chnode->next;
			}
			break;
	     case XRADIO:
			rdbutton = (XRADIO_NODE *) node->data_node;
			twidget = rdbutton->widget;
			break;
	     case XBUTTON:
			xbutton = (XBUTTON_NODE *) node->data_node;
			twidget = xbutton->widget;
			break;
	    }
	    if (twidget)
	    {
		if (act == BUT_MASK)
                    XtSetArg(args[0], XtNsensitive, FALSE);
                else
                    XtSetArg(args[0], XtNsensitive, TRUE);
		XtSetValues(twidget, args, 1);
	    }
	    node = node->next;
	}
}


pShell_map(w, client_data, event)
  Widget          w;
  caddr_t         client_data;
  XEvent          *event;
{
        if (event->type == UnmapNotify)
            disp_preview = 0;
        else if (event->type == MapNotify)
	{
            disp_preview = 1;
	}
}


void
close_preview_proc(w, c_data, d_data)
Widget  w;
caddr_t c_data;
XtPointer  d_data;
{
	XtPopdown (previewShell);
}


void
clear_preview_window(w, c_data, d_data)
Widget  w;
caddr_t c_data;
XtPointer  d_data;
{
	int	num;

	num = (int) c_data;
	if (num == 1)
	{
	   OlTextEditClearBuffer(infoWindow);
	}
	else if (num == 2)
	   OlTextEditClearBuffer(outWindow);
}


create_preview_window()
{
     int      k, rows;
     Widget   form, scroll, cwidget, bwidget;
     char     *resource;

     sprintf(tmpstr, "%s Preview", defName);
     n = 0;
     XtSetArg(args[n], XtNallowShellResize, TRUE);  n++;
     XtSetArg(args[n], XtNresizeCorners, TRUE);  n++;
     XtSetArg(args[n], XtNx, 400);  n++;
     XtSetArg(args[n], XtNy, 420);  n++;
     XtSetArg(args[n], XtNtitle, tmpstr);  n++;
     XtSetArg(args[n], XtNwindowGroup, XtUnspecifiedWindowGroup); n++;
     previewShell = XtCreatePopupShell("Preview",
			 transientShellWidgetClass, topShell, args, n);
     n = 0;
     form = XtCreateManagedWidget("", formWidgetClass, previewShell, args, n);
     n = 0;
     XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
     XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
     XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
     XtSetArg (args[n], XtNxResizable, TRUE);  n++;
     XtSetArg (args[n], XtNyResizable, FALSE);  n++;
     cwidget = XtCreateManagedWidget("",
                     controlAreaWidgetClass, form, args, n);

     n = 0;
     XtSetArg (args[n], XtNgravity, WestGravity);  n++;
     XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
     XtSetArg(args[n], XtNstring, "Item info:"); n++;
     bwidget = XtCreateManagedWidget ("text",
                     staticTextWidgetClass, cwidget, args, n);
     n = 0;
     XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
     XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
     XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
     XtSetArg (args[n], XtNxResizable, TRUE);  n++;
     XtSetArg (args[n], XtNborderWidth, 1); n++;
     XtSetArg (args[n], XtNyRefWidget, cwidget); n++;
     XtSetArg (args[n], XtNyResizable, FALSE);  n++;

     scroll = XtCreateManagedWidget("sw",
                        scrolledWindowWidgetClass, form, args, n);
     rows = 6;
     resource = get_x_resource("Glide", "*Glide*debug*item*rows");
     if (resource != NULL)
	 rows = atoi(resource);
     n = 0;
     XtSetArg(args[n], XtNsensitive, TRUE);   n++;
     XtSetArg(args[n], XtNcharsVisible, 60); n++;
     XtSetArg(args[n], XtNlinesVisible, rows); n++;
     XtSetArg(args[n], XtNwrapMode, OL_WRAP_OFF); n++;
     infoWindow = (TextEditWidget) XtCreateManagedWidget("text",
                        textEditWidgetClass, scroll, args, n);
     n = 0;
     XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
     XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
     XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
     XtSetArg (args[n], XtNxResizable, TRUE);  n++;
     XtSetArg (args[n], XtNyResizable, FALSE);  n++;
     XtSetArg (args[n], XtNyRefWidget, scroll); n++;
     cwidget = XtCreateManagedWidget("",
                     controlAreaWidgetClass, form, args, n);
     n = 0;
     XtSetArg (args[n], XtNgravity, WestGravity);  n++;
     XtSetArg (args[n], XtNalignment, OL_LEFT);  n++;
     XtSetArg(args[n], XtNstring, "Output:"); n++;
     outTitle = XtCreateManagedWidget ("text",
                     staticTextWidgetClass, cwidget, args, n);

     n = 0;
     XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
     XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
     XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
     XtSetArg (args[n], XtNxResizable, TRUE);  n++;
     XtSetArg (args[n], XtNborderWidth, 1); n++;
     XtSetArg (args[n], XtNyRefWidget, cwidget); n++;
     XtSetArg (args[n], XtNyResizable, True);  n++;
     scroll = XtCreateManagedWidget("sw",
                        scrolledWindowWidgetClass, form, args, n);
     rows = 9;
     resource = get_x_resource("Glide", "*Glide*debug*output*rows");
     if (resource != NULL)
	 rows = atoi(resource);
     n = 0;
     XtSetArg(args[n], XtNsensitive, TRUE);   n++;
     XtSetArg(args[n], XtNcharsVisible, 60); n++;
     XtSetArg(args[n], XtNlinesVisible, rows); n++;
     XtSetArg(args[n], XtNwrapMode, OL_WRAP_OFF); n++;
     outWindow = (TextEditWidget) XtCreateManagedWidget("text",
                        textEditWidgetClass, scroll, args, n);
     n = 0;
     XtSetArg (args[n], XtNyAddHeight, TRUE); n++;
     XtSetArg (args[n], XtNxAddWidth, TRUE); n++;
     XtSetArg (args[n], XtNxAttachRight, TRUE); n++;
     XtSetArg (args[n], XtNxResizable, TRUE);  n++;
     XtSetArg (args[n], XtNyAttachBottom, TRUE);  n++;
     XtSetArg (args[n], XtNyRefWidget, scroll); n++;
     XtSetArg (args[n], XtNhPad, 20);  n++;
     XtSetArg (args[n], XtNhSpace, 20); n++;
     cwidget = XtCreateManagedWidget("",
                     controlAreaWidgetClass, form, args, n);
     n = 0;
     XtSetArg (args[n], XtNlabel, "Clear Item"); n++;
     bwidget = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, cwidget, args, n);
     XtAddCallback(bwidget, XtNselect, clear_preview_window, 1);
     n = 0;
     XtSetArg (args[n], XtNlabel, "Clear Output"); n++;
     bwidget = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, cwidget, args, n);
     XtAddCallback(bwidget, XtNselect, clear_preview_window, 2);

     n = 0;
     XtSetArg (args[n], XtNlabel, "Close"); n++;
     bwidget = XtCreateManagedWidget("button",
                        oblongButtonWidgetClass, cwidget, args, n);
     XtAddCallback(bwidget, XtNselect, close_preview_proc, NULL);
     XtAddEventHandler(previewShell,SubstructureNotifyMask, False, 
			pShell_map, NULL);
}


