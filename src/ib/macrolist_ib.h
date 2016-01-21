/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _MACRO_LIST_H
#define _MACRO_LIST_H


#ifdef LINUX
#include "generic.h"
#else
#include <generic.h>
#endif

#define List_CreateAppendMember(TYPE)                                        \
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

#define List_CreatePrependMember(TYPE)                                       \
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

#define List_CreateRemoveLinkMember(TYPE)                                    \
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

#define List_CreateDequeueMember(TYPE)                                       \
                                                                             \
TYPE* name2(TYPE,List)::Dequeue() {                                          \
  name2(TYPE,Link)* l = Remove(first);                                       \
  if (!l) return 0;                                                          \
  TYPE* o = l->item;                                                         \
  delete l;                                                                  \
  count--;                                                                   \
  return o;                                                                  \
}


#ifndef DEBUG
#define List_CreateRemoveItemMember(TYPE)                                    \
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

#define List_CreatePrintMember(TYPE)                                         \
                                                                             \
name2(TYPE,List)& name2(TYPE,List)::Print()  {                               \
  name2(TYPE,Link) *l=first;                                                 \
  while (l) {                                                                \
    l->Print();                                                              \
    l=l->next;                                                               \
  }                                                                          \
  return *this;                                                              \
}

#define List_Class(TYPE)                                                     \
                                                                             \
class name2(TYPE,List) {                                                     \
protected:                                                                   \
  name2(TYPE,Link) *first;                                                   \
  name2(TYPE,Link) *last;                                                    \
  int count;                                                                 \
public:                                                                      \
  name2(TYPE,List) () { first = last = 0; count = 0;}                        \
  virtual ~name2(TYPE,List)() {}                                             \
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
};


#define Link_Class(TYPE)                                                     \
                                                                             \
class name2(TYPE,Link) {                                                     \
  friend class name2(TYPE,List);                                             \
  protected:                                                                 \
    name2(TYPE,Link) *next;                                                  \
    name2(TYPE,Link) *prev;                                                  \
    TYPE* item;                                                              \
  public:                                                                    \
    name2(TYPE,Link) (TYPE* i)   { next = prev = 0 ; item = i; }             \
    name2(TYPE,Link)* Next() { return next; }                                \
    name2(TYPE,Link)* Prev() { return prev; }                                \
    TYPE* Item() { return item; }                                            \
    name2(TYPE,Link)& Print();                                               \
    virtual ~name2(TYPE,Link) ();                                            \
};

#define Create_ListClass(TYPE)               \
                                             \
List_CreateAppendMember(TYPE)                \
List_CreateRemoveLinkMember(TYPE)            \
List_CreateRemoveItemMember(TYPE)            \
List_CreateDequeueMember(TYPE)               \
List_CreatePrependMember(TYPE)               \
List_CreatePrintMember(TYPE) 
     
#define Declare_ListClass(TYPE)              \
                                             \
Link_Class(TYPE)                             \
List_Class(TYPE)


#endif _MACRO_LIST_H
