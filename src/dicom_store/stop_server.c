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

/********************************************************************/
/* Name        : stop_server.c                                      */
/* Description : Stops the archive image server by killing the      */
/*                                image_server process              */
/* Created by  : Mindteck (India) limited                           */
/* Created on  : 01.05.2003                                         */
/* Modified on :                                                    */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************************************************************/
/* Function    : main                                               */
/* Description : The function kills the image_server process        */
/********************************************************************/

int main (int argc, char *argv[])
{
   char strCommand[151]; 

/* Initialize variables */
   strCommand[0]='\0';     /* The command that kills the process */

/* Check for correct usage of the application */
  while (--argc > 0 && *(++argv)[0] == '-') {
	switch ((*argv)[1]) {
             default:
                 printf("Unrecognized option: %s\n", *argv);
                 break;
        }
  }

/* Construct the command */

   strcpy (strCommand , "/usr/bin/kill -9 ");
   strcat (strCommand, 
           " `ps -ef | grep 'image_server -f' | tail -1 | awk '{ print $2 }'`");

   printf (strCommand);
   printf ("\n");

/* Execute the command */
   system(strCommand);
}
