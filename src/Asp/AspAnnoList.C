/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspUtil.h"
#include "AspAnnoList.h"
#include <float.h>
#include <math.h>

AspAnnoList::AspAnnoList() {
   annoMap = new AspAnnoMap;
   disList = new list<AspAnno *>;
}

AspAnnoList::~AspAnnoList() {
   delete annoMap;
   delete disList;
}

void AspAnnoList::clearList() {
   annoMap->clear();
   disList->clear();
}

void AspAnnoList::addAnno(AspAnno *anno) {
   int ind = annoMap->size();
   anno->index = ind;
   annoMap->insert(AspAnnoMap::value_type(ind,anno));
}

AspAnno *AspAnnoList::getFirstAnno(AspAnnoMap::iterator& annoItr) {
    if (annoMap && annoMap->size() > 0) {
        annoItr = annoMap->begin();
        if (annoItr != annoMap->end()) {
            return annoItr->second;
        }
    }
    return NULL;
}

AspAnno *AspAnnoList::getNextAnno(AspAnnoMap::iterator& annoItr) {
    if (++annoItr == annoMap->end()) {
        return NULL;
    }
    return annoItr->second;
}

AspAnno *AspAnnoList::getSelAnno() {
   AspAnnoMap::iterator itr;
   AspAnno *anno;
   for (anno= getFirstAnno(itr); anno != NULL; anno= getNextAnno(itr)) {
        if(anno->selected != 0) return anno;
   }
   return NULL;
}

// called by annoList. Note, anno->index may be different than map ind.
AspAnno *AspAnnoList::getAnno(int ind) {
   AspAnnoMap::iterator itr;
   AspAnno *anno;
   for (anno= getFirstAnno(itr); anno != NULL; anno= getNextAnno(itr)) {
        if(anno->index == ind) return anno;
   }
   return NULL;
}

void AspAnnoList::deleteAnno(int ind) {
   AspAnnoMap::iterator itr;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
        if(itr->second->index == ind) {
	  annoMap->erase(itr->first);
	  break;
	}
   }
   int inx=0;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
      itr->second->index = inx;
      inx++;
   }
}

void AspAnnoList::deleteAnno() {
   AspAnnoMap::iterator itr;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
        if(itr->second->selected != 0) {
	  annoMap->erase(itr->first);
	}
   }
   int inx=0;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
      itr->second->index = inx;
      inx++;
   }
}

void AspAnnoList::display(spAspCell_t cell, spAspDataInfo_t dataInfo) {

   double px,py,pw,ph;
   cell->getPixCell(px,py,pw,ph);

   if (showPlotBx)
      set_clip_region((int)px,(int)py,(int)pw,(int)ph);
   
   AspAnnoMap::iterator itr;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
	itr->second->display(cell,dataInfo);
   }
   set_clip_region(0,0,0,0);
}

AspAnno *AspAnnoList::selectAnno(int x, int y, bool &changeFlag) {
   AspAnno *prevAnno = getSelAnno();
   int selectedHandle = 0;
   if(prevAnno != NULL) {
      selectedHandle = prevAnno->selectedHandle;
   }
   AspAnno *anno = NULL;;
   AspAnnoMap::iterator itr;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
	itr->second->selected=0;
	itr->second->selectedHandle=0;
        if(anno == NULL) {
           if(itr->second->select(x,y)) anno = itr->second;
	}
   }
   if(prevAnno == NULL && anno == NULL) changeFlag = false;
   else if(prevAnno == NULL || anno == NULL) changeFlag = true;
   else if(prevAnno == anno) {
       if(anno->selectedHandle == selectedHandle) changeFlag = false;
       else changeFlag = true;
   } else changeFlag = true;

   return anno;
}

void AspAnnoList::resetProperties() {
   AspAnnoMap::iterator itr;
   for (itr = annoMap->begin(); itr != annoMap->end(); ++itr) {
	itr->second->resetProperties();
   }
}
