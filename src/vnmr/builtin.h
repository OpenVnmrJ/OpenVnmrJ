/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|	builtin.h
|
|	This include file contains the structure of a command
+-------------------------------------------------------------------------*/
struct _cmd { char   *n;
	      int   (*f)();
	      int    graphcmd;
	      /* int    destruct; */
	    };
typedef struct _cmd cmd;

