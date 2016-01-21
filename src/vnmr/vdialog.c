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
#include <errno.h>
#include <sys/signal.h>
#ifdef X11
#include <X11/Intrinsic.h>
#endif 
#include "group.h"
#include "vnmrsys.h"
#include "variables.h"
#include "vglide.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"

extern char     mainFrameFds[];
extern char     buttonWinFds[];
extern char     *get_cwd();
extern char     curexpdir[];

extern int  execString(const char *buffer);
extern int expdir_to_expnum(char *expdir);
extern int is_exp_active( int this_expnum );
extern void register_child_exit_func(int signum, int pid,
             void (*func_exit)(), void (*func_kill)() );

#ifdef X11
extern Widget   canvasShell;
#endif 

typedef struct _d_list {
	 int	used;
	 int	pid;
	 int	fout;
	 int	fin;
	 int	cfout;
	 int	cfin;
	 int	wait;
	 char	fname[MAXPATHL];
	 struct _d_list *next;
	} DLIST;

static DLIST *dialog_list = NULL;
static int  gpipe1[2];
static int  gpipe2[2];
static int  queryFlag;
static char data[1024];
static char **argvs = NULL;
static char *infile = "-Q";
static char *outfile = "-P";
static char *waitStr = "-W";
static char *prgName = "dialog";
static char *VArgs[9];

static int  debug = 0;
static int  waitFlag = 1;



static void
close_fd(DLIST *dlist)
{
	DLIST   *nlist;

	nlist = NULL;
	if (dlist->fout >= 0)
	    close(dlist->fout);
	if (dlist->cfin >= 0)
	    close(dlist->cfin);
	dlist->fout = -1;
	dlist->cfin = -1;
	if (dlist->wait)
	{
	    nlist = dialog_list;
	    while (nlist != NULL)
	    {
		if (nlist->used && nlist->wait)
		{
		    if (nlist->pid != dlist->pid)
		    {
			if (nlist->fin == dlist->fin && nlist->fin >= 0)
			     break;
		    }
		}
		nlist = nlist->next;
	    }
	}
	if (nlist == NULL)
	{
	    if (dlist->cfout >= 0)
               close (dlist->cfout);
	    if (dlist->fin >= 0)
               close (dlist->fin);
	    dlist->fin = -1;
	    dlist->cfout = -1;
	}
}

static char *exec_macro(char *data, int num)
{
        int     p, k;
        char    *d;
        char    cdata[120];
        char    mdata[16];

        d = data;
        while (*d == ' ' || *d == '\t')
           d++;
        if (*d != '(')
           return(NULL);
        d++;
        p = 1;
        k = 0;
        while (p != 0)
        {
           if (*d == '\0')
                return (NULL);
           if (*d == ')')
           {
                p--;
                if (p == 0)
                {
                    d++;
                    break;
                }
           }
           else if (*d == '(')
                p++;
           cdata[k] = *d;
           k++;
           d++;
        }

        if (k > 0)
        {
           cdata[k] = '\0';
	   k--;
	   while (k > 0)
	   {
		if (cdata[k] == ' ')
		{
		    cdata[k] = '\0';
		    k--;
		}
		else
		    break;
	   } 
           sprintf (mdata, ":$glidev%d\n", num);
           strcat (cdata, mdata);
           execString(cdata);
           return (d);
        }
        else
           return (NULL);
}

static char *modify_show_str(char *showStr, char *dstr)
{
        int     k, k2, m;
        char    *d1, *d2, *d3, *dest;
        char    macro[16];

        d1 = showStr;
        k = 0;
        k2 = 0;
        m = 1;
        dest = dstr;
        while (*d1 != '\0')
        {
            d2 = strstr(d1, "macro_value");
            if (d2 == NULL)
                break;
            while (d1 != d2)
            {
                *dest = *d1;
                d1++;
                dest++;
            }
            d3 = exec_macro(d2+11, m);
            if (d3 != NULL)
            {
                sprintf(macro, " $glidev%d ", m);
                m++;
                k2 = 0;
                while (macro[k2] != '\0')
                {
                    *dest = macro[k2++];
                    dest++;
                }
                d1 = d3;
            }
            else
            {
                *dest = *d1++;
                dest++;
            }
        }

        while (*d1 != '\0')
        {
            *dest = *d1++;
            dest++;
        }
        *dest = '\0';
        return(dstr);
}


static void
dialog_sync_func(int cmd, int argc, char **argv)
{
	int	r, this_expnum, cpid;
	int	gfd;
	double  dval;
	char	*retStr;
	DLIST   *dlist;
	vInfo   info;
	int	ret, tree;
        char    dstr[256];

	if (debug > 1)
	  fprintf(stderr, " dialog_sync_func: cmd= %d  argc= %d\n", cmd, argc);
	cpid = atoi(argv[3]);
	if (debug > 1)
	  fprintf(stderr, " dialog command: %d, pid %d\n", cmd, cpid);
	dlist = dialog_list;
	gfd = -1;
	while (dlist != NULL)
	{
	    if (dlist->pid == cpid)
	    {
		gfd = dlist->fout;
		break;
	    }
	    dlist = dlist->next;
	}
	if (dlist == NULL || gfd < 0)
	{
	    if (debug > 1)
	       fprintf(stderr, " dialog_sync_func: could not find process\n");
	    return;
	}
	if (debug > 1)
	  fprintf(stderr, " execute command: %d, pid %d\n", cmd, cpid);
	switch (cmd) {
	  case  PWD:
		retStr = get_cwd();
             	write(gfd, retStr, strlen(retStr));
		break;
	  case  EXPDIR:
             	write(gfd, curexpdir, strlen(curexpdir));
		break;
	  case  GEXIT:
		dlist->used = 0;
		close_fd(dlist);
		dlist->pid = -1;
		break;
	  case  GOK: /* verify console is available */
		this_expnum = expdir_to_expnum( curexpdir );
  		r = is_exp_active( this_expnum );
		sprintf(data, "%d  \n", r);
             	write(gfd, data, strlen(data));
		break;
	  case  GQUERY:
		if (argc < 5)
		{
		    strcpy(data, "0  \n");
                    write(gfd, data, strlen(data));
		    return;
		}
		retStr = NULL;
                if (strstr(argv[4], "macro_value") != NULL)
                {
                    retStr = modify_show_str(argv[4], dstr);
                }
                if (retStr == NULL)
                    retStr = argv[4];

		queryFlag = 0;
		sprintf(data, "if (%s) then dialog('SYNCFUNC', %d, %d, '1') else dialog('SYNCFUNC', %d, %d, '0') endif\n", retStr, GTEST,cpid, GTEST, cpid);
		execString(data);
		if (queryFlag == 0)
                {
                    strcpy(data, "0  \n");
                    write(gfd, data, strlen(data));
                }
		break;
	  case  GTEST:
		if (argc < 5)
		    return;
		queryFlag = 1;
		if (debug > 1)
	  	    fprintf(stderr, " write to dialog: '%s'\n", argv[4]);
		sprintf(data, "%s   \n", argv[4]);
             	write(gfd, data, strlen(data));
		break;
	  case  VEXEC:
		if (argc < 5)
		    return;
		execString(argv[4]);
		break;
	  case  VGET:
		sprintf(data, "    \n");
		if (argc < 5)
		{
             	    write(gfd, data, strlen(data));
		    return;
		}
		tree = CURRENT;
		if ( (ret = P_getVarInfo(tree, argv[4], &info)) )
		{
		    tree = GLOBAL;
		    ret = P_getVarInfo(tree, argv[4], &info);
		}
		if (ret != 0)
		{
             	    write(gfd, data, strlen(data));
		    return;
		}
		if (info.basicType == T_STRING)
		{
		    P_getstring(tree,argv[4],data,1,1023);
		    if (strlen(data) < 4)
			strcat (data, "    \n");
		}
		else
		{
		    dval = 0.0;
		    P_getreal(tree,argv[4],&dval,1);
		    sprintf(data, "%g\n", dval);
		}
             	write(gfd, data, strlen(data));
		break;
	}
}
		

static void
kill_dialog(int pid)
{
	DLIST   *dlist;

	dlist = dialog_list;
	while (dlist != NULL)
	{
	    if (dlist->pid == pid && dlist->fout > 0)
	    {
		sprintf(data, "%c  1  ,\"exit\" ", ESC);
           	write(dlist->fout, data, strlen(data));
		dlist->used = 0;
		close_fd(dlist);
		dlist->pid = -1;
		break;
	    }
	    dlist = dlist->next;
	}
}


static void
clear_dialog(int pid)
{
	DLIST   *dlist;

	dlist = dialog_list;
	while (dlist != NULL)
	{
	    if (dlist->pid == pid)
	    {
		if (debug > 1)
	  	   fprintf(stderr, "*** clean dialog: pid (%d)\n", pid);
		dlist->used = 0;
		close_fd(dlist);
		dlist->pid = -1;
		break;
	    }
	    dlist = dlist->next;
	}
}


static int
check_input_file(char *fname)
{
	if (*fname == '/')
	{
	    if (access (fname, R_OK) == 0)
		return(1);
	    return(0);
	}
	sprintf(data, "%s/templates/%s", userdir, fname);
	if (access (data, R_OK) == 0)
	    return(1);
	sprintf(data, "%s/user_templates/%s", systemdir, fname);
	if (access (data, R_OK) == 0)
	    return(1);
	return(0);
}

static int
is_new_dialog(char *fname, int exitFlag)
{
	DLIST   *dlist;

	dlist = dialog_list;
	while (dlist != NULL)
	{
	    if (dlist->used && (strcmp(dlist->fname, fname) == 0))
	    {
		if (exitFlag)
		    sprintf(data, "%c  1  ,\"exit\" ", ESC);
		else
		    sprintf(data, "%c  1  ,\"up\" ", ESC);
           	write(dlist->fout, data, strlen(data));
		return(0);
	    }
	    dlist = dlist->next;
	}
	return(1);
}


static void
exec_dialog_input(char *cmdStr)
{
        int    vargc;

        if (sscanf(cmdStr, "%d", &vargc) < 1)
           return;
	if (vargc > 8) /* too many, its wrong */
           return;
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
	if (vargc > 3 && strcmp(VArgs[1],"SYNCFUNC") == 0)
	{
                sync();
		dialog_sync_func(atoi(VArgs[2]), vargc, VArgs);
	}
        return;
}


int
dialog(int argc, char **argv, int retc, char **retv)
{
        int     i, k, exitFlag;
        char    addr[MAXPATHL];
        char    fd1str[10];
        char    fd2str[10];
        int     defIn, defOut;
	int	fin, fout, cfin, cfout;
        char    winShell[12];
        char    inputs[256];
        char    cmdstr[256];
        char    dstr[6];
	char    *din, *din2;
	DLIST   *dlist, *nlist, *plist;
	static int  dfin, pin;

#ifndef  X11
        Werrprintf("Dialog not available from SunView");
	     RETURN;
#else 
        int   pid;

	if (argc < 2)
	{
             Werrprintf("Dialog: missing arguments");
	     RETURN;
	}
	if (!Wissun())
	     RETURN;

	if (argc > 3 && strcmp(argv[1],"SYNCFUNC") == 0)
	{
                sync();
		dialog_sync_func(atoi(argv[2]), argc, argv);
		RETURN;
	}
	exitFlag = 0;
	waitFlag = 1;
	debug = 0;
	defIn = -1;
	defOut = -1;
	for(i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-debug") == 0)
	            debug = 0;
		else if (strcmp(argv[i], "debug") == 0)
		    debug = 1;
		else if (strcmp(argv[i], "-nowait") == 0)
	            waitFlag = 1;
		else if (strcmp(argv[i], "nowait") == 0)
	            waitFlag = 0;
		else if (strcmp(argv[i], "wait") == 0)
	            waitFlag = 1;
		else if (strcmp(argv[i], "-wait") == 0)
	            waitFlag = 0;
		else if (strcmp(argv[i], "exit") == 0)
	            exitFlag = 1;
		else
		{
		    if (defIn < 0)
			defIn = i;	
		    else if (defOut < 0)
			defOut = i;
		}
	}
	if (defIn < 0)
	{
             Werrprintf("Dialog: missing argument");
	     RETURN;
	}
        if (P_getstring(GLOBAL,"vnmraddr", data, 1, sizeof(data)-1))
        {
                Werrprintf("Dialog: cannot get vnmraddr");
                RETURN;
        }
        sprintf(addr, "-h%s", data);
	if (!exitFlag && !check_input_file(argv[defIn]))
	{
                Werrprintf("Dialog: cannot locate file %s", argv[defIn]);
                RETURN;
	}
	if ( !is_new_dialog(argv[1], exitFlag))
                RETURN;
	if (exitFlag)
                RETURN;
	if (argvs == NULL)
	    argvs = (char **)calloc(1, sizeof(char *) * 22);
	plist = NULL;
	if (waitFlag)
	{
	     plist = dialog_list;
	     while (plist != NULL)
	     {
		 if (plist->used && plist->wait)
		     break;
		 plist = plist->next;
	     }
	}
        if (pipe(gpipe1) < 0)
        {
                Werrprintf("dialog: pipe error");
                RETURN;
        }
	fout = gpipe1[1];
	cfin = gpipe1[0];
	gpipe2[0] = 0;
	if (plist != NULL && plist->fin >= 0)
	{
	     cfout = plist->cfout;
	     fin = plist->fin;
	}
	else
	{
            if (pipe(gpipe2) < 0)
            {
                Werrprintf("dialog: pipe error");
		close(gpipe1[0]);
		close(gpipe1[1]);
                RETURN;
            }
	    cfout = gpipe2[1];
	    fin = gpipe2[0];
	}
        pid = fork();
        if (pid < 0)
        {
	    if (gpipe1[0])
	    {
		close(gpipe1[0]);
		close(gpipe1[1]);
	    }
	    if (gpipe2[0])
	    {
		close(gpipe2[0]);
		close(gpipe2[1]);
	    }
            Werrprintf("dialog: fork error");
            RETURN;
        }

        if (pid == 0)
        {
                sprintf(fd1str, "-O%d", cfout);
                sprintf(fd2str, "-I%d", cfin);
                sprintf(dstr, "-d%d", debug);
                for(i = 3; i < 30; i++)
                {
                        if (i != cfin && i != cfout)
                            close(i);
                }
		argvs[0] = prgName;
		argvs[1] = infile;
		argvs[2] = argv[defIn];
		i = 3;
		if (defOut > 0)
		{
		    argvs[3] = outfile;
		    argvs[4] = argv[defOut];
		    i = 5;
		}
		sprintf(winShell, "-S%d", (int) XtWindow(canvasShell));
		argvs[i++] = winShell;
		for (k = 1; k < argc; k++)
		{
		    if (k != defIn && k != defOut)
		        argvs[i++] = argv[k];
		    if (i > 15)
			break;
		}
		argvs[i++] = fd1str;
		argvs[i++] = fd2str;
		argvs[i++] = addr;
		if (debug)
		    argvs[i++] = dstr;
		if (waitFlag)
		    argvs[i++] = waitStr;
		argvs[i] = (char *) NULL;
                execvp("glide", argvs);
                sleep(1);
                exit(0);

        }
        else
        {
	     if (!waitFlag)
	     {
	        if (gpipe1[0])
                   close (gpipe1[0]);
	        if (gpipe2[0])
                   close (gpipe2[1]);
		cfin = -1;
		cfout = -1;
	     }
             register_child_exit_func(SIGCHLD, pid, clear_dialog, kill_dialog);
	     dlist = dialog_list;
	     while (dlist != NULL)
	     {
		 if (dlist->used == 0)
		     break;
		 dlist = dlist->next;
	     }
	     if (dlist == NULL)
	     {
		 if(debug > 1)
		     fprintf(stderr, " ## create new dialog list\n");
		 dlist = (DLIST *)allocateWithId(sizeof(DLIST), "xdialog");
	         dlist->next = NULL;
		 if (dialog_list == NULL)
                	dialog_list = dlist;
		 else
		 {
	     		nlist = dialog_list;
	     		while (nlist->next != NULL)
	          	    nlist = nlist->next;
	     		nlist->next = dlist;
		 }
	     }
	     dlist->used = 1;
	     dlist->pid = pid;
	     dlist->fout = fout;
	     dlist->fin = fin;
	     dlist->cfout = cfout;
	     dlist->cfin = cfin;
	     dlist->wait = waitFlag;
	     strcpy(dlist->fname, argv[1]);
	     if (waitFlag)
	     {
		dfin = fin;
		din = cmdstr;
		*din = '\0';
		while (1)
		{
		    if((k = read(dfin, inputs, 255)) <= 0)
          	    {
             		if ( (k == -1) && (errno == EINTR))
                	    continue;
             		else
                	    break;
          	    }
		    else
		    {
          	        if (debug > 1)
                	   fprintf(stderr, " read from dialog => %s\n",inputs);
			pin = 0;
                        din2 = inputs;
                        while (pin < k)
                        {
                            if (*din2 == ESC || *din2 == '\0')
                            {
                                *din = '\0';
                                if ((int)strlen(cmdstr) > 0)
                                    exec_dialog_input(cmdstr);
                                din = cmdstr;
                                *din = '\0';
                            }
			    else
			    {
				*din = *din2;
				din++;
			    }
                            pin++;
                            din2++;
                        }
/*
			exec_dialog_input(inputs);
*/
		    }
		}
	     }
             RETURN;
        }
#endif 
}


int
dialog_active()
{
	return(0);
}
