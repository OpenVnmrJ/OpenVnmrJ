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
/*-----------------------------------------------------------------------------
|  findps - receives the output of ps ax, and looks for psg 
|		when it is found it attaches dbxtool to psg
|			Author: Greg Brissey	11/2/87
+-----------------------------------------------------------------------------*/
main(argc,argv)
int argc;
char *argv[];
{
    char buffer[256];
    char proc[256];
    char syscmd[256];
    char *fields[100];
    char *chrptr;
    char *rindex();
    int i,j,k,nchr,len;
    int signal,pid;
    int debug;

    debug = 0;
    /*for (i=0; i < argc;i++)
	printf("arg[%d]: '%s' \n",i,argv[i]);*/
    if (argc < 2) 
    {
       fprintf(stdout,"Usage: findps(pulse seq. name) \n");
       exit(0);
    }
    strcpy(proc,argv[1]);
    if (argc > 2)
    {
	if (strcmp(argv[2],"debug") == 0)
	    debug = 1;
    }
    k = 0;
    /* --------- get the ps ax that is being piped to us ----------------*/
    while (gets(buffer))
    {
	if (debug)
    	   fprintf(stdout,"'%s'\n",buffer);
    	/*fprintf(stdout,"size = %d\n",strlen(buffer));*/
        nchr = 0;
        j = 0;
	chrptr = &buffer[0];
	len = strlen(buffer);
	for (i=0;i<len;i++)
        {
	    /*fprintf(stdout,"buffer[%d] = '%c'\n",i,buffer[i]);*/
	    if ( ((buffer[i] == ' ') || (buffer[i] == '	'))
			 && (nchr != 0) )
 	    {
	        fields[j] = chrptr;
		j++;
		buffer[i] = '\0';
	        chrptr = &buffer[i+1];
		nchr = 0;
	    }
	    else
	       if ( !((buffer[i] == ' ') || (buffer[i] == '	')) )
		  nchr++;
	       else
		  chrptr++;
        }
	fields[j] = chrptr;
	j++;

        /* ----------------------------------------*/
	/*for (i=0;i<len;i++)
	   fprintf(stdout,"%c",buffer[i]); */
        /*fprintf(stdout,"\n");*/
	/*for (i=0;i<j;i++)
    	   fprintf(stdout,"fields[%d]='%s'\n",i,fields[i]);*/
        /* ----------------------------------------*/

        for (i=4; i < j; i++)
        {
	   if (debug)
	       fprintf(stdout,"field[%d]:'%s'\n",i,fields[i]);
           chrptr = rindex(fields[i],'/');
           if (chrptr == (char *) 0)
              chrptr = fields[i];
	   /*if (debug)
	       fprintf(stdout,"field[%d]:'%s'\n",i,chrptr);*/
           if (chrptr == (char *) 0)
		continue;
 
           if (chrptr != NULL)
	   {
	     if (strcmp(chrptr,"psg") == 0)
	     {
	        pid = atoi(fields[0]);
	        fprintf(stdout,"attaching dbxtool to %s with pid = %d\n",proc,pid);
		sprintf(syscmd,"dbxtool %s %d &",proc,pid);
		system(syscmd);
		exit(0);
	     }
	   }
	}
    }
    fprintf(stdout,"No psg process found.\n");
    exit(0);
}
