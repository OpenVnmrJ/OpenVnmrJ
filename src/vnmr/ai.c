/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------
|							|
|			ai.c				|
|							|
|							|
|     ph    -    phased display (1D, F2)		|
|     av    -    absolute value display (1D, F2)	|
|     pwr   -    power display (1D, F2)			|
|     pa   -     phase angle display (1D, F2)		|
|     db   -     20log(pwr)+170 power display (1D, F2)	|
|     ph1   -    phased display (F1)			|
|     av1   -    absolute display (F1)			|
|     pwr1  -    power display (F1)			|
|     ph2   -    phased display third dimension         |
|     av2   -    absolute display third dimension       |
|     pwr2  -    power display third dimension          |
|							|
|     ai    -    absolute intensity mode		|
|     nm    -    normalized mode			|
|     dc    -    set flag for spectral DC correction	|
|     cdc   -    cancel spectral DC flag		|
|							|
+------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "group.h"
#include "data.h"
#include "init2d.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"
#include "vnmrsys.h"

extern int debug1;
extern int currentindex();
extern int rel_spec();
extern void dodc(float *ptr, int offset, double oldlvl, double oldtlt);
extern void  Wturnoff_buttons();

/*********/
static int dc()
/*********/
{
    float *spectrum;
    int   trace;      /* index of the current spectrum         */

    if(init2d(1,1)) ABORT;
    trace = currentindex();
    if (trace == 0) trace = 1;
    if ((spectrum=gettrace(trace-1,0)) == 0) ABORT;
    disp_status("DC      ");
    dodc(spectrum,1,c_block.head->lvl,c_block.head->tlt);
    disp_status("        ");
    P_setreal(CURRENT,"lvl",lvl,0);
    P_setreal(CURRENT,"tlt",tlt,0);
    return(rel_spec());
}


/*---------------------------------------
|					|
|		   ai()			|
|					|
|   Main entry point for ai() routine.	|
|					|
+--------------------------------------*/
int ai(int argc, char *argv[], int retc, char *retv[])
{
   int e;

   (void) argc;
   (void) retc;
   (void) retv;
   Wturnoff_buttons();
   if ((strcmp(argv[0], "av") == 0) ||
       (strcmp(argv[0], "ph") == 0) ||
       (strcmp(argv[0], "pa") == 0) || 
       (strcmp(argv[0], "db") == 0) ||
       (strcmp(argv[0], "pwr") == 0))
   {
      if ( (e = P_setstring(CURRENT, "dmg", argv[0], 0)) )
         P_err(e, "dmg", ":"); 
      if (strcmp(argv[0], "pa") == 0)
        if ( (e = P_setstring(CURRENT, "dmg", "pa", 0)) )
           P_err(e, "dmg", ":");
      appendvarlist("dmg");
   }
   else if ((strcmp(argv[0], "av1") == 0) ||
            (strcmp(argv[0], "ph1") == 0) ||
            (strcmp(argv[0], "pa1") == 0) ||
            (strcmp(argv[0], "pwr1") == 0))
   {
      if ( (e = P_setstring(CURRENT, "dmg1", argv[0], 0)) )
      {
         if ( (e = P_creatvar(CURRENT,"dmg1",ST_STRING)) )
         {
            P_err(e, "dmg1", ":");
         }
         else
         {
	    P_setgroup(CURRENT,"dmg1",G_DISPLAY);
            P_setlimits(CURRENT,"dmg1",5.0,0.0,0.0);
            if ( (e = P_setstring(CURRENT, "dmg1", argv[0], 0)) )
               P_err(e, "dmg1", ":");
            if (strcmp(argv[0], "pa1") == 0)
               if ( (e = P_setstring(CURRENT, "dmg1", "pa1", 0)) )
                  P_err(e, "dmg1", ":");
         }
      }
      appendvarlist("dmg1");
      if (strcmp(argv[0], "pa1") == 0)
         if ( (e = P_setstring(CURRENT, "dmg1", "pa1", 0)) )
            appendvarlist("dmg1");
   }
   else if ((strcmp(argv[0], "av2") == 0) ||
            (strcmp(argv[0], "ph2") == 0) ||
            (strcmp(argv[0], "pa2mode") == 0) ||
            (strcmp(argv[0], "pwr2") == 0))
   {
      if ( (e = P_setstring(CURRENT, "dmg2", argv[0], 0)) )
      {
         if ( (e = P_creatvar(CURRENT,"dmg2",ST_STRING)) )
         {
            P_err(e, "dmg2", ":");
         }
         else
         {
	    P_setgroup(CURRENT,"dmg2",G_DISPLAY);
            P_setlimits(CURRENT,"dmg2",5.0,0.0,0.0);
            if ( (e = P_setstring(CURRENT, "dmg2", argv[0], 0)) )
               P_err(e, "dmg2", ":");
            if (strcmp(argv[0], "pa2mode") == 0)
               if ( (e = P_setstring(CURRENT, "dmg2", "pa2", 0)) )
                  P_err(e, "dmg2", ":");
         }
      }
      appendvarlist("dmg2");
      if (strcmp(argv[0], "pa2mode") == 0)
         if ( (e = P_setstring(CURRENT, "dmg2", "pa2", 0)) )
            appendvarlist("dmg2");
   }
   else if ((strcmp(argv[0], "ai") == 0) ||
            (strcmp(argv[0], "nm") == 0))
   {
      if ( (e = P_setstring(CURRENT, "aig", argv[0], 0)) )
         P_err(e, "aig", ":"); 
      appendvarlist("aig");
   }
   else if (strcmp(argv[0], "cdc") == 0)
   {
      P_setreal(CURRENT, "lvl", 0.0, 0);
      P_setreal(CURRENT, "tlt", 0.0, 0);
      appendvarlist("lvl");
      appendvarlist("tlt");
      if ( (e = P_setstring(CURRENT, "dcg", "cdc", 0)) )
         P_err(e, "dcg", ":"); 
   }
   else if (strcmp(argv[0], "dc") == 0)
   {
      if (dc())
      {
         Werrprintf("cannot do drift correction");
      }
      else
      {
         if ( (e = P_setstring(CURRENT, "dcg", "", 0)) )
            P_err(e, "dcg", ":"); 
         appendvarlist("lvl");
         appendvarlist("tlt");
      }
   }
   RETURN;
}
