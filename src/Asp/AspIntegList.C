/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspUtil.h"
#include "AspIntegList.h"
#include <float.h>
#include <math.h>

AspIntegList::AspIntegList() {
   integMap = new AspIntegMap;
   integList = new list<spAspInteg_t>;
   disList = new list<spAspInteg_t>;
}

AspIntegList::~AspIntegList() {
   delete integMap;
   delete integList;
   delete disList;
}

void AspIntegList::clearList() {
   integMap->clear();
   integList->clear();
   disList->clear();
}

void AspIntegList::addInteg(int ind, spAspInteg_t integ) {
   integ->index=ind;
   string key = integ->getKey();
   integMap->erase(key);
   integMap->insert(AspIntegMap::value_type(key,integ));

}

spAspInteg_t AspIntegList::getFirstInteg(AspIntegMap::iterator& integItr) {
    if (integMap && integMap->size() > 0) {
        integItr = integMap->begin();
        if (integItr != integMap->end()) {
            return integItr->second;
        }
    }
    return (spAspInteg_t)NULL;
}

spAspInteg_t AspIntegList::getNextInteg(AspIntegMap::iterator& integItr) {
    if (++integItr == integMap->end()) {
        return (spAspInteg_t)NULL;
    }
    return integItr->second;
}

spAspInteg_t AspIntegList::getSelInteg() {
   AspIntegMap::iterator itr;
   spAspInteg_t integ;
   for (integ= getFirstInteg(itr); integ != nullAspInteg; integ= getNextInteg(itr)) {
        if(integ->selected != 0) return integ;
   }
   return nullAspInteg;
}

// called by integList. Note, integ->index may be different than map ind.
spAspInteg_t AspIntegList::getInteg(int ind) {
   AspIntegMap::iterator itr;
   spAspInteg_t integ;
   for (integ= getFirstInteg(itr); integ != nullAspInteg; integ= getNextInteg(itr)) {
        if(integ->index == ind) return integ;
   }
   return nullAspInteg;
}

void AspIntegList::deleteInteg(int ind) {
   AspIntegMap::iterator itr;
   spAspInteg_t integ;
   for (itr = integMap->begin(); itr != integMap->end(); ++itr) {
        if(itr->second->index == ind) {
	  integMap->erase(itr->first);
	  return;
	}
   }
}

void AspIntegList::deleteInteg() {
   AspIntegMap::iterator itr;
   spAspInteg_t integ;
   for (itr = integMap->begin(); itr != integMap->end(); ++itr) {
        if(itr->second->selected != 0) {
	  integMap->erase(itr->first);
	}
   }
}

void AspIntegList::display(spAspCell_t cell, AspTraceList *selTraceList, spAspDataInfo_t dataInfo, int integFlag, int specFlag) {

   double yoff = AspUtil::getReal("io", 0.0);
   double scale = AspUtil::getReal("is", 1.0);
   double ins = AspUtil::getReal("ins", 100);
   double px,py,pw,ph;
   cell->getPixCell(px,py,pw,ph);
   double vx,vy,vw,vh;
   cell->getValCell(vx,vy,vw,vh);
	
   set_clip_region((int)px-1,(int)(py+2*(cell->getCali(VERT))),(int)pw,(int)ph);

   //list<spAspInteg_t>::iterator itr;
   //for (itr = integList->begin(); itr != integList->end(); ++itr) {

   AspIntegMap::iterator itr;
   spAspInteg_t integ;

   // norm sum to "ins"
   double norm=0.0;
   for (itr = integMap->begin(); itr != integMap->end(); ++itr) {
         norm += itr->second->absValue;
   }
   if(ins > 0 && norm > 0) norm = ins/norm; 
   else norm = 1.0;

   for (itr = integMap->begin(); itr != integMap->end(); ++itr) {
        integ = itr->second;
	integ->normValue = norm*(integ->absValue);
	if(integ->dataID != "") {
	   spAspTrace_t trace = selTraceList->getTraceByKey(integ->dataID);
	   if(trace != nullAspTrace) {
		integ->display(cell,trace,dataInfo,integFlag, scale, yoff);
	   }
        }
   }
   set_clip_region(0,0,0,0);
}

spAspInteg_t AspIntegList::selectInteg(spAspCell_t cell, int x, int y, bool &changeFlag) {
   spAspInteg_t prevInteg = getSelInteg();
   int selected = 0;
   if(prevInteg != nullAspInteg) {
      selected = prevInteg->selected;
   }
   spAspInteg_t integ = nullAspInteg;;
   AspIntegMap::iterator itr;
   for (itr = integMap->begin(); itr != integMap->end(); ++itr) {
	itr->second->selected=0;
        if(integ == nullAspInteg) {
           if(itr->second->select(cell,x,y)) integ = itr->second;
	}
   }
   if(prevInteg == nullAspInteg && integ == nullAspInteg) changeFlag = false;
   else if(prevInteg == nullAspInteg || integ == nullAspInteg) changeFlag = true;
   else if(prevInteg == integ) {
       if(integ->selected == selected) changeFlag = false;
       else changeFlag = true;
   } else changeFlag = true;

   return integ;
}

void AspIntegList::resetLabels() {
   list<spAspInteg_t>::iterator itr;
   for (itr = integList->begin(); itr != integList->end(); ++itr) {
  //      (*itr)->getLabelPos()->x=0.0;
   //     (*itr)->getLabelPos()->y=0.0;
   }
}

spAspInteg_t AspIntegList::getFirstInteg(string dataID) {
   list<spAspInteg_t>::iterator itr;
   for (itr = integList->begin(); itr != integList->end(); ++itr) {
        if((*itr)->dataID == dataID) return (*itr);
   }
   return nullAspInteg;
}

list<spAspInteg_t> AspIntegList::getIntegList(string dataID) {
   list<spAspInteg_t> newList;
   list<spAspInteg_t>::iterator itr;
   for (itr = integList->begin(); itr != integList->end(); ++itr) {
        if((*itr)->dataID == dataID) newList.push_back(*itr);
   }
   return newList;
}

// update lifrq, liamp
void AspIntegList::update() {
}
