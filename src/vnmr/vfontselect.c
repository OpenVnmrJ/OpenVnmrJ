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
#include <unistd.h>
#include <sys/signal.h>
#include "vnmrsys.h"
#include "group.h"

extern int  graf_width, graf_height;
static int  gpipe[2];
static int  pfd = -1;
static int  prog_pid = 0;
static char data[256];
static char **argvs = NULL;
static char argvs_num = 0;
static int  debug = 0;


static
fontselect_sync_func(argc, argv)
int	argc;
char    **argv;
{
	int	r, this_expnum, cpid;
	int	gfd;
	double  dval;
	char	*retStr;
	int	ret, tree;
	int	item;


	item = atoi(argv[2]);
	if (item < 99)
	    add_graphics_font(argv[3]);
	else if (item  == 199)
	{
	     show_graph_font();
	}
	else
	{
	    auto_switch_graphics_font();
	    appendvarlist("cr");
	    autoRedisplay();
	}
}


static void
kill_font_prog(pid)
int	pid;
{
	if (prog_pid > 0)
	{
 	    sprintf(data, "exit");
             write(pfd, data, strlen(data));
	    prog_pid = 0;
	    pfd = -1;
        }
}

send2fontselect(s, r, c, name)
int	s, r, c;
char	*name;
{
	if (prog_pid <= 0 || pfd <= 0)
	    return;
	sprintf(data, "font %d %d %d %s ", s, r, c, name);
        write(pfd, data, strlen(data));
}

int
is_fontselect_active()
{
	if (prog_pid <= 0 || pfd <= 0)
	    return(0);
	return(1);
}

static void
clear_font_prog(pid)
 int  pid;
{
        prog_pid = 0;
        close (pfd);
        pfd = -1;
}

fontselect(argc, argv, retc, retv)
int  argc, retc;
char **argv, **retv;
{
        int     i, k, exitFlag;
        char    vaddr[256];
        char    v1str[10];
        char    v2str[10];
        char    v3str[10];
        char    v4str[10];
        char    v5str[10];
	int	defFont, scrWidth;
	int	pid, argptr;

#ifndef  X11
        Werrprintf("fontselect is not available from SunView");
	     RETURN;
#else 
	if (!Wissun())
	     RETURN;
	
	if (argc > 1 && strcmp(argv[1],"SYNCFUNC") == 0)
	{
	    if (argc > 3)
		fontselect_sync_func(argc, argv);
	    RETURN;
	}
	exitFlag = 0;
	debug = 0;
	defFont = 0;
	argptr = 0;
	if (argvs_num < argc + 6)
	{
	    if (argvs != NULL)
		free (argvs);
	    argvs_num = argc + 6;
	    argvs = (char **)calloc(1, sizeof(char *) * argvs_num);
	}
	    
	for(i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-debug") == 0)
	            debug = 0;
		else if (strcmp(argv[i], "debug") == 0)
		    debug = 1;
		else if (strcmp(argv[i], "exit") == 0)
	            exitFlag = 1;
		else if (strcmp(argv[i], "-fn") == 0)
		{
		    if (argc > i+1)
		    {
	            	defFont = i;
		    	argvs[argptr++] = argv[i];
		    	argvs[argptr++] = argv[i+1];
			i++;
		    }
		}
		else
		   argvs[argptr++] = argv[i];
	}
	if (exitFlag)
	{
	     if (prog_pid > 0 && pfd > 0)
	     {
 	     	strcpy(data, "exit");
             	write(pfd, data, strlen(data));
	     }
             RETURN;
	}
	if (prog_pid > 0 && pfd > 0)
	{
 	     sprintf(data, "up  ");
             write(pfd, data, strlen(data));
             RETURN;
	}

	if (getparm("vnmraddr", "STRING", GLOBAL, data, MAXPATHL-1))
        {
                Werrprintf("fontseleect: cannot get vnmraddr");
                RETURN;
        }
        sprintf(vaddr, "-h%s", data);
        if (pipe(gpipe) < 0)
        {
             Werrprintf("fontselect: pipe error");
             RETURN;
        }
	pid = fork();
        if (pid < 0)
        {
	    close(gpipe[0]);
	    close(gpipe[1]);
            Werrprintf("fontselect: fork error");
            RETURN;
        }
        if (pid == 0)
        {
	    if (close(gpipe[1]) == -1)
            {
                   Werrprintf("close read file error");
                   RETURN;
            }
            sprintf(v1str, "-I%d", gpipe[0]);
            sprintf(v5str, "-G%d", graf_width);
	    argvs[argptr++] = v1str;
	    argvs[argptr++] = v5str;
	    argvs[argptr++] = vaddr;
	    if (defFont == 0)
	    {
                sprintf(v2str, "-fn");
	        scrWidth = widthOfScreen();
		if (scrWidth <= 600)
		    strcpy(v3str, "6x13");
		else if (scrWidth <= 800)
		    strcpy(v3str, "7x13");
		else
		    strcpy(v3str, "8x13");
	        argvs[argptr++] = v2str;
		argvs[argptr++] = v3str;
	    }
	    argvs[argptr] = (char *) NULL;
            execvp("fontselect", argvs);
            sleep(1);
            exit(0);
        }
        else
        {
	     close (gpipe[0]);
	     prog_pid = pid;
	     pfd = gpipe[1];
             register_child_exit_func(SIGCHLD, pid, clear_font_prog, kill_font_prog);
             RETURN;
        }
#endif 
}

