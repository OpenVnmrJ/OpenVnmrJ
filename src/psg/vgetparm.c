/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*--------------------------------------------------------------
|  getparm(variable_name,variable_type,tree,variable_address,varsize)
|  char *variable_name,*variable_type,*variable_address,*tree
|  return 1 if error
|  passes back variable
|
|  varsize is the length of the string value tobe returned, no effect on reals
|
|   Author Greg Brissey   4/28/86
+---------------------------------------------------------------*/
extern int Tflag;
getparm(varname,vartype,tree,varaddr,size)
char *varname;
char *vartype;
int   tree;
char *varaddr; /* For Reals, pointer is recased as a double; ie.(double *) */
int	 size;
{
    int ret;
    if ( (strcmp(vartype,"REAL") == 0) || (strcmp(vartype,"real") == 0) )
    {
        if ((ret = P_getreal(tree,varname,(double *)varaddr,1)) < 0)
        {    Werrprintf("Cannot find parameter: %s",varname);
	     if (Tflag)
	         P_err(ret,varname,": ");
	     return(1);
        }
    }
    else 
    {
	if ( (strcmp(vartype,"STRING") == 0) || 
	     (strcmp(vartype,"string") == 0) )
        {
            if ((ret = P_getstring(tree,varname,varaddr,1,size)) < 0)
            {   Werrprintf("Cannot find parameter: %s",varname);
		if (Tflag)
	     	    P_err(ret,varname,": ");
		return(1);
            }
	}
	else
	{   Werrprintf("Variable '%s' is neither a 'real' or 'string'.",
			vartype);
	    return(1);
	}
      }
    return(0);
}

