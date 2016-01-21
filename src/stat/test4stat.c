/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  This program MUST BE ON THE RECEIVING END
    of a pipe whose input is 'ps -xa'		*/

#include <stdio.h>
#include <string.h>

main()
{
    char buffer[256];
    char *fields[100];
    char *chrptr;
    char *rindex();
    int i,j,k,nchr,len;
    int signal,pid;
    k = 0;
    while (gets(buffer))
    {
        nchr = 0;
        j = 0;
	chrptr = &buffer[0];
	len = strlen(buffer);
	for (i=0;i<len;i++)
        {
	    if ( ((buffer[i] == ' ') || (buffer[i] == '\t'))
			 && (nchr != 0) )
 	    {
	        fields[j] = chrptr;
		j++;
		buffer[i] = '\0';
	        chrptr = &buffer[i+1];
		nchr = 0;
	    }
	    else
	       if ( !((buffer[i] == ' ') || (buffer[i] == '\t')) )
		  nchr++;
	       else
		  chrptr++;
        }
	fields[j] = chrptr;
	j++;

        chrptr = rindex(fields[4],'A');
        
        if (chrptr != NULL)
	{
	  if (strcmp(chrptr,"Acqstat") == 0)
	  {
	     k++;
	  }
	}
    }
    if (k > 1) {
/*	fprintf(stdout,"Multiple Acqstat(s) found\n");  */
	exit(1);
    }
    if (k == 1) {
/*	fprintf(stdout,"Single Acqstat found\n" );  */
	exit(1);
    }
/*    fprintf( stdout,"No Acqstat found\n" );  */
    exit(0);
}
