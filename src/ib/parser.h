/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DDL_PARSER_H
#define DDL_PARSER_H

#include <string.h>


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

void execerror(char*, char* token = 0);
char *emalloc(unsigned int n);
void yyerror(const char *s);
int yylex();
int yyparse();
double Pow(double x, double y);


typedef double (*doublef)(double);


class DDLNode;

void CheckDelete(DDLNode*);

DDLNode* ExprStatement(DDLNode *item1, DDLNode *item2);

DDLNode *TypelessAsgn(DDLNode *var, DDLNode *expr);
DDLNode *TypedAsgn(DDLNode *var, DDLNode *expr);

DDLNode *RealExpr(double real);
DDLNode *MinusRealExpr(double real);
DDLNode *StructExpr();
DDLNode *StructExprInit();
DDLNode *StringExpr(char *s);
DDLNode *ArrayStatement(DDLNode *elements);
DDLNode *ElementsDone();
DDLNode *ExprElements(DDLNode *expr);
DDLNode *AppendElement(DDLNode *elements, DDLNode *expr);


#endif
