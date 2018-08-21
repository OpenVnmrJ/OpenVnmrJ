/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "ddlList.h"
#include "ddlNode.h"

int DDLNodeLink::refs = 0;
int DDLNodeList::refs = 0;

int DDLNode::nnodes = 0;/*CMP*/







DDLNodeList& DDLNodeList::Append(DDLNode* s) {
    DDLNodeLink* l = new DDLNodeLink(s);
    if (last) {
        last->next = l;
        l->prev = last;
    } else
        first = l;
    last = l;
    count++;
    return *this;
} 


DDLNodeList& DDLNodeList::Prepend(DDLNode* s) {
    DDLNodeLink * l = new DDLNodeLink (s);
    if (first) {
        first->prev = l;
        l->next = first;
    } else {
        last = l;
    }
    first = l;
    count++;
    return *this;
}


DDLNodeLink* DDLNodeList::Remove(DDLNodeLink* l) {
    if (!l) return l;
    if (l == first) first = first->next;
    if (l == last)  last = last->prev;
    if (l->next) {
        l->next->prev = l->prev;
    }
    if (l->prev) {
        l->prev->next = l->next;
    }
    count--;
    return l;
} 


DDLNode* DDLNodeList::Dequeue() {
    if (!first) return 0;
    DDLNode* o = first->item;
    delete Remove(first);
    count--;
    return o;
}



DDLNode* DDLNodeList::Remove(DDLNode* s) {
    DDLNodeLink * l = first;
    while (l) {
        if (l->item == s) break;
        l = l->next;
    }
    if (l == 0) return 0;
    if (l == first) first = first->next;
    if (l == last)  last = last->prev;
    if (l->next) {
        l->next->prev = l->prev;
    }
    if (l->prev) {
        l->prev->next = l->next;
    }
    DDLNode* item = l->item;
    Remove(l);
    count--;
    return item;
} 


int   DDLNodeList::LengthOf() {
    DDLNodeLink* l = first;
    int i = 0;
    while (l) {
        l = l->next;
        i++;
    }
    return i;
}


DDLNode* DDLNodeList::operator[](int i) {
    DDLNodeLink* l = first;
    if (i < 0) i = 0;
    if (i >= count) i = count-1;
    while (l && i--) {
        l = l->next;
    }
    return (l ? l->item : (last ? last->item : 0));
}



DDLNodeList& DDLNodeList::Print()  {
    DDLNodeLink *l=first;
    while (l) {
        l->Print();
        l=l->next;
    }
    return *this;
}







DDLNode* DDLNode::CloneList(bool dataflag)
{
  DDLNodeIterator st(this);
  DDLNode* temp = Clone(dataflag);
  DDLNode* sp;

  while ( (sp = ++st) ) {
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
  
  while ( (dp = Dequeue()) ) {
//    cout << "DDLNode::Delete():  Dequeue(): ";
//    (*dp) >> cout << endl;
    if (dp->val) dp->val->Delete();
    delete dp;
  }

  if (val) val->Delete();

  delete this;
}

void ArrayData::Delete()
{
    DDLNode* dp;
  
    while ( (dp = Dequeue()) ) {
        if (dp->val) dp->val->Delete();
        dp->Delete();
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
    name = (char *)malloc(strlen(newname)+1);
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
    //fprintf(stderr,"new DDLNode(): n=%d\n", ++nnodes);
  datatype = VOID_DATA;
  val = 0;
  name = 0;
  symtype = 0;
}

DDLNode::DDLNode(const char *s, int t)
{
    //fprintf(stderr,"new DDLNode(%s,t): n=%d\n", s, ++nnodes);
  name = (char *)malloc(strlen(s)+1);
  strcpy(name, s);
  symtype = t;
  datatype = VOID_DATA;
  val = 0;
};

DDLNode::DDLNode(DDLNode *src)
{
    //fprintf(stderr,"new DDLNode(src): n=%d\n", ++nnodes);
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
    //fprintf(stderr,"~DDLNode(): n=%d\n", --nnodes);
    delete name;
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



StringData::StringData(const char* s)
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

void StringData::SetString(const char* newstring)
{
  if (string) free(string);
  string = (char *)malloc(strlen(newstring)+1);
  strcpy(string, newstring==NULL ? "" : newstring);
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
  if (ptr) return os << ptr << "()";
  else return os;
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

bool
ArrayData::GetValue(int& value, int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return false;
  } else { 
    value = *(int *)q;
    return true;
  }
}

bool
ArrayData::GetValue(double& value, int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return false;
  } else { 
    value = (double) *(Index(idx));
    return true;
  }
}

bool
ArrayData::GetValue(char*& value, int idx)
{
  DDLNode *q = Index(idx);
  if (q == 0) {
    return false;
  } else { 
    value = (char*) *(Index(idx));
    return true;
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
