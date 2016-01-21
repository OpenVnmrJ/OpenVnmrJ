/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspUtil.h"
#include "AspRegionList.h"
#include <float.h>
#include <math.h>

AspRegionList::AspRegionList() {
   regionMap = new AspRegionMap;
}

AspRegionList::~AspRegionList() {
   delete regionMap;
}

void AspRegionList::clearList() {
   regionMap->clear();
}

void AspRegionList::addRegion(int ind, spAspRegion_t region) {
   region->index=ind;
   regionMap->erase(region->sCoord[0].x);
   regionMap->insert(AspRegionMap::value_type(region->sCoord[0].x,region));

}

spAspRegion_t AspRegionList::getFirstRegion(AspRegionMap::iterator& regionItr) {
    if (regionMap && regionMap->size() > 0) {
        regionItr = regionMap->begin();
        if (regionItr != regionMap->end()) {
            return regionItr->second;
        }
    }
    return (spAspRegion_t)NULL;
}

spAspRegion_t AspRegionList::getNextRegion(AspRegionMap::iterator& regionItr) {
    if (++regionItr == regionMap->end()) {
        return (spAspRegion_t)NULL;
    }
    return regionItr->second;
}

spAspRegion_t AspRegionList::getSelRegion() {
   AspRegionMap::iterator itr;
   spAspRegion_t region;
   for (region= getFirstRegion(itr); region != nullAspRegion; region= getNextRegion(itr)) {
        if(region->selected != 0) return region;
   }
   return nullAspRegion;
}

// called by regionList. Note, region->index may be different than map ind.
spAspRegion_t AspRegionList::getRegion(int ind) {
   AspRegionMap::iterator itr;
   spAspRegion_t region;
   for (region= getFirstRegion(itr); region != nullAspRegion; region= getNextRegion(itr)) {
        if(region->index == ind) return region;
   }
   return nullAspRegion;
}

void AspRegionList::deleteRegion(int ind) {
   AspRegionMap::iterator itr;
   spAspRegion_t region;
   for (itr = regionMap->begin(); itr != regionMap->end(); ++itr) {
        if(itr->second->index == ind) {
	  regionMap->erase(itr->first);
	  return;
	}
   }
}

void AspRegionList::deleteRegion() {
   AspRegionMap::iterator itr;
   spAspRegion_t region;
   for (itr = regionMap->begin(); itr != regionMap->end(); ++itr) {
        if(itr->second->selected != 0) {
	  regionMap->erase(itr->first);
	}
   }
}

void AspRegionList::display(spAspCell_t cell, AspTraceList *selTraceList, spAspDataInfo_t dataInfo) {

   double px,py,pw,ph;
   cell->getPixCell(px,py,pw,ph);
   double vx,vy,vw,vh;
   cell->getValCell(vx,vy,vw,vh);
	
   set_clip_region((int)px-1,(int)(py+2*(cell->getCali(VERT))),(int)pw,(int)ph);

   AspRegionMap::iterator itr;
   spAspRegion_t region;

   for (itr = regionMap->begin(); itr != regionMap->end(); ++itr) {
        region = itr->second;
	if(region->dataID != "") {
	   spAspTrace_t trace = selTraceList->getTraceByKey(region->dataID);
	   if(trace != nullAspTrace) {
		region->display(cell,trace,dataInfo);
	   }
        }
   }
   selTraceList->showBCModel(cell); // shown only if model exist and BC not applied

   set_clip_region(0,0,0,0);
}

spAspRegion_t AspRegionList::selectRegion(spAspCell_t cell, int x, int y, bool &changeFlag) {
   spAspRegion_t prevRegion = getSelRegion();
   int selected = 0;
   if(prevRegion != nullAspRegion) {
      selected = prevRegion->selected;
   }
   spAspRegion_t region = nullAspRegion;;
   AspRegionMap::iterator itr;
   for (itr = regionMap->begin(); itr != regionMap->end(); ++itr) {
	itr->second->selected=0;
        if(region == nullAspRegion) {
           if(itr->second->select(cell,x,y)) region = itr->second;
	}
   }
   if(prevRegion == nullAspRegion && region == nullAspRegion) changeFlag = false;
   else if(prevRegion == nullAspRegion || region == nullAspRegion) changeFlag = true;
   else if(prevRegion == region) {
       if(region->selected == selected) changeFlag = false;
       else changeFlag = true;
   } else changeFlag = true;

   return region;
}

void AspRegionList::getBCMask(spAspTrace_t trace, float *baseData, float *mask, int npts) {

   for(int i=0; i<npts; i++) {
	mask[i]=0;
	baseData[i]=0;
   }

   if(trace->getTotalpts() != npts) return;
   if(trace == nullAspTrace) return;

   AspRegionMap::iterator itr;
   spAspRegion_t region;
   for (itr = regionMap->begin(); itr != regionMap->end(); ++itr) {
        region = itr->second;
	int i = trace->val2dpt(region->sCoord[0].x);
	mask[i]=1;
	baseData[i]=region->sCoord[0].y;
   }
}
