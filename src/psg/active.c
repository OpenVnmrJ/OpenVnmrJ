/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>
#include <stdio.h>
#include "variables.h"
#include "group.h"
#include "pvars.h"
extern int bgflag;
/* ---------------------------------------------------------------
|  var_active(varname,tree);
|
|  Purpose:
|	This module determines if the variable passed is active or not.
|	This is determined by looking at the variable information
|	 structure "_vInfo".
|	if a real variable the info.active is check.
|	if a string then it's value is checked for 'unused' value.
|       Returns 1 if active, 0 if not, and -1 if error.
|
|				Author Greg Brissey  5/13/86
+------------------------------------------------------------------*/
int var_active(char *varname, int tree)
{
    char	strval[20];
    int		ret;
    vInfo	varinfo;		/* variable information structure */

    if (bgflag)
	fprintf(stderr,"active(): Variable: %s, Tree: %d \n",varname,tree);
    if ( (ret = P_getVarInfo(tree,varname,&varinfo)) )
    {
	if (bgflag)
           fprintf(stderr,"Cannot find the variable: %s",varname);
	return(-1);
    }
    if (bgflag)
	fprintf(stderr,"active(): vInfo.basicType = %d \n",varinfo.basicType);
    if (varinfo.basicType != T_STRING)
    {
    	if (bgflag)
	    fprintf(stderr,"active(): vInfo.active = %d \n",varinfo.active);
    	if (varinfo.active == ACT_ON)
	    return(1);
    	else
	    return(0);
    }
    else  /* --- variable is string check is value for 'undef' --- */
    {
	P_getstring(tree,varname,strval,1,10);
	/*getparm(varname,"string",tree,&strval[0],10);*/
	if (bgflag)
	    fprintf(stderr,"active(): Value of String variable: '%s'\n",strval);
    	if ((strncmp(strval,"unused",6) == 0) || 
	    (strncmp(strval,"Unused",6) == 0) )
	    return(0);
    	else
	    return(1);
    }
}

