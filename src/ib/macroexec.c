/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/


/* 
 */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <setjmp.h>

#ifdef __OBJECTCENTER__
#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#endif
#include <stdarg.h>
#endif

#include "stderr.h"
#include "macroexec.h"
#include "magicfuncs.h"

#ifdef CSI
#include "csicommands.h"
#else
#include "ibcommands.h"
#endif
#include "msgprt.h"
#include "initstart.h"

extern void FlushGraphics();
static jmp_buf setjmp_env;
static int setjmp_set = FALSE;

MacroExec *macroexec = NULL;

MacroExec::MacroExec()
{
    char buf[100];
    char initname[1024];	// init file
    const int maxrows = 10;
    Panel panel;
    Panel_item widget;
    int xitempos = 5;
    int xpos;
    int yitempos = 5;
    int ypos;

    frame = NULL;
    popup = NULL;
    recording = FALSE;

    // Get the initialized file of window position
    (void)init_get_win_filename(initname);

    // Get the position of the control panel
    if (init_get_val(initname, "MACRO_CTL", "dd", &xpos, &ypos) == NOT_OK){
	xpos = 100;
	ypos = 100;
    }

    frame = xv_create(NULL, FRAME,
		      NULL);

    popup = xv_create(frame, FRAME_CMD,
		      XV_X, xpos,
		      XV_Y, ypos,
		      FRAME_LABEL, "Macros",
		      FRAME_CMD_PUSHPIN_IN, TRUE,
		      FRAME_SHOW_RESIZE_CORNER, TRUE,
		      NULL);

    panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

    record_widget =
    xv_create(panel, PANEL_CHOICE,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_LABEL_STRING, "Record:",
	      PANEL_CHOICE_STRINGS, "Off", "On", NULL,
	      PANEL_NOTIFY_PROC, MacroExec::record_callback,
	      NULL);

    xitempos += (int)xv_get(record_widget, XV_WIDTH) + 20;
    list_widget = widget =
    xv_create(panel, PANEL_BUTTON,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_LABEL_STRING, "List",
	      PANEL_NOTIFY_PROC, MacroExec::list_callback,
	      NULL);

    // xitempos += (int)xv_get(list_widget, XV_WIDTH) + 20;
    // widget =
    // xv_create(panel, PANEL_BUTTON,
	//       XV_X, xitempos,
	//       XV_Y, yitempos,
	//       PANEL_LABEL_STRING, "Resume",
	//       PANEL_NOTIFY_PROC, MacroExec::resume_callback,
	//       NULL);

    xitempos += (int)xv_get(widget, XV_WIDTH) + 5;
    exec_widget =
    xv_create(panel, PANEL_BUTTON,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_LABEL_STRING, "Execute",
	      PANEL_NOTIFY_PROC, MacroExec::exec_callback,
	      NULL);
    xv_set(panel, PANEL_DEFAULT_ITEM, exec_widget, NULL);

    yitempos += (int)xv_get(record_widget, XV_HEIGHT) + 5;
    xitempos = 5;
    name_widget =
    xv_create(panel, PANEL_TEXT,
	      XV_X, xitempos,
	      XV_Y, yitempos,
	      PANEL_VALUE_DISPLAY_LENGTH, 25,
	      PANEL_LABEL_STRING, "Name:",
	      PANEL_VALUE, "",
	      PANEL_NOTIFY_STRING, "\r",
	      PANEL_NOTIFY_PROC, MacroExec::name_callback,
	      NULL);

    window_fit_height(panel);
    yitempos = (int)xv_get(panel, XV_HEIGHT);
    listing_widget =
    xv_create (popup, TEXTSW,
	       TEXTSW_INSERT_MAKES_VISIBLE, TEXTSW_ALWAYS,
	       TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
	       WIN_INHERIT_COLORS, FALSE,
	       WIN_KBD_FOCUS, TRUE,
	       XV_X, 0,
	       XV_Y, yitempos,
	       /*XV_WIDTH, 500,*/
	       XV_HEIGHT, 200,
	       0);

    magical_register_user_func_finder(getFunc);

    window_fit(listing_widget);
    window_fit(popup);
    window_fit(frame);
    xv_set(popup, XV_SHOW, TRUE, NULL);
}

//
//	Show the output file format properties window.
//	(STATIC)
//
void
MacroExec::show_window()
{
    if (macroexec == NULL){
	macroexec = new MacroExec;
    }else{
	xv_set(macroexec->popup, XV_SHOW, TRUE, NULL);
    }
}

//
//	Add stuff to the current macro.
//
void
MacroExec::record(char *format, ...)
{
    if (macroexec == NULL || format == NULL || !recording){
	return;
    }
    
    char msgbuf[1024];	// Message buffer
    va_list vargs;	// Variable argument pointer

    va_start(vargs, format);
    (void)vsprintf(msgbuf, format, vargs);
    va_end(vargs);

    int point = (int)xv_get(listing_widget, TEXTSW_INSERTION_POINT);
    textsw_insert(listing_widget, msgbuf, strlen(msgbuf));
    if (point == 0){
	xv_set(listing_widget, TEXTSW_FIRST, point, NULL);
    }
}

//
//	Callback routine for the "Record" choice.
//	(STATIC)
//
void
MacroExec::record_callback(Panel_item, int value, Event *)
{
    // fprintf(stderr,"MacroExec::record_callback(%d)\n", value);
    if (macroexec){
	macroexec->recording = value;
    }
}

//
//	Callback routine for the "List" button.
//	(STATIC)
//
void
MacroExec::list_callback(Panel_item, int, Event *)
{
    char fname[1025];

    // fprintf(stderr,"MacroExec::list_callback()\n");
    char *name = (char *)xv_get(macroexec->name_widget, PANEL_VALUE);
    for ( ; isspace(*name); name++);
    if (*name == '/'){
	fname[0] = '\0';
    }else{
	init_get_env_name(fname);
	strcat(fname, "/macro/");
    }
    strcat(fname, name);
    xv_set(macroexec->listing_widget,
	   TEXTSW_FILE, fname,
	   NULL);
}

//
//	Callback routine for the "Exec" button.
//	(STATIC)
//
void
MacroExec::exec_callback(Panel_item, int, Event *)
{
    //fprintf(stderr,"MacroExec::exec_callback()\n");
    char *name = (char *)xv_get(macroexec->name_widget, PANEL_VALUE);
    char *cmd = new char[strlen(name)+2];
    sprintf(cmd,"%s\n", name);
    loadAndExec(cmd);
    delete [] cmd;
    //execMacro(name);
}

//
//	Callback routine for the "Name" text field
//	(STATIC)
//
Panel_setting
MacroExec::name_callback(Panel_item, Event *)
{
    // fprintf(stderr,"MacroExec::name_callback()\n");
    exec_callback(0, 0, 0);
    return PANEL_NONE;
}

void
MacroExec::execMacro(char *macname)
{
    char cmd[1024];
    FILE *fd;
    char fname[1025];
    int i;
    char *pc;

    // fprintf(stderr,"execMacro(%s)\n", macname);
    // Open macro file
    for (i=0; isspace(macname[i]); i++);
    if (macname[i] == '/'){
	fname[0] = '\0';
    }else{
	init_get_env_name(fname);
	strcat(fname, "/macro/");
    }
    strcat(fname, &macname[i]);
    fd = fopen(fname, "r");
    if (fd){
	while (fgets(cmd, sizeof(cmd), fd)){
	    if (pc=strchr(cmd, '\n')){
		*pc = '\0'; // For pretty printing
	    }
	    execProc(cmd);
	}
    }
}

void
MacroExec::execProc(char *cmd)
{
    int argc;
    char **argv;
    int (*func)(int, char **, int, char **);
    int retc = 0;
    char **retv = 0;

    if (func=getFuncAndArgs(cmd, &argv, &argc)){
	((*func)(argc, argv, retc, retv));
    }else{
	msgerr_print("execProc(): Command not found: %.100s", cmd);
    }
}

int (*MacroExec::
     getFunc(char *name))(int, char **, int, char **)
{
    int (*rtn)(int, char **, int, char **) = 0;
    int len = strlen(name);
    Cmd *p;
    for (p=cmd_table;p->name; p++){
	if (strncasecmp(name, p->name, len) == 0){
	    rtn = p->func;
	    break;
	}
    }
    return rtn;
}

int (*MacroExec::
     getFuncAndArgs(char *cmd, char ***pargv, int *pargc)
     )(int, char **, int, char **)
{
    int (*rtn)(int, char **, int, char **) = 0;
    // fprintf(stderr,"getFuncAndArgs(%s)\n", cmd);

    char name[100];
    sscanf(cmd,"%99[^ \t\n(]", name);
    int len = strlen(name);

    Cmd *p;
    for (p=cmd_table;p->name; p++){
	if (strncmp(name, p->name, len) == 0){
	    rtn = p->func;
	    break;
	}
    }

    if (rtn){
	char **argv;
	int i;
	*pargc = 0;
	*pargv = 0;
	char *term;
	char arg[100];
	char *pc = cmd + len;
	while(*pc){
	    // Skip to beginning of next arg
	    while (isspace(*pc) || *pc == '(' || *pc == ','){
		pc++;
	    }
	    // Check for end of arg list
	    if (*pc == ')'){
		break;
	    }
	    // Read in next arg
	    term = ", \t\n)";
	    if (*pc == '\''){
		pc++;
		term = "'";
	    }
	    for (i=0; i<99; i++, pc++){
		if (!*pc || strchr(term, *pc)){
		    break;
		}
		arg[i] = *pc;
	    }
	    arg[i] = '\0';
	    if (*pc){
		pc++;		// Skip the terminator
	    }

	    // Put arg into callers list
	    // Get space for argv array so far
	    argv = (char **)malloc((*pargc + 1) * sizeof(char *));
	    // Copy previous pointers into it
	    for (i=0; i<*pargc; i++){
		argv[i] = (*pargv)[i];
	    }
	    // Release previous array
	    if (*pargv){
		free(*pargv);
	    }
	    // Point callers argv to new array
	    *pargv = argv;
	    // Put in latest argument string
	    (*pargv)[*pargc] = (char *)malloc(strlen(arg) + 1);
	    strcpy((*pargv)[*pargc], arg);
	    (*pargc)++;
	}
    }

    return rtn;
}

/************************************************************************
*									*
*  Get a bunch of integer values from an argv list
*  Returns number of values decoded.
*									*/
int
MacroExec::getIntArgs(int argc,	// Argument count
		      char **argv, // Argument array
		      int *values, // Decoded values
		      int nvals	   // Number of values to get
		      )
{
    int err;
    int i;
    int rtn = 0;
    for (i=0; i<nvals && i<argc; i++){
	err = sscanf(*argv, "%i", &values[i]);
	if (err != 1){
	    break;
	}
	rtn++;
	argv++;
    }
    return rtn;
}

/************************************************************************
*									*
*  Get a bunch of "double" values from an argv list
*  Returns number of values decoded.
*									*/
int
MacroExec::getDoubleArgs(int argc,	 // Argument count
			 char **argv,	 // Argument array
			 double *values, // Decoded values
			 int nvals	 // Number of values to get
			 )
{
    int err;
    int i;
    int rtn = 0;
    for (i=0; i<nvals && i<argc; i++){
	err = sscanf(*argv, "%lf", &values[i]);
	if (err != 1){
	    break;
	}
	rtn++;
	argv++;
    }
    return rtn;
}

/************************************************************************
*									*
*  Suspend execution of macros (or anything else) to do some
*  interactive stuff for a while.
*									*/
int
MacroExec::Suspend(int argc, char **, int, char **)
{
#ifdef NOTDEFINED
    argc--;

    if (argc != 0){
	ABORT;
    }

#ifdef FORK
    int pid;
    int stat;
    if (pid=fork()){
	fprintf(stderr,"Parent waiting for childs death\n");/*CMP*/
	waitpid(pid, &stat, 0);
	fprintf(stderr,"Parent resuming\n");/*CMP*/
    }else{
	fprintf(stderr,"Child is in control\n");/*CMP*/
	ABORT;
    }
#else
    if (setjmp(setjmp_env) == 1){
	setjmp_set = FALSE;
	fprintf(stderr,"Back again!\n");/*CMP*/
    }else{
	setjmp_set = TRUE;
	fprintf(stderr,"Ready for jump\n");/*CMP*/
	int i;
	//sleep(10);
	//browser_loop();
	for (i=0; i<200; i++){
	    usleep(100000);
	    if (i%20 == 0){
		fprintf(stderr,"%d ", i/10);
	    }
	    //notify_dispatch();
	    //FlushGraphics();
	}
	/*NOTREACHED*/
    }
#endif /*FORK*/
#endif /*NOTDEFINED*/
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Resume execution of macros where we left off with a call to Suspend()
*									*/
void
MacroExec::resume_callback(Panel_item, int, Event *)
{
    //exit(0);
    fprintf(stderr,"resume_callback(), setjmp_set=%d\n", setjmp_set);/*CMP*/
    if (setjmp_set){
	longjmp(setjmp_env, 1);
	/*NOTREACHED*/
    }
    // If we return, it is because setjmp() has not been called.
}
