/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef ASPINTEGLIST_H
#define ASPINTEGLIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "AspInteg.h"
#include "AspDataInfo.h"
#include "AspCell.h"
#include "AspTraceList.h"

typedef std::map<string, spAspInteg_t> AspIntegMap;

class AspIntegList 
{
public:

    AspIntegList();
    ~AspIntegList();

    void display(spAspCell_t cell, AspTraceList *selTraceList, spAspDataInfo_t dataInfo, 
	int integFlag, int specFlag);
    void addInteg(int ind, spAspInteg_t integ);
    void clearList();
    void deleteInteg(int ind);
    void deleteInteg();
    spAspInteg_t getInteg(int id);
    spAspInteg_t getSelInteg();
    spAspInteg_t getFirstInteg(AspIntegMap::iterator& integItr);
    spAspInteg_t getNextInteg(AspIntegMap::iterator& integItr);
    spAspInteg_t selectInteg(spAspCell_t cell, int x, int y, bool &changeFlag);

    AspIntegMap *getIntegMap() {return integMap;}
    list<spAspInteg_t> *getIntegList(bool update=false);

    int getSize() {return integMap->size();}
    void resetLabels();

    //get first integ for trace specified by dataID
    spAspInteg_t getFirstInteg(string dataID);
    // get all integs for a trace
    list<spAspInteg_t> getIntegList(string dataID);

    void update();

    double normCursor;
protected:
    
private:

    AspIntegMap *integMap;
    list<spAspInteg_t> *integList;
    list<spAspInteg_t> *disList;
};

#endif /* ASPINTEGLIST_H */
