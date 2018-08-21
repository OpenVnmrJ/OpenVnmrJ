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
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------
|  trmap - map generated from nm68 for a lo68 link load, is pipe to this
|          routine, the HEX address is translated to decimal. The decimal 
|	   value is added to the map listing. Then sorting the map is 
|	   straightforward.
|	   (e.g., cat xr.map | trmap | sort -t\( +1n > xr.smap ) 
|			Author: Greg Brissey	6/10/88
+-----------------------------------------------------------------------------*/
main(argc,argv)
int argc;
char *argv[];
{
    char buffer[256];
    char *fields[100];
    char *chrptr;
    char *rindex();
    int i,j,k,nchr,len;
    int signal,pid;
    int debug;
    long value;

    debug = 0;
    /*for (i=0; i < argc;i++)
	printf("arg[%d]: '%s' \n",i,argv[i]);*/
    if (argc > 1)
    {
	if (strcmp(argv[1],"debug") == 0)
	    debug = 1;
    }
    k = 0;
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
	/*for (i=0;i<j;i++)
    	   fprintf(stdout,"'%s'\n",fields[i]);*/
        if (j > 2)
        {
	    sscanf(fields[1],"%lx",&value); 
        }
	fprintf(stdout,"%s    	",fields[0]);
        for(i=1;i<j;i++)
	   fprintf(stdout,"%s  ",fields[i]);
        fprintf(stdout,"(%ld)",value);
        fprintf(stdout,"\n");
    }

    exit(0);
}
