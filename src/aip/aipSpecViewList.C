/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "aipSpecViewList.h"
#include "aipVnmrFuncs.h"
#include "aipGraphicsWin.h"
#include "aipSpecDataMgr.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "graphics.h"

extern "C" {
#include "iplan.h"
#include "iplan_graphics.h"
#include "init2d.h"
}

SpecViewList::SpecViewList(int id) {
   layoutType=CSILAYOUT;
   selSpec=false;
   frameID=id;
   showGrid=false;
   showNumber=false;
   show3DGrid=false;
   specViewMap = new SpecViewMap;
   initColors();
   rows=0;
   cols=0;
}

SpecViewList::~SpecViewList() {
   delete specViewMap;
}

void SpecViewList::setSpecKeys(std::list<string> keys) {
   clearList();
   string key;
   std::list<string>::iterator ip;
   for (ip = keys.begin(); ip != keys.end(); ++ip) {
      key = *ip;
      if(strcasecmp(key.c_str(), "grid") == 0) showGrid=true; 
      else if(strcasecmp(key.c_str(), "num") == 0) showNumber=true; 
      else if(strcasecmp(key.c_str(), "3dgrid") == 0) show3DGrid=true; 
      else if(key.length() > 0) dataKeys.push_back(key);
   }
}

void SpecViewList::clearList() {
   specViewMap->clear();
   dataKeys.clear();
   showGrid=false;
   showNumber=false;
   show3DGrid=false;
}

bool SpecViewList::hasSpec() {
   if(specViewMap->size() > 0) return true;
   else return false;
}

bool SpecViewList::showSpec() {
   if(dataKeys.size() > 0 || showGrid || show3DGrid || showNumber) return true;
   else return false;
}

void SpecViewList::setSpecViews(graphInfo_t *gInfo, int n) {
   
   specViewMap->clear();
   int i, ind;
   for(i=0; i<n; i++) {
       spSpecView_t view = spSpecView_t(new SpecView(gInfo[i]));
       if(view == (spSpecView_t)NULL) { 
	   Winfoprintf("setSpecViews: Error create SpecView.");
	   return;
       }

       ind = SpecView::getIndex(gInfo[i]);
       specViewMap->insert(SpecViewMap::value_type(ind, view));
   }
}

graphInfo_t *SpecViewList::getGraphInfo(int ind) {
   if(specViewMap->size() < 1) return NULL;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
	if(pd->first == ind) return pd->second->getGraphInfo();
   }
   return NULL;
}

// return first selected view.
int SpecViewList::selectSpecView(int x, int y) {
   if(specViewMap->size() < 1) return -1;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
	if(pd->second->isSelected(x,y)) return pd->first;
   }
   return -1;
}

void SpecViewList::selectSpecView(std::list<int> selectedViews) {
   
   if(specViewMap->size() < 1) return;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
	pd->second->select(false);
   }

   std::list<int>::iterator it;
   SpecViewMap::iterator it2;
   for(it = selectedViews.begin(); it != selectedViews.end(); ++it) {
        it2 = specViewMap->find(*it);
        if(it2 != specViewMap->end()) {
           it2->second->select(true);
        }
   }
}

void SpecViewList::displaySpec(int fx, int fy, int fw, int fh) {

   if(showGrid) drawGrid(fx,fy,fw,fh);
   if(showNumber) drawNumber(fx,fy,fw,fh);
   if(show3DGrid) aip_drawCSI3DMesh(frameID, 2);
   drawSelectedGrid(fx,fy,fw,fh);

   if(dataKeys.size() < 1) return;
   if(specViewMap->size() < 1) return;

   std::list<string>::iterator ip;
   int i;
   for (i = 0, ip = dataKeys.begin(); ip != dataKeys.end(); ++ip, ++i) {
       if(i>=12) i=0; 
       drawSpec(*ip, fx, fy, fw, fh, specColors[i]);
   }
}

void SpecViewList::drawFid(int fx, int fy, int fw, int fh, int color) {
   if(specViewMap->size() < 1) return;
   revflag = 0;
   if(initfid(1)) {
     Winfoprintf("Error: fid data does not exist.");
     return;
   }

   int ntraces = getNtraces(); 
   int maxpts = getFidPoints();

   int fidflag = 1;
   double vscale = getVScale(fidflag);

   int first = getFistPoint();
   int npts = getNumPoints();
   if(npts == 0 || npts > maxpts) npts = maxpts;
   double yoff = 0.5; // zero is half of voxel height
   int step = 2; // complex data

   int ind;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
      ind = pd->first-1;
      if(ind >= ntraces) return;
      float *d = aip_GetFid(ind);
      if(d != NULL)
         pd->second->drawSpec(d+first*step, npts*step, step, vscale, yoff, fx, fy, fw, fh, color);
   } 
}

// this calls getTrace, which works for combo key.
void SpecViewList::drawPhasefile(string key, int fx, int fy, int fw, int fh, int color) {
   if(specViewMap->size() < 1) return;
   if(init2d(GET_REV,GRAPHICS)) { 
	// failed to init spec display, try to display fids.
        drawFid(fx, fy, fw, fh, color);
	return;
   }

   int ntraces = getNtraces(); 
   int maxpts = getSpecPoints();

   int fidflag = 0;
   double vscale = getVScale(fidflag);

   int first = getFistPoint();
   int npts = getNumPoints();
   if(npts == 0 || npts > maxpts) npts = maxpts;
   double yoff = getYoff(); // zero is the bottom of voxel.

   int ind;
   SpecViewMap::iterator pd;
   SpecDataMgr *sdm = SpecDataMgr::get();
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
      ind = pd->first-1;
      if(ind >= ntraces) return;
      float *d = sdm->getTrace(key, ind, 1, maxpts);
      if(d != NULL)
         pd->second->drawSpec(d+first, npts, 1, vscale, yoff, fx, fy, fw, fh, color);
   } 
}

void SpecViewList::drawSpec(string key, int fx, int fy, int fw, int fh, int color) {
   if(specViewMap->size() < 1) return;

   if(key == "FID") {
      drawFid(fx, fy, fw, fh, color);
      return;
   } else if(key == "SPEC") {
      drawPhasefile("SPEC", fx, fy, fw, fh, color);
      return;
   } else if(key == "BASE") {
      drawPhasefile("BASE", fx, fy, fw, fh, color);
      return;
   } else if(key == "FIT") {
      drawPhasefile("FIT", fx, fy, fw, fh, color);
      return;
   }

   spSpecData_t sd = SpecDataMgr::get()->getDataByKey(key);
   if(sd == (spSpecData_t)NULL && key == "spec") {
      // fdf spec is not loaded. Show phasefile
      drawPhasefile("SPEC", fx, fy, fw, fh, color);
      return;
   } else if(sd == (spSpecData_t)NULL) {
      // failed to load fdf. Try getTrace(key,...)
      drawPhasefile(key, fx, fy, fw, fh, color);
      return;
   }
   specStruct_t *ss = sd->getSpecStruct();
   if(ss->data == NULL) return;
   if(ss->rank <  1) return;
   if(ss->matrix[0] <  1) return;

   int ntraces = sd->getNumTraces();

   int fidflag = 0;
   double vscale = getVScale(fidflag);

   int step = 1; 
   if(strcmp(ss->type,"complex") == 0) step=2;

   int maxpts = ss->matrix[0]/step; // size of spec trace 
   int mpts = getSpecPoints(); // size of phasefile trace (fn/2) 
   if(mpts < 1) mpts = maxpts;
   // in the case where maxpts and mpts are defferent, first, npts of "init2d"
   // should be scaled. make sure "first" is even if getFistPoint() is even
   double ratio = (double)maxpts/(double)mpts; 
   int first = (int)(getFistPoint()*ratio);
   int npts = (int)(getNumPoints()*ratio);
   if(npts == 0 || npts > maxpts) npts = maxpts;
   double yoff = getYoff(); // zero is the bottom of voxel.

   int ind;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
      ind = pd->first-1;
      if(ind >= ntraces) return;
      float *d = ss->data;
      if(d != NULL)
         pd->second->drawSpec(d+ ind*ss->matrix[0] + first, npts, step, vscale, yoff, fx, fy, fw, fh, color);
   } 
}

void SpecViewList::drawSelectedGrid(int fx, int fy, int fw, int fh) {
   if(specViewMap->size() < 1) return;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
        if(pd->second->isSelected()) {
           pd->second->drawGrid(fx, fy, fw, fh, gridColors[2]);
        }
   } 
}

void SpecViewList::drawGrid(int fx, int fy, int fw, int fh) {
   if(specViewMap->size() < 1) return;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
        pd->second->drawGrid(fx, fy, fw, fh, gridColors[0]);
   } 
}

void SpecViewList::drawNumber(int fx, int fy, int fw, int fh) {
   if(specViewMap->size() < 1) return;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
        pd->second->drawNumber(fx, fy, fw, fh, gridColors[3]);
   } 
}

// hard coded for now
void SpecViewList::initColors() {
   gridColors[0]=CYAN;
   gridColors[1]=BORDER_COLOR;
   gridColors[2]=RED;
   gridColors[3]=YELLOW;

   specColors[0]=GREEN;
   specColors[1]=ORANGE;
   specColors[2]=MAGENTA;
   specColors[3]=BLUE;
   specColors[4]=PINK_COLOR;
   specColors[5]=CYAN;
   specColors[6]=YELLOW;
   specColors[7]=RED;
   specColors[8]=WHITE;
   specColors[9]=GRAY_COLOR;
   specColors[10]=GRAYCUR;
   specColors[11]=FIRST_GRAY_COLOR;
}

string SpecViewList::getFirstKey() {
   if(dataKeys.size() < 1) return string("");
   std::list<string>::iterator ip = dataKeys.begin();
   if(ip != dataKeys.end()) return *ip;
   else return string("");
}

void SpecViewList::getSpecKeys(std::list<string> *keys) {
   keys->clear();
   if(showGrid) keys->push_back("grid");
   if(showNumber) keys->push_back("num");
   if(show3DGrid) keys->push_back("3dgrid");
   std::list<string>::iterator ip;
   for (ip = dataKeys.begin(); ip != dataKeys.end(); ++ip) {
      keys->push_back(*ip);
   }
}

void SpecViewList::showSpecView(int ind) {
   if(specViewMap->size() < 1) return;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
      if(pd->second->getIndex() == ind) {
	pd->second->setShow(true);
	return;
      } 
   } 
}

void SpecViewList::setRoi(bool b) {
   if(specViewMap->size() < 1) return;
   SpecViewMap::iterator pd;
   for (pd = specViewMap->begin(); pd != specViewMap->end(); ++pd) {
        pd->second->setShow(b);
   } 
}

specStruct_t *SpecViewList::get2DCSISpecStruct(string key) {
   spSpecData_t sd = SpecDataMgr::get()->getDataByKey(key);
   if(sd == (spSpecData_t)NULL) return NULL;

   specStruct_t *ss = sd->getSpecStruct();
   if(ss->data == NULL) return NULL;
   if(ss->rank <  3) return NULL;
   else return ss;
}
