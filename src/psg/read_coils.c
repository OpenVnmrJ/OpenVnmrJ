/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>

#define MAX_SHIMS 32
#define ERROR -1
#define OK 0

extern int debug;

/* save enough space for shim variable names */
char *shimnames[MAX_SHIMS] = {
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          ", "          ",
                        "          ", "          " };


/*-----------------------------------------------------------------------
|       read_shimcoils()/0
|       read shim coil assignment file and fill in shimnames ordered
|       in coil number.
|
+-----------------------------------------------------------------------*/
read_shimcoils()
{
FILE    *fopen();
FILE    *shimfile;
 
extern char     systemdir[];
char            waste[130];
char            filename[1024];
char            name[10];
int             i,line,shim,shims,ret;
int             coil;
 
 
/* clear and initialize shimnames */
    for (shim=0;shim<MAX_SHIMS;shim++)
        strcpy(shimnames[shim],"");

    strcpy(filename,systemdir);
    strcat(filename,"/shims/coil_assignments");
#ifdef DEBUG
    if (debug)
        fprintf(stdout,"reading coil assignment file,filename=%s\n",filename);
#endif


   shimfile=fopen(filename,"r");
   if (shimfile==0)
   {
     fprintf(stdout,"Cannot open coil assignment file:'%s'\n",filename);
     return(ERROR);
   }

/* read past comments in file */
   line = 1;
   do
   { fgets(waste,128,shimfile); line++;
     /*fprintf(stderr,"line %d: '%s'",line,waste);*/
   } while (waste[0] == '#');
 
   /* Number of shims to read from file */
   if (ret = sscanf(waste,"%d",&shims)!=1)
   {
     fprintf(stdout,"syntax error, file=%s, line=%d\n",filename,line);
     fclose(shimfile);
     return(ERROR);
   }
   /*fprintf(stderr,"shims: %d\n",shims);*/
 
   /* read shims  */
   for (shim=0; shim<shims; shim++)
   {
     ret = fscanf(shimfile,"%s%d",name,&coil);
     if (ret == EOF) /* if end of file stop */
     {
           break;
     }
     /*fprintf(stderr,"name: '%s', coil: %d\n",name,coil);*/
     strcpy(shimnames[coil],name);
   }
   return(OK);
}

shimcoil(name)
char *name;
{
    register int i;

    for (i=0; i< MAX_SHIMS ; i++)
    {
      if (strcmp(name,shimnames[i]) == 0 )
         return(i);
    }
    return(-1);
}
