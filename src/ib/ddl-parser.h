/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* @(#)ddl-parser.h 18.1 03/21/08 (c)1991 SISCO */

#ifndef DDL_PARSER_H
#define DDL_PARSER_H

#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include "macrolist.h"

DeclareListClass(DDLNode);

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

extern ofstream debug;

typedef double (*doublef)(double);


typedef enum {
  VOID_DATA,
  REAL_DATA,
  STRING_DATA,
  CODE_DATA,
  ARRAY_DATA
} DataType;

class DDLNode : public DDLNodeList {
 public:
  DataType datatype;
  char *name;  /* A Node with a valid name is a symbol */
  int symtype;
  class DDLNode *val;

  /* Member functions, operators */

  void SetValue(class DDLNode *v)
    {val = v;};

  DDLNode *GetValue()
    {return val;}

  DataType GetType()
    { return datatype;}

  int IsType(DataType d)
    { if (this) return datatype == d ? 1 : 0 ; else return 0;};

  virtual operator double()
    { if (val) return (double) *val; else return 0;}

  virtual operator char*()
    { if (val) return (char*) *val; else return 0;}

  virtual DDLNode* operator =()
    { return val;}

  class ArrayData *GetArray();

  DDLNode* Index(int i);

  DDLNode()
    { datatype = VOID_DATA; val = 0; name = 0;};
  DDLNode(char* name, int t = ARRAY_DATA);
  ~DDLNode()
    {if (name) free(name);};

  virtual ostream& operator>>(ostream& os);
};


class RealData : public DDLNode {
 public:
  double val;
  RealData(double value)
    { val = value; datatype = REAL_DATA; };
  operator double()
    { return val;}
  ostream& operator >>(ostream& os);
};

class ArrayData : public DDLNode {
 public:
  void Close() {};
  ostream& operator>>(ostream& os);
  ArrayData()
    {val = 0; datatype = ARRAY_DATA;};
  ~ArrayData() {};
};

class StringData : public DDLNode {
 public:
  char *val;
  StringData(char* s)
    {val = emalloc(strlen(s)+1); strcpy(val, s); datatype = STRING_DATA;};
  ~StringData()
    {if (val) free(val);};
  operator char*()
    { return val;}
  ostream& operator >>(ostream& os);
};
     
class CodeData : public DDLNode {
 public:
  doublef ptr;
  CodeData(doublef f)
    { ptr = f; datatype = CODE_DATA;}
  ostream& operator >>(ostream& os);
};



DDLNode *lookup(char*);
DDLNode *install(char*, int);
DDLNode *install(char*, int, double);
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


class DDLNodeIterator {
public:
  DDLNodeIterator(DDLNodeList* r) : rtl(r) {
    if (rtl)
      next = rtl->First();
    else
      next = 0;
  }
  
  ~DDLNodeIterator() {};
  
  int NotEmpty() { return (next ? TRUE : FALSE) ; }
  int  IsEmpty() { return (next ? FALSE : TRUE) ; }
  
  DDLNodeIterator& GotoLast()  {
    if (rtl)
      next = rtl->Last();
    return *this;
  }
  
  DDLNodeIterator& GotoFirst() {
    if (rtl)
      next = rtl->First();
    return *this;
  }
  
  DDLNode *operator++() {
    DDLNodeLink* curr;
    curr = next;
    if (next)
      next = next->Next();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
  DDLNode *operator--() {
    DDLNodeLink* curr;
    curr = next;
    if (next)
      next = next->Prev();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
private:
  
  DDLNodeLink* next;
  DDLNodeList* rtl;
  
};

#endif
