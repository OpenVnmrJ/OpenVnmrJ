/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef DDLNODE_H
#define DDLNODE_H

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
using namespace std;

#include "ddlList.h"
//DeclareListClass(DDLNode);

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
  int symtype;
  class DDLNode *val;

  /* Member functions, operators */

  char *GetName();
  void SetName(char* newname);
  void SetValue(class DDLNode *v);
  DDLNode *GetValue();
  DataType GetType();
  int IsType(DataType d);
  virtual operator double();
  virtual operator char*();
  virtual DDLNode* operator =(DDLNode*);
  class ArrayData *GetArray();
  virtual DDLNode* Clone(bool dataflag=true) {(void) dataflag; return new DDLNode(this); };

  DDLNode* Index(int i);

  DDLNode* CloneList(bool dataflag=true);
  virtual void Delete();
  
  DDLNode();
  DDLNode(DDLNode *old);
  DDLNode(const char* name, int t = ARRAY_DATA);

  ~DDLNode();

  virtual ostream& operator>>(ostream& os);

 private:
  char *name;  /* A Node with a valid name is a symbol */
    static int nnodes;
};


class RealData : public DDLNode {

 public:
  double value;
  RealData(double value);
  RealData(RealData* old);
  virtual DDLNode* Clone(bool dataflag=true) {(void) dataflag; return new RealData(this); };
  operator double();
  ostream& operator >>(ostream& os);
};


class StringData : public DDLNode {

 public:
  
  void SetString(const char *s);
  char *GetString();
  StringData(const char* s);
  StringData(StringData* old);
  virtual DDLNode* Clone(bool dataflag=true) {(void) dataflag; return new StringData(this); };
  ~StringData();
  operator char*();
  ostream& operator >>(ostream& os);

 private:
  char *string;
};

class ArrayData : public DDLNode {

 public:

  void Close() {};
  ostream& operator>>(ostream& os);
  ArrayData *AppendElement(double value);
  ArrayData *AppendElement(char* value);
  ArrayData *AppendElement(ArrayData *value);
  bool GetValue(int& value, int index);
  bool GetValue(double& value, int index);
  bool GetValue(char*& value, int index);
  ArrayData *GetArray(int index);
  ArrayData();
  ArrayData(ArrayData* old);
  virtual DDLNode* Clone(bool dataflag=true) {(void) dataflag; return new ArrayData(); };
  void Delete();
  ~ArrayData();
};


class CodeData : public DDLNode {
 public:
  doublef ptr;
  CodeData(doublef f);
  ostream& operator >>(ostream& os);
  CodeData();
  CodeData(CodeData* old);
  virtual DDLNode* Clone(bool dataflag=true) {(void) dataflag; return new CodeData(); };
  ~CodeData() {};
};



class DDLNodeIterator {

public:
  DDLNodeIterator(DDLNodeList* r) : rtl(r) {
    if (rtl)
      next = rtl->First();
    else
      next = 0;
  }
  
  ~DDLNodeIterator() {};
  
  int NotEmpty() { return (next ? true : false) ; }
  int  IsEmpty() { return (next ? false : true) ; }
  
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

extern DDLNode *NullDDL;
extern ArrayData *NullArray;

#endif /* DDLNODE_H */
