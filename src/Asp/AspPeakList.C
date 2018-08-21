/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspUtil.h"
#include "AspPeakList.h"
#include <float.h>
#include <math.h>

AspPeakList::AspPeakList() {
   peakMap = new AspPeakMap;
   peakList = new list<spAspPeak_t>;
   disList = new list<spAspPeak_t>;
}

AspPeakList::~AspPeakList() {
   delete peakMap;
   delete peakList;
   delete disList;
}

void AspPeakList::clearList() {
   peakMap->clear();
   peakList->clear();
   disList->clear();
}

void AspPeakList::addPeak(int ind, spAspPeak_t peak) {
   peak->setIndex(ind);
   double xval = peak->getXval();
   char str[MAXSTR];
   if(peak->dataID=="") 
     sprintf(str,":%.4g",xval);
   else
     sprintf(str,"%s:%.4g",peak->dataID.c_str(),xval);
   string key = string(str);
   peakMap->erase(key);
   peakMap->insert(AspPeakMap::value_type(key,peak));

   // update sorted peak list
   getPeakList(true);
}

spAspPeak_t AspPeakList::getFirstPeak(AspPeakMap::iterator& peakItr) {
    if (peakMap && peakMap->size() > 0) {
        peakItr = peakMap->begin();
        if (peakItr != peakMap->end()) {
            return peakItr->second;
        }
    }
    return (spAspPeak_t)NULL;
}

spAspPeak_t AspPeakList::getNextPeak(AspPeakMap::iterator& peakItr) {
    if (++peakItr == peakMap->end()) {
        return (spAspPeak_t)NULL;
    }
    return peakItr->second;
}

spAspPeak_t AspPeakList::getSelPeak() {
   AspPeakMap::iterator itr;
   spAspPeak_t peak;
   for (peak= getFirstPeak(itr); peak != nullAspPeak; peak= getNextPeak(itr)) {
        if(peak->selected != 0) return peak;
   }
   return nullAspPeak;
}

// called by peakList. Note, peak->index may be different than map ind.
spAspPeak_t AspPeakList::getPeak(int ind) {
   AspPeakMap::iterator itr;
   spAspPeak_t peak;
   for (peak= getFirstPeak(itr); peak != nullAspPeak; peak= getNextPeak(itr)) {
        if(peak->getIndex() == ind) return peak;
   }
   return nullAspPeak;
}

void AspPeakList::deletePeak(int ind) {
   AspPeakMap::iterator itr;
   spAspPeak_t peak;
   for (itr = peakMap->begin(); itr != peakMap->end(); ++itr) {
        if(itr->second->getIndex() == ind) {
	  peakMap->erase(itr->first);
	  return;
	}
   }
   getPeakList(true);
}

void AspPeakList::deletePeak() {
   AspPeakMap::iterator itr;
   spAspPeak_t peak;
   for (itr = peakMap->begin(); itr != peakMap->end(); ++itr) {
        if(itr->second->selected != 0) {
	  peakMap->erase(itr->first);
	}
   }
   getPeakList(true);
}

list<spAspPeak_t> *AspPeakList::getPeakList(bool update) {
   if(update || peakList->size() != peakMap->size()) {
     peakList->clear();
     AspPeakMap::iterator itr1;
     list<spAspPeak_t>::iterator itr2;
     spAspPeak_t peak;
     for (itr1 = peakMap->begin(); itr1 != peakMap->end(); ++itr1) {
	peak = itr1->second;
	bool found=false;
        for (itr2 = peakList->begin(); itr2 != peakList->end(); ++itr2) {
           if(!found && (*itr2)->getXval() > peak->getXval()) {
	       	peakList->insert(itr2,peak);
	   	found=true;
	 	continue;
	   }
	}
	if(!found) peakList->push_back(peak);
      }
   }
   return peakList;	
}

// this is also called when no auto layout (to fill disList)
void AspPeakList::autoLayout(spAspCell_t cell, spAspDataInfo_t dataInfo, int peakFlag) {

    disList->clear();

    list<spAspPeak_t>::iterator itr = peakList->begin();
    if(itr == peakList->end()) return;

    string label;
    int lwd,lht;
    (*itr)->getLabel(dataInfo, label, lht, lwd);
    double lwd2 = lwd/2.0;

    double px,py,pw,ph;
    double vx,vy,vw,vh;
    cell->getPixCell(px,py,pw,ph);
    cell->getValCell(vx,vy,vw,vh);
    int maxLabels = (int)(pw/lwd);
    double lastX = px+pw;
    spAspPeak_t peak;
    double x,xval,yval;
    // keep only peaks within FOV
    int count=0;
    for (itr = peakList->begin(); itr != peakList->end(); ++itr) {
	peak = (*itr);
	peak->getAutoPos()->x=0; 
	x = cell->val2pix(HORIZ,peak->getXval());
	if(x<px || x>(px+pw)) continue;

	disList->push_back(peak);

	x += lwd2;
	if(x > lastX) {
	   count++; // number of overlapped peak labels
	   lastX=lastX-lwd;
	} else lastX = x - lwd;
    } 

    if(!(peakFlag & PEAK_VERT) || count == 0) { // no auto layout or no overlap
	return;
    }

    // the following is auto layout
    // elimiate smaller peaks if too many to fit
    int size = disList->size();
    if(size>maxLabels) {
       list<spAspPeak_t>::iterator itr2;
       double minY;
       while(size>maxLabels) { // remove smaller peaks from disList.
          minY=FLT_MAX;
          itr2 = disList->end();
          for (itr = disList->begin(); itr != disList->end(); ++itr) {
	     yval = (*itr)->getYval();
	     if(yval < minY) {
		minY=yval;
		itr2=itr;
	     }
          }
          if(itr2 != disList->end()) {
		disList->erase(itr2);
	  }
	  size = disList->size();
       }

	// now there are max # of labels, just line they up (from right to left).
	count=0;
	lastX = (px+pw)-lwd2;
	for (itr = disList->begin(); itr != disList->end(); ++itr) {
           peak = (*itr);
	   xval = peak->getXval();
	   if(xval <(vx-vw) || xval > vx) continue;

	   x = cell->val2pix(HORIZ,xval);
	   peak->getAutoPos()->x=lastX-x;
           lastX -= lwd; 
	   count++;
	}
	return;
    }

    // number of peaks are less than maxLabels, but there are overlaps 
    lastX=px+pw; // left of previous label
    double pos,center;
    list<spAspPeak_t>::iterator itr2;
    count=0;
    for (itr = disList->begin(); itr != disList->end(); ++itr) {
	peak = (*itr);
	xval = peak->getXval();

	x = cell->val2pix(HORIZ,xval) + lwd2; // right of the label 
	if(x > lastX) { // overlapped
	   count++;  // count + 1 is number of peaks in this overlapped group
           // find center of the group 
	   itr2=itr;
	   int i=0;
	   while(i<count/2) {
              if(itr2 != disList->begin()) itr2--;
              i++;
	   }
	   if(count%2 != 0) { 
                center = cell->val2pix(HORIZ,(*itr2)->getXval());
                if(itr2 != disList->begin()) itr2--;
                center =0.5*(center+cell->val2pix(HORIZ,(*itr2)->getXval()));
	   } else {
	 	center = cell->val2pix(HORIZ,(*itr2)->getXval());
	   }
	   // adjust for left limit
	   if((center - count*lwd2-lwd2) < px) center = px+count*lwd2+lwd2; 
	   // adjust for right limit
	   if((center + count*lwd2+lwd2) >(px+pw)) center = px+pw-(count*lwd2)-lwd2;
	   lastX=center-count*lwd2;
	   // set auto label location (getAutoPos()->x) of the group
	   itr2=itr;
           i=0;
           while(i<=count) { // number of group is count+1
               double x2 = cell->val2pix(HORIZ,(*itr2)->getXval());
               //(*itr2)->getAutoPos()->x = ((i-n+0.5)*(double)lwd)+center-x2;
               (*itr2)->getAutoPos()->x = lastX-x2;
               if(i<count && itr2 != disList->begin()) {
	         lastX += lwd;
		 itr2--;
	       }
               i++;
           }
	   lastX += lwd2;
	   // itr2 points at last (right end) of the group
	   // fix overlap on the right 
	   while(itr2 != disList->begin()) {
	     itr2--;
             double x2 = cell->val2pix(HORIZ,(*itr2)->getXval());
             pos = x2 + (*itr2)->getAutoPos()->x - lwd2;
	     if(lastX > pos) (*itr2)->getAutoPos()->x = lastX-x2+lwd2;
	     else break;
	     lastX = x2 + (*itr2)->getAutoPos()->x + lwd2;
	   }
	   lastX = center-count*lwd2-lwd2;
	} else {
	   lastX=x-lwd;
	   count=0; // reset group
	}
    }
}

void AspPeakList::display(spAspCell_t cell, AspTraceList *selTraceList, spAspDataInfo_t dataInfo, int peakFlag, int specFlag) {

   // this fill disList and do auto layout if flag is on.
   autoLayout(cell,dataInfo,peakFlag);
   double voff = dataInfo->getVoff()*cell->getCali(VERT);

   double hoff = 0.0;
   double px,py,pw,ph;
   cell->getPixCell(px,py,pw,ph);
   double vx,vy,vw,vh;
   cell->getValCell(vx,vy,vw,vh);
   if(specFlag & SPEC_DSS) {
      hoff = -dataInfo->getHoff()*cell->getCali(HORIZ);
      if(hoff<0) {
	pw += (hoff*(selTraceList->getSize()-1));
	px -= (hoff*(selTraceList->getSize()-1));
      } else {
	pw -= (hoff*(selTraceList->getSize()-1));
      }
   }
   double px0 = px;
	
   char thickName[64];
   getOptName(PEAK_MARK,thickName);
   set_line_thickness(thickName);

   set_clip_region((int)px-1,(int)(py+2*(cell->getCali(VERT))),(int)pw,(int)ph);

   list<spAspPeak_t>::iterator itr;
   spAspPeak_t peak;
   for (itr = disList->begin(); itr != disList->end(); ++itr) {
        peak = (*itr);
	int xoff = 0;
	int yoff = 0;
	if(peak->dataID != "") {
	   spAspTrace_t trace = selTraceList->getTraceByKey(peak->dataID);
	   if(trace != nullAspTrace) {
		if(specFlag & SPEC_DSS) {
		  double p0 = cell->val2pix(HORIZ,peak->getXval());
		  px = px0 + hoff*(trace->traceInd);
		  xoff = (int)(p0-(px + (vx-peak->getXval()) *(pw / vw)));
		}
		yoff = (int) (voff*(trace->vp));
	   }
        }
        peak->display(cell,xoff,yoff,dataInfo,peakFlag);
   }
   set_clip_region(0,0,0,0);
   set_line_thickness("Default");
}

spAspPeak_t AspPeakList::selectPeak(int x, int y, bool &changeFlag) {
   spAspPeak_t prevPeak = getSelPeak();
   int selected = 0;
   if(prevPeak != nullAspPeak) {
      selected = prevPeak->selected;
   }
   spAspPeak_t peak = nullAspPeak;;
   AspPeakMap::iterator itr;
   for (itr = peakMap->begin(); itr != peakMap->end(); ++itr) {
	itr->second->selected=0;
        if(peak == nullAspPeak) {
           if(itr->second->select(x,y)) peak = itr->second;
	}
   }
   if(prevPeak == nullAspPeak && peak == nullAspPeak) changeFlag = false;
   else if(prevPeak == nullAspPeak || peak == nullAspPeak) changeFlag = true;
   else if(prevPeak == peak) {
       if(peak->selected == selected) changeFlag = false;
       else changeFlag = true;
   } else changeFlag = true;

   return peak;
}

void AspPeakList::resetLabels() {
   list<spAspPeak_t>::iterator itr;
   for (itr = peakList->begin(); itr != peakList->end(); ++itr) {
        (*itr)->getLabelPos()->x=0.0;
        (*itr)->getLabelPos()->y=0.0;
   }
}

spAspPeak_t AspPeakList::getFirstPeak(string dataID) {
   list<spAspPeak_t>::iterator itr;
   for (itr = peakList->begin(); itr != peakList->end(); ++itr) {
        if((*itr)->dataID == dataID) return (*itr);
   }
   return nullAspPeak;
}

list<spAspPeak_t> AspPeakList::getPeakList(string dataID) {
   list<spAspPeak_t> newList;
   list<spAspPeak_t>::iterator itr;
   for (itr = peakList->begin(); itr != peakList->end(); ++itr) {
        if((*itr)->dataID == dataID) newList.push_back(*itr);
   }
   return newList;
}

void AspPeakList::shiftPeaks(string dataID, double shift) {
   list<spAspPeak_t>::iterator itr;
   for (itr = peakList->begin(); itr != peakList->end(); ++itr) {
        if((*itr)->dataID == dataID) (*itr)->setXval((*itr)->getXval()+shift);
   }
}
