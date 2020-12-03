/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "AspFrame.h"
#include "AspFrameMgr.h"
#include "AspDataInfo.h"
#include "AspUtil.h"
#include "AspMouse.h"
#include "AspDis1D.h"
#include "AspDisPeaks.h"
#include "AspDisInteg.h"
#include "AspDisAnno.h"
#include "AspDisRegion.h"
#include "aipSpecDataMgr.h"

extern "C" {
void Wclear(int window);
void clearGraphFunc(); 
int saveProcpar(char* path);
int P_read(int, char*);
void set_top_frame_on_top(int onTop);
int currentindex();
int getAxisOnly();
int do_mkdir(const char *dir, int psub, mode_t mode);
}

spAspFrame_t nullAspFrame = spAspFrame_t(NULL);

AspFrame::AspFrame(int i, double x, double y, double w, double h) {
    id = i;
    int xmargin=xcharpixels;
    int ymargin=ycharpixels;
    if(w<4*xmargin) w=4*xmargin+1.0;
    if(h<5*ymargin) h=5*ymargin+1.0;
    pixstx = x;
    pixsty = y;
    pixwd = w;
    pixht = h;
    pstx = x+2*xmargin;
    psty = y+ymargin;
    pwd = w - 4*xmargin;
    pht = h - 5*ymargin;
     
    dataMap = new AspDataMap;
    cellList = new AspCellList(id);
    roiList = new AspRoiList(id);
    traceList = new AspTraceList();
    selTraceList = new AspTraceList();
    peakList = new AspPeakList();
    integList = new AspIntegList();
    annoList = new AspAnnoList();
    defaultData = nullAspData;
    specFlag=0;
    disFlag=0;
    annoFlag=0;
    peakFlag=0;
    integFlag=0;
    axisFlag=0;
    threshSelected=false;    
    rows=1;
    cols=1;
    currentTrace=0;
    frameSel=0;
    aspMode=false;
    annoTop=true;
}

AspFrame::~AspFrame() {
    delete dataMap;
    delete cellList;
    delete roiList;
    delete traceList;
    delete selTraceList;
    delete peakList;
    delete integList;
    delete annoList;
}

void AspFrame::setSize(int w, int h) {
  pixwd=w;
  pixht=h;
  int xmargin=xcharpixels;
  int ymargin=ycharpixels;
  pstx = pixstx+2*xmargin;
  psty = pixsty+ymargin;
  pwd = w - 4*xmargin;
  pht = h - 5*ymargin;
  if(!specFlag) setDefaultFOV();
}

void AspFrame::setLoc(int x, int y) {
  pixstx=x;
  pixsty=y;
}

spAspDataInfo_t AspFrame::getDefaultDataInfo(bool update) {
   if(defaultData == nullAspData) { 
	defaultData = spAspDataInfo_t(new AspDataInfo());
   }
   if(update) defaultData->updateDataInfo();

   return defaultData;
}

void AspFrame::setFullSize() {
    int xmargin=xcharpixels;
    int ymargin=ycharpixels;
    pstx = pixstx+2*xmargin;
    psty = pixsty+ymargin;
    pwd = pixwd - 4*xmargin;
    pht = pixht - 5*ymargin;
}

void AspFrame::setCellFOV(int rows, int cols) {
   Wclear(2); // clear graphics window
   Wsetgraphicsdisplay("");
   clearGraphFunc(); // so won't redo ds etc...
   
   if(!aspMode) setFullSize();
   cellList->clearList();

   aspMode=true;

   spAspDataInfo_t dataInfo = getDefaultDataInfo(true);
   double vstx = dataInfo->haxis.start;
   double vsty = dataInfo->vaxis.start;
   double vwd = dataInfo->haxis.width;
   double vht = dataInfo->vaxis.width;

   double mx,my,mw,mh;
   getFOVLimits(mx,my,mw,mh);

   if(pstx < mx) pstx=mx;
   if((pstx+pwd) > (mx+mw)) pwd=(mx+mw)-pstx;;
   if(psty < my) psty=my;
   if((psty+pht) > (my+mh)) pht=(my+mh)-psty;;

/*
// for now, use FOV of ds or dconi
   pstx  = ((double)(mnumxpnts-right_edge)*(wcmax-sc-wc)/wcmax);
   pwd  = ((double)(mnumxpnts-right_edge)*wc/wcmax);
   psty = ((double)(mnumypnts-ymin)*sc2/wc2max)+ymin;
   pht = ((double)(mnumypnts-ymin)*wc2/wc2max);
   psty = mnumypnts - (psty+pht);

   if(axisFlag & AX_SOUTH) {
   } else { 
	pht += ymin; 
   }

   if(pstx < xcharpixels) {
	pwd -= (xcharpixels - pstx);
	pstx = xcharpixels;
   }
   if((axisFlag & AX_WEST)) {
      double west = xcharpixels * 8;
      if (west > pstx && west < pwd) {
                 west = west - pstx;
                 pstx += west;
                 pwd -= west;
      }
   }
   if ((pstx + pwd) > (pixstx+pixwd-xcharpixels)) {
	pwd -= (pstx + pwd -pixstx - pixwd + xcharpixels); 
   }
   if(psty < ycharpixels) {
	pht -= (ycharpixels - psty);
	psty = ycharpixels;
   }
   if((psty + pht) > (pixsty+pixht-2*ycharpixels)) {
        pht -= (psty + pht -pixsty - pixht + 2*ycharpixels);
   }
*/

// divide into rows, cols.
   double wd = pwd/cols;
   double ht = pht/rows;
   int i,j,k;
   for(i=0;i<rows;i++) {
      for(j=0;j<cols;j++) {
        k=1 + i*cols + j;
   	spAspCell_t cell = spAspCell_t(new AspCell(pstx+j*wd,psty+i*ht,wd,ht,vstx,vsty,vwd,vht));
   	cell->setAxisNames(dataInfo->haxis.name,dataInfo->vaxis.name);
	cell->setDataInfo(dataInfo);
   	cellList->addCell(k,cell); 
   // DEBUG
   // 	AspUtil::drawBox(pstx+j*wd,psty+i*ht,wd,ht,3);
      }
   }
}

// this FOV should be exactly the same as "ds" or "dconi", so ROIs can be displayed correctly
void AspFrame::setDefaultFOV() {

   cellList->clearList();

   aspMode=false;

   spAspDataInfo_t dataInfo = getDefaultDataInfo(true);
   double vstx = dataInfo->haxis.start;
   double vsty = dataInfo->vaxis.start;
   double vwd = dataInfo->haxis.width;
   double vht = dataInfo->vaxis.width;

   getPlotBox_Pix(&pstx, &psty, &pwd, &pht);
   if(pwd<=0 || pht<=0) {
	pstx=0.0;
	psty=0.0;
	pwd=pixwd;
	pht=pixht;
   }
   // these are dfpnt,dnpnt,dfpnt2,dnpnt2, starting from lower left corner.
   // they need to be converted to upper left corner.
   // i.e. y is reversed. 
   //psty = pixht - (psty+pht);
   psty = mnumypnts - (psty+pht);

   // add 2.0*dispcalib for 1D display (so spectrum is a little above axis).
   if(dataInfo->rank == 1) {
      double dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
      pht = pht + 2.0*dispcalib;
   }

//Winfoprintf("### pixstx,pixsty,pixwd,pixht %f %f %f %f",pixstx,pixsty,pixwd,pixht);
   // DEBUG
   // AspUtil::drawBox(pixstx, pixsty, pixwd, pixht,2);
   // AspUtil::drawBox(pstx, psty, pwd, pht, 3);

//Winfoprintf("setDefaultFOV %f %f %f %f %f %f %f %f",pstx, psty, pwd, pht, vstx, vsty, vwd, vht);

   // Note, start values vstx,vsty should be upper left, otherwise the axis
   // is reversed, and vwd, vht will be negative. 
   spAspCell_t cell = spAspCell_t(new AspCell(pstx,psty,pwd,pht,vstx,vsty,vwd,vht));
   cell->setAxisNames(dataInfo->haxis.name,dataInfo->vaxis.name);
   cell->setDataInfo(dataInfo);

   cellList->addCell(0,cell); 
}

void AspFrame::resetYminmax() {

   spAspDataInfo_t dataInfo = getDefaultDataInfo(true);
   dataInfo->resetYminmax();
}

void AspFrame::draw() {

     if(annoTop) set_top_frame_on_top(1);
     else set_top_frame_on_top(0);
     drawSpec(); // currently do nothing

     callAspDisplayListeners(); 

     displayTop();
}

bool AspFrame::doDs() {
   return (specFlag & SPEC_DS || specFlag & SPEC_DSS || specFlag & SPEC_GRID);
}

void AspFrame::drawSpec() {

   if(specFlag == 0) return;

   // for now we require that an exp data is loaded
   if(!(getDefaultDataInfo()->hasData)) return;

   set_top_frame_off();
   if(doDs()) dsAgain(); 
   if(frameSel) drawPlotBox();
}

void AspFrame::displayTop()
{
     clear_top_frame();
     //if(!annoFlag && !peakFlag && !integFlag) return;

     // for now we require that an exp data is loaded
     if(!(getDefaultDataInfo()->hasData)) return;

     if(cellList->getCellMap()->size() < 1) return;
 
     set_top_frame_on();
     if(annoFlag & ANN_ROIS) drawRois();
     if(peakFlag != 0) AspDisPeaks::display(AspFrameMgr::get()->getFrame(id));
     if(integFlag != 0) AspDisInteg::display(AspFrameMgr::get()->getFrame(id));
     if(annoFlag & ANN_ANNO) AspDisAnno::display(AspFrameMgr::get()->getFrame(id));
     if(AspDisRegion::getRegionFlag() != 0) AspDisRegion::display(AspFrameMgr::get()->getFrame(id));
     set_top_frame_off();
}

void AspFrame::clearRois()
{
     showRois(false);
     roiList->getRoiMap()->clear();
}

void AspFrame::drawRois() {
// for now, only draw on default frame 
   if(cellList->getCellMap()->size() != 1) return;

   AspCellMap::iterator ci;
   spAspCell_t cell;
   for (cell= cellList->getFirstCell(ci); cell != nullAspCell; cell= cellList->getNextCell(ci)) {
        roiList->drawRois(cell);
   }
}

void AspFrame::unselectRois() {
   roiList->unselectRois();
}

spAspRoi_t AspFrame::selectRoi(int x, int y, bool handle) {
    spAspCell_t cell =  selectCell(x,y);
    if(cell == nullAspCell) return nullAspRoi;

    spAspRoi_t roi = roiList->selectRoi(cell, x,y,handle); 

    if(roi != prevRoi || (roi != nullAspRoi && roi->mouseOverChanged)) displayTop();

    prevRoi = roi; 

    return roi;
}

// annotation etc.. only use first cell. multiple cells exist only for grid display.
spAspCell_t  AspFrame::getFirstCell() {
     if(cellList->getCellMap()->size() < 1) {
	if(specFlag == 0) setDefaultFOV();
	else setCellFOV(1,1); 
     }
     AspCellMap::iterator itr;
     return getCellList()->getFirstCell(itr);
}

spAspCell_t AspFrame::selectCell(int x, int y) {
     if(cellList->getCellMap()->size() < 1) {
	if(specFlag == 0) setDefaultFOV();
	else setCellFOV(1,1); 
     }
    
    return cellList->selectCell(x,y);

}

int AspFrame::getCursorMode() {
    if(specFlag == 0) {
	if(getString("crmode","c") == "b") return 2;
	else  return 1;
    } else if(AspMouse::getState() == AspMouse::cursor2) return 2;
    else if(AspMouse::getState() == AspMouse::cursor1) return 1;
    else return 0;
}

void AspFrame::deleteRoi()
{
    double c1,c2;
    double reffrq = getDefaultDataInfo()->haxis.scale;
    if(getCursorMode() == 2) {
      P_getreal(CURRENT, "cr", &c1, 1);
      P_getreal(CURRENT, "delta", &c2, 1);
      c2 = c1 -c2;
      if (reffrq>0) {
	c1 /= reffrq;
	c2 /= reffrq;
      }
      roiList->deleteRoi(c1,c2);
    } else {
      P_getreal(CURRENT, "cr", &c1, 1);
      if (reffrq>0) {
	c1 /= reffrq;
      }
      roiList->deleteRoi(c1);
    }
    displayTop();
}

void AspFrame::deleteRoi(spAspRoi_t roi)
{
    roiList->deleteRoi(roi);
    displayTop();
        
}

void AspFrame::deleteRoi(int x, int y)
{
    spAspCell_t cell =  selectCell(x,y);
    if(cell == nullAspCell) return;

    spAspRoi_t roi = roiList->selectRoi(cell, x,y); 
    if(roi != nullAspRoi) {
	deleteRoi(roi);
    }
}

spAspRoi_t AspFrame::addRoi(int type, int x, int y) {
    spAspCell_t cell =  selectCell(x,y);
    if(cell == nullAspCell) return nullAspRoi;

    spAspRoi_t roi = nullAspRoi;
    spAspCursor_t cursor1, cursor2;
    if(type == ROI_BOX) { 
	double vx = cell->pix2val(HORIZ,x);
	double vy = cell->pix2val(VERT,y);
        cursor1 = spAspCursor_t(new AspCursor(vx,vy,cell->getXname(),cell->getYname()));
        cursor2 = spAspCursor_t(new AspCursor(vx,vy,cell->getXname(),cell->getYname()));
    	roi = spAspRoi_t(new AspRoi(cursor1,cursor2,roiList->getRoiMap()->size()));
    	roiList->addRoi(roi);
    } else if(type == ROI_BAND) {
	double vx = cell->pix2val(HORIZ,x);
        cursor1 = spAspCursor_t(new AspCursor(vx,cell->getXname()));
        cursor2 = spAspCursor_t(new AspCursor(vx,cell->getXname()));
    	roi = spAspRoi_t(new AspRoi(cursor1,cursor2,roiList->getRoiMap()->size()));
    	roiList->addRoi(roi);
    }
    setAnnoFlag(ANN_ROIS);
    return roi;
}

// c1, c2 are in ppm, c1 is downfield, bigger ppm.
void AspFrame::addRoiFromCursors(double c1, double c2) {

    if(c1<c2) {double tmp=c1; c1=c2; c2=tmp;}

    spAspDataInfo_t dataInfo = getDefaultDataInfo();
    spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(c1,dataInfo->haxis.name));
    spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(c2,dataInfo->haxis.name));
    spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,roiList->getRoiMap()->size()));
    roiList->addRoi(roi);
    setAnnoFlag(ANN_ROIS);
    displayTop();
}

void AspFrame::addRoiFromCursors(double c11, double c12, double c21, double c22) {

    if(c11<c12) {double tmp=c11; c11=c12; c12=tmp;}
    if(c21<c22) {double tmp=c21; c21=c22; c22=tmp;}

    spAspDataInfo_t dataInfo = getDefaultDataInfo();
    spAspCursor_t cursor1 = spAspCursor_t(new AspCursor(c11,c21,dataInfo->haxis.name,dataInfo->vaxis.name));
    spAspCursor_t cursor2 = spAspCursor_t(new AspCursor(c12,c22,dataInfo->haxis.name,dataInfo->vaxis.name));
    spAspRoi_t roi = spAspRoi_t(new AspRoi(cursor1,cursor2,roiList->getRoiMap()->size()));
    roiList->addRoi(roi);
    setAnnoFlag(ANN_ROIS);
    displayTop();
}

void AspFrame::addRoiFromCursors() {
  if(getCursorMode() != 2) {
	Winfoprintf("Try again after selecting second cursor with right mouse button.");
	return;
  }
  spAspDataInfo_t dataInfo = getDefaultDataInfo();
  if(dataInfo->vaxis.rev) { // 1D band
    double c1,c2;
    P_getreal(CURRENT, "cr", &c1, 1);
    P_getreal(CURRENT, "delta", &c2, 1);
    c2 = c1 - c2;
    double reffrq = dataInfo->haxis.scale;
    if (reffrq>0) {
	c1 /= reffrq;
	c2 /= reffrq;
    }
    addRoiFromCursors(c1,c2);
  } else { // 2D box
    double c11,c12,c21,c22;
    P_getreal(CURRENT, "cr", &c11, 1);
    P_getreal(CURRENT, "delta", &c12, 1);
    c12 = c11 - c12;
    double reffrq = dataInfo->haxis.scale;
    if (reffrq>0) {
	c11 /= reffrq;
	c12 /= reffrq;
    }
    P_getreal(CURRENT, "cr1", &c21, 1);
    P_getreal(CURRENT, "delta1", &c22, 1);
    c22 = c21 - c22;
    double reffrq1 = dataInfo->vaxis.scale;
    if (reffrq1>0) {
	c21 /= reffrq1;
	c22 /= reffrq1;
    }
    addRoiFromCursors(c11,c12,c21,c22);
  }
}

void AspFrame::selectRoiHandle(spAspRoi_t roi, int x, int y,bool handle)
{
    spAspCell_t cell =  selectCell(x,y);
    if(cell == nullAspCell) return;

    bool creating = AspMouse::creating;
    int prevX=AspMouse::prevX;
    int prevY=AspMouse::prevY;

    if(creating && roi->getRank() > 1) { 
	if(x >= prevX && y >= prevY) roi->mouseOver = HANDLE3;
	else if(x >= prevX) roi->mouseOver = HANDLE2;
	else if(y >= prevY) roi->mouseOver = HANDLE4;
	else roi->mouseOver = HANDLE1;
    } else if(creating) {
	if(x >= prevX) roi->mouseOver = LINE2;
	else roi->mouseOver = LINE1;
    } else {
        roi->selectHandle(cell,x, y,handle);
    }

    displayTop();
}

void AspFrame::modifyRoi(spAspRoi_t roi, int x, int y)
{ 
    if(roi == nullAspRoi) return;

    spAspCell_t cell =  selectCell(x,y);
    if(cell == nullAspCell) return;

    roi->modify(cell,x, y, AspMouse::prevX, AspMouse::prevY);
    displayTop();
}

void AspFrame::showRois(bool b) {
    getRoiList()->setShowRois(b);
    setAnnoFlag(ANN_ROIS,b);
   
    displayTop();
}

#ifdef TEST
void AspFrame::registerAspDisplayListener(DFUNC_t func) 
{
    AspDisplayListenerList::iterator begin = displayListenerList.begin();
    AspDisplayListenerList::iterator end = displayListenerList.end();
    if (func && find(begin, end, func) == end) {
        // Only insert it if non-null and not already there
        displayListenerList.push_back(func);
    }
}

void AspFrame::unregisterAspDisplayListener(DFUNC_t func) 
{
    if (func) {
        displayListenerList.remove(func);
    }
}
#endif

void AspFrame::callAspDisplayListeners() 
{

    AspDisplayListenerList::iterator itr;
    int k = displayListenerList.size();
    itr = displayListenerList.begin();
    while ( k > 0 )
    {
        DFUNC_t func = *itr;
        (*func)(id);
        k--;
        itr++;
    }
}

void AspFrame::setRoiColor(char *name)
{
    roiList->setRoiColor(name);
    displayTop();
}

void AspFrame::setRoiOpaque(int op)
{
    roiList->setRoiOpaque(op);
    displayTop();
}

void AspFrame::setRoiHeight(int h)
{
    roiList->setRoiHeight(h);
    displayTop();
}

// use first cell to determine wheither rois suitable for current FOV 
void AspFrame::loadRois(char *path) {
   spAspCell_t cell = getFirstCell();
   if(cell != nullAspCell) roiList->loadRois(cell,path);
}

void AspFrame::clear() {
   clearAnnotations();
   getSelTraceList()->clearList();
   getTraceList()->clearList();
   getPeakList()->clearList();
   initPeakFlag(0);
   initDisFlag(0);
   initSpecFlag(0);
   Wclear(2);
}

// TODO: init according to a preference parameter?
void AspFrame::initSpecFlag(int flag) {
   if(flag == specFlag) return;
   specFlag = flag;
   if(flag == 0) {
	axisFlag = 0;
	disFlag = 0;
	AspMouse::setState(AspMouse::noState);
   }
}

void AspFrame::setSpecFlag(int flg, bool on) {
   if(on) specFlag |= flg;
   else specFlag &= ~flg;
}

void AspFrame::initDisFlag(int flag) {
   disFlag = flag;
}

void AspFrame::setDisFlag(int flg, bool on) {
   if(on) disFlag |= flg;
   else disFlag &= ~flg;
}

void AspFrame::initAnnoFlag(int flag) {
   annoFlag = flag;
}

void AspFrame::setAnnoFlag(int flg, bool on) {
    if(on) annoFlag |= flg;
    else annoFlag &= ~flg;
}

void AspFrame::initPeakFlag(int flag) {
   peakFlag = flag;
}

void AspFrame::setPeakFlag(int flg, bool on) {
    if(on) peakFlag |= flg;
    else peakFlag &= ~flg;
}

void AspFrame::initIntegFlag(int flag) {
   integFlag = flag;
}

void AspFrame::setIntegFlag(int flg, bool on) {
    if(on) integFlag |= flg;
    else integFlag &= ~flg;
}

void AspFrame::initAxisFlag(int flag) {
   axisFlag = flag;
}

void AspFrame::setAxisFlag(int flg, bool on) {
    if(on) axisFlag |= flg;
    else axisFlag &= ~flg;
}

// called by draw/redraw after cells are ccreated and filled with data.
void AspFrame::dsAgain() {

//   if(AspMouse::getState() == AspMouse::noState )
//	AspMouse::setState(AspMouse::cursor1);

   AspDis1D::display(AspFrameMgr::get()->getFrame(id));
   Wsetgraphicsdisplay("asp1D('again')");

}

void AspFrame::specVS(int clicks, double factor) {
   if(doDs()) AspDis1D::specVS(id,clicks,factor);
}

void AspFrame::showCursor(int x, int y, int mode, int cursor) {
   if(doDs()) AspDis1D::showCursor(id,x,y,mode,cursor);
}

void AspFrame::startZoom(int x, int y) {
   if(doDs()) writelineToVnmrJ("vnmrjcmd", "canvas rubberarea");
   //else if(doDcon()) writelineToVnmrJ("vnmrjcmd", "canvas rubberbox");
}

void AspFrame::setZoom(int x, int y, int prevX, int prevY) {
   if(doDs()) AspDis1D::setZoom(id,x,y,prevX,prevY);
}

void AspFrame::zoomSpec(int x, int y, int mode) {
   if(doDs()) AspDis1D::zoomSpec(id,x,y,mode);
}

void AspFrame::panSpec(int x, int y, int prevX, int prevY, int mode) {
   if(doDs()) AspDis1D::panSpec(id,x,y,prevX,prevY,mode);
}

bool AspFrame::ownScreen() {
   return (aspMode && specFlag != 0);
}

void AspFrame::clearAnnotations() {
   initAnnoFlag(0);
   getAnnoList()->clearList();
   roiList->getRoiMap()->clear();
}

void AspFrame::writeFields(char *str) {
   int x=(int)pixstx+4;
   int y=(int)(pixsty+pixht);
   int w=(int)pixwd;
   int h=ycharpixels;
   AspUtil::clearFields(x,y-ycharpixels,w,h);
   AspUtil::writeFields(str,x,y,w,h);
}

int AspFrame::saveSession(char *path, bool full) {
// path is a full path for session file, or the parent directory of session file.
// path file or directory may not exist
   struct stat fstat;
   string dir = "";
   char sessionFile[MAXSTR2];
   if (stat(path, &fstat) != 0) { // path does not exist
      if(full) {
         dir = string(path);
	 sprintf(sessionFile,"%s/aspSession",path);
      } else {
         string tmp = string(path);
	 dir = tmp.substr(0,tmp.find_last_of("/"));
	 sprintf(sessionFile,"%s",path);
      }
      // make sure dir exists
      struct stat dstat;
      if (stat(dir.c_str(), &dstat) != 0) {
         if (do_mkdir(dir.c_str(), 1, 0777))
         {
	        Winfoprintf("Failed to make directory %s.",dir.c_str());
	        return 0;
         }
      }

   } else if(fstat.st_mode & S_IFDIR) { // is a directory
         dir = string(path);
	 sprintf(sessionFile,"%s/aspSession",path);
   } else {
         string tmp = string(path);
	 dir = tmp.substr(0,tmp.find_last_of("/"));
	 sprintf(sessionFile,"%s",path);
   }

   FILE *fp;
   if(!(fp = fopen(sessionFile, "w"))) {
	Winfoprintf("Failed to create session file %s.",sessionFile);
	return 0;
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

   spAspDataInfo_t dataInfo = getDefaultDataInfo();
   fprintf(fp,"# Created by %s on %s at machine %s.\n",UserName,datetim,HostName);

   // save display flags 
   fprintf(fp,"#disFlag,axisFlag,annoFlag,specFlag,peakFlag,integFalg\n");
   fprintf(fp,"disFlags: %d %d %d %d %d %d\n",disFlag,axisFlag,annoFlag,specFlag,peakFlag,integFlag);

   // save traces in order of traceInd
   fprintf(fp,"#number_of_traces, n, vp, vo\n");
   fprintf(fp,"number_of_traces: %d %f %f\n",traceList->getSize(),
	dataInfo->getVpos()-2,dataInfo->getVoff());

   if(full) { // save trace data (full session)
     // save current parameters
     char procPath[MAXSTR2];
     sprintf(procPath,"%s/procpar",dir.c_str()); 
     saveProcpar(procPath);
     fprintf(fp,"procpar: %s\n",procPath);

     spAspTrace_t trace = traceList->getTraceByInd(0);
     string root = "";
     if(trace != nullAspTrace) {
        trace->save(dir);
        root = trace->rootPath; 
        if(root != "") fprintf(fp,"root: %s\n",root.c_str());
     }
     fprintf(fp,"# traceInd,fdfpath,label,dataKey,dataInd,minX,maxX,scale,vp,color,labelFlag\n");
     int i;
     for(i=0;i<traceList->getSize(); i++) {
	trace = traceList->getTraceByInd(i);
	if(trace != nullAspTrace) { 
          if(i>0) trace->save(dir);
	  fprintf(fp,"%s\n",trace->toString(root).c_str());
	  list<spAspTrace_t> *sumList = trace->getSumTraceList();
	  list<spAspTrace_t>::iterator itr;
	  int ind = trace->traceInd+1;
	  for(itr=sumList->begin(); itr != sumList->end(); ++itr) {
	    string color = (*itr)->colorStr;
	    double min = (*itr)->getMinX() + (*itr)->getShift();
	    double max = (*itr)->getMaxX() + (*itr)->getShift();
	    fprintf(fp,"color %d %s %f %f\n",ind,color.c_str(),min,max);
	  }
	}
     }
   } else {
     spAspTrace_t trace = traceList->getTraceByInd(0);
     string root = "";
     if(trace != nullAspTrace) {
        root = trace->rootPath; 
        if(root != "") fprintf(fp,"root: %s\n",root.c_str());
     }
     fprintf(fp,"# traceInd,fdfpath,label,dataKey,dataInd,minX,maxX,scale,vp,color,labelFlag\n");
     int i;
     for(i=0;i<traceList->getSize(); i++) {
	trace = traceList->getTraceByInd(i);
	if(trace != nullAspTrace) { 
	  fprintf(fp,"%s\n",trace->toString(root).c_str());
	  list<spAspTrace_t> *sumList = trace->getSumTraceList();
	  list<spAspTrace_t>::iterator itr;
	  int ind = trace->traceInd+1;
	  for(itr=sumList->begin(); itr != sumList->end(); ++itr) {
	    if((*itr)->path != "") {
	      fprintf(fp,"%s\n",(*itr)->toString(root).c_str());
	    } else {
	      string color = (*itr)->colorStr;
              double min = (*itr)->getMinX() + (*itr)->getShift();
              double max = (*itr)->getMaxX() + (*itr)->getShift();
              fprintf(fp,"color %d %s %f %f\n",ind,color.c_str(),min,max);
	    }
	  }
	}
     }
   }

   fprintf(fp,"#indexes for displayed_traces\n");
   fprintf(fp,"displayed_traces: %s\n",selTraceList->getIndList().c_str());
   fprintf(fp,"#\n");

   char file[MAXSTR2];
   spAspFrame_t frame = AspFrameMgr::get()->getFrame(this);

   if(annoFlag & ANN_ROIS) { // save Rois
     sprintf(file,"%s/Rois",dir.c_str());
     fprintf(fp,"roiFile: Rois\n");
     getRoiList()->saveRois(file);
   }

   if(annoFlag & ANN_ANNO) { // save annotations
	sprintf(file,"%s/annos",dir.c_str());
	fprintf(fp,"annoFile: annos\n");
	AspDisAnno::save(frame, file); 
   }

   if(peakFlag) { // save peaks 
	sprintf(file,"%s/peaks",dir.c_str());
	fprintf(fp,"peakFile: peaks\n");
	AspDisPeaks::save(frame, file); 
   }

   if(integFlag) { // save integrals
	sprintf(file,"%s/integs",dir.c_str());
	fprintf(fp,"integFile: integs\n");
	AspDisInteg::save(frame, file); 
   }

   fclose(fp);
   return 1;
}

int AspFrame::loadSession(char *path) {
   struct stat fstat;
   string dir="";
   char sessionFile[MAXSTR2],file[MAXSTR2];
   if(stat(path, &fstat) != 0) {
        Winfoprintf("Error: cannot find %s.",path);
        return 0;
   } else if(fstat.st_mode & S_IFDIR) { // is a directory
         dir = string(path);
	 sprintf(sessionFile,"%s/aspSession",path);
   } else {
         string tmp = string(path);
	 dir = tmp.substr(0,tmp.find_last_of("/"));
	 sprintf(sessionFile,"%s",path);
   }

   FILE *fp;
   if(!(fp = fopen(sessionFile, "r"))) {
	Winfoprintf("Failed to open session file %s.",sessionFile);
	return 0;
   }

   traceList->clearList();	
   selTraceList->clearList();	
   peakList->clearList();	
   integList->clearList();	
   annoList->clearList();	

   char  buf[MAXSTR2], words[MAXWORDNUM][MAXSTR2], *tokptr;
   int nw, count=0;
   int ntraces = 0;
   char str[MAXSTR2];
   string root="";
   while (fgets(buf,sizeof(buf),fp)) {
      if(strlen(buf) < 1 || buf[0] == '#') continue;
          // break buf into tok of parameter names

      nw=0;
      tokptr = strtok(buf, ", :\n\r");
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, ", :\n\r");
      }

      if(nw < 2) continue;

      if(strcasecmp(words[0],"disFlags")==0 && nw > 4) {
	//e.g. disFlags: 0 132096 0 256
	disFlag = atoi(words[1]);	
	axisFlag = atoi(words[2]);	
	annoFlag = atoi(words[3]);	
	specFlag = atoi(words[4]);	
	if(nw > 5) peakFlag = atoi(words[5]);	
	if(nw > 6) integFlag = atoi(words[6]);	
      } else if(strcasecmp(words[0],"disParams")==0 && nw > 1) {
	//e.g. disParams: 22.000000 8.000000
	if(nw > 2) {
	   P_setreal(CURRENT, "vp", atof(words[1]), 1);
	   P_setreal(CURRENT, "vo", atof(words[2]), 1);
	}
      } else if(strcasecmp(words[0],"root")==0 && nw > 1) {
        root=string(words[1]);
	if(root.find_last_of("/") == root.length()-1)
	   root=root.substr(0,root.length()-1);
      } else if(strcasecmp(words[0],"procpar")==0 && nw > 1) {
         P_read(CURRENT,words[1]);
      } else if(nw > 11 && strstr(words[0],"load") == words[0]) {
        if(root != "" && words[2][0] != '/') {
	   strcpy(str,words[2]);
	   sprintf(words[2],"%s/%s",root.c_str(),str);
	}
	//e.g. load 1 /tmp/spec.fdf spec1 spec1 0 -2.022703 14.022636 1.000000 0.000000 11 0
	//e.g. load 2 /tmp/spec.fdf spec1 spec1 1 -2.022703 14.022636 1.000000 0.000000 11 0
	// each line corresponds to a AspTrace	
        spAspTrace_t trace = spAspTrace_t(new AspTrace(words,nw));
	trace->rootPath=string(root);
	sprintf(str,"%s:%d",trace->dataKey.c_str(),trace->dataInd);
	string newKey = string(str); 
	traceList->addTrace(newKey, trace);
	selTraceList->maxInd= traceList->getSize();
	ntraces++;	
	count++;

      } else if(nw > 11 && (strstr(words[0],"add") == words[0] || 
		strstr(words[0],"sub") == words[0] || strstr(words[0],"rep") == words[0])) {

        if(root != "" && words[2][0] != '/') {
	   strcpy(str,words[2]);
	   sprintf(words[2],"%s/%s",root.c_str(),str);
	}
	//e.g. add 1 /tmp/spec.fdf spec1 spec1 0 -2.022703 14.022636 1.000000 0.000000 11 0
	//e.g. sub 1 /tmp/spec.fdf spec1 spec1 1 -2.022703 14.022636 1.000000 0.000000 11 0
	// each line corresponds to a AspTrace to be add/sub to a specified trace	
        spAspTrace_t trace = spAspTrace_t(new AspTrace(words,nw));
	trace->rootPath=string(root);
	int flag = (strstr(words[0],"sub") == words[0]) ? SUB_TRACE:ADD_TRACE;
        if(strstr(words[0],"rep") == words[0]) flag = REP_TRACE;
        spAspTrace_t sumTrace = traceList->getTraceByInd(atoi(words[1])-1);
        if(sumTrace != nullAspTrace) sumTrace->sumTrace(trace,flag);
	else {
	   sprintf(str,"%s:%d",trace->dataKey.c_str(),trace->dataInd);
	   string newKey = string(str); 
	   traceList->addTrace(newKey, trace);
	   selTraceList->maxInd= traceList->getSize();
	}
	count++;
      } else if(strcasecmp(words[0],"color")==0 && nw > 4) {
	spAspTrace_t trace = traceList->getTraceByInd(atoi(words[1])-1);
	if(trace != nullAspTrace) {
	   double min = atof(words[3]);
	   double max = atof(words[4]);
	   string str = string("");
           spAspTrace_t sumTrace = spAspTrace_t(new AspTrace(0,str,str,0,0,min,max));
	   sumTrace->colorStr=string(words[2]);
	
	   list<spAspTrace_t> *sumList = trace->getSumTraceList();
	   sumList->push_back(sumTrace);
	}
      } else if(strcasecmp(words[0],"displayed_traces")==0 && nw > 1) {
	//e.g. displayed_traces: 1 2 3 4 5 6 7 8 9 10
	int i;
	for(i=1; i<nw;i++) {
	   spAspTrace_t trace = traceList->getTraceByInd(atoi(words[i])-1);
           if(trace != nullAspTrace) {
	     trace->selected=false;
             selTraceList->addTrace(trace->getKeyInd(), trace);
           }

	}
	selTraceList->maxInd= traceList->getSize();
		
      } else if(strcasecmp(words[0],"roiFile")==0 && nw > 1) {
	sprintf(file,"%s/%s",dir.c_str(),words[1]);
	loadRois(file);
      } else if(strcasecmp(words[0],"annoFile")==0 && nw > 1) {
   	spAspFrame_t frame = AspFrameMgr::get()->getFrame(this);
	sprintf(file,"%s/%s",dir.c_str(),words[1]);
	AspDisAnno::load(frame, file);
      } else if(strcasecmp(words[0],"peakFile")==0 && nw > 1) {
	sprintf(file,"%s/%s",dir.c_str(),words[1]);
   	spAspFrame_t frame = AspFrameMgr::get()->getFrame(this);
	AspDisPeaks::load(frame, file);
      } else if(strcasecmp(words[0],"integFile")==0 && nw > 1) {
	sprintf(file,"%s/%s",dir.c_str(),words[1]);
   	spAspFrame_t frame = AspFrameMgr::get()->getFrame(this);
	AspDisInteg::load(frame, file);
      } 
   }

   // display all traces if displayed_traces: is missing
   if(selTraceList->getSize() < 1) {
     for(int i=0; i<traceList->getSize(); i++) {
        spAspTrace_t trace = traceList->getTraceByInd(i);
        if(trace != nullAspTrace) {
          trace->selected=false;
          selTraceList->addTrace(trace->getKeyInd(), trace);
        }
     }
   }

   fclose(fp);

   if(specFlag) {
      AspMouse::setState(AspMouse::cursor1);
      Wsetgraphicsdisplay("");
   } else {
      AspMouse::setState(AspMouse::noState);
   } 
   getDefaultDataInfo(true);

   draw();
   return count;
}

int AspFrame::testSession(char *path, int &ntraces, int &straces, int &dataok) {
   ntraces=straces=dataok=0;

   struct stat fstat;
   if(stat(path, &fstat) != 0) {
        Winfoprintf("Error: cannot find %s.",path);
        return 0;
   }

   FILE *fp;
   if(!(fp = fopen(path, "r"))) {
	Winfoprintf("Failed to open session file %s.",path);
	return 0;
   }

   char  buf[MAXSTR2], words[MAXWORDNUM][MAXSTR2], *tokptr;
   int nw;
   int count=0;
   dataok=1;
   char str[MAXSTR2];
   string root="";
   spAspTrace_t trace;
   while (fgets(buf,sizeof(buf),fp)) {
      if(strlen(buf) < 1 || buf[0] == '#') continue;
          // break buf into tok of parameter names

      nw=0;
      tokptr = strtok(buf, ", :\n\r");
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, ", :\n\r");
      }

      if(nw < 2) continue;

      if(strcasecmp(words[0],"root")==0 && nw > 1) {
        root=string(words[1]);
	if(root.find_last_of("/") == root.length()-1)
	   root=root.substr(0,root.length()-1);

      } else if(nw > 11 && strstr(words[0],"load") == words[0]) {
        if(root != "" && words[2][0] != '/') {
	   strcpy(str,words[2]);
	   sprintf(words[2],"%s/%s",root.c_str(),str);
	}
        trace = spAspTrace_t(new AspTrace(words,nw));
	if(trace != nullAspTrace && trace->getData()) {
	  ntraces++;
	  count++;
	} else {
	  Winfoprintf("Error to load %s",buf);
	  dataok=0;
	}

      } else if(nw > 11 && (strstr(words[0],"add") == words[0] || 
		strstr(words[0],"sub") == words[0])) {
        if(root != "" && words[2][0] != '/') {
	   strcpy(str,words[2]);
	   sprintf(words[2],"%s/%s",root.c_str(),str);
	}
        trace = spAspTrace_t(new AspTrace(words,nw));
	if(trace != nullAspTrace && trace->getData()) {
	  count++;
	} else {
	  Winfoprintf("Error to load %s",buf);
	  dataok=0;
	}
      } else if(strcasecmp(words[0],"displayed_traces")==0 && nw > 1) {
	//e.g. displayed_traces: 1 2 3 4 5 6 7 8 9 10
	straces = nw-1;
      } 
   }
   if(straces==0) straces=ntraces;

   fclose(fp);
   
   return count;
}

int AspFrame::select(int x, int y) {
   int oldSel=frameSel;
   frameSel=AspUtil::select(x,y,pstx,psty,pwd,pht,2,true);
   if(oldSel != frameSel) {
     drawPlotBox();
   }
   return frameSel;
}

void AspFrame::drawPlotBox() {
   if(!aspMode) return;

   // erase previous highlight
   AspUtil::drawBox(pstx, psty, pwd, pht, SCALE_COLOR);

   if(frameSel>=HANDLE1 && frameSel <= HANDLE4)
       AspUtil::drawHandle(frameSel, pstx, psty, pwd, pht, ACTIVE_COLOR);
   else if(frameSel >= LINE1 && frameSel <= LINE4)
       AspUtil::drawBox(pstx, psty, pwd, pht, ACTIVE_COLOR);
}

void AspFrame::getFOVLimits(double &x, double &y, double &w, double &h) {
   if((axisFlag & AX_WEST)) {
	x=8*xcharpixels;
   	w=pixwd-10*xcharpixels;
   } else {
	x=2*xcharpixels;
   	w=pixwd-4*xcharpixels;
   }
   y = ycharpixels;
   h = pixht-6*ycharpixels;
} 

void AspFrame::moveFrame(int x,int y,int prevX,int prevY) {
   if(frameSel >= LINE1 && frameSel <= LINE4) {
     double newX = pstx + (x-prevX); 
     double newY = psty + (y-prevY); 
     double mx,my,mw,mh;
     getFOVLimits(mx,my,mw,mh);
     if(newX<mx || (newX+pwd)>(mx+mw)) return;
     if(newY<my || (newY+pht)>(my+mh)) return;
     pstx=newX;
     psty=newY; 
     draw();
   }
}

// handles start from upper left corner and clockwise
void AspFrame::resizeFrame(int x,int y,int prevX,int prevY) {
   int dx = x-prevX;
   int dy = y-prevY;
   if(frameSel==HANDLE1) {
	pstx += dx;	
	psty += dy;	
	pwd -= dx;
	pht -= dy;
   } else if(frameSel==HANDLE2) {
	psty += dy;	
	pwd += dx;
	pht -= dy;
   } else if(frameSel==HANDLE3) {
	pwd += dx;
	pht += dy;
   } else if(frameSel==HANDLE4) {
	pstx += dx;	
	pwd -= dx;
	pht += dy;
   }
   draw();
}

spAspTrace_t AspFrame::getDefaultTrace() {
   if(traceList->getSize() < 1) {
	spAspFrame_t frame = AspFrameMgr::get()->getFrame(this);
	AspDis1D::loadData(frame,true);
   }

   spAspTrace_t trace;
   if(aspMode) {
     AspTraceMap::iterator itr;
     trace = selTraceList->getFirstTrace(itr);
     if(trace == nullAspTrace) trace = traceList->getFirstTrace(itr);
   } else {
     int ind = currentindex()-1;
     if(ind>=traceList->getSize()) ind=traceList->getSize()-1;
     if(ind<0) ind=0;
     trace = selTraceList->getTraceByInd(ind);
     if(trace == nullAspTrace) trace = traceList->getTraceByInd(ind);
   }
   return trace;
}

void AspFrame::updateMenu() {
   if(ownScreen()) execString("menu('asp')\n");
   else {
	char str[20];
        Wgetgraphicsdisplay(str, 20);
	if(strstr(str,"dconi") == str)
        {
           if (getAxisOnly() )
              execString("menu('dconi_ao')\n");
           else
              execString("menu('dconi')\n");
        }
	else if(strstr(str,"dss") == str) execString("menu('display_1D')\n");
	else if(strstr(str,"ds") == str) execString("menu('ds_1')\n");
	else if(strstr(str,"dfs") == str) execString("menu('fiddisp_1D')\n");
	else if(strstr(str,"df") == str) execString("menu('dfid')\n");
   }
}
