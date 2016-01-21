/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
static char *sccsID(){
    return "@(#)ddlnode.c 18.1 03/21/08 (c)1991 SISCO";
}
/*
*/

#include "ddlnode2.h"


DDLNode* DDLNode::CloneList(int dataflag)
{
  DDLNodeIterator st(this);
  DDLNode* temp = Clone(dataflag);
  DDLNode* sp;

  while (sp = ++st) {
    // cout << "DDLNodeClone: ";
    // *sp >> cout << endl;
    temp->Push(sp->CloneList(dataflag));
  }
  if (val) temp->val = val->CloneList(dataflag);
  return temp;
}


void DDLNode::Delete()
{
  DDLNode* dp;
  
  while (dp = Dequeue()) {
//    cout << "DDLNode::Delete():  Dequeue(): ";
//    (*dp) >> cout << endl;
    if (dp->val) dp->val->Delete();
  }

  if (val) val->Delete();

  delete this;
}

char* DDLNode::GetName()
{
  return name;
}

void DDLNode::SetName(char* newname)
{
  if (name) free(name);
  if (newname) {
    name = emalloc(strlen(newname)+1);
    strcpy(name, newname);
  } else {
    name = NULL;
  }
}

void DDLNode::SetValue(class DDLNode *v)
{
  val = v;
}

DDLNode* DDLNode::GetValue()
{
  return val;
}

DataType DDLNode::GetType()
{
  return datatype;
}

int DDLNode::IsType(DataType d)
{
  if (this) {
    return (datatype == d ? 1 : 0);
  } else {
    return 0;
  }
}

DDLNode::operator double()
{
  if (val) {
    return (double) *val;
  } else {
    return 0;
  }
}

DDLNode::operator char*()
{
  if (val) {
    return (char*) *val;
  } else {
    return 0;
  }
}


DDLNode* DDLNode::operator =(DDLNode* a)
{
  return a->val;
}

DDLNode::DDLNode()
{
  // cout << "DDLNode() invoked!" << endl;
  datatype = VOID_DATA;
  val = 0;
  name = 0;
  symtype = 0;
}

DDLNode::DDLNode(char *s, int t)
{
  name = emalloc(strlen(s)+1);
  strcpy(name, s);
  symtype = t;
  datatype = VOID_DATA;
  val = 0;
};

DDLNode::DDLNode(DDLNode *src)
{
//  if (src->static_node) return this;
  
  // cout << "DDLNode(DDLNode *src) invoked!" << endl;
  datatype = src->datatype;
  symtype = src->symtype;
  
  if (src->val) {
    val = src->val->Clone();
  } else {
    val = 0;
  }
  name = NULL;
  SetName(src->GetName());
}

DDLNode::~DDLNode()
{
  if (name) delete name;
}



ArrayData* DDLNode::GetArray()
{
    return (ArrayData*) val;
}

DDLNode* DDLNode::Index(int i)
{
  return (*this)[i];
}

ostream& DDLNode::operator>>(ostream& os)
{
  if (val) return (*val)>>os ;
  else return os;
};


RealData::RealData(double _value)
{
  // cout << "ReadData(double value) invoked!" << endl;
  value = _value;
  datatype = REAL_DATA;
}

RealData::RealData(RealData* old) : DDLNode(old)
{
  // cout << "RealData(RealData *old) invoked!" << endl;
  value = old->value;
  datatype = REAL_DATA;
}

RealData::operator double()
{
  return value;
}



StringData::StringData(char* s)
{
  string = 0;
  SetString(s);
  datatype = STRING_DATA;
}

StringData::StringData(StringData *old) : DDLNode(old)
{
  string = 0;
  SetString(old->GetString());
  datatype = STRING_DATA;
}

StringData::~StringData()
{
  if (string) delete string;
}

StringData::operator char*()
{
  return string;
}

char* StringData::GetString()
{
  return string;
}

void StringData::SetString(char* newstring)
{
  if (string) free(string);
  if (newstring == NULL) {
    newstring = "";
  }
  string = emalloc(strlen(newstring)+1);
  strcpy(string, newstring);
}


ArrayData* ArrayData::AppendElement(double value)
{
  RealData *rd = new RealData(value);
  Push(rd);
  return this;
}

ArrayData*  ArrayData::AppendElement(char* value)
{
  StringData *sd = new StringData(value);
  Push(sd);
  return this;
}

ArrayData* ArrayData::AppendElement(ArrayData *value)
{
  Push(value);
  return this;
}

ArrayData::ArrayData()
{
  val = 0;
  datatype = ARRAY_DATA;
}

ArrayData::~ArrayData() {}

CodeData::CodeData(doublef f)
{
  ptr = f;
  datatype = CODE_DATA;
}

CodeData::CodeData() {}

CodeData::CodeData(CodeData* old) : DDLNode(old)
{
  ptr = old->ptr;
  datatype = CODE_DATA;
}

     

ostream& RealData::operator>>(ostream& os)
{
  if (value == (unsigned int) value) {
    return os << (unsigned int) value ;
  } else {
    return os << value ;
  }
};

ostream& StringData::operator>>(ostream& os)
{
  if (string) return os << '"' << string << '"';
  else return os;
};

ostream& CodeData::operator>>(ostream& os)
{
    /*if (ptr) return os << ptr << "()";
      else*/ return os;
};


ostream& ArrayData::operator>>(ostream& os)
{
  os << " { ";
  DDLNodeIterator node(this);
  DDLNode *d = ++node;
  while (d) {
    *d >> os;
    d = ++node;
    if (d) os << ", ";
  }
  return os << " } ";
}

int ArrayData::GetValue(int& value, int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return FALSE;
  } else { 
    value = *(int *)q;
    return TRUE;
  }
}

int ArrayData::GetValue(double& value, int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return FALSE;
  } else { 
    value = (double) *(Index(idx));
    return TRUE;
  }
}

int ArrayData::GetValue(char*& value, int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return FALSE;
  } else { 
    value = (char*) *(Index(idx));
    return TRUE;
  }
}

ArrayData* ArrayData::GetArray(int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return NullArray;
  } else {
    if (q->datatype == ARRAY_DATA) {
      return (ArrayData*) q;
    } else {
      return NullArray;
    }
  }
}
