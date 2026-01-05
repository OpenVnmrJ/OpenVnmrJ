/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <algorithm>
using std::swap;
#include <cmath>
using std::sqrt;
using std::fabs;
using namespace std;

#include "AspUtil.h"
#include "AspDisRegion.h"
#include "AspMouse.h"
#include "AspFrameMgr.h"
#include "AspDis1D.h"

AspRegionList *AspDisRegion::regionList = new AspRegionList();
int AspDisRegion::regionFlag = 0;

extern "C" {
void BaselineFit(float *indata, int n, float *basemask, double smoothness);
void BaseMask(float *inData, float *basmask, int n, int aScale);
int calcBCFit(float *data, float *model, float *mask, int npts, int bcpts);
void window_redisplay();
}

int AspDisRegion::aspRegion(int argc, char *argv[], int retc, char *retv[]) {

    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();

    if(frame == nullAspFrame) RETURN;

    if(argc==1 && retc > 0) { // quary display info
	if(regionFlag == 0) retv[0]=realString(0.0);
	else retv[0]=realString(1.0);
	if(retc>1) retv[1]=realString((double)(getRegionList()->getSize()));
	RETURN;
    }

    spAspDataInfo_t dataInfo = frame->getDefaultDataInfo(true);
    if(dataInfo == nullAspData) RETURN;

   if(argc>1 && strcmp(argv[1],"showCWT") == 0) { // auto define base mask with CWT
	calcBaseMask(frame);
   } else if(argc>1 && strcasecmp(argv[1],"CWT") == 0) { // model baseline based on CWT
	getRegionList()->clearList();
	// BCmodel will be calculated by calcBCModel_CWT
	int mode = (int)AspUtil::getReal("bcmode",1,0);
	if(mode>0) AspUtil::setReal("bcmode",1,0,true);
   	calcBCModel_CWT(frame->getDefaultTrace());

	regionFlag = 1;
	frame->displayTop();

   } else if(argc>1 && strcasecmp(argv[1],"bm") == 0) { // calc baseline model with Whittaker smoother
   	calcBCModel(frame->getDefaultTrace());
   } else if(argc>1 && strcasecmp(argv[1],"save") == 0) {
        char path[MAXSTR2];
        if(argc >2) {
	    strcpy(path,argv[2]);
            save(frame,path);
	} else saveBCfrq(frame);

   } else if(argc>1 && strcasecmp(argv[1],"load") == 0) {
        char path[MAXSTR2];
        if(argc >2) {
	   strcpy(path,argv[2]);
           load(frame,path,true);
	} else loadBCfrq(frame);

   } else if(argc>1 && strcasecmp(argv[1],"show") == 0) {
	 regionFlag = 1; 
   	 frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"hide") == 0) {
	 regionFlag = 0; 
   	 frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"doBC") == 0) { // do baseline correction
	 doBC(frame);
	 regionFlag = 0;
	 window_redisplay();
   } else if(argc>1 && strcasecmp(argv[1],"undoBC") == 0) { // undo baseline correction
	 undoBC(frame);
	 regionFlag = 1;
	 window_redisplay();
   } else if(argc>1 && strcasecmp(argv[1],"delete") == 0) {
	 deleteRegion(frame, nullAspRegion);
   } else if(argc>1 && strcasecmp(argv[1],"clear") == 0) {
         getRegionList()->clearList();
	 regionFlag = 0;
   	 frame->displayTop();
   } else if(argc>3 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
           spAspRegion_t region = getRegionList()->getRegion(atoi(argv[2])-1);
           if(region != nullAspRegion) {
                retv[0] = newString(region->getProperty(argv[3]).c_str());
           }
   } else if(argc>2 && retc > 0 && strcasecmp(argv[1],"get") == 0) {
           spAspRegion_t region = getRegionList()->getSelRegion();
           if(region != nullAspRegion) {
                retv[0] = newString(region->getProperty(argv[2]).c_str());
           }
   } else if(argc>4 && strcasecmp(argv[1],"set") == 0) {
           spAspRegion_t region = getRegionList()->getRegion(atoi(argv[2])-1);
           if(region != nullAspRegion) {
                region->setProperty(argv[3],argv[4]);
                frame->displayTop();
           }
         frame->displayTop();
   } else if(argc>3 && strcasecmp(argv[1],"set") == 0) {
           spAspRegion_t region = getRegionList()->getSelRegion();
           if(region != nullAspRegion) {
                region->setProperty(argv[2],argv[3]);
                frame->displayTop();
           }
         frame->displayTop();
   } else if(argc>1 && isdigit(argv[1][0]) ) { // find base points, argv[1] is number of points
	basePoints(frame, atoi(argv[1]));
   }

   frame->updateMenu();

   RETURN;
}

void AspDisRegion::save(spAspFrame_t frame, char *path) {
   // make sure dir exists
   string tmp = string(path);
   string dir = tmp.substr(0,tmp.find_last_of("/"));

   struct stat fstat;
   if (stat(dir.c_str(), &fstat) != 0) {
       char str[MAXSTR2];
       (void)sprintf(str, "mkdir -p %s \n", dir.c_str());
       (void)system(str);
   }

   FILE *fp;
   if(!(fp = fopen(path, "w"))) {
        Winfoprintf("Failed to create region file %s.",path);
        return;
   }

   time_t clock;
   char *tdate;
   char datetim[26];
   clock = time(NULL);
   tdate = ctime(&clock);
   if (tdate != NULL) {
     strcpy(datetim,tdate);
     datetim[24] = '\0';
   } else {
     strcpy(datetim,"???");
   }

   AspRegionList *regionList = getRegionList();
   int nregions = regionList->getSize();

   fprintf(fp,"# Created by %s on %s at machine %s.\n",UserName,datetim,HostName);
   fprintf(fp,"number_of_regions: %d\n",nregions);
   fprintf(fp,"# index,ppm1 ppm2,dataID,color\n");

   int i;
   spAspRegion_t region;
   for(i=0;i<nregions;i++) {
	region = regionList->getRegion(i);
	if(region != nullAspRegion) { 
	   fprintf(fp,"%s\n",region->toString().c_str());
	}
   }

   fclose(fp);
}

void AspDisRegion::load(spAspFrame_t frame, char *path, bool show) {

   struct stat fstat;
   if(stat(path, &fstat) != 0) {
        Winfoprintf("Error: cannot find %s.",path);
        return;
   }

   // make sure at least one cell exists
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;
   
   FILE *fp;
   if(!(fp = fopen(path, "r"))) {
        Winfoprintf("Failed to open session file %s.",path);
        return;
   }

   char  buf[MAXSTR], words[MAXWORDNUM][MAXSTR], *tokptr;
   int nw=0;

   AspRegionList *regionList = getRegionList();
   regionList->clearList();

   int nregions=0;
   int count=0;
   while (fgets(buf,sizeof(buf),fp)) {
      if(strlen(buf) < 1 || buf[0] == '#') continue;
          // break buf into tok of parameter names

      nw=0;
      tokptr = strtok(buf, " \n");
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, " \n");
      }

      if(nw < 2) continue;
      if(strcasecmp(words[0],"number_of_regions:")==0) {
	nregions = atoi(words[1]); 
      } else if(nregions>0 && nw>4) {
	spAspRegion_t region = spAspRegion_t(new AspRegion(words,nw));
	regionList->addRegion(region->index, region);	
	count++;
      }
   }
   fclose(fp);

   if(count == 0) { 
	Winfoprintf("0 regionral loaded.");
	return;	
   }

   if(nregions != count) Winfoprintf("Warning: number of regions does not match %d %d",nregions,count);

   regionFlag = 1;
   if(show) frame->displayTop();
}

// save BC points to bcfrq bcamp parameters
void AspDisRegion::saveBCfrq(spAspFrame_t frame) {
    
   if(frame == nullAspFrame) return;

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return;

   execString("bcfrq=0\n");
   execString("bcamp=0\n");

   AspRegionList *regionList = getRegionList();
   AspRegionMap *regionMap = regionList->getRegionMap();
   AspRegionMap::iterator itr;
   spAspRegion_t region;
   int count=0;
   for (itr = regionMap->begin(); itr != regionMap->end(); ++itr) {
        region = itr->second;
        AspUtil::setReal("bcfrq", count+1, region->sCoord[0].x,false);
        AspUtil::setReal("bcamp", count+1, region->sCoord[0].y,false);
	count++;
   }
}

// load and display BC points from bcfrq parameter
void AspDisRegion::loadBCfrq(spAspFrame_t frame) {
    
   if(frame == nullAspFrame) return;

   double freq, amp;
   int nregions = AspUtil::getParSize("bcfrq");
   if(nregions < 2) {
	Winfoprintf("%d BC points are defined by bcfrq.",nregions);
	return;
   }

   AspRegionList *regionList = getRegionList();
   regionList->clearList();

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return;

   int maxpts = trace->getTotalpts();
   if(maxpts<1) return;

   int i;
   for(i=0;i<nregions;i++) {
	freq=AspUtil::getReal("bcfrq",i+1,0.0); // bcfrq in ppm 
	amp=AspUtil::getReal("bcamp",i+1,0.0); // bcamp in abs intensity 
        spAspRegion_t region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
        region->dataID = trace->getKeyInd();
        regionList->addRegion(region->index, region);	
   }
   calcBCModel(trace);

   regionFlag = 1;
   frame->displayTop();
}

#define BASESTEP 20
spAspRegion_t AspDisRegion::createRegion(spAspFrame_t frame, int x,int y,int prevX, int prevY) {
   if(frame == nullAspFrame) return nullAspRegion;
   spAspCell_t cell = frame->selectCell(x,y);
   if(cell == nullAspCell) return nullAspRegion;

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return nullAspRegion;

   // determine amp based on trace
   float *traceData = trace->getFirstDataPtr();
   if(!traceData) return nullAspRegion;

   int maxpts = trace->getTotalpts();
   if(maxpts<1) return nullAspRegion;

   AspRegionList *regionList = getRegionList();
   
   int step, nstep;
   if(x<prevX) { // make sure x > prevX
	step=prevX;
	prevX=x;
	x=step;
   }

   double freq,amp;
   int pos;
   spAspRegion_t region;
   step = x-prevX;
   nstep = step/BASESTEP; 
   if(step<(BASESTEP/2)) { // one point
      freq = cell->pix2val(HORIZ,0.5*(x+prevX)); 
      amp = cell->pix2val(VERT,0.5*(y+prevY)); // amp from y pixel position
      region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
      region->dataID = trace->getKeyInd();
      regionList->addRegion(region->index, region);	
   } else {

      if(nstep < 1) nstep = 1;
   // devide into nsteps and add average point for each step 
	step = step/nstep; 

      double freq1,freq2,xsum,ysum;
      int p1,p2,i,j;
      for(i=0; i<nstep; i++) {
        freq1 = cell->pix2val(HORIZ,prevX+i*step);  
        freq2 = cell->pix2val(HORIZ,prevX+(i+1)*step);  
        p1 = trace->val2dpt(freq1);
        p2 = trace->val2dpt(freq2);
        if(p1==p2) continue;

        xsum=ysum=0;
        for(j=p1;j<p2;j++) {
	  xsum += j; 
	  ysum += traceData[j]; 
        }
        pos = (int)(xsum/(double)(p2-p1)); 
	amp = ysum/(double)(p2-p1);
	freq=trace->dpt2val(pos);	
        region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
        region->dataID = trace->getKeyInd();
        regionList->addRegion(region->index, region);	
      }
   }

   calcBCModel(trace);
   
   regionFlag = 1;
   frame->displayTop();

   //region->select(cell,x,y);
   return nullAspRegion; // not used
}

void AspDisRegion::modifyRegion(spAspFrame_t frame, spAspRegion_t region, int x, int y, int prevX, int prevY, int mask) {
   if(region == nullAspRegion) return;
   if(frame == nullAspFrame) return;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return;

   region->modify(cell,x,y,prevX,prevY); 

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return;

   calcBCModel(trace);

   frame->displayTop();
}

void AspDisRegion::deleteRegion(spAspFrame_t frame, spAspRegion_t region) {
   AspRegionList *regionList = getRegionList();
   if(regionList == NULL) return;

   if(region == nullAspRegion) regionList->deleteRegion(); 
   else regionList->deleteRegion(region->index); 
   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return;

   calcBCModel(trace);
   frame->displayTop();
}

spAspRegion_t AspDisRegion::selectRegion(spAspFrame_t frame, int x, int y) {
   if(frame == nullAspFrame) return nullAspRegion;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return nullAspRegion;
   if(getRegionList()->getSize() < 1) return nullAspRegion;

   bool changeFlag=false;
   
   spAspRegion_t region = getRegionList()->selectRegion(cell,x,y,changeFlag);

   if(changeFlag) frame->displayTop();

   return region;
}

// called by draw/redraw after cells are ccreated and filled with data.
void AspDisRegion::display(spAspFrame_t frame) {
   if(regionFlag == 0) return;
   if(frame == nullAspFrame) return;

   //if(getRegionList()->getSize() < 1) return;

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;

   // for now only display it in first cell
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;

   getRegionList()->display(cell, frame->getSelTraceList(), dataInfo);
}

void AspDisRegion::calcBCModel(spAspTrace_t trace) {

   if(trace == nullAspTrace) return;

   AspRegionList *regionList = getRegionList();

   int bcflag = (int)AspUtil::getReal("bcmode",1,1);
   if(bcflag < 1) {
	calcBCModel_CWT(trace);
	return;
   }
   
   int npts = trace->getTotalpts();
   float *model = new float[npts];
   float *bcMask = new float[npts];
   double smoothness = AspUtil::getReal("bcmode",7,10);
   //double smoothness = 1.0;

   // input model is so called weight spectral data (non-zero only if mask=1),
   // but non-zero values are baseline point sCoord[0].y, not spectrum data.
   // output model is smoothed baseline model
   regionList->getBCMask(trace, model, bcMask, npts);

   if(bcflag == 1 || bcflag == 2) {
        float *data = trace->getFirstDataPtr();
   	if(calcBCFit(data, model, bcMask,npts, regionList->getSize())) {
   	   trace->setBCModel(model,npts);
	   delete[] model;
   	   delete[] bcMask;
	   return;
	}
   } 

   BaselineFit(model,npts,bcMask,smoothness);
   trace->setBCModel(model,npts);
   delete[] model;
   delete[] bcMask;
} 

void AspDisRegion::calcBaseMask(spAspFrame_t frame) {
   if(frame == nullAspFrame) return;

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return;

   AspRegionList *regionList = getRegionList();

   int npts = trace->getTotalpts();
   float *data = trace->getFirstDataPtr();
   float *baseMask = new float[npts];
   int aScale = (int)AspUtil::getReal("bcmode",6,10);
   BaseMask(data,baseMask,npts,aScale);

   // fill regionList
   regionList->clearList();
//   bool base=false;
   double freq, amp;
   int j;
   for(int i=0; i<npts; i++) {
      j=i+1;
      if(j==npts) j=i;
/*
      if((baseMask[i] > 0 && !base ) || 
	 (baseMask[i] > 0 && i==j) || 
	 (baseMask[i] > 0 && baseMask[j]==0)) {
*/
      if(baseMask[i] > 0) {
	freq=trace->dpt2val(i);	
        int pos = trace->val2dpt(freq);
        amp = (*(data+pos));
        spAspRegion_t region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
        region->dataID = trace->getKeyInd();
        regionList->addRegion(region->index, region);	
//	base=true;
      }
//      if(baseMask[i] ==0) base=false;
   }

   calcBCModel(trace);
   
   delete[] baseMask;
   regionFlag = 1;
   frame->displayTop();
}

// auto generate base points
// divide spectrum data into bcpts regions and find a baseline point
// in each region that best represent that region 
void AspDisRegion::basePoints(spAspFrame_t frame, int bcpts) {
   if(frame == nullAspFrame) return;

   spAspTrace_t trace = frame->getDefaultTrace(); 
   if(trace == nullAspTrace) return;

   if(bcpts<2) bcpts=2; // at least two points
   if(bcpts>200) bcpts=200; 

   int npts = trace->getTotalpts();
   float *data = trace->getFirstDataPtr();

   // average of np points at left end of spectrum
   // note, it is average of fabs(data[i]-data[i+1]).
   double noise=0;
   int np=npts/100;
   if(np < 1) return;
   for(int i=np; i<(np+np); i++) {
	noise += fabs(data[i]-data[i+1]);
   } 
   noise /= (double)np;

   // calc base mask
   AspRegionList *regionList = getRegionList();
   regionList->clearList();
   float *baseMask = new float[npts];
   int aScale = (int)AspUtil::getReal("bcmode",6,10);
   BaseMask(data,baseMask,npts,aScale);

   // fill regionList
   double freq, amp, xsum,ysum;
   double sum1,sum2;
   int k,k1,pos;
   int first=0,last=0;
   int step = npts/bcpts; // data points in a region 
   int minpts = step /2; // min none zero baseMask point in a region 
   spAspRegion_t region;
   for(int i=0; i<bcpts; i++) {
      xsum=ysum=0;
      sum1=sum2=0;
      k1=0;
      for(int j=0; j<step-1; j++) {
        k = i*step+j;
	sum1 += fabs(data[k]);
        if(baseMask[k] > 0) {
	  if(first<1) first=k; // first point
	  if(i==(bcpts-1)) last=k; //last point
	  xsum += k;
	  ysum += data[k]; 
	  sum2 += fabs(data[k]);
	  k1++;
        }
      }
      if(k1<minpts) continue;
      sum1 /= (double)step;
      sum2 /= (double)k1;
      if(sum1 > (2*sum2)) continue;
      pos=(int)(xsum/(double)k1);
      amp=ysum/(double)k1;
	if(fabs(data[pos] - amp) > 5*noise) continue;

	freq=trace->dpt2val(pos);	
        region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
        region->dataID = trace->getKeyInd();
        regionList->addRegion(region->index, region);
   }

   if(first>0) {
	freq=trace->dpt2val(first);        
        amp = data[first];
        region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
        region->dataID = trace->getKeyInd();
        regionList->addRegion(region->index, region);
   }
   if(last>0) {
	freq=trace->dpt2val(last);        
        amp = data[last];
        region = spAspRegion_t(new AspRegion(regionList->getSize(),freq,amp));
        region->dataID = trace->getKeyInd();
        regionList->addRegion(region->index, region);
   }

   calcBCModel(trace);
   delete[] baseMask;

   regionFlag = 1;
   frame->displayTop();
}

// model is calculated based on "default trace", that is the first selected trace
void AspDisRegion::doBC(spAspFrame_t frame) {
    if(frame == nullAspFrame) return;

    spAspTrace_t trace = frame->getDefaultTrace(); 
    if(trace == nullAspTrace) return;

    int npts = trace->getTotalpts();
    if(npts < 1) return;

    float *model = trace->getBCModel();
    int flag = (int)AspUtil::getReal("bcmode",1,0);
    if(model == NULL && flag > 0) { // model not calculated yet 
        if(getRegionList()->getSize() < 1) { // calculate base points
      	   loadBCfrq(frame);
	   if(getRegionList()->getSize() < 1) {
		int n = (int)AspUtil::getReal("bcmode",5, 50);
		basePoints(frame, n);
	   }
	}
	calcBCModel(trace); // calculated model
    	model = trace->getBCModel();
    } else if(model == NULL) { // do CWT and Whittaker modeling
	calcBCModel_CWT(trace);
    	model = trace->getBCModel();
    }

    if(model == NULL) return;

    saveBCfrq(frame);

    int allTrace = (int)AspUtil::getReal("bcmode",8, 0);
    if(allTrace) { // apply to model to all traces
	AspTraceMap::iterator itr;
	AspTraceMap *traceMap = frame->getSelTraceList()->getTraceMap();
        for (itr = traceMap->begin(); itr != traceMap->end(); ++itr) {
	   if(itr->second->getTotalpts() == npts) {
   	   	itr->second->setBCModel(model,npts);
           	itr->second->doBC();
	   }
	}
     } else trace->doBC(); // apply to default trace only
}

void AspDisRegion::undoBC(spAspFrame_t frame) {
	AspTraceMap::iterator itr;
	AspTraceMap *traceMap = frame->getSelTraceList()->getTraceMap();
        for (itr = traceMap->begin(); itr != traceMap->end(); ++itr) {
           itr->second->undoBC();
	}
}

void AspDisRegion::calcBCModel_CWT(spAspTrace_t trace) {

   if(trace == nullAspTrace) return;

   int npts = trace->getTotalpts();
   float *data = trace->getFirstDataPtr();
   float *model = new float[npts];
   float *bcMask = new float[npts];
   double scale = AspUtil::getReal("bcmode",2,10);
   double smoothness = AspUtil::getReal("bcmode",3,1000);
   BaseMask(data,bcMask,npts,(int)scale);

   memcpy(model,data,npts*sizeof(float) );
   for(int i=0; i<npts; i++) {
	model[i] *= bcMask[i];
   }
   BaselineFit(model,npts,bcMask,smoothness);
   trace->setBCModel(model,npts);
   delete[] model;
   delete[] bcMask;
}
