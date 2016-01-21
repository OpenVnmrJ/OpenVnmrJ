/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>

#include "group.h"
#include "vnmrsys.h"

#ifdef SUN
#ifndef  X11
#include <suntool/sunview.h>
#include <sunwindow/notify.h>
#include <sys/resource.h>
#endif   X11
#include <sys/signal.h>
#include <sys/wait.h>
#endif SUN
static char fdstr[10], str[MAXPATHL];
static char path[MAXPATHL];
static int  pfd[2],i;
static int  status_pid;
extern char Xserver[];
  
status(argc, argv, retc, retv)
int  argc, retc;
char **argv, **retv;

{
#ifdef SUN
#ifdef  X11
	void wait_handler();
#else
        static  int my_client;
	static  int *me = &my_client;
        static Notify_value  wait_handler();
#endif X11
        int   pid, t;
        char  file1[MAXPATHL], file2[MAXPATHL];;

        if (argc > 1)
		strcpy(path, argv[1]);
	else
	{
            /*    if(in_auto_mode() == 0)
                {
                       Werrprintf("not in auto mode, needs argument");
                       return; 
                }         */ 
                t = P_getstring(GLOBAL, "autodir", path, 1, MAXPATHL);
                if(t != 0)
                {
		      Werrprintf("can not find auto directory, needs pathname");
		      return;
		}
	}
        sprintf(file1, "%s/enterQ", path);
        sprintf(file2, "%s/doneQ", path);
        if ((access(file1, 4) != 0) && (access(file2, 4) != 0))   
        {
               Werrprintf("Neither enterQ nor doneQ exists");
               return;
	}
        if (pipe(pfd) == -1)
        { 
                Werrprintf("pipe error");
                return;
        } 
        pid = fork();
        if (pid == -1)
        {
                Werrprintf("fork error");
                return; 
        }
        if (pid == 0)
        {
                if (close(pfd[0]) == -1)
                {
                      Werrprintf("close read file error");
                      return;  
                } 
                sprintf(fdstr, "%d", pfd[1]);   
                for(i = 3; i < 30; i++)
                {
                        if (pfd[1] != i)
                            close(i);     
                }
                if (Xserver[0] != '\0')
                    execlp("status", "status","vnmr", path, fdstr, "-display", Xserver,0);
                else
                    execlp("status", "status","vnmr", path, fdstr, 0);
                exit(1);
        }
	else
	{         
                close (pfd[1]);
#ifdef  X11
		status_pid = pid;
		signal(SIGCHLD, wait_handler);
		/*******
                wait(0);
                wait_handler();
		******/
#else
                (void)notify_set_wait3_func(me, wait_handler, pid);
#endif  X11
                return;
        }
#endif SUN
}


#ifdef SUN
#ifdef X11
void
wait_handler()
{
	int  child_pid, status;
        int  len;
#ifdef AIX
        child_pid = wait3(&status, WNOHANG, NULL);
#else
        child_pid = waitpid(-1, &status, WNOHANG);
#endif
        if (WIFEXITED(status) )
        {
           if (child_pid == status_pid)
           {
                if ((len = read(pfd[0], str, sizeof(str))) > 1)
                {
                     /* read does not null terminate the string */
                     str[len] = '\0';
                     strcat(str, "\n");
		     close(pfd[0]);
                     execString(str);
		     return;
                }
		close(pfd[0]);
		return;
           }
        }
}
#else

static Notify_value
wait_handler(me, pid, status, rusage)
  int  *me, pid;
  union wait *status;
  struct rusage *rusage;

{
                if (read(pfd[0], str, sizeof(str)) > 1)
                {
                     strcat(str, "\n");
		     execString(str);
                     close(pfd[0]);
                     return(NOTIFY_DONE);
                }
                close(pfd[0]);
                return(NOTIFY_IGNORED);

}
#endif X11
#endif SUN
