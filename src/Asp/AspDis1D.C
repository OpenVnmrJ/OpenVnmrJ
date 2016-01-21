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
#include "AspDis1D.h"
#include "AspFrameMgr.h"
#include "aipSpecData.h"
#include "aipSpecDataMgr.h"
#include "AspMouse.h"
#include "AspDisPeaks.h"

// aspDs('nxm',...)
// aspDs('dssh',...)
// aspDs('dss',...)
// aspDs('all',...)
int AspDis1D::asp1D(int argc, char *argv[], int retc, char *retv[]) {

    spAspFrame_t frame = AspFrameMgr::get()->getCurrentFrame();

    if(frame == nullAspFrame) RETURN;

    int disFlag = frame->getDisFlag();

    if(retc > 0) { // quary display info
	if(argc>1 && strcasecmp(argv[1],"fdf")==0) {
  	   string nucleus = AspUtil::getString("tn","H1");
	   int n=SpecDataMgr::get()->getNtraces(nucleus);
	   if(n>0) retv[0]=realString(1.0);
	   else retv[0]=realString(0.0);
 	   if(retc>1) retv[1]=realString((double)n);
	} else if(argc>1 && strcasecmp(argv[1],"traces")==0) {
	   int n = frame->getTraceList()->getSize(); // total traces
	   retv[0]=realString((double)n);
	   if(retc > 1) {	// displayed traces
	     n=frame->getSelTraceList()->getSize();
	     retv[1]=realString((double)n);
	   }
	   if(retc > 2) {
	       retv[2]=newString(frame->getSelTraceList()->getIndList().c_str());       
	   }
	} else if(argc>2 && strcasecmp(argv[1],"path")==0) {
	   int ind = atoi(argv[2])-1;
	   spAspTrace_t trace = frame->getTraceList()->getTraceByInd(ind);
	   if(trace != nullAspTrace) retv[0]=newString(trace->path.c_str());
	} else if(argc>2 && strcasecmp(argv[1],"key")==0) {
	   int ind = atoi(argv[2])-1;
	   spAspTrace_t trace = frame->getTraceList()->getTraceByInd(ind);
	   if(trace != nullAspTrace) {
	     char str[MAXSTR];
	     sprintf(str,"%s:%d",trace->dataKey.c_str(),trace->dataInd);
	     retv[0]=newString(str);
	   }
	} else if(argc>1 && strcasecmp(argv[1],"pix")==0 && retc > 3) {  
	   spAspCell_t cell = frame->getFirstCell();
	   if(cell != nullAspCell) {
		double x,y,w,h;
		cell->getPixCell(x,y,w,h);
		retv[0]=realString(x);
		retv[1]=realString(y);
		retv[2]=realString(w);
		retv[3]=realString(h);
	   }
	} else if(argc>1 && strcasecmp(argv[1],"val")==0 && retc > 3) {  
	   spAspCell_t cell = frame->getFirstCell();
	   if(cell != nullAspCell) {
		double x,y,w,h;
		cell->getValCell(x,y,w,h);
		retv[0]=realString(x);
		retv[1]=realString(y);
		retv[2]=realString(w);
		retv[3]=realString(h);
	   }
	} else if(argc>1 && strcasecmp(argv[1],"px2v")==0 && argc>2) { // horiz 
	   spAspCell_t cell = frame->getFirstCell();
	   if(cell != nullAspCell) {
		retv[0]=realString(cell->pix2val(HORIZ,atof(argv[2])));
	   }
	} else if(argc>1 && strcasecmp(argv[1],"py2v")==0 && argc>2) { // vertical
	   spAspCell_t cell = frame->getFirstCell();
	   if(cell != nullAspCell) {
		retv[0]=realString(cell->pix2val(VERT,atof(argv[2])));
	   }
	} else {
	   int flag = frame->getSpecFlag();
	   if((flag & SPEC_GRID) && frame->rows>1) retv[0]=newString("nxm");
	   else if(flag & SPEC_GRID) retv[0]=newString("dssh");
	   else if(flag & SPEC_DSS) retv[0]=newString("dss");
	   else if(flag & SPEC_DS) retv[0]=newString("ds");
	   else retv[0]=newString("");
	   if(retc>1) 
	      retv[1]=realString(frame->getSelTraceList()->getSize());
	   
	   if(retc>3) {
	      retv[2]=realString(frame->rows);
	      retv[3]=realString(frame->cols);
	   } else if(retc>2) {
	      retv[2]=realString(frame->currentTrace);
	   }
	}

	if(argc>1 && strcasecmp(argv[1],"thresh")==0) {
	   if(disFlag & SPEC_THRESH) retv[0]=realString(1.0);
	   else retv[0]=realString(0.0);
	} else if(argc>1 && strcasecmp(argv[1],"scale")==0) {
           int flag = frame->getAxisFlag();
	   if(flag & AX_WEST) retv[0]=newString("vscale");
	   else if(flag & AX_SOUTH) retv[0]=newString("dscale");
	   else retv[0]=newString("nscale");
	} else if(argc>1 && strcasecmp(argv[1],"label")==0) {
	   if(disFlag & SPEC_LABEL) retv[0]=realString(1.0);
	   else retv[0]=realString(0.0);
	} else if(argc>1 && strcasecmp(argv[1],"index")==0) {
	   if(disFlag & SPEC_INDEX) retv[0]=realString(1.0);
	   else retv[0]=realString(0.0);
	} else if(argc>1 && strcasecmp(argv[1],"cursor")==0) {
	   if(AspMouse::getState() == AspMouse::cursor1) 
		retv[0]=realString(1.0);
	   else if(AspMouse::getState() == AspMouse::cursor2) 
		retv[0]=realString(2.0);
	   else retv[0]=realString(0.0);
	} 
	RETURN;
    }

   if(argc>1 && strcasecmp(argv[1],"exit") ==0) {
	if(frame->doDs()) frame->clear();
	frame->getSelTraceList()->clearList();
	frame->getTraceList()->clearList();
	SpecDataMgr::get()->removeData("all");
	AspMouse::setState(AspMouse::noState);
	frame->threshSelected = false;
	frame->currentTrace=0;
	frame->rows=frame->cols=1;
	execString("menu('main')\n");
   	RETURN;
   }

   // before getDefaultDataInfo, clear ds etc
   Wsetgraphicsdisplay("asp1D('')"); 

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo(true);
   if(dataInfo == nullAspData) RETURN;

   if(argc>2 && strcasecmp(argv[1],"setColor") ==0) {
	int ntraces = frame->getTraceList()->getSize();
	int first = 0;
	int last = ntraces-1;
  	int step = 1;
	if(argc>3 && strstr(argv[2],"-") != NULL) {
           AspUtil::getFirstLastStep(argv[2], ntraces, first, last, step);
	   argc--;
	   argv++;
	} else if(argc>3) {
	   first=last=atoi(argv[2])-1;
	   argc--;
	   argv++;
	}
        for(int i=first;i<=last;i+=step) {
	     spAspTrace_t trace = frame->getTraceList()->getTraceByInd(i);
	     if(trace != nullAspTrace) trace->colorStr = string(argv[2]);
	}
   	RETURN;
   
   } else if(argc>1 && strcasecmp(argv[1],"dscale") ==0) {
	if(!frame->doDs()) RETURN;
	if(frame->getAxisFlag() & AX_WEST) {
	   frame->pstx -= (6*xcharpixels);
	   frame->pwd += (6*xcharpixels);
	}
        frame->initAxisFlag(AX_BOX | AX_SOUTH);
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"vscale") ==0) {
	if(!frame->doDs()) RETURN;
        frame->initAxisFlag(AX_BOX | AX_SOUTH | AX_WEST);
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"nscale") ==0) {
	if(!frame->doDs()) RETURN;
	if(frame->getAxisFlag() & AX_WEST) {
	   frame->pstx -= (6*xcharpixels);
	   frame->pwd += (6*xcharpixels);
	}
        frame->setAxisFlag(AX_WEST,false);
        frame->setAxisFlag(AX_SOUTH,false);
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"full") ==0) { // full size
	if(!frame->doDs()) RETURN;
	frame->setFullSize();
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"again") ==0) {
	if(frame->doDs()) frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"expand") ==0) {
	if(frame->doDs()) dsExpand(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"f") ==0) { // full zoom out
	if(!frame->doDs()) RETURN;
   	double scale = dataInfo->haxis.scale;
	double c1 = frame->getSelTraceList()->getMaxX();
	double c2 = frame->getSelTraceList()->getMinX();
   	setSpecRegion(c1,c2,scale);
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"phase") ==0) {
        if(SpecDataMgr::get()->getSpecDataMap()->size()>0) {
	   Winfoprintf("Cannot phase fdf spectra.");
	   RETURN; 
	}
	if(!frame->doDs()) RETURN;
	AspMouse::setState(AspMouse::phasing);
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"cursor") ==0) {
	if(!frame->doDs()) RETURN;
	if(AspMouse::getState() == AspMouse::cursor1) {
	  AspMouse::setState(AspMouse::cursor2);
	} else {
	  AspMouse::setState(AspMouse::cursor1);
	}
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"thresh") ==0) {
	if(!frame->doDs()) RETURN;
	if(!(disFlag & SPEC_THRESH)) frame->setDisFlag(SPEC_THRESH, true);
	else frame->setDisFlag(SPEC_THRESH, false);
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"next") == 0) {
        if(frame->doDs()) {
   	     string nucleus = dataInfo->haxis.name;
	     int n = dataInfo->getNtraces(nucleus);
	     if(frame->currentTrace < (n-1)) {
		frame->currentTrace++;
       		argv++;
       		argc--;
		ds(frame,argc,argv);
	     }
	}
   } else if(argc>1 && strcasecmp(argv[1],"prev") == 0) {
        if(frame->doDs()) {
	     if(frame->currentTrace > 0) {
		frame->currentTrace--;
       		argv++;
       		argc--;
		ds(frame,argc,argv);
	     }
	}
   } else if(argc>4 && strcasecmp(argv[1],"setRange") == 0) {
	int ind = atoi(argv[2])-1;
	double min = atof(argv[3]);	
	double max = atof(argv[4]);	
	if(min>max) {
	  double tmp=max;
	  max=min;
	  min=tmp;
	}
	spAspTrace_t trace = frame->getTraceList()->getTraceByInd(ind);
	if(trace != nullAspTrace) {
	   trace->setMinX(min);
	   trace->setMaxX(max);
	} 
	if(frame->doDs()) frame->draw();
	
   } else if(argc>2 && (strstr(argv[1],"load") == argv[1] ||
		strstr(argv[1],"add") == argv[1] || 
		strstr(argv[1],"sub") == argv[1] ||
		strstr(argv[1],"rep") == argv[1] )) {
	// clear phasefile display if needed.
   	string nucleus = dataInfo->haxis.name;
        if(SpecDataMgr::get()->getNtraces(nucleus) < 1) { 
   	   frame->getSelTraceList()->clearList();
   	   frame->getTraceList()->clearList();
	}

	loadData(frame,argc,argv);
   } else if(argc>1 && strstr(argv[1],"align") == argv[1]) {
	if(argc>2) alignSpec(frame,argv[2]);
	else alignSpec(frame);	

   } else if(argc>2 && strcasecmp(argv[1],"trace") == 0) {
	if(!frame->doDs()) RETURN;
	if(strcasecmp(argv[2],"hide") == 0 && argc>3) {
	   int ind = atoi(argv[3])-1;
	   frame->getSelTraceList()->deleteTrace(ind); 
	} else if(strcasecmp(argv[2],"hide") == 0) {
	   frame->getSelTraceList()->deleteTrace(); 
	} else if(strcasecmp(argv[2],"remove") == 0) {
	   if(argc>3) {
	     int ind = atoi(argv[3])-1;
	     frame->getSelTraceList()->deleteTrace(ind); 
	     frame->getTraceList()->deleteTrace(ind); 
	   } else {
	     frame->getSelTraceList()->deleteTrace(); 
	     frame->getTraceList()->deleteTrace(); 
	   }
/*
	   spAspTrace_t trace;
	   if(argc>3) {
	     int ind = atoi(argv[3])-1;
	     trace = frame->getTraceList()->getTraceByInd(ind);
	   } else trace=frame->getSelTraceList()->getSelTrace();
	   string key = trace->dataKey;
	   // note we delete all traces related to dataKey.
	   SpecDataMgr::get()->removeData((char *)key.c_str());
	   frame->getSelTraceList()->deleteTrace(key); 
	   frame->getTraceList()->deleteTrace(key); 
*/
	}
	frame->draw();

   } else if(argc>1 && strcasecmp(argv[1],"reset") == 0) {
	// currently only reset trace vo
     	frame->getTraceList()->resetVpos(); 
	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"clear") == 0) {
	// clear display and fdf data
        if(!frame->doDs()) {
   	   frame->initAxisFlag(AX_BOX | AX_SOUTH);
   	   frame->initSpecFlag(SPEC_DS);
	}
	SpecDataMgr::get()->removeData("all");
   	frame->getSelTraceList()->clearList();
   	frame->getTraceList()->clearList();
   	frame->getPeakList()->clearList();
	frame->clearAnnotations();
	frame->initPeakFlag(0);
	frame->initDisFlag(0);
   	//frame->initAxisFlag(AX_BOX | AX_SOUTH);
   	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"dindex") == 0) {
	frame->setDisFlag(SPEC_INDEX, true);
//	frame->setDisFlag(SPEC_LABEL, false);
   	frame->getTraceList()->setLabelFlag(frame->getDisFlag());
   	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"dlabel") == 0) {
	frame->setDisFlag(SPEC_LABEL, true);
//	frame->setDisFlag(SPEC_INDEX, false);
	if(argc>2) {
	  argc-=2;
	  argv+=2;
	  frame->getSelTraceList()->setLabels(argc,argv, frame->getDisFlag()); // this also set showLabel flag
	} else 
   	  frame->getTraceList()->setLabelFlag(frame->getDisFlag());
   	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"nlabel") == 0) {
	frame->setDisFlag(SPEC_LABEL, false);
   	frame->getTraceList()->setLabelFlag(frame->getDisFlag());
   	frame->draw();
   } else if(argc>1 && strcasecmp(argv[1],"nindex") == 0) {
	frame->setDisFlag(SPEC_INDEX, false);
   	frame->getTraceList()->setLabelFlag(frame->getDisFlag());
   	frame->draw();
   } else if(argc>1 && (strcasecmp(argv[1],"nxm") ==0)) {
	nxm(frame,argc,argv);
   } else if(argc>1 && isdigit(argv[1][0]) && strstr(argv[1],"x") != NULL) {
	nxm(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"dssh") ==0) {
	dssh(frame,argc,argv);
   } else if(argc>1 && strcasecmp(argv[1],"dss") == 0) {
	dss(frame,argc,argv);
   } else {
	AspMouse::setState(AspMouse::cursor1);
	ds(frame,argc,argv);
   }

   setReal("aspInfo",1,frame->getTraceList()->getSize(),true);
   setReal("aspInfo",2,frame->getSelTraceList()->getSize(),true);
   execString("menu('asp')\n");
   //Wsetgraphicsdisplay("asp1D('again')"); // not used.
   RETURN;
}

void AspDis1D::dsExpand(spAspFrame_t frame, int argc, char *argv[]) {
	if(!frame->doDs()) return;

	double c1,c2,scale=1.0;
	bool zoomin=true;
	P_getreal(CURRENT,"cr",&c1,1);
	P_getreal(CURRENT,"delta",&c2,1);
	c2 = c1-c2;
	if(argc>3) { // assume input is in Hz or in ppm (e.g. 2.7p)
		     // so will be converted to Hz by Magical.
	  c1 = atof(argv[2]); 
	  c2 = atof(argv[3]);
	} else if(argc>2 || AspMouse::getState() != AspMouse::cursor2) { 
	  // by zoom factor
   	  double factor;
	  if(argc>2) { // argv[2] is +/- zoom factor 
   	    factor = atof(argv[2]); 
	  } else {
   	    factor = getReal("zoomfactor", 1, 1.4);
	  }
   	  double vx,vy,vw,vh;
	  spAspCell_t cell = frame->getFirstCell();
	  if(cell == nullAspCell) return;
   	  cell->getValCell(vx,vy,vw,vh);
	  double c = vx - 0.5*vw; // center
	  if(factor > 0) { // zoom in
	    c1 = c + 0.5*vw/factor;	
	    c2 = c - 0.5*vw/factor;
	  } else if(factor < 0) { // zoom out
	    c1 = c + 0.5*vw*factor;	
	    c2 = c - 0.5*vw*factor;	
	    zoomin=false;
	  }
   	  scale = frame->getDefaultDataInfo()->haxis.scale;
	}
	setSpecRegion(c1,c2,scale,zoomin);
        if(zoomin && AspMouse::getState() == AspMouse::cursor2) { 
	  AspMouse::setState(AspMouse::cursor1);
        }
	frame->draw();
}

void AspDis1D::nxm(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   int ntraces;
   if(argc>1 && strcasecmp(argv[1],"nxm") ==0) {
       argv++;
       argc--;
       ntraces = mspec(frame,argc,argv);
       if(ntraces < 1) return;

       // auto layout
       double d = sqrt(ntraces);
       int rows = (int) (d*(frame->pixht)/(frame->pixwd) + 0.5);
       int cols = (int) (d*(frame->pixwd)/(frame->pixht) + 0.5);
       if(rows<1) rows=1;
       if(cols<1) cols=1;
       while((rows*cols) > ntraces && cols>1) cols--; 
       while((rows*cols) < ntraces) cols++; 
       if(rows==1) cols=ntraces;
       if(cols==1) rows=ntraces;

       // allows aspGrid overwrite
       rows = (int)getReal("aspGrid",1,rows);
       cols = (int)getReal("aspGrid",2,cols);

       frame->rows = rows;
       frame->cols = cols;

   } else {

     unsigned pos;
     string str=string(argv[1]);
     if((pos = str.find("x")) != string::npos) {
       frame->rows = atoi(str.substr(0,pos).c_str());
       frame->cols = atoi(str.substr(pos+1).c_str());
       argv++;
       argc--;
     } else {
       frame->rows=1;
       frame->cols=1;
     }
     ntraces = mspec(frame,argc,argv);
     if(!ntraces) return;
   }

   frame->currentTrace = -1; // set it to an invalid number, so all traces will be displayed by default 

   frame->initSpecFlag(SPEC_GRID);
   frame->initAxisFlag(AX_BOX);
   frame->draw();
}

void AspDis1D::dssh(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   argv++;
   argc--;
   frame->currentTrace = -1; // set it to an invalid number, so all traces will be displayed by default 
   int ntraces = mspec(frame,argc,argv);
   if(!ntraces) return;

   frame->rows=1;
   frame->cols=ntraces;

   frame->initSpecFlag(SPEC_GRID);
   frame->initAxisFlag(0);
   frame->draw();
}

void AspDis1D::dss(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   argv++;
   argc--;
   frame->currentTrace = -1; // set it to an invalid number, so all traces will be displayed by default 
   int ntraces = mspec(frame,argc,argv);
   if(!ntraces) return;

   // set ho,vo
   double hoff = frame->getDefaultDataInfo()->getHoff();
   double voff = frame->getDefaultDataInfo()->getVoff();
   if(fabs(hoff) > wc/ntraces) {
      P_setreal(CURRENT, "ho", wc*hoff/(double)ntraces/fabs(hoff), 1);
   }
   if((int)voff == 0)
   P_setreal(CURRENT, "vo", wc2/(1+ntraces), 1);

   frame->rows=1;
   frame->cols=1;

   frame->initSpecFlag(SPEC_DSS);
   frame->initAxisFlag(AX_BOX | AX_SOUTH);
   frame->draw();
}

void AspDis1D::ds(spAspFrame_t frame, int argc, char *argv[]) {
   if(frame == nullAspFrame) return;

   int ntraces = mspec(frame,argc,argv);
   if(!ntraces) return;

   frame->rows=1;
   frame->cols=1;

   frame->initSpecFlag(SPEC_DS);
   frame->initAxisFlag(AX_BOX | AX_SOUTH);
   frame->draw();
}

int AspDis1D::loadData(spAspFrame_t frame, bool sel) {

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) RETURN;

   SpecDataMgr *dataMgr = SpecDataMgr::get();
   SpecDataMap::iterator pd;

   string nucleus = dataInfo->haxis.name;

   int maxTraces = dataInfo->getNtraces(nucleus);
   if(maxTraces < 1) return 0;

   int i, count=0, npts, ind;
   double minX, maxX,scale=1.0;
   char str[16];
   char color[16];
 
   int disFlag = frame->getDisFlag();
   AspTraceList *selTraceList = frame->getSelTraceList();
   AspTraceList *traceList = frame->getTraceList();

   traceList->clearList();
   selTraceList->clearList();

   for(i=0; i<maxTraces; i++) {
/* 
	//123456789 987654321 123456789
        ind = floor(i/9);
        if(ind%2 == 0) ind = i % 9 + 1;
	else ind = 9 - i % 9;
*/
	// 12345678 98765432 12345678 98765432
        ind = (int)floor(count/(double)8);
        if(ind%2 == 0) ind = count % 8 + 1;
	else ind = 9 - count % 8;

        sprintf(color,"Spectrum%d",ind); 

	string key="";
        char file[MAXSTR2];
	P_getstring(CURRENT, "file", file, 1, MAXSTR2);
	string path=string(file);
	dataMgr->getTraceInfo(i, key, path,ind, npts, minX, maxX,nucleus);
	if(npts > 0) {
	  spAspTrace_t trace = spAspTrace_t(new AspTrace(count,key,path,ind,npts,minX,maxX));
	  string clr = getColor(key);
	  if(clr == "") trace->colorStr=string(color);
	  else trace->colorStr=string(clr);
	  trace->labelFlag=disFlag;
	  trace->scale=scale;
	  trace->vp = (double)count;
	  sprintf(str,"%s:%d",key.c_str(),ind);
	  traceList->addTrace(string(str), trace);
	  if(sel) selTraceList->addTrace(trace->getKeyInd(), trace);
          count++;
	}
   } 
   return count;
}

int AspDis1D::mspec(spAspFrame_t frame, int argc, char *argv[]) {

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) RETURN;

   SpecDataMgr *dataMgr = SpecDataMgr::get();
   SpecDataMap::iterator pd;

   string nucleus = dataInfo->haxis.name;

   int maxTraces = dataInfo->getNtraces(nucleus);
   if(maxTraces < 1) {
      if(!init2d(1,1)) {
	maxTraces = dataInfo->getNtraces(nucleus);
	dataInfo->updateDataInfo();
      }
      if(maxTraces < 1) return 0;
   }

   int i, count=0, npts, ind, iColor, jk=0;
   double minX, maxX,scale=1.0;
   char str[16];
   char color[16];
 
   int disFlag = frame->getDisFlag();
   AspTraceList *selTraceList = frame->getSelTraceList();
   AspTraceList *traceList = frame->getTraceList();
   //if(traceList->getSize() < 1 || traceList->getSize() != maxTraces) { 
   if(traceList->getSize() < 1) { 
     // fill traceList. This will fill the list with fdf data if any is loaded.
     // if not fdf data is loaded, it will fill th elist with curexp data 
	count = loadData(frame);
        if(count < 1) return 0;

   } else if(argc<2 && frame->currentTrace < 0 && selTraceList->getSize() > 0) { 
	// no selection argument, so return current selection
	return selTraceList->getSize();
   } 

   selTraceList->clearList(); 

   selTraceList->maxInd = traceList->getSize();

   //frame->initAnnoFlag(0);

  bool showAll = false;
  if(argc>1 && (strcasecmp(argv[1],"all") == 0 || strcasecmp(argv[1],"1-") == 0)) {
     argv++;
     argc--;
     showAll = true;
  }
/*
  if(argc<2 && (frame->currentTrace < 0 || frame->currentTrace >= maxTraces) ) {
	showAll = true; // show all by default 
  } else if(argc<2) { // show current trace
	spAspTrace_t trace = traceList->getTraceByInd(frame->currentTrace);
	if(trace != nullAspTrace) {
	  trace->selected=false;
	  selTraceList->addTrace(trace->getKeyInd(), trace);
	  return 1;
	} else return 0;
  } 
*/
  if(argc<2 && selTraceList->getSize() > 0) {
        // no selection argument, so return current selection
        return selTraceList->getSize();
  } else if(argc<2) showAll = true;

  frame->currentTrace=0;
  if(showAll) { 
	// all in the array

     count=0;
     for(i=0; i<maxTraces; i++) {

	// 12345678 98765432 12345678 98765432
        ind = (int)floor(count/(double)8);
        if(ind%2 == 0) ind = count % 8 + 1;
	else ind = 9 - count % 8;

        sprintf(color,"Spectrum%d",ind); 

	spAspTrace_t trace = traceList->getTraceByInd(i);
	if(trace != nullAspTrace) {
	  trace->selected=false;
	  string clr = getColor(trace->dataKey);
	  if(clr == "") trace->colorStr=string(color);
	  else trace->colorStr=string(clr);
	  selTraceList->addTrace(trace->getKeyInd(), trace);
	  count++;	
	}
     }

     argv++;
     argc--;
     iColor = 0;
     while(argc && iColor < i) {
	if(!isdigit(argv[0][0]) || strstr(argv[0],"0x")==argv[0] || strstr(argv[0],",") != NULL) {
	  selTraceList->getTrace(iColor)->colorStr=string(argv[0]);
	  iColor++;
	}
        argv++;
        argc--;
     }
 
     if(iColor > 0 && iColor < i) {
        int j;
        ind = i;
        for(i=iColor; i<ind; i+=iColor) {
	   for(j=0; j<iColor; j++) {
	      selTraceList->getTrace(i+j)->colorStr = string(selTraceList->getTrace(j)->colorStr);
	   }
	}
     }
  } else if(isdigit(argv[1][0]) && strstr(argv[1],"-") != NULL) {
  // aspDs('1-') - all
  // aspDs('6-') - 6 to last trace
  // aspDs('2-10')
  // aspDs('2-10:2') - 2 to 10 with step size 2
  // aspDs('2-:2') - 2 to last trace, with step size 2

     argv++;
     argc--;

   char words[64][64];
   char *tokptr = strtok(argv[0], ", ");
   int nwords=0;
   while(tokptr != NULL) {
     if(strlen(tokptr) > 0) {
       strcpy(words[nwords], tokptr);
       nwords++;
     }
     tokptr = strtok(NULL, ", ");
   }

   count=0;
   int k;
   for(k=0;k<nwords;k++) {
   
     int first=1, last=maxTraces, step=1;
     AspUtil::getFirstLastStep(words[k], maxTraces, first, last, step);

     for(i=first;i<=last;i+=step) {

	spAspTrace_t trace = traceList->getTraceByInd(i);
	if(trace != nullAspTrace) {	
	  trace->selected=false;
	  selTraceList->addTrace(trace->getKeyInd(), trace);
	  count++;
	}
     }
   }

     argv++;
     argc--;
     iColor = 0;
     while(argc && iColor < count) {
	if(!isdigit(argv[0][0]) || strstr(argv[0],"0x")==argv[0] || strstr(argv[0],",") != NULL) {
	  selTraceList->getTrace(iColor)->colorStr = string(argv[0]);
	  iColor++;
	}
        argv++;
        argc--;
     }
 
     if(iColor > 0 && iColor < count) {
        int j;
        ind = count;
        for(i=iColor; i<ind; i+=iColor) {
	   for(j=0; j<iColor; j++) {
	      selTraceList->getTrace(i+j)->colorStr = string(selTraceList->getTrace(j)->colorStr);
	   }
	}
     }
 
  } else if(argc > 1 && isdigit(argv[1][0])) { // asp1D(1,3,4,...,color,...) 
     int tr;
     iColor = 0;
     i=0;
     argv++;
     argc--;
     while(argc) {

        char file[MAXSTR2];
	P_getstring(CURRENT, "file", file, 1, MAXSTR2);
	string path=string(file);

        if(isdigit(argv[0][0]) && strstr(argv[0],"0x") == NULL &&
		strstr(argv[0],",") == NULL) {
          tr = atoi(argv[0]);
          if(tr < 1 || tr > maxTraces) {
            Winfoprintf("Index %d out of bounds (ignored)",tr);
            argv++;
            argc--;
            continue;
	  }
	  tr--;
 	  spAspTrace_t trace = traceList->getTraceByInd(tr);
	  if(trace != nullAspTrace) {
	    trace->selected=false;
	    selTraceList->addTrace(trace->getKeyInd(), trace);
            i++;
	  }

        } else if(iColor < selTraceList->getSize()) {
	  if(!isdigit(argv[0][0]) || strstr(argv[0],"0x")==argv[0] || 
		strstr(argv[0],",") != NULL) {
	    selTraceList->getTrace(iColor)->colorStr=string(argv[0]);
	    iColor++;
	  }
        }
        argv++;
        argc--;
     }
 
     count = i;
     if(iColor > 0 && iColor < count) {
        int j;
        ind = i;
        for(i=iColor; i<ind; i+=iColor) {
	   for(j=0; j<iColor; j++) {
	        if((i+j) < selTraceList->getSize()) { 
	          selTraceList->getTrace(i+j)->colorStr=string(selTraceList->getTrace(j)->colorStr);
		}
           }
	      
	}
     }
  } else if(argc>1) { // asp1D('SPEC:1','BASE:1',...)

     // Note, in this case, new traces are create to add to selTraceList.
     // in all other cases, selTraceList is a subset of traceList.
     count=0;
     argv++;
     argc--;
     while(argc) {
/*
        ind = floor(i/9);
        if(ind%2 == 0) ind = i % 9 + 1;
	else ind = 9 - i % 9;
*/
        ind = (int)floor(count/(double)8);
        if(ind%2 == 0) ind = count % 8 + 1;
	else ind = 9 - count % 8;

        char file[MAXSTR2];
	P_getstring(CURRENT, "file", file, 1, MAXSTR2);
	string path=string(file);

	sprintf(color,"Spectrum%d",ind); 
        if(isdigit(argv[0][0])) {
	  i = atoi(argv[0]);
          if(i < 1 || i > maxTraces) {
            Winfoprintf("Index %d out of bounds (ignored)",i);
            argv++;
            argc--;
            continue;
	  }
	  i--;
          string key="";
	  dataMgr->getTraceInfo(i, key, path,ind, npts, minX, maxX,nucleus);
	  if(npts > 0) {
	    spAspTrace_t trace = spAspTrace_t(new AspTrace(i,key,path,ind,npts,minX,maxX));
	    trace->scale=scale;
	    string clr = getColor(key);
	    if(clr=="") trace->colorStr=string(color);
	    else trace->colorStr=string(clr);
	    trace->labelFlag=disFlag;
	    sprintf(str,"%s:%d",key.c_str(),ind);
	    selTraceList->addTrace(string(str), trace);
            count++;
	  }
        } else {
	  string key = string(argv[0]);
	  ind = 1;
	  if(strstr(argv[0],":") != NULL) {
	    char *strptr = argv[0];
	    char *tokptr = (char*) strtok(strptr, ":");
	    if(tokptr == NULL) continue;
	    key = string(tokptr);
            strptr = (char *) 0;
            if((tokptr = (char*) strtok(strptr, ":")) != (char *) 0) ind = atoi(tokptr);
	  }
	  ind--;
/*
	  string str=string(argv[0]);
	  key = str.substr(0,str.find(":"));
	  ind = atoi(str.substr(str.find(":")+1).c_str()); 
*/
	  dataMgr->getTraceInfo(0, key, path,jk, npts, minX, maxX,nucleus);
	  if(npts > 0) {
	    spAspTrace_t trace = spAspTrace_t(new AspTrace(i,key,path,ind,npts,minX,maxX));
	    trace->scale=scale;
	    string clr = getColor(key);
	    if(clr=="") trace->colorStr=(color);
	    else trace->colorStr=(clr);
	    trace->labelFlag=disFlag;
	    sprintf(str,"%s:%d",key.c_str(),ind);
	    selTraceList->addTrace(string(str), trace);
            count++;
	  }
        }
        argv++;
        argc--;
     }
  }

  if(selTraceList->getSize()>0) frame->currentTrace = 0;

  return count;
}

// slow scroll, clicks = +/-1
// fast scroll, clicks = +/-2
// scroll down, clicks > 0; scroll up, clicks < 0. 
// factor = 1.189207; // 2**1/4 by default, 
// but can be set with global parameter scrollfactor (does not exist by default)
void AspDis1D::specVS(int frameID, int clicks, double factor) {

   spAspFrame_t frame = AspFrameMgr::get()->getFrame(frameID);
   if(frame == nullAspFrame) return;

   double vscale;
   if (!P_getreal(CURRENT, "vs", &vscale, 1)) {

        factor = getReal("scrollfactor", 1, factor);
        if(factor > 10) factor=10.0;

        int j=0;
        if (clicks > 0) {
            while (j < clicks) {
              vscale /= factor;
              j++;
            }
        } else {
            while (j > clicks) {
              vscale *= factor;
              j--;
            }
        }

        P_setreal(CURRENT, "vs", vscale, 1);
        frame->draw();
   }

}

// mode=1, one cursor mode
// mode=2, two cursor mode
// cursor=1, first cursor (right mouse button)
// cursor=2, second cursor (left mouse button)
void AspDis1D::showCursor(int frameID, int x, int y, int mode, int cursor) {

   spAspFrame_t frame = AspFrameMgr::get()->getFrame(frameID);
   if(frame == nullAspFrame) return;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return;

   double scale = frame->getDefaultDataInfo()->haxis.scale;
   if(scale == 0) scale = 1.0;
   double xval = cell->pix2val(HORIZ,x)*scale;
   if(mode == 1) {
        if(cursor==1) {
           P_setreal(CURRENT, "cr",xval, 1);
   	   // display first cursor
	   vj_x_cursor(0, NULL, x, CURSOR_COLOR);
        }
   } else if(mode == 2) {
        if(cursor==1) {
           double c;
           double pstx,psty,pwd,pht;
	   cell->getPixCell(pstx,psty,pwd,pht);
           P_getreal(CURRENT, "delta", &c, 1);
	   c = (xval - c)/scale;
	   int x2 = (int) cell->val2pix(HORIZ,c); 
	   if(x2 >= (pstx + pwd)) {
		x2 = (int)(pstx + pwd);
		c = cell->pix2val(HORIZ,pstx + pwd)*scale;
		P_setreal(CURRENT, "delta", xval-c, 1);
	   }
           P_setreal(CURRENT, "cr",xval, 1);
	   vj_x_cursor(0, NULL, x, CURSOR_COLOR);
	   vj_x_cursor(1, NULL, x2, CURSOR_COLOR);
        } else if(cursor==2) {
           double c;
           P_getreal(CURRENT, "cr", &c, 1);
	   if(c>xval) {
             P_setreal(CURRENT, "delta", (c-xval), 1);
	     int x2 = (int) cell->val2pix(HORIZ,c/scale);
	     vj_x_cursor(0, NULL, x2, CURSOR_COLOR);
	     vj_x_cursor(1, NULL, x, CURSOR_COLOR);
	   }
        }
   }
   showFields(frame,cell);

}

void AspDis1D::setZoom(int frameID, int x, int y, int prevX, int prevY) {
   if(x==prevX) return;
   spAspFrame_t frame = AspFrameMgr::get()->getFrame(frameID);
   if(frame == nullAspFrame) return;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return;
   spAspCell_t cell2 =  frame->selectCell(prevX,prevY);
   if(cell2 == nullAspCell) return;

   double scale = frame->getDefaultDataInfo()->haxis.scale;
   double c1,c2;
   if(cell2 != cell) return;  // TODO inter-zoom

   if(x<prevX) { // reset to full
	c1 = frame->getSelTraceList()->getMaxX();
	c2 = frame->getSelTraceList()->getMinX();
   	setSpecRegion(c1,c2,scale);
   } else {
        c1 = cell->pix2val(HORIZ,x); 
        c2 = cell->pix2val(HORIZ,prevX); 
   	setSpecRegion(c1,c2,scale,true);
/*
        if(AspMouse::getState() == AspMouse::cursor2) { 
	  AspMouse::setState(AspMouse::cursor1);
        }
*/
   }

   frame->draw();
}

void AspDis1D::setSpecRegion(double c1, double c2, double scale, bool zoomin) {
   if(c1==c2) return;
   if(scale == 0) scale = 1.0;
   c1 *= scale;
   c2 *= scale;
   double cr,delta;
   if(c2>c1) {
	cr = c2;
	delta = c2-c1;
	P_setreal(CURRENT, "sp", c1, 1);
	P_setreal(CURRENT, "wp", delta, 1);
   } else {
        cr = c1;
	delta = c1-c2;
	P_setreal(CURRENT, "sp", c2, 1);
	P_setreal(CURRENT, "wp", delta, 1);
   }

   // set cr, delta if zoomed in
/*
   if(zoomin) {

      cr -= 0.1*delta;
      delta *= 0.8;
      P_setreal(CURRENT, "cr", cr, 1);
      P_setreal(CURRENT, "delta", delta, 1); 
      writelineToVnmrJ("pnew", "4 sp,wp,cr,delta");

   } else 
*/
	writelineToVnmrJ("pnew", "2 sp,wp");

}

void AspDis1D::zoomSpec(int frameID, int x, int y, int mode) {
   spAspFrame_t frame = AspFrameMgr::get()->getFrame(frameID);
   if(frame == nullAspFrame) return;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return;

   double scale = frame->getDefaultDataInfo()->haxis.scale;
   double c = cell->pix2val(HORIZ,x);
   double factor = getReal("zoomfactor", 1, 1.4);
   if(factor == 0) factor=1.4;

   double vx,vy,vw,vh;
   cell->getValCell(vx,vy,vw,vh);

   double c1,c2;
   if(mode == FULL) {
	c1 = frame->getSelTraceList()->getMaxX();
	c2 = frame->getSelTraceList()->getMinX();
   } else if(mode == ZOOM_IN) {
	c1 = c + 0.5*vw/factor;	
	c2 = c - 0.5*vw/factor;	
        setSpecRegion(c1,c2,scale,true);
/*
        if(AspMouse::getState() == AspMouse::cursor2) { 
	  AspMouse::setState(AspMouse::cursor1);
        }
*/
   } else if(mode == ZOOM_OUT) {
	c1 = c + 0.5*vw*factor;	
	c2 = c - 0.5*vw*factor;	
        setSpecRegion(c1,c2,scale);
   } else return;
   frame->draw();
}

void AspDis1D::panSpec(int frameID, int x, int y, int prevX, int prevY, int mode) {
   spAspFrame_t frame = AspFrameMgr::get()->getFrame(frameID);
   if(frame == nullAspFrame) return;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return;

   double vx,vy,vw,vh;
   cell->getValCell(vx,vy,vw,vh);

   double scale = frame->getDefaultDataInfo()->haxis.scale;
   double c1,c2, d1,d2;
   if(mode == CENTER) {
   	d1 = cell->pix2val(HORIZ,x);
        c1 = d1 + 0.5*vw;
        c2 = d1 - 0.5*vw;
   } else if(mode == PAN_1D) { // ctrl or shift drag
   	double px,py,pw,ph;
  	cell->getPixCell(px,py,pw,ph);
        if((x-px) > 10) { // pan horizontally if x is near left border 
   	  d1 = cell->pix2val(HORIZ,x);
   	  d2 = cell->pix2val(HORIZ,prevX);
	  c2 = vx-vw + (d2-d1);
	  c1 = c2 + vw;
	} else { // pan vertically
          P_getreal(CURRENT, "vp", &d1, 1);
	  d2 = (y-prevY)*getReal("wc2max",100.0)/(frame->pixht);
          P_setreal(CURRENT, "vp", d1-d2, 1);
   	  frame->draw();
	  return;	
	}
   } else if(mode == PAN_2D) {
          P_getreal(CURRENT, "vp", &d1, 1);
	  d2 = (y-prevY)*getReal("wc2max",100.0)/(frame->pixht);
          P_setreal(CURRENT, "vp", d1-d2, 1);
   	  d1 = cell->pix2val(HORIZ,x);
   	  d2 = cell->pix2val(HORIZ,prevX);
	  c2 = vx-vw + (d2-d1);
	  c1 = c2 + vw;
   } else if(mode == PAN_ZOOM) {
        double factor = getReal("zoomfactor", 1, 1.4);
   	if(factor == 0) factor=1.4;
	factor = 1.0 + (factor - 1.0)/10.0; // 1/10 of the factor comparing to click.
	double c = vx-0.5*vw; // center of spectrum
	if(y > prevY) { // zoom out
	  c1 = c + 0.5*vw*factor;	
	  c2 = c - 0.5*vw*factor;	
	} else { // zoom in
	  c1 = c + 0.5*vw/factor;	
	  c2 = c - 0.5*vw/factor;	
	}
   } else if(mode == RESET) {
          P_setreal(CURRENT, "vp", 0, 1);
	  c1 = frame->getSelTraceList()->getMaxX();
	  c2 = frame->getSelTraceList()->getMinX();
   } else return;
   setSpecRegion(c1,c2,scale);
   frame->draw();
}

int AspDis1D::getThreshPix(spAspCell_t cell) {
   double th,vpos;
   if (P_getreal(CURRENT, "th", &th, 1)) th=0.0;
   if (P_getreal(CURRENT, "vp", &vpos, 1)) vpos=0.0;
   th += vpos;
   th *= cell->getCali(VERT);
   double x,y,w,h;
   cell->getPixCell(x,y,w,h);
   return (int)(y+h-th);
}

bool AspDis1D::selectThresh(spAspFrame_t frame, int x, int y) {
   int flag = frame->getDisFlag();
   if(!(flag & SPEC_THRESH)) return false;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return false;

   int th = getThreshPix(cell);
//Winfoprintf("thresh %d %d",y,th);

   bool threshSelected;
   if(y < (th+2) && y > (th-2)) {
	threshSelected = true;
   } else threshSelected = false; 

   if(frame->threshSelected != threshSelected) { // erase oldTh
	frame->threshSelected = threshSelected;
	frame->setDisFlag(SPEC_THRESH, false);
	frame->draw();
	frame->setDisFlag(SPEC_THRESH, true);
   } 

   drawThresh(cell, threshSelected);

   return threshSelected;
}

void AspDis1D::moveThresh(spAspFrame_t frame, int x, int y, int prevX, int prevY) {
// do peak picking if in peak picking mode
   int flag = frame->getDisFlag();
   if(!(flag & SPEC_THRESH)) return;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return;

   double th,vpos;
   double px,py,pw,ph;
   cell->getPixCell(px,py,pw,ph);
   if (P_getreal(CURRENT, "vp", &vpos, 1)) vpos=0.0;
   th = (py+ph-y)/cell->getCali(VERT) - frame->getDefaultDataInfo()->getVpos();

//Winfoprintf("set thresh %f",th);
   if(th <= 0) return;
   
   P_setreal(CURRENT, "th", th, 1);
   drawThresh(cell, frame->threshSelected);
   showFields(frame,cell);
}

void AspDis1D::drawThresh(spAspCell_t cell, bool threshSelected) {
  
  int th = getThreshPix(cell);
  if(threshSelected) { 
    vj_y_cursor(1, NULL, mnumypnts-th, ACTIVE_COLOR);
  } else {
    vj_y_cursor(1, NULL, mnumypnts-th, THRESH_COLOR);
  }

}

void AspDis1D::showFields(spAspFrame_t frame, spAspCell_t cell){
 
   char str[MAXSTR2];

   AspMouse::mouseState_t state = AspMouse::getState();
   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;

   int flag = frame->getDisFlag();
   if(state == AspMouse::select && (flag & SPEC_THRESH)) {
      double vscale = dataInfo->getVscale();
      double vpos = dataInfo->getVpos();
/* 
      // display value
      double th = getThreshPix(cell);
      th = cell->pix2val(VERT,th);
*/
      // display mm 
      double th;
      P_getreal(CURRENT, "th", &th, 1);
      sprintf(str,"V position [%.2f]    V scale [%.2f]    thresh [%.2f]",vpos,vscale,th);

   } else if(state == AspMouse::cursor1 || state == AspMouse::cursor2) {
     double cr, delta;
     P_getreal(CURRENT, "cr", &cr, 1);
     P_getreal(CURRENT, "delta", &delta, 1);
     double scale = dataInfo->haxis.scale;
     if(flag & SPEC_THRESH) {
       double vscale = dataInfo->getVscale();
/*
       // display value
       double th = getThreshPix(cell);
       th = cell->pix2val(VERT,th);
*/
       // display mm 
       double th;
       P_getreal(CURRENT, "th", &th, 1);
       sprintf(str,"cursor [%.4f]    delta [%.4f]    V scale [%.2f]    thresh [%.2f]",cr/scale,delta/scale,vscale,th);
     } else {
       sprintf(str,"cursor [%.4f]    delta [%.4f]",cr/scale,delta/scale);
     }

   } else if(state == AspMouse::zoom || state == AspMouse::pan) {
      double cr, delta,sp,wp;
      P_getreal(CURRENT, "cr", &cr, 1);
      P_getreal(CURRENT, "delta", &delta, 1);
      P_getreal(CURRENT, "sp", &sp, 1);
      P_getreal(CURRENT, "wp", &wp, 1);
      double scale = dataInfo->haxis.scale;
      sprintf(str,"start [%.4f]    width [%.4f]    cr [%.4f]    delta [%.4f]",sp/scale,wp/scale,cr/scale,delta/scale);
   } else if(state == AspMouse::array) {
      double vpos = dataInfo->getVpos();
      double vscale = dataInfo->getVscale();
      double hoff,voff;
      P_getreal(CURRENT,"vo",&voff,1);
      P_getreal(CURRENT,"ho",&hoff,1);
     sprintf(str,"V posistion [%.2f]    VOff [%.2f]    HOff [%.2f]    V scale [%.2f]",vpos,voff,hoff,vscale);
   } else if(state == AspMouse::phasing) {
      double rp,lp;
      P_getreal(CURRENT, "rp", &rp, 1);
      P_getreal(CURRENT, "lp", &lp, 1);
      sprintf(str,"PH0 [%.2f]    PH1 [%.2f]",rp,lp);
   } else {
      double vscale = dataInfo->getVscale();
      double vpos = dataInfo->getVpos();
      sprintf(str,"V position [%.2f]    V scale [%.2f]",vpos,vscale);
   } 

   frame->writeFields(str);
}

void AspDis1D::setArray(spAspFrame_t frame, int x, int y, int prevX, int prevY, int mode) {
  int flag = frame->getSpecFlag();
  if(!(flag & SPEC_DSS) && !(flag & SPEC_DS)) return;

   spAspCell_t cell =  frame->selectCell(x,y);
   if(cell == nullAspCell) return;
   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   if(dataInfo == nullAspData) return;
   int n = frame->getSelTraceList()->getNtraces();
   if(n < 1) return;

   double vpos = dataInfo->getVpos();
   double voff = dataInfo->getVoff();
   double hoff = dataInfo->getHoff();

  if(mode == ARRAY_RESET) {
     P_setreal(CURRENT, "vo", 0.0, 1);
     P_setreal(CURRENT, "ho", 0.0, 1);
  } else  if(mode == ARRAY_RESETALL) {
     P_setreal(CURRENT, "vo", 0.0, 1);
     P_setreal(CURRENT, "ho", 0.0, 1);
     P_setreal(CURRENT, "vp", 0.0, 1);
     frame->getTraceList()->resetVpos(); 
  } else  if(mode == ARRAY_VP) {
	double d2;
	d2 = (y-prevY)*getReal("wc2max",100.0)/(frame->pixht);
        vpos = vpos-d2;
        P_setreal(CURRENT, "vp", vpos, 1);
  } else  if(mode == ARRAY_OFFSET) {
	double d2;
	if(abs(x-prevX) > abs(y-prevY)) { // change ho 
	  double cali = getReal("wcmax",100.0)/(frame->pixwd);
	  d2 = (x-prevX)*cali;
	  hoff= hoff - d2;
	  double maxoff = wc/n;
	  if(fabs(hoff) > maxoff) hoff=hoff*maxoff/fabs(hoff);
          P_setreal(CURRENT, "ho", hoff, 1);
	} else { // change vo
	  double d2;
	  //d2 = (y-prevY)*getReal("wc2max",100.0)/(frame->pixht);
	  if(voff != 0) d2 = (y-prevY)/voff;
	  else d2 = (y-prevY);
  	  voff = voff-d2;
	  voff = ((voff-d2) > 0) ? (voff-d2) : 0.0;
          P_setreal(CURRENT, "vo", voff, 1);
	}
  }

  frame->draw();
}

void AspDis1D::deleteTrace(spAspFrame_t frame, spAspTrace_t trace) {

   if(trace == nullAspTrace) return;
   //???
   //AspTraceList *traceList = frame->getTraceList();
   AspTraceList *traceList = frame->getSelTraceList();
   if(traceList == NULL) return;

   AspTraceMap::iterator itr;
   AspTraceMap *traceMap = traceList->getTraceMap();
   
   for (itr = traceMap->begin(); itr != traceMap->end(); ++itr) {
	if(itr->second == trace) {
	   traceMap->erase(itr->first);
  	   frame->draw();
	   break; 
	}
   }
}

spAspTrace_t AspDis1D::selectTrace(spAspFrame_t frame, int x, int y) {
   if(!frame->doDs()) return nullAspTrace;
   spAspCell_t cell =  frame->selectCell(x,y);

   if(cell == nullAspCell) return nullAspTrace;

   bool changeFlag=false;
   spAspTrace_t trace = frame->getSelTraceList()->selectTrace(cell,x, y, changeFlag);
   if(changeFlag) frame->draw();

   if(trace != nullAspTrace) { 
        char str[MAXSTR2];
        string key = trace->dataKey;
        sprintf(str,"path [%s]    trace [%d]",trace->path.c_str(),trace->dataInd+1);
        frame->writeFields(str);
   }

   return trace;
}

void AspDis1D::setPhase0(spAspFrame_t frame, int x, int y, int prevX, int prevY, int mode) {
  double rp;
  P_getreal(CURRENT, "rp", &rp, 1);
  if(mode == COARSE) {
    if(y > prevY) rp += 2;
    else rp -= 2;
    P_setreal(CURRENT, "rp", rp, 1);
  } else if(mode == FINE) {
    if(y > prevY) rp += 0.5;
    else rp -= 0.5;
    P_setreal(CURRENT, "rp", rp, 1);
  } else if(mode == AUTO) {
    execString("aph0\n");
  }

  frame->draw();
}

void AspDis1D::moveTrace(spAspFrame_t frame, spAspTrace_t trace, int y, int prevY) {
   //trace->vp += (prevY-y)*getReal("wc2max",100)/(frame->pixht);
   double voff = frame->getDefaultDataInfo()->getVoff();
   if(voff != 0) {
     trace->vp += (prevY-y)/voff;
     frame->draw();
   }
}

string AspDis1D::getColor(string key) {
	spSpecData_t sd = SpecDataMgr::get()->getDataByKey(key);
   	if(sd != (spSpecData_t)NULL) return sd->getColor();
	else return "";
}

int AspDis1D::loadData(spAspFrame_t frame, int argc, char *argv[]) {
	if(frame == nullAspFrame || argc < 3) RETURN;

  	spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();
   	AspTraceList *traceList = frame->getTraceList();
   	AspTraceList *selTraceList = frame->getSelTraceList();

	spAspTrace_t sumTrace;
	int sumTraceFlag = 0;
        if(strstr(argv[1],"add") == argv[1]) sumTraceFlag = ADD_TRACE;
	else if(strstr(argv[1],"sub") == argv[1]) sumTraceFlag = SUB_TRACE;
	else if(strstr(argv[1],"rep") == argv[1]) sumTraceFlag = REP_TRACE;
	if(sumTraceFlag != 0) {
	   if(isdigit(argv[2][0])) {
		sumTrace = frame->getTraceList()->getTraceByInd(atoi(argv[2])-1);
		argc--;
		argv++;
		if(argc < 3) RETURN;
	   } else sumTrace = frame->getTraceList()->getTraceByInd(0); 
	} else sumTrace = nullAspTrace; 

        string color = "";
 	if(argc>4) color=string(argv[4]);

	double vp=0.0;
	if(argc>5) vp=atof(argv[5]);
	
	double vscale=1.0;
	if(argc>6) vscale=atof(argv[6]);

	double shift=0.0;
        if(argc>7) shift = atof(argv[7]);
 
	double min = 0;
	double max = 0;
	if(argc>9) {
	   int l1 = strlen(argv[8]);
	   int l2 = strlen(argv[9]);
	   if(isdigit(argv[8][l1-1]) && isdigit(argv[9][l2-1]) ) {
	     min= atof(argv[8]);
	     max= atof(argv[9]);
	   }
	}
	if(min>max) {
	  double tmp=min;
	  min=max;
	  max=tmp;
	}

	double dcMargin=0.0;
        if(argc>10) dcMargin = atof(argv[10]);
 
	// argv[3] is both key and label
	// but key could be generated automatically
	string key;
	if(argc>3 && strlen(argv[3])>0) key=string(argv[3]);
	else key =SpecDataMgr::get()->getNextKey();
	
	spSpecData_t sd = SpecDataMgr::get()->getDataByKey(key);
	if(sd == (spSpecData_t)NULL) {
	   if(!SpecDataMgr::get()->loadSpec(key, argv[2]))
     	   {
	     Winfoprintf("Error loading %s",argv[2]);
	     RETURN;
	   }
	}

	sd = SpecDataMgr::get()->getDataByKey(key);
   	specStruct_t *ss = NULL;
   	if(sd != (spSpecData_t)NULL) ss = sd->getSpecStruct();
	if(ss == NULL) {
	   Winfoprintf("Error loading %s",argv[2]);
	   RETURN;
	}
	
	if(argc>3) sd->setLabel(string(argv[3])); 
	if(color!="") sd->setColor(color);

  	string nucleus = dataInfo->haxis.name;

        if(string(ss->nucleus[0]) != nucleus) {
	   Winfoprintf("Cannot load %s data.",ss->nucleus[0]);
	   RETURN;
	}

	int first,last,step;

        int i=1, ntraces=0;
        while(i < (ss->rank)) {
           ntraces += ss->matrix[i];
           i++;
        }
	first=0; last=ntraces-1; step=1;
	if(argc>13) {
	  first=atoi(argv[11])-1;
	  last=atoi(argv[12])-1;
	  step=atoi(argv[13]);
	} else if(argc>12) {
	  first=atoi(argv[11])-1;
	  last=atoi(argv[12])-1;
	} else if(argc>11) {
	  first=last=atoi(argv[11])-1;
	}
        if(first<0) first=0;
	if(last>(ntraces-1)) last = ntraces-1;
	
	string path;
	int ind,npts;
	double minX,maxX;
	char str[16];
 	int disFlag = frame->getDisFlag();
        for(i=first; i<=last; i+=step) {
	   sprintf(str,"%s:%d",key.c_str(),i);
	   string newKey=string(str);
	   int count=traceList->getSize();
	   if(traceList->getTraceByKey(newKey) != nullAspTrace) count--;
           path = sd->getPath();
           ind = i;
           npts = ss->matrix[0];
           minX = ss->upfield[0]/ss->reffrq[0];
           maxX = ss->sw[0]/ss->reffrq[0] + minX;
	   spAspTrace_t trace = spAspTrace_t(new AspTrace(count,key,path,ind,npts,minX,maxX));
	   if(argc>5) trace->vp=vp;
	   else trace->vp = (double)count;
	   if(color=="") { 
		char clr[16];
        	int ic = (int)floor(count/(double)8);
        	if(ic%2 == 0) ic = count % 8 + 1;
		else ic = 9 - count % 8;

        	sprintf(clr,"Spectrum%d",ic); 
		color=string(clr);
	   } 
	   trace->colorStr=string(color);
	   trace->scale=vscale;
	   trace->labelFlag=disFlag;
	   trace->dcMargin=dcMargin;
	   if(argc>7)
	        trace->setShift(shift);
	   if(argc>9 && min != max) {
		trace->setMinX(min);
		trace->setMaxX(max);
	   }
	   trace->cmd=string(argv[1]); // overwrite default cmd "load" (could be "load_dc",...)
	   if(strstr(argv[1],"_dc") != NULL) trace->dodc();

	   if(sumTrace != nullAspTrace) {
	      sumTrace->sumTrace(trace, sumTraceFlag);
	   } else {
	      traceList->addTrace(newKey, trace);
	      selTraceList->addTrace(newKey, trace);
	   }
	}
	selTraceList->maxInd= traceList->getSize();

   	frame->initSpecFlag(SPEC_DS);
   	frame->initAxisFlag(AX_BOX | AX_SOUTH);
	AspMouse::setState(AspMouse::cursor1);
	frame->draw();
	RETURN;
}

// called by draw/redraw after cells are ccreated and filled with data.
void AspDis1D::display(spAspFrame_t frame) {
   if(frame == nullAspFrame) return;

   int specFlag = frame->getSpecFlag();
   int disFlag = frame->getDisFlag();
   int axisFlag = frame->getAxisFlag();
   int rows = frame->rows;
   int cols = frame->cols;
   bool threshSelected = frame->threshSelected;
   AspCellList *cellList = frame->getCellList(); 
   AspTraceList *selTraceList = frame->getSelTraceList();
   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo();

   AspCellMap::iterator ci;
   spAspCell_t cell;
   if(specFlag & SPEC_GRID) {

      if(axisFlag == 0) { // draw plot box here because it won't be drawn by "cells".
        frame->setCellFOV(1,1); // layout 1x1 cell to draw the box
        cell = cellList->getFirstCell(ci);
        if(cell == nullAspCell) return;

	double pstx,psty,pwd,pht;
        cell->getPixCell(pstx,psty,pwd,pht);

        // now layout the grid (multiple cells)
        frame->setCellFOV(rows,cols);

	// Note, setCellFOV will clear graphics, so draw the box after the second setCellFOV
      	set_background_region((int)pstx,(int)psty,(int)pwd,(int)pht,BOX_BACK,100);
      	AspUtil::drawBox(pstx,psty,pwd,pht,SCALE_COLOR);
      } else {
        frame->setCellFOV(rows,cols);
      }

      AspTraceMap::iterator itr;
      AspTraceMap *traces = selTraceList->getTraceMap();
      spAspTrace_t trace;

      for(cell= cellList->getFirstCell(ci), itr=traces->begin(); 
	cell != nullAspCell && itr != traces->end(); cell= cellList->getNextCell(ci), ++itr) {
	cell->showAxis(axisFlag, ASP_GRID_MODE);
	selTraceList->showSpec(cell,itr->second);
      }
   } else { 

      frame->setCellFOV(1,1);
      cell = cellList->getFirstCell(ci);

      if(cell != nullAspCell) {
        if(axisFlag == 0) { // draw plot box here because it won't be drawn by "cell".
	  double pstx,psty,pwd,pht;
          cell->getPixCell(pstx,psty,pwd,pht);
      	  set_background_region((int)pstx,(int)psty,(int)pwd,(int)pht,BOX_BACK,100);
      	  AspUtil::drawBox(pstx,psty,pwd,pht,SCALE_COLOR);
	} else {
	  cell->showAxis(axisFlag, ASP_MODE);
	}
	selTraceList->showSpec(cell,specFlag);
        if(AspMouse::numCursors == 2 || AspMouse::getState() == AspMouse::cursor2) {
          double c1,c2,d;
          if (P_getreal(CURRENT, "cr", &c1, 1)) c1=0.0;
          if (P_getreal(CURRENT, "delta", &d, 1)) d=0.0;
	  c1 /= dataInfo->haxis.scale;
	  d /= dataInfo->haxis.scale;
          c2 = c1-d;
	  double vx,vy,vw,vh;
	  cell->getValCell(vx,vy,vw,vh);
	  if(c1>vx) {
	    c1=vx;
	    c2=c1-d; 
	  } else if(c1<(vx-vw)) {
	    c1 = vx-vw;
	    c2=c1-d; 
	  }
	  if(c2>vx) {
	    c2=vx;
	  } else if(c2<(vx-vw)) {
	    c2 = vx-vw;
	  }
	  if(c1>c2) d = c1-c2;
	  else {
	    d=c2; c2=c1; c1=d; d=c1-c2;
	  }
	  P_setreal(CURRENT, "cr", c1*dataInfo->haxis.scale, 1);
	  P_setreal(CURRENT, "delta", d*dataInfo->haxis.scale, 1);
	  vj_x_cursor(0, NULL, (int)(cell->val2pix(HORIZ,c1)), CURSOR_COLOR);
	  vj_x_cursor(1, NULL, (int)(cell->val2pix(HORIZ,c2)), CURSOR_COLOR);
        } else if(AspMouse::numCursors==1 || AspMouse::getState() == AspMouse::cursor1 ) {
	  double c1;
          if (P_getreal(CURRENT, "cr", &c1, 1)) c1=0.0;
	  c1 /= dataInfo->haxis.scale;
	  double vx,vy,vw,vh;
	  cell->getValCell(vx,vy,vw,vh);
	  if(c1>vx) c1=vx; 
	  else if(c1<(vx-vw)) c1 = vx-vw;
	  P_setreal(CURRENT, "cr", c1*dataInfo->haxis.scale, 1);
	  vj_x_cursor(0, NULL, (int)(cell->val2pix(HORIZ,c1)), CURSOR_COLOR);
        }

	if(disFlag & SPEC_THRESH)
	  drawThresh(cell,threshSelected); // if flag is on
      }
   }
   cell = cellList->getFirstCell(ci);
   showFields(frame,cell);
}

void AspDis1D::unselectTraces(spAspFrame_t frame) {
   if(!frame->doDs()) return;

   bool selected=false;
   AspTraceMap *traceMap = frame->getSelTraceList()->getTraceMap();
   AspTraceMap::iterator ti;
   for (ti = traceMap->begin(); ti != traceMap->end(); ++ti) {
	if(ti->second->selected) {
	   selected=true;
	   ti->second->selected=false;
	}
   }
   if(selected) frame->draw();
}

// this routine aligns multiple traces by align a marked peak in each trace 
// the traces are shifted to align with the first trace
void AspDis1D::alignSpec(spAspFrame_t frame, char *peakFile) {
   if(peakFile) { // clear and load new peak list 
	frame->getPeakList()->clearList();
	AspDisPeaks::load(frame,peakFile);
   }
   AspPeakList *peakList = frame->getPeakList();
   if(peakList->getSize()<1) {
	Winfoprintf("Use peak picking tool to mark peaks, then try again.");
	return;
   }
   
   list<spAspTrace_t> *traceList = frame->getSelTraceList()->getTraceList();
   list<spAspTrace_t>::iterator itr;
   int i;
   double firstMark=0.0;
   bool hasFirstMark=false;
   bool doAlign = false;
   spAspPeak_t peak;
   spAspTrace_t trace;
   for(itr=traceList->begin(), i=0; itr != traceList->end() && i<MAXTRACE; ++itr, ++i) {
	trace = (*itr);
	// get first peak for this trace (there should be one peak per trace
	// could have more peaks, but the first one will be used
	peak = peakList->getFirstPeak(trace->getKeyInd());
	if(peak != nullAspPeak && !hasFirstMark) { // mark of first trace which is not shifted 
	   firstMark = peak->getXval();
	   hasFirstMark=true;
	} else if(peak != nullAspPeak) { // subsequent traces are shifted 
	   double shift = firstMark - (peak->getXval());
	   trace->setShift(trace->getShift()+shift); // shift trace 
	   peakList->shiftPeaks(trace->getKeyInd(),shift); // shift all peaks of the trace
	   doAlign=true;
	}
   }   
   if(doAlign) frame->draw();
}
