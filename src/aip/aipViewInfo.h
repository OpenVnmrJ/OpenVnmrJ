/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPVIEWINFO_H
#define AIPVIEWINFO_H


#include <list>

#include "sharedPtr.h"
#include "aipImgInfo.h"
#include "aipVsInfo.h"
#include "aipDataInfo.h"
#include "aipInterpolation.h"

class ViewInfo
{
public:
    ViewInfo(spImgInfo_t);
    ~ViewInfo();

    spImgInfo_t imgInfo;
    /*
     * The window area occupied by the image.  These are integers
     * because we don't "split pixels" on the screen.  A pixel is
     * either entirely within the image or entirely out of it.
     * Note that the screen distance between the first and last
     * pixel in a row is (pixwd - 1).
     * For 1-d data, pixht is still the height of the area drawn on.
     */
    int pixstx, pixsty;		/* Upper-left corner of image on window */
    int pixwd, pixht;		/* Number of pixels along edges of image */
    double p2d[3][3];		// Pixel to data address conversion
    double d2p[3][3];		// Data to pixel address conversion
    double p2m[4][4];           // Pixel address to magnet coord conversion
    double m2p[4][4];           // Magnet coord to pixel address conversion
    double u2m[3][4];
    double m2u[3][4];
    double pixelsPerCm;
    int pixstx_off, pixsty_off;		// offset of pixstx, pixsty relative to base image
    double cm_off[3]; 	// offset in cm of overlay image relative to base image

    XID_t draw();
    void print(double scale);
    bool pointIsOnImage(int x, int y);
    void setVs(int x, int y, bool maxOrMin);
    void pixToData(double pixX, double pixY, double& dataX, double& dataY);
    void dataToPix(double dataX, double dataY, double& pixX, double& pixY);
    void pixToUser(double pixX, double pixY, double pixZ, double& userX, double& userY, double& userZ);
    void userToPix(double userX, double userY, double userZ, double &pixX, double &pixY, double &pixZ);
    void pixToMagnet(double pixX, double pixY, double pixZ, double& mX, double& mY, double& mZ);
    void magnetToPix(double mX, double mY, double mZ, double &pixX, double &pixY, double &pixZ);
    void updateScaleFactors();
    void keepPointInData(double& x, double& y);
    rebin_t chooseRebinMethod(double numInputBins, int numOutputBins);
    bool updateViewData();
    void freeViewData();
    bool viewDataOOD();
    void setViewDataOOD(bool);
    int getRotation();		// How to rotate data to get image
    void setRotation(int rotation);
    Histogram *getHistogram() { return histogram; }
    bool isOblique();

private:
    static int vsRadius;	// Size of square to look at to set vs
				//  (should be a disc?)

    int rotation;
    ImgDataStore_t rebinnedData;
    Histogram *histogram;
};

typedef boost::shared_ptr<ViewInfo> spViewInfo_t;
extern spViewInfo_t nullView;

typedef std::list<spViewInfo_t> ViewInfoList;

#endif /* AIPVIEWINFO_H */
