/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspUtil.h"
#include "AspTraceList.h"
#include "aipSpecData.h"

AspTraceList::AspTraceList() {
   traceMap = new AspTraceMap;
   traceList = new list<spAspTrace_t>;
   renderedTraces = new list<AspRenderedTrace>;
   maxInd=0;
}

AspTraceList::~AspTraceList() {
   delete traceMap;
   delete traceList;
   delete renderedTraces;
}

void AspTraceList::clearList() {
   traceMap->clear();
   traceList->clear();
   renderedTraces->clear();
}

void AspTraceList::addTrace(string key, spAspTrace_t trace) {
   if(trace->getNpts() < 2) {
	Winfoprintf("Error creating trace %s: %d data points.",trace->getKeyInd().c_str(),trace->getNpts());
	return;
   }
 
   traceMap->erase(key);
   traceMap->insert(AspTraceMap::value_type(key,trace));
}

spAspTrace_t AspTraceList::getFirstTrace(AspTraceMap::iterator& traceItr) {
    if (traceMap && traceMap->size() > 0) {
        traceItr = traceMap->begin();
        if (traceItr != traceMap->end()) {
            return traceItr->second;
        }
    }
    return (spAspTrace_t)NULL;
}

spAspTrace_t AspTraceList::getNextTrace(AspTraceMap::iterator& traceItr) {
    if (++traceItr == traceMap->end()) {
        return (spAspTrace_t)NULL;
    }
    return traceItr->second;
}

spAspTrace_t AspTraceList::getSelTrace() {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (trace= getFirstTrace(ti); trace != nullAspTrace; trace= getNextTrace(ti)) {
        if(trace->selected) return trace;
   }
   return nullAspTrace;
}

spAspTrace_t AspTraceList::getTraceByKey(string key) {
   if(key == "") return nullAspTrace;
   AspTraceMap::iterator ti;
   for (ti = traceMap->begin(); ti != traceMap->end(); ++ti) {
        if(ti->first == key) return ti->second;
   }
   return nullAspTrace;
}

// called by traceList
spAspTrace_t AspTraceList::getTraceByInd(int id) {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (trace= getFirstTrace(ti); trace != nullAspTrace; trace= getNextTrace(ti)) {
        if(trace->traceInd == id) return trace;
   }
   return nullAspTrace;
}

// this depends on how traces are order in the map.
// called only by selTraceList when setting colors
spAspTrace_t AspTraceList::getTrace(int id) {
   list<spAspTrace_t> *traces = getTraceList();
   list<spAspTrace_t>::iterator ti;
   int i;
   for (ti = traces->begin(), i=0; ti != traces->end(); ++ti, i++) {
        if(i == id) return *ti;
   }
   return nullAspTrace;
}

double AspTraceList::getMinX() {
   if(traceMap->size() < 1) return 0.0;
   AspTraceMap::iterator pd = traceMap->begin();
   double min = pd->second->getMinX();
   double d;
   while(pd != traceMap->end()) {
	d = pd->second->getMinX();
        if(d<min) min=d;;
	++pd;
   }
   return min;
}

double AspTraceList::getMaxX() {
   if(traceMap->size() < 1) return 0.0;
   AspTraceMap::iterator pd = traceMap->begin();
   double max = pd->second->getMaxX();
   double d;
   while(pd != traceMap->end()) {
	d = pd->second->getMaxX();
        if(d>max) max=d;;
	++pd;
   }
   return max;
}

void AspTraceList::resetVpos() {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (trace= getFirstTrace(ti); trace != nullAspTrace; trace= getNextTrace(ti)) {
        trace->vp=0.0;
   }
}

void AspTraceList::setLabelFlag(int flag) {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (trace= getFirstTrace(ti); trace != nullAspTrace; trace= getNextTrace(ti)) {
        trace->labelFlag=flag;
   }
}

void AspTraceList::setLabels(int argc, char *argv[], int flag) {
   list<spAspTrace_t> *traces = getTraceList();
   list<spAspTrace_t>::iterator ti;
   int i;
   for (ti = traces->begin(), i=0; ti != traces->end() && i<argc; ++ti, i++) {
        (*ti)->label=string(argv[i]);
	(*ti)->labelFlag=flag;
   }
}

string AspTraceList::getIndList() {
   string str = "";
   list<spAspTrace_t> *traces = getTraceList();
   list<spAspTrace_t>::iterator ti;
   for (ti = traces->begin(); ti != traces->end(); ++ti) {
        str = str + (*ti)->getIndex() + " ";
   }
   return str;
}

void AspTraceList::deleteTrace(int ind) {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (ti = traceMap->begin(); ti != traceMap->end(); ++ti) {
        if(ti->second->traceInd == ind) {
	  traceMap->erase(ti->first);
	  return;
	}
   }
}

// Note, this is dataKey, not trace key.
void AspTraceList::deleteTrace(string key) {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (ti = traceMap->begin(); ti != traceMap->end(); ++ti) {
        if(ti->second->dataKey == key) {
	  traceMap->erase(ti->first);
	}
   }
}

void AspTraceList::deleteTrace() {
   AspTraceMap::iterator ti;
   spAspTrace_t trace;
   for (ti = traceMap->begin(); ti != traceMap->end(); ++ti) {
        if(ti->second->selected) {
	  traceMap->erase(ti->first);
	}
   }
}

list<spAspTrace_t> *AspTraceList::getTraceList() {
	traceList->clear();
	int i;
	int n = (maxInd>0) ? maxInd:traceMap->size();
	for(i=0;i<n;i++) {
	  spAspTrace_t trace = getTraceByInd(i);
	  if(trace != nullAspTrace) traceList->push_back(trace); 
	}
	return traceList;	
}

void AspTraceList::showSpec(spAspCell_t cell, int mode) {

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  getTraceList();

  if(traceList == NULL || traceList->size() < 1) return;
  if(dataInfo == nullAspData) return;

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  //set line thickness
  char name1[64], name2[64];
  getOptName(SPEC_LINE_MIN,name1);
  getOptName(SPEC_LINE_MAX,name2);
  double thickFactor;
  if(dataInfo->rank>1) thickFactor = getReal("thickFactor", 2, 0.0); // 2D trace
  else thickFactor = getReal("thickFactor", 1, 0.0); // 1D trace
  double pts = (dataInfo->haxis.npts) * (dataInfo->haxis.width) / (dataInfo->haxis.maxwidth);
  if(pts < 1) pts=1.0;
  // line thickness min + d*(max-min);
  // d should be 0.0 to 1.0
  double d=thickFactor * pwd / pts; 
  if(d>1.0) d=1.0;
  if(d<0) d=0.0;
  
  set_spectrum_thickness(name1, name2, d);

  set_clip_region((int)pstx-1,(int)psty,(int)pwd,(int)pht);

  if(mode == SPEC_DSS) showSpec_dss(cell);
  else showSpec_ds(cell);

  set_clip_region(0,0,0,0);
  set_spectrum_thickness(name1, name2, 0.0);
}

void AspTraceList::showSpec_ds(spAspCell_t cell) {

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  getTraceList();

  if(traceList == NULL or traceList->size() < 1) return;
  if(dataInfo == nullAspData) return;

  int autoVS = (int)getReal("autoVS",0.0);

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  double cali = cell->getCali(VERT);
  double voff = cali*(dataInfo->getVoff());
  double vpos = cali*(dataInfo->getVpos());
  double vscale = cali*(dataInfo->getVscale());
  
   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;
   int i;
   renderedTraces->clear();
   for(itr=traceList->begin(), i=0; itr != traceList->end() && i<MAXTRACE; ++itr, ++i) {
	trace = *itr;
	float *data = trace->getData();
	if(!data) continue;
	double min = trace->getMinX() + trace->getShift();
	double max = trace->getMaxX() + trace->getShift();
	double wd = max-min;
	int npts = trace->getNpts();
	int step = 1;
	double scale = 1.0;
	if(autoVS) {
	  float miny, maxy;
	  SpecData::getDataMin(data, npts, &miny);
          SpecData::getDataMax(data, npts, &maxy);
     	  scale = 1.0 / (double)(maxy-miny);
	} else {
	  scale = vscale*(trace->scale); 
	}
	// old definition of trace->vp, i.e., as addition to i*vo 
	//double yoff = vpos + (i*voff) + (trace->vp)*cali;
	// new definition of trace->vp, i.e., as a multiply factor of vo 
	double yoff = vpos + voff*(trace->vp);
	// the following set start of data, points of data, and FOV.
	// and call drawPolyline to draw spectra.
	double px,pw;
	if(max < vstx && min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, max);
	  pw = cell->val2pix(HORIZ, min) - px;
	} else if(max < vstx) {
	  px = cell->val2pix(HORIZ, max);
	  pw = pwd - (px - pstx);
	  npts = (int)(0.5+npts*(vwd - (vstx-max))/wd);
	} else if(min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, min);
	  pw = pwd - (pstx + pwd - px);
	  px = pstx;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*(vstx - min)/wd);
	} else {
	  px = pstx;
	  pw = pwd;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*vwd/wd);
	}
	if((trace->labelFlag & SPEC_INDEX) || (trace->labelFlag & SPEC_LABEL)) { 
     	   int yp = (int) (psty + pht - yoff) + ycharpixels;
	   AspUtil::writeFields((char *)(trace->getLabel(dataInfo).c_str()),2+(int)px,yp,0,0);
	}
	double vx = cell->pix2val(HORIZ,px);
	double vw = vx-cell->pix2val(HORIZ, px+pw);
	renderedTraces->push_back(AspRenderedTrace(trace,px,pw,vx,vw,scale,yoff));

        int color; 
        if(trace->selected || !(trace->ismultiColor())) { // mono color
	   // this following line is needed to correct a shift caused by "expand" method, 
	   // where the step is calculated by (pw-1)/(npts-1)
           if(pw > npts && (npts+1) < (trace->getTotalpts())) npts += 1;
	   color = trace->setColor(); 
 	   cell->drawPolyline(data,npts,step,px,psty,pw,pht,color,scale,yoff);
        } else { // multi color trace 
	   double savemin = trace->getMinX();
	   double savemax = trace->getMaxX();
	   list<spAspTrace_t> *sumTraceList = trace->getSumTraceList();
	   list<spAspTrace_t>::iterator sitr;
	   int count=0;
	   // draw spectral segment from right to left
	   // draw two spec segments for each trace segment (one before the segment).
           for (sitr=sumTraceList->begin(); count<=(int)sumTraceList->size(); ++sitr) {
	      // draw segment before this trace segment in spec color
	      if(sitr == sumTraceList->end() ) max = savemax + trace->getShift();
	      else max = (*sitr)->getMinX() + (*sitr)->getShift();
	      if(max>min) {
                color = trace->setColor();
                trace->setMinX(min);
                trace->setMaxX(max);
                data = trace->getData();
                if(!data) continue;
                wd = max-min;
                npts = trace->getNpts();
                if(max < vstx && min > (vstx-vwd)) {
                  px = cell->val2pix(HORIZ, max);
                  pw = cell->val2pix(HORIZ, min) - px;
                } else if(max < vstx) {
                  px = cell->val2pix(HORIZ, max);
                  pw = pwd - (px - pstx);
                  npts = (int)(0.5+npts*(vwd - (vstx-max))/wd);
                } else if(min > (vstx-vwd)) {
                  px = cell->val2pix(HORIZ, min);
                  pw = pwd - (pstx + pwd - px);
                  px = pstx;
                  data += (int)(0.5+(max-vstx)*npts/wd); 
                  npts = (int)(0.5+npts*(vstx - min)/wd);
                } else {
                  px = pstx;
                  pw = pwd;
                  data += (int)(0.5+(max-vstx)*npts/wd); 
                  npts = (int)(0.5+npts*vwd/wd);
                }
           	if(pw > npts && (npts+1) < (trace->getTotalpts())) npts += 1;
                cell->drawPolyline(data,npts,step,px,psty,pw,pht,color,scale,yoff);
	      }

	      // draw this trace segment in this trace color 
              min=max;
	      if(sitr == sumTraceList->end() ) max = savemax + trace->getShift();
	      else max = (*sitr)->getMaxX() + (*sitr)->getShift();
	      if(max>min && sitr != sumTraceList->end() ) {
                color = (*sitr)->setColor();
                trace->setMinX(min);
                trace->setMaxX(max);
                data = trace->getData();
                if(!data) continue;
                wd = max-min;
                npts = trace->getNpts();
                if(max < vstx && min > (vstx-vwd)) {
                  px = cell->val2pix(HORIZ, max);
                  pw = cell->val2pix(HORIZ, min) - px;
                } else if(max < vstx) {
                  px = cell->val2pix(HORIZ, max);
                  pw = pwd - (px - pstx);
                  npts = (int)(0.5+npts*(vwd - (vstx-max))/wd);
                } else if(min > (vstx-vwd)) {
                  px = cell->val2pix(HORIZ, min);
                  pw = pwd - (pstx + pwd - px);
                  px = pstx;
                  data += (int)(0.5+(max-vstx)*npts/wd); 
                  npts = (int)(0.5+npts*(vstx - min)/wd);
                } else {
                  px = pstx;
                  pw = pwd;
                  data += (int)(0.5+(max-vstx)*npts/wd); 
                  npts = (int)(0.5+npts*vwd/wd);
                }
           	if(pw > npts && (npts+1) < (trace->getTotalpts())) npts += 1;
                cell->drawPolyline(data,npts,step,px,psty,pw,pht,color,scale,yoff);
	      }
              min=max;
	      count++;
	   }
	   trace->setMinX(savemin);
	   trace->setMaxX(savemax);
        }
   }
}

void AspTraceList::showSpec_dss(spAspCell_t cell) {

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  getTraceList();

  if(traceList == NULL or traceList->size() < 1) return;
  if(dataInfo == nullAspData) return;

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  int autoVS = (int)getReal("autoVS",0.0);

  double vcali = cell->getCali(VERT);
  double voff = vcali*(dataInfo->getVoff());
  double vpos = vcali*(dataInfo->getVpos());
  double vscale = vcali*(dataInfo->getVscale());
  double hcali = cell->getCali(HORIZ);
  double hoff = -hcali*(dataInfo->getHoff());
  
  double pstxSave=pstx;
  double pwdSave=pwd;

   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;

   if(hoff<0) {
   	pwd += (hoff*(traceList->size()-1));
	pstx -= (hoff*(traceList->size()-1));
   } else {
   	pwd -= (hoff*(traceList->size()-1));
   }

   int i;
   renderedTraces->clear();
   for(itr=traceList->begin(), i=0; itr != traceList->end() && i<MAXTRACE; ++itr, ++i) {
	trace = *itr;
	float *data = trace->getData();
	if(!data) continue;
	double min = trace->getMinX() + trace->getShift();
	double max = trace->getMaxX() + trace->getShift();
	double wd = max-min;
	int npts = trace->getNpts();
	int step = 1;
	double scale = 1.0;
	if(autoVS) {
	  float miny, maxy;
	  SpecData::getDataMin(data, npts, &miny);
          SpecData::getDataMax(data, npts, &maxy);
     	  scale = 1.0 / (double)(max-miny);
	} else {
	  scale = vscale*(trace->scale); 
	}
	// old definition of trace->vp, i.e., as addition to i*vo 
	//double yoff = vpos + (i*voff) + (trace->vp)*vcali;
	// new definition of trace->vp, i.e., as a multiply factor of vo 
	double yoff = vpos + voff*(trace->vp);
	// the following set start of data, points of data, and FOV.
	// and call drawPolyline to draw spectra.
	double px,pw;
	if(max < vstx && min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, max);
	  pw = cell->val2pix(HORIZ, min) - px;
	} else if(max < vstx) {
	  px = cell->val2pix(HORIZ, max);
	  pw = pwd - (px - pstx);
	  npts = (int)(0.5+npts*(vwd - (vstx-max))/wd);
	} else if(min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, min);
	  pw = pwd - (pstx + pwd - px);
	  px = pstx;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*(vstx - min)/wd);
	} else {
	  px = pstx;
	  pw = pwd;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*vwd/wd);
	}
        if(pw > npts && (npts+1) < (trace->getTotalpts())) npts += 1;
	int color = trace->setColor();
 	cell->drawPolyline(data,npts,step,px,psty,pw,pht,color,scale,yoff);
	if((trace->labelFlag & SPEC_INDEX) || (trace->labelFlag & SPEC_LABEL)) { 
     	   int yp = (int) (psty + pht - yoff) + ycharpixels;
	   AspUtil::writeFields((char *)(trace->getLabel(dataInfo).c_str()),2+(int)px,yp,0,0);
	}
	double vx = cell->pix2val(HORIZ, px);
	double vw = vx-cell->pix2val(HORIZ, px+pw);
	renderedTraces->push_back(AspRenderedTrace(trace,px,pw,vx,vw,scale,yoff));
	pstx += hoff;
   }

   pstx=pstxSave;
   pwd=pwdSave;
}

// display a single trace in the cell.
void AspTraceList::showSpec(spAspCell_t cell, spAspTrace_t trace) {

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  getTraceList();

  if(trace == nullAspTrace) return;
  if(dataInfo == nullAspData) return;

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  int autoVS = (int)getReal("autoVS",0.0);

  double vcali = cell->getCali(VERT);
  double vpos = vcali*(dataInfo->getVpos());
  double vscale = vcali*(dataInfo->getVscale());

  int off = (int)(2*(cell->getCali(VERT)));
  // x margin
  int xoff = (int)(0.04*pwd);
  if(xoff>4) xoff=4;
  set_clip_region((int)pstx+xoff,(int)psty+off,(int)pwd-2*xoff,(int)pht);
	float *data = trace->getData();
	if(!data) return;
	double min = trace->getMinX() + trace->getShift();
	double max = trace->getMaxX() + trace->getShift();
	double wd = max-min;
	int npts = trace->getNpts();
	int step = 1;
	double scale = 1.0;
	if(autoVS) {
	  float miny, maxy;
	  SpecData::getDataMin(data, npts, &miny);
          SpecData::getDataMax(data, npts, &maxy);
     	  scale = 1.0 / (double)(maxy-miny);
	} else {
	  scale = vscale*(trace->scale); 
	}
	double yoff = vpos;
	// the following set start of data, points of data, and FOV.
	// and call drawPolyline to draw spectra.
	double px,pw;
	if(max < vstx && min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, max);
	  pw = cell->val2pix(HORIZ, min) - px;
	} else if(max < vstx) {
	  px = cell->val2pix(HORIZ, max);
	  pw = pwd - (px - pstx);
	  npts = (int)(0.5+npts*(vwd - (vstx-max))/wd);
	} else if(min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, min);
	  pw = pwd - (pstx + pwd - px);
	  px = pstx;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*(vstx - min)/wd);
	} else {
	  px = pstx;
	  pw = pwd;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*vwd/wd);
	}
        if(pw > npts && (npts+1) < (trace->getTotalpts())) npts += 1;
	int color = trace->setColor();
 	cell->drawPolyline(data,npts,step,px,psty,pw,pht,color,scale,yoff);
	if((trace->labelFlag & SPEC_INDEX) || (trace->labelFlag & SPEC_LABEL)) { 
     	   int yp = (int) (psty + pht - yoff) + ycharpixels;
	   AspUtil::writeFields((char *)(trace->getLabel(dataInfo).c_str()),(int)px+xoff,yp,0,0);
	}
  set_clip_region(0,0,0,0);
  renderedTraces->clear();
  double vx = cell->pix2val(HORIZ, px);
  double vw = vx-cell->pix2val(HORIZ, px+pw);
  renderedTraces->push_back(AspRenderedTrace(trace,px,pw,vx,vw,scale,yoff));

}

spAspTrace_t AspTraceList::nearestTrace(spAspCell_t cell, int x, int y) {

  if(traceMap->size()<1) return nullAspTrace;

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  double cali = cell->getCali(VERT);
  double voff = cali*(dataInfo->getVoff());

   double fpp = vwd/pwd; // freq ppm per pixel
   double ppp, freq,amp,d, mind=fabs(pht);
   int pos,i;
   float *data;
   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;
   string selKey ="";
   for(itr=traceList->begin(); itr != traceList->end(); ++itr) {
        trace = (*itr);
	freq = cell->pix2val(HORIZ,(double)x) - trace->getShift();
        ppp = trace->getPPP(); // ppm per data point
	if(ppp == 0) continue;
        ppp = fpp/ppp;// points per pixel
	pos = trace->val2dpt(freq)-(int)ppp-1;	
	data = trace->getData()+pos;
	for(i=0;i<2*ppp+2;i++) {
	   amp = fabs(*(data+i)); 
	   d = cell->val2pix(VERT,amp);
	   d = fabs(d - voff*(trace->vp) - y);
	   if(d < mind) {
		mind=d; 
		selKey= string(trace->getKeyInd());
	   }
	}
   }

   return getTraceByKey(selKey);
}

spAspTrace_t AspTraceList::selectTrace(spAspCell_t cell, int x, int y, bool &changeFlag) {

  if(traceMap->size()<1) return nullAspTrace;

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  double cali = cell->getCali(VERT);
  double voff = cali*(dataInfo->getVoff());

   double fpp = vwd/pwd; // freq ppm per pixel
   double ppp, freq,amp,d, mind=3; // 3 pixels
   int pos,i;
   float *data;
   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;
   string selKey ="";
   string prevKey = "";
   for(itr=traceList->begin(); itr != traceList->end(); ++itr) {
        trace = (*itr);
	if(trace->selected) prevKey = string(trace->getKeyInd()); 
        trace->selected = false;
	if(selKey == "") {
	  freq = cell->pix2val(HORIZ,(double)x) - trace->getShift();
          ppp = trace->getPPP(); // ppm per data point
	  if(ppp == 0) continue;
          ppp = fpp/ppp;// points per pixel
	  pos = trace->val2dpt(freq)-(int)ppp-1;	
	  data = trace->getData()+pos;
	  for(i=0;i<2*ppp+2;i++) {
	    amp = fabs(*(data+i)); 
	    d = cell->val2pix(VERT,amp);
	    d = fabs(d - voff*(trace->vp) - y);
	    if(d < mind) {
		trace->selected = true;
		selKey= string(trace->getKeyInd());
	    }
	  }
	}
   }

   changeFlag=(selKey != prevKey) ? true:false;

   return getTraceByKey(selKey);;
}

/*
spAspTrace_t AspTraceList::selectTrace(spAspCell_t cell, int x, int y, bool &changeFlag) {

   spAspTrace_t selTrace = nullAspTrace;

  if(renderedTraces->size()<1) return selTrace;

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

   double vstxSave=vstx;
   double vwdSave=vwd;
   double pstxSave=pstx;
   double pwdSave=pwd;
   double d,dmin = pht;
   list<AspRenderedTrace>::iterator itr;
   list<AspRenderedTrace>::iterator itr2 = renderedTraces->end();
   spAspTrace_t trace;
   for (itr = renderedTraces->begin(); itr != renderedTraces->end(); ++itr) {
      trace = itr->trace;
      if(trace->selected) itr2 = itr;
      trace->selected = false;
      pstx = itr->pstx;
      pwd = itr->pwd;
      vstx = itr->vstx;
      vwd = itr->vwd;
      if(x < pstx || x > (pstx+pwd)) continue;
      if(y < psty || y > (psty+pht)) continue;

      double voff = itr->voff;
      double vscale = itr->vscale;
      double freq = cell->pix2val(HORIZ,x);
      float amp = trace->getAmp(freq); 
      if(amp==0) continue;
      d = psty + pht - voff - vscale * amp;
//Winfoprintf("trace %d %d %f %f %f %f %f %f %f %f %f",x,y,d,pstx,psty,pwd,pht,vstx,vsty,vwd,vht);
      d = fabs(d-y);
      if(d < dmin) {
	dmin=d; 
	if(dmin < 4) selTrace=trace;
      }
   }

   vstx=vstxSave;
   vwd=vwdSave;
   pstx=pstxSave;
   pwd=pwdSave;

   if(selTrace != nullAspTrace) { // less than 4 pixels
        selTrace->selected = true;
   } 

   if(itr2 != renderedTraces->end()) 
      changeFlag = (selTrace != itr2->trace);
   else changeFlag = (selTrace != nullAspTrace); 

   return selTrace;
}
*/

void AspTraceList::showBCModel(spAspCell_t cell) {

  spAspDataInfo_t dataInfo = cell->getDataInfo();
  getTraceList();

  if(traceList == NULL or traceList->size() < 1) return;
  if(dataInfo == nullAspData) return;

  int autoVS = (int)getReal("autoVS",0.0);

  double pstx,psty,pwd,pht;
  cell->getPixCell(pstx,psty,pwd,pht);
  double vstx,vsty,vwd,vht;
  cell->getValCell(vstx,vsty,vwd,vht);

  double cali = cell->getCali(VERT);
  double voff = cali*(dataInfo->getVoff());
  double vpos = cali*(dataInfo->getVpos());
  double vscale = cali*(dataInfo->getVscale());
  
   list<spAspTrace_t>::iterator itr;
   spAspTrace_t trace;
   int i;
   renderedTraces->clear();
   for(itr=traceList->begin(), i=0; itr != traceList->end() && i<MAXTRACE; ++itr, ++i) {
	trace = *itr;
        if(trace->getDoneBC()) continue; // don't show if BC applied
	float *data = trace->getBCModel();
        data += trace->getFirst();
	if(!data) continue;
	double min = trace->getMinX() + trace->getShift();
	double max = trace->getMaxX() + trace->getShift();
	double wd = max-min;
	int npts = trace->getNpts();
	int step = 1;
	int color = PINK_COLOR;
	double scale = 1.0;
	if(autoVS) {
	  float miny, maxy;
	  SpecData::getDataMin(data, npts, &miny);
          SpecData::getDataMax(data, npts, &maxy);
     	  scale = 1.0 / (double)(maxy-miny);
	} else {
	  scale = vscale*(trace->scale); 
	}
	double yoff = vpos + voff*(trace->vp);
	double px,pw;
	if(max < vstx && min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, max);
	  pw = cell->val2pix(HORIZ, min) - px;
	} else if(max < vstx) {
	  px = cell->val2pix(HORIZ, max);
	  pw = pwd - (px - pstx);
	  npts = (int)(0.5+npts*(vwd - (vstx-max))/wd);
	} else if(min > (vstx-vwd)) {
	  px = cell->val2pix(HORIZ, min);
	  pw = pwd - (pstx + pwd - px);
	  px = pstx;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*(vstx - min)/wd);
	} else {
	  px = pstx;
	  pw = pwd;
	  data += (int)(0.5+(max-vstx)*npts/wd); 
	  npts = (int)(0.5+npts*vwd/wd);
	}
	
        if(pw > npts && (npts+1) < (trace->getTotalpts())) npts += 1;
 	cell->drawPolyline(data,npts,step,px,psty,pw,pht,color,scale,yoff);
   }
}
