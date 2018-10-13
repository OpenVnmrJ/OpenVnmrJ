/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	history.c
|
|	This file contains support code for a basic command history.  The
|	following functions are provided.
|
|		stuffCommand/1          Add a command to head of history list.
|		previousCommand/0       Return previous command.
|		nextCommand/0           Return next command.
|		purgeHistory/0          Purge the history list.
|		showHistory/0           Output the history list.
|
+-----------------------------------------------------------------------------*/

#include <stdio.h>
#include "allocate.h"
#include "tools.h"

#define maxHistory  32
#define succ(A)     (((A)+1)%maxHistory)
#define pred(A)     ((((A)-1) < 0) ? maxHistory-1 : ((A)-1) % maxHistory)

static char *historyList[maxHistory];
static int   historyHead = 0;
static int   historyTail = 0;
static int   historyLast = 0;

/*------------------------------------------------------------------------------
|
|	stuffCommand/1
|
|	Stuff a command into the history list (overflows off the back are
|	released, thus lost).  Notice that calling this procedure resets
|	the selector to the head of the list (the next call to the function
|	"previousCommand" will return the command just stuffed).
|
+-----------------------------------------------------------------------------*/

void stuffCommand(char *s)
{  if (succ(historyHead) == historyTail)
   {  if (historyList[historyTail])
	 release(historyList[historyTail]);
      historyTail = succ(historyTail);
   }
   historyList[historyHead] = newStringId(s,"history");
   historyHead = succ(historyHead);
   historyLast = historyHead;
}

/*------------------------------------------------------------------------------
|
|	previousCommand/0
|
|	Select a previous command.  Repeated calls move back through the
|	history list.  The stuffCommand procedure resets the selection so
|	that the next call selects the command just stuffed (the selector
|	moves back to the new head of the history list).  When the end of
|	the list is eventually reached (through repeated calls) a NULL is
|	returned.
|
+-----------------------------------------------------------------------------*/

char *previousCommand()
{  if (historyLast == historyTail)
      return(NULL);
   else
   {  historyLast = pred(historyLast);
      return(historyList[historyLast]);
   }
}

/*------------------------------------------------------------------------------
|
|	nextCommand/0
|
|	Select the previous command.  Repeated calls move forward through
|	the history list.  When the head of the history list is reached a
|	NULL is returned.
|
+-----------------------------------------------------------------------------*/

char *nextCommand()
{  if (historyLast == historyHead)
      return(NULL);
   else
   {  historyLast = succ(historyLast);
      return(historyList[historyLast]);
   }
}

/*------------------------------------------------------------------------------
|
|	purgeHistory/0
|
|	Purge the history list (emptying the list and releasing tied up
|	memory).
|
+-----------------------------------------------------------------------------*/

int purgeHistory()
{  releaseWithId("history");
   historyHead = 0;
   historyTail = 0;
   historyLast = 0;
   return(0);
}

/*------------------------------------------------------------------------------
|
|	showHistory/0
|
|	Output the current contents of the history list (from most recent to
|	oldest).
|
+-----------------------------------------------------------------------------*/

int showHistory()
{  int h;

   if (historyHead == historyTail)
      printf("History list is empty");
   else
   {  printf("Command history (from most recent)...\n");
      h = succ(historyHead);
      while (1)
      {  if (historyList[h])
	    printf("%s",historyList[h]);
	 else
	    printf("\"problem with history list, nil pointer\"\n");
	 if (h == historyTail)
	    break;
	 else
	    h = succ(h);
      }
   }
   return(0);
}
