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
#include <algorithm>
using std::min;
using std::max;
#include <math.h>               // For copysign()
#include <unistd.h>

#include "aipUtils.h"
#include "aipVnmrFuncs.h"
#include "aipStructs.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipLine.h"
#include "aipDataManager.h"

bool Line::firstTime = true;
bool Line::showMIP = false;
const string Line::baseFilename = "/tmp/VjLineProfile";

static const int buflen=128;
static char lengthStr[buflen] = "---";
static char startStr[buflen] = "---";
static char endStr[buflen] = "---";

/*
 * Constructor from pixel location.
 * Assume gf is valid.
 */
Line::Line(spGframe_t gf, int x, int y) :
    Roi() {
    created_type = ROI_LINE;
    init(gf, 2);
    initPix(x, y);
    pntData->name = "Line";
    magnetCoords->name="Line";
    Roi::create(gf); 
}

/*
 * Constructor from data location.
 * Assume gf is valid.
 */
Line::Line(spGframe_t gf, spCoordVector_t dpts, bool pixflag) :
    Roi() {
    created_type = ROI_LINE;
    init(gf, 2);
    if(pixflag) {
       pntData = dpts;
       setMagnetCoordsFromPixels(); // TODO: Should be set from data
    } else {
       magnetCoords = dpts;
       setDataPntsFromMagnet();
    }
    setPixPntsFromData(); // Sets pntPix, npnts, min/max
    gf->addRoi(this);
    pntData->name = "Line";
    magnetCoords->name="Line";
}

/*
 * Creator with initialization to given data location.
 * This line is intended for calculations only, so is not displayed.
 */
Line::Line(Dpoint end0, Dpoint end1) :
    Roi() {
    created_type = ROI_LINE;

    // Line needs two end points;
    pntPix.reserve(2);
    pntData->coords.reserve(2);
    pntPix[0].x = ROI_NO_POSITION; // Initialize
    npnts = 2;
    state = 0;
    visibility = VISIBLE_NEVER; // Note!
    drawable = FALSE; // Note!
    resizable = FALSE; // Note!
    pntData->coords[0].x = end0.x;
    pntData->coords[0].y = end0.y;
    pntData->coords[1].x = end1.x;
    pntData->coords[1].y = end1.y;
    pntData->name = name();
    magnetCoords->name=pntData->name;
}

/************************************************************************
 *                                  *
 *  Return a length of a line (in pixels).              *
 *                                  */
double Line::length(void) {
    int x = (int) (pntPix[1].x - pntPix[0].x);
    int y = (int) (pntPix[1].y - pntPix[0].y);

    return sqrt((double)(x * x + y * y));
}

/*
 * Update one end of a line.
 */
ReactionType Line::create(short x, short y, short constraint) {
    int dist_x = (int) (x - basex); // X distance from last mouse position
    int dist_y = (int) (y - basey); // Y distance from last mouse position
    double new_x = pntPix[rolloverHandle].x + dist_x;
    double new_y = pntPix[rolloverHandle].y + dist_y;
    int other = rolloverHandle ^ 1;
    if (constraint == movePointConstrained) {
        dist_x = (int) fabs(pntPix[other].x - new_x); // X distance from other end
        dist_y = (int) fabs(pntPix[other].y - new_y); // Y distance from other end
        // NB: 0.414 = tan(45/2)
        if (dist_x < 0.414 * dist_y) {
            // Make it vertical
            new_x = pntPix[other].x;
        } else if (dist_y < 0.414 * dist_x) {
            // Make it horizontal
            new_y = pntPix[other].y;
        } else {
            // Make it 45 degrees
            double d = sqrt((double)(dist_x * dist_x + dist_y * dist_y))
                    / 1.414;
            dist_x = (int)copysign(d, new_x - pntPix[other].x);
            dist_y = (int)copysign(d, new_y - pntPix[other].y);
            new_x = pntPix[other].x + dist_x;
            new_y = pntPix[other].y + dist_y;
        }
        double imgX0, imgY0, imgX1, imgY1;
        getImageBoundsInPixels(imgX0, imgY0, imgX1, imgY1);
        clipLineToRect(pntPix[other].x, pntPix[other].y, new_x, new_y, imgX0,
                imgY0, imgX1, imgY1);
    } else if (constraint == movePoint) {
        keep_point_in_image(&new_x, &new_y);
    } else {
        return REACTION_NONE;
    }

    if (new_x == pntPix[rolloverHandle].x&&new_y == pntPix[rolloverHandle].y) {
        // Same position, do nothing
        return REACTION_NONE;
    }
    basex += new_x - pntPix[rolloverHandle].x;
    basey += new_y - pntPix[rolloverHandle].y;

    erase(); // Erase previous line

    // Update the point
    setPixelCoord(rolloverHandle, new_x, new_y);

    // Assign minmax values
    x_min = min(pntPix[0].x, pntPix[1].x);
    x_max = max(pntPix[0].x, pntPix[1].x);
    y_min = min(pntPix[0].y, pntPix[1].y);
    y_max = max(pntPix[0].y, pntPix[1].y);

    draw(); // Draw a new line
    //update_data_coords();
    someInfo(true);
    updateSlaves(true);
    return REACTION_NONE;
}

/*
 * Move a line to a new location.
 */
ReactionType Line::move(short x, short y) {
    short dist_x = (short) (x - basex); // distance x
    short dist_y = (short) (y - basey); // distance y

    keep_roi_in_image(&dist_x, &dist_y);

    // Same position, do nothing
    if ((!dist_x) && (!dist_y)) {
        return REACTION_NONE;
    }

    erase(); // Erase previous line

    // Update line's pixel, data & magnet coords
    setPixelCoord(0, pntPix[0].x + dist_x, pntPix[0].y + dist_y);
    setPixelCoord(1, pntPix[1].x + dist_x, pntPix[1].y + dist_y);

    x_min += dist_x;
    x_max += dist_x;
    y_min += dist_y;
    y_max += dist_y;

    draw(); // Draw a line

    basex += dist_x;
    basey += dist_y;

    someInfo(true);
    updateSlaves(true);

    return REACTION_NONE;
}

/************************************************************************
 *                                  *
 *  User just stops moving.                     *
 *                                  */
/* Never gets called?? */
ReactionType Line::move_done(short, short) {
    basex = ROI_NO_POSITION;

    // Assign minmax value for a line
    x_min = min(pntPix[0].x, pntPix[1].x);
    x_max = max(pntPix[0].x, pntPix[1].x);
    y_min = min(pntPix[0].y, pntPix[1].y);
    y_max = max(pntPix[0].y, pntPix[1].y);

    someInfo(false);

    return REACTION_NONE;
}

/*
 * Copy this ROI to another Gframe
 */
Roi *Line::copy(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return NULL;
    }
    Roi *roi;
    roi = new Line(gframe, magnetCoords);
    return roi;
}

/*
 * Get the distance from a given point to this line.
 * If "far" >= 0, can return "far" if actual distance is > "far".
 */
double Line::distanceFrom(int x, int y, double far) {
    if (far > 0&&(x > x_max + far || x < x_min - far ||y > y_max + far || y
            < y_min - far)) {
        return far+1; // Return something farther away
    }
    return distanceFromLine(x, y, pntPix[0].x, pntPix[0].y, pntPix[1].x,
            pntPix[1].y, far);
}

/*
 *  Check whether a line is selected or not.
 *  There are two possiblities:
 *     1. Positioning a cursor at the end points will RESIZE a line.
 *     2. Positioning a cursor close to the line (but not at end points)
 *        will MOVE a line.
 *        The algorithm used to select a line is to find the
 *        perpendicular distance from a current position to the line.
 *        If it is less than "aperture", it means that a line is
 *        selected.
 *  If the line is selected, if will set the 'acttype' variable.
 *  Return true or false.
 */
bool Line::is_selected(short x, short y) {
    // Check whether or not a line is on the screen
    /*
     if (!roi_state(ROI_STATE_EXIST))
     return(FALSE);
     */

    // ---- Check for RESIZE -----
    // Check the end points of a line.  If the cursor is close enough to
    // the end points, we will assign the opposite position of the point
    // to variables 'basex' and 'basey' because this is the way we
    // interactively create a line.
    if (force_acttype != ROI_MOVE) {
        if ((fabs(pntPix[0].x - x) < aperture) &&(fabs(pntPix[0].y - y)
                < aperture)) {
            basex = pntPix[1].x;
            basey = pntPix[1].y;
            acttype = ROI_RESIZE;
            return true;
        } else if ((fabs(pntPix[1].x - x) < aperture) &&(fabs(pntPix[1].y - y)
                < aperture)) {
            basex = pntPix[0].x;
            basey = pntPix[0].y;
            acttype = ROI_RESIZE;
            return true;
        }
    }

    // ----- Check for MOVE -----
    // Check if x and y point is within the range of a line
    if (distanceFrom(x, y, aperture) < aperture) {
        acttype = ROI_MOVE;
        return (true);
    }

    // ----- Line is not selected ------
    return (false);
}

/*
 *  Save the current ROI line endpoints into the following format:
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
 *        X2 Y2 is the other point
 */
void Line::save(ofstream &outfile, bool pixflag) {
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
 * Once used to set/show/whatever a pixel, now puts the coordinates
 * in the Ipoint array passed to it at the location passed to it
 */
void Line::SetPixel(short x, short y, Ipoint *result, short result_index) {
    result[result_index].x = x ;
    result[result_index].y = (float)y ;
}

/*
 * Initializes variables needed to step through all the points on a line.
 * Returns address of first pixel in line.
 */
/* STATIC */
float *Line::InitIterator(int width, // Width of data set
        int height, // Height of data set
        float x0, // One end of line on data
        float y0, float x1, // Other end of line on data
        float y1, float *dat, // Beginning of data
        double *ds, // Out: Stepsize in shorter direction
        int *np, // Out: Number of pixels in line
        int *ip, // Out: Which step we are on (=1)
        int *step0, // Out: Increment through data always done
        int *step1, // Out: Increment through data sometimes done
        double *stest) // Out: Test for optional step

{
    /* (Commented out to scan in direction line was drawn.  This is
     how MIP profiles and polyline profiles work.)
     if (x0 > x1 || (x0 == x1 && y0 > y1)) {
     // Scan left-to-right (or top-to-bottom)
     float t;
     t = x0; x0 = x1; x1 = t;
     t = y0; y0 = y1; y1 = t;
     }
     */
    int first_x_pix = (int)x0;
    int first_y_pix = (int)y0;
    int nx = (int)x1 - first_x_pix;
    int ny = (int)y1 - first_y_pix;
    double eps = 1e-6;

    // Make sure points are inside data
    if (x0 < 0) {
        x0 = 0;
    } else if (x0 >= width) {
        x0 = width - eps;
    }
    if (y0 < 0) {
        y0 = 0;
    } else if (y0 >= height) {
        y0 = height - eps;
    }
    if (x1 < 0) {
        x1 = 0;
    } else if (x1 >= width) {
        x1 = width - eps;
    }
    if (y1 < 0) {
        y1 = 0;
    } else if (y1 >= height) {
        y1 = height - eps;
    }

    if (abs(nx) >= abs(ny)) {
        *np = abs(nx) + 1;
        float dx = fabs(x1 - x0);
        if (dx == 0) {
            // We get this case if line has zero length.
            dx = 1.0;
        }
        *ds = (y1 - y0) / dx;

        *stest = (y0 + *ds * (first_x_pix + 0.5- x0));
        if (*ds != 0&& *stest + *ds * (*np - 1)< 0) {
            // *ds must be negative, and we're going too far
            *np = (int)(1 - *stest / *ds - eps);
        } else if (*ds != 0&& *stest + *ds * (*np - 1)> height - 1) {
            // *ds must be positive, and we're going too far
            *np = (int)((height - 1- *stest) / *ds + 1- eps);
        }
        first_y_pix = (int)*stest;
        *stest -= first_y_pix;
        if (nx > 0) {
            *step0 = 1;
        } else {
            *step0 = -1;
        }
        if (ny >= 0) {
            *step1 = width;
        } else {
            *step1 = -width;
        }
    } else {
        *np = abs(ny) + 1;
        *ds = ((x1 - x0) / fabs(y1 - y0));
        *stest = (x0 + *ds * (first_y_pix + 0.5- y0));
        if (*ds != 0&& *stest + *ds * (*np - 1)< 0) {
            // *ds must be negative, and we're going too far
            *np = (int)(1 - *stest / *ds - eps);
        } else if (*ds != 0&& *stest + *ds * (*np - 1)> width - 1) {
            // *ds must be positive, and we're going too far
            *np = (int)((width - 1- *stest) / *ds + 1- eps);
        }
        first_x_pix = (int)*stest;
        *stest -= first_x_pix;

        if (ny > 0) {
            *step0 = width;
        } else {
            *step0 = -width;
        }
        if (nx >= 0) {
            *step1 = 1;
        } else {
            *step1 = -1;
        }
    }

    *ip = 1;
    if (*ds < 0) {
        *ds = -*ds;
        *stest = 1 - *stest;
    }
    return dat + first_x_pix + width * first_y_pix;
}

/*
 * Returns the address of the first data pixel in this ROI.
 * Initializes variables needed by NextPixel() to step through all the
 * pixels in the ROI.
 */
float *Line::firstPixel() {
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    if (img == nullImg) {
        return NULL;
    }
    spDataInfo_t di = img->getDataInfo();
    data = InitIterator(di->getFast(), di->getMedium(), pntData->coords[0].x,
            pntData->coords[0].y, pntData->coords[1].x, pntData->coords[1].y,
            di->getData(), &pix_step, &npix, &ipix, &req_data_step,
            &opt_data_step, &test);
    return data;
}

/*
 * After initialization by calling FirstPixel(), each call to NextPixel()
 * returns the address of the next data pixel that is inside this ROI.
 * Successive calls to NextPixel() step through all the data in the ROI.
 * If no pixels are left, returns 0.
 */
float *Line::nextPixel() {
    if (ipix >= npix) {
        return 0;
    } else {
        ipix++;
        data += req_data_step;
        if ((test += pix_step) > 1) {
            data += opt_data_step;
            test--;
        }
        return data;
    }
}

/************************************************************************
 *                                  *
 * Ramani's Bresenham implementation. 
 *                                  */
void Line::SlowLine(short x1, short y1, short x2, short y2, Ipoint * result) {
    int dx, dy, xi, yi, xsi, ysi, r, temp, runlimit, rbig, rsmall;
    short result_index = 0; /* next element of result to be filled in */

#define PixelWidth 1

    /* printf("\nLine: x1 = %d  y1 = %d  x2 = %d  y2 = %d ",x1,y1,x2,y2); */
    if (x2 < x1 && y2 < y1) {
        temp = x1;
        x1 = x2;
        x2 = temp;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    xsi = x1;
    ysi = y1;
    dx = x2 - x1;
    dy = y2 - y1;

    if (abs(dx) >= abs(dy)) {
        rbig = 2 * dx - 2 * dy;
        rsmall = 2 * dy;
        if (dy >= 0&& dx >= 0) {
            xi = x1;
            runlimit = dx + x1;
            r = 2 * dy - dx;
            for (; xi <= runlimit; xi += PixelWidth) {
                Line::SetPixel(xi, ysi, result, result_index++);
                if (r >= 0) {
                    ysi += PixelWidth;
                    r -= rbig;

                } else {
                    r += rsmall;
                }
            }
            return;
        }
        if (dy <= 0&& dx >= 0) {
            dy = -dy;
        }
        if (dy >= 0&& dx <= 0) {
            xi = x2;
            dx = -dx;
            ysi = y2;
            runlimit = dx + x2;
        } else {
            xi = x1;
            runlimit = dx + x1;
        }
        r = 2 * dy - dx;
        for (; xi <= runlimit; xi += PixelWidth) {
            Line::SetPixel(xi, ysi, result, result_index++);
            if (r >= 0) {
                ysi -= PixelWidth;
                r -= 2 * dx - 2 * dy;
            } else {
                r += 2 * dy;
            }
        }
    } else {
        if (dx >= 0&& dy >= 0) {
            r = 2 * dx - dy;
            for (yi = y1; yi <= dy + y1; yi += PixelWidth) {
                Line::SetPixel(xsi, yi, result, result_index++);
                if (r >= 0) {
                    xsi += PixelWidth;
                    r -= 2 * dy - 2 * dx;
                } else {
                    r += 2 * dx;
                }
            }
            return;
        }
        if ((dx <= 0&& dy >= 0) || (dx >= 0&& dy <= 0)) {
            if (dy < 0) {
                dy = -dy;
                xsi = x2;
            }
            if (dx < 0) {
                dx = -dx;
                yi = y1;
            } else {
                yi = y2;
                xsi = x2;
            }
            r = 2 * dx - dy;
            runlimit = dy + yi;
            for (; yi <= runlimit; yi += PixelWidth) {
                Line::SetPixel(xsi, yi, result, result_index++);
                if (r >= 0) {
                    xsi -= PixelWidth;
                    r -= 2 * dy - 2 * dx;
                } else {
                    r += 2 * dx;
                }
            }
            return;
        }
    }
}

/* STATIC */
void Line::clearInfo() {
    RoiManager::infoLine = NULL;
    strcpy(lengthStr, "Length:");
    setString("aipProfileLengthMsg", lengthStr, true);
    strcpy(startStr, "Start:");
    setString("aipProfileDataCoordsStartMsg", startStr, true);
    strcpy(endStr, "End:");
    setString("aipProfileDataCoordsEndMsg", endStr, true);
    drawProfile(NULL, 0, 0, " ");
    setReal("aipLineInfoNumber", 0, true);
}

/*
 * Calculate some line info
 */
void Line::someInfo(bool isMoving, bool clear) {
    const int buflen = 100;

    int i; // counter
    int num_points; // number of data points underneath this line
    float *pdata; // pointer to image data  (intensities)

    if (isMoving && getReal("aipInfoUpdateOnMove", 0) == 0) {
        return; // Don't update while still dragging ROI
    }

    if (firstTime) {
        deleteOldFiles(baseFilename);
        firstTime = false;
    }
    // Do not do anything if we are not the active ROI
    //if (RoiManager::get()->getFirstActive() != this) {
    //return;
    //}

    // Remember that we are the line to update
    if (clear) {
        RoiManager::infoLine = NULL;
    } else {
        RoiManager::infoLine = this;
    }
    setReal("aipLineInfoNumber", getRoiNumber(), true);

    spImgInfo_t img = pOwnerFrame->getFirstImage();
    spDataInfo_t di = img->getDataInfo();

    // Calculate and print out the line length
    double wd = di->getSpan(0);
    double ht = di->getSpan(1);
    double xscale = wd / di->getFast();
    double yscale = ht / di->getMedium();
    double lenx = pntData->coords[1].x - pntData->coords[0].x;
    double lenxcm = lenx * xscale;
    double leny = pntData->coords[1].y - pntData->coords[0].y;
    double lenycm = leny * yscale;
    double line_len = (double) sqrt((lenxcm*lenxcm) + (lenycm*lenycm));

    char buf[buflen];

    if (clear) {
        sprintf(buf, "Length:");
    } else {
        sprintf(buf, "Length: %.3f cm", line_len);
    }
    if (strcmp(buf, lengthStr)) {
        strcpy(lengthStr, buf);
        setString("aipProfileLengthMsg", buf, true);
    }

    // Print the line end coords.
    if (clear) {
        sprintf(buf, "Start:");
    } else {
        sprintf(buf, "Start: (%.2f, %.2f)", pntData->coords[0].x,
                pntData->coords[0].y);
    }
    if (strcmp(buf, startStr)) {
        strcpy(startStr, buf);
        setString("aipProfileDataCoordsStartMsg", buf, true);
    }
    if (clear) {
        sprintf(buf, "End:");
    } else {
        sprintf(buf, "End: (%.2f, %.2f)", pntData->coords[1].x,
                pntData->coords[1].y);
    }
    if (strcmp(buf, endStr)) {
        strcpy(endStr, buf);
        setString("aipProfileDataCoordsEndMsg", buf, true);
    }

    int x0 = (int)pntData->coords[0].x;
    int x1 = (int)pntData->coords[1].x;
    int y0 = (int)pntData->coords[0].y;
    int y1 = (int)pntData->coords[1].y;
    num_points = abs(x1 - x0);
    if (abs(y1 - y0) > num_points ) {
        num_points = abs(y1 - y0);
    }
    num_points++;

    float *project = 0;
    string ylabel = "Intensity";
    showMIP = getReal("aipProfileMIP", 0) != 0;
    if (!clear && showMIP == false) {
        project = new float[num_points];
        for (pdata=firstPixel(), i=0; pdata && i<num_points; pdata=nextPixel()) {
            project[i++] = *pdata;
        }
    } else if (!clear && !isMoving) { // showMIP == true
        ylabel = "MIP";
        // Increase number of points, so we don't miss any pixels.
        num_points *= 2;
        project = new float[num_points];
        // Step through "num_points" equally spaced points along the line.
        Dpoint orig;
        Dpoint slope;
        Dpoint clip[2];
        Dpoint ends[2];
        float dx = lenx / (num_points - 1);
        float dy = leny / (num_points - 1);
        orig.x = pntData->coords[0].x;
        orig.y = pntData->coords[0].y;
        slope.x = -lenycm / xscale; // Slope of normal to the line
        slope.y = lenxcm / yscale; //  in pixel units.
        clip[0].x = clip[0].y = 0;
        clip[1].x = di->getFast() - 1;
        clip[1].y = di->getMedium() - 1;

        float t;
        for (i=0; i<num_points; i++, orig.x += dx, orig.y += dy) {
            // Erect a normal to the line at this (orig) location that
            // just fits in the data space.  The slope of the normal is
            // (-lenx/leny).  The following routine returns the endpoints
            // of the required line.
            extend_line(orig, // A point on the line
                    slope, // Slope of the line
                    clip, // Clipping corners
                    ends); // Returns endpoints
            // Find the maximum value along the line.
            Line *line = new Line(ends[0], ends[1]);
            line->pOwnerFrame = pOwnerFrame;
            t = *(pdata = line->firstPixel());
            for (; pdata; pdata=line->nextPixel()) {
                if (*pdata > t) {
                    t = *pdata;
                }
            }
            project[i] = t;
            delete line;
        }
    }
    drawProfile(project, num_points, line_len, ylabel);
    delete[] project;
}

/* STATIC */
void Line::drawProfile(float *y, int npts, double len, string ylabel) {
    const int buflen = 100;
    static int index = 0;

    if (y && len <= 0) {
        y = NULL;
    }

    // Open file
    char fname[128];
    sprintf(fname, "%s%d.%d", baseFilename.c_str(), getpid(), index);
    index++;
    index %= 2;
    FILE *fd = fopen(fname, "w");
    if (!fd) {
        fprintf(stderr,"Cannot open file for line profile: %s\n", fname);
        return;
    }

    char buf[buflen];
    if (y == NULL) {
        // Write out header
        fprintf(fd, "XLabel Distance (cm)\n");
        fprintf(fd, "YLabel %s\n", ylabel.c_str());
        //fprintf(fd,"YMin 0\n");
        //fprintf(fd,"YMax getReal("aipProfileMaxY", 0)\n");
        fprintf(fd, "NPoints 0\n");
        fprintf(fd, "Data\n");
        sprintf(buf, "Max:");
    } else {
        double ymax;
        // Set scale
        ymax = y[0];
        for (int i=0; i<npts; ++i) {
            if (ymax < y[i]) {
                ymax = y[i];
            }
        }
        double graphYmax = ymax;
        if (::isActive("aipProfileMaxY")) {
            graphYmax = max(ymax, getReal("aipProfileMaxY", 0));
        }
        setReal("aipProfileMaxY", ymax, false); // No notification

        // Write out header
        //fprintf(fd,"YMin 0\n");
        //fprintf(fd,"YMax %g\n", graphYmax);
        string showZero;
        showZero = getString("aipProfileShowZero", "y");
        if (showZero[0] == 'y' || showZero[0] == 'Y') {
            fprintf(fd,"ShowZeroY\n");
        }
        fprintf(fd,"XLabel Distance (cm)\n");
        fprintf(fd,"YLabel %s\n", ylabel.c_str());
        fprintf(fd,"NPoints %d\n", npts);
        fprintf(fd,"Data\n");
        sprintf(buf, "Max: %.4g", ymax);

        // Write out the data
        double x = 0;
        double dx = len / (npts - 1);
        for (int i=0; i<npts; ++i, x += dx) {
            fprintf(fd,"%g %g\n", x, y[i]);
        }
    }
    fprintf(fd,"EndData\n");
    fclose(fd);

    // Set parameters
    setString("aipProfileFile", fname, true);// Send notification of change
    static char maxStr[buflen] = "---";
    if (strcmp(buf, maxStr)) {
        strcpy(maxStr, buf);
        setString("aipProfileMaxMsg", buf, true);
    }
}

/*
 * Handy distance finding function for "extend_line()".
 */
static inline float distsq(Dpoint p0, Dpoint p1) {
    float t1 = p0.x - p1.x;
    float t2 = p0.y - p1.y;
    return (t1 * t1 + t2 * t2);
}

// ************************************************************************
// Takes a line specified by a point and slope and
// returns the end points of the line segment that is inside
// a specified clipping rectangle.
// Assumes clip[0].x < clip[1].x and clip[0].y < clip[1].y
// Assumes the line passes through the clipping rectangle.
// ************************************************************************
void extend_line(Dpoint origin, // A point on the line
        Dpoint slope, // The slope of the line
        Dpoint *clip, // Two corners of clipping rectangle
        Dpoint *endpts) // Returns the two end points
{
    int i;
    int endindex = 0;
    // Prepare to find up to four end points.  (If the line passes
    // through two opposite corners of the clip rectangle, we find
    // each end point twice.)
    Dpoint end[4];

    // First, deal with special cases of vertical and horizontal lines.
    if (slope.x == 0) {
        endpts[0].x = endpts[1].x = origin.x;
        endpts[0].y = clip[0].y;
        endpts[1].y = clip[1].y;
        return;
    }

    if (slope.y == 0) {
        endpts[0].y = endpts[1].y = origin.y;
        endpts[0].x = clip[0].x;
        endpts[1].x = clip[1].x;
        return;
    }

    // Now deal with the general case.
    float xcut, ycut;
    for (i=0; i<2; i++) {
        xcut = origin.x + (clip[i].y - origin.y) * slope.x/ slope.y;
        if (xcut >= clip[0].x&& xcut <= clip[1].x) {
            // Line crosses the y clip line between the x clip lines.
            end[endindex].x = xcut;
            end[endindex].y = clip[i].y;
            endindex++;
        }
        ycut = origin.y + (clip[i].x - origin.x) * slope.y/ slope.x;
        if (ycut >= clip[0].y&& ycut <= clip[1].y) {
            end[endindex].x = clip[i].x;
            end[endindex].y = ycut;
            endindex++;
        }
    }
    if (endindex == 0) {
        // Line does not intersect clip rectangle.
        fprintf(stderr,"Internal error in extend_line()");
        end[0].x = end[0].y = 0;
    }
    endpts[0] = end[0];
    if (endindex <= 1) {
        endpts[1] = endpts[0];
    } else {
        // We have found more than one end point.
        // We will return the first one, and the one that is farthest from
        // that one.
        float t;
        endpts[1] = end[1];
        float d = distsq(end[0], end[1]);
        for (i=2; i<endindex; i++) {
            if ( (t=distsq(end[0], end[i])) > d) {
                d = t;
                endpts[1] = end[i];
            }
        }
    }
    return;
}

// ************************************************************************
//
// ************************************************************************
/*
 void Line::update_screen_coords()
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
 void Line::update_data_coords()
 {
 pntData->coords[0].x = xpix_to_data(pntPix[0].x);
 pntData->coords[0].y = ypix_to_data(pntPix[0].y);
 pntData->coords[1].x = xpix_to_data(pntPix[1].x);
 pntData->coords[1].y = ypix_to_data(pntPix[1].y);
 }
 */

