/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------
|	node.h
|	
|	This file condtains definitions used for macro node tress and
|	macro stuff
|
/+-----------------------------------------------------------------------*/
#ifndef NODE_H
#define NODE_H


#define NOSEARCH	0
#define SEARCH 		1

/*  Use PERMANENET with saveMacro to store the macro's code tree in
    the permanent cache.  Use EXTENT_OF_CMD to store the macro's
    code tree in a temporary cache which is deleted when the command
    entered at the keyboard is complete.				*/

#define PERMANENT	0
#define EXTENT_OF_CMD	1

struct _fileInfo { int             column;
		   int             line;
		   char           *file;
		 };
typedef struct _fileInfo fileInfo;

struct _node { int             flavour;
	       fileInfo        location;
	       union { char   *s;
		       double  r;
		     } v;
	       struct _node   *Lson;
	       struct _node   *Rbro;
	     };
typedef struct _node node;

extern node *newNode(int, fileInfo *);
extern void addLeftSon(node *, node *);

#endif
