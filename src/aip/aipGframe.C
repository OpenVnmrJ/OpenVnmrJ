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

#include "graphics.h"
#include "aipVnmrFuncs.h"
#include "aipStderr.h"
#include "sharedPtr.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipGframe.h"
#include "aipGraphicsWin.h"
#include "aipImgInfo.h"
#include "aipGframeManager.h"
#include "aipDataManager.h"
#include "aipInterface.h"
#include "group.h"
#include "aipWinMovie.h"
#include "aipMovie.h"
#include "aipVolData.h"
#include "aipOrthoSlices.h"
#include "aipSpecViewMgr.h"
#include "aipAxisInfo.h"
#include "aipImgOverlay.h"

int Gframe::nextId = 1;
int Gframe::pixstxOffset = 2;
int Gframe::pixstyOffset = 2;
int Gframe::normalColor = 4;
int Gframe::highlightColor = 2; // Color when mouse is nearby
int Gframe::selectColor = 4; // Color when we are selected
int Gframe::bkgColor = BG_IMAGE_COLOR;
int Gframe::FOVColor = BORDER_COLOR;
int Gframe::markLen = 8;
int Gframe::vsXRef = 0;
int Gframe::vsYRef = 0;
int Gframe::vsDynamicBinding = 0;
int Gframe::levelOfDraw = 0;
bool Gframe::inSaveState = false;
bool Gframe::inDrawState = false;

double Gframe::vsCenterRef = 0;
double Gframe::vsRangeRef = 0;
double Gframe::pixelsPerCm_fit = 0;

spGframe_t nullFrame = spGframe_t(NULL);
bool roiState = false;



extern "C" {
void graph_batch(int on);
void set_aipframe_draw(int on, XID_t map, int w, int h);
void set_aipframe_drawOverlay(int on, XID_t map, int w, int h);
int isJprintMode();
}

/* CONSTRUCTOR */
GframeLocation::GframeLocation(string k, int r, int c, int x, int y, int w,
        int h) {
    dataKey = k;
    row = r;
    col = c;
    pixstx = x;
    pixsty = y;
    pixwd = w;
    pixht = h;
}

/* CONSTRUCTOR */
Gframe::Gframe(int r, int c, int x, int y, int w, int h) :
    GframeLocation("", r, c, x, y, w, h) {
    aipDprint(DEBUGBIT_0,"Gframe()\n");
    id = nextId;
    if (++nextId == 0) {
        nextId = 1;
    } // New non-zero ID
    imgBackup.id = 0;
    imgBackup.width = 0;
    imgBackup.height = 0;

    // Set these to clear the whole interior of the frame
    //bkgstx = pixstx + pixstxOffset;
    //bkgsty = pixsty + pixht - pixstyOffset;

    viewList = new ViewInfoList;
    roiList = new RoiList;
    displayOod = true;
    imgBackupOod = true;
    highlighted = false;
    selected = false;
    color = normalColor;
    specList = new SpecViewList(id);
    zoomFactor=1.0;
    zoomSpecID=0;
    layerID=0;
}

/* DESTRUCTOR */
Gframe::~Gframe() {
    aipDprint(DEBUGBIT_0,"~Gframe()\n");
    clearFrame();
    delete viewList;
    delete roiList;
    delete specList;
}

void Gframe::clearFrame() {
    clearViewList();
    if (imgBackup.id != 0) {
        aipFreePixmap(imgBackup.id);
        imgBackup.id = 0;
        imgBackup.width = 0;
        imgBackup.height = 0;
    }
}

/*
 * Convert an x-y position in pixels into an x-y-z position
 * in lab (magnet) coordinates.
 */
/*
 void
 Gframe::pixToLab(double px, double py, double& lx, double& ly, double& lz)
 {
 lx = px * lscaleXX + py * lscaleYX + loffsetX;
 ly = px * lscaleXY + py * lscaleYY + loffsetY;
 lz = px * lscaleXZ + py * lscaleYZ + loffsetZ;
 }
 */

//
// Convert from position in the image (measured in pixel units from the
// first pixel in the data set) to position in the magnet reference frame
// (measured in cm from the center of the magnet with standard orientation).
//
/*
 D3Dpoint_t
 Gframe::pixel_to_magnet_frame(Fpoint pixel)
 {
 int i, j, k;
 double dcos[3][3];     // Direction cosines
 double span[2];        // Dimensions of data slab
 double loc[3];     // Distance from magnet center to slab center
 //    in slab coordinate frame orientation
 double dpixel[2];      // Same as "pixel", but in cm
 double dist[3];        // The answer in slab coordinate system.
 double magd[3];        // The answer as a vector.
 D3Dpoint ret;      // The answer.

 // Get some basic data
 for (i=0; i<3; i++){
 st->GetValue("location", loc[i], i);
 if (i < 2){
 st->GetValue("span", span[i], i);
 }
 }
 for (i=0, j=0; j<3; j++){
 for (k=0; k<3; k++, i++){
 st->GetValue("orientation", dcos[j][k], i);
 }
 }

 // Scale pixel address to cm
 dpixel[0] = pixel.x * GetRatioFast() / GetFast();
 dpixel[1] = pixel.y * GetRatioMedium() / GetMedium();

 // Add components of distance from magnet center to point
 dist[0] = loc[0] - 0.5 * span[0] + dpixel[0];
 dist[1] = loc[1] - 0.5 * span[1] + dpixel[1];
 dist[2] = loc[2];

 // Coordinate rotation to magnet frame
 for (i=0; i<3; i++){
 magd[i] = 0;
 for (j=0; j<3; j++){
 magd[i] += dcos[j][i] * dist[j];
 }
 }
 ret.x = magd[0];
 ret.y = magd[1];
 ret.z = magd[2];
 return ret;
 }
 */

/*
 * Load one image into the frame.
 * Deletes any existing views, and creates a new view.
 */
void Gframe::loadView(spImgInfo_t img, bool drawit) {
    /*fprintf(stderr,"loadView()\n");  CMP*/
    spViewInfo_t view = spViewInfo_t(new ViewInfo(img));
    //clearViewList();

// commented out because the view shouldn't be rotated
/*
    if(VolData::get()->showingObliquePlanesPanel())
    {
    	int irot, iframe;
		spGframe_t gf_current;
		GframeManager *gfm = GframeManager::get();
		gf_current = gfm->getFrameToLoad();
		iframe = gf_current->getGframeNumber();
		iframe -= 1;
		if (iframe > 2)
			iframe = 2;
		if (iframe < 0)
			iframe = 0;

    	irot = VolData::get()->extractRotations[iframe]; // the previous rotation for this frame

		if (irot < 0) // update rotations for display
		{
			irot = view->getRotation();
			VolData::get()->extractRotations[iframe] = irot;
		}
		else  // use previous rotation value
			view->setRotation(irot);
    }
*/

    viewList->push_back(view);

    if(viewList->size() > 1) fitOverlayView(view);
    else fitView(view);
   
//overlay: first image will fitView, additional image(s) will call a new function

    if (drawit) {
        draw();
    }
}

bool Gframe::coPlane(spViewInfo_t view, spViewInfo_t baseView) {
   if(view  == nullView || baseView == nullView) return false;
   spImgInfo_t img1 = view->imgInfo;
   spImgInfo_t img2 = baseView->imgInfo;
   if(img1 == nullImg) return false;
   if(img2 == nullImg) return false;

   return ImgOverlay::isCoPlane(img1->getDataInfo(),img2->getDataInfo());
}

bool Gframe::parallelPlane(spViewInfo_t view, spViewInfo_t baseView) {
   if(view  == nullView || baseView == nullView) return false;
   spImgInfo_t img1 = view->imgInfo;
   spImgInfo_t img2 = baseView->imgInfo;
   if(img1 == nullImg) return false;
   if(img2 == nullImg) return false;

   return ImgOverlay::isParallel(img1->getDataInfo(),img2->getDataInfo());
}

void Gframe::fitOverlayView(spViewInfo_t view) {

    if (view == nullView) return;

    spViewInfo_t baseView = getFirstView();
    if (baseView == nullView) {
	Winfoprintf("Error overlay: no base image.");
	return;
    }

    // get center of baseView and center x,y,z in user frame
    int x=(baseView->pixstx + 0.5*baseView->pixwd);
    int y=(baseView->pixsty + 0.5*baseView->pixht);
    double ux,uy,uz;
    baseView->pixToMagnet((double)x,(double)y,0.0,ux,uy,uz);

    spImgInfo_t oimg = view->imgInfo;
    if(oimg == nullImg) return;

    double fit = pixelsPerCm_fit; // save pixelsPerCm_fit before fit overlay image

    double ox1, ox2, oy1, oy2;
    oimg->getLabFrameImageExtents(ox1, oy1, ox2, oy2);
    view->pixelsPerCm = getFitInFrame(fabs(ox2 - ox1), fabs(oy2 - oy1), pixwd, pixht,
            view->getRotation(), view->pixwd, view->pixht);
    view->pixstx = pixstx + pixstxOffset;
    view->pixsty = pixsty + pixstyOffset;
    view->updateScaleFactors(); // Update data <--> pixel coord conversions

    checkOffset(baseView,view->cm_off);

    pixelsPerCm_fit = fit;

    if(!parallelPlane(view,baseView) ) {
        imgBackupOod = true;
	Winfoprintf("Warning overlay: image orientations are different.");
	return;
    }
/*
    if(coPlane(view,baseView) ) {
	Winfoprintf("Images are co-plane.");
    }
*/
 
    // use center ux,uy,uz to get center pixel px,py of overlay view
    // (overlay view shares the same ux,uy,uz with base view) 
    ux += view->cm_off[0];
    uy += view->cm_off[1];
    uz += view->cm_off[2];

    // use "user" coordinates instead of "Data" coordinates
    if(!calcZoom(view,ux,uy,uz,pixelsPerCm,baseView)) {
	return;
    }

    imgBackupOod = true;
}

void Gframe::fitView(spViewInfo_t view) {
    // Update scaling factors
    //bkgstx = pixstx + pixstxOffset;
    //bkgsty = pixsty + pixstyOffset;
    double x1, x2, y1, y2;
    spImgInfo_t img = view->imgInfo;

    img->getLabFrameImageExtents(x1, y1, x2, y2);
    pixelsPerCm = getFitInFrame(fabs(x2 - x1), fabs(y2 - y1), pixwd, pixht,
            view->getRotation(), view->pixwd, view->pixht);

    view->pixstx = pixstx + (pixwd - view->pixwd)/2 + pixstxOffset;
    view->pixsty = pixsty + (pixht - view->pixht)/2 + pixstyOffset;
    view->pixstx_off=0;
    view->pixsty_off=0;
    //if (bkgstx < view->pixstx + view->pixwd) {
    //bkgstx = view->pixstx + view->pixwd;
    //}
    //if (bkgsty < view->pixsty + view->pixht) {
    //bkgsty = view->pixsty + view->pixht;
    //}
    view->pixelsPerCm = pixelsPerCm;
    view->updateScaleFactors(); // Update data <--> pixel coord conversions
    updateScaleFactors(); // Update pixel <--> magnet conversions

    // Load ROIs, if present
    //spDataInfo_t di = img->getDataInfo();
    //roiList = di->roiList; // TODO
    updateRoiPixels();

    // TODO: Update owner frame pointers?

    imgBackupOod = true;
}

/**
 * Load the contents of a frame into this frame.
 * Redoes spatial scaling to fit the frame.
 */
void Gframe::loadFrame(spGframe_t gf, bool samePixmap) {
    memcpy(p2m, gf->p2m, sizeof(p2m));
    memcpy(m2p, gf->m2p, sizeof(m2p));
    if (samePixmap) {
        imgBackupOod = gf->imgBackupOod;
        imgBackup = gf->imgBackup;
    }
    //clearViewList();
    viewList = gf->copyViewList(samePixmap);
    spViewInfo_t view = getFirstView();
    if (view == nullView) {
        return;
    }

    // TODO: axis info? annotation?

    // Update scaling factors
    double x1, x2, y1, y2;
    spImgInfo_t img = gf->getFirstImage();
    if (img == nullImg) {
        return;
    }
    img->getLabFrameImageExtents(x1, y1, x2, y2);
    pixelsPerCm = getFitInFrame(fabs(x2 - x1), fabs(y2 - y1),
    pixwd, pixht,
    view->getRotation(),
    view->pixwd, view->pixht);
    view->pixstx = pixstx + pixstxOffset;
    view->pixsty = pixsty + pixstyOffset;
    view->pixelsPerCm = pixelsPerCm;
    view->updateScaleFactors(); // Update data <--> pixel coord conversions
    updateScaleFactors(); // Update pixel <--> magnet conversions

    // Copy ROI list
    //clearRoiList();
    //roilist = copyRoiList(gf->roilist);

    // ?? imgBackupOod = true;
    draw();
}

ViewInfoList *Gframe::copyViewList(bool sameBackingStore) {
    ViewInfoList *newList = new ViewInfoList;
    ViewInfoList::iterator pView;
    for (pView=viewList->begin(); pView!=viewList->end(); pView++) {

        newList->push_back(*pView);
        if (!sameBackingStore) {
        }
    }

    return newList;
}

RoiList *Gframe::copyRoiList() {
    return NULL;
}

void Gframe::updateViews() {
    if (displayOod) {
        draw();
    }
}

/**
 * Move all the views for a new frame position with same size.
 */
void Gframe::moveAllViews(int dx, int dy) {
    if (dx == 0&& dy == 0) {
        return;
    }
    ViewInfoList::iterator pView;
    for (pView = viewList->begin(); pView != viewList->end(); ++pView) {
        (*pView)->pixstx += dx;
        (*pView)->pixsty += dy;
        (*pView)->updateScaleFactors();
    }
    updateScaleFactors();
}

int Gframe::getViewCount() {
    if (viewList) {
        return viewList->size();
    } else {
        return 0;
    }
}

Roi *Gframe::getFirstRoi() {
    if (roiList && roiList->size() > 0) {
        roiItr = roiList->begin();
        if (roiItr != roiList->end()) {
            return *roiItr;
        }
    }
    return NULL;
}

Roi *Gframe::getNextRoi() {
    if (++roiItr == roiList->end()) {
        return NULL;
    }
    return *roiItr;
}

bool Gframe::containsRoi(Roi *roi) {
    RoiList::iterator itr;
    if (roiList && roiList->size() > 0) {
        for (itr = roiList->begin(); itr != roiList->end(); ++itr) {
            if ( *itr == roi ) {
                return true;
            }
        }
    }
    return false;
}

Roi *Gframe::getFirstUnselectedRoi() {
    RoiList::iterator itr;
    if (roiList && roiList->size() > 0) {
        for (itr = roiList->begin(); itr != roiList->end(); ++itr) {
            if ( !(*itr)->is_selected() ) {
                return *itr;
            }
        }
    }
    return NULL;
}

void Gframe::addRoi(Roi *roi) {
    roiList->push_back(roi);
    roi->draw();
}

bool Gframe::deleteRoi(Roi *roi) {
    RoiList::iterator itr;
    if (!roiList || roiList->size() < 1) {
        return false;
    }
    for (itr=roiList->begin(); itr != roiList->end(); ++itr) {
        if (*itr == roi) {
            roiList->erase(itr);
            return true;
        }
    }
    return false;
}

bool Gframe::hasASelectedRoi() {
    // TODO: Efficiency: Rewrite to check each ROI in this frame?
    Roi *roi;
    RoiManager *roim = RoiManager::get();
    for (roi = roim->getFirstSelected(); roi; roi = roim->getNextSelected()) {
        if (roi->pOwnerFrame == this) {
            return true;
        }
    }
    return false;
}

void Gframe::updateRoiPixels() {
    Roi *roi;
    for (roi=getFirstRoi(); roi; roi=getNextRoi()) {
        roi->setPixPntsFromData();
    }
}

bool Gframe::setDisplayOOD(bool flag) {
    bool rtn = displayOod;
    displayOod = flag;
    return rtn;
}

void Gframe::setViewDataOOD(bool flag) {
    spViewInfo_t vi = getFirstView();
    if (vi != nullView) {
        vi->setViewDataOOD(flag);
    }
}

/*
 * Save to canvas backing store everything except the specified
 * ROI (if any).
 */
bool Gframe::saveCanvasBackup(Roi *roi) {
    if (!roi->isActive()) {
        draw();
        return false;
    }

    if (!GraphicsWin::allocateCanvasBacking()) {
        return false;
    }
    XID_t bid = GraphicsWin::getCanvasBackingId();
    // Copy (superposition of) all images to canvas backing store
    spViewInfo_t view = getFirstView();
    bool ok;
    /*
     int  w = imgBackup.width + 19;
     int  h = imgBackup.height + 9;
     */
    int w = imgBackup.width;
    int h = imgBackup.height;

    XID_t cid = GraphicsWin::setDrawable(bid); // Draw on pixmap--not screen
    GraphicsWin::clearRect(view->pixstx, view->pixsty, w, h);

    if (w >= pixwd - 4)
        w = pixwd - 4;
    if (h >= pixht - 4)
        h = pixht - 4;
    ok = GraphicsWin::copyImage(imgBackup.id, bid, // Src, dst
            0, 0, // Src position
            w, h, // Src size
            view->pixstx, view->pixsty); // Dst position
    if (!ok) {
        //return false;     // TODO: Draw image if no pixmap?
    }

    // Draw all ROIs except "roi" on canvas backing store
    inSaveState = true;
    setClipRegion(FRAME_CLIP_TO_FRAME);
    inDrawState = true;

    Roi *r;
    for (r=getFirstRoi(); r; r=getNextRoi()) {
        if (r != roi) {
            r->draw();
        }
    }
    if (!viewList->empty()) {
        spViewInfo_t view = *viewList->begin();
        spImgInfo_t img = view->imgInfo;
        drawAnnotation(img->getDataInfo()->getKey().c_str(), pixstx, pixsty, pixwd, pixht, getGframeNumber(), id,
                0);
    }

    aipCallDisplayListeners(id, false);
    inSaveState = false;
    inDrawState = false;
    setClipRegion(FRAME_NO_CLIP);
    GraphicsWin::setDrawable(cid); // Draw on canvas (or whatever) again
    return true;
}

/*
 * Redraw the specified rectangle from the canvas backing store
 */
bool Gframe::drawCanvasBackup(int x1, int y1, int x2, int y2) {
    int xmin = x1 < x2 ? x1 : x2;
    int ymin = y1 < y2 ? y1 : y2;
    int xmax = x1 > x2 ? x1 : x2;
    int ymax = y1 > y2 ? y1 : y2;
    int sx1 = minX();
    int sy1 = minY();
    int sx2 = maxX();
    int sy2 = maxY();
    int dx1 = x1;
    int dy1 = y1;
    int dx2 = x2;
    int dy2 = y2;
    bool ok = true;

    // Clip drawing to interior of frame
    if (xmin < sx1) {
        xmin = sx1;
    } else if (xmin > sx2) {
        xmin = sx2;
    }
    if (ymin < sy1) {
        ymin = sy1;
    } else if (ymin > sy2) {
        ymin = sy2;
    }
    if (xmax < sx1) {
        xmax = sx1;
    } else if (xmax > sx2) {
        xmax = sx2;
    }
    if (ymax < sy1) {
        ymax = sy1;
    } else if (ymax > sy2) {
        ymax = sy2;
    }

    // Clear that rectangle (in case part is outside of image)
    GraphicsWin::clearRect(xmin, ymin, xmax - xmin, ymax - ymin+1);

    // Redraw image part from backing store
    XID_t bid = GraphicsWin::getCanvasBackingId();
    spViewInfo_t view = getFirstView();
    if (view != nullView) {
        // Clip operation to image area
        dx1 = (xmin < view->pixstx) ? view->pixstx : xmin;
        dy1 = (ymin < view->pixsty) ? view->pixsty : ymin;
        dx2 = (xmax >= view->pixstx + view->pixwd) ? view->pixstx + view->pixwd
                : xmax;
        dy2 = (ymax >= view->pixsty + view->pixht) ? view->pixsty + view->pixht
                : ymax;

        if (dx2 <= dx1 || dy2 <= dy1)
            return false;
        ok = GraphicsWin::copyImage(bid, 0, // 0 means canvas
                dx1, dy1, // Source rect position
                dx2 - dx1, dy2 - dy1+1, // Size of rect
                dx1, dy1); // Dest rect position
    }
    if (!ok) {
        return false;
    }
    // aipCallDisplayListeners(id, false);
    return true;
}

bool Gframe::imgBackupOOD() {
    if (imgBackupOod) {
        return true;
    } // Maybe dont need the rest?
    if (imgBackup.id == 0) {
        return true;
    }
    spViewInfo_t view = getFirstView();
    spImgInfo_t img = getFirstImage();
    if (view == nullView || img == nullImg) {
        return true;
    }
    if (imgBackup.width < view->pixwd) {
        return true;
    }
    if (imgBackup.height < view->pixht) {
        return true;
    }
    if (imgBackup.datastx != img->getDatastx()) {
        return true;
    }
    if (imgBackup.datasty != img->getDatasty()) {
        return true;
    }
    if (imgBackup.datawd != img->getDatawd()) {
        return true;
    }
    if (imgBackup.dataht != img->getDataht()) {
        return true;
    }
    if (imgBackup.vsfunc != img->getVsInfo()) {
        return true;
    }
    // TODO: Check interp mode and orientation of backup image
    return false;
}

void Gframe::setSelect(bool selFlag, bool appendFlag) {
    bool keepOthers = selFlag == false|| appendFlag == true;
    if (selected == selFlag && keepOthers) {
        return;
    }
    GframeManager *gfm = GframeManager::get();
    if (!keepOthers) {
        gfm->clearSelectedList();
    }
    gfm->setSelect(this, selFlag);
    if (keepOthers) {
        drawFrame(); // Just draw me
    } else {
        gfm->drawFrames(); // Draw everybody
    }
}

void Gframe::setSelected(bool b) {
    if (selected == b) {
        return;
    }
    selected = b;
    if (highlighted) {
        color = highlightColor;
    } else if (selected) {
        color = selectColor;
    } else {
        color = normalColor;
    }
    drawFrame();
}

void Gframe::setHighlight(bool b) {
    if (highlighted == b) {
        return;
    }
    highlighted = b;
    if (highlighted) {
        color = highlightColor;
    } else if (selected) {
        color = selectColor;
    } else {
        color = normalColor;
    }
    drawFrame();
}

spGframe_t Gframe::highlight(int x, int y, spGframe_t prevGframe) {
    const int frac = 5;
    int aperture = (pixwd < pixht) ? pixwd / frac : pixht / frac;
    aperture = (aperture > Roi::aperture) ? aperture : Roi::aperture;
    bool isNear = distanceFrom(x, y) < aperture;
    if (prevGframe.get() && this != prevGframe.get()) {
        prevGframe->setHighlight(false);
    }
    setHighlight(isNear);
    GframeManager *gfm = GframeManager::get();
    return isNear ? gfm->getPtr(this) : nullFrame;
}

/*
 * Get the distance from (x,y) to the frame boundary
 * Assume (at lease for now) that (x,y) is inside the frame.
 */
double Gframe::distanceFrom(int x, int y) {
    double d = x - pixstx;
    double d1 = pixstx + pixwd - 1- x;
    if (d1 < d)
        d = d1;
    d1 = y - pixsty;
    if (d1 < d)
        d = d1;
    d1 = pixsty + pixht - 1- y;
    if (d1 < d)
        d = d1;
    return d;
}

void Gframe::drawFOV() {
    if(viewList->empty()) return;

    int show = (int) getReal("aipShowFOV", 0);
    if(show == 0) return;

    spViewInfo_t view;
    ViewInfoList::iterator pView;
    for (pView = viewList->begin(); pView != viewList->end(); ++pView) {
        if(*pView == nullView) continue;
 	view = *pView;
        //if(pixelsPerCm_fit < 1 || pixelsPerCm_fit < pixelsPerCm) return;
    
      if(pView == viewList->begin()) { // draw box for base image
        GraphicsWin::drawRect(view->pixstx, view->pixsty, view->pixwd-3, view->pixht-3, FOVColor);
      } else { // corner for overlay image
        int x = view->pixstx;
        int y = view->pixsty;
        GraphicsWin::drawLine(x, y, x, y + markLen, FOVColor);
        GraphicsWin::drawLine(x, y, x + markLen, y, FOVColor);
        x = view->pixstx + view->pixwd - 3;
        GraphicsWin::drawLine(x, y, x, y + markLen, FOVColor);
        GraphicsWin::drawLine(x, y, x - markLen, y, FOVColor);
        y = view->pixsty + view->pixht - 3;
        GraphicsWin::drawLine(x, y, x, y - markLen, FOVColor);
        GraphicsWin::drawLine(x, y, x - markLen, y, FOVColor);
        x = view->pixstx;
        GraphicsWin::drawLine(x, y, x, y - markLen, FOVColor);
        GraphicsWin::drawLine(x, y, x + markLen, y, FOVColor);
      }
    }
}

void Gframe::drawFrame() {
    drawFrameLabel();
    if (isJprintMode())
        return;
    if (selected) {
        int x = pixstx + 1;
        int y = pixsty + 1;
        if (highlighted)
        {
           GraphicsWin::drawRect(pixstx, pixsty, pixwd-1, pixht-1, -color);
        }
        else
        {
           GraphicsWin::drawRect(pixstx, pixsty, pixwd-1, pixht-1, -bkgColor);
           GraphicsWin::drawRect(pixstx, pixsty, pixwd-1, pixht-1, color);
        }
        GraphicsWin::drawRect(pixstx+1, pixsty+1, pixwd-3, pixht-3, -bkgColor);
        GraphicsWin::drawLine(x, y, x, y + markLen, -color);
        GraphicsWin::drawLine(x, y, x + markLen, y, -color);
        x = pixstx + pixwd - 2;
        GraphicsWin::drawLine(x, y, x, y + markLen, -color);
        GraphicsWin::drawLine(x, y, x - markLen, y, -color);
        y = pixsty + pixht - 2;
        GraphicsWin::drawLine(x, y, x, y - markLen, -color);
        GraphicsWin::drawLine(x, y, x - markLen, y, -color);
        x = pixstx + 1;
        GraphicsWin::drawLine(x, y, x, y - markLen, -color);
        GraphicsWin::drawLine(x, y, x + markLen, y, -color);
    }
    else
    {
       if (highlighted)
       {
        GraphicsWin::drawRect(pixstx, pixsty, pixwd-1, pixht-1, -color);
       }
       else
       {
        GraphicsWin::drawRect(pixstx, pixsty, pixwd-1, pixht-1, -bkgColor);
        GraphicsWin::drawRect(pixstx, pixsty, pixwd-1, pixht-1, color);
       }
       GraphicsWin::drawRect(pixstx+1, pixsty+1, pixwd-3, pixht-3, -bkgColor);
    }

    GraphicsWin::loadFont(14); // TODO: Variable font size.  Don't always load?
    // drawFrameLabel();
}

void Gframe::drawFrameLabel() {
    int show = (int)getReal("aipNumberFrames", 1);
    char buf[1000];
    int ascent;
    int descent;
    int cwd;

    if (transparency_level > 80)
       return;
    // Get width and height of a character (cwd, cht)
    GraphicsWin::getTextExtents("0", 14, &ascent, &descent, &cwd);
    // cht = ascent + descent;
    int numLeft = maxX();
    int nMax = DataManager::get()->getNumberOfImages();
    int n;
    int maxNumWd = 0;
    for (maxNumWd = cwd, n = 10; n <= nMax; maxNumWd += cwd, n *= 10)
        ;

    setClipRegion(FRAME_CLIP_TO_FRAME);

    if (show && (n = getGframeNumber()) > 0&& maxNumWd <= maxX() - minX()) {

        numLeft = maxX() - maxNumWd;
        sprintf(buf, "%d", n);
        int wd;
        GraphicsWin::getTextExtents(buf, 14, &ascent, &descent, &wd);
        int x = maxX() - wd;
        int y = maxY() - descent;
        if (y - ascent >= minY()) {
            // Frame big enough for numbering
            if (show < 2) {
                // Don't clear background
                GraphicsWin::drawString(buf, x, y, -bkgColor, 0, 0, 0);
                GraphicsWin::drawString(buf, x, y, color, 0, 0, 0);
            } else {
                GraphicsWin::drawString(buf, x, y, -bkgColor, wd, ascent, descent);
                GraphicsWin::drawString(buf, x, y, color, wd, ascent, descent);
            }
        }
    }

    char name[MAXSTR];
    sprintf(name,"%s",getGframeName().c_str());
    if (strlen(name) != 0 && minX() + cwd < numLeft) {
        int wd;
        GraphicsWin::getTextExtents(name, 14, &ascent, &descent, &wd);
        int x = minX();
        int y = maxY() - descent;
        if (y - ascent >= minY()) {
            // Frame big enough for the name label
            if (x + wd > numLeft - cwd) {
                x = numLeft - cwd - wd;
            }
            GraphicsWin::drawString(name, x, y, -bkgColor, 0, 0, 0);
            GraphicsWin::drawString(name, x, y, color, 0, 0, 0);
        }
    }
    setClipRegion(FRAME_NO_CLIP);
}

void Gframe::print(double scale) {
    // Print first image only
    ViewInfoList::iterator pView = viewList->begin();
    if (pView != viewList->end()) {
        (*pView)->print(scale);
    }
}

void Gframe::saveRoi() {
    if (viewList->empty())
        return;
    if (roiList->size() <= 0)
        return;

    spViewInfo_t view = *viewList->begin();
    spImgInfo_t img = view->imgInfo;
    Roi *roi;
    for (roi = getFirstRoi(); roi; roi = getNextRoi()) {
        img->getDataInfo()->addRoi("", roi->getMagnetCoords(), false, false);
    }
}

void Gframe::draw() {
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf=gfm->getPtr(this);
    if (!gfm->isFrameDisplayed(gf))
        return;
    if (!aipHasScreen())
        grabMouse();

    // Draw images
    spImgInfo_t img;
    spViewInfo_t view;
    ViewInfoList::iterator pView;
    // TODO: align images wrt each other
    // TODO: store pixmap of superposition of images
    XID_t pixmap = 0;
    bool changeFlag = false;
    bool isImage = false;
    bool movieStopped = WinMovie::get()->movieStopped();
    if(movieStopped) movieStopped = Movie::get()->movieStopped();

    if (inSaveState)
        return;
    // graph_batch(1);
    if (viewList->empty() && movieStopped && !VolData::get()->showingObliquePlanesPanel()) {
        // OB: No images; clear the frame
        GraphicsWin::clearRect(minX(), minY(), maxX() - minX() + 1, maxY()
                - minY() + 1);
    	// NB: don't clear (causes background flicker when redrawing new data into old frame)
    } else {
        int w = pixwd;
        int h = pixwd;
        int x2 = pixstx + pixwd;
        int y2 = pixsty + pixht;

        for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
            isImage = true;
            view = *pView;
            img = view->imgInfo;
            if (pView == viewList->begin()) { // Different processing for 1st image
                set_aipframe_draw(0, 0, 0, 0);
                GraphicsWin::clearRect(minX(), minY(), maxX()
                            - minX() + 1, maxY() - minY() + 1);
                // Draw Base Image
                if (!imgBackupOOD()) {

                    /*fprintf(stderr,"Base image from pixmap (%d, %d)\n",
                     view->pixstx, view->pixsty);  CMP*/
                    w = imgBackup.width;
                    h = imgBackup.height;
                    if (view->pixstx + w > x2)
                        w = x2 - view->pixstx;
                    if (view->pixsty + h > y2)
                        h = y2 - view->pixsty;
                    GraphicsWin::copyImage(imgBackup.id, 0, 0, 0, w, h,
                            view->pixstx, view->pixsty);
                } else {
                    changeFlag = true;
                    set_aipframe_draw(1, imgBackup.id, imgBackup.width,
                            imgBackup.height);
                    GraphicsWin::clearRect(0, 0, imgBackup.width, imgBackup.height);
                    /*fprintf(stderr,"Redraw base image from view (%d, %d)\n",
                     view->pixstx, view->pixsty);  CMP*/
                    /*
                     // the view->draw will call aip_displayImage which will
                     //  free and create pixmap.
                     if (imgBackup.id) {
                     aipFreePixmap(imgBackup.id);
                     }
                     */
                    pixmap = view->draw(); // Draw image on canvas; return pixmap
                    //  if (imgBackup.id != pixmap) {
                    imgBackup.width = view->pixwd;
                    imgBackup.height = view->pixht;
                    //  }
                    imgBackup.id = pixmap;
                    if (imgBackup.id) {
                        imgBackupOod = false;
                        //  imgBackup.width = view->pixwd;
                        //  imgBackup.height = view->pixht;
                        imgBackup.datastx = img->getDatastx();
                        imgBackup.datasty = img->getDatasty();
                        imgBackup.datawd = img->getDatawd();
                        imgBackup.dataht = img->getDataht();
                        imgBackup.vsfunc = img->getVsInfo();
                        // TODO: Save interp mode and orientation of backup image
                        //setImgBackupOOD(false);

                    } else {
                        imgBackup.width = 0;
                        imgBackup.height = 0;
                    }
                    // set_aipframe_draw(0, 0, 0, 0);
                }
                // Clear any empty space around first image
               /****************
                if (minX() < view->pixstx) {
                    GraphicsWin::clearRect(minX(), minY(), view->pixstx
                            - minX() + 1, maxY() - minY() + 1);
                }
                if (minY() < view->pixsty) {
                    GraphicsWin::clearRect(minX(), minY(), maxX() - minX() + 1,
                            view->pixsty - minY() + 1);
                }
                if (maxX() >= view->pixstx + w) {
                    GraphicsWin::clearRect(view->pixstx + w, minY(), maxX()
                            - view->pixstx- w + 1, maxY() - minY() + 1);
                }
                if (maxY() >= view->pixsty + h) {
                    GraphicsWin::clearRect(minX(), view->pixsty + h, maxX()
                            - minX() + 1, maxY() - view->pixsty- h + 1);
                }
               ****************/
            } else {
                // Draw overlaid images
                if (pixmap < 1 && imgBackup.id > 1)
                    set_aipframe_draw(1, imgBackup.id, imgBackup.width, imgBackup.height);
                changeFlag = true;
                view->draw();
            }
        }
        // Do this to get image in Vnmr's backing store
        // TODO: emlimimate need for 2nd image copy
        if (!viewList->empty()) {
            view = *viewList->begin();
            w = imgBackup.width;
            h = imgBackup.height;
            if (view->pixstx + w > x2)
               w = x2 - view->pixstx;
            if (view->pixsty + h > y2)
               h = y2 - view->pixsty;
            GraphicsWin::copyImage(imgBackup.id, 0, 0, 0, w, h,
                                view->pixstx, view->pixsty);
        }
        set_aipframe_draw(0, 0, 0, 0);
    }

    // Draw ROIs
    setClipRegion(FRAME_CLIP_TO_FRAME);
    inDrawState = true;
    levelOfDraw++;

    Roi *roi;
    GraphicsWin::loadFont(14); // TODO: Variable font size.  Don't always load?
    if (!movieStopped && getReal("aipMovieSettings", 2, 1) == 0) {
        // no do not draw ROIs and annotation.
    } else {
        for (roi = getFirstRoi(); roi; roi = getNextRoi()) {
            roi->draw();
        }

	AxisInfo::displayAxis(gf);

        // Draw annotation (TODO)
        // of first view
        if (!viewList->empty()) {
            view = *viewList->begin();
            img = view->imgInfo;
            drawAnnotation(img->getDataInfo()->getKey().c_str(), pixstx, pixsty, pixwd, pixht, getGframeNumber(),
                    id, changeFlag);
            //view->pixstx, view->pixsty, view->pixwd, view->pixht);
        }
        if (isImage && !movieStopped) {
            aipCallDisplayListeners(id, true);
        } else if (isImage) {
            aipCallDisplayListeners(id, changeFlag);
        }
        if (!viewList->empty()) {
            view = *viewList->begin();
            disSpec(view->pixstx, view->pixsty, view->pixwd, view->pixht, changeFlag);
        } else disSpec(pixstx, pixsty, pixwd, pixht, changeFlag);
    }

    if (isImage) {
        setString("aipCurrentKey", img->getDataInfo()->getKey().c_str(), false);
    }

    displayOod = false;

    // Draw frame
    // NB: Used to draw the frame first, but then sometimes the first
    // frame in the window does not appear after a window resize.
    // There seems to be a race condition where VJ erases the screen
    // *after* telling BG to update it.
    // drawFrame();

    levelOfDraw--;
    if (levelOfDraw <= 0) {
        levelOfDraw = 0;
        inDrawState = false;
        setClipRegion(FRAME_NO_CLIP);
        drawFrame();
        //graph_batch(-9);
    }
    drawFOV();
}

bool Gframe::updateScaleFactors() {
    int i, j;

    ViewInfoList::iterator pView = viewList->begin();
    if (pView == viewList->end()) {
        return false;
    }
    
    for(i=0;i<4;i++)
      for(j=0;j<4;j++) {
	p2m[i][j]=(*pView)->p2m[i][j];
	m2p[i][j]=(*pView)->m2p[i][j];
    }

    return true;
}

void Gframe::pixToMagnet(double px, double py, double pz, double &mx,
        double &my, double &mz) {
    int i, j;
    double p[3];
    double m[3];
    p[0] = px;
    p[1] = py;
    p[2] = pz;

    for (i=0; i<3; i++) {
        m[i] = p2m[i][3];
        for (j=0; j<3; j++) {
            m[i] += p2m[i][j] * p[j];
        }
    }
    mx = m[0];
    my = m[1];
    mz = m[2];

    // TESTING
    /*
     double p2[3];
     for (i=0; i<3; i++) {
     p2[i] = m2p[i][3];
     for (j=0; j<3; j++) {
     p2[i] += m2p[i][j] * m[j];
     }
     }
     double err = sqrt((p[0]-p2[0])*(p[0]-p2[0]) +
     (p[1]-p2[1])*(p[1]-p2[1]) +
     (p[2]-p2[2])*(p[2]-p2[2]));
     double err2D = sqrt((p[0]-p2[0])*(p[0]-p2[0]) +
     (p[1]-p2[1])*(p[1]-p2[1]));
     fprintf(stderr,"err=%g, 2D err=%g\n", err, err2D);
     */
}

void Gframe::setClipRegion(ClipStyle style) {
    if (inDrawState)
        return;

    if (style == FRAME_NO_CLIP) {
        // Turn off clipping
        GraphicsWin::setClipRectangle(0, 0, 0, 0);
    } else if (style == FRAME_CLIP_TO_IMAGE) {
        // Clip to area of image
        ViewInfoList::iterator pView;
        spViewInfo_t viewInfo;
        pView = viewList->begin();
        if (pView == viewList->end()) {
            return;
        }
        viewInfo = *pView;
        GraphicsWin::setClipRectangle(viewInfo->pixstx, viewInfo->pixsty,
                viewInfo->pixwd, viewInfo->pixht);
    } else if (style == FRAME_CLIP_TO_FRAME) {
        // Clip to interior of frame
        GraphicsWin::setClipRectangle(minX(), minY(), maxX() - minX() + 1,
                maxY() - minY() + 1);
    }
}

spViewInfo_t Gframe::getFirstView() {
    viewItr = viewList->begin();
    if (viewItr != viewList->end()) {
        return *viewItr;
    }
    return nullView;
}

spViewInfo_t Gframe::getNextView() {
    // TODO: Bug in this?? Gets viewItr==NULL - exception in return line.
    if (++viewItr == viewList->end()) {
        return nullView;
    }
    return *viewItr;
}

spImgInfo_t Gframe::getFirstImage() {
    ViewInfoList::iterator pView;
    pView = viewList->begin();
    if (pView != viewList->end()) {
        return (*pView)->imgInfo;
    }
    return nullImg;
}

/*
 * Returns order of this Gframe's data in data list (starting from 1).
 * If no data or not in list, returns 0.
 */
int Gframe::getDataNumber() {
    spImgInfo_t img = getFirstImage();
    if (img == nullImg) {
        return 0;
    }
    int num = img->getDataInfo()->getDataNumber();
    if (num <= 0) {
         DataManager::get()->numberData();
         num = img->getDataInfo()->getDataNumber();
    }
    return num;
}

/*
 * Returns order of this Gframe in list (starting from 1).
 * If not in list, returns -1.
 */
int Gframe::getGframeNumber() {
    int i;
    spGframe_t frame;
    GframeManager *gfm = GframeManager::get();
    GframeList::iterator gfi;
    for (i=1, frame = gfm->getFirstFrame(gfi); frame != nullFrame; i++, frame
            = gfm->getNextFrame(gfi) ) {
        if (frame.get() == this && getFirstView() != nullView) {
            return i;
        }
    }
    return -1;
}

/*
 * Returns a short name for the file this image came from.
 * If no data, returns an empty string.
 */
string Gframe::getGframeName() {
    string name = "";
    spImgInfo_t img = getFirstImage();
    if (img == nullImg) {
        return name;
    }

    int show = (int)getReal("aipNameFrames", 0);
    if(show == 1) { 
      name = img->getDataInfo()->getShortName();
      if (name.length() == 0) {
         DataManager::get()->makeShortNames();
         name = img->getDataInfo()->getShortName();
      }
    } else if(show == 2) {
      name = img->getDataInfo()->getImageNumber();
    } else if(show == 3) {
      char str[MAXSTR];
      sprintf(str,"%d",getDataNumber());
      name = string(str);
    }
    return name;
}

void Gframe::autoVs() {
    //spImgInfo_t img = getFirstImage();
    spImgInfo_t img = getSelImage();
    img->autoVscale();
    imgBackupOod = true;
    draw();
    //vscaleOtherFrames(img->getVsMin(), img->getVsMax(), img->getVsInfo());
    vscaleOtherImages();
}

void Gframe::quickVs(int x, int y, bool maxmin) {
    if (viewList->size() == 0) {
        return;
    }
    //spViewInfo_t view = getFirstView(); // Rescale only first image in stack
    spViewInfo_t view = getSelView(); // Rescale only first image in stack
    if (view == nullView || !view->pointIsOnImage(x, y)) {
        return;
    }
    view->setVs(x, y, maxmin);
    imgBackupOod = true;
    draw();

    //spImgInfo_t img = getFirstImage();
    spImgInfo_t img = getSelImage();
    // double min = img->getVsMin();
    // double max = img->getVsMax();
    spVsInfo_t vsi = img->getVsInfo();
    //fprintf(stderr,"Call vscaleOtherImages\n");/*CMP*/
    vscaleOtherImages();

    VsInfo::setVsHistogram(this);
    /*
     if (maxmin) {
     view->getMaxInRect();
     }
     */
}

void Gframe::vscaleOtherImages(int m) {

    int mode = m;
    if (mode == -1)
        mode = VsInfo::getVsMode();
    //fprintf(stderr,"mode=%d\n", mode);/*CMP*/
    DataManager *dm = DataManager::get();
    spImgInfo_t img = getSelImage();
    spDataInfo_t di = img->getDataInfo();
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    spViewInfo_t view;
    GframeCache_t::iterator gfi;
    GframeList::iterator gitr;

    set<string> keyList;
    switch (mode) {
    case VS_NONE:
    case VS_INDIVIDUAL:
        return; // Nothing to do for these
        break;
    case VS_HEADER:
        /*
         keyList = dm->getKeys(DATA_SELECTED_FRAMES);
         if(keyList.size() <= 0)
         for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame;
         gf=gfm->getNextCachedFrame(gfi)) {
         if ((view = gf->getSelView()) == nullView) continue;

         keyList.insert(view->imgInfo->getDataInfo()->getKey());
         }
         break;
         */
    case VS_UNIFORM:
        //keyList = dm->getKeys(DATA_ALL);
        // get images of cashed frames instead.
        for (gf=gfm->getFirstCachedFrame(gfi); gf != nullFrame; gf
                =gfm->getNextCachedFrame(gfi)) {
            if ((view = gf->getSelView()) == nullView)
                continue;

            keyList.insert(view->imgInfo->getDataInfo()->getKey());
        }
        break;
    case VS_GROUP:
        keyList = dm->getKeys(DATA_GROUP, di->getGroup());
        break;
    case VS_OPERATE:
        keyList = dm->getKeys(DATA_OPERATE, di->getKey());
        break;
    case VS_DISPLAYED:
        m = VsInfo::getVsMode(); // actual mode
        for (gf=gfm->getFirstFrame(gitr); gf != nullFrame; gf
                =gfm->getNextFrame(gitr)) {
            if ((view = gf->getSelView()) == nullView)
                continue;

            if (m == VS_GROUP) {
                if (di->getGroup() == view->imgInfo->getDataInfo()->getGroup())
                    keyList.insert(view->imgInfo->getDataInfo()->getKey());
            } else {
                keyList.insert(view->imgInfo->getDataInfo()->getKey());
            }
        }
        break;
    case VS_SELECTEDFRAMES:
        keyList = dm->getKeys(DATA_SELECTED_FRAMES);
        break;
    }

    double min = img->getVsMin();
    double max = img->getVsMax();
    spVsInfo_t vsi = img->getVsInfo();
    string mykey = di->getKey();

    //fprintf(stderr,"key list length=%d\n", keyList.size());/*CMP*/
    set<string>::iterator keyItr;
    for (keyItr = keyList.begin(); keyItr != keyList.end(); ++keyItr) {
        if (*keyItr != mykey) {
            //fprintf(stderr,"Key=%s\n", keyItr->c_str());/*CMP*/
            gf = gfm->getCachedFrame(*keyItr);
            if (gf != nullFrame) {
                img = gf->getSelImage();
                if (img != nullImg) {
                    img->setVsMax(max);
                    img->setVsMin(min);
                    img->setVsInfo(vsi);
                }
                gf->imgBackupOod = true;
                if (gfm->isFrameDisplayed(gf)) {
                    gf->draw();
                }
            }
        }
    }
}

void Gframe::vscaleOtherFrames(double min, double max, spVsInfo_t vsi) {
    int mode;
    //if ((mode= (int)getReal("aipVsBind", 0)) <= 0 ||
    if ((mode=VsInfo::getVsMode()) <= 0||(mode == 1&& !selected)) {
        return; // Nothing to do
    }
    // Scale "other" frames
    GframeManager *gfm = GframeManager::get();
    spGframe_t gf;
    GframeList::iterator gfi;
    gf
            = (mode == 1 ? gfm->getFirstSelectedFrame(gfi)
                    : gfm->getFirstFrame(gfi));
    while (gf != nullFrame) {
        spImgInfo_t img;
        if (gf.get() != this&& (img=gf->getSelImage()) != nullImg) {
            img->setVsMax(max);
            img->setVsMin(min);
            img->setVsInfo(vsi);
            gf->imgBackupOod = true;
            gf->draw();
        }
        gf = (mode == 1 ? gfm->getNextSelectedFrame(gfi)
                : gfm->getNextFrame(gfi));
    }
}

void Gframe::startInteractiveVs(int x, int y) {
    vsXRef = x;
    vsYRef = y;
    spImgInfo_t img = getSelImage();
    vsCenterRef = (img->getVsMin() + img->getVsMax()) / 2;
    vsRangeRef = img->getVsMax() - img->getVsMin();
    vsDynamicBinding = (int)getReal("aipVsDynamicBinding", 0);
}

void Gframe::doInteractiveVs(int x, int y) {
    double dx = x - vsXRef;
    double dy = y - vsYRef;
    double range = vsRangeRef * exp(-dx / 250);
    double center = vsCenterRef + (dy / 500) * vsRangeRef;
    spImgInfo_t img = getSelImage();
    double vsMin = center - range / 2;
    double vsMax = center + range / 2;
    img->setVsMin(vsMin);
    img->setVsMax(vsMax);
    imgBackupOod = true;
    draw();
    if (vsDynamicBinding) {
        //vscaleOtherFrames(vsMin, vsMax, img->getVsInfo());
        int mode = VsInfo::getVsMode();
        //if(mode == VS_UNIFORM || mode == VS_GROUP || mode == VS_OPERATE)
        if (mode == VS_UNIFORM|| mode == VS_GROUP)
            vscaleOtherImages(VS_DISPLAYED);
        else
            vscaleOtherImages(mode);
    }
}

void Gframe::finishInteractiveVs() {
    int mode = VsInfo::getVsMode();
    //bool b = (mode == VS_UNIFORM || mode == VS_GROUP || mode == VS_OPERATE);
    bool b = (mode == VS_UNIFORM|| mode == VS_GROUP);
    if (!b && vsDynamicBinding) {
        return; // Already did it
    }
    spImgInfo_t img = getSelImage();
    // double vsMin = img->getVsMin();
    // double vsMax = img->getVsMax();
    vscaleOtherImages(mode);
    //vscaleOtherFrames(vsMin, vsMax, img->getVsInfo());
    VsInfo::setVsHistogram(this);
}

void Gframe::setVs(spVsInfo_t vsi) {
    spImgInfo_t img = getSelImage();
    if (img == nullImg) {
        return;
    }
    img->setVsInfo(vsi);
    imgBackupOod = true;
    draw();
}

spVsInfo_t Gframe::getVs() {
    spImgInfo_t img = getSelImage();
    if (img == nullImg) {
        return nullVs;
    }
    return img->getVsInfo();
}

int Gframe::resetOverlayPan() {
    overlayOff[0]=0.0;
    overlayOff[1]=0.0;
    overlayOff[2]=0.0;
    spViewInfo_t view = getFirstView();
    if (view != nullView) {
        int overlayPan=false;
        if(viewList->size()>1) {
          ViewInfoList::iterator pView = viewList->begin();
          pView++;
          while(pView != viewList->end()) {
             if((*pView)->cm_off[0] != 0.0 || (*pView)->cm_off[1] != 0.0
                || (*pView)->cm_off[2] != 0.0) {
                overlayPan=true;
               (*pView)->cm_off[0]=0;
               (*pView)->cm_off[1]=0;
               (*pView)->cm_off[2]=0;
             }
             fitOverlayView(*pView);
             pView++;
          }
          if(overlayPan) {
            draw();
            return 1;
	  }
        }
    }
    return 0;
}

void Gframe::resetZoom(bool master) {

    if(resetOverlayPan()) return;

    spViewInfo_t view = getFirstView();
    if (view != nullView) {
        spImgInfo_t img = view->imgInfo;
        spDataInfo_t data = img->getDataInfo();
        img->datastx = 0;
        img->datasty = 0;
        img->datawd = data->getFast();
        img->dataht = data->getMedium();
        fitView(view);
	if(viewList->size()>1) {
          ViewInfoList::iterator pView = viewList->begin();
          pView++;
          while(pView != viewList->end()) {
             (*pView)->cm_off[0]=0;
             (*pView)->cm_off[1]=0;
             (*pView)->cm_off[2]=0;
	     fitOverlayView(*pView);
             pView++;
          } 
	}
        draw();
    } else if(hasSpec()) {
        resetSpecFrame();
        return;
    }

    int mode;
    if (master && (mode= (int)getReal("aipZoomBind", 0)) > 0) {
        // mode: 1==> bind selected; 2==> bind all
        if (mode == 1&& !selected) {
            return;
        }

        // Reset other frames also
        GframeManager *gfm = GframeManager::get();
        spGframe_t gf;
        GframeList::iterator gfi;
        gf = (mode == 1) ? gfm->getFirstSelectedFrame(gfi)
                : gfm->getFirstFrame(gfi);
        while (gf != nullFrame) {
            if (gf.get() != this) {
		gf->resetZoom(false);
            }
            gf = (mode == 1) ? gfm->getNextSelectedFrame(gfi)
                    : gfm->getNextFrame(gfi);
        }
    }
}

void Gframe::quickZoom(int x, int y, double factor) {
    spViewInfo_t view = getFirstView();
    if (view == nullView) {
        if(hasSpec()) zoomSpecFrame(x,y,factor);
        return;
    }
    spImgInfo_t img = view->imgInfo;
    spDataInfo_t di = img->getDataInfo();
    double newPixPerCm = pixelsPerCm * factor;
    double xdata, ydata;
    view->pixToData(x, y, xdata, ydata);

    // convert data center to magnet frame for aipZoomBind to use
    // do this before setZoom because it will change pixToMagnet.
    double mx,my,mz;
    view->pixToMagnet((double)x,(double)y,0.0,mx,my,mz);
        
    setZoom(xdata, ydata, newPixPerCm);

    int mode;
    if ((mode= (int)getReal("aipZoomBind", 0)) > 0) {
        // mode: 1==> bind selected; 2==> bind all
        if (mode == 1&& !selected) {
            return;
        }

        spImgInfo_t img = view->imgInfo;
        spDataInfo_t di = img->getDataInfo();
        int sizex = di->getFast();
        int sizey = di->getMedium();
        double scalex=1;
        double scaley=1;
        spViewInfo_t view2;
	int magnet = (int)getReal("aipZoomBindMode", 0);

        // Scale other frames to same pixPerCm, centered on same data point
        GframeManager *gfm = GframeManager::get();
        spGframe_t gf;
        GframeList::iterator gfi;
        gf = mode==1 ? gfm->getFirstSelectedFrame(gfi)
                : gfm->getFirstFrame(gfi);
        while (gf != nullFrame) {
            if (gf.get() != this) {
                if ((view2=gf->getFirstView()) != nullView) {
        	    img = view2->imgInfo;
        	    di = img->getDataInfo();
		    if(magnet) {
		        double px1,px2,px3;
			view2->magnetToPix(mx,my,mz,px1,px2,px3);
	                view2->pixToData((int)px1,(int)px2,xdata,ydata);
                        gf->setZoom(xdata*scalex, ydata*scaley, newPixPerCm);
		    } else {
        	    	if(sizex>0) scalex = ((double)di->getFast())/sizex;
        	    	if(sizey>0) scaley = ((double)di->getMedium())/sizey;
                    	gf->setZoom(xdata*scalex, ydata*scaley, newPixPerCm);
		    }
                }
            }
            gf = mode==1 ? gfm->getNextSelectedFrame(gfi)
                    : gfm->getNextFrame(gfi);
        }
    }
// this is commented out because it may crashe Vnmrbg, and 
// there is no need to redraw cursors. But need to track down the problem 
// in updateCursors().
// 
//    if(getReal("showObliquePlanesPanel", 0)> 0)
//                        OrthoSlices::get()->updateCursors();

    /*
     Movie_frame *mhead;
     if (mhead = in_a_movie(gframe->imginfo)){
     // Set displayed region for all images in movie.
     Movie_frame *mframe = mhead->nextframe;
     do{
     mframe->img->datastx = stx;
     mframe->img->datasty = sty;
     mframe->img->img->datawd = dwd;
     mframe->img->img->dataht = dht;
     mframe = mframe->nextframe;
     } while (mframe != mhead->nextframe);
     }
     */
    //warp_pointer(XDataToScreen(xdata), YDataToScreen(ydata));
}

// note, this uses the same basex, basey as setPan. 
// So need to use pixToMagnet and magnetToPix to convert from baseView to overlay view.
void Gframe::setOverlayPan(int x, int y, bool moving) {
    spViewInfo_t view = getSelView();
    if (view == nullView) {
        return;
    }
    if (layerID < 1 || viewList->size()<2) {
        setPan(x,y,moving);
        return;
    }
    spViewInfo_t baseView = getFirstView();
    if (baseView == nullView) {
	Winfoprintf("Error overlay: no base image.");
	return;
    }

    // convert base data x,y to user frame
    double bpx, bpy;
    double bux,buy,buz;
    baseView->dataToPix(basex, basey, bpx, bpy);
    baseView->pixToMagnet(bpx,bpy,0.0,bux,buy,buz);
    bux += overlayOff[0]; 
    buy += overlayOff[1]; 
    buz += overlayOff[2]; 

    // convert center data to user frame 
    double cux,cuy,cuz;
    baseView->pixToMagnet(baseView->pixstx + 0.5*baseView->pixwd, baseView->pixsty + 0.5*baseView->pixht, 0.0, cux,cuy,cuz);

    // convert clicked data to user frame 
    double ux,uy,uz;
    baseView->pixToMagnet((double)x,(double)y,0.0,ux,uy,uz);

    spImgInfo_t oimg = view->imgInfo;
    if(oimg == nullImg) return;

    double fit = pixelsPerCm_fit; // save pixelsPerCm_fit before fit overlay image

    double ox1, ox2, oy1, oy2;
    oimg->getLabFrameImageExtents(ox1, oy1, ox2, oy2);
    view->pixelsPerCm = getFitInFrame(fabs(ox2 - ox1), fabs(oy2 - oy1), pixwd, pixht,
            view->getRotation(), view->pixwd, view->pixht);
    view->pixstx = pixstx + pixstxOffset;
    view->pixsty = pixsty + pixstyOffset;
    view->updateScaleFactors(); // Update data <--> pixel coord conversions

    pixelsPerCm_fit = fit;

    if(!parallelPlane(view,baseView) ) {
        imgBackupOod = true;
	Winfoprintf("Warning overlay: image orientations are different.");
	return;
    }
/*
    if(coPlane(view,baseView) ) {
	Winfoprintf("Images are co-plane.");
    }
*/
 
    view->cm_off[0]=bux-ux;
    view->cm_off[1]=buy-uy;
    view->cm_off[2]=buz-uz;

    checkOffset(baseView,view->cm_off);

    double xDataCenter = cux + view->cm_off[0];
    double yDataCenter = cuy + view->cm_off[1];
    double zDataCenter = cuz + view->cm_off[2];

    if(!calcZoom(view,xDataCenter,yDataCenter,zDataCenter,pixelsPerCm,baseView)) {
	return;
    }

    imgBackupOod = true;
    draw();
}

void Gframe::checkOffset(spViewInfo_t baseView, double *offset) {

    spImgInfo_t bimg = baseView->imgInfo;
    if(bimg != nullImg) {
       double ox1, ox2, oy1, oy2;
       bimg->getLabFrameImageExtents(ox1, oy1, ox2, oy2);
       if(offset[0] < ox1) offset[0] = ox1;
       if(offset[0] > ox2) offset[0] = ox2;
       if(offset[1] < oy1) offset[1] = oy1;
       if(offset[1] > oy2) offset[1] = oy2;
    }
}

void Gframe::setBase(int x, int y) {
    spViewInfo_t view = getFirstView();
    if (view == nullView) {
        return;
    }
    view->pixToData(x, y, basex, basey);

    if (viewList->size()<2) return;
    view = getSelView();
    if (view == nullView) return;

    overlayOff[0]=view->cm_off[0];
    overlayOff[1]=view->cm_off[1];
    overlayOff[2]=view->cm_off[2];
}

void Gframe::setPan(int x, int y, bool moving) {
    double datax, datay;
    spViewInfo_t view = getFirstView();
    if (view == nullView) {
        return;
    }
    view->pixToData(x, y, datax, datay);
    spImgInfo_t img = view->imgInfo;
    spDataInfo_t di = img->getDataInfo();
    // NB: Cursor was at basex.  Because of cursor motion, datax != basex.
    // We need to move the image so that the cursor is again at basex.
    double dataCenterX, dataCenterY;
    view->pixToData((minX() + maxX()) / 2, (minY() + maxY()) / 2, dataCenterX,
            dataCenterY);
    dataCenterX += basex - datax;
    dataCenterY += basey - datay;

    // convert data center to magnet frame for aipZoomBind to use
    // do this before setZoom bacause it will change dataToPix and piToMagnet.
    double mx,my,mz;
    double px,py;
    view->dataToPix(dataCenterX, dataCenterY,px,py);
    view->pixToMagnet(px,py,0.0,mx,my,mz);

    setZoom(dataCenterX, dataCenterY, pixelsPerCm);

    int mode;
    if ((mode = (int)getReal("aipZoomBind", 0)) > 0&& (!moving || getReal(
            "aipZoomBindOnDrag", 0) != 0)) {

        // mode: 1==> bind selected; 2==> bind all
        if (mode == 1&& !selected) {
            return;
        }

        spImgInfo_t img = view->imgInfo;
        spDataInfo_t di = img->getDataInfo();
        int sizex = di->getFast();
        int sizey = di->getMedium();
        double scalex=1;
        double scaley=1;
        spViewInfo_t view2;
	int magnet = (int)getReal("aipZoomBindMode", 0);

        // Set zoom for other frames the same way
        GframeManager *gfm = GframeManager::get();
        spGframe_t gf;
        GframeList::iterator gfi;
        gf = mode==1 ? gfm->getFirstSelectedFrame(gfi)
                : gfm->getFirstFrame(gfi);
        while (gf != nullFrame) {
            if (gf.get() != this) {
                if ((view2=gf->getFirstView()) != nullView) {
        	    img = view2->imgInfo;
        	    di = img->getDataInfo();
		    if(magnet) {
			double px1,px2,px3;
			view2->magnetToPix(mx,my,mz,px1,px2,px3);
		        view2->pixToData((int)px1, (int)px2,dataCenterX,dataCenterY);
                        gf->setZoom(dataCenterX*scalex, dataCenterY*scaley, pixelsPerCm);
		    } else { 
        	    	if(sizex>0) scalex = ((double)di->getFast())/sizex;
        	    	if(sizey>0) scaley = ((double)di->getMedium())/sizey;
                        gf->setZoom(dataCenterX*scalex, dataCenterY*scaley, pixelsPerCm);
		    }
		}
            }
            gf = mode==1 ? gfm->getNextSelectedFrame(gfi)
                    : gfm->getNextFrame(gfi);
        }
    }

}

void Gframe::segmentSelectedRois(bool minFlag, double minData, bool maxFlag,
        double maxData, bool clear) {
    char *cptr;
    float *data;
    spImgInfo_t img = getFirstImage();
    if (img == nullImg) {
        return; // Nothing to do
    }

    spDataInfo_t di = img->getDataInfo();
    float *fptr = di->getData();
    float fmin = minData;
    float fmax = maxData;

    // Initialize a flag buffer the size of the image
    int size = di->getFast() * di->getMedium();
    char *flag = new char[size];
    (void)memset(flag, 0, size);
    char *cend = flag + size;

    // For every selected ROI
    RoiManager *roim = RoiManager::get();
    Roi *r;
    
    for (r=roim->getFirstSelected(); r; r=roim->getNextSelected()) {
        // If ROI is in this frame ...
        if (r->pOwnerFrame == this) {
            //check Roi size. return if bigger than data size.
            spCoordVector_t dat = r->getpntData();
            int rsize;
            for (int i=0; i<(int) dat->coords.size(); i++) {
                rsize = (int)(dat->coords[i].x*dat->coords[i].y);
                if (rsize > size)
                    return;
            }
            // ... flag the pixels inside the ROI
            for (data = r->firstPixel(); data; data = r->nextPixel()) {
                flag[data-fptr] = 1;
            }
        }
    }

    // Clip flagged points; zero or ignore unflagged points
    if (minFlag && maxFlag && clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (!*cptr || *fptr < fmin || *fptr > fmax) {
                *fptr = 0.0;
            }
        }
    } else if (minFlag && maxFlag && !clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (*cptr && (*fptr < fmin || *fptr > fmax)) {
                *fptr = 0.0;
            }
        }
    } else if (minFlag && clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (!*cptr || *fptr < fmin) {
                *fptr = 0.0;
            }
        }
    } else if (minFlag && !clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (*cptr && *fptr < fmin) {
                *fptr = 0.0;
            }
        }
    } else if (maxFlag && clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (!*cptr || *fptr > fmax) {
                *fptr = 0.0;
            }
        }
    } else if (maxFlag && !clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (*cptr && *fptr > fmax) {
                *fptr = 0.0;
            }
        }
    } else if (clear) {
        for (cptr=flag; cptr<cend; cptr++, fptr++) {
            if (!*cptr) {
                *fptr = 0.0;
            }
        }
    }
    delete[] flag;
    setDisplayOOD(true);
    imgBackupOod = true;
    setViewDataOOD(true);
    //GframeManager *gfm = GframeManager::get();
    //spGframe_t gf=gfm->getPtr(this);
    //if (gfm->isFrameDisplayed(gf))
        draw();
    RoiStat::get()->calculate(false);
}

void Gframe::segmentImage(bool minFlag, double minData, bool maxFlag,
        double maxData) {
    // float *data;
    spImgInfo_t img = getFirstImage();
    if (img == nullImg) {
        return; // Nothing to do
    }
    spDataInfo_t di = img->getDataInfo();
    float *fptr = di->getData();
    float fmin = minData;
    float fmax = maxData;

    int size = di->getFast() * di->getMedium();
    float *fend = fptr + size;

    // Zero points outside of range
    if (minFlag && maxFlag) {
        for (; fptr < fend; fptr++) {
            if (*fptr < fmin || *fptr > fmax) {
                *fptr = 0;
            }
        }
    } else if (minFlag) {
        for (; fptr < fend; fptr++) {
            if (*fptr < fmin) {
                *fptr = 0;
            }
        }
    } else if (maxFlag) {
        for (; fptr < fend; fptr++) {
            if (*fptr > fmax) {
                *fptr = 0;
            }
        }
    } else {
        fprintf(stderr,"No limits specified for segmentation.");
        return;
    }
    setDisplayOOD(true);
    imgBackupOod = true;
    setViewDataOOD(true);
    draw();
    RoiStat::get()->calculate(false);
}

// Go through all of the Roi, and call flagUpdate for each.
// This is to flag them as out of date.
void Gframe::flagUpdateAllRoi() {
    Roi *roi;

    for (roi=getFirstRoi(); roi; roi=getNextRoi()) {
        roi->flagUpdate();
    }
}

// Go through all of the Roi, and call rot90_data_coords for each.
void Gframe::rot90DataCoordsAllRoi(int datawidth) {
    Roi *roi;

    for (roi=getFirstRoi(); roi; roi=getNextRoi()) {
        roi->rot90_data_coords(datawidth);
    }
}

// Go through all of the Roi, and call flip_data_coords for each.
void Gframe::flipDataCoordsAllRoi(int datawidth) {
    Roi *roi;

    for (roi=getFirstRoi(); roi; roi=getNextRoi()) {
        roi->flip_data_coords(datawidth);
    }
}

void Gframe::setZoom(double xDataCenter, double yDataCenter, double newPixPerCm) {
    spViewInfo_t view = getFirstView();
    if (view == nullView) {
        return;
    }

    // Check for problems
    spImgInfo_t img = view->imgInfo;
    spDataInfo_t di = img->getDataInfo();
    if (xDataCenter < 0 || xDataCenter > di->getFast()
       || yDataCenter < 0 || yDataCenter > di->getMedium()) return;

    double bpx, bpy;
    double bux,buy,buz;
    view->dataToPix(xDataCenter, yDataCenter, bpx, bpy);
    view->pixToMagnet(bpx,bpy,0.0,bux,buy,buz);
    if(!calcZoom(view,bux,buy,buz,newPixPerCm)) return;
    //if(!calcZoom(view,xDataCenter,yDataCenter,newPixPerCm)) return;

    pixelsPerCm = newPixPerCm;

    view->pixelsPerCm = pixelsPerCm;
    view->updateScaleFactors(); // Update data <--> pixel coord conversions
    updateScaleFactors(); // Update pixel <--> magnet conversions

    // Fix ROI displayed sizes
    updateRoiPixels();

    // zoom overlay view(s)
    if(viewList->size()>1) {
       ViewInfoList::iterator pView = viewList->begin();
       pView++;
       while(pView != viewList->end()) {
	 fitOverlayView(*pView);
         pView++;
       } 
    }
    draw();
}

// note, xDataCenter,yDataCenter, are "User" coordinates corresponding to pix center.
int Gframe::calcZoom(spViewInfo_t view, double xDataCenter, double yDataCenter, double mz, double newPixPerCm, spViewInfo_t baseView) {
    if (view == nullView) {
        return 0;
    }
    spImgInfo_t img = view->imgInfo;
    spDataInfo_t di = img->getDataInfo();

/* comment out because xDataCenter,yDataCenter are "User" frame center, not Data center
    // Check for problems
    if (xDataCenter < 0) {
        xDataCenter = 0;
    } else if (xDataCenter > di->getFast()) {
        xDataCenter = di->getFast();
    }
    if (yDataCenter < 0) {
        yDataCenter = 0;
    } else if (yDataCenter > di->getMedium()) {
        yDataCenter = di->getMedium();
    }
*/
    if (newPixPerCm * di->getRoi(0) < 10&&newPixPerCm * di->getRoi(1) < 10) {
        return 0; // Don't make image too small
    }
    int maxv,minv,maxh,minh; 
    if(baseView != nullView) {
	minh = baseView->pixstx;
	minv = baseView->pixsty;
	maxh = baseView->pixstx+baseView->pixwd;
	maxv = baseView->pixsty+baseView->pixht;
    } else {
	minh = minX();
	minv = minY();
	maxh = maxX();
	maxv = maxY();
    }
    double xDatelsPerPixel = di->getFast() / (di->getSpan(0) * newPixPerCm);
    double yDatelsPerPixel = di->getMedium() / (di->getSpan(1) * newPixPerCm);
    if (view->getRotation() & 4) {
        // X/Y are swapped between data and pixels
        if (xDatelsPerPixel * (maxv - minv + 1)< 0.5&&yDatelsPerPixel
                * (maxh - minh + 1)< 0.5) {
            return 0; // Don't make image too big
        }
    } else {
        if (xDatelsPerPixel * (maxh - minh + 1)< 0.5&&yDatelsPerPixel
                * (maxv - minv + 1)< 0.5) {
            return 0; // Don't make image too big
        }
    }

    // First, set scale factors to desired scale, without worrying
    // about correct position and extents
    view->pixwd = maxh - minh + 1;
    view->pixht = maxv - minv + 1;
    img->datawd = view->pixwd*di->getFast() / (di->getSpan(0)*newPixPerCm);
    img->dataht = view->pixht*di->getMedium() / (di->getSpan(1)*newPixPerCm);
    if (view->getRotation() & 4) {
        // X/Y are swapped between data and pixels
        img->datawd *= (double)view->pixht / view->pixwd;
        img->dataht *= (double)view->pixwd / view->pixht;
    }
    view->pixelsPerCm = newPixPerCm;
    view->updateScaleFactors(); // Update data <--> pixel coord conversions

    // Get distance of desired center from data edges in units of
    // the new pixel distances.
    double pixCorner0X, pixCorner0Y;
    view->dataToPix(0, 0, pixCorner0X, pixCorner0Y);
    double pixCorner1X, pixCorner1Y;
    view->dataToPix(di->getFast(), di->getMedium(), pixCorner1X, pixCorner1Y);
    if (pixCorner0X > pixCorner1X) {
        swap(pixCorner0X, pixCorner1X);
    }
    if (pixCorner0Y > pixCorner1Y) {
        swap(pixCorner0Y, pixCorner1Y);
    }
    double xCenter, yCenter, z;
    //view->dataToPix(xDataCenter, yDataCenter, xCenter, yCenter);
    view->magnetToPix(xDataCenter, yDataCenter, mz, xCenter, yCenter,z);
    double offWest = (xCenter - pixCorner0X);
    double offEast = (pixCorner1X - xCenter);
    double offNorth = (yCenter - pixCorner0Y);
    double offSouth = (pixCorner1Y - yCenter);

    // Set new image boundaries in pixels
    // We always put the specified point in the center.
    double pixCenterX = (maxh + minh) / 2;
    double pixCenterY = (maxv + minv) / 2;
    double pstx, psty, pwd, pht;
    if (minh < pixCenterX - offWest) {
        pstx = pixCenterX - offWest;
    } else {
        pstx = minh;
    }
    if (minv < pixCenterY - offNorth) {
        psty = pixCenterY - offNorth;
    } else {
        psty = minv;
    }
    if (maxh > pixCenterX + offEast) {
        pwd = pixCenterX + offEast - pstx + 1;
    } else {
        pwd = maxh - pstx + 1;
    }
    if (maxv > pixCenterY + offSouth) {
        pht = pixCenterY + offSouth - psty + 1;
    } else {
        pht = maxv - psty + 1;
    }

    // Now calculate where the new corners are in data space.
    // We offset the new corner pixel positions to the old (current)
    // scaling, and transform to data coordinates.
    double px0 = pstx - pixCenterX + xCenter;
    double py0 = psty - pixCenterY + yCenter;
    double px1 = pstx + pwd - 1- pixCenterX + xCenter;
    double py1 = psty + pht - 1- pixCenterY + yCenter;
    double newDataX0, newDataY0, newDataX1, newDataY1;
    view->pixToData(px0, py0, newDataX0, newDataY0);
    view->pixToData(px1, py1, newDataX1, newDataY1);
    if (newDataX0 > newDataX1) {
        swap(newDataX0, newDataX1);
    }
    if (newDataY0 > newDataY1) {
        swap(newDataY0, newDataY1);
    }

    // Change the settings
    img->datastx = fabs(newDataX0);
    img->datasty = fabs(newDataY0);
    img->datawd = fabs(newDataX1 - newDataX0);
    img->dataht = fabs(newDataY1 - newDataY0);
    view->pixstx = (int) pstx;
    view->pixsty = (int) psty;
    view->pixwd = (int) pwd;
    view->pixht = (int) pht;

    if(baseView != nullView) {
      view->pixstx_off=view->pixstx-baseView->pixstx;
      view->pixsty_off=view->pixsty-baseView->pixsty;
      if(view->pixstx_off < 0) view->pixstx_off=0;
      if(view->pixsty_off < 0) view->pixsty_off=0;
    }

    return 1;
}

/* STATIC */
double Gframe::getFitInFrame(double cmWd, double cmHt, int frameWd,
        int frameHt, int rotation, int& imgWd, int& imgHt) {
    double scale; // pixels / cm
    if (rotation & 4) {
        swap(cmWd, cmHt);
    }
    frameWd -= 2 * pixstxOffset;
    frameHt -= 2 * pixstyOffset;
    if (fabs(frameWd / cmWd) > fabs(frameHt / cmHt)) {
        scale = fabs((frameHt - 1) / cmHt);
        imgWd = (int)(fabs(frameHt / cmHt) * cmWd);
        imgHt = frameHt;
    } else {
        scale =fabs((frameWd - 1) / cmWd);
        imgWd = frameWd;
        imgHt = (int)(fabs(frameWd / cmWd) * cmHt);
    }
    pixelsPerCm_fit = scale;
    return scale;
}

/* PRIVATE */
void Gframe::clearViewList() {
    /*
     spImgInfo_t imgInfo = getFirstImage();
     if (imgInfo != nullImg) {
     GframeManager::get()->unsaveFrame(imgInfo->getDataInfo()->getKey());
     }
     */
    clearRoiList();
    viewList->clear();
}

/* PRIVATE */
void Gframe::clearRoiList() {
    // We first make a list of what we are going to delete, because
    // "remove()" modifies the list, which would break the
    // iteration over ROIs.
    Roi *roi;
    RoiList tmpList;
    for (roi=getFirstRoi(); roi; roi=getNextRoi()) {
        tmpList.push_back(roi);
    }
    RoiList::iterator itr;
    for (itr=tmpList.begin(); itr!=tmpList.end(); itr++) {
        RoiManager::get()->remove(*itr);
    }
}

bool Gframe::hasSpec() {
   return specList->hasSpec();
}

void Gframe::setSpecKeys(std::list<string> keys) {
   specList->setSpecKeys(keys);
}

void Gframe::disSpec(int fx, int fy, int fw, int fh, bool update) {
   if(!hasSpec()) return;
   spViewInfo_t view = getFirstView();
   if(!specList->showSpec() && view == nullView) return; // no spec and no view
   if(view == nullView) update = true; // always update if no image displayed
   // update layout if frame or image is changed.
   if(update && specList->getLayoutType() == CSILAYOUT && view != nullView) { 
	SpecViewMgr::get()->updateCSIGrid(specList, id);
   } else if(update) {
	SpecViewMgr::get()->updateArrayLayout(specList, id);
   }
   specList->displaySpec(fx,fy,fw,fh);
}

void Gframe::selectSpecView(int x, int y, int mask) {
   if(!hasSpec()) return;
   int ind = specList->selectSpecView(x, y);
   SpecViewMgr::get()->selectSpecView(specList, ind, mask);
}

void Gframe::deleteSpecList() {
   specList->clearList();
}

void Gframe::resetSpecFrame() {
   zoomFactor=1.0;
   zoomSpecID=0; //valid zoomSpecID starts from 1
   SpecViewMgr::get()->updateArrayLayout(specList, id);
   draw();
}

void Gframe::zoomSpecFrame(int x, int y, double factor) {
   if(factor <= 0) {
	resetSpecFrame();
   } else {
      zoomFactor*=factor;
      zoomSpecID = specList->selectSpecView(x, y);
      SpecViewMgr::get()->updateArrayLayout(specList, id);
      draw();
   }
}

// ind start from 0
spViewInfo_t Gframe::getViewByIndex(int ind) {
    int i = 0;
    ViewInfoList::iterator pView;
    for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
        if(i == ind) return (*pView); 
 	i++;
    }
    return getFirstView();
}

// name is fnumber:shortname
// names are separated by space
string Gframe::getViewNames() {
    string names = "";
    spViewInfo_t view;
    spImgInfo_t img;
    ViewInfoList::iterator pView;
    int fnumber = getGframeNumber();
    char str[MAXSTR];
    for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
        img = (*pView)->imgInfo;
        if(img == nullImg) continue;

	sprintf(str,"%d:%s",fnumber,img->getDataInfo()->getShortName().c_str());
        names += img->getDataInfo()->getShortName(); 
	names += " ";
    }
    return names;
}

// keys are separated by space
string Gframe::getViewKeys() {
    string names = "";
    spViewInfo_t view;
    spImgInfo_t img;
    ViewInfoList::iterator pView;
    for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
        img = (*pView)->imgInfo;
        if(img == nullImg) continue;

        names += img->getDataInfo()->getKey(); 
	names += " ";
    }
    return names;
}

// ind start from 0
string Gframe::getViewName(int ind) {
   string name = "";
   spViewInfo_t view = getViewByIndex(ind);
   if(view == nullView) return name;

   spImgInfo_t img = view->imgInfo;
   if(img == nullImg) return name;

   name = img->getDataInfo()->getShortName().c_str();
   if (name.length() == 0) {
         DataManager::get()->makeShortNames();
         name = img->getDataInfo()->getShortName().c_str();
   }
   int fnumber = getGframeNumber();
   char str[MAXSTR];
   sprintf(str,"%d:%s",fnumber,name.c_str());
   return string(str);
}

void Gframe::setColormap(const char *name) {
   spViewInfo_t view = getViewByIndex(layerID);
   if(view == nullView) return;

   spImgInfo_t img = view->imgInfo;
   if(img == nullImg) return;
   img->setColormap(string(name)); 
   sendImageInfo();
}

void Gframe::setTransparency(int value) {
   spViewInfo_t view = getViewByIndex(layerID);
   if(view == nullView) return;

   spImgInfo_t img = view->imgInfo;
   if(img == nullImg) return;
   img->setTransparency(value); 
   // sendImageInfo();
}

void Gframe::removeOverlayImg() {
    layerID = 0;
    if(viewList->size() < 2) return;
    ViewInfoList::iterator pView = viewList->end();
    pView--;
    while(pView != viewList->begin()) {
	viewList->erase(pView);
        pView = viewList->end();
        pView--;
    }
    imgBackupOod=true;
    draw();
    setReal("aipLayerSel",0, true);
}

// path can be the key
void Gframe::loadOverlayImg(const char *path, const char *cmapName, int colormapId) {
   string key = string(path);
   if(strstr(path, " ") == NULL) { 
     key += " 0";
     key.replace(key.find_last_of("/"), 1, " ");
   }
   spDataInfo_t data = DataManager::get()->getDataInfoByKey(key, true);
   if(data == (spDataInfo_t)NULL) {
      Winfoprintf("Cannot load image data %s",path);
      return;
   }
   spImgInfo_t imgInfo = spImgInfo_t(new ImgInfo(data,true)); 
   if (imgInfo == nullImg) {
      Winfoprintf("Cannot load image info %s",path);
      return;
   }
   loadView(imgInfo, false);
   layerID=viewList->size()-1;
   setReal("aipLayerSel",viewList->size()-1,true);
   if (colormapId > 0) {
      imgInfo->setColormapID(colormapId);
   }
   else {
      string colorMapName = getString("aipOverlayColor", "");
      if(colorMapName != "") setColormap((char *)colorMapName.c_str()); 
      else if( strcmp(cmapName,"") ) setColormap(cmapName);
   }
   draw();

   GframeManager::overlaid=true;
   grabMouse();  // to update graphics toolbar
}

void Gframe::loadOverlayImg(const char *path, const char *cmapName) {
   loadOverlayImg(path, cmapName, 0);
}

// fill layer list
void Gframe::sendImageInfo() {

    set_aip_image_info(0,0,0,0,(char *)"",0); // always clear current info display

    if(!viewList->empty()) {

      int n = viewList->size();
      ViewInfoList::iterator pView = viewList->end();
      while(n>0) {
	n--;
  	pView--;
	spViewInfo_t view = *pView;
        if(view == nullView) continue;
        spImgInfo_t img = view->imgInfo;
        if(img == nullImg) continue;

        string imgName = getViewName(n);
        int mapId = img->getColormapID();
        int transparency = img->getTransparency();
	if(n==layerID)
          set_aip_image_info(n+1,n+1,mapId,transparency, (char *)imgName.c_str(),1);
        else
          set_aip_image_info(n+1,n+1,mapId,transparency, (char *)imgName.c_str(),0);
      }
    }
}

spViewInfo_t Gframe::getSelView() {
   spViewInfo_t view = getViewByIndex(layerID);
   if(view != nullView) return view;
   else return getFirstView();
}

spImgInfo_t Gframe::getSelImage() {
   spViewInfo_t view = getViewByIndex(layerID);
   if(view != nullView) {
      spImgInfo_t img = view->imgInfo;
      if(img != nullImg) return img;
   }
   return getFirstImage();
}

void Gframe::selectViewLayer(int imgId, int order, int mapId) {
  layerID = order-1;
  setReal("aipLayerSel", order-1, 0);
  VsInfo::setVsHistogram(this);
}

int Gframe::getPositionInfo(spViewInfo_t view, int x, int y, int &i, int &j, double &mag) {
   if(view == nullView) return 0;
   spImgInfo_t img = view->imgInfo;
   if (img == nullImg) return 0;
   spDataInfo_t di = img->getDataInfo();

   double dx,dy;
   view->pixToData((double)x, (double)y, dx,dy);
   i = (int)dx;
   j = (int)dy;
   if(i<0) i=0;
   if(j<0) j=0;
   float *data = di->getData()+di->getFast()*j + i;
   mag = (double)(*data); 
//Winfoprintf("info %d %d %d %d %d %d %f",x,y,i,j,di->getFast(),di->getMedium(),mag);
   return 1;
}


void Gframe::setPositionInfo(int x, int y) {
  
    char info[MAXSTR];
    if(P_getstring(GLOBAL, "aipPositionInfo", info, 1, MAXSTR)) return;

    if(!viewList->empty()) {

      int i=0;
      int dx,dy;
      double mag;
      string name;
      ViewInfoList::iterator pView;
      clearVar("aipPositionInfo");
      for (pView=viewList->begin(); pView!=viewList->end(); pView++) {
	spViewInfo_t view = *pView;
        if(view == nullView) continue;

        if(!getPositionInfo(view, x, y, dx, dy, mag)) continue;
        spImgInfo_t img = view->imgInfo;
        if (img == nullImg) continue;
        spDataInfo_t di = img->getDataInfo();

        i++;
        name = di->getShortName();
        int p = name.find_last_of(" ");
        if(p != (int) string::npos) {
           name=name.substr(p);
        }
        sprintf(info,"%s %d %d %.0f", name.c_str(),dx,dy,mag);
        P_setstring(GLOBAL, "aipPositionInfo", info, i); 
      }
      appendvarlist("aipPositionInfo");
    }
}
