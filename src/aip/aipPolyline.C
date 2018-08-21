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
//#include <vector>
//using std::vector;

#include "aipUtils.h"
#include "aipVnmrFuncs.h"
#include "aipStructs.h"
#include "aipGraphicsWin.h"
#include "aipGframe.h"
#include "aipGframeManager.h"
#include "aipRoi.h"
#include "aipRoiManager.h"
#include "aipLine.h"
#include "aipPolygon.h"
#include "aipPolyline.h"

bool Polyline::firstTime = true;

/*
 * Constructor from pixel location.
 * Assume gf is valid.
 */
Polyline::Polyline(spGframe_t gf, int x, int y) :
    Polygon() {
    init(gf, 2);
    created_type = ROI_POLYGON_OPEN;
    closed = false;
    initEdgelist();
    initPix(x, y);
    lastx = (int) basex;
    lasty = (int) basey;
    pntData->name = "Polyline";
    magnetCoords->name="Polyline";
    Roi::create(gf);
}

/*
 * Constructor from data location.
 * Assume gf is valid.
 */
Polyline::Polyline(spGframe_t gf, spCoordVector_t dpts, bool pixflag) :
    Polygon() {
    init(gf, dpts->coords.size());
    created_type = ROI_POLYGON_OPEN;
    closed = false;
    initEdgelist();
    if(pixflag) {
       pntData = dpts;
       setMagnetCoordsFromPixels(); // TODO: Should be set from data
    } else {
       magnetCoords = dpts;
       setDataPntsFromMagnet();
    }
    setPixPntsFromData(); // Sets pntPix, npnts, min/max
    gf->addRoi(this);
    pntData->name = "Polyline";
    magnetCoords->name="Polyline";
}

/*
 * FindDuplicateVertex returns the index of the first vertex that is
 * "near" its successor vertex.  Near is defined to be within the
 * specified "tolerance".
 */
int Polyline::findDuplicateVertex(int tolerance) {
    for (int i = 1; i < npnts; i++) {
        int j = i - 1; // Previous point
        if (fabs(pntPix[i].x - pntPix[j].x) <= tolerance &&fabs(pntPix[i].y
                - pntPix[j].y) <= tolerance) {
            //printf("duplicate vertex[%d] \n", i);
            return i;
        }
    }
    return -1;
}

/*
 * Copy this ROI to another Gframe
 */
Roi *Polyline::copy(spGframe_t gframe) {
    if (gframe == nullFrame) {
        return NULL;
    }
    Roi *roi;
    roi = new Polyline(gframe, magnetCoords);
    return roi;
}

/*
 * Draw the polyline.
 */
void Polyline::draw() {
    Roi::draw();
}

/*
 * Returns the address of the first data pixel in this ROI.
 * Initializes variables needed by NextPixel() to step through all the
 * pixels in the ROI.
 */
float *Polyline::firstPixel() {
    spImgInfo_t img = pOwnerFrame->getFirstImage();
    if (img == nullImg || npnts < 2) {
        return NULL;
    }
    spDataInfo_t di = img->getDataInfo();
    data = Line::InitIterator(di->getFast(), di->getMedium(),
            pntData->coords[0].x, pntData->coords[0].y, pntData->coords[1].x,
            pntData->coords[1].y, di->getData(), &pix_step, &npix, &ipix,
            &req_data_step, &opt_data_step, &test);
    ivertex = 1;
    return data;
}

/*
 * After initialization by calling FirstPixel(), each call to NextPixel()
 * returns the address of the next data pixel that is inside this ROI.
 * Successive calls to NextPixel() step through all the data in the ROI.
 * If no pixels are left, returns 0.
 */
float *Polyline::nextPixel() {
    if (ipix >= npix) {
        if (++ivertex >= npnts) {
            return 0;
        } else {
            spDataInfo_t di = pOwnerFrame->getFirstImage()->getDataInfo();
            data = Line::InitIterator(di->getFast(), di->getMedium(),
                    pntData->coords[ivertex-1].x, pntData->coords[ivertex-1].y,
                    pntData->coords[ivertex].x, pntData->coords[ivertex].y,
                    di->getData(), &pix_step, &npix, &ipix, &req_data_step,
                    &opt_data_step, &test);
            return data;
        }
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

/*
 * Calculate some polyline info
 */
void Polyline::someInfo(bool isMoving, bool clear) {
    const int buflen = 100;

    int i, j; // counters

    if (isMoving && getReal("aipInfoUpdateOnMove", 0) == 0) {
        return; // Don't update while still dragging ROI
    }

    if (firstTime) {
        deleteOldFiles(Line::baseFilename);
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
        if (npnts < 2) {
            return;
        }
    }

    spImgInfo_t img = pOwnerFrame->getFirstImage();
    spDataInfo_t di = img->getDataInfo();

    // Calculate and print out the line length
    double wd = di->getSpan(0);
    double ht = di->getSpan(1);
    double xscale = wd / di->getFast();
    double yscale = ht / di->getMedium();
    double lenx;
    double lenxcm;
    double leny;
    double lenycm;
    double line_len = 0;
    double *segment_len = new double [npnts]; // NB: Segment numbers start at 1
    for (i=1; i<npnts; i++) {
        lenx = pntData->coords[i].x - pntData->coords[i-1].x;
        lenxcm = lenx * xscale;
        leny = pntData->coords[i].y - pntData->coords[i-1].y;
        lenycm = leny * yscale;
        segment_len[i] = sqrt((lenxcm * lenxcm) + (lenycm * lenycm));
        line_len += segment_len[i];
    }
    char buf[buflen]; // holds text to be displayed

    static char lengthStr[buflen] = "---";
    if (clear) {
        sprintf(buf, "Length:");
    } else {
        sprintf(buf, "Length of polyline: %.4g cm", line_len);
    }
    if (strcmp(buf, lengthStr)) {
        strcpy(lengthStr, buf);
        setString("aipProfileLengthMsg", buf, true);
    }
    //setReal("aipProfileLength", line_len, true);

    // Print the line end coords.
    static char startStr[buflen] = "---";
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
    static char endStr[buflen] = "---";
    if (clear) {
        sprintf(buf, "  End:");
    } else {
        sprintf(buf, "  End: (%.2f, %.2f)", pntData->coords[npnts-1].x,
                pntData->coords[npnts-1].y);
    }
    if (strcmp(buf, endStr)) {
        strcpy(endStr, buf);
        setString("aipProfileDataCoordsEndMsg", buf, true);
    }

    float *profile= NULL;
    int num_points = 0;
    if (!clear) {
        // Calculate line profile info -- (we never do projections!)
        setReal("aipProfileMIP", 0, true);
        float *ldata = di->getData();
        int pixperrow = di->getFast();
        double scale = xscale < yscale ? xscale : yscale; // Min cm per pixel
        scale /= 2; // cm between points we sample
        num_points = (int) (line_len / scale) + 1;
        if (num_points < 2) {
            delete[] segment_len;
            return;
        }
        profile = new float[num_points];
        double x, y, s, cs, dx, dy, ds;
        cs = 0; // Current location along segment (cm)
        ds = line_len / (num_points - 1); // Distance between samples

        for (i=1, j=0; i<npnts; i++) {
            s = segment_len[i];
            if (s <= 0) {
                continue; // Skip 0 length segments
            }
            dx = (pntData->coords[i].x - pntData->coords[i-1].x) * ds / s;
            dy = (pntData->coords[i].y - pntData->coords[i-1].y) * ds / s;
            x = pntData->coords[i-1].x + cs * dx / ds;
            y = pntData->coords[i-1].y + cs * dy / ds;
            for (; cs <= s && j < num_points; j++, x += dx, y += dy, cs += ds) {
                profile[j] = *(ldata + (int)x + (int)y * pixperrow);
            }
            cs -= s;
        }
        num_points = j; // Last point could be missed (roundoff)
    }
    Line::drawProfile(profile, num_points, line_len, "Intensity");
    delete[] profile;
    delete[] segment_len;
}
