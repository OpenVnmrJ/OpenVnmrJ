/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPRQNODE_H
#define AIPRQNODE_H

#include <string>
#include <fstream>
#include <list>
#include <map>
using namespace std;

#include "aipDataInfo.h"

#define MAXSTR  256

class RQnode;
typedef std::map<std::string, std::string> AttrMap;
typedef std::list<RQnode> ChildList;

class RQnode
{

public:

   RQnode();
   virtual ~RQnode();

   string getAttribute(string name);
   string getPath();
   string getKey();
   void setAttribute(string name, string value);
   void setAttribute(string name, int value);
   void setAttribute(string name, float value);
   void setAttribute(string name, double value);
   RQnode *getParent() {return parent;}
   void setParent(RQnode *p) {parent = p;}

   void addChild(RQnode &);
   void addChild(int index, RQnode &);
   void addChild(string nodeid, RQnode &);
   RQnode *getChild(int);
   RQnode *getChild(string nid);
   RQnode *getChildByKey(string key);
   int getChildrenCount();

   void remove(string key);

   void readNode(string attributes);
   void writeNode(ofstream &fout, string indent=" ");

   void setIndent(int);
   void showAttributes();

   AttrMap *getAttributeMap() {return &attributeMap;}
   ChildList *getChildren() {return &children;}

private:

   ChildList children;
   RQnode *parent;
   AttrMap attributeMap;

protected:

   map<int, RQnode *> indexMap;
   map<string, RQnode *>  dimMap;
   list<RQnode *> imageList;
   spDataInfo_t m_dataInfo;
   bool doSort;

};

#endif /* AIPRQNODE_H */
