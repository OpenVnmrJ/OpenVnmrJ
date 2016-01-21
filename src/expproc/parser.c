/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>


#include <errno.h>

#include "errLogLib.h"
#include "hhashLib.h"
#include "parser.h"

static HASH_ID cmdtbl;

/**************************************************************
*
*  initCmdParser - Initialize the command hash table.
*
* This routine initializes the command hash table.
*  
* RETURNS:
* OK , or ERROR on error. 
*
*       Author Greg Brissey 8/4/94
*/
int initCmdParser()
{
   cmd *table;

    int i, size;

    /* size = sizeof(table); */
    size = sizeOfCmdTable();
    cmdtbl = hashCreate(size, NULL, NULL, "Cmd Hash Table");
    if (cmdtbl == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"initCmdParser: hashCreate() failed");

    i = 0;
    table = addrOfCmdTable();
    while(table[i].n)
    {
       hashPut(cmdtbl, table[i].n, (char *) table[i].f);
       i++;
    }
 
#ifdef DEBUG
    if (DebugLevel >= 3)
       hashShow(cmdtbl,2);
#endif
    return(0);
}

int parser(char* str)
{
    char *cmdstr;
    char *newcmdstr;
    PFI  cmdfunc;

    cmdstr = strtok(str," ");
    cmdfunc = (PFI) hashGet(cmdtbl,cmdstr);
    if (cmdfunc != (PFI) -1L)
    {
	/* skip over the cmdstr, and pass this along 
           as the argument to the called function, this allows strtok_r
	   to work if used  */
	newcmdstr = cmdstr + strlen(cmdstr) + 1;
       (*cmdfunc)(newcmdstr);   /* was (*cmdfunc)(cmdstr); */
    }
    else
    {
       errLogRet(LOGOPT,debugInfo,
	  "parser: Cmd Str: '%s' is an unknown command\n",cmdstr);
       return(-1);
    }
    return(0);
}

int listCmds(char *str)
{
   cmd *table;
   int i;

    diagPrint(NULL,"Known Commands:\n");
    i = 0;
    table = addrOfCmdTable();
    while(table[i].n)
    {
       diagPrint(NULL,"                 %s: %s\n",table[i].n,table[i].d);
       i++;
    }
    return(0);
}
