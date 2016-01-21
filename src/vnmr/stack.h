/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	Parser stack types...
|
|	Note that <tval> is used for handing back token values.  This allows
|	file (name, line and column) info to be associated with each token
|	seen.  The declaration for <fileInfo> may be found in node.h
|
+-----------------------------------------------------------------------------*/

typedef struct { char       *value;
		 fileInfo    location;
	       }                        charInfo;

typedef struct { double      value;
		 fileInfo    location;
	       }                        doubleInfo;

typedef struct { int         token;
		 fileInfo    location;
	       }                        tokenInfo;

typedef union  { charInfo    cval;
		 doubleInfo  dval;
		 node       *nval;
		 tokenInfo   tval;
	       }                        YYSTYPE;
