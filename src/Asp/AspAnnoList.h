/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef ASPANNOLIST_H
#define ASPANNOLIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "AspAnno.h"
#include "AspDataInfo.h"
#include "AspCell.h"
#include "AspTraceList.h"

typedef std::map<int, AspAnno *> AspAnnoMap;

class AspAnnoList 
{
public:

    AspAnnoList();
    ~AspAnnoList();

    void display(spAspCell_t cell, spAspDataInfo_t dataInfo);
    void addAnno(AspAnno *anno);
    void clearList();
    void deleteAnno(int ind);
    void deleteAnno();
    AspAnno *getAnno(int id);
    AspAnno *getSelAnno();
    AspAnno *getFirstAnno(AspAnnoMap::iterator& annoItr);
    AspAnno *getNextAnno(AspAnnoMap::iterator& annoItr);
    AspAnno *selectAnno(int x, int y, bool &changeFlag);

    AspAnnoMap *getAnnoMap() {return annoMap;}

    int getSize() {return annoMap->size();}
    void resetProperties();

protected:
    
private:

    AspAnnoMap *annoMap;
    list<AspAnno *> *disList;
};

#endif /* ASPANNOLIST_H */
