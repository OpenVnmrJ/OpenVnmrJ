/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* taskdelhook.c 11.1 07/09/07 - Task Delete Hook */
/* 
 */

/*
modification history
--------------------
11-2-94,gmb  created 
*/

/*
DESCRIPTION

  This Task  handles any of the clean up required if one of the
  system tasks is deleted.
  E.G. Closing open channels, etc..

*/


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdioLib.h>
#include <taskLib.h>
#include "logMsgLib.h"
#include "hostMsgChannels.h"

typedef struct _taskwchan_ {
			char *tname;
			int  channum;
		   } TASKWCHAN;

TASKWCHAN TaskWChans[] = { { "tUpLink", RECVPROC_CHANNEL }, 
		       { "tMonitor", EXPPROC_CHANNEL },
		       { "tHostAgent", EXPPROC_CHANNEL },
		       { "tDownLink", SENDPROC_CHANNEL },
		       { NULL, 0 }
			};
			
/********************************************************
* systemTaskDel - task deletion cleanup
*
*  Close channels for task that are deleted
*/
void systemTaskDel(WIND_TCB *pTcb)
{
   int stat;
   int indx = 0;

   /* first check names against those that have channels open
      and close them.
   */
   while (TaskWChans[indx].tname != NULL)
   {
      DPRINT2(3,"delTask: '%s', cmp: '%s'\n",pTcb->name,TaskWChans[indx].tname);
      if (strcmp(pTcb->name,TaskWChans[indx].tname) == 0)
      {
	 DPRINT2(2,"systemTaskDel: closeChannel: %d, for deleted task: '%s'\n",
		TaskWChans[indx].channum,TaskWChans[indx].tname);
	 stat = closeChannel(TaskWChans[indx].channum);
	 DPRINT1(2,"systemTaskDel: closeChannel: status %d\n",stat);
	 break;
      }
      indx++;
   }
}

addSystemDelHook()
{
   taskDeleteHookAdd(systemTaskDel);
}

