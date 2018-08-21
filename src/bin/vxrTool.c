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
/*---------------------------------------------------------------------------
|
|	vxrTool
|
|	This program creates a frame and ttysubwindow in the frame to
|	create a popup shell or task.  When the task dies, it should
|	kill terminate this program gracefully. 
|	This is the same as shelltool with focus set on.  It is
|	too bad Sun does not have an option to set focus on.  Maybe
|	they should be called to find out if they have an undocumented
|	option since this routine takes up 1/2 megabytes.
|
|	Author  R. Lasslo   5/31/87
+---------------------------------------------------------------------------*/
#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/tty.h>


/*static int   taskDeath;
static int  *taskDeathPtr = &taskDeath;
static int   taskPid; */


main(argc,argv)  int argc; char *argv[]; 
{  char *tty_argv[10]; 
   int   i; 
   int   taskPid;
   int   ttyFrameFD;
   Frame ttyFrame;
   Tty   ttySw;

   ttyFrame = window_create(NULL,FRAME
				,FRAME_SHOW_LABEL, TRUE
				,FRAME_LABEL, "popup tty"
				,0
				);	
   /* ttyFrameFD = (int)(window_get(ttyFrame,WIN_FD,0)); */
   tty_argv[0] = NULL;
   for (i=1; i<argc && i<9 ; i++)
   {  tty_argv[i-1] = argv[i];
      tty_argv[i] = NULL;
   }
   ttySw = window_create(ttyFrame,TTY
			,TTY_ARGV, tty_argv
			,WIN_KBD_FOCUS, TRUE
			,TTY_QUIT_ON_CHILD_DEATH,  TRUE
			,0
			);
   /*taskPid = (int) (window_get(ttySw, TTY_PID,0)); */
   /* Register  procedure to be called when ttyTask dies */
   /*notify_set_wait3_func(taskDeathPtr,taskFuneral,taskPid);
   /* window_set(ttyFrame
		,WIN_SHOW,TRUE
		,0
		);
   */
   window_main_loop(ttyFrame);
}
