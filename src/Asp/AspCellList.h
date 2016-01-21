/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef ASPCELLLIST_H
#define ASPCELLLIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "AspCell.h"
#include "AspDataInfo.h"
#include "aipSpecStruct.h"

#define DEFAULTLAYOUT 0
#define HORIZLAYOUT 1
#define VERTLAYOUT 2
#define GRIDLAYOUT 3
#define STACKLAYOUT 4

typedef std::map<int, spAspCell_t> AspCellMap;

class AspCellList 
{
public:

    AspCellList(int id);
    ~AspCellList();
    void addCell(int ind, spAspCell_t cell);
    void clearList();
    void setLayoutType(int layout) {layoutType=layout;}
    int getLayoutType() {return layoutType;}

    spAspCell_t getCell(int id);
    spAspCell_t getFirstCell(AspCellMap::iterator& cellItr);
    spAspCell_t getNextCell(AspCellMap::iterator& cellItr);

    spAspCell_t selectCell(int x, int y);

    AspCellMap *getCellMap() {return cellMap;}
    int frameID;

protected:
    
private:

    AspCellMap *cellMap;
    int layoutType;

};

#endif /* ASPCELLLIST_H */
