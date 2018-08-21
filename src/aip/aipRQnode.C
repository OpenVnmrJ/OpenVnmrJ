/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

using namespace std;
#include "aipRQnode.h"

RQnode::RQnode()
{
   parent = (RQnode *)NULL;
}

RQnode::~RQnode()
{
}

string 
RQnode::getAttribute(string name) 
{
    AttrMap::iterator itr = attributeMap.find(name);
    if(itr != attributeMap.end()) return itr->second;
    else return string("");
}

string
RQnode::getPath()
{
   return getAttribute("dir")+"/"+getAttribute("name")
         +getAttribute("ext");
}

string
RQnode::getKey()
{
// first char of copy is a space.
   return getAttribute("dir")+" "+getAttribute("name")
         +getAttribute("ext")+getAttribute("copy");
}

void 
RQnode::setAttribute(string name, string value) 
{
    AttrMap::iterator itr = attributeMap.find(name);
    if(itr != attributeMap.end()) attributeMap.erase(itr);
    attributeMap.insert(AttrMap::value_type(name, value));
}

void 
RQnode::setAttribute(string name, int value) 
{
    char str[MAXSTR];
    sprintf(str, "%d", value);
    AttrMap::iterator itr = attributeMap.find(name);
    if(itr != attributeMap.end()) attributeMap.erase(itr);
    attributeMap.insert(AttrMap::value_type(name, string(str)));
}

void 
RQnode::setAttribute(string name, float value) 
{
    char str[MAXSTR];
    sprintf(str, "%f", value);
    AttrMap::iterator itr = attributeMap.find(name);
    if(itr != attributeMap.end()) attributeMap.erase(itr);
    attributeMap.insert(AttrMap::value_type(name, string(str)));
}

void 
RQnode::setAttribute(string name, double value) 
{
    char str[MAXSTR];
    sprintf(str, "%f", value);
    AttrMap::iterator itr = attributeMap.find(name);
    if(itr != attributeMap.end()) attributeMap.erase(itr);
    attributeMap.insert(AttrMap::value_type(name, string(str)));
}

void
RQnode::addChild(RQnode &child)
{
    child.setParent((RQnode *)this);
    children.push_back(child);
}

void
RQnode::addChild(string id, RQnode &child)
{
    child.setParent((RQnode *)this);
    ChildList::iterator itr;
    for(itr = children.begin(); itr != children.end(); ++itr) { 
       if(itr->getAttribute("nid") == id) {
	 children.insert(itr, child);
	 return;
       }
    }
    children.push_back(child); // append it if cannot find id;
}

void
RQnode::addChild(int i, RQnode &child)
{
    child.setParent((RQnode *)this);
    ChildList::iterator itr;
    int j;
    for(j=1, itr = children.begin(); itr != children.end(); ++itr, ++j) { 
        if(j == i) {
	   children.insert(itr, child);
	   return;
	}
    }
    children.push_back(child); // append it if index is out of range.
}

RQnode *
RQnode::getChild(int i)
{
    ChildList::iterator itr;
    int j;
    for(j=1, itr = children.begin(); itr != children.end(); ++itr, ++j) { 
       if(j == i) return &(*itr);
    }
    return (RQnode *)NULL;
}

RQnode *
RQnode::getChildByKey(string key)
{
    ChildList::iterator itr;
    for(itr = children.begin(); itr != children.end(); ++itr) { 
       if(itr->getKey() == key) return &(*itr);
    }
    return (RQnode *)NULL;
}

RQnode *
RQnode::getChild(string nid)
{
    ChildList::iterator itr;
    for(itr = children.begin(); itr != children.end(); ++itr) { 
       if(itr->getAttribute("nid") == nid) return &(*itr);
    }
    return (RQnode *)NULL;
}

int
RQnode::getChildrenCount()
{
    return children.size();
}

void
RQnode::readNode(string attributes)
{
// attributes has the format:
// <filenode key1 ="value1"  key2="value2" key3... >
// or <filenode key1 ="value1"  key2="value2" key3...\>
// at least one space is required before the keys.

    int p = attributes.find("<filenode", 0);
    if(p == string::npos) return;

    if(p > 0) attributes = attributes.substr(p);

    char *strptr = (char *)strtok((char *)attributes.c_str(), "=");
    if(strptr == NULL) return;

    string value;
    string key;
    string str;
    AttrMap::iterator itr;

    char *name = strstr(strptr, " ");
    while(strlen(name) > 0 && strstr(name, ">") == NULL) {
        while(name[0] == ' ') name++;
        key = string(name);
        if((p = key.find(" ",1)) != string::npos) key = key.substr(0, p);

        strptr = (char*) strtok(NULL,"=");
        while(strlen(strptr) > 0 && strptr[0] == ' ') strptr++;

        str = string(strptr);
        if((p = str.find("\"",1)) != string::npos) {

          value = str.substr(1, p-1);
          str = str.substr(p+1); // for next name and key.
          name = (char *)str.c_str();

          if((itr = attributeMap.find(key)) != attributeMap.end()) {
            attributeMap.erase(itr);
          }
          attributeMap.insert(AttrMap::value_type(key, value));

        } else name[0] = '\0';
    }
}

void
RQnode::writeNode(ofstream &fout, string indent)
{
    fout << indent << "<filenode ";

    AttrMap::iterator itr;
    for(itr = attributeMap.begin(); itr != attributeMap.end(); ++itr) {
	fout << itr->first << "=\"" << itr->second << "\" ";
    }	

    if(indexMap.size() > 0) {
      int tsize = indexMap.size();
      string str = getAttribute("tsize");
      if(str != "") tsize = atoi(str.c_str());

	fout << ">" << endl;
 	map<int, RQnode *>::iterator citr;	
	int i;

	if(doSort) { // sorted
          for(i=0, citr = indexMap.begin(); i < tsize && citr != indexMap.end(); ++i, ++citr) {
	    citr->second->writeNode(fout, indent+" ");	
	  }	
	} else { // not sorted
          ChildList::iterator itr;
          for(itr = children.begin(); itr != children.end(); ++itr) { 
           itr->writeNode(fout, indent+" ");
	  }
	}

	fout << indent << "</filenode>" << endl;

    } else {

// tsize is the max children to write out in the xml file.
      int tsize = children.size();
      string str = getAttribute("tsize");
      if(str != "") tsize = atoi(str.c_str());

      if(children.size() <= 0) {
	fout << "/>" << endl;
      } else {
	fout << ">" << endl;
 	ChildList::iterator citr;	
	int i;
        for(i=0, citr = children.begin(); i < tsize && citr != children.end(); ++i, ++citr) {
	    citr->writeNode(fout, indent+" ");	
	}	
	fout << indent << "</filenode>" << endl;
      }
    }
}

void
RQnode::showAttributes()
{
    AttrMap::iterator itr;
    for(itr = attributeMap.begin(); itr != attributeMap.end(); ++itr) {
       cout << "|" << itr->first << "| |" << itr->second << "|" << endl;
    }
}

void
RQnode::remove(string key)
{
    ChildList::iterator itr;
    for(itr = children.begin(); itr != children.end(); ++itr) {
       if(itr->getKey() == key) {
           children.erase(itr);
	   break;
       }
    }
}
