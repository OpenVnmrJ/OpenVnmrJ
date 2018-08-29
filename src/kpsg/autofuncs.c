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
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms2.h"
#include "shims.h"
#include "pvars.h"
#include "abort.h"
#include "cps.h"
/* #include "macros.h" */


extern int  bgflag;	/* debug flag */
extern int newacq;
extern int oldwhenshim;
extern int whenshim;
/* extern int	ok2bumpflag; */
/*------------------------------------------------------------------
|	loadshims()/0
|	Load shims if load == 'y'.  Will be set by arrayfuncs if
|	any shims are arrayed.
+-----------------------------------------------------------------*/
loadshims()
{
    char load[MAXSTR];

    if ((P_getstring(CURRENT,"load",load,1,15) >= 0) &&
        ((load[0] == 'y') || (load[0] == 'Y')) )
    {
        int index;
        const char *sh_name;

        P_setstring(CURRENT,"load","n",0);
        putcode(LOADSHIM);		/* Load Shim DAC values,  Acode */
        putcode(MAX_SHIMS);
        for (index= Z0 + 1; index < MAX_SHIMS; index++)
        {
           if ((sh_name = get_shimname(index)) != NULL)
              putcode( (codeint) getval(sh_name ) );
           else
              putcode(0);
        }
    }
}

/*------------------------------------------------------------------
|	initwshim()/0
|	invoke autoshimming if required
+-----------------------------------------------------------------*/
initwshim()
{
    int shimatanyfid,shimmode;
    char  flg[MAXSTR];

    /*----  BACKGROUND SHIMMING ---------*/
     whenshim = setshimflag(wshim,&shimatanyfid);
    if (bgflag)
        fprintf(stderr,"initauto2(): shimmask: %d whenshim = %d \n",
	    shimatanyfid,whenshim);
    if ((P_getstring(GLOBAL,"hdwshim",flg,1,15)) < 0)
    	shimmode = 0;
    else if ((flg[0] == 'y') || (flg[0] == 'Y'))
    	shimmode = 2;
    else
    	shimmode = 0;
    if ( (shimatanyfid || (whenshim != oldwhenshim) ) &&
	 ( (setupflag == GO) || (setupflag == SHIM) || (setupflag == SAMPLE) )
       )
    {
        shimmode += 1;
        oldwhenshim = whenshim;
/*	ss4autoshim(); */
    }
    putmethod(shimmode);        /* generate Acodes for auto shimming */
}
/*-------------------------------------------------------------------
|
|       setshimflag()
|       From wshim set shimatanyfid flag and the automation whenmask
|       return whenmask;
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   9/20/89   Greg B.     1. Corrected arguments to new call form in cps.c
+-------------------------------------------------------------------*/
setshimflag(wshim,flag)
char *wshim;
int *flag;
{
    int shimatanyfid;
    int whenmask;

    if (setupflag == SHIM)
        strcpy(wshim,"fid");            /* force wshim=fid when SHIM given */
   
    switch( wshim[0] )
    {
        case 'N':
        case 'n': shimatanyfid = FALSE;         /* no autoshimming */
                  whenmask = 0;
                  break;
	case 'G':
	case 'g': shimatanyfid = FALSE;		/* no autoshimming */
		  whenmask = 0;		/* dummy case for gradient shimming */
		  break;
        case 'E':
        case 'e': shimatanyfid = FALSE;         /* shimming between experiment*/
                  whenmask = 1;
                  break;
        case 'F':                               /* shimming between fids */
        case 'f': if ((wshim[1] >= '0') && (wshim[1] <= '9'))
                  {  int count = 0;
                     int i = 1;
                     while ((wshim[i] >= '0') && (wshim[i] <= '9'))
                     {
                       count = count * 10 + (wshim[i] - '0');
                       i++;
                     }
                    shimatanyfid = (((ix - 1) % count) == 0) ? TRUE : FALSE;
                  }
                  else
                    shimatanyfid = TRUE;
                  whenmask = 2;
                  break;
        case 'B':
        case 'b': shimatanyfid = TRUE;          /* shimming between block size*/
                  whenmask = 4;
                  break;
        case 'S':
        case 's': shimatanyfid = FALSE;         /*shimming after sample change*/
                  whenmask = 8;
                  break;
        default:  text_error("wshim has an invalid value");
                  psg_abort(1);
    }
    *flag = shimatanyfid;
    return(whenmask);
}

