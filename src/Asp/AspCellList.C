/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspUtil.h"
#include "AspCellList.h"

AspCellList::AspCellList(int id) {
   frameID=id;
   layoutType=DEFAULTLAYOUT;
   cellMap = new AspCellMap;
}

AspCellList::~AspCellList() {
   delete cellMap;
}

void AspCellList::clearList() {
   cellMap->clear();
}

void AspCellList::addCell(int ind, spAspCell_t cell) {
   cellMap->insert(AspCellMap::value_type(ind,cell));
}

spAspCell_t AspCellList::getFirstCell(AspCellMap::iterator& cellItr) {
    if (cellMap && cellMap->size() > 0) {
        cellItr = cellMap->begin();
        if (cellItr != cellMap->end()) {
            return cellItr->second;
        }
    }
    return (spAspCell_t)NULL;
}

spAspCell_t AspCellList::getNextCell(AspCellMap::iterator& cellItr) {
    if (++cellItr == cellMap->end()) {
        return (spAspCell_t)NULL;
    }
    return cellItr->second;
}

spAspCell_t AspCellList::getCell(int id) {
   AspCellMap::iterator ci;
   spAspCell_t cell;
   int i;
   for (cell= getFirstCell(ci), i=0; cell != nullAspCell; cell= getNextCell(ci), i++) {
        if(i == id) return cell;
   }
   return nullAspCell;
}

spAspCell_t AspCellList::selectCell(int x, int y) {
   if(cellMap->size() < 1) return nullAspCell;
   AspCellMap::iterator pd;
   for (pd = cellMap->begin(); pd != cellMap->end(); ++pd) {
        if(pd->second->select(x,y)) return pd->second;
   }
   return nullAspCell;
}
