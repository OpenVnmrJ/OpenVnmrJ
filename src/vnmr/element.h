/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------
|       element.h
|
|       This include file is for programs using the element routine
|	for handling muliple item selection in the canvas.
+--------------------------------------------------------------------*/

struct _Element {               /* stucture of an element */
        char *info;             /* pointer to info packet */
        char *name;             /* pointer to element name */
        int   marked;           /* if item is selected (or marked */
        struct _Element *prev;   /* pointer to previous element */
        struct _Element *next;   /* pointer to next element */
                };
typedef struct _Element Element;

struct _Elist {
	struct _Element *firstElement;	/* pointer to element list */
	struct _Element *lastElement;	/* pointer to last element */
	int	        size;		/* size of element list */
		};
typedef struct _Elist Elist;

extern struct _Elist *initElementStruct();
extern char          *getFirstSelection();
extern char          *getNextSelection();
extern char          *getInfoFromElement();
