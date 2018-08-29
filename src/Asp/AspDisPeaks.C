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
#include "AspDisPeaks.h"
#include "AspMouse.h"
#include "AspFrameMgr.h"
#include "AspDis1D.h"

#define MAXPEAKS 100

extern "C" {
int pickLines(double noisemult, int pos, float *specData, int fpts, int npts, 
	double thresh, double vScale, double *lineFrq, double *lineamp, int maxLines,double sw, double rflrfp, int fn);
}

double AspDisPeaks::noisemult = 3;
int AspDisPeaks::posPeaks = 0;

int AspDisPeaks::aspPeaks(int argc, char *argv[], int retc, char *retv[]) {

    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();

    if(frame == nullAspFrame) RETURN;

    if(argc==1 && retc > 0) { // quary display info
	if(frame->getPeakFlag() == 0) retv[0]=realString(0.0);
	else retv[0]=realString(1.0);
	if(retc>1) retv[1]=realString((double)(frame->getPeakList()->getSize()));
	RETURN;
    }

    spAspDataInfo_t dataInfo = frame->getDefaultDataInfo(true);
    if(dataInfo == nullAspData) RETURN;

   if(argc>1 && strcasecmp(argv[1],"nll") ==0) {
        nll(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"save") == 0) {
        char path[MAXSTR2];
        if(argc >2) strcpy(path,argv[2]);
        else sprintf(path,"%s/datdir/peaks",curexpdir); 

        save(frame,path);
   } else if(argc>1 && strcasecmp(argv[1],"load") == 0) {
        char path[MAXSTR2];
        if(argc >2) strcpy(path,argv[2]);
        else sprintf(path,"%s/datdir/peaks",curexpdir); 

        load(frame,path,true);
   } else if(argc>1 && strcasecmp(argv[1],"reset") == 0) {
	frame->getPeakList()->resetLabels();
        frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"delete") == 0) {
	 deletePeak(frame, nullAspPeak);
   } else if(argc>1 && strcasecmp(argv[1],"clear") == 0) {
         frame->getPeakList()->clearList();
   	 frame->initPeakFlag(0);
   	 frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"dpf") == 0) {
	dpf(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"vert") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_VERT,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_VERT) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"nolink") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_NOLINK,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_NOLINK) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"value") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_VALUE,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_VALUE) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"name") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_NAME,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_NAME) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"top") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_TOP,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_TOP) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"mark") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_MARKING,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_MARKING) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>1 && strcasecmp(argv[1],"short") == 0) {
	if(argc>2) frame->setPeakFlag(PEAK_SHORT,(atoi(argv[2])>0));
	if(retc>0) {
           int value;
           if(frame->getPeakFlag() & PEAK_SHORT) value = 1;
           else value=0;
           retv[0] = realString((double)value);
	} else frame->displayTop();
   } else if(argc>2 && strcasecmp(argv[1],"label") == 0) {
	int ind=atoi(argv[2]);
	spAspPeak_t peak = frame->getPeakList()->getPeak(ind);
	if(peak != nullAspPeak) {
	   if(argc>3 && strlen(argv[3])>0) peak->setLabel(string(argv[3]));
	   if(retc>0) retv[0] = newString(peak->getLabel().c_str());
	   else frame->displayTop();
	}
   } else if(argc>1 && strcasecmp(argv[1],"noisemult") == 0) {
	if(argc>2) noisemult = atof(argv[2]); 
	if(retc>0) retv[0] = realString(noisemult);
   } else if(argc>1 && strcasecmp(argv[1],"posPeaks") == 0) {
	if(argc>2) posPeaks = atoi(argv[2]); 
	if(retc>0) retv[0] = realString(posPeaks);
   }

   frame->updateMenu();

   RETURN;
}

void AspDisPeaks::save(spAspFrame_t frame, char *path) {
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
        Winfoprintf("Failed to create peak file %s.",path);
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

   AspPeakList *peakList = frame->getPeakList();
   int npeaks = peakList->getSize();

   fprintf(fp,"# Created by %s on %s at machine %s.\n",UserName,datetim,HostName);
   fprintf(fp,"number_of_peaks: %d\n",npeaks);
   fprintf(fp,"# index,rank,nucname,asignedName,ppm,peakLabel,labelX,labelY,peak height,integral,amplitude,dataID\n");

   int i;
   spAspPeak_t peak;
   for(i=0;i<npeaks;i++) {
	peak = peakList->getPeak(i);
	if(peak != nullAspPeak) { 
	   fprintf(fp,"%s\n",peak->toString().c_str());
	}
   }

   fclose(fp);
}

void AspDisPeaks::load(spAspFrame_t frame, char *path, bool show) {

   struct stat fstat;
   if(stat(path, &fstat) != 0) {
        Winfoprintf("Error: cannot find %s.",path);
        return;
   }

   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;

   FILE *fp;
   if(!(fp = fopen(path, "r"))) {
        Winfoprintf("Failed to open session file %s.",path);
        return;
   }

   char  buf[MAXSTR], words[MAXWORDNUM][MAXSTR], *tokptr;
   int nw=0;

   AspPeakList *peakList = frame->getPeakList();
   peakList->clearList();

   int npeaks=0;
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
      if(strcasecmp(words[0],"number_of_peaks:")==0) {
	npeaks = atoi(words[1]); 
      } else if(strcasecmp(words[0],"???")==0 && nw>10) {
      } else if(npeaks > 0 && nw>7) {
	spAspPeak_t peak = spAspPeak_t(new AspPeak(words,nw));
	peakList->addPeak(peak->getIndex(), peak);	
	count++;
      }
   }
   fclose(fp);

   if(count == 0) { 
	Winfoprintf("0 peak loaded.");
	return;	
   }

   if(npeaks != count) Winfoprintf("Warning: number of peaks does not match %d %d",npeaks,count);

   if(!frame->getPeakFlag()) frame->initPeakFlag(PEAK_VALUE | PEAK_VERT);
   if(show) frame->displayTop();
}

// auto pick above threshold (similar to nll)
void AspDisPeaks::nll(spAspFrame_t frame, int argc, char *argv[]) {
   
   posPeaks = 0;
   if (argc>2 && isdigit(argv[2][0])) noisemult=atof(argv[2]);
   else if (argc>2 && strcmp(argv[2],"pos") == 0) posPeaks = 1;
   if (argc>3 && isdigit(argv[3][0])) noisemult=atof(argv[3]);
   else if (argc>3 && strcmp(argv[3],"pos") == 0) posPeaks = 1;

   peakPicking(frame,0,0,0,0);
}

void AspDisPeaks::dpf(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   argc--;
   argv++;
   if(argc>1) {
      frame->initPeakFlag(0);
      frame->getPeakList()->resetLabels();
      if(strstr(argv[1],"top") != NULL) {
	frame->setPeakFlag(PEAK_TOP,true);
	if(strstr(argv[1],"name") != NULL) frame->setPeakFlag(PEAK_NAME,true);
	else frame->setPeakFlag(PEAK_VALUE,true);
      }
      if(strstr(argv[1],"horiz") != NULL) {
	frame->setPeakFlag(PEAK_HORIZ,true);
	if(strstr(argv[1],"name") != NULL) frame->setPeakFlag(PEAK_NAME,true);
	else frame->setPeakFlag(PEAK_VALUE,true);
      }
      if(strstr(argv[1],"vert") != NULL) {
	frame->setPeakFlag(PEAK_VERT,true);
	if(strstr(argv[1],"name") != NULL) frame->setPeakFlag(PEAK_NAME,true);
	else frame->setPeakFlag(PEAK_VALUE,true);
      }
      if(strstr(argv[1],"auto") != NULL) {
	frame->setPeakFlag(PEAK_AUTO,true);
	if(strstr(argv[1],"name") != NULL) frame->setPeakFlag(PEAK_NAME,true);
	else frame->setPeakFlag(PEAK_VALUE,true);
      }
      if(strstr(argv[1],"name") != NULL) frame->setPeakFlag(PEAK_NAME,true);
      if(strstr(argv[1],"value") != NULL) frame->setPeakFlag(PEAK_VALUE,true);
      if(strstr(argv[1],"nolink") != NULL) frame->setPeakFlag(PEAK_NOLINK,true);
      if(strstr(argv[1],"mark") != NULL) frame->setPeakFlag(PEAK_MARKING,true);
      if(strstr(argv[1],"short") != NULL) frame->setPeakFlag(PEAK_SHORT,true);
   }
   
   int npeaks = frame->getPeakList()->getSize();
   if(!npeaks) {
	Winfoprintf("0 peak is picked or loaded.");
	return;
   }

   frame->displayTop();
}

// this is called when mouse released from dragging rubber band 
// threshold th will be used if x=y=prevX=prevY=0
void AspDisPeaks::peakPicking(spAspFrame_t frame, int x, int y, int prevX, int prevY) {

    if(frame == nullAspFrame) return; 
    spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
    if(dataInfo == nullAspData) return;
    spAspCell_t cell;
    if(x > 0) cell = frame->selectCell(x,y);
    else {
	cell = frame->getFirstCell();
    }
    if(cell == nullAspCell) return;

    if(frame->getSelTraceList()->getSize() < 1) {
	frame->getDefaultTrace();
    }

    list<spAspTrace_t>::iterator itr;
    list<spAspTrace_t> *traceList = frame->getSelTraceList()->getTraceList();
    if(traceList->size() < 1) return;

    AspPeakList *peakList = frame->getPeakList();
    int count=peakList->getSize();
    double voff = cell->getCali(VERT)*(dataInfo->getVoff());

    bool changeFlag;
    if(x > 0 && x==prevX && y==prevY) { // put a peak at x,y and return
        spAspTrace_t trace = frame->getSelTraceList()->selectTrace(cell,x, y,changeFlag);
        if(trace != nullAspTrace) {
	    double freq=cell->pix2val(HORIZ,x);
	    spAspCursor_t cursor= spAspCursor_t(new AspCursor(freq, dataInfo->haxis.name ));
  	    spAspPeak_t peak = spAspPeak_t(new AspPeak(count,cursor));
	    peak->dataID = string(trace->getKeyInd());
	    peak->setHeight(cell->pix2val(VERT,y+ voff*(trace->vp))); 
	    peakList->addPeak(count, peak);
	    trace->selected=false;
            if(!frame->getPeakFlag()) frame->initPeakFlag(PEAK_VALUE | PEAK_VERT);
            frame->displayTop();
	}
        return;
    }

    // do peak picking
    double vstx,vwd,vsty,vht,vsty2;
    double vScale,vpos,th0;
    vScale = dataInfo->getVscale();
    voff = dataInfo->getVoff();
    vpos = dataInfo->getVpos();
    bool box;
    if(x==0 && prevX==0 && y==0 && prevY==0) { // pick above threshold
      box=false;
      cell->getValCell(vstx,vsty,vwd,vht);
      vsty2 = vsty-vht;
      if (P_getreal(CURRENT, "th", &th0, 1)) th0=0.0;
      th0 += vpos;
    } else { // pick by x,y,prevX,prevY
      box=true;
      // make sure prevX<x and prevY<y
      if(prevX>x) {
	int tmp=x;
	x=prevX;
	prevX=tmp;
      }
      if(prevY>y) {
	int tmp=y;
	y=prevY;
	prevY=tmp;
      }

      // convert mouse position to ppm values 
      vstx = cell->pix2val(HORIZ,prevX);
      vwd = vstx - cell->pix2val(HORIZ,x);
      vsty = cell->pix2val(VERT,prevY);
      vsty2 = cell->pix2val(VERT,y);

      // define threshold by y
      double px,py,pw,ph;
      cell->getPixCell(px,py,pw,ph);
      th0 = (py+ph-y)/cell->getCali(VERT);
   }

    double *peakFrq = new double[MAXPEAKS];
    double *peakAmp = new double[MAXPEAKS];

   double reffrq = dataInfo->haxis.scale; // in MHz
   double rflrfp = dataInfo->haxis.minfirst; // in ppm 

    spAspTrace_t trace;
    for(itr=traceList->begin(); itr != traceList->end();  ++itr) {
	trace = *itr;
        string dataID=trace->getKeyInd();
	float *data = trace->getFirstDataPtr();
        if(!data) continue;
	double min = trace->getMminX();
        double max = trace->getMmaxX();
	double shift = trace->getShift();
	double wd = max-min;
	int tnpts = trace->getTotalpts();
 	int fptr = (int)(1.0+(max+shift-vstx)*tnpts/wd); // add 1 to make sure fptr is in box
	int npts = (int)(0.5+tnpts*vwd/wd);
        if(npts < 3) { // need at least 3 points to find max
	  npts = 3;
	  if(*(data+fptr) > *(data+fptr+1)) {
	    if(fptr>0) fptr--;
	  }
	}
	rflrfp = min;
	// adjust thresh for vpos, voff
        //double thresh = th0 - (vpos + voff*(trace->vp));
        double thresh = th0-vpos;
        double ymin,ymax;
 	double yoff = voff*(trace->vp)*cell->getCali(VERT);
        if(box) {
	   if((thresh+y-prevY) < voff*(trace->vp)) continue;
	   thresh -= voff*(trace->vp);
	   ymin = cell->pix2val(VERT, y + yoff);
	   ymax = cell->pix2val(VERT,prevY+yoff);
	} else {
	   ymin = vsty2;
	   ymax = vsty;
	}

        int npeaks = pickLines(noisemult, posPeaks, data, fptr, npts, thresh, 
		vScale, peakFrq, peakAmp, MAXPEAKS, wd*reffrq, -rflrfp*reffrq, 2*tnpts);
      int i;
      double freq,amp;
      rflrfp += shift;
      for(i=0;i<npeaks;i++) {
	amp=peakAmp[i];	
//Winfoprintf("npeaks,i,ymin,ymax,amp %d %d %f %f %f",npeaks,i,ymin,ymax,amp);
	if(!box || (amp>=ymin && amp<=ymax)) { // only include peaks within the rubber box
	  freq=peakFrq[i] / reffrq + rflrfp; // llfrq is in Hz 
	  spAspCursor_t cursor= spAspCursor_t(new AspCursor(freq, dataInfo->haxis.name ));
  	  spAspPeak_t peak = spAspPeak_t(new AspPeak(count,cursor));
	  peak->setHeight(amp);
	  peak->dataID = string(dataID);
	  peakList->addPeak(count, peak);
	  count++;
	}
      }

     }

   delete[] peakFrq;
   delete[] peakAmp;

   if(!frame->getPeakFlag()) frame->initPeakFlag(PEAK_VALUE | PEAK_VERT);
   frame->displayTop();
}

void AspDisPeaks::modifyPeak(spAspFrame_t frame, spAspPeak_t peak, int x, int y) {
   if(peak == nullAspPeak) return;
   if(frame == nullAspFrame) return;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return;

   peak->modify(cell,x,y); 
   frame->displayTop();
}

void AspDisPeaks::deletePeak(spAspFrame_t frame, spAspPeak_t peak) {
   AspPeakList *peakList = frame->getPeakList();
   if(peakList == NULL) return;

   if(peak == nullAspPeak) peakList->deletePeak(); 
   else peakList->deletePeak(peak->getIndex()); 
   frame->displayTop();
}

spAspPeak_t AspDisPeaks::selectPeak(spAspFrame_t frame, int x, int y) {
//Winfoprintf("selectPeak");
   if(frame == nullAspFrame) return nullAspPeak;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return nullAspPeak;
   if(frame->getPeakList()->getSize() < 1) return nullAspPeak;

   bool changeFlag=false;
   
   spAspPeak_t peak = frame->getPeakList()->selectPeak(x,y,changeFlag);

   if(changeFlag) frame->displayTop();

   return peak;
}

// called by draw/redraw after cells are ccreated and filled with data.
void AspDisPeaks::display(spAspFrame_t frame) {
   if(frame == nullAspFrame) return;

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;

   int peakFlag = frame->getPeakFlag();
   int specFlag = frame->getSpecFlag();
   if(peakFlag == 0) return;

   // for now only display it in first cell
   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) return;

   frame->getPeakList()->display(cell, frame->getSelTraceList(), dataInfo, peakFlag, specFlag);
}


