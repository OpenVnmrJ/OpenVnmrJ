/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

//#include <generic.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <iostream.h>
//#include <fstream.h>
//#include "ddllib.h"
//#include "ddl-tab.h"

#include <iostream>
using std::cout;

#include "ddlParser.h"
#include "ddlNode.h"
#include "ddlGrammar.h"

//#define debug name2(/,/)
//#define debug cout

DDLNode* ExprStatement(DDLNode *item1, DDLNode *item2)
{
  if (item2) {
    (*item2) >> cout << "\n";
  }
  return item1;
}

DDLNode *TypelessAsgn(DDLNode *var, DDLNode *expr)
{
  var->SetValue(expr);
  var->symtype = VAR;
  return var;
}


DDLNode *TypedAsgn(DDLNode *var, DDLNode *expr)
{
  var->SetValue(expr);
  var->symtype = VAR;
  return var;
}

DDLNode *RealExpr(double real)
{
   return  new RealData(real);
}


DDLNode *MinusRealExpr(double real)
{
  RealData *rd = new RealData(-real);
  return rd;
}

DDLNode *StructExpr()
{
  cout << "STRUCT !!" <<endl;
  return 0;
}

DDLNode *StructExprInit()
{
  cout << "STRUCT !!" <<endl;
  return 0;
}

DDLNode *StringExpr(char *s)
{
  StringData *sd = new StringData(s);
  free(s);
  return sd;
}

DDLNode *ArrayStatement(DDLNode *elements)
{
  //debug << "rule: '{' elements '}'" <<"\n";
  
  ((ArrayData*) elements)->Close();

  return elements;
}

DDLNode *ElementsDone()
{
  //debug << "rule: elements: empty"<<"\n";
  ArrayData *ad = new ArrayData();
  //debug << "array done" <<"\n";
  return ad;
}

DDLNode *ExprElements(DDLNode *expr)
{
  //debug << "push expr only" <<"\n";
  ArrayData *ad = new ArrayData();
  ad->Push(expr);
  return ad;
}

DDLNode *AppendElement(DDLNode *elements, DDLNode *expr)
{
  //debug <<"push expr again" << "\n";
  elements->Push(expr);
  return elements;
}

