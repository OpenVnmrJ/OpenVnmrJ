/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include "allocate.h"
#include "tools.h"
#include "wjunk.h"


struct stringElement {
	struct stringElement	*nextElement;
	char			*thisString;
};


/*  Each method expects a base argument (or a reference to base).
    The application program is responsible for storing it, but
    can express it as an opaque type, for example: char *base     */

/*  Several methods expect an id argument.  See allocateWithId in
    allocate.c for general information on this kind of argument.  */

static struct stringElement *findLastElement(void *base)
{
	struct stringElement	*thisElement;

	if (base == NULL)
	  return( NULL );

	thisElement = (struct stringElement *)base;

	while (thisElement->nextElement != NULL)
	  thisElement = thisElement->nextElement;

	return( thisElement );
}

int getNumberOfStrings(void *base )
{
	int			 count;
	struct stringElement	*thisElement;

	if (base == NULL)
	  return( 0 );

	thisElement = (struct stringElement *)base;
	count = 1;

	while (thisElement->nextElement != NULL) {
		thisElement = thisElement->nextElement;
		count++;
	}

	return( count );
}

int addStringToEnd(void **refToBase, char *string, char *id )
{
	struct stringElement	*thisElement, *lastElement;

	thisElement = (struct stringElement *)
			allocateWithId( sizeof( struct stringElement ), id );
	if (thisElement == NULL)
	  return( -1 );

	thisElement->nextElement = NULL;
	thisElement->thisString = newStringId( string, id );
	if (thisElement->thisString == NULL)
	  return( -1 );

/*  If you look at the current newStringId (tools.c) you will see a
    NULL return value from newStringId is not very likely.  Nevertheless
    the preceeding check is useful should newStringId ever get changed.  */

	if (*refToBase == NULL) 
	  *refToBase = thisElement;
	else {
		lastElement = findLastElement( *refToBase );
		lastElement->nextElement = thisElement;
	}

	return( 0 );
}

void deleteFirstString(void **refToBase )
{
	struct stringElement	*thisElement;

	if (*refToBase == NULL)
	  return;

	thisElement = (struct stringElement *) *refToBase;
	*refToBase = thisElement->nextElement;

	release( thisElement->thisString );
	release( thisElement );
}

void printEachString(void *base )
{
	struct stringElement	*thisElement;

	if (base == NULL)
	  return;

	thisElement = (struct stringElement *)base;

	do {
		if (thisElement->thisString)
		  Wscrprintf("%s\n", thisElement->thisString );
		thisElement = thisElement->nextElement;
	}
	  while (thisElement != NULL);
}

/*  debug... debug... debug...  */

void dumpList(void *base )
{
	struct stringElement	*thisElement;

	if (base == NULL) {
		fprintf( stderr, "no list\n" );
		return;
	}

	thisElement = (struct stringElement *)base;

	do {
		fprintf( stderr, "element present at %p\n", thisElement );
		if (thisElement->thisString)
		  fprintf( stderr, "%d:  %s\n", (int) strlen( thisElement->thisString ),
	                   thisElement->thisString
		  );
		else
		  fprintf( stderr, "element's string is NULL\n" );
		fprintf( stderr, "next element: %p\n", thisElement->nextElement );

		thisElement = thisElement->nextElement;
	}
	  while (thisElement != NULL);
}
