/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

%{
    /*
#include <generic.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>
    */
#include "ddlParser.h"
    /*extern void free(void *);*/
%}

%union {
  double val;
  char *str;
  DDLNode *sym;
}

%token <val>  DDL_REAL
%token <str>  DDL_STRING
%token <sym>  VAR BUILTIN UNDEFINED CODE KEYWORD STRUCT
%token <sym>  DDL_FLOAT DDL_INT DDL_CHAR
%token <sym>  VOID TYPEDEF
%type  <sym>  sexpr expr sasgn asgn sarray array selements elements statement decl

%right '='
%left  OR
%left  AND
%left  GT GE LT LE EQ NE
%left  '+' '-'
%left  '*' '/'
%left  UNARYMINUS UNARYPLUS NOT
%right  '^'
%%

statement:                 { $$ = 0; }
  | statement ';'          { $$ = $1;}
  | statement sasgn ';'    { $$ = $1;}
  | statement asgn ';'     { $$ = $1;}
  | statement decl ';'     { $$ = $1;}
  | statement expr ';'     { $$ = ExprStatement($1, $2);}
  | statement error ';'    { yyerrok; $$ = $1;}
  ;

sasgn: DDL_CHAR '*' VAR arrayspec '=' sexpr {$$ = TypedAsgn($3, $6);}
  ;

asgn:  VAR arrayspec '=' expr         {$$ = TypelessAsgn($1, $4);}
  |   typename VAR arrayspec '=' expr {$$ = TypedAsgn($2, $5);}
  ;

decl:  typename VAR arrayspec           { $$ = $2; }
  ;

typename: DDL_FLOAT | DDL_INT | VAR;

arrayspec:  '[' ']' | ;

expr:  DDL_REAL  {$$ = RealExpr($1);}
  | '-' DDL_REAL  %prec UNARYMINUS                    {$$ = RealExpr(-$2);}
  | '+' DDL_REAL  %prec UNARYPLUS                     {$$ = RealExpr($2);}
  | VAR                                           {$$ = $1;}
  | asgn                                          {$$ = $1;}
  | array                                         {$$ = $1;}
  | STRUCT '{' statement '}' VAR arrayspec           {$$ = StructExpr();}
  | STRUCT '{' statement '}' VAR arrayspec '=' array {$$ = StructExprInit();}
  ;

sexpr: sarray                                         {$$ = $1;}
  | DDL_STRING                                  {$$ = StringExpr($1);} 
  ;

array: '{' elements '}'  {$$ = ArrayStatement($2);}
  ;

sarray: '{' selements '}'  {$$ = ArrayStatement($2);}
  ;

elements:   {$$ = ElementsDone();}
  | expr    {$$ = ExprElements($1);}
  | elements ',' expr  {$$ = AppendElement($1, $3);}
  ;

selements:   {$$ = ElementsDone();}
  | sexpr    {$$ = ExprElements($1);}
  | selements ',' sexpr  {$$ = AppendElement($1, $3);}
  ;

%%
  
	 
