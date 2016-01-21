/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifdef HEADER_ID


#else (not) HEADER_ID

#ifndef TREE_H_DEFINED
#define TREE_H_DEFINED

/* the data structure used to hold nodes in the storage area */

typedef struct _node {
   void         *key;
   void         *data;
   struct _node *left;
   struct _node *right;
   unsigned      bal : 3;
} NODE;

/* these are the allowed values for the "bal" field in a node */
#define L  0   /* sub-tree is unbalanced to the left */
#define B  1   /* sub-tree is balanced */
#define R  2   /* sub-tree is unbalanced to the right */

/* the data structure for storing tree nodes on a stack, used by functions
   "Storage_first()" and "Storage_next()" */

typedef struct {
   NODE *p_node;
   short relation;
} STK_ENTRY;

/* these are the allowed values for the "relation" field in a stack entry */
#define ROOT  0
#define LEFT  1
#define RIGHT 2

typedef struct {
   STK_ENTRY *curr;
   unsigned   size;
   STK_ENTRY  ents[1];
} STACK;

/* the data structure for a storage area header */

typedef struct {
   NODE    *root;
#  ifdef __STDC__
   int    (*compare)(const char *, const char *);
   int    (*delete)(void *, void *);
#  else
   int    (*compare)();
   int    (*delete)();
#  endif
   unsigned count;
   Stack    stack;
} STORAGE;

#endif (not) TREE_H_DEFINED

#endif HEADER_ID
