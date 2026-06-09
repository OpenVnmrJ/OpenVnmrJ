/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
/*------------------------------------------------------------------
|
|	findsname(name,item,count)
|
|  	tests to see if the given name is in the name list. 
|	The list must be sorted.
|	Performs a binary search for string.
|	returns posistion (0-count) in list if found.
|	returns -1 if not found.
|
| 		Author  Greg Brissey  6/04/86
+---------------------------------------------------------------------*/
extern int bgflag;
#define NOTFOUND -1
#define DGLEVEL 2

findsname(name,item,count)
char *name;
char *item[];
int   count;
{
    register int low,high,mid;
    register int result;

    low = 0;
    high = count - 1;
    while( low <= high)
    {
	mid = (low + high) / 2;
	result = strcmp(name,item[mid]);
	if (bgflag > DGLEVEL)
	    fprintf(stderr,"findsname(): name: '%s', list[%d]: '%s' \n",
			name,mid,item[mid]);
	if (result < 0)
	    high = mid - 1;
	else
	    if (result > 0)
		low = mid + 1;
	    else
		return(mid);

    }
    return(NOTFOUND);	/* not found */
}


