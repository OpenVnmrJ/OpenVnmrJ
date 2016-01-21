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
#include <stdarg.h>
#include <string.h>

#include "node.h"
#include "stack.h"
#include "magical_io.h"
#include "magical_cmds.h"

#ifdef  DEBUG
extern int      Tflag;
#define TPRINT0(str) 		if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) 	if (Tflag) fprintf(stderr,str,arg1)
#else   DEBUG
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#endif  DEBUG

node   *doingNode = NULL;
static void (*magicErrorPrinter)() = NULL;
static void (*magicInfoPrinter)() = NULL;

/*----------------------------------------------------------------------
|
|	This file contains the interfaces to user supplied functions.
|	In most cases, a default function action is supplied, and the
|	library user may register their own function to be used in
|	in its place.  In some cases, there is no reasonable default
|	action, and a user function is required.
|
+----------------------------------------------------------------------*/

static int (*(*func_finder)())() = NULL;

/*----------------------------------------------------------------------
|
|	Looks for a builtin function called "name".
|	If found, returns a pointer to the function;
|	otherwise, returns NULL.
|
+----------------------------------------------------------------------*/
int
(*builtin(name))()
char *name;			/* Name of (possible) function. */
{
    int (*rtn)() = NULL;

    if (func_finder){
	/* Call user supplied function */
	rtn = (*func_finder)(name);
    }
    if (!rtn){
	/* Call built-in function finder */
	rtn = getBuiltinFunc(name);
    }
    return rtn;
}

void
magical_register_user_func_finder(pfunc)
  int (*(*pfunc)())();
{
    func_finder = pfunc;
}

void
magical_set_macro_search_path(dirs)
  char **dirs;
{
    magicMacroDirList = dirs;
}

void
magical_set_error_print(pfunc)
  void (*pfunc)();
{
    magicErrorPrinter = pfunc;
}

void
magical_set_info_print(pfunc)
  void (*pfunc)();
{
    magicInfoPrinter = pfunc;
}

/*----------------------------------------------------------------------
|
|	This procedure creates a string containing the error position
|	and filename. If there is no file name, this routine returns
|	a null in the buf. 
|
+----------------------------------------------------------------------*/
static char *
getErrorPosition(buf,size)
  char *buf;
  int size;
{
    char tmp[128];

    if (doingNode && 1 <= doingNode->location.line && doingNode->location.file){
	char  tempFilename[64];
        char *ptr;
        /*  remove path prefix and .suffix from filename */
        if (ptr = strrchr(doingNode->location.file,'/')){
	    strncpy(tempFilename, ptr+1, 64);
	    tempFilename[63] = '\0';
	    if (ptr = (char *)strrchr(tempFilename,'.')){
		*ptr = '\0';			 /* get rid of . part */
	    }
	}else{
	    strncpy(tempFilename, doingNode->location.file, 64);
	    tempFilename[63] = '\0';
	}
        sprintf(tmp," at line %d (col %d) in %s"
		,doingNode->location.line
		,doingNode->location.column
		,tempFilename
		);
    }else{
	tmp[0] = '\0';
    }
    strncpy(buf, tmp, size);
    buf[size-1] = '\0';
    return(buf);
}

/*----------------------------------------------------------------------
|
|	This procedure  prints a message in the error line.
|	and beeps the bell and/or panel. It includes the position in
|       the macro file where error has occured.
|
+----------------------------------------------------------------------*/
void WerrprintfWithPos(const char *format, ...)
{
    va_list      vargs;
    char         str[1024];
    char         tmp[128];

    va_start(vargs,format);
    TPRINT1("WerrprintfWithPos: format ='%s'\n", format);
    getErrorPosition(tmp, 128);
    {
        vsprintf(str, format, vargs);
        strcat(str,tmp);
	if (magicErrorPrinter){
	    (*magicErrorPrinter)(str);
	}else{
	    fprintf(stderr, "%s\007\n", str);
	}
    }

    va_end(vargs);
    TPRINT0("WerrprintfWithPos: returning\n");
}

/*----------------------------------------------------------------------
|
|	This procedure  prints a message in the error line.
|	and beeps the bell and/or panel. It does not give position
|       information.
|
+----------------------------------------------------------------------*/
void Werrprintf(char *format, ...)
{   char         str[1024];
    va_list      vargs;

    va_start(vargs,format);
    TPRINT1("Werrprintf: format ='%s'\n",format);
    vsprintf(str,format,vargs);
    if (magicErrorPrinter){
	(*magicErrorPrinter)(str);
    }else{
        fprintf(stderr, "%s\007\n", str);
    }

    va_end(vargs);
    TPRINT0("Werrprintf: returning\n");
}

/*----------------------------------------------------------------------
|
|	This procedure prints an informative message.
|       It is based on Werrprintf. It does not make a bell or noise.
|
+----------------------------------------------------------------------*/
void Winfoprintf(char *format, ...)
{
    char    str[1024];
    va_list vargs;

    va_start(vargs,format);
    TPRINT1("Winfoprintf: format ='%s'\n",format);
    vsprintf(str,format,vargs);
    strcat(str, "\n");
    if (magicInfoPrinter){
	(*magicInfoPrinter)(str);
    }else{
        fprintf(stderr,"%s", str);
    }

    va_end(vargs);
    TPRINT0("Werrprintf: returning\n");
}

/*----------------------------------------------------------------------
|
|	This procedure is like Winfoprintf except it doesnt add a newline
|
+----------------------------------------------------------------------*/
void Wscrprintf(char *format, ...)
{
    char    str[1024];
    va_list vargs;

    va_start(vargs,format);
    TPRINT1("Wscrprintf: format ='%s'\n",format);
    vsprintf(str,format,vargs);
    if (magicInfoPrinter){
	(*magicInfoPrinter)(str);
    }else{
        fprintf(stderr,"%s", str);
    }

    va_end(vargs);
}

/*----------------------------------------------------------------------
|
|	This procedure positions a cursor to a position and then printfs.
|	The  user is responsible for any carriage returns.	
|	The cursor is positioned to a line relative to the window screen
|	chosen.  Lines start from line 1. Columns also start at 1.
|	If a negative column is passed, Wprintfpos will print from the
|	cursor position onwards. 
|	If the SUN is being used, Wprintfpos will attempt to create an
|	panel_item or canvas_item and then display the item in the
|	correct window.
|
+----------------------------------------------------------------------*/
void Wprintfpos(int window, int line, int col, char *format, ...)
{
    va_list vargs;

    va_start(vargs,format);
    vfprintf(stdout, format, vargs); /* print out the number */
    va_end(vargs);
}
