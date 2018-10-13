/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "abort.h"
#include "group.h"
#include "pvars.h"
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

static int dowarn=1;
void getparmnwarn()
{
  dowarn=0;
}

extern int bgflag;
int getparm(char *varname, char *vartype, int tree,
            void *varaddr, int size)
{
    int ret;

    if ( (strcmp(vartype,"REAL") == 0) || (strcmp(vartype,"real") == 0) )
    {
        if ((ret = P_getreal(tree,varname,(double *)varaddr,1)) < 0)
        {
           if (dowarn)
           {
             text_error("Cannot get parameter: %s\n",varname);
	     if (bgflag)
	         P_err(ret,varname,": ");
           }
           else
           {
              dowarn = 1;
           }
	   return(1);
        }
    }
    else 
    {
	if ( (strcmp(vartype,"STRING") == 0) || 
	     (strcmp(vartype,"string") == 0) )
        {
            if ((ret = P_getstring(tree,varname,(char *)varaddr,1,size)) < 0)
            {
              if (dowarn)
              {
                text_error("Cannot get parameter: %s\n",varname);
		if (bgflag)
	     	    P_err(ret,varname,": ");
              }
              else
              {
                 dowarn=1;
              }
              return(1);
            }
	}
	else
	{   text_error("Variable '%s' is neither a 'real' or 'string'.\n",
			vartype);
	    return(1);
	}
      }
    return(0);
}

int getArrayparval(const char *parname, double *parval[])
{
  int nn, j, k;
  double am=0.0;

  if (P_getsize(CURRENT,parname,&nn) <= 0)
  {
      abort_message("parameter %s does not exist\n", parname);
  }

  (*parval = (double *) calloc(nn, sizeof(double)));
  for (j=0; j<nn; j++)
  {
	if ((k = P_getreal(CURRENT, parname, &am, j+1)) < 0)
	{
           abort_message("parameter %s does not exist\n", parname);
	}
	(*parval)[j] = am;
  }
  return nn;
}

