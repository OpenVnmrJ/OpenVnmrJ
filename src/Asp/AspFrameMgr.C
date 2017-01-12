/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspFrameMgr.h"
#include "AspUtil.h"
#include "AspMouse.h"

extern "C" {
void jframeFunc(const char *cmd, int id, int x, int y, int w, int h);
int addFrame(const char *type, const char *cmd, int id, int x, int y, int w, int h);
void deleteFrames();
void deleteFrame(int id);
void get_main_win_size(int *x, int *y, int *w, int *h);
void window_redisplay();
int setplotter();
void Jturnoff_aspMouse();
}

AspFrameMgr *AspFrameMgr::aspFrameMgr = NULL;

AspFrameMgr::AspFrameMgr() {
    frameList = new aspFrameList;
    currentFrameID = 0;
}

AspFrameMgr *AspFrameMgr::get() {
    if (!aspFrameMgr) {
        aspFrameMgr = new AspFrameMgr();
    }
    return aspFrameMgr;
}

void AspFrameMgr::draw() {
        
   aspFrameList::iterator fi;
   spAspFrame_t frame;
   for (frame= getFirstFrame(fi); frame != nullAspFrame; frame= getNextFrame(fi)) {
	frame->draw();
   } 
}

void AspFrameMgr::drawFrame(int id) {
      spAspFrame_t frame = getFrame(id);
      if(frame != nullAspFrame) frame->draw();
}

int AspFrameMgr::aspFrame(int argc, char *argv[], int retc, char *retv[]) {

   if(argc<2) {
	Winfoprintf("Usage: aspFrame(...).");
	RETURN;
   } 

   AspFrameMgr *mgr = AspFrameMgr::get();

   // layout/remove/clear frames
   if(strcasecmp(argv[1],"overlay")==0) {
	mgr->makeFrames(0,"overlay");
	RETURN;
   } else if(strcasecmp(argv[1],"make")==0) {

	if(argc>2 && (strstr(argv[2],"x") != 0 || strstr(argv[2],"X") != 0)) {
	  mgr->makeFrames(0,argv[2]);
	  RETURN;
	} 

	int n=1;
	if(argc>2) {
	   n=atoi(argv[2]);
	}
	if(argc>3) mgr->makeFrames(n,string(argv[3]));
	else mgr->makeFrames(n);
   	RETURN;
   } else if(strcasecmp(argv[1],"clear")==0) {
        mgr->clearFrame(mgr->getCurrentFrameID()); 
   } else if(strcasecmp(argv[1],"clearAll")==0) {
	mgr->clearAllFrame();
   } else if(strcasecmp(argv[1],"close")==0) {
	mgr->removeFrame(mgr->getCurrentFrameID());
   } else if(strcasecmp(argv[1],"closeAll")==0) {
	mgr->removeFrames();
   } else if(strcasecmp(argv[1],"show")==0) {
	mgr->draw();
   }

   spAspFrame_t frame = mgr->getCurrentFrame();
   if(frame == nullAspFrame) RETURN;

   if(strcasecmp(argv[1],"annoTop")==0) {
	if(argc>2) frame->annoTop=(atoi(argv[2]) > 0);
	else frame->annoTop = (frame->annoTop==false); // toggle
	window_redisplay();
	RETURN;

   }  
   
   if(strcasecmp(argv[1],"plot")==0) {
	if(argc>2 && strcasecmp(argv[2],"off")==0) {
	   setdisplay();
	} else {
	   setplotter();
	}
	frame->setSize(mnumxpnts,mnumypnts);
   }

   if(strcasecmp(argv[1],"pixmax")==0 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	double pstx, psty, pwd, pht;
	cell->getPixCell(pstx, psty, pwd, pht);
        retv[0] = realString(pwd);
   } else if(strcasecmp(argv[1],"pixmax2")==0 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	double pstx, psty, pwd, pht;
	cell->getPixCell(pstx, psty, pwd, pht);
        retv[0] = realString(pht);
   } else if(strcasecmp(argv[1],"pix2val")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->pix2val(HORIZ,atof(argv[2])));	
   } else if(strcasecmp(argv[1],"pix2val2")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->pix2val(VERT,atof(argv[2]))); 
   } else if(strcasecmp(argv[1],"val2pix")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->val2pix(HORIZ,atof(argv[2])));	
   } else if(strcasecmp(argv[1],"val2pix2")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->val2pix(VERT,atof(argv[2]))); 
   } else if(strcasecmp(argv[1],"mm2pix")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->mm2pix(HORIZ,atof(argv[2])));	
   } else if(strcasecmp(argv[1],"mm2pix2")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->mm2pix(VERT,atof(argv[2]))); 
   } else if(strcasecmp(argv[1],"pix2mm")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->pix2mm(HORIZ,atof(argv[2])));	
   } else if(strcasecmp(argv[1],"pix2mm2")==0 && argc > 2 && retc > 0) {
	spAspCell_t cell = frame->getFirstCell();
        if(cell == nullAspCell) RETURN;
	retv[0] = realString(cell->pix2mm(VERT,atof(argv[2]))); 
   }
   
   RETURN;
}

int AspFrameMgr::aspRoi(int argc, char *argv[], int retc, char *retv[]) {
   AspFrameMgr *mgr = AspFrameMgr::get();

   spAspFrame_t frame = mgr->getCurrentFrame();
   if(frame == nullAspFrame) {
	Winfoprintf("Frame is not defined.");
	RETURN;
   }

   if(retc>0) {
	int flag = frame->getAnnoFlag();
	if(flag & ANN_ROIS) retv[0]=realString(1.0);
	else retv[0]=realString(0.0);
	retc--;
	retv++;
	if(retc>1) {
	  retv[0]=realString((double)frame->getRoiList()->getNumRois());
	  retc--;
	  retv++;
	}
	AspRoiList *roiList = frame->getRoiList();
	AspRoiMap::iterator itr;
	spAspRoi_t roi;
	for (roi= roiList->getFirstRoi(itr); roi != nullAspRoi && retc>1; roi= roiList->getNextRoi(itr)) {
	  retv[0] = realString(roi->cursor1->resonances[0].freq); 
	  retc--;
	  retv++;
	  retv[0] = realString(roi->cursor2->resonances[0].freq); 
	  retc--;
	  retv++;
	}
	RETURN;
   }

   if(argc < 2) {
	Winfoprintf("Usage: aspRoi(...).");
	RETURN;
   }

   spAspDataInfo_t dataInfo = frame->getDefaultDataInfo(true);

   spAspCell_t cell = frame->getFirstCell();
   if(cell == nullAspCell) RETURN;

   if(strcasecmp(argv[1],"show") == 0) {
      frame->showRois(true);
   } else if(strcasecmp(argv[1],"hide") == 0) {
      if(argc>2) frame->getRoiList()->hideRoi(atoi(argv[2])-1);
      else frame->getRoiList()->hideSelRoi();
      frame->displayTop();
   } else if(strcasecmp(argv[1],"hideAll") == 0) {
      frame->showRois(false);
   } else if(strcasecmp(argv[1],"remove") == 0) {
      if(argc>2) frame->getRoiList()->deleteRoi(atoi(argv[2])-1);
      else frame->getRoiList()->deleteSelRoi();
      frame->displayTop();
   } else if(strcasecmp(argv[1],"clear") == 0) {
      frame->clearRois();
   } else if(strcasecmp(argv[1],"add") == 0 && argc>3) {
      frame->getRoiList()->addRois(dataInfo->haxis.name,argc,argv);
      frame->showRois(true);
      //frame->addRoiFromCursors(atof(argv[2]),atof(argv[3]));
   } else if(strcasecmp(argv[1],"add") == 0) {
      frame->addRoiFromCursors();
   } else if(strcasecmp(argv[1],"delete") == 0) {
      frame->deleteRoi();
   } else if(strcasecmp(argv[1],"setColor") == 0 && argc>2) {
      frame->setRoiColor(argv[2]);
   } else if(strcasecmp(argv[1],"setOpaque") == 0 && argc>2) {
      frame->setRoiOpaque(atoi(argv[2]));
   } else if(strcasecmp(argv[1],"setHeight") == 0 && argc>2) {
      frame->setRoiHeight(atoi(argv[2]));
   } else if(strcasecmp(argv[1],"load") == 0 || strcasecmp(argv[1],"save") == 0) {
      char path[MAXSTR2];
      if(argc>2 && argv[2][0] == '/') {
        sprintf(path,"%s",argv[2]);
      } else if(argc>2) {
        sprintf(path,"%s/%s",curexpdir, argv[2]);
      } else {
        sprintf(path,"%s/ROIs/specRois",curexpdir);
      }
      if(strcasecmp(argv[1],"load") == 0) {
	frame->loadRois(path);
        frame->showRois(true);
      } else frame->getRoiList()->saveRois(path);
   } else if(strcasecmp(argv[1],"autoAdjust") == 0 && argc>2) {
      if(strcasecmp(argv[2],"off") == 0) frame->getRoiList()->autoAdjusting=false;
      else frame->getRoiList()->autoAdjusting=true;
   } else if(strcasecmp(argv[1],"autoAdjust") == 0) {
      frame->getRoiList()->autoAdjust();
      frame->displayTop();
   } else if(strcasecmp(argv[1],"autoDefine") == 0 && argc>3) {
      frame->getRoiList()->autoDefine(frame->getSelTraceList(), dataInfo,
	atof(argv[2]), atof(argv[3]) );
      frame->showRois(true);
   } else if(strcasecmp(argv[1],"autoDefine") == 0) {
      frame->getRoiList()->autoDefine(frame->getSelTraceList(), dataInfo);
      frame->showRois(true);
   }

   frame->updateMenu();

   RETURN;
}

int AspFrameMgr::aspSession(int argc, char *argv[], int retc, char *retv[]) {
   if(argc<2) {
	Winfoprintf("Usage: aspSession('save'/'load'<,session_file>)");
	RETURN;
   }

   char path[MAXSTR2];
      if(argc>2 && argv[2][0] == '/') {
        sprintf(path,"%s",argv[2]);
      } else if(argc>2) {
        sprintf(path,"%s/%s",curexpdir, argv[2]);
      } else {
        sprintf(path,"%s/aspSession",curexpdir);
      }

   AspFrameMgr *mgr = AspFrameMgr::get();

   spAspFrame_t frame = mgr->getCurrentFrame();
   if(frame == nullAspFrame) {
	Winfoprintf("Frame is not defined.");
	RETURN;
   }

   if(strcasecmp(argv[1],"savef") == 0) { // save also trace data
     int ret = frame->saveSession(path,true); 
     if(retc>0) retv[0]=realString((double)ret); 
   } else if(strcasecmp(argv[1],"save") == 0) {
     int ret = frame->saveSession(path); 
     if(retc>0) retv[0]=realString((double)ret); 
   } else if(strcasecmp(argv[1],"load") == 0) {
     int ret = frame->loadSession(path); 
     if(retc>0) retv[0]=realString((double)ret); 
     if(retc>1) retv[1]=realString((double)(frame->getTraceList()->getSize()));
     if(retc>2) retv[2]=realString((double)(frame->getSelTraceList()->getSize()));
   } else if(strcasecmp(argv[1],"test") == 0) {
     int ntraces, straces,dataok;
     int ret = frame->testSession(path,ntraces, straces,dataok);
     if(retc>0) retv[0]=realString((double)ret);
     if(retc>1) retv[1]=realString((double)ntraces);
     if(retc>2) retv[2]=realString((double)straces);
     if(retc>3) retv[3]=realString((double)dataok);
   }

   frame->updateMenu();

   RETURN;
}

int AspFrameMgr::getNumFrames() {
   return frameList->size();
}

// delete all frames except first one.
void AspFrameMgr::removeFrames() {

   //jframeFunc("closeall",0,0,0,0,0);
   deleteFrames();

   aspFrameList::iterator fi=frameList->begin();
   ++fi;
   while(fi!=frameList->end()) {
	frameList->erase(fi->first);
	fi=frameList->begin();
	++fi;
   }
}

void AspFrameMgr::removeFrame(int id) {

   if(id<1) id = currentFrameID;
   //jframeFunc("close",id,0,0,0,0);
   deleteFrame(id);

   aspFrameList::iterator fi=frameList->begin();
   while(fi!=frameList->end()) {
	if(fi->first == id) {
/*
	   if(id<=1) jframeFunc("open",1,0,0,0,0); // cannot delete first frame
	   else frameList->erase(fi->first);
*/
	   frameList->erase(fi->first);
	   break;
	}
	++fi;
   }
}

void AspFrameMgr::makeFrames(int n, string type) {

   if(n<1) n=1;
   if(n>9) {
	n=9;
	Winfoprintf("Max # of frame is 9.");
   }
  
   int nf = frameList->size();

   if(type == "overlay") {
	int i;
	for(i=0;i<nf;i++) {
	   jframeFunc("reopen",i+1,0,0,0,0); // this set max size
	}
	return;
   } 

   int pixstx,pixsty,pixwd,pixht;
   get_main_win_size(&pixstx, &pixsty, &pixwd, &pixht);
   pixwd = (pixwd > 0) ? pixwd:100;
   pixht = (pixht > 0) ? pixht:100;
   size_t pos;

   // reuse exsiting frames
   clearAllFrame();
   if(type == "vert") {
	pixht /= n; 
	int i;
	for(i=n;i<nf;i++) { // remove extra frames
	    removeFrame(i+1);   
	}
	i=1;
	while(i<=n) {
	   if(i>nf) {
   	   	addFrame("graphics","",i, 0, (i-1)*pixht, pixwd,pixht);
	   } else {
		jframeFunc("open",i,0,(i-1)*pixht,pixwd,pixht);
	   }
   	   addFrame("graphics","",i, 0, (i-1)*pixht, pixwd,pixht);
	   i++;
	}
   } else if(type=="horiz") {
	pixwd /= n; 
	int i;
	for(i=n;i<nf;i++) {
	    removeFrame(i+1);   
	}
	i=1;
	while(i<=n) {
	   if(i>nf) {
   	     	addFrame("graphics","",i, (i-1)*pixwd, 0, pixwd,pixht);
	   } else {
		jframeFunc("open",i,(i-1)*pixwd,0,pixwd,pixht);
	   }
	   i++;
	}
   } else if((pos = type.find("x")) != string::npos) {
	int rows = atoi(type.substr(0,pos).c_str());
	int cols = atoi(type.substr(pos+1).c_str());
	if(rows<1) rows=1;
	if(cols<1) cols=1;
   	if((rows*cols)>9) {
	    rows=3;
	    cols=3;
	    Winfoprintf("Max # of frame is 9.");
	}
	pixwd /= cols;	
	pixht /= rows;	
	n = rows*cols;
	int i,j,k;
	for(i=n;i<nf;i++) {
	    removeFrame(i+1);   
	}
	for(i=0;i<rows;i++) {
	   for(j=0;j<cols;j++) {
		k=1 + i*cols + j;
		if(k>nf) {
   	           addFrame("graphics","",k, j*pixwd, i*pixht, pixwd,pixht);
		} else {
		   jframeFunc("open",k,j*pixwd,i*pixht,pixwd,pixht);
		}
	   }
	}
   } else if(n>1) { //type="auto"

	int rows,cols;
	if(pixwd > pixht) {
	   double ratio = (double)pixwd/(double)pixht;
 	   rows=1;
	   cols=n/rows;
	   while(((double)cols/(double)rows) > ratio) {
		rows++;
		cols = (int) ((double)n/(double)rows + 0.5);	
	   }
	   if(rows>1) {
		rows--;
		cols = (int) ((double)n/(double)rows + 0.5);	
	   }
	} else {
	   double ratio = (double)pixht/(double)pixwd;
 	   cols=1;
	   rows=n/cols;
	   while(((double)rows/(double)cols) > ratio) {
		cols++;
		rows = (int) ((double)n/(double)cols + 0.5);	
	   }
	   if(cols>1) {
		cols--;
		rows = (int) ((double)n/(double)cols + 0.5);	
	   }
	}
	pixwd /= cols;	
	pixht /= rows;	
	n = rows*cols;
	int i,j,k;
	for(i=n;i<nf;i++) {
	    removeFrame(i+1);   
	}
	for(i=0;i<rows;i++) {
	   for(j=0;j<cols;j++) {
		k=1 + i*cols + j;
		if(k>nf) {
   	           addFrame("graphics","",k, j*pixwd, i*pixht, pixwd,pixht);
		} else {
		   jframeFunc("open",k,j*pixwd,i*pixht,pixwd,pixht);
		}
	   }
	}
   } else {
   	removeFrames(); // this does not remove the first frame.
   } 
   
   //draw();
}

spAspFrame_t AspFrameMgr::getCurrentFrame() {
   return getFrame(currentFrameID);
}

spAspFrame_t AspFrameMgr::getFirstFrame(aspFrameList::iterator& frameItr) {
    if (frameList && frameList->size() > 0) {
        frameItr = frameList->begin();
        if (frameItr != frameList->end()) {
            return frameItr->second;
        }
    }
    return (spAspFrame_t)NULL;
}

spAspFrame_t AspFrameMgr::getNextFrame(aspFrameList::iterator& frameItr) {
    if (++frameItr == frameList->end()) {
        return (spAspFrame_t)NULL;
    }
    return frameItr->second;
}

spAspFrame_t AspFrameMgr::getFrame(int id) {

   if(id < 1) id = currentFrameID;
   aspFrameList::iterator fi;
   for(fi=frameList->begin(); fi!=frameList->end(); ++fi) {
	if(fi->first == id) return fi->second;
   }
   return nullAspFrame;
}

void AspFrameMgr::clearAllFrame() {
   aspFrameList::iterator fi;
   spAspFrame_t frame;
   for (frame= getFirstFrame(fi); frame != nullAspFrame; frame= getNextFrame(fi)) {
      frame->clear();
   } 
}

void AspFrameMgr::clearFrame(int id) {
   spAspFrame_t frame = getFrame(id);
   if(frame != nullAspFrame) frame->clear();
}

int AspFrameMgr::frameFunc(char *keyword, int frameID, int x, int y, int w, int h) {
    string key = string(keyword);

//Winfoprintf("frameFunc %s %d %d %d %d %d",keyword,frameID,x,y,w,h);
// frameID starts from 1 
    if(frameID < 1) frameID = currentFrameID;
    spAspFrame_t frame = getFrame(frameID);

// NOTE: x,y are not used, should always be zero.
    if(key=="open" || key=="new") {
        if(frame != nullAspFrame) frameList->erase(frameID);
       	//frame = spAspFrame_t(new AspFrame(frameID,x,y,w,h));
       	frame = spAspFrame_t(new AspFrame(frameID,0,0,w,h));
   	frameList->insert(aspFrameList::value_type(frameID, frame)); 
	AspMouse::setState(AspMouse::noState);
	return 0;
    }
    if(frame == nullAspFrame) return 0;

    if(key=="clearROIs") { // called by rt with frameID=0.
//	frame->clearAnnotations();
	clearAllFrame();
	return 0;
    } else if(key=="clearAspSpec") { // called by jexp with frameID=0 
	if(frame->ownScreen()) { // clear asp rois 
	     frame->clearAnnotations();
	     Jturnoff_aspMouse();
	}
	//frame->getTraceList()->clearList();
	//frame->getSelTraceList()->clearList();
	frame->initSpecFlag(0); // clear Asp spectral display
	return 0;
    } else if(key=="clear") { // called by jexp with frameID=0 
	clearAllFrame();
	return 0;
    } else if(key=="ds" || key=="dconi") { // called by ds with frameID=0
	frame->setDefaultFOV();
	frame->displayTop();
	return 0;
    } else if(key=="resetVscale") {
	frame->resetYminmax(); // clear data min max for vertical display
	return 0;
    } else if(key=="redraw") {
	frame->draw(); 
	return frame->ownScreen();
    }

    if(key=="resize") {
	frame->setSize(w,h);
    } else if(key=="loc") {
//	frame->setLoc(x,y);
    } else if(key=="active") {
	frame->setSize(w,h);
	currentFrameID = frameID;
    } 

    // Note, x,y,w,h are x,y,prevX,prevY
    if(key=="enddrag") AspMouse::endDrag(frame,x,y,w,h);

    return frame->ownScreen();
}

spAspFrame_t AspFrameMgr::getFrame(AspFrame *frame) {
   aspFrameList::iterator fi;
   for(fi=frameList->begin(); fi!=frameList->end(); ++fi) {
	if(fi->second.get() == frame) return fi->second;
   }
   return nullAspFrame;

}
