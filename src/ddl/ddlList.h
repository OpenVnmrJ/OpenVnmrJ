/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef DDLLIST_H
#define DDLLIST_H

class DDLNode;
class DDLNodeList;
class DDLNodeLink;

class DDLNodeLink {
    friend class DDLNodeList;
protected:
    DDLNodeLink *next;
    DDLNodeLink *prev;
    DDLNode* item;
    static int refs;
public:
    DDLNodeLink (DDLNode* i) {
        next = prev = 0 ;
        item = i;
        refs++;
    }
    DDLNodeLink* Next() {
        return next;
    }
    DDLNodeLink* Prev() {
        return prev;
    }
    DDLNode* Item() {
        return item;
    }
    DDLNodeLink& Print();
    virtual ~DDLNodeLink () {
        refs--;
    };
    static int Init() {
        return refs = 0;
    };
};


class DDLNodeList {
protected:
    DDLNodeLink *first;
    DDLNodeLink *last;
    int count;
    int index;
    static int refs;
public:
    DDLNodeList () { first = last = 0; count = index = 0; refs++;}
    virtual ~DDLNodeList() {
        refs--;
    };
    static int Init() {
        return refs = 0;
    };
    DDLNodeLink *Last() {
        return last;
    }
    DDLNodeLink *First() {
        return first;
    }
    DDLNodeList& Append(DDLNode*);
    DDLNodeList& Prepend(DDLNode*);
    DDLNodeLink* Remove(DDLNodeLink*);
    DDLNodeList& Push(DDLNode* s) {
        return Append(s);
    }
    DDLNodeList& Print();
    DDLNode* Remove(DDLNode*);
    DDLNode* Dequeue();
    DDLNode* Pop() {
        DDLNode* item = last ? last->item : 0 ;
        DDLNodeLink* l = Remove(last);
        delete l;
        return item; }
    DDLNode* Top() {
        return last->item;
    }
    int Count() {
        return count;
    }
    int Index() {
        return index;
    }
    virtual DDLNode* operator[](int i);
    int LengthOf();
    DDLNode* operator++();
};

#endif /* DDLLIST_H */
