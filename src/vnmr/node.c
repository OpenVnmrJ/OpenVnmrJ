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
|       node.c
|
|       These prcedures and functions are used to create and maintain code
|       trees.
|
+-----------------------------------------------------------------------------*/

#include "vnmrsys.h"
#include "node.h"
#ifdef UNIX
#include "magic.gram.h"
#else 
#include "magic_gram.h"
#endif 
#include "stack.h"
#include "allocate.h"
#include <stdio.h>
#include <stdlib.h>

extern char *tempID;
extern int   Dflag;
extern void  space();

void showTree(int n, char *m, node *p);

/*-----------------------------------------------------------------------------
|
|       newNode/2
|
|       This function is used to create a new code tree node (of a given
|       flavour).
|
+----------------------------------------------------------------------------*/

node *newNode(int flavour, fileInfo *location)
{
   node *p;

   if ((p=(node *)allocateWithId(sizeof(node),tempID)) == NULL)
   {  fprintf(stderr,"out of memory!\n");
      exit(1);
   }
   p->flavour         = flavour;
   p->location.line   = location->line;
   p->location.column = location->column;
   p->location.file   = NULL;
   p->v.s             = NULL;
   p->Lson            = NULL;
   p->Rbro            = NULL;
   return(p);
}

/*------------------------------------------------------------------------------
|
|       dispose/1
|
|       This procedure is used to dispose a given code tree.  Disposal is
|       done recursivly for each bother and son.  Certain nodes have attached
|       character strings, these are released.
|
+-----------------------------------------------------------------------------*/

void dispose(node *p)
{  node *q;
   node *qsav;

   if (p)
   {  q = p->Lson;
      while (q)
      {
         qsav = q->Rbro;
         dispose(q);
         q = qsav;
      }
      p->Lson = NULL;
      switch (p->flavour)
      { case _NAME:
        case STRING: if (p->v.s)
                     {   release(p->v.s);
                         p->v.s = NULL;
                     }
        default:     break;
      }
      if (p->location.file)
          release(p->location.file);
      release((char *) p);
   }
}

/*------------------------------------------------------------------------------
|
|       addLeftSon/2
|
|       This procedure is used to insert a node (tree) as the new left son
|       of a given node (tree) the previous left son becomes the right brother
|       of this new left son.
|
+-----------------------------------------------------------------------------*/

void addLeftSon(node *p, node *q)
{   q->Rbro = p->Lson;
    p->Lson = q;
}

/*------------------------------------------------------------------------------
|
|       showFlavour/2
|
|       This procedure is used to output the flavour of a single node.
|
+-----------------------------------------------------------------------------*/

void showFlavour(FILE *f, node *p)
{   switch(p->flavour)
    { case AND:     fprintf(f,"AND");
                    break;
      case BOMB:    fprintf(f,"BOMB");
                    break;
      case BREAK:   fprintf(f,"BREAK");
                    break;
      case _Car:    fprintf(f,"Car (call with args and returns)");
                    break;
      case _Ca_:    fprintf(f,"Ca_ (call with args)");
                    break;
      case _C_r:    fprintf(f,"C_r (call with returns)");
                    break;
      case _C__:    fprintf(f,"C__ (call)");
                    break;
      case CM:      fprintf(f,"CM");
                    break;
      case CMLIST:  fprintf(f,"CMLIST");
                    break;
      case DIV:     fprintf(f,"DIV");
                    break;
      case DOLLAR:  fprintf(f,"DOLLAR");
                    break;
      case DONT:    fprintf(f,"DONT");
                    break;
      case ELSE:    fprintf(f,"ELSE");
                    break;
      case ENDFILE: fprintf(f,"ENDFILE");
                    break;
      case EOL:     fprintf(f,"EOL");
                    break;
      case EQ:      fprintf(f,"EQ");
                    break;
      case EXIT:    fprintf(f,"EXIT");
                    break;
      case GE:      fprintf(f,"GE");
                    break;
      case GT:      fprintf(f,"GT");
                    break;
      case ID:      fprintf(f,"ID");
                    break;
      case IGNORE:  fprintf(f,"IGNORE");
                    break;
      case LB:      fprintf(f,"LB");
                    break;
      case LC:      fprintf(f,"LC");
                    break;
      case LE:      fprintf(f,"LE");
                    break;
      case LT:      fprintf(f,"LT");
                    break;
      case MINUS:   fprintf(f,"MINUS");
                    break;
      case MOD:     fprintf(f,"MOD");
                    break;
      case MULT:    fprintf(f,"MULT");
                    break;
      case _NAME:   if (p->v.s)
                        fprintf(f,"_NAME (=%s)",p->v.s);
                    else
                        fprintf(f,"_NAME");
                    break;
      case NE:      fprintf(f,"NE");
                    break;
      case _NEG:    fprintf(f,"NEG");
                    break;
      case NOT:     fprintf(f,"NOT");
                    break;
      case PLUS:    fprintf(f,"PLUS");
                    break;
      case OR:      fprintf(f,"OR");
                    break;
      case REAL:    fprintf(f,"REAL (=%.13g)",p->v.r);
                    break;
      case SHOW:    fprintf(f,"SHOW");
                    break;
      case SQRT:    fprintf(f,"SQRT");
      		    break;
      case STRING:  if (p->v.s)
                        fprintf(f,"STRING (=\"%s\")",p->v.s);
                    else
                        fprintf(f,"STRING (no value)");
                    break;
      case THEN:    fprintf(f,"THEN");
                    break;
      case UNTIL:   fprintf(f,"UNTIL");
                    break;
      case WHILE:   fprintf(f,"WHILE");
                    break;
      default:      fprintf(f,"unknown (=%d)",p->flavour);
                    break;
    }
}

void showLocation(FILE *f, node *p)
{  if (p->location.file)
      fprintf(f," (at line %d, column %d in file %s)"
               ,p->location.line
               ,p->location.column
               ,p->location.file
             );
}

/*------------------------------------------------------------------------------
|
|       showTree/3
|
|       This procedure is used to output a code tree in a nice readable form
|       reflecting its tree structure.
|
+-----------------------------------------------------------------------------*/

void showTree(int n, char *m, node *p)
{   node *q;

    if (p)
    {   if (m)
	  fprintf(stderr,"%s",m);
	else
	  fprintf(stderr,"(null)");
	space(stderr,n);
	showFlavour(stderr,p);
	showLocation(stderr,p);
	fprintf(stderr,"\n");
        q = p->Lson;
        while (q)
        {   showTree(n+2,m,q);
            q = q->Rbro;
        }
    }
}

/*------------------------------------------------------------------------------
|
|       listLength/1
|
|       This function examines a tree and calculates the length of the
|       represented list.  A list is a number of sub-trees joined by CM's.
|
+-----------------------------------------------------------------------------*/

int listLength(node *n)
{   int L;

    if (n)
    {   L = 1;
        while (n->flavour == CM)
        {   n  = n->Lson;
            L += 1;
        }
        return(L);
    }
    else
        return(0);
}
