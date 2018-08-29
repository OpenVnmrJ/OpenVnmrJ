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
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "msgQLib.h"

/* #include ipcMsgQLib.h ipcKeyDbm.h mfileObj.h shrMLib.h */


extern int errno;

#define MAXRETRY 8
#define SLEEP 1

#define DELAY 2		/* blocking IO flag */
#define ERROR -1
#define HOSTLEN 128
#define MAX_MASTERS 4
#define MAX_ADDRLEN 126
#define VALL 1
#define VPID 2
#define VFILE 3

static char    tmpstr[256];
static int     solaris;
static int    debug = 0;

/* keep this option from prying eyes (e.g. strings) */
static char    ChkStr[] = { 'G','o','d','K','i','n','g','\0' };

/*--------------------------------------------------------------------------
|  send2Vnmr - looks for Vnmr master
|		if found sends message to it. 
|			Author: Greg Brissey	2/19/88
|
|   change for INOVA     Greg Brissey 3/28/96
+--------------------------------------------------------------------------*/
main(argc,argv)
int argc;
char *argv[];
{

    char msgestr[256];
    char addr[MAX_ADDRLEN];
    char hostname[HOSTLEN];
    char message[2048];
    char *chrptr;
    char    *cptr, *proc_name, *ptr;
    int i,j,k,nchr,len;
    int pid,port,stat;
    int master_pid[MAX_MASTERS],master_cnt, argtype;
    ulong_t vpid;

    FILE    *accessfile;
    MSG_Q_ID pVnmrMsgQ;

/*
    for (i=0; i < argc;i++)
	printf("arg[%d]: '%s' \n",i,argv[i]);
*/

    if (argc > 2)
    {
	if (strcmp(argv[2],"debug") == 0)
	{
	    printf("send2Vnmr: Must pass message string to be sent!\n");
	    exit(0);
	}
	else if (argc > 3)
	{
	    if (strcmp(argv[3],"debug") == 0)
	       debug = 1;
	}
    }
    else
    {
	printf("send2Vnmr: Must pass access file name and message string.\n");
	    exit(0);
    }

#ifndef AIX
    /* Better be Solaris else terminate */
    if ( get_systemtype() != 1)
    {
       printf("send2Vnmr: Not a Solaris 2.x system, aborted.\n");
       exit(1);
    }
#endif
    /*-----------------------------------------------------------------------------------
    * The 1st argument can be:
    *         1. secret code to talk to all Vnmr(s) found bypassing access permission
    *         2. Process ID of the Vnmr to send message to, bypassing access permission
    *         3. The Full path to the Vnmr access file, typically in $vnmruser/.talk
    *    	   which contain the Vnmr address (hostname pid)
    * # 3 is the Normal argument type
    *
    * The 2nd argument is the message to be sent to Vnmr (message is typically within double quotes)
    *
    * The 3rd and optional argument is the string "debug" for diagnostic output
    *
    * Note: arg types 1 & 2 only work for INOVA type systems
    */
    /* check for 1st argument type */
    if (strcmp(argv[1],ChkStr) == 0)
    {
       argtype = VALL;
    }
    else if ( (vpid = strtoul(argv[1],(char**) NULL, 10)) != 0)
    {
       argtype = VPID;
    }
    else
    {
       argtype = VFILE;
    }
    /* printf("send2Vnmr: 1st Arg '%s' is %d (1-all,2-pid,3-file)\n",argv[1],argtype); */
    if (debug)
       printf("send2Vnmr: 2nd Arg (message): '%s'\n",argv[2]);
    
    switch(argtype)
    {

      case VALL:
		if ((master_cnt = find_VnmrMaster(&master_pid,MAX_MASTERS)) == 0)
		{
		   printf("send2Vnmr: Could not find Vnmr master\n");
		   exit(1);
		}
		else if (master_cnt > 1)
		{
		   printf("send2Vnmr: WARNING  Multiple Vnmr master(s) found.\n");
		}
         break;

      case VPID:
		master_pid[0] = vpid;
		master_cnt = 1;
         break;

      case VFILE:
		/* open  to see if Vnmr access file is present */
		if ( (accessfile = fopen(argv[1],"r")) == NULL)
		{
		   perror("send2Vnmr: Vnmr access file error:");
		   exit(1);
		}
		if ( (cptr = fgets(tmpstr, 120, accessfile )) == NULL)
		{
		   perror("send2Vnmr: Vnmr access file error:");
		   exit(1);
		}
	        chrptr = strtok(tmpstr,"\n");
		strncpy(addr,chrptr,MAX_ADDRLEN-1);
		addr[MAX_ADDRLEN] = '\0';
/*
		chrptr = strtok(tmpstr," \n");
		strncpy(hostname,chrptr,HOSTLEN-1);
		hostname[HOSTLEN] = '\0';
		chrptr = strtok(NULL," \n");
		vpid = strtoul(chrptr,(char**) NULL, 10);
		master_pid[0] = vpid;
		master_cnt = 1;
*/
		master_cnt = 1;
         break;

    }


    /* get hostname if Arg type is PID or ALL */
    if (argtype != VFILE)
    {
       if ((get_hostname(&hostname[0], HOSTLEN-1)) != 0)
       {
           printf("send2Vnmr: Cannot get hostname, abort !\n");
           exit(1);
       }
    }

    for(i=0; i< master_cnt; i++)
    {
       if (argtype != VFILE)
       {
          sprintf(addr,"%s %d",hostname,master_pid[i]);
       }
       if (debug)
          printf("send2Vnmr: Vnmr MsgQ Addr: '%s', msge: '%s'\n",addr,argv[2]);

       /* libacqcomm.a functions handles sockets or INOVA msgQs*/
       initVnmrComm(addr);
       stat = sendToVnmr( argv[2] );
       if (stat == -1)
       {
          printf("send2Vnmr: Could not send message to Vnmr master: '%s'\n",addr);
          exit(1);
       }
       if (debug)
          printf("send2Vnmr: Message sent.\n");

    }

    exit(0);
}

/* set global solaris flag to system type */
get_systemtype()
{
        char    *cptr;
        FILE    *fpipe;

        fpipe = popen( "uname -r", "r" );
	solaris = 0;
	if (fpipe == NULL)
                return;
	while ((cptr = fgets(tmpstr, 120, fpipe )) != NULL)
	{
	     if (debug)
	       fprintf(stdout,"get_systemtype: '%s'\n",tmpstr);
	     if ((int)strlen(tmpstr) > 0)
	     {
		while (*cptr == ' ')
		   cptr++;
		if ( *cptr == '5')
		    solaris = 1;
	     }
	}
	pclose(fpipe);

  return(solaris);
}

int get_hostname(char *hostname,int namelen)
{
   FILE    *fpipe;
   char *tmp[256];

   fpipe = popen( "uname -n", "r" );
   if (fpipe == NULL)
    {
       *hostname = 0;
       return(-1);
    }
    fgets(tmpstr, 120, fpipe);
    if (debug)
       fprintf(stdout,"get_hostname: '%s'\n",tmpstr);
    strtok(tmpstr,"\n");
    strncpy(hostname,tmpstr,namelen);
    pclose(fpipe);

    return(0);
}

find_VnmrMaster(int *masterpid, int maxpids)
{
        char    field1[12],field2[ 12 ], field3[ 12 ], field4[ 20 ], field5[ 122 ];
        char    field6[12],field7[ 12 ], field8[ 12 ], field9[ 126 ], field10[126];
        char    *fields[4];
        char    *cptr, *proc_name, *ptr;
        int     proc_id, parent_id,ival, len;
        int     master_id,cnt,i;
        int     *pids;
        FILE    *fpipe;


        master_id = 0;
        pids = masterpid;

        fields[0] = field7;
        fields[1] = field8;
        fields[2] = field9;
        fields[3] = field10;

        fpipe = popen( "ps -ef", "r" );
	proc_name = field9;

	if (fpipe == NULL)
                return;

        cnt = 0;
	while ((cptr = fgets(tmpstr, 256, fpipe )) != NULL)
	{
	    if (debug)
	       fprintf(stdout,"find_Vnmrmaster: '%s'\n",tmpstr);
            ival = sscanf(
                        tmpstr, "%s%d%d%s%s%s%s%s%s%s",
                        &field1[ 0 ],
                        &proc_id,
                        &parent_id,
                        &field4[ 0 ],
                        &field5[ 0 ],
                        &field6[ 0 ],
                        &field7[ 0 ],
                        &field8[ 0 ],
                        &field9[ 0 ],
                        &field10[ 0 ]
                         );
            /* fprintf(stdout,"ival: %d\n",ival); */
            if (ival < 6) continue;
            /* fprintf(stdout,"proc name: '%s', PID: %d, f4: '%s', f5: '%s', f6: '%s'\n", 
			proc_name,proc_id,field4,field5,field6); */
            for(i=7; i <= ival; i++)
            {
               if (debug)
	          fprintf(stdout,"fields[%d]: '%s'\n",i,fields[i-7]);
	       len = strlen(fields[i-7]);
             
	       if (len >= 6)
	       {
		   ptr = fields[i-7] + (len - 6);
	           if (strcmp(ptr, "master") == 0)
	           {
		      if (debug)
		         fprintf(stdout,"-----------------> FOUND MASTER <-------------------\n");
                      *pids++ = proc_id;
		      cnt++;
		   }
	       }
	    }
	    if (cnt >= maxpids )
               break;
	}
	pclose(fpipe);

     return(cnt);
}
