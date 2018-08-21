/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "wjunk.h"
#ifdef X11
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include "group.h"
#include "pvars.h"
#include "vnmrsys.h"
#include "variables.h"
#include "vglide.h"

extern char     mainFrameFds[];
extern char     buttonWinFds[];
extern char     *get_cwd();
extern char     curexpdir[];

static int  gpipe2[2];
static int  gfd;
static char data[1024];

static int  debug;
static int  qFlag;
#endif
static int  glide_pid = -1;


#ifdef X11

void send_to_glide(char *mess)
{
	if ((int) strlen(mess) <= 1)
	{
	    strcat(mess, "    ");
	    mess[4] = '\0';
	}
	if (debug)
	    fprintf(stderr, " send to glide: %s\n", mess);
        write(gfd, mess, strlen(mess));
	flush(gfd);
}

static char *exec_macro(data, num)
char	*data;
int	num;
{
	int	p, k;
	char	*d;
	char	cdata[120];
	char	mdata[16];

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


static char *modify_show_str(showStr)
char	*showStr;
{
	int	k, k2, m;
	char    *d1, *d2, *d3, *dest;
	char	macro[16];
	static char dstr[256];

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

static void glide_sync_func(cmd, argc, argv)
int	cmd, argc;
char    **argv;
{
	int	r, this_expnum;
	double  dval;
	char	*retStr;
        vInfo   info;
        int     ret, tree;

	if (debug)
	  fprintf(stderr, " glide command: %d, %s\n", cmd, argv[2]);
	if (gfd < 0)
	    return;
	switch (cmd) {
	  case  PWD:
		retStr = get_cwd();
             	write(gfd, retStr, strlen(retStr));
		flush(gfd);
		break;
	  case  EXPDIR:
             	write(gfd, curexpdir, strlen(curexpdir));
		flush(gfd);
		break;
	  case  GEXIT:
		glide_pid = -1;
		break;
	  case  GOK: /* verify console is available */
		this_expnum = expdir_to_expnum( curexpdir );
  		r = is_exp_active( this_expnum );
		sprintf(data, "%d  \n", r);
             	write(gfd, data, strlen(data));
		flush(gfd);
		break;
	  case  GQUERY:
		if (argc < 5)
		{
		    strcpy(data, "0  \n");
                    write(gfd, data, strlen(data));
		    flush(gfd);
		    return;
		}
		retStr = NULL;
		if (strstr(argv[4], "macro_value") != NULL)
		{
		    retStr = modify_show_str(argv[4]);
		}
		if (retStr == NULL)
		    retStr = argv[4];
		qFlag = 0;
		sprintf(data, "if (%s) then glide('SYNCFUNC', %d, '1') else glide('SYNCFUNC', %d, '0') endif\n", retStr, GTEST, GTEST);
		execString(data);
		if (qFlag == 0)
                {
                    strcpy(data, "0  \n");
                    write(gfd, data, strlen(data));
		    flush(gfd);
                }
		break;
	  case  GTEST:
		if (argc < 4)
		    return;
		qFlag = 1;
		if (debug)
	  	    fprintf(stderr, " write to glide: '%s'\n", argv[3]);
		sprintf(data, "%s   \n", argv[3]);
             	write(gfd, data, strlen(data));
		flush(gfd);
		break;
	  case  VGET:
		sprintf(data, "    \n");
                if (argc < 5)
                {
                    write(gfd, data, strlen(data));
		    flush(gfd);
                    return;
                }
                tree = CURRENT;
                if (ret = P_getVarInfo(tree, argv[4], &info))
                {
                    tree = GLOBAL;
                    ret = P_getVarInfo(tree, argv[4], &info);
                }
                if (ret != 0)
                {
                    write(gfd, data, strlen(data));
		    flush(gfd);
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
		flush(gfd);
		break;
	}
}
		

static void
kill_glide()
{
	if (glide_pid > 0)
	{
	   sprintf(data, "%c  1  ,\"exit\" ", ESC);
           write(gfd, data, strlen(data));
	   flush(gfd);
	}
}


static void
clear_glide(pid)
 int  pid;
{
        glide_pid = -1;
        close (gfd);
	gfd = -1;
        return;
}

#endif 
			   
int glide(argc, argv, retc, retv)
int  argc, retc;
char **argv, **retv;
{
#ifndef  X11
        Werrprintf("GLIDE is not available");
        return(0);
#else 
        int     i;
        char    addr[MAXPATHL];
        char    fd2str[10];
        char    dstr[6];
        int   pid;

        if (glide_pid > 0)
        {
	     if (argc > 2 && strcmp(argv[1],"SYNCFUNC") == 0)
	     {
                sync();
		glide_sync_func(atoi(argv[2]), argc, argv);
		RETURN;
	     }
	     if (argc == 1)
	     	  sprintf(data, "%c  1  ,\"updown\" ", ESC);
             else
             {
	     	  sprintf(data, "%c  %d  ", ESC, argc);
		  for (i = 1; i < argc; i++)
		  {
		      strcat(data, ",\"");
		      strcat(data, argv[i]);
		      strcat(data, "\" ");
		  }
             }
	     if (gfd >= 0)
	     {
                  write(gfd, data, strlen(data));
		  flush(gfd);
	     }
             RETURN;
        }
	debug = 0;
	if (argc > 1)
	{
	      if (strcmp(argv[1], "-debug") == 0 || strcmp(argv[1], "debug") == 0)
	      {
		   debug = 1;
		   if (argc > 2)
			debug = atoi(argv[2]);
		   sprintf(dstr, "-d%d", debug);
	      }
	      else
		   RETURN;
	}
	if (getparm("vnmraddr", "STRING", GLOBAL, data, MAXPATHL-1))
        {
                Werrprintf("glide: cannot get vnmraddr");
                RETURN;
        }
        if (pipe(gpipe2) < 0)
        {
                Werrprintf("glide pipe error");
                RETURN;
        }
        pid = fork();
        if (pid < 0)
        {
                Werrprintf("glide fork error");
                RETURN;
        }
        if (pid == 0)
        {
                if (close(gpipe2[1]) == -1)
                {
                      Werrprintf("close read file error");
                      RETURN;
                }
                sprintf(fd2str, "-I%d", gpipe2[0]);
                sprintf(addr, "-h%s", data);
                for(i = 3; i < 30; i++)
                {
                        if (i != gpipe2[0])
                            close(i);
                }
		if (debug)
                    execlp("glide", "glide", addr, dstr, fd2str, mainFrameFds, buttonWinFds, NULL);
		else
                    execlp("glide", "glide", addr, fd2str, mainFrameFds, buttonWinFds, NULL);
                sleep(1);
                exit(0);

        }
        else
        {
                close (gpipe2[0]);
                glide_pid = pid;
		gfd = gpipe2[1];
                register_child_exit_func(SIGCHLD, pid, clear_glide, kill_glide);
                RETURN;
        }
#endif 
}

int
glide_active()
{
	if (glide_pid < 0)
	    return(0);
	else
	    return(1);
}
