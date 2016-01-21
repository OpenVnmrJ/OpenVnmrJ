/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef DDLPARSER_H
#define DDLPARSER_H

//#include <string.h>

char *emalloc(unsigned int n);
void ddl_yyerror(char *s);
int ddl_yylex();
int ddl_yyparse();
double Pow(double x, double y);

//typedef double (*doublef)(double);

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

#endif /* DDLPARSER_H */
