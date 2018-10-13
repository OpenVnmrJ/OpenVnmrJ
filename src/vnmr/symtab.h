/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SYMTAB_H
#define SYMTAB_H
/*------------------------------------------------------------------------------
|
|	symbol.h
|
|	Typedefs for general purpose symbol table support.
|
+-----------------------------------------------------------------------------*/

struct _symbol { char           *name;
	         void           *val;
	         struct _symbol *left;
	         struct _symbol *right;
	       };
typedef struct _symbol symbol;

extern void balance(symbol **pp);
extern int  delName(symbol **pp, const char *n);
extern int  delNameWithBalance(symbol **pp, const char *n);
#endif
