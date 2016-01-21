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
#include <strings.h>
#include <interix/security.h>

extern char **environ;

int main(int argc, char* argv[])
{
    struct usersec *userdata;
    int  success = 0;
    int  numArgs = argc - 4;
    int  i;
    char *path;
    char *args[numArgs];
    int  debug = 0;  /* set to 1 to get debug output */
   
   
    /* Fill the usersec for sending to execv_asuser() */
    userdata->user = argv[1];
    userdata->password = argv[2];
    path = argv[3];

    if(debug)
        printf("exec_asuser: username=%s, password=%s, path=%s,", 
               userdata->user, userdata->password, path);


    /* Fill args with how ever many args there are. */
    for (i=0; i < numArgs; i++) {
        args[i] = argv[i + 4];
        if(debug)
            printf(" arg%d=%s,", i, args[i]);
    }
    if(debug)
        printf("\n"); 

    /* Execute the command */
    success = execv_asuser(*userdata, path, args);

}

