/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------------
|
|	createparams.c
|
|	This module contains code to do new parameter creation using Jpsg
|
|
+-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "vnmrsys.h"
#include "group.h"
#include <unistd.h>
#include "vfilesys.h"
#include "wjunk.h"

extern int getparm(char *vname, char *vtype, int tree, void *vaddr, int size);
extern int acq(int argc, char *argv[], int retc, char *retv[]);

int createparams(int argc, char *argv[], int retc, char *retv[])
{
  char    *sfile;
  char    psgfile[MAXPATH];
  char    dpsfile[MAXPATH];
#ifdef VNMRJ
  char    VJpanelfilename[MAXPATH];
  FILE    *VJpanelfile;
#endif 

  (void) retc;
  (void) retv;
  int JFlag=0, k;

           if (getparm("seqfil", "string", CURRENT, dpsfile, MAXPATH))
           {   Werrprintf("Error: parameter 'seqfil' does not exist");
               return(0);
           }
           sfile = dpsfile;

   /* check for existence of .psg file in seqlib */
        if (*sfile != '/')
        {
            char    psgJname[MAXPATH];

            strcpy(psgJname,sfile);
            strcat(psgJname,".psg");
            if (appdirFind(psgJname, "seqlib", psgfile, "", R_OK|X_OK|F_OK) == 0)
            {
                Werrprintf("Error: sequence '%s' does not exist. Compile pulse sequence", sfile);
                return(1);
            }
        }
        else
        {
            strcpy (psgfile, sfile);
            if (access(psgfile, 0) != 0) {
               Werrprintf("Error: sequence '%s' does not exist. Compile pulse sequence", sfile);
               return(1);
            }
        }

    k = strlen(psgfile);
    if ( ! strcmp(psgfile+k-3,"psg") )
    {
       JFlag = 1;
    }
    else 
    {
      Werrprintf("Error: createparams not implemented for C sequences ");
      return(1);
    }

    if (JFlag == 1) {
      acq(argc,argv,0,NULL);

#ifdef VNMRJ
      sprintf(VJpanelfilename,"%s/templates/vnmrj/interface/spincadparams.xml",userdir);
      VJpanelfile = fopen(VJpanelfilename,"r");
      if (VJpanelfile != NULL) {
        fclose(VJpanelfile);
      } else {
        Werrprintf("Error in go command (called from SpinCAD) or in parsing pulse sequence");
        Werrprintf("spincadparams.xml file not generated. Unable to do panel display");
        Werrprintf("Check to make sure ~/vnmrsys/templates/vnmrj/interface directory exists");
        return(1);
      }
#endif 
      
    }
    RETURN;
}
