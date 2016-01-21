/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef DDLMACROS_H
#define DDLMACROS_H

#include <generic.h>

#define ListCreateAppendMember(TYPE)                                         \
                                                                             \
name2(TYPE,List)& name2(TYPE,List)::Append(TYPE* s) {                        \
  name2(TYPE,Link)* l = new name2(TYPE,Link)(s);                             \
  if (last) {                                                                \
    last->next = l;                                                          \
    l->prev = last;                                                          \
  } else                                                                     \
    first = l;                                                               \
  last = l;                                                                  \
  count++;                                                                   \
  return *this;                                                              \
} 

#define ListCreatePrependMember(TYPE)                                        \
                                                                             \
name2(TYPE,List)& name2(TYPE,List)::Prepend(TYPE* s) {                       \
  name2(TYPE,Link) * l = new name2(TYPE,Link) (s);                           \
  if (first) {                                                               \
    first->prev = l;                                                         \
    l->next = first;                                                         \
  } else {                                                                   \
    last = l;                                                                \
  }                                                                          \
  first = l;                                                                 \
  count++;                                                                   \
  return *this;                                                              \
}

#define ListCreateRemoveLinkMember(TYPE)                                     \
                                                                             \
name2(TYPE,Link)* name2(TYPE,List)::Remove(name2(TYPE,Link)* l) {            \
  if (!l) return l;                                                          \
  if (l == first) first = first->next;                                       \
  if (l == last)  last = last->prev;                                         \
  if (l->next) {                                                             \
    l->next->prev = l->prev;                                                 \
  }                                                                          \
  if (l->prev) {                                                             \
    l->prev->next = l->next;                                                 \
  }                                                                          \
  count--;                                                                   \
  return l;                                                                  \
} 

#define ListCreateDequeueMember(TYPE)                                        \
                                                                             \
TYPE* name2(TYPE,List)::Dequeue() {                                          \
  name2(TYPE,Link)* l = Remove(first);                                       \
  if (!l) return 0;                                                          \
  TYPE* o = l->item;                                                         \
  delete l;  /* Is this right? - CMP */                                      \
  count--;                                                                   \
  return o;                                                                  \
}


#ifndef DEBUG
#define ListCreateRemoveItemMember(TYPE)                                     \
                                                                             \
TYPE* name2(TYPE,List)::Remove(TYPE* s) {                                    \
  name2(TYPE,Link) * l = first;                                              \
  while (l) {                                                                \
    if (l->item == s) break;                                                 \
    l = l->next;                                                             \
  }                                                                          \
  if (l == 0) return 0;                                                      \
  if (l == first) first = first->next;                                       \
  if (l == last)  last = last->prev;                                         \
  if (l->next) {                                                             \
    l->next->prev = l->prev;                                                 \
  }                                                                          \
  if (l->prev) {                                                             \
    l->prev->next = l->next;                                                 \
  }                                                                          \
  TYPE* item = l->item;                                                      \
  delete l;                                                                  \
  count--;                                                                   \
  return item;                                                               \
} 
#endif

#define ListLengthOfMember(TYPE)                                             \
                                                                             \
int   name2(TYPE,List)::LengthOf() {                                         \
    name2(TYPE,Link)* l = first;                                             \
    int i = 0;                                                               \
    while (l) {                                                              \
      l = l->next;                                                           \
      i++;                                                                   \
    }                                                                        \
    return i;                                                                \
  }                                                                          \

#define ListArrayAccessMember(TYPE) \
\
TYPE* name2(TYPE,List)::operator[](int i) { \
    name2(TYPE,Link)* l = first;                                             \
    if (i < 0) i = 0;                                                        \
    if (i >= count) i = count-1;                                             \
    while (l && i--) {                                                       \
      l = l->next;                                                           \
    }                                                                        \
    return (l ? l->item : (last ? last->item : 0));                          \
  }                                                                          \


#define ListCreatePrintMember(TYPE)                                          \
                                                                             \
name2(TYPE,List)& name2(TYPE,List)::Print()  {                               \
  name2(TYPE,Link) *l=first;                                                 \
  while (l) {                                                                \
    l->Print();                                                              \
    l=l->next;                                                               \
  }                                                                          \
  return *this;                                                              \
}

#define ListClass(TYPE)                                                      \
                                                                             \
class name2(TYPE,List) {                                                     \
protected:                                                                   \
  name2(TYPE,Link) *first;                                                   \
  name2(TYPE,Link) *last;                                                    \
  int count;                                                                 \
  int index;                                                                 \
  static int refs;                                                           \
public:                                                                      \
  name2(TYPE,List) () { first = last = 0; count = index = 0; refs++;}        \
  virtual ~name2(TYPE,List)() {refs--;};                                     \
  static int Init() { return refs = 0; };                                        \
  name2(TYPE,Link) *Last() { return last; }                                  \
  name2(TYPE,Link) *First() {return first; }                                 \
  name2(TYPE,List)& Append(TYPE*);                                           \
  name2(TYPE,List)& Prepend(TYPE*);                                          \
  name2(TYPE,Link)* Remove(name2(TYPE,Link)*);                               \
  name2(TYPE,List)& Push(TYPE* s) { return Append(s); }                      \
  name2(TYPE,List)& Print();                                                 \
  TYPE* Remove(TYPE*);                                                       \
  TYPE* Dequeue();                                                           \
  TYPE* Pop() {                                                              \
    TYPE* item = last ? last->item : 0 ;                                     \
    name2(TYPE,Link)* l = Remove(last);                                      \
    delete l;                                                                \
    return item; }                                                           \
  TYPE* Top() { return last->item; }                                         \
  int Count() { return count; }                                              \
  int Index() { return index; }                                              \
  virtual TYPE* operator[](int i);                                           \
  int LengthOf();                                                            \
  TYPE* operator++();                                                        \
};


#define LinkClass(TYPE)                                                      \
                                                                             \
class TYPE;                                                                  \
class name2(TYPE,Link) {                                                     \
  friend class name2(TYPE,List);                                             \
  protected:                                                                 \
    name2(TYPE,Link) *next;                                                  \
    name2(TYPE,Link) *prev;                                                  \
    TYPE* item;                                                              \
    static int refs;                                                         \
  public:                                                                    \
    name2(TYPE,Link) (TYPE* i)   { next = prev = 0 ; item = i; refs++;}      \
    name2(TYPE,Link)* Next() { return next; }                                \
    name2(TYPE,Link)* Prev() { return prev; }                                \
    TYPE* Item() { return item; }                                            \
    name2(TYPE,Link)& Print();                                               \
    virtual ~name2(TYPE,Link) () {refs--;};                                   \
    static int Init() { return refs = 0; };                                      \
};

#define DeclareListClass(TYPE)                              \
                                                            \
LinkClass(TYPE)                                             \
ListClass(TYPE)                                             \
static int name2(TYPE,Linkrefs) = name2(TYPE,Link)::Init();   \
static int name2(TYPE,Listrefs) = name2(TYPE,List)::Init();   \


#define DefineListClass(TYPE)               \
                                            \
ListCreateAppendMember(TYPE)                \
ListCreateRemoveLinkMember(TYPE)            \
ListCreateRemoveItemMember(TYPE)            \
ListCreateDequeueMember(TYPE)               \
ListCreatePrependMember(TYPE)               \
ListLengthOfMember(TYPE)                    \
ListArrayAccessMember(TYPE)                 \
ListCreatePrintMember(TYPE)                 \

#endif /* DDLMACROS_H */
