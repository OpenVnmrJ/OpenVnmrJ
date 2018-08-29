/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <iostream>
#include <fstream>
using std::ofstream;
using std::ifstream;
#include <cmath>
using std::sqrt;

#include "graphics.h"
#include "aipVnmrFuncs.h"
#include "aipStructs.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipPoint.h"
#include "aipDataManager.h"
#include "aipAxisInfo.h"

static const int buflen = 100;
static char dataCoordStr[buflen] = "---";
static char intensityStr[buflen] = "---";
static char labCoordStr[buflen] = "---";
static char distanceStr[buflen] = "---";
static char projDistanceStr[buflen] = "---";

int Point::id = 1;
int Point::curr_id = 0;
int Point::prev_id = 0; // ID of the previous point
int Point::currRoiNum = 0;
int Point::prevRoiNum = 0;

/*
 * Constructor from pixel location.
 * Assume gf is valid.
 */
Point::Point(spGframe_t gf, int x, int y) :
    Roi() {
    init(gf, 1);
    created_type = ROI_POINT;
    myID = ++id; // Unique ID for this Point ROI
    initPix(x, y);
    pntData->name = "Point";
    magnetCoords->name="Point";
    Roi::create(gf);
}

/*
 * Constructor from data location.
 * Assume gf is valid.
 */
Point::Point(spGframe_t gf, spCoordVector_t dpts, bool pixflag) :
    Roi() {
    init(gf, 1);
    created_type = ROI_POINT;
    myID = ++id; // Unique ID for this Point ROI
    // note: dpts is in magnet frame  
    if(pixflag) {
       pntData = dpts;
       setMagnetCoordsFromPixels(); // TODO: Should be set from data
    } else {
       magnetCoords = dpts;
       setDataPntsFromMagnet();
    } 
    setPixPntsFromData(); // Sets pntPix, npnts, min/max
    gf->addRoi(this);
    pntData->name = "Point";
    magnetCoords->name="Point";
}

Point::~Point() {
    if (myID == curr_id) {
        curr_id = 0;
    }
    if (myID == prev_id) {
        prev_id = 0;
    }
}

/*
 * Update the position of a point.
 */
ReactionType Point::create(short x, short y, short action) {
    if (action == movePoint || action == movePointConstrained) {
        return move(x, y);
    } else {
        return REACTION_NONE;
    }
}

/*
 * Copy this ROI to another Gframe
 */
Roi *Point::copy(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return NULL;
    }
    Roi *roi;
    roi = new Point(gframe, magnetCoords);
    return roi;
}

/*
 * Move a point to a new location.
 */
ReactionType Point::move(short x, short y) {
    double dist_x = x - basex; // distance x
    double dist_y = y - basey; // distance y

    keep_roi_in_image(&dist_x, &dist_y);

    // Same position, do nothing
    if ((!dist_x) && (!dist_y)) {
        return REACTION_NONE;
    }

    erase(); // Erase previous point

    // Update point's pixel, data & magnet coords
    setPixelCoord(0, pntPix[0].x + dist_x, pntPix[0].y + dist_y);

    x_min = x_max = pntPix[0].x;
    y_min = y_max = pntPix[0].y;
    basex += dist_x;
    basey += dist_y;

    draw(); // Draw new point

    someInfo(true); // MovingFlag = true
    updateSlaves(true);

    return REACTION_NONE;
}

/*
 * User just stops moving.
 */
ReactionType Point::move_done(short, short) {
    //x_min = x_max = pntPix[0].x;
    //y_min = y_max = pntPix[0].y;

    //setMagnetCoords();
    someInfo(false); // MovingFlag = false

    basex = ROI_NO_POSITION;
    return REACTION_NONE;
}

/*
 * How far is the Point from a given pixel?
 * If "far" >= 0, can return "far" if actual distance is > "far".
 * This implementation always returns the actual distance.
 */
double Point::distanceFrom(int x, int y, double far) {
    double dx = pntPix[0].x - x;
    double dy = pntPix[0].y - y;
    return sqrt(dx * dx + dy * dy);
}

/*
 * Check whether a point is selected or not.
 * This will happen when the mouse button is clicked within "aperture"
 * pixels of the point.
 * Return true if it is selected, otherwise false.
 */
bool Point::is_selected(short x, short y) {

    if ((fabs(pntPix[0].x - x) < aperture) &&(fabs(pntPix[0].y - y) < aperture)) {
        acttype = ROI_MOVE ;
        return true;
    } else {
        return false;
    }
}

/*
 * We cannot select a handle on a point
 */
int Point::getHandle(int x, int y) {
    return -1;
}

/*
 *  Save the current ROI point into the following format:
 *
 *     # <comments>
 *     Point
 *     X Y
 *
 *  where:
 *        # indicates comments
 *        X,Y is the point
 */
void Point::save(ofstream& outfile, bool pixflag) {
    outfile << name() << "\n";
    //outfile << magnetCoords->coords[0].x << " ";
    //outfile << magnetCoords->coords[0].y << "\n";
  if(pixflag) {
    outfile << pntData->coords[0].x<< " ";
    outfile << pntData->coords[0].y<< "\n";
  } else {
    outfile << magnetCoords->coords[0].x<< " ";
    outfile << magnetCoords->coords[0].y<< " ";
    outfile << magnetCoords->coords[0].z<< "\n";
  }
}


void Point::draw() {
    if (pntPix[0].x == ROI_NO_POSITION || !drawable || !isVisible()) {
        return;
    }
    calc_xyminmax();

    if (visibility != VISIBLE_NEVER) {
        pOwnerFrame->setClipRegion(FRAME_CLIP_TO_IMAGE);
        GraphicsWin::drawLine (pntPix[0].x - HAIR_LEN, pntPix[0].y, pntPix[0].x
                - HAIR_GAP, pntPix[0].y, my_color);
        GraphicsWin::drawLine (pntPix[0].x + HAIR_LEN, pntPix[0].y, pntPix[0].x
                + HAIR_GAP, pntPix[0].y, my_color);
        GraphicsWin::drawLine (pntPix[0].x, pntPix[0].y - HAIR_LEN,
                pntPix[0].x, pntPix[0].y - HAIR_GAP, my_color);
        GraphicsWin::drawLine (pntPix[0].x, pntPix[0].y + HAIR_LEN,
                pntPix[0].x, pntPix[0].y + HAIR_GAP, my_color);
        pOwnerFrame->setClipRegion(FRAME_NO_CLIP);
    }
    if (roi_state(ROI_STATE_MARK)) {
        mark();
    }

    bool show = (getReal("aipShowROIPos", 0)>0);
    if(show && getReal("aipShowROIOpt", 0) > 0) { // show intensity
      showIntensity();
    } else if(show) { // show position
      spGframe_t gf = GframeManager::get()->getPtr(pOwnerFrame);
      AxisInfo::showPosition(gf,(int)pntPix[0].x,(int)pntPix[0].y,true,false);
    }
    drawRoiNumber();

}

void Point::mark(void) {
    if (!markable || pntPix[0].x == ROI_NO_POSITION) {
        return;
    }
    pOwnerFrame->setClipRegion(FRAME_CLIP_TO_IMAGE);
    draw_mark((int)pntPix[0].x + HAIR_LEN, (int)pntPix[0].y + HAIR_LEN);
    draw_mark((int)pntPix[0].x + HAIR_LEN, (int)pntPix[0].y - HAIR_LEN);
    draw_mark((int)pntPix[0].x - HAIR_LEN, (int)pntPix[0].y + HAIR_LEN);
    draw_mark((int)pntPix[0].x - HAIR_LEN, (int)pntPix[0].y - HAIR_LEN);
    pOwnerFrame->setClipRegion(FRAME_NO_CLIP);
}

void Point::erase() {
    pOwnerFrame->setClipRegion(FRAME_CLIP_TO_IMAGE);
    if(getReal("aipShowROIPos", 0)>0) pOwnerFrame->draw(); 
    redisplay_bkg((int)x_min - HAIR_LEN- MARK_SIZE, (int)y_min - HAIR_LEN- MARK_SIZE,
            (int)x_max + HAIR_LEN+ MARK_SIZE, (int)y_max + HAIR_LEN+ MARK_SIZE);
    pOwnerFrame->setClipRegion(FRAME_NO_CLIP);
    roi_clear_state(ROI_STATE_EXIST);
    eraseRoiNumber();
}

/* STATIC */
void Point::clearInfo() {
    RoiManager::infoPoint = NULL;
    strcpy(dataCoordStr, "Data Coordinates:");
    setString("aipPointDataCoordsMsg", dataCoordStr, true);
    strcpy(intensityStr, "Intensity:");
    setString("aipPointIntensityMsg", intensityStr, true);
    strcpy(labCoordStr, "Lab Coordinates:");
    setString("aipPointLabCoordsMsg", labCoordStr, true);
    strcpy(distanceStr, " ");
    setString("aipPointSeparationMsg", distanceStr, true);
    strcpy(projDistanceStr, " ");
    setString("aipPointProjSeparationMsg", projDistanceStr, true);
    setReal("aipPointInfoNumber", 0, true);
}

void Point::someInfo(bool isMoving, bool clear) {
    char buf[buflen];

    static Dpoint prev_loc2D = { 0, 0 }; // Position of last point in cm
    static D3Dpoint prev_loc3D = { 0, 0, 0 }; // Previous position in magnet frame
    static Dpoint curr_loc2D = { 0, 0 }; // Position of last point in cm
    static D3Dpoint curr_loc3D = { 0, 0, 0 }; // Current position
    // Implementation note: The current and previous cursor positions are
    //  stored in the static variables curr_... and prev_... at the
    //  end of this routine.  What gets stored where depends on the
    //  IDs of the cursor (point) objects.  The idea is that we need
    //  to print the distance from the current cursor to the previous
    //  different cursor.  We don't want to overwrite the previous
    //  position of the other cursor with the position of the current
    //  one.  If "this->myID" is the same as curr_id, the
    //  curr_... variables are just updated.  If it is different, the
    //  curr_... variables replace the prev_... ones and the new
    //  values are placed in the curr_...  variables.  The distances
    //  are always those between the just retrieved location, and the
    //  most recent other location with a different ID.

    if (clear) {
        RoiManager::infoPoint = NULL;
    } else {
        RoiManager::infoPoint = this;
    }
    setReal("aipPointInfoNumber", getRoiNumber(), true);

    // Find which frame we are in
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    spDataInfo_t di = img->getDataInfo();

    // Which data pixel we're on
/*
    int x = (int)pntData->coords[0].x;
    int y = (int)pntData->coords[0].y;
 */

    if (clear) {
        sprintf(buf, "Data Coordinates:");
    } else {
        sprintf(buf, "Data Coordinates: (%.2f, %.2f) voxels",
                pntData->coords[0].x, pntData->coords[0].y);
    }
    if (strcmp(buf, dataCoordStr)) {
        strcpy(dataCoordStr, buf);
        setString("aipPointDataCoordsMsg", buf, true);
    }

    // Calculate the scale factors
    double wd = di->getSpan(0);
    double ht = di->getSpan(1);
    double xscale = wd / di->getFast();
    double yscale = ht / di->getMedium();
    Dpoint_t loc2D;
    loc2D.x = pntData->coords[0].x * xscale;
    loc2D.y = pntData->coords[0].y * yscale;

    // Display intensity and coordinate info.
    float intensity = *firstPixel();
    if (clear) {
        sprintf(buf, "Intensity:");
    } else {
        sprintf(buf, "Intensity: %.4g", intensity);
    }
    if (strcmp(buf, intensityStr)) {
        strcpy(intensityStr, buf);
        setString("aipPointIntensityMsg", buf, true);
    }

    if (clear) {
        sprintf(buf, "Lab Coordinates:");
    } else {
        sprintf(buf, "Lab Coordinates: (%.2f, %.2f, %.2f) cm",
                magnetCoords->coords[0].x, magnetCoords->coords[0].y,
                magnetCoords->coords[0].z);
    }
    if (strcmp(buf, labCoordStr)) {
        strcpy(labCoordStr, buf);
        setString("aipPointLabCoordsMsg", buf, true);
    }

    // Find distances from previously selected point
    int found = 0;
    double dx, dy, dz;
    double dx2, dy2;
    int otherRoiNum;
    if (!clear && curr_id && curr_id != myID) {
        otherRoiNum = currRoiNum;
        dx = curr_loc3D.x - magnetCoords->coords[0].x;
        dy = curr_loc3D.y - magnetCoords->coords[0].y;
        dz = curr_loc3D.z - magnetCoords->coords[0].z;
        dx2 = curr_loc2D.x - loc2D.x;
        dy2 = curr_loc2D.y - loc2D.y;
        found++;
    } else if (!clear && prev_id && prev_id != myID) {
        otherRoiNum = prevRoiNum;
        dx = prev_loc3D.x - magnetCoords->coords[0].x;
        dy = prev_loc3D.y - magnetCoords->coords[0].y;
        dz = prev_loc3D.z - magnetCoords->coords[0].z;
        dx2 = prev_loc2D.x - loc2D.x;
        dy2 = prev_loc2D.y - loc2D.y;
        found++;
    }

    // Display the relevant distances
    // (We should really figure out if the two images are in the same
    // location and orientation, and, if they are, only print out the
    // "distance"--which is the same as the projected distance.  If the
    // orientations of the two images differ, the "projected distance"
    // doesn't make much sense.)
    if (found) {
        double dist2D = sqrt(dx2*dx2 + dy2*dy2);
        double dist3D = sqrt(dx*dx + dy*dy + dz*dz);
        sprintf(buf, "Distance from Point #%d: %.2f cm", otherRoiNum, dist3D);
        if (strcmp(buf, distanceStr)) {
            strcpy(distanceStr, buf);
            setString("aipPointSeparationMsg", buf, true);
        }
        //setReal("aipPointSeparation", dist3D, true);
        sprintf(buf, "Projected onto Image Plane: %.2f cm", dist2D);
        if (strcmp(buf, projDistanceStr)) {
            strcpy(projDistanceStr, buf);
            setString("aipPointProjSeparationMsg", buf, true);
        }
        //setReal("aipPointProjSeparation", dist2D, true);
    } else {
        sprintf(buf, " ");
        if (strcmp(buf, distanceStr)) {
            strcpy(distanceStr, buf);
            setString("aipPointSeparationMsg", buf, true);
        }
        //setReal("aipPointSeparation", -1, true);
        if (strcmp(buf, projDistanceStr)) {
            strcpy(projDistanceStr, buf);
            setString("aipPointProjSeparationMsg", buf, true);
        }
        //setReal("aipPointProjSeparation", -1, true);
    }

    // Remember facts about this point
    if (curr_id != myID) {
        prevRoiNum = currRoiNum;
        prev_id = curr_id;
        prev_loc3D = curr_loc3D;
        prev_loc2D = curr_loc2D;
        curr_id = myID;
        currRoiNum = getRoiNumber();
    }
    curr_loc3D = magnetCoords->coords[0];
    curr_loc2D = loc2D;
}

float *Point::firstPixel() {
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    if (img == nullImg) {
        return NULL;
    }
    spDataInfo_t di = img->getDataInfo();
    float *data = (di->getData()+ (int)pntData->coords[0].x+ di->getFast()
            * (int)pntData->coords[0].y);
    return data;
}
