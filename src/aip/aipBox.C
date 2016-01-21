/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <limits>
#include <iostream>
#include <fstream>
using std::ofstream;
using std::ifstream;
#include <cmath>
using std::sqrt;
#include <algorithm>
using std::min;
using std::max;
#include <math.h>               // For copysign()
#include "aipUtils.h"
#include "aipStructs.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipImgInfo.h"
#include "aipBox.h"
#include "aipOval.h"
#include "aipReviewQueue.h"
#include "aipDataManager.h"

/*
 * Default constructor (for sub-classes).
 */
Box::Box() :
    Roi() {
    //pntData->name = name();
}

/*
 * Constructor from pixel location.
 * Assume gf is valid.
 */
Box::Box(spGframe_t gf, int x, int y) : Roi() {
    created_type = ROI_BOX;
    init(gf, 2);
    initPix(x, y);
    pntData->name = "Box";
    magnetCoords->name="Box";
    Roi::create(gf);   
}

/*
 * Constructor from data location.
 * Assume gf is valid.
 */
Box::Box(spGframe_t gf, spCoordVector_t dpts, bool pixflag) : Roi() {
    created_type = ROI_BOX;
    init(gf, 2);
    if(pixflag) {
       pntData = dpts;
       setMagnetCoordsFromPixels(); // TODO: Should be set from data
    } else {
       magnetCoords = dpts; // dpts is magnetCoords
       setDataPntsFromMagnet(); // fill pntData
    }
    setPixPntsFromData(); // Sets pntPix, npnts, min/max
    gf->addRoi(this);
    pntData->name = "Box";
    magnetCoords->name="Box";
}

void Box::setBase(int x, int y) {
    basex = x;
    basey = y;
    // NB: off_x is the offset of the desired position from
    //     the pointer position.
    off_x = ( (rolloverHandle==0) || (rolloverHandle== 3) ) ? (int) x_min : (int) x_max;
    off_y = (rolloverHandle<2) ? (int) y_min : (int) y_max;
    off_x -= x;
    off_y -= y;
    opposite_x = rolloverHandle==0|| rolloverHandle== 3 ? x_max : x_min;
    opposite_y = rolloverHandle<2 ? y_max : y_min;
}
/*
 * Update one corner of a box.
 */
ReactionType Box::create(short x, short y, short constraint) {
    int i;
    double this_x = x + off_x;
    double this_y = y + off_y;

    if (constraint == movePointConstrained) {
        int dx = (int) (this_x - opposite_x);
        int dy = (int) (this_y - opposite_y);
        int d = (int) (sqrt((double)(dx * dx + dy * dy)) / 1.414);
        this_x = opposite_x + copysign(d, dx);
        this_y = opposite_y + copysign(d, dy);
        double imgX0, imgY0, imgX1, imgY1;
        getImageBoundsInPixels(imgX0, imgY0, imgX1, imgY1);
        clipLineToRect(opposite_x, opposite_y, this_x, this_y, imgX0, imgY0,
                imgX1, imgY1);
    } else if (constraint == movePoint) {
        keep_point_in_image(&this_x, &this_y);
    } else {
        return REACTION_NONE;
    }

    erase();

    if (this_x < opposite_x) {
        x_min = this_x;
        x_max = opposite_x;
        if (this_y < opposite_y) {
            rolloverHandle = 0;
            y_min = this_y;
            y_max = opposite_y;
        } else {
            rolloverHandle = 3;
            y_min = opposite_y;
            y_max = this_y;
        }
    } else {
        x_min = opposite_x;
        x_max = this_x;
        if (this_y < opposite_y) {
            rolloverHandle = 1;
            y_min = this_y;
            y_max = opposite_y;
        } else {
            rolloverHandle = 2;
            y_min = opposite_y;
            y_max = this_y;
        }
    }

    setPixelCoord(0, x_min, y_min);
    setPixelCoord(1, x_max, y_max);
    draw();
    updateSlaves(true);
    return REACTION_NONE;
}

/*
 * Move a box to a new location.
 */
ReactionType Box::move(short x, short y) {
    double dist_x = x - basex; // distance x
    double dist_y = y - basey; // distance y

    keep_roi_in_image(&dist_x, &dist_y);

    // Same position, do nothing
    if ((!dist_x) && (!dist_y)) {
        return REACTION_NONE;
    }

    erase(); // Erase previous box

    // Update box's pixel, data & magnet coords
    setPixelCoord(0, pntPix[0].x + dist_x, pntPix[0].y + dist_y);
    setPixelCoord(1, pntPix[1].x + dist_x, pntPix[1].y + dist_y);

    x_min += dist_x;
    x_max += dist_x;
    y_min += dist_y;
    y_max += dist_y;

    draw(); // Draw a box

    basex += dist_x;
    basey += dist_y;

    updateSlaves(true);
    return REACTION_NONE;
}

/************************************************************************
 *                                  *
 *  User just stops moving.                     *
 *                                  */
ReactionType Box::move_done(short, short) {
    basex = ROI_NO_POSITION;

    // Assign minmax value for a box
    calc_xyminmax();
    //x_min = min(pntPix[0].x, pntPix[1].x);
    //x_max = max(pntPix[0].x, pntPix[1].x);
    //y_min = min(pntPix[0].y, pntPix[1].y);
    //y_max = max(pntPix[0].y, pntPix[1].y);

    return REACTION_NONE;
}

/*
 * Copy this ROI to another Gframe
 */
Roi *Box::copy(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return NULL;
    }
    Roi *roi;
    roi = new Box(gframe, magnetCoords);
    return roi;
}

/*
 * Get the distance from a given point to any side of this box.
 * If "far" >= 0, can return "far" if actual distance is > "far".
 */
double Box::distanceFrom(int x, int y, double far) {
    if (far > 0&&(x > x_max + far || x < x_min - far ||y > y_max + far || y
            < y_min - far)) {
        return far+1; // Return something farther away
    }

    double d1, d2;

    d1 = distanceFromLine(x, y, pntPix[0].x, pntPix[0].y, pntPix[1].x,
            pntPix[0].y, far);
    d2 = distanceFromLine(x, y, pntPix[1].x, pntPix[0].y, pntPix[1].x,
            pntPix[1].y, far);
    d1 = min(d1, d2);
    d2 = distanceFromLine(x, y, pntPix[1].x, pntPix[1].y, pntPix[0].x,
            pntPix[1].y, far);
    d1 = min(d1, d2);
    d2 = distanceFromLine(x, y, pntPix[0].x, pntPix[1].y, pntPix[0].x,
            pntPix[0].y, far);
    return min(d1, d2);
}

/*
 * Draw a box.
 */
void Box::draw(void) {
    if (pntPix[0].x == ROI_NO_POSITION  || !drawable || !isVisible()) {
        return;
    }
    //calc_xyminmax();

    if (created_type != ROI_SELECTOR) {
        setClipRegion(FRAME_CLIP_TO_IMAGE);
    }

    if (visibility != VISIBLE_NEVER && isVisible()) {
        GraphicsWin::drawRect((int)x_min, (int)y_min, (int)(x_max - x_min),
                (int)(y_max - y_min), my_color);
    }
    roi_set_state(ROI_STATE_EXIST);

    if (roi_state(ROI_STATE_MARK) || rolloverHandle >= 0) {
        mark();
    }
    setClipRegion(FRAME_NO_CLIP);

    if(getReal("aipShowROIPos", 0) > 0 && getReal("aipShowROIOpt", 0) > 0) { // show intensity
      showIntensity();
    }

    drawRoiNumber();
}

/*
 *  Check whether a box is selected or not.
 *  There are two possiblities:
 *      1. Positioning a cursor at either corner will RESIZE a box.
 *  2. Positioning a cursor inside a box will MOVE a box.
 *  If a box is selected, it will set the 'acttype' variable.
 *  Return TRUE or FALSE.
 */
bool Box::is_selected(short x, short y) {
    // ----- Check to RESIZE a box -----
    // Check for the corner of a box.  If the cursor is close enough to
    // the end points, we will assign the opposite position of the point 
    // to variables 'basex' and 'basey' because this is the way we
    // interactively create a box.

    /* NOT USED ???? **************

     if ((force_acttype != ROI_MOVE) && resizable) {
     if (Corner_Check(x, y, pntPix[0].x, pntPix[0].y, aperture))   {
     // Upper-left
     basex = pntPix[1].x; basey = pntPix[1].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     
     } else if (Corner_Check(x, y, pntPix[1].x, pntPix[0].y, aperture))    {
     // Upper-right
     basex = pntPix[0].x; basey = pntPix[1].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     
     } else if (Corner_Check(x, y, pntPix[1].x, pntPix[1].y, aperture)) {
     // Bottom-right
     basex = pntPix[0].x; basey = pntPix[0].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     
     } else if (Corner_Check(x, y, pntPix[0].x, pntPix[1].y, aperture))  {
     // Bottom-left
     basex = pntPix[1].x; basey = pntPix[0].y;
     acttype = ROI_RESIZE;
     return(TRUE);
     }
     }
     if (com_point_in_rect(x,y, pntPix[0].x,pntPix[0].y, pntPix[1].x,pntPix[1].y))  {
     // ----- Check to MOVE a box ------
     acttype = ROI_MOVE;
     return(TRUE);
     }
     */

    // ----- Box is not selected ------
    return (false);
}

/*
 * Figure out which handle, if any, we are near.
 */
int Box::getHandle(int x, int y) {
    double dx = x_min - x;
    double dx2min = dx * dx;
    dx = x_max - x;
    double dx2max = dx * dx;
    double dy = y_min - y;
    double dy2min = dy * dy;
    dy = y_max - y;
    double dy2max = dy * dy;

    double minDist;
    int minHandle;
    if (dx2min < dx2max) {
        // Corner 0 or 3 is closest
        if (dy2min < dy2max) {
            // Corner 0 is closest
            minHandle = 0;
            minDist = dx2min + dy2min;
        } else {
            // Corner 3 is closest
            minHandle = 3;
            minDist = dx2min + dy2max;
        }
    } else {
        // Corner 1 or 2 is closest
        if (dy2min < dy2max) {
            // Corner 1 is closest
            minHandle = 1;
            minDist = dx2max + dy2min;
        } else {
            // Corner 2 is closest
            minHandle = 2;
            minDist = dx2max + dy2max;
        }
    }

    if (minDist >= aperture * aperture) {
        return -1;
    } else {
        return minHandle;
    }
}

/*
 * Mark the box.
 */
void Box::mark(void) {
    if (!markable)
        return;

    int i;
    int saveColor = my_color;
    if (!roi_state(ROI_STATE_MARK) && rolloverHandle >= 0) {
        // Draw only the active mark
        my_color = rollover_color;
        int x = ( (rolloverHandle == 0) || (rolloverHandle == 3) ) ? (int) x_min : (int) x_max;
        int y = (rolloverHandle < 2) ? (int) y_min : (int) y_max;
        draw_mark(x, y);
        my_color = saveColor;
    } else {
        // Draw all the marks
        for (i=0; i<4; i++) {
            if (rolloverHandle == i) {
                my_color = rollover_color;
            }
            int x = ( (i == 0) || (i == 3) ) ? (int) x_min : (int) x_max;
            int y = (i < 2) ? (int) y_min : (int) y_max;
            draw_mark(x, y);
            my_color = saveColor;
        }
    }
}

/*
 *  Save the current ROI box endpoints in the following format:
 *
 *     # <comments>
 *     Line
 *     X1 Y1
 *     X2 Y2
 *
 *  where
 *        # indicates comments
 *   Line is the ROI "name"
 *        X1 Y1 is one point
 *        X2 Y2 is the opposite point
 */
void Box::save(ofstream &outfile, bool pixflag) {
    outfile << name() << "\n";
  if(pixflag) {
    outfile << pntData->coords[0].x<< " "<< pntData->coords[0].y<< "\n";
    outfile << pntData->coords[1].x<< " "<< pntData->coords[1].y<< "\n";
  } else {
    outfile << magnetCoords->coords[0].x<< " "<< magnetCoords->coords[0].y<< " "<< magnetCoords->coords[0].z <<"\n";
    outfile << magnetCoords->coords[1].x<< " "<< magnetCoords->coords[1].y<< " "<< magnetCoords->coords[1].z <<"\n";
  }
}

/*
 * Returns the address of the first data pixel in this ROI.
 * Initializes variables needed by NextPixel() to step through all the
 * pixels in the ROI.
 */
float *Box::firstPixel() {
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    if (img == nullImg) {
        return NULL;
    }
    spDataInfo_t di = img->getDataInfo();
    data_width = di->getFast();
    int x0 = (int)(0.5 + min(pntData->coords[0].x, pntData->coords[1].x));
    int x1 = (int)(0.5 + max(pntData->coords[0].x, pntData->coords[1].x));
    int y0 = (int)(0.5 + min(pntData->coords[0].y, pntData->coords[1].y));
    int y1 = (int)(0.5 + max(pntData->coords[0].y, pntData->coords[1].y));
    beg_of_row = (di->getData() + y0 * data_width + x0); // First pixel in row
    roi_width = x1 - x0; // # of columns in ROI
    end_of_row = beg_of_row + roi_width - 1; // Last ROI pixel in row
    roi_height = y1 - y0; // # of rows in ROI
    row = 1;
    if (roi_width < 1|| roi_height < 1) {
        return 0;
    } else {
        return (data = beg_of_row);
    }
}

/*
 * After initialization by calling FirstPixel(), each call to NextPixel()
 * returns the address of the next data pixel that is inside this ROI.
 * Successive calls to NextPixel() step through all the data in the ROI.
 * If no pixels are left, returns 0.
 */
float *Box::nextPixel() {
    if (data < end_of_row) {
        return ++data;
    } else if (row < roi_height) {
        row++;
        data = (beg_of_row += data_width);
        end_of_row = beg_of_row + roi_width - 1;
        return data;
    } else {
        return 0;
    }
}

// ************************************************************************
//
// ************************************************************************
/*
 void Box::update_screen_coords()
 {
 pntPix[0].x = x_min = data_to_xpix(pntData->coords[0].x);
 pntPix[0].y = y_min = data_to_ypix(pntData->coords[0].y);
 pntPix[1].x = x_max = data_to_xpix(pntData->coords[1].x);
 pntPix[1].y = y_max = data_to_ypix(pntData->coords[1].y);
 }
 */

// ************************************************************************
//
// ************************************************************************
/*
 void Box::update_data_coords()
 {
 pntData->coords[0].x = xpix_to_data(pntPix[0].x);
 pntData->coords[0].y = ypix_to_data(pntPix[0].y);
 pntData->coords[1].x = xpix_to_data(pntPix[1].x);
 pntData->coords[1].y = ypix_to_data(pntPix[1].y);
 }
 */

int Box::getHandlePoint(int i, double &x, double &y) {
   if(npnts < 1 || i < 0) return 0;
   if(i == 0) {
     x=pntPix[0].x;
     y=pntPix[0].y;
     return 1;
   } else if(i == 1) {
     x=pntPix[1].x;
     y=pntPix[0].y;
     return 1;
   } else if( i == 2) {
     x=pntPix[1].x;
     y=pntPix[1].y;
     return 1;
   } else if( i == 3) {
     x=pntPix[0].x;
     y=pntPix[1].y;
     return 1;
   } else return 0;
}

