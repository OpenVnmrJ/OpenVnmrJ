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
using std::fabs;
using std::sqrt;
using std::atan2;
using std::sin;
using std::cos;
#include <algorithm>
using std::min;
using std::max;
#include <math.h>               // For rint()
//#include "aipVnmrFuncs.h"
//using namespace aip;
#include "aipUtils.h"
#include "aipStructs.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
//#include "aipRoiManager.h"
#include "aipRoi.h"
#include "aipBox.h"
#include "aipOval.h"

/*
 * Constructor from pixel location.
 * Assume gf is valid.
 */
Oval::Oval(spGframe_t gf, int x, int y) : Box() {
    created_type = ROI_OVAL;
    init(gf, 2);
    initPix(x, y);
    pntData->name = "Oval";
    magnetCoords->name = "Oval";
    Roi::create(gf);   
}

/*
 * Constructor from data location.
 * Assume gf is valid.
 */
Oval::Oval(spGframe_t gf, spCoordVector_t dpts, bool pixflag) : Box() {
    created_type = ROI_OVAL;
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
    pntData->name = "Oval";
    magnetCoords->name = "Oval";
}


/*
 * Destructor.
 */
//Oval::~Oval(void)
//{
//}

/*
 * Copy this ROI to another Gframe
 */
Roi *Oval::copy(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return NULL;
    }
    Roi *roi;
    roi = new Oval(gframe, magnetCoords);
    return roi;
}

/*
 * Draw the oval.
 */
void Oval::draw() {
    if (pntPix[0].x == ROI_NO_POSITION || !drawable || !isVisible()) {
        return;
    }

    setClipRegion(FRAME_CLIP_TO_IMAGE);
    //calc_xyminmax();

    if (visibility != VISIBLE_NEVER) {
        GraphicsWin::drawOval((int)x_min, (int)y_min, (int)(x_max-x_min)+1,
                (int)(y_max-y_min)+1, my_color);
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
 * Get the distance from a given point to the ellipse.
 * If "far" >= 0, can return anything ">far" if actual distance is ">far".
 */
double Oval::distanceFrom(int xint, int yint, double far) {
    // Distance to an ellipse is rather complicated, so we only do
    // an approximate calculation.  The goal is to get something
    // accurate to about 1 pixel when we are within several pixels
    // of the ellipse.
    if (far > 0&&(xint > x_max + far || xint < x_min - far ||yint > y_max + far
            || yint < y_min - far)) {
        return far+1; // Return something farther away
    }

    double xc = (double)(x_min + x_max) / 2;
    double yc = (double)(y_min + y_max) / 2;
    double a = (double)(x_max - x_min) / 2;
    double b = (double)(y_max - y_min) / 2;
/*
    double aa = a * a;
    double bb = b * b;
 */
    // Simplify by throwing point into 1st quadrant:
    double x = fabs(xint - xc);
    double y = fabs(yint - yc);

    // Dispose of some degenerate cases
    if (a == 0) {
        if (y <= b) {
            return x;
        } else {
            return sqrt(x * x + (y - b) * (y - b));
        }
    } else if (b == 0) {
        if (x <= a) {
            return y;
        } else {
            return sqrt((x - a) * (x - a)+ y * y);
        }
    }

    /*
     // Iterate to find distance
     double t__ = atan2(y, x);
     double sint__ = sin(t__);
     double cost__ = cos(t__);
     double r__ = sqrt(aa * sint__ * sint__ + bb * cost__ * cost__);
     r__ = a * b / r__;
     double xe = r__ * cost__;   // Point on ellipse
     double ye = r__ * sint__;
     double d = sqrt((xe - x) * (xe - x) + (ye - y) * (ye - y));
     fprintf(stderr,"d=%f\n", d);//CMP
     if (a == b) {
     // Return exact distance
     //return d;
     } else {
     do {
     double d1 = d;

     // Calculate next iteration
     // Normal to ellipse at last point
     double m = (ye * aa) / (xe * bb);// (always positive)
     double b = y - m * x; // For line w/ that slope to hit our point
     // Coefficients for quadratic eqn for next point on ellipse:
     double A = m * m + bb;
     double B = 2 * m * b;
     double C = bb * (1 - aa);
     // Next point on ellipse
     xe = xe;//CMP
     ye = sqrt((1 - (xe * xe) / aa) * bb);
     // New distance
     d = sqrt((xe - x) * (xe - x) + (ye - y) * (ye - y));
     fprintf(stderr,"d=%f\n", d);//CMP
     } while (abs(d - d1) > 0.1);
     }
     */

    // The following is exact for a circular ellipse
    double t = atan2(y, x);
    double sint = sin(t);
    double cost = cos(t);
    double r = sqrt(a * a * sint * sint + b * b * cost * cost);
    r = a * b / r;
    double dx = r * cost - x;
    double dy = r * sint - y;
    double dd1 = dx * dx + dy * dy; // Squared distance - 1st estimate

    // Check distance to the handle, also
    double dd2 = (a - x) * (a - x)+ (b - y) * (b - y); // 2nd estimate
    if (dd1 > dd2) {
        dd1 = dd2;
    }

    // For very squished ellipses, get another estimate by just
    // dropping a perpendicular down to the long axis.
    if (a/b > 1.5) {
        // Long axis is X
        if (x >= a * 0.999) {
            // Just get distance to pointy end
            dx = a - x;
            dy = y;
            dd2 = dx * dx + dy * dy;
        } else {
            dy = sqrt((1 - (x * x) / (a * a)) * b * b) - y;
            dd2 = dy * dy;
        }
    } else if (b/a > 1.5) {
        // Long axis is Y
        if (y >= b * 0.999) {
            // Just get distance to pointy end
            dx = x;
            dy = b - y;
            dd2 = dx * dx + dy * dy;
        } else {
            dx = sqrt((1 - (y * y) / (b * b)) * a * a) - x;
            dd2 = dx * dx;
        }
    } else {
        // Ellipse is pretty round, just give the circular calc.
        return sqrt(dd1);
    }

    // Return smaller estimate
    return dd1 < dd2 ? sqrt(dd1) : sqrt(dd2);
}

/*
 * Returns the address of the first data pixel in this ROI.
 * Initializes variables needed by NextPixel() to step through all the
 * pixels in the ROI.
 */
float *Oval::firstPixel() {
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    if (img == nullImg) {
        return NULL;
    }
    spDataInfo_t di = img->getDataInfo();
    data_width = di->getFast();
    double x0 = min(pntData->coords[0].x, pntData->coords[1].x);
    double x1 = max(pntData->coords[0].x, pntData->coords[1].x);
    double y0 = min(pntData->coords[0].y, pntData->coords[1].y);
    double y1 = max(pntData->coords[0].y, pntData->coords[1].y);
    m_xc = (x0 + x1) / 2;
    m_yc = (y0 + y1) / 2;
    m_a = (x1 - x0) / 2;
    m_b = (y1 - y0) / 2;
    if (m_a == 0|| m_b == 0) {
        return NULL;
    }
    row = (int)rint(y0);
    double y = (row + 0.5) - m_yc;
    double x;
    double disc = 1 - (y * y) / (m_b * m_b);
    if (disc <= 0) {
        x = 0;
    } else {
        x = m_a * sqrt(disc);
    }
    int col = (int)rint(m_xc - x);
    m_beg_of_data = di->getData();
    beg_of_row = (m_beg_of_data + row * data_width + col); // First pixel in row
    col = (int)rint(m_xc + x) - 1;
    end_of_row = (m_beg_of_data + row * data_width + col); // Last pixel in row
    m_last_row = (int)rint(y1) - 1; // Last row in ellipse
    return (data = beg_of_row);
}

/*
 * After initialization by calling FirstPixel(), each call to NextPixel()
 * returns the address of the next data pixel that is inside this ROI.
 * Successive calls to NextPixel() step through all the data in the ROI.
 * If no pixels are left, returns 0.
 */
float *Oval::nextPixel() {
    if (data < end_of_row) {
        return ++data;
    } else if (row < m_last_row) {
        ++row;
        double y = (row + 0.5) - m_yc;
        double x;
        double disc = 1 - (y * y) / (m_b * m_b);
        if (disc <= 0) {
            x = 0;
        } else {
            x = m_a * sqrt(disc);
        }
        int col = (int)rint(m_xc - x);
        beg_of_row = (m_beg_of_data + row * data_width + col);
        col = (int)rint(m_xc + x) - 1;
        end_of_row = (m_beg_of_data + row * data_width + col);
        return (data = beg_of_row);
    } else {
        return NULL;
    }
}
