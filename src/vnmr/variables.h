/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef VARIABLES_H
#define VARIABLES_H
/*------------------------------------------------------------------------------
|
|	variables.h
|
|	This include file contains definitions for the various structures
|	used for variables.  Variables come in two main types: Reals and
|	Strings.  A variable may also be an array of either of these two
|	basic types.
|
+-----------------------------------------------------------------------------*/

#define T_UNDEF  0
#define T_REAL   1
#define T_STRING 2

#define ST_UNDEF	0
#define ST_REAL		1
#define ST_STRING	2
#define ST_DELAY	3
#define ST_FLAG		4
#define ST_FREQUENCY	5
#define ST_PULSE	6
#define ST_INTEGER	7

union _choice { char   *s;
		 double  r;
	       };
typedef union _choice choice;

/*
 *	Variables have Lvals (addresses) while constants and expressions
 *	usually don't.  An Lval consists of a pointer to an Rval.
 *
 *	Both variables and expressions have Rvals (the "value" of the
 *	variable or expression).
 *
 *	Expressions have Tvals (variables don't since their Lvals take care
 *	of this).  A Tval represents a "type", simple and plain.
 */

struct _Rval { union _choice  v;
	       struct _Rval  *next;
	     };
typedef struct _Rval Rval;

struct _Tval {
	       short basicType;
               int size;
	     };
typedef struct _Tval Tval;

struct _pair { struct _Tval   T;
	       struct _Rval  *R;
	     };
typedef struct _pair pair;

/*
 *	This is a variable information packet used in the 
 *	P_getVarInfo call
*/
struct _vInfo   { short		active;    /* 1-active  0-not active */
		  short		Dgroup;    /* Display group */
		  short		group;	   /* group */
		  short		basicType; 
		  short		subtype;   
		  short		Esize;     /*  size of enumeration array */
		  int		size;      /*  size of array */
		  int		prot;      /*  protection  bits */
		  double	minVal;
		  double 	maxVal;
		  double	step;
		};
typedef struct _vInfo vInfo;

/*
 *	Each variable has a mess of info associated with it...
 */

struct _varInfo { short           active;
		  short           Dgroup;
		  short           Ggroup;
		  short		subtype;
		  int 		prot;
		  double        minVal;
		  double        maxVal;
		  double        step;
		  struct _Tval  T;
		  struct _Rval *R;
		  struct _Tval  ET;
		  struct _Rval *E;
		};
typedef struct _varInfo varInfo;


extern varInfo *createVar(char *n);
extern varInfo *findVar(const char *n);
extern int      getTreeIndex(const char *n);
extern varInfo *P_getVarInfoAddr(int tree, char *name);
extern int      P_getVarInfo(int tree, const char *name, vInfo *info);
extern int goodName(const char *n);
const char  *whatActive(int active);
const char  *whatGroup(int group);
const char  *whatType(int type);

#ifdef VNMRJ

#include "symtab.h"
extern const char *getRoot(int index);
extern symbol **getTreeRoot(const char *n);
extern symbol **getTreeRootByIndex(int index);
extern int setTreeRootByIndex(int index, symbol *pp);
extern symbol **selectVarTree(char *n);
extern varInfo *RcreateVar(const char *n, symbol **pp, int type);
extern varInfo *rfindVar(const char *n, symbol **pp);
extern void copyVar(const char *n, varInfo *v, symbol **root);

#endif

#endif
